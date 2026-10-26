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

Ese `rm -f sdkconfig` borra también lo de `menuconfig` > Casa (la bomba R y
las URLs): el de la casa regresa con `git checkout sdkconfig`. Del monitor se
sale con `Ctrl+]`. Los mensajes dicen a cuántos ms quedaron armadas las
pantallas y a cuántos se prendió la pantalla.

Para regresar a la consola por el USB nativo sin perder lo de Casa:
`idf.py menuconfig` > Component config > ESP-STDIO > Channel for console
output > USB Serial/JTAG Controller.

## Desde CLion

Run > Edit Configurations > + > Shell Script, y en Script path
`firmware/hmi/flashear.sh`. Con el switch en UART1, después de un `git pull`
nomás es darle Run: compila y flashea. Busca solo el puerto del chip
USB-serial de la placa; si no lo encuentra, se le pasa como argumento.

## Actualizar por WiFi

La compu le manda el `.bin` a la placa (como ESPHome): `./subir-ota.sh`
desde la laptop o la Mac, con la placa en la red de la casa. La placa lo
recibe, revisa que sea de este proyecto y que venga firmado con la llave de
la casa, contesta al momento y se reinicia sola (si la bomba R está
corriendo, cuando acabe). No hay server que dejar prendido.

```fish
git pull; ./firmware/hmi/subir-ota.sh
```

- La IP de la placa: apártala en el DHCP del módem para su MAC
  (`14:c1:9f:5d:5d:10`); el script trae la 192.168.1.112. Si es otra, va de
  argumento: `./subir-ota.sh 192.168.1.120`. Diagnóstico la dice en el
  renglón del WiFi. Los Deco van en modo AP: las IPs las da el módem.
- Lo que contesta: "Instalada ...", "Ya tiene ...: nada que instalar", "La
  firma no es de la llave de la casa" o "... ya falló una vez". Si la bomba R
  está corriendo no espera: dice que se revise luego con `/estado`. Si justo está
  bajando de la URL, espera hasta 20 s a que acabe. Antes de
  compilar pregunta qué versión trae (`/version`); si no contesta, ni compila.
- La placa compara el hash del binario, no la versión. Un `.bin` que ya falló
  una vez no se vuelve a instalar. Después de un `git pull` o un commit,
  `idf.py build` vuelve a configurar CMake solo, para que la versión sea la del
  commit nuevo.
- En otra compu (la Mac): ESP-IDF v6.1 (arriba) y la llave, copiada de la
  laptop: `secretos/firma_ota.pem` (con otra llave la placa no acepta nada).
  `sdkconfig` sí va en git (26 oct 2026): lo de `menuconfig` (bomba R, URLs,
  consola) se cambia en una compu, se hace commit y la otra lo baja con
  `git pull`. No trae nada secreto. Mientras no se haga commit, la versión
  sale con "+". `wifi.csv` no: la red ya está grabada en la placa y la OTA no
  la toca; solo hace falta para la primera carga por cable.

La versión nueva arranca a prueba: si en 5 min no se conecta a la red, regresa
sola a la anterior. Si se conecta, a los 30 s queda como buena. Ya probado en
la 7B (23 oct 2026, con la otra forma).

Después de "Instalada" el script no se sale: espera a que la placa se
reinicie con la nueva (hasta 3 min), a que pase la prueba (30 s) y luego
imprime cómo quedó, para revisarla desde arriba sin ir a verla:

```text
✓ Versión 1a2b3c4+: quedó como buena
✓ Encendida hace 31 s
✓ Táctil GT911
✓ Expansor CH32V003
✓ WiFi 192.168.1.112 · -63 dBm
✓ Hora del NTP 2026-10-24 23:41
✓ Memoria: leyó 52 datos guardados
✓ ADS1115 · P1 0.441 V, P2 0.438 V, nivel 0.591 V, LDR 0.880 V
· Detectados: presión sí, nivel sí, LDR sí
✓ Filtros: R 3.9, 101 % de la referencia, Normal
· Bomba R: no está en menuconfig
· Servidor: sin configurar
```

✗ es para revisar (el script sale con error); · solo dice. Si la nueva no
arranca y el bootloader regresa a la anterior, lo dice. La placa, mientras,
abre Diagnóstico sola y el pie dice "a prueba: buena en 25 s" en ámbar y luego
"quedó como buena" en verde (10 min). Lo mismo cuando sea:
`curl http://192.168.1.112/estado`.

Si se va la luz a media actualización no pasa nada: la nueva se escribe en la
otra partición y la placa solo cambia de partición cuando llegó completa y con
la firma buena. Hasta ese momento sigue arrancando la de antes; se vuelve a
mandar y ya.

### La otra forma: que la placa venga por él

Si `menuconfig` > Casa > Red y servidor tiene una URL, la placa también revisa
ahí cada minuto (para el NAS, o una placa que todavía no tiene `/ota`). Con la
URL de la laptop, `./servir-ota.sh` compila, sirve solo `hmi.bin` hasta que la
placa lo baja y se sale. No sirve la carpeta `build` (ahí está `wifi.bin`, con
la contraseña de la red en texto plano), ni un `.bin` sin URL, ni si la IP de
la URL ya no es de esta compu. Revisar no gasta la flash: ESP-IDF solo borra
la partición cuando de verdad empieza a bajar. Sin nadie sirviendo, el monitor
dice una vez "Nadie sirve ..." y sigue callado.

## Conectar los sensores

Con la placa apagada al conectar (el ADS1115 y el INA219 se buscan al
arrancar). Los del ADS1115 se detectan solos (`core/detect.h`): a los 5 s de
dar una lectura de sensor conectado salen en las pantallas y se quedan
guardados, aunque luego se suelte un cable. Mientras, Diagnóstico enseña en
gris lo que lee cada entrada. La bomba R no se detecta: va en `menuconfig`.

| Qué | Aparece cuando | En Diagnóstico | Luego |
|---|---|---|---|
| F (GPIO6) | siempre | Hz con agua corriendo | Prueba de cubeta |
| P1 y P2 (A0, A1) | los dos pasan de 0.3 V | ~0.44 V cada uno sin presión | Hacer cero con V1 cerrada |
| Nivel (A2) | pasa de 0.10 V | ~0.600 V en el aire (4 mA; medido 0.591) | Calibrar nivel |
| LDR (A3) | pasa de 0.15 V (con luz) | cambia al taparlo | |
| Bomba R (GPIO44, INA219) | `menuconfig` > Casa > Bomba R | la corriente al prenderla | umbrales de la prueba 9b |

- Ya detectado, si un sensor deja de leer (cable suelto, o el ADS1115 no
  contesta) 5 s seguidos, sale "Sin señal" en ámbar donde iba su número, la
  pastilla en Advertencia y el evento en Alarmas ("P1 sin señal · revisa su
  cable"). Mientras, no da avisos falsos: P1 suelto lee 0 psi, igual que si
  se fuera el agua, y no sale "Sin agua"; el nivel suelto no apaga la bomba
  R. Con 5 s de lectura buena regresa solo. `/estado` lo lista con ✗.
- Antes de conectar el ADS1115 a la 7B: un pull-down de 100k de P1 y de P2
  a tierra. Sin él, P1 y P2 sueltos flotan entre 0.4 y 0.6 V, arriba de los
  0.3 V: la placa los daría por instalados a los 5 s y lo guarda (desde la
  pantalla no se puede olvidar un sensor). Va del lado del sensor, antes de
  R2 y R3: del lado del A0, con la de 10k hace un divisor y lee 9 % menos
  (100k / 110k), y eso Hacer cero no lo quita.
- P1 y P2 llegan directo al A0 y A1: R2 y R3 de 10k con C2 y C3 de 100 nF a
  tierra son un filtro, no un divisor (medido el 23 oct 2026). Sin presión
  marcan ~0.44 V (el de fábrica dice 0.5: Hacer cero lo corrige). A la
  presión de la casa (35 a 65 psi) dan de 1.1 a 1.6 V; arriba de ~160 psi
  pasarían los 3.3 V del ADS1115 y la lectura se queda topada, con la de 10k
  limitando la corriente.
- El nivel cierra por R8, de 150 Ω 1 % (el documento decía 33 Ω): 4 mA son
  0.60 V y 20 mA 3.00 V, y entra al A2 por R9 de 10k con C6 de 100 nF. Sin
  lazo, R8 lo deja en 0 V. Un cero guardado por un firmware de antes (0.132
  V, de 33 Ω) no se cree: regresa a 0.600 hasta que hagas cero en el aire.
- La bomba R al final: su pin es el RX de UART0, así que antes se sale del
  modo mesa (la consola al USB nativo en `menuconfig`, ver "En la mesa, sin
  bomba R") y el switch va en UART2.

## Para afinar con la placa en la mano

- `HMI_LCD_PCLK_MHZ`: 30 (~33 refrescos por segundo) se probó en la 7B sin
  rayas; el demo de Waveshare usa 24 y parpadea. Si la imagen se recorre,
  bajarlo. La PSRAM va a 80 MHz; el demo usa 120, que en IDF es experimental.
- El brillo con el LDR (`adjust_brightness` en `main/app.cpp`) y las rayitas
  de señal del WiFi son a ojo. El brillo va a todo desde 0.4 V del LDR y de
  noche no baja de 20 % (26 oct: de día se quedaba bajo; antes llegaba a
  todo hasta 0.9 V). Si de día sigue bajo, `/estado` dice cuánto marca el
  LDR (A3) y se baja `kLdrBrightVolts`.
- Los cartuchos (`core/clog.h`): con cartuchos nuevos y el agua corriendo
  pareja un minuto, Ajustes > Cartuchos > Guardar referencia. Sin eso dice
  "Sin referencia" y no avisa nada.
- F en reposo: con la interrupción de la ESPDuino, sin filtro, salía a veces
  un pulso suelto con el agua parada. El PCNT ya filtra lo de menos de 10 µs
  (el máximo del S3 es ~12.8 µs). Para ver si con eso basta, con el agua
  parada: `curl http://192.168.1.112/estado` dos veces con unos minutos de
  diferencia; si "F: N pulsos" sube, sí se cuelan y hay que filtrarlos en
  el código.
- Los umbrales de la bomba R (sin 12 V, nivel bajo, atorada) salen de la
  prueba 9b; los de `menuconfig` son para arrancar.
