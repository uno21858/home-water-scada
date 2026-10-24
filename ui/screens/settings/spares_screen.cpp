#include "screens/settings/spares_screen.h"

#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "ui.h"

// No está en el canvas: renglones como los de Cartuchos (992 x 60 cada 72 px)
// con un − y un + del alto de su botón.

namespace ui {
namespace {

constexpr uint8_t kMaxSpares = 99;  // dos cifras: más no cabe ni hace falta

// El renglón 0 es el 20 µm: core::Filter empieza con el prefiltro
core::Filter filter_of(size_t row) { return static_cast<core::Filter>(row + 1); }

}  // namespace

lv_obj_t* SparesScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    for (size_t i = 0; i < rows_.size(); ++i) {
        Row& r = rows_[i];
        lv_obj_t* c = card(page, 16, 12 + 72 * static_cast<int32_t>(i), 992, 60);
        lv_obj_set_pos(icon(c, &icon_cartucho_32, colors::text), 16, 13);
        lv_obj_set_pos(label(c, &font_body_32, colors::text, fmt::filter_name(static_cast<unsigned>(filter_of(i)))),
                       60, 10);

        // − cifra +: cada botón guarda en su user_data el renglón y si suma
        r.minus = button_secondary(c, 690, 1, 88, "\xE2\x88\x92", handle_step, this);  // U+2212
        lv_obj_set_height(r.minus, 56);
        lv_obj_set_user_data(r.minus, reinterpret_cast<void*>(static_cast<uintptr_t>(i * 2)));
        r.count = label(c, &font_digits_44, colors::text, "");
        lv_obj_set_width(r.count, 112);
        lv_obj_set_style_text_align(r.count, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(r.count, 778, 10);
        r.plus = button_secondary(c, 890, 1, 88, "+", handle_step, this);
        lv_obj_set_height(r.plus, 56);
        lv_obj_set_user_data(r.plus, reinterpret_cast<void*>(static_cast<uintptr_t>(i * 2 + 1)));
    }

    lv_obj_t* note = label(page, &font_label_26, colors::text_secondary,
                           "Bajan solos cuando registras un cambio en Cartuchos.");
    lv_obj_set_pos(note, 16, 380);
    return page;
}

void SparesScreen::update(const core::SystemState& s) {
    for (size_t i = 0; i < rows_.size(); ++i) {
        counts_[i] = s.spares[static_cast<size_t>(filter_of(i))];
        show_count(i);
    }
}

void SparesScreen::show_count(size_t row) {
    char buf[8];
    snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(counts_[row]));
    set_text(rows_[row].count, buf);
    lv_obj_set_disabled(rows_[row].minus, counts_[row] == 0);
    lv_obj_set_disabled(rows_[row].plus, counts_[row] >= kMaxSpares);
}

void SparesScreen::handle_step(lv_event_t* e) {
    auto* self = static_cast<SparesScreen*>(lv_event_get_user_data(e));
    const auto code = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_current_target_obj(e)));
    const size_t row = code / 2;
    const bool plus = (code % 2) == 1;
    uint8_t& count = self->counts_[row];
    if (plus && count < kMaxSpares) ++count;
    if (!plus && count > 0) --count;
    // Se ve al instante; la app lo guarda y el siguiente update() lo confirma
    self->show_count(row);
    if (actions().spares_set != nullptr) actions().spares_set(filter_of(row), count);
}

}  // namespace ui
