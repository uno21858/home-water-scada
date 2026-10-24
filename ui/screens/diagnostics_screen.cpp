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

// Una entrada del ADS1115: con su sensor, los volts; sin él pero con el
// ADS1115 contestando, los volts en texto-3 (se ven mientras se conecta, y
// a los 5 s se detecta: core/detect.h); sin ADS1115, lo de la fase
void analog(lv_obj_t* value, bool installed, bool ads_ok, float volts, const char* missing) {
    if (!installed && !ads_ok) {
        future(value, missing);
        return;
    }
    char buf[16];
    snprintf(buf, sizeof buf, "%.3f V", static_cast<double>(volts));
    set_text(value, buf);
    set_text_color(value, installed ? colors::text : colors::text_disabled);
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
    // El nombre de la red puede tener 32 letras ("INFINITUM1234_2.4GHz" ya
    // se encimaba con la señal): se corta con "..." antes de "señal regular"
    lv_obj_set_size(wifi_name_, 268, lv_font_get_line_height(&font_label_26));
    lv_label_set_long_mode(wifi_name_, LV_LABEL_LONG_MODE_DOTS);
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

    // El canvas dice "Firmware 0.1 · encendida hace 3 días"; con la versión
    // de git ("30c340c+") ya no cabe y va sin "Firmware" (update). Mide 500:
    // la del canvas ocupa 499 y se pasa 9 px del margen. Si aun así no cabe,
    // se corta con "..." en vez de salirse de la pantalla.
    about_ = label(page, &font_label_26, colors::text_disabled, "");
    lv_obj_set_pos(about_, 518, 412);
    lv_obj_set_size(about_, 500, lv_font_get_line_height(&font_label_26));
    lv_label_set_long_mode(about_, LV_LABEL_LONG_MODE_DOTS);
    return page;
}

void DiagnosticsScreen::update(const core::SystemState& s) {
    char buf[64];
    const float hz = (s.flow_lpm >= 0.05f ? s.flow_lpm : 0.0f) * static_cast<float>(s.flow_pulses_per_liter) / 60.0f;
    snprintf(buf, sizeof buf, "%.1f Hz", static_cast<double>(hz));
    set_text(flow_, buf);
    // P1 y P2 entran por el ADS1115, que llega en la fase 1.5
    const bool ads = s.devices.ads1115;
    analog(a0_, s.has_pressure, ads, s.p1_volts, "fase 1.5");
    analog(a1_, s.has_pressure, ads, s.p2_volts, "fase 1.5");
    // El ADS1115 llega con P1 y P2, o antes si ya está el LDR o el nivel
    if (s.has_pressure || s.has_ldr || s.has_level || ads) {
        device(ads_, ads);
    } else {
        future(ads_, "fase 1.5");
    }
    set_text(a3_name_, s.has_ldr ? "A3 · luz (LDR)" : "A3");
    analog(a3_, s.has_ldr, ads, s.ldr_volts, "libre");
    analog(a2_, s.has_level, ads, s.level_volts, "libre");
    device(gt911_, s.devices.gt911);
    device(expander_, s.devices.expander);
    // El INA219 vigila la bomba R: sin bomba no está conectado
    if (s.has_mixer) {
        device(ina_, s.devices.ina219);
    } else {
        future(ina_, "sin bomba");
    }

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
    // Sin la URL de VictoriaMetrics no hay a dónde mandar: no es un pendiente
    const bool up_to_date = s.wifi_ok && s.pending_uploads == 0;
    if (!s.has_server) {
        future(server_, "sin configurar");
    } else {
        set_text(server_, up_to_date ? "al día" : "pendiente");
        set_text_color(server_, up_to_date ? colors::ok : colors::warning);
    }
    fmt::thousands(buf, sizeof buf, s.pending_uploads);
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
    const char* version = s.firmware[0] != '\0' ? s.firmware : "?";
    snprintf(buf, sizeof buf, "Firmware %s · encendida hace %s", version, since);
    lv_point_t size;
    lv_text_get_size(&size, buf, &font_label_26, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    if (size.x > lv_obj_get_width(about_)) {
        snprintf(buf, sizeof buf, "%s · encendida hace %s", version, since);
    }
    set_text(about_, buf);
}

}  // namespace ui
