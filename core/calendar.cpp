#include "calendar.h"

#include <algorithm>
#include <cmath>

namespace core {
namespace {

// Recorre una gráfica un lugar y mete la muestra nueva al final
template <typename T, std::size_t N, typename V>
void push(std::array<T, N>& samples, V value) {
    std::rotate(samples.begin(), samples.begin() + 1, samples.end());
    samples.back() = static_cast<T>(value);
}

uint8_t psi8(int psi) { return static_cast<uint8_t>(std::clamp(psi, 0, 255)); }

// Abre la hora de ahorita en el historial y las gráficas por hora
void open_hour(SystemState& s) {
    s.hours[s.hour % s.hours.size()] = {0, 0, psi8(s.p1_psi), psi8(s.p2_psi), static_cast<int16_t>(s.orp_mv)};
    push(s.p1_24h, psi8(s.p1_psi));
    push(s.p2_24h, psi8(s.p2_psi));
    push(s.orp_7d, s.orp_mv);
}

// Cierra el día: el historial se recorre y lo de hoy empieza en 0
void open_day(SystemState& s) {
    push(s.days, Totals{});
    s.hours = {};
    s.liters_today = 0;
    s.mixer_today_minutes = 0;
    s.mixer_today_runs = 0;
}

}  // namespace

int32_t days_from_civil(int32_t year, int32_t month, int32_t day) {
    year -= month <= 2 ? 1 : 0;
    const int32_t era = (year >= 0 ? year : year - 399) / 400;
    const int32_t yoe = year - era * 400;                                    // año de la era
    const int32_t doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;  // día del año desde marzo
    const int32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;               // día de la era
    return era * 146097 + doe - 719468;
}

void set_time(SystemState& s, const Clock& now) {
    const int32_t before = days_from_civil(s.year, s.month, s.day);
    const int32_t after = days_from_civil(now.year, now.month, now.day);
    const int32_t hours_before = before * 24 + s.hour;
    const int32_t hours_after = after * 24 + now.hour;

    s.year = now.year;
    s.month = now.month;
    s.day = now.day;
    s.weekday = static_cast<uint8_t>((after % 7 + 7 + 4) % 7);  // el 1 ene 1970 fue jueves (4)
    s.hour = now.hour;
    s.minute = now.minute;
    if (s.today == 0) {  // la primera vez: el conteo de días arranca aquí
        s.today = static_cast<uint32_t>(std::max<int32_t>(1, after));
        return;
    }
    if (hours_after <= hours_before) return;  // misma hora, o el reloj se fue para atrás

    // Día nuevo: uno por cada día que pasó, hasta 30
    const int32_t days = std::min<int32_t>(after - before, kHistoryDays);
    for (int32_t i = 0; i < days; ++i) open_day(s);
    s.today += static_cast<uint32_t>(std::max<int32_t>(0, after - before));
    open_hour(s);
}

void sample_minute(SystemState& s) {
    push(s.flow_2h, s.flow_lpm);
    if (s.minute % 5 == 0) push(s.flow_24h, s.flow_lpm);

    // Promedio de la hora con las muestras que lleva (una por minuto)
    Totals& hour = s.hours[s.hour % s.hours.size()];
    const int n = s.minute;
    auto mean = [n](int avg, int value) { return static_cast<int>(std::lround((avg * n + value) / (n + 1.0))); };
    hour.p1_psi = psi8(mean(hour.p1_psi, s.p1_psi));
    hour.p2_psi = psi8(mean(hour.p2_psi, s.p2_psi));
    hour.orp_mv = static_cast<int16_t>(mean(hour.orp_mv, s.orp_mv));

    // El día: promedio de sus horas hasta ahorita
    int p1 = 0;
    int p2 = 0;
    int orp = 0;
    const int hours = s.hour + 1;
    for (int h = 0; h < hours; ++h) {
        p1 += s.hours[static_cast<std::size_t>(h)].p1_psi;
        p2 += s.hours[static_cast<std::size_t>(h)].p2_psi;
        orp += s.hours[static_cast<std::size_t>(h)].orp_mv;
    }
    Totals& day = s.days.back();
    day.p1_psi = psi8(static_cast<int>(std::lround(p1 / static_cast<double>(hours))));
    day.p2_psi = psi8(static_cast<int>(std::lround(p2 / static_cast<double>(hours))));
    day.orp_mv = static_cast<int16_t>(std::lround(orp / static_cast<double>(hours)));
}

}  // namespace core
