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

// Historial: los últimos 30 días, hoy al final con lo que va
inline constexpr int kHistoryDays = 30;

// Pulsos por litro del YF-B6 según su hoja de datos (F = 6.6 x Q)
inline constexpr uint16_t kFactoryPulsesPerLiter = 396;

enum class Level : uint8_t { ok, warning, alarm };

// Lo que se guarda de un día o de una hora del historial
struct Totals {
    uint16_t liters = 0;
    uint16_t dose_ml = 0;
    uint8_t p1_psi = 0;  // promedio
    uint8_t p2_psi = 0;
    int16_t orp_mv = 0;  // promedio
};

inline bool operator==(const Totals& a, const Totals& b) {
    return a.liters == b.liters && a.dose_ml == b.dose_ml && a.p1_psi == b.p1_psi && a.p2_psi == b.p2_psi &&
           a.orp_mv == b.orp_mv;
}
inline bool operator!=(const Totals& a, const Totals& b) { return !(a == b); }

// Fecha sin año, como se muestra: "Vie 2 oct"
struct Date {
    uint8_t weekday = 0;  // 0 domingo ... 6 sábado
    uint8_t day = 1;
    uint8_t month = 1;
};

// Cartuchos del tren, en el orden del agua
enum class Cartridge : uint8_t { pp20, pp10, pp5, carbon, scale };

enum class EventKind : uint8_t {
    dose,          // "Ya le puse"
    dose_typed,    // "Registrar otra cantidad"
    cartridge,     // cambio de cartucho
    drop_warning,  // caída del tren en Revisar
    drop_alarm,    // caída del tren en Tapados
};

// Algo que pasó, para la lista de Eventos
struct Event {
    uint32_t day = 0;   // mismo conteo que SystemState::today
    uint16_t at = 0;    // minuto del día
    EventKind kind = EventKind::dose;
    uint16_t value = 0; // mL de la dosis, psi de la caída o el Cartridge que se cambió
};

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
    uint16_t year = 2026;
    uint8_t hour = 0;
    uint8_t minute = 0;
    bool wifi_ok = false;
    char wifi_ssid[33] = "";      // nombre de la red, hasta 32 letras
    uint8_t wifi_bars = 0;        // señal de 0 a 3
    uint16_t last_upload_at = 0;  // minuto del día del último envío de datos
    uint16_t pending_uploads = 0; // lecturas guardadas que faltan por enviar

    // Flujo
    uint16_t flow_pulses_per_liter = kFactoryPulsesPerLiter;  // factor del sensor F
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
    float p1_raw_psi = 0.0f;  // con un decimal, para hacer cero con el tren sin presión
    float p2_raw_psi = 0.0f;

    // Cartuchos
    std::array<uint8_t, 5> spares{};       // repuestos en casa, por Cartridge
    Cartridge last_cartridge = Cartridge::pp20;
    uint32_t last_cartridge_day = 0;       // mismo conteo que today; 0 = no hay registro

    // Pantalla
    bool night_off = true;                 // apagar la luz de noche
    uint16_t night_from = 23 * 60;         // minuto del día
    uint16_t night_to = 6 * 60 + 30;
    uint8_t idle_minutes = 2;              // modo pasillo si nadie toca en 1, 2 o 5 min

    // ORP de la cisterna
    int orp_mv = 0;
    std::array<int16_t, kOrpSamples7d> orp_7d{};
    float chlorine_mgl = 0.0f;  // cloro libre estimado con el ORP y la última tira
    StripCheck last_strip{};

    // Historial y eventos
    std::array<Totals, kHistoryDays> days{};  // el último es hoy
    std::array<Totals, 24> hours{};           // las horas de hoy; valen de 0 a la de ahorita
    std::array<Event, 5> recent_events{};     // el más nuevo primero
    uint8_t recent_event_count = 0;

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

inline uint8_t days_in_month(uint8_t month, uint16_t year) {
    constexpr uint8_t kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    return month == 2 && leap ? 29 : kDays[(month + 11) % 12];
}

inline Date today(const SystemState& s) { return {s.weekday, s.day, s.month}; }

// La fecha de hace n días: 3 oct y 29 días -> 4 sep. year es el de `from`.
inline Date date_days_ago(Date from, uint16_t year, uint32_t n) {
    Date d = from;
    d.weekday = static_cast<uint8_t>((from.weekday + 7 - n % 7) % 7);
    while (n >= d.day) {  // retrocede mes por mes hasta que quepa
        n -= d.day;
        if (d.month == 1) --year;
        d.month = static_cast<uint8_t>(d.month == 1 ? 12 : d.month - 1);
        d.day = days_in_month(d.month, year);
    }
    d.day = static_cast<uint8_t>(d.day - n);
    return d;
}

}  // namespace core
