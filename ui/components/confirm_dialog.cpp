#include "components/confirm_dialog.h"

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"

namespace ui {

void ConfirmDialog::create(lv_obj_t* layer) {
    veil_ = box(layer, 0, 0, 1024, 600);
    lv_obj_set_clickable(veil_, true);  // se come los toques: no pasan a la pantalla
    lv_obj_set_style_bg_color(veil_, color(colors::overlay), 0);
    lv_obj_set_style_bg_opa(veil_, colors::overlay_opa, 0);

    lv_obj_t* dialog = box(veil_, 172, 120, 680, 360);
    lv_obj_set_style_bg_color(dialog, color(colors::panel), 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dialog, color(colors::border_control), 0);
    lv_obj_set_style_border_width(dialog, 2, 0);
    lv_obj_set_style_radius(dialog, 16, 0);

    // Pregunta en titulo-xl y consecuencia en texto, como pide el DS
    title_ = label(dialog, &font_title_xl_48, colors::text, "");
    lv_obj_set_pos(title_, 30, 30);
    body_ = label(dialog, &font_body_32, colors::text_secondary, "");
    lv_obj_set_pos(body_, 30, 104);
    lv_obj_set_width(body_, 616);
    lv_label_set_long_mode(body_, LV_LABEL_LONG_MODE_WRAP);

    button_secondary(dialog, 30, 248, 220, "Cancelar", handle_cancel, this);
    lv_obj_t* confirm = button_primary(dialog, 264, 248, 384, "", handle_confirm, this);
    confirm_text_ = lv_obj_get_child(confirm, -1);  // el label va después de la palomita

    lv_obj_set_hidden(veil_, true);
}

void ConfirmDialog::open(const char* title, const char* body, const char* confirm_text, Callback on_confirm) {
    lv_label_set_text(title_, title);
    lv_label_set_text(body_, body);
    lv_label_set_text(confirm_text_, confirm_text);
    on_confirm_ = on_confirm;
    lv_obj_set_hidden(veil_, false);
}

void ConfirmDialog::close() {
    lv_obj_set_hidden(veil_, true);
    on_confirm_ = nullptr;
}

void ConfirmDialog::handle_cancel(lv_event_t* e) {
    static_cast<ConfirmDialog*>(lv_event_get_user_data(e))->close();
}

void ConfirmDialog::handle_confirm(lv_event_t* e) {
    auto* self = static_cast<ConfirmDialog*>(lv_event_get_user_data(e));
    const Callback callback = self->on_confirm_;
    self->close();
    if (callback != nullptr) callback();
}

}  // namespace ui
