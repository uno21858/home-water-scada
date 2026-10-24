#!/usr/bin/env python3
"""Sirve hmi.bin a la 7B una vez y se sale solo (lo corre servir-ota.sh).

La placa revisa la URL de menuconfig > Casa cada minuto: lee la cabecera del
.bin y, si es otra versión, baja el resto. Aquí se espera a que lo baje
completo y se sale. Si la placa ya corre esa versión, cuelga luego luego:
también se sale, porque no hay nada que mandar.

Uso: servir_ota.py build/hmi.bin http://192.168.1.69:8070/hmi.bin
"""

import http.server
import os
import socket
import sys
import time
import urllib.parse

WAIT_MINUTES = 20   # si la placa no viene en este tiempo, algo anda mal
SAME_SECONDS = 3    # cortar antes de esto es leer la cabecera y dejarlo


def say(text):
    print(time.strftime("%H:%M:%S"), text, flush=True)


def is_mine(host):
    """La IP de la URL es de esta compu: se puede abrir un socket en ella."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
            probe.bind((host, 0))
        return True
    except OSError:
        return False


def main():
    path, url = sys.argv[1], sys.argv[2]
    parts = urllib.parse.urlsplit(url)
    host, port, wanted = parts.hostname, parts.port or 80, parts.path
    if not is_mine(host):
        print(f"La placa busca el .bin en {host} y esa IP no es de esta compu.", file=sys.stderr)
        print("¿Cambió la IP de la laptop? Apártala en el módem o cambia la URL en menuconfig.", file=sys.stderr)
        return 1
    size = os.path.getsize(path)
    result = {"done": False}

    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path != wanted:
                self.send_error(404)  # solo el .bin: nada más de la carpeta build
                return
            board = self.client_address[0]
            start = time.monotonic()
            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Length", str(size))
            self.end_headers()
            try:
                with open(path, "rb") as f:
                    while chunk := f.read(64 * 1024):
                        self.wfile.write(chunk)
                # Ya se mandó todo: la placa cierra limpio (FIN) al terminar de
                # leer; si cuelga a medias llega un RST
                self.connection.shutdown(socket.SHUT_WR)
                self.connection.settimeout(60)
                self.connection.recv(1)
            except (ConnectionResetError, BrokenPipeError, socket.timeout):
                if time.monotonic() - start < SAME_SECONDS:
                    say(f"{board} ya tiene esta versión: no hay nada que instalar.")
                    result["done"] = True
                else:
                    say(f"{board} cortó la descarga; lo vuelve a intentar en su siguiente revisión.")
                return
            say(f"{board} lo bajó completo. Si la firma cuadra, se reinicia sola en cuanto pueda.")
            result["done"] = True

        def log_message(self, *args):
            pass  # sin el renglón por cada petición

    with http.server.HTTPServer(("", port), Handler) as server:
        server.timeout = 1
        say(f"Esperando a que la placa venga por {url} (revisa cada minuto). Ctrl+C para parar.")
        deadline = time.monotonic() + WAIT_MINUTES * 60
        while not result["done"]:
            if time.monotonic() > deadline:
                say(f"En {WAIT_MINUTES} min la placa no vino: ¿está prendida y con red?")
                return 1
            server.handle_request()
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print()
        sys.exit(130)
