#include "level.h"

#include <algorithm>
#include <cmath>

namespace core {

float level_cm(const SystemState& s) {
    // Lo que tiene encima: la parte del lazo arriba del cero, en proporción al rango
    const float above = (s.level_volts - s.level_zero_volts) / kLevelSpanVolts * static_cast<float>(s.level_range_cm);
    return static_cast<float>(s.level_sensor_cm) + std::max(0.0f, above);
}

int level_pct(const SystemState& s) {
    const float intake = static_cast<float>(s.level_intake_cm);
    const float full = static_cast<float>(s.level_full_cm);
    if (s.level_full_cm == 0 || full <= intake) return -1;
    const float pct = (level_cm(s) - intake) / (full - intake) * 100.0f;
    return static_cast<int>(std::lround(std::clamp(pct, 0.0f, 100.0f)));
}

void level_zero(SystemState& s) { s.level_zero_volts = s.level_volts; }

void level_mark_full(SystemState& s) {
    s.level_full_cm = static_cast<uint16_t>(std::lround(level_cm(s)));
}

}  // namespace core
