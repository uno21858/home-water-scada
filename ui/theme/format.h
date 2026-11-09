#pragma once
// Formato numérico del DS (México): punto decimal y coma de miles.

#include <cstddef>
#include <cstdint>

namespace ui::fmt {

// 1860 -> "1,860"
void thousands(char* out, size_t size, uint32_t value);

// Entero con el signo menos del DS (U+2212): -50 -> "−50"
void signed_int(char* out, size_t size, int value);

// Un decimal. Abajo de 10 lleva un espacio de cifra al inicio para que
// el número mida lo mismo y la unidad no brinque: 12.4 -> "12.4", 9.8 -> " 9.8"
void one_decimal(char* out, size_t size, float value);

// Minuto del día -> "HH:MM"
void clock(char* out, size_t size, unsigned minute_of_day);

// Minutos que faltan, redondeados a lo que se les puede creer: de 5 en 5
// abajo de una hora, de 10 en 10 hasta 10 horas y de hora en hora después.
// `shown` es lo que ya se ve (0 = nada): se queda mientras el estimado no se
// aleje 3/4 de paso, para que en la orilla de un redondeo no brinque.
uint32_t rough_minutes(uint32_t minutes, uint32_t shown);

// Lo que da rough_minutes -> "45 min", "2 h 30 min", "11 h"; de 2 días para
// arriba, "más de 2 días"
void rough_duration(char* out, size_t size, uint32_t minutes);

// 0 domingo ... 6 sábado -> "Dom" ... "Sáb"
const char* weekday_short(unsigned weekday);

// 1 enero ... 12 diciembre -> "ene" ... "dic"
const char* month_short(unsigned month);

// core::Filter -> "Prefiltro", "20 µm", "10 µm", "5 µm", "CTO", "Sarro"
const char* filter_name(unsigned filter);

// core::AlarmKind -> "Sin flujo mientras dosifica", "Sensor F sin señal"...
const char* alarm_title(unsigned kind);
// Lo mismo, corto para las listas angostas: "Sin flujo al dosificar"
const char* alarm_short(unsigned kind);

// core::Filter -> cada cuánto se purga o se cambia: "cada 7 días", "2 a 3 meses"
const char* filter_every(unsigned filter);

// core::MixerFault -> la palabra del estado: "Sin 12 V", "Nivel bajo", "Atorada"
const char* mixer_fault(unsigned fault);
// Y qué pasa con la mezcla, para el renglón de abajo (caben 456 px en Resumen)
const char* mixer_fault_hint(unsigned fault);

// core::Sensor -> "P1", "P2", "Nivel": el que se quedó sin señal
const char* sensor_name(unsigned sensor);

}  // namespace ui::fmt
