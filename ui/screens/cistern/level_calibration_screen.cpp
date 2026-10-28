#include "screens/cistern/level_calibration_screen.h"

#include <cmath>
#include <cstdio>

#include "components/keypad_dialog.h"
#include "components/primitives.h"
#include "level.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "ui.h"

namespace ui {
namespace {

struct RowText {
    const char* name;
    const char* button;
};
const RowText kRowText[6] = {
    {"Rango del sensor", "Cambiar"},
    {"Cero en el aire", "Hacer cero"},
    {"Altura del sensor", "Cambiar"},
    {"Toma de la bomba de pozo", "Cambiar"},
    {"Llena", "Está llena"},
    {"Litros por cm", "Medir"},
};
// Seis renglones de 60 con 6 de aire (eran cinco con 12) y la nota abajo
constexpr int32_t kRowStep = 66;

}  // namespace

lv_obj_t* LevelCalibrationScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    for (size_t i = 0; i < kRows; ++i) {
        lv_obj_t* c = card(page, 16, 12 + kRowStep * static_cast<int32_t>(i), 992, 60);
        lv_obj_set_pos(label(c, &font_body_32, colors::text, kRowText[i].name), 20, 10);
        values_[i] = label(c, &font_body_32, colors::text_secondary, "");
        lv_obj_set_pos(values_[i], 500, 10);  // "Toma de la bomba de pozo" llega a x 480
        lv_obj_t* button = button_secondary(c, 786, 1, 196, kRowText[i].button, handle_row, this);
        lv_obj_set_height(button, 56);
        lv_obj_set_user_data(button, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        buttons_[i] = button;
    }
    note_ = label(page, &font_label_26, colors::text_secondary, "");
    lv_obj_set_pos(note_, 16, 12 + kRowStep * static_cast<int32_t>(kRows) + 6);
    lv_obj_set_width(note_, 992);
    return page;
}

void LevelCalibrationScreen::update(const core::SystemState& s) {
    volts_ = s.level_volts;
    cm_ = core::level_cm(s);
    sensor_cm_ = s.level_sensor_cm;
    intake_cm_ = s.level_intake_cm;
    char buf[72];  // la nota sin señal mide 52 bytes
    snprintf(buf, sizeof buf, "0 a %u cm", static_cast<unsigned>(s.level_range_cm));
    set_text(values_[range], buf);
    snprintf(buf, sizeof buf, "%.3f V", static_cast<double>(s.level_zero_volts));
    set_text(values_[zero], buf);
    snprintf(buf, sizeof buf, "%u cm del fondo", static_cast<unsigned>(s.level_sensor_cm));
    set_text(values_[sensor], buf);
    snprintf(buf, sizeof buf, "%u cm del fondo", static_cast<unsigned>(s.level_intake_cm));
    set_text(values_[intake], buf);
    if (s.level_full_cm > 0) {
        snprintf(buf, sizeof buf, "%u cm", static_cast<unsigned>(s.level_full_cm));
    } else {
        snprintf(buf, sizeof buf, "Sin marcar");
    }
    set_text(values_[full], buf);
    if (s.level_measuring) {
        // Caben 280 px antes del botón: "Midiendo: 12.4 cm" no
        snprintf(buf, sizeof buf, "Subió %.1f cm", static_cast<double>(core::level_measure_rise(s)));
    } else if (s.level_liters_per_cm > 0.0f) {
        char value[16];
        fmt::one_decimal(value, sizeof value, s.level_liters_per_cm);
        snprintf(buf, sizeof buf, "%s L por cm", value);
    } else {
        snprintf(buf, sizeof buf, "Sin medir");
    }
    set_text(values_[liters], buf);

    // Sin señal lo que se guardara sería de un cable suelto: el cero y "Está
    // llena" esperan a que regrese
    const bool lost = s.no_signal.level;
    lv_obj_set_disabled(buttons_[zero], lost);
    lv_obj_set_disabled(buttons_[full], lost);
    if (lost) {
        snprintf(buf, sizeof buf, "Sin señal (%.3f V): revisa el cable del sensor.", static_cast<double>(s.level_volts));
    } else {
        snprintf(buf, sizeof buf, "Ahorita marca %ld cm desde el fondo y %.3f V.", std::lround(core::level_cm(s)),
                 static_cast<double>(s.level_volts));
    }
    set_text(note_, buf);
    set_text_color(note_, lost ? colors::warning : colors::text_secondary);
}

void LevelCalibrationScreen::handle_row(lv_event_t* e) {
    const auto* self = static_cast<LevelCalibrationScreen*>(lv_event_get_user_data(e));
    const auto which = static_cast<Row>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_current_target_obj(e))));
    char hint[48];
    char body[160];
    switch (which) {
        case range:
            snprintf(hint, sizeof hint, "En la etiqueta: 0 a 2 m es 200");
            open_keypad({"Rango del sensor", "Hasta", "cm", 0, 4, hint, nullptr, nullptr}, [](uint32_t cm) {
                if (actions().level_range_set != nullptr) actions().level_range_set(cm);
            });
            break;
        case zero:
            snprintf(body, sizeof body, "Con el sensor fuera del agua, lo que marca ahorita (%.3f V) queda como 0 cm.",
                     static_cast<double>(self->volts_));
            open_confirm("¿Hacer cero del nivel?", body, "Sí, hacer cero", [] {
                if (actions().level_zeroed != nullptr) actions().level_zeroed();
            });
            break;
        case sensor:
            snprintf(hint, sizeof hint, "Ahorita: %u cm", static_cast<unsigned>(self->sensor_cm_));
            open_keypad({"Altura del sensor", "Del fondo", "cm", 0, 2, hint, nullptr, nullptr}, [](uint32_t cm) {
                if (actions().level_sensor_set != nullptr) actions().level_sensor_set(cm);
            });
            break;
        case intake:
            snprintf(hint, sizeof hint, "Ahorita: %u cm", static_cast<unsigned>(self->intake_cm_));
            open_keypad({"Toma de la bomba de pozo", "Del fondo", "cm", 0, 3, hint, nullptr, nullptr},
                        [](uint32_t cm) {
                            if (actions().level_intake_set != nullptr) actions().level_intake_set(cm);
                        });
            break;
        case full:
            snprintf(body, sizeof body, "Tócalo con el flotador cerrado: los %ld cm de ahorita quedan como el 100 %%.",
                     std::lround(self->cm_));
            open_confirm("¿Ya está llena?", body, "Sí, está llena", [] {
                if (actions().level_full_marked != nullptr) actions().level_full_marked();
            });
            break;
        case liters:
            show(Screen::level_liters);
            break;
        case kRows:
            break;
    }
}

}  // namespace ui
