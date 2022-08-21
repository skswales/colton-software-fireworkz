/* windows/ho_xform.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS November 1993 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/******************************************************************************
*
* returns tl of device pixel corresponding to point
*
******************************************************************************/

extern void
gdi_point_from_pixit_point_and_context(
    _OutRef_    P_GDI_POINT p_gdi_point,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT_POINT pixit_point;
    GDI_POINT gdi_point;

    pixit_point.x = p_pixit_point->x + p_redraw_context->pixit_origin.x;
    pixit_point.y = p_pixit_point->y + p_redraw_context->pixit_origin.y;

    gdi_point.x = muldiv64_round_floor(pixit_point.x, p_redraw_context->host_xform.windows.divisor_of_pixels.x, p_redraw_context->host_xform.windows.multiplier_of_pixels.x);
    gdi_point.y = muldiv64_round_floor(pixit_point.y, p_redraw_context->host_xform.windows.divisor_of_pixels.y, p_redraw_context->host_xform.windows.multiplier_of_pixels.y);

    p_gdi_point->x = gdi_point.x - p_redraw_context->gdi_org.x;
    p_gdi_point->y = gdi_point.y - p_redraw_context->gdi_org.y;
}

/******************************************************************************
*
* returns x0i,y0i,x1i,y1i rect corresponding to rect
*
* STATUS_OK iff rect should not be plotted else STATUS_DONE
*
******************************************************************************/

_Check_return_
extern STATUS
gdi_rect_from_pixit_rect_and_context(
    _OutRef_    P_GDI_RECT p_gdi_rect,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT_RECT pixit_rect;
    GDI_RECT gdi_rect;

    pixit_rect.tl.x = p_pixit_rect->tl.x + p_redraw_context->pixit_origin.x;
    pixit_rect.tl.y = p_pixit_rect->tl.y + p_redraw_context->pixit_origin.y;
    pixit_rect.br.x = p_pixit_rect->br.x + p_redraw_context->pixit_origin.x;
    pixit_rect.br.y = p_pixit_rect->br.y + p_redraw_context->pixit_origin.y;

    gdi_rect.tl.x = muldiv64_round_floor(pixit_rect.tl.x, p_redraw_context->host_xform.windows.divisor_of_pixels.x, p_redraw_context->host_xform.windows.multiplier_of_pixels.x);
    gdi_rect.tl.y = muldiv64_round_floor(pixit_rect.tl.y, p_redraw_context->host_xform.windows.divisor_of_pixels.y, p_redraw_context->host_xform.windows.multiplier_of_pixels.y);
    gdi_rect.br.x = muldiv64_round_floor(pixit_rect.br.x, p_redraw_context->host_xform.windows.divisor_of_pixels.x, p_redraw_context->host_xform.windows.multiplier_of_pixels.x);
    gdi_rect.br.y = muldiv64_round_floor(pixit_rect.br.y, p_redraw_context->host_xform.windows.divisor_of_pixels.y, p_redraw_context->host_xform.windows.multiplier_of_pixels.y);

    p_gdi_rect->tl.x = gdi_rect.tl.x - p_redraw_context->gdi_org.x;
    p_gdi_rect->tl.y = gdi_rect.tl.y - p_redraw_context->gdi_org.y;
    p_gdi_rect->br.x = gdi_rect.br.x - p_redraw_context->gdi_org.x;
    p_gdi_rect->br.y = gdi_rect.br.y - p_redraw_context->gdi_org.y;

    /* i,i,e,e return! */
    if((p_gdi_rect->tl.x >= p_gdi_rect->br.x) || (p_gdi_rect->tl.y >= p_gdi_rect->br.y))
        return(STATUS_OK);

    return(STATUS_DONE);
}

/* STATUS_OK iff rect should not be plotted else STATUS_DONE */

_Check_return_
extern STATUS
gdi_rect_limited_from_pixit_rect_and_context(
    _OutRef_    P_GDI_RECT p_gdi_rect,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    status_consume(gdi_rect_from_pixit_rect_and_context(p_gdi_rect, p_pixit_rect, p_redraw_context));

    if(p_gdi_rect->tl.x < T5_GDI_MIN_X) p_gdi_rect->tl.x = T5_GDI_MIN_X; /* left */
    if(p_gdi_rect->tl.y < T5_GDI_MIN_Y) p_gdi_rect->tl.y = T5_GDI_MIN_Y; /* top */
    if(p_gdi_rect->br.x > T5_GDI_MAX_X) p_gdi_rect->br.x = T5_GDI_MAX_X; /* right */
    if(p_gdi_rect->br.y > T5_GDI_MAX_Y) p_gdi_rect->br.y = T5_GDI_MAX_Y; /* bottom */

    if((p_gdi_rect->tl.x >= p_gdi_rect->br.x) || (p_gdi_rect->tl.y >= p_gdi_rect->br.y))
        return(STATUS_OK);

    return(STATUS_DONE);
}

_Check_return_
extern STATUS
RECT_limited_from_pixit_rect_and_context(
    _OutRef_    PRECT p_rect,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    GDI_RECT gdi_rect;

    status_consume(gdi_rect_from_pixit_rect_and_context(&gdi_rect, p_pixit_rect, p_redraw_context));

    p_rect->left   = MAX(T5_GDI_MIN_X, gdi_rect.tl.x);
    p_rect->top    = MAX(T5_GDI_MIN_Y, gdi_rect.tl.y);
    p_rect->right  = MIN(T5_GDI_MAX_X, gdi_rect.br.x);
    p_rect->bottom = MIN(T5_GDI_MAX_Y, gdi_rect.br.y);

    /* i,i,e,e return! */
    if((p_rect->left >= p_rect->right) || (p_rect->top >= p_rect->bottom))
        return(STATUS_OK);

    return(STATUS_DONE);
}

_Check_return_
extern STATUS
gdi_rect_from_pixit_rect_and_context_draw(
    _OutRef_    P_GDI_RECT p_gdi_rect,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT_RECT pixit_rect;
    S32_RECT draw_units;

    pixit_rect.tl.x = p_pixit_rect->tl.x + p_redraw_context->pixit_origin.x;
    pixit_rect.tl.y = p_pixit_rect->tl.y + p_redraw_context->pixit_origin.y;
    pixit_rect.br.x = p_pixit_rect->br.x + p_redraw_context->pixit_origin.x;
    pixit_rect.br.y = p_pixit_rect->br.y + p_redraw_context->pixit_origin.y;

    draw_units.tl.x = muldiv64_round_floor(pixit_rect.tl.x, p_redraw_context->host_xform.windows.divisor_of_pixels.x << 8, p_redraw_context->host_xform.windows.multiplier_of_pixels.x);
    draw_units.tl.y = muldiv64_round_floor(pixit_rect.tl.y, p_redraw_context->host_xform.windows.divisor_of_pixels.y << 8, p_redraw_context->host_xform.windows.multiplier_of_pixels.y);
    draw_units.br.x = muldiv64_round_floor(pixit_rect.br.x, p_redraw_context->host_xform.windows.divisor_of_pixels.x << 8, p_redraw_context->host_xform.windows.multiplier_of_pixels.x);
    draw_units.br.y = muldiv64_round_floor(pixit_rect.br.y, p_redraw_context->host_xform.windows.divisor_of_pixels.y << 8, p_redraw_context->host_xform.windows.multiplier_of_pixels.y);

    p_gdi_rect->tl.x = draw_units.tl.x - (p_redraw_context->gdi_org.x << 8);
    p_gdi_rect->tl.y = draw_units.tl.y - (p_redraw_context->gdi_org.y << 8);
    p_gdi_rect->br.x = draw_units.br.x - (p_redraw_context->gdi_org.x << 8);
    p_gdi_rect->br.y = draw_units.br.y - (p_redraw_context->gdi_org.y << 8);

    return(STATUS_DONE);
}

/******************************************************************************
*
* NB Does not set org or origin
*
******************************************************************************/

extern void
host_redraw_context_set_host_xform(
    _InoutRef_  P_REDRAW_CONTEXT p_redraw_context,
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/)
{
    p_host_xform->windows.pixels_per_inch.x = GetDeviceCaps(p_redraw_context->windows.paintstruct.hdc, LOGPIXELSX);
    p_host_xform->windows.pixels_per_inch.y = GetDeviceCaps(p_redraw_context->windows.paintstruct.hdc, LOGPIXELSY);

    p_host_xform->windows.pixels_per_metre.x = muldiv64(p_host_xform->windows.pixels_per_inch.x, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);
    p_host_xform->windows.pixels_per_metre.y = muldiv64(p_host_xform->windows.pixels_per_inch.y, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);

    p_host_xform->windows.multiplier_of_pixels.x = PIXITS_PER_METRE * p_host_xform->scale.b.x;
    p_host_xform->windows.multiplier_of_pixels.y = PIXITS_PER_METRE * p_host_xform->scale.b.y;

    p_host_xform->windows.divisor_of_pixels.x = p_host_xform->windows.pixels_per_metre.x * p_host_xform->scale.t.x;
    p_host_xform->windows.divisor_of_pixels.y = p_host_xform->windows.pixels_per_metre.y * p_host_xform->scale.t.y;

    p_redraw_context->host_xform = *p_host_xform;
}

extern void
host_redraw_context_fillin(
    _InoutRef_  P_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT_RECT pixit_rect;
    GDI_RECT gdi_rect;

    /* users of one_pixel please note that it is only an approximation and MUST BE >= real pixel size */
    p_redraw_context->one_real_pixel.x = muldiv64(p_redraw_context->host_xform.scale.b.x, PIXITS_PER_METRE, p_redraw_context->host_xform.windows.pixels_per_metre.x);
    p_redraw_context->one_real_pixel.x = idiv_ceil(p_redraw_context->one_real_pixel.x, p_redraw_context->host_xform.scale.t.x);
    p_redraw_context->one_real_pixel.y = muldiv64(p_redraw_context->host_xform.scale.b.y, PIXITS_PER_METRE, p_redraw_context->host_xform.windows.pixels_per_metre.y);
    p_redraw_context->one_real_pixel.y = idiv_ceil(p_redraw_context->one_real_pixel.y, p_redraw_context->host_xform.scale.t.y);

    p_redraw_context->one_program_pixel.x = p_redraw_context->host_xform.scale.b.x * PIXITS_PER_PROGRAM_PIXEL_X;
    p_redraw_context->one_program_pixel.x = idiv_ceil(p_redraw_context->one_program_pixel.x, p_redraw_context->host_xform.scale.t.x);
    p_redraw_context->one_program_pixel.y = p_redraw_context->host_xform.scale.b.y * PIXITS_PER_PROGRAM_PIXEL_Y;
    p_redraw_context->one_program_pixel.y = idiv_ceil(p_redraw_context->one_program_pixel.y, p_redraw_context->host_xform.scale.t.y);

    /* standard (half-width) lines */
    p_redraw_context->line_width.x = p_redraw_context->border_width.x / 2;
    p_redraw_context->line_width.y = p_redraw_context->border_width.y / 2;

    pixit_rect.tl.x = pixit_rect.tl.y = 0;
    pixit_rect.br.x = pixit_rect.tl.x + p_redraw_context->line_width.x;
    pixit_rect.br.y = pixit_rect.tl.y + p_redraw_context->line_width.y;

    status_consume(window_rect_from_pixit_rect(&gdi_rect, &pixit_rect, &p_redraw_context->host_xform));

    p_redraw_context->line_width_eff.x = ((gdi_rect.br.x - gdi_rect.tl.x) > 1)
                                       ? p_redraw_context->line_width.x
                                       : MIN(p_redraw_context->border_width.x, p_redraw_context->one_real_pixel.x);
    p_redraw_context->line_width_eff.y = ((gdi_rect.br.y - gdi_rect.tl.y) > 1)
                                       ? p_redraw_context->line_width.y
                                       : MIN(p_redraw_context->border_width.y, p_redraw_context->one_real_pixel.y);
    /* thin lines */
    p_redraw_context->thin_width.x = 1; /* pixits */
    p_redraw_context->thin_width.y = 1;

    pixit_rect.tl.x = pixit_rect.tl.y = 0;
    pixit_rect.br.x = pixit_rect.tl.x + p_redraw_context->thin_width.x;
    pixit_rect.br.y = pixit_rect.tl.y + p_redraw_context->thin_width.y;

    status_consume(window_rect_from_pixit_rect(&gdi_rect, &pixit_rect, &p_redraw_context->host_xform));

    p_redraw_context->thin_width_eff.x = ((gdi_rect.br.x - gdi_rect.tl.x) > 1)
                                       ? p_redraw_context->thin_width.x
                                       : MIN(p_redraw_context->border_width.x, p_redraw_context->one_real_pixel.x);
    p_redraw_context->thin_width_eff.y = ((gdi_rect.br.y - gdi_rect.tl.y) > 1)
                                       ? p_redraw_context->thin_width.y
                                       : MIN(p_redraw_context->border_width.y, p_redraw_context->one_real_pixel.y);

    p_redraw_context->pixels_per_inch.cx = p_redraw_context->host_xform.windows.pixels_per_inch.x;
    p_redraw_context->pixels_per_inch.cy = p_redraw_context->host_xform.windows.pixels_per_inch.y;
    assert(p_redraw_context->pixels_per_inch.cx == GetDeviceCaps(p_redraw_context->windows.paintstruct.hdc, LOGPIXELSX));
    assert(p_redraw_context->pixels_per_inch.cy == GetDeviceCaps(p_redraw_context->windows.paintstruct.hdc, LOGPIXELSY));
}

static void
host_click_context_set_host_xform(
    _InoutRef_  P_CLICK_CONTEXT p_click_context,
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/)
{
    { /* NB. clicks are in views and so pertain only to the screen */
    const HDC hic_display = CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);
    assert(hic_display);
    p_host_xform->windows.pixels_per_inch.x = GetDeviceCaps(hic_display, LOGPIXELSX);
    p_host_xform->windows.pixels_per_inch.y = GetDeviceCaps(hic_display, LOGPIXELSY);
    void_WrapOsBoolChecking(DeleteDC(hic_display));
    } /*block*/

    p_host_xform->windows.pixels_per_metre.x = muldiv64(p_host_xform->windows.pixels_per_inch.x, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);
    p_host_xform->windows.pixels_per_metre.y = muldiv64(p_host_xform->windows.pixels_per_inch.y, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);

    p_host_xform->windows.multiplier_of_pixels.x = PIXITS_PER_METRE * p_host_xform->scale.b.x;
    p_host_xform->windows.multiplier_of_pixels.y = PIXITS_PER_METRE * p_host_xform->scale.b.y;

    p_host_xform->windows.divisor_of_pixels.x = p_host_xform->windows.pixels_per_metre.x * p_host_xform->scale.t.x;
    p_host_xform->windows.divisor_of_pixels.y = p_host_xform->windows.pixels_per_metre.y * p_host_xform->scale.t.y;

    p_click_context->host_xform = *p_host_xform;
}

extern void
host_set_click_context(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InoutRef_  P_CLICK_CONTEXT p_click_context,
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/)
{
    host_click_context_set_host_xform(p_click_context, p_host_xform);

    p_click_context->p_view = p_view;

    p_click_context->pixit_origin.x = 0;
    p_click_context->pixit_origin.y = 0;

    p_click_context->display_mode = p_view->display_mode;

    p_click_context->border_width.x = p_click_context->border_width.y = p_docu->page_def.grid_size;

    /* users of one_pixel please note that it is only an approximation and MUST BE >= real pixel size */
    p_click_context->one_real_pixel.x = muldiv64(p_click_context->host_xform.scale.b.x, PIXITS_PER_METRE, p_click_context->host_xform.windows.pixels_per_metre.x);
    p_click_context->one_real_pixel.x = idiv_ceil(p_click_context->one_real_pixel.x, p_click_context->host_xform.scale.t.x);
    p_click_context->one_real_pixel.y = muldiv64(p_click_context->host_xform.scale.b.y, PIXITS_PER_METRE, p_click_context->host_xform.windows.pixels_per_metre.y);
    p_click_context->one_real_pixel.y = idiv_ceil(p_click_context->one_real_pixel.y, p_click_context->host_xform.scale.t.y);

    p_click_context->one_program_pixel.x = p_click_context->host_xform.scale.b.x * PIXITS_PER_PROGRAM_PIXEL_X;
    p_click_context->one_program_pixel.x = idiv_ceil(p_click_context->one_program_pixel.x, p_click_context->host_xform.scale.t.x);
    p_click_context->one_program_pixel.y = p_click_context->host_xform.scale.b.y * PIXITS_PER_PROGRAM_PIXEL_Y;
    p_click_context->one_program_pixel.y = idiv_ceil(p_click_context->one_program_pixel.y, p_click_context->host_xform.scale.t.y);
}

extern void
pixit_point_from_window_point(
    _OutRef_    P_PIXIT_POINT p_pixit_point,
    _InRef_     PC_GDI_POINT p_gdi_point,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    p_pixit_point->x = muldiv64_floor(p_gdi_point->x, p_host_xform->windows.multiplier_of_pixels.x, p_host_xform->windows.divisor_of_pixels.x);
    p_pixit_point->y = muldiv64_floor(p_gdi_point->y, p_host_xform->windows.multiplier_of_pixels.y, p_host_xform->windows.divisor_of_pixels.y);
}

extern void
pixit_rect_from_window_rect(
    _OutRef_    P_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_GDI_RECT p_gdi_rect,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    p_pixit_rect->tl.x = muldiv64_floor(p_gdi_rect->tl.x, p_host_xform->windows.multiplier_of_pixels.x, p_host_xform->windows.divisor_of_pixels.x);
    p_pixit_rect->tl.y = muldiv64_floor(p_gdi_rect->tl.y, p_host_xform->windows.multiplier_of_pixels.y, p_host_xform->windows.divisor_of_pixels.y);
    p_pixit_rect->br.x = muldiv64_ceil( p_gdi_rect->br.x, p_host_xform->windows.multiplier_of_pixels.x, p_host_xform->windows.divisor_of_pixels.x);
    p_pixit_rect->br.y = muldiv64_ceil( p_gdi_rect->br.y, p_host_xform->windows.multiplier_of_pixels.y, p_host_xform->windows.divisor_of_pixels.y);
}

_Check_return_
extern PIXIT
scale_pixit_x(
    _InVal_     PIXIT pixit_x,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    return((pixit_x * p_host_xform->scale.t.x) / p_host_xform->scale.b.x);
}

_Check_return_
extern PIXIT
scale_pixit_y(
    _InVal_     PIXIT pixit_y,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    return((pixit_y * p_host_xform->scale.t.y) / p_host_xform->scale.b.y);
}

extern void
set_host_xform_for_view(
    _OutRef_    P_HOST_XFORM p_host_xform,
    _ViewRef_   P_VIEW p_view,
    _InVal_     BOOL do_x_scale,
    _InVal_     BOOL do_y_scale)
{
    set_host_xform_for_view_common(p_host_xform, p_view, do_x_scale, do_y_scale);

    { /* NB. views pertain only to the screen */
    const HDC hic_display = CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);
    assert(hic_display);
    p_host_xform->windows.pixels_per_inch.x = GetDeviceCaps(hic_display, LOGPIXELSX);
    p_host_xform->windows.pixels_per_inch.y = GetDeviceCaps(hic_display, LOGPIXELSY);
    void_WrapOsBoolChecking(DeleteDC(hic_display));
    } /*block*/

    p_host_xform->windows.pixels_per_metre.x = muldiv64(p_host_xform->windows.pixels_per_inch.x, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);
    p_host_xform->windows.pixels_per_metre.y = muldiv64(p_host_xform->windows.pixels_per_inch.y, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);

    p_host_xform->windows.multiplier_of_pixels.x = PIXITS_PER_METRE * p_host_xform->scale.b.x;
    p_host_xform->windows.multiplier_of_pixels.y = PIXITS_PER_METRE * p_host_xform->scale.b.y;

    p_host_xform->windows.divisor_of_pixels.x = p_host_xform->windows.pixels_per_metre.x * p_host_xform->scale.t.x;
    p_host_xform->windows.divisor_of_pixels.y = p_host_xform->windows.pixels_per_metre.y * p_host_xform->scale.t.y;
}

/******************************************************************************
*
* returns tl of device pixel corresponding to point
*
******************************************************************************/

extern void
window_point_from_pixit_point(
    _OutRef_    P_GDI_POINT p_gdi_point,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    p_gdi_point->x = muldiv64_round_floor(p_pixit_point->x, p_host_xform->windows.divisor_of_pixels.x, p_host_xform->windows.multiplier_of_pixels.x);
    p_gdi_point->y = muldiv64_round_floor(p_pixit_point->y, p_host_xform->windows.divisor_of_pixels.y, p_host_xform->windows.multiplier_of_pixels.y);
}

/******************************************************************************
*
* returns x0i,y0i,x1i,y1i rect corresponding to rect
*
* STATUS_OK iff rect should not be plotted else STATUS_DONE
*
******************************************************************************/

_Check_return_
extern STATUS
window_rect_from_pixit_rect(
    _OutRef_    P_GDI_RECT p_gdi_rect,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    p_gdi_rect->tl.x = muldiv64_round_floor(p_pixit_rect->tl.x, p_host_xform->windows.divisor_of_pixels.x, p_host_xform->windows.multiplier_of_pixels.x);
    p_gdi_rect->tl.y = muldiv64_round_floor(p_pixit_rect->tl.y, p_host_xform->windows.divisor_of_pixels.y, p_host_xform->windows.multiplier_of_pixels.y);
    p_gdi_rect->br.x = muldiv64_round_floor(p_pixit_rect->br.x, p_host_xform->windows.divisor_of_pixels.x, p_host_xform->windows.multiplier_of_pixels.x);
    p_gdi_rect->br.y = muldiv64_round_floor(p_pixit_rect->br.y, p_host_xform->windows.divisor_of_pixels.y, p_host_xform->windows.multiplier_of_pixels.y);

    /* i,i,e,e return! */
    if((p_gdi_rect->tl.x >= p_gdi_rect->br.x) || (p_gdi_rect->tl.y >= p_gdi_rect->br.y))
        return(STATUS_OK);

    return(STATUS_DONE);
}

/* end of windows/ho_xform.c */
