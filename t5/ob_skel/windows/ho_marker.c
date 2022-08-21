/* windows/ho_marker.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Windows hosted painting code for Fireworkz */

/* David De Vorchik (diz) December 1993 (XMAS) */

#include "common/gflags.h"

#if WINDOWS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
internal functions
*/

typedef struct MARKER_BITMAP_TABLE
{
    PCTSTR id; /* for LoadImage */
    GDI_POINT offset; /* in pixels */
    RECT extra_hit; /* extra hit rect surrounding the bitmap in pixels */
}
MARKER_BITMAP_TABLE; typedef const MARKER_BITMAP_TABLE * PC_MARKER_BITMAP_TABLE;

/* Table of descriptions that define the information about
 * the resources used for painting the ruler markers.
 */

static const MARKER_BITMAP_TABLE
marker_bitmap_table[RULER_MARKER_COUNT] =
{
    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_MARL),   { -4,  16-8    }, { 2, 0, 2, 0 } },
    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_MARP),   { -4,  0       }, { 2, 0, 2, 0 } },
    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_MARR),   { -4,  16-8    }, { 2, 0, 0, 0 } }, /*non-centred for overlap with col_r*/
    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_COLR),   { -1,  0       }, { 0, 0, 2, 0 } }, /*non-centred for overlap with mar_r*/
    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_TABL),   { -1,  16-6-0  }, { 2, 0, 2, 0 } },
    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_TABC),   { -4,  16-6-0  }, { 2, 0, 2, 0 } },
    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_TABR),   { -5,  16-6-0  }, { 2, 0, 2, 0 } },
    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_TABD),   { -4,  16-6-0  }, { 2, 0, 2, 0 } },

    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_MARH),   {  0,  0 }, { 2, 2, 2, 4 } },
    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_MARF),   {  0, -3 }, { 2, 4, 2, 2 } }, /*non-centred for overlap with mar_h*/
    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_MARF),   {  0, -3 }, { 2, 4, 2, 2 } },
    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_MARH),   {  0,  0 }, { 2, 2, 2, 4 } }, /*non-centred for overlap with mar_f*/
    { MAKEINTRESOURCE(SKEL_ID_BM_RULER_ROWB),   {  0, -1 }, { 2, 2, 2, 2 } },

    { 0 /* col sep */ },
    { 0 /* row sep */ }
};

static GDI_SIZE
marker_bitmap_size[RULER_MARKER_COUNT];

_Check_return_
static BOOL
cache_bitmap_size(
    _InVal_     RULER_MARKER ruler_marker,
    _OutRef_    P_GDI_SIZE p_size)
{
    const PC_MARKER_BITMAP_TABLE p_marker_bitmap = &marker_bitmap_table[ruler_marker];
    HBITMAP hBitmap = (HBITMAP) LoadImage(resource_get_object_resources(OBJECT_ID_SKEL), p_marker_bitmap->id, IMAGE_BITMAP, 0, 0, 0);
    BITMAP bitmap;

    if(NULL == hBitmap)
    {
        p_size->cx = p_size->cy = 0;
        return(FALSE);
    }

    GetObject(hBitmap, sizeof32(bitmap), &bitmap);

    DeleteBitmap(hBitmap);

    p_size->cx = bitmap.bmWidth;
    p_size->cy = bitmap.bmHeight;

    return(TRUE);
}

/* Generate a suitable set of rectangle flags based on the specified ruler marker.
 * This code uses the bounding box of the image and the offsets defined within
 * the marker description table.
*/

_Check_return_
extern RECT_FLAGS
host_marker_rect_flags(
    _InVal_     RULER_MARKER ruler_marker)
{
    const PC_MARKER_BITMAP_TABLE p_marker_bitmap = &marker_bitmap_table[ruler_marker];
    const P_GDI_SIZE p_size = &marker_bitmap_size[ruler_marker];
    S32 program_pixels;
    RECT_FLAGS rect_flags;
    RECT_FLAGS_CLEAR(rect_flags);

    assert(ruler_marker > RULER_NO_MARK);
    assert(ruler_marker < RULER_MARKER_COUNT);

    if(0 == p_size->cx)
        if(!cache_bitmap_size(ruler_marker, p_size))
            return(rect_flags);

    program_pixels = /*idiv_ceil(*/ (-p_marker_bitmap->offset.x) /*, DU_PER_PROGRAM_PIXEL_X)*/;
    if(program_pixels > 0)
    {
        assert((program_pixels >= 0) &&           (program_pixels <= 7));
        rect_flags.extend_left_ppixels  = UBF_PACK((program_pixels & 7));
    }

    program_pixels = idiv_ceil(     (-p_marker_bitmap->offset.y), DU_PER_PROGRAM_PIXEL_Y);
    if(program_pixels > 0)
    {
        assert((program_pixels >= 0) &&           (program_pixels <= 7));
        rect_flags.extend_up_ppixels    = UBF_PACK((program_pixels & 7));
    }

    program_pixels = /*idiv_ceil(*/ (p_size->cx + p_marker_bitmap->offset.x) /*, DU_PER_PROGRAM_PIXEL_X)*/;
    assert((program_pixels >= 0) &&           (program_pixels <= 31));
    rect_flags.extend_right_ppixels = UBF_PACK((program_pixels & 31));

    program_pixels = idiv_ceil(     (p_size->cy + p_marker_bitmap->offset.y), DU_PER_PROGRAM_PIXEL_Y);
    assert((program_pixels >= 0) &&           (program_pixels <= 15));
    rect_flags.extend_down_ppixels  = UBF_PACK((program_pixels & 15));

    return(rect_flags);
}

/* Perform simple hit detection; we deduce the bounding box of marker that is
 * being used for the bounding box.  We are given the marker position and the
 * pointer position we must then perform a simple check to see if the marker
 * and the pointer are as one, if not then we return FALSE, otherwise TRUE.
 */

_Check_return_
extern BOOL
host_over_marker(
    _InRef_     PC_CLICK_CONTEXT p_click_context,
    _InVal_     RULER_MARKER ruler_marker,
    _InRef_     PC_PIXIT_POINT p_marker_pos,
    _InRef_     PC_PIXIT_POINT p_pointer_pos)
{
    const PC_MARKER_BITMAP_TABLE p_marker_bitmap = &marker_bitmap_table[ruler_marker];
    const P_GDI_SIZE p_size = &marker_bitmap_size[ruler_marker];
    PIXIT_POINT marker_pos = *p_marker_pos;
    PIXIT_POINT pointer_pos = *p_pointer_pos;
    GDI_POINT marker_gdi_point;
    GDI_POINT pointer_gdi_point;
    GDI_RECT gdi_rect;

    assert(ruler_marker > RULER_NO_MARK);
    assert(ruler_marker < RULER_MARKER_COUNT);

    if(0 == p_size->cx)
        if(!cache_bitmap_size(ruler_marker, p_size))
            return(FALSE);

    marker_pos.x += p_click_context->pixit_origin.x;
    marker_pos.y += p_click_context->pixit_origin.y;

    pointer_pos.x += p_click_context->pixit_origin.x;
    pointer_pos.y += p_click_context->pixit_origin.y;

    window_point_from_pixit_point(&marker_gdi_point, &marker_pos, &p_click_context->host_xform);
    window_point_from_pixit_point(&pointer_gdi_point, &pointer_pos, &p_click_context->host_xform);

    gdi_rect.tl.x = marker_gdi_point.x + p_marker_bitmap->offset.x; /* offset in pixels */
    gdi_rect.tl.y = marker_gdi_point.y + p_marker_bitmap->offset.y;

    gdi_rect.br.x = gdi_rect.tl.x + p_size->cx;
    gdi_rect.br.y = gdi_rect.tl.y + p_size->cy;

    gdi_rect.tl.x -= p_marker_bitmap->extra_hit.left; /* extra_hit in pixels */
    gdi_rect.tl.y -= p_marker_bitmap->extra_hit.top;
    gdi_rect.br.x += p_marker_bitmap->extra_hit.right;
    gdi_rect.br.y += p_marker_bitmap->extra_hit.bottom;

    return( (pointer_gdi_point.x >= gdi_rect.tl.x) &&
            (pointer_gdi_point.x <  gdi_rect.br.x) &&
            (pointer_gdi_point.y >= gdi_rect.tl.y) &&
            (pointer_gdi_point.y <  gdi_rect.br.y) );
}

/* Load the bitmap for the marker, and then attempt to render it at the
 * specified point in the redraw context.  The marker has to be painted
 * in two passes, the first to remove the existing marker and then the
 * new one to render the new improved one.
 */

extern void
host_paint_marker(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InVal_     RULER_MARKER ruler_marker,
    _InRef_     PC_PIXIT_POINT p_pixit_point)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    const PC_MARKER_BITMAP_TABLE p_marker_bitmap = &marker_bitmap_table[ruler_marker];
    HBITMAP hBitmap = (HBITMAP) LoadImage(resource_get_object_resources(OBJECT_ID_SKEL), p_marker_bitmap->id, IMAGE_BITMAP, 0, 0, 0);
    GDI_POINT gdi_point;
    BITMAP bm;
    POINT ptSize;
    HDC hdcTemp;

    assert(ruler_marker > RULER_NO_MARK);
    assert(ruler_marker < RULER_MARKER_COUNT);

    if(NULL == hBitmap)
        return;

    gdi_point_from_pixit_point_and_context(&gdi_point, p_pixit_point, p_redraw_context);

    gdi_point.x += p_marker_bitmap->offset.x;
    gdi_point.y += p_marker_bitmap->offset.y;

    PTR_ASSERT(hdc);
    hdcTemp = CreateCompatibleDC(hdc);

    SelectBitmap(hdcTemp, hBitmap);

    GetObject(hBitmap, sizeof32(BITMAP), &bm);

    ptSize.x = bm.bmWidth;
    ptSize.y = bm.bmHeight;
    DPtoLP(hdcTemp, &ptSize, 1);

    TransparentBlt(hdc, gdi_point.x, gdi_point.y, ptSize.x, ptSize.y, hdcTemp, 0, 0, ptSize.x, ptSize.y, RGB(0xFF,0xFF,0xFF) /*cTransparentColor*/);

    DeleteDC(hdcTemp);

    DeleteBitmap(hBitmap);
}

#endif /* WINDOWS */

/* end of windows/ho_marker.c */
