#pragma once
// Detalle del sensor de flujo F (YF-B6): el caudal con el estado de la
// señal, dónde cae en el rango del sensor, los pulsos que llegan a DI0, el
// factor K y los litros contados. Se llega desde Flujo y desde F en el Tren.

#include <array>
#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class FlowSensorScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    void create_reading(lv_obj_t* page);
    void create_range(lv_obj_t* page);
    void create_pulses(lv_obj_t* page);
    void create_side(lv_obj_t* page);
    void draw_wave(float hz);

    lv_obj_t* flow_value_ = nullptr;
    lv_obj_t* signal_ = nullptr;
    lv_obj_t* signal_icon_ = nullptr;
    lv_obj_t* signal_text_ = nullptr;
    int signal_ok_ = -1;  // -1 = todavía no se pinta

    lv_obj_t* range_fill_ = nullptr;
    lv_obj_t* range_marker_ = nullptr;

    // Onda cuadrada de los pulsos de los últimos 100 ms
    lv_obj_t* wave_ = nullptr;
    std::array<lv_point_precise_t, 128> wave_points_{};
    int32_t wave_tenths_hz_ = -1;  // la que está dibujada
    lv_obj_t* hz_ = nullptr;
    lv_obj_t* per_minute_ = nullptr;

    lv_obj_t* factor_ = nullptr;
    lv_obj_t* factor_note_ = nullptr;
    lv_obj_t* today_ = nullptr;
    lv_obj_t* since_dose_ = nullptr;
    lv_obj_t* week_ = nullptr;
};

}  // namespace ui
