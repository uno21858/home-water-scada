#include "screens/diagnostics_screen.h"

#include <cctype>
#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "ui.h"

// Medidas tomadas del artboard Diagnostico del canvas (cada y menos 64).

namespace ui {
namespace {

// Tarjeta con título y una tabla de nombre (texto-2) y valor (a la derecha)
lv_obj_t* table_card(lv_obj_t* page, int32_t x, int32_t y, int32_t h, const char* title) {
    lv_obj_t* c = card(page, x, y, 490, h);
    lv_obj_set_style_pad_ver(c, 6, 0);
    lv_obj_set_style_pad_hor(c, 16, 0);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    label(c, &font_body_32, colors::text, title);
    lv_obj_set_style_margin_bottom(lv_obj_get_child(c, 0), 4, 0);
    return c;
}

// Un renglón: regresa el valor, por si cambia
lv_obj_t* entry(lv_obj_t* c, const char* name, const char* value, uint32_t tone = colors::text) {
    lv_obj_t* r = box(c);
    lv_obj_set_size(r, lv_pct(100), 32);
    lv_obj_t* n = label(r, &font_label_26, colors::text_secondary, name);
    lv_obj_align(n, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t* v = label(r, &font_label_26, tone, value);
    lv_obj_align(v, LV_ALIGN_RIGHT_MID, 0, 0);
    return v;
}

// "Responde" en verde o "No responde" en rojo
void device(lv_obj_t* value, bool ok) {
    set_text(value, ok ? "Responde" : "No responde");
    set_text_color(value, ok ? colors::ok : colors::alarm);
}

// Lo de una fase que todavía no se instala: "fase 2" en texto-3
void future(lv_obj_t* value, const char* text) {
    set_text(value, text);
    set_text_color(value, colors::text_disabled);
}

}  // namespace

lv_obj_t* DiagnosticsScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);

    // El canvas es de la 5B (DI, DO y el EZO en el I2C); esto es la 7B del
    // documento de proyecto: F en el GPIO6, la bomba R en el GPIO44 y la
    // sonda ORP por RS485
    lv_obj_t* in = table_card(page, 16, 12, 216, "Entradas");
    flow_ = entry(in, "GPIO6 · flujo F", "");
    a0_ = entry(in, "A0 · presión P1", "");
    a1_ = entry(in, "A1 · presión P2", "");
    a3_ = entry(in, "", "");
    a3_name_ = lv_obj_get_child(lv_obj_get_parent(a3_), 0);
    a2_ = entry(in, "A2 · nivel", "");  // el sensor de nivel 4-20 mA, o libre

    lv_obj_t* bus = table_card(page, 16, 240, 184, "Bus I2C");
    gt911_ = entry(bus, "0x5D · táctil GT911", "");
    expander_ = entry(bus, "0x24 · CH32V003", "");
    ads_ = entry(bus, "0x48 · ADS1115", "");
    ina_ = entry(bus, "0x40 · INA219", "");

    lv_obj_t* out = table_card(page, 518, 12, 152, "Salidas y RS485");
    mixer_ = entry(out, "GPIO44 · bomba R", "");
    pumps_ = entry(out, "0x20 · bombas A y B", "");
    orp_ = entry(out, "RS485 · sonda ORP", "");

    lv_obj_t* net = table_card(page, 518, 176, 152, "Red y datos");
    wifi_ = entry(net, "", "");
    wifi_name_ = lv_obj_get_child(lv_obj_get_parent(wifi_), 0);
    server_ = entry(net, "Servidor de la casa", "");
    pending_ = entry(net, "Por enviar", "");

    // Probar alarma: la app prende una alarma de prueba. Sin el ícono del
    // canvas: con él no cabe en 239 px (en el canvas se sale del botón)
    lv_obj_t* test = button_secondary(page, 518, 340, 239, "Probar alarma", [](lv_event_t*) {
        if (actions().alarm_test != nullptr) actions().alarm_test();
    }, nullptr);
    lv_obj_set_height(test, 64);

    lv_obj_t* restart = button_secondary(page, 769, 340, 239, "Reiniciar", [](lv_event_t*) {
        open_confirm("¿Reiniciar la pantalla?", "Se apaga y se prende sola. Lo que no se ha enviado queda guardado.",
                     "Sí, reiniciar", [] {
                         if (actions().restart_requested != nullptr) actions().restart_requested();
                     });
    }, nullptr);
    lv_obj_set_height(restart, 64);

    about_ = label(page, &font_label_26, colors::text_disabled, "");
    lv_obj_set_pos(about_, 518, 412);
    return page;
}

void DiagnosticsScreen::update(const core::SystemState& s) {
    char buf[64];
    const float hz = (s.flow_lpm >= 0.05f ? s.flow_lpm : 0.0f) * static_cast<float>(s.flow_pulses_per_liter) / 60.0f;
    snprintf(buf, sizeof buf, "%.1f Hz", static_cast<double>(hz));
    set_text(flow_, buf);
    // P1 y P2 entran por el ADS1115, que llega en la fase 1.5
    if (s.has_pressure) {
        snprintf(buf, sizeof buf, "%.3f V", static_cast<double>(s.p1_volts));
        set_text(a0_, buf);
        set_text_color(a0_, colors::text);
        snprintf(buf, sizeof buf, "%.3f V", static_cast<double>(s.p2_volts));
        set_text(a1_, buf);
        set_text_color(a1_, colors::text);
    } else {
        future(a0_, "fase 1.5");
        future(a1_, "fase 1.5");
    }
    // El ADS1115 llega con P1 y P2, o antes si ya está el LDR o el nivel
    if (s.has_pressure || s.has_ldr || s.has_level) {
        device(ads_, s.devices.ads1115);
    } else {
        future(ads_, "fase 1.5");
    }
    set_text(a3_name_, s.has_ldr ? "A3 · luz (LDR)" : "A3");
    if (s.has_ldr) {
        snprintf(buf, sizeof buf, "%.3f V", static_cast<double>(s.ldr_volts));
        set_text(a3_, buf);
        set_text_color(a3_, colors::text);
    } else {
        future(a3_, "libre");
    }
    if (s.has_level) {
        snprintf(buf, sizeof buf, "%.3f V", static_cast<double>(s.level_volts));
        set_text(a2_, buf);
        set_text_color(a2_, colors::text);
    } else {
        future(a2_, "libre");
    }
    device(gt911_, s.devices.gt911);
    device(expander_, s.devices.expander);
    device(ina_, s.devices.ina219);

    // Bomba R con lo que lee el INA219: la corriente si anda (que pase
    // corriente es que está prendida), el aviso que dejó o "sin 12 V" aunque
    // esté apagada (0 V es apagón o fusible), en ámbar y en minúscula como los
    // demás. "prendida · 1.58 A" no cabe junto al nombre.
    const bool no_power = s.mixer_volts < 1.0f;
    if (!s.has_mixer) {
        future(mixer_, "sin bomba");
    } else if (s.mixer_fault != core::MixerFault::none || no_power) {
        const auto fault = no_power ? core::MixerFault::no_power : s.mixer_fault;
        snprintf(buf, sizeof buf, "%s", fmt::mixer_fault(static_cast<unsigned>(fault)));
        buf[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(buf[0])));
        set_text(mixer_, buf);
        set_text_color(mixer_, colors::warning);
    } else {
        if (s.mixer_on) {
            snprintf(buf, sizeof buf, "%.2f A", static_cast<double>(s.mixer_amps));
        } else {
            snprintf(buf, sizeof buf, "apagada");
        }
        set_text(mixer_, buf);
        set_text_color(mixer_, colors::text);
    }
    if (s.has_pumps) {
        device(pumps_, s.devices.pcf8574);
    } else {
        future(pumps_, "fase 3");
    }
    if (s.has_orp) {
        device(orp_, s.devices.ezo_orp);
    } else {
        future(orp_, "fase 2");
    }

    snprintf(buf, sizeof buf, "WiFi %s", s.wifi_ssid[0] != '\0' ? s.wifi_ssid : "sin configurar");
    set_text(wifi_name_, buf);
    const char* const strength[4] = {"sin señal", "señal débil", "señal regular", "señal buena"};
    set_text(wifi_, s.wifi_ok ? strength[s.wifi_bars % 4] : "sin conexión");
    set_text_color(wifi_, s.wifi_ok ? colors::text : colors::warning);
    const bool up_to_date = s.wifi_ok && s.pending_uploads == 0;
    set_text(server_, up_to_date ? "al día" : "pendiente");
    set_text_color(server_, up_to_date ? colors::ok : colors::warning);
    snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(s.pending_uploads));
    set_text(pending_, buf);

    // "encendida hace 3 días", "hace 5 h" o "hace 12 min"
    const uint32_t m = s.uptime_minutes;
    char since[24];
    if (m >= 24 * 60) {
        const uint32_t days = m / (24 * 60);
        snprintf(since, sizeof since, days == 1 ? "1 día" : "%lu días", static_cast<unsigned long>(days));
    } else if (m >= 60) {
        snprintf(since, sizeof since, "%lu h", static_cast<unsigned long>(m / 60));
    } else {
        snprintf(since, sizeof since, "%lu min", static_cast<unsigned long>(m));
    }
    snprintf(buf, sizeof buf, "Firmware %s · encendida hace %s", s.firmware[0] != '\0' ? s.firmware : "?", since);
    set_text(about_, buf);
}

}  // namespace ui
