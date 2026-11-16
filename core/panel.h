#pragma once
// El panel del teléfono de la pared: lo básico del Pasillo en JSON, para la
// página que sirve la placa (firmware/hmi/main/panel.html, GET / y GET
// /datos). La página pregunta cada segundo y pone las palabras; de aquí
// salen números y códigos.
//
// {"time":"18:42","status":0,"flow":5.6,"no_supply":false,"today":1860,
//  "dose_ml":12,"level":73,"level_lost":false,"liters":2250,"fill_min":150,
//  "filters":0,"filters_pct":101,"mixer":1,"mixer_min":32,
//  "no_supply_at":870,"no_supply_days":0,"alarm":-1,"alarm_at":0,
//  "p1":22,"p2":1}
//
// - status: 0 OK, 1 Advertencia, 2 Alarma, 3 Mantenimiento (la pastilla).
// - level, liters, fill_min, filters_pct: -1 si no hay (sin sensor, sin
//   calibrar, sin flujo...). liters son los que se pueden usar.
// - filters: -1 sin P1 y P2; 0 Normal, 1 Revisar, 2 Tapados, 3 sin
//   referencia, 4 sin señal, 5 con referencia pero sin evaluar todavía.
// - mixer: -1 sin bomba R; 0 apagada, 1 prendida (mixer_min: lo que falta),
//   2 sin 12 V, 3 nivel bajo, 4 atorada.
// - no_supply_at: desde cuándo no hay agua del SIAPA, en minutos del día
//   (870 = 14:30), y no_supply_days cuántos días atrás empezó (0 hoy, 1
//   ayer). -1 si hay agua, o si se acaba de ir y todavía no se anota (se
//   anota al minuto, core/monitor.h).
// - p1, p2: psi antes y después de los filtros; -1 sin sensores de presión,
//   -2 ese sensor sin señal.
// - alarm: la AlarmKind de la primera alarma activa (0 sin flujo al
//   dosificar, 1 sensor F, 2 tope diario) y alarm_at su minuto del día; -1
//   sin alarma. Con status 2 y alarm -1 son los cartuchos tapados.

#include <cstddef>

#include "system_state.h"

namespace core {

// Escribe el JSON en buf (con su '\0') y regresa cuántos bytes, sin el '\0'.
// Si no cabe regresa 0 y buf queda vacío: nunca un JSON cortado.
std::size_t panel_json(const SystemState& s, char* buf, std::size_t size);

}  // namespace core
