#pragma once
// La NVS de la 7B: lo que core::load y core::save guardan (partición nvs) y
// la red de la casa (partición wifi, grabada desde wifi.csv por USB).

#include <cstddef>

#include "nvs.h"
#include "persist.h"

namespace storage {

// core::Store sobre la NVS: cada llave es un blob con su tamaño exacto
class NvsStore : public core::Store {
public:
    bool open();  // inicia la partición nvs; false si ni borrándola arranca
    bool read(const char* key, void* data, std::size_t size) override;
    bool write(const char* key, const void* data, std::size_t size) override;
    void commit();  // lo escrito queda en la flash

private:
    nvs_handle_t handle_ = 0;
    bool open_ = false;
};

// Nombre y contraseña de la red (partición wifi). false si no se grabaron.
bool wifi_credentials(char* ssid, std::size_t ssid_size, char* password, std::size_t password_size);

}  // namespace storage
