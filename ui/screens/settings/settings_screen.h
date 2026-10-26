#pragma once
// Pantalla Ajustes: cinco categorías a la izquierda (Sensores, Dosis,
// Conexión, Cartuchos y Pantalla) y dos tarjetas de la elegida a la derecha.
// Lo que se cambia aquí se escribe con el teclado o se elige con un selector,
// y le llega a la app por actions().

#include <array>
#include <cstdint>

#include "components/segmented.h"
#include "lvgl.h"
#include "system_state.h"

namespace ui {

class SettingsScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);

private:
    enum class Category : uint8_t { sensors, dose, connection, cartridges, display };

    // Botón de una categoría: ícono, nombre y un resumen abajo
    struct Tab {
        lv_obj_t* obj = nullptr;
        lv_obj_t* icon = nullptr;
        lv_obj_t* summary = nullptr;
    };

    void create_categories(lv_obj_t* page);
    lv_obj_t* create_sensors(lv_obj_t* page);  // cada una regresa su grupo de tarjetas
    lv_obj_t* create_dose(lv_obj_t* page);
    lv_obj_t* create_connection(lv_obj_t* page);
    lv_obj_t* create_cartridges(lv_obj_t* page);
    lv_obj_t* create_display(lv_obj_t* page);
    void select(Category category);

    static void handle_category(lv_event_t* e);
    static void handle_factor(lv_event_t* e);
    static void handle_ratio(lv_event_t* e);
    static void handle_cap(lv_event_t* e);
    static void handle_reference(lv_event_t* e);  // Cartuchos nuevos: guardar referencia

    std::array<Tab, 5> tabs_{};
    std::array<lv_obj_t*, 5> groups_{};
    Category current_ = Category::sensors;

    // Sensores
    lv_obj_t* factor_ = nullptr;
    lv_obj_t* pressure_live_ = nullptr;
    lv_obj_t* p1_raw_ = nullptr;
    lv_obj_t* p2_raw_ = nullptr;
    lv_obj_t* zero_button_ = nullptr;  // sin señal guardaría un cero falso
    lv_obj_t* zero_note_ = nullptr;    // cómo hacer cero, o qué sensor no tiene señal

    // Dosis
    lv_obj_t* ratio_ = nullptr;
    lv_obj_t* ratio_note_ = nullptr;
    lv_obj_t* cap_ = nullptr;
    lv_obj_t* cap_card_ = nullptr;

    // Conexión
    lv_obj_t* ssid_ = nullptr;
    lv_obj_t* signal_ = nullptr;
    lv_obj_t* upload_icon_ = nullptr;
    lv_obj_t* upload_word_ = nullptr;
    lv_obj_t* upload_note_ = nullptr;

    // Cartuchos
    lv_obj_t* spares_pp_ = nullptr;
    lv_obj_t* ref_value_ = nullptr;   // R0, o "Sin referencia"
    lv_obj_t* ref_note_ = nullptr;    // de cuándo es, o qué falta para guardarla
    lv_obj_t* ref_button_ = nullptr;  // sin flujo estable no se deja
    float ref_r_ = 0.0f;              // el R de ahorita, para el diálogo
    float ref_lpm_ = 0.0f;
    lv_obj_t* spares_rest_ = nullptr;

    // Pantalla
    Segmented idle_;

    // Lo de ahorita, para la pista del teclado
    uint16_t factor_value_ = 0;
    uint16_t ratio_value_ = 0;  // mL por litro x 1000
    uint16_t cap_value_ = 0;
};

}  // namespace ui
