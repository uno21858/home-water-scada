#pragma once
// Lo que se manda al servidor de la casa: VictoriaMetrics en el NAS, por HTTP
// con el formato de línea de Influx (un POST de texto a /write). Una lectura
// cada 10 s y cada evento nuevo. Sin red se quedan en la cola (Outbox) y se
// mandan después, los más viejos primero; "Por enviar" es lo que hay en ella.
//
// En la 7B, cada 10 s:
//   core::queue_reading(s, outbox, time(nullptr));
//   std::size_t records = 0;
//   const std::size_t bytes = core::format_batch(outbox, body, sizeof body, &records);
//   if (bytes > 0 && post(url, body, bytes) == 204) core::mark_sent(s, outbox, records);
// Al arrancar: outbox.last_event_seq = s.event_seq, para no mandar otra vez
// los eventos que ya estaban guardados.

#include <cstddef>
#include <cstdint>

#include "system_state.h"

namespace core {

// Una lectura o un evento, como espera en la cola
struct Record {
    enum class Type : uint8_t { reading, event };
    Type type = Type::reading;
    uint32_t time = 0;  // segundos desde 1970 en UTC (del NTP)

    // Lectura. Lo que no está instalado no se manda.
    bool pressure = false;  // P1 y P2 instalados
    bool level = false;     // sensor de nivel
    bool mixer = false;     // bomba R
    bool orp = false;       // sonda ORP
    float flow_lpm = 0.0f;
    uint32_t liters_today = 0;
    int16_t p1_psi = 0;
    int16_t p2_psi = 0;
    float clog_r = 0.0f;     // R del último minuto con flujo estable (core/clog.h); 0 = no se manda
    uint16_t clog_pct = 0;   // R/R0 en %; 0 = sin referencia
    float level_cm = 0.0f;
    int8_t level_pct = -1;  // -1 = falta marcar llena: no se manda
    bool mixer_on = false;
    float mixer_amps = 0.0f;
    int16_t orp_mv = 0;

    // Evento
    EventKind event = EventKind::dose;
    uint16_t value = 0;
};

// Cola de anillo sobre la memoria del que llama: en la 7B va en la PSRAM
// (un día sin red son 8,640 lecturas de 48 bytes: 405 KB), en la compu donde sea.
// Si se llena, se cae lo más viejo y se cuenta en dropped().
class Outbox {
public:
    Outbox(Record* buffer, std::size_t capacity) : buffer_(buffer), capacity_(capacity) {}

    void push(const Record& r);
    std::size_t size() const { return count_; }
    const Record& at(std::size_t i) const { return buffer_[(head_ + i) % capacity_]; }  // 0 = el más viejo
    void pop(std::size_t n);  // ya se mandaron los n más viejos
    uint32_t dropped() const { return dropped_; }

    uint32_t last_event_seq = 0;  // el último evento que ya entró a la cola

private:
    Record* buffer_;
    std::size_t capacity_;
    std::size_t head_ = 0;  // el más viejo
    std::size_t count_ = 0;
    uint32_t dropped_ = 0;
};

// Pone en la cola la lectura de ahorita y los eventos que la cola no ha visto
void queue_reading(SystemState& s, Outbox& out, uint32_t utc);

// Una línea del formato de Influx, sin el salto de línea:
//   water flow_lpm=12.40,liters_today=1860i,p1_psi=42i ... 1760000000000000000
//   water_event,kind=dose value=185i 1760000000000000000
// Regresa los bytes escritos, o 0 si no cupo en size.
std::size_t format_record(const Record& r, char* out, std::size_t size);

// Los más viejos de la cola que quepan en body, uno por renglón. Regresa los
// bytes y en records cuántos entraron; 0 si la cola está vacía.
std::size_t format_batch(const Outbox& out, char* body, std::size_t size, std::size_t* records);

// El servidor contestó que sí: salen de la cola y se anota la hora del envío
void mark_sent(SystemState& s, Outbox& out, std::size_t records);

}  // namespace core
