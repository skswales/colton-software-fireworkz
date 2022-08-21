/* ob_ruler.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Ruler object module for Fireworkz */

/* SKS June 1993 separated out from skel */

#include "common/gflags.h"

#include "ob_ruler/ob_ruler.h"

#ifndef         __vi_edge_h
#include "ob_skel/vi_edge.h"
#endif

#if RISCOS
#define MSG_WEAK &rb_ruler_msg_weak
extern PC_U8 rb_ruler_msg_weak;
#endif
#define P_BOUND_RESOURCES_OBJECT_ID_RULER NULL

/*
internal structure
*/

typedef struct RULER_HORZ_AREA_OFFSETS
{
    PIXIT cells_area_left;
    PIXIT cells_area_right;
}
RULER_HORZ_AREA_OFFSETS, * P_RULER_HORZ_AREA_OFFSETS;

typedef struct RULER_VERT_AREA_OFFSETS
{
    PIXIT margin_header_top;
    PIXIT margin_header_text;
    PIXIT margin_header_bottom;
    PIXIT margin_col_top;
    PIXIT margin_col_bottom;
    PIXIT cells_area_top;
    PIXIT cells_area_bottom;
    PIXIT margin_footer_top;
    PIXIT margin_footer_text;
    PIXIT margin_footer_bottom;
}
RULER_VERT_AREA_OFFSETS, * P_RULER_VERT_AREA_OFFSETS;

typedef struct RULER_DRAG_STATUS
{
    PC_USTR ustr_style_prefix;

    FP_PIXIT fp_pixits_per_unit;    /* PIXITS_PER_CM     , PIXITS_PER_MM     , PIXITS_PER_INCH      */
    STATUS unit_message;            /* MSG_DRAG_STATUS_CM, MSG_DRAG_STATUS_MM, MSG_DRAG_STATUS_INCH */

    STATUS drag_message;            /* eg MSG_DRAG_STATUS_RULER_HORZ_TAB which is "Left tab" */
    STATUS post_message;
}
RULER_DRAG_STATUS, * P_RULER_DRAG_STATUS;

/*
*
* click_stop_origin the absolute origin relative to which things are measured
* init_value        the initial value of the thing being dragged
* value             the current value of the thing being dragged
* min_limit         the minimum value allowed for the thing being dragged
* max_limit         the maximum ...
* click_stop_step   value clicks at these stops between the two limits
* negative          dragging right(down) makes the reported value (relative to click_stop_origin) decrease, not increase
*
*/

typedef struct CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION
{
    S32 reason_code;

    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);

    COL col;
    S32 pag;

    PIXIT    click_stop_origin;
    PIXIT    value, init_value, min_limit, max_limit;
    FP_PIXIT click_stop_step;
    BOOL     negative;

    RULER_DRAG_STATUS drag_status;

    PIXIT width_left, width_right;

    SKEL_RECT  update_skel_rect; /* add delta before use */
    RECT_FLAGS update_rect_flags;

    RULER_MARKER ruler_marker;
    ARRAY_INDEX  tab_number; /* valid iff ruler_marker is TAB_xxx */

    struct CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION_TRACKING /* whilst dragging the left margin, the para margin must be redrawn as well */
    {                                                         /* ditto for the column width indicator and right margin */
        RULER_MARKER ruler_marker;
        SKEL_RECT    update_skel_rect;
        RECT_FLAGS   update_rect_flags;
    }
    tracking;

    BOOL dragging_single;
}
CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION, * P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION;

typedef struct CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION
{
    S32 reason_code;

    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);

    ROW row;
    S32 pag;

    PIXIT    click_stop_origin;
    PIXIT    init_value, value, min_limit, max_limit;
    FP_PIXIT click_stop_step;
    BOOL     negative;

    RULER_DRAG_STATUS drag_status;

    SKEL_RECT  update_skel_rect; /* add delta before use */
    RECT_FLAGS update_rect_flags;

    RULER_MARKER ruler_marker;
}
CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION, * P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION;

typedef struct RULER_STATUS
{
    POINTER_SHAPE pointer_shape;
    STATUS drag_message;
    STATUS post_message;
    STYLE_BIT_NUMBER style_bit_number;
}
RULER_STATUS, * P_RULER_STATUS;

/*
internal routines
*/

_Check_return_
static STATUS
ruler_horz_drag_ended(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION p_horz_blk);

_Check_return_
static STATUS
ruler_horz_drag_limits(
    _DocuRef_   P_DOCU p_docu,
    _In_        PAGE_NUM page_num,
    _InVal_     COL col,
    _InVal_     RULER_MARKER ruler_marker,
    _InVal_     ARRAY_INDEX tab_number,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION p_horz_blk);

static void
ruler_horz_area_offsets_for_page(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y,
    _OutRef_    P_RULER_HORZ_AREA_OFFSETS p_horzruler_area_offsets);

T5_MSG_PROTO(static, ruler_horz_event_redraw, _InoutRef_ P_SKELEVENT_REDRAW p_skelevent_redraw);

_Check_return_
static STATUS
ruler_horz_ruler_info_ensure(
    _DocuRef_   P_DOCU p_docu);

#if 0

static void
ruler_horz_update_margins_and_tab_stops(
    _DocuRef_   P_DOCU p_docu,
    P_RULER_INFO p_ruler_info_old,
    P_RULER_INFO p_ruler_info_new);

static void
ruler_horz_update_whole_ruler(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y);

#endif

static void
ruler_horz_update_whole_tabbar(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y);

static void
ruler_horz_where(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_CLICK_CONTEXT p_click_context,
    P_SKEL_POINT p_skel_point,
    P_RULER_MARKER p_ruler_marker,
    P_ARRAY_INDEX p_tab_number,
    _InVal_     BOOL set_pointer_shape,
    _InVal_     BOOL set_status_line);

#if MARKERS_INSIDE_RULER_HORZ
#define RULER_HORZ_MARKER_Y     (PIXITS_PER_PROGRAM_PIXEL_Y *  0)
#else
#define RULER_HORZ_MARKER_Y     (RULER_HORZ_SCALE_BOT_Y + 1 * PIXITS_PER_PROGRAM_PIXEL_Y) /* skip incl, and leave a tiny gap */
#endif

static PIXIT ruler_horz_marker_y = RULER_HORZ_MARKER_Y;

static void
ruler_scale_and_figures(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_PIXIT_LINE p_base_line,
    _InVal_     PIXIT zero_xy,
    _InVal_     BOOL horizontal_ruler,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_STYLE p_ruler_style);

static void
ruler_show_status(
    _DocuRef_   P_DOCU p_docu,
    P_RULER_DRAG_STATUS p_drag_status,
    _InVal_     PIXIT value);

_Check_return_
static STATUS
ruler_vert_drag_limits(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     RULER_MARKER ruler_marker,
    _InVal_     PAGE page,
    _InVal_     ROW row,
    _InoutRef_  P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION p_vert_blk);

_Check_return_
static STATUS
ruler_vert_drag_apply(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     RULER_MARKER ruler_marker,
    _InVal_     PAGE page_y,
    _InVal_     PIXIT drag_delta_y);

static void
ruler_vert_area_offsets_for_page(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y,
    _OutRef_    P_RULER_VERT_AREA_OFFSETS p_vertruler_area_offsets);

T5_MSG_PROTO(static, ruler_vert_event_redraw, _InoutRef_ P_SKELEVENT_REDRAW p_skelevent_redraw);

_Check_return_
static STATUS
ruler_vert_row_apply_delta(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row,
    _InVal_     PIXIT drag_delta_y);

static void
ruler_vert_where(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_CLICK_CONTEXT p_click_context,
    P_SKEL_POINT p_skel_point,
    P_RULER_MARKER p_ruler_marker,
    _InVal_     BOOL set_pointer_shape,
    _InVal_     BOOL set_status_line);

#if MARKERS_INSIDE_RULER_VERT
#define RULER_VERT_MARKER_X         (2 * PIXITS_PER_PROGRAM_PIXEL_X)
#else
#define RULER_VERT_MARKER_X         (RULER_VERT_SCALE_RIGHT_X + 2 * PIXITS_PER_PROGRAM_PIXEL_X) /* skip incl, and leave a tiny gap */
#endif

static PIXIT ruler_vert_marker_x = RULER_VERT_MARKER_X;

static RULER_STATUS
ruler_status[RULER_MARKER_COUNT] =
{
    /* ruler_horz markers */
/*RULER_MARKER_MARGIN_LEFT*/  { POINTER_DRAG_MARGIN, RULER_MSG_STATUS_RULER_HORZ_MARGIN_LEFT,   RULER_MSG_STATUS_RULER_HORZ_DRAG_MARGIN, STYLE_SW_PS_MARGIN_LEFT },
/*RULER_MARKER_MARGIN_PARA */ { POINTER_DRAG_MARGIN, RULER_MSG_STATUS_RULER_HORZ_MARGIN_PARA,   RULER_MSG_STATUS_RULER_HORZ_DRAG_MARGIN, STYLE_SW_PS_MARGIN_PARA },
/*RULER_MARKER_MARGIN_RIGHT*/ { POINTER_DRAG_MARGIN, RULER_MSG_STATUS_RULER_HORZ_MARGIN_RIGHT,  RULER_MSG_STATUS_RULER_HORZ_DRAG_MARGIN, STYLE_SW_PS_MARGIN_RIGHT },
/*RULER_MARKER_COL_RIGHT*/    { POINTER_DRAG_COLUMN, RULER_MSG_STATUS_RULER_HORZ_COL_WIDTH,     MSG_STATUS_DRAG_COL,                     STYLE_SW_CS_WIDTH },
/*RULER_MARKER_TAB_LEFT*/     { POINTER_DRAG_TAB,    RULER_MSG_STATUS_RULER_HORZ_TAB_LEFT,      RULER_MSG_STATUS_RULER_HORZ_DRAG_TAB,    STYLE_SW_PS_TAB_LIST },
/*RULER_MARKER_TAB_CENTRE*/   { POINTER_DRAG_TAB,    RULER_MSG_STATUS_RULER_HORZ_TAB_CENTRE,    RULER_MSG_STATUS_RULER_HORZ_DRAG_TAB,    STYLE_SW_PS_TAB_LIST },
/*RULER_MARKER_TAB_RIGHT*/    { POINTER_DRAG_TAB,    RULER_MSG_STATUS_RULER_HORZ_TAB_RIGHT,     RULER_MSG_STATUS_RULER_HORZ_DRAG_TAB,    STYLE_SW_PS_TAB_LIST },
/*RULER_MARKER_TAB_DECIMAL*/  { POINTER_DRAG_TAB,    RULER_MSG_STATUS_RULER_HORZ_TAB_DECIMAL,   RULER_MSG_STATUS_RULER_HORZ_DRAG_TAB,    STYLE_SW_PS_TAB_LIST },

    /* ruler_vert markers */
/*RULER_MARKER_HEADER_EXT*/   { POINTER_DRAG_HEFO,   RULER_MSG_STATUS_RULER_VERT_HEADER_MARGIN, RULER_MSG_STATUS_RULER_HORZ_DRAG_MARGIN, (STYLE_BIT_NUMBER) 0 },
/*RULER_MARKER_HEADER_OFF*/   { POINTER_DRAG_HEFO,   RULER_MSG_STATUS_RULER_VERT_HEADER_OFFSET, RULER_MSG_STATUS_RULER_HORZ_DRAG_MARGIN, (STYLE_BIT_NUMBER) 0 },
/*RULER_MARKER_FOOTER_EXT*/   { POINTER_DRAG_HEFO,   RULER_MSG_STATUS_RULER_VERT_FOOTER_MARGIN, RULER_MSG_STATUS_RULER_HORZ_DRAG_MARGIN, (STYLE_BIT_NUMBER) 0 },
/*RULER_MARKER_FOOTER_OFF*/   { POINTER_DRAG_HEFO,   RULER_MSG_STATUS_RULER_VERT_FOOTER_OFFSET, RULER_MSG_STATUS_RULER_HORZ_DRAG_MARGIN, (STYLE_BIT_NUMBER) 0 },
/*RULER_MARKER_ROW_BOTTOM*/   { POINTER_DRAG_ROW,    RULER_MSG_STATUS_RULER_VERT_ROW_HEIGHT,    MSG_STATUS_DRAG_ROW,                     STYLE_SW_RS_HEIGHT }
};

/*
only one on these can be active at once; identified by host_drag_in_progress()
*/

static
union CB_DATA_RULER_UNION
{
    CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION ruler_horz;
    CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION ruler_vert;
}
adjust_marker_position_blk;

static RULER_MARKER
ruler_marker_from_tab_type[4] =
{
    RULER_MARKER_TAB_LEFT,
    RULER_MARKER_TAB_CENTRE,
    RULER_MARKER_TAB_RIGHT,
    RULER_MARKER_TAB_DECIMAL,
};

/******************************************************************************
*
* Events for the ruler_horz come here
*
******************************************************************************/

_Check_return_
static STATUS
ruler_horz_event_click_left_single_place_tab(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click)
{
    STATUS status = STATUS_OK;

    if(p_docu->ruler_horz_info.ruler_info.valid)
    {
        RULER_INFO ruler_info = p_docu->ruler_horz_info.ruler_info;
        P_COL_INFO p_col_info = &ruler_info.col_info;
        RULER_HORZ_AREA_OFFSETS ruler_horz_area_offsets;

        ruler_horz_area_offsets_for_page(p_docu, p_skelevent_click->skel_point.page_num.y, &ruler_horz_area_offsets);

        { /* MUST copy tab list, inserting new tab in correct place  */
        ARRAY_INDEX tab_count = array_elements(&p_col_info->h_tab_list);
        STYLE style;

        style_init(&style);
        style_bit_set(&style, STYLE_SW_PS_TAB_LIST);

        if(NULL != al_array_alloc(&style.para_style.h_tab_list, TAB_INFO, tab_count + 1, &p_col_info->tab_init_block, &status))
        {
            SCALE_INFO scale_info;
            FP_PIXIT click_stop_step;
            ARRAY_INDEX tab_index;
            PC_TAB_INFO p_src_tab_info;
            P_TAB_INFO p_dst_tab_info;
            SKEL_RECT update_skel_rect;
            RECT_FLAGS update_rect_flags;
            PIXIT insert_tab_position;

            scale_info_from_docu(p_docu, TRUE, &scale_info);

            click_stop_initialise(&click_stop_step, &scale_info, TRUE);

            /* place user clicked, left margin relative */
            insert_tab_position = click_stop_limited(p_skelevent_click->skel_point.pixit_point.x -
                                                     (ruler_horz_area_offsets.cells_area_left + p_col_info->edge_left.pixit + p_col_info->margin_left),
                                                     MIN(p_col_info->margin_left, p_col_info->margin_para),
                                                     p_docu->page_def.cells_usable_x - p_col_info->edge_left.pixit,
                                                     p_col_info->edge_left.pixit, &click_stop_step);

            p_src_tab_info = array_rangec(&p_col_info->h_tab_list, TAB_INFO, 0, tab_count);
            p_dst_tab_info = array_range(&style.para_style.h_tab_list, TAB_INFO, 0, tab_count);

            for(tab_index = 0; tab_index < tab_count; ++tab_index)
            {
                if(p_src_tab_info->offset > insert_tab_position)
                    break;

                *p_dst_tab_info++ = *p_src_tab_info++;
            }

            /* tab type was selected previously by the user, place it where he clicked */
            p_dst_tab_info->type   = p_docu->insert_tab_type;
            p_dst_tab_info->offset = insert_tab_position;
            p_dst_tab_info++;

            memcpy32(p_dst_tab_info, p_src_tab_info, sizeof32(*p_src_tab_info) * (tab_count - tab_index));

            update_skel_rect.tl.page_num.x = p_col_info->edge_left.page;
            update_skel_rect.tl.page_num.y = cur_page_y(p_docu);
            update_skel_rect.tl.pixit_point.x = p_col_info->edge_left.pixit + ruler_horz_area_offsets.cells_area_left + p_col_info->margin_left + insert_tab_position;
            update_skel_rect.tl.pixit_point.y = ruler_horz_marker_y;
            update_skel_rect.br = update_skel_rect.tl;

            update_rect_flags = host_marker_rect_flags(ruler_marker_from_tab_type[p_docu->insert_tab_type - TAB_LEFT]);

            view_update_later(p_docu, UPDATE_RULER_HORZ, &update_skel_rect, update_rect_flags);

            status = style_apply_struct_to_source(p_docu, &style);
        }

        style_free_resources_all(&style);
        } /*block*/

        p_col_info->h_tab_list = 0;
    }

    /* now that we've placed a new tab, get the pointer to change shape, update status line etc */
    status_accumulate(status, ruler_horz_ruler_info_ensure(p_docu));

#if 0 /* new poller should come round real soon and get it */
    ruler_horz_wher_(p_docu, &p_skelevent_click->click_context, &p_skelevent_click->skel_point,
                    NULL, NULL, TRUE /*set pointer shape*/, TRUE /*set status line*/);
#endif

    return(status);
}

T5_MSG_PROTO(static, ruler_horz_event_click_left_single, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    RULER_MARKER ruler_marker;
    ARRAY_INDEX tab_number; /* valid iff ruler_marker is TAB_xxx */

    IGNOREPARM_InVal_(t5_message);

    p_skelevent_click->processed = 1;

    ruler_horz_where(p_docu, &p_skelevent_click->click_context, &p_skelevent_click->skel_point,
                     &ruler_marker /*filled*/, &tab_number /*filled*/, TRUE /*set pointer shape*/, TRUE /*set status line*/);

    switch(ruler_marker)
    {
    case RULER_NO_MARK_VALID_AREA:
        /* Place a tab where the user clicked */
        return(ruler_horz_event_click_left_single_place_tab(p_docu, p_skelevent_click));

    default: default_unhandled();
#if CHECKING
    case RULER_NO_MARK:
    case RULER_MARKER_MARGIN_LEFT:
    case RULER_MARKER_MARGIN_PARA:
    case RULER_MARKER_MARGIN_RIGHT:
    case RULER_MARKER_COL_RIGHT:
    case RULER_MARKER_TAB_LEFT:
    case RULER_MARKER_TAB_CENTRE:
    case RULER_MARKER_TAB_RIGHT:
    case RULER_MARKER_TAB_DECIMAL:
#endif
        return(STATUS_OK);
    }
}

/* left double-click on an existing tab, so delete it */

_Check_return_
static STATUS
ruler_horz_event_click_left_double_tab(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     RULER_MARKER ruler_marker,
    _InVal_     ARRAY_INDEX tab_number)
{
    STATUS status = STATUS_OK;
    RULER_INFO ruler_info = p_docu->ruler_horz_info.ruler_info;
    P_COL_INFO p_col_info = &ruler_info.col_info;

    assert(array_index_valid(&p_col_info->h_tab_list, tab_number));

    {
    P_TAB_INFO p_tab_info = array_ptr(&p_col_info->h_tab_list, TAB_INFO, tab_number);
    SKEL_RECT update_skel_rect;
    RECT_FLAGS update_rect_flags;
    RULER_HORZ_AREA_OFFSETS ruler_horz_area_offsets;

    ruler_horz_area_offsets_for_page(p_docu, p_skelevent_click->skel_point.page_num.y, &ruler_horz_area_offsets);

    update_skel_rect.tl.page_num.x = p_col_info->edge_left.page;
    update_skel_rect.tl.page_num.y = cur_page_y(p_docu);
    update_skel_rect.tl.pixit_point.x = p_col_info->edge_left.pixit + ruler_horz_area_offsets.cells_area_left + p_col_info->margin_left + p_tab_info->offset;
    update_skel_rect.tl.pixit_point.y = ruler_horz_marker_y;
    update_skel_rect.br = update_skel_rect.tl;
    update_rect_flags = host_marker_rect_flags(ruler_marker);
    view_update_later(p_docu, UPDATE_RULER_HORZ, &update_skel_rect, update_rect_flags);
    } /*block*/

    { /* copy and apply tab list, excluding the unwanted tab */
    STYLE style;

    style_init(&style);
    style_bit_set(&style, STYLE_SW_PS_TAB_LIST);

    if(status_ok(status = al_array_duplicate(&style.para_style.h_tab_list, &p_col_info->h_tab_list)))
    {
        al_array_delete_at(&style.para_style.h_tab_list, -1, tab_number);

        status = style_apply_struct_to_source(p_docu, &style);
    }

    style_free_resources_all(&style);
    } /*block*/

    p_col_info->h_tab_list = 0;

    /* now that we've deleted a tab, get the pointer to change shape, update status bar etc */
    status_accumulate(status, ruler_horz_ruler_info_ensure(p_docu));

#if 0 /* new poller should come round real soon and get change */
    ruler_horz_wher_(p_docu, &p_skelevent_click->click_context, &p_skelevent_click->skel_point,
                    NULL, NULL, TRUE /*set pointer shape*/, TRUE /*set status line*/);
#endif

    return(status);
}

T5_MSG_PROTO(static, ruler_horz_event_click_left_double, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    RULER_MARKER ruler_marker;
    ARRAY_INDEX tab_number;    /* valid iff ruler_marker is TAB_xxx */

    IGNOREPARM_InVal_(t5_message);

    p_skelevent_click->processed = 1;

    ruler_horz_where(p_docu, &p_skelevent_click->click_context, &p_skelevent_click->skel_point,
                     &ruler_marker /*filled*/, &tab_number /*filled*/, TRUE /*set pointer shape*/, TRUE /*set status line*/);

    switch(ruler_marker)
    {
    default: default_unhandled();
#if CHECKING
    case RULER_MARKER_MARGIN_LEFT:
    case RULER_MARKER_MARGIN_PARA:
    case RULER_MARKER_MARGIN_RIGHT:
#endif
        return(STATUS_OK);

    case RULER_MARKER_COL_RIGHT:
        /* left double click on column width marker, do an auto width */
        return(execute_command(OBJECT_ID_SKEL, p_docu, T5_CMD_AUTO_WIDTH, _P_DATA_NONE(P_ARGLIST_HANDLE)));

    case RULER_MARKER_TAB_LEFT:
    case RULER_MARKER_TAB_CENTRE:
    case RULER_MARKER_TAB_RIGHT:
    case RULER_MARKER_TAB_DECIMAL:
        return(ruler_horz_event_click_left_double_tab(p_docu, p_skelevent_click, ruler_marker, tab_number));
    }
}

/* right double-click on an existing tab, so mutate it */

_Check_return_
static STATUS
ruler_horz_event_click_right_double_tab(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     RULER_MARKER ruler_marker,
    _InVal_     ARRAY_INDEX tab_number)
{
    STATUS status = STATUS_OK;
    RULER_INFO ruler_info = p_docu->ruler_horz_info.ruler_info;
    P_COL_INFO p_col_info = &ruler_info.col_info;

    assert(array_index_valid(&p_col_info->h_tab_list, tab_number));

    {
    P_TAB_INFO p_tab_info = array_ptr(&p_col_info->h_tab_list, TAB_INFO, tab_number);
    SKEL_RECT update_skel_rect;
    RECT_FLAGS update_rect_flags;
    RULER_HORZ_AREA_OFFSETS ruler_horz_area_offsets;

    ruler_horz_area_offsets_for_page(p_docu, p_skelevent_click->skel_point.page_num.y, &ruler_horz_area_offsets);

    update_skel_rect.tl.page_num.x = p_col_info->edge_left.page;
    update_skel_rect.tl.page_num.y = cur_page_y(p_docu);
    update_skel_rect.tl.pixit_point.x = p_col_info->edge_left.pixit + ruler_horz_area_offsets.cells_area_left + p_col_info->margin_left + p_tab_info->offset;
    update_skel_rect.tl.pixit_point.y = ruler_horz_marker_y;
    update_skel_rect.br = update_skel_rect.tl;
    update_rect_flags = host_marker_rect_flags(ruler_marker);
    view_update_later(p_docu, UPDATE_RULER_HORZ, &update_skel_rect, update_rect_flags);
    } /*block*/

    { /* copy and apply tab list, mutating the selected tab */
    STYLE style;

    style_init(&style);
    style_bit_set(&style, STYLE_SW_PS_TAB_LIST);

    if(status_ok(status = al_array_duplicate(&style.para_style.h_tab_list, &p_col_info->h_tab_list)))
    {
        P_TAB_INFO p_tab_info = array_ptr(&style.para_style.h_tab_list, TAB_INFO, tab_number);

        p_tab_info->type += 1;

        if(p_tab_info->type >= N_TAB_TYPES)
            p_tab_info->type = 0;

        status = style_apply_struct_to_source(p_docu, &style);
    }

    style_free_resources_all(&style);
    } /*block*/

    p_col_info->h_tab_list = 0;

    /* now that we've mutated a tab, get the pointer to change shape, update status bar etc */
    status_accumulate(status, ruler_horz_ruler_info_ensure(p_docu));

    return(status);
}

T5_MSG_PROTO(static, ruler_horz_event_click_right_double, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    RULER_MARKER ruler_marker;
    ARRAY_INDEX tab_number;    /* valid iff ruler_marker is TAB_xxx */

    IGNOREPARM_InVal_(t5_message);

    p_skelevent_click->processed = 1;

    ruler_horz_where(p_docu, &p_skelevent_click->click_context, &p_skelevent_click->skel_point,
                     &ruler_marker /*filled*/, &tab_number /*filled*/, TRUE /*set pointer shape*/, TRUE /*set status line*/);

    switch(ruler_marker)
    {
    default: default_unhandled();
#if CHECKING
    case RULER_MARKER_MARGIN_LEFT:
    case RULER_MARKER_MARGIN_PARA:
    case RULER_MARKER_MARGIN_RIGHT:
#endif
        return(STATUS_OK);

    case RULER_MARKER_COL_RIGHT:
        /* right double click on column width marker, do a straddle */
        return(execute_command(OBJECT_ID_SKEL, p_docu, T5_CMD_STRADDLE_HORZ, _P_DATA_NONE(P_ARGLIST_HANDLE)));

    case RULER_MARKER_TAB_LEFT:
    case RULER_MARKER_TAB_CENTRE:
    case RULER_MARKER_TAB_RIGHT:
    case RULER_MARKER_TAB_DECIMAL:
        return(ruler_horz_event_click_right_double_tab(p_docu, p_skelevent_click, ruler_marker, tab_number));
    }
}

T5_MSG_PROTO(static, ruler_horz_event_click_drag, _InRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    RULER_MARKER ruler_marker;
    ARRAY_INDEX tab_number; /* valid iff ruler_marker is TAB_xxx */

    p_skelevent_click->processed = 1;

    ruler_horz_where(p_docu, &p_skelevent_click->click_context, &p_skelevent_click->skel_point,
                     &ruler_marker /*filled*/, &tab_number /*filled*/, TRUE /*set pointer shape*/, TRUE /*set status line*/);

    if(ruler_marker > RULER_NO_MARK)
    {
        if(status_ok(ruler_horz_drag_limits(p_docu, p_skelevent_click->skel_point.page_num, p_docu->cur.slr.col, ruler_marker, tab_number, t5_message, &adjust_marker_position_blk.ruler_horz)))
        {
            adjust_marker_position_blk.ruler_horz.reason_code = CB_CODE_RULER_HORZ_ADJUST_MARKER_POSITION;

            host_drag_start(&adjust_marker_position_blk.ruler_horz);

            quick_ublock_with_buffer_setup(adjust_marker_position_blk.ruler_horz.quick_ublock);

            if(status_ok(style_text_for_marker(p_docu, &adjust_marker_position_blk.ruler_horz.quick_ublock, ruler_status[ruler_marker].style_bit_number)))
                adjust_marker_position_blk.ruler_horz.drag_status.ustr_style_prefix = quick_ublock_ustr(&adjust_marker_position_blk.ruler_horz.quick_ublock);
        }
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ruler_horz_event_click_drag_movement_adjust_marker_position, _InRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION p_horz_blk = (P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION) p_skelevent_click->data.drag.p_reason_data;
    BOOL show = TRUE;

    if(T5_EVENT_CLICK_DRAG_MOVEMENT == t5_message)
    {
        PIXIT value = p_horz_blk->init_value + p_skelevent_click->data.drag.pixit_delta.x;

        value = click_stop_limited(value, p_horz_blk->min_limit, p_horz_blk->max_limit, p_horz_blk->click_stop_origin, &p_horz_blk->click_stop_step);

        if(p_horz_blk->value != value)
        {
            SKEL_RECT skel_rect;
            PIXIT delta;

            delta = p_horz_blk->value - p_horz_blk->init_value;

            skel_rect = p_horz_blk->update_skel_rect;
            skel_rect.tl.pixit_point.x += delta;
            skel_rect.br.pixit_point.x += delta;
            view_update_later(p_docu, UPDATE_RULER_HORZ, &skel_rect, p_horz_blk->update_rect_flags);

            if(p_horz_blk->tracking.ruler_marker > RULER_NO_MARK)
            {
                skel_rect = p_horz_blk->tracking.update_skel_rect;
                skel_rect.tl.pixit_point.x += delta;
                skel_rect.br.pixit_point.x += delta;
                view_update_later(p_docu, UPDATE_RULER_HORZ, &skel_rect, p_horz_blk->tracking.update_rect_flags);
            }

            p_horz_blk->value = value;

            delta = p_horz_blk->value - p_horz_blk->init_value;

            skel_rect = p_horz_blk->update_skel_rect;
            skel_rect.tl.pixit_point.x += delta;
            skel_rect.br.pixit_point.x += delta;
            view_update_later(p_docu, UPDATE_RULER_HORZ, &skel_rect, p_horz_blk->update_rect_flags);

            if(p_horz_blk->tracking.ruler_marker > RULER_NO_MARK)
            {
                skel_rect = p_horz_blk->tracking.update_skel_rect;
                skel_rect.tl.pixit_point.x += delta;
                skel_rect.br.pixit_point.x += delta;
                view_update_later(p_docu, UPDATE_RULER_HORZ, &skel_rect, p_horz_blk->tracking.update_rect_flags);
            }
        }
        else
            show = FALSE;
    }

    if(show)
    {
        PIXIT value = p_horz_blk->value - p_horz_blk->click_stop_origin;

        if(p_horz_blk->negative)
            value = -value;

        ruler_show_status(p_docu, &p_horz_blk->drag_status, value);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ruler_horz_event_click_drag_movement, _InRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION p_horz_blk = (P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION) p_skelevent_click->data.drag.p_reason_data;

    trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_ruler_horz T5_EVENT_CLICK_DRAG_MOVEMENT"));

    switch(p_horz_blk->reason_code)
    {
    case CB_CODE_RULER_HORZ_ADJUST_MARKER_POSITION:
        return(ruler_horz_event_click_drag_movement_adjust_marker_position(p_docu, t5_message, p_skelevent_click));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, ruler_horz_event_click_drag_finished_adjust_marker_position, _InRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION p_horz_blk = (P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION) p_skelevent_click->data.drag.p_reason_data;
    STATUS status = STATUS_OK;

    if(T5_EVENT_CLICK_DRAG_FINISHED == t5_message)
    {
        p_horz_blk->value = click_stop_limited(p_horz_blk->init_value + p_skelevent_click->data.drag.pixit_delta.x,
                                               p_horz_blk->min_limit, p_horz_blk->max_limit, p_horz_blk->click_stop_origin, &p_horz_blk->click_stop_step);

        status = ruler_horz_drag_ended(p_docu, p_horz_blk);
    }
    else /* T5_EVENT_CLICK_DRAG_ABORTED */
    {
        ruler_horz_update_whole_tabbar(p_docu, cur_page_y(p_docu));
    }

    status_accumulate(status, ruler_horz_ruler_info_ensure(p_docu));

    status_line_clear(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING);

    quick_ublock_dispose(&p_horz_blk->quick_ublock);

    return(status);
}

T5_MSG_PROTO(static, ruler_horz_event_click_drag_finished, _InRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION p_horz_blk = (P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION) p_skelevent_click->data.drag.p_reason_data;

    trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_ruler_horz T5_EVENT_CLICK_DRAG_FINISHED"));

    switch(p_horz_blk->reason_code)
    {
    case CB_CODE_RULER_HORZ_ADJUST_MARKER_POSITION:
        return(ruler_horz_event_click_drag_finished_adjust_marker_position(p_docu, t5_message, p_skelevent_click));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, ruler_horz_event_click_pointer_movement, _InRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    IGNOREPARM_InVal_(t5_message);

    trace_v0(TRACE_APP_CLICK,
            (t5_message == T5_EVENT_POINTER_ENTERS_WINDOW)
                ? TEXT("edge_window_event_ruler_horz T5_EVENT_POINTER_ENTERS_WINDOW")
                : TEXT("edge_window_event_ruler_horz T5_EVENT_POINTER_MOVEMENT"));

    ruler_horz_where(p_docu, &p_skelevent_click->click_context, &p_skelevent_click->skel_point, NULL, NULL, TRUE, TRUE);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ruler_horz_event_click_pointer_leaves_window, _InRef_ PC_SKELEVENT_CLICK p_skelevent_click)
{
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_skelevent_click);

    trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_ruler_horz T5_EVENT_POINTER_LEAVES_WINDOW"));

    status_line_clear(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING);

    return(STATUS_OK);
}

PROC_EVENT_PROTO(static, edge_window_event_ruler_horz)
{
    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(ruler_horz_event_redraw(p_docu, t5_message, (P_SKELEVENT_REDRAW) p_data));

    case T5_EVENT_CLICK_LEFT_SINGLE:
        return(ruler_horz_event_click_left_single(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_LEFT_DOUBLE:
        return(ruler_horz_event_click_left_double(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_RIGHT_DOUBLE:
        return(ruler_horz_event_click_right_double(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        return(ruler_horz_event_click_drag(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_STARTED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
        return(ruler_horz_event_click_drag_movement(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_ABORTED:
    case T5_EVENT_CLICK_DRAG_FINISHED:
        return(ruler_horz_event_click_drag_finished(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_POINTER_ENTERS_WINDOW:
    case T5_EVENT_POINTER_MOVEMENT:
        return(ruler_horz_event_click_pointer_movement(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_POINTER_LEAVES_WINDOW:
        return(ruler_horz_event_click_pointer_leaves_window(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* Draw horizontal ruler
*
* ruler scale, left, para and right margins, tab stops and col width indicator
*
******************************************************************************/

_Check_return_
_Ret_valid_
static inline PC_RGB
colour_of_ruler_scale(
    _InRef_     PC_STYLE p_ruler_style)
{
    return(
        style_bit_test(p_ruler_style, STYLE_SW_PS_RGB_GRID_LEFT)
            ? &p_ruler_style->para_style.rgb_grid_left
            : &rgb_stash[COLOUR_OF_RULER_SCALE]);
}

static void
col_info_adjust_for_drag_adjust_marker_position(
    _InoutRef_  P_COL_INFO p_col_info,
    _InRef_     PC_ARRAY_HANDLE p_h_tab_list_copy,
    _InRef_     P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION p_horz_blk)
{
    PIXIT value = p_horz_blk->value - p_horz_blk->click_stop_origin;

    switch(p_horz_blk->ruler_marker)
    {
    case RULER_MARKER_MARGIN_LEFT:
        p_col_info->margin_left = value;
        break;

    case RULER_MARKER_MARGIN_PARA:
        p_col_info->margin_para = value;
        break;

    case RULER_MARKER_MARGIN_RIGHT:
        p_col_info->margin_right = -value;
        break;

    case RULER_MARKER_COL_RIGHT:
        p_col_info->edge_right.pixit = p_col_info->edge_left.pixit + value;
        break;

    default: default_unhandled();
#if CHECKING
    case RULER_MARKER_TAB_LEFT:
    case RULER_MARKER_TAB_CENTRE:
    case RULER_MARKER_TAB_RIGHT:
    case RULER_MARKER_TAB_DECIMAL:
#endif
        {
        const PIXIT delta = p_horz_blk->value - p_horz_blk->init_value;
        const ARRAY_INDEX tab_list_elements = array_elements(p_h_tab_list_copy);
        ARRAY_INDEX i;

        for(i = p_horz_blk->tab_number; i < tab_list_elements; ++i)
        {
            const P_TAB_INFO p_tab_info = array_ptr(p_h_tab_list_copy, TAB_INFO, i);

            p_tab_info->offset += delta;

            if(p_horz_blk->dragging_single)
                break;
        }

        break;
        }
    }
}

static void
col_info_adjust_for_drag(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_COL_INFO p_col_info,
    _InRef_     PC_ARRAY_HANDLE p_h_tab_list_copy)
{
    P_ANY reasondata;

    if(host_drag_in_progress(p_docu, &reasondata))
    {
        const P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION p_horz_blk = (P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION) reasondata;

        switch(p_horz_blk->reason_code)
        {
        case CB_CODE_RULER_HORZ_ADJUST_MARKER_POSITION:
            col_info_adjust_for_drag_adjust_marker_position(p_col_info, p_h_tab_list_copy, p_horz_blk);
            break;

        default:
            break;
        }
    }
}

typedef struct HORZ_MARGINS_INFO
{
    PIXIT_POINT margin_left;
    PIXIT_POINT margin_para;
    PIXIT_POINT margin_right;
    PIXIT_POINT column_right;
}
HORZ_MARGINS_INFO, * P_HORZ_MARGINS_INFO;

T5_MSG_PROTO(static, ruler_horz_event_redraw, _InoutRef_ P_SKELEVENT_REDRAW p_skelevent_redraw)
{
    const PC_REDRAW_CONTEXT p_redraw_context = &p_skelevent_redraw->redraw_context;
    const PC_STYLE p_ruler_style = p_style_for_ruler_horz();
    RULER_HORZ_AREA_OFFSETS ruler_horz_area_offsets;
    RULER_INFO ruler_info;
    P_COL_INFO p_col_info;
    ARRAY_HANDLE h_tab_list_copy;
    PIXIT col_edge_left;
    PIXIT_LINE base_line;
    HORZ_MARGINS_INFO margins_info;

    IGNOREPARM_InVal_(t5_message);

    assert((p_skelevent_redraw->clip_skel_rect.tl.page_num.x == p_skelevent_redraw->clip_skel_rect.br.page_num.x) &&
           (p_skelevent_redraw->clip_skel_rect.tl.page_num.y == p_skelevent_redraw->clip_skel_rect.br.page_num.y));

    ruler_horz_area_offsets_for_page(p_docu, p_skelevent_redraw->clip_skel_rect.tl.page_num.y, &ruler_horz_area_offsets);

    base_line.tl.x = p_skelevent_redraw->clip_skel_rect.tl.pixit_point.x - (8 * PIXITS_PER_PROGRAM_PIXEL_X); /* extend to improve rendering of figures near clip edge */
    base_line.br.x = p_skelevent_redraw->clip_skel_rect.br.pixit_point.x + (8 * PIXITS_PER_PROGRAM_PIXEL_X);
    base_line.tl.y = RULER_HORZ_SCALE_TOP_Y;
    base_line.br.y = base_line.tl.y;
    base_line.horizontal = 1;

    ruler_scale_and_figures(p_docu, &base_line, ruler_horz_area_offsets.cells_area_left /*zero_origin*/, TRUE, p_redraw_context, p_ruler_style);

#if MARKERS_INSIDE_RULER_HORZ
#else
    /* base_line.br.y would be where to draw below if we have markers below base line */
    base_line.br.y += 1 * PIXITS_PER_PROGRAM_PIXEL_Y;

    ruler_horz_marker_y = base_line.br.y;
#endif

    if(!p_docu->ruler_horz_info.ruler_info.valid)
        status_assert(ruler_horz_ruler_info_ensure(p_docu));

    if(!p_docu->ruler_horz_info.ruler_info.valid)
        return(STATUS_OK);

    ruler_info = p_docu->ruler_horz_info.ruler_info; /* copy the supplied ruler_info block, as we alter it whilst dragging */

    p_col_info = &ruler_info.col_info;

    if(p_col_info->edge_left.page != p_skelevent_redraw->clip_skel_rect.tl.page_num.x)
        return(STATUS_OK);

    col_edge_left = ruler_horz_area_offsets.cells_area_left + p_col_info->edge_left.pixit;

    /* MUST copy tab list, 'cos we may be dragging tabs */
    status_assert(al_array_duplicate(&h_tab_list_copy, &p_col_info->h_tab_list));

    col_info_adjust_for_drag(p_docu, p_col_info, &h_tab_list_copy);

    p_col_info->h_tab_list = 0; /* from now on, use h_tab_list_copy */

    /* left margin is relative to left edge of column */
    /* para margin is relative to left margin */
    /* right margin is relative to right edge of column (opposite sign to normal!) */

    margins_info.margin_left.x  = col_edge_left + p_col_info->margin_left;
    margins_info.margin_para.x  = margins_info.margin_left.x + p_col_info->margin_para;
    margins_info.column_right.x = col_edge_left + (p_col_info->edge_right.pixit - p_col_info->edge_left.pixit);
    margins_info.margin_right.x = margins_info.column_right.x - p_col_info->margin_right;

    /* margins and tabs are plotted hanging down from ruler_horz_marker_y */
    margins_info.margin_left.y =
    margins_info.margin_para.y =
    margins_info.margin_right.y =
    margins_info.column_right.y =
        ruler_horz_marker_y;

    {
    ARRAY_INDEX i;

    assert_EQ((TAB_CENTRE  - TAB_LEFT), (RULER_MARKER_TAB_CENTRE  - RULER_MARKER_TAB_LEFT));
    assert_EQ((TAB_RIGHT   - TAB_LEFT), (RULER_MARKER_TAB_RIGHT   - RULER_MARKER_TAB_LEFT));
    assert_EQ((TAB_DECIMAL - TAB_LEFT), (RULER_MARKER_TAB_DECIMAL - RULER_MARKER_TAB_LEFT));

    for(i = 0; i < array_elements(&h_tab_list_copy); ++i)
    {
        P_TAB_INFO p_tab_info = array_ptr(&h_tab_list_copy, TAB_INFO, i);
        RULER_MARKER ruler_marker = ruler_marker_from_tab_type[p_tab_info->type - TAB_LEFT];
        PIXIT_POINT tab_stop;

        /* tabs are relative to left margin */
        tab_stop.x = margins_info.margin_left.x + p_tab_info->offset;
        tab_stop.y = ruler_horz_marker_y;

        /* disable display of tabs beyond right margin */
        if(tab_stop.x < margins_info.margin_right.x)
            host_paint_marker(p_redraw_context, ruler_marker, &tab_stop);
    }
    } /*block*/

    /* free copied tab list */
    al_array_dispose(&h_tab_list_copy);

    host_paint_marker(p_redraw_context, RULER_MARKER_MARGIN_LEFT, &margins_info.margin_left);
    host_paint_marker(p_redraw_context, RULER_MARKER_MARGIN_PARA, &margins_info.margin_para);
    host_paint_marker(p_redraw_context, RULER_MARKER_MARGIN_RIGHT, &margins_info.margin_right);
    host_paint_marker(p_redraw_context, RULER_MARKER_COL_RIGHT, &margins_info.column_right);

    return(STATUS_OK);
}

static void
ruler_horz_where(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_CLICK_CONTEXT p_click_context,
    P_SKEL_POINT p_skel_point,
    P_RULER_MARKER p_ruler_marker,
    P_ARRAY_INDEX p_tab_number,
    _InVal_     BOOL set_pointer_shape,
    _InVal_     BOOL set_status_line)
{
    RULER_MARKER ruler_marker;
    ARRAY_INDEX tab_number;
    PIXIT value = 0;

    ruler_marker = RULER_NO_MARK;
    tab_number = -1;

    if(p_docu->ruler_horz_info.ruler_info.valid)
    {
        RULER_INFO ruler_info = p_docu->ruler_horz_info.ruler_info;
        P_COL_INFO p_col_info = &ruler_info.col_info;

        /* check for marker hit only if on page containing cur.slr.col */
        if(p_skel_point->page_num.x == p_col_info->edge_left.page)
        {
            RULER_HORZ_AREA_OFFSETS ruler_horz_area_offsets;
            PIXIT col_edge_left;
            PIXIT_POINT marker_pos; /* absolute position (sort of) */

            ruler_horz_area_offsets_for_page(p_docu, p_skel_point->page_num.y, &ruler_horz_area_offsets);

            col_edge_left = ruler_horz_area_offsets.cells_area_left + p_col_info->edge_left.pixit;

            marker_pos.y = ruler_horz_marker_y;

            for(;;) /* loop for structure */
            {
                PIXIT margin_leftmost;
                PIXIT margin_right;

                ruler_marker = RULER_MARKER_MARGIN_RIGHT;
                value = MAX(p_col_info->margin_right, 0); /* forbid -ve right margins */
                marker_pos.x = col_edge_left + (p_col_info->edge_right.pixit - p_col_info->edge_left.pixit) - value;
                margin_right = marker_pos.x;

                if(host_over_marker(p_click_context, ruler_marker, &marker_pos, &p_skel_point->pixit_point))
                {
                    trace_0(TRACE_APP_CLICK, TEXT("Over right margin marker"));
                    break;
                }

                ruler_marker = RULER_MARKER_COL_RIGHT;
                value = p_col_info->edge_right.pixit - p_col_info->edge_left.pixit;
                marker_pos.x = col_edge_left + value;

                if(host_over_marker(p_click_context, ruler_marker, &marker_pos, &p_skel_point->pixit_point))
                {
                    trace_0(TRACE_APP_CLICK, TEXT("Over column width marker"));
                    break;
                }

                if(p_skel_point->pixit_point.x > margin_right)
                {
                    ruler_marker = RULER_NO_MARK;
                    break;
                }

                ruler_marker = RULER_MARKER_MARGIN_PARA;
                value = p_col_info->margin_para;
                marker_pos.x = col_edge_left + p_col_info->margin_left + value;
                margin_leftmost = marker_pos.x;

                if(host_over_marker(p_click_context, ruler_marker, &marker_pos, &p_skel_point->pixit_point))
                {
                    trace_0(TRACE_APP_CLICK, TEXT("Over paragraph indent marker"));
                    break;
                }

                ruler_marker = RULER_MARKER_MARGIN_LEFT;
                value = MAX(p_col_info->margin_left, 0); /* forbid -ve left margins */
                marker_pos.x = col_edge_left + value;
                margin_leftmost = MIN(margin_leftmost, marker_pos.x);

                if(host_over_marker(p_click_context, ruler_marker, &marker_pos, &p_skel_point->pixit_point))
                {
                    trace_0(TRACE_APP_CLICK, TEXT("Over left margin marker"));
                    break;
                }

                if(p_skel_point->pixit_point.x < margin_leftmost)
                {
                    ruler_marker = RULER_NO_MARK;
                    break;
                }

                /* check tabs in reverse order to which they are drawn */
                tab_number = array_elements(&p_col_info->h_tab_list);

                while(--tab_number >= 0)
                {
                    P_TAB_INFO p_tab_info = array_ptr(&p_col_info->h_tab_list, TAB_INFO, tab_number);
                    RULER_MARKER try_ruler_marker = ruler_marker_from_tab_type[p_tab_info->type - TAB_LEFT];

                    value = p_tab_info->offset;
                    marker_pos.x = col_edge_left + p_col_info->margin_left + value;

                    if(marker_pos.x > margin_right)
                        continue;

                    if(host_over_marker(p_click_context, try_ruler_marker, &marker_pos, &p_skel_point->pixit_point))
                    {
                        trace_1(TRACE_APP_CLICK, TEXT("Over tab ") S32_TFMT, (S32) tab_number);
                        ruler_marker = try_ruler_marker;
                        break; /* found tab, so break from tab scan loop  */
                    }
                }

                if(tab_number >= 0)
                    break;

                ruler_marker = RULER_NO_MARK_VALID_AREA; /* nothing found so far */

                break;  /* don't forget to quit the dummy loop */
                /*NOTREACHED*/
            }
        }
    }

    if(NULL != p_tab_number)
        *p_tab_number = tab_number;

    if(NULL != p_ruler_marker)
        *p_ruler_marker = ruler_marker;

    if(set_pointer_shape)
    {
        if(ruler_marker > RULER_NO_MARK)
            host_set_pointer_shape(ruler_status[ruler_marker].pointer_shape);
        else
            host_set_pointer_shape(POINTER_DEFAULT);
    }

    if(set_status_line)
    {
        if(ruler_marker == RULER_NO_MARK_VALID_AREA)
        {
            /* SKS 26nov95 tell them what they can do here. Too hard to tell them what style/region they're going to modify though */
            UI_TEXT ui_text;
            ui_text.type = UI_TEXT_TYPE_RESID;
            ui_text.text.resource_id = RULER_MSG_STATUS_RULER_HORZ_NO_MARK_VALID;
            status_line_set(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING, &ui_text);
        }
        else if(ruler_marker == RULER_NO_MARK)
        {
            UI_TEXT ui_text;
            ui_text.type = UI_TEXT_TYPE_RESID;
            ui_text.text.resource_id = RULER_MSG_STATUS_RULER_HORZ_NO_MARK;
            status_line_set(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING, &ui_text);
        }
        else
        {
            SCALE_INFO scale_info;
            DISPLAY_UNIT_INFO display_unit_info;
            RULER_DRAG_STATUS status_line;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
            quick_ublock_with_buffer_setup(quick_ublock);

            scale_info_from_docu(p_docu, TRUE, &scale_info);

            display_unit_info_from_display_unit(&display_unit_info, scale_info.display_unit);

            status_line.unit_message       = display_unit_info.unit_message;
            status_line.fp_pixits_per_unit = display_unit_info.fp_pixits_per_unit;

            status_assert(style_text_for_marker(p_docu, &quick_ublock, ruler_status[ruler_marker].style_bit_number));
            status_line.ustr_style_prefix = quick_ublock_ustr(&quick_ublock);

            status_line.drag_message = ruler_status[ruler_marker].drag_message;
            status_line.post_message = ruler_status[ruler_marker].post_message;

            ruler_show_status(p_docu, &status_line, value);

            quick_ublock_dispose(&quick_ublock);
        }
    }
}

/******************************************************************************
*
* Return the drag limits of the selected margins/tabs
*
******************************************************************************/

_Check_return_
static STATUS
ruler_horz_drag_limits(
    _DocuRef_   P_DOCU p_docu,
    _In_        PAGE_NUM page_num,
    _InVal_     COL col,
    _InVal_     RULER_MARKER ruler_marker,
    _InVal_     ARRAY_INDEX tab_number,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION p_horz_blk)
{
    STATUS status = STATUS_OK;
    PIXIT tracker = 0;
    RULER_INFO ruler_info;
    PC_COL_INFO p_col_info;
    RULER_HORZ_AREA_OFFSETS ruler_horz_area_offsets;
    PIXIT col_edge_left, column_width, work_area_x;

    if(!p_docu->ruler_horz_info.ruler_info.valid)
        return(STATUS_FAIL);

    ruler_info = p_docu->ruler_horz_info.ruler_info;

    ruler_info.col_info.margin_right = MAX(ruler_info.col_info.margin_right, 0); /* forbid -ve right margins */

    p_col_info = &ruler_info.col_info;

    ruler_horz_area_offsets_for_page(p_docu, page_num.y, &ruler_horz_area_offsets);

    col_edge_left = ruler_horz_area_offsets.cells_area_left + p_col_info->edge_left.pixit;
    column_width  = p_col_info->edge_right.pixit - p_col_info->edge_left.pixit;
    work_area_x   = p_docu->page_def.cells_usable_x;

    p_horz_blk->tracking.ruler_marker = RULER_NO_MARK;

    p_horz_blk->width_left  = 0;
    p_horz_blk->width_right = 0;

    p_horz_blk->negative = 0;

    /* many of ruler markers take column left edge as their origin */
    p_horz_blk->click_stop_origin = col_edge_left;

    p_horz_blk->dragging_single = FALSE;

    p_horz_blk->update_rect_flags = host_marker_rect_flags(ruler_marker);

    switch(ruler_marker)
    {
    case RULER_MARKER_MARGIN_LEFT:
        p_horz_blk->value = p_horz_blk->click_stop_origin + p_col_info->margin_left;

        p_horz_blk->min_limit = col_edge_left; /* maximum left, would hit column left edge  */
        p_horz_blk->max_limit = col_edge_left + column_width - p_col_info->margin_right; /* maximum right would hit margin_right, but may hit column right edge first */

        p_horz_blk->tracking.ruler_marker = RULER_MARKER_MARGIN_PARA;
        tracker = p_horz_blk->value + p_col_info->margin_para;

        if(p_col_info->margin_para > 0)
            p_horz_blk->max_limit -= p_col_info->margin_para;
        else if(p_col_info->margin_para < 0)
            p_horz_blk->min_limit -= p_col_info->margin_para;

        p_horz_blk->update_rect_flags.extend_right_window = TRUE; /* drags a lot around with it */
        break;

    case RULER_MARKER_MARGIN_PARA:
        p_horz_blk->click_stop_origin += p_col_info->margin_left;

        p_horz_blk->value = p_horz_blk->click_stop_origin + p_col_info->margin_para;

        p_horz_blk->min_limit = col_edge_left; /* maximum left, would hit column left edge  */
        p_horz_blk->max_limit = col_edge_left + column_width - p_col_info->margin_right; /* maximum right would hit margin_right, but may hit column right edge first */

        /* if there are tabs to the left of margin_left, we mustn't allow margin_para to be dragged passed them */
        if(array_elements(&p_col_info->h_tab_list))
        {
            PC_TAB_INFO p_tab_info = array_basec(&p_col_info->h_tab_list, TAB_INFO);

            if(p_tab_info->offset < 0)
                /* maximum right limited to the leftmost tab */
                p_horz_blk->max_limit = MIN(p_horz_blk->max_limit, p_horz_blk->click_stop_origin + p_tab_info->offset);
        }

        break;

    case RULER_MARKER_MARGIN_RIGHT:
        p_horz_blk->click_stop_origin = col_edge_left + column_width;

        p_horz_blk->value = p_horz_blk->click_stop_origin - p_col_info->margin_right;

        /* rightmost of paragraph and left margins limits right margin dragging leftwards */
        p_horz_blk->min_limit = col_edge_left + p_col_info->margin_left + MAX(p_col_info->margin_para, 0);
        p_horz_blk->max_limit = p_horz_blk->click_stop_origin; /* maximum right, would hit column right edge */

        p_horz_blk->negative = 1;

        p_horz_blk->update_rect_flags.extend_right_window = TRUE; /* may override/uncover tabs */
        break;

    case RULER_MARKER_COL_RIGHT:
        p_horz_blk->value = col_edge_left + column_width;

        /* rightmost of paragraph and left margins limits column dragging leftwards. also allow for right margin which moves as well */
        p_horz_blk->min_limit = col_edge_left + p_col_info->margin_left + MAX(p_col_info->margin_para, 0) + p_col_info->margin_right;
        p_horz_blk->max_limit = col_edge_left + work_area_x - p_col_info->edge_left.pixit; /* maximum right, limited to edge of printable area */

        tracker = col_edge_left + column_width - p_col_info->margin_right;
        p_horz_blk->tracking.ruler_marker = RULER_MARKER_MARGIN_RIGHT;

        p_horz_blk->dragging_single = (t5_message == T5_EVENT_CLICK_LEFT_DRAG);

        if(!p_horz_blk->dragging_single)
        {
            COL next_col = col + 1;
            ARRAY_HANDLE next_col_array = 0;

            if(next_col < p_docu->cols_logical) /* don't bother! assert in sk_col screws up debugging */
                status_break(status = skel_col_enum(p_docu, p_docu->cur.slr.row, (PAGE) -1, next_col, &next_col_array));

            if(0 == array_elements(&next_col_array))
            {   /* can't find next column, so just drag single column width */
                p_horz_blk->dragging_single = TRUE;
            }
            else
            {
                const PC_COL_INFO next_p_col_info = array_basec(&next_col_array, COL_INFO);
                PIXIT next_column_width = next_p_col_info->edge_right.pixit - next_p_col_info->edge_left.pixit;

                assert(1 == array_elements(&next_col_array));

                p_horz_blk->width_left  = column_width;
                p_horz_blk->width_right = next_column_width;

                p_horz_blk->max_limit = MIN(p_horz_blk->max_limit, col_edge_left + (p_horz_blk->width_left + p_horz_blk->width_right));
            }
        }

        p_horz_blk->update_rect_flags.extend_right_window = TRUE; /* moves right margin around too */
        break;

    default: default_unhandled();
#if CHECKING
    case RULER_MARKER_TAB_LEFT:
    case RULER_MARKER_TAB_CENTRE:
    case RULER_MARKER_TAB_RIGHT:
    case RULER_MARKER_TAB_DECIMAL:
#endif
        {
        P_TAB_INFO p_tab_info = array_ptr(&p_col_info->h_tab_list, TAB_INFO, tab_number);

        p_horz_blk->click_stop_origin += p_col_info->margin_left;

        p_horz_blk->value = p_horz_blk->click_stop_origin + p_tab_info->offset;

        /* leftmost of paragraph and left margins limits tab dragging */
        p_horz_blk->min_limit = col_edge_left + p_col_info->margin_left + MIN(p_col_info->margin_para, 0);
        p_horz_blk->max_limit = col_edge_left + column_width - p_col_info->margin_right;

        p_horz_blk->dragging_single = (t5_message == T5_EVENT_CLICK_RIGHT_DRAG);

        if(!p_horz_blk->dragging_single)
            p_horz_blk->update_rect_flags.extend_right_window = TRUE; /* drags other tabs around with it */

        /* never drag any tab to the left of its predecessor */
        if(tab_number > 0)
        {
            P_TAB_INFO p_tab_info_prev = array_ptr(&p_col_info->h_tab_list, TAB_INFO, tab_number - 1);

            p_horz_blk->min_limit = MAX(p_horz_blk->min_limit, (p_horz_blk->click_stop_origin + p_tab_info_prev->offset));
        }

        /* never drag single tab to the right of the next one along */
        if(p_horz_blk->dragging_single && (tab_number < array_elements(&p_col_info->h_tab_list) - 1))
        {
            P_TAB_INFO p_tab_info_next = array_ptr(&p_col_info->h_tab_list, TAB_INFO, array_elements(&p_col_info->h_tab_list) - 1);

            p_horz_blk->max_limit = MIN(p_horz_blk->max_limit, (p_horz_blk->click_stop_origin + p_tab_info_next->offset));
        }

        break;
        }
    }

    p_horz_blk->update_skel_rect.tl.page_num.x = p_col_info->edge_left.page;
    p_horz_blk->update_skel_rect.tl.page_num.y = page_num.y;
    p_horz_blk->update_skel_rect.tl.pixit_point.x = p_horz_blk->value;
    p_horz_blk->update_skel_rect.tl.pixit_point.y = ruler_horz_marker_y;
    p_horz_blk->update_skel_rect.br = p_horz_blk->update_skel_rect.tl;

    if(p_horz_blk->tracking.ruler_marker > RULER_NO_MARK)
    {
        p_horz_blk->tracking.update_rect_flags = host_marker_rect_flags(p_horz_blk->tracking.ruler_marker);
        p_horz_blk->tracking.update_skel_rect.tl = p_horz_blk->update_skel_rect.tl;
        p_horz_blk->tracking.update_skel_rect.tl.pixit_point.x = tracker;
        p_horz_blk->tracking.update_skel_rect.br = p_horz_blk->tracking.update_skel_rect.tl;
    }

    p_horz_blk->pag = page_num.x;
    p_horz_blk->col = col;

    p_horz_blk->ruler_marker = ruler_marker;
    p_horz_blk->tab_number   = tab_number;

    p_horz_blk->init_value = p_horz_blk->value;

    p_horz_blk->drag_status.ustr_style_prefix = NULL;
    p_horz_blk->drag_status.drag_message = ruler_status[ruler_marker].drag_message;
    p_horz_blk->drag_status.post_message = 0;

    {
    SCALE_INFO scale_info;
    DISPLAY_UNIT_INFO display_unit_info;

    scale_info_from_docu(p_docu, TRUE, &scale_info);

    display_unit_info_from_display_unit(&display_unit_info, scale_info.display_unit);

    p_horz_blk->drag_status.fp_pixits_per_unit = display_unit_info.fp_pixits_per_unit;
    p_horz_blk->drag_status.unit_message       = display_unit_info.unit_message;

    click_stop_initialise(&p_horz_blk->click_stop_step, &scale_info, TRUE);
    } /*block*/

    /* Mark claims he has ownership of the tab array, so I MUST NOT dispose of it*/

    return(status);
}

_Check_return_
static STATUS
ruler_horz_drag_ended(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CB_DATA_RULER_HORZ_ADJUST_MARKER_POSITION p_horz_blk)
{
    RULER_INFO ruler_info = p_docu->ruler_horz_info.ruler_info;
    const PC_COL_INFO p_col_info = &ruler_info.col_info;
    ARRAY_HANDLE h_tab_list_copy = 0;
    STYLE style;
    STYLE_BIT_NUMBER style_bit_number;
    STATUS status = STATUS_OK;
    PIXIT value = p_horz_blk->value - p_horz_blk->click_stop_origin;

    assert(p_docu->ruler_horz_info.ruler_info.valid);

    style_init(&style);

    switch(p_horz_blk->ruler_marker)
    {
    case RULER_MARKER_MARGIN_LEFT:
        style.para_style.margin_left = value;
        style_bit_number = STYLE_SW_PS_MARGIN_LEFT;
        break;

    case RULER_MARKER_MARGIN_PARA:
        style.para_style.margin_para = value;
        style_bit_number = STYLE_SW_PS_MARGIN_PARA;
        break;

    case RULER_MARKER_MARGIN_RIGHT:
        style.para_style.margin_right = -value;
        style_bit_number = STYLE_SW_PS_MARGIN_RIGHT;
        break;

    case RULER_MARKER_COL_RIGHT:
        if(!p_horz_blk->dragging_single)
        {
            COL_WIDTH_ADJUST col_width_adjust;

            col_width_adjust.col = p_horz_blk->col;
            col_width_adjust.width_left  = value;
            col_width_adjust.width_right = (p_horz_blk->width_left + p_horz_blk->width_right) - col_width_adjust.width_left;

            return(object_skel(p_docu, T5_MSG_COL_WIDTH_ADJUST, &col_width_adjust));
        }

        style.col_style.width = value;
        style_bit_number = STYLE_SW_CS_WIDTH;
        break;

    default: default_unhandled();
#if CHECKING
    case RULER_MARKER_TAB_LEFT:
    case RULER_MARKER_TAB_CENTRE:
    case RULER_MARKER_TAB_RIGHT:
    case RULER_MARKER_TAB_DECIMAL:
#endif
        {
        PIXIT delta = p_horz_blk->value - p_horz_blk->init_value;
        ARRAY_INDEX i;

        /* must copy tab list if we are dragging tab(s) */
        assert(p_horz_blk->tab_number >= 0);

        status_assert(al_array_duplicate(&h_tab_list_copy, &p_col_info->h_tab_list));

        for(i = p_horz_blk->tab_number; array_index_valid(&h_tab_list_copy, i); ++i)
        {
            P_TAB_INFO p_tab_info = array_ptr_no_checks(&h_tab_list_copy, TAB_INFO, i);

            p_tab_info->offset += delta;

            if(p_horz_blk->dragging_single)
                break;
        }

        style.para_style.h_tab_list = h_tab_list_copy;
        style_bit_number = STYLE_SW_PS_TAB_LIST;
        break;
        }
    }

    style_bit_set(&style, style_bit_number);

    status = style_apply_struct_to_source(p_docu, &style);

    al_array_dispose(&h_tab_list_copy);

    return(status);
}

/******************************************************************************
*
* Draw a ruler scale with figures
*
* If horizontal, ruler baseline runs from (start_xy, base_line_yx) to (end_xy, base_line_yx)
* if vertical,   ruler baseline runs from (base_line_yx, start_xy) to (base_line_yx, end_xy)
*
******************************************************************************/

#define BUF_RULERTEXT_MAX 12

#if RISCOS
/*#define RULER_SCALE_MAIN_HORZ    24 * PIXITS_PER_RISCOS*/ /* turn these on for calibration */
/*#define RULER_SCALE_MAIN_VERT    24 * PIXITS_PER_RISCOS*/
#define RULER_SCALE_COARSE_HORZ  8 * PIXITS_PER_RISCOS
#define RULER_SCALE_COARSE_VERT  8 * PIXITS_PER_RISCOS
#define RULER_SCALE_FINE_HORZ    4 * PIXITS_PER_RISCOS
#define RULER_SCALE_FINE_VERT    4 * PIXITS_PER_RISCOS
#elif WINDOWS
/*#define RULER_SCALE_MAIN_HORZ    12 * PIXITS_PER_PIXEL*/
/*#define RULER_SCALE_MAIN_VERT    12 * PIXITS_PER_PIXEL*/
#define RULER_SCALE_COARSE_HORZ  4 * PIXITS_PER_PIXEL
#define RULER_SCALE_COARSE_VERT  4 * PIXITS_PER_PIXEL
#define RULER_SCALE_FINE_HORZ    2 * PIXITS_PER_PIXEL
#define RULER_SCALE_FINE_VERT    2 * PIXITS_PER_PIXEL
#endif

static void
ruler_scale_and_figures(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_PIXIT_LINE p_base_line,
    _InVal_     PIXIT zero_xy,
    _InVal_     BOOL horizontal_ruler,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_STYLE p_ruler_style)
{
    const P_VIEW p_view = p_redraw_context->p_view;
    FONT_SPEC font_spec;
    const PC_RGB p_rgb_ruler_back = colour_of_ruler(p_ruler_style);
    const PC_RGB p_rgb_ruler_scale = colour_of_ruler_scale(p_ruler_style);
    const PC_RGB p_rgb_ruler_text = &font_spec.colour; /* will be defined very soon ... */
    SCALE_INFO scale_info;
    DISPLAY_UNIT_INFO display_unit_info;
    S32 coarse_div, fine_div;
    S32 numbered_units_multiplier;
    FP_PIXIT numbered_units_step_fp_pixits;
    FP_PIXIT coarse_step_fp_pixits;
    PIXIT fine_step_pixits;
    S32 numbered_units_loop_start, numbered_units_loop_end, numbered_units_loop;
    PIXIT_LINE centre_line = { { 0, 0 }, { 0, 0 }, 0 }; /* Keep dataflower happy */
    PIXIT_LINE mark_line = { { 0, 0 }, { 0, 0 }, 0 }; /* Keep dataflower happy */
    PIXIT_POINT init_figure_point = { 0, 0 }; /* Keep dataflower happy */
    HOST_FONT host_font_redraw = HOST_FONT_NONE;
    STATUS status;
    PIXIT start_xy = horizontal_ruler ? p_base_line->tl.x : p_base_line->tl.y;
    PIXIT end_xy   = horizontal_ruler ? p_base_line->br.x : p_base_line->br.y;
    PIXIT base_line = 0;
    PIXIT two_digits_width = 0;
    PIXIT digits_height = 0;
    S32 figure_divisor = 0;

    font_spec_from_ui_style(&font_spec, p_ruler_style);

    scale_info_from_docu(p_docu, horizontal_ruler, &scale_info);

    display_unit_info_from_display_unit(&display_unit_info, scale_info.display_unit);

    numbered_units_multiplier = scale_info.numbered_units_multiplier;

    numbered_units_step_fp_pixits = display_unit_info.fp_pixits_per_unit * scale_info.numbered_units_multiplier;

    coarse_div = scale_info.coarse_div;
    assert(coarse_div > 0);
    coarse_step_fp_pixits = numbered_units_step_fp_pixits / coarse_div;

    fine_div = scale_info.fine_div;
    assert(fine_div > 0);
    fine_step_pixits = (PIXIT) (coarse_step_fp_pixits / fine_div);

    start_xy -= zero_xy;
    end_xy   -= zero_xy;

    numbered_units_loop_start = div_round_floor_fn(start_xy, (PIXIT) numbered_units_step_fp_pixits); /* inclusive */
    numbered_units_loop_end   = div_round_ceil_fn(end_xy,    (PIXIT) numbered_units_step_fp_pixits); /* inclusive */

    if(status_ok(status = fonty_handle_from_font_spec(&font_spec, FALSE)))
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

#if RISCOS
    {
    const PIXIT pixits_per_riscos_d_y = PIXITS_PER_RISCOS * p_redraw_context->host_xform.riscos.d_y;
    _kernel_swi_regs rs;
    rs.r[0] = host_font_redraw;
    rs.r[1] = '0';
    rs.r[2] = 0;
    if(NULL == _kernel_swi(/*Font_CharBBox*/ 0x04008E, &rs, &rs))
    {
        two_digits_width = pixits_from_millipoints_ceil(abs(rs.r[3])) * 2;
        digits_height = pixits_from_millipoints_ceil(abs(rs.r[4]));
    }
    else
    {
        two_digits_width = 2 * 16 * PIXITS_PER_RISCOS;
        base_line = font_spec.size_y * 3 / 4;
        digits_height = base_line;
    }

    digits_height = div_round_ceil_u(digits_height, pixits_per_riscos_d_y);
    digits_height *= pixits_per_riscos_d_y;
    } /*block*/

    base_line = digits_height;
#elif WINDOWS
    { /* dpi-dependent pixels */
    const HDC hdc = p_redraw_context->windows.paintstruct.hdc;
    HFONT h_font_old = SelectFont(hdc, host_font_redraw);
    SIZE size;
    TEXTMETRIC textmetric;
    status_consume(GetTextExtentPoint32(hdc, TEXT("0123456789"), 5*2, &size));
    WrapOsBoolChecking(GetTextMetrics(hdc, &textmetric));
    consume(HFONT, SelectFont(hdc, h_font_old));

    two_digits_width = div_round_ceil_u(size.cx, 5); /* average width of two digits */
    digits_height    = textmetric.tmAscent - textmetric.tmInternalLeading;
    } /*block*/

    { /* convert to dpi-independent pixits */ /* DPI-aware */
    SIZE PixelsPerInch;
    host_get_pixel_size(NULL /*screen*/, &PixelsPerInch); /* Get current pixel size for the screen e.g. 96 or 120 */

    two_digits_width = div_round_ceil_u(two_digits_width * PIXITS_PER_INCH, PixelsPerInch.cx);
    digits_height    = div_round_ceil_u(digits_height    * PIXITS_PER_INCH, PixelsPerInch.cy);
    } /*block*/

    base_line = digits_height;
#endif

    centre_line.horizontal = !horizontal_ruler; /* marks are orthogonal to ruler */

    if(horizontal_ruler)
    {
        init_figure_point.y = RULER_HORZ_SCALE_TOP_Y;

        init_figure_point.y += base_line;

        p_base_line->tl.y = p_base_line->br.y = init_figure_point.y;
    reportf(TEXT("H BL y ") PIXIT_TFMT, p_base_line->br.y);

        centre_line.br.y = init_figure_point.y - (base_line / 2);

#if MARKERS_INSIDE_RULER_HORZ
        /* make sure we have room for the markers, recentre if needed */
        if(centre_line.br.y - (4 * PIXITS_PER_PROGRAM_PIXEL_Y) > 0)
            ruler_horz_marker_y = centre_line.br.y - (4 * PIXITS_PER_PROGRAM_PIXEL_Y);
        else
        {
            ruler_horz_marker_y = 0;
            centre_line.br.y = (4 * PIXITS_PER_PROGRAM_PIXEL_Y);
            init_figure_point.y = centre_line.br.y + (base_line / 2);
        }
#endif
    }
    else
    {
#if MARKERS_INSIDE_RULER_VERT
        const PIXIT digits_half_width = two_digits_width / 2;

        init_figure_point.x = RULER_VERT_SCALE_LEFT_X + digits_half_width;

        p_base_line->tl.x = p_base_line->br.x = init_figure_point.x;
    reportf(TEXT("V BL x ") PIXIT_TFMT, p_base_line->br.x);

        centre_line.br.x = init_figure_point.x;

        /* make sure we have room for the markers, recentre if needed */
        if(centre_line.br.x - (7 * PIXITS_PER_PROGRAM_PIXEL_X) > 0)
            ruler_vert_marker_x = centre_line.br.x - (7 * PIXITS_PER_PROGRAM_PIXEL_X);
        else
        {
            ruler_vert_marker_x = 0;
            centre_line.br.x = (7 * PIXITS_PER_PROGRAM_PIXEL_X);
            init_figure_point.x = centre_line.br.x + (base_line / 2);
        }
#else
        PIXIT digits_extra_width = (4 * PIXITS_PER_PROGRAM_PIXEL_X);

        init_figure_point.x = p_base_line->br.x + (2 * PIXITS_PER_PROGRAM_PIXEL_X);

        p_base_line->tl.x = p_base_line->br.x = init_figure_point.x + two_digits_width + digits_extra_width;
    reportf(TEXT("V BL x ") PIXIT_TFMT, p_base_line->br.x);

        init_figure_point.x += ((two_digits_width + digits_extra_width) / 2);

        centre_line.br.x = init_figure_point.x;
#endif

        if(p_redraw_context->host_xform.scale.t.y != p_redraw_context->host_xform.scale.b.y) /* abnormal transform as font size independent of scale */
            base_line = muldiv64(base_line, p_redraw_context->host_xform.scale.b.y, p_redraw_context->host_xform.scale.t.y);
    }

    mark_line = centre_line;

    /* at certain points on the view scale, start knocking out alternate figures */
    if(p_view->scalet != p_view->scaleb)
    {
        S32 t100 = 100 * p_view->scalet;

        if(t100 < 50 * p_view->scaleb)
        {
            figure_divisor = 2;
            if(t100 < 25 * p_view->scaleb)
            {
                figure_divisor = 5;
                if(t100 < 10 * p_view->scaleb)
                    figure_divisor = 10;
            }
        }
    }

    /*host_paint_line_solid(p_redraw_context, p_base_line, colour_of_ruler_scale(p_ruler_style));*/

    for(numbered_units_loop = numbered_units_loop_start;
        numbered_units_loop <= numbered_units_loop_end;
        ++numbered_units_loop)
    {
        PIXIT c_mark = zero_xy + (PIXIT) (numbered_units_loop * numbered_units_step_fp_pixits);
        PIXIT_POINT this_figure_point;

        if(horizontal_ruler)
        {
#if defined(RULER_SCALE_MAIN_HORZ)
            mark_line.tl.x = mark_line.br.x = c_mark;
            mark_line.br.y = centre_line.br.y + (RULER_SCALE_MAIN_HORZ / 2);
            mark_line.tl.y = mark_line.br.y - RULER_SCALE_MAIN_HORZ;

            host_paint_line_solid(p_redraw_context, &mark_line, p_rgb_ruler_scale);
#endif /* RULER_SCALE_MAIN_HORZ */

            this_figure_point.x = c_mark;
            this_figure_point.y = init_figure_point.y;

#if RISCOS
            /* try sub-pixel adjustment */
            this_figure_point.x += p_redraw_context->one_real_pixel.x / 2;
#elif WINDOWS
            /* try pixel adjustment */
            this_figure_point.x += p_redraw_context->one_real_pixel.x;
#endif
        }
        else
        {
#if defined(RULER_SCALE_MAIN_VERT)
            mark_line.tl.y = mark_line.br.y = c_mark;
            mark_line.br.x = centre_line.br.x + (RULER_SCALE_MAIN_VERT / 2);
            mark_line.tl.x = mark_line.br.x - RULER_SCALE_MAIN_VERT;

            host_paint_line_solid(p_redraw_context, &mark_line, p_rgb_ruler_scale);
#endif /* RULER_SCALE_MAIN_VERT */

            this_figure_point.x = init_figure_point.x;
            this_figure_point.y = c_mark;

            this_figure_point.y += base_line / 2;
        }

        {
        S32 coarse_loop;

        for(coarse_loop = 0; coarse_loop < coarse_div; ++coarse_loop)
        {
            PIXIT m_mark = c_mark;

            if(coarse_loop)
            {
                m_mark = c_mark + (PIXIT) (coarse_loop * coarse_step_fp_pixits);

                if(horizontal_ruler)
                {
                    mark_line.tl.x = mark_line.br.x = m_mark;
                    mark_line.br.y = centre_line.br.y + (RULER_SCALE_COARSE_HORZ / 2);
                    mark_line.tl.y = mark_line.br.y - RULER_SCALE_COARSE_HORZ;
                }
                else
                {
                    mark_line.tl.y = mark_line.br.y = m_mark;
                    mark_line.br.x = centre_line.br.x + (RULER_SCALE_COARSE_VERT / 2);
                    mark_line.tl.x = mark_line.br.x - RULER_SCALE_COARSE_VERT;
                }

                host_paint_line_solid(p_redraw_context, &mark_line, p_rgb_ruler_scale);
            }

#if defined(RULER_SCALE_FINE_HORZ)
            {
            S32 fine_loop;

            for(fine_loop = 1; fine_loop < fine_div; ++fine_loop)
            {
                /* since fine_loop contains no other loop, we can run it from '1' and avoid the '=0' test */
                PIXIT f_mark = m_mark + (fine_loop * fine_step_pixits);

                if(horizontal_ruler)
                {
                    mark_line.tl.x = mark_line.br.x = f_mark;
                    mark_line.br.y = centre_line.br.y + (RULER_SCALE_FINE_HORZ / 2);
                    mark_line.tl.y = mark_line.br.y - RULER_SCALE_FINE_HORZ;
                }
                else
                {
                    mark_line.tl.y = mark_line.br.y = f_mark;
                    mark_line.br.x = centre_line.br.x + (RULER_SCALE_FINE_VERT / 2);
                    mark_line.tl.x = mark_line.br.x - RULER_SCALE_FINE_VERT;
                }

                host_paint_line_solid(p_redraw_context, &mark_line, p_rgb_ruler_scale);
            }
            } /*block*/
#endif /* RULER_SCALE_FINE_HORZ */
        }
        } /*block*/

        {
        S32 figure = (S32) abs(numbered_units_loop * numbered_units_multiplier);

        if(!figure_divisor || (0 == (figure % figure_divisor)))
        {
            UCHARZ text[BUF_RULERTEXT_MAX];
            PC_UCHARS uchars = uchars_bptr(text);
            int len = xsnprintf(text, sizeof32(text), S32_FMT, figure); /* no need for ustr_ - it's just a number */
            host_fonty_text_paint_uchars_simple(p_redraw_context, &this_figure_point, uchars, len, p_rgb_ruler_text, p_rgb_ruler_back, host_font_redraw, TA_CENTER);
        }
        } /*block*/
    }

    /* host font handles belong to fonty session (upper redraw layers) */
}

_Check_return_
static STATUS
ruler_horz_ruler_info_ensure(
    _DocuRef_   P_DOCU p_docu)
{
    RULER_INFO ruler_info;

    ruler_info.valid = FALSE;
    ruler_info.col   = p_docu->cur.slr.col;

    if(status_ok(object_skel(p_docu, T5_MSG_RULER_INFO, &ruler_info)) && ruler_info.valid)
    {
        ARRAY_HANDLE array_handle;

        status_return(al_array_duplicate(&array_handle, &ruler_info.col_info.h_tab_list));

        al_array_dispose(&p_docu->ruler_horz_info.ruler_info.col_info.h_tab_list);

        p_docu->ruler_horz_info.ruler_info = ruler_info;
        p_docu->ruler_horz_info.ruler_info.col_info.h_tab_list = array_handle;
        p_docu->ruler_horz_info.ruler_info.valid = TRUE;

        return(STATUS_DONE);
    }

    return(STATUS_OK);
}

static void
ruler_horz_update_margins_and_tab_stops(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_RULER_INFO p_ruler_info_old,
    _InRef_     PC_RULER_INFO p_ruler_info_new)
{
    PC_COL_INFO p_col_info_old;
    PC_COL_INFO p_col_info_new;
    BOOL differ;

    if(p_ruler_info_old->valid != p_ruler_info_new->valid)
    {
        /* info valid state change, so repaint all of ruler */
        SKEL_RECT skel_rect;
        RECT_FLAGS rect_flags;

        skel_rect.tl.page_num.x = 0;
        skel_rect.tl.page_num.y = 0;
        skel_rect.tl.pixit_point.x = 0;
        skel_rect.tl.pixit_point.y = ruler_horz_marker_y;
        skel_rect.br = skel_rect.tl;

        RECT_FLAGS_CLEAR(rect_flags);
        rect_flags.extend_down_window = rect_flags.extend_right_window = 1;

        view_update_later(p_docu, UPDATE_RULER_HORZ, &skel_rect, rect_flags);

        return;
    }

    p_col_info_old = &p_ruler_info_old->col_info;
    p_col_info_new = &p_ruler_info_new->col_info;

    if(p_col_info_old->edge_left.page != p_col_info_new->edge_left.page)
    {
        /* current column now on a different page, so repaint all of ruler for old page and new page */
        SKEL_RECT skel_rect;
        RECT_FLAGS rect_flags;

        skel_rect.tl.page_num.y = 0;
        skel_rect.tl.pixit_point.x = 0;
        skel_rect.tl.pixit_point.y = ruler_horz_marker_y;
        skel_rect.br = skel_rect.tl;

        RECT_FLAGS_CLEAR(rect_flags);
        rect_flags.extend_down_window = rect_flags.extend_right_page = 1;

        skel_rect.tl.page_num.x  = skel_rect.br.page_num.x = p_col_info_old->edge_left.page;
        view_update_later(p_docu, UPDATE_RULER_HORZ, &skel_rect, rect_flags);

        skel_rect.tl.page_num.x  = skel_rect.br.page_num.x = p_col_info_new->edge_left.page;
        view_update_later(p_docu, UPDATE_RULER_HORZ, &skel_rect, rect_flags);

        return;
    }

    /* current column still on same page, so repaint only if the absolute position of every marker has changed */

    differ  = ((p_col_info_old->edge_left.pixit + p_col_info_old->margin_left ) !=
               (p_col_info_new->edge_left.pixit + p_col_info_new->margin_left ) );
    differ |= ((p_col_info_old->edge_left.pixit + p_col_info_old->margin_left + p_col_info_old->margin_para ) !=
               (p_col_info_new->edge_left.pixit + p_col_info_new->margin_left + p_col_info_new->margin_para ) );
    differ |= ((p_col_info_old->edge_right.pixit - p_col_info_old->margin_right ) !=
               (p_col_info_new->edge_right.pixit - p_col_info_new->margin_right ) );

    if(!differ)
    {
        ARRAY_INDEX tab_count, tab_index;
        P_TAB_INFO p_tab_info_old, p_tab_info_new;

        /* margin positions are identical (even though column left edges may differ!) */
        /* now check that tabs are identical in number, type and position */

        tab_count = array_elements(&p_col_info_new->h_tab_list);

        differ    = tab_count != array_elements(&p_col_info_old->h_tab_list);

        if(!differ)
        {
            for(tab_index = 0; tab_index < tab_count; ++tab_index)
            {
                p_tab_info_old = array_ptr(&p_col_info_old->h_tab_list, TAB_INFO, tab_index);
                p_tab_info_new = array_ptr(&p_col_info_new->h_tab_list, TAB_INFO, tab_index);

                if(  (p_tab_info_old->type !=
                      p_tab_info_new->type
                     )
                  ||
                     ( (p_col_info_old->edge_left.pixit + p_col_info_old->margin_left + p_tab_info_old->offset) !=
                       (p_col_info_new->edge_left.pixit + p_col_info_new->margin_left + p_tab_info_new->offset)
                     )
                  )
                {
                    differ = TRUE;
                    break;
                }
            }
        }
    }

    if(differ)
    {
        SKEL_RECT skel_rect;
        RECT_FLAGS rect_flags;

        skel_rect.tl.page_num.y = 0;
        skel_rect.tl.pixit_point.x = 0;
        skel_rect.tl.pixit_point.y = ruler_horz_marker_y;
        skel_rect.br = skel_rect.tl;

        RECT_FLAGS_CLEAR(rect_flags);
        rect_flags.extend_down_window = rect_flags.extend_right_page = 1;

        skel_rect.tl.page_num.x = skel_rect.br.page_num.x = p_col_info_new->edge_left.page;
        view_update_later(p_docu, UPDATE_RULER_HORZ, &skel_rect, rect_flags);
    }
}

static void
ruler_horz_update_whole_ruler(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y)
{
    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;

    skel_rect.tl.page_num.x = 0;
    skel_rect.tl.page_num.y = page_y;
    skel_rect.tl.pixit_point.x = 0;
    skel_rect.tl.pixit_point.y = 0;
    skel_rect.br = skel_rect.tl;

    RECT_FLAGS_CLEAR(rect_flags);
    rect_flags.extend_right_window = rect_flags.extend_down_window = 1;

    /* repaint whole ruler */
    view_update_later(p_docu, UPDATE_RULER_HORZ, &skel_rect, rect_flags);
}

static void
ruler_horz_update_whole_tabbar(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y)
{
    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;

    skel_rect.tl.page_num.x = 0;
    skel_rect.tl.page_num.y = page_y;
    skel_rect.tl.pixit_point.x = 0;
    skel_rect.tl.pixit_point.y = ruler_horz_marker_y;
    skel_rect.br = skel_rect.tl;

    RECT_FLAGS_CLEAR(rect_flags);
    rect_flags.extend_right_window = rect_flags.extend_down_window = 1;

    /* repaint whole tabbar (used to be just the lower part of ruler_horz) */
    view_update_later(p_docu, UPDATE_RULER_HORZ, &skel_rect, rect_flags);
}

/******************************************************************************
*
* Events for the ruler_vert come here
*
******************************************************************************/

T5_MSG_PROTO(static, ruler_vert_event_click_left_double, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    RULER_MARKER ruler_marker;

    IGNOREPARM_InVal_(t5_message);

    p_skelevent_click->processed = 1;

    ruler_vert_where(p_docu, &p_skelevent_click->click_context, &p_skelevent_click->skel_point,
                     &ruler_marker /*filled*/, TRUE /*set pointer shape*/, TRUE /*set status line*/);

    switch(ruler_marker)
    {
    case RULER_MARKER_ROW_BOTTOM:
        /* left double click on row height marker, do an auto height */
        return(execute_command(OBJECT_ID_SKEL, p_docu, T5_CMD_AUTO_HEIGHT, _P_DATA_NONE(P_ARGLIST_HANDLE)));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, ruler_vert_event_click_left_drag, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    RULER_MARKER ruler_marker;

    IGNOREPARM_InVal_(t5_message);

    p_skelevent_click->processed = 1;

    ruler_vert_where(p_docu, &p_skelevent_click->click_context, &p_skelevent_click->skel_point,
                     &ruler_marker /*filled*/, TRUE /*set pointer shape*/, TRUE /*set status line*/);

    if(ruler_marker > RULER_NO_MARK)
    {
        if(status_ok(ruler_vert_drag_limits(p_docu, ruler_marker, p_skelevent_click->skel_point.page_num.y, p_docu->cur.slr.row, &adjust_marker_position_blk.ruler_vert)))
        {
            adjust_marker_position_blk.ruler_vert.reason_code = CB_CODE_RULER_VERT_ADJUST_MARKER_POSITION;

            host_drag_start(&adjust_marker_position_blk.ruler_vert);

            quick_ublock_with_buffer_setup(adjust_marker_position_blk.ruler_vert.quick_ublock);

            if( (ruler_marker == RULER_MARKER_ROW_BOTTOM) &&
                status_ok(style_text_for_marker(p_docu, &adjust_marker_position_blk.ruler_vert.quick_ublock, ruler_status[ruler_marker].style_bit_number)))
                    adjust_marker_position_blk.ruler_vert.drag_status.ustr_style_prefix = quick_ublock_ustr(&adjust_marker_position_blk.ruler_vert.quick_ublock);
        }
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ruler_vert_event_click_drag_movement_adjust_marker_position, _InRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION p_vert_blk = (P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION) p_skelevent_click->data.drag.p_reason_data;
    BOOL show = 1;

    if(t5_message == T5_EVENT_CLICK_DRAG_MOVEMENT)
    {
        PIXIT value = click_stop_limited(p_vert_blk->init_value + p_skelevent_click->data.drag.pixit_delta.y,
                                         p_vert_blk->min_limit, p_vert_blk->max_limit, p_vert_blk->click_stop_origin, &p_vert_blk->click_stop_step);

        if(p_vert_blk->value != value)
        {
            SKEL_RECT skel_rect;
            PIXIT delta;

            /* force update of marker's current position */
            delta = p_vert_blk->value - p_vert_blk->init_value;

            skel_rect = p_vert_blk->update_skel_rect;
            skel_rect.tl.pixit_point.y += delta;
            skel_rect.br.pixit_point.y += delta;
            view_update_later(p_docu, UPDATE_RULER_VERT, &skel_rect, p_vert_blk->update_rect_flags);

            /* move marker to new position */
            p_vert_blk->value = value;

            delta = p_vert_blk->value - p_vert_blk->init_value;

            /* force update of new position */
            skel_rect = p_vert_blk->update_skel_rect;
            skel_rect.tl.pixit_point.y += delta;
            skel_rect.br.pixit_point.y += delta;
            view_update_later(p_docu, UPDATE_RULER_VERT, &skel_rect, p_vert_blk->update_rect_flags);
        }
        else
            show = 0;
    }

    if(show)
    {
        PIXIT value = p_vert_blk->value - p_vert_blk->click_stop_origin;

        if(p_vert_blk->negative)
            value = -value;

        ruler_show_status(p_docu, &p_vert_blk->drag_status, value);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ruler_vert_event_click_drag_movement, _InRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION p_vert_blk = (P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION) p_skelevent_click->data.drag.p_reason_data;

    trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_ruler_vert T5_EVENT_CLICK_DRAG_MOVEMENT"));

    switch(p_vert_blk->reason_code)
    {
    case CB_CODE_RULER_VERT_ADJUST_MARKER_POSITION:
        return(ruler_vert_event_click_drag_movement_adjust_marker_position(p_docu, t5_message, p_skelevent_click));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, ruler_vert_event_click_drag_finished_adjust_marker_position, _InRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION p_vert_blk = (P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION) p_skelevent_click->data.drag.p_reason_data;
    STATUS status = STATUS_OK;

    if(t5_message == T5_EVENT_CLICK_DRAG_FINISHED)
    {
        PIXIT value = click_stop_limited(p_vert_blk->init_value + p_skelevent_click->data.drag.pixit_delta.y,
                                         p_vert_blk->min_limit, p_vert_blk->max_limit, p_vert_blk->click_stop_origin, &p_vert_blk->click_stop_step);

        if(p_vert_blk->ruler_marker == RULER_MARKER_ROW_BOTTOM)
            status = ruler_vert_row_apply_delta(p_docu, p_vert_blk->row, value - p_vert_blk->init_value);
        else
            status = ruler_vert_drag_apply(p_docu, p_vert_blk->ruler_marker, p_vert_blk->pag, value - p_vert_blk->init_value);
    }
    else /* T5_EVENT_CLICK_DRAG_ABORTED */
    {
        SKEL_RECT skel_rect;
        RECT_FLAGS rect_flags;

        skel_rect.tl.pixit_point.x = 0;
        skel_rect.tl.pixit_point.y = 0;
        skel_rect.tl.page_num.x = 0;
        skel_rect.tl.page_num.y = p_vert_blk->pag;
        skel_rect.br = skel_rect.tl;

        RECT_FLAGS_CLEAR(rect_flags);
        rect_flags.extend_right_page = rect_flags.extend_down_window = 1;

        view_update_later(p_docu, UPDATE_RULER_VERT, &skel_rect, rect_flags);
    }

    status_line_clear(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING);

    quick_ublock_dispose(&p_vert_blk->quick_ublock);

    return(status);
}

T5_MSG_PROTO(static, ruler_vert_event_click_drag_finished, _InRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION p_vert_blk = (P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION) p_skelevent_click->data.drag.p_reason_data;

    trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_ruler_vert T5_EVENT_CLICK_DRAG_FINISHED"));

    switch(p_vert_blk->reason_code)
    {
    case CB_CODE_RULER_VERT_ADJUST_MARKER_POSITION:
        return(ruler_vert_event_click_drag_finished_adjust_marker_position(p_docu, t5_message, p_skelevent_click));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, ruler_vert_event_click_pointer_movement, _InRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    IGNOREPARM_InVal_(t5_message);

    trace_v0(TRACE_APP_CLICK,
            (t5_message == T5_EVENT_POINTER_ENTERS_WINDOW)
                ? TEXT("edge_window_event_ruler_vert T5_EVENT_POINTER_ENTERS_WINDOW")
                : TEXT("edge_window_event_ruler_vert T5_EVENT_POINTER_MOVEMENT"));

    ruler_vert_where(p_docu, &p_skelevent_click->click_context, &p_skelevent_click->skel_point,
                     NULL, TRUE, TRUE);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ruler_vert_event_click_pointer_leaves_window, _InRef_ PC_SKELEVENT_CLICK p_skelevent_click)
{
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_skelevent_click);

    trace_0(TRACE_APP_CLICK, TEXT("edge_window_event_ruler_vert T5_EVENT_POINTER_LEAVES_WINDOW"));

    status_line_clear(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING);

    return(STATUS_OK);
}

PROC_EVENT_PROTO(static, edge_window_event_ruler_vert)
{
    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(ruler_vert_event_redraw(p_docu, t5_message, (P_SKELEVENT_REDRAW) p_data));

    case T5_EVENT_CLICK_LEFT_DOUBLE:
        return(ruler_vert_event_click_left_double(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_LEFT_DRAG:
        return(ruler_vert_event_click_left_drag(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_STARTED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
        return(ruler_vert_event_click_drag_movement(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_ABORTED:
    case T5_EVENT_CLICK_DRAG_FINISHED:
        return(ruler_vert_event_click_drag_finished(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_POINTER_ENTERS_WINDOW:
    case T5_EVENT_POINTER_MOVEMENT:
        return(ruler_vert_event_click_pointer_movement(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_POINTER_LEAVES_WINDOW:
        return(ruler_vert_event_click_pointer_leaves_window(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    default:
    /*case T5_EVENT_CLICK_LEFT_SINGLE:*/
    /*case T5_EVENT_CLICK_LEFT_TRIPLE:*/
    /*case T5_EVENT_CLICK_RIGHT_SINGLE:*/
    /*case T5_EVENT_CLICK_RIGHT_DRAG:*/
    /*case T5_EVENT_CLICK_RIGHT_DOUBLE:*/
    /*case T5_EVENT_CLICK_RIGHT_TRIPLE:*/
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
ruler_vert_drag_apply(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     RULER_MARKER ruler_marker,
    _InVal_     PAGE page_y,
    _InVal_     PIXIT drag_delta_y)
{
    OBJECT_ID object_id_focus;
    ROW row;
    P_HEADFOOT_DEF p_headfoot_def;

    switch(ruler_marker)
    {
    default: default_unhandled();
#if CHECKING
    case RULER_MARKER_ROW_BOTTOM:
        assert0();

        /*FALLTHRU*/

    case RULER_MARKER_HEADER_EXT:
    case RULER_MARKER_HEADER_OFF:
#endif
        object_id_focus = OBJECT_ID_HEADER;
        break;

    case RULER_MARKER_FOOTER_EXT:
    case RULER_MARKER_FOOTER_OFF:
        object_id_focus = OBJECT_ID_FOOTER;
        break;
    }

    if(NULL == (p_headfoot_def = p_headfoot_def_from_page_y(p_docu, &row, page_y, object_id_focus)))
    {
        PTR_ASSERT(p_headfoot_def);
        return(STATUS_FAIL);
    }

    switch(ruler_marker)
    {
    case RULER_MARKER_HEADER_EXT:
        p_headfoot_def->headfoot_sizes.margin += drag_delta_y;
        break;

    case RULER_MARKER_FOOTER_EXT:
        p_headfoot_def->headfoot_sizes.margin -= drag_delta_y;
        break;

    case RULER_MARKER_HEADER_OFF:
    case RULER_MARKER_FOOTER_OFF:
        p_headfoot_def->headfoot_sizes.offset += drag_delta_y;
        break;
    }

    docu_modify(p_docu); /* cos nobody else does it */

    {
    DOCU_REFORMAT docu_reformat;
    docu_reformat.data_space = DATA_NONE;
    docu_reformat.action = REFORMAT_HEFO_Y;
    docu_reformat.data_type = REFORMAT_ROW;
    docu_reformat.data.row = row;
    status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
ruler_vert_drag_limits(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     RULER_MARKER ruler_marker,
    _InVal_     PAGE page_y,
    _InVal_     ROW row,
    _InoutRef_  P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION p_vert_blk)
{
    PIXIT work_area_y = p_docu->page_def.cells_usable_y;
    RULER_VERT_AREA_OFFSETS ruler_vert_area_offsets;

    ruler_vert_area_offsets_for_page(p_docu, page_y, &ruler_vert_area_offsets);

    p_vert_blk->negative = 0;

    switch(ruler_marker)
    {
    case RULER_MARKER_HEADER_EXT:
        p_vert_blk->click_stop_origin = ruler_vert_area_offsets.margin_header_top;

        p_vert_blk->value = ruler_vert_area_offsets.margin_header_bottom;

        p_vert_blk->min_limit = p_vert_blk->click_stop_origin;
        p_vert_blk->max_limit = ruler_vert_area_offsets.cells_area_bottom; /* would give zero height cells area */
        break;

    case RULER_MARKER_HEADER_OFF:
        p_vert_blk->click_stop_origin = ruler_vert_area_offsets.margin_header_top;

        p_vert_blk->value = ruler_vert_area_offsets.margin_header_text;

        p_vert_blk->min_limit = p_vert_blk->click_stop_origin;
        p_vert_blk->max_limit = ruler_vert_area_offsets.margin_header_bottom;
        break;

    case RULER_MARKER_FOOTER_EXT:
        p_vert_blk->click_stop_origin = ruler_vert_area_offsets.margin_footer_bottom;

        p_vert_blk->value = ruler_vert_area_offsets.margin_footer_top;

        p_vert_blk->min_limit = ruler_vert_area_offsets.cells_area_top; /* would give zero height cells area */
        p_vert_blk->max_limit = p_vert_blk->click_stop_origin;

        p_vert_blk->negative = 1;
        break;

    case RULER_MARKER_FOOTER_OFF:
        p_vert_blk->click_stop_origin = ruler_vert_area_offsets.margin_footer_top;

        p_vert_blk->value = ruler_vert_area_offsets.margin_footer_text;

        p_vert_blk->min_limit = p_vert_blk->click_stop_origin;
        p_vert_blk->max_limit = ruler_vert_area_offsets.margin_footer_bottom;
        break;

    default: default_unhandled();
#if CHECKING
    case RULER_MARKER_ROW_BOTTOM:
#endif
        {
        ROW_ENTRY row_entry;
        row_entry_from_row(p_docu, &row_entry, row);

        p_vert_blk->click_stop_origin = ruler_vert_area_offsets.cells_area_top + row_entry.rowtab.edge_top.pixit;
        p_vert_blk->value = ruler_vert_area_offsets.cells_area_top + row_entry.rowtab.edge_bot.pixit;

#if 0
        /* we can reduce row height to zero if whole of row is on page, or until bottom of row is at top of page */
        /* we can increase row height until bottom of row is at bottom of page */
        delta_min = ((row_entry.rowtab.edge_top.page == row_entry.rowtab.edge_bot.page)
                  ? row_entry.rowtab.edge_top.pixit - row_entry.rowtab.edge_bot.pixit
                  : 0 - row_entry.rowtab.edge_bot.pixit);
#endif

        p_vert_blk->min_limit = p_vert_blk->click_stop_origin;
        p_vert_blk->max_limit = ruler_vert_area_offsets.cells_area_top + work_area_y;
        break;
        }
    }

    p_vert_blk->init_value = p_vert_blk->value;

    p_vert_blk->update_rect_flags = host_marker_rect_flags(ruler_marker);
    p_vert_blk->update_skel_rect.tl.pixit_point.x = ruler_vert_marker_x;
    p_vert_blk->update_skel_rect.tl.pixit_point.y = p_vert_blk->value;
    p_vert_blk->update_skel_rect.tl.page_num.x = 0;
    p_vert_blk->update_skel_rect.tl.page_num.y = page_y;
    p_vert_blk->update_skel_rect.br = p_vert_blk->update_skel_rect.tl;

    p_vert_blk->pag = page_y;
    p_vert_blk->row = row;   /* only used if dragging row height */
    p_vert_blk->ruler_marker = ruler_marker;

    p_vert_blk->drag_status.ustr_style_prefix = NULL;
    p_vert_blk->drag_status.drag_message = ruler_status[ruler_marker].drag_message;
    p_vert_blk->drag_status.post_message = 0;

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

_Check_return_ _Success_(status_ok(return))
static STATUS
ruler_vert_markers_for_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y,
    _InVal_     ROW row,
    _OutRef_    P_PIXIT p_bottom,
    _OutRef_    P_PIXIT p_top)
{
    ROW_ENTRY row_entry;
    row_entry_from_row(p_docu, &row_entry, row);

    if((row_entry.rowtab.edge_top.page == row_entry.rowtab.edge_bot.page) && (row_entry.rowtab.edge_bot.page == page_y))
    {
        *p_bottom = row_entry.rowtab.edge_bot.pixit;
        *p_top    = row_entry.rowtab.edge_top.pixit;
        return(STATUS_OK);
    }

    return(STATUS_FAIL);
}

/******************************************************************************
*
* Draw vertical ruler
*
* ruler scale, header margin, footer margin and row height indicators
*
******************************************************************************/

typedef struct VERT_MARGINS_INFO
{
    PIXIT_POINT margin_header;
    PIXIT_POINT margin_header_text;
    PIXIT_POINT margin_footer;
    PIXIT_POINT margin_footer_text;
    PIXIT_POINT row_bottom;
}
VERT_MARGINS_INFO, * P_VERT_MARGINS_INFO;

static void
vert_margins_info_adjust_for_drag_adjust_marker_position(
    _InRef_     PC_SKELEVENT_REDRAW p_skelevent_redraw,
    _InoutRef_  P_VERT_MARGINS_INFO p_margins_info,
    _InRef_     P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION p_vert_blk)
{
    if(p_skelevent_redraw->clip_skel_rect.tl.page_num.y == p_vert_blk->pag)
    {
        PIXIT value = p_vert_blk->value;

        switch(p_vert_blk->ruler_marker)
        {
        default: default_unhandled();
#if CHECKING
        case RULER_MARKER_ROW_BOTTOM:
#endif
            p_margins_info->row_bottom.y = value;
            break;

        case RULER_MARKER_HEADER_EXT:
            p_margins_info->margin_header.y = value;
            break;

        case RULER_MARKER_HEADER_OFF:
            p_margins_info->margin_header_text.y = value;
            break;

        case RULER_MARKER_FOOTER_EXT:
            p_margins_info->margin_footer.y = -value;
            break;

        case RULER_MARKER_FOOTER_OFF:
            p_margins_info->margin_footer_text.y = value;
            break;
        }
    }

}

static void
vert_margins_info_adjust_for_drag(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SKELEVENT_REDRAW p_skelevent_redraw,
    _InoutRef_  P_VERT_MARGINS_INFO p_margins_info)
{
    P_ANY reasondata;

    if(host_drag_in_progress(p_docu, &reasondata))
    {
        const P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION p_vert_blk = (P_CB_DATA_RULER_VERT_ADJUST_MARKER_POSITION) reasondata;

        switch(p_vert_blk->reason_code)
        {
        case CB_CODE_RULER_VERT_ADJUST_MARKER_POSITION:
            vert_margins_info_adjust_for_drag_adjust_marker_position(p_skelevent_redraw, p_margins_info, p_vert_blk);
            break;

        default:
            break;
        }
    }
}

T5_MSG_PROTO(static, ruler_vert_event_redraw, _InoutRef_ P_SKELEVENT_REDRAW p_skelevent_redraw)
{
    const PC_REDRAW_CONTEXT p_redraw_context = &p_skelevent_redraw->redraw_context;
    const PC_STYLE p_ruler_style = p_style_for_ruler_vert();
    RULER_VERT_AREA_OFFSETS ruler_vert_area_offsets;
    BOOL show_row;
    PIXIT_LINE base_line;
    VERT_MARGINS_INFO margins_info;

    IGNOREPARM_InVal_(t5_message);

    assert((p_skelevent_redraw->clip_skel_rect.tl.page_num.x == p_skelevent_redraw->clip_skel_rect.br.page_num.x) &&
           (p_skelevent_redraw->clip_skel_rect.tl.page_num.y == p_skelevent_redraw->clip_skel_rect.br.page_num.y));

    ruler_vert_area_offsets_for_page(p_docu, p_skelevent_redraw->clip_skel_rect.tl.page_num.y, &ruler_vert_area_offsets);

    base_line.tl.x = RULER_VERT_SCALE_LEFT_X;
    base_line.br.x = base_line.tl.x;
    base_line.tl.y = p_skelevent_redraw->clip_skel_rect.tl.pixit_point.y - (8 * PIXITS_PER_PROGRAM_PIXEL_Y); /* extend to improve rendering of figures near clip edge */
    base_line.br.y = p_skelevent_redraw->clip_skel_rect.br.pixit_point.y + (8 * PIXITS_PER_PROGRAM_PIXEL_Y);
    base_line.horizontal = 0;

    ruler_scale_and_figures(p_docu, &base_line, ruler_vert_area_offsets.cells_area_top /*zero_origin*/, FALSE, p_redraw_context, p_ruler_style);

#if MARKERS_INSIDE_RULER_VERT
#else
    /* base_line.br.x would be where to draw to the right of if we have markers to the right of base line */
    base_line.br.x += 2 * PIXITS_PER_PROGRAM_PIXEL_X;

    ruler_vert_marker_x = base_line.br.x;
#endif

    {
    PIXIT row_top;
    show_row = status_ok(ruler_vert_markers_for_row(p_docu, p_skelevent_redraw->clip_skel_rect.tl.page_num.y, p_docu->cur.slr.row, &margins_info.row_bottom.y, &row_top));
    } /*block*/

    /* margins are plotted to the right of ruler_horz_marker_x */
    margins_info.margin_header.x =
    margins_info.margin_header_text.x =
    margins_info.margin_footer.x =
    margins_info.margin_footer_text.x =
    margins_info.row_bottom.x =
        ruler_vert_marker_x;

    margins_info.margin_header.y      = ruler_vert_area_offsets.margin_header_bottom; /* header margin is the top of the work area */
    margins_info.margin_header_text.y = ruler_vert_area_offsets.margin_header_text;
    margins_info.margin_footer.y      = ruler_vert_area_offsets.margin_footer_top;    /* footer margin is the bottom of the work area */
    margins_info.margin_footer_text.y = ruler_vert_area_offsets.margin_footer_text;
    margins_info.row_bottom.y        += ruler_vert_area_offsets.cells_area_top;       /* row bottom is the bottom of the current slot: += 'cos value was cells area relative */

    vert_margins_info_adjust_for_drag(p_docu, p_skelevent_redraw, &margins_info);

    switch(p_redraw_context->display_mode)
    {
    default: default_unhandled();
#if CHECKING
    case DISPLAY_DESK_AREA:
    case DISPLAY_PRINT_AREA:
#endif
        host_paint_marker(p_redraw_context, RULER_MARKER_HEADER_EXT, &margins_info.margin_header);
        host_paint_marker(p_redraw_context, RULER_MARKER_HEADER_OFF, &margins_info.margin_header_text);
        host_paint_marker(p_redraw_context, RULER_MARKER_FOOTER_EXT, &margins_info.margin_footer);
        host_paint_marker(p_redraw_context, RULER_MARKER_FOOTER_OFF, &margins_info.margin_footer_text);
        break;

    case DISPLAY_WORK_AREA:
        /* hefo margins are not displayed */
        break;
    }

    if(show_row)
        host_paint_marker(p_redraw_context, RULER_MARKER_ROW_BOTTOM, &margins_info.row_bottom);

    return(STATUS_OK);
}

static void
ruler_vert_where(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_CLICK_CONTEXT p_click_context,
    P_SKEL_POINT p_skel_point,
    P_RULER_MARKER p_ruler_marker,
    _InVal_     BOOL set_pointer_shape,
    _InVal_     BOOL set_status_line)
{
    RULER_VERT_AREA_OFFSETS ruler_vert_area_offsets;
    RULER_MARKER ruler_marker = RULER_NO_MARK;
    PIXIT_POINT marker_pos; /* absolute position (sort of) */
    PIXIT value = 0; /* the number the user is used to seeing in the style dboxes */

    ruler_vert_area_offsets_for_page(p_docu, p_skel_point->page_num.y, &ruler_vert_area_offsets);

    marker_pos.x = ruler_vert_marker_x;

    for(;;) /* dummy loop, to allow break-out */
    {
        PIXIT row_bottom, row_top;
        RULER_MARKER ruler_marker_try;

        if(status_ok(ruler_vert_markers_for_row(p_docu, p_skel_point->page_num.y, p_docu->cur.slr.row, &row_bottom, &row_top)))
        {
            ruler_marker_try = RULER_MARKER_ROW_BOTTOM;
            marker_pos.y = ruler_vert_area_offsets.cells_area_top + row_bottom; /* 'cos value returned was cells area relative */
            if(host_over_marker(p_click_context, ruler_marker_try, &marker_pos, &p_skel_point->pixit_point))
            {
                switch(p_docu->focus_owner)
                {
                default:
                    break;

                case OBJECT_ID_CELLS:
                    trace_0(TRACE_APP_CLICK, TEXT("Over row height adjustor"));
                    ruler_marker = ruler_marker_try;
                    value = row_bottom - row_top;
                    break;
                }
            }

            if(RULER_NO_MARK != ruler_marker)
                break;
        }

        switch(p_click_context->display_mode)
        {
        default: default_unhandled();
#if CHECKING
        case DISPLAY_DESK_AREA:
        case DISPLAY_PRINT_AREA:
#endif
            ruler_marker_try = RULER_MARKER_HEADER_EXT;
            value = ruler_vert_area_offsets.margin_header_bottom - ruler_vert_area_offsets.margin_header_top;
            marker_pos.y = ruler_vert_area_offsets.margin_header_bottom;
            if(host_over_marker(p_click_context, ruler_marker_try, &marker_pos, &p_skel_point->pixit_point))
            {
                trace_0(TRACE_APP_CLICK, TEXT("Over header margin marker"));
                ruler_marker = ruler_marker_try;
                break;
            }

            ruler_marker_try = RULER_MARKER_HEADER_OFF;
            value = ruler_vert_area_offsets.margin_header_text - ruler_vert_area_offsets.margin_header_top;
            marker_pos.y = ruler_vert_area_offsets.margin_header_text;
            if(host_over_marker(p_click_context, ruler_marker_try, &marker_pos, &p_skel_point->pixit_point))
            {
                trace_0(TRACE_APP_CLICK, TEXT("Over header text offset marker"));
                ruler_marker = ruler_marker_try;
                break;
            }

            ruler_marker_try = RULER_MARKER_FOOTER_EXT;
            value = ruler_vert_area_offsets.margin_footer_bottom - ruler_vert_area_offsets.margin_footer_top;
            marker_pos.y = ruler_vert_area_offsets.margin_footer_top;
            if(host_over_marker(p_click_context, ruler_marker_try, &marker_pos, &p_skel_point->pixit_point))
            {
                trace_0(TRACE_APP_CLICK, TEXT("Over footer margin marker"));
                ruler_marker = ruler_marker_try;
                break;
            }

            ruler_marker_try = RULER_MARKER_FOOTER_OFF;
            value = ruler_vert_area_offsets.margin_footer_text - ruler_vert_area_offsets.margin_footer_top;
            marker_pos.y = ruler_vert_area_offsets.margin_footer_text;
            if(host_over_marker(p_click_context, ruler_marker_try, &marker_pos, &p_skel_point->pixit_point))
            {
                trace_0(TRACE_APP_CLICK, TEXT("Over footer text offset marker"));
                ruler_marker = ruler_marker_try;
                break;
            }

            break;

        case DISPLAY_WORK_AREA:
            break;
        }

        break; /* don't forget to quit the loop */
        /*NOTREACHED*/
    }

    if(NULL != p_ruler_marker)
        *p_ruler_marker = ruler_marker;

    if(set_pointer_shape)
    {
        if(ruler_marker > RULER_NO_MARK)
            host_set_pointer_shape(ruler_status[ruler_marker].pointer_shape);
        else
            host_set_pointer_shape(POINTER_DEFAULT);
    }

    if(set_status_line)
    {
        if(ruler_marker > RULER_NO_MARK)
        {
            SCALE_INFO scale_info;
            DISPLAY_UNIT_INFO display_unit_info;
            RULER_DRAG_STATUS status_line;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
            quick_ublock_with_buffer_setup(quick_ublock);

            scale_info_from_docu(p_docu, FALSE, &scale_info);

            display_unit_info_from_display_unit(&display_unit_info, scale_info.display_unit);

            status_line.fp_pixits_per_unit= display_unit_info.fp_pixits_per_unit;
            status_line.unit_message      = display_unit_info.unit_message;

            status_line.ustr_style_prefix = NULL;
            if( (ruler_marker == RULER_MARKER_ROW_BOTTOM)  &&
                status_ok(style_text_for_marker(p_docu, &quick_ublock, ruler_status[ruler_marker].style_bit_number)))
                    status_line.ustr_style_prefix = quick_ublock_ustr(&quick_ublock);

            status_line.drag_message = ruler_status[ruler_marker].drag_message;
            status_line.post_message = ruler_status[ruler_marker].post_message;

            ruler_show_status(p_docu, &status_line, value);

            quick_ublock_dispose(&quick_ublock);
        }
        else
        {
#if 1 /* SKS 26nov95 tell them what they can do here */
            UI_TEXT ui_text;
            ui_text.type = UI_TEXT_TYPE_RESID;
            ui_text.text.resource_id = RULER_MSG_STATUS_RULER_VERT_NO_MARK;
            status_line_set(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING, &ui_text);
#else
            status_line_clear(p_docu, STATUS_LINE_LEVEL_PANEWINDOW_TRACKING);
#endif
        }
    }
}

static void
ruler_horz_update(
    _DocuRef_   P_DOCU p_docu)
{
    RULER_INFO ruler_info;

    ruler_info.valid = FALSE;
    ruler_info.col = p_docu->cur.slr.col;
    ruler_info.col_info.h_tab_list = 0;

    if(status_fail(object_skel(p_docu, T5_MSG_RULER_INFO, &ruler_info)))
        ruler_info.valid = FALSE;

    if(ruler_info.valid)
    {
        ARRAY_HANDLE array_handle;
        status_assert(al_array_duplicate(&array_handle, &ruler_info.col_info.h_tab_list)); /* failure immaterial */
        ruler_info.col_info.h_tab_list = array_handle;
    }

    ruler_horz_update_margins_and_tab_stops(p_docu, &p_docu->ruler_horz_info.ruler_info, &ruler_info);

    al_array_dispose(&p_docu->ruler_horz_info.ruler_info.col_info.h_tab_list);

    p_docu->ruler_horz_info.ruler_info = ruler_info;
}

/******************************************************************************
*
* This master event handler acts for all the ruler_horz and ruler_vert edge windows
* in all the (0..n) views of this document
*
******************************************************************************/

static void
ruler_msg_docu_colrow(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr_old)
{
    /* move the row marker up and down as needed too */
    RECT_FLAGS rect_flags;
    SKEL_RECT old_slot, new_slot, skel_rect;
    RULER_VERT_AREA_OFFSETS ruler_vert_area_offsets;

    if(!p_docu->flags.document_active)
        return;

    ruler_horz_update(p_docu);

    if(p_docu->cur.slr.row == p_slr_old->row)
        return;

    if(p_slr_old->row >= n_rows(p_docu))
        return;

    if(p_slr_old->col >= n_cols_logical(p_docu))
        return;

    rect_flags = host_marker_rect_flags(RULER_MARKER_ROW_BOTTOM);

    skel_rect_from_slr(p_docu, &old_slot, p_slr_old);
    skel_rect_from_slr(p_docu, &new_slot, &p_docu->cur.slr);

    ruler_vert_area_offsets_for_page(p_docu, old_slot.tl.page_num.y, &ruler_vert_area_offsets);

    skel_rect.br.page_num.x = 0;
    skel_rect.br.page_num.y = old_slot.br.page_num.y;
    skel_rect.br.pixit_point.x = ruler_vert_marker_x;
    skel_rect.br.pixit_point.y = old_slot.br.pixit_point.y + ruler_vert_area_offsets.cells_area_top;
    skel_rect.tl = skel_rect.br;
    view_update_later(p_docu, UPDATE_RULER_VERT, &skel_rect, rect_flags);

    if(old_slot.tl.page_num.y != new_slot.tl.page_num.y)
    {
         /* binding margins are a pain */
         ruler_horz_update_whole_ruler(p_docu, old_slot.tl.page_num.y);
         ruler_horz_update_whole_ruler(p_docu, new_slot.tl.page_num.y);

         ruler_vert_area_offsets_for_page(p_docu, new_slot.tl.page_num.y, &ruler_vert_area_offsets);
     }

    skel_rect.br.page_num.y = new_slot.br.page_num.y;
    skel_rect.br.pixit_point.y = new_slot.br.pixit_point.y + ruler_vert_area_offsets.cells_area_top;
    skel_rect.tl = skel_rect.br;
    view_update_later(p_docu, UPDATE_RULER_VERT, &skel_rect, rect_flags);
}

MAEVE_EVENT_PROTO(static, maeve_event_ob_ruler)
{
    const STATUS status = STATUS_OK;

    IGNOREPARM_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_SELECTION_NEW:
    case T5_MSG_FOCUS_CHANGED:
    case T5_MSG_CUR_CHANGE_AFTER:
    case T5_MSG_STYLE_CHANGED:
    case T5_MSG_STYLE_DOCU_AREA_CHANGED:
        {
        if(!p_docu->flags.document_active)
            break;

        ruler_horz_update(p_docu);

        break;
        }

    case T5_MSG_DOCU_COLROW:
        ruler_msg_docu_colrow(p_docu, (PC_SLR) p_data);
        break;

    default:
        break;
    }

    return(status);
}

static void
ruler_show_status(
    _DocuRef_   P_DOCU p_docu,
    P_RULER_DRAG_STATUS p_drag_status,
    _InVal_     PIXIT value)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 96);
    quick_ublock_with_buffer_setup(quick_ublock);

    for(;;) /* loop for structure */
    {
        if(p_drag_status->ustr_style_prefix)
        {
            status_break(status = quick_ublock_ustr_add(&quick_ublock, p_drag_status->ustr_style_prefix));
            status_break(status = quick_ublock_ustr_add(&quick_ublock, USTR_TEXT(". ")));
        }

        status_break(status = resource_lookup_quick_ublock(&quick_ublock, p_drag_status->drag_message));

        status_break(status = quick_ublock_ustr_add(&quick_ublock, USTR_TEXT(": ")));

        {
        EV_DATA ev_data;
        STATUS numform_resource_id;
        UCHARZ numform_unit_ustr_buf[elemof32("0.,####")];
        NUMFORM_PARMS numform_parms;

        ev_data_set_real(&ev_data, /*FP_USER_UNIT*/ ((FP_PIXIT) value / p_drag_status->fp_pixits_per_unit));

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

        status_break(status = numform(&quick_ublock, P_QUICK_TBLOCK_NONE, &ev_data, &numform_parms));
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

/******************************************************************************
*
* Return position of cells area relative to paper for a given page
*
* Used by ruler_horz when ruler and tab markers etc
*
******************************************************************************/

static void
ruler_horz_area_offsets_for_page(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y,
    _OutRef_    P_RULER_HORZ_AREA_OFFSETS p_ruler_horz_area_offsets)
{
    PAGE_NUM page_num;
    PIXIT_RECT pixit_rect_paper;
    PIXIT_RECT pixit_rect_cells_area;
    PIXIT_RECT clip_rect; /* data not used */

    page_num.x = 0;
    page_num.y = page_y;

    pixit_rect_from_page(p_docu, &pixit_rect_paper,      &clip_rect, DISPLAY_DESK_AREA, UPDATE_PANE_PAPER,      &page_num);
    pixit_rect_from_page(p_docu, &pixit_rect_cells_area, &clip_rect, DISPLAY_DESK_AREA, UPDATE_PANE_CELLS_AREA, &page_num);

    p_ruler_horz_area_offsets->cells_area_left  = pixit_rect_cells_area.tl.x - pixit_rect_paper.tl.x;
    p_ruler_horz_area_offsets->cells_area_right = pixit_rect_cells_area.br.x - pixit_rect_paper.tl.x;
}

/******************************************************************************
*
* Return positions of various headers footers
* etc relative to workarea for given page
*
* Used by ruler_vert when plotting margin markers etc
*
******************************************************************************/

static void
ruler_vert_area_offsets_for_page(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y,
    _OutRef_    P_RULER_VERT_AREA_OFFSETS p_ruler_vert_area_offsets)
{
    PAGE_NUM page_num;
    PIXIT_RECT pixit_rect_paper;
    PIXIT_RECT pixit_rect_header;
    PIXIT_RECT pixit_rect_col;
    PIXIT_RECT pixit_rect_cells_area;
    PIXIT_RECT pixit_rect_footer;
    PIXIT_RECT clip_rect; /* data not used */
    HEADFOOT_BOTH headfoot_both;

    page_num.x = 0;
    page_num.y = page_y;

    pixit_rect_from_page(p_docu, &pixit_rect_paper,      &clip_rect, DISPLAY_DESK_AREA, UPDATE_PANE_PAPER,         &page_num);
    pixit_rect_from_page(p_docu, &pixit_rect_header,     &clip_rect, DISPLAY_DESK_AREA, UPDATE_PANE_MARGIN_HEADER, &page_num);
    pixit_rect_from_page(p_docu, &pixit_rect_col,        &clip_rect, DISPLAY_DESK_AREA, UPDATE_PANE_MARGIN_COL,    &page_num);
    pixit_rect_from_page(p_docu, &pixit_rect_cells_area, &clip_rect, DISPLAY_DESK_AREA, UPDATE_PANE_CELLS_AREA,    &page_num);
    pixit_rect_from_page(p_docu, &pixit_rect_footer,     &clip_rect, DISPLAY_DESK_AREA, UPDATE_PANE_MARGIN_FOOTER, &page_num);

    p_ruler_vert_area_offsets->margin_header_top    = pixit_rect_header.tl.y    - pixit_rect_paper.tl.y;
    p_ruler_vert_area_offsets->margin_header_bottom = pixit_rect_header.br.y    - pixit_rect_paper.tl.y;
    p_ruler_vert_area_offsets->margin_col_top       = pixit_rect_col.tl.y       - pixit_rect_paper.tl.y;
    p_ruler_vert_area_offsets->margin_col_bottom    = pixit_rect_col.br.y       - pixit_rect_paper.tl.y;
    p_ruler_vert_area_offsets->cells_area_top       = pixit_rect_cells_area.tl.y - pixit_rect_paper.tl.y;
    p_ruler_vert_area_offsets->cells_area_bottom    = pixit_rect_cells_area.br.y - pixit_rect_paper.tl.y;
    p_ruler_vert_area_offsets->margin_footer_top    = pixit_rect_footer.tl.y    - pixit_rect_paper.tl.y;
    p_ruler_vert_area_offsets->margin_footer_bottom = pixit_rect_footer.br.y    - pixit_rect_paper.tl.y;

    headfoot_both_from_page_y(p_docu, &headfoot_both, page_y);

    p_ruler_vert_area_offsets->margin_header_text = p_ruler_vert_area_offsets->margin_header_top + headfoot_both.header.offset;
    p_ruler_vert_area_offsets->margin_footer_text = p_ruler_vert_area_offsets->margin_footer_top + headfoot_both.footer.offset;
}

_Check_return_
static STATUS
ruler_vert_row_apply_delta(
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

/******************************************************************************
*
* ruler object event handler
*
******************************************************************************/

_Check_return_
static STATUS
ruler_msg_startup(void)
{
    view_install_edge_window_handler(WIN_RULER_HORZ, edge_window_event_ruler_horz);
    view_install_edge_window_handler(WIN_RULER_VERT, edge_window_event_ruler_vert);

    return(STATUS_OK);
}

_Check_return_
static STATUS
ruler_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    p_docu->ruler_horz_info.ruler_info.valid = FALSE;
    p_docu->ruler_horz_info.ruler_info.col_info.h_tab_list = 0;

    return(maeve_event_handler_add(p_docu, maeve_event_ob_ruler, (CLIENT_HANDLE) 0));
}

_Check_return_
static STATUS
ruler_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    maeve_event_handler_del(p_docu, maeve_event_ob_ruler, (CLIENT_HANDLE) 0);

    if(p_docu->ruler_horz_info.ruler_info.valid)
    {
        p_docu->ruler_horz_info.ruler_info.valid = FALSE;

        al_array_dispose(&p_docu->ruler_horz_info.ruler_info.col_info.h_tab_list);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ruler_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(resource_init(OBJECT_ID_RULER, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_RULER));

        return(ruler_msg_startup());

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_RULER));

    case T5_MSG_IC__INIT1:
        return(ruler_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(ruler_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
ruler_msg_choices_changed(
    _DocuRef_   P_DOCU p_docu)
{
    /* see if ruler info change pertinent */
    BOOL repaint_ruler_horz = 0;
    BOOL repaint_ruler_vert = 0;
    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;

    /* more testing might have been possible, but it's really the job of the Choices box to give us a bitset of changes */
    if(!p_docu->scale_info.loaded)
        repaint_ruler_horz = 1;

    if(!p_docu->vscale_info.loaded && !p_docu->scale_info.loaded)
        repaint_ruler_vert = 1;

    skel_rect.tl.page_num.x = skel_rect.tl.page_num.y = 0;
    skel_rect.tl.pixit_point.x = skel_rect.tl.pixit_point.y = 0;
    skel_rect.br = skel_rect.tl;

    RECT_FLAGS_CLEAR(rect_flags);
    rect_flags.extend_right_window = rect_flags.extend_down_window = 1;

    if(repaint_ruler_horz)
        view_update_later(p_docu, UPDATE_RULER_HORZ, &skel_rect, rect_flags);

    if(repaint_ruler_vert)
        view_update_later(p_docu, UPDATE_RULER_VERT, &skel_rect, rect_flags);

    return(STATUS_OK);
}

OBJECT_PROTO(extern, object_ruler);
OBJECT_PROTO(extern, object_ruler)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(ruler_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_CHOICES_CHANGED:
        return(ruler_msg_choices_changed(p_docu));

    default:
        return(STATUS_OK);
    }
}

/* end of ob_ruler.c */
