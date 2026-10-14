#pragma once
// Pantalla Tren (inicio): el P&ID del tren de filtros con P1, P2, F, B,
// la cisterna y la franja de abajo. Por ahora es un aviso; se llena después.

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class HomeScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);
};

}  // namespace ui
