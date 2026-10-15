#pragma once
// Datos de ejemplo para el simulador: los mismos del DS
// (18:42, 12.4 L/min, 1,860 L hoy, 37 mL, P1 42, P2 38, bomba R en Auto).

#include <cstdint>

#include "system_state.h"

namespace demo {

const core::SystemState& state();

// Avanza un segundo: el caudal se mueve un poco, suben los litros,
// y cada minuto avanzan la hora y la cuenta de la bomba R.
void tick_1s();

// Lo que haría el firmware al registrar una dosis: la guarda y el conteo
// de litros desde la última dosis vuelve a 0
void on_dose_confirmed(uint32_t ml);  // "Sí, ya le puse"
void on_dose_typed(uint32_t ml);      // "Registrar otra cantidad"

// La tira de "Calibrar con tira": se guarda con el ORP de ahorita
void on_strip_checked(uint32_t tenths_mgl);

// Bomba R: modo elegido en Mezcla y los minutos del programa Auto
void on_mixer_mode(core::MixerMode mode);
void on_mixer_after_dose(uint32_t minutes);
void on_mixer_every_4h(uint32_t minutes);

// Ajustes
void on_flow_factor(uint32_t pulses_per_liter);
void on_pressure_zeroed();
void on_dose_ratio(uint32_t thousandths);
void on_daily_cap(uint32_t ml);
void on_night_off(bool on);
void on_idle_minutes(uint32_t minutes);

// Cartuchos: purgó el prefiltro o cambió (o puso) un cartucho
void on_filter_serviced(core::Filter filter);

}  // namespace demo
