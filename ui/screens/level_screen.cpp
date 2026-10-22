#include "screens/level_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "components/primitives.h"
#include "level.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/icons.h"
#include "theme/styles.h"
#include "ui.h"

// No está en el canvas: copia el acomodo de Cisterna (título en y 12 y la
// cifra de 136 bajada 10 px) con el tanque en lugar de la escala.

namespace ui {
namespace {

constexpr int32_t kValueY = 44 + 10;

// Tanque dentro de su tarjeta
// Angosto para que "Llena 183 cm" quepa a su derecha dentro de la tarjeta
constexpr int32_t kTankX = 24;
constexpr int32_t kTankY = 24;
constexpr int32_t kTankW = 130;
constexpr int32_t kTankH = 380;
constexpr int32_t kInner = kTankH - 4;  // adentro del borde de 2

// La raya de la rejilla es punteada: lv_line solo la puntea si es horizontal
const lv_point_precise_t kIntakeLine[2] = {{0, 0}, {kTankW - 4, 0}};

// Renglón de la tarjeta de cm: nombre a la izquierda y cifra con "cm"
lv_obj_t* cm_row(lv_obj_t* parent, const char* name) {
    lv_obj_t* r = row(parent, 8, LV_FLEX_ALIGN_END);
    lv_obj_set_width(r, 552);
    lv_obj_set_flex_grow(label(r, &font_body_32, colors::text_secondary, name), 1);
    lv_obj_t* value = label(r, &font_body_32, colors::text, "");
    label(r, &font_label_26, colors::text_secondary, "cm");
    return value;
}

}  // namespace

lv_obj_t* LevelScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    create_tank(page);
    create_reading(page);
    return page;
}

void LevelScreen::create_tank(lv_obj_t* page) {
    lv_obj_t* c = card(page, 16, 12, 360, 432);
    lv_obj_t* tank = box(c, kTankX, kTankY, kTankW, kTankH);
    lv_obj_set_style_border_width(tank, 2, 0);
    lv_obj_set_style_border_color(tank, color(colors::border_control), 0);
    lv_obj_set_style_radius(tank, 8, 0);
    lv_obj_set_style_clip_corner(tank, true, 0);

    // El agua: la de abajo de la rejilla en gris y la útil en azul
    dead_ = box(tank, 0, 0, kTankW - 4, 0);
    fill(dead_, colors::pipe);
    lv_obj_set_style_bg_opa(dead_, LV_OPA_40, 0);
    usable_ = box(tank, 0, 0, kTankW - 4, 0);
    fill(usable_, colors::water);
    lv_obj_set_style_bg_opa(usable_, LV_OPA_30, 0);
    surface_ = box(tank, 0, 0, kTankW - 4, 3);
    fill(surface_, colors::water);

    // Llena: raya fina; rejilla: punteada. Su texto a la derecha del tanque.
    full_line_ = box(tank, 0, 0, kTankW - 4, 2);
    fill(full_line_, colors::text_secondary);
    intake_line_ = lv_line_create(tank);
    lv_line_set_points(intake_line_, kIntakeLine, 2);
    lv_obj_set_style_line_width(intake_line_, 2, 0);
    lv_obj_set_style_line_color(intake_line_, color(colors::text), 0);
    lv_obj_set_style_line_dash_width(intake_line_, 8, 0);
    lv_obj_set_style_line_dash_gap(intake_line_, 6, 0);
    full_text_ = label(c, &font_label_26, colors::text_secondary, "");
    intake_text_ = label(c, &font_label_26, colors::text_secondary, "");
}

void LevelScreen::create_reading(lv_obj_t* page) {
    lv_obj_set_pos(label(page, &font_body_32, colors::text_secondary, "Agua que la casa puede usar"), 408, 12);

    // Como "650 mV" en Cisterna: la unidad arriba, 13 px después de la cifra
    pct_ = label(page, &font_digits_136, colors::water, "");
    lv_obj_set_pos(pct_, 400, kValueY);
    unit_ = label(page, &font_title_36, colors::text_secondary, "%");
    lv_obj_set_y(unit_, 50);

    uncalibrated_ = label(page, &font_title_xl_48, colors::text_secondary, "Sin calibrar");
    lv_obj_set_pos(uncalibrated_, 408, 92);

    lv_obj_t* c = card(page, 392, 220, 616, 140);
    lv_obj_t* rows = column(c, 6, LV_FLEX_ALIGN_START);
    lv_obj_set_pos(rows, 16, 8);
    from_bottom_ = cm_row(rows, "Agua desde el fondo");
    above_intake_ = cm_row(rows, "Arriba de la toma");
    below_intake_ = cm_row(rows, "Se queda abajo de la toma");

    lv_obj_t* calibrate = button_secondary(page, 392, 368, 320, "Calibrar nivel", nullptr, nullptr);
    lv_obj_set_height(calibrate, 76);
    lv_obj_move_to_index(icon(calibrate, &icon_editar_32, colors::text), 0);
    link_to(calibrate, Screen::level_calibration, &styles::button_secondary_pressed);
}

void LevelScreen::update(const core::SystemState& s) {
    if (!s.has_level) return;
    const float level = core::level_cm(s);
    const int pct = core::level_pct(s);
    const float intake = static_cast<float>(s.level_intake_cm);

    // Lectura: % en grande y los cm. Sin marcar llena no hay %.
    char buf[16];
    lv_obj_set_hidden(pct_, pct < 0);
    lv_obj_set_hidden(unit_, pct < 0);
    lv_obj_set_hidden(uncalibrated_, pct >= 0);
    snprintf(buf, sizeof buf, "%d", std::max(pct, 0));
    set_text(pct_, buf);
    lv_point_t size;
    lv_text_get_size(&size, buf, &font_digits_136, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_obj_set_x(unit_, 400 + size.x + 13);
    snprintf(buf, sizeof buf, "%ld", std::lround(level));
    set_text(from_bottom_, buf);
    snprintf(buf, sizeof buf, "%ld", std::lround(std::max(0.0f, level - intake)));
    set_text(above_intake_, buf);
    snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(s.level_intake_cm));
    set_text(below_intake_, buf);

    // Tanque: arriba cabe llena más un 8 %; sin marcar, el rango del sensor
    const float top = s.level_full_cm > 0 ? static_cast<float>(s.level_full_cm) * 1.08f
                                          : static_cast<float>(s.level_sensor_cm + s.level_range_cm);
    auto y_of = [top](float cm) {
        return kInner - static_cast<int32_t>(std::lround(std::clamp(cm / top, 0.0f, 1.0f) * static_cast<float>(kInner)));
    };
    const int32_t y_level = y_of(level);
    const int32_t y_intake = y_of(intake);
    lv_obj_set_y(surface_, std::min(y_level, kInner - 3));
    // Útil: de la superficie a la rejilla; abajo de la rejilla, gris
    lv_obj_set_pos(usable_, 0, y_level);
    lv_obj_set_height(usable_, std::max<int32_t>(0, y_intake - y_level));  // en el ESP32 int32_t es long
    const int32_t dead_top = std::max(y_level, y_intake);
    lv_obj_set_pos(dead_, 0, dead_top);
    lv_obj_set_height(dead_, kInner - dead_top);

    lv_obj_set_y(intake_line_, y_intake - 1);
    snprintf(buf, sizeof buf, "Toma %u cm", static_cast<unsigned>(s.level_intake_cm));
    set_text(intake_text_, buf);
    lv_obj_set_pos(intake_text_, kTankX + kTankW + 12, kTankY + 2 + y_intake - 16);

    const bool marked = s.level_full_cm > 0;
    lv_obj_set_hidden(full_line_, !marked);
    lv_obj_set_hidden(full_text_, !marked);
    if (marked) {
        const int32_t y_full = y_of(static_cast<float>(s.level_full_cm));
        lv_obj_set_y(full_line_, y_full - 1);
        snprintf(buf, sizeof buf, "Llena %u cm", static_cast<unsigned>(s.level_full_cm));
        set_text(full_text_, buf);
        lv_obj_set_pos(full_text_, kTankX + kTankW + 12, kTankY + 2 + y_full - 16);
    }
}

}  // namespace ui
