cmake_minimum_required(VERSION 3.16)
project(hmi_simulator C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

# LVGL como librería; le decimos dónde está nuestro lv_conf.h
add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})

add_executable(simulator main.cpp)
target_compile_definitions(simulator PRIVATE LV_CONF_INCLUDE_SIMPLE)

target_link_libraries(simulator PRIVATE ui lvgl ${SDL2_LIBRARIES} m pthread)


add_subdirectory(${PROJECT_SOURCE_DIR}/../ui ${CMAKE_BINARY_DIR}/ui)