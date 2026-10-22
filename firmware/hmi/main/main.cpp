// Firmware de la 7B. Ver app.h para cómo se reparte el trabajo.

#include "app.h"
#include "sensors.h"

extern "C" void app_main() {
    sensors::pump_off_now();  // documento, pág. 39: el GPIO44 en bajo antes que nada
    app::start();
}
