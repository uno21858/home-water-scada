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

struct DoseRecord {
    uint32_t day = 0;    // mismo conteo que SystemState::today
    uint16_t at = 0;     // minuto del día
    uint16_t ml = 0;
    bool typed = false;  // se capturó a mano con "Registrar otra cantidad"
};

struct SystemState {
    // Qué está instalado
    bool has_pressure = false;  // fase 1.5: sensores P1 y P2
    bool has_orp = false;       // fase 2: sonda ORP en la cisterna
    bool has_mixer = false;     // fase 2: bomba R

    // Fecha, hora y red
    uint32_t today = 0;   // número de día desde una fecha fija: sirve para comparar días
    uint8_t weekday = 0;  // 0 domingo ... 6 sábado
    uint8_t day = 1;      // día del mes
    uint8_t month = 1;    // 1 enero ... 12 diciembre
    uint8_t hour = 0;
    uint8_t minute = 0;
    bool wifi_ok = false;

    // Flujo
    float flow_lpm = 0.0f;
    uint32_t liters_today = 0;
    uint32_t liters_since_dose = 0;
    std::array<float, kFlowSamples24h> flow_24h{};

    // Dosis a mano
    float ml_per_liter = 0.020f;               // mL de Cloralex por litro
    uint16_t daily_cap_ml = 120;               // tope al día (lo usa la fase 3)
    std::array<DoseRecord, 5> recent_doses{};  // la más nueva primero
    uint8_t recent_dose_count = 0;

    // Presión antes (P1) y después (P2) de los filtros
    int p1_psi = 0;
    int p2_psi = 0;

    // ORP de la cisterna
    int orp_mv = 0;

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

// ORP de la cisterna con los umbrales del DS:
// 650 mV o más Oxidante, de -50 a 649 Neutro, menos de -50 Reductor
constexpr Level orp_level(int mv) {
    if (mv < -50) return Level::alarm;
    if (mv < 650) return Level::warning;
    return Level::ok;
}

// La pastilla del encabezado: lo peor de lo que se está vigilando
inline Level overall_level(const SystemState& s) {
    Level worst = Level::ok;
    if (s.has_pressure) worst = pressure_drop_level(s.p1_psi - s.p2_psi);
    if (s.has_orp && orp_level(s.orp_mv) > worst) worst = orp_level(s.orp_mv);
    return worst;
}

inline uint16_t minute_of_day(const SystemState& s) {
    return static_cast<uint16_t>(s.hour * 60 + s.minute);
}

}  // namespace core
