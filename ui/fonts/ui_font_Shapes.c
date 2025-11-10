/*******************************************************************************
 * Size: 24 px
 * Bpp: 4
 * Opts: --bpp 4 --size 24 --font /Users/adamclarkson/dev/m4_ecosystem/m4_mcu/SQUARELINE/assets/Arial Black.ttf -o /Users/adamclarkson/dev/m4_ecosystem/m4_mcu/SQUARELINE/assets/ui_font_Shapes.c --format lvgl --symbols ▲▼▶◀
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_SHAPES
#define UI_FONT_SHAPES 1
#endif

#if UI_FONT_SHAPES

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+25B2 "▲" */
    0x0, 0xfc, 0x40, 0x1f, 0xfc, 0x58, 0x70, 0xf,
    0xfe, 0x19, 0x2c, 0x80, 0x7f, 0xf0, 0xe0, 0x5,
    0xc0, 0x3f, 0xf8, 0x24, 0xa0, 0x9, 0x0, 0xff,
    0xe0, 0xc0, 0x4, 0x2e, 0x1, 0xff, 0x12, 0x80,
    0x69, 0x0, 0xff, 0xa0, 0x3, 0x85, 0xc0, 0x3f,
    0x89, 0x40, 0x3d, 0x20, 0x1f, 0xd0, 0x1, 0xf0,
    0xb8, 0x7, 0xc4, 0xa0, 0x1f, 0xa4, 0x3, 0xe8,
    0x0, 0xfe, 0x17, 0x0, 0xe3, 0x50, 0xf, 0xf4,
    0x80, 0x74, 0x0, 0x7f, 0xc2, 0xe0, 0x11, 0xa0,
    0x7, 0xff, 0x2, 0x40, 0x28, 0x0, 0xff, 0xe0,
    0x8b, 0x81, 0xa0, 0x7, 0xff, 0xa, 0x40,

    /* U+25BC "▼" */
    0x14, 0x4f, 0xfe, 0x20, 0x2, 0xef, 0xff, 0x87,
    0x20, 0x2e, 0x1, 0xff, 0xc2, 0x80, 0x4, 0x80,
    0x7f, 0xf0, 0x49, 0x40, 0x2, 0xe0, 0x1f, 0xfc,
    0x8, 0x0, 0xd2, 0x1, 0xff, 0x12, 0x80, 0x61,
    0x70, 0xf, 0xf4, 0x0, 0x7a, 0x40, 0x3f, 0x89,
    0x40, 0x3c, 0x2e, 0x1, 0xfa, 0x0, 0x3f, 0x48,
    0x7, 0xc4, 0xa0, 0x1f, 0x85, 0xc0, 0x3d, 0x0,
    0x1f, 0xe9, 0x0, 0xe2, 0x50, 0xf, 0xf0, 0xb8,
    0x6, 0x80, 0xf, 0xfe, 0x4, 0x80, 0x44, 0xa0,
    0x1f, 0xfc, 0x1, 0x70, 0x4, 0x0, 0x7f, 0xf0,
    0xa4, 0x9, 0x40, 0x3f, 0xf8, 0x42, 0xf0, 0x1,
    0xff, 0xc4, 0x85, 0x0, 0xfc
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 380, .box_w = 18, .box_h = 17, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 87, .adv_w = 380, .box_w = 18, .box_h = 18, .ofs_x = 3, .ofs_y = -1}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0xa
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 9650, .range_length = 11, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 1,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_Shapes = {
#else
lv_font_t ui_font_Shapes = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 18,          /*The maximum line height required by the font*/
    .base_line = 1,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -3,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_SHAPES*/

