#pragma once
// Pantalla Tren (inicio): el P&ID del tren de filtros. Agua del SIAPA,
// medidor, prefiltro, V1, los 5 vasos, F, P2, la bajada con V2, B, V3, V4
// y V5, y la cisterna con la sonda ORP y la bomba R. Abajo la franja con
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
    void update_clog(const core::SystemState& s);     // el recuadro de los cartuchos (clog_look)
    void update_orp(const core::SystemState& s);
    void update_level(const core::SystemState& s);  // el agua de la cisterna y su %
    void update_mixer(const core::SystemState& s);
    void layout_strip(bool with_drop);  // el renglón de abajo, con o sin caída

    FlowPipes pipes_;
    std::array<lv_obj_t*, 4> pressure_parts_{};  // tomas de P1 y P2 con su raya
    bool has_pumps_ = false;                     // B abre Inyección o Dosis

    // Vasos del 20 µm al sarro: nombre arriba y el cartucho adentro
    struct Housing {
        lv_obj_t* name = nullptr;
        lv_obj_t* cartridge = nullptr;
        int installed = -1;  // -1 = todavía no se pinta
    };
    std::array<Housing, 5> housings_{};

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
    lv_obj_t* r_state_ = nullptr;

    // Cisterna
    lv_obj_t* cistern_name_ = nullptr;  // "Cisterna", o "Cisterna · 78 %" con la sonda
    lv_obj_t* cistern_word_ = nullptr;  // estado del ORP, o el nivel sin sonda
    lv_obj_t* cistern_liters_ = nullptr;  // "≈ 2,520 L", sin sonda (con ella ahí va su etiqueta)
    lv_obj_t* water_ = nullptr;         // agua de la cisterna, con sensor de nivel
    lv_obj_t* water_line_ = nullptr;
    lv_obj_t* cistern_ = nullptr;
    lv_obj_t* mast_ = nullptr;
    lv_obj_t* probe_ = nullptr;
    lv_obj_t* pump_ring_ = nullptr;
    lv_obj_t* pump_rotor_ = nullptr;

    // Franja de abajo
    lv_obj_t* drop_live_ = nullptr;
    lv_obj_t* today_ = nullptr;
    lv_obj_t* due_ = nullptr;
    int with_drop_ = -1;  // -1 = todavía no se acomoda
    lv_obj_t* drop_name_ = nullptr;    // "R"
    lv_obj_t* drop_value_ = nullptr;   // R con un decimal
    lv_obj_t* drop_detail_ = nullptr;  // "· 101 %", "Sin flujo" o "Filtros"
    lv_obj_t* drop_word_ = nullptr;    // el estado, con su hora sin flujo
    lv_obj_t* today_value_ = nullptr;
    lv_obj_t* dose_value_ = nullptr;

    // Para no repintar lo que no cambió (-1 = todavía no se pinta)
    int flowing_ = -1;
    lv_obj_t* no_supply_ = nullptr;  // "Sin agua" arriba de SIAPA: sin flujo ni presión
    int drop_level_ = -1;  // ClogLook::Kind pintado
    int orp_level_ = -1;
    int pump_state_ = -1;
};

}  // namespace ui
