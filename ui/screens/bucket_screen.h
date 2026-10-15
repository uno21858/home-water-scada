#pragma once
// Prueba de cubeta (desde Ajustes y Sensor F): se llena una cubeta de 10 L
// por V4 mientras F cuenta pulsos; el factor nuevo es pulsos / 10.
// A la izquierda los 4 pasos; a la derecha lo del paso de ahorita. Los pasos
// 1 y 2 se confirman con su botón y el conteo empieza en 0 hasta el 3: si
// V2 sigue abierta, F contaría agua que no cae en la cubeta.

#include <array>
#include <cstdint>

#include "lvgl.h"
#include "system_state.h"

namespace ui {

class BucketScreen {
public:
    lv_obj_t* create(lv_obj_t* parent);  // regresa el contenedor de la pantalla
    void update(const core::SystemState& s);
    void enter();  // al llegar a la pantalla: vuelve al paso 1

private:
    enum class Phase : uint8_t { close_v2, place_bucket, counting, result, saved };

    struct Step {
        lv_obj_t* obj = nullptr;
        lv_obj_t* done = nullptr;    // palomita
        lv_obj_t* number = nullptr;  // círculo con el número
        lv_obj_t* text = nullptr;
    };

    void create_steps(lv_obj_t* page);
    void create_prepare(lv_obj_t* page);  // pasos 1 y 2: la instrucción y su botón
    void create_counting(lv_obj_t* page);
    void create_result(lv_obj_t* page);
    void create_saved(lv_obj_t* page);
    void set_phase(Phase phase);
    static void draw_bucket(lv_event_t* e);

    Phase phase_ = Phase::counting;
    uint32_t pulses_ = 0;         // contados ahorita, o los de cuando tocó Ya llegué
    uint16_t factor_ = 0;         // el de ahorita, para comparar
    uint16_t new_factor_ = 0;

    std::array<Step, 4> steps_{};
    lv_obj_t* prepare_ = nullptr;
    lv_obj_t* prepare_step_ = nullptr;    // "Paso 1 de 4"
    lv_obj_t* prepare_title_ = nullptr;   // "Cierra V2"
    lv_obj_t* prepare_note_ = nullptr;
    lv_obj_t* prepare_button_ = nullptr;  // el texto de "Ya la cerré"
    lv_obj_t* counting_ = nullptr;
    lv_obj_t* result_ = nullptr;
    lv_obj_t* saved_ = nullptr;

    lv_obj_t* count_ = nullptr;
    lv_obj_t* bucket_ = nullptr;
    lv_obj_t* with_factor_ = nullptr;
    lv_obj_t* liters_ = nullptr;
    float level_ = 0.0f;  // de 0 a 1: cuánto lleva la cubeta

    lv_obj_t* arrived_ = nullptr;  // "Ya llegué": apagado con 0 pulsos
    lv_obj_t* save_ = nullptr;     // "Guardar 387": apagado si el factor no es creíble
    lv_obj_t* new_k_ = nullptr;
    lv_obj_t* explain_ = nullptr;
    lv_obj_t* save_text_ = nullptr;
    lv_obj_t* saved_text_ = nullptr;
};

}  // namespace ui
