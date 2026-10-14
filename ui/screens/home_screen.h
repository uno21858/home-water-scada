#pragma once
// Pantalla Tren (inicio): el P&ID del tren de filtros. Agua del SIAPA,
// medidor, prefiltro, V1, los 5 vasos, F, P2, la bajada con V5, B, V3, V4
// y V2, y la cisterna con la sonda ORP y la bomba R. Abajo la franja con
// caída, litros de hoy y lo que toca echar.

#include "components/pipe.h"
#include "components/sensor_tag.h"
#include "lvgl.h"
#include "system_state.h"

namespace ui {

class HomeScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    void create_pipes(lv_obj_t* page);
    void create_inlet(lv_obj_t* page);
    void create_housings(lv_obj_t* page);
    void create_return(lv_obj_t* page);
    void create_cistern(lv_obj_t* page);
    void create_tags(lv_obj_t* page);
    void create_strip(lv_obj_t* page);

    void update_pressure(const core::SystemState& s);
    void update_orp(const core::SystemState& s);
    void update_mixer(const core::SystemState& s);

    FlowPipes pipes_;

    // Etiquetas de valor
    SensorTag p1_;
    SensorTag p2_;
    SensorTag f_;
    SensorTag orp_;
    lv_obj_t* p1_marker_ = nullptr;
    lv_obj_t* p2_marker_ = nullptr;
    lv_obj_t* f_fill_ = nullptr;
    lv_obj_t* orp_zones_[3] = {};
    lv_obj_t* orp_marker_ = nullptr;
    lv_obj_t* r_live_ = nullptr;
    lv_obj_t* r_locked_ = nullptr;
    lv_obj_t* r_state_ = nullptr;

    // Cisterna
    lv_obj_t* cistern_word_ = nullptr;
    lv_obj_t* pump_ring_ = nullptr;
    lv_obj_t* pump_rotor_ = nullptr;

    // Franja de abajo
    lv_obj_t* drop_live_ = nullptr;
    lv_obj_t* drop_locked_ = nullptr;
    lv_obj_t* drop_value_ = nullptr;
    lv_obj_t* drop_word_ = nullptr;
    lv_obj_t* today_value_ = nullptr;
    lv_obj_t* dose_value_ = nullptr;

    // Para no repintar lo que no cambió (-1 = todavía no se pinta)
    int flowing_ = -1;
    int drop_level_ = -1;
    int orp_level_ = -1;
    int pump_state_ = -1;
};

}  // namespace ui
