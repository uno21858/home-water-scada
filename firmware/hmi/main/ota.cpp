#include "ota.h"

#include <cstring>

#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "net.h"
#include "sdkconfig.h"

namespace ota {
namespace {

constexpr const char* kTag = "ota";
constexpr uint32_t kFirstCheckMs = 60 * 1000;  // que arranque todo antes de la primera revisada
constexpr uint32_t kWaitMs = 30 * 1000;        // cada cuánto vuelve a ver si ya puede reiniciar

bool (*can_install)() = nullptr;
void (*restart)() = nullptr;

// ¿Vale la pena bajar este binario?
bool wanted(const esp_app_desc_t& incoming) {
    const esp_app_desc_t* running = esp_app_get_description();
    if (std::strncmp(incoming.project_name, running->project_name, sizeof incoming.project_name) != 0) {
        ESP_LOGW(kTag, "El .bin es de otro proyecto (%.32s): no se instala", incoming.project_name);
        return false;
    }
    if (std::memcmp(incoming.app_elf_sha256, running->app_elf_sha256, sizeof incoming.app_elf_sha256) == 0) {
        return false;  // es el que ya corre
    }
    const esp_partition_t* failed = esp_ota_get_last_invalid_partition();
    esp_app_desc_t failed_desc = {};
    if (failed != nullptr && esp_ota_get_partition_description(failed, &failed_desc) == ESP_OK &&
        std::memcmp(failed_desc.app_elf_sha256, incoming.app_elf_sha256, sizeof incoming.app_elf_sha256) == 0) {
        ESP_LOGW(kTag, "%.32s ya falló una vez: se queda la que corre", incoming.version);
        return false;
    }
    return true;
}

void check() {
    const char* url = CONFIG_HMI_OTA_URL;
    if (url[0] == '\0' || !net::online() || !can_install()) return;

    esp_http_client_config_t http = {};
    http.url = url;
    http.timeout_ms = 10000;
    http.keep_alive_enable = true;
    if (net::is_https(url)) http.crt_bundle_attach = esp_crt_bundle_attach;
    esp_https_ota_config_t config = {};
    config.http_config = &http;

    esp_https_ota_handle_t handle = nullptr;
    esp_err_t err = esp_https_ota_begin(&config, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "No se pudo leer %s (%s)", url, esp_err_to_name(err));
        return;
    }
    esp_app_desc_t incoming = {};
    if (esp_https_ota_get_img_desc(handle, &incoming) != ESP_OK || !wanted(incoming)) {
        esp_https_ota_abort(handle);
        return;
    }

    ESP_LOGI(kTag, "Bajando %.32s", incoming.version);
    do {
        err = esp_https_ota_perform(handle);
    } while (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS);
    if (err != ESP_OK || !esp_https_ota_is_complete_data_received(handle)) {
        ESP_LOGW(kTag, "Se cortó la descarga (%s)", esp_err_to_name(err));
        esp_https_ota_abort(handle);
        return;
    }
    // Aquí se revisa la firma; si cuadra, queda para el siguiente arranque
    err = esp_https_ota_finish(handle);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "No se instaló %.32s (%s): ¿firmada con otra llave?", incoming.version, esp_err_to_name(err));
        return;
    }
    ESP_LOGI(kTag, "%.32s lista: reinicia en cuanto se pueda", incoming.version);
    while (!can_install()) vTaskDelay(pdMS_TO_TICKS(kWaitMs));
    restart();
}

void task(void*) {
    vTaskDelay(pdMS_TO_TICKS(kFirstCheckMs));
    while (true) {
        check();
        vTaskDelay(pdMS_TO_TICKS(CONFIG_HMI_OTA_CHECK_MINUTES * 60 * 1000));
    }
}

}  // namespace

void start(bool (*can_install_now)(), void (*restart_now)()) {
    can_install = can_install_now;
    restart = restart_now;
    if (CONFIG_HMI_OTA_URL[0] == '\0') {
        ESP_LOGI(kTag, "Sin URL de OTA: no se buscan versiones nuevas");
        return;
    }
    xTaskCreate(task, "ota", 8192, nullptr, 2, nullptr);
}

Health check_health(uint32_t uptime_seconds, bool online) {
    static bool testing = [] {
        esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
        return esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) == ESP_OK &&
               state == ESP_OTA_IMG_PENDING_VERIFY;
    }();
    if (!testing) return Health::fine;
    if (online && uptime_seconds >= kHealthySeconds) {
        esp_ota_mark_app_valid_cancel_rollback();
        testing = false;
        ESP_LOGI(kTag, "Esta versión queda como buena");
        return Health::fine;
    }
    return uptime_seconds >= kHealthTimeoutSeconds ? Health::failed : Health::testing;
}

void rollback() {
    ESP_LOGE(kTag, "La versión nueva no se conectó: regresa a la anterior");
    esp_ota_mark_app_invalid_rollback_and_reboot();
}

}  // namespace ota
