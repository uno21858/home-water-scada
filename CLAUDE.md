# home-water-scada

HMI casera para el tren de filtros y la cloración de la cisterna. Corre en una
Waveshare ESP32-S3-Touch-LCD-7B (1024 x 600, touch GT911) con LVGL 9.6.0 en C++17.
Mientras llega el hardware, se desarrolla en el simulador con SDL (Mac y Fedora).

## Cómo trabajar con Erick

- Español. Comentarios del código en español; nombres en inglés.
- Cuando él tenga que correr algo en su máquina, un paso a la vez: da uno y
  espera su resultado.
- Está aprendiendo C++ (punteros, clases, callbacks): explica en corto el patrón
  que usaste en cada cambio.

## Estructura

- `core/`: lógica pura, sin LVGL ni hardware. `system_state.h` es el estado que
  muestra la interfaz y los umbrales del DS (caída del tren, ORP).
- `ui/`: la interfaz. No sabe nada de SDL ni del ESP32.
  - `ui.h/.cpp`: `init()`, `update()`, `show()` y las `Actions` que regresan a la
    app. Una sola screen con encabezado, contenido y barra fijos; cada pantalla es
    un contenedor dentro del contenido que se esconde o se muestra, nada se recrea.
  - `theme/`: `colors.h` (tokens del DS), `fonts/` (generadas), `icons/` (A8,
    generados), `pid/` (símbolos del P&ID en ARGB8888, generados), `styles`,
    `format` (números al formato de México).
  - `components/`: `primitives` (box, label, icon, card, link_to, botones),
    `header` (atrás, título, chip y enlace según la pantalla), `nav_bar`,
    `confirm_dialog`, `keypad_dialog` (unidad y decimales con `KeypadSpec`),
    `segmented` (selector de 2 a 4 opciones), `mixer` (impulsor que gira y el
    renglón de estado de la bomba R), `pipe` (puntos de flujo), `sensor_tag`.
  - Pantallas de detalle: van en el enum `Screen` después de las 7 pestañas.
    `kPages` en `ui.cpp` dice el encabezado de cada una y qué pestaña marca la
    barra.
  - `screens/`: `home_screen` (Tren), `summary_screen` (Resumen), `flow_screen`
    (Flujo), `cistern_screen` (Cisterna), `mixer_screen` (Mezcla),
    `dose_screen` (Dosis), `history_screen` (Historial). Ajustes todavía es un
    placeholder en `ui.cpp`.
  - Lo que la pantalla le pide a la app sale por `actions()`; un campo nuevo
    se captura con `open_keypad(KeypadSpec, callback)`.
- `simulator/`: ventana SDL. `demo.cpp` llena el estado con los datos de ejemplo
  del DS y lo mueve cada segundo.
- `firmware/hmi` (la 7B) y `firmware/nodo` (ESP32-S3 en la caja del pilar con el
  YF-B6, ADS1115, EZO-ORP y bombas, enlazado por RS485): vacíos todavía.
- `hardware/nodo`: proyecto de KiCad del nodo.
- `docs/diseno/`: la especificación de diseño. Leer `docs/diseno/LEEME.md`.

El firmware hará lo mismo que `simulator/main.cpp`: llenar `core::SystemState`,
llamar `ui::update()` y atender lo que llegue por `ui::Actions`.

## Compilar y correr el simulador

```bash
git submodule update --init           # LVGL fijado en v9.6.0
cmake -S simulator -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/simulator
```

En Mac, SDL2 viene de Homebrew (`brew install sdl2`). Si CMake no lo encuentra:
`-DCMAKE_PREFIX_PATH=/opt/homebrew`.

## Reglas del código

- Clases en PascalCase y archivos en snake_case (`SummaryScreen` en
  `summary_screen.h`), todo en `namespace ui`. Miembros privados con `_` al final.
- Sin tema de LVGL (`lv_display_set_theme(..., nullptr)` en `ui::init`): todo el
  look sale de `theme/styles` y de las piezas de `components/primitives`.
- Las medidas se copian de la pantalla en `docs/diseno/pantallas/`: cada y menos 64.
- Los labels se actualizan solo si cambian (`set_text()`).
- Formato de México: `1,860` y `12.4`. Abajo de 10 se rellena con espacio de cifra
  U+2007 para que el número no brinque (`fmt::one_decimal`).
- Estado nunca solo en color: siempre con palabra o ícono. Una acción primaria por
  vista. Lo de fases futuras se ve pero no se toca: candado y texto en texto-3.
  Detalle en `docs/diseno/ds/README.md`.
- Fuentes, íconos y símbolos se regeneran con el `generate.sh` de su carpeta; no
  se editan los `.c` a mano.

## LVGL 9.6: lo que ya mordió

- Los headers públicos están en `simulator/lvgl/include/lvgl/`. Los de `src/` son
  stubs que solo avisan.
- `lv_obj_add_flag()` y `lv_obj_remove_flag()` están deprecadas desde la 9.6 y se
  quitan en la 10. Usar `lv_obj_set_hidden()`, `lv_obj_set_clickable()`,
  `lv_obj_set_scrollable()`, `lv_obj_set_disabled()`...
- Las fuentes se generan con `--no-compress`: no hace falta
  `LV_USE_FONT_COMPRESSED` y la ESP32 no descomprime cada letra.
- Un label con ancho fijo parte la palabra por letras si no cabe ("V3" en 34 px
  salía en dos renglones). Darle ancho suficiente.
- Cifra y unidad en la misma línea base: fila flex con `LV_FLEX_ALIGN_END` y 2 px
  de `pad_bottom` a la unidad cuando la cifra es de 72 y la unidad de 36
  (`docs/diseno/ds/lvgl.md`).
- Callbacks: funciones `static` o lambdas sin captura; el objeto llega por
  `user_data`.
- Con la misma caja, las fuentes de cifras quedan más arriba que en el canvas:
  el navegador centra la fuente en el interlineado y LVGL usa `base_line`.
  Medido: 232 px 17 px más arriba, 136 px 10 y 72 px 5; por cuenta, 44 px unos
  4. Las cifras grandes que van solas abajo de un título se bajan esos px para
  no encimarse (232 en Flujo, 136 en Cisterna); las de 72 y 44 se quedan como en
  Resumen y Dosis.
- Gráficas que no caben en un `lv_chart` (barras pareadas, banda entre dos
  líneas): un objeto con `LV_EVENT_DRAW_MAIN_END` que dibuja con `lv_draw_rect`,
  `lv_draw_line` (con `points` para una polilínea) y `lv_draw_triangle`. Las
  coordenadas son de pantalla y los arreglos de puntos tienen que seguir vivos
  hasta que termine el dibujo: van como miembros, no en la pila. Ver Historial.
- Línea punteada: `lv_line` con `line_dash_width` y `line_dash_gap`. Solo sale
  en líneas horizontales o verticales, y el patrón va pegado a la x de la
  pantalla: no se anima.

## Antes de dar por buena una pantalla

1. Compila sin warnings propios con gcc y con clang.
2. Corre el simulador y compárala contra su pantalla en `docs/diseno/pantallas/`,
   en cada estado de sus `data-props`.
3. Revisa que los números salgan con el formato del DS.

## Estado (octubre 2026)

Hecho: Tren (inicio), Resumen, Flujo, Cisterna, Mezcla, Dosis, Historial, el
diálogo de Ya le puse y el teclado (Registrar otra cantidad, Calibrar con tira y los
minutos de la bomba R).

Pendiente, en este orden:

1. Ajustes.
2. Pasillo (reposo a los 2 min sin tocar). El `header` ya tiene atrás, chip y
   enlace; le faltan el "?" de ayuda (Leyenda) y el chip de sin red. Cada enlace
   se prende en `kPages` cuando exista su pantalla: Casa → Panorama, Sensor F,
   Inyección, Alarmas, Diagnóstico.
3. Detalles: DetalleF, DetalleP, DetalleCaida, Cartuchos, Inyeccion.
4. Estados globales: alarma a pantalla completa (EstadoAlarma) y sin red.
5. Servicio: Alarmas, Diagnóstico, Prueba de cubeta, Leyenda, Panorama.
6. Firmware de la 7B y del nodo.

## Diferencias a propósito con el canvas

- P1 y P2 bloqueadas dicen "1.5" y no "fase 1.5": con el candado no cabe en 172 px
  (en el canvas se sale de la etiqueta).
- El label "V3" mide 40 px en lugar de 34.
- P1, P2, los vasos y la caída del Tren todavía no abren nada: sus pantallas de
  detalle no existen.
- Los chips de fecha y hora del teclado solo muestran; no hay selector diseñado.
- En Flujo el aviso "Sin flujo" va junto al título "Caudal" y no en x 578: mide
  174 px y en el canvas queda abajo de la tarjeta de Litros hoy (x 700).
- En Flujo la cifra de 232 px va en y 57 y no en 40 (ver la nota de cifras en
  "LVGL 9.6: lo que ya mordió"); así cae donde la dibuja el canvas.
- El encabezado de Flujo todavía no tiene el enlace "Sensor F" a DetalleF.
- Cisterna sin sonda ORP (`has_orp` falso) se ve como una tarjeta bloqueada de
  fase 2, y sin tira dice "Sin calibrar" y no estima el cloro. El canvas solo
  dibuja el caso con sonda y calibración.
- "Calibrar con tira" abre el teclado en mg/L con un decimal y guarda la tira con
  el ORP de ese momento (`Actions::strip_checked`). El canvas no tiene esa vista.
- En Cisterna, "mV" y el estado se recorren a la derecha si el ORP negativo no
  cabe antes de x 318.
- En Historial, Presiones sin P1 y P2 y ORP sin sonda muestran "Próximamente ·
  fase 1.5" o "fase 2" en lugar de la gráfica. Las marcas de abajo miden 100 px
  y no 80 para que "14 sep" no se parta. "18:00" y "24:00" se tocan igual que en
  el canvas.
- En Mezcla, los renglones del programa Auto abren el teclado en minutos (de 1
  a 99) y sin bomba R la pantalla se ve bloqueada. El selector marca la opción
  al tocarla y el estado la confirma en el siguiente `update()`.
