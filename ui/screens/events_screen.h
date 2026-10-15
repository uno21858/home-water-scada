#pragma once
// Alarmas y eventos (desde Historial): las alarmas activas, lo de hoy o los
// últimos 7 días, con cuándo pasó y cómo quedó.

#include <array>
#include <cstdint>

#include "components/segmented.h"
#include "lvgl.h"
#include "system_state.h"

namespace ui {

class EventsScreen {
public:
    enum class Filter : uint8_t { active, today, week };

    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    struct Row {
        lv_obj_t* obj = nullptr;
        lv_obj_t* icon = nullptr;
        lv_obj_t* title = nullptr;
        lv_obj_t* when = nullptr;
        lv_obj_t* status = nullptr;
    };

    static void on_filter(int index, void* user_data);
    void refresh();  // vuelve a llenar la lista con lo último que llegó

    Segmented picker_;
    Filter filter_ = Filter::week;
    lv_obj_t* summary_ = nullptr;
    lv_obj_t* list_ = nullptr;
    std::array<Row, core::kEventCount> rows_{};
    Filter shown_filter_ = Filter::week;
    lv_obj_t* empty_ = nullptr;
    lv_obj_t* empty_note_ = nullptr;

    // Copia de lo que se lista, para poder cambiar de filtro sin esperar update()
    std::array<core::Event, core::kEventCount> events_{};
    uint8_t event_count_ = 0;
    std::array<core::Alarm, core::kAlarmKinds> alarms_{};
    uint32_t today_ = 0;
    core::Date date_{};
    uint16_t year_ = 0;
};

}  // namespace ui
