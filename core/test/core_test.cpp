// Pruebas de core en la compu, sin pantalla ni hardware:
//   ctest --test-dir build      (o ./build/core_test)
// Cada CHECK que falle dice el renglón; al final, cuántas fallaron.

#include <cmath>
#include <cstdio>
#include <cstring>

#include "calendar.h"
#include "events.h"
#include "level.h"
#include "mixing.h"
#include "monitor.h"
#include "system_state.h"
#include "telemetry.h"
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

void mixer_waits_for_water() {
    core::SystemState s = house();
    s.has_level = true;
    s.level_range_cm = 200;
    s.level_sensor_cm = 3;
    s.level_intake_cm = 60;
    s.level_full_cm = 183;
    auto set_level = [&s](float cm) {
        s.level_volts = s.level_zero_volts + (cm - 3.0f) / 200.0f * core::kLevelSpanVolts;
    };
    set_level(58.0f);  // abajo del mínimo: 0 %
    CHECK(core::mixer_low_water(s));
    core::register_dose(s, 40, false);
    CHECK(!s.mixer_on);
    CHECK(s.mixer_pending_min == 45);
    minutes(s, 30);
    CHECK(!s.mixer_on);
    set_level(90.0f);  // regresó el agua
    core::tick_minute(s);
    CHECK(s.mixer_on);
    CHECK(s.mixer_runs[0].reason == core::MixerReason::dose);
    // Andando, la cisterna baja al mínimo: se apaga y queda pendiente
    minutes(s, 10);
    set_level(59.0f);
    core::tick_minute(s);
    CHECK(!s.mixer_on);
    CHECK(s.mixer_pending_min == 45);
}

void calendar_day_change() {
    core::SystemState s = house();
    s.today = 1000;
    s.year = 2026;
    s.month = 10;
    s.day = 3;
    s.hour = 23;
    s.minute = 58;
    s.liters_today = 1860;
    s.mixer_today_minutes = 72;
    s.days.back().liters = 1860;
    core::set_time(s, {2026, 10, 3, 23, 59});  // misma hora: no cierra nada
    CHECK(s.liters_today == 1860);
    core::set_time(s, {2026, 10, 4, 0, 0});
    CHECK(s.today == 1001);
    CHECK(s.weekday == 0);  // domingo 4 de octubre de 2026
    CHECK(s.liters_today == 0);
    CHECK(s.mixer_today_minutes == 0);
    CHECK(s.days[core::kHistoryDays - 2].liters == 1860);  // ayer
    CHECK(s.days.back().liters == 0);
    // Apagada tres días: el historial se recorre tres
    core::set_time(s, {2026, 10, 7, 9, 30});
    CHECK(s.today == 1004);
    CHECK(s.days[core::kHistoryDays - 5].liters == 1860);
    // El reloj para atrás no cierra nada
    core::set_time(s, {2026, 10, 7, 8, 0});
    CHECK(s.today == 1004);
    CHECK(core::days_from_civil(1970, 1, 1) == 0);
    CHECK(core::days_from_civil(2026, 12, 31) + 1 == core::days_from_civil(2027, 1, 1));
    CHECK(core::days_from_civil(2028, 2, 28) + 2 == core::days_from_civil(2028, 3, 1));  // bisiesto
}

void calendar_hour_samples() {
    core::SystemState s = house();
    s.today = 10;
    s.hour = 8;
    s.minute = 59;
    s.p1_psi = 47;
    core::set_time(s, {2026, 1, 1, 9, 0});
    CHECK(s.p1_24h.back() == 47);
    CHECK(s.hours[9].p1_psi == 47);
    core::set_flow(s, 11.0f);
    CHECK(s.flow_2h.back() == 11.0f);
    core::tick_minute(s);  // la muestra de este minuto entra a la de 2 h
    CHECK(s.flow_2h[s.flow_2h.size() - 2] == 11.0f);
}

void drop_notices() {
    core::SystemState s = house();  // sin línea base: lo que subió es la caída completa
    auto open = [&s](core::EventKind kind) { return core::has_open_event(s, kind); };
    s.p2_psi = s.p1_psi - 5;
    core::check_drop(s);
    CHECK(open(core::EventKind::drop_warning));
    s.p2_psi = s.p1_psi - 4;  // dentro del margen: sigue en Revisar
    core::check_drop(s);
    CHECK(open(core::EventKind::drop_warning));
    s.p2_psi = s.p1_psi - 3;
    core::check_drop(s);
    CHECK(!open(core::EventKind::drop_warning));
    s.p2_psi = s.p1_psi - 10;
    core::check_drop(s);
    CHECK(open(core::EventKind::drop_alarm));
    s.p2_psi = s.p1_psi - 9;  // margen de Tapados
    core::check_drop(s);
    CHECK(open(core::EventKind::drop_alarm));
    s.p2_psi = s.p1_psi - 6;  // baja a Revisar
    core::check_drop(s);
    CHECK(!open(core::EventKind::drop_alarm));
    CHECK(open(core::EventKind::drop_warning));
    // Sin agua pasando la caída no dice nada: no se quita
    s.flow_lpm = 0.0f;
    s.p2_psi = s.p1_psi;
    core::check_drop(s);
    CHECK(open(core::EventKind::drop_warning));
}

// Telemetría: el renglón tiene que salir exacto, el servidor no adivina
bool line_is(const core::Record& r, const char* expected) {
    char line[256];
    const std::size_t n = core::format_record(r, line, sizeof line);
    if (n != std::strlen(expected) || std::strcmp(line, expected) != 0) {
        std::printf("  sale:   %s\n  espera: %s\n", line, expected);
        return false;
    }
    return true;
}

void telemetry_lines() {
    core::Record r;
    r.time = 1760000000;
    CHECK(line_is(r, "water flow_lpm=0.00,liters_today=0i 1760000000000000000"));
    r.flow_lpm = 12.4f;
    r.liters_today = 1860;
    r.pressure = true;
    r.p1_psi = 42;
    r.p2_psi = 38;
    r.drop_rise = 1.4f;
    r.level = true;
    r.level_cm = 150.0f;
    r.level_pct = 73;
    r.mixer = true;
    r.mixer_on = true;
    r.mixer_amps = 1.58f;
    r.orp = true;
    r.orp_mv = 650;
    CHECK(line_is(r,
                  "water flow_lpm=12.40,liters_today=1860i,p1_psi=42i,p2_psi=38i,drop_rise=1.4,"
                  "level_cm=150.0,level_pct=73i,mixer_on=1i,mixer_amps=1.58,orp_mv=650i 1760000000000000000"));
    // Sin marcar llena no va el %; un sensor raro (NaN) sale como 0
    r.pressure = r.mixer = r.orp = false;
    r.level_pct = -1;
    r.flow_lpm = NAN;
    CHECK(line_is(r, "water flow_lpm=0.00,liters_today=1860i,level_cm=150.0 1760000000000000000"));

    core::Record e;
    e.type = core::Record::Type::event;
    e.time = 1760000000;
    e.event = core::EventKind::dose;
    e.value = 185;
    CHECK(line_is(e, "water_event,kind=dose value=185i 1760000000000000000"));
    e.event = core::EventKind::no_supply;
    e.value = 0;
    CHECK(line_is(e, "water_event,kind=no_supply value=0i 1760000000000000000"));

    // Si no cabe no sale medio renglón
    char small[20];
    CHECK(core::format_record(e, small, sizeof small) == 0);
    CHECK(small[0] == '\0');
}

void telemetry_events_once() {
    core::SystemState s = house();
    core::register_dose(s, 40, false);  // ya estaba antes de prender
    core::Record buffer[16];
    core::Outbox out(buffer, 16);
    out.last_event_seq = s.event_seq;  // como al arrancar la 7B

    core::queue_reading(s, out, 1000);
    CHECK(out.size() == 1);  // solo la lectura: el evento viejo no se repite
    CHECK(out.at(0).type == core::Record::Type::reading);

    core::register_dose(s, 185, false);
    core::register_filter(s, core::Filter::pp20);
    core::queue_reading(s, out, 1010);
    CHECK(out.size() == 4);  // lectura, dosis y cartucho, del más viejo al más nuevo
    CHECK(out.at(2).type == core::Record::Type::event);
    CHECK(out.at(2).event == core::EventKind::dose);
    CHECK(out.at(2).value == 185);
    CHECK(out.at(3).event == core::EventKind::cartridge);
    CHECK(s.pending_uploads == 4);

    core::queue_reading(s, out, 1020);
    CHECK(out.size() == 5);  // otra lectura, sin repetir eventos
    CHECK(out.at(4).type == core::Record::Type::reading);
    CHECK(out.at(4).time == 1020);
}

void telemetry_full_queue() {
    core::Record buffer[3];
    core::Outbox out(buffer, 3);
    for (uint32_t t = 1; t <= 5; ++t) {
        core::Record r;
        r.time = t;
        out.push(r);
    }
    CHECK(out.size() == 3);  // se quedan los 3 más nuevos
    CHECK(out.dropped() == 2);
    CHECK(out.at(0).time == 3);
    CHECK(out.at(2).time == 5);
    out.pop(2);
    CHECK(out.size() == 1);
    CHECK(out.at(0).time == 5);
    core::Record r;
    r.time = 6;
    out.push(r);  // da la vuelta al arreglo
    CHECK(out.at(1).time == 6);
    out.pop(10);  // pedir de más no rompe nada
    CHECK(out.size() == 0);
}

void telemetry_batch() {
    core::SystemState s = house();
    s.minute = 30;
    core::Record buffer[8];
    core::Outbox out(buffer, 8);
    char body[512];
    std::size_t records = 99;
    CHECK(core::format_batch(out, body, sizeof body, &records) == 0);  // cola vacía
    CHECK(records == 0);
    CHECK(body[0] == '\0');

    for (uint32_t t = 0; t < 3; ++t) core::queue_reading(s, out, 1760000000 + t * 10);
    const std::size_t bytes = core::format_batch(out, body, sizeof body, &records);
    CHECK(records == 3);
    CHECK(bytes == std::strlen(body));
    CHECK(body[bytes - 1] == '\n');
    int lines = 0;
    for (std::size_t i = 0; i < bytes; ++i) lines += body[i] == '\n';
    CHECK(lines == 3);

    // Un body chico se lleva solo los más viejos que quepan
    char line[256];
    const std::size_t one = core::format_record(out.at(0), line, sizeof line);
    char tight[256];
    CHECK(core::format_batch(out, tight, one + 2, &records) == one + 1);
    CHECK(records == 1);
    CHECK(core::format_batch(out, tight, one + 1, &records) == 0);  // no cabe el '\0'
    CHECK(records == 0);

    core::mark_sent(s, out, 1);
    CHECK(out.size() == 2);
    CHECK(s.pending_uploads == 2);
    CHECK(s.last_upload_at == 8 * 60 + 30);
    CHECK(out.at(0).time == 1760000010);
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
    mixer_waits_for_water();
    calendar_day_change();
    calendar_hour_samples();
    drop_notices();
    telemetry_lines();
    telemetry_events_once();
    telemetry_full_queue();
    telemetry_batch();
    if (failures == 0) std::printf("core: todo bien\n");
    return failures == 0 ? 0 : 1;
}
