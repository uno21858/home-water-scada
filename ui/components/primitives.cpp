#include "components/primitives.h"

#include <cstring>

#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/icons.h"
#include "theme/styles.h"

namespace ui {

lv_obj_t* box(lv_obj_t* parent) {
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_clickable(obj, false);
    lv_obj_set_scrollable(obj, false);
    return obj;
}

lv_obj_t* box(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h) {
    lv_obj_t* obj = box(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    return obj;
}

static lv_obj_t* flex(lv_obj_t* parent, lv_flex_flow_t flow, int32_t gap, lv_flex_align_t cross) {
    lv_obj_t* obj = box(parent);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(obj, flow);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, cross, cross);
    lv_obj_set_style_pad_column(obj, gap, 0);
    lv_obj_set_style_pad_row(obj, gap, 0);
    return obj;
}

lv_obj_t* row(lv_obj_t* parent, int32_t gap, lv_flex_align_t cross) {
    return flex(parent, LV_FLEX_FLOW_ROW, gap, cross);
}

lv_obj_t* column(lv_obj_t* parent, int32_t gap, lv_flex_align_t cross) {
    return flex(parent, LV_FLEX_FLOW_COLUMN, gap, cross);
}

lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, uint32_t hex, const char* text) {
    lv_obj_t* obj = lv_label_create(parent);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, color(hex), 0);
    lv_label_set_text(obj, text);
    return obj;
}

lv_obj_t* icon(lv_obj_t* parent, const lv_image_dsc_t* src, uint32_t hex) {
    lv_obj_t* obj = lv_image_create(parent);
    lv_image_set_src(obj, src);
    set_icon_color(obj, hex);
    return obj;
}

void set_icon_color(lv_obj_t* icon, uint32_t hex) {
    lv_obj_set_style_image_recolor(icon, color(hex), 0);
    lv_obj_set_style_image_recolor_opa(icon, LV_OPA_COVER, 0);
}

lv_obj_t* card(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h) {
    lv_obj_t* obj = box(parent, x, y, w, h);
    lv_obj_add_style(obj, &styles::card, 0);
    return obj;
}

lv_obj_t* locked_card(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h,
                      const char* title, const char* line1, const char* line2) {
    lv_obj_t* obj = box(parent, x, y, w, h);
    lv_obj_add_style(obj, &styles::card_locked, 0);

    lv_obj_t* head = row(obj, 10, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(head, 14, 10);
    icon(head, &icon_candado_32, colors::text_secondary);
    label(head, &font_body_32, colors::text_secondary, title);

    lv_obj_t* first = label(obj, &font_label_26, colors::text_disabled, line1);
    lv_obj_set_pos(first, 14, 58);
    lv_obj_t* second = label(obj, &font_label_26, colors::text_disabled, line2);
    lv_obj_set_pos(second, 14, 94);
    return obj;
}

void fill(lv_obj_t* obj, uint32_t hex) {
    lv_obj_set_style_bg_color(obj, color(hex), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
}

void link_to(lv_obj_t* obj, Screen target, const lv_style_t* pressed) {
    lv_obj_set_clickable(obj, true);
    lv_obj_add_style(obj, pressed != nullptr ? pressed : &styles::card_pressed, LV_STATE_PRESSED);
    lv_obj_add_event_cb(
        obj,
        [](lv_event_t* e) {
            const auto target = static_cast<Screen>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
            show(target);
        },
        LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<uintptr_t>(target)));
}

lv_obj_t* chevron(lv_obj_t* parent) {
    lv_obj_t* obj = icon(parent, &icon_chevron_28, colors::text_secondary);
    lv_obj_align(obj, LV_ALIGN_TOP_RIGHT, -14, 8);
    return obj;
}

static lv_obj_t* button(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const lv_style_t* normal,
                        const lv_style_t* pressed, lv_event_cb_t on_click, void* user_data) {
    lv_obj_t* obj = box(parent, x, y, w, 80);
    lv_obj_set_clickable(obj, true);
    lv_obj_add_style(obj, normal, 0);
    lv_obj_add_style(obj, pressed, LV_STATE_PRESSED);
    lv_obj_add_style(obj, &styles::button_disabled, LV_STATE_DISABLED);
    // El estado del botón (presionado, deshabilitado) baja también al ícono
    lv_obj_set_state_trickle(obj, true);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(obj, 12, 0);
    if (on_click != nullptr) lv_obj_add_event_cb(obj, on_click, LV_EVENT_CLICKED, user_data);
    return obj;
}

// Texto de botón sin color propio: lo hereda del estilo del botón según su estado
static lv_obj_t* button_text(lv_obj_t* parent, const lv_font_t* font, const char* text) {
    lv_obj_t* obj = lv_label_create(parent);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_label_set_text(obj, text);
    return obj;
}

lv_obj_t* button_primary(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const char* text,
                         lv_event_cb_t on_click, void* user_data) {
    lv_obj_t* obj = button(parent, x, y, w, &styles::button_primary, &styles::button_primary_pressed,
                           on_click, user_data);
    lv_obj_t* check = icon(obj, &icon_check_32, colors::on_primary);
    lv_obj_set_style_image_recolor(check, color(colors::text_disabled), LV_STATE_DISABLED);
    button_text(obj, &font_title_36, text);
    return obj;
}

lv_obj_t* button_secondary(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const char* text,
                           lv_event_cb_t on_click, void* user_data) {
    lv_obj_t* obj = button(parent, x, y, w, &styles::button_secondary, &styles::button_secondary_pressed,
                           on_click, user_data);
    button_text(obj, &font_body_32, text);
    return obj;
}

void set_text(lv_obj_t* label, const char* text) {
    if (std::strcmp(lv_label_get_text(label), text) != 0) lv_label_set_text(label, text);
}

}  // namespace ui
