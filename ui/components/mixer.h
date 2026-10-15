#pragma once
// Piezas de la bomba R que comparten Resumen y Mezcla: el impulsor que gira
// cuando está prendida y el renglón que dice por qué está así.

#include <cstddef>
#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

// Aro de 52 px con las aspas de 56 encima. Prendida: aspas en agua, una
// vuelta cada 2 s. Apagada: aspas en tubería, quietas.
class Impeller {
public:
    void create(lv_obj_t* parent, int32_t x, int32_t y);  // x, y: esquina de las aspas
    void set_spinning(bool on);

private:
    lv_obj_t* rotor_ = nullptr;
    int spinning_ = -1;  // -1 = todavía no se pinta
};

// "Auto · programa de las 18:30", "A mano · se apaga sola a las 19:42"...
void mixer_status(char* out, size_t size, const core::SystemState& s);

}  // namespace ui
