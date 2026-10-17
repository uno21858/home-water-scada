#include "monitor.h"

#include "events.h"
#include "mixing.h"

namespace core {

void tick_minute(SystemState& s) {
    mixer_tick_minute(s);
    if (s.service_minutes_left > 0) --s.service_minutes_left;
    check_supply(s);
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

}  // namespace core
