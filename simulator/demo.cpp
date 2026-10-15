#include "demo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

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

core::SystemState make() {
    core::SystemState s;
    s.has_pressure = true;
    s.has_orp = true;
    s.has_mixer = true;
    s.today = 1000;  // cualquier número: solo sirve para comparar días
    s.weekday = 6;   // sábado 3 de octubre
    s.day = 3;
    s.month = 10;
    s.hour = 18;
    s.minute = 42;
    s.wifi_ok = true;
    std::snprintf(s.wifi_ssid, sizeof s.wifi_ssid, "Casa-2.4G");
    s.wifi_bars = 3;
    s.last_upload_at = 18 * 60 + 42;

    s.flow_lpm = 12.4f;
    s.liters_today = 1860;
    s.liters_since_dose = 1850;  // x 0.020 = 37 mL
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

    // Últimas dosis, la más nueva primero; la del jueves se capturó a mano
    s.recent_doses = {{
        {1000, 6 * 60 + 10, 40, false},
        {999, 6 * 60 + 25, 36, false},
        {998, 7 * 60 + 2, 45, true},
        {997, 6 * 60 + 48, 36, false},
        {996, 6 * 60 + 31, 43, false},
    }};
    s.recent_dose_count = 5;

    s.p1_psi = 42;
    s.p2_psi = 38;
    s.p1_raw_psi = 42.0f;
    s.p2_raw_psi = 38.0f;

    // Cartuchos: 5 de cada PP, 4 de carbón y ninguno de sarro; el último
    // cambio fue el de 5 µm el jueves 1 de octubre
    s.spares = {5, 5, 5, 4, 0};
    s.last_cartridge = core::Cartridge::pp5;
    s.last_cartridge_day = 998;

    // Historial: las cifras del canvas de Historial. 30 días con la última
    // semana fija y hoy hasta las 18:00, hora por hora (suman 1,860 L).
    const uint16_t week_liters[7] = {1920, 2140, 1780, 1900, 1800, 1990, 1860};
    const uint16_t week_dose[7] = {41, 38, 43, 36, 45, 36, 40};
    const uint8_t week_p1[7] = {43, 42, 43, 41, 42, 42, 42};
    const uint8_t week_p2[7] = {39, 38, 38, 35, 38, 38, 38};
    for (int i = 0; i < core::kHistoryDays; ++i) {
        core::Totals& d = s.days[static_cast<size_t>(i)];
        d.liters = static_cast<uint16_t>(std::lround(1900 + 220 * std::sin(i * 0.9) + 90 * std::sin(i * 2.3)));
        d.dose_ml = static_cast<uint16_t>(std::lround(40 + 4 * std::sin(i * 1.3)));
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
        t.dose_ml = h == 6 ? 40 : 0;
        t.p1_psi = hour_p1[h];
        // P2 baja con el caudal: 4 psi de caída a 12.4 L/min
        const float flow = static_cast<float>(hour_liters[h]) / (h == 18 ? 42.0f : 60.0f);
        t.p2_psi = static_cast<uint8_t>(std::lround(hour_p1[h] - 4 * std::min(1.0f, flow / 12.4f)));
    }

    // Eventos, el más nuevo primero
    s.recent_events = {{
        {1000, 6 * 60 + 10, core::EventKind::dose, 40},
        {999, 6 * 60 + 25, core::EventKind::dose, 36},
        {998, 16 * 60 + 40, core::EventKind::cartridge, static_cast<uint16_t>(core::Cartridge::pp5)},
        {998, 7 * 60 + 2, core::EventKind::dose_typed, 45},
        {997, 22 * 60 + 14, core::EventKind::drop_warning, 6},
    }};
    s.recent_event_count = 5;
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

    s.mixer_mode = core::MixerMode::automatic;
    s.mixer_on = true;
    s.mixer_minutes_left = 3;
    s.mixer_progress_pct = 80;
    s.mixer_started_at = 18 * 60 + 30;
    s.mixer_next_at = 22 * 60 + 30;

    // Hoy: 02:30, 06:10 (después de la dosis), 10:30, 14:30 y la de ahorita.
    // 15 + 30 + 15 + 15 + 12 que lleva = 87 min.
    s.mixer_today_minutes = 87;
    s.mixer_today_runs = 5;
    s.mixer_runs = {{
        {18 * 60 + 30, 12, core::MixerReason::schedule},
        {14 * 60 + 30, 15, core::MixerReason::schedule},
        {10 * 60 + 30, 15, core::MixerReason::schedule},
        {6 * 60 + 10, 30, core::MixerReason::dose},
    }};
    s.mixer_run_count = 4;
    return s;
}

core::SystemState s = make();
uint32_t seconds = 0;
float liters_fraction = 0.0f;
uint8_t mixer_run_total = 15;  // minutos de la vuelta de ahorita: 15 en Auto, 60 a mano
bool mixer_by_hand = false;    // la vuelta de ahorita se prendió a mano

void start_mixer(core::MixerReason reason, uint8_t minutes) {
    std::rotate(s.mixer_runs.rbegin(), s.mixer_runs.rbegin() + 1, s.mixer_runs.rend());
    s.mixer_runs[0] = {core::minute_of_day(s), 0, reason};
    s.mixer_run_count = static_cast<uint8_t>(std::min<int>(s.mixer_run_count + 1, 4));
    ++s.mixer_today_runs;
    s.mixer_on = true;
    s.mixer_started_at = core::minute_of_day(s);
    s.mixer_minutes_left = minutes;
    s.mixer_progress_pct = 0;
    mixer_run_total = minutes;
    mixer_by_hand = reason == core::MixerReason::manual;
}

void add_event(core::EventKind kind, uint16_t value) {
    std::rotate(s.recent_events.rbegin(), s.recent_events.rbegin() + 1, s.recent_events.rend());
    s.recent_events[0] = {s.today, core::minute_of_day(s), kind, value};
    s.recent_event_count = static_cast<uint8_t>(std::min<int>(s.recent_event_count + 1, 5));
}

void add_dose(uint32_t ml, bool typed) {
    std::rotate(s.recent_doses.rbegin(), s.recent_doses.rbegin() + 1, s.recent_doses.rend());
    s.recent_doses[0] = {s.today, core::minute_of_day(s), static_cast<uint16_t>(ml), typed};
    s.recent_dose_count = static_cast<uint8_t>(std::min<int>(s.recent_dose_count + 1, 5));
    s.liters_since_dose = 0;
    s.hours[s.hour].dose_ml = static_cast<uint16_t>(s.hours[s.hour].dose_ml + ml);
    s.days.back().dose_ml = static_cast<uint16_t>(s.days.back().dose_ml + ml);
    add_event(typed ? core::EventKind::dose_typed : core::EventKind::dose, static_cast<uint16_t>(ml));
}

void next_minute() {
    s.minute = static_cast<uint8_t>((s.minute + 1) % 60);
    if (s.minute == 0) s.hour = static_cast<uint8_t>((s.hour + 1) % 24);
    if (s.wifi_ok) s.last_upload_at = core::minute_of_day(s);  // envía cada minuto

    if (s.minute == 0 && s.hour == 0) {  // medianoche (el demo no se fija en el fin de mes)
        ++s.today;
        s.weekday = static_cast<uint8_t>((s.weekday + 1) % 7);
        ++s.day;
        s.liters_today = 0;
        s.mixer_today_minutes = 0;
        s.mixer_today_runs = 0;
        std::rotate(s.days.begin(), s.days.begin() + 1, s.days.end());
        s.days.back() = {};
        s.hours = {};
    }

    // Bomba R: cuenta hacia atrás la vuelta de ahorita. A mano, al terminar
    // queda Apagada; en Auto espera la siguiente del programa.
    if (s.mixer_on && s.mixer_minutes_left > 0) {
        --s.mixer_minutes_left;
        ++s.mixer_today_minutes;
        ++s.mixer_runs[0].minutes;
        s.mixer_progress_pct = static_cast<uint8_t>((mixer_run_total - s.mixer_minutes_left) * 100 / mixer_run_total);
        if (s.mixer_minutes_left == 0) {
            s.mixer_on = false;
            if (s.mixer_mode == core::MixerMode::on) s.mixer_mode = core::MixerMode::off;
        }
    }
    if (s.mixer_mode == core::MixerMode::automatic && !s.mixer_on && core::minute_of_day(s) == s.mixer_next_at) {
        start_mixer(core::MixerReason::schedule, s.mixer_every_4h_min);
        s.mixer_next_at = static_cast<uint16_t>((s.mixer_next_at + 4 * 60) % 1440);
    }

    // Cada hora entra una muestra nueva a la gráfica de ORP de 7 días y la
    // hora nueva del historial arranca con las presiones y el ORP de ahorita
    if (s.minute == 0) {
        s.hours[s.hour] = {0, 0, static_cast<uint8_t>(s.p1_psi), static_cast<uint8_t>(s.p2_psi),
                           static_cast<int16_t>(s.orp_mv)};
        std::rotate(s.orp_7d.begin(), s.orp_7d.begin() + 1, s.orp_7d.end());
        s.orp_7d.back() = static_cast<int16_t>(s.orp_mv);
    }

    // Cada minuto entra una muestra nueva a la gráfica de 2 h
    std::rotate(s.flow_2h.begin(), s.flow_2h.begin() + 1, s.flow_2h.end());
    s.flow_2h.back() = s.flow_lpm;

    // Cada 5 min entra una muestra nueva a la gráfica de 24 h
    if (s.minute % 5 == 0) {
        std::rotate(s.flow_24h.begin(), s.flow_24h.begin() + 1, s.flow_24h.end());
        s.flow_24h.back() = s.flow_lpm;
    }
}

}  // namespace

const core::SystemState& state() { return s; }

void tick_1s() {
    ++seconds;
    s.flow_lpm = 12.4f + 0.15f * std::sin(seconds / 3.0f);
    s.flow_24h.back() = s.flow_lpm;
    s.flow_2h.back() = s.flow_lpm;

    liters_fraction += s.flow_lpm / 60.0f;
    while (liters_fraction >= 1.0f) {
        liters_fraction -= 1.0f;
        ++s.liters_today;
        ++s.liters_since_dose;
        ++s.hours[s.hour].liters;
        ++s.days.back().liters;
    }

    if (seconds % 60 == 0) next_minute();
}

void on_dose_confirmed(uint32_t ml) { add_dose(ml, false); }

void on_dose_typed(uint32_t ml) { add_dose(ml, true); }

void on_mixer_mode(core::MixerMode mode) {
    if (mode == s.mixer_mode) return;
    s.mixer_mode = mode;
    if (mode == core::MixerMode::on) {
        start_mixer(core::MixerReason::manual, s.mixer_manual_max_min);
    } else if (mode == core::MixerMode::off || mixer_by_hand) {
        s.mixer_on = false;  // apagarla, o pasar de a mano a Auto: espera la siguiente
    }
}

void on_mixer_after_dose(uint32_t minutes) { s.mixer_after_dose_min = static_cast<uint8_t>(minutes); }

void on_mixer_every_4h(uint32_t minutes) { s.mixer_every_4h_min = static_cast<uint8_t>(minutes); }

void on_flow_factor(uint32_t pulses_per_liter) { s.flow_pulses_per_liter = static_cast<uint16_t>(pulses_per_liter); }

void on_pressure_zeroed() {
    // En la casa se hace con V1 cerrada; el demo nomás deja la lectura en 0
    s.p1_raw_psi = 0.0f;
    s.p2_raw_psi = 0.0f;
}

void on_dose_ratio(uint32_t thousandths) { s.ml_per_liter = static_cast<float>(thousandths) / 1000.0f; }

void on_daily_cap(uint32_t ml) { s.daily_cap_ml = static_cast<uint16_t>(ml); }

void on_night_off(bool on) { s.night_off = on; }

void on_idle_minutes(uint32_t minutes) { s.idle_minutes = static_cast<uint8_t>(minutes); }

void on_strip_checked(uint32_t tenths_mgl) {
    s.last_strip = {s.weekday, s.day, s.month, static_cast<uint8_t>(tenths_mgl), static_cast<int16_t>(s.orp_mv)};
    s.chlorine_mgl = static_cast<float>(tenths_mgl) / 10.0f;  // el demo no estima: copia la tira
}

}  // namespace demo
