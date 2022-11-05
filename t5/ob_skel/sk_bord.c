/* sk_bord.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Event routines for borders in a document view */

/* RCM Jan 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef         __vi_edge_h
#include "ob_skel/vi_edge.h"
#endif

/*
internal structure
*/

/* border_horz grab points */
typedef enum BORDER_HORZ_POSITION
{
    OVER_COLUMN_DEAD_SPACE = -1,

    OVER_COLUMN_CENTRE = 0,
    OVER_COLUMN_WIDTH_ADJUSTOR,

    BORDER_HORZ_POSITION_COUNT
}
BORDER_HORZ_POSITION, * P_BORDER_HORZ_POSITION;

/* border_vert grab points */
typedef enum BORDER_VERT_POSITION
{
    OVER_ROW_DEAD_SPACE = -1,

    OVER_ROW_CENTRE = 0,
    OVER_ROW_HEIGHT_ADJUSTOR,

    BORDER_VERT_POSITION_COUNT
}
BORDER_VERT_POSITION, * P_BORDER_VERT_POSITION;

typedef struct BORDER_DRAG_STATUS
{
    PC_USTR ustr_style_prefix;

    FP_PIXIT fp_pixits_per_unit;    /* PIXITS_PER_CM     , PIXITS_PER_CM     , PIXITS_PER_INCH      */
    STATUS unit_message;            /* MSG_DRAG_STATUS_CM, MSG_DRAG_STATUS_MM, MSG_DRAG_STATUS_INCH */

    STATUS drag_message;            /* e.g. MSG_DRAG_STATUS_RULER_HORZ_TAB which is "Left tab" */
    STATUS post_message;
}
BORDER_DRAG_STATUS, * P_BORDER_DRAG_STATUS;

typedef struct CB_DATA_BORDER_HORZ_ADJUST_COLUMN_WIDTH
{
    S32 reason_code;

    COL col;
    ROW row;
    S32 pag;

    PIXIT deltax;
    PIXIT delta_min;
    PIXIT delta_max;

    PIXIT width_left, width_right;

    FP_PIXIT click_stop_step;

    PIXIT param;

    BORDER_DRAG_STATUS drag_status;    /* add deltax to param field before use */
}
CB_DATA_BORDER_HORZ_ADJUST_COLUMN_WIDTH, * P_CB_DATA_BORDER_HORZ_ADJUST_COLUMN_WIDTH;

typedef struct CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT
{
    S32 reason_code;

    COL col;
    ROW row;
    S32 pag;
    PIXIT deltay;
    PIXIT delta_min;
    PIXIT delta_max;

    FP_PIXIT click_stop_step;

    PIXIT param;

    BORDER_DRAG_STATUS drag_status;
}
CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT, * P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT;

/*
internal routines
*/

static void
border_show_status(
    _DocuRef_   P_DOCU p_docu,
    P_BORDER_DRAG_STATUS p_drag_status,
    _InVal_     PIXIT value,
    _InVal_     S32 col_or_row);

_Check_return_ _Success_(status_ok(return))
static STATUS
border_horz_drag_limits(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page,
    _InVal_     COL col,
    _InVal_     ROW row,
    _OutRef_    P_CB_DATA_BORDER_HORZ_ADJUST_COLUMN_WIDTH p_horz_blk,
    _InVal_     S32 ratio);

static void
border_horz_where(
    _DocuRef_   P_DOCU p_docu,
    PC_SKELEVENT_CLICK p_skelevent_click,
    P_BORDER_HORZ_POSITION p_position,
    P_COL p_col_number,
    _InVal_     S32 set_pointer_shape,
    _InVal_     S32 set_status_line);

_Check_return_
static STATUS
border_horz_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL show_dragging /*,style info font colour etc*/,
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw,
    _InRef_     PC_STYLE p_border_style);

_Check_return_
static STATUS
apply_new_col_width(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col,
    _InVal_     ROW row,
    _InVal_     PIXIT drag_delta_x);

_Check_return_
static STATUS
border_vert_drag_limits(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page,
    _InVal_     COL col,
    _InVal_     ROW row,
    _OutRef_    P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT p_blk);

_Check_return_
static STATUS
border_vert_row_apply_delta(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row,
    _InVal_     PIXIT drag_delta_y);

static void
border_vert_where(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click,
    P_BORDER_VERT_POSITION p_position,
    P_ROW p_row_number,
    _InVal_     S32 set_pointer_shape,
    _InVal_     S32 set_status_line);

_Check_return_
static STATUS
border_vert_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL show_dragging /*,style info font colour etc*/,
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw,
    _InRef_     PC_STYLE p_border_style);

typedef struct BORDER_STATUS
{
  /*POINTER_SHAPE pointer_shape;*/
    STATUS        drag_message;
    STATUS        post_message;
}
BORDER_STATUS;

static BORDER_STATUS hborder_status[BORDER_HORZ_POSITION_COUNT] =
{
    /* border_horz markers */
/*OVER_COLUMN_CENTRE*/         { MSG_STATUS_BORDER_HORZ_COL_AND_WIDTH, 0 },
/*OVER_COLUMN_WIDTH_ADJUSTOR*/ { MSG_STATUS_BORDER_HORZ_COL_AND_WIDTH, MSG_STATUS_DRAG_COL }
};

static BORDER_STATUS vborder_status[BORDER_VERT_POSITION_COUNT] =
{
    /* border_vert markers */
/*OVER_ROW_CENTRE*/          { MSG_STATUS_BORDER_VERT_ROW_AND_HEIGHT, 0 },
/*OVER_ROW_HEIGHT_ADJUSTOR*/ { MSG_STATUS_BORDER_VERT_ROW_AND_HEIGHT, MSG_STATUS_DRAG_ROW }
};

/*
only one of these can be active at once; identified by host_drag_in_progress() (union is overkill!)
*/

static CB_DATA_BORDER_HORZ_ADJUST_COLUMN_WIDTH adjust_blk_border_horz_column_width;
static CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT   adjust_blk_border_vert_row_height;

/******************************************************************************
*
* This master event handler acts for all the border_horz and border_vert
* windows in all the (0..n) views of this document
*
******************************************************************************/

_Check_return_
static STATUS
sk_bord_docu_colrow(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr_old)
{
    BOOL col_change = (p_docu->cur.slr.col != p_slr_old->col);
    BOOL row_change = (p_docu->cur.slr.row != p_slr_old->row);

    if((p_slr_old->row >= n_rows(p_docu)) || (p_slr_old->col >= n_cols_logical(p_docu)))
    {
        /* One of the columns/rows we wish to access has been deleted. This change message probably
         * occured as a result of deleteing the last column/row of the document. Mark claims he will
         * have triggered an appropriate BORDER_HORZ/BORDER_VERT redraw.
         *
         * aside: The ruler event handler causes a redraw of RULER_HORZ, but we need do nothing.
         */

        /*EMPTY*/
    }
    else
    {
        /* new and old cells exist, so do update_later of borders */
        SKEL_RECT old_slot, new_slot;

        skel_rect_from_slr(p_docu, &old_slot, p_slr_old);
        skel_rect_from_slr(p_docu, &new_slot, &p_docu->cur.slr);

        if(col_change && !row_change)
        {
            SKEL_RECT  skel_rect;
            RECT_FLAGS rect_flags;

            /*skel_rect.tl.page_num.y    = skel_rect.br.page_num.y    = 0;*/
            /*skel_rect.tl.pixit_point.y = skel_rect.br.pixit_point.y = 0;*/

            RECT_FLAGS_CLEAR(rect_flags);
            rect_flags.extend_down_window = 1;

            skel_rect = old_slot;
            skel_rect.tl.pixit_point.y = skel_rect.br.pixit_point.y = 0;

            view_update_later(p_docu, UPDATE_BORDER_HORZ, &skel_rect, rect_flags);

            skel_rect = new_slot;
            skel_rect.tl.pixit_point.y = skel_rect.br.pixit_point.y = 0;

            view_update_later(p_docu, UPDATE_BORDER_HORZ, &skel_rect, rect_flags);
        }
        else if(row_change)
        {
            SKEL_RECT  skel_rect;
            RECT_FLAGS rect_flags;

            /* avoid unnecessary top border redraws MRJC 2.8.93 */
            if(col_change
               ||
               skel_point_compare(&new_slot.tl, &old_slot.tl, x)
               ||
               skel_point_compare(&new_slot.br, &old_slot.br, x))
            {
                skel_rect.tl.pixit_point.x = 0;
                skel_rect.tl.pixit_point.y = 0;
                skel_rect.tl.page_num.x = 0;
                skel_rect.tl.page_num.y = 0;
                skel_rect.br = skel_rect.tl;

                RECT_FLAGS_CLEAR(rect_flags);
                rect_flags.extend_right_window = rect_flags.extend_down_window = 1;

                view_update_later(p_docu, UPDATE_BORDER_HORZ, &skel_rect, rect_flags);
            }

            skel_rect.tl.page_num.x    = skel_rect.br.page_num.x    = 0;
            skel_rect.tl.pixit_point.x = skel_rect.br.pixit_point.x = 0;

            RECT_FLAGS_CLEAR(rect_flags);
            rect_flags.extend_right_window = 1;

            skel_rect.tl.page_num.y    = old_slot.tl.page_num.y;
            skel_rect.tl.pixit_point.y = old_slot.tl.pixit_point.y;
            skel_rect.br.page_num.y    = old_slot.br.page_num.y;
            skel_rect.br.pixit_point.y = old_slot.br.pixit_point.y;

            view_update_later(p_docu, UPDATE_BORDER_VERT, &skel_rect, rect_flags);

            skel_rect.tl.page_num.y    = new_slot.tl.page_num.y;
            skel_rect.tl.pixit_point.y = new_slot.tl.pixit_point.y;
            skel_rect.br.page_num.y    = new_slot.br.page_num.y;
            skel_rect.br.pixit_point.y = new_slot.br.pixit_point.y;

            view_update_later(p_docu, UPDATE_BORDER_VERT, &skel_rect, rect_flags);
        }
    }

    return(STATUS_OK);
}

MAEVE_EVENT_PROTO(static, maeve_event_sk_bord)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_DOCU_COLROW:
        return(sk_bord_docu_colrow(p_docu, (PC_SLR) p_data));

    default:
        return(STATUS_OK);
    }
}

/*
Event handler for a column border rendered within a page c.f. edge_window_event_border_horz()
*/

#define p_style_for_margin_col p_style_for_border_horz /* for now */

T5_MSG_PROTO(static, margin_col_event_redraw, P_SKELEVENT_REDRAW p_skelevent_redraw)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    return(border_horz_redraw(p_docu, FALSE, p_skelevent_redraw, p_style_for_margin_col())); /* drag operations in window are NOT shown */
}

PROC_EVENT_PROTO(extern, proc_event_margin_col)
{
    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(margin_col_event_redraw(p_docu, t5_message, (P_SKELEVENT_REDRAW) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* Events for the border_horz come here
*
******************************************************************************/

/* goto column col_number, current row */

_Check_return_
static STATUS
border_horz_event_click_left_single_column_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_number)
{
    SKELCMD_GOTO skelcmd_goto;

    skelcmd_goto.slr.col = col_number;
    skelcmd_goto.slr.row = p_docu->cur.slr.row;
    skelcmd_goto.keep_selection = FALSE;

    skel_point_from_slr_tl(p_docu, &skelcmd_goto.skel_point, &skelcmd_goto.slr);

    return(object_skel(p_docu, T5_MSG_GOTO_SLR, &skelcmd_goto));
}

/* select column col_number */

_Check_return_
static STATUS
border_horz_event_click_left_double_column_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_number)
{
    p_docu->anchor_mark.docu_area.tl.slr.col = col_number;
    p_docu->anchor_mark.docu_area.tl.slr.row = 0;
    p_docu->anchor_mark.docu_area.tl.object_position.object_id = OBJECT_ID_NONE;

    p_docu->anchor_mark.docu_area.br.slr.col = col_number;
    p_docu->anchor_mark.docu_area.br.slr.row = n_rows(p_docu) - 1;
    p_docu->anchor_mark.docu_area.br.object_position.object_id = OBJECT_ID_NONE;

    p_docu->anchor_mark.docu_area.whole_col = TRUE;
    p_docu->anchor_mark.docu_area.whole_row = FALSE;

    status_assert(anchor_to_markers_new(p_docu));

    anchor_to_markers_finish(p_docu);

    markers_show(p_docu);

    return(STATUS_OK);
}

/* select column col_number and start drag */

_Check_return_
static STATUS
border_horz_event_click_left_drag_column_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_number)
{
    p_docu->anchor_mark.docu_area.tl.slr.col = col_number;
    p_docu->anchor_mark.docu_area.tl.slr.row = 0;
    p_docu->anchor_mark.docu_area.tl.object_position.object_id = OBJECT_ID_NONE;

    p_docu->anchor_mark.docu_area.br.slr.col = col_number;
    p_docu->anchor_mark.docu_area.br.slr.row = n_rows(p_docu) - 1;
    p_docu->anchor_mark.docu_area.br.object_position.object_id = OBJECT_ID_NONE;

    p_docu->anchor_mark.docu_area.whole_col = TRUE;
    p_docu->anchor_mark.docu_area.whole_row = FALSE;

    status_assert(anchor_to_markers_new(p_docu));

    anchor_to_markers_update(p_docu);

    markers_show(p_docu);

    adjust_blk_border_horz_column_width.reason_code = CB_CODE_BORDER_HORZ_EXTEND_SELECTION;

    host_drag_start(&adjust_blk_border_horz_column_width);

    return(STATUS_OK);
}

_Check_return_
static STATUS
border_horz_event_click_right_column_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message, /* single, double or drag */
    _InVal_     COL col_number)
{
    /* set up anchor start position - use current position if no selection */
    if(!p_docu->mark_info_cells.h_markers)
        p_docu->anchor_mark.docu_area.tl = p_docu->cur;

    p_docu->anchor_mark.docu_area.br.slr.col = col_number;
    p_docu->anchor_mark.docu_area.br.slr.row = n_rows(p_docu) - 1;
    p_docu->anchor_mark.docu_area.br.object_position.object_id = OBJECT_ID_NONE;

    p_docu->anchor_mark.docu_area.whole_col = TRUE;
    p_docu->anchor_mark.docu_area.whole_row = FALSE;

    /* put anchor block in list */
    if(!p_docu->mark_info_cells.h_markers)
        status_assert(anchor_to_markers_new(p_docu));

    if(T5_EVENT_CLICK_RIGHT_DRAG == t5_message)
    {
        anchor_to_markers_update(p_docu);

        markers_show(p_docu);

        adjust_blk_border_horz_column_width.reason_code = CB_CODE_BORDER_HORZ_EXTEND_SELECTION;

        host_drag_start(&adjust_blk_border_horz_column_width);
    }
    else
    {
        anchor_to_markers_finish(p_docu);

        markers_show(p_docu);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
border_horz_event_click_single_column_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     COL col_number)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_SINGLE;
    const T5_MESSAGE t5_message_effective = right_message_if_shift(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(border_horz_event_click_right_column_centre(p_docu, t5_message_effective, col_number));

    return(border_horz_event_click_left_single_column_centre(p_docu, col_number));
}

_Check_return_
static STATUS
border_horz_event_click_double_column_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     COL col_number)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DOUBLE;
    const T5_MESSAGE t5_message_effective = right_message_if_shift(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(border_horz_event_click_right_column_centre(p_docu, t5_message_effective, col_number));

    return(border_horz_event_click_left_double_column_centre(p_docu, col_number));
}

_Check_return_
static STATUS
border_horz_event_click_drag_column_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     COL col_number)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DRAG;
    const T5_MESSAGE t5_message_effective = right_message_if_shift(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(border_horz_event_click_right_column_centre(p_docu, t5_message_effective, col_number));

    return(border_horz_event_click_left_drag_column_centre(p_docu, col_number));
}

_Check_return_
static STATUS
border_horz_event_click_column_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     COL col_number)
{
    switch(t5_message)
    {
    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
        return(border_horz_event_click_single_column_centre(p_docu, t5_message, p_skelevent_click, col_number));

    case T5_EVENT_CLICK_LEFT_DOUBLE:
    case T5_EVENT_CLICK_RIGHT_DOUBLE:
        return(border_horz_event_click_double_column_centre(p_docu, t5_message, p_skelevent_click, col_number));

    default: default_unhandled();
#if CHECKING
    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_DRAG:
#endif
        return(border_horz_event_click_drag_column_centre(p_docu, t5_message, p_skelevent_click, col_number));
    }
}

_Check_return_
static STATUS
border_horz_event_click_left_double_column_width_adjustor(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_number)
{
    if(p_docu->cur.slr.col != col_number)
    {   /* 'goto' the new column, cos auto width applies to the current column */
        SKELCMD_GOTO skelcmd_goto;

        skelcmd_goto.slr.col = col_number;
        skelcmd_goto.slr.row = p_docu->cur.slr.row;
        skelcmd_goto.keep_selection = TRUE;

        skel_point_from_slr_tl(p_docu, &skelcmd_goto.skel_point, &skelcmd_goto.slr);

        status_return(object_skel(p_docu, T5_MSG_GOTO_SLR, &skelcmd_goto));
        /*>>>this may need changing if 'goto' tries to prevent entry to zero height columns */
    }

    return(execute_command(p_docu, T5_CMD_AUTO_WIDTH, _P_DATA_NONE(P_ARGLIST_HANDLE), OBJECT_ID_SKEL));
}

_Check_return_
static STATUS
border_horz_event_click_right_double_column_width_adjustor(
    _DocuRef_   P_DOCU p_docu)
{
    return(execute_command(p_docu, T5_CMD_STRADDLE_HORZ, _P_DATA_NONE(P_ARGLIST_HANDLE), OBJECT_ID_SKEL));
}

_Check_return_
static STATUS
border_horz_event_click_double_column_width_adjustor(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     COL col_number)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DOUBLE;
    const T5_MESSAGE t5_message_effective = right_message_if_ctrl(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(border_horz_event_click_right_double_column_width_adjustor(p_docu));

    return(border_horz_event_click_left_double_column_width_adjustor(p_docu, col_number));
}

_Check_return_
static STATUS
border_horz_event_click_drag_column_width_adjustor(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     COL col_number)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DRAG;
    const T5_MESSAGE t5_message_effective = right_message_if_ctrl(t5_message, t5_message_right, p_skelevent_click);
    BOOL split = (T5_EVENT_CLICK_RIGHT_DRAG == t5_message_effective) && ((col_number+1) < n_cols_logical(p_docu));

    /* 'goto' the new column, cos resizes apply to current column */
    if(p_docu->cur.slr.col != col_number)
    {
        SKELCMD_GOTO skelcmd_goto;

        skelcmd_goto.slr.col = col_number;
        skelcmd_goto.slr.row = p_docu->cur.slr.row;
        skelcmd_goto.keep_selection = FALSE;

        skel_point_from_slr_tl(p_docu, &skelcmd_goto.skel_point, &skelcmd_goto.slr);

        status_return(object_skel(p_docu, T5_MSG_GOTO_SLR, &skelcmd_goto));
        /*>>>this may need changing if 'goto' tries to prevent entry to zero width columns */
    }

    if(status_ok(border_horz_drag_limits(p_docu, p_skelevent_click->skel_point.page_num.x, col_number, p_docu->cur.slr.row, &adjust_blk_border_horz_column_width, split)))
    {
        adjust_blk_border_horz_column_width.reason_code = split
                ? CB_CODE_BORDER_HORZ_ADJUST_COLUMN_SPLIT
                : CB_CODE_BORDER_HORZ_ADJUST_COLUMN_WIDTH;

        host_drag_start(&adjust_blk_border_horz_column_width);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
border_horz_event_click_column_width_adjustor(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     COL col_number)
{
    switch(t5_message)
    {
    case T5_EVENT_CLICK_LEFT_DOUBLE:
    case T5_EVENT_CLICK_RIGHT_DOUBLE:
        return(border_horz_event_click_double_column_width_adjustor(p_docu, t5_message, p_skelevent_click, col_number));

    case T5_EVENT_CLICK_LEFT_DRAG: /* Increase/decrease column width */
    case T5_EVENT_CLICK_RIGHT_DRAG: /* Move dividing line between this column and the next */
        return(border_horz_event_click_drag_column_width_adjustor(p_docu, t5_message, p_skelevent_click, col_number));

    default: default_unhandled();
#if CHECKING
    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
#endif
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, border_horz_event_click, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    BORDER_HORZ_POSITION position;
    COL col_number;

    p_skelevent_click->processed = 1;

    border_horz_where(p_docu, p_skelevent_click, &position /*filled*/, &col_number, TRUE, TRUE);

    switch(position)
    {
    case OVER_COLUMN_CENTRE:
        return(border_horz_event_click_column_centre(p_docu, t5_message, p_skelevent_click, col_number));

    case OVER_COLUMN_WIDTH_ADJUSTOR:
        return(border_horz_event_click_column_width_adjustor(p_docu, t5_message, p_skelevent_click, col_number));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, border_horz_event_click_drag_movement, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const P_CB_DATA_BORDER_HORZ_ADJUST_COLUMN_WIDTH p_horz_blk = (P_CB_DATA_BORDER_HORZ_ADJUST_COLUMN_WIDTH) p_skelevent_click->data.drag.p_reason_data;

    trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_border_horz T5_EVENT_CLICK_DRAG_MOVEMENT"));

    switch(p_horz_blk->reason_code)
    {
    default: default_unhandled();
#if CHECKING
    case CB_CODE_BORDER_HORZ_ADJUST_COLUMN_SPLIT:
    case CB_CODE_BORDER_HORZ_ADJUST_COLUMN_WIDTH:
#endif
        {
        PIXIT delta_x;
        BOOL show = 0;

        if(T5_EVENT_CLICK_DRAG_MOVEMENT == t5_message)
        {
            delta_x = click_stop_limited(p_skelevent_click->data.drag.pixit_delta.x, p_horz_blk->delta_min, p_horz_blk->delta_max, p_horz_blk->delta_min, &p_horz_blk->click_stop_step);

            if(p_horz_blk->deltax != delta_x)
            {
                /* redraw BORDER_HORZ and RULER_HORZ for this page */
                SKEL_RECT skel_rect;
                RECT_FLAGS rect_flags;
                REDRAW_FLAGS redraw_flags;

                p_horz_blk->deltax = delta_x;

                show = 1;

                skel_rect.tl.pixit_point.x = 0;
                skel_rect.tl.pixit_point.y = 0;
                skel_rect.tl.page_num.x = p_horz_blk->pag;
                skel_rect.tl.page_num.y = 0;
                skel_rect.br = skel_rect.tl;

                RECT_FLAGS_CLEAR(rect_flags);
                rect_flags.extend_right_page = rect_flags.extend_down_window = 1;

                REDRAW_FLAGS_CLEAR(redraw_flags);
                redraw_flags.show_selection = TRUE;

                view_update_now(p_docu, UPDATE_BORDER_HORZ, &skel_rect, rect_flags, redraw_flags, LAYER_CELLS);
#if FALSE
                view_update_later(p_docu, UPDATE_RULER_HORZ, &skel_rect, rect_flags);
#endif
            }
        }
        else
        {
            delta_x = 0;

            show = 1;
        }

        if(show)
        {   /* show column width on status line */
            border_show_status(p_docu, &p_horz_blk->drag_status, p_horz_blk->param + p_horz_blk->deltax, p_horz_blk->col);
        }

        break;
        }

    case CB_CODE_BORDER_HORZ_EXTEND_SELECTION:
        {
        BORDER_HORZ_POSITION position;
        COL col_number;

        border_horz_where(p_docu, p_skelevent_click, &position /*filled*/, &col_number /*filled*/, FALSE, FALSE);

        if(position != OVER_COLUMN_DEAD_SPACE)
        {
            p_docu->anchor_mark.docu_area.br.slr.col = col_number;
            anchor_to_markers_update(p_docu);
            markers_show(p_docu);
        }

        break;
        }
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, border_horz_event_click_drag_finished, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const P_CB_DATA_BORDER_HORZ_ADJUST_COLUMN_WIDTH p_horz_blk = (P_CB_DATA_BORDER_HORZ_ADJUST_COLUMN_WIDTH) p_skelevent_click->data.drag.p_reason_data;
    BOOL redraw_border;
    STATUS status = STATUS_OK;

    trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_border_horz T5_EVENT_CLICK_DRAG_FINISHED"));

    if(T5_EVENT_CLICK_DRAG_FINISHED == t5_message)
    {
        redraw_border = 0;

        switch(p_horz_blk->reason_code)
        {
        default: default_unhandled();
#if CHECKING
        case CB_CODE_BORDER_HORZ_ADJUST_COLUMN_SPLIT:
#endif
            {
            PIXIT drag_delta_x = click_stop_limited(p_skelevent_click->data.drag.pixit_delta.x, p_horz_blk->delta_min, p_horz_blk->delta_max, p_horz_blk->delta_min, &p_horz_blk->click_stop_step);
            COL_WIDTH_ADJUST col_width_adjust;
            /* dragging the split point of col/col+1 means apply +drag_delta to col and -drag_delta to col+1, so... */

            col_width_adjust.col         = p_horz_blk->col;
            col_width_adjust.width_left  = p_horz_blk->width_left  + drag_delta_x;
            col_width_adjust.width_right = p_horz_blk->width_right - drag_delta_x;

            if(status_fail(status = object_skel(p_docu, T5_MSG_COL_WIDTH_ADJUST, &col_width_adjust)))
                redraw_border = 1;

            break;
            }

        case CB_CODE_BORDER_HORZ_ADJUST_COLUMN_WIDTH:
            {
            PIXIT drag_delta_x = click_stop_limited(p_skelevent_click->data.drag.pixit_delta.x, p_horz_blk->delta_min, p_horz_blk->delta_max, p_horz_blk->delta_min, &p_horz_blk->click_stop_step);

            if(status_fail(status = apply_new_col_width(p_docu, p_horz_blk->col, p_horz_blk->row, drag_delta_x)))
                redraw_border = 1;

            break;
            }

        case CB_CODE_BORDER_HORZ_EXTEND_SELECTION:
            {
            BORDER_HORZ_POSITION position;
            COL col_number;

            border_horz_where(p_docu, p_skelevent_click, &position /*filled*/, &col_number /*filled*/, TRUE, TRUE);

            if(position != OVER_COLUMN_DEAD_SPACE)
            {
                p_docu->anchor_mark.docu_area.br.slr.col = col_number;
                anchor_to_markers_finish(p_docu);
                markers_show(p_docu);
            }

            break;
            }
        }
    }
    else
        redraw_border = 1;

    status_line_clear(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING);

    if(redraw_border)
    {
        SKEL_RECT skel_rect;
        RECT_FLAGS rect_flags;

        skel_rect.tl.pixit_point.x = 0;
        skel_rect.tl.pixit_point.y = 0;
        skel_rect.tl.page_num.x = p_horz_blk->pag;
        skel_rect.tl.page_num.y = 0;
        skel_rect.br = skel_rect.tl;

        RECT_FLAGS_CLEAR(rect_flags);
        rect_flags.extend_right_page = rect_flags.extend_down_page  = 1;

        view_update_later(p_docu, UPDATE_BORDER_HORZ, &skel_rect, rect_flags);
    }

    return(status);
}

T5_MSG_PROTO(static, border_horz_event_pointer_movement, _InRef_ PC_SKELEVENT_CLICK p_skelevent_click)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    trace_v0(TRACE_APP_CLICK,
            (T5_EVENT_POINTER_ENTERS_WINDOW == t5_message)
                ? TEXT("edge_window_event_border_horz T5_EVENT_POINTER_ENTERS_WINDOW")
                : TEXT("edge_window_event_border_horz T5_EVENT_POINTER_MOVEMENT"));

    border_horz_where(p_docu, p_skelevent_click, NULL, NULL, TRUE, TRUE);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, border_horz_event_pointer_leaves_window, _InRef_ PC_SKELEVENT_CLICK p_skelevent_click)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InVal_(p_skelevent_click);

    trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_border_horz T5_EVENT_POINTER_LEAVES_WINDOW"));

    status_line_clear(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING);

    return(STATUS_OK);
}

#define     COLTEXT_MAX 11
#define BUF_COLTEXT_MAX 12

T5_MSG_PROTO(static, border_horz_event_redraw, P_SKELEVENT_REDRAW p_skelevent_redraw)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* Fix the bottom right to define the height of the border region */
    p_skelevent_redraw->work_skel_rect.br.pixit_point.y = view_border_pixit_size(p_skelevent_redraw->redraw_context.p_view, TRUE);

    return(border_horz_redraw(p_docu, TRUE, p_skelevent_redraw, p_style_for_border_horz())); /* drag operations in this window (border_horz) are shown */
}

PROC_EVENT_PROTO(extern, edge_window_event_border_horz)
{
    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(border_horz_event_redraw(p_docu, t5_message, (P_SKELEVENT_REDRAW) p_data));

    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_LEFT_DOUBLE:
    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_DOUBLE:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        return(border_horz_event_click(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_STARTED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
        return(border_horz_event_click_drag_movement(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_ABORTED:
    case T5_EVENT_CLICK_DRAG_FINISHED:
        return(border_horz_event_click_drag_finished(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_POINTER_ENTERS_WINDOW:
    case T5_EVENT_POINTER_MOVEMENT:
        return(border_horz_event_pointer_movement(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_POINTER_LEAVES_WINDOW:
        return(border_horz_event_pointer_leaves_window(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* Redraw horizontal border
*
* Called by edge_window_event_border_horz()
*           proc_event_margin_col()
*
******************************************************************************/

_Check_return_
_Ret_valid_
static inline PC_RGB
colour_of_border_frame(
    _InRef_     PC_STYLE p_border_style)
{
    return(
        style_bit_test(p_border_style, STYLE_SW_PS_RGB_GRID_LEFT)
            ? &p_border_style->para_style.rgb_grid_left
            : &rgb_stash[COLOUR_OF_BORDER_FRAME]);
}

_Check_return_
static RGB
colour_of_border_adjust_to_current(
    _InRef_     PC_RGB p_fill_colour,
    _InRef_     PC_RGB p_text_colour)
{
    RGB border_colour_current = *p_fill_colour;
    const S32 mix = 85;
    border_colour_current.r = (U8) (((border_colour_current.r * mix) + ((p_text_colour->r * (100 - mix)))) / 100);
    border_colour_current.g = (U8) (((border_colour_current.g * mix) + ((p_text_colour->g * (100 - mix)))) / 100);
    border_colour_current.b = (U8) (((border_colour_current.b * mix) + ((p_text_colour->b * (100 - mix)))) / 100);
    return(border_colour_current);
}

static BORDER_FLAGS
init_border_horz_border_flags = { { TRUE, FALSE }, { TRUE, FALSE }, { TRUE, FALSE }, { TRUE, FALSE } };

_Check_return_
static STATUS
border_horz_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL show_dragging /*,style info font colour etc*/,
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw,
    _InRef_     PC_STYLE p_border_style)
{
    STATUS status = STATUS_OK;
    const P_REDRAW_CONTEXT p_redraw_context = &p_skelevent_redraw->redraw_context;
    ARRAY_HANDLE col_array;
    PIXIT_RECT col_rect;
    BORDER_FLAGS border_flags = init_border_horz_border_flags; /*>>>should this be a parameter???*/
    S32 button = show_dragging;

    assert((p_skelevent_redraw->clip_skel_rect.tl.page_num.x == p_skelevent_redraw->clip_skel_rect.br.page_num.x) &&
           (p_skelevent_redraw->clip_skel_rect.tl.page_num.y == p_skelevent_redraw->clip_skel_rect.br.page_num.y));

    col_rect.br.x = p_skelevent_redraw->work_skel_rect.tl.pixit_point.x; /* in case we have no columns */

    col_rect.tl.y = p_skelevent_redraw->work_skel_rect.tl.pixit_point.y;
    col_rect.br.y = p_skelevent_redraw->work_skel_rect.br.pixit_point.y;

    status_assert(skel_col_enum(p_docu, p_docu->cur.slr.row, p_skelevent_redraw->clip_skel_rect.tl.page_num.x, (COL) -1 /* i.e. all columns for given row on given page */, &col_array));

    if(0 != array_elements(&col_array))
    {
        const ARRAY_INDEX col_count = array_elements(&col_array);
        ARRAY_INDEX col_index;
        P_CB_DATA_BORDER_HORZ_ADJUST_COLUMN_WIDTH p_horz_blk = NULL;
        BOOL adjusting_1 = FALSE;
        BOOL adjusting_2 = FALSE;
        HOST_FONT host_font_redraw = HOST_FONT_NONE;

        if(status_ok(status = fonty_handle_from_font_spec(&p_border_style->font_spec, FALSE)))
        {
            const FONTY_HANDLE fonty_handle = (FONTY_HANDLE) status;

            host_font_redraw = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, FALSE);
        }
        else
        {
#if WINDOWS
            host_font_redraw = GetStockFont(ANSI_VAR_FONT);
#endif
            status = STATUS_OK;
        }

        if(show_dragging)
        {
            P_ANY reasondata;

            if(host_drag_in_progress(p_docu, &reasondata))
            {
                p_horz_blk = (P_CB_DATA_BORDER_HORZ_ADJUST_COLUMN_WIDTH) reasondata;

                switch(p_horz_blk->reason_code)
                {
                default:
                    break;

                case CB_CODE_BORDER_HORZ_ADJUST_COLUMN_SPLIT:
                    adjusting_2 = TRUE;
                /*>>>should we check (p_horz_blk->row == p_docu->cur.slr.row)???*/
                    break;

                case CB_CODE_BORDER_HORZ_ADJUST_COLUMN_WIDTH:
                    adjusting_1 = TRUE;
                /*>>>should we check (p_horz_blk->row == p_docu->cur.slr.row)???*/
                    break;
                }
            }
        }

        assert(border_flags.top.show);
        assert(!border_flags.top.dashed);
        assert(border_flags.bottom.show);
        assert(!border_flags.bottom.dashed);
        assert(border_flags.left.show);
        assert(!border_flags.left.dashed);
        assert(border_flags.right.show);
        /*border_flags.right.dashed assigned in loop */

        /* at least one column per page */
        col_index = 0;

        assert(col_count);

        do  {
            const PC_COL_INFO p_col_info = array_ptrc(&col_array, COL_INFO, col_index);
            FRAMED_BOX_STYLE border_style = FRAMED_BOX_BUTTON_OUT;
            /*S32 line_wimpcolour = COLOUR_OF_BORDER_FRAME;*/
            const PC_RGB p_rgb_line = colour_of_border_frame(p_border_style);
            S32 fill_wimpcolour = COLOUR_OF_BORDER;
            RGB rgb_fill = *colour_of_border(p_border_style);
            /*const S32 text_wimpcolour = COLOUR_OF_TEXT;*/
            const PC_RGB p_rgb_text = &p_border_style->font_spec.colour;

            col_rect.tl.x = p_col_info->edge_left.pixit;
            col_rect.br.x = p_col_info->edge_right.pixit;
#if FALSE
            col_rect.tl.y = p_skelevent_redraw->work_skel_rect.tl.pixit_point.y;
            col_rect.br.y = p_skelevent_redraw->work_skel_rect.br.pixit_point.y;
#endif

            if(adjusting_1 && (p_col_info->edge_left.page == p_horz_blk->pag))
            {
                if(p_col_info->col >  p_horz_blk->col)
                    col_rect.tl.x += p_horz_blk->deltax;

                if(p_col_info->col >= p_horz_blk->col)
                    col_rect.br.x += p_horz_blk->deltax;
            }

            if(adjusting_2 && (p_col_info->edge_left.page == p_horz_blk->pag))
            {
                if(p_col_info->col == (p_horz_blk->col + 1))
                    col_rect.tl.x += p_horz_blk->deltax;

                if(p_col_info->col == p_horz_blk->col)
                    col_rect.br.x += p_horz_blk->deltax;
            }

            border_flags.right.dashed = 0;

            if(button && (p_col_info->col == p_docu->cur.slr.col))
            {
                border_style = FRAMED_BOX_BUTTON_IN;
                fill_wimpcolour = COLOUR_OF_BORDER_CURRENT;
                rgb_fill = colour_of_border_adjust_to_current(&rgb_fill, p_rgb_text);
            }

            {
            STYLE style;
            STYLE_SELECTOR selector;
            SLR slr;
            PC_USTR ustr_numform_res = ustr_empty_string;
            QUICK_UBLOCK_WITH_BUFFER(numform_res, 16);
            quick_ublock_with_buffer_setup(numform_res);

            style_init(&style);
            style_selector_clear(&selector);
            style_selector_bit_set(&selector, STYLE_SW_CS_COL_NAME);

            slr.row = p_docu->cur.slr.row;
            slr.col = p_col_info->col;
            style_from_slr(p_docu, &style, &selector, &slr);

            if(array_elements(&style.col_style.h_numform))
            {
                PC_USTR ustr_numform = array_ustr(&style.col_style.h_numform);

                /* SKS 22dec94 make it so col headings don't need quotes */
                if(NULL != ustrchr(ustr_numform, CH_NUMBER_SIGN))
                {
                    NUMFORM_PARMS numform_parms;
                    SS_DATA ss_data;

                    ss_data_set_integer(&ss_data, (S32) p_col_info->col + 1);

                    zero_struct(numform_parms);
                    numform_parms.ustr_numform_numeric = ustr_numform;
                    numform_parms.p_numform_context = get_p_numform_context(p_docu);

                    status_assert(status = numform(&numform_res, P_QUICK_TBLOCK_NONE, &ss_data, &numform_parms));

                    ustr_numform_res = quick_ublock_ustr(&numform_res);
                }
                else
                    ustr_numform_res = ustr_numform;
            }

            style_dispose(&style);

            if(status_ok(status))
            {
#if defined(UNUSED)
                if(button)
                    host_fonty_text_paint_uchars_in_framed_box(&p_skelevent_redraw->redraw_context, &col_rect, ustr_numform_res, ustrlen32(ustr_numform_res),
                                                               border_style, fill_wimpcolour, text_wimpcolour, host_font_redraw);
                else
#endif /* UNUSED */
                    host_fonty_text_paint_uchars_in_rectangle( p_redraw_context, &col_rect, ustr_numform_res, ustrlen32(ustr_numform_res),
                                                               &border_flags, &rgb_fill, p_rgb_line, p_rgb_text, host_font_redraw);
            }

            quick_ublock_dispose(&numform_res);
            } /*block*/

            border_flags.left.dashed = border_flags.right.dashed;
        }
        while(++col_index < col_count);

        /* host font handles belong to fonty session (upper redraw layers) */
    }

    /* draw empty space to the right of the columns */
    if(!p_skelevent_redraw->flags.show_content)
    {
        col_rect.tl.x = col_rect.br.x;
        col_rect.br.x = p_skelevent_redraw->clip_skel_rect.br.pixit_point.x;
        if(col_rect.br.x > col_rect.tl.x)
        {
            const PC_STYLE p_ruler_style = p_style_for_ruler_horz();
            const PC_RGB p_colour_of_ruler = colour_of_ruler(p_ruler_style);
            host_paint_rectangle_filled(&p_skelevent_redraw->redraw_context, &col_rect, p_colour_of_ruler);
        }
    }

    return(STATUS_OK);
}

/*
Event handler for a row border rendered within a page c.f. edge_window_event_border_vert()
*/

#define p_style_for_margin_row p_style_for_border_vert /* for now */

T5_MSG_PROTO(static, margin_row_event_redraw, P_SKELEVENT_REDRAW p_skelevent_redraw)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    return(border_vert_redraw(p_docu, FALSE, p_skelevent_redraw, p_style_for_margin_row())); /* drag operations in window are NOT shown */
}

PROC_EVENT_PROTO(extern, proc_event_margin_row)
{
    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(margin_row_event_redraw(p_docu, t5_message, (P_SKELEVENT_REDRAW) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* Events for the border_vert come here
*
******************************************************************************/

static BORDER_FLAGS
init_border_vert_border_flags = { { TRUE, FALSE }, { TRUE, FALSE }, { TRUE, FALSE }, { TRUE, FALSE } };

/* goto row row_number, current column */

_Check_return_
static STATUS
border_vert_event_click_left_single_row_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_number)
{
    SKELCMD_GOTO skelcmd_goto;

    skelcmd_goto.slr.col = p_docu->cur.slr.col;
    skelcmd_goto.slr.row = row_number;
    skelcmd_goto.keep_selection = FALSE;

    skel_point_from_slr_tl(p_docu, &skelcmd_goto.skel_point, &skelcmd_goto.slr);

    return(object_skel(p_docu, T5_MSG_GOTO_SLR, &skelcmd_goto));
}

/* select row row_number */

_Check_return_
static STATUS
border_vert_event_click_left_double_row_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_number)
{
    p_docu->anchor_mark.docu_area.tl.slr.col = 0;
    p_docu->anchor_mark.docu_area.tl.slr.row = row_number;
    p_docu->anchor_mark.docu_area.tl.object_position.object_id = OBJECT_ID_NONE;

    p_docu->anchor_mark.docu_area.br.slr.col = n_cols_logical(p_docu) - 1;
    p_docu->anchor_mark.docu_area.br.slr.row = row_number;
    p_docu->anchor_mark.docu_area.br.object_position.object_id = OBJECT_ID_NONE;

    p_docu->anchor_mark.docu_area.whole_col = FALSE;
    p_docu->anchor_mark.docu_area.whole_row = TRUE;

    status_assert(anchor_to_markers_new(p_docu));

    anchor_to_markers_finish(p_docu);

    markers_show(p_docu);

    return(STATUS_OK);
}

/* select row row_number and start drag */

_Check_return_
static STATUS
border_vert_event_click_left_drag_row_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_number)
{
    p_docu->anchor_mark.docu_area.tl.slr.col = 0;
    p_docu->anchor_mark.docu_area.tl.slr.row = row_number;
    p_docu->anchor_mark.docu_area.tl.object_position.object_id = OBJECT_ID_NONE;

    p_docu->anchor_mark.docu_area.br.slr.col = n_cols_logical(p_docu) - 1;
    p_docu->anchor_mark.docu_area.br.slr.row = row_number;
    p_docu->anchor_mark.docu_area.br.object_position.object_id = OBJECT_ID_NONE;

    p_docu->anchor_mark.docu_area.whole_col = FALSE;
    p_docu->anchor_mark.docu_area.whole_row = TRUE;

    status_assert(anchor_to_markers_new(p_docu));

    anchor_to_markers_update(p_docu);

    markers_show(p_docu);

    /* start drag process to capture rows (anchors at row_number) */
    adjust_blk_border_vert_row_height.reason_code = CB_CODE_BORDER_VERT_EXTEND_SELECTION;

    host_drag_start(&adjust_blk_border_vert_row_height);

    return(STATUS_OK);
}

/* right click in border is treated as a right click in page last_page_x */

_Check_return_
static STATUS
border_vert_event_click_right_single_row_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message, /* single or drag */
    _InVal_     ROW row_number)
{
    /* set up anchor start position - use current position if no selection */
    if(!p_docu->mark_info_cells.h_markers)
        p_docu->anchor_mark.docu_area.tl = p_docu->cur;

    p_docu->anchor_mark.docu_area.br.slr.col = n_cols_logical(p_docu) - 1;
    p_docu->anchor_mark.docu_area.br.slr.row = row_number;
    p_docu->anchor_mark.docu_area.br.object_position.object_id = OBJECT_ID_NONE;

    p_docu->anchor_mark.docu_area.whole_col = FALSE;
    p_docu->anchor_mark.docu_area.whole_row = TRUE;

    /* put anchor block in list */
    if(!p_docu->mark_info_cells.h_markers)
        status_assert(anchor_to_markers_new(p_docu));

    if(T5_EVENT_CLICK_RIGHT_DRAG == t5_message)
    {
        anchor_to_markers_update(p_docu);

        markers_show(p_docu);

        adjust_blk_border_vert_row_height.reason_code = CB_CODE_BORDER_VERT_EXTEND_SELECTION;

        host_drag_start(&adjust_blk_border_vert_row_height);
    }
    else /* T5_EVENT_CLICK_RIGHT_SINGLE */
    {
        anchor_to_markers_finish(p_docu);

        markers_show(p_docu);
    }

    /* >>>> soon: view_hide_caret(p_docu); */
    return(STATUS_OK);
}

_Check_return_
static STATUS
border_vert_event_click_right_double_row_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     ROW row_number)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_skelevent_click);
    UNREFERENCED_PARAMETER_InVal_(row_number);

    return(STATUS_OK);
}

_Check_return_
static STATUS
border_vert_event_click_right_drag_row_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     ROW row_number)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_skelevent_click);
    UNREFERENCED_PARAMETER_InVal_(row_number);

    return(STATUS_OK);
}

_Check_return_
static STATUS
border_vert_event_click_single_row_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     ROW row_number)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_SINGLE;
    const T5_MESSAGE t5_message_effective = right_message_if_shift(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(border_vert_event_click_right_single_row_centre(p_docu, t5_message_effective, row_number));

    return(border_vert_event_click_left_single_row_centre(p_docu, row_number));
}

_Check_return_
static STATUS
border_vert_event_click_double_row_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     ROW row_number)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DOUBLE;
    const T5_MESSAGE t5_message_effective = right_message_if_shift(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(border_vert_event_click_right_double_row_centre(p_docu, t5_message_effective, p_skelevent_click, row_number));

    return(border_vert_event_click_left_double_row_centre(p_docu, row_number));
}

_Check_return_
static STATUS
border_vert_event_click_drag_row_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     ROW row_number)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DRAG;
    const T5_MESSAGE t5_message_effective = right_message_if_shift(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(border_vert_event_click_right_drag_row_centre(p_docu, t5_message_effective, p_skelevent_click, row_number));

    return(border_vert_event_click_left_drag_row_centre(p_docu, row_number));
}

static STATUS
border_vert_event_click_row_centre(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     ROW row_number)
{
    switch(t5_message)
    {
    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
        return(border_vert_event_click_single_row_centre(p_docu, t5_message, p_skelevent_click, row_number));

    case T5_EVENT_CLICK_LEFT_DOUBLE:
    case T5_EVENT_CLICK_RIGHT_DOUBLE:
        return(border_vert_event_click_double_row_centre(p_docu, t5_message, p_skelevent_click, row_number));

    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        return(border_vert_event_click_drag_row_centre(p_docu, t5_message, p_skelevent_click, row_number));

    default: default_unhandled();
#if CHECKING
    case T5_EVENT_CLICK_LEFT_TRIPLE:
    case T5_EVENT_CLICK_RIGHT_TRIPLE:
#endif
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
border_vert_event_click_left_double_height_adjustor(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_number)
{
    if(p_docu->cur.slr.row != row_number)
    {   /* 'goto' the new row, cos auto height applies to the current row */
        SKELCMD_GOTO skelcmd_goto;

        skelcmd_goto.slr.col = p_docu->cur.slr.col;
        skelcmd_goto.slr.row = row_number;
        skelcmd_goto.keep_selection = TRUE;

        skel_point_from_slr_tl(p_docu, &skelcmd_goto.skel_point, &skelcmd_goto.slr);

        status_return(object_skel(p_docu, T5_MSG_GOTO_SLR, &skelcmd_goto));
        /*>>>this may need changing if 'goto' tries to prevent entry to zero height rows */
    }

    return(execute_command(p_docu, T5_CMD_AUTO_HEIGHT, _P_DATA_NONE(P_ARGLIST_HANDLE), OBJECT_ID_SKEL));
}

/* Increase/decrease row height */

_Check_return_
static STATUS
border_vert_event_click_left_drag_height_adjustor(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     ROW row_number)
{
    if(p_docu->cur.slr.row != row_number)
    {   /* 'goto' the new row, 'cos resizes apply to current row */
        SKELCMD_GOTO skelcmd_goto;

        skelcmd_goto.slr.col = p_docu->cur.slr.col;
        skelcmd_goto.slr.row = row_number;
        skelcmd_goto.keep_selection = FALSE;

        skel_point_from_slr_tl(p_docu, &skelcmd_goto.skel_point, &skelcmd_goto.slr);

        status_return(object_skel(p_docu, T5_MSG_GOTO_SLR, &skelcmd_goto));
        /*>>>this may need changing if 'goto' tries to prevent entry to zero height rows */
    }

    if(status_ok(border_vert_drag_limits(p_docu, p_skelevent_click->skel_point.page_num.y, p_docu->cur.slr.col, row_number, &adjust_blk_border_vert_row_height)))
    {
        adjust_blk_border_vert_row_height.reason_code = CB_CODE_BORDER_VERT_ADJUST_ROW_HEIGHT;

        host_drag_start(&adjust_blk_border_vert_row_height);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
border_vert_event_click_right_double_height_adjustor(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_number)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(row_number);

    return(STATUS_OK);
}

_Check_return_
static STATUS
border_vert_event_click_right_drag_height_adjustor(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     ROW row_number)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InRef_(p_skelevent_click);
    UNREFERENCED_PARAMETER_InVal_(row_number);

    return(STATUS_OK);
}

_Check_return_
static STATUS
border_vert_event_click_double_height_adjustor(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     ROW row_number)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DOUBLE;
    const T5_MESSAGE t5_message_effective = right_message_if_ctrl(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(border_vert_event_click_right_double_height_adjustor(p_docu, row_number));

    return(border_vert_event_click_left_double_height_adjustor(p_docu, row_number));
}

_Check_return_
static STATUS
border_vert_event_click_drag_height_adjustor(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     ROW row_number)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DRAG;
    const T5_MESSAGE t5_message_effective = right_message_if_ctrl(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(border_vert_event_click_right_drag_height_adjustor(p_docu, p_skelevent_click, row_number));

    return(border_vert_event_click_left_drag_height_adjustor(p_docu, p_skelevent_click, row_number));
}

_Check_return_
static STATUS
border_vert_event_click_row_height_adjustor(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     ROW row_number)
{
    switch(t5_message)
    {
    case T5_EVENT_CLICK_LEFT_DOUBLE:
    case T5_EVENT_CLICK_RIGHT_DOUBLE:
        return(border_vert_event_click_double_height_adjustor(p_docu, t5_message, p_skelevent_click, row_number));

    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        return(border_vert_event_click_drag_height_adjustor(p_docu, t5_message, p_skelevent_click, row_number));

    default: default_unhandled();
#if CHECKING
    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
    case T5_EVENT_CLICK_LEFT_TRIPLE:
    case T5_EVENT_CLICK_RIGHT_TRIPLE:
#endif
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
border_vert_event_click(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click)
{
    BORDER_VERT_POSITION position;
    ROW row_number;

    p_skelevent_click->processed = 1;

    border_vert_where(p_docu, p_skelevent_click, &position /*filled*/, &row_number /*filled*/, TRUE, TRUE);

    switch(position)
    {
    case OVER_ROW_CENTRE:
        return(border_vert_event_click_row_centre(p_docu, t5_message, p_skelevent_click, row_number));

    case OVER_ROW_HEIGHT_ADJUSTOR:
        return(border_vert_event_click_row_height_adjustor(p_docu, t5_message, p_skelevent_click, row_number));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
border_vert_event_drag_movement_adjust_row_height(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click)
{
    const P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT p_vert_blk = (P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT) p_skelevent_click->data.drag.p_reason_data;
    PIXIT delta_y;
    BOOL show = 0;

    if(T5_EVENT_CLICK_DRAG_MOVEMENT == t5_message)
    {
        delta_y = click_stop_limited(p_skelevent_click->data.drag.pixit_delta.y, p_vert_blk->delta_min, p_vert_blk->delta_max, p_vert_blk->delta_min, &p_vert_blk->click_stop_step);

        if(p_vert_blk->deltay != delta_y)
        {
            SKEL_RECT skel_rect;
            RECT_FLAGS rect_flags;
            REDRAW_FLAGS redraw_flags;

            p_vert_blk->deltay = delta_y;

            show = 1;

            /* redraw BORDER_VERT for this page */
            skel_rect.tl.pixit_point.x =
            skel_rect.tl.pixit_point.y = 0;
            skel_rect.tl.page_num.x    = 0;
            skel_rect.tl.page_num.y    = p_vert_blk->pag;
            skel_rect.br               = skel_rect.tl;

            RECT_FLAGS_CLEAR(rect_flags);
            rect_flags.extend_right_window = rect_flags.extend_down_page = 1;

            REDRAW_FLAGS_CLEAR(redraw_flags);
            redraw_flags.show_selection = TRUE;

            view_update_now(p_docu, UPDATE_BORDER_VERT, &skel_rect, rect_flags, redraw_flags, LAYER_CELLS);
        }
    }
    else
    {
        delta_y = 0;

        show = 1;
    }

    if(show)
    {   /* show row height on status line */
        border_show_status(p_docu, &p_vert_blk->drag_status, p_vert_blk->param + delta_y, p_vert_blk->row);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
border_vert_event_drag_movement_extend_selection(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click)
{
    /*const P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT p_vert_blk = (P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT) p_skelevent_click->data.drag.p_reason_data;*/
    BORDER_VERT_POSITION position;
    ROW row_number;

    border_vert_where(p_docu, p_skelevent_click, &position /*filled*/, &row_number /*filled*/, FALSE, FALSE);

    if(OVER_ROW_DEAD_SPACE != position)
    {
        p_docu->anchor_mark.docu_area.br.slr.row = row_number;
        anchor_to_markers_update(p_docu);
        markers_show(p_docu);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
border_vert_event_drag_movement(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click)
{
    const P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT p_vert_blk = (P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT) p_skelevent_click->data.drag.p_reason_data;

    trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_border_vert T5_EVENT_CLICK_DRAG_MOVEMENT"));

    switch(p_vert_blk->reason_code)
    {
    default: default_unhandled();
#if CHECKING
    case CB_CODE_BORDER_VERT_ADJUST_ROW_HEIGHT:
#endif
        return(border_vert_event_drag_movement_adjust_row_height(p_docu, t5_message, p_skelevent_click));

    case CB_CODE_BORDER_VERT_EXTEND_SELECTION:
        return(border_vert_event_drag_movement_extend_selection(p_docu, p_skelevent_click));
    }
}

T5_MSG_PROTO(static, border_vert_event_redraw, P_SKELEVENT_REDRAW p_skelevent_redraw)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* Fix the width of the vertical ruler to the pixit width */
    p_skelevent_redraw->work_skel_rect.br.pixit_point.x = view_border_pixit_size(p_skelevent_redraw->redraw_context.p_view, FALSE);

    return(border_vert_redraw(p_docu, TRUE, p_skelevent_redraw, p_style_for_border_vert())); /* drag operations in this window (border_vert) are shown */
}

PROC_EVENT_PROTO(extern, edge_window_event_border_vert)
{
    STATUS status = STATUS_OK;

    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(border_vert_event_redraw(p_docu, t5_message, (P_SKELEVENT_REDRAW) p_data));

    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_LEFT_DOUBLE:
    case T5_EVENT_CLICK_LEFT_TRIPLE:
    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_DOUBLE:
    case T5_EVENT_CLICK_RIGHT_TRIPLE:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        return(border_vert_event_click(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_STARTED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
        return(border_vert_event_drag_movement(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_ABORTED:
    case T5_EVENT_CLICK_DRAG_FINISHED:
        {
        P_SKELEVENT_CLICK p_skelevent_click = (P_SKELEVENT_CLICK) p_data;
        const P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT p_vert_blk = (P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT) p_skelevent_click->data.drag.p_reason_data;
        BOOL redraw_border;

        trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_border_vert T5_EVENT_CLICK_DRAG_FINISHED"));

        if(T5_EVENT_CLICK_DRAG_FINISHED == t5_message)
        {
            redraw_border = 0;

            switch(p_vert_blk->reason_code)
            {
            default: default_unhandled();
#if CHECKING
            case CB_CODE_BORDER_VERT_ADJUST_ROW_HEIGHT:
#endif
                {
                PIXIT drag_delta_y = click_stop_limited(p_skelevent_click->data.drag.pixit_delta.y, p_vert_blk->delta_min, p_vert_blk->delta_max, p_vert_blk->delta_min, &p_vert_blk->click_stop_step);

                if(status_fail(border_vert_row_apply_delta(p_docu, p_vert_blk->row, drag_delta_y)))
                    redraw_border = 1;

                break;
                }

            case CB_CODE_BORDER_VERT_EXTEND_SELECTION:
                {
                BORDER_VERT_POSITION position;
                ROW row_number;

                border_vert_where(p_docu, p_skelevent_click, &position /*filled*/, &row_number /*filled*/, TRUE, TRUE);

                if(position != OVER_ROW_DEAD_SPACE)
                {
                    p_docu->anchor_mark.docu_area.br.slr.row = row_number;
                    anchor_to_markers_finish(p_docu);
                    markers_show(p_docu);
                }

                break;
                }
            }
        }
        else
            redraw_border = 1;

        status_line_clear(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING);

        if(redraw_border)
        {
            SKEL_RECT skel_rect;
            RECT_FLAGS rect_flags;

            skel_rect.tl.pixit_point.x = 0;
            skel_rect.tl.pixit_point.y = 0;
            skel_rect.tl.page_num.x = 0;
            skel_rect.tl.page_num.y = p_vert_blk->pag;
            skel_rect.br = skel_rect.tl;

            RECT_FLAGS_CLEAR(rect_flags);
            rect_flags.extend_right_window = rect_flags.extend_down_page = 1;

            view_update_later(p_docu, UPDATE_BORDER_VERT, &skel_rect, rect_flags);
        }

        break;
        }

    case T5_EVENT_POINTER_ENTERS_WINDOW:
        {
        P_SKELEVENT_CLICK p_skelevent_click = (P_SKELEVENT_CLICK) p_data;
        trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_border_vert T5_EVENT_POINTER_ENTERS_WINDOW"));
        border_vert_where(p_docu, p_skelevent_click, NULL, NULL, TRUE, TRUE);
        break;
        }

    case T5_EVENT_POINTER_MOVEMENT:
        {
        P_SKELEVENT_CLICK p_skelevent_click = (P_SKELEVENT_CLICK) p_data;
        trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_border_vert T5_EVENT_POINTER_MOVEMENT"));
        border_vert_where(p_docu, p_skelevent_click, NULL, NULL, TRUE, TRUE);
        break;
        }

    case T5_EVENT_POINTER_LEAVES_WINDOW:
        trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_border_vert T5_EVENT_POINTER_LEAVES_WINDOW"));
        status_line_clear(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING);
        break;

    default:
        break;
    }

    return(status);
}

/******************************************************************************
*
* Redraw vertical border
*
* Called by edge_window_event_border_vert()
*           proc_event_margin_row()
*
******************************************************************************/

#define ROW_HEAD_DEFAULT USTR_TEXT("#")

_Check_return_
static STATUS
border_vert_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL show_dragging /*,style info font colour etc*/,
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw,
    _InRef_     PC_STYLE p_border_style)
{
    STATUS status = STATUS_OK;
    const P_REDRAW_CONTEXT p_redraw_context = &p_skelevent_redraw->redraw_context;
    ROW_ENTRY row_entry_start, row_entry_end;
    ROW row_index;
    ARRAY_INDEX adjust_row;
    PIXIT adjust_delta;
    PIXIT_RECT row_rect;
    BORDER_FLAGS border_flags = init_border_vert_border_flags; /*>>>should this be a parameter???*/
    S32 button = show_dragging;
    HOST_FONT host_font_redraw = HOST_FONT_NONE;

    assert((p_skelevent_redraw->clip_skel_rect.tl.page_num.x == p_skelevent_redraw->clip_skel_rect.br.page_num.x) &&
           (p_skelevent_redraw->clip_skel_rect.tl.page_num.y == p_skelevent_redraw->clip_skel_rect.br.page_num.y));

    /* SKS - note that we can get rows that aren't on this page: catered for below */
    row_entry_from_skel_point(p_docu, &row_entry_start, &p_skelevent_redraw->clip_skel_rect.tl, FALSE);
    row_entry_from_skel_point(p_docu, &row_entry_end, &p_skelevent_redraw->clip_skel_rect.br, FALSE);

    /* default values that cause no row repositioning */
    adjust_row = row_entry_end.row + 1;
    adjust_delta = 0;

    if(status_ok(status = fonty_handle_from_font_spec(&p_border_style->font_spec, FALSE)))
    {
        const FONTY_HANDLE fonty_handle = (FONTY_HANDLE) status;

        host_font_redraw = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, FALSE);
    }
    else
    {
#if WINDOWS
        host_font_redraw = GetStockFont(ANSI_VAR_FONT);
#endif
        status = STATUS_OK;
    }

    if(show_dragging)
    {
        P_ANY reasondata;

        if(host_drag_in_progress(p_docu, &reasondata))
        {
            /*>>>should also check page number??? */
            P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT p_blk = (P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT) reasondata;

            switch(p_blk->reason_code)
            {
            default:
                break;

            case CB_CODE_BORDER_VERT_ADJUST_ROW_HEIGHT:
                adjust_row   = p_blk->row;
                adjust_delta = p_blk->deltay;
                break;
            }
        }
    }

    row_rect.tl.x = p_skelevent_redraw->work_skel_rect.tl.pixit_point.x;
    row_rect.br.x = p_skelevent_redraw->work_skel_rect.br.pixit_point.x;

    row_rect.br.y = p_skelevent_redraw->clip_skel_rect.tl.pixit_point.y; /* in case we need to clear to edge of page */

    for(row_index = row_entry_start.row; row_index <= row_entry_end.row; ++row_index)
    {
        ROW_ENTRY row_entry;
        FRAMED_BOX_STYLE border_style = FRAMED_BOX_BUTTON_OUT;
        /*S32 line_wimpcolour = COLOUR_OF_BORDER_FRAME;*/
        const PC_RGB p_rgb_line = colour_of_border_frame(p_border_style);
        S32 fill_wimpcolour = COLOUR_OF_BORDER;
        RGB rgb_fill = *colour_of_border(p_border_style);
        /*const S32 text_wimpcolour = COLOUR_OF_TEXT;*/
        const PC_RGB p_rgb_text = &p_border_style->font_spec.colour;

        row_entry_from_row(p_docu, &row_entry, row_index);

        /* ensure some part of this row is visible on this page */
        border_flags.top.show = (row_entry.rowtab.edge_top.page == p_skelevent_redraw->clip_skel_rect.tl.page_num.y);
        if(row_entry.rowtab.edge_top.page > p_skelevent_redraw->clip_skel_rect.tl.page_num.y)
            continue;

        border_flags.bottom.show = (row_entry.rowtab.edge_bot.page == p_skelevent_redraw->clip_skel_rect.br.page_num.y);
        if(row_entry.rowtab.edge_bot.page < p_skelevent_redraw->clip_skel_rect.br.page_num.y)
            continue;

        row_rect.tl.y = (border_flags.top.show ? row_entry.rowtab.edge_top.pixit : 0);
        row_rect.br.y = (border_flags.bottom.show ? row_entry.rowtab.edge_bot.pixit : p_skelevent_redraw->work_skel_rect.br.pixit_point.y);

        /*>>>code is probably in wrong place */
        if(row_index >= adjust_row)
        {
            row_rect.br.y += adjust_delta;
        }
        if(row_index > adjust_row)
        {
            row_rect.tl.y += adjust_delta;
        }

        if(button && (row_index == p_docu->cur.slr.row))
        {
            border_style = FRAMED_BOX_BUTTON_IN;
            fill_wimpcolour = COLOUR_OF_BORDER_CURRENT;
            rgb_fill = colour_of_border_adjust_to_current(&rgb_fill, p_rgb_text);
        }

        {
        STYLE style;
        STYLE_SELECTOR selector;
        SLR slr;
        SS_DATA ss_data;
        NUMFORM_PARMS numform_parms;
        PC_USTR ustr_numform;
        QUICK_UBLOCK_WITH_BUFFER(numform_res_quick_ublock, 16);
        quick_ublock_with_buffer_setup(numform_res_quick_ublock);

        style_init(&style);
        style_selector_clear(&selector);
        style_selector_bit_set(&selector, STYLE_SW_RS_ROW_NAME);

        slr.row = row_index;
        slr.col = p_docu->cur.slr.col;
        style_from_slr(p_docu, &style, &selector, &slr);

        ss_data_set_integer(&ss_data, (S32) row_index + 1);

        zero_struct(numform_parms);

        numform_parms.ustr_numform_numeric =
            style.row_style.h_numform
                ? array_ustr(&style.row_style.h_numform)
                : ROW_HEAD_DEFAULT;

        style_dispose(&style);

        numform_parms.p_numform_context = get_p_numform_context(p_docu);

        status_assert(status = numform(&numform_res_quick_ublock, P_QUICK_TBLOCK_NONE, &ss_data, &numform_parms));

        ustr_numform = quick_ublock_ustr(&numform_res_quick_ublock);

        if(status_ok(status))
        {
#if defined(UNUSED)
            if(button)
                host_fonty_text_paint_uchars_in_framed_box(&p_skelevent_redraw->redraw_context, &row_rect, ustr_numform, ustrlen32(ustr_numform),
                                                           border_style, fill_wimpcolour, text_wimpcolour, host_font_redraw);
            else
#endif /* UNUSED */
                host_fonty_text_paint_uchars_in_rectangle( &p_skelevent_redraw->redraw_context, &row_rect, ustr_numform, ustrlen32(ustr_numform),
                                                           &border_flags, &rgb_fill, p_rgb_line, p_rgb_text, host_font_redraw);
        }

        quick_ublock_dispose(&numform_res_quick_ublock);
        } /*block*/
    }

    /* host font handles belong to fonty session (upper redraw layers) */

    /* draw empty space below the rows */
    if(!p_skelevent_redraw->flags.show_content)
    {
        row_rect.tl.y = row_rect.br.y;
        row_rect.br.y = p_skelevent_redraw->clip_skel_rect.br.pixit_point.y;

        if(row_rect.br.y > row_rect.tl.y)
        {
            const PC_STYLE p_ruler_style = p_style_for_ruler_vert();
            const PC_RGB p_colour_of_ruler = colour_of_ruler(p_ruler_style);
            host_paint_rectangle_filled(&p_skelevent_redraw->redraw_context, &row_rect, p_colour_of_ruler);
        }
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
border_vert_row_apply_delta(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row,
    _InVal_     PIXIT drag_delta_y)
{
    ROW_ENTRY row_entry;
    row_entry_from_row(p_docu, &row_entry, row);

    if(row_entry.rowtab.edge_top.page == row_entry.rowtab.edge_bot.page)
    {
        const PIXIT old_height = row_entry.rowtab.edge_bot.pixit - row_entry.rowtab.edge_top.pixit;
        PIXIT new_height = old_height + drag_delta_y;

        new_height = MAX(new_height, 0); /* enforce minimum row height of zero */

        if(new_height != old_height)
        {
            STYLE style;
            style_init(&style);

            style_bit_set(&style, STYLE_SW_RS_HEIGHT);
            style_bit_set(&style, STYLE_SW_RS_HEIGHT_FIXED);
            style.row_style.height = new_height;
            style.row_style.height_fixed = 1;

            if(p_docu->flags.virtual_row_table)
                return(style_apply_struct_to_source(p_docu, &style)); /* modify BaseSheet or whatever */

            { /* apply to marked cells or to whole row at current cell */
            DOCU_AREA docu_area;
            STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;

            STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);

            if(p_docu->mark_info_cells.h_markers)
                docu_area_from_markers_first(p_docu, &docu_area);
            else
            {
                docu_area_from_position(&docu_area, &p_docu->cur);
                docu_area.whole_row = 1;
                docu_area_normalise(p_docu, &docu_area, &docu_area);
            }

            return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
            } /*block*/
        }

        return(STATUS_OK);
    }

    return(STATUS_FAIL); /* row went AWOL or crosses page boundary (so should never have got this far) */
}

static void
border_horz_where(
    _DocuRef_   P_DOCU p_docu,
    PC_SKELEVENT_CLICK p_skelevent_click,
    P_BORDER_HORZ_POSITION p_position,
    P_COL p_col_number,
    _InVal_     S32 set_pointer_shape,
    _InVal_     S32 set_status_line)
{
    PC_SKEL_POINT p_skel_point = &p_skelevent_click->skel_point;
    BORDER_HORZ_POSITION obj = OVER_COLUMN_DEAD_SPACE;
    COL col = -1;
    POINTER_SHAPE pointer_shape = POINTER_DEFAULT;
    ARRAY_HANDLE col_array = 0;
    PIXIT width = 0;

    switch(p_docu->focus_owner)
    {
    default:
        break;

    case OBJECT_ID_CELLS:
    case OBJECT_ID_REC_FLOW:
        {
        SKEL_POINT skel_point_minus_delta = *p_skel_point;
        SKEL_POINT skel_point_plus_delta  = *p_skel_point;
        PIXIT hitband = p_skelevent_click->click_context.one_program_pixel.x * 4; /* why oh why did it take till 12apr95!!! to do this */

        skel_point_minus_delta.pixit_point.x -= hitband;
        skel_point_plus_delta.pixit_point.x  += hitband;

        status_break(skel_col_enum(p_docu, p_docu->cur.slr.row, p_skel_point->page_num.x, (COL) -1 /* i.e. all columns for given row on given page */, &col_array));

        if((col = col_at_skel_point(p_docu, col_array, p_skel_point)) >= 0)
        {
            /* Somewhere over column col */
            if(col != col_at_skel_point(p_docu, col_array, &skel_point_plus_delta))
            {
                trace_1(TRACE_APP_CLICK, TEXT("At right of column ") COL_TFMT TEXT(", ready to drag its width"), col);

                obj = OVER_COLUMN_WIDTH_ADJUSTOR;
                pointer_shape = POINTER_DRAG_COLUMN;
            }
            else if(col != col_at_skel_point(p_docu, col_array, &skel_point_minus_delta))
            {
                COL start_col = array_ptr(&col_array, COL_INFO, 0)->col;

                trace_1(TRACE_APP_CLICK, TEXT("At left of column ") COL_TFMT TEXT(", "), col);

                if(col > start_col)
                {
                    trace_1(TRACE_APP_CLICK, TEXT("ready to drag the width of column ") COL_TFMT, col - 1);

                    obj = OVER_COLUMN_WIDTH_ADJUSTOR;
                    pointer_shape = POINTER_DRAG_COLUMN;
                    col--;
                }
                else
                {
                    trace_0(TRACE_APP_CLICK, TEXT("which is at the left of the page - treated as if over middle of column"));

                    obj = OVER_COLUMN_CENTRE;
                    pointer_shape = POINTER_DEFAULT;        /*>>>might be nice to use another shape */
                }
            }
            else
            {
                trace_1(TRACE_APP_CLICK, TEXT("Over middle of column ") COL_TFMT TEXT(", "), col);

                obj = OVER_COLUMN_CENTRE;
                pointer_shape = POINTER_DEFAULT;        /*>>>might be nice to use another shape */
            }
        }
        else
        {
            const ARRAY_INDEX n_elements = array_elements(&col_array);
            PC_COL_INFO p_col_info;

            if(n_elements > 0)
            {
                S32 n_cols_vis = 0;

                /* this grokky piece of code to do with base_single_col was added
                 * by MRJC on 23.10.93 to stop dragging out of the special zero width
                 * base region in letter type single col documents
                 * it wants removing as soon as possible
                 */
                if(p_docu->flags.base_single_col)
                {
                    /* count non-zero width columns */
                    COL col_i;
                    for(col_i = 0, p_col_info = array_range(&col_array, COL_INFO, 0, n_elements); col_i < n_elements; ++col_i, ++p_col_info)
                        if(p_col_info->edge_right.pixit - p_col_info->edge_left.pixit)
                            n_cols_vis += 1;
                }

                if(!p_docu->flags.base_single_col || n_cols_vis > 1)
                {
                    p_col_info = array_ptrc(&col_array, COL_INFO, n_elements - 1);

                    if(/* is column rhs past the left edge of the hitband ? */
                       (edge_skel_point_compare(&p_col_info->edge_right, &skel_point_minus_delta, x) > 0) &&
                       /* and the point (centre line of hitband) is to the right of lhs */
                       (edge_skel_point_compare(&p_col_info->edge_left,  p_skel_point, x) <= 0)
                      )
                    {
                        trace_1(TRACE_APP_CLICK,
                                TEXT("right of column headings, ready to drag the width of column ") COL_TFMT,
                                p_col_info->col);

                        obj = OVER_COLUMN_WIDTH_ADJUSTOR;
                        pointer_shape = POINTER_DRAG_COLUMN;
                        col           = p_col_info->col;
                    }
                    else
                        trace_0(TRACE_APP_CLICK, TEXT("off left/right of column headings - ignored"));
                }
            }
            else
                trace_0(TRACE_APP_CLICK, TEXT("no columns on this page - ignored"));
        }

        break;
        }
    } /* esac */

    /* if somewhere over a column, we'll need its width to display in the status line */
    if(obj == OVER_COLUMN_CENTRE)
    {
        const COL start_col = array_basec(&col_array, COL_INFO)->col;
        const PC_COL_INFO p_col_info = array_ptrc(&col_array, COL_INFO, col - start_col);

        width = p_col_info->edge_right.pixit - p_col_info->edge_left.pixit;
    }
    else if(obj == OVER_COLUMN_WIDTH_ADJUSTOR)
    {
        const COL start_col = array_basec(&col_array, COL_INFO)->col;
        const PC_COL_INFO p_col_info = array_ptrc(&col_array, COL_INFO, col - start_col);
        S32 dirn = 3;

        /* if column rh edge touches rh of printable area, we won't be allowed to drag to right */
        if(p_col_info->edge_right.pixit >= p_docu->page_def.cells_usable_x)
            dirn = dirn & ~1;

        /* if column width is zero, we won't be allowed to drag to left */
        if(p_col_info->edge_right.pixit == p_col_info->edge_left.pixit)
            dirn = dirn & ~2;

        switch(dirn)
        {
        case 0: /* zero width column jammed against rh edge of printable area */
            obj = OVER_COLUMN_DEAD_SPACE;
            col = -1;
            pointer_shape = POINTER_DEFAULT;
            break;

        case 2:
            pointer_shape = POINTER_DRAG_COLUMN_LEFT;
            break;

        case 1:
            pointer_shape = POINTER_DRAG_COLUMN_RIGHT;
            break;

        default:
            break;
        }

        width = p_col_info->edge_right.pixit - p_col_info->edge_left.pixit;
    }

    if(NULL != p_position)
        *p_position = obj;

    if(NULL != p_col_number)
        *p_col_number = col;

    if(set_pointer_shape)
        host_set_pointer_shape(pointer_shape);

    if(set_status_line)
    {
        if((obj == OVER_COLUMN_CENTRE) || (obj == OVER_COLUMN_WIDTH_ADJUSTOR))
        {
            SCALE_INFO scale_info;
            DISPLAY_UNIT_INFO display_unit_info;
            BORDER_DRAG_STATUS status_line;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
            quick_ublock_with_buffer_setup(quick_ublock);

            scale_info_from_docu(p_docu, TRUE, &scale_info);

            display_unit_info_from_display_unit(&display_unit_info, scale_info.display_unit);

            status_line.fp_pixits_per_unit = display_unit_info.fp_pixits_per_unit;
            status_line.unit_message       = display_unit_info.unit_message;

            status_assert(style_text_for_marker(p_docu, &quick_ublock, STYLE_SW_CS_WIDTH));
            status_line.ustr_style_prefix = quick_ublock_ustr(&quick_ublock);

            status_line.drag_message = hborder_status[obj].drag_message;
            status_line.post_message = hborder_status[obj].post_message;

            border_show_status(p_docu, &status_line, width, col);

            quick_ublock_dispose(&quick_ublock);
        }
        else
            status_line_clear(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING);
    }
}

static void
border_vert_where(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click,
    P_BORDER_VERT_POSITION p_position,
    P_ROW p_row_number,
    _InVal_     S32 set_pointer_shape,
    _InVal_     S32 set_status_line)
{
    P_SKEL_POINT p_skel_point = &p_skelevent_click->skel_point;
    BORDER_VERT_POSITION obj = OVER_ROW_DEAD_SPACE;
    ROW row = -1;
    POINTER_SHAPE pointer_shape = POINTER_DEFAULT;
    BOOL unsplit = FALSE;
    PIXIT height = 0;

    switch(p_docu->focus_owner)
    {
    default:
        break;

    case OBJECT_ID_CELLS:
    case OBJECT_ID_REC_FLOW:
        {
        PIXIT hitband = p_skelevent_click->click_context.one_program_pixel.y * 2; /* why oh why did it take till 12apr95!!! to do this */
        SKEL_POINT skel_point_minus_delta = *p_skel_point;
        SKEL_POINT skel_point_plus_delta  = *p_skel_point;
        ROW_ENTRY row_entry_first, row_entry_point;

        skel_point_minus_delta.pixit_point.y -= hitband;
        skel_point_plus_delta.pixit_point.y  += hitband;

        row_entry_from_row(p_docu, &row_entry_first, 0);

        if(status_ok(row_entry_at_skel_point(p_docu, &row_entry_point, p_skel_point)))
        {
            obj = OVER_ROW_CENTRE;              /*>>>is this line needed??? */
            row = row_entry_point.row - row_entry_first.row;

            /* somewhere over row <row> */

            unsplit = (row_entry_point.rowtab.edge_bot.page == row_entry_point.rowtab.edge_top.page);
            height  = (unsplit ? row_entry_point.rowtab.edge_bot.pixit - row_entry_point.rowtab.edge_top.pixit : 0);

            if(unsplit &&
               (skel_point_edge_compare(&skel_point_plus_delta, &row_entry_point.rowtab.edge_bot, y) >= 0)     /*>>> >=0 ???*/
              )
            {
                trace_1(TRACE_APP_CLICK, TEXT("At bottom of row ") ROW_TFMT TEXT(", ready to drag its height"), row);

                obj = OVER_ROW_HEIGHT_ADJUSTOR;
                pointer_shape = POINTER_DRAG_ROW;
            }
            else
            {
                ROW_ENTRY row_entry_prev;
                row_entry_from_row(p_docu, &row_entry_prev, row > 0 ? row - 1 : 0);

                if(row > 0 &&
                   row_entry_prev.rowtab.edge_bot.page == row_entry_prev.rowtab.edge_top.page &&
                   edge_skel_point_compare(&row_entry_prev.rowtab.edge_bot, &skel_point_minus_delta, y) > 0)
                {
                    trace_1(TRACE_APP_CLICK, TEXT("At top of row ") ROW_TFMT TEXT(", "), row);

                    unsplit = TRUE;
                    height  = row_entry_prev.rowtab.edge_bot.pixit - row_entry_prev.rowtab.edge_top.pixit;

                    {
                    trace_1(TRACE_APP_CLICK, TEXT("ready to drag the height of row ") ROW_TFMT, row - 1);

                    obj = OVER_ROW_HEIGHT_ADJUSTOR;
                    pointer_shape = POINTER_DRAG_ROW;
                    row--;
                    } /*block*/
                }
                else
                {
                    trace_1(TRACE_APP_CLICK, TEXT("Over middle of row ") ROW_TFMT TEXT(", "), row);

                    obj = OVER_ROW_CENTRE;
                    pointer_shape = POINTER_DEFAULT;        /*>>>might be nice to use another shape */
                }
            }
        }
        else
        { /*EMPTY*/ } /*>>>need to check if within hitband of last row on page (or is it in document?)*/

        break;
        }
    }

    /* if over a row edge, the drag direction may need restricting or preventing */
    if(obj == OVER_ROW_HEIGHT_ADJUSTOR)
    {
        ROW_ENTRY row_entry;
        S32 dirn = 3;

        row_entry_from_row(p_docu, &row_entry, row);

        /* if row bottom edge touches bottom of printable area, we won't be allowed to drag to down */
        if(row_entry.rowtab.edge_bot.pixit >= p_docu->page_def.cells_usable_y)
            dirn = dirn & ~1;

        /* if row height is zero, we won't be allowed to drag to up */
        if(row_entry.rowtab.edge_bot.pixit == row_entry.rowtab.edge_top.pixit)
            dirn = dirn & ~2;

        switch(dirn)
        {
        case 0: /* zero height row jammed against bottom edge of printable area */
            obj = OVER_ROW_DEAD_SPACE;
            row = -1;
            pointer_shape = POINTER_DEFAULT;
            break;

        case 2:
            pointer_shape = POINTER_DRAG_ROW_UP;
            break;

        case 1:
            pointer_shape = POINTER_DRAG_ROW_DOWN;
            break;

        default:
            break;
        }
    }

    if(NULL != p_position)
        *p_position = obj;

    if(NULL != p_row_number)
        *p_row_number = row;

    if(set_pointer_shape)
        host_set_pointer_shape(pointer_shape);

    if(set_status_line)
    {
        if((obj == OVER_ROW_CENTRE) || (obj == OVER_ROW_HEIGHT_ADJUSTOR))
        {
            BORDER_DRAG_STATUS status_line;
            SCALE_INFO scale_info;
            DISPLAY_UNIT_INFO display_unit_info;

            scale_info_from_docu(p_docu, FALSE, &scale_info);

            display_unit_info_from_display_unit(&display_unit_info, scale_info.display_unit);

            status_line.fp_pixits_per_unit = display_unit_info.fp_pixits_per_unit;
            status_line.unit_message       = display_unit_info.unit_message;

            status_line.ustr_style_prefix  = NULL;

            status_line.drag_message = vborder_status[obj].drag_message;
            status_line.post_message = vborder_status[obj].post_message;

            border_show_status(p_docu, &status_line, height, row);
        }
        else
            status_line_clear(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING);
    }
}

_Check_return_
static STATUS
apply_new_col_width(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col,
    _InVal_     ROW row,
    _InVal_     PIXIT drag_delta_x)
{
    STATUS status;
    ARRAY_HANDLE col_array;

    status_assert(status = skel_col_enum(p_docu, row, (PAGE) -1, col, &col_array));

    if(status_ok(status))
    {
        const PC_COL_INFO p_col_info = array_basec(&col_array, COL_INFO);
        const PIXIT old_width = p_col_info->edge_right.pixit - p_col_info->edge_left.pixit;
        PIXIT new_width = old_width + drag_delta_x;

        new_width = MAX(new_width, 0); /* enforce minimum column width of zero */
        new_width = MIN(new_width, p_docu->page_def.cells_usable_x);

        if(new_width != old_width)
        {
            STYLE style;
            style_init(&style);

            /* set new column width: minimum column width is zero, maximum is page_def.cells_usable_x */
            style_bit_set(&style, STYLE_SW_CS_WIDTH);
            style.col_style.width = new_width;

            status = style_apply_struct_to_source(p_docu, &style);
        }
    }

    return(status);
}

_Check_return_ _Success_(status_ok(return))
static STATUS
border_horz_drag_limits(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page,
    _InVal_     COL col,
    _InVal_     ROW row,
    _OutRef_    P_CB_DATA_BORDER_HORZ_ADJUST_COLUMN_WIDTH p_horz_blk,
    _InVal_     S32 ratio)
{
    ARRAY_HANDLE col_array;
    PC_COL_INFO p_col_info;
    BORDER_HORZ_POSITION obj = OVER_COLUMN_WIDTH_ADJUSTOR;
    PIXIT delta_min = 0;
    PIXIT delta_max = 0;
    PIXIT width_left  = 0;
    PIXIT width_right = 0;

    status_return(skel_col_enum(p_docu, row, (PAGE) -1, col, &col_array));

    if(0 == array_elements(&col_array))
        return(STATUS_FAIL); /* can't find column, so don't start dragging */

    assert(1 == array_elements(&col_array));

    p_col_info = array_ptr(&col_array, COL_INFO, 0);

    width_left = p_col_info->edge_right.pixit - p_col_info->edge_left.pixit;

    delta_min = p_col_info->edge_left.pixit - p_col_info->edge_right.pixit;
    delta_max = p_docu->page_def.cells_usable_x - p_col_info->edge_right.pixit;

    if(ratio)
    {
        COL next_col = col + 1;
        ARRAY_HANDLE next_col_array;
        PC_COL_INFO next_p_col_info;

        status_return(skel_col_enum(p_docu, row, (PAGE) -1, next_col, &next_col_array));

        if(0 == array_elements(&next_col_array))
            return(STATUS_FAIL); /* can't find next column, so don't start dragging */

        assert(1 == array_elements(&next_col_array));

        next_p_col_info = array_basec(&next_col_array, COL_INFO);

        width_right = next_p_col_info->edge_right.pixit - next_p_col_info->edge_left.pixit;

        delta_max = MIN(delta_max, width_right);
    }

    p_horz_blk->param = p_col_info->edge_right.pixit - p_col_info->edge_left.pixit;

    p_horz_blk->pag = page;
    p_horz_blk->col = col;
    p_horz_blk->row = row;

    p_horz_blk->deltax = 0;
    p_horz_blk->delta_min = delta_min;
    p_horz_blk->delta_max = delta_max;

    p_horz_blk->width_left  = width_left;
    p_horz_blk->width_right = width_right;

    p_horz_blk->drag_status.ustr_style_prefix = NULL;

    p_horz_blk->drag_status.drag_message = hborder_status[obj].drag_message;
    p_horz_blk->drag_status.post_message = 0; /* omit waffle during drag */

    {
    SCALE_INFO scale_info;
    DISPLAY_UNIT_INFO display_unit_info;

    scale_info_from_docu(p_docu, TRUE, &scale_info);

    display_unit_info_from_display_unit(&display_unit_info, scale_info.display_unit);

    p_horz_blk->drag_status.fp_pixits_per_unit = display_unit_info.fp_pixits_per_unit;
    p_horz_blk->drag_status.unit_message       = display_unit_info.unit_message;

    click_stop_initialise(&p_horz_blk->click_stop_step, &scale_info, TRUE);
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
border_vert_drag_limits(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page,
    _InVal_     COL col,
    _InVal_     ROW row,
    /*S32 ratio,*/
    _OutRef_    P_CB_DATA_BORDER_VERT_ADJUST_ROW_HEIGHT p_vert_blk)
{
    BORDER_VERT_POSITION obj = OVER_ROW_HEIGHT_ADJUSTOR;
    PIXIT delta_min, delta_max;
    ROW_ENTRY row_entry;

    row_entry_from_row(p_docu, &row_entry, row);

    /* we can reduce row height to zero if whole of row is on page, or until bottom of row is at top of page */
    /* we can increase row height until bottom of row is at bottom of page */
    delta_min = ((row_entry.rowtab.edge_top.page == row_entry.rowtab.edge_bot.page)
                 ? row_entry.rowtab.edge_top.pixit - row_entry.rowtab.edge_bot.pixit
                 : 0 - row_entry.rowtab.edge_bot.pixit
                );
    delta_max = p_docu->page_def.cells_usable_y - row_entry.rowtab.edge_bot.pixit;

    /*>>>wrong, cos rows cross pages*/
    p_vert_blk->param = row_entry.rowtab.edge_bot.pixit - row_entry.rowtab.edge_top.pixit;

    p_vert_blk->pag = page;
    p_vert_blk->col = col;
    p_vert_blk->row = row;

    p_vert_blk->deltay    = 0;
    p_vert_blk->delta_min = delta_min;
    p_vert_blk->delta_max = delta_max;

    p_vert_blk->drag_status.ustr_style_prefix = NULL;

    p_vert_blk->drag_status.drag_message = vborder_status[obj].drag_message;
    p_vert_blk->drag_status.post_message = 0; /* omit waffle during drag */

    {
    SCALE_INFO scale_info;
    DISPLAY_UNIT_INFO display_unit_info;

    scale_info_from_docu(p_docu, FALSE, &scale_info);

    display_unit_info_from_display_unit(&display_unit_info, scale_info.display_unit);

    p_vert_blk->drag_status.fp_pixits_per_unit = display_unit_info.fp_pixits_per_unit;
    p_vert_blk->drag_status.unit_message       = display_unit_info.unit_message;

    click_stop_initialise(&p_vert_blk->click_stop_step, &scale_info, TRUE);
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
extern STATUS
skel_get_visible_row_ranges(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_row_range_list)
{
    P_ROW_RANGE p_row_range;
    ARRAY_INDEX viewix;
    STATUS status = STATUS_OK;

    DOCU_ASSERT(p_docu);
    PTR_ASSERT(p_h_row_range_list);
    *p_h_row_range_list = 0;

    for(viewix = 0; viewix < array_elements(&p_docu->h_view_table); ++viewix)
    {
        const PC_VIEW p_view = array_ptrc(&p_docu->h_view_table, VIEW, viewix);
        PANE_ID pane_id = NUMPANE;

        if(view_void_entry(p_view))
            continue;

        do  {
            PANE_ID_DECR(pane_id);

            if(HOST_WND_NONE != p_view->pane[pane_id].hwnd)
            {
                /* add row_range to list */
                SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_row_range), 0);

                if(NULL == (p_row_range = al_array_extend_by(p_h_row_range_list, ROW_RANGE, 1, &array_init_block, &status)))
                    break;

                al_array_auto_compact_set(p_h_row_range_list); /* probably unnecessary */

                *p_row_range = p_view->pane[pane_id].visible_row_range;
            }
        } while(PANE_ID_START != pane_id);

        status_break(status);
    }

    if(status_fail(status))
        al_array_dispose(p_h_row_range_list);

    return(status);
}

extern void
skel_visible_row_range(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _OutRef_    P_ROW_RANGE p_row_range)
{
    ROW_ENTRY row_entry_top, row_entry_bot;

    UNREFERENCED_PARAMETER_ViewRef_(p_view);

    row_entry_from_skel_point(p_docu, &row_entry_top, &p_skel_rect->tl, FALSE);
    row_entry_from_skel_point(p_docu, &row_entry_bot, &p_skel_rect->br, FALSE);

    p_row_range->top = row_entry_top.row;
    p_row_range->bot = row_entry_bot.row + 1;
}

static void
border_show_status(
    _DocuRef_   P_DOCU p_docu,
    P_BORDER_DRAG_STATUS p_drag_status,
    _InVal_     PIXIT value,
    _InVal_     S32 col_or_row)
{
    STATUS status = STATUS_OK;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 96);
    quick_ublock_with_buffer_setup(quick_ublock);

    for(;;) /* loop for structure */
    {
        if(p_drag_status->ustr_style_prefix)
        {
            status_break(status = quick_ublock_ustr_add(&quick_ublock, p_drag_status->ustr_style_prefix));
            status_break(status = quick_ublock_ustr_add(&quick_ublock, USTR_TEXT(". ")));
        }

        {
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock_inner, 96);
        quick_ublock_with_buffer_setup(quick_ublock_inner);
        if( status_ok(status = resource_lookup_quick_ublock(&quick_ublock_inner, p_drag_status->drag_message))
        &&  status_ok(status = quick_ublock_nullch_add(&quick_ublock_inner)) )
            status = quick_ublock_printf(&quick_ublock, quick_ublock_ustr(&quick_ublock_inner), col_or_row + 1);
        quick_ublock_dispose(&quick_ublock_inner);
        status_break(status);
        } /*block*/

        status_break(status = quick_ublock_ustr_add(&quick_ublock, USTR_TEXT(": ")));

        {
        SS_DATA ss_data;
        STATUS numform_resource_id;
        UCHARZ numform_unit_ustr_buf[sizeof32("0.,####")];
        NUMFORM_PARMS numform_parms;

        ss_data_set_real(&ss_data, /*FP_USER_UNIT*/ ((FP_PIXIT) value / p_drag_status->fp_pixits_per_unit));

        switch(p_drag_status->unit_message)
        {
        default: default_unhandled();
#if CHECKING
        case MSG_DIALOG_UNITS_CM:
#endif
                                      numform_resource_id = MSG_NUMFORM_2_DP; break;
        case MSG_DIALOG_UNITS_MM:     numform_resource_id = MSG_NUMFORM_1_DP; break;
        case MSG_DIALOG_UNITS_INCHES: numform_resource_id = MSG_NUMFORM_3_DP; break;
        case MSG_DIALOG_UNITS_POINTS: numform_resource_id = MSG_NUMFORM_1_DP; break;
        }

        resource_lookup_ustr_buffer(ustr_bptr(numform_unit_ustr_buf), elemof32(numform_unit_ustr_buf), numform_resource_id);

        zero_struct(numform_parms);
        numform_parms.ustr_numform_numeric = ustr_bptr(numform_unit_ustr_buf);
        numform_parms.p_numform_context = get_p_numform_context(p_docu);

        status_break(status = numform(&quick_ublock, P_QUICK_TBLOCK_NONE, &ss_data, &numform_parms));
        } /*block*/

        PtrPutByteOff(quick_ublock_uchars_wr(&quick_ublock), quick_ublock_bytes(&quick_ublock)-1, CH_SPACE); /* replace CH_NULL */
        status_break(status = resource_lookup_quick_ublock(&quick_ublock, p_drag_status->unit_message));

        if(p_drag_status->post_message)
        {
            status_break(status = quick_ublock_ustr_add(&quick_ublock, USTR_TEXT(". ")));
            status_break(status = resource_lookup_quick_ublock(&quick_ublock, p_drag_status->post_message));
        }

        status_break(status = quick_ublock_nullch_add(&quick_ublock));

        {
        UI_TEXT ui_text;
        ui_text.type = UI_TEXT_TYPE_USTR_TEMP;
        ui_text.text.ustr = quick_ublock_ustr(&quick_ublock);
        status_line_set(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING, &ui_text);
        } /*block*/

        break; /* end of loop for structure */
        /*NOTREACHED*/
    }

    status_assertc(status);

    quick_ublock_dispose(&quick_ublock);
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_bord);

T5_MSG_PROTO(static, maeve_services_sk_bord_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT1:
        return(maeve_event_handler_add(p_docu, maeve_event_sk_bord, (CLIENT_HANDLE) 0));

    case T5_MSG_IC__CLOSE1:
        maeve_event_handler_del(p_docu, maeve_event_sk_bord, (CLIENT_HANDLE) 0);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_bord)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_bord_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* generate text for border/ruler dragging
*
******************************************************************************/

_Check_return_
extern STATUS
style_text_for_marker(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _InVal_     STYLE_BIT_NUMBER style_bit_number)
{
    STATUS status = STATUS_OK;
    BOOL region = TRUE;
    MARK_INFO mark_info;

    if(status_fail(object_skel(p_docu, T5_MSG_MARK_INFO_READ, &mark_info)))
        mark_info.h_markers = 0;

    if(!mark_info.h_markers)
    {
        STYLE_SELECTOR selector;
        P_STYLE_DOCU_AREA p_style_docu_area;

        style_selector_clear(&selector);
        style_selector_bit_set(&selector, style_bit_number);

        p_style_docu_area = style_effect_source_find(p_docu, &selector, &p_docu->cur, &p_docu->h_style_docu_area, FALSE);

        if(NULL != p_style_docu_area)
            if(p_style_docu_area->style_handle)
            {
                status = style_text_convert(p_docu,
                                            p_quick_ublock,
                                            p_style_from_docu_area(p_docu, p_style_docu_area),
                                            &style_selector_all);
                region = FALSE;
            }
    }

    if(region)
        if(status_ok(status = resource_lookup_quick_ublock(p_quick_ublock, MSG_REGION)))
            status = quick_ublock_nullch_add(p_quick_ublock);

    return(status);
}

/* end of sk_bord.c */
