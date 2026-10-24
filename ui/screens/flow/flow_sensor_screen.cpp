#include "screens/flow/flow_sensor_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "theme/styles.h"

// Medidas tomadas del artboard DetalleF del canvas (cada y menos 64).

namespace ui {
namespace {

constexpr int32_t kValueY = 44 + 10;  // cifra de 136 bajada como en Cisterna

// Rango del YF-B6: de 0 a 30 L/min en 576 px
constexpr int32_t kRangeW = 576;
constexpr float kRangeMax = 30.0f;
int32_t range_x(float lpm) {
    return static_cast<int32_t>(std::lround(std::clamp(lpm, 0.0f, kRangeMax) * kRangeW / kRangeMax));
}

// Onda: 568 px son 100 ms; arriba en y 6 y abajo en y 42
constexpr float kWaveW = 568.0f;
constexpr float kWaveHigh = 6.0f;
constexpr float kWaveLow = 42.0f;

// Renglón de la tabla de litros: nombre a la izquierda, cifra y "L" a la derecha
lv_obj_t* liters_row(lv_obj_t* parent, const char* name) {
    lv_obj_t* r = row(parent, 8, LV_FLEX_ALIGN_END);
    lv_obj_set_width(r, 348);
    lv_obj_set_flex_grow(label(r, &font_label_26, colors::text_secondary, name), 1);
    lv_obj_t* value = label(r, &font_body_32, colors::text, "");
    label(r, &font_label_26, colors::text_secondary, "L");
    return value;
}

}  // namespace

lv_obj_t* FlowSensorScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    create_reading(page);
    create_range(page);
    create_pulses(page);
    create_side(page);
    return page;
}

void FlowSensorScreen::create_reading(lv_obj_t* page) {
    lv_obj_set_pos(label(page, &font_body_32, colors::text_secondary, "Caudal ahora"), 24, 12);
    flow_value_ = label(page, &font_digits_136, colors::water, "");
    lv_obj_set_pos(flow_value_, 16, kValueY);
    lv_obj_set_pos(label(page, &font_title_36, colors::text_secondary, "L/min"), 352, 50);

    // Pastilla de la señal: ícono y palabra
    signal_ = row(page, 8, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(signal_, 352, 104);
    lv_obj_set_height(signal_, 44);
    lv_obj_set_style_radius(signal_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(signal_, 2, 0);
    lv_obj_set_style_bg_opa(signal_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(signal_, 10, 0);
    lv_obj_set_style_pad_right(signal_, 18, 0);
    signal_icon_ = icon(signal_, &icon_ok_32, colors::ok);
    signal_text_ = label(signal_, &font_label_26, colors::ok, "");
}

void FlowSensorScreen::create_range(lv_obj_t* page) {
    lv_obj_set_pos(label(page, &font_label_26, colors::text_secondary, "Rango del YF-B6: 1 a 30 L/min"), 24, 172);

    lv_obj_t* scale = box(page, 24, 208, kRangeW, 56);
    lv_obj_t* track = box(scale, 0, 4, kRangeW, 20);
    lv_obj_set_style_radius(track, 4, 0);
    lv_obj_set_style_clip_corner(track, true, 0);
    fill(track, colors::panel_raised);
    fill(box(track, 0, 0, range_x(1.0f), 20), colors::border);  // abajo de 1 L/min el sensor no cuenta bien
    range_fill_ = box(track, 0, 0, 0, 20);
    fill(range_fill_, colors::water);
    range_marker_ = box(scale, 0, -4, 4, 36);
    lv_obj_set_style_radius(range_marker_, 2, 0);
    fill(range_marker_, colors::text);

    const char* const marks[4] = {"0", "10", "20", "30"};
    const lv_text_align_t aligns[4] = {LV_TEXT_ALIGN_LEFT, LV_TEXT_ALIGN_CENTER, LV_TEXT_ALIGN_CENTER,
                                       LV_TEXT_ALIGN_RIGHT};
    for (int i = 0; i < 4; ++i) {
        lv_obj_t* l = label(scale, &font_label_26, colors::text_disabled, marks[i]);
        lv_obj_set_width(l, 60);
        lv_obj_set_style_text_align(l, aligns[i], 0);
        const int32_t x = kRangeW * i / 3;
        lv_obj_set_pos(l, aligns[i] == LV_TEXT_ALIGN_LEFT ? x : aligns[i] == LV_TEXT_ALIGN_RIGHT ? x - 60 : x - 30, 29);
    }
}

void FlowSensorScreen::create_pulses(lv_obj_t* page) {
    lv_obj_t* c = card(page, 16, 280, 600, 164);
    lv_obj_set_pos(label(c, &font_label_26, colors::text_secondary, "Pulsos en DI0 · últimos 100 ms"), 16, 6);

    lv_obj_t* area = box(c, 16, 44, 568, 48);
    fill(box(area, 0, 42, 568, 1), colors::border);
    wave_ = lv_line_create(area);
    lv_obj_set_pos(wave_, 0, 0);
    lv_obj_set_style_line_width(wave_, 3, 0);
    lv_obj_set_style_line_color(wave_, color(colors::text), 0);

    lv_obj_t* hz = row(c, 8, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(hz, 16, 112);
    hz_ = label(hz, &font_digits_44, colors::text, "");
    label(hz, &font_label_26, colors::text_secondary, "Hz");

    lv_obj_t* pm = row(c, 8, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(pm, 216, 112);
    per_minute_ = label(pm, &font_digits_44, colors::text, "");
    label(pm, &font_label_26, colors::text_secondary, "pulsos por minuto");
}

void FlowSensorScreen::create_side(lv_obj_t* page) {
    lv_obj_t* k = card(page, 628, 12, 380, 168);
    lv_obj_set_pos(label(k, &font_label_26, colors::text_secondary, "Factor K"), 16, 10);
    lv_obj_t* r = row(k, 10, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(r, 16, 46);
    factor_ = label(r, &font_digits_72, colors::text, "");
    lv_obj_t* unit = label(r, &font_title_36, colors::text_secondary, "pulsos/L");
    lv_obj_set_style_pad_bottom(unit, 2, 0);
    factor_note_ = label(k, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(factor_note_, 16, 120);

    lv_obj_t* t = card(page, 628, 192, 380, 164);
    lv_obj_t* rows = column(t, 8, LV_FLEX_ALIGN_START);
    lv_obj_set_pos(rows, 16, 12);
    today_ = liters_row(rows, "Hoy");
    since_dose_ = liters_row(rows, "Desde la dosis");
    week_ = liters_row(rows, "Esta semana");

    lv_obj_t* bucket = button_secondary(page, 628, 368, 380, "Prueba de cubeta", nullptr, nullptr);
    lv_obj_set_height(bucket, 76);
    lv_obj_t* i = icon(bucket, &icon_cubeta_32, colors::text);
    lv_obj_set_style_image_recolor(i, color(colors::text_disabled), LV_STATE_DISABLED);
    lv_obj_move_to_index(i, 0);
    link_to(bucket, Screen::bucket_test, &styles::button_secondary_pressed);
}

void FlowSensorScreen::update(const core::SystemState& s) {
    char buf[24];
    const bool flowing = s.flow_lpm >= 0.05f;
    const float lpm = flowing ? s.flow_lpm : 0.0f;
    fmt::one_decimal(buf, sizeof buf, lpm);
    set_text(flow_value_, buf);
    lv_obj_set_style_text_color(flow_value_, color(flowing ? colors::water : colors::text_disabled), 0);

    if (static_cast<int>(s.flow_sensor_ok) != signal_ok_) {
        signal_ok_ = s.flow_sensor_ok;
        const uint32_t fg = s.flow_sensor_ok ? colors::ok : colors::alarm;
        lv_obj_set_style_bg_color(signal_, color(s.flow_sensor_ok ? colors::ok_bg : colors::alarm_bg), 0);
        lv_obj_set_style_border_color(signal_, color(fg), 0);
        lv_image_set_src(signal_icon_, s.flow_sensor_ok ? &icon_ok_32 : &icon_alarma_32);
        set_icon_color(signal_icon_, fg);
        lv_label_set_text(signal_text_, s.flow_sensor_ok ? "Señal OK" : "Sin señal");
        lv_obj_set_style_text_color(signal_text_, color(fg), 0);
    }

    lv_obj_set_width(range_fill_, range_x(lpm));
    lv_obj_set_x(range_marker_, range_x(lpm) - 2);

    // F = Q x K / 60: con 12.4 L/min y 396 pulsos/L salen 81.8 Hz
    const float hz = lpm * static_cast<float>(s.flow_pulses_per_liter) / 60.0f;
    snprintf(buf, sizeof buf, "%.1f", static_cast<double>(hz));
    set_text(hz_, buf);
    fmt::thousands(buf, sizeof buf, static_cast<uint32_t>(std::lround(lpm * s.flow_pulses_per_liter)));
    set_text(per_minute_, buf);
    draw_wave(hz);

    snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(s.flow_pulses_per_liter));
    set_text(factor_, buf);
    if (s.flow_pulses_per_liter == core::kFactoryPulsesPerLiter) {
        set_text(factor_note_, "De fábrica · F = 6.6 × Q");
    } else {
        snprintf(buf, sizeof buf, "De fábrica: %u", static_cast<unsigned>(core::kFactoryPulsesPerLiter));
        set_text(factor_note_, buf);
    }

    fmt::thousands(buf, sizeof buf, s.liters_today);
    set_text(today_, buf);
    fmt::thousands(buf, sizeof buf, s.liters_since_dose);
    set_text(since_dose_, buf);
    uint32_t week = 0;
    for (size_t i = core::kHistoryDays - 7; i < core::kHistoryDays; ++i) week += s.days[i].liters;
    fmt::thousands(buf, sizeof buf, week);
    set_text(week_, buf);
}

void FlowSensorScreen::draw_wave(float hz) {
    // Solo se rehace si cambió la décima de Hz
    const int32_t tenths = static_cast<int32_t>(std::lround(hz * 10.0f));
    if (tenths == wave_tenths_hz_) return;
    wave_tenths_hz_ = tenths;

    // Medio periodo en px: 568 px son 100 ms, así que caben hz / 10 periodos
    int n = 0;
    auto add = [this, &n](float x, float y) {
        wave_points_[static_cast<size_t>(n++)] = {static_cast<lv_value_precise_t>(x), static_cast<lv_value_precise_t>(y)};
    };
    if (tenths <= 0) {
        add(0, kWaveLow);  // sin pulsos: la línea se queda abajo
        add(kWaveW, kWaveLow);
    } else {
        const float half = kWaveW / (hz / 10.0f) / 2.0f;
        float x = 0;
        bool high = true;
        add(0, kWaveHigh);
        while (x < kWaveW && n + 2 < static_cast<int>(wave_points_.size())) {
            const float next = std::min(kWaveW, x + half);
            add(next, high ? kWaveHigh : kWaveLow);
            if (next < kWaveW) {
                high = !high;
                add(next, high ? kWaveHigh : kWaveLow);
            }
            x = next;
        }
    }
    lv_line_set_points_mutable(wave_, wave_points_.data(), static_cast<uint32_t>(n));
}

}  // namespace ui
