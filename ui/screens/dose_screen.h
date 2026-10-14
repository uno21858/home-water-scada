#pragma once
// Pantalla Dosis: los mL de Cloralex que tocan y de dónde salen,
// Ya le puse, Registrar otra cantidad, las últimas 5 dosis y el panel
// de las bombas de la fase 3 (se ve pero no se toca).

#include <array>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class DoseScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    struct Tile {
        lv_obj_t* obj = nullptr;
        lv_obj_t* day = nullptr;
        lv_obj_t* time = nullptr;
        lv_obj_t* ml = nullptr;
        lv_obj_t* typed = nullptr;
    };

    void create_due_card(lv_obj_t* page);
    void create_history(lv_obj_t* page);
    void create_phase3_panel(lv_obj_t* page);
    void layout_due(bool three_digits);

    lv_obj_t* due_value_ = nullptr;
    lv_obj_t* due_unit_ = nullptr;
    lv_obj_t* due_liters_ = nullptr;
    lv_obj_t* due_ratio_ = nullptr;
    lv_obj_t* done_button_ = nullptr;
    lv_obj_t* cap_ = nullptr;
    std::array<Tile, 5> tiles_{};
    int three_digits_ = -1;
};

}  // namespace ui
