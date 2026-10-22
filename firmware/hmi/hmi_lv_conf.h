/**
 * Configuración de LVGL para la 7B: la misma del simulador (simulator/lv_conf.h,
 * así las pantallas se ven igual) con lo que cambia en la placa.
 */
#ifndef HMI_LV_CONF_H
#define HMI_LV_CONF_H

#include "../../simulator/lv_conf.h"

/* Memoria de LVGL: 2 MB de la PSRAM. En la RAM interna no caben los objetos
 * de las 22 pantallas junto con el WiFi. */
#undef LV_MEM_SIZE
#define LV_MEM_SIZE (2 * 1024 * 1024)
#define LV_MEM_POOL_INCLUDE "esp_heap_caps.h"
#define LV_MEM_POOL_ALLOC(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM)

/* Sin SDL: la pantalla y el táctil los pone firmware/hmi/main/board.cpp */
#undef LV_USE_SDL
#define LV_USE_SDL 0

/* Los chequeos lentos se quedan en el simulador */
#undef LV_USE_ASSERT_STYLE
#define LV_USE_ASSERT_STYLE 0
#undef LV_USE_ASSERT_MEM_INTEGRITY
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#undef LV_USE_ASSERT_OBJ
#define LV_USE_ASSERT_OBJ 0
#undef LV_USE_ASSERT_NULL
#define LV_USE_ASSERT_NULL 1
#undef LV_USE_ASSERT_MALLOC
#define LV_USE_ASSERT_MALLOC 1
#undef LV_USE_SYSMON
#define LV_USE_SYSMON 0

#endif /* HMI_LV_CONF_H */
