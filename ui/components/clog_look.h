#pragma once
// Lo que dicen de los cartuchos las tarjetas del Tren, Resumen, Pasillo,
// Panorama, Presión y Caída: la misma regla en todas (core/clog.h), como
// pill_look para la pastilla.
//
// - Con P1 o P2 sin señal: "Sin señal" en ámbar.
// - Sin R0: "Sin referencia" en gris, sin colores de estado.
// - Con R0: el último estado (Normal, Revisar o Tapados) con sus colores,
//   aunque ahorita no pase agua. Sin flujo, cada tarjeta lo dice en gris y
//   enseña la hora de ese estado: nunca "Normal" por una caída de 0.

#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

struct ClogLook {
    // Orden de core::ClogState para los tres primeros; las pantallas guardan
    // el que pintaron para no repintar
    enum Kind : uint8_t { normal, check, clogged, no_reference, no_signal, waiting };
    Kind kind = no_reference;
    const char* word = "";       // "Normal", "Revisar", "Tapados", "Sin referencia", "Sin señal"
    uint32_t fg = 0;             // palabra e ícono
    uint32_t bg = 0;             // fondo de la caja
    uint32_t border = 0;
    const lv_image_dsc_t* icon = nullptr;  // de 28
    bool flowing = false;        // F de 1 L/min o más
    bool has_r = false;          // hay un R evaluado
    int pct = -1;                // R/R0 en %; -1 sin referencia o sin R
    char r[8] = "";              // "3.9"; de 10 para arriba sin decimal ("12"), para que quepa
    char at[8] = "";             // "18:42": cuándo se evaluó
};

ClogLook clog_look(const core::SystemState& s);

}  // namespace ui
