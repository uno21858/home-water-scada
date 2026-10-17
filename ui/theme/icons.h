#pragma once
// Íconos A8 (solo alfa) generados con icons/generate.sh.
// El color se pone en código con image_recolor (ver icon() en primitives.h).

#include "lvgl.h"

// Barra de navegación
LV_IMAGE_DECLARE(icon_tren_32);
LV_IMAGE_DECLARE(icon_resumen_32);
LV_IMAGE_DECLARE(icon_flujo_32);
LV_IMAGE_DECLARE(icon_cisterna_32);
LV_IMAGE_DECLARE(icon_dosis_32);
LV_IMAGE_DECLARE(icon_historial_32);
LV_IMAGE_DECLARE(icon_ajustes_32);

// Estados
LV_IMAGE_DECLARE(icon_ok_32);
LV_IMAGE_DECLARE(icon_advertencia_32);
LV_IMAGE_DECLARE(icon_alarma_32);
LV_IMAGE_DECLARE(icon_ok_28);
LV_IMAGE_DECLARE(icon_advertencia_28);
LV_IMAGE_DECLARE(icon_alarma_28);
LV_IMAGE_DECLARE(icon_ok_40);           // pastilla de Pasillo
LV_IMAGE_DECLARE(icon_advertencia_40);
LV_IMAGE_DECLARE(icon_alarma_40);
LV_IMAGE_DECLARE(icon_cartucho_40);     // pastilla de Pasillo en cambio de cartuchos
LV_IMAGE_DECLARE(icon_alarma_96);         // alarma a pantalla completa
LV_IMAGE_DECLARE(icon_ok_56);             // Sin alarmas activas
LV_IMAGE_DECLARE(icon_info_28);         // aviso neutro: Sin flujo
LV_IMAGE_DECLARE(icon_info_32);         // "?" del encabezado: Leyenda
LV_IMAGE_DECLARE(icon_wifi_32);
LV_IMAGE_DECLARE(icon_wifi_off_32);

// Controles
LV_IMAGE_DECLARE(icon_chevron_28);
LV_IMAGE_DECLARE(icon_chevron_32);
LV_IMAGE_DECLARE(icon_atras_32);        // botón atrás del encabezado
LV_IMAGE_DECLARE(icon_check_32);
LV_IMAGE_DECLARE(icon_candado_32);
LV_IMAGE_DECLARE(icon_candado_28);
LV_IMAGE_DECLARE(icon_candado_24);
LV_IMAGE_DECLARE(icon_editar_24);       // dosis capturada a mano
LV_IMAGE_DECLARE(icon_borrar_48);       // tecla borrar
LV_IMAGE_DECLARE(icon_calendario_32);
LV_IMAGE_DECLARE(icon_reloj_32);
LV_IMAGE_DECLARE(icon_bomba_28);        // pulso de prueba (fase 3)
LV_IMAGE_DECLARE(icon_gota_28);         // RO de la cocina en Panorama
LV_IMAGE_DECLARE(icon_gota_32);         // Calibrar con tira
LV_IMAGE_DECLARE(icon_cartucho_32);     // cambio de cartucho en los eventos
LV_IMAGE_DECLARE(icon_editar_32);       // escribir un valor con el teclado
LV_IMAGE_DECLARE(icon_cubeta_32);       // Prueba de cubeta

// Categorías de Ajustes (dosis, conexión y cartuchos usan gota, wifi y cartucho)
LV_IMAGE_DECLARE(icon_medidor_32);      // sensores
LV_IMAGE_DECLARE(icon_sol_32);          // pantalla

// Equipos
LV_IMAGE_DECLARE(icon_impulsor_56);        // aspas de la bomba R; gira con lv_image_set_rotation
LV_IMAGE_DECLARE(icon_impulsor_chico_26);  // las mismas aspas, en el diagrama del tren
