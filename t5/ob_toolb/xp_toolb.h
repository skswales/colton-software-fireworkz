/* xp_toolb.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Toolbar object module header */

/* SKS December 1993 */

#ifndef __xp_toolb_h
#define __xp_toolb_h

/* tools are always identifed by a string */

typedef enum T5_TOOLBAR_TOOL_TYPE
{
    T5_TOOLBAR_TOOL_TYPE_COMMAND = 1, /* simple pushbuttons */
    T5_TOOLBAR_TOOL_TYPE_STATE, /* bold, italic etc. ***not*** auto class */
    T5_TOOLBAR_TOOL_TYPE_DROPDOWN, /* font, size dropdowns */
    T5_TOOLBAR_TOOL_TYPE_USER /* only formula line */
}
T5_TOOLBAR_TOOL_TYPE;

typedef struct T5_TOOLBAR_TOOL_DESC_BITS
{
    UBF type : 8;       /* packed T5_TOOLBAR_TOOL_TYPE */
    UBF immediate : 1;  /* act on key down event, not on key up */
    UBF set_view : 1;   /* spectacles box etc. */
    UBF thin : 1;       /* e.g. for superscript, tabs */
    UBF auto_repeat : 1;
}
T5_TOOLBAR_TOOL_DESC_BITS;

typedef struct T5_TOOLBAR_TOOL_DESC
{
    PC_USTR name;
    OBJECT_ID command_object_id; /* for command & state change */
    T5_MESSAGE t5_message;
    OBJECT_ID resource_object_id;
#if RISCOS
    PC_SBSTR resource_id; /* sprite name - can't be UTF-8 */
    PC_SBSTR resource_id_on; /* ditto, for 'on' state */
#else
    UINT resource_id; /* for LoadImage */
    UINT resource_id_on; /* ditto, for 'on' state */
#endif
    T5_TOOLBAR_TOOL_DESC_BITS bits;
    UI_TEXT ui_text; /* help text */
    T5_MESSAGE t5_message_alternate;
}
T5_TOOLBAR_TOOL_DESC /*, * P_T5_TOOLBAR_TOOL_DESC*/; typedef const T5_TOOLBAR_TOOL_DESC * PC_T5_TOOLBAR_TOOL_DESC;

typedef struct T5_TOOLBAR_TOOLS
{
    /*IN*/
    S32 n_tool_desc;
    PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc;
}
T5_TOOLBAR_TOOLS /*, * P_T5_TOOLBAR_TOOLS*/; typedef const T5_TOOLBAR_TOOLS * PC_T5_TOOLBAR_TOOLS;

typedef union T5_TOOLBAR_TOOL_STATE
{
    BOOL state;
    UI_TEXT dropdown;
}
T5_TOOLBAR_TOOL_STATE;

typedef struct T5_TOOLBAR_TOOL_DISABLE
{
    /*IN*/
    PC_USTR name;
    BOOL disabled;
    U8 disable_id;
}
T5_TOOLBAR_TOOL_DISABLE /*, * P_T5_TOOLBAR_TOOL_DISABLE*/; typedef const T5_TOOLBAR_TOOL_DISABLE * PC_T5_TOOLBAR_TOOL_DISABLE;

typedef struct T5_TOOLBAR_TOOL_ENABLE
{
    /*IN*/
    PC_USTR name;
    BOOL enabled;
    U8 enable_id;
}
T5_TOOLBAR_TOOL_ENABLE, * P_T5_TOOLBAR_TOOL_ENABLE; typedef const T5_TOOLBAR_TOOL_ENABLE * PC_T5_TOOLBAR_TOOL_ENABLE;

typedef struct T5_TOOLBAR_TOOL_NOBBLE
{
    /*IN*/
    PC_USTR name;
    BOOL nobbled;
}
T5_TOOLBAR_TOOL_NOBBLE /*, * P_T5_TOOLBAR_TOOL_NOBBLE*/; typedef const T5_TOOLBAR_TOOL_NOBBLE * PC_T5_TOOLBAR_TOOL_NOBBLE;

typedef struct T5_TOOLBAR_TOOL_QUERY
{
    /*IN*/
    PC_USTR name;

    /*OUT*/
    T5_TOOLBAR_TOOL_STATE state;
}
T5_TOOLBAR_TOOL_QUERY, * P_T5_TOOLBAR_TOOL_QUERY;

typedef struct T5_TOOLBAR_TOOL_SET
{
    /*IN*/
    PC_USTR name;
    T5_TOOLBAR_TOOL_STATE state;
}
T5_TOOLBAR_TOOL_SET /*, * P_T5_TOOLBAR_TOOL_SET*/; typedef const T5_TOOLBAR_TOOL_SET * PC_T5_TOOLBAR_TOOL_SET;

typedef struct T5_TOOLBAR_TOOL_ENABLE_QUERY
{
    /*IN*/
    PC_USTR name;

    /*OUT*/
    BOOL enabled;
}
T5_TOOLBAR_TOOL_ENABLE_QUERY, * P_T5_TOOLBAR_TOOL_ENABLE_QUERY;

/* messages back to client */

typedef struct T5_TOOLBAR_TOOL_STATE_CHANGE
{
    /*IN*/
    PC_USTR name;
    T5_TOOLBAR_TOOL_STATE current_state;
    T5_TOOLBAR_TOOL_STATE proposed_state;
}
T5_TOOLBAR_TOOL_STATE_CHANGE, * P_T5_TOOLBAR_TOOL_STATE_CHANGE;

typedef struct T5_TOOLBAR_TOOL_USER_VIEW_NEW
{
    /*IN:0;OUT:42*/
    CLIENT_HANDLE client_handle;
    /*IN*/
    PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc;
    VIEWNO viewno;
}
T5_TOOLBAR_TOOL_USER_VIEW_NEW, * P_T5_TOOLBAR_TOOL_USER_VIEW_NEW;

typedef struct T5_TOOLBAR_TOOL_USER_VIEW_DELETE
{
    /*IN*/
    CLIENT_HANDLE client_handle;
    PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc;
    VIEWNO viewno;
}
T5_TOOLBAR_TOOL_USER_VIEW_DELETE /*, * P_T5_TOOLBAR_TOOL_USER_VIEW_DELETE*/; typedef T5_TOOLBAR_TOOL_USER_VIEW_DELETE * PC_T5_TOOLBAR_TOOL_USER_VIEW_DELETE;

typedef struct T5_TOOLBAR_TOOL_USER_SIZE_QUERY
{
    /*IN*/
    CLIENT_HANDLE client_handle;
    PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc;
    VIEWNO viewno;

    PIXIT_SIZE pixit_size; /*INOUT*/
}
T5_TOOLBAR_TOOL_USER_SIZE_QUERY, * P_T5_TOOLBAR_TOOL_USER_SIZE_QUERY;

typedef struct T5_TOOLBAR_TOOL_USER_POSN_SET
{
    /*IN*/
    CLIENT_HANDLE client_handle;
    PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc;
    VIEWNO viewno;

    PIXIT_RECT pixit_rect;
}
T5_TOOLBAR_TOOL_USER_POSN_SET /*, * P_T5_TOOLBAR_TOOL_USER_POSN_SET*/; typedef const T5_TOOLBAR_TOOL_USER_POSN_SET * PC_T5_TOOLBAR_TOOL_USER_POSN_SET;

typedef struct BACK_WINDOW_EVENT
{
    T5_MESSAGE t5_message;
    P_ANY p_data;
    BOOL processed;
}
BACK_WINDOW_EVENT, * P_BACK_WINDOW_EVENT;

/*
T5_MSG_TOOLBAR_CODE_CTL_USER_MOUSE
*/

typedef struct T5_TOOLBAR_TOOL_USER_MOUSE
{
    /*IN*/
    CLIENT_HANDLE client_handle;
    PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc;
    VIEWNO viewno;

    SKELEVENT_CLICK skelevent_click; /*IN;modified*/
    T5_MESSAGE t5_message; /*IN*/
    PIXIT_RECT pixit_rect; /*IN*/
}
T5_TOOLBAR_TOOL_USER_MOUSE, * P_T5_TOOLBAR_TOOL_USER_MOUSE;

/*
T5_MSG_TOOLBAR_TOOL_USER_REDRAW
*/

typedef struct T5_TOOLBAR_TOOL_USER_REDRAW
{
    /*IN*/
    CLIENT_HANDLE client_handle;
    PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc;
    VIEWNO viewno;

    REDRAW_CONTEXT redraw_context; /*IN;modified*/
    PIXIT_RECT control_outer_pixit_rect; /*IN*/
    PIXIT_RECT control_inner_pixit_rect; /*IN*/

    S32 enabled; /*IN*/
}
T5_TOOLBAR_TOOL_USER_REDRAW, * P_T5_TOOLBAR_TOOL_USER_REDRAW;

#define TOOL_ENABLE_STANDARD 0
#define TOOL_ENABLE_HEFO_FOCUS 1
#define TOOL_ENABLE_HEFO_FOCUS_AND_SELECTION 2
#define TOOL_ENABLE_CELLS_FOCUS 3
#define TOOL_ENABLE_CELLS_FOCUS_AND_SELECTION 4
#define TOOL_ENABLE_REC_FOCUS 5
#define TOOL_ENABLE_NOTE_FOCUS 6

_Check_return_
extern STATUS
_tool_enable( /* direct call */
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_T5_TOOLBAR_TOOL_ENABLE p_t5_toolbar_tool_enable);

static void inline
tool_enable(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_T5_TOOLBAR_TOOL_ENABLE p_t5_toolbar_tool_enable,
    _In_z_      PC_USTR name)
{
    p_t5_toolbar_tool_enable->name = name;

#if defined(BIND_OB_TOOLBAR) /* direct call is possible */
    status_consume(_tool_enable(p_docu, p_t5_toolbar_tool_enable));
#else /* must call via object interface with message */
    status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_ENABLE, p_t5_toolbar_tool_enable));
#endif /* BIND_OB_TOOLBAR */
}

#endif /* __xp_toolb_h */

/* end of xp_toolb.h */
