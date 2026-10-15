#include "screens/settings_screen.h"

#include <cmath>
#include <cstdio>

#include "components/keypad_dialog.h"
#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "theme/styles.h"
#include "ui.h"

// Medidas tomadas del artboard Ajustes del canvas (cada y menos 64).
// Las coordenadas dentro de una tarjeta cuentan desde adentro del borde.

namespace ui {
namespace {

constexpr int32_t kCardX = 388;
constexpr int32_t kCardW = 620;
constexpr int32_t kCardH = 210;
constexpr int32_t kTopY = 12;      // tarjeta de arriba
constexpr int32_t kBottomY = 234;  // tarjeta de abajo

lv_obj_t* settings_card(lv_obj_t* group, int32_t y, const char* title) {
    lv_obj_t* c = card(group, kCardX, y, kCardW, kCardH);
    lv_obj_t* t = label(c, &font_body_32, colors::text_secondary, title);
    lv_obj_set_pos(t, 20, 8);
    return c;
}

// Cifra de 72 con su unidad, en la misma línea base. Regresa la cifra.
lv_obj_t* big_value(lv_obj_t* c, const char* unit) {
    lv_obj_t* r = row(c, 10, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(r, 20, 52);
    lv_obj_t* value = label(r, &font_digits_72, colors::text, "");
    lv_obj_t* u = label(r, &font_title_36, colors::text_secondary, unit);
    lv_obj_set_style_pad_bottom(u, 2, 0);
    return value;
}

// Nota de dos renglones a la derecha de un botón
lv_obj_t* note(lv_obj_t* c, int32_t x, int32_t y, int32_t w, const char* text) {
    lv_obj_t* n = label(c, &font_label_26, colors::text_secondary, text);
    lv_obj_set_width(n, w);
    lv_obj_set_pos(n, x, y);
    return n;
}

// Botón secundario de 64 px. Con ícono antes del texto (Prueba de cubeta,
// Cambiar) o con flecha después (Diagnóstico, Ver cartuchos, Verlo).
lv_obj_t* small_button(lv_obj_t* c, int32_t x, int32_t y, int32_t w, const char* text, const lv_image_dsc_t* src,
                       bool arrow, lv_event_cb_t on_click, void* user_data) {
    lv_obj_t* b = button_secondary(c, x, y, w, text, on_click, user_data);
    lv_obj_set_height(b, 64);
    if (src != nullptr) {
        lv_obj_t* i = icon(b, src, colors::text);
        lv_obj_set_style_image_recolor(i, color(colors::text_disabled), LV_STATE_DISABLED);
        lv_obj_move_to_index(i, 0);
    }
    if (arrow) {
        lv_obj_set_style_pad_column(b, 8, 0);
        lv_obj_t* i = icon(b, &icon_chevron_32, colors::text_secondary);
        lv_obj_set_style_image_recolor(i, color(colors::text_disabled), LV_STATE_DISABLED);
    }
    return b;
}

// Lo que todavía no tiene su pantalla o su vista diseñada: se ve, no se toca
lv_obj_t* pending(lv_obj_t* button) {
    lv_obj_set_disabled(button, true);
    return button;
}

void on_night(int index, void*) {
    if (actions().night_off_set != nullptr) actions().night_off_set(index == 0);
}

void on_idle(int index, void*) {
    const uint32_t minutes[3] = {1, 2, 5};
    if (actions().idle_minutes_set != nullptr) actions().idle_minutes_set(minutes[index % 3]);
}

void format_ratio(char* out, size_t size, uint16_t thousandths) {
    snprintf(out, size, "%u.%03u", static_cast<unsigned>(thousandths / 1000), static_cast<unsigned>(thousandths % 1000));
}

}  // namespace

lv_obj_t* SettingsScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    create_categories(page);
    groups_[0] = create_sensors(page);
    groups_[1] = create_dose(page);
    groups_[2] = create_connection(page);
    groups_[3] = create_cartridges(page);
    groups_[4] = create_display(page);
    select(Category::sensors);
    return page;
}

void SettingsScreen::create_categories(lv_obj_t* page) {
    struct Def {
        const char* name;
        const lv_image_dsc_t* icon;
        const char* summary;  // fijo; los demás se llenan en update()
    };
    const Def defs[5] = {
        {"Sensores", &icon_medidor_32, "Flujo y presión"},
        {"Dosis", &icon_gota_32, ""},
        {"Conexión", &icon_wifi_32, ""},
        {"Cartuchos", &icon_cartucho_32, ""},
        {"Pantalla", &icon_sol_32, "Noche y modo pasillo"},
    };
    for (size_t i = 0; i < tabs_.size(); ++i) {
        Tab& t = tabs_[i];
        t.obj = box(page, 16, 12 + 88 * static_cast<int32_t>(i), 360, 76);
        lv_obj_set_clickable(t.obj, true);
        lv_obj_set_style_radius(t.obj, 10, 0);
        lv_obj_set_style_bg_opa(t.obj, LV_OPA_COVER, 0);
        lv_obj_add_style(t.obj, &styles::card_pressed, LV_STATE_PRESSED);
        lv_obj_set_user_data(t.obj, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        lv_obj_add_event_cb(t.obj, handle_category, LV_EVENT_CLICKED, this);

        t.icon = icon(t.obj, defs[i].icon, colors::text_secondary);
        lv_obj_set_pos(t.icon, 16, 22);
        lv_obj_t* name = label(t.obj, &font_body_32, colors::text, defs[i].name);
        lv_obj_set_pos(name, 62, 2);
        t.summary = label(t.obj, &font_label_26, colors::text_secondary, defs[i].summary);
        lv_obj_set_pos(t.summary, 62, 38);
    }
}

lv_obj_t* SettingsScreen::create_sensors(lv_obj_t* page) {
    lv_obj_t* group = box(page, 0, 0, 1024, 456);

    lv_obj_t* f = settings_card(group, kTopY, "Sensor de flujo F · YF-B6");
    factor_ = big_value(f, "pulsos/L");
    char buf[32];
    snprintf(buf, sizeof buf, "De fábrica: %u", static_cast<unsigned>(core::kFactoryPulsesPerLiter));
    lv_obj_t* factory = label(f, &font_label_26, colors::text_disabled, buf);
    lv_obj_align(factory, LV_ALIGN_TOP_RIGHT, -20, 72);
    link_to(small_button(f, 20, 130, 380, "Prueba de cubeta", &icon_cubeta_32, false, nullptr, nullptr),
            Screen::bucket_test, &styles::button_secondary_pressed);
    // Solo el lápiz: escribir el factor a mano
    lv_obj_t* edit = button_secondary(f, 412, 130, 64, "", handle_factor, this);
    lv_obj_set_height(edit, 64);
    lv_obj_delete(lv_obj_get_child(edit, 0));
    icon(edit, &icon_editar_32, colors::text);

    pressure_live_ = settings_card(group, kBottomY, "Presión P1 y P2 · cero");
    lv_obj_t* r = row(pressure_live_, 10, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(r, 20, 52);
    auto reading = [r](const char* name, bool gap) {
        lv_obj_t* n = label(r, &font_title_36, colors::text_secondary, name);
        lv_obj_set_style_pad_bottom(n, 2, 0);
        if (gap) lv_obj_set_style_margin_left(n, 24, 0);
        return label(r, &font_digits_72, colors::text, "");
    };
    p1_raw_ = reading("P1", false);
    p2_raw_ = reading("P2", true);
    lv_obj_t* psi = label(r, &font_title_36, colors::text_secondary, "psi");
    lv_obj_set_style_pad_bottom(psi, 2, 0);
    small_button(pressure_live_, 20, 130, 240, "Hacer cero", nullptr, false, [](lv_event_t*) { ask_pressure_zero(); },
                 nullptr);
    note(pressure_live_, 276, 128, 324, "Con V1 cerrada y el tren sin presión.");
    return group;
}

lv_obj_t* SettingsScreen::create_dose(lv_obj_t* page) {
    lv_obj_t* group = box(page, 0, 0, 1024, 456);

    lv_obj_t* r = settings_card(group, kTopY, "Relación de dosis");
    ratio_ = big_value(r, "mL por litro");
    small_button(r, 20, 130, 240, "Cambiar", &icon_editar_32, false, handle_ratio, this);
    ratio_note_ = note(r, 276, 128, 324, "");

    lv_obj_t* c = settings_card(group, kBottomY, "Tope diario");
    cap_ = big_value(c, "mL al día");
    small_button(c, 20, 130, 240, "Cambiar", &icon_editar_32, false, handle_cap, this);
    cap_note_ = note(c, 276, 128, 324, "La bomba B se para al llegar.");  // solo con bombas (fase 3)
    return group;
}

lv_obj_t* SettingsScreen::create_connection(lv_obj_t* page) {
    lv_obj_t* group = box(page, 0, 0, 1024, 456);

    lv_obj_t* w = settings_card(group, kTopY, "Red WiFi");
    ssid_ = label(w, &font_title_36, colors::text, "");
    lv_obj_set_pos(ssid_, 20, 52);
    signal_ = label(w, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(signal_, 20, 98);
    pending(small_button(w, 20, 136, 260, "Cambiar red", nullptr, false, nullptr, nullptr));

    lv_obj_t* u = settings_card(group, kBottomY, "Envío de datos");
    lv_obj_t* state = row(u, 8, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(state, 20, 52);
    upload_icon_ = icon(state, &icon_ok_32, colors::ok);
    upload_word_ = label(state, &font_title_36, colors::ok, "");
    upload_note_ = label(u, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(upload_note_, 20, 98);
    link_to(small_button(u, 20, 136, 300, "Diagnóstico", nullptr, true, nullptr, nullptr), Screen::diagnostics,
            &styles::button_secondary_pressed);
    return group;
}

lv_obj_t* SettingsScreen::create_cartridges(lv_obj_t* page) {
    lv_obj_t* group = box(page, 0, 0, 1024, 456);

    // Tabla fija: cada cuánto se cambia cada cartucho
    lv_obj_t* t = settings_card(group, kTopY, "Cada cuánto se cambian");
    const char* const rows[4][2] = {
        {"20 µm", "2 a 3 meses"}, {"10 y 5 µm", "3 a 6 meses"}, {"CTO", "6 meses"}, {"Sarro", "1 año"}};
    for (int i = 0; i < 4; ++i) {
        const int32_t y = 54 + 36 * i;
        lv_obj_set_pos(label(t, &font_label_26, colors::text_secondary, rows[i][0]), 20, y);
        lv_obj_set_pos(label(t, &font_label_26, colors::text, rows[i][1]), 20 + 150 + 16, y);
    }

    lv_obj_t* s = settings_card(group, kBottomY, "Repuestos en casa");
    spares_pp_ = label(s, &font_body_32, colors::text, "");
    lv_obj_set_pos(spares_pp_, 20, 54);
    spares_rest_ = label(s, &font_body_32, colors::text, "");
    lv_obj_set_pos(spares_rest_, 20, 92);
    // Cambiar abre los − y + de cada uno (no está en el canvas)
    link_to(small_button(s, 20, 136, 240, "Cambiar", &icon_editar_32, false, nullptr, nullptr), Screen::spares,
            &styles::button_secondary_pressed);
    link_to(small_button(s, 272, 136, 300, "Ver cartuchos", nullptr, true, nullptr, nullptr), Screen::filters,
            &styles::button_secondary_pressed);
    return group;
}

lv_obj_t* SettingsScreen::create_display(lv_obj_t* page) {
    lv_obj_t* group = box(page, 0, 0, 1024, 456);

    lv_obj_t* n = settings_card(group, kTopY, "Apagar la luz de noche");
    night_hours_ = label(n, &font_title_36, colors::text, "");
    lv_obj_set_pos(night_hours_, 20, 58);
    static const char* const kYesNo[2] = {"Sí", "No"};
    night_.create(n, 412, 52, 188, 56, kYesNo, 2, on_night);
    pending(small_button(n, 20, 130, 300, "Cambiar horario", nullptr, false, nullptr, nullptr));
    note(n, 336, 134, 264, "Un toque la vuelve a prender.");

    lv_obj_t* p = settings_card(group, kBottomY, "Modo pasillo");
    lv_obj_set_pos(label(p, &font_body_32, colors::text, "Cifras grandes si nadie toca en"), 20, 52);
    static const char* const kMinutes[3] = {"1 min", "2 min", "5 min"};
    idle_.create(p, 20, 130, 360, 64, kMinutes, 3, on_idle);
    small_button(p, 392, 130, 208, "Verlo", nullptr, true, [](lv_event_t*) { show_idle(); }, nullptr);
    return group;
}

void SettingsScreen::select(Category category) {
    current_ = category;
    for (size_t i = 0; i < tabs_.size(); ++i) {
        const bool on = static_cast<size_t>(category) == i;
        Tab& t = tabs_[i];
        // La elegida: panel-alto con borde de 2 px en texto; las demás, tarjeta normal
        lv_obj_set_style_bg_color(t.obj, color(on ? colors::panel_raised : colors::panel), 0);
        lv_obj_set_style_border_width(t.obj, on ? 2 : 1, 0);
        lv_obj_set_style_border_color(t.obj, color(on ? colors::text : colors::border), 0);
        set_icon_color(t.icon, on ? colors::text : colors::text_secondary);
        lv_obj_set_hidden(groups_[i], !on);
    }
}

void SettingsScreen::update(const core::SystemState& s) {
    char buf[96];
    char a[16];
    char b[16];

    // Sensores
    factor_value_ = s.flow_pulses_per_liter;
    snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(factor_value_));
    set_text(factor_, buf);
    lv_obj_set_hidden(pressure_live_, !s.has_pressure);
    set_text(tabs_[0].summary, s.has_pressure ? "Flujo y presión" : "Flujo");
    lv_obj_set_hidden(cap_note_, !s.has_pumps);
    snprintf(buf, sizeof buf, "%.1f", static_cast<double>(s.p1_zero_psi));
    set_text(p1_raw_, buf);
    snprintf(buf, sizeof buf, "%.1f", static_cast<double>(s.p2_zero_psi));
    set_text(p2_raw_, buf);

    // Dosis
    ratio_value_ = static_cast<uint16_t>(std::lround(s.ml_per_liter * 1000.0f));
    cap_value_ = s.daily_cap_ml;
    format_ratio(a, sizeof a, ratio_value_);
    set_text(ratio_, a);
    fmt::thousands(b, sizeof b, s.liters_since_dose);
    snprintf(buf, sizeof buf, "Hoy: %s L × %s = %lu mL de Cloralex", b, a,
             static_cast<unsigned long>(core::dose_ml(s)));
    set_text(ratio_note_, buf);
    snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(cap_value_));
    set_text(cap_, buf);
    snprintf(buf, sizeof buf, "%s mL/L · %u mL", a, static_cast<unsigned>(cap_value_));
    set_text(tabs_[1].summary, buf);

    // Conexión
    const char* ssid = s.wifi_ssid[0] != '\0' ? s.wifi_ssid : "Sin red";
    set_text(ssid_, ssid);
    const char* const strength[4] = {"Sin señal", "Señal débil", "Señal regular", "Señal buena"};
    snprintf(buf, sizeof buf, "%s · %s", strength[s.wifi_bars % 4], s.wifi_ok ? "en línea" : "sin conexión");
    set_text(signal_, buf);
    snprintf(buf, sizeof buf, "%s · %s", ssid, s.wifi_ok ? "en línea" : "sin conexión");
    set_text(tabs_[2].summary, buf);
    const bool up_to_date = s.pending_uploads == 0;
    const uint32_t tone = up_to_date ? colors::ok : colors::warning;
    lv_image_set_src(upload_icon_, up_to_date ? &icon_ok_32 : &icon_advertencia_32);
    set_icon_color(upload_icon_, tone);
    set_text(upload_word_, up_to_date ? "Al día" : "Por enviar");
    lv_obj_set_style_text_color(upload_word_, color(tone), 0);
    fmt::clock(a, sizeof a, s.last_upload_at);
    snprintf(buf, sizeof buf, "Último envío %s · %u por enviar", a, static_cast<unsigned>(s.pending_uploads));
    set_text(upload_note_, buf);

    // Cartuchos
    auto spare = [&s](core::Filter f) { return static_cast<unsigned>(s.spares[static_cast<size_t>(f)]); };
    const unsigned pp20 = spare(core::Filter::pp20);
    const unsigned pp10 = spare(core::Filter::pp10);
    const unsigned pp5 = spare(core::Filter::pp5);
    if (pp20 == pp10 && pp10 == pp5) {
        snprintf(buf, sizeof buf, "PP 20, 10 y 5 µm: %u de cada uno", pp20);
    } else {
        snprintf(buf, sizeof buf, "PP 20: %u · 10: %u · 5 µm: %u", pp20, pp10, pp5);
    }
    set_text(spares_pp_, buf);
    snprintf(buf, sizeof buf, "CTO: %u · Sarro: %u", spare(core::Filter::carbon), spare(core::Filter::scale));
    set_text(spares_rest_, buf);
    if (s.last_cartridge_day == 0) {
        set_text(tabs_[3].summary, "Sin cambios registrados");
    } else {
        const uint32_t ago = s.today >= s.last_cartridge_day ? s.today - s.last_cartridge_day : 0;
        const core::Date d = core::date_days_ago(core::today(s), s.year, ago);
        snprintf(buf, sizeof buf, "Último: %s, %u %s", fmt::filter_name(static_cast<unsigned>(s.last_cartridge)),
                 static_cast<unsigned>(d.day), fmt::month_short(d.month));
        set_text(tabs_[3].summary, buf);
    }

    // Pantalla
    fmt::clock(a, sizeof a, s.night_from);
    fmt::clock(b, sizeof b, s.night_to);
    snprintf(buf, sizeof buf, "%s a %s", a, b);
    set_text(night_hours_, buf);
    night_.set_selected(s.night_off ? 0 : 1);
    idle_.set_selected(s.idle_minutes <= 1 ? 0 : s.idle_minutes <= 2 ? 1 : 2);
}

void SettingsScreen::handle_category(lv_event_t* e) {
    auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    lv_obj_t* tab = lv_event_get_current_target_obj(e);
    self->select(static_cast<Category>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(tab))));
}

void SettingsScreen::handle_factor(lv_event_t* e) {
    const auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    char hint[40];
    snprintf(hint, sizeof hint, "Ahorita: %u · de fábrica: %u", static_cast<unsigned>(self->factor_value_),
             static_cast<unsigned>(core::kFactoryPulsesPerLiter));
    open_keypad({"Factor del sensor F", "Pulsos por litro", "pulsos/L", 0, 4, hint, nullptr, nullptr},
                [](uint32_t value) {
                    if (actions().flow_factor_set != nullptr) actions().flow_factor_set(value);
                });
}

void SettingsScreen::handle_ratio(lv_event_t* e) {
    const auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    char ratio[12];
    char hint[40];
    format_ratio(ratio, sizeof ratio, self->ratio_value_);
    snprintf(hint, sizeof hint, "Ahorita: %s mL por litro", ratio);
    open_keypad({"Relación de dosis", "mL de Cloralex por litro", "mL/L", 3, 4, hint, nullptr, nullptr},
                [](uint32_t value) {
                    if (actions().dose_ratio_set != nullptr) actions().dose_ratio_set(value);
                });
}

void SettingsScreen::handle_cap(lv_event_t* e) {
    const auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
    char hint[32];
    snprintf(hint, sizeof hint, "Ahorita: %u mL al día", static_cast<unsigned>(self->cap_value_));
    open_keypad({"Tope diario", "mL al día", "mL", 0, 3, hint, nullptr, nullptr}, [](uint32_t value) {
        if (actions().daily_cap_set != nullptr) actions().daily_cap_set(value);
    });
}

}  // namespace ui
