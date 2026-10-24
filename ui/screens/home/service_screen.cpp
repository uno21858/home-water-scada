#include "screens/home/service_screen.h"

#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "ui.h"

namespace ui {
namespace {

const char* const kSteps[4] = {
    "Toca Empezar antes de cerrar V1",
    "Cierra V1 y cambia los cartuchos",
    "Regístralos con Lo cambié",
    "Abre V1 y toca Ya terminé",
};

}  // namespace

lv_obj_t* ServiceScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);

    // Los pasos, como en Prueba de cubeta pero sin marcar en cuál va
    lv_obj_t* list = card(page, 16, 12, 452, 432);
    lv_obj_set_style_pad_all(list, 12, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 8, 0);
    for (size_t i = 0; i < 4; ++i) {
        lv_obj_t* step = box(list);
        lv_obj_set_size(step, lv_pct(100), 94);
        lv_obj_t* number = box(step, 14, 14, 36, 36);
        lv_obj_set_style_radius(number, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(number, 2, 0);
        lv_obj_set_style_border_color(number, color(colors::text_secondary), 0);
        const char n[2] = {static_cast<char>('1' + i), '\0'};
        lv_obj_center(label(number, &font_label_26, colors::text, n));
        lv_obj_t* text = label(step, &font_body_32, colors::text, kSteps[i]);
        lv_obj_set_width(text, 354);
        lv_obj_set_pos(text, 64, 12);
    }

    // Cómo van los avisos y el botón
    lv_obj_t* side = column(page, 12, LV_FLEX_ALIGN_START);
    lv_obj_set_pos(side, 496, 12);
    lv_obj_set_width(side, 512);
    label(side, &font_body_32, colors::text_secondary, "Avisos de sin agua y caída");
    title_ = label(side, &font_title_xl_48, colors::text, "");
    note_ = label(side, &font_body_32, colors::text_secondary, "");
    lv_obj_set_width(note_, 512);

    lv_obj_t* button = button_primary(page, 488, 364, 320, "", handle_button, this);
    button_text_ = lv_obj_get_child(button, -1);
    show_state(false, 0);
    return page;
}

void ServiceScreen::update(const core::SystemState& s) { show_state(core::in_service(s), s.service_minutes_left); }

void ServiceScreen::show_state(bool active, uint8_t minutes_left) {
    active_ = active;
    set_text(title_, active ? "En pausa" : "Activos");
    char buf[96];
    if (active) {
        snprintf(buf, sizeof buf, "No avisan ni se guardan en el historial. Regresan en %u min.",
                 static_cast<unsigned>(minutes_left));
    } else {
        snprintf(buf, sizeof buf, "Con V1 cerrada la presión baja a 0 y parece que se fue el SIAPA.");
    }
    set_text(note_, buf);
    set_text(button_text_, active ? "Ya terminé" : "Empezar");
}

void ServiceScreen::handle_button(lv_event_t* e) {
    auto* self = static_cast<ServiceScreen*>(lv_event_get_user_data(e));
    const bool start = !self->active_;
    // Se ve al instante; el siguiente update() lo confirma con lo que guardó la app
    self->show_state(start, start ? core::kServiceMinutes : 0);
    if (actions().service_set != nullptr) actions().service_set(start);
}

}  // namespace ui
