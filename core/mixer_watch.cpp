#include "mixer_watch.h"

namespace core {
namespace {

// Lleva la cuenta de un lado del umbral: true mientras siga de ese lado
void track(bool beyond, bool& on, uint32_t& since_ms, uint32_t now_ms) {
    if (!beyond) {
        on = false;
    } else if (!on) {
        on = true;
        since_ms = now_ms;
    }
}

}  // namespace

void mixer_watch_start(MixerWatch& w, uint32_t now_ms) {
    w = MixerWatch{};
    w.started_ms = now_ms;
}

MixerFault mixer_watch_sample(MixerWatch& w, const MixerLimits& limits, uint32_t now_ms, bool ok, float volts,
                              float amps) {
    // Con un hueco largo, lo de antes ya no es "seguido": esta lectura empieza otra cuenta.
    // Las restas sin signo aguantan que el reloj de milisegundos dé la vuelta.
    if (w.has_last && now_ms - w.last_ms > kMixerMaxGapMs) w.high = w.low = false;
    w.last_ms = now_ms;
    w.has_last = true;

    const bool blind = now_ms - w.started_ms < kMixerBlindMs;
    if (blind || !ok || volts < limits.min_volts) {
        w.high = w.low = false;
        return MixerFault::none;
    }
    track(amps > limits.stuck_amps, w.high, w.high_since_ms, now_ms);
    track(amps < limits.dry_amps, w.low, w.low_since_ms, now_ms);
    if (w.high && now_ms - w.high_since_ms >= kMixerSustainMs) return MixerFault::stuck;
    if (w.low && now_ms - w.low_since_ms >= kMixerSustainMs) return MixerFault::dry;
    return MixerFault::none;
}

}  // namespace core
