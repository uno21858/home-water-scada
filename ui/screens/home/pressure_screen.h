#pragma once
// Detalle de presión P1 (entrada) y P2 (retorno): las dos cifras, la caída
// del tren (abre su detalle), las últimas 24 h, el voltaje que lee el
// ADS1115 y el cero de los sensores.

#include <array>
#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class PressureScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    void create_readings(lv_obj_t* page);
    void create_drop(lv_obj_t* page);
    void create_chart(lv_obj_t* page);
    void create_side(lv_obj_t* page);
    void update_chart(const core::SystemState& s);

    lv_obj_t* live_ = nullptr;
    lv_obj_t* locked_ = nullptr;

    lv_obj_t* p1_ = nullptr;
    lv_obj_t* p2_ = nullptr;

    lv_obj_t* drop_ = nullptr;
    lv_obj_t* drop_value_ = nullptr;
    lv_obj_t* drop_word_ = nullptr;
    int drop_level_ = -1;  // -1 = todavía no se pinta

    lv_obj_t* chart_ = nullptr;
    lv_chart_series_t* p1_series_ = nullptr;
    lv_chart_series_t* p2_series_ = nullptr;
    std::array<lv_obj_t*, 2> nights_{};    // bandas de 0:00 a 6:00
    lv_obj_t* night_label_ = nullptr;
    std::array<lv_obj_t*, 5> hour_labels_{};
    std::array<uint8_t, 24> shown_p1_{};
    std::array<uint8_t, 24> shown_p2_{};
    int shown_hour_ = -1;

    lv_obj_t* a0_ = nullptr;
    lv_obj_t* a1_ = nullptr;
    lv_obj_t* zero_p1_ = nullptr;
    lv_obj_t* zero_p2_ = nullptr;
};

}  // namespace ui
