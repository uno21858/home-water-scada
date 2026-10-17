#include "water.h"

#include <algorithm>

#include "events.h"
#include "mixing.h"

namespace core {
namespace {

// Suma sin pasarse del tope de un uint16_t
uint16_t add16(uint16_t a, uint32_t b) { return static_cast<uint16_t>(std::min<uint32_t>(a + b, 0xFFFF)); }

}  // namespace

void count_liters(SystemState& s, uint32_t liters) {
    if (liters == 0) return;
    s.liters_today += liters;
    s.liters_since_dose += liters;
    Totals& hour = s.hours[s.hour % s.hours.size()];
    hour.liters = add16(hour.liters, liters);
    s.days.back().liters = add16(s.days.back().liters, liters);
    for (FilterStatus& f : s.filters) {
        if (f.installed) f.liters += liters;
    }
    mixer_add_liters(s, liters);
}

void register_dose(SystemState& s, uint32_t ml, bool typed) {
    const uint16_t amount = add16(0, ml);
    std::rotate(s.recent_doses.rbegin(), s.recent_doses.rbegin() + 1, s.recent_doses.rend());
    s.recent_doses[0] = {s.today, minute_of_day(s), amount, typed};
    s.recent_dose_count = static_cast<uint8_t>(std::min<int>(s.recent_dose_count + 1, static_cast<int>(s.recent_doses.size())));
    s.liters_since_dose = 0;
    Totals& hour = s.hours[s.hour % s.hours.size()];
    hour.dose_ml = add16(hour.dose_ml, amount);
    s.days.back().dose_ml = add16(s.days.back().dose_ml, amount);
    add_event(s, typed ? EventKind::dose_typed : EventKind::dose, amount);
    mixer_after_dose(s);
}

void register_filter(SystemState& s, Filter filter) {
    const auto i = static_cast<std::size_t>(filter);
    const bool was_empty = !s.filters[i].installed;
    s.filters[i] = {s.today, 0, true};
    if (filter == Filter::prefilter) {
        add_event(s, EventKind::purge, 0);
        return;
    }
    if (s.spares[i] > 0) --s.spares[i];
    s.last_cartridge = filter;
    s.last_cartridge_day = s.today;
    add_event(s, was_empty ? EventKind::cartridge_added : EventKind::cartridge, static_cast<uint16_t>(filter));
}

}  // namespace core
