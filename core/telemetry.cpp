#include "telemetry.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "level.h"

namespace core {
namespace {

// El nombre que lleva cada evento en el servidor (etiqueta kind)
const char* event_name(EventKind kind) {
    switch (kind) {
        case EventKind::dose: return "dose";
        case EventKind::dose_typed: return "dose_typed";
        case EventKind::cartridge: return "cartridge";
        case EventKind::cartridge_added: return "cartridge_added";
        case EventKind::purge: return "purge";
        case EventKind::alarm: return "alarm";
        case EventKind::drop_warning: return "drop_warning";
        case EventKind::drop_alarm: return "drop_alarm";
        case EventKind::offline: return "offline";
        case EventKind::mixer_fault: return "mixer_fault";
        case EventKind::no_supply: return "no_supply";
    }
    return "unknown";
}

// Un número que no es número (sensor raro) se manda como 0, no como "nan":
// el servidor rechazaría todo el lote
float finite(float v) { return std::isfinite(v) ? v : 0.0f; }

// Agrega a una línea sin pasarse; regresa false si ya no cupo
struct Line {
    char* out;
    std::size_t size;
    std::size_t used = 0;
    bool ok = true;

    template <typename... Args>
    void add(const char* format, Args... args) {
        if (!ok) return;
        const int n = std::snprintf(out + used, size - used, format, args...);
        if (n < 0 || static_cast<std::size_t>(n) >= size - used) {
            ok = false;
            return;
        }
        used += static_cast<std::size_t>(n);
    }
};

}  // namespace

void Outbox::push(const Record& r) {
    if (capacity_ == 0) return;
    if (count_ == capacity_) {  // llena: se cae el más viejo
        head_ = (head_ + 1) % capacity_;
        --count_;
        ++dropped_;
    }
    buffer_[(head_ + count_) % capacity_] = r;
    ++count_;
}

void Outbox::pop(std::size_t n) {
    if (n > count_) n = count_;
    head_ = capacity_ == 0 ? 0 : (head_ + n) % capacity_;
    count_ -= n;
}

void queue_reading(SystemState& s, Outbox& out, uint32_t utc) {
    Record r;
    r.type = Record::Type::reading;
    r.time = utc;
    r.flow_lpm = s.flow_lpm;
    r.liters_today = s.liters_today;
    r.pressure = s.has_pressure;
    r.p1_psi = static_cast<int16_t>(s.p1_psi);
    r.p2_psi = static_cast<int16_t>(s.p2_psi);
    r.drop_rise = s.has_pressure ? drop_rise(s) : 0.0f;
    r.level = s.has_level;
    if (s.has_level) {
        r.level_cm = level_cm(s);
        r.level_pct = static_cast<int8_t>(level_pct(s));
    }
    r.mixer = s.has_mixer;
    r.mixer_on = s.mixer_on;
    r.mixer_amps = s.mixer_amps;
    r.orp = s.has_orp;
    r.orp_mv = static_cast<int16_t>(s.orp_mv);
    out.push(r);

    // Eventos que la cola no ha visto, del más viejo al más nuevo
    for (int i = s.recent_event_count - 1; i >= 0; --i) {
        const Event& e = s.recent_events[static_cast<std::size_t>(i)];
        if (e.seq <= out.last_event_seq) continue;
        Record ev;
        ev.type = Record::Type::event;
        ev.time = utc;
        ev.event = e.kind;
        ev.value = e.value;
        out.push(ev);
        out.last_event_seq = e.seq;
    }
    s.pending_uploads = static_cast<uint32_t>(out.size());
}

std::size_t format_record(const Record& r, char* out, std::size_t size) {
    if (size == 0) return 0;
    Line line{out, size};
    const unsigned long long ns = static_cast<unsigned long long>(r.time) * 1000000000ULL;
    if (r.type == Record::Type::event) {
        line.add("water_event,kind=%s value=%ui %llu", event_name(r.event), static_cast<unsigned>(r.value), ns);
    } else {
        line.add("water flow_lpm=%.2f,liters_today=%lui", static_cast<double>(finite(r.flow_lpm)),
                 static_cast<unsigned long>(r.liters_today));
        if (r.pressure) {
            line.add(",p1_psi=%di,p2_psi=%di,drop_rise=%.1f", r.p1_psi, r.p2_psi,
                     static_cast<double>(finite(r.drop_rise)));
        }
        if (r.level) {
            line.add(",level_cm=%.1f", static_cast<double>(finite(r.level_cm)));
            if (r.level_pct >= 0) line.add(",level_pct=%di", r.level_pct);
        }
        if (r.mixer) {
            line.add(",mixer_on=%di,mixer_amps=%.2f", r.mixer_on ? 1 : 0, static_cast<double>(finite(r.mixer_amps)));
        }
        if (r.orp) line.add(",orp_mv=%di", r.orp_mv);
        line.add(" %llu", ns);
    }
    if (!line.ok) {
        out[0] = '\0';
        return 0;
    }
    return line.used;
}

std::size_t format_batch(const Outbox& out, char* body, std::size_t size, std::size_t* records) {
    std::size_t used = 0;
    std::size_t n = 0;
    char line[256];
    while (n < out.size()) {
        const std::size_t len = format_record(out.at(n), line, sizeof line);
        if (len == 0 || used + len + 1 >= size) break;  // + el salto de línea y el '\0'
        std::memcpy(body + used, line, len);
        used += len;
        body[used++] = '\n';
        ++n;
    }
    if (size > 0) body[used < size ? used : size - 1] = '\0';
    if (records != nullptr) *records = n;
    return used;
}

void mark_sent(SystemState& s, Outbox& out, std::size_t records) {
    out.pop(records);
    s.last_upload_at = minute_of_day(s);
    s.pending_uploads = static_cast<uint32_t>(out.size());
}

}  // namespace core
