#include "mixing.h"

#include <algorithm>

#include "events.h"
#include "level.h"

namespace core {
namespace {

// Algo no deja correr la bomba ahorita: un aviso del INA219 o la cisterna en 0 %
bool blocked(const SystemState& s) { return s.mixer_fault != MixerFault::none || mixer_low_water(s); }

// La vuelta que iba o que no pudo correr: se corre completa cuando se pueda
void pend(SystemState& s, MixerReason reason, uint8_t minutes) {
    if (minutes >= s.mixer_pending_min) {
        s.mixer_pending_min = minutes;
        s.mixer_pending_reason = reason;
    }
}

// Arranca una vuelta. La de ahorita va primero en la lista de las últimas.
void start(SystemState& s, MixerReason reason, uint8_t minutes) {
    if (minutes == 0) return;
    std::rotate(s.mixer_runs.rbegin(), s.mixer_runs.rbegin() + 1, s.mixer_runs.rend());
    s.mixer_runs[0] = {minute_of_day(s), 0, reason};
    s.mixer_run_count = static_cast<uint8_t>(std::min<int>(s.mixer_run_count + 1, static_cast<int>(s.mixer_runs.size())));
    ++s.mixer_today_runs;
    s.mixer_on = true;
    s.mixer_started_at = minute_of_day(s);
    s.mixer_minutes_left = minutes;
    s.mixer_run_minutes = minutes;
    s.mixer_progress_pct = 0;
    s.liters_since_mix = 0;  // esta vuelta también revuelve el agua que ya entró
    s.mixer_pending_min = 0;
}

void stop(SystemState& s) {
    s.mixer_on = false;
    s.mixer_minutes_left = 0;
}

// Pide una vuelta. Con un aviso queda pendiente (gana la más larga). Si ya
// está corriendo, solo la cambia si la nueva dura más que lo que le falta.
void request(SystemState& s, MixerReason reason, uint8_t minutes) {
    if (blocked(s)) {
        pend(s, reason, minutes);
        return;
    }
    if (!s.mixer_on || minutes > s.mixer_minutes_left) start(s, reason, minutes);
}

// Lo que toca correr en cuanto se pueda: lo pendiente o la vuelta por agua
void run_due(SystemState& s) {
    if (s.mixer_on || s.mixer_mode == MixerMode::off || blocked(s)) return;
    if (s.mixer_pending_min > 0) {
        start(s, s.mixer_pending_reason, s.mixer_pending_min);
    } else if (s.mixer_mode == MixerMode::automatic && s.liters_since_mix >= kMixEveryLiters) {
        start(s, MixerReason::water, s.mixer_every_liters_min);
    }
}

void clear_fault(SystemState& s) {
    if (s.mixer_fault == MixerFault::none) return;
    s.mixer_fault = MixerFault::none;
    resolve_event(s, EventKind::mixer_fault);
}

}  // namespace

void mixer_set_mode(SystemState& s, MixerMode mode) {
    if (!s.has_mixer || mode == s.mixer_mode) return;
    const bool by_hand = s.mixer_on && s.mixer_run_count > 0 && s.mixer_runs[0].reason == MixerReason::manual;
    s.mixer_mode = mode;
    // Sin 12 V no se quita aquí: eso lo dice el INA219 cuando regresan
    if (s.mixer_fault == MixerFault::stuck || s.mixer_fault == MixerFault::dry) clear_fault(s);

    switch (mode) {
        case MixerMode::on:
            request(s, MixerReason::manual, s.mixer_manual_max_min);
            break;
        case MixerMode::off:
            stop(s);
            s.mixer_pending_min = 0;
            break;
        case MixerMode::automatic:
            if (by_hand) stop(s);  // de a mano a Auto: espera lo que siga del programa
            run_due(s);
            break;
    }
}

void mixer_after_dose(SystemState& s) {
    if (!s.has_mixer || s.mixer_mode == MixerMode::off) return;
    request(s, MixerReason::dose, s.mixer_after_dose_min);
}

void mixer_add_liters(SystemState& s, uint32_t liters) {
    if (!s.has_mixer) return;
    s.liters_since_mix = static_cast<uint16_t>(std::min<uint32_t>(s.liters_since_mix + liters, 0xFFFF));
    // Nivel bajo: vuelve a intentar cuando ya entraron kMixEveryLiters desde
    // la vuelta que no pudo (el documento: hasta que entre agua nueva)
    if (s.mixer_fault == MixerFault::dry && s.liters_since_mix >= kMixEveryLiters) clear_fault(s);
    run_due(s);
}

bool mixer_low_water(const SystemState& s) { return !s.mixer_bench && level_ok(s) && level_pct(s) == 0; }

void mixer_set_bench(SystemState& s, bool on) {
    if (!s.has_mixer || on == s.mixer_bench) return;
    s.mixer_bench = on;
    // En banco el nivel bajo no cuenta: el aviso que hubiera estorba la prueba
    if (on && s.mixer_fault == MixerFault::dry) clear_fault(s);
}

void mixer_tick_minute(SystemState& s) {
    if (!s.has_mixer) return;
    // La cisterna bajó al mínimo con la bomba andando: se apaga y la vuelta
    // queda pendiente para cuando regrese el agua
    if (s.mixer_on && mixer_low_water(s) && s.mixer_run_count > 0) {
        pend(s, s.mixer_runs[0].reason, s.mixer_run_minutes);
        stop(s);
    }
    if (s.mixer_on && s.mixer_minutes_left > 0) {
        --s.mixer_minutes_left;
        ++s.mixer_today_minutes;
        ++s.mixer_runs[0].minutes;
        const unsigned done = s.mixer_run_minutes - s.mixer_minutes_left;
        s.mixer_progress_pct = static_cast<uint8_t>(done * 100 / std::max<unsigned>(1, s.mixer_run_minutes));
        if (s.mixer_minutes_left == 0) {
            s.mixer_on = false;
            if (s.mixer_mode == MixerMode::on) s.mixer_mode = MixerMode::off;  // a mano: una sola vuelta
        }
    }
    run_due(s);
}

void mixer_report_fault(SystemState& s, MixerFault fault) {
    if (fault == MixerFault::none || fault == s.mixer_fault) return;
    if (fault == MixerFault::dry && s.mixer_bench) return;  // en banco el nivel no se cuida
    clear_fault(s);  // si ya había otro aviso, ese se cierra y queda este
    // La vuelta que se cortó queda pendiente; atorada no, esa la revisa alguien
    if (s.mixer_on && fault != MixerFault::stuck && s.mixer_run_count > 0) pend(s, s.mixer_runs[0].reason, s.mixer_run_minutes);
    stop(s);
    if (fault == MixerFault::stuck) s.mixer_pending_min = 0;
    if (s.mixer_mode == MixerMode::on && s.mixer_pending_min == 0) s.mixer_mode = MixerMode::off;
    s.mixer_fault = fault;
    add_event(s, EventKind::mixer_fault, static_cast<uint16_t>(fault));
}

void mixer_power_back(SystemState& s) {
    if (s.mixer_fault != MixerFault::no_power) return;
    clear_fault(s);
    run_due(s);
}

void mixer_after_restart(SystemState& s) {
    if (!s.mixer_on) return;
    if (s.mixer_run_count > 0) pend(s, s.mixer_runs[0].reason, s.mixer_minutes_left);
    stop(s);
}

}  // namespace core
