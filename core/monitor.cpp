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
    // Con P1 sin señal no se sabe: el aviso se queda como estaba
    if (in_service(s) || s.no_signal.p1) return;
    const bool lost = supply_lost(s);
    const bool open = has_open_event(s, EventKind::no_supply);
    if (lost && !open) add_event(s, EventKind::no_supply, 0);
    if (!lost && open) resolve_event(s, EventKind::no_supply);
}

void check_drop(SystemState& s) {
    if (!pressure_ok(s) || in_service(s) || s.flow_lpm < kFlowingLpm) return;
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

void network_tick(SystemState& s, bool online) {
    s.wifi_ok = online;
    if (!online) {
        ++s.offline_seconds;
        return;
    }
    if (s.offline_seconds >= kOfflineEventSeconds) {
        const uint32_t minutes = s.offline_seconds / 60;
        add_event(s, EventKind::offline, static_cast<uint16_t>(std::min<uint32_t>(minutes, 0xFFFF)));
        // El evento va desde que se cayó; si fue antes de la medianoche, es de ayer
        Event& e = s.recent_events[0];
        const int32_t start = static_cast<int32_t>(minute_of_day(s)) - static_cast<int32_t>(minutes);
        const int32_t days_back = start < 0 ? (-start + 1439) / 1440 : 0;
        e.at = static_cast<uint16_t>(start + days_back * 1440);
        e.day = e.day > static_cast<uint32_t>(days_back) ? e.day - static_cast<uint32_t>(days_back) : e.day;
        e.resolved_at = minute_of_day(s);
    }
    s.offline_seconds = 0;
}

}  // namespace core
