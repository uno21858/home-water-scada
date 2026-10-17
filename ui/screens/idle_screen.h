#pragma once
// Pasillo: el modo de reposo. Si nadie toca la pantalla en 1, 2 o 5 min
// (Ajustes), tapa todo con cifras para leer de lejos: caudal, lo que toca
// echar, litros de hoy, caída del tren, ORP y cisterna. Un toque en
// cualquier lado regresa al Tren.
//
// Con sensor de nivel, el fondo es el agua de la cisterna a su altura, con
// una olita en la superficie. El agua quieta solo se redibuja cuando cambia
// el %; la olita es una franja de kWaveH px que un timer redibuja mientras
// el Pasillo está abierto: así la 7B no repinta toda la pantalla.
//
// No es una página del contenido: va en lv_layer_top(), encima de todo,
// sin encabezado ni barra.

#include <array>
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
    static void tick_wave(lv_timer_t* timer);   // mueve la olita un poco
    static void draw_wave(lv_event_t* e);       // pinta la franja de la superficie
    void layout_dose(bool three_digits);
    void place_water(int pct);                  // -1 = sin nivel: sin agua

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

    // Agua de la cisterna de fondo
    lv_obj_t* water_ = nullptr;  // de abajo de la olita al fondo de la pantalla
    lv_obj_t* wave_ = nullptr;   // franja de la superficie
    lv_timer_t* wave_timer_ = nullptr;
    float wave_phase_ = 0.0f;
    std::array<lv_point_precise_t, 1024 / 8 + 1> crest_{};  // vivos hasta que termine el dibujo
    lv_obj_t* cistern_ = nullptr;  // "Cisterna 73 %" abajo a la derecha
    lv_obj_t* cistern_value_ = nullptr;
    int water_pct_ = -2;  // -2 = todavía no se pinta

    // -1 = todavía no se pinta
    int level_ = -1;
    int flow_state_ = -1;  // 0 con flujo, 1 sin flujo, 2 sin agua
    int drop_level_ = -1;
    int three_digits_ = -1;
};

}  // namespace ui
