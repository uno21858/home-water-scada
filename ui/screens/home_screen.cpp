#include "screens/home_screen.h"

#include <algorithm>
#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "theme/pid.h"
#include "theme/styles.h"
#include "ui.h"

// Medidas tomadas del artboard "1 · Tren de filtros" del canvas. El canvas
// cuenta y desde arriba de la pantalla; aquí el contenido empieza en y 64,
// así que cada y es la del canvas menos 64.

namespace ui {
namespace {

using Dir = FlowPipes::Dir;

lv_obj_t* place(lv_obj_t* obj, int32_t x, int32_t y) {
    lv_obj_set_pos(obj, x, y);
    return obj;
}

// Símbolo del P&ID (válvula, medidor, embudo...) en su lugar
lv_obj_t* symbol(lv_obj_t* page, const lv_image_dsc_t* src, int32_t x, int32_t y) {
    lv_obj_t* img = lv_image_create(page);
    lv_image_set_src(img, src);
    return place(img, x, y);
}

// Texto chico del diagrama (V1, PRF, 20 µm...). w > 0: centrado en ese ancho.
lv_obj_t* tag_text(lv_obj_t* page, const char* text, int32_t x, int32_t y, int32_t w = 0,
                   lv_text_align_t align = LV_TEXT_ALIGN_CENTER, uint32_t hex = colors::text_secondary) {
    lv_obj_t* l = place(label(page, &font_label_26, hex, text), x, y);
    if (w > 0) {
        lv_obj_set_width(l, w);
        lv_obj_set_style_text_align(l, align, 0);
    }
    return l;
}

// Recuadro con borde (vasos, cabezas, medidor de flujo)
lv_obj_t* frame(lv_obj_t* page, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t bg, uint32_t border,
                int32_t radius) {
    lv_obj_t* b = box(page, x, y, w, h);
    fill(b, bg);
    lv_obj_set_style_border_color(b, color(border), 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_set_style_radius(b, radius, 0);
    return b;
}

// Toma de presión: circulito de 16 px
void tap(lv_obj_t* page, int32_t x, int32_t y) {
    frame(page, x, y, 16, 16, colors::background, colors::text, LV_RADIUS_CIRCLE);
}

// Barra de rango de P1 y P2: de 0 a 80 psi
int32_t range_x(int psi, int32_t width) { return (std::clamp(psi, 0, 80) * width + 40) / 80; }

// ORP en la barra de 3 zonas: cada zona tiene su escala
//   -300 a -50 Reductor, -50 a 650 Neutro, 650 a 900 Oxidante
int32_t orp_x(int mv) {
    mv = std::clamp(mv, -300, 900);
    if (mv < -50) return (mv + 300) * 45 / 250;
    if (mv < 650) return 47 + (mv + 50) * 126 / 700;
    return 175 + (mv - 650) * 41 / 250;
}

void rotate(void* obj, int32_t angle) { lv_image_set_rotation(static_cast<lv_obj_t*>(obj), angle); }

}  // namespace

lv_obj_t* HomeScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    // El orden importa: lo que se crea después queda encima
    create_pipes(page);
    create_inlet(page);
    create_housings(page);
    create_return(page);
    create_cistern(page);
    create_tags(page);
    create_strip(page);
    return page;
}

void HomeScreen::create_pipes(lv_obj_t* page) {
    pipes_.add(page, 60, 287, 157, 10, Dir::right);   // entrada desde el medidor
    pipes_.add(page, 207, 65, 10, 232, Dir::up);      // subida
    pipes_.add(page, 207, 65, 790, 10, Dir::right);   // eje del tren
    pipes_.add(page, 987, 65, 10, 115, Dir::down);    // bajada derecha
    pipes_.add(page, 243, 170, 754, 10, Dir::left);   // retorno bajo los vasos
    pipes_.add(page, 243, 170, 10, 195, Dir::down);   // bajada final
    pipes_.add(page, 243, 355, 141, 10, Dir::right);  // cobre bajo el piso a la cisterna

    // Ramales sin flujo: boca B y descarga de V4
    fill(box(page, 253, 241, 99, 10), colors::pipe);
    fill(box(page, 253, 293, 43, 10), colors::pipe);
}

void HomeScreen::create_inlet(lv_obj_t* page) {
    tag_text(page, "SIAPA", 16, 240);
    symbol(page, &pid_medidor, 16, 276);

    // Prefiltro PRF. El cuerpo tiene esquinas redondas solo abajo: se hace
    // 10 px más alto hacia arriba y la cabeza tapa las esquinas de arriba.
    tag_text(page, "PRF", 16, 310, 70, LV_TEXT_ALIGN_RIGHT);
    frame(page, 94, 289, 40, 58, colors::panel, colors::border_control, 10);
    symbol(page, &pid_malla, 102, 307);
    frame(page, 88, 283, 52, 18, colors::panel_raised, colors::border_control, 4);
    symbol(page, &pid_purga, 104, 347);

    symbol(page, &pid_valvula_v1, 160, 269);
    tag_text(page, "V1", 155, 308, 38);

    // Toma P1 en la subida
    fill(box(page, 188, 125, 16, 2), colors::pipe);
    tap(page, 204, 118);
}

void HomeScreen::create_housings(lv_obj_t* page) {
    const char* const names[5] = {"20 µm", "10 µm", "5 µm", "CTO", "Sarro"};
    for (int i = 0; i < 5; ++i) {
        const int32_t x = 294 + i * 140;
        const bool empty = i == 4;  // el vaso del antisarro todavía no lleva cartucho
        tag_text(page, names[i], x - 29, 20, 114, LV_TEXT_ALIGN_CENTER,
                 empty ? colors::text_disabled : colors::text_secondary);

        // Mismo truco que el PRF: 12 px más alto, la cabeza tapa las esquinas
        frame(page, x, 66, 56, 76, colors::panel, colors::border_control, 12);
        lv_obj_t* cartridge = box(page, x + 16, 88, 24, 44);
        lv_obj_set_style_radius(cartridge, 4, 0);
        if (empty) {
            lv_obj_set_style_border_color(cartridge, color(colors::border), 0);
            lv_obj_set_style_border_width(cartridge, 2, 0);
        } else {
            fill(cartridge, colors::panel_raised);
        }
        frame(page, x - 8, 60, 72, 20, colors::panel_raised, colors::border_control, 4);
    }
}

void HomeScreen::create_return(lv_obj_t* page) {
    // Medidor de flujo F en el retorno
    frame(page, 872, 161, 44, 28, colors::background, colors::text, 6);
    symbol(page, &pid_turbina, 880, 167);
    fill(box(page, 893, 189, 2, 7), colors::pipe);

    // Toma P2
    tap(page, 692, 167);
    fill(box(page, 699, 183, 2, 13), colors::pipe);

    // Bajada final: V5, boca B con V3, descarga V4 y V2 antes de la cisterna
    symbol(page, &pid_valvula_v, 238, 190);
    tag_text(page, "V5", 262, 186, 40, LV_TEXT_ALIGN_LEFT);
    symbol(page, &pid_embudo, 338, 218);
    symbol(page, &pid_valvula_h, 300, 236);
    tag_text(page, "V3", 293, 256, 40);  // en el canvas mide 34, pero "V3" mide 35 y LVGL lo partiría
    symbol(page, &pid_valvula_h, 262, 288);
    symbol(page, &pid_espiga, 296, 290);
    tag_text(page, "V4", 318, 282, 40, LV_TEXT_ALIGN_LEFT);
    symbol(page, &pid_valvula_v, 238, 322);
    tag_text(page, "V2", 262, 318, 40, LV_TEXT_ALIGN_LEFT);
}

void HomeScreen::create_cistern(lv_obj_t* page) {
    lv_obj_t* cistern = frame(page, 384, 284, 624, 92, colors::panel, colors::border_control, 10);
    link_to(cistern, Screen::cistern);
    fill(box(page, 386, 294, 620, 2), colors::water);  // nivel del agua

    tag_text(page, "Cisterna", 396, 304);
    cistern_word_ = tag_text(page, "", 396, 332);

    // Mástil con la sonda ORP y la bomba R abajo
    fill(box(page, 539, 276, 3, 72), colors::text_secondary);
    lv_obj_t* probe = box(page, 535, 308, 11, 20);
    lv_obj_set_style_radius(probe, 3, 0);
    fill(probe, colors::text_secondary);
    pump_ring_ = frame(page, 528, 347, 25, 25, colors::background, colors::text_secondary, LV_RADIUS_CIRCLE);
    pump_rotor_ = place(icon(page, &icon_impulsor_chico_26, colors::pipe), 527, 346);
}

void HomeScreen::create_tags(lv_obj_t* page) {
    // P1 y P2: barra de 0 a 80 psi con la banda normal de 20 a 50.
    // Bloqueadas dicen "1.5" y no "fase 1.5": "P1 + candado + fase 1.5"
    // mide 180 px y no cabe en 172 (en el canvas se sale de la etiqueta).
    auto pressure_tag = [page](SensorTag& tag, lv_obj_t*& marker, int32_t x, int32_t y, const char* name) {
        tag = sensor_tag(page, x, y, 172, name, "psi", "1.5");
        lv_obj_set_style_radius(tag_bar(tag, 0, tag.bar_w, colors::border), 2, 0);
        tag_bar(tag, range_x(20, tag.bar_w), range_x(50, tag.bar_w) - range_x(20, tag.bar_w), colors::pipe);
        marker = tag_marker(tag);
    };
    pressure_tag(p1_, p1_marker_, 16, 98, "P1");

    // B: la boca donde se echa el cloro a mano
    lv_obj_t* b = frame(page, 378, 196, 168, 56, colors::panel_raised, colors::border, 6);
    link_to(b, Screen::dose, &styles::button_secondary_pressed);
    lv_obj_t* b_row = row(b, 10, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(b_row, LV_ALIGN_LEFT_MID, 12, 0);
    label(b_row, &font_label_26, colors::text_secondary, "B");
    label(b_row, &font_body_32, colors::text, "Manual");

    pressure_tag(p2_, p2_marker_, 584, 196, "P2");

    // F: barra que se llena de 0 a 30 L/min
    f_ = sensor_tag(page, 772, 196, 236, "F", "L/min", "");
    link_to(f_.live, Screen::flow, &styles::button_secondary_pressed);
    lv_obj_set_style_radius(tag_bar(f_, 0, f_.bar_w, colors::border), 2, 0);
    f_fill_ = tag_bar(f_, 0, 0, colors::water);
    lv_obj_set_style_radius(f_fill_, 2, 0);

    // ORP: tres zonas; la de ahorita va en su color fuerte
    orp_ = sensor_tag(page, 566, 302, 244, "ORP", "mV", "fase 2");
    link_to(orp_.live, Screen::cistern, &styles::button_secondary_pressed);
    orp_zones_[0] = tag_bar(orp_, 0, 45, colors::alarm_bg);
    orp_zones_[1] = tag_bar(orp_, 47, 126, colors::warning_bg);
    orp_zones_[2] = tag_bar(orp_, 175, 41, colors::ok_bg);
    orp_marker_ = tag_marker(orp_);

    // R: la bomba que revuelve la cisterna
    r_live_ = frame(page, 822, 302, 176, 56, colors::panel_raised, colors::border, 6);
    link_to(r_live_, Screen::mixer, &styles::button_secondary_pressed);
    lv_obj_t* r_row = row(r_live_, 8, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(r_row, LV_ALIGN_LEFT_MID, 10, 0);
    label(r_row, &font_label_26, colors::text_secondary, "R");
    r_state_ = label(r_row, &font_label_26, colors::text, "");

    r_locked_ = frame(page, 822, 302, 176, 56, colors::background, colors::border, 6);
    lv_obj_t* r_lock = row(r_locked_, 8, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(r_lock, LV_ALIGN_LEFT_MID, 10, 0);
    label(r_lock, &font_label_26, colors::text_disabled, "R");
    icon(r_lock, &icon_candado_24, colors::text_disabled);
    label(r_lock, &font_label_26, colors::text_disabled, "fase 2");
}

void HomeScreen::create_strip(lv_obj_t* page) {
    // Caída del tren: el color y la palabra dicen si está bien
    drop_live_ = box(page, 16, 388, 360, 56);
    lv_obj_set_style_radius(drop_live_, 10, 0);
    lv_obj_set_style_border_width(drop_live_, 2, 0);
    lv_obj_set_style_bg_opa(drop_live_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(drop_live_, 16, 0);
    lv_obj_set_flex_flow(drop_live_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(drop_live_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(drop_live_, 8, 0);
    lv_obj_set_flex_grow(label(drop_live_, &font_label_26, colors::text_secondary, "Caída"), 1);
    drop_value_ = label(drop_live_, &font_digits_44, colors::text, "");
    label(drop_live_, &font_label_26, colors::text_secondary, "psi");
    drop_word_ = label(drop_live_, &font_label_26, colors::ok, "");
    lv_obj_set_style_margin_left(drop_word_, 8, 0);

    drop_locked_ = frame(page, 16, 388, 360, 56, colors::background, colors::border, 10);
    lv_obj_t* lock = row(drop_locked_, 10, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(lock, LV_ALIGN_LEFT_MID, 14, 0);
    icon(lock, &icon_candado_28, colors::text_disabled);
    label(lock, &font_label_26, colors::text_disabled, "Caída · fase 1.5");

    // Litros de hoy
    lv_obj_t* today = card(page, 388, 388, 264, 56);
    lv_obj_set_style_pad_hor(today, 16, 0);
    lv_obj_set_flex_flow(today, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(today, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(today, 8, 0);
    lv_obj_set_flex_grow(label(today, &font_label_26, colors::text_secondary, "Hoy"), 1);
    today_value_ = label(today, &font_digits_44, colors::text, "");
    label(today, &font_label_26, colors::text_secondary, "L");

    // Toca echar: lleva a Dosis
    lv_obj_t* due = frame(page, 664, 388, 344, 56, colors::panel_raised, colors::border_control, 10);
    link_to(due, Screen::dose, &styles::button_secondary_pressed);
    lv_obj_set_style_pad_left(due, 16, 0);
    lv_obj_set_style_pad_right(due, 12, 0);
    lv_obj_set_flex_flow(due, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(due, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(due, 8, 0);
    lv_obj_set_flex_grow(label(due, &font_label_26, colors::text_secondary, "Toca echar"), 1);
    dose_value_ = label(due, &font_digits_44, colors::text, "");
    label(due, &font_label_26, colors::text_secondary, "mL");
    lv_obj_set_style_margin_left(icon(due, &icon_chevron_32, colors::text_secondary), 4, 0);
}

void HomeScreen::update(const core::SystemState& s) {
    char buf[16];

    // Flujo: tubería con puntos, cifra F y su barra
    const bool flowing = s.flow_lpm >= 0.05f;
    pipes_.set_flow(flowing ? s.flow_lpm : 0.0f);
    fmt::one_decimal(buf, sizeof buf, flowing ? s.flow_lpm : 0.0f);
    set_text(f_.value, buf);
    if (static_cast<int>(flowing) != flowing_) {
        flowing_ = flowing;
        lv_obj_set_style_text_color(f_.value, color(flowing ? colors::text : colors::text_disabled), 0);
    }
    const float fraction = std::clamp(flowing ? s.flow_lpm / 30.0f : 0.0f, 0.0f, 1.0f);
    lv_obj_set_width(f_fill_, static_cast<int32_t>(fraction * static_cast<float>(f_.bar_w)));

    update_pressure(s);
    update_orp(s);
    update_mixer(s);

    fmt::thousands(buf, sizeof buf, s.liters_today);
    set_text(today_value_, buf);
    snprintf(buf, sizeof buf, "%lu", static_cast<unsigned long>(core::dose_ml(s)));
    set_text(dose_value_, buf);
}

void HomeScreen::update_pressure(const core::SystemState& s) {
    p1_.set_installed(s.has_pressure);
    p2_.set_installed(s.has_pressure);
    lv_obj_set_hidden(drop_live_, !s.has_pressure);
    lv_obj_set_hidden(drop_locked_, s.has_pressure);
    if (!s.has_pressure) return;

    char buf[12];
    snprintf(buf, sizeof buf, "%d", s.p1_psi);
    set_text(p1_.value, buf);
    snprintf(buf, sizeof buf, "%d", s.p2_psi);
    set_text(p2_.value, buf);
    lv_obj_set_x(p1_marker_, p1_.bar_x + range_x(s.p1_psi, p1_.bar_w) - 1);
    lv_obj_set_x(p2_marker_, p2_.bar_x + range_x(s.p2_psi, p2_.bar_w) - 1);

    const int drop = s.p1_psi - s.p2_psi;
    snprintf(buf, sizeof buf, "%d", drop);
    set_text(drop_value_, buf);

    const core::Level level = core::pressure_drop_level(drop);
    if (static_cast<int>(level) == drop_level_) return;
    drop_level_ = static_cast<int>(level);
    uint32_t fg = colors::ok;
    uint32_t bg = colors::ok_bg;
    const char* word = "Normal";
    if (level == core::Level::warning) {
        fg = colors::warning;
        bg = colors::warning_bg;
        word = "Revisar";
    } else if (level == core::Level::alarm) {
        fg = colors::alarm;
        bg = colors::alarm_bg;
        word = "Tapados";
    }
    lv_obj_set_style_bg_color(drop_live_, color(bg), 0);
    lv_obj_set_style_border_color(drop_live_, color(fg), 0);
    lv_label_set_text(drop_word_, word);
    lv_obj_set_style_text_color(drop_word_, color(fg), 0);
}

void HomeScreen::update_orp(const core::SystemState& s) {
    orp_.set_installed(s.has_orp);
    if (!s.has_orp) {
        set_text(cistern_word_, "10 m³");
        lv_obj_set_style_text_color(cistern_word_, color(colors::text_disabled), 0);
        orp_level_ = -1;
        return;
    }

    char buf[12];
    snprintf(buf, sizeof buf, "%d", s.orp_mv);
    set_text(orp_.value, buf);
    lv_obj_set_x(orp_marker_, orp_.bar_x + orp_x(s.orp_mv) - 1);

    const core::Level level = core::orp_level(s.orp_mv);
    if (static_cast<int>(level) == orp_level_) return;
    orp_level_ = static_cast<int>(level);

    // Zonas apagadas en su color de fondo y la de ahorita en su color fuerte
    const uint32_t dim[3] = {colors::alarm_bg, colors::warning_bg, colors::ok_bg};
    const uint32_t lit[3] = {colors::alarm, colors::warning, colors::ok};
    const int zone = level == core::Level::alarm ? 0 : level == core::Level::warning ? 1 : 2;
    for (int i = 0; i < 3; ++i) fill(orp_zones_[i], i == zone ? lit[i] : dim[i]);

    const char* const words[3] = {"Reductor", "Neutro", "Oxidante"};
    set_text(cistern_word_, words[zone]);
    lv_obj_set_style_text_color(cistern_word_, color(lit[zone]), 0);
}

void HomeScreen::update_mixer(const core::SystemState& s) {
    lv_obj_set_hidden(r_live_, !s.has_mixer);
    lv_obj_set_hidden(r_locked_, s.has_mixer);
    if (s.has_mixer) set_text(r_state_, s.mixer_on ? "Prendida" : "Apagada");

    // 0 = no instalada, 1 = apagada, 2 = girando
    const int state = !s.has_mixer ? 0 : s.mixer_on ? 2 : 1;
    if (state == pump_state_) return;
    pump_state_ = state;

    lv_obj_set_style_border_color(pump_ring_, color(state == 0 ? colors::border : colors::text_secondary), 0);
    set_icon_color(pump_rotor_, state == 0 ? colors::border : state == 1 ? colors::pipe : colors::water);
    if (state == 2) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, pump_rotor_);
        lv_anim_set_exec_cb(&a, rotate);
        lv_anim_set_values(&a, 0, 3600);
        lv_anim_set_duration(&a, 2000);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&a);
    } else {
        lv_anim_delete(pump_rotor_, rotate);
        lv_image_set_rotation(pump_rotor_, 0);
    }
}

}  // namespace ui
