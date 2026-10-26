#include "components/clog_look.h"

#include <cmath>
#include <cstdio>

#include "clog.h"
#include "theme/colors.h"
#include "theme/format.h"
#include "theme/icons.h"

namespace ui {

ClogLook clog_look(const core::SystemState& s) {
    ClogLook look;
    look.flowing = s.flow_lpm >= core::kClogMinLpm;
    look.has_r = s.clog_r > 0.0f;
    if (look.has_r) {
        snprintf(look.r, sizeof look.r, s.clog_r >= 10.0f ? "%.0f" : "%.1f", static_cast<double>(s.clog_r));
        fmt::clock(look.at, sizeof look.at, s.clog_r_at);
    }
    const float ratio = core::clog_ratio(s);
    if (ratio > 0.0f) look.pct = static_cast<int>(std::lround(ratio * 100.0f));

    if (!core::pressure_ok(s)) {
        look.kind = ClogLook::no_signal;
        look.word = "Sin señal";
        look.fg = look.border = colors::warning;
        look.bg = colors::warning_bg;
        look.icon = &icon_advertencia_28;
        return look;
    }
    if (s.clog_r0 <= 0.0f || s.clog_state == core::ClogState::none) {
        // Sin R0 no se califica; con R0 pero sin evaluar todavía, tampoco
        look.kind = s.clog_r0 <= 0.0f ? ClogLook::no_reference : ClogLook::waiting;
        look.word = s.clog_r0 <= 0.0f ? "Sin referencia" : "";
        look.fg = colors::text_secondary;
        look.bg = colors::panel_raised;
        look.border = colors::border;
        look.icon = &icon_info_28;
        return look;
    }
    switch (s.clog_state) {
        case core::ClogState::clogged:
            look.kind = ClogLook::clogged;
            look.word = "Tapados";
            look.fg = look.border = colors::alarm;
            look.bg = colors::alarm_bg;
            look.icon = &icon_alarma_28;
            break;
        case core::ClogState::check:
            look.kind = ClogLook::check;
            look.word = "Revisar";
            look.fg = look.border = colors::warning;
            look.bg = colors::warning_bg;
            look.icon = &icon_advertencia_28;
            break;
        case core::ClogState::normal:
        case core::ClogState::none:  // no llega: arriba
            look.kind = ClogLook::normal;
            look.word = "Normal";
            look.fg = look.border = colors::ok;
            look.bg = colors::ok_bg;
            look.icon = &icon_ok_28;
            break;
    }
    return look;
}

}  // namespace ui
