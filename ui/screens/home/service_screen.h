#pragma once
// Cambio de cartuchos (desde Cartuchos): con V1 cerrada P1 baja a 0 igual que
// si se fuera el SIAPA. "Empezar" pausa ese aviso y el de la caída del tren
// hasta "Ya terminé" o core::kServiceMinutes. No está en el canvas: copia el
// acomodo de Prueba de cubeta, los pasos a la izquierda y a la derecha cómo
// van los avisos con el botón.

#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class ServiceScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    static void handle_button(lv_event_t* e);
    void show_state(bool active, uint8_t minutes_left);

    lv_obj_t* title_ = nullptr;
    lv_obj_t* note_ = nullptr;
    lv_obj_t* button_text_ = nullptr;
    bool active_ = false;  // lo que se ve: el botón hace lo contrario
};

}  // namespace ui
