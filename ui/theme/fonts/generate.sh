#!/usr/bin/env bash
# Regenera las fuentes de LVGL desde los TTF de src/.
# Requiere Node: usa lv_font_conv 1.5.3 con npx.
# Uso: cd ui/theme/fonts && ./generate.sh
set -euo pipefail
cd "$(dirname "$0")"

# Texto: ASCII, Latin-1 (acentos, ñ, ¿, ¡, ·, °, µ) y los signos del DS
TXT="0x20-0x7E,0xA0-0xFF,0x2007,0x2009,0x2026,0x2192,0x2212,0x2248,0x2264,0x2265"
# Cifras: espacio, coma, guion, punto, dígitos, espacio de cifra y signo menos
DIG="0x20,0x2C-0x2E,0x30-0x39,0x2007,0x2212"

gen() { # tamaño peso rango nombre
  npx --yes lv_font_conv@1.5.3 --bpp 4 --no-compress --size "$1" --font "src/MontserratTnum-$2.ttf" \
    -r "$3" --format lvgl --lv-font-name "$4" -o "$4.c"
}

gen 26  Medium   "$TXT" font_label_26
gen 32  Medium   "$TXT" font_body_32
gen 36  SemiBold "$TXT" font_title_36
gen 48  SemiBold "$TXT" font_title_xl_48
gen 44  SemiBold "$DIG" font_digits_44
gen 72  SemiBold "$DIG" font_digits_72
gen 136 SemiBold "$DIG" font_digits_136
gen 232 SemiBold "$DIG" font_digits_232
