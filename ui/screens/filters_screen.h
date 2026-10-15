#pragma once
// Pantalla Cartuchos: un renglón por filtro (prefiltro y los 5 vasos) con
// los días y litros desde que se purgó o se cambió, una barra de cuánto le
// queda, cada cuánto se cambia y el botón para registrarlo.

#include <array>
#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class FiltersScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    struct Row {
        lv_obj_t* obj = nullptr;
        lv_obj_t* icon = nullptr;
        lv_obj_t* name = nullptr;
        lv_obj_t* days = nullptr;
        lv_obj_t* liters = nullptr;
        lv_obj_t* track = nullptr;
        lv_obj_t* bar = nullptr;
        lv_obj_t* every = nullptr;
        lv_obj_t* button_text = nullptr;
        int installed = -1;  // -1 = todavía no se pinta
    };

    static void handle_service(lv_event_t* e);

    std::array<Row, core::kFilterCount> rows_{};
    std::array<bool, core::kFilterCount> installed_{};  // para la pregunta del diálogo
    uint16_t now_ = 0;                                  // minuto del día, para el diálogo
};

}  // namespace ui
