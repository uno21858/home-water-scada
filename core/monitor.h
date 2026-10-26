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

// Cierra los avisos de caída que dejó el firmware de antes (umbrales fijos
// en psi). Ahora los cartuchos se califican con R contra R0: core/clog.h.
void close_old_drop_notices(SystemState& s);

// Cada segundo, con o sin red. Al regresar, si estuvo caída al menos
// kOfflineEventSeconds, anota "Sin red" con los minutos y desde cuándo (lo
// que se quedó en la cola se mandó después). Las caídas cortas no se anotan.
inline constexpr uint32_t kOfflineEventSeconds = 120;
void network_tick(SystemState& s, bool online);

}  // namespace core
