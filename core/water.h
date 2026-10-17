#pragma once
// Lo que pasa cuando entra agua, se echa cloro o se cambia un cartucho.

#include <cstdint>

#include "system_state.h"

namespace core {

// F contó estos litros: suben los del día, los de la hora, los que van desde
// la dosis, los de cada cartucho y los de la mezcla (pueden prender la bomba R)
void count_liters(SystemState& s, uint32_t liters);

// Se registró una dosis ("Ya le puse" o "Registrar otra cantidad"): la anota,
// la cuenta de litros vuelve a 0 y en Auto prende la mezcla
void register_dose(SystemState& s, uint32_t ml, bool typed);

// "Lo cambié", "Lo puse" o "Ya purgué": el filtro vuelve a contar desde hoy
// y, si es cartucho, baja uno de los repuestos
void register_filter(SystemState& s, Filter filter);

}  // namespace core
