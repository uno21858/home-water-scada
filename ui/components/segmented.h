#pragma once
// Selector segmentado de 64 px de alto: de 2 a 4 opciones y una elegida.
// La elegida va en accion (relleno claro, texto oscuro); las demás en texto-2.

#include <array>
#include <cstdint>

#include "lvgl.h"

namespace ui {

class Segmented {
public:
    using Callback = void (*)(int index);

    void create(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const char* const* options, int count,
                Callback on_pick);
    void set_selected(int index);

private:
    static void handle_click(lv_event_t* e);

    std::array<lv_obj_t*, 4> buttons_{};
    std::array<lv_obj_t*, 4> texts_{};
    int count_ = 0;
    int selected_ = -1;  // -1 = ninguna todavía
    Callback on_pick_ = nullptr;
};

}  // namespace ui
