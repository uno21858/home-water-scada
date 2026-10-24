#include "screens/update_screen.h"

#include <algorithm>
#include <cstdio>

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"

// No está en el canvas: centrada, con las piezas del DS (título de 48, cifra
// de 72 con su unidad de 36 y una barra como la de Cartuchos, en agua).

namespace ui {
namespace {

constexpr int32_t kTrackWidth = 640;

const char* failure_text(core::UpdateFailure failure) {
    switch (failure) {
        case core::UpdateFailure::signature:
            return "La firma no es de la llave de la casa";
        case core::UpdateFailure::cut:
            return "Se cortó a medias: vuelve a mandarla";
        case core::UpdateFailure::other:
            break;
    }
    return "No se pudo escribir: vuelve a mandarla";
}

}  // namespace

void UpdateScreen::create(lv_obj_t* layer) {
    root_ = box(layer, 0, 0, 1024, 600);
    fill(root_, colors::background);
    lv_obj_set_clickable(root_, true);  // se come los toques: que nadie mueva nada mientras

    title_ = label(root_, &font_title_xl_48, colors::text, "");
    lv_obj_align(title_, LV_ALIGN_TOP_MID, 0, 128);
    versions_ = label(root_, &font_body_32, colors::text_secondary, "");
    lv_obj_align(versions_, LV_ALIGN_TOP_MID, 0, 200);

    // Cifra y unidad en la misma línea base (docs/diseno/ds/lvgl.md)
    amount_ = row(root_, 4, LV_FLEX_ALIGN_END);
    lv_obj_align(amount_, LV_ALIGN_TOP_MID, 0, 262);
    pct_ = label(amount_, &font_digits_72, colors::text, "0");
    lv_obj_t* unit = label(amount_, &font_title_36, colors::text_secondary, "%");
    lv_obj_set_style_pad_bottom(unit, 2, 0);

    track_ = box(root_, (1024 - kTrackWidth) / 2, 372, kTrackWidth, 16);
    lv_obj_set_style_radius(track_, 8, 0);
    lv_obj_set_style_clip_corner(track_, true, 0);
    fill(track_, colors::panel_raised);
    bar_ = box(track_, 0, 0, 0, 16);
    fill(bar_, colors::water);

    note_ = label(root_, &font_body_32, colors::text_secondary, "");
    lv_obj_align(note_, LV_ALIGN_TOP_MID, 0, 428);

    lv_obj_set_hidden(root_, true);
}

void UpdateScreen::update(const core::SystemState& s) {
    const core::UpdateStatus& u = s.update;
    const bool shown = u.stage != core::UpdateStage::none;
    lv_obj_set_hidden(root_, !shown);
    if (!shown) return;

    const bool failed = u.stage == core::UpdateStage::failed;
    const bool done = u.stage == core::UpdateStage::installed;
    set_text(title_, failed ? "No se actualizó" : done ? "Lista" : "Actualizando");
    set_text_color(title_, failed ? colors::warning : colors::text);

    char buf[64];
    if (failed) {
        snprintf(buf, sizeof buf, "Sigue la %s", s.firmware);
    } else {
        snprintf(buf, sizeof buf, "De %s a %s", s.firmware, u.version);
    }
    set_text(versions_, buf);

    lv_obj_set_hidden(amount_, failed);
    lv_obj_set_hidden(track_, failed);
    const unsigned pct = done ? 100u : std::min<unsigned>(u.pct, 100u);
    snprintf(buf, sizeof buf, "%u", pct);
    set_text(pct_, buf);
    lv_obj_set_width(bar_, static_cast<int32_t>(kTrackWidth * static_cast<int32_t>(pct) / 100));

    const char* note = "No la desconectes: se reinicia sola";
    if (done) note = u.waits_for_mixer ? "Se reinicia cuando acabe la bomba R" : "Reiniciando";
    if (failed) note = failure_text(u.failure);
    set_text(note_, note);
    set_text_color(note_, failed ? colors::text : colors::text_secondary);
    // Sin cifra ni barra, el porqué sube a donde iba la cifra
    lv_obj_align(note_, LV_ALIGN_TOP_MID, 0, failed ? 262 : 428);
}

}  // namespace ui
