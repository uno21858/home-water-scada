#include "sensors.h"

#include <algorithm>
#include <array>

#include "board.h"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mixer_watch.h"
#include "sdkconfig.h"

namespace sensors {
namespace {

constexpr const char* kTag = "sensors";
constexpr int kTimeoutMs = 50;

// F: el YF-B6 en el GPIO6. El PCNT cuenta flancos de bajada solo, con filtro
// de 10 µs (documento, pág. 39). Al llegar al tope suma y vuelve a empezar.
constexpr gpio_num_t kFlowPin = GPIO_NUM_6;
constexpr int kPcntLimit = 30000;

// Bomba R: módulo MOSFET en el RXD del header UART2. Sin bomba R el GPIO44
// no se toca: con el switch en UART1 ese pin es la salida del chip USB-serial,
// y con la consola en UART0 es su RX. Con bomba R la consola no puede ir ahí.
constexpr gpio_num_t kPumpPin = GPIO_NUM_44;
#ifdef CONFIG_HMI_HAS_MIXER
constexpr bool kHasPump = true;
#if CONFIG_ESP_CONSOLE_UART_NUM == 0
#error "La bomba R va en el GPIO44, el RX de UART0: con bomba R la consola va por el USB nativo (menuconfig > ESP-STDIO)"
#endif
#else
constexpr bool kHasPump = false;
#endif

// ADS1115 en 0x48: ±4.096 V y 128 muestras/s (documento, pág. 39)
constexpr uint8_t kAdsAddress = 0x48;
constexpr uint8_t kAdsConversion = 0x00;
constexpr uint8_t kAdsConfig = 0x01;
// Una conversión sola (bit 15) a ±4.096 V (bits 11-9 en 001), modo de una
// lectura (bit 8), 128 muestras/s (bits 7-5 en 100) y sin comparador (bits
// 1-0 en 11). Los bits 14-12 eligen la entrada contra tierra: 100 es A0.
constexpr uint16_t kAdsRead = 0x8000 | (0b001 << 9) | (1 << 8) | (0b100 << 5) | 0b11;
constexpr float kAdsVoltsPerCount = 4.096f / 32768.0f;
constexpr int kAdsChannels = 4;  // A0 P1, A1 P2, A2 nivel, A3 LDR
constexpr int kAverage = 16;

// INA219 en 0x40. La corriente sale del registro del shunt entre la
// resistencia (V / R): no usa la calibración, que solo sirve para los
// registros de corriente y potencia (0x04 y 0x03), y esos no se leen.
constexpr uint8_t kInaAddress = 0x40;
constexpr uint8_t kInaConfig = 0x00;
constexpr uint8_t kInaShunt = 0x01;  // 10 µV por cuenta, con signo, en cualquier rango
constexpr uint8_t kInaBus = 0x02;    // bits 15-3, 4 mV por cuenta
constexpr float kShuntOhms = CONFIG_HMI_INA219_SHUNT_MOHM / 1000.0f;
// 32 V (bit 13), ±320 mV con el PGA en /8 (bits 12-11 en 11), 12 bits y una
// muestra en bus y shunt (bits 10-7 y 6-3 en 0011) y los dos continuos (bits
// 2-0 en 111). Es la de fábrica, pero se escribe al arrancar: el chip se
// queda con lo que le escriban mientras tenga corriente, y una OTA o un
// reinicio de la 7B no se la quitan. Con el R100 (0.1 Ω) llega a 3.2 A.
constexpr uint16_t kInaSetup = 0x399F;

pcnt_unit_handle_t pcnt = nullptr;
i2c_master_dev_handle_t ads = nullptr;
i2c_master_dev_handle_t ina = nullptr;

// Lo que deja la tarea para latest(). Un spinlock basta: solo se copian floats.
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
Readings shared;

// La bomba R, bajo el mismo lock: set_pump() la prende y cuenta cuántas veces
// (cada vez que cambia, la tarea empieza otra ventana ciega)
bool pump_on = false;
uint32_t pump_starts = 0;

// Umbrales de menuconfig > Casa > Bomba R (INA219)
constexpr core::MixerLimits kMixerLimits{CONFIG_HMI_MIXER_MIN_VOLTS_X10 / 10.0f,
                                         CONFIG_HMI_MIXER_DRY_AMPS_X100 / 100.0f,
                                         CONFIG_HMI_MIXER_STUCK_AMPS_X100 / 100.0f};

uint32_t now_ms() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }

// Promedio de las últimas kAverage muestras de un canal
class Average {
public:
    float add(float value) {
        samples_[next_] = value;
        next_ = (next_ + 1) % kAverage;
        if (count_ < kAverage) ++count_;
        float sum = 0.0f;
        for (int i = 0; i < count_; ++i) sum += samples_[i];
        return sum / static_cast<float>(count_);
    }

private:
    std::array<float, kAverage> samples_{};
    int next_ = 0;
    int count_ = 0;
};

i2c_master_dev_handle_t add_device(uint8_t address) {
    if (!board::probe(address)) return nullptr;
    i2c_device_config_t config = {};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = address;
    config.scl_speed_hz = 400 * 1000;
    i2c_master_dev_handle_t device = nullptr;
    if (i2c_master_bus_add_device(board::i2c(), &config, &device) != ESP_OK) return nullptr;
    return device;
}

bool read16(i2c_master_dev_handle_t device, uint8_t reg, uint16_t* value) {
    uint8_t data[2] = {};
    if (i2c_master_transmit_receive(device, &reg, 1, data, sizeof data, kTimeoutMs) != ESP_OK) return false;
    *value = static_cast<uint16_t>((data[0] << 8) | data[1]);
    return true;
}

bool ads_read(int channel, float* volts) {
    const uint16_t config = kAdsRead | static_cast<uint16_t>((0b100 + channel) << 12);
    const uint8_t data[3] = {kAdsConfig, static_cast<uint8_t>(config >> 8), static_cast<uint8_t>(config & 0xFF)};
    if (i2c_master_transmit(ads, data, sizeof data, kTimeoutMs) != ESP_OK) return false;
    vTaskDelay(pdMS_TO_TICKS(10));  // 1/128 s más el 10 % que permite la hoja de datos
    uint16_t raw = 0;
    if (!read16(ads, kAdsConversion, &raw)) return false;
    *volts = static_cast<float>(static_cast<int16_t>(raw)) * kAdsVoltsPerCount;
    return true;
}

// Una lectura del INA219. La configuración va también: si algo le cambiara
// el rango, el log lo enseña.
struct InaSample {
    float bus_volts = 0.0f;
    float shunt_mv = 0.0f;  // crudo: 10 µV por cuenta
    uint16_t config = 0;
};

bool ina_read(InaSample* out) {
    uint16_t bus = 0;
    uint16_t shunt = 0;
    uint16_t config = 0;
    if (!read16(ina, kInaBus, &bus) || !read16(ina, kInaShunt, &shunt) || !read16(ina, kInaConfig, &config)) {
        return false;
    }
    out->bus_volts = static_cast<float>(bus >> 3) * 0.004f;
    out->shunt_mv = static_cast<float>(static_cast<int16_t>(shunt)) * 0.01f;
    out->config = config;
    return true;
}

float shunt_amps(float mv) { return mv / 1000.0f / kShuntOhms; }

// Le escribe kInaSetup y dice en la consola qué traía antes
void ina_setup() {
    uint16_t before = 0;
    const bool had = read16(ina, kInaConfig, &before);
    const uint8_t data[3] = {kInaConfig, static_cast<uint8_t>(kInaSetup >> 8), static_cast<uint8_t>(kInaSetup & 0xFF)};
    uint16_t after = 0;
    if (i2c_master_transmit(ina, data, sizeof data, kTimeoutMs) != ESP_OK || !read16(ina, kInaConfig, &after)) {
        ESP_LOGE(kTag, "INA219: no se pudo escribir la configuración");
        return;
    }
    ESP_LOGI(kTag, "INA219: traía la configuración 0x%04x%s, ahora 0x%04x (±320 mV: hasta %.2f A)",
             static_cast<unsigned>(before), had ? "" : " (no se pudo leer)", static_cast<unsigned>(after),
             static_cast<double>(shunt_amps(320.0f)));
}

// El shunt durante un segundo, para el log: con lo que dice el multímetro se
// compara el promedio; el mínimo y el máximo dicen cuánto se mueve
struct ShuntSecond {
    uint32_t start_ms = 0;
    int count = 0;
    float sum = 0.0f;
    float lo = 0.0f;
    float hi = 0.0f;

    void add(float mv) {
        lo = count == 0 ? mv : std::min(lo, mv);
        hi = count == 0 ? mv : std::max(hi, mv);
        sum += mv;
        ++count;
    }
};

// Lee el I2C sin parar: una vuelta por los 4 canales son ~45 ms, así que el
// promedio de 16 cubre ~0.7 s. Con la bomba R prendida, cada lectura del
// INA219 pasa por core::mixer_watch_sample: atorada o en seco solo con 500 ms
// seguidos, no con la lectura suelta que la app ve cada segundo.
void task(void*) {
    std::array<Average, kAdsChannels> average;
    std::array<float, kAdsChannels> volts{};
    core::MixerWatch watch;
    uint32_t watched_start = 0;  // la vuelta de pump_starts que vigila watch
    ShuntSecond second;
    while (true) {
        bool ads_ok = ads != nullptr;
        for (int ch = 0; ch < kAdsChannels && ads_ok; ++ch) {
            float v = 0.0f;
            ads_ok = ads_read(ch, &v);
            if (ads_ok) volts[ch] = average[ch].add(v);
        }
        InaSample ina_sample;
        const bool ina_ok = ina != nullptr && ina_read(&ina_sample);
        const float mixer_volts = ina_sample.bus_volts;
        const float mixer_amps = shunt_amps(ina_sample.shunt_mv);
        const uint32_t now = now_ms();
        if (ina_ok) second.add(ina_sample.shunt_mv);
        const bool second_done = now - second.start_ms >= 1000;

        portENTER_CRITICAL(&lock);
        const bool on = pump_on;
        const uint32_t starts = pump_starts;
        portEXIT_CRITICAL(&lock);
        core::MixerFault fault = core::MixerFault::none;
        if (on) {
            if (starts != watched_start) {
                watched_start = starts;
                core::mixer_watch_start(watch, now);
            }
            fault = core::mixer_watch_sample(watch, kMixerLimits, now, ina_ok, mixer_volts, mixer_amps);
        }

        portENTER_CRITICAL(&lock);
        shared.ads_ok = ads_ok;
        if (ads_ok) {
            // Sin divisor: R2 y R3 de 10k con C2 y C3 de 100 nF son un filtro,
            // el A0 y el A1 ven la salida del sensor (perforada, 23 oct 2026)
            shared.p1_volts = volts[0];
            shared.p2_volts = volts[1];
            shared.level_volts = volts[2];
            shared.ldr_volts = volts[3];
        }
        shared.ina_ok = ina_ok;
        if (ina_ok) {
            shared.mixer_volts = mixer_volts;
            shared.mixer_amps = mixer_amps;
            shared.mixer_shunt_mv = ina_sample.shunt_mv;
            shared.ina_config = ina_sample.config;
        }
        if (second_done && second.count > 0) {
            shared.mixer_shunt_avg_mv = second.sum / static_cast<float>(second.count);
            shared.mixer_shunt_min_mv = second.lo;
            shared.mixer_shunt_max_mv = second.hi;
        }
        // La app lo lee una vez por segundo. Atorada se queda hasta que la
        // apague, para que no se pierda entre dos lecturas; en seco es lo de
        // ahorita: si la corriente ya regresó (en banco la sacaron un momento
        // del agua) ya no cuenta. Si se apagó o se volvió a prender mientras,
        // esta lectura ya no es de esa vuelta.
        if (!pump_on || pump_starts != starts) {
            shared.mixer_fault = core::MixerFault::none;
        } else if (shared.mixer_fault != core::MixerFault::stuck) {
            shared.mixer_fault = fault;
        }
        portEXIT_CRITICAL(&lock);
        if (second_done) {
            second = ShuntSecond{};
            second.start_ms = now;
        }

        vTaskDelay(pdMS_TO_TICKS(ads == nullptr && ina == nullptr ? 1000 : 5));
    }
}

void start_flow() {
    pcnt_unit_config_t unit = {};
    unit.low_limit = -kPcntLimit;
    unit.high_limit = kPcntLimit;
    unit.flags.accum_count = 1;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit, &pcnt));

    pcnt_glitch_filter_config_t filter = {};
    filter.max_glitch_ns = 10000;
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt, &filter));

    pcnt_chan_config_t channel_config = {};
    channel_config.edge_gpio_num = kFlowPin;
    channel_config.level_gpio_num = -1;
    pcnt_channel_handle_t channel = nullptr;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt, &channel_config, &channel));
    // Cuenta en la bajada y nada en la subida
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(channel, PCNT_CHANNEL_EDGE_ACTION_HOLD,
                                                 PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    gpio_set_pull_mode(kFlowPin, GPIO_PULLUP_ONLY);

    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt, kPcntLimit));  // para que sume al llegar al tope
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt));
}

}  // namespace

void pump_off_now() {
    if (!kHasPump) return;
    gpio_set_level(kPumpPin, 0);  // primero el nivel, para que al volverse salida ya salga en bajo
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << kPumpPin;
    config.mode = GPIO_MODE_OUTPUT;
    gpio_config(&config);
    gpio_set_level(kPumpPin, 0);
}

void start() {
    start_flow();
    ads = add_device(kAdsAddress);
    ina = add_device(kInaAddress);
    if (ads == nullptr) ESP_LOGW(kTag, "El ADS1115 (0x%02x) no contesta", kAdsAddress);
    if (ina == nullptr) ESP_LOGW(kTag, "El INA219 (0x%02x) no contesta", kInaAddress);
    if (ina != nullptr) ina_setup();
    xTaskCreate(task, "sensors", 4096, nullptr, 4, nullptr);
}

Readings latest() {
    portENTER_CRITICAL(&lock);
    Readings r = shared;
    portEXIT_CRITICAL(&lock);
    int count = 0;
    if (pcnt != nullptr && pcnt_unit_get_count(pcnt, &count) == ESP_OK) r.pulses = static_cast<uint32_t>(count);
    return r;
}

void set_pump(bool on) {
    if (!kHasPump) return;
    gpio_set_level(kPumpPin, on ? 1 : 0);
    portENTER_CRITICAL(&lock);
    if (on && !pump_on) ++pump_starts;  // recién prendida: otra ventana ciega
    pump_on = on;
    if (!on) shared.mixer_fault = core::MixerFault::none;
    portEXIT_CRITICAL(&lock);
}

}  // namespace sensors
