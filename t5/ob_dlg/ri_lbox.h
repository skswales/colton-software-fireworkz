/* ri_lbox.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* List box for RISC OS header */

/* SKS March 1992 */

#ifndef __ri_lbox_h
#define __ri_lbox_h

/*
a handle to a list box control for our clients (and ourselves if the truth be known)
*/

typedef U32 RI_LBOX_HANDLE; typedef RI_LBOX_HANDLE * P_RI_LBOX_HANDLE;
#define RI_LBOX_HANDLE_NONE ((RI_LBOX_HANDLE) 0)

#define RI_LBOX_ITEM_LM     8

/*
height in OS units of each item
*/

#define RI_LBOX_ITEM_TM     4
#define RI_LBOX_ITEM_BM     4
#define RI_LBOX_ITEM_HEIGHT (RI_LBOX_ITEM_TM + 32 + RI_LBOX_ITEM_BM)

/*
structure to pass to ri_lbox_new()
*/

typedef struct RI_LBOX_NEW_BITS
{
    UBF pane                  : 1; /*1->set pane bit*/
    UBF force_v_scroll        : 1;
    UBF dont_shrink_to_fit    : 1;
    UBF allow_non_multiple    : 1;
    UBF force_h_scroll        : 1;
    UBF always_show_selection : 1;
    UBF trespass              : 1;
    UBF disable_double        : 1;
    UBF disable_frame         : 1;
}
RI_LBOX_NEW_BITS;

typedef struct RI_LBOX_NEW
{
    BBox bbox /*abs coords*/;

    PC_UI_TEXT caption /*NULL->none*/;

    RI_LBOX_NEW_BITS bits;

    UI_DATA_TYPE ui_data_type;
    const void /*UI_CONTROL*/ * p_ui_control;
    PC_UI_SOURCE p_ui_source;

    P_DOCU       p_docu;

    P_PROC_DIALOG_EVENT p_proc_dialog_event;
    CLIENT_HANDLE client_handle;
}
RI_LBOX_NEW, * P_RI_LBOX_NEW;

typedef struct DIALOG_MSG_LBN_DESTROY
{
    CLIENT_HANDLE  client_handle;
    RI_LBOX_HANDLE lbox_handle;

    BOOL processed; /*OUT*/
}
DIALOG_MSG_LBN_DESTROY, * P_DIALOG_MSG_LBN_DESTROY;

typedef struct DIALOG_MSG_LBN_FOCUS
{
    CLIENT_HANDLE  client_handle;
    RI_LBOX_HANDLE lbox_handle;
}
DIALOG_MSG_LBN_FOCUS, * P_DIALOG_MSG_LBN_FOCUS;

typedef struct DIALOG_MSG_LBN_CLK
{
    CLIENT_HANDLE  client_handle;
    RI_LBOX_HANDLE lbox_handle;
    S32            selected_item;
}
DIALOG_MSG_LBN_CLK, * P_DIALOG_MSG_LBN_CLK;

typedef struct DIALOG_MSG_LBN_KEY
{
    CLIENT_HANDLE  client_handle;
    RI_LBOX_HANDLE lbox_handle;
    S32            selected_item;
    KMAP_CODE      kmap_code;
    S32            processed;
}
DIALOG_MSG_LBN_KEY, * P_DIALOG_MSG_LBN_KEY;

#define RI_LBOX_SUBMENU_MENU            0
#define RI_LBOX_SUBMENU_SUBMENU         1
#define RI_LBOX_SUBMENU_OF_COMPLEX_MENU 2

#define RI_LBOX_SELECTION_NONE -1

/*
exported routines
*/

_Check_return_
extern STATUS
ri_lbox_dispose(
    _InoutRef_  P_RI_LBOX_HANDLE p_handle);

extern void
ri_lbox_enable(
    _InVal_     RI_LBOX_HANDLE handle,
    _InVal_     BOOL enable);

_Check_return_
extern STATUS
ri_lbox_focus_query(
    _InVal_     RI_LBOX_HANDLE handle);

_Check_return_
extern STATUS
ri_lbox_focus_set(
    _InVal_     RI_LBOX_HANDLE handle);

extern HOST_WND
ri_lbox_get_host_handle(
    _InVal_     RI_LBOX_HANDLE handle);

_Check_return_
extern STATUS
ri_lbox_hide(
    _InVal_     RI_LBOX_HANDLE handle);

_Check_return_
extern STATUS
ri_lbox_make_child(
    _InVal_     RI_LBOX_HANDLE handle,
    _InVal_     wimp_w parent_window_handle,
    _InVal_     wimp_i parent_icon_handle);

_Check_return_
extern STATUS
ri_lbox_new(
    _OutRef_    P_RI_LBOX_HANDLE handle,
    /*const*/ P_RI_LBOX_NEW p_ri_lbox_new);

_Check_return_
extern S32
ri_lbox_selection_get(
    _InVal_     RI_LBOX_HANDLE handle);

/*ncr*/
extern S32
ri_lbox_selection_set_from_itemno(
    _InVal_     RI_LBOX_HANDLE handle,
    _In_        S32 selected_item);

/*ncr*/
extern S32
ri_lbox_selection_set_from_tstr(
    _InVal_     RI_LBOX_HANDLE handle,
    _In_z_      PCTSTR tstr);

_Check_return_
extern STATUS
ri_lbox_show(
    _InVal_     RI_LBOX_HANDLE handle);

_Check_return_
extern STATUS
ri_lbox_show_submenu(
    _InVal_     RI_LBOX_HANDLE handle,
    _InVal_     S32 submenu /*0->main menu*/,
    _InRef_opt_ PC_GDI_POINT p_tl /*NULL->lookup*/);

_Check_return_
extern STATUS
ri_lbox_source_modified(
    _InVal_     RI_LBOX_HANDLE handle);

#endif /* __ri_lbox_h */

/* end of ri_lbox.h */
