# Lógica pura, sin LVGL ni hardware: la usan el firmware y el simulador.
# Toma cualquier .cpp nuevo dentro de core/.
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(core STATIC ${CORE_SOURCES})
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(core PUBLIC cxx_std_17)
target_compile_options(core PRIVATE -Wall -Wextra)
