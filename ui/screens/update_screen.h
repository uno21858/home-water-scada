#pragma once
// Actualización por WiFi a pantalla completa: mientras la placa recibe o baja
// una versión nueva (subir-ota.sh o la URL de la OTA) dice de cuál a cuál,
// cuánto lleva y que no se desconecte; luego "Lista" y se reinicia sola. Si
// no se pudo, dice por qué y se quita sola: la que corre sigue igual.
// Va en lv_layer_top(), encima de todo, alarma incluida. No está en el canvas.

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class UpdateScreen {
public:
    void create(lv_obj_t* layer);
    void update(const core::SystemState& s);

private:
    lv_obj_t* root_ = nullptr;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* versions_ = nullptr;  // la fila "a7dc67c+ → a7bd074+"
    lv_obj_t* from_ = nullptr;
    lv_obj_t* arrow_ = nullptr;
    lv_obj_t* to_ = nullptr;
    lv_obj_t* amount_ = nullptr;  // la fila "45 %"
    lv_obj_t* pct_ = nullptr;
    lv_obj_t* track_ = nullptr;
    lv_obj_t* bar_ = nullptr;
    lv_obj_t* note_ = nullptr;
};

}  // namespace ui
