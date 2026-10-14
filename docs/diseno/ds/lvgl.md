# Implementación en LVGL 9

Todo lo de esta sección se revisó contra el código de LVGL v9.6.0. Entre versiones menores de la 9 cambian algunos nombres: si usas otra, confirma en sus encabezados.

## Fuentes

`fonts/MontserratTnum-Medium.ttf` y `fonts/MontserratTnum-SemiBold.ttf` ya traen los dígitos tabulares congelados. Para regenerarlas desde Montserrat original:

```bash
pip install opentype-feature-freezer
pyftfeatfreeze -f tnum -S -U Tnum Montserrat-Medium.ttf MontserratTnum-Medium.ttf
pyftfeatfreeze -f tnum -S -U Tnum Montserrat-SemiBold.ttf MontserratTnum-SemiBold.ttf
```

Las 8 fuentes de la escala, con lv_font_conv (probado con la 1.5.3):

```bash
TXT="0x20-0x7E,0xA0-0xFF,0x2007,0x2009,0x2026,0x2192,0x2212,0x2248,0x2264,0x2265"
DIG="0x20,0x2C-0x2E,0x30-0x39,0x2007,0x2212"
f() { npx lv_font_conv --bpp 4 --size "$1" --font "MontserratTnum-$2.ttf" -r "$3" --format lvgl --lv-font-name "$4" -o "$4.c"; }
f 232 SemiBold "$DIG" font_cifra_hero_232
f 136 SemiBold "$DIG" font_cifra_xl_136
f 72  SemiBold "$DIG" font_cifra_lg_72
f 44  SemiBold "$DIG" font_cifra_md_44
f 48  SemiBold "$TXT" font_titulo_xl_48
f 36  SemiBold "$TXT" font_titulo_36
f 32  Medium   "$TXT" font_texto_32
f 26  Medium   "$TXT" font_etiqueta_26
```

- Medido con `--format bin`: las 8 juntas pesan unos 150 KB con la compresión RLE por defecto y unos 380 KB con `--no-compress`. Con 16 MB de flash no pesa. Si el caudal de 232 px se redibuja lento, genera solo las tres cifras grandes con `--no-compress`.
- Declara cada una con `LV_FONT_DECLARE(font_texto_32);` y asígnala con `lv_obj_set_style_text_font(obj, &font_texto_32, 0);`.
- El alto de un label es el `line_height` de su fuente generada: 200, 119, 63, 38, 58, 45, 38 y 32 px en el orden de la lista. Es el lineHeight de los tokens; los frames se maquetaron con esas cajas.
- Las fuentes de cifras no traen letras: la unidad siempre es otro label.
- Alinear cifra y unidad: en flex, alinea abajo (`LV_FLEX_ALIGN_END`). `cifra-md`, `texto` y `etiqueta` tienen el mismo `base_line` (7 px) y quedan en la misma línea base; con `cifra-lg` (11) y `titulo` (9) agrega 2 px de `pad_bottom` al label de la unidad.
- Espacio de cifra para que el ancho no salte: en C es la secuencia UTF-8 `"\xE2\x80\x87"`.

## Colores y estilos

Cada token de color es `lv_color_hex(0x1b1f24)` y así. Quita los estilos del tema a todo control propio con `lv_obj_remove_style_all(obj)` y arma un estilo por papel y estado. Ejemplo del botón primario:

```c
static lv_style_t st_prim, st_prim_pr, st_off;

void estilos_init(void) {
    lv_style_init(&st_prim);
    lv_style_set_bg_color(&st_prim, lv_color_hex(0xe3e8ed));   /* accion */
    lv_style_set_bg_opa(&st_prim, LV_OPA_COVER);
    lv_style_set_radius(&st_prim, 10);                          /* radio-m */
    lv_style_set_text_color(&st_prim, lv_color_hex(0x14181c));  /* en-accion */
    lv_style_set_text_font(&st_prim, &font_titulo_36);

    lv_style_init(&st_prim_pr);
    lv_style_set_bg_color(&st_prim_pr, lv_color_hex(0xb9c3cd)); /* accion-presionado */

    lv_style_init(&st_off);
    lv_style_set_bg_color(&st_off, lv_color_hex(0x232930));     /* panel */
    lv_style_set_border_color(&st_off, lv_color_hex(0x3a434d)); /* borde */
    lv_style_set_border_width(&st_off, 2);
    lv_style_set_text_color(&st_off, lv_color_hex(0x7c8793));   /* texto-3 */
}

lv_obj_t * boton_primario(lv_obj_t * padre, const char * txt) {
    lv_obj_t * b = lv_button_create(padre);
    lv_obj_remove_style_all(b);
    lv_obj_add_style(b, &st_prim, 0);
    lv_obj_add_style(b, &st_prim_pr, LV_STATE_PRESSED);
    lv_obj_add_style(b, &st_off, LV_STATE_DISABLED);
    lv_obj_set_height(b, 80);                                   /* alto-boton */
    lv_obj_t * l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
    return b;
}
```

## Estructura de objetos

- Una sola screen base con tres hijos fijos: encabezado (1024 x 64 en y 0), contenido (1024 x 456 en y 64) y navegación (1024 x 80 en y 520).
- Las 6 pantallas son 6 contenedores creados al arrancar dentro de contenido; la navegación muestra uno y oculta los demás con `LV_OBJ_FLAG_HIDDEN`. Encabezado y navegación nunca se recrean.
- Alarma, diálogo y teclado van en `lv_layer_top()`, encima de todo. El velo es un `lv_obj` de 1024 x 600 con `bg_color` de `sobrecapa` y `bg_opa` 204.
- Actualiza los labels solo cuando cambia el valor (compara antes de `lv_label_set_text`) para no invalidar áreas de más.

## Piezas

| Pieza | En LVGL |
|---|---|
| Encabezado | `lv_obj` + label de título y hora (`font_titulo_36`) + pastilla: `lv_obj` con radio `LV_RADIUS_CIRCLE`, borde 2 px y flex en fila, con ícono A8 y label |
| Barra inferior | 6 `lv_button` con `LV_OBJ_FLAG_CHECKABLE`. Activa = `LV_STATE_CHECKED`: fondo `panel-alto` y borde superior de 4 px en `texto` (`border_side` = `LV_BORDER_SIDE_TOP`) |
| Botones | `lv_button` sin estilos del tema + un estilo por estado: normal, `LV_STATE_PRESSED`, `LV_STATE_DISABLED` |
| Selector segmentado | `lv_buttonmatrix` con `LV_BUTTONMATRIX_CTRL_CHECKABLE` en cada botón y `lv_buttonmatrix_set_one_checked(obj, true)` |
| Etiqueta de valor | `lv_obj` con flex en fila y 2 o 3 labels (id, cifra, unidad); un estilo por umbral (normal, ok, advertencia, alarma) |
| Tubería | Rectángulos `lv_obj` de 10 px de grueso; en cada codo los tramos se enciman 5 px para que la esquina quede cuadrada |
| Puntos de flujo | Ver la siguiente sección |
| Cifra grande | `lv_label` con `font_cifra_hero_232`, ancho fijo y texto alineado a la derecha |
| Tendencias | `lv_chart` con `LV_CHART_TYPE_LINE` (caudal: 121 puntos, `LV_CHART_UPDATE_MODE_SHIFT`, `lv_chart_set_next_value` cada minuto) o `LV_CHART_TYPE_BAR` (litros y dosis, la dosis en `LV_CHART_AXIS_SECONDARY_Y`); ejes con `lv_scale`; umbrales con `lv_line` punteada |
| Escala ORP | 3 `lv_obj` (zonas) + 1 `lv_obj` de 6 x 48 (marcador) + labels. Alternativa: `lv_scale` con `lv_scale_add_section` y `lv_scale_section_set_range` |
| Teclas | 12 `lv_button` en rejilla de 3 x 4; la tecla borrar lleva el ícono A8 de 48 px |
| Modal | `lv_obj` propio en `lv_layer_top()` o `lv_msgbox_create(NULL)` con `lv_msgbox_add_title`, `lv_msgbox_add_text` y `lv_msgbox_add_footer_button`, quitando los estilos del tema |
| Alarma | `lv_obj` de 1024 x 600 en `lv_layer_top()` con fondo `rojo-alarma` |
| Iconos | `lv_image` con fuente A8, color con `lv_obj_set_style_image_recolor` y `lv_obj_set_style_image_recolor_opa(obj, LV_OPA_COVER, 0)` |

## Animación de flujo

Cómo sí: un objeto por tramo recto de tubería, sin estilos, que dibuja sus propios puntos; un solo `lv_timer` mueve la fase e invalida cada tramo. Son unas 10 áreas invalidadas por cuadro.

```c
#define PASO 24                      /* paso-puntos */
typedef enum { DER, IZQ, ABAJO } dir_t;
typedef struct { lv_obj_t * obj; dir_t dir; } tramo_t;

static tramo_t tramos[10];
static int n_tramos;
static int32_t fase_q8;              /* fase en 1/256 de px */
static int32_t vel = 50;             /* px/s = 4 * caudal, limitado de 12 a 90 */

static void tramo_dibuja(lv_event_t * e) {
    tramo_t * t = lv_event_get_user_data(e);
    lv_layer_t * capa = lv_event_get_layer(e);
    lv_area_t c;
    lv_obj_get_coords(t->obj, &c);
    int32_t largo = (t->dir == ABAJO) ? lv_area_get_height(&c) : lv_area_get_width(&c);
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = lv_color_hex(0xb6f1fd);                        /* agua-punto */
    d.radius = 3;
    for (int32_t s = (fase_q8 >> 8) + 3; s < largo - 3; s += PASO) {
        int32_t x = (c.x1 + c.x2) / 2, y = (c.y1 + c.y2) / 2;
        if (t->dir == DER) x = c.x1 + s;
        else if (t->dir == IZQ) x = c.x2 - s;
        else y = c.y1 + s;
        lv_area_t a = { x - 3, y - 3, x + 2, y + 2 };
        lv_draw_rect(capa, &d, &a);
    }
}

static void flujo_tick(lv_timer_t * tm) {
    fase_q8 = (fase_q8 + vel * 256 * 33 / 1000) % (PASO * 256);
    for (int i = 0; i < n_tramos; i++) lv_obj_invalidate(tramos[i].obj);
}

void tramo_nuevo(lv_obj_t * padre, int32_t x, int32_t y, int32_t w, int32_t h, dir_t dir) {
    tramo_t * t = &tramos[n_tramos++];
    t->obj = lv_obj_create(padre);
    lv_obj_remove_style_all(t->obj);
    lv_obj_remove_flag(t->obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(t->obj, x, y);
    lv_obj_set_size(t->obj, w, h);
    t->dir = dir;
    lv_obj_add_event_cb(t->obj, tramo_dibuja, LV_EVENT_DRAW_MAIN_END, t);
}
/* al arrancar: lv_timer_create(flujo_tick, 33, NULL); con caudal 0: lv_timer_pause y ocultar los tramos */
```

Cómo no:

- No animes una `lv_line` punteada: en el render por software de LVGL el patrón de guiones va anclado a la x de la pantalla, así que mover la línea no mueve los guiones.
- No hagas un `lv_obj` por punto: cada punto que se mueve invalida dos áreas, con 80 puntos se pasan de las 32 áreas que guarda LVGL (`LV_INV_BUF_SIZE`) y se redibuja la pantalla completa en cada cuadro.

## Iconos

1. Exporta cada SVG del grupo Iconos a PNG con transparencia a 32, 48 y 96 px: `cairosvg tren.svg -o tren_32.png --output-width 32 --output-height 32` (o `rsvg-convert -w 32 -h 32 tren.svg -o tren_32.png`).
2. Conviértelo con el script de LVGL (necesita `pip install pypng lz4`): `python3 lvgl/scripts/LVGLImage.py --ofmt C --cf A8 -o iconos tren_32.png`. Genera `iconos/tren_32.c` con `const lv_image_dsc_t tren_32` en formato `LV_COLOR_FORMAT_A8`.
3. En pantalla: `LV_IMAGE_DECLARE(tren_32);`, `lv_image_set_src(img, &tren_32)` y el color con `image_recolor`.

## Pantalla y retroiluminación

- La wiki de Waveshare de la ESP32-S3-Touch-LCD-5 lista la retroiluminación en EXIO2 del expansor CH422G: es prender y apagar, sin PWM documentado. El apagado por horario sí sale así; para atenuar (Brillo 70 %) en placas de esta familia se ha reportado soldar el pad de control de la retroiluminación a un GPIO con PWM (en la de 7 in). Si no haces esa modificación, deja en Ajustes solo el apagado por horario y quita el brillo.
- Con la pantalla apagada el touch sigue activo: un toque la enciende y ese primer toque no se pasa al control que esté debajo.
