#pragma once
// La hora de la casa y lo que cambia con ella. El firmware la saca del NTP;
// el simulador, de su reloj de mentira. Las dos llaman set_time() y luego
// tick_minute() (core/monitor.h) cada minuto.

#include <cstdint>

#include "system_state.h"

namespace core {

// Hora local, como la da el NTP ya con la zona de Guadalajara
struct Clock {
    uint16_t year = 2026;
    uint8_t month = 1;  // 1 enero ... 12 diciembre
    uint8_t day = 1;
    uint8_t hour = 0;
    uint8_t minute = 0;
};

// Días desde el 1 de enero de 1970 (algoritmo de Howard Hinnant)
int32_t days_from_civil(int32_t year, int32_t month, int32_t day);

// Pone la hora y cierra lo que haya que cerrar:
// - hora nueva: el historial abre esa hora con las presiones y el ORP de
//   ahorita, y las gráficas de presión de 24 h y de ORP de 7 días reciben
//   una muestra
// - día nuevo: los litros y la bomba R de hoy vuelven a 0, las horas
//   empiezan de 0 y el historial de 30 días se recorre uno por cada día
//   que pasó (si la placa estuvo apagada, los días de en medio quedan en 0)
// La primera vez (today en 0) solo pone la hora y arranca el conteo de días;
// si el reloj va para atrás, tampoco cierra nada.
void set_time(SystemState& s, const Clock& now);

// Cada minuto: muestras de caudal (2 h cada minuto, 24 h cada 5) y los
// promedios de presión y ORP de la hora y del día
void sample_minute(SystemState& s);

}  // namespace core
