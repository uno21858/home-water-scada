#include "screens/home/overview_screen.h"

#include <cstdio>
#include <cstring>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "theme/pid.h"
#include "theme/styles.h"

// Medidas tomadas del artboard Panorama del canvas (cada y menos 64).

namespace ui {
namespace {

using Dir = FlowPipes::Dir;

// Recuadro con borde de 2 px (SIAPA, presostato, hidros, cisterna...)
lv_obj_t* frame(lv_obj_t* page, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t bg, uint32_t border,
                int32_t radius) {
    lv_obj_t* b = box(page, x, y, w, h);
    fill(b, bg);
    lv_obj_set_style_border_color(b, color(border), 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_set_style_radius(b, radius, 0);
    return b;
}

// Borde punteado de 2 px en borde, para lo que está por instalar. LVGL no
// puntea bordes: se dibujan las rayas a mano y las esquinas con arcos.
void draw_dashed(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_current_target_obj(e);
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    const int32_t r = lv_obj_get_style_radius(obj, LV_PART_MAIN);
    constexpr int32_t kWidth = 2;
    constexpr int32_t kDash = 6;
    constexpr int32_t kGap = 6;

    lv_draw_rect_dsc_t dash;
    lv_draw_rect_dsc_init(&dash);
    dash.bg_color = color(colors::border);
    // Un lado: rayas de kDash con kGap, del fin de una esquina al de la otra
    auto side = [&](bool horizontal, int32_t fixed, int32_t from, int32_t to) {
        for (int32_t p = from; p <= to; p += kDash + kGap) {
            const int32_t end = p + kDash - 1 < to ? p + kDash - 1 : to;
            const lv_area_t d = horizontal ? lv_area_t{p, fixed, end, fixed + kWidth - 1}
                                           : lv_area_t{fixed, p, fixed + kWidth - 1, end};
            lv_draw_rect(layer, &dash, &d);
        }
    };
    side(true, a.y1, a.x1 + r + 2, a.x2 - r - 2);
    side(true, a.y2 - kWidth + 1, a.x1 + r + 2, a.x2 - r - 2);
    side(false, a.x1, a.y1 + r + 2, a.y2 - r - 2);
    side(false, a.x2 - kWidth + 1, a.y1 + r + 2, a.y2 - r - 2);

    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.color = color(colors::border);
    arc.width = kWidth;
    arc.radius = static_cast<uint16_t>(r);
    const lv_point_t centers[4] = {
        {a.x1 + r, a.y1 + r}, {a.x2 - r, a.y1 + r}, {a.x2 - r, a.y2 - r}, {a.x1 + r, a.y2 - r}};
    const int32_t starts[4] = {180, 270, 0, 90};
    for (int i = 0; i < 4; ++i) {
        arc.center = centers[i];
        arc.start_angle = static_cast<lv_value_precise_t>(starts[i]);
        arc.end_angle = static_cast<lv_value_precise_t>(starts[i] + 90);
        lv_draw_arc(layer, &arc);
    }
}

lv_obj_t* dashed_box(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius) {
    lv_obj_t* b = box(parent, x, y, w, h);
    lv_obj_set_style_radius(b, radius, 0);
    lv_obj_add_event_cb(b, draw_dashed, LV_EVENT_DRAW_MAIN_END, nullptr);
    return b;
}

// Texto de 26 px en su lugar
lv_obj_t* small_text(lv_obj_t* parent, const char* text, int32_t x, int32_t y, uint32_t hex) {
    lv_obj_t* l = label(parent, &font_label_26, hex, text);
    lv_obj_set_pos(l, x, y);
    return l;
}

void rotate(void* obj, int32_t angle) { lv_image_set_rotation(static_cast<lv_obj_t*>(obj), angle); }

}  // namespace

lv_obj_t* OverviewScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);

    // El piso: lo de abajo está enterrado
    fill(box(page, 16, 295, 992, 2), colors::border);
    small_text(page, "Bajo el piso", 16, 304, colors::text_disabled);

    // Tubos que mide F: del SIAPA al tren y del tren a la cisterna
    pipes_.add(page, 176, 175, 56, 10, Dir::right);
    pipes_.add(page, 351, 252, 10, 76, Dir::down);
    // Sin sensor: de la bomba a los hidros y de los hidros a la casa
    fill(box(page, 593, 76, 10, 252), colors::pipe);
    fill(box(page, 593, 71, 143, 10), colors::pipe);
    fill(box(page, 803, 112, 10, 20), colors::pipe);

    lv_obj_t* siapa = frame(page, 16, 136, 160, 88, colors::background, colors::pipe, 10);
    lv_obj_set_flex_flow(siapa, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(siapa, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_hor(siapa, 14, 0);
    label(siapa, &font_body_32, colors::text_secondary, "SIAPA");
    label(siapa, &font_label_26, colors::text_disabled, "medidor");

    create_train(page);

    // Presostato sobre el tubo de la bomba. En el canvas mide 140 px y
    // "35 a 65 psi" se sale; aquí mide lo que su texto y queda centrado en el tubo.
    lv_obj_t* switch_box = frame(page, 0, 164, LV_SIZE_CONTENT, 64, colors::background, colors::pipe, 10);
    lv_obj_set_flex_flow(switch_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(switch_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(switch_box, 8, 0);
    lv_obj_set_style_pad_row(switch_box, -4, 0);  // renglones de 28 como en el canvas
    label(switch_box, &font_label_26, colors::text_secondary, "Presostato");
    label(switch_box, &font_label_26, colors::text_disabled, "35 a 65 psi");
    lv_obj_align(switch_box, LV_ALIGN_TOP_MID, 598 - 512, 164);

    create_cistern(page);
    create_house(page);
    create_key(page);
    return page;
}

void OverviewScreen::create_train(lv_obj_t* page) {
    lv_obj_t* c = frame(page, 232, 112, 264, 140, colors::panel, colors::border_control, 10);
    link_to(c, Screen::home);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(c, 4, 0);
    lv_obj_set_style_pad_top(c, 12, 0);
    lv_obj_set_style_pad_left(c, 16, 0);
    lv_obj_set_style_pad_right(c, 12, 0);

    // "Tren de filtros" mide 222 px de 232: la flecha va pegada al texto y se
    // mete en el margen (el canvas la saca de la tarjeta)
    lv_obj_t* top = box(c, 0, 0, 246, 38);
    lv_obj_t* name = label(top, &font_body_32, colors::text, "Tren de filtros");
    lv_obj_align_to(icon(top, &icon_chevron_32, colors::text_secondary), name, LV_ALIGN_OUT_RIGHT_MID, -8, 0);

    lv_obj_t* flow = row(c, 8, LV_FLEX_ALIGN_END);
    lv_obj_set_height(flow, 38);
    flow_ = label(flow, &font_digits_44, colors::water, "");
    label(flow, &font_label_26, colors::text_secondary, "L/min");

    // "Caída 4 psi · Normal" no cabe (264 px de 232): el estado va con su ícono
    drop_row_ = row(c, 6, LV_FLEX_ALIGN_CENTER);
    drop_ = label(drop_row_, &font_label_26, colors::text_secondary, "");
    drop_icon_ = icon(drop_row_, &icon_ok_28, colors::ok);
}

void OverviewScreen::create_cistern(lv_obj_t* page) {
    lv_obj_t* c = frame(page, 232, 328, 480, 112, colors::panel, colors::border_control, 10);
    link_to(c, Screen::cistern);  // solo abre con la sonda ORP (update)
    cistern_ = c;
    fill(box(c, 0, 8, 476, 2), colors::water);  // nivel del agua

    lv_obj_t* name = row(c, 10, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(name, 14, 18);
    label(name, &font_body_32, colors::text, "Cisterna");
    label(name, &font_label_26, colors::text_secondary, "10 m³");

    orp_row_ = row(c, 8, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(orp_row_, 14, 62);
    label(orp_row_, &font_label_26, colors::text_secondary, "ORP");
    orp_value_ = label(orp_row_, &font_digits_44, colors::text, "");
    label(orp_row_, &font_label_26, colors::text_secondary, "mV");
    orp_word_ = label(orp_row_, &font_label_26, colors::ok, "");
    lv_obj_set_style_pad_left(orp_word_, 6, 0);

    // Encima de la tarjeta: el tubo y la bomba de ½ HP
    fill(box(page, 593, 330, 10, 36), colors::pipe);
    lv_obj_t* pump = lv_image_create(page);
    lv_image_set_src(pump, &pid_bomba_sumergible);
    lv_obj_set_pos(pump, 582, 364);
    small_text(page, "½ HP", 622, 364, colors::text_disabled);

    // Mástil de la sonda ORP con la bomba R abajo; R abre Mezcla
    mast_ = box(page, 548, 330, 3, 26);
    fill(mast_, colors::text_secondary);
    mixer_ = box(page, 500, 340, 70, 50);
    link_to(mixer_, Screen::mixer, &styles::button_secondary_pressed);
    lv_obj_set_style_radius(mixer_, 10, 0);
    lv_obj_t* r = small_text(mixer_, "R", 6, 13, colors::text_secondary);
    lv_obj_set_width(r, 24);
    lv_obj_set_style_text_align(r, LV_TEXT_ALIGN_RIGHT, 0);
    frame(mixer_, 37, 17, 25, 25, colors::background, colors::text_secondary, LV_RADIUS_CIRCLE);
    rotor_ = icon(mixer_, &icon_impulsor_chico_26, colors::pipe);
    lv_obj_set_pos(rotor_, 36, 16);
}

void OverviewScreen::create_house(lv_obj_t* page) {
    // La casa: marco sin piso, abierto hacia el suelo
    lv_obj_t* house = box(page, 712, 112, 296, 184);
    lv_obj_set_style_border_width(house, 2, 0);
    lv_obj_set_style_border_color(house, color(colors::pipe), 0);
    lv_obj_set_style_border_side(
        house, static_cast<lv_border_side_t>(LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT), 0);

    frame(page, 736, 40, 40, 72, colors::background, colors::pipe, 18);
    frame(page, 788, 40, 40, 72, colors::background, colors::pipe, 18);
    small_text(page, "Hidros", 844, 40, colors::text_secondary);
    small_text(page, "2 × 210 L", 844, 72, colors::text_disabled);

    lv_obj_t* c = card(page, 736, 132, 256, 80);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_hor(c, 14, 0);
    label(c, &font_body_32, colors::text, "Casa");
    lv_obj_t* today = row(c, 6, LV_FLEX_ALIGN_END);
    label(today, &font_label_26, colors::text_secondary, "≈");
    liters_ = label(today, &font_label_26, colors::text, "");
    label(today, &font_label_26, colors::text_secondary, "hoy");

    // RO de la cocina, si se piensa poner
    ro_ = box(page, 736, 212, 256, 76);
    fill(box(ro_, 127, 0, 10, 12), colors::pipe);
    lv_obj_t* ro = dashed_box(ro_, 0, 12, 256, 64, 10);
    lv_obj_set_flex_flow(ro, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ro, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(ro, 16, 0);  // 14 del canvas más el borde
    lv_obj_set_style_pad_column(ro, 10, 0);
    icon(ro, &icon_gota_28, colors::text_disabled);
    label(ro, &font_label_26, colors::text_disabled, "RO de la cocina");
}

void OverviewScreen::create_key(lv_obj_t* page) {
    key_ = card(page, 728, 332, 280, 112);

    // Renglones de 32 px: muestra de 40 px y texto a 12
    auto entry = [this](const char* text) {
        lv_obj_t* r = box(key_, 16, 0, 240, 32);
        small_text(r, text, 52, 0, colors::text_secondary);
        return r;
    };
    lv_obj_t* measured = entry("Lo mide F");
    key_pipe_.add(measured, 0, 11, 40, 10, Dir::right);
    key_pipe_.set_flow(8.0f);
    lv_obj_t* plain = entry("Sin sensor");
    fill(box(plain, 0, 11, 40, 10), colors::pipe);
    key_ro_ = entry("Por instalar");
    dashed_box(key_ro_, 0, 6, 40, 20, 4);
}

void OverviewScreen::update(const core::SystemState& s) {
    char buf[32];

    const bool flowing = s.flow_lpm >= 0.05f;
    fmt::one_decimal(buf, sizeof buf, flowing ? s.flow_lpm : 0.0f);
    set_text(flow_, buf);
    pipes_.set_flow(flowing ? s.flow_lpm : 0.0f);
    if (static_cast<int>(flowing) != flowing_) {
        flowing_ = flowing;
        lv_obj_set_style_text_color(flow_, color(flowing ? colors::water : colors::text_disabled), 0);
    }

    // Caída del tren: solo con P1 y P2
    lv_obj_set_hidden(drop_row_, !s.has_pressure);
    if (s.has_pressure) {
        // Sin señal de P1 o P2 no hay caída: "Caída sin señal" (230 px de 232)
        const bool ok = core::pressure_ok(s);
        if (ok) {
            snprintf(buf, sizeof buf, "Caída %d psi", s.p1_psi - s.p2_psi);
        } else {
            snprintf(buf, sizeof buf, "Caída sin señal");
        }
        set_text(drop_, buf);
        const int level = ok ? static_cast<int>(core::drop_level(s)) : kNoSignal;
        if (level != drop_level_) {
            drop_level_ = level;
            static const lv_image_dsc_t* const kIcons[4] = {&icon_ok_28, &icon_advertencia_28, &icon_alarma_28,
                                                             &icon_advertencia_28};
            static const uint32_t kTones[4] = {colors::ok, colors::warning, colors::alarm, colors::warning};
            lv_image_set_src(drop_icon_, kIcons[level]);
            set_icon_color(drop_icon_, kTones[level]);
        }
    }

    lv_obj_set_hidden(orp_row_, !s.has_orp);
    lv_obj_set_clickable(cistern_, s.has_orp);  // sin sonda no hay pestaña Cisterna
    if (s.has_orp) {
        fmt::signed_int(buf, sizeof buf, s.orp_mv);
        set_text(orp_value_, buf);
        const core::Level level = core::orp_level(s.orp_mv);
        if (static_cast<int>(level) != orp_level_) {
            orp_level_ = static_cast<int>(level);
            set_text(orp_word_, level == core::Level::ok        ? "Oxidante"
                                : level == core::Level::warning ? "Neutro"
                                                                : "Reductor");
            const uint32_t tone = level == core::Level::ok        ? colors::ok
                                  : level == core::Level::warning ? colors::warning
                                                                  : colors::alarm;
            lv_obj_set_style_text_color(orp_word_, color(tone), 0);
        }
    }

    // Bomba R: gira y en agua cuando está prendida, como en el Tren
    lv_obj_set_hidden(mast_, !s.has_orp && !s.has_mixer);
    lv_obj_set_hidden(mixer_, !s.has_mixer);
    const bool spinning = s.has_mixer && s.mixer_on;
    if (static_cast<int>(spinning) != spinning_) {
        spinning_ = spinning;
        set_icon_color(rotor_, spinning ? colors::water : colors::pipe);
        if (spinning) {
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, rotor_);
            lv_anim_set_exec_cb(&a, rotate);
            lv_anim_set_values(&a, 0, 3600);
            lv_anim_set_duration(&a, 2000);
            lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
            lv_anim_start(&a);
        } else {
            lv_anim_delete(rotor_, rotate);
            lv_image_set_rotation(rotor_, 0);
        }
    }

    fmt::thousands(buf, sizeof buf, s.liters_today);
    strncat(buf, " L", sizeof buf - strlen(buf) - 1);
    set_text(liters_, buf);

    // RO de la cocina: el recuadro y su renglón en la tarjeta de los tubos
    if (static_cast<int>(s.ro_kitchen_planned) != ro_shown_) {
        ro_shown_ = s.ro_kitchen_planned;
        lv_obj_set_hidden(ro_, !s.ro_kitchen_planned);
        lv_obj_set_hidden(key_ro_, !s.ro_kitchen_planned);
        // Los renglones van centrados en los 110 px de adentro, con 2 de hueco
        const int rows = s.ro_kitchen_planned ? 3 : 2;
        const int32_t top = (110 - (rows * 32 + (rows - 1) * 2)) / 2;
        for (int i = 0; i < 3; ++i) lv_obj_set_y(lv_obj_get_child(key_, i), top + i * 34);
    }
}

}  // namespace ui
