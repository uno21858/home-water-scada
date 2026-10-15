#include "screens/filters_screen.h"

#include <algorithm>
#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "ui.h"

// Medidas tomadas del artboard Cartuchos del canvas (cada y menos 64).

namespace ui {
namespace {

// Cada cuánto se purga o se cambia, en días, para la barra
constexpr uint16_t kLifeDays[core::kFilterCount] = {7, 75, 135, 135, 180, 365};
const char* const kEvery[core::kFilterCount] = {"cada 7 días", "2 a 3 meses", "3 a 6 meses",
                                                "3 a 6 meses", "6 meses",     "1 año"};

// El diálogo de confirmar no pasa datos a su callback: aquí se guarda cuál se preguntó
core::Filter asked = core::Filter::prefilter;

void on_confirmed() {
    if (actions().filter_serviced != nullptr) actions().filter_serviced(asked);
}

}  // namespace

lv_obj_t* FiltersScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    for (size_t i = 0; i < rows_.size(); ++i) {
        Row& r = rows_[i];
        r.obj = card(page, 16, 12 + 72 * static_cast<int32_t>(i), 992, 60);
        r.icon = icon(r.obj, &icon_cartucho_32, colors::text);
        lv_obj_set_pos(r.icon, 16, 13);
        r.name = label(r.obj, &font_body_32, colors::text, fmt::filter_name(static_cast<unsigned>(i)));
        lv_obj_set_pos(r.name, 60, 10);
        r.days = label(r.obj, &font_body_32, colors::text, "");
        lv_obj_set_pos(r.days, 216, 10);
        r.liters = label(r.obj, &font_body_32, colors::text, "");
        lv_obj_set_pos(r.liters, 340, 10);

        // Barra de lo que lleva de su vida útil
        r.track = box(r.obj, 500, 24, 140, 10);
        lv_obj_set_style_radius(r.track, 5, 0);
        lv_obj_set_style_clip_corner(r.track, true, 0);
        r.bar = box(r.track, 0, 0, 0, 10);
        fill(r.bar, colors::text_secondary);

        r.every = label(r.obj, &font_label_26, colors::text_secondary, "");
        lv_obj_set_pos(r.every, 652, 13);

        lv_obj_t* button = button_secondary(r.obj, 806, 1, 176, "", handle_service, this);
        lv_obj_set_height(button, 56);
        lv_obj_set_user_data(button, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        r.button_text = lv_obj_get_child(button, 0);
    }
    return page;
}

void FiltersScreen::update(const core::SystemState& s) {
    char buf[32];
    for (size_t i = 0; i < rows_.size(); ++i) {
        Row& r = rows_[i];
        const core::FilterStatus& f = s.filters[i];
        installed_[i] = f.installed;

        // Sin cartucho: todo el renglón en texto-3 y la pista oscura
        if (static_cast<int>(f.installed) != r.installed) {
            r.installed = f.installed;
            const uint32_t tone = f.installed ? colors::text : colors::text_disabled;
            set_icon_color(r.icon, tone);
            for (lv_obj_t* text : {r.name, r.days, r.liters}) lv_obj_set_style_text_color(text, color(tone), 0);
            fill(r.track, f.installed ? colors::panel_raised : colors::panel);
        }

        const bool prefilter = i == static_cast<size_t>(core::Filter::prefilter);
        set_text(r.button_text, prefilter ? "Ya purgué" : f.installed ? "Lo cambié" : "Lo puse");

        if (!f.installed) {
            set_text(r.days, "Fuera");
            set_text(r.liters, "vaso vacío");
            lv_obj_set_width(r.bar, 0);
            set_text(r.every, s.spares[i] == 0 ? "sin repuesto" : kEvery[i]);
            continue;
        }
        const uint32_t days = s.today >= f.since_day ? s.today - f.since_day : 0;
        if (days == 0) {
            snprintf(buf, sizeof buf, "Hoy");
        } else {
            snprintf(buf, sizeof buf, days == 1 ? "%lu día" : "%lu días", static_cast<unsigned long>(days));
        }
        set_text(r.days, buf);
        char liters[16];
        fmt::thousands(liters, sizeof liters, f.liters);
        snprintf(buf, sizeof buf, "%s L", liters);
        set_text(r.liters, buf);
        const uint32_t pct = std::min<uint32_t>(100, (days * 100 + kLifeDays[i] / 2) / kLifeDays[i]);
        lv_obj_set_width(r.bar, static_cast<int32_t>(140 * pct / 100));
        set_text(r.every, kEvery[i]);
    }
    now_ = core::minute_of_day(s);
}

void FiltersScreen::handle_service(lv_event_t* e) {
    const auto* self = static_cast<FiltersScreen*>(lv_event_get_user_data(e));
    lv_obj_t* button = lv_event_get_current_target_obj(e);
    const auto i = static_cast<size_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(button)));
    asked = static_cast<core::Filter>(i);

    char question[48];
    const char* yes = "Sí, lo cambié";
    if (asked == core::Filter::prefilter) {
        snprintf(question, sizeof question, "¿Ya purgaste el prefiltro?");
        yes = "Sí, ya purgué";
    } else if (!self->installed_[i]) {
        snprintf(question, sizeof question, "¿Pusiste el de %s?", i == 5 ? "sarro" : fmt::filter_name(i));
        yes = "Sí, ya lo puse";
    } else if (asked == core::Filter::carbon) {
        snprintf(question, sizeof question, "¿Cambiaste el CTO?");
    } else {
        snprintf(question, sizeof question, "¿Cambiaste el de %s?", i == 5 ? "sarro" : fmt::filter_name(i));
    }
    char time[8];
    char body[96];
    fmt::clock(time, sizeof time, self->now_);
    snprintf(body, sizeof body, "Se registra hoy a las %s y su conteo de días y litros vuelve a 0.", time);
    open_confirm(question, body, yes, on_confirmed);
}

}  // namespace ui
