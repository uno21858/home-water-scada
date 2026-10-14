# Diseño de la HMI

Copia del canvas "HMI Cloración · Pantallas" y del design system "HMI Cloración"
(los dos viven en claude.ai y son privados), para leerlos sin abrir el canvas.
Es la especificación: cada pantalla del código sale de aquí.

## Qué hay

- `pantallas/*.dc.html`: un archivo por pantalla del canvas. No abren solos en el
  navegador (usan el runtime del canvas); se leen como especificación: posiciones,
  tamaños, colores y textos.
- `canvas.json`: la lista de pantallas con su título y su fila en el canvas
  (vista general, pantallas de la barra, detalle, servicio, estados y componentes).
- `ds/README.md`: reglas del design system: color, tipografía, umbrales, voz y
  datos de ejemplo.
- `ds/lvgl.md`: cómo pasar cada pieza a LVGL 9.6.
- `ds/tokens.json`: todos los tokens.

## Cómo leer una pantalla

- Todo es posición absoluta en px sobre 1024 x 600, igual que en LVGL.
- El encabezado (`Encabezado.dc.html`) va de y 0 a 64 y la barra
  (`Navegacion.dc.html`) de y 520 a 600. En el código el contenido empieza en
  y 64: cada y de la pantalla menos 64.
- Dentro de un recuadro con borde, las posiciones cuentan desde adentro del borde.
  En LVGL es igual.
- Cada color en hex es un token de `ui/theme/colors.h`.
- Los `data-props` del final (flujo, caida, fase, bombaR...) son los estados que
  hay que soportar; `renderVals()` dice qué cambia en cada uno.
- Tamaños de letra: 26, 32, 36 y 48 para texto; 44, 72, 136 y 232 para cifras.
  Son las fuentes de `ui/theme/fonts.h`.

## Ojo

- El DS se escribió para la pantalla de 5 in (ESP32-S3-Touch-LCD-5B). Ya se cambió
  a la 7B: mismos 1024 x 600, pero el pixel es más grande.
- Las diferencias a propósito entre el código y el canvas están en `CLAUDE.md`.
