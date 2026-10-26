#include "clog.h"

#include <algorithm>
#include <cmath>

#include "events.h"

namespace core {
namespace {

// Con margen: para salir de Tapados o de Revisar hay que bajar kClogHysteresis
ClogState classify(float ratio, ClogState now) {
    if (ratio > kClogAlarmRatio || (now == ClogState::clogged && ratio > kClogAlarmRatio - kClogHysteresis)) {
        return ClogState::clogged;
    }
    const bool was_up = now == ClogState::check || now == ClogState::clogged;
    if (ratio >= kClogWarningRatio || (was_up && ratio >= kClogWarningRatio - kClogHysteresis)) {
        return ClogState::check;
    }
    return ClogState::normal;
}

// Los avisos del registro siguen al estado: uno abierto por el que está
void log_change(SystemState& s, ClogState before, ClogState after, float ratio) {
    if (after == before) return;
    const auto pct = static_cast<uint16_t>(std::lround(std::min(ratio * 100.0f, 65535.0f)));
    if (before == ClogState::check) resolve_event(s, EventKind::clog_warning);
    if (before == ClogState::clogged) resolve_event(s, EventKind::clog_alarm);
    if (after == ClogState::check) add_event(s, EventKind::clog_warning, pct);
    if (after == ClogState::clogged) add_event(s, EventKind::clog_alarm, pct);
}

void evaluate(SystemState& s, float r, float lpm) {
    s.clog_r = r;
    s.clog_r_lpm = lpm;
    s.clog_r_day = s.today;
    s.clog_r_at = minute_of_day(s);
    s.clog_age_s = 0;

    // El promedio de hoy, para la tendencia (en centésimas; 0 es día sin agua)
    uint16_t& today = s.clog_days.back();
    const float before = static_cast<float>(today) / 100.0f;
    const float n = static_cast<float>(s.clog_today_count);
    const float mean = (before * n + r) / (n + 1.0f);
    today = static_cast<uint16_t>(std::clamp(std::lround(mean * 100.0f), 1L, 65535L));
    if (s.clog_today_count < 0xFFFF) ++s.clog_today_count;

    if (s.clog_r0 <= 0.0f) {
        s.clog_state = ClogState::none;  // sin referencia no se califica
        return;
    }
    const float ratio = r / s.clog_r0;
    const ClogState next = classify(ratio, s.clog_state);
    log_change(s, s.clog_state, next, ratio);
    s.clog_state = next;
}

}  // namespace

bool clog_tick(SystemState& s, ClogWindow& w) {
    if (s.clog_age_s < 0xFFFF) ++s.clog_age_s;
    // Solo con P1 y P2 leyendo, fuera del cambio de cartuchos y con agua: si
    // no, la ventana empieza de nuevo
    if (!pressure_ok(s) || in_service(s) || s.flow_lpm < kClogMinLpm) {
        w.count = 0;
        return false;
    }
    if (w.count == kClogWindowSeconds) {
        std::rotate(w.lpm.begin(), w.lpm.begin() + 1, w.lpm.end());
        std::rotate(w.psi.begin(), w.psi.begin() + 1, w.psi.end());
    } else {
        ++w.count;
    }
    const auto last = static_cast<std::size_t>(w.count - 1);
    w.lpm[last] = s.flow_lpm;
    w.psi[last] = static_cast<float>(std::max(0, s.p1_psi - s.p2_psi));
    if (w.count < kClogWindowSeconds) return false;

    // Estable: en los últimos 30 s el caudal no se movió más de 15 %. Si no,
    // la ventana sigue corriendo un segundo a la vez.
    const auto stable_from = w.lpm.end() - kClogStableSeconds;
    const auto [lo, hi] = std::minmax_element(stable_from, w.lpm.end());
    if (*hi > *lo * (1.0f + kClogStableSpread)) return false;

    float r = 0.0f;
    float lpm = 0.0f;
    for (std::size_t i = 0; i < w.lpm.size(); ++i) {
        r += w.psi[i] / w.lpm[i];
        lpm += w.lpm[i];
    }
    evaluate(s, r / kClogWindowSeconds, lpm / kClogWindowSeconds);
    w.count = 0;  // la siguiente, con otros 60 s
    return true;
}

float clog_ratio(const SystemState& s) {
    if (s.clog_r0 <= 0.0f || s.clog_r <= 0.0f) return 0.0f;
    return s.clog_r / s.clog_r0;
}

bool clog_fresh(const SystemState& s) { return s.clog_r > 0.0f && s.clog_age_s <= kClogFreshSeconds; }

bool clog_save_reference(SystemState& s) {
    if (!clog_fresh(s)) return false;
    s.clog_r0 = s.clog_r;
    s.clog_r0_lpm = s.clog_r_lpm;
    s.clog_r0_day = s.today;
    // Contra sí misma es 100 %: Normal, y se cierran los avisos que hubiera
    log_change(s, s.clog_state, ClogState::normal, 1.0f);
    s.clog_state = ClogState::normal;
    return true;
}

}  // namespace core
