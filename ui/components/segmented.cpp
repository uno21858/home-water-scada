#include "components/segmented.h"

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"

namespace ui {

void Segmented::create(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const char* const* options, int count,
                       Callback on_pick) {
    count_ = count < 4 ? count : 4;
    on_pick_ = on_pick;

    lv_obj_t* frame = box(parent, x, y, w, 64);
    fill(frame, colors::panel);
    lv_obj_set_style_border_width(frame, 2, 0);
    lv_obj_set_style_border_color(frame, color(colors::border_control), 0);
    lv_obj_set_style_radius(frame, 10, 0);
    lv_obj_set_style_clip_corner(frame, true, 0);  // la elegida no se sale de las esquinas redondas

    // Adentro del borde quedan w - 4 px; el sobrante va en las primeras
    const int32_t inner = w - 4;
    int32_t left = 0;
    for (int i = 0; i < count_; ++i) {
        const int32_t width = inner / count_ + (i < inner % count_ ? 1 : 0);
        lv_obj_t* b = box(frame, left, 0, width, 60);
        lv_obj_set_clickable(b, true);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(b, color(colors::panel), 0);
        lv_obj_set_style_bg_color(b, color(colors::secondary_pressed), LV_STATE_PRESSED);
        if (i > 0) {  // raya de 2 px entre opciones
            lv_obj_set_style_border_side(b, LV_BORDER_SIDE_LEFT, 0);
            lv_obj_set_style_border_width(b, 2, 0);
            lv_obj_set_style_border_color(b, color(colors::border_control), 0);
        }
        texts_[i] = label(b, &font_body_32, colors::text_secondary, options[i]);
        lv_obj_center(texts_[i]);
        lv_obj_set_user_data(b, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(b, handle_click, LV_EVENT_CLICKED, this);
        buttons_[i] = b;
        left += width;
    }
}

void Segmented::set_selected(int index) {
    if (index == selected_) return;
    selected_ = index;
    for (int i = 0; i < count_; ++i) {
        const bool on = i == index;
        lv_obj_set_style_bg_color(buttons_[i], color(on ? colors::primary : colors::panel), 0);
        lv_obj_set_style_text_color(texts_[i], color(on ? colors::on_primary : colors::text_secondary), 0);
    }
}

void Segmented::handle_click(lv_event_t* e) {
    auto* self = static_cast<Segmented*>(lv_event_get_user_data(e));
    lv_obj_t* button = lv_event_get_current_target_obj(e);
    const int index = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(button)));
    self->set_selected(index);  // se ve al instante; el estado lo confirma en el siguiente update
    if (self->on_pick_ != nullptr) self->on_pick_(index);
}

}  // namespace ui
