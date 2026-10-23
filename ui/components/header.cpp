#include "components/header.h"

#include <cstdio>

#include "components/primitives.h"
#include "components/status_pill.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "theme/styles.h"

// Medidas del componente Encabezado del canvas. El borde de abajo no cuenta
// para el contenido (LVGL respeta border_side): las filas miden 63 px.

namespace ui {

void Header::create(lv_obj_t* parent) {
    lv_obj_t* root = box(parent, 0, 0, 1024, 64);
    lv_obj_set_style_bg_color(root, color(colors::panel), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(root, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(root, 1, 0);
    lv_obj_set_style_border_color(root, color(colors::border), 0);

    // Fila de la izquierda: las piezas escondidas no ocupan lugar
    left_ = row(root, 12, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_height(left_, 63);

    back_ = box(left_, 0, 0, 56, 56);
    lv_obj_set_clickable(back_, true);
    lv_obj_add_style(back_, &styles::button_secondary, 0);
    lv_obj_add_style(back_, &styles::button_secondary_pressed, LV_STATE_PRESSED);
    lv_obj_t* arrow = icon(back_, &icon_atras_32, colors::text);
    lv_obj_center(arrow);
    lv_obj_add_event_cb(back_, handle_back, LV_EVENT_CLICKED, this);

    title_ = label(left_, &font_title_36, colors::text, "");

    link_ = row(left_, 4, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_height(link_, 56);
    lv_obj_set_clickable(link_, true);
    lv_obj_add_style(link_, &styles::button_secondary, 0);
    lv_obj_add_style(link_, &styles::button_secondary_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_pad_left(link_, 16, 0);
    lv_obj_set_style_pad_right(link_, 8, 0);
    link_text_ = label(link_, &font_label_26, colors::text, "");
    icon(link_, &icon_chevron_28, colors::text_secondary);
    lv_obj_add_event_cb(link_, handle_link, LV_EVENT_CLICKED, this);

    // "?" de ayuda: sin relleno ni borde, solo el ícono en texto-2
    help_ = box(left_, 0, 0, 56, 56);
    lv_obj_set_clickable(help_, true);
    lv_obj_set_style_radius(help_, 10, 0);
    lv_obj_add_style(help_, &styles::button_secondary_pressed, LV_STATE_PRESSED);
    lv_obj_center(icon(help_, &icon_info_32, colors::text_secondary));
    lv_obj_add_event_cb(help_, handle_help, LV_EVENT_CLICKED, this);

    lv_obj_t* right = row(root, 16, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(right, LV_ALIGN_RIGHT_MID, -20, 0);

    // Sin red: chip ámbar con lo que falta por enviar. Nunca un diálogo.
    offline_ = row(right, 0, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_height(offline_, 40);
    lv_obj_set_style_radius(offline_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(offline_, 2, 0);
    lv_obj_set_style_border_color(offline_, color(colors::warning), 0);
    fill(offline_, colors::warning_bg);
    lv_obj_set_style_pad_hor(offline_, 14, 0);
    offline_text_ = label(offline_, &font_label_26, colors::warning, "");
    lv_obj_set_hidden(offline_, true);

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

void Header::set_page(const HeaderSpec& spec) {
    // Con botón atrás la fila arranca en x 4; sin él, el título va en x 24
    lv_obj_set_pos(left_, spec.back ? 4 : 24, 0);
    lv_obj_set_hidden(back_, !spec.back);
    if (spec.back) back_target_ = *spec.back;

    set_text(title_, spec.title);

    const bool has_link = spec.link_text != nullptr && spec.link;
    has_link_ = has_link;
    lv_obj_set_hidden(link_, !has_link || !lv_obj_is_hidden(offline_));
    if (has_link) {
        set_text(link_text_, spec.link_text);
        link_target_ = *spec.link;
    }

    lv_obj_set_hidden(help_, !spec.help);
    if (spec.help) help_target_ = *spec.help;
}

void Header::handle_back(lv_event_t* e) {
    const auto* self = static_cast<Header*>(lv_event_get_user_data(e));
    show(self->back_target_);
}

void Header::handle_link(lv_event_t* e) {
    const auto* self = static_cast<Header*>(lv_event_get_user_data(e));
    show(self->link_target_);
}

void Header::handle_help(lv_event_t* e) {
    const auto* self = static_cast<Header*>(lv_event_get_user_data(e));
    show(self->help_target_);
}

void Header::set_status(const core::SystemState& s) {
    const PillLook look = pill_look(s);
    if (static_cast<int>(look.kind) == level_) return;
    level_ = static_cast<int>(look.kind);

    static const lv_image_dsc_t* const kIcons[4] = {&icon_ok_32, &icon_advertencia_32, &icon_alarma_32,
                                                     &icon_cartucho_32};
    lv_obj_set_style_bg_color(pill_, color(look.bg), 0);
    lv_obj_set_style_border_color(pill_, color(look.border), 0);
    lv_image_set_src(pill_icon_, kIcons[look.kind]);
    set_icon_color(pill_icon_, look.fg);
    lv_label_set_text(pill_text_, look.text);
    lv_obj_set_style_text_color(pill_text_, color(look.fg), 0);
}

void Header::set_time(uint8_t hour, uint8_t minute) {
    char buf[8];
    snprintf(buf, sizeof buf, "%02u:%02u", static_cast<unsigned>(hour), static_cast<unsigned>(minute));
    set_text(clock_, buf);
}

void Header::set_network(bool ok, uint32_t pending) {
    if (!ok && pending == 0) {
        set_text(offline_text_, "Sin red");  // sin servidor no hay nada por enviar
    } else if (!ok) {
        char count[16];
        fmt::thousands(count, sizeof count, pending);  // un día sin red son 8,640 lecturas
        char buf[40];
        snprintf(buf, sizeof buf, "Sin red · %s por enviar", count);
        set_text(offline_text_, buf);
    }
    if (static_cast<int>(ok) == wifi_ok_) return;
    wifi_ok_ = ok;
    lv_obj_set_hidden(offline_, ok);
    lv_obj_set_hidden(link_, !has_link_ || !ok);
    lv_image_set_src(wifi_, ok ? &icon_wifi_32 : &icon_wifi_off_32);
    set_icon_color(wifi_, ok ? colors::text_secondary : colors::warning);
}

}  // namespace ui
