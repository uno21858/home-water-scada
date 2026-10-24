# Firmware de la 7B (Waveshare ESP32-S3-Touch-LCD-7B) con ESP-IDF v6.1.
# Usa los mismos core/ y ui/ que el simulador, y el LVGL del submódulo.
cmake_minimum_required(VERSION 3.22)

set(REPO_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

# Versión: el commit de git, con + si hay cambios sin commit ("4946811+").
# La ve Diagnóstico; la OTA no se fija en ella sino en el hash del binario.
execute_process(
    COMMAND git describe --always --dirty=+
    WORKING_DIRECTORY ${REPO_DIR}
    OUTPUT_VARIABLE PROJECT_VER
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)

# Eso solo corre cuando CMake se configura: tras un git pull o un commit que
# no toca el código, la versión se quedaba en la anterior y salía el mismo
# binario (la OTA lo ve igual y no instala nada). CMake se vuelve a configurar
# cuando cambia el commit: HEAD (otra rama), la rama (pull, commit) o
# packed-refs (donde queda la rama después de un git gc).
execute_process(
    COMMAND git symbolic-ref -q HEAD
    WORKING_DIRECTORY ${REPO_DIR}
    OUTPUT_VARIABLE git_branch
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
foreach(ref HEAD ${git_branch} packed-refs)
    execute_process(
        COMMAND git rev-parse --path-format=absolute --git-path ${ref}
        WORKING_DIRECTORY ${REPO_DIR}
        OUTPUT_VARIABLE ref_file
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(EXISTS "${ref_file}")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ref_file}")
    endif()
endforeach()

# LVGL es un componente de ESP-IDF tal cual (simulator/lvgl/env_support/cmake/esp.cmake)
set(EXTRA_COMPONENT_DIRS ${REPO_DIR}/simulator/lvgl)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hmi)

# LVGL toma su configuración de hmi_lv_conf.h y no del Kconfig de LVGL. Va
# PUBLIC en LVGL para que ui y main la vean igual.
idf_component_get_property(lvgl_lib lvgl COMPONENT_LIB)
target_compile_definitions(${lvgl_lib} PUBLIC
    "LV_CONF_PATH=\"${CMAKE_CURRENT_LIST_DIR}/hmi_lv_conf.h\""
    LV_KCONFIG_IGNORE)
# LVGL trae printf con %lu para int32_t: en el ESP32 es int y IDF lo vuelve
# error. Solo en el código de LVGL (el demo de Waveshare hace lo mismo).
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)

# La red de la casa va en su propia partición (wifi.csv, fuera de git): no
# viaja en el .bin de la OTA y se graba con idf.py flash.
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/wifi.csv)
    nvs_create_partition_image(wifi ${CMAKE_CURRENT_LIST_DIR}/wifi.csv FLASH_IN_PROJECT)
else()
    # Para subir por WiFi no hace falta: la red ya está grabada en la placa
    message(STATUS "Sin firmware/hmi/wifi.csv: idf.py flash no graba la red (subir-ota.sh no la necesita)")
endif()
