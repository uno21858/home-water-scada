#include "screens/settings/bench_screen.h"

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "ui.h"

namespace ui {
namespace {

const char* const kSteps[4] = {
    "Mete la bomba R en una cubeta con agua",
    "Toca Empezar y préndela en Mezcla",
    "Su corriente sale en Diagnóstico",
    "Al terminar, toca Ya terminé",
};

// La de ui.cpp: el diálogo de confirmar solo regresa una función sin datos
BenchScreen* instance = nullptr;

}  // namespace

lv_obj_t* BenchScreen::create(lv_obj_t* parent) {
    instance = this;
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);

    // Los pasos, como en Cambio de cartuchos
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

    // Cómo está la protección y el botón
    lv_obj_t* side = column(page, 12, LV_FLEX_ALIGN_START);
    lv_obj_set_pos(side, 496, 12);
    lv_obj_set_width(side, 512);
    label(side, &font_body_32, colors::text_secondary, "Protección de nivel");
    title_ = label(side, &font_title_xl_48, colors::text, "");
    note_ = label(side, &font_body_32, colors::text_secondary, "");
    lv_obj_set_width(note_, 512);

    lv_obj_t* button = button_primary(page, 488, 364, 320, "", handle_button, this);
    button_text_ = lv_obj_get_child(button, -1);
    show_state(false);
    return page;
}

void BenchScreen::update(const core::SystemState& s) { show_state(s.mixer_bench); }

void BenchScreen::show_state(bool active) {
    active_ = active;
    set_text(title_, active ? "Quitada" : "Activa");
    set_text_color(title_, active ? colors::warning : colors::text);
    set_text(note_, active ? "Prende con la cisterna en 0 % y Nivel bajo no la apaga. Atorada y Sin 12 V siguen. "
                             "Regresa sola al reiniciar."
                           : "Con la cisterna en 0 % no prende, y si jala poca corriente se apaga con Nivel bajo.");
    set_text(button_text_, active ? "Ya terminé" : "Empezar");
}

void BenchScreen::handle_button(lv_event_t* e) {
    auto* self = static_cast<BenchScreen*>(lv_event_get_user_data(e));
    if (self->active_) {
        // Regresar la protección no necesita confirmar
        self->show_state(false);
        if (actions().mixer_bench_set != nullptr) actions().mixer_bench_set(false);
        return;
    }
    open_confirm("¿Quitar la protección de nivel?",
                 "La bomba R puede correr en seco. Solo con ella en una cubeta con agua.", "Sí, quitarla",
                 on_confirmed);
}

void BenchScreen::on_confirmed() {
    // Se ve al instante; el siguiente update() lo confirma con lo que guardó la app
    if (instance != nullptr) instance->show_state(true);
    if (actions().mixer_bench_set != nullptr) actions().mixer_bench_set(true);
}

}  // namespace ui
