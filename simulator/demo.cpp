#include "demo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "calendar.h"
#include "detect.h"
#include "events.h"
#include "level.h"
#include "mixing.h"
#include "monitor.h"
#include "telemetry.h"
#include "water.h"

namespace demo {
namespace {

// Bloques de caudal de las últimas 24 h: [desde, hasta] en minutos desde
// las 18:42 de ayer y L/min. Suman los 1,850 L desde la dosis de las 06:10.
struct Block {
    int from;
    int to;
    float lpm;
};
constexpr Block kBlocks[] = {
    {38, 78, 11.6f},     {168, 183, 7.0f},     {453, 458, 2.0f},    {688, 733, 12.3f},
    {863, 893, 10.5f},   {1108, 1128, 11.0f},  {1348, 1373, 6.6f},  {1390, 1440, 12.2f},
};

// Lo que leería el A2 con el agua a esa altura del fondo
float level_volts_at(float cm, const core::SystemState& s) {
    return s.level_zero_volts + (cm - s.level_sensor_cm) / static_cast<float>(s.level_range_cm) * core::kLevelSpanVolts;
}

core::SystemState make() {
    core::SystemState s;
    // Lo que hay en la casa (octubre 2026): todo menos la sonda ORP y las
    // bombas A y B. HMI_DEMO=completo prende la sonda para ver el canvas.
    s.has_pressure = true;
    s.has_orp = false;
    s.has_mixer = true;
    s.today = 1000;  // cualquier número: solo sirve para comparar días
    s.weekday = 6;   // sábado 3 de octubre
    s.day = 3;
    s.month = 10;
    s.hour = 18;
    s.minute = 42;
    s.wifi_ok = true;
    s.has_server = true;
    std::snprintf(s.wifi_ip, sizeof s.wifi_ip, "192.168.1.112");
    std::snprintf(s.wifi_ssid, sizeof s.wifi_ssid, "Casa-2.4G");
    s.devices.ezo_orp = false;
    s.has_ldr = true;
    s.ldr_volts = 0.88f;  // lo que dio el LDR con luz de cuarto en la prueba del 11 oct

    // Sensor de nivel de 0 a 2 m, a 3 cm del fondo, con cero en el aire de
    // 0.134 V, el mínimo en 60 cm y marcado lleno a 183 cm. Ahorita: 150 cm,
    // 73 % útil.
    s.has_level = true;
    s.level_range_cm = 200;
    s.level_sensor_cm = 3;
    s.level_intake_cm = 60;
    s.level_zero_volts = 0.134f;
    s.level_full_cm = 183;
    s.level_volts = level_volts_at(150.0f, s);
    s.mixer_volts = 12.1f;
    s.mixer_amps = 1.58f;  // arranca prendida
    s.wifi_bars = 3;
    s.last_upload_at = 18 * 60 + 42;

    s.flow_lpm = 12.4f;
    s.liters_today = 1860;
    s.liters_since_dose = 1850;  // x 0.100 = 185 mL
    for (int i = 0; i < core::kFlowSamples24h; ++i) {
        const int t = i * 5;
        float v = 0.0f;
        for (const Block& b : kBlocks) {
            if (t >= b.from && t <= b.to) {
                v = b.lpm + 0.35f * std::sin(t / 6.3f) + 0.15f * std::sin(t / 2.9f);
            }
        }
        s.flow_24h[i] = std::max(0.0f, v);
    }
    s.flow_24h.back() = s.flow_lpm;

    // Últimas 2 h, una muestra por minuto: la curva del canvas de Flujo.
    // Son los dos últimos bloques de arriba (6.6 y 12.2 L/min) vistos de cerca.
    for (int i = 0; i < core::kFlowSamples2h; ++i) {
        float v = 0.0f;
        if (i >= 28 && i < 53) {
            v = 6.6f + 1.6f * std::sin((i - 28) / 4.0f) + (i % 3) * 0.25f;
        } else if (i >= 70 && i < 74) {
            v = 4.0f + (i - 70) * 2.1f;
        } else if (i >= 74) {
            v = 12.1f + 0.45f * std::sin(i / 3.1f) + 0.2f * std::sin(i / 1.7f);
        }
        s.flow_2h[i] = std::max(0.0f, v);
    }
    s.flow_2h.back() = s.flow_lpm;

    // Últimas dosis, la más nueva primero; la del jueves se capturó a mano.
    // Con 0.1 mL por litro (prueba de jarras): cinco veces las del canvas.
    s.recent_doses = {{
        {1000, 6 * 60 + 10, 200, false},
        {999, 6 * 60 + 25, 180, false},
        {998, 7 * 60 + 2, 225, true},
        {997, 6 * 60 + 48, 180, false},
        {996, 6 * 60 + 31, 215, false},
    }};
    s.recent_dose_count = 5;

    s.p1_psi = 42;
    s.p2_psi = 38;
    s.p1_volts = 0.5f + 42 * 4.0f / 232;  // SEN0257: 0.5 a 4.5 V es 0 a 232 psi
    s.p2_volts = 0.5f + 38 * 4.0f / 232;

    // Últimas 24 h de presión, de las 19:00 de ayer a las 18:00 de hoy (canvas DetalleP)
    const uint8_t p1_24h[24] = {44, 46, 47, 48, 48, 48, 49, 50, 50, 49, 46, 30,
                                24, 22, 23, 24, 25, 23, 24, 26, 27, 30, 36, 42};
    const uint8_t drop_24h[24] = {3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4};
    for (size_t i = 0; i < 24; ++i) {
        s.p1_24h[i] = p1_24h[i];
        s.p2_24h[i] = static_cast<uint8_t>(p1_24h[i] - drop_24h[i]);
    }

    // Caída contra caudal (canvas DetalleCaida): la curva con cartuchos nuevos
    // del 2 de septiembre y 42 lecturas del último mes que van subiendo
    auto fresh = [](double q) { return 0.12 * q + 0.0072 * q * q; };
    for (size_t q = 0; q < 16; ++q) s.drop_new_tenths[q] = static_cast<uint8_t>(std::lround(fresh(static_cast<double>(q)) * 10));
    s.drop_new_day = 969;
    auto frac = [](double x) { return x - std::floor(x); };
    for (int i = 0; i < core::kDropSamples; ++i) {
        const double day = i * 30.0 / 42.0;
        const double q = 3 + frac(std::sin(i * 12.9898) * 43758.5453) * 10;
        const double noise = (frac(std::sin(i * 78.233) * 12345.678) - 0.5) * 0.4;
        const double p = fresh(q) * (1.05 + 0.45 * day / 30) + noise;
        s.drop_samples[static_cast<size_t>(i)] = {971 + static_cast<uint32_t>(day), static_cast<uint8_t>(std::lround(q * 10)),
                                                  static_cast<uint8_t>(std::lround(std::max(0.0, p) * 10))};
    }
    s.drop_sample_count = core::kDropSamples;

    // Filtros: el prefiltro se purgó hace 6 días, el de 5 µm se cambió el
    // jueves 1 de octubre y el vaso del sarro está vacío
    s.filters = {{
        {994, 11300, true}, {969, 58900, true}, {969, 58900, true},
        {998, 4410, true},  {969, 58900, true}, {0, 0, false},
    }};
    s.spares = {0, 5, 5, 5, 4, 0};
    s.last_cartridge = core::Filter::pp5;
    s.last_cartridge_day = 998;

    // Historial: las cifras del canvas de Historial. 30 días con la última
    // semana fija y hoy hasta las 18:00, hora por hora (suman 1,860 L).
    const uint16_t week_liters[7] = {1920, 2140, 1780, 1900, 1800, 1990, 1860};
    const uint16_t week_dose[7] = {205, 190, 215, 180, 225, 180, 200};
    const uint8_t week_p1[7] = {43, 42, 43, 41, 42, 42, 42};
    const uint8_t week_p2[7] = {39, 38, 38, 35, 38, 38, 38};
    for (int i = 0; i < core::kHistoryDays; ++i) {
        core::Totals& d = s.days[static_cast<size_t>(i)];
        d.liters = static_cast<uint16_t>(std::lround(1900 + 220 * std::sin(i * 0.9) + 90 * std::sin(i * 2.3)));
        d.dose_ml = static_cast<uint16_t>(std::lround(200 + 20 * std::sin(i * 1.3)));
        d.p1_psi = static_cast<uint8_t>(std::lround(41 + 2 * std::sin(i * 0.7)));
        d.p2_psi = static_cast<uint8_t>(d.p1_psi - (i < 27 ? 3 + std::lround(i * 0.08) : 4));
        d.orp_mv = static_cast<int16_t>(std::lround(655 + 12 * std::sin(i * 0.8) - (i == 25 ? 45 : 0)));
        if (i >= 23) {
            d.liters = week_liters[i - 23];
            d.dose_ml = week_dose[i - 23];
            d.p1_psi = week_p1[i - 23];
            d.p2_psi = week_p2[i - 23];
        }
    }
    const uint16_t hour_liters[19] = {10, 5, 3, 3, 5, 25, 99, 156, 140, 99, 78, 70, 86, 95, 78, 70, 82, 245, 511};
    const uint8_t hour_p1[19] = {48, 49, 50, 50, 49, 46, 30, 24, 22, 23, 24, 25, 23, 24, 26, 27, 30, 36, 42};
    for (int h = 0; h < 19; ++h) {
        core::Totals& t = s.hours[static_cast<size_t>(h)];
        t.liters = hour_liters[h];
        t.dose_ml = h == 6 ? 200 : 0;
        t.p1_psi = hour_p1[h];
        // P2 baja con el caudal: 4 psi de caída a 12.4 L/min
        const float flow = static_cast<float>(hour_liters[h]) / (h == 18 ? 42.0f : 60.0f);
        t.p2_psi = static_cast<uint8_t>(std::lround(hour_p1[h] - 4 * std::min(1.0f, flow / 12.4f)));
    }

    // Eventos, el más nuevo primero
    // (los del canvas de Historial y los de Alarmas, juntos)
    s.recent_events = {{
        {1000, 6 * 60 + 10, core::EventKind::dose, 200, core::kNoTime, core::kNoTime},
        {1000, 3 * 60 + 20, core::EventKind::offline, 12, core::kNoTime, 3 * 60 + 32},
        {999, 6 * 60 + 25, core::EventKind::dose, 180, core::kNoTime, core::kNoTime},
        {998, 16 * 60 + 40, core::EventKind::cartridge, static_cast<uint16_t>(core::Filter::pp5), core::kNoTime,
         core::kNoTime},
        {998, 7 * 60 + 2, core::EventKind::dose_typed, 225, core::kNoTime, core::kNoTime},
        {997, 22 * 60 + 14, core::EventKind::drop_warning, 6, core::kNoTime, 23 * 60 + 5},
        {996, 13 * 60 + 5, core::EventKind::alarm, static_cast<uint16_t>(core::AlarmKind::sensor), 13 * 60 + 7,
         13 * 60 + 40},
    }};
    s.recent_event_count = 7;
    std::snprintf(s.firmware, sizeof s.firmware, "0.1");
    s.uptime_minutes = 3 * 24 * 60 + 5 * 60;
    s.orp_mv = 650;

    // ORP de los últimos 7 días, una muestra por hora: la curva del canvas de
    // Cisterna. Sube después de la dosis de las 06:30 y baja despacio; el
    // miércoles hubo un bajón.
    for (int i = 0; i < core::kOrpSamples7d; ++i) {
        const int hour = (19 + i) % 24;
        const float t = std::fmod(hour - 6.5f + 24.0f, 24.0f);
        float v = 560.0f + 165.0f * std::exp(-t / 20.0f) + 8.0f * std::sin(i / 2.3f);
        if (i > 60 && i < 84) v -= 70.0f * std::exp(-std::abs(i - 72) / 8.0f);
        s.orp_7d[i] = static_cast<int16_t>(std::lround(v));
    }
    s.orp_7d.back() = static_cast<int16_t>(s.orp_mv);
    for (int h = 0; h < 19; ++h) s.hours[static_cast<size_t>(h)].orp_mv = s.orp_7d[static_cast<size_t>(149 + h)];
    s.days.back().orp_mv = s.orp_mv;
    s.chlorine_mgl = 0.5f;
    s.last_strip = {6, 26, 9, 5, 641};  // sábado 26 de septiembre: 0.5 mg/L con 641 mV

    // La bomba R va en la vuelta por agua nueva de las 18:30: lleva 12 de 15
    // min y desde que arrancó entraron unos 150 L
    s.mixer_mode = core::MixerMode::automatic;
    s.mixer_on = true;
    s.mixer_minutes_left = 3;
    s.mixer_run_minutes = 15;
    s.mixer_progress_pct = 80;
    s.mixer_started_at = 18 * 60 + 30;
    s.liters_since_mix = 150;

    // Hoy: 06:10 (la dosis), 12:40 (1,000 L después) y la de ahorita.
    // 45 + 15 + 12 que lleva = 72 min. La de las 22:05 fue ayer.
    s.mixer_today_minutes = 72;
    s.mixer_today_runs = 3;
    s.mixer_runs = {{
        {18 * 60 + 30, 12, core::MixerReason::water},
        {12 * 60 + 40, 15, core::MixerReason::water},
        {6 * 60 + 10, 45, core::MixerReason::dose},
        {22 * 60 + 5, 15, core::MixerReason::water},
    }};
    s.mixer_run_count = 4;
    return s;
}

core::SystemState s = make();
uint32_t seconds = 0;
bool no_flow = false;  // escenario sin-flujo: el caudal se queda en 0
bool updating = false;  // escenario actualizando: una vuelta de 40 s
float liters_fraction = 0.0f;
bool bucket_running = false;   // prueba de cubeta contando
float bucket_fraction = 0.0f;
bool test_alarm = false;  // la alarma de ahorita es la de Probar alarma
// En el demo F en realidad da 387.2 pulsos por litro y no los 396 de fábrica:
// la cubeta se llena en 3,872 pulsos y la prueba propone K 387, como el canvas
constexpr float kBucketPulsesPerLiter = 387.2f;
constexpr uint32_t kBucketFull = 3872;

// La cola de lo que va al servidor, como en el firmware: cabe un día sin red
constexpr std::size_t kOutboxSize = 8640;
core::Record outbox_buffer[kOutboxSize];
core::Outbox make_outbox() {
    core::Outbox out(outbox_buffer, kOutboxSize);
    out.last_event_seq = s.event_seq;  // los eventos del ejemplo ya estaban en el servidor
    return out;
}
core::Outbox outbox = make_outbox();

// Segundos UTC del reloj del demo. Zapopan va en UTC-6 todo el año: México
// quitó el horario de verano en 2022.
uint32_t utc_now() {
    const int64_t days = core::days_from_civil(s.year, s.month, s.day);
    return static_cast<uint32_t>(days * 86400 + (s.hour * 60 + s.minute) * 60 + seconds % 60 + 6 * 3600);
}

// Cada 10 s, lo que hará el firmware: la lectura a la cola y, con red, el
// lote al servidor. El demo no manda nada: con red da el envío por bueno.
void upload() {
    core::queue_reading(s, outbox, utc_now());
    if (!s.wifi_ok) return;
    static char body[4096];
    std::size_t records = 0;
    if (core::format_batch(outbox, body, sizeof body, &records) > 0) core::mark_sent(s, outbox, records);
}

// El reloj del demo avanza un minuto y hace lo mismo que hará el firmware
// con la hora del NTP: set_time cierra la hora o el día, tick_minute lo demás
void next_minute() {
    core::Clock now{s.year, s.month, s.day, s.hour, s.minute};
    if (++now.minute == 60) {
        now.minute = 0;
        if (++now.hour == 24) {
            now.hour = 0;
            if (++now.day > core::days_in_month(now.month, now.year)) {
                now.day = 1;
                if (++now.month == 13) {
                    now.month = 1;
                    ++now.year;
                }
            }
        }
    }
    core::set_time(s, now);
    core::tick_minute(s);
}

}  // namespace

const core::SystemState& state() { return s; }

bool set_scenario(const char* name) {
    auto alarm = [](core::AlarmKind kind) { core::alarm_raise(s, kind); };
    auto drop = [](int psi) {
        s.p2_psi = s.p1_psi - psi;
        s.p2_24h.back() = static_cast<uint8_t>(s.p2_psi);
    };
    // La bomba R se apaga y deja su aviso, como lo haría el firmware con el INA219
    auto mixer_fault = [](core::MixerFault fault) {
        if (fault == core::MixerFault::no_power) s.mixer_volts = 0.0f;
        core::mixer_report_fault(s, fault);
    };
    auto stop_flow = [] {
        no_flow = true;
        s.flow_lpm = 0.0f;
        for (int i = 114; i < core::kFlowSamples2h; ++i) s.flow_2h[static_cast<size_t>(i)] = 0.0f;
        s.flow_24h.back() = 0.0f;
    };
    if (std::strcmp(name, "sin-flujo") == 0) {
        stop_flow();
    } else if (std::strcmp(name, "sin-red") == 0) {
        s.wifi_ok = false;
        s.wifi_bars = 0;
        // Lleva 4 min sin red: 24 lecturas en la cola, como el canvas
        s.last_upload_at = static_cast<uint16_t>(core::minute_of_day(s) - 4);
        for (uint32_t i = 24; i > 0; --i) core::queue_reading(s, outbox, utc_now() - i * 10);
    } else if (std::strcmp(name, "alarma") == 0) {
        stop_flow();
        alarm(core::AlarmKind::no_flow);
    } else if (std::strcmp(name, "sensor") == 0) {
        s.flow_sensor_ok = false;
        alarm(core::AlarmKind::sensor);
    } else if (std::strcmp(name, "tope") == 0) {
        alarm(core::AlarmKind::daily_cap);
    } else if (std::strcmp(name, "fase1") == 0) {
        // Fase 1 del documento: F y la bomba R; sin ADS1115 no hay P1, P2 ni LDR
        s.has_pressure = s.has_orp = false;
        s.devices.ads1115 = s.devices.ezo_orp = false;
        s.has_ldr = s.has_level = false;
        // Sin P1 y P2 no hay avisos de caída en el registro
        const auto end = std::remove_if(s.recent_events.begin(), s.recent_events.begin() + s.recent_event_count,
                                   [](const core::Event& e) {
                                       return e.kind == core::EventKind::drop_warning ||
                                              e.kind == core::EventKind::drop_alarm;
                                   });
        s.recent_event_count = static_cast<uint8_t>(end - s.recent_events.begin());
    } else if (std::strcmp(name, "completo") == 0) {
        s.has_orp = true;
        s.devices.ezo_orp = true;
    } else if (std::strcmp(name, "ro-cocina") == 0) {
        s.ro_kitchen_planned = true;
    } else if (std::strcmp(name, "actualizando") == 0) {
        updating = true;
    } else if (std::strcmp(name, "a-prueba") == 0) {
        s.trial = core::FirmwareTrial::testing;
        s.trial_seconds = 30;  // ota::kHealthySeconds
    } else if (std::strcmp(name, "revisar") == 0) {
        drop(8);  // 5.4 psi más que los 2.6 de nuevos a 12.4 L/min
    } else if (std::strcmp(name, "tapados") == 0) {
        drop(13);  // 10.4 más
    } else if (std::strcmp(name, "sin-agua") == 0 || std::strcmp(name, "cisterna-vacia") == 0) {
        // Se fue el agua del SIAPA: sin flujo y P1 y P2 en 0 al mismo tiempo
        stop_flow();
        s.p1_psi = s.p2_psi = 0;
        s.p1_volts = s.p2_volts = 0.5f;
        s.p1_24h.back() = s.p2_24h.back() = 0;
        // La casa se ha ido comiendo la cisterna; vacía ya pasó del mínimo y
        // la bomba R se apaga a esperar agua
        const bool empty = std::strcmp(name, "cisterna-vacia") == 0;
        s.level_volts = level_volts_at(empty ? 58.0f : 70.0f, s);
        core::check_supply(s);  // lo anota ya, sin esperar al siguiente minuto
        if (empty) core::mixer_tick_minute(s);
    } else if (std::strcmp(name, "sin-senal") == 0) {
        // Se soltaron los cables de P1 y del nivel; P2 sigue leyendo. Pasa
        // por core::detect_sensors como en el firmware: 5 s y quedan sin señal.
        s.p1_volts = s.level_volts = 0.0f;
        s.p1_psi = 0;
        core::Detection d;
        const core::AnalogReadings r{true, s.p1_volts, s.p2_volts, s.level_volts, s.ldr_volts};
        for (uint8_t i = 0; i < core::kSeenSeconds; ++i) core::detect_sensors(s, d, r);
    } else if (std::strcmp(name, "sin-12v") == 0) {
        mixer_fault(core::MixerFault::no_power);
    } else if (std::strcmp(name, "nivel-bajo") == 0) {
        mixer_fault(core::MixerFault::dry);
    } else if (std::strcmp(name, "atorada") == 0) {
        mixer_fault(core::MixerFault::stuck);
    } else {
        return false;
    }
    return true;
}

void on_alarm_acknowledged(core::AlarmKind kind) { core::alarm_acknowledge(s, kind); }

void on_alarm_test() {
    core::alarm_raise(s, core::AlarmKind::no_flow);
    test_alarm = true;
}

void on_restart() {
    // El demo no se reinicia: el firmware sí
}

void on_bucket_test_started() {
    s.bucket_pulses = 0;
    bucket_fraction = 0.0f;
    bucket_running = true;
}

// Escenario actualizando: 20 s subiendo, 4 s "Lista", 2 s sin nada, 10 s
// "No se actualizó" (firma de otra llave) y 4 s sin nada
static void tick_update() {
    const uint32_t t = seconds % 40;
    core::UpdateStatus& u = s.update;
    u = {};
    std::snprintf(u.version, sizeof u.version, "feb55f4");
    if (t < 20) {
        u.stage = core::UpdateStage::receiving;
        u.pct = static_cast<uint8_t>(t * 5);
    } else if (t < 24) {
        u.stage = core::UpdateStage::installed;
    } else if (t >= 26 && t < 36) {
        u.stage = core::UpdateStage::failed;
        u.failure = core::UpdateFailure::signature;
    }
}

void tick_1s() {
    ++seconds;
    if (updating) tick_update();
    // a-prueba: la cuenta baja con red y al llegar a 0 queda como buena
    if (s.trial == core::FirmwareTrial::testing && --s.trial_seconds == 0) s.trial = core::FirmwareTrial::passed;
    core::set_flow(s, no_flow ? 0.0f : 12.4f + 0.15f * std::sin(seconds / 3.0f));

    // Los litros enteros que contó F en este segundo, como el PCNT del firmware
    liters_fraction += s.flow_lpm / 60.0f;
    const auto whole_liters = static_cast<uint32_t>(liters_fraction);
    liters_fraction -= static_cast<float>(whole_liters);
    core::count_liters(s, whole_liters);

    // Prueba de cubeta: cuenta hasta que la cubeta llega a la marca
    if (bucket_running) {
        bucket_fraction += s.flow_lpm / 60.0f * kBucketPulsesPerLiter;
        const auto whole = static_cast<uint32_t>(bucket_fraction);
        bucket_fraction -= static_cast<float>(whole);
        s.bucket_pulses = std::min(kBucketFull, s.bucket_pulses + whole);
        if (s.bucket_pulses == kBucketFull) bucket_running = false;
    }

    // La de Probar alarma se quita sola cuando la reconocen
    if (test_alarm && s.alarms[static_cast<size_t>(core::AlarmKind::no_flow)].acknowledged) {
        core::alarm_clear(s, core::AlarmKind::no_flow);
        test_alarm = false;
    }

    // INA219: la bomba R jala ~1.6 A sumergida (documento de proyecto)
    s.mixer_amps = s.mixer_on ? 1.58f + 0.02f * std::sin(seconds / 5.0f) : 0.0f;

    if (seconds % 60 == 0) next_minute();
    if (seconds % 10 == 0) upload();
}

void on_dose_confirmed(uint32_t ml) { core::register_dose(s, ml, false); }

void on_dose_typed(uint32_t ml) { core::register_dose(s, ml, true); }

void on_mixer_mode(core::MixerMode mode) { core::mixer_set_mode(s, mode); }

void on_mixer_after_dose(uint32_t minutes) { s.mixer_after_dose_min = static_cast<uint8_t>(minutes); }

void on_mixer_every_liters(uint32_t minutes) { s.mixer_every_liters_min = static_cast<uint8_t>(minutes); }

void on_level_range(uint32_t cm) {
    if (cm > 0) s.level_range_cm = static_cast<uint16_t>(cm);
}

void on_level_zeroed() { core::level_zero(s); }

void on_level_sensor(uint32_t cm) { s.level_sensor_cm = static_cast<uint8_t>(cm); }

void on_level_intake(uint32_t cm) { s.level_intake_cm = static_cast<uint8_t>(cm); }

void on_level_full() { core::level_mark_full(s); }

void on_service(bool on) {
    if (on) {
        core::service_start(s);
    } else {
        core::service_end(s);
    }
}

void on_flow_factor(uint32_t pulses_per_liter) { s.flow_pulses_per_liter = static_cast<uint16_t>(pulses_per_liter); }

void on_pressure_zeroed() {
    // En la casa se hace con V1 cerrada; el demo nomás deja la lectura en 0
    s.p1_zero_psi = 0.3f;
    s.p2_zero_psi = 0.2f;
}

void on_dose_ratio(uint32_t thousandths) { s.ml_per_liter = static_cast<float>(thousandths) / 1000.0f; }

void on_daily_cap(uint32_t ml) { s.daily_cap_ml = static_cast<uint16_t>(ml); }

void on_idle_minutes(uint32_t minutes) { s.idle_minutes = static_cast<uint8_t>(minutes); }

void on_filter_serviced(core::Filter filter) { core::register_filter(s, filter); }

void on_spares_set(core::Filter filter, uint32_t count) {
    s.spares[static_cast<size_t>(filter)] = static_cast<uint8_t>(count);
}

void on_strip_checked(uint32_t tenths_mgl) {
    s.last_strip = {s.weekday, s.day, s.month, static_cast<uint8_t>(tenths_mgl), static_cast<int16_t>(s.orp_mv)};
    s.chlorine_mgl = static_cast<float>(tenths_mgl) / 10.0f;  // el demo no estima: copia la tira
}

}  // namespace demo
