#include "screens/dose_screen.h"

#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "theme/styles.h"
#include "ui.h"

// Medidas tomadas del artboard Dosis del canvas.

namespace ui {
namespace {

// Tarjeta "Toca echar": 704 de ancho, 702 por dentro del borde
constexpr int32_t kDueTextX = 238;        // texto a la derecha de una cifra de 2 dígitos
constexpr int32_t kDueTextXWide = 334;    // con 3 dígitos (100 mL o más)
constexpr int32_t kDueRight = 702 - 16;   // margen derecho por dentro

// Selector Manual | Auto deshabilitado, como en el DS para la fase 3
void disabled_segmented(lv_obj_t* parent, int32_t x, int32_t y) {
    lv_obj_t* seg = box(parent, x, y, 244, 56);
    lv_obj_set_style_bg_color(seg, color(colors::panel), 0);
    lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(seg, color(colors::border), 0);
    lv_obj_set_style_border_width(seg, 2, 0);
    lv_obj_set_style_radius(seg, 10, 0);
    lv_obj_set_style_clip_corner(seg, true, 0);  // que lo de adentro respete las esquinas

    lv_obj_t* manual = box(seg, 0, 0, 120, 52);
    lv_obj_set_style_bg_color(manual, color(colors::panel_raised), 0);
    lv_obj_set_style_bg_opa(manual, LV_OPA_COVER, 0);
    lv_obj_center(label(manual, &font_label_26, colors::text_disabled, "Manual"));

    lv_obj_t* automatic = box(seg, 120, 0, 120, 52);
    lv_obj_set_style_border_side(automatic, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(automatic, 2, 0);
    lv_obj_set_style_border_color(automatic, color(colors::border), 0);
    lv_obj_center(label(automatic, &font_label_26, colors::text_disabled, "Auto"));
}

// Botón de pulso deshabilitado: bomba y letra
void disabled_pulse(lv_obj_t* parent, int32_t x, int32_t y, const char* pump) {
    lv_obj_t* b = box(parent, x, y, 116, 56);
    lv_obj_set_style_bg_color(b, color(colors::panel), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(b, color(colors::border), 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_flex_flow(b, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(b, 8, 0);
    icon(b, &icon_bomba_28, colors::text_disabled);
    label(b, &font_body_32, colors::text_disabled, pump);
}

}  // namespace

lv_obj_t* DoseScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    create_due_card(page);

    done_button_ = button_primary(page, 16, 204, 280, "Ya le puse",
                                  [](lv_event_t*) { ask_dose_confirmation(); }, nullptr);
    button_secondary(page, 308, 204, 412, "Registrar otra cantidad",
                     [](lv_event_t*) { open_dose_keypad(); }, nullptr);

    create_history(page);
    create_phase3_panel(page);
    return page;
}

void DoseScreen::create_due_card(lv_obj_t* page) {
    lv_obj_t* c = card(page, 16, 12, 704, 180);

    lv_obj_t* title = label(c, &font_body_32, colors::text_secondary, "Toca echar");
    lv_obj_set_pos(title, 24, 8);

    due_value_ = label(c, &font_digits_136, colors::text, "");
    lv_obj_set_pos(due_value_, 16, 48);

    due_unit_ = label(c, &font_title_36, colors::text, "mL de Cloralex");
    lv_obj_set_y(due_unit_, 52);
    due_liters_ = label(c, &font_label_26, colors::text_secondary, "");
    lv_obj_set_y(due_liters_, 102);
    due_ratio_ = label(c, &font_label_26, colors::text_secondary, "");
    lv_obj_set_y(due_ratio_, 134);
}

void DoseScreen::create_history(lv_obj_t* page) {
    lv_obj_t* title = label(page, &font_label_26, colors::text_secondary, "Últimas dosis");
    lv_obj_set_pos(title, 16, 294);

    // 5 tarjetas que llenan 704 px con 12 px entre ellas
    constexpr int32_t kWidths[5] = {131, 131, 132, 131, 131};
    int32_t x = 16;
    for (size_t i = 0; i < tiles_.size(); ++i) {
        Tile& t = tiles_[i];
        t.obj = card(page, x, 330, kWidths[i], 114);
        t.day = label(t.obj, &font_label_26, colors::text_secondary, "");
        lv_obj_set_pos(t.day, 12, 4);
        t.time = label(t.obj, &font_label_26, colors::text_secondary, "");
        lv_obj_set_pos(t.time, 12, 32);
        lv_obj_t* r = row(t.obj, 6, LV_FLEX_ALIGN_END);
        lv_obj_set_pos(r, 12, 68);
        t.ml = label(r, &font_digits_44, colors::text, "");
        label(r, &font_label_26, colors::text_secondary, "mL");
        t.typed = icon(t.obj, &icon_editar_24, colors::text_secondary);
        lv_obj_set_pos(t.typed, 95, 8);
        x += kWidths[i] + 12;
    }
}

void DoseScreen::create_phase3_panel(lv_obj_t* page) {
    lv_obj_t* p = box(page, 732, 12, 276, 432);
    lv_obj_add_style(p, &styles::card_locked, 0);

    lv_obj_t* head = row(p, 10, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(head, 14, 10);
    icon(head, &icon_candado_32, colors::text_secondary);
    label(head, &font_body_32, colors::text_secondary, "Fase 3");

    struct Line {
        const char* text;
        int32_t y;
    };
    const Line lines[] = {{"Próximamente", 50}, {"Bomba A", 94}, {"Bomba B · boca", 196}, {"Pulso de prueba", 298}};
    for (const Line& l : lines) lv_obj_set_pos(label(p, &font_label_26, colors::text_disabled, l.text), 14, l.y);

    disabled_segmented(p, 14, 128);
    disabled_segmented(p, 14, 230);
    disabled_pulse(p, 14, 332, "A");
    disabled_pulse(p, 142, 332, "B");

    cap_ = label(p, &font_label_26, colors::text_disabled, "");
    lv_obj_set_pos(cap_, 14, 394);
}

void DoseScreen::layout_due(bool three_digits) {
    if (static_cast<int>(three_digits) == three_digits_) return;
    three_digits_ = three_digits;

    // Con 3 dígitos la cifra crece 95 px: el texto se recorre, la cuenta de
    // litros se parte en dos renglones y la línea de la relación se esconde.
    const int32_t x = three_digits ? kDueTextXWide : kDueTextX;
    lv_obj_set_x(due_unit_, x);
    lv_obj_set_x(due_liters_, x);
    lv_obj_set_x(due_ratio_, x);
    lv_obj_set_width(due_liters_, three_digits ? kDueRight - x : LV_SIZE_CONTENT);
    lv_obj_set_hidden(due_ratio_, three_digits);
}

void DoseScreen::update(const core::SystemState& s) {
    const uint32_t ml = core::dose_ml(s);
    char buf[64];
    snprintf(buf, sizeof buf, "%lu", static_cast<unsigned long>(ml));
    set_text(due_value_, buf);
    layout_due(ml >= 100);
    lv_obj_set_disabled(done_button_, ml == 0);

    char liters[16];
    fmt::thousands(liters, sizeof liters, s.liters_since_dose);
    snprintf(buf, sizeof buf, "%s L desde la última dosis", liters);
    set_text(due_liters_, buf);
    snprintf(buf, sizeof buf, "× %.3f mL por litro · en la boca B", static_cast<double>(s.ml_per_liter));
    set_text(due_ratio_, buf);

    for (size_t i = 0; i < tiles_.size(); ++i) {
        Tile& t = tiles_[i];
        const bool used = i < s.recent_dose_count;
        lv_obj_set_hidden(t.obj, !used);
        if (!used) continue;

        const core::DoseRecord& r = s.recent_doses[i];
        const uint32_t ago = s.today - r.day;
        set_text(t.day, ago == 0 ? "Hoy" : fmt::weekday_short((s.weekday + 7 - ago % 7) % 7));
        fmt::clock(buf, sizeof buf, r.at);
        set_text(t.time, buf);
        snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(r.ml));
        set_text(t.ml, buf);
        lv_obj_set_hidden(t.typed, !r.typed);
    }

    snprintf(buf, sizeof buf, "Tope: %u mL/día", static_cast<unsigned>(s.daily_cap_ml));
    set_text(cap_, buf);
}

}  // namespace ui
