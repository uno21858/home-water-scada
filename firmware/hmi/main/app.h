#pragma once
// La app de la 7B: hace lo mismo que simulator/main.cpp y demo.cpp, pero con
// la placa. Pasa los sensores a core y core a la interfaz, atiende lo que
// toca la persona, guarda en la NVS, manda los datos al NAS y deja que la OTA
// reinicie cuando no estorba.
//
// Tareas:
//   lvgl     pantalla, táctil y un timer de 1 s con la lógica (núcleo 1)
//   sensors  el I2C del ADS1115 y del INA219 (sensors.cpp)
//   upload   cada 10 s la lectura a la cola y, con red, el POST
//   ota      revisa si hay firmware nuevo (ota.cpp)
// El estado (core::SystemState) lo comparten: solo se toca con el lock.

namespace app {

// Lo llama app_main después de sensors::pump_off_now()
void start();

}  // namespace app
