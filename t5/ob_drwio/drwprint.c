/* drwprint.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2001-2015 R W Colton */

/* 'print' to Drawfile */

/* SKS July 2001 */

#include "common/gflags.h"

#include "ob_drwio/ob_drwio.h"

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

#include "cmodules/gr_diag.h"

#include "cmodules/gr_rdia3.h"

#if WINDOWS && 0

/* metafile equivalent of host_redraw_context_set_host_xform() */

static void
metafile_host_redraw_context_set_host_xform(
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/)
{
    p_host_xform->windows.pixels_per_inch.x =
    p_host_xform->windows.pixels_per_inch.y = 96;

    p_host_xform->windows.d.x = muldiv64(p_host_xform->windows.pixels_per_inch.x, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);
    p_host_xform->windows.d.y = muldiv64(p_host_xform->windows.pixels_per_inch.y, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);

    p_host_xform->windows.multiplier_of_pixels.x = PIXITS_PER_METRE * p_host_xform->scale.b.x;
    p_host_xform->windows.multiplier_of_pixels.y = PIXITS_PER_METRE * p_host_xform->scale.b.y;

    p_host_xform->windows.divisor_of_pixels.x = p_host_xform->windows.d.x * p_host_xform->scale.t.x;
    p_host_xform->windows.divisor_of_pixels.y = p_host_xform->windows.d.y * p_host_xform->scale.t.y;
}

T5_CMD_PROTO(static, t5_cmd_test_save_metafile)
{
    STATUS status = STATUS_OK;

    HDC hdc; // of the metafile created below

    // Obtain a handle to a reference device context.
    const HDC hdcRef = GetDC(NULL);

    // Determine the picture frame dimensions.
    // iWidthMM is the display width in millimeters.
    // iHeightMM is the display height in millimeters.
    // iWidthPels is the display width in pixels.
    // iHeightPels is the display height in pixels
    int iWidthMM = GetDeviceCaps(hdcRef, HORZSIZE);
    int iHeightMM = GetDeviceCaps(hdcRef, VERTSIZE);
    int iWidthPels = GetDeviceCaps(hdcRef, HORZRES);
    int iHeightPels = GetDeviceCaps(hdcRef, VERTRES);

    // Retrieve the coordinates of the client rectangle, in pixels.
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = iWidthPels;
    rect.bottom = iHeightPels;

    // Convert client coordinates to .01-mm units.
    // Use iWidthMM, iWidthPels, iHeightMM, and
    // iHeightPels to determine the number of
    // .01-millimeter units per pixel in the x-
    //  and y-directions.
    rect.left   = (rect.left   * iWidthMM  * 100)/iWidthPels;
    rect.top    = (rect.top    * iHeightMM * 100)/iHeightPels;
    rect.right  = (rect.right  * iWidthMM  * 100)/iWidthPels;
    rect.bottom = (rect.bottom * iHeightMM * 100)/iHeightPels;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    // Create the metafile device context.
    hdc =
        CreateEnhMetaFile(hdcRef,
                          (PCTSTR) TEXT("C:\\Temp\\sks.emf"),
                          &rect,
                          (PCTSTR) TEXT("Colton Fireworkz\0A Passion Play\0\0"));

    // Release the reference device context.

    void_WrapOsBoolChecking(1 == ReleaseDC(NULL, hdcRef));

    if(NULL != hdc)
    { /*flat*/
    /* from host_print_document */
    REDRAW_CONTEXT_CACHE redraw_context_cache;
    REDRAW_CONTEXT redraw_context;
    const P_REDRAW_CONTEXT p_redraw_context = &redraw_context;
    POINT offset[1];
    S32 scale;

#if RISCOS
    POINT os_units_per_inch, page_size; /* OS units */

    os_units_per_inch.x =
    os_units_per_inch.y = 96;

    page_size.x = os_units_per_inch.x * 9;
    page_size.y = os_units_per_inch.y * 12;
#elif WINDOWS
    POINT pixels_per_inch, page_size, printing_offset, phys_page_size; /* pixels */
    RECT margin;
    int lrm, tbm;

    pixels_per_inch.x =
    pixels_per_inch.y = 96;

    page_size.x = pixels_per_inch.x * 9;
    page_size.y = pixels_per_inch.y * 12;

    printing_offset.x = 0;
    printing_offset.y = 0;

    phys_page_size.x = page_size.x;
    phys_page_size.y = page_size.y;

    margin.left = printing_offset.x;
    margin.top = printing_offset.y;
    margin.right = phys_page_size.x - page_size.x - margin.left;
    margin.bottom = phys_page_size.y - page_size.y - margin.top;

    lrm = MAX(margin.left, margin.right);
    tbm = MAX(margin.top, margin.bottom);
#endif /* OS */

    zero_struct_ptr(p_redraw_context);

    zero_struct(redraw_context_cache);
    p_redraw_context->p_redraw_context_cache = &redraw_context_cache;

    p_redraw_context->flags.metafile = TRUE;

    p_redraw_context->windows.paintstruct.hdc = hdc;
    p_redraw_context->windows.paintstruct.fErase = FALSE;
    // rcPaint set up shortly, rest of paintstruct is Windows internal garbage

    // DC that comes back is where the printer wants it, so if we want printable area to
    // start further down(across) the page we obviously have to arrange it so that
    // a +ve offset is added when going to GDI space i.e. our top margin - printer top offset
    // p_redraw_context->gdi_org.y(x) is subtracted from the point so that needs to be -ve in the above case
    {
    PIXIT_POINT tl;

    scale = p_docu->paper_scale;

    tl.x = margin_left_from(&p_docu->page_def, 0);
    tl.y = p_docu->page_def.margin_top;

    offset[0].x = (int) muldiv64(tl.x, scale * pixels_per_inch.x, 100 * PIXITS_PER_INCH); // ignore printing_offset for the mo
    offset[0].y = (int) muldiv64(tl.y, scale * pixels_per_inch.y, 100 * PIXITS_PER_INCH);
    } /*block*/

    p_redraw_context->host_xform.scale.t.x = scale;
    p_redraw_context->host_xform.scale.t.y = scale;
    p_redraw_context->host_xform.scale.b.x = 100;
    p_redraw_context->host_xform.scale.b.y = 100;
    p_redraw_context->host_xform

    p_redraw_context->display_mode = DISPLAY_PRINT_AREA;

    p_redraw_context->border_width.x   = p_redraw_context->border_width.y   =     p_docu->page_def.grid_size;
    p_redraw_context->border_width_2.x = p_redraw_context->border_width_2.y = 2 * p_docu->page_def.grid_size;

    metafile_host_redraw_context_set_host_xform(&p_redraw_context->host_xform);

    host_redraw_context_fillin(p_redraw_context);

    { /* print just the top left page */
    PAGE page_num_x = 0;/*1;*/
    PAGE page_num_y = 0;/*1;*/

    p_redraw_context->gdi_org.x = printing_offset.x - offset[0].x;
    p_redraw_context->gdi_org.y = printing_offset.y - offset[0].y;

    p_redraw_context->windows.paintstruct.rcPaint.left = 0;
    p_redraw_context->windows.paintstruct.rcPaint.top = 0;
    p_redraw_context->windows.paintstruct.rcPaint.right = page_size.x;
    p_redraw_context->windows.paintstruct.rcPaint.bottom = page_size.y;

    p_redraw_context->windows.clip_rect = p_redraw_context->windows.paintstruct.rcPaint;

    { // really do clip to this now
    HRGN h_clip_region = CreateRectRgn(p_redraw_context->windows.clip_rect.left, p_redraw_context->windows.clip_rect.top,  p_redraw_context->windows.clip_rect.right,  p_redraw_context->windows.clip_rect.bottom);
    if(NULL != h_clip_region)
    {
        SelectClipRgn(p_redraw_context->windows.paintstruct.hdc, h_clip_region);
        DeleteRgn(h_clip_region);
    }
    } /*block*/

    /*if (1)*/
    {
        PIXIT_RECT print_area;
        SKELEVENT_REDRAW skelevent_redraw;

        print_area.tl.x = 0;
        print_area.tl.y = 0;
        print_area.br.x = p_docu->page_def.size_x - margin_left_from(&p_docu->page_def, page_num_y) - margin_right_from(&p_docu->page_def, page_num_y);
        print_area.br.y = p_docu->page_def.size_y - p_docu->page_def.margin_top - p_docu->page_def.margin_bottom;

        zero_struct(skelevent_redraw);

        /*skelevent_redraw.flags = REDRAW_FLAGS_INIT;*/
        skelevent_redraw.flags.show_content = TRUE;

        skelevent_redraw.redraw_context = redraw_context;

        {
        GDI_RECT gdi_rect;
        PIXIT_RECT pixit_rect;
        gdi_rect.tl.x = p_redraw_context->windows.paintstruct.rcPaint.left   + p_redraw_context->gdi_org.x;
        gdi_rect.tl.y = p_redraw_context->windows.paintstruct.rcPaint.top    + p_redraw_context->gdi_org.y;
        gdi_rect.br.x = p_redraw_context->windows.paintstruct.rcPaint.right  + p_redraw_context->gdi_org.x;
        gdi_rect.br.y = p_redraw_context->windows.paintstruct.rcPaint.bottom + p_redraw_context->gdi_org.y;
        pixit_rect_from_window_rect(&pixit_rect, &gdi_rect, &p_redraw_context->host_xform);
        skelevent_redraw.clip_skel_rect.tl.pixit_point = pixit_rect.tl;
        skelevent_redraw.clip_skel_rect.br.pixit_point = pixit_rect.br;
        } /*block*/

        skelevent_redraw.clip_skel_rect.tl.page_num.x = page_num_x;
        skelevent_redraw.clip_skel_rect.tl.page_num.y = page_num_y;
        skelevent_redraw.clip_skel_rect.br.page_num.x = page_num_x;
        skelevent_redraw.clip_skel_rect.br.page_num.y = page_num_y;

        host_paint_start(&skelevent_redraw.redraw_context);
        view_redraw_page(p_docu, &skelevent_redraw, &print_area);
        host_paint_end(&skelevent_redraw.redraw_context);
    } /*block*/
    } /*block*/

    } /*fi*/

    if(NULL != hdc) DeleteEnhMetaFile(CloseEnhMetaFile(hdc));

    UNREFERENCED_PARAMETER(p_t5_cmd);

    return(status);
}

#endif

#include "cmodules/gr_diag.h"

/* drawfile equivalent of host_redraw_context_set_host_xform() */

static void
drawfile_host_redraw_context_set_host_xform(
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/)
{
#if RISCOS
    p_host_xform->riscos.dx = 2; /* cast in stone somewhere! */
    p_host_xform->riscos.dy = 2;

    p_host_xform->riscos.XEigFactor = 1;
    p_host_xform->riscos.YEigFactor = 1;
#elif WINDOWS
    p_host_xform->windows.pixels_per_inch.x = 
    p_host_xform->windows.pixels_per_inch.y = 96;

    p_host_xform->windows.d.x = muldiv64(p_host_xform->windows.pixels_per_inch.x, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);
    p_host_xform->windows.d.y = muldiv64(p_host_xform->windows.pixels_per_inch.y, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);

    p_host_xform->windows.multiplier_of_pixels.x = PIXITS_PER_METRE * p_host_xform->scale.b.x;
    p_host_xform->windows.multiplier_of_pixels.y = PIXITS_PER_METRE * p_host_xform->scale.b.y;

    p_host_xform->windows.divisor_of_pixels.x = p_host_xform->windows.d.x * p_host_xform->scale.t.x;
    p_host_xform->windows.divisor_of_pixels.y = p_host_xform->windows.d.y * p_host_xform->scale.t.y;
#endif /* OS */
}

/* c.f. view_rect_from_screen_rect_and_context */

#if RISCOS

static void
skel_rect_from_print_box(
    _InoutRef_  P_SKEL_RECT p_skel_rect /*page_num untouched*/,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_GDI_BOX p_print_box)
{
    GDI_RECT gdi_rect;
    PIXIT_RECT pixit_rect;

    gdi_rect.tl.x = (p_print_box->x0 - p_redraw_context->gdi_org.x);
    gdi_rect.br.y = (p_print_box->y0 - p_redraw_context->gdi_org.y);
    gdi_rect.br.x = (p_print_box->x1 - p_redraw_context->gdi_org.x);
    gdi_rect.tl.y = (p_print_box->y1 - p_redraw_context->gdi_org.y);

#elif WINDOWS

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

#endif /* OS */

    pixit_rect_from_window_rect(&pixit_rect, &gdi_rect, &p_redraw_context->host_xform);

    p_skel_rect->tl.pixit_point.x = pixit_rect.tl.x;
    p_skel_rect->tl.pixit_point.y = pixit_rect.tl.y;
    p_skel_rect->br.pixit_point.x = pixit_rect.br.x;
    p_skel_rect->br.pixit_point.y = pixit_rect.br.y;
}

static PAGE page_min_x, page_min_y;
static PAGE page_max_x, page_max_y;

_Check_return_
static STATUS
save_as_drawfile_one_document_page(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ARRAY_HANDLE h_page_list,
    _InVal_     ARRAY_INDEX page_ident,
    P_GR_RISCDIAG p_gr_riscdiag_saving,
    P_GR_RISCDIAG p_gr_riscdiag_lookup)
{
    REDRAW_CONTEXT_CACHE redraw_context_cache;
    REDRAW_CONTEXT redraw_context;
    const P_REDRAW_CONTEXT p_redraw_context = &redraw_context;
    GDI_POINT page_size;
    GDI_POINT offset[1]; /* never two-up */
    S32 scale;
    STATUS status = STATUS_OK;

#if RISCOS
    static const GDI_POINT pixels_per_inch = { 180, 180 }; /* actually OS units here */
#elif WINDOWS
    static const GDI_POINT pixels_per_inch = { 600, 600 }; /* like a typical medium-res printer */
#endif

    zero_struct_ptr(p_redraw_context);

    zero_struct(redraw_context_cache);
    p_redraw_context->p_redraw_context_cache = &redraw_context_cache;

    p_redraw_context->flags.drawfile = 1;

    p_redraw_context->p_gr_riscdiag = p_gr_riscdiag_saving;
    p_redraw_context->lookup_gr_riscdiag = p_gr_riscdiag_lookup;

    { /* never two-up */
        PIXIT_POINT tl;

        scale = p_docu->paper_scale;

        tl.x = margin_left_from(&p_docu->page_def, 0);
        tl.y = p_docu->page_def.margin_top;

        offset[0].x = (int) muldiv64(tl.x, scale * pixels_per_inch.x, 100 * PIXITS_PER_INCH); // ignore printing_offset for the mo
        offset[0].y = (int) muldiv64(tl.y, scale * pixels_per_inch.y, 100 * PIXITS_PER_INCH);

        page_size.x = (int) muldiv64(p_docu->phys_page_def.size_x, pixels_per_inch.x, PIXITS_PER_INCH); /* not scaled - host_xform scales coords */
        page_size.y = (int) muldiv64(p_docu->phys_page_def.size_y, pixels_per_inch.y, PIXITS_PER_INCH);
    } /*block*/

    p_redraw_context->host_xform.scale.t.x = scale;
    p_redraw_context->host_xform.scale.t.y = scale;
    p_redraw_context->host_xform.scale.b.x = 100;
    p_redraw_context->host_xform.scale.b.y = 100;

    p_redraw_context->display_mode = DISPLAY_PRINT_AREA;

    p_redraw_context->border_width.x = p_redraw_context->border_width.y = p_docu->page_def.grid_size;
    p_redraw_context->border_width_2.x = p_redraw_context->border_width_2.y = 2 * p_docu->page_def.grid_size;

    drawfile_host_redraw_context_set_host_xform(&p_redraw_context->host_xform);

    host_redraw_context_fillin(p_redraw_context);

    if(array_index_is_valid(&h_page_list, page_ident))
    {
        const PC_PAGE_ENTRY p_page_entry = array_ptrc_no_checks(&h_page_list, PAGE_ENTRY, page_ident);

        p_redraw_context->gdi_org.x = 0; //- offset[0].x;
        p_redraw_context->gdi_org.y = 0; //- offset[0].y;

#if RISCOS
        p_redraw_context->riscos.host_machine_clip_box.x0 = 0;
        p_redraw_context->riscos.host_machine_clip_box.y0 = -page_size.y;
        p_redraw_context->riscos.host_machine_clip_box.x1 = +page_size.x;
        p_redraw_context->riscos.host_machine_clip_box.y1 = 0;
#elif WINDOWS
        p_redraw_context->windows.paintstruct.rcPaint.left   = 0;
        p_redraw_context->windows.paintstruct.rcPaint.top    = 0;
        p_redraw_context->windows.paintstruct.rcPaint.right  = page_size.x;
        p_redraw_context->windows.paintstruct.rcPaint.bottom = page_size.y;

        p_redraw_context->windows.host_machine_clip_rect = p_redraw_context->windows.paintstruct.rcPaint;

        /* no actual GDI clipping region required to be set */
#endif /* OS */

        /* I tried to use origin but didn't work. Added page_pixit_origin_draw instead. */
        p_redraw_context->page_pixit_origin_draw.x = + ((p_page_entry->page.x - page_min_x) * muldiv64(p_docu->page_def.size_x, scale, 100));

        p_redraw_context->page_pixit_origin_draw.y = - ((p_page_entry->page.y - page_min_y) * muldiv64(p_docu->page_def.size_y, scale, 100));

#if 1
        /* All pages above y = 0.  Last page is just above */
        p_redraw_context->page_pixit_origin_draw.y += (page_max_y - page_min_y + 1) * muldiv64(p_docu->page_def.size_y, scale, 100);
#else
        /* First y page stands up from y = 0, rest are rendered below */
        p_redraw_context->page_pixit_origin_draw.y += p_docu->page_def.size_y;
#endif

        if((p_page_entry->page.x >= 0) && (p_page_entry->page.y >= 0))
        {
            PIXIT_RECT print_area;
            SKELEVENT_REDRAW skelevent_redraw;

            print_area.tl.x = 0;
            print_area.tl.y = 0;
            print_area.br.x = p_docu->page_def.size_x - margin_left_from(&p_docu->page_def, p_page_entry->page.y) - margin_right_from(&p_docu->page_def, p_page_entry->page.y);
            print_area.br.y = p_docu->page_def.size_y - p_docu->page_def.margin_top  - p_docu->page_def.margin_bottom;

            zero_struct(skelevent_redraw);

            /*skelevent_redraw.flags = REDRAW_FLAGS_INIT;*/
            skelevent_redraw.flags.show_content = TRUE;

            skelevent_redraw.redraw_context = redraw_context;

#if RISCOS
            { /* the printer driver lies through its teeth, so enlarge the clip rectangle it supplied by some small amount */
            GDI_BOX bigger_clip;

            bigger_clip.x0 = p_redraw_context->riscos.host_machine_clip_box.x0 - 8;
            bigger_clip.y0 = p_redraw_context->riscos.host_machine_clip_box.y0 - 8;
            bigger_clip.x1 = p_redraw_context->riscos.host_machine_clip_box.x1 + 8;
            bigger_clip.y1 = p_redraw_context->riscos.host_machine_clip_box.y1 + 8; /* was 16 */

            skel_rect_from_print_box(&skelevent_redraw.clip_skel_rect, p_redraw_context, &bigger_clip);
            } /*block*/
#elif WINDOWS
            skel_rect_from_print_box(&skelevent_redraw.clip_skel_rect, p_redraw_context);
#endif

            skelevent_redraw.clip_skel_rect.tl.page_num = p_page_entry->page;
            skelevent_redraw.clip_skel_rect.br.page_num = p_page_entry->page;

            { /* create a group object to contain this page's objects */
            U8Z groupName[16];
            DRAW_DIAG_OFFSET groupStart;

            if(page_min_x != page_max_x)
                consume_int(xsnprintf(groupName, sizeof32(groupName), "Page " S32_FMT "." S32_FMT, p_page_entry->page.x + 1, p_page_entry->page.y + 1));
            else
                consume_int(xsnprintf(groupName, sizeof32(groupName), "Page " S32_FMT, p_page_entry->page.y + 1));

            if(status_ok(status = gr_riscdiag_group_new(p_gr_riscdiag_saving, &groupStart, groupName)))
            {
                /* based on send_redrawevent_to_skel in c.view */
                /* from first switch statement, when p_view->display_mode is DISPLAY_PRINT_AREA */

                /* in second switch statement, when p_view->display_mode is DISPLAY_PRINT_AREA */
                /* do the same header_area, footer_area, row_area, col_area, work_area calculations */
                /* so replace by a proc */

                host_paint_start(&skelevent_redraw.redraw_context);
                view_redraw_page(p_docu, &skelevent_redraw, &print_area);
                host_paint_end(&skelevent_redraw.redraw_context);

                /* complete this page's group object */
                gr_riscdiag_group_end(p_gr_riscdiag_saving, groupStart);
            }
            } /*block*/
        }
    }

    return(status);
}

_Check_return_
static STATUS
save_as_drawfile_host_print_document_core(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_PRINT_CTRL p_print_ctrl,
    P_GR_RISCDIAG p_gr_riscdiag_saving,
    P_GR_RISCDIAG p_gr_riscdiag_lookup)
{
    STATUS status = STATUS_OK;
    const ARRAY_INDEX page_count = array_elements(&p_print_ctrl->h_page_list);
    ARRAY_INDEX page_index, docu_pages_per_printer_page;
    PRINTER_PERCENTAGE printer_percentage;

    docu_pages_per_printer_page = 1; /* probably ... as we don't emulate two up etc. */

    save_as_drawfile_percentage_initialise(p_docu, &printer_percentage, page_count);

    /* find the minimum and maximum (non-blank) pages present */
    page_min_x = S32_MAX;
    page_min_y = S32_MAX;
    page_max_x = -1;
    page_max_y = -1;

    for(page_index = 0; page_index < page_count; page_index += 1/*docu_pages_per_printer_page*/)
    {
        const PC_PAGE_ENTRY p_page_entry = array_ptrc(&p_print_ctrl->h_page_list, PAGE_ENTRY, page_index);

        if((p_page_entry->page.x >= 0) && (p_page_entry->page.y >= 0))
        {
            if(page_min_x > p_page_entry->page.x) page_min_x = p_page_entry->page.x;
            if(page_min_y > p_page_entry->page.y) page_min_y = p_page_entry->page.y;
            if(page_max_x < p_page_entry->page.x) page_max_x = p_page_entry->page.x;
            if(page_max_y < p_page_entry->page.y) page_max_y = p_page_entry->page.y;
        }
    }

    for(page_index = 0; page_index < page_count; page_index += docu_pages_per_printer_page)
    {
        status_break(status = save_as_drawfile_one_document_page(p_docu, p_print_ctrl->h_page_list, page_index, p_gr_riscdiag_saving, p_gr_riscdiag_lookup));

        save_as_drawfile_percentage_page_inc(&printer_percentage);
    }

    save_as_drawfile_percentage_finalise(&printer_percentage);

    status_return(status);

    if(p_gr_riscdiag_saving->dd_options)
    {   /* adjust options for paper size etc. */
        P_BYTE pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag_saving, p_gr_riscdiag_saving->dd_options);
        DRAW_OBJECT_OPTIONS options;

        memcpy32(&options, pObject, sizeof32(options));

        /* half-hearted stab at an improvement */
        if(((page_max_y + 1) - page_min_y) < 2)
            options.paper_size = (4 + 1) << 8; /* A4 */
        else if(((page_max_y + 1) - page_min_y) < 4)
            options.paper_size = (2 + 1) << 8; /* A2 */
        else
            options.paper_size = (0 + 1) << 8; /* A0 */

        options.paper_limits.landscape = p_print_ctrl->flags.landscape;

        memcpy32(pObject, &options, sizeof32(options));
    }

    return(status);
}

_Check_return_
static STATUS
save_as_drawfile_drawfile_initialise(
    _OutRef_    P_P_GR_RISCDIAG p_p_gr_riscdiag_saving,
    _OutRef_    P_P_GR_RISCDIAG p_p_gr_riscdiag_lookup,
    _InVal_     T5_FILETYPE t5_filetype)
{
    const PC_SBSTR creator_name = (FILETYPE_T5_DRAW == t5_filetype) ? "FwkzHybrid" : "FwkzExport";
    STATUS status = STATUS_OK;

    *p_p_gr_riscdiag_lookup = NULL; /* well done code analysis! */

    status_return(status = gr_riscdiag_diagram_new(p_p_gr_riscdiag_saving, creator_name, 0, 0, TRUE /*options*/));
    status_return(status = gr_riscdiag_diagram_new(p_p_gr_riscdiag_lookup, "FwkzLookup", 0, 0, FALSE /*options*/));

    PTR_ASSERT(*p_p_gr_riscdiag_lookup);
    { /* create a font list object header in the lookup riscdiag to which we can add - this will be inserted in the saving riscdiag at the end */
    P_GR_RISCDIAG p_gr_riscdiag_lookup = *p_p_gr_riscdiag_lookup;
    DRAW_DIAG_OFFSET fontListStart = gr_riscdiag_query_offset(p_gr_riscdiag_lookup);

    P_BYTE pObject;
    DRAW_OBJECT_HEADER_NO_BBOX objhdr; /* NB this object doesn't have a bounding box */

    objhdr.type = DRAW_OBJECT_TYPE_FONTLIST;
    objhdr.size = sizeof32(objhdr);

    if(NULL != (pObject = gr_riscdiag_ensure(BYTE, p_gr_riscdiag_lookup, objhdr.size, &status)))
    {
        memcpy32(pObject, &objhdr, sizeof32(objhdr));

        p_gr_riscdiag_lookup->dd_fontListR = fontListStart;
    }
    } /*block*/

    return(status);
}

_Check_return_
static STATUS
save_as_drawfile_drawfile_finalise(
    _InoutRef_  P_P_GR_RISCDIAG p_p_gr_riscdiag_saving,
    _InoutRef_  P_P_GR_RISCDIAG p_p_gr_riscdiag_lookup,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype)
{
    P_GR_RISCDIAG p_gr_riscdiag_saving = *p_p_gr_riscdiag_saving;
    P_GR_RISCDIAG p_gr_riscdiag_lookup = *p_p_gr_riscdiag_lookup;
    STATUS status = STATUS_OK;

    /* complete the lookup riscdiag's font object - pad up to 3 bytes with 0 */
    if(0 != p_gr_riscdiag_lookup->draw_diag.length)
    {
        U32 pad_bytes = ((p_gr_riscdiag_lookup->draw_diag.length + (4-1)) & ~(4-1)) - p_gr_riscdiag_lookup->draw_diag.length;
        if(pad_bytes)
        {
            P_BYTE p_u8;
            assert(pad_bytes < 4);
            if(NULL != (p_u8 = gr_riscdiag_ensure(BYTE, p_gr_riscdiag_lookup, 4/*pad_bytes - no, must be multiple of 4*/, &status)))
            {
                memset32(p_u8, 0, pad_bytes);
                /* but now truncate at the correct length */
                p_gr_riscdiag_lookup->draw_diag.length -= (4 - pad_bytes);
                /* and adjust the font object header too */
                p_u8 = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag_lookup, p_gr_riscdiag_lookup->dd_fontListR);
                ((P_DRAW_OBJECT_HEADER_NO_BBOX) p_u8)->size += pad_bytes;
            }
        }
    }

    if((0 != p_gr_riscdiag_saving->draw_diag.length) && (0 != p_gr_riscdiag_lookup->dd_fontListR))
    {
        /* insert the lookup riscdiag's font object in saving riscdiag prior to ending */
        S32 saving_diag_contents_size = p_gr_riscdiag_saving->draw_diag.length - sizeof32(DRAW_FILE_HEADER);
        const S32 font_object_size = gr_riscdiag_getoffptr(DRAW_OBJECT_HEADER_NO_BBOX, p_gr_riscdiag_lookup, p_gr_riscdiag_lookup->dd_fontListR)->size;

        if(NULL != gr_riscdiag_ensure(BYTE, p_gr_riscdiag_saving, font_object_size, &status))
        {
            P_BYTE p_u8 = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag_saving, sizeof32(DRAW_FILE_HEADER));
            /* move what's there already up out of the way, leaving a hole ready */
            memmove32(p_u8 + font_object_size, p_u8, saving_diag_contents_size);

            /* and insert a copy of the font object from the lookup riscdiag */
            memcpy32(p_u8,
                     gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag_lookup, p_gr_riscdiag_lookup->dd_fontListR),
                     font_object_size);

            /* we've now acquired a font object for lookup! */
            p_gr_riscdiag_saving->dd_fontListR = sizeof32(DRAW_FILE_HEADER);

            /* also the options object (if there is one) will have moved! */
            if(0 != p_gr_riscdiag_saving->dd_options)
                p_gr_riscdiag_saving->dd_options += font_object_size;

            /* also the root group (if there is one) will have moved! */
            if(0 != p_gr_riscdiag_saving->dd_rootGroupStart)
                p_gr_riscdiag_saving->dd_rootGroupStart += font_object_size;
        }
    }

    if(0 != p_gr_riscdiag_saving->draw_diag.length)
    {
        { /* always set sensible bbox before shifting it */
        GR_RISCDIAG_PROCESS_T process;
        * (int *) &process = 0;
        process.recurse = 1;
        gr_riscdiag_diagram_reset_bbox(p_gr_riscdiag_saving, process);
        } /*block*/

        /* which will itself set sensible bbox again */
        gr_riscdiag_diagram_end(p_gr_riscdiag_saving);

        status = gr_riscdiag_diagram_save(p_gr_riscdiag_saving, filename, t5_filetype);
    }

    /* and finally tidy up */
    gr_riscdiag_diagram_dispose(p_p_gr_riscdiag_saving);
    gr_riscdiag_diagram_dispose(p_p_gr_riscdiag_lookup);

    return(status);
}

_Check_return_
extern STATUS
save_as_drawfile_host_print_document(
    _DocuRef_   P_DOCU p_docu,
    P_PRINT_CTRL p_print_ctrl,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status = STATUS_OK;
    P_GR_RISCDIAG p_gr_riscdiag_saving, p_gr_riscdiag_lookup;

    status_return(status = save_as_drawfile_drawfile_initialise(&p_gr_riscdiag_saving, &p_gr_riscdiag_lookup, t5_filetype));

    status = save_as_drawfile_host_print_document_core(p_docu, p_print_ctrl, p_gr_riscdiag_saving, p_gr_riscdiag_lookup);

    status = save_as_drawfile_drawfile_finalise(&p_gr_riscdiag_saving, &p_gr_riscdiag_lookup, filename, t5_filetype);

    return(status);
}

/* end of drwprint.c */
