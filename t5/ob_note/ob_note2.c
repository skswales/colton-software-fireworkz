/* ob_note2.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* More Fireworkz note layer */

/* SKS July 1993 */

#include "common/gflags.h"

#include "ob_note/ob_note.h"

#include "ob_toolb/xp_toolb.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

/*
callback routines
*/

PROC_UREF_EVENT_PROTO(static, proc_uref_event_sk_note);

/*
internal routines
*/

static void
note_uref_unlink(
    _DocuRef_   P_DOCU p_docu,
    P_NOTE_INFO p_note_info);

/*
offsets in args_cmd_note[]
*/

enum ARG_CMD_NOTE
{
    ARG_CMD_NOTE_OWNER_CS     =  0,
    ARG_CMD_NOTE_DATA_TYPE_CS =  1,

    ARG_CMD_NOTE_LAYER        =  2,
    ARG_CMD_NOTE_MOUNT        =  3,

    ARG_CMD_NOTE_POS_TL_X     =  4,
    ARG_CMD_NOTE_POS_TL_Y     =  5,

    ARG_CMD_NOTE_6            =  6,
    ARG_CMD_NOTE_7            =  7,
    ARG_CMD_NOTE_8            =  8,
    ARG_CMD_NOTE_9            =  9,
    ARG_CMD_NOTE_10           = 10,
    ARG_CMD_NOTE_11           = 11,

    ARG_CMD_NOTE_CONTENTS     = 12,
    ARG_CMD_NOTE_DONT_PRINT   = 13,

    ARG_CMD_NOTE_SCALE_X      = 14,
    ARG_CMD_NOTE_SCALE_Y      = 15,

    ARG_CMD_NOTE_SIZE_X       = 16, /* 2.01 */
    ARG_CMD_NOTE_SIZE_Y       = 17,

    ARG_CMD_NOTE_N_ARGS       = 18
};

static S32 next_client_handle = 0; /* each note registered by us with uref has a unique handle (any start value will do) */

/******************************************************************************
*
* Delete a note
*
******************************************************************************/

extern void
note_delete(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    status_consume(object_call_id(p_note_info->object_id, p_docu, T5_MSG_NOTE_DELETE, p_note_info->object_data_ref));

    note_uref_unlink(p_docu, p_note_info);

    al_array_delete_at(&p_docu->h_note_list, -1, array_indexof_element(&p_docu->h_note_list, NOTE_INFO, p_note_info));
}

static void
note_drop_object_selection(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    switch(p_note_info->note_selection)
    {
    default:
        break;

    case NOTE_SELECTION_EDITED:
        { /* tell the object that his selection has better go too */
        NOTE_OBJECT_SELECTION_CLEAR note_object_selection_clear;
        note_object_selection_clear.object_data_ref = p_note_info->object_data_ref;
        note_object_selection_clear.p_note_info = p_note_info;
        status_consume(object_call_id(p_note_info->object_id, p_docu, T5_MSG_SELECTION_CLEAR, &note_object_selection_clear));
        break;
        }
    }
}

_Check_return_
static STATUS
note_save(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_NOTE_INFO p_note_info_in,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     STATUS clipping)
{
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    T5_MESSAGE t5_message;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    P_ARGLIST_ARG p_args;
    S32 extref;
    STATUS status;
    NOTE_INFO note_info = *p_note_info_in; /* copy it */
    const P_NOTE_INFO p_note_info = &note_info; /* then point at that copy */
    S32 ext_mount_type;

    if(clipping)
    {
        if(NOTE_PIN_CELLS_TWIN == note_info.note_pinning)
        {
            PIXIT_RECT pixit_rect;

            /* obtain PIXIT_RECT of note relative to its top-left corner - this allows us to find its size */
            relative_pixit_rect_from_note(p_docu, &note_info, &note_info.skel_rect.tl.page_num, &pixit_rect);

            note_info.pixit_size.cx = pixit_rect_width(&pixit_rect);
            note_info.pixit_size.cy = pixit_rect_height(&pixit_rect);

            note_info.note_pinning = NOTE_PIN_CELLS_SINGLE; /* and then do... */
        }

        if(NOTE_PIN_CELLS_SINGLE == note_info.note_pinning)
        {
            note_info.offset_tl.x = 0;
            note_info.offset_tl.y = 0;

            note_info.region.tl.col = 0;
            note_info.region.tl.row = 0;
        }
    }

    {
    NOTE_ENSURE_SAVED note_ensure_saved;

    note_ensure_saved.object_data_ref = p_note_info->object_data_ref;
    note_ensure_saved.p_of_op_format = p_of_op_format;

    status_return(object_call_id(p_note_info->object_id, p_docu, T5_MSG_NOTE_ENSURE_SAVED, &note_ensure_saved));

    extref = note_ensure_saved.extref;
    } /*block*/

    switch(p_note_info->note_pinning)
    {
    case NOTE_PIN_CELLS_SINGLE:
        t5_message = T5_CMD_NOTE;
        break;

    case NOTE_PIN_CELLS_TWIN:
        t5_message = T5_CMD_NOTE_TWIN;
        break;

    default:default_unhandled();
#if CHECKING
    case NOTE_UNPINNED:
#endif
        if(p_note_info->layer == LAYER_PAPER_BELOW)
            t5_message = T5_CMD_NOTE_BACKDROP;
        else
            t5_message = T5_CMD_NOTE;
        break;
    }
    status_return(arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table));

    p_args = p_arglist_args(&arglist_handle, ARG_CMD_NOTE_N_ARGS);

    /* note type, drawfile, chart etc. */
    p_args[ARG_CMD_NOTE_OWNER_CS].val.u8c = construct_id_from_object_id(p_note_info->object_id);
    p_args[ARG_CMD_NOTE_DATA_TYPE_CS].val.u8c = 'O';
    p_args[ARG_CMD_NOTE_LAYER].val.s32 = (p_note_info->layer > LAYER_CELLS) ? +1 : -1;

    p_args[ARG_CMD_NOTE_POS_TL_X].val.pixit = p_note_info->offset_tl.x;
    p_args[ARG_CMD_NOTE_POS_TL_Y].val.pixit = p_note_info->offset_tl.y;

    switch(p_note_info->note_pinning)
    {
    case NOTE_PIN_CELLS_SINGLE:
        { /* 1, x,y,c,r, w,h */
        ext_mount_type = EXT_ID_NOTE_PIN_CELLS_SINGLE;

        p_args[6].type = ARG_TYPE_COL;
        p_args[6].val.col = p_note_info->region.tl.col;
        p_args[7].type = ARG_TYPE_ROW;
        p_args[7].val.row = p_note_info->region.tl.row;

        p_args[8].val.pixit = p_note_info->pixit_size.cx;
        p_args[9].val.pixit = p_note_info->pixit_size.cy;

        arg_dispose(&arglist_handle, 10);
        arg_dispose(&arglist_handle, 11);

        break;
        }

    case NOTE_PIN_CELLS_TWIN:
        { /* 2, x,y,c,r, x,y,c,r */
        ext_mount_type = EXT_ID_NOTE_PIN_CELLS_TWIN;

        p_args[6].type = ARG_TYPE_COL;
        p_args[6].val.col = p_note_info->region.tl.col;
        p_args[7].type = ARG_TYPE_ROW;
        p_args[7].val.row = p_note_info->region.tl.row;

        p_args[8].val.pixit = p_note_info->offset_br.x;
        p_args[9].val.pixit = p_note_info->offset_br.y;

        p_args[10].type = ARG_TYPE_COL;
        p_args[10].val.col = p_note_info->region.br.col - 1; /* e,e -> i,i in file */
        p_args[11].type = ARG_TYPE_ROW;
        p_args[11].val.row = p_note_info->region.br.row - 1;

        break;
        }

    default: default_unhandled();
#if CHECKING
    case NOTE_UNPINNED:
#endif
        { /* 3/4/5, x,y,w,h */
        switch(p_note_info->layer)
        {
        default: default_unhandled();
#if CHECKING
        case LAYER_CELLS_AREA_ABOVE:
        case LAYER_CELLS_AREA_BELOW:
#endif
            ext_mount_type = EXT_ID_NOTE_PIN_CELLS_AREA;
            break;

        case LAYER_PRINT_AREA_ABOVE:
        case LAYER_PRINT_AREA_BELOW:
            ext_mount_type = EXT_ID_NOTE_PIN_PRINT_AREA;
            break;

        case LAYER_PAPER_ABOVE:
        case LAYER_PAPER_BELOW:
            ext_mount_type = EXT_ID_NOTE_PIN_PAPER;
            break;
        }

        p_args[6].val.pixit = p_note_info->pixit_size.cx;
        p_args[7].val.pixit = p_note_info->pixit_size.cy;

        p_args[8].val.s32 = p_note_info->flags.scale_to_fit;
        p_args[9].val.s32 = p_note_info->flags.all_pages;

        p_args[10].val.s32 = p_note_info->skel_rect.tl.page_num.x; /* SKS after 1.05 24oct93 save the page number! (previously unused args) */
        p_args[11].val.s32 = p_note_info->skel_rect.tl.page_num.y;

        break;
        }
    }

    p_args[ARG_CMD_NOTE_MOUNT].val.s32 = ext_mount_type;

    p_args[ARG_CMD_NOTE_CONTENTS].val.s32 = extref;

    p_args[ARG_CMD_NOTE_DONT_PRINT].val.s32 = (S32) p_note_info->flags.dont_print;

    p_args[ARG_CMD_NOTE_SCALE_X].val.s32 = (S32) p_note_info->gr_scale_pair.x;
    p_args[ARG_CMD_NOTE_SCALE_Y].val.s32 = (S32) p_note_info->gr_scale_pair.y;

    p_args[ARG_CMD_NOTE_SIZE_X].val.pixit = p_note_info->pixit_size.cx; /* as of 2.01 we save the size here - old readers will ignore it */
    p_args[ARG_CMD_NOTE_SIZE_Y].val.pixit = p_note_info->pixit_size.cy;

    status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

    arglist_dispose(&arglist_handle);

    return(status);
}

_Check_return_
static STATUS
note_uref_link(
    _DocuRef_   P_DOCU p_docu,
    P_NOTE_INFO p_note_info)
{
    switch(p_note_info->note_pinning)
    {
    case NOTE_UNPINNED:
        return(STATUS_OK);

    default: default_unhandled();
#if CHECKING
    case NOTE_PIN_CELLS_SINGLE:
    case NOTE_PIN_CELLS_TWIN:
#endif
        break;
    }

    status_return(uref_add_dependency(p_docu, &p_note_info->region, proc_uref_event_sk_note, next_client_handle, &p_note_info->uref_handle, FALSE));

    p_note_info->client_handle = next_client_handle++; /* passed to us when proc_event_sk_note_uref is called, identifies this note */

    p_note_info->flags.uref_registered = TRUE;

    return(STATUS_OK);
}

static void
note_uref_unlink(
    _DocuRef_   P_DOCU p_docu,
    P_NOTE_INFO p_note_info)
{
    if(p_note_info->flags.uref_registered)
    {
        p_note_info->flags.uref_registered = FALSE;

        uref_del_dependency(docno_from_p_docu(p_docu), p_note_info->uref_handle);
    }
}

_Check_return_
extern STATUS
notelayer_save_clip_data(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_ARRAY_HANDLE p_h_clip_data)
{
    OF_OP_FORMAT of_op_format = OF_OP_FORMAT_INIT;
    STATUS status;
    ARRAY_INDEX note_index;

    of_op_format.process_status.flags.foreground = 1;
    of_op_format.process_status.reason.type = UI_TEXT_TYPE_RESID;
    of_op_format.process_status.reason.text.resource_id = MSG_STATUS_CUTTING;

    status_return(status = ownform_initialise_save(p_docu, &of_op_format, p_h_clip_data, NULL, FILETYPE_TEXT, NULL));

    for(note_index = 0; note_index < array_elements(&p_docu->h_note_list); ++note_index)
    {
        const P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        if(p_note_info->note_selection == NOTE_SELECTION_SELECTED)
            status_break(status = note_save(p_docu, p_note_info, &of_op_format, 1 /*clipping*/));
    }

    return(ownform_finalise_save(&p_docu, &of_op_format, status));
}

static void
notelayer_mark_as_repositioned(
    _DocuRef_   P_DOCU p_docu)
{
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    while(--note_index >= 0)
    {
        const P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        p_note_info->flags.skel_rect_valid = 0;
    }
}

/******************************************************************************
*
* Calculate the note bbox
*
******************************************************************************/

extern void
notelayer_mount_note(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    const REDRAW_TAG redraw_tag = redraw_tag_from_layer(p_note_info->layer);

    switch(p_note_info->note_pinning)
    {
    case NOTE_UNPINNED:
        /* in all cases p_note_info->skel_rect.tl.page_num must be valid */
        if(p_note_info->flags.scale_to_fit)
        {
            PIXIT_POINT work_area;

            p_note_info->skel_rect.tl.pixit_point.x = 0;
            p_note_info->skel_rect.tl.pixit_point.y = 0;

            page_limits_from_page(p_docu, &work_area, redraw_tag, &p_note_info->skel_rect.tl.page_num);

            /* now calculate scale factors to apply to the natural size to get to this size */
            p_note_info->gr_scale_pair.x = gr_scale_from_s32_pair(work_area.x, p_note_info->pixit_size.cx);
            p_note_info->gr_scale_pair.y = gr_scale_from_s32_pair(work_area.y, p_note_info->pixit_size.cy);
        }
        else
        {
            p_note_info->skel_rect.tl.pixit_point = p_note_info->offset_tl;
        }

        p_note_info->skel_rect.br = p_note_info->skel_rect.tl;
        p_note_info->skel_rect.br.pixit_point.x += gr_coord_scale(p_note_info->pixit_size.cx, p_note_info->gr_scale_pair.x);
        p_note_info->skel_rect.br.pixit_point.y += gr_coord_scale(p_note_info->pixit_size.cy, p_note_info->gr_scale_pair.y);
        break;

    default: default_unhandled();
#if CHECKING
    case NOTE_PIN_CELLS_SINGLE:
    case NOTE_PIN_CELLS_TWIN:
#endif
        skel_point_from_slr_tl(p_docu, &p_note_info->skel_rect.tl, &p_note_info->region.tl);
        p_note_info->skel_rect.tl.pixit_point.x += p_note_info->offset_tl.x;
        p_note_info->skel_rect.tl.pixit_point.y += p_note_info->offset_tl.y;
        skel_point_normalise(p_docu, &p_note_info->skel_rect.tl, redraw_tag);

        p_note_info->skel_rect.br = p_note_info->skel_rect.tl;
        if(NOTE_PIN_CELLS_TWIN == p_note_info->note_pinning)
        {
            SLR slr;
            slr.col = p_note_info->region.br.col - 1; /* SKS after 1.19/11 28jan95 */
            slr.row = p_note_info->region.br.row - 1;
            skel_point_from_slr_tl(p_docu, &p_note_info->skel_rect.br, &slr);
            p_note_info->skel_rect.br.pixit_point.x += p_note_info->offset_br.x;
            p_note_info->skel_rect.br.pixit_point.y += p_note_info->offset_br.y;
        }
        else /* NOTE_PIN_CELLS_SINGLE */
        {
            PIXIT_SIZE scaled_pixit_size;
            scaled_pixit_size.cx = gr_coord_scale(p_note_info->pixit_size.cx, p_note_info->gr_scale_pair.x);
            scaled_pixit_size.cy = gr_coord_scale(p_note_info->pixit_size.cy, p_note_info->gr_scale_pair.y);
            p_note_info->skel_rect.br.pixit_point.x += scaled_pixit_size.cx;
            p_note_info->skel_rect.br.pixit_point.y += scaled_pixit_size.cy;
        }
        skel_point_normalise(p_docu, &p_note_info->skel_rect.br, redraw_tag);

        if(NOTE_PIN_CELLS_TWIN == p_note_info->note_pinning)
        {   /* for NOTE_PIN_CELLS_TWIN we need to derive and note the gr_scale_pair for future use */
            PIXIT_POINT tl_pixit_point, br_pixit_point, distance;

            /* first of all, calculate the distance from tl to br */
            relative_pixit_point_from_skel_point_in_layer(p_docu, &p_note_info->skel_rect.tl.page_num, &p_note_info->skel_rect.tl, &tl_pixit_point, p_note_info->layer);
            relative_pixit_point_from_skel_point_in_layer(p_docu, &p_note_info->skel_rect.tl.page_num, &p_note_info->skel_rect.br, &br_pixit_point, p_note_info->layer);

            distance.x = br_pixit_point.x - tl_pixit_point.x;
            distance.y = br_pixit_point.y - tl_pixit_point.y;

            /* now calculate scale factors to apply to the natural size to get to this size */
            p_note_info->gr_scale_pair.x = gr_scale_from_s32_pair(distance.x, p_note_info->pixit_size.cx);
            p_note_info->gr_scale_pair.y = gr_scale_from_s32_pair(distance.y, p_note_info->pixit_size.cy);
        }

        break;
    }

    p_note_info->flags.skel_rect_valid = 1;
}

/*ncr*/
_Ret_maybenull_
extern P_NOTE_INFO
notelayer_new_note(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_NOTE_INFO p_note_info_in,
    _OutRef_    P_STATUS p_status)
{
    P_NOTE_INFO p_note_info;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_note_info), FALSE);

    if(NULL != (p_note_info = al_array_extend_by(&p_docu->h_note_list, NOTE_INFO, 1, &array_init_block, p_status)))
    {
        *p_note_info = *p_note_info_in;

        note_install_layer_handler(p_docu, p_note_info);

        *p_status = note_uref_link(p_docu, p_note_info);
    }

    return(p_note_info);
}

/******************************************************************************
*
* Delete all the selected notes
*
******************************************************************************/

_Check_return_
static S32
notelayer_selection_delete_selected_note(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    switch(p_note_info->note_selection)
    {
    case NOTE_SELECTION_NONE:
        return(0);

    default:
        {
        note_update_later(p_docu, p_note_info, NOTE_UPDATE_ALL);

        note_delete(p_docu, p_note_info);

        return(1);
        }
    }
}

extern void
notelayer_selection_delete(
    _DocuRef_   P_DOCU p_docu)
{
    S32 selection_changed = 0;
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    while(--note_index >= 0)
    {
        const P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        selection_changed += notelayer_selection_delete_selected_note(p_docu, p_note_info);
    }

    if(0 != selection_changed)
    {   /* tell everyone that selection has changed */
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_NEW, P_DATA_NONE));

        caret_show_claim(p_docu, p_docu->focus_owner_old, FALSE);/* ensure focus is not in notelayer */
    }
}

_Check_return_
_Ret_maybenull_
static P_NOTE_INFO
p_note_info_from_client_handle(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     CLIENT_HANDLE client_handle)
{
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    while(--note_index >= 0)
    {
        P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        if(!p_note_info->flags.uref_registered)
            continue;

        if(p_note_info->client_handle != client_handle)
            continue;

        return(p_note_info);
    }

    return(NULL);
}

/*
main events
*/

_Check_return_
static STATUS
sk_note_msg_focus_changed(
    _DocuRef_   P_DOCU p_docu)
{
    T5_TOOLBAR_TOOL_ENABLE t5_toolbar_tool_enable;
    t5_toolbar_tool_enable.enabled = (OBJECT_ID_NOTE == p_docu->focus_owner);
    t5_toolbar_tool_enable.enable_id = TOOL_ENABLE_NOTE_FOCUS;
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("COPY"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("CUT"));
    return(STATUS_OK);
}

T5_MSG_PROTO(static, sk_note_msg_reformat, _InRef_ PC_DOCU_REFORMAT p_docu_reformat)
{
    IGNOREPARM_InVal_(t5_message);

    switch(p_docu_reformat->action)
    {
    case REFORMAT_HEFO_Y:       /* actually only need to reposition backdrop note */

        /*FALLTHRU*/

    case REFORMAT_XY:           /* don't actually need to reposition backdrop note */
        notelayer_mark_as_repositioned(p_docu);
        break;

    case REFORMAT_Y:            /* don't actually need to reposition backdrop note */
        notelayer_mark_as_repositioned(p_docu);
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static S32
sk_note_msg_selection_clear_selected_note(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    switch(p_note_info->note_selection)
    {
    case NOTE_SELECTION_NONE:
        return(0);

    default:
        {
        note_drop_object_selection(p_docu, p_note_info);

        note_update_later(p_docu, p_note_info, NOTE_UPDATE_SELECTION_MARKS);

        p_note_info->note_selection = NOTE_SELECTION_NONE;

        return(1);
        }
    }
}

_Check_return_
static STATUS
sk_note_msg_selection_clear(
    _DocuRef_   P_DOCU p_docu)
{
    S32 selection_changed = 0;
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    while(--note_index >= 0)
    {
        const P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        selection_changed += sk_note_msg_selection_clear_selected_note(p_docu, p_note_info);
    }

    if(0 != selection_changed)
    {
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_NEW, P_DATA_NONE));

        /*notelayer_lose_focus(p_docu); debate whose responsibility this is */
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
sk_note_msg_data_save_3(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX note_index;

    for(note_index = 0; note_index < array_elements(&p_docu->h_note_list); ++note_index)
    {
        const P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);
        STATUS do_save = (p_of_op_format->of_template.data_class >= DATA_SAVE_DOC);

        switch(p_note_info->note_pinning)
        {
        case NOTE_UNPINNED:
            break;

        default: default_unhandled();
#if CHECKING
        case NOTE_PIN_CELLS_SINGLE:
        case NOTE_PIN_CELLS_TWIN:
#endif
            {
            REGION region, region_t;

            region_from_two_slrs(&region, &p_note_info->region.tl, &p_note_info->region.tl, TRUE);

            do_save = region_from_docu_area_min(&region_t, &p_of_op_format->save_docu_area)
                      &&
                      region_intersect_region(&region, &region_t);
            break;
            }
        }

        if(do_save)
            status_break(status = note_save(p_docu, p_note_info, p_of_op_format, 0));
    }

    return(status);
}

T5_MSG_PROTO(static, sk_note_msg_save, _InRef_ PC_MSG_SAVE p_msg_save)
{
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_save->t5_msg_save_message)
    {
    case T5_MSG_SAVE__DATA_SAVE_3:
        return(sk_note_msg_data_save_3(p_docu, p_msg_save->p_of_op_format));

    default:
        return(STATUS_OK);
    }
}

MAEVE_EVENT_PROTO(extern, maeve_event_ob_note)
{
    IGNOREPARM_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_FOCUS_CHANGED:
        return(sk_note_msg_focus_changed(p_docu));

    case T5_MSG_REFORMAT:
        return(sk_note_msg_reformat(p_docu, t5_message, (PC_DOCU_REFORMAT) p_data));

    case T5_MSG_SELECTION_CLEAR:
        return(sk_note_msg_selection_clear(p_docu));

    case T5_MSG_SAVE:
        return(sk_note_msg_save(p_docu, t5_message, (PC_MSG_SAVE) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* handle uref events for notes pinned to cells
*
******************************************************************************/

PROC_UREF_EVENT_PROTO(static, sk_note_ref_dep_delete) /* dependency must be deleted */
{
    const P_NOTE_INFO p_note_info = p_note_info_from_client_handle(p_docu, p_uref_event_block->uref_id.client_handle);

    switch(t5_message)
    {
    default:
        /* free a region */
        if(NULL != p_note_info)
            note_delete(p_docu, p_note_info);
        break;

    case T5_MSG_UREF_CLOSE2:
        /* remove uref dependency */
        if(NULL != p_note_info)
            note_uref_unlink(p_docu, p_note_info);
        break;
    }

    return(STATUS_OK);
}

PROC_UREF_EVENT_PROTO(static, sk_note_ref_dep_update) /* dependency region must be updated */
{
    const P_NOTE_INFO p_note_info = p_note_info_from_client_handle(p_docu, p_uref_event_block->uref_id.client_handle);

    if(NULL != p_note_info)
    {
        switch(p_note_info->note_pinning)
        {
        default: default_unhandled();
            break;

        case NOTE_PIN_CELLS_SINGLE:
            /* simple update if ok */
            if(uref_match_slr(&p_note_info->region.tl, t5_message, p_uref_event_block) == DEP_DELETE)
                note_delete(p_docu, p_note_info);
            break;

        case NOTE_PIN_CELLS_TWIN:
            if(uref_match_region(&p_note_info->region, t5_message, p_uref_event_block) == DEP_DELETE)
                note_delete(p_docu, p_note_info);
            break;
        }
    }

    return(STATUS_OK);
}

PROC_UREF_EVENT_PROTO(static, proc_uref_event_sk_note)
{
    switch(p_uref_event_block->reason.code)
    {
    case DEP_DELETE: /* dependency must be deleted */
        return(sk_note_ref_dep_delete(p_docu, t5_message, p_uref_event_block));

    case DEP_INFORM:
        return(STATUS_OK);

    case DEP_UPDATE: /* dependency region must be updated */
        return(sk_note_ref_dep_update(p_docu, t5_message, p_uref_event_block));

    default: default_unhandled();
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* Add an object to one of the note layers
*
* NOTE_UNPINNED         : area relative
*    the top left corner is nailed relative to the top left of an area (PIXIT_POINT)
*
* NOTE_PIN_CELLS_SINGLE : cell relative
*    the top left corner is nailed relative to the top left of a cell (SLR + PIXIT_POINT)
*
* NOTE_PIN_CELLS_TWIN   : cell spanning
*    the top left corner is nailed relative to the top left of a cell (SLR + PIXIT_POINT)
*    and the bottom right corner is nailed relative to the bottom right of a second cell
*
******************************************************************************/

T5_CMD_PROTO(extern, t5_cmd_note)
{
    const P_OF_IP_FORMAT p_of_ip_format = p_t5_cmd->p_of_ip_format;
    const P_ARGLIST_ARG p_args = p_arglist_args(&p_t5_cmd->arglist_handle, ARG_CMD_NOTE_N_ARGS); /* NB we mutate some S32 as COL/ROW args */
    const BOOL at_front = (p_args[ARG_CMD_NOTE_LAYER].val.s32 > 0);
    LAYER layer;
    NOTE_INFO note_info;
    zero_struct(note_info);

    IGNOREPARM_InVal_(t5_message);

    note_info.object_id = OBJECT_ID_NONE;

    if(arg_is_present(p_args, 0))
    {
        STATUS status;
        if(status_fail(status = object_id_from_construct_id(p_args[0].val.u8c, &note_info.object_id)))
        {
            if(status != STATUS_FAIL)
                return(status);
            status = STATUS_OK;
        }
    }

    note_info.offset_tl.x = p_args[ARG_CMD_NOTE_POS_TL_X].val.pixit;
    note_info.offset_tl.y = p_args[ARG_CMD_NOTE_POS_TL_Y].val.pixit;

    switch(p_args[ARG_CMD_NOTE_MOUNT].val.s32)
    {
    case EXT_ID_NOTE_PIN_CELLS_SINGLE:
        {
        SLR tl;

        layer = at_front ? LAYER_CELLS_AREA_ABOVE : LAYER_CELLS_AREA_BELOW;
        note_info.note_pinning = NOTE_PIN_CELLS_SINGLE;

        PTR_ASSERT(p_of_ip_format);
        of_load_S32_arg_offset_as_COL(p_of_ip_format, &p_args[6]);
        of_load_S32_arg_offset_as_ROW(p_of_ip_format, &p_args[7]);
        tl.col = p_args[6].val.col;
        tl.row = p_args[7].val.row;

        region_from_two_slrs(&note_info.region, &tl, &tl, 1);

        note_info.pixit_size.cx = p_args[8].val.pixit;
        note_info.pixit_size.cy = p_args[9].val.pixit;

        break;
        }

    case EXT_ID_NOTE_PIN_CELLS_TWIN:
        {
        SLR tl, br;

        layer = at_front ? LAYER_CELLS_AREA_ABOVE : LAYER_CELLS_AREA_BELOW;
        note_info.note_pinning = NOTE_PIN_CELLS_TWIN;

        PTR_ASSERT(p_of_ip_format);
        of_load_S32_arg_offset_as_COL(p_of_ip_format, &p_args[6]);
        of_load_S32_arg_offset_as_ROW(p_of_ip_format, &p_args[7]);
        tl.col = p_args[6].val.col;
        tl.row = p_args[7].val.row;

        note_info.offset_br.x   = p_args[8].val.pixit;
        note_info.offset_br.y   = p_args[9].val.pixit;

        of_load_S32_arg_offset_as_COL(p_of_ip_format, &p_args[10]);
        of_load_S32_arg_offset_as_ROW(p_of_ip_format, &p_args[11]);
        br.col = p_args[10].val.col;
        br.row = p_args[11].val.row;

        region_from_two_slrs(&note_info.region, &tl, &br, 1);

        break;
        }

    default: default_unhandled();
#if CHECKING
    case EXT_ID_NOTE_PIN_CELLS_AREA:
#endif
        layer = at_front ? LAYER_CELLS_AREA_ABOVE : LAYER_CELLS_AREA_BELOW;
        note_info.note_pinning = NOTE_UNPINNED;
        goto ext_slot_print_paper;

    case EXT_ID_NOTE_PIN_PRINT_AREA:
        layer = at_front ? LAYER_PRINT_AREA_ABOVE : LAYER_PRINT_AREA_BELOW;
        note_info.note_pinning = NOTE_UNPINNED;
        goto ext_slot_print_paper;

    case EXT_ID_NOTE_PIN_PAPER:
        layer = at_front ? LAYER_PAPER_ABOVE : LAYER_PAPER_BELOW;
        note_info.note_pinning = NOTE_UNPINNED;

    ext_slot_print_paper:;
        note_info.pixit_size.cx = p_args[6].val.pixit;
        note_info.pixit_size.cy = p_args[7].val.pixit;

        note_info.flags.scale_to_fit = p_args[8].val.fBool;
        note_info.flags.all_pages    = p_args[9].val.fBool;

        if(arg_is_present(p_args, 10)) /* SKS after 1.05 24oct93 use args for page number */
            note_info.skel_rect.tl.page_num.x = p_args[10].val.s32;

        if(arg_is_present(p_args, 11))
            note_info.skel_rect.tl.page_num.y = p_args[11].val.s32;
        break;
    }

    note_info.layer = layer;

    if(arg_is_present(p_args, ARG_CMD_NOTE_DONT_PRINT))
        note_info.flags.dont_print = p_args[ARG_CMD_NOTE_DONT_PRINT].val.fBool;

    /* SKS 21jul95 added to sort out duff charts that reappear in very old docs */
    if( note_info.pixit_size.cx < 0)
        note_info.pixit_size.cx = -note_info.pixit_size.cx;
    if( note_info.pixit_size.cy < 0)
        note_info.pixit_size.cy = -note_info.pixit_size.cy;

    /* As of 2.01, these new uniformly-saved args take precedence */
    if(arg_is_present(p_args, ARG_CMD_NOTE_SIZE_X))
        note_info.pixit_size.cx = p_args[ARG_CMD_NOTE_SIZE_X].val.pixit;
    if(arg_is_present(p_args, ARG_CMD_NOTE_SIZE_Y))
        note_info.pixit_size.cy = p_args[ARG_CMD_NOTE_SIZE_Y].val.pixit;

    if(OBJECT_ID_NONE != note_info.object_id)
    {
        NOTE_REF note_ref;

        note_ref.extref = p_args[ARG_CMD_NOTE_CONTENTS].val.s32;
        assert(p_t5_cmd->p_of_ip_format);
        note_ref.p_of_ip_format = p_of_ip_format;

        if(status_ok(object_call_id(note_info.object_id, p_docu, T5_MSG_NOTE_LOAD_INTREF_FROM_EXTREF, &note_ref)))
            note_info.object_data_ref = note_ref.object_data_ref;
        else
            note_info.object_id = OBJECT_ID_NONE;
    }

    /* discard the note; it has no owner */
    if(OBJECT_ID_NONE == note_info.object_id)
        return(STATUS_OK);

    {
    GR_SCALE_PAIR gr_scale_pair = { GR_SCALE_ONE, GR_SCALE_ONE};
    BOOL scale_args_present = arg_is_present(p_args, ARG_CMD_NOTE_SCALE_X);
    BOOL scale_args_invalid;

    if(scale_args_present)
    {
        gr_scale_pair.x = p_args[ARG_CMD_NOTE_SCALE_X].val.s32;
        gr_scale_pair.y = (arg_is_present(p_args, ARG_CMD_NOTE_SCALE_Y) ? p_args[ARG_CMD_NOTE_SCALE_Y].val.s32 : gr_scale_pair.x);
    }

    scale_args_invalid = ((gr_scale_pair.x <= 0) || (gr_scale_pair.y <= 0)); /* SKS after 1.19/11 28jan95 add validation 'cos some 1.07 files are real shite */

    if(scale_args_invalid || !scale_args_present || (note_info.note_pinning == NOTE_PIN_CELLS_TWIN))
    {
        NOTE_OBJECT_SIZE note_object_size;
        note_object_size.pixit_size.cx = note_object_size.pixit_size.cy = PIXITS_PER_INCH;
        note_object_size.object_data_ref = note_info.object_data_ref;
        note_object_size.processed = 0;
        status_consume(object_call_id(note_info.object_id, p_docu, T5_MSG_NOTE_OBJECT_SIZE_QUERY, &note_object_size));
        if(note_object_size.processed)
        {
            if((note_info.note_pinning == NOTE_PIN_CELLS_SINGLE) || (note_info.note_pinning == NOTE_UNPINNED))
             {
                 gr_scale_pair.x = muldiv64(GR_SCALE_ONE, note_info.pixit_size.cx, note_object_size.pixit_size.cx);
                 gr_scale_pair.y = muldiv64(GR_SCALE_ONE, note_info.pixit_size.cy, note_object_size.pixit_size.cy);
             }
        }
        if(scale_args_invalid && (NOTE_PIN_CELLS_TWIN == note_info.note_pinning))
        {
            note_info.note_pinning = NOTE_PIN_CELLS_SINGLE;
            note_info.offset_tl.x = 0;
            note_info.offset_tl.y = 0;
            region_from_two_slrs(&note_info.region, &note_info.region.tl, &note_info.region.tl, 1);
            gr_scale_pair.x = gr_scale_pair.y = GR_SCALE_ONE;
        }
        note_info.pixit_size = note_object_size.pixit_size;
    }

    note_info.gr_scale_pair = gr_scale_pair;
    } /*block*/

    {
    STATUS status;
    P_NOTE_INFO p_note_info;
    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;

    if(NULL == (p_note_info = notelayer_new_note(p_docu, &note_info, &status)))
        return(status);

    skel_rect.tl.page_num.x = skel_rect.tl.page_num.y = 0;
    skel_rect.tl.pixit_point.x = skel_rect.tl.pixit_point.y = 0;
    skel_rect.br = skel_rect.tl;

    RECT_FLAGS_CLEAR(rect_flags);
    rect_flags.extend_right_window = rect_flags.extend_down_window = 1;

    view_update_later(p_docu, redraw_tag_from_layer(p_note_info->layer), &skel_rect, rect_flags);
    } /*block*/

    return(STATUS_OK);
}

/******************************************************************************
*
* Send all the selected notes behind the cells layer
*
******************************************************************************/

_Check_return_
static S32
notelayer_back_selected_note(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    switch(p_note_info->note_selection)
    {
    case NOTE_SELECTION_NONE:
        return(0);

    default:
        {
        note_drop_object_selection(p_docu, p_note_info);

        /* repaint 'cos ears disappear and other things revealed */
        note_update_later(p_docu, p_note_info, NOTE_UPDATE_ALL);

        p_note_info->note_selection = NOTE_SELECTION_NONE;

        switch(p_note_info->layer)
        {
        default:
            /* leave LAYER_xxx_BELOW alone! */
            break;

        case LAYER_CELLS_AREA_ABOVE:
            p_note_info->layer = LAYER_CELLS_AREA_BELOW;
            break;

        case LAYER_PRINT_AREA_ABOVE:
            p_note_info->layer = LAYER_PRINT_AREA_BELOW;
            break;

        case LAYER_PAPER_ABOVE:
            p_note_info->layer = LAYER_PAPER_BELOW;
            break;
        }

        note_install_layer_handler(p_docu, p_note_info);
        return(1);
        }
    }
}

T5_CMD_PROTO(extern, t5_cmd_note_back)
{
    S32 selection_changed = 0;
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    while(--note_index >= 0)
    {
        const P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        selection_changed += notelayer_back_selected_note(p_docu, p_note_info);
    }

    if(0 != selection_changed)
    {   /* tell everyone that selection has changed */
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_NEW, P_DATA_NONE));

        caret_show_claim(p_docu, p_docu->focus_owner_old, FALSE); /* ensure focus is not in notelayer */
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* embed all the selected notes
*
******************************************************************************/

_Check_return_
static STATUS
notelayer_embed_selected_note(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    switch(p_note_info->note_selection)
    {
    default:
        return(STATUS_OK);

    case NOTE_SELECTION_SELECTED:
        {
        NOTE_ENSURE_EMBEDDED note_ensure_embedded;
        note_ensure_embedded.object_data_ref = p_note_info->object_data_ref;
        return(object_call_id(p_note_info->object_id, p_docu, T5_MSG_NOTE_ENSURE_EMBEDED, &note_ensure_embedded));
        }
    }
}

T5_CMD_PROTO(extern, t5_cmd_note_embed)
{
    ARRAY_INDEX note_index;

    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    for(note_index = 0; note_index < array_elements(&p_docu->h_note_list); ++note_index)
    {
        const P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        status_return(notelayer_embed_selected_note(p_docu, p_note_info));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* Swap front and back layers
*
******************************************************************************/

_Check_return_
static S32
notelayer_swap_this_note(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info)
{
    S32 selection_changed = 0;

    note_update_later(p_docu, p_note_info, NOTE_UPDATE_ALL);

    if(NOTE_SELECTION_NONE != p_note_info->note_selection)
    {
        note_drop_object_selection(p_docu, p_note_info);

        p_note_info->note_selection = NOTE_SELECTION_NONE;

        selection_changed = 1;
    }

    switch(p_note_info->layer)
    {
    default: default_unhandled();
#if CHECKING
    case LAYER_CELLS_AREA_ABOVE:
#endif
        p_note_info->layer = LAYER_CELLS_AREA_BELOW;
        break;

    case LAYER_CELLS_AREA_BELOW:
        p_note_info->layer = LAYER_CELLS_AREA_ABOVE;
        break;

    case LAYER_PRINT_AREA_ABOVE:
        p_note_info->layer = LAYER_PRINT_AREA_BELOW;
        break;

    case LAYER_PRINT_AREA_BELOW:
        p_note_info->layer = LAYER_PRINT_AREA_ABOVE;
        break;

    case LAYER_PAPER_ABOVE:
        p_note_info->layer = LAYER_PAPER_BELOW;
        break;

    case LAYER_PAPER_BELOW:
        p_note_info->layer = LAYER_PAPER_ABOVE;
        break;
    }

    note_install_layer_handler(p_docu, p_note_info);

    return(selection_changed);
}

T5_CMD_PROTO(extern, t5_cmd_note_swap)
{
    S32 selection_changed = 0;
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    while(--note_index >= 0)
    {
        const P_NOTE_INFO p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        selection_changed += notelayer_swap_this_note(p_docu, p_note_info);
    }

    if(0 != selection_changed)
    {   /* tell everyone that selection has changed */
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_NEW, P_DATA_NONE));

        caret_show_claim(p_docu, p_docu->focus_owner_old, FALSE); /* ensure focus is not in notelayer */
    }

    return(STATUS_OK);
}

/*
offsets in args_cmd_backdrop[]
*/

enum ARG_CMD_BACKDROP
{
    ARG_CMD_BACKDROP_MOUNT        = 0,
    ARG_CMD_BACKDROP_SCALE_TO_FIT = 1,
    ARG_CMD_BACKDROP_ALL_PAGES    = 2,
    ARG_CMD_BACKDROP_DOES_PRINT   = 3,

    ARG_CMD_BACKDROP_N_ARGS       = 4,
};

enum BACKDROP_CONTROL_IDS
{
#define BACKDROP_OK_H DIALOG_STDRADIO_H + DIALOG_SYSCHARS_H("Remove")

    BACKDROP_ID_PAGE_GROUP = 120,
    BACKDROP_ID_PAGE_FIRST,
    BACKDROP_ID_PAGE_ALL,
#define BACKDROP_PAGES_H DIALOG_STDRADIO_H + DIALOG_RADIOGAP_H + DIALOG_SYSCHARS_H("First page")

    BACKDROP_ID_ORIGIN_GROUP = 130,
    BACKDROP_ID_ORIGIN_WORK_AREA,
    BACKDROP_ID_ORIGIN_PRINT_AREA,
    BACKDROP_ID_ORIGIN_PAPER,
#define BACKDROP_ORIGINS_H DIALOG_STDRADIO_H + DIALOG_RADIOGAP_H + DIALOG_SYSCHARS_H("Printable area")

    BACKDROP_ID_DOES_PRINT = 140,
    BACKDROP_ID_SCALE_FIT
#define BACKDROP_RHS_FIELDS_H DIALOG_STDCHECK_H + DIALOG_CHECKGAP_H + DIALOG_SYSCHARS_H("Scale to fit")
};

static const DIALOG_CONTROL
backdrop_page_group =
{
    BACKDROP_ID_PAGE_GROUP, DIALOG_CONTROL_WINDOW,

    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },

    { 0 },

    { DRT(LTRB, GROUPBOX), 0, 1 /* logical_group*/ }
};

static const DIALOG_CONTROL
backdrop_page_first =
{
    BACKDROP_ID_PAGE_FIRST, BACKDROP_ID_PAGE_GROUP,

    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },

    { 0, 0, BACKDROP_PAGES_H, DIALOG_STDRADIO_V },

    { DRT(LTLT, RADIOBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
backdrop_page_first_data = { { 0 }, 0 /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_BACKDROP_PAGE_FIRST) };

static const DIALOG_CONTROL
backdrop_page_all =
{
    BACKDROP_ID_PAGE_ALL, BACKDROP_ID_PAGE_GROUP,

    { BACKDROP_ID_PAGE_FIRST, BACKDROP_ID_PAGE_FIRST, BACKDROP_ID_PAGE_FIRST },

    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDRADIO_V },

    { DRT(LBRT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
backdrop_page_all_data = { { 0 }, 1 /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_BACKDROP_PAGE_ALL) };

static const DIALOG_CONTROL
backdrop_origin_group =
{
    BACKDROP_ID_ORIGIN_GROUP, DIALOG_CONTROL_WINDOW,

    { BACKDROP_ID_PAGE_GROUP, BACKDROP_ID_PAGE_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },

    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },

    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
backdrop_origin_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_BACKDROP_ORIGIN), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
backdrop_origin_work_area =
{
    BACKDROP_ID_ORIGIN_WORK_AREA, BACKDROP_ID_ORIGIN_GROUP,

    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },

    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, BACKDROP_ORIGINS_H, DIALOG_STDRADIO_V },

    { DRT(LTLT, RADIOBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
backdrop_origin_work_area_data = { { 0 }, EXT_ID_NOTE_PIN_CELLS_AREA /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_BACKDROP_ORIGIN_WORK_AREA) };

static const DIALOG_CONTROL
backdrop_origin_print_area =
{
    BACKDROP_ID_ORIGIN_PRINT_AREA, BACKDROP_ID_ORIGIN_GROUP,

    { BACKDROP_ID_ORIGIN_WORK_AREA, BACKDROP_ID_ORIGIN_WORK_AREA, BACKDROP_ID_ORIGIN_WORK_AREA },

    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDRADIO_V },

    { DRT(LBRT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
backdrop_origin_print_area_data = { { 0 }, EXT_ID_NOTE_PIN_PRINT_AREA /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_BACKDROP_ORIGIN_PRINT_AREA) };

static const DIALOG_CONTROL
backdrop_origin_paper =
{
    BACKDROP_ID_ORIGIN_PAPER, BACKDROP_ID_ORIGIN_GROUP,

    { BACKDROP_ID_ORIGIN_PRINT_AREA, BACKDROP_ID_ORIGIN_PRINT_AREA, BACKDROP_ID_ORIGIN_PRINT_AREA },

    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDRADIO_V },

    { DRT(LBRT, RADIOBUTTON) }
};

static DIALOG_CONTROL_DATA_RADIOBUTTON
backdrop_origin_paper_data = { { 0 }, EXT_ID_NOTE_PIN_PAPER /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_BACKDROP_ORIGIN_PAPER) };

static const DIALOG_CONTROL
backdrop_does_print =
{
    BACKDROP_ID_DOES_PRINT, DIALOG_CONTROL_WINDOW,

    { BACKDROP_ID_PAGE_GROUP, BACKDROP_ID_PAGE_GROUP },

    { DIALOG_STDSPACING_H, 0, BACKDROP_RHS_FIELDS_H, DIALOG_STDCHECK_V },

    { DRT(RTLT, CHECKBOX), 1 }
};

static DIALOG_CONTROL_DATA_CHECKBOX
backdrop_does_print_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_BACKDROP_PRINT) };

static const DIALOG_CONTROL
backdrop_scale_fit =
{
    BACKDROP_ID_SCALE_FIT, DIALOG_CONTROL_WINDOW,

    { BACKDROP_ID_DOES_PRINT, BACKDROP_ID_DOES_PRINT, BACKDROP_ID_DOES_PRINT },

    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCHECK_V },

    { DRT(LBRT, CHECKBOX), 1 }
};

static DIALOG_CONTROL_DATA_CHECKBOX
backdrop_scale_fit_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_BACKDROP_FIT) };

static const DIALOG_CONTROL
backdrop_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,

    { BACKDROP_ID_ORIGIN_GROUP, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF, BACKDROP_ID_ORIGIN_GROUP },

    { DIALOG_STDSPACING_H, DIALOG_DEFPUSHBUTTON_V, BACKDROP_OK_H, 0 },

    { DRT(RBLB, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_ID
backdrop_ok_data_argmap[] =
{
    BACKDROP_ID_ORIGIN_GROUP,
    BACKDROP_ID_SCALE_FIT,
    BACKDROP_ID_PAGE_GROUP,
    BACKDROP_ID_DOES_PRINT
};

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
backdrop_ok_command = { T5_CMD_BACKDROP, OBJECT_ID_SKEL, NULL, backdrop_ok_data_argmap, { 0, 0, 0, 1 /*lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
backdrop_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_OK), &backdrop_ok_command };

static const DIALOG_CONTROL
backdrop_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,

    { IDOK, DIALOG_CONTROL_SELF, IDOK, IDOK },

    { -DIALOG_DEFPUSHEXTRA_H, DIALOG_STDPUSHBUTTON_V, -DIALOG_DEFPUSHEXTRA_H, DIALOG_STDSPACING_V },

    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CTL_CREATE
backdrop_ctl_create[] =
{
    { &backdrop_page_group },
    { &backdrop_page_first,        &backdrop_page_first_data },
    { &backdrop_page_all,          &backdrop_page_all_data   },

    { &backdrop_origin_group,      &backdrop_origin_group_data      },
    { &backdrop_origin_work_area,  &backdrop_origin_work_area_data  },
    { &backdrop_origin_print_area, &backdrop_origin_print_area_data },
    { &backdrop_origin_paper,      &backdrop_origin_paper_data      },

    { &backdrop_does_print,        &backdrop_does_print_data },
    { &backdrop_scale_fit,         &backdrop_scale_fit_data  },

    { &backdrop_cancel,            &stdbutton_cancel_data },
    { &backdrop_ok,                &backdrop_ok_data }
};

typedef struct BACKDROP_CALLBACK
{
    S32 selected_mount;
    BOOL all_pages;
}
BACKDROP_CALLBACK, * P_BACKDROP_CALLBACK;

_Check_return_
static STATUS
dialog_backdrop_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    const P_BACKDROP_CALLBACK p_backdrop_callback = (P_BACKDROP_CALLBACK) p_dialog_msg_ctl_create_state->client_handle;

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case BACKDROP_ID_ORIGIN_GROUP:
        p_dialog_msg_ctl_create_state->state_set.state.radiobutton = p_backdrop_callback->selected_mount;
        break;

    case BACKDROP_ID_PAGE_GROUP:
        p_dialog_msg_ctl_create_state->state_set.state.radiobutton = p_backdrop_callback->all_pages;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_backdrop)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_backdrop_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(extern, t5_cmd_backdrop_intro)
{
    P_NOTE_INFO p_note_info = NULL;
    S32 n_selected = 0;
    S32 n_behind = 0;
    BACKDROP_CALLBACK backdrop_callback;

    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    { /* operate on selected note or backdrop */
    ARRAY_INDEX i = array_elements(&p_docu->h_note_list);

    while(--i >= 0)
    {
        P_NOTE_INFO this_p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, i);

        if(NOTE_SELECTION_NONE != this_p_note_info->note_selection)
        {
            ++n_selected;

            p_note_info = this_p_note_info;
        }

        switch(this_p_note_info->layer)
        {
        default:
            break;

        case LAYER_PAPER_BELOW:
        case LAYER_PRINT_AREA_BELOW:
        case LAYER_CELLS_AREA_BELOW:
            if(NOTE_UNPINNED == this_p_note_info->note_pinning)
            {
                ++n_behind;

                if(NULL == p_note_info)
                    p_note_info = this_p_note_info;

                break; /*???*/
            }
        }
    }
    } /*block*/

    if((n_selected != 1) && (n_behind != 1))
    {
        host_bleep();
        return(STATUS_OK);
    }

    switch(p_note_info->note_pinning)
    {
    default: default_unhandled();
#if CHECKING
    case NOTE_PIN_CELLS_SINGLE:
    case NOTE_PIN_CELLS_TWIN:
#endif
        backdrop_callback.selected_mount = EXT_ID_NOTE_PIN_CELLS_AREA;
        break;

    case NOTE_UNPINNED:
        {
        switch(p_note_info->layer)
        {
        default: default_unhandled();
#if CHECKING
        case LAYER_CELLS_AREA_ABOVE:
        case LAYER_CELLS_AREA_BELOW:
#endif
            backdrop_callback.selected_mount = EXT_ID_NOTE_PIN_CELLS_AREA;
            break;

        case LAYER_PRINT_AREA_ABOVE:
        case LAYER_PRINT_AREA_BELOW:
            backdrop_callback.selected_mount = EXT_ID_NOTE_PIN_PRINT_AREA;
            break;

        case LAYER_PAPER_ABOVE:
        case LAYER_PAPER_BELOW:
            backdrop_callback.selected_mount = EXT_ID_NOTE_PIN_PAPER;
            break;
        }

        break;
        }
    }

    backdrop_callback.all_pages = p_note_info->flags.all_pages;

    backdrop_scale_fit_data.init_state = (U8) p_note_info->flags.scale_to_fit;
    backdrop_does_print_data.init_state = (U8) !p_note_info->flags.dont_print;

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, backdrop_ctl_create, elemof32(backdrop_ctl_create), MSG_DIALOG_BACKDROP_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_BACKDROP_CAPTION;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &backdrop_callback;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_backdrop;
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
    } /*block*/
}

T5_CMD_PROTO(extern, t5_cmd_backdrop)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, ARG_CMD_BACKDROP_N_ARGS);
    S32 selected_mount = p_args[ARG_CMD_BACKDROP_MOUNT].val.s32;
    BOOL new_scale_to_fit = p_args[ARG_CMD_BACKDROP_SCALE_TO_FIT].val.fBool;
    BOOL new_all_pages = (p_args[ARG_CMD_BACKDROP_ALL_PAGES].val.s32 != 0);
    BOOL new_does_print = p_args[ARG_CMD_BACKDROP_DOES_PRINT].val.fBool;
    P_NOTE_INFO p_note_info = NULL;
    S32 n_selected = 0;
    S32 n_behind = 0;

    IGNOREPARM_InVal_(t5_message);

    { /* operate on selected note or backdrop */
    ARRAY_INDEX note_index = array_elements(&p_docu->h_note_list);

    while(--note_index >= 0)
    {
        const P_NOTE_INFO this_p_note_info = array_ptr(&p_docu->h_note_list, NOTE_INFO, note_index);

        if(this_p_note_info->note_selection != NOTE_SELECTION_NONE)
        {
            ++n_selected;

            p_note_info = this_p_note_info;
        }

        switch(this_p_note_info->layer)
        {
        default:
            break;

        case LAYER_PAPER_BELOW:
        case LAYER_PRINT_AREA_BELOW:
        case LAYER_CELLS_AREA_BELOW:
            if(this_p_note_info->note_pinning == NOTE_UNPINNED)
            {
                ++n_behind;

                if(NULL == p_note_info)
                    p_note_info = this_p_note_info;

                break; /*???*/
            }
        }
    }
    } /*block*/

    if((n_selected != 1) && (n_behind != 1))
    {
        host_bleep();
        return(STATUS_OK);
    }

    note_update_later(p_docu, p_note_info, NOTE_UPDATE_ALL);

    note_uref_unlink(p_docu, p_note_info);

    switch(selected_mount)
    {
    default: default_unhandled();
#if CHECKING
    case EXT_ID_NOTE_PIN_CELLS_AREA:
#endif
        p_note_info->layer = LAYER_CELLS_AREA_BELOW;
        break;

    case EXT_ID_NOTE_PIN_PRINT_AREA:
        p_note_info->layer = LAYER_PRINT_AREA_BELOW;
        break;

    case EXT_ID_NOTE_PIN_PAPER:
        p_note_info->layer = LAYER_PAPER_BELOW;
        break;
    }

    note_install_layer_handler(p_docu, p_note_info);

    p_note_info->note_pinning = NOTE_UNPINNED;

    switch(p_note_info->note_selection)
    {
    case NOTE_SELECTION_NONE:
        break;

    default:
        {
        note_drop_object_selection(p_docu, p_note_info);

        p_note_info->note_selection = NOTE_SELECTION_NONE;

        /* tell everyone that selection has changed */
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_NEW, P_DATA_NONE));

        caret_show_claim(p_docu, p_docu->focus_owner_old, FALSE); /* ensure focus is not in notelayer */
        break;
        }
    }

    p_note_info->flags.scale_to_fit = new_scale_to_fit;
    p_note_info->flags.all_pages = new_all_pages;
    p_note_info->flags.dont_print = !new_does_print;

    notelayer_mount_note(p_docu, p_note_info);

    note_update_later(p_docu, p_note_info, NOTE_UPDATE_ALL);

    return(STATUS_OK);
}

extern void
note_move(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info,
    _InRef_     PC_SKEL_POINT p_skel_point,
    _InRef_     PC_PIXIT_SIZE p_pixit_size)
{
    SKEL_POINT tl_skel_point = *p_skel_point;
    const REDRAW_TAG redraw_tag = redraw_tag_from_layer(p_note_info->layer);

    skel_point_normalise(p_docu, &tl_skel_point, redraw_tag);

    /* cause a redraw of where we were */
    note_update_later(p_docu, p_note_info, NOTE_UPDATE_ALL);

    p_note_info->flags.skel_rect_valid = 0;

    note_uref_unlink(p_docu, p_note_info);

    switch(p_note_info->note_pinning)
    {
    default: default_unhandled();
#if CHECKING
    case NOTE_UNPINNED:
#endif
        p_note_info->skel_rect.tl.page_num = tl_skel_point.page_num;
        p_note_info->offset_tl = tl_skel_point.pixit_point;
        p_note_info->offset_br.x = 0;
        p_note_info->offset_br.y = 0;
        break;

    case NOTE_PIN_CELLS_SINGLE:
    case NOTE_PIN_CELLS_TWIN:
        {
        SLR tl_slr;
        SKEL_POINT tl_slot_skel_point;

        status_assert(slr_owner_from_skel_point(p_docu, &tl_slr, &tl_slot_skel_point, &tl_skel_point, ON_ROW_EDGE_GO_DOWN));

        /*>>>what happens if slr_owner_from_skel_point fails?, change to different pinning???*/

        p_note_info->offset_tl.x = 0;
        p_note_info->offset_tl.y = 0;
        p_note_info->offset_br.x = 0;
        p_note_info->offset_br.y = 0;

        if((tl_skel_point.page_num.x == tl_slot_skel_point.page_num.x) && (tl_skel_point.page_num.y == tl_slot_skel_point.page_num.y))
        {
            p_note_info->offset_tl.x = tl_skel_point.pixit_point.x - tl_slot_skel_point.pixit_point.x;
            p_note_info->offset_tl.y = tl_skel_point.pixit_point.y - tl_slot_skel_point.pixit_point.y;
        }

        if(NOTE_PIN_CELLS_TWIN == p_note_info->note_pinning)
        {
            SKEL_POINT br_skel_point;
            SLR br_slr;
            SKEL_POINT br_slot_skel_point;

            br_skel_point = tl_slot_skel_point; /* offset from where we ended up */
            br_skel_point.pixit_point.x += p_note_info->offset_tl.x;
            br_skel_point.pixit_point.y += p_note_info->offset_tl.y;
            br_skel_point.pixit_point.x += p_pixit_size->cx;
            br_skel_point.pixit_point.y += p_pixit_size->cy;

            if(status_fail(slr_owner_from_skel_point(p_docu, &br_slr, &br_slot_skel_point, &br_skel_point, ON_ROW_EDGE_GO_DOWN)))
            {
                p_note_info->note_pinning = NOTE_PIN_CELLS_SINGLE;
            }
            else if((br_skel_point.page_num.x != br_slot_skel_point.page_num.x) || (br_skel_point.page_num.y != br_slot_skel_point.page_num.y))
            {
                p_note_info->note_pinning = NOTE_PIN_CELLS_SINGLE;
            }
            else
            {
                p_note_info->offset_br.x = br_skel_point.pixit_point.x - br_slot_skel_point.pixit_point.x;
                p_note_info->offset_br.y = br_skel_point.pixit_point.y - br_slot_skel_point.pixit_point.y;

                region_from_two_slrs(&p_note_info->region, &tl_slr, &br_slr, 1);
            }
        }

        if(NOTE_PIN_CELLS_SINGLE == p_note_info->note_pinning)
        {
            region_from_two_slrs(&p_note_info->region, &tl_slr, &tl_slr, 1);
        }

        break;
        }
    }

    status_assert(note_uref_link(p_docu, p_note_info));

    /* cause a redraw of where we are now */
    note_update_later(p_docu, p_note_info, NOTE_UPDATE_ALL);
}

extern void
note_pin_change(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info,
    _In_        NOTE_PINNING new_note_pinning)
{
    if(p_note_info->note_pinning == new_note_pinning)
        return;

    note_update_later(p_docu, p_note_info, NOTE_UPDATE_ALL);

    switch(new_note_pinning)
    {
    default: default_unhandled();
#if CHECKING
    case NOTE_UNPINNED:
#endif
        {
        switch(p_note_info->note_pinning)
        {
        default: default_unhandled();
        case NOTE_UNPINNED:
            return;

        case NOTE_PIN_CELLS_SINGLE:
            { /* need to find the current cell tl and offset by that as we're going work area relative */
            SLR tl_slr = p_note_info->region.tl;
            SKEL_POINT tl_slot_skel_point;

            skel_point_from_slr_tl(p_docu, &tl_slot_skel_point, &tl_slr);

            p_note_info->offset_tl.x += tl_slot_skel_point.pixit_point.x;
            p_note_info->offset_tl.y += tl_slot_skel_point.pixit_point.y;

            /*p_note_info->region = REGION_INIT;*/
            break;
            }

        case NOTE_PIN_CELLS_TWIN:
            { /* need to find the current cell tl and offset by that as we're going work area relative */
            SLR tl_slr = p_note_info->region.tl;
            SKEL_POINT tl_slot_skel_point;

            skel_point_from_slr_tl(p_docu, &tl_slot_skel_point, &tl_slr);

            p_note_info->offset_tl.x += tl_slot_skel_point.pixit_point.x;
            p_note_info->offset_tl.y += tl_slot_skel_point.pixit_point.y;

            /* <<< */
            /*p_note_info->region = REGION_INIT;*/
            break;
            }
        }

        break;
        }

    case NOTE_PIN_CELLS_SINGLE:
        {
        switch(p_note_info->note_pinning)
        {
        case NOTE_UNPINNED:
            { /* need to find the cell containing the current work area relative tl and offset by that as we're going cell relative */
            SKEL_POINT skel_point = p_note_info->skel_rect.tl;
            SLR tl_slr;
            SKEL_POINT tl_slot_skel_point;

            if(status_fail(slr_owner_from_skel_point(p_docu, &tl_slr, &tl_slot_skel_point, &skel_point, ON_ROW_EDGE_GO_DOWN)))
                return;

            p_note_info->offset_tl.x = 0;
            p_note_info->offset_tl.y = 0;

            /* if the cell is on the same page, calc the offset from the cell's tl */
            if((tl_slot_skel_point.page_num.x == skel_point.page_num.x) && (tl_slot_skel_point.page_num.y == skel_point.page_num.y))
            {
                p_note_info->offset_tl.x = skel_point.pixit_point.x - tl_slot_skel_point.pixit_point.x;
                p_note_info->offset_tl.y = skel_point.pixit_point.y - tl_slot_skel_point.pixit_point.y;
            }

            region_from_two_slrs(&p_note_info->region, &tl_slr, &tl_slr, 1);

            break;
            }

        default: default_unhandled();
        case NOTE_PIN_CELLS_SINGLE:
            return;

        case NOTE_PIN_CELLS_TWIN:
            {
            /*p_note_info->region.br = SLR_INIT;*/
            break;
            }
        }

        break;
        }

    case NOTE_PIN_CELLS_TWIN:
        {
        switch(p_note_info->note_pinning)
        {
        case NOTE_UNPINNED:
            assert0();
            return;

        case NOTE_PIN_CELLS_SINGLE:
            { /* need to find the current br */
            SLR tl_slr = p_note_info->region.tl;
            SKEL_POINT br_skel_point;
            SLR br_slr;
            SKEL_POINT br_slot_skel_point;

            br_skel_point = p_note_info->skel_rect.br;

            if(status_fail(slr_owner_from_skel_point(p_docu, &br_slr, &br_slot_skel_point, &br_skel_point, ON_ROW_EDGE_GO_DOWN)))
                return;

            if((br_skel_point.page_num.x != br_slot_skel_point.page_num.x) || (br_skel_point.page_num.y != br_slot_skel_point.page_num.y))
                return;

            p_note_info->offset_br.x = br_skel_point.pixit_point.x - br_slot_skel_point.pixit_point.x;
            p_note_info->offset_br.y = br_skel_point.pixit_point.y - br_slot_skel_point.pixit_point.y;

            region_from_two_slrs(&p_note_info->region, &tl_slr, &br_slr, 1);
            break;
            }

        default: default_unhandled();
        case NOTE_PIN_CELLS_TWIN:
            return;
        }

        break;
        }
    }

    note_uref_unlink(p_docu, p_note_info);

    p_note_info->note_pinning = new_note_pinning;

    status_assert(note_uref_link(p_docu, p_note_info));

    notelayer_mount_note(p_docu, p_note_info);

    note_update_later(p_docu, p_note_info, NOTE_UPDATE_ALL);
}

_Check_return_
extern REDRAW_TAG
redraw_tag_from_layer(
    _InVal_     LAYER layer)
{
    switch(layer)
    {
    default: default_unhandled();
#if CHECKING
    case LAYER_CELLS_AREA_ABOVE:
    case LAYER_CELLS_AREA_BELOW:
#endif
        return(UPDATE_PANE_CELLS_AREA);

    case LAYER_PRINT_AREA_ABOVE:
    case LAYER_PRINT_AREA_BELOW:
        return(UPDATE_PANE_PRINT_AREA);

    case LAYER_PAPER_ABOVE:
    case LAYER_PAPER_BELOW:
        return(UPDATE_PANE_PAPER);
    }
}

extern void
relative_pixit_rect_from_note(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_NOTE_INFO p_note_info,
    _InRef_     PC_PAGE_NUM p_page_num,
    _OutRef_    P_PIXIT_RECT p_pixit_rect)
{
    switch(p_note_info->note_pinning)
    {
    default: default_unhandled();
#if CHECKING
    case NOTE_PIN_CELLS_SINGLE:
    case NOTE_PIN_CELLS_TWIN:
#endif
        break;

    case NOTE_UNPINNED:
        if(p_note_info->flags.all_pages)
        {
            p_pixit_rect->tl = p_note_info->skel_rect.tl.pixit_point;
            p_pixit_rect->br = p_note_info->skel_rect.br.pixit_point;
            return;
        }

        break;
    }

    relative_pixit_point_from_skel_point_in_layer(p_docu, p_page_num, &p_note_info->skel_rect.tl, &p_pixit_rect->tl, p_note_info->layer);
    relative_pixit_point_from_skel_point_in_layer(p_docu, p_page_num, &p_note_info->skel_rect.br, &p_pixit_rect->br, p_note_info->layer);
}

extern void
relative_pixit_point_from_skel_point_in_layer(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_PAGE_NUM p_page_num,
    _InRef_     PC_SKEL_POINT p_skel_point,
    _OutRef_    P_PIXIT_POINT p_pixit_point,
    _InVal_     LAYER layer)
{
    SKEL_POINT skel_point = *p_skel_point;
    PIXIT_POINT work_area;
    REDRAW_TAG redraw_tag = redraw_tag_from_layer(layer);

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

    *p_pixit_point = skel_point.pixit_point;
}

/* end of ob_note2.c */
