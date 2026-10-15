#pragma once
// Símbolos del diagrama (el "?" del Tren): qué quiere decir cada dibujo del
// P&ID. Es fija: no lee el estado, así que no tiene update().

#include "components/pipe.h"
#include "lvgl.h"

namespace ui {

class LegendScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla

private:
    FlowPipes pipe_;  // el tubo de "Con flujo": sus puntos siempre corren
};

}  // namespace ui
