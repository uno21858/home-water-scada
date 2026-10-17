#pragma once
// Lo que se revisa cada minuto y el modo de cambio de cartuchos.

#include "system_state.h"

namespace core {

// El firmware lo llama cada minuto: cuenta atrás de la bomba R y del modo de
// cambio, y anota o quita el aviso de sin agua del SIAPA
void tick_minute(SystemState& s);

// Empieza el cambio de cartuchos: pausa los avisos de caída y de sin agua
// kServiceMinutes, o hasta service_end()
void service_start(SystemState& s);
void service_end(SystemState& s);  // "Ya terminé"

// Anota el aviso de sin agua cuando empieza y lo quita cuando regresa.
// Durante el cambio de cartuchos no toca nada: V1 cerrada no es el SIAPA.
void check_supply(SystemState& s);

}  // namespace core
