#include "ota.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>

#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_http_server.h"
#include "esp_https_ota.h"
#include "esp_image_format.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "net.h"
#include "sdkconfig.h"

namespace ota {
namespace {

constexpr const char* kTag = "ota";
constexpr uint32_t kFirstCheckMs = 60 * 1000;  // que arranque todo antes de la primera revisada
constexpr uint32_t kWaitMs = 30 * 1000;        // cada cuánto vuelve a ver si ya puede reiniciar

// Dónde va la descripción (versión, proyecto, hash) dentro del .bin: después
// de la cabecera de la imagen y la del primer segmento (igual que esp_https_ota)
constexpr std::size_t kDescOffset = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);
constexpr std::size_t kHeadBytes = kDescOffset + sizeof(esp_app_desc_t);

bool (*can_install)() = nullptr;
void (*restart)() = nullptr;

// Una sola escritura a la vez a la otra partición: la revisada de la URL o
// una subida. Ya instalada una, ninguna más hasta reiniciar.
std::atomic<bool> busy{false};
std::atomic<bool> installed{false};

// Toma la partición para escribir; se suelta sola al salir del bloque
class Writer {
public:
    Writer() {
        bool expected = false;
        owned_ = !installed && busy.compare_exchange_strong(expected, true);
    }
    ~Writer() {
        if (owned_) busy = false;
    }
    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;
    bool owned() const { return owned_; }

private:
    bool owned_ = false;
};

// Lo que ve la pantalla de actualización (progress); lo tocan la tarea de la
// URL y el servidor de subidas, y lo lee la de LVGL
portMUX_TYPE status_lock = portMUX_INITIALIZER_UNLOCKED;
core::UpdateStatus status{};
int64_t failed_at_us = 0;
constexpr int64_t kFailedShowUs = 15LL * 1000 * 1000;
constexpr uint32_t kShowDoneMs = 2500;  // "Lista" en pantalla antes de reiniciar

void report_start(const char* version) {
    core::UpdateStatus fresh{};  // se arma afuera: en la sección crítica, nada de snprintf
    fresh.stage = core::UpdateStage::receiving;
    std::snprintf(fresh.version, sizeof fresh.version, "%.*s", static_cast<int>(sizeof fresh.version - 1), version);
    taskENTER_CRITICAL(&status_lock);
    status = fresh;
    taskEXIT_CRITICAL(&status_lock);
}

void report_pct(std::size_t done, std::size_t total) {
    if (total == 0) return;
    const auto pct = static_cast<uint8_t>(std::min<std::size_t>(100, done * 100 / total));
    taskENTER_CRITICAL(&status_lock);
    status.pct = pct;
    taskEXIT_CRITICAL(&status_lock);
}

void report_installed(bool waits_for_mixer) {
    taskENTER_CRITICAL(&status_lock);
    status.stage = core::UpdateStage::installed;
    status.pct = 100;
    status.waits_for_mixer = waits_for_mixer;
    taskEXIT_CRITICAL(&status_lock);
}

void report_failed(core::UpdateFailure failure) {
    const int64_t now = esp_timer_get_time();
    taskENTER_CRITICAL(&status_lock);
    status.stage = core::UpdateStage::failed;
    status.failure = failure;
    failed_at_us = now;
    taskEXIT_CRITICAL(&status_lock);
}

// Para no llenar el monitor cada minuto: "no contesta" y "es la que ya
// corre" se dicen una vez, hasta que cambie
bool reachable = true;
uint8_t said_same[32] = {};  // app_elf_sha256 de la que ya se dijo

// ¿Se instala este binario?
enum class Verdict : uint8_t { install, not_firmware, other_project, same, failed_before };

Verdict judge(const esp_app_desc_t& incoming) {
    if (incoming.magic_word != ESP_APP_DESC_MAGIC_WORD) return Verdict::not_firmware;
    const esp_app_desc_t* running = esp_app_get_description();
    if (std::strncmp(incoming.project_name, running->project_name, sizeof incoming.project_name) != 0) {
        return Verdict::other_project;
    }
    if (std::memcmp(incoming.app_elf_sha256, running->app_elf_sha256, sizeof incoming.app_elf_sha256) == 0) {
        return Verdict::same;
    }
    const esp_partition_t* failed = esp_ota_get_last_invalid_partition();
    esp_app_desc_t failed_desc = {};
    if (failed != nullptr && esp_ota_get_partition_description(failed, &failed_desc) == ESP_OK &&
        std::memcmp(failed_desc.app_elf_sha256, incoming.app_elf_sha256, sizeof incoming.app_elf_sha256) == 0) {
        return Verdict::failed_before;
    }
    return Verdict::install;
}

// Ya quedó la nueva para el siguiente arranque: reinicia cuando no esté la
// bomba R corriendo. En su propia tarea, para no detener al que la mandó.
void restart_when_free(void*) {
    vTaskDelay(pdMS_TO_TICKS(kShowDoneMs));  // que salga la respuesta y se vea "Lista"
    while (!can_install()) vTaskDelay(pdMS_TO_TICKS(kWaitMs));
    restart();
}

// --- Revisar la URL de menuconfig (la placa jala) ---

void check() {
    const char* url = CONFIG_HMI_OTA_URL;
    if (url[0] == '\0' || !net::online() || !can_install()) return;
    Writer writer;
    if (!writer.owned()) return;  // hay una subida en curso, o ya se instaló una

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
        if (reachable) {
            ESP_LOGI(kTag, "Nadie sirve %s (%s): sigo revisando cada %d min", url, esp_err_to_name(err),
                     CONFIG_HMI_OTA_CHECK_MINUTES);
        }
        reachable = false;
        return;
    }
    if (!reachable) ESP_LOGI(kTag, "%s ya contesta", url);
    reachable = true;

    esp_app_desc_t incoming = {};
    if (esp_https_ota_get_img_desc(handle, &incoming) != ESP_OK) {
        esp_https_ota_abort(handle);
        return;
    }
    const Verdict verdict = judge(incoming);
    if (verdict != Verdict::install) {
        // Cuelga después de la cabecera (servir_ota.py lo ve y se sale)
        static_assert(sizeof said_same == sizeof incoming.app_elf_sha256);
        if (verdict == Verdict::same && std::memcmp(said_same, incoming.app_elf_sha256, sizeof said_same) != 0) {
            ESP_LOGI(kTag, "%.32s es la que ya corre: nada que instalar", incoming.version);
            std::memcpy(said_same, incoming.app_elf_sha256, sizeof said_same);
        } else if (verdict == Verdict::other_project) {
            ESP_LOGW(kTag, "El .bin es de otro proyecto (%.32s): no se instala", incoming.project_name);
        } else if (verdict == Verdict::failed_before) {
            ESP_LOGW(kTag, "%.32s ya falló una vez: se queda la que corre", incoming.version);
        }
        esp_https_ota_abort(handle);
        return;
    }

    ESP_LOGI(kTag, "Bajando %.32s", incoming.version);
    report_start(incoming.version);
    const int total = esp_https_ota_get_image_size(handle);
    do {
        err = esp_https_ota_perform(handle);
        const int read = esp_https_ota_get_image_len_read(handle);
        if (total > 0 && read > 0) report_pct(static_cast<std::size_t>(read), static_cast<std::size_t>(total));
    } while (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS);
    if (err != ESP_OK || !esp_https_ota_is_complete_data_received(handle)) {
        ESP_LOGW(kTag, "Se cortó la descarga (%s)", esp_err_to_name(err));
        report_failed(core::UpdateFailure::cut);
        esp_https_ota_abort(handle);
        return;
    }
    // Aquí se revisa la firma; si cuadra, queda para el siguiente arranque
    err = esp_https_ota_finish(handle);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "No se instaló %.32s (%s): ¿firmada con otra llave?", incoming.version, esp_err_to_name(err));
        report_failed(err == ESP_ERR_OTA_VALIDATE_FAILED ? core::UpdateFailure::signature : core::UpdateFailure::other);
        return;
    }
    installed = true;
    report_installed(!can_install());
    ESP_LOGI(kTag, "%.32s lista: reinicia en cuanto se pueda", incoming.version);
    vTaskDelay(pdMS_TO_TICKS(kShowDoneMs));  // que se vea "Lista"
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

// --- Subir desde la compu (la compu empuja: subir-ota.sh) ---

// Contesta texto con su estado HTTP y lo deja también en el monitor
esp_err_t reply(httpd_req_t* req, const char* status, const char* text) {
    if (std::strncmp(status, "200", 3) == 0) {
        ESP_LOGI(kTag, "%s", text);
    } else {
        ESP_LOGW(kTag, "%s", text);
    }
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_sendstr(req, text);
}

// Lee justo n bytes del cuerpo; false si la conexión se cortó
bool receive(httpd_req_t* req, char* buf, std::size_t n) {
    int timeouts = 0;
    for (std::size_t got = 0; got < n;) {
        const int r = httpd_req_recv(req, buf + got, n - got);
        if (r == HTTPD_SOCK_ERR_TIMEOUT && ++timeouts < 3) continue;
        if (r <= 0) return false;
        got += static_cast<std::size_t>(r);
    }
    return true;
}

// GET /version: lo que corre, para que el script diga si ya está al día
esp_err_t handle_version(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_sendstr(req, esp_app_get_description()->version);
}

// POST /ota con el hmi.bin en el cuerpo. Se escribe en la otra partición
// conforme llega; esp_ota_end revisa la firma con la llave de la casa.
esp_err_t handle_upload(httpd_req_t* req) {
    char text[96];
    Writer writer;
    if (!writer.owned()) {
        return reply(req, "409 Conflict", installed ? "Ya se instaló una versión: espera a que reinicie"
                                                    : "Ya está bajando o recibiendo otra versión");
    }
    // Recién actualizada, la que corre está a prueba (ota.h) y ESP-IDF no deja
    // escribir la otra hasta que quede como buena: al minuto con red
    esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
    if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) == ESP_OK &&
        state == ESP_OTA_IMG_PENDING_VERIFY) {
        return reply(req, "409 Conflict", "Acaba de actualizarse y sigue a prueba: intenta en un minuto");
    }
    const esp_partition_t* partition = esp_ota_get_next_update_partition(nullptr);
    if (partition == nullptr) return reply(req, "500 Internal Server Error", "No hay partición para la OTA");
    if (req->content_len < kHeadBytes) return reply(req, "400 Bad Request", "Eso no es un firmware");
    if (req->content_len > partition->size) return reply(req, "413 Content Too Large", "No cabe en la partición");

    // La cabecera primero: con ella se sabe si vale la pena
    static char buf[4096];  // static: no cabe en la pila del servidor, y hay una subida a la vez
    static_assert(sizeof buf >= kHeadBytes);
    if (!receive(req, buf, kHeadBytes)) return ESP_FAIL;
    esp_app_desc_t incoming = {};
    std::memcpy(&incoming, buf + kDescOffset, sizeof incoming);
    switch (judge(incoming)) {
        case Verdict::install:
            break;
        case Verdict::not_firmware:
            return reply(req, "400 Bad Request", "Eso no es un firmware de ESP-IDF");
        case Verdict::other_project:
            std::snprintf(text, sizeof text, "Es de otro proyecto (%.32s): no se instala", incoming.project_name);
            return reply(req, "400 Bad Request", text);
        case Verdict::same:
            std::snprintf(text, sizeof text, "Ya tiene %.32s: nada que instalar", incoming.version);
            return reply(req, "200 OK", text);  // el resto del cuerpo lo tira el servidor
        case Verdict::failed_before:
            std::snprintf(text, sizeof text, "%.32s ya falló una vez: se queda la que corre", incoming.version);
            return reply(req, "409 Conflict", text);
    }

    ESP_LOGI(kTag, "Recibiendo %.32s (%u KB)", incoming.version, static_cast<unsigned>(req->content_len / 1024));
    report_start(incoming.version);
    esp_ota_handle_t ota = 0;
    esp_err_t err = esp_ota_begin(partition, req->content_len, &ota);  // borra solo lo que va a ocupar
    if (err == ESP_OK) err = esp_ota_write(ota, buf, kHeadBytes);
    for (std::size_t left = req->content_len - kHeadBytes; err == ESP_OK && left > 0;) {
        const std::size_t n = std::min(left, sizeof buf);
        if (!receive(req, buf, n)) {
            esp_ota_abort(ota);
            ESP_LOGW(kTag, "Se cortó la subida");
            report_failed(core::UpdateFailure::cut);
            return ESP_FAIL;
        }
        err = esp_ota_write(ota, buf, n);
        left -= n;
        report_pct(req->content_len - left, req->content_len);
    }
    if (err != ESP_OK) {
        if (ota != 0) esp_ota_abort(ota);
        report_failed(core::UpdateFailure::other);
        std::snprintf(text, sizeof text, "No se pudo escribir (%s)", esp_err_to_name(err));
        return reply(req, "500 Internal Server Error", text);
    }
    err = esp_ota_end(ota);  // aquí se revisa la firma
    if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
        report_failed(core::UpdateFailure::signature);
        return reply(req, "422 Unprocessable Content", "La firma no es de la llave de la casa (o llegó dañado): no se instaló");
    }
    if (err == ESP_OK) err = esp_ota_set_boot_partition(partition);
    if (err != ESP_OK) {
        report_failed(core::UpdateFailure::other);
        std::snprintf(text, sizeof text, "No se instaló (%s)", esp_err_to_name(err));
        return reply(req, "500 Internal Server Error", text);
    }

    installed = true;
    report_installed(!can_install());
    xTaskCreate(restart_when_free, "ota_restart", 8192, nullptr, 2, nullptr);  // guarda la NVS: como la de la OTA
    std::snprintf(text, sizeof text, can_install() ? "Instalada %.32s: se reinicia sola"
                                                   : "Instalada %.32s: se reinicia cuando acabe la bomba R",
                  incoming.version);
    return reply(req, "200 OK", text);
}

void start_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 6144;
    config.recv_wait_timeout = 10;
    httpd_handle_t server = nullptr;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGW(kTag, "No arrancó el servidor de subidas: solo queda la URL");
        return;
    }
    httpd_uri_t version = {};
    version.uri = "/version";
    version.method = HTTP_GET;
    version.handler = handle_version;
    httpd_register_uri_handler(server, &version);
    httpd_uri_t upload = {};
    upload.uri = "/ota";
    upload.method = HTTP_POST;
    upload.handler = handle_upload;
    httpd_register_uri_handler(server, &upload);
}

}  // namespace

void start(bool (*can_install_now)(), void (*restart_now)()) {
    can_install = can_install_now;
    restart = restart_now;
    start_server();
    if (CONFIG_HMI_OTA_URL[0] == '\0') {
        ESP_LOGI(kTag, "Sin URL de OTA: solo por subir-ota.sh");
        return;
    }
    xTaskCreate(task, "ota", 8192, nullptr, 2, nullptr);
}

core::UpdateStatus progress() {
    const int64_t now = esp_timer_get_time();
    taskENTER_CRITICAL(&status_lock);
    if (status.stage == core::UpdateStage::failed && now - failed_at_us > kFailedShowUs) status = {};
    const core::UpdateStatus copy = status;
    taskEXIT_CRITICAL(&status_lock);
    return copy;
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
