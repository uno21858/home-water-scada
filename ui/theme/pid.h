#pragma once
// Símbolos del P&ID del tren, generados con pid/generate.sh.
// ARGB8888 con los colores del DS ya puestos: no se les cambia el color.

#include "lvgl.h"

LV_IMAGE_DECLARE(pid_medidor);     // 44 x 32  medidor del SIAPA
LV_IMAGE_DECLARE(pid_malla);       // 24 x 32  malla del prefiltro PRF
LV_IMAGE_DECLARE(pid_purga);       // 20 x 24  purga del PRF
LV_IMAGE_DECLARE(pid_valvula_v1);  // 28 x 34  válvula de entrada con maneral
LV_IMAGE_DECLARE(pid_valvula_v);   // 20 x 24  válvula en tubo vertical (V2, V5)
LV_IMAGE_DECLARE(pid_valvula_h);   // 26 x 20  válvula en tubo horizontal (V3, V4)
LV_IMAGE_DECLARE(pid_embudo);      // 28 x 24  boca B
LV_IMAGE_DECLARE(pid_espiga);      // 18 x 16  espiga para manguera de V4
LV_IMAGE_DECLARE(pid_turbina);     // 28 x 16  turbina del medidor de flujo F
LV_IMAGE_DECLARE(pid_bomba_sumergible);  // 32 x 32  bomba de la cisterna en Panorama

// Leyenda: cada símbolo con su tramo de tubo, 72 x 60
LV_IMAGE_DECLARE(pid_leyenda_valvula);
LV_IMAGE_DECLARE(pid_leyenda_prefiltro);
LV_IMAGE_DECLARE(pid_leyenda_vasos);
LV_IMAGE_DECLARE(pid_leyenda_toma);
LV_IMAGE_DECLARE(pid_leyenda_medidor);
LV_IMAGE_DECLARE(pid_leyenda_boca);
LV_IMAGE_DECLARE(pid_leyenda_cisterna);
LV_IMAGE_DECLARE(pid_leyenda_bomba);
LV_IMAGE_DECLARE(pid_leyenda_conector);
