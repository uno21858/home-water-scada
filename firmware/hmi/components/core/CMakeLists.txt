# core/ del repo como componente de ESP-IDF: la misma lógica que prueba el simulador
set(CORE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../core)
# Un .cpp nuevo se ve con idf.py reconfigure
file(GLOB CORE_SOURCES ${CORE_DIR}/*.cpp)

idf_component_register(SRCS ${CORE_SOURCES} INCLUDE_DIRS ${CORE_DIR})
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra)
