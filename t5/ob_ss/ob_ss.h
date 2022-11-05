/* ob_ss.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Spreadsheet object module internal header */

/* MRJC April 1992 */

#ifndef __ob_ss_h
#define __ob_ss_h

#ifndef       __xp_ss_h
#include "ob_ss/xp_ss.h"
#endif

#include "cmodules/ev_evali.h"

#include "ob_ss/resource/resource.h"

/*
sle data
*/

typedef struct SLE_INFO_BLOCK
{
    DOCNO docno;                    /* document number */
    VIEWNO prefered_viewno;         /* prefered view no */

    BOOL has_focus;                 /* field has input focus */
    BOOL status_line_set;           /* have we set the status line */

    ARRAY_HANDLE_USTR h_ustr_inline;

    S32 caret_index;                /* index of caret in ustr_inline */

    BOOL selection;                 /* is there a selection? */
    BOOL pseudo_selection;          /* pseudo selection for cell references */
    S32 selection_stt_index;        /* index of selection stt - valid only if 'selection' */
    S32 selection_end_index;        /* index of selection end - valid only if 'selection'*/

    struct SLE_INFO_BLOCK_DISPLAY
    {
#if WINDOWS
        int caret_pixels;           /* caret position in pixels from edge of window */
        int selection_stt_pixels;   /* selection left edge in pixels */
        int selection_end_pixels;   /* selection right edge in pixels */
        int scroll_offset_pixels;   /* scroll offset in pixels */
        int string_end_pixels;      /* position in pixels of the end of the string */
#else
        PIXIT caret_pixits;         /* caret position */
        PIXIT selection_stt_pixits; /* selection stt */
        PIXIT selection_end_pixits; /* selection end */
        PIXIT scroll_offset_pixits; /* scroll offset */
#endif
    }
    display;

    PIXIT_RECT pixit_rect;

#if WINDOWS
    BOOL captured;                  /* mouse is captured */
#endif
}
SLE_INFO_BLOCK, * P_SLE_INFO_BLOCK;

/*
instance data for the sle
*/

typedef struct SLE_INSTANCE_DATA
{
    SLE_INFO_BLOCK ss_editor;
    S32 maeve_click_filter_handle;
}
SLE_INSTANCE_DATA, * P_SLE_INSTANCE_DATA;

_Check_return_
_Ret_valid_
static inline P_SLE_INSTANCE_DATA
p_object_instance_data_SLE(
    _InRef_     P_DOCU p_docu)
{
    P_SLE_INSTANCE_DATA p_sle_instance_data = (P_SLE_INSTANCE_DATA)
        _p_object_instance_data(p_docu, OBJECT_ID_SLE CODE_ANALYSIS_ONLY_ARG(sizeof32(SLE_INSTANCE_DATA)));
    PTR_ASSERT(p_sle_instance_data);
    return(p_sle_instance_data);
}

/*
Structure used for passing text into the editline
*/

typedef struct T5_PASTE_EDITLINE
{
    BOOL select;
    BOOL special_select;
    PC_QUICK_UBLOCK p_quick_ublock;
}
T5_PASTE_EDITLINE, * P_T5_PASTE_EDITLINE;

/*
Structure used for obtaining function argument help for the SLE
*/

typedef struct SS_FUNCTION_ARGUMENT_HELP
{
    /*IN*/
    STATUS help_id;
    U32 arg_index;

    /*OUT*/
    P_QUICK_UBLOCK p_quick_ublock; /*appended*/
}
SS_FUNCTION_ARGUMENT_HELP, * P_SS_FUNCTION_ARGUMENT_HELP;

/*
sle_ss.c
*/

_Check_return_
extern STATUS
sle_change_text(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  PC_QUICK_UBLOCK p_quick_ublock,
    _InVal_     BOOL select,
    _InVal_     BOOL pseudo_select);

_Check_return_
extern STATUS
sle_create(
    _DocuRef_   P_DOCU p_docu);

extern HOST_WND
sle_view_create(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

extern void
sle_view_destroy(
    _ViewRef_   P_VIEW p_view);

_Check_return_
extern STATUS
sle_tool_user_mouse(
    _DocuRef_   P_DOCU p_docu,
    P_ANY /*T5_TOOLBAR_TOOL_USER_MOUSE*/ p_data);

extern void
sle_view_prefer(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

_Check_return_
extern STATUS
sle_tool_user_redraw(
    _DocuRef_   P_DOCU p_docu,
    P_ANY /*T5_TOOLBAR_TOOL_USER_REDRAW*/ p_data);

_Check_return_
extern STATUS
sle_tool_user_posn_set(
    _DocuRef_   P_DOCU p_docu,
    P_ANY /*T5_TOOLBAR_TOOL_USER_POSN_SET*/ p_data);

/*
internal export from ui_name.c
*/

T5_CMD_PROTO(extern, t5_cmd_ss_name_intro);

/*
internal exports from ui_ss.h
*/

PROC_EVENT_PROTO(extern, proc_event_ui_ss_direct);

_Check_return_
extern STATUS
ss_backcontrols_enable(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
ss_formula_reflect_contents(
    _DocuRef_   P_DOCU p_docu);

#endif /* __ob_ss_h */

/* end of ob_ss.h */
