#pragma once
// Lo que pasa cuando entra agua, se echa cloro o se cambia un cartucho.

#include <cstdint>

#include "system_state.h"

namespace core {

// Caudal de ahorita (F en la última ventana de 1 s). También es la última
// muestra de las gráficas de 2 h y 24 h, que así se mueven en vivo.
void set_flow(SystemState& s, float lpm);

// F contó estos litros: suben los del día, los de la hora, los que van desde
// la dosis, los de cada cartucho y los de la mezcla (pueden prender la bomba R)
void count_liters(SystemState& s, uint32_t liters);

// Se registró una dosis ("Ya le puse" o "Registrar otra cantidad"): la anota,
// la cuenta de litros vuelve a 0 y en Auto prende la mezcla
void register_dose(SystemState& s, uint32_t ml, bool typed);

// Presión del SEN0257: 0 a 1.6 MPa en 0.5 a 4.5 V (documento, pág. 39):
// psi = (V − V0) × 1.6 / 4 × 145, con V0 el cero de cada sensor. El cero se
// guarda en psi contra los 0.5 V nominales (p1_zero_psi): P1 dio 0.448 V al
// aire, o sea −3.0 psi.
inline constexpr float kPressureNominalZeroVolts = 0.5f;
inline constexpr float kPsiPerVolt = 1.6f / 4.0f * 145.0f;  // 58

// Lo que leyeron A0 y A1: la salida de cada sensor, sin divisor (R2 y R3 de
// 10k con 100 nF son un filtro). Pone los volts y los psi con su cero (abajo
// de 0 queda en 0).
void set_pressures(SystemState& s, float p1_volts, float p2_volts);

// Hacer cero con V1 cerrada y sin presión: lo que marcan ahorita es el 0
void pressure_zero(SystemState& s);

// "Lo cambié", "Lo puse" o "Ya purgué": el filtro vuelve a contar desde hoy
// y, si es cartucho, baja uno de los repuestos. Antes de la primera hora
// del NTP (today en 0) queda sin fecha.
void register_filter(SystemState& s, Filter filter);

}  // namespace core
