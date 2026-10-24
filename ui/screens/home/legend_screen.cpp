#include "screens/home/legend_screen.h"

#include "components/primitives.h"
#include "theme/colors.h"
#include "theme/fonts.h"
#include "theme/pid.h"

// Medidas tomadas del artboard Leyenda del canvas (cada y menos 64). Sin la
// tarjeta "Fase futura": lo que no está instalado ya no se muestra.
// Rejilla de 3 columnas de 320 px con 16 de hueco; renglones de 76 con 12.

namespace ui {
namespace {

// Una tarjeta de la rejilla: el dibujo va a la izquierda, en 12, 7
lv_obj_t* tile(lv_obj_t* page, int index, const char* title, const char* note) {
    const int32_t x = 16 + (index % 3) * (320 + 16);
    const int32_t y = 12 + (index / 3) * (76 + 12);
    lv_obj_t* c = card(page, x, y, 320, 76);
    lv_obj_set_pos(label(c, &font_label_26, colors::text, title), 100, 4);
    lv_obj_set_pos(label(c, &font_label_26, colors::text_secondary, note), 100, 36);
    return c;
}

void symbol(lv_obj_t* c, const lv_image_dsc_t* src) {
    lv_obj_t* img = lv_image_create(c);
    lv_image_set_src(img, src);
    lv_obj_set_pos(img, 12, 7);
}

// Recuadro chico de 40 px de alto con borde de 2 (Etiqueta)
lv_obj_t* small_box(lv_obj_t* c, int32_t x, int32_t w, uint32_t bg) {
    lv_obj_t* b = box(c, x, 18, w, 40);
    fill(b, bg);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_set_style_border_color(b, color(colors::border), 0);
    lv_obj_set_style_radius(b, 6, 0);
    return b;
}

}  // namespace

lv_obj_t* LegendScreen::create(lv_obj_t* parent) {
    lv_obj_t* page = box(parent, 0, 0, 1024, 456);

    lv_obj_t* c = tile(page, 0, "Con flujo", "puntos = agua");
    pipe_.add(c, 12, 32, 72, 10, FlowPipes::Dir::right);
    pipe_.set_flow(8.0f);  // un ejemplo: corre siempre, haya flujo o no

    c = tile(page, 1, "Sin flujo", "o sin sensor");
    fill(box(c, 12, 32, 72, 10), colors::pipe);

    struct Symbol {
        const lv_image_dsc_t* src;
        const char* title;
        const char* note;
    };
    static const Symbol kSymbols[] = {
        {&pid_leyenda_valvula, "Válvula", "V1 a V5, a mano"},
        {&pid_leyenda_prefiltro, "Prefiltro", "malla y purga"},
        {&pid_leyenda_vasos, "Vaso", "lleno o vacío"},
        {&pid_leyenda_toma, "Toma", "presión P1, P2"},
        {&pid_leyenda_medidor, "Medidor", "de flujo F"},
        {&pid_leyenda_boca, "Boca", "punto B"},
        {&pid_leyenda_cisterna, "Cisterna", "con sonda ORP"},
        {&pid_leyenda_bomba, "Bomba R", "gira = prendida"},
        {&pid_leyenda_conector, "Conector", "entra o sale"},
    };
    int index = 2;
    for (const Symbol& s : kSymbols) symbol(tile(page, index++, s.title, s.note), s.src);

    // Etiqueta: como las de P1 y P2 del Tren
    c = tile(page, index++, "Etiqueta", "tócala y abre");
    lv_obj_t* tag = small_box(c, 8, 86, colors::panel_raised);
    lv_obj_set_flex_flow(tag, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tag, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tag, 4, 0);
    label(tag, &font_label_26, colors::text, "42");
    label(tag, &font_label_26, colors::text_secondary, "psi");

    // Colores: verde, ámbar y rojo
    c = tile(page, index++, "Colores", "con su palabra");
    lv_obj_t* dots = row(c, 6, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(dots, 12, 26);
    for (const uint32_t tone : {colors::ok, colors::warning, colors::alarm}) {
        lv_obj_t* d = box(dots, 0, 0, 20, 20);
        lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
        fill(d, tone);
    }
    return page;
}

}  // namespace ui
