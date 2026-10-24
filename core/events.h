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

// Lo mismo, solo los de ese value (no_signal: cada Sensor lleva el suyo)
bool resolve_event(SystemState& s, EventKind kind, uint16_t value);
bool has_open_event(const SystemState& s, EventKind kind, uint16_t value);

// Alarmas a pantalla completa (las de la fase 3; Probar alarma usa la de sin
// flujo). Prender una la saca en pantalla y la anota; si ya estaba, nada.
void alarm_raise(SystemState& s, AlarmKind kind);

// Tocaron Entendido: sale de la pantalla y su evento queda reconocido a esta
// hora, pero sigue activa (y la pastilla en Alarma) hasta alarm_clear()
void alarm_acknowledge(SystemState& s, AlarmKind kind);

// Se quitó la condición: deja de estar activa y su evento queda cerrado
void alarm_clear(SystemState& s, AlarmKind kind);

}  // namespace core
