#pragma once
// Fuentes de la escala del DS, generadas con fonts/generate.sh.
// Montserrat Tnum: todos los dígitos miden lo mismo y el número no "baila".
// Las font_digits_* solo traen cifras, coma, punto, menos y espacio de cifra:
// la unidad siempre va en otro label.
// En el comentario: token del DS y alto de línea (lo que mide el label).

#include "lvgl.h"

LV_FONT_DECLARE(font_label_26)     // etiqueta    26 px Medium    32
LV_FONT_DECLARE(font_body_32)      // texto       32 px Medium    38
LV_FONT_DECLARE(font_title_36)     // titulo      36 px SemiBold  45
LV_FONT_DECLARE(font_title_xl_48)  // titulo-xl   48 px SemiBold  58
LV_FONT_DECLARE(font_digits_44)    // cifra-md    44 px SemiBold  38
LV_FONT_DECLARE(font_digits_72)    // cifra-lg    72 px SemiBold  63
LV_FONT_DECLARE(font_digits_136)   // cifra-xl   136 px SemiBold 119
LV_FONT_DECLARE(font_digits_232)   // cifra-hero 232 px SemiBold 200
