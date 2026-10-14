add_library(ui STATIC ui.cpp
        screens/home_screen.cpp
        screens/home_screen.h
        theme/colors.h
        components/nav_bar.cpp
        components/nav_bar.h)
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ui PUBLIC lvgl)