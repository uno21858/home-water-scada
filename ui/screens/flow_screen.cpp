#include "screens/flow_screen.h"

#include <algorithm>
#include <cmath>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"

// Medidas tomadas del artboard Flujo del canvas (cada y menos 64).
// Las coordenadas dentro de una tarjeta cuentan desde adentro del borde,
// igual que en LVGL.

namespace ui {
namespace {

// Cifra de 232 px. Con la misma caja de 200 px, LVGL deja la línea base 17 px
// más arriba que el navegador (base_line de 35 contra el interlineado de CSS)
// y la cifra se encima con "Caudal". Se baja 17 px: así cae donde la dibuja el
// canvas (top 104 y línea base en 287 de la pantalla).
constexpr int32_t kValueY = 40 + 17;

// Gráfica de 2 h dentro de su tarjeta
constexpr int32_t kPlotX = 71;         // x de la primera muestra
constexpr int32_t kPlotY = 45;         // y de 20 L/min
constexpr int32_t kPlotW = 896;        // de la primera a la última muestra
constexpr int32_t kPlotH = 92;         // de 20 L/min a 0
constexpr int32_t kPlotPad = 4;        // margen para que la línea no se corte en las orillas
constexpr int32_t kPlotMaxDeci = 200;  // 20.0 L/min en décimas
constexpr int32_t kTimesY = 144;       // labels de las horas
constexpr int32_t kDotSize = 17;       // punto del final: 11 px de color y 3 de aro

// La misma cuenta que hace lv_chart para la y de un valor
int32_t plot_y(int32_t deci) { return kPlotY + kPlotH - deci * kPlotH / kPlotMaxDeci; }

// Tarjeta de litros: nombre arriba y cifra con su "L". Regresa el label de la cifra.
lv_obj_t* liters_card(lv_obj_t* page, int32_t y, const char* name) {
    lv_obj_t* c = card(page, 700, y, 308, 116);

    lv_obj_t* title = label(c, &font_label_26, colors::text_secondary, name);
    lv_obj_set_pos(title, 16, 10);

    // Cifra y unidad en la misma línea base: la unidad (titulo) lleva 2 px abajo
    lv_obj_t* value_row = row(c, 10, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(value_row, 16, 46);
    lv_obj_t* value = label(value_row, &font_digits_72, colors::text, "");
    lv_obj_t* unit = label(value_row, &font_title_36, colors::text_secondary, "L");
    lv_obj_set_style_pad_bottom(unit, 2, 0);
    return value;
}

}  // namespace

lv_obj_t* FlowScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    create_reading(page);
    create_totals(page);
    create_chart(page);
    return page;
}

void FlowScreen::create_reading(lv_obj_t* page) {
    // "Caudal" y a su lado el aviso de Sin flujo. Con el alto fijo de 40 el
    // título se queda en y 12 aunque el aviso se esconda.
    lv_obj_t* head = row(page, 16, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(head, 24, 11);
    lv_obj_set_height(head, 40);
    label(head, &font_body_32, colors::text_secondary, "Caudal");

    // Sin flujo: palabra e ícono, el gris de la cifra no basta. Si también se
    // fue la presión es "Sin agua del SIAPA", en ámbar.
    // En el canvas va en x 578, pero mide 174 px y queda abajo de la tarjeta
    // de Litros hoy (x 700). Aquí va junto al título.
    no_flow_chip_ = row(head, 8, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_height(no_flow_chip_, 40);
    lv_obj_set_style_radius(no_flow_chip_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(no_flow_chip_, 2, 0);
    lv_obj_set_style_border_color(no_flow_chip_, color(colors::border_control), 0);
    lv_obj_set_style_pad_hor(no_flow_chip_, 14, 0);
    no_flow_icon_ = icon(no_flow_chip_, &icon_info_28, colors::text_secondary);
    no_flow_text_ = label(no_flow_chip_, &font_label_26, colors::text_secondary, "Sin flujo");

    flow_value_ = label(page, &font_digits_232, colors::water, "");
    lv_obj_set_pos(flow_value_, 16, kValueY);

    lv_obj_t* unit = label(page, &font_title_36, colors::text_secondary, "L/min");
    lv_obj_set_pos(unit, 578, 164);
}

void FlowScreen::create_totals(lv_obj_t* page) {
    liters_today_ = liters_card(page, 12, "Litros hoy");
    liters_since_dose_ = liters_card(page, 140, "Desde la última dosis");
}

void FlowScreen::create_chart(lv_obj_t* page) {
    lv_obj_t* c = card(page, 16, 268, 992, 176);

    lv_obj_t* title = label(c, &font_label_26, colors::text_secondary, "Caudal de las últimas 2 horas · L/min");
    lv_obj_set_pos(title, 16, 4);

    // Eje de L/min: rejilla en 20 y 10, línea de cero más gruesa.
    // Va antes que la gráfica para que quede abajo de la línea.
    for (const int32_t lpm : {20, 10, 0}) {
        const int32_t y = plot_y(lpm * 10);
        lv_obj_t* value = label(c, &font_label_26, colors::text_disabled, "");
        lv_label_set_text_fmt(value, "%d", static_cast<int>(lpm));
        lv_obj_set_width(value, 40);
        lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_pos(value, kPlotX - 52, y - 16);  // centrado en su línea, 12 px antes de la gráfica

        if (lpm == 0) {
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
    lv_chart_set_point_count(chart_, core::kFlowSamples2h);
    lv_chart_set_axis_range(chart_, LV_CHART_AXIS_PRIMARY_Y, 0, kPlotMaxDeci);
    lv_obj_set_style_line_width(chart_, 3, LV_PART_ITEMS);
    lv_obj_set_style_line_rounded(chart_, true, LV_PART_ITEMS);
    lv_obj_set_style_size(chart_, 0, 0, LV_PART_INDICATOR);  // sin puntito en cada muestra
    series_ = lv_chart_add_series(chart_, color(colors::water), LV_CHART_AXIS_PRIMARY_Y);

    // Horas: de hace 2 h a ahorita cada 30 min. La primera va pegada a la
    // izquierda de la gráfica, la última a la derecha y las demás centradas.
    for (size_t i = 0; i < time_labels_.size(); ++i) {
        const int32_t x = kPlotX + kPlotW * static_cast<int32_t>(i) / 4;
        int32_t left = x - 50;
        lv_text_align_t align = LV_TEXT_ALIGN_CENTER;
        if (i == 0) {
            left = x;
            align = LV_TEXT_ALIGN_LEFT;
        } else if (i + 1 == time_labels_.size()) {
            left = x - 100;
            align = LV_TEXT_ALIGN_RIGHT;
        }
        time_labels_[i] = label(c, &font_label_26, colors::text_disabled, "");
        lv_obj_set_width(time_labels_[i], 100);
        lv_obj_set_style_text_align(time_labels_[i], align, 0);
        lv_obj_set_pos(time_labels_[i], left, kTimesY);
    }

    // En el canvas de Flujo el punto se queda en agua aunque no haya flujo
    chart_dot_ = box(c, 0, 0, kDotSize, kDotSize);
    lv_obj_set_style_radius(chart_dot_, LV_RADIUS_CIRCLE, 0);
    fill(chart_dot_, colors::water);
    lv_obj_set_style_border_width(chart_dot_, 3, 0);
    lv_obj_set_style_border_color(chart_dot_, color(colors::panel), 0);
}

void FlowScreen::update(const core::SystemState& s) {
    update_reading(s);

    char buf[16];
    fmt::thousands(buf, sizeof buf, s.liters_today);
    set_text(liters_today_, buf);
    fmt::thousands(buf, sizeof buf, s.liters_since_dose);
    set_text(liters_since_dose_, buf);

    update_chart(s);
    update_time_marks(s);
}

void FlowScreen::update_reading(const core::SystemState& s) {
    const bool flowing = s.flow_lpm >= 0.05f;
    char buf[16];
    fmt::one_decimal(buf, sizeof buf, flowing ? s.flow_lpm : 0.0f);
    set_text(flow_value_, buf);

    const int state = flowing ? 0 : core::supply_lost(s) ? 2 : 1;
    if (state == flow_state_) return;
    flow_state_ = state;
    lv_obj_set_style_text_color(flow_value_, color(flowing ? colors::water : colors::text_disabled), 0);
    lv_obj_set_hidden(no_flow_chip_, flowing);
    const bool no_supply = state == 2;
    const uint32_t tone = no_supply ? colors::warning : colors::text_secondary;
    lv_obj_set_style_border_color(no_flow_chip_, color(no_supply ? colors::warning : colors::border_control), 0);
    lv_image_set_src(no_flow_icon_, no_supply ? &icon_advertencia_28 : &icon_info_28);
    set_icon_color(no_flow_icon_, tone);
    set_text(no_flow_text_, no_supply ? "Sin agua del SIAPA" : "Sin flujo");
    set_text_color(no_flow_text_, tone);
}

void FlowScreen::update_chart(const core::SystemState& s) {
    // La gráfica solo se redibuja si cambió alguna muestra
    std::array<int32_t, core::kFlowSamples2h> values{};
    for (size_t i = 0; i < values.size(); ++i) {
        values[i] = std::clamp(static_cast<int32_t>(std::lround(s.flow_2h[i] * 10.0f)), int32_t{0}, kPlotMaxDeci);
    }
    if (chart_drawn_ && values == chart_values_) return;
    chart_drawn_ = true;
    chart_values_ = values;
    std::copy(values.begin(), values.end(), lv_chart_get_series_y_array(chart_, series_));
    lv_chart_refresh(chart_);
    lv_obj_set_pos(chart_dot_, kPlotX + kPlotW - kDotSize / 2, plot_y(values.back()) - kDotSize / 2);
}

void FlowScreen::update_time_marks(const core::SystemState& s) {
    const int now = core::minute_of_day(s);
    if (now == marks_minute_) return;
    marks_minute_ = now;

    char buf[8];
    const int last = static_cast<int>(time_labels_.size()) - 1;
    for (int i = 0; i <= last; ++i) {
        const int ago = (last - i) * 30;
        fmt::clock(buf, sizeof buf, static_cast<unsigned>((now - ago + 1440) % 1440));
        set_text(time_labels_[static_cast<size_t>(i)], buf);
    }
}

}  // namespace ui
