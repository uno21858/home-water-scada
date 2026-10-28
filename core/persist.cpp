#include "persist.h"

#include <cmath>
#include <cstring>
#include <type_traits>

#include "level.h"

namespace core {
namespace {

// Una pasada por los campos de un grupo: f(llave, campo). Las llaves de la
// NVS miden hasta 15 letras y no se repiten (lo revisan las pruebas).
template <typename S, typename F>
void each(SaveGroup group, S& s, F&& f) {
    switch (group) {
        case SaveGroup::settings:
            f("flow_k", s.flow_pulses_per_liter);
            f("ml_per_l", s.ml_per_liter);
            f("daily_cap", s.daily_cap_ml);
            f("p1_zero", s.p1_zero_psi);
            f("p2_zero", s.p2_zero_psi);
            f("idle_min", s.idle_minutes);
            f("strip", s.last_strip);
            f("mix_mode", s.mixer_mode);
            f("mix_dose_min", s.mixer_after_dose_min);
            f("mix_water_min", s.mixer_every_liters_min);
            f("mix_manual_min", s.mixer_manual_max_min);
            f("lvl_zero", s.level_zero_volts);
            f("lvl_range", s.level_range_cm);
            f("lvl_sensor", s.level_sensor_cm);
            f("lvl_intake", s.level_intake_cm);
            f("lvl_full", s.level_full_cm);
            f("lvl_l_per_cm", s.level_liters_per_cm);
            f("clog_r0", s.clog_r0);
            f("clog_r0_lpm", s.clog_r0_lpm);
            f("clog_r0_day", s.clog_r0_day);
            f("spares", s.spares);
            f("last_cart", s.last_cartridge);
            f("last_cart_day", s.last_cartridge_day);
            // Los sensores que ya se vieron (core/detect.h)
            f("has_pressure", s.has_pressure);
            f("has_level", s.has_level);
            f("has_ldr", s.has_ldr);
            return;
        case SaveGroup::counters:
            f("today", s.today);
            f("weekday", s.weekday);
            f("day", s.day);
            f("month", s.month);
            f("year", s.year);
            f("hour", s.hour);
            f("minute", s.minute);
            f("liters_today", s.liters_today);
            f("since_dose", s.liters_since_dose);
            f("since_mix", s.liters_since_mix);
            f("filters", s.filters);
            f("doses", s.recent_doses);
            f("dose_count", s.recent_dose_count);
            f("events", s.recent_events);
            f("event_count", s.recent_event_count);
            f("event_seq", s.event_seq);
            f("mix_on", s.mixer_on);
            f("mix_left", s.mixer_minutes_left);
            f("mix_run_min", s.mixer_run_minutes);
            f("mix_pct", s.mixer_progress_pct);
            f("mix_started", s.mixer_started_at);
            f("mix_pend", s.mixer_pending_min);
            f("mix_pend_why", s.mixer_pending_reason);
            f("mix_fault", s.mixer_fault);
            f("mix_today_min", s.mixer_today_minutes);
            f("mix_today_runs", s.mixer_today_runs);
            f("mix_runs", s.mixer_runs);
            f("mix_run_count", s.mixer_run_count);
            f("service_left", s.service_minutes_left);
            f("clog_r", s.clog_r);
            f("clog_r_lpm", s.clog_r_lpm);
            f("clog_r_day", s.clog_r_day);
            f("clog_r_at", s.clog_r_at);
            f("clog_state", s.clog_state);
            f("clog_today_n", s.clog_today_count);
            return;
        case SaveGroup::history:
            f("days", s.days);
            f("hours", s.hours);
            f("flow_24h", s.flow_24h);
            f("flow_2h", s.flow_2h);
            f("p1_24h", s.p1_24h);
            f("p2_24h", s.p2_24h);
            f("orp_7d", s.orp_7d);
            f("clog_days", s.clog_days);
            return;
    }
}

template <typename T>
bool in(T v, T lo, T hi) {
    return v >= lo && v <= hi;
}

bool finite_in(float v, float lo, float hi) { return std::isfinite(v) && v >= lo && v <= hi; }

template <typename E>
bool enum_le(E v, E last) {
    return static_cast<uint8_t>(v) <= static_cast<uint8_t>(last);
}

// Lo leído de la flash no se cree a ciegas: lo que no tiene sentido regresa a
// su valor de fábrica, y una lista con algo raro se vacía
void sanitize(SystemState& s) {
    static const SystemState factory{};
    const SystemState& f = factory;

    // Ajustes
    if (!in<uint16_t>(s.flow_pulses_per_liter, 100, 2000)) s.flow_pulses_per_liter = f.flow_pulses_per_liter;
    if (!finite_in(s.ml_per_liter, 0.001f, 1.0f)) s.ml_per_liter = f.ml_per_liter;
    if (!finite_in(s.p1_zero_psi, -30.0f, 30.0f)) s.p1_zero_psi = f.p1_zero_psi;
    if (!finite_in(s.p2_zero_psi, -30.0f, 30.0f)) s.p2_zero_psi = f.p2_zero_psi;
    if (!in<uint8_t>(s.idle_minutes, 1, 10)) s.idle_minutes = f.idle_minutes;
    if (!enum_le(s.mixer_mode, MixerMode::automatic)) s.mixer_mode = f.mixer_mode;
    if (!in<uint8_t>(s.mixer_after_dose_min, 1, 99)) s.mixer_after_dose_min = f.mixer_after_dose_min;
    if (!in<uint8_t>(s.mixer_every_liters_min, 1, 99)) s.mixer_every_liters_min = f.mixer_every_liters_min;
    // Documento: la bomba R nunca más de 60 minutos seguidos
    if (!in<uint8_t>(s.mixer_manual_max_min, 1, 60)) s.mixer_manual_max_min = f.mixer_manual_max_min;
    // De 3 a 5 mA: el 0.132 que guardaba el firmware de 33 Ω regresa a fábrica
    if (!finite_in(s.level_zero_volts, kLevelZeroMinVolts, kLevelZeroMaxVolts)) s.level_zero_volts = f.level_zero_volts;
    // 0 es sin medir; lo demás, lo que la medición se cree
    if (s.level_liters_per_cm != 0.0f &&
        !finite_in(s.level_liters_per_cm, kLevelLitersPerCmMin, kLevelLitersPerCmMax)) {
        s.level_liters_per_cm = f.level_liters_per_cm;
    }
    if (!in<uint16_t>(s.level_range_cm, 10, 2000)) s.level_range_cm = f.level_range_cm;
    if (s.level_sensor_cm > 50) s.level_sensor_cm = f.level_sensor_cm;
    if (static_cast<uint8_t>(s.last_cartridge) >= kFilterCount) s.last_cartridge = f.last_cartridge;

    // Reloj: si no cuadra, se espera a la hora del NTP como la primera vez
    if (!in<uint8_t>(s.month, 1, 12) || !in<uint8_t>(s.day, 1, 31) || s.hour > 23 || s.minute > 59 ||
        s.weekday > 6) {
        s.today = 0;
        s.year = f.year;
        s.month = f.month;
        s.day = f.day;
        s.hour = f.hour;
        s.minute = f.minute;
        s.weekday = f.weekday;
    }

    // Listas
    if (s.recent_event_count > kEventCount) s.recent_event_count = 0;
    for (uint8_t i = 0; i < s.recent_event_count; ++i) {
        if (!enum_le(s.recent_events[i].kind, EventKind::clog_alarm)) s.recent_event_count = 0;
    }
    if (s.recent_dose_count > s.recent_doses.size()) s.recent_dose_count = 0;
    // R y R0 en psi por L/min: arriba de 1,000 sería un caudal de casi 0
    if (!finite_in(s.clog_r0, 0.0f, 1000.0f) || !finite_in(s.clog_r0_lpm, 0.0f, 100.0f)) {
        s.clog_r0 = f.clog_r0;
        s.clog_r0_lpm = f.clog_r0_lpm;
    }
    if (!finite_in(s.clog_r, 0.0f, 1000.0f) || !finite_in(s.clog_r_lpm, 0.0f, 100.0f)) {
        s.clog_r = f.clog_r;
        s.clog_r_lpm = f.clog_r_lpm;
    }
    if (!enum_le(s.clog_state, ClogState::clogged)) s.clog_state = f.clog_state;
    if (s.mixer_run_count > s.mixer_runs.size()) s.mixer_run_count = 0;
    for (uint8_t i = 0; i < s.mixer_run_count; ++i) {
        if (!enum_le(s.mixer_runs[i].reason, MixerReason::manual)) s.mixer_run_count = 0;
    }
    for (float& v : s.flow_24h) {
        if (!finite_in(v, 0.0f, 1000.0f)) v = 0.0f;
    }
    for (float& v : s.flow_2h) {
        if (!finite_in(v, 0.0f, 1000.0f)) v = 0.0f;
    }

    // Bomba R
    if (!enum_le(s.mixer_fault, MixerFault::stuck)) s.mixer_fault = MixerFault::none;
    if (!enum_le(s.mixer_pending_reason, MixerReason::manual)) s.mixer_pending_reason = MixerReason::water;
    if (s.mixer_pending_min > 99) s.mixer_pending_min = 0;
    if (s.mixer_minutes_left > 99 || s.mixer_run_minutes > 99 || s.mixer_run_count == 0) {
        s.mixer_on = false;
        s.mixer_minutes_left = 0;
    }
    if (s.service_minutes_left > kServiceMinutes) s.service_minutes_left = 0;
}

}  // namespace

int load(SystemState& s, Store& store) {
    int found = 0;
    for (SaveGroup group : kSaveGroups) {
        each(group, s, [&store, &found](const char* key, auto& field) {
            using T = std::remove_reference_t<decltype(field)>;
            static_assert(std::is_trivially_copyable_v<T>, "solo campos que se copian byte por byte");
            if constexpr (std::is_same_v<T, bool>) {
                // Un bool con otro byte que 0 o 1 no es válido: se lee el byte
                uint8_t byte = 0;
                static_assert(sizeof(bool) == sizeof byte, "un bool ocupa un byte");
                if (store.read(key, &byte, sizeof byte)) {
                    field = byte == 1;
                    ++found;
                }
            } else {
                T value{};
                if (store.read(key, &value, sizeof value)) {
                    field = value;
                    ++found;
                }
            }
        });
    }
    sanitize(s);
    return found;
}

int save(const SystemState& s, Store& store, SaveGroup group) {
    int written = 0;
    bool failed = false;
    each(group, s, [&](const char* key, const auto& field) {
        using T = std::remove_cv_t<std::remove_reference_t<decltype(field)>>;
        static_assert(std::is_trivially_copyable_v<T>, "solo campos que se copian byte por byte");
        unsigned char saved[sizeof(T)];
        if (store.read(key, saved, sizeof saved) && std::memcmp(saved, &field, sizeof saved) == 0) return;
        if (store.write(key, &field, sizeof(T))) {
            ++written;
        } else {
            failed = true;
        }
    });
    return failed ? -1 : written;
}

}  // namespace core
