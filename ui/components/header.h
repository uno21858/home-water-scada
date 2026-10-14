#pragma once
// Encabezado de 1024 x 64: título a la izquierda; pastilla de estado,
// WiFi y hora a la derecha. Se crea una vez y nunca se recrea.

#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class Header {
public:
    void create(lv_obj_t* parent);
    void set_title(const char* title);
    void set_level(core::Level level);
    void set_time(uint8_t hour, uint8_t minute);
    void set_wifi(bool ok);

private:
    lv_obj_t* title_ = nullptr;
    lv_obj_t* pill_ = nullptr;
    lv_obj_t* pill_icon_ = nullptr;
    lv_obj_t* pill_text_ = nullptr;
    lv_obj_t* wifi_ = nullptr;
    lv_obj_t* clock_ = nullptr;
    int level_ = -1;    // -1 = todavía no se pinta
    int wifi_ok_ = -1;
};

}  // namespace ui
