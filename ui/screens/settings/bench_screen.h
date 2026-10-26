#pragma once
// Prueba de banco de la bomba R (desde Diagnóstico, el enlace Banco): con la bomba
// en una cubeta, "Empezar" quita la protección de nivel. Prende aunque la
// cisterna esté en 0 % y el nivel bajo del INA219 no la apaga; atorada y sin
// 12 V siguen. Se quita con "Ya terminé" o al reiniciar (no se guarda). No
// está en el canvas: copia el acomodo de Cambio de cartuchos.

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class BenchScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    static void handle_button(lv_event_t* e);
    static void on_confirmed();  // "Sí, quitarla" del diálogo
    void show_state(bool active);

    lv_obj_t* title_ = nullptr;
    lv_obj_t* note_ = nullptr;
    lv_obj_t* button_text_ = nullptr;
    bool active_ = false;  // lo que se ve: el botón hace lo contrario
};

}  // namespace ui
