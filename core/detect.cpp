#include "detect.h"

#include "events.h"

namespace core {
namespace {

// Cuenta los segundos seguidos que se ve; con kSeenSeconds lo da por instalado
bool watch(bool& installed, uint8_t& seconds, bool seen) {
    if (installed) return false;
    seconds = seen ? static_cast<uint8_t>(seconds + 1) : 0;
    if (seconds < kSeenSeconds) return false;
    installed = true;
    return true;
}

// Ya instalado: cambia de Sin señal a con señal (o al revés) con kSeenSeconds
// seguidos de lo contrario, y lo anota en el registro
void watch_signal(SystemState& s, Sensor sensor, bool& lost, uint8_t& seconds, bool installed, bool reads) {
    if (!installed) {
        lost = false;
        seconds = 0;
        return;
    }
    seconds = reads == lost ? static_cast<uint8_t>(seconds + 1) : 0;  // lleva la contraria
    if (seconds < kSeenSeconds) return;
    seconds = 0;
    lost = !reads;
    const auto value = static_cast<uint16_t>(sensor);
    if (lost && !has_open_event(s, EventKind::no_signal, value)) add_event(s, EventKind::no_signal, value);
    if (!lost) resolve_event(s, EventKind::no_signal, value);
}

}  // namespace

bool detect_sensors(SystemState& s, Detection& d, const AnalogReadings& r) {
    const bool p1 = r.ads_ok && r.p1_volts > kPressureSeenVolts;
    const bool p2 = r.ads_ok && r.p2_volts > kPressureSeenVolts;
    const bool level = r.ads_ok && r.level_volts > kLevelSeenVolts;
    const bool ldr = r.ads_ok && r.ldr_volts > kLdrSeenVolts;
    bool found = watch(s.has_pressure, d.pressure, p1 && p2);
    found = watch(s.has_level, d.level, level) || found;
    found = watch(s.has_ldr, d.ldr, ldr) || found;
    watch_signal(s, Sensor::p1, s.no_signal.p1, d.p1_flip, s.has_pressure, p1);
    watch_signal(s, Sensor::p2, s.no_signal.p2, d.p2_flip, s.has_pressure, p2);
    watch_signal(s, Sensor::level, s.no_signal.level, d.level_flip, s.has_level, level);
    return found;
}

}  // namespace core
