#pragma once
// Registro de eventos, lo que listan Alarmas e Historial: agregar uno y
// anotar cuándo se quitó un aviso.

#include <cstdint>

#include "system_state.h"

namespace core {

// Agrega un evento con la hora de ahorita. Si ya hay kEventCount, se cae el
// más viejo.
void add_event(SystemState& s, EventKind kind, uint16_t value);

// El aviso más nuevo de ese tipo que sigue abierto se quitó ahorita.
// Regresa false si no había ninguno abierto.
bool resolve_event(SystemState& s, EventKind kind);

// ¿Hay un aviso de ese tipo que todavía no se quita?
bool has_open_event(const SystemState& s, EventKind kind);

}  // namespace core
