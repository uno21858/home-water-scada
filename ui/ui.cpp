#include "ui.h"

#include <array>
#include <cstdio>

#include "components/confirm_dialog.h"
#include "components/header.h"
#include "components/keypad_dialog.h"
#include "components/nav_bar.h"
#include "components/primitives.h"
#include "screens/cistern_screen.h"
#include "screens/dose_screen.h"
#include "screens/flow_screen.h"
#include "screens/history_screen.h"
#include "screens/home_screen.h"
#include "screens/mixer_screen.h"
#include "screens/settings_screen.h"
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
// Mismo orden que el enum Screen. En el encabezado van título, atrás, chip,
// texto del enlace y a dónde lleva; {} es "no tiene".
struct Page {
    HeaderSpec header;
    Screen tab;
};
const std::array<Page, kScreenCount> kPages = {{
    {{"Tren de filtros", {}, nullptr, nullptr, {}}, Screen::home},
    {{"Resumen", {}, nullptr, nullptr, {}}, Screen::summary},
    {{"Flujo", {}, nullptr, nullptr, {}}, Screen::flow},
    {{"Cisterna", {}, "Fase 2", "Bomba R", Screen::mixer}, Screen::cistern},
    {{"Dosis de cloro", {}, nullptr, nullptr, {}}, Screen::dose},
    {{"Historial", {}, nullptr, nullptr, {}}, Screen::history},
    {{"Ajustes", {}, nullptr, nullptr, {}}, Screen::settings},
    {{"Mezcla de la cisterna", Screen::cistern, "Fase 2", nullptr, {}}, Screen::cistern},
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
DoseScreen dose;
std::array<lv_obj_t*, kScreenCount> pages{};

size_t index_of(Screen screen) { return static_cast<size_t>(screen); }

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

    header.create(screen);
    nav.create(screen);
    dialog.create(lv_layer_top());
    keypad.create(lv_layer_top());

    update(s);
    show(Screen::home);  // Tren es la de inicio
}

void update(const core::SystemState& s) {
    state = s;
    header.set_level(core::overall_level(s));
    header.set_time(s.hour, s.minute);
    header.set_wifi(s.wifi_ok);
    home.update(s);
    summary.update(s);
    flow.update(s);
    cistern.update(s);
    mixer.update(s);
    dose.update(s);
    history.update(s);
    settings.update(s);
}

void show(Screen screen) {
    for (size_t i = 0; i < pages.size(); ++i) lv_obj_set_hidden(pages[i], i != index_of(screen));
    const Page& page = kPages[index_of(screen)];
    header.set_page(page.header);
    nav.set_active(page.tab);
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
    snprintf(hint, sizeof hint, "Se guarda con el ORP de ahorita: %s mV", mv);
    snprintf(date, sizeof date, "Hoy %u %s", static_cast<unsigned>(state.day), fmt::month_short(state.month));
    fmt::clock(time, sizeof time, core::minute_of_day(state));
    keypad.open({"Calibrar con tira", "Cloro libre de la tira", "mg/L", 1, 2, hint, date, time}, on_strip_checked);
}

void open_keypad(const KeypadSpec& spec, void (*on_save)(uint32_t value)) { keypad.open(spec, on_save); }

void open_confirm(const char* title, const char* body, const char* confirm_text, void (*on_confirm)()) {
    dialog.open(title, body, confirm_text, on_confirm);
}

const Actions& actions() { return app_actions; }

}  // namespace ui
