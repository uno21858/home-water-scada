#include "components/header.h"

#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/icons.h"

namespace ui {

void Header::create(lv_obj_t* parent) {
    lv_obj_t* root = box(parent, 0, 0, 1024, 64);
    lv_obj_set_style_bg_color(root, color(colors::panel), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(root, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(root, 1, 0);
    lv_obj_set_style_border_color(root, color(colors::border), 0);

    title_ = label(root, &font_title_36, colors::text, "");
    lv_obj_align(title_, LV_ALIGN_LEFT_MID, 24, 0);

    lv_obj_t* right = row(root, 16, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(right, LV_ALIGN_RIGHT_MID, -20, 0);

    // Pastilla: ícono y palabra, nunca solo color
    pill_ = row(right, 8, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_height(pill_, 44);
    lv_obj_set_style_radius(pill_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(pill_, 2, 0);
    lv_obj_set_style_bg_opa(pill_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(pill_, 10, 0);
    lv_obj_set_style_pad_right(pill_, 18, 0);
    pill_icon_ = icon(pill_, &icon_ok_32, colors::ok);
    pill_text_ = label(pill_, &font_body_32, colors::ok, "");

    wifi_ = icon(right, &icon_wifi_32, colors::text_secondary);
    clock_ = label(right, &font_title_36, colors::text, "--:--");
}

void Header::set_title(const char* title) { set_text(title_, title); }

void Header::set_level(core::Level level) {
    if (static_cast<int>(level) == level_) return;
    level_ = static_cast<int>(level);

    uint32_t fg = colors::ok;
    uint32_t bg = colors::ok_bg;
    uint32_t border = colors::ok;
    const char* text = "OK";
    const lv_image_dsc_t* src = &icon_ok_32;
    if (level == core::Level::warning) {
        fg = border = colors::warning;
        bg = colors::warning_bg;
        text = "Advertencia";
        src = &icon_advertencia_32;
    } else if (level == core::Level::alarm) {
        fg = colors::on_alarm;
        bg = border = colors::alarm_screen;
        text = "Alarma";
        src = &icon_alarma_32;
    }
    lv_obj_set_style_bg_color(pill_, color(bg), 0);
    lv_obj_set_style_border_color(pill_, color(border), 0);
    lv_image_set_src(pill_icon_, src);
    set_icon_color(pill_icon_, fg);
    lv_label_set_text(pill_text_, text);
    lv_obj_set_style_text_color(pill_text_, color(fg), 0);
}

void Header::set_time(uint8_t hour, uint8_t minute) {
    char buf[8];
    snprintf(buf, sizeof buf, "%02u:%02u", static_cast<unsigned>(hour), static_cast<unsigned>(minute));
    set_text(clock_, buf);
}

void Header::set_wifi(bool ok) {
    if (static_cast<int>(ok) == wifi_ok_) return;
    wifi_ok_ = ok;
    lv_image_set_src(wifi_, ok ? &icon_wifi_32 : &icon_wifi_off_32);
    set_icon_color(wifi_, ok ? colors::text_secondary : colors::warning);
}

}  // namespace ui
