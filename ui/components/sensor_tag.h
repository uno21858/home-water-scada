#pragma once
// Etiqueta de valor del P&ID (P1, P2, F, ORP): 56 px de alto, panel-alto,
// borde de 2 px y radio 6. Arriba "P1 42 psi" y abajo una barra de 4 px
// que dibuja cada pantalla a su modo (rango, llenado o zonas).
// Lo de una fase que no está instalada no se muestra: set_installed(false)
// la esconde.

#include <cstdint>

#include "lvgl.h"

namespace ui {

struct SensorTag {
    lv_obj_t* live = nullptr;    // la etiqueta con valor
    lv_obj_t* value = nullptr;   // el label de la cifra
    int32_t bar_x = 12;          // dónde empieza la barra (dentro del borde)
    int32_t bar_y = 45;
    int32_t bar_w = 0;           // ancho útil de la barra

    void set_installed(bool installed) const;
};

SensorTag sensor_tag(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const char* name, const char* unit);

// Pedazo de barra de 4 px dentro de la etiqueta
lv_obj_t* tag_bar(const SensorTag& tag, int32_t x, int32_t w, uint32_t hex);

// Marcador blanco de 3 x 10 que cruza la barra
lv_obj_t* tag_marker(const SensorTag& tag);

}  // namespace ui
