#include "components/keypad_dialog.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/icons.h"
#include "theme/styles.h"

namespace ui {
namespace {

constexpr char kClear = 'C';
constexpr char kBackspace = '<';

// Rejilla de 3 x 4 a la derecha del diálogo: teclas de 138 x 117 con 12 px entre ellas
constexpr char kKeys[12] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', kClear, '0', kBackspace};

// Chip que muestra fecha u hora. Todavía no abre un selector: no es tocable.
// Regresa el chip; su texto es el último hijo.
lv_obj_t* info_chip(lv_obj_t* parent, int32_t x, int32_t w, const lv_image_dsc_t* src) {
    lv_obj_t* chip = box(parent, x, 312, w, 64);
    lv_obj_add_style(chip, &styles::button_secondary, 0);
    lv_obj_set_flex_flow(chip, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(chip, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(chip, 12, 0);
    icon(chip, src, colors::text);
    label(chip, &font_body_32, colors::text, "");
    return chip;
}

}  // namespace

void KeypadDialog::create(lv_obj_t* layer) {
    veil_ = box(layer, 0, 0, 1024, 600);
    lv_obj_set_clickable(veil_, true);  // se come los toques: no pasan a la pantalla
    lv_obj_set_style_bg_color(veil_, color(colors::overlay), 0);
    lv_obj_set_style_bg_opa(veil_, colors::overlay_opa, 0);

    lv_obj_t* dialog = box(veil_, 32, 24, 960, 552);
    lv_obj_set_style_bg_color(dialog, color(colors::panel), 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dialog, color(colors::border_control), 0);
    lv_obj_set_style_border_width(dialog, 2, 0);
    lv_obj_set_style_radius(dialog, 16, 0);

    title_ = label(dialog, &font_title_36, colors::text, "");
    lv_obj_set_pos(title_, 30, 22);

    caption_ = label(dialog, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(caption_, 30, 84);

    // Campo: la cifra, un cursor fijo (el DS no quiere parpadeos) y la unidad
    lv_obj_t* field = box(dialog, 30, 118, 426, 96);
    lv_obj_set_style_bg_color(field, color(colors::background), 0);
    lv_obj_set_style_bg_opa(field, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(field, color(colors::text), 0);
    lv_obj_set_style_border_width(field, 2, 0);
    lv_obj_set_style_radius(field, 10, 0);
    value_ = label(field, &font_digits_72, colors::text, "");
    lv_obj_set_pos(value_, 22, 14);
    cursor_ = box(field, 28, 16, 4, 60);
    lv_obj_set_style_bg_color(cursor_, color(colors::text), 0);
    lv_obj_set_style_bg_opa(cursor_, LV_OPA_COVER, 0);
    unit_ = label(field, &font_title_36, colors::text_secondary, "");
    lv_obj_align(unit_, LV_ALIGN_TOP_RIGHT, -22, 24);

    hint_ = label(dialog, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(hint_, 30, 222);

    when_ = label(dialog, &font_label_26, colors::text_secondary, "Fecha y hora");
    lv_obj_set_pos(when_, 30, 276);
    date_chip_ = info_chip(dialog, 30, 240, &icon_calendario_32);
    date_ = lv_obj_get_child(date_chip_, -1);
    time_chip_ = info_chip(dialog, 282, 174, &icon_reloj_32);
    time_ = lv_obj_get_child(time_chip_, -1);

    button_secondary(dialog, 30, 444, 184, "Cancelar", handle_cancel, this);
    save_ = button_primary(dialog, 226, 444, 230, "Guardar", handle_save, this);

    for (int i = 0; i < 12; ++i) {
        const int32_t x = 490 + (i % 3) * 150;
        const int32_t y = 24 + (i / 3) * 129;
        lv_obj_t* key = box(dialog, x, y, 138, 117);
        lv_obj_set_clickable(key, true);
        lv_obj_add_style(key, &styles::key, 0);
        lv_obj_add_style(key, &styles::key_pressed, LV_STATE_PRESSED);
        lv_obj_set_user_data(key, reinterpret_cast<void*>(static_cast<uintptr_t>(kKeys[i])));
        lv_obj_add_event_cb(key, handle_key, LV_EVENT_CLICKED, this);

        lv_obj_t* face = nullptr;
        if (kKeys[i] == kBackspace) {
            face = icon(key, &icon_borrar_48, colors::text);
        } else if (kKeys[i] == kClear) {
            face = label(key, &font_body_32, colors::text, "Limpiar");
        } else {
            const char text[2] = {kKeys[i], '\0'};
            face = label(key, &font_digits_72, colors::text, text);
        }
        lv_obj_center(face);
    }

    lv_obj_set_hidden(veil_, true);
}

void KeypadDialog::open(const KeypadSpec& spec, Callback on_save) {
    lv_label_set_text(title_, spec.title);
    lv_label_set_text(caption_, spec.caption);
    lv_label_set_text(unit_, spec.unit);
    lv_label_set_text(hint_, spec.hint);

    const bool when = spec.date != nullptr && spec.time != nullptr;
    lv_obj_set_hidden(when_, !when);
    lv_obj_set_hidden(date_chip_, !when);
    lv_obj_set_hidden(time_chip_, !when);
    if (when) {
        lv_label_set_text(date_, spec.date);
        lv_label_set_text(time_, spec.time);
    }

    decimals_ = spec.decimals;
    max_digits_ = spec.max_digits < sizeof digits_ ? spec.max_digits : sizeof digits_ - 1;
    digits_[0] = '\0';
    on_save_ = on_save;
    refresh();
    lv_obj_set_hidden(veil_, false);
}

void KeypadDialog::close() {
    lv_obj_set_hidden(veil_, true);
    on_save_ = nullptr;
}

void KeypadDialog::press(char key) {
    const size_t len = std::strlen(digits_);
    if (key == kClear) {
        digits_[0] = '\0';
    } else if (key == kBackspace) {
        if (len > 0) digits_[len - 1] = '\0';
    } else if (std::strcmp(digits_, "0") == 0) {
        digits_[0] = key;  // un 0 solo se reemplaza: no queremos "045"
    } else if (len < max_digits_) {
        digits_[len] = key;
        digits_[len + 1] = '\0';
    }
    refresh();
}

void KeypadDialog::refresh() {
    // Con decimales se muestra ya con su punto: "5" con 1 decimal es "0.5"
    const unsigned value = static_cast<unsigned>(std::atoi(digits_));
    char text[24] = "";  // de sobra para cualquier unsigned: así el compilador no avisa
    if (digits_[0] == '\0' || decimals_ == 0) {
        snprintf(text, sizeof text, "%s", digits_);
    } else {
        const int places = decimals_ < 3 ? decimals_ : 3;
        unsigned scale = 1;
        for (int i = 0; i < places; ++i) scale *= 10;
        snprintf(text, sizeof text, "%u.%0*u", value / scale, places, value % scale);
    }
    lv_label_set_text(value_, text);

    // El cursor va 6 px después de la última cifra
    lv_point_t size;
    lv_text_get_size(&size, text, &font_digits_72, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_obj_set_x(cursor_, 22 + size.x + 6);
    lv_obj_set_disabled(save_, value == 0);
}

void KeypadDialog::handle_key(lv_event_t* e) {
    auto* self = static_cast<KeypadDialog*>(lv_event_get_user_data(e));
    lv_obj_t* key = lv_event_get_current_target_obj(e);
    self->press(static_cast<char>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(key))));
}

void KeypadDialog::handle_cancel(lv_event_t* e) {
    static_cast<KeypadDialog*>(lv_event_get_user_data(e))->close();
}

void KeypadDialog::handle_save(lv_event_t* e) {
    auto* self = static_cast<KeypadDialog*>(lv_event_get_user_data(e));
    const int value = std::atoi(self->digits_);
    if (value <= 0) return;
    const Callback callback = self->on_save_;
    self->close();
    if (callback != nullptr) callback(static_cast<uint32_t>(value));
}

}  // namespace ui
