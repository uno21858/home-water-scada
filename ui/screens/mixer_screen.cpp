#include "screens/mixer_screen.h"

#include <cstdio>

#include "components/keypad_dialog.h"
#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "theme/styles.h"
#include "ui.h"

// Medidas tomadas del artboard Mezcla del canvas (cada y menos 64).
// Las coordenadas dentro de una tarjeta cuentan desde adentro del borde.

namespace ui {
namespace {

// Mismo orden que el selector y que core::MixerMode
const char* const kModes[3] = {"Apagada", "Prendida", "Auto"};

const char* reason_word(core::MixerReason reason) {
    switch (reason) {
        case core::MixerReason::dose: return "dosis";
        case core::MixerReason::manual: return "a mano";
        case core::MixerReason::schedule: break;
    }
    return "programa";
}

void on_mode(int index, void*) {
    if (actions().mixer_mode_set != nullptr) actions().mixer_mode_set(static_cast<core::MixerMode>(index));
}

// Renglón tocable del programa: nombre, minutos y flecha. Regresa la cifra.
lv_obj_t* program_row(lv_obj_t* parent, int32_t y, const char* name, lv_event_cb_t on_click, void* user_data) {
    lv_obj_t* r = box(parent, 0, y, 598, 52);
    lv_obj_set_clickable(r, true);
    lv_obj_add_style(r, &styles::card_pressed, LV_STATE_PRESSED);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(r, 16, 0);
    lv_obj_set_style_pad_column(r, 8, 0);
    lv_obj_add_event_cb(r, on_click, LV_EVENT_CLICKED, user_data);

    lv_obj_t* text = label(r, &font_body_32, colors::text, name);
    lv_obj_set_flex_grow(text, 1);
    lv_obj_t* value = label(r, &font_digits_44, colors::text, "");
    label(r, &font_label_26, colors::text_secondary, "min");
    lv_obj_t* arrow = icon(r, &icon_chevron_28, colors::text_secondary);
    lv_obj_set_style_margin_left(arrow, 8, 0);
    return value;
}

}  // namespace

lv_obj_t* MixerScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    live_ = box(page, 0, 0, 1024, 456);
    create_status(live_);
    mode_.create(live_, 16, 184, 600, 64, kModes, 3, on_mode);
    create_program(live_);
    create_today(live_);
    create_runs(live_);
    locked_ = locked_card(page, 16, 12, 992, 432, "Bomba R · mezcla", "Próximamente · fase 2",
                          "Revuelve el agua de la cisterna");
    return page;
}

void MixerScreen::create_status(lv_obj_t* page) {
    lv_obj_t* c = card(page, 16, 12, 600, 160);

    impeller_.create(c, 16, 16);
    lv_obj_t* name = label(c, &font_label_26, colors::text_secondary, "Bomba R");
    lv_obj_set_pos(name, 88, 8);
    state_ = label(c, &font_title_xl_48, colors::text, "");
    lv_obj_set_pos(state_, 88, 38);

    // A la derecha: "Faltan 3 min" en Auto, "Se apaga en 60 min" a mano
    side_ = column(c, 0, LV_FLEX_ALIGN_END);
    lv_obj_align(side_, LV_ALIGN_TOP_RIGHT, -16, 10);
    side_text_ = label(side_, &font_label_26, colors::text_secondary, "");
    lv_obj_t* minutes = row(side_, 6, LV_FLEX_ALIGN_END);
    side_minutes_ = label(minutes, &font_digits_44, colors::text, "");
    label(minutes, &font_label_26, colors::text_secondary, "min");

    bar_ = lv_bar_create(c);
    lv_obj_remove_style_all(bar_);
    lv_obj_set_clickable(bar_, false);
    lv_obj_set_pos(bar_, 16, 106);
    lv_obj_set_size(bar_, 566, 8);
    lv_obj_set_style_radius(bar_, 4, 0);
    fill(bar_, colors::border);
    lv_obj_set_style_radius(bar_, 4, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar_, color(colors::water), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar_, LV_OPA_COVER, LV_PART_INDICATOR);

    sub_ = label(c, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(sub_, 16, 120);
}

void MixerScreen::create_program(lv_obj_t* page) {
    lv_obj_t* c = card(page, 16, 260, 600, 184);

    lv_obj_t* title = label(c, &font_label_26, colors::text_secondary, "Programa del modo Auto");
    lv_obj_set_pos(title, 16, 6);

    after_dose_ = program_row(c, 42, "Después de cada dosis", handle_after_dose, this);
    fill(box(c, 16, 94, 566, 1), colors::border);
    every_4h_ = program_row(c, 95, "Cada 4 horas", handle_every_4h, this);

    manual_note_ = label(c, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(manual_note_, 16, 148);
}

void MixerScreen::create_today(lv_obj_t* page) {
    lv_obj_t* c = card(page, 628, 12, 380, 160);

    lv_obj_t* title = label(c, &font_label_26, colors::text_secondary, "Hoy prendida");
    lv_obj_set_pos(title, 16, 8);

    // Cifra y unidad en la misma línea base: la unidad (titulo) lleva 2 px abajo
    lv_obj_t* value_row = row(c, 10, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(value_row, 16, 44);
    today_minutes_ = label(value_row, &font_digits_72, colors::text, "");
    lv_obj_t* unit = label(value_row, &font_title_36, colors::text_secondary, "min");
    lv_obj_set_style_pad_bottom(unit, 2, 0);

    today_runs_ = label(c, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(today_runs_, 16, 116);
}

void MixerScreen::create_runs(lv_obj_t* page) {
    lv_obj_t* c = card(page, 628, 184, 380, 260);

    lv_obj_t* title = label(c, &font_label_26, colors::text_secondary, "Últimas veces · min");
    lv_obj_set_pos(title, 16, 6);

    // Hora, por qué y cuánto; entre renglones una raya de 1 px
    for (size_t i = 0; i < runs_.size(); ++i) {
        Run& r = runs_[i];
        r.obj = box(c, 0, 42 + 52 * static_cast<int32_t>(i), 378, 52);
        lv_obj_set_flex_flow(r.obj, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(r.obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_hor(r.obj, 16, 0);
        if (i > 0) {
            lv_obj_set_style_border_side(r.obj, LV_BORDER_SIDE_TOP, 0);
            lv_obj_set_style_border_width(r.obj, 1, 0);
            lv_obj_set_style_border_color(r.obj, color(colors::border), 0);
        }
        r.time = label(r.obj, &font_body_32, colors::text, "");
        lv_obj_set_width(r.time, 112);
        r.reason = label(r.obj, &font_label_26, colors::text_secondary, "");
        lv_obj_set_flex_grow(r.reason, 1);
        r.minutes = label(r.obj, &font_body_32, colors::text, "");
        r.now = label(r.obj, &font_label_26, colors::water, "ahora");
    }
}

void MixerScreen::update(const core::SystemState& s) {
    lv_obj_set_hidden(live_, !s.has_mixer);
    lv_obj_set_hidden(locked_, s.has_mixer);
    if (!s.has_mixer) {
        impeller_.set_spinning(false);
        return;
    }

    char buf[64];
    set_text(state_, s.mixer_on ? "Prendida" : "Apagada");
    impeller_.set_spinning(s.mixer_on);
    lv_obj_set_hidden(side_, !s.mixer_on);
    lv_obj_set_hidden(bar_, !s.mixer_on);
    set_text(side_text_, s.mixer_mode == core::MixerMode::automatic ? "Faltan" : "Se apaga en");
    snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(s.mixer_minutes_left));
    set_text(side_minutes_, buf);
    if (lv_bar_get_value(bar_) != s.mixer_progress_pct) lv_bar_set_value(bar_, s.mixer_progress_pct, LV_ANIM_OFF);
    mixer_status(buf, sizeof buf, s);
    set_text(sub_, buf);
    mode_.set_selected(static_cast<int>(s.mixer_mode));

    after_dose_min_ = s.mixer_after_dose_min;
    every_4h_min_ = s.mixer_every_4h_min;
    snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(after_dose_min_));
    set_text(after_dose_, buf);
    snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(every_4h_min_));
    set_text(every_4h_, buf);
    snprintf(buf, sizeof buf, "A mano se apaga sola a los %u min", static_cast<unsigned>(s.mixer_manual_max_min));
    set_text(manual_note_, buf);

    snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(s.mixer_today_minutes));
    set_text(today_minutes_, buf);
    snprintf(buf, sizeof buf, s.mixer_today_runs == 1 ? "%u vez" : "%u veces",
             static_cast<unsigned>(s.mixer_today_runs));
    set_text(today_runs_, buf);

    for (size_t i = 0; i < runs_.size(); ++i) {
        Run& r = runs_[i];
        const bool used = i < s.mixer_run_count;
        lv_obj_set_hidden(r.obj, !used);
        if (!used) continue;
        const core::MixerRun& run = s.mixer_runs[i];
        fmt::clock(buf, sizeof buf, run.at);
        set_text(r.time, buf);
        set_text(r.reason, reason_word(run.reason));
        const bool running = i == 0 && s.mixer_on;  // la más nueva, si sigue prendida
        lv_obj_set_hidden(r.minutes, running);
        lv_obj_set_hidden(r.now, !running);
        snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(run.minutes));
        set_text(r.minutes, buf);
    }
}

void MixerScreen::handle_after_dose(lv_event_t* e) {
    const auto* self = static_cast<MixerScreen*>(lv_event_get_user_data(e));
    char hint[32];
    snprintf(hint, sizeof hint, "Ahorita: %u min", static_cast<unsigned>(self->after_dose_min_));
    open_keypad({"Después de cada dosis", "Minutos que corre", "min", 0, 2, hint, nullptr, nullptr},
                [](uint32_t minutes) {
                    if (actions().mixer_after_dose_set != nullptr) actions().mixer_after_dose_set(minutes);
                });
}

void MixerScreen::handle_every_4h(lv_event_t* e) {
    const auto* self = static_cast<MixerScreen*>(lv_event_get_user_data(e));
    char hint[32];
    snprintf(hint, sizeof hint, "Ahorita: %u min", static_cast<unsigned>(self->every_4h_min_));
    open_keypad({"Cada 4 horas", "Minutos que corre", "min", 0, 2, hint, nullptr, nullptr}, [](uint32_t minutes) {
        if (actions().mixer_every_4h_set != nullptr) actions().mixer_every_4h_set(minutes);
    });
}

}  // namespace ui
