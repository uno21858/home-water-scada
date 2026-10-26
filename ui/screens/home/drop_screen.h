#pragma once
// Detalle de los cartuchos: R = (P1 - P2) / F contra la referencia de
// cartuchos nuevos (core/clog.h). El % en grande con su estado, la escala
// de 50 a 250 % (Revisar desde 150, Tapados arriba de 200), R por día del
// último mes con la raya de nuevos, el último R y la referencia, y qué hacer
// si llega a Revisar. Sin flujo enseña el último estado con su hora.

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
    void create_reading(lv_obj_t* page);
    void create_scale(lv_obj_t* page);
    void create_chart(lv_obj_t* page);
    void create_side(lv_obj_t* page);
    void update_reading(const core::SystemState& s);
    void update_side(const core::SystemState& s);
    void update_chart(const core::SystemState& s);
    static void draw_plot(lv_event_t* e);

    lv_obj_t* live_ = nullptr;
    lv_obj_t* locked_ = nullptr;

    lv_obj_t* value_ = nullptr;  // R/R0 en %
    lv_obj_t* unit_ = nullptr;
    lv_obj_t* state_ = nullptr;
    lv_obj_t* state_icon_ = nullptr;
    lv_obj_t* state_word_ = nullptr;
    lv_obj_t* note_ = nullptr;   // sin flujo, sin referencia o sin señal: qué pasa

    std::array<lv_obj_t*, 3> zones_{};
    std::array<lv_obj_t*, 3> zone_names_{};
    lv_obj_t* marker_ = nullptr;
    int kind_ = -1;  // ClogLook::Kind pintado; -1 = todavía no se pinta

    // Gráfica: lo que dibuja draw_plot, en coordenadas de la tarjeta
    lv_obj_t* plot_ = nullptr;
    lv_obj_t* top_label_ = nullptr;  // el R de arriba del eje
    std::array<lv_point_precise_t, core::kHistoryDays> points_{};
    std::array<bool, core::kHistoryDays> has_point_{};  // días sin agua no llevan punto
    std::array<int32_t, 3> lines_y_{};  // nuevos, 150 % y 200 %; < 0 sin referencia
    float plot_max_ = 0.0f;

    lv_obj_t* last_value_ = nullptr;   // el último R
    lv_obj_t* last_note_ = nullptr;    // "22 psi a 5.6 L/min · 18:42"
    lv_obj_t* ref_value_ = nullptr;    // R0
    lv_obj_t* ref_note_ = nullptr;     // "5.6 L/min · 24 oct", o dónde guardarla
};

}  // namespace ui
