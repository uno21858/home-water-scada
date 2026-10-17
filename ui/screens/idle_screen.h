#pragma once
// Pasillo: el modo de reposo. Si nadie toca la pantalla en 1, 2 o 5 min
// (Ajustes), tapa todo con cifras para leer de lejos: caudal, lo que toca
// echar, litros de hoy, caída del tren y ORP. Un toque en cualquier lado
// regresa al Tren.
//
// No es una página del contenido: va en lv_layer_top(), encima de todo,
// sin encabezado ni barra.

#include <cstdint>

#include "components/pipe.h"
#include "lvgl.h"
#include "system_state.h"

namespace ui {

class IdleScreen {
public:
    using Callback = void (*)();

    void create(lv_obj_t* layer, Callback on_wake);
    void update(const core::SystemState& s);
    void open();
    void close();
    bool is_open() const;

private:
    static void handle_touch(lv_event_t* e);
    void layout_dose(bool three_digits);

    lv_obj_t* root_ = nullptr;
    Callback on_wake_ = nullptr;

    lv_obj_t* clock_ = nullptr;
    lv_obj_t* pill_ = nullptr;
    lv_obj_t* pill_icon_ = nullptr;
    lv_obj_t* pill_text_ = nullptr;

    lv_obj_t* flow_title_ = nullptr;
    lv_obj_t* flow_value_ = nullptr;

    lv_obj_t* dose_value_ = nullptr;
    lv_obj_t* dose_unit_ = nullptr;

    lv_obj_t* today_ = nullptr;
    lv_obj_t* drop_ = nullptr;   // grupo de la caída: se esconde sin P1 y P2
    lv_obj_t* drop_value_ = nullptr;
    lv_obj_t* drop_word_ = nullptr;
    lv_obj_t* orp_ = nullptr;    // grupo del ORP: se esconde sin sonda
    lv_obj_t* orp_value_ = nullptr;

    FlowPipes pipe_;

    // -1 = todavía no se pinta
    int level_ = -1;
    int flow_state_ = -1;  // 0 con flujo, 1 sin flujo, 2 sin agua
    int drop_level_ = -1;
    int three_digits_ = -1;
};

}  // namespace ui
