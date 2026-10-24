# Firmware de la 7B

ESP-IDF v6.1 para la Waveshare ESP32-S3-Touch-LCD-7B. Usa los mismos `core/` y
`ui/` que el simulador y el LVGL de `simulator/lvgl`.

Qué hace (detalle en `main/app.h`):

- Pantalla RGB 1024 x 600 y táctil GT911 con los pines y tiempos del demo de
  Waveshare (`main/board.cpp`).
- F por el PCNT del GPIO6, ADS1115 (P1, P2, nivel y LDR) e INA219 de la bomba R,
  que sale por el GPIO44 (`main/sensors.cpp`). El GPIO44 se pone en bajo antes
  que nada.
- Guarda en la NVS los ajustes al tocarlos y los litros cuando para el agua o
  cada 10 min (`core/persist.h`). Después de un apagón o una OTA sigue donde iba.
- WiFi, hora del NTP (Guadalajara, UTC−6) y cada 10 s los datos a
  VictoriaMetrics; sin red se quedan en la cola ("Por enviar").
- OTA desde la LAN: busca el `.bin` cada 10 min y solo instala uno firmado con
  la llave de la casa. Si la versión nueva no se conecta a la red en 5 min,
  regresa sola a la anterior.

Al principio todo lo de la caja está apagado en `menuconfig`: la 7B sola en la
mesa. Cada cosa se prende conforme se conecta.

## Una sola vez en la compu

1. ESP-IDF v6.1. En Mac primero `brew install cmake ninja dfu-util python3`; en
   Fedora, lo de la guía de Espressif para Linux. Luego:

   ```bash
   git clone -b v6.1 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf
   cd ~/esp/esp-idf && ./install.sh esp32s3
   ```

   En cada terminal nueva: `. ~/esp/esp-idf/export.sh` (zsh) o
   `. ~/esp/esp-idf/export.fish` (fish).

2. La llave que firma las OTA, dentro de `firmware/hmi`:

   ```bash
   idf.py secure-generate-signing-key --scheme rsa3072 secretos/firma_ota.pem
   ```

   No va a git. Respáldala en el NAS o en tu gestor de contraseñas y cópiala a
   la otra compu: la placa solo acepta OTA firmadas con la misma llave que el
   firmware que ya trae. Si se pierde, se genera otra y la primera carga con la
   nueva va por cable.

3. La red: `cp wifi.csv.ejemplo wifi.csv` y pon el nombre y la contraseña.
   Tampoco va a git, y no viaja en el `.bin` de la OTA: va en su propia
   partición y se graba con `idf.py flash`.

4. `idf.py menuconfig` > **Casa (home-water-scada)**: qué está instalado, la
   URL de VictoriaMetrics y la del firmware para la OTA.

## Primera carga, por cable

Documento, pág. 41: el switch UART en **UART1** y el cable al USB-C marcado
UART1 (mientras, la bomba R queda desconectada de su pin).

```bash
idf.py build
idf.py -p PUERTO flash
```

Al terminar, el switch de regreso a **UART2**. Los mensajes de la placa salen
por el USB nativo, no por UART0 (su RX es la bomba R): `idf.py -p PUERTO monitor`
con el cable en el otro USB-C.

## En la mesa, sin bomba R

Con `sdkconfig.mesa` la consola sale por UART0. Con el switch en UART1, el
mismo USB-C sirve para flashear y para ver los mensajes. Sin bomba R el
firmware no toca el GPIO44, así que se puede quedar en UART1. Con la bomba R
prendida en `menuconfig` así ya no compila: su pin es el RX de UART0.

```fish
rm -f sdkconfig
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.mesa" build
idf.py -p /dev/ttyACM0 flash monitor
```

Ese `rm -f sdkconfig` borra también lo de `menuconfig` > Casa (la URL de la
OTA): solo para la primera vez. Del monitor se sale con `Ctrl+]`. Los
mensajes dicen a cuántos ms quedaron armadas las pantallas y a cuántos se
prendió la pantalla.

Para regresar a la consola por el USB nativo sin perder lo de Casa:
`idf.py menuconfig` > Component config > ESP-STDIO > Channel for console
output > USB Serial/JTAG Controller. No borres `sdkconfig`: sin la URL de la
OTA, la placa que instale ese `.bin` ya no busca versiones nuevas y la
siguiente va por cable (`servir-ota.sh` no sirve un `.bin` sin URL).

## Desde CLion

Run > Edit Configurations > + > Shell Script, y en Script path
`firmware/hmi/flashear.sh`. Con el switch en UART1, después de un `git pull`
nomás es darle Run: compila y flashea. Busca solo el puerto del chip
USB-serial de la placa; si no lo encuentra, se le pasa como argumento.

## Actualizar por WiFi

La URL de la OTA va dentro del firmware, así que la primera vez se carga por
cable con la URL ya puesta. Después ya no hace falta el cable.

1. Elige la compu que va a servir el `.bin` y apártale su IP en el DHCP del
   módem, para que la URL no cambie. Los Deco van en modo AP: toda la casa,
   cable y WiFi, es una sola red 192.168.1.x y las IPs las da el módem. Para
   siempre conviene el NAS (192.168.1.68), que ya tiene la suya fija.
2. `idf.py menuconfig` > Casa > Red y servidor > URL del firmware para la
   OTA: `http://IP-de-esa-compu:8070/hmi.bin`.
3. Flashéala por cable (`./flashear.sh`) y regresa el switch a UART2.
4. Cada actualización: `git pull` y `./servir-ota.sh` (en CLion, una
   configuración Shell Script como la de `flashear.sh`). Compila, espera a
   que la placa venga por el `.bin` y se sale solo cuando lo bajó completo, o
   cuando ve que ya tenía esa versión. No hay que dejar nada prendido: la
   placa revisa cada minuto, y si no hay nadie sirviendo no pasa nada.

   Sirve solo `hmi.bin` (`servir_ota.py`), no la carpeta `build`: ahí está
   `wifi.bin`, con la contraseña de la red en texto plano. Tampoco sirve un
   `.bin` sin URL de OTA, ni si la IP de la URL ya no es de esta compu.

   La compu puede estar por cable o por WiFi. La 7B lo baja, revisa la firma y
   reinicia; si la bomba R está corriendo, espera a que acabe. Ya probado en
   la 7B (23 oct 2026).

La URL también se cambia por OTA: el firmware nuevo trae la suya. Para pasar
de la laptop al NAS, se compila con la URL del NAS y se sirve una última vez
desde la laptop.

Si compilas en otra compu (la Mac), lleva la misma llave
`secretos/firma_ota.pem`: con otra llave la placa no acepta la OTA.

La placa compara el hash del binario, no la versión: lo que esté en la URL es lo
que va a correr. Un `.bin` que ya falló una vez no se vuelve a instalar. Si es
el mismo que ya corre, la placa lo deja después de la cabecera ("es la que ya
corre" en el monitor, una vez) y no borra nada de la flash: ESP-IDF solo borra
la partición cuando de verdad empieza a bajar. Sin nadie sirviendo, el monitor
dice una vez "Nadie sirve ..." y sigue revisando callado. Después de un `git pull` o un commit, `idf.py build`
vuelve a configurar CMake solo, para que la versión sea la del commit nuevo.

## Conectar los sensores

Con la placa apagada al conectar (el ADS1115 y el INA219 se buscan al
arrancar). Los del ADS1115 se detectan solos (`core/detect.h`): a los 5 s de
dar una lectura de sensor conectado salen en las pantallas y se quedan
guardados, aunque luego se suelte un cable. Mientras, Diagnóstico enseña en
gris lo que lee cada entrada. La bomba R no se detecta: va en `menuconfig`.

| Qué | Aparece cuando | En Diagnóstico | Luego |
|---|---|---|---|
| F (GPIO6) | siempre | Hz con agua corriendo | Prueba de cubeta |
| P1 y P2 (A0, A1) | los dos pasan de 0.3 V | ~0.500 V cada uno sin presión | Hacer cero con V1 cerrada |
| Nivel (A2) | pasa de 0.10 V | 0.132 V en el aire (4 mA) | Calibrar nivel |
| LDR (A3) | pasa de 0.15 V (con luz) | cambia al taparlo | |
| Bomba R (GPIO44, INA219) | `menuconfig` > Casa > Bomba R | la corriente al prenderla | umbrales de la prueba 9b |

- P1 y P2 entran por su divisor 10k/10k. Sin él, los 4.5 V del sensor pasan
  el máximo del ADS1115 a 3.3 V (VDD + 0.3 V, hoja de datos de TI).
- La bomba R al final: su pin es el RX de UART0, así que antes se sale del
  modo mesa (la consola al USB nativo en `menuconfig`, ver "En la mesa, sin
  bomba R") y el switch va en UART2.

## Para afinar con la placa en la mano

- `HMI_LCD_PCLK_MHZ`: 30 (~33 refrescos por segundo) se probó en la 7B sin
  rayas; el demo de Waveshare usa 24 y parpadea. Si la imagen se recorre,
  bajarlo. La PSRAM va a 80 MHz; el demo usa 120, que en IDF es experimental.
- El brillo con el LDR (`adjust_brightness` en `main/app.cpp`) y las rayitas
  de señal del WiFi son a ojo.
- Los umbrales de la bomba R (sin 12 V, nivel bajo, atorada) salen de la
  prueba 9b; los de `menuconfig` son para arrancar.
