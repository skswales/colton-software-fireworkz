/* riscos/ho_xform.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS January 1993 */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/xp_skelr.h"

/******************************************************************************
*
* returns bl of pixel corresponding to point
*
******************************************************************************/

extern void
gdi_point_from_pixit_point_and_context(
    _OutRef_    P_GDI_POINT p_gdi_point,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT_POINT pixit_point;
    const U32 XEigFactor = p_redraw_context->host_xform.riscos.XEigFactor;
    const U32 YEigFactor = p_redraw_context->host_xform.riscos.YEigFactor;
    S32_POINT divisor; /* scale.b * pixits per pixel */
    S32_POINT pixels;

    pixit_point.x = p_pixit_point->x + p_redraw_context->pixit_origin.x;
    pixit_point.y = p_pixit_point->y + p_redraw_context->pixit_origin.y;

    divisor.x = (p_redraw_context->host_xform.scale.b.x * PIXITS_PER_RISCOS) << XEigFactor;
    divisor.y = (p_redraw_context->host_xform.scale.b.y * PIXITS_PER_RISCOS) << YEigFactor;

    pixels.x = +muldiv64_round_floor(/*+*/pixit_point.x, p_redraw_context->host_xform.scale.t.x, divisor.x);
    pixels.y = -muldiv64_round_floor(/*-*/pixit_point.y, p_redraw_context->host_xform.scale.t.y, divisor.y);

    p_gdi_point->x = (pixels.x << XEigFactor) + p_redraw_context->gdi_org.x;
    p_gdi_point->y = (pixels.y << YEigFactor) + p_redraw_context->gdi_org.y;
}

/******************************************************************************
*
* returns tl.xi,br.yi,br.xe,tl.ye box corresponding to rect
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
    const U32 XEigFactor = p_redraw_context->host_xform.riscos.XEigFactor;
    const U32 YEigFactor = p_redraw_context->host_xform.riscos.YEigFactor;
    S32_POINT divisor; /* scale.b * pixits per pixel */
    S32_RECT pixels;

    pixit_rect.tl.x = p_pixit_rect->tl.x + p_redraw_context->pixit_origin.x;
    pixit_rect.tl.y = p_pixit_rect->tl.y + p_redraw_context->pixit_origin.y;
    pixit_rect.br.x = p_pixit_rect->br.x + p_redraw_context->pixit_origin.x;
    pixit_rect.br.y = p_pixit_rect->br.y + p_redraw_context->pixit_origin.y;

    divisor.x = (p_redraw_context->host_xform.scale.b.x * PIXITS_PER_RISCOS) << XEigFactor;
    divisor.y = (p_redraw_context->host_xform.scale.b.y * PIXITS_PER_RISCOS) << YEigFactor;

    pixels.tl.x = +muldiv64_round_floor(pixit_rect.tl.x, p_redraw_context->host_xform.scale.t.x, divisor.x);
    pixels.tl.y = -muldiv64_round_floor(pixit_rect.tl.y, p_redraw_context->host_xform.scale.t.y, divisor.y);
    pixels.br.x = +muldiv64_round_floor(pixit_rect.br.x, p_redraw_context->host_xform.scale.t.x, divisor.x);
    pixels.br.y = -muldiv64_round_floor(pixit_rect.br.y, p_redraw_context->host_xform.scale.t.y, divisor.y);

    p_gdi_rect->tl.x = (pixels.tl.x << XEigFactor) + p_redraw_context->gdi_org.x;
    p_gdi_rect->tl.y = (pixels.tl.y << YEigFactor) + p_redraw_context->gdi_org.y;
    p_gdi_rect->br.x = (pixels.br.x << XEigFactor) + p_redraw_context->gdi_org.x;
    p_gdi_rect->br.y = (pixels.br.y << YEigFactor) + p_redraw_context->gdi_org.y;

    if((p_gdi_rect->tl.x >= p_gdi_rect->br.x) || (p_gdi_rect->br.y >= p_gdi_rect->tl.y))
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
    if(p_gdi_rect->tl.y > T5_GDI_MAX_Y) p_gdi_rect->tl.y = T5_GDI_MAX_Y; /* top */
    if(p_gdi_rect->br.x > T5_GDI_MAX_X) p_gdi_rect->br.x = T5_GDI_MAX_X; /* right */
    if(p_gdi_rect->br.y < T5_GDI_MIN_Y) p_gdi_rect->br.y = T5_GDI_MIN_Y; /* bottom */

    if((p_gdi_rect->tl.x >= p_gdi_rect->br.x) || (p_gdi_rect->br.y >= p_gdi_rect->tl.y))
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
    S32_POINT divisor;
    S32_RECT draw_units;

    pixit_rect.tl.x = p_pixit_rect->tl.x + p_redraw_context->pixit_origin.x;
    pixit_rect.tl.y = p_pixit_rect->tl.y + p_redraw_context->pixit_origin.y;
    pixit_rect.br.x = p_pixit_rect->br.x + p_redraw_context->pixit_origin.x;
    pixit_rect.br.y = p_pixit_rect->br.y + p_redraw_context->pixit_origin.y;

    divisor.x = (PIXITS_PER_RISCOS * p_redraw_context->host_xform.scale.b.x) * p_redraw_context->host_xform.riscos.dx;
    divisor.y = (PIXITS_PER_RISCOS * p_redraw_context->host_xform.scale.b.y) * p_redraw_context->host_xform.riscos.dy;

    draw_units.tl.x = +muldiv64(pixit_rect.tl.x, (p_redraw_context->host_xform.riscos.dx << 8) * p_redraw_context->host_xform.scale.t.x, divisor.x);
    draw_units.tl.y = -muldiv64(pixit_rect.tl.y, (p_redraw_context->host_xform.riscos.dy << 8) * p_redraw_context->host_xform.scale.t.y, divisor.y);
    draw_units.br.x = +muldiv64(pixit_rect.br.x, (p_redraw_context->host_xform.riscos.dx << 8) * p_redraw_context->host_xform.scale.t.x, divisor.x);
    draw_units.br.y = -muldiv64(pixit_rect.br.y, (p_redraw_context->host_xform.riscos.dy << 8) * p_redraw_context->host_xform.scale.t.y, divisor.y);

    p_gdi_rect->tl.x = draw_units.tl.x + (p_redraw_context->gdi_org.x << 8);
    p_gdi_rect->tl.y = draw_units.tl.y + (p_redraw_context->gdi_org.y << 8);
    p_gdi_rect->br.x = draw_units.br.x + (p_redraw_context->gdi_org.x << 8);
    p_gdi_rect->br.y = draw_units.br.y + (p_redraw_context->gdi_org.y << 8);

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
    p_host_xform->riscos.dx = host_modevar_cache_current.dx;
    p_host_xform->riscos.dy = host_modevar_cache_current.dy;

    p_host_xform->riscos.XEigFactor = host_modevar_cache_current.XEigFactor;
    p_host_xform->riscos.YEigFactor = host_modevar_cache_current.YEigFactor;

    p_redraw_context->host_xform = *p_host_xform;
}

extern void
host_redraw_context_fillin(
    _InoutRef_  P_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT_RECT pixit_rect;
    GDI_RECT gdi_rect;

    /* users of one_pixel please note that it is only an approximation and MUST BE >= real pixel size */
    p_redraw_context->one_real_pixel.x = p_redraw_context->host_xform.riscos.dx * (PIXITS_PER_RISCOS * p_redraw_context->host_xform.scale.b.x);
    p_redraw_context->one_real_pixel.x = idiv_ceil(p_redraw_context->one_real_pixel.x, p_redraw_context->host_xform.scale.t.x);
    p_redraw_context->one_real_pixel.y = p_redraw_context->host_xform.riscos.dy * (PIXITS_PER_RISCOS * p_redraw_context->host_xform.scale.b.y);
    p_redraw_context->one_real_pixel.y = idiv_ceil(p_redraw_context->one_real_pixel.y, p_redraw_context->host_xform.scale.t.y);

    p_redraw_context->one_program_pixel.x = p_redraw_context->host_xform.scale.b.x * PIXITS_PER_PROGRAM_PIXEL_X;
    p_redraw_context->one_program_pixel.x = idiv_ceil(p_redraw_context->one_program_pixel.x, p_redraw_context->host_xform.scale.t.x);
    p_redraw_context->one_program_pixel.y = p_redraw_context->host_xform.scale.b.y * PIXITS_PER_PROGRAM_PIXEL_Y;
    p_redraw_context->one_program_pixel.y = idiv_ceil(p_redraw_context->one_program_pixel.y, p_redraw_context->host_xform.scale.t.y);

    /* standard (half-width) lines */

    p_redraw_context->line_width.x = p_redraw_context->border_width.x / 2;
    p_redraw_context->line_width.y = p_redraw_context->border_width.y / 2;

    pixit_rect.tl.x = pixit_rect.tl.y = -1;
    pixit_rect.br.x = pixit_rect.tl.x + p_redraw_context->line_width.x;
    pixit_rect.br.y = pixit_rect.tl.y + p_redraw_context->line_width.y;

    status_consume(gdi_rect_from_pixit_rect_and_context(&gdi_rect, &pixit_rect, p_redraw_context)); /* origin,org semi-irrelevant for diffs */

    p_redraw_context->line_width_eff.x = ((U32) (gdi_rect.br.x - gdi_rect.tl.x) > p_redraw_context->host_xform.riscos.dx)
                                       ? p_redraw_context->line_width.x
                                       : MIN(p_redraw_context->border_width.x, p_redraw_context->one_real_pixel.x);
    p_redraw_context->line_width_eff.y = ((U32) (gdi_rect.tl.y - gdi_rect.br.y) > p_redraw_context->host_xform.riscos.dy)
                                       ? p_redraw_context->line_width.y
                                       : MIN(p_redraw_context->border_width.y, p_redraw_context->one_real_pixel.y);

    /* thin lines */

    p_redraw_context->thin_width.x = 1; /* pixits */
    p_redraw_context->thin_width.y = 1;

    pixit_rect.tl.x = pixit_rect.tl.y = -1;
    pixit_rect.br.x = pixit_rect.tl.x + p_redraw_context->thin_width.x;
    pixit_rect.br.y = pixit_rect.tl.y + p_redraw_context->thin_width.y;

    status_consume(gdi_rect_from_pixit_rect_and_context(&gdi_rect, &pixit_rect, p_redraw_context)); /* origin,org semi-irrelevant for diffs */

    p_redraw_context->thin_width_eff.x = ((U32) (gdi_rect.br.x - gdi_rect.tl.x) > p_redraw_context->host_xform.riscos.dx)
                                       ? p_redraw_context->thin_width.x
                                       : MIN(p_redraw_context->border_width.x, p_redraw_context->one_real_pixel.x);
    p_redraw_context->thin_width_eff.y = ((U32) (gdi_rect.tl.y - gdi_rect.br.y) > p_redraw_context->host_xform.riscos.dy)
                                       ? p_redraw_context->thin_width.y
                                       : MIN(p_redraw_context->border_width.y, p_redraw_context->one_real_pixel.y);
}

static void
host_click_context_set_host_xform(
    _InoutRef_  P_CLICK_CONTEXT p_click_context,
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/)
{
    /* NB. clicks are in views and so pertain only to the screen */
    p_host_xform->riscos.dx = host_modevar_cache_current.dx;
    p_host_xform->riscos.dy = host_modevar_cache_current.dy;

    p_host_xform->riscos.XEigFactor = host_modevar_cache_current.XEigFactor;
    p_host_xform->riscos.YEigFactor = host_modevar_cache_current.YEigFactor;

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
    p_click_context->one_real_pixel.x = p_click_context->host_xform.riscos.dx * (PIXITS_PER_RISCOS * p_click_context->host_xform.scale.b.x);
    p_click_context->one_real_pixel.x = idiv_ceil(p_click_context->one_real_pixel.x, p_click_context->host_xform.scale.t.x);
    p_click_context->one_real_pixel.y = p_click_context->host_xform.riscos.dy * (PIXITS_PER_RISCOS * p_click_context->host_xform.scale.b.y);
    p_click_context->one_real_pixel.y = idiv_ceil(p_click_context->one_real_pixel.y, p_click_context->host_xform.scale.t.y);

    p_click_context->one_program_pixel.x = p_click_context->host_xform.scale.b.x * PIXITS_PER_PROGRAM_PIXEL_X;
    p_click_context->one_program_pixel.x = idiv_ceil(p_click_context->one_program_pixel.x, p_click_context->host_xform.scale.t.x);
    p_click_context->one_program_pixel.y = p_click_context->host_xform.scale.b.y * PIXITS_PER_PROGRAM_PIXEL_Y;
    p_click_context->one_program_pixel.y = idiv_ceil(p_click_context->one_program_pixel.y, p_click_context->host_xform.scale.t.y);
}

_Check_return_
extern GDI_COORD
os_unit_from_pixit_x(
    _InVal_     PIXIT pixit_x,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    return((pixit_x * p_host_xform->scale.t.x) / (PIXITS_PER_RISCOS * p_host_xform->scale.b.x));
}

_Check_return_
extern GDI_COORD
os_unit_from_pixit_y(
    _InVal_     PIXIT pixit_y,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    return((pixit_y * p_host_xform->scale.t.y) / (PIXITS_PER_RISCOS * p_host_xform->scale.b.y));
}

extern void
pixit_point_from_window_point(
    _OutRef_    P_PIXIT_POINT p_pixit_point,
    _InRef_     PC_GDI_POINT p_gdi_point,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    GDI_POINT gdi_point;
    PIXIT_POINT multiplier;

    multiplier.x = (PIXITS_PER_RISCOS * p_host_xform->scale.b.x);
    multiplier.y = (PIXITS_PER_RISCOS * p_host_xform->scale.b.y);

    /* yet more apparent illogicality */
    gdi_point.x = muldiv64_ceil( p_gdi_point->x, multiplier.x, p_host_xform->scale.t.x);
    gdi_point.y = muldiv64_ceil( p_gdi_point->y, multiplier.y, p_host_xform->scale.t.y);

    p_pixit_point->x = + gdi_point.x;
    p_pixit_point->y = - gdi_point.y;
}

extern void
pixit_rect_from_screen_rect_and_context(
    _OutRef_    P_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_GDI_RECT p_gdi_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    GDI_RECT gdi_rect = *p_gdi_rect;

    gdi_rect.tl.x -= p_redraw_context->gdi_org.x;
    gdi_rect.tl.y -= p_redraw_context->gdi_org.y;
    gdi_rect.br.x -= p_redraw_context->gdi_org.x;
    gdi_rect.br.y -= p_redraw_context->gdi_org.y;

    pixit_rect_from_window_rect(p_pixit_rect, &gdi_rect, &p_redraw_context->host_xform);
}

extern void
pixit_rect_from_window_rect(
    _OutRef_    P_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_GDI_RECT p_gdi_rect,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    GDI_RECT gdi_rect = *p_gdi_rect;
    PIXIT_POINT multiplier;

    multiplier.x = PIXITS_PER_RISCOS * p_host_xform->scale.b.x;
    multiplier.y = PIXITS_PER_RISCOS * p_host_xform->scale.b.y;

    p_pixit_rect->tl.x = muldiv64_floor(+gdi_rect.tl.x, multiplier.x, p_host_xform->scale.t.x);
    p_pixit_rect->tl.y = muldiv64_floor(-gdi_rect.tl.y, multiplier.y, p_host_xform->scale.t.y);
    p_pixit_rect->br.x = muldiv64_ceil( +gdi_rect.br.x, multiplier.x, p_host_xform->scale.t.x);
    p_pixit_rect->br.y = muldiv64_ceil( -gdi_rect.br.y, multiplier.y, p_host_xform->scale.t.y);

#if 0 /* old code */
    multiplier.x = PIXITS_PER_RISCOS * p_host_xform->scale.b.x;
    multiplier.y = PIXITS_PER_RISCOS * p_host_xform->scale.b.y;

    box.x0 = p_gr_box->x0 - p_host_xform->d.x / 2;
    box.y0 = p_gr_box->y0 - p_host_xform->d.y / 2;
    box.x1 = p_gr_box->x1 + p_host_xform->d.x / 2;
    box.y1 = p_gr_box->y1 + p_host_xform->d.y / 2;

    /* yet more apparent illogicality */
    box.x0 = muldiv64_ceil( box.x0, multiplier.x, p_host_xform->scale.t.x);
    box.y0 = muldiv64_floor(box.y0, multiplier.y, p_host_xform->scale.t.y);
    box.x1 = muldiv64_ceil( box.x1, multiplier.x, p_host_xform->scale.t.x);
    box.y1 = muldiv64_floor(box.y1, multiplier.y, p_host_xform->scale.t.y);

    p_pixit_rect->tl.x = + box.x0;
    p_pixit_rect->br.y = - box.y0;
    p_pixit_rect->br.x = + box.x1;
    p_pixit_rect->tl.y = - box.y1;
#endif
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

    p_host_xform->riscos.dx = host_modevar_cache_current.dx;
    p_host_xform->riscos.dy = host_modevar_cache_current.dy;

    p_host_xform->riscos.XEigFactor = host_modevar_cache_current.XEigFactor;
    p_host_xform->riscos.YEigFactor = host_modevar_cache_current.YEigFactor;
}

/******************************************************************************
*
* returns bl of pixel corresponding to point
*
******************************************************************************/

extern void
window_point_from_pixit_point(
    _OutRef_    P_GDI_POINT p_gdi_point,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    const U32 XEigFactor = p_host_xform->riscos.XEigFactor;
    const U32 YEigFactor = p_host_xform->riscos.YEigFactor;
    S32_POINT divisor; /* scale.b * pixits per pixel */
    S32_POINT pixels;

    divisor.x = (p_host_xform->scale.b.x * PIXITS_PER_RISCOS) << XEigFactor;
    divisor.y = (p_host_xform->scale.b.y * PIXITS_PER_RISCOS) << YEigFactor;

    pixels.x = +muldiv64_round_floor(/*+*/p_pixit_point->x, p_host_xform->scale.t.x, divisor.x);
    pixels.y = -muldiv64_round_floor(/*-*/p_pixit_point->y, p_host_xform->scale.t.y, divisor.y);

    p_gdi_point->x = pixels.x << XEigFactor;
    p_gdi_point->y = pixels.y << YEigFactor;
}

/******************************************************************************
*
* returns tl.xi,br.yi,br.xe,tl.ye box corresponding to rect
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
    const U32 XEigFactor = p_host_xform->riscos.XEigFactor;
    const U32 YEigFactor = p_host_xform->riscos.YEigFactor;
    S32_POINT divisor; /* scale.b * pixits per pixel */
    S32_RECT pixels;

    divisor.x = (p_host_xform->scale.b.x * PIXITS_PER_RISCOS) << XEigFactor;
    divisor.y = (p_host_xform->scale.b.y * PIXITS_PER_RISCOS) << YEigFactor;

    pixels.tl.x = +muldiv64_round_floor(p_pixit_rect->tl.x, p_host_xform->scale.t.x, divisor.x);
    pixels.tl.y = -muldiv64_round_floor(p_pixit_rect->tl.y, p_host_xform->scale.t.y, divisor.y);
    pixels.br.x = +muldiv64_round_floor(p_pixit_rect->br.x, p_host_xform->scale.t.x, divisor.x);
    pixels.br.y = -muldiv64_round_floor(p_pixit_rect->br.y, p_host_xform->scale.t.y, divisor.y);

    p_gdi_rect->tl.x = pixels.tl.x << XEigFactor;
    p_gdi_rect->tl.y = pixels.tl.y << YEigFactor;
    p_gdi_rect->br.x = pixels.br.x << XEigFactor;
    p_gdi_rect->br.y = pixels.br.y << YEigFactor;

    if((p_gdi_rect->tl.x >= p_gdi_rect->br.x) || (p_gdi_rect->br.y >= p_gdi_rect->tl.y))
        return(STATUS_OK);

    return(STATUS_DONE);
}

#endif /* RISCOS */

/* end of riscos/ho_xform.c */
