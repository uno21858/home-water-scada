#pragma once
// Estado del sistema que muestra la pantalla.
// Lógica pura: sin LVGL ni hardware, así lo usan igual el firmware y el simulador.

#include <array>
#include <cstdint>

namespace core {

// Caudal de las últimas 24 h, una muestra cada 5 min: 24 * 12 + 1 = 289.
// La última muestra es la de ahorita.
inline constexpr int kFlowSamples24h = 289;

// Caudal de las últimas 2 h, una muestra por minuto: 120 + 1 = 121.
// La última muestra es la de ahorita.
inline constexpr int kFlowSamples2h = 121;

// ORP de la cisterna de los últimos 7 días, una muestra por hora: 7 * 24 = 168.
// La última muestra es la de ahorita.
inline constexpr int kOrpSamples7d = 168;

enum class Level : uint8_t { ok, warning, alarm };

enum class MixerMode : uint8_t { off, on, automatic };

// Por qué se prendió la bomba R
enum class MixerReason : uint8_t { schedule, dose, manual };

// Una vez que se prendió la bomba R
struct MixerRun {
    uint16_t at = 0;      // minuto del día en que arrancó
    uint8_t minutes = 0;  // cuánto corrió
    MixerReason reason = MixerReason::schedule;
};

struct DoseRecord {
    uint32_t day = 0;    // mismo conteo que SystemState::today
    uint16_t at = 0;     // minuto del día
    uint16_t ml = 0;
    bool typed = false;  // se capturó a mano con "Registrar otra cantidad"
};

// Calibración del cloro libre con una tira: lo que marcó y el ORP de ese momento
struct StripCheck {
    uint8_t weekday = 0;     // 0 domingo ... 6 sábado
    uint8_t day = 0;         // día del mes; 0 = todavía no se calibra
    uint8_t month = 0;
    uint8_t tenths_mgl = 0;  // cloro libre en décimas de mg/L: 5 = 0.5 mg/L
    int16_t orp_mv = 0;
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
    std::array<float, kFlowSamples2h> flow_2h{};

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
    std::array<int16_t, kOrpSamples7d> orp_7d{};
    float chlorine_mgl = 0.0f;  // cloro libre estimado con el ORP y la última tira
    StripCheck last_strip{};

    // Bomba R de la cisterna
    MixerMode mixer_mode = MixerMode::automatic;
    bool mixer_on = false;
    uint8_t mixer_minutes_left = 0;
    uint8_t mixer_progress_pct = 0;
    uint16_t mixer_started_at = 0;  // minuto del día en que arrancó
    uint16_t mixer_next_at = 0;     // minuto del día de la siguiente vuelta en Auto
    uint16_t mixer_today_minutes = 0;      // cuánto lleva prendida hoy
    uint8_t mixer_today_runs = 0;          // cuántas veces se prendió hoy
    std::array<MixerRun, 4> mixer_runs{};  // las últimas, la más nueva primero;
    uint8_t mixer_run_count = 0;           // si está prendida, la primera es la de ahorita

    // Programa del modo Auto y el tope a mano, en minutos
    uint8_t mixer_after_dose_min = 30;  // corre después de cada dosis
    uint8_t mixer_every_4h_min = 15;    // corre cada 4 horas
    uint8_t mixer_manual_max_min = 60;  // a mano se apaga sola
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
