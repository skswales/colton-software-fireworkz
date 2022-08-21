/* riscos/ho_marker.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RISC OS specific graphics routines for Fireworkz */

/* RCM April 1992 */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#define EXPOSE_RISCOS_BBC 1
#define EXPOSE_RISCOS_SWIS 1
#define EXPOSE_RISCOS_FONT 1

#include "ob_skel/xp_skelr.h"

#ifndef __gr_diag_h
#include "cmodules/gr_diag.h"
#endif

#ifndef __gr_rdia3_h
#include "cmodules/gr_rdia3.h"
#endif

#include "cmodules/collect.h"

#if CROSS_COMPILE
#include "drawmod.h" /* csnf...RISC_OSLib */
#else
#include "RISC_OSLib:drawmod.h" /* RISC_OSLib: */
#endif

/*
 * margin_left and margin_para have overwide BBoxes to make them
 * easier to grab, they also don't overlap in the y-direction
 *
 * margin_righ and col_right have non-centred BBoxes that overhang
 * to the left and right respectively to allow col_right to
 * be selected even when margin_righ is plotted on top of it
 * (ie when margin right offset is zero)
 *
 * also margin_head/off, margin_foot/off have non-centred BBoxes
 */

typedef struct MARKER_BITMAP_TABLE
{
    RESOURCE_BITMAP_ID id;
    GDI_SIZE bm_size; /* in PROGRAM_PIXELS */
    GDI_POINT offset; /* offset in screen space to bitmap top left from plot point (tl) in PROGRAM_PIXELS */
    GDI_BOX extra_hit; /* extra hit rect surrounding the bitmap in PROGRAM_PIXELS */
}
MARKER_BITMAP_TABLE, * P_MARKER_BITMAP_TABLE; typedef const MARKER_BITMAP_TABLE * PC_MARKER_BITMAP_TABLE;

static const MARKER_BITMAP_TABLE
marker_bitmap_table[RULER_MARKER_COUNT] =
{
    { { OBJECT_ID_SKEL, "margin_left" }, {  9,  4 }, { -4, -(8-4) }, { 2, 0, 2, 0 } },
    { { OBJECT_ID_SKEL, "margin_para" }, {  9,  4 }, { -4, 0      }, { 2, 0, 2, 0 } },
    { { OBJECT_ID_SKEL, "margin_righ" }, {  9,  4 }, { -4, -(8-4) }, { 2, 0, 0, 0 } }, /*non-centred for overlap with col_right*/
    { { OBJECT_ID_SKEL, "col_right"   }, {  3,  8 }, { -1, 0      }, { 0, 0, 2, 0 } }, /*non-centred for overlap with margin_righ*/
    { { OBJECT_ID_SKEL, "tab_l"       }, {  6,  3 }, { -1, -(8-3-0) }, { 2, 0, 2, 2 } },
    { { OBJECT_ID_SKEL, "tab_c"       }, {  8,  3 }, { -4, -(8-3-0) }, { 2, 0, 2, 2 } },
    { { OBJECT_ID_SKEL, "tab_r"       }, {  6,  3 }, { -5, -(8-3-0) }, { 2, 0, 2, 2 } },
    { { OBJECT_ID_SKEL, "tab_d"       }, {  8,  3 }, { -4, -(8-3-0) }, { 2, 0, 2, 2 } },

    { { OBJECT_ID_SKEL, "margin_head" }, { 16,  2 }, {  0,  0 }, { 2, 1, 2, 0 } },
    { { OBJECT_ID_SKEL, "margin_foot" }, { 16,  2 }, {  0, +2 }, { 2, 0, 2, 1 } }, /*borrow this for header offset */
    { { OBJECT_ID_SKEL, "margin_foot" }, { 16,  2 }, {  0,  0 }, { 2, 1, 2, 0 } },
    { { OBJECT_ID_SKEL, "margin_head" }, { 16,  2 }, {  0,  0 }, { 2, 0, 2, 1 } }, /*borrow this for footer offset */
    { { OBJECT_ID_SKEL, "row_bottom"  }, { 16,  2 }, {  0, +1 }, { 2, 1, 2, 1 } },

    { { OBJECT_ID_SKEL, "col_sep"     }, {  0, 10 }, {  0,  0 }, { 2, 0, 2, 0 } },
    { { OBJECT_ID_SKEL, "row_sep"     }, { 32,  0 }, {  0,  0 }, { 2, 1, 2, 1 } }
};

_Check_return_
extern RECT_FLAGS
host_marker_rect_flags(
    _InVal_     RULER_MARKER ruler_marker)
{
    const PC_MARKER_BITMAP_TABLE p_marker_bitmap = &marker_bitmap_table[ruler_marker];
    S32 program_pixels;
    RECT_FLAGS rect_flags;
    RECT_FLAGS_CLEAR(rect_flags);

    assert(ruler_marker > RULER_NO_MARK);
    assert(ruler_marker < RULER_MARKER_COUNT);

    program_pixels = -p_marker_bitmap->offset.x;
    assert((program_pixels <= 7) && (program_pixels >= 0));
    rect_flags.extend_left_ppixels  = (UBF) (program_pixels &  7);

    program_pixels = +p_marker_bitmap->offset.y;
    assert((program_pixels <= 7) && (program_pixels >= 0));
    rect_flags.extend_up_ppixels = (UBF) (program_pixels &  7);

    program_pixels = p_marker_bitmap->bm_size.cx + p_marker_bitmap->offset.x;
    assert((program_pixels <= 31) && (program_pixels >= 0));
    rect_flags.extend_right_ppixels = (UBF) (program_pixels & 31);

    program_pixels = p_marker_bitmap->bm_size.cy - p_marker_bitmap->offset.y;
    assert((program_pixels <= 15) && (program_pixels >= 0));
    rect_flags.extend_down_ppixels  = (UBF) (program_pixels & 15);

    return(rect_flags);
}

_Check_return_
extern BOOL
host_over_marker(
    _InRef_     PC_CLICK_CONTEXT p_click_context,
    _InVal_     RULER_MARKER ruler_marker,
    _InRef_     PC_PIXIT_POINT p_marker_pos,
    _InRef_     PC_PIXIT_POINT p_pixit_point_pointer)
{
    const PC_MARKER_BITMAP_TABLE p_marker_bitmap = &marker_bitmap_table[ruler_marker];
    PIXIT_POINT offset;
    GDI_BOX box;

    assert(ruler_marker > RULER_NO_MARK);
    assert(ruler_marker < RULER_MARKER_COUNT);

    offset.x = p_pixit_point_pointer->x - p_marker_pos->x;
    offset.y = p_marker_pos->y - p_pixit_point_pointer->y; /* coordinate flip */

    box.x0 = p_marker_bitmap->offset.x;
    box.y1 = p_marker_bitmap->offset.y;
    box.x1 = box.x0 + p_marker_bitmap->bm_size.cx;
    box.y0 = box.y1 - p_marker_bitmap->bm_size.cy;

    box.x0 -= p_marker_bitmap->extra_hit.x0;
    box.y0 -= p_marker_bitmap->extra_hit.y0;
    box.x1 += p_marker_bitmap->extra_hit.x1;
    box.y1 += p_marker_bitmap->extra_hit.x1;

    box.x0 *= p_click_context->one_program_pixel.x;
    box.y0 *= p_click_context->one_program_pixel.y;
    box.x1 *= p_click_context->one_program_pixel.x;
    box.y1 *= p_click_context->one_program_pixel.y;

    return( (offset.x >= box.x0) &&
            (offset.x <  box.x1) &&
            (offset.y >= box.y0) &&
            (offset.y <  box.y1) );
}

extern void
host_paint_marker(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InVal_     RULER_MARKER ruler_marker,
    _InRef_     PC_PIXIT_POINT p_pixit_point)
{
    const PC_MARKER_BITMAP_TABLE p_marker_bitmap = &marker_bitmap_table[ruler_marker];
    const PC_RESOURCE_BITMAP_ID p_id = &p_marker_bitmap->id;
    PIXIT_POINT pixit_point = *p_pixit_point;
    GDI_POINT gdi_point;
    WimpIconBlockWithBitset icon;

    assert(ruler_marker > RULER_NO_MARK);
    assert(ruler_marker < RULER_MARKER_COUNT);

    /* NB host_ploticon() plots window relative */
    pixit_point.x += p_redraw_context->pixit_origin.x;
    pixit_point.y += p_redraw_context->pixit_origin.y;
    window_point_from_pixit_point(&gdi_point, &pixit_point, &p_redraw_context->host_xform);

    icon.bbox.xmin = gdi_point.x + (p_marker_bitmap->offset.x * RISCOS_PER_PROGRAM_PIXEL_X);
    icon.bbox.ymax = gdi_point.y + (p_marker_bitmap->offset.y * RISCOS_PER_PROGRAM_PIXEL_Y);

    icon.bbox.xmax = icon.bbox.xmin + (p_marker_bitmap->bm_size.cx * RISCOS_PER_PROGRAM_PIXEL_X);
    icon.bbox.ymin = icon.bbox.ymax - (p_marker_bitmap->bm_size.cy * RISCOS_PER_PROGRAM_PIXEL_Y);

    icon.flags.u32 = 0;

#if 0
    icon.flags.bits.filled = 1; /* Useful for debugging */
    icon.flags.bits.bg_colour = 2;
#endif

    icon.flags.bits.sprite   = 1;
    icon.flags.bits.indirect = 1;

    icon.data.is.sprite = resource_bitmap_find(p_id).p_u8;
    icon.data.is.sprite_area = (void *) 1; /* Window Manager's sprite area - shouldn't be needed */
    icon.data.is.sprite_name_length = 0;

    host_ploticon(&icon);
}

#endif /* RISCOS */

/* end of riscos/ho_marker.c */
