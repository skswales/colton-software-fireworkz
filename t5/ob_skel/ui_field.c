/* ui_field.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* UI for adding fields */

/* SKS January 1993 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

/******************************************************************************
*
* insert a field into a document
*
******************************************************************************/

typedef struct FIELD_LIST_ENTRY
{
    P_USTR ustr_numform;

    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 1); /*[sizeof32("18th September 1962")]*/ /* NB buffer adjacent for fixup */
}
FIELD_LIST_ENTRY, * P_FIELD_LIST_ENTRY;

typedef struct INSERT_FIELD_INTRO_CALLBACK
{
    P_S32 p_dead_field;
    P_S32 p_selected_field;
    T5_MESSAGE t5_message;
}
INSERT_FIELD_INTRO_CALLBACK, * P_INSERT_FIELD_INTRO_CALLBACK;

enum INSERT_FIELD_CONTROL_IDS
{
    INSERT_FIELD_ID_LIST = 333,
    INSERT_FIELD_ID_LIVE
};

static /*poked*/ DIALOG_CONTROL
insert_field_list =
{
    INSERT_FIELD_ID_LIST, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0 /*[2,3] set at runtime */},
    { DRT(LTLT, LIST_TEXT), 1 }
};

static const DIALOG_CONTROL
insert_field_live =
{
    INSERT_FIELD_ID_LIVE, DIALOG_MAIN_GROUP,
    { INSERT_FIELD_ID_LIST, INSERT_FIELD_ID_LIST, INSERT_FIELD_ID_LIST, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCHECK_V },
    { DRT(LBRT, CHECKBOX), 1 }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
insert_field_live_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_INSERT_FIELD_LIVE) };

static const DIALOG_CTL_CREATE
insert_field_date_ctl_create[] =
{
    { &dialog_main_group },

    { &insert_field_list, &stdlisttext_data },
    { &insert_field_live, &insert_field_live_data },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &defbutton_ok, &defbutton_ok_data }
};

static const DIALOG_CTL_CREATE
insert_field_date_dead_only_ctl_create[] =
{
    { &dialog_main_group },

    { &insert_field_list, &stdlisttext_data },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &defbutton_ok, &defbutton_ok_data }
};

static const DIALOG_CTL_CREATE
insert_field_page_ctl_create[] =
{
    { &dialog_main_group },

    { &insert_field_list, &stdlisttext_data },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &defbutton_ok, &defbutton_ok_data }
};

_Check_return_
static STATUS
insert_field_intro_setup_entry(
    _DocuRef_   P_DOCU p_docu,
    P_FIELD_LIST_ENTRY p_field_list_entry,
    _InVal_     T5_MESSAGE t5_message,
    PC_ANY p_setup_data,
    _InoutRef_  P_PIXIT p_max_width)
{
    EV_DATA ev_data;
    NUMFORM_PARMS numform_parms;
    STATUS status;

    zero_struct(numform_parms);

    /* each ***entry*** has a quick buf! */
    quick_ublock_with_buffer_setup(p_field_list_entry->quick_ublock);

    switch(t5_message)
    {
    case T5_CMD_INSERT_FIELD_INTRO_DATE:
        ev_data.did_num = RPN_DAT_DATE;
        ss_local_time_as_ev_date(&ev_data.arg.ev_date);
        numform_parms.ustr_numform_datetime = p_field_list_entry->ustr_numform;
        break;

    case T5_CMD_INSERT_FIELD_INTRO_FILE_DATE:
        ev_data.did_num = RPN_DAT_DATE;
        ev_data.arg.ev_date = p_docu->file_date;
        numform_parms.ustr_numform_datetime = p_field_list_entry->ustr_numform;
        break;

    case T5_CMD_INSERT_FIELD_INTRO_PAGE_X:
        ev_data_set_integer(&ev_data, ((P_PAGE_NUM) p_setup_data)->x + 1);
        numform_parms.ustr_numform_numeric = p_field_list_entry->ustr_numform;
        break;

    default: default_unhandled();
#if CHECKING
    case T5_CMD_INSERT_FIELD_INTRO_PAGE_Y:
#endif
        ev_data_set_integer(&ev_data, page_number_from_page_y(p_docu, ((P_PAGE_NUM) p_setup_data)->y) + 1);
        numform_parms.ustr_numform_numeric = p_field_list_entry->ustr_numform;
        break;
    }

    numform_parms.p_numform_context = get_p_numform_context(p_docu);

    status_assert(status = numform(&p_field_list_entry->quick_ublock, P_QUICK_TBLOCK_NONE, &ev_data, &numform_parms));

    {
    PIXIT width = ui_width_from_ustr(quick_ublock_ustr(&p_field_list_entry->quick_ublock));
    if(*p_max_width < width)
       *p_max_width = width;
    } /*block*/

    return(status);
}

static UI_SOURCE
field_list_source;

_Check_return_
static STATUS
dialog_insert_field_intro_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case INSERT_FIELD_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &field_list_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_insert_field_intro_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    const P_INSERT_FIELD_INTRO_CALLBACK p_insert_field_intro_callback = (P_INSERT_FIELD_INTRO_CALLBACK) p_dialog_msg_ctl_create_state->client_handle;

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case INSERT_FIELD_ID_LIST:
        p_dialog_msg_ctl_create_state->state_set.bits = DIALOG_STATE_SET_ALTERNATE;
        p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = *(p_insert_field_intro_callback->p_selected_field);
        break;

    case INSERT_FIELD_ID_LIVE:
        p_dialog_msg_ctl_create_state->state_set.bits = 0;
        p_dialog_msg_ctl_create_state->state_set.state.checkbox = (U8) !*(p_insert_field_intro_callback->p_dead_field);
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_insert_field_intro_process_end(
_InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_INSERT_FIELD_INTRO_CALLBACK p_insert_field_intro_callback = (P_INSERT_FIELD_INTRO_CALLBACK) p_dialog_msg_process_end->client_handle;

        *(p_insert_field_intro_callback->p_selected_field) = ui_dlg_get_list_idx(p_dialog_msg_process_end->h_dialog, INSERT_FIELD_ID_LIST);

        switch(p_insert_field_intro_callback->t5_message)
        {
        case T5_CMD_INSERT_FIELD_INTRO_DATE:
        case T5_CMD_INSERT_FIELD_INTRO_FILE_DATE:
            *(p_insert_field_intro_callback->p_dead_field) = !ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, INSERT_FIELD_ID_LIVE);
            break;

        default:
            break;
        }
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_insert_field_intro)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_insert_field_intro_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_insert_field_intro_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_insert_field_intro_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(extern, t5_cmd_insert_field_intro)
{
    static struct INSERT_FIELD_INTRO_STATICS
    {
        BOOL dead_file_date_field;
        BOOL dead_date_field;
        BOOL dead_page_field;
        S32 selected_date_field; /* SKS after 1.03 always have selection in list */
        S32 selected_page_field;
    }
    insert_field_intro =
    {
        FALSE,
        TRUE, /*after 1.19/09*/
        FALSE
    };

    PC_ARGLIST_ARG p_arg;
    S32 numeric_arg = 0;
    INSERT_FIELD_INTRO_CALLBACK insert_field_intro_callback;
    ARRAY_HANDLE field_list_handle;
    P_FIELD_LIST_ENTRY p_field_list_entry;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*p_field_list_entry), 0);
    STATUS status = STATUS_OK;
    PIXIT max_width = 0;
    T5_MESSAGE new_t5_message;
    P_ANY p_setup_data = P_DATA_NONE;
    BOOL dead_only = FALSE;

    if((0 != n_arglist_args(&p_t5_cmd->arglist_handle)) && arg_present(&p_t5_cmd->arglist_handle, 0, &p_arg))
        numeric_arg = p_arg->val.s32;

    /* strictly speaking we should enquire as to whether this is possible */
    if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
        dead_only = TRUE;

    field_list_handle = 0;
    field_list_source.type = UI_SOURCE_TYPE_NONE;

    {
    PC_ARRAY_HANDLE p_ui_numform_handle = &p_docu_from_config()->numforms;
    ARRAY_INDEX i;
    UI_NUMFORM_CLASS ui_numform_class;
    PAGE_NUM page_num;

    switch(insert_field_intro_callback.t5_message = t5_message)
    {
    case T5_CMD_INSERT_FIELD_INTRO_DATE:
        insert_field_intro_callback.p_dead_field = &insert_field_intro.dead_date_field;
        insert_field_intro_callback.p_selected_field = &insert_field_intro.selected_date_field;
        ui_numform_class = numeric_arg ? UI_NUMFORM_CLASS_TIME : UI_NUMFORM_CLASS_DATE;
        break;

    case T5_CMD_INSERT_FIELD_INTRO_FILE_DATE:
        insert_field_intro_callback.p_dead_field = &insert_field_intro.dead_file_date_field;
        insert_field_intro_callback.p_selected_field = &insert_field_intro.selected_date_field;
        ui_numform_class = numeric_arg ? UI_NUMFORM_CLASS_TIME : UI_NUMFORM_CLASS_DATE;
        break;

    default: default_unhandled();
#if CHECKING
    case T5_CMD_INSERT_FIELD_INTRO_PAGE_Y:
    case T5_CMD_INSERT_FIELD_INTRO_PAGE_X:
#endif
        insert_field_intro_callback.p_dead_field = &insert_field_intro.dead_page_field;
        insert_field_intro_callback.p_selected_field = &insert_field_intro.selected_page_field;
        ui_numform_class  = UI_NUMFORM_CLASS_PAGE;

        switch(p_docu->focus_owner)
        {
        case OBJECT_ID_HEADER:
        case OBJECT_ID_FOOTER:
            page_num = p_docu->hefo_position.page_num;
            break;

        default:
            {
            SKEL_POINT skel_point;
            skel_point_from_slr_tl(p_docu, &skel_point, &p_docu->cur.slr);
            page_num = skel_point.page_num;
            break;
            }
        }

        p_setup_data = &page_num;
        break;
    }

    for(i = 0; i < array_elements(p_ui_numform_handle); ++i)
    {
        PC_UI_NUMFORM p_ui_numform = array_ptrc(p_ui_numform_handle, UI_NUMFORM, i);

        if(p_ui_numform->numform_class != ui_numform_class)
            continue;

        if(NULL != (p_field_list_entry = al_array_extend_by(&field_list_handle, FIELD_LIST_ENTRY, 1, &array_init_block, &status)))
        {
            p_field_list_entry->ustr_numform = p_ui_numform->ustr_numform;

            status = insert_field_intro_setup_entry(p_docu, p_field_list_entry, t5_message, p_setup_data, &max_width);
        }

        status_break(status);
    }
    } /*block*/

    /* make a source of text pointers to these elements for list box processing */
    if(status_ok(status))
        status = ui_source_create_ub(&field_list_handle, &field_list_source, UI_TEXT_TYPE_USTR_PERM, offsetof32(FIELD_LIST_ENTRY, quick_ublock));

    if(status_ok(status))
    {
        UI_TEXT caption;

        caption.type = UI_TEXT_TYPE_RESID;
        switch(t5_message)
        {
        case T5_CMD_INSERT_FIELD_INTRO_DATE:
            caption.text.resource_id = numeric_arg ? MSG_DIALOG_INSERT_FIELD_TIME_CAPTION : MSG_DIALOG_INSERT_FIELD_DATE_CAPTION;
            new_t5_message = T5_CMD_FIELD_INS_DATE;
            break;

        case T5_CMD_INSERT_FIELD_INTRO_FILE_DATE:
            caption.text.resource_id = numeric_arg ? MSG_DIALOG_INSERT_FIELD_FILE_TIME_CAPTION : MSG_DIALOG_INSERT_FIELD_FILE_DATE_CAPTION;
            new_t5_message = T5_CMD_FIELD_INS_FILE_DATE;
            break;

        case T5_CMD_INSERT_FIELD_INTRO_PAGE_X:
            caption.text.resource_id = MSG_DIALOG_INSERT_FIELD_PAGE_X_CAPTION;
            new_t5_message = T5_CMD_FIELD_INS_PAGE_X;
            break;

        default: default_unhandled();
#if CHECKING
        case T5_CMD_INSERT_FIELD_INTRO_PAGE_Y:
#endif
            caption.text.resource_id = MSG_DIALOG_INSERT_FIELD_PAGE_Y_CAPTION;
            new_t5_message = T5_CMD_FIELD_INS_PAGE_Y;
            break;
        }

        { /* make appropriate size box */
        const PIXIT buttons_width = DIALOG_DEFOK_H + DIALOG_STDSPACING_H + DIALOG_STDCANCEL_H;
        const PIXIT caption_width = ui_width_from_p_ui_text(&caption) + DIALOG_CAPTIONOVH_H;
        PIXIT_SIZE list_size;
        DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
        dialog_cmd_ctl_size_estimate.p_dialog_control = &insert_field_list;
        dialog_cmd_ctl_size_estimate.p_dialog_control_data = &stdlisttext_data;
        ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
        dialog_cmd_ctl_size_estimate.size.x += max_width;
        ui_list_size_estimate(array_elements(&field_list_handle), &list_size);
        dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
        dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
        dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, buttons_width);
        dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, caption_width);
        insert_field_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
        insert_field_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
        } /*block*/

        {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        zero_struct(dialog_cmd_process_dbox);
        dialog_cmd_process_dbox.caption = caption;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_insert_field_intro;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &insert_field_intro_callback;
        switch(t5_message)
        {
        case T5_CMD_INSERT_FIELD_INTRO_DATE:
        case T5_CMD_INSERT_FIELD_INTRO_FILE_DATE:
            if(dead_only)
            {
                dialog_cmd_process_dbox.n_ctls = elemof32(insert_field_date_dead_only_ctl_create);
                dialog_cmd_process_dbox.p_ctl_create  = insert_field_date_dead_only_ctl_create;
            }
            else
            {
                dialog_cmd_process_dbox.n_ctls = elemof32(insert_field_date_ctl_create);
                dialog_cmd_process_dbox.p_ctl_create  = insert_field_date_ctl_create;
            }
            break;

        default:
            dialog_cmd_process_dbox.n_ctls = elemof32(insert_field_page_ctl_create);
            dialog_cmd_process_dbox.p_ctl_create  = insert_field_page_ctl_create;
            break;
        }
        status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        } /*block*/

        if(DIALOG_COMPLETION_OK == status)
        {
            /* selected field to use NB. UI list unsorted so maps directly onto field list index */
            P_FIELD_LIST_ENTRY p_field_list_entry = array_ptr(&field_list_handle, FIELD_LIST_ENTRY, *(insert_field_intro_callback.p_selected_field));

            if(dead_only || *(insert_field_intro_callback.p_dead_field))
            {
                /* simply pipe in UI representation */
                PC_USTR ustr = ui_text_ustr(array_ptrc(&field_list_source.source.array_handle, UI_TEXT, *(insert_field_intro_callback.p_selected_field)));
                SKELEVENT_KEYS skelevent_keys;
                QUICK_UBLOCK quick_ublock;
                quick_ublock_setup_fill_from_const_ubuf(&quick_ublock, ustr, ustrlen32(ustr));

                skelevent_keys.p_quick_ublock = &quick_ublock;
                skelevent_keys.processed = 0;
                status = object_skel(p_docu, T5_EVENT_KEYS, &skelevent_keys);
                docu_modify(p_docu);
            }
            else
            {
                const OBJECT_ID object_id = OBJECT_ID_SKEL;
                PC_CONSTRUCT_TABLE p_construct_table;
                ARGLIST_HANDLE arglist_handle;

                if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, new_t5_message, &p_construct_table)))
                {
                    const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
                    p_args[0].val.ustr = p_field_list_entry->ustr_numform;
                    status = execute_command(object_id, p_docu, new_t5_message, &arglist_handle);
                    arglist_dispose(&arglist_handle);
                }
            }
        }
    }

    ui_lists_dispose_ub(&field_list_handle, &field_list_source, offsetof32(FIELD_LIST_ENTRY, quick_ublock));

    return(status);
}

/* end of ui_field.c */
