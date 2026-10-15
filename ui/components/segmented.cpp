#include "components/segmented.h"

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"

namespace ui {

lv_obj_t* Segmented::create(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h,
                            const char* const* options, int count, Callback on_pick, void* user_data) {
    count_ = count < 4 ? count : 4;
    on_pick_ = on_pick;
    user_data_ = user_data;

    lv_obj_t* frame = box(parent, x, y, w > 0 ? w : LV_SIZE_CONTENT, h);
    lv_obj_set_flex_flow(frame, LV_FLEX_FLOW_ROW);
    fill(frame, colors::panel);
    lv_obj_set_style_border_width(frame, 2, 0);
    lv_obj_set_style_border_color(frame, color(colors::border_control), 0);
    lv_obj_set_style_radius(frame, 10, 0);
    lv_obj_set_style_clip_corner(frame, true, 0);  // la elegida no se sale de las esquinas redondas

    // Adentro del borde quedan w - 4 px; el sobrante va en las primeras
    const int32_t inner = w - 4;
    for (int i = 0; i < count_; ++i) {
        lv_obj_t* b = box(frame);
        if (w > 0) {
            lv_obj_set_size(b, inner / count_ + (i < inner % count_ ? 1 : 0), h - 4);
        } else {
            lv_obj_set_size(b, LV_SIZE_CONTENT, h - 4);
            lv_obj_set_style_pad_hor(b, 18, 0);
        }
        lv_obj_set_clickable(b, true);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(b, color(colors::panel), 0);
        lv_obj_set_style_bg_color(b, color(colors::secondary_pressed), LV_STATE_PRESSED);
        if (i > 0) {  // raya de 2 px entre opciones
            lv_obj_set_style_border_side(b, LV_BORDER_SIDE_LEFT, 0);
            lv_obj_set_style_border_width(b, 2, 0);
            lv_obj_set_style_border_color(b, color(colors::border_control), 0);
        }
        lv_obj_set_flex_flow(b, LV_FLEX_FLOW_ROW);  // el texto al centro, mida lo que mida el botón
        lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        texts_[i] = label(b, &font_body_32, colors::text_secondary, options[i]);
        lv_obj_set_user_data(b, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(b, handle_click, LV_EVENT_CLICKED, this);
        buttons_[i] = b;
    }
    return frame;
}

void Segmented::set_option_hidden(int index, bool hidden) {
    if (index >= 0 && index < count_) lv_obj_set_hidden(buttons_[index], hidden);
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
    if (self->on_pick_ != nullptr) self->on_pick_(index, self->user_data_);
}

}  // namespace ui
