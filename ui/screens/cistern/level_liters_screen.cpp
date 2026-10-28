#include "screens/cistern/level_liters_screen.h"

#include <cmath>
#include <cstdio>

#include "components/primitives.h"
#include "level.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "ui.h"

namespace ui {
namespace {

// Cada paso cabe en dos renglones de 354 px
const char* const kSteps[4] = {
    "Apaga la bomba de pozo",
    "Con agua entrando, toca Empezar",
    "Espera a que suba unos 10 cm",
    "Guarda y vuelve a prender la bomba",
};

}  // namespace

lv_obj_t* LevelLitersScreen::create(lv_obj_t* parent) {
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

    // Lo medido: cifra de 72 con su unidad, como los ajustes
    lv_obj_set_pos(label(page, &font_body_32, colors::text_secondary, "Litros por cm"), 496, 12);
    lv_obj_t* r = row(page, 10, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(r, 496, 56);
    value_ = label(r, &font_digits_72, colors::text, "");
    unit_ = label(r, &font_title_36, colors::text_secondary, "L por cm");
    lv_obj_set_style_pad_bottom(unit_, 2, 0);
    none_ = label(page, &font_title_xl_48, colors::text_secondary, "Sin medir");
    lv_obj_set_pos(none_, 496, 64);
    note_ = label(page, &font_body_32, colors::text_secondary, "");
    lv_obj_set_width(note_, 512);
    lv_obj_set_pos(note_, 496, 156);

    primary_ = button_primary(page, 488, 364, 320, "", handle_primary, this);
    primary_text_ = lv_obj_get_child(primary_, -1);
    cancel_ = button_secondary(page, 820, 364, 188, "Cancelar", [](lv_event_t*) {
        if (actions().level_measure_canceled != nullptr) actions().level_measure_canceled();
    }, nullptr);
    return page;
}

void LevelLitersScreen::update(const core::SystemState& s) {
    measuring_ = s.level_measuring;
    const bool ready = core::level_measure_ready(s);
    // Midiendo enseña lo que va; si no, lo guardado
    const float shown = measuring_ ? core::level_measure_ratio(s) : s.level_liters_per_cm;
    char buf[160];
    fmt::one_decimal(buf, sizeof buf, shown);
    set_text(value_, buf);
    lv_obj_set_hidden(value_, shown <= 0.0f);
    lv_obj_set_hidden(unit_, shown <= 0.0f);
    lv_obj_set_hidden(none_, shown > 0.0f);
    set_text(none_, measuring_ ? "Midiendo" : "Sin medir");

    // La nota: cómo va, o qué falta
    const bool lost = s.no_signal.level;
    bool warn = false;
    if (!measuring_) {
        snprintf(buf, sizeof buf, "%s",
                 s.level_liters_per_cm > 0.0f
                     ? "Con esto salen los litros de la cisterna y el tope de \"Toca echar\". Se puede volver a medir."
                     : "Con la bomba de pozo apagada nadie saca agua: F cuenta los litros que entran y el "
                       "sensor los cm que sube.");
    } else if (lost) {
        snprintf(buf, sizeof buf, "Sin señal del nivel: revisa el cable del sensor.");
        warn = true;
    } else {
        // En la frase sin el espacio de cifra de one_decimal: "subió  3.2" se ve con hueco
        char liters[16];
        char rise[16];
        fmt::thousands(liters, sizeof liters, s.level_measure_liters);
        snprintf(rise, sizeof rise, "%.1f", static_cast<double>(core::level_measure_rise(s)));
        const bool flowing = s.flow_lpm >= core::kFlowingLpm;
        if (!flowing) {
            snprintf(buf, sizeof buf, "Entraron %s L y subió %s cm. Ahorita no está entrando agua.", liters, rise);
            warn = true;
        } else if (!ready) {
            snprintf(buf, sizeof buf, "Entraron %s L y subió %s cm. Se puede guardar desde %.0f cm.", liters, rise,
                     static_cast<double>(core::kLevelMeasureMinCm));
        } else {
            snprintf(buf, sizeof buf, "Entraron %s L y subió %s cm. Mientras más suba, más exacto.", liters, rise);
        }
    }
    set_text(note_, buf);
    set_text_color(note_, warn ? colors::warning : colors::text_secondary);

    if (measuring_ && ready) {
        char value[16];
        fmt::one_decimal(value, sizeof value, core::level_measure_ratio(s));
        snprintf(buf, sizeof buf, "Guardar %s", value);
    } else {
        snprintf(buf, sizeof buf, "%s", measuring_ ? "Guardar" : "Empezar");
    }
    set_text(primary_text_, buf);
    lv_obj_set_disabled(primary_, measuring_ ? !ready : lost || !s.has_level);
    lv_obj_set_hidden(cancel_, !measuring_);
}

void LevelLitersScreen::handle_primary(lv_event_t* e) {
    const auto* self = static_cast<LevelLitersScreen*>(lv_event_get_user_data(e));
    if (self->measuring_) {
        if (actions().level_measure_saved != nullptr) actions().level_measure_saved();
    } else {
        if (actions().level_measure_started != nullptr) actions().level_measure_started();
    }
}

}  // namespace ui
