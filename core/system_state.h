#pragma once
// Estado del sistema que muestra la pantalla.
// Lógica pura: sin LVGL ni hardware, así lo usan igual el firmware y el simulador.

#include <algorithm>
#include <array>
#include <cstddef>
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

// Lo que filtra, en el orden del agua: el prefiltro de malla y los 5 vasos
enum class Filter : uint8_t { prefilter, pp20, pp10, pp5, carbon, scale };
inline constexpr int kFilterCount = 6;

// Cada cuánto se purga o se cambia cada filtro, en días (DS): prefiltro, 20 µm,
// 10 µm, 5 µm, CTO y sarro
inline constexpr uint16_t kFilterLifeDays[kFilterCount] = {7, 75, 135, 135, 180, 365};

// Cómo va cada filtro desde que se purgó o se cambió
struct FilterStatus {
    uint32_t since_day = 0;  // mismo conteo que SystemState::today; 0 = sin fecha (nunca se registró)
    uint32_t liters = 0;     // litros que han pasado desde entonces
    bool installed = true;   // el vaso del sarro puede estar vacío
};

// Una lectura de caída contra caudal, para ver si los cartuchos se tapan
struct DropSample {
    uint32_t day = 0;         // mismo conteo que SystemState::today
    uint8_t flow_tenths = 0;  // L/min x 10
    uint8_t drop_tenths = 0;  // psi x 10
};
inline constexpr int kDropSamples = 42;

// Alarmas a pantalla completa (DS: las tres de la fase 3)
enum class AlarmKind : uint8_t {
    no_flow,    // sin flujo mientras dosifica
    sensor,     // F sin pulsos aunque la caída dice que pasa agua
    daily_cap,  // la bomba B llegó al tope del día
};
inline constexpr int kAlarmKinds = 3;

// Una alarma activa. Al tocar Entendido se reconoce y se quita de la
// pantalla, pero sigue activa (y la pastilla en Alarma) hasta que la
// condición se quite.
struct Alarm {
    bool active = false;
    bool acknowledged = false;
    uint16_t since = 0;  // minuto del día en que empezó
};

enum class EventKind : uint8_t {
    dose,          // "Ya le puse"
    dose_typed,    // "Registrar otra cantidad"
    cartridge,     // cambio de cartucho
    cartridge_added,  // se puso cartucho en un vaso vacío (el del sarro)
    purge,         // purga del prefiltro
    alarm,         // una alarma; value es su AlarmKind
    drop_warning,  // caída del tren en Revisar
    drop_alarm,    // caída del tren en Tapados
    offline,       // se cayó la red; value son los minutos sin red
    mixer_fault,   // aviso de la bomba R; value es su MixerFault
    no_supply,     // se fue el agua del SIAPA
};

inline constexpr uint16_t kNoTime = 0xFFFF;  // "todavía no" en un minuto del día
inline constexpr int kEventCount = 8;

// Algo que pasó, para la lista de Eventos
struct Event {
    uint32_t day = 0;   // mismo conteo que SystemState::today
    uint16_t at = 0;    // minuto del día
    EventKind kind = EventKind::dose;
    uint16_t value = 0; // mL de la dosis, psi de la caída o el Filter que se cambió
    uint16_t acknowledged_at = kNoTime;  // alarmas: cuándo tocaron Entendido
    uint16_t resolved_at = kNoTime;      // alarmas y avisos: cuándo se quitó
    uint32_t seq = 0;                    // número del evento: el servidor recibe los que no ha visto
};

// Lo que responde. En el I2C de la 7B (GPIO8/9): táctil, expansor, ADS1115,
// INA219 y, en fase 3, el PCF8574. La sonda ORP va aparte, por RS485.
struct Devices {
    bool ads1115 = true;   // 0x48: P1, P2 y el LDR
    bool gt911 = true;     // 0x5D: táctil
    bool expander = true;  // 0x24: CH32V003 de la 7B, PWM del backlight
    bool ina219 = true;    // 0x40: voltaje y corriente de la bomba R
    bool pcf8574 = false;  // 0x20: pulsos a las bombas A y B (fase 3)
    bool ezo_orp = false;  // EZO-ORP por RS485 (fase 2)
};

enum class MixerMode : uint8_t { off, on, automatic };

// Por qué se prendió la bomba R
enum class MixerReason : uint8_t {
    water,   // Auto: entraron kMixEveryLiters de agua nueva desde la última vuelta
    dose,    // Auto: se registró una dosis
    manual,  // la prendieron a mano
};

// Auto mezcla cada que entran estos litros: el 10 % de la cisterna de 10 m³.
// Es el agua nueva sin cloro la que vuelve a dejar dispareja la cisterna.
inline constexpr uint16_t kMixEveryLiters = 1000;

// Lo que vio el INA219 al arrancar la bomba R o mientras corre (documento,
// pág. 40). El firmware la apaga, lo pone aquí y lo quita cuando se arregla.
enum class MixerFault : uint8_t {
    none,
    no_power,  // sin 12 V (apagón o fusible): la mezcla queda pendiente
    dry,       // corriente baja, en seco: nivel bajo; espera agua nueva
    stuck,     // corriente alta: atorada
};

// Una vez que se prendió la bomba R
struct MixerRun {
    uint16_t at = 0;      // minuto del día en que arrancó
    uint8_t minutes = 0;  // cuánto corrió
    MixerReason reason = MixerReason::water;
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
    bool has_mixer = false;     // fase 1: bomba R de mezcla (MOSFET en GPIO44)
    bool has_ldr = false;       // LDR en A3 para el brillo de la pantalla
    bool has_level = false;     // sensor de nivel 4-20 mA en A2 (core/level.h)
    bool has_pumps = false;     // fase 3: bombas A y B de cloro
    bool ro_kitchen_planned = false;  // Panorama: el RO de la cocina, por instalar

    // Fecha, hora y red
    uint32_t today = 0;   // número de día desde una fecha fija: sirve para comparar días
    uint8_t weekday = 0;  // 0 domingo ... 6 sábado
    uint8_t day = 1;      // día del mes
    uint8_t month = 1;    // 1 enero ... 12 diciembre
    uint16_t year = 2026;
    uint8_t hour = 0;
    uint8_t minute = 0;
    bool wifi_ok = false;
    uint32_t offline_seconds = 0;  // cuánto lleva sin red (network_tick)
    bool flow_sensor_ok = true;   // llegan pulsos de F cuando debe
    std::array<Alarm, kAlarmKinds> alarms{};  // una por AlarmKind
    char wifi_ssid[33] = "";      // nombre de la red, hasta 32 letras
    char wifi_ip[16] = "";        // "192.168.1.120": a dónde manda subir-ota.sh
    uint8_t wifi_bars = 0;        // señal de 0 a 3
    uint16_t last_upload_at = 0;  // minuto del día del último envío de datos
    uint32_t pending_uploads = 0; // lecturas y eventos que faltan por enviar (core::Outbox)
    bool has_server = false;      // hay a dónde mandarlos (la URL de VictoriaMetrics)

    // Flujo
    uint16_t flow_pulses_per_liter = kFactoryPulsesPerLiter;  // factor del sensor F
    float flow_lpm = 0.0f;
    uint32_t liters_today = 0;
    uint32_t liters_since_dose = 0;
    std::array<float, kFlowSamples24h> flow_24h{};
    std::array<float, kFlowSamples2h> flow_2h{};

    // Dosis a mano
    float ml_per_liter = 0.100f;               // mL de Cloralex por litro (prueba de jarras: 100 por 1,000 L)
    uint16_t daily_cap_ml = 120;               // tope al día de cada bomba (fase 3)
    std::array<DoseRecord, 5> recent_doses{};  // la más nueva primero
    uint8_t recent_dose_count = 0;

    // Presión antes (P1) y después (P2) de los filtros
    int p1_psi = 0;
    int p2_psi = 0;
    float p1_zero_psi = 0.0f;  // corrimiento del cero guardado con Hacer cero
    float p2_zero_psi = 0.0f;
    float p1_volts = 0.0f;     // salida de P1 y P2: el ADS1115 lee la mitad por el divisor 10k/10k
    float p2_volts = 0.0f;
    float ldr_volts = 0.0f;    // A3: la luz que le llega a la pantalla
    float level_volts = 0.0f;  // A2: el lazo 4-20 mA del nivel sobre la de 33 Ω
    float mixer_volts = 0.0f;  // INA219 en la rama de la bomba R: 0 V es apagón o fusible
    float mixer_amps = 0.0f;   // ~1.6 A sumergida; menos es en seco, más es atorada
    MixerFault mixer_fault = MixerFault::none;
    std::array<uint8_t, 24> p1_24h{};  // una por hora, la última es la de ahorita
    std::array<uint8_t, 24> p2_24h{};

    // Caída contra caudal: con cartuchos nuevos y las lecturas del último mes
    std::array<uint8_t, 16> drop_new_tenths{};  // caída nueva a 0, 1 ... 15 L/min, en décimas
    uint32_t drop_new_day = 0;                  // cuándo se midió con cartuchos nuevos
    std::array<DropSample, kDropSamples> drop_samples{};  // la más vieja primero
    uint8_t drop_sample_count = 0;

    // Filtros y cartuchos
    std::array<FilterStatus, kFilterCount> filters{};
    std::array<uint8_t, kFilterCount> spares{};  // repuestos en casa, por Filter
    Filter last_cartridge = Filter::pp20;        // el último que se cambió
    uint32_t last_cartridge_day = 0;             // mismo conteo que today; 0 = no hay registro

    // Diagnóstico
    Devices devices{};
    char firmware[12] = "";       // versión: "0.1"
    uint32_t uptime_minutes = 0;  // desde que se prendió
    uint32_t bucket_pulses = 0;   // pulsos de F desde que empezó la prueba de cubeta

    // Pantalla
    uint8_t idle_minutes = 2;              // modo pasillo si nadie toca en 1, 2 o 5 min

    // ORP de la cisterna
    int orp_mv = 0;
    std::array<int16_t, kOrpSamples7d> orp_7d{};
    float chlorine_mgl = 0.0f;  // cloro libre estimado con el ORP y la última tira
    StripCheck last_strip{};

    // Historial y eventos
    std::array<Totals, kHistoryDays> days{};  // el último es hoy
    std::array<Totals, 24> hours{};           // las horas de hoy; valen de 0 a la de ahorita
    std::array<Event, kEventCount> recent_events{};  // el más nuevo primero
    uint8_t recent_event_count = 0;
    uint32_t event_seq = 0;  // el seq del último evento que se anotó

    // Bomba R de la cisterna (la mueve core/mixing.h)
    MixerMode mixer_mode = MixerMode::automatic;
    bool mixer_on = false;
    uint8_t mixer_minutes_left = 0;
    uint8_t mixer_run_minutes = 0;  // cuánto dura la vuelta de ahorita
    uint8_t mixer_progress_pct = 0;
    uint16_t mixer_started_at = 0;  // minuto del día en que arrancó
    uint16_t liters_since_mix = 0;  // agua nueva desde que arrancó la última vuelta
    uint8_t mixer_pending_min = 0;  // vuelta que no pudo correr por un aviso; 0 = ninguna
    MixerReason mixer_pending_reason = MixerReason::water;
    uint16_t mixer_today_minutes = 0;      // cuánto lleva prendida hoy
    uint8_t mixer_today_runs = 0;          // cuántas veces se prendió hoy
    std::array<MixerRun, 4> mixer_runs{};  // las últimas, la más nueva primero;
    uint8_t mixer_run_count = 0;           // si está prendida, la primera es la de ahorita

    // Programa del modo Auto y el tope a mano, en minutos
    uint8_t mixer_after_dose_min = 45;    // corre después de cada dosis (el documento: ~45)
    uint8_t mixer_every_liters_min = 15;  // corre cada kMixEveryLiters de agua nueva
    uint8_t mixer_manual_max_min = 60;    // a mano se apaga sola

    // Calibración del sensor de nivel (Cisterna > Calibrar nivel), en cm
    float level_zero_volts = 0.132f;  // lo que marca en el aire; de fábrica 4 mA x 33 Ω
    uint16_t level_range_cm = 200;    // rango de la etiqueta: 0 a 2 m es 200
    uint8_t level_sensor_cm = 3;      // a cuánto del fondo quedó el sensor
    uint8_t level_intake_cm = 60;     // el 0 %: la rejilla de la bomba de pozo (documento: ~45)
                                      // y un colchón; con 60 la bomba R (~50) sigue cubierta
    uint16_t level_full_cm = 0;       // nivel con la cisterna llena; 0 = sin marcar

    // Cambio de cartuchos: con V1 cerrada P1 baja a 0 igual que si se fuera el
    // SIAPA, así que mientras dura no se avisa ni la caída ni el sin agua.
    uint8_t service_minutes_left = 0;  // 0 = no se está cambiando nada
};

// Lo que dura el modo de cambio de cartuchos si nadie toca "Ya terminé"
inline constexpr uint8_t kServiceMinutes = 60;
inline bool in_service(const SystemState& s) { return s.service_minutes_left > 0; }

// mL de Cloralex que tocan: litros desde la última dosis por la relación
inline uint32_t dose_ml(const SystemState& s) {
    return static_cast<uint32_t>(static_cast<float>(s.liters_since_dose) * s.ml_per_liter + 0.5f);
}

// Abajo de esto F no cuenta y se toma como sin flujo
inline constexpr float kFlowingLpm = 0.05f;

// Caída que tenían los cartuchos nuevos a ese caudal, entre las de cada L/min.
// Sin línea base guardada la curva es 0 y la caída cuenta completa.
inline float baseline_drop(const SystemState& s, float lpm) {
    const float q = std::clamp(lpm, 0.0f, 15.0f);
    const int i = std::min(14, static_cast<int>(q));
    const float a = static_cast<float>(s.drop_new_tenths[static_cast<std::size_t>(i)]) / 10.0f;
    const float b = static_cast<float>(s.drop_new_tenths[static_cast<std::size_t>(i + 1)]) / 10.0f;
    return a + (b - a) * (q - static_cast<float>(i));
}

// Cuánto subió la caída (P1 - P2) sobre la de cartuchos nuevos al mismo caudal.
// Así 6 psi a 15 L/min con cartuchos nuevos no se confunde con tapados.
inline float drop_rise(const SystemState& s) {
    return static_cast<float>(s.p1_psi - s.p2_psi) - baseline_drop(s, s.flow_lpm);
}

// Documento, pág. 40: Revisar a +5 psi sobre la base y Tapados a +10
inline constexpr float kDropWarningRise = 5.0f;
inline constexpr float kDropAlarmRise = 10.0f;
constexpr Level drop_rise_level(float rise) {
    if (rise >= kDropAlarmRise) return Level::alarm;
    if (rise >= kDropWarningRise) return Level::warning;
    return Level::ok;
}
inline Level drop_level(const SystemState& s) { return drop_rise_level(drop_rise(s)); }

// Se fue el agua del SIAPA: sin flujo y P1 abajo de 3 psi al mismo tiempo.
// Sin flujo pero con presión es la cisterna llena (el flotador cerró). Sin P1
// no se sabe. El firmware pasa P1 ya promediado, así un brinco no lo prende.
inline constexpr int kNoSupplyPsi = 3;
inline bool supply_lost(const SystemState& s) {
    return s.has_pressure && !in_service(s) && s.flow_lpm < kFlowingLpm && s.p1_psi < kNoSupplyPsi;
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
    for (const Alarm& a : s.alarms) {
        if (a.active) return Level::alarm;
    }
    Level worst = Level::ok;
    if (s.has_pressure && !in_service(s)) worst = drop_level(s);
    if (s.has_orp && orp_level(s.orp_mv) > worst) worst = orp_level(s.orp_mv);
    // Avisos: la bomba R que no pudo mezclar y el SIAPA sin agua
    const bool notice = (s.has_mixer && s.mixer_fault != MixerFault::none) || supply_lost(s);
    if (notice && worst < Level::warning) worst = Level::warning;
    return worst;
}

// Si se sabe desde cuándo trae ese cartucho. Recién instalada la pantalla
// ninguno tiene fecha: se pone con "Lo cambié" o "Ya purgué".
inline bool filter_dated(const SystemState& s, Filter filter) {
    return s.filters[static_cast<std::size_t>(filter)].since_day != 0;
}

// Días desde que se purgó o se cambió; 0 si no tiene fecha
inline uint32_t filter_days(const SystemState& s, Filter filter) {
    const FilterStatus& f = s.filters[static_cast<std::size_t>(filter)];
    if (f.since_day == 0 || s.today < f.since_day) return 0;
    return s.today - f.since_day;
}

// Lo que lleva un cartucho de su vida, en %: puede pasar de 100 si ya toca.
// Sin cartucho o sin fecha, 0.
inline uint32_t filter_wear_pct(const SystemState& s, Filter filter) {
    const auto i = static_cast<std::size_t>(filter);
    if (!s.filters[i].installed) return 0;
    return (filter_days(s, filter) * 100 + kFilterLifeDays[i] / 2) / kFilterLifeDays[i];
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
