#pragma once
// Lo que se mide en la caja (documento, págs. 31 y 39): F en el GPIO6 por el
// PCNT, el ADS1115 (P1, P2, nivel y LDR) y el INA219 de la bomba R, que sale
// por el GPIO44. Una tarea propia lee el I2C; la app toma la última lectura
// cada segundo con latest() y nunca espera al bus.

#include <cstdint>

namespace sensors {

struct Readings {
    uint32_t pulses = 0;  // pulsos de F desde que arrancó (cuenta del hardware; da la vuelta)

    bool ads_ok = false;      // el ADS1115 contestó la última vuelta
    float p1_volts = 0.0f;    // salida de cada sensor: lo que leyó A0 y A1 por 2 (divisor 10k/10k)
    float p2_volts = 0.0f;
    float level_volts = 0.0f; // A2: el 4-20 mA sobre la de 33 Ω
    float ldr_volts = 0.0f;   // A3

    bool ina_ok = false;      // el INA219 contestó la última vuelta
    float mixer_volts = 0.0f;
    float mixer_amps = 0.0f;
};

// El GPIO44 en bajo. Lo primero que hace app_main (documento: antes de
// inicializar cualquier otra cosa, para que la bomba R no se mueva). Sin
// bomba R en menuconfig no toca el pin.
void pump_off_now();

// PCNT, ADS1115, INA219 y su tarea. Necesita el bus de board::start_i2c().
void start();

// La lectura más nueva, promediada (16 muestras por canal del ADS1115)
Readings latest();

// Prende o apaga la bomba R
void set_pump(bool on);

}  // namespace sensors
