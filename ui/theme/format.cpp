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

void signed_int(char* out, size_t size, int value) {
    // U+2212 en UTF-8; el guion normal es más corto y se ve como guion
    const char* minus = value < 0 ? "\xE2\x88\x92" : "";
    const long magnitude = value < 0 ? -static_cast<long>(value) : value;
    snprintf(out, size, "%s%ld", minus, magnitude);
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

const char* weekday_short(unsigned weekday) {
    static const char* const kNames[7] = {"Dom", "Lun", "Mar", "Mié", "Jue", "Vie", "Sáb"};
    return kNames[weekday % 7];
}

const char* month_short(unsigned month) {
    static const char* const kNames[12] = {"ene", "feb", "mar", "abr", "may", "jun",
                                           "jul", "ago", "sep", "oct", "nov", "dic"};
    return (month >= 1 && month <= 12) ? kNames[month - 1] : "";
}

const char* alarm_title(unsigned kind) {
    static const char* const kTitles[3] = {"Sin flujo mientras dosifica", "Sensor F sin señal",
                                           "Tope diario alcanzado"};
    return kind < 3 ? kTitles[kind] : "";
}

const char* alarm_short(unsigned kind) {
    static const char* const kTitles[3] = {"Sin flujo al dosificar", "Sensor F sin señal", "Tope diario alcanzado"};
    return kind < 3 ? kTitles[kind] : "";
}

const char* filter_name(unsigned filter) {
    static const char* const kNames[6] = {"Prefiltro", "20 µm", "10 µm", "5 µm", "CTO", "Sarro"};
    return filter < 6 ? kNames[filter] : "";
}

}  // namespace ui::fmt
