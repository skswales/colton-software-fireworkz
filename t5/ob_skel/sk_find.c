/* sk_find.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Search & replace */

/* SKS December 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_find);

_Check_return_
static STATUS
sk_find_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    p_docu->h_arglist_search = 0;
    return(STATUS_OK);
}

_Check_return_
static STATUS
sk_find_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    arglist_dispose(&p_docu->h_arglist_search);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_sk_find_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT1:
        return(sk_find_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(sk_find_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_find)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_find_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

enum SEARCH_QUERY_CONTROL_IDS
{
    SEARCH_QUERY_ID_CANCEL = IDCANCEL,

    SEARCH_QUERY_ID_NEXT = 348,
    SEARCH_QUERY_ID_REPLACE,
    SEARCH_QUERY_ID_REPLACE_ALL
};

enum SEARCH_QUERY_COMPLETION_CODE
{
    SEARCH_QUERY_COMPLETION_NEXT = 456,
    SEARCH_QUERY_COMPLETION_REPLACE,
    SEARCH_QUERY_COMPLETION_REPLACE_ALL
};

#define SEARCH_QUERY_BUTTONS_H (DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARS_H("Replace all"))

/*
next
*/

static const DIALOG_CONTROL
search_query_next =
{
    SEARCH_QUERY_ID_NEXT, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, SEARCH_QUERY_BUTTONS_H, DIALOG_STDPUSHBUTTON_V },
    { DRT(LTLT, PUSHBUTTON), 1}
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
search_query_next_data = { { SEARCH_QUERY_COMPLETION_NEXT }, UI_TEXT_INIT_RESID(MSG_DIALOG_SEARCH_QUERY_NEXT) };

/*
replace
*/

static const DIALOG_CONTROL
search_query_replace =
{
    SEARCH_QUERY_ID_REPLACE, DIALOG_MAIN_GROUP,
    { SEARCH_QUERY_ID_NEXT, SEARCH_QUERY_ID_NEXT, SEARCH_QUERY_ID_NEXT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1}
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
search_query_replace_data = { { SEARCH_QUERY_COMPLETION_REPLACE }, UI_TEXT_INIT_RESID(MSG_REPLACE) };

/*
replace all
*/

static const DIALOG_CONTROL
search_query_replace_all =
{
    SEARCH_QUERY_ID_REPLACE_ALL, DIALOG_MAIN_GROUP,
    { SEARCH_QUERY_ID_REPLACE, SEARCH_QUERY_ID_REPLACE, SEARCH_QUERY_ID_REPLACE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1}
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
search_query_replace_all_data = { { SEARCH_QUERY_COMPLETION_REPLACE_ALL }, UI_TEXT_INIT_RESID(MSG_DIALOG_SEARCH_QUERY_REPLACE_ALL) };

/*
cancel
*/

static const DIALOG_CONTROL
search_query_cancel =
{
    SEARCH_QUERY_ID_CANCEL, DIALOG_MAIN_GROUP,
    { SEARCH_QUERY_ID_REPLACE_ALL, SEARCH_QUERY_ID_REPLACE_ALL, SEARCH_QUERY_ID_REPLACE_ALL },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1}
};

static const DIALOG_CTL_CREATE
search_query_ctl_create[] =
{
    { &dialog_main_group },
    { &search_query_next,        &search_query_next_data        },
    { &search_query_replace,     &search_query_replace_data     },
    { &search_query_replace_all, &search_query_replace_all_data },
    { &search_query_cancel, &stdbutton_cancel_data }
};

/******************************************************************************
*
* search intro dialog
*
******************************************************************************/

enum SEARCH_INTRO_CONTROL_IDS
{
    SEARCH_INTRO_ID_FROM_TOP = 155,
    SEARCH_INTRO_ID_FROM_CARET,

    SEARCH_INTRO_ID_FIND = 164,
    SEARCH_INTRO_ID_FIND_ORNAMENT,
    SEARCH_INTRO_ID_IGNORE_CAPITALS,
    SEARCH_INTRO_ID_WHOLE_WORDS,
    SEARCH_INTRO_ID_REPLACE_ENABLE,
    SEARCH_INTRO_ID_REPLACE,
    SEARCH_INTRO_ID_COPY_CAPITALS
};

#define SEARCH_INTRO_FIND_H (7 * PIXITS_PER_INCH / 2)

static const DIALOG_CONTROL
search_intro_find_ornament =
{
    SEARCH_INTRO_ID_FIND_ORNAMENT, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, SEARCH_INTRO_ID_FIND, DIALOG_CONTROL_SELF, SEARCH_INTRO_ID_FIND },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
search_intro_find_ornament_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_SEARCH_FIND), { 0 /*left_text*/ } };

static const DIALOG_CONTROL
search_intro_find =
{
    SEARCH_INTRO_ID_FIND, DIALOG_MAIN_GROUP,
    { SEARCH_INTRO_ID_FIND_ORNAMENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDSPACING_H, 0, SEARCH_INTRO_FIND_H, DIALOG_STDEDIT_V },
    { DRT(RTLT, EDIT), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_EDIT
search_intro_find_data = { { { FRAMED_BOX_EDIT } } };

static const DIALOG_CONTROL
search_intro_ignore_capitals =
{
    SEARCH_INTRO_ID_IGNORE_CAPITALS, DIALOG_MAIN_GROUP,
    { SEARCH_INTRO_ID_FIND_ORNAMENT, SEARCH_INTRO_ID_FIND_ORNAMENT },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
search_intro_ignore_capitals_data = {  { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_SEARCH_IGNORE_CAPITALS),  1 /* state */ };

static const DIALOG_CONTROL
search_intro_whole_words =
{
    SEARCH_INTRO_ID_WHOLE_WORDS, DIALOG_MAIN_GROUP,
    { SEARCH_INTRO_ID_IGNORE_CAPITALS, SEARCH_INTRO_ID_IGNORE_CAPITALS },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
search_intro_whole_words_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_SEARCH_WHOLE_WORDS) };

static const DIALOG_CONTROL
search_intro_replace_enable =
{
    SEARCH_INTRO_ID_REPLACE_ENABLE, DIALOG_MAIN_GROUP,
    { SEARCH_INTRO_ID_WHOLE_WORDS, SEARCH_INTRO_ID_REPLACE, DIALOG_CONTROL_SELF, SEARCH_INTRO_ID_REPLACE },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOXF
search_intro_replace_enable_data = { { { 0 /*left_text*/, 1 /*move_focus*/ } , UI_TEXT_INIT_RESID(MSG_DIALOG_SEARCH_REPLACE) }, SEARCH_INTRO_ID_REPLACE };

static /*poked*/ DIALOG_CONTROL
search_intro_replace =
{
    SEARCH_INTRO_ID_REPLACE, DIALOG_MAIN_GROUP,
    { SEARCH_INTRO_ID_REPLACE_ENABLE, SEARCH_INTRO_ID_WHOLE_WORDS /* or SEARCH_INTRO_ID_IGNORE_CAPITALS */, SEARCH_INTRO_ID_FIND },
    { DIALOG_STDSPACING_H, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V },
    { DRT(RBRT, EDIT), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_EDIT
search_intro_replace_data = { { { FRAMED_BOX_EDIT } } };

static const DIALOG_CONTROL
search_intro_copy_capitals =
{
    SEARCH_INTRO_ID_COPY_CAPITALS, DIALOG_MAIN_GROUP,
    { SEARCH_INTRO_ID_IGNORE_CAPITALS, SEARCH_INTRO_ID_REPLACE },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
search_intro_copy_capitals_data = { { 0 /*left_text*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_SEARCH_COPY_CAPITALS) };

static const DIALOG_CONTROL
search_intro_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, SEARCH_INTRO_ID_FROM_TOP, SEARCH_INTRO_ID_FROM_TOP, SEARCH_INTRO_ID_FROM_TOP },
#if WINDOWS
    { DIALOG_STDCANCEL_H, 0/*-DIALOG_DEFPUSHEXTRA_V*/, DIALOG_STDSPACING_H, 0 /*-DIALOG_DEFPUSHEXTRA_V*/ },
#else
    { DIALOG_CONTENTS_CALC, 0/*-DIALOG_DEFPUSHEXTRA_V*/, DIALOG_STDSPACING_H, 0 /*-DIALOG_DEFPUSHEXTRA_V*/ },
#endif
    { DRT(RTLB, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_ID
search_argmap[] =
{
#define ARG_SEARCH_FIND            0
    SEARCH_INTRO_ID_FIND,
#define ARG_SEARCH_IGNORE_CAPITALS 1
    SEARCH_INTRO_ID_IGNORE_CAPITALS,
#define ARG_SEARCH_WHOLE_WORDS     2
    SEARCH_INTRO_ID_WHOLE_WORDS,

#define ARG_SEARCH_REPLACE_ENABLE  3
    SEARCH_INTRO_ID_REPLACE_ENABLE,
#define ARG_SEARCH_REPLACE         4
    SEARCH_INTRO_ID_REPLACE,
#define ARG_SEARCH_COPY_CAPITALS   5
    SEARCH_INTRO_ID_COPY_CAPITALS,

#define ARG_SEARCH_FROM            6
#define SEARCH_FROM_CARET 0
#define SEARCH_FROM_TOP   1
    0 /* missing */

#define ARG_SEARCH_N_ARGS          7
};

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
search_intro_command = { T5_CMD_SEARCH, OBJECT_ID_SKEL, NULL, search_argmap, { 0, 0, 0, 1 /*lookup_arglist*/ } };

/*
from top
*/

static const DIALOG_CONTROL
search_intro_from_top =
{
    SEARCH_INTRO_ID_FROM_TOP, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, SEARCH_INTRO_ID_FROM_CARET, SEARCH_INTRO_ID_FROM_CARET, SEARCH_INTRO_ID_FROM_CARET },
    { DIALOG_CONTENTS_CALC, 0, DIALOG_STDSPACING_H, 0 },
    { DRT(RTLB, PUSHBUTTON), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_PUSHBUTTON
search_intro_from_top_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_SEARCH_FROM_TOP), &search_intro_command };

/*
from caret
*/

static const DIALOG_CONTROL
search_intro_from_caret =
{
    SEARCH_INTRO_ID_FROM_CARET, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_MAIN_GROUP, DIALOG_MAIN_GROUP },
    { DIALOG_CONTENTS_CALC, 0, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_PUSHBUTTON
search_intro_from_caret_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_SEARCH_FROM_CARET), &search_intro_command };

static const DIALOG_CTL_CREATE
search_intro_ctl_create[] =
{
    { &dialog_main_group },
    { &search_intro_find,            &search_intro_find_data },
    { &search_intro_find_ornament,   &search_intro_find_ornament_data },
    { &search_intro_ignore_capitals, &search_intro_ignore_capitals_data },
    { &search_intro_whole_words,     &search_intro_whole_words_data },
    { &search_intro_replace_enable,  &search_intro_replace_enable_data },
    { &search_intro_replace,         &search_intro_replace_data },
    { &search_intro_copy_capitals,   &search_intro_copy_capitals_data },
    { &search_intro_cancel, &stdbutton_cancel_data },
    { &search_intro_from_top, &search_intro_from_top_data },
    { &search_intro_from_caret, &search_intro_from_caret_data }
};

static void
enable_replace_section(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     BOOL enabled)
{
    ui_dlg_ctl_enable(h_dialog, SEARCH_INTRO_ID_REPLACE, enabled);
    ui_dlg_ctl_enable(h_dialog, SEARCH_INTRO_ID_COPY_CAPITALS, enabled);
}

_Check_return_
static STATUS
dialog_search_intro_preprocess_command(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_PREPROCESS_COMMAND p_dialog_msg_preprocess_command)
{
    /* SKS after 1.05 25oct93 - setup here so we can reset default to that last used */
    /*           1.06 08nov93 - setup BEFORE the command duplicate so subsequent searches retain default */
    const P_ARGLIST_ARG p_arg = p_arglist_arg(&p_dialog_msg_preprocess_command->arglist_handle, ARG_SEARCH_FROM);
    p_arg->type    = ARG_TYPE_S32;
    p_arg->val.s32 = (p_dialog_msg_preprocess_command->p_dialog_control->dialog_control_id == SEARCH_INTRO_ID_FROM_TOP) ? SEARCH_FROM_TOP : SEARCH_FROM_CARET;

    /* take a copy for use by next match */
    arglist_dispose(&p_docu->h_arglist_search);

    return(arglist_duplicate(&p_docu->h_arglist_search, &p_dialog_msg_preprocess_command->arglist_handle));
}

_Check_return_
static STATUS
dialog_search_intro_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    if(p_dialog_msg_ctl_create_state->dialog_control_id == SEARCH_INTRO_ID_REPLACE_ENABLE)
    {
        enable_replace_section(p_dialog_msg_ctl_create_state->h_dialog, (p_dialog_msg_ctl_create_state->state_set.state.checkbox == DIALOG_BUTTONSTATE_ON));

        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_DONT_MSG;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_search_intro_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    if(p_dialog_msg_ctl_state_change->dialog_control_id == SEARCH_INTRO_ID_REPLACE_ENABLE)
    {
        enable_replace_section(p_dialog_msg_ctl_state_change->h_dialog, (p_dialog_msg_ctl_state_change->new_state.checkbox == DIALOG_BUTTONSTATE_ON));
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_search_intro)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PREPROCESS_COMMAND:
        return(dialog_search_intro_preprocess_command(p_docu, (P_DIALOG_MSG_PREPROCESS_COMMAND) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_search_intro_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_search_intro_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    default:
        return(STATUS_OK);
    }
}

static void
search_setup_from_handle(
    _InRef_     PC_ARGLIST_HANDLE p_arglist_handle)
{
    if(0 != *p_arglist_handle)
    {
        P_ARGLIST_ARG p_arg;

        if(arg_present(p_arglist_handle, ARG_SEARCH_FIND, &p_arg))
        {
            search_intro_find_data.state.type = UI_TEXT_TYPE_USTR_TEMP;
            search_intro_find_data.state.text.ustr = p_arg->val.ustr;
        }

        if(arg_present(p_arglist_handle, ARG_SEARCH_IGNORE_CAPITALS, &p_arg))
            search_intro_ignore_capitals_data.init_state = p_arg->val.u8n;

        if(arg_present(p_arglist_handle, ARG_SEARCH_WHOLE_WORDS, &p_arg))
            search_intro_whole_words_data.init_state = p_arg->val.u8n;

        if(arg_present(p_arglist_handle, ARG_SEARCH_REPLACE_ENABLE, &p_arg))
            search_intro_replace_enable_data.checkbox.init_state = p_arg->val.u8n;

        if(arg_present(p_arglist_handle, ARG_SEARCH_REPLACE, &p_arg))
        {
            search_intro_replace_data.state.type = UI_TEXT_TYPE_USTR_TEMP;
            search_intro_replace_data.state.text.ustr = p_arg->val.ustr;
        }

        if(arg_present(p_arglist_handle, ARG_SEARCH_COPY_CAPITALS, &p_arg))
            search_intro_copy_capitals_data.init_state = p_arg->val.u8n;

        if(arg_present(p_arglist_handle, ARG_SEARCH_FROM, &p_arg))
            search_intro_from_top_data.push_xx.def_pushbutton = (p_arg->val.s32 == SEARCH_FROM_TOP);
    }
}

/******************************************************************************
*
* actually do the search
*
******************************************************************************/

T5_CMD_PROTO(extern, t5_cmd_search_do)
{
    ARRAY_HANDLE h_arglist = p_t5_cmd->arglist_handle;
    BOOL bump_position = FALSE;
    BOOL found = FALSE;
    S32 n_replaced = 0;
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(0 == n_arglist_args(&h_arglist))
    {   /* next match (no arguments given) */
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));
        h_arglist = p_docu->h_arglist_search;
        bump_position = TRUE;
    }

    if(0 != n_arglist_args(&h_arglist))
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&h_arglist, ARG_SEARCH_N_ARGS);
        PC_USTR ustr_find_string = p_args[ARG_SEARCH_FIND].val.ustr;
        enum SCAN_INIT_WHAT scan_what;
        BOOL stop_search = 0, replace_all = 0;
        BOOL ignore_capitals = 1, whole_words = 0;
        PC_USTR ustr_replace_string = NULL;
        BOOL replace = 0, copy_capitals = 0;

        if(!ustr_find_string || (CH_NULL == PtrGetByte(ustr_find_string)))
            return(STATUS_OK); /* nothing to search for */

        if(arg_is_present(p_args, ARG_SEARCH_IGNORE_CAPITALS))
            ignore_capitals = p_args[ARG_SEARCH_IGNORE_CAPITALS].val.fBool;

        if(arg_is_present(p_args, ARG_SEARCH_WHOLE_WORDS))
            whole_words = p_args[ARG_SEARCH_WHOLE_WORDS].val.fBool;

        if(arg_is_present(p_args, ARG_SEARCH_REPLACE_ENABLE))
            replace = p_args[ARG_SEARCH_REPLACE_ENABLE].val.fBool;

        if(arg_is_present(p_args, ARG_SEARCH_REPLACE))
            ustr_replace_string = p_args[ARG_SEARCH_REPLACE].val.ustr;

        if(arg_is_present(p_args, ARG_SEARCH_COPY_CAPITALS))
            copy_capitals = p_args[ARG_SEARCH_COPY_CAPITALS].val.fBool;

        if(!replace)
            ustr_replace_string = NULL;

        if(p_docu->mark_info_cells.h_markers
           &&
          !docu_area_is_frag(p_docu_area_from_markers_first(p_docu)))
            scan_what = SCAN_MARKERS;
        else
        {
            scan_what = SCAN_FROM_CUR;

            /* SKS after 1.06 08nov93 only take notice of this if it's NOT next match */
            if((0 != n_arglist_args(&p_t5_cmd->arglist_handle)) && arg_is_present(p_args, ARG_SEARCH_FROM))
            {
                if(p_args[ARG_SEARCH_FROM].val.s32 == SEARCH_FROM_TOP)
                {
                    scan_what = SCAN_WHOLE;
                    bump_position = 0;
                }
            }
        }

        {
        SCAN_BLOCK scan_block;

        if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_ACROSS, scan_what, NULL, OBJECT_ID_NONE)))
        {
            OBJECT_STRING_SEARCH object_string_search;

            while(status_done(cells_scan_next(p_docu, &object_string_search.object_data, &scan_block)))
            {
                for(;;)
                {
                    object_string_search.ustr_search_for = ustr_find_string;
                    object_position_init(&object_string_search.object_position_found_start);
                    object_string_search.ignore_capitals = ignore_capitals;
                    object_string_search.whole_words = whole_words;

                    if(bump_position)
                    {
                        OBJECT_POSITION_SET object_position_set;
                        object_position_set.object_data = object_string_search.object_data;
                        object_position_set.action = OBJECT_POSITION_SET_FORWARD;
                        status_consume(cell_call_id(object_string_search.object_data.object_id, p_docu, T5_MSG_OBJECT_POSITION_SET, &object_position_set, OK_CELLS_EDIT));
                        object_string_search.object_data.object_position_start = object_position_set.object_data.object_position_start;
                        bump_position = 0;
                    }

                    trace_0(TRACE_APP_SKEL, TEXT("t5_cmd_search_do calling OBJECT_STRING_SEARCH"));

                    if(status_ok(status = cell_call_id(object_string_search.object_data.object_id,
                                                       p_docu,
                                                       T5_MSG_OBJECT_STRING_SEARCH,
                                                       &object_string_search,
                                                       OK_CELLS_EDIT)))
                    {
                        if(OBJECT_ID_NONE != object_string_search.object_position_found_start.object_id)
                        {
                            found = 1;

                            if(!replace_all)
                            {
                                DOCU_AREA docu_area;

                                p_docu->cur.slr = object_string_search.object_data.data_ref.arg.slr;
                                p_docu->cur.object_position = object_string_search.object_position_found_start;

                                /* show found string */
                                docu_area_init(&docu_area);
                                docu_area.tl = p_docu->cur;
                                docu_area.br = p_docu->cur;
                                docu_area.br.slr.col += 1;
                                docu_area.br.slr.row += 1;
                                docu_area.br.object_position = object_string_search.object_position_found_end;
                                object_skel(p_docu, T5_MSG_SELECTION_MAKE, &docu_area);

                                caret_show_claim(p_docu, p_docu->focus_owner, TRUE);
                            }

                            if(!replace)
                            {
                                stop_search = 1;
                                break;
                            }
                            else
                            {
                                S32 do_replace = 0;

                                if(!replace_all)
                                {
                                    S32 search_query;

                                    {
                                    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
                                    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, search_query_ctl_create, elemof32(search_query_ctl_create), MSG_DIALOG_SEARCH_QUERY_HELP_TOPIC);
                                    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
                                    dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_SEARCH_QUERY_CAPTION;
                                    dialog_cmd_process_dbox.bits.note_position = 1;
                                    /*dialog_cmd_process_dbox.p_proc_client = NULL;*/
                                    search_query = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
                                    } /*block*/

                                    switch(search_query)
                                    {
                                    case SEARCH_QUERY_COMPLETION_NEXT:
                                        object_string_search.object_data.object_position_start = object_string_search.object_position_found_end;
                                        continue;

                                    case SEARCH_QUERY_COMPLETION_REPLACE:
                                        do_replace = 1;
                                        break;

                                    case SEARCH_QUERY_COMPLETION_REPLACE_ALL:
                                        do_replace = replace_all = 1;
                                        break;

                                    default: default_unhandled();
#if CHECKING
                                    case DIALOG_COMPLETION_CANCEL:
#endif
                                        stop_search = 1;
                                        break;
                                    }
                                }
                                else
                                    do_replace = 1;

                                if(do_replace)
                                {
                                    OBJECT_STRING_REPLACE object_string_replace;
                                    QUICK_UBLOCK quick_ublock;
                                    quick_ublock_setup_fill_from_const_ubuf(&quick_ublock, ustr_replace_string, ustrlen32(ustr_replace_string));

                                    status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

                                    docu_modify(p_docu);

                                    object_string_replace.object_data = object_string_search.object_data;
                                    object_string_replace.object_data.object_position_start = object_string_search.object_position_found_start;
                                    object_string_replace.object_data.object_position_end = object_string_search.object_position_found_end;
                                    object_string_replace.p_quick_ublock = &quick_ublock;
                                    object_string_replace.copy_capitals = copy_capitals;
                                    status_break(status = cell_call_id(object_string_replace.object_data.object_id,
                                                                       p_docu,
                                                                       T5_MSG_OBJECT_STRING_REPLACE,
                                                                       &object_string_replace,
                                                                       OK_CELLS_EDIT));

                                    object_string_search.object_data.u.p_object = object_string_replace.object_data.u.p_object;
                                    object_string_search.object_data.object_position_start = object_string_replace.object_position_after;
                                    n_replaced += 1;

                                    status_line_setf(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, MSG_STATUS_N_REPLACED, n_replaced);

                                    continue;
                                }
                            }
                        }
                    }

                    break;
                    /*NOTREACHED*/
                }

                if(status_fail(status))
                {
                    /* SKS 25apr95 just ignore non-editable results */
                    if(STATUS_FAIL == status)
                        status = STATUS_OK;
                    else
                        break;
                }

                if(stop_search)
                    break;
            }
        }
        } /*block*/

    }

    if(!found)
    {
        static const UI_TEXT ui_text = UI_TEXT_INIT_RESID(MSG_STATUS_NOT_FOUND);
        status_line_set(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, &ui_text);
        host_bleep();
    }

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_search_button_poss_db_queries)
{
    if(object_present(OBJECT_ID_REC))
    {
        STATUS status = object_call_id(OBJECT_ID_REC, p_docu, T5_CMD_VIEW_RECORDZ, P_DATA_NONE);

        if(STATUS_OK != status) /* done, or error */
            return(status);
    }

    return(t5_cmd_search_intro(p_docu, t5_message, p_t5_cmd));
}

T5_CMD_PROTO(extern, t5_cmd_search_button_poss_db_query)
{
    if(object_present(OBJECT_ID_REC))
    {
        STATUS status = object_call_id(OBJECT_ID_REC, p_docu, T5_CMD_SEARCH_RECORDZ, P_DATA_NONE);

        if(STATUS_OK != status) /* done, or error */
            return(status);
    }

    return(t5_cmd_search_intro(p_docu, t5_message, p_t5_cmd));
}

T5_CMD_PROTO(extern, t5_cmd_search_intro)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
    quick_ublock_with_buffer_setup(quick_ublock);

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    /* encode initial state of control(s) */
    search_intro_from_top_data.push_xx.def_pushbutton = 0;

    search_intro_find_data.state.type    = UI_TEXT_TYPE_NONE;
    search_intro_replace_data.state.type = UI_TEXT_TYPE_NONE;

    /* default data is taken from last operation */
    search_setup_from_handle(&p_docu->h_arglist_search);

    /* override existing data with data supplied from command */
    /*search_setup_from_handle(&p_t5_cmd->arglist_handle);*/

    search_intro_from_caret_data.push_xx.def_pushbutton = !search_intro_from_top_data.push_xx.def_pushbutton;

    if(p_docu->mark_info_cells.h_markers)
    {
        P_DOCU_AREA p_docu_area = p_docu_area_from_markers_first(p_docu);

        if(docu_area_is_frag(p_docu_area))
        {
            OBJECT_READ_TEXT object_read_text;
            status_consume(object_data_from_docu_area_tl(p_docu, &object_read_text.object_data, p_docu_area));
            object_read_text.p_quick_ublock = &quick_ublock;
            object_read_text.type = OBJECT_READ_TEXT_SEARCH;
            if(status_ok(object_call_id(object_read_text.object_data.object_id, p_docu, T5_MSG_OBJECT_READ_TEXT, &object_read_text)))
            if(status_ok(quick_ublock_nullch_add(&quick_ublock)))
            {
                search_intro_find_data.state.type = UI_TEXT_TYPE_USTR_PERM; /* won't move during dialog */
                search_intro_find_data.state.text.ustr = quick_ublock_ustr(&quick_ublock);
            }
        }
    }

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, search_intro_ctl_create, elemof32(search_intro_ctl_create), MSG_DIALOG_SEARCH_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_SEARCH_CAPTION;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_search_intro;
    status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

/* end of sk_find.c */
