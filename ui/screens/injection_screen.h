#pragma once
// Pantalla Inyección A y B (detalle de Dosis): las dos bocas por donde
// entra el cloro. A todavía no se usa; B es la boca a la cisterna donde
// hoy se echa el Cloralex a mano. Las bombas son de la fase 3: se ven sus
// controles pero no se tocan.

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class InjectionScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    lv_obj_t* last_ = nullptr;
    lv_obj_t* due_ = nullptr;
};

}  // namespace ui
