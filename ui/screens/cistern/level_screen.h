#pragma once
// Nivel de la cisterna (pestaña Cisterna mientras no haya sonda ORP; con la
// sonda, desde el enlace "Nivel" de Cisterna). No está en el canvas.
// A la izquierda el tanque con el agua: arriba de la rejilla de la bomba de
// pozo en azul, abajo en gris (esa no la saca), apretado a la quinta parte
// del tanque. A la derecha el % útil en grande, junto los litros que hay en
// total (con los litros por cm medidos), los cm y "Calibrar nivel".

#include <array>
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
    std::array<lv_obj_t*, 2> cm_units_{};  // "cm" de los dos que se miden: sin señal se esconden
    lv_obj_t* above_intake_ = nullptr;
    lv_obj_t* below_intake_ = nullptr;
    // Los litros que hay, a la derecha del %
    lv_obj_t* liters_ = nullptr;
    lv_obj_t* liters_row_ = nullptr;
    lv_obj_t* liters_value_ = nullptr;
    lv_obj_t* liters_none_ = nullptr;  // "Sin medir"
};

}  // namespace ui
