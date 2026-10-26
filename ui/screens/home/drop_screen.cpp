#include "screens/home/drop_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "calendar.h"
#include "clog.h"
#include "components/clog_look.h"
#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "theme/styles.h"

// El acomodo es el del artboard DetalleCaida del canvas (cada y menos 64),
// con lo que se mide ahora: R = (P1 - P2) / F contra cartuchos nuevos
// (core/clog.h). Las coordenadas dentro de una tarjeta cuentan desde adentro
// del borde.

namespace ui {
namespace {

constexpr int32_t kValueY = 44 + 10;  // cifra de 136 bajada como en Cisterna

// Escala de R/R0 de 50 a 250 % en 576 px: Normal hasta 150, Revisar hasta
// 200 y Tapados arriba
constexpr int32_t kScaleW = 576;
constexpr float kScaleMin = 50.0f;
constexpr float kScaleMax = 250.0f;
int32_t scale_x(float pct) {
    return static_cast<int32_t>(
        std::lround((std::clamp(pct, kScaleMin, kScaleMax) - kScaleMin) * kScaleW / (kScaleMax - kScaleMin)));
}

struct Zone {
    float from;  // %
    float to;
    const char* name;
    uint32_t dim;
};
const Zone kZones[3] = {
    {kScaleMin, core::kClogWarningRatio * 100.0f, "Normal", colors::ok_bg},
    {core::kClogWarningRatio * 100.0f, core::kClogAlarmRatio * 100.0f, "Revisar", colors::warning_bg},
    {core::kClogAlarmRatio * 100.0f, kScaleMax, "Tapados", colors::alarm_bg},
};

// Gráfica dentro de su tarjeta: los 30 días y R de 0 a plot_max_
constexpr float kPlotX = 71.0f;
constexpr float kPlotW = 512.0f;
constexpr float kPlotBase = 142.0f;  // y de R = 0
constexpr float kPlotH = 102.0f;
int32_t px(float v) { return static_cast<int32_t>(std::lround(v)); }

lv_obj_t* axis_label(lv_obj_t* parent, const char* text, int32_t x, int32_t y, lv_text_align_t align) {
    lv_obj_t* obj = label(parent, &font_label_26, colors::text_disabled, text);
    lv_obj_set_width(obj, 180);  // "hace 29 días" mide 169
    lv_obj_set_style_text_align(obj, align, 0);
    lv_obj_set_pos(obj, align == LV_TEXT_ALIGN_LEFT ? x : align == LV_TEXT_ALIGN_RIGHT ? x - 180 : x - 90, y);
    return obj;
}

// "Último ......... 3.9" con su nota abajo
lv_obj_t* r_row(lv_obj_t* parent, const char* name, int32_t y, lv_obj_t** note) {
    lv_obj_t* r = row(parent, 8, LV_FLEX_ALIGN_END);
    lv_obj_set_width(r, 348);
    lv_obj_set_pos(r, 16, y);
    lv_obj_set_flex_grow(label(r, &font_label_26, colors::text_secondary, name), 1);
    lv_obj_t* value = label(r, &font_body_32, colors::text, "");
    *note = label(parent, &font_label_26, colors::text_disabled, "");
    lv_obj_set_pos(*note, 16, y + 38);
    lv_obj_set_width(*note, 348);
    lv_label_set_long_mode(*note, LV_LABEL_LONG_MODE_DOTS);
    return value;
}

}  // namespace

lv_obj_t* DropScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    live_ = box(page, 0, 0, 1024, 456);
    create_reading(live_);
    create_scale(live_);
    create_chart(live_);
    create_side(live_);
    locked_ = locked_card(page, 16, 12, 992, 432, "Caída del tren", "Próximamente · fase 1.5", "P1 menos P2");
    return page;
}

void DropScreen::create_reading(lv_obj_t* page) {
    lv_obj_set_pos(label(page, &font_body_32, colors::text_secondary, "Contra cartuchos nuevos"), 24, 12);
    value_ = label(page, &font_digits_136, colors::text, "");
    lv_obj_set_pos(value_, 16, kValueY);
    unit_ = label(page, &font_title_36, colors::text_secondary, "%");
    lv_obj_set_y(unit_, 50);
    state_ = row(page, 8, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_y(state_, 102);
    state_icon_ = icon(state_, &icon_ok_32, colors::ok);
    state_word_ = label(state_, &font_title_36, colors::ok, "");
    note_ = label(page, &font_label_26, colors::text_disabled, "");
    lv_obj_set_width(note_, 576);
    lv_label_set_long_mode(note_, LV_LABEL_LONG_MODE_DOTS);
}

void DropScreen::create_scale(lv_obj_t* page) {
    lv_obj_t* scale = box(page, 24, 172, kScaleW, 88);
    for (size_t i = 0; i < zones_.size(); ++i) {
        const Zone& z = kZones[i];
        const int32_t x = scale_x(z.from);
        const int32_t w = scale_x(z.to) - x - (i + 1 < zones_.size() ? 1 : 0);
        zones_[i] = box(scale, x, 36, w, 24);
        fill(zones_[i], z.dim);
        zone_names_[i] = axis_label(scale, z.name, x + w / 2, 61, LV_TEXT_ALIGN_CENTER);
        lv_obj_set_style_text_color(zone_names_[i], color(colors::text_secondary), 0);
    }
    axis_label(scale, "100 %", scale_x(100), -3, LV_TEXT_ALIGN_CENTER);
    axis_label(scale, "150", scale_x(core::kClogWarningRatio * 100.0f), -3, LV_TEXT_ALIGN_CENTER);
    axis_label(scale, "200", scale_x(core::kClogAlarmRatio * 100.0f), -3, LV_TEXT_ALIGN_CENTER);
    axis_label(scale, "250 %", kScaleW, -3, LV_TEXT_ALIGN_RIGHT);
    marker_ = box(scale, 0, 26, 6, 44);
    lv_obj_set_style_radius(marker_, 2, 0);
    fill(marker_, colors::text);
}

void DropScreen::create_chart(lv_obj_t* page) {
    lv_obj_t* c = card(page, 16, 272, 600, 172);

    // Título y leyenda: la línea de R de cada día y la raya de nuevos
    lv_obj_t* head = row(c, 16, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(head, 16, 4);
    label(head, &font_label_26, colors::text_secondary, "R por día · 30 días");
    lv_obj_t* fresh = row(head, 6, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* dash = box(fresh, 0, 0, 24, 2);
    fill(box(dash, 0, 0, 8, 2), colors::text_disabled);
    fill(box(dash, 14, 0, 8, 2), colors::text_disabled);
    label(fresh, &font_label_26, colors::text_secondary, "nuevos");

    fill(box(c, px(kPlotX), px(kPlotBase) - 1, px(kPlotW) + 1, 2), colors::border_control);
    axis_label(c, "0", px(kPlotX) - 12, px(kPlotBase) - 16, LV_TEXT_ALIGN_RIGHT);
    top_label_ = axis_label(c, "", px(kPlotX) - 12, px(kPlotBase - kPlotH) - 16, LV_TEXT_ALIGN_RIGHT);
    fill(box(c, px(kPlotX), px(kPlotBase - kPlotH), px(kPlotW) + 1, 1), colors::border);
    axis_label(c, "hace 29 días", px(kPlotX) + 1, 145, LV_TEXT_ALIGN_LEFT);
    axis_label(c, "hoy", px(kPlotX + kPlotW) + 1, 145, LV_TEXT_ALIGN_RIGHT);

    plot_ = box(c, 0, 0, 598, 170);
    lv_obj_add_event_cb(plot_, draw_plot, LV_EVENT_DRAW_MAIN_END, this);
}

void DropScreen::create_side(lv_obj_t* page) {
    lv_obj_t* r = card(page, 628, 12, 380, 196);
    lv_obj_set_pos(label(r, &font_label_26, colors::text_secondary, "R, en psi por L/min"), 16, 10);
    last_value_ = r_row(r, "Último", 46, &last_note_);
    ref_value_ = r_row(r, "Cartuchos nuevos", 118, &ref_note_);

    lv_obj_t* todo = card(page, 628, 220, 380, 136);
    lv_obj_set_pos(label(todo, &font_label_26, colors::text_secondary, "Si pasa de 150 % (Revisar)"), 16, 10);
    lv_obj_t* steps = label(todo, &font_label_26, colors::text, "1. Purga el PRF\n2. Cambia el de 20 µm");
    lv_obj_set_pos(steps, 16, 46);

    lv_obj_t* filters = button_secondary(page, 628, 368, 380, "Ver cartuchos", nullptr, nullptr);
    lv_obj_set_height(filters, 76);
    lv_obj_move_to_index(icon(filters, &icon_cartucho_32, colors::text), 0);
    link_to(filters, Screen::filters, &styles::button_secondary_pressed);
}

void DropScreen::update(const core::SystemState& s) {
    lv_obj_set_hidden(live_, !s.has_pressure);
    lv_obj_set_hidden(locked_, s.has_pressure);
    if (!s.has_pressure) return;
    update_reading(s);
    update_side(s);
    update_chart(s);
}

// El % en grande con su estado. Sin flujo se queda el último, con su hora;
// sin referencia o sin señal no hay %: lo dice la palabra
void DropScreen::update_reading(const core::SystemState& s) {
    const ClogLook look = clog_look(s);
    const bool rated = look.kind <= ClogLook::clogged;
    lv_obj_set_hidden(value_, !rated);
    lv_obj_set_hidden(unit_, !rated);
    lv_obj_set_hidden(marker_, !rated);

    char buf[64];
    int32_t state_x = 24;
    int32_t state_y = 64;
    if (rated) {
        snprintf(buf, sizeof buf, "%d", look.pct);
        set_text(value_, buf);
        // "%" y el estado van 13 px después de la cifra, como en el canvas
        lv_point_t size;
        lv_text_get_size(&size, buf, &font_digits_136, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        lv_obj_set_x(unit_, 16 + size.x + 13);
        state_x = 16 + size.x + 13;
        state_y = 102;
        lv_obj_set_x(marker_, scale_x(static_cast<float>(look.pct)) - 3);
    }
    lv_obj_set_pos(state_, state_x, state_y);

    // Qué pasa, abajo del estado
    if (look.kind == ClogLook::no_signal) {
        snprintf(buf, sizeof buf, "%s sin señal: revisa su cable",
                 s.no_signal.p1 && s.no_signal.p2 ? "P1 y P2" : s.no_signal.p1 ? "P1" : "P2");
    } else if (look.kind == ClogLook::no_reference) {
        snprintf(buf, sizeof buf, "Guárdala en Ajustes > Cartuchos");
    } else if (look.kind == ClogLook::waiting) {
        snprintf(buf, sizeof buf, "Se califica al minuto de flujo estable");
    } else if (!look.flowing) {
        snprintf(buf, sizeof buf, "Sin flujo · %s", look.at);  // caben 286 px junto a un % de 3 cifras
    } else {
        snprintf(buf, sizeof buf, "Con flujo · %s", look.at);
    }
    set_text(note_, buf);
    // Hasta la orilla de la escala (x 600): a la derecha va la tarjeta de R
    const int32_t note_x = rated ? state_x : 24;
    lv_obj_set_pos(note_, note_x, rated ? 142 : 112);
    lv_obj_set_width(note_, 600 - note_x);

    const char* word = look.kind == ClogLook::waiting ? "Esperando flujo" : look.word;
    set_text(state_word_, word);
    if (look.kind == kind_) return;
    kind_ = look.kind;
    for (size_t i = 0; i < zones_.size(); ++i) {
        const bool on = static_cast<int>(i) == look.kind;
        lv_obj_set_style_border_width(zones_[i], on ? 2 : 0, 0);
        lv_obj_set_style_border_color(zones_[i], color(look.fg), 0);
        lv_obj_set_style_text_color(zone_names_[i], color(on ? look.fg : colors::text_secondary), 0);
    }
    // El de 32 del mismo estado que el de 28 de clog_look
    const lv_image_dsc_t* src = look.kind == ClogLook::normal    ? &icon_ok_32
                                : look.kind == ClogLook::clogged ? &icon_alarma_32
                                : look.kind == ClogLook::check || look.kind == ClogLook::no_signal
                                    ? &icon_advertencia_32
                                    : &icon_info_32;
    lv_image_set_src(state_icon_, src);
    set_icon_color(state_icon_, look.fg);
    lv_obj_set_style_text_color(state_word_, color(look.fg), 0);
}

// El último R (con su caída y caudal) y la referencia (con su caudal y fecha)
void DropScreen::update_side(const core::SystemState& s) {
    char buf[48];
    if (s.clog_r > 0.0f) {
        snprintf(buf, sizeof buf, "%.1f", static_cast<double>(s.clog_r));
        set_text(last_value_, buf);
        char at[8];
        fmt::clock(at, sizeof at, s.clog_r_at);
        const float psi = s.clog_r * s.clog_r_lpm;
        snprintf(buf, sizeof buf, "%.0f psi a %.1f L/min · %s", static_cast<double>(psi), static_cast<double>(s.clog_r_lpm), at);
        set_text(last_note_, buf);
    } else {
        set_text(last_value_, "--");
        set_text(last_note_, "Al minuto de flujo estable");
    }
    if (s.clog_r0 > 0.0f) {
        snprintf(buf, sizeof buf, "%.1f", static_cast<double>(s.clog_r0));
        set_text(ref_value_, buf);
        const uint32_t ago = s.today >= s.clog_r0_day ? s.today - s.clog_r0_day : 0;
        const core::Date d = core::date_days_ago(core::today(s), s.year, ago);
        snprintf(buf, sizeof buf, "%.1f L/min · %u %s", static_cast<double>(s.clog_r0_lpm), static_cast<unsigned>(d.day),
                 fmt::month_short(d.month));
        set_text(ref_note_, buf);
    } else {
        set_text(ref_value_, "--");
        set_text(ref_note_, "Sin guardar todavía");
    }
}

// R de cada día del último mes. El eje llega a 2.5 veces la referencia (o
// más si un día no cabe); sin referencia, a 1.5 veces el R más alto
void DropScreen::update_chart(const core::SystemState& s) {
    float most = 0.0f;
    for (const uint16_t r : s.clog_days) most = std::max(most, static_cast<float>(r) / 100.0f);
    float top = s.clog_r0 > 0.0f ? std::max(s.clog_r0 * 2.5f, most * 1.1f) : std::max(most * 1.5f, 1.0f);
    top = std::ceil(top);
    if (top != plot_max_) {
        plot_max_ = top;
        char buf[8];
        snprintf(buf, sizeof buf, "%.0f", static_cast<double>(top));
        set_text(top_label_, buf);
    }
    auto y_of = [top](float r) { return kPlotBase - std::clamp(r / top, 0.0f, 1.0f) * kPlotH; };
    const auto n = static_cast<int>(s.clog_days.size());
    for (int i = 0; i < n; ++i) {
        const uint16_t r = s.clog_days[static_cast<size_t>(i)];
        has_point_[static_cast<size_t>(i)] = r > 0;
        points_[static_cast<size_t>(i)] = {static_cast<lv_value_precise_t>(kPlotX + kPlotW * i / (n - 1)),
                                           static_cast<lv_value_precise_t>(y_of(static_cast<float>(r) / 100.0f))};
    }
    const float ratios[3] = {1.0f, core::kClogWarningRatio, core::kClogAlarmRatio};
    for (size_t i = 0; i < lines_y_.size(); ++i) {
        lines_y_[i] = s.clog_r0 > 0.0f ? px(y_of(s.clog_r0 * ratios[i])) : -1;
    }
    lv_obj_invalidate(plot_);
}

void DropScreen::draw_plot(lv_event_t* e) {
    const auto* self = static_cast<DropScreen*>(lv_event_get_user_data(e));
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_area_t area;
    lv_obj_get_coords(self->plot_, &area);
    const float x0 = static_cast<float>(area.x1);
    const float y0 = static_cast<float>(area.y1);

    // Nuevos punteada; 150 y 200 % delgadas en su color
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    const uint32_t tones[3] = {colors::text_disabled, colors::warning, colors::alarm};
    for (size_t i = 0; i < self->lines_y_.size(); ++i) {
        if (self->lines_y_[i] < 0) continue;
        line.color = color(tones[i]);
        line.width = i == 0 ? 2 : 1;
        line.dash_width = i == 0 ? 8 : 0;
        line.dash_gap = i == 0 ? 6 : 0;
        line.p1 = {static_cast<lv_value_precise_t>(x0 + kPlotX), static_cast<lv_value_precise_t>(y0 + self->lines_y_[i])};
        line.p2 = {static_cast<lv_value_precise_t>(x0 + kPlotX + kPlotW),
                   static_cast<lv_value_precise_t>(y0 + self->lines_y_[i])};
        lv_draw_line(layer, &line);
    }

    // R de cada día: línea entre días seguidos con agua y un punto en cada uno
    line.color = color(colors::text);
    line.width = 3;
    line.dash_width = 0;
    line.dash_gap = 0;
    line.round_start = 1;
    line.round_end = 1;
    lv_draw_rect_dsc_t dot;
    lv_draw_rect_dsc_init(&dot);
    dot.radius = LV_RADIUS_CIRCLE;
    dot.bg_opa = LV_OPA_COVER;
    dot.bg_color = color(colors::text);
    for (size_t i = 0; i < self->points_.size(); ++i) {
        if (!self->has_point_[i]) continue;
        const lv_point_precise_t& p = self->points_[i];
        if (i > 0 && self->has_point_[i - 1]) {
            const lv_point_precise_t& q = self->points_[i - 1];
            line.p1 = {static_cast<lv_value_precise_t>(x0 + q.x), static_cast<lv_value_precise_t>(y0 + q.y)};
            line.p2 = {static_cast<lv_value_precise_t>(x0 + p.x), static_cast<lv_value_precise_t>(y0 + p.y)};
            lv_draw_line(layer, &line);
        }
        const int32_t cx = px(x0 + static_cast<float>(p.x));
        const int32_t cy = px(y0 + static_cast<float>(p.y));
        const lv_area_t a = {cx - 4, cy - 4, cx + 3, cy + 3};
        lv_draw_rect(layer, &dot, &a);
    }
}

}  // namespace ui
