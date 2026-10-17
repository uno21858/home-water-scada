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

- `core/`: lógica pura, sin LVGL ni hardware; la usan igual el simulador y el
  firmware. `system_state.h` es el estado que muestra la interfaz y los
  umbrales: la caída del tren contra la de cartuchos nuevos al mismo caudal
  (`drop_level`, documento pág. 40), el ORP del DS y el sin agua del SIAPA
  (`supply_lost`). Lo que decide:
  - `water`: `count_liters` (lo que contó F), `register_dose` y
    `register_filter`.
  - `mixing`: el programa de la bomba R (no `mixer.h`: chocaría con
    `ui/components/mixer.h` al incluirlo desde `ui/components/`). En Auto corre 45 min después de cada
    dosis y 15 cada `kMixEveryLiters` (1,000 L) de agua nueva; Prendida corre
    60 y queda Apagada. Con un aviso del INA219 la vuelta queda pendiente, y
    con la cisterna en 0 % (el mínimo de Calibrar nivel) no arranca o se
    apaga: corre cuando sube el agua.
  - `monitor`: `tick_minute` (cada minuto) y el modo de cambio de cartuchos
    (`service_start`, `service_end`).
  - `events`: el registro que leen Alarmas e Historial.
  - `level`: el sensor de nivel 4-20 mA del A2 (`level_cm` desde el fondo y
    `level_pct`, lo útil de la rejilla de la bomba de pozo a llena) y su
    calibración (`level_zero` en el aire, `level_mark_full`).
  - `test/core_test.cpp`: pruebas en la compu (`ctest --test-dir build`).
- `ui/`: la interfaz. No sabe nada de SDL ni del ESP32.
  - `ui.h/.cpp`: `init()`, `update()`, `show()` y las `Actions` que regresan a la
    app. Una sola screen con encabezado, contenido y barra fijos; cada pantalla es
    un contenedor dentro del contenido que se esconde o se muestra, nada se recrea.
  - `theme/`: `colors.h` (tokens del DS), `fonts/` (generadas), `icons/` (A8,
    generados), `pid/` (símbolos del P&ID en ARGB8888, generados), `styles`,
    `format` (números al formato de México).
  - `components/`: `primitives` (box, label, icon, card, link_to, botones),
    `header` (atrás, título, enlace y "?" según la pantalla), `status_pill`
    (OK, Advertencia, Alarma o Mantenimiento; la usan encabezado y Pasillo), `nav_bar`,
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
    `legend_screen` (Símbolos del diagrama, el "?" del Tren),
    `overview_screen` (Panorama de la casa, Casa en el Tren),
    `spares_screen` (Repuestos en casa, Cambiar en Ajustes > Cartuchos),
    `service_screen` (Cambio de cartuchos, el enlace Cambio de Cartuchos),
    `level_screen` (Nivel: la pestaña Cisterna sin sonda ORP, o el enlace
    Nivel de Cisterna con ella) y `level_calibration_screen` (Calibrar nivel).
    `alarm_screen` es la alarma a pantalla completa: va en `lv_layer_top()`
    encima de todo, Pasillo incluido.
  - Lo que la pantalla le pide a la app sale por `actions()`; un campo nuevo
    se captura con `open_keypad(KeypadSpec, callback)`.
- `simulator/`: ventana SDL. `demo.cpp` llena el estado con los datos de ejemplo
  del DS y lo mueve cada segundo.
- `firmware/hmi` (la 7B): vacío todavía. Según el documento de proyecto (15 oct
  2026) hay un solo micro, la 7B. F entra al GPIO6 (PCNT) y la bomba R sale
  por un MOSFET en el GPIO44, vigilada por un INA219 (0x40). En la caja
  Hammond, junto a la pantalla y por I2C, va el ADS1115 (0x48: A0 P1, A1 P2,
  A2 libre para un sensor de nivel, A3 el LDR). P1 y P2 entran por un divisor
  10k/10k (Erick lo dejó así, octubre 2026; el documento lo trae como opción):
  el firmware multiplica por 2 y `p1_volts` es la salida del sensor. El
  nivel es un transmisor hidrostático 4-20 mA en A2 por una de 33 Ω
  (0.132 a 0.660 V; documento pág. 30): `level_volts` es lo que lee el A2. El
  INA219 apaga la bomba R y deja `mixer_fault` (sin 12 V, nivel bajo,
  atorada); el firmware lo quita cuando se arregla. El RS485 de la 7B es solo para
  el EZO-ORP de la fase 2, y las bombas A y B (fase 3) van por un PCF8574
  (0x20).
- `firmware/nodo` y `hardware/nodo`: del plan viejo con un segundo ESP32 por
  RS485, que el documento ya no lleva. Vacíos.
- `docs/diseno/`: la especificación de diseño. Leer `docs/diseno/LEEME.md`.

El firmware hará lo mismo que `simulator/main.cpp` y `demo.cpp`: leer los
sensores, pasarlos a `core` (`count_liters`, `mixer_report_fault`...), llamar
`core::tick_minute()` cada minuto y `ui::update()`, atender lo que llegue por
`ui::Actions` llamando a `core`, y prender el GPIO44 según `mixer_on`.

## Compilar y correr el simulador

```bash
git submodule update --init           # LVGL fijado en v9.6.0
cmake -S simulator -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/simulator
ctest --test-dir build                # pruebas de core
```

Para ver un estado del canvas sin tocar código: `HMI_DEMO=alarma ./build/simulator`,
o el target de ese escenario (`cmake --build build --target demo-alarma`; en
CLion sale en la lista de configuraciones). La lista de targets está en
`DEMO_SCENARIOS` de `simulator/CMakeLists.txt`: si se agrega un escenario al
demo, va también ahí.
Escenarios: `sin-flujo`, `sin-red`, `alarma`, `sensor`, `tope`, `fase1`,
`revisar`, `tapados`, `sin-agua`, `cisterna-vacia`, `sin-12v`, `nivel-bajo`, `atorada`,
`ro-cocina` y `completo` (`demo::set_scenario` en `simulator/demo.h`).
El demo arranca como la casa de Erick, en la fase 1.5: todo menos la sonda ORP
y las bombas A y B. `HMI_DEMO=completo` prende la sonda para comparar con el
canvas. Dosis con 0.1 mL por litro (la prueba de jarras del documento: el
canvas usa 0.02, así que sus mL salen 5 veces más chicos) y 45 min de mezcla
después de cada dosis; registrar una dosis prende la bomba R.

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
  `has_mixer` (fase 1), `has_pressure` (fase 1.5), `has_orp` (fase 2) y
  `has_pumps` (fase 3). La pestaña Cisterna sale con la sonda ORP o con el
  sensor de nivel (`has_level`, `NavBar::set_tab_visible`); sin sonda,
  `show()` manda la pestaña a Nivel y con las dos Cisterna lleva el enlace
  "Nivel". `available()` en `ui.cpp` quita del encabezado los enlaces a lo
  que no está, y Mezcla sin Cisterna regresa al Tren. Las pantallas que lo esconden dejan su lugar a lo
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

1. Firmware de la 7B.

En `core` falta todavía (hoy lo hace el demo o nadie): los avisos de caída
(necesitan histéresis para no prenderse y apagarse en el borde), las alarmas
de la fase 3 y el cambio de día.

## Diferencias a propósito con el canvas

- El label "V3" mide 40 px en lugar de 34.
- V2 y V5 van al revés que en el canvas, como está en la casa: V2 arriba en la
  bajada (se cierra para abrir V3 y echar el cloro en B) y V5 abajo, la salida
  a la cisterna (se cierra para purgar por V4). Por eso Prueba de cubeta dice
  "Cierra V5" y "Vuelve a abrir V5" y no V2 como el canvas.
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
  arriba sin decimal). "Si llega a Revisar" se leía como verbo: dice "Si sube
  5 psi (Revisar)".
- La caída se califica contra la de cartuchos nuevos al mismo caudal, como
  dice el documento (Revisar a +5 psi y Tapados a +10), no con 5 y 10 psi
  fijos como el DS. La cifra grande sigue siendo P1 − P2, pero la escala de
  DetalleCaida va de 0 a "+15 psi" y la marca es lo que subió.
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
- En Ajustes, "Cambiar red" sale deshabilitado: el canvas no tiene la vista
  que abre. Pantalla no lleva la tarjeta "Apagar la luz de noche" del canvas:
  el brillo lo lleva el LDR del firmware (de noche baja solo) y el Pasillo ya
  se traga el primer toque, así que no hace falta apagarla. "Hacer cero"
  pide confirmación (el DS lo pide para lo que reinicia algo) y el envío de
  datos dice "Por enviar" en ámbar si hay pendientes.
- En Mezcla, los renglones del programa Auto abren el teclado en minutos (de 1
  a 99) y sin bomba R la pantalla se ve bloqueada. El selector marca la opción
  al tocarla y el estado la confirma en el siguiente `update()`. "Cada 4
  horas" del canvas es "Cada 1,000 L de agua" (Erick, octubre 2026: lo que
  deja dispareja la cisterna es el agua nueva, y de noche no entra); en las
  últimas veces dice "agua" y apagada en Auto dice "la siguiente en 850 L".
- Cambio de cartuchos no está en el canvas: con V1 cerrada P1 baja a 0 igual
  que si se fuera el SIAPA. Cartuchos lleva el enlace "Cambio" en el
  encabezado y abre los pasos con "Empezar" y "Ya terminé" (acomodo de Prueba
  de cubeta). Mientras dura (60 min o Ya terminé) no hay avisos de sin agua
  ni de caída y la pastilla dice "Mantenimiento" en gris; un aviso de la
  bomba R o una alarma pesan más y se ven igual.
- Alarmas e Historial leen el mismo registro (`SystemState::recent_events`).
  El demo junta los eventos de los dos canvas, así que la lista no sale igual
  a ninguno; con más de 5 renglones se recorre con el dedo. "Activas" lista
  las alarmas activas (reconocidas o no) con "Desde 18:42" y los avisos que
  siguen sin quitarse; el canvas solo dibuja el caso sin ninguna. Historial
  no muestra los "Sin red".
- Avisos que no están en el DS, en ámbar con palabra (pastilla en
  Advertencia, sin pantalla completa):
  - Bomba R (documento pág. 40): "Sin 12 V", "Nivel bajo" o "Atorada" en
    lugar de Prendida o Apagada en Mezcla, Resumen y la R del Tren, con lo
    que pasa abajo ("No se mezcló: espera agua nueva"). En Alarmas e Historial
    "Bomba R · Atorada". El demo lo quita al tocar otro modo (sin 12 V no).
  - Sin agua del SIAPA: sin flujo y P1 abajo de 3 psi al mismo tiempo (sin
    flujo con presión es la cisterna llena). "Sin agua" arriba de SIAPA en el
    Tren, "Sin agua del SIAPA" en lugar de "Sin flujo" en Flujo, Resumen y
    Pasillo, y en Alarmas e Historial.
- Registrar una dosis prende la mezcla (`mixer_after_dose_min`) y la bomba R
  dice "Por la dosis de las 18:42".
- En Diagnóstico, "Probar alarma" va sin ícono: con él no cabe en 239 px (en el
  canvas se sale del botón). El canvas es de la 5B con un nodo; aquí es el
  mapa de pines de la 7B del documento: Entradas (GPIO6, A0, A1, A3 y A2
  libre), Bus I2C (GT911, CH32V003, ADS1115, INA219) y "Salidas y RS485"
  (GPIO44 bomba R, 0x20 bombas A y B, RS485 sonda ORP). Lo de una fase que no
  está instalada dice "fase 1.5", "fase 2" o "fase 3" en texto-3. La bomba R
  dice su corriente del INA219 si anda ("1.58 A": "prendida · 1.58 A" no
  cabe), "apagada" si no y "sin 12 V" en ámbar si no le llega voltaje. En el
  demo, Probar alarma prende la de sin flujo y se quita al tocar Entendido. El
  expansor dice CH32V003 y no CH422G: la 7B trae un CH32V003, que también
  lleva el PWM del backlight
  (https://github.com/esphome/feature-requests/issues/3202). A3 es el LDR del
  brillo (`has_ldr`, `ldr_volts`).
- En Prueba de cubeta el demo cuenta como si F diera 387.2 pulsos por litro y
  se para en 3,872 (la marca), para que proponga 387 como el canvas.
- Prueba de cubeta: el canvas da los pasos 1 y 2 por hechos y cuenta desde
  que se entra. Aquí cada uno tiene su botón ("Ya la cerré", "Ya está") y el
  conteo empieza en 0 en el paso 3: con V5 abierta F contaría agua que no cae
  en la cubeta. V4 se cierra en la marca antes de "Ya llegué" (paso 3 "Abre V4
  y ciérrala en la marca", paso 4 "Vuelve a abrir V5"): así lo contado es lo
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
- Resumen: donde no van Filtros (fase 1.5) o Bomba R (fase 1) sale "Cartuchos
  · el que sigue" (el de más vida gastada, `core::filter_wear_pct`), a lo
  ancho si faltan los dos. No está en el canvas; copia el acomodo de la de la
  bomba R. "Ya toca" va en ámbar con la palabra.
- Sin bombas A y B (`has_pumps`, fase 3) Dosis no lleva su tarjeta y lo demás
  mide 992; Inyección no se abre (el enlace del encabezado se quita) y la B
  del Tren lleva a Dosis. Ajustes no lleva "Tope diario" (es de cada bomba) y
  el resumen de Dosis dice "0.100 mL por litro". En el Tren, sin P1 y P2, Hoy y Toca echar se
  reparten el renglón de abajo.
- Nivel y Calibrar nivel no están en el canvas. Nivel copia el acomodo de
  Cisterna: a la izquierda el tanque (útil en azul arriba de la rejilla de la
  bomba de pozo, gris abajo; Llena y Toma con sus cm), a la derecha el %
  útil en la cifra de 136 y los cm. Sin marcar "Está llena" dice "Sin
  calibrar". Calibrar nivel son 5 renglones como Cartuchos, en el orden en
  que se instala: rango de la etiqueta, cero en el aire, altura del sensor
  (2 o 3 cm del fondo para que no lo tape el lodo), la rejilla y Está llena.
  En el Tren el agua de la cisterna sube y baja con el nivel y dice "76 %
  útil" (con sonda ORP, "Cisterna · 76 %"). Diagnóstico muestra el A2 en V.
  "Toma de la bomba de pozo" es el 0 %: Erick pone 60 cm (la rejilla anda
  en ~45 y la bomba R en ~50, así quedan cubiertas). En 0 % la bomba R
  espera agua ("Cisterna en 0 %: espera agua"). El demo trae un sensor de 0
  a 2 m, el mínimo en 60, lleno a 183 cm y con 150 cm (73 %); `sin-agua` lo
  baja a 70 cm y `cisterna-vacia` a 58 (0 %).
- Repuestos en casa no está en el canvas: Ajustes > Cartuchos lleva
  "Cambiar" junto a "Ver cartuchos" y abre un renglón por cartucho (del 20 µm
  al sarro; el prefiltro no lleva) con − y + de 0 a 99. Cada toque se ve al
  instante y avisa por `Actions::spares_set`; siguen bajando solos con "Lo
  cambié".
