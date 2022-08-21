/* xp_skeld.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __xp_skeld_h
#define __xp_skeld_h

#ifndef        __xp_dlg_h
#include "ob_dlg/xp_dlg.h"
#endif

/*
ui_data.c
*/

extern const DIALOG_CONTROL
dialog_main_group;

extern const DIALOG_CONTROL
dialog_col1_group;

extern const DIALOG_CONTROL
dialog_col2_group;

#if WINDOWS
#define ID_LEFT_OKCANCEL IDOK
#else
#define ID_LEFT_OKCANCEL IDCANCEL
#endif

extern const DIALOG_CONTROL
defbutton_close;

extern const DIALOG_CONTROL_DATA_PUSHBUTTON
defbutton_close_data;

extern const DIALOG_CONTROL
defbutton_ok;

extern const DIALOG_CONTROL_DATA_PUSHBUTTON
defbutton_ok_data;

extern const DIALOG_CONTROL_DATA_PUSHBUTTONR
defbutton_ok_persist_data;

extern const DIALOG_CONTROL
stdbutton_cancel;

extern const DIALOG_CONTROL_DATA_PUSHBUTTON
stdbutton_cancel_data;

extern const DIALOG_CONTROL_DATA_PUSHBUTTON
defbutton_apply_data;

extern const DIALOG_CONTROL_DATA_PUSHBUTTONR
defbutton_apply_persist_data;

extern const DIALOG_CONTROL_DATA_PUSHBUTTON
defbutton_create_data;

extern const DIALOG_CONTROL_DATA_PUSHBUTTON
defbutton_save_data;

extern const DIALOG_CONTROL_DATA_LIST_TEXT
stdlisttext_data;

extern const DIALOG_CONTROL_DATA_LIST_TEXT
stdlisttext_data_vscroll;

extern const DIALOG_CONTROL_DATA_LIST_TEXT
stdlisttext_data_dd;

extern const DIALOG_CONTROL_DATA_LIST_TEXT
stdlisttext_data_dd_vscroll;

extern const DIALOG_CONTROL_DATA_RADIOPICTURE
line_style_data[5];

#if RISCOS
#define LINE_STYLE_H (48 * 2 * PIXITS_PER_RISCOS + (4 + 2 + 8) * 2 * PIXITS_PER_RISCOS)
#define LINE_STYLE_V ( 9 * 4 * PIXITS_PER_RISCOS)
#elif WINDOWS
#define LINE_STYLE_H (32 * PIXITS_PER_WDU_H)
#define LINE_STYLE_V (14 * PIXITS_PER_WDU_V)
#endif

#define DIALOG_ID_RGB_GROUP       ((DIALOG_CONTROL_ID) 1024)
#define DIALOG_ID_RGB_GROUP_INNER ((DIALOG_CONTROL_ID) 1025)
#define DIALOG_ID_RGB_TX_R        ((DIALOG_CONTROL_ID) 1026)
#define DIALOG_ID_RGB_TX_G        ((DIALOG_CONTROL_ID) 1027)
#define DIALOG_ID_RGB_TX_B        ((DIALOG_CONTROL_ID) 1028)
#define DIALOG_ID_RGB_R           ((DIALOG_CONTROL_ID) 1029)
#define DIALOG_ID_RGB_G           ((DIALOG_CONTROL_ID) 1030)
#define DIALOG_ID_RGB_B           ((DIALOG_CONTROL_ID) 1031)
#define DIALOG_ID_RGB_PATCH       ((DIALOG_CONTROL_ID) 1032)
#define DIALOG_ID_RGB_BUTTON      ((DIALOG_CONTROL_ID) 1033)
#define DIALOG_ID_RGB_0           ((DIALOG_CONTROL_ID) 1034)
#define DIALOG_ID_RGB_1           ((DIALOG_CONTROL_ID) 1035)
#define DIALOG_ID_RGB_2           ((DIALOG_CONTROL_ID) 1036)
#define DIALOG_ID_RGB_3           ((DIALOG_CONTROL_ID) 1037)
#define DIALOG_ID_RGB_4           ((DIALOG_CONTROL_ID) 1038)
#define DIALOG_ID_RGB_5           ((DIALOG_CONTROL_ID) 1039)
#define DIALOG_ID_RGB_6           ((DIALOG_CONTROL_ID) 1040)
#define DIALOG_ID_RGB_7           ((DIALOG_CONTROL_ID) 1041)
#define DIALOG_ID_RGB_8           ((DIALOG_CONTROL_ID) 1042)
#define DIALOG_ID_RGB_9           ((DIALOG_CONTROL_ID) 1043)
#define DIALOG_ID_RGB_10          ((DIALOG_CONTROL_ID) 1044)
#define DIALOG_ID_RGB_11          ((DIALOG_CONTROL_ID) 1045)
#define DIALOG_ID_RGB_12          ((DIALOG_CONTROL_ID) 1046)
#define DIALOG_ID_RGB_13          ((DIALOG_CONTROL_ID) 1047)
#define DIALOG_ID_RGB_14          ((DIALOG_CONTROL_ID) 1048)
#define DIALOG_ID_RGB_15          ((DIALOG_CONTROL_ID) 1049)
#define DIALOG_ID_RGB_T           ((DIALOG_CONTROL_ID) 1050)

extern const DIALOG_CONTROL
rgb_group_inner;

extern const DIALOG_CONTROL /*TEXTLABEL*/
rgb_tx[3];

extern const DIALOG_CONTROL /*BUMP_S32*/
rgb_bump[3];

extern const DIALOG_CONTROL
rgb_patch;

extern const DIALOG_CONTROL
rgb_button;

extern const DIALOG_CONTROL
rgb_patches[16];

extern const DIALOG_CONTROL
rgb_transparent;

extern const DIALOG_CONTROL_DATA_GROUPBOX
rgb_group_data;

extern const DIALOG_CONTROL_DATA_TEXTLABEL
rgb_tx_data[3];
#define RGB_TX_IX_R 0
#define RGB_TX_IX_G 1
#define RGB_TX_IX_B 2

extern const DIALOG_CONTROL_DATA_BUMP_S32
rgb_bump_data;

extern const DIALOG_CONTROL_DATA_PUSHBUTTON
rgb_button_data;

extern const DIALOG_CONTROL_DATA_USER
rgb_patch_data;

extern const DIALOG_CONTROL_DATA_USER
rgb_patches_data[16];

extern const DIALOG_CONTROL_DATA_CHECKBOX
rgb_transparent_data;

extern const DIALOG_CONTROL_DATA_CHECKPICTURE
style_text_bold_data;

extern const DIALOG_CONTROL_DATA_CHECKPICTURE
style_text_italic_data;

extern const DIALOG_CONTROL_DATA_STATICTEXT
measurement_points_data;

#define RGB_FIELDS_H  DIALOG_BUMP_H(3)

#if RISCOS
#define RGB_TX_H      (28 * PIXITS_PER_RISCOS)

#define RGB_PATCHES_H (32 * PIXITS_PER_RISCOS) /* was 28 */
#define RGB_PATCHES_V (32 * PIXITS_PER_RISCOS)
#elif WINDOWS
#define RGB_TX_H      (8 * PIXITS_PER_WDU_H)

#define RGB_PATCHES_H (8 * PIXITS_PER_WDU_H)
#define RGB_PATCHES_V (8 * PIXITS_PER_WDU_V)
#endif

/*
exported routines
*/

#if defined(BIND_OB_DIALOG)
_Check_return_
extern STATUS
dialog_event( /* direct call */
    _InVal_     DIALOG_MESSAGE dialog_message,
    P_ANY p_data);
#endif

_Check_return_
static inline STATUS
object_call_DIALOG(
    _InVal_     DIALOG_MESSAGE dialog_message,
    /*_Inout_*/ P_ANY p_data)
{
#if defined(BIND_OB_DIALOG) /* direct call is possible */
    return(dialog_event(dialog_message, p_data));
#else /* must call via object interface with message */
    return(object_call_id(OBJECT_ID_DIALOG, P_DOCU_NONE, (const T5_MESSAGE) dialog_message, p_data));
#endif /* BIND_OB_TOOLBAR */
}

_Check_return_
static inline STATUS
object_call_DIALOG_with_docu(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     DIALOG_MESSAGE dialog_message,
    /*_Inout_*/ P_ANY p_data)
{
    return(object_call_id(OBJECT_ID_DIALOG, p_docu, (const T5_MESSAGE) dialog_message, p_data));
}

extern void
dialog_cmd_process_dbox_setup(
    _OutRef_    P_DIALOG_CMD_PROCESS_DBOX p_dialog_cmd_process_dbox,
    _In_reads_opt_(n_ctls) P_DIALOG_CTL_CREATE p_ctl_create,
    _InVal_     U32 n_ctls,
    _InVal_     STATUS caption_resource_id);

extern void
dialog_cmd_process_dbox_setup_ui_text(
    _OutRef_    P_DIALOG_CMD_PROCESS_DBOX p_dialog_cmd_process_dbox,
    _In_reads_opt_(n_ctls) P_DIALOG_CTL_CREATE p_ctl_create,
    _InVal_     U32 n_ctls,
    _InRef_     PC_UI_TEXT p_ui_text);

extern void
ui_dlg_ctl_enable(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     BOOL enabled);

extern void
ui_dlg_ctl_encode(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id);

extern void
ui_dlg_ctl_size_estimate(
    P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate);

_Check_return_
extern F64
ui_dlg_get_f64(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id);

_Check_return_
extern S32
ui_dlg_get_s32(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id);

_Check_return_
extern BOOL /*onoff*/
ui_dlg_get_check(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id);

extern void
ui_dlg_get_edit(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _OutRef_    P_UI_TEXT p_ui_text);

_Check_return_
extern S32 /*radio_state*/
ui_dlg_get_radio(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id);

_Check_return_
extern S32
ui_dlg_get_list_idx(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id);

extern void
ui_dlg_get_list_text(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _OutRef_    P_UI_TEXT p_ui_text);

extern void
ui_dlg_ctl_new_source(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id);

_Check_return_
extern STATUS
ui_dlg_set_f64(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     F64 f64);

_Check_return_
extern STATUS
ui_dlg_set_s32(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     S32 s32);

_Check_return_
extern STATUS
ui_dlg_set_check(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     BOOL onoff);

_Check_return_
extern STATUS
ui_dlg_set_check_forcing(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     BOOL onoff);

_Check_return_
extern STATUS
ui_dlg_set_edit(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InRef_     PC_UI_TEXT p_ui_text);

_Check_return_
extern STATUS
ui_dlg_set_list_idx(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     S32 itemno);

_Check_return_
extern STATUS
ui_dlg_set_radio(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     S32 radio_state);

_Check_return_
extern STATUS
ui_dlg_set_radio_forcing(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     S32 radio_state);

_Check_return_
extern STATUS
ui_dlg_ctl_set_default(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id);

_Check_return_
extern S32
ui_dlg_s32_from_f64(
    _InVal_     F64 f64,
    _InVal_     F64 f64_multiplier,
    _InVal_     S32 s32_min,
    _InVal_     S32 s32_max);

/*
choices
*/

typedef struct CHOICES_QUERY_BLOCK
{
    ARRAY_HANDLE ctl_create; /* [] DIALOG_CTL_CREATE */
    DIALOG_CONTROL_ID tr_dialog_control_id;
    DIALOG_CONTROL_ID br_dialog_control_id;
}
CHOICES_QUERY_BLOCK, * P_CHOICES_QUERY_BLOCK;

typedef struct CHOICES_SET_BLOCK
{
    H_DIALOG h_dialog;
}
CHOICES_SET_BLOCK, * P_CHOICES_SET_BLOCK;

#endif /* __xp_skeld_h */

/* end of xp_skeld.h */
