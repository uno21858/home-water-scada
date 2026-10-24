// Pruebas de core en la compu, sin pantalla ni hardware:
//   ctest --test-dir build      (o ./build/core_test)
// Cada CHECK que falle dice el renglón; al final, cuántas fallaron.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "calendar.h"
#include "detect.h"
#include "events.h"
#include "level.h"
#include "mixing.h"
#include "monitor.h"
#include "persist.h"
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

void alarms_raise_and_acknowledge() {
    core::SystemState s = house();
    const auto kind = core::AlarmKind::no_flow;
    core::alarm_raise(s, kind);
    CHECK(s.alarms[0].active && !s.alarms[0].acknowledged);
    CHECK(s.recent_events[0].kind == core::EventKind::alarm);
    const uint8_t count = s.recent_event_count;
    core::alarm_raise(s, kind);  // ya estaba: no se anota otra vez
    CHECK(s.recent_event_count == count);
    s.minute = 5;
    core::alarm_acknowledge(s, kind);
    CHECK(s.alarms[0].acknowledged && s.alarms[0].active);
    CHECK(s.recent_events[0].acknowledged_at == 8 * 60 + 5);
    s.minute = 9;
    core::alarm_clear(s, kind);
    CHECK(!s.alarms[0].active);
    CHECK(s.recent_events[0].resolved_at == 8 * 60 + 9);
}

void pressure_with_zero() {
    core::SystemState s;
    core::set_pressures(s, 0.5f + 42.0f / core::kPsiPerVolt, 0.5f + 38.0f / core::kPsiPerVolt);
    CHECK(s.p1_psi == 42 && s.p2_psi == 38);
    // V1 cerrada: P1 dio 0.448 V al aire (documento), abajo del 0.5 nominal
    core::set_pressures(s, 0.448f, 0.4495f);
    CHECK(s.p1_psi == 0);  // abajo de 0 queda en 0
    core::pressure_zero(s);
    CHECK(std::fabs(s.p1_zero_psi + 3.016f) < 0.01f);
    core::set_pressures(s, 0.448f + 42.0f / core::kPsiPerVolt, 0.4495f + 38.0f / core::kPsiPerVolt);
    CHECK(s.p1_psi == 42 && s.p2_psi == 38);
    core::set_pressures(s, NAN, 0.0f);  // el ADS1115 no contestó bien
    CHECK(s.p1_psi == 0);
}

void mixer_resumes_after_restart() {
    core::SystemState s = house();
    core::register_dose(s, 40, false);
    minutes(s, 10);
    CHECK(s.mixer_on && s.mixer_minutes_left == 35);
    core::mixer_after_restart(s);  // se fue la luz o llegó una OTA
    CHECK(!s.mixer_on);
    CHECK(s.mixer_pending_min == 35);
    CHECK(s.mixer_pending_reason == core::MixerReason::dose);
    core::tick_minute(s);  // al minuto sigue lo que faltaba
    CHECK(s.mixer_on && s.mixer_minutes_left == 35);
}

void offline_event() {
    core::SystemState s = house();
    s.today = 10;
    s.hour = 0;
    s.minute = 1;
    for (int i = 0; i < 100; ++i) core::network_tick(s, false);
    CHECK(!s.wifi_ok);
    core::network_tick(s, true);  // caída corta: no se anota
    CHECK(s.wifi_ok && s.recent_event_count == 0);
    for (int i = 0; i < 5 * 60; ++i) core::network_tick(s, false);
    core::network_tick(s, true);
    CHECK(s.recent_event_count == 1);
    const core::Event& e = s.recent_events[0];
    CHECK(e.kind == core::EventKind::offline && e.value == 5);
    CHECK(e.day == 9 && e.at == 23 * 60 + 56);  // se cayó a las 23:56 de ayer
    CHECK(e.resolved_at == 1);
    CHECK(s.offline_seconds == 0);
}

// La NVS de mentira: un mapa de llave a bytes
struct MemoryStore : core::Store {
    std::map<std::string, std::vector<unsigned char>> data;
    int writes = 0;
    bool broken = false;

    bool read(const char* key, void* out, std::size_t size) override {
        const auto it = data.find(key);
        if (it == data.end() || it->second.size() != size) return false;
        std::memcpy(out, it->second.data(), size);
        return true;
    }
    bool write(const char* key, const void* in, std::size_t size) override {
        if (broken) return false;
        ++writes;
        const auto* bytes = static_cast<const unsigned char*>(in);
        data[key].assign(bytes, bytes + size);
        return true;
    }
    template <typename T>
    void put(const char* key, const T& value) {
        const auto* bytes = reinterpret_cast<const unsigned char*>(&value);
        data[key].assign(bytes, bytes + sizeof value);
    }
};

void persist_round_trip() {
    core::SystemState s = house();
    s.today = 1000;
    s.flow_pulses_per_liter = 387;
    s.p1_zero_psi = -3.0f;
    s.level_full_cm = 183;
    core::count_liters(s, 250);
    core::register_dose(s, 40, false);
    core::register_filter(s, core::Filter::pp20);
    s.days.back().liters = 1860;
    s.flow_24h[10] = 7.5f;

    MemoryStore store;
    for (core::SaveGroup g : core::kSaveGroups) CHECK(core::save(s, store, g) > 0);
    CHECK(store.data.size() == static_cast<std::size_t>(store.writes));  // ninguna llave repetida
    for (const auto& entry : store.data) CHECK(entry.first.size() <= 15);  // lo más que deja la NVS

    // Sin cambios no se gasta la flash
    for (core::SaveGroup g : core::kSaveGroups) CHECK(core::save(s, store, g) == 0);
    s.liters_today += 5;
    CHECK(core::save(s, store, core::SaveGroup::counters) == 1);

    core::SystemState back;
    CHECK(core::load(back, store) == static_cast<int>(store.data.size()));  // leyó todas
    CHECK(back.flow_pulses_per_liter == 387);
    CHECK(back.p1_zero_psi == -3.0f);
    CHECK(back.level_full_cm == 183);
    CHECK(back.today == 1000 && back.hour == 8);
    CHECK(back.liters_today == s.liters_today);
    CHECK(back.liters_since_dose == 0);
    CHECK(back.recent_event_count == s.recent_event_count);
    CHECK(back.recent_events[0].kind == core::EventKind::cartridge);
    CHECK(back.event_seq == s.event_seq);
    CHECK(back.filters[1].since_day == 1000);
    CHECK(back.mixer_on && back.mixer_minutes_left == 45);
    CHECK(back.days.back().liters == 1860);
    CHECK(back.flow_24h[10] == 7.5f);
}

void persist_ignores_bad_data() {
    MemoryStore store;
    store.put("flow_k", uint32_t{387});      // mide otra cosa: de otra versión
    store.put("lvl_range", uint16_t{0});     // no tiene sentido
    store.put("ml_per_l", NAN);
    store.put("event_count", uint8_t{200});
    store.put("month", uint8_t{13});
    store.put("today", uint32_t{1000});
    store.put("mix_on", true);               // prendida, pero sin vueltas en la lista
    store.put("has_level", uint8_t{7});      // un bool que no es 0 ni 1
    core::SystemState s;
    core::load(s, store);
    CHECK(s.flow_pulses_per_liter == core::kFactoryPulsesPerLiter);
    CHECK(s.level_range_cm == 200);
    CHECK(s.ml_per_liter == 0.100f);
    CHECK(s.recent_event_count == 0);
    CHECK(s.today == 0);  // reloj raro: espera al NTP
    CHECK(!s.mixer_on);
    CHECK(!s.has_level);

    store.broken = true;  // la flash no deja escribir: se avisa
    CHECK(core::save(house(), store, core::SaveGroup::settings) == -1);
}

// Recién instalada la pantalla los cartuchos no tienen fecha: no cuentan
// los días desde 1970 (20,748 en la 7B) ni le ganan a los demás
void filters_without_date() {
    core::SystemState s;
    core::register_filter(s, core::Filter::pp20);  // antes del NTP: sigue sin fecha
    CHECK(!core::filter_dated(s, core::Filter::pp20));

    core::set_time(s, {2026, 10, 23, 23, 29});
    CHECK(s.today > 20000);
    CHECK(!core::filter_dated(s, core::Filter::pp10));
    CHECK(core::filter_days(s, core::Filter::pp10) == 0);
    CHECK(core::filter_wear_pct(s, core::Filter::pp10) == 0);

    core::register_filter(s, core::Filter::prefilter);
    CHECK(core::filter_dated(s, core::Filter::prefilter));
    core::set_time(s, {2026, 10, 31, 8, 0});
    CHECK(core::filter_days(s, core::Filter::prefilter) == 8);
    CHECK(core::filter_wear_pct(s, core::Filter::prefilter) == 114);  // 8 de 7 días
    CHECK(core::filter_wear_pct(s, core::Filter::pp10) == 0);
}

// Lo que se conecta aparece solo: 5 s seguidos con la lectura de un sensor
// conectado. Ya visto, sigue instalado aunque luego lea 0.
void sensors_detect() {
    core::SystemState s;
    core::Detection d;
    core::AnalogReadings r;
    r.ads_ok = true;  // el ADS1115 sin nada conectado: todo en 0 V
    for (int i = 0; i < 10; ++i) CHECK(!core::detect_sensors(s, d, r));
    CHECK(!s.has_pressure && !s.has_level && !s.has_ldr);

    r.p1_volts = 0.52f;     // P1 sin P2 todavía no cuenta
    r.level_volts = 0.132f; // el nivel en 4 mA
    for (int i = 0; i < 4; ++i) CHECK(!core::detect_sensors(s, d, r));
    CHECK(core::detect_sensors(s, d, r));  // al quinto segundo
    CHECK(s.has_level && !s.has_pressure);

    r.p2_volts = 0.51f;
    r.level_volts = 0.0f;
    core::detect_sensors(s, d, r);
    core::detect_sensors(s, d, r);
    r.p2_volts = 0.0f;  // se soltó un cable: vuelve a contar desde 0
    core::detect_sensors(s, d, r);
    r.p2_volts = 0.51f;
    for (int i = 0; i < 4; ++i) CHECK(!core::detect_sensors(s, d, r));
    CHECK(core::detect_sensors(s, d, r));
    CHECK(s.has_pressure && s.has_level);  // el nivel ya visto sigue aunque lea 0

    r.ads_ok = false;  // sin ADS1115 no cuenta nada
    r.ldr_volts = 0.88f;
    for (int i = 0; i < 10; ++i) CHECK(!core::detect_sensors(s, d, r));
    CHECK(!s.has_ldr);

    // Se guarda con los ajustes y regresa después de un reinicio
    MemoryStore store;
    CHECK(core::save(s, store, core::SaveGroup::settings) > 0);
    core::SystemState back;
    core::load(back, store);
    CHECK(back.has_pressure && back.has_level && !back.has_ldr);
}

// Un sensor instalado que se suelta: Sin señal a los 5 s, sin avisos falsos
void sensors_lose_signal() {
    core::SystemState s;
    s.has_pressure = s.has_level = true;
    s.level_full_cm = 183;
    s.level_intake_cm = 60;
    core::Detection d;
    core::AnalogReadings r;
    r.ads_ok = true;
    r.p1_volts = 0.5f;  // 0 psi: el SIAPA sin agua, pero el sensor sí contesta
    r.p2_volts = 0.5f;
    r.level_volts = 0.30f;
    for (int i = 0; i < 10; ++i) core::detect_sensors(s, d, r);
    CHECK(!s.no_signal.p1 && !s.no_signal.p2 && !s.no_signal.level);

    r.p1_volts = 0.0f;  // se soltó el cable de P1
    for (int i = 0; i < 4; ++i) core::detect_sensors(s, d, r);
    CHECK(!s.no_signal.p1);  // un brinco de 4 s no cuenta
    core::detect_sensors(s, d, r);
    CHECK(s.no_signal.p1 && !s.no_signal.p2);
    CHECK(s.recent_event_count == 1 && s.recent_events[0].kind == core::EventKind::no_signal);
    CHECK(s.recent_events[0].value == static_cast<uint16_t>(core::Sensor::p1));
    CHECK(!core::pressure_ok(s) && core::signal_lost(s));
    CHECK(core::overall_level(s) == core::Level::warning);

    // Sin flujo y P1 en 0 psi sería "Sin agua"; sin señal no se sabe
    core::set_pressures(s, r.p1_volts, r.p2_volts);
    s.flow_lpm = 0.0f;
    CHECK(!core::supply_lost(s));
    core::check_supply(s);
    CHECK(!core::has_open_event(s, core::EventKind::no_supply));

    // El ADS1115 deja de contestar: los tres sin señal, cada uno con su evento
    r.ads_ok = false;
    for (int i = 0; i < 5; ++i) core::detect_sensors(s, d, r);
    CHECK(s.no_signal.p1 && s.no_signal.p2 && s.no_signal.level);
    CHECK(s.recent_event_count == 3);
    CHECK(!core::level_ok(s) && !core::mixer_low_water(s));  // la bomba R no espera agua por eso

    // Regresa: 5 s de lectura buena y se cierran los tres
    r.ads_ok = true;
    r.p1_volts = 0.5f;
    for (int i = 0; i < 5; ++i) core::detect_sensors(s, d, r);
    CHECK(!s.no_signal.p1 && !s.no_signal.p2 && !s.no_signal.level);
    for (uint8_t i = 0; i < s.recent_event_count; ++i) CHECK(s.recent_events[i].resolved_at != core::kNoTime);
    CHECK(!core::signal_lost(s));

    // Sin el sensor instalado no hay Sin señal
    core::SystemState bare;
    core::Detection d2;
    r.ads_ok = false;
    for (int i = 0; i < 10; ++i) core::detect_sensors(bare, d2, r);
    CHECK(!bare.no_signal.p1 && !bare.no_signal.level && bare.recent_event_count == 0);

    // El evento nuevo sobrevive a un reinicio
    MemoryStore store;
    core::save(s, store, core::SaveGroup::history);
    core::save(s, store, core::SaveGroup::counters);
    core::SystemState back;
    core::load(back, store);
    CHECK(back.recent_event_count == 3 && back.recent_events[0].kind == core::EventKind::no_signal);
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
    alarms_raise_and_acknowledge();
    pressure_with_zero();
    mixer_resumes_after_restart();
    offline_event();
    persist_round_trip();
    persist_ignores_bad_data();
    filters_without_date();
    sensors_detect();
    sensors_lose_signal();
    if (failures == 0) std::printf("core: todo bien\n");
    return failures == 0 ? 0 : 1;
}
