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

// 0 domingo ... 6 sábado -> "Dom" ... "Sáb"
const char* weekday_short(unsigned weekday);

// 1 enero ... 12 diciembre -> "ene" ... "dic"
const char* month_short(unsigned month);

// core::Filter -> "Prefiltro", "20 µm", "10 µm", "5 µm", "CTO", "Sarro"
const char* filter_name(unsigned filter);

// core::AlarmKind -> "Sin flujo mientras dosifica", "Sensor F sin señal"...
const char* alarm_title(unsigned kind);

}  // namespace ui::fmt
