Interfaz táctil del sistema casero de monitoreo y cloración de agua. Corre en una Waveshare ESP32-S3-Touch-LCD-5B (1024 x 600, IPS, touch capacitivo GT911) con firmware en LVGL 9, montada en un pasillo techado a 1.45 m. Se opera de pie a 0.5 m y las cifras principales se leen a 2 m. Estilo SCADA sobrio: fondo oscuro, gris para lo normal y color solo cuando significa algo. Todo se dibuja con rectángulos, texto, líneas, arcos y gráficas simples; nada de sombras, degradados, fotos ni transparencias, salvo el velo plano detrás de los diálogos.

Las 6 pantallas y los 4 estados globales están como componentes de los grupos Pantallas y Estados, cada uno a 1024 x 600 exactos; sus PNG están en el grupo de assets Frames. La Hoja de componentes junta paleta, escala tipográfica, botones, estados e íconos. La sección Implementación en LVGL 9 dice cómo pasar cada pieza a código.

## Principios

- Gris es normal. `agua` es agua y flujo; `verde` es en rango; `ambar` es advertencia; `rojo` es solo alarma. Ninguno se usa como decoración.
- El estado nunca va solo en color: siempre lleva palabra o ícono (OK, Advertencia, Alarma; Normal, Revisar, Cartuchos tapados; Reductor, Neutro, Oxidante).
- Una cifra, una unidad: la cifra en un estilo `cifra-*` y la unidad pegada en `etiqueta` con `texto-2`.
- Una acción primaria por vista, en `accion` (Ya le puse, Entendido, Guardar). Todo lo demás es secundario.
- Nada tapa los datos salvo una alarma. El aviso de red vive en el encabezado.
- Lo que todavía no existe (fase 2 y fase 3) se ve pero no se toca: `texto-3`, panel con borde `borde`, candado y la palabra Próximamente.

## Pantalla, rejilla y tacto

- Frame fijo de 1024 x 600 horizontal, con coordenadas absolutas igual que en LVGL.
- Encabezado de y 0 a 64 (`alto-encabezado`), contenido de y 64 a 520 con margen `esp-16`, navegación de y 520 a 600 (`alto-navegacion`) con 6 pestañas de 170 o 171 px.
- Paso de pixel estimado: 0.107 mm (5.0 in nominales, pixel cuadrado). Waveshare no publica el área activa de la 5B: al recibirla mide el ancho iluminado en mm, divídelo entre 1024 y corrige la tabla de legibilidad.
- 56 px son 6.0 mm (`tactil-minimo`): es el piso, no la meta. Acciones frecuentes a 80 px (8.6 mm, `alto-boton`), secundarias, segmentos y filas a 64 px (`alto-boton-2`), pestañas de 170 x 80 (18 x 8.6 mm). Hueco mínimo entre objetivos: 12 px (`separacion-tactil`).

## Legibilidad

ANSI/HFES 100-2007 pide que cada carácter subtienda al menos 16 minutos de arco (2.3 mm a 50 cm) y de 20 a 22 para lectura cómoda. Con 0.107 mm por pixel y la altura de mayúscula y dígito de Montserrat (0.70 em):

| Estilo | Tamaño | Dígito o mayúscula | A 0.5 m | A 2 m |
|---|---|---|---|---|
| `cifra-hero` | 232 px | 162 px · 17.4 mm | 119.5′ | 29.9′ |
| `cifra-xl` | 136 px | 95 px · 10.2 mm | 70.0′ | 17.5′ |
| `cifra-lg` | 72 px | 50 px · 5.4 mm | 37.1′ | 9.3′ |
| `titulo-xl` | 48 px | 34 px · 3.6 mm | 24.7′ | 6.2′ |
| `cifra-md` | 44 px | 31 px · 3.3 mm | 22.7′ | 5.7′ |
| `titulo` | 36 px | 25 px · 2.7 mm | 18.5′ | 4.6′ |
| `texto` | 32 px | 22 px · 2.4 mm | 16.5′ | 4.1′ |
| `etiqueta` | 26 px | 18 px · 1.9 mm | 13.4′ | 3.3′ |

- A 2 m se leen el caudal en Flujo (`cifra-hero`), los mL que tocan en Dosis y el ORP en Cisterna (`cifra-xl`) y el color de la pastilla de estado. Tren se lee de cerca: tiene siete valores vivos y a tamaño de 2 m no caben.
- `etiqueta` queda debajo del mínimo a 0.5 m. Úsala solo junto a algo más grande (unidad, eje, fecha, nombre de equipo) y nunca para algo que haya que leer para actuar.

## Color

- `fondo` en toda pantalla y debajo del P&ID; `panel` en tarjetas, encabezado y navegación; `panel-alto` en lo que se toca o sobresale (botón secundario, etiqueta de valor, pestaña activa).
- `borde` es decorativo (1 px). `borde-control` es el borde de 2 px de lo que importa: controles, recipientes del tren, cisterna.
- Texto principal en `texto`, etiquetas y unidades en `texto-2`, deshabilitado y ejes en `texto-3`. Cada par de texto pasa 4.5:1 sobre `fondo`, `panel` y `panel-alto`; cada trazo con significado pasa 3:1.
- Agua: cifra de caudal y línea de caudal en `agua`; tubería con flujo en `agua-tubo` con puntos `agua-punto`; tubería sin flujo en `tuberia`.
- Estados: texto `verde`, `ambar` o `rojo` sobre su `verde-fondo`, `ambar-fondo` o `rojo-fondo`, con borde de 2 px del mismo color.
- Primario: relleno `accion`, texto `en-accion`; al presionar, `accion-presionado`. Secundario al presionar: `secundario-presionado` con borde `texto`.
- Alarma a pantalla completa: campo `rojo-alarma`, texto e íconos `blanco`, botón `blanco` con texto `rojo-alarma`.
- Velo de diálogo y teclado: `sobrecapa` (80 %), un solo blend al abrir.
- Verde y rojo se distinguen por tono más que por claridad (1.3:1 entre sí); por eso cada estado lleva palabra o ícono además del color.

### Umbrales

| Señal | En rango | Advertencia | Alarma |
|---|---|---|---|
| Caída del tren (P1 − P2) | menor a 5 psi · Normal | 5 a 10 psi · Revisar | mayor a 10 psi · Cartuchos tapados |
| ORP de la cisterna | 650 mV o más · Oxidante | de −50 a 649 mV · Neutro | menor a −50 mV · Reductor |

- 650 mV es el potencial que la OMS asocia con agua potable desinfectada, según la ficha de ORP de Oregon Health Authority. De −50 a −250 mV es el rango de formación de sulfuro según YSI. Son valores de arranque: ajústalos con tus tiras y tus lecturas.

## Tipografía

- Una sola familia: Montserrat Tnum, Medium 500 y SemiBold 600, en `fonts/`. Es Montserrat (licencia OFL, en `licencias/`) con la función `tnum` congelada: los diez dígitos miden 0.70 em y el número no baila al cambiar.
- No uses las Montserrat que trae LVGL: tienen dígitos proporcionales (en la de 48 px el 1 avanza 17.75 px y el 0 avanza 32 px) y solo cubren ASCII, ° y •, sin acentos ni ñ.
- Cifras en `cifra-hero`, `cifra-xl`, `cifra-lg` y `cifra-md` (fuentes de solo dígitos). Todo lo demás en `titulo-xl`, `titulo`, `texto` y `etiqueta`.
- Diseña siempre en Montserrat Tnum. Montserrat es más ancha que Helvetica o Inter; un layout medido con otra fuente se desborda en el firmware.
- Formato numérico de México: punto decimal y coma de miles (1,860 · 12.4). Caudal y mg/L con un decimal; psi, mV, L y mL enteros; el cero de presión con un decimal. Negativos con el signo − (U+2212). Para que el ancho no salte, rellena a la izquierda con espacio de cifra U+2007: " 9.8" ocupa lo mismo que "12.4".

## Voz

- Español de México, frase normal, sin mayúsculas sostenidas, sin signos de exclamación y sin emojis.
- Verbos directos y las palabras de la casa: Toca echar, Ya le puse, Registrar otra cantidad, Hacer cero, Prueba de cubeta, Sin flujo, Próximamente, Entendido.
- Una alarma dice qué pasó, qué hizo el sistema y qué revisar, en ese orden: "Con la bomba B encendida, el sensor F marcó 0 L/min durante 10 s. La bomba se apagó." y "Revisa que la válvula V1 esté abierta y que llegue agua del SIAPA."
- Un diálogo pregunta en `titulo-xl` y dice la consecuencia en `texto`: "¿Ya le pusiste 37 mL?" y "Se registra hoy a las 18:42 y el conteo de litros desde la última dosis vuelve a 0."

## Iconografía

- 29 íconos propios en el grupo de assets Iconos: rejilla de 32, trazo de 2.5 px, puntas y uniones redondas, una sola tinta. Sin emojis ni ilustraciones.
- En el firmware cada ícono es una imagen A8 (solo alfa) que toma el color de `image_recolor`: el mismo archivo sirve en `texto`, `texto-2`, `ambar` o `blanco`. Exporta a 32 px (navegación, encabezado, botones), 48 px (tecla borrar) y 96 px (alarma).
- La tinta #76818d de los SVG es solo para verlos en este visor.
- Los símbolos del P&ID (válvula, recipiente con cartucho, toma de medición, inyección, cisterna con sonda) no son íconos: se dibujan con líneas y rectángulos dentro de la pantalla Tren.

## Movimiento

- Una sola animación: los puntos de flujo. Puntos de 6 px (`punto-flujo`) a paso de 24 px (`paso-puntos`) que avanzan en el sentido del agua a 4 px/s por cada L/min, entre 12 y 90 px/s. Con F en 0 se quitan los puntos y la tubería cambia a `tuberia`.
- Sin transiciones entre pantallas ni parpadeos.

## Estados globales

- Encabezado: nombre de pantalla, pastilla de estado (OK, Advertencia, Alarma, con ícono), WiFi con 3, 2 o 1 arcos o tachado, y hora.
- Alarma a pantalla completa para tres casos: sin flujo mientras dosifica, sensor desconectado y tope diario alcanzado. Muestra título, qué pasó, qué revisar, datos clave y Entendido. Al reconocerla se cierra; la pastilla sigue en Alarma hasta que la condición se quite y el evento queda en Historial.
- Confirmación antes de lo que reinicia algo: Ya le puse (los litros desde la última dosis vuelven a 0), Hacer cero, registrar cambio de cartucho.
- Teclado numérico para capturar mL, relación, tope y factor del sensor.
- Sin red: chip ámbar en el encabezado con lo que falta por enviar. Nunca un diálogo ni un bloqueo.

## Pantallas

| Pantalla | Qué muestra | Acción principal |
|---|---|---|
| 1 Tren | P&ID del tren con P1, P2, F, ORP, A, B y caída del tren; franja con litros de hoy, desde la última dosis y mL que tocan | Toca echar lleva a Dosis |
| 2 Flujo | Caudal a 232 px, litros de hoy y desde la última dosis, 2 h de caudal | Ninguna, es de lectura |
| 3 Cisterna | ORP con escala de tres zonas, 7 días, cloro libre estimado y calibración (fase 2) | Calibrar con tira |
| 4 Dosis | mL de Cloralex que tocan y su base, últimas 5 dosis, bombas de fase 3 deshabilitadas | Ya le puse |
| 5 Historial | Litros y dosis, presiones u ORP por día, semana o mes; eventos | Cambiar rango o señal |
| 6 Ajustes | Sensores, dosis, conexión, cartuchos y pantalla en lista con detalle | Prueba de cubeta, Hacer cero |

Datos de ejemplo, iguales en todas las vistas: 18:42 · caudal 12.4 L/min · 1,860 L hoy · 1,850 L desde la dosis de las 06:10 · P1 42 psi · P2 38 psi · caída 4 psi · ORP 650 mV · relación 0.020 mL/L · dosis 37 mL (1,850 x 0.020) · tope 120 mL al día · YF-B6 con 396 pulsos por litro.

## Fuentes de los números

- ANSI/HFES 100-2007, altura de carácter en minutos de arco: https://www.xybix.com/hubfs/ANSI_HFES_100-200727E2.pdf
- ORP de 650 mV para desinfección (OMS citada por Oregon Health Authority): https://www.oregon.gov/oha/PH/HEALTHYENVIRONMENTS/RECREATION/POOLSLODGING/Documents/orphandout.pdf
- Formación de sulfuro de −50 a −250 mV (YSI): https://www.ysi.com/ysi-blog/water-blogged-blog/2013/08/orp-management-in-wastewater-as-an-indicator-of-process-efficiency
- YF-B6, F = 6.6 x Q y 396 pulsos por litro: https://download.mikroe.com/documents/datasheets/YF-B6_Flow_Sensor_datasheet.pdf
- Waveshare ESP32-S3-Touch-LCD-5B (resolución, GT911, 16 MB de flash, 8 MB de PSRAM): https://docs.waveshare.com/ESP32-S3-Touch-LCD-5
