/* ui_styin.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __ui_styin_h
#define __ui_styin_h

#include "ob_skspt/xp_uisty.h"

#include "ob_skspt/ob_skspt.h"

#if RISCOS
#define ES_LIGHT_H         (20 * PIXITS_PER_RISCOS)
#define ES_LIGHT_V         (20 * PIXITS_PER_RISCOS)
#elif WINDOWS
#define ES_LIGHT_H         (6 * PIXITS_PER_WDU_H)
#define ES_LIGHT_V         (6 * PIXITS_PER_WDU_V)
#endif
#define ES_LIGHT_V_OFFSET  ((DIALOG_STDRADIO_V - ES_LIGHT_V) / 2)

#define ES_LIGHTMAINSPACING_H DIALOG_SMALLSPACING_H

#define ES_DIALOG_MAIN_H   (DIALOG_STDRADIO_H + DIALOG_RADIOGAP_H + DIALOG_FATCHAR_H + DIALOG_SYSCHARS_H("olours"))

#define ES_PRECTL_H        DIALOG_SYSCHAR_H

enum ES_CONTROL_IDS
{
    ES_ID_STT = 100,
    ES_ID_RHS_GROUP,
    ES_ID_SET,
    ES_ID_REVERT,
    ES_ID_ADJUST_APPLY,
#ifdef SDR
#define ES_SUBDIALOG_REVERT_H (DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARS_H("Revert"))
#endif

    ES_ID_CATEGORY_GROUP,

    ES_ID_RADIO_STT = 120,
    ES_ID_NAME = ES_ID_RADIO_STT,
    ES_ID_FS,
    ES_ID_PS2,
    ES_ID_PS1,
    ES_ID_PS3,
    ES_ID_PS4,
    ES_ID_RS,

    ES_ID_LIGHT_STT = 140,

    ES_NAME_ID_GROUP = 200,
    ES_NAME_ID_NAME,
    ES_NAME_ID_NAME_ENABLE,
#if 1
#define ES_NAME_NAME_H ES_KEY_LIST_H
#else
#define ES_NAME_NAME_H (DIALOG_STDEDITOVH_H + DIALOG_SYSCHARS_H("QuiteLongNamesTypedHere"))
#endif

    ES_KEY_ID_GROUP = 250,
    ES_KEY_ID_LIST,
    ES_KEY_ID_LIST_ENABLE,
#define ES_KEY_LIST_H (DIALOG_STDLISTOVH_H + DIALOG_SYSCHARS_H("Ctrl+Shift+F11"))
#define ES_KEY_LIST_V (DIALOG_STDLISTOVH_V + 4 * DIALOG_STDLISTITEM_V)

    ES_FS_ID_GROUP = 300,
    ES_FS_ID_TYPEFACE,
    ES_FS_ID_TYPEFACE_ENABLE,
    ES_FS_ID_HEIGHT,
    ES_FS_ID_HEIGHT_ENABLE,
    ES_FS_ID_HEIGHT_UNITS,
    ES_FS_ID_WIDTH,
    ES_FS_ID_WIDTH_ENABLE,
    ES_FS_ID_WIDTH_UNITS,
    ES_FS_ID_HW_CAPTION_GROUP,
#if RISCOS
#define ES_FS_TYPEFACE_H    (320 * PIXITS_PER_RISCOS)
#elif WINDOWS
#define ES_FS_TYPEFACE_V    (DIALOG_STDLISTOVH_V + 7 * DIALOG_STDLISTITEM_V)
#endif
#define ES_FS_SIZE_FIELDS_H DIALOG_BUMP_H(4)
#if RISCOS
#define ES_FS_BIU_SPACING_V DIALOG_SMALLSPACING_V
#else
#define ES_FS_BIU_SPACING_V DIALOG_STDSPACING_V
#endif

    ES_FS_ID_BOLD = 350,
    ES_FS_ID_BOLD_ENABLE,
    ES_FS_ID_ITALIC,
    ES_FS_ID_ITALIC_ENABLE,
    ES_FS_ID_UNDERLINE,
    ES_FS_ID_UNDERLINE_ENABLE,
    ES_FS_ID_SUPERSCRIPT,
    ES_FS_ID_SUPERSCRIPT_ENABLE,
    ES_FS_ID_SUBSCRIPT,
    ES_FS_ID_SUBSCRIPT_ENABLE,
#if RISCOS && 0
#define ES_FS_TYPE_H    ((44 * PIXITS_PER_RISCOS) + DIALOG_PUSHPICTUREOVH_H)
#define ES_FS_TYPE_V    ((44 * PIXITS_PER_RISCOS) + DIALOG_PUSHPICTUREOVH_V)
#else
#define ES_FS_TYPE_H    PIXITS_PER_STDTOOL_H
#define ES_FS_TYPE_V    PIXITS_PER_STDTOOL_V
#endif

    ES_FS_ID_COLOUR_GROUP_MAIN = 400,
    ES_FS_ID_COLOUR_GROUP,
    ES_FS_ID_COLOUR_GROUP_ENABLE,
    ES_FS_ID_COLOUR_TX_R,
    ES_FS_ID_COLOUR_TX_G,
    ES_FS_ID_COLOUR_TX_B,
    ES_FS_ID_COLOUR_R,
    ES_FS_ID_COLOUR_G,
    ES_FS_ID_COLOUR_B,
    ES_FS_ID_COLOUR_PATCH,
    ES_FS_ID_COLOUR_BUTTON,

    ES_FS_ID_COLOUR_0 = 420,
    ES_FS_ID_COLOUR_1,
    ES_FS_ID_COLOUR_2,
    ES_FS_ID_COLOUR_3,
    ES_FS_ID_COLOUR_4,
    ES_FS_ID_COLOUR_5,
    ES_FS_ID_COLOUR_6,
    ES_FS_ID_COLOUR_7,
    ES_FS_ID_COLOUR_8,
    ES_FS_ID_COLOUR_9,
    ES_FS_ID_COLOUR_10,
    ES_FS_ID_COLOUR_11,
    ES_FS_ID_COLOUR_12,
    ES_FS_ID_COLOUR_13,
    ES_FS_ID_COLOUR_14,
    ES_FS_ID_COLOUR_15,

    ES_PS_ID_NUMFORM_GROUP = 500,
    ES_PS_ID_NUMFORM_LIST_NU,
    ES_PS_ID_NUMFORM_LIST_NU_ENABLE,
    ES_PS_ID_NUMFORM_LIST_DT,
    ES_PS_ID_NUMFORM_LIST_DT_ENABLE,
    ES_PS_ID_NUMFORM_LIST_SE,
    ES_PS_ID_NUMFORM_LIST_SE_ENABLE,

#if WINDOWS
#define ES_PS_NUMFORM_LIST_H DIALOG_STDLISTOVH_H + DIALOG_SYSCHARSL_H(44)
#define ES_PS_NUMFORM_LIST_DROP_V (DIALOG_STDLISTOVH_V + 8 * DIALOG_STDLISTITEM_V)
#else
#define ES_PS_NUMFORM_LIST_H DIALOG_STDLISTOVH_H + DIALOG_SYSCHARSL_H(44)
#define ES_PS_NUMFORM_LIST_DROP_V (DIALOG_STDLISTOVH_V + 8 * DIALOG_STDLISTITEM_V)
#endif

    ES_PS_ID_NEW_OBJECT_GROUP = 600,
    ES_PS_ID_NEW_OBJECT_LIST,
    ES_PS_ID_NEW_OBJECT_LIST_ENABLE,
#if WINDOWS
#define ES_PS_NEW_OBJECT_LIST_H DIALOG_STDLISTOVH_H + DIALOG_SYSCHARSL_H(8)
#define ES_PS_NEW_OBJECT_LIST_DROP_V (DIALOG_STDLISTOVH_V + 3 * DIALOG_STDLISTITEM_V)
#else
#define ES_PS_NEW_OBJECT_LIST_H DIALOG_STDLISTOVH_H + DIALOG_SYSCHARSL_H(8)
#define ES_PS_NEW_OBJECT_LIST_DROP_V (DIALOG_STDLISTOVH_V + 3 * DIALOG_STDLISTITEM_V)
#endif

    ES_PS_ID_PROTECTION = 650,
    ES_PS_ID_PROTECTION_ENABLE,

    ES_PS_ID_BACK_GROUP = 700,
    ES_PS_ID_RGB_BACK_GROUP,
    ES_PS_ID_RGB_BACK_GROUP_ENABLE,
    ES_PS_ID_RGB_BACK_TX_R,
    ES_PS_ID_RGB_BACK_TX_G,
    ES_PS_ID_RGB_BACK_TX_B,
    ES_PS_ID_RGB_BACK_R,
    ES_PS_ID_RGB_BACK_G,
    ES_PS_ID_RGB_BACK_B,
    ES_PS_ID_RGB_BACK_PATCH,
    ES_PS_ID_RGB_BACK_BUTTON,
    ES_PS_ID_RGB_BACK_GROUP_INNER,
    ES_PS_ID_RGB_BACK_T,

    ES_PS_ID_RGB_BACK_0 = 720,
    ES_PS_ID_RGB_BACK_1,
    ES_PS_ID_RGB_BACK_2,
    ES_PS_ID_RGB_BACK_3,
    ES_PS_ID_RGB_BACK_4,
    ES_PS_ID_RGB_BACK_5,
    ES_PS_ID_RGB_BACK_6,
    ES_PS_ID_RGB_BACK_7,
    ES_PS_ID_RGB_BACK_8,
    ES_PS_ID_RGB_BACK_9,
    ES_PS_ID_RGB_BACK_10,
    ES_PS_ID_RGB_BACK_11,
    ES_PS_ID_RGB_BACK_12,
    ES_PS_ID_RGB_BACK_13,
    ES_PS_ID_RGB_BACK_14,
    ES_PS_ID_RGB_BACK_15,

    ES_PS_ID_BORDER_GROUP = 800,
    ES_PS_ID_BORDER_RGB_GROUP,
    ES_PS_ID_BORDER_RGB_GROUP_ENABLE,
    ES_PS_ID_BORDER_TX_R,
    ES_PS_ID_BORDER_TX_G,
    ES_PS_ID_BORDER_TX_B,
    ES_PS_ID_BORDER_R,
    ES_PS_ID_BORDER_G,
    ES_PS_ID_BORDER_B,
    ES_PS_ID_BORDER_PATCH,
    ES_PS_ID_BORDER_BUTTON,

    ES_PS_ID_BORDER_0 = 820,
    ES_PS_ID_BORDER_1,
    ES_PS_ID_BORDER_2,
    ES_PS_ID_BORDER_3,
    ES_PS_ID_BORDER_4,
    ES_PS_ID_BORDER_5,
    ES_PS_ID_BORDER_6,
    ES_PS_ID_BORDER_7,
    ES_PS_ID_BORDER_8,
    ES_PS_ID_BORDER_9,
    ES_PS_ID_BORDER_10,
    ES_PS_ID_BORDER_11,
    ES_PS_ID_BORDER_12,
    ES_PS_ID_BORDER_13,
    ES_PS_ID_BORDER_14,
    ES_PS_ID_BORDER_15,
#define ES_PS_BORDER_ENABLE_H     (7 * PIXITS_PER_INCH / 8)

    ES_PS_ID_BORDER_LINE_GROUP = 850,
    ES_PS_ID_BORDER_LINE_GROUP_ENABLE,
    ES_PS_ID_BORDER_LINE_0,
    ES_PS_ID_BORDER_LINE_1,
    ES_PS_ID_BORDER_LINE_2,
    ES_PS_ID_BORDER_LINE_3,
    ES_PS_ID_BORDER_LINE_4,
#define ES_PS_BORDER_LINE_H LINE_STYLE_H
#define ES_PS_BORDER_LINE_V LINE_STYLE_V

    ES_PS_ID_GRID_GROUP = 900,
    ES_PS_ID_GRID_RGB_GROUP,
    ES_PS_ID_GRID_RGB_GROUP_ENABLE,
    ES_PS_ID_GRID_TX_R,
    ES_PS_ID_GRID_TX_G,
    ES_PS_ID_GRID_TX_B,
    ES_PS_ID_GRID_R,
    ES_PS_ID_GRID_G,
    ES_PS_ID_GRID_B,
    ES_PS_ID_GRID_PATCH,
    ES_PS_ID_GRID_BUTTON,

    ES_PS_ID_GRID_0 = 920,
    ES_PS_ID_GRID_1,
    ES_PS_ID_GRID_2,
    ES_PS_ID_GRID_3,
    ES_PS_ID_GRID_4,
    ES_PS_ID_GRID_5,
    ES_PS_ID_GRID_6,
    ES_PS_ID_GRID_7,
    ES_PS_ID_GRID_8,
    ES_PS_ID_GRID_9,
    ES_PS_ID_GRID_10,
    ES_PS_ID_GRID_11,
    ES_PS_ID_GRID_12,
    ES_PS_ID_GRID_13,
    ES_PS_ID_GRID_14,
    ES_PS_ID_GRID_15,
#define ES_PS_GRID_ENABLE_H ES_PS_BORDER_ENABLE_H

    ES_PS_ID_GRID_LINE_GROUP = 950,
    ES_PS_ID_GRID_LINE_GROUP_ENABLE,
    ES_PS_ID_GRID_LINE_0,
    ES_PS_ID_GRID_LINE_1,
    ES_PS_ID_GRID_LINE_2,
    ES_PS_ID_GRID_LINE_3,
    ES_PS_ID_GRID_LINE_4,
#define ES_PS_GRID_LINE_H ES_PS_BORDER_LINE_H
#define ES_PS_GRID_LINE_V ES_PS_BORDER_LINE_V

    ES_PS_ID_MARGINS_GROUP = 1000,
    ES_PS_ID_MARGIN_LEFT,
    ES_PS_ID_MARGIN_LEFT_ENABLE,
    ES_PS_ID_MARGIN_LEFT_UNITS,
    ES_PS_ID_MARGIN_PARA,
    ES_PS_ID_MARGIN_PARA_ENABLE,
    ES_PS_ID_MARGIN_PARA_UNITS,
    ES_PS_ID_MARGIN_RIGHT,
    ES_PS_ID_MARGIN_RIGHT_ENABLE,
    ES_PS_ID_MARGIN_RIGHT_UNITS,
    ES_PS_ID_MARGINS_CAPTION_GROUP,
#define ES_PS_MARGIN_FIELDS_H DIALOG_BUMP_H(6)

    ES_PS_ID_TAB_LIST_GROUP = 1050,
    ES_PS_ID_TAB_LIST,
    ES_PS_ID_TAB_LIST_ENABLE,
    ES_PS_ID_TAB_LIST_UNITS,

    ES_PS_ID_HORZ_JUSTIFY_GROUP_MAIN = 1100,
    ES_PS_ID_HORZ_JUSTIFY_GROUP,
    ES_PS_ID_HORZ_JUSTIFY_GROUP_ENABLE,
    ES_PS_ID_HORZ_JUSTIFY_LEFT,
    ES_PS_ID_HORZ_JUSTIFY_CENTRE,
    ES_PS_ID_HORZ_JUSTIFY_RIGHT,
    ES_PS_ID_HORZ_JUSTIFY_BOTH,
#if RISCOS && 0
#define ES_PS_JUSTIFY_TYPE_H    ((44 * PIXITS_PER_RISCOS) + DIALOG_PUSHPICTUREOVH_H)
#define ES_PS_JUSTIFY_TYPE_V    ((44 * PIXITS_PER_RISCOS) + DIALOG_PUSHPICTUREOVH_V)
#else
#define ES_PS_JUSTIFY_TYPE_H    PIXITS_PER_STDTOOL_H
#define ES_PS_JUSTIFY_TYPE_V    PIXITS_PER_STDTOOL_V
#endif

    ES_PS_ID_VERT_JUSTIFY_GROUP_MAIN = 1150,
    ES_PS_ID_VERT_JUSTIFY_GROUP,
    ES_PS_ID_VERT_JUSTIFY_GROUP_ENABLE,
    ES_PS_ID_VERT_JUSTIFY_TOP,
    ES_PS_ID_VERT_JUSTIFY_CENTRE,
    ES_PS_ID_VERT_JUSTIFY_BOTTOM,

    ES_PS_ID_PARA_SPACE_GROUP = 1200,
    ES_PS_ID_PARA_SPACE_CAPTION_GROUP,
    ES_PS_ID_PARA_START,
    ES_PS_ID_PARA_START_ENABLE,
    ES_PS_ID_PARA_START_UNITS,
    ES_PS_ID_PARA_END,
    ES_PS_ID_PARA_END_ENABLE,
    ES_PS_ID_PARA_END_UNITS,
#define ES_PS_SPACING_FIELDS_H DIALOG_BUMP_H(6)

    ES_PS_ID_LINE_SPACE_GROUP_MAIN = 1300,
    ES_PS_ID_LINE_SPACE_GROUP,
    ES_PS_ID_LINE_SPACE_GROUP_ENABLE,
    ES_PS_ID_LINE_SPACE_SINGLE,
    ES_PS_ID_LINE_SPACE_ONEP5,
    ES_PS_ID_LINE_SPACE_DOUBLE,
    ES_PS_ID_LINE_SPACE_N,
    ES_PS_ID_LINE_SPACE_N_VAL,
    ES_PS_ID_LINE_SPACE_N_VAL_UNITS,
#if RISCOS && 0
#define ES_PS_LINE_SPACE_TYPE_H ((44 * PIXITS_PER_RISCOS) + DIALOG_PUSHPICTUREOVH_H)
#define ES_PS_LINE_SPACE_TYPE_V ((44 * PIXITS_PER_RISCOS) + DIALOG_PUSHPICTUREOVH_V)
#else
#define ES_PS_LINE_SPACE_TYPE_H PIXITS_PER_STDTOOL_H
#define ES_PS_LINE_SPACE_TYPE_V PIXITS_PER_STDTOOL_V
#endif
#define ES_PS_LINE_SPACE_FIELDS_H DIALOG_BUMP_H(5)

    ES_RS_ID_GROUP = 1400,
    ES_RS_ID_CAPTION_GROUP,
    ES_RS_ID_HEIGHT,
    ES_RS_ID_HEIGHT_ENABLE,
    ES_RS_ID_HEIGHT_UNITS,
    ES_RS_ID_HEIGHT_FIXED,
    ES_RS_ID_HEIGHT_FIXED_ENABLE,
    ES_RS_ID_UNBREAKABLE,
    ES_RS_ID_UNBREAKABLE_ENABLE,
    ES_RS_ID_ROW_NAME,
    ES_RS_ID_ROW_NAME_ENABLE,
#define ES_RS_FIELDS_H DIALOG_BUMP_H(6)
#define ES_RS_NAME_H   (DIALOG_STDEDITOVH_H + DIALOG_SYSCHARS_H("QuiteLongNamesTypedHere"))

    ES_CS_ID_GROUP = 1500,
    ES_CS_ID_WIDTH,
    ES_CS_ID_WIDTH_ENABLE,
    ES_CS_ID_WIDTH_UNITS,
    ES_CS_ID_COL_NAME,
    ES_CS_ID_COL_NAME_ENABLE,
    ES_CS_ID_COL_CAPTION_GROUP,
#define ES_CS_FIELDS_H DIALOG_BUMP_H(6)
#define ES_CS_NAME_H   (DIALOG_STDEDITOVH_H + DIALOG_SYSCHARS_H("QuiteLongNamesTypedHere"))

    ES_ID_MAX
};

#if RISCOS
#define P_ES_STDENABLE_ON  NULL
#define P_ES_STDENABLE_OFF NULL
#endif

#if RISCOS && 0
#define ES_FS_ID_COLOURS_RELATIVE   ES_FS_ID_COLOUR_PATCH
#define ES_PS_ID_BORDERS_RELATIVE   ES_PS_ID_BORDER_PATCH
#define ES_PS_ID_GRIDS_RELATIVE     ES_PS_ID_GRID_PATCH
#define ES_PS_ID_RGB_BACKS_RELATIVE ES_PS_ID_RGB_BACK_PATCH
#else
#define ES_FS_ID_COLOURS_RELATIVE   ES_FS_ID_COLOUR_BUTTON
#define ES_PS_ID_BORDERS_RELATIVE   ES_PS_ID_BORDER_BUTTON
#define ES_PS_ID_GRIDS_RELATIVE     ES_PS_ID_GRID_BUTTON
#define ES_PS_ID_RGB_BACKS_RELATIVE ES_PS_ID_RGB_BACK_BUTTON
#endif

/*
Common variables
*/

/* these MUST correspond to the es_subdialogs table in ui_styl2 */

#define ES_SUBDIALOG_NAME 0
#define ES_SUBDIALOG_FS   1
#define ES_SUBDIALOG_PS2  2
#define ES_SUBDIALOG_PS1  3
#define ES_SUBDIALOG_PS3  4
#define ES_SUBDIALOG_PS4  5
#define ES_SUBDIALOG_RS   6
#define ES_SUBDIALOG_MAX  7

extern S32
es_subdialog_current;

typedef struct ES_USER_UNIT_INFO_NF
{
    F64 user_unit_multiple;
    UCHARZ user_unit_numform_ustr_buf[32];
}
ES_USER_UNIT_INFO_NF;

typedef struct ES_USER_UNIT_INFO
{
    FP_PIXIT fp_pixits_per_user_unit;
    STATUS user_unit_resource_id;
    ES_USER_UNIT_INFO_NF normal, fine;
}
ES_USER_UNIT_INFO, * P_ES_USER_UNIT_INFO;

/*
main edit style callback
*/

typedef struct ES_CALLBACK
{
    HOST_WND hwnd_parent;
    UI_TEXT caption;
    S32 subdialog_current;

    BOOL creating;
    BOOL atx;
    BOOL num;
    BOOL has_colour_picker;

    STYLE_HANDLE style_handle_being_modified;

#define IDX_HORZ 0
#define IDX_VERT 1
    ES_USER_UNIT_INFO info[2];

    /*
    style in this subdialog edit
    */

    STYLE                 style;
    STYLE_SELECTOR style_selector;

    STYLE_SELECTOR style_modified;          /* bits of style that have been modified in this subdialog edit */
    STYLE_SELECTOR style_selector_modified; /* bits of style selector that have been modified in this subdialog edit */

    /*
    style committed so far this full edit
    */

    STYLE                 committed_style;
    STYLE_SELECTOR committed_style_selector;

    STYLE_SELECTOR committed_style_modified;          /* bits of committed style that have been modified in this edit */
    STYLE_SELECTOR committed_style_selector_modified; /* bits of committed style selector that have been modified in this edit */

    STYLE_SELECTOR prohibited_enabler; /* enable bits which may not be modified */
    STYLE_SELECTOR prohibited_enabler_2; /* enable bits which may not be modified */
}
ES_CALLBACK, * P_ES_CALLBACK; typedef const ES_CALLBACK * PC_ES_CALLBACK;

/*
ui_remov.c
*/

T5_CMD_PROTO(extern, t5_cmd_style_region_edit);
T5_CMD_PROTO(extern, t5_cmd_box_intro);

T5_MSG_PROTO(extern, t5_msg_box_apply, P_BOX_APPLY p_box_apply);

/*
internally exported routines from ui_style.c
*/

T5_CMD_PROTO(extern, t5_cmd_effects_button);
T5_CMD_PROTO(extern, t5_cmd_style_intro);

T5_MSG_PROTO(extern, t5_msg_uistyle_colour_picker, P_MSG_UISTYLE_COLOUR_PICKER p_msg_uistyle_colour_picker);
T5_MSG_PROTO(extern, t5_msg_uistyle_style_edit, P_MSG_UISTYLE_STYLE_EDIT p_msg_uistyle_style_edit);

_Check_return_
extern STATUS
ui_style_msg_startup(void);

/*
internally exported routine from ui_styl2.c
*/

_Check_return_
extern STATUS
es_process(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     HOST_WND hwnd_parent,
    _InRef_     PC_UI_TEXT p_caption,
    _InRef_     PC_STYLE p_style_in,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _OutRef_opt_ P_STYLE_SELECTOR p_style_modified,
    _OutRef_opt_ P_STYLE_SELECTOR p_style_selector_modified,
    _InRef_opt_ PC_STYLE_SELECTOR p_prohibited_enabler,
    _InRef_opt_ PC_STYLE_SELECTOR p_prohibited_enabler_2,
    _OutRef_    P_STYLE p_style_out,
    _InVal_     STYLE_HANDLE style_handle_being_modified /*ie not effects*/,
    _InVal_     S32 subdialog /*-1 -> whichever was last */);

/*
internally exported routines from ui_styl3.c
*/

_Check_return_
extern BOOL
es_subdialog_style_selector_test(
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InVal_     U32 subdialog);

extern void
es_tweak_style_init(
    P_ES_CALLBACK p_es_callback);

extern void
es_tweak_style_precreate(
    P_ES_CALLBACK p_es_callback,
    P_DIALOG_CMD_PROCESS_DBOX p_dialog_cmd_process_dbox);

PROC_DIALOG_EVENT_PROTO(extern, dialog_event_tweak_style);

#endif /* __ui_styin_h */

/* end of ui_styin.h */
