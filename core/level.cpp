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

float level_liters(const SystemState& s) {
    if (s.level_liters_per_cm <= 0.0f || !level_ok(s)) return -1.0f;
    return level_cm(s) * s.level_liters_per_cm;
}

void level_set_liters_per_cm(SystemState& s, float liters_per_cm) {
    if (!std::isfinite(liters_per_cm) || liters_per_cm < kLevelLitersPerCmMin ||
        liters_per_cm > kLevelLitersPerCmMax) {
        return;
    }
    s.level_liters_per_cm = liters_per_cm;
    s.level_liters_per_cm_measured = false;
    level_measure_cancel(s);
}

void level_measure_start(SystemState& s) {
    s.level_measuring = true;
    s.level_measure_from_cm = level_cm(s);
    s.level_measure_liters = 0;
}

float level_measure_rise(const SystemState& s) {
    if (!s.level_measuring) return 0.0f;
    return std::max(0.0f, level_cm(s) - s.level_measure_from_cm);
}

float level_measure_ratio(const SystemState& s) {
    const float rise = level_measure_rise(s);
    if (rise < 0.5f) return 0.0f;
    return static_cast<float>(s.level_measure_liters) / rise;
}

bool level_measure_ready(const SystemState& s) {
    if (!s.level_measuring || !level_ok(s)) return false;
    const float ratio = level_measure_ratio(s);
    return level_measure_rise(s) >= kLevelMeasureMinCm && ratio >= kLevelLitersPerCmMin &&
           ratio <= kLevelLitersPerCmMax;
}

bool level_measure_save(SystemState& s) {
    if (!level_measure_ready(s)) return false;
    s.level_liters_per_cm = level_measure_ratio(s);
    s.level_liters_per_cm_measured = true;
    s.level_measuring = false;
    return true;
}

void level_measure_cancel(SystemState& s) { s.level_measuring = false; }

void level_set_range(SystemState& s, uint32_t range_cm) {
    if (range_cm < kLevelRangeMinCm || range_cm > kLevelRangeMaxCm || range_cm == s.level_range_cm) return;
    if (s.level_range_cm > 0) {
        const float k = static_cast<float>(range_cm) / static_cast<float>(s.level_range_cm);
        if (s.level_full_cm > 0) {
            // Lo de arriba del sensor crece con el rango; los cm del sensor no
            const float sensor = static_cast<float>(s.level_sensor_cm);
            const float above = std::max(0.0f, static_cast<float>(s.level_full_cm) - sensor);
            s.level_full_cm = static_cast<uint16_t>(std::lround(std::min(sensor + above * k, 65535.0f)));
        }
        // Solo los medidos con F: los de fábrica o escritos son de la cisterna
        // (Erick, 3 nov 2026: los 44.5 de fábrica quedaron en 17.8 al pasar
        // de 200 a 500 y decía 2,300 L con 129 cm de agua)
        if (s.level_liters_per_cm_measured && s.level_liters_per_cm > 0.0f) s.level_liters_per_cm /= k;
    }
    level_measure_cancel(s);
    s.level_range_cm = static_cast<uint16_t>(range_cm);
}

void level_fill_tick(SystemState& s) {
    if (s.flow_lpm < kFlowingLpm) {
        s.fill_lpm = 0.0f;
    } else if (s.fill_lpm <= 0.0f) {
        s.fill_lpm = s.flow_lpm;  // desde 0 tardaría medio minuto en dar un número creíble
    } else {
        // Promedio corrido: cada segundo se acerca 1/30 a lo que marca F
        s.fill_lpm += (s.flow_lpm - s.fill_lpm) / kFillAverageSeconds;
    }
}

int level_fill_minutes(const SystemState& s) {
    if (s.fill_lpm < kFlowingLpm || s.level_full_cm == 0 || s.level_liters_per_cm <= 0.0f || !level_ok(s)) return -1;
    // A menos de medio cm ya está llena: el nivel se dice en cm enteros
    const float missing_cm = static_cast<float>(s.level_full_cm) - level_cm(s);
    if (missing_cm < 0.5f) return -1;
    // Tope de una semana: con un hilito de agua la cuenta no dice nada
    const float minutes = std::min(missing_cm * s.level_liters_per_cm / s.fill_lpm, 7.0f * 24.0f * 60.0f);
    return std::max(1, static_cast<int>(std::lround(minutes)));
}

void level_zero(SystemState& s) { s.level_zero_volts = s.level_volts; }

void level_mark_full(SystemState& s) {
    s.level_full_cm = static_cast<uint16_t>(std::lround(level_cm(s)));
}

}  // namespace core
