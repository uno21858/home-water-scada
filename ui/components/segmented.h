#pragma once
// Selector segmentado de 64 px de alto: de 2 a 4 opciones y una elegida.
// La elegida va en accion (relleno claro, texto oscuro); las demás en texto-2.
// Con ancho fijo las opciones se reparten parejo; con w = 0 cada una mide su
// texto más 18 px por lado.

#include <array>
#include <cstdint>

#include "lvgl.h"

namespace ui {

class Segmented {
public:
    using Callback = void (*)(int index, void* user_data);

    // Regresa el marco, por si hay que alinearlo (a la derecha, por ejemplo)
    lv_obj_t* create(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const char* const* options, int count,
                     Callback on_pick, void* user_data = nullptr);
    void set_selected(int index);

private:
    static void handle_click(lv_event_t* e);

    std::array<lv_obj_t*, 4> buttons_{};
    std::array<lv_obj_t*, 4> texts_{};
    int count_ = 0;
    int selected_ = -1;  // -1 = ninguna todavía
    Callback on_pick_ = nullptr;
    void* user_data_ = nullptr;
};

}  // namespace ui
