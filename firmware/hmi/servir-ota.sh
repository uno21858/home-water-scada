#!/usr/bin/env bash
# Compila y sirve solo el hmi.bin para que la 7B lo baje por WiFi (la URL de
# menuconfig > Casa apunta a esta compu, puerto 8070). Se para con Ctrl+C.
#
# No sirve la carpeta build entera: ahí también está wifi.bin, la partición
# con el nombre y la contraseña de la red en texto plano.
set -euo pipefail

PORT=8070

# ESP-IDF instalado en ~/esp/esp-idf (firmware/hmi/LEEME.md)
IDF_DIR="${IDF_PATH:-$HOME/esp/esp-idf}"
# shellcheck disable=SC1091
. "$IDF_DIR/export.sh" > /dev/null

cd "$(dirname "$0")"
idf.py build

# Sin URL de OTA, la placa que instale este .bin ya no busca versiones nuevas
# y la siguiente tendría que ir por cable (pasa si se borra sdkconfig)
if ! grep -qE '^CONFIG_HMI_OTA_URL=".+"' sdkconfig; then
    echo "Este .bin no trae URL de OTA (menuconfig > Casa > Red y servidor):" >&2
    echo "la placa ya no se podría actualizar por WiFi. No lo sirvo." >&2
    exit 1
fi

# Una carpeta aparte con solo el .bin; se borra al salir
served="$(mktemp -d)"
trap 'rm -rf "$served"' EXIT
cp build/hmi.bin "$served/"

version="$(python3 -c 'import json; print(json.load(open("build/project_description.json"))["project_version"])' 2>/dev/null || true)"
echo
echo "Sirviendo hmi.bin ${version:+($version) }en el puerto $PORT. La placa lo baja en su siguiente revisión."
echo "Si ya corre esa versión, la placa cuelga después de la cabecera y aquí sale un"
echo "ConnectionResetError: no es error. Ctrl+C para parar."
python3 -m http.server "$PORT" --directory "$served"
