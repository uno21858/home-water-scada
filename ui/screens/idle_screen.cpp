#include "screens/idle_screen.h"

#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"

// Medidas tomadas del artboard Pasillo del canvas. Aquí no se resta 64:
// la pantalla es completa, de 1024 x 600.

namespace ui {
namespace {

// Cifras grandes bajadas como en Flujo y Cisterna (ver CLAUDE.md)
constexpr int32_t kFlowY = 164 + 17;
constexpr int32_t kDoseY = 222 + 10;
// Con 3 cifras los mL no caben a 136 px antes de la orilla: van a 72 px con
// la misma línea base (98 px abajo del top a 136 y 52 a 72)
constexpr int32_t kDoseY72 = kDoseY + 98 - 52;

// "Hoy 1,860 L": nombre, cifra de 44 y unidad en la misma línea base
lv_obj_t* reading(lv_obj_t* parent, const char* name, const char* unit, lv_obj_t** value) {
    lv_obj_t* r = row(parent, 10, LV_FLEX_ALIGN_END);
    label(r, &font_body_32, colors::text_secondary, name);
    *value = label(r, &font_digits_44, colors::text, "");
    label(r, &font_label_26, colors::text_secondary, unit);
    return r;
}

}  // namespace

void IdleScreen::create(lv_obj_t* layer, Callback on_wake) {
    on_wake_ = on_wake;

    root_ = box(layer, 0, 0, 1024, 600);
    fill(root_, colors::background);
    lv_obj_set_clickable(root_, true);  // un toque en cualquier lado lo cierra
    lv_obj_add_event_cb(root_, handle_touch, LV_EVENT_CLICKED, this);

    clock_ = label(root_, &font_title_xl_48, colors::text, "");
    lv_obj_set_pos(clock_, 32, 28);

    // Pastilla de estado, más grande que la del encabezado
    pill_ = row(root_, 10, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_height(pill_, 58);
    lv_obj_set_style_radius(pill_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(pill_, 2, 0);
    lv_obj_set_style_bg_opa(pill_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(pill_, 12, 0);
    lv_obj_set_style_pad_right(pill_, 22, 0);
    lv_obj_align(pill_, LV_ALIGN_TOP_RIGHT, -32, 28);
    pill_icon_ = icon(pill_, &icon_ok_40, colors::ok);
    pill_text_ = label(pill_, &font_title_36, colors::ok, "");

    // Caudal
    flow_title_ = label(root_, &font_body_32, colors::text_secondary, "");
    lv_obj_set_pos(flow_title_, 32, 124);
    flow_value_ = label(root_, &font_digits_232, colors::water, "");
    lv_obj_set_pos(flow_value_, 22, kFlowY);
    lv_obj_set_pos(label(root_, &font_title_36, colors::text_secondary, "L/min"), 584, 288);

    // Toca echar, a la derecha de una raya
    fill(box(root_, 712, 176, 2, 216), colors::border);
    lv_obj_set_pos(label(root_, &font_body_32, colors::text_secondary, "Toca echar"), 744, 176);
    dose_value_ = label(root_, &font_digits_136, colors::text, "");
    dose_unit_ = label(root_, &font_title_36, colors::text_secondary, "mL");
    lv_obj_set_pos(label(root_, &font_label_26, colors::text_secondary, "de Cloralex en B"), 744, 350);

    // Renglón de abajo: hoy, caída y ORP
    lv_obj_t* bottom = row(root_, 40, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(bottom, 32, 410);
    reading(bottom, "Hoy", "L", &today_);
    drop_ = reading(bottom, "Caída", "psi", &drop_value_);
    drop_word_ = label(drop_, &font_label_26, colors::ok, "");
    orp_ = reading(bottom, "ORP", "mV", &orp_value_);

    pipe_.add(root_, 0, 500, 1024, 10, FlowPipes::Dir::right);

    lv_obj_t* hint = label(root_, &font_label_26, colors::text_disabled, "Toca la pantalla para volver al tren");
    lv_obj_set_pos(hint, 32, 540);

    lv_obj_set_hidden(root_, true);
}

void IdleScreen::update(const core::SystemState& s) {
    char buf[16];
    snprintf(buf, sizeof buf, "%02u:%02u", static_cast<unsigned>(s.hour), static_cast<unsigned>(s.minute));
    set_text(clock_, buf);

    const core::Level level = core::overall_level(s);
    if (static_cast<int>(level) != level_) {
        level_ = static_cast<int>(level);
        uint32_t fg = colors::ok;
        uint32_t bg = colors::ok_bg;
        uint32_t border = colors::ok;
        const char* text = "OK";
        const lv_image_dsc_t* src = &icon_ok_40;
        if (level == core::Level::warning) {
            fg = border = colors::warning;
            bg = colors::warning_bg;
            text = "Advertencia";
            src = &icon_advertencia_40;
        } else if (level == core::Level::alarm) {
            fg = colors::on_alarm;
            bg = border = colors::alarm_screen;
            text = "Alarma";
            src = &icon_alarma_40;
        }
        lv_obj_set_style_bg_color(pill_, color(bg), 0);
        lv_obj_set_style_border_color(pill_, color(border), 0);
        lv_image_set_src(pill_icon_, src);
        set_icon_color(pill_icon_, fg);
        lv_label_set_text(pill_text_, text);
        lv_obj_set_style_text_color(pill_text_, color(fg), 0);
    }

    const bool flowing = s.flow_lpm >= 0.05f;
    fmt::one_decimal(buf, sizeof buf, flowing ? s.flow_lpm : 0.0f);
    set_text(flow_value_, buf);
    pipe_.set_flow(flowing ? s.flow_lpm : 0.0f);
    const int state = flowing ? 0 : core::supply_lost(s) ? 2 : 1;
    if (state != flow_state_) {
        flow_state_ = state;
        set_text(flow_title_, state == 0 ? "Caudal" : state == 1 ? "Sin flujo" : "Sin agua del SIAPA");
        set_text_color(flow_title_, state == 2 ? colors::warning : colors::text_secondary);
        lv_obj_set_style_text_color(flow_value_, color(flowing ? colors::water : colors::text_disabled), 0);
    }

    const uint32_t ml = core::dose_ml(s);
    snprintf(buf, sizeof buf, "%lu", static_cast<unsigned long>(ml));
    set_text(dose_value_, buf);
    layout_dose(ml >= 100);

    fmt::thousands(buf, sizeof buf, s.liters_today);
    set_text(today_, buf);

    lv_obj_set_hidden(drop_, !s.has_pressure);
    if (s.has_pressure) {
        const int drop = s.p1_psi - s.p2_psi;
        snprintf(buf, sizeof buf, "%d", drop);
        set_text(drop_value_, buf);
        const core::Level drop_level = core::drop_level(s);
        if (static_cast<int>(drop_level) != drop_level_) {
            drop_level_ = static_cast<int>(drop_level);
            const uint32_t tone = drop_level == core::Level::ok        ? colors::ok
                                  : drop_level == core::Level::warning ? colors::warning
                                                                       : colors::alarm;
            set_text(drop_word_, drop_level == core::Level::ok        ? "Normal"
                                 : drop_level == core::Level::warning ? "Revisar"
                                                                      : "Tapados");
            lv_obj_set_style_text_color(drop_word_, color(tone), 0);
        }
    }

    lv_obj_set_hidden(orp_, !s.has_orp);
    if (s.has_orp) {
        fmt::signed_int(buf, sizeof buf, s.orp_mv);
        set_text(orp_value_, buf);
    }
}

void IdleScreen::layout_dose(bool three_digits) {
    if (static_cast<int>(three_digits) == three_digits_) return;
    three_digits_ = three_digits;
    lv_obj_set_style_text_font(dose_value_, three_digits ? &font_digits_72 : &font_digits_136, 0);
    lv_obj_set_pos(dose_value_, 740, three_digits ? kDoseY72 : kDoseY);
    // A 136 px "mL" va arriba a la derecha de la cifra; a 72, en su misma línea base
    lv_obj_set_pos(dose_unit_, three_digits ? 900 : 940, three_digits ? kDoseY72 + 16 : 228);
}

void IdleScreen::open() { lv_obj_set_hidden(root_, false); }

void IdleScreen::close() { lv_obj_set_hidden(root_, true); }

bool IdleScreen::is_open() const { return !lv_obj_is_hidden(root_); }

void IdleScreen::handle_touch(lv_event_t* e) {
    auto* self = static_cast<IdleScreen*>(lv_event_get_user_data(e));
    self->close();
    if (self->on_wake_ != nullptr) self->on_wake_();
}

}  // namespace ui
