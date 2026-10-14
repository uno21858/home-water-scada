#pragma once
// Tuberías del P&ID con los puntos de flujo (la única animación del DS).
//
// Cada tramo recto es un objeto de 10 px de grueso que dibuja sus propios
// puntos en su evento de dibujo. Un solo lv_timer mueve la fase de todos e
// invalida cada tramo: son unas 10 áreas por cuadro. No se hace un objeto
// por punto: con 80 puntos se pasan de las áreas que guarda LVGL y redibuja
// la pantalla completa en cada cuadro.

#include <array>
#include <cstdint>

#include "lvgl.h"

namespace ui {

class FlowPipes {
public:
    enum class Dir : uint8_t { right, left, down, up };  // hacia dónde corre el agua

    void add(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, Dir dir);

    // Con 0 la tubería se pone gris y se quitan los puntos.
    // Con flujo los puntos avanzan a 4 px/s por cada L/min, de 12 a 90 px/s.
    void set_flow(float lpm);

private:
    struct Segment {
        lv_obj_t* obj = nullptr;
        Dir dir = Dir::right;
        const FlowPipes* owner = nullptr;
    };

    static void draw_dots(lv_event_t* e);
    static void tick(lv_timer_t* timer);

    std::array<Segment, 12> segments_{};
    int count_ = 0;
    lv_timer_t* timer_ = nullptr;
    int32_t phase_q8_ = 0;  // fase en 1/256 de px, para que se mueva parejo a cualquier velocidad
    int32_t speed_ = 0;     // px/s; 0 = sin flujo
    int flowing_ = -1;
};

}  // namespace ui
