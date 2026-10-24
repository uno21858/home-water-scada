#include "screens/idle_screen.h"

#include <cmath>
#include <cstdio>

#include "components/primitives.h"
#include "components/status_pill.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"
#include "level.h"

// Medidas tomadas del artboard Pasillo del canvas. Aquí no se resta 64:
// la pantalla es completa, de 1024 x 600.

namespace ui {
namespace {

// Cifras grandes bajadas como en Flujo y Cisterna (ver CLAUDE.md)
constexpr int32_t kFlowY = 164 + 17;
constexpr int32_t kDoseY = 222 + 10;
// Con 3 cifras los mL no caben a 136 px antes de la orilla: van a 72 px con
// la misma línea base (98 px abajo del top a 136 y 52 a 72)
constexpr int32_t kDoseY72 = kDoseY + 98 - 52;

// Agua de fondo: 100 % llega a y 110, abajo del reloj y la pastilla; 0 % es
// el borde de abajo. Oscura y casi transparente para que el caudal (en azul
// agua) se siga leyendo de lejos.
constexpr int32_t kWaterTopY = 110;
constexpr lv_opa_t kWaterOpa = LV_OPA_20;
// Olita: franja de 24 px, 5 px de alto de ola, una ola cada 256 px y una
// vuelta cada ~3 s a 12.5 cuadros por segundo. Si en la 7B se ve pesada,
// kWave = false y queda el agua quieta.
constexpr bool kWave = true;
constexpr int32_t kWaveH = 24;
constexpr float kWaveAmp = 5.0f;
constexpr float kWaveLength = 256.0f;
constexpr uint32_t kWavePeriodMs = 80;
constexpr float kWaveStep = 6.2832f / 40.0f;
constexpr int32_t kWaveColumn = 8;  // la ola se rellena en columnas de 8 px

// "Hoy 1,860 L": nombre, cifra de 44 y unidad en la misma línea base
lv_obj_t* reading(lv_obj_t* parent, const char* name, const char* unit, lv_obj_t** value) {
    lv_obj_t* r = row(parent, 10, LV_FLEX_ALIGN_END);
    label(r, &font_body_32, colors::text_secondary, name);
    *value = label(r, &font_digits_44, colors::text, "");
    label(r, &font_label_26, colors::text_secondary, unit);
    return r;
}

}  // namespace

void IdleScreen::create(lv_obj_t* layer, Callback on_wake) {
    on_wake_ = on_wake;

    root_ = box(layer, 0, 0, 1024, 600);
    fill(root_, colors::background);
    lv_obj_set_clickable(root_, true);  // un toque en cualquier lado lo cierra
    lv_obj_add_event_cb(root_, handle_touch, LV_EVENT_CLICKED, this);

    // El agua va primero para quedar atrás de todo
    water_ = box(root_, 0, 600, 1024, 0);
    fill(water_, colors::water_pipe);
    lv_obj_set_style_bg_opa(water_, kWaterOpa, 0);
    wave_ = box(root_, 0, 600, 1024, kWaveH);
    lv_obj_add_event_cb(wave_, draw_wave, LV_EVENT_DRAW_MAIN_END, this);
    if (kWave) {
        wave_timer_ = lv_timer_create(tick_wave, kWavePeriodMs, this);
        lv_timer_pause(wave_timer_);  // corre solo con el Pasillo abierto
    }

    clock_ = label(root_, &font_title_xl_48, colors::text, "");
    lv_obj_set_pos(clock_, 32, 28);

    // Arriba a la derecha el WiFi y la pastilla de estado, más grandes que en
    // el encabezado. La pastilla queda donde la pone el canvas y el WiFi a su
    // izquierda: crece hacia la izquierda con "Advertencia" sin encimarse.
    lv_obj_t* corner = row(root_, 24, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(corner, LV_ALIGN_TOP_RIGHT, -32, 28);
    wifi_ = icon(corner, &icon_wifi_40, colors::text_secondary);
    pill_ = row(corner, 10, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_height(pill_, 58);
    lv_obj_set_style_radius(pill_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(pill_, 2, 0);
    lv_obj_set_style_bg_opa(pill_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(pill_, 12, 0);
    lv_obj_set_style_pad_right(pill_, 22, 0);
    pill_icon_ = icon(pill_, &icon_ok_40, colors::ok);
    pill_text_ = label(pill_, &font_title_36, colors::ok, "");

    // Caudal
    flow_title_ = label(root_, &font_body_32, colors::text_secondary, "");
    lv_obj_set_pos(flow_title_, 32, 124);
    flow_value_ = label(root_, &font_digits_232, colors::water, "");
    lv_obj_set_pos(flow_value_, 22, kFlowY);
    lv_obj_set_pos(label(root_, &font_title_36, colors::text_secondary, "L/min"), 584, 288);

    // Toca echar, a la derecha de una raya
    fill(box(root_, 712, 176, 2, 216), colors::border);
    lv_obj_set_pos(label(root_, &font_body_32, colors::text_secondary, "Toca echar"), 744, 176);
    dose_value_ = label(root_, &font_digits_136, colors::text, "");
    dose_unit_ = label(root_, &font_title_36, colors::text_secondary, "mL");
    lv_obj_set_pos(label(root_, &font_label_26, colors::text_secondary, "de Cloralex en B"), 744, 350);

    // Renglón de abajo: hoy, caída y ORP
    lv_obj_t* bottom = row(root_, 40, LV_FLEX_ALIGN_END);
    lv_obj_set_pos(bottom, 32, 410);
    reading(bottom, "Hoy", "L", &today_);
    drop_ = reading(bottom, "Caída", "psi", &drop_value_);
    drop_word_ = label(drop_, &font_label_26, colors::ok, "");
    orp_ = reading(bottom, "ORP", "mV", &orp_value_);

    pipe_.add(root_, 0, 500, 1024, 10, FlowPipes::Dir::right);

    lv_obj_t* hint = label(root_, &font_label_26, colors::text_disabled, "Toca la pantalla para volver al tren");
    lv_obj_set_pos(hint, 32, 540);

    // Cisterna abajo a la derecha, junto a la pista: en el renglón de Hoy,
    // Caída y ORP ya no cabe cuando están los cuatro
    cistern_ = reading(root_, "Cisterna", "%", &cistern_value_);
    lv_obj_align(cistern_, LV_ALIGN_BOTTOM_RIGHT, -32, -24);

    lv_obj_set_hidden(root_, true);
}

void IdleScreen::update(const core::SystemState& s) {
    char buf[16];
    snprintf(buf, sizeof buf, "%02u:%02u", static_cast<unsigned>(s.hour), static_cast<unsigned>(s.minute));
    set_text(clock_, buf);

    const lv_image_dsc_t* wifi = wifi_icon(s.wifi_ok, s.wifi_bars, true);
    if (wifi != wifi_src_) {
        wifi_src_ = wifi;
        lv_image_set_src(wifi_, wifi);
        set_icon_color(wifi_, s.wifi_ok ? colors::text_secondary : colors::warning);
    }

    const PillLook look = pill_look(s);
    if (static_cast<int>(look.kind) != level_) {
        level_ = static_cast<int>(look.kind);
        static const lv_image_dsc_t* const kIcons[4] = {&icon_ok_40, &icon_advertencia_40, &icon_alarma_40,
                                                         &icon_cartucho_40};
        lv_obj_set_style_bg_color(pill_, color(look.bg), 0);
        lv_obj_set_style_border_color(pill_, color(look.border), 0);
        lv_image_set_src(pill_icon_, kIcons[look.kind]);
        set_icon_color(pill_icon_, look.fg);
        lv_label_set_text(pill_text_, look.text);
        lv_obj_set_style_text_color(pill_text_, color(look.fg), 0);
    }

    const bool flowing = s.flow_lpm >= 0.05f;
    fmt::one_decimal(buf, sizeof buf, flowing ? s.flow_lpm : 0.0f);
    set_text(flow_value_, buf);
    pipe_.set_flow(flowing ? s.flow_lpm : 0.0f);
    const int state = flowing ? 0 : core::supply_lost(s) ? 2 : 1;
    if (state != flow_state_) {
        flow_state_ = state;
        set_text(flow_title_, state == 0 ? "Caudal" : state == 1 ? "Sin flujo" : "Sin agua del SIAPA");
        set_text_color(flow_title_, state == 2 ? colors::warning : colors::text_secondary);
        lv_obj_set_style_text_color(flow_value_, color(flowing ? colors::water : colors::text_disabled), 0);
    }

    const uint32_t ml = core::dose_ml(s);
    snprintf(buf, sizeof buf, "%lu", static_cast<unsigned long>(ml));
    set_text(dose_value_, buf);
    layout_dose(ml >= 100);

    fmt::thousands(buf, sizeof buf, s.liters_today);
    set_text(today_, buf);

    lv_obj_set_hidden(drop_, !s.has_pressure);
    if (s.has_pressure) {
        const int drop = s.p1_psi - s.p2_psi;
        snprintf(buf, sizeof buf, "%d", drop);
        set_text(drop_value_, buf);
        const core::Level drop_level = core::drop_level(s);
        if (static_cast<int>(drop_level) != drop_level_) {
            drop_level_ = static_cast<int>(drop_level);
            const uint32_t tone = drop_level == core::Level::ok        ? colors::ok
                                  : drop_level == core::Level::warning ? colors::warning
                                                                       : colors::alarm;
            set_text(drop_word_, drop_level == core::Level::ok        ? "Normal"
                                 : drop_level == core::Level::warning ? "Revisar"
                                                                      : "Tapados");
            lv_obj_set_style_text_color(drop_word_, color(tone), 0);
        }
    }

    lv_obj_set_hidden(orp_, !s.has_orp);
    if (s.has_orp) {
        fmt::signed_int(buf, sizeof buf, s.orp_mv);
        set_text(orp_value_, buf);
    }

    // Cisterna: el número abajo y el agua de fondo. Sin marcar llena no hay %.
    const int pct = s.has_level ? core::level_pct(s) : -1;
    lv_obj_set_hidden(cistern_, pct < 0);
    if (pct >= 0) {
        snprintf(buf, sizeof buf, "%d", pct);
        set_text(cistern_value_, buf);
    }
    place_water(pct);
}

void IdleScreen::place_water(int pct) {
    if (pct == water_pct_) return;  // solo se redibuja cuando cambia
    water_pct_ = pct;
    const bool shown = pct > 0;
    lv_obj_set_hidden(water_, !shown);
    lv_obj_set_hidden(wave_, !shown);
    if (!shown) return;
    const int32_t surface = 600 - (600 - kWaterTopY) * pct / 100;
    lv_obj_set_y(wave_, surface - kWaveH / 2);
    lv_obj_set_pos(water_, 0, surface + kWaveH / 2);
    lv_obj_set_height(water_, 600 - (surface + kWaveH / 2));
}

void IdleScreen::tick_wave(lv_timer_t* timer) {
    auto* self = static_cast<IdleScreen*>(lv_timer_get_user_data(timer));
    if (lv_obj_is_hidden(self->wave_)) return;
    self->wave_phase_ = std::fmod(self->wave_phase_ + kWaveStep, 6.2832f);
    lv_obj_invalidate(self->wave_);  // solo la franja
}

void IdleScreen::draw_wave(lv_event_t* e) {
    auto* self = static_cast<IdleScreen*>(lv_event_get_user_data(e));
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_area_t area;
    lv_obj_get_coords(self->wave_, &area);
    const float mid = static_cast<float>(area.y1 + kWaveH / 2);
    auto crest_y = [self, mid](int32_t x) {
        return mid + kWaveAmp * std::sin(static_cast<float>(x) / kWaveLength * 6.2832f + self->wave_phase_);
    };

    // Relleno: una columna de 8 px de la ola hasta abajo de la franja, del
    // mismo color que el agua quieta para que no se vea la unión
    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = color(colors::water_pipe);
    rect.bg_opa = kWaterOpa;
    for (int32_t x = 0; x < 1024; x += kWaveColumn) {
        const auto top = static_cast<int32_t>(std::lround(crest_y(x + kWaveColumn / 2)));
        const lv_area_t column = {area.x1 + x, top, area.x1 + x + kWaveColumn - 1, area.y2};
        lv_draw_rect(layer, &rect, &column);
    }

    // La raya de la superficie, un poco más clara
    for (size_t i = 0; i < self->crest_.size(); ++i) {
        const auto x = static_cast<int32_t>(i) * kWaveColumn;
        self->crest_[i] = {static_cast<lv_value_precise_t>(area.x1 + x), static_cast<lv_value_precise_t>(crest_y(x))};
    }
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.color = color(colors::water);
    line.opa = LV_OPA_40;
    line.width = 2;
    line.points = self->crest_.data();
    line.point_cnt = static_cast<uint32_t>(self->crest_.size());
    lv_draw_line(layer, &line);
}

void IdleScreen::layout_dose(bool three_digits) {
    if (static_cast<int>(three_digits) == three_digits_) return;
    three_digits_ = three_digits;
    lv_obj_set_style_text_font(dose_value_, three_digits ? &font_digits_72 : &font_digits_136, 0);
    lv_obj_set_pos(dose_value_, 740, three_digits ? kDoseY72 : kDoseY);
    // A 136 px "mL" va arriba a la derecha de la cifra; a 72, en su misma línea base
    lv_obj_set_pos(dose_unit_, three_digits ? 900 : 940, three_digits ? kDoseY72 + 16 : 228);
}

void IdleScreen::open() {
    lv_obj_set_hidden(root_, false);
    if (wave_timer_ != nullptr) lv_timer_resume(wave_timer_);
}

void IdleScreen::close() {
    lv_obj_set_hidden(root_, true);
    if (wave_timer_ != nullptr) lv_timer_pause(wave_timer_);
}

bool IdleScreen::is_open() const { return !lv_obj_is_hidden(root_); }

void IdleScreen::handle_touch(lv_event_t* e) {
    auto* self = static_cast<IdleScreen*>(lv_event_get_user_data(e));
    self->close();
    if (self->on_wake_ != nullptr) self->on_wake_();
}

}  // namespace ui
