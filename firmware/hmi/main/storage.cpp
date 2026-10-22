#include "storage.h"

#include "esp_log.h"
#include "nvs_flash.h"

namespace storage {
namespace {

constexpr const char* kTag = "storage";
constexpr const char* kNamespace = "hmi";
constexpr const char* kWifiPartition = "wifi";

}  // namespace

bool NvsStore::open() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Partición llena o de otro formato: se borra y arranca de fábrica
        ESP_LOGW(kTag, "La NVS no abre (%s): se borra", esp_err_to_name(err));
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err == ESP_OK) err = nvs_open(kNamespace, NVS_READWRITE, &handle_);
    open_ = err == ESP_OK;
    if (!open_) ESP_LOGE(kTag, "Sin NVS (%s): no se va a guardar nada", esp_err_to_name(err));
    return open_;
}

bool NvsStore::read(const char* key, void* data, std::size_t size) {
    if (!open_) return false;
    std::size_t saved = 0;
    if (nvs_get_blob(handle_, key, nullptr, &saved) != ESP_OK || saved != size) return false;
    return nvs_get_blob(handle_, key, data, &saved) == ESP_OK;
}

bool NvsStore::write(const char* key, const void* data, std::size_t size) {
    if (!open_) return false;
    const esp_err_t err = nvs_set_blob(handle_, key, data, size);
    if (err != ESP_OK) ESP_LOGE(kTag, "No se guardó %s (%s)", key, esp_err_to_name(err));
    return err == ESP_OK;
}

void NvsStore::commit() {
    if (open_) nvs_commit(handle_);
}

bool wifi_credentials(char* ssid, std::size_t ssid_size, char* password, std::size_t password_size) {
    if (nvs_flash_init_partition(kWifiPartition) != ESP_OK) return false;
    nvs_handle_t handle = 0;
    if (nvs_open_from_partition(kWifiPartition, "wifi", NVS_READONLY, &handle) != ESP_OK) return false;
    const bool ok = nvs_get_str(handle, "ssid", ssid, &ssid_size) == ESP_OK &&
                    nvs_get_str(handle, "password", password, &password_size) == ESP_OK;
    nvs_close(handle);
    return ok;
}

}  // namespace storage
