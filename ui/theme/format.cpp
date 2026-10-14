#include "theme/format.h"

#include <cmath>
#include <cstdio>

namespace ui::fmt {

void thousands(char* out, size_t size, uint32_t value) {
    if (size == 0) return;
    char digits[12];
    const int n = snprintf(digits, sizeof digits, "%lu", static_cast<unsigned long>(value));
    size_t j = 0;
    for (int i = 0; i < n; ++i) {
        const bool comma = i > 0 && (n - i) % 3 == 0;
        if (j + (comma ? 2 : 1) >= size) break;
        if (comma) out[j++] = ',';
        out[j++] = digits[i];
    }
    out[j] = '\0';
}

void one_decimal(char* out, size_t size, float value) {
    // Se decide con el valor ya redondeado: 9.96 sale "10.0" y no lleva espacio
    const float rounded = std::round(value * 10.0f) / 10.0f;
    // U+2007 (espacio de cifra) en UTF-8: mide lo mismo que un dígito
    const char* pad = rounded < 10.0f ? "\xE2\x80\x87" : "";
    snprintf(out, size, "%s%.1f", pad, static_cast<double>(rounded));
}

void clock(char* out, size_t size, unsigned minute_of_day) {
    snprintf(out, size, "%02u:%02u", (minute_of_day / 60) % 24, minute_of_day % 60);
}

}  // namespace ui::fmt
