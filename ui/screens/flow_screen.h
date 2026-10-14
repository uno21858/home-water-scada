#pragma once
// Pantalla Flujo: el caudal de ahorita en grande (se lee a 2 m), los litros
// de hoy y desde la última dosis, y la gráfica de las últimas 2 h.
// Es de lectura: no tiene botones.

#include <array>
#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class FlowScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    void create_reading(lv_obj_t* page);
    void create_totals(lv_obj_t* page);
    void create_chart(lv_obj_t* page);

    void update_reading(const core::SystemState& s);
    void update_chart(const core::SystemState& s);
    void update_time_marks(const core::SystemState& s);

    // Caudal de ahorita
    lv_obj_t* flow_value_ = nullptr;
    lv_obj_t* no_flow_chip_ = nullptr;
    int flowing_ = -1;  // -1 = todavía no se pinta

    // Litros
    lv_obj_t* liters_today_ = nullptr;
    lv_obj_t* liters_since_dose_ = nullptr;

    // Gráfica de 2 h
    lv_obj_t* chart_ = nullptr;
    lv_chart_series_t* series_ = nullptr;
    lv_obj_t* chart_dot_ = nullptr;
    std::array<lv_obj_t*, 5> time_labels_{};  // hace 2 h, 1:30, 1 h, 30 min y ahorita
    std::array<int32_t, core::kFlowSamples2h> chart_values_{};
    bool chart_drawn_ = false;
    int marks_minute_ = -1;
};

}  // namespace ui
