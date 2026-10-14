#pragma once
// Estilos compartidos. Un estilo es un paquete de propiedades (fondo, borde,
// radio, texto) que se define una vez y se le pega a muchos objetos con
// lv_obj_add_style(). Si cambias el radio de las tarjetas aquí, cambian todas.

#include "lvgl.h"

namespace ui::styles {

void init();  // una vez, antes de crear la interfaz

extern lv_style_t screen;                    // fondo y texto por defecto
extern lv_style_t card;                      // panel, borde de 1 px, radio 10
extern lv_style_t card_pressed;              // tarjeta o zona tocable mientras la presionas
extern lv_style_t card_locked;               // fase futura: se ve pero no se toca
extern lv_style_t button_primary;            // accion: la acción principal de la vista
extern lv_style_t button_primary_pressed;
extern lv_style_t button_secondary;          // panel-alto con borde-control
extern lv_style_t button_secondary_pressed;

}  // namespace ui::styles
