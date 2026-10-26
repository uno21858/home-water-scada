#pragma once
// Programa de la bomba R, la que revuelve la cisterna.
//
// - Auto: corre mixer_after_dose_min después de cada dosis (documento, pág.
//   39) y mixer_every_liters_min cada que entran kMixEveryLiters de agua
//   nueva. De noche o sin SIAPA no entra agua y no corre de oquis.
// - Prendida: corre hasta mixer_manual_max_min y queda Apagada.
// - Apagada: no corre por nada.
//
// Si el INA219 vio algo (sin 12 V, nivel bajo, atorada) la bomba se apaga y
// la vuelta queda pendiente: corre en cuanto se arregla. Atorada no se
// reintenta sola: la revisa una persona y toca el selector. Con sensor de
// nivel, en 0 % (el mínimo de Calibrar nivel) tampoco arranca, y si iba
// corriendo se apaga: la vuelta corre cuando la cisterna sube.
//
// Prueba de banco (mixer_set_bench), con la bomba en una cubeta: no espera
// a la cisterna ni la apaga el nivel bajo. Atorada y sin 12 V siguen.
//
// El firmware llama estas funciones y prende el GPIO44 según s.mixer_on.

#include <cstdint>

#include "system_state.h"

namespace core {

// Tocaron Apagada, Prendida o Auto. También es "ya la revisé": quita el
// aviso de atorada o de nivel bajo.
void mixer_set_mode(SystemState& s, MixerMode mode);

// Se registró una dosis: en Auto o Prendida, la vuelta de la dosis
void mixer_after_dose(SystemState& s);

// Entró agua nueva a la cisterna (la que contó F)
void mixer_add_liters(SystemState& s, uint32_t liters);

// Pasó un minuto: cuenta atrás de la vuelta y arranca lo que toque
void mixer_tick_minute(SystemState& s);

// El INA219 vio algo: la apaga, deja pendiente la vuelta y lo anota
void mixer_report_fault(SystemState& s, MixerFault fault);

// Regresaron los 12 V: se quita el aviso y corre lo pendiente
void mixer_power_back(SystemState& s);

// La cisterna está en el mínimo (0 %): la bomba R espera agua. En prueba de
// banco nunca.
bool mixer_low_water(const SystemState& s);

// Empieza o termina la prueba de banco. Al empezar se quita el aviso de
// nivel bajo que hubiera. No se guarda: al reiniciar regresa la protección.
void mixer_set_bench(SystemState& s, bool on);

// Se reinició la placa (apagón, OTA o Reiniciar) con la bomba andando: arranca
// apagada (documento: el GPIO44 en bajo antes que nada) y lo que le faltaba a
// la vuelta queda pendiente; corre en el siguiente minuto si nada lo impide
void mixer_after_restart(SystemState& s);

}  // namespace core
