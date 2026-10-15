#pragma once
// Pantalla Historial: una gráfica que cambia con dos selectores (Día, Semana o
// Mes; Litros y dosis, Presiones u ORP) y los últimos eventos.
//
// La gráfica no es un lv_chart: es un objeto que se dibuja a sí mismo (barras,
// líneas, la banda de la caída y el umbral) con la API de dibujo de LVGL.
// Así caben las nueve combinaciones sin crear un objeto por barra.

#include <array>
#include <cstdint>

#include "components/segmented.h"
#include "lvgl.h"
#include "system_state.h"

namespace ui {

class HistoryScreen {
public:
    enum class Range : uint8_t { day, week, month };
    enum class Signal : uint8_t { liters, pressure, orp };

    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    struct Legend {
        lv_obj_t* obj = nullptr;
        lv_obj_t* swatch = nullptr;
        lv_obj_t* text = nullptr;
    };
    struct EventRow {
        lv_obj_t* obj = nullptr;
        lv_obj_t* icon = nullptr;
        lv_obj_t* title = nullptr;
        lv_obj_t* when = nullptr;
    };
    struct Bar {
        int32_t x;  // izquierda, en la gráfica
        int32_t y;  // arriba
        int32_t w;
    };

    void create_chart(lv_obj_t* page);
    void create_events(lv_obj_t* page);
    void rebuild();  // vuelve a calcular la gráfica con lo último que llegó
    void update_events(const core::SystemState& s);
    static void draw_plot(lv_event_t* e);
    static void on_range(int index, void* user_data);
    static void on_signal(int index, void* user_data);

    Segmented range_picker_;
    Segmented signal_picker_;
    Range range_ = Range::week;
    Signal signal_ = Signal::liters;

    lv_obj_t* title_ = nullptr;
    std::array<Legend, 3> legend_{};
    std::array<lv_obj_t*, 4> y_labels_{};
    std::array<lv_obj_t*, 3> y2_labels_{};
    std::array<lv_obj_t*, 7> x_labels_{};
    lv_obj_t* plot_ = nullptr;
    lv_obj_t* locked_ = nullptr;  // "Próximamente" si la señal no está instalada

    // Lo que dibuja draw_plot, en coordenadas de la tarjeta
    std::array<int32_t, 3> grid_y_{};
    int grid_count_ = 0;
    std::array<Bar, 30> bars_a_{};  // litros
    std::array<Bar, 30> bars_b_{};  // dosis
    int bars_a_count_ = 0;
    int bars_b_count_ = 0;
    std::array<lv_point_precise_t, core::kOrpSamples7d> line1_{};
    std::array<lv_point_precise_t, 30> line2_{};
    int line1_count_ = 0;
    int line2_count_ = 0;
    uint32_t line1_color_ = 0;
    bool band_ = false;     // banda entre las dos líneas (caída del tren)
    int32_t ref_y_ = -1;    // umbral punteado de 650 mV; -1 = no hay
    // Las líneas se dibujan con coordenadas de pantalla: se copian aquí al dibujar
    std::array<lv_point_precise_t, core::kOrpSamples7d> line1_abs_{};
    std::array<lv_point_precise_t, 30> line2_abs_{};

    // Lo que se graficó la última vez, para no redibujar si no cambió
    std::array<core::Totals, core::kHistoryDays> shown_days_{};
    std::array<core::Totals, 24> shown_hours_{};
    std::array<int16_t, core::kOrpSamples7d> shown_orp_{};
    uint32_t shown_today_ = 0;
    core::Date shown_date_{};
    uint16_t shown_year_ = 0;
    int shown_hour_ = -1;
    bool shown_pressure_ = false;
    bool shown_orp_installed_ = false;

    std::array<EventRow, 5> events_{};
};

}  // namespace ui
