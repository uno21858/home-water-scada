//
// Created by Erick on 09/10/26.
//

#include "ui.h"

#include "lvgl.h";


void ui_init() {
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hola desde ui/");
    lv_obj_center(label);
}
