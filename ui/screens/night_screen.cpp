#include "screens/night_screen.h"

#include "components/primitives.h"

namespace ui {

void NightScreen::create(lv_obj_t* layer, Callback on_wake) {
    on_wake_ = on_wake;
    // Negro de verdad, no el fondo del DS: con la luz apagada da igual, y si
    // la app no la apaga (sin backlight_set) la pantalla se ve apagada
    root_ = box(layer, 0, 0, 1024, 600);
    lv_obj_set_style_bg_color(root_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_clickable(root_, true);
    lv_obj_add_event_cb(root_, handle_touch, LV_EVENT_CLICKED, this);
    lv_obj_set_hidden(root_, true);
}

void NightScreen::open() { lv_obj_set_hidden(root_, false); }

void NightScreen::close() { lv_obj_set_hidden(root_, true); }

bool NightScreen::is_open() const { return !lv_obj_is_hidden(root_); }

void NightScreen::handle_touch(lv_event_t* e) {
    auto* self = static_cast<NightScreen*>(lv_event_get_user_data(e));
    self->close();
    if (self->on_wake_ != nullptr) self->on_wake_();
}

}  // namespace ui
