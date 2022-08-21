/* windows/ho_paint.c */

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

#include "external/Microsoft/InsideOLE2/BTTNCURP/bttncur.h"

#ifndef          __gr_diag_h
#include "cmodules/gr_diag.h"
#endif

#include "external/Dial_Solutions/drawfile.h"

#include "ob_skel/ho_gdip_image.h"

#include "cmodules/gr_rdia3.h"

#if defined(USE_CACHED_ABC_WIDTHS)
typedef const ABC * PC_ABC;
#endif

/*
internal functions
*/

_Check_return_
static HBRUSH
hbrush_from_colour(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_RGB p_rgb_brush);

#if defined(UNUSED_KEEP_ALIVE)

static void
host_frame_render(
    _InRef_     PCRECT p_rect,
    _In_        FRAMED_BOX_STYLE frame,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

static void
host_frame_trim_rect(
    _InoutRef_  PRECT p_rect,
    _InVal_     FRAMED_BOX_STYLE frame);

#endif /* UNUSED_KEEP_ALIVE */

static BOOL
host_dithering = TRUE;

_Check_return_
static COLORREF
colorref_from_rgb(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_RGB p_rgb)
{
    COLORREF colorref;

    if( p_redraw_context->flags.printer  ||
        p_redraw_context->flags.metafile ||
        p_redraw_context->flags.drawfile )
    {
        colorref = RGB(p_rgb->r, p_rgb->g, p_rgb->b);
    }
    else if(host_dithering)
    {
        colorref = PALETTERGB(p_rgb->r, p_rgb->g, p_rgb->b);
    }
    else
    {
        const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
        colorref = RGB(p_rgb->r, p_rgb->g, p_rgb->b);
        void_WrapOsBoolChecking(CLR_INVALID != (colorref =
            GetNearestColor(hdc, colorref)));
        if(CLR_INVALID == colorref)
            colorref = RGB(0, 0, 0);
    }

    return(colorref);
}

extern void
host_dithering_set(
    _InVal_     BOOL dither)
{
    host_dithering = dither;
}

static PC_REDRAW_CONTEXT cur_p_redraw_context = P_REDRAW_CONTEXT_NONE; /* nasty nasty nasty but some fonty_cache_trash has no idea */

extern void
host_paint_start(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;

    cur_p_redraw_context = p_redraw_context;

    // NB org.x,y must have been set up prior to calling this function

    if( p_redraw_context->flags.printer  ||
        p_redraw_context->flags.metafile ||
        p_redraw_context->flags.drawfile )
    {
        p_redraw_context->p_redraw_context_cache->init_h_palette = NULL;
    }
    else
    {
        host_select_default_palette(hdc, &p_redraw_context->p_redraw_context_cache->init_h_palette);

        host_rgb_stash(hdc); // it's fast enough to do every time, and copes with input focus changes wrt RealizePalette
    }

    p_redraw_context->p_redraw_context_cache->init_h_font = NULL;
    p_redraw_context->p_redraw_context_cache->h_font = NULL;
    p_redraw_context->p_redraw_context_cache->h_font_delete_after = FALSE;

    p_redraw_context->p_redraw_context_cache->init_h_brush = NULL;
    p_redraw_context->p_redraw_context_cache->h_brush = NULL;

    p_redraw_context->p_redraw_context_cache->init_h_pen = NULL;
    p_redraw_context->p_redraw_context_cache->h_pen = NULL;

    if( !p_redraw_context->flags.metafile &&
        !p_redraw_context->flags.drawfile )
    {
        void_WrapOsBoolChecking(SetMapMode(hdc, MM_TEXT));
    }

    if(!p_redraw_context->flags.drawfile)
    {
        void_WrapOsBoolChecking(SetBkMode(hdc, TRANSPARENT));

        void_WrapOsBoolChecking(GDI_ERROR !=
            SetTextAlign(hdc, TA_LEFT | TA_TOP));
    }

    if( !p_redraw_context->flags.metafile &&
        !p_redraw_context->flags.drawfile )
    {
        void_WrapOsBoolChecking(
            SetBrushOrgEx(hdc, p_redraw_context->gdi_org.x, p_redraw_context->gdi_org.y, NULL));
    }
}

/* Dispose of the cached information about the current redraw context.
 * This is used to blow away pen, brush and any other data we may have defined.
 * Note that metafiles don't have a default context to restore to! Can cause errors apparently
 */

static void
host_paint_end_delete_font(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    if(p_redraw_context->p_redraw_context_cache->h_font)
    {
        if(p_redraw_context->p_redraw_context_cache->h_font_delete_after)
        {
            p_redraw_context->p_redraw_context_cache->h_font_delete_after = FALSE;
            void_WrapOsBoolChecking(DeleteFont(p_redraw_context->p_redraw_context_cache->h_font));
        }
        p_redraw_context->p_redraw_context_cache->h_font = NULL;
    }
    assert(!p_redraw_context->p_redraw_context_cache->h_font_delete_after);
}

extern void
host_paint_end(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;

    if(p_redraw_context->p_redraw_context_cache->init_h_palette)
        host_select_old_palette(hdc, &p_redraw_context->p_redraw_context_cache->init_h_palette);

    fonty_cache_trash(p_redraw_context);

    if(p_redraw_context->p_redraw_context_cache->init_h_font)
    {
        if( !p_redraw_context->flags.metafile &&
            !p_redraw_context->flags.drawfile )
        {
            consume(HFONT, SelectFont(hdc, p_redraw_context->p_redraw_context_cache->init_h_font));
        }
        p_redraw_context->p_redraw_context_cache->init_h_font = NULL;
    }

    host_paint_end_delete_font(p_redraw_context);

    if(p_redraw_context->p_redraw_context_cache->init_h_brush)
    {
        if( !p_redraw_context->flags.metafile &&
            !p_redraw_context->flags.drawfile )
        {
            consume(HBRUSH, SelectBrush(hdc, p_redraw_context->p_redraw_context_cache->init_h_brush));
        }
        p_redraw_context->p_redraw_context_cache->init_h_brush = NULL;
    }

    if(p_redraw_context->p_redraw_context_cache->h_brush)
    {
        void_WrapOsBoolChecking(DeleteBrush(p_redraw_context->p_redraw_context_cache->h_brush));
        p_redraw_context->p_redraw_context_cache->h_brush = NULL;
    }

    if(p_redraw_context->p_redraw_context_cache->init_h_pen)
    {
        if( !p_redraw_context->flags.metafile &&
            !p_redraw_context->flags.drawfile )
        {
            consume(HPEN, SelectPen(hdc, p_redraw_context->p_redraw_context_cache->init_h_pen));
        }
        p_redraw_context->p_redraw_context_cache->init_h_pen = NULL;
    }

    if(p_redraw_context->p_redraw_context_cache->h_pen)
    {
        void_WrapOsBoolChecking(DeletePen(p_redraw_context->p_redraw_context_cache->h_pen));
        p_redraw_context->p_redraw_context_cache->h_pen = NULL;
    }

    cur_p_redraw_context = P_REDRAW_CONTEXT_NONE;
}

/* Return a brush handle for the specified RGB value */

_Check_return_
static HBRUSH
hbrush_from_colour(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_RGB p_rgb)
{
    assert(!p_rgb->transparent);

    if(p_redraw_context->p_redraw_context_cache->h_brush)
        if(!rgb_compare_not_equals(&p_redraw_context->p_redraw_context_cache->current_brush_rgb, p_rgb))
            return(p_redraw_context->p_redraw_context_cache->h_brush); // cache hit

    {
    HBRUSH delete_h_brush = p_redraw_context->p_redraw_context_cache->h_brush;
    HBRUSH return_h_brush = CreateSolidBrush(colorref_from_rgb(p_redraw_context, p_rgb));

    assert(!delete_h_brush || p_redraw_context->p_redraw_context_cache->init_h_brush);

    if(return_h_brush)
    {
        const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
        const HBRUSH old_h_brush = SelectBrush(hdc, return_h_brush);
        if(!p_redraw_context->p_redraw_context_cache->init_h_brush)
            p_redraw_context->p_redraw_context_cache->init_h_brush = old_h_brush;
        p_redraw_context->p_redraw_context_cache->h_brush = return_h_brush;
        p_redraw_context->p_redraw_context_cache->current_brush_rgb = *p_rgb;
    }
    else
    { // failed to create new brush, so keep on using current one (i.e. don't delete it if it's one of ours)
        return_h_brush = delete_h_brush;
        delete_h_brush = NULL;
    }

    if(delete_h_brush)
        void_WrapOsBoolChecking(DeleteBrush(delete_h_brush));

    return(return_h_brush);
    } /*block*/
}

_Check_return_
static HPEN
hpen_from_colour(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_RGB p_rgb,
    _In_        int width)
{
    assert(!p_rgb->transparent);

    if(p_redraw_context->p_redraw_context_cache->h_pen)
        if(width == p_redraw_context->p_redraw_context_cache->current_pen_width)
            if(!rgb_compare_not_equals(&p_redraw_context->p_redraw_context_cache->current_pen_rgb, p_rgb))
                return(p_redraw_context->p_redraw_context_cache->h_pen); // cache hit

    {
    HPEN delete_h_pen = p_redraw_context->p_redraw_context_cache->h_pen;
    HPEN return_h_pen =
        CreatePen(PS_SOLID, width,
                  colorref_from_rgb(p_redraw_context, p_rgb));

    assert(!delete_h_pen || p_redraw_context->p_redraw_context_cache->init_h_pen);

    if(return_h_pen)
    {
        const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
        const HPEN old_h_pen = SelectPen(hdc, return_h_pen);
        if(!p_redraw_context->p_redraw_context_cache->init_h_pen)
            p_redraw_context->p_redraw_context_cache->init_h_pen = old_h_pen;
        p_redraw_context->p_redraw_context_cache->h_pen = return_h_pen;
        p_redraw_context->p_redraw_context_cache->current_pen_rgb = *p_rgb;
        p_redraw_context->p_redraw_context_cache->current_pen_width = width;
    }
    else
    { // failed to create new pen, so keep on using current one (i.e. don't delete it if it's one of ours)
        return_h_pen = delete_h_pen;
        delete_h_pen = NULL;
    }

    if(delete_h_pen)
        void_WrapOsBoolChecking(DeletePen(delete_h_pen));

    return(return_h_pen);
    } /*block*/
}

/* Given a string and a rectangle centre the given string within that bounding box.
 * The co-ordinate returned align with the top left of the bounding box.
 */

static void
host_centre_text(
    _OutRef_    PPOINT p_point,
    _InRef_     PCRECT p_rect,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    SIZE size;

    status_consume(
        uchars_GetTextExtentPoint32(hdc, uchars, uchars_n, &size));

    p_point->x = p_rect->left + (((p_rect->right  - p_rect->left) - size.cx) / 2);
    p_point->y = p_rect->top  + (((p_rect->bottom - p_rect->top)  - size.cy) / 2);
}

#if defined(UNUSED_KEEP_ALIVE)

/* Plot a slab type inside the given rectangle.  The colors
 * for the slabs are read from the windows system and stored
 * within the graphics context.  They are cached
 * as required by the function.
 */

static void
host_frame_render(
    _InRef_     PCRECT p_rect,
    _In_        FRAMED_BOX_STYLE frame,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    RECT rect;
    HBRUSH hbrush;

    switch(frame & (~FRAMED_BOX_DISABLED))
    {
    default: default_unhandled();
#if CHECKING
    /* Buttons used in cell boundaries */
    case FRAMED_BOX_BUTTON_IN:
    case FRAMED_BOX_BUTTON_OUT:
#endif
        if((frame & (~FRAMED_BOX_DISABLED)) == FRAMED_BOX_BUTTON_IN)
        {
            /* paint the sunk button */
            hbrush = hbrush_from_colour(p_redraw_context, &rgb_stash[4 /*mid gray*/]);
            rect = *p_rect; rect.left += 1; rect.top += 1; FillRect(hdc, &rect, hbrush); // core

            hbrush = GetStockBrush(BLACK_BRUSH);
            rect = *p_rect; rect.right = rect.left + 1; FillRect(hdc, &rect, hbrush); // left
            rect = *p_rect; rect.left += 1; rect.bottom = rect.top + 1; FillRect(hdc, &rect, hbrush); // top
        }
        else
        {
            /* normal state - upright button */
            hbrush = GetStockBrush(LTGRAY_BRUSH);
            rect = *p_rect; rect.left += 1; rect.top += 1; rect.right -= 1; rect.bottom -= 1; FillRect(hdc, &rect, hbrush); // core

            hbrush = GetStockBrush(WHITE_BRUSH);
            rect = *p_rect; rect.right = rect.left + 1; FillRect(hdc, &rect, hbrush); // left
            rect = *p_rect; rect.left += 1; rect.bottom = rect.top + 1; FillRect(hdc, &rect, hbrush); // top

            hbrush = hbrush_from_colour(p_redraw_context, &rgb_stash[4 /*mid gray*/]);
            rect = *p_rect; rect.left = rect.right - 1; rect.top += 1; FillRect(hdc, &rect, hbrush); // right
            rect = *p_rect; rect.left += 1; rect.top = rect.bottom - 1; rect.right -= 1; FillRect(hdc, &rect, hbrush); // bottom
        }

        break;
    }
}

/* Trim the given rectangle based on the slab frame type
 * specified.  This should be used to adjust the
 * rectangle having plotted some text.
 */

static void
host_frame_trim_rect(
    _InoutRef_  PRECT p_rect,
    _InVal_     FRAMED_BOX_STYLE frame)
{
    trace_1(TRACE_APP_HOST_PAINT, TEXT("host_frame_trim_rect: frame style == %d"), frame);
    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_frame_trim_rect: rect before == tl %d,%d, br %d,%d"), p_rect->left, p_rect->top, p_rect->right, p_rect->bottom);

    switch(frame & (~FRAMED_BOX_DISABLED))
    {
    default: default_unhandled();
#if CHECKING
    case FRAMED_BOX_BUTTON_IN:
    case FRAMED_BOX_BUTTON_OUT:
#endif
        p_rect->left   += 1;
        p_rect->top    += 1;
        p_rect->right  -= 1;
        p_rect->bottom -= 1;
        break;
    }

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_frame_trim_rect: rect after; tl %d,%d, br %d,%d"), p_rect->left, p_rect->top, p_rect->right, p_rect->bottom);
}

#endif /* UNUSED_KEEP_ALIVE */

#if defined(UNUSED_KEEP_ALIVE)

/* Compare the two given RGB values returning a signed comparison
 * giving the difference of either of the R, G or B components.
 */

_Check_return_
extern int
host_compare_colours(
    _InRef_     PC_RGB p_rgb_1,
    _InRef_     PC_RGB p_rgb_2)
{
    int result;

    if(p_rgb_1->transparent && p_rgb_2->transparent)
        return(0); /* same - both transparent */

    if(p_rgb_1->transparent != p_rgb_2->transparent)
        return(1); /* differ - one transparent, one not */

    if(0 == (result = (int) p_rgb_1->r - (int) p_rgb_2->r))
    if(0 == (result = (int) p_rgb_1->g - (int) p_rgb_2->g))
             result = (int) p_rgb_1->b - (int) p_rgb_2->b;

    return(result);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* Return a machine specific font handle for the specified font.
*
* Use the scaling info contained in the REDRAW_CONTEXT
* or use 1:1 scaling when there is no drawing context (e.g. formatting)
*
******************************************************************************/

_Check_return_
extern HOST_FONT
host_font_find(
    _InRef_     PC_HOST_FONT_SPEC p_host_font_spec,
    _InRef_maybenone_ PC_REDRAW_CONTEXT p_redraw_context)
{
    HOST_FONT host_font;
    LOGFONT logfont = p_host_font_spec->logfont; /* take all data except size info from font enumeration */
    S32 numer, denom;

    if(!IS_REDRAW_CONTEXT_NONE(p_redraw_context))
    {
        /* do_x_scale=FALSE do_y_scale=FALSE use y values (unscaled, should be the same) e.g. back window */
        /* do_x_scale=TRUE  do_y_scale=FALSE use y values (unscaled) e.g. horz ruler */
        /* do_x_scale=FALSE do_y_scale=TRUE  use x values (unscaled) e.g. vert ruler */
        /* do_x_scale=TRUE  do_y_scale=TRUE  use y values (scaled) e.g. panes */
        if(p_redraw_context->host_xform.do_x_scale || !p_redraw_context->host_xform.do_y_scale)
        {
            numer = p_redraw_context->host_xform.windows.divisor_of_pixels.y;
            denom = p_redraw_context->host_xform.windows.multiplier_of_pixels.y;
        }
        else
        {
            numer = p_redraw_context->host_xform.windows.divisor_of_pixels.x;
            denom = p_redraw_context->host_xform.windows.multiplier_of_pixels.x;
        }

        logfont.lfHeight = - (int) muldiv64_round_floor(p_host_font_spec->size_y, numer, denom);
    }
    else
    {   /* NB negative lfHeight specifies character height (see MSDN) i.e. without internal or external leading */
        logfont.lfHeight = - (int) p_host_font_spec->size_y;
    }

    if(0 == p_host_font_spec->size_x)
    {   /* doesn't cater for anisotropic scaling with zero size_x, but we don't do that... */
        logfont.lfWidth = 0; /* normally select sensible aspect ratio for font */
    }
    else
    {
        /* SKS 19may94 attempts a quick m/n fudge to get documents looking ok */
        numer = 13;
        denom = 32;

        if(!IS_REDRAW_CONTEXT_NONE(p_redraw_context))
        {
            /* do_x_scale=FALSE do_y_scale=FALSE use x values (unscaled, should be the same) e.g. back window */
            /* do_x_scale=TRUE  do_y_scale=FALSE use y values (unscaled) e.g. horz ruler */
            /* do_x_scale=FALSE do_y_scale=TRUE  use x values (unscaled) e.g. vert ruler */
            /* do_x_scale=TRUE  do_y_scale=TRUE  use x values (scaled) e.g. panes */
            if(!p_redraw_context->host_xform.do_x_scale || p_redraw_context->host_xform.do_y_scale)
            {
                numer *= p_redraw_context->host_xform.windows.divisor_of_pixels.x;
                denom *= p_redraw_context->host_xform.windows.multiplier_of_pixels.x;
            }
            else
            {
                numer *= p_redraw_context->host_xform.windows.divisor_of_pixels.y;
                denom *= p_redraw_context->host_xform.windows.multiplier_of_pixels.y;
            }
        }

        logfont.lfWidth = (int) muldiv64_round_floor(p_host_font_spec->size_x, numer, denom);
    }

    void_WrapOsBoolChecking(HOST_FONT_NONE != (
    host_font = CreateFontIndirect(&logfont)));

#if TRACE_ALLOWED && 1
    if_constant(tracing(TRACE_APP_FONTS))
    {
        trace_4(TRACE_APP_FONTS,
                TEXT("host_font_find(%s): sent: %s, lfWidth=%d, lfHeight=%d"),
                IS_REDRAW_CONTEXT_NONE(p_redraw_context) ? TEXT("formatting") : TEXT("redraw"),
                logfont.lfFaceName, logfont.lfWidth, logfont.lfHeight);
        if(HOST_FONT_NONE != host_font)
        {
            const HDC hdc = IS_REDRAW_CONTEXT_NONE(p_redraw_context) ? host_get_hic_format_pixits() : p_redraw_context->windows.paintstruct.hdc;
            HFONT h_font_old = SelectFont(hdc, host_font);
            TCHARZ face_buffer[100];
            int len;
            
            len = GetTextFace(hdc, elemof32(face_buffer), face_buffer);
            assert(0 != len);

            len = GetOutlineTextMetrics(hdc, 0, NULL);
            if(0 != len)
            {
                STATUS status;
                OUTLINETEXTMETRIC * p_outlinetextmetric = al_ptr_alloc_bytes(OUTLINETEXTMETRIC *, len, &status);
                if(NULL != p_outlinetextmetric)
                {
                    GetOutlineTextMetrics(hdc, len, p_outlinetextmetric);

                    {
                    PCTSTR tstr_fullname = PtrAddBytes(PCTSTR, p_outlinetextmetric, (intptr_t) p_outlinetextmetric->otmpFullName);
                    trace_4(TRACE_APP_FONTS,
                            TEXT("host_font_find: got: %s, height=%d, a.c.w.=%d %s"),
                            tstr_fullname,
                            p_outlinetextmetric->otmTextMetrics.tmHeight,
                            p_outlinetextmetric->otmTextMetrics.tmAveCharWidth,
                            (MM_TWIPS == GetMapMode(hdc)) ? TEXT("twips") : TEXT("pixels"));
                    } /*block*/

                    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_outlinetextmetric));
                }
            }

            consume(HFONT, SelectFont(hdc, h_font_old));
        }
    }
#endif

    return(host_font);
}

extern void
host_font_dispose(
    _InoutRef_  P_HOST_FONT p_host_font,
    _InRef_maybenone_ PC_REDRAW_CONTEXT p_redraw_context)
{
    HOST_FONT host_font = *p_host_font;

    if(HOST_FONT_NONE != host_font)
    {
        *p_host_font = HOST_FONT_NONE;

        host_font_delete(host_font, p_redraw_context);
    }
}

/* cope with the font we want to delete being in a redraw_context_cache */

extern void
host_font_delete(
    _HfontRef_  HOST_FONT host_font,
    _InRef_maybenone_ PC_REDRAW_CONTEXT p_redraw_context_in)
{
    const PC_REDRAW_CONTEXT p_redraw_context = (P_REDRAW_CONTEXT_NONE != p_redraw_context_in) ? p_redraw_context_in : cur_p_redraw_context;

    if(!IS_REDRAW_CONTEXT_NONE(p_redraw_context))
    {
        if(host_font == p_redraw_context->p_redraw_context_cache->h_font)
        {
            host_paint_end_delete_font(p_redraw_context);
            return;
        }
    }

    DeleteFont(host_font);
}

extern void
host_font_select(
    _HfontRef_  HOST_FONT host_font,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InVal_     BOOL delete_after)
{
    if(host_font == p_redraw_context->p_redraw_context_cache->h_font)
        return; // cache hit

    {
    HFONT delete_h_font =
        p_redraw_context->p_redraw_context_cache->h_font_delete_after
        ? p_redraw_context->p_redraw_context_cache->h_font
        : NULL;
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    HFONT old_h_font = SelectFont(hdc, host_font);

    if(!p_redraw_context->p_redraw_context_cache->init_h_font)
        p_redraw_context->p_redraw_context_cache->init_h_font = old_h_font;

    p_redraw_context->p_redraw_context_cache->h_font = host_font;
    p_redraw_context->p_redraw_context_cache->h_font_delete_after = delete_after;
    assert(!p_redraw_context->p_redraw_context_cache->h_font_delete_after || (NULL != p_redraw_context->p_redraw_context_cache->h_font));

    if(delete_h_font)
        void_WrapOsBoolChecking(DeleteFont(delete_h_font));
    } /*block*/
}

#if TRACE_ALLOWED && 0

static void
trace_selected_font(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _In_z_      PCTSTR caller_name)
{
    if_constant(tracing(TRACE_APP_FONTS))
        if(!p_redraw_context->flags.metafile)
        {
            const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
            TCHARZ face_buffer[100];
            TEXTMETRIC textmetric;
            GetTextMetrics(hdc, &textmetric);
            GetTextFace(hdc, elemof32(face_buffer), face_buffer);
            trace_3(TRACE_APP_FONTS, TEXT("%s: %s, height: ") S32_TFMT, caller_name, face_buffer, textmetric.tmHeight);
        }
}
#endif


#if defined(UNUSED_KEEP_ALIVE)

/* Given a pixit point and the string plot some text at that position,
 * using the current colours selected.  This is done by passing the
 * call the the paint counted function.
 */

extern void
host_paint_plain_text_counted(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_RGB p_rgb)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    GDI_POINT gdi_point;
    COLORREF old_text_color;

    if(p_rgb->transparent)
        return;

    gdi_point_from_pixit_point_and_context(&gdi_point, p_pixit_point, p_redraw_context);

    void_WrapOsBoolChecking(CLR_INVALID != (old_text_color =
        SetTextColor(hdc, colorref_from_rgb(p_redraw_context, p_rgb))));

    host_font_select(GetStockFont(ANSI_VAR_FONT), p_redraw_context, FALSE /*stock object*/);

    status_assert(
        uchars_ExtTextOut(hdc,
                          gdi_point.x, gdi_point.y,
                          0, NULL,
                          uchars, uchars_n,
                          NULL));

    void_WrapOsBoolChecking(CLR_INVALID !=
        SetTextColor(hdc, old_text_color));
}

#endif /* UNUSED_KEEP_ALIVE */

/* Plot the text given within the specified rectangle.
 * Outlining the frame with a suitable border - as defined.
 *
 * Text colour is specified as an index into the system palette.
 * Text is centred within the given area and clipped within that area.
 */

extern void
host_fonty_text_paint_uchars_in_rectangle(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_BORDER_FLAGS p_border_flags,
    _InRef_     PC_RGB p_rgb_fill,
    _InRef_     PC_RGB p_rgb_line,
    _InRef_     PC_RGB p_rgb_text,
    _HfontRef_  HOST_FONT host_font)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    COLORREF old_text_color;
    COLORREF old_back_color;
    POINT point;
    RECT rect;

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&rect, p_pixit_rect, p_redraw_context)))
        return;

    if(0 != uchars_n)
    {
        host_font_select(host_font, p_redraw_context, FALSE /*caller-controlled*/);

#if TRACE_ALLOWED && 0
        if_constant(tracing(TRACE_APP_FONTS)) trace_selected_font(p_redraw_context, TEXT("host_fonty_text_paint_uchars_in_rectangle"));
#endif

        host_centre_text(&point, &rect, uchars, uchars_n, p_redraw_context);
    }
    else
    {   /* need to still render zero characters for the fill */
        point.x = rect.left;
        point.y = rect.top;
    }

    void_WrapOsBoolChecking(GDI_ERROR !=
        SetTextAlign(hdc, TA_LEFT | TA_TOP));

    void_WrapOsBoolChecking(CLR_INVALID != (old_text_color =
        SetTextColor(hdc, colorref_from_rgb(p_redraw_context, p_rgb_text))));
    void_WrapOsBoolChecking(CLR_INVALID != (old_back_color =
        SetBkColor(hdc, colorref_from_rgb(p_redraw_context, p_rgb_fill))));

    status_assert(
        uchars_ExtTextOut(hdc,
                          point.x, point.y,
                          ETO_CLIPPED | ETO_OPAQUE, &rect,
                          uchars, uchars_n,
                          NULL));

    void_WrapOsBoolChecking(CLR_INVALID !=
        SetTextColor(hdc, old_text_color));
    void_WrapOsBoolChecking(CLR_INVALID !=
        SetBkColor(hdc, old_back_color));

    if( p_border_flags->left.show   ||
        p_border_flags->right.show  ||
        p_border_flags->top.show    ||
        p_border_flags->bottom.show )
    {
        HBRUSH hbrush = hbrush_from_colour(p_redraw_context, p_rgb_line);
        RECT r;

        if( p_border_flags->left.show   &&
            p_border_flags->right.show  &&
            p_border_flags->top.show    &&
            p_border_flags->bottom.show )
        {
            void_WrapOsBoolChecking(
                FrameRect(hdc, &rect, hbrush));
            return;
        }

        if(p_border_flags->left.show)
        {
            r = rect; r.right = r.left + 1;

            void_WrapOsBoolChecking(
                FillRect(hdc, &rect, hbrush));
        }

        if(p_border_flags->right.show)
        {
            r = rect; r.left = r.right - 1;

            void_WrapOsBoolChecking(
                FillRect(hdc, &rect, hbrush));
        }

        if(p_border_flags->top.show)
        {
            r = rect; r.bottom = r.top + 1;

            void_WrapOsBoolChecking(
                FillRect(hdc, &rect, hbrush));
        }

        if(p_border_flags->bottom.show)
        {
            r = rect; r.top = r.bottom - 1;

            void_WrapOsBoolChecking(
                FillRect(hdc, &rect, hbrush));
        }
    }
}

#if defined(UNUSED_KEEP_ALIVE)

/* Plot some text in a framed box.
 * Framed boxes are basically bordered icons a la RISC OS.
 * They consist of a frame (slab in/out etc) and these are plotted on the inside of the given rectangle.
 *
 * The text in then plotted centred in the remaining area of the box in the given colour.
 * Frame painting and bounding box trimming is performed by local functions.
 */

extern void
host_fonty_text_paint_uchars_in_framed_box(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _In_        FRAMED_BOX_STYLE border_style,
    _InVal_     S32 fill_wimpcolour,
    _InVal_     S32 text_wimpcolour,
    _HfontRef_  HOST_FONT host_font)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    RECT rect;

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&rect, p_pixit_rect, p_redraw_context)))
        return;

    host_frame_render(&rect, border_style, p_redraw_context);

    host_frame_trim_rect(&rect, border_style);

    if(0 == uchars_n)
        return;

    if((rect.left < rect.right) && (rect.top < rect.bottom))
    {
        COLORREF old_text_color;
        COLORREF old_back_color;
        POINT point;

        host_font_select(host_font, p_redraw_context, FALSE /*caller-controlled*/);

#if TRACE_ALLOWED && 0
        if_constant(tracing(TRACE_APP_FONTS)) trace_selected_font(p_redraw_context, TEXT("host_fonty_text_paint_uchars_in_framed_box"));
#endif

        host_centre_text(&point, &rect, uchars, uchars_n, p_redraw_context);

        void_WrapOsBoolChecking(GDI_ERROR !=
            SetTextAlign(hdc, TA_LEFT | TA_TOP));

        void_WrapOsBoolChecking(CLR_INVALID != (old_text_color =
            SetTextColor(hdc, colorref_from_rgb(p_redraw_context, &rgb_stash[text_wimpcolour]))));
        void_WrapOsBoolChecking(CLR_INVALID != (old_back_color =
            SetBkColor(hdc, colorref_from_rgb(p_redraw_context, &rgb_stash[fill_wimpcolour]))));

        status_assert(
            uchars_ExtTextOut(hdc,
                              point.x, point.y,
                              ETO_CLIPPED, &rect,
                              uchars, uchars_n,
                              NULL));

        void_WrapOsBoolChecking(CLR_INVALID !=
            SetTextColor(hdc, old_text_color));
        void_WrapOsBoolChecking(CLR_INVALID !=
            SetBkColor(hdc, old_back_color));
    }
}

#endif /* UNUSED_KEEP_ALIVE */

extern void
host_fonty_text_paint_uchars_simple(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_USTR uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background,
    _HfontRef_  HOST_FONT host_font,
    _InVal_     int text_align_lcr)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    COLORREF old_text_color;
    COLORREF old_back_color;
    GDI_POINT gdi_point;

    if(0 == uchars_n)
        return;

    gdi_point_from_pixit_point_and_context(&gdi_point, p_pixit_point, p_redraw_context);

    host_font_select(host_font, p_redraw_context, FALSE /*caller-controlled*/);

#if TRACE_ALLOWED && 0
    if_constant(tracing(TRACE_APP_FONTS)) trace_selected_font(p_redraw_context, TEXT("host_fonty_text_paint_uchars_simple"));
#endif

    void_WrapOsBoolChecking(GDI_ERROR !=
        SetTextAlign(hdc, text_align_lcr | TA_BASELINE));

    void_WrapOsBoolChecking(CLR_INVALID != (old_text_color =
        SetTextColor(hdc, colorref_from_rgb(p_redraw_context, p_rgb_foreground))));
    void_WrapOsBoolChecking(CLR_INVALID != (old_back_color =
        SetBkColor(hdc, colorref_from_rgb(p_redraw_context, p_rgb_background))));

    status_assert(
        uchars_ExtTextOut(hdc,
                          gdi_point.x, gdi_point.y,
                          0, NULL,
                          uchars, uchars_n,
                          NULL));

    void_WrapOsBoolChecking(CLR_INVALID !=
        SetTextColor(hdc, old_text_color));
    void_WrapOsBoolChecking(CLR_INVALID !=
        SetBkColor(hdc, old_back_color));
}

/* Set the clipping rectangle to the pixit one specified, also clipping it to the parent rectangle currently defined */

extern BOOL
host_set_clip_rectangle(
    _InoutRef_  P_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InVal_     RECT_FLAGS rect_flags)
{
    PIXIT_RECT pixit_rect = *p_pixit_rect;
    RECT clip_rect;

    assert_EQ(sizeof32(rect_flags), sizeof32(U32));

    if(0 != * (PC_U32) &rect_flags) /* SKS 18apr95 more optimisation */
    {
      /*pixit_rect.tl.x -= rect_flags.extend_left_currently_unused  * p_redraw_context->border_width.x;*/
        if(rect_flags.reduce_left_by_2)
            pixit_rect.tl.x += p_redraw_context->border_width_2.x;
        if(rect_flags.reduce_left_by_1) /* only used by PMF */
            pixit_rect.tl.x += p_redraw_context->border_width.x;

      /*pixit_rect.tl.y -= rect_flags.extend_up_currently_unused    * p_redraw_context->border_width.y;*/
        if(rect_flags.reduce_up_by_2)
            pixit_rect.tl.y += p_redraw_context->border_width_2.y;
        if(rect_flags.reduce_up_by_1)
            pixit_rect.tl.y += p_redraw_context->border_width.y;

        if(rect_flags.extend_right_by_1)
            pixit_rect.br.x += p_redraw_context->border_width.x;
        if(rect_flags.reduce_right_by_1)
            pixit_rect.br.x -= p_redraw_context->border_width.x;

        if(rect_flags.extend_down_by_1)
            pixit_rect.br.y += p_redraw_context->border_width.y;
        if(rect_flags.reduce_down_by_1)
            pixit_rect.br.y -= p_redraw_context->border_width.y;
    }

    trace_1(TRACE_APP_HOST_PAINT, TEXT("host_set_clip_rectangle: ") PIXIT_RECT_TFMT, PIXIT_RECT_ARGS(pixit_rect));

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&clip_rect, &pixit_rect, p_redraw_context)))
    {
        trace_0(TRACE_APP_HOST_PAINT, TEXT("host_set_clip_rectangle: *** RECTANGLE IS NULL ***"));
        return(FALSE);
    }

    clip_rect.left   = MAX(p_redraw_context->windows.host_machine_clip_rect.left,   clip_rect.left);
    clip_rect.top    = MAX(p_redraw_context->windows.host_machine_clip_rect.top,    clip_rect.top);
    clip_rect.right  = MIN(p_redraw_context->windows.host_machine_clip_rect.right,  clip_rect.right);
    clip_rect.bottom = MIN(p_redraw_context->windows.host_machine_clip_rect.bottom, clip_rect.bottom);

    if((clip_rect.left >= clip_rect.right) || (clip_rect.top >= clip_rect.bottom))
    {
        trace_0(TRACE_APP_HOST_PAINT, TEXT("host_set_clip_rectangle: *** RECTANGLE IS NULL ***"));
        return(FALSE);
    }

    p_redraw_context->windows.host_machine_clip_rect = clip_rect;

    trace_4(TRACE_APP_HOST_PAINT,
        TEXT("host_set_clip_rectangle: called tl=") S32_TFMT TEXT(",") S32_TFMT TEXT(" br=") S32_TFMT TEXT(",") S32_TFMT,
        clip_rect.left, clip_rect.top, clip_rect.right, clip_rect.bottom);

    if(p_redraw_context->flags.metafile) // clipping regions do not scale in metafiles. incredible !!!
        return(TRUE);

    if(p_redraw_context->flags.drawfile) // no clipping regions available in drawfiles.
        return(TRUE);

    {
    HRGN h_clip_region = CreateRectRgn(clip_rect.left, clip_rect.top, clip_rect.right, clip_rect.bottom);

    if(NULL != h_clip_region)
    {
        const HDC hdc = p_redraw_context->windows.paintstruct.hdc;

        void_WrapOsBoolChecking(ERROR !=
            SelectClipRgn(hdc, h_clip_region));

        DeleteObject(h_clip_region);
    }
    } /*block*/

    return(TRUE);
}

/* Given two pixit rectangles clip them together and then set the clip region to that of the resulting box */

extern BOOL
host_set_clip_rectangle2(
    _InoutRef_  P_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect_paper,
    _InRef_     PC_PIXIT_RECT p_rect_object,
    _InVal_     RECT_FLAGS rect_flags)
{
    PIXIT_RECT pixit_rect = *p_rect_object;
    RECT clip_rect;

    assert_EQ(sizeof32(rect_flags), sizeof32(U32));

    if(0 != * (PC_U32) &rect_flags) /* SKS 18apr95 more optimisation */
    {
      /*pixit_rect.tl.x -= rect_flags.extend_left_currently_unused  * p_redraw_context->border_width.x;*/
        if(rect_flags.reduce_left_by_2)
            pixit_rect.tl.x += p_redraw_context->border_width_2.x;
        if(rect_flags.reduce_left_by_1) /* only used by PMF */
            pixit_rect.tl.x += p_redraw_context->border_width.x;

      /*pixit_rect.tl.y -= rect_flags.extend_up_currently_unused    * p_redraw_context->border_width.y;*/
        if(rect_flags.reduce_up_by_2)
            pixit_rect.tl.y += p_redraw_context->border_width_2.y;
        if(rect_flags.reduce_up_by_1)
            pixit_rect.tl.y += p_redraw_context->border_width.y;

        if(rect_flags.extend_right_by_1)
            pixit_rect.br.x += p_redraw_context->border_width.x;
        if(rect_flags.reduce_right_by_1)
            pixit_rect.br.x -= p_redraw_context->border_width.x;

        if(rect_flags.extend_down_by_1)
            pixit_rect.br.y += p_redraw_context->border_width.y;
        if(rect_flags.reduce_down_by_1)
            pixit_rect.br.y -= p_redraw_context->border_width.y;
    }

    pixit_rect.tl.x = MAX(p_pixit_rect_paper->tl.x, pixit_rect.tl.x);
    pixit_rect.tl.y = MAX(p_pixit_rect_paper->tl.y, pixit_rect.tl.y);
    pixit_rect.br.x = MIN(p_pixit_rect_paper->br.x, pixit_rect.br.x);
    pixit_rect.br.y = MIN(p_pixit_rect_paper->br.y, pixit_rect.br.y);

    /* SKS 18apr95 copy of the above code to avoid slow multiply by zero cases! */
    trace_1(TRACE_APP_HOST_PAINT, TEXT("host_set_clip_rectangle2: ") PIXIT_RECT_TFMT, PIXIT_RECT_ARGS(pixit_rect));

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&clip_rect, &pixit_rect, p_redraw_context)))
    {
        trace_0(TRACE_APP_HOST_PAINT, TEXT("host_set_clip_rectangle2: *** RECTANGLE IS NULL ***"));
        return(FALSE);
    }

    clip_rect.left   = MAX(p_redraw_context->windows.host_machine_clip_rect.left,   clip_rect.left);
    clip_rect.top    = MAX(p_redraw_context->windows.host_machine_clip_rect.top,    clip_rect.top);
    clip_rect.right  = MIN(p_redraw_context->windows.host_machine_clip_rect.right,  clip_rect.right);
    clip_rect.bottom = MIN(p_redraw_context->windows.host_machine_clip_rect.bottom, clip_rect.bottom);

    if((clip_rect.left >= clip_rect.right) || (clip_rect.top >= clip_rect.bottom))
    {
        trace_0(TRACE_APP_HOST_PAINT, TEXT("host_set_clip_rectangle2: *** RECTANGLE IS NULL ***"));
        return(FALSE);
    }

    p_redraw_context->windows.host_machine_clip_rect = clip_rect;

    trace_4(TRACE_APP_HOST_PAINT,
        TEXT("host_set_clip_rectangle2: called tl=") S32_TFMT TEXT(",") S32_TFMT TEXT(" br=") S32_TFMT TEXT(",") S32_TFMT,
        clip_rect.left, clip_rect.top, clip_rect.right, clip_rect.bottom);

    if(p_redraw_context->flags.metafile) // clipping regions do not scale in metafiles. incredible
        return(TRUE);

    if(p_redraw_context->flags.drawfile) // no clipping regions available in drawfiles.
        return(TRUE);

    {
    HRGN h_clip_region = CreateRectRgn(clip_rect.left, clip_rect.top, clip_rect.right, clip_rect.bottom);

    if(NULL != h_clip_region)
    {
        const HDC hdc = p_redraw_context->windows.paintstruct.hdc;

        void_WrapOsBoolChecking(ERROR !=
            SelectClipRgn(hdc, h_clip_region));

        DeleteObject(h_clip_region);
    }
    } /*block*/

    return(TRUE);
}

/* Extract the clip rectangle from the redraw context and then set it.
 * This must first be converted to a region that is then passed on and processed.
 * NB regions don't scale in metafiles
 */

extern void
host_restore_clip_rectangle(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    HRGN h_clip_region;

    if( p_redraw_context->flags.metafile ||
        p_redraw_context->flags.drawfile )
        return;

    trace_4(TRACE_APP_HOST_PAINT,
        TEXT("host_restore_clip_rectangle: called tl=") S32_TFMT TEXT(",") S32_TFMT TEXT(" br=") S32_TFMT TEXT(",") S32_TFMT,
        p_redraw_context->windows.host_machine_clip_rect.left,
        p_redraw_context->windows.host_machine_clip_rect.top,
        p_redraw_context->windows.host_machine_clip_rect.right,
        p_redraw_context->windows.host_machine_clip_rect.bottom);

    h_clip_region =
        CreateRectRgn(
            p_redraw_context->windows.host_machine_clip_rect.left,
            p_redraw_context->windows.host_machine_clip_rect.top,
            p_redraw_context->windows.host_machine_clip_rect.right,
            p_redraw_context->windows.host_machine_clip_rect.bottom);

    if(NULL != h_clip_region)
    {
        const HDC hdc = p_redraw_context->windows.paintstruct.hdc;

        void_WrapOsBoolChecking(ERROR !=
            SelectClipRgn(hdc, h_clip_region));

        DeleteObject(h_clip_region);
    }
}

/* Fill or outline the given rectangle points on the page.
 * The coordinates are transformed to something sensible
 * for the GDI and then the rendering is performed.
 */

extern void
host_paint_rectangle_filled(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_RGB p_rgb)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    RECT rect;

    if(p_rgb->transparent)
        return;

    if(p_redraw_context->flags.drawfile)
    {
        drawfile_paint_rectangle_filled(p_redraw_context, p_pixit_rect, p_rgb);
        return;
    }

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&rect, p_pixit_rect, p_redraw_context)))
        return;

    void_WrapOsBoolChecking(
        FillRect(hdc, &rect, hbrush_from_colour(p_redraw_context, p_rgb)));
}

extern void
host_paint_rectangle_outline(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_RGB p_rgb)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    RECT rect;

    if(p_rgb->transparent)
        return;

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&rect, p_pixit_rect, p_redraw_context)))
        return;

    void_WrapOsBoolChecking(
        FrameRect(hdc, &rect, hbrush_from_colour(p_redraw_context, p_rgb)));
}

extern void
host_paint_rectangle_crossed(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_RGB p_rgb)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    RECT rect;

    if(p_rgb->transparent)
        return;

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&rect, p_pixit_rect, p_redraw_context)))
        return;

    void_WrapOsBoolChecking(
        FrameRect(hdc, &rect, hbrush_from_colour(p_redraw_context, p_rgb)));

    if((rect.left != rect.right) && (rect.top != rect.bottom))
    {
        (void) hpen_from_colour(p_redraw_context, p_rgb, 0);

        void_WrapOsBoolChecking(
            MoveToEx(hdc, rect.left, rect.top, NULL));
        void_WrapOsBoolChecking(
            LineTo(hdc, rect.right, rect.bottom));

        void_WrapOsBoolChecking(
            MoveToEx(hdc, rect.left, rect.bottom, NULL));
        void_WrapOsBoolChecking(
            LineTo(hdc, rect.right, rect.top));
    }
}

/* Inverted rectangle plotting, these use the same rules as the rectangle
 * plotting functions, yet.  They modify the colours based on the
 * XOR of the fg and bg colour to ensure that the fg is the colour
 * always seen on screen.
 *
 * Because of the nature of these two calls, the graphics state is
 * maintained internally rather than using the cached values
 * within the redraw context.
 */

extern void
host_invert_rectangle_filled(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_RGB p_fore,
    _InRef_     PC_RGB p_back)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    RECT rect;

    UNREFERENCED_PARAMETER_InRef_(p_fore);
    UNREFERENCED_PARAMETER_InRef_(p_back);

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&rect, p_pixit_rect, p_redraw_context)))
        return;

    void_WrapOsBoolChecking(InvertRect(hdc, &rect));
}

extern void
host_invert_rectangle_outline(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_RGB p_fore,
    _InRef_     PC_RGB p_back)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    RECT rect;
    RECT r;

    UNREFERENCED_PARAMETER_InRef_(p_fore);
    UNREFERENCED_PARAMETER_InRef_(p_back);

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&rect, p_pixit_rect, p_redraw_context)))
        return;

    r = rect;

    if(r.left + 1 >= r.right)
    {   /* thin vertical line */
        r.right = r.left + 1;
        void_WrapOsBoolChecking(InvertRect(hdc, &r));
    }
    else if(r.top + 1 >= r.bottom)
    {   /* thin horizontal line */
        r.bottom = r.top + 1;
        void_WrapOsBoolChecking(InvertRect(hdc, &r));
    }
    else
    {   /* rectangle outline */
        r.right -= 1;
        r.bottom = r.top + 1;
        void_WrapOsBoolChecking(InvertRect(hdc, &r));
        r = rect;
        r.left = r.right - 1;
        r.bottom -= 1;
        void_WrapOsBoolChecking(InvertRect(hdc, &r));
        r = rect;
        r.left += 1;
        r.top = r.bottom - 1;
        void_WrapOsBoolChecking(InvertRect(hdc, &r));
        r = rect;
        r.right = r.left + 1;
        r.top += 1;
        void_WrapOsBoolChecking(InvertRect(hdc, &r));
    }
}

/* used to render the ruler */

extern void
host_paint_line_solid(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    PIXIT_RECT pixit_rect;
    RECT rect;

    if(p_rgb->transparent)
        return;

    assert(!p_redraw_context->flags.printer);
    assert(!p_redraw_context->flags.drawfile);

    pixit_rect.tl = p_pixit_line->tl;
    pixit_rect.br = p_pixit_line->br;

    if(p_pixit_line->horizontal)
        pixit_rect.br.y = pixit_rect.tl.y + p_redraw_context->one_program_pixel.y;
    else
        pixit_rect.br.x = pixit_rect.tl.x + p_redraw_context->one_program_pixel.x;

    /* clip close to GDI limits (OK because lines either horizontal or vertical */
    if(!status_done(RECT_limited_from_pixit_rect_and_context(&rect, &pixit_rect, p_redraw_context)))
        return;

    /* ruler lines always drawn very thin on screen, never scaled */
    if(p_pixit_line->horizontal)
        rect.bottom = rect.top + 1;
    else
        rect.right = rect.left + 1;

    void_WrapOsBoolChecking(
        FillRect(hdc, &rect, hbrush_from_colour(p_redraw_context, p_rgb)));
}

/******************************************************************************
*
* host_paint_border_line
*
******************************************************************************/

_Check_return_
static GDI_COORD
riscos_unit_from_pixit_x(
    _InVal_     PIXIT pixit_x,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    return((pixit_x * p_host_xform->scale.t.x) / (p_host_xform->scale.b.x * PIXITS_PER_RISCOS));
}

_Check_return_
static GDI_COORD
riscos_unit_from_pixit_y(
    _InVal_     PIXIT pixit_y,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    return((pixit_y * p_host_xform->scale.t.y) / (p_host_xform->scale.b.y * PIXITS_PER_RISCOS));
}

extern void
host_paint_border_line(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb,
    _In_        BORDER_LINE_FLAGS flags)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    PIXIT_LINE pixit_line = *p_pixit_line;
    PIXIT_POINT line_width, line_width_eff, line_width_select;
    PIXIT_RECT pixit_rect;
    GDI_RECT gdi_rect;

    if(p_rgb->transparent)
        return;

    switch(flags.border_style)
    {
    case SF_BORDER_NONE:
        return;

    case SF_BORDER_THIN:
        line_width     = p_redraw_context->thin_width;
        line_width_eff = p_redraw_context->thin_width_eff;
        break;

    default: default_unhandled();
#if CHECKING
    case SF_BORDER_STANDARD:
    case SF_BORDER_BROKEN:
#endif
        line_width     = p_redraw_context->line_width;
        line_width_eff = p_redraw_context->line_width_eff;
        break;

    case SF_BORDER_THICK:
        line_width     = p_redraw_context->border_width;
        line_width_eff = p_redraw_context->border_width;
        break;
    }

    // different to RISC OS here as we can see finer pixels so can always do rect_fill

#if TRACE_ALLOWED
    tracef(TRACE_APP_HOST_PAINT, TEXT("host_paint_border_line: line ") PIXIT_RECT_TFMT TEXT(" %s lw ") PIXIT_TFMT TEXT(",") PIXIT_TFMT TEXT(" lw ") PIXIT_TFMT TEXT(",") PIXIT_TFMT,
           PIXIT_RECT_ARGS(pixit_line),
           pixit_line.horizontal ? TEXT("H") : TEXT("V"),
           line_width.x, line_width.y, line_width_eff.x, line_width_eff.y);
#endif

    if( p_redraw_context->flags.printer  ||
        p_redraw_context->flags.metafile ||
        p_redraw_context->flags.drawfile )
        line_width_select = line_width;
    else
        line_width_select = line_width_eff;

    if(flags.add_lw_to_l)
        pixit_line.tl.x += line_width_select.x;
    if(flags.add_gw_to_l)
        pixit_line.tl.x += p_redraw_context->border_width.x;
    if(flags.sub_gw_from_l)
        pixit_line.tl.x -= p_redraw_context->border_width.x;

    if(flags.add_lw_to_t)
        pixit_line.tl.y += line_width_select.y;
    if(flags.add_gw_to_t)
        pixit_line.tl.y += p_redraw_context->border_width.y;
    if(flags.sub_gw_from_t)
        pixit_line.tl.y -= p_redraw_context->border_width.y;

    if(flags.add_lw_to_r)
        pixit_line.br.x += line_width_select.x;
    if(flags.add_gw_to_r)
        pixit_line.br.x += p_redraw_context->border_width.x;
    if(flags.sub_gw_from_r)
        pixit_line.br.x -= p_redraw_context->border_width.x;

    if(flags.add_lw_to_b)
        pixit_line.br.y += line_width_select.y;
    if(flags.add_gw_to_b)
        pixit_line.br.y += p_redraw_context->border_width.y;
    if(flags.sub_gw_from_b)
        pixit_line.br.y -= p_redraw_context->border_width.y;

    pixit_rect.tl.x = pixit_line.tl.x;
    pixit_rect.tl.y = pixit_line.tl.y;

    if(pixit_line.horizontal)
    {
        pixit_rect.br.x = pixit_line.br.x;
        pixit_rect.br.y = pixit_rect.tl.y + line_width_select.y;
    }
    else
    {
        pixit_rect.br.x = pixit_rect.tl.x + line_width_select.x;
        pixit_rect.br.y = pixit_line.br.y;
    }

    trace_1(TRACE_APP_HOST_PAINT, TEXT("host_paint_border_line: rect ") PIXIT_RECT_TFMT, PIXIT_RECT_ARGS(pixit_rect));

    /* clip close to GDI limits (only ok because lines either horizontal or vertical) */
    if(!status_done(gdi_rect_limited_from_pixit_rect_and_context(&gdi_rect, &pixit_rect, p_redraw_context)))
        return;

    if(p_redraw_context->flags.metafile) // NB NOT drawfile !!!
    {
        /* sad bit of code here - punters want to see the lines in their metafiles in tiny views */
        int full_width = (pixit_line.horizontal ? (gdi_rect.br.y - gdi_rect.tl.y) : (gdi_rect.br.x - gdi_rect.tl.x));
        int half_width = full_width / 2;
        const int pen_style = PS_INSIDEFRAME;
        POINT wanky_point_start;
        POINT wanky_point_end;
        /* put the line down the middle of the rectangle that the line lies in */
        if(pixit_line.horizontal)
        {
            wanky_point_start.x = gdi_rect.tl.x + half_width; /* account for wanky rounded GDI end caps */
            wanky_point_start.y = gdi_rect.tl.y + half_width;
            wanky_point_end.x   = gdi_rect.br.x - half_width / 2;
            wanky_point_end.y   = wanky_point_start.y;
        }
        else
        {
            wanky_point_start.x = gdi_rect.tl.x + half_width;
            wanky_point_start.y = gdi_rect.tl.y + half_width;
            wanky_point_end.x   = wanky_point_start.x;
            wanky_point_end.y   = gdi_rect.br.y - half_width;
        }

        {
        HPEN hpen =
            CreatePen(pen_style,
                      full_width,
                      colorref_from_rgb(p_redraw_context, p_rgb));

        if(NULL != hpen)
        {
            HPEN old_hpen = SelectPen(hdc, hpen);

            void_WrapOsBoolChecking(
                MoveToEx(hdc, wanky_point_start.x, wanky_point_start.y, NULL));

            void_WrapOsBoolChecking(
                LineTo(hdc, wanky_point_end.x, wanky_point_end.y));

            void_WrapOsBoolChecking(
                DeletePen(SelectPen(hdc, old_hpen)));
        }
        } /*block*/

        if(flags.border_style == SF_BORDER_THIN)
            return;

        if(flags.border_style == SF_BORDER_BROKEN)
            return;

        /*else ... always finish off with a rectangle in any case */
    }

    if(p_redraw_context->flags.drawfile)
    {
        struct { DRAW_DASH_HEADER header; S32 pattern[2]; } dash_pattern;
        PC_DRAW_DASH_HEADER line_dash_pattern = NULL;
        S32 thickness = 0; /* thin, unless otherwise specified */

#define BROKEN_LINE_MARK  14513 /* 180*256*8/25.4 i.e. 1mm */
#define BROKEN_LINE_SPACE 14513

        if(flags.border_style == SF_BORDER_BROKEN)
        {
            dash_pattern.header.dashstart = pixit_line.horizontal
                                          ? riscos_unit_from_pixit_x(pixit_rect.tl.x, &p_redraw_context->host_xform)
                                          : riscos_unit_from_pixit_y(pixit_rect.tl.y, &p_redraw_context->host_xform);

            dash_pattern.pattern[0] = riscos_unit_from_pixit_x(BROKEN_LINE_MARK,  &p_redraw_context->host_xform);
            dash_pattern.pattern[1] = riscos_unit_from_pixit_x(BROKEN_LINE_SPACE, &p_redraw_context->host_xform);

            dash_pattern.header.dashstart = ((U32) abs((int) dash_pattern.header.dashstart)) << 8;
            dash_pattern.header.dashcount = 2;
#if 0 /*1 to test */
            assert_EQ(2U, elemof32(dash_pattern.pattern));
#endif

            line_dash_pattern = &dash_pattern.header;
        }

#undef  BROKEN_LINE_MARK
#undef  BROKEN_LINE_SPACE

        if(flags.border_style != SF_BORDER_THIN)
        {
            PIXIT pixit = pixit_line.horizontal ? (pixit_rect.br.y - pixit_rect.tl.y) : (pixit_rect.br.x - pixit_rect.tl.x);
            if(pixit > 0)
                thickness = pixit * GR_RISCDRAW_PER_PIXIT;
        }

        drawfile_paint_line(p_redraw_context, &pixit_rect, thickness, line_dash_pattern, p_rgb);

        return;
    }

    if(flags.border_style != SF_BORDER_BROKEN)
    {
        RECT rect;

        if((gdi_rect.tl.x >= gdi_rect.br.x) || (gdi_rect.tl.y >= gdi_rect.br.y))
        {
            /* line too small to plot using FillRect */
            trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_border_line: box ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT TEXT(" FAILED"), gdi_rect.tl.x, gdi_rect.tl.y, gdi_rect.br.x, gdi_rect.br.y);
            return;
        }

        trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_border_line: box ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT, gdi_rect.tl.x, gdi_rect.tl.y, gdi_rect.br.x, gdi_rect.br.y);

        rect.left   = gdi_rect.tl.x;
        rect.top    = gdi_rect.tl.y;
        rect.right  = gdi_rect.br.x;
        rect.bottom = gdi_rect.br.y;

        if(flags.border_style == SF_BORDER_THIN)
        { /* thin is always drawn very thin on screen, whereas standard and thick may be scaled */
            if(pixit_line.horizontal)
                rect.top = rect.bottom - 1;
            else
                rect.right = rect.left + 1;
        }

        void_WrapOsBoolChecking(
            FillRect(hdc, &rect, hbrush_from_colour(p_redraw_context, p_rgb)));

        return;
    }

    // due to Draw_Stroke using PolyPolygon and all the bugs therein, do the job ourselves

    if((gdi_rect.tl.x >= gdi_rect.br.x) || (gdi_rect.tl.y >= gdi_rect.br.y))
    {
        /* line too small to plot using FillRect */
        trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_border_line: box ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT TEXT(" FAILED"), gdi_rect.tl.x, gdi_rect.tl.y, gdi_rect.br.x, gdi_rect.br.y);
        return;
    }

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_border_line: box ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT, gdi_rect.tl.x, gdi_rect.tl.y, gdi_rect.br.x, gdi_rect.br.y);

#define BROKEN_LINE_MARK  57 /* near 1mm */
#define BROKEN_LINE_SPACE 57

    {
    HBRUSH hbrush = hbrush_from_colour(p_redraw_context, p_rgb);
    int mark  = riscos_unit_from_pixit_x(BROKEN_LINE_MARK,  &p_redraw_context->host_xform);
    int space = riscos_unit_from_pixit_x(BROKEN_LINE_SPACE, &p_redraw_context->host_xform);
    int m_and_s = mark + space;
    GDI_POINT start, end;
    RECT rect;
    start.x = gdi_rect.tl.x; /*inc*/
    start.y = gdi_rect.tl.y; /*inc*/
    end.x   = gdi_rect.br.x; /*exc*/
    end.y   = gdi_rect.br.y; /*exc*/
    rect.left   = start.x;
    rect.top    = start.y;
    rect.right  = end.x;
    rect.bottom = end.y;
    if(pixit_line.horizontal)
    { // draw line segments at the same vertical position
        int totlen = rect.right - rect.left;
        int x = rect.left - rect.left % m_and_s;
        while(totlen > 0)
        {
            rect.left = x;
            rect.right = rect.left + mark;
            if(rect.left < start.x) // don't draw off the start of the line segment
                rect.left = start.x;
            if(rect.right > end.x) // don't draw off the end of the line segment
                rect.right = end.x;
            if(rect.right > rect.left) // note the case where tl.x is in a space in the line segment
            {
                void_WrapOsBoolChecking(FillRect(hdc, &rect, hbrush));
            }
            totlen -= m_and_s;
            x += m_and_s;
        }
    }
    else
    { // draw line segments at the same vertical position
        int totlen = rect.bottom - rect.top;
        int y = rect.top - rect.top % m_and_s;
        while(totlen > 0)
        {
            rect.top = y;
            rect.bottom = rect.top + mark;
            if(rect.top < start.y) // don't draw off the start of the line segment
                rect.top = start.y;
            if(rect.bottom > end.y) // don't draw off the end of the line segment
                rect.bottom = end.y;
            if(rect.bottom > rect.top) // note the case where tl.y is in a space in the line segment
            {
                void_WrapOsBoolChecking(FillRect(hdc, &rect, hbrush));
            }
            totlen -= m_and_s;
            y += m_and_s;
        }
    }
    } /*block*/
}

extern void
host_paint_underline(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb,
    _InVal_     PIXIT line_thickness)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    PIXIT_LINE pixit_line = *p_pixit_line;
    PIXIT line_width;
    PIXIT_RECT pixit_rect;
    GDI_RECT gdi_rect;
    RECT rect;

    if(p_rgb->transparent)
        return;

    if( p_redraw_context->flags.printer  ||
        p_redraw_context->flags.drawfile )
        line_width = line_thickness /* was p_redraw_context->line_width.y */;
    else
        line_width = MAX(line_thickness, p_redraw_context->one_real_pixel.y /* was thin_width_eff */);

    // different to RISC OS here as we can see finer pixels so can always do rect_fill

#if TRACE_ALLOWED
    tracef(TRACE_APP_HOST_PAINT, TEXT("host_paint_underline: line tl ") S32_TFMT TEXT(",") S32_TFMT TEXT(" br ") S32_TFMT TEXT(",") S32_TFMT TEXT(" %s lw ") S32_TFMT,
           pixit_line.tl.x, pixit_line.tl.y, pixit_line.br.x, pixit_line.br.y, pixit_line.horizontal ? TEXT("H") : TEXT("V"),
           line_width);
#endif

    pixit_rect.tl.x = pixit_line.tl.x;
    pixit_rect.tl.y = pixit_line.tl.y;

    if(pixit_line.horizontal)
    {
        pixit_rect.br.x = pixit_line.br.x;
        pixit_rect.br.y = pixit_rect.tl.y + line_width;
    }
    else
    {
        pixit_rect.br.x = pixit_rect.tl.x + line_width;
        pixit_rect.br.y = pixit_line.br.y;
    }

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_underline: rect tl ") S32_TFMT TEXT(",") S32_TFMT TEXT(" br ") S32_TFMT TEXT(",") S32_TFMT, pixit_rect.tl.x, pixit_rect.tl.y, pixit_rect.br.x, pixit_rect.br.y);

    /* clip close to GDI limits (only ok because lines either horizontal or vertical) */
    if(!status_done(gdi_rect_limited_from_pixit_rect_and_context(&gdi_rect, &pixit_rect, p_redraw_context)))
        return;

    if(p_redraw_context->flags.metafile) // NB NOT drawfile !!!
    {
        /* sad bit of code here - punters want to see the lines in their metafiles in tiny views */
        int full_width = (pixit_line.horizontal ? (gdi_rect.br.y - gdi_rect.tl.y) : (gdi_rect.br.x - gdi_rect.tl.x));
        int half_width = full_width / 2;
        const int pen_style = PS_INSIDEFRAME;
        POINT wanky_point_start;
        POINT wanky_point_end;
        /* put the line down the middle of the rectangle that the line lies in */
        if(pixit_line.horizontal)
        {
            wanky_point_start.x = gdi_rect.tl.x + half_width; /* account for wanky rounded GDI end caps */
            wanky_point_start.y = gdi_rect.tl.y + half_width;
            wanky_point_end.x   = gdi_rect.br.x - half_width / 2;
            wanky_point_end.y   = wanky_point_start.y;
        }
        else
        {
            wanky_point_start.x = gdi_rect.tl.x + half_width;
            wanky_point_start.y = gdi_rect.tl.y + half_width;
            wanky_point_end.x   = wanky_point_start.x;
            wanky_point_end.y   = gdi_rect.br.y - half_width;
        }

        {
        HPEN hpen =
            CreatePen(pen_style,
                      full_width,
                      colorref_from_rgb(p_redraw_context, p_rgb));

        if(NULL != hpen)
        {
            HPEN old_hpen = SelectPen(hdc, hpen);

            void_WrapOsBoolChecking(
                MoveToEx(hdc, wanky_point_start.x, wanky_point_start.y, NULL));

            void_WrapOsBoolChecking(
                LineTo(hdc, wanky_point_end.x, wanky_point_end.y));

            void_WrapOsBoolChecking(
                DeletePen(SelectPen(hdc, old_hpen)));
        }
        } /*block*/

        /* always finish off with a rectangle */
    }

    if(p_redraw_context->flags.drawfile)
    {
        PC_DRAW_DASH_HEADER line_dash_pattern = NULL;
        S32 thickness = 0; /* thin, unless otherwise specified */

        {
        PIXIT pixit = pixit_line.horizontal ? (pixit_rect.br.y - pixit_rect.tl.y) : (pixit_rect.br.x - pixit_rect.tl.x);
        if(pixit > 0)
            thickness = pixit * GR_RISCDRAW_PER_PIXIT;
        }

        drawfile_paint_line(p_redraw_context, &pixit_rect, thickness, line_dash_pattern, p_rgb);

        return;
    }

    if((gdi_rect.tl.x >= gdi_rect.br.x) || (gdi_rect.tl.y >= gdi_rect.br.y))
    {
        /* line too small to plot using FillRect */
        trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_underline: box ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT TEXT(" FAILED"), gdi_rect.tl.x, gdi_rect.tl.y, gdi_rect.br.x, gdi_rect.br.y);
        return;
    }

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_underline: box ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT, gdi_rect.tl.x, gdi_rect.tl.y, gdi_rect.br.x, gdi_rect.br.y);

    rect.left   = gdi_rect.tl.x;
    rect.top    = gdi_rect.tl.y;
    rect.right  = gdi_rect.br.x;
    rect.bottom = gdi_rect.br.y;

    void_WrapOsBoolChecking(
        FillRect(hdc, &rect, hbrush_from_colour(p_redraw_context, p_rgb)));
}

/* Enquire about the screen size (actually the biggest client area will do)
 * and return the size in pixels for the caller to use.
 */

extern void
host_work_area_gdi_size_query(
    _OutRef_    P_GDI_SIZE p_gdi_size)
{
    RECT screen_rect;

    if(!WrapOsBoolChecking(SystemParametersInfo(SPI_GETWORKAREA, 0, &screen_rect, 0)))
    {
        p_gdi_size->cx = 0;
        p_gdi_size->cy = 0;
        return;
    }

    p_gdi_size->cx = screen_rect.right  - screen_rect.left;
    p_gdi_size->cy = screen_rect.bottom - screen_rect.top;
}

/* Render the given bitmap in the given rectangle.
*/

extern void
host_paint_bitmap(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _In_        HBITMAP hbitmap)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    RECT rect;
    LONG bmWidth;
    LONG bmHeight;

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&rect, p_pixit_rect, p_redraw_context)))
        return;
    {
    BITMAP bm;
    GetObject(hbitmap, sizeof(bm), &bm);
    bmWidth  = bm.bmWidth;
    bmHeight = bm.bmHeight;
    } /*block*/

    {
    const HDC hdcMem = CreateCompatibleDC(NULL);
    HBITMAP hbmOld = SelectBitmap(hdcMem, hbitmap);

    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255 /*alpha*/, AC_SRC_ALPHA };
    void_WrapOsBoolChecking(AlphaBlend(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hdcMem, 0, 0, bmWidth, bmHeight, bf));

    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    } /*block*/
}

/* Render the given GdipImage in the given rectangle.
*/

extern void
host_paint_gdip_image(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _In_        GdipImage gdip_image)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    RECT rect;

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&rect, p_pixit_rect, p_redraw_context)))
        return;

    consume_bool(GdipImage_Render(gdip_image, hdc, &rect));
}

/* Render the given Draw file at the required position.
 * This code calls the Draw rendering module from Dial Solutions
 * having first setup the transformation matrix.
*/

#if defined(UNUSED_KEEP_ALIVE)

/* test code for rendering-by-parts */

extern /*static*/ void
draw_render_range(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    HPDRAWMATRIX t,
    _InVal_     DRAW_DIAG_OFFSET sttObject_in,
    _InVal_     DRAW_DIAG_OFFSET endObject_in)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    DRAW_DIAG_OFFSET sttObject = sttObject_in;
    DRAW_DIAG_OFFSET endObject = endObject_in;
    P_BYTE pObject;
    draw_diag d;
    draw_error de;
    zero_struct(de);

    d.data = p_gr_riscdiag->draw_diag.data;
    d.length = p_gr_riscdiag->draw_diag.length;

    if(gr_riscdiag_object_first(p_gr_riscdiag, &sttObject, &endObject, &pObject, TRUE))
    {
        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = 0x0000FF00;
        rect.bottom = 0x0000FF00;

        do  {
            const U32 objectType = *DRAW_OBJHDR(U32, pObject, type);
            const U32 objectSize = *DRAW_OBJHDR(U32, pObject, size);

            switch(objectType)
            {
            case DRAW_OBJECT_TYPE_FONTLIST:
            case DRAW_OBJECT_TYPE_DS_WINFONTLIST:
            case DRAW_OBJECT_TYPE_OPTIONS:
                /*reportf(TEXT("draw_render_range(%p,0x%x:%d:0x%x..0x%x) - object ignored"), d.data, d.length, objectType, sttObject, sttObject + objectSize);*/
                break;

            case DRAW_OBJECT_TYPE_GROUP:
                /*reportf(TEXT("draw_render_range(%p,0x%x:%d:0x%x..0x%x) - group"), d.data, d.length, objectType, sttObject, sttObject + objectSize);*/
                break;

            default:
                /*reportf(TEXT("draw_render_range(%p,0x%x:%d:0x%x..0x%x)"), d.data, d.length, objectType, sttObject, sttObject + objectSize);*/
                consume_bool(Draw_RenderRange(&d, hdc, &rect/*&p_redraw_context->windows.paintstruct.rcPaint*/, t, sttObject, sttObject + objectSize, &de));
                /*if(de.errnum) reportf(TEXT("draw_render_range:de(%d,%s)"), de.errnum, report_sbstr(de.errmess));*/
                break;
            }
        }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &sttObject, &endObject, &pObject, TRUE));
    }
}

#endif /* UNUSED_KEEP_ALIVE */

extern void
host_paint_drawfile(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_GR_SCALE_PAIR p_gr_scale_pair,
    _InRef_     PC_DRAW_DIAG p_draw_diag,
    _InVal_     BOOL eor_paths)
{
    GDI_POINT gdi_point;
    GR_SCALE_PAIR gr_scale_pair;
    draw_matrix t;

    if(!p_draw_diag->length)
        return;

    assert(p_draw_diag->data);
    if(NULL == p_draw_diag->data)
        return;

    if(p_redraw_context->flags.drawfile)
    {
        assert(!eor_paths);
        drawfile_paint_drawfile(p_redraw_context, p_pixit_point, p_gr_scale_pair, p_draw_diag);
        return;
    }

    gdi_point_from_pixit_point_and_context(&gdi_point, p_pixit_point, p_redraw_context);

    gr_scale_pair.x = muldiv64(p_gr_scale_pair->x, p_redraw_context->host_xform.scale.t.x, p_redraw_context->host_xform.scale.b.x);
    gr_scale_pair.y = muldiv64(p_gr_scale_pair->y, p_redraw_context->host_xform.scale.t.y, p_redraw_context->host_xform.scale.b.y);

    /* translation part of matrix; move diagram to the point and then ever so slightly right and down */
    {
    PC_DRAW_FILE_HEADER p_draw_file_header;
    DRAW_BOX bbox;

    /* scaling part of matrix */
    t.a = + muldiv64(gr_scale_pair.x, p_redraw_context->pixels_per_inch.cx, 180 /* RISC OS units per inch */);
    t.b = 0;
    t.c = 0;
    t.d = - muldiv64(gr_scale_pair.y, p_redraw_context->pixels_per_inch.cy, 180);

    p_draw_file_header = (PC_DRAW_FILE_HEADER) p_draw_diag->data;
    bbox = p_draw_file_header->bbox;

    t.e = gdi_point.x << 8;
    t.f = gdi_point.y << 8;

    t.e -= muldiv64_floor(bbox.x0, p_redraw_context->pixels_per_inch.cx * gr_scale_pair.x, GR_SCALE_ONE * 180);
    t.f += muldiv64_ceil( bbox.y1, p_redraw_context->pixels_per_inch.cx * gr_scale_pair.y, GR_SCALE_ONE * 180);
    } /*block*/

    {
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    int OldMapMode = 0; /* keep dataflower happy */
    POINT old_origin = { 0, 0 };
    draw_diag d;
    draw_error de;
    zero_struct(de);

    d.data = p_draw_diag->data;
    d.length = p_draw_diag->length;

    Draw_SetColourMapping(host_dithering ? DRAW_COLOURDITHER : DRAW_COLOURNEAREST, (COLORREF) 0);

    assert(!eor_paths); /* XOR no longer works with GDI+ */
    UNREFERENCED_PARAMETER_InVal_(eor_paths);

    if(!p_redraw_context->flags.metafile)
    {
        OldMapMode = GetMapMode(hdc); /* preserve over call to Dial Solutions Draw rendering DLL */
        SetWindowOrgEx(hdc, 0, 0, &old_origin); /* NB I believe the new DLLs do this correctly */
    }

reportf(TEXT("hpd: Draw_RenderDiag(%p,%d)"), d.data, d.length);
    consume_bool(Draw_RenderDiag(&d, hdc, de_const_cast(LPRECT, &p_redraw_context->windows.paintstruct.rcPaint), &t, &de));
if(de.errmess[0]) reportf(TEXT("hpd:de %d,%s"), de.errnum, report_sbstr(de.errmess));

    if(!p_redraw_context->flags.metafile)
    {
        SetMapMode(hdc, OldMapMode);
        SetWindowOrgEx(hdc, old_origin.x, old_origin.y, NULL); /* after resetting mapping mode */
    }
    } /*block*/
}

/* Read the pixit size of the Draw file, returning the width and height in pixits */

extern void
host_read_drawfile_pixit_size(
    /*_In_reads_bytes_(DRAW_FILE_HEADER)*/ PC_ANY p_any,
    _OutRef_    P_PIXIT_SIZE p_pixit_size)
{
    PC_DRAW_FILE_HEADER p_draw_file_header = (PC_DRAW_FILE_HEADER) p_any;
    DRAW_POINT draw_size;

    draw_size.x = p_draw_file_header->bbox.x1 - p_draw_file_header->bbox.x0;
    draw_size.y = p_draw_file_header->bbox.y1 - p_draw_file_header->bbox.y0;

    /* flooring ok 'cos paths got rounded out on loading to worst-case pixels */
    p_pixit_size->cx = muldiv64_floor(draw_size.x, 1, 2 * RISCDRAW_PER_RISCOS) * 2 * PIXITS_PER_RISCOS;
    p_pixit_size->cy = muldiv64_floor(draw_size.y, 1, 2 * RISCDRAW_PER_RISCOS) * 2 * PIXITS_PER_RISCOS;
}

/*
calculate super- / sub- shifts
*/

_Check_return_
static inline PIXIT /* NB y +ve down, unlike RISC OS version */
windows_fonty_paint_calc_shift_y(
    _InRef_     PC_FONT_CONTEXT p_font_context)
{
    if(p_font_context->font_spec.superscript)
        return((p_font_context->ascent * 0) / 64);

    if(p_font_context->font_spec.subscript)
        return((p_font_context->ascent * 38) / 64);

    return(0);
}

_Check_return_
extern PIXIT
host_fonty_uchars_width(
    _HfontRef_  HOST_FONT host_font,
    _In_reads_(uchars_n) PC_USTR uchars,
    _InVal_     U32 uchars_n)
{
    const HDC hic_format_pixits = host_get_hic_format_pixits();
    HFONT h_font_old = SelectFont(hic_format_pixits, host_font);
    SIZE size;
    PIXIT pixit_width = 0;

    if(status_ok(uchars_GetTextExtentPoint32(hic_format_pixits, uchars, uchars_n, &size)))
        pixit_width = size.cx;

    consume(HFONT, SelectFont(hic_format_pixits, h_font_old));

    return(pixit_width);
}

#if defined(USE_CACHED_ABC_WIDTHS)

_Check_return_
static STATUS
compute_pixel_dx_array(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_FONT_CONTEXT p_font_context,
    _InRef_     PC_GDI_POINT p_gdi_point,
    _OutRef_    P_ARRAY_HANDLE p_h_log_pos)
{
    STATUS status;
    PC_ABC p_abc = array_rangec(&p_font_context->h_abc_widths, ABC, 0, 256);
    PIXIT pixit_x = p_pixit_point->x + p_redraw_context->pixit_origin.x;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(int), FALSE);
    U32 i;
    int * p_int;

    if(NULL == (p_int = al_array_alloc(p_h_log_pos, int, uchars_n, &array_init_block, &status)))
        return(status);

    /* work out the pixel position of each character (no need to subtract gdi_org and gdi_point in the loop) */
    for(i = 0; i < uchars_n; ++i)
    {
        U8 ch = uchars[i];
        pixit_x += p_abc[ch].abcA + p_abc[ch].abcB + p_abc[ch].abcC;
        p_int[i] = (int) (muldiv64_round_floor(pixit_x, p_redraw_context->host_xform.windows.divisor_of_pixels.x, p_redraw_context->host_xform.windows.multiplier_of_pixels.x));
        /*trace_4(TRACE_APP_FONTS, "[%d]%c x=%d pixits -> x=%d pixels", i, ch, pixit_x, p_int[i]);*/
    }

    /* convert the pixel position of each character into its width */
    i = uchars_n;
    while(--i >= 1)
        p_int[i] -= p_int[i - 1];
    p_int[0] -= (p_redraw_context->gdi_org.x + p_gdi_point->x); /* this is the one that needs offsetting */

#if TRACE_ALLOWED && 0
    if_constant(tracing(TRACE_APP_FONTS))
    {
        for(i = 0; i < uchars_n; ++i)
        {
            U8 ch = uchars[i];
            trace_3(TRACE_APP_FONTS, "[%d]%c dx=%d pixels", i, ch, p_int[i]);
        }
    }
#endif

    return(STATUS_OK);
}

#endif /* USE_CACHED_ABC_WIDTHS */

/* Attempt to paint the text into the current redraw context.
 * If printing the text is clipped to the current clipping rectangle.
 */

extern void
fonty_text_paint_rubout_uchars(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     S32 uchars_n,
    _InVal_     PIXIT base_line,
    _InRef_     PC_RGB p_rgb_back,
    _InVal_     FONTY_HANDLE fonty_handle,
    _InRef_     PC_PIXIT_RECT p_pixit_rect_rubout)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    PIXIT_POINT pixit_point = *p_pixit_point;
    const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);
    HOST_FONT host_font;
    GDI_POINT gdi_point;
    RECT rect;
#if defined(USE_CACHED_ABC_WIDTHS)
    ARRAY_HANDLE h_log_pos = 0;
#endif

    PTR_ASSERT(p_pixit_rect_rubout);

    assert(!(p_redraw_context->flags.printer  ||
             p_redraw_context->flags.metafile ||
             p_redraw_context->flags.drawfile ) );

    pixit_point.y += base_line;
    
    pixit_point.y += windows_fonty_paint_calc_shift_y(p_font_context);

    pixit_point.y -= p_font_context->ascent;

    gdi_point_from_pixit_point_and_context(&gdi_point, &pixit_point, p_redraw_context);

    host_font = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, p_docu->flags.draft_mode);
    assert(HOST_FONT_NONE != host_font);
    if(HOST_FONT_NONE == host_font)
        return;

    host_font_select(host_font, p_redraw_context, FALSE /*caller-controlled*/);

#if TRACE_ALLOWED && 0
    if_constant(tracing(TRACE_APP_FONTS)) trace_selected_font(p_redraw_context, TEXT("fonty_text_paint_rubout_uchars"));
#endif

    if(status_done(RECT_limited_from_pixit_rect_and_context(&rect, p_pixit_rect_rubout, p_redraw_context)))
    {
        PC_RGB p_rgb_back_fill = p_rgb_back->transparent ? &rgb_stash[COLOUR_OF_PAPER] : p_rgb_back;

        void_WrapOsBoolChecking(
            FillRect(hdc, &rect, hbrush_from_colour(p_redraw_context, p_rgb_back_fill)));
    }
    //SetBkColor(hdc, PALETTERGB(p_rgb_back->r, p_rgb_back->g, p_rgb_back->b));

    if(0 == uchars_n)
        return;

    void_WrapOsBoolChecking(CLR_INVALID !=
        SetTextColor(hdc, colorref_from_rgb(p_redraw_context, &p_font_context->font_spec.colour)));

#if USTR_IS_SBSTR && defined(USE_CACHED_ABC_WIDTHS)
    { /* 29.4.94 set character spacing in our sub-pixel units */
    if((uchars_n > 1) /* && !(p_redraw_context->flags.printer || p_redraw_context->flags.metafile || p_redraw_context->flags.drawfile)*/)
    {
        if(p_font_context->h_abc_widths)
        {
            STATUS status = compute_pixel_dx_array(p_redraw_context, &pixit_point, uchars, (U32) uchars_n, p_font_context, &gdi_point, &h_log_pos);

            status_assert(status);
        }
    }
    } /*block*/

    status_assert(
        uchars_ExtTextOut(hdc,
                          gdi_point.x, gdi_point.y,
                          0, NULL,
                          uchars, (int) uchars_n,
                          h_log_pos ? array_rangec(&h_log_pos, INT, 0, uchars_n - 1) : NULL));

    al_array_dispose(&h_log_pos);
#else
    status_assert(
        uchars_ExtTextOut(hdc,
                          gdi_point.x, gdi_point.y,
                          0, NULL,
                          uchars, (int) uchars_n,
                          NULL));
#endif
}

extern void
fonty_text_paint_simple_uchars(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     S32 uchars_n,
    _InVal_     PIXIT base_line,
    _InRef_     PC_RGB p_rgb_back,
    _InVal_     FONTY_HANDLE fonty_handle)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    PIXIT_POINT pixit_point = *p_pixit_point;
    const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);
    HOST_FONT host_font;
    PCRECT prect = NULL;
    GDI_POINT gdi_point;
#if defined(USE_CACHED_ABC_WIDTHS)
    ARRAY_HANDLE h_log_pos = 0;
#endif

    if(0 == uchars_n)
        return;

    pixit_point.y += base_line;

    if(p_redraw_context->flags.drawfile)
    {
        pixit_point.y += drawfile_fonty_paint_calc_shift_y(p_font_context);
        drawfile_paint_uchars(p_redraw_context, &pixit_point, uchars, uchars_n, p_rgb_back, p_font_context);
        return;
    }

    pixit_point.y += windows_fonty_paint_calc_shift_y(p_font_context);

    pixit_point.y -= p_font_context->ascent;

    gdi_point_from_pixit_point_and_context(&gdi_point, &pixit_point, p_redraw_context);

    host_font = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, p_docu->flags.draft_mode);
    assert(HOST_FONT_NONE != host_font);
    if(HOST_FONT_NONE == host_font)
        return;

    host_font_select(host_font, p_redraw_context, FALSE /*caller-controlled*/);

#if TRACE_ALLOWED && 0
    if_constant(tracing(TRACE_APP_FONTS)) trace_selected_font(p_redraw_context, TEXT("fonty_text_paint_simple_uchars"));
#endif

    if(p_redraw_context->flags.metafile)
        prect = &p_redraw_context->windows.host_machine_clip_rect;

    void_WrapOsBoolChecking(CLR_INVALID !=
        SetTextColor(hdc, colorref_from_rgb(p_redraw_context, &p_font_context->font_spec.colour)));

#if USTR_IS_SBSTR && defined(USE_CACHED_ABC_WIDTHS)
    { /* 29.4.94 set character spacing in our sub-pixel units */
    if((uchars_n > 1) && !(p_redraw_context->flags.printer || p_redraw_context->flags.metafile || p_redraw_context->flags.drawfile))
    {
        if(p_font_context->h_abc_widths)
        {
            STATUS status = compute_pixel_dx_array(p_redraw_context, &pixit_point, uchars, (U32) uchars_n, p_font_context, &gdi_point, &h_log_pos);

            status_assert(status);
        }
    }
    } /*block*/

    status_assert(
        uchars_ExtTextOut(hdc,
                          gdi_point.x, gdi_point.y,
                          prect ? ETO_CLIPPED : 0, /* force to clip when output to metafile */
                          prect,
                          uchars, (int) uchars_n,
                          h_log_pos ? array_rangec(&h_log_pos, INT, 0, uchars_n - 1) : NULL));

    al_array_dispose(&h_log_pos);
#else
    status_assert(
        uchars_ExtTextOut(hdc,
                          gdi_point.x, gdi_point.y,
                          prect ? ETO_CLIPPED : 0, /* force to clip when output to metafile */
                          prect,
                          uchars, (int) uchars_n,
                          NULL));
#endif
}

#if TSTR_IS_SBSTR

extern void
fonty_text_paint_rubout_wchars(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(wchars_n) PCWCH wchars,
    _InVal_     S32 wchars_n,
    _InVal_     PIXIT base_line,
    _InRef_     PC_RGB p_rgb_back,
    _InVal_     FONTY_HANDLE fonty_handle,
    _InRef_     PC_PIXIT_RECT p_pixit_rect_rubout)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    PIXIT_POINT pixit_point = *p_pixit_point;
    const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);
    HOST_FONT host_font;
    GDI_POINT gdi_point;
    RECT rect;

    PTR_ASSERT(p_pixit_rect_rubout);

    assert(!(p_redraw_context->flags.printer  ||
             p_redraw_context->flags.metafile ||
             p_redraw_context->flags.drawfile ) );

    pixit_point.y += base_line;
    
    pixit_point.y += windows_fonty_paint_calc_shift_y(p_font_context);

    pixit_point.y -= p_font_context->ascent;

    gdi_point_from_pixit_point_and_context(&gdi_point, &pixit_point, p_redraw_context);

    host_font = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, p_docu->flags.draft_mode);
    assert(HOST_FONT_NONE != host_font);
    if(HOST_FONT_NONE == host_font)
        return;

    host_font_select(host_font, p_redraw_context, FALSE /*caller-controlled*/);

#if TRACE_ALLOWED && 0
    if_constant(tracing(TRACE_APP_FONTS)) trace_selected_font(p_redraw_context, TEXT("fonty_text_paint_rubout_wchars"));
#endif

    if(status_done(RECT_limited_from_pixit_rect_and_context(&rect, p_pixit_rect_rubout, p_redraw_context)))
    {
        PC_RGB p_rgb_back_fill = p_rgb_back->transparent ? &rgb_stash[COLOUR_OF_PAPER] : p_rgb_back;

        void_WrapOsBoolChecking(
            FillRect(hdc, &rect, hbrush_from_colour(p_redraw_context, p_rgb_back_fill)));
    }
    //SetBkColor(hdc, PALETTERGB(p_rgb_back->r, p_rgb_back->g, p_rgb_back->b));

    void_WrapOsBoolChecking(CLR_INVALID !=
        SetTextColor(hdc, colorref_from_rgb(p_redraw_context, &p_font_context->font_spec.colour)));

    status_assert(
              ExtTextOutW(hdc,
                          gdi_point.x, gdi_point.y,
                          0, NULL,
                          wchars, (int) wchars_n,
                          NULL));
}

extern void
fonty_text_paint_simple_wchars(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(wchars_n) PCWCH wchars,
    _InVal_     S32 wchars_n,
    _InVal_     PIXIT base_line,
    _InRef_     PC_RGB p_rgb_back,
    _InVal_     FONTY_HANDLE fonty_handle)
{
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    PIXIT_POINT pixit_point = *p_pixit_point;
    const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);
    HOST_FONT host_font;
    PCRECT prect = NULL;
    GDI_POINT gdi_point;

    pixit_point.y += base_line;

    if(p_redraw_context->flags.drawfile)
    {
        pixit_point.y += drawfile_fonty_paint_calc_shift_y(p_font_context);
        UNREFERENCED_PARAMETER_InRef_(p_rgb_back);
        // <<< drawfile_paint_uchars(p_redraw_context, &pixit_point, uchars, uchars_n, p_rgb_back, p_font_context);
        return;
    }

    pixit_point.y += windows_fonty_paint_calc_shift_y(p_font_context);

    pixit_point.y -= p_font_context->ascent;

    gdi_point_from_pixit_point_and_context(&gdi_point, &pixit_point, p_redraw_context);

    host_font = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, p_docu->flags.draft_mode);
    assert(HOST_FONT_NONE != host_font);
    if(HOST_FONT_NONE == host_font)
        return;

    host_font_select(host_font, p_redraw_context, FALSE /*caller-controlled*/);

#if TRACE_ALLOWED && 0
    if_constant(tracing(TRACE_APP_FONTS)) trace_selected_font(p_redraw_context, TEXT("fonty_text_paint_simple_wchars"));
#endif

    if(p_redraw_context->flags.metafile)
        prect = &p_redraw_context->windows.host_machine_clip_rect;

    void_WrapOsBoolChecking(CLR_INVALID !=
        SetTextColor(hdc, colorref_from_rgb(p_redraw_context, &p_font_context->font_spec.colour)));

    status_assert(
              ExtTextOutW(hdc,
                          gdi_point.x, gdi_point.y,
                          prect ? ETO_CLIPPED : 0, /* force to clip when output to metafile */
                          prect,
                          wchars, (int) wchars_n,
                          NULL));
}

#endif /* TSTR_IS_SBSTR */

#if USTR_IS_SBSTR

/* uchars STATUS returning functions like their UTF-8 counterparts */

_Check_return_
extern STATUS
uchars_ExtTextOut(
    _HdcRef_    HDC hdc,
    _In_        int x,
    _In_        int y,
    _In_        UINT options,
    _In_opt_    CONST RECT *pRect,
    _In_reads_opt_(uchars_n) PC_UCHARS pString,
    _InVal_     U32 uchars_n,
    _In_opt_    CONST INT *pDx)
{
    STATUS status = STATUS_OK;
    BOOL res;

    res = WrapOsBoolChecking(ExtTextOut(hdc, x, y, options, pRect, pString, uchars_n, pDx));

    if(!res)
        status = status_check();

    return(status);
}

_Check_return_
extern STATUS
uchars_GetTextExtentPoint32(
    _HdcRef_    HDC hdc,
    _In_reads_(uchars_n) PC_UCHARS pString,
    _InVal_     U32 uchars_n,
    _OutRef_    PSIZE pSize)
{
    STATUS status = STATUS_OK;
    BOOL res;
    
    res = WrapOsBoolChecking(GetTextExtentPoint32(hdc, pString, uchars_n, pSize));

    if(!res)
        status = status_check();
#if TRACE_ALLOWED
    else if_constant(tracing(TRACE_APP_FONTS))
    {
        trace_5(TRACE_APP_FONTS,
                TEXT("uchars_GetTextExtentPoint32: got: %d,%d %s for %.*s"),
                pSize->cx, pSize->cy,
                (MM_TWIPS == GetMapMode(hdc)) ? TEXT("twips") : TEXT("pixels"),
                uchars_n, report_ustr(pString));
    }
#endif

    return(status);
}

#endif /* USTR_IS_SBSTR */

/* Handle the pointer changes within the system, the pointer shape can originate from several sources,
 * including system ones, bttncur extensions and bound cursors.
 * Some such cursors may need to be discarded and therefore require the the code tidies up before loading a new shape.
 *
 * To further complicate the situation we must also handle the hourglass shape ourself, this infact
 * is quite easy in that we check the count, if it is non-zero then we keep a copy of the pointer shape (for restore)
 * and then change it to be a wait symbol.
 */

#define TYPE5_RESOURCE_CURSOR_ARROWS    100
#define TYPE5_RESOURCE_CURSOR_ARROWS_LR 101
#define TYPE5_RESOURCE_CURSOR_ARROWS_UD 102
#define TYPE5_RESOURCE_CURSOR_COL       103
#define TYPE5_RESOURCE_CURSOR_COL_L     104
#define TYPE5_RESOURCE_CURSOR_COL_R     105
#define TYPE5_RESOURCE_CURSOR_ROW       106
#define TYPE5_RESOURCE_CURSOR_ROW_U     107
#define TYPE5_RESOURCE_CURSOR_ROW_D     108
#define TYPE5_RESOURCE_CURSOR_SPLIT     109
#define TYPE5_RESOURCE_CURSOR_SPLIT_LR  110
#define TYPE5_RESOURCE_CURSOR_SPLIT_UD  111
#define TYPE5_RESOURCE_CURSOR_TANK      112

enum CURSOR_SOURCE
{
    CURSOR_SOURCE_SYSTEM,
    CURSOR_SOURCE_BOUND
};

typedef struct POINTER_INFO
{
    BOOL discard;
    enum CURSOR_SOURCE cursor_source;
    POINT offset;
    PCTSTR id; /* for LoadImage */
}
POINTER_INFO; typedef const POINTER_INFO * PC_POINTER_INFO;

static const POINTER_INFO
pointer_table[POINTER_SHAPE_COUNT] =
{
    { 0, CURSOR_SOURCE_SYSTEM,  {  0,  0 }, IDC_ARROW },                                        /* pointer default */
    { 0, CURSOR_SOURCE_SYSTEM,  {  0,  0 }, IDC_IBEAM },                                        /* caret */
    { 0, CURSOR_SOURCE_SYSTEM,  {  0,  0 }, IDC_ARROW },                                        /* menu icon - indicates a drop down (RISC OS only) */

    { 1, CURSOR_SOURCE_BOUND,   {  0,  0 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_TANK) },      /* formula line cursor */

    { 0, CURSOR_SOURCE_SYSTEM,  {  0,  0 }, IDC_HAND },                                         /* a hand */

    { 1, CURSOR_SOURCE_BOUND,   { -2, -2 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_ARROWS) },    /* drag all - four arrows */
    { 1, CURSOR_SOURCE_BOUND,   { -2,  0 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_ARROWS_LR) }, /* arrows left right */
    { 1, CURSOR_SOURCE_BOUND,   {  0, -2 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_ARROWS_UD) }, /* arrows up and down */

    { 1, CURSOR_SOURCE_BOUND,   {  0,  0 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_COL) },       /* drag column */
    { 1, CURSOR_SOURCE_BOUND,   {  0,  0 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_COL_L) },     /* drag column left */
    { 1, CURSOR_SOURCE_BOUND,   {  0,  0 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_COL_R) },     /* drag column right */

    { 1, CURSOR_SOURCE_BOUND,   {  0,  0 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_ROW) },       /* drag row */
    { 1, CURSOR_SOURCE_BOUND,   {  0,  0 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_ROW_U) },     /* drag row up */
    { 1, CURSOR_SOURCE_BOUND,   {  0,  0 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_ROW_D) },     /* drag row down */

    { 1, CURSOR_SOURCE_BOUND,   { -2, -2 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_SPLIT) },     /* split cursor (all four points) */
    { 1, CURSOR_SOURCE_BOUND,   { -2,  0 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_SPLIT_LR) },  /* split point (left / right) */
    { 1, CURSOR_SOURCE_BOUND,   {  0, -2 }, MAKEINTRESOURCE(TYPE5_RESOURCE_CURSOR_SPLIT_UD) }   /* split point (up / down) */
};

/* Cursor state information - which one is active etc. */

static
struct CURSOR_STATE_INFO
{
    POINTER_SHAPE pointer_shape;
    POINTER_SHAPE old_pointer_shape;

    HCURSOR hcursor;
    HCURSOR hcursor_discard;
}
cursor =
{
    POINTER_UNINIT /*no cursor initially*/,
    POINTER_UNINIT
};

extern void
ho_paint_SetCursor(void) // called on WM_SETCURSOR
{
    if(NULL != cursor.hcursor)
    {
        if(cursor.hcursor == cursor.hcursor_discard) // some confusion of states can get it here!
            cursor.hcursor_discard = NULL;

        SetCursor(cursor.hcursor);
    }

    if(NULL != cursor.hcursor_discard) // Blow cursor to be deleted only after new one has been selected
    {
        void_WrapOsBoolChecking(DestroyCursor(cursor.hcursor_discard));
        cursor.hcursor_discard = NULL;
    }
}

extern void
host_set_pointer_shape(
    _In_        POINTER_SHAPE pointer_shape)
{
    /* Check for same cursor already cached */
    if(cursor.pointer_shape == pointer_shape)
        return;

    trace_1(TRACE_WINDOWS_HOST, TEXT("host_set_pointer_shape: Caching pointer shape %d"), pointer_shape);

    if(cursor.pointer_shape != POINTER_UNINIT)
    {
        if((U32) cursor.pointer_shape >= elemof32(pointer_table))
        {
            assert((U32) cursor.pointer_shape < elemof32(pointer_table));
            return;
        }

        if(pointer_table[cursor.pointer_shape].discard)
            cursor.hcursor_discard = cursor.hcursor;
    }

    if((U32) pointer_shape >= elemof32(pointer_table))
    {
        assert((U32) pointer_shape < elemof32(pointer_table));
        return;
    }

    cursor.pointer_shape = pointer_shape;

    {
    const PC_POINTER_INFO p_pointer_info = &pointer_table[pointer_shape];

    /* Load from the required source */
    switch(p_pointer_info->cursor_source)
    {
    default: default_unhandled();
#if CHECKING
    case CURSOR_SOURCE_SYSTEM:
#endif
        cursor.hcursor = LoadCursor(NULL, p_pointer_info->id); /* don't destroy these */
        break;

    case CURSOR_SOURCE_BOUND:
        {
        HINSTANCE hInstance = resource_get_object_resources(OBJECT_ID_SKEL);
        cursor.hcursor = (HCURSOR) LoadImage(hInstance, p_pointer_info->id, IMAGE_CURSOR, 0, 0, 0);
        break;
        }
    }
    } /*block*/
}

extern void
host_modify_click_point(
    _InoutRef_  P_GDI_POINT p_gdi_point)
{
    if((cursor.pointer_shape >= 0) && (cursor.pointer_shape < elemof32(pointer_table)))
    {
        const PC_POINTER_INFO p_pointer_info = &pointer_table[cursor.pointer_shape];
        p_gdi_point->x += p_pointer_info->offset.x;
        p_gdi_point->y += p_pointer_info->offset.y;
    }
}

extern void
ho_paint_msg_exit2(void)
{
    /* Remove any cached and non-system cursor shapes */
    if(NULL != cursor.hcursor)
    {
        const POINTER_SHAPE pointer_shape = (cursor.pointer_shape >= 0) ? cursor.pointer_shape : cursor.old_pointer_shape;

        if((pointer_shape >= 0) && pointer_table[pointer_shape].discard)
            void_WrapOsBoolChecking(DestroyCursor(cursor.hcursor));

        cursor.hcursor = NULL;
    }

    if(NULL != cursor.hcursor_discard) // Blow cursor to be deleted in case we haven't already
    {
        void_WrapOsBoolChecking(DestroyCursor(cursor.hcursor_discard));
        cursor.hcursor_discard = NULL;
    }
}

#endif /* WINDOWS */

/* end of windows/ho_paint.c */
