#pragma once
// Diálogo de confirmación antes de lo que reinicia algo (Ya le puse,
// Hacer cero, cambio de cartucho). Vive en lv_layer_top(), encima de todo,
// con un velo al 80 % que no deja tocar lo de abajo.

#include "lvgl.h"

namespace ui {

class ConfirmDialog {
public:
    using Callback = void (*)();

    void create(lv_obj_t* layer);
    void open(const char* title, const char* body, const char* confirm_text, Callback on_confirm);
    void close();

private:
    static void handle_cancel(lv_event_t* e);
    static void handle_confirm(lv_event_t* e);

    lv_obj_t* veil_ = nullptr;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* body_ = nullptr;
    lv_obj_t* confirm_text_ = nullptr;
    Callback on_confirm_ = nullptr;
};

}  // namespace ui
