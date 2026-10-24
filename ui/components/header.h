#pragma once
// Encabezado de 1024 x 64. A la izquierda: botón atrás (en las pantallas de
// detalle), título, un enlace a otra pantalla ("Bomba R") y el "?" del Tren.
// A la derecha: aviso de sin red, pastilla de estado, WiFi y hora. Se crea una
// vez; al cambiar de pantalla solo se esconden o se muestran sus piezas.

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
    const char* link_text = nullptr;  // botón a otra pantalla: "Bomba R"
    std::optional<Screen> link;
    std::optional<Screen> help;       // "?" a la Leyenda (solo en el Tren)
};

class Header {
public:
    void create(lv_obj_t* parent);
    void set_page(const HeaderSpec& spec);
    // Pastilla: OK, Advertencia, Alarma o Mantenimiento (cambio de cartuchos)
    void set_status(const core::SystemState& s);
    void set_time(uint8_t hour, uint8_t minute);
    // Sin red sale el chip ámbar "Sin red · 24 por enviar" y el enlace se
    // esconde para que quepa (así lo hace Flujo en el canvas). Con red, el
    // WiFi lleva 3, 2 o 1 arcos según la señal (bars, como Diagnóstico).
    void set_network(bool ok, uint8_t bars, uint32_t pending);

private:
    static void handle_back(lv_event_t* e);
    static void handle_link(lv_event_t* e);
    static void handle_help(lv_event_t* e);

    lv_obj_t* left_ = nullptr;
    lv_obj_t* back_ = nullptr;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* link_ = nullptr;
    lv_obj_t* link_text_ = nullptr;
    Screen back_target_ = Screen::home;
    Screen link_target_ = Screen::home;
    lv_obj_t* help_ = nullptr;
    Screen help_target_ = Screen::home;
    bool has_link_ = false;

    lv_obj_t* offline_ = nullptr;
    lv_obj_t* offline_text_ = nullptr;

    lv_obj_t* pill_ = nullptr;
    lv_obj_t* pill_icon_ = nullptr;
    lv_obj_t* pill_text_ = nullptr;
    lv_obj_t* wifi_ = nullptr;
    lv_obj_t* clock_ = nullptr;
    int level_ = -1;    // PillLook::Kind de lo pintado; -1 = todavía no se pinta
    int wifi_ok_ = -1;
    int wifi_icon_ = -1;  // arcos pintados (1 a 3) o 0 tachado; -1 = todavía no se pinta
};

}  // namespace ui
