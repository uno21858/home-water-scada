#!/usr/bin/env bash
# Compila y flashea la 7B por el USB-C de UART1 (con el switch en UART1).
# Desde CLion: Run > Edit Configurations > + > Shell Script, con este archivo.
# En la terminal: ./flashear.sh, o ./flashear.sh /dev/ttyACM0 para elegir el puerto.
set -euo pipefail

# ESP-IDF instalado en ~/esp/esp-idf (firmware/hmi/LEEME.md)
IDF_DIR="${IDF_PATH:-$HOME/esp/esp-idf}"
# shellcheck disable=SC1091
. "$IDF_DIR/export.sh" > /dev/null

cd "$(dirname "$0")"

# El puerto: el que se pase, o el chip USB-serial de la placa (WCH, 1a86)
PORT="${1:-}"
if [ -z "$PORT" ]; then
    for candidate in /dev/serial/by-id/usb-1a86_* /dev/cu.usbmodem* /dev/cu.wchusbserial*; do
        if [ -e "$candidate" ]; then
            PORT="$candidate"
            break
        fi
    done
fi
if [ -z "$PORT" ]; then
    echo "No encuentro la placa: ¿está conectada al USB-C de UART1 con el switch en UART1?" >&2
    exit 1
fi

echo "Flasheando por $PORT"
idf.py -p "$PORT" flash
