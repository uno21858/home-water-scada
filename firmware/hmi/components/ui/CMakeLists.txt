# ui/ del repo como componente de ESP-IDF: las mismas pantallas del simulador
set(UI_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../ui)
# Un .cpp nuevo se ve con idf.py reconfigure
file(GLOB_RECURSE UI_SOURCES ${UI_DIR}/*.cpp ${UI_DIR}/*.c)

idf_component_register(SRCS ${UI_SOURCES} INCLUDE_DIRS ${UI_DIR} REQUIRES lvgl core)

# Warnings solo para nuestro C++, no para las fuentes e íconos generados
target_compile_options(${COMPONENT_LIB} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra>")
