/* ob_dlg2.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* dialog UI handling */

/* SKS April 1992 */

#include "common/gflags.h"

#include "ob_dlg/ui_dlgin.h"

/*
internal functions
*/

static void
dialog_control_rect_changed_zap_dependents_in(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     BIT_NUMBER bit_number,
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     DIALOG_CONTROL_ID parent_dialog_control_id);

static void
dialog_dbox_process_focus_return_post(
    P_DIALOG p_dialog);

/******************************************************************************
*
* build the arglist for a command given current state
*
******************************************************************************/

_Check_return_
static STATUS
dialog_arglist_arg_xstr_from_ui_text(
    _InRef_     PC_ARGLIST_HANDLE p_arglist_handle /*[arg_idx] data modified*/,
    _InVal_     U32 arg_idx,
    _InRef_     PC_UI_TEXT p_ui_text)
{
    const PC_ARGLIST_ARG p_arg = pc_arglist_arg(p_arglist_handle, arg_idx);

    switch(p_arg->type & ARG_TYPE_MASK)
    {
    case ARG_TYPE_USTR:
        return(arg_alloc_ustr(p_arglist_handle, arg_idx, ui_text_ustr(p_ui_text)));

#if defined(ARG_TYPE_TSTR_DISTINCT)
    case ARG_TYPE_TSTR:
        return(arg_alloc_tstr(p_arglist_handle, arg_idx, ui_text_tstr(p_ui_text)));
#endif

    default:
        assert0();
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
dialog_arglist_construct(
    _OutRef_    P_ARGLIST_HANDLE p_arglist_handle,
    P_DIALOG p_dialog,
    _InRef_     PC_ARG_TYPE p_arg_types /*[], terminator is ARG_TYPE_NONE*/,
    const PC_DIALOG_CTL_ID p_dialog_control_id /*[n_arglist_args]*/)
{
    status_return(arglist_prepare(p_arglist_handle, p_arg_types));

    if(0 != n_arglist_args(p_arglist_handle))
    {
        const U32 n_args = n_arglist_args(p_arglist_handle);
        const P_ARGLIST_ARG p_args = p_arglist_args(p_arglist_handle, n_args);
        U32 arg_idx;

        for(arg_idx = 0; arg_idx < n_args; ++arg_idx)
        {
            const P_ARGLIST_ARG p_arg = &p_args[arg_idx];
            DIALOG_CMD_CTL_STATE_QUERY dialog_cmd_ctl_state_query;
            STATUS status;

            dialog_cmd_ctl_state_query.dialog_control_id = p_dialog_control_id[arg_idx];

            if(0 == dialog_cmd_ctl_state_query.dialog_control_id)
            {
                p_arg->type = ARG_TYPE_NONE; /*arg_dispose(p_arglist_handle, arg_idx);*/
                continue;
            }

            dialog_cmd_ctl_state_query.h_dialog = p_dialog->h_dialog;
            dialog_cmd_ctl_state_query.bits = 0;
            status = object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query);

            if(status_fail(status))
            {   /* e.g. view control may not have controls for ruler on/off */
                p_arg->type = ARG_TYPE_NONE; /*arg_dispose(p_arglist_handle, i);*/
                continue;
            }
            switch(dialog_cmd_ctl_state_query.dialog_control_type)
            {
            default: default_unhandled();
#if CHECKING
            case DIALOG_CONTROL_CHECKBOX:
            case DIALOG_CONTROL_CHECKPICTURE:
            case DIALOG_CONTROL_TRISTATE:
            case DIALOG_CONTROL_TRIPICTURE:
                /* all these are generic_u8n or bool */
#endif
                assert(((p_arg->type & ARG_TYPE_MASK) == ARG_TYPE_U8N) || ((p_arg->type & ARG_TYPE_MASK) == ARG_TYPE_BOOL));
                p_arg->val.u8n = dialog_cmd_ctl_state_query.state.generic_u8n;
                break;

#if CHECKING
            case DIALOG_CONTROL_RADIOBUTTON:
            case DIALOG_CONTROL_RADIOPICTURE:
                myassert0(TEXT("lookup the group!"));

                /*FALLTHRU*/
#endif

            case DIALOG_CONTROL_GROUPBOX:
            case DIALOG_CONTROL_BUMP_S32:
                assert((p_arg->type & ARG_TYPE_MASK) == ARG_TYPE_S32);
                p_arg->val.s32 = dialog_cmd_ctl_state_query.state.bump_s32;
                break;

            case DIALOG_CONTROL_BUMP_F64:
                assert((p_arg->type & ARG_TYPE_MASK) == ARG_TYPE_F64);
                p_arg->val.f64 = dialog_cmd_ctl_state_query.state.bump_f64;
                break;

            case DIALOG_CONTROL_EDIT:
                status = dialog_arglist_arg_xstr_from_ui_text(p_arglist_handle, arg_idx, &dialog_cmd_ctl_state_query.state.edit.ui_text);
                break;

            case DIALOG_CONTROL_LIST_S32:
                assert((p_arg->type & ARG_TYPE_MASK) == ARG_TYPE_S32);
                assert(dialog_cmd_ctl_state_query.state.list_s32.itemno >= 0);
                p_arg->val.s32 = dialog_cmd_ctl_state_query.state.list_s32.s32;
                break;

            case DIALOG_CONTROL_LIST_TEXT:
                assert(dialog_cmd_ctl_state_query.state.list_text.itemno >= 0);
                status = dialog_arglist_arg_xstr_from_ui_text(p_arglist_handle, arg_idx, &dialog_cmd_ctl_state_query.state.list_text.ui_text);
                break;

            case DIALOG_CONTROL_COMBO_S32:
                assert((p_arg->type & ARG_TYPE_MASK) == ARG_TYPE_S32);
                assert(dialog_cmd_ctl_state_query.state.combo_s32.itemno >= 0);
                p_arg->val.s32 = dialog_cmd_ctl_state_query.state.combo_s32.s32;
                break;

            case DIALOG_CONTROL_COMBO_TEXT:
                assert(dialog_cmd_ctl_state_query.state.list_text.itemno != DIALOG_CTL_STATE_LIST_ITEM_NONE);
                status = dialog_arglist_arg_xstr_from_ui_text(p_arglist_handle, arg_idx, &dialog_cmd_ctl_state_query.state.combo_text.ui_text);
                break;

            case DIALOG_CONTROL_USER:
                assert((p_arg->type & ARG_TYPE_MASK) == ARG_TYPE_X32);
                p_arg->val.x32 = dialog_cmd_ctl_state_query.state.user.u32;
                break;

            case DIALOG_CONTROL_STATICTEXT:
            case DIALOG_CONTROL_STATICFRAME:
            case DIALOG_CONTROL_PUSHBUTTON:
                status = dialog_arglist_arg_xstr_from_ui_text(p_arglist_handle, arg_idx, &dialog_cmd_ctl_state_query.state.pushbutton);
                break;
            }

            status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY_DISPOSE, &dialog_cmd_ctl_state_query));
        }
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
dialog_call_client(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_MESSAGE dialog_message,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     P_PROC_DIALOG_EVENT p_proc_client)
{
    if(NULL == p_proc_client)
        return(STATUS_OK);

    return((* p_proc_client) (p_docu_from_docno(p_dialog->docno), dialog_message, p_data));
}

_Check_return_
extern STATUS
dialog_click_bump_xx(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    _InVal_     BOOL inc)
{
    U32 n_bytes;
    UI_DATA ui_data, ui_data_2;

    { /* due to updating mechanisms, the value stored in the state will be up-to-date wrt. the value shown in the edit field */
    const void /*UI_CONTROL*/ * p_ui_control;
    UI_DATA_TYPE ui_data_type;

    p_ui_control = p_dialog_ictl->p_dialog_control_data.bump_s32->bump_xx.p_uic;

    switch(p_dialog_ictl->dialog_control_type)
    {
    default: default_unhandled();
#if CHECKING
    case DIALOG_CONTROL_BUMP_S32:
#endif
        ui_data.s32 = p_dialog_ictl->state.bump_s32;
        ui_data_type = UI_DATA_TYPE_S32;
        n_bytes = sizeof32(ui_data.s32);
        break;

    case DIALOG_CONTROL_BUMP_F64:
        ui_data.f64 = p_dialog_ictl->state.bump_f64;
        ui_data_type = UI_DATA_TYPE_F64;
        n_bytes = sizeof32(ui_data.f64);
        break;
    }

    ui_data_2 = ui_data;

    status_assert(ui_data_inc_dec(ui_data_type, &ui_data, p_ui_control, inc));
    } /*block*/

    { /* command ourseleves to set this new value iff changed */
    int changed = memcmp32(&ui_data, &ui_data_2, n_bytes);

    if(changed)
    {
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
        msgclr(dialog_cmd_ctl_state_set);
        dialog_cmd_ctl_state_set.h_dialog = p_dialog->h_dialog;
        dialog_cmd_ctl_state_set.dialog_control_id = p_dialog_ictl->dialog_control_id;
        dialog_cmd_ctl_state_set.bits = 0;

        switch(p_dialog_ictl->dialog_control_type)
        {
        default: default_unhandled();
#if CHECKING
        case DIALOG_CONTROL_BUMP_S32:
#endif
            dialog_cmd_ctl_state_set.state.bump_s32 = ui_data.s32;
            break;

        case DIALOG_CONTROL_BUMP_F64:
            dialog_cmd_ctl_state_set.state.bump_f64 = ui_data.f64;
            break;
        }

        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
    }

    /* SKS 21apr93 after 1.03 - all callers of this will have want control to be current and want caret to go to end */
    if(changed)
        dialog_current_set(p_dialog, p_dialog_ictl->dialog_control_id, FALSE);

    return(changed ? 1 : STATUS_OK);
    } /*block*/
}

/******************************************************************************
*
* send a pushbutton event to a control
*
******************************************************************************/

_Check_return_
extern STATUS
dialog_click_pushbutton(
    _In_        P_DIALOG p_dialog,
    _In_        P_DIALOG_ICTL p_dialog_ictl,
    _InVal_     BOOL right_button,
    _InVal_     DIALOG_CONTROL_ID double_dialog_control_id)
{
    const H_DIALOG h_dialog = p_dialog->h_dialog;
    const DIALOG_CONTROL_ID dialog_control_id = p_dialog_ictl->dialog_control_id;
    STATUS status;

    switch(p_dialog_ictl->dialog_control_type)
    {
    case DIALOG_CONTROL_PUSHBUTTON:
    case DIALOG_CONTROL_PUSHPICTURE:
        break;

    default: default_unhandled();
        /* non-buttons get no further */
        return(STATUS_OK);
    }

    /* SKS after 1.23 13nov95 has a good idea ... */
    if(!p_dialog_ictl->p_dialog_control_data.pushbutton->push_xx.no_memory_froth_check)
        status_return(ensure_memory_froth());

    {
    P_PROC_DIALOG_EVENT p_proc_client;
    DIALOG_MSG_CTL_PUSHBUTTON dialog_msg_ctl_pushbutton;
    msgclr(dialog_msg_ctl_pushbutton);

    if(NULL != (p_proc_client = dialog_find_handler(p_dialog, dialog_control_id, &dialog_msg_ctl_pushbutton.client_handle)))
    {
        DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_pushbutton, p_dialog, p_dialog_ictl);

        dialog_msg_ctl_pushbutton.processed = 0;
        dialog_msg_ctl_pushbutton.completion_code = 0;
        dialog_msg_ctl_pushbutton.right_button = right_button;
        dialog_msg_ctl_pushbutton.double_dialog_control_id = double_dialog_control_id;

        status_return(dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_PUSHBUTTON, &dialog_msg_ctl_pushbutton, p_proc_client));

        /* recache after going to client */
        p_dialog = p_dialog_from_h_dialog(h_dialog);
        if(NULL != p_dialog)
            p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, dialog_control_id);

        if(dialog_msg_ctl_pushbutton.processed)
        {
            if(dialog_msg_ctl_pushbutton.completion_code) /* 0 (STATUS_OK) not allowed */
            {
                DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
                msgclr(dialog_cmd_complete_dbox);
                dialog_cmd_complete_dbox.h_dialog = p_dialog->h_dialog;
                dialog_cmd_complete_dbox.completion_code = dialog_msg_ctl_pushbutton.completion_code;
                status_assert(object_call_DIALOG(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox));
            }

            return(STATUS_OK);
        }
    }
    } /*block*/

#if WINDOWS
    if((NULL != p_dialog) && p_dialog_ictl->p_dialog_control_data.pushbutton->push_xx.help_id_offset)
        return(dialog_windows_help(p_dialog));
#endif

    { /* is there an associated completion code for this button? */
    S32 completion_code = p_dialog_ictl->p_dialog_control_data.pushbutton->push_xx.completion_code;

    if(right_button && p_dialog_ictl->p_dialog_control_data.pushbutton->push_xx.alternate_right)
    {
        completion_code = (p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_PUSHPICTURE)
            ? p_dialog_ictl->p_dialog_control_data.pushpicturer->completion_code_r
            : p_dialog_ictl->p_dialog_control_data.pushbuttonr->completion_code_r;

        assert(completion_code); /* 0 (STATUS_OK) not allowed */
    }

    if(completion_code)
    {
        DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
        msgclr(dialog_cmd_complete_dbox);
        dialog_cmd_complete_dbox.h_dialog = h_dialog;
        dialog_cmd_complete_dbox.completion_code = completion_code;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox));
        return(STATUS_OK);
    }
    } /*block*/

    {
    PC_DIALOG_CONTROL_DATA_PUSH_COMMAND p;

    switch(p_dialog_ictl->dialog_control_type)
    {
    default: default_unhandled();
#if CHECKING
    case DIALOG_CONTROL_PUSHBUTTON:
#endif
        p = /*(right_button && p_dialog_ictl->p_dialog_control_data.pushbutton->push_xx.alternate_right)
          ? p_dialog_ictl->p_dialog_control_data.pushbuttonr->command_r
          :*/ p_dialog_ictl->p_dialog_control_data.pushbutton->command;
        break;

    case DIALOG_CONTROL_PUSHPICTURE:
        p = /*(right_button && p_dialog_ictl->p_dialog_control_data.pushpicture->push_xx.alternate_right)
          ? p_dialog_ictl->p_dialog_control_data.pushpicturer->command_r
          :*/ p_dialog_ictl->p_dialog_control_data.pushpicture->command;
        break;
    }

    if((NULL != p) && (NULL != p_dialog))
    {   /* is there an associated command for this button? */
        if(p->t5_message)
        {
            ARGLIST_HANDLE arglist_handle;
            PC_ARG_TYPE p_arg_type = p->p_arg_type;

            if(p->bits.lookup_arglist)
            {
                PC_CONSTRUCT_TABLE p_construct_table = construct_table_lookup_message(p->object_id, p->t5_message);
                PTR_ASSERT(p_construct_table);
                p_arg_type = p_construct_table->args;
            }

            status_return(status = dialog_arglist_construct(&arglist_handle, p_dialog, p_arg_type, p->p_dialog_control_id));

            { /* allow command preprocessing by dialog callback */
            P_PROC_DIALOG_EVENT p_proc_client;
            DIALOG_MSG_PREPROCESS_COMMAND dialog_msg_preprocess_command;
            msgclr(dialog_msg_preprocess_command);

            if(NULL != (p_proc_client = dialog_find_handler(p_dialog, dialog_control_id, &dialog_msg_preprocess_command.client_handle)))
            {
                DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_preprocess_command, p_dialog, p_dialog_ictl);

                dialog_msg_preprocess_command.arglist_handle = arglist_handle;

                status_assert(status = dialog_call_client(p_dialog, DIALOG_MSG_CODE_PREPROCESS_COMMAND, &dialog_msg_preprocess_command, p_proc_client));

                arglist_handle = dialog_msg_preprocess_command.arglist_handle;

                /* recache after going to client */
                p_dialog = p_dialog_from_h_dialog(h_dialog);
                if(NULL != p_dialog)
                    p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, dialog_control_id);
            }
            } /*block*/

            if(status_ok(status) && p_dialog)
            {
                if(p->bits.set_interactive)
                    command_set_interactive();

                status = execute_command(p_docu_from_docno(p_dialog->docno), p->t5_message, &arglist_handle, p->object_id);
            }

            arglist_dispose(&arglist_handle);

            status_return(status);

            if(NULL != (p_dialog = p_dialog_from_h_dialog(h_dialog)))
            {
                /* if dialog boxing, then complete unless right button */
                if(p->bits.dont_complete || right_button)
                {
                    /* tell the client we're being persistent */
                    CLIENT_HANDLE client_handle;
                    const P_PROC_DIALOG_EVENT p_proc_client = dialog_find_handler(p_dialog, dialog_control_id, &client_handle);

                    if(NULL != p_proc_client)
                    {
                        DIALOG_MSG_PERSISTING dialog_msg_persisting;
                        msgclr(dialog_msg_persisting);
                        dialog_msg_persisting.h_dialog = p_dialog->h_dialog;
                        dialog_msg_persisting.client_handle = client_handle;
                        status_assert(status = dialog_call_client(p_dialog, DIALOG_MSG_CODE_PERSISTING, &dialog_msg_persisting, p_proc_client));
                    }
                }
                else
                {
                    DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
                    msgclr(dialog_cmd_complete_dbox);
                    dialog_cmd_complete_dbox.h_dialog = p_dialog->h_dialog;
                    dialog_cmd_complete_dbox.completion_code = DIALOG_COMPLETION_OK;
                    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox));
                }
            }
        }
    }

    return(STATUS_OK);
    } /*block*/
}

/******************************************************************************
*
* received a click for a radio button
*
******************************************************************************/

_Check_return_
extern STATUS
dialog_click_radiobutton(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    /* set this state into the button's group */
    status_return(ui_dlg_set_radio(p_dialog->h_dialog, p_dialog_ictl->p_dialog_control->parent_dialog_control_id, p_dialog_ictl->p_dialog_control_data.radiobutton->activate_state));

    if(p_dialog_ictl->p_dialog_control_data.radiobutton->bits.move_focus)
    {
        DIALOG_CMD_CTL_FOCUS_SET dialog_cmd_ctl_focus_set;
        msgclr(dialog_cmd_ctl_focus_set);
        dialog_cmd_ctl_focus_set.h_dialog = p_dialog->h_dialog;
        dialog_cmd_ctl_focus_set.dialog_control_id = p_dialog_ictl->p_dialog_control_data.radiobuttonf->move_focus_dialog_control_id;
        status_return(object_call_DIALOG(DIALOG_CMD_CODE_CTL_FOCUS_SET, &dialog_cmd_ctl_focus_set));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* received a click for a check box
*
******************************************************************************/

_Check_return_
extern STATUS
dialog_click_checkbox(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = p_dialog->h_dialog;
    dialog_cmd_ctl_state_set.dialog_control_id = p_dialog_ictl->dialog_control_id;
    dialog_cmd_ctl_state_set.bits = 0;

    /* toggle state */
    switch(p_dialog_ictl->state.checkbox)
    {
    default: default_unhandled();
#if CHECKING
    case DIALOG_BUTTONSTATE_OFF:
#endif
        dialog_cmd_ctl_state_set.state.checkbox = DIALOG_BUTTONSTATE_ON;
        break;

    case DIALOG_BUTTONSTATE_ON:
        dialog_cmd_ctl_state_set.state.checkbox = DIALOG_BUTTONSTATE_OFF;
        break;
    }

    status_return(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));

    if(p_dialog_ictl->p_dialog_control_data.checkbox->bits.move_focus
    && (dialog_cmd_ctl_state_set.state.checkbox == DIALOG_BUTTONSTATE_ON))
    {
        DIALOG_CMD_CTL_FOCUS_SET dialog_cmd_ctl_focus_set;
        msgclr(dialog_cmd_ctl_focus_set);
        dialog_cmd_ctl_focus_set.h_dialog = p_dialog->h_dialog;
        dialog_cmd_ctl_focus_set.dialog_control_id = p_dialog_ictl->p_dialog_control_data.checkboxf->move_focus_dialog_control_id;
        status_return(object_call_DIALOG(DIALOG_CMD_CODE_CTL_FOCUS_SET, &dialog_cmd_ctl_focus_set));
    }

    return(STATUS_OK);
}

#ifdef DIALOG_HAS_TRISTATE

/******************************************************************************
*
* received a click for a tristate
*
******************************************************************************/

_Check_return_
extern STATUS
dialog_click_tristate(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = p_dialog->h_dialog;
    dialog_cmd_ctl_state_set.dialog_control_id = p_dialog_ictl->dialog_control_id;
    dialog_cmd_ctl_state_set.bits = 0;

    /* cycle state */
    switch(p_dialog_ictl->state.tristate)
    {
    default: default_unhandled();
#if CHECKING
    case DIALOG_TRISTATE_DONT_CARE:
#endif
        dialog_cmd_ctl_state_set.state.tristate = DIALOG_TRISTATE_ON;
        break;

    case DIALOG_TRISTATE_ON:
        dialog_cmd_ctl_state_set.state.tristate = DIALOG_TRISTATE_OFF;
        break;

    case DIALOG_TRISTATE_OFF:
        dialog_cmd_ctl_state_set.state.tristate = DIALOG_TRISTATE_DONT_CARE;
        break;
    }

    return(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
}

#endif

/******************************************************************************
*
* find the control id of a group
*
******************************************************************************/

static DIALOG_CONTROL_ID
dialog_control_id_of_group_in(
    P_DIALOG_ICTL_GROUP p_ictl_group,
    P_DIALOG_ICTL_GROUP p_ictl_group_group)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        /* check at this level */
        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            if(&p_dialog_ictl->data.groupbox.ictls == p_ictl_group_group)
                return(p_dialog_ictl->dialog_control_id);

            { /* recurse into subgroups */
            const DIALOG_CONTROL_ID dialog_control_id = dialog_control_id_of_group_in(&p_dialog_ictl->data.groupbox.ictls, p_ictl_group_group);
            if(dialog_control_id > 0)
                return(dialog_control_id);
            } /*block*/

            break;
        }
    }

    return(DIALOG_CONTROL_WINDOW);
}

extern DIALOG_CONTROL_ID
dialog_control_id_of_group(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_ictl_group_group)
{
    if(&p_dialog->ictls == p_ictl_group_group)
        return(DIALOG_CONTROL_WINDOW);

    return(dialog_control_id_of_group_in(&p_dialog->ictls, p_ictl_group_group));
}

/******************************************************************************
*
* return the root window relative box of a control
*
******************************************************************************/

#if CHECKING

static void
stop_here(void)
{
}

typedef struct DSC
{
    DIALOG_CONTROL_ID dep_id;
    BIT_NUMBER dep_bn;
}
DSC, * P_DSC;

#endif

static void
dialog_control_rect_using_bitmap(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    _InoutRef_opt_ P_PIXIT_RECT p_pixit_rect /*NULL->ensure*/,
    /*inout*/ P_BITMAP p_bitmap
    CHECKING_ONLY_ARG(_InRef_ P_ARRAY_HANDLE p_stack))
{
    PC_DIALOG_CONTROL p_dialog_control = p_dialog_ictl->p_dialog_control;
    const DIALOG_CONTROL_ID this_dialog_control_id = p_dialog_ictl->dialog_control_id;
    BIT_NUMBER bit_number;
    DIALOG_CONTROL_ID cached_dialog_control_id;
    BITMAP(cached_control_valid, 4);
    PIXIT_RECT cached_control_rect = { { 0, 0 }, { 0, 0 } }; /* dataflower */

    cached_dialog_control_id = 0;
    bitmap_clear(cached_control_valid, N_BITS_ARG(4));

    bit_number = 4; /* get the ball rolling; read edges in pairings for cache efficiency */

    for(;;)
    {
        switch(bit_number)
        {
        default: bit_number = 0; break;
        case 0:  bit_number = 2; break;
        case 1:  bit_number = 3; break;
        case 2:  bit_number = 1; break;
        case 3:
            if(NULL != p_pixit_rect)
            {
                /* return all our valid bits and rect so far */
                bitmap_copy(p_bitmap, (PC_BITMAP) &p_dialog_ictl->bits, N_BITS_ARG(4));

                *p_pixit_rect = p_dialog_ictl->pixit_rect;
            }
            return;
        }

        if(!bitmap_bit_test(p_bitmap, bit_number, N_BITS_ARG(4)))
            continue;

#if CHECKING
        if(this_dialog_control_id == 42)
            stop_here();
#endif

        /* NB. l,t,r,b valid fields must be first bitfields in bits */
        if(bitmap_bit_test((PC_BITMAP) &p_dialog_ictl->bits, bit_number, N_BITS_ARG(4)))
        {
            trace_3(TRACE_APP_DIALOG,
                    TEXT("rect_bitmap: id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(" is ") PIXIT_TFMT,
                    (U32) this_dialog_control_id, (S32) bit_number, ((PC_PIXIT) &p_dialog_ictl->pixit_rect)[bit_number]);
            continue;
        }

        { /* we don't yet have the coord containing this bit defined, so calculate it */
        const DIALOG_CONTROL_ID relative_dialog_control_id_actual = p_dialog_control->relative_dialog_control_id[bit_number];
        PIXIT relative_offset = p_dialog_control->relative_offset[bit_number];
        /* first byte of bits field is set of four pairs of bits, indexed by bit_number */
        BIT_NUMBER relative_bit_number = ((* (P_U32) &p_dialog_control->bits) >> (2 * bit_number)) & 0x03;
        PIXIT coord;

        if(DIALOG_CONTENTS_CALC == relative_offset) /* SKS 01apr95 adds some dynamic sizing stuff - use with care */
        {
            if((0 == relative_bit_number) || (2 == relative_bit_number))
            {
                DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
                dialog_cmd_ctl_size_estimate.p_dialog_control = p_dialog_control;
                dialog_cmd_ctl_size_estimate.p_dialog_control_data = p_dialog_ictl->p_dialog_control_data.p_any;
                status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_SIZE_ESTIMATE, &dialog_cmd_ctl_size_estimate));
                relative_offset = dialog_cmd_ctl_size_estimate.size.x;
            }
            else
                myassert1(TEXT("can't specify bit %d with DIALOG_CONTENTS_CALC"), relative_bit_number);
        }

#if CHECKING
        /* check appropriate pairing */
        myassert4x((relative_bit_number == bit_number) ||
                  (relative_bit_number == bit_number + ((relative_bit_number > bit_number) ? +2 : -2)),
                  TEXT("control id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(" can't be relative to id/bit ") U32_TFMT TEXT("/") S32_TFMT,
                  (U32) this_dialog_control_id, (S32) bit_number, (U32) relative_dialog_control_id_actual, (S32) relative_bit_number);

        if(DIALOG_CONTROL_PARENT == relative_dialog_control_id_actual)
            myassert2x((DIALOG_CONTROL_WINDOW == p_dialog_control->parent_dialog_control_id)
                   || (/*(p_dialog_control->parent_dialog_control_id >= 0) &&*/ (p_dialog_control->parent_dialog_control_id < DIALOG_CONTROL_MAX)),
                      TEXT("control id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(" must be relative to a given parent id or window"),
                      (U32) this_dialog_control_id, (S32) bit_number);

        {
        const U32 n_stack_elements = array_elements32(p_stack);
        P_DSC bottom = array_range(p_stack, DSC, 0, n_stack_elements);
        P_DSC top = bottom + n_stack_elements;
        P_DSC ptr = top;

        for(;;)
        {
            DSC dsc;

            assert(ptr >= bottom);
            if(ptr == bottom)
                break;

            dsc = *--ptr;

            if((dsc.dep_id == this_dialog_control_id) && (dsc.dep_bn == bit_number))
            {
                myassert3(TEXT("control id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(" is self-dependent over a cycle of ") S32_TFMT, (U32) dsc.dep_id, (S32) dsc.dep_bn, (S32) ((top - ptr) / 2));

                for(;;)
                {
                    assert(ptr <= top);
                    if(ptr == top)
                        break;

                    dsc = *ptr++;

                    trace_2(TRACE_OUT | TRACE_ANY, TEXT("loop: control id/bit ") S32_TFMT TEXT("/") S32_TFMT, (S32) dsc.dep_id, (S32) dsc.dep_bn);
                }

                dsc.dep_bn = (bit_number + 2) % 4;

                break;
            }
        }
        } /*block*/
#endif

        if(relative_bit_number < 2)
        {   /* yuk. I'm sure this was unintentional... */
            relative_offset = 0 - relative_offset; /* if relative to left or top then -ve offset, else +ve */ /* except where turned around below! surely clearer to make explicit in each case */
        }

        if(relative_dialog_control_id_actual == DIALOG_CONTROL_CONTENTS)
        {
            P_DIALOG_ICTL_GROUP p_ictl_group = &p_dialog_ictl->data.groupbox.ictls;
            ARRAY_INDEX i;

            assert(p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_GROUPBOX);

            trace_4(TRACE_APP_DIALOG,
                    TEXT("rect_bitmap: id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(" - group needs data from contents bit ") S32_TFMT TEXT(", offset ") PIXIT_TFMT,
                    (U32) this_dialog_control_id, (S32) bit_number, (S32) relative_bit_number, relative_offset);

            coord = (relative_bit_number < 2) ? S16_MAX : S16_MIN;

            for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
            {
                /* query each item in this group in turn for its coord */
                P_DIALOG_ICTL relative_p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);
                PIXIT_RECT relative_pixit_rect = { { 0, 0 }, { 0, 0 } }; /* dataflower */
                BITMAP(relative_bitmap, 4);
                PIXIT relative_coord;

                bitmap_clear(relative_bitmap, N_BITS_ARG(4));
                bitmap_bit_set(relative_bitmap, relative_bit_number, N_BITS_ARG(4));

                PTR_ASSERT(relative_p_dialog_ictl);
#if CHECKING
                if(NULL != relative_p_dialog_ictl)
                {
                    /* push an element on dependency stack */
                    DSC dsc;
                    dsc.dep_id = this_dialog_control_id;
                    dsc.dep_bn = bit_number;
                    status_assert(al_array_add(p_stack, DSC, 1, PC_ARRAY_INIT_BLOCK_NONE, &dsc));
#endif

                    dialog_control_rect_using_bitmap(p_dialog, relative_p_dialog_ictl, &relative_pixit_rect, relative_bitmap CHECKING_ONLY_ARG(p_stack));

#if CHECKING
                    /* pop element off dependency stack */
                    al_array_shrink_by(p_stack, -1);
                }
#endif

                relative_coord = ((PC_PIXIT) &relative_pixit_rect)[relative_bit_number];

                if(relative_bit_number < 2)
                {
                    if( coord > relative_coord)
                        coord = relative_coord;
                }
                else
                {
                    if( coord < relative_coord)
                        coord = relative_coord;
                }
            }
        }
        else
        {
            if( (DIALOG_CONTROL_WINDOW == relative_dialog_control_id_actual)
            || ((DIALOG_CONTROL_PARENT == relative_dialog_control_id_actual) && (DIALOG_CONTROL_WINDOW == p_dialog_control->parent_dialog_control_id)))
            {
                relative_offset = 0 - relative_offset; /* if relative to dialog, then come back in the other direction (like self) */

                switch(relative_bit_number)
                {
                default: default_unhandled();
#if CHECKING
                case DIALOG_RELATIVE_BIT_L:
                case DIALOG_RELATIVE_BIT_T:
#endif
                    coord = 0;
                    break;

                case DIALOG_RELATIVE_BIT_R:
                    coord = p_dialog->pixit_size.cx;
                    break;

                case DIALOG_RELATIVE_BIT_B:
                    coord = p_dialog->pixit_size.cy;
                    break;
                }

                trace_4(TRACE_APP_DIALOG,
                        TEXT("rect_bitmap: id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(" needs data from dialog window bit ") S32_TFMT TEXT(", offset ") PIXIT_TFMT,
                        (U32) this_dialog_control_id, (S32) bit_number, (S32) relative_bit_number, relative_offset);
            }
            else
            {
                DIALOG_CONTROL_ID relative_dialog_control_id;

                if(DIALOG_CONTROL_SELF == relative_dialog_control_id_actual)
                {
                    /* if relative to self, then come back in the other direction */
                    relative_dialog_control_id = this_dialog_control_id;

                    relative_offset = 0 - relative_offset;

                    if(relative_bit_number == bit_number)
                    {
                        myassert2(TEXT("control id ") U32_TFMT TEXT(" bit number ") S32_TFMT TEXT(" is self-relative"), (U32) this_dialog_control_id, (S32) relative_bit_number);
                        relative_bit_number = (bit_number + 2) & 3;
                    }

                    trace_4(TRACE_APP_DIALOG,
                            TEXT("rect_bitmap: id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(" needs data self bit ") S32_TFMT TEXT(", offset ") PIXIT_TFMT,
                            (U32) this_dialog_control_id, (S32) bit_number, (S32) relative_bit_number, relative_offset);
                }
                else if(DIALOG_CONTROL_PARENT == relative_dialog_control_id_actual)
                {
                    /* if relative to parent, then come back in the other direction */
                    relative_dialog_control_id = p_dialog_control->parent_dialog_control_id;

                    relative_offset = 0 - relative_offset;

                    trace_5(TRACE_APP_DIALOG,
                            TEXT("rect_bitmap: id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(" needs data from parent id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(", offset ") PIXIT_TFMT,
                            (U32) this_dialog_control_id, (S32) bit_number, (U32) relative_dialog_control_id, (S32) relative_bit_number, relative_offset);
                }
                else
                {
                    relative_dialog_control_id = relative_dialog_control_id_actual;

                    trace_5(TRACE_APP_DIALOG,
                            TEXT("rect_bitmap: id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(" needs data from id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(", offset ") PIXIT_TFMT,
                            (U32) this_dialog_control_id, (S32) bit_number, (U32) relative_dialog_control_id, (S32) relative_bit_number, relative_offset);
                }

                /* quick optimisation for already cached valid coords */
                if((relative_dialog_control_id == cached_dialog_control_id) && bitmap_bit_test(cached_control_valid, relative_bit_number, N_BITS_ARG(4)))
                {
                    coord = ((PC_PIXIT) &cached_control_rect)[relative_bit_number];

                    trace_3(TRACE_APP_DIALOG,
                            TEXT("rect_bitmap: data from id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(" was in local cache rect at level ") TEXT(", value ") PIXIT_TFMT,
                            (U32) relative_dialog_control_id, (S32) relative_bit_number, coord);
                }
                else
                {
                    P_DIALOG_ICTL relative_p_dialog_ictl;

                    if(DIALOG_CONTROL_SELF == relative_dialog_control_id_actual)
                        relative_p_dialog_ictl = p_dialog_ictl;
                    else
                    {
                        P_DIALOG_ICTL_GROUP relative_p_parent_ictls;
                        relative_p_dialog_ictl = p_dialog_ictl_from_control_id_in(&p_dialog->ictls, relative_dialog_control_id, &relative_p_parent_ictls);
                    }

                    /* load up the cache with what we find */
                    cached_dialog_control_id = relative_dialog_control_id;
                    bitmap_clear(cached_control_valid, N_BITS_ARG(4));
                    bitmap_bit_set(cached_control_valid, relative_bit_number, N_BITS_ARG(4));

                    PTR_ASSERT(relative_p_dialog_ictl);
#if CHECKING
                    if(NULL != relative_p_dialog_ictl)
                    {
                        /* push an element on dependency stack */
                        DSC dsc;
                        dsc.dep_id = this_dialog_control_id;
                        dsc.dep_bn = bit_number;
                        status_assert(al_array_add(p_stack, DSC, 1, PC_ARRAY_INIT_BLOCK_NONE, &dsc));
#endif

                        dialog_control_rect_using_bitmap(p_dialog, relative_p_dialog_ictl, &cached_control_rect, cached_control_valid CHECKING_ONLY_ARG(p_stack));

#if CHECKING
                        /* pop element off dependency stack */
                        al_array_shrink_by(p_stack, -1);
                    }
#endif

                    assert(bitmap_bit_test(cached_control_valid, relative_bit_number, N_BITS_ARG(4)));
                    coord = ((PC_PIXIT) &cached_control_rect)[relative_bit_number];
                }
            }
        }

        coord += relative_offset;

        /* enter coord into control's cache */
        ((P_PIXIT) &p_dialog_ictl->pixit_rect)[bit_number] = coord;

        trace_3(TRACE_APP_DIALOG,
                TEXT("rect_bitmap: id/bit ") U32_TFMT TEXT("/") S32_TFMT TEXT(" := ") PIXIT_TFMT,
                (U32) this_dialog_control_id, (S32) bit_number, ((PC_PIXIT) &p_dialog_ictl->pixit_rect)[bit_number]);

        bitmap_bit_set((P_BITMAP) &p_dialog_ictl->bits, bit_number, N_BITS_ARG(4));
        } /*block*/

        /* loop until all bits done, exit at top of loop */
    }
}

extern void
dialog_control_rect(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _OutRef_opt_ P_PIXIT_RECT p_pixit_rect /*NULL->ensure*/)
{
    P_DIALOG_ICTL_GROUP p_parent_ictls;
    const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id_in(&p_dialog->ictls, dialog_control_id, &p_parent_ictls);

    if(!p_dialog_ictl->bits.valid_rect)
    {
        BITMAP(ask_for, 4);

#if CHECKING
        ARRAY_HANDLE stack_handle;
        SC_ARRAY_INIT_BLOCK aib_stack = aib_init(4, sizeof32(DSC), TRUE);
        status_assert(al_array_preallocate_zero(&stack_handle, &aib_stack));
#endif

        bitmap_set(ask_for, N_BITS_ARG(4));

        dialog_control_rect_using_bitmap(p_dialog, p_dialog_ictl, &p_dialog_ictl->pixit_rect, ask_for CHECKING_ONLY_ARG(&stack_handle));

#if CHECKING
        al_array_dispose(&stack_handle);
#endif
        p_dialog_ictl->bits.valid_rect = 1;
    }

    if(NULL != p_pixit_rect)
        *p_pixit_rect = p_dialog_ictl->pixit_rect;
}

extern void
dialog_control_rect_changed(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InRef_     PC_BITMAP p_bitmap)
{
    BIT_NUMBER bit_number;

    for(bit_number = 0; bit_number < 4; ++bit_number)
        if(bitmap_bit_test(p_bitmap, bit_number, N_BITS_ARG(4)))
            /* find and zap all dependents of this control and edge in this dialog */
            dialog_control_rect_changed_zap_dependents_in(p_dialog, dialog_control_id, bit_number, &p_dialog->ictls, DIALOG_CONTROL_WINDOW);
}

/******************************************************************************
*
* when a coordinate associated with a control changes,
* find its dependents and notify them. to get minimal
* redraw we find the roots of the tree (i.e. a control
* which is depended on but has no dependents) and
* then work down all its branches. all changed controls
* have their valid_rect bits nobbled, and have invalid
* ... not done as yet
*
******************************************************************************/

static void
dialog_control_rect_changed_zap_dependents_in(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     BIT_NUMBER bit_number,
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     DIALOG_CONTROL_ID parent_dialog_control_id)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);
        const DIALOG_CONTROL_ID this_dialog_control_id = p_dialog_ictl->dialog_control_id;
        BIT_NUMBER relative_bit_number;

        for(relative_bit_number = 0; relative_bit_number < 4; ++relative_bit_number)
        {
            DIALOG_CONTROL_ID relative_dialog_control_id = p_dialog_ictl->p_dialog_control->relative_dialog_control_id[relative_bit_number];

            if(DIALOG_CONTROL_PARENT == relative_dialog_control_id)
                relative_dialog_control_id = parent_dialog_control_id;
            else if(DIALOG_CONTROL_SELF == relative_dialog_control_id)
                relative_dialog_control_id = this_dialog_control_id;

            if(relative_dialog_control_id == dialog_control_id)
            {   /* this control has a dependency on the given control and bit */
                BITMAP(    changed, 4);
                PIXIT_RECT pixit_rect;

                bitmap_clear(changed, N_BITS_ARG(4));

                bitmap_bit_set(changed, relative_bit_number, N_BITS_ARG(4));

                /* therefore one of its edges is no longer valid */
                bitmap_bit_clear((P_BITMAP) &p_dialog_ictl->bits, relative_bit_number, N_BITS_ARG(4));

                /* therefore the entire box is no longer valid */
                p_dialog_ictl->bits.valid_rect = 0;

                /* rejig new coordinates */
                dialog_control_rect(p_dialog, this_dialog_control_id, &pixit_rect);

#ifdef DIALOG_COORD_DEBUG
                myassert3x(pixit_rect.tl.x < pixit_rect.br.x - 1, TEXT("dialog_control_id ") U32_TFMT TEXT(" pixit_rect tl.x ") PIXIT_TFMT TEXT(" br.x ") PIXIT_TFMT, (U32) this_dialog_control_id, pixit_rect.tl.x, pixit_rect.br.x);
                myassert3x(pixit_rect.tl.y < pixit_rect.br.y - 1, TEXT("dialog_control_id ") U32_TFMT TEXT(" pixit_rect tl.y ") PIXIT_TFMT TEXT(" br.y ") PIXIT_TFMT, (U32) this_dialog_control_id, pixit_rect.tl.y, pixit_rect.br.y);
#else
                myassert3x(pixit_rect.tl.x <= pixit_rect.br.x, TEXT("dialog_control_id ") U32_TFMT TEXT(" pixit_rect tl.x ") PIXIT_TFMT TEXT(" br.x ") PIXIT_TFMT, (U32) this_dialog_control_id, pixit_rect.tl.x, pixit_rect.br.x);
                myassert3x(pixit_rect.tl.y <= pixit_rect.br.y, TEXT("dialog_control_id ") U32_TFMT TEXT(" pixit_rect tl.y ") PIXIT_TFMT TEXT(" br.y ") PIXIT_TFMT, (U32) this_dialog_control_id, pixit_rect.tl.y, pixit_rect.br.y);
#endif
            }

            switch(p_dialog_ictl->dialog_control_type)
            {
            case DIALOG_CONTROL_GROUPBOX:
                dialog_control_rect_changed_zap_dependents_in(p_dialog, dialog_control_id, bit_number, &p_dialog_ictl->data.groupbox.ictls, this_dialog_control_id);
                break;

            default:
                break;
            }
        }
    }
}

typedef struct DIALOG_CTL_CONTEXT
{
    P_DIALOG_ICTL_GROUP p_ictl_group;
    ARRAY_INDEX i;
}
DIALOG_CTL_CONTEXT, * P_DIALOG_CTL_CONTEXT;

extern DIALOG_CONTROL_ID
dialog_current_move(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID current_dialog_control_id,
    _InVal_     STATUS movement)
{
    DIALOG_CTL_CONTEXT dialog_ctl_context;
    ARRAY_HANDLE context_handle = 0;
    P_DIALOG_CTL_CONTEXT p_dialog_ctl_context;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(*p_dialog_ctl_context), FALSE);
    P_DIALOG_ICTL p_dialog_ictl;
    DIALOG_CONTROL_ID dialog_control_id = 0;
    BOOL seen_current = 0;
    BOOL just_one_pass = (current_dialog_control_id == 0); /* if no current id, just run through once */
    BOOL forwards = (movement >= 0);
    STATUS status = STATUS_OK;
    UINT pass = 1;
    BOOL load_context_i;
    BOOL ended_this_level = 0;

    memset32(&dialog_ctl_context, 0, sizeof32(dialog_ctl_context)); /* keep dataflower happy */
    dialog_ctl_context.p_ictl_group = &p_dialog->ictls;
    load_context_i = 1;

    /* find a control to move to */
    while(!dialog_control_id)
    {
        if(load_context_i)
        {
            load_context_i = 0;

            dialog_ctl_context.i = forwards ? -1 : n_ictls_from_group(dialog_ctl_context.p_ictl_group);

            continue;
        }

        if(forwards)
        {
            if(++dialog_ctl_context.i >= n_ictls_from_group(dialog_ctl_context.p_ictl_group))
                ended_this_level = 1;
        }
        else
        {
            if(--dialog_ctl_context.i < 0)
                ended_this_level = 1;
        }

        if(ended_this_level)
        {
            ARRAY_INDEX top_element;

            ended_this_level = 0;

            if((top_element = array_elements(&context_handle) - 1) < 0)
            {
                if(just_one_pass)
                    break;

                if(pass == 2)
                {
                    assert0(); /* really we shouldn't ever get here but paranoia rules ok! */
                    break;
                }

                pass++;

                dialog_ctl_context.p_ictl_group = &p_dialog->ictls;
                load_context_i = 1;
            }
            else
            {
                /* pull tos context and keep going */
                p_dialog_ctl_context = array_ptr(&context_handle, DIALOG_CTL_CONTEXT, top_element);

                dialog_ctl_context = *p_dialog_ctl_context;

                al_array_shrink_by(&context_handle, -1);
            }

            continue;
        }

        p_dialog_ictl = p_dialog_ictl_from(dialog_ctl_context.p_ictl_group, dialog_ctl_context.i);

        if(p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_GROUPBOX)
        {
            /* push current context and keep going */
            if(NULL == (p_dialog_ctl_context = al_array_extend_by(&context_handle, DIALOG_CTL_CONTEXT, 1, &array_init_block, &status)))
                break;

            *p_dialog_ctl_context = dialog_ctl_context;

            dialog_ctl_context.p_ictl_group = &p_dialog_ictl->data.groupbox.ictls;
            load_context_i = 1;

            continue;
        }

        if(current_dialog_control_id == p_dialog_ictl->dialog_control_id)
        {
            if(!seen_current)
            {
                seen_current = 1;
                continue;
            }

            if(pass == 2)
                /* failed to find another control before this one on pass 2 */
                break;
        }

        if(p_dialog_ictl->p_dialog_control->bits.tabstop && !p_dialog_ictl->bits.disabled)
        {
            if(seen_current || just_one_pass)
                switch(p_dialog_ictl->dialog_control_type)
                {
                case DIALOG_CONTROL_EDIT:
                case DIALOG_CONTROL_BUMP_S32:
                case DIALOG_CONTROL_BUMP_F64:
                case DIALOG_CONTROL_LIST_S32:
                case DIALOG_CONTROL_LIST_TEXT:
                case DIALOG_CONTROL_COMBO_S32:
                case DIALOG_CONTROL_COMBO_TEXT:
                    dialog_control_id = p_dialog_ictl->dialog_control_id;
                    break;

                default:
                    break;
                }
        }
    }

    al_array_dispose(&context_handle);

    if(status_fail(status))
    {
        status_assertc(status);
        return(0);
    }

    return(dialog_control_id);
}

/******************************************************************************
*
* note a control in a dialog as 'current'
*
******************************************************************************/

extern void
dialog_current_set(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     BOOL disallow_movement)
{
    const DIALOG_CONTROL_ID old_current_dialog_control_id = p_dialog->current_dialog_control_id;

#if !RISCOS
    IGNOREPARM_InVal_(disallow_movement);
#endif

#if RISCOS
    if(p_dialog->current_dialog_control_id)
    {
        if(p_dialog->current_dialog_control_id != dialog_control_id)
        {
            const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog->current_dialog_control_id);
            const P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);

            if(NULL != p_dialog_ictl_edit_xx)
            {   /* scroll any edit controls back to the start when they are no longer current */
                if(p_dialog_ictl_edit_xx->readonly)
                    return;

                if(NULL != p_dialog_ictl_edit_xx->riscos.mlec)
                    mlec__cursor_texthome(p_dialog_ictl_edit_xx->riscos.mlec);
            }
        }
    }
#endif

#if RISCOS
    {
    WimpCaret caretstr;
    caretstr.window_handle = p_dialog->hwnd;
    caretstr.icon_handle = BAD_WIMP_I;
    caretstr.xoffset = 0;
    caretstr.yoffset = 0;
    caretstr.height = 1 << 25; /* invisible */
    caretstr.index = 0;
    void_WrapOsErrorReporting(wimp_set_caret_position_block(&caretstr));
    } /*block*/
#endif

    p_dialog->current_dialog_control_id = dialog_control_id;

    if(p_dialog->current_dialog_control_id != old_current_dialog_control_id)
    {
        DIALOG_MSG_CTL_CURRENT dialog_msg_ctl_current;
        P_PROC_DIALOG_EVENT p_proc_client;

        if(NULL != (p_proc_client = dialog_main_handler(p_dialog, &dialog_msg_ctl_current.client_handle)))
        {
            if(0 != p_dialog->current_dialog_control_id)
            {
                const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog->current_dialog_control_id);
                DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_current, p_dialog, p_dialog_ictl);
            }
            else
            {
                DIALOG_MSG_HDR_from_dialog(dialog_msg_ctl_current, p_dialog);
                dialog_msg_ctl_current.dialog_control_id = 0;
                dialog_msg_ctl_current.p_dialog_control = NULL;
                dialog_msg_ctl_current.p_dialog_control_data = NULL;
            }

            status_assert(dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_CURRENT, &dialog_msg_ctl_current, p_proc_client));
        }
    }

#if RISCOS
    if(p_dialog->current_dialog_control_id)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog->current_dialog_control_id);
        const P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);

        if(NULL != p_dialog_ictl_edit_xx)
        {   /* position cursor at end of any edit control */
            if(!p_dialog_ictl_edit_xx->readonly && (NULL != p_dialog_ictl_edit_xx->riscos.mlec))
            {
                mlec_claim_focus(p_dialog_ictl_edit_xx->riscos.mlec);

                if(!disallow_movement)
                    mlec__cursor_textend(p_dialog_ictl_edit_xx->riscos.mlec);
            }

            return;
        }

        switch(p_dialog_ictl->dialog_control_type)
        {
#if CHECKING
            case DIALOG_CONTROL_STATICTEXT:
            case DIALOG_CONTROL_STATICFRAME:
            case DIALOG_CONTROL_STATICPICTURE:
                assert0();
#endif
            default:
                break;

            case DIALOG_CONTROL_LIST_S32:
            case DIALOG_CONTROL_LIST_TEXT:
                {
                const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog->current_dialog_control_id);
                assert(p_dialog_ictl->data.list_xx.list_xx.riscos.lbox);
                ri_lbox_focus_set(p_dialog_ictl->data.list_xx.list_xx.riscos.lbox);
                break;
                }

            case DIALOG_CONTROL_COMBO_S32:
            case DIALOG_CONTROL_COMBO_TEXT:
                /* ensure dropdown opened ??? */
                break;
        }
    }
#endif
}

/******************************************************************************
*
* DIALOG_CMD_CODE_COMPLETE_DBOX
*
******************************************************************************/

_Check_return_
extern STATUS
dialog_cmd_complete_dbox(
    P_DIALOG_CMD_COMPLETE_DBOX p_dialog_cmd_complete_dbox)
{
    P_DIALOG p_dialog;

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_complete_dbox->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    /* set completion code to be picked up by poller */
    p_dialog->completion_code = p_dialog_cmd_complete_dbox->completion_code;
    assert(p_dialog->completion_code); /* 0 (STATUS_OK) not allowed */

    {
    P_PROC_DIALOG_EVENT p_proc_client;

    if(NULL != (p_proc_client = p_dialog->p_proc_client))
    {
        DIALOG_MSG_COMPLETE_DBOX dialog_msg_complete_dbox;
        dialog_msg_complete_dbox.h_dialog = p_dialog->h_dialog;
        dialog_msg_complete_dbox.client_handle = p_dialog->client_handle;
        dialog_msg_complete_dbox.completion_code = p_dialog->completion_code;
        status_assert(dialog_call_client(p_dialog, DIALOG_MSG_CODE_COMPLETE_DBOX, &dialog_msg_complete_dbox, p_proc_client));
    }
    } /*block*/

#if RISCOS
    /* the pointer is about to 'leave' this window */
    if(p_dialog->had_pointer)
    {
        DIALOG_RISCOS_EVENT_POINTER_ENTER dialog_riscos_event_pointer_enter;
        dialog_riscos_event_pointer_enter.h_dialog = p_dialog->h_dialog;
        dialog_riscos_event_pointer_enter.enter = 0;
        status_assert(object_call_DIALOG(DIALOG_RISCOS_EVENT_CODE_POINTER_ENTER, &dialog_riscos_event_pointer_enter));
    }
#endif

#if WINDOWS
    if(HOST_WND_NONE != p_dialog->hwnd)
    {
        if(dialog_statics.note_position)
        {
            RECT window_rect;
            GetWindowRect(p_dialog->hwnd, &window_rect);
            dialog_statics.note_position = FALSE;
            dialog_statics.noted_position = TRUE;
            dialog_statics.noted_gdi_tl.x = window_rect.left;
            dialog_statics.noted_gdi_tl.y = window_rect.top;
        }

        if(p_dialog->windows.help_engine_used)
        {
            p_dialog->windows.help_engine_used = FALSE;
            //WinHelp(p_dialog->hwnd, p_dialog->windows.help_filename, HELP_QUIT, 0L);
        }

        EndDialog(p_dialog->hwnd, TRUE);
    }
#endif

    return(STATUS_OK);
}

/******************************************************************************
*
* dispose of a dialog box
*
******************************************************************************/

extern void
dialog_dbox_dispose(
    _InVal_     H_DIALOG h_dialog)
{
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);

    if(NULL == p_dialog)
    {
        assert0();
        return;
    }

    if(0 != p_dialog->stolen_focus_.maeve_handle)
    {
        const P_DOCU p_docu = p_docu_from_docno(p_dialog->stolen_focus_.docno);
        if(!IS_DOCU_NONE(p_docu))
            maeve_event_handler_del_handle(p_docu, p_dialog->stolen_focus_.maeve_handle);
        p_dialog->stolen_focus_.maeve_handle = 0;
        p_dialog->stolen_focus_.docno = DOCNO_NONE;
    }

    dialog_ictls_dispose_in(p_dialog, &p_dialog->ictls);

    ui_text_dispose(&p_dialog->caption);

#if RISCOS
    /* a bit more extra for dboxes (after all controls deleted) */
    dialog_riscos_free_cached_bitmaps(p_dialog);

    void_WrapOsErrorReporting(winx_dispose_window(&p_dialog->hwnd));
#elif WINDOWS
    if(HOST_WND_NONE != p_dialog->hwnd)
    {
        DestroyWindow(p_dialog->hwnd);
        p_dialog->hwnd = 0;
    }

    if(p_dialog->windows.dlg_filter_hook)
    {
        UnhookWindowsHookEx(p_dialog->windows.dlg_filter_hook);
        p_dialog->windows.dlg_filter_hook = NULL;
    }

    if(p_dialog->windows.dlgtemplate)
    {
        /*GlobalUnlock(p_dialog->windows.dlgtemplate);*/ /* Unnecessary with GPTR */
        GlobalFree(p_dialog->windows.dlgtemplate);
        p_dialog->windows.dlgtemplate = 0;
    }

    if(p_dialog->windows.hfont)
    {
        if(p_dialog->windows.hfont != dialog_statics.windows.hfont)
            DeleteFont(p_dialog->windows.hfont);
        p_dialog->windows.hfont = NULL;
    }

    al_array_dispose(&p_dialog->windows.h_windows_ctl_map);
#endif

    /* send a MSG_DISPOSE iff MSG_CREATE sent */
    if(p_dialog->msg_create_sent)
    {
        DIALOG_MSG_DISPOSE dialog_msg_dispose;
        P_PROC_DIALOG_EVENT p_proc_client;
        msgclr(dialog_msg_dispose);
        p_dialog->msg_create_sent = 0;
        if(NULL != (p_proc_client = p_dialog->p_proc_client))
        {
            dialog_msg_dispose.h_dialog = p_dialog->h_dialog;
            dialog_msg_dispose.client_handle = p_dialog->client_handle;
            status_assert(dialog_call_client(p_dialog, DIALOG_MSG_CODE_DISPOSE, &dialog_msg_dispose, p_proc_client));
        }
    }

    if(0 != p_dialog->maeve_handle)
    {
        const P_DOCU p_docu = p_docu_from_docno(p_dialog->docno);
        if(!IS_DOCU_NONE(p_docu))
            maeve_event_handler_del_handle(p_docu, p_dialog->maeve_handle);
        p_dialog->maeve_handle = 0;
    }

    dialog_dbox_process_focus_return_post(p_dialog);

    /* free this DIALOG */
    al_array_delete_at(&dialog_statics.handles, -1, array_indexof_element(&dialog_statics.handles, DIALOG, p_dialog));
}

_Check_return_
extern STATUS
dialog_cmd_dispose_dbox(
    P_DIALOG_CMD_DISPOSE_DBOX p_dialog_cmd_dispose_dbox)
{
    dialog_dbox_dispose(p_dialog_cmd_dispose_dbox->h_dialog);
    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_PROCESS_DBOX
*
******************************************************************************/

/* start to steal the caret away from its current owner */

static void
dialog_dbox_process_focus_steal(
    P_DIALOG p_dialog)
{
    p_dialog->stolen_focus = 1;

#if RISCOS
    void_WrapOsErrorReporting(wimp_get_caret_position(&p_dialog->riscos.stolen_focus_caretstr));
#endif

    /* is it one of our documents that we have stolen from? */
#if RISCOS
    if(p_dialog->riscos.stolen_focus_caretstr.window_handle != (wimp_w) -1)
#endif
    {
        DOCNO docno = DOCNO_NONE;

        while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
        {
            const P_DOCU p_docu = p_docu_from_docno_valid(docno);
            const P_VIEW p_view = p_view_from_viewno_caret(p_docu);

            if(IS_VIEW_NONE(p_view))
                continue;

#if RISCOS
            if(p_view->pane[p_view->cur_pane].hwnd == p_dialog->riscos.stolen_focus_caretstr.window_handle)
#endif
            {
                STATUS maeve_handle;

                p_dialog->stolen_focus_from_doc = 1;
                p_dialog->stolen_focus_.docno = docno;
                p_dialog->stolen_focus_.focus_owner = p_docu->focus_owner;

                /*status_consume(object_skel(p_docu, T5_MSG_CARET_SHOW_CLAIM, &focus));*/ /*just claim machine focus*/

                status_assert(maeve_handle = maeve_event_handler_add(p_docu_from_docno(p_dialog->stolen_focus_.docno), maeve_event_dialog_stolen_focus, (CLIENT_HANDLE) p_dialog->h_dialog));
                p_dialog->stolen_focus_.maeve_handle = (MAEVE_HANDLE) maeve_handle;
                break;
            }
        }
    }
}

/* before closing down the dialog tree, see if it still has the input focus */

static void
dialog_dbox_process_focus_return_pre(
    P_DIALOG p_dialog)
{
#if RISCOS
    if(p_dialog->stolen_focus)
    {
        WimpCaret caretstr;

        void_WrapOsErrorReporting(wimp_get_caret_position(&caretstr));

        if(caretstr.window_handle != p_dialog->hwnd)
        {
            if(caretstr.window_handle != (wimp_w) -1) /* Window Manager may have shut us up already the git (which would return -1) */
            {
                P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog->current_dialog_control_id);

                if((NULL != p_dialog_ictl) && status_fail(dialog_riscos_ictl_focus_query(p_dialog, p_dialog_ictl)))
                    p_dialog->stolen_focus = 0;
            }
        }
    }
#else
    IGNOREPARM(p_dialog);
#endif
}

/* after closing down the dialog tree, try to return focus to rightful owner */

static void
dialog_dbox_process_focus_return_post(
    P_DIALOG p_dialog)
{
    if(p_dialog->stolen_focus)
    {
        if(p_dialog->stolen_focus_from_doc)
        {
            if(DOCNO_NONE != p_dialog->stolen_focus_.docno)
            {
                const P_DOCU p_docu = p_docu_from_docno(p_dialog->stolen_focus_.docno);
                caret_show_claim(p_docu, p_docu->focus_owner, FALSE);
            }
        }
#if RISCOS
        else
        {
            /* attempt to restore caret a la Window Manager */
            WimpGetWindowStateBlock window_state;

            window_state.window_handle = p_dialog->riscos.stolen_focus_caretstr.window_handle;
            if(!wimp_get_window_state(&window_state))
                if(WimpWindow_Open & window_state.flags)
                    void_WrapOsErrorReporting(wimp_set_caret_position_block(&p_dialog->riscos.stolen_focus_caretstr));
        }
#endif
    }

    if(0 != p_dialog->stolen_focus_.maeve_handle)
    {
        const P_DOCU p_docu = p_docu_from_docno(p_dialog->stolen_focus_.docno);
        if(!IS_DOCU_NONE(p_docu))
            maeve_event_handler_del_handle(p_docu, p_dialog->stolen_focus_.maeve_handle);
        p_dialog->stolen_focus_.maeve_handle = 0;
        p_dialog->stolen_focus_.docno = DOCNO_NONE;
    }
}

#if RISCOS

_Check_return_
static STATUS
dialog_dbox_process_riscos(
    P_DIALOG p_dialog,
    P_DIALOG_CMD_PROCESS_DBOX p_dialog_cmd_process_dbox,
    _InRef_     PC_PIXIT_RECT p_pixit_rect)
{
    DIALOG_POSITION_TYPE dialog_position_type;
    S32 menu_requested;
    S32 menu_used;
    const GDI_SIZE screen_gdi_size = host_modevar_cache_current.gdi_size;
    GDI_POINT gdi_tl = { 0, 0 }; /* actually OS units */ /* dataflower */
    GDI_COORD overhead;

    { /*a*/
    WimpWindowWithBitset wind_defn;
    GDI_SIZE gdi_size;
    GDI_BOX gdi_box;

    zero_struct(wind_defn);

    wind_defn.behind = (wimp_w) -1; /* open at the top of the window stack */

    wind_defn.title_fg = '\x07'; /* black  */
    wind_defn.title_bg = '\x02'; /* light grey */
    wind_defn.work_fg = '\x07'; /* black */
    wind_defn.work_bg = '\x01'; /* light grey */
    wind_defn.scroll_outer = '\x03'; /* mid grey */
    wind_defn.scroll_inner = '\x01'; /* light grey */
    wind_defn.highlight_bg = wind_defn.title_bg;

    wind_defn.work_flags.bits.button_type = ButtonType_DoubleClickDrag;
    wind_defn.sprite_area = (void *) 1; /* Window Manager sprite area (needed to satisfy RISC PC) */
    wind_defn.min_width = 1;
    wind_defn.min_height = 1;

    wind_defn.flags.bits.flags_are_new = 1;
    wind_defn.flags.bits.moveable = 1;

    if(p_dialog->riscos.caption)
    {
        wind_defn.flags.bits.has_title = 1;

        wind_defn.title_flags.bits.text        = 1;
        wind_defn.title_flags.bits.horz_centre = 1;
        wind_defn.title_flags.bits.indirect    = 1;

        wind_defn.titledata.it.buffer = p_dialog->riscos.caption;
        wind_defn.titledata.it.buffer_size = strlen32p1(wind_defn.titledata.it.buffer); /*CH_NULL*/
    }

    dialog_riscos_box_from_pixit_rect(&gdi_box, p_pixit_rect);

    /* create offsets such that DIALOG_CONTROL_PARENT no longer neccesarily refers to this tl point! */
    /* add top and left margins */
    gdi_box.x0 -= +(DIALOG_BOX_LM / PIXITS_PER_RISCOS);
    gdi_box.y1 -= -(DIALOG_BOX_TM / PIXITS_PER_RISCOS);

    p_dialog->riscos.gdi_offset_tl.x = gdi_box.x0;
    p_dialog->riscos.gdi_offset_tl.y = gdi_box.y1;

    gdi_size.cx = gdi_box.x1 - gdi_box.x0;
    gdi_size.cy = gdi_box.y1 - gdi_box.y0;

    /* round size to worst possible pixel granularity */
    gdi_size.cx = 4 * div_round_ceil_u(gdi_size.cx, 4);
    gdi_size.cy = 4 * div_round_ceil_u(gdi_size.cy, 4);

    menu_requested = p_dialog_cmd_process_dbox->bits.use_riscos_menu
                   ? p_dialog_cmd_process_dbox->riscos.menu
                   : DIALOG_RISCOS_MENU;

    if(DIALOG_RISCOS_MENU == menu_requested)
    {
        if(event_query_submenudata_valid())
        {   /* mustn't round posn to worst possible pixel granularity as Window Manager will get upset */
            status_assert(event_read_submenudata(NULL, (int *) &gdi_tl.x, (int *) &gdi_tl.y)); /* ask about menu position */
        }
        else
        {   /* degrade request to being a standalone menu (i.e. not submenu) */
            menu_requested = DIALOG_RISCOS_STANDALONE_MENU;
        }
    }

    menu_used = menu_requested;

    dialog_riscos_dbox_modify_open_type(p_dialog, &menu_used);

    dialog_position_type = ENUM_UNPACK(DIALOG_POSITION_TYPE, p_dialog_cmd_process_dbox->bits.dialog_position_type);

    if(DIALOG_POSITION_DEFAULT == dialog_position_type)
        dialog_position_type = DIALOG_POSITION_NEAR_MOUSE;

    if(DIALOG_RISCOS_MENU != menu_requested) /* otherwise we already have the position */
    switch(dialog_position_type)
    {
    case DIALOG_POSITION_CENTRE_WINDOW: /* unsupported - default to screen */
    case DIALOG_POSITION_CENTRE_SCREEN:
        gdi_tl.x = (screen_gdi_size.cx - gdi_size.cx) / 2;
        gdi_tl.y = (screen_gdi_size.cy + gdi_size.cy) / 2;

        /* round posn to worst possible pixel granularity */
        gdi_tl.x = 4 * div_round_floor(gdi_tl.x, 4);
        gdi_tl.y = 4 * div_round_floor(gdi_tl.y, 4);
        break;

    default: default_unhandled();
#if CHECKING
    case DIALOG_POSITION_NEAR_MOUSE:
#endif
        switch(menu_requested)
        {
        case DIALOG_RISCOS_MENU:
            break;

        default: default_unhandled();
#if CHECKING
        case DIALOG_RISCOS_STANDALONE_MENU:
        case DIALOG_RISCOS_NOT_MENU:
#endif
            if(dialog_statics.noted_gdi_tl.x && dialog_statics.noted_gdi_tl.y)
            {
                gdi_tl = dialog_statics.noted_gdi_tl;

                dialog_statics.noted_gdi_tl.x = 0;
                dialog_statics.noted_gdi_tl.y = 0;
            }
            else
            {
                WimpGetPointerInfoBlock m;

                void_WrapOsErrorReporting(wimp_get_pointer_info(&m));

                gdi_tl.x = m.x - 64 /*32*/; /* try to be a bit into the window */
                gdi_tl.y = m.y + 64 /*32*/;

                /* round posn to worst possible pixel granularity */
                gdi_tl.x = 4 * div_round_floor(gdi_tl.x, 4);
                gdi_tl.y = 4 * div_round_floor(gdi_tl.y, 4);
            }
            break;
        }
        break;
    }

    wind_defn.visible_area.xmin = gdi_tl.x;
    wind_defn.visible_area.ymax = gdi_tl.y;

    wind_defn.visible_area.xmax = wind_defn.visible_area.xmin + gdi_size.cx;
    wind_defn.visible_area.ymin = wind_defn.visible_area.ymax - gdi_size.cy;

    /* try not to overlap icon bar much */
    if(wimp_iconbar_height > wind_defn.visible_area.ymin)
    {
        const int shift_y = wimp_iconbar_height - wind_defn.visible_area.ymin;
        wind_defn.visible_area.ymin += shift_y;
        wind_defn.visible_area.ymax += shift_y;
    }

    /* make initial scrolls */
    wind_defn.xscroll = 0;
    wind_defn.yscroll = 0;

    wind_defn.extent.xmin = - wind_defn.xscroll; /* top left */
    wind_defn.extent.ymax = - wind_defn.yscroll;

    wind_defn.extent.xmax = wind_defn.extent.xmin + gdi_size.cx; /* bottom right */
    wind_defn.extent.ymin = wind_defn.extent.ymax - gdi_size.cy;

    overhead = 0;
    if(wind_defn.flags.bits.has_vert_scroll)
        overhead += wimp_win_vscroll_width(4);

    if(gdi_size.cx > screen_gdi_size.cx - overhead)
    {
        gdi_size.cx = screen_gdi_size.cx - overhead;
        wind_defn.visible_area.xmax = wind_defn.visible_area.xmin + gdi_size.cx;
        wind_defn.flags.bits.has_horz_scroll = 1;
        wind_defn.flags.bits.has_adjust_size = 1;
    }

    overhead = 0;
    if(wind_defn.flags.bits.has_title)
        overhead += wimp_win_title_height(4);
    if(wind_defn.flags.bits.has_horz_scroll)
        overhead += wimp_win_hscroll_height(4);

    if(gdi_size.cy > screen_gdi_size.cy - overhead)
    {
        gdi_size.cy = screen_gdi_size.cy - overhead;
        wind_defn.visible_area.ymax = wind_defn.visible_area.ymin + gdi_size.cy;
        wind_defn.flags.bits.has_vert_scroll = 1;
        wind_defn.flags.bits.has_adjust_size = 1;
    }

    /* it may be necessary to force the dialog box to have a close icon */
    if(p_dialog->modeless || ((DIALOG_RISCOS_NOT_MENU == menu_used) && (NULL == p_dialog_ictl_from_control_id(p_dialog, IDCANCEL))))
    {
        wind_defn.flags.bits.has_close = 1;
        wind_defn.flags.bits.has_title = 1;
    }

    status_return(winx_create_window(&wind_defn, &p_dialog->hwnd, dialog_riscos_dbox_event_handler, (P_ANY) p_dialog->h_dialog));
    } /*a*/ /*block*/

    /* <<< any WM_INITDIALOG-like initialisation for RISC OS goes here*/

    /* loop over DIALOG_ICTLs creating control representations and logging data */
    status_return(dialog_riscos_ictls_create_in(p_dialog, &p_dialog->ictls));

    if(p_dialog->modeless)
        /* bring window up now (without input focus?) */
        winx_send_front_window_request(p_dialog->hwnd, TRUE, NULL);
    else
    {
        switch(menu_used)
        {
        case DIALOG_RISCOS_MENU:
            /* make window a submenu window */
            winx_create_submenu(p_dialog->hwnd, gdi_tl.x, gdi_tl.y);
            break;

        case DIALOG_RISCOS_STANDALONE_MENU:
            /* make window a menu window */
            winx_create_menu(p_dialog->hwnd, gdi_tl.x, gdi_tl.y);
            break;

        default: default_unhandled();
#if CHECKING
        case DIALOG_RISCOS_NOT_MENU:
#endif
            /* behave more like a modal submenu type window */
            /*if(menu_requested != DIALOG_RISCOS_NOT_MENU) - what on earth is this???*/
                winx_create_complex_menu(p_dialog->hwnd);

            /* bring window up now - otherwise polling loop will detect the fact it ain't open! */
            winx_send_front_window_request(p_dialog->hwnd, TRUE, NULL);
            break;
        }
    }

    /* loop over these new control representations encoding them and enabling them as appropriate */
    dialog_ictls_encode_in(p_dialog, &p_dialog->ictls, 0);
    dialog_ictls_enable_in(p_dialog, &p_dialog->ictls);

    { /* tell client that dialog processing is about to start */
    P_PROC_DIALOG_EVENT p_proc_client;
    DIALOG_MSG_PROCESS_START dialog_msg_process_start;
    msgclr(dialog_msg_process_start);
    dialog_msg_process_start.initial_focus_dialog_control_id = 0;
    if(NULL != (p_proc_client = p_dialog->p_proc_client))
    {
        dialog_msg_process_start.h_dialog = p_dialog->h_dialog;
        dialog_msg_process_start.client_handle = p_dialog->client_handle;
        status_assert(dialog_call_client(p_dialog, DIALOG_MSG_CODE_PROCESS_START, &dialog_msg_process_start, p_proc_client));
    }

    if(!p_dialog->modeless)
    {
        /* can only set the focus into a window that has been opened */
        dialog_current_set(p_dialog, 0, FALSE);

        {
        DIALOG_CMD_CTL_FOCUS_SET dialog_cmd_ctl_focus_set;
        msgclr(dialog_cmd_ctl_focus_set);
        dialog_cmd_ctl_focus_set.h_dialog = p_dialog->h_dialog;
        dialog_cmd_ctl_focus_set.dialog_control_id =
            dialog_msg_process_start.initial_focus_dialog_control_id
            ? dialog_msg_process_start.initial_focus_dialog_control_id
            : dialog_current_move(p_dialog, 0, 0);
        if(dialog_cmd_ctl_focus_set.dialog_control_id)
            status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_FOCUS_SET, &dialog_cmd_ctl_focus_set));
        } /*block*/

        host_key_buffer_flush();
    }
    } /*block*/

    return(STATUS_OK);
}

#endif /* RISCOS */

/******************************************************************************
*
* create a dialog box structure
*
******************************************************************************/

_Check_return_
static STATUS
dialog_dbox_create(
    _DocuRef_   P_DOCU p_docu,
    P_DIALOG_CMD_PROCESS_DBOX p_dialog_cmd_process_dbox,
    /*out*/ P_DIALOG * p_p_dialog)
{
    P_DIALOG p_dialog;
    H_DIALOG h_dialog;
    STATUS status;

    p_dialog_cmd_process_dbox->modeless_h_dialog = 0;

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_dialog), TRUE);
    if(NULL == (p_dialog = al_array_extend_by(&dialog_statics.handles, DIALOG, 1, &array_init_block, &status)))
        return(status);
    } /*block*/

    /* never the same key twice, always non-zero */
    h_dialog = (H_DIALOG) ++dialog_statics.handle_gen;

    p_dialog->h_dialog = h_dialog;

    p_dialog->client_handle = p_dialog_cmd_process_dbox->client_handle;
    p_dialog->p_proc_client = p_dialog_cmd_process_dbox->p_proc_client;

    p_dialog->help_topic_resource_id = p_dialog_cmd_process_dbox->help_topic_resource_id;

    p_dialog->docno = docno_from_p_docu(p_docu);

    if(p_dialog->docno)
    {
        if(status_fail(status = maeve_event_handler_add(p_docu_from_docno(p_dialog->docno), maeve_event_dialog, (CLIENT_HANDLE) p_dialog->h_dialog)))
        {
            al_array_delete_at(&dialog_statics.handles, -1, array_indexof_element(&dialog_statics.handles, DIALOG, p_dialog));
            return(status);
        }

        p_dialog->maeve_handle = (MAEVE_HANDLE) status;
    }

    {
    DIALOG_MSG_CREATE dialog_msg_create;
    P_PROC_DIALOG_EVENT p_proc_client;
    msgclr(dialog_msg_create);
    if(NULL != (p_proc_client = p_dialog->p_proc_client))
    {
        dialog_msg_create.h_dialog = p_dialog->h_dialog;
        dialog_msg_create.client_handle = p_dialog->client_handle;
        status_assert(status = dialog_call_client(p_dialog, DIALOG_MSG_CODE_CREATE, &dialog_msg_create, p_proc_client));
        if(status_ok(status))
            p_dialog->msg_create_sent = 1;
    }
    else
        status = STATUS_OK;
    } /*block*/

    if(status_ok(status))
        status = dialog_ictls_create(p_dialog, p_dialog_cmd_process_dbox->n_ctls, p_dialog_cmd_process_dbox->p_ctl_create);

    if(status_fail(status))
    {
        dialog_dbox_dispose(p_dialog->h_dialog);
        return(status);
    }

    *p_p_dialog = p_dialog;

    return(STATUS_OK);
}

T5_MSG_PROTO(extern, dialog_dbox_process, P_DIALOG_CMD_PROCESS_DBOX p_dialog_cmd_process_dbox)
{
    P_DIALOG p_dialog;
    STATUS status = STATUS_OK;

    IGNOREPARM_InVal_(t5_message);

    assert(p_dialog_cmd_process_dbox->caption.type != 0xBCBCBCBC);
    assert(* (PC_U32) &p_dialog_cmd_process_dbox->bits != (U32) 0xBCBCBCBC);

    status_return(dialog_dbox_create(p_docu, p_dialog_cmd_process_dbox, &p_dialog));

    p_dialog->modeless = p_dialog_cmd_process_dbox->bits.modeless;

    dialog_statics.note_position = p_dialog_cmd_process_dbox->bits.note_position;

    for(;;) /* loop for structure */
    { /*1*/
        PIXIT_RECT pixit_rect;

        if(p_dialog_cmd_process_dbox->caption.type != UI_TEXT_TYPE_NONE)
        {
            status_break(status = ui_text_copy(&p_dialog->caption, &p_dialog_cmd_process_dbox->caption));

#if RISCOS
            status_break(status = ui_text_copy_as_sbstr(&p_dialog->riscos.caption, &p_dialog->caption));
#endif
        }

        dialog_dbox_process_focus_steal(p_dialog);

        /* loop over DIALOG_ICTLs to determine size of dialog */
        gr_box_make_bad((P_GR_BOX) &pixit_rect);
        status_assert(dialog_ictls_bbox_in(p_dialog, &p_dialog->ictls, &pixit_rect));

        /* add right and bottom margins (still in pixit space) */
        pixit_rect.br.x += DIALOG_BOX_RM;
        pixit_rect.br.y += DIALOG_BOX_BM;

#if RISCOS
        status = dialog_dbox_process_riscos(p_dialog, p_dialog_cmd_process_dbox, &pixit_rect);
#elif WINDOWS
        /* add top and left margins (still in pixit space) */
        pixit_rect.tl.x -= DIALOG_BOX_LM;
        pixit_rect.tl.y -= DIALOG_BOX_TM;

        status = dialog_dbox_process_windows(p_dialog, p_dialog_cmd_process_dbox, &pixit_rect);
#endif

        status_break(status);

        if(p_dialog->modeless)
        {   /* let modeless events come in through normal mechanisms. they will need separate cancelling */
            p_dialog_cmd_process_dbox->modeless_h_dialog = p_dialog->h_dialog;
            return(STATUS_OK);
        }

#if RISCOS /* Windows has already done this */
        {
        const H_DIALOG h_dialog = p_dialog->h_dialog;

        /* modal dialogs: loop processing one event at a time using fg null events until dialog process is complete */
        p_dialog_cmd_process_dbox->modal_completion_code = 0;

        while(!p_dialog->completion_code)
        {
            trace_2(TRACE_APP_DIALOG, TEXT("modal dialog ") PTR_XTFMT TEXT(" about to poll: sp ~= ") PTR_XTFMT, h_dialog, &p_dialog);

            (void) wm_event_get(1); /* suck foreground null events */

            /* check for dialog going away under our feet */
            p_dialog = p_dialog_from_h_dialog(h_dialog);

            if(NULL == p_dialog)
            {
                trace_1(TRACE_APP_DIALOG, TEXT("modal dialog ") PTR_XTFMT TEXT(" abnormal completion: code = 0"), h_dialog);
                return(p_dialog_cmd_process_dbox->modal_completion_code = DIALOG_COMPLETION_CANCEL);
            }
        }
        } /*block*/
#endif /* RISCOS */

        p_dialog_cmd_process_dbox->modal_completion_code = p_dialog->completion_code;

#if RISCOS
        /* Windows has already done this */
        dialog_statics.noted_position = FALSE;
        dialog_statics.noted_gdi_tl.x = 0;
        dialog_statics.noted_gdi_tl.y = 0;

        if(dialog_statics.note_position)
        {
            dialog_statics.note_position = FALSE;

            if(p_dialog_cmd_process_dbox->modal_completion_code > 0)
            {   /* read last window position, esp for tweak style dialog */
                WimpGetWindowStateBlock window_state;

                window_state.window_handle = p_dialog->hwnd;
                if(!wimp_get_window_state(&window_state))
                {
                    dialog_statics.noted_position = TRUE;
                    dialog_statics.noted_gdi_tl.x = window_state.visible_area.xmin;
                    dialog_statics.noted_gdi_tl.y = window_state.visible_area.ymax;
                }
            }
        }
#endif

        if(NULL != p_dialog)
        {
            DIALOG_MSG_PROCESS_END dialog_msg_process_end;
            P_PROC_DIALOG_EVENT p_proc_client;
            msgclr(dialog_msg_process_end);
            if(NULL != (p_proc_client = p_dialog->p_proc_client))
            {
                dialog_msg_process_end.h_dialog = p_dialog->h_dialog;
                dialog_msg_process_end.client_handle = p_dialog->client_handle;
                dialog_msg_process_end.completion_code = p_dialog->completion_code;
                status_assert(status = dialog_call_client(p_dialog, DIALOG_MSG_CODE_PROCESS_END, &dialog_msg_process_end, p_proc_client));
                p_dialog->completion_code = dialog_msg_process_end.completion_code;
            }

            dialog_dbox_process_focus_return_pre(p_dialog);
        }

        status = p_dialog_cmd_process_dbox->modal_completion_code;

        break; /* out of loop for structure */
        /*NOTREACHED*/
    } /*1*/

    trace_2(TRACE_APP_DIALOG, TEXT("modal dialog ") H_DIALOG_XTFMT TEXT(" completing: code = ") S32_TFMT, p_dialog->h_dialog, p_dialog_cmd_process_dbox->modal_completion_code);

    dialog_dbox_dispose(p_dialog->h_dialog);

    return(status);
}

/* end of ob_dlg2.c */
