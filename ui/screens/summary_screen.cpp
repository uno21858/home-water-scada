#include "screens/summary_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "components/mixer.h"
#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "ui.h"

// Medidas tomadas del artboard Resumen del canvas.
// Las coordenadas dentro de una tarjeta cuentan desde adentro del borde,
// igual que en LVGL.

namespace ui {
namespace {

// Gráfica de 24 h dentro de la tarjeta de caudal
constexpr int32_t kPlotX = 15;          // x de la primera muestra
constexpr int32_t kPlotY = 114;         // y de 20 L/min
constexpr int32_t kPlotW = 568;         // de la primera a la última muestra
constexpr int32_t kPlotH = 72;          // de 20 L/min a 0
constexpr int32_t kPlotPad = 4;         // margen para que la línea no se corte en las orillas
constexpr int32_t kPlotMaxDeci = 200;   // 20.0 L/min en décimas
constexpr int32_t kMarksY = 191;        // labels de las horas
constexpr int32_t kDotSize = 15;        // punto del final: 9 px de color y 3 de aro

// La misma cuenta que hace lv_chart para la y de un valor
int32_t plot_y(int32_t deci) { return kPlotY + kPlotH - deci * kPlotH / kPlotMaxDeci; }

// Barra de rango de P1 y P2: de 0 a 80 psi, banda normal de 20 a 50
constexpr int32_t kRangeW = 212;
int32_t range_x(int psi) { return (std::clamp(psi, 0, 80) * kRangeW + 40) / 80; }

}  // namespace

lv_obj_t* SummaryScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    create_flow_card(page);
    create_dose_card(page);
    create_filters_card(page);
    create_mixer_card(page);
    create_cartridges_card(page);
    return page;
}

void SummaryScreen::create_flow_card(lv_obj_t* page) {
    lv_obj_t* c = card(page, 16, 12, 600, 236);
    link_to(c, Screen::flow);

    flow_title_ = label(c, &font_label_26, colors::text_secondary, "Caudal");
    lv_obj_set_pos(flow_title_, 16, 6);
    chevron(c);

    // Cifra y unidad en la misma línea base: la unidad (titulo) lleva 2 px abajo
    lv_obj_t* value_row = row(c, 10, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(value_row, 16, 36);
    flow_value_ = label(value_row, &font_digits_72, colors::water, "");
    lv_obj_t* unit = label(value_row, &font_title_36, colors::text_secondary, "L/min");
    lv_obj_set_style_pad_bottom(unit, 2, 0);

    // "Hoy 1,860 L" a la derecha, misma línea base que la cifra grande
    lv_obj_t* today = row(c, 8, LV_FLEX_ALIGN_END);
    lv_obj_align(today, LV_ALIGN_TOP_RIGHT, -16, 57);
    label(today, &font_label_26, colors::text_secondary, "Hoy");
    liters_today_ = label(today, &font_digits_44, colors::text, "");
    label(today, &font_label_26, colors::text_secondary, "L");

    // Rejilla antes que la gráfica para que quede abajo de la línea
    for (size_t i = 0; i < mark_lines_.size(); ++i) {
        mark_lines_[i] = box(c, kPlotX, kPlotY, 1, kPlotH);
        fill(mark_lines_[i], colors::border);
        mark_labels_[i] = label(c, &font_label_26, colors::text_disabled, "");
        lv_obj_set_width(mark_labels_[i], 100);
        lv_obj_set_style_text_align(mark_labels_[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_y(mark_labels_[i], kMarksY);
    }
    lv_obj_t* zero = box(c, kPlotX, kPlotY + kPlotH - 1, kPlotW + 1, 2);
    fill(zero, colors::border_control);

    chart_ = lv_chart_create(c);
    lv_obj_remove_style_all(chart_);
    lv_obj_set_clickable(chart_, false);
    lv_obj_set_scrollable(chart_, false);
    lv_obj_set_pos(chart_, kPlotX - kPlotPad, kPlotY - kPlotPad);
    lv_obj_set_size(chart_, kPlotW + 2 * kPlotPad, kPlotH + 2 * kPlotPad);
    lv_obj_set_style_pad_all(chart_, kPlotPad, 0);
    lv_chart_set_type(chart_, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(chart_, 0, 0);
    lv_chart_set_point_count(chart_, core::kFlowSamples24h);
    lv_chart_set_axis_range(chart_, LV_CHART_AXIS_PRIMARY_Y, 0, kPlotMaxDeci);
    lv_obj_set_style_line_width(chart_, 3, LV_PART_ITEMS);
    lv_obj_set_style_line_rounded(chart_, true, LV_PART_ITEMS);
    lv_obj_set_style_size(chart_, 0, 0, LV_PART_INDICATOR);  // sin puntito en cada muestra
    series_ = lv_chart_add_series(chart_, color(colors::water), LV_CHART_AXIS_PRIMARY_Y);

    now_label_ = label(c, &font_label_26, colors::text_disabled, "");
    lv_obj_set_width(now_label_, 100);
    lv_obj_set_style_text_align(now_label_, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(now_label_, kPlotX + kPlotW - 100, kMarksY);

    chart_dot_ = box(c, 0, 0, kDotSize, kDotSize);
    lv_obj_set_style_radius(chart_dot_, LV_RADIUS_CIRCLE, 0);
    fill(chart_dot_, colors::water);
    lv_obj_set_style_border_width(chart_dot_, 3, 0);
    lv_obj_set_style_border_color(chart_dot_, color(colors::panel), 0);
}

void SummaryScreen::create_dose_card(lv_obj_t* page) {
    lv_obj_t* c = card(page, 628, 12, 380, 236);

    // Zona de arriba: abre Dosis. El botón va aparte, a 12 px.
    lv_obj_t* area = box(c, 0, 0, 378, 130);
    lv_obj_set_style_radius(area, 9, 0);
    link_to(area, Screen::dose);

    lv_obj_t* title = label(area, &font_label_26, colors::text_secondary, "Toca echar");
    lv_obj_set_pos(title, 16, 6);
    chevron(area);

    lv_obj_t* value_row = row(area, 10, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(value_row, 16, 36);
    dose_value_ = label(value_row, &font_digits_72, colors::text, "");
    lv_obj_t* unit = label(value_row, &font_title_36, colors::text_secondary, "mL");
    lv_obj_set_style_pad_bottom(unit, 2, 0);

    lv_obj_t* where = label(area, &font_label_26, colors::text_secondary, "de Cloralex en la boca B");
    lv_obj_set_pos(where, 16, 98);

    dose_button_ = button_primary(c, 16, 142, 346, "Ya le puse", [](lv_event_t*) { ask_dose_confirmation(); },
                                  nullptr);
}

void SummaryScreen::create_filters_card(lv_obj_t* page) {
    filters_live_ = card(page, 16, 260, 490, 184);
    link_to(filters_live_, Screen::pressure_drop);

    lv_obj_t* title = label(filters_live_, &font_label_26, colors::text_secondary, "Filtros");
    lv_obj_set_pos(title, 16, 6);
    chevron(filters_live_);

    // P1 y P2: cifra con su barrita de rango (0 a 80 psi, banda normal 20 a 50)
    auto pressure = [this](int32_t x, const char* name, lv_obj_t** value, lv_obj_t** marker) {
        lv_obj_t* r = row(filters_live_, 8, LV_FLEX_ALIGN_END);
        lv_obj_set_pos(r, x, 42);
        label(r, &font_label_26, colors::text_secondary, name);
        *value = label(r, &font_digits_44, colors::text, "");
        label(r, &font_label_26, colors::text_secondary, "psi");

        lv_obj_t* track = box(filters_live_, x, 86, kRangeW, 4);
        lv_obj_set_style_radius(track, 2, 0);
        fill(track, colors::border);
        lv_obj_t* band = box(filters_live_, x + range_x(20), 86, range_x(50) - range_x(20), 4);
        fill(band, colors::pipe);
        *marker = box(filters_live_, x, 83, 3, 10);
        fill(*marker, colors::text);
    };
    pressure(16, "P1", &p1_value_, &p1_marker_);
    pressure(260, "P2", &p2_value_, &p2_marker_);

    // Caída del tren: el color y la palabra dicen si está bien
    drop_tile_ = box(filters_live_, 16, 110, 456, 56);
    lv_obj_set_style_radius(drop_tile_, 10, 0);
    lv_obj_set_style_border_width(drop_tile_, 2, 0);
    lv_obj_set_style_bg_opa(drop_tile_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(drop_tile_, 16, 0);
    lv_obj_set_flex_flow(drop_tile_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(drop_tile_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(drop_tile_, 8, 0);
    lv_obj_t* name = label(drop_tile_, &font_label_26, colors::text_secondary, "Caída");
    lv_obj_set_flex_grow(name, 1);
    drop_value_ = label(drop_tile_, &font_digits_44, colors::text, "");
    label(drop_tile_, &font_label_26, colors::text_secondary, "psi");
    lv_obj_set_size(box(drop_tile_), 8, 1);  // separa la palabra de "psi"
    lv_obj_t* state = row(drop_tile_, 6, LV_FLEX_ALIGN_CENTER);
    drop_icon_ = icon(state, &icon_ok_28, colors::ok);
    drop_word_ = label(state, &font_body_32, colors::ok, "");
}

void SummaryScreen::create_mixer_card(lv_obj_t* page) {
    mixer_live_ = card(page, 518, 260, 490, 184);
    link_to(mixer_live_, Screen::mixer);

    lv_obj_t* title = label(mixer_live_, &font_label_26, colors::text_secondary, "Bomba R · mezcla");
    lv_obj_set_pos(title, 16, 6);
    chevron(mixer_live_);

    // Bomba: aro fijo y aspas que giran cuando está prendida
    impeller_.create(mixer_live_, 16, 44);

    mixer_state_ = label(mixer_live_, &font_title_xl_48, colors::text, "");
    lv_obj_set_pos(mixer_state_, 88, 43);

    mixer_left_ = column(mixer_live_, 0, LV_FLEX_ALIGN_END);
    lv_obj_align(mixer_left_, LV_ALIGN_TOP_RIGHT, -16, 40);
    label(mixer_left_, &font_label_26, colors::text_secondary, "Faltan");
    lv_obj_t* minutes = row(mixer_left_, 6, LV_FLEX_ALIGN_END);
    mixer_minutes_ = label(minutes, &font_digits_44, colors::text, "");
    label(minutes, &font_label_26, colors::text_secondary, "min");

    mixer_bar_ = lv_bar_create(mixer_live_);
    lv_obj_remove_style_all(mixer_bar_);
    lv_obj_set_clickable(mixer_bar_, false);
    lv_obj_set_pos(mixer_bar_, 16, 116);
    lv_obj_set_size(mixer_bar_, 456, 8);
    lv_obj_set_style_radius(mixer_bar_, 4, 0);
    fill(mixer_bar_, colors::border);
    lv_obj_set_style_radius(mixer_bar_, 4, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(mixer_bar_, color(colors::water), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(mixer_bar_, LV_OPA_COVER, LV_PART_INDICATOR);

    mixer_sub_ = label(mixer_live_, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(mixer_sub_, 16, 134);
}

// Cartuchos: ocupa el lugar que dejan Filtros (fase 1.5) y Bomba R (fase 1)
// mientras no estén. Dice cuál es el que sigue: el que lleva más de su vida.
// No está en el canvas; copia el acomodo de la tarjeta de la bomba R.
void SummaryScreen::create_cartridges_card(lv_obj_t* page) {
    cartridges_ = card(page, 16, 260, 490, 184);
    link_to(cartridges_, Screen::filters);

    lv_obj_t* title = label(cartridges_, &font_label_26, colors::text_secondary, "Cartuchos · el que sigue");
    lv_obj_set_pos(title, 16, 6);
    chevron(cartridges_);

    next_name_ = label(cartridges_, &font_title_xl_48, colors::text, "");
    lv_obj_set_pos(next_name_, 16, 43);

    lv_obj_t* days = column(cartridges_, 0, LV_FLEX_ALIGN_END);
    lv_obj_align(days, LV_ALIGN_TOP_RIGHT, -16, 40);
    label(days, &font_label_26, colors::text_secondary, "Lleva");
    lv_obj_t* r = row(days, 6, LV_FLEX_ALIGN_END);
    next_days_ = label(r, &font_digits_44, colors::text, "");
    next_unit_ = label(r, &font_label_26, colors::text_secondary, "");

    next_track_ = box(cartridges_, 16, 116, 456, 8);
    lv_obj_set_style_radius(next_track_, 4, 0);
    lv_obj_set_style_clip_corner(next_track_, true, 0);
    fill(next_track_, colors::border);
    next_bar_ = box(next_track_, 0, 0, 0, 8);
    fill(next_bar_, colors::text_secondary);

    next_sub_ = label(cartridges_, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(next_sub_, 16, 134);
}

void SummaryScreen::update(const core::SystemState& s) {
    update_flow(s);
    update_time_marks(s);

    const uint32_t ml = core::dose_ml(s);
    char buf[16];
    snprintf(buf, sizeof buf, "%lu", static_cast<unsigned long>(ml));
    set_text(dose_value_, buf);
    lv_obj_set_disabled(dose_button_, ml == 0);

    update_filters(s);
    update_mixer(s);
    update_cartridges(s);
}

void SummaryScreen::update_flow(const core::SystemState& s) {
    const bool flowing = s.flow_lpm >= 0.05f;
    char buf[16];
    fmt::one_decimal(buf, sizeof buf, flowing ? s.flow_lpm : 0.0f);
    set_text(flow_value_, buf);
    if (static_cast<int>(flowing) != flowing_) {
        flowing_ = flowing;
        set_text(flow_title_, flowing ? "Caudal" : "Sin flujo");
        const uint32_t tone = flowing ? colors::water : colors::text_disabled;
        lv_obj_set_style_text_color(flow_value_, color(tone), 0);
        lv_obj_set_style_bg_color(chart_dot_, color(tone), 0);
    }

    fmt::thousands(buf, sizeof buf, s.liters_today);
    set_text(liters_today_, buf);

    // La gráfica solo se redibuja si cambió alguna muestra
    std::array<int32_t, core::kFlowSamples24h> values{};
    for (size_t i = 0; i < values.size(); ++i) {
        values[i] = std::clamp(static_cast<int32_t>(std::lround(s.flow_24h[i] * 10.0f)), int32_t{0}, kPlotMaxDeci);
    }
    if (chart_drawn_ && values == chart_values_) return;
    chart_drawn_ = true;
    chart_values_ = values;
    std::copy(values.begin(), values.end(), lv_chart_get_series_y_array(chart_, series_));
    lv_chart_refresh(chart_);
    lv_obj_set_pos(chart_dot_, kPlotX + kPlotW - kDotSize / 2, plot_y(values.back()) - kDotSize / 2);
}

void SummaryScreen::update_time_marks(const core::SystemState& s) {
    const int now = core::minute_of_day(s);
    if (now == marks_minute_) return;
    marks_minute_ = now;

    char buf[8];
    fmt::clock(buf, sizeof buf, static_cast<unsigned>(now));
    set_text(now_label_, buf);

    // Marcas a las 00, 06, 12 y 18 h dentro de la ventana de 24 h que termina ahorita.
    // Se esconden si chocan con la orilla izquierda o con la hora de ahorita.
    for (size_t i = 0; i < mark_lines_.size(); ++i) {
        const int mark = static_cast<int>(i) * 360;
        const int ago = (now - mark + 1440) % 1440;
        const int32_t x = kPlotX + kPlotW * (1440 - ago) / 1440;
        const bool fits = x >= kPlotX + 50 && x <= kPlotX + kPlotW - 100;
        lv_obj_set_hidden(mark_lines_[i], !fits);
        lv_obj_set_hidden(mark_labels_[i], !fits);
        if (!fits) continue;
        lv_obj_set_x(mark_lines_[i], x);
        lv_obj_set_x(mark_labels_[i], x - 50);
        fmt::clock(buf, sizeof buf, static_cast<unsigned>(mark));
        set_text(mark_labels_[i], buf);
    }
}

void SummaryScreen::update_cartridges(const core::SystemState& s) {
    // Va donde no hay Filtros ni Bomba R: a todo lo ancho, a la izquierda o a la derecha
    const int slot = s.has_pressure && s.has_mixer ? 0 : !s.has_pressure && !s.has_mixer ? 3 : s.has_pressure ? 2 : 1;
    if (slot != cartridges_slot_) {
        cartridges_slot_ = slot;
        lv_obj_set_hidden(cartridges_, slot == 0);
        const int32_t w = slot == 3 ? 992 : 490;
        lv_obj_set_pos(cartridges_, slot == 2 ? 518 : 16, 260);
        lv_obj_set_width(cartridges_, w);
        lv_obj_set_width(next_track_, w - 2 - 32);  // menos borde y márgenes
    }
    if (slot == 0) return;

    // El que lleva más de su vida; con empate, el primero (el prefiltro)
    size_t next = 0;
    uint32_t most = 0;
    for (size_t i = 0; i < core::kFilterCount; ++i) {
        const uint32_t pct = core::filter_wear_pct(s, static_cast<core::Filter>(i));
        if (s.filters[i].installed && pct > most) {
            most = pct;
            next = i;
        }
    }
    const bool prefilter = next == static_cast<size_t>(core::Filter::prefilter);
    char buf[48];
    if (prefilter) {
        snprintf(buf, sizeof buf, "Prefiltro");
    } else {
        snprintf(buf, sizeof buf, "Cartucho %s", fmt::filter_name(static_cast<unsigned>(next)));
    }
    set_text(next_name_, buf);

    const core::FilterStatus& f = s.filters[next];
    const uint32_t days = s.today >= f.since_day ? s.today - f.since_day : 0;
    snprintf(buf, sizeof buf, "%lu", static_cast<unsigned long>(days));
    set_text(next_days_, buf);
    set_text(next_unit_, days == 1 ? "día" : "días");

    const bool due = most >= 100;
    const int32_t track_w = (cartridges_slot_ == 3 ? 992 : 490) - 2 - 32;  // el ancho de la pista
    lv_obj_set_width(next_bar_, static_cast<int32_t>(track_w * static_cast<int32_t>(std::min<uint32_t>(most, 100)) / 100));
    fill(next_bar_, due ? colors::warning : colors::text_secondary);

    // Ya toca: la palabra en ámbar, no solo la barra
    if (due) {
        snprintf(buf, sizeof buf, prefilter ? "Ya toca purgarlo" : "Ya toca cambiarlo");
    } else if (prefilter) {
        snprintf(buf, sizeof buf, "Se purga %s", fmt::filter_every(static_cast<unsigned>(next)));
    } else {
        snprintf(buf, sizeof buf, "Dura %s", fmt::filter_every(static_cast<unsigned>(next)));
    }
    set_text(next_sub_, buf);
    lv_obj_set_style_text_color(next_sub_, color(due ? colors::warning : colors::text_secondary), 0);
}

void SummaryScreen::update_filters(const core::SystemState& s) {
    lv_obj_set_hidden(filters_live_, !s.has_pressure);
    if (!s.has_pressure) return;

    char buf[12];
    snprintf(buf, sizeof buf, "%d", s.p1_psi);
    set_text(p1_value_, buf);
    snprintf(buf, sizeof buf, "%d", s.p2_psi);
    set_text(p2_value_, buf);
    lv_obj_set_x(p1_marker_, 16 + range_x(s.p1_psi) - 1);
    lv_obj_set_x(p2_marker_, 260 + range_x(s.p2_psi) - 1);

    const int drop = s.p1_psi - s.p2_psi;
    snprintf(buf, sizeof buf, "%d", drop);
    set_text(drop_value_, buf);

    const core::Level level = core::pressure_drop_level(drop);
    if (static_cast<int>(level) == drop_level_) return;
    drop_level_ = static_cast<int>(level);

    uint32_t fg = colors::ok;
    uint32_t bg = colors::ok_bg;
    const char* word = "Normal";
    const lv_image_dsc_t* src = &icon_ok_28;
    if (level == core::Level::warning) {
        fg = colors::warning;
        bg = colors::warning_bg;
        word = "Revisar";
        src = &icon_advertencia_28;
    } else if (level == core::Level::alarm) {
        fg = colors::alarm;
        bg = colors::alarm_bg;
        word = "Tapados";
        src = &icon_alarma_28;
    }
    lv_obj_set_style_bg_color(drop_tile_, color(bg), 0);
    lv_obj_set_style_border_color(drop_tile_, color(fg), 0);
    lv_image_set_src(drop_icon_, src);
    set_icon_color(drop_icon_, fg);
    lv_label_set_text(drop_word_, word);
    lv_obj_set_style_text_color(drop_word_, color(fg), 0);
}

void SummaryScreen::update_mixer(const core::SystemState& s) {
    lv_obj_set_hidden(mixer_live_, !s.has_mixer);
    if (!s.has_mixer) {
        impeller_.set_spinning(false);
        return;
    }

    set_text(mixer_state_, s.mixer_on ? "Prendida" : "Apagada");
    lv_obj_set_hidden(mixer_left_, !s.mixer_on);
    lv_obj_set_hidden(mixer_bar_, !s.mixer_on);

    char buf[64];
    snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(s.mixer_minutes_left));
    set_text(mixer_minutes_, buf);
    if (lv_bar_get_value(mixer_bar_) != s.mixer_progress_pct) {
        lv_bar_set_value(mixer_bar_, s.mixer_progress_pct, LV_ANIM_OFF);
    }
    mixer_status(buf, sizeof buf, s);
    set_text(mixer_sub_, buf);
    impeller_.set_spinning(s.mixer_on);
}

}  // namespace ui
