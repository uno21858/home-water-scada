#include "screens/cistern_screen.h"

#include <algorithm>
#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "ui.h"

// Medidas tomadas del artboard Cisterna del canvas (cada y menos 64).
// Las coordenadas dentro de una tarjeta cuentan desde adentro del borde.

namespace ui {
namespace {

// Cifra de 136 px: como la de 232 en Flujo, LVGL la deja 10 px más arriba que
// el navegador y queda pegada al título. Se baja para que caiga donde la
// dibuja el canvas.
constexpr int32_t kValueY = 44 + 10;
constexpr int32_t kUnitX = 318;  // "mV" y el estado, a 16 px de "650"

// Escala de ORP: de -300 a 900 mV en 576 px
constexpr int32_t kScaleW = 576;
constexpr int kScaleMin = -300;
constexpr int kScaleMax = 900;
int32_t scale_x(int mv) { return (std::clamp(mv, kScaleMin, kScaleMax) - kScaleMin) * kScaleW / (kScaleMax - kScaleMin); }

// Gráfica de 7 días dentro de su tarjeta: de 400 a 800 mV
constexpr int32_t kPlotX = 71;     // x de la primera muestra
constexpr int32_t kPlotY = 45;     // y de 800 mV
constexpr int32_t kPlotW = 512;    // de la primera a la última muestra
constexpr int32_t kPlotH = 82;     // de 800 a 400 mV
constexpr int32_t kPlotPad = 4;    // margen para que la línea no se corte en las orillas
constexpr int32_t kPlotMin = 400;
constexpr int32_t kPlotMax = 800;
constexpr int32_t kDaysY = 134;    // labels de los días
constexpr int32_t kDotSize = 17;   // punto del final: 11 px de color y 3 de aro

// La misma cuenta que hace lv_chart para la y de un valor
int32_t plot_y(int32_t mv) { return kPlotY + kPlotH - (mv - kPlotMin) * kPlotH / (kPlotMax - kPlotMin); }

// Umbral de oxidante: línea punteada de 8 px con huecos de 6
const lv_point_precise_t kThreshold[2] = {{0, 1}, {kPlotW, 1}};

// Zonas: 0 reductor, 1 neutro, 2 oxidante
struct Zone {
    int32_t x;
    int32_t w;
    int32_t name_x;  // centro del nombre
    const char* name;
    uint32_t dim;    // relleno
    uint32_t lit;    // borde, palabra e ícono cuando es la de ahorita
    const lv_image_dsc_t* icon;
};
const Zone kZones[3] = {
    {0, 120, 60, "Reductor", colors::alarm_bg, colors::alarm, &icon_alarma_32},
    {121, 335, 288, "Neutro", colors::warning_bg, colors::warning, &icon_advertencia_32},
    {457, 119, 516, "Oxidante", colors::ok_bg, colors::ok, &icon_ok_32},
};

int zone_of(int mv) {
    const core::Level level = core::orp_level(mv);
    return level == core::Level::alarm ? 0 : level == core::Level::warning ? 1 : 2;
}

// Label de 26 px en texto-3 alineado a la izquierda, al centro o a la derecha de x
lv_obj_t* axis_label(lv_obj_t* parent, const char* text, int32_t x, int32_t y, lv_text_align_t align) {
    constexpr int32_t kWidth = 100;
    lv_obj_t* obj = label(parent, &font_label_26, colors::text_disabled, text);
    lv_obj_set_width(obj, kWidth);
    lv_obj_set_style_text_align(obj, align, 0);
    const int32_t left = align == LV_TEXT_ALIGN_LEFT ? x : align == LV_TEXT_ALIGN_RIGHT ? x - kWidth : x - kWidth / 2;
    lv_obj_set_pos(obj, left, y);
    return obj;
}

}  // namespace

lv_obj_t* CisternScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    live_ = box(page, 0, 0, 1024, 456);
    create_reading(live_);
    create_scale(live_);
    create_chart(live_);
    create_chlorine(live_);
    create_calibration(live_);
    locked_ = locked_card(page, 16, 12, 992, 432, "Cisterna", "Próximamente · fase 2",
                          "ORP, cloro libre estimado y calibración con tira");
    return page;
}

void CisternScreen::create_reading(lv_obj_t* page) {
    lv_obj_t* title = label(page, &font_body_32, colors::text_secondary, "ORP de la cisterna");
    lv_obj_set_pos(title, 24, 12);

    orp_value_ = label(page, &font_digits_136, colors::text, "");
    lv_obj_set_pos(orp_value_, 16, kValueY);

    orp_unit_ = label(page, &font_title_36, colors::text_secondary, "mV");
    lv_obj_set_pos(orp_unit_, kUnitX, 50);

    // Estado con ícono y palabra, en el color de su zona
    state_ = row(page, 8, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(state_, kUnitX, 102);
    state_icon_ = icon(state_, &icon_ok_32, colors::ok);
    state_word_ = label(state_, &font_title_36, colors::ok, "");
}

void CisternScreen::create_scale(lv_obj_t* page) {
    lv_obj_t* scale = box(page, 24, 164, kScaleW, 112);

    for (size_t i = 0; i < zones_.size(); ++i) {
        const Zone& z = kZones[i];
        zones_[i] = box(scale, z.x, 40, z.w, 28);
        fill(zones_[i], z.dim);
        lv_obj_set_style_border_color(zones_[i], color(z.lit), 0);
        zone_names_[i] = label(scale, &font_label_26, colors::text_secondary, z.name);
        lv_obj_set_width(zone_names_[i], 140);
        lv_obj_set_style_text_align(zone_names_[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(zone_names_[i], z.name_x - 70, 79);
    }

    // Límites de las zonas arriba, con el menos del DS
    char buf[12];
    const int limits[4] = {kScaleMin, -50, 650, kScaleMax};
    const lv_text_align_t aligns[4] = {LV_TEXT_ALIGN_LEFT, LV_TEXT_ALIGN_CENTER, LV_TEXT_ALIGN_CENTER,
                                       LV_TEXT_ALIGN_RIGHT};
    for (int i = 0; i < 4; ++i) {
        fmt::signed_int(buf, sizeof buf, limits[i]);
        axis_label(scale, buf, scale_x(limits[i]), -1, aligns[i]);
    }

    marker_ = box(scale, 0, 30, 6, 48);
    lv_obj_set_style_radius(marker_, 2, 0);
    fill(marker_, colors::text);
}

void CisternScreen::create_chart(lv_obj_t* page) {
    lv_obj_t* c = card(page, 16, 280, 600, 164);

    lv_obj_t* title = label(c, &font_label_26, colors::text_secondary, "Últimos 7 días · mV");
    lv_obj_set_pos(title, 16, 2);

    // Rejilla en 800 y 600, línea de 400 más gruesa. Antes que la gráfica para
    // que quede abajo de la línea.
    for (const int32_t mv : {800, 600, 400}) {
        const int32_t y = plot_y(mv);
        char buf[8];
        snprintf(buf, sizeof buf, "%d", static_cast<int>(mv));
        axis_label(c, buf, kPlotX - 12, y - 16, LV_TEXT_ALIGN_RIGHT);
        if (mv == kPlotMin) {
            fill(box(c, kPlotX, y - 1, kPlotW + 1, 2), colors::border_control);
        } else {
            fill(box(c, kPlotX, y, kPlotW + 1, 1), colors::border);
        }
    }

    lv_obj_t* threshold = lv_line_create(c);
    lv_line_set_points(threshold, kThreshold, 2);
    lv_obj_set_pos(threshold, kPlotX, plot_y(650) - 1);
    lv_obj_set_style_line_width(threshold, 2, 0);
    lv_obj_set_style_line_color(threshold, color(colors::ok), 0);
    lv_obj_set_style_line_dash_width(threshold, 8, 0);
    lv_obj_set_style_line_dash_gap(threshold, 6, 0);

    chart_ = lv_chart_create(c);
    lv_obj_remove_style_all(chart_);
    lv_obj_set_clickable(chart_, false);
    lv_obj_set_scrollable(chart_, false);
    lv_obj_set_pos(chart_, kPlotX - kPlotPad, kPlotY - kPlotPad);
    lv_obj_set_size(chart_, kPlotW + 2 * kPlotPad, kPlotH + 2 * kPlotPad);
    lv_obj_set_style_pad_all(chart_, kPlotPad, 0);
    lv_chart_set_type(chart_, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(chart_, 0, 0);
    lv_chart_set_point_count(chart_, core::kOrpSamples7d);
    lv_chart_set_axis_range(chart_, LV_CHART_AXIS_PRIMARY_Y, kPlotMin, kPlotMax);
    lv_obj_set_style_line_width(chart_, 3, LV_PART_ITEMS);
    lv_obj_set_style_line_rounded(chart_, true, LV_PART_ITEMS);
    lv_obj_set_style_size(chart_, 0, 0, LV_PART_INDICATOR);  // sin puntito en cada muestra
    series_ = lv_chart_add_series(chart_, color(colors::water), LV_CHART_AXIS_PRIMARY_Y);

    // Un día abajo de cada tramo de 24 h, centrado en la muestra 6 de ese tramo
    for (size_t i = 0; i < day_labels_.size(); ++i) {
        const int32_t sample = 6 + 24 * static_cast<int32_t>(i);
        const int32_t x = kPlotX + kPlotW * sample / (core::kOrpSamples7d - 1);
        day_labels_[i] = axis_label(c, "", x, kDaysY, LV_TEXT_ALIGN_CENTER);
    }

    chart_dot_ = box(c, 0, 0, kDotSize, kDotSize);
    lv_obj_set_style_radius(chart_dot_, LV_RADIUS_CIRCLE, 0);
    fill(chart_dot_, colors::water);
    lv_obj_set_style_border_width(chart_dot_, 3, 0);
    lv_obj_set_style_border_color(chart_dot_, color(colors::panel), 0);
}

void CisternScreen::create_chlorine(lv_obj_t* page) {
    lv_obj_t* c = card(page, 628, 12, 380, 212);

    lv_obj_t* title = label(c, &font_body_32, colors::text_secondary, "Cloro libre estimado");
    lv_obj_set_pos(title, 16, 12);

    // "≈ 0.5 mg/L": cifra y unidades en la misma línea base
    chlorine_row_ = row(c, 10, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(chlorine_row_, 16, 64);
    lv_obj_t* about = label(chlorine_row_, &font_title_36, colors::text_secondary, "≈");
    lv_obj_set_style_pad_bottom(about, 2, 0);
    chlorine_value_ = label(chlorine_row_, &font_digits_72, colors::text, "");
    lv_obj_t* unit = label(chlorine_row_, &font_title_36, colors::text_secondary, "mg/L");
    lv_obj_set_style_pad_bottom(unit, 2, 0);

    chlorine_note_ = label(c, &font_label_26, colors::text_secondary, "");
    lv_obj_set_width(chlorine_note_, 348);  // parte en dos renglones
    lv_obj_set_pos(chlorine_note_, 16, 140);
}

void CisternScreen::create_calibration(lv_obj_t* page) {
    lv_obj_t* c = card(page, 628, 236, 380, 208);

    lv_obj_t* title = label(c, &font_body_32, colors::text_secondary, "Última calibración");
    lv_obj_set_pos(title, 16, 12);

    strip_date_ = label(c, &font_title_36, colors::text, "");
    lv_obj_set_pos(strip_date_, 16, 56);
    strip_detail_ = label(c, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(strip_detail_, 16, 102);

    // La acción de la vista: secundario de 64 px con la gota antes del texto
    lv_obj_t* button = button_secondary(c, 16, 132, 348, "Calibrar con tira",
                                        [](lv_event_t*) { open_strip_keypad(); }, nullptr);
    lv_obj_set_height(button, 64);
    lv_obj_t* drop = icon(button, &icon_gota_32, colors::text);
    lv_obj_move_to_index(drop, 0);
}

void CisternScreen::update(const core::SystemState& s) {
    lv_obj_set_hidden(live_, !s.has_orp);
    lv_obj_set_hidden(locked_, s.has_orp);
    if (!s.has_orp) return;

    update_reading(s);
    update_chart(s);
    update_day_marks(s);
    update_strip(s);
}

void CisternScreen::update_reading(const core::SystemState& s) {
    char buf[16];
    fmt::signed_int(buf, sizeof buf, s.orp_mv);
    set_text(orp_value_, buf);

    // "mV" y el estado van 16 px después de la cifra; con "650" eso es x 318.
    // Un negativo de 4 signos los empuja a la derecha en lugar de taparlos.
    lv_point_t size;
    lv_text_get_size(&size, buf, &font_digits_136, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    const int32_t x = std::max(kUnitX, 16 + size.x + 16);
    lv_obj_set_x(orp_unit_, x);
    lv_obj_set_x(state_, x);

    lv_obj_set_x(marker_, scale_x(s.orp_mv) - 3);

    const int zone = zone_of(s.orp_mv);
    if (zone == zone_) return;
    zone_ = zone;

    // Todas las zonas en su fondo; la de ahorita lleva borde y nombre en su color
    for (int i = 0; i < 3; ++i) {
        const bool on = i == zone;
        lv_obj_set_style_border_width(zones_[i], on ? 2 : 0, 0);
        lv_obj_set_style_text_color(zone_names_[i], color(on ? kZones[i].lit : colors::text_secondary), 0);
    }
    const Zone& z = kZones[zone];
    lv_image_set_src(state_icon_, z.icon);
    set_icon_color(state_icon_, z.lit);
    lv_label_set_text(state_word_, z.name);
    lv_obj_set_style_text_color(state_word_, color(z.lit), 0);
}

void CisternScreen::update_chart(const core::SystemState& s) {
    // La gráfica solo se redibuja si cambió alguna muestra
    std::array<int32_t, core::kOrpSamples7d> values{};
    for (size_t i = 0; i < values.size(); ++i) {
        values[i] = std::clamp(static_cast<int32_t>(s.orp_7d[i]), kPlotMin, kPlotMax);
    }
    if (chart_drawn_ && values == chart_values_) return;
    chart_drawn_ = true;
    chart_values_ = values;
    std::copy(values.begin(), values.end(), lv_chart_get_series_y_array(chart_, series_));
    lv_chart_refresh(chart_);
    lv_obj_set_pos(chart_dot_, kPlotX + kPlotW - kDotSize / 2, plot_y(values.back()) - kDotSize / 2);
}

void CisternScreen::update_day_marks(const core::SystemState& s) {
    const int now = core::minute_of_day(s);
    if (now == marks_minute_) return;
    marks_minute_ = now;

    // Cada muestra es de una hora; la última es la de ahorita. Se ve qué día
    // fue la muestra de cada label contando hacia atrás desde hoy.
    for (size_t i = 0; i < day_labels_.size(); ++i) {
        const int sample = 6 + 24 * static_cast<int>(i);
        const int minute = now - (core::kOrpSamples7d - 1 - sample) * 60;  // negativo = días antes
        const int days_back = minute >= 0 ? 0 : (-minute + 1439) / 1440;
        set_text(day_labels_[i], fmt::weekday_short(static_cast<unsigned>((s.weekday + 7 * 2 - days_back) % 7)));
    }
}

void CisternScreen::update_strip(const core::SystemState& s) {
    const core::StripCheck& strip = s.last_strip;
    const bool calibrated = strip.day != 0;

    // Sin tira no hay con qué estimar el cloro libre
    lv_obj_set_hidden(chlorine_row_, !calibrated);
    set_text(chlorine_note_, calibrated ? "Estimado con el ORP y la última tira" : "Falta calibrar con una tira");
    char buf[48];
    snprintf(buf, sizeof buf, "%.1f", static_cast<double>(s.chlorine_mgl));
    set_text(chlorine_value_, buf);

    if (!calibrated) {
        set_text(strip_date_, "Sin calibrar");
        set_text(strip_detail_, "");
        return;
    }
    snprintf(buf, sizeof buf, "%s %u %s", fmt::weekday_short(strip.weekday), static_cast<unsigned>(strip.day),
             fmt::month_short(strip.month));
    set_text(strip_date_, buf);

    char mv[12];
    fmt::signed_int(mv, sizeof mv, strip.orp_mv);
    snprintf(buf, sizeof buf, "Tira %u.%u mg/L · %s mV", static_cast<unsigned>(strip.tenths_mgl / 10),
             static_cast<unsigned>(strip.tenths_mgl % 10), mv);
    set_text(strip_detail_, buf);
}

}  // namespace ui
