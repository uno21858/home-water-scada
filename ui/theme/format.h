#pragma once
// Formato numérico del DS (México): punto decimal y coma de miles.

#include <cstddef>
#include <cstdint>

namespace ui::fmt {

// 1860 -> "1,860"
void thousands(char* out, size_t size, uint32_t value);

// Un decimal. Abajo de 10 lleva un espacio de cifra al inicio para que
// el número mida lo mismo y la unidad no brinque: 12.4 -> "12.4", 9.8 -> " 9.8"
void one_decimal(char* out, size_t size, float value);

// Minuto del día -> "HH:MM"
void clock(char* out, size_t size, unsigned minute_of_day);

}  // namespace ui::fmt
