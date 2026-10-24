#!/usr/bin/env bash
# Compila y le manda el firmware a la 7B por WiFi: sirve solo el hmi.bin en
# la URL de menuconfig > Casa y se sale solo cuando la placa ya lo bajó (o ya
# tenía esa versión). Desde CLion: Shell Script, como flashear.sh.
#
# No sirve la carpeta build entera: ahí también está wifi.bin, la partición
# con el nombre y la contraseña de la red en texto plano.
set -euo pipefail

# ESP-IDF instalado en ~/esp/esp-idf (firmware/hmi/LEEME.md)
IDF_DIR="${IDF_PATH:-$HOME/esp/esp-idf}"
# shellcheck disable=SC1091
. "$IDF_DIR/export.sh" > /dev/null

cd "$(dirname "$0")"
idf.py build

# Sin URL de OTA, la placa que instale este .bin ya no busca versiones nuevas
# y la siguiente tendría que ir por cable
url="$(sed -n 's/^CONFIG_HMI_OTA_URL="\(.*\)"$/\1/p' sdkconfig)"
if [ -z "$url" ]; then
    echo "Este .bin no trae URL de OTA (menuconfig > Casa > Red y servidor):" >&2
    echo "la placa ya no se podría actualizar por WiFi. No lo mando." >&2
    exit 1
fi

version="$(python3 -c 'import json; print(json.load(open("build/project_description.json"))["project_version"])' 2>/dev/null || true)"
echo
echo "Versión ${version:-?}"
python3 servir_ota.py build/hmi.bin "$url"
