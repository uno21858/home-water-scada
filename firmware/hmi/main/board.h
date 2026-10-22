#pragma once
// La Waveshare ESP32-S3-Touch-LCD-7B: pantalla RGB de 1024 x 600, táctil
// GT911 y el expansor CH32V003 (reset del táctil, backlight y USB o CAN).
// Pines y tiempos del demo de Waveshare para la 7B
// (github.com/waveshareteam/ESP32-S3-Touch-LCD-7B, examples/ESP-IDF/17_lvgl_v9_demo).

#include <cstdint>

#include "driver/i2c_master.h"
#include "lvgl.h"

namespace board {

// El bus I2C de la placa (GPIO8 y GPIO9). Ahí viven también el ADS1115 y el
// INA219 de la caja (documento, pág. 31).
void start_i2c();
i2c_master_bus_handle_t i2c();

// ¿Contesta alguien en esa dirección?
bool probe(uint8_t address);

// Pantalla y táctil, registrados en LVGL. Se llama desde la tarea de LVGL,
// después de lv_init(): esa tarea espera ahí el cambio de framebuffer.
lv_display_t* start_display();

// Lo que respondió al arrancar, para Diagnóstico
bool touch_ok();
bool expander_ok();

// Backlight: prendido o apagado, y el brillo de 0 a 100
void backlight(bool on);
void set_brightness(uint8_t percent);

}  // namespace board
