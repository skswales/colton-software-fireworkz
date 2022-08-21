/* colourpick.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2021 Stuart Swales */

#ifndef __colourpick_h
#define __colourpick_h

enum COLOUR_PICKER_TYPE
{
    COLOUR_PICKER_NORMAL  = 0,
    COLOUR_PICKER_MENU    = 1,
    COLOUR_PICKER_TOOLBOX = 2,
    COLOUR_PICKER_SUBMENU = 3
};

/*
exported functions
*/

extern BOOL
riscos_colour_picker(
    _InVal_     enum COLOUR_PICKER_TYPE colour_picker_type,
    _HwndRef_   HOST_WND parent_window_handle,
    _InoutRef_  P_RGB p_rgb,
    _InVal_     BOOL allow_transparent,
    _In_z_      PC_U8Z p_title);

#endif /* __colourpick_h */

/* end of colourpick.h */
