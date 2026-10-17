#include "ui.h"

#include <array>
#include <cstdio>

#include "components/confirm_dialog.h"
#include "components/header.h"
#include "components/keypad_dialog.h"
#include "components/nav_bar.h"
#include "components/primitives.h"
#include "screens/alarm_screen.h"
#include "screens/bucket_screen.h"
#include "screens/cistern_screen.h"
#include "screens/diagnostics_screen.h"
#include "screens/dose_screen.h"
#include "screens/drop_screen.h"
#include "screens/events_screen.h"
#include "screens/filters_screen.h"
#include "screens/flow_screen.h"
#include "screens/flow_sensor_screen.h"
#include "screens/history_screen.h"
#include "screens/home_screen.h"
#include "screens/idle_screen.h"
#include "screens/injection_screen.h"
#include "screens/legend_screen.h"
#include "screens/mixer_screen.h"
#include "screens/overview_screen.h"
#include "screens/pressure_screen.h"
#include "screens/settings_screen.h"
#include "screens/spares_screen.h"
#include "screens/summary_screen.h"
#include "theme/format.h"
#include "theme/styles.h"

// Estructura (ver lvgl.md del DS): una sola screen con tres partes fijas.
// Encabezado arriba, contenido en medio y barra abajo. Cada pantalla es un
// contenedor dentro del contenido; cambiar de pantalla es esconder uno y
// mostrar otro, nada se recrea.

namespace ui {
namespace {

constexpr int32_t kHeaderHeight = 64;
constexpr int32_t kContentHeight = 456;  // de y 64 a y 520

// Cada pantalla: su encabezado y la pestaña que marca la barra.
// Mismo orden que el enum Screen. En el encabezado van título, atrás,
// texto del enlace, a dónde lleva y el "?"; {} es "no tiene".
struct Page {
    HeaderSpec header;
    Screen tab;
};
const std::array<Page, kScreenCount> kPages = {{
    {{"Tren de filtros", {}, "Casa", Screen::overview, Screen::legend}, Screen::home},
    {{"Resumen", {}, nullptr, {}, {}}, Screen::summary},
    {{"Flujo", {}, "Sensor F", Screen::flow_sensor, {}}, Screen::flow},
    {{"Cisterna", {}, "Bomba R", Screen::mixer, {}}, Screen::cistern},
    {{"Dosis de cloro", {}, "Inyección", Screen::injection, {}}, Screen::dose},
    {{"Historial", {}, "Alarmas", Screen::events, {}}, Screen::history},
    {{"Ajustes", {}, "Diagnóstico", Screen::diagnostics, {}}, Screen::settings},
    {{"Mezcla de la cisterna", Screen::cistern, nullptr, {}, {}}, Screen::cistern},
    {{"Sensor de flujo F", Screen::flow, nullptr, {}, {}}, Screen::flow},
    {{"Presión P1 y P2", Screen::home, nullptr, {}, {}}, Screen::home},
    {{"Caída del tren", Screen::home, nullptr, {}, {}}, Screen::home},
    {{"Cartuchos", Screen::home, nullptr, {}, {}}, Screen::home},
    {{"Inyección A y B", Screen::dose, nullptr, {}, {}}, Screen::dose},
    {{"Alarmas y eventos", Screen::history, nullptr, {}, {}}, Screen::history},
    {{"Diagnóstico", Screen::settings, nullptr, {}, {}}, Screen::settings},
    {{"Prueba de cubeta", Screen::settings, nullptr, {}, {}}, Screen::settings},
    {{"Símbolos del diagrama", Screen::home, nullptr, {}, {}}, Screen::home},
    {{"Panorama de la casa", Screen::home, nullptr, {}, {}}, Screen::home},
    {{"Repuestos en casa", Screen::settings, nullptr, {}, {}}, Screen::settings},
}};

Actions app_actions;
core::SystemState state;  // la última copia que llegó en update()
Header header;
NavBar nav;
ConfirmDialog dialog;
KeypadDialog keypad;
HomeScreen home;
SummaryScreen summary;
FlowScreen flow;
CisternScreen cistern;
MixerScreen mixer;
HistoryScreen history;
SettingsScreen settings;
IdleScreen idle;
AlarmScreen alarm;
FlowSensorScreen flow_sensor;
PressureScreen pressure;
DropScreen pressure_drop;
FiltersScreen filters;
InjectionScreen injection;
DoseScreen dose;
EventsScreen events;
DiagnosticsScreen diagnostics;
BucketScreen bucket;
LegendScreen legend;
OverviewScreen overview;
SparesScreen spares;
std::array<lv_obj_t*, kScreenCount> pages{};

size_t index_of(Screen screen) { return static_cast<size_t>(screen); }

// Lo de fases que no están instaladas no se muestra: ni su pestaña ni los
// enlaces del encabezado que llevan a ello
bool available(Screen screen) {
    switch (screen) {
        case Screen::cistern:  // sin la sonda solo quedaría el candado del ORP
            return state.has_orp;
        case Screen::mixer:
            return state.has_mixer;
        case Screen::pressure:
        case Screen::pressure_drop:
            return state.has_pressure;
        case Screen::injection:
            return state.has_pumps;
        default:
            return true;
    }
}

void on_dose_confirmed() {
    if (app_actions.dose_confirmed != nullptr) app_actions.dose_confirmed(core::dose_ml(state));
}

void on_dose_typed(uint32_t ml) {
    if (app_actions.dose_typed != nullptr) app_actions.dose_typed(ml);
}

void on_strip_checked(uint32_t tenths_mgl) {
    if (app_actions.strip_checked != nullptr) app_actions.strip_checked(tenths_mgl);
}

}  // namespace

void init(const core::SystemState& s, const Actions& a) {
    state = s;
    app_actions = a;

    // Sin tema de LVGL: todo el look sale de theme/
    lv_display_set_theme(lv_display_get_default(), nullptr);
    styles::init();

    lv_obj_t* screen = lv_screen_active();
    lv_obj_add_style(screen, &styles::screen, 0);
    lv_obj_set_scrollable(screen, false);

    lv_obj_t* content = box(screen, 0, kHeaderHeight, 1024, kContentHeight);
    pages[index_of(Screen::home)] = home.create(content);
    pages[index_of(Screen::summary)] = summary.create(content);
    pages[index_of(Screen::flow)] = flow.create(content);
    pages[index_of(Screen::cistern)] = cistern.create(content);
    pages[index_of(Screen::dose)] = dose.create(content);
    pages[index_of(Screen::history)] = history.create(content);
    pages[index_of(Screen::settings)] = settings.create(content);
    pages[index_of(Screen::mixer)] = mixer.create(content);
    pages[index_of(Screen::flow_sensor)] = flow_sensor.create(content);
    pages[index_of(Screen::pressure)] = pressure.create(content);
    pages[index_of(Screen::pressure_drop)] = pressure_drop.create(content);
    pages[index_of(Screen::filters)] = filters.create(content);
    pages[index_of(Screen::injection)] = injection.create(content);
    pages[index_of(Screen::events)] = events.create(content);
    pages[index_of(Screen::diagnostics)] = diagnostics.create(content);
    pages[index_of(Screen::bucket_test)] = bucket.create(content);
    pages[index_of(Screen::legend)] = legend.create(content);
    pages[index_of(Screen::overview)] = overview.create(content);
    pages[index_of(Screen::spares)] = spares.create(content);

    header.create(screen);
    nav.create(screen);
    dialog.create(lv_layer_top());
    keypad.create(lv_layer_top());
    idle.create(lv_layer_top(), [] { show(Screen::home); });  // encima de los diálogos
    alarm.create(lv_layer_top());                                // y la alarma encima de todo

    update(s);
    show(Screen::home);  // Tren es la de inicio
}

void update(const core::SystemState& s) {
    state = s;
    header.set_level(core::overall_level(s));
    header.set_time(s.hour, s.minute);
    header.set_network(s.wifi_ok, s.pending_uploads);
    nav.set_tab_visible(Screen::cistern, available(Screen::cistern));
    home.update(s);
    summary.update(s);
    flow.update(s);
    cistern.update(s);
    mixer.update(s);
    dose.update(s);
    history.update(s);
    settings.update(s);
    flow_sensor.update(s);
    pressure.update(s);
    pressure_drop.update(s);
    filters.update(s);
    injection.update(s);
    events.update(s);
    diagnostics.update(s);
    bucket.update(s);
    overview.update(s);
    spares.update(s);
    idle.update(s);
    alarm.update(s);

    // Pasillo: nadie ha tocado la pantalla en los minutos de Ajustes
    const uint32_t idle_ms = static_cast<uint32_t>(s.idle_minutes) * 60u * 1000u;
    if (!idle.is_open() && lv_display_get_inactive_time(nullptr) >= idle_ms) show_idle();
}

void show(Screen screen) {
    if (screen == Screen::bucket_test) bucket.enter();  // cada vez empieza a contar desde 0
    for (size_t i = 0; i < pages.size(); ++i) lv_obj_set_hidden(pages[i], i != index_of(screen));
    const Page& page = kPages[index_of(screen)];
    HeaderSpec spec = page.header;
    if (spec.link && !available(*spec.link)) {  // "Bomba R" sin bomba, "Inyección" sin bombas A y B
        spec.link = {};
        spec.link_text = nullptr;
    }
    // Mezcla sin Cisterna (bomba R sin sonda): atrás y la pestaña van al Tren,
    // que es de donde se llega (la R)
    if (spec.back && !available(*spec.back)) spec.back = Screen::home;
    header.set_page(spec);
    nav.set_active(available(page.tab) ? page.tab : Screen::home);
}

void ask_dose_confirmation() {
    const uint32_t ml = core::dose_ml(state);
    if (ml == 0) return;  // el botón ya sale deshabilitado; esto es por si acaso
    char title[48];
    char body[160];
    snprintf(title, sizeof title, "¿Ya le pusiste %lu mL?", static_cast<unsigned long>(ml));
    snprintf(body, sizeof body,
             "Se registra hoy a las %02u:%02u y el conteo de litros desde la última dosis vuelve a 0.",
             static_cast<unsigned>(state.hour), static_cast<unsigned>(state.minute));
    dialog.open(title, body, "Sí, ya le puse", on_dose_confirmed);
}

void ask_pressure_zero() {
    dialog.open("¿Hacer cero de P1 y P2?",
                "Con V1 cerrada y el tren sin presión, lo que marquen ahorita queda como 0 psi.", "Sí, hacer cero", [] {
                    if (app_actions.pressure_zeroed != nullptr) app_actions.pressure_zeroed();
                });
}

void open_dose_keypad() {
    char hint[32];
    char date[24];
    char time[8];
    snprintf(hint, sizeof hint, "Sugerido: %lu mL", static_cast<unsigned long>(core::dose_ml(state)));
    snprintf(date, sizeof date, "Hoy %u %s", static_cast<unsigned>(state.day), fmt::month_short(state.month));
    fmt::clock(time, sizeof time, core::minute_of_day(state));
    keypad.open({"Registrar otra cantidad", "Cantidad", "mL", 0, 3, hint, date, time}, on_dose_typed);
}

void open_strip_keypad() {
    char hint[48];
    char mv[12];
    char date[24];
    char time[8];
    fmt::signed_int(mv, sizeof mv, state.orp_mv);
    snprintf(hint, sizeof hint, "ORP de ahorita: %s mV", mv);  // con más no cabe en 426 px
    snprintf(date, sizeof date, "Hoy %u %s", static_cast<unsigned>(state.day), fmt::month_short(state.month));
    fmt::clock(time, sizeof time, core::minute_of_day(state));
    keypad.open({"Calibrar con tira", "Cloro libre de la tira", "mg/L", 1, 2, hint, date, time}, on_strip_checked);
}

void open_keypad(const KeypadSpec& spec, void (*on_save)(uint32_t value)) { keypad.open(spec, on_save); }

void show_idle() {
    // Lo que se estaba capturando se cancela: al volver se regresa al Tren
    dialog.close();
    keypad.close();
    idle.open();
}

void open_confirm(const char* title, const char* body, const char* confirm_text, void (*on_confirm)()) {
    dialog.open(title, body, confirm_text, on_confirm);
}

const Actions& actions() { return app_actions; }

}  // namespace ui
