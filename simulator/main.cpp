//
// Created by Erick on 07/10/26.
//


#include <chrono>
#include <cstdint>
#include <thread>

#include "lvgl/lvgl.h"


#include "ui.h"

int main()
{
    lv_init();

    // Display: ventana SDL del tamaño de la 7B
    lv_display_t* disp = lv_sdl_window_create(1024, 600);

    // El mouse hace de dedo (touch)
    lv_indev_t* touch = lv_sdl_mouse_create();
    lv_indev_set_display(touch, disp);

    // Construye la interfaz (vive en ui/)
    ui_init();

    while (true) {
        uint32_t ms = lv_timer_handler();
        if (ms == LV_NO_TIMER_READY) {
            ms = LV_DEF_REFR_PERIOD;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}