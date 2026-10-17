#include "components/mixer.h"

#include <cstdio>

#include "components/primitives.h"
#include "mixing.h"
#include "theme/colors.h"
#include "theme/format.h"
#include "theme/icons.h"

namespace ui {
namespace {

void rotate(void* obj, int32_t angle) { lv_image_set_rotation(static_cast<lv_obj_t*>(obj), angle); }

}  // namespace

void Impeller::create(lv_obj_t* parent, int32_t x, int32_t y) {
    lv_obj_t* housing = box(parent, x + 2, y + 2, 52, 52);
    lv_obj_set_style_radius(housing, LV_RADIUS_CIRCLE, 0);
    fill(housing, colors::background);
    lv_obj_set_style_border_width(housing, 2, 0);
    lv_obj_set_style_border_color(housing, color(colors::text_secondary), 0);
    rotor_ = icon(parent, &icon_impulsor_56, colors::pipe);
    lv_obj_set_pos(rotor_, x, y);
}

void Impeller::set_spinning(bool on) {
    if (static_cast<int>(on) == spinning_) return;
    spinning_ = on;
    set_icon_color(rotor_, on ? colors::water : colors::pipe);
    if (on) {
        // Una vuelta cada 2 s. El ángulo de LVGL va en décimas de grado.
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, rotor_);
        lv_anim_set_exec_cb(&a, rotate);
        lv_anim_set_values(&a, 0, 3600);
        lv_anim_set_duration(&a, 2000);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&a);
    } else {
        lv_anim_delete(rotor_, rotate);
        lv_image_set_rotation(rotor_, 0);
    }
}

void mixer_status(char* out, size_t size, const core::SystemState& s) {
    char at[8];
    const core::MixerReason reason = s.mixer_run_count > 0 ? s.mixer_runs[0].reason : core::MixerReason::manual;
    if (s.mixer_fault != core::MixerFault::none) {
        snprintf(out, size, "%s", fmt::mixer_fault_hint(static_cast<unsigned>(s.mixer_fault)));
    } else if (!s.mixer_on && s.mixer_mode != core::MixerMode::off && core::mixer_low_water(s)) {
        snprintf(out, size, "Cisterna en 0 %%: espera agua");
    } else if (s.mixer_on && reason == core::MixerReason::dose) {
        fmt::clock(at, sizeof at, s.mixer_started_at);
        snprintf(out, size, "Por la dosis de las %s", at);
    } else if (s.mixer_on && reason == core::MixerReason::water) {
        fmt::clock(at, sizeof at, s.mixer_started_at);
        snprintf(out, size, "Por agua nueva · desde las %s", at);
    } else if (s.mixer_on) {
        fmt::clock(at, sizeof at, core::minute_of_day(s) + s.mixer_minutes_left);
        snprintf(out, size, "A mano · se apaga sola a las %s", at);
    } else if (s.mixer_mode == core::MixerMode::automatic) {
        // Cuánta agua falta para la siguiente vuelta por litros
        const unsigned left = s.liters_since_mix < core::kMixEveryLiters ? core::kMixEveryLiters - s.liters_since_mix : 0;
        char liters[16];
        fmt::thousands(liters, sizeof liters, left);
        snprintf(out, size, "Auto · la siguiente en %s L", liters);
    } else {
        snprintf(out, size, "A mano · el programa no la prende");
    }
}

void mixer_state(lv_obj_t* label, const core::SystemState& s) {
    const bool fault = s.mixer_fault != core::MixerFault::none;
    set_text(label, fault ? fmt::mixer_fault(static_cast<unsigned>(s.mixer_fault)) : s.mixer_on ? "Prendida" : "Apagada");
    set_text_color(label, fault ? colors::warning : colors::text);
}

}  // namespace ui
