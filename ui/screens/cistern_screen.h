#pragma once
// Pantalla Cisterna (fase 2): el ORP en grande con su estado, la escala de
// tres zonas, los últimos 7 días, el cloro libre estimado y la última
// calibración con tira. Sin sonda ORP se ve bloqueada.

#include <array>
#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class CisternScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    void create_reading(lv_obj_t* page);
    void create_scale(lv_obj_t* page);
    void create_chart(lv_obj_t* page);
    void create_chlorine(lv_obj_t* page);
    void create_calibration(lv_obj_t* page);

    void update_reading(const core::SystemState& s);
    void update_chart(const core::SystemState& s);
    void update_day_marks(const core::SystemState& s);
    void update_strip(const core::SystemState& s);

    lv_obj_t* live_ = nullptr;    // todo lo de la pantalla con sonda
    lv_obj_t* locked_ = nullptr;  // tarjeta de Próximamente sin sonda

    // ORP de ahorita
    lv_obj_t* orp_value_ = nullptr;
    lv_obj_t* orp_unit_ = nullptr;
    lv_obj_t* state_ = nullptr;
    lv_obj_t* state_icon_ = nullptr;
    lv_obj_t* state_word_ = nullptr;

    // Escala de tres zonas: reductor, neutro y oxidante
    std::array<lv_obj_t*, 3> zones_{};
    std::array<lv_obj_t*, 3> zone_names_{};
    lv_obj_t* marker_ = nullptr;
    int zone_ = -1;  // -1 = todavía no se pinta

    // Gráfica de 7 días
    lv_obj_t* chart_ = nullptr;
    lv_chart_series_t* series_ = nullptr;
    lv_obj_t* chart_dot_ = nullptr;
    std::array<lv_obj_t*, 7> day_labels_{};
    std::array<int32_t, core::kOrpSamples7d> chart_values_{};
    bool chart_drawn_ = false;
    int marks_minute_ = -1;

    // Cloro libre y calibración
    lv_obj_t* chlorine_row_ = nullptr;
    lv_obj_t* chlorine_value_ = nullptr;
    lv_obj_t* chlorine_note_ = nullptr;
    lv_obj_t* strip_date_ = nullptr;
    lv_obj_t* strip_detail_ = nullptr;
};

}  // namespace ui
