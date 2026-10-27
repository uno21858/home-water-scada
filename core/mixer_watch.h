#pragma once
// Vigila la corriente de la bomba R con cada lectura del INA219 (en la 7B,
// una cada ~50 ms) y decide si va en seco (nivel bajo) o atorada.
//
// - Ventana ciega: los primeros kMixerBlindMs después de prenderla no
//   cuentan. Un motor de escobillas jala de 3 a 5 veces su corriente al
//   arrancar (de 100 a 300 ms).
// - Una falla necesita kMixerSustainMs seguidos del mismo lado del umbral.
//   Una lectura suelta no apaga nada: el INA219 mide medio milisegundo y
//   agarra el rizo de las escobillas, arriba o abajo del promedio.
// - Si entre dos lecturas pasan más de kMixerMaxGapMs (el I2C se tardó), la
//   cuenta empieza otra vez: "seguidos" es con lecturas de verdad.
// - Sin lectura o abajo de min_volts la corriente no se evalúa: eso es sin
//   12 V, y lo decide la app con su propia espera.
//
// El firmware lo corre en la tarea de sensores (firmware/hmi/main/sensors.cpp)
// y la app le pasa la falla a core::mixer_report_fault.

#include <cstdint>

#include "system_state.h"

namespace core {

inline constexpr uint32_t kMixerBlindMs = 500;
inline constexpr uint32_t kMixerSustainMs = 500;
inline constexpr uint32_t kMixerMaxGapMs = 200;

struct MixerLimits {
    float min_volts = 9.0f;   // abajo: sin 12 V, la corriente no cuenta
    float dry_amps = 1.0f;    // abajo, sostenido: nivel bajo (en seco)
    float stuck_amps = 2.5f;  // arriba, sostenido: atorada (1.28 veces los 1.96 A medidos de la ZYW680)
};

struct MixerWatch {
    uint32_t started_ms = 0;    // cuándo se prendió
    uint32_t last_ms = 0;       // la lectura anterior
    bool has_last = false;
    bool high = false;          // las últimas lecturas, arriba de stuck_amps
    uint32_t high_since_ms = 0; // desde cuándo
    bool low = false;           // las últimas lecturas, abajo de dry_amps
    uint32_t low_since_ms = 0;
};

// Se prendió la bomba: empieza la ventana ciega y se olvida lo de antes
void mixer_watch_start(MixerWatch& w, uint32_t now_ms);

// Una lectura del INA219 con la bomba prendida. Regresa stuck o dry cuando la
// corriente lleva kMixerSustainMs del otro lado de su umbral; si no, none.
MixerFault mixer_watch_sample(MixerWatch& w, const MixerLimits& limits, uint32_t now_ms, bool ok, float volts,
                              float amps);

}  // namespace core
