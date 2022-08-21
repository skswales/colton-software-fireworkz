/* windows/ho_print.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Printer interface stuff for Fireworkz for Windows */

/* David De Vorchik (diz) 6th January 1994 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "common/res_defs.h"

#include "ob_skel/ho_print.h"

#include "commdlg.h"

#include "cderr.h"

#include "winspool.h"

/*
lovely statics
*/

static HDC
g_hic_format_pixits; /* actually an IC */ /*const-to-you*/

static HGLOBAL
hDevMode;

static HGLOBAL
hDevNames;

static BOOL
bAbort; /* FALSE if user cancels printing      */

static HWND
hAbortDlgWnd;

static ABORTPROC
lpAbortProc;

extern void
host_printer_name_query(
    _OutRef_    P_UI_TEXT p_ui_text)
{
    p_ui_text->type = UI_TEXT_TYPE_NONE;

    {
    LPDEVMODE lpDevMode = (LPDEVMODE) GlobalLock(hDevMode);
    if(lpDevMode)
    {
        PCTSTR p_device_name = lpDevMode->dmDeviceName;
        status_assert(ui_text_alloc_from_tstr(p_ui_text, p_device_name));
        GlobalUnlock(hDevMode);
    }
    } /*block*/

    if(p_ui_text->type == UI_TEXT_TYPE_NONE)
    {
        p_ui_text->type = UI_TEXT_TYPE_RESID;
        p_ui_text->text.resource_id = MSG_DIALOG_NO_PRINTER;
    }
}

extern void
host_read_default_paper_details(
    P_PAPER p_paper)
{
    /* assume a sort of minimalist (big-margin) A4 */
    p_paper->x_size = 11901; /* 209.1 mm */
    p_paper->y_size = 16839; /* 297.0 mm */

    p_paper->lm = 567; /* 10 mm */
    p_paper->bm = 567; /* 10 mm */

    p_paper->rm = 844; /* 15 mm */
    p_paper->tm = 844; /* 15 mm */

    p_paper->print_area_x = p_paper->x_size - (p_paper->lm + p_paper->rm);
    p_paper->print_area_y = p_paper->y_size - (p_paper->tm + p_paper->bm);
}

_Check_return_
extern STATUS
host_read_printer_paper_details(
    P_PAPER p_paper)
{
    STATUS status = STATUS_OK;
    SIZE phys_page_size;
    POINT pixels_per_inch, printing_offset;
    const HDC hic_format_pixits = host_get_hic_format_pixits();

    assert(hic_format_pixits);

    pixels_per_inch.x = GetDeviceCaps(hic_format_pixits, LOGPIXELSX);
    pixels_per_inch.y = GetDeviceCaps(hic_format_pixits, LOGPIXELSY);
    trace_2(TRACE_WINDOWS_HOST, TEXT("host_read_printer_paper_details: pixels_per_inch x == %d, y == %d"), pixels_per_inch.x, pixels_per_inch.y);

    phys_page_size.cx = GetDeviceCaps(hic_format_pixits, PHYSICALWIDTH);
    phys_page_size.cy = GetDeviceCaps(hic_format_pixits, PHYSICALHEIGHT);
    trace_2(TRACE_WINDOWS_HOST, TEXT("host_read_printer_paper_details: phys_page_size x == %d, y == %d"), phys_page_size.cx, phys_page_size.cy);

    printing_offset.x = GetDeviceCaps(hic_format_pixits, PHYSICALOFFSETX);
    printing_offset.y = GetDeviceCaps(hic_format_pixits, PHYSICALOFFSETX);
    trace_2(TRACE_WINDOWS_HOST, TEXT("host_read_printer_paper_details: offset x == %d, y == %d"), printing_offset.x, printing_offset.y);

    p_paper->print_area_x = muldiv64_ceil(GetDeviceCaps(hic_format_pixits, HORZRES), PIXITS_PER_INCH, pixels_per_inch.x);
    p_paper->print_area_y = muldiv64_ceil(GetDeviceCaps(hic_format_pixits, VERTRES), PIXITS_PER_INCH, pixels_per_inch.y);

    if(phys_page_size.cx > 0)
    {
        p_paper->x_size = muldiv64_ceil(phys_page_size.cx, PIXITS_PER_INCH, pixels_per_inch.x);
        p_paper->y_size = muldiv64_ceil(phys_page_size.cy, PIXITS_PER_INCH, pixels_per_inch.y);
    }
    else
    {
        p_paper->x_size = p_paper->print_area_x;
        p_paper->y_size = p_paper->print_area_y;
        status = create_error(ERR_PRINT_PAPER_DETAILS); /* have to return some indication so that read printer button can be disabled */
    }

    p_paper->lm = muldiv64_ceil(printing_offset.x, PIXITS_PER_INCH, pixels_per_inch.x);
    p_paper->tm = muldiv64_ceil(printing_offset.y, PIXITS_PER_INCH, pixels_per_inch.y);

    p_paper->rm = p_paper->x_size - (p_paper->lm + p_paper->print_area_x);
    p_paper->bm = p_paper->y_size - (p_paper->tm + p_paper->print_area_y);

    return(status);
}

/* Internal function that converts a return code from a printer function into
 * something that the Fireworkz world can understand properly without barfing!
 *
 * Printer return codes are -VE and these map into a set of STATUS_ bla.
*/

_Check_return_
static STATUS
status_from_print(
    _In_        int return_code)
{
    switch(return_code)
    {
    case SP_ERROR:       return(create_error(ERR_PRINT_UNKNOWN));
    case SP_OUTOFDISK:   return(create_error(ERR_PRINT_DISK_FULL));
    case SP_OUTOFMEMORY: return(create_error(ERR_PRINT_MEMORY_FULL));
    case SP_USERABORT:   return(create_error(ERR_PRINT_TERMINATED_VIA_PM));
    case SP_APPABORT:    return(create_error(ERR_PRINT_TERMINATED));
    default:
        if(return_code < 0)
            return(create_error(ERR_PRINT_UNKNOWN));
        break;
    }

    return(STATUS_OK);
}

extern BOOL CALLBACK
AbortProc(
    _HdcRef_    HDC hPr,
    _In_        int Code)
{
    UNREFERENCED_PARAMETER_InRef_(hPr);
    UNREFERENCED_PARAMETER(Code);

    if(hAbortDlgWnd) /* abort dialog up yet? */
    {
        MSG msg;

        /* Process messages intended for the abort dialog box */
        while(!bAbort && PeekMessage(&msg, NULL, 0, 0, TRUE))
            if(!IsDialogMessage(hAbortDlgWnd, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
    }

    /* bAbort is TRUE (return is FALSE) if the user has aborted */
    return(!bAbort);
}

extern INT_PTR CALLBACK
AbortDlg(
    _HwndRef_   HWND hDlg,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    switch(uiMsg)
    {
    case WM_INITDIALOG:
        /* Set the focus to the Cancel box of the dialog */
        hAbortDlgWnd = hDlg;
        SetFocus(GetDlgItem(hDlg, IDCANCEL));
        return(0); // done SetFocus

    /* Watch for Cancel button, RETURN key, ESCAPE key, or SPACE BAR */
    case WM_COMMAND:
        bAbort = TRUE;
        return(TRUE);
    }

    return(FALSE);
}

static PCTSTR g_host_printer_name;

static HDC saved_hic_format_pixits;

static void
hic_format_pixits_dispose(void)
{
    if(NULL == g_hic_format_pixits)
        return;

    consume(HFONT, SelectFont(g_hic_format_pixits, GetStockFont(ANSI_FIXED_FONT))); /* just in case we left something selected */

    void_WrapOsBoolChecking(DeleteDC(g_hic_format_pixits));
    g_hic_format_pixits = NULL;
}

_Check_return_
extern STATUS
host_printer_set(
    _In_opt_z_  PCTSTR printername)
{
    g_host_printer_name = printername;

    if(NULL == printername)
    {   /* delete the transient IC */
        hic_format_pixits_dispose();

        /* restore the saved IC */
        g_hic_format_pixits = saved_hic_format_pixits;
        return(STATUS_OK);
    }

    saved_hic_format_pixits = g_hic_format_pixits;

    g_hic_format_pixits = CreateIC(NULL, printername, NULL, NULL);

    if(NULL == g_hic_format_pixits)
    {   /* restore the saved IC */
        g_hic_format_pixits = saved_hic_format_pixits;
        return(ERR_PRINT_UNKNOWN);
    }

    SetMapMode(g_hic_format_pixits, MM_TWIPS); /* PIXITs are TWIPs */
    return(STATUS_OK);
}

/* c.f. view_rect_from_screen_rect_and_context */

static void
skel_rect_from_print_box(
    _InoutRef_  P_SKEL_RECT p_skel_rect /*page_num untouched*/,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    GDI_RECT gdi_rect;
    PIXIT_RECT pixit_rect;

    gdi_rect.tl.x = p_redraw_context->windows.paintstruct.rcPaint.left   + p_redraw_context->gdi_org.x;
    gdi_rect.tl.y = p_redraw_context->windows.paintstruct.rcPaint.top    + p_redraw_context->gdi_org.y;
    gdi_rect.br.x = p_redraw_context->windows.paintstruct.rcPaint.right  + p_redraw_context->gdi_org.x;
    gdi_rect.br.y = p_redraw_context->windows.paintstruct.rcPaint.bottom + p_redraw_context->gdi_org.y;

    pixit_rect_from_window_rect(&pixit_rect, &gdi_rect, &p_redraw_context->host_xform);

    p_skel_rect->tl.pixit_point.x = pixit_rect.tl.x;
    p_skel_rect->tl.pixit_point.y = pixit_rect.tl.y;
    p_skel_rect->br.pixit_point.x = pixit_rect.br.x;
    p_skel_rect->br.pixit_point.y = pixit_rect.br.y;
}

/* printer equivalent of host_redraw_context_set_host_xform() */

static void
print_host_redraw_context_set_host_xform(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/)
{
    p_host_xform->windows.pixels_per_inch.x = GetDeviceCaps(p_redraw_context->windows.paintstruct.hdc, LOGPIXELSX);
    p_host_xform->windows.pixels_per_inch.y = GetDeviceCaps(p_redraw_context->windows.paintstruct.hdc, LOGPIXELSY);

    p_host_xform->windows.d.x = muldiv64(p_host_xform->windows.pixels_per_inch.x, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);
    p_host_xform->windows.d.y = muldiv64(p_host_xform->windows.pixels_per_inch.y, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);

    p_host_xform->windows.multiplier_of_pixels.x = PIXITS_PER_METRE * p_host_xform->scale.b.x;
    p_host_xform->windows.multiplier_of_pixels.y = PIXITS_PER_METRE * p_host_xform->scale.b.y;

    p_host_xform->windows.divisor_of_pixels.x = p_host_xform->windows.d.x * p_host_xform->scale.t.x;
    p_host_xform->windows.divisor_of_pixels.y = p_host_xform->windows.d.y * p_host_xform->scale.t.y;
}

_Check_return_
static STATUS
print_one_printer_page(
    _HdcRef_    HDC hdc,
    _DocuRef_   P_DOCU p_docu,
    /*const*/ P_PRINT_CTRL p_print_ctrl,
    _InVal_     ARRAY_INDEX page_index,
    _InVal_     UINT pages_per_printer_page)
{
    REDRAW_CONTEXT_CACHE redraw_context_cache;
    REDRAW_CONTEXT redraw_context;
    const P_REDRAW_CONTEXT p_redraw_context = &redraw_context;
    POINT pixels_per_inch, page_size, printing_offset, phys_page_size; // pixels
    int lrm, tbm;
    POINT offset[2];
    UINT page_inner_index;
    S32 scale;
    STATUS status;

    trace_1(TRACE_WINDOWS_HOST, TEXT("host_print_document: page_index == ") S32_TFMT, page_index);
    status_return(status = status_from_print(StartPage(hdc)));

    pixels_per_inch.x = GetDeviceCaps(hdc, LOGPIXELSX);
    pixels_per_inch.y = GetDeviceCaps(hdc, LOGPIXELSY);

    page_size.x = GetDeviceCaps(hdc, HORZRES);
    page_size.y = GetDeviceCaps(hdc, VERTRES);

    printing_offset.x = GetDeviceCaps(hdc, PHYSICALOFFSETX);
    printing_offset.y = GetDeviceCaps(hdc, PHYSICALOFFSETY);

    phys_page_size.x = GetDeviceCaps(hdc, PHYSICALWIDTH);
    phys_page_size.y = GetDeviceCaps(hdc, PHYSICALHEIGHT);

    {
    RECT margin;

    margin.left = printing_offset.x;
    margin.top = printing_offset.y;
    margin.right = phys_page_size.x - page_size.x - margin.left;
    margin.bottom = phys_page_size.y - page_size.y - margin.top;

    lrm = MAX(margin.left, margin.right);
    tbm = MAX(margin.top, margin.bottom);
    } /*block*/

    zero_struct_ptr(p_redraw_context);

    zero_struct(redraw_context_cache);
    p_redraw_context->p_redraw_context_cache = &redraw_context_cache;

    p_redraw_context->flags.printer = 1;

    p_redraw_context->windows.paintstruct.hdc = hdc;
    p_redraw_context->windows.paintstruct.fErase = FALSE;
    // rcPaint set up shortly, rest of paintstruct is Windows internal garbage

    // DC that comes back is where the printer wants it, so if we want printable area to
    // start further down(across) the page we obviously have to arrange it so that
    // a +ve offset is added when going to GDI space i.e. our top margin - printer top offset
    // p_redraw_context->gdi_org.y(x) is subtracted from the point so that needs to be -ve in the above case
    if(p_print_ctrl->flags.two_up)
    {
        POINT pa, spa;
        PIXIT_POINT print_area;
        S32 scale1, scale2;

        pa.x = phys_page_size.x / 2 - 2 * lrm;
        pa.y = phys_page_size.y - 2 * tbm;

        print_area.x = p_docu->page_def.size_x - margin_left_from(&p_docu->page_def, 0) - margin_right_from(&p_docu->page_def, 0);
        print_area.y = p_docu->page_def.size_y - p_docu->page_def.margin_top - p_docu->page_def.margin_bottom;

        scale1 = muldiv64(100 * PIXITS_PER_INCH, pa.x, print_area.x * pixels_per_inch.x);
        scale2 = muldiv64(100 * PIXITS_PER_INCH, pa.y, print_area.y * pixels_per_inch.y);
        scale = MIN(scale1, scale2);

        offset[0].x = lrm; // ignore printing_offset for the mo
        offset[0].y = tbm;
        offset[1].x = phys_page_size.x / 2 + lrm;
        offset[1].y = offset[0].y;

        // attempt to centre page images in these holes
        spa.x = (int) muldiv64(print_area.x * pixels_per_inch.x, scale, 100 * PIXITS_PER_INCH);
        spa.y = (int) muldiv64(print_area.y * pixels_per_inch.y, scale, 100 * PIXITS_PER_INCH);

        offset[0].x += (pa.x - spa.x) / 2;
        offset[1].x += (pa.x - spa.x) / 2;
        offset[0].y += (pa.y - spa.y) / 2;
        offset[1].y += (pa.y - spa.y) / 2;
    }
    else
    {
        PIXIT_POINT tl;

        scale = p_docu->paper_scale;

        tl.x = margin_left_from(&p_docu->page_def, 0);
        tl.y = p_docu->page_def.margin_top;

        offset[0].x = (int) muldiv64(tl.x, scale * pixels_per_inch.x, 100 * PIXITS_PER_INCH); // ignore printing_offset for the mo
        offset[0].y = (int) muldiv64(tl.y, scale * pixels_per_inch.y, 100 * PIXITS_PER_INCH);
    }

    p_redraw_context->host_xform.scale.t.x = scale;
    p_redraw_context->host_xform.scale.t.y = scale;
    p_redraw_context->host_xform.scale.b.x = 100;
    p_redraw_context->host_xform.scale.b.y = 100;

    p_redraw_context->display_mode = DISPLAY_PRINT_AREA;

    p_redraw_context->border_width.x = p_redraw_context->border_width.y = p_docu->page_def.grid_size;
    p_redraw_context->border_width_2.x = p_redraw_context->border_width_2.y = 2 * p_docu->page_def.grid_size;

    print_host_redraw_context_set_host_xform(p_redraw_context, &p_redraw_context->host_xform);

    host_redraw_context_fillin(p_redraw_context);

    for(page_inner_index = 0; page_inner_index < pages_per_printer_page; ++page_inner_index)
    {
        S32 page_ident = page_index + page_inner_index;

        if(array_index_is_valid(&p_print_ctrl->h_page_list, page_ident))
        {
            const PC_PAGE_ENTRY p_page_entry = array_ptrc_no_checks(&p_print_ctrl->h_page_list, PAGE_ENTRY, page_ident);
            /* PAGE page_num_y = p_page_entry->page.y; */
            /* S32 pos_index = page_num_y & MAX(0, pages_per_printer_page - 1); */

            p_redraw_context->gdi_org.x = printing_offset.x - offset[page_inner_index].x;
            p_redraw_context->gdi_org.y = printing_offset.y - offset[page_inner_index].y;

            p_redraw_context->windows.paintstruct.rcPaint.left = 0;
            p_redraw_context->windows.paintstruct.rcPaint.top = 0;
            p_redraw_context->windows.paintstruct.rcPaint.right = page_size.x;
            p_redraw_context->windows.paintstruct.rcPaint.bottom = page_size.y;

            if(p_print_ctrl->flags.two_up)
            {
                int hx = phys_page_size.x / 2;
                if(page_inner_index == 0)
                    p_redraw_context->windows.paintstruct.rcPaint.right = hx - printing_offset.x;
                else
                    p_redraw_context->windows.paintstruct.rcPaint.left = hx - printing_offset.x;
            }

            p_redraw_context->windows.host_machine_clip_rect = p_redraw_context->windows.paintstruct.rcPaint;

            { // really do clip to this now
            HRGN h_clip_region = CreateRectRgn(p_redraw_context->windows.host_machine_clip_rect.left, p_redraw_context->windows.host_machine_clip_rect.top,  p_redraw_context->windows.host_machine_clip_rect.right,  p_redraw_context->windows.host_machine_clip_rect.bottom);
            if(NULL != h_clip_region)
            {
                SelectClipRgn(p_redraw_context->windows.paintstruct.hdc, h_clip_region);
                DeleteRgn(h_clip_region);
            }
            } /*block*/

            if((p_page_entry->page.x >= 0) && (p_page_entry->page.y >= 0))
            {
                PIXIT_RECT print_area;
                SKELEVENT_REDRAW skelevent_redraw;

                print_area.tl.x = 0;
                print_area.tl.y = 0;
                print_area.br.x = p_docu->page_def.size_x - margin_left_from(&p_docu->page_def, p_page_entry->page.y) - margin_right_from(&p_docu->page_def, p_page_entry->page.y);
                print_area.br.y = p_docu->page_def.size_y - p_docu->page_def.margin_top - p_docu->page_def.margin_bottom;

                zero_struct(skelevent_redraw);

                /*skelevent_redraw.flags = REDRAW_FLAGS_INIT;*/
                skelevent_redraw.flags.show_content = TRUE;

                skelevent_redraw.redraw_context = redraw_context;

                skel_rect_from_print_box(&skelevent_redraw.clip_skel_rect, p_redraw_context);
                skelevent_redraw.clip_skel_rect.tl.page_num = p_page_entry->page;
                skelevent_redraw.clip_skel_rect.br.page_num = p_page_entry->page;

                host_paint_start(&skelevent_redraw.redraw_context);
                view_redraw_page(p_docu, &skelevent_redraw, &print_area);
                host_paint_end(&skelevent_redraw.redraw_context);
            }
        }
    }

    status = status_from_print(EndPage(hdc));

    return(status);
}

_Check_return_
extern STATUS
host_print_document(
    _DocuRef_   P_DOCU p_docu,
    /*const*/ P_PRINT_CTRL p_print_ctrl)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX outer_copies = p_print_ctrl->flags.collate ? p_print_ctrl->copies : 1;
    ARRAY_INDEX inner_copies = p_print_ctrl->flags.collate ? 1 : p_print_ctrl->copies;
    UINT pages_per_printer_page;
    HDC hdc = NULL;
    HCURSOR hcursor_old;

    trace_0(TRACE_WINDOWS_HOST, TEXT("host_print_document: start"));

    {
    LPDEVNAMES lpDevNames = (LPDEVNAMES) GlobalLock(hDevNames);
    if(NULL != lpDevNames)
    {    
        PCTSTR lpszDriverName = (PCTSTR) lpDevNames + lpDevNames->wDriverOffset;
        PCTSTR lpszDeviceName = (PCTSTR) lpDevNames + lpDevNames->wDeviceOffset;
        PCTSTR lpszPortName   = (PCTSTR) lpDevNames + lpDevNames->wOutputOffset;
        LPDEVMODE lpDevMode = (LPDEVMODE) GlobalLock(hDevMode); // must butcher in place 'cos driver has added extra info that we don't know about
        if(NULL != lpDevMode)
        {
            if(0 == (lpDevMode->dmFields & DM_COPIES))
            {
                /* SKS 12apr95 allow for non-multiple copy drivers */
                if(inner_copies > 1)
                {
                    assert(1 == outer_copies);
                    outer_copies = inner_copies;
                    inner_copies = 1;
                }
            }

            lpDevMode->dmFields |= DM_ORIENTATION;

            /* NB headers for Windows SDK v6.0A and on force nameless in any case here */
            lpDevMode->dmOrientation =
                (p_print_ctrl->flags.landscape || p_print_ctrl->flags.two_up)
                    ? DMORIENT_LANDSCAPE
                    : DMORIENT_PORTRAIT;

            /* NB _WIN32_WINNT controls whether this is a Windows 2000 compatible structure or not */
            lpDevMode->dmCopies = (short) inner_copies;

            if(NULL != g_host_printer_name)
            {
                HANDLE hPrinter;
                static DEVMODE g_devmode;
                zero_struct(g_devmode);
                g_devmode.dmSize = sizeof32(g_devmode);
                lpDevMode = NULL;
                if(OpenPrinter(de_const_cast(PTSTR, g_host_printer_name), &hPrinter, NULL))
                {
                    if(DocumentProperties(NULL, hPrinter, de_const_cast(PTSTR, g_host_printer_name), &g_devmode, NULL, DM_OUT_BUFFER))
                    {
                        lpDevMode = &g_devmode;
                    }
                    ClosePrinter(hPrinter);
                }
                lpszDeviceName = g_host_printer_name;
                lpszPortName = NULL;
            }
            hdc = CreateDC(lpszDriverName, lpszDeviceName, lpszPortName, lpDevMode);
            GlobalUnlock(hDevMode);
        }
        GlobalUnlock(hDevNames);
    }
    } /*block*/

    if(NULL == hdc)
        return(create_error(ERR_PRINT_WONT_OPEN));

    if(p_print_ctrl->flags.two_up)
        pages_per_printer_page = 2;
    else
        pages_per_printer_page = 1;

    hcursor_old = SetCursor(LoadCursor(NULL, IDC_WAIT));

    bAbort = FALSE;

    {
    HWND hwnd_parent = NULL /*host_get_icon_hwnd()*/;
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;
    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
    {
        const HWND hwnd = p_view->main[WIN_BACK].hwnd;
        EnableWindow(hwnd, FALSE); /* Disable the main window to avoid reentrancy problems */
        if(viewno == p_docu->viewno_caret)
            hwnd_parent = hwnd;
    }

    lpAbortProc = AbortProc;
    if(lpAbortProc)
    {
        (void) SetAbortProc(hdc, lpAbortProc);

        (void) CreateDialog(GetInstanceHandle(), TEXT("PrintAbortDlg"), hwnd_parent, AbortDlg); // modeless dialog
        if(hAbortDlgWnd)
        {
            SetDlgItemText(hAbortDlgWnd, 105 /*IDC_FILENAME*/, p_docu->docu_name.leaf_name);
            ShowWindow(hAbortDlgWnd, SW_NORMAL);
        }
    }
    } /*block*/

    (void) SetCursor(hcursor_old); /* Remove the hourglass */

    {
    DOCINFO docinfo;
    docinfo.cbSize = sizeof32(docinfo);
    docinfo.lpszDocName = p_docu->docu_name.leaf_name;
    docinfo.lpszOutput = NULL;
    status = status_from_print(StartDoc(hdc, &docinfo));
    } /*block*/

    if(status_ok(status))
    {
        { /* print n collated sets */
        ARRAY_INDEX outer_copy_idx;
        for(outer_copy_idx = 0; outer_copy_idx < outer_copies; ++outer_copy_idx)
        {
            const ARRAY_INDEX page_count = array_elements(&p_print_ctrl->h_page_list);
            ARRAY_INDEX page_index;
            PRINTER_PERCENTAGE printer_percentage;

            print_percentage_initialise(p_docu, &printer_percentage, page_count);

            for(page_index = 0; page_index < page_count; page_index += pages_per_printer_page)
            {
                status_break(status = print_one_printer_page(hdc, p_docu, p_print_ctrl, page_index, pages_per_printer_page));

                if(bAbort)
                    status_break(status = create_error(ERR_PRINT_TERMINATED));

                print_percentage_page_inc(&printer_percentage);
            }

            print_percentage_finalise(&printer_percentage);

            status_break(status);
        }
        } /*block*/

        if(status_ok(status))
        {
            trace_0(TRACE_WINDOWS_HOST, TEXT("host_print_document: print was successful - EndDoc posted"));
            status = status_from_print(EndDoc(hdc));
        }
        else
        {
            STATUS status1;
            trace_0(TRACE_WINDOWS_HOST, TEXT("host_print_document: print was a failure -  AbortDoc posted"));
            status1 = status_from_print(AbortDoc(hdc));
        }
    }

    { // reenable the windows on this document
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;
    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
    {
        const HWND hwnd = p_view->main[WIN_BACK].hwnd;
        EnableWindow(hwnd, TRUE);
    }
    } /*block*/

    if(hAbortDlgWnd)
    {
        DestroyWindow(hAbortDlgWnd);
        hAbortDlgWnd = NULL;
    }

    //SetCursor(hcursor);

    DeleteDC(hdc);

    trace_0(TRACE_WINDOWS_HOST, TEXT("host_print_document: finished"));

    return(status);
}

/* Windows specific command for performing print setup.
 * This invokes the standard dialog for print setup.
 * This is owned by the printer driver itself and presents printer specific options.
*/

T5_CMD_PROTO(extern, t5_cmd_print_setup)
{
    const P_VIEW p_view = p_view_from_viewno_caret(p_docu);
    STATUS status = STATUS_OK;
    PRINTDLG pd;
    BOOL ok;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    zero_struct(pd);

    pd.lStructSize = sizeof32(pd);
    pd.hwndOwner = p_view->main[WIN_BACK].hwnd;
    pd.Flags = PD_PRINTSETUP | PD_RETURNIC;
    pd.hDevMode = hDevMode;
    pd.hDevNames = hDevNames;

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_APP_FONTS))
        if(NULL != hDevNames)
        {
            LPDEVNAMES lpDevNames = (LPDEVNAMES) GlobalLock(hDevNames);
            if(NULL != lpDevNames)
            {
                PCTSTR lpszDriverName = (PCTSTR) lpDevNames + lpDevNames->wDriverOffset;
                PCTSTR lpszDeviceName = (PCTSTR) lpDevNames + lpDevNames->wDeviceOffset;
                PCTSTR lpszPortName   = (PCTSTR) lpDevNames + lpDevNames->wOutputOffset;
                trace_3(TRACE_APP_FONTS, TEXT("print_setup (in) printer: '%s' '%s' '%s'"), lpszDriverName, lpszDeviceName, lpszPortName);
                GlobalUnlock(hDevNames);
            }
        }
#endif

    ok = PrintDlg(&pd);

    if(!ok)
    {
        DWORD dwError = CommDlgExtendedError();

        switch(dwError)
        {
        case 0:
            break;

        case CDERR_FINDRESFAILURE:
            break;

        case CDERR_INITIALIZATION:
        case CDERR_LOADRESFAILURE:
        case CDERR_LOCKRESFAILURE:
        case CDERR_LOADSTRFAILURE:
        case CDERR_MEMALLOCFAILURE:
        case CDERR_MEMLOCKFAILURE:
            status = status_nomem();
            break;

        case CDERR_NOHINSTANCE:
        case CDERR_NOHOOK:
        case CDERR_NOTEMPLATE:
            assert0();
            break;

        case CDERR_REGISTERMSGFAIL:
            break;
        case CDERR_STRUCTSIZE:
            break;

        case PDERR_CREATEICFAILURE:
            break;
        case PDERR_DEFAULTDIFFERENT:
            break;
        case PDERR_DNDMMISMATCH:
            break;
        case PDERR_GETDEVMODEFAIL:
            break;
        case PDERR_INITFAILURE:
            break;
        case PDERR_LOADDRVFAILURE:
            break;
        case PDERR_NODEFAULTPRN:
            break;
        case PDERR_NODEVICES:
            break;
        case PDERR_PARSEFAILURE:
            break;
        case PDERR_PRINTERNOTFOUND:
            break;
        case PDERR_SETUPFAILURE:
            break;

        default:
            break;
        }
    }

    hic_format_pixits_dispose();

    hDevMode = pd.hDevMode;
    hDevNames = pd.hDevNames;
    g_hic_format_pixits = pd.hDC;

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_APP_FONTS))
        if(NULL != hDevNames)
        {
            LPDEVNAMES lpDevNames = (LPDEVNAMES) GlobalLock(hDevNames);
            if(NULL != lpDevNames)
            {
                PCTSTR lpszDriverName = (PCTSTR) lpDevNames + lpDevNames->wDriverOffset;
                PCTSTR lpszDeviceName = (PCTSTR) lpDevNames + lpDevNames->wDeviceOffset;
                PCTSTR lpszPortName   = (PCTSTR) lpDevNames + lpDevNames->wOutputOffset;
                trace_3(TRACE_APP_FONTS, TEXT("print_setup (out) printer: '%s' '%s' '%s'"), lpszDriverName, lpszDeviceName, lpszPortName);
                GlobalUnlock(hDevNames);
            }
        }
#endif

    /* if the printer stuff failed, get an IC for the screen */
    if(NULL == g_hic_format_pixits)
        g_hic_format_pixits = CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);

    if(NULL == g_hic_format_pixits)
        status_accumulate(status, ERR_PRINT_UNKNOWN);
    else
        SetMapMode(g_hic_format_pixits, MM_TWIPS); /* PIXITs are TWIPs */

    return(status);
}

static void
dispose_hic_printer(void)
{
    if(hDevMode)
    {
        GlobalFree(hDevMode);
        hDevMode = NULL;
    }

    if(hDevNames)
    {
        GlobalFree(hDevNames);
        hDevNames = NULL;
    }

    hic_format_pixits_dispose();
}

/* set up default printer information context - used for formatting */

static void
default_hic_printer(void)
{
    dispose_hic_printer();

    {
    BOOL res;
    PRINTDLG pd; /* this is the most amazing call ever! */
    zero_struct(pd);
    pd.lStructSize = sizeof32(pd);
    if(NULL == (pd.hwndOwner = GetActiveWindow()))
        pd.hwndOwner = GetDesktopWindow();
    pd.Flags = PD_RETURNDEFAULT | PD_RETURNIC;
    res = PrintDlg(&pd);
    if(res)
    {
        hDevMode = pd.hDevMode;
        hDevNames = pd.hDevNames;
        g_hic_format_pixits = pd.hDC;
    }
    } /*block*/

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_APP_FONTS))
        if(NULL != hDevNames)
        {
            LPDEVNAMES lpDevNames = (LPDEVNAMES) GlobalLock(hDevNames);
            if(NULL != lpDevNames)
            {
                PCTSTR lpszDriverName = (PCTSTR) lpDevNames + lpDevNames->wDriverOffset;
                PCTSTR lpszDeviceName = (PCTSTR) lpDevNames + lpDevNames->wDeviceOffset;
                PCTSTR lpszPortName   = (PCTSTR) lpDevNames + lpDevNames->wOutputOffset;
                trace_3(TRACE_APP_FONTS, TEXT("read printer: '%s' '%s' '%s'"), lpszDriverName, lpszDeviceName, lpszPortName);
                GlobalUnlock(hDevNames);
            }
        }
#endif

    /* if the printer stuff failed (possibly no default printer), get an IC for the screen */
    if(NULL == g_hic_format_pixits)
        g_hic_format_pixits = CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);

    if(NULL == g_hic_format_pixits)
        return;

    SetMapMode(g_hic_format_pixits, MM_TWIPS); /* PIXITs are TWIPs */
}

_Check_return_
extern HDC /* actually an IC */
host_get_hic_format_pixits(void)
{
    if(NULL == g_hic_format_pixits)
    {
        default_hic_printer();
        assert(NULL != g_hic_format_pixits);
    }

    return(g_hic_format_pixits);
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_print);

_Check_return_
static STATUS
ho_print_msg_exit2(void)
{
    dispose_hic_printer();

    return(STATUS_OK);
}

_Check_return_
static STATUS
maeve_services_ho_print_msg_initclose(
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose)
{
    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__EXIT2:
        return(ho_print_msg_exit2());

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
maeve_services_ho_print_msg_from_windows(
    _InRef_     P_MSG_FROM_WINDOWS p_t5_msg_from_windows)
{
    if(WM_SETTINGCHANGE == p_t5_msg_from_windows->uiMsg) /* track default printer change */
    {
        PCTSTR pszSection = (PCTSTR) p_t5_msg_from_windows->lParam;

        if((NULL == pszSection) || (0 == _tcsicmp(pszSection, TEXT("windows"))))
        {
            default_hic_printer();

            return(STATUS_OK);
        }
    }

    return(STATUS_OK);
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_print)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_ho_print_msg_initclose((PC_MSG_INITCLOSE) p_data));

    case T5_MSG_FROM_WINDOWS:
        return(maeve_services_ho_print_msg_from_windows((P_MSG_FROM_WINDOWS) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of windows/ho_print.c */
