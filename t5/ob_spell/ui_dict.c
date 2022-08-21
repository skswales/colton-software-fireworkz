/* ui_dict.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Dictionary managment UI for Wordz */

/* RCM Nov 1992 */

#include "common/gflags.h"

#include "ob_spell/ob_spell.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

/*
internal structure
*/

typedef struct BROWSE_ENTRY
{
    U8Z prefix[2];
    U8Z string[BUF_MAX_WORD];
}
BROWSE_ENTRY, * P_BROWSE_ENTRY;

#if WINDOWS
#define N_DICTIONARY_LIST_ITEMS 7       /* number of words shown by browse word list in dictionary DBOX */
#define BROWSE_CENTRE 3
#else
#define N_DICTIONARY_LIST_ITEMS 5       /* number of words shown by browse word list in dictionary DBOX */
#define BROWSE_CENTRE 2
#endif
#define BROWSE_PAGETHROW (N_DICTIONARY_LIST_ITEMS-1)

typedef struct DICTIONARY_CALLBACK
{
    UI_SOURCE ui_source;

    BOOL search_all;                     /* 0=search user dictionaries only, 1=search all dictionaries */
    BOOL iswild;
    U8Z wild_string[BUF_MAX_WORD];

    BROWSE_ENTRY wordlist[N_DICTIONARY_LIST_ITEMS];
}
DICTIONARY_CALLBACK, * P_DICTIONARY_CALLBACK;

/*
internal routines
*/

static void
browse_centre(
    _InVal_     H_DIALOG h_dialog,
    _InRef_     P_DICTIONARY_CALLBACK p_dictionary_callback);

_Check_return_
static STATUS
browse_wordlist(
    _DocuRef_   P_DOCU p_docu,
    P_DICTIONARY_CALLBACK p_dictionary_callback,
    _In_z_      PC_SBSTR source_word);

static void
browse_scrolldown(
    P_DICTIONARY_CALLBACK p_dictionary_callback,
    _InVal_     S32 lines);

static void
browse_scrollup(
    P_DICTIONARY_CALLBACK p_dictionary_callback,
    _InVal_     S32 lines);

_Check_return_
static STATUS
compile_wild_string(
    P_U8Z to,
    _In_z_      PC_U8Z from);

static void
set_wordlist_entry(
    P_BROWSE_ENTRY p_browse_entry,
    P_U8 word,
    _InVal_     S32 dictionary_id);

enum DICT_CONTROL_IDS
{
    CONTROL_ID_WORD     = 64,
    CONTROL_ID_WORD_ORNAMENT,
    CONTROL_ID_LIST,
    CONTROL_ID_ADD,
    CONTROL_ID_DELETE,
    CONTROL_ID_CANCEL,
    CONTROL_ID_ALL,
    CONTROL_ID_UP,
    CONTROL_ID_DOWN
};

#define CONTROL_BUTT_H (DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARS_H("Delete"))
#define CONTROL_EDIT_H (16 * PIXITS_PER_INCH) / 8
#define CONTROL_LIST_V (10 * PIXITS_PER_INCH) / 8

static const DIALOG_CONTROL
dict_word_ornament =
{
    CONTROL_ID_WORD_ORNAMENT, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, CONTROL_BUTT_H, DIALOG_STDEDIT_V },
    { DRT(LTLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
dict_word_ornament_data = { UI_TEXT_INIT_RESID(OB_SPELL_MSG_WORD), { 1/*left_text*/ } };

static const DIALOG_CONTROL
dict_word =
{
    CONTROL_ID_WORD, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_WORD_ORNAMENT, CONTROL_ID_WORD_ORNAMENT, DIALOG_CONTROL_SELF, CONTROL_ID_WORD_ORNAMENT },
    { DIALOG_STDSPACING_H, 0, CONTROL_EDIT_H, 0 },
    { DRT(RTLB, EDIT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_EDIT
dict_word_data = { { { FRAMED_BOX_EDIT } }, /*EDIT_XX*/ { UI_TEXT_TYPE_NONE } /* UI_TEXT state */ };

static const DIALOG_CONTROL
dict_list =
{
    CONTROL_ID_LIST, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_WORD, CONTROL_ID_WORD, CONTROL_ID_WORD },
    { 0, DIALOG_STDSPACING_V, 0, CONTROL_LIST_V },
    { DRT(LBRT, LIST_TEXT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_LIST_TEXT
dict_list_data = { { 0 /*force_v_scroll*/, 1 /*disable_double*/, 2 /*tab_position*/} };

static const DIALOG_CONTROL_ID
dict_add_data_argmap[] = { CONTROL_ID_WORD };

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
dict_add_command = { T5_CMD_SPELL_DICTIONARY_ADD_WORD, OBJECT_ID_SPELL, NULL, dict_add_data_argmap, { 0, 0, 1, 1 /*lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
dict_add_data = { { 0 }, UI_TEXT_INIT_RESID(OB_SPELL_MSG_ADD_WORD), &dict_add_command };

static const DIALOG_CONTROL
dict_add =
{
    CONTROL_ID_ADD, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_WORD_ORNAMENT, CONTROL_ID_WORD_ORNAMENT, CONTROL_ID_WORD_ORNAMENT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_ID
dict_delete_data_argmap[] = { CONTROL_ID_LIST };

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
dict_delete_command = { T5_CMD_SPELL_DICTIONARY_DELETE_WORD, OBJECT_ID_SPELL, NULL, dict_delete_data_argmap, { 0, 0, 1, 1 /* lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
dict_delete_data = { { 0 }, UI_TEXT_INIT_RESID(OB_SPELL_MSG_DELETE_WORD), &dict_delete_command };

static const DIALOG_CONTROL
dict_delete =
{
    CONTROL_ID_DELETE, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_ADD, CONTROL_ID_ADD, CONTROL_ID_ADD },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
dict_cancel =
{
    CONTROL_ID_CANCEL, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_DELETE, CONTROL_ID_DELETE, CONTROL_ID_DELETE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
dict_all =
{
    CONTROL_ID_ALL, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_CANCEL, CONTROL_ID_CANCEL, CONTROL_ID_CANCEL },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, CHECKBOX), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
dict_all_data = { { 0 },  UI_TEXT_INIT_RESID(OB_SPELL_MSG_ALL_DICTS), 1 };

#if RISCOS
#define DICT_UD_SPACING_H 0
#define DICT_UD_BUTTONS_H 60 * PIXITS_PER_RISCOS
#define DICT_UD_BUTTONS_V 60 * PIXITS_PER_RISCOS
#elif WINDOWS
#define DICT_UD_SPACING_H  1 * PIXITS_PER_WDU_H
#define DICT_UD_BUTTONS_H 13 * PIXITS_PER_WDU_H
#define DICT_UD_BUTTONS_V 13 * PIXITS_PER_WDU_V
#endif

static const DIALOG_CONTROL
dict_up =
{
    CONTROL_ID_UP, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_LIST, CONTROL_ID_LIST },
    { DICT_UD_SPACING_H, 0, DICT_UD_BUTTONS_H, DICT_UD_BUTTONS_V },
    { DRT(RTLT, PUSHPICTURE) }
};

static const DIALOG_CONTROL_DATA_PUSHPICTURE
dict_up_data = { { 0, 0, 0, 0, 1 /*auto repeat*/, 1 /*not_dlg_framed*/ }, { OBJECT_ID_SKEL, SKEL_ID_BM_INC }};

static const DIALOG_CONTROL
dict_down =
{
    CONTROL_ID_DOWN, DIALOG_CONTROL_WINDOW,
    { CONTROL_ID_UP, CONTROL_ID_LIST, CONTROL_ID_UP, CONTROL_ID_LIST },
    { 0, -DICT_UD_BUTTONS_V, 0, 0 },
    { DRT(LBRB, PUSHPICTURE) }
};

static const DIALOG_CONTROL_DATA_PUSHPICTURE
dict_down_data = { { 0, 0, 0, 0, 1 /*auto repeat*/, 1 /*not_dlg_framed*/ }, { OBJECT_ID_SKEL, SKEL_ID_BM_DEC } };

static const DIALOG_CTL_CREATE
dictionary_dialog_create[] =
{
    { &dict_word_ornament, &dict_word_ornament_data },
    { &dict_word, &dict_word_data },
    { &dict_list, &dict_list_data },
    { &dict_add, &dict_add_data },
    { &dict_delete, &dict_delete_data },
    { &dict_cancel, &stdbutton_cancel_data },
    { &dict_all, &dict_all_data },
    { &dict_up, &dict_up_data },
    { &dict_down, &dict_down_data }
};

_Check_return_
static STATUS
dialog_dictionary_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    const P_DICTIONARY_CALLBACK p_dictionary_callback = (P_DICTIONARY_CALLBACK) p_dialog_msg_ctl_fill_source->client_handle;

    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case CONTROL_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &p_dictionary_callback->ui_source;;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_dictionary_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_DICTIONARY_CALLBACK p_dictionary_callback = (P_DICTIONARY_CALLBACK) p_dialog_msg_process_start->client_handle;

    /* set control states here */
    browse_centre(p_dialog_msg_process_start->h_dialog, p_dictionary_callback);

    ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, CONTROL_ID_ADD, global_preferences.spell_write_user);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_dictionary_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case CONTROL_ID_ALL:
        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALWAYS_MSG;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_dictionary_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    const P_DICTIONARY_CALLBACK p_dictionary_callback = (P_DICTIONARY_CALLBACK) p_dialog_msg_ctl_pushbutton->client_handle;
    const DIALOG_CONTROL_ID dialog_control_id = p_dialog_msg_ctl_pushbutton->dialog_control_id;

    switch(dialog_control_id)
    {
    case CONTROL_ID_UP:
    case CONTROL_ID_DOWN:
        {
        BOOL up = (CONTROL_ID_UP == dialog_control_id);

        if(p_dialog_msg_ctl_pushbutton->right_button)
            up = !up;

        (up ? browse_scrollup : browse_scrolldown) (p_dictionary_callback, 1);

        /*ui_dlg_set_list_idx(p_dialog_msg_ctl_pushbutton->h_dialog, CONTROL_ID_LIST, -1);*/

        browse_centre(p_dialog_msg_ctl_pushbutton->h_dialog, p_dictionary_callback);
        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_dictionary_msg_persisting(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_PERSISTING p_dialog_msg_persisting)
{
    const P_DICTIONARY_CALLBACK p_dictionary_callback = (P_DICTIONARY_CALLBACK) p_dialog_msg_persisting->client_handle;
    UI_TEXT ui_text;

    ui_dlg_get_edit(p_dialog_msg_persisting->h_dialog, CONTROL_ID_WORD, &ui_text);

    status_assert(browse_wordlist(p_docu, p_dictionary_callback,
                        !ui_text_is_blank(&ui_text)
                        ? _sbstr_from_ustr(ui_text_ustr(&ui_text))
                        : p_dictionary_callback->wordlist[BROWSE_CENTRE].string[0]
                        ? p_dictionary_callback->wordlist[BROWSE_CENTRE].string
                        : "a"));

    browse_centre(p_dialog_msg_persisting->h_dialog, p_dictionary_callback);

    ui_text_dispose(&ui_text);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_dictionary_ctl_state_change(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_DICTIONARY_CALLBACK p_dictionary_callback = (P_DICTIONARY_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case CONTROL_ID_WORD:
        {
        const PC_UI_TEXT p_ui_text = &p_dialog_msg_ctl_state_change->new_state.edit.ui_text;

        status_assert(browse_wordlist(p_docu, p_dictionary_callback,
                            !ui_text_is_blank(p_ui_text)
                            ? _sbstr_from_ustr(ui_text_ustr(p_ui_text))
                            : "a"));

        /*ui_dlg_set_list_idx(p_dialog_msg_ctl_state_change->h_dialog, CONTROL_ID_LIST, -1);*/

        browse_centre(p_dialog_msg_ctl_state_change->h_dialog, p_dictionary_callback);
        break;
        }

    case CONTROL_ID_LIST:
        if(p_dialog_msg_ctl_state_change->new_state.list_text.itemno >= 0)
        {
            if(p_dialog_msg_ctl_state_change->new_state.list_text.itemno < BROWSE_CENTRE)
                browse_scrollup(p_dictionary_callback, BROWSE_CENTRE - p_dialog_msg_ctl_state_change->new_state.list_text.itemno);
            else if((p_dialog_msg_ctl_state_change->new_state.list_text.itemno > BROWSE_CENTRE)
                 && (p_dialog_msg_ctl_state_change->new_state.list_text.itemno < N_DICTIONARY_LIST_ITEMS))
                browse_scrolldown(p_dictionary_callback, p_dialog_msg_ctl_state_change->new_state.list_text.itemno - BROWSE_CENTRE);
        }

        /*ui_dlg_set_list_idx(p_dialog_msg_ctl_state_change->h_dialog, CONTROL_ID_LIST, -1);*/

        browse_centre(p_dialog_msg_ctl_state_change->h_dialog, p_dictionary_callback);
        break;

    case CONTROL_ID_ALL:
        p_dictionary_callback->search_all = dict_all_data.init_state = p_dialog_msg_ctl_state_change->new_state.checkbox;

        status_assert(browse_wordlist(p_docu, p_dictionary_callback,
                            (CH_NULL != p_dictionary_callback->wordlist[BROWSE_CENTRE].string[0])
                            ? p_dictionary_callback->wordlist[BROWSE_CENTRE].string
                            : "a"));

        /*ui_dlg_set_list_idx(p_dialog_msg_ctl_state_change->h_dialog, CONTROL_ID_LIST, -1);*/

        browse_centre(p_dialog_msg_ctl_state_change->h_dialog, p_dictionary_callback);
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_dictionary)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_dictionary_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_dictionary_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_dictionary_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_dictionary_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    case DIALOG_MSG_CODE_PERSISTING:
        return(dialog_dictionary_msg_persisting(p_docu, (P_DIALOG_MSG_PERSISTING) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_dictionary_ctl_state_change(p_docu, (PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
t5_cmd_spell_dictionary(
    _DocuRef_   P_DOCU p_docu)
{
    DICTIONARY_CALLBACK dictionary_callback;
    STATUS status = STATUS_OK;

    zero_struct(dictionary_callback);

    dictionary_callback.search_all = dict_all_data.init_state;

    {
    ARRAY_INDEX index;

    if(NULL == al_array_alloc_UI_TEXT(&dictionary_callback.ui_source.source.array_handle, N_DICTIONARY_LIST_ITEMS, &array_init_block_ui_text, &status))
        return(status);

    dictionary_callback.ui_source.type = UI_SOURCE_TYPE_ARRAY;

    for(index = 0; index < N_DICTIONARY_LIST_ITEMS; ++index)
    {
        P_UI_TEXT p_ui_text = array_ptr(&dictionary_callback.ui_source.source.array_handle, UI_TEXT, index);

#if USTR_IS_SBSTR
        p_ui_text->type = UI_TEXT_TYPE_USTR_PERM;
        p_ui_text->text.ustr = dictionary_callback.wordlist[index].prefix;
#else
        p_ui_text->type = UI_TEXT_TYPE_USTR_TEMP;
        p_ui_text->text.ustr = _ustr_from_tstr(_tstr_from_sbstr(dictionary_callback.wordlist[index].prefix));
#endif
    }
    } /*block*/

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, dictionary_dialog_create, elemof32(dictionary_dialog_create), OB_SPELL_MSG_DICTIONARY_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = OB_SPELL_MSG_DICTIONARY_CAPTION;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_dictionary;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &dictionary_callback;
    status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    ui_source_dispose(&dictionary_callback.ui_source);

    return(status);
}

/******************************************************************************
*
* fill DICT_WORDLIST, highlighting the centre word entry
* does NOT update DICT_WORDTEMPLATE
*
******************************************************************************/

_Check_return_
static STATUS
browse_wordlist(
    _DocuRef_   P_DOCU p_docu,
    P_DICTIONARY_CALLBACK p_dictionary_callback,
    _In_z_      PC_SBSTR source_word)
{
    U8Z template_buffer[BUF_MAX_WORD];
    U8Z wild_string[BUF_MAX_WORD];
    BOOL iswild;

    P_U8 src, dst;
    U8 ch;
    U8Z szCentre[BUF_MAX_WORD];
    U8Z szWord[BUF_MAX_WORD];
    S32 y;
    STATUS err;
    S32 ctrlflag = 0;

    if(status_fail(err = compile_wild_string(wild_string, source_word)))
        return(err);

    iswild = (BOOL) err;

    if(iswild)
        template_buffer[0] = CH_NULL;
    else
    {
        wild_string[0] = CH_NULL;
        xstrkpy(template_buffer, sizeof32(template_buffer), source_word);
    }

#if TRUE
    /* the user types in mixed case, we want lower case in the list box */
    src = template_buffer; /*>>>should we do src = iswild ? NULLSTR : template_buffer; */
    dst = szCentre;
    do  {
        ch = *src++;
        if(ch)
            ch = (U8) t5_tolower(ch);   /*>>>should this be (ob_)spell_tolower???*/
        *dst++ = ch;
    }
    while(ch);
#else
    (void) strcpy(szCentre, template_buffer);
#endif

    p_dictionary_callback->iswild = iswild;
    xstrkpy(p_dictionary_callback->wild_string, sizeof32(p_dictionary_callback->wild_string), wild_string);

    /* get a valid word for centre list box entry */ /* don't search skip list, search all dictionaries */
    if(iswild || ((err = ob_spell_checkword(p_docu, ustr_bptr(template_buffer), FALSE, TRUE, p_dictionary_callback->search_all)) == 0))
        if((err = ob_spell_nextword(szCentre, sizeof32(szCentre), template_buffer, wild_string, &ctrlflag, p_dictionary_callback->search_all)) == 0)
            err = ob_spell_prevword(szCentre, sizeof32(szCentre), template_buffer, wild_string, &ctrlflag, p_dictionary_callback->search_all);

    set_wordlist_entry(&p_dictionary_callback->wordlist[BROWSE_CENTRE], szCentre, (S32) err);

    /* find the preceeding words */
    xstrkpy(szWord, sizeof32(szWord), szCentre);
    for(y = BROWSE_CENTRE-1; y >= 0; --y)
    {
        if(ctrlflag || (err < 0) || !*szWord ||
            ((err = ob_spell_prevword(szWord, sizeof32(szWord), szWord, wild_string, &ctrlflag, p_dictionary_callback->search_all)) <= 0)
           )
            *szWord = CH_NULL;

        set_wordlist_entry(&p_dictionary_callback->wordlist[y], szWord, (S32) err);
    }

    /* and the ones after it */
    xstrkpy(szWord, sizeof32(szWord), szCentre);
    for(y = BROWSE_CENTRE + 1; y <= N_DICTIONARY_LIST_ITEMS-1; ++y)
    {
        if(ctrlflag || (err < 0) || !*szWord ||
            ((err = ob_spell_nextword(szWord, sizeof32(szWord), szWord, wild_string, &ctrlflag, p_dictionary_callback->search_all)) <= 0)
           )
            *szWord = CH_NULL;

        set_wordlist_entry(&p_dictionary_callback->wordlist[y], szWord, (S32) err);
    }

    return(err);
}

/******************************************************************************
*
* scroll DICT_WORDLIST, calling ob_spell_nextword as appropriate
* may update DICT_WORDTEMPLATE
*
******************************************************************************/

static void
browse_scrolldown(
    P_DICTIONARY_CALLBACK p_dictionary_callback,
    _InVal_     S32 lines)
{
  /*S32  iswild      = p_dictionary_callback->iswild;*/
    P_U8 wild_string = p_dictionary_callback->wild_string;
    U8Z szWord[BUF_MAX_WORD];
    S32 y;
    S32 doscroll;
    STATUS err      = 0;
    S32 ctrlflag = 0;

    xstrkpy(szWord, sizeof32(szWord), p_dictionary_callback->wordlist[N_DICTIONARY_LIST_ITEMS-1].string);

    doscroll = (0 != *p_dictionary_callback->wordlist[BROWSE_CENTRE+1].string);

    for(y = 1; (y <= lines) && (doscroll); ++y)
    {
        if(!*szWord ||
            ((err = ob_spell_nextword(szWord, sizeof32(szWord), szWord, wild_string, &ctrlflag, p_dictionary_callback->search_all)) <= 0)
           )
            *szWord = CH_NULL;

        doscroll = (0 != *p_dictionary_callback->wordlist[BROWSE_CENTRE+2].string);

        {
        UINT index;
        for(index = 0; index < N_DICTIONARY_LIST_ITEMS-1; ++index)
            p_dictionary_callback->wordlist[index] = p_dictionary_callback->wordlist[index + 1];
        } /*block*/

        set_wordlist_entry(&p_dictionary_callback->wordlist[N_DICTIONARY_LIST_ITEMS-1], szWord, (S32) err);
    }
}

/******************************************************************************
*
* scroll DICT_WORDLIST, calling ob_spell_prevword as appropriate
* may update DICT_WORDTEMPLATE
*
******************************************************************************/

static void
browse_scrollup(
    P_DICTIONARY_CALLBACK p_dictionary_callback,
    _InVal_     S32 lines)
{
  /*S32  iswild      = p_dictionary_callback->iswild;*/
    P_U8 wild_string = p_dictionary_callback->wild_string;
    U8Z szWord[BUF_MAX_WORD];
    S32 y;
    S32 doscroll;
    STATUS err      = 0;
    S32 ctrlflag = 0;

    xstrkpy(szWord, sizeof32(szWord), p_dictionary_callback->wordlist[0].string);

    doscroll = (0 != *p_dictionary_callback->wordlist[BROWSE_CENTRE-1].string);

    for(y = 1; (y <= lines) && (doscroll); ++y)
    {
        if(!*szWord ||
            ((err = ob_spell_prevword(szWord, sizeof32(szWord), szWord, wild_string, &ctrlflag, p_dictionary_callback->search_all)) <= 0)
           )
            *szWord = CH_NULL;

        doscroll = (0 != *p_dictionary_callback->wordlist[BROWSE_CENTRE-2].string);

        {
        UINT index = N_DICTIONARY_LIST_ITEMS;
        while(--index > 0)
            p_dictionary_callback->wordlist[index] = p_dictionary_callback->wordlist[index-1];
        } /*block*/

        set_wordlist_entry(&p_dictionary_callback->wordlist[0], szWord, (S32) err);
    }
}

static void
browse_centre(
    _InVal_     H_DIALOG h_dialog,
    _InRef_     P_DICTIONARY_CALLBACK p_dictionary_callback)
{
    ui_dlg_ctl_new_source(h_dialog, CONTROL_ID_LIST);

    /*ui_dlg_set_list_idx(h_dialog, CONTROL_ID_LIST, BROWSE_CENTRE);*/

    /* selected word has changed, so enable/disable delete button as appropriate */
    ui_dlg_ctl_enable(h_dialog, CONTROL_ID_DELETE, (CH_SPACE != *p_dictionary_callback->wordlist[BROWSE_CENTRE].prefix));
}

/******************************************************************************
*
* compile the wild string - returning whether it is wild
*
******************************************************************************/

_Check_return_
static STATUS
compile_wild_string(
    P_U8Z to,
    _In_z_      PC_U8Z from)
{
    PC_U8Z ptr = from;
    BOOL iswild = 0;    /* wild?, it bit my hand off */
    U8 ch;

    /* get word template from wild_string */
    if(ptr && *ptr)
        while((ch = *ptr++) != CH_NULL)
        {
            if(ch == '*')
            {
                *to++ = SPELL_WILD_MULTIPLE;
                iswild = TRUE;
            }
            else if(ch == CH_QUESTION_MARK)
            {
                *to++ = SPELL_WILD_SINGLE;
                iswild = TRUE;
            }
            else if(!((ptr == from) ? ob_spell_valid_1 : ob_spell_iswordc)(ch))
                return(create_error(SPELL_ERR_BADWORD));
            else
                *to++ = (U8)t5_tolower(ch);        /*>>>should this be (ob_)spell_tolower*/
        }

    *to = CH_NULL;
    return(iswild);
}

/******************************************************************************
*
* 0 no word
* 1 from lower case user dictionary
* 2 from upper case user dictionary
* 3 from lower case master dictionary
* 4 from upper case master dictionary
*
******************************************************************************/

static void
set_wordlist_entry(
    P_BROWSE_ENTRY p_browse_entry,
    P_U8 word,
    _InVal_     S32 dictionary_id)
{
    p_browse_entry->prefix[0] = CH_SPACE;
    if((dictionary_id == DICTIONARY_USER) || (dictionary_id == DICTIONARY_USER_CAPITALISED))
        p_browse_entry->prefix[0] = '*';
    p_browse_entry->prefix[1] = '\t';

    xstrkpy(p_browse_entry->string, sizeof32(p_browse_entry->string), word);
    ob_spell_setcase(p_browse_entry->string, dictionary_id);
}

/* end of ui_dict.c */
