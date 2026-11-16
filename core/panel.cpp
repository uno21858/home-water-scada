#include "panel.h"

#include <cmath>
#include <cstdio>

#include "clog.h"
#include "level.h"

namespace core {

namespace {

// La pastilla, como ui::pill_look: un aviso o una alarma pesan más que el
// cambio de cartuchos
int status_code(const SystemState& s) {
    const Level level = overall_level(s);
    if (level == Level::alarm) return 2;
    if (level == Level::warning) return 1;
    return in_service(s) ? 3 : 0;
}

// Los cartuchos, con la regla de ui::clog_look
int filters_code(const SystemState& s) {
    if (!s.has_pressure) return -1;
    if (!pressure_ok(s)) return 4;
    if (s.clog_r0 <= 0.0f) return 3;
    switch (s.clog_state) {
        case ClogState::none: return 5;
        case ClogState::normal: return 0;
        case ClogState::check: return 1;
        case ClogState::clogged: return 2;
    }
    return 5;
}

int mixer_code(const SystemState& s) {
    if (!s.has_mixer) return -1;
    switch (s.mixer_fault) {
        case MixerFault::no_power: return 2;
        case MixerFault::dry: return 3;
        case MixerFault::stuck: return 4;
        case MixerFault::none: break;
    }
    return s.mixer_on ? 1 : 0;
}

// psi de P1 o P2; -1 sin sensores de presión, -2 ese sin señal
int psi_code(const SystemState& s, int psi, bool lost) { return !s.has_pressure ? -1 : lost ? -2 : psi; }

// El aviso de sin agua que sigue abierto; nullptr si no hay
const Event* open_no_supply(const SystemState& s) {
    for (std::size_t i = 0; i < s.recent_event_count && i < s.recent_events.size(); ++i) {
        const Event& e = s.recent_events[i];
        if (e.kind == EventKind::no_supply && e.resolved_at == kNoTime) return &e;
    }
    return nullptr;
}

}  // namespace

std::size_t panel_json(const SystemState& s, char* buf, std::size_t size) {
    if (size == 0) return 0;
    const bool flowing = s.flow_lpm >= kFlowingLpm;
    const bool lost = s.has_level && s.no_signal.level;
    const float usable = lost ? -1.0f : level_usable_liters(s);
    const float ratio = clog_ratio(s);
    // Desde cuándo no hay agua: solo mientras de verdad no hay
    const Event* dry = supply_lost(s) ? open_no_supply(s) : nullptr;
    const unsigned long dry_days = dry != nullptr && s.today > dry->day ? s.today - dry->day : 0;
    int alarm = -1;
    for (std::size_t i = 0; i < s.alarms.size() && alarm < 0; ++i) {
        if (s.alarms[i].active) alarm = static_cast<int>(i);
    }
    const int n = std::snprintf(
        buf, size,
        "{\"time\":\"%02u:%02u\",\"status\":%d,\"flow\":%.1f,\"no_supply\":%s,\"today\":%lu,\"dose_ml\":%lu,"
        "\"level\":%d,\"level_lost\":%s,\"liters\":%ld,\"fill_min\":%d,"
        "\"filters\":%d,\"filters_pct\":%ld,\"mixer\":%d,\"mixer_min\":%u,"
        "\"no_supply_at\":%d,\"no_supply_days\":%lu,\"alarm\":%d,\"alarm_at\":%u,\"p1\":%d,\"p2\":%d}",
        static_cast<unsigned>(s.hour), static_cast<unsigned>(s.minute), status_code(s),
        static_cast<double>(flowing ? s.flow_lpm : 0.0f), supply_lost(s) ? "true" : "false",
        static_cast<unsigned long>(s.liters_today), static_cast<unsigned long>(dose_ml(s)),
        level_ok(s) ? level_pct(s) : -1, lost ? "true" : "false",
        usable < 0.0f ? -1L : std::lround(usable / 10.0f) * 10,  // de 10 en 10: es aproximado
        flowing ? level_fill_minutes(s) : -1, filters_code(s),
        ratio > 0.0f ? std::lround(ratio * 100.0f) : -1L, mixer_code(s), static_cast<unsigned>(s.mixer_minutes_left),
        dry != nullptr ? static_cast<int>(dry->at) : -1, dry_days, alarm,
        alarm >= 0 ? static_cast<unsigned>(s.alarms[static_cast<std::size_t>(alarm)].since) : 0U,
        psi_code(s, static_cast<int>(s.p1_psi), s.no_signal.p1), psi_code(s, static_cast<int>(s.p2_psi), s.no_signal.p2));
    if (n < 0 || static_cast<std::size_t>(n) >= size) {
        buf[0] = '\0';
        return 0;
    }
    return static_cast<std::size_t>(n);
}

}  // namespace core
