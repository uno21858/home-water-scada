#pragma once
// Panorama de la casa (Casa, en el Tren): de dónde viene el agua y a dónde
// va. SIAPA, el tren, la cisterna con su bomba, los hidros y la casa. Solo
// los tubos que mide F llevan puntos; el Tren y la Cisterna se tocan y abren.

#include <cstdint>

#include "components/pipe.h"
#include "lvgl.h"
#include "system_state.h"

namespace ui {

class OverviewScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    void create_train(lv_obj_t* page);
    void create_cistern(lv_obj_t* page);
    void create_house(lv_obj_t* page);
    void create_key(lv_obj_t* page);  // la tarjeta que explica los tubos

    FlowPipes pipes_;      // entrada al tren y bajada a la cisterna
    FlowPipes key_pipe_;   // la muestra de "Lo mide F": corre siempre

    // Tren
    lv_obj_t* flow_ = nullptr;
    lv_obj_t* drop_row_ = nullptr;  // "Caída 4 psi" con su ícono; sin P1 y P2 se esconde
    lv_obj_t* drop_ = nullptr;
    lv_obj_t* drop_icon_ = nullptr;  // ok, advertencia o alarma

    // Cisterna
    lv_obj_t* orp_row_ = nullptr;   // ORP 650 mV Oxidante; sin sonda se esconde
    lv_obj_t* orp_value_ = nullptr;
    lv_obj_t* orp_word_ = nullptr;
    lv_obj_t* cistern_ = nullptr;
    lv_obj_t* mast_ = nullptr;
    lv_obj_t* mixer_ = nullptr;     // R, aro y aspas; sin bomba R se esconde
    lv_obj_t* rotor_ = nullptr;

    // Casa
    lv_obj_t* liters_ = nullptr;
    lv_obj_t* ro_ = nullptr;        // RO de la cocina por instalar
    lv_obj_t* key_ro_ = nullptr;
    lv_obj_t* key_ = nullptr;

    // -1 = todavía no se pinta
    int flowing_ = -1;
    int drop_level_ = -1;
    int orp_level_ = -1;
    int spinning_ = -1;
    int ro_shown_ = -1;
};

}  // namespace ui
