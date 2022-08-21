/* ob_toolb.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "ob_toolb/xp_toolb.h"

/* #define TOOLB_MSG_BASE    (STATUS_MSG_INCREMENT * OBJECT_ID_TOOLBAR) */

/*#define TOOLB_MSG_STATUS_LINE_spare_1 (TOOLB_MSG_BASE + 1)*/

#define TOOLB_ERR_BASE  (STATUS_ERR_INCREMENT * OBJECT_ID_TOOLBAR)

#define TOOLB_ERR(n)    (TOOLB_ERR_BASE - (n))

#define TOOLB_ERR_UNKNOWN_CONTROL   TOOLB_ERR(0)
/* -1 is ERROR_RQ */
#define TOOLB_ERR_spare             TOOLB_ERR(2)


#if WINDOWS
#include "external/Microsoft/InsideOLE2/BTTNCURP/bttncur.h"
/* NB If this #include fails, try
 * Build\w32\setup.cmd
 */
#endif

/*
internal types
*/

#define BUF_MAX_T5_TOOLBAR_NAME 16

typedef struct T5_TOOLBAR_REQUESTED_TOOL_DESC
{
    /*IN*/
    UCHARZ name[BUF_MAX_T5_TOOLBAR_NAME];
    U8 row;
}
T5_TOOLBAR_REQUESTED_TOOL_DESC /*, * P_T5_TOOLBAR_REQUESTED_TOOL_DESC*/; typedef const T5_TOOLBAR_REQUESTED_TOOL_DESC * PC_T5_TOOLBAR_REQUESTED_TOOL_DESC;

/*typedef struct T5_TOOLBAR_DISALLOWED_TOOL_DESC
{
    UCHARZ name[BUF_MAX_T5_TOOLBAR_NAME];
}
T5_TOOLBAR_DISALLOWED_TOOL_DESC, * P_T5_TOOLBAR_DISALLOWED_TOOL_DESC;*/

typedef struct T5_TOOLBAR_DOCU_TOOL_DESC
{
    PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc;
    U32 name_hash; /* hashed to save derefs in lookup */
    T5_TOOLBAR_TOOL_STATE state;
    BITMAP(enable_state, 32);
    BITMAP(disable_state, 32);
    BOOL nobble_state;
}
T5_TOOLBAR_DOCU_TOOL_DESC, * P_T5_TOOLBAR_DOCU_TOOL_DESC; typedef const T5_TOOLBAR_DOCU_TOOL_DESC * PC_T5_TOOLBAR_DOCU_TOOL_DESC;

typedef struct T5_TOOLBAR_VIEW_ROW_DESC
{
    PIXIT height;
    ARRAY_HANDLE h_toolbar_view_tool_desc; /* [] of T5_TOOLBAR_VIEW_TOOL_DESC */
    U32 scheduled_event_id;
}
T5_TOOLBAR_VIEW_ROW_DESC, * P_T5_TOOLBAR_VIEW_ROW_DESC; typedef const T5_TOOLBAR_VIEW_ROW_DESC * PC_T5_TOOLBAR_VIEW_ROW_DESC;

typedef struct T5_TOOLBAR_VIEW_TOOL_DESC
{
    ARRAY_INDEX docu_tool_index;
    UINT button_down;
    BOOL had_mouse_down;
    BOOL separator;
    U8 rhs_extra_div_20;
    PIXIT_RECT pixit_rect;
    CLIENT_HANDLE client_handle;
    S32 redraw_count;
}
T5_TOOLBAR_VIEW_TOOL_DESC, * P_T5_TOOLBAR_VIEW_TOOL_DESC; typedef const T5_TOOLBAR_VIEW_TOOL_DESC * PC_T5_TOOLBAR_VIEW_TOOL_DESC;

/*
internally exported routines
*/

#if WINDOWS

_Check_return_
extern STATUS
status_line_register_wndclass(void);

_Check_return_
extern STATUS
toolbar_register_wndclass(void);

#endif /* WINDOWS */

extern void
execute_tool(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_maybenone_ P_VIEW p_view,
    _InRef_     PC_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc,
    _InVal_     BOOL alternate);

#if WINDOWS

extern void
paint_status_line_windows(
    _HwndRef_   HWND hwnd,
    _InRef_     PPAINTSTRUCT p_paintstruct,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

extern void
paint_toolbar_windows(
    _HwndRef_   HWND hwnd,
    _InRef_     PPAINTSTRUCT p_paintstruct,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

#endif /* WINDOWS */

extern void
redraw_tool(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc);

_Check_return_
extern BOOL
tool_enabled_query(
    _InRef_     PC_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc);

_Check_return_
_Ret_maybenull_
extern P_T5_TOOLBAR_VIEW_TOOL_DESC
tool_from_point(
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_PIXIT_POINT p_pixit_point);

#if WINDOWS

T5_MSG_PROTO(extern, t5_msg_frame_windows_describe, _InoutRef_ P_MSG_FRAME_WINDOW p_msg_frame_window);
T5_MSG_PROTO(extern, t5_msg_frame_windows_posn, _InoutRef_ P_MSG_FRAME_WINDOW p_msg_frame_window);

extern GDI_COORD status_line_height_pixels;

extern GDI_COORD toolbar_height_pixels;

#endif /* WINDOWS */

/* end of ob_toolb.h */
