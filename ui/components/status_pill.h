#pragma once
// Cómo se ve la pastilla de estado. La usan el encabezado y el Pasillo, cada
// uno con sus íconos (32 y 40 px).

#include <cstdint>

#include "system_state.h"

namespace ui {

struct PillLook {
    enum Kind { ok, warning, alarm, service };  // también es el índice del ícono
    Kind kind;
    uint32_t fg;
    uint32_t bg;
    uint32_t border;
    const char* text;
};

// En cambio de cartuchos la pastilla lo dice, salvo que haya un aviso o una
// alarma: eso pesa más
PillLook pill_look(const core::SystemState& s);

}  // namespace ui
