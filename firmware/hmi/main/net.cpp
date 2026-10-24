#include "net.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <initializer_list>

#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "esp_wifi.h"

namespace net {
namespace {

constexpr const char* kTag = "net";
constexpr uint64_t kRetryUs = 5ULL * 1000 * 1000;  // espera entre intentos de conexión
constexpr std::time_t kYear2026 = 1767225600;       // 1 ene 2026 en UTC: antes, el NTP no ha llegado
constexpr const char* kHostname = "hmi-agua";

std::atomic<bool> connected{false};
esp_timer_handle_t retry = nullptr;
esp_netif_t* station = nullptr;

void reconnect(void*) { esp_wifi_connect(); }

// Copia a un campo de wifi_config_t, que no lleva '\0' si va lleno
template <std::size_t N>
void copy(uint8_t (&field)[N], const char* text) {
    std::memcpy(field, text, std::min(N, std::strlen(text)));
}

void on_event(void*, esp_event_base_t base, int32_t id, void*) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        connected = false;
        esp_timer_stop(retry);  // por si ya iba uno
        esp_timer_start_once(retry, kRetryUs);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        connected = true;
        ESP_LOGI(kTag, "En línea");
    }
}

}  // namespace

void start(const char* ssid, const char* password) {
    // Cada conexión que falla (sin servidor de OTA o de datos) son 4 a 6
    // renglones rojos de estos componentes de IDF; ota y net lo dicen en uno
    for (const char* tag : {"esp_https_ota", "HTTP_CLIENT", "esp-tls", "transport_base", "transport"}) {
        esp_log_level_set(tag, ESP_LOG_NONE);
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    station = esp_netif_create_default_wifi_sta();
    // Así sale en la lista de equipos del módem (el DHCP), en vez de "espressif"
    esp_netif_set_hostname(station, kHostname);

    esp_timer_create_args_t timer = {};
    timer.callback = reconnect;
    timer.name = "wifi_retry";
    ESP_ERROR_CHECK(esp_timer_create(&timer, &retry));

    const wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));  // la red ya vive en la partición wifi
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, nullptr, nullptr));

    wifi_config_t config = {};
    copy(config.sta.ssid, ssid);
    copy(config.sta.password, password);
    config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // La hora de Guadalajara: UTC−6 todo el año (México quitó el horario de
    // verano en 2022). El NTP da UTC y localtime() la convierte.
    setenv("TZ", "CST6", 1);
    tzset();
    esp_sntp_config_t sntp = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    ESP_ERROR_CHECK(esp_netif_sntp_init(&sntp));
}

bool online() { return connected; }

void ip_address(char* out, std::size_t size) {
    if (size == 0) return;
    out[0] = '\0';
    esp_netif_ip_info_t info = {};
    if (!connected || station == nullptr || esp_netif_get_ip_info(station, &info) != ESP_OK) return;
    std::snprintf(out, size, IPSTR, IP2STR(&info.ip));
}

int rssi() {
    int value = 0;
    if (!connected || esp_wifi_sta_get_rssi(&value) != ESP_OK) return 0;
    return value;
}

bool time_valid() { return std::time(nullptr) >= kYear2026; }

bool is_https(const char* url) { return std::strncmp(url, "https://", 8) == 0; }

bool post(const char* url, const char* body, std::size_t size) {
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 5000;
    if (is_https(url)) config.crt_bundle_attach = esp_crt_bundle_attach;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) return false;
    esp_http_client_set_header(client, "Content-Type", "text/plain");
    esp_http_client_set_post_field(client, body, static_cast<int>(size));
    const esp_err_t err = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (err != ESP_OK || status < 200 || status >= 300) {
        ESP_LOGW(kTag, "POST a %s: %s, %d", url, esp_err_to_name(err), status);
        return false;
    }
    return true;
}

}  // namespace net
