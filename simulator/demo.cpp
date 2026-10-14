#include "demo.h"

#include <algorithm>
#include <cmath>

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
    s.has_mixer = true;
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

    s.p1_psi = 42;
    s.p2_psi = 38;

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

void next_minute() {
    s.minute = static_cast<uint8_t>((s.minute + 1) % 60);
    if (s.minute == 0) s.hour = static_cast<uint8_t>((s.hour + 1) % 24);

    // Bomba R: corre 15 min con el programa de las 18:30
    if (s.mixer_on && s.mixer_minutes_left > 0) {
        --s.mixer_minutes_left;
        s.mixer_progress_pct = static_cast<uint8_t>((15 - s.mixer_minutes_left) * 100 / 15);
        if (s.mixer_minutes_left == 0) s.mixer_on = false;
    }

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

    liters_fraction += s.flow_lpm / 60.0f;
    while (liters_fraction >= 1.0f) {
        liters_fraction -= 1.0f;
        ++s.liters_today;
        ++s.liters_since_dose;
    }

    if (seconds % 60 == 0) next_minute();
}

void on_dose_confirmed(uint32_t /*ml*/) {
    // El firmware además lo guardaría en el historial
    s.liters_since_dose = 0;
}

}  // namespace demo
