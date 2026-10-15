#pragma once
// Alarma a pantalla completa: lo único que tapa los datos. Dice qué pasó, qué
// hizo el sistema y qué revisar, con los datos clave en chips y el botón
// Entendido. Va en lv_layer_top(), encima de todo, Pasillo incluido.
//
// Sale mientras haya una alarma activa sin reconocer. Entendido la reconoce
// (Actions::alarm_acknowledged); la pastilla del encabezado sigue en Alarma
// hasta que la condición se quite.

#include <array>
#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class AlarmScreen {
public:
    void create(lv_obj_t* layer);
    void update(const core::SystemState& s);

private:
    static void handle_ok(lv_event_t* e);

    lv_obj_t* root_ = nullptr;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* what_ = nullptr;
    lv_obj_t* check_ = nullptr;
    std::array<lv_obj_t*, 3> chips_{};
    lv_obj_t* count_ = nullptr;
    int shown_ = -1;  // AlarmKind en pantalla; -1 = ninguna
};

}  // namespace ui
