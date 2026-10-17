#include "screens/events_screen.h"

#include <cctype>
#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"

// Medidas tomadas del artboard Alarmas del canvas (cada y menos 64).

namespace ui {
namespace {

// Lo que se pinta de un renglón
struct Look {
    const lv_image_dsc_t* icon;
    uint32_t tone;        // ícono
    uint32_t title_tone;  // título: los avisos y alarmas van en su color
};

Look look_of(core::EventKind kind) {
    switch (kind) {
        case core::EventKind::cartridge:
        case core::EventKind::cartridge_added:
        case core::EventKind::purge:
            return {&icon_cartucho_32, colors::text_secondary, colors::text};
        case core::EventKind::drop_warning:
        case core::EventKind::offline:
        case core::EventKind::mixer_fault:
        case core::EventKind::no_supply:
            return {&icon_advertencia_32, colors::warning, colors::warning};
        case core::EventKind::drop_alarm:
        case core::EventKind::alarm:
            return {&icon_alarma_32, colors::alarm, colors::alarm};
        case core::EventKind::dose:
        case core::EventKind::dose_typed:
            break;
    }
    return {&icon_dosis_32, colors::text_secondary, colors::text};
}

// "Hoy 06:10" o "Vie 2 oct 06:25"
void when_text(char* out, size_t size, core::Date today, uint16_t year, uint32_t today_count, const core::Event& ev) {
    char time[8];
    fmt::clock(time, sizeof time, ev.at);
    const uint32_t ago = today_count >= ev.day ? today_count - ev.day : 0;
    if (ago == 0) {
        snprintf(out, size, "Hoy %s", time);
        return;
    }
    const core::Date d = core::date_days_ago(today, year, ago);
    snprintf(out, size, "%s %u %s %s", fmt::weekday_short(d.weekday), static_cast<unsigned>(d.day),
             fmt::month_short(d.month), time);
}

}  // namespace

lv_obj_t* EventsScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);

    static const char* const kFilters[3] = {"Activas", "Hoy", "Últimos 7 días"};
    lv_obj_t* frame = picker_.create(page, 16, 12, 0, 64, kFilters, 3, on_filter, this);
    // Aquí el canvas les da 20 px por lado y no 18 como en Historial
    for (uint32_t i = 0; i < lv_obj_get_child_count(frame); ++i) {
        lv_obj_set_style_pad_hor(lv_obj_get_child(frame, static_cast<int32_t>(i)), 20, 0);
    }
    picker_.set_selected(static_cast<int>(filter_));

    summary_ = label(page, &font_label_26, colors::text_secondary, "");
    lv_obj_align(summary_, LV_ALIGN_TOP_RIGHT, -16, 28);

    // Caben 5 renglones; con más, la lista se recorre con el dedo
    lv_obj_t* list = box(page, 16, 88, 992, 368);
    lv_obj_set_clickable(list, true);
    lv_obj_set_scrollable(list, true);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_style_width(list, 4, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(list, 2, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(list, color(colors::text_disabled), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_pad_bottom(list, 8, 0);  // el último renglón no queda pegado a la barra
    list_ = list;

    for (size_t i = 0; i < rows_.size(); ++i) {
        Row& r = rows_[i];
        r.obj = card(list, 0, 73 * static_cast<int32_t>(i), 992, 64);
        r.icon = icon(r.obj, &icon_dosis_32, colors::text_secondary);
        lv_obj_set_pos(r.icon, 16, 15);
        r.title = label(r.obj, &font_body_32, colors::text, "");
        lv_obj_set_pos(r.title, 60, 2);
        r.when = label(r.obj, &font_label_26, colors::text_secondary, "");
        lv_obj_set_pos(r.when, 60, 30);
        r.status = label(r.obj, &font_label_26, colors::text_secondary, "");
        lv_obj_align(r.status, LV_ALIGN_TOP_RIGHT, -16, 15);
    }

    // Sin alarmas activas: una tarjeta grande con la palomita
    empty_ = card(page, 16, 88, 992, 356);
    lv_obj_set_flex_flow(empty_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(empty_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(empty_, 12, 0);
    icon(empty_, &icon_ok_56, colors::ok);
    label(empty_, &font_title_36, colors::text, "Sin alarmas activas");
    empty_note_ = label(empty_, &font_label_26, colors::text_secondary, "");
    return page;
}

void EventsScreen::update(const core::SystemState& s) {
    events_ = s.recent_events;
    event_count_ = s.recent_event_count;
    alarms_ = s.alarms;
    today_ = s.today;
    date_ = core::today(s);
    year_ = s.year;
    refresh();
}

void EventsScreen::on_filter(int index, void* user_data) {
    auto* self = static_cast<EventsScreen*>(user_data);
    self->filter_ = static_cast<Filter>(index);
    self->refresh();
}

void EventsScreen::refresh() {
    char title[48];
    char when[96];
    char status[32];
    char time[8];
    size_t shown = 0;

    if (filter_ == Filter::active) {
        // Las alarmas que siguen activas, reconocidas o no
        for (int k = 0; k < core::kAlarmKinds && shown < rows_.size(); ++k) {
            const core::Alarm& a = alarms_[static_cast<size_t>(k)];
            if (!a.active) continue;
            Row& r = rows_[shown++];
            lv_obj_set_hidden(r.obj, false);
            lv_image_set_src(r.icon, &icon_alarma_32);
            set_icon_color(r.icon, colors::alarm);
            set_text(r.title, fmt::alarm_title(static_cast<unsigned>(k)));
            lv_obj_set_style_text_color(r.title, color(colors::alarm), 0);
            fmt::clock(time, sizeof time, a.since);
            snprintf(when, sizeof when, "Desde %s", time);
            set_text(r.when, when);
            set_text(r.status, a.acknowledged ? "Reconocida" : "Activa");
        }
    }
    for (size_t i = 0; i < event_count_ && shown < rows_.size(); ++i) {
        const core::Event& ev = events_[i];
        const uint32_t ago = today_ >= ev.day ? today_ - ev.day : 0;
        // Avisos y alarmas: si ya se resolvió y cuándo
        const bool notice = ev.kind == core::EventKind::drop_warning || ev.kind == core::EventKind::drop_alarm ||
                            ev.kind == core::EventKind::offline || ev.kind == core::EventKind::alarm ||
                            ev.kind == core::EventKind::mixer_fault || ev.kind == core::EventKind::no_supply;
        if (filter_ == Filter::active) {
            // Además de las alarmas, los avisos que siguen sin quitarse
            if (!notice || ev.kind == core::EventKind::alarm || ev.resolved_at != core::kNoTime) continue;
        } else if (filter_ == Filter::today ? ago != 0 : ago >= 7) {
            continue;
        }

        const char* detail = "";
        char hint[48] = "";
        switch (ev.kind) {
            case core::EventKind::dose:
            case core::EventKind::dose_typed:
                snprintf(title, sizeof title, "Dosis %u mL", static_cast<unsigned>(ev.value));
                detail = ev.kind == core::EventKind::dose ? " · en la boca B" : " · otra cantidad";
                snprintf(status, sizeof status, "Registrada");
                break;
            case core::EventKind::cartridge:
                snprintf(title, sizeof title, "Cambio de cartucho %s", fmt::filter_name(ev.value));
                snprintf(status, sizeof status, "Registrado");
                break;
            case core::EventKind::cartridge_added:
                snprintf(title, sizeof title, "Se puso cartucho %s", fmt::filter_name(ev.value));
                snprintf(status, sizeof status, "Registrado");
                break;
            case core::EventKind::purge:
                snprintf(title, sizeof title, "Purga del prefiltro");
                snprintf(status, sizeof status, "Registrada");
                break;
            case core::EventKind::drop_warning:
            case core::EventKind::drop_alarm:
                snprintf(title, sizeof title, "Caída del tren %u psi · %s", static_cast<unsigned>(ev.value),
                         ev.kind == core::EventKind::drop_warning ? "Revisar" : "Tapados");
                break;
            case core::EventKind::offline:
                snprintf(title, sizeof title, "Sin red %u min", static_cast<unsigned>(ev.value));
                detail = " · se guardó y se mandó después";
                break;
            case core::EventKind::alarm:
                snprintf(title, sizeof title, "%s", fmt::alarm_title(ev.value));
                break;
            case core::EventKind::mixer_fault:
                snprintf(title, sizeof title, "Bomba R · %s", fmt::mixer_fault(ev.value));
                // A media frase en minúscula: "Hoy 18:42 · no se mezcló: espera agua nueva"
                {
                    // El "·" son 2 bytes en UTF-8: la letra va después de todo el prefijo
                    static constexpr char kDot[] = " · ";
                    snprintf(hint, sizeof hint, "%s%s", kDot, fmt::mixer_fault_hint(ev.value));
                    char& first = hint[sizeof kDot - 1];
                    first = static_cast<char>(std::tolower(static_cast<unsigned char>(first)));
                }
                detail = hint;
                break;
            case core::EventKind::no_supply:
                snprintf(title, sizeof title, "Sin agua del SIAPA");
                detail = " · sin flujo ni presión";
                break;
        }
        if (notice) {
            if (ev.resolved_at == core::kNoTime) {
                snprintf(status, sizeof status, "Activa");
            } else {
                fmt::clock(time, sizeof time, ev.resolved_at);
                snprintf(status, sizeof status, "Resuelta %s", time);
            }
        }
        char ack[32] = "";
        if (ev.kind == core::EventKind::alarm && ev.acknowledged_at != core::kNoTime) {
            fmt::clock(time, sizeof time, ev.acknowledged_at);
            snprintf(ack, sizeof ack, " · reconocida %s", time);
            detail = ack;
        }
        char date[32];
        when_text(date, sizeof date, date_, year_, today_, ev);
        snprintf(when, sizeof when, "%s%s", date, detail);

        Row& r = rows_[shown++];
        const Look look = look_of(ev.kind);
        lv_obj_set_hidden(r.obj, false);
        lv_image_set_src(r.icon, look.icon);
        set_icon_color(r.icon, look.tone);
        set_text(r.title, title);
        lv_obj_set_style_text_color(r.title, color(look.title_tone), 0);
        set_text(r.when, when);
        set_text(r.status, status);
    }
    if (filter_ == Filter::active) {
        snprintf(status, sizeof status, shown == 1 ? "1 activa" : "%u activas", static_cast<unsigned>(shown));
    } else {
        snprintf(status, sizeof status, shown == 1 ? "1 evento" : "%u eventos", static_cast<unsigned>(shown));
    }
    set_text(summary_, status);
    for (size_t i = shown; i < rows_.size(); ++i) lv_obj_set_hidden(rows_[i].obj, true);
    if (filter_ != shown_filter_) {  // otro filtro: la lista vuelve arriba
        shown_filter_ = filter_;
        lv_obj_scroll_to_y(list_, 0, LV_ANIM_OFF);
    }

    // Activas sin nada: la tarjeta grande dice cuándo fue la última
    const bool empty = filter_ == Filter::active && shown == 0;
    lv_obj_set_hidden(empty_, !empty);
    if (!empty) return;
    for (size_t i = 0; i < event_count_; ++i) {
        const core::Event& ev = events_[i];
        if (ev.kind != core::EventKind::alarm) continue;
        const core::Date d = core::date_days_ago(date_, year_, today_ >= ev.day ? today_ - ev.day : 0);
        // A media frase el día va en minúscula: "el mar 29 sep"
        char weekday[8];
        snprintf(weekday, sizeof weekday, "%s", fmt::weekday_short(d.weekday));
        weekday[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(weekday[0])));
        snprintf(when, sizeof when, "La última fue el %s %u %s y ya se resolvió", weekday,
                 static_cast<unsigned>(d.day), fmt::month_short(d.month));
        set_text(empty_note_, when);
        return;
    }
    set_text(empty_note_, "No ha habido alarmas");
}

}  // namespace ui
