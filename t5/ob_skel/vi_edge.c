/* vi_edge.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef         __vi_edge_h
#include "ob_skel/vi_edge.h"
#endif

#if RISCOS
#define EXPOSE_RISCOS_SWIS 1
#include "ob_skel/xp_skelr.h"
#endif

static STYLE_SELECTOR _border_style_selector; /* const after T5_MSG_IC__STARTUP */

_Check_return_
_Ret_valid_
static inline PC_STYLE_SELECTOR
p_border_style_selector(void)
{
    return(&_border_style_selector);
}

static void
border_style_startup(void)
{
    style_selector_copy(&_border_style_selector, &style_selector_font_spec);
    style_selector_bit_set(&_border_style_selector, STYLE_SW_PS_RGB_BACK);
    style_selector_bit_set(&_border_style_selector, STYLE_SW_PS_RGB_GRID_LEFT);
}

static BOOL  recache_border_horz_style = TRUE;
static BOOL  recache_border_vert_style = TRUE;

static STYLE _border_horz_style;
static STYLE _border_vert_style;

_Ret_valid_
static P_STYLE
border_style_cache_do(
    _DocuRef_   PC_DOCU p_docu_config,
    _OutRef_    P_STYLE p_style_out,
    _In_z_      PCTSTR p_specific_style_name)
{
    const PC_STYLE_SELECTOR p_desired_style_selector = p_border_style_selector();
    STYLE_HANDLE style_handle;
    STYLE style;

    CHECKING_ONLY(zero_struct(style)); /* sure makes it easier to debug! */
    style_init(&style);

    /* look up the most specific style first */
    if(0 != (style_handle = style_handle_from_name(p_docu_config, p_specific_style_name)))
        style_struct_from_handle(p_docu_config, &style, style_handle, p_desired_style_selector);

    if(0 != style_selector_compare(p_desired_style_selector, &style.selector))
    {   /* fill in anything missing from the more generic style */
        STYLE_SELECTOR style_selector_ensure;

        void_style_selector_bic(&style_selector_ensure, p_desired_style_selector, &style.selector);

        if(0 != (style_handle = style_handle_from_name(p_docu_config, STYLE_NAME_UI_BORDER)))
            style_struct_from_handle(p_docu_config, &style, style_handle, &style_selector_ensure);

        if(0 != style_selector_compare(p_desired_style_selector, &style.selector))
        {   /* fill in anything still missing from the base UI style defaults */
            void_style_selector_bic(&style_selector_ensure, p_desired_style_selector, &style.selector);

            style_copy_defaults(p_docu_config, &style, &style_selector_ensure);
        }
    }

    /* all callers need fully populated font_spec */
    font_spec_from_ui_style(&style.font_spec, &style);

    /* style is now populated with everything we need */
    *p_style_out = style;

    return(p_style_out);
}

_Check_return_
_Ret_valid_
static PC_STYLE
border_horz_style_cache(
    _InoutRef_  P_STYLE p_style_ensured)
{
    if(!recache_border_horz_style)
        return(p_style_ensured);

    recache_border_horz_style = FALSE;

    return(border_style_cache_do(p_docu_from_config(), p_style_ensured, STYLE_NAME_UI_BORDER_HORZ));
}

_Check_return_
_Ret_valid_
static PC_STYLE
border_vert_style_cache(
    _InoutRef_  P_STYLE p_style_ensured)
{
    if(!recache_border_vert_style)
        return(p_style_ensured);

    recache_border_vert_style = FALSE;

    return(border_style_cache_do(p_docu_from_config(), p_style_ensured, STYLE_NAME_UI_BORDER_VERT));
}

_Check_return_
_Ret_valid_
extern PC_STYLE
p_style_for_border_horz(void)
{
    return(border_horz_style_cache(&_border_horz_style));
}

_Check_return_
_Ret_valid_
extern PC_STYLE
p_style_for_border_vert(void)
{
    return(border_vert_style_cache(&_border_vert_style));
}

static PIXIT g_border_horz_pixit_height = 0;
static PIXIT g_border_vert_pixit_width  = 0;

static void
border_horz_cache_height(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    const PC_STYLE p_border_style = p_style_for_border_horz();
    HOST_FONT host_font_redraw = HOST_FONT_NONE;
    STATUS status;
    PIXIT col_repr_height = 0;
    PIXIT col_repr_descent = 0;
    PIXIT border_horz_tm = BORDER_HORZ_TM_MIN;
    PIXIT border_horz_bm = BORDER_HORZ_BM_MIN;

    g_border_horz_pixit_height = BORDER_HORZ_PIXIT_HEIGHT;

    if(status_ok(status = fonty_handle_from_font_spec(&p_border_style->font_spec, FALSE)))
    {
        const FONTY_HANDLE fonty_handle = (FONTY_HANDLE) status;

        host_font_redraw = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, FALSE);
    }

    if(HOST_FONT_NONE != host_font_redraw)
    {
#if RISCOS
        const PIXIT pixits_per_riscos_dy = PIXITS_PER_RISCOS << p_redraw_context->host_xform.riscos.YEigFactor;
        _kernel_swi_regs rs;
        rs.r[0] = host_font_redraw;
        rs.r[1] = 'A';
        rs.r[2] = 0;
        if(NULL == _kernel_swi(Font_CharBBox, &rs, &rs))
        {
            col_repr_height  = pixits_from_millipoints_ceil(abs(rs.r[4]));
            col_repr_descent = pixits_from_millipoints_ceil(abs(rs.r[2]));
        }
        else
        {
            const PIXIT base_line = p_border_style->font_spec.size_y * 3 / 4;
            col_repr_height  = base_line;
            col_repr_descent = p_border_style->font_spec.size_y - col_repr_height;
        }

        col_repr_height   = idiv_ceil_u(col_repr_height,  pixits_per_riscos_dy);
        col_repr_height  *= pixits_per_riscos_dy;

        col_repr_descent  = idiv_ceil_u(col_repr_descent, pixits_per_riscos_dy);
        col_repr_descent *= pixits_per_riscos_dy;
#elif WINDOWS
        { /* dpi-dependent pixels */
        const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
        HFONT h_font_old = SelectFont(hdc, host_font_redraw);
        TEXTMETRIC textmetric;
        WrapOsBoolChecking(GetTextMetrics(hdc, &textmetric));
        consume(HFONT, SelectFont(hdc, h_font_old));

        col_repr_height  = textmetric.tmAscent - textmetric.tmInternalLeading;
        col_repr_descent = textmetric.tmDescent;
        } /*block*/

        { /* convert to dpi-independent pixits */ /* DPI-aware */
        GDI_SIZE PixelsPerInch;
        host_get_pixel_size(NULL /*screen*/, &PixelsPerInch.cx, &PixelsPerInch.cy); /* Get current pixel size for the screen e.g. 96 or 120 */

        col_repr_height  = idiv_ceil_u(col_repr_height  * PIXITS_PER_INCH, PixelsPerInch.cy);
        col_repr_descent = idiv_ceil_u(col_repr_descent * PIXITS_PER_INCH, PixelsPerInch.cy);
        } /*block*/
#endif /* OS */
    }

    if(border_horz_bm < col_repr_descent)
    {
        border_horz_bm = col_repr_descent;
        border_horz_bm += PIXITS_PER_PROGRAM_PIXEL_Y;

        border_horz_tm = border_horz_bm;
    }

    g_border_horz_pixit_height = border_horz_tm + col_repr_height + border_horz_bm;
}

static void
border_vert_cache_width(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
#if RISCOS
    ROW last_row = n_rows(p_redraw_context->p_docu);
    g_border_vert_pixit_width = BORDER_VERT_PIXIT_MIN_WIDTH;
    if(last_row >= 10000000)
        g_border_vert_pixit_width = (g_border_vert_pixit_width * 10) / 8;
    else if(last_row >= 1000000)
        g_border_vert_pixit_width = (g_border_vert_pixit_width *  9) / 8;
#elif WINDOWS
    const PC_STYLE p_border_style = p_style_for_border_vert();
    HOST_FONT host_font_redraw = HOST_FONT_NONE;
    STATUS status;
    PIXIT four_digits_width = 0;

    g_border_vert_pixit_width = BORDER_VERT_PIXIT_MIN_WIDTH;

    if(status_ok(status = fonty_handle_from_font_spec(&p_border_style->font_spec, FALSE)))
    {
        const FONTY_HANDLE fonty_handle = (FONTY_HANDLE) status;

        host_font_redraw = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, FALSE);
    }

    if(HOST_FONT_NONE != host_font_redraw)
    {
        { /* dpi-dependent pixels */
        const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
        HFONT h_font_old = SelectFont(hdc, host_font_redraw);
        SIZE size;
        status_consume(GetTextExtentPoint32(hdc, TEXT("01234567"), 4*2, &size));
        consume(HFONT, SelectFont(hdc, h_font_old));

        four_digits_width = idiv_ceil_u(size.cx, 2); /* average width of four digits */
        } /*block*/

        { /* convert to dpi-independent pixits */ /* DPI-aware */
        GDI_SIZE PixelsPerInch;
        host_get_pixel_size(NULL /*screen*/, &PixelsPerInch.cx, &PixelsPerInch.cy); /* Get current pixel size for the screen e.g. 96 or 120 */

        four_digits_width = idiv_ceil_u(four_digits_width  * PIXITS_PER_INCH, PixelsPerInch.cy);
        } /*block*/

        g_border_vert_pixit_width = MAX(four_digits_width + 2 * (4 * PIXITS_PER_PROGRAM_PIXEL_X) /*lm,rm*/, BORDER_VERT_PIXIT_MIN_WIDTH);
    }
#endif /* OS */
}

static STYLE_SELECTOR _ruler_style_selector; /* const after T5_MSG_IC__STARTUP */

_Check_return_
_Ret_valid_
static inline PC_STYLE_SELECTOR
p_ruler_style_selector(void)
{
    return(&_ruler_style_selector);
}

static void
ruler_style_startup(void)
{
    style_selector_copy(&_ruler_style_selector, &style_selector_font_spec);
    style_selector_bit_set(&_ruler_style_selector, STYLE_SW_PS_RGB_BACK);
    style_selector_bit_set(&_ruler_style_selector, STYLE_SW_PS_RGB_GRID_LEFT);
}

static BOOL  recache_ruler_horz_style = TRUE;
static BOOL  recache_ruler_vert_style = TRUE;

static STYLE _ruler_horz_style;
static STYLE _ruler_vert_style;

_Ret_valid_
static P_STYLE
ruler_style_cache_do(
    _DocuRef_   PC_DOCU p_docu_config,
    _OutRef_    P_STYLE p_style_out,
    _In_z_      PCTSTR p_specific_style_name)
{
    const PC_STYLE_SELECTOR p_desired_style_selector = p_ruler_style_selector();
    STYLE_HANDLE style_handle;
    STYLE style;

    CHECKING_ONLY(zero_struct(style)); /* sure makes it easier to debug! */
    style_init(&style);

    /* look up the most specific style first */
    if(0 != (style_handle = style_handle_from_name(p_docu_config, p_specific_style_name)))
        style_struct_from_handle(p_docu_config, &style, style_handle, p_desired_style_selector);

    if(0 != style_selector_compare(p_desired_style_selector, &style.selector))
    {   /* fill in anything missing from the more generic style */
        STYLE_SELECTOR style_selector_ensure;

        void_style_selector_bic(&style_selector_ensure, p_desired_style_selector, &style.selector);

        if(0 != (style_handle = style_handle_from_name(p_docu_config, STYLE_NAME_UI_RULER)))
            style_struct_from_handle(p_docu_config, &style, style_handle, &style_selector_ensure);

        if(0 != style_selector_compare(p_desired_style_selector, &style.selector))
        {   /* fill in anything still missing from the base UI style defaults */
            void_style_selector_bic(&style_selector_ensure, p_desired_style_selector, &style.selector);

            style_copy_defaults(p_docu_config, &style, &style_selector_ensure);
        }
    }

    /* all callers need fully populated font_spec */
    font_spec_from_ui_style(&style.font_spec, &style);

    /* style is now populated with everything we need */
    *p_style_out = style;

    return(p_style_out);
}

_Check_return_
_Ret_valid_
static PC_STYLE
ruler_horz_style_cache(
    _InoutRef_  P_STYLE p_style_ensured)
{
    if(!recache_ruler_horz_style)
        return(p_style_ensured);

    recache_ruler_horz_style = FALSE;

    return(ruler_style_cache_do(p_docu_from_config(), p_style_ensured, STYLE_NAME_UI_RULER_HORZ));
}

_Check_return_
_Ret_valid_
static PC_STYLE
ruler_vert_style_cache(
    _InoutRef_  P_STYLE p_style_ensured)
{
    if(!recache_ruler_vert_style)
        return(p_style_ensured);

    recache_ruler_vert_style = FALSE;

    return(ruler_style_cache_do(p_docu_from_config(), p_style_ensured, STYLE_NAME_UI_RULER_VERT));
}

_Check_return_
_Ret_valid_
extern PC_STYLE
p_style_for_ruler_horz(void)
{
    return(ruler_horz_style_cache(&_ruler_horz_style));
}

_Check_return_
_Ret_valid_
extern PC_STYLE
p_style_for_ruler_vert(void)
{
    return(ruler_vert_style_cache(&_ruler_vert_style));
}

static PIXIT g_ruler_horz_pixit_height = 0;
static PIXIT g_ruler_vert_pixit_width  = 0;

static void
ruler_horz_cache_height(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT ruler_horz_marker_y;
    const PC_STYLE p_ruler_style = p_style_for_ruler_horz();
    PIXIT_POINT centre;
    PIXIT_POINT init_figure_point;
    HOST_FONT host_font_redraw = HOST_FONT_NONE;
    STATUS status;
    PIXIT base_line = 0;
    PIXIT digits_height = 0;

    g_ruler_horz_pixit_height = RULER_HORZ_PIXIT_HEIGHT;

    if(status_ok(status = fonty_handle_from_font_spec(&p_ruler_style->font_spec, FALSE)))
    {
        const FONTY_HANDLE fonty_handle = (FONTY_HANDLE) status;

        host_font_redraw = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, FALSE);
    }

    if(HOST_FONT_NONE != host_font_redraw)
    {
#if RISCOS
        const PIXIT pixits_per_riscos_dy = PIXITS_PER_RISCOS << p_redraw_context->host_xform.riscos.YEigFactor;
        _kernel_swi_regs rs;
        rs.r[0] = host_font_redraw;
        rs.r[1] = '0';
        rs.r[2] = 0;
        if(NULL == _kernel_swi(Font_CharBBox, &rs, &rs))
        {
            digits_height = pixits_from_millipoints_ceil(abs(rs.r[4]));
            base_line = digits_height;
        }
        else
        {
            base_line = p_ruler_style->font_spec.size_y * 3 / 4;
            digits_height = base_line;
        }

        digits_height = idiv_ceil_u(digits_height, pixits_per_riscos_dy);
        digits_height *= pixits_per_riscos_dy;
#elif WINDOWS
        { /* dpi-dependent pixels */
        const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
        HFONT h_font_old = SelectFont(hdc, host_font_redraw);
        TEXTMETRIC textmetric;
        WrapOsBoolChecking(GetTextMetrics(hdc, &textmetric));
        consume(HFONT, SelectFont(hdc, h_font_old));

        digits_height = textmetric.tmAscent - textmetric.tmInternalLeading;
        } /*block*/

        { /* convert to dpi-independent pixits */ /* DPI-aware */
        GDI_SIZE PixelsPerInch;
        host_get_pixel_size(NULL /*screen*/, NULL /*cx*/, &PixelsPerInch.cy); /* Get current pixel size for the screen e.g. 96 or 120 */

        digits_height = idiv_ceil_u(digits_height * PIXITS_PER_INCH, PixelsPerInch.cy);
        } /*block*/

        base_line = digits_height;
#endif /* OS */
    }

    init_figure_point.y = RULER_HORZ_SCALE_TOP_Y;

    init_figure_point.y += base_line;

    centre.y = init_figure_point.y - (base_line / 2);

    /* make sure we have room for the markers, recentre if needed */
    if(centre.y - (4 * PIXITS_PER_PROGRAM_PIXEL_Y) > 0)
        ruler_horz_marker_y = centre.y - (4 * PIXITS_PER_PROGRAM_PIXEL_Y);
    else
    {
        ruler_horz_marker_y = 0;
        centre.y = (4 * PIXITS_PER_PROGRAM_PIXEL_Y);
    }

    g_ruler_horz_pixit_height = ruler_horz_marker_y /*tm*/ + ( 8 * PIXITS_PER_PROGRAM_PIXEL_Y) + ruler_horz_marker_y /*bm*/;
}

static void
ruler_vert_cache_width(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT ruler_vert_marker_x;
    const PC_STYLE p_ruler_style = p_style_for_ruler_vert();
    PIXIT_POINT centre;
    PIXIT_POINT init_figure_point;
    HOST_FONT host_font_redraw = HOST_FONT_NONE;
    STATUS status;
    PIXIT two_digits_width = 0;

    g_ruler_vert_pixit_width  = RULER_VERT_PIXIT_WIDTH;

    if(status_ok(status = fonty_handle_from_font_spec(&p_ruler_style->font_spec, FALSE)))
    {
        const FONTY_HANDLE fonty_handle = (FONTY_HANDLE) status;

        host_font_redraw = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, FALSE);
    }

    if(HOST_FONT_NONE != host_font_redraw)
    {
#if RISCOS
        _kernel_swi_regs rs;
        rs.r[0] = host_font_redraw;
        rs.r[1] = '0';
        rs.r[2] = 0;
        if(NULL == _kernel_swi(Font_CharBBox, &rs, &rs))
        {
            two_digits_width = pixits_from_millipoints_ceil(abs(rs.r[3])) * 2;
        }
        else
        {
            two_digits_width = 2 * 16 * PIXITS_PER_RISCOS;
        }
#elif WINDOWS
        { /* dpi-dependent pixels */
        const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
        HFONT h_font_old = SelectFont(hdc, host_font_redraw);
        SIZE size;
        status_consume(GetTextExtentPoint32(hdc, TEXT("0123456789"), 10, &size));
        consume(HFONT, SelectFont(hdc, h_font_old));

        two_digits_width = idiv_ceil_u(size.cx, 5); /*(10/2)*/ /* average width of two digits */
        } /*block*/

        { /* convert to dpi-independent pixits */ /* DPI-aware */
        GDI_SIZE PixelsPerInch;
        host_get_pixel_size(NULL /*screen*/, NULL /*cx*/, &PixelsPerInch.cy); /* Get current pixel size for the screen e.g. 96 or 120 */

        two_digits_width = idiv_ceil_u(two_digits_width  * PIXITS_PER_INCH, PixelsPerInch.cy);
        } /*block*/
#endif /* OS */
    }

    {
    const PIXIT digits_half_width = two_digits_width / 2;

    init_figure_point.x = RULER_VERT_SCALE_LEFT_X + digits_half_width;

    centre.x = init_figure_point.x;

    /* make sure we have room for the markers, recentre if needed */
    if(centre.x - (8 * PIXITS_PER_PROGRAM_PIXEL_X) > 0)
        ruler_vert_marker_x = centre.x - (8 * PIXITS_PER_PROGRAM_PIXEL_X);
    else
    {
        ruler_vert_marker_x = 0;
        centre.x = (8 * PIXITS_PER_PROGRAM_PIXEL_X);
    }
    } /*block*/

    g_ruler_vert_pixit_width  = ruler_vert_marker_x /*lm*/ + (16 * PIXITS_PER_PROGRAM_PIXEL_X) + ruler_vert_marker_x /*lm*/;
}

static void
view_edge_windows_cache_size(
    _ViewRef_   P_VIEW p_view)
{
    const P_DOCU p_docu = p_docu_from_docno(p_view->docno);
    REDRAW_CONTEXT_CACHE redraw_context_cache = { NULL };
    REDRAW_CONTEXT redraw_context;
    const P_REDRAW_CONTEXT p_redraw_context = &redraw_context;
#if WINDOWS
    const HWND hwnd = NULL; /* screen */
    const HDC hdc = GetDC(hwnd); /* only used here as an IC */
#endif

    zero_struct_ptr_fn(p_redraw_context);

    p_redraw_context->p_redraw_context_cache = &redraw_context_cache;

    p_redraw_context->p_docu = p_docu;
    p_redraw_context->p_view = p_view;

#if WINDOWS
    p_redraw_context->windows.paintstruct.hdc = hdc;
#endif

    p_redraw_context->display_mode = DISPLAY_DESK_AREA;

    p_redraw_context->border_width.x = p_redraw_context->border_width.y = p_docu->page_def.grid_size;

    host_redraw_context_set_host_xform(p_redraw_context, &p_view->host_xform[XFORM_HORZ]);

    host_redraw_context_fillin(p_redraw_context);

    if(0 == g_border_horz_pixit_height)
        border_horz_cache_height(p_redraw_context);

    if(0 == g_ruler_horz_pixit_height)
        ruler_horz_cache_height(p_redraw_context);

    host_redraw_context_set_host_xform(p_redraw_context, &p_view->host_xform[XFORM_VERT]);

    host_redraw_context_fillin(p_redraw_context);

    if(0 == g_border_vert_pixit_width)
        border_vert_cache_width(p_redraw_context);

    if(0 == g_ruler_vert_pixit_width)
        ruler_vert_cache_width(p_redraw_context);

    fonty_cache_trash(p_redraw_context); /* host font handles belong to fonty session */

#if WINDOWS
    void_WrapOsBoolChecking(1 == ReleaseDC(hwnd, hdc));
#endif
}

_Check_return_
extern PIXIT
view_border_pixit_size(
    _ViewRef_   P_VIEW p_view,
    _InVal_     BOOL horizontal_border)
{
    if(horizontal_border)
    {
        if(0 == g_border_horz_pixit_height)
            view_edge_windows_cache_size(p_view);
        return(g_border_horz_pixit_height);
    }
    else
    {
        if(0 == g_border_vert_pixit_width)
            view_edge_windows_cache_size(p_view);
        return(g_border_vert_pixit_width);
    }
}

extern void
view_border_reset_pixit_size(
    _InVal_     BOOL horizontal_border)
{
    if(horizontal_border)
        g_border_horz_pixit_height = 0;
    else
        g_border_vert_pixit_width  = 0;
}

_Check_return_
extern PIXIT
view_ruler_pixit_size(
    _ViewRef_   P_VIEW p_view,
    _InVal_     BOOL horizontal_ruler)
{
    if(horizontal_ruler)
    {
        if(0 == g_ruler_horz_pixit_height)
            view_edge_windows_cache_size(p_view);
        return(g_ruler_horz_pixit_height);
    }
    else
    {
        if(0 == g_ruler_vert_pixit_width)
            view_edge_windows_cache_size(p_view);
        return(g_ruler_vert_pixit_width);
    }
}

extern void
view_ruler_reset_pixit_size(
    _InVal_     BOOL horizontal_ruler)
{
    if(horizontal_ruler)
        g_ruler_horz_pixit_height = 0;
    else
        g_ruler_vert_pixit_width  = 0;
}

extern void
view_edge_windows_cache_info(
    _ViewRef_   P_VIEW p_view)
{
    p_view->vert_border_gdi_width  = 0;
    p_view->horz_border_gdi_height = 0;

    p_view->vert_ruler_gdi_width  = 0;
    p_view->horz_ruler_gdi_height = 0;

    { /* Get the h/v border sizes in device units from the pixit version */
    PIXIT_RECT pixit_rect;
    GDI_RECT gdi_rect;
    pixit_rect.tl.x = pixit_rect.tl.y = 0;
    pixit_rect.br.x = view_border_pixit_size(p_view, FALSE);
    pixit_rect.br.y = view_border_pixit_size(p_view, TRUE);
    status_consume(window_rect_from_pixit_rect(&gdi_rect, &pixit_rect, &p_view->host_xform[XFORM_BACK]));
    p_view->vert_border_gdi_width  = gdi_rect.br.x;
    p_view->horz_border_gdi_height = abs(gdi_rect.br.y); /* RISC OS GDI upside-down */
    } /*block*/

    { /* Get the h/v ruler sizes in device units from the pixit version */
    PIXIT_RECT pixit_rect;
    GDI_RECT gdi_rect;
    pixit_rect.tl.x = pixit_rect.tl.y = 0;
    pixit_rect.br.x = view_ruler_pixit_size(p_view, FALSE);
    pixit_rect.br.y = view_ruler_pixit_size(p_view, TRUE);
    status_consume(window_rect_from_pixit_rect(&gdi_rect, &pixit_rect, &p_view->host_xform[XFORM_BACK]));
    p_view->vert_ruler_gdi_width  = gdi_rect.br.x;
    p_view->horz_ruler_gdi_height = abs(gdi_rect.br.y); /* RISC OS GDI upside-down */
    } /*block*/
}

static void
default_scale_info(
    _OutRef_    P_SCALE_INFO p_scale_info)
{
    p_scale_info->display_unit              = DISPLAY_UNIT_CM;  /* figures in cm */
    p_scale_info->numbered_units_multiplier = 1;                /* unit marks at 1 cm intervals */
    p_scale_info->coarse_div                = 2;                /* coarse marks at 0.5 cm intervals */
    p_scale_info->fine_div                  = 5;                /* fine marks at 0.1 cm intervals */
    p_scale_info->loaded                    = 0;                /* so caller can tell it's not local */
}

extern void
scale_info_from_docu(
    _DocuRef_   PC_DOCU p_docu,
    _InVal_     BOOL horizontal,
    _OutRef_    P_SCALE_INFO p_scale_info)
{
    profile_ensure_frame();

    if(!horizontal && p_docu->vscale_info.loaded)
    {
        *p_scale_info = p_docu->vscale_info;
        return;
    }

    if(p_docu->scale_info.loaded)
    {
        *p_scale_info = p_docu->scale_info;
        return;
    }

    {
    const PC_DOCU p_docu_config = p_docu_from_config();

    if(!horizontal && p_docu_config->vscale_info.loaded)
    {
        *p_scale_info = p_docu_config->vscale_info;
        p_scale_info->loaded = 0; /* so caller can tell it's not local */
        return;
    }

    if(p_docu_config->scale_info.loaded)
    {
        *p_scale_info = p_docu_config->scale_info;
        p_scale_info->loaded = 0; /* so caller can tell it's not local */
        return;
    }
    } /*block*/

    assert0();
    default_scale_info(p_scale_info);
}

T5_CMD_PROTO(extern, t5_cmd_ruler_scale)
{
    SCALE_INFO scale_info;

    {
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 4);

    /* prevent division by zero etc. in release code */
    scale_info.display_unit = (DISPLAY_UNIT) p_args[0].val.s32;
    if((scale_info.display_unit < DISPLAY_UNIT_STT) || (scale_info.display_unit >= DISPLAY_UNIT_COUNT))
    {
        default_scale_info(&scale_info);
    }
    else
    {
        scale_info.coarse_div = p_args[1].val.s32;
        assert(scale_info.coarse_div > 0);
        scale_info.coarse_div = MAX(1, scale_info.coarse_div);

        scale_info.fine_div = p_args[2].val.s32;
        assert(scale_info.fine_div > 0);
        scale_info.fine_div = MAX(1, scale_info.fine_div);

        scale_info.numbered_units_multiplier = p_args[3].val.s32;
        assert(scale_info.numbered_units_multiplier > 0);
        scale_info.numbered_units_multiplier = MAX(1, scale_info.numbered_units_multiplier);
    }

    scale_info.loaded = TRUE;
    } /*block*/

    {
    P_SCALE_INFO p_scale_info = (t5_message == T5_CMD_RULER_SCALE_V) ? &p_docu->vscale_info : &p_docu->scale_info;

    if(0 != memcmp32(p_scale_info, &scale_info, sizeof32(scale_info)))
    {
        /* repaint whichever (or both) ruler(s) */
        SKEL_RECT skel_rect;
        RECT_FLAGS rect_flags;

        *p_scale_info = scale_info;

        skel_rect.tl.pixit_point.x = skel_rect.tl.pixit_point.y = 0;
        skel_rect.tl.page_num.x = skel_rect.tl.page_num.y = 0;
        skel_rect.br = skel_rect.tl;

        RECT_FLAGS_CLEAR(rect_flags);
        rect_flags.extend_right_window = rect_flags.extend_down_window = 1;

        view_update_later(p_docu, (t5_message == T5_CMD_RULER_SCALE_V) ? UPDATE_RULER_VERT : UPDATE_RULER_HORZ, &skel_rect, rect_flags);

        /* SKS after 1.04 23sep93 vert ruler may be being controlled by horz ruler info */
        if(t5_message != T5_CMD_RULER_SCALE_V)
            if(!p_docu->vscale_info.loaded)
                view_update_later(p_docu, UPDATE_RULER_VERT, &skel_rect, rect_flags);
    }
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
scale_info_save(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     BOOL horizontal)
{
    const PC_SCALE_INFO p_scale_info = horizontal ? &p_docu->scale_info : &p_docu->vscale_info;
    STATUS status = STATUS_OK;

    if(p_scale_info->loaded)
    {
        const OBJECT_ID object_id = OBJECT_ID_SKEL;
        const T5_MESSAGE t5_message = horizontal ? T5_CMD_RULER_SCALE : T5_CMD_RULER_SCALE_V;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE arglist_handle;

        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 4);
            p_args[0].val.s32 = p_scale_info->display_unit;
            p_args[1].val.s32 = p_scale_info->coarse_div;
            p_args[2].val.s32 = p_scale_info->fine_div;
            p_args[3].val.s32 = p_scale_info->numbered_units_multiplier;
            status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
            arglist_dispose(&arglist_handle);
        }
    }

    return(status);
}

_Check_return_
extern STATUS
vi_edge_msg_post_save_1(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    if(p_of_op_format->of_template.ruler_scale)
    {
        status_return(scale_info_save(p_docu, p_of_op_format, 1));
        status_return(scale_info_save(p_docu, p_of_op_format, 0));
    }

    return(STATUS_OK);
}

/*struct DISPLAY_UNIT_INFO*/
    /*FP_PIXIT fp_pixits_per_unit;*/
    /*STATUS msg_unit;*/

static const
DISPLAY_UNIT_INFO
display_unit_info_tab[DISPLAY_UNIT_COUNT] =
{
    { FP_PIXITS_PER_CM, MSG_DIALOG_UNITS_CM },
    { FP_PIXITS_PER_MM, MSG_DIALOG_UNITS_MM },
    { PIXITS_PER_INCH,  MSG_DIALOG_UNITS_INCHES },
    { PIXITS_PER_POINT, MSG_DIALOG_UNITS_POINTS }
};

extern void
display_unit_info_from_display_unit(
    _OutRef_    P_DISPLAY_UNIT_INFO p_display_unit_info,
    _InVal_     DISPLAY_UNIT display_unit)
{
    /* if any of these fail, display_unit_info_tab[] may need reordering/extending */
    assert_EQ(DISPLAY_UNIT_STT   , 0);
    assert_EQ(DISPLAY_UNIT_CM    , 0);
    assert_EQ(DISPLAY_UNIT_MM    , 1);
    assert_EQ(DISPLAY_UNIT_INCHES, 2);
    assert_EQ(DISPLAY_UNIT_POINTS, 3);
    assert_EQ(DISPLAY_UNIT_COUNT , 4);
    assert((display_unit >= DISPLAY_UNIT_STT) && (display_unit < DISPLAY_UNIT_COUNT));

    if((display_unit < DISPLAY_UNIT_STT) || (display_unit >= DISPLAY_UNIT_COUNT))
    {
        *p_display_unit_info = display_unit_info_tab[DISPLAY_UNIT_CM];
        return;
    }

    *p_display_unit_info = display_unit_info_tab[display_unit];
}

_Check_return_
static PIXIT
click_stop_round(
    _InVal_     PIXIT pixit_in,
    _InRef_     PC_FP_PIXIT p_click_stop_step)
{
    const FP_PIXIT click_stop_step = *p_click_stop_step;
    FP_PIXIT fp_pixit = (FP_PIXIT) pixit_in;
    F64 f64 = fp_pixit / click_stop_step;
    S32 s32;
    PIXIT pixit_out;

    if(f64 < 0.0)
        f64 -= 0.5;
    else
        f64 += 0.5;
    s32 = (S32) f64; /* cast truncates towards zero */

    fp_pixit = s32 * click_stop_step;
    fp_pixit += 0.5; /* SKS 23feb96 consider rounding to 4mm (226.772 pixits) - give the next pixit value */
    pixit_out = (PIXIT) fp_pixit;

    return(pixit_out);
}

#include "cmodules/mathxtri.h"

_Check_return_
static PIXIT
click_stop_floor(
    _InVal_     PIXIT pixit_in,
    _InRef_     PC_FP_PIXIT p_click_stop_step)
{
    const FP_PIXIT click_stop_step = *p_click_stop_step;
    FP_PIXIT fp_pixit = (FP_PIXIT) pixit_in;
    F64 f64 = fp_pixit / click_stop_step;
    S32 s32;
    PIXIT truncated;
    PIXIT pixit_out;

    s32 = (S32) f64; /* cast truncates towards zero */

    if((F64) s32 == f64)
    {   /* pixit_in is already at the correct multiple */
        return(pixit_in);
    }

    if(click_stop_step >= 2.0)
    {   /* attempt to make click_stop_floor idempotent - stops leftward drift of objects */
        static const F64 f64_one_mins_eps = 1.0 - 16*F64_EPSILON;
        const F64 f64_one_scaled = f64_one_mins_eps / click_stop_step;
        f64 += f64_one_scaled;
        s32 = (S32) floor(f64);
    }

    fp_pixit = s32 * click_stop_step;

    truncated = (PIXIT) fp_pixit; /* cast truncates towards zero */

    pixit_out = truncated;

    return(pixit_out);
}

_Check_return_
static PIXIT
click_stop_ceil(
    _InVal_     PIXIT pixit_in,
    _InRef_     PC_FP_PIXIT p_click_stop_step)
{
    const FP_PIXIT click_stop_step = *p_click_stop_step;
    FP_PIXIT fp_pixit = (FP_PIXIT) pixit_in;
    F64 f64 = fp_pixit / click_stop_step;
    S32 s32;
    PIXIT truncated;
    PIXIT pixit_out;

    s32 = (S32) ceil(f64);

    fp_pixit = s32 * click_stop_step;

    truncated = (PIXIT) fp_pixit;

    if(pixit_in <= truncated)
        pixit_out = truncated;
    else
        pixit_out = (PIXIT) (truncated + click_stop_step);

    return(pixit_out);
}

extern void
click_stop_initialise(
    _OutRef_    P_FP_PIXIT p_click_stop_step,
    _InRef_     PC_SCALE_INFO p_scale_info,
    _InVal_     BOOL fine)
{
    const DISPLAY_UNIT display_unit = p_scale_info->display_unit;
    FP_PIXIT click_stop_step;

    assert((display_unit >= DISPLAY_UNIT_STT) && (display_unit < DISPLAY_UNIT_COUNT));
    click_stop_step = display_unit_info_tab[display_unit].fp_pixits_per_unit;

    click_stop_step *= p_scale_info->numbered_units_multiplier;

    click_stop_step /= p_scale_info->coarse_div;

    if(fine)
        click_stop_step /= p_scale_info->fine_div;

    trace_6(TRACE_APP_MEASUREMENT, TEXT("click_stop_initialise(fine=%s): c.s.s.:=%g from p.p.u.=%g, n.u.m.=%d, c.div=%d, f.div=%d"),
        report_boolstring(fine), click_stop_step,
        display_unit_info_tab[display_unit].fp_pixits_per_unit, p_scale_info->numbered_units_multiplier,
        p_scale_info->coarse_div, p_scale_info->fine_div);

    *p_click_stop_step = click_stop_step;
}

_Check_return_
extern PIXIT
click_stop_limited(
    _InVal_     PIXIT try_value,
    _InVal_     PIXIT min_value,
    _InVal_     PIXIT max_value,
    _InVal_     PIXIT click_stop_origin,
    _InRef_     PC_FP_PIXIT p_click_stop_step)
{
    PIXIT value;

    /* round the value (-, then +, c.s.o.) and clip again */
    value = click_stop_round(try_value - click_stop_origin, p_click_stop_step) + click_stop_origin;

    if(value < min_value)
    {
        trace_10(TRACE_APP_MEASUREMENT, TEXT("click_stop_limited: MIN c.s.l.!=%d(%d) from try=%d(%d), min=%d(%d), max=%d(%d), c.s.s.=%g, origin=%d"),
            value, value - click_stop_origin,
            try_value, try_value - click_stop_origin,
            min_value, min_value - click_stop_origin,
            max_value, max_value - click_stop_origin,
            *p_click_stop_step, click_stop_origin);
        return(min_value);
    }

    if(value > max_value)
    {
        trace_10(TRACE_APP_MEASUREMENT, TEXT("click_stop_limited: MAX c.s.l.!=%d(%d) from try=%d(%d), min=%d(%d), max=%d(%d), c.s.s.=%g, origin=%d"),
            value, value - click_stop_origin,
            try_value, try_value - click_stop_origin,
            min_value, min_value - click_stop_origin,
            max_value, max_value - click_stop_origin,
            *p_click_stop_step, click_stop_origin);
        return(max_value);
    }

    trace_10(TRACE_APP_MEASUREMENT, TEXT("click_stop_limited: c.s.l.:=%d(%d) from try=%d(%d), min=%d(%d), max=%d(%d), c.s.s.=%g, origin=%d"),
            value, value - click_stop_origin,
            try_value, try_value - click_stop_origin,
            min_value, min_value - click_stop_origin,
            max_value, max_value - click_stop_origin,
            *p_click_stop_step, click_stop_origin);

    return(value);
}

_Check_return_
extern PIXIT
skel_ruler_snap_to_click_stop(
    _DocuRef_   PC_DOCU p_docu,
    _InVal_     BOOL horizontal,
    _InVal_     PIXIT pixit_in,
    _InVal_     SNAP_TO_CLICK_STOP_MODE mode)
{
    PIXIT pixit_out = pixit_in;
    SCALE_INFO scale_info;
    FP_PIXIT click_stop_step;

#if 0 /* proof */
    static int test = 1;
    if(test)
    {
        PIXIT i;
        click_stop_step = 2.4;
        for(i = -14; i <= 14; ++i)
        {
            pixit_out = click_stop_floor(i, &click_stop_step);
            reportf(TEXT("TEST FLOOR: out:=%d from in=%d"),
                pixit_out, i);
        }
        for(i = -14; i <= 14; ++i)
        {
            pixit_out = click_stop_ceil(i, &click_stop_step);
            reportf(TEXT("TEST CEIL: out:=%d from in=%d"),
                pixit_out, i);
        }
        test = 0;
    }
#endif

    scale_info_from_docu(p_docu, horizontal, &scale_info);

    switch(mode)
    {
    default: default_unhandled();
#if CHECKING
    case SNAP_TO_CLICK_STOP_ROUND:
    case SNAP_TO_CLICK_STOP_ROUND_COARSE:
#endif
        click_stop_initialise(&click_stop_step, &scale_info, (SNAP_TO_CLICK_STOP_ROUND_COARSE != mode));
        pixit_out = click_stop_round(pixit_in, &click_stop_step);
        trace_4(TRACE_APP_MEASUREMENT, TEXT("skel_ruler_snap_to_click_stop(horz=%s, mode=%d): out:=%d from in=%d"),
            report_boolstring(horizontal), mode, pixit_out, pixit_in);
        break;

    case SNAP_TO_CLICK_STOP_FLOOR:
        click_stop_initialise(&click_stop_step, &scale_info, TRUE);
        pixit_out = click_stop_floor(pixit_in, &click_stop_step);
        trace_3(TRACE_APP_MEASUREMENT, TEXT("skel_ruler_snap_to_click_stop(horz=%s, mode=FLOOR): out:=%d from in=%d"),
            report_boolstring(horizontal), pixit_out, pixit_in);
        break;

    case SNAP_TO_CLICK_STOP_CEIL:
        click_stop_initialise(&click_stop_step, &scale_info, TRUE);
        pixit_out = click_stop_ceil(pixit_in, &click_stop_step);
        trace_3(TRACE_APP_MEASUREMENT, TEXT("skel_ruler_snap_to_click_stop(horz=%s, mode=CEIL): out:=%d from in=%d"),
            report_boolstring(horizontal), pixit_out, pixit_in);
        break;
    }

    return(pixit_out);
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_vi_edge);

_Check_return_
static STATUS
vi_edge_msg_startup(void)
{
    border_style_startup();
    ruler_style_startup();

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_vi_edge_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(vi_edge_msg_startup());

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
vi_edge_msg_choice_changed_ui_styles(
    _DocuRef_       P_DOCU p_docu)
{
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;

    if(DOCNO_CONFIG == docno_from_p_docu(p_docu))
    {
        recache_border_horz_style = TRUE;
        recache_border_vert_style = TRUE;

        recache_ruler_horz_style = TRUE;
        recache_ruler_vert_style = TRUE;

        view_border_reset_pixit_size(TRUE);
        view_border_reset_pixit_size(FALSE);

        view_ruler_reset_pixit_size(TRUE);
        view_ruler_reset_pixit_size(FALSE);
    }

    view_update_all(p_docu, UPDATE_BORDER_HORZ);
    view_update_all(p_docu, UPDATE_BORDER_VERT);
    view_update_all(p_docu, UPDATE_RULER_HORZ);
    view_update_all(p_docu, UPDATE_RULER_VERT);

    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
    {
        view_cache_info(p_view);

        host_view_reopen(p_docu, p_view);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_vi_edge_msg_choice_changed, _InRef_ PC_MSG_CHOICE_CHANGED p_msg_choice_changed)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_choice_changed->t5_message)
    {
    case T5_CMD_STYLE_FOR_CONFIG:
        return(vi_edge_msg_choice_changed_ui_styles(p_docu));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_vi_edge)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_vi_edge_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_CHOICE_CHANGED:
        return(maeve_services_vi_edge_msg_choice_changed(p_docu, t5_message, (PC_MSG_CHOICE_CHANGED) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of vi_edge.c */
