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

// La memoria de la pantalla (framebuffers en PSRAM y bounce buffers en RAM
// interna), sin prenderla. Va al principio, antes del WiFi: pidiéndola hasta
// prender la pantalla, con el WiFi ya conectado, a veces ya no había RAM
// interna para los bounce buffers y tronaba en ciclo (28 oct 2026).
void reserve_display();

// El display de LVGL, todavía sin pantalla: ahí se arma la interfaz
lv_display_t* create_display();

// Prende la pantalla (ya con su memoria, de reserve_display) y le da a LVGL
// sus framebuffers. Va después de armar la
// interfaz: mientras la pantalla se refresca, le quita PSRAM a LVGL. Se llama
// desde la tarea de LVGL, que espera ahí el cambio de framebuffer.
void start_display(lv_display_t* display);

// El táctil (su reset tarda 0.4 s): después del primer cuadro, para que la
// pantalla no espere
void start_touch(lv_display_t* display);

// Lo que respondió al arrancar, para Diagnóstico
bool touch_ok();
bool expander_ok();

// Backlight: prendido o apagado, y el brillo de 0 a 100
void backlight(bool on);
void set_brightness(uint8_t percent);

}  // namespace board
