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
    `header` (atrás, título, chip, enlace y "?" según la pantalla), `nav_bar`,
    `confirm_dialog`, `keypad_dialog` (unidad y decimales con `KeypadSpec`),
    `segmented` (selector de 2 a 4 opciones), `mixer` (impulsor que gira y el
    renglón de estado de la bomba R), `pipe` (puntos de flujo), `sensor_tag`.
  - Pantallas de detalle: van en el enum `Screen` después de las 7 pestañas.
    `kPages` en `ui.cpp` dice el encabezado de cada una y qué pestaña marca la
    barra.
  - `screens/`: `home_screen` (Tren), `summary_screen` (Resumen), `flow_screen`
    (Flujo), `cistern_screen` (Cisterna), `mixer_screen` (Mezcla),
    `dose_screen` (Dosis), `history_screen` (Historial), `settings_screen`
    (Ajustes) e `idle_screen` (Pasillo: va en `lv_layer_top()`, encima de los
    diálogos, y `ui::update()` lo abre con `lv_display_get_inactive_time()`).
    Detalles: `flow_sensor_screen` (DetalleF), `pressure_screen` (DetalleP),
    `drop_screen` (DetalleCaida), `filters_screen` (Cartuchos) e
    `injection_screen` (Inyección). Servicio: `events_screen` (Alarmas y
    eventos, desde Historial), `diagnostics_screen` (Diagnóstico, desde
    Ajustes) y `bucket_screen` (Prueba de cubeta, desde Ajustes y DetalleF;
    `show()` le llama `enter()` para que cada vez cuente desde 0),
    `legend_screen` (Símbolos del diagrama, el "?" del Tren) y
    `overview_screen` (Panorama de la casa, Casa en el Tren).
    `alarm_screen` es la alarma a pantalla completa: va en `lv_layer_top()`
    encima de todo, Pasillo incluido.
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

Para ver un estado del canvas sin tocar código: `HMI_DEMO=alarma ./build/simulator`.
Escenarios: `sin-flujo`, `sin-red`, `alarma`, `sensor`, `tope`, `fase1`, `fase15`,
`revisar`, `tapados`, `ro-cocina` y `completo` (`demo::set_scenario` en `simulator/demo.h`).
El demo arranca como la casa de Erick: todo menos la sonda ORP y las bombas A
y B. `HMI_DEMO=completo` prende la sonda para comparar con el canvas.

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
  vista. Detalle en `docs/diseno/ds/README.md`.
- Lo de una fase que no está instalada no se muestra (el DS decía candado y
  "Próximamente"; Erick prefirió esconderlo, octubre 2026). Lo dicen
  `has_pressure` (fase 1.5), `has_orp` y `has_mixer` (fase 2) y `has_pumps`
  (fase 3). Sin la sonda ORP no hay pestaña Cisterna (`NavBar::
  set_tab_visible`): todo lo de ahí sale del ORP. `available()` en `ui.cpp`
  quita del encabezado los enlaces a lo que no está, y Mezcla sin Cisterna
  regresa al Tren. Las pantallas que lo esconden dejan su lugar a lo
  demás: no quedan huecos. Diagnóstico sí lo lista, como mapa de pines.
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

Hecho: las 7 pestañas (Tren, Resumen, Flujo, Cisterna, Dosis, Historial,
Ajustes), Mezcla, Pasillo, los 5 detalles (DetalleF, DetalleP, DetalleCaida,
Cartuchos, Inyección), Alarmas, Diagnóstico, Prueba de cubeta, Leyenda,
Panorama, la alarma a pantalla completa, el aviso de sin red en
el encabezado, el diálogo de confirmar y el teclado (Registrar otra cantidad, Calibrar con tira y los
minutos de la bomba R).

Pendiente, en este orden:

1. Firmware de la 7B y del nodo.

## Diferencias a propósito con el canvas

- El label "V3" mide 40 px en lugar de 34.
- Los chips de fecha y hora del teclado solo muestran; no hay selector diseñado.
- "P1 0.0 P2 0.0" en Ajustes y en DetalleP es el corrimiento del cero que se
  guardó con Hacer cero (`p1_zero_psi`), no la lectura en vivo.
- En Flujo el aviso "Sin flujo" va junto al título "Caudal" y no en x 578: mide
  174 px y en el canvas queda abajo de la tarjeta de Litros hoy (x 700).
- En Flujo la cifra de 232 px va en y 57 y no en 40 (ver la nota de cifras en
  "LVGL 9.6: lo que ya mordió"); así cae donde la dibuja el canvas.
- Cisterna sin tira dice "Sin calibrar" y no estima el cloro. El canvas solo
  dibuja el caso con sonda y calibración.
- "Calibrar con tira" abre el teclado en mg/L con un decimal y guarda la tira con
  el ORP de ese momento (`Actions::strip_checked`). El canvas no tiene esa vista.
  La pista dice "ORP de ahorita: 650 mV": las pistas del teclado tienen 426 px.
- En Cisterna, "mV" y el estado se recorren a la derecha si el ORP negativo no
  cabe antes de x 318.
- En Historial, Presiones sale con P1 y P2 y ORP con la sonda; con solo Litros
  y dosis no hay selector. Las marcas de abajo miden 100 px y no 80 para que
  "14 sep" no se parta, y el día no lleva "24:00" (se encimaba con "18:00").
  Los ejes son los del canvas (600 L por hora, 3,000 por día, 60 mL, 60 psi)
  pero crecen si un dato no cabe (`nice_step`): antes la barra se salía hasta
  el título. Con 100 mL o más, las marcas de la derecha se recorren 12 px.
- En DetalleP, "Con V1 cerrada y sin presión" no cabe en 348 px (en el canvas se
  sale de la tarjeta): dice "Con V1 cerrada, sin presión".
- La curva de "nuevos" en DetalleCaida se puntea a mano: LVGL solo puntea
  líneas horizontales o verticales.
- Textos que en el canvas tampoco caben y se quedaron igual, para arreglar en el
  diseño: en Cartuchos "vaso vacío" queda abajo de la barra y "Lo cambié" toca
  el borde de su botón.
- En DetalleCaida los textos que no cabían se acortaron: "con 12.4 L/min",
  "Mismo caudal: 12.4 L/min" y "Subió 1.4 psi desde 2 sep" (de 10 psi para
  arriba sin decimal). "Si llega a Revisar" se leía como verbo: dice "Si llega
  a 5 psi (Revisar)".
- Los vasos del Tren salen llenos o vacíos según `filters[].installed` (en el
  canvas el del sarro siempre está vacío). Poner cartucho en un vaso vacío se
  registra como `EventKind::cartridge_added`: "· puesto" en Historial y "Se
  puso cartucho Sarro" en Alarmas. No hay "Quitar".
- En la lista de Historial las alarmas van con su título corto
  (`fmt::alarm_short`): "Sin flujo mientras dosifica" no cabe en 358 px.
- Sin red, el encabezado esconde su enlace para que quepa el chip (el canvas lo
  hace en Flujo; aquí en todas).
- La alarma sale mientras haya una activa sin reconocer (`SystemState::alarms`).
  Entendido la quita y avisa por `Actions::alarm_acknowledged`; la pastilla
  sigue en Alarma hasta que la alarma deje de estar activa.
- En Pasillo, con 100 mL o más la cifra de Toca echar baja a 72 px (a 136 no
  cabe antes de la orilla). Al entrar se cierran el diálogo y el teclado: el
  toque para salir regresa al Tren.
- En Ajustes, "Cambiar red" y "Cambiar horario" salen deshabilitados: el
  canvas no tiene la vista que abren. "Hacer cero"
  pide confirmación (el DS lo pide para lo que reinicia algo) y el envío de
  datos dice "Por enviar" en ámbar si hay pendientes.
- En Mezcla, los renglones del programa Auto abren el teclado en minutos (de 1
  a 99) y sin bomba R la pantalla se ve bloqueada. El selector marca la opción
  al tocarla y el estado la confirma en el siguiente `update()`.
- Alarmas e Historial leen el mismo registro (`SystemState::recent_events`).
  El demo junta los eventos de los dos canvas, así que la lista no sale igual
  a ninguno; con más de 5 renglones se recorre con el dedo. "Activas" lista
  las alarmas activas (reconocidas o no) con "Desde 18:42"; el canvas solo
  dibuja el caso sin ninguna. Historial no muestra los "Sin red".
- En Diagnóstico, "Probar alarma" va sin ícono: con él no cabe en 239 px (en el
  canvas se sale del botón). Lo que es de una fase que no está instalada dice
  "fase 1.5" o "fase 2" en texto-3 según `has_pressure`, `has_orp` y
  `has_mixer`; el canvas dibuja la fase 1.5 (`HMI_DEMO=fase15`). En el demo,
  Probar alarma prende la de sin flujo y se quita al tocar Entendido. El
  expansor dice CH32V003 y no CH422G: el canvas es de la 5B y la 7B trae un
  CH32V003, que también lleva el PWM del backlight
  (https://github.com/esphome/feature-requests/issues/3202). A3 es el LDR del
  brillo (`has_ldr`, `ldr_volts`); DI1 y A2 van juntos como libres para que la
  tarjeta siga de 5 renglones.
- En Prueba de cubeta el demo cuenta como si F diera 387.2 pulsos por litro y
  se para en 3,872 (la marca), para que proponga 387 como el canvas.
- Prueba de cubeta: el canvas da los pasos 1 y 2 por hechos y cuenta desde
  que se entra. Aquí cada uno tiene su botón ("Ya la cerré", "Ya está") y el
  conteo empieza en 0 en el paso 3: con V2 abierta F contaría agua que no cae
  en la cubeta. V4 se cierra en la marca antes de "Ya llegué" (paso 3 "Abre V4
  y ciérrala en la marca", paso 4 "Vuelve a abrir V2"): así lo contado es lo
  que hay en la cubeta y no cuenta qué tan rápido se toca. "Ya llegué" se
  prende cuando F lleva 1.5 s sin contar, y "Guardar" solo se deja si el
  factor queda a 30 % del de fábrica (277 a 514); si no, dice que se repita.
- En Leyenda los títulos van en etiqueta (26 Medium) y no en 26 SemiBold como
  en el canvas: el DS no tiene ese estilo y no vale otra fuente por eso. Los
  dibujos son símbolos del P&ID de 72 x 60 (`pid/svg/leyenda_*.svg`); el tubo
  de "Con flujo" corre siempre.
- En Panorama la tarjeta del Tren dice "Caída 4 psi" con el ícono del estado
  y no "Caída 4 psi · Normal": mide 264 px y caben 232 (en el canvas se sale).
  La flecha va pegada al título por lo mismo. El presostato mide lo que su
  texto (en el canvas "35 a 65 psi" se sale de 140 px) y queda centrado en el
  tubo. Sin P1 y P2 no sale la caída, sin sonda no sale el ORP y sin bomba R
  no sale la R. El RO de la cocina sale con `ro_kitchen_planned`
  (`HMI_DEMO=ro-cocina`); su borde punteado se dibuja a mano.
- Leyenda no lleva la tarjeta "Fase futura": ya no hay candados que explicar.
- Resumen: donde no van Filtros (fase 1.5) o Bomba R (fase 2) sale "Cartuchos
  · el que sigue" (el de más vida gastada, `core::filter_wear_pct`), a lo
  ancho si faltan los dos. No está en el canvas; copia el acomodo de la de la
  bomba R. "Ya toca" va en ámbar con la palabra.
- Sin bombas A y B (`has_pumps`, fase 3) Dosis no lleva su tarjeta y lo demás
  mide 992; Inyección no se abre (el enlace del encabezado se quita) y la B
  del Tren lleva a Dosis. En el Tren, sin P1 y P2, Hoy y Toca echar se
  reparten el renglón de abajo.
