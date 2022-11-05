/* sk_cont.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Window controls for Fireworkz */

/* RCM May 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_toolb/xp_toolb.h"

/*
callback routines
*/

PROC_EVENT_PROTO(extern, proc_event_sk_cont_direct);

/*
internal routines
*/

static void
skel_controls_query_and_encode_style_bitmap(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE_SELECTOR p_style_selector);

static void
skel_controls_selectors_init(void);

/*
an interlock is needed to prevent terminal mutual recursion of style encoding/setting
*/

static
STYLE_SELECTOR skel_controls_selector_all;

static
STYLE_SELECTOR skel_controls_selector_character;

static const T5_TOOLBAR_TOOL_DESC
skel_tools[] =
{
    { USTR_TEXT("VIEW"),
        OBJECT_ID_SKEL, T5_CMD_VIEW_CONTROL_INTRO,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_VIEW, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0/*im*/, 1/*sv*/ },
        UI_TEXT_INIT_RESID(MSG_STATUS_VIEW_CONTROL), T5_CMD_VIEW_SCALE_INTRO },

    { USTR_TEXT("NEW"),
        OBJECT_ID_SKEL, T5_CMD_NEW_DOCUMENT_INTRO,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_NEW, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_NEW_DOCUMENT) },

#if !RISCOS
    { USTR_TEXT("OPEN"),
        OBJECT_ID_FILE, T5_CMD_OPEN_DOCUMENT,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_OPEN, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_OPEN_DOCUMENT), T5_CMD_INSERT_FILE_WINDOWS },
#endif

#if RISCOS /* swap the alternate and normal commands */
    { USTR_TEXT("SAVE"),
        OBJECT_ID_SKEL, T5_CMD_SAVE_AS_INTRO,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_SAVE, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_SAVE), T5_CMD_SAVE },
#else
    { USTR_TEXT("SAVE"),
        OBJECT_ID_SKEL, T5_CMD_SAVE,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_SAVE, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_SAVE), T5_CMD_SAVE_AS_INTRO },
#endif

    { USTR_TEXT("PRINT"),
        OBJECT_ID_SKEL, T5_CMD_PRINT_INTRO,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_PRINT, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_PRINT), T5_CMD_PRINT_EXTRA_INTRO },

    { USTR_TEXT("CUT"),
        OBJECT_ID_SKEL, T5_CMD_SELECTION_CUT,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_CUT, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_CUT), T5_CMD_SELECTION_DELETE },

    { USTR_TEXT("COPY"),
        OBJECT_ID_SKEL, T5_CMD_SELECTION_COPY,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_COPY, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0, },
        UI_TEXT_INIT_RESID(MSG_STATUS_COPY) },

    { USTR_TEXT("PASTE"),
        OBJECT_ID_SKEL, T5_CMD_PASTE_AT_CURSOR,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_PASTE, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_PASTE) },

    { USTR_TEXT("SELECTION"),
        OBJECT_ID_SKEL, T5_CMD_TOGGLE_MARKS,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_MARKS, SKEL_ID_BM_TOOLBAR_MARKS_ON,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_TOGGLE_MARKS) },

    { USTR_TEXT("SEARCH"),
        OBJECT_ID_SKEL, T5_CMD_SEARCH_BUTTON,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_SEARCH, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_SEARCH), T5_CMD_SEARCH_BUTTON_ALTERNATE },

    { USTR_TEXT("SORT"),
        OBJECT_ID_SKEL, T5_CMD_SORT_INTRO,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_SORT, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_SORT) },

    { USTR_TEXT("CHECK"),
        OBJECT_ID_SPELL, T5_CMD_SPELL_CHECK,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_CHECK, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_CHECK), T5_CMD_SPELL_DICTIONARY },

#if RISCOS
    { USTR_TEXT("THESAURUS"),
        OBJECT_ID_SKEL, T5_CMD_THESAURUS,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_THESAURUS, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_THESAURUS) },
#endif

    { USTR_TEXT("TABLE"),
        OBJECT_ID_SKEL, T5_CMD_INSERT_TABLE_INTRO,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_TABLE, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_INSERT_TABLE) },

    { USTR_TEXT("INSERT_DATE"),
        OBJECT_ID_SKEL, T5_CMD_INSERT_FIELD_INTRO_DATE,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_INSERT_DATE, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_INSERT_FIELD_DATE), T5_CMD_INSERT_FIELD_INTRO_TIME },

    { USTR_TEXT("BOX"),
        OBJECT_ID_SKEL, T5_CMD_BOX_INTRO,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_BOX, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_BOX) },

    { USTR_TEXT("STYLE"),
        OBJECT_ID_SKEL, T5_CMD_STYLE_BUTTON,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_STYLE, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_APPLY_STYLE), T5_CMD_STYLE_FOR_CONFIG },

    { USTR_TEXT("EFFECTS"),
        OBJECT_ID_SKEL, T5_CMD_EFFECTS_BUTTON,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_EFFECTS, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_APPLY_EFFECTS) },

    { USTR_TEXT("BOLD"),
        OBJECT_ID_SKEL, T5_MSG_STYLE_CHANGE_BOLD,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_BOLD, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_APPLY_BOLD) },

    { USTR_TEXT("ITALIC"),
        OBJECT_ID_SKEL, T5_MSG_STYLE_CHANGE_ITALIC,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_ITALIC, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_APPLY_ITALIC) },

    { USTR_TEXT("UNDERLINE"),
        OBJECT_ID_SKEL, T5_MSG_STYLE_CHANGE_UNDERLINE,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_UNDERLINE, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_APPLY_UNDERLINE) },

    { USTR_TEXT("SUPERSCRIPT"),
        OBJECT_ID_SKEL, T5_MSG_STYLE_CHANGE_SUPERSCRIPT,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_SUPERSCRIPT_THIN, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0/*im*/, 0/*sv*/, 1 /*thin*/ },
        UI_TEXT_INIT_RESID(MSG_STATUS_APPLY_SUPERSCRIPT) },

    { USTR_TEXT("SUBSCRIPT"),
        OBJECT_ID_SKEL, T5_MSG_STYLE_CHANGE_SUBSCRIPT,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_SUBSCRIPT_THIN, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0/*im*/, 0/*sv*/, 1 /*thin*/ },
        UI_TEXT_INIT_RESID(MSG_STATUS_APPLY_SUBSCRIPT) },

    { USTR_TEXT("JUSTIFY_LEFT"),
        OBJECT_ID_SKEL, T5_MSG_STYLE_CHANGE_JUSTIFY_LEFT,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_J_LEFT, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_JUSTIFY_LEFT) },

    { USTR_TEXT("JUSTIFY_CENTRE"),
        OBJECT_ID_SKEL, T5_MSG_STYLE_CHANGE_JUSTIFY_CENTRE,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_J_CENTRE, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_JUSTIFY_CENTRE) },

    { USTR_TEXT("JUSTIFY_RIGHT"),
        OBJECT_ID_SKEL, T5_MSG_STYLE_CHANGE_JUSTIFY_RIGHT,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_J_RIGHT, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_JUSTIFY_RIGHT) },

    { USTR_TEXT("JUSTIFY_FULL"),
        OBJECT_ID_SKEL, T5_MSG_STYLE_CHANGE_JUSTIFY_FULL,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_J_FULL, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_JUSTIFY_FULL) },

    { USTR_TEXT("TAB_LEFT"),
        OBJECT_ID_SKEL, T5_MSG_SET_TAB_LEFT,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_TAB_LEFT, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0/*im*/, 0/*sv*/, 1 /*thin*/ },
        UI_TEXT_INIT_RESID(MSG_STATUS_TAB_LEFT) },

    { USTR_TEXT("TAB_CENTRE"),
        OBJECT_ID_SKEL, T5_MSG_SET_TAB_CENTRE,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_TAB_CENTRE, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0/*im*/,0/*sv*/,  1 /*thin*/ }, 
        UI_TEXT_INIT_RESID(MSG_STATUS_TAB_CENTRE) },

    { USTR_TEXT("TAB_RIGHT"),
        OBJECT_ID_SKEL, T5_MSG_SET_TAB_RIGHT,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_TAB_RIGHT, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0/*im*/, 0/*sv*/, 1 /*thin*/ },
        UI_TEXT_INIT_RESID(MSG_STATUS_TAB_RIGHT) },

    { USTR_TEXT("TAB_DECIMAL"),
        OBJECT_ID_SKEL, T5_MSG_SET_TAB_DECIMAL,
        OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_TAB_DECIMAL, 0,
        { T5_TOOLBAR_TOOL_TYPE_STATE, 0/*im*/, 0/*sv*/, 1 /*thin*/ },
        UI_TEXT_INIT_RESID(MSG_STATUS_TAB_DECIMAL) }
};

/* bit numbers for enable setting */
#define SKEL_ENABLE_CLIPBOARD_PASTE_FROM 0
#define SKEL_ENABLE_CLIPBOARD_COPY_TO 1
#define SKEL_ENABLE_FOR_FOCUS 2
#define SKEL_ENABLE_FOR_FOCUS_AND_SELECTION 4

#define MAX_TITLEBAR     (MAX_PATHSTRING + 32 + BUF_MAX_S32_FMT-1)
#define BUF_MAX_TITLEBAR (MAX_PATHSTRING + 32 + BUF_MAX_S32_FMT-1 + 1)

static void
view_set_title(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    TCHARZ wintitle[BUF_MAX_TITLEBAR];
    U32 len = 0;
    STATUS resource_id = p_docu->flags.read_only ? MSG_TITLEBAR_READ_ONLY : MSG_TITLEBAR_NORMAL;

    wintitle[0] = CH_NULL;
    ui_string_to_text(p_docu, wintitle, elemof32(wintitle), resource_lookup_tstr(resource_id));
    len = tstrlen32(wintitle);

    if(p_docu->modified)
    {
        resource_lookup_tstr_buffer(wintitle + len, elemof32(wintitle) - len, MSG_TITLEBAR_MODIFIED);
        len += tstrlen32(wintitle + len);
    }

    if(p_view->scalet != p_view->scaleb)
    {
        F64 scale = (p_view->scalet * 100.0) / p_view->scaleb;

        len += tstr_xsnprintf(wintitle + len, elemof32(wintitle) - len,
                              resource_lookup_tstr(MSG_AT_N_PERCENT),
                              scale);
    }

    if(p_docu->n_views != 1)
        len += tstr_xsnprintf(wintitle + len, elemof32(wintitle) - len,
                              TEXT(" ") S32_TFMT,
                              (S32) p_docu->n_views);

    host_settitle(p_docu, p_view, wintitle);
}

/*
main events
*/

_Check_return_
static STATUS
sk_cont_msg_docu__title(
    _DocuRef_   P_DOCU p_docu)
{
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;

    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
        view_set_title(p_docu, p_view);

    return(STATUS_OK);
}

MAEVE_EVENT_PROTO(static, maeve_event_sk_cont)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_VIEW_ZOOMFACTOR:
        view_set_title(p_docu, (P_VIEW) p_data);
        return(STATUS_OK);

    case T5_MSG_DOCU_NAME:
    case T5_MSG_DOCU_VIEWCOUNT:
    case T5_MSG_DOCU_MODSTATUS:
    case T5_MSG_DOCU_READWRITE:
        return(sk_cont_msg_docu__title(p_docu));

    case T5_MSG_SELECTION_NEW:
        /* reflect pertinent style from whatever the selection is */
        skel_controls_query_and_encode_style_bitmap(p_docu, &skel_controls_selector_all);
        return(STATUS_OK);

    case T5_MSG_CUR_CHANGE_AFTER:
    case T5_MSG_FOCUS_CHANGED:
    case T5_MSG_DOCU_COLROW:
        /* on larger changes of position, read all pertinent style */
        skel_controls_query_and_encode_style_bitmap(p_docu, &skel_controls_selector_all);
        return(STATUS_OK);

    case T5_MSG_DOCU_CARETMOVE:
        /* on smaller (sub-cell) changes of position, read just text style */
        if(slr_equal(&p_docu->cur.slr, &p_docu->old.slr))
            skel_controls_query_and_encode_style_bitmap(p_docu, &skel_controls_selector_character);
        /* otherwise we'll get a T5_MSG_DOCU_COLROW any minute now */
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_cont);

_Check_return_
static STATUS
skel_controls_msg_startup(void)
{
    STATUS status;

    T5_TOOLBAR_TOOLS t5_toolbar_tools;
    t5_toolbar_tools.n_tool_desc = elemof32(skel_tools);
    t5_toolbar_tools.p_t5_toolbar_tool_desc = skel_tools;
    status = object_call_id(OBJECT_ID_TOOLBAR, P_DOCU_NONE, T5_MSG_TOOLBAR_TOOLS, &t5_toolbar_tools);

    skel_controls_selectors_init();

    return(status);
}

_Check_return_
static STATUS
skel_controls_msg_init2(
    _DocuRef_   P_DOCU p_docu)
{
    static const PC_USTR tools[] = /* some tools ought to be enabled all the time */
    {
        USTR_TEXT("VIEW"),
        USTR_TEXT("NEW"),
#if WINDOWS
        USTR_TEXT("OPEN"),
#endif
        USTR_TEXT("SAVE"),
        USTR_TEXT("PRINT"),
        USTR_TEXT("SELECTION"),
        USTR_TEXT("TAB_LEFT"),
        USTR_TEXT("TAB_CENTRE"),
        USTR_TEXT("TAB_RIGHT"),
        USTR_TEXT("TAB_DECIMAL")
    };

    U32 i;

    for(i = 0; i < elemof32(tools); ++i)
    {
        T5_TOOLBAR_TOOL_ENABLE t5_toolbar_tool_enable;
        t5_toolbar_tool_enable.enabled = 1;
        t5_toolbar_tool_enable.enable_id = 0;
        tool_enable(p_docu, &t5_toolbar_tool_enable, tools[i]);
    }

    proc_event_sk_cont_direct(p_docu, T5_MSG_SET_TAB_LEFT, P_DATA_NONE); /* initialise TAB controls UI and ruler code */

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_sk_cont_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(skel_controls_msg_startup());

    case T5_MSG_IC__INIT1:
        return(maeve_event_handler_add(p_docu, maeve_event_sk_cont, (CLIENT_HANDLE) 0));

    case T5_MSG_IC__INIT2:
        return(skel_controls_msg_init2(p_docu));

    case T5_MSG_IC__CLOSE1:
        maeve_event_handler_del(p_docu, maeve_event_sk_cont, (CLIENT_HANDLE) 0);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_cont)
{
    UNREFERENCED_PARAMETER_InVal_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_cont_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* given a bitmap of style selector bits, ask what styles are known about in
* the selection and if they are fuzzy or not, and encode into the justify and
* style groups
*
******************************************************************************/

static const
struct BBAR_STYLES
{
    PC_USTR name;
    STYLE_BIT_NUMBER style_bit_number;
    U32 offset;
}
bbar_styles[] =
{
    { USTR_TEXT("BOLD"),        STYLE_SW_FS_BOLD,        offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, bold) },
    { USTR_TEXT("ITALIC"),      STYLE_SW_FS_ITALIC,      offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, italic) },
    { USTR_TEXT("UNDERLINE"),   STYLE_SW_FS_UNDERLINE,   offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, underline) },
    { USTR_TEXT("SUPERSCRIPT"), STYLE_SW_FS_SUPERSCRIPT, offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, superscript) },
    { USTR_TEXT("SUBSCRIPT"),   STYLE_SW_FS_SUBSCRIPT,   offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, subscript) }
};

#define N_BBAR_STYLES elemof32(bbar_styles)

static void
skel_controls_query_and_encode_style_bitmap(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    T5_TOOLBAR_TOOL_SET t5_toolbar_tool_set;
    SELECTION_STYLE selection_style;
    STYLE_BIT_NUMBER style_bit_number;

    /* enquire about just these bits of state */
    style_selector_copy(&selection_style.selector_in, p_style_selector);

    style_selector_clear(&selection_style.selector_fuzzy_out);

    style_init(&selection_style.style_out);

    /* ignore transient styles */
    style_region_class_limit_set(p_docu, REGION_UPPER);
    /* may return STATUS_FAIL but we've had to take care of style & selectors anyway */
    status_consume(object_skel(p_docu, T5_MSG_SELECTION_STYLE, &selection_style));
    style_region_class_limit_set(p_docu, REGION_END);

    { /* anything that isn't defined gets classed as fuzzy */
    STYLE_SELECTOR selector;
    void_style_selector_not(&selector, &selection_style.style_out.selector);
    void_style_selector_or(&selection_style.selector_fuzzy_out, &selection_style.selector_fuzzy_out, &selector);
    }

    style_bit_number = STYLE_SW_PS_JUSTIFY;

    if(style_selector_bit_test(p_style_selector, style_bit_number))
    {
        if(style_selector_bit_test(&selection_style.selector_fuzzy_out, style_bit_number))
            selection_style.style_out.para_style.justify = 0xFF;

        t5_toolbar_tool_set.state.state = (selection_style.style_out.para_style.justify == SF_JUSTIFY_LEFT);
        t5_toolbar_tool_set.name = USTR_TEXT("JUSTIFY_LEFT");
        status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_SET, &t5_toolbar_tool_set));

        t5_toolbar_tool_set.state.state = (selection_style.style_out.para_style.justify == SF_JUSTIFY_CENTRE);
        t5_toolbar_tool_set.name = USTR_TEXT("JUSTIFY_CENTRE");
        status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_SET, &t5_toolbar_tool_set));

        t5_toolbar_tool_set.state.state = (selection_style.style_out.para_style.justify == SF_JUSTIFY_RIGHT);
        t5_toolbar_tool_set.name = USTR_TEXT("JUSTIFY_RIGHT");
        status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_SET, &t5_toolbar_tool_set));

        t5_toolbar_tool_set.state.state = (selection_style.style_out.para_style.justify == SF_JUSTIFY_BOTH);
        t5_toolbar_tool_set.name = USTR_TEXT("JUSTIFY_FULL");
        status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_SET, &t5_toolbar_tool_set));
    }

    /* loop over bbar styles */
    {
    UINT i;

    for(i = 0; i < N_BBAR_STYLES; ++i)
    {
        style_bit_number = bbar_styles[i].style_bit_number;

        if(style_selector_bit_test(p_style_selector, style_bit_number))
        {
            P_U8 p_style_attrib = (P_U8) &selection_style.style_out + bbar_styles[i].offset;

            if(style_selector_bit_test(&selection_style.selector_fuzzy_out, style_bit_number))
                *p_style_attrib = 0;

            t5_toolbar_tool_set.state.state = *p_style_attrib;
            t5_toolbar_tool_set.name = bbar_styles[i].name;
            status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_SET, &t5_toolbar_tool_set));
        }
    }
    } /*block*/

    style_dispose(&selection_style.style_out);
}

static void
skel_controls_selectors_init(void)
{
    UINT i;

    for(i = 0; i < N_BBAR_STYLES; ++i)
        style_selector_bit_set(&skel_controls_selector_character, bbar_styles[i].style_bit_number);

    style_selector_copy(&skel_controls_selector_all, &skel_controls_selector_character);

    style_selector_bit_set(&skel_controls_selector_all, STYLE_SW_PS_JUSTIFY);
}

_Check_return_
static STATUS
skel_style_set(
    _DocuRef_   P_DOCU p_docu,
    STYLE_BIT_NUMBER style_bit_number,
    _InVal_     U8 attr)
{
    STYLE style;
    P_U8 p_style_attr = p_style_member(U8, &style, style_bit_number);
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;

    style_init(&style);

    *p_style_attr = attr;
    style_bit_set(&style, style_bit_number);

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_STYLE_APPLY, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 512);
        quick_ublock_with_buffer_setup(quick_ublock);
#if 1
        if(attr) /* turn other style attribute off if one is going on */
        {
            if(style_bit_number == STYLE_SW_FS_SUPERSCRIPT)
            {
                style.font_spec.subscript = 0;
                style_bit_set(&style, STYLE_SW_FS_SUBSCRIPT);
            }
            else if(style_bit_number == STYLE_SW_FS_SUBSCRIPT)
            {
                style.font_spec.superscript = 0;
                style_bit_set(&style, STYLE_SW_FS_SUPERSCRIPT);
            }
        }
#else
        if(attr) /* turn other style attribute off if one is going on */
        {
            BOOL do_off_style = FALSE;
            STYLE off_style;
            style_init(&off_style);
            if(style_bit_number == STYLE_SW_FS_SUPERSCRIPT)
            {
                off_style.font_spec.subscript = 0;
                bitmap_bit_set(off_style.selector, STYLE_SW_FS_SUBSCRIPT);
                do_off_style = TRUE;
            }
            else if(style_bit_number == STYLE_SW_FS_SUBSCRIPT)
            {
                off_style.font_spec.superscript = 0;
                bitmap_bit_set(off_style.selector, STYLE_SW_FS_SUPERSCRIPT);
                do_off_style = TRUE;
            }
            if(do_off_style && status_ok(status = style_ustr_inline_from_struct(p_docu, &quick_ublock, &off_style)))
            {
                const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
                p_args[0].val.ustr_inline = quick_ublock_ustr_inline(&quick_ublock);
                status = execute_command(object_id, p_docu, T5_CMD_STYLE_APPLY, &arglist_handle);
                quick_ublock_dispose(&quick_ublock);
            }
        }
#endif
        if(status_ok(status = style_ustr_inline_from_struct(p_docu, &quick_ublock, &style)))
        {
            p_args[0].val.ustr_inline = quick_ublock_ustr_inline(&quick_ublock);
            status = execute_command(p_docu, T5_CMD_STYLE_APPLY, &arglist_handle, object_id);
            quick_ublock_dispose(&quick_ublock);
        }
        arglist_dispose(&arglist_handle);
    }

    /* reencode selection's state into tools' state etc. whatever status happened */
    skel_controls_query_and_encode_style_bitmap(p_docu, &style.selector);

    return(status);
}

static STATUS
encode_toolbar_tabs(
    _DocuRef_   P_DOCU p_docu)
{
    /* set up tabs user interface in toolbar */
    T5_TOOLBAR_TOOL_SET t5_toolbar_tool_set;

    t5_toolbar_tool_set.state.state = (p_docu->insert_tab_type == TAB_LEFT);
    t5_toolbar_tool_set.name = USTR_TEXT("TAB_LEFT");
    status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_SET, &t5_toolbar_tool_set));

    t5_toolbar_tool_set.state.state = (p_docu->insert_tab_type == TAB_CENTRE);
    t5_toolbar_tool_set.name = USTR_TEXT("TAB_CENTRE");
    status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_SET, &t5_toolbar_tool_set));

    t5_toolbar_tool_set.state.state = (p_docu->insert_tab_type == TAB_RIGHT);
    t5_toolbar_tool_set.name = USTR_TEXT("TAB_RIGHT");
    status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_SET, &t5_toolbar_tool_set));

    t5_toolbar_tool_set.state.state = (p_docu->insert_tab_type == TAB_DECIMAL);
    t5_toolbar_tool_set.name = USTR_TEXT("TAB_DECIMAL");
    status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_SET, &t5_toolbar_tool_set));

    return(STATUS_OK);
}

PROC_EVENT_PROTO(extern, proc_event_sk_cont_direct)
{
    switch(t5_message)
    {
#if 0
    case T5_MSG_STATUS_LINE_MESSAGE_QUERY:
        {
        P_UI_TEXT p_ui_text = (P_UI_TEXT) p_data;
        if(p_ui_text->type == UI_TEXT_TYPE_RESID)
            if(p_ui_text->text.resource_id == MSG_STATUS_APPLY_STYLE)
                status_consume(object_call_id(OBJECT_ID_REC, p_docu, t5_message, p_data));
        }
        return(STATUS_OK);
#endif

    case T5_MSG_SET_TAB_LEFT:
    /*case T5_MSG_SET_TAB_CENTRE:*/
    /*case T5_MSG_SET_TAB_RIGHT:*/
    /*case T5_MSG_SET_TAB_DECIMAL:*/
        return(encode_toolbar_tabs(p_docu));

    case T5_MSG_STYLE_CHANGE_JUSTIFY_LEFT:
    case T5_MSG_STYLE_CHANGE_JUSTIFY_CENTRE:
    case T5_MSG_STYLE_CHANGE_JUSTIFY_RIGHT:
    case T5_MSG_STYLE_CHANGE_JUSTIFY_FULL:
        return(skel_style_set(p_docu, STYLE_SW_PS_JUSTIFY, (U8) ((t5_message - T5_MSG_STYLE_CHANGE_JUSTIFY_LEFT) + SF_JUSTIFY_LEFT)));

    case T5_MSG_STYLE_CHANGE_BOLD:
        {
        const P_T5_TOOLBAR_TOOL_STATE_CHANGE p_t5_toolbar_tool_state_change = (P_T5_TOOLBAR_TOOL_STATE_CHANGE) p_data;
        return(skel_style_set(p_docu, STYLE_SW_FS_BOLD, (U8) p_t5_toolbar_tool_state_change->proposed_state.state));
        }

    case T5_MSG_STYLE_CHANGE_ITALIC:
        {
        const P_T5_TOOLBAR_TOOL_STATE_CHANGE p_t5_toolbar_tool_state_change = (P_T5_TOOLBAR_TOOL_STATE_CHANGE) p_data;
        return(skel_style_set(p_docu, STYLE_SW_FS_ITALIC, (U8) p_t5_toolbar_tool_state_change->proposed_state.state));
        }

    case T5_MSG_STYLE_CHANGE_UNDERLINE:
        {
        const P_T5_TOOLBAR_TOOL_STATE_CHANGE p_t5_toolbar_tool_state_change = (P_T5_TOOLBAR_TOOL_STATE_CHANGE) p_data;
        return(skel_style_set(p_docu, STYLE_SW_FS_UNDERLINE, (U8) p_t5_toolbar_tool_state_change->proposed_state.state));
        }

    case T5_MSG_STYLE_CHANGE_SUPERSCRIPT:
        {
        const P_T5_TOOLBAR_TOOL_STATE_CHANGE p_t5_toolbar_tool_state_change = (P_T5_TOOLBAR_TOOL_STATE_CHANGE) p_data;
        return(skel_style_set(p_docu,  STYLE_SW_FS_SUPERSCRIPT, (U8) p_t5_toolbar_tool_state_change->proposed_state.state));
        }

    case T5_MSG_STYLE_CHANGE_SUBSCRIPT:
        {
        const P_T5_TOOLBAR_TOOL_STATE_CHANGE p_t5_toolbar_tool_state_change = (P_T5_TOOLBAR_TOOL_STATE_CHANGE) p_data;
        return(skel_style_set(p_docu, STYLE_SW_FS_SUBSCRIPT, (U8) p_t5_toolbar_tool_state_change->proposed_state.state));
        }

    default: default_unhandled();
        return(STATUS_OK);
    }
}

/* end of sk_cont.c */
