#include "components/status_pill.h"

#include "theme/colors.h"

namespace ui {

PillLook pill_look(const core::SystemState& s) {
    switch (core::overall_level(s)) {
        case core::Level::warning:
            return {PillLook::warning, colors::warning, colors::warning_bg, colors::warning, "Advertencia"};
        case core::Level::alarm:
            return {PillLook::alarm, colors::on_alarm, colors::alarm_screen, colors::alarm_screen, "Alarma"};
        case core::Level::ok:
            break;
    }
    if (core::in_service(s)) {
        return {PillLook::service, colors::text, colors::panel_raised, colors::border_control, "Mantenimiento"};
    }
    return {PillLook::ok, colors::ok, colors::ok_bg, colors::ok, "OK"};
}

}  // namespace ui
