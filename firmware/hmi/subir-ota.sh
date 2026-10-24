#!/usr/bin/env bash
# Compila y le manda el firmware a la 7B por WiFi, desde cualquier compu de la
# casa que tenga ESP-IDF y la llave de las OTA (secretos/firma_ota.pem). La
# placa lo recibe, revisa la firma y contesta al momento; si quedó, se
# reinicia sola (si la bomba R está corriendo, cuando acabe).
# Desde CLion: Shell Script, como flashear.sh.
#
# Uso: ./subir-ota.sh                 la placa en su IP de siempre (abajo)
#      ./subir-ota.sh 192.168.1.110   otra IP (Diagnóstico la dice, en el WiFi)
set -euo pipefail

# La IP que le apartaste a la placa en el módem (MAC 14:c1:9f:5d:5d:10)
HMI="${1:-${HMI:-192.168.1.120}}"

# ESP-IDF instalado en ~/esp/esp-idf (firmware/hmi/LEEME.md)
IDF_DIR="${IDF_PATH:-$HOME/esp/esp-idf}"
# shellcheck disable=SC1091
. "$IDF_DIR/export.sh" > /dev/null

cd "$(dirname "$0")"

# Primero ver que esté, antes de esperar a que compile
running="$(curl -s --max-time 5 "http://$HMI/version" || true)"
if [ -z "$running" ]; then
    echo "La placa no contesta en $HMI. ¿Está prendida y con red? Su IP sale en" >&2
    echo "Ajustes > Diagnóstico (renglón del WiFi): ./subir-ota.sh ESA-IP" >&2
    exit 1
fi

idf.py build
version="$(python3 -c 'import json; print(json.load(open("build/project_description.json"))["project_version"])' 2>/dev/null || true)"
echo
echo "La placa trae $running; aquí está ${version:-?}. Mandando..."

# La respuesta de la placa y, en el último renglón, el código HTTP
answer="$(curl -sS --max-time 180 -H 'Content-Type: application/octet-stream' \
    --data-binary @build/hmi.bin -w '\n%{http_code}' "http://$HMI/ota")"
code="${answer##*$'\n'}"
echo "${answer%$'\n'*}"
[ "$code" -lt 400 ]
