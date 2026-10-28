#include "screens/home/home_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "components/mixer.h"
#include "level.h"
#include "components/clog_look.h"
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

// Agua de la cisterna en el diagrama: llena en la raya de siempre y vacía
// en el borde de abajo de la tarjeta
constexpr int32_t kWaterTop = 294;
constexpr int32_t kWaterBottom = 374;

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
lv_obj_t* tap(lv_obj_t* page, int32_t x, int32_t y) {
    return frame(page, x, y, 16, 16, colors::background, colors::text, LV_RADIUS_CIRCLE);
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

    // Se fue el agua: el aviso va pegado a de dónde viene, con ícono y palabra
    no_supply_ = row(page, 6, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(no_supply_, 16, 200);
    icon(no_supply_, &icon_advertencia_28, colors::warning);
    label(no_supply_, &font_label_26, colors::warning, "Sin agua");
    lv_obj_set_hidden(no_supply_, true);

    // Prefiltro PRF. El cuerpo tiene esquinas redondas solo abajo: se hace
    // 10 px más alto hacia arriba y la cabeza tapa las esquinas de arriba.
    tag_text(page, "PRF", 16, 310, 70, LV_TEXT_ALIGN_RIGHT);
    frame(page, 94, 289, 40, 58, colors::panel, colors::border_control, 10);
    symbol(page, &pid_malla, 102, 307);
    frame(page, 88, 283, 52, 18, colors::panel_raised, colors::border_control, 4);
    symbol(page, &pid_purga, 104, 347);
    link_to(box(page, 94, 299, 40, 48), Screen::filters, &styles::button_secondary_pressed);

    symbol(page, &pid_valvula_v1, 160, 269);
    tag_text(page, "V1", 155, 308, 38);

    // Toma P1 en la subida (fase 1.5)
    pressure_parts_[0] = box(page, 188, 125, 16, 2);
    fill(pressure_parts_[0], colors::pipe);
    pressure_parts_[1] = tap(page, 204, 118);
}

void HomeScreen::create_housings(lv_obj_t* page) {
    const char* const names[5] = {"20 µm", "10 µm", "5 µm", "CTO", "Sarro"};
    for (int i = 0; i < 5; ++i) {
        const int32_t x = 294 + i * 140;
        // Lleno o vacío lo dice update(): el del sarro empieza sin cartucho
        housings_[i].name = tag_text(page, names[i], x - 29, 20, 114);

        // Mismo truco que el PRF: 12 px más alto, la cabeza tapa las esquinas
        frame(page, x, 66, 56, 76, colors::panel, colors::border_control, 12);
        housings_[i].cartridge = box(page, x + 16, 88, 24, 44);
        lv_obj_set_style_radius(housings_[i].cartridge, 4, 0);
        lv_obj_set_style_border_color(housings_[i].cartridge, color(colors::border), 0);
        frame(page, x - 8, 60, 72, 20, colors::panel_raised, colors::border_control, 4);

        // Zona tocable del vaso (la del canvas): abre Cartuchos
        link_to(box(page, x, 78, 56, 64), Screen::filters, &styles::button_secondary_pressed);
    }
}

void HomeScreen::create_return(lv_obj_t* page) {
    // Medidor de flujo F en el retorno
    frame(page, 872, 161, 44, 28, colors::background, colors::text, 6);
    symbol(page, &pid_turbina, 880, 167);
    fill(box(page, 893, 189, 2, 7), colors::pipe);

    // Toma P2 (fase 1.5)
    pressure_parts_[2] = tap(page, 692, 167);
    pressure_parts_[3] = box(page, 699, 183, 2, 13);
    fill(pressure_parts_[3], colors::pipe);

    // Bajada final: V2, boca B con V3, descarga V4 y V5 antes de la cisterna.
    // Al revés que el canvas, como está en la casa: V2 se cierra para abrir V3
    // y echar el cloro; V5 se cierra para purgar por V4
    symbol(page, &pid_valvula_v, 238, 190);
    tag_text(page, "V2", 262, 186, 40, LV_TEXT_ALIGN_LEFT);
    symbol(page, &pid_embudo, 338, 218);
    symbol(page, &pid_valvula_h, 300, 236);
    tag_text(page, "V3", 293, 256, 40);  // en el canvas mide 34, pero "V3" mide 35 y LVGL lo partiría
    symbol(page, &pid_valvula_h, 262, 288);
    symbol(page, &pid_espiga, 296, 290);
    tag_text(page, "V4", 318, 282, 40, LV_TEXT_ALIGN_LEFT);
    symbol(page, &pid_valvula_v, 238, 322);
    tag_text(page, "V5", 262, 318, 40, LV_TEXT_ALIGN_LEFT);
}

void HomeScreen::create_cistern(lv_obj_t* page) {
    // Abre Cisterna solo si hay qué ver ahí (sonda ORP o nivel): update() lo prende
    cistern_ = frame(page, 384, 284, 624, 92, colors::panel, colors::border_control, 10);
    link_to(cistern_, Screen::cistern);
    // El agua: con sensor de nivel sube y baja; sin lectura no se dibuja
    water_ = box(page, 386, kWaterTop, 620, kWaterBottom - kWaterTop);
    fill(water_, colors::water);
    lv_obj_set_style_bg_opa(water_, LV_OPA_20, 0);
    lv_obj_set_hidden(water_, true);
    water_line_ = box(page, 386, kWaterTop, 620, 2);
    fill(water_line_, colors::water);

    cistern_name_ = tag_text(page, "Cisterna", 396, 304);
    cistern_word_ = tag_text(page, "", 396, 332);
    // Los litros van a la derecha del palo de la bomba R (x 539): junto al %
    // se le encimaban
    cistern_liters_ = tag_text(page, "", 566, 332, 0, LV_TEXT_ALIGN_LEFT, colors::text);

    // Mástil con la sonda ORP (fase 2) y la bomba R abajo (fase 1)
    mast_ = box(page, 539, 276, 3, 72);
    fill(mast_, colors::text_secondary);
    probe_ = box(page, 535, 308, 11, 20);
    lv_obj_set_style_radius(probe_, 3, 0);
    fill(probe_, colors::text_secondary);
    pump_ring_ = frame(page, 528, 347, 25, 25, colors::background, colors::text_secondary, LV_RADIUS_CIRCLE);
    pump_rotor_ = place(icon(page, &icon_impulsor_chico_26, colors::pipe), 527, 346);
}

void HomeScreen::create_tags(lv_obj_t* page) {
    // P1 y P2: barra de 0 a 80 psi con la banda normal de 20 a 50
    auto pressure_tag = [page](SensorTag& tag, lv_obj_t*& marker, int32_t x, int32_t y, const char* name) {
        tag = sensor_tag(page, x, y, 172, name, "psi");
        link_to(tag.live, Screen::pressure, &styles::button_secondary_pressed);
        lv_obj_set_style_radius(tag_bar(tag, 0, tag.bar_w, colors::border), 2, 0);
        tag_bar(tag, range_x(20, tag.bar_w), range_x(50, tag.bar_w) - range_x(20, tag.bar_w), colors::pipe);
        marker = tag_marker(tag);
    };
    pressure_tag(p1_, p1_marker_, 16, 98, "P1");

    // B: la boca donde se echa el cloro a mano. Abre Inyección con las bombas
    // de la fase 3; antes, Dosis (donde se registra lo que se echó)
    lv_obj_t* b = frame(page, 378, 196, 168, 56, colors::panel_raised, colors::border, 6);
    lv_obj_set_clickable(b, true);
    lv_obj_add_style(b, &styles::button_secondary_pressed, LV_STATE_PRESSED);
    lv_obj_add_event_cb(
        b,
        [](lv_event_t* e) {
            const auto* self = static_cast<HomeScreen*>(lv_event_get_user_data(e));
            show(self->has_pumps_ ? Screen::injection : Screen::dose);
        },
        LV_EVENT_CLICKED, this);
    lv_obj_t* b_row = row(b, 10, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(b_row, LV_ALIGN_LEFT_MID, 12, 0);
    label(b_row, &font_label_26, colors::text_secondary, "B");
    label(b_row, &font_body_32, colors::text, "Manual");

    pressure_tag(p2_, p2_marker_, 584, 196, "P2");

    // F: barra que se llena de 0 a 30 L/min
    f_ = sensor_tag(page, 772, 196, 236, "F", "L/min");
    link_to(f_.live, Screen::flow_sensor, &styles::button_secondary_pressed);
    lv_obj_set_style_radius(tag_bar(f_, 0, f_.bar_w, colors::border), 2, 0);
    f_fill_ = tag_bar(f_, 0, 0, colors::water);
    lv_obj_set_style_radius(f_fill_, 2, 0);

    // ORP: tres zonas; la de ahorita va en su color fuerte
    orp_ = sensor_tag(page, 566, 302, 244, "ORP", "mV");
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
}

void HomeScreen::create_strip(lv_obj_t* page) {
    // Los cartuchos: R, cuánto contra la referencia y el estado (clog_look).
    // "R 3.9 · 101 %" y "Tapados" miden 326 de 328 px
    drop_live_ = box(page, 16, 388, 360, 56);
    link_to(drop_live_, Screen::pressure_drop, &styles::button_secondary_pressed);
    lv_obj_set_style_radius(drop_live_, 10, 0);
    lv_obj_set_style_border_width(drop_live_, 2, 0);
    lv_obj_set_style_bg_opa(drop_live_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(drop_live_, 16, 0);
    lv_obj_set_flex_flow(drop_live_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(drop_live_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(drop_live_, 8, 0);
    drop_name_ = label(drop_live_, &font_label_26, colors::text_secondary, "R");
    drop_value_ = label(drop_live_, &font_digits_44, colors::text, "");
    drop_detail_ = label(drop_live_, &font_label_26, colors::text_secondary, "");
    lv_obj_set_flex_grow(box(drop_live_, 0, 0, 0, 1), 1);  // la palabra a la derecha
    drop_word_ = label(drop_live_, &font_label_26, colors::ok, "");

    // Litros de hoy. Sin P1 y P2 la caída no sale y Hoy y Toca echar
    // ocupan todo el renglón (layout_strip)
    today_ = card(page, 388, 388, 264, 56);
    lv_obj_set_style_pad_hor(today_, 16, 0);
    lv_obj_set_flex_flow(today_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(today_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(today_, 8, 0);
    lv_obj_set_flex_grow(label(today_, &font_label_26, colors::text_secondary, "Hoy"), 1);
    today_value_ = label(today_, &font_digits_44, colors::text, "");
    label(today_, &font_label_26, colors::text_secondary, "L");

    // Toca echar: lleva a Dosis
    due_ = frame(page, 664, 388, 344, 56, colors::panel_raised, colors::border_control, 10);
    link_to(due_, Screen::dose, &styles::button_secondary_pressed);
    lv_obj_set_style_pad_left(due_, 16, 0);
    lv_obj_set_style_pad_right(due_, 12, 0);
    lv_obj_set_flex_flow(due_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(due_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(due_, 8, 0);
    lv_obj_set_flex_grow(label(due_, &font_label_26, colors::text_secondary, "Toca echar"), 1);
    dose_value_ = label(due_, &font_digits_44, colors::text, "");
    label(due_, &font_label_26, colors::text_secondary, "mL");
    lv_obj_set_style_margin_left(icon(due_, &icon_chevron_32, colors::text_secondary), 4, 0);
}

void HomeScreen::layout_strip(bool with_drop) {
    if (static_cast<int>(with_drop) == with_drop_) return;
    with_drop_ = with_drop;
    // Con caída: 360 + 264 + 344 como el canvas. Sin ella: 484 + 496
    lv_obj_set_pos(today_, with_drop ? 388 : 16, 388);
    lv_obj_set_width(today_, with_drop ? 264 : 484);
    lv_obj_set_pos(due_, with_drop ? 664 : 512, 388);
    lv_obj_set_width(due_, with_drop ? 344 : 496);
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

    lv_obj_set_hidden(no_supply_, !core::supply_lost(s));

    has_pumps_ = s.has_pumps;
    update_pressure(s);
    update_orp(s);
    update_level(s);
    update_mixer(s);

    // Vasos: cartucho relleno o solo el contorno si está vacío. Los vasos
    // van del 20 µm al sarro, que en core::Filter empiezan en el 1
    for (size_t i = 0; i < housings_.size(); ++i) {
        Housing& h = housings_[i];
        const bool installed = s.filters[i + 1].installed;
        if (static_cast<int>(installed) == h.installed) continue;
        h.installed = installed;
        lv_obj_set_style_bg_color(h.cartridge, color(colors::panel_raised), 0);
        lv_obj_set_style_bg_opa(h.cartridge, installed ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(h.cartridge, installed ? 0 : 2, 0);
        lv_obj_set_style_text_color(h.name, color(installed ? colors::text_secondary : colors::text_disabled), 0);
    }

    fmt::thousands(buf, sizeof buf, s.liters_today);
    set_text(today_value_, buf);
    snprintf(buf, sizeof buf, "%lu", static_cast<unsigned long>(core::dose_ml(s)));
    set_text(dose_value_, buf);
}

void HomeScreen::update_pressure(const core::SystemState& s) {
    p1_.set_installed(s.has_pressure);
    p2_.set_installed(s.has_pressure);
    for (lv_obj_t* part : pressure_parts_) lv_obj_set_hidden(part, !s.has_pressure);
    lv_obj_set_hidden(drop_live_, !s.has_pressure);
    layout_strip(s.has_pressure);
    if (!s.has_pressure) return;

    char buf[12];
    snprintf(buf, sizeof buf, "%d", s.p1_psi);
    set_text(p1_.value, buf);
    snprintf(buf, sizeof buf, "%d", s.p2_psi);
    set_text(p2_.value, buf);
    lv_obj_set_x(p1_marker_, p1_.bar_x + range_x(s.p1_psi, p1_.bar_w) - 1);
    lv_obj_set_x(p2_marker_, p2_.bar_x + range_x(s.p2_psi, p2_.bar_w) - 1);
    // Sin señal: ícono en lugar de la cifra y sin marca (no hay lectura)
    p1_.set_signal(!s.no_signal.p1);
    p2_.set_signal(!s.no_signal.p2);
    lv_obj_set_hidden(p1_marker_, s.no_signal.p1);
    lv_obj_set_hidden(p2_marker_, s.no_signal.p2);

    update_clog(s);
}

// R, % y estado de los cartuchos. Sin flujo: "Sin flujo" en gris y el último
// estado con su hora; sin R, "Filtros" y lo que se sepa
void HomeScreen::update_clog(const core::SystemState& s) {
    const ClogLook look = clog_look(s);
    const bool rated = look.kind <= ClogLook::clogged;
    const bool show_r = look.flowing && look.has_r && look.kind != ClogLook::no_signal;
    lv_obj_set_hidden(drop_name_, !show_r);
    lv_obj_set_hidden(drop_value_, !show_r);
    set_text(drop_value_, look.r);
    char buf[24];
    if (show_r) {
        if (look.pct >= 0) {
            snprintf(buf, sizeof buf, "· %d %%", look.pct);
        } else {
            buf[0] = '\0';
        }
    } else {
        snprintf(buf, sizeof buf, "%s", look.kind != ClogLook::no_signal && !look.flowing ? "Sin flujo" : "Filtros");
    }
    set_text(drop_detail_, buf);
    // Sin flujo, el estado lleva su hora: "Normal 18:42"
    if (rated && !look.flowing) {
        snprintf(buf, sizeof buf, "%s %s", look.word, look.at);
    } else {
        snprintf(buf, sizeof buf, "%s", look.word);
    }
    set_text(drop_word_, buf);

    if (look.kind == drop_level_) return;
    drop_level_ = look.kind;
    lv_obj_set_style_bg_color(drop_live_, color(look.bg), 0);
    lv_obj_set_style_border_color(drop_live_, color(look.border), 0);
    lv_obj_set_style_text_color(drop_word_, color(look.fg), 0);
}

void HomeScreen::update_level(const core::SystemState& s) {
    // Llena queda en la raya de siempre (y 294) y el fondo abajo de la tarjeta.
    // Sin señal tampoco hay agua que dibujar.
    const int pct = core::level_ok(s) ? core::level_pct(s) : -1;
    const bool shown = pct >= 0;
    int32_t y = kWaterTop;
    if (shown) {
        const float ratio = std::clamp(core::level_cm(s) / static_cast<float>(s.level_full_cm), 0.0f, 1.0f);
        y = kWaterBottom - static_cast<int32_t>(std::lround(ratio * static_cast<float>(kWaterBottom - kWaterTop)));
    }
    // Sin lectura (sin sensor o sin calibrar) no hay agua que dibujar: la
    // raya del canvas, siempre arriba, se leía como cisterna llena
    lv_obj_set_hidden(water_, !shown);
    lv_obj_set_hidden(water_line_, !shown);
    lv_obj_set_y(water_line_, y);
    lv_obj_set_y(water_, y);
    lv_obj_set_height(water_, kWaterBottom - y);

    char buf[24];
    if (shown) {
        snprintf(buf, sizeof buf, s.has_orp ? "Cisterna · %d %%" : "Cisterna", pct);
    } else {
        snprintf(buf, sizeof buf, "Cisterna");
    }
    set_text(cistern_name_, buf);
}

void HomeScreen::update_orp(const core::SystemState& s) {
    orp_.set_installed(s.has_orp);
    lv_obj_set_hidden(probe_, !s.has_orp);
    lv_obj_set_hidden(mast_, !s.has_orp && !s.has_mixer);
    lv_obj_set_clickable(cistern_, s.has_orp || s.has_level);  // sin ninguno no hay pestaña Cisterna
    if (!s.has_orp) {
        // Sin sonda la palabra es el nivel; sin nada, el tamaño
        char buf[24];
        const int pct = core::level_ok(s) ? core::level_pct(s) : -1;
        const bool lost = s.has_level && s.no_signal.level;
        const float liters = core::level_liters(s);
        lv_obj_set_hidden(cistern_liters_, liters < 0.0f);
        if (liters >= 0.0f) {
            char amount[16];
            fmt::thousands(amount, sizeof amount, static_cast<uint32_t>(std::lround(liters / 10.0f) * 10));
            char text[24];
            snprintf(text, sizeof text, "\xE2\x89\x88 %s L", amount);
            set_text(cistern_liters_, text);
        }
        if (pct >= 0) {
            snprintf(buf, sizeof buf, "%d %% útil", pct);
        } else {
            snprintf(buf, sizeof buf, lost ? "Sin señal" : s.has_level ? "Sin calibrar" : "10 m³");
        }
        set_text(cistern_word_, buf);
        set_text_color(cistern_word_, lost ? colors::warning : s.has_level ? colors::text : colors::text_disabled);
        orp_level_ = -1;
        return;
    }

    lv_obj_set_hidden(cistern_liters_, true);  // con sonda ahí va su etiqueta
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
    lv_obj_set_hidden(pump_ring_, !s.has_mixer);
    lv_obj_set_hidden(pump_rotor_, !s.has_mixer);
    if (s.has_mixer) mixer_state(r_state_, s);

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
