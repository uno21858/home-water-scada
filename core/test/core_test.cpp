// Pruebas de core en la compu, sin pantalla ni hardware:
//   ctest --test-dir build      (o ./build/core_test)
// Cada CHECK que falle dice el renglón; al final, cuántas fallaron.

#include <cmath>
#include <cstdio>

#include "events.h"
#include "level.h"
#include "mixer.h"
#include "monitor.h"
#include "system_state.h"
#include "water.h"

namespace {

int failures = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            std::printf("FALLA %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                \
        }                                                              \
    } while (0)

// Una casa con bomba R en Auto y sin vueltas
core::SystemState house() {
    core::SystemState s;
    s.has_mixer = true;
    s.has_pressure = true;
    s.p1_psi = 42;
    s.p2_psi = 38;
    s.flow_lpm = 12.4f;
    s.hour = 8;
    return s;
}

void minutes(core::SystemState& s, int n) {
    for (int i = 0; i < n; ++i) core::tick_minute(s);
}

void auto_by_liters() {
    core::SystemState s = house();
    core::count_liters(s, 999);
    CHECK(!s.mixer_on);
    core::count_liters(s, 1);  // llegan los 1,000 L
    CHECK(s.mixer_on);
    CHECK(s.mixer_runs[0].reason == core::MixerReason::water);
    CHECK(s.mixer_minutes_left == s.mixer_every_liters_min);
    CHECK(s.liters_since_mix == 0);
    minutes(s, s.mixer_every_liters_min);
    CHECK(!s.mixer_on);
    CHECK(s.mixer_today_minutes == s.mixer_every_liters_min);
}

void dose_mixes() {
    core::SystemState s = house();
    core::count_liters(s, 400);
    core::register_dose(s, 40, false);
    CHECK(s.liters_since_dose == 0);
    CHECK(s.mixer_on);
    CHECK(s.mixer_runs[0].reason == core::MixerReason::dose);
    CHECK(s.mixer_minutes_left == 45);
    CHECK(s.recent_events[0].kind == core::EventKind::dose);
    // Una dosis con la bomba Apagada no la prende
    core::SystemState off = house();
    core::mixer_set_mode(off, core::MixerMode::off);
    core::register_dose(off, 40, false);
    CHECK(!off.mixer_on);
}

void off_never_runs() {
    core::SystemState s = house();
    core::mixer_set_mode(s, core::MixerMode::off);
    core::count_liters(s, 5000);
    minutes(s, 10);
    CHECK(!s.mixer_on);
}

void no_power_waits() {
    core::SystemState s = house();
    core::register_dose(s, 40, false);
    minutes(s, 5);
    core::mixer_report_fault(s, core::MixerFault::no_power);
    CHECK(!s.mixer_on);
    CHECK(s.mixer_pending_min == 45);  // la vuelta de la dosis queda pendiente
    CHECK(core::has_open_event(s, core::EventKind::mixer_fault));
    core::count_liters(s, 2000);  // sin 12 V no arranca aunque entre agua
    minutes(s, 30);
    CHECK(!s.mixer_on);
    core::mixer_power_back(s);
    CHECK(s.mixer_on);
    CHECK(s.mixer_runs[0].reason == core::MixerReason::dose);
    CHECK(!core::has_open_event(s, core::EventKind::mixer_fault));
}

void dry_retries_after_water() {
    core::SystemState s = house();
    core::count_liters(s, 1000);
    core::mixer_report_fault(s, core::MixerFault::dry);
    CHECK(!s.mixer_on);
    core::count_liters(s, 500);
    CHECK(!s.mixer_on);
    CHECK(s.mixer_fault == core::MixerFault::dry);
    core::count_liters(s, 500);  // ya entraron 1,000 L desde que no pudo
    CHECK(s.mixer_fault == core::MixerFault::none);
    CHECK(s.mixer_on);
}

void stuck_needs_a_person() {
    core::SystemState s = house();
    core::count_liters(s, 1000);
    core::mixer_report_fault(s, core::MixerFault::stuck);
    CHECK(s.mixer_pending_min == 0);
    core::count_liters(s, 5000);
    minutes(s, 60);
    CHECK(!s.mixer_on);
    // La revisaron y la prenden a mano: corre 60 min y queda Apagada
    core::mixer_set_mode(s, core::MixerMode::on);
    CHECK(s.mixer_fault == core::MixerFault::none);
    CHECK(s.mixer_on);
    minutes(s, s.mixer_manual_max_min);
    CHECK(!s.mixer_on);
    CHECK(s.mixer_mode == core::MixerMode::off);
}

void supply_and_service() {
    core::SystemState s = house();
    s.flow_lpm = 0.0f;
    CHECK(!core::supply_lost(s));  // sin flujo pero con presión: cisterna llena
    s.p1_psi = s.p2_psi = 0;
    CHECK(core::supply_lost(s));
    core::tick_minute(s);
    CHECK(core::has_open_event(s, core::EventKind::no_supply));
    CHECK(core::overall_level(s) == core::Level::warning);
    s.p1_psi = 40;
    core::tick_minute(s);
    CHECK(!core::has_open_event(s, core::EventKind::no_supply));

    // Cambio de cartuchos: V1 cerrada no avisa ni se anota
    core::service_start(s);
    s.p1_psi = s.p2_psi = 0;
    CHECK(!core::supply_lost(s));
    CHECK(core::overall_level(s) == core::Level::ok);
    minutes(s, core::kServiceMinutes - 1);
    CHECK(core::in_service(s));
    CHECK(!core::has_open_event(s, core::EventKind::no_supply));
    core::tick_minute(s);  // se acabó el tiempo: vuelve a avisar
    CHECK(!core::in_service(s));
    CHECK(core::has_open_event(s, core::EventKind::no_supply));
}

void drop_against_baseline() {
    core::SystemState s = house();
    for (size_t q = 0; q < s.drop_new_tenths.size(); ++q) s.drop_new_tenths[q] = static_cast<uint8_t>(q * 2);  // 0.2 psi por L/min
    s.flow_lpm = 10.0f;  // nuevos: 2.0 psi
    s.p2_psi = s.p1_psi - 6;
    CHECK(core::drop_level(s) == core::Level::ok);  // subió 4
    s.p2_psi = s.p1_psi - 7;
    CHECK(core::drop_level(s) == core::Level::warning);  // subió 5
    s.p2_psi = s.p1_psi - 12;
    CHECK(core::drop_level(s) == core::Level::alarm);  // subió 10
}

void level_from_the_bottom() {
    core::SystemState s = house();
    s.has_level = true;
    s.level_range_cm = 200;
    s.level_sensor_cm = 3;
    s.level_intake_cm = 45;
    s.level_volts = 0.140f;
    core::level_zero(s);  // en el aire marca 0.140 V y no los 0.132 de fábrica
    CHECK(std::fabs(core::level_cm(s) - 3.0f) < 0.01f);  // en el aire: solo la altura del sensor
    CHECK(core::level_pct(s) == -1);                      // falta marcar llena
    s.level_volts = 0.140f + core::kLevelSpanVolts * 0.9f;  // 180 cm encima
    CHECK(std::fabs(core::level_cm(s) - 183.0f) < 0.01f);
    core::level_mark_full(s);
    CHECK(s.level_full_cm == 183);
    CHECK(core::level_pct(s) == 100);
    s.level_volts = 0.140f + core::kLevelSpanVolts * (114.0f - 3.0f) / 200.0f;  // a 114 cm: a la mitad útil
    CHECK(core::level_pct(s) == 50);
    s.level_volts = 0.140f;  // abajo de la rejilla: 0 %, no negativo
    CHECK(core::level_pct(s) == 0);
}

}  // namespace

int main() {
    auto_by_liters();
    dose_mixes();
    off_never_runs();
    no_power_waits();
    dry_retries_after_water();
    stuck_needs_a_person();
    supply_and_service();
    drop_against_baseline();
    level_from_the_bottom();
    if (failures == 0) std::printf("core: todo bien\n");
    return failures == 0 ? 0 : 1;
}
