#include "screens/pressure_screen.h"

#include <algorithm>
#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "theme/styles.h"
#include "ui.h"

// Medidas tomadas del artboard DetalleP del canvas (cada y menos 64).
// Las coordenadas dentro de una tarjeta cuentan desde adentro del borde.

namespace ui {
namespace {

constexpr int32_t kValueY = 44 + 10;  // cifras de 136 bajadas como en Cisterna

// Gráfica de 24 h dentro de su tarjeta: de 0 a 60 psi
constexpr int32_t kPlotX = 71;
constexpr int32_t kPlotY = 42;    // y de 60 psi
constexpr int32_t kPlotW = 512;
constexpr int32_t kPlotH = 126;   // de 60 a 0
constexpr int32_t kPlotPad = 4;
constexpr int32_t kPlotMax = 60;
constexpr int32_t kHoursY = 175;  // labels de las horas

int32_t plot_y(int32_t psi) { return kPlotY + kPlotH - psi * kPlotH / kPlotMax; }
int32_t sample_x(int i) { return kPlotX + kPlotW * i / 23; }

// Label de 26 px en texto-3 de 100 px de ancho con su alineación
lv_obj_t* axis_label(lv_obj_t* parent, lv_text_align_t align) {
    lv_obj_t* obj = label(parent, &font_label_26, colors::text_disabled, "");
    lv_obj_set_width(obj, 100);
    lv_obj_set_style_text_align(obj, align, 0);
    return obj;
}

// "A0 · P1   1.224 V"
lv_obj_t* volts_row(lv_obj_t* parent, const char* name) {
    lv_obj_t* r = row(parent, 8, LV_FLEX_ALIGN_END);
    lv_obj_set_width(r, 348);
    lv_obj_set_flex_grow(label(r, &font_label_26, colors::text_secondary, name), 1);
    lv_obj_t* value = label(r, &font_body_32, colors::text, "");
    label(r, &font_label_26, colors::text_secondary, "V");
    return value;
}

}  // namespace

lv_obj_t* PressureScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    live_ = box(page, 0, 0, 1024, 456);
    create_readings(live_);
    create_drop(live_);
    create_chart(live_);
    create_side(live_);
    locked_ = locked_card(page, 16, 12, 992, 432, "Presión P1 y P2", "Próximamente · fase 1.5",
                          "Presión antes y después del tren de filtros");
    return page;
}

void PressureScreen::create_readings(lv_obj_t* page) {
    lv_obj_set_pos(label(page, &font_body_32, colors::text_secondary, "P1 · entrada"), 24, 12);
    p1_ = label(page, &font_digits_136, colors::text, "");
    lv_obj_set_pos(p1_, 16, kValueY);
    lv_obj_set_pos(label(page, &font_title_36, colors::text_secondary, "psi"), 214, 50);

    lv_obj_set_pos(label(page, &font_body_32, colors::text_secondary, "P2 · retorno"), 324, 12);
    p2_ = label(page, &font_digits_136, colors::text, "");
    lv_obj_set_pos(p2_, 316, kValueY);
    lv_obj_set_pos(label(page, &font_title_36, colors::text_secondary, "psi"), 514, 50);
}

void PressureScreen::create_drop(lv_obj_t* page) {
    // Barra de la caída: el color y la palabra dicen si está bien; abre su detalle
    drop_ = box(page, 16, 172, 600, 56);
    lv_obj_set_style_radius(drop_, 6, 0);
    lv_obj_set_style_border_width(drop_, 2, 0);
    lv_obj_set_style_bg_opa(drop_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(drop_, 16, 0);
    lv_obj_set_style_pad_right(drop_, 12, 0);
    lv_obj_set_flex_flow(drop_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(drop_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(drop_, 8, 0);
    link_to(drop_, Screen::pressure_drop, &styles::button_secondary_pressed);
    label(drop_, &font_label_26, colors::text_secondary, "Caída del tren");
    drop_value_ = label(drop_, &font_digits_44, colors::text, "");
    label(drop_, &font_label_26, colors::text_secondary, "psi");
    drop_word_ = label(drop_, &font_label_26, colors::ok, "");
    lv_obj_set_flex_grow(drop_word_, 1);
    lv_obj_set_style_margin_left(drop_word_, 4, 0);
    icon(drop_, &icon_chevron_32, colors::text_secondary);
}

void PressureScreen::create_chart(lv_obj_t* page) {
    lv_obj_t* c = card(page, 16, 240, 600, 204);

    // Título y leyenda en un renglón
    lv_obj_t* head = row(c, 18, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(head, 16, 4);
    label(head, &font_label_26, colors::text_secondary, "Últimas 24 h · psi");
    for (int i = 0; i < 2; ++i) {
        lv_obj_t* item = row(head, 8, LV_FLEX_ALIGN_CENTER);
        lv_obj_t* swatch = box(item, 0, 0, 24, 4);
        fill(swatch, i == 0 ? colors::text : colors::text_secondary);
        label(item, &font_label_26, colors::text_secondary, i == 0 ? "P1" : "P2");
    }

    // Noche de 0:00 a 6:00 abajo de todo
    for (lv_obj_t*& band : nights_) {
        band = box(c, 0, kPlotY, 0, kPlotH);
        fill(band, colors::panel_raised);
    }
    night_label_ = axis_label(c, LV_TEXT_ALIGN_CENTER);
    set_text(night_label_, "noche");
    lv_obj_set_y(night_label_, 39);

    for (const int32_t psi : {60, 40, 20, 0}) {
        const int32_t y = plot_y(psi);
        lv_obj_t* value = axis_label(c, LV_TEXT_ALIGN_RIGHT);
        lv_label_set_text_fmt(value, "%d", static_cast<int>(psi));
        lv_obj_set_pos(value, kPlotX - 12 - 100, y - 16);
        if (psi == 0) {
            fill(box(c, kPlotX, y - 1, kPlotW + 1, 2), colors::border_control);
        } else {
            fill(box(c, kPlotX, y, kPlotW + 1, 1), colors::border);
        }
    }

    chart_ = lv_chart_create(c);
    lv_obj_remove_style_all(chart_);
    lv_obj_set_clickable(chart_, false);
    lv_obj_set_scrollable(chart_, false);
    lv_obj_set_pos(chart_, kPlotX - kPlotPad, kPlotY - kPlotPad);
    lv_obj_set_size(chart_, kPlotW + 2 * kPlotPad, kPlotH + 2 * kPlotPad);
    lv_obj_set_style_pad_all(chart_, kPlotPad, 0);
    lv_chart_set_type(chart_, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(chart_, 0, 0);
    lv_chart_set_point_count(chart_, 24);
    lv_chart_set_axis_range(chart_, LV_CHART_AXIS_PRIMARY_Y, 0, kPlotMax);
    lv_obj_set_style_line_width(chart_, 3, LV_PART_ITEMS);
    lv_obj_set_style_line_rounded(chart_, true, LV_PART_ITEMS);
    lv_obj_set_style_size(chart_, 0, 0, LV_PART_INDICATOR);
    // P2 primero para que P1 quede encima
    p2_series_ = lv_chart_add_series(chart_, color(colors::text_secondary), LV_CHART_AXIS_PRIMARY_Y);
    p1_series_ = lv_chart_add_series(chart_, color(colors::text), LV_CHART_AXIS_PRIMARY_Y);

    for (lv_obj_t*& l : hour_labels_) {
        l = axis_label(c, LV_TEXT_ALIGN_CENTER);
        lv_obj_set_y(l, kHoursY);
    }
}

void PressureScreen::create_side(lv_obj_t* page) {
    lv_obj_t* a = card(page, 628, 12, 380, 200);
    lv_obj_set_pos(label(a, &font_label_26, colors::text_secondary, "Señal en el ADS1115"), 16, 10);
    lv_obj_t* rows = column(a, 6, LV_FLEX_ALIGN_START);
    lv_obj_set_pos(rows, 16, 48);
    a0_ = volts_row(rows, "A0 · P1");
    a1_ = volts_row(rows, "A1 · P2");
    lv_obj_t* note = label(a, &font_label_26, colors::text_disabled, "SEN0257: 0.5 a 4.5 V es 0 a 232 psi");
    lv_obj_set_width(note, 348);
    lv_obj_set_pos(note, 16, 132);

    lv_obj_t* z = card(page, 628, 224, 380, 220);
    lv_obj_set_pos(label(z, &font_label_26, colors::text_secondary, "Cero de los sensores"), 16, 10);
    lv_obj_t* r = row(z, 8, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(r, 16, 46);
    label(r, &font_label_26, colors::text_secondary, "P1");
    zero_p1_ = label(r, &font_digits_44, colors::text, "");
    lv_obj_set_style_margin_left(label(r, &font_label_26, colors::text_secondary, "P2"), 16, 0);
    zero_p2_ = label(r, &font_digits_44, colors::text, "");
    label(r, &font_label_26, colors::text_secondary, "psi");
    lv_obj_t* zero = button_secondary(z, 16, 98, 348, "Hacer cero", [](lv_event_t*) { ask_pressure_zero(); }, nullptr);
    lv_obj_set_height(zero, 64);
    lv_obj_t* note2 = label(z, &font_label_26, colors::text_secondary, "Con V1 cerrada, sin presión");  // "y sin" no cabe en 348 px
    lv_obj_set_pos(note2, 16, 172);
}

void PressureScreen::update(const core::SystemState& s) {
    lv_obj_set_hidden(live_, !s.has_pressure);
    lv_obj_set_hidden(locked_, s.has_pressure);
    if (!s.has_pressure) return;

    char buf[16];
    snprintf(buf, sizeof buf, "%d", s.p1_psi);
    set_text(p1_, buf);
    snprintf(buf, sizeof buf, "%d", s.p2_psi);
    set_text(p2_, buf);

    const int drop = s.p1_psi - s.p2_psi;
    snprintf(buf, sizeof buf, "%d", drop);
    set_text(drop_value_, buf);
    const core::Level level = core::drop_level(s);
    if (static_cast<int>(level) != drop_level_) {
        drop_level_ = static_cast<int>(level);
        const uint32_t fg = level == core::Level::ok ? colors::ok : level == core::Level::warning ? colors::warning
                                                                                                  : colors::alarm;
        const uint32_t bg = level == core::Level::ok      ? colors::ok_bg
                            : level == core::Level::warning ? colors::warning_bg
                                                            : colors::alarm_bg;
        lv_obj_set_style_bg_color(drop_, color(bg), 0);
        lv_obj_set_style_border_color(drop_, color(fg), 0);
        set_text(drop_word_, level == core::Level::ok ? "Normal" : level == core::Level::warning ? "Revisar" : "Tapados");
        lv_obj_set_style_text_color(drop_word_, color(fg), 0);
    }

    update_chart(s);

    snprintf(buf, sizeof buf, "%.3f", static_cast<double>(s.p1_volts));
    set_text(a0_, buf);
    snprintf(buf, sizeof buf, "%.3f", static_cast<double>(s.p2_volts));
    set_text(a1_, buf);
    snprintf(buf, sizeof buf, "%.1f", static_cast<double>(s.p1_zero_psi));
    set_text(zero_p1_, buf);
    snprintf(buf, sizeof buf, "%.1f", static_cast<double>(s.p2_zero_psi));
    set_text(zero_p2_, buf);
}

void PressureScreen::update_chart(const core::SystemState& s) {
    if (s.p1_24h == shown_p1_ && s.p2_24h == shown_p2_ && s.hour == shown_hour_) return;
    shown_p1_ = s.p1_24h;
    shown_p2_ = s.p2_24h;
    shown_hour_ = s.hour;

    int32_t* p1 = lv_chart_get_series_y_array(chart_, p1_series_);
    int32_t* p2 = lv_chart_get_series_y_array(chart_, p2_series_);
    for (size_t i = 0; i < 24; ++i) {
        p1[i] = s.p1_24h[i];
        p2[i] = s.p2_24h[i];
    }
    lv_chart_refresh(chart_);

    // La muestra i es de la hora first + i; la última es la de ahorita
    const int first = (s.hour + 1) % 24;
    auto hour_of = [first](int i) { return (first + i) % 24; };

    // Noche: de 0:00 a 6:00. Puede quedar partida en las dos orillas.
    int bands = 0;
    for (int i = 0; i < 24 && bands < 2; ++i) {
        const bool night = hour_of(i) < 6;
        const bool starts = night && (i == 0 || hour_of(i - 1) >= 6);
        if (!starts) continue;
        const int end = std::min(23, i + (6 - hour_of(i)));
        lv_obj_set_hidden(nights_[static_cast<size_t>(bands)], false);
        lv_obj_set_x(nights_[static_cast<size_t>(bands)], sample_x(i));
        lv_obj_set_width(nights_[static_cast<size_t>(bands)], sample_x(end) - sample_x(i));
        if (bands == 0) lv_obj_set_x(night_label_, (sample_x(i) + sample_x(end)) / 2 - 50);
        ++bands;
    }
    for (int b = bands; b < 2; ++b) lv_obj_set_hidden(nights_[static_cast<size_t>(b)], true);
    lv_obj_set_hidden(night_label_, bands == 0);

    // Horas abajo: la primera y la última en las orillas, y 0:00, 6:00, 12:00
    // y 18:00 donde caigan si no chocan con ellas
    char buf[8];
    int n = 0;
    auto put = [this, &n, &buf](int i, lv_text_align_t align) {
        lv_obj_t* l = hour_labels_[static_cast<size_t>(n++)];
        lv_obj_set_hidden(l, false);
        set_text(l, buf);
        lv_obj_set_style_text_align(l, align, 0);
        const int32_t x = sample_x(i) + 1;  // + 1: el canvas cuenta sin el borde
        lv_obj_set_x(l, align == LV_TEXT_ALIGN_LEFT ? x : align == LV_TEXT_ALIGN_RIGHT ? x - 100 : x - 50);
    };
    snprintf(buf, sizeof buf, "%d:00", hour_of(0));
    put(0, LV_TEXT_ALIGN_LEFT);
    for (int i = 1; i < 23 && n < 4; ++i) {
        if (hour_of(i) % 6 != 0) continue;
        if (sample_x(i) - sample_x(0) < 80 || sample_x(23) - sample_x(i) < 80) continue;
        snprintf(buf, sizeof buf, "%d:00", hour_of(i));
        put(i, LV_TEXT_ALIGN_CENTER);
    }
    snprintf(buf, sizeof buf, "%d:00", hour_of(23));
    put(23, LV_TEXT_ALIGN_RIGHT);
    for (int i = n; i < static_cast<int>(hour_labels_.size()); ++i) lv_obj_set_hidden(hour_labels_[static_cast<size_t>(i)], true);
}

}  // namespace ui
