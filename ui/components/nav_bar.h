#pragma once
// Barra de navegación de 1024 x 80 con 7 pestañas de 146 o 147 px.
// La activa lleva fondo panel-alto y una barra de 4 px arriba. Una pestaña
// escondida (Cisterna sin ORP ni bomba R) deja su ancho a las demás.

#include <array>

#include "lvgl.h"
#include "ui.h"

namespace ui {

class NavBar {
public:
    void create(lv_obj_t* parent);
    void set_active(Screen active);  // una de las 7 pestañas
    void set_tab_visible(Screen tab, bool visible);

private:
    struct Tab {
        lv_obj_t* obj = nullptr;
        lv_obj_t* icon = nullptr;
        lv_obj_t* text = nullptr;
    };
    void layout();  // reparte los 1024 px entre las que se ven

    std::array<Tab, kTabCount> tabs_{};
};

}  // namespace ui
