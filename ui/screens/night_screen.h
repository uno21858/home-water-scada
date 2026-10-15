#pragma once
// Pantalla apagada de noche: en el horario de Ajustes, cuando nadie toca en
// los minutos del Pasillo, se apaga la luz (Actions::backlight_set) y esta
// capa negra tapa todo. El primer toque solo la prende y regresa al Tren:
// a oscuras no se puede tocar "Ya le puse" sin querer. Una alarma la prende.
//
// Va en lv_layer_top(), encima del Pasillo y abajo de la alarma.

#include "lvgl.h"

namespace ui {

class NightScreen {
public:
    using Callback = void (*)();

    void create(lv_obj_t* layer, Callback on_wake);
    void open();
    void close();
    bool is_open() const;

private:
    static void handle_touch(lv_event_t* e);

    lv_obj_t* root_ = nullptr;
    Callback on_wake_ = nullptr;
};

}  // namespace ui
