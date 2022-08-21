/* xp_dlgr.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* dialog manager header - host specific structures (RISC OS) */

/* SKS July 1992 */

#if RISCOS

#ifndef __xp_dlgr_h
#define __xp_dlgr_h

#ifndef         __xp_skelr_h
#include "ob_skel/xp_skelr.h"
#endif

_Check_return_
extern STATUS
dialog_riscos_file_icon_drag(
    _InVal_     H_DIALOG h_dialog,
    _In_        WimpIconBlockWithBitset * const p_icon);

extern void
dialog_riscos_file_icon_setup(
    _Inout_     WimpIconBlockWithBitset * const p_icon,
    _InVal_     T5_FILETYPE t5_filetype);

struct DIALOG_MSG_CTL_USER_MOUSE_RISCOS
{
    wimp_w window_handle;
    WimpIconBlockWithBitset icon;
};

/*
DIALOG_MSG_CODE_CTL_USER_MOUSE
*/

struct DIALOG_MSG_CTL_USER_MOUSE
{
    DIALOG_MSG_CTL_HDR_DEF

    DIALOG_MSG_CTL_USER_MOUSE_CLICK click; /*IN*/

    struct DIALOG_MSG_CTL_USER_MOUSE_RISCOS riscos;
};

/*
DIALOG_MSG_CODE_CTL_USER_POINTER_QUERY
*/

struct DIALOG_MSG_CTL_USER_POINTER_QUERY
{
    DIALOG_MSG_CTL_HDR_DEF

    POINTER_SHAPE pointer_shape; /*IN=0;OUT*/

    struct DIALOG_MSG_CTL_USER_MOUSE_RISCOS riscos;
};

/*
DIALOG_MSG_CODE_CTL_USER_REDRAW
*/

struct DIALOG_MSG_CTL_USER_REDRAW
{
    DIALOG_MSG_CTL_HDR_DEF

    REDRAW_CONTEXT redraw_context;    /*IN*/
    PIXIT_RECT     control_outer_box; /*IN*/
    PIXIT_RECT     control_inner_box; /*IN*/

    struct DIALOG_MSG_CTL_USER_REDRAW_RISCOS
    {
        WimpRedrawWindowBlock redraw_window; /*IN*/
    }
    riscos;

    S32 enabled; /*IN*/
};

/*
DIALOG_MSG_CODE_RISCOS_DRAG_ENDED
*/

typedef struct DIALOG_MSG_RISCOS_DRAG_ENDED
{
    DIALOG_MSG_CTL_HDR_DEF

    WimpGetPointerInfoBlock mouse; /*IN*/
}
DIALOG_MSG_RISCOS_DRAG_ENDED, * P_DIALOG_MSG_RISCOS_DRAG_ENDED;

/*
DIALOG_RISCOS_EVENT_CODE_REDRAW_WINDOW
*/

typedef struct DIALOG_RISCOS_EVENT_REDRAW_WINDOW
{
    H_DIALOG       h_dialog; /*IN*/
    STATUS         is_update_now; /*IN*/
    REDRAW_CONTEXT redraw_context; /*IN*/

    PIXIT_RECT     area_rect; /*IN*/
}
DIALOG_RISCOS_EVENT_REDRAW_WINDOW, * P_DIALOG_RISCOS_EVENT_REDRAW_WINDOW;

/*
DIALOG_RISCOS_EVENT_CODE_MOUSE_CLICK
*/

typedef struct DIALOG_RISCOS_EVENT_MOUSE_CLICK
{
    H_DIALOG h_dialog; /*IN*/

    WimpMouseClickEvent mouse_click; /*IN*/
}
DIALOG_RISCOS_EVENT_MOUSE_CLICK, * P_DIALOG_RISCOS_EVENT_MOUSE_CLICK;

/*
DIALOG_RISCOS_EVENT_CODE_KEY_PRESSED
*/

typedef struct DIALOG_RISCOS_EVENT_KEY_PRESSED
{
    H_DIALOG h_dialog; /*IN*/

    WimpKeyPressedEvent key_pressed; /*IN*/

    BOOL processed; /*OUT*/
}
DIALOG_RISCOS_EVENT_KEY_PRESSED, * P_DIALOG_RISCOS_EVENT_KEY_PRESSED;

/*
DIALOG_RISCOS_EVENT_CODE_POINTER_ENTER
*/

typedef struct DIALOG_RISCOS_EVENT_POINTER_ENTER
{
    H_DIALOG h_dialog; /*IN*/

    BOOL enter; /*IN*/
}
DIALOG_RISCOS_EVENT_POINTER_ENTER, * P_DIALOG_RISCOS_EVENT_POINTER_ENTER;

/*
DIALOG_RISCOS_EVENT_CODE_USER_DRAG
*/

typedef struct DIALOG_RISCOS_EVENT_USER_DRAG
{
    H_DIALOG h_dialog; /*IN*/

    WimpUserDragBoxEvent user_drag_box; /*IN*/
}
DIALOG_RISCOS_EVENT_USER_DRAG, * P_DIALOG_RISCOS_EVENT_USER_DRAG;

/*
DIALOG_RISCOS_EVENT_CODE_RESIZE
*/

typedef struct DIALOG_RISCOS_EVENT_RESIZE
{
    H_DIALOG h_dialog; /*IN*/

    GR_POINT size;     /*IN (host units, not pixits)*/
}
DIALOG_RISCOS_EVENT_RESIZE, * P_DIALOG_RISCOS_EVENT_RESIZE;

#endif /* __xp_dlgr_h */

#endif /* RISCOS */

/* end of xp_dlgr.h */
