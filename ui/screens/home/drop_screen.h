#pragma once
// Detalle de la caída del tren (P1 - P2): la cifra con su estado, la
// escala de 0 a 15 psi, la caída contra el caudal del último mes junto a la
// de cartuchos nuevos, la comparación al caudal de ahorita y qué hacer si
// llega a Revisar.

#include <array>
#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class DropScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    struct Dot {
        int32_t x;
        int32_t y;
        bool recent;  // de los últimos 3 días
    };

    void create_reading(lv_obj_t* page);
    void create_scale(lv_obj_t* page);
    void create_chart(lv_obj_t* page);
    void create_side(lv_obj_t* page);
    void show_no_signal(const core::SystemState& s);
    float update_baseline(const core::SystemState& s);
    void update_chart(const core::SystemState& s);
    static void draw_plot(lv_event_t* e);

    lv_obj_t* live_ = nullptr;
    lv_obj_t* locked_ = nullptr;

    lv_obj_t* value_ = nullptr;
    lv_obj_t* unit_ = nullptr;
    lv_obj_t* state_ = nullptr;
    lv_obj_t* state_icon_ = nullptr;
    lv_obj_t* state_word_ = nullptr;
    lv_obj_t* formula_ = nullptr;
    lv_obj_t* flow_note_ = nullptr;

    std::array<lv_obj_t*, 3> zones_{};
    std::array<lv_obj_t*, 3> zone_names_{};
    lv_obj_t* marker_ = nullptr;
    int level_ = -1;  // -1 = todavía no se pinta; kNoSignal sin P1 o P2

    // Gráfica: lo que dibuja draw_plot, en coordenadas de la tarjeta
    lv_obj_t* plot_ = nullptr;
    std::array<lv_point_precise_t, 16> curve_{};  // caída con cartuchos nuevos
    std::array<Dot, core::kDropSamples> dots_{};
    int dot_count_ = 0;
    int32_t today_x_ = 0;
    int32_t today_y_ = 0;
    bool today_shown_ = false;

    lv_obj_t* same_flow_title_ = nullptr;
    lv_obj_t* new_value_ = nullptr;
    lv_obj_t* now_value_ = nullptr;
    lv_obj_t* now_unit_ = nullptr;
    lv_obj_t* rise_ = nullptr;
};

}  // namespace ui
