#include "events.h"

#include <algorithm>

namespace core {
namespace {

// El evento más nuevo de esa alarma
Event* alarm_event(SystemState& s, AlarmKind kind) {
    for (uint8_t i = 0; i < s.recent_event_count; ++i) {
        Event& e = s.recent_events[i];
        if (e.kind == EventKind::alarm && e.value == static_cast<uint16_t>(kind)) return &e;
    }
    return nullptr;
}

}  // namespace

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

void alarm_raise(SystemState& s, AlarmKind kind) {
    Alarm& a = s.alarms[static_cast<std::size_t>(kind)];
    if (a.active) return;
    a = {true, false, minute_of_day(s)};
    add_event(s, EventKind::alarm, static_cast<uint16_t>(kind));
}

void alarm_acknowledge(SystemState& s, AlarmKind kind) {
    s.alarms[static_cast<std::size_t>(kind)].acknowledged = true;
    Event* e = alarm_event(s, kind);
    if (e != nullptr && e->acknowledged_at == kNoTime) e->acknowledged_at = minute_of_day(s);
}

void alarm_clear(SystemState& s, AlarmKind kind) {
    Alarm& a = s.alarms[static_cast<std::size_t>(kind)];
    if (!a.active) return;
    a.active = false;
    Event* e = alarm_event(s, kind);
    if (e != nullptr && e->resolved_at == kNoTime) e->resolved_at = minute_of_day(s);
}

}  // namespace core
