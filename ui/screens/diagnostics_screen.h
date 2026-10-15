#pragma once
// Diagnóstico (desde Ajustes): qué lee cada entrada, qué responde en el bus
// I2C, cómo están las salidas y la red. Con Probar alarma y Reiniciar.

#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class DiagnosticsScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    lv_obj_t* di0_ = nullptr;
    lv_obj_t* a0_ = nullptr;
    lv_obj_t* a1_ = nullptr;
    lv_obj_t* ads_ = nullptr;
    lv_obj_t* gt911_ = nullptr;
    lv_obj_t* ch422g_ = nullptr;
    lv_obj_t* ezo_ = nullptr;
    lv_obj_t* do2_ = nullptr;
    lv_obj_t* wifi_name_ = nullptr;
    lv_obj_t* wifi_ = nullptr;
    lv_obj_t* server_ = nullptr;
    lv_obj_t* pending_ = nullptr;
    lv_obj_t* about_ = nullptr;
};

}  // namespace ui
