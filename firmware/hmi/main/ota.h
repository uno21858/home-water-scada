#pragma once
// Actualización por WiFi (documento, pág. 41: dos particiones y rollback),
// de dos formas:
// - La compu empuja: subir-ota.sh manda el .bin a POST /ota (puerto 80) y
//   pregunta la versión en GET /version. La placa contesta al momento.
// - La placa jala: cada CONFIG_HMI_OTA_CHECK_MINUTES lee el encabezado del .bin
//   de CONFIG_HMI_OTA_URL (si hay) y lo baja (servir-ota.sh, o el NAS).
// Las dos instalan solo si es otro binario (se compara el hash, no la
// versión), del mismo proyecto y no es uno que ya falló. ESP-IDF revisa que
// venga firmado con la llave de la casa; si no, no lo instala. Con la bomba R
// andando espera a que acabe para reiniciar.
//
// La versión nueva arranca "a prueba": si en kHealthTimeoutSeconds no se
// conecta a la red (sin red ya no llegaría otra OTA), regresa a la anterior.

#include <cstddef>
#include <cstdint>

#include "system_state.h"

namespace ota {

inline constexpr uint32_t kHealthySeconds = 60;         // tiene que aguantar al menos esto
inline constexpr uint32_t kHealthTimeoutSeconds = 300;  // y conectarse antes de esto

// can_install: ¿se puede reiniciar ahorita? restart: guarda todo y reinicia.
// status: escribe el checklist de GET /estado (lo que imprime subir-ota.sh
// después de actualizar) y regresa cuántos bytes. Se llaman desde la tarea de
// la OTA y desde el servidor de subidas.
void start(bool (*can_install)(), void (*restart)(), std::size_t (*status)(char* buf, std::size_t size));

// ¿Esta versión acaba de llegar y está a prueba? Se puede preguntar antes de start().
bool on_trial();

// Lo que va de una actualización, para la pantalla (la app lo copia a
// SystemState::update cada segundo). Un fallo se ve 15 s y se quita.
core::UpdateStatus progress();

enum class Health : uint8_t { fine, testing, failed };

// Cada segundo. Si esta versión está a prueba: fine cuando ya lleva
// kHealthySeconds con red (queda como buena), failed si se acabó el tiempo
// sin red (la app guarda y llama rollback()).
Health check_health(uint32_t uptime_seconds, bool online);
void rollback();

}  // namespace ota
