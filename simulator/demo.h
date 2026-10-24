#pragma once
// Datos de ejemplo para el simulador: los mismos del DS
// (18:42, 12.4 L/min, 1,860 L hoy, 37 mL, P1 42, P2 38, bomba R en Auto).

#include <cstdint>

#include "system_state.h"

namespace demo {

const core::SystemState& state();

// Escenarios para ver cada estado del canvas sin tocar código. Se eligen con
// la variable HMI_DEMO al correr el simulador:
//   sin-flujo  el caudal en 0 (Flujo, Resumen, Pasillo)
//   sin-red    WiFi caído con 24 lecturas por enviar (encabezado)
//   alarma     sin flujo mientras dosifica; sensor y tope, las otras dos
// Sin escenario arranca como la casa, en la fase 1.5: todo menos la sonda ORP
// y las bombas A y B.
//   completo   con la sonda ORP, como el canvas (Cisterna, ORP en Tren y Resumen)
//   fase1      solo F y la bomba R: sin P1, P2 ni LDR
//   revisar    caída 5.4 psi arriba de la de nuevos; tapados, 10.4 arriba
//   sin-agua   se fue el agua del SIAPA: sin flujo y sin presión; cisterna-vacia,
//              además la cisterna bajó del mínimo y la bomba R espera agua
//   sin-12v    la bomba R sin 12 V; nivel-bajo y atorada, los otros avisos del INA219
//   ro-cocina  el RO de la cocina por instalar (Panorama)
//   sin-senal  se soltaron los cables de P1 y del nivel: "Sin señal" en ámbar
//   actualizando  una versión nueva llegando por WiFi: sube, "Lista", y luego
//              una que no se instala (firma de otra llave); y otra vez
//   a-prueba   recién actualizada: arranca en Diagnóstico con la cuenta de la
//              prueba (30 s) y luego "quedó como buena"
// Regresa false si el nombre no existe.
bool set_scenario(const char* name);

// Avanza un segundo: el caudal se mueve un poco, suben los litros,
// y cada minuto avanzan la hora y la cuenta de la bomba R.
void tick_1s();

// Registrar una dosis: core::register_dose la guarda, prende la mezcla (si
// la bomba R no está Apagada) y el conteo de litros desde la dosis vuelve a 0
void on_dose_confirmed(uint32_t ml);  // "Sí, ya le puse"
void on_dose_typed(uint32_t ml);      // "Registrar otra cantidad"

// La tira de "Calibrar con tira": se guarda con el ORP de ahorita
void on_strip_checked(uint32_t tenths_mgl);

// Bomba R: modo elegido en Mezcla y los minutos del programa Auto
void on_mixer_mode(core::MixerMode mode);
void on_mixer_after_dose(uint32_t minutes);
void on_mixer_every_liters(uint32_t minutes);
void on_service(bool on);  // empezó o terminó el cambio de cartuchos

// Calibrar nivel
void on_level_range(uint32_t cm);
void on_level_zeroed();
void on_level_sensor(uint32_t cm);
void on_level_intake(uint32_t cm);
void on_level_full();

// Ajustes
void on_flow_factor(uint32_t pulses_per_liter);
void on_pressure_zeroed();
void on_dose_ratio(uint32_t thousandths);
void on_daily_cap(uint32_t ml);
void on_idle_minutes(uint32_t minutes);

// Cartuchos: purgó el prefiltro o cambió (o puso) un cartucho
void on_filter_serviced(core::Filter filter);

// Repuestos en casa: lo que se puso con − y +
void on_spares_set(core::Filter filter, uint32_t count);

// Alarma: tocó Entendido
void on_alarm_acknowledged(core::AlarmKind kind);

// Diagnóstico: Probar alarma prende la de sin flujo hasta que la reconozcan;
// Reiniciar no hace nada en el demo
void on_alarm_test();
void on_restart();

// Prueba de cubeta: bucket_pulses vuelve a 0 y cuenta con el caudal
void on_bucket_test_started();

}  // namespace demo
