# Interfaz en LVGL. No sabe nada de SDL ni del ESP32: la usan el simulador y el firmware.
# Toma solo cualquier .cpp o .c nuevo dentro de ui/ (fuentes e íconos generados incluidos).
file(GLOB_RECURSE UI_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(ui STATIC ${UI_SOURCES})
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ui PUBLIC lvgl core)
target_compile_features(ui PUBLIC cxx_std_17)

# Warnings solo para nuestro C++, no para las fuentes e íconos generados
target_compile_options(ui PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra>")
