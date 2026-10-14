#pragma once
// Colores del DS "HMI Cloración" como 0xRRGGBB.
// En el comentario va el nombre del token en el DS.
// Uso: lv_color_hex(ui::colors::background)

#include <cstdint>

namespace ui::colors {

inline constexpr uint32_t background = 0x1b1f24;         // fondo
inline constexpr uint32_t panel = 0x232930;              // panel
inline constexpr uint32_t panel_raised = 0x2d343d;       // panel-alto
inline constexpr uint32_t border = 0x3a434d;             // borde
inline constexpr uint32_t border_control = 0x76818d;     // borde-control
inline constexpr uint32_t pipe = 0x6d7987;               // tuberia
inline constexpr uint32_t text = 0xe9edf1;               // texto
inline constexpr uint32_t text_secondary = 0xa9b3be;     // texto-2
inline constexpr uint32_t text_disabled = 0x7c8793;      // texto-3
inline constexpr uint32_t water = 0x3cc8ea;              // agua
inline constexpr uint32_t water_pipe = 0x1f8eab;         // agua-tubo
inline constexpr uint32_t water_dot = 0xb6f1fd;          // agua-punto
inline constexpr uint32_t ok = 0x55c993;                 // verde
inline constexpr uint32_t ok_bg = 0x16301f;              // verde-fondo
inline constexpr uint32_t warning = 0xf2a83a;            // ambar
inline constexpr uint32_t warning_bg = 0x3a2a12;         // ambar-fondo
inline constexpr uint32_t alarm = 0xff7069;              // rojo
inline constexpr uint32_t alarm_bg = 0x46181b;           // rojo-fondo
inline constexpr uint32_t alarm_screen = 0xa11f1b;       // rojo-alarma
inline constexpr uint32_t on_alarm = 0xffffff;           // blanco
inline constexpr uint32_t primary = 0xe3e8ed;            // accion
inline constexpr uint32_t primary_pressed = 0xb9c3cd;    // accion-presionado
inline constexpr uint32_t on_primary = 0x14181c;         // en-accion
inline constexpr uint32_t secondary_pressed = 0x39424d;  // secundario-presionado
inline constexpr uint32_t overlay = 0x0b0d10;            // sobrecapa
inline constexpr uint8_t overlay_opa = 204;              // sobrecapa al 80 %

}  // namespace ui::colors
