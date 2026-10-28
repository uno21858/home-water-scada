#pragma once
// Litros por cm de la cisterna (desde Calibrar nivel, "Medir"). Una Rotoplas
// no es pareja y no se saben sus medidas: con la bomba de pozo apagada y agua
// entrando, F cuenta los litros y el sensor de nivel los cm que sube
// (core/level.h). Con eso salen los litros que hay y el tope de la dosis. No
// está en el canvas: copia el acomodo de Prueba de cubeta, los pasos a la
// izquierda y a la derecha lo que va con el botón.

#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class LevelLitersScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    static void handle_primary(lv_event_t* e);

    lv_obj_t* value_ = nullptr;    // los litros por cm guardados, o los que van
    lv_obj_t* unit_ = nullptr;
    lv_obj_t* none_ = nullptr;     // "Sin medir"
    lv_obj_t* note_ = nullptr;
    lv_obj_t* primary_ = nullptr;  // Empezar o Guardar
    lv_obj_t* primary_text_ = nullptr;
    lv_obj_t* cancel_ = nullptr;
    bool measuring_ = false;       // lo que se ve: el botón hace lo que toca
};

}  // namespace ui
