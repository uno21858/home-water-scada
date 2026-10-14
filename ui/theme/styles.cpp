#include "theme/styles.h"

#include "theme/colors.h"
#include "theme/fonts.h"

namespace ui::styles {

lv_style_t screen;
lv_style_t card;
lv_style_t card_pressed;
lv_style_t card_locked;
lv_style_t button_primary;
lv_style_t button_primary_pressed;
lv_style_t button_secondary;
lv_style_t button_secondary_pressed;

namespace {
lv_color_t c(uint32_t hex) { return lv_color_hex(hex); }
}  // namespace

void init() {
    static bool ready = false;
    if (ready) return;
    ready = true;

    lv_style_init(&screen);
    lv_style_set_bg_color(&screen, c(colors::background));
    lv_style_set_bg_opa(&screen, LV_OPA_COVER);
    lv_style_set_text_color(&screen, c(colors::text));
    lv_style_set_text_font(&screen, &font_label_26);

    lv_style_init(&card);
    lv_style_set_bg_color(&card, c(colors::panel));
    lv_style_set_bg_opa(&card, LV_OPA_COVER);
    lv_style_set_border_color(&card, c(colors::border));
    lv_style_set_border_width(&card, 1);
    lv_style_set_radius(&card, 10);

    lv_style_init(&card_pressed);
    lv_style_set_bg_color(&card_pressed, c(colors::panel_raised));
    lv_style_set_bg_opa(&card_pressed, LV_OPA_COVER);

    lv_style_init(&card_locked);
    lv_style_set_bg_color(&card_locked, c(colors::background));
    lv_style_set_bg_opa(&card_locked, LV_OPA_COVER);
    lv_style_set_border_color(&card_locked, c(colors::border));
    lv_style_set_border_width(&card_locked, 2);
    lv_style_set_radius(&card_locked, 10);

    lv_style_init(&button_primary);
    lv_style_set_bg_color(&button_primary, c(colors::primary));
    lv_style_set_bg_opa(&button_primary, LV_OPA_COVER);
    lv_style_set_border_color(&button_primary, c(colors::primary));
    lv_style_set_border_width(&button_primary, 2);
    lv_style_set_radius(&button_primary, 10);
    lv_style_set_text_color(&button_primary, c(colors::on_primary));

    lv_style_init(&button_primary_pressed);
    lv_style_set_bg_color(&button_primary_pressed, c(colors::primary_pressed));
    lv_style_set_border_color(&button_primary_pressed, c(colors::primary_pressed));

    lv_style_init(&button_secondary);
    lv_style_set_bg_color(&button_secondary, c(colors::panel_raised));
    lv_style_set_bg_opa(&button_secondary, LV_OPA_COVER);
    lv_style_set_border_color(&button_secondary, c(colors::border_control));
    lv_style_set_border_width(&button_secondary, 2);
    lv_style_set_radius(&button_secondary, 10);
    lv_style_set_text_color(&button_secondary, c(colors::text));

    lv_style_init(&button_secondary_pressed);
    lv_style_set_bg_color(&button_secondary_pressed, c(colors::secondary_pressed));
    lv_style_set_border_color(&button_secondary_pressed, c(colors::text));
}

}  // namespace ui::styles
