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
    T5_MESSAGE t5_message;
    BOOL dead_only; /* e.g. inserting into database */

    P_S32 p_selector; /* radio button */
    P_S32 p_selected_field[2]; /* list */
    P_S32 p_dead_field[2]; /* check box */
}
INSERT_FIELD_INTRO_CALLBACK, * P_INSERT_FIELD_INTRO_CALLBACK;

enum INSERT_FIELD_CONTROL_IDS
{
    INSERT_FIELD_ID_SELECTOR_GROUP = 333,
    INSERT_FIELD_ID_SELECTOR_0,
    INSERT_FIELD_ID_SELECTOR_1,
    INSERT_FIELD_ID_LIST,
    INSERT_FIELD_ID_LIVE
};

static const DIALOG_CONTROL
insert_field_selector_group =
{
    INSERT_FIELD_ID_SELECTOR_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0 },
    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
insert_field_selector_0 =
{
    INSERT_FIELD_ID_SELECTOR_0, INSERT_FIELD_ID_SELECTOR_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
insert_field_date_selector_0_data = { { 0 }, 0, UI_TEXT_INIT_RESID(MSG_DIALOG_INSERT_FIELD_FILE_DATE_LABEL) };

static const DIALOG_CONTROL_DATA_RADIOBUTTON
insert_field_page_selector_0_data = { { 0 }, 0, UI_TEXT_INIT_RESID(MSG_DIALOG_INSERT_FIELD_PAGE_Y_LABEL) };

static const DIALOG_CONTROL
insert_field_selector_1 =
{
    INSERT_FIELD_ID_SELECTOR_1, INSERT_FIELD_ID_SELECTOR_GROUP,
    { INSERT_FIELD_ID_SELECTOR_0, INSERT_FIELD_ID_SELECTOR_0 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
insert_field_date_selector_1_data = { { 0 }, 1, UI_TEXT_INIT_RESID(MSG_DIALOG_INSERT_FIELD_DATE_LABEL) };

static const DIALOG_CONTROL_DATA_RADIOBUTTON
insert_field_page_selector_1_data = { { 0 }, 1, UI_TEXT_INIT_RESID(MSG_DIALOG_INSERT_FIELD_PAGE_X_LABEL) };

static /*poked*/ DIALOG_CONTROL
insert_field_list =
{
    INSERT_FIELD_ID_LIST, DIALOG_MAIN_GROUP,
    { INSERT_FIELD_ID_SELECTOR_GROUP, INSERT_FIELD_ID_SELECTOR_GROUP },
    { 0, DIALOG_STDSPACING_V /*[2,3] set at runtime */},
    { DRT(LBLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static /*poked*/ DIALOG_CONTROL
insert_field_list_dt =
{
    INSERT_FIELD_ID_LIST, DIALOG_MAIN_GROUP,
#if WINDOWS /* action buttons are so much wider */
    { INSERT_FIELD_ID_SELECTOR_GROUP, INSERT_FIELD_ID_SELECTOR_GROUP },
    { 0, DIALOG_STDSPACING_V /*[2,3] set at runtime */},
    { DRT(LBLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
#else
    { INSERT_FIELD_ID_SELECTOR_GROUP, INSERT_FIELD_ID_SELECTOR_GROUP, INSERT_FIELD_ID_SELECTOR_GROUP },
    { 0, DIALOG_STDSPACING_V, 0 /*[3] set at runtime */},
    { DRT(LBRT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
#endif
};

static const DIALOG_CONTROL
insert_field_live =
{
    INSERT_FIELD_ID_LIVE, DIALOG_MAIN_GROUP,
    { INSERT_FIELD_ID_LIST, INSERT_FIELD_ID_LIST, INSERT_FIELD_ID_LIST, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCHECK_V },
    { DRT(LBRT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
insert_field_live_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_INSERT_FIELD_LIVE) };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
insert_field_insert_data = { { DIALOG_COMPLETION_OK }, UI_TEXT_INIT_RESID(MSG_INSERT) };

static const DIALOG_CTL_CREATE
insert_field_date_ctl_create[] =
{
    { &dialog_main_group },

    { &insert_field_selector_group, NULL },
    { &insert_field_selector_0, &insert_field_date_selector_0_data },
    { &insert_field_selector_1, &insert_field_date_selector_1_data },

    { &insert_field_list_dt, &stdlisttext_data },

    { &insert_field_live, &insert_field_live_data },

    { &defbutton_ok, &insert_field_insert_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

static const DIALOG_CTL_CREATE
insert_field_date_dead_only_ctl_create[] =
{
    { &dialog_main_group },

    { &insert_field_selector_group, NULL },
    { &insert_field_selector_0, &insert_field_date_selector_0_data },
    { &insert_field_selector_1, &insert_field_date_selector_1_data },

    { &insert_field_list_dt, &stdlisttext_data },

    { &defbutton_ok, &insert_field_insert_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

static const DIALOG_CTL_CREATE
insert_field_page_ctl_create[] =
{
    { &dialog_main_group },

    { &insert_field_selector_group, NULL },
    { &insert_field_selector_0, &insert_field_page_selector_0_data },
    { &insert_field_selector_1, &insert_field_page_selector_1_data },

    { &insert_field_list, &stdlisttext_data },

    { &defbutton_ok, &insert_field_insert_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

_Check_return_
static STATUS
insert_field_intro_setup_entry_date(
    _DocuRef_   P_DOCU p_docu,
    P_FIELD_LIST_ENTRY p_field_list_entry,
    _InVal_     U32 selector_idx,
    PC_ANY p_setup_data,
    _InoutRef_  P_PIXIT p_max_width)
{
    EV_DATA ev_data;
    NUMFORM_PARMS numform_parms;
    STATUS status;

    UNREFERENCED_PARAMETER(p_setup_data);

    zero_struct(ev_data);
    zero_struct(numform_parms);

    /* each ***entry*** has a quick buf! */
    quick_ublock_with_buffer_setup(p_field_list_entry->quick_ublock);

    switch(selector_idx)
    {
    case 1:
        ev_data.did_num = RPN_DAT_DATE;
        ss_local_time_as_ev_date(&ev_data.arg.ev_date);
        break;

    default: default_unhandled();
#if CHECKING
    case 0:
#endif
        ev_data.did_num = RPN_DAT_DATE;
        ev_data.arg.ev_date = p_docu->file_date;
        break;
    }

    numform_parms.ustr_numform_datetime = p_field_list_entry->ustr_numform;
    numform_parms.p_numform_context = get_p_numform_context(p_docu);

    status_assert(status = numform(&p_field_list_entry->quick_ublock, P_QUICK_TBLOCK_NONE, &ev_data, &numform_parms));

    {
    PIXIT width = ui_width_from_ustr(quick_ublock_ustr(&p_field_list_entry->quick_ublock));
    if(*p_max_width < width)
       *p_max_width = width;
    } /*block*/

    return(status);
}

_Check_return_
static STATUS
insert_field_intro_setup_entry_page(
    _DocuRef_   P_DOCU p_docu,
    P_FIELD_LIST_ENTRY p_field_list_entry,
    _InVal_     U32 selector_idx,
    PC_ANY p_setup_data,
    _InoutRef_  P_PIXIT p_max_width)
{
    EV_DATA ev_data;
    NUMFORM_PARMS numform_parms;
    STATUS status;

    zero_struct(numform_parms);

    /* each ***entry*** has a quick buf! */
    quick_ublock_with_buffer_setup(p_field_list_entry->quick_ublock);

    switch(selector_idx)
    {
    case 1:
        ev_data_set_integer(&ev_data, ((PC_PAGE_NUM) p_setup_data)->x + 1);
        break;

    default: default_unhandled();
#if CHECKING
    case 0:
#endif
        ev_data_set_integer(&ev_data, page_number_from_page_y(p_docu, ((PC_PAGE_NUM) p_setup_data)->y) + 1);
        break;
    }

    numform_parms.ustr_numform_numeric = p_field_list_entry->ustr_numform;
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
field_list_source[2];

static UI_SOURCE
current_field_list_source; /* a copy of one of the above */

_Check_return_
static STATUS
dialog_insert_field_intro_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    const P_INSERT_FIELD_INTRO_CALLBACK p_insert_field_intro_callback = (P_INSERT_FIELD_INTRO_CALLBACK) p_dialog_msg_ctl_fill_source->client_handle;

    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case INSERT_FIELD_ID_LIST:
        current_field_list_source = field_list_source[*(p_insert_field_intro_callback->p_selector)];
        p_dialog_msg_ctl_fill_source->p_ui_source = &current_field_list_source;
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
    const S32 selector = *(p_insert_field_intro_callback->p_selector);

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case INSERT_FIELD_ID_SELECTOR_GROUP:
        p_dialog_msg_ctl_create_state->state_set.bits = 0;
        p_dialog_msg_ctl_create_state->state_set.state.radiobutton = selector;
        break;

    case INSERT_FIELD_ID_LIST:
        p_dialog_msg_ctl_create_state->state_set.bits = DIALOG_STATE_SET_ALTERNATE;
        p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = *(p_insert_field_intro_callback->p_selected_field[selector]);
        break;

    case INSERT_FIELD_ID_LIVE:
        p_dialog_msg_ctl_create_state->state_set.bits = 0;
        p_dialog_msg_ctl_create_state->state_set.state.checkbox = (U8) !*(p_insert_field_intro_callback->p_dead_field[selector]);
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_insert_field_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_INSERT_FIELD_INTRO_CALLBACK p_insert_field_intro_callback = (P_INSERT_FIELD_INTRO_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case INSERT_FIELD_ID_SELECTOR_GROUP:
        {
        STATUS status = STATUS_OK;
        S32 selector = *(p_insert_field_intro_callback->p_selector) = p_dialog_msg_ctl_state_change->new_state.radiobutton;

        if(!p_insert_field_intro_callback->dead_only && (p_insert_field_intro_callback->t5_message != T5_CMD_INSERT_FIELD_INTRO_PAGE))
            status = ui_dlg_set_check(p_dialog_msg_ctl_state_change->h_dialog, INSERT_FIELD_ID_LIVE, !*(p_insert_field_intro_callback->p_dead_field[selector]));

        current_field_list_source = field_list_source[selector];
        ui_dlg_ctl_new_source(p_dialog_msg_ctl_state_change->h_dialog, INSERT_FIELD_ID_LIST);
        return(status);
        }

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
dialog_insert_field_intro_process_end(
_InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_INSERT_FIELD_INTRO_CALLBACK p_insert_field_intro_callback = (P_INSERT_FIELD_INTRO_CALLBACK) p_dialog_msg_process_end->client_handle;
        const S32 selector = *(p_insert_field_intro_callback->p_selector);

        /* remember selected field and live state for this particular option */
        *(p_insert_field_intro_callback->p_selected_field[selector]) = ui_dlg_get_list_idx(p_dialog_msg_process_end->h_dialog, INSERT_FIELD_ID_LIST);

        if(!p_insert_field_intro_callback->dead_only && (p_insert_field_intro_callback->t5_message != T5_CMD_INSERT_FIELD_INTRO_PAGE))
            *(p_insert_field_intro_callback->p_dead_field[selector]) = !ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, INSERT_FIELD_ID_LIVE);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_insert_field_intro)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_insert_field_intro_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_insert_field_intro_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_insert_field_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_insert_field_intro_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

/* make appropriate size box */

static void
insert_field_list_get_size(
    _InRef_     PC_UI_TEXT p_ui_text_caption,
    _InRef_     PC_PIXIT_SIZE p_list_size,
    _InVal_     PIXIT max_width,
    _OutRef_    P_PIXIT_SIZE p_pixit_size_out)
{
    const PIXIT buttons_width = DIALOG_DEFOK_H + DIALOG_STDSPACING_H + DIALOG_STDCANCEL_H;
    const PIXIT caption_width = ui_width_from_p_ui_text(p_ui_text_caption) + DIALOG_CAPTIONOVH_H;
    DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
    dialog_cmd_ctl_size_estimate.p_dialog_control = &insert_field_list;
    dialog_cmd_ctl_size_estimate.p_dialog_control_data = &stdlisttext_data;
    ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
    dialog_cmd_ctl_size_estimate.size.x += max_width;
    dialog_cmd_ctl_size_estimate.size.x += p_list_size->cx;
    dialog_cmd_ctl_size_estimate.size.y += p_list_size->cy;
    dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, buttons_width);
    dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, caption_width);
    p_pixit_size_out->cx = dialog_cmd_ctl_size_estimate.size.x;
    p_pixit_size_out->cy = dialog_cmd_ctl_size_estimate.size.y;
}

/* simply pipe in UI representation */

_Check_return_
static STATUS
insert_field_as_text(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     U32 selector_idx,
    _InVal_     S32 selected_field)
{
    STATUS status;
    PC_USTR ustr = ui_text_ustr(array_ptrc(&field_list_source[selector_idx].source.array_handle, UI_TEXT, selected_field));
    SKELEVENT_KEYS skelevent_keys;
    QUICK_UBLOCK quick_ublock;
    quick_ublock_setup_fill_from_const_ubuf(&quick_ublock, ustr, ustrlen32(ustr));

    skelevent_keys.p_quick_ublock = &quick_ublock;
    skelevent_keys.processed = 0;
    status = object_skel(p_docu, T5_EVENT_KEYS, &skelevent_keys);

    docu_modify(p_docu);

    return(status);
}

_Check_return_
static STATUS
insert_field_using_command(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _In_z_      PC_USTR ustr_numform)
{
    STATUS status;
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        p_args[0].val.ustr = ustr_numform;
        status = execute_command(p_docu, t5_message, &arglist_handle, object_id);
        arglist_dispose(&arglist_handle);
    }

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_insert_field_intro_date)
{
    static struct INSERT_FIELD_INTRO_DATE_STATICS
    {
        S32 selector_date; /* 0 inserts file modification date/time */
        S32 selector_time;
        S32 selected_date_field[2];
        S32 selected_time_field[2];
        BOOL dead_date_field[2];
        BOOL dead_time_field[2];
    }
    insert_field_intro_date =
    {
        0,
        0,
        { 0, 0 },
        { 0, 0 },
        { FALSE, TRUE }, /*after 1.19/09*/
        { FALSE, TRUE }
    };

    const BOOL insert_time = (t5_message == T5_CMD_INSERT_FIELD_INTRO_TIME);
    INSERT_FIELD_INTRO_CALLBACK insert_field_intro_callback;
    ARRAY_HANDLE field_list_handle[2];
    const U32 num_s = 2;
    U32 s_idx;
    P_FIELD_LIST_ENTRY p_field_list_entry;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*p_field_list_entry), 0);
    STATUS status = STATUS_OK;
    PIXIT max_width = 0;
    P_ANY p_setup_data = P_DATA_NONE;

    insert_field_intro_callback.dead_only = FALSE;

    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    /* strictly speaking we should enquire as to whether this is possible */
    if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
        insert_field_intro_callback.dead_only = TRUE;

    for(s_idx = 0; s_idx < num_s; ++s_idx)
    {
        field_list_handle[s_idx] = 0;
        field_list_source[s_idx].type = UI_SOURCE_TYPE_NONE;
    }

    {
    const PC_ARRAY_HANDLE p_ui_numform_handle = &p_docu_from_config()->numforms;
    ARRAY_INDEX i;
    const UI_NUMFORM_CLASS ui_numform_class = insert_time ? UI_NUMFORM_CLASS_TIME : UI_NUMFORM_CLASS_DATE;

    insert_field_intro_callback.t5_message = t5_message;
    insert_field_intro_callback.p_selector = insert_time ? &insert_field_intro_date.selector_time : &insert_field_intro_date.selector_date;

    for(s_idx = 0; s_idx < num_s; ++s_idx)
    {
        insert_field_intro_callback.p_selected_field[s_idx] = insert_time ? &insert_field_intro_date.selected_time_field[s_idx] : &insert_field_intro_date.selected_date_field[s_idx];
        insert_field_intro_callback.p_dead_field[s_idx] = insert_time ? &insert_field_intro_date.dead_time_field[s_idx] : &insert_field_intro_date.dead_date_field[s_idx];
    }

    for(i = 0; i < array_elements(p_ui_numform_handle); ++i)
    {
        const PC_UI_NUMFORM p_ui_numform = array_ptrc(p_ui_numform_handle, UI_NUMFORM, i);

        if(p_ui_numform->numform_class != ui_numform_class)
            continue;

        for(s_idx = 0; s_idx < num_s; ++s_idx)
        {
            if(NULL != (p_field_list_entry = al_array_extend_by(&field_list_handle[s_idx], FIELD_LIST_ENTRY, 1, &array_init_block, &status)))
            {
                p_field_list_entry->ustr_numform = p_ui_numform->ustr_numform;

                status = insert_field_intro_setup_entry_date(p_docu, p_field_list_entry, s_idx, p_setup_data, &max_width);
            }

            status_break(status);
        }

        status_break(status);
    }
    } /*block*/

    /* make a source of text pointers to these elements for list box processing */
    for(s_idx = 0; s_idx < num_s; ++s_idx)
    {
        if(status_ok(status))
            status = ui_source_create_ub(&field_list_handle[s_idx], &field_list_source[s_idx], UI_TEXT_TYPE_USTR_PERM, offsetof32(FIELD_LIST_ENTRY, quick_ublock));
    }

    if(status_ok(status))
    {
        UI_TEXT caption;

        caption.type = UI_TEXT_TYPE_RESID;
        caption.text.resource_id = insert_time ? MSG_DIALOG_INSERT_FIELD_TIME_CAPTION : MSG_DIALOG_INSERT_FIELD_DATE_CAPTION;

        { /* make appropriate size box */
        PIXIT_SIZE list_size[2];
        PIXIT_SIZE max_list_size;
        PIXIT_SIZE pixit_size;
        ui_list_size_estimate(array_elements(&field_list_handle[0]), &list_size[0]);
        ui_list_size_estimate(array_elements(&field_list_handle[1]), &list_size[1]);
        max_list_size.cx = MAX(list_size[0].cx, list_size[1].cx);
        max_list_size.cy = MAX(list_size[0].cy, list_size[1].cy);
        insert_field_list_get_size(&caption, &max_list_size, max_width, &pixit_size);
#if WINDOWS
        insert_field_list_dt.relative_offset[2] = pixit_size.cx;
#endif
        insert_field_list_dt.relative_offset[3] = pixit_size.cy;
        } /*block*/

        {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        zero_struct(dialog_cmd_process_dbox);
        dialog_cmd_process_dbox.caption = caption;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_insert_field_intro;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &insert_field_intro_callback;
        if(insert_field_intro_callback.dead_only)
        {
            dialog_cmd_process_dbox.n_ctls = elemof32(insert_field_date_dead_only_ctl_create);
            dialog_cmd_process_dbox.p_ctl_create  = insert_field_date_dead_only_ctl_create;
        }
        else
        {
            dialog_cmd_process_dbox.n_ctls = elemof32(insert_field_date_ctl_create);
            dialog_cmd_process_dbox.p_ctl_create  = insert_field_date_ctl_create;
        }
        status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        } /*block*/

        if(DIALOG_COMPLETION_OK == status)
        {
            /* selected field to use NB. UI list unsorted so maps directly onto field list index */
            S32 selected_field;

            s_idx = (U32) *(insert_field_intro_callback.p_selector);

            selected_field = *(insert_field_intro_callback.p_selected_field[s_idx]);

            if(insert_field_intro_callback.dead_only || *(insert_field_intro_callback.p_dead_field[s_idx]))
            {
                /* simply pipe in UI representation */
                status = insert_field_as_text(p_docu, s_idx, selected_field);
            }
            else
            {
                T5_MESSAGE new_t5_message;

                p_field_list_entry = array_ptr(&field_list_handle[s_idx], FIELD_LIST_ENTRY, selected_field);

                switch(s_idx)
                {
                case 1:
                    new_t5_message = T5_CMD_INSERT_FIELD_DATE;
                    break;

                default: default_unhandled();
#if CHECKING
                case 0:
#endif
                    new_t5_message = T5_CMD_INSERT_FIELD_FILE_DATE;
                    break;
                }

                status = insert_field_using_command(p_docu, new_t5_message, p_field_list_entry->ustr_numform);
            }
        }
    }

    for(s_idx = 0; s_idx < num_s; ++s_idx)
        ui_lists_dispose_ub(&field_list_handle[s_idx], &field_list_source[s_idx], offsetof32(FIELD_LIST_ENTRY, quick_ublock));

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_insert_field_intro_page)
{
    static struct INSERT_FIELD_INTRO_PAGE_STATICS
    {
        S32 selector_page; /* 0 inserts y page number */
        S32 selected_page_field[2];
    }
    insert_field_intro_page =
    {
        0,
        { 0, 0 }
    };

    INSERT_FIELD_INTRO_CALLBACK insert_field_intro_callback;
    ARRAY_HANDLE field_list_handle[2];
    const U32 num_s = 2;
    U32 s_idx;
    P_FIELD_LIST_ENTRY p_field_list_entry;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*p_field_list_entry), 0);
    STATUS status = STATUS_OK;
    PIXIT max_width = 0;
    P_ANY p_setup_data = P_DATA_NONE;
    BOOL dead_page_field[2]; /* not persistent */

    insert_field_intro_callback.dead_only = FALSE;

    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    /* strictly speaking we should enquire as to whether this is possible */
    if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
        insert_field_intro_callback.dead_only = TRUE;

    for(s_idx = 0; s_idx < num_s; ++s_idx)
    {
        dead_page_field[s_idx] = insert_field_intro_callback.dead_only;

        field_list_handle[s_idx] = 0;
        field_list_source[s_idx].type = UI_SOURCE_TYPE_NONE;
    }

    {
    PC_ARRAY_HANDLE p_ui_numform_handle = &p_docu_from_config()->numforms;
    ARRAY_INDEX i;
    const UI_NUMFORM_CLASS ui_numform_class= UI_NUMFORM_CLASS_PAGE;
    PAGE_NUM page_num;

    insert_field_intro_callback.t5_message = t5_message;
    insert_field_intro_callback.p_selector = &insert_field_intro_page.selector_page;

    for(s_idx = 0; s_idx < num_s; ++s_idx)
    {
        insert_field_intro_callback.p_selected_field[s_idx] = &insert_field_intro_page.selected_page_field[s_idx];
        insert_field_intro_callback.p_dead_field[s_idx] = &dead_page_field[s_idx];
    }

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

    for(i = 0; i < array_elements(p_ui_numform_handle); ++i)
    {
        PC_UI_NUMFORM p_ui_numform = array_ptrc(p_ui_numform_handle, UI_NUMFORM, i);

        if(p_ui_numform->numform_class != ui_numform_class)
            continue;

        for(s_idx = 0; s_idx < num_s; ++s_idx)
        {
            if(NULL != (p_field_list_entry = al_array_extend_by(&field_list_handle[s_idx], FIELD_LIST_ENTRY, 1, &array_init_block, &status)))
            {
                p_field_list_entry->ustr_numform = p_ui_numform->ustr_numform;

                status = insert_field_intro_setup_entry_page(p_docu, p_field_list_entry, s_idx, p_setup_data, &max_width);
            }

            status_break(status);
        }

        status_break(status);
    }
    } /*block*/

    /* make a source of text pointers to these elements for list box processing */
    for(s_idx = 0; s_idx < num_s; ++s_idx)
    {
        if(status_ok(status))
            status = ui_source_create_ub(&field_list_handle[s_idx], &field_list_source[s_idx], UI_TEXT_TYPE_USTR_PERM, offsetof32(FIELD_LIST_ENTRY, quick_ublock));
    }

    if(status_ok(status))
    {
        UI_TEXT caption;

        caption.type = UI_TEXT_TYPE_RESID;
        caption.text.resource_id = MSG_DIALOG_INSERT_FIELD_PAGE_CAPTION;

        { /* make appropriate size box */
        PIXIT_SIZE list_size[2];
        PIXIT_SIZE max_list_size;
        PIXIT_SIZE pixit_size;
        ui_list_size_estimate(array_elements(&field_list_handle[0]), &list_size[0]);
        ui_list_size_estimate(array_elements(&field_list_handle[1]), &list_size[1]);
        max_list_size.cx = MAX(list_size[0].cx, list_size[1].cx);
        max_list_size.cy = MAX(list_size[0].cy, list_size[1].cy);
        insert_field_list_get_size(&caption, &max_list_size, max_width, &pixit_size);
        insert_field_list.relative_offset[2] = pixit_size.cx;
        insert_field_list.relative_offset[3] = pixit_size.cy;
        } /*block*/

        {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        zero_struct(dialog_cmd_process_dbox);
        dialog_cmd_process_dbox.caption = caption;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_insert_field_intro;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &insert_field_intro_callback;
        dialog_cmd_process_dbox.n_ctls = elemof32(insert_field_page_ctl_create);
        dialog_cmd_process_dbox.p_ctl_create  = insert_field_page_ctl_create;
        status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        } /*block*/

        if(DIALOG_COMPLETION_OK == status)
        {
            /* selected field to use NB. UI list unsorted so maps directly onto field list index */
            S32 selected_field;

            s_idx = (U32) *(insert_field_intro_callback.p_selector);

            selected_field = *(insert_field_intro_callback.p_selected_field[s_idx]);

            if(insert_field_intro_callback.dead_only || *(insert_field_intro_callback.p_dead_field[s_idx]))
            {
                /* simply pipe in UI representation */
                status = insert_field_as_text(p_docu, s_idx, selected_field);
            }
            else
            {
                T5_MESSAGE new_t5_message;

                p_field_list_entry = array_ptr(&field_list_handle[s_idx], FIELD_LIST_ENTRY, selected_field);

                switch(s_idx)
                {
                case 1:
                    new_t5_message = T5_CMD_INSERT_FIELD_PAGE_X;
                    break;

                default: default_unhandled();
#if CHECKING
                case 0:
#endif
                    new_t5_message = T5_CMD_INSERT_FIELD_PAGE_Y;
                    break;
                }

                status = insert_field_using_command(p_docu, new_t5_message, p_field_list_entry->ustr_numform);
            }
        }
    }

    for(s_idx = 0; s_idx < num_s; ++s_idx)
        ui_lists_dispose_ub(&field_list_handle[s_idx], &field_list_source[s_idx], offsetof32(FIELD_LIST_ENTRY, quick_ublock));

    return(status);
}

/* end of ui_field.c */
