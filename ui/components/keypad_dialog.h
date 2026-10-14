#pragma once
// Teclado numérico para capturar una cantidad: los mL de "Registrar otra
// cantidad", la tira de cloro y lo que se cambia en Ajustes. Vive en
// lv_layer_top() con el mismo velo que el diálogo de confirmación.
//
// No hay tecla de punto: con decimales, las cifras entran por la derecha
// como en una calculadora de caja. Con 3 decimales, teclear 2 y 0 da 0.020.

#include <cstdint>

#include "lvgl.h"

namespace ui {

// Qué se captura y cómo se muestra
struct KeypadSpec {
    const char* title;     // "Registrar otra cantidad"
    const char* caption;   // nombre del campo: "Cantidad"
    const char* unit;      // "mL"
    uint8_t decimals;      // 0 para enteros, 1 para 0.5, 3 para 0.020 (hasta 3)
    uint8_t max_digits;    // cuántas cifras se pueden teclear, hasta 4
    const char* hint;      // abajo del campo: "Sugerido: 37 mL"
    const char* date;      // chips de fecha y hora: cuándo se registra;
    const char* time;      // nullptr = sin chips
};

class KeypadDialog {
public:
    // value llega en la unidad más chica: con 1 decimal, 0.5 llega como 5
    using Callback = void (*)(uint32_t value);

    void create(lv_obj_t* layer);
    void open(const KeypadSpec& spec, Callback on_save);
    void close();

private:
    static void handle_key(lv_event_t* e);
    static void handle_cancel(lv_event_t* e);
    static void handle_save(lv_event_t* e);
    void press(char key);
    void refresh();

    lv_obj_t* veil_ = nullptr;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* caption_ = nullptr;
    lv_obj_t* value_ = nullptr;
    lv_obj_t* cursor_ = nullptr;
    lv_obj_t* unit_ = nullptr;
    lv_obj_t* hint_ = nullptr;
    lv_obj_t* when_ = nullptr;
    lv_obj_t* date_chip_ = nullptr;
    lv_obj_t* time_chip_ = nullptr;
    lv_obj_t* date_ = nullptr;
    lv_obj_t* time_ = nullptr;
    lv_obj_t* save_ = nullptr;
    char digits_[5] = "";  // hasta 4 cifras y el '\0'
    uint8_t decimals_ = 0;
    uint8_t max_digits_ = 3;
    Callback on_save_ = nullptr;
};

}  // namespace ui
