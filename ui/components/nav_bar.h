#pragma once
// Barra de navegación de 1024 x 80 con 7 pestañas de 146 o 147 px.
// La activa lleva fondo panel-alto y una barra de 4 px arriba.

#include <array>

#include "lvgl.h"
#include "ui.h"

namespace ui {

class NavBar {
public:
    void create(lv_obj_t* parent);
    void set_active(Screen screen);

private:
    struct Tab {
        lv_obj_t* obj = nullptr;
        lv_obj_t* icon = nullptr;
        lv_obj_t* text = nullptr;
    };
    std::array<Tab, kScreenCount> tabs_{};
};

}  // namespace ui
