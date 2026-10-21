#include "events.h"

#include <algorithm>

namespace core {

void add_event(SystemState& s, EventKind kind, uint16_t value) {
    // Recorre todos un lugar hacia atrás: el último se cae y el nuevo va primero
    std::rotate(s.recent_events.rbegin(), s.recent_events.rbegin() + 1, s.recent_events.rend());
    s.recent_events[0] = {s.today, minute_of_day(s), kind, value, kNoTime, kNoTime, ++s.event_seq};
    s.recent_event_count = static_cast<uint8_t>(std::min<int>(s.recent_event_count + 1, kEventCount));
}

bool resolve_event(SystemState& s, EventKind kind) {
    for (uint8_t i = 0; i < s.recent_event_count; ++i) {
        Event& e = s.recent_events[i];
        if (e.kind == kind && e.resolved_at == kNoTime) {
            e.resolved_at = minute_of_day(s);
            return true;
        }
    }
    return false;
}

bool has_open_event(const SystemState& s, EventKind kind) {
    for (uint8_t i = 0; i < s.recent_event_count; ++i) {
        const Event& e = s.recent_events[i];
        if (e.kind == kind && e.resolved_at == kNoTime) return true;
    }
    return false;
}

}  // namespace core
