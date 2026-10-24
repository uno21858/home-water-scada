#pragma once
// Repuestos en casa (Ajustes > Cartuchos > Cambiar): cuántos cartuchos de
// cada uno hay guardados, con − y + para sumar lo que se compró o corregir.
// Bajan solos al registrar un cambio en Cartuchos. El prefiltro no lleva:
// su malla se lava, no se cambia.

#include <array>
#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class SparesScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    // Un renglón por cartucho: del 20 µm al sarro (core::Filter 1 a 5)
    struct Row {
        lv_obj_t* count = nullptr;
        lv_obj_t* minus = nullptr;
        lv_obj_t* plus = nullptr;
    };

    static void handle_step(lv_event_t* e);  // − o +: el botón lleva su renglón y el paso
    void show_count(size_t row);

    std::array<Row, 5> rows_{};
    // Copia de lo que se ve: los toques suman aquí sin esperar a update()
    std::array<uint8_t, 5> counts_{};
};

}  // namespace ui
