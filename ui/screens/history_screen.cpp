#include "screens/history_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/format.h"
#include "theme/icons.h"

// Medidas tomadas del artboard Historial del canvas (cada y menos 64).
// Las coordenadas dentro de una tarjeta cuentan desde adentro del borde.

namespace ui {
namespace {

// Área de la gráfica dentro de su tarjeta: de x 87 a 507 y de y 98 a 312
constexpr float kL = 87.0f;   // orilla izquierda
constexpr float kW = 420.0f;  // ancho
constexpr float kB = 312.0f;  // línea de abajo (el mínimo)
constexpr float kH = 214.0f;  // alto del mínimo al máximo

// Centro de la banda i de n, la x de la hora h y la y de un valor
float band_x(int i, int n) { return kL + (static_cast<float>(i) + 0.5f) * kW / static_cast<float>(n); }
float hour_x(int h) { return kL + static_cast<float>(h) * kW / 24.0f; }
float value_y(float v, float min, float max) { return kB - (v - min) / (max - min) * kH; }
int32_t px(float v) { return static_cast<int32_t>(std::lround(v)); }

// Punto para lv_draw_line y lv_draw_triangle (float o entero según lv_conf.h)
lv_point_precise_t pt(float x, float y) {
    return {static_cast<lv_value_precise_t>(x), static_cast<lv_value_precise_t>(y)};
}

// Iniciales de los días en el eje de la semana
const char* const kDayLetters[7] = {"Do", "Lu", "Ma", "Mi", "Ju", "Vi", "Sá"};

// Label de un eje: 26 px en texto-3, con ancho fijo y alineado
lv_obj_t* axis_label(lv_obj_t* parent, int32_t w, lv_text_align_t align) {
    lv_obj_t* obj = label(parent, &font_label_26, colors::text_disabled, "");
    lv_obj_set_width(obj, w);
    lv_obj_set_style_text_align(obj, align, 0);
    return obj;
}

}  // namespace

lv_obj_t* HistoryScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);

    static const char* const kRanges[3] = {"Día", "Semana", "Mes"};
    static const char* const kSignals[3] = {"Litros y dosis", "Presiones", "ORP"};
    range_picker_.create(page, 16, 12, 0, 64, kRanges, 3, on_range, this);
    lv_obj_t* signals = signal_picker_.create(page, 0, 12, 0, 64, kSignals, 3, on_signal, this);
    lv_obj_align(signals, LV_ALIGN_TOP_RIGHT, -16, 12);
    range_picker_.set_selected(static_cast<int>(range_));
    signal_picker_.set_selected(static_cast<int>(signal_));

    create_chart(page);
    create_events(page);
    return page;
}

void HistoryScreen::create_chart(lv_obj_t* page) {
    lv_obj_t* c = card(page, 16, 88, 560, 356);

    title_ = label(c, &font_body_32, colors::text, "");
    lv_obj_set_pos(title_, 16, 8);

    lv_obj_t* legend = row(c, 20, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(legend, 16, 48);
    for (Legend& item : legend_) {
        item.obj = row(legend, 8, LV_FLEX_ALIGN_CENTER);
        item.swatch = box(item.obj);
        lv_obj_set_style_radius(item.swatch, 2, 0);
        item.text = label(item.obj, &font_label_26, colors::text_secondary, "");
    }

    // La gráfica se dibuja en draw_plot; ocupa toda la tarjeta
    plot_ = box(c, 0, 0, 558, 354);
    lv_obj_add_event_cb(plot_, draw_plot, LV_EVENT_DRAW_MAIN_END, this);

    // Cifras del eje: terminan en x 76. "3,000" no cabe en 76 px: el label es
    // más ancho y arranca antes; la tarjeta recorta lo que se salga, como en el canvas.
    for (lv_obj_t*& l : y_labels_) {
        l = axis_label(c, 100, LV_TEXT_ALIGN_RIGHT);
        lv_obj_set_x(l, 76 - 100);
    }
    for (lv_obj_t*& l : y2_labels_) {
        l = axis_label(c, 44, LV_TEXT_ALIGN_LEFT);
    }
    // Marcas de abajo: en el canvas miden 80 px pero "14 sep" no cabe y se
    // partiría en dos renglones. Miden 100 y se acomodan con el mismo centro.
    for (lv_obj_t*& l : x_labels_) {
        l = axis_label(c, 100, LV_TEXT_ALIGN_CENTER);
        lv_obj_set_y(l, 316);
    }

    // Señal de una fase que todavía no está: candado y Próximamente
    locked_ = row(c, 10, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(locked_, LV_ALIGN_CENTER, 0, 20);
    icon(locked_, &icon_candado_32, colors::text_disabled);
    label(locked_, &font_body_32, colors::text_disabled, "");
}

void HistoryScreen::create_events(lv_obj_t* page) {
    lv_obj_t* c = card(page, 588, 88, 420, 356);

    lv_obj_t* title = label(c, &font_body_32, colors::text, "Eventos");
    lv_obj_set_pos(title, 16, 6);

    for (size_t i = 0; i < events_.size(); ++i) {
        EventRow& r = events_[i];
        r.obj = box(c, 0, 48 + 61 * static_cast<int32_t>(i), 418, 61);
        if (i + 1 < events_.size()) {
            lv_obj_set_style_border_side(r.obj, LV_BORDER_SIDE_BOTTOM, 0);
            lv_obj_set_style_border_width(r.obj, 1, 0);
            lv_obj_set_style_border_color(r.obj, color(colors::border), 0);
        }
        r.icon = icon(r.obj, &icon_dosis_32, colors::text_secondary);
        lv_obj_set_pos(r.icon, 16, 12);
        r.title = label(r.obj, &font_body_32, colors::text, "");
        lv_obj_set_pos(r.title, 60, 0);
        r.when = label(r.obj, &font_label_26, colors::text_secondary, "");
        lv_obj_set_pos(r.when, 60, 30);
    }
}

void HistoryScreen::update(const core::SystemState& s) {
    // Se copia solo lo que se grafica; si nada cambió, la gráfica se queda
    const bool changed = s.days != shown_days_ || s.hours != shown_hours_ || s.orp_7d != shown_orp_ ||
                         s.today != shown_today_ || s.hour != shown_hour_ ||
                         s.has_pressure != shown_pressure_ || s.has_orp != shown_orp_installed_;
    if (changed) {
        shown_days_ = s.days;
        shown_hours_ = s.hours;
        shown_orp_ = s.orp_7d;
        shown_today_ = s.today;
        shown_hour_ = s.hour;
        shown_date_ = core::today(s);
        shown_year_ = s.year;
        shown_pressure_ = s.has_pressure;
        shown_orp_installed_ = s.has_orp;
        rebuild();
    }
    update_events(s);
}

void HistoryScreen::on_range(int index, void* user_data) {
    auto* self = static_cast<HistoryScreen*>(user_data);
    self->range_ = static_cast<Range>(index);
    self->rebuild();
}

void HistoryScreen::on_signal(int index, void* user_data) {
    auto* self = static_cast<HistoryScreen*>(user_data);
    self->signal_ = static_cast<Signal>(index);
    self->rebuild();
}

void HistoryScreen::rebuild() {
    // Cuántos valores hay y dónde va cada uno en x
    const int hours = shown_hour_ + 1;  // de las 0 a la hora de ahorita
    const int count = range_ == Range::day ? hours : range_ == Range::week ? 7 : core::kHistoryDays;
    const int first_day = core::kHistoryDays - count;  // semana y mes: los últimos días
    auto totals = [this, first_day](int i) -> const core::Totals& {
        return range_ == Range::day ? shown_hours_[static_cast<size_t>(i)]
                                    : shown_days_[static_cast<size_t>(first_day + i)];
    };
    auto point_x = [this, count](int i) {
        return range_ == Range::day ? hour_x(i) : band_x(i, count);
    };

    // Lo que dibuja draw_plot empieza vacío
    bars_a_count_ = bars_b_count_ = line1_count_ = line2_count_ = grid_count_ = 0;
    band_ = false;
    ref_y_ = -1;

    const bool installed = signal_ == Signal::liters || (signal_ == Signal::pressure && shown_pressure_) ||
                           (signal_ == Signal::orp && shown_orp_installed_);

    float min = 0.0f;
    float max = 1.0f;
    int ticks[4] = {};
    int tick_count = 0;
    std::array<const char*, 3> names{};
    std::array<uint32_t, 3> swatch_colors{};
    std::array<bool, 3> swatch_square{};  // cuadro para barras y banda, raya para líneas
    int name_count = 0;
    const char* title = "";

    if (signal_ == Signal::liters) {
        max = range_ == Range::day ? 600.0f : 3000.0f;
        const int hour_ticks[4] = {0, 200, 400, 600};
        const int day_ticks[4] = {0, 1000, 2000, 3000};
        const int* src = range_ == Range::day ? hour_ticks : day_ticks;
        std::copy(src, src + 4, ticks);
        tick_count = 4;
        title = range_ == Range::day ? "Litros y dosis por hora"
                : range_ == Range::week ? "Litros y dosis por día"
                                        : "Litros y dosis, 30 días";
        names = {"Litros", "Dosis mL (eje derecho)", nullptr};
        swatch_colors = {colors::water, colors::text_secondary, 0};
        swatch_square = {true, true, false};
        name_count = 2;

        // Litros a la izquierda de su banda y dosis a la derecha
        const int bands = range_ == Range::day ? 24 : count;
        const float w = range_ == Range::day ? 6.0f : range_ == Range::week ? 20.0f : 5.0f;
        const float off = range_ == Range::day ? 3.5f : range_ == Range::week ? 11.0f : 3.0f;
        for (int i = 0; i < count; ++i) {
            const core::Totals& t = totals(i);
            const float cx = band_x(i, bands);
            bars_a_[static_cast<size_t>(bars_a_count_++)] = {px(cx - off - w / 2), px(value_y(t.liters, min, max)),
                                                             px(w)};
            if (t.dose_ml > 0) {
                bars_b_[static_cast<size_t>(bars_b_count_++)] = {px(cx + off - w / 2), px(value_y(t.dose_ml, 0, 60)),
                                                                 px(w)};
            }
        }
    } else if (signal_ == Signal::pressure) {
        max = 60.0f;
        const int p_ticks[4] = {0, 20, 40, 60};
        std::copy(p_ticks, p_ticks + 4, ticks);
        tick_count = 4;
        title = "P1 y P2 · psi";
        names = {"P1", "P2", "Caída del tren"};
        swatch_colors = {colors::text, colors::text_secondary, colors::secondary_pressed};
        swatch_square = {false, false, true};
        name_count = 3;
        for (int i = 0; i < count; ++i) {
            const core::Totals& t = totals(i);
            line1_[static_cast<size_t>(line1_count_++)] = pt(point_x(i), value_y(t.p1_psi, min, max));
            line2_[static_cast<size_t>(line2_count_++)] = pt(point_x(i), value_y(t.p2_psi, min, max));
        }
        line1_color_ = colors::text;
        band_ = true;
    } else {
        min = 400.0f;
        max = 800.0f;
        const int o_ticks[3] = {400, 600, 800};
        std::copy(o_ticks, o_ticks + 3, ticks);
        tick_count = 3;
        title = "ORP de la cisterna · mV";
        names = {"ORP", "Oxidante desde 650", nullptr};
        swatch_colors = {colors::water, colors::ok, 0};
        swatch_square = {false, false, false};
        name_count = 2;
        auto orp_y = [min, max](int mv) { return value_y(std::clamp(static_cast<float>(mv), min, max), min, max); };
        if (range_ == Range::week) {
            // La semana de ORP es de una muestra por hora: las 168 de orp_7d
            for (int i = 0; i < core::kOrpSamples7d; ++i) {
                const float x = kL + static_cast<float>(i) * kW / static_cast<float>(core::kOrpSamples7d - 1);
                line1_[static_cast<size_t>(line1_count_++)] = pt(x, orp_y(shown_orp_[static_cast<size_t>(i)]));
            }
        } else {
            for (int i = 0; i < count; ++i) {
                line1_[static_cast<size_t>(line1_count_++)] = pt(point_x(i), orp_y(totals(i).orp_mv));
            }
        }
        line1_color_ = colors::water;
        ref_y_ = px(value_y(650.0f, min, max));
    }

    // Título, leyenda y ejes
    set_text(title_, title);
    for (size_t i = 0; i < legend_.size(); ++i) {
        const bool used = static_cast<int>(i) < name_count && installed;
        lv_obj_set_hidden(legend_[i].obj, !used);
        if (!used) continue;
        set_text(legend_[i].text, names[i]);
        lv_obj_set_size(legend_[i].swatch, swatch_square[i] ? 18 : 24, swatch_square[i] ? 18 : 4);
        fill(legend_[i].swatch, swatch_colors[i]);
    }

    char buf[16];
    for (int i = 0; i < static_cast<int>(y_labels_.size()); ++i) {
        const bool used = i < tick_count && installed;
        lv_obj_set_hidden(y_labels_[static_cast<size_t>(i)], !used);
        if (!used) continue;
        const int32_t y = px(value_y(static_cast<float>(ticks[i]), min, max));
        fmt::thousands(buf, sizeof buf, static_cast<uint32_t>(ticks[i]));
        set_text(y_labels_[static_cast<size_t>(i)], buf);
        lv_obj_set_y(y_labels_[static_cast<size_t>(i)], y - 16);
        if (ticks[i] != static_cast<int>(min)) grid_y_[static_cast<size_t>(grid_count_++)] = y;
    }
    const int dose_ticks[3] = {0, 30, 60};
    for (size_t i = 0; i < y2_labels_.size(); ++i) {
        const bool used = signal_ == Signal::liters;
        lv_obj_set_hidden(y2_labels_[i], !used);
        snprintf(buf, sizeof buf, "%d", dose_ticks[i]);
        set_text(y2_labels_[i], buf);
        lv_obj_set_pos(y2_labels_[i], 516, px(value_y(static_cast<float>(dose_ticks[i]), 0, 60)) - 16);
    }

    // Horas, días o fechas abajo. La primera va pegada a la izquierda, la
    // última a la derecha y las demás centradas en su x (x del canvas, sin el
    // borde: por eso el + 1).
    struct Mark {
        float x;
        char text[12];
        lv_text_align_t align;
    };
    std::array<Mark, 7> marks{};
    int mark_count = 0;
    auto add = [&marks, &mark_count](float x, const char* text, lv_text_align_t align) {
        Mark& m = marks[static_cast<size_t>(mark_count++)];
        m.x = x + 1.0f;
        snprintf(m.text, sizeof m.text, "%s", text);
        m.align = align;
    };
    if (range_ == Range::day) {
        add(hour_x(0), "0:00", LV_TEXT_ALIGN_LEFT);
        add(hour_x(6), "6:00", LV_TEXT_ALIGN_CENTER);
        add(hour_x(12), "12:00", LV_TEXT_ALIGN_CENTER);
        add(hour_x(18), "18:00", LV_TEXT_ALIGN_CENTER);
        add(hour_x(24), "24:00", LV_TEXT_ALIGN_RIGHT);
    } else if (range_ == Range::week) {
        for (int i = 0; i < 7; ++i) {
            const core::Date d = core::date_days_ago(shown_date_, shown_year_, static_cast<uint32_t>(6 - i));
            add(band_x(i, 7), i == 6 ? "Hoy" : kDayLetters[d.weekday % 7], LV_TEXT_ALIGN_CENTER);
        }
    } else {
        const int at[3] = {0, 10, 20};
        for (int i = 0; i < 3; ++i) {
            const core::Date d = core::date_days_ago(shown_date_, shown_year_, static_cast<uint32_t>(29 - at[i]));
            snprintf(buf, sizeof buf, "%u %s", static_cast<unsigned>(d.day), fmt::month_short(d.month));
            add(band_x(at[i], 30), buf, i == 0 ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_CENTER);
        }
        add(band_x(29, 30), "Hoy", LV_TEXT_ALIGN_RIGHT);
    }
    for (size_t i = 0; i < x_labels_.size(); ++i) {
        const bool used = static_cast<int>(i) < mark_count && installed;
        lv_obj_set_hidden(x_labels_[i], !used);
        if (!used) continue;
        const Mark& m = marks[i];
        const float left = m.align == LV_TEXT_ALIGN_LEFT ? m.x - 8 : m.align == LV_TEXT_ALIGN_RIGHT ? m.x - 92 : m.x - 50;
        set_text(x_labels_[i], m.text);
        lv_obj_set_style_text_align(x_labels_[i], m.align, 0);
        lv_obj_set_x(x_labels_[i], px(left));
    }

    // Sin la fase, solo el título y el aviso
    lv_obj_set_hidden(plot_, !installed);
    lv_obj_set_hidden(locked_, installed);
    if (!installed) {
        set_text(lv_obj_get_child(locked_, -1),
                 signal_ == Signal::pressure ? "Próximamente · fase 1.5" : "Próximamente · fase 2");
    }
    lv_obj_invalidate(plot_);
}

void HistoryScreen::draw_plot(lv_event_t* e) {
    auto* self = static_cast<HistoryScreen*>(lv_event_get_user_data(e));
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_area_t area;
    lv_obj_get_coords(self->plot_, &area);
    const int32_t x0 = area.x1;
    const int32_t y0 = area.y1;
    const int32_t left = x0 + px(kL);
    const int32_t right = x0 + px(kL + kW);

    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.bg_opa = LV_OPA_COVER;

    // Rejilla de 1 px
    rect.bg_color = color(colors::border);
    for (int i = 0; i < self->grid_count_; ++i) {
        const int32_t y = y0 + self->grid_y_[static_cast<size_t>(i)];
        const lv_area_t a = {left, y, right, y};
        lv_draw_rect(layer, &rect, &a);
    }

    // Banda de la caída: un cuadrilátero entre P1 y P2 por cada tramo, en dos triángulos
    if (self->band_) {
        lv_draw_triangle_dsc_t tri;
        lv_draw_triangle_dsc_init(&tri);
        tri.color = color(colors::secondary_pressed);
        tri.opa = LV_OPA_COVER;
        for (int i = 0; i + 1 < self->line1_count_; ++i) {
            const lv_point_precise_t& a = self->line1_[static_cast<size_t>(i)];
            const lv_point_precise_t& b = self->line1_[static_cast<size_t>(i + 1)];
            const lv_point_precise_t& c = self->line2_[static_cast<size_t>(i + 1)];
            const lv_point_precise_t& d = self->line2_[static_cast<size_t>(i)];
            tri.p[0] = pt(x0 + a.x, y0 + a.y);
            tri.p[1] = pt(x0 + b.x, y0 + b.y);
            tri.p[2] = pt(x0 + c.x, y0 + c.y);
            lv_draw_triangle(layer, &tri);
            tri.p[1] = pt(x0 + c.x, y0 + c.y);
            tri.p[2] = pt(x0 + d.x, y0 + d.y);
            lv_draw_triangle(layer, &tri);
        }
    }

    // Barras: litros en agua y dosis en texto-2, hasta la línea de abajo
    const int32_t base = y0 + px(kB);
    rect.bg_color = color(colors::water);
    for (int i = 0; i < self->bars_a_count_; ++i) {
        const Bar& b = self->bars_a_[static_cast<size_t>(i)];
        const lv_area_t a = {x0 + b.x, y0 + b.y, x0 + b.x + b.w - 1, base};
        lv_draw_rect(layer, &rect, &a);
    }
    rect.bg_color = color(colors::text_secondary);
    for (int i = 0; i < self->bars_b_count_; ++i) {
        const Bar& b = self->bars_b_[static_cast<size_t>(i)];
        const lv_area_t a = {x0 + b.x, y0 + b.y, x0 + b.x + b.w - 1, base};
        lv_draw_rect(layer, &rect, &a);
    }

    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.width = 2;

    // Umbral de 650 mV: punteado de 8 con huecos de 6
    if (self->ref_y_ >= 0) {
        line.color = color(colors::ok);
        line.dash_width = 8;
        line.dash_gap = 6;
        line.p1 = pt(static_cast<float>(left), static_cast<float>(y0 + self->ref_y_));
        line.p2 = pt(static_cast<float>(right), static_cast<float>(y0 + self->ref_y_));
        lv_draw_line(layer, &line);
        line.dash_width = 0;
        line.dash_gap = 0;
    }

    // Líneas de 3 px con las puntas redondas: P2 abajo de P1
    line.width = 3;
    line.round_start = 1;
    line.round_end = 1;
    auto polyline = [&](const lv_point_precise_t* src, lv_point_precise_t* dst, int n, uint32_t hex) {
        if (n < 2) return;
        for (int i = 0; i < n; ++i) dst[i] = pt(x0 + src[i].x, y0 + src[i].y);
        line.color = color(hex);
        line.points = dst;
        line.point_cnt = n;
        lv_draw_line(layer, &line);
    };
    polyline(self->line2_.data(), self->line2_abs_.data(), self->line2_count_, colors::text_secondary);
    polyline(self->line1_.data(), self->line1_abs_.data(), self->line1_count_, self->line1_color_);

    // Línea de abajo de 2 px
    rect.bg_color = color(colors::border_control);
    const lv_area_t floor = {left, base - 1, right, base};
    lv_draw_rect(layer, &rect, &floor);
}

void HistoryScreen::update_events(const core::SystemState& s) {
    char title[40];
    char when[48];
    char date[24];
    char time[8];
    // Los avisos de red no van aquí: se ven en Alarmas y eventos
    size_t next = 0;
    for (size_t i = 0; i < events_.size(); ++i) {
        EventRow& r = events_[i];
        while (next < s.recent_event_count && s.recent_events[next].kind == core::EventKind::offline) ++next;
        const bool used = next < s.recent_event_count;
        lv_obj_set_hidden(r.obj, !used);
        if (!used) continue;
        const core::Event& ev = s.recent_events[next++];

        // Cuándo: "Hoy 06:10" o "Vie 2 oct 06:25", más el detalle si hay
        const uint32_t ago = s.today >= ev.day ? s.today - ev.day : 0;
        const core::Date d = core::date_days_ago(core::today(s), s.year, ago);
        if (ago == 0) {
            snprintf(date, sizeof date, "Hoy");
        } else {
            snprintf(date, sizeof date, "%s %u %s", fmt::weekday_short(d.weekday), static_cast<unsigned>(d.day),
                     fmt::month_short(d.month));
        }
        fmt::clock(time, sizeof time, ev.at);
        const char* detail = ev.kind == core::EventKind::cartridge    ? " · cambio"
                             : ev.kind == core::EventKind::dose_typed ? " · otra"
                                                                      : "";
        snprintf(when, sizeof when, "%s %s%s", date, time, detail);

        uint32_t tone = colors::text;
        uint32_t icon_tone = colors::text_secondary;
        const lv_image_dsc_t* src = &icon_dosis_32;
        switch (ev.kind) {
            case core::EventKind::dose:
            case core::EventKind::dose_typed:
                snprintf(title, sizeof title, "Dosis %u mL", static_cast<unsigned>(ev.value));
                break;
            case core::EventKind::cartridge:
                snprintf(title, sizeof title, "Cartucho %s", fmt::filter_name(ev.value));
                src = &icon_cartucho_32;
                break;
            case core::EventKind::purge:
                snprintf(title, sizeof title, "Purga del prefiltro");
                src = &icon_cartucho_32;
                break;
            case core::EventKind::alarm:
                snprintf(title, sizeof title, "%s", fmt::alarm_title(ev.value));
                tone = icon_tone = colors::alarm;
                src = &icon_alarma_32;
                break;
            case core::EventKind::offline:  // no llega: se brincan arriba
                break;
            case core::EventKind::drop_warning:
            case core::EventKind::drop_alarm: {
                const bool alarm = ev.kind == core::EventKind::drop_alarm;
                snprintf(title, sizeof title, "Caída del tren %u psi", static_cast<unsigned>(ev.value));
                tone = icon_tone = alarm ? colors::alarm : colors::warning;
                src = alarm ? &icon_alarma_32 : &icon_advertencia_32;
                break;
            }
        }
        set_text(r.title, title);
        set_text(r.when, when);
        lv_obj_set_style_text_color(r.title, color(tone), 0);
        lv_image_set_src(r.icon, src);
        set_icon_color(r.icon, icon_tone);
    }
}

}  // namespace ui
