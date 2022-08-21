/* sk_form.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Fireworkz formatting */

/* MRJC December 1991 */

#include "common/gflags.h"

#ifndef    __cells_ob_cells_h
#include "ob_cells/ob_cells.h"
#endif

/*
callbacks
*/

PROC_EVENT_PROTO(static, null_event_rowtab_format);

#define ROWTAB_NULL_CLIENT_HANDLE ((CLIENT_HANDLE) 0x00000003)

/*
internal routines
*/

static void
format_row_move_y(
    _DocuRef_   P_DOCU p_docu,
    P_SKEL_POINT p_skel_point,
    _InVal_     ARRAY_HANDLE h_col_info_in /* can be zero */,
    _InoutRef_  P_ROW_INFO p_row_info);

/******************************************************************************
*
* flags.new_extent - done as a null event handler now.
*
******************************************************************************/

#define DNE_NULL_CLIENT_HANDLE ((CLIENT_HANDLE) 0x00000002)

#define TRACE_DNE_NULL 0

T5_MSG_PROTO(static, docu_new_extent_null, P_NULL_EVENT_BLOCK p_null_event_block)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER(p_null_event_block);

    trace_1(TRACE_DNE_NULL, TEXT("docu_new_extent_null(docno=") DOCNO_TFMT TEXT("): null_event"), docno_from_p_docu(p_docu));

    assert(p_docu->flags.new_extent);

    docu_set_and_show_extent_all_views(p_docu);

    return(STATUS_OK);
}

PROC_EVENT_PROTO(static, null_event_docu_new_extent)
{
#if CHECKING
    switch(t5_message)
    {
    default: default_unhandled();
        return(STATUS_OK);

    case T5_EVENT_NULL:
#else
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    {
#endif
        return(docu_new_extent_null(p_docu, t5_message, (P_NULL_EVENT_BLOCK) p_data));
    }
}

extern void
docu_flags_new_extent_clear(
    _DocuRef_   P_DOCU p_docu)
{
    if(!p_docu->flags.new_extent)
        return;

    p_docu->flags.new_extent = 0;

    trace_1(TRACE_DNE_NULL, TEXT("docu_flags_new_extent_clear(docno=") DOCNO_TFMT TEXT(") - *** null_events_stop()"), docno_from_p_docu(p_docu));
    null_events_stop(docno_from_p_docu(p_docu), T5_EVENT_NULL, null_event_docu_new_extent, DNE_NULL_CLIENT_HANDLE);
}

extern void
docu_flags_new_extent_set(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->flags.new_extent)
        return;

    p_docu->flags.new_extent = 1;

    trace_1(TRACE_DNE_NULL, TEXT("docu_flags_new_extent_set(docno=") DOCNO_TFMT TEXT(") - *** null_events_start()"), docno_from_p_docu(p_docu));
    status_assert(null_events_start(docno_from_p_docu(p_docu), T5_EVENT_NULL, null_event_docu_new_extent, DNE_NULL_CLIENT_HANDLE));
}

/******************************************************************************
*
* ask an object how big it is
*
******************************************************************************/

extern void
format_object_how_big(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_HOW_BIG p_object_how_big)
{
    if(object_present(p_object_how_big->object_data.object_id))
    {
        status_consume(cell_call_id(p_object_how_big->object_data.object_id, p_docu, T5_MSG_OBJECT_HOW_BIG, p_object_how_big, NO_CELLS_EDIT));
    }
    else
    { /* when object is not present - we'll guess at the object size */
        STYLE style;
        style_init(&style);
        style_from_slr(p_docu, &style, &style_selector_para_format, &p_object_how_big->object_data.data_ref.arg.slr);

        p_object_how_big->skel_rect.br.pixit_point.x = p_object_how_big->skel_rect.tl.pixit_point.x
                                                     + style.col_style.width;
        p_object_how_big->skel_rect.br.pixit_point.y = p_object_how_big->skel_rect.tl.pixit_point.y
                                                     + style_leading_from_style(&style, &style.font_spec, p_docu->flags.draft_mode)
                                                     + style.para_style.para_start
                                                     + style.para_style.para_end;
    }
}

/******************************************************************************
*
* calculate information about pages when there's a virtual row table
*
******************************************************************************/

static void
virtual_page_info_read(
    _DocuRef_   P_DOCU p_docu)
{
    { /* page size is derived from row zero's header & footer settings */
    HEADFOOT_BOTH headfoot_both;
    headfoot_both_from_row_and_page_y(p_docu, &headfoot_both, 0, 0);
    p_docu->virtual_page_info.page_height = p_docu->page_def.cells_usable_y - headfoot_both.header.margin - headfoot_both.footer.margin;
    p_docu->virtual_page_info.page_height = MAX(PIXITS_PER_INCH, p_docu->virtual_page_info.page_height); /* SKS 04apr96 don't let this go stupid */
    } /*block*/

    { /* row height is read from row zero */
    STYLE style;
    style_init(&style);
    style_from_row(p_docu, &style, &style_selector_row, 0);
    p_docu->virtual_page_info.row_height = MIN(style.row_style.height, p_docu->virtual_page_info.page_height);
    if(0 == p_docu->virtual_page_info.row_height) /* SKS 04apr96 don't let this go stupid */
    {   /* e.g. Letter-based document */
        p_docu->virtual_page_info.row_height = 256; /* slightly taller than PIXITS_PER_INCH / 6 but makes for much faster division*/
    }
    } /*block*/

    p_docu->virtual_page_info.rows_per_page = p_docu->virtual_page_info.page_height / p_docu->virtual_page_info.row_height;
    p_docu->virtual_page_info.rows_per_page = MAX(1, p_docu->virtual_page_info.rows_per_page); /* SKS 04apr96 don't let this go stupid */
}

/******************************************************************************
*
* change the virtual row table flag and therefore the document format
*
******************************************************************************/

_Check_return_
extern STATUS
virtual_row_table_set(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL new_vrt)
{
    STATUS status = STATUS_OK;

    trace_2(TRACE_OUT | TRACE_ANY, TEXT("virtual_row_table_set(docno=") DOCNO_TFMT TEXT(", vrt=%s)"), docno_from_p_docu(p_docu), report_boolstring(new_vrt));

    if((BOOL) p_docu->flags.virtual_row_table != new_vrt)
    {
        ROW rows = n_rows(p_docu);
        p_docu->flags.virtual_row_table = new_vrt;
        if(p_docu->flags.virtual_row_table)
            virtual_page_info_read(p_docu);
        if(0 == rows) /* SKS 11jun96 we can never have zero rows in a document */
            rows = 1;
        if(status_fail(status = n_rows_set(p_docu, rows)))
        {
            p_docu->flags.virtual_row_table = 1;
            virtual_page_info_read(p_docu);
            status_assert(n_rows_set(p_docu, rows));
        }
        reformat_all(p_docu);
    }

    return(status);
}

/******************************************************************************
*
* check about y extent needing update
*
******************************************************************************/

static void
extent_y_check(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row,
    _InVal_     PAGE page)
{
    const PAGE old_page_y = p_docu->_last_page_y;

    if((row + 1) == n_rows(p_docu))
        p_docu->_last_page_y = page + 1;
    else
        p_docu->_last_page_y = MAX(p_docu->_last_page_y, page + 1);

    /* mark document for LATER extent changes */
    if(old_page_y == p_docu->_last_page_y)
        return;

    docu_flags_new_extent_set(p_docu);
}

/******************************************************************************
*
* set up for reformat below a given row
*
******************************************************************************/

static void
format_below_row(
    _DocuRef_   P_DOCU p_docu,
    _In_        ROW row)
{
    if(!p_docu->flags.has_data)
        return;

    if(row >= p_docu->format_start_row)
        /* row is below the current format_start_row value so will be formatted anyway */
        return;

    row = MIN(row, n_rows(p_docu) - 1);

#if !TRACE_ALLOWED
    p_docu->format_start_row = row;
#else
    {
    ROW prev_format_start_row = p_docu->format_start_row;

    p_docu->format_start_row = row;

    if(p_docu->flags.null_event_rowtab_format_started)
    {   /* already running - null event will pick up this new format_start_row value */
        trace_3(TRACE_OUT | TRACE_ANY, TEXT("format_below_row(docno=") DOCNO_TFMT TEXT(", row=") ROW_TFMT TEXT(") - prev format_start_row ") ROW_TFMT TEXT(" - *** null_events already running"), docno_from_p_docu(p_docu), row, prev_format_start_row);
    }
    else
    {
        trace_3(TRACE_OUT | TRACE_ANY, TEXT("format_below_row(docno=") DOCNO_TFMT TEXT(", row=") ROW_TFMT TEXT(") - prev format_start_row ") ROW_TFMT TEXT(" - *** null_events_start()"), docno_from_p_docu(p_docu), row, prev_format_start_row);
    }
    } /*block*/
#endif /* TRACE_ALLOWED */

    if(!p_docu->flags.null_event_rowtab_format_started)
    {   /* ensure background formatter started */
        if(status_ok(status_wrap(null_events_start(docno_from_p_docu(p_docu), T5_EVENT_NULL, null_event_rowtab_format, ROWTAB_NULL_CLIENT_HANDLE))))
            p_docu->flags.null_event_rowtab_format_started = TRUE;
    }

    status_assert(maeve_event(p_docu, T5_MSG_ROW_MOVE_Y, &row));

    if((0 != row) && row_is_visible(p_docu, row))
    {
        SKEL_RECT skel_rect;

        trace_2(TRACE_OUT | TRACE_ANY, TEXT("format_below_row(docno=") DOCNO_TFMT TEXT(", row=") ROW_TFMT TEXT(") visible"), docno_from_p_docu(p_docu), row);

        {
        ROW_ENTRY row_entry;
        row_entry_from_row(p_docu, &row_entry, row ? row - 1 : 0 /* avoid reformat as a result of this call */);
        skel_point_update_from_edge(&skel_rect.tl, row ? &row_entry.rowtab.edge_bot : &row_entry.rowtab.edge_top, y);
        } /*block*/

        skel_rect.tl.pixit_point.x = 0;
        skel_rect.tl.page_num.x = 0;
        skel_rect.br = skel_rect.tl;

        {
        RECT_FLAGS rect_flags;
        RECT_FLAGS_CLEAR(rect_flags);
        rect_flags.extend_right_window = rect_flags.extend_down_window = 1;
        view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);
        view_update_later(p_docu, UPDATE_PANE_MARGIN_ROW, &skel_rect, rect_flags);
        view_update_later(p_docu, UPDATE_BORDER_VERT, &skel_rect, rect_flags);
        view_update_later(p_docu, UPDATE_RULER_VERT, &skel_rect, rect_flags);
        } /*block*/

        view_update_all(p_docu, UPDATE_BORDER_HORZ);
        view_update_all(p_docu, UPDATE_RULER_HORZ);
    }
    else
    {
        trace_3(TRACE_OUT | TRACE_ANY, TEXT("format_below_row(docno=") DOCNO_TFMT TEXT(", row=") ROW_TFMT TEXT(") %s"), docno_from_p_docu(p_docu), row, (row == 0) ? TEXT(" ALL") : TEXT(" invisible"));

        view_update_all(p_docu, UPDATE_PANE_CELLS_AREA);
        view_update_all(p_docu, UPDATE_PANE_MARGIN_ROW);
        view_update_all(p_docu, UPDATE_BORDER_HORZ);
        view_update_all(p_docu, UPDATE_BORDER_VERT);

        view_update_all(p_docu, UPDATE_RULER_HORZ);
        view_update_all(p_docu, UPDATE_RULER_VERT);
    }
}

/******************************************************************************
*
* set up a row info block from a given row
*
******************************************************************************/

static void
row_info_from_row__virtual_row_table(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ROW_INFO p_row_info,
    _InVal_     ROW row)
{
    p_row_info->row = row;
    p_row_info->height = p_docu->virtual_page_info.row_height;
    p_row_info->height_fixed = TRUE;
    p_row_info->unbreakable = TRUE;
    p_row_info->page = row / p_docu->virtual_page_info.rows_per_page;
    p_row_info->page_height = p_docu->virtual_page_info.page_height;
}

static void
row_info_from_row__normal_row_table(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_ROW_INFO p_row_info,
    _InVal_     ROW row)
{
    P_ROWTAB p_rowtab;

    if(p_row_info->row != row)
    {
        STYLE style;

        p_row_info->row = row;

        style_init(&style);
        style_from_row(p_docu, &style, &style_selector_row, row);

        p_row_info->height = style.row_style.height;
        p_row_info->height_fixed = style.row_style.height_fixed;
        p_row_info->unbreakable = style.row_style.unbreakable;

        style_dispose(&style);
    }

    p_rowtab = array_ptr(&p_docu->h_rowtab, ROWTAB, row);

    if(p_row_info->page != p_rowtab->edge_top.page)
    {
        ROW row_t;
        HEADFOOT_BOTH headfoot_both;

        p_row_info->page = p_rowtab->edge_top.page;

        row_t = row;

        /* find row at the top of this page */
        while(row_t)
        {
            if(p_rowtab[-1].edge_top.page != p_row_info->page)
                break;
            --row_t;
            --p_rowtab;
        }

        /* read page style at top of page */
        headfoot_both_from_row_and_page_y(p_docu, &headfoot_both, row_t, p_row_info->page);
        p_row_info->page_height = p_docu->page_def.cells_usable_y - headfoot_both.header.margin - headfoot_both.footer.margin;
        p_row_info->page_height = MAX(0, p_row_info->page_height);
    }
}

static void
row_info_from_row(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_ROW_INFO p_row_info,
    _InVal_     ROW row)
{
    if(p_docu->flags.virtual_row_table)
        row_info_from_row__virtual_row_table(p_docu, p_row_info, row);
    else
        row_info_from_row__normal_row_table(p_docu, p_row_info, row);
}

/******************************************************************************
*
* set logical column and row extents for a document
*
******************************************************************************/

_Check_return_
extern STATUS
format_col_row_extents_set(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL n_cols,
    _InVal_     ROW n_rows)
{
    COL old_logical = p_docu->cols_logical;

    status_return(n_rows_set(p_docu, n_rows));

    p_docu->cols_logical = n_cols;

    if(n_cols != old_logical)
        page_x_extent_set(p_docu);

    return(STATUS_OK);
}

/******************************************************************************
*
* set the object size
*
******************************************************************************/

static S32 /* row moved or changed height */
format_object_size_change(
    _DocuRef_   P_DOCU p_docu,
    /*inout*/ P_SKEL_RECT p_skel_rect_new /* rectangle now covering object */,
    _InRef_opt_ PC_SKEL_RECT p_skel_rect_old /* old object rectangle */,
    _InRef_     PC_SLR p_slr,
    _InVal_     ARRAY_HANDLE h_col_info /* can be zero */,
    _InoutRef_  P_ROW_INFO p_row_info)
{
    BOOL row_moved = FALSE;

    if(!p_docu->flags.virtual_row_table)
    {
        const P_CELL p_cell = p_cell_from_slr(p_docu, p_slr);
        const P_ROWTAB p_rowtab = array_ptr(&p_docu->h_rowtab, ROWTAB, p_slr->row);
        S32 res;

        /* typically, an object should not need to know about pages; therefore we check here if
         * the object needs moving onto the next page; certain object types may take account
         * of pages themselves: e.g. text objects can distribute themselves over different
         * pages and also move themselves onto different pages
         */
        if(p_skel_rect_new->tl.page_num.y == p_skel_rect_new->br.page_num.y)
        {
            if(p_skel_rect_new->br.pixit_point.y > p_row_info->page_height && p_skel_rect_new->tl.pixit_point.y != 0)
                skel_rect_move_to_next_page(p_skel_rect_new);
        }
        else if(p_row_info->unbreakable && p_skel_rect_new->tl.pixit_point.y != 0)
            skel_rect_move_to_next_page(p_skel_rect_new);

        if(p_rowtab->edge_top.page < p_skel_rect_new->tl.page_num.y)
        {
            /* if the object was moved onto a new page,
             * move the whole row onto a new page
             */
            format_row_move_y(p_docu, &p_skel_rect_new->tl, h_col_info, p_row_info);
            row_moved = 1;
        }
        else
        {
            if(!p_row_info->height_fixed)
            {
                if((res = skel_point_edge_compare(&p_skel_rect_new->br, &p_rowtab->edge_bot, y)) > 0)
                {
                    /* object has grown beyond existing row bottom
                     */
                    edge_set_from_skel_point(&p_rowtab->edge_bot, &p_skel_rect_new->br, y);
                    row_moved = 1;
                }
                else if(res < 0)
                {
                    /* object is smaller than row - was it bigger than the row before ?
                     */
                    if((NULL == p_skel_rect_old)
                       ||
                       skel_point_edge_compare(&p_skel_rect_old->br, &p_rowtab->edge_bot, y) >= 0)
                    {
                        if(!p_cell
                           ||
                           (p_cell->cell_new || p_cell->cell_at_row_bottom))
                        {
                            /* bottom of object is above existing row bottom,
                             * and may previously have been at the row bottom;
                             * is row above minimum height ?
                             */
                            if(p_rowtab->edge_top.page != p_rowtab->edge_bot.page
                               ||
                               p_row_info->height      != p_rowtab->edge_bot.pixit - p_rowtab->edge_top.pixit)
                                row_moved = 1;
                        }
                    }
                }
            }

            /* save current object size */
            if(NULL != p_cell)
            {
                p_cell->cell_new = 0;
                if(!skel_point_edge_compare(&p_skel_rect_new->br, &p_rowtab->edge_bot, y))
                    p_cell->cell_at_row_bottom = 1;
                else
                    p_cell->cell_at_row_bottom = 0;
            }
        }
    }

    return(row_moved);
}

/******************************************************************************
*
* call back from object to adjust object size
*
******************************************************************************/

_Check_return_
extern STATUS /* >0 object - please update yourself */
format_object_size_set(
    _DocuRef_   P_DOCU p_docu,
    _InRef_opt_ PC_SKEL_RECT p_skel_rect_new /* new object rect */,
    _InRef_opt_ PC_SKEL_RECT p_skel_rect_old /* old rectangle */,
    _InRef_     PC_SLR p_slr,
    _InVal_     BOOL object_self_update /* TRUE = object wants to update itself if it can */)
{
    STATUS status = STATUS_OK;
    BOOL reformat_below = 0;

    if(p_slr->row < p_docu->format_start_row)
    {
        SKEL_RECT skel_rect;
        ROW_INFO row_info;

        if(NULL == p_skel_rect_new)
        {
            OBJECT_HOW_BIG object_how_big;

            (void) cell_data_from_slr(p_docu, &object_how_big.object_data, p_slr);
            skel_rect_from_slr(p_docu, &object_how_big.skel_rect, p_slr);
            format_object_how_big(p_docu, &object_how_big);
            skel_rect = object_how_big.skel_rect;
        }
        else
            skel_rect = *p_skel_rect_new;

        row_info.row = -1;
        row_info.page = -1;
        row_info_from_row(p_docu, &row_info, p_slr->row);

        /* store cell size and check for mega updates */
        if(format_object_size_change(p_docu, &skel_rect, p_skel_rect_old, p_slr, 0, &row_info) != 0)
            reformat_below = 1;
    }

    if(reformat_below)
        /* do large reformat when row moves */
        reformat_from_row(p_docu, p_slr->row, REFORMAT_Y);
    else
    {
        /* row has not moved - the object may be able to update itself */
        if(!object_self_update)
        {
            /* redraw whole cell */
            SKEL_RECT skel_rect;

            skel_rect_from_slr(p_docu, &skel_rect, p_slr);

            {
            RECT_FLAGS rect_flags;
            RECT_FLAGS_CLEAR(rect_flags);
            view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);
            } /*block*/

            trace_4(TRACE_APP_FORMAT,
                    TEXT("format_object_size_set tl: ") PIXIT_TFMT TEXT(",") PIXIT_TFMT TEXT(", br: ") PIXIT_TFMT TEXT(",") PIXIT_TFMT,
                    skel_rect.tl.pixit_point.x, skel_rect.tl.pixit_point.y,
                    skel_rect.br.pixit_point.x, skel_rect.br.pixit_point.y);
        }
        else
            status = STATUS_DONE;
    }

    return(status);
}

/******************************************************************************
*
* format a row in the document structure
* stores the bottom right position of the
* object's rectangle in the cell entry
*
******************************************************************************/

static void
format_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ARRAY_HANDLE h_col_info,
    _InoutRef_  P_ROW_INFO p_row_info)
{
    P_ROWTAB p_rowtab;
    S32 page_eject = 0;

    assert(array_elements(&h_col_info));

    p_rowtab = array_ptr(&p_docu->h_rowtab, ROWTAB, p_row_info->row);
    p_rowtab->edge_bot = p_rowtab->edge_top;

    if(p_rowtab->edge_top.pixit != 0 && page_break_row(p_docu, p_row_info->row))
        page_eject = 1;
    else if(p_row_info->height_fixed)
    {
        /* set bottom edge of fixed height row */
        p_rowtab->edge_bot.pixit += p_row_info->height;

        if(p_rowtab->edge_top.pixit != 0 && p_rowtab->edge_bot.pixit > p_row_info->page_height)
            page_eject = 1;
    }

    if(page_eject)
    {
        SKEL_POINT skel_point;

        skel_point.pixit_point.y = 0;
        skel_point.page_num.y = p_rowtab->edge_bot.page + 1;
        format_row_move_y(p_docu, &skel_point, h_col_info, p_row_info);
    }
    else if(!p_row_info->height_fixed)
    {
        SLR slr;
        ARRAY_HANDLE h_col_changes;
        ARRAY_INDEX col_change_index = 0;
        ARRAY_HANDLE h_cell_bottoms = 0;
        ARRAY_INDEX n_col_changes;
        P_COL p_col;

        h_col_changes = style_change_between_cols(p_docu, 0, n_cols_logical(p_docu), p_row_info->row, &style_selector_para_text);
        n_col_changes = array_elements(&h_col_changes);
        p_col = array_ptr(&h_col_changes, COL, col_change_index);
        trace_1(TRACE_APP_FORMAT, TEXT("format_row found ") S32_TFMT TEXT(" style changes between cols"), n_col_changes);

        {
        STATUS status;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(SKEL_POINT), TRUE);
        consume_ptr(al_array_alloc(&h_cell_bottoms, SKEL_POINT, n_cols_physical(p_docu), &array_init_block, &status));
        status_assert(status);
        } /*block*/

        slr.col = 0;
        slr.row = p_row_info->row;

        while((slr.col < n_cols_physical(p_docu))
              ||
              (col_change_index < n_col_changes))
        {
            if(col_change_index < n_col_changes && slr.col >= n_cols_physical(p_docu))
                slr.col = *p_col;

            if(col_change_index < n_col_changes && slr.col == *p_col)
            {
                ++col_change_index;
                ++p_col;
            }

            if(slr.col >= array_elements(&h_col_info))
            {
                trace_0(TRACE_APP_SKEL, TEXT("format_row got no column data from skel_col_enum - ignoring column"));
            }
            else
            {
                const PC_COL_INFO p_col_info = array_ptrc(&h_col_info, COL_INFO, slr.col);
                SKEL_RECT skel_rect;

                /* set up minimum size for cell from grid and style */
                skel_point_update_from_edge(&skel_rect.tl, &p_col_info->edge_left, x);
                skel_point_update_from_edge(&skel_rect.tl, &p_rowtab->edge_top, y);

                skel_rect.br = skel_rect.tl;
                skel_point_update_from_edge(&skel_rect.br, &p_col_info->edge_right, x);

                if(skel_rect.br.pixit_point.x > skel_rect.tl.pixit_point.x)
                {
                    OBJECT_HOW_BIG object_how_big;
                    /* just check if we need to bump the page */
                    if(skel_rect.br.pixit_point.y > p_row_info->page_height && skel_rect.tl.pixit_point.y != 0)
                        skel_rect_move_to_next_page(&skel_rect);

                    /* ask object how big it is */
                    (void) cell_data_from_slr(p_docu, &object_how_big.object_data, &slr);
                    object_how_big.skel_rect = skel_rect;
                    format_object_how_big(p_docu, &object_how_big);

                    /* get largest of object and grid rectangles */
                    skel_rect.tl = object_how_big.skel_rect.tl;
                    if(skel_point_compare(&object_how_big.skel_rect.br, &skel_rect.br, x) > 0)
                    {
                        skel_rect.br.page_num.x = object_how_big.skel_rect.br.page_num.x;
                        skel_rect.br.pixit_point.x = object_how_big.skel_rect.br.pixit_point.x;
                    }
                    if(skel_point_compare(&object_how_big.skel_rect.br, &skel_rect.br, y) > 0)
                    {
                        skel_rect.br.page_num.y = object_how_big.skel_rect.br.page_num.y;
                        skel_rect.br.pixit_point.y = object_how_big.skel_rect.br.pixit_point.y;
                    }

                    trace_2(TRACE_APP_FORMAT, TEXT("format_row col: ") COL_TFMT TEXT(", row: ") ROW_TFMT, slr.col, slr.row);

                    format_object_size_change(p_docu, &skel_rect, NULL, &slr, h_col_info, p_row_info);

                    if(h_cell_bottoms && slr.col < array_elements(&h_cell_bottoms))
                        *array_ptr(&h_cell_bottoms, SKEL_POINT, slr.col) = skel_rect.br;
                }
            }

            ++slr.col;
        }

        if(0 != h_cell_bottoms)
        {
            const COL n_cols = array_elements(&h_cell_bottoms);
            PC_SKEL_POINT p_skel_point = array_rangec(&h_cell_bottoms, SKEL_POINT, 0, n_cols);

            for(slr.col = 0; slr.col < n_cols; ++slr.col, ++p_skel_point)
            {
                const P_CELL p_cell = p_cell_from_slr(p_docu, &slr);

                if(NULL != p_cell)
                {
                    p_cell->cell_new = 0;

                    if(!skel_point_edge_compare(p_skel_point, &p_rowtab->edge_bot, y))
                        p_cell->cell_at_row_bottom = 1;
                    else
                        p_cell->cell_at_row_bottom = 0;
                }
            }
        }

        al_array_dispose(&h_col_changes);
        al_array_dispose(&h_cell_bottoms);
    }

    extent_y_check(p_docu, p_row_info->row, p_rowtab->edge_bot.page);
}

/******************************************************************************
*
* move a row (and all below it) to a given y position
*
******************************************************************************/

static void
format_row_move_y(
    _DocuRef_   P_DOCU p_docu,
    P_SKEL_POINT p_skel_point,
    _InVal_     ARRAY_HANDLE h_col_info_in /* can be zero */,
    _InoutRef_  P_ROW_INFO p_row_info)
{
    P_ROWTAB p_rowtab;
    ARRAY_HANDLE h_col_info = h_col_info_in;

    if(0 == h_col_info_in)
        status_assert(skel_col_enum(p_docu, p_row_info->row, (PAGE) -1, (COL) -1, &h_col_info));

    p_rowtab = array_ptr(&p_docu->h_rowtab, ROWTAB, p_row_info->row);
    edge_set_from_skel_point(&p_rowtab->edge_top, p_skel_point, y);

    row_info_from_row(p_docu, p_row_info, p_row_info->row);

    /* this message is issued to cause the text object to release any cached data */
    status_assert(maeve_event(p_docu, T5_MSG_ROW_MOVE_Y, &p_row_info->row));

    format_row(p_docu, h_col_info, p_row_info);
}

/******************************************************************************
*
* ensure that the document is formatted up to and including the given row
*
******************************************************************************/

static void
ensure_formatted_upto_row__virtual_row_table(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    if(row >= p_docu->format_start_row)
    {
        ROW rows;
        
        rows = n_rows(p_docu) - 1;

        virtual_page_info_read(p_docu);

        extent_y_check(p_docu, rows, rows / p_docu->virtual_page_info.rows_per_page);

        p_docu->format_start_row = rows + 1; /* <<< SKS surely this should be rows + 1 (was row + 1) */

        trace_3(TRACE_OUT | TRACE_ANY, TEXT("ensure_formatted_upto_row__virtual_row_table(docno=") DOCNO_TFMT TEXT(", row=") ROW_TFMT TEXT(") - format_start_row := ") ROW_TFMT, docno_from_p_docu(p_docu), row, p_docu->format_start_row);
    }
}

static void
ensure_formatted_upto_row__normal_row_table(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    if(row >= p_docu->format_start_row)
    {
        ARRAY_HANDLE h_row_changes;

        h_row_changes = col_changes_between_rows(p_docu, p_docu->format_start_row, row + 1);

        if(h_row_changes != 0)
        {
            ROW i_row;
            ARRAY_INDEX row_changes_index = 0;
            ARRAY_HANDLE h_col_info = 0;
            ROW_INFO row_info;

            row_info.row = -1;
            row_info.page = -1;

            for(i_row = p_docu->format_start_row; i_row <= row; ++i_row)
            {
                P_ROW p_row = array_ptr(&h_row_changes, ROW, row_changes_index);
                P_ROWTAB p_rowtab;

                /* read column info when there is a change */
                if(i_row == *p_row)
                {
                    status_assert(skel_col_enum(p_docu, i_row, (PAGE) -1, (COL) -1, &h_col_info));
                    trace_1(TRACE_APP_FORMAT,
                            TEXT("ensure_formatted_upto_row__normal_row_table got: ") S32_TFMT TEXT(" cols from skel_col_enum"),
                            array_elements(&h_col_info));

                    /* move onto next change in column info, if there is one */
                    if(row_changes_index < array_elements(&h_row_changes))
                        ++row_changes_index;
                }

                /* initialise top of row from bottom of previous row */
                p_rowtab = array_ptr(&p_docu->h_rowtab, ROWTAB, i_row);

                if(!i_row)
                {
                    /* top of row 0 is always at top of page 0 */
                    p_rowtab[0].edge_top.pixit = 0;
                    p_rowtab[0].edge_top.page  = 0;
                }
                else
                    /* top of next row starts at bottom of previous row */
                    p_rowtab[0].edge_top = p_rowtab[-1].edge_bot;

                row_info_from_row(p_docu, &row_info, i_row);
                format_row(p_docu, h_col_info, &row_info);

                trace_3(TRACE_APP_FORMAT,
                        TEXT("ensure_formatted_upto_row__normal_row_table: ") ROW_TFMT TEXT(", edge_bot.page: ") S32_TFMT TEXT(", edge_bot.pixit: ") PIXIT_TFMT,
                        row, p_rowtab->edge_bot.page, p_rowtab->edge_bot.pixit);
            }
        }

        al_array_dispose(&h_row_changes);

        p_docu->format_start_row = row + 1;

        trace_3(TRACE_APP_FORMAT, TEXT("ensure_formatted_upto_row__normal_row_table(docno=") DOCNO_TFMT TEXT(", row=") ROW_TFMT TEXT(") - format_start_row := ") ROW_TFMT, docno_from_p_docu(p_docu), row, p_docu->format_start_row);
    }
}

static inline void
ensure_formatted_upto_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    if(p_docu->flags.virtual_row_table)
        ensure_formatted_upto_row__virtual_row_table(p_docu, row);
    else
        ensure_formatted_upto_row__normal_row_table(p_docu, row);
}

/******************************************************************************
*
* set document row count by adjusting row table size
*
******************************************************************************/

_Check_return_
static STATUS
n_rows_set__virtual_row_table(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW rows)
{
    STATUS status = STATUS_OK;

    al_array_dispose(&p_docu->h_rowtab);

    if(p_docu->rows_logical != rows)
    {
        p_docu->rows_logical = rows;
        status_assert(maeve_event(p_docu, T5_MSG_ROW_EXTENT_CHANGED, P_DATA_NONE));
    }

    return(status);
}

_Check_return_
static STATUS
n_rows_set__normal_row_table(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW rows)
{
    STATUS status = STATUS_OK;
    const ARRAY_INDEX old_size = array_elements(&p_docu->h_rowtab);
    const ARRAY_INDEX new_size = rows + 1; /* +1 is cos we need rowtab[1] too */

    if(new_size > old_size)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(64, sizeof32(ROWTAB), TRUE);
        consume(P_ROWTAB, al_array_extend_by(&p_docu->h_rowtab, ROWTAB, (new_size - old_size), &array_init_block, &status));
    }
    else if(new_size < old_size)
    {
        al_array_auto_compact_set(&p_docu->h_rowtab);

        al_array_shrink_by(&p_docu->h_rowtab, (new_size - old_size) /*-ve*/);
    }

    if(status_ok(status))
    {
        p_docu->rows_logical = rows;
        p_docu->last_used_row = MIN(p_docu->last_used_row, rows - 1);
        assert(p_docu->last_used_row >= 0);
    }

    if(old_size != array_elements(&p_docu->h_rowtab))
        status_assert(maeve_event(p_docu, T5_MSG_ROW_EXTENT_CHANGED, P_DATA_NONE));

    return(status);
}

_Check_return_
extern STATUS
n_rows_set(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW rows_in)
{
    const ROW rows = MAX(1, rows_in); /* SKS 24sep15 try to ensure sanity prevails at this low level */

    if(p_docu->flags.virtual_row_table)
        return(n_rows_set__virtual_row_table(p_docu, rows));
    else
        return(n_rows_set__normal_row_table(p_docu, rows));
}

/*
reformat everything
*/

extern void
reformat_all(
    _DocuRef_   P_DOCU p_docu)
{
    DOCU_REFORMAT docu_reformat;
    docu_reformat.action = REFORMAT_XY;
    docu_reformat.data_type = REFORMAT_ALL;
    docu_reformat.data_space = DATA_NONE;
    status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
}

/*
reformat from the given row
*/

extern void
reformat_from_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row,
    _InVal_     enum DOCU_REFORMAT_ACTION reformat_action)
{
    DOCU_REFORMAT docu_reformat;
    docu_reformat.action = reformat_action;
    docu_reformat.data_type = REFORMAT_ROW;
    docu_reformat.data_space = DATA_SLOT;
    docu_reformat.data.row = row;
    status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
}

/******************************************************************************
*
* given a point in skeleton coordinates
* return the row table entry
*
* reformats as necessary
*
******************************************************************************/

_Check_return_
extern STATUS
row_entry_at_skel_point(
    _DocuRef_   P_DOCU p_docu,
    P_ROW_ENTRY p_row_entry,
    _InRef_     PC_SKEL_POINT p_skel_point)
{
    STATUS status = STATUS_OK;

    row_entry_from_skel_point(p_docu, p_row_entry, p_skel_point, TRUE);

    /* check point is actually in row */
    if(edge_skel_point_compare(&p_row_entry->rowtab.edge_top, p_skel_point, y) > 0
       ||
       skel_point_edge_compare(p_skel_point, &p_row_entry->rowtab.edge_bot, y) > 0)
        status = STATUS_FAIL;

    return(status);
}

/******************************************************************************
*
* find the row table entry for a row
*
* reformats as necessary
*
******************************************************************************/

static void
row_entries_from_row__virtual_row_table(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ROW_ENTRY p_row_entry,
    _OutRef_opt_ P_ROW_ENTRY p_row_entry_next,
    _InVal_     ROW row)
{
    ROW row_at_top_of_page;

    if(row >= p_docu->format_start_row)
    {
        trace_3(TRACE_OUT | TRACE_ANY, TEXT("row_entries_from_row__virtual_row_table(docno=") DOCNO_TFMT TEXT(", row=") ROW_TFMT TEXT(") >= format_start_row ") ROW_TFMT, docno_from_p_docu(p_docu), row, p_docu->format_start_row);
        ensure_formatted_upto_row__virtual_row_table(p_docu, row);
    }

    p_docu->last_used_row = row;

    p_row_entry->rowtab.edge_top.page = row / p_docu->virtual_page_info.rows_per_page;
    p_row_entry->rowtab.edge_bot.page = p_row_entry->rowtab.edge_top.page;

    row_at_top_of_page = p_row_entry->rowtab.edge_top.page * p_docu->virtual_page_info.rows_per_page;

    p_row_entry->rowtab.edge_top.pixit = (row - row_at_top_of_page) * p_docu->virtual_page_info.row_height;
    p_row_entry->rowtab.edge_bot.pixit = p_row_entry->rowtab.edge_top.pixit + p_docu->virtual_page_info.row_height;

    p_row_entry->row = row;

    if(NULL != p_row_entry_next)
    {
        p_row_entry_next->rowtab.edge_top = p_row_entry->rowtab.edge_bot;

        if((p_row_entry_next->rowtab.edge_top.pixit + p_docu->virtual_page_info.row_height) > p_docu->virtual_page_info.page_height)
        {
            p_row_entry_next->rowtab.edge_top.page = p_row_entry->rowtab.edge_top.page + 1;
            p_row_entry_next->rowtab.edge_bot.page = p_row_entry->rowtab.edge_bot.page + 1;
            p_row_entry_next->rowtab.edge_top.pixit = 0;
            p_row_entry_next->rowtab.edge_bot.pixit = 0;
        }
        else
        {
            p_row_entry_next->rowtab.edge_bot.page = p_row_entry_next->rowtab.edge_top.page;
        }

        p_row_entry_next->rowtab.edge_bot.pixit = p_row_entry_next->rowtab.edge_top.pixit + p_docu->virtual_page_info.row_height;

        p_row_entry_next->row = row + 1;
    }
}

static void
row_entries_from_row__normal_row_table(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ROW_ENTRY p_row_entry,
    _OutRef_opt_ P_ROW_ENTRY p_row_entry_next,
    _InVal_     ROW row)
{
    P_ROWTAB p_rowtab;

    if(row >= p_docu->format_start_row)
    {
        trace_3(TRACE_OUT | TRACE_ANY, TEXT("row_entries_from_row__normal_row_table(docno=") DOCNO_TFMT TEXT(", row=") ROW_TFMT TEXT(") >= format_start_row ") ROW_TFMT, docno_from_p_docu(p_docu), row, p_docu->format_start_row);
        ensure_formatted_upto_row__normal_row_table(p_docu, row); /* <<< SKS for p_rowtab[1] to be valid this would have to be row + 1 */
    }

    p_docu->last_used_row = row;

    assert(row < n_rows(p_docu));

    p_rowtab = array_range(&p_docu->h_rowtab, ROWTAB, row, (1 + (NULL != p_row_entry_next)));

    p_row_entry->rowtab = p_rowtab[0];

    p_row_entry->row = row;

    if(NULL != p_row_entry_next)
    {
        p_row_entry_next->rowtab = p_rowtab[1];

        p_row_entry_next->row = row + 1;
    }
}

extern void
row_entry_from_row(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ROW_ENTRY p_row_entry,
    _InVal_     ROW row)
{
    if(p_docu->flags.virtual_row_table)
        row_entries_from_row__virtual_row_table(p_docu, p_row_entry, NULL, row);
    else
        row_entries_from_row__normal_row_table(p_docu, p_row_entry, NULL, row);
}

extern void
row_entries_from_row(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ROW_ENTRY p_row_entry,
    _OutRef_    P_ROW_ENTRY p_row_entry_next,
    _InVal_     ROW row)
{
    if(p_docu->flags.virtual_row_table)
        row_entries_from_row__virtual_row_table(p_docu, p_row_entry, p_row_entry_next, row);
    else
        row_entries_from_row__normal_row_table(p_docu, p_row_entry, p_row_entry_next, row);
}

/******************************************************************************
*
* given a point in skeleton coordinates return a pointer to the row table
* if the point is beyond the bottom of the document, or in a gap, the previous row is returned
*
******************************************************************************/

extern void
row_entry_from_skel_point(
    _DocuRef_   P_DOCU p_docu,
    P_ROW_ENTRY p_row_entry,
    _InRef_     PC_SKEL_POINT p_skel_point,
    _InVal_     BOOL down /* points on row boundaries go down */)
{
    ROW rows_n = n_rows(p_docu);
    ROW row = p_docu->last_used_row;
    S32 res;

    /* try to start in a sensible place to do the least work to find the row -
     * either last used row or last formatted row, depending on where the formatting stops
     */
    if( row > p_docu->format_start_row)
        row = p_docu->format_start_row;

    row_entry_from_row(p_docu, p_row_entry, row);

    /* is point at or below top edge of row ? */
    if((res = skel_point_edge_compare(p_skel_point, &p_row_entry->rowtab.edge_top, y)) >= 0)
    {
        while(res > 0 && row + 1 < rows_n)
        {
            row += 1;
            row_entry_from_row(p_docu, p_row_entry, row);
            res = skel_point_edge_compare(p_skel_point, &p_row_entry->rowtab.edge_top, y);
        }

        /* if we went past the target, or hit the end, step back */
        if((row > 0) && (res < 0 || (res == 0 && !down)))
        {
            --row;
            row_entry_from_row(p_docu, p_row_entry, row);
        }
    }
    else
    {
        /* point is above top of row */
        while((row > 0) && (res < 0 || (res == 0 && !down)))
        {
            --row;
            row_entry_from_row(p_docu, p_row_entry, row);
            res = skel_point_edge_compare(p_skel_point, &p_row_entry->rowtab.edge_top, y);
        }
    }

    assert(row >= 0);

    p_docu->last_used_row = row;
}

/******************************************************************************
*
* work out the height available for cols/rows given a y page number
*
******************************************************************************/

_Check_return_
static PIXIT
page_height_from_row__normal_row_table(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    ROW_INFO row_info;
    row_info.row = -1;
    row_info.page = -1;
    row_info_from_row__normal_row_table(p_docu, &row_info, row);
    return(row_info.page_height);
}

_Check_return_
extern PIXIT
page_height_from_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    if(p_docu->flags.virtual_row_table)
        return(p_docu->virtual_page_info.page_height);
    else
        return(page_height_from_row__normal_row_table(p_docu, row));
}

/******************************************************************************
*
* extract row from DOCU_REFORMAT
*
******************************************************************************/

_Check_return_
extern ROW
row_from_docu_reformat(
    _InRef_     PC_DOCU_REFORMAT p_docu_reformat)
{
    switch(p_docu_reformat->data_type)
    {
    default: default_unhandled();
#if CHECKING
    case REFORMAT_ALL:
#endif
        return(0);

    case REFORMAT_ROW:
        return(p_docu_reformat->data.row);

    case REFORMAT_SLR:
        return(p_docu_reformat->data.slr.row);

    case REFORMAT_DOCU_AREA:
        return(p_docu_reformat->data.docu_area.whole_col ? 0 : p_docu_reformat->data.docu_area.tl.slr.row);
    }
}

/******************************************************************************
*
* find the row at the top of the given y page
*
******************************************************************************/

_Check_return_
extern ROW
row_from_page_y(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y)
{
    SKEL_POINT skel_point;
    ROW_ENTRY row_entry;

    skel_point.pixit_point.x =
    skel_point.pixit_point.y = 0;
    skel_point.page_num.x = 0;
    skel_point.page_num.y = page_y;

    row_entry_from_skel_point(p_docu, &row_entry, &skel_point, TRUE);
    return(row_entry.row);
}

/******************************************************************************
*
* capture NULL events for background reformat
*
******************************************************************************/

T5_MSG_PROTO(static, rowtab_event_null, P_NULL_EVENT_BLOCK p_null_event_block)
{
    const MONOTIME time_started = p_null_event_block->initial_time;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    trace_3(TRACE_OUT | TRACE_ANY /*TRACE_APP_FORMAT*/, TEXT("null_event_rowtab_format(docno=") DOCNO_TFMT TEXT(") - null_event: n_rows: ") ROW_TFMT TEXT(", format_start_row: ") ROW_TFMT, docno_from_p_docu(p_docu), n_rows(p_docu), p_docu->format_start_row);

#if RISCOS
    /* if we ran out last (or some other) time, see if we've got some memory to run in now */
    if(p_docu->flags.no_memory_for_bg_format)
    {
        STATUS status_froth = ensure_memory_froth();

        if(status_fail(status_froth))
            return(STATUS_OK);

        p_docu->flags.no_memory_for_bg_format = 0;
    }
#endif /* RISCOS */

    for(;;)
    {

#if RISCOS
        { /* SKS 03jun93 - try to keep memory full away from guts of formatter */
        STATUS status = ensure_memory_froth();

        if(status_fail(status))
        {
            p_docu->flags.no_memory_for_bg_format = 1;
            reperr_null(status);
            break;
        }
        } /*block*/
#endif /* RISCOS */

        if(p_docu->format_start_row < n_rows(p_docu))
            ensure_formatted_upto_row(p_docu, p_docu->format_start_row);

        if(p_docu->format_start_row >= n_rows(p_docu))
        {
            if(p_docu->flags.null_event_rowtab_format_started)
            {
                p_docu->flags.null_event_rowtab_format_started = FALSE;
                trace_1(TRACE_OUT | TRACE_ANY, TEXT("null_event_rowtab_format(docno=") DOCNO_TFMT TEXT(") - reformat ended - *** null_events_stop()"), docno_from_p_docu(p_docu));
                null_events_stop(docno_from_p_docu(p_docu), T5_EVENT_NULL, null_event_rowtab_format, ROWTAB_NULL_CLIENT_HANDLE);
            }
            else
            {
                trace_1(TRACE_OUT | TRACE_ANY, TEXT("null_event_rowtab_format(docno=") DOCNO_TFMT TEXT(") - reformat ended - *** null_events ALREADY STOPPED"), docno_from_p_docu(p_docu));
            }
            break;
        }

        if(monotime_diff(time_started) >
#if TRACE_ALLOWED
            (trace_is_on() ? BACKGROUND_SLICE * 10 : BACKGROUND_SLICE)
#else
            BACKGROUND_SLICE
#endif
            )
            break;
    }

    docu_flags_new_extent_set(p_docu);

    return(STATUS_OK);
}

PROC_EVENT_PROTO(static, null_event_rowtab_format)
{
#if CHECKING
    switch(t5_message)
    {
    default: default_unhandled();
        return(STATUS_OK);

    case T5_EVENT_NULL:
#else
    {
#endif
        return(rowtab_event_null(p_docu, t5_message, (P_NULL_EVENT_BLOCK) p_data));
    }
}

/*
main events
*/

T5_MSG_PROTO(static, rowtab_msg_reformat, _InRef_ PC_DOCU_REFORMAT p_docu_reformat)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_docu_reformat->data_space)
    {
    case DATA_NONE:
    case DATA_SLOT:
        {
        BOOL caret_repos = 0;

        switch(p_docu_reformat->action)
        {
        case REFORMAT_XY:
            page_x_extent_set(p_docu);

            format_below_row(p_docu, row_from_docu_reformat(p_docu_reformat));

            caret_repos = p_docu->flags.is_current; /* 8.11.93 */

#if TRACE_ALLOWED
            if_constant(tracing(TRACE_APP_MEMORY_USE))
            {
                trace_1(TRACE_APP_MEMORY_USE, TEXT("REFORMAT_XY, row: ") ROW_TFMT, row_from_docu_reformat(p_docu_reformat));

                if(row_from_docu_reformat(p_docu_reformat) == 0)
                {
                    COL col;
                    U32 total_used = 0, total_size = 0, total_pools = 0;

                    for(col = 0; col < n_cols_physical(p_docu); ++ col)
                    {
                        U32 used, size, pools;
                        list_size(array_ptr(&p_docu->h_col_list, LIST_BLOCK, col), &used, &size, &pools);
                        total_used += used;
                        total_size += size;
                        total_pools += pools;
                    }

                    trace_4(TRACE_APP_MEMORY_USE,
                            TEXT("TOTAL cols: ") COL_TFMT TEXT(", used: ") U32_TFMT TEXT(", size: ") U32_TFMT TEXT(", pools: ") U32_TFMT,
                            n_cols_physical(p_docu), total_used, total_size, total_pools);
                    trace_2(TRACE_APP_MEMORY_USE,
                            TEXT("handle table has: ") U32_TFMT TEXT(" entries, total: ") U32_TFMT TEXT(" bytes"),
                            (U32) array_root.used_handles, array_root.size * sizeof32(ARRAY_BLOCK));
                    trace_2(TRACE_APP_MEMORY_USE,
                            TEXT("row table has: ") U32_TFMT TEXT(" entries, total: ") U32_TFMT TEXT(" bytes"),
                            array_elements32(&p_docu->h_rowtab), array_elements32(&p_docu->h_rowtab) * sizeof32(ROWTAB));
                }

            }

#endif /* TRACE_ALLOWED */

            break;

        case REFORMAT_HEFO_Y:
            trace_0(TRACE_APP_SKEL, TEXT("REFORMAT_HEFO_Y"));

            /*FALLTHRU*/

        case REFORMAT_Y:
            format_below_row(p_docu, row_from_docu_reformat(p_docu_reformat));

            caret_repos = p_docu->flags.is_current; /* 8.11.93 */

            trace_1(TRACE_APP_SKEL, TEXT("REFORMAT_Y row: ") ROW_TFMT, row_from_docu_reformat(p_docu_reformat));
            break;
        }

        if(caret_repos)
            if((OBJECT_ID_CELLS == p_docu->focus_owner) || (OBJECT_ID_REC_FLOW == p_docu->focus_owner))
                p_docu->flags.caret_position_later = 1;

        return(STATUS_OK);
        }

    default:
        return(STATUS_OK);
    }
}

MAEVE_EVENT_PROTO(static, maeve_event_sk_form)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_REFORMAT:
        return(rowtab_msg_reformat(p_docu, t5_message, (PC_DOCU_REFORMAT) p_data));

    default:
        return(STATUS_OK);
    }
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_form);

_Check_return_
static STATUS
sk_form_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    p_docu->h_rowtab = 0;
    p_docu->rows_logical = 0;

    p_docu->format_start_row = MAX_ROW;
    p_docu->last_used_row = 0;

    /* ensure we have at least one entry */
    p_docu->flags.virtual_row_table = 0; /* SKS 03feb96 vvv bodge for fast loading */

    trace_1(TRACE_OUT | TRACE_ANY, TEXT("sk_form_msg_init1(docno=") DOCNO_TFMT TEXT("): calling n_rows_set(1)"), docno_from_p_docu(p_docu));
    status_return(n_rows_set(p_docu, 1)); /* SKS 11jun96 do this without virtual_row_table set to start with */

    trace_1(TRACE_OUT | TRACE_ANY, TEXT("sk_form_msg_init1(docno=") DOCNO_TFMT TEXT("): calling virtual_row_table_set(1)"), docno_from_p_docu(p_docu));
    status_assert(virtual_row_table_set(p_docu, 1)); /* SKS 11jun96 make with 0 then write 1 to ensure transition */

    p_docu->_last_page_y = 1;

    trace_1(TRACE_OUT | TRACE_ANY, TEXT("sk_form_msg_init1(docno=") DOCNO_TFMT TEXT("): calling page_x_extent_set"), docno_from_p_docu(p_docu));
    page_x_extent_set(p_docu);

    return(maeve_event_handler_add(p_docu, maeve_event_sk_form, (CLIENT_HANDLE) 0));
}

_Check_return_
static STATUS
sk_form_msg_close2(
    _DocuRef_   P_DOCU p_docu)
{
    maeve_event_handler_del(p_docu, maeve_event_sk_form, (CLIENT_HANDLE) 0);

    al_array_dispose(&p_docu->h_rowtab);
    p_docu->rows_logical = 0;

    if(p_docu->flags.null_event_rowtab_format_started)
    {
        p_docu->flags.null_event_rowtab_format_started = FALSE;
        trace_1(TRACE_OUT | TRACE_ANY, TEXT("sk_form_msg_close2(docno=") DOCNO_TFMT TEXT(") - *** null_events_stop()"), docno_from_p_docu(p_docu));
        null_events_stop(docno_from_p_docu(p_docu), T5_EVENT_NULL, null_event_rowtab_format, ROWTAB_NULL_CLIENT_HANDLE);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_sk_form_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT1:
        return(sk_form_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE2:
        return(sk_form_msg_close2(p_docu));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_form)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_form_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of sk_form.c */
