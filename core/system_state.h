#pragma once
// Estado del sistema que muestra la pantalla.
// Lógica pura: sin LVGL ni hardware, así lo usan igual el firmware y el simulador.

#include <array>
#include <cstdint>

namespace core {

// Caudal de las últimas 24 h, una muestra cada 5 min: 24 * 12 + 1 = 289.
// La última muestra es la de ahorita.
inline constexpr int kFlowSamples24h = 289;

enum class Level : uint8_t { ok, warning, alarm };

enum class MixerMode : uint8_t { off, on, automatic };

struct SystemState {
    // Qué está instalado
    bool has_pressure = false;  // fase 1.5: sensores P1 y P2
    bool has_mixer = false;     // fase 2: bomba R

    // Hora y red
    uint8_t hour = 0;
    uint8_t minute = 0;
    bool wifi_ok = false;

    // Flujo
    float flow_lpm = 0.0f;
    uint32_t liters_today = 0;
    uint32_t liters_since_dose = 0;
    std::array<float, kFlowSamples24h> flow_24h{};

    // Dosis a mano
    float ml_per_liter = 0.020f;  // mL de Cloralex por litro

    // Presión antes (P1) y después (P2) de los filtros
    int p1_psi = 0;
    int p2_psi = 0;

    // Bomba R de la cisterna
    MixerMode mixer_mode = MixerMode::automatic;
    bool mixer_on = false;
    uint8_t mixer_minutes_left = 0;
    uint8_t mixer_progress_pct = 0;
    uint16_t mixer_started_at = 0;  // minuto del día en que arrancó
    uint16_t mixer_next_at = 0;     // minuto del día de la siguiente vuelta en Auto
};

// mL de Cloralex que tocan: litros desde la última dosis por la relación
inline uint32_t dose_ml(const SystemState& s) {
    return static_cast<uint32_t>(static_cast<float>(s.liters_since_dose) * s.ml_per_liter + 0.5f);
}

// Caída del tren (P1 - P2) con los umbrales del DS:
// menor a 5 psi Normal, de 5 a 10 Revisar, mayor a 10 Tapados
constexpr Level pressure_drop_level(int drop_psi) {
    if (drop_psi > 10) return Level::alarm;
    if (drop_psi >= 5) return Level::warning;
    return Level::ok;
}

// La pastilla del encabezado: lo peor de lo que se está vigilando
inline Level overall_level(const SystemState& s) {
    return s.has_pressure ? pressure_drop_level(s.p1_psi - s.p2_psi) : Level::ok;
}

inline uint16_t minute_of_day(const SystemState& s) {
    return static_cast<uint16_t>(s.hour * 60 + s.minute);
}

}  // namespace core
