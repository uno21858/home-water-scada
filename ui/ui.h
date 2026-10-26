#pragma once
// Punto de entrada de la interfaz.
// La app (firmware o simulador) llama init() una vez y update() cada que
// cambia el estado. Lo que hace el usuario regresa a la app por Actions.

#include <cstdint>

#include "system_state.h"

namespace ui {

// Primero las 7 pestañas de la barra, en su orden. Después vienen las
// pantallas de detalle y de servicio: se llega a ellas desde una pestaña y la
// barra sigue marcando esa pestaña.
enum class Screen : uint8_t {
    home, summary, flow, cistern, dose, history, settings,  // pestañas
    mixer,          // Mezcla, desde Cisterna
    flow_sensor,    // Sensor de flujo F, desde Flujo
    pressure,       // Presión P1 y P2, desde el Tren
    pressure_drop,  // Caída del tren, desde el Tren, Resumen y Presión
    filters,        // Cartuchos, desde el Tren, la caída y Ajustes
    injection,      // Inyección A y B, desde Dosis
    events,         // Alarmas y eventos, desde Historial
    diagnostics,    // Diagnóstico, desde Ajustes
    bucket_test,    // Prueba de cubeta, desde Ajustes y Sensor F
    legend,         // Símbolos del diagrama, desde el "?" del Tren
    overview,       // Panorama de la casa, desde Casa en el Tren
    spares,         // Repuestos en casa, desde Ajustes > Cartuchos
    service,        // Cambio de cartuchos, desde Cartuchos
    level,          // Nivel de la cisterna: la pestaña Cisterna sin sonda ORP
    level_calibration,  // Calibrar nivel, desde Nivel
    bench,          // Prueba de banco de la bomba R, desde Diagnóstico
};
inline constexpr int kTabCount = 7;
inline constexpr int kScreenCount = 23;

// Lo que la interfaz le pide a la app. Cualquiera puede quedar en nullptr.
struct Actions {
    void (*dose_confirmed)(uint32_t ml) = nullptr;  // tocó "Sí, ya le puse"
    void (*dose_typed)(uint32_t ml) = nullptr;      // guardó otra cantidad en el teclado
    void (*strip_checked)(uint32_t tenths_mgl) = nullptr;  // guardó la tira: 5 = 0.5 mg/L
    void (*mixer_mode_set)(core::MixerMode mode) = nullptr;   // eligió Apagada, Prendida o Auto
    void (*mixer_after_dose_set)(uint32_t minutes) = nullptr; // programa: después de cada dosis
    void (*mixer_every_liters_set)(uint32_t minutes) = nullptr; // programa: cada 1,000 L que entran
    void (*mixer_bench_set)(bool on) = nullptr;  // Prueba de banco: Empezar (ya confirmado) o Ya terminé
    void (*flow_factor_set)(uint32_t pulses_per_liter) = nullptr;
    void (*pressure_zeroed)() = nullptr;                      // confirmó Hacer cero
    void (*dose_ratio_set)(uint32_t thousandths) = nullptr;   // mL por litro x 1000: 20 = 0.020
    void (*daily_cap_set)(uint32_t ml) = nullptr;
    void (*idle_minutes_set)(uint32_t minutes) = nullptr;     // modo pasillo: 1, 2 o 5
    void (*filter_serviced)(core::Filter filter) = nullptr;   // purgó el prefiltro o cambió un cartucho
    void (*spares_set)(core::Filter filter, uint32_t count) = nullptr;  // repuestos en casa, con − y +
    void (*alarm_acknowledged)(core::AlarmKind kind) = nullptr;  // tocó Entendido en la alarma
    void (*alarm_test)() = nullptr;            // Diagnóstico: Probar alarma
    void (*restart_requested)() = nullptr;     // Diagnóstico: Reiniciar, ya confirmado
    void (*bucket_test_started)() = nullptr;   // entró a la prueba de cubeta: bucket_pulses a 0
    void (*clog_reference_saved)() = nullptr;  // "Cartuchos nuevos: guardar referencia", ya confirmado (core/clog.h)
    void (*service_set)(bool on) = nullptr;    // Empezar o Ya terminé el cambio de cartuchos
    // Calibrar nivel
    void (*level_range_set)(uint32_t cm) = nullptr;   // rango de la etiqueta del sensor
    void (*level_zeroed)() = nullptr;                 // Hacer cero, con el sensor en el aire
    void (*level_sensor_set)(uint32_t cm) = nullptr;  // a cuánto del fondo quedó
    void (*level_intake_set)(uint32_t cm) = nullptr;  // la rejilla de la bomba de pozo
    void (*level_full_marked)() = nullptr;            // Está llena
};

struct KeypadSpec;  // components/keypad_dialog.h

void init(const core::SystemState& state, const Actions& actions);
void update(const core::SystemState& state);
void show(Screen screen);

// Abre "¿Ya le pusiste X mL?". Lo usan Resumen y Dosis.
void ask_dose_confirmation();

// Pregunta antes de hacer cero de P1 y P2. Lo usan Ajustes y Presión.
void ask_pressure_zero();

// Abre el teclado de "Registrar otra cantidad"
void open_dose_keypad();

// Abre el teclado de "Calibrar con tira" (Cisterna)
void open_strip_keypad();

// Abre el teclado con lo que diga spec; al guardar llama on_save con el valor
void open_keypad(const KeypadSpec& spec, void (*on_save)(uint32_t value));

// Muestra Pasillo ahorita (Ajustes → Verlo). Solo, sale a los minutos sin tocar.
void show_idle();

// Abre el diálogo de confirmación; on_confirm se llama si dice que sí
void open_confirm(const char* title, const char* body, const char* confirm_text, void (*on_confirm)());

// Lo que la app pidió que le avisaran (las pantallas llaman de aquí)
const Actions& actions();

}  // namespace ui
