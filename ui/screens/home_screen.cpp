#include "screens/home_screen.h"

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"

namespace ui {

lv_obj_t* HomeScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    lv_obj_t* text = label(page, &font_body_32, colors::text_disabled, "Tren: próximamente");
    lv_obj_center(text);
    return page;
}

void HomeScreen::update(const core::SystemState& /*s*/) {}

}  // namespace ui
