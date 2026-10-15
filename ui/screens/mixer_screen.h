#pragma once
// Pantalla Mezcla (detalle de Cisterna, fase 2): la bomba R que revuelve la
// cisterna. Estado con lo que falta, selector Apagada · Prendida · Auto, el
// programa del modo Auto, cuánto lleva hoy y las últimas veces.

#include <array>
#include <cstdint>

#include "components/mixer.h"
#include "components/segmented.h"
#include "lvgl.h"
#include "system_state.h"

namespace ui {

class MixerScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    // Un renglón de "Últimas veces"
    struct Run {
        lv_obj_t* obj = nullptr;
        lv_obj_t* time = nullptr;
        lv_obj_t* reason = nullptr;
        lv_obj_t* minutes = nullptr;
        lv_obj_t* now = nullptr;  // "ahora" si sigue prendida
    };

    void create_status(lv_obj_t* page);
    void create_program(lv_obj_t* page);
    void create_today(lv_obj_t* page);
    void create_runs(lv_obj_t* page);

    static void handle_after_dose(lv_event_t* e);
    static void handle_every_4h(lv_event_t* e);

    lv_obj_t* live_ = nullptr;    // todo lo de la pantalla con bomba
    lv_obj_t* locked_ = nullptr;  // tarjeta de Próximamente sin bomba

    // Estado
    Impeller impeller_;
    lv_obj_t* state_ = nullptr;
    lv_obj_t* side_ = nullptr;
    lv_obj_t* side_text_ = nullptr;
    lv_obj_t* side_minutes_ = nullptr;
    lv_obj_t* bar_ = nullptr;
    lv_obj_t* sub_ = nullptr;
    Segmented mode_;

    // Programa del modo Auto
    lv_obj_t* after_dose_ = nullptr;
    lv_obj_t* every_4h_ = nullptr;
    lv_obj_t* manual_note_ = nullptr;
    uint8_t after_dose_min_ = 0;  // para la pista del teclado
    uint8_t every_4h_min_ = 0;

    // Hoy y últimas veces
    lv_obj_t* today_minutes_ = nullptr;
    lv_obj_t* today_runs_ = nullptr;
    std::array<Run, 4> runs_{};
};

}  // namespace ui
