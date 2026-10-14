#include "components/sensor_tag.h"

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/icons.h"

namespace ui {

void SensorTag::set_installed(bool installed) const {
    lv_obj_set_hidden(live, !installed);
    lv_obj_set_hidden(locked, installed);
}

SensorTag sensor_tag(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const char* name, const char* unit,
                     const char* locked_text) {
    SensorTag tag;

    tag.live = box(parent, x, y, w, 56);
    lv_obj_set_style_bg_color(tag.live, color(colors::panel_raised), 0);
    lv_obj_set_style_bg_opa(tag.live, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(tag.live, color(colors::border), 0);
    lv_obj_set_style_border_width(tag.live, 2, 0);
    lv_obj_set_style_radius(tag.live, 6, 0);

    lv_obj_t* r = row(tag.live, 8, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(r, 12, 3);
    label(r, &font_label_26, colors::text_secondary, name);
    tag.value = label(r, &font_digits_44, colors::text, "");
    label(r, &font_label_26, colors::text_secondary, unit);
    tag.bar_w = w - 2 * 2 - 2 * 12;  // menos bordes y márgenes

    tag.locked = box(parent, x, y, w, 56);
    lv_obj_set_style_bg_color(tag.locked, color(colors::background), 0);
    lv_obj_set_style_bg_opa(tag.locked, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(tag.locked, color(colors::border), 0);
    lv_obj_set_style_border_width(tag.locked, 2, 0);
    lv_obj_set_style_radius(tag.locked, 6, 0);
    lv_obj_t* l = row(tag.locked, 8, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 12, 0);
    label(l, &font_label_26, colors::text_disabled, name);
    icon(l, &icon_candado_24, colors::text_disabled);
    label(l, &font_label_26, colors::text_disabled, locked_text);
    lv_obj_set_hidden(tag.locked, true);
    return tag;
}

lv_obj_t* tag_bar(const SensorTag& tag, int32_t x, int32_t w, uint32_t hex) {
    lv_obj_t* bar = box(tag.live, tag.bar_x + x, tag.bar_y, w, 4);
    lv_obj_set_style_bg_color(bar, color(hex), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    return bar;
}

lv_obj_t* tag_marker(const SensorTag& tag) {
    lv_obj_t* m = box(tag.live, tag.bar_x, tag.bar_y - 3, 3, 10);
    lv_obj_set_style_bg_color(m, color(colors::text), 0);
    lv_obj_set_style_bg_opa(m, LV_OPA_COVER, 0);
    return m;
}

}  // namespace ui
