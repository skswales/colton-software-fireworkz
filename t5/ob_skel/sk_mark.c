/* sk_mark.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Redraw code for skeleton */

/* MRJC January 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
internal routines
*/

static void
markers_clear(
    _DocuRef_   P_DOCU p_docu);

static void
skel_rect_from_markers(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SKEL_RECT p_skel_rect,
    _InRef_     PC_ARRAY_HANDLE p_h_markers);

/*
main events
*/

_Check_return_
static STATUS
sk_mark_msg_selection_clear(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->mark_info_cells.h_markers)
    {
        DOCU_AREA docu_area;
        docu_area_normalise(p_docu, &docu_area, p_docu_area_from_markers_first(p_docu));
        p_docu->cur = docu_area.tl;
        markers_clear(p_docu);
        markers_show(p_docu);
    }

    return(STATUS_OK);
}

MAEVE_EVENT_PROTO(static, maeve_event_sk_mark)
{
    UNREFERENCED_PARAMETER(p_data);
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_SELECTION_CLEAR:
        return(sk_mark_msg_selection_clear(p_docu));

    default:
        return(STATUS_OK);
    }
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_mark);

_Check_return_
static STATUS
sk_mark_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    p_docu->mark_info_cells.h_markers = 0;
    p_docu->mark_info_cells.h_markers_screen = 0;

    return(maeve_event_handler_add(p_docu, maeve_event_sk_mark, (CLIENT_HANDLE) 0));
}

_Check_return_
static STATUS
sk_mark_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    maeve_event_handler_del(p_docu, maeve_event_sk_mark, (CLIENT_HANDLE) 0);

    al_array_dispose(&p_docu->mark_info_cells.h_markers);
    al_array_dispose(&p_docu->mark_info_cells.h_markers_screen);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_sk_mark_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT1:
        return(sk_mark_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(sk_mark_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_mark)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_mark_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* called to update markers when dragging is finished
*
******************************************************************************/

extern void
anchor_to_markers_finish(
    _DocuRef_   P_DOCU p_docu)
{
    anchor_to_markers_update(p_docu);

    /* check for empty markers and throw em away */
    if(p_docu->mark_info_cells.h_markers)
    {
        if(docu_area_is_empty(p_docu_area_from_markers_first(p_docu)))
            markers_clear(p_docu);
        else
            status_assert(maeve_event(p_docu, T5_MSG_SELECTION_NEW, P_DATA_NONE));
    }
}

static void
anchor_to_markers(
    _DocuRef_   P_DOCU p_docu,
    /*out*/ P_MARKERS p_markers)
{
    P_DOCU_AREA p_docu_area = &p_markers->docu_area;

    *p_markers = p_docu->anchor_mark;

    /* stuff sub-object granularity if spanning more than one column */
    if(p_docu_area->whole_row || (p_docu_area->tl.slr.col != p_docu_area->br.slr.col))
    {
        p_docu_area->tl.object_position.object_id = OBJECT_ID_NONE;
        p_docu_area->br.object_position.object_id = OBJECT_ID_NONE;
    }

#if 1 /* fudge for ob_rec to limit marking to a single field */
    if((OBJECT_ID_REC == p_docu_area->tl.object_position.object_id) && (OBJECT_ID_REC == p_docu_area->br.object_position.object_id))
    {
        if(!slr_equal(&p_docu_area->br.slr, &p_docu_area->tl.slr)
           ||
           p_docu_area->tl.object_position.more_data != p_docu_area->br.object_position.more_data)
        {
            p_docu_area->br.slr = p_docu_area->tl.slr;
            p_docu_area->br.object_position.more_data = p_docu_area->tl.object_position.more_data;
            p_docu_area->tl.object_position.data = 0;
            p_docu_area->br.object_position.data = S32_MAX;
        }
    }
#endif

    /* sort markers into tl, br */
    if(!p_docu_area->whole_row)
        if(p_docu_area->tl.slr.col > p_docu_area->br.slr.col)
        {
            COL temp_col = p_docu_area->tl.slr.col;
            p_docu_area->tl.slr.col = p_docu_area->br.slr.col;
            p_docu_area->br.slr.col = temp_col;
        }

    if(!p_docu_area->whole_col)
        if(p_docu_area->tl.slr.row > p_docu_area->br.slr.row)
        {
            ROW temp_row = p_docu_area->tl.slr.row;
            OBJECT_POSITION temp_object_position = p_docu_area->tl.object_position;
            p_docu_area->tl.slr.row = p_docu_area->br.slr.row;
            p_docu_area->tl.object_position = p_docu_area->br.object_position;
            p_docu_area->br.slr.row = temp_row;
            p_docu_area->br.object_position = temp_object_position;
        }

    /* sort object positions */
    if(!(p_docu_area->whole_col || p_docu_area->whole_row))
    {
        if(slr_equal(&p_docu_area->tl.slr, &p_docu_area->br.slr))
        {
            if(object_position_compare(&p_docu_area->tl.object_position, &p_docu_area->br.object_position, OBJECT_POSITION_COMPARE_SE) >0)
            {
                OBJECT_POSITION temp = p_docu_area->tl.object_position;
                p_docu_area->tl.object_position = p_docu_area->br.object_position;
                p_docu_area->br.object_position = temp;
            }
        }

        if((OBJECT_ID_NONE != p_docu_area->br.object_position.object_id) && object_position_at_start(&p_docu_area->br.object_position))
        {
            p_docu_area->br.object_position.object_id = OBJECT_ID_NONE;
            p_docu_area->br.slr.row -= 1;
        }
    }

    /* make end points into exclusive co-ords */
    if(!p_docu_area->whole_row)
    {
        assert(p_docu->anchor_mark.docu_area.tl.slr.col >= 0);
        assert(p_docu->anchor_mark.docu_area.br.slr.col >= 0);
        p_docu_area->br.slr.col += 1;
    }

    if(!p_docu_area->whole_col)
    {
        assert(p_docu->anchor_mark.docu_area.tl.slr.row >= 0);
        assert(p_docu->anchor_mark.docu_area.br.slr.row >= 0);
        p_docu_area->br.slr.row += 1;
    }
}

/******************************************************************************
*
* add the anchor marks to the list as a new entry
*
******************************************************************************/

_Check_return_
extern STATUS
anchor_to_markers_new(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;
    P_MARKERS p_markers;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_markers), TRUE);

#if 1 /* not yet doing discontiguous marking */
    al_array_dispose(&p_docu->mark_info_cells.h_markers);
#endif

    if(NULL != (p_markers = al_array_extend_by(&p_docu->mark_info_cells.h_markers, MARKERS, 1, &array_init_block, &status)))
        anchor_to_markers(p_docu, p_markers);

    return(status);
}

/******************************************************************************
*
* update the most recent marker with a new anchor
*
******************************************************************************/

extern void
anchor_to_markers_update(
    _DocuRef_   P_DOCU p_docu)
{
    if(!array_elements(&p_docu->mark_info_cells.h_markers)) /* SKS 30sep93 after 1.04 - odd things can happen to clear the markers out, so let's recreate */
    {
        status_assert(anchor_to_markers_new(p_docu)); /* this calls anchor_to_markers itself */
        return;
    }

    anchor_to_markers(p_docu, p_markers_first(p_docu));
}

static void
markers_clear(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->mark_info_cells.h_markers)
    {
        al_array_dispose(&p_docu->mark_info_cells.h_markers);

        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_NEW, P_DATA_NONE));
    }
}

/******************************************************************************
*
* enusre that no markers are visible on
* screen without actually altering marker data
*
******************************************************************************/

#if 0

static void
markers_hide(
    _DocuRef_   P_DOCU p_docu)
{
    ARRAY_HANDLE h_markers_temp;

    h_markers_temp = p_docu->mark_info_cells.h_markers;
    p_docu->mark_info_cells.h_markers = 0;
    markers_show(p_docu);
    p_docu->mark_info_cells.h_markers = h_markers_temp;
}

#endif

/******************************************************************************
*
* reflect current marker state on screen
*
* takes markers now defined in the h_markers
* array and shows them on the screen
*
* then the h_markers array is copied to the
* h_markers_screen array
*
******************************************************************************/

extern void
markers_show(
    _DocuRef_   P_DOCU p_docu)
{
    SKEL_RECT skel_rect_all;

    if(p_docu->mark_info_cells.h_markers)
    {
        /* if markers are about to be shown, this will hide caret first
         * Windows needs its caret out of the way during inverts otherwise you get ghosts
         */
        caret_show_claim(p_docu, p_docu->focus_owner, FALSE);
    }

    skel_rect_empty_set(&skel_rect_all);

    /* subsume all new marker areas */
    skel_rect_from_markers(p_docu, &skel_rect_all, &p_docu->mark_info_cells.h_markers);

    /* and all marker areas already on screen */
    skel_rect_from_markers(p_docu, &skel_rect_all, &p_docu->mark_info_cells.h_markers_screen);

    if(!skel_rect_empty(&skel_rect_all))
    {
        RECT_FLAGS rect_flags;
        REDRAW_FLAGS redraw_flags;

        RECT_FLAGS_CLEAR(rect_flags);

        REDRAW_FLAGS_CLEAR(redraw_flags);
        redraw_flags.show_selection = TRUE;

        view_update_now(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect_all, rect_flags, redraw_flags, LAYER_CELLS);

        trace_4(TRACE_APP_SKEL_DRAW,
                TEXT("markers_show skel_rect_all pixits: ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT,
                skel_rect_all.tl.pixit_point.x,
                skel_rect_all.tl.pixit_point.y,
                skel_rect_all.br.pixit_point.x,
                skel_rect_all.br.pixit_point.y);
        trace_4(TRACE_APP_SKEL_DRAW,
                TEXT("markers_show skel_rect_all pages: ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT,
                skel_rect_all.tl.page_num.x,
                skel_rect_all.tl.page_num.y,
                skel_rect_all.br.page_num.x,
                skel_rect_all.br.page_num.y);
    }

    /* dispose of markers that were */
    al_array_dispose(&p_docu->mark_info_cells.h_markers_screen);

    /* update our record of markers on screen */
    status_assert(al_array_duplicate(&p_docu->mark_info_cells.h_markers_screen, &p_docu->mark_info_cells.h_markers));

    if(!p_docu->mark_info_cells.h_markers)
    {
        /* if markers have been cleared, this will show caret after:
         * Windows needs its caret out of the way during inverts otherwise you get ghosts
         */
        caret_show_claim(p_docu, p_docu->focus_owner, FALSE);
    }
}

/******************************************************************************
*
* produce a skel rect from a marker list,
* clipped to the visible rows and columns
*
******************************************************************************/

static void
skel_rect_from_markers(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SKEL_RECT p_skel_rect,
    _InRef_     PC_ARRAY_HANDLE p_h_markers)
{
    DOCU_AREA docu_area_visible;
    ARRAY_HANDLE h_row_range_visible;

    if(0 == array_elements(p_h_markers))
        return;

    docu_area_visible.whole_row = 1;
    docu_area_visible.tl.slr.row = MAX_ROW;
    docu_area_visible.br.slr.row = 0;
    docu_area_visible.whole_col = 0;
    docu_area_visible.tl.object_position.object_id =
    docu_area_visible.br.object_position.object_id = OBJECT_ID_NONE;

    if(status_ok(skel_get_visible_row_ranges(p_docu, &h_row_range_visible)))
    {
        const ARRAY_INDEX n_elements = array_elements(&h_row_range_visible);
        ARRAY_INDEX i;
        PC_ROW_RANGE p_row_range = array_rangec(&h_row_range_visible, ROW_RANGE, 0, n_elements);
        const ARRAY_INDEX n_markers = array_elements(p_h_markers);
        ARRAY_INDEX ix_markers;
        PC_MARKERS p_markers = array_rangec(p_h_markers, MARKERS, 0, n_markers);

        for(i = 0; i < n_elements; ++i, ++p_row_range)
        {
            if( docu_area_visible.tl.slr.row > p_row_range->top)
                docu_area_visible.tl.slr.row = p_row_range->top;
            if( docu_area_visible.br.slr.row < p_row_range->bot)
                docu_area_visible.br.slr.row = p_row_range->bot;
        }

        trace_2(TRACE_APP_SKEL_DRAW,
                TEXT("skel_rect_from_markers rows visible: ") ROW_TFMT TEXT(",") ROW_TFMT,
                docu_area_visible.tl.slr.row,
                docu_area_visible.br.slr.row);

        /* subsume all new marker areas */
        for(ix_markers = 0; ix_markers < n_markers; ++ix_markers, ++p_markers)
        {
            DOCU_AREA docu_area_out;

            if(docu_area_intersect_docu_area_out(&docu_area_out, &p_markers->docu_area, &docu_area_visible))
            {
                SKEL_RECT skel_rect;

                trace_4(TRACE_APP_SKEL_DRAW,
                        TEXT("skel_rect_from_markers markers tl: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(", br: ") COL_TFMT TEXT(",") ROW_TFMT,
                        p_markers->docu_area.tl.slr.col,
                        p_markers->docu_area.tl.slr.row,
                        p_markers->docu_area.br.slr.col,
                        p_markers->docu_area.br.slr.row);
                trace_4(TRACE_APP_SKEL_DRAW,
                        TEXT("skel_rect_from_markers out tl: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(", br: ") COL_TFMT TEXT(",") ROW_TFMT,
                        docu_area_out.tl.slr.col,
                        docu_area_out.tl.slr.row,
                        docu_area_out.br.slr.col,
                        docu_area_out.br.slr.row);

                skel_rect_from_docu_area(p_docu, &skel_rect, &docu_area_out);

                skel_rect_union(p_skel_rect, p_skel_rect, &skel_rect);
            }
        }

        al_array_dispose(&h_row_range_visible);
    }
}

/* end of sk_mark.c */
