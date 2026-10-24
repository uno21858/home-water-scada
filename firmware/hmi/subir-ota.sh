#!/usr/bin/env bash
# Compila y le manda el firmware a la 7B por WiFi, desde cualquier compu de la
# casa que tenga ESP-IDF y la llave de las OTA (secretos/firma_ota.pem). La
# placa lo recibe, revisa la firma y contesta al momento; si quedó, se
# reinicia sola (si la bomba R está corriendo, cuando acabe). Luego espera a
# que regrese con la nueva y pase la prueba (30 s) e imprime cómo quedó
# (GET /estado): táctil, WiFi, hora, memoria y sensores.
# Desde CLion: Shell Script, como flashear.sh.
#
# Uso: ./subir-ota.sh                 la placa en su IP de siempre (abajo)
#      ./subir-ota.sh 192.168.1.120   otra IP (Diagnóstico la dice, en el WiFi)
set -euo pipefail

# La IP de la placa: la que le dio el módem (MAC 14:c1:9f:5d:5d:10) y que
# conviene apartarle en su DHCP para que no cambie
HMI="${1:-${HMI:-192.168.1.112}}"

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
text="${answer%$'\n'*}"
echo "$text"
[ "$code" -lt 400 ] || exit 1
case "$text" in
    *"bomba R"*)
        echo "Para ver cómo quedó, cuando reinicie: curl http://$HMI/estado"
        exit 0 ;;
    Instalada*) ;;
    *) exit 0 ;;  # ya la tenía: nada que esperar
esac

# Que se vaya y regrese con la nueva. Si la nueva se cae al arrancar, el
# bootloader regresa a la anterior: se ve que volvió la de antes.
echo
echo "Esperando a que reinicie..."
down=0
now=""
for _ in $(seq 60); do  # 3 s x 60: 3 min
    sleep 3
    now="$(curl -sf --max-time 3 "http://$HMI/version" || true)"
    if [ -z "$now" ]; then
        down=1
    elif [ "$down" = 1 ] && [ "$now" != "$running" ]; then
        break
    elif [ "$down" = 1 ]; then
        echo "Regresó con la $running: la nueva no arrancó bien. Míralo con el cable (idf.py monitor)." >&2
        exit 1
    fi
done
if [ "$down" = 0 ] || [ -z "$now" ] || [ "$now" = "$running" ]; then
    echo "No regresó en 3 min. Revisa la placa; si se quedó sin red, en 5 min vuelve sola a la $running." >&2
    exit 1
fi

# A prueba: con red, a los 30 s queda como buena
echo "Arrancó con la $now. A prueba..."
state=""
for _ in $(seq 120); do  # 3 s x 120: 6 min, más que los 5 de la prueba
    state="$(curl -sf --max-time 5 "http://$HMI/estado" || true)"
    case "${state%%$'\n'*}" in
        *"a prueba"*) ;;
        "") ;;
        *) break ;;
    esac
    sleep 3
done
echo
echo "$state"
echo
case "$state" in
    *"a prueba"*|"")
        echo "Sigue a prueba después de 6 min: revisa la red de la placa." >&2
        exit 1 ;;
    *"✗"*)
        echo "Quedó, pero revisa lo marcado con ✗." >&2
        exit 1 ;;
esac
echo "Todo bien."
