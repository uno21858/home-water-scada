#include "app.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <new>

#include "board.h"
#include "calendar.h"
#include "detect.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "level.h"
#include "lvgl.h"
#include "mixing.h"
#include "monitor.h"
#include "net.h"
#include "ota.h"
#include "persist.h"
#include "sdkconfig.h"
#include "sensors.h"
#include "storage.h"
#include "telemetry.h"
#include "ui.h"
#include "water.h"

namespace app {
namespace {

constexpr const char* kTag = "app";

// Qué está instalado desde el arranque (menuconfig > Casa > Qué está
// instalado). P1 y P2, el nivel y el LDR además se detectan solos
// (core/detect.h); la bomba R solo con menuconfig.
#ifdef CONFIG_HMI_HAS_MIXER
constexpr bool kHasMixer = true;
#else
constexpr bool kHasMixer = false;
#endif
#ifdef CONFIG_HMI_HAS_PRESSURE
constexpr bool kHasPressure = true;
#else
constexpr bool kHasPressure = false;
#endif
#ifdef CONFIG_HMI_HAS_LEVEL
constexpr bool kHasLevel = true;
#else
constexpr bool kHasLevel = false;
#endif
#ifdef CONFIG_HMI_HAS_LDR
constexpr bool kHasLdr = true;
#else
constexpr bool kHasLdr = false;
#endif

// Bomba R: a los 3 s de arrancar, y luego cada segundo, se revisa el INA219
// (documento, pág. 39). Los umbrales salen de la prueba 9b.
constexpr uint32_t kMixerCheckSeconds = 3;
constexpr float kMixerMinVolts = CONFIG_HMI_MIXER_MIN_VOLTS_X10 / 10.0f;
constexpr float kMixerDryAmps = CONFIG_HMI_MIXER_DRY_AMPS_X100 / 100.0f;
constexpr float kMixerStuckAmps = CONFIG_HMI_MIXER_STUCK_AMPS_X100 / 100.0f;

// Documento, pág. 39: los litros se guardan cuando para el agua o cada 10 min
constexpr uint32_t kSaveEveryMinutes = 10;

// Datos al NAS: una lectura cada 10 s; sin red caben 8,640 (un día) en la PSRAM
constexpr const char* kMetricsUrl = CONFIG_HMI_METRICS_URL;
constexpr uint32_t kUploadMs = 10 * 1000;
constexpr std::size_t kOutboxSize = 8640;
constexpr std::size_t kBodySize = 4096;

// El estado que comparten la interfaz, la red y la OTA
core::SystemState s;
SemaphoreHandle_t lock = nullptr;

// Toma el lock al crearse y lo suelta al salir de su bloque, pase lo que pase
// adentro: no hay forma de olvidarse de soltarlo
class Locked {
public:
    Locked() { xSemaphoreTake(lock, portMAX_DELAY); }
    ~Locked() { xSemaphoreGive(lock); }
    Locked(const Locked&) = delete;
    Locked& operator=(const Locked&) = delete;
};

// Lo de abajo solo lo tocan funciones que ya tienen el lock
storage::NvsStore nvs;
core::Outbox* outbox = nullptr;
bool settings_changed = false;  // un ajuste o un registro: se guarda al siguiente segundo
uint32_t seconds = 0;           // desde que arrancó
uint32_t last_pulses = 0;       // la cuenta de F del segundo pasado
uint32_t pulse_remainder = 0;   // pulsos que todavía no hacen un litro
core::Detection detection;      // segundos que lleva viéndose cada sensor nuevo
uint32_t mixer_seconds = 0;     // cuánto lleva prendida la bomba R
uint32_t minutes_since_save = 0;
int last_minute = -1;
bool was_flowing = false;
bool test_alarm = false;        // la alarma de ahorita es la de Probar alarma
float brightness = 100.0f;

// Desde que arrancó la app (sin contar el ROM ni el bootloader), para medir
unsigned long ms_since_boot() { return static_cast<unsigned long>(esp_timer_get_time() / 1000); }

void persist(bool settings, bool history) {
    bool ok = true;
    if (settings) ok = core::save(s, nvs, core::SaveGroup::settings) >= 0 && ok;
    ok = core::save(s, nvs, core::SaveGroup::counters) >= 0 && ok;
    if (history) ok = core::save(s, nvs, core::SaveGroup::history) >= 0 && ok;
    nvs.commit();
    if (!ok) ESP_LOGE(kTag, "No se guardó todo en la NVS");
}

[[noreturn]] void restart_locked() {
    persist(true, true);
    sensors::set_pump(false);
    esp_restart();
}

void changed() { settings_changed = true; }

// F: los pulsos de este segundo dan el caudal; los litros enteros van a core
// y lo que sobra se queda para el siguiente
void read_flow(const sensors::Readings& r) {
    const uint32_t delta = r.pulses - last_pulses;  // sin signo: aunque la cuenta dé la vuelta
    last_pulses = r.pulses;
    const uint32_t k = std::max<uint32_t>(1, s.flow_pulses_per_liter);
    core::set_flow(s, static_cast<float>(delta) * 60.0f / static_cast<float>(k));
    pulse_remainder += delta;
    const uint32_t liters = pulse_remainder / k;
    pulse_remainder -= liters * k;
    if (liters > 0) core::count_liters(s, liters);
    s.bucket_pulses += delta;
}

void read_analog(const sensors::Readings& r) {
    s.devices.ads1115 = r.ads_ok;
    s.devices.ina219 = r.ina_ok;
    if (r.ads_ok) {
        // Los volts van siempre: Diagnóstico los enseña mientras se conecta
        s.level_volts = r.level_volts;
        s.ldr_volts = r.ldr_volts;
        if (s.has_pressure) {
            core::set_pressures(s, r.p1_volts, r.p2_volts);
        } else {
            s.p1_volts = r.p1_volts;
            s.p2_volts = r.p2_volts;
        }
    }
    // Lo que se conecta aparece solo y se guarda con los ajustes
    const core::AnalogReadings analog{r.ads_ok, r.p1_volts, r.p2_volts, r.level_volts, r.ldr_volts};
    if (core::detect_sensors(s, detection, analog)) {
        ESP_LOGI(kTag, "Sensores: presión %d, nivel %d, LDR %d", s.has_pressure, s.has_level, s.has_ldr);
        settings_changed = true;
    }
    if (r.ina_ok) {
        s.mixer_volts = r.mixer_volts;
        s.mixer_amps = r.mixer_amps;
    }
}

// Bomba R: el INA219 dice si hay 12 V, si mueve agua o si está atorada. Sin
// INA219 no se sabe: se trata como sin 12 V y se apaga (documento, pág. 44).
void watch_mixer(const sensors::Readings& r) {
    if (!s.has_mixer) {
        sensors::set_pump(false);
        return;
    }
    if (s.mixer_on) {
        if (++mixer_seconds >= kMixerCheckSeconds) {
            if (!r.ina_ok || r.mixer_volts < kMixerMinVolts) {
                core::mixer_report_fault(s, core::MixerFault::no_power);
            } else if (r.mixer_amps < kMixerDryAmps) {
                core::mixer_report_fault(s, core::MixerFault::dry);
            } else if (r.mixer_amps > kMixerStuckAmps) {
                core::mixer_report_fault(s, core::MixerFault::stuck);
            }
        }
    } else {
        mixer_seconds = 0;
        // Apagada, el INA219 sigue viendo el voltaje de la rama
        if (s.mixer_fault == core::MixerFault::no_power && r.ina_ok && r.mixer_volts >= kMixerMinVolts) {
            core::mixer_power_back(s);
        }
    }
    sensors::set_pump(s.mixer_on);
}

void read_network() {
    core::network_tick(s, net::online());
    const int rssi = net::rssi();  // a ojo: -60 dBm o más es buena señal
    s.wifi_bars = rssi == 0 ? 0 : rssi > -60 ? 3 : rssi > -70 ? 2 : rssi > -80 ? 1 : 0;
}

// Con LDR, de noche baja solo: 0.88 V es luz de cuarto (prueba del 11 oct).
// Se mueve poco a poco para que no parpadee. Para afinar con la placa puesta.
void adjust_brightness() {
    const float target = s.has_ldr ? std::clamp(15.0f + (s.ldr_volts - 0.1f) / 0.8f * 85.0f, 15.0f, 100.0f) : 100.0f;
    const float before = brightness;
    brightness += (target - brightness) * 0.1f;
    if (std::lround(before) != std::lround(brightness)) board::set_brightness(static_cast<uint8_t>(std::lround(brightness)));
}

// Con la hora del NTP, el minuto es el del reloj; sin ella, cada 60 s
bool minute_passed() {
    if (!net::time_valid()) return seconds % 60 == 0;
    const std::time_t now = std::time(nullptr);
    std::tm local = {};
    localtime_r(&now, &local);
    if (local.tm_min == last_minute) return false;
    last_minute = local.tm_min;
    core::set_time(s, core::Clock{static_cast<uint16_t>(local.tm_year + 1900), static_cast<uint8_t>(local.tm_mon + 1),
                                  static_cast<uint8_t>(local.tm_mday), static_cast<uint8_t>(local.tm_hour),
                                  static_cast<uint8_t>(local.tm_min)});
    return true;
}

// Cada segundo, dentro de la tarea de LVGL (con el lock tomado)
void every_second(lv_timer_t*) {
    ++seconds;
    s.uptime_minutes = seconds / 60;
    const sensors::Readings r = sensors::latest();
    read_flow(r);
    read_analog(r);
    watch_mixer(r);
    read_network();

    if (test_alarm && s.alarms[static_cast<std::size_t>(core::AlarmKind::no_flow)].acknowledged) {
        core::alarm_clear(s, core::AlarmKind::no_flow);
        test_alarm = false;
    }
    if (minute_passed()) {
        core::tick_minute(s);
        if (++minutes_since_save >= kSaveEveryMinutes) {
            persist(false, true);
            minutes_since_save = 0;
        }
    }
    const bool flowing = s.flow_lpm >= core::kFlowingLpm;
    if (was_flowing && !flowing) persist(false, false);  // paró el agua: los litros a la flash
    was_flowing = flowing;
    if (settings_changed) {
        persist(true, false);
        settings_changed = false;
    }

    adjust_brightness();
    if (ota::check_health(seconds, s.wifi_ok) == ota::Health::failed) {
        persist(true, true);
        ota::rollback();
    }
    ui::update(s);
}

// Lo que toca la persona. LVGL las llama con el lock ya tomado.
ui::Actions actions() {
    ui::Actions a;
    a.dose_confirmed = [](uint32_t ml) { core::register_dose(s, ml, false); changed(); };
    a.dose_typed = [](uint32_t ml) { core::register_dose(s, ml, true); changed(); };
    a.strip_checked = [](uint32_t tenths_mgl) {
        s.last_strip = {s.weekday, s.day, s.month, static_cast<uint8_t>(tenths_mgl), static_cast<int16_t>(s.orp_mv)};
        changed();
    };
    a.mixer_mode_set = [](core::MixerMode mode) { core::mixer_set_mode(s, mode); changed(); };
    a.mixer_after_dose_set = [](uint32_t minutes) { s.mixer_after_dose_min = static_cast<uint8_t>(minutes); changed(); };
    a.mixer_every_liters_set = [](uint32_t minutes) { s.mixer_every_liters_min = static_cast<uint8_t>(minutes); changed(); };
    a.flow_factor_set = [](uint32_t k) { s.flow_pulses_per_liter = static_cast<uint16_t>(k); changed(); };
    a.pressure_zeroed = [] { core::pressure_zero(s); changed(); };
    a.dose_ratio_set = [](uint32_t thousandths) { s.ml_per_liter = static_cast<float>(thousandths) / 1000.0f; changed(); };
    a.daily_cap_set = [](uint32_t ml) { s.daily_cap_ml = static_cast<uint16_t>(ml); changed(); };
    a.idle_minutes_set = [](uint32_t minutes) { s.idle_minutes = static_cast<uint8_t>(minutes); changed(); };
    a.filter_serviced = [](core::Filter filter) { core::register_filter(s, filter); changed(); };
    a.spares_set = [](core::Filter filter, uint32_t count) {
        s.spares[static_cast<std::size_t>(filter)] = static_cast<uint8_t>(count);
        changed();
    };
    a.service_set = [](bool on) {
        if (on) {
            core::service_start(s);
        } else {
            core::service_end(s);
        }
        changed();
    };
    a.level_range_set = [](uint32_t cm) {
        if (cm > 0) s.level_range_cm = static_cast<uint16_t>(cm);
        changed();
    };
    a.level_zeroed = [] { core::level_zero(s); changed(); };
    a.level_sensor_set = [](uint32_t cm) { s.level_sensor_cm = static_cast<uint8_t>(cm); changed(); };
    a.level_intake_set = [](uint32_t cm) { s.level_intake_cm = static_cast<uint8_t>(cm); changed(); };
    a.level_full_marked = [] { core::level_mark_full(s); changed(); };
    a.alarm_acknowledged = [](core::AlarmKind kind) { core::alarm_acknowledge(s, kind); changed(); };
    a.alarm_test = [] {
        core::alarm_raise(s, core::AlarmKind::no_flow);
        test_alarm = true;
        changed();
    };
    a.restart_requested = [] { restart_locked(); };
    a.bucket_test_started = [] { s.bucket_pulses = 0; };
    return a;
}

void lvgl_task(void*) {
    lv_init();
    lv_tick_set_cb([]() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); });
    lv_display_t* display = board::create_display();
    {
        Locked locked;
        ui::init(s, actions());
        ESP_LOGI(kTag, "Pantallas armadas a los %lu ms", ms_since_boot());
        lv_timer_create(every_second, 1000, nullptr);
        board::start_display(display);
        lv_refr_now(display);  // el primer cuadro antes del backlight: que no se vea basura
    }
    board::set_brightness(static_cast<uint8_t>(brightness));
    board::backlight(true);
    ESP_LOGI(kTag, "Pantalla prendida a los %lu ms", ms_since_boot());
    board::start_touch(display);
    {
        Locked locked;
        s.devices.gt911 = board::touch_ok();
    }

    while (true) {
        uint32_t wait_ms = 0;
        {
            Locked locked;
            wait_ms = lv_timer_handler();
        }
        vTaskDelay(pdMS_TO_TICKS(std::clamp<uint32_t>(wait_ms, 1, 100)));
    }
}

// Cada 10 s la lectura entra a la cola; con red se manda lo más viejo que
// quepa en un POST. Si quedó algo, sigue sin esperar, hasta vaciarla. Sin
// la URL de VictoriaMetrics no se encola nada: no hay a dónde mandarlo.
void upload_task(void*) {
    static char body[kBodySize];
    bool more = false;
    while (true) {
        if (!more) vTaskDelay(pdMS_TO_TICKS(kUploadMs));
        std::size_t records = 0;
        std::size_t bytes = 0;
        {
            Locked locked;
            if (!more && s.has_server && net::time_valid()) {
                core::queue_reading(s, *outbox, static_cast<uint32_t>(std::time(nullptr)));
            }
            if (net::online() && kMetricsUrl[0] != '\0') bytes = core::format_batch(*outbox, body, sizeof body, &records);
        }
        more = false;
        if (bytes == 0 || !net::post(kMetricsUrl, body, bytes)) continue;
        Locked locked;
        core::mark_sent(s, *outbox, records);
        more = outbox->size() > 0;
    }
}

bool can_install() {
    Locked locked;
    return !s.mixer_on;
}

void restart_for_update() {
    Locked locked;
    restart_locked();
}

// La cola de la telemetría va en la PSRAM: 8,640 de 48 bytes son 405 KB
core::Outbox* make_outbox() {
    std::size_t capacity = kOutboxSize;
    void* memory = heap_caps_malloc(capacity * sizeof(core::Record), MALLOC_CAP_SPIRAM);
    if (memory == nullptr) {
        ESP_LOGE(kTag, "Sin PSRAM para la cola: caben solo 60 lecturas");
        capacity = 60;
        memory = heap_caps_malloc(capacity * sizeof(core::Record), MALLOC_CAP_DEFAULT);
    }
    auto* records = static_cast<core::Record*>(memory);
    for (std::size_t i = 0; i < capacity; ++i) new (&records[i]) core::Record();
    static core::Outbox box(records, capacity);
    return &box;
}

}  // namespace

void start() {
    lock = xSemaphoreCreateMutex();

    // Lo que se guardó antes del reinicio. La bomba R arranca apagada y lo
    // que le faltaba queda pendiente.
    nvs.open();
    core::load(s, nvs);
    core::mixer_after_restart(s);
    // Los sensores que ya se vieron vienen de la NVS; menuconfig solo agrega
    s.has_mixer = kHasMixer;
    s.has_pressure = s.has_pressure || kHasPressure;
    s.has_level = s.has_level || kHasLevel;
    s.has_ldr = s.has_ldr || kHasLdr;
    s.has_server = kMetricsUrl[0] != '\0';
    s.devices.pcf8574 = false;  // fase 3
    s.devices.ezo_orp = false;  // fase 2
    const char* version = esp_app_get_description()->version;
    const std::size_t length = strnlen(version, sizeof s.firmware - 1);
    std::memcpy(s.firmware, version, length);
    s.firmware[length] = '\0';
    ESP_LOGI(kTag, "Firmware %s", s.firmware);

    outbox = make_outbox();
    outbox->last_event_seq = s.event_seq;  // los eventos guardados ya se habían mandado

    board::start_i2c();
    s.devices.expander = board::expander_ok();
    sensors::start();
    last_pulses = sensors::latest().pulses;

    char ssid[33] = {};
    char password[65] = {};
    const bool has_wifi = storage::wifi_credentials(ssid, sizeof ssid, password, sizeof password);
    if (has_wifi) std::memcpy(s.wifi_ssid, ssid, sizeof s.wifi_ssid);

    // Primero la pantalla; el WiFi tarda en arrancar y no tiene por qué esperarla
    xTaskCreatePinnedToCore(lvgl_task, "lvgl", 16 * 1024, nullptr, 3, nullptr, 1);
    if (has_wifi) {
        net::start(ssid, password);
    } else {
        ESP_LOGE(kTag, "Sin red: falta grabar wifi.csv (idf.py flash)");
    }
    xTaskCreate(upload_task, "upload", 6 * 1024, nullptr, 2, nullptr);
    ota::start(can_install, restart_for_update);
}

}  // namespace app
