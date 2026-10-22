#include "sensors.h"

#include <array>

#include "board.h"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace sensors {
namespace {

constexpr const char* kTag = "sensors";
constexpr int kTimeoutMs = 50;

// F: el YF-B6 en el GPIO6. El PCNT cuenta flancos de bajada solo, con filtro
// de 10 µs (documento, pág. 39). Al llegar al tope suma y vuelve a empezar.
constexpr gpio_num_t kFlowPin = GPIO_NUM_6;
constexpr int kPcntLimit = 30000;

// Bomba R: módulo MOSFET en el RXD del header UART2
constexpr gpio_num_t kPumpPin = GPIO_NUM_44;

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

// INA219 en 0x40, con su configuración de fábrica (32 V y ±320 mV)
constexpr uint8_t kInaAddress = 0x40;
constexpr uint8_t kInaShunt = 0x01;  // 10 µV por cuenta, con signo
constexpr uint8_t kInaBus = 0x02;    // bits 15-3, 4 mV por cuenta
constexpr float kShuntOhms = CONFIG_HMI_INA219_SHUNT_MOHM / 1000.0f;

pcnt_unit_handle_t pcnt = nullptr;
i2c_master_dev_handle_t ads = nullptr;
i2c_master_dev_handle_t ina = nullptr;

// Lo que deja la tarea para latest(). Un spinlock basta: solo se copian floats.
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
Readings shared;

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

bool ina_read(float* volts, float* amps) {
    uint16_t bus = 0;
    uint16_t shunt = 0;
    if (!read16(ina, kInaBus, &bus) || !read16(ina, kInaShunt, &shunt)) return false;
    *volts = static_cast<float>(bus >> 3) * 0.004f;
    *amps = static_cast<float>(static_cast<int16_t>(shunt)) * 10e-6f / kShuntOhms;
    return true;
}

// Lee el I2C sin parar: una vuelta por los 4 canales son ~45 ms, así que el
// promedio de 16 cubre ~0.7 s
void task(void*) {
    std::array<Average, kAdsChannels> average;
    std::array<float, kAdsChannels> volts{};
    while (true) {
        bool ads_ok = ads != nullptr;
        for (int ch = 0; ch < kAdsChannels && ads_ok; ++ch) {
            float v = 0.0f;
            ads_ok = ads_read(ch, &v);
            if (ads_ok) volts[ch] = average[ch].add(v);
        }
        float mixer_volts = 0.0f;
        float mixer_amps = 0.0f;
        const bool ina_ok = ina != nullptr && ina_read(&mixer_volts, &mixer_amps);

        portENTER_CRITICAL(&lock);
        shared.ads_ok = ads_ok;
        if (ads_ok) {
            shared.p1_volts = volts[0] * 2.0f;  // divisor 10k/10k
            shared.p2_volts = volts[1] * 2.0f;
            shared.level_volts = volts[2];
            shared.ldr_volts = volts[3];
        }
        shared.ina_ok = ina_ok;
        if (ina_ok) {
            shared.mixer_volts = mixer_volts;
            shared.mixer_amps = mixer_amps;
        }
        portEXIT_CRITICAL(&lock);

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

void set_pump(bool on) { gpio_set_level(kPumpPin, on ? 1 : 0); }

}  // namespace sensors
