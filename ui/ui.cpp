#include "ui.h"

#include <array>
#include <cstdio>

#include "components/confirm_dialog.h"
#include "components/header.h"
#include "components/nav_bar.h"
#include "components/primitives.h"
#include "screens/home_screen.h"
#include "screens/summary_screen.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/styles.h"

// Estructura (ver lvgl.md del DS): una sola screen con tres partes fijas.
// Encabezado arriba, contenido en medio y barra abajo. Cada pantalla es un
// contenedor dentro del contenido; cambiar de pantalla es esconder uno y
// mostrar otro, nada se recrea.

namespace ui {
namespace {

constexpr int32_t kHeaderHeight = 64;
constexpr int32_t kContentHeight = 456;  // de y 64 a y 520

constexpr std::array<const char*, kScreenCount> kTitles = {
    "Tren de filtros", "Resumen", "Flujo", "Cisterna", "Dosis de cloro", "Historial", "Ajustes",
};

Actions actions;
core::SystemState state;  // la última copia que llegó en update()
Header header;
NavBar nav;
ConfirmDialog dialog;
HomeScreen home;
SummaryScreen summary;
std::array<lv_obj_t*, kScreenCount> pages{};

size_t index_of(Screen screen) { return static_cast<size_t>(screen); }

// Pantalla que todavía no está hecha
lv_obj_t* placeholder(lv_obj_t* parent, const char* name) {
    lv_obj_t* page = box(parent, 0, 0, 1024, kContentHeight);
    lv_obj_t* text = label(page, &font_body_32, colors::text_disabled, "");
    lv_label_set_text_fmt(text, "%s: próximamente", name);
    lv_obj_center(text);
    return page;
}

void on_dose_confirmed() {
    if (actions.dose_confirmed != nullptr) actions.dose_confirmed(core::dose_ml(state));
}

}  // namespace

void init(const core::SystemState& s, const Actions& a) {
    state = s;
    actions = a;

    // Sin tema de LVGL: todo el look sale de theme/
    lv_display_set_theme(lv_display_get_default(), nullptr);
    styles::init();

    lv_obj_t* screen = lv_screen_active();
    lv_obj_add_style(screen, &styles::screen, 0);
    lv_obj_set_scrollable(screen, false);

    lv_obj_t* content = box(screen, 0, kHeaderHeight, 1024, kContentHeight);
    pages[index_of(Screen::home)] = home.create(content);
    pages[index_of(Screen::summary)] = summary.create(content);
    pages[index_of(Screen::flow)] = placeholder(content, "Flujo");
    pages[index_of(Screen::cistern)] = placeholder(content, "Cisterna");
    pages[index_of(Screen::dose)] = placeholder(content, "Dosis");
    pages[index_of(Screen::history)] = placeholder(content, "Historial");
    pages[index_of(Screen::settings)] = placeholder(content, "Ajustes");

    header.create(screen);
    nav.create(screen);
    dialog.create(lv_layer_top());

    update(s);
    show(Screen::summary);
}

void update(const core::SystemState& s) {
    state = s;
    header.set_level(core::overall_level(s));
    header.set_time(s.hour, s.minute);
    header.set_wifi(s.wifi_ok);
    home.update(s);
    summary.update(s);
}

void show(Screen screen) {
    for (size_t i = 0; i < pages.size(); ++i) lv_obj_set_hidden(pages[i], i != index_of(screen));
    header.set_title(kTitles[index_of(screen)]);
    nav.set_active(screen);
}

void ask_dose_confirmation() {
    char title[48];
    char body[160];
    snprintf(title, sizeof title, "¿Ya le pusiste %lu mL?", static_cast<unsigned long>(core::dose_ml(state)));
    snprintf(body, sizeof body,
             "Se registra hoy a las %02u:%02u y el conteo de litros desde la última dosis vuelve a 0.",
             static_cast<unsigned>(state.hour), static_cast<unsigned>(state.minute));
    dialog.open(title, body, "Sí, ya le puse", on_dose_confirmed);
}

}  // namespace ui
