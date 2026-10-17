#pragma once
// Lo que se revisa cada minuto y el modo de cambio de cartuchos.

#include "system_state.h"

namespace core {

// El firmware lo llama cada minuto, después de set_time() (core/calendar.h):
// muestras de las gráficas, cuenta atrás de la bomba R y del modo de cambio,
// y los avisos de sin agua y de caída
void tick_minute(SystemState& s);

// Empieza el cambio de cartuchos: pausa los avisos de caída y de sin agua
// kServiceMinutes, o hasta service_end()
void service_start(SystemState& s);
void service_end(SystemState& s);  // "Ya terminé"

// Anota el aviso de sin agua cuando empieza y lo quita cuando regresa.
// Durante el cambio de cartuchos no toca nada: V1 cerrada no es el SIAPA.
void check_supply(SystemState& s);

// Anota la caída del tren en Revisar o Tapados y la quita cuando baja. Solo
// con agua pasando (sin flujo la caída no dice nada) y con 1 psi de margen
// para salir, para que no se prenda y se apague cuando anda en el borde.
inline constexpr float kDropHysteresis = 1.0f;
void check_drop(SystemState& s);

}  // namespace core
