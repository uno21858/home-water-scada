#include "screens/injection_screen.h"

#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "theme/styles.h"

// Medidas tomadas del artboard Inyeccion del canvas (cada y menos 64).
// Las coordenadas dentro de una tarjeta cuentan desde adentro del borde.

namespace ui {
namespace {

// Tarjeta de una boca: letra grande, para qué es y abajo de la raya la bomba
// de la fase 3 con sus controles apagados
lv_obj_t* outlet_card(lv_obj_t* page, int32_t x, const char* letter, const char* purpose, const char* pump) {
    lv_obj_t* c = card(page, x, 12, 490, 432);

    lv_obj_t* head = row(c, 14, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(head, 16, 10);
    label(head, &font_title_xl_48, colors::text, letter);
    lv_obj_t* what = label(head, &font_body_32, colors::text_secondary, purpose);
    lv_obj_set_style_pad_bottom(what, 4, 0);  // misma línea base que la letra (11 contra 7)

    fill(box(c, 16, 226, 456, 1), colors::border);

    lv_obj_t* lock = row(c, 10, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(lock, 16, 240);
    icon(lock, &icon_candado_28, colors::text_disabled);
    label(lock, &font_label_26, colors::text_disabled, pump);

    // Manual · Auto sin tocar: borde decorativo y todo en texto-3
    lv_obj_t* modes = box(c, 16, 284, 280, 56);
    lv_obj_set_style_border_width(modes, 2, 0);
    lv_obj_set_style_border_color(modes, color(colors::border), 0);
    lv_obj_set_style_radius(modes, 10, 0);
    lv_obj_set_style_clip_corner(modes, true, 0);
    fill(modes, colors::panel);
    lv_obj_t* manual = box(modes, 0, 0, 138, 52);
    fill(manual, colors::panel_raised);
    lv_obj_center(label(manual, &font_label_26, colors::text_disabled, "Manual"));
    lv_obj_t* automatic = box(modes, 138, 0, 138, 52);
    lv_obj_set_style_border_side(automatic, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(automatic, 2, 0);
    lv_obj_set_style_border_color(automatic, color(colors::border), 0);
    lv_obj_center(label(automatic, &font_label_26, colors::text_disabled, "Auto"));

    lv_obj_t* test = row(c, 8, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(test, 308, 284);
    lv_obj_set_size(test, 164, 56);
    lv_obj_set_flex_align(test, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_radius(test, 10, 0);
    lv_obj_set_style_border_width(test, 2, 0);
    lv_obj_set_style_border_color(test, color(colors::border), 0);
    icon(test, &icon_bomba_28, colors::text_disabled);
    label(test, &font_label_26, colors::text_disabled, "Prueba");
    return c;
}

}  // namespace

lv_obj_t* InjectionScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);

    lv_obj_t* a = outlet_card(page, 16, "A", "Para después", "Bomba A · fase 3 · salida DO0");
    lv_obj_set_pos(label(a, &font_body_32, colors::text, "Todavía no se usa"), 16, 106);
    lv_obj_t* note = label(a, &font_label_26, colors::text_secondary, "Por ahora no sale en el diagrama del tren");
    lv_obj_set_width(note, 456);
    lv_obj_set_pos(note, 16, 146);

    lv_obj_t* b = outlet_card(page, 518, "B", "Boca a la cisterna", "Bomba B · fase 3 · salida DO1");
    lv_obj_set_pos(label(b, &font_label_26, colors::text_secondary, "Hoy"), 16, 76);
    lv_obj_set_pos(label(b, &font_body_32, colors::text, "Cloralex a mano por V3"), 16, 106);
    last_ = label(b, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(last_, 16, 146);

    // Toca echar: lleva a Dosis
    lv_obj_t* due = box(b, 16, 352, 456, 64);
    lv_obj_add_style(due, &styles::button_secondary, 0);
    link_to(due, Screen::dose, &styles::button_secondary_pressed);
    lv_obj_set_flex_flow(due, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(due, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(due, 20, 0);
    lv_obj_set_style_pad_right(due, 12, 0);
    lv_obj_set_style_pad_column(due, 10, 0);
    lv_obj_set_flex_grow(label(due, &font_body_32, colors::text, "Toca echar"), 1);
    due_ = label(due, &font_digits_44, colors::text, "");
    label(due, &font_label_26, colors::text_secondary, "mL");
    icon(due, &icon_chevron_32, colors::text_secondary);
    return page;
}

void InjectionScreen::update(const core::SystemState& s) {
    char buf[64];
    snprintf(buf, sizeof buf, "%lu", static_cast<unsigned long>(core::dose_ml(s)));
    set_text(due_, buf);

    if (s.recent_dose_count == 0) {
        set_text(last_, "Todavía no hay dosis");
        return;
    }
    const core::DoseRecord& d = s.recent_doses[0];
    char when[24];
    char time[8];
    const uint32_t ago = s.today >= d.day ? s.today - d.day : 0;
    if (ago == 0) {
        snprintf(when, sizeof when, "hoy");
    } else {
        const core::Date date = core::date_days_ago(core::today(s), s.year, ago);
        snprintf(when, sizeof when, "%s %u %s", fmt::weekday_short(date.weekday), static_cast<unsigned>(date.day),
                 fmt::month_short(date.month));
    }
    fmt::clock(time, sizeof time, d.at);
    snprintf(buf, sizeof buf, "Última: %s %s · %u mL", when, time, static_cast<unsigned>(d.ml));
    set_text(last_, buf);
}

}  // namespace ui
