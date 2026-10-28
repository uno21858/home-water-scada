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
// - litros por cm: una Rotoplas no es pareja y no se saben sus medidas, así
//   que se miden con F. Con agua entrando y nadie sacando (la bomba de pozo
//   apagada), los litros que contó F entre los cm que subió. Con eso salen
//   los litros que hay y el tope de la dosis (dose_liters).

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

// Para guardar los litros por cm tiene que subir al menos esto: con menos,
// el ruido del sensor pesa (medio cm en 5 son 10 %)
inline constexpr float kLevelMeasureMinCm = 5.0f;
// Los litros por cm que load() y la medición se creen
inline constexpr float kLevelLitersPerCmMin = 1.0f;
inline constexpr float kLevelLitersPerCmMax = 1000.0f;

// Litros que hay en la cisterna, con lo de abajo de la toma: los cm desde el
// fondo por los litros por cm. Aproximado: la cisterna no es pareja de
// arriba abajo. -1 sin litros por cm o sin lectura del nivel.
float level_liters(const SystemState& s);

// Medir los litros por cm: empieza en el nivel de ahorita con F en 0
void level_measure_start(SystemState& s);

// Cuánto ha subido desde que empezó, en cm
float level_measure_rise(const SystemState& s);

// Los litros por cm que van; 0 mientras no suba ni medio cm
float level_measure_ratio(const SystemState& s);

// Ya subió kLevelMeasureMinCm con señal y el número es creíble
bool level_measure_ready(const SystemState& s);

// Guarda lo medido y termina; false si todavía no está lista
bool level_measure_save(SystemState& s);

// Termina sin guardar
void level_measure_cancel(SystemState& s);

// Los rangos que load() y level_set_range se creen, en cm
inline constexpr uint16_t kLevelRangeMinCm = 10;
inline constexpr uint16_t kLevelRangeMaxCm = 2000;

// Cambia el rango de la etiqueta. "Está llena" y los litros por cm se
// guardaron con los volts de ese momento: se recalculan con el rango nuevo,
// así no hay que esperar a que se llene ni volver a medir (Erick, 28 oct
// 2026: con 200 puesto y un sensor de más rango, llena salía en 88 cm en una
// cisterna de 220). La toma y la altura del sensor son cm medidos a mano y
// se quedan. Una medición a medias se cancela.
void level_set_range(SystemState& s, uint32_t range_cm);

// "Hacer cero": lo que marca ahorita, con el sensor en el aire, es 0 cm
void level_zero(SystemState& s);

// "Está llena": el nivel de ahorita es el 100 %
void level_mark_full(SystemState& s);

}  // namespace core
