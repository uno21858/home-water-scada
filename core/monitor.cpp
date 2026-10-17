#include "monitor.h"

#include <algorithm>
#include <cmath>

#include "calendar.h"
#include "events.h"
#include "mixing.h"

namespace core {

void tick_minute(SystemState& s) {
    sample_minute(s);
    mixer_tick_minute(s);
    if (s.service_minutes_left > 0) --s.service_minutes_left;
    check_supply(s);
    check_drop(s);
}

void service_start(SystemState& s) { s.service_minutes_left = kServiceMinutes; }

void service_end(SystemState& s) { s.service_minutes_left = 0; }

void check_supply(SystemState& s) {
    if (in_service(s)) return;
    const bool lost = supply_lost(s);
    const bool open = has_open_event(s, EventKind::no_supply);
    if (lost && !open) add_event(s, EventKind::no_supply, 0);
    if (!lost && open) resolve_event(s, EventKind::no_supply);
}

void check_drop(SystemState& s) {
    if (!s.has_pressure || in_service(s) || s.flow_lpm < kFlowingLpm) return;
    const float rise = drop_rise(s);
    const auto psi = static_cast<uint16_t>(std::max(0, s.p1_psi - s.p2_psi));
    const bool warning = has_open_event(s, EventKind::drop_warning);
    const bool alarm = has_open_event(s, EventKind::drop_alarm);

    if (alarm) {
        if (rise >= kDropAlarmRise - kDropHysteresis) return;  // sigue en Tapados
        resolve_event(s, EventKind::drop_alarm);
        // Bajó de Tapados pero sigue arriba de lo que saca de Revisar
        if (rise >= kDropWarningRise - kDropHysteresis) add_event(s, EventKind::drop_warning, psi);
        return;
    }
    if (rise >= kDropAlarmRise) {
        if (warning) resolve_event(s, EventKind::drop_warning);
        add_event(s, EventKind::drop_alarm, psi);
        return;
    }
    if (warning) {
        if (rise < kDropWarningRise - kDropHysteresis) resolve_event(s, EventKind::drop_warning);
        return;
    }
    if (rise >= kDropWarningRise) add_event(s, EventKind::drop_warning, psi);
}

}  // namespace core
