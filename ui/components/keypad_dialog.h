#pragma once
// Teclado numérico para capturar una cantidad entera de hasta 3 cifras
// (por ahora los mL de "Registrar otra cantidad"). Vive en lv_layer_top()
// con el mismo velo que el diálogo de confirmación.

#include <cstdint>

#include "lvgl.h"

namespace ui {

class KeypadDialog {
public:
    using Callback = void (*)(uint32_t value);

    void create(lv_obj_t* layer);
    // hint: "Sugerido: 37 mL"; date y time: cuándo se va a registrar
    void open(const char* title, const char* hint, const char* date, const char* time, Callback on_save);
    void close();

private:
    static void handle_key(lv_event_t* e);
    static void handle_cancel(lv_event_t* e);
    static void handle_save(lv_event_t* e);
    void press(char key);
    void refresh();

    lv_obj_t* veil_ = nullptr;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* value_ = nullptr;
    lv_obj_t* cursor_ = nullptr;
    lv_obj_t* hint_ = nullptr;
    lv_obj_t* date_ = nullptr;
    lv_obj_t* time_ = nullptr;
    lv_obj_t* save_ = nullptr;
    char digits_[4] = "";  // hasta 3 cifras y el '\0'
    Callback on_save_ = nullptr;
};

}  // namespace ui
