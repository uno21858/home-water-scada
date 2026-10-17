#pragma once
// Pantalla Resumen: caudal con las últimas 24 h, lo que toca echar,
// filtros (P1, P2 y caída) y la bomba R. Cada tarjeta abre su pantalla.

#include <array>
#include <cstdint>

#include "components/mixer.h"
#include "lvgl.h"
#include "system_state.h"

namespace ui {

class SummaryScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    void create_flow_card(lv_obj_t* page);
    void create_dose_card(lv_obj_t* page);
    void create_filters_card(lv_obj_t* page);
    void create_mixer_card(lv_obj_t* page);
    void create_cartridges_card(lv_obj_t* page);

    void update_flow(const core::SystemState& s);
    void update_time_marks(const core::SystemState& s);
    void update_filters(const core::SystemState& s);
    void update_mixer(const core::SystemState& s);
    void update_cartridges(const core::SystemState& s);

    // Caudal
    lv_obj_t* flow_title_ = nullptr;
    lv_obj_t* flow_value_ = nullptr;
    lv_obj_t* liters_today_ = nullptr;
    lv_obj_t* chart_ = nullptr;
    lv_chart_series_t* series_ = nullptr;
    lv_obj_t* chart_dot_ = nullptr;
    lv_obj_t* now_label_ = nullptr;
    std::array<lv_obj_t*, 4> mark_lines_{};
    std::array<lv_obj_t*, 4> mark_labels_{};
    std::array<int32_t, core::kFlowSamples24h> chart_values_{};
    bool chart_drawn_ = false;
    int flow_state_ = -1;  // 0 con flujo, 1 sin flujo, 2 sin agua
    int marks_minute_ = -1;

    // Toca echar
    lv_obj_t* dose_value_ = nullptr;
    lv_obj_t* dose_button_ = nullptr;

    // Filtros
    lv_obj_t* filters_live_ = nullptr;
    lv_obj_t* p1_value_ = nullptr;
    lv_obj_t* p2_value_ = nullptr;
    lv_obj_t* p1_marker_ = nullptr;
    lv_obj_t* p2_marker_ = nullptr;
    lv_obj_t* drop_tile_ = nullptr;
    lv_obj_t* drop_value_ = nullptr;
    lv_obj_t* drop_icon_ = nullptr;
    lv_obj_t* drop_word_ = nullptr;
    int drop_level_ = -1;

    // Bomba R
    lv_obj_t* mixer_live_ = nullptr;
    Impeller impeller_;
    lv_obj_t* mixer_state_ = nullptr;
    lv_obj_t* mixer_left_ = nullptr;
    lv_obj_t* mixer_minutes_ = nullptr;
    lv_obj_t* mixer_bar_ = nullptr;
    lv_obj_t* mixer_sub_ = nullptr;

    // Cartuchos (sin Filtros o sin Bomba R)
    lv_obj_t* cartridges_ = nullptr;
    lv_obj_t* next_name_ = nullptr;
    lv_obj_t* next_days_ = nullptr;
    lv_obj_t* next_unit_ = nullptr;
    lv_obj_t* next_track_ = nullptr;
    lv_obj_t* next_bar_ = nullptr;
    lv_obj_t* next_sub_ = nullptr;
    int cartridges_slot_ = -1;  // 0 no sale, 1 izquierda, 2 derecha, 3 todo el ancho
};

}  // namespace ui
