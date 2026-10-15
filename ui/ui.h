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
    mixer,                                                  // Mezcla, desde Cisterna
};
inline constexpr int kTabCount = 7;
inline constexpr int kScreenCount = 8;

// Lo que la interfaz le pide a la app. Cualquiera puede quedar en nullptr.
struct Actions {
    void (*dose_confirmed)(uint32_t ml) = nullptr;  // tocó "Sí, ya le puse"
    void (*dose_typed)(uint32_t ml) = nullptr;      // guardó otra cantidad en el teclado
    void (*strip_checked)(uint32_t tenths_mgl) = nullptr;  // guardó la tira: 5 = 0.5 mg/L
    void (*mixer_mode_set)(core::MixerMode mode) = nullptr;   // eligió Apagada, Prendida o Auto
    void (*mixer_after_dose_set)(uint32_t minutes) = nullptr; // programa: después de cada dosis
    void (*mixer_every_4h_set)(uint32_t minutes) = nullptr;   // programa: cada 4 horas
};

struct KeypadSpec;  // components/keypad_dialog.h

void init(const core::SystemState& state, const Actions& actions);
void update(const core::SystemState& state);
void show(Screen screen);

// Abre "¿Ya le pusiste X mL?". Lo usan Resumen y Dosis.
void ask_dose_confirmation();

// Abre el teclado de "Registrar otra cantidad"
void open_dose_keypad();

// Abre el teclado de "Calibrar con tira" (Cisterna)
void open_strip_keypad();

// Abre el teclado con lo que diga spec; al guardar llama on_save con el valor
void open_keypad(const KeypadSpec& spec, void (*on_save)(uint32_t value));

// Lo que la app pidió que le avisaran (las pantallas llaman de aquí)
const Actions& actions();

}  // namespace ui
