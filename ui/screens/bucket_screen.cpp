#include "screens/bucket_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "ui.h"

// Medidas tomadas del artboard PruebaCubeta del canvas (cada y menos 64).

namespace ui {
namespace {

constexpr int32_t kValueY = 44 + 10;  // cifra de 136 bajada como en Cisterna
constexpr float kBucketLiters = 10.0f;

const char* const kSteps[4] = {"Cierra V2, la salida a la cisterna", "Pon la cubeta de 10 L bajo V4",
                               "Abre V4 y llénala hasta la marca", "Cierra V4 y vuelve a abrir V2"};

lv_point_precise_t pt(float x, float y) {
    return {static_cast<lv_value_precise_t>(x), static_cast<lv_value_precise_t>(y)};
}

}  // namespace

lv_obj_t* BucketScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);
    create_steps(page);
    create_counting(page);
    create_result(page);
    create_saved(page);
    set_phase(Phase::counting);
    return page;
}

void BucketScreen::create_steps(lv_obj_t* page) {
    lv_obj_t* list = card(page, 16, 12, 452, 432);
    lv_obj_set_style_pad_all(list, 12, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 8, 0);
    for (size_t i = 0; i < steps_.size(); ++i) {
        Step& st = steps_[i];
        st.obj = box(list);
        lv_obj_set_size(st.obj, lv_pct(100), 94);
        lv_obj_set_style_radius(st.obj, 10, 0);
        st.done = icon(st.obj, &icon_ok_32, colors::ok);
        lv_obj_set_pos(st.done, 16, 16);
        st.number = box(st.obj, 14, 14, 36, 36);
        lv_obj_set_style_radius(st.number, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(st.number, 2, 0);
        char n[2] = {static_cast<char>('1' + i), '\0'};
        lv_obj_center(label(st.number, &font_label_26, colors::text, n));
        st.text = label(st.obj, &font_body_32, colors::text_secondary, kSteps[i]);
        lv_obj_set_width(st.text, 354);
        lv_obj_set_pos(st.text, 64, 12);
    }
}

void BucketScreen::create_counting(lv_obj_t* page) {
    counting_ = box(page, 0, 0, 1024, 456);
    lv_obj_set_pos(label(counting_, &font_body_32, colors::text_secondary, "Pulsos contados"), 496, 12);
    count_ = label(counting_, &font_digits_136, colors::text, "");
    lv_obj_set_pos(count_, 488, kValueY);

    // La cubeta: el agua sube hasta la marca de 10 L
    bucket_ = box(counting_, 496, 188, 140, 136);
    lv_obj_add_event_cb(bucket_, draw_bucket, LV_EVENT_DRAW_MAIN_END, this);

    lv_obj_set_pos(label(counting_, &font_label_26, colors::text_secondary, "La marca de 10 L"), 652, 188);
    lv_obj_t* r = row(counting_, 8, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(r, 652, 232);
    with_factor_ = label(r, &font_label_26, colors::text_secondary, "");
    liters_ = label(r, &font_digits_44, colors::text, "");
    label(r, &font_label_26, colors::text_secondary, "L");
    lv_obj_t* note = label(counting_, &font_label_26, colors::text_secondary,
                           "Cuando el agua toque la marca, toca Ya llegué.");
    lv_obj_set_width(note, 356);
    lv_obj_set_pos(note, 652, 280);

    button_primary(counting_, 488, 364, 320, "Ya llegué", [](lv_event_t* e) {
        auto* self = static_cast<BucketScreen*>(lv_event_get_user_data(e));
        self->new_factor_ = static_cast<uint16_t>(std::max<long>(1, std::lround(self->pulses_ / kBucketLiters)));
        self->set_phase(Phase::result);
    }, this);
    button_secondary(counting_, 820, 364, 188, "Cancelar", [](lv_event_t*) { show(Screen::settings); }, nullptr);
}

void BucketScreen::create_result(lv_obj_t* page) {
    result_ = box(page, 0, 0, 1024, 456);
    lv_obj_set_pos(label(result_, &font_body_32, colors::text_secondary, "Factor nuevo del sensor"), 496, 12);
    new_k_ = label(result_, &font_digits_136, colors::text, "");
    lv_obj_set_pos(new_k_, 488, kValueY);
    lv_obj_set_pos(label(result_, &font_title_36, colors::text_secondary, "pulsos/L"), 790, 50);
    explain_ = label(result_, &font_body_32, colors::text_secondary, "");
    lv_obj_set_width(explain_, 512);
    lv_obj_set_pos(explain_, 496, 180);

    lv_obj_t* save = button_primary(result_, 488, 364, 320, "", [](lv_event_t* e) {
        auto* self = static_cast<BucketScreen*>(lv_event_get_user_data(e));
        if (actions().flow_factor_set != nullptr) actions().flow_factor_set(self->new_factor_);
        self->set_phase(Phase::saved);
    }, this);
    save_text_ = lv_obj_get_child(save, -1);
    button_secondary(result_, 820, 364, 188, "Repetir", [](lv_event_t* e) {
        static_cast<BucketScreen*>(lv_event_get_user_data(e))->enter();
    }, this);
}

void BucketScreen::create_saved(lv_obj_t* page) {
    saved_ = box(page, 0, 0, 1024, 456);
    lv_obj_t* head = row(saved_, 10, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(head, 496, 12);
    icon(head, &icon_ok_40, colors::ok);
    label(head, &font_title_36, colors::ok, "Guardado");
    saved_text_ = label(saved_, &font_body_32, colors::text_secondary, "");
    lv_obj_set_width(saved_text_, 512);
    lv_obj_set_pos(saved_text_, 496, 76);

    // Listo: primario sin palomita, regresa a Ajustes
    lv_obj_t* done = button_primary(saved_, 488, 364, 320, "Listo", [](lv_event_t*) { show(Screen::settings); },
                                    nullptr);
    lv_obj_delete(lv_obj_get_child(done, 0));
}

void BucketScreen::enter() {
    pulses_ = 0;
    set_phase(Phase::counting);
    if (actions().bucket_test_started != nullptr) actions().bucket_test_started();
}

void BucketScreen::set_phase(Phase phase) {
    phase_ = phase;
    lv_obj_set_hidden(counting_, phase != Phase::counting);
    lv_obj_set_hidden(result_, phase != Phase::result);
    lv_obj_set_hidden(saved_, phase != Phase::saved);

    // Pasos: 1 y 2 hechos; el 3 es el de ahorita mientras cuenta, luego el 4
    const int active = phase == Phase::counting ? 2 : 3;
    for (int i = 0; i < 4; ++i) {
        Step& st = steps_[static_cast<size_t>(i)];
        const bool done = i < active;
        const bool now = i == active;
        lv_obj_set_hidden(st.done, !done);
        lv_obj_set_hidden(st.number, done);
        fill(st.obj, now ? colors::panel_raised : colors::panel);
        lv_obj_set_style_border_width(st.obj, now ? 2 : 1, 0);
        lv_obj_set_style_border_color(st.obj, color(now ? colors::text : colors::border), 0);
        const uint32_t tone = now ? colors::text : done ? colors::text_secondary : colors::text_disabled;
        lv_obj_set_style_text_color(st.text, color(tone), 0);
        lv_obj_set_style_border_color(st.number, color(now ? colors::text : colors::text_disabled), 0);
        lv_obj_set_style_text_color(lv_obj_get_child(st.number, 0), color(now ? colors::text : colors::text_disabled), 0);
    }

    char buf[96];
    char pulses[16];
    fmt::thousands(pulses, sizeof pulses, pulses_);
    if (phase == Phase::result) {
        snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(new_factor_));
        set_text(new_k_, buf);
        const float change = (static_cast<float>(new_factor_) - factor_) * 100.0f / std::max<float>(1, factor_);
        snprintf(buf, sizeof buf, "%s pulsos en 10 L. Antes era %u, cambia %s%.1f %%.", pulses,
                 static_cast<unsigned>(factor_), change >= 0 ? "+" : "\xE2\x88\x92",
                 static_cast<double>(std::fabs(change)));
        set_text(explain_, buf);
        snprintf(buf, sizeof buf, "Guardar %u", static_cast<unsigned>(new_factor_));
        set_text(save_text_, buf);
    } else if (phase == Phase::saved) {
        snprintf(buf, sizeof buf, "F ahora cuenta %u pulsos por litro. Cierra V4 y abre V2.",
                 static_cast<unsigned>(new_factor_));
        set_text(saved_text_, buf);
    }
}

void BucketScreen::update(const core::SystemState& s) {
    if (phase_ == Phase::counting) {
        factor_ = s.flow_pulses_per_liter;
        pulses_ = s.bucket_pulses;
    }
    char buf[32];
    fmt::thousands(buf, sizeof buf, pulses_);
    set_text(count_, buf);
    snprintf(buf, sizeof buf, "Con %u van", static_cast<unsigned>(factor_));
    set_text(with_factor_, buf);
    const float liters = static_cast<float>(pulses_) / std::max<float>(1, factor_);
    snprintf(buf, sizeof buf, "%.1f", static_cast<double>(liters));
    set_text(liters_, buf);
    const float level = std::min(1.0f, liters / kBucketLiters);
    if (level != level_) {
        level_ = level;
        lv_obj_invalidate(bucket_);
    }
}

void BucketScreen::draw_bucket(lv_event_t* e) {
    const auto* self = static_cast<BucketScreen*>(lv_event_get_user_data(e));
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_area_t a;
    lv_obj_get_coords(self->bucket_, &a);
    const float x0 = static_cast<float>(a.x1);
    const float y0 = static_cast<float>(a.y1);

    // Agua: un trapecio del fondo (y 128) al nivel; las paredes se cierran 0.1 px por px
    if (self->level_ > 0) {
        const float y = 128.0f - self->level_ * 116.0f;
        const float left = 10.0f + 0.1f * (y - 8.0f);
        const float right = 130.0f - 0.1f * (y - 8.0f);
        // Un rectángulo al centro y un triángulo delgado en cada pared: con
        // dos triángulos en diagonal el suavizado dejaba ver la costura
        const float top = std::round(y);
        lv_draw_rect_dsc_t rect;
        lv_draw_rect_dsc_init(&rect);
        rect.bg_color = color(colors::water_pipe);
        lv_area_t water = {static_cast<int32_t>(x0) + 22, static_cast<int32_t>(y0 + top), static_cast<int32_t>(x0) + 117,
                           static_cast<int32_t>(y0) + 127};
        lv_draw_rect(layer, &rect, &water);
        lv_draw_triangle_dsc_t tri;
        lv_draw_triangle_dsc_init(&tri);
        tri.color = color(colors::water_pipe);
        tri.opa = LV_OPA_COVER;
        tri.p[0] = pt(x0 + left, y0 + top);
        tri.p[1] = pt(x0 + 22, y0 + top);
        tri.p[2] = pt(x0 + 22, y0 + 128);
        lv_draw_triangle(layer, &tri);
        tri.p[0] = pt(x0 + 118, y0 + top);
        tri.p[1] = pt(x0 + right, y0 + top);
        tri.p[2] = pt(x0 + 118, y0 + 128);
        lv_draw_triangle(layer, &tri);
    }

    // Cubeta: el contorno de 3 px
    static lv_point_precise_t outline[5];
    const float xs[5] = {10, 130, 118, 22, 10};
    const float ys[5] = {8, 8, 128, 128, 8};
    for (int i = 0; i < 5; ++i) outline[i] = pt(x0 + xs[i], y0 + ys[i]);
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.width = 3;
    line.color = color(colors::border_control);
    line.round_start = 1;
    line.round_end = 1;
    line.points = outline;
    line.point_cnt = 5;
    lv_draw_line(layer, &line);

    // Marca de 10 L: punteada de 6 con huecos de 5 (horizontal: LVGL sí la puntea)
    lv_draw_line_dsc_t mark;
    lv_draw_line_dsc_init(&mark);
    mark.width = 2;
    mark.color = color(colors::text);
    mark.dash_width = 6;
    mark.dash_gap = 5;
    mark.p1 = pt(x0 + 6, y0 + 12);
    mark.p2 = pt(x0 + 134, y0 + 12);
    lv_draw_line(layer, &mark);
}

}  // namespace ui
