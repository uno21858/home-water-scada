#pragma once
// Qué tan tapados van los cartuchos: R = (P1 − P2) / F, en psi por L/min.
//
// El tren descarga a la cisterna abierta: con flujo P2 queda en 0 a 1 psi y
// la caída es casi todo P1, que sube y baja con lo que da el SIAPA (sin
// flujo, P1 = P2). Por eso la caída sola no dice nada: 22 psi a 5.6 L/min
// pueden ser cartuchos nuevos. Dividida entre el caudal sale la resistencia
// del tren, que no depende del SIAPA: con cartuchos nuevos es R0 y sube
// conforme se tapan (Erick, 26 oct 2026).
//
// - Se evalúa con flujo de 1 L/min o más durante 60 s seguidos y estable en
//   los últimos 30 (sin moverse más de 15 %): R es el promedio de esos 60 s.
//   Después junta otros 60: una evaluación por minuto de agua.
// - R0 se guarda a mano con cartuchos nuevos ("Cartuchos nuevos: guardar
//   referencia" en Ajustes > Cartuchos), con su fecha y su caudal.
// - Contra R0: menos de 150 % Normal, de 150 a 200 Revisar, más de 200
//   Tapados. Para salir de un estado hay que bajar 10 puntos de su umbral:
//   en la orilla no brinca cada minuto. Sin R0, "Sin referencia" y sin avisos.
// - Sin flujo se queda el último estado, con su hora; nunca "Normal" por una
//   caída de 0.
// - Cada evaluación entra al promedio del día (clog_days): la tendencia.

#include <array>
#include <cstdint>

#include "system_state.h"

namespace core {

inline constexpr float kClogMinLpm = 1.0f;          // abajo de esto, "Sin flujo"
inline constexpr int kClogWindowSeconds = 60;       // R promedio de 60 s
inline constexpr int kClogStableSeconds = 30;       // los últimos 30, estables
inline constexpr float kClogStableSpread = 0.15f;   // sin moverse más de 15 %
inline constexpr float kClogWarningRatio = 1.5f;    // Revisar desde 150 %
inline constexpr float kClogAlarmRatio = 2.0f;      // Tapados arriba de 200 %
inline constexpr float kClogHysteresis = 0.1f;      // para salir, 10 puntos menos
inline constexpr uint16_t kClogFreshSeconds = 90;   // la referencia se toma de una evaluación así de reciente

// Los últimos segundos de caudal y caída; vive en el firmware (y en el demo),
// como core::Detection
struct ClogWindow {
    std::array<float, kClogWindowSeconds> lpm{};
    std::array<float, kClogWindowSeconds> psi{};
    int count = 0;
};

// Una vez por segundo, después de set_flow y set_pressures. Regresa true si
// evaluó (hay algo nuevo que guardar).
bool clog_tick(SystemState& s, ClogWindow& w);

// R entre R0 del último evaluado; 0 sin referencia o sin evaluar
float clog_ratio(const SystemState& s);

// ¿Hay una evaluación de hace menos de kClogFreshSeconds?
bool clog_fresh(const SystemState& s);

// "Cartuchos nuevos: guardar referencia": R0 es el último R, con su caudal y
// la fecha de hoy. Sin una evaluación reciente (no hay flujo estable) no
// guarda nada y regresa false.
bool clog_save_reference(SystemState& s);

}  // namespace core
