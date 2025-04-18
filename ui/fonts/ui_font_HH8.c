/*******************************************************************************
 * Size: 33 px
 * Bpp: 4
 * Opts: --bpp 4 --size 33 --font /Users/adamclarkson/dev/m4_ecosystem/m4_mcu/SQUARELINE/assets/HH_MONO_NUM.ttf -o /Users/adamclarkson/dev/m4_ecosystem/m4_mcu/SQUARELINE/assets/ui_font_HH8.c --format lvgl -r 0x20-0x39 --symbols 0x20-0x39 --no-compress --no-prefilter
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_HH8
#define UI_FONT_HH8 1
#endif

#if UI_FONT_HH8

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0021 "!" */
    0x0, 0x0, 0x3f, 0xff, 0xf0, 0x0, 0x0, 0x6f,
    0xff, 0xc0, 0x0, 0x0, 0xaf, 0xff, 0x90, 0x0,
    0x0, 0xdf, 0xff, 0x60, 0x0, 0x0, 0xff, 0xff,
    0x20, 0x0, 0x4, 0xff, 0xff, 0x0, 0x0, 0x7,
    0xff, 0xfc, 0x0, 0x0, 0xa, 0xff, 0xf8, 0x0,
    0x0, 0xe, 0xff, 0xf5, 0x0, 0x0, 0x1f, 0xff,
    0xf2, 0x0, 0x0, 0x4f, 0xff, 0xe0, 0x0, 0x0,
    0x8f, 0xff, 0xb0, 0x0, 0x0, 0xbf, 0xff, 0x80,
    0x0, 0x0, 0xef, 0xff, 0x40, 0x0, 0x2, 0xff,
    0xff, 0x10, 0x0, 0x3, 0xaa, 0xa9, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x11, 0x10,
    0x0, 0x0, 0xe, 0xff, 0xf5, 0x0, 0x0, 0x1f,
    0xff, 0xf2, 0x0, 0x0, 0x4f, 0xff, 0xe0, 0x0,
    0x0, 0x8f, 0xff, 0xb0, 0x0, 0x0, 0xbf, 0xff,
    0x80, 0x0, 0x0,

    /* U+0022 "\"" */
    0x0, 0x4f, 0xff, 0x0, 0x2f, 0xff, 0x20, 0x7,
    0xff, 0xd0, 0x6, 0xff, 0xf0, 0x0, 0xbf, 0xfa,
    0x0, 0x9f, 0xfb, 0x0, 0xe, 0xff, 0x60, 0xc,
    0xff, 0x80, 0x1, 0xff, 0xf3, 0x0, 0xff, 0xf5,
    0x0, 0x5f, 0xff, 0x0, 0x3f, 0xff, 0x10, 0x8,
    0xff, 0xc0, 0x6, 0xff, 0xe0, 0x0, 0xbf, 0xf9,
    0x0, 0xaf, 0xfb, 0x0, 0xf, 0xff, 0x60, 0xd,
    0xff, 0x70, 0x2, 0xff, 0xf2, 0x0, 0xff, 0xf4,
    0x0, 0x16, 0x65, 0x0, 0x16, 0x66, 0x0, 0x0,

    /* U+0023 "#" */
    0x0, 0x0, 0x0, 0x1f, 0xfa, 0x0, 0xb, 0xff,
    0x0, 0x0, 0x0, 0x0, 0x4, 0xff, 0x60, 0x0,
    0xef, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x7f, 0xf3,
    0x0, 0x2f, 0xf9, 0x0, 0x0, 0x0, 0x0, 0xb,
    0xff, 0x0, 0x5, 0xff, 0x50, 0x0, 0x0, 0x0,
    0x0, 0xef, 0xc0, 0x0, 0x8f, 0xf2, 0x0, 0x0,
    0x0, 0x0, 0x2f, 0xf9, 0x0, 0xc, 0xfe, 0x0,
    0x0, 0x0, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfe, 0x0, 0xc, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xb0, 0x0, 0x66, 0x6e, 0xff,
    0x66, 0x6a, 0xff, 0x96, 0x63, 0x0, 0x0, 0x0,
    0xef, 0xc0, 0x0, 0x9f, 0xf2, 0x0, 0x0, 0x0,
    0x0, 0x2f, 0xf9, 0x0, 0xc, 0xff, 0x0, 0x0,
    0x0, 0x0, 0x5, 0xff, 0x60, 0x0, 0xff, 0xc0,
    0x0, 0x0, 0x0, 0x0, 0x8f, 0xf3, 0x0, 0x2f,
    0xf9, 0x0, 0x0, 0x0, 0x0, 0xb, 0xff, 0x0,
    0x5, 0xff, 0x60, 0x0, 0x0, 0x16, 0x66, 0xef,
    0xe6, 0x66, 0xbf, 0xf8, 0x66, 0x20, 0x5, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf3, 0x0,
    0x8f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x0, 0x0, 0x0, 0x8f, 0xf3, 0x0, 0x2f, 0xf9,
    0x0, 0x0, 0x0, 0x0, 0xb, 0xff, 0x0, 0x5,
    0xff, 0x50, 0x0, 0x0, 0x0, 0x0, 0xef, 0xc0,
    0x0, 0x8f, 0xf2, 0x0, 0x0, 0x0, 0x0, 0x2f,
    0xf9, 0x0, 0xc, 0xfe, 0x0, 0x0, 0x0, 0x0,
    0x5, 0xff, 0x50, 0x0, 0xff, 0xb0, 0x0, 0x0,
    0x0, 0x0, 0x9f, 0xf2, 0x0, 0x3f, 0xf7, 0x0,
    0x0, 0x0, 0x0,

    /* U+0024 "$" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x8f, 0xfc, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xb, 0xff,
    0x90, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xdf, 0xf6, 0x0, 0x0, 0x0, 0x0, 0x3, 0xcf,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x0, 0x1,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x0,
    0x0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x80, 0x0, 0xb, 0xff, 0xf6, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0xef, 0xff, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x2f, 0xff, 0xc0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5, 0xff,
    0xfa, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x8f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfa, 0x0,
    0x0, 0x7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xf3, 0x0, 0x0, 0x7, 0xaa, 0xaa, 0xaa, 0xab,
    0xff, 0xff, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0xf, 0xff, 0xf0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x2, 0xff, 0xfc, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x6f, 0xff, 0x80, 0x0,
    0x4, 0x55, 0x55, 0x55, 0x55, 0x5d, 0xff, 0xf5,
    0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x10, 0x0, 0x3f, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xa0, 0x0, 0x7, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xfe, 0x90, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x9f, 0xfa, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xd, 0xff, 0x60, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0xff, 0xf3, 0x0, 0x0,
    0x0, 0x0, 0x0,

    /* U+0025 "%" */
    0x0, 0x5e, 0xff, 0xff, 0xff, 0xd1, 0x0, 0x0,
    0x0, 0x5f, 0xf4, 0x0, 0xe, 0xff, 0xff, 0xff,
    0xff, 0x20, 0x0, 0x0, 0x3f, 0xf7, 0x0, 0x3,
    0xff, 0xea, 0xab, 0xff, 0xf0, 0x0, 0x0, 0x1d,
    0xfb, 0x0, 0x0, 0x6f, 0xf8, 0x0, 0x1f, 0xfc,
    0x0, 0x0, 0xb, 0xfd, 0x0, 0x0, 0x9, 0xff,
    0x40, 0x5, 0xff, 0x80, 0x0, 0x8, 0xff, 0x20,
    0x0, 0x0, 0xdf, 0xf1, 0x0, 0x8f, 0xf5, 0x0,
    0x5, 0xff, 0x50, 0x0, 0x0, 0xf, 0xfd, 0x0,
    0xc, 0xff, 0x20, 0x2, 0xff, 0x80, 0x0, 0x0,
    0x4, 0xff, 0x90, 0x0, 0xff, 0xe0, 0x0, 0xdf,
    0xb0, 0x0, 0x0, 0x0, 0x7f, 0xfe, 0xcc, 0xdf,
    0xfb, 0x0, 0xaf, 0xd1, 0x0, 0x0, 0x0, 0xa,
    0xff, 0xff, 0xff, 0xff, 0x60, 0x7f, 0xf3, 0x0,
    0x0, 0x0, 0x0, 0x6f, 0xff, 0xff, 0xff, 0xb0,
    0x4f, 0xf5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x2e, 0xf9, 0x18, 0xaa, 0xaa,
    0xaa, 0x60, 0x0, 0x0, 0x0, 0x0, 0xd, 0xfc,
    0xc, 0xff, 0xff, 0xff, 0xff, 0x20, 0x0, 0x0,
    0x0, 0xa, 0xfe, 0x11, 0xff, 0xff, 0xff, 0xff,
    0xf1, 0x0, 0x0, 0x0, 0x7, 0xff, 0x30, 0x5f,
    0xf9, 0x0, 0x1f, 0xfd, 0x0, 0x0, 0x0, 0x3,
    0xff, 0x60, 0x8, 0xff, 0x50, 0x3, 0xff, 0xa0,
    0x0, 0x0, 0x1, 0xef, 0x90, 0x0, 0xbf, 0xf2,
    0x0, 0x7f, 0xf7, 0x0, 0x0, 0x0, 0xcf, 0xc0,
    0x0, 0xf, 0xff, 0x0, 0xa, 0xff, 0x40, 0x0,
    0x0, 0x9f, 0xe1, 0x0, 0x2, 0xff, 0xc0, 0x0,
    0xdf, 0xf0, 0x0, 0x0, 0x6f, 0xf4, 0x0, 0x0,
    0x5f, 0xf8, 0x0, 0x1f, 0xfd, 0x0, 0x0, 0x3f,
    0xf7, 0x0, 0x0, 0x8, 0xff, 0xec, 0xcd, 0xff,
    0xa0, 0x0, 0x1e, 0xfa, 0x0, 0x0, 0x0, 0xbf,
    0xff, 0xff, 0xff, 0xf5, 0x0, 0xb, 0xfd, 0x0,
    0x0, 0x0, 0x6, 0xff, 0xff, 0xff, 0xfa, 0x0,
    0x0,

    /* U+0026 "&" */
    0x0, 0x0, 0x2, 0xae, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xe0, 0x0, 0x0, 0x0, 0x3f, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xb0, 0x0, 0x0, 0x0,
    0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80,
    0x0, 0x0, 0x2, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0x40, 0x0, 0x0, 0x6, 0xff, 0xff,
    0x60, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x9, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xc, 0xff, 0xfd, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff,
    0xf9, 0x0, 0x0, 0x0, 0xbe, 0xee, 0x10, 0x0,
    0x0, 0x2f, 0xff, 0xf6, 0x0, 0x0, 0x0, 0xff,
    0xfe, 0x0, 0x0, 0x0, 0x1f, 0xff, 0xfc, 0x66,
    0x66, 0x68, 0xff, 0xfd, 0x66, 0x66, 0x0, 0x4,
    0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xfc, 0x0, 0x4d, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xf8, 0x1, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf5, 0x5,
    0xff, 0xff, 0x80, 0x0, 0x0, 0x1f, 0xff, 0xd0,
    0x0, 0x0, 0x9, 0xff, 0xff, 0x20, 0x0, 0x0,
    0x4f, 0xff, 0xa0, 0x0, 0x0, 0xc, 0xff, 0xfe,
    0x0, 0x0, 0x0, 0x7f, 0xff, 0x70, 0x0, 0x0,
    0xf, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xbf, 0xff,
    0x40, 0x0, 0x0, 0x3f, 0xff, 0xf8, 0x0, 0x0,
    0x0, 0xef, 0xff, 0x10, 0x0, 0x0, 0x6f, 0xff,
    0xfc, 0x55, 0x55, 0x56, 0xff, 0xff, 0x75, 0x54,
    0x0, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xfb, 0x0, 0xaf, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0x0, 0x7f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0xff,
    0xf4, 0x0, 0x9, 0xef, 0xff, 0xff, 0xff, 0xc3,
    0x1a, 0xef, 0xff, 0xf1, 0x0,

    /* U+0027 "'" */
    0x0, 0x4f, 0xff, 0x0, 0x7, 0xff, 0xd0, 0x0,
    0xbf, 0xfa, 0x0, 0xe, 0xff, 0x60, 0x1, 0xff,
    0xf3, 0x0, 0x5f, 0xff, 0x0, 0x8, 0xff, 0xc0,
    0x0, 0xbf, 0xf9, 0x0, 0xf, 0xff, 0x60, 0x2,
    0xff, 0xf2, 0x0, 0x16, 0x65, 0x0, 0x0,

    /* U+0028 "(" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x67, 0x0,
    0x0, 0x0, 0x0, 0x2, 0xaf, 0xff, 0xe0, 0x0,
    0x0, 0x0, 0x7, 0xff, 0xff, 0xfa, 0x0, 0x0,
    0x0, 0x7, 0xff, 0xf8, 0x31, 0x0, 0x0, 0x0,
    0x3, 0xff, 0xe2, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xcf, 0xf4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3f,
    0xfc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8, 0xff,
    0x60, 0x0, 0x0, 0x0, 0x0, 0x0, 0xcf, 0xf2,
    0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x3, 0xff, 0xb0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x6f, 0xf8, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xa, 0xff, 0x50, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xdf, 0xf1, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xf, 0xfe, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x4, 0xff, 0xb0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x7f, 0xf7, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xa, 0xff, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xef, 0xf1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1f,
    0xfd, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4, 0xff,
    0xa0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8f, 0xf7,
    0x0, 0x0, 0x0, 0x0, 0x0, 0xb, 0xff, 0x30,
    0x0, 0x0, 0x0, 0x0, 0x0, 0xef, 0xf0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x1f, 0xfd, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x4, 0xff, 0xa0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x6f, 0xf9, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x5, 0xff, 0xb0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x3f, 0xff, 0x20, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xef, 0xff, 0x83, 0x0, 0x0, 0x0,
    0x0, 0x5, 0xff, 0xff, 0xfc, 0x0, 0x0, 0x0,
    0x0, 0x4, 0xdf, 0xff, 0x80, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x25, 0x73, 0x0, 0x0, 0x0, 0x0,

    /* U+0029 ")" */
    0x0, 0x0, 0x0, 0x3f, 0xeb, 0x60, 0x0, 0x0,
    0x0, 0x0, 0x7f, 0xff, 0xfd, 0x10, 0x0, 0x0,
    0x0, 0x49, 0xdf, 0xff, 0xc0, 0x0, 0x0, 0x0,
    0x0, 0x6, 0xff, 0xf3, 0x0, 0x0, 0x0, 0x0,
    0x0, 0xbf, 0xf7, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x7f, 0xf8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7f,
    0xf7, 0x0, 0x0, 0x0, 0x0, 0x0, 0x9f, 0xf5,
    0x0, 0x0, 0x0, 0x0, 0x0, 0xcf, 0xf2, 0x0,
    0x0, 0x0, 0x0, 0x0, 0xff, 0xf0, 0x0, 0x0,
    0x0, 0x0, 0x3, 0xff, 0xc0, 0x0, 0x0, 0x0,
    0x0, 0x6, 0xff, 0x80, 0x0, 0x0, 0x0, 0x0,
    0x9, 0xff, 0x50, 0x0, 0x0, 0x0, 0x0, 0xd,
    0xff, 0x20, 0x0, 0x0, 0x0, 0x0, 0xf, 0xfe,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x3f, 0xfb, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x7f, 0xf8, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xaf, 0xf4, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xdf, 0xf1, 0x0, 0x0, 0x0, 0x0,
    0x1, 0xff, 0xe0, 0x0, 0x0, 0x0, 0x0, 0x4,
    0xff, 0xb0, 0x0, 0x0, 0x0, 0x0, 0x7, 0xff,
    0x70, 0x0, 0x0, 0x0, 0x0, 0xb, 0xff, 0x40,
    0x0, 0x0, 0x0, 0x0, 0xe, 0xff, 0x10, 0x0,
    0x0, 0x0, 0x0, 0x2f, 0xfd, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x6f, 0xf8, 0x0, 0x0, 0x0, 0x0,
    0x0, 0xdf, 0xf2, 0x0, 0x0, 0x0, 0x0, 0x7,
    0xff, 0xa0, 0x0, 0x0, 0x0, 0x0, 0x7f, 0xfe,
    0x10, 0x0, 0x0, 0x39, 0xbf, 0xff, 0xe3, 0x0,
    0x0, 0x0, 0x9f, 0xff, 0xfb, 0x20, 0x0, 0x0,
    0x0, 0xce, 0xc8, 0x30, 0x0, 0x0, 0x0, 0x0,

    /* U+002A "*" */
    0x0, 0x0, 0x0, 0x4, 0xff, 0x30, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x4, 0xff, 0x30, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0xff, 0x30,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0xff,
    0x20, 0x0, 0x0, 0x0, 0x3, 0x20, 0x0, 0x2,
    0xff, 0x20, 0x0, 0x2, 0x30, 0xb, 0xfc, 0x71,
    0x2, 0xff, 0x10, 0x17, 0xdf, 0xb0, 0xf, 0xff,
    0xff, 0xb6, 0xff, 0x6b, 0xff, 0xff, 0xf0, 0x3,
    0x8d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc7, 0x20,
    0x0, 0x0, 0x16, 0xbf, 0xff, 0xfa, 0x61, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x3f, 0xff, 0xf4, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x1, 0xef, 0xcc, 0xfe,
    0x10, 0x0, 0x0, 0x0, 0x0, 0xb, 0xff, 0x22,
    0xff, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x7f, 0xf7,
    0x0, 0x6f, 0xf8, 0x0, 0x0, 0x0, 0x4, 0xff,
    0xc0, 0x0, 0xb, 0xff, 0x40, 0x0, 0x0, 0x7,
    0xff, 0x20, 0x0, 0x2, 0xff, 0x80, 0x0, 0x0,
    0x0, 0x46, 0x0, 0x0, 0x0, 0x54, 0x0, 0x0,

    /* U+002B "+" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0xef, 0xf0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1f, 0xfd,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4,
    0xff, 0x90, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x8f, 0xf6, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xb, 0xff, 0x20, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0xff, 0xf0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x2f, 0xfb, 0x0,
    0x0, 0x0, 0x0, 0x8e, 0xee, 0xee, 0xef, 0xff,
    0xfe, 0xee, 0xee, 0xea, 0xb, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x70, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf4, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xe0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x3f, 0xfb, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x6, 0xff, 0x80,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x9f,
    0xf4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xd, 0xff, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xff, 0xe0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x3f, 0xfa, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x3, 0x88, 0x40, 0x0,
    0x0, 0x0, 0x0,

    /* U+002C "," */
    0x0, 0x3, 0x66, 0x63, 0x0, 0xd, 0xff, 0xf5,
    0x0, 0x3f, 0xff, 0xe0, 0x0, 0x9f, 0xff, 0x70,
    0x0, 0xef, 0xff, 0x0, 0x5, 0xff, 0xf9, 0x0,
    0xb, 0xff, 0xf2, 0x0, 0x1f, 0xff, 0xb0, 0x0,
    0x7f, 0xff, 0x40, 0x0,

    /* U+002D "-" */
    0x4d, 0xdd, 0xdd, 0xdd, 0xdd, 0xd3, 0x8f, 0xff,
    0xff, 0xff, 0xff, 0xf1, 0xcf, 0xff, 0xff, 0xff,
    0xff, 0xd0,

    /* U+002E "." */
    0x0, 0x11, 0x11, 0x0, 0x3f, 0xff, 0xf0, 0x6,
    0xff, 0xfc, 0x0, 0xaf, 0xff, 0x90, 0xd, 0xff,
    0xf6, 0x0, 0xff, 0xff, 0x20,

    /* U+002F "/" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xbf,
    0xf5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4,
    0xff, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xd, 0xff, 0x30, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x6f, 0xfa, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xef, 0xf2, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x7, 0xff, 0x90, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x1f, 0xff, 0x10, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x9f, 0xf7, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x2, 0xff, 0xe0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0xa, 0xff, 0x50,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3f, 0xfd,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xcf,
    0xf4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5,
    0xff, 0xb0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xd, 0xff, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x6f, 0xfa, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xef, 0xf1, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x8, 0xff, 0x80, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x1f, 0xfe, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x9f, 0xf6, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x2, 0xff, 0xd0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0xb, 0xff, 0x50,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4f, 0xfc,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xcf,
    0xf3, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5,
    0xff, 0xa0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xd, 0xff, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0,

    /* U+0030 "0" */
    0x0, 0x0, 0x5, 0xcf, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfe, 0xa1, 0x0, 0x0, 0x6f, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0x0, 0x0,
    0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xfc, 0x0, 0x4, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xfb, 0x0, 0x8, 0xff, 0xff,
    0xa6, 0x66, 0x66, 0x66, 0xbf, 0xff, 0xf8, 0x0,
    0xb, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x6f,
    0xff, 0xf5, 0x0, 0xe, 0xff, 0xfc, 0x0, 0x0,
    0x0, 0x0, 0x9f, 0xff, 0xf2, 0x0, 0x2f, 0xff,
    0xf9, 0x0, 0x0, 0x0, 0x0, 0xcf, 0xff, 0xe0,
    0x0, 0x5f, 0xff, 0xf5, 0x0, 0x0, 0x0, 0x0,
    0xff, 0xff, 0xb0, 0x0, 0x8f, 0xff, 0xf2, 0x0,
    0x0, 0x0, 0x3, 0xff, 0xff, 0x80, 0x0, 0xcf,
    0xff, 0xf0, 0x0, 0x0, 0x0, 0x6, 0xff, 0xff,
    0x50, 0x0, 0xff, 0xff, 0xc0, 0x0, 0x0, 0x0,
    0x9, 0xff, 0xff, 0x10, 0x2, 0xff, 0xff, 0x80,
    0x0, 0x0, 0x0, 0xc, 0xff, 0xfe, 0x0, 0x5,
    0xff, 0xff, 0x50, 0x0, 0x0, 0x0, 0xf, 0xff,
    0xfb, 0x0, 0x9, 0xff, 0xff, 0x20, 0x0, 0x0,
    0x0, 0x3f, 0xff, 0xf8, 0x0, 0xc, 0xff, 0xfe,
    0x0, 0x0, 0x0, 0x0, 0x6f, 0xff, 0xf4, 0x0,
    0xf, 0xff, 0xfb, 0x0, 0x0, 0x0, 0x0, 0xaf,
    0xff, 0xf1, 0x0, 0x2f, 0xff, 0xf8, 0x0, 0x0,
    0x0, 0x0, 0xdf, 0xff, 0xe0, 0x0, 0x6f, 0xff,
    0xfe, 0x88, 0x88, 0x88, 0x8b, 0xff, 0xff, 0xb0,
    0x0, 0x8f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0x70, 0x0, 0xaf, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x20, 0x0, 0x7f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8,
    0x0, 0x0, 0x9, 0xef, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfd, 0x70, 0x0, 0x0,

    /* U+0031 "1" */
    0x0, 0x8f, 0xff, 0xff, 0xff, 0x30, 0xb, 0xff,
    0xff, 0xff, 0xf0, 0x0, 0xef, 0xff, 0xff, 0xfd,
    0x0, 0x2f, 0xff, 0xff, 0xff, 0x90, 0x1, 0x55,
    0x8f, 0xff, 0xf6, 0x0, 0x0, 0x8, 0xff, 0xff,
    0x30, 0x0, 0x0, 0xbf, 0xff, 0xf0, 0x0, 0x0,
    0xe, 0xff, 0xfc, 0x0, 0x0, 0x1, 0xff, 0xff,
    0x90, 0x0, 0x0, 0x5f, 0xff, 0xf6, 0x0, 0x0,
    0x8, 0xff, 0xff, 0x30, 0x0, 0x0, 0xbf, 0xff,
    0xf0, 0x0, 0x0, 0xe, 0xff, 0xfc, 0x0, 0x0,
    0x2, 0xff, 0xff, 0x90, 0x0, 0x0, 0x5f, 0xff,
    0xf5, 0x0, 0x0, 0x8, 0xff, 0xff, 0x20, 0x0,
    0x0, 0xcf, 0xff, 0xf0, 0x0, 0x0, 0xf, 0xff,
    0xfc, 0x0, 0x0, 0x2, 0xff, 0xff, 0x80, 0x0,
    0x0, 0x6f, 0xff, 0xf5, 0x0, 0x0, 0x9, 0xff,
    0xff, 0x20, 0x0, 0x0, 0xcf, 0xff, 0xe0, 0x0,
    0x0, 0xf, 0xff, 0xfb, 0x0, 0x0, 0x0,

    /* U+0032 "2" */
    0x0, 0x0, 0x0, 0x2a, 0xef, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xe9, 0x0, 0x0, 0x0, 0x3f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x0,
    0x0, 0xc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xb0, 0x0, 0x2, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xfa, 0x0, 0x0,
    0x6f, 0xff, 0xf6, 0x11, 0x11, 0x11, 0x16, 0xff,
    0xff, 0x70, 0x0, 0x9, 0xff, 0xfe, 0x0, 0x0,
    0x0, 0x0, 0x6f, 0xff, 0xf4, 0x0, 0x0, 0xcf,
    0xff, 0xb0, 0x0, 0x0, 0x0, 0x9, 0xff, 0xff,
    0x10, 0x0, 0x2, 0x22, 0x21, 0x0, 0x0, 0x0,
    0x0, 0xcf, 0xff, 0xd0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xfa, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x6,
    0xff, 0xff, 0x70, 0x0, 0x0, 0x3a, 0xef, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf3, 0x0, 0x0,
    0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xfd, 0x0, 0x0, 0xc, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x30, 0x0, 0x1, 0xff,
    0xff, 0xfc, 0xbb, 0xbb, 0xbb, 0xbb, 0xa8, 0x10,
    0x0, 0x0, 0x5f, 0xff, 0xf7, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x8, 0xff, 0xff,
    0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0xcf, 0xff, 0xe0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xfb, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2,
    0xff, 0xff, 0xa4, 0x44, 0x44, 0x44, 0x44, 0x44,
    0x42, 0x0, 0x0, 0x6f, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x60, 0x0, 0x9, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf2,
    0x0, 0x0, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x0, 0x0, 0xf, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x0,
    0x0,

    /* U+0033 "3" */
    0x0, 0x0, 0x2, 0x9d, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfd, 0x80, 0x0, 0x0, 0x2f, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf5, 0x0, 0x0,
    0xaf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xf8, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xf8, 0x0, 0x3, 0xff, 0xff,
    0x71, 0x11, 0x11, 0x11, 0x7f, 0xff, 0xf5, 0x0,
    0x7, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x6f,
    0xff, 0xf2, 0x0, 0xa, 0xff, 0xfd, 0x0, 0x0,
    0x0, 0x0, 0x9f, 0xff, 0xf0, 0x0, 0x1, 0x22,
    0x21, 0x0, 0x0, 0x0, 0x0, 0xcf, 0xff, 0xb0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xff, 0xff, 0x70, 0x0, 0x0, 0x0, 0x0, 0x1,
    0x55, 0x55, 0x59, 0xff, 0xff, 0x20, 0x0, 0x0,
    0x0, 0x0, 0x6, 0xff, 0xff, 0xff, 0xff, 0xe4,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x9, 0xff, 0xff,
    0xff, 0xfe, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xd, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2f, 0xff,
    0xf9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x2f, 0xff, 0xf6, 0x0, 0x4, 0x66, 0x65,
    0x0, 0x0, 0x0, 0x0, 0x6f, 0xff, 0xf3, 0x0,
    0xd, 0xff, 0xfb, 0x0, 0x0, 0x0, 0x0, 0x9f,
    0xff, 0xf0, 0x0, 0x1f, 0xff, 0xf8, 0x0, 0x0,
    0x0, 0x0, 0xcf, 0xff, 0xc0, 0x0, 0x4f, 0xff,
    0xfb, 0x33, 0x33, 0x33, 0x36, 0xff, 0xff, 0x90,
    0x0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0x50, 0x0, 0x8f, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x10, 0x0, 0x5f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7,
    0x0, 0x0, 0x8, 0xef, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfc, 0x60, 0x0, 0x0,

    /* U+0034 "4" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1e, 0xff,
    0xff, 0xf9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xcf, 0xff, 0xff, 0xf6, 0x0, 0x0, 0x0, 0x0,
    0x0, 0xa, 0xff, 0xff, 0xff, 0xf3, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x7f, 0xff, 0xff, 0xff, 0xf0,
    0x0, 0x0, 0x0, 0x0, 0x4, 0xff, 0xff, 0xdf,
    0xff, 0xd0, 0x0, 0x0, 0x0, 0x0, 0x2f, 0xff,
    0xf8, 0xbf, 0xff, 0x90, 0x0, 0x0, 0x0, 0x1,
    0xdf, 0xff, 0xb0, 0xef, 0xff, 0x60, 0x0, 0x0,
    0x0, 0xb, 0xff, 0xfd, 0x12, 0xff, 0xff, 0x30,
    0x0, 0x0, 0x0, 0x8f, 0xff, 0xe2, 0x5, 0xff,
    0xff, 0x0, 0x0, 0x0, 0x6, 0xff, 0xff, 0x40,
    0x8, 0xff, 0xfd, 0x0, 0x0, 0x0, 0x3f, 0xff,
    0xf6, 0x0, 0xb, 0xff, 0xf9, 0x0, 0x0, 0x1,
    0xef, 0xff, 0x90, 0x0, 0xe, 0xff, 0xf6, 0x0,
    0x0, 0xc, 0xff, 0xfc, 0x0, 0x0, 0x2f, 0xff,
    0xf3, 0x0, 0x0, 0xaf, 0xff, 0xd1, 0x0, 0x0,
    0x5f, 0xff, 0xf0, 0x0, 0x7, 0xff, 0xff, 0x20,
    0x0, 0x0, 0x8f, 0xff, 0xd0, 0x0, 0xd, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9,
    0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xf5, 0x4f, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xf2, 0x6e, 0xee, 0xee, 0xee,
    0xee, 0xef, 0xff, 0xff, 0xee, 0xd0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x9, 0xff, 0xfc, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0xd, 0xff, 0xf8,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf,
    0xff, 0xf5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x4f, 0xff, 0xf1, 0x0, 0x0,

    /* U+0035 "5" */
    0x0, 0x0, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xfa, 0x0, 0x0, 0xcf, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf6, 0x0, 0x0,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xf3, 0x0, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xf0, 0x0, 0x6, 0xff, 0xff,
    0x85, 0x55, 0x55, 0x55, 0x55, 0x55, 0x40, 0x0,
    0x9, 0xff, 0xff, 0x20, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xc, 0xff, 0xfe, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff,
    0xfb, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x3f, 0xff, 0xf8, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x6f, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xd3, 0x0, 0x0, 0x9f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd,
    0x0, 0x0, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x0, 0x0, 0xbc, 0xcc, 0xcc,
    0xcc, 0xcc, 0xcc, 0xef, 0xff, 0xfd, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3f, 0xff,
    0xf9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x5f, 0xff, 0xf6, 0x0, 0x4, 0x66, 0x65,
    0x0, 0x0, 0x0, 0x0, 0x8f, 0xff, 0xf3, 0x0,
    0xd, 0xff, 0xfc, 0x0, 0x0, 0x0, 0x0, 0xcf,
    0xff, 0xf0, 0x0, 0xf, 0xff, 0xf9, 0x0, 0x0,
    0x0, 0x0, 0xff, 0xff, 0xc0, 0x0, 0x4f, 0xff,
    0xfa, 0x33, 0x33, 0x33, 0x37, 0xff, 0xff, 0x90,
    0x0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0x50, 0x0, 0x8f, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x5f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7,
    0x0, 0x0, 0x8, 0xef, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfc, 0x60, 0x0, 0x0,

    /* U+0036 "6" */
    0x0, 0x0, 0x4, 0xbe, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfe, 0x80, 0x0, 0x0, 0x4f, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0x0, 0x0,
    0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xfa, 0x0, 0x2, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xf9, 0x0, 0x5, 0xff, 0xff,
    0x61, 0x11, 0x11, 0x11, 0x6f, 0xff, 0xf7, 0x0,
    0x9, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x4f,
    0xff, 0xf3, 0x0, 0xc, 0xff, 0xfb, 0x0, 0x0,
    0x0, 0x0, 0x7f, 0xff, 0xf0, 0x0, 0xf, 0xff,
    0xf8, 0x0, 0x0, 0x0, 0x0, 0x11, 0x11, 0x10,
    0x0, 0x3f, 0xff, 0xf5, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x6f, 0xff, 0xf9, 0x88,
    0x88, 0x88, 0x88, 0x87, 0x40, 0x0, 0x0, 0x9f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8,
    0x0, 0x0, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xfd, 0x0, 0x0, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x0, 0x3,
    0xff, 0xff, 0x50, 0x0, 0x0, 0x0, 0x1f, 0xff,
    0xf9, 0x0, 0x6, 0xff, 0xff, 0x10, 0x0, 0x0,
    0x0, 0xf, 0xff, 0xf6, 0x0, 0x9, 0xff, 0xfe,
    0x0, 0x0, 0x0, 0x0, 0x3f, 0xff, 0xf2, 0x0,
    0xd, 0xff, 0xfb, 0x0, 0x0, 0x0, 0x0, 0x6f,
    0xff, 0xf0, 0x0, 0xf, 0xff, 0xf8, 0x0, 0x0,
    0x0, 0x0, 0xaf, 0xff, 0xc0, 0x0, 0x3f, 0xff,
    0xfb, 0x33, 0x33, 0x33, 0x35, 0xff, 0xff, 0x80,
    0x0, 0x6f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0x50, 0x0, 0x7f, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x5f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7,
    0x0, 0x0, 0x8, 0xef, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfc, 0x60, 0x0, 0x0,

    /* U+0037 "7" */
    0x0, 0xa, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xf5, 0x0, 0xd, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xf2, 0x0, 0x1f, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x4f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xb0,
    0x0, 0x25, 0x55, 0x55, 0x55, 0x55, 0x5d, 0xff,
    0xff, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x6f, 0xff, 0xf6, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x2, 0xff, 0xff, 0xb0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xc, 0xff, 0xff, 0x10, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x7f, 0xff, 0xf6,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0xff,
    0xff, 0xb0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xd, 0xff, 0xfe, 0x10, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x9f, 0xff, 0xf5, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x4, 0xff, 0xff, 0xa0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x1e, 0xff, 0xfe,
    0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xaf,
    0xff, 0xf5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x5, 0xff, 0xff, 0xa0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x1e, 0xff, 0xfe, 0x10, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0xbf, 0xff, 0xf5, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x6, 0xff, 0xff,
    0xa0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2f,
    0xff, 0xfe, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0xcf, 0xff, 0xf4, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x8, 0xff, 0xff, 0x90, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x3f, 0xff, 0xfe, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0,

    /* U+0038 "8" */
    0x0, 0x0, 0x1, 0x9d, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfd, 0x70, 0x0, 0x0, 0x2e, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf5, 0x0, 0x0,
    0xaf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xf8, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xf8, 0x0, 0x3, 0xff, 0xff,
    0x81, 0x11, 0x11, 0x11, 0x7f, 0xff, 0xf5, 0x0,
    0x7, 0xff, 0xff, 0x10, 0x0, 0x0, 0x0, 0x6f,
    0xff, 0xf2, 0x0, 0xa, 0xff, 0xfd, 0x0, 0x0,
    0x0, 0x0, 0x9f, 0xff, 0xf0, 0x0, 0xd, 0xff,
    0xfa, 0x0, 0x0, 0x0, 0x0, 0xcf, 0xff, 0xb0,
    0x0, 0xf, 0xff, 0xf7, 0x0, 0x0, 0x0, 0x0,
    0xff, 0xff, 0x80, 0x0, 0xf, 0xff, 0xfa, 0x33,
    0x33, 0x33, 0x38, 0xff, 0xff, 0x20, 0x0, 0xa,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf5,
    0x0, 0x0, 0x6, 0xdf, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfe, 0x80, 0x0, 0x0, 0xaf, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0x0, 0x2,
    0xff, 0xff, 0x80, 0x0, 0x0, 0x0, 0x2f, 0xff,
    0xf9, 0x0, 0x6, 0xff, 0xff, 0x20, 0x0, 0x0,
    0x0, 0x2f, 0xff, 0xf6, 0x0, 0xa, 0xff, 0xff,
    0x0, 0x0, 0x0, 0x0, 0x6f, 0xff, 0xf3, 0x0,
    0xd, 0xff, 0xfb, 0x0, 0x0, 0x0, 0x0, 0x9f,
    0xff, 0xf0, 0x0, 0xf, 0xff, 0xf8, 0x0, 0x0,
    0x0, 0x0, 0xcf, 0xff, 0xc0, 0x0, 0x4f, 0xff,
    0xfb, 0x33, 0x33, 0x33, 0x36, 0xff, 0xff, 0x90,
    0x0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0x50, 0x0, 0x8f, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x10, 0x0, 0x5f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7,
    0x0, 0x0, 0x8, 0xef, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfc, 0x60, 0x0, 0x0,

    /* U+0039 "9" */
    0x0, 0x0, 0x4, 0xbf, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfe, 0x90, 0x0, 0x0, 0x4f, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0x0, 0x0,
    0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xfa, 0x0, 0x2, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xf9, 0x0, 0x6, 0xff, 0xff,
    0x63, 0x33, 0x33, 0x33, 0x8f, 0xff, 0xf6, 0x0,
    0x9, 0xff, 0xfd, 0x0, 0x0, 0x0, 0x0, 0x5f,
    0xff, 0xf3, 0x0, 0xd, 0xff, 0xf9, 0x0, 0x0,
    0x0, 0x0, 0x8f, 0xff, 0xf0, 0x0, 0xf, 0xff,
    0xf6, 0x0, 0x0, 0x0, 0x0, 0xbf, 0xff, 0xc0,
    0x0, 0x3f, 0xff, 0xf3, 0x0, 0x0, 0x0, 0x0,
    0xef, 0xff, 0x90, 0x0, 0x6f, 0xff, 0xf2, 0x0,
    0x0, 0x0, 0x2, 0xff, 0xff, 0x60, 0x0, 0x9f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x30, 0x0, 0xaf, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x0, 0x0, 0x5f, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x0, 0x0,
    0x3, 0x78, 0x88, 0x88, 0x88, 0x88, 0x8f, 0xff,
    0xf9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x2f, 0xff, 0xf5, 0x0, 0x4, 0x66, 0x65,
    0x0, 0x0, 0x0, 0x0, 0x5f, 0xff, 0xf2, 0x0,
    0xd, 0xff, 0xfb, 0x0, 0x0, 0x0, 0x0, 0x9f,
    0xff, 0xf0, 0x0, 0x1f, 0xff, 0xf7, 0x0, 0x0,
    0x0, 0x0, 0xcf, 0xff, 0xc0, 0x0, 0x4f, 0xff,
    0xfa, 0x33, 0x33, 0x33, 0x35, 0xff, 0xff, 0x80,
    0x0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0x50, 0x0, 0x8f, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x5f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7,
    0x0, 0x0, 0x7, 0xdf, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfc, 0x60, 0x0, 0x0,

    /* U+0078 "x" */
    0x0, 0x0, 0x6f, 0xff, 0xff, 0x10, 0x0, 0x0,
    0x6, 0xff, 0xff, 0xc0, 0x0, 0x0, 0xe, 0xff,
    0xff, 0x80, 0x0, 0x0, 0x4f, 0xff, 0xfd, 0x0,
    0x0, 0x0, 0x6, 0xff, 0xff, 0xf1, 0x0, 0x3,
    0xff, 0xff, 0xe1, 0x0, 0x0, 0x0, 0x0, 0xef,
    0xff, 0xf7, 0x0, 0x2e, 0xff, 0xfe, 0x20, 0x0,
    0x0, 0x0, 0x0, 0x6f, 0xff, 0xfe, 0x1, 0xdf,
    0xff, 0xf3, 0x0, 0x0, 0x0, 0x0, 0x0, 0xe,
    0xff, 0xff, 0x7c, 0xff, 0xff, 0x40, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x6, 0xff, 0xff, 0xff, 0xff,
    0xf6, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xef, 0xff, 0xff, 0xff, 0x70, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x4, 0xff, 0xff, 0xff, 0xff,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2e,
    0xff, 0xff, 0xff, 0xff, 0x60, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x1, 0xef, 0xff, 0xfd, 0xff, 0xff,
    0xd0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xd, 0xff,
    0xff, 0x73, 0xff, 0xff, 0xf5, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xbf, 0xff, 0xf9, 0x0, 0xbf, 0xff,
    0xfc, 0x0, 0x0, 0x0, 0x0, 0x9, 0xff, 0xff,
    0xb0, 0x0, 0x4f, 0xff, 0xff, 0x40, 0x0, 0x0,
    0x0, 0x8f, 0xff, 0xfc, 0x0, 0x0, 0xd, 0xff,
    0xff, 0xb0, 0x0, 0x0, 0x6, 0xff, 0xff, 0xd1,
    0x0, 0x0, 0x6, 0xff, 0xff, 0xf3, 0x0, 0x0,
    0x4f, 0xff, 0xfe, 0x20, 0x0, 0x0, 0x0, 0xef,
    0xff, 0xfa, 0x0, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 343, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 137, .box_w = 10, .box_h = 23, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 115, .adv_w = 210, .box_w = 13, .box_h = 11, .ofs_x = 4, .ofs_y = 13},
    {.bitmap_index = 187, .adv_w = 284, .box_w = 19, .box_h = 23, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 406, .adv_w = 281, .box_w = 19, .box_h = 23, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 625, .adv_w = 412, .box_w = 23, .box_h = 23, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 890, .adv_w = 352, .box_w = 22, .box_h = 23, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1143, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 4, .ofs_y = 13},
    {.bitmap_index = 1182, .adv_w = 200, .box_w = 15, .box_h = 33, .ofs_x = 3, .ofs_y = -5},
    {.bitmap_index = 1430, .adv_w = 201, .box_w = 14, .box_h = 32, .ofs_x = 0, .ofs_y = -5},
    {.bitmap_index = 1654, .adv_w = 285, .box_w = 18, .box_h = 16, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 1798, .adv_w = 334, .box_w = 19, .box_h = 18, .ofs_x = 3, .ofs_y = 2},
    {.bitmap_index = 1969, .adv_w = 137, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2005, .adv_w = 203, .box_w = 12, .box_h = 3, .ofs_x = 3, .ofs_y = 10},
    {.bitmap_index = 2023, .adv_w = 126, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2044, .adv_w = 218, .box_w = 18, .box_h = 25, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 2269, .adv_w = 343, .box_w = 22, .box_h = 23, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 2522, .adv_w = 343, .box_w = 11, .box_h = 23, .ofs_x = 9, .ofs_y = 0},
    {.bitmap_index = 2649, .adv_w = 343, .box_w = 23, .box_h = 23, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2914, .adv_w = 343, .box_w = 22, .box_h = 23, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 3167, .adv_w = 343, .box_w = 20, .box_h = 23, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 3397, .adv_w = 343, .box_w = 22, .box_h = 23, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 3650, .adv_w = 343, .box_w = 22, .box_h = 23, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 3903, .adv_w = 343, .box_w = 20, .box_h = 23, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 4133, .adv_w = 343, .box_w = 22, .box_h = 23, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 4386, .adv_w = 343, .box_w = 22, .box_h = 23, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 4639, .adv_w = 323, .box_w = 24, .box_h = 17, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 26, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 120, .range_length = 1, .glyph_id_start = 27,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
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
    .cmap_num = 2,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_HH8 = {
#else
lv_font_t ui_font_HH8 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 33,          /*The maximum line height required by the font*/
    .base_line = 5,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 2,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_HH8*/

