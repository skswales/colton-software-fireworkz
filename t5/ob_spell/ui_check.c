/* ui_check.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Spelling checker UI for Wordz */

/* RCM Nov 1992 */

#include "common/gflags.h"

#include "ob_spell/ob_spell.h"

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#define N_CHECK_LIST_ITEMS 5       /* number of words shown by guess word list in CHECK DBOX */

typedef struct CHECK_CALLBACK
{
    P_WORD_CHECK p_word_check;
    PC_USTR ustr_init_word;
    STATUS status;
    UCHARZ ustr_replace[BUF_MAX_WORD];
    UI_SOURCE ui_source;
}
CHECK_CALLBACK, * P_CHECK_CALLBACK;

_Check_return_
static STATUS
build_guesses(
    _DocuRef_   P_DOCU p_docu,
    P_CHECK_CALLBACK p_check_callback,
    _In_z_      PC_USTR ustr)
{
    STATUS status = STATUS_OK;

    ui_source_dispose(&p_check_callback->ui_source);

    if(ustr && (ustrlen32(ustr) <= 20 /* 'cos big words take too long */) && status_ok(status = guess_init(ustr)))
    {
        U8Z word[BUF_MAX_WORD];

        while((status = guess_next(p_docu, word, sizeof32(word))) > 0)
        {
            ARRAY_INDEX list_index;
            int match = -1;

            if(status != 2)
                continue;

            trace_1(TRACE_APP_TYPE5_SPELL, TEXT("found '%s'"), report_sbstr(word));

            for(list_index = 0; list_index < array_elements(&p_check_callback->ui_source.source.array_handle); ++list_index)
            {
                P_UI_TEXT p_ui_text = array_ptr(&p_check_callback->ui_source.source.array_handle, UI_TEXT, list_index);
                if((match = /*"C"*/strcmp(word, _sbstr_from_ustr(ui_text_ustr(p_ui_text))))<= 0)
                    break;
            }

            if(match != 0)
            {   /* insert at alphabetic place in list - ought to change to have an idea of sense - transposition/like/more/less */
                P_UI_TEXT p_ui_text;
                if(NULL == (p_ui_text = al_array_insert_before_UI_TEXT(&p_check_callback->ui_source.source.array_handle, 1, &array_init_block_ui_text, &status, list_index)))
                    break;
                p_check_callback->ui_source.type = UI_SOURCE_TYPE_ARRAY;
                status_break(status = ui_text_alloc_from_tstr(p_ui_text, _tstr_from_sbstr(word)));
            }
        }

        guess_end();
    }

    if(status_fail(status))
        ui_source_dispose(&p_check_callback->ui_source);

    return(status);
}

enum CHECK_CONTROL_IDS
{
    CONTROL_ID_REPLACE = IDOK,

    CONTROL_ID_WORD = 64,
    CONTROL_ID_GUESS,
    CONTROL_ID_LIST,
    CONTROL_ID_ADD,
    CONTROL_ID_SKIP
};

#define CONTROL_BUTT_H (DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARS_H(" Replace"))

static const DIALOG_CONTROL
dict_guess =
{
    CONTROL_ID_GUESS, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_PARENT, CONTROL_ID_WORD, DIALOG_CONTROL_SELF, CONTROL_ID_WORD },
    { 0, 0, CONTROL_BUTT_H, 0 },
    { DRT(LTLB, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
dict_guess_data = { { 0 }, UI_TEXT_INIT_RESID(OB_SPELL_MSG_GUESS) };

static const DIALOG_CONTROL
dict_word =
{
    CONTROL_ID_WORD, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_GUESS, DIALOG_CONTROL_PARENT },
    { DIALOG_STDSPACING_H, 0, (16 * PIXITS_PER_INCH) / 8, DIALOG_STDEDIT_V },
    { DRT(RTLT, EDIT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_EDIT
dict_word_data = { { { FRAMED_BOX_EDIT } }, /*EDIT_XX*/ { UI_TEXT_TYPE_NONE } /* UI_TEXT state */ };

static const DIALOG_CONTROL
dict_list =
{
    CONTROL_ID_LIST, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_WORD, CONTROL_ID_WORD, CONTROL_ID_WORD, IDCANCEL },
    { 0, DIALOG_STDSPACING_V, 0, 0 },
    { DRT(LBRB, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
dict_add =
{
    CONTROL_ID_ADD, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_GUESS, CONTROL_ID_GUESS, CONTROL_ID_GUESS },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL_ID
dict_add_data_argmap[] = { CONTROL_ID_WORD };

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
dict_add_command = { T5_CMD_SPELL_DICTIONARY_ADD_WORD, OBJECT_ID_SPELL, NULL, dict_add_data_argmap, { 0, 0, 0, 1 /*lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
dict_add_data = { { 0 }, UI_TEXT_INIT_RESID(OB_SPELL_MSG_ADD_WORD), &dict_add_command };

static const DIALOG_CONTROL
dict_replace =
{
    CONTROL_ID_REPLACE, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_ADD, CONTROL_ID_ADD, CONTROL_ID_ADD },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
dict_replace_data = { { CONTROL_ID_REPLACE }, UI_TEXT_INIT_RESID(OB_SPELL_MSG_REPLACE) };

static const DIALOG_CONTROL
dict_skip =
{
    CONTROL_ID_SKIP, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_REPLACE, CONTROL_ID_REPLACE, CONTROL_ID_REPLACE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
dict_skip_data = { { CONTROL_ID_SKIP }, UI_TEXT_INIT_RESID(OB_SPELL_MSG_SKIP) };

static const DIALOG_CONTROL
dict_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_SKIP, CONTROL_ID_SKIP, CONTROL_ID_SKIP },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CTL_CREATE
check_dialog_create[] =
{
    { &dict_guess, &dict_guess_data },
    { &dict_word, &dict_word_data },
    { &dict_list, &stdlisttext_data },
    { &dict_add, &dict_add_data },
    { &dict_replace, &dict_replace_data },
    { &dict_skip, &dict_skip_data },
    { &dict_cancel, &stdbutton_cancel_data } /* SKS 02aug94 added Cancel button */
};

_Check_return_
static STATUS
dialog_check_ctl_fill_source(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    const P_CHECK_CALLBACK p_check_callback = (P_CHECK_CALLBACK) p_dialog_msg_ctl_fill_source->client_handle;

    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case CONTROL_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &p_check_callback->ui_source;
        return(build_guesses(p_docu, p_check_callback, p_check_callback->ustr_init_word));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
dialog_check_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_CHECK_CALLBACK p_check_callback = (P_CHECK_CALLBACK) p_dialog_msg_process_start->client_handle;
    STATUS status = STATUS_OK;

    ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, CONTROL_ID_GUESS, 0); /* disable guess button until user changes the word */

    {
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = p_dialog_msg_process_start->h_dialog;
    dialog_cmd_ctl_state_set.dialog_control_id = CONTROL_ID_WORD,
    dialog_cmd_ctl_state_set.bits = 0;
    dialog_cmd_ctl_state_set.state.edit.ui_text.type = UI_TEXT_TYPE_USTR_PERM;
    dialog_cmd_ctl_state_set.state.edit.ui_text.text.ustr = p_check_callback->ustr_init_word;
    status_assert(status = object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
    } /*block*/

    if(status_ok(status) && (0 != array_elements(&p_check_callback->ui_source.source.array_handle)))
        status = ui_dlg_set_list_idx(p_dialog_msg_process_start->h_dialog, CONTROL_ID_LIST, 0); /* keep Adrienne happy */

    return(status);
}

_Check_return_
static STATUS
dialog_check_ctl_pushbutton(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    const P_CHECK_CALLBACK p_check_callback = (P_CHECK_CALLBACK) p_dialog_msg_ctl_pushbutton->client_handle;
    STATUS status = STATUS_OK;

    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case CONTROL_ID_GUESS:
        ui_source_dispose(&p_check_callback->ui_source);

        /* we've just done a guess, so disable the guess button until the user changes the word */
        ui_dlg_ctl_enable(p_dialog_msg_ctl_pushbutton->h_dialog, CONTROL_ID_GUESS, 0);

        {
        UI_TEXT ui_text;
        ui_dlg_get_edit(p_dialog_msg_ctl_pushbutton->h_dialog, CONTROL_ID_WORD, &ui_text);
        status_assert(status = build_guesses(p_docu, p_check_callback, ui_text_ustr(&ui_text)));
        ui_text_dispose(&ui_text);
        } /*block*/

        ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, CONTROL_ID_LIST);
        break;

    case CONTROL_ID_ADD:
        p_check_callback->status = CHECK_CONTINUE; /* just about to execute command */
        break;

    case CONTROL_ID_REPLACE:
        {
        OBJECT_STRING_REPLACE object_string_replace;
        QUICK_UBLOCK quick_ublock;
        quick_ublock_setup_fill_from_ubuf(&quick_ublock, ustr_bptr(p_check_callback->ustr_replace), ustrlen32(ustr_bptr(p_check_callback->ustr_replace)));

        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

        object_string_replace.p_quick_ublock = &quick_ublock;
        object_string_replace.object_data = p_check_callback->p_word_check->object_data;
        object_string_replace.copy_capitals = (ob_spell_isupper(PtrGetByte(p_check_callback->ustr_replace)) <= 0);
        p_check_callback->status = object_call_id(object_string_replace.object_data.object_id, p_docu, T5_MSG_OBJECT_STRING_REPLACE, &object_string_replace);

        if(status_ok(p_check_callback->status))
        {
            /* MRJC: feed back possibly altered object pointer! */
            p_check_callback->p_word_check->object_data.u.p_object = object_string_replace.object_data.u.p_object;

            docu_modify(p_docu);

            p_check_callback->status = CHECK_CONTINUE_RECHECK;
        }

        quick_ublock_dispose(&quick_ublock);
        break;
        }

    case CONTROL_ID_SKIP:
        if(status_ok(p_check_callback->status = skip_list_add_word(p_docu, p_check_callback->ustr_init_word)))
            p_check_callback->status = CHECK_CONTINUE;
        break;

    default:
        break;
    }

    return(status);
}

_Check_return_
static STATUS
dialog_check_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_CHECK_CALLBACK p_check_callback = (P_CHECK_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case CONTROL_ID_WORD:
        if(ui_text_is_blank(&p_dialog_msg_ctl_state_change->new_state.edit.ui_text))
            PtrPutByte(p_check_callback->ustr_replace, CH_NULL);
        else
            ustr_xstrnkpy(ustr_bptr(p_check_callback->ustr_replace), sizeof32(p_check_callback->ustr_replace), ui_text_ustr(&p_dialog_msg_ctl_state_change->new_state.edit.ui_text), MAX_WORD);

        /* user has changed the word, so enable the guess button iff sensible */
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, CONTROL_ID_GUESS, (PtrGetByte(p_check_callback->ustr_replace) != CH_NULL));
        break;

    case CONTROL_ID_LIST:
        if(ui_text_is_blank(&p_dialog_msg_ctl_state_change->new_state.list_text.ui_text))
            PtrPutByte(p_check_callback->ustr_replace, CH_NULL);
        else
            ustr_xstrnkpy(ustr_bptr(p_check_callback->ustr_replace), sizeof32(p_check_callback->ustr_replace), ui_text_ustr(&p_dialog_msg_ctl_state_change->new_state.list_text.ui_text), MAX_WORD);
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_check)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_check_ctl_fill_source(p_docu, (P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_check_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_check_ctl_pushbutton(p_docu, (P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_check_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
t5_cmd_spell_check(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;
    SCAN_BLOCK scan_block;
    OBJECT_CHECK object_check;

    p_docu->spelling_mistakes = 0;

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_DOWN, p_docu->mark_info_cells.h_markers ? SCAN_MARKERS : SCAN_WHOLE, NULL, OBJECT_ID_NONE)))
    {
        while(status_ok(status) && status_done(cells_scan_next(p_docu, &object_check.object_data, &scan_block)))
        {
            object_check.status = CHECK_CONTINUE;
            status = object_call_id(object_check.object_data.object_id, p_docu, T5_MSG_OBJECT_CHECK, &object_check);
            if(object_check.status == CHECK_CANCEL)
                break;
        }
    }

    status_line_setf(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR,
                    (p_docu->spelling_mistakes == 0)
                        ? OB_SPELL_MSG_CHECK_COMPLETE
                        : (p_docu->spelling_mistakes == 1)
                        ? OB_SPELL_MSG_CHECK_WITH_ERR
                        : OB_SPELL_MSG_CHECK_WITH_ERRS,
                    p_docu->spelling_mistakes);

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    return(status);
}

/******************************************************************************
*
* Put up a dbox to report a misspelt/unknown word. Allows the user
* to add the word to a dictionary, skip it or replace it.
*
* returns <0 error
*         CHECK_CONTINUE         if word added to dictionary or skipped
*         CHECK_CONTINUE_RECHECK if word replaced by another
*         CHECK_CANCEL
*
******************************************************************************/

_Check_return_
extern STATUS
ui_check_dbox(
    _DocuRef_   P_DOCU p_docu,
    P_WORD_CHECK p_word_check,
    _In_z_      PC_USTR ustr_init_word)
{
    STATUS status = STATUS_OK;
    CHECK_CALLBACK check_callback;
    zero_struct(check_callback);

    check_callback.p_word_check = p_word_check;
    check_callback.ustr_init_word = ustr_init_word;
    check_callback.status = CHECK_CONTINUE;

    assert(ustrlen32(ustr_init_word) <= MAX_WORD);
    ustr_xstrnkpy(ustr_bptr(check_callback.ustr_replace), sizeof32(check_callback.ustr_replace), ustr_init_word, MAX_WORD);

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, check_dialog_create, elemof32(check_dialog_create), OB_SPELL_MSG_CHECK_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = OB_SPELL_MSG_CHECK_CAPTION;
    dialog_cmd_process_dbox.bits.note_position = 1;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_check;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &check_callback;
    status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    if(status_ok(status))
        status = check_callback.status;
    else if(status == STATUS_CANCEL)
        status = CHECK_CANCEL;

    ui_source_dispose(&check_callback.ui_source);

    status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

    return(status);
}

/* end of ui_check.c */
