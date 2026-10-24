cmake_minimum_required(VERSION 3.16)
project(hmi_simulator C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

# Sin los ejemplos ni los demos de LVGL: no los usamos y alargan la compilación
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)

# LVGL como librería; le decimos dónde está nuestro lv_conf.h
add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})

add_executable(simulator main.cpp demo.cpp)
target_compile_definitions(simulator PRIVATE LV_CONF_INCLUDE_SIMPLE)

target_link_libraries(simulator PRIVATE ui core lvgl ${SDL2_LIBRARIES} m pthread)

add_subdirectory(${PROJECT_SOURCE_DIR}/../core ${CMAKE_BINARY_DIR}/core)

# Pruebas de core en la compu: ctest --test-dir build
enable_testing()
add_executable(core_test ${PROJECT_SOURCE_DIR}/../core/test/core_test.cpp)
target_link_libraries(core_test PRIVATE core)
target_compile_options(core_test PRIVATE -Wall -Wextra)
add_test(NAME core COMMAND core_test)
add_subdirectory(${PROJECT_SOURCE_DIR}/../ui ${CMAKE_BINARY_DIR}/ui)

# Un target por escenario del demo (los de demo::set_scenario en demo.h):
# en CLion salen en la lista de configuraciones y en la terminal es
#   cmake --build build --target demo-sin-agua
# cmake -E env le pone HMI_DEMO solo a ese comando, igual en Mac y Fedora.
set(DEMO_SCENARIOS
    sin-flujo sin-red alarma sensor tope fase1 completo ro-cocina
    revisar tapados sin-agua cisterna-vacia sin-12v nivel-bajo atorada actualizando a-prueba)
foreach(scenario IN LISTS DEMO_SCENARIOS)
    add_custom_target(demo-${scenario}
        COMMAND ${CMAKE_COMMAND} -E env HMI_DEMO=${scenario} $<TARGET_FILE:simulator>
        USES_TERMINAL
        COMMENT "Simulador con HMI_DEMO=${scenario}")
    add_dependencies(demo-${scenario} simulator)  # primero compila lo que haga falta
endforeach()
