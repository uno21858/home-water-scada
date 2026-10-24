#include "screens/overlays/alarm_screen.h"

#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "ui.h"

// Medidas tomadas del artboard EstadoAlarma del canvas (pantalla completa).

namespace ui {

void AlarmScreen::create(lv_obj_t* layer) {
    root_ = box(layer, 0, 0, 1024, 600);
    fill(root_, colors::alarm_screen);
    lv_obj_set_clickable(root_, true);  // se come los toques: no pasan a lo de abajo

    lv_obj_set_pos(icon(root_, &icon_alarma_96, colors::on_alarm), 48, 44);

    // Título, qué pasó, qué revisar y los datos, uno abajo del otro
    lv_obj_t* text = column(root_, 0, LV_FLEX_ALIGN_START);
    lv_obj_set_pos(text, 176, 44);
    lv_obj_set_width(text, 800);
    title_ = label(text, &font_title_xl_48, colors::on_alarm, "");
    lv_obj_set_width(title_, 800);
    lv_obj_set_style_margin_bottom(title_, 20, 0);
    what_ = label(text, &font_body_32, colors::on_alarm, "");
    lv_obj_set_width(what_, 800);
    lv_obj_set_style_margin_bottom(what_, 16, 0);
    check_ = label(text, &font_body_32, colors::on_alarm, "");
    lv_obj_set_width(check_, 800);
    lv_obj_set_style_margin_bottom(check_, 28, 0);
    lv_obj_t* chips = row(text, 12, LV_FLEX_ALIGN_CENTER);
    for (lv_obj_t*& chip : chips_) {
        lv_obj_t* frame = box(chips);
        lv_obj_set_size(frame, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_border_width(frame, 2, 0);
        lv_obj_set_style_border_color(frame, color(colors::on_alarm), 0);
        lv_obj_set_style_radius(frame, 6, 0);
        lv_obj_set_style_pad_ver(frame, 6, 0);
        lv_obj_set_style_pad_hor(frame, 14, 0);
        chip = label(frame, &font_label_26, colors::on_alarm, "");
    }

    count_ = label(root_, &font_label_26, colors::on_alarm, "");
    lv_obj_set_pos(count_, 48, 488);

    // Entendido: blanco con letra roja
    lv_obj_t* ok = box(root_, 616, 456, 360, 96);
    lv_obj_set_clickable(ok, true);
    fill(ok, colors::on_alarm);
    lv_obj_set_style_bg_color(ok, color(colors::primary_pressed), LV_STATE_PRESSED);
    lv_obj_set_style_radius(ok, 10, 0);
    lv_obj_set_flex_flow(ok, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ok, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(ok, 12, 0);
    icon(ok, &icon_check_32, colors::alarm_screen);
    label(ok, &font_title_36, colors::alarm_screen, "Entendido");
    lv_obj_add_event_cb(ok, handle_ok, LV_EVENT_CLICKED, this);

    lv_obj_set_hidden(root_, true);
}

void AlarmScreen::update(const core::SystemState& s) {
    // La primera sin reconocer, y cuántas hay
    int first = -1;
    int pending = 0;
    for (int i = 0; i < core::kAlarmKinds; ++i) {
        const core::Alarm& a = s.alarms[static_cast<size_t>(i)];
        if (!a.active || a.acknowledged) continue;
        if (first < 0) first = i;
        ++pending;
    }

    lv_obj_set_hidden(root_, first < 0);
    if (first < 0) {
        shown_ = -1;
        return;
    }
    shown_ = first;

    char since[8];
    fmt::clock(since, sizeof since, s.alarms[static_cast<size_t>(first)].since);
    char buf[200];
    char c0[32];
    char c1[32];
    char c2[32];
    snprintf(c2, sizeof c2, "Desde %s", since);
    set_text(title_, fmt::alarm_title(static_cast<unsigned>(first)));
    switch (static_cast<core::AlarmKind>(first)) {
        case core::AlarmKind::no_flow:
            set_text(what_, "Con la bomba B encendida, el sensor F marcó 0 L/min durante 10 s. La bomba se apagó.");
            set_text(check_, "Revisa que la válvula V1 esté abierta y que llegue agua del SIAPA.");
            snprintf(c0, sizeof c0, "F %.1f L/min", static_cast<double>(s.flow_lpm));
            snprintf(c1, sizeof c1, "Bomba B apagada");
            break;
        case core::AlarmKind::sensor:
            snprintf(buf, sizeof buf,
                     "La caída del tren marca %d psi, o sea que sí pasa agua, pero F lleva 30 s sin mandar pulsos. "
                     "El conteo de litros se pausó.",
                     s.p1_psi - s.p2_psi);
            set_text(what_, buf);
            set_text(check_, "Revisa el cable de F en la caja del pilar y que el conector esté seco.");
            snprintf(c0, sizeof c0, "F sin pulsos");
            snprintf(c1, sizeof c1, "Caída %d psi", s.p1_psi - s.p2_psi);
            break;
        case core::AlarmKind::daily_cap:
            snprintf(buf, sizeof buf, "La bomba B ya echó %u mL hoy, el tope que pusiste. Se detuvo hasta mañana.",
                     static_cast<unsigned>(s.daily_cap_ml));
            set_text(what_, buf);
            set_text(check_, "Si sí hace falta más cloro, sube el tope en Ajustes, Dosis.");
            snprintf(c0, sizeof c0, "%u mL hoy", static_cast<unsigned>(s.daily_cap_ml));
            snprintf(c1, sizeof c1, "Bomba B parada");
            break;
    }
    set_text(chips_[0], c0);
    set_text(chips_[1], c1);
    set_text(chips_[2], c2);

    // "Alarma 1 de 2": cuántas faltan por reconocer, contando esta
    snprintf(buf, sizeof buf, "Alarma 1 de %d · queda en Historial", pending);
    set_text(count_, buf);
}

void AlarmScreen::handle_ok(lv_event_t* e) {
    auto* self = static_cast<AlarmScreen*>(lv_event_get_user_data(e));
    if (self->shown_ < 0) return;
    const auto kind = static_cast<core::AlarmKind>(self->shown_);
    // Se quita al instante; si la app no la reconoce, regresa en el siguiente update
    lv_obj_set_hidden(self->root_, true);
    if (actions().alarm_acknowledged != nullptr) actions().alarm_acknowledged(kind);
}

}  // namespace ui
