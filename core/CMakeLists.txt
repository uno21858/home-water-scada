# Lógica pura, sin LVGL ni hardware: la usan el firmware y el simulador.
add_library(core INTERFACE)
target_include_directories(core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(core INTERFACE cxx_std_17)
