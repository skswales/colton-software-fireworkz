/* sk_draw.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Redraw code for skeleton cells */

/* MRJC January 1992 */

#include "common/gflags.h"

#ifndef    __cells_ob_cells_h
#include "ob_cells/ob_cells.h"
#endif

/******************************************************************************
*
* given a cell rectangle, calculate the covering skeleton rectangle and also
* the covering pixit rectangle for the specified y-page
*
******************************************************************************/

static void
rects_from_region_and_page(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SKEL_RECT p_skel_rect,
    _OutRef_    P_PIXIT_RECT p_pixit_rect,
    _InVal_     COL col_s,
    _InVal_     COL col_e,
    _InVal_     ROW row_s,
    _InVal_     ROW row_e,
    P_ARRAY_HANDLE p_h_col_info,
    _InVal_     PAGE page_y)
{
    const PC_COL_INFO p_col_info = array_basec(p_h_col_info, COL_INFO);
    COL col_start = p_col_info->col;

    skel_point_update_from_edge(&p_skel_rect->tl, &p_col_info[col_s - col_start].edge_left, x);
    skel_point_update_from_edge(&p_skel_rect->br, &p_col_info[col_e - col_start - 1].edge_right, x);

    {
    ROW_ENTRY row_entry;
    row_entry_from_row(p_docu, &row_entry, row_s);
    skel_point_update_from_edge(&p_skel_rect->tl, &row_entry.rowtab.edge_top, y);
    row_entry_from_row(p_docu, &row_entry, row_e - 1);
    skel_point_update_from_edge(&p_skel_rect->br, &row_entry.rowtab.edge_bot, y);
    } /*block*/

    /* set up pixit rectangle */
    p_pixit_rect->tl = p_skel_rect->tl.pixit_point;
    p_pixit_rect->br = p_skel_rect->br.pixit_point;

    /* limit pixit rectangle to current page
     * - start of row on previous page ?
     */
    if(p_skel_rect->tl.page_num.y < page_y)
        p_pixit_rect->tl.y = 0;

    /* - end of row on next page ? */
    if(p_skel_rect->br.page_num.y > page_y)
    {
        HEADFOOT_BOTH headfoot_both;
        headfoot_both_from_row_and_page_y(p_docu,
                                          &headfoot_both,
                                          row_e - 1,
                                          page_y);
        p_pixit_rect->br.y = p_docu->page_def.cells_usable_y - headfoot_both.header.margin - headfoot_both.footer.margin;
    }
}

/******************************************************************************
*
* check if cell is in a marker list
*
******************************************************************************/

static BOOL
slr_in_marker_list(
    _InRef_     PC_ARRAY_HANDLE p_h_marker_list,
    _InRef_     PC_SLR p_slr,
    _OutRef_    P_OBJECT_POSITION p_object_position_start,
    _OutRef_    P_OBJECT_POSITION p_object_position_end)
{
    const ARRAY_INDEX n_markers = array_elements(p_h_marker_list);
    ARRAY_INDEX ix_markers;
    P_MARKERS p_markers = array_range(p_h_marker_list, MARKERS, 0, n_markers);

    p_object_position_start->object_id = OBJECT_ID_NONE;
    p_object_position_end->object_id = OBJECT_ID_NONE;

    for(ix_markers = 0; ix_markers < n_markers; ++ix_markers, ++p_markers)
    {
        if(slr_in_docu_area(&p_markers->docu_area, p_slr))
        {
            SLR slr_end;

            /* set up object specific data */
            if(slr_equal(p_slr, &p_markers->docu_area.tl.slr))
                *p_object_position_start = p_markers->docu_area.tl.object_position;

            slr_end = p_markers->docu_area.br.slr;
            slr_end.col -= 1;
            slr_end.row -= 1;
            if(slr_equal(p_slr, &slr_end))
                *p_object_position_end = p_markers->docu_area.br.object_position;

            return(TRUE);
        }
    }

    return(FALSE);
}

/******************************************************************************
*
* draw the background in a rectangle
*
******************************************************************************/

static void
draw_background(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_s,
    _InVal_     ROW row_e,
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw)
{
    ARRAY_HANDLE h_row_changes;
    STYLE_SELECTOR selector_row;
    STYLE_SELECTOR selector_col;

    style_selector_clear(&selector_row);
    style_selector_bit_set(&selector_row, STYLE_SW_CS_WIDTH);
    style_selector_bit_set(&selector_row, STYLE_SW_PS_RGB_BACK);

    style_selector_clear(&selector_col);
    style_selector_bit_set(&selector_col, STYLE_SW_PS_RGB_BACK);

    if((h_row_changes = style_change_between_rows(p_docu, row_s, row_e, &selector_row)) != 0)
    {
        P_ROW p_row;
        ARRAY_INDEX i;

        /* loop over blocks of cells of the same width */
        for(i = 0, p_row = array_ptr(&h_row_changes, ROW, i);
            i < array_elements(&h_row_changes) && p_row[0] < row_e;
            ++i, ++p_row)
        {
            ROW row_last;
            ARRAY_HANDLE h_col_info = 0;

            if(i + 1 < array_elements(&h_row_changes))
                row_last = p_row[1];
            else
                row_last = row_e;

            /* read column info */
            status_assert(skel_col_enum(p_docu, p_row[0], p_skelevent_redraw->clip_skel_rect.tl.page_num.x, (COL) -1, &h_col_info));

            if(0 != array_elements(&h_col_info))
            {
                COL col_l;

                if((col_l = col_at_skel_point(p_docu, h_col_info, &p_skelevent_redraw->clip_skel_rect.tl)) >= 0)
                {
                    ARRAY_HANDLE h_col_changes;
                    COL col_r;

                    col_r = 1 + col_left_from_skel_point(p_docu, h_col_info, &p_skelevent_redraw->clip_skel_rect.br);

                    if((h_col_changes = style_change_between_cols(p_docu, col_l, col_r, p_row[0], &selector_col)) != 0)
                    {
                        ARRAY_INDEX i_col;
                        P_COL p_col;

                        for(i_col = 0, p_col = array_ptr(&h_col_changes, COL, i_col);
                            i_col < array_elements(&h_col_changes);
                            ++i_col, ++p_col)
                        {
                            STYLE style;
                            SLR slr;

                            style_init(&style);
                            slr.col = p_col[0];
                            slr.row = p_row[0];
                            style_from_slr(p_docu, &style, &selector_col, &slr);

                            if(rgb_compare_not_equals(&style.para_style.rgb_back, &rgb_stash[COLOUR_OF_PAPER]))
                            {
                                COL col_first, col_last;

                                if(i_col + 1 < array_elements(&h_col_changes))
                                    col_last = p_col[1];
                                else
                                    col_last = col_r;

                                col_first = MAX(p_col[0], col_l);
                                col_last  = MIN(col_last, col_r);

                                if(col_last > col_first)
                                {
                                    SKEL_RECT skel_rect;
                                    PIXIT_RECT pixit_rect;

                                    rects_from_region_and_page(p_docu,
                                                               &skel_rect,
                                                               &pixit_rect,
                                                               col_first,
                                                               col_last,
                                                               p_row[0],
                                                               row_last,
                                                               &h_col_info,
                                                               p_skelevent_redraw->clip_skel_rect.tl.page_num.y);

                                    host_paint_rectangle_filled(&p_skelevent_redraw->redraw_context,
                                                                &pixit_rect,
                                                                &style.para_style.rgb_back);
                                }
                            }
                        }
                        al_array_dispose(&h_col_changes);
                    }
                }
            }
        }
    }
    al_array_dispose(&h_row_changes);
}

/******************************************************************************
*
* draw the grids in a rectangle
*
******************************************************************************/

static void
draw_grid(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_s,
    _InVal_     ROW row_e,
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw)
{
    ARRAY_HANDLE h_row_changes;
    STYLE_SELECTOR selector;

    style_selector_clear(&selector);
    style_selector_bit_set(&selector, STYLE_SW_CS_WIDTH);

    if((h_row_changes = style_change_between_rows(p_docu, row_s, row_e, &selector)) != 0)
    {
        P_ROW p_row;
        ARRAY_INDEX i;

        /* loop over blocks of cells of the same width */
        for(i = 0, p_row = array_ptr(&h_row_changes, ROW, i);
            i < array_elements(&h_row_changes) && p_row[0] < row_e;
            ++i, ++p_row)
        {
            ROW row_last;
            ARRAY_HANDLE h_col_info = 0;

            if(i + 1 < array_elements(&h_row_changes))
                row_last = p_row[1];
            else
                row_last = row_e;

            /* read column info */
            status_assert(skel_col_enum(p_docu, p_row[0], p_skelevent_redraw->clip_skel_rect.tl.page_num.x, (COL) -1, &h_col_info));

            if(0 != array_elements(&h_col_info))
            {
                COL col_start = array_basec(&h_col_info, COL_INFO)->col;
                COL col_l;

                if((col_l = col_at_skel_point(p_docu, h_col_info, &p_skelevent_redraw->clip_skel_rect.tl)) >= 0)
                {
                    COL col_r;
                    GRID_BLOCK grid_block;

                    style_grid_block_init(&grid_block);

                    col_r = 1 + col_left_from_skel_point(p_docu, h_col_info, &p_skelevent_redraw->clip_skel_rect.br);

                    /* set page flags for this region */
                    { /* --- */
                    PAGE_FLAGS page_flags;
                    zero_struct(page_flags);

                    {
                    ROW_ENTRY row_entry, row_entry_next;
                    row_entry_from_row(p_docu, &row_entry, p_row[0]);
                    page_flags.first_row_on_page = row_entry.rowtab.edge_top.pixit == 0;
                    row_entries_from_row(p_docu, &row_entry, &row_entry_next, row_last - 1);
                    page_flags.last_row_on_page = (row_last == row_e) && (row_entry.rowtab.edge_top.page != row_entry_next.rowtab.edge_top.page);
                    } /*block*/

                    /* set page flags for col_l / col_r */
                    page_flags.first_col_on_page = array_ptr(&h_col_info, COL_INFO, col_l - col_start)->edge_left.pixit == 0;
                    page_flags.last_col_on_page = (col_r - col_start) >= array_elements(&h_col_info);

                    { /* +++ */
                    REGION region;
                    region.tl.col = col_l;
                    region.tl.row = p_row[0];
                    region.br.col = col_r;
                    region.br.row = row_last;
                    region.whole_col = region.whole_row = 0;

                    if(status_ok(style_grid_block_for_region_new(p_docu, &grid_block, &region, page_flags)))
                    {

                        { /* draw horizontal gridlines */
                        ROW row = p_row[0];
                        GRID_FLAGS grid_flags;

                        zero_struct(grid_flags);
                        if(p_docu->flags.faint_grid)
                        {
                            if(!(p_skelevent_redraw->redraw_context.flags.printer  ||
                                 p_skelevent_redraw->redraw_context.flags.metafile ||
                                 p_skelevent_redraw->redraw_context.flags.drawfile ) )
                            {
                                grid_flags.faint_grid = TRUE;
                            }
                        }

                        for(;;)
                        {
                            COL col = col_l;

                            while(col < col_r)
                            {
                                GRID_LINE_STYLE grid_line_style;
                                COL col_next;
                                SKEL_RECT skel_rect;
                                PIXIT_RECT pixit_rect;
                                PIXIT_LINE pixit_line;

                                col_next = style_grid_from_grid_block_h(&grid_line_style, &grid_block, col, row, grid_flags);

                                rects_from_region_and_page(p_docu,
                                                           &skel_rect,
                                                           &pixit_rect,
                                                           col,
                                                           col_next,
                                                           row,
                                                           row + 1,
                                                           &h_col_info,
                                                           p_skelevent_redraw->clip_skel_rect.tl.page_num.y);

                                if(!grid_flags.outer_edge)
                                    pixit_line.tl.y = pixit_line.br.y = pixit_rect.tl.y;
                                else
                                    pixit_line.tl.y = pixit_line.br.y = pixit_rect.br.y;

                                pixit_line.tl.x = pixit_rect.tl.x;
                                pixit_line.br.x = pixit_rect.br.x;
                                pixit_line.horizontal = 1;

                                if(pixit_line.br.x > pixit_line.tl.x
                                   ||
                                   pixit_line.br.y > pixit_line.tl.y)
                                    host_paint_border_line(&p_skelevent_redraw->redraw_context,
                                                           &pixit_line,
                                                           &grid_line_style.rgb,
                                                           grid_line_style.border_line_flags);

                                col = col_next;
                            }

                            if(grid_flags.outer_edge)
                                break;
                            else if(row + 1 < row_last)
                                row += 1;
                            else
                                grid_flags.outer_edge = 1;
                        }
                        } /*block*/

                        { /* draw vertical gridlines */
                        COL col = col_l;
                        GRID_FLAGS grid_flags;

                        zero_struct(grid_flags);
                        if(p_docu->flags.faint_grid)
                        {
                            if(!(p_skelevent_redraw->redraw_context.flags.printer  ||
                                 p_skelevent_redraw->redraw_context.flags.metafile ||
                                 p_skelevent_redraw->redraw_context.flags.drawfile ) )
                            {
                                grid_flags.faint_grid = TRUE;
                            }
                        }

                        for(;;)
                        {
                            ROW row = p_row[0];

                            while(row < row_last)
                            {
                                GRID_LINE_STYLE grid_line_style;
                                ROW row_next;
                                SKEL_RECT skel_rect;
                                PIXIT_RECT pixit_rect;
                                PIXIT_LINE pixit_line;

                                row_next = style_grid_from_grid_block_v(&grid_line_style, &grid_block, col, row, grid_flags);

                                rects_from_region_and_page(p_docu,
                                                           &skel_rect,
                                                           &pixit_rect,
                                                           col,
                                                           col + 1,
                                                           row,
                                                           row_next,
                                                           &h_col_info,
                                                           p_skelevent_redraw->clip_skel_rect.tl.page_num.y);

                                if(!grid_flags.outer_edge)
                                    pixit_line.tl.x = pixit_line.br.x = pixit_rect.tl.x;
                                else
                                    pixit_line.tl.x = pixit_line.br.x = pixit_rect.br.x;

                                pixit_line.tl.y = pixit_rect.tl.y;
                                pixit_line.br.y = pixit_rect.br.y;
                                pixit_line.horizontal = 0;

                                if(pixit_line.br.x > pixit_line.tl.x
                                   ||
                                   pixit_line.br.y > pixit_line.tl.y)
                                    host_paint_border_line(&p_skelevent_redraw->redraw_context,
                                                           &pixit_line,
                                                           &grid_line_style.rgb,
                                                           grid_line_style.border_line_flags);

                                row = row_next;
                            }

                            if(grid_flags.outer_edge)
                                break;
                            else if(col + 1 < col_r)
                                col += 1;
                            else
                                grid_flags.outer_edge = 1;
                        }
                        } /*block*/

                    } /* style_grid_block_for_region_new */

                    } /* +++ */ /*block*/

                    } /* --- */ /*block*/

                    style_grid_block_dispose(&grid_block);
                }
            }
        }
    }
    al_array_dispose(&h_row_changes);
}

/******************************************************************************
*
* draw all objects in a rectangle
*
******************************************************************************/

static void
draw_these_objects(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_s,
    _InVal_     ROW row_e,
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw)
{
    ARRAY_HANDLE h_row_changes;

    /* get list of changes of column information */
    if((h_row_changes = col_changes_between_rows(p_docu, row_s, row_e)) != 0)
    {
        ARRAY_INDEX row_changes_idx = 0;
        ARRAY_HANDLE h_col_info = 0;
        SLR slr;

        /* loop over rows in redraw rectangle */
        for(slr.row = row_s; slr.row < row_e; ++slr.row)
        {
            P_ROW p_row = array_ptr(&h_row_changes, ROW, row_changes_idx);

            /* read column info when there is a change */
            if(slr.row == *p_row)
            {
                skel_col_enum_cache_release(h_col_info);
                status_assert(skel_col_enum(p_docu, slr.row, p_skelevent_redraw->clip_skel_rect.tl.page_num.x, (COL) -1, &h_col_info));
                skel_col_enum_cache_lock(h_col_info); /* NB handle may be zero */

                /* move onto next change in column info, if there is one */
                if(row_changes_idx < array_elements(&h_row_changes))
                    ++row_changes_idx;
            }

            if(0 != array_elements(&h_col_info))
            {
                COL col_l;

                if((col_l = col_at_skel_point(p_docu, h_col_info, &p_skelevent_redraw->clip_skel_rect.tl)) >= 0)
                {
                    COL col_r = col_left_from_skel_point(p_docu, h_col_info, &p_skelevent_redraw->clip_skel_rect.br);

                    for(slr.col = col_l; slr.col >= 0 && slr.col <= col_r; slr.col += 1)
                    {
                        OBJECT_REDRAW object_redraw;
                        STYLE style;
                        STYLE_SELECTOR selector;
                        PIXIT_RECT pixit_rect;
                        RECT_FLAGS rect_flags_clip;
                        RECT_FLAGS_CLEAR(rect_flags_clip);

                        /* skip zero width columns */
                        if(!col_width(h_col_info, slr.col))
                            continue;

                        (void) cell_data_from_slr(p_docu, &object_redraw.object_data, &slr);

                        rects_from_region_and_page(p_docu,
                                                   &object_redraw.skel_rect_object,
                                                   &object_redraw.pixit_rect_object,
                                                   slr.col,
                                                   slr.col + 1,
                                                   slr.row,
                                                   slr.row + 1,
                                                   &h_col_info,
                                                   p_skelevent_redraw->clip_skel_rect.tl.page_num.y);

                        pixit_rect.tl = p_skelevent_redraw->clip_skel_rect.tl.pixit_point;
                        pixit_rect.br = p_skelevent_redraw->clip_skel_rect.br.pixit_point;

                        object_redraw.skel_rect_clip = p_skelevent_redraw->clip_skel_rect;
                        object_redraw.skel_rect_work = p_skelevent_redraw->work_skel_rect;
                        object_redraw.redraw_context = p_skelevent_redraw->redraw_context;

                        /* work out any markers on cell */
                        object_redraw.flags.marked_now    = slr_in_marker_list(&p_docu->mark_info_cells.h_markers,
                                                                               &slr,
                                                                               &object_redraw.start_now,
                                                                               &object_redraw.end_now);
                        object_redraw.flags.marked_screen = slr_in_marker_list(&p_docu->mark_info_cells.h_markers_screen,
                                                                               &slr,
                                                                               &object_redraw.start_screen,
                                                                               &object_redraw.end_screen);

                        object_redraw.flags.show_content = p_skelevent_redraw->flags.show_content;
                        object_redraw.flags.show_selection = p_skelevent_redraw->flags.show_selection;

                        /******* read style info required for borders, grids and backgrounds *******/
                        style_selector_copy(&selector, &style_selector_para_border);
                        style_selector_bit_set(&selector, STYLE_SW_PS_RGB_BACK);
                        style_selector_bit_set(&selector, STYLE_SW_FS_COLOUR);

                        style_init(&style);
                        style_from_slr(p_docu, &style, &selector, &slr);
                        object_redraw.rgb_fore = style.font_spec.colour;
                        object_redraw.rgb_back = style.para_style.rgb_back;

                        if(host_set_clip_rectangle2(&object_redraw.redraw_context,
                                                    &pixit_rect,
                                                    &object_redraw.pixit_rect_object,
                                                    rect_flags_clip))
                        {
                            object_border(p_docu, &object_redraw, &style);
                        }

                        object_redraw.redraw_context = p_skelevent_redraw->redraw_context; /* must copy again for new clip rect */
                        RECT_FLAGS_CLEAR(rect_flags_clip);

                        if(!object_redraw.redraw_context.flags.printer)
                        {
                            if((style.para_style.border != SF_BORDER_NONE) /*|| (!style.para_style.rgb_border.transparent)*/)
                            {   /* stop object painting on grids and borders */
                                rect_flags_clip.reduce_left_by_2  = 1;
                                rect_flags_clip.reduce_up_by_2    = 1;
                                rect_flags_clip.reduce_right_by_1 = 1;
                                rect_flags_clip.reduce_down_by_1  = 1;
                            }
                            else
                            {   /* stop object painting on grids (border is NONE and transparent for maximum cell content display) */
                                rect_flags_clip.reduce_left_by_1  = 1;
                                rect_flags_clip.reduce_up_by_1    = 1;
                            }
                        }

                        if(host_set_clip_rectangle2(&object_redraw.redraw_context,
                                                    &pixit_rect,
                                                    &object_redraw.pixit_rect_object,
                                                    rect_flags_clip))
                        {
                            trace_2(TRACE_APP_SKEL_DRAW, TEXT(">>> redraw object col: ") COL_TFMT TEXT(", row: ") ROW_TFMT, slr.col, slr.row);

                            /* call the object for redraw/inversion */
                            status_consume(cell_call_id(object_redraw.object_data.object_id, p_docu, T5_EVENT_REDRAW, &object_redraw, NO_CELLS_EDIT));
                        }

                        /* clip rectangles will be reset by higher level */
                    }
                }
            }
        }

        skel_col_enum_cache_release(h_col_info);
    }
    al_array_dispose(&h_row_changes);
}

/******************************************************************************
*
* paint object background
*
******************************************************************************/

extern void
object_background(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    _InRef_     PC_STYLE p_style)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    /* only render background if requested */
    if(!p_object_redraw->flags.show_content)
        return;

    if(rgb_compare_not_equals(&p_style->para_style.rgb_back, &rgb_stash[COLOUR_OF_PAPER]))
        host_paint_rectangle_filled(&p_object_redraw->redraw_context,
                                    &p_object_redraw->pixit_rect_object,
                                    &p_style->para_style.rgb_back);
}

/******************************************************************************
*
* paint object internal borders
*
******************************************************************************/

extern void
object_border(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    _InRef_     PC_STYLE p_style)
{
    /* only render border if requested */
    if(!p_object_redraw->flags.show_content)
        return;

    if(p_style->para_style.border == SF_BORDER_NONE)
        return;

    if(grid_is_on(p_docu))
    {
        PC_RGB p_rgb = &p_style->para_style.rgb_border;
        const PC_REDRAW_CONTEXT p_redraw_context = &p_object_redraw->redraw_context;
        PIXIT_RECT pixit_rect;
        PIXIT_LINE pixit_line;
        BORDER_LINE_FLAGS border_line_flags;

        /* adjust to border rectangle */
        pixit_rect = p_object_redraw->pixit_rect_object;

        /* left border */
        pixit_line.tl = pixit_rect.tl;
        pixit_line.br = pixit_rect.br;
        pixit_line.br.x = pixit_line.tl.x;
        pixit_line.horizontal = 0;

        CLEAR_BORDER_LINE_FLAGS(border_line_flags);
        border_line_flags.add_gw_to_l = 1;
        border_line_flags.add_gw_to_r = 1;
        border_line_flags.add_gw_to_t = 1;
        border_line_flags.sub_gw_from_b = 1;
        border_line_flags.add_lw_to_b = 1;
        border_line_flags.border_style = p_style->para_style.border;

#ifdef SKS_BORDER_DEBUG
        p_rgb = &rgb_stash[8];
#endif
        host_paint_border_line(p_redraw_context, &pixit_line, p_rgb, border_line_flags);

        /* top border */
        pixit_line.tl = pixit_rect.tl;
        pixit_line.br = pixit_rect.br;
        pixit_line.br.y = pixit_line.tl.y;
        pixit_line.horizontal = 1;

        CLEAR_BORDER_LINE_FLAGS(border_line_flags);
        border_line_flags.add_gw_to_t = 1;
        border_line_flags.add_gw_to_b = 1;
        border_line_flags.add_gw_to_l = 1;
        border_line_flags.add_lw_to_l = 1;
        border_line_flags.sub_gw_from_r = 1;
        border_line_flags.border_style = p_style->para_style.border;

#ifdef SKS_BORDER_DEBUG
        p_rgb = &rgb_stash[9];
#endif
        host_paint_border_line(p_redraw_context, &pixit_line, p_rgb, border_line_flags);

        /* right border */
        pixit_line.tl = pixit_rect.tl;
        pixit_line.br = pixit_rect.br;
        pixit_line.tl.x = pixit_line.br.x;
        pixit_line.horizontal = 0;

        CLEAR_BORDER_LINE_FLAGS(border_line_flags);
        border_line_flags.sub_gw_from_l = 1;
        border_line_flags.sub_gw_from_r = 1;
        border_line_flags.add_gw_to_t = 1;
        border_line_flags.sub_gw_from_b = 1;
        border_line_flags.add_lw_to_b = 1;
        border_line_flags.border_style = p_style->para_style.border;

#ifdef SKS_BORDER_DEBUG
        p_rgb = &rgb_stash[10];
#endif
        host_paint_border_line(p_redraw_context, &pixit_line, p_rgb, border_line_flags);

        /* bottom border */
        pixit_line.tl = pixit_rect.tl;
        pixit_line.br = pixit_rect.br;
        pixit_line.tl.y = pixit_line.br.y;
        pixit_line.horizontal = 1;

        CLEAR_BORDER_LINE_FLAGS(border_line_flags);
        border_line_flags.sub_gw_from_t = 1;
        border_line_flags.sub_gw_from_b = 1;
        border_line_flags.add_gw_to_l = 1;
        border_line_flags.add_lw_to_l = 1;
        border_line_flags.sub_gw_from_r = 1;
        border_line_flags.border_style = p_style->para_style.border;

#ifdef SKS_BORDER_DEBUG
        p_rgb = &rgb_stash[11];
#endif
        host_paint_border_line(p_redraw_context, &pixit_line, p_rgb, border_line_flags);
    }
}

/******************************************************************************
*
* redraw request for a rectangle
* rectangle may never span more than a page
*
******************************************************************************/

T5_MSG_PROTO(extern, skel_event_redraw, _InoutRef_ P_SKELEVENT_REDRAW p_skelevent_redraw)
{
    ROW_ENTRY row_entry_top;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    trace_4(TRACE_APP_SKEL_DRAW,
            TEXT("skel_rect_redraw pixits tl: ") PIXIT_TFMT TEXT(",") PIXIT_TFMT TEXT(" br: ") PIXIT_TFMT TEXT(",") PIXIT_TFMT,
            p_skelevent_redraw->clip_skel_rect.tl.pixit_point.x, p_skelevent_redraw->clip_skel_rect.tl.pixit_point.y,
            p_skelevent_redraw->clip_skel_rect.br.pixit_point.x, p_skelevent_redraw->clip_skel_rect.br.pixit_point.y);
    trace_4(TRACE_APP_SKEL_DRAW,
            TEXT("skel_rect_redraw pages tl: ") PIXIT_TFMT TEXT(",") PIXIT_TFMT TEXT(" br: ") PIXIT_TFMT TEXT(",") PIXIT_TFMT,
            p_skelevent_redraw->clip_skel_rect.tl.page_num.x, p_skelevent_redraw->clip_skel_rect.tl.page_num.y,
            p_skelevent_redraw->clip_skel_rect.br.page_num.x, p_skelevent_redraw->clip_skel_rect.br.page_num.y);

    /* get top and bottom rows */
    if(status_ok(row_entry_at_skel_point(p_docu, &row_entry_top, &p_skelevent_redraw->clip_skel_rect.tl)))
    {
        ROW_ENTRY row_entry_bot;
        row_entry_from_skel_point(p_docu, &row_entry_bot, &p_skelevent_redraw->clip_skel_rect.br, FALSE);

        assert(row_entry_bot.row + 1 > row_entry_top.row);

        /* forget OS font handles - note after potential reformat caused by rowtab accesses above */
        fonty_cache_trash(&p_skelevent_redraw->redraw_context);

        if(p_skelevent_redraw->redraw_context.flags.printer  ||
           p_skelevent_redraw->redraw_context.flags.metafile ||
           p_skelevent_redraw->redraw_context.flags.drawfile )
        {
            style_region_class_limit_set(p_docu, REGION_UPPER);
        }

        trace_2(TRACE_APP_SKEL_DRAW,
                TEXT("skel_rect_redraw t row: ") ROW_TFMT TEXT(", b row: ") ROW_TFMT,
                row_entry_top.row, row_entry_bot.row);

        trace_4(TRACE_APP_SKEL_DRAW,
                TEXT("toprow t,b: ") PIXIT_TFMT TEXT(",") PIXIT_TFMT TEXT(" botrow t,b: ") PIXIT_TFMT TEXT(",") PIXIT_TFMT,
                row_entry_top.rowtab.edge_top.pixit, row_entry_top.rowtab.edge_bot.pixit,
                row_entry_bot.rowtab.edge_top.pixit, row_entry_bot.rowtab.edge_bot.pixit);

        if(p_skelevent_redraw->flags.show_content)
        {
            draw_background(p_docu, row_entry_top.row, row_entry_bot.row + 1, p_skelevent_redraw);
            if(grid_is_on(p_docu))
                draw_grid(p_docu, row_entry_top.row, row_entry_bot.row + 1, p_skelevent_redraw);
        }

        draw_these_objects(p_docu, row_entry_top.row, row_entry_bot.row + 1, p_skelevent_redraw);

        if(p_skelevent_redraw->redraw_context.flags.printer  ||
           p_skelevent_redraw->redraw_context.flags.metafile ||
           p_skelevent_redraw->redraw_context.flags.drawfile )
        {
            style_region_class_limit_set(p_docu, REGION_END);
        }

        /* forget OS font handles */
        fonty_cache_trash(&p_skelevent_redraw->redraw_context);
    }

    return(STATUS_OK);
}

/* end of sk_draw.c */
