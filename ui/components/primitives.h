#pragma once
// Piezas básicas con las que se arma todo lo demás.
// Todas crean objetos sin estilos del tema; el look sale de theme/.
//
// Ojo con LVGL 9.6: lv_obj_add_flag() y lv_obj_remove_flag() ya están
// deprecadas (se quitan en la 10). Se usan los setters de cada bandera:
// lv_obj_set_hidden(), lv_obj_set_clickable(), lv_obj_set_scrollable()...

#include <cstdint>

#include "lvgl.h"
#include "ui.h"

namespace ui {

inline lv_color_t color(uint32_t hex) { return lv_color_hex(hex); }

// Contenedor invisible: sin estilo, sin clic y sin scroll
lv_obj_t* box(lv_obj_t* parent);
lv_obj_t* box(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h);

// Fila o columna flex que mide lo que su contenido.
// cross = cómo se alinean los hijos en el otro eje. Con LV_FLEX_ALIGN_END
// en una fila, cifra y unidad quedan pegadas abajo (ver la nota de línea base
// en el DS: con cifra-lg o titulo la unidad lleva 2 px de pad_bottom).
lv_obj_t* row(lv_obj_t* parent, int32_t gap, lv_flex_align_t cross);
lv_obj_t* column(lv_obj_t* parent, int32_t gap, lv_flex_align_t cross);

lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, uint32_t hex, const char* text);

// Ícono A8: el color se lo pone image_recolor
lv_obj_t* icon(lv_obj_t* parent, const lv_image_dsc_t* src, uint32_t hex);
void set_icon_color(lv_obj_t* icon, uint32_t hex);

// Tarjeta del DS: panel con borde de 1 px y radio 10
lv_obj_t* card(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h);

// Tarjeta de algo que todavía no existe (fase 1.5, 2 o 3): candado y Próximamente
lv_obj_t* locked_card(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h,
                      const char* title, const char* line1, const char* line2);

// Hace tocable un objeto: se aclara al presionarlo y abre otra pantalla
void link_to(lv_obj_t* obj, Screen target);

// Flechita ">" arriba a la derecha de una tarjeta tocable
lv_obj_t* chevron(lv_obj_t* parent);

// Botones de 80 px de alto. El primario lleva palomita.
// Para deshabilitarlos: lv_obj_set_disabled(boton, true).
lv_obj_t* button_primary(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const char* text,
                         lv_event_cb_t on_click, void* user_data);
lv_obj_t* button_secondary(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const char* text,
                           lv_event_cb_t on_click, void* user_data);

// Cambia el texto solo si es distinto: cada lv_label_set_text redibuja
void set_text(lv_obj_t* label, const char* text);

}  // namespace ui
