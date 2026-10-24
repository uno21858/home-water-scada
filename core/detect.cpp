#include "detect.h"

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

}  // namespace

bool detect_sensors(SystemState& s, Detection& d, const AnalogReadings& r) {
    const bool pressure = r.ads_ok && r.p1_volts > kPressureSeenVolts && r.p2_volts > kPressureSeenVolts;
    const bool level = r.ads_ok && r.level_volts > kLevelSeenVolts;
    const bool ldr = r.ads_ok && r.ldr_volts > kLdrSeenVolts;
    bool found = watch(s.has_pressure, d.pressure, pressure);
    found = watch(s.has_level, d.level, level) || found;
    found = watch(s.has_ldr, d.ldr, ldr) || found;
    return found;
}

}  // namespace core
