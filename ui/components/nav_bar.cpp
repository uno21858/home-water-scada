#include "components/nav_bar.h"

#include <cstdint>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/icons.h"

namespace ui {
namespace {

struct TabDef {
    const char* name;
    const lv_image_dsc_t* icon;
    int32_t width;
};

// Mismo orden que el enum Screen. 5 x 146 + 2 x 147 = 1024
const TabDef kTabs[kScreenCount] = {
    {"Tren", &icon_tren_32, 146},          {"Resumen", &icon_resumen_32, 146},
    {"Flujo", &icon_flujo_32, 146},        {"Cisterna", &icon_cisterna_32, 147},
    {"Dosis", &icon_dosis_32, 146},        {"Historial", &icon_historial_32, 146},
    {"Ajustes", &icon_ajustes_32, 147},
};

void on_click(lv_event_t* e) {
    show(static_cast<Screen>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e))));
}

}  // namespace

void NavBar::create(lv_obj_t* parent) {
    lv_obj_t* root = box(parent, 0, 520, 1024, 80);
    lv_obj_set_style_bg_color(root, color(colors::panel), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    lv_obj_t* line = box(root, 0, 0, 1024, 1);
    lv_obj_set_style_bg_color(line, color(colors::border), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);

    int32_t x = 0;
    for (int i = 0; i < kScreenCount; ++i) {
        Tab& tab = tabs_[i];
        tab.obj = box(root, x, 0, kTabs[i].width, 80);
        lv_obj_set_clickable(tab.obj, true);
        lv_obj_set_flex_flow(tab.obj, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tab.obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(tab.obj, 2, 0);

        // La barra de 4 px existe en todas y solo se ve en la activa:
        // así el ícono y el texto no brincan al cambiar de pestaña
        lv_obj_set_style_border_side(tab.obj, LV_BORDER_SIDE_TOP, 0);
        lv_obj_set_style_border_width(tab.obj, 4, 0);
        lv_obj_set_style_border_color(tab.obj, color(colors::text), 0);
        lv_obj_set_style_border_opa(tab.obj, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_bottom(tab.obj, 3, 0);
        lv_obj_set_style_bg_color(tab.obj, color(colors::panel_raised), 0);
        lv_obj_set_style_bg_opa(tab.obj, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(tab.obj, color(colors::secondary_pressed), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(tab.obj, LV_OPA_COVER, LV_STATE_PRESSED);

        tab.icon = icon(tab.obj, kTabs[i].icon, colors::text_secondary);
        tab.text = label(tab.obj, &font_label_26, colors::text_secondary, kTabs[i].name);
        lv_obj_add_event_cb(tab.obj, on_click, LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        x += kTabs[i].width;
    }
}

void NavBar::set_active(Screen screen) {
    for (int i = 0; i < kScreenCount; ++i) {
        const bool on = i == static_cast<int>(screen);
        Tab& tab = tabs_[i];
        lv_obj_set_style_bg_opa(tab.obj, on ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(tab.obj, on ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        const uint32_t fg = on ? colors::text : colors::text_secondary;
        set_icon_color(tab.icon, fg);
        lv_obj_set_style_text_color(tab.text, color(fg), 0);
    }
}

}  // namespace ui
