#pragma once
// Nivel de la cisterna con un sensor hidrostático 4-20 mA en el A2 del
// ADS1115: el lazo cierra por R8, de 150 Ω 1 % (la perforada, medido el 23
// oct 2026; el documento, pág. 30, decía 33 Ω), así que 4 mA son 0.60 V y
// 20 mA 3.00 V. Llega al A2 por R9 de 10k con C6 de 100 nF a tierra: un
// filtro, no cambia los volts. En el aire midió 0.591 V. El sensor mide la
// columna de agua que tiene encima; el nivel desde el fondo le suma a cuánto
// quedó del fondo.
//
// Calibración (Cisterna > Calibrar nivel):
// - rango de la etiqueta (0 a 2 m = 200 cm)
// - cero con el sensor en el aire
// - a cuántos cm del fondo quedó (2 o 3, para que no lo tape el lodo)
// - a cuántos cm del fondo está la rejilla de la bomba de pozo
// - "Está llena" con el flotador cerrado: ese nivel es el 100 %

#include "system_state.h"

namespace core {

inline constexpr float kLevelShuntOhms = 150.0f;
inline constexpr float kLevelSpanVolts = 0.016f * kLevelShuntOhms;  // de 4 a 20 mA: 2.4 V
// El cero en el aire que load() se cree: de 3 a 5 mA (0.45 a 0.75 V). El de
// fábrica, 4 mA, es SystemState::level_zero_volts.
inline constexpr float kLevelZeroMinVolts = 0.003f * kLevelShuntOhms;
inline constexpr float kLevelZeroMaxVolts = 0.005f * kLevelShuntOhms;

// Nivel desde el fondo, en cm
float level_cm(const SystemState& s);

// Lo útil, de la rejilla de la bomba de pozo (0 %) a llena (100 %): lo que
// la casa todavía puede sacar. -1 si falta marcar "Está llena".
int level_pct(const SystemState& s);

// "Hacer cero": lo que marca ahorita, con el sensor en el aire, es 0 cm
void level_zero(SystemState& s);

// "Está llena": el nivel de ahorita es el 100 %
void level_mark_full(SystemState& s);

}  // namespace core
