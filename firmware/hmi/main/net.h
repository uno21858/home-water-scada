#pragma once
// WiFi de la casa, la hora del NTP y los envíos por HTTP. Todo se reintenta
// solo: sin red la placa sigue midiendo y mezclando (documento, pág. 44: la
// red nunca es condición para la seguridad).

#include <cstddef>
#include <cstdint>

namespace net {

// Se conecta a la red y pide la hora. Regresa enseguida; la conexión llega sola.
void start(const char* ssid, const char* password);

bool online();   // con IP
int rssi();      // dBm de la señal; 0 sin red

// La IP de la placa ("192.168.1.120"); vacía sin red
void ip_address(char* out, std::size_t size);

// ¿Ya llegó la hora del NTP? Antes de eso time() da 1970.
bool time_valid();

// POST de texto. true si el servidor contestó 2xx.
bool post(const char* url, const char* body, std::size_t size);

// ¿La URL es https? Entonces se revisa su certificado con el paquete de
// autoridades de ESP-IDF; con http (la LAN) no hay certificado que revisar.
bool is_https(const char* url);

}  // namespace net
