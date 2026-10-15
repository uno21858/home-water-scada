#include "screens/drop_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "theme/styles.h"

// Medidas tomadas del artboard DetalleCaida del canvas (cada y menos 64).
// Las coordenadas dentro de una tarjeta cuentan desde adentro del borde.

namespace ui {
namespace {

constexpr int32_t kValueY = 44 + 10;  // cifra de 136 bajada como en Cisterna

// Escala de 0 a 15 psi en 576 px, tres zonas de 5 psi
constexpr int32_t kScaleW = 576;
int32_t scale_x(float psi) { return static_cast<int32_t>(std::lround(std::clamp(psi, 0.0f, 15.0f) * kScaleW / 15.0f)); }

struct Zone {
    int32_t x;
    int32_t w;
    const char* name;
    uint32_t dim;
    uint32_t lit;
    const lv_image_dsc_t* icon;
};
const Zone kZones[3] = {
    {0, 192, "Normal", colors::ok_bg, colors::ok, &icon_ok_32},
    {193, 191, "Revisar", colors::warning_bg, colors::warning, &icon_advertencia_32},
    {385, 191, "Tapados", colors::alarm_bg, colors::alarm, &icon_alarma_32},
};

// Gráfica dentro de su tarjeta: caudal de 0 a 15 L/min y caída de 0 a 6 psi
constexpr float kPlotX = 71.0f;
constexpr float kPlotW = 512.0f;
constexpr float kPlotBase = 142.0f;  // y de 0 psi
constexpr float kPlotH = 102.0f;     // de 0 a 6 psi
float chart_x(float lpm) { return kPlotX + std::clamp(lpm, 0.0f, 15.0f) / 15.0f * kPlotW; }
float chart_y(float psi) { return kPlotBase - std::clamp(psi, 0.0f, 6.0f) / 6.0f * kPlotH; }
int32_t px(float v) { return static_cast<int32_t>(std::lround(v)); }

// Caída con cartuchos nuevos a cualquier caudal, entre las de cada L/min
float new_drop(const core::SystemState& s, float lpm) {
    const float q = std::clamp(lpm, 0.0f, 15.0f);
    const int i = std::min(14, static_cast<int>(q));
    const float a = s.drop_new_tenths[static_cast<size_t>(i)] / 10.0f;
    const float b = s.drop_new_tenths[static_cast<size_t>(i + 1)] / 10.0f;
    return a + (b - a) * (q - static_cast<float>(i));
}

lv_obj_t* axis_label(lv_obj_t* parent, const char* text, int32_t x, int32_t y, lv_text_align_t align) {
    lv_obj_t* obj = label(parent, &font_label_26, colors::text_disabled, text);
    lv_obj_set_width(obj, 120);
    lv_obj_set_style_text_align(obj, align, 0);
    lv_obj_set_pos(obj, align == LV_TEXT_ALIGN_LEFT ? x : align == LV_TEXT_ALIGN_RIGHT ? x - 120 : x - 60, y);
    return obj;
}

lv_obj_t* psi_row(lv_obj_t* parent, const char* name) {
    lv_obj_t* r = row(parent, 8, LV_FLEX_ALIGN_END);
    lv_obj_set_width(r, 348);
    lv_obj_set_flex_grow(label(r, &font_label_26, colors::text_secondary, name), 1);
    lv_obj_t* value = label(r, &font_body_32, colors::text, "");
    label(r, &font_label_26, colors::text_secondary, "psi");
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
    lv_obj_set_pos(label(page, &font_body_32, colors::text_secondary, "Caída del tren"), 24, 12);
    value_ = label(page, &font_digits_136, colors::text, "");
    lv_obj_set_pos(value_, 16, kValueY);
    unit_ = label(page, &font_title_36, colors::text_secondary, "psi");
    lv_obj_set_y(unit_, 50);
    state_ = row(page, 8, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_y(state_, 102);
    state_icon_ = icon(state_, &icon_ok_32, colors::ok);
    state_word_ = label(state_, &font_title_36, colors::ok, "");

    formula_ = label(page, &font_body_32, colors::text_secondary, "");
    lv_obj_set_pos(formula_, 344, 56);
    flow_note_ = label(page, &font_label_26, colors::text_disabled, "");
    lv_obj_set_pos(flow_note_, 344, 98);
}

void DropScreen::create_scale(lv_obj_t* page) {
    lv_obj_t* scale = box(page, 24, 172, kScaleW, 88);
    for (size_t i = 0; i < zones_.size(); ++i) {
        const Zone& z = kZones[i];
        zones_[i] = box(scale, z.x, 36, z.w, 24);
        fill(zones_[i], z.dim);
        lv_obj_set_style_border_color(zones_[i], color(z.lit), 0);
        zone_names_[i] = axis_label(scale, z.name, z.x + z.w / 2, 61, LV_TEXT_ALIGN_CENTER);
        lv_obj_set_style_text_color(zone_names_[i], color(colors::text_secondary), 0);
    }
    axis_label(scale, "0", 0, -3, LV_TEXT_ALIGN_LEFT);
    axis_label(scale, "5", scale_x(5), -3, LV_TEXT_ALIGN_CENTER);
    axis_label(scale, "10", scale_x(10), -3, LV_TEXT_ALIGN_CENTER);
    axis_label(scale, "15 psi", kScaleW, -3, LV_TEXT_ALIGN_RIGHT);
    marker_ = box(scale, 0, 26, 6, 44);
    lv_obj_set_style_radius(marker_, 2, 0);
    fill(marker_, colors::text);
}

void DropScreen::create_chart(lv_obj_t* page) {
    lv_obj_t* c = card(page, 16, 272, 600, 172);

    // Título y leyenda: punto de los últimos 3 días y raya punteada de nuevos
    lv_obj_t* head = row(c, 16, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(head, 16, 4);
    label(head, &font_label_26, colors::text_secondary, "Caída contra caudal");
    lv_obj_t* recent = row(head, 6, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* dot = box(recent, 0, 0, 10, 10);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    fill(dot, colors::text);
    label(recent, &font_label_26, colors::text_secondary, "3 días");
    lv_obj_t* fresh = row(head, 6, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* dash = box(fresh, 0, 0, 24, 2);
    fill(box(dash, 0, 0, 8, 2), colors::text_disabled);
    fill(box(dash, 14, 0, 8, 2), colors::text_disabled);
    label(fresh, &font_label_26, colors::text_secondary, "nuevos");

    for (const int psi : {6, 3, 0}) {
        const int32_t y = px(chart_y(static_cast<float>(psi)));
        char buf[4];
        snprintf(buf, sizeof buf, "%d", psi);
        axis_label(c, buf, px(kPlotX) - 12, y - 16, LV_TEXT_ALIGN_RIGHT);
        if (psi == 0) {
            fill(box(c, px(kPlotX), y - 1, px(kPlotW) + 1, 2), colors::border_control);
        } else {
            fill(box(c, px(kPlotX), y, px(kPlotW) + 1, 1), colors::border);
        }
    }
    // Caudal abajo (x del canvas, sin el borde: + 1)
    axis_label(c, "0", px(chart_x(0)) + 1, 145, LV_TEXT_ALIGN_LEFT);
    axis_label(c, "5", px(chart_x(5)) + 1, 145, LV_TEXT_ALIGN_CENTER);
    axis_label(c, "10", px(chart_x(10)) + 1, 145, LV_TEXT_ALIGN_CENTER);
    axis_label(c, "15 L/min", px(chart_x(15)) + 1, 145, LV_TEXT_ALIGN_RIGHT);

    plot_ = box(c, 0, 0, 598, 170);
    lv_obj_add_event_cb(plot_, draw_plot, LV_EVENT_DRAW_MAIN_END, this);
}

void DropScreen::create_side(lv_obj_t* page) {
    lv_obj_t* same = card(page, 628, 12, 380, 196);
    same_flow_title_ = label(same, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(same_flow_title_, 16, 10);
    lv_obj_t* rows = column(same, 8, LV_FLEX_ALIGN_START);
    lv_obj_set_pos(rows, 16, 50);
    new_value_ = psi_row(rows, "Cartuchos nuevos");
    now_value_ = psi_row(rows, "Hoy");
    rise_ = label(same, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(rise_, 16, 144);

    lv_obj_t* todo = card(page, 628, 220, 380, 136);
    lv_obj_set_pos(label(todo, &font_label_26, colors::text_secondary, "Si llega a Revisar"), 16, 10);
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

    char buf[64];
    const int drop = s.p1_psi - s.p2_psi;
    snprintf(buf, sizeof buf, "%d", drop);
    set_text(value_, buf);

    // "psi" y el estado van 13 px después de la cifra, como en el canvas
    lv_point_t size;
    lv_text_get_size(&size, buf, &font_digits_136, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_obj_set_x(unit_, 16 + size.x + 13);
    lv_obj_set_x(state_, 16 + size.x + 13);
    lv_obj_set_x(marker_, scale_x(static_cast<float>(drop)) - 3);

    snprintf(buf, sizeof buf, "P1 %d \xE2\x88\x92 P2 %d", s.p1_psi, s.p2_psi);
    set_text(formula_, buf);
    const bool flowing = s.flow_lpm >= 0.05f;
    if (flowing) {
        snprintf(buf, sizeof buf, "con %.1f L/min pasando", static_cast<double>(s.flow_lpm));
    } else {
        snprintf(buf, sizeof buf, "sin agua pasando");
    }
    set_text(flow_note_, buf);

    const core::Level level = core::pressure_drop_level(drop);
    if (static_cast<int>(level) != level_) {
        level_ = static_cast<int>(level);
        const int zone = static_cast<int>(level);
        for (int i = 0; i < 3; ++i) {
            const bool on = i == zone;
            lv_obj_set_style_border_width(zones_[static_cast<size_t>(i)], on ? 2 : 0, 0);
            lv_obj_set_style_text_color(zone_names_[static_cast<size_t>(i)],
                                        color(on ? kZones[i].lit : colors::text_secondary), 0);
        }
        const Zone& z = kZones[zone];
        lv_image_set_src(state_icon_, z.icon);
        set_icon_color(state_icon_, z.lit);
        lv_label_set_text(state_word_, z.name);
        lv_obj_set_style_text_color(state_word_, color(z.lit), 0);
    }

    // Al mismo caudal: la caída de nuevos contra la de hoy
    snprintf(buf, sizeof buf, "Al mismo caudal, %.1f L/min", static_cast<double>(s.flow_lpm));
    set_text(same_flow_title_, buf);
    const float fresh = new_drop(s, s.flow_lpm);
    snprintf(buf, sizeof buf, "%.1f", static_cast<double>(fresh));
    set_text(new_value_, buf);
    snprintf(buf, sizeof buf, "%.1f", static_cast<double>(drop));
    set_text(now_value_, buf);
    // Desde cuándo: el día que se midió con cartuchos nuevos (0 = no hay)
    const bool measured = s.drop_new_day != 0 && s.drop_new_day <= s.today;
    lv_obj_set_hidden(rise_, !measured);
    if (measured) {
        const core::Date since = core::date_days_ago(core::today(s), s.year, s.today - s.drop_new_day);
        const float rise = static_cast<float>(drop) - fresh;
        snprintf(buf, sizeof buf, "%s %.1f psi desde el %u %s", rise >= 0 ? "Subió" : "Bajó",
                 static_cast<double>(std::fabs(rise)), static_cast<unsigned>(since.day), fmt::month_short(since.month));
        set_text(rise_, buf);
    }

    update_chart(s);
}

void DropScreen::update_chart(const core::SystemState& s) {
    for (size_t q = 0; q < curve_.size(); ++q) {
        curve_[q] = {static_cast<lv_value_precise_t>(chart_x(static_cast<float>(q))),
                     static_cast<lv_value_precise_t>(chart_y(s.drop_new_tenths[q] / 10.0f))};
    }
    dot_count_ = s.drop_sample_count;
    for (int i = 0; i < dot_count_; ++i) {
        const core::DropSample& d = s.drop_samples[static_cast<size_t>(i)];
        dots_[static_cast<size_t>(i)] = {px(chart_x(d.flow_tenths / 10.0f)), px(chart_y(d.drop_tenths / 10.0f)),
                                         s.today - d.day < 3};
    }
    today_shown_ = s.flow_lpm >= 0.05f;
    today_x_ = px(chart_x(s.flow_lpm));
    today_y_ = px(chart_y(static_cast<float>(s.p1_psi - s.p2_psi)));
    lv_obj_invalidate(plot_);
}

void DropScreen::draw_plot(lv_event_t* e) {
    auto* self = static_cast<DropScreen*>(lv_event_get_user_data(e));
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_area_t area;
    lv_obj_get_coords(self->plot_, &area);
    const float x0 = static_cast<float>(area.x1);
    const float y0 = static_cast<float>(area.y1);

    // Curva de cartuchos nuevos punteada: 8 px de raya y 6 de hueco a lo largo
    // de la curva. LVGL solo puntea líneas horizontales o verticales.
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.width = 2;
    line.color = color(colors::text_disabled);
    float carry = 0.0f;  // cuánto lleva el patrón de raya y hueco
    for (size_t i = 0; i + 1 < self->curve_.size(); ++i) {
        const float ax = static_cast<float>(self->curve_[i].x);
        const float ay = static_cast<float>(self->curve_[i].y);
        const float bx = static_cast<float>(self->curve_[i + 1].x);
        const float by = static_cast<float>(self->curve_[i + 1].y);
        const float len = std::hypot(bx - ax, by - ay);
        float t = 0.0f;
        while (t < len) {
            const bool on = carry < 8.0f;
            const float step = std::min(len - t, on ? 8.0f - carry : 14.0f - carry);
            if (on) {
                line.p1 = {static_cast<lv_value_precise_t>(x0 + ax + (bx - ax) * t / len),
                           static_cast<lv_value_precise_t>(y0 + ay + (by - ay) * t / len)};
                line.p2 = {static_cast<lv_value_precise_t>(x0 + ax + (bx - ax) * (t + step) / len),
                           static_cast<lv_value_precise_t>(y0 + ay + (by - ay) * (t + step) / len)};
                lv_draw_line(layer, &line);
            }
            t += step;
            carry = std::fmod(carry + step, 14.0f);
        }
    }

    // Puntos de 8 px: los viejos en tubería y los de los últimos 3 días en texto
    lv_draw_rect_dsc_t dot;
    lv_draw_rect_dsc_init(&dot);
    dot.radius = LV_RADIUS_CIRCLE;
    dot.bg_opa = LV_OPA_COVER;
    for (const bool recent : {false, true}) {
        dot.bg_color = color(recent ? colors::text : colors::pipe);
        for (int i = 0; i < self->dot_count_; ++i) {
            const Dot& d = self->dots_[static_cast<size_t>(i)];
            if (d.recent != recent) continue;
            const lv_area_t a = {area.x1 + d.x - 4, area.y1 + d.y - 4, area.x1 + d.x + 3, area.y1 + d.y + 3};
            lv_draw_rect(layer, &dot, &a);
        }
    }

    // Hoy: un aro alrededor de la caída de ahorita
    if (self->today_shown_) {
        lv_draw_rect_dsc_t ring;
        lv_draw_rect_dsc_init(&ring);
        ring.radius = LV_RADIUS_CIRCLE;
        ring.bg_opa = LV_OPA_TRANSP;
        ring.border_color = color(colors::text);
        ring.border_width = 3;
        ring.border_opa = LV_OPA_COVER;
        const lv_area_t a = {area.x1 + self->today_x_ - 10, area.y1 + self->today_y_ - 10,
                             area.x1 + self->today_x_ + 10, area.y1 + self->today_y_ + 10};
        lv_draw_rect(layer, &ring, &a);
    }
}

}  // namespace ui
