//
// Created by Erick on 07/10/26.
//


#include <chrono>
#include <cstdint>
#include <thread>

#include "lvgl/lvgl.h"

int main()
{
    lv_init();

    // Display: ventana SDL del tamaño de la 7B
    lv_display_t* disp = lv_sdl_window_create(1024, 600);

    // El mouse hace de dedo (touch)
    lv_indev_t* touch = lv_sdl_mouse_create();
    lv_indev_set_display(touch, disp);

    // Prueba: un texto al centro para confirmar que dibuja
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "home-water-scada");
    lv_obj_center(label);

    while (true) {
        uint32_t ms = lv_timer_handler();
        if (ms == LV_NO_TIMER_READY) {
            ms = LV_DEF_REFR_PERIOD;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}