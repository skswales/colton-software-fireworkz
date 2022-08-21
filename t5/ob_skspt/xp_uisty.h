/* xp_uisty.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Style editing UI exports */

#ifndef __xp_uisty_h
#define __xp_uisty_h

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

typedef struct MSG_UISTYLE_COLOUR_PICKER
{
    H_DIALOG h_dialog;
    DIALOG_CONTROL_ID rgb_dialog_control_id; /* assumes this control id has a U32 == RGB state to set/get */
    DIALOG_CONTROL_ID button_dialog_control_id; /* this one is disabled during colour selection as a trivial interlock */
}
MSG_UISTYLE_COLOUR_PICKER, * P_MSG_UISTYLE_COLOUR_PICKER;

typedef struct MSG_UISTYLE_STYLE_EDIT
{
    HOST_WND hwnd_parent;
    PC_UI_TEXT p_caption;
    PC_STYLE p_style_in /*_In_*/;
    PC_STYLE_SELECTOR p_style_selector /*_In_*/;
    P_STYLE_SELECTOR p_style_modified /*_Out_opt_*/;
    P_STYLE_SELECTOR p_style_selector_modified /*_Out_opt_*/;
    PC_STYLE_SELECTOR p_prohibited_enabler /*_In_opt_*/;
    PC_STYLE_SELECTOR p_prohibited_enabler_2 /*_In_opt_*/;
    P_STYLE p_style_out /*_Out_*/;
    STYLE_HANDLE style_handle_being_modified /*i.e. not effects*/;
    S32 subdialog;
}
MSG_UISTYLE_STYLE_EDIT, * P_MSG_UISTYLE_STYLE_EDIT;

#endif /* __xp_uisty_h */

/* end of xp_uisty.h */
