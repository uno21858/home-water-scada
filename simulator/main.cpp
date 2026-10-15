//
// Created by Erick on 07/10/26.
//


#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "lvgl/lvgl.h"

#include "demo.h"
#include "ui.h"

int main()
{
    lv_init();

    // Display: ventana SDL del tamaño de la 7B
    lv_display_t* disp = lv_sdl_window_create(1024, 600);

    // El mouse hace de dedo (touch)
    lv_indev_t* touch = lv_sdl_mouse_create();
    lv_indev_set_display(touch, disp);

    // Escenario del demo: HMI_DEMO=sin-flujo, sin-red, alarma... (ver demo.h)
    if (const char* scenario = std::getenv("HMI_DEMO")) {
        if (!demo::set_scenario(scenario)) std::fprintf(stderr, "HMI_DEMO: no conozco \"%s\"\n", scenario);
    }

    // Construye la interfaz (vive en ui/) con los datos de ejemplo
    ui::Actions actions;
    actions.dose_confirmed = demo::on_dose_confirmed;
    actions.dose_typed = demo::on_dose_typed;
    actions.strip_checked = demo::on_strip_checked;
    actions.mixer_mode_set = demo::on_mixer_mode;
    actions.mixer_after_dose_set = demo::on_mixer_after_dose;
    actions.mixer_every_4h_set = demo::on_mixer_every_4h;
    actions.flow_factor_set = demo::on_flow_factor;
    actions.pressure_zeroed = demo::on_pressure_zeroed;
    actions.dose_ratio_set = demo::on_dose_ratio;
    actions.daily_cap_set = demo::on_daily_cap;
    actions.night_off_set = demo::on_night_off;
    actions.idle_minutes_set = demo::on_idle_minutes;
    actions.filter_serviced = demo::on_filter_serviced;
    actions.alarm_acknowledged = demo::on_alarm_acknowledged;
    ui::init(demo::state(), actions);

    // Cada segundo cambian los datos, como lo haría el firmware
    lv_timer_create([](lv_timer_t*) {
        demo::tick_1s();
        ui::update(demo::state());
    }, 1000, nullptr);

    while (true) {
        uint32_t ms = lv_timer_handler();
        if (ms == LV_NO_TIMER_READY) {
            ms = LV_DEF_REFR_PERIOD;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}
