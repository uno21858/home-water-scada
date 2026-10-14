#include "demo.h"

#include <algorithm>
#include <cmath>
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
    s.chlorine_mgl = 0.5f;
    s.last_strip = {6, 26, 9, 5, 641};  // sábado 26 de septiembre: 0.5 mg/L con 641 mV

    s.mixer_mode = core::MixerMode::automatic;
    s.mixer_on = true;
    s.mixer_minutes_left = 3;
    s.mixer_progress_pct = 80;
    s.mixer_started_at = 18 * 60 + 30;
    s.mixer_next_at = 22 * 60 + 30;
    return s;
}

core::SystemState s = make();
uint32_t seconds = 0;
float liters_fraction = 0.0f;

void add_dose(uint32_t ml, bool typed) {
    std::rotate(s.recent_doses.rbegin(), s.recent_doses.rbegin() + 1, s.recent_doses.rend());
    s.recent_doses[0] = {s.today, core::minute_of_day(s), static_cast<uint16_t>(ml), typed};
    s.recent_dose_count = static_cast<uint8_t>(std::min<int>(s.recent_dose_count + 1, 5));
    s.liters_since_dose = 0;
}

void next_minute() {
    s.minute = static_cast<uint8_t>((s.minute + 1) % 60);
    if (s.minute == 0) s.hour = static_cast<uint8_t>((s.hour + 1) % 24);
    if (s.minute == 0 && s.hour == 0) {  // medianoche (el demo no se fija en el fin de mes)
        ++s.today;
        s.weekday = static_cast<uint8_t>((s.weekday + 1) % 7);
        ++s.day;
        s.liters_today = 0;
    }

    // Bomba R: corre 15 min con el programa de las 18:30
    if (s.mixer_on && s.mixer_minutes_left > 0) {
        --s.mixer_minutes_left;
        s.mixer_progress_pct = static_cast<uint8_t>((15 - s.mixer_minutes_left) * 100 / 15);
        if (s.mixer_minutes_left == 0) s.mixer_on = false;
    }

    // Cada hora entra una muestra nueva a la gráfica de ORP de 7 días
    if (s.minute == 0) {
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
    }

    if (seconds % 60 == 0) next_minute();
}

void on_dose_confirmed(uint32_t ml) { add_dose(ml, false); }

void on_dose_typed(uint32_t ml) { add_dose(ml, true); }

void on_strip_checked(uint32_t tenths_mgl) {
    s.last_strip = {s.weekday, s.day, s.month, static_cast<uint8_t>(tenths_mgl), static_cast<int16_t>(s.orp_mv)};
    s.chlorine_mgl = static_cast<float>(tenths_mgl) / 10.0f;  // el demo no estima: copia la tira
}

}  // namespace demo
