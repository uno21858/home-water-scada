#pragma once
// Encabezado de 1024 x 64. A la izquierda: botón atrás (en las pantallas de
// detalle), título, chip ("Fase 2") y un enlace a otra pantalla ("Bomba R").
// A la derecha: pastilla de estado, WiFi y hora. Se crea una vez; al cambiar
// de pantalla solo se esconden o se muestran sus piezas.

#include <cstdint>
#include <optional>

#include "lvgl.h"
#include "system_state.h"
#include "ui.h"

namespace ui {

// Lo que cambia del encabezado de una pantalla a otra
struct HeaderSpec {
    const char* title = "";
    std::optional<Screen> back;       // botón atrás a esa pantalla
    const char* chip = nullptr;       // chip junto al título: "Fase 2"
    const char* link_text = nullptr;  // botón a otra pantalla: "Bomba R"
    std::optional<Screen> link;
};

class Header {
public:
    void create(lv_obj_t* parent);
    void set_page(const HeaderSpec& spec);
    void set_level(core::Level level);
    void set_time(uint8_t hour, uint8_t minute);
    void set_wifi(bool ok);

private:
    static void handle_back(lv_event_t* e);
    static void handle_link(lv_event_t* e);

    lv_obj_t* left_ = nullptr;
    lv_obj_t* back_ = nullptr;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* chip_ = nullptr;
    lv_obj_t* chip_text_ = nullptr;
    lv_obj_t* link_ = nullptr;
    lv_obj_t* link_text_ = nullptr;
    Screen back_target_ = Screen::home;
    Screen link_target_ = Screen::home;

    lv_obj_t* pill_ = nullptr;
    lv_obj_t* pill_icon_ = nullptr;
    lv_obj_t* pill_text_ = nullptr;
    lv_obj_t* wifi_ = nullptr;
    lv_obj_t* clock_ = nullptr;
    int level_ = -1;    // -1 = todavía no se pinta
    int wifi_ok_ = -1;
};

}  // namespace ui
