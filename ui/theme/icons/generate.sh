#!/usr/bin/env bash
# Regenera los íconos A8 de LVGL desde los SVG de svg/.
# Requiere: pip install cairosvg pypng lz4
# Uso: cd ui/theme/icons && ./generate.sh
#
# A8 = solo alfa. El color se pone en código con image_recolor,
# así el mismo ícono sirve en texto, texto-2, ámbar o blanco.
set -euo pipefail
cd "$(dirname "$0")"

LVGLIMAGE="../../../simulator/lvgl/scripts/LVGLImage.py"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# nombre del svg y tamaño en px
ICONS=(
  "tren 32" "resumen 32" "flujo 32" "cisterna 32" "dosis 32" "historial 32" "ajustes 32"
  "ok 32" "advertencia 32" "alarma 32" "wifi 32" "wifi_off 32"
  "ok 28" "advertencia 28" "alarma 28"
  "chevron 28" "chevron 32" "check 32" "candado 32" "candado 28" "candado 24"
  "editar 24" "borrar 48" "calendario 32" "reloj 32" "bomba 28" "info 28" "atras 32" "gota 32" "cartucho 32" "medidor 32" "sol 32" "cubeta 32" "editar 32"
  "impulsor 56" "impulsor_chico 26"
)

for entry in "${ICONS[@]}"; do
  read -r name size <<< "$entry"
  out="icon_${name}_${size}"
  python3 -c "import cairosvg, sys; cairosvg.svg2png(url=sys.argv[1], write_to=sys.argv[2], output_width=int(sys.argv[3]), output_height=int(sys.argv[3]))" \
    "svg/${name}.svg" "$TMP/${out}.png" "$size"
  python3 "$LVGLIMAGE" --ofmt C --cf A8 --name "$out" -o . "$TMP/${out}.png" > /dev/null
done
