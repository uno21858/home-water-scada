#pragma once
// Calibrar nivel (desde Nivel): un renglón por dato, en el orden en que se
// hace al instalarlo. Rango de la etiqueta, cero en el aire, a cuántos cm del
// fondo quedó, la rejilla de la bomba de pozo y "Está llena". No está en el
// canvas: copia los renglones de Cartuchos.

#include <array>
#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class LevelCalibrationScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    // Cada botón lleva su renglón en el user_data
    enum Row : uint8_t { range, zero, sensor, intake, full, kRows };
    static void handle_row(lv_event_t* e);

    std::array<lv_obj_t*, kRows> values_{};
    lv_obj_t* note_ = nullptr;
    // Lo último que llegó, para las pistas de los diálogos (copiar todo el
    // estado sería mucho por cuatro números)
    float volts_ = 0.0f;
    float cm_ = 0.0f;
    uint8_t sensor_cm_ = 0;
    uint8_t intake_cm_ = 0;
};

}  // namespace ui
