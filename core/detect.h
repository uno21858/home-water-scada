#pragma once
// Los sensores del ADS1115 se descubren solos: lo que se conecta aparece sin
// tocar menuconfig. Cada uno da una lectura que sin él no sale (sin nada, la
// entrada queda en 0 V por su resistencia a tierra):
// - P1 y P2: 0.5 V en 0 psi (documento, pág. 39); los dos arriba de 0.3 V
// - nivel: 4 mA en la de 33 Ω son 0.132 V; arriba de 0.10 V (3 mA)
// - LDR: 0.88 V con luz de cuarto; arriba de 0.15 V (de día lo agarra)
// Con 5 s seguidos queda instalado y el firmware lo guarda (persist): un
// sensor que ya se vio y luego no contesta sigue instalado, no desaparece.
// La bomba R no: su pin choca con la consola de la mesa y va en menuconfig.
//
// Ya instalado, si P1, P2 o el nivel bajan de esa lectura (o el ADS1115 no
// contesta) 5 s seguidos, quedan en Sin señal (SystemState::no_signal) y se
// anota el evento; con 5 s seguidos de lectura buena regresan y el evento se
// cierra. Los 5 s para los dos lados: un falso contacto no parpadea. El LDR
// no: sin luz también lee 0 V.

#include <cstdint>

#include "system_state.h"

namespace core {

inline constexpr float kPressureSeenVolts = 0.3f;
inline constexpr float kLevelSeenVolts = 0.10f;
inline constexpr float kLdrSeenVolts = 0.15f;
inline constexpr uint8_t kSeenSeconds = 5;

// Lo que lee el ADS1115 en un segundo, en volts a la salida de cada sensor
struct AnalogReadings {
    bool ads_ok = false;  // el ADS1115 contestó
    float p1_volts = 0.0f;
    float p2_volts = 0.0f;
    float level_volts = 0.0f;
    float ldr_volts = 0.0f;
};

// Segundos seguidos que cada uno lleva viéndose (o, ya instalado, llevando
// la contraria a lo que dice no_signal); vive en el firmware
struct Detection {
    uint8_t pressure = 0;
    uint8_t level = 0;
    uint8_t ldr = 0;
    uint8_t p1_flip = 0;
    uint8_t p2_flip = 0;
    uint8_t level_flip = 0;
};

// Una vez por segundo. Regresa true si algo quedó instalado (hay que guardar
// los ajustes). Sin señal no se guarda: al reiniciar se vuelve a ver.
bool detect_sensors(SystemState& s, Detection& d, const AnalogReadings& r);

}  // namespace core
