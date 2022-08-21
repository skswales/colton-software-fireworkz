/* ob_note.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* notelayer object module for Fireworkz */

/* MRJC September 1992; SKS July 1993 / May 1994 moved to module */

#include "common/gflags.h"

#include "ob_note/ob_note.h"

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_NOTE)
extern PC_U8 rb_note_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_NOTE &rb_note_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_NOTE DONT_LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_NOTE DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_NOTE DONT_LOAD_RESOURCES

/*
internal structure
*/

typedef struct NOTE_DRAG_STATUS
{
    STATUS      drag_message;
    PIXIT       param[6];
    STATUS      unit_message;
    FP_PIXIT    fp_pixits_per_unit;
    STATUS      post_message;
}
NOTE_DRAG_STATUS, * P_NOTE_DRAG_STATUS;

typedef struct CB_DATA_NOTELAYER_ADJUST
{
    BOOL tl_x, tl_y, br_x, br_y;
}
CB_DATA_NOTELAYER_ADJUST;

typedef struct CB_DATA_NOTELAYER_DRAG_DATA
{
    S32 reason_code;

    PIXIT_SIZE original_pixit_size; /* original size of note */
    GR_SCALE_PAIR original_gr_scale_pair; /* original scale factors of note */

    SKEL_POINT skel_point; /* last recorded pointer position */

    SKEL_RECT skel_rect_current; /* the raw unnormalised skel_rect one gets from adding skel_point and translate_box_pixit_rect_offset etc. */

    CB_DATA_NOTELAYER_ADJUST resize;
    BOOL resize_tie_xy;
    SKEL_POINT resize_first_skel_point;
    SKEL_RECT resize_original_skel_rect;
    PIXIT_SIZE resize_current_pixit_size;

    GR_SCALE_PAIR rescale_gr_scale_pair;

    PIXIT_RECT translate_box_pixit_rect_offset; /* offset of dragging box (when translating) from the skel_point where the pointer is */

    NOTE_DRAG_STATUS note_drag_status;
}
CB_DATA_NOTELAYER_DRAG_DATA, * P_CB_DATA_NOTELAYER_DRAG_DATA;

typedef struct NOTE_DIRN NOTE_DIRN;

/* PIXIT_RECT_EAR_XXX */
typedef S32 NOTE_POSITION; typedef NOTE_POSITION * P_NOTE_POSITION;

typedef struct NOTE_OBJECT_BOUNDS
{
    PIXIT_RECT_EARS ears;
    BOOL pinned[PIXIT_RECT_EAR_COUNT];
}
NOTE_OBJECT_BOUNDS, * P_NOTE_OBJECT_BOUNDS, ** P_P_NOTE_OBJECT_BOUNDS;

#define POINT_INSIDE_RECTANGLE(click_point, obj_rect) ( \
    (click_point.x >= obj_rect.tl.x) && \
    (click_point.x <  obj_rect.br.x) && \
    (click_point.y >= obj_rect.tl.y) && \
    (click_point.y <  obj_rect.br.y)    )

#define RECTANGLE_IS_VISIBLE(clip_skel_rect, obj_rect) ( \
    (clip_skel_rect.br.pixit_point.x >  obj_rect.tl.x) && \
    (clip_skel_rect.br.pixit_point.y >  obj_rect.tl.y) && \
    (clip_skel_rect.tl.pixit_point.x <  obj_rect.br.x) && \
    (clip_skel_rect.tl.pixit_point.y <  obj_rect.br.y)    )

/*
internal routines
*/

static void
expand_pixit_rect(
    P_PIXIT_RECT p_pixit_rect,
    P_PIXIT_POINT p_one_pixel);

_Check_return_
static STATUS
notelayer_rect_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw,
    _InVal_     LAYER layer);

static void
notelayer_invert_drag_boxes(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     LAYER layer);

static void
notelayer_object_bounds(
    P_NOTE_OBJECT_BOUNDS p_note_object_bounds,
    _In_        NOTE_PINNING note_pinning,
    P_PIXIT_RECT p_pixit_rect,
    P_PIXIT_POINT p_one_pixel);

#define rgb_frame      &rgb_stash[11 /*red*/]
#define rgb_ear_outer  &rgb_stash[11 /*red*/]
#define rgb_ear_inner  &rgb_stash[0 /*white*/]
#define rgb_greyed_out &rgb_stash[4 /*grey*/]

/*
only one drag operation goes on at once
*/

static CB_DATA_NOTELAYER_DRAG_DATA notelayer_drag_data;

#if 0

/* NB ensure this table kept in step with grab handle defs */

static const POINTER_SHAPE
notelayer_pointer_shape[PIXIT_RECT_EAR_COUNT] =
{
    POINTER_MOVE,           /* centre       */
    POINTER_DRAG_ALL,       /* top left     */
    POINTER_DRAG_UPDOWN,    /* top          */
    POINTER_DRAG_LEFTRIGHT, /* left         */
    POINTER_DRAG_ALL,       /* bottom left  */
    POINTER_DRAG_ALL,       /* top right    */
    POINTER_DRAG_UPDOWN,    /* bottom       */
    POINTER_DRAG_LEFTRIGHT, /* right        */
    POINTER_DRAG_ALL        /* bottom right */
};

#endif

static const CB_DATA_NOTELAYER_ADJUST
notelayer_resize[PIXIT_RECT_EAR_COUNT] =
{   /*   tl      br  */
    /*  x, y,   x, y */
    {   0, 0,   0, 0    },  /* centre       */
    {   1, 1,   0, 0    },  /* top left     */
    {   0, 1,   0, 0    },  /* top          */
    {   1, 0,   0, 0    },  /* left         */
    {   1, 0,   0, 1    },  /* bottom left  */
    {   0, 1,   1, 0    },  /* top right    */
    {   0, 0,   0, 1    },  /* bottom       */
    {   0, 0,   1, 0    },  /* right        */
    {   0, 0,   1, 1    }   /* bottom right */
};

static BOOL static_flag = 0; /* nasty temporary bodge for suppressing otherwise degenerate selection redraw */

static void
note_select(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    if(NULL == notelayer_selection_first(p_docu))
    {
        /* tell selections in other layers to clear */
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

        p_docu->focus_owner_old = p_docu->focus_owner;

        /* claim focus */
        caret_show_claim(p_docu, OBJECT_ID_NOTE, FALSE);
    }

    p_note_info->note_selection = NOTE_SELECTION_SELECTED;

    note_update_later(p_docu, p_note_info, NOTE_UPDATE_SELECTION_MARKS);

    /* tell everyone that selection has changed */
    status_assert(maeve_event(p_docu, T5_MSG_SELECTION_NEW, P_DATA_NONE));
}

extern void
note_update_later(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info,
    _In_        S32 note_update_flags)
{
    RECT_FLAGS rect_flags;
    REDRAW_TAG redraw_tag;
    SKEL_RECT skel_rect;

    if((note_update_flags == NOTE_UPDATE_SELECTION_MARKS) && (p_note_info->note_selection == NOTE_SELECTION_NONE))
        /* nothing to do */
        return;

    RECT_FLAGS_CLEAR(rect_flags);

    /* extend slightly anyway so that thin (zero width vertical or zero height horizontal) lines are displayed */
    rect_flags.extend_left_ppixels  =
    rect_flags.extend_right_ppixels = (p_note_info->note_selection != NOTE_SELECTION_NONE) ? 6 /*ears*/ : 1 /*thin*/;

    rect_flags.extend_up_ppixels    =
    rect_flags.extend_down_ppixels  = (p_note_info->note_selection != NOTE_SELECTION_NONE) ? 3 /*ears*/ : 1 /*thin*/;

    redraw_tag = redraw_tag_from_layer(p_note_info->layer);

    if(NOTE_UNPINNED == p_note_info->note_pinning)
    {
        if(p_note_info->flags.all_pages)
        {
            note_update_flags = NOTE_UPDATE_ALL;

            rect_flags.extend_right_window = rect_flags.extend_down_window = TRUE;
        }
    }

    if(!p_note_info->flags.skel_rect_valid)
        notelayer_mount_note(p_docu, p_note_info);

    if(note_update_flags == NOTE_UPDATE_SELECTION_MARKS)
    {
        /* top */
        skel_rect = p_note_info->skel_rect;
        skel_rect.br.page_num.y = skel_rect.tl.page_num.y;
        skel_rect.br.pixit_point.y = skel_rect.tl.pixit_point.y;
        view_update_later(p_docu, redraw_tag, &skel_rect, rect_flags);

        /* bottom */
        skel_rect = p_note_info->skel_rect;
        skel_rect.tl.page_num.y = skel_rect.br.page_num.y;
        skel_rect.tl.pixit_point.y = skel_rect.br.pixit_point.y;
        view_update_later(p_docu, redraw_tag, &skel_rect, rect_flags);

        /* left */
        skel_rect = p_note_info->skel_rect;
        skel_rect.br.page_num.x = skel_rect.tl.page_num.x;
        skel_rect.br.pixit_point.x = skel_rect.tl.pixit_point.x;
        view_update_later(p_docu, redraw_tag, &skel_rect, rect_flags);

        /* right */
        skel_rect = p_note_info->skel_rect;
        skel_rect.tl.page_num.x = skel_rect.br.page_num.x;
        skel_rect.tl.pixit_point.x = skel_rect.br.pixit_point.x;
        view_update_later(p_docu, redraw_tag, &skel_rect, rect_flags);
    }
    else
    {
        skel_rect = p_note_info->skel_rect;
        view_update_later(p_docu, redraw_tag, &skel_rect, rect_flags);
    }
}

/******************************************************************************
*
* Return the number of notes in the selection
*
******************************************************************************/

_Check_return_
static S32
notelayer_count_selection(
    _DocuRef_   P_DOCU p_docu)
{
    S32 selection_count = 0;
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    while(--note_index >= 0)
    {
        const PC_NOTE_INFO p_note_info = array_ptrc(&p_docu->h_note_list, NOTE_INFO, note_index);

        if(NOTE_SELECTION_NONE == p_note_info->note_selection)
            continue;

        ++selection_count;
    }

    return(selection_count);
}

static void
notelayer_drag_status_set(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_NOTE_INFO p_note_info,
    _InoutRef_  P_NOTE_DRAG_STATUS p_note_drag_status,
    _InRef_opt_ P_CB_DATA_NOTELAYER_DRAG_DATA p_notelayer_drag_data)
{
    SCALE_INFO scale_info;
    DISPLAY_UNIT_INFO display_unit_info;
    PIXIT_POINT pixit_point;
    PIXIT_SIZE scaled_pixit_size;
    GR_SCALE_PAIR gr_scale_pair;

    scale_info_from_docu(p_docu, TRUE, &scale_info);

    display_unit_info_from_display_unit(&display_unit_info, scale_info.display_unit);

    p_note_drag_status->fp_pixits_per_unit = display_unit_info.fp_pixits_per_unit;

    switch(scale_info.display_unit)
    {
    default: default_unhandled();
#if CHECKING
    case DISPLAY_UNIT_CM:
#endif
        p_note_drag_status->drag_message = MSG_STATUS_NOTE_COORDINATES_CM;
        p_note_drag_status->unit_message = MSG_NUMFORM_2_DP;
        break;

    case DISPLAY_UNIT_MM:
        p_note_drag_status->drag_message = MSG_STATUS_NOTE_COORDINATES_MM;
        p_note_drag_status->unit_message = MSG_NUMFORM_1_DP;
        break;

    case DISPLAY_UNIT_INCHES:
        p_note_drag_status->drag_message = MSG_STATUS_NOTE_COORDINATES_INCHES;
        p_note_drag_status->unit_message = MSG_NUMFORM_3_DP;
        break;

    case DISPLAY_UNIT_POINTS:
        p_note_drag_status->drag_message = MSG_STATUS_NOTE_COORDINATES_POINTS;
        p_note_drag_status->unit_message = MSG_NUMFORM_1_DP;
        break;
    }

    if(NULL != p_notelayer_drag_data)
    {
        pixit_point.x = p_notelayer_drag_data->skel_rect_current.tl.pixit_point.x;
        pixit_point.y = p_notelayer_drag_data->skel_rect_current.tl.pixit_point.y;
        scaled_pixit_size.cx = p_notelayer_drag_data->skel_rect_current.br.pixit_point.x - p_notelayer_drag_data->skel_rect_current.tl.pixit_point.x;
        scaled_pixit_size.cy = p_notelayer_drag_data->skel_rect_current.br.pixit_point.y - p_notelayer_drag_data->skel_rect_current.tl.pixit_point.y;
        gr_scale_pair = (p_notelayer_drag_data->reason_code == CB_CODE_NOTELAYER_NOTE_RESCALE)
            ? p_notelayer_drag_data->rescale_gr_scale_pair
            : p_notelayer_drag_data->original_gr_scale_pair;
    }
    else
    {
        pixit_point.x = p_note_info->skel_rect.tl.pixit_point.x;
        pixit_point.y = p_note_info->skel_rect.tl.pixit_point.y;
        scaled_pixit_size.cx = p_note_info->skel_rect.br.pixit_point.x - p_note_info->skel_rect.tl.pixit_point.x;
        scaled_pixit_size.cy = p_note_info->skel_rect.br.pixit_point.y - p_note_info->skel_rect.tl.pixit_point.y;
        gr_scale_pair = p_note_info->gr_scale_pair;
    }

    p_note_drag_status->param[0] = pixit_point.x;
    p_note_drag_status->param[1] = pixit_point.y;
    p_note_drag_status->param[2] = scaled_pixit_size.cx;
    p_note_drag_status->param[3] = scaled_pixit_size.cy;
    p_note_drag_status->param[4] = muldiv64_round_floor(100, gr_scale_pair.x, GR_SCALE_ONE);
    p_note_drag_status->param[5] = muldiv64_round_floor(100, gr_scale_pair.y, GR_SCALE_ONE);
}

static void
notelayer_drag_status_show(
    _DocuRef_   P_DOCU p_docu,
    P_NOTE_DRAG_STATUS p_note_drag_status)
{
    UCHARZ numform_res_quick_ublock_buffer[4][16];
    QUICK_UBLOCK numform_res_quick_ublock[4];
    NUMFORM_PARMS numform_parms;
    STATUS status = STATUS_OK;
    ARRAY_INDEX i;

    zero_struct(numform_parms);
    numform_parms.ustr_numform_numeric = resource_lookup_ustr(p_note_drag_status->unit_message);
    numform_parms.p_numform_context = get_p_numform_context(p_docu);

    for(i = 0; i < 4; i++)
        quick_ublock_setup(&numform_res_quick_ublock[i], numform_res_quick_ublock_buffer[i]);

    for(i = 0; i < 4; i++)
    {
        SS_DATA ss_data;

        ss_data_set_real(&ss_data, (F64) p_note_drag_status->param[i] / p_note_drag_status->fp_pixits_per_unit);

        status_assert(status = numform(&numform_res_quick_ublock[i], P_QUICK_TBLOCK_NONE, &ss_data, &numform_parms));

        status_break(status);
    }

    if(status_ok(status))
    {
        status_line_setf(p_docu, STATUS_LINE_LEVEL_DRAGGING, p_note_drag_status->drag_message,
                         quick_ublock_ustr(&numform_res_quick_ublock[0]),
                         quick_ublock_ustr(&numform_res_quick_ublock[1]),
                         quick_ublock_ustr(&numform_res_quick_ublock[2]),
                         quick_ublock_ustr(&numform_res_quick_ublock[3]),
                         p_note_drag_status->param[4],
                         p_note_drag_status->param[5]);
    }

    for(i = 0; i < 4; i++)
        quick_ublock_dispose(&numform_res_quick_ublock[i]);
}

_Check_return_
static STATUS
notelayer_get_selection_info(
    _DocuRef_   P_DOCU p_docu,
    /*out*/ P_OBJECT_ID p_object_id,
    /*out*/ P_P_ANY p_object_data_ref)
{
    const PC_NOTE_INFO p_note_info = notelayer_selection_first(p_docu);

    if(NULL == p_note_info)
        return(STATUS_FAIL);

    *p_object_id = p_note_info->object_id;
    *p_object_data_ref = p_note_info->object_data_ref;

    return(STATUS_OK);
}

T5_MSG_PROTO(static, note_msg_notelayer_selection_info,  P_NOTELAYER_SELECTION_INFO p_notelayer_selection_info)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    return(notelayer_get_selection_info(p_docu, &p_notelayer_selection_info->object_id, &p_notelayer_selection_info->object_data_ref));
}

/******************************************************************************
*
* Return useful info, so that host menu system can grey out bits of the tree
*
******************************************************************************/

T5_MSG_PROTO(static, note_msg_menu_other_info, _InoutRef_ P_MENU_OTHER_INFO p_menu_other_info)
{
    S32 behind_count = 0;
    S32 picture_count = 0;
    S32 selection_count = 0;
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    while(--note_index >= 0)
    {
        const PC_NOTE_INFO p_note_info = array_ptrc(&p_docu->h_note_list, NOTE_INFO, note_index);

        ++picture_count;

        if(NOTE_SELECTION_NONE != p_note_info->note_selection)
            ++selection_count;

        switch(p_note_info->layer)
        {
        default:
            break;

        case LAYER_PAPER_BELOW:
        case LAYER_PRINT_AREA_BELOW:
        case LAYER_CELLS_AREA_BELOW:
            if(NOTE_UNPINNED == p_note_info->note_pinning)
                ++behind_count;
            break;
        }
    }

    p_menu_other_info->note_behind_count = behind_count;
    p_menu_other_info->note_picture_count = picture_count;
    p_menu_other_info->note_selection_count = selection_count;

    return(STATUS_OK);
}

_Check_return_
static MENU_ROOT_ID
notelayer_menu_query(
    _DocuRef_   P_DOCU p_docu)
{
    MENU_ROOT_ID menu_root_id = MENU_ROOT_DOCU;
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    while(--note_index >= 0)
    {
        const PC_NOTE_INFO p_note_info = array_ptrc(&p_docu->h_note_list, NOTE_INFO, note_index);

        if(NOTE_SELECTION_IN_EDIT != p_note_info->note_selection)
            continue;

        if(OBJECT_ID_CHART == p_note_info->object_id)
        {
            /* eventually do object call */
            menu_root_id = MENU_ROOT_CHART_EDIT;
            break;
        }
    }

    return(menu_root_id);
}

T5_MSG_PROTO(static, note_msg_note_menu_query, P_NOTE_MENU_QUERY p_note_menu_query)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_note_menu_query->menu_root_id = notelayer_menu_query(p_docu);

    return(STATUS_OK);
}

static void
notelayer_replace_note(
    _DocuRef_   P_DOCU p_docu,
    P_NOTE_INFO p_note_info,
    _InVal_     OBJECT_ID object_id,
    P_ANY object_data_ref)
{
    note_update_later(p_docu, p_note_info, NOTE_UPDATE_ALL);

    p_note_info->object_id = object_id;
    p_note_info->object_data_ref = object_data_ref;
}

extern void
notelayer_replace_selection(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id,
    P_ANY object_data)
{
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    while(--note_index >= 0)
    {
        const P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        if(NOTE_SELECTION_SELECTED != p_note_info->note_selection)
            continue;

        notelayer_replace_note(p_docu, p_note_info, object_id, object_data);
    }
}

/******************************************************************************
*
* Return a pointer to the topmost selected note
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_NOTE_INFO
notelayer_selection_first(
    _DocuRef_   P_DOCU p_docu)
{
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    while(--note_index >= 0)
    {
        const P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        if(NOTE_SELECTION_NONE != p_note_info->note_selection)
            return(p_note_info);
    }

    return(NULL);
}

T5_MSG_PROTO(static, note_msg_note_update_object, P_NOTE_UPDATE_OBJECT p_note_update_object)
{
    OBJECT_ID object_id = p_note_update_object->object_id;
    P_ANY object_data_ref = p_note_update_object->object_data_ref;
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    while(--note_index >= 0)
    {
        const P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        if((p_note_info->object_id != object_id) || (p_note_info->object_data_ref != object_data_ref))
            continue;

        note_update_later(p_docu, p_note_info, NOTE_UPDATE_ALL);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, note_msg_note_update_object_info, _InRef_ P_NOTE_UPDATE_OBJECT_INFO p_note_update_object_info)
{
    const P_NOTE_INFO p_note_info = p_note_update_object_info->p_note_info;
    OBJECT_ID object_id = p_note_update_object_info->object_id;
    P_ANY object_data_ref = p_note_update_object_info->object_data_ref;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_note_info->object_id != object_id)
        status_consume(object_call_id(p_note_info->object_id, p_docu, T5_MSG_NOTE_DELETE, p_note_info->object_data_ref));

    p_note_info->object_id = object_id;
    p_note_info->object_data_ref = object_data_ref;

    return(STATUS_OK);
}

_Check_return_
static STATUS
note_msg_caret_show_claim(
    _DocuRef_   P_DOCU p_docu)
{
    /* switch off caret */
    view_show_caret(p_docu, UPDATE_PANE_CELLS_AREA, &p_docu->caret, 0);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, note_msg_mark_info_read, P_MARK_INFO p_mark_info)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_mark_info->h_markers = notelayer_count_selection(p_docu);

    return(STATUS_OK);
}

_Check_return_
static STATUS
note_cmd_delete_character_lr(
    _DocuRef_   P_DOCU p_docu)
{
    /* if a note is being edited then it will be its own focus owner */
    if(notelayer_count_selection(p_docu))
        notelayer_selection_delete(p_docu);

    return(STATUS_OK);
}

_Check_return_
static STATUS
note_cmd_selection_clear(
    _DocuRef_   P_DOCU p_docu)
{
    status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

    caret_show_claim(p_docu, p_docu->focus_owner_old, FALSE); /* ensure focus is not in notelayer */

    return(STATUS_OK);
}

_Check_return_
static STATUS
note_cmd_selection_copy_or_cut(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message)
{
    const BOOL cut_operation = (T5_CMD_SELECTION_CUT == t5_message);
    STATUS status = STATUS_OK;
    S32 count = notelayer_count_selection(p_docu);

    if(0 != count)
    {
        const BOOL clip_data_from_cut_operation = cut_operation;
        ARRAY_HANDLE h_clip_data;
        BOOL f_local_clip_data;

        status_return(clip_data_array_handle_prepare(&h_clip_data, &f_local_clip_data));

        status = notelayer_save_clip_data(p_docu, &h_clip_data);

        if(status_fail(status))
        {
            al_array_dispose(&h_clip_data);
            return(status);
        }

        clip_data_set(p_docu, f_local_clip_data, h_clip_data, clip_data_from_cut_operation, OBJECT_ID_NOTE); /* handle donated */
    }

    if(cut_operation)
        notelayer_selection_delete(p_docu);

    return(status);
}

_Check_return_
static STATUS
note_cmd_selection_delete(
    _DocuRef_   P_DOCU p_docu)
{
    notelayer_selection_delete(p_docu);

    return(STATUS_OK);
}

_Check_return_
static STATUS
note_cmd_paste_at_cursor(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;
    S32 count = notelayer_count_selection(p_docu);
    BOOL f_local_clip_data = FALSE;
    ARRAY_HANDLE h_clip_data = 0;
    ARRAY_HANDLE h_local_clip_data = local_clip_data_query();

    if(0 == h_local_clip_data)
        return(status);

    if(0 != count)
    {
        status_return(clip_data_array_handle_prepare(&h_clip_data, &f_local_clip_data));

        status = notelayer_save_clip_data(p_docu, &h_clip_data);

        if(status_fail(status))
        {
            al_array_dispose(&h_clip_data);
            return(status);
        }

        notelayer_selection_delete(p_docu);
    }

    status_assert(status = load_ownform_from_array_handle(p_docu, &h_local_clip_data, P_POSITION_NONE, FALSE));

    /* force redraw of the pasted notes (if any!) */
    notelayer_view_update_later_full(p_docu);

    if(0 != h_clip_data)
    {   /* replace current clipboard data with the saved data from above */
        clip_data_set(p_docu, f_local_clip_data, h_clip_data, TRUE /*clip_data_from_cut_operation*/, OBJECT_ID_NOTE); /* handle donated */
    }

    return(status);
}

_Check_return_
static STATUS
note_cmd_toggle_marks(
    _DocuRef_   P_DOCU p_docu)
{
    assert(notelayer_count_selection(p_docu));

    status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

    caret_show_claim(p_docu, p_docu->focus_owner_old, FALSE); /* ensure focus is not in notelayer */

    return(STATUS_OK);
}

_Check_return_
static STATUS
note_cmd_snapshot(
    _DocuRef_   P_DOCU p_docu)
{
    NOTE_OBJECT_SNAPSHOT note_object_snapshot;
    P_NOTE_INFO p_note_info = notelayer_selection_first(p_docu);

    if(NULL == p_note_info)
        return(STATUS_OK);

    note_object_snapshot.p_note_info = p_note_info;
    note_object_snapshot.object_data_ref = p_note_info->object_data_ref;

    return(object_call_id(p_note_info->object_id, p_docu, T5_MSG_NOTE_OBJECT_SNAPSHOT, &note_object_snapshot));
}

_Check_return_
static STATUS
note_cmd_force_recalc(
    _DocuRef_   P_DOCU p_docu)
{
    P_NOTE_INFO p_note_info = notelayer_selection_first(p_docu);

    if(NULL == p_note_info)
        return(STATUS_OK);

    return(object_call_id(p_note_info->object_id, p_docu, T5_CMD_FORCE_RECALC, p_note_info->object_data_ref));
}

extern void
notelayer_view_update_later_full(
    _DocuRef_   P_DOCU p_docu)
{
    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;

    skel_rect.tl.page_num.x = skel_rect.tl.page_num.y = 0;
    skel_rect.tl.pixit_point.x = skel_rect.tl.pixit_point.y = 0;
    skel_rect.br = skel_rect.tl;

    RECT_FLAGS_CLEAR(rect_flags);
    rect_flags.extend_right_window = rect_flags.extend_down_window = 1;

    view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);
}

static void
notelayer_note_translate_for_client(
    _DocuRef_   P_DOCU p_docu,
    P_NOTE_OBJECT_CLICK p_note_object_click)
{
    P_NOTE_INFO p_note_info = p_note_object_click->p_note_info;
    P_SKELEVENT_CLICK p_skelevent_click = p_note_object_click->p_skelevent_click;
    SKEL_RECT skel_rect = p_note_info->skel_rect;

    notelayer_drag_data.reason_code = CB_CODE_NOTELAYER_NOTE_TRANSLATE_FOR_CLIENT;

    /* work out the offset to the tl and br points of the box to be dragged from the point where we clicked */

    skel_rect.tl.pixit_point.x -= p_skelevent_click->skel_point.pixit_point.x;
    skel_rect.tl.pixit_point.y -= p_skelevent_click->skel_point.pixit_point.y;
    skel_rect.br.pixit_point.x -= p_skelevent_click->skel_point.pixit_point.x;
    skel_rect.br.pixit_point.y -= p_skelevent_click->skel_point.pixit_point.y;

    skel_rect.tl.pixit_point.x -= p_note_object_click->subselection_pixit_rect.tl.x;
    skel_rect.tl.pixit_point.y -= p_note_object_click->subselection_pixit_rect.tl.y;

    if(!p_note_info->flags.all_pages)
    {
        REDRAW_TAG redraw_tag = redraw_tag_from_layer(p_note_info->layer);

        assert(skel_rect.tl.page_num.x <= p_skelevent_click->skel_point.page_num.x);
        while(skel_rect.tl.page_num.x < p_skelevent_click->skel_point.page_num.x)
        {
            PIXIT_POINT work_area;
            page_limits_from_page(p_docu, &work_area, redraw_tag, &skel_rect.tl.page_num);
            skel_rect.tl.pixit_point.x -= work_area.x;
            skel_rect.tl.page_num.x += 1;
        }

        assert(skel_rect.tl.page_num.y <= p_skelevent_click->skel_point.page_num.y);
        while(skel_rect.tl.page_num.y < p_skelevent_click->skel_point.page_num.y)
        {
            PIXIT_POINT work_area;
            page_limits_from_page(p_docu, &work_area, redraw_tag, &skel_rect.tl.page_num);
            skel_rect.tl.pixit_point.y -= work_area.y;
            skel_rect.tl.page_num.y += 1;
        }

        assert(skel_rect.br.page_num.x >= p_skelevent_click->skel_point.page_num.x);
        while(skel_rect.br.page_num.x > p_skelevent_click->skel_point.page_num.x)
        {
            PIXIT_POINT work_area;
            skel_rect.br.page_num.x -= 1;
            page_limits_from_page(p_docu, &work_area, redraw_tag, &skel_rect.br.page_num);
            skel_rect.br.pixit_point.x += work_area.x;
        }

        assert(skel_rect.br.page_num.y >= p_skelevent_click->skel_point.page_num.y);
        while(skel_rect.br.page_num.y > p_skelevent_click->skel_point.page_num.y)
        {
            PIXIT_POINT work_area;
            skel_rect.br.page_num.y -= 1;
            page_limits_from_page(p_docu, &work_area, redraw_tag, &skel_rect.br.page_num);
            skel_rect.br.pixit_point.y += work_area.y;
        }
    }

    notelayer_drag_data.translate_box_pixit_rect_offset.tl = skel_rect.tl.pixit_point;
    notelayer_drag_data.translate_box_pixit_rect_offset.br = skel_rect.br.pixit_point;

    host_drag_start(&notelayer_drag_data);
}

#if CHECKING

_Check_return_
static STATUS
notelayer_click_trap(void)
{
    return(STATUS_OK);
}

#endif

_Check_return_
static ARRAY_INDEX
notelayer_click_find_note(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     LAYER layer,
    _OutRef_    P_NOTE_POSITION p_note_position)
{
    NOTE_POSITION note_position = PIXIT_RECT_EAR_NONE;
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    while(--note_index >= 0)
    {
        const PC_NOTE_INFO p_note_info = array_ptrc(&p_docu->h_note_list, NOTE_INFO, note_index);
        PIXIT_RECT pixit_rect;

        if(p_note_info->layer != layer)
            continue;

        if(!p_note_info->flags.skel_rect_valid)
        {   /* bbox will be set if we can see it, and if we can't see it,
             * we certainly aren't in any position to be clicking on it
            */
            continue;
        }

        relative_pixit_rect_from_note(p_docu, p_note_info, &p_skelevent_click->skel_point.page_num, &pixit_rect);

        switch(p_note_info->note_selection)
        {
        default: default_unhandled();
#if CHECKING
        case NOTE_SELECTION_NONE:
#endif
            /* make thin horizontal/vertical lines easier to click on */
            expand_pixit_rect(&pixit_rect, &p_skelevent_click->click_context.one_program_pixel);

            if(!POINT_INSIDE_RECTANGLE(p_skelevent_click->skel_point.pixit_point, pixit_rect))
                continue;

            note_position = PIXIT_RECT_EAR_CENTRE;
            break;

        case NOTE_SELECTION_SELECTED:
        case NOTE_SELECTION_IN_EDIT:
            {
            NOTE_OBJECT_BOUNDS note_object_bounds;

            notelayer_object_bounds(&note_object_bounds, p_note_info->note_pinning, &pixit_rect, &p_skelevent_click->click_context.one_program_pixel);

            if(!POINT_INSIDE_RECTANGLE(p_skelevent_click->skel_point.pixit_point, note_object_bounds.ears.outer_bound))
                continue;

            note_position = PIXIT_RECT_EAR_COUNT;

            while(--note_position >= 0)
            {
                if(!note_object_bounds.ears.ear_active[note_position])
                    continue;

                if(!POINT_INSIDE_RECTANGLE(p_skelevent_click->skel_point.pixit_point, note_object_bounds.ears.ear[note_position]))
                    continue;

                break;
            }

            if(note_position < 0)
                note_position = PIXIT_RECT_EAR_CENTRE; /* even if missed everything else! */

            break;
            }
        }

        assert(PIXIT_RECT_EAR_NONE != note_position);
        if(PIXIT_RECT_EAR_NONE != note_position)
        {
            *p_note_position = note_position;
            return(note_index);
        }
    }

    *p_note_position = PIXIT_RECT_EAR_NONE;
    return(-1);
}

_Check_return_
static STATUS
notelayer_click_ear_start_drag(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_SKELEVENT_CLICK p_skelevent_click,
    _InoutRef_  P_NOTE_INFO p_note_info,
    _InVal_     NOTE_POSITION note_position)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DRAG;
    const T5_MESSAGE t5_message_effective = right_message_if_ctrl(t5_message, t5_message_right, p_skelevent_click);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(p_note_info->flags.scale_to_fit)
        return(create_error(ERR_CANT_EDIT_SCALED_TO_FIT_NOTE));

    notelayer_drag_data.resize = notelayer_resize[note_position];
    notelayer_drag_data.resize_tie_xy = 0;

    switch(t5_message_effective)
    {
    case T5_EVENT_CLICK_LEFT_DRAG:
        notelayer_drag_data.reason_code = CB_CODE_NOTELAYER_NOTE_RESIZE;
        if((note_position == PIXIT_RECT_EAR_TL) || (note_position == PIXIT_RECT_EAR_BR))
            notelayer_drag_data.resize_tie_xy = 1;
        break;

    default: default_unhandled();
    case T5_EVENT_CLICK_RIGHT_DRAG:
        notelayer_drag_data.reason_code = CB_CODE_NOTELAYER_NOTE_RESCALE;
        break;
    }

    host_drag_start(&notelayer_drag_data);

    return(STATUS_OK);
}

_Check_return_
static STATUS
notelayer_click_ear_TL(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_SKELEVENT_CLICK p_skelevent_click,
    _InoutRef_  P_NOTE_INFO p_note_info,
    _InVal_     NOTE_POSITION note_position)
{
    switch(t5_message)
    {
    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        return(notelayer_click_ear_start_drag(p_docu, t5_message, p_skelevent_click, p_note_info, note_position));

    case T5_EVENT_CLICK_LEFT_DOUBLE:
        {
        if(p_note_info->flags.scale_to_fit)
            return(create_error(ERR_CANT_EDIT_SCALED_TO_FIT_NOTE));

        note_pin_change(p_docu, p_note_info, (NOTE_UNPINNED == p_note_info->note_pinning) ? NOTE_PIN_CELLS_SINGLE : NOTE_UNPINNED);
        return(STATUS_OK);
        }

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
notelayer_click_ear_BR(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_SKELEVENT_CLICK p_skelevent_click,
    _InoutRef_  P_NOTE_INFO p_note_info,
    _InVal_     NOTE_POSITION note_position)
{
    switch(t5_message)
    {
    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        return(notelayer_click_ear_start_drag(p_docu, t5_message, p_skelevent_click, p_note_info, note_position));

    case T5_EVENT_CLICK_LEFT_DOUBLE:
        {
        if(p_note_info->flags.scale_to_fit)
            return(create_error(ERR_CANT_EDIT_SCALED_TO_FIT_NOTE));

        note_pin_change(p_docu, p_note_info, (NOTE_PIN_CELLS_SINGLE == p_note_info->note_pinning) ? NOTE_PIN_CELLS_TWIN : NOTE_PIN_CELLS_SINGLE);
        return(STATUS_OK);
        }

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
notelayer_click_ear_other(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_SKELEVENT_CLICK p_skelevent_click,
    _InoutRef_  P_NOTE_INFO p_note_info,
    _InVal_     NOTE_POSITION note_position)
{
    switch(t5_message)
    {
    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        return(notelayer_click_ear_start_drag(p_docu, t5_message, p_skelevent_click, p_note_info, note_position));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
notelayer_click_ear_CENTRE_IN_EDIT(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_SKELEVENT_CLICK p_skelevent_click,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    NOTE_OBJECT_CLICK note_object_click;
    note_object_click.object_data_ref = p_note_info->object_data_ref;
    note_object_click.t5_message = t5_message;
    note_object_click.pixit_point.x = p_skelevent_click->skel_point.pixit_point.x - p_note_info->skel_rect.tl.pixit_point.x;
    note_object_click.pixit_point.y = p_skelevent_click->skel_point.pixit_point.y - p_note_info->skel_rect.tl.pixit_point.y;
    note_object_click.pixit_point.x = gr_coord_scale_inverse(note_object_click.pixit_point.x, p_note_info->gr_scale_pair.x);
    note_object_click.pixit_point.y = gr_coord_scale_inverse(note_object_click.pixit_point.y, p_note_info->gr_scale_pair.y);
    note_object_click.p_note_info = p_note_info;
    note_object_click.p_skelevent_click = p_skelevent_click;
    note_object_click.processed = 0;
    status_consume(object_call_id(p_note_info->object_id, p_docu, T5_MSG_NOTE_OBJECT_CLICK, &note_object_click));
    if(1 == note_object_click.processed)
        return(STATUS_DONE);
    if(CB_CODE_NOTELAYER_NOTE_TRANSLATE_FOR_CLIENT == note_object_click.processed)
    {
        notelayer_note_translate_for_client(p_docu, &note_object_click);
        return(STATUS_DONE);
    }
    return(STATUS_OK);
}

_Check_return_
static STATUS
notelayer_click_ear_CENTRE_single(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_SKELEVENT_CLICK p_skelevent_click,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER(p_skelevent_click);

    if(NOTE_SELECTION_SELECTED == p_note_info->note_selection)
    {
        /* 'select' a selected item - leave selection intact */ /*EMPTY*/
    }
    else
    {
        /* 'select' an unselected item - clear selection (either in note layer or elsewhere) and select this item */
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

        note_select(p_docu, p_note_info);
    }

    return(STATUS_OK);
}

#if 0 /* goodbye multiple selection */

_Check_return_
static STATUS
notelayer_click_ear_CENTRE_right_single(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_SKELEVENT_CLICK p_skelevent_click,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    if(NOTE_SELECTION_SELECTED == p_note_info->note_selection)
    {
        /* 'adjust' a selected item - remove it from the selection */
        note_update_later(p_docu, p_note_info, NOTE_UPDATE_SELECTION_MARKS);

        p_note_info->note_selection = NOTE_SELECTION_NONE;

        /* tell everyone that selection has changed */
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_NEW, P_DATA_NONE));

        if(!notelayer_selection_first(p_docu))
            caret_show_claim(p_docu, p_docu->focus_owner_old, FALSE); /* ensure focus is not in notelayer */
    }
    else
    {
        /* 'adjust' an unselected item - add it to the selection */
        P_NOTE_INFO sel_p_note_info = notelayer_selection_first(p_docu);

        /* however, if the current selection is an edited note, drop that first */
        if(NOTE_SELECTION_IN_EDIT == sel_p_note_info->note_selection)
            status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

        note_select(p_docu, p_note_info);
    }
}

#endif

_Check_return_
static STATUS
notelayer_click_ear_CENTRE_left_double(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_SKELEVENT_CLICK p_skelevent_click,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER(p_skelevent_click);

    { /* if the current selection contains any other, drop them first */
    STATUS clear_sel = 0;
    STATUS sel_changed = 0;
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    while(--note_index >= 0)
    {
        const P_NOTE_INFO this_p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        switch(this_p_note_info->note_selection)
        {
        default: default_unhandled();
        case NOTE_SELECTION_NONE:
            break;

        case NOTE_SELECTION_SELECTED:
            if(this_p_note_info != p_note_info)
                sel_changed = 1;
            break;

        case NOTE_SELECTION_IN_EDIT:
            clear_sel = 1;
            break;
        }
    }

    if(clear_sel || sel_changed)
        status_assert(maeve_event(p_docu, clear_sel ? T5_MSG_SELECTION_CLEAR : T5_MSG_SELECTION_NEW, P_DATA_NONE));
    } /*block*/

    { /* start an edit for the object */
    NOTE_OBJECT_EDIT_START note_object_edit_start;
    note_object_edit_start.processed = 0;
    note_object_edit_start.object_data_ref = p_note_info->object_data_ref;
    note_object_edit_start.p_note_info = p_note_info;
    status_consume(object_call_id(p_note_info->object_id, p_docu, T5_MSG_NOTE_OBJECT_EDIT_START, &note_object_edit_start));
    if(!note_object_edit_start.processed)
        return(STATUS_OK);
    } /*block*/

    p_note_info->note_selection = NOTE_SELECTION_IN_EDIT;

    note_update_later(p_docu, p_note_info, NOTE_UPDATE_SELECTION_MARKS);

    p_docu->focus_owner_old = p_docu->focus_owner;

    /* give the bugger the input focus too */
    caret_show_claim(p_docu, p_note_info->object_id, FALSE);

    return(STATUS_OK);
}

_Check_return_
static STATUS
notelayer_click_ear_CENTRE_drag(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_SKELEVENT_CLICK p_skelevent_click,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* T5_EVENT_CLICK_RIGHT_DRAG, select note incase earlier T5_EVENT_CLICK_RIGHT_SINGLE deselected it */
    /* T5_EVENT_CLICK_LEFT_DRAG, note should (on RISCOS maybe?) already be selected, but making sure won't hurt */
    if(NOTE_SELECTION_NONE == p_note_info->note_selection)
        note_select(p_docu, p_note_info);

    notelayer_drag_data.reason_code = CB_CODE_NOTELAYER_NOTE_TRANSLATE;

    { /* work out the offset to the tl and br points of the box to be dragged from the point where we clicked */
    SKEL_RECT skel_rect = p_note_info->skel_rect;

    skel_rect.tl.pixit_point.x -= p_skelevent_click->skel_point.pixit_point.x;
    skel_rect.tl.pixit_point.y -= p_skelevent_click->skel_point.pixit_point.y;
    skel_rect.br.pixit_point.x -= p_skelevent_click->skel_point.pixit_point.x;
    skel_rect.br.pixit_point.y -= p_skelevent_click->skel_point.pixit_point.y;

    if(!p_note_info->flags.all_pages)
    {
        REDRAW_TAG redraw_tag = redraw_tag_from_layer(p_note_info->layer);

        assert(skel_rect.tl.page_num.x <= p_skelevent_click->skel_point.page_num.x);
        while(skel_rect.tl.page_num.x < p_skelevent_click->skel_point.page_num.x)
        {
            PIXIT_POINT work_area;
            page_limits_from_page(p_docu, &work_area, redraw_tag, &skel_rect.tl.page_num);
            skel_rect.tl.pixit_point.x -= work_area.x;
            skel_rect.tl.page_num.x += 1;
        }

        assert(skel_rect.tl.page_num.y <= p_skelevent_click->skel_point.page_num.y);
        while(skel_rect.tl.page_num.y < p_skelevent_click->skel_point.page_num.y)
        {
            PIXIT_POINT work_area;
            page_limits_from_page(p_docu, &work_area, redraw_tag, &skel_rect.tl.page_num);
            skel_rect.tl.pixit_point.y -= work_area.y;
            skel_rect.tl.page_num.y += 1;
        }

        assert(skel_rect.br.page_num.x >= p_skelevent_click->skel_point.page_num.x);
        while(skel_rect.br.page_num.x > p_skelevent_click->skel_point.page_num.x)
        {
            PIXIT_POINT work_area;
            skel_rect.br.page_num.x -= 1;
            page_limits_from_page(p_docu, &work_area, redraw_tag, &skel_rect.br.page_num);
            skel_rect.br.pixit_point.x += work_area.x;
        }

        assert(skel_rect.br.page_num.y >= p_skelevent_click->skel_point.page_num.y);
        while(skel_rect.br.page_num.y > p_skelevent_click->skel_point.page_num.y)
        {
            PIXIT_POINT work_area;
            skel_rect.br.page_num.y -= 1;
            page_limits_from_page(p_docu, &work_area, redraw_tag, &skel_rect.br.page_num);
            skel_rect.br.pixit_point.y += work_area.y;
        }
    }

    notelayer_drag_data.translate_box_pixit_rect_offset.tl = skel_rect.tl.pixit_point;
    notelayer_drag_data.translate_box_pixit_rect_offset.br = skel_rect.br.pixit_point;
    } /*block*/

    host_drag_start(&notelayer_drag_data);

    return(STATUS_OK);
}

_Check_return_
static STATUS
notelayer_click_ear_CENTRE(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_SKELEVENT_CLICK p_skelevent_click,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    if(NOTE_SELECTION_IN_EDIT == p_note_info->note_selection)
    {
        if(status_done(notelayer_click_ear_CENTRE_IN_EDIT(p_docu, t5_message, p_skelevent_click, p_note_info)))
            return(STATUS_OK);
    }

    switch(t5_message)
    {
    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
        return(notelayer_click_ear_CENTRE_single(p_docu, t5_message, p_skelevent_click, p_note_info));

    case T5_EVENT_CLICK_LEFT_DOUBLE:
        return(notelayer_click_ear_CENTRE_left_double(p_docu, t5_message, p_skelevent_click, p_note_info));

    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        return(notelayer_click_ear_CENTRE_drag(p_docu, t5_message, p_skelevent_click, p_note_info));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
notelayer_click(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     LAYER layer)
{
    NOTE_POSITION note_position;
    ARRAY_INDEX note_index;
    P_NOTE_INFO p_note_info;

#if CHECKING && 1
    /* make trapping certain click types easier */
    switch(t5_message)
    {
    case T5_EVENT_CLICK_LEFT_DRAG:
        status_return(notelayer_click_trap());
        break;

    default:
        break;
    }
#endif

    note_index = notelayer_click_find_note(p_docu, p_skelevent_click, layer, &note_position);

    if(note_index < 0)
        return(STATUS_OK);

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_APP_CLICK))
        switch(note_position)
        {
        case PIXIT_RECT_EAR_CENTRE: trace_0(TRACE_APP_CLICK, TEXT("over note centre"));    break;
        case PIXIT_RECT_EAR_TC: trace_0(TRACE_APP_CLICK, TEXT("over note top centre"));    break;
        case PIXIT_RECT_EAR_LC: trace_0(TRACE_APP_CLICK, TEXT("over note left centre"));   break;
        case PIXIT_RECT_EAR_RC: trace_0(TRACE_APP_CLICK, TEXT("over note right centre"));  break;
        case PIXIT_RECT_EAR_BC: trace_0(TRACE_APP_CLICK, TEXT("over note bottom centre")); break;
        case PIXIT_RECT_EAR_TR: trace_0(TRACE_APP_CLICK, TEXT("over note top right"));     break;
        case PIXIT_RECT_EAR_BL: trace_0(TRACE_APP_CLICK, TEXT("over note bottom left"));   break;
        case PIXIT_RECT_EAR_TL: trace_0(TRACE_APP_CLICK, TEXT("over note top left"));      break;
        case PIXIT_RECT_EAR_BR: trace_0(TRACE_APP_CLICK, TEXT("over note bottom right"));  break;
        }
#endif

    p_skelevent_click->processed = 1;

    p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

    switch(note_position)
    {
    case PIXIT_RECT_EAR_NONE:
        assert0();
        return(STATUS_OK);

    case PIXIT_RECT_EAR_CENTRE:
        return(notelayer_click_ear_CENTRE(p_docu, t5_message, p_skelevent_click, p_note_info));

    case PIXIT_RECT_EAR_TL:
        return(notelayer_click_ear_TL(p_docu, t5_message, p_skelevent_click, p_note_info, note_position));

    case PIXIT_RECT_EAR_BR:
        return(notelayer_click_ear_BR(p_docu, t5_message, p_skelevent_click, p_note_info, note_position));

    default: default_unhandled();
#if CHECKING
    case PIXIT_RECT_EAR_TC:
    case PIXIT_RECT_EAR_LC:
    case PIXIT_RECT_EAR_RC:
    case PIXIT_RECT_EAR_BC:
    case PIXIT_RECT_EAR_TR:
    case PIXIT_RECT_EAR_BL:
#endif
        return(notelayer_click_ear_other(p_docu, t5_message, p_skelevent_click, p_note_info, note_position));
    }
}

static void
notelayer_drag_start_setup(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click,
    _InRef_     P_CB_DATA_NOTELAYER_DRAG_DATA p_notelayer_drag_data,
    _InRef_     P_NOTE_INFO p_note_info)
{
    p_notelayer_drag_data->original_pixit_size = p_note_info->pixit_size;
    p_notelayer_drag_data->original_gr_scale_pair = p_note_info->gr_scale_pair;
    p_notelayer_drag_data->skel_point.page_num.x = -1; /* zot this to get page related stuff cached below */

    switch(p_notelayer_drag_data->reason_code)
    {
    default: default_unhandled();
#if CHECKING
    case CB_CODE_NOTELAYER_NOTE_TRANSLATE:
    case CB_CODE_NOTELAYER_NOTE_TRANSLATE_FOR_CLIENT:
#endif
        break;

    case CB_CODE_NOTELAYER_NOTE_RESCALE:
        p_notelayer_drag_data->resize_first_skel_point = p_skelevent_click->skel_point;
        p_notelayer_drag_data->skel_rect_current = p_note_info->skel_rect;
        p_notelayer_drag_data->resize_original_skel_rect = p_note_info->skel_rect;
        break;

    case CB_CODE_NOTELAYER_NOTE_RESIZE:
        {
        NOTE_OBJECT_SIZE note_object_size;
        note_object_size.object_data_ref = p_note_info->object_data_ref;
        note_object_size.processed = 0;
        status_consume(object_call_id(p_note_info->object_id, p_docu, T5_MSG_NOTE_OBJECT_SIZE_SET_POSSIBLE, &note_object_size));
        if(!note_object_size.processed)
            p_notelayer_drag_data->reason_code = CB_CODE_NOTELAYER_NOTE_RESCALE;

        p_notelayer_drag_data->resize_first_skel_point = p_skelevent_click->skel_point;
        p_notelayer_drag_data->skel_rect_current = p_note_info->skel_rect;
        p_notelayer_drag_data->resize_original_skel_rect = p_note_info->skel_rect;
        /* don't bother initialising resize_current_size */
        break;
        }
    }
}

_Check_return_
static STATUS
notelayer_drag_started_or_movement(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     LAYER layer)
{
    const P_CB_DATA_NOTELAYER_DRAG_DATA p_notelayer_drag_data = (P_CB_DATA_NOTELAYER_DRAG_DATA) p_skelevent_click->data.drag.p_reason_data;
    STATUS status = STATUS_OK;
    P_NOTE_INFO p_note_info = notelayer_selection_first(p_docu);
    const BOOL ctrl_pressed = p_skelevent_click->click_context.ctrl_pressed;
    const BOOL do_snap = !ctrl_pressed; /* Ctrl allows finer placement */
    PTR_ASSERT(p_note_info);

    trace_0(TRACE_APP_CLICK, TEXT("proc_event_note T5_EVENT_CLICK_DRAG_MOVEMENT"));

    if(T5_EVENT_CLICK_DRAG_STARTED == t5_message)
        notelayer_drag_start_setup(p_docu, p_skelevent_click, p_notelayer_drag_data, p_note_info);

    if( (p_notelayer_drag_data->skel_point.page_num.x != p_skelevent_click->skel_point.page_num.x) ||
        (p_notelayer_drag_data->skel_point.page_num.y != p_skelevent_click->skel_point.page_num.y) )
    {
        /* may be tempted to cache page related stuff here */ /*EMPTY*/
    }

    p_notelayer_drag_data->skel_point = p_skelevent_click->skel_point;

    { /* work out changes to box (if any) */
    SKEL_POINT skel_point = p_skelevent_click->skel_point;
    SKEL_RECT skel_rect = p_notelayer_drag_data->skel_rect_current;
    BOOL moved = FALSE;

    switch(p_notelayer_drag_data->reason_code)
    {
    default: default_unhandled();
#if CHECKING
    case CB_CODE_NOTELAYER_NOTE_TRANSLATE:
    case CB_CODE_NOTELAYER_NOTE_TRANSLATE_FOR_CLIENT:
#endif
        {
        PIXIT_SIZE pixit_size;

        pixit_size.cx = p_notelayer_drag_data->translate_box_pixit_rect_offset.br.x - p_notelayer_drag_data->translate_box_pixit_rect_offset.tl.x;
        pixit_size.cy = p_notelayer_drag_data->translate_box_pixit_rect_offset.br.y - p_notelayer_drag_data->translate_box_pixit_rect_offset.tl.y;

        skel_rect.tl = p_notelayer_drag_data->skel_point;

        skel_rect.tl.pixit_point.x += p_notelayer_drag_data->translate_box_pixit_rect_offset.tl.x;
        skel_rect.tl.pixit_point.y += p_notelayer_drag_data->translate_box_pixit_rect_offset.tl.y;

        if(do_snap)
        {   /* top and left edges of note both snap to click stop during translate op */
            skel_rect.tl.pixit_point.x = skel_ruler_snap_to_click_stop(p_docu, 1, skel_rect.tl.pixit_point.x, SNAP_TO_CLICK_STOP_FLOOR);
            skel_rect.tl.pixit_point.y = skel_ruler_snap_to_click_stop(p_docu, 0, skel_rect.tl.pixit_point.y, SNAP_TO_CLICK_STOP_FLOOR);
        }

        /* but keep the note's existing size */
        skel_rect.br = skel_rect.tl;

        skel_rect.br.pixit_point.x += pixit_size.cx;
        skel_rect.br.pixit_point.y += pixit_size.cy;

        break;
        }

    case CB_CODE_NOTELAYER_NOTE_RESCALE:
    case CB_CODE_NOTELAYER_NOTE_RESIZE:
        {
        CB_DATA_NOTELAYER_ADJUST resize = p_notelayer_drag_data->resize;
        PIXIT_RECT pixit_rect;
        PIXIT_SIZE pixit_size;

        if(resize.tl_x)
        {
            skel_rect.tl.page_num.x = skel_point.page_num.x;
            skel_rect.tl.pixit_point.x = skel_point.pixit_point.x;
        }

        if(resize.tl_y)
        {
            skel_rect.tl.page_num.y = skel_point.page_num.y;
            skel_rect.tl.pixit_point.y = skel_point.pixit_point.y;
        }

        if(resize.br_x)
        {
            skel_rect.br.page_num.x = skel_point.page_num.x;
            skel_rect.br.pixit_point.x = skel_point.pixit_point.x;
        }

        if(resize.br_y)
        {
            skel_rect.br.page_num.y = skel_point.page_num.y;
            skel_rect.br.pixit_point.y = skel_point.pixit_point.y;
        }

        relative_pixit_point_from_skel_point_in_layer(p_docu, &skel_point.page_num, &skel_rect.tl, &pixit_rect.tl, layer);
        relative_pixit_point_from_skel_point_in_layer(p_docu, &skel_point.page_num, &skel_rect.br, &pixit_rect.br, layer);

        pixit_size.cx = pixit_rect_width(&pixit_rect);
        pixit_size.cy = pixit_rect_height(&pixit_rect);

        if(do_snap)
        {
            if(resize.tl_x) /* snap tl_x? */
            {
                PIXIT new_x = skel_ruler_snap_to_click_stop(p_docu, 1, skel_rect.tl.pixit_point.x, SNAP_TO_CLICK_STOP_FLOOR);
                PIXIT delta_x = new_x - skel_rect.tl.pixit_point.x;
                if(0 != delta_x)
                {
                    PIXIT new_width = pixit_size.cx - delta_x; /* +ve delta_x at tl shrinks object */
                    skel_rect.tl.pixit_point.x = new_x;
                    pixit_size.cx = new_width;
                }
            }

            if(resize.tl_y) /* snap tl_y? */
            {
                PIXIT new_y = skel_ruler_snap_to_click_stop(p_docu, 0, skel_rect.tl.pixit_point.y, SNAP_TO_CLICK_STOP_FLOOR);
                PIXIT delta_y = new_y - skel_rect.tl.pixit_point.y;
                if(0 != delta_y)
                {
                    PIXIT new_height = pixit_size.cy - delta_y; /* +ve delta_y at tl shrinks object */
                    skel_rect.tl.pixit_point.y = new_y;
                    pixit_size.cy = new_height;
                }
            }

            if(resize.br_x) /* snap br_x? */
            {
                PIXIT new_x = skel_ruler_snap_to_click_stop(p_docu, 1, skel_rect.br.pixit_point.x, SNAP_TO_CLICK_STOP_ROUND);
                PIXIT delta_x = new_x - skel_rect.br.pixit_point.x;
                if(0 != delta_x)
                {
                    PIXIT new_width = pixit_size.cx + delta_x; /* +ve delta_x at br grows object */
                    skel_rect.br.pixit_point.x = new_x;
                    pixit_size.cx = new_width;
                }
            }

            if(resize.br_y) /* snap br_y? */
            {
                PIXIT new_y = skel_ruler_snap_to_click_stop(p_docu, 0, skel_rect.br.pixit_point.y, SNAP_TO_CLICK_STOP_ROUND);
                PIXIT delta_y = new_y - skel_rect.br.pixit_point.y;
                if(0 != delta_y)
                {
                    PIXIT new_height = pixit_size.cy + delta_y; /* +ve delta_y at br grows object */
                    skel_rect.br.pixit_point.y = new_y;
                    pixit_size.cy = new_height;
                }
            }
        }

        p_notelayer_drag_data->rescale_gr_scale_pair.x = gr_scale_from_s32_pair(pixit_size.cx, p_notelayer_drag_data->original_pixit_size.cx);
        p_notelayer_drag_data->rescale_gr_scale_pair.y = gr_scale_from_s32_pair(pixit_size.cy, p_notelayer_drag_data->original_pixit_size.cy);

        if(p_notelayer_drag_data->resize_tie_xy)
        {
            GR_SCALE_PAIR ratio_gr_scale_pair;
            GR_SCALE gr_scale;

            ratio_gr_scale_pair.x = muldiv64(p_notelayer_drag_data->rescale_gr_scale_pair.x, GR_SCALE_ONE, p_notelayer_drag_data->original_gr_scale_pair.x);
            ratio_gr_scale_pair.y = muldiv64(p_notelayer_drag_data->rescale_gr_scale_pair.y, GR_SCALE_ONE, p_notelayer_drag_data->original_gr_scale_pair.y);

            if((ratio_gr_scale_pair.x >= GR_SCALE_ONE) && (ratio_gr_scale_pair.y >= GR_SCALE_ONE))
                gr_scale = MIN(ratio_gr_scale_pair.x, ratio_gr_scale_pair.y);
            else if((ratio_gr_scale_pair.x < GR_SCALE_ONE) && (ratio_gr_scale_pair.y < GR_SCALE_ONE))
                gr_scale = MAX(ratio_gr_scale_pair.x, ratio_gr_scale_pair.y);
            else
                gr_scale = GR_SCALE_ONE;

            p_notelayer_drag_data->rescale_gr_scale_pair.x = muldiv64(gr_scale, p_notelayer_drag_data->original_gr_scale_pair.x, GR_SCALE_ONE);
            p_notelayer_drag_data->rescale_gr_scale_pair.y = muldiv64(gr_scale, p_notelayer_drag_data->original_gr_scale_pair.y, GR_SCALE_ONE);

            pixit_size.cx = muldiv64(p_notelayer_drag_data->original_pixit_size.cx, p_notelayer_drag_data->rescale_gr_scale_pair.x, GR_SCALE_ONE);
            pixit_size.cy = muldiv64(p_notelayer_drag_data->original_pixit_size.cy, p_notelayer_drag_data->rescale_gr_scale_pair.y, GR_SCALE_ONE);

            if(resize.tl_x)
                skel_rect.tl.pixit_point.x = pixit_rect.br.x - pixit_size.cx;

            if(resize.tl_y)
                skel_rect.tl.pixit_point.y = pixit_rect.br.y - pixit_size.cy;

            if(resize.br_x)
                skel_rect.br.pixit_point.x = pixit_rect.tl.x + pixit_size.cx;

            if(resize.br_y)
                skel_rect.br.pixit_point.y = pixit_rect.tl.y + pixit_size.cy;
        }

        p_notelayer_drag_data->resize_current_pixit_size = pixit_size;

        break;
        }
    }

    moved = FALSE;

    if(T5_EVENT_CLICK_DRAG_STARTED == t5_message)
    {
        moved = TRUE;
    }
    else /*(T5_EVENT_CLICK_DRAG_MOVEMENT == t5_message)*/
    {
        if(0 != memcmp32(&skel_rect, &p_notelayer_drag_data->skel_rect_current, sizeof32(skel_rect)))
        {
            /* remove the grey boxes */
            notelayer_invert_drag_boxes(p_docu, layer);

            moved = TRUE;
        }
    }

    if(!moved)
        return(status);

    p_notelayer_drag_data->skel_rect_current = skel_rect;
    } /*block*/

    /* (re)paint the grey boxes */
    notelayer_invert_drag_boxes(p_docu, layer);

    notelayer_drag_status_set(p_docu, p_note_info, &p_notelayer_drag_data->note_drag_status, p_notelayer_drag_data);

    notelayer_drag_status_show(p_docu, &p_notelayer_drag_data->note_drag_status);

    return(status);
}

_Check_return_
static STATUS
notelayer_drag_finished_or_aborted(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     LAYER layer)
{
    const P_CB_DATA_NOTELAYER_DRAG_DATA p_notelayer_drag_data = (P_CB_DATA_NOTELAYER_DRAG_DATA) p_skelevent_click->data.drag.p_reason_data;
    STATUS status = STATUS_OK;
    P_NOTE_INFO p_note_info = notelayer_selection_first(p_docu);
    PTR_ASSERT(p_note_info);

    /* remove the grey boxes */
    notelayer_invert_drag_boxes(p_docu, layer);

    status_line_clear(p_docu, STATUS_LINE_LEVEL_DRAGGING);

    if(T5_EVENT_CLICK_DRAG_FINISHED == t5_message)
    {
        switch(p_notelayer_drag_data->reason_code)
        {
        case CB_CODE_NOTELAYER_NOTE_TRANSLATE_FOR_CLIENT:
            break;

        default: default_unhandled();
#if CHECKING
        case CB_CODE_NOTELAYER_NOTE_TRANSLATE:
#endif
            {
            const SKEL_POINT skel_point = p_notelayer_drag_data->skel_rect_current.tl;
            const PIXIT_SIZE pixit_size = p_notelayer_drag_data->original_pixit_size;
            PTR_ASSERT(p_note_info);
            note_move(p_docu, p_note_info, &skel_point, &pixit_size);
            break;
            }

        case CB_CODE_NOTELAYER_NOTE_RESIZE:
            {
            const SKEL_POINT skel_point = p_notelayer_drag_data->skel_rect_current.tl;
            NOTE_OBJECT_SIZE note_object_size;
            note_object_size.object_data_ref = p_note_info->object_data_ref;
            note_object_size.pixit_size = p_notelayer_drag_data->resize_current_pixit_size;
            note_object_size.pixit_size.cx = muldiv64(note_object_size.pixit_size.cx, GR_SCALE_ONE, p_notelayer_drag_data->original_gr_scale_pair.x);
            note_object_size.pixit_size.cy = muldiv64(note_object_size.pixit_size.cy, GR_SCALE_ONE, p_notelayer_drag_data->original_gr_scale_pair.y);
            note_object_size.processed = 0;
            status_consume(object_call_id(p_note_info->object_id, p_docu, T5_MSG_NOTE_OBJECT_SIZE_SET, &note_object_size));
            assert(note_object_size.processed);
            if(note_object_size.processed)
            {
                p_note_info->pixit_size = note_object_size.pixit_size;
                note_move(p_docu, p_note_info, &skel_point, &note_object_size.pixit_size);
            }
            break;
            }

        case CB_CODE_NOTELAYER_NOTE_RESCALE:
            {
            const SKEL_POINT skel_point = p_notelayer_drag_data->skel_rect_current.tl;
            const PIXIT_SIZE pixit_size = p_notelayer_drag_data->original_pixit_size;
            p_note_info->gr_scale_pair = p_notelayer_drag_data->rescale_gr_scale_pair;
            note_move(p_docu, p_note_info, &skel_point, &pixit_size);
            break;
            }
        }

        docu_modify(p_docu);
    }

    return(status);
}

_Check_return_
static STATUS
notelayer_drag(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     LAYER layer)
{
    const P_CB_DATA_NOTELAYER_DRAG_DATA p_notelayer_drag_data = (P_CB_DATA_NOTELAYER_DRAG_DATA) p_skelevent_click->data.drag.p_reason_data;

    switch(p_notelayer_drag_data->reason_code)
    {
    case CB_CODE_NOTELAYER_NOTE_TRANSLATE:
    case CB_CODE_NOTELAYER_NOTE_RESCALE:
    case CB_CODE_NOTELAYER_NOTE_RESIZE:
        break;

    /* SKS 30apr95 pass on to note contents */
    case CB_CODE_NOTELAYER_NOTE_TRANSLATE_FOR_CLIENT:
    case CB_CODE_NOTELAYER_NOTE_RESIZE_FOR_CLIENT:
        break;

    default: default_unhandled();
        return(STATUS_OK);
    }

    switch(t5_message)
    {
    case T5_EVENT_CLICK_DRAG_STARTED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
        return(notelayer_drag_started_or_movement(p_docu, t5_message, p_skelevent_click, layer));

    default: default_unhandled();
#if CHECKING
    case T5_EVENT_CLICK_DRAG_ABORTED:
    case T5_EVENT_CLICK_DRAG_FINISHED:
#endif
        return(notelayer_drag_finished_or_aborted(p_docu, t5_message, p_skelevent_click, layer));
    }
}

_Check_return_
static STATUS
note_send_event_to_layer(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InVal_     LAYER layer)
{
    if(0 == array_elements(&p_docu->h_note_list))
        return(STATUS_OK);

    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(notelayer_rect_redraw(p_docu, (P_SKELEVENT_REDRAW) p_data, layer));

    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_LEFT_DOUBLE:
    case T5_EVENT_CLICK_LEFT_TRIPLE:
    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_DOUBLE:
    case T5_EVENT_CLICK_RIGHT_TRIPLE:
    case T5_EVENT_CLICK_RIGHT_DRAG:
    case T5_EVENT_FILEINSERT_DOINSERT:
        return(notelayer_click(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data, layer));

    case T5_EVENT_CLICK_DRAG_STARTED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
    case T5_EVENT_CLICK_DRAG_FINISHED:
    case T5_EVENT_CLICK_DRAG_ABORTED:
        return(notelayer_drag(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data, layer));

    default:
        return(STATUS_OK);
    }
}

PROC_EVENT_PROTO(static, note_event_paper_above)
{
    return(note_send_event_to_layer(p_docu, t5_message, p_data, LAYER_PAPER_ABOVE));
}

PROC_EVENT_PROTO(static, note_event_paper_below)
{
    return(note_send_event_to_layer(p_docu, t5_message, p_data, LAYER_PAPER_BELOW));
}

PROC_EVENT_PROTO(static, note_event_print_area_above)
{
    return(note_send_event_to_layer(p_docu, t5_message, p_data, LAYER_PRINT_AREA_ABOVE));
}

PROC_EVENT_PROTO(static, note_event_print_area_below)
{
    return(note_send_event_to_layer(p_docu, t5_message, p_data, LAYER_PRINT_AREA_BELOW));
}

PROC_EVENT_PROTO(static, note_event_cells_area_above)
{
    return(note_send_event_to_layer(p_docu, t5_message, p_data, LAYER_CELLS_AREA_ABOVE));
}

PROC_EVENT_PROTO(static, note_event_cells_area_below)
{
    return(note_send_event_to_layer(p_docu, t5_message, p_data, LAYER_CELLS_AREA_BELOW));
}

extern void
note_install_layer_handler(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_NOTE_INFO p_note_info)
{
    const LAYER layer = p_note_info->layer;
    P_PROC_EVENT p_proc_event;

    switch(layer)
    {
    default: default_unhandled();
#if CHECKING
    case LAYER_PAPER_BELOW:
#endif
        p_proc_event = note_event_paper_below;
        break;

    case LAYER_PRINT_AREA_BELOW:
        p_proc_event = note_event_print_area_below;
        break;

    case LAYER_CELLS_AREA_BELOW:
        p_proc_event = note_event_cells_area_below;
        break;

    case LAYER_CELLS_AREA_ABOVE:
        p_proc_event = note_event_cells_area_above;
        break;

    case LAYER_PRINT_AREA_ABOVE:
        p_proc_event = note_event_print_area_above;
        break;

    case LAYER_PAPER_ABOVE:
        p_proc_event = note_event_paper_above;
        break;
    }

    view_install_pane_window_handler(p_docu, layer, p_proc_event);
}

static void
make_skel_point_relative_to_page_num_in_layer(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_PAGE_NUM p_page_num,
    /*inout*/ P_SKEL_POINT p_skel_point,
    _InVal_     LAYER layer)
{
    SKEL_POINT skel_point = *p_skel_point;
    REDRAW_TAG redraw_tag = redraw_tag_from_layer(layer);
    PIXIT_POINT work_area;

    while(p_page_num->x > skel_point.page_num.x)
    {
        page_limits_from_page(p_docu, &work_area, redraw_tag, &skel_point.page_num);
        skel_point.pixit_point.x -= work_area.x;
        skel_point.page_num.x += 1;
    }

    while(p_page_num->x < skel_point.page_num.x)
    {
        skel_point.page_num.x -= 1;
        page_limits_from_page(p_docu, &work_area, redraw_tag, &skel_point.page_num);
        skel_point.pixit_point.x += work_area.x;
    }

    while(p_page_num->y > skel_point.page_num.y)
    {
        page_limits_from_page(p_docu, &work_area, redraw_tag, &skel_point.page_num);
        skel_point.pixit_point.y -= work_area.y;
        skel_point.page_num.y += 1;
    }

    while(p_page_num->y < skel_point.page_num.y)
    {
        skel_point.page_num.y -= 1;
        page_limits_from_page(p_docu, &work_area, redraw_tag, &skel_point.page_num);
        skel_point.pixit_point.y += work_area.y;
    }

    *p_skel_point = skel_point;
}

static void
notelayer_rect_redraw_drag(
    _DocuRef_   P_DOCU p_docu,
    P_CB_DATA_NOTELAYER_DRAG_DATA p_notelayer_drag_data,
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw,
    _InVal_     LAYER layer)
{
    switch(p_notelayer_drag_data->reason_code)
    {
    default:
        break;

    case CB_CODE_NOTELAYER_NOTE_TRANSLATE_FOR_CLIENT:
    case CB_CODE_NOTELAYER_NOTE_RESIZE_FOR_CLIENT:
        {
        P_NOTE_INFO p_note_info = notelayer_selection_first(p_docu);
        SKEL_RECT skel_rect;
        PIXIT_RECT pixit_rect;

        PTR_ASSERT(p_note_info);

        if(p_note_info->layer != layer)
            return;

        skel_rect = p_notelayer_drag_data->skel_rect_current;

        /* make rect relative to the page that is being redrawn rather than the page that the pointer was at and in the right layer! */
        make_skel_point_relative_to_page_num_in_layer(p_docu, &p_skelevent_redraw->clip_skel_rect.tl.page_num, &skel_rect.tl, p_note_info->layer);
        make_skel_point_relative_to_page_num_in_layer(p_docu, &p_skelevent_redraw->clip_skel_rect.tl.page_num, &skel_rect.br, p_note_info->layer);

        pixit_rect.tl = skel_rect.tl.pixit_point;
        pixit_rect.br = skel_rect.br.pixit_point;

        if(RECTANGLE_IS_VISIBLE(p_skelevent_redraw->clip_skel_rect, pixit_rect))
            host_invert_rectangle_outline(&p_skelevent_redraw->redraw_context, &pixit_rect, &rgb_stash[2 /*grey*/], &rgb_stash[0 /*white*/]);

        break;
        }

    case CB_CODE_NOTELAYER_NOTE_TRANSLATE:
    case CB_CODE_NOTELAYER_NOTE_RESCALE:
    case CB_CODE_NOTELAYER_NOTE_RESIZE:
        {
        P_NOTE_INFO p_note_info = notelayer_selection_first(p_docu);
        SKEL_RECT skel_rect;
        PIXIT_RECT pixit_rect;

        PTR_ASSERT(p_note_info);

        if(p_note_info->layer != layer)
            return;

        skel_rect = p_notelayer_drag_data->skel_rect_current;

        /* make rect relative to the page that is being redrawn rather than the page that the pointer was at and in the right layer! */
        make_skel_point_relative_to_page_num_in_layer(p_docu, &p_skelevent_redraw->clip_skel_rect.tl.page_num, &skel_rect.tl, p_note_info->layer);
        make_skel_point_relative_to_page_num_in_layer(p_docu, &p_skelevent_redraw->clip_skel_rect.tl.page_num, &skel_rect.br, p_note_info->layer);

        pixit_rect.tl = skel_rect.tl.pixit_point;
        pixit_rect.br = skel_rect.br.pixit_point;

        if(RECTANGLE_IS_VISIBLE(p_skelevent_redraw->clip_skel_rect, pixit_rect))
            host_invert_rectangle_outline(&p_skelevent_redraw->redraw_context, &pixit_rect, &rgb_stash[2 /*grey*/], &rgb_stash[0 /*white*/]);

        break;
        }
    }
}

_Check_return_
static STATUS
notelayer_rect_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw,
    _InVal_     LAYER layer)
{
    BOOL config_display_pictures = !global_preferences.dont_display_pictures;
    ARRAY_INDEX note_index;

    assert((p_skelevent_redraw->clip_skel_rect.tl.page_num.x == p_skelevent_redraw->clip_skel_rect.br.page_num.x) &&
           (p_skelevent_redraw->clip_skel_rect.tl.page_num.y == p_skelevent_redraw->clip_skel_rect.br.page_num.y));

    for(note_index = 0; note_index < array_elements(&p_docu->h_note_list); note_index++)
    {
        const P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);
        BOOL display_picture = config_display_pictures;

        if(p_note_info->layer != layer)
            continue;

        if(p_skelevent_redraw->redraw_context.flags.printer)
        {
            if(p_note_info->flags.dont_print)
                continue;

            display_picture = 1;
        }

        if(!p_note_info->flags.skel_rect_valid)
            notelayer_mount_note(p_docu, p_note_info);

        switch(p_note_info->note_pinning)
        {
        case NOTE_UNPINNED:
            {
            if(!p_note_info->flags.all_pages)
            {   /* check note tl corner's page_num against clip_skel_rect.page_num so we don't cause too much fg formatting to go on */
                if(p_note_info->skel_rect.tl.page_num.y > p_skelevent_redraw->clip_skel_rect.tl.page_num.y)
                    continue;
            }

            break;
            }

        default: default_unhandled();
#if CHECKING
        case NOTE_PIN_CELLS_SINGLE:
        case NOTE_PIN_CELLS_TWIN:
#endif
            {
            /* note is pinned to a cell: do a quick check of its tl.row against
             * clip_skel_rect.br so we don't cause too much fg formatting to go on
            */
            ROW_ENTRY row_entry;
            row_entry_from_skel_point(p_docu, &row_entry, &p_skelevent_redraw->clip_skel_rect.br, FALSE);

            /* off the end of the visible area? */
            if(p_note_info->region.tl.row > row_entry.row)
                continue;

            break;
            }
        }

        {
        PIXIT_RECT pixit_rect;

        /* render note iff note is displayed on page being rendered and the clip rectangle and note bbox (on that page) overlap */
        relative_pixit_rect_from_note(p_docu, p_note_info, &p_skelevent_redraw->clip_skel_rect.tl.page_num, &pixit_rect);

        if(RECTANGLE_IS_VISIBLE(p_skelevent_redraw->clip_skel_rect, pixit_rect))
        {
            /* set up object redraw information */
            NOTE_OBJECT_REDRAW note_object_redraw;
            /*RECT_FLAGS rect_flags;RECT_FLAGS_CLEAR(rect_flags);*/
            STATUS status = STATUS_OK;

            /* set up object rectangle */
            note_object_redraw.object_redraw.pixit_rect_object = pixit_rect;

            note_object_redraw.object_redraw.skel_rect_object = p_note_info->skel_rect;

            status_consume(object_data_from_slr(p_docu, &note_object_redraw.object_redraw.object_data, &p_note_info->region.tl));
            note_object_redraw.object_redraw.skel_rect_clip = p_skelevent_redraw->clip_skel_rect;

            note_object_redraw.object_redraw.flags.show_content = p_skelevent_redraw->flags.show_content;
            note_object_redraw.object_redraw.flags.show_selection = p_skelevent_redraw->flags.show_selection;

            /* work out any markers on cell */
            note_object_redraw.object_redraw.flags.marked_now = (p_note_info->note_selection != NOTE_SELECTION_NONE);
            note_object_redraw.object_redraw.flags.marked_screen = 0;

            note_object_redraw.object_redraw.redraw_context = p_skelevent_redraw->redraw_context;

            /*if(host_set_clip_rectangle(&note_object_redraw.object_redraw.redraw_context, &pixit_rect, rect_flags))*/
            {
#if !RELEASED && defined(SKS_BONK_WINDOW)
                if(host_setbgcolour(&rgb_stash[15]))
                    host_clg();
#endif

                if(display_picture)
                {
                    /* call the object for redraw/inversion */

                    /* MRJC: this is disgusting; really notes should have their own space in DATA_REFs and
                     * should be passed about in there rather than fudging it into a pointer
                     */
                    note_object_redraw.object_redraw.object_data.u.p_object = p_note_info->object_data_ref;
                    note_object_redraw.gr_scale_pair.x = p_note_info->gr_scale_pair.x ? p_note_info->gr_scale_pair.x : GR_SCALE_ONE;
                    note_object_redraw.gr_scale_pair.y = p_note_info->gr_scale_pair.y ? p_note_info->gr_scale_pair.y : GR_SCALE_ONE;

                    status = object_call_id(p_note_info->object_id, p_docu, T5_EVENT_REDRAW, &note_object_redraw);
                }

                if(!display_picture || (ERR_NOTE_NOT_LOADED == status))
                {
                    if(p_skelevent_redraw->flags.show_content)
                    {   /* show a picture frame filling the box */
                        host_paint_rectangle_crossed(&p_skelevent_redraw->redraw_context, &pixit_rect, &rgb_stash[(ERR_NOTE_NOT_LOADED == status) ? 11 : 3]);
                    }
                }

                host_restore_clip_rectangle(&p_skelevent_redraw->redraw_context);
            }
        }

        /* if note selected, consider drawing a border with grab ears around it */
        if(p_skelevent_redraw->flags.show_selection && !static_flag /* needs expansion of show_selection state eventually <<< */)
        {
            switch(p_note_info->note_selection)
            {
            case NOTE_SELECTION_SELECTED:
            case NOTE_SELECTION_IN_EDIT:
                {
                NOTE_OBJECT_BOUNDS note_object_bounds;

                notelayer_object_bounds(&note_object_bounds, p_note_info->note_pinning, &pixit_rect, &p_skelevent_redraw->redraw_context.one_program_pixel);

                /* render border iff clip rectangle and border bbox overlap */
                if(RECTANGLE_IS_VISIBLE(p_skelevent_redraw->clip_skel_rect, note_object_bounds.ears.outer_bound))
                {
                    NOTE_POSITION note_position = PIXIT_RECT_EAR_CENTRE;

                    if(NOTE_SELECTION_IN_EDIT == p_note_info->note_selection)
                        host_paint_rectangle_outline(&p_skelevent_redraw->redraw_context, &note_object_bounds.ears.outer_bound, rgb_frame);

                    host_paint_rectangle_outline(&p_skelevent_redraw->redraw_context, &note_object_bounds.ears.ear[note_position], rgb_frame);

                    while(++note_position < PIXIT_RECT_EAR_COUNT)
                    {
                        if(!note_object_bounds.ears.ear_active[note_position])
                            continue;

                        if(note_object_bounds.pinned[note_position])
                        {
                            host_paint_rectangle_filled(&p_skelevent_redraw->redraw_context, &note_object_bounds.ears.ear[note_position], rgb_ear_outer);
                        }
                        else
                        {
                            host_paint_rectangle_filled(&p_skelevent_redraw->redraw_context, &note_object_bounds.ears.ear[note_position], rgb_ear_inner);

                            host_paint_rectangle_outline(&p_skelevent_redraw->redraw_context, &note_object_bounds.ears.ear[note_position], rgb_ear_outer);
                        }
                    }
                }

                break;
                }

            default:
                break;
            }
        }
        } /*block*/
    } /*for(each note)*/

    { /* if dragging, eor boxes to show current position */
    P_ANY reasondata;
    if(host_drag_in_progress(p_docu, &reasondata))
        notelayer_rect_redraw_drag(p_docu, (P_CB_DATA_NOTELAYER_DRAG_DATA) reasondata, p_skelevent_redraw, layer);
    } /*block*/

    return(STATUS_OK);
}

/******************************************************************************
*
* Scan note layer to see if skel_point (usually the mouse position) is over anything interesting
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_NOTE_INFO
where_in_notelayer(
    _DocuRef_   P_DOCU p_docu,
    P_SKEL_POINT p_skel_point,
    P_PIXIT_POINT p_one_program_pixel,
    /*out*/ P_NOTE_POSITION p_note_position,
    _InVal_     BOOL set_pointer_shape)
{
#if 1

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER(p_skel_point);
    UNREFERENCED_PARAMETER(p_one_program_pixel);
    UNREFERENCED_PARAMETER(p_note_position);
    UNREFERENCED_PARAMETER_InVal_(set_pointer_shape);

    return(NULL); /* <<< watch me !!! */

#else

    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);
    POINTER_SHAPE pointer_shape = POINTER_DEFAULT;
    P_NOTE_INFO this_p_note_info = NULL;

    if(NULL != p_note_position)
        *p_note_position = PIXIT_RECT_EAR_NONE;

    while(--note_index >= 0)
    {
        P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        if(p_note_info->layer >= LAYER_CELLS_AREA_ABOVE)
        {
            PIXIT_RECT pixit_rect;

            relative_pixit_rect_from_note(p_docu, p_note_info, &p_skel_point->page_num, &pixit_rect);

            switch(p_note_info->flags.selection)
            {
            case NOTE_SELECTION_SELECTED:
            case NOTE_SELECTION_IN_EDIT:
                {
                NOTE_OBJECT_BOUNDS note_object_bounds;

                notelayer_object_bounds(&note_object_bounds, p_note_info->note_pinning, &pixit_rect, p_one_program_pixel);

                if(POINT_INSIDE_RECTANGLE(p_skel_point->pixit_point, note_object_bounds.ears.outer_bound))
                {
                    NOTE_POSITION note_position = PIXIT_RECT_EAR_COUNT;

                    while(--note_position >= 0)
                    {
                        if(!note_object_bounds.ears.ear_active[note_position])
                            continue;

                        if(POINT_INSIDE_RECTANGLE(p_skel_point->pixit_point, note_object_bounds.ears.ear[note_position]))
                        {
                            NOTE_DRAG_STATUS note_drag_status;

                            this_p_note_info = p_note_info;

                            if(NULL != p_note_position)
                                *p_note_position = note_position;

                            pointer_shape = notelayer_pointer_shape[note_position];

                            notelayer_drag_status_set(p_docu, p_note_info, &note_drag_status, NULL);

                            notelayer_drag_status_show(p_docu, &note_drag_status);

                            break; /*for(note_position)*/
                        }
                    }
                }

                break;
                }

            default:
                /* make thin horizontal/vertical lines easier to click on */
                expand_pixit_rect(&pixit_rect, p_one_program_pixel);

                if(POINT_INSIDE_RECTANGLE(p_skel_point->pixit_point, pixit_rect))
                {
                    this_p_note_info = p_note_info;

                    if(NULL != p_note_position)
                        *p_note_position = PIXIT_RECT_EAR_CENTRE;
                }

                break;
            }
        }

        if(this_p_note_info)
            break;
    }

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_APP_CLICK))
    if(NULL != p_note_position)
    {
        switch(*p_note_position)
        {
        case PIXIT_RECT_EAR_CENTRE: trace_0(TRACE_APP_CLICK, TEXT("over note centre"));    break;
        case PIXIT_RECT_EAR_TC: trace_0(TRACE_APP_CLICK, TEXT("over note top centre"));    break;
        case PIXIT_RECT_EAR_LC: trace_0(TRACE_APP_CLICK, TEXT("over note left centre"));   break;
        case PIXIT_RECT_EAR_RC: trace_0(TRACE_APP_CLICK, TEXT("over note right centre"));  break;
        case PIXIT_RECT_EAR_BC: trace_0(TRACE_APP_CLICK, TEXT("over note bottom centre")); break;
        case PIXIT_RECT_EAR_TR: trace_0(TRACE_APP_CLICK, TEXT("over note top right"));     break;
        case PIXIT_RECT_EAR_BL: trace_0(TRACE_APP_CLICK, TEXT("over note bottom left"));   break;
        case PIXIT_RECT_EAR_TL: trace_0(TRACE_APP_CLICK, TEXT("over note top left"));      break;
        case PIXIT_RECT_EAR_BR: trace_0(TRACE_APP_CLICK, TEXT("over note bottom right"));  break;
        }
    }
#endif

    if(set_pointer_shape)
        host_set_pointer_shape(pointer_shape);

    return(this_p_note_info);

#endif /* 0 */
}

static void
notelayer_invert_drag_boxes(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     LAYER layer)
{
    const REDRAW_TAG redraw_tag = redraw_tag_from_layer(layer);
    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;
    REDRAW_FLAGS redraw_flags;

    skel_rect.tl.page_num.x = skel_rect.tl.page_num.y = 0;
    skel_rect.tl.pixit_point.x = skel_rect.tl.pixit_point.y = 0;
    skel_rect.br = skel_rect.tl;

    RECT_FLAGS_CLEAR(rect_flags);
    rect_flags.extend_right_window = rect_flags.extend_down_window = 1;

    REDRAW_FLAGS_CLEAR(redraw_flags);
    redraw_flags.show_selection = TRUE;

    static_flag = 1;
    view_update_now(p_docu, redraw_tag, &skel_rect, rect_flags, redraw_flags, layer);
    static_flag = 0;
}

T5_MSG_PROTO(static, note_msg_note_update_now, P_NOTE_UPDATE_NOW p_note_update_now)
{
    const P_NOTE_INFO p_note_info = p_note_update_now->p_note_info;
    const LAYER layer = p_note_info->layer;
    const REDRAW_TAG redraw_tag = redraw_tag_from_layer(layer);
    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    skel_rect.tl.page_num.x = skel_rect.tl.page_num.y = 0;
    skel_rect.tl.pixit_point.x = skel_rect.tl.pixit_point.y = 0;
    skel_rect.br = skel_rect.tl;

    RECT_FLAGS_CLEAR(rect_flags);
    rect_flags.extend_right_window = rect_flags.extend_down_window = 1;

    view_update_now(p_docu, redraw_tag, &skel_rect, rect_flags, p_note_update_now->redraw_flags, layer);

    return(STATUS_OK);
}

/******************************************************************************
*
* return the bounding boxes of the object and each ear (grab patch)
* can be used for rendering or click position detection
*
******************************************************************************/

static void
notelayer_object_bounds(
    P_NOTE_OBJECT_BOUNDS p_note_object_bounds,
    _In_        NOTE_PINNING note_pinning,
    P_PIXIT_RECT p_pixit_rect,
    P_PIXIT_POINT p_one_program_pixel)
{
    pixit_rect_get_ears(&p_note_object_bounds->ears, p_pixit_rect, p_one_program_pixel);

    p_note_object_bounds->pinned[PIXIT_RECT_EAR_CENTRE] = FALSE;
    p_note_object_bounds->pinned[PIXIT_RECT_EAR_TL] = (NOTE_PIN_CELLS_TWIN == note_pinning) || (NOTE_PIN_CELLS_SINGLE == note_pinning);
    p_note_object_bounds->pinned[PIXIT_RECT_EAR_BR] = (NOTE_PIN_CELLS_TWIN == note_pinning);
    p_note_object_bounds->pinned[PIXIT_RECT_EAR_BL] = FALSE;
    p_note_object_bounds->pinned[PIXIT_RECT_EAR_TR] = FALSE;
    p_note_object_bounds->pinned[PIXIT_RECT_EAR_LC] = FALSE;
    p_note_object_bounds->pinned[PIXIT_RECT_EAR_RC] = FALSE;
    p_note_object_bounds->pinned[PIXIT_RECT_EAR_TC] = FALSE;
    p_note_object_bounds->pinned[PIXIT_RECT_EAR_BC] = FALSE;
}

static void
expand_pixit_rect(
    P_PIXIT_RECT p_pixit_rect,
    P_PIXIT_POINT p_one_program_pixel)
{
    p_pixit_rect->tl.x -= p_one_program_pixel->x;
    p_pixit_rect->tl.y -= p_one_program_pixel->y;
    p_pixit_rect->br.x += p_one_program_pixel->x;
    p_pixit_rect->br.y += p_one_program_pixel->y;
}

_Check_return_
static STATUS
note_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    p_docu->h_note_list = 0;

    return(maeve_event_handler_add(p_docu, maeve_event_ob_note, (CLIENT_HANDLE) 0));
}

_Check_return_
static STATUS
note_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    ARRAY_INDEX i = array_elements(&p_docu->h_note_list);

    while(--i >= 0)
        note_delete(p_docu, array_ptr(&p_docu->h_note_list, NOTE_INFO, i));

    al_array_dispose(&p_docu->h_note_list);

    maeve_event_handler_del(p_docu, maeve_event_ob_note, (CLIENT_HANDLE) 0);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, note_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(resource_init(OBJECT_ID_NOTE, P_BOUND_MESSAGES_OBJECT_ID_NOTE, P_BOUND_RESOURCES_OBJECT_ID_NOTE));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_NOTE));

    case T5_MSG_IC__INIT1:
        return(note_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(note_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, note_msg_note_new, _InRef_ PC_NOTE_INFO p_note_info)
{
    STATUS status;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    consume_ptr(notelayer_new_note(p_docu, p_note_info, &status));

    return(status);
}

_Check_return_
static STATUS
note_msg_choice_changed_display_pictures(
    _DocuRef_   P_DOCU p_docu)
{
    /* display pictures option may have changed, so repaint docu iff any notes */
    if(array_elements(&p_docu->h_note_list))
    {
        SKEL_RECT skel_rect;
        RECT_FLAGS rect_flags;

        skel_rect.tl.page_num.x = skel_rect.tl.page_num.y = 0;
        skel_rect.tl.pixit_point.x = skel_rect.tl.pixit_point.y = 0;
        skel_rect.br = skel_rect.tl;

        RECT_FLAGS_CLEAR(rect_flags);
        rect_flags.extend_right_window = rect_flags.extend_down_window = 1;

        view_update_later(p_docu, UPDATE_PANE_PAPER, &skel_rect, rect_flags);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, note_msg_choice_changed, _InRef_ PC_MSG_CHOICE_CHANGED p_msg_choice_changed)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_choice_changed->t5_message)
    {
    case T5_CMD_CHOICES_DISPLAY_PICTURES:
        return(note_msg_choice_changed_display_pictures(p_docu));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, note_event_pointer_movement, _InRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    trace_4(TRACE_APP_CLICK, TEXT("proc_event_note T5_EVENT_POINTER_MOVEMENT at page(") S32_TFMT TEXT(",") S32_TFMT TEXT(") pixit(") PIXIT_TFMT TEXT(",") PIXIT_TFMT TEXT(")"),
            p_skelevent_click->skel_point.page_num.x,    p_skelevent_click->skel_point.page_num.y,
            p_skelevent_click->skel_point.pixit_point.x, p_skelevent_click->skel_point.pixit_point.y);

    (void) where_in_notelayer(p_docu, &p_skelevent_click->skel_point, &p_skelevent_click->click_context.one_program_pixel, NULL, TRUE /* set pointer shape */);

    return(STATUS_OK);
}

_Check_return_
static STATUS
note_event_pointer_leaves_window(
    _DocuRef_   P_DOCU p_docu)
{
    status_line_clear(p_docu, STATUS_LINE_LEVEL_DRAGGING);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, object_note_cmd)
{
    switch(T5_MESSAGE_CMD_OFFSET(t5_message))
    {
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_NOTE):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_NOTE_TWIN):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_NOTE_BACKDROP):
        return(t5_cmd_note(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_NOTE_BACK):
        return(t5_cmd_note_back(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_NOTE_EMBED):
        return(t5_cmd_note_embed(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_NOTE_SWAP):
        return(t5_cmd_note_swap(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_BACKDROP):
        return(t5_cmd_backdrop(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_BACKDROP_INTRO):
        return(t5_cmd_backdrop_intro(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DELETE_CHARACTER_LEFT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DELETE_CHARACTER_RIGHT):
        return(note_cmd_delete_character_lr(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_TOGGLE_MARKS):
        return(note_cmd_toggle_marks(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SELECTION_CLEAR):
        return(note_cmd_selection_clear(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SELECTION_COPY):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SELECTION_CUT):
        return(note_cmd_selection_copy_or_cut(p_docu, t5_message));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SELECTION_DELETE):
        return(note_cmd_selection_delete(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_PASTE_AT_CURSOR):
        return(note_cmd_paste_at_cursor(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SNAPSHOT):
        return(note_cmd_snapshot(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_FORCE_RECALC):
        return(note_cmd_force_recalc(p_docu));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_note);
OBJECT_PROTO(extern, object_note)
{
    if(T5_MESSAGE_IS_CMD(t5_message))
        return(object_note_cmd(p_docu, t5_message, (PC_T5_CMD) p_data));

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(note_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_CHOICE_CHANGED:
        return(note_msg_choice_changed(p_docu, t5_message, (PC_MSG_CHOICE_CHANGED) p_data));

    case T5_MSG_NOTE_NEW:
        return(note_msg_note_new(p_docu, t5_message, (PC_NOTE_INFO) p_data));

    case T5_MSG_NOTE_UPDATE_NOW:
        return(note_msg_note_update_now(p_docu, t5_message, (P_NOTE_UPDATE_NOW) p_data));

    case T5_MSG_NOTE_UPDATE_OBJECT:
        return(note_msg_note_update_object(p_docu, t5_message, (P_NOTE_UPDATE_OBJECT) p_data));

    case T5_MSG_NOTE_UPDATE_OBJECT_INFO:
        return(note_msg_note_update_object_info(p_docu, t5_message, (P_NOTE_UPDATE_OBJECT_INFO) p_data));

    case T5_MSG_CARET_SHOW_CLAIM:
        return(note_msg_caret_show_claim(p_docu));

    case T5_MSG_MARK_INFO_READ:
        return(note_msg_mark_info_read(p_docu, t5_message, (P_MARK_INFO) p_data));

    case T5_MSG_MENU_OTHER_INFO:
        return(note_msg_menu_other_info(p_docu,  t5_message, (P_MENU_OTHER_INFO) p_data));

    case T5_MSG_NOTE_MENU_QUERY:
        return(note_msg_note_menu_query(p_docu, t5_message, (P_NOTE_MENU_QUERY) p_data));

    case T5_MSG_NOTELAYER_SELECTION_INFO:
        return(note_msg_notelayer_selection_info(p_docu, t5_message, (P_NOTELAYER_SELECTION_INFO) p_data));

    case T5_EVENT_POINTER_ENTERS_WINDOW:
    case T5_EVENT_POINTER_MOVEMENT:
        return(note_event_pointer_movement(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_POINTER_LEAVES_WINDOW:
        return(note_event_pointer_leaves_window(p_docu));

    default:
        return(STATUS_OK);
    }
}

/* end of ob_note.c */
