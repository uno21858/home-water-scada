#!/usr/bin/env bash
# Regenera los símbolos del P&ID (válvulas, medidor, embudo...) desde svg/.
# Requiere: pip install cairosvg pypng lz4
# Uso: cd ui/theme/pid && ./generate.sh
#
# A diferencia de los íconos, estos llevan dos colores (relleno y trazo) que
# nunca cambian, así que van en ARGB8888 con los colores del DS ya puestos.
set -euo pipefail
cd "$(dirname "$0")"

LVGLIMAGE="../../../simulator/lvgl/scripts/LVGLImage.py"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

for svg in svg/*.svg; do
  name="pid_$(basename "$svg" .svg)"
  python3 -c "import cairosvg, sys; cairosvg.svg2png(url=sys.argv[1], write_to=sys.argv[2])" "$svg" "$TMP/$name.png"
  python3 "$LVGLIMAGE" --ofmt C --cf ARGB8888 --name "$name" -o . "$TMP/$name.png" > /dev/null
done
