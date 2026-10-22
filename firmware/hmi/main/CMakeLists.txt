idf_component_register(
    SRCS main.cpp app.cpp board.cpp net.cpp ota.cpp sensors.cpp storage.cpp
    INCLUDE_DIRS "."
    REQUIRES core ui lvgl
    PRIV_REQUIRES app_update esp_app_format esp_driver_gpio esp_driver_i2c esp_driver_pcnt esp_event
                  esp_http_client esp_https_ota esp_lcd esp_netif esp_timer esp_wifi mbedtls nvs_flash)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra)
