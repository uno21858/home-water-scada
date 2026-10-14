#include "components/pipe.h"

#include <algorithm>
#include <cmath>

#include "components/primitives.h"
#include "theme/colors.h"

namespace ui {
namespace {

constexpr int32_t kStep = 24;      // paso-puntos: un punto cada 24 px
constexpr int32_t kDot = 6;        // punto-flujo: 6 px
constexpr uint32_t kPeriodMs = 33; // unos 30 cuadros por segundo

}  // namespace

void FlowPipes::add(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, Dir dir) {
    if (count_ >= static_cast<int>(segments_.size())) return;
    Segment& seg = segments_[count_++];
    seg.obj = box(parent, x, y, w, h);
    seg.dir = dir;
    seg.owner = this;
    lv_obj_set_style_bg_color(seg.obj, color(colors::pipe), 0);
    lv_obj_set_style_bg_opa(seg.obj, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(seg.obj, draw_dots, LV_EVENT_DRAW_MAIN_END, &seg);

    if (timer_ == nullptr) {
        timer_ = lv_timer_create(tick, kPeriodMs, this);
        lv_timer_pause(timer_);
    }
}

void FlowPipes::set_flow(float lpm) {
    const bool flowing = lpm >= 0.05f;
    speed_ = flowing ? std::clamp(static_cast<int32_t>(std::lround(4.0f * lpm)), int32_t{12}, int32_t{90}) : 0;
    if (static_cast<int>(flowing) == flowing_) return;
    flowing_ = flowing;

    for (int i = 0; i < count_; ++i) {
        lv_obj_set_style_bg_color(segments_[i].obj, color(flowing ? colors::water_pipe : colors::pipe), 0);
    }
    if (timer_ == nullptr) return;
    if (flowing) {
        lv_timer_resume(timer_);
    } else {
        lv_timer_pause(timer_);
    }
}

void FlowPipes::draw_dots(lv_event_t* e) {
    const auto* seg = static_cast<const Segment*>(lv_event_get_user_data(e));
    if (seg->owner->speed_ == 0) return;

    lv_area_t c;
    lv_obj_get_coords(seg->obj, &c);
    const bool vertical = seg->dir == Dir::down || seg->dir == Dir::up;
    const int32_t length = vertical ? lv_area_get_height(&c) : lv_area_get_width(&c);
    const int32_t phase = seg->owner->phase_q8_ >> 8;

    lv_draw_rect_dsc_t dot;
    lv_draw_rect_dsc_init(&dot);
    dot.bg_color = color(colors::water_dot);
    dot.radius = kDot / 2;

    lv_layer_t* layer = lv_event_get_layer(e);
    // Los puntos van centrados en cada paso de 24 px, recorridos por la fase
    for (int32_t s = phase + kStep / 2 - kStep; s < length; s += kStep) {
        if (s < kDot / 2 || s > length - kDot / 2) continue;
        int32_t x = (c.x1 + c.x2) / 2;
        int32_t y = (c.y1 + c.y2) / 2;
        switch (seg->dir) {
            case Dir::right: x = c.x1 + s; break;
            case Dir::left: x = c.x2 - s; break;
            case Dir::down: y = c.y1 + s; break;
            case Dir::up: y = c.y2 - s; break;
        }
        const lv_area_t a = {x - kDot / 2, y - kDot / 2, x + kDot / 2 - 1, y + kDot / 2 - 1};
        lv_draw_rect(layer, &dot, &a);
    }
}

void FlowPipes::tick(lv_timer_t* timer) {
    auto* self = static_cast<FlowPipes*>(lv_timer_get_user_data(timer));
    self->phase_q8_ = (self->phase_q8_ + self->speed_ * 256 * static_cast<int32_t>(kPeriodMs) / 1000) % (kStep * 256);
    for (int i = 0; i < self->count_; ++i) lv_obj_invalidate(self->segments_[i].obj);
}

}  // namespace ui
