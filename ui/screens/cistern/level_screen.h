#pragma once
// Nivel de la cisterna (pestaña Cisterna mientras no haya sonda ORP; con la
// sonda, desde el enlace "Nivel" de Cisterna). No está en el canvas.
// A la izquierda el tanque con el agua: arriba de la rejilla de la bomba de
// pozo en azul, abajo en gris (esa no la saca). A la derecha el % útil en
// grande, los cm y "Calibrar nivel".

#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class LevelScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    void create_tank(lv_obj_t* page);
    void create_reading(lv_obj_t* page);

    // Tanque
    lv_obj_t* usable_ = nullptr;   // agua arriba de la rejilla
    lv_obj_t* dead_ = nullptr;     // agua abajo de la rejilla
    lv_obj_t* surface_ = nullptr;  // raya del nivel
    lv_obj_t* full_line_ = nullptr;
    lv_obj_t* full_text_ = nullptr;
    lv_obj_t* intake_line_ = nullptr;
    lv_obj_t* intake_text_ = nullptr;

    // Lectura
    lv_obj_t* pct_ = nullptr;   // "78 %"; sin marcar llena se esconde
    lv_obj_t* unit_ = nullptr;
    lv_obj_t* uncalibrated_ = nullptr;
    lv_obj_t* from_bottom_ = nullptr;
    lv_obj_t* above_intake_ = nullptr;
    lv_obj_t* below_intake_ = nullptr;
};

}  // namespace ui
