/* ui_styl2.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* UI data for style editing for Fireworkz */

/* SKS September 1992 */

#include "common/gflags.h"

#include "ob_skspt/ui_styin.h"

#include "ob_skel/resource/resource.h"

#include "ob_toolb/xp_toolb.h"

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

/*
internal structure
*/

/* these are bits */
#define ES_ALW 0 /* control always present */
#define ES_ATX 1 /* control only present if 'advanced text' */
#define ES_NUM 2 /* control only present if 'numbers' */
#define ES_spare_4 4
#define ES_NRO 8 /* control only absent if RISC OS and no colour picker */

typedef struct ES_DIALOG_CTL_CREATE
{
    UINT flags;
    DIALOG_CTL_CREATE dialog_ctl_create;
}
ES_DIALOG_CTL_CREATE; typedef const struct ES_DIALOG_CTL_CREATE * PC_ES_DIALOG_CTL_CREATE;

/*
internal routines
*/

_Check_return_
static STATUS
es_subdialog_process(
    _DocuRef_   P_DOCU p_docu,
    P_ES_CALLBACK p_es_callback);

/*
common control data
*/

static const DIALOG_CONTROL_DATA_CHECKBOX
es_tx_t_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_RGB_TX_T) };

static UCHARZ
es_points_numform_ustr_buf[elemof32("0.,##")];

static const UI_CONTROL_F64
es_points_control = { 0.0, 1000.0, 1.0, (P_USTR) es_points_numform_ustr_buf, 1.0 };

static const UI_CONTROL_F64
es_fs_height_points_control = { 1.0, 1000.0, 1.0, (P_USTR) es_points_numform_ustr_buf, 1.0 };

static /*poked*/ UI_CONTROL_F64
es_h_units_control;

static /*poked*/ UI_CONTROL_F64
es_ps_margin_para_control; /* SKS 11jul96 this needs to be a separate entity as it is poked specifically for margin_para */

static /*poked*/ UI_CONTROL_F64
es_v_units_control;

static /*poked*/ UI_CONTROL_F64
es_v_units_fine_control;

/*
enablers that can't move focus
*/

static const DIALOG_CONTROL_DATA_CHECKBOX
es_checkbox_data = { { 0 }, { UI_TEXT_TYPE_NONE } };

#define es_fs_bold_enable_data               es_checkbox_data
#define es_fs_italic_enable_data             es_checkbox_data
#define es_fs_underline_enable_data          es_checkbox_data
#define es_fs_superscript_enable_data        es_checkbox_data
#define es_fs_subscript_enable_data          es_checkbox_data
#define es_fs_colour_group_enable_data       es_checkbox_data
#define es_ps_rgb_back_group_enable_data     es_checkbox_data
#define es_ps_border_rgb_group_enable_data   es_checkbox_data
#define es_ps_grid_rgb_group_enable_data     es_checkbox_data
#define es_ps_horz_justify_group_enable_data es_checkbox_data
#define es_ps_vert_justify_group_enable_data es_checkbox_data
#define es_ps_line_space_group_enable_data   es_checkbox_data
#define es_ps_new_object_list_enable_data    es_checkbox_data

/*
these and most of the above ought to be tristates
*/

#define es_rs_height_fixed_data es_checkbox_data
#define es_rs_unbreakable_data  es_checkbox_data
#define es_ps_protection_data   es_checkbox_data

static const DIALOG_CONTROL
es_rhs_group =
{
    ES_ID_RHS_GROUP, DIALOG_CONTROL_WINDOW,
    { ES_ID_CATEGORY_GROUP, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H },
    { DRT(RTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
es_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
#if WINDOWS /* OK is to the left of Cancel */
    { DIALOG_CONTROL_SELF, ES_ID_RHS_GROUP, IDCANCEL, DIALOG_CONTROL_SELF },
    { DIALOG_DEFOK_H, DIALOG_STDSPACING_V, DIALOG_STDSPACING_H, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBLT, PUSHBUTTON), 1 /*tabstop*/, 1 /*logical_group*/ }
#else /* OK is the bottom rightmost button */
    { DIALOG_CONTROL_SELF, ES_ID_RHS_GROUP, ES_ID_RHS_GROUP, DIALOG_CONTROL_SELF },
    { DIALOG_CONTENTS_CALC, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 /*tabstop*/, 1 /*logical_group*/ }
#endif
};

#if WINDOWS /* Cancel is the bottom rightmost button */

const DIALOG_CONTROL
es_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, IDOK, ES_ID_RHS_GROUP, IDOK },
    { DIALOG_STDCANCEL_H, DIALOG_DEFPUSHEXTRA_V, 0, -DIALOG_DEFPUSHEXTRA_V },
    { DRT(RTRB, PUSHBUTTON), 1 /*tabstop*/ }
};

#else /* Cancel is to the left of OK */
#define es_cancel stdbutton_cancel
#endif

static const DIALOG_CONTROL_DATA_PUSHBUTTONR
es_ok_data =
{
#ifndef STYLE_DONT_ADJUST_APPLY
    { DIALOG_COMPLETION_OK, 0, 0, 1 /*alternate_right*/ }, UI_TEXT_INIT_RESID(MSG_BUTTON_APPLY), ES_ID_ADJUST_APPLY
#else
    { DIALOG_COMPLETION_OK, 0, 0, 1 /*alternate_right*/ }, UI_TEXT_INIT_RESID(MSG_BUTTON_APPLY), ES_ID_SET
#endif
};

static const DIALOG_CONTROL_DATA_PUSHBUTTONR
es_cancel_data = { { DIALOG_COMPLETION_CANCEL, 0, 0, 1 /*alternate_right*/ }, UI_TEXT_INIT_RESID(MSG_BUTTON_CANCEL), ES_ID_REVERT };

/******************************************************************************
*
* style name
*
******************************************************************************/

static const DIALOG_CONTROL
es_name_group =
{
    ES_NAME_ID_GROUP, ES_ID_RHS_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, ES_NAME_ID_NAME, ES_NAME_ID_NAME },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_name_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_STYLE_NAME), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
es_name_edit_enable =
{
    ES_NAME_ID_NAME_ENABLE, ES_NAME_ID_GROUP,
    { DIALOG_CONTROL_PARENT, ES_NAME_ID_NAME, DIALOG_CONTROL_SELF, ES_NAME_ID_NAME },
    { DIALOG_STDGROUP_LM, 0, DIALOG_STDCHECK_H },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_name_edit_enable_data = { { { 0, 1 /*move_focus*/ }, { UI_TEXT_TYPE_NONE } }, ES_NAME_ID_NAME };

static const DIALOG_CONTROL
es_name_edit =
{
    ES_NAME_ID_NAME, ES_NAME_ID_GROUP,
    { ES_NAME_ID_NAME_ENABLE, DIALOG_CONTROL_PARENT },
    { DIALOG_STDSPACING_H, DIALOG_STDGROUP_TM, ES_NAME_NAME_H, DIALOG_STDEDIT_V },
    { DRT(RTLT, EDIT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_EDIT
es_name_edit_data = { { { FRAMED_BOX_EDIT } }, /*EDIT_XX*/ { UI_TEXT_TYPE_NONE } /* UI_TEXT state */ };

/*
key list 'buddies'
*/

static const DIALOG_CONTROL
es_key_group =
{
    ES_KEY_ID_GROUP, ES_ID_RHS_GROUP,
    { ES_NAME_ID_GROUP, ES_NAME_ID_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_key_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_STYLE_KEY), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
es_key_list_enable =
{
    ES_KEY_ID_LIST_ENABLE, ES_KEY_ID_GROUP,
    { DIALOG_CONTROL_PARENT, ES_KEY_ID_LIST, DIALOG_CONTROL_SELF, ES_KEY_ID_LIST },
    { DIALOG_STDGROUP_LM, 0, DIALOG_STDCHECK_H, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_key_list_enable_data = { { { 0, 1 /*move_focus*/ }, { UI_TEXT_TYPE_NONE } }, ES_KEY_ID_LIST };

static const DIALOG_CONTROL
es_key_list =
{
    ES_KEY_ID_LIST, ES_KEY_ID_GROUP,
    { ES_KEY_ID_LIST_ENABLE, DIALOG_CONTROL_PARENT },
    { DIALOG_STDSPACING_H, DIALOG_STDGROUP_TM, ES_KEY_LIST_H, ES_KEY_LIST_V },
    { DRT(RTLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

/*
ok
*/

#define es_name_ok es_ok

static const ES_DIALOG_CTL_CREATE
es_name_ctl_create[] =
{
    { ES_ALW, { { &es_name_group }, &es_name_group_data } },
    { ES_ALW, { { &es_name_edit }, &es_name_edit_data } },
    { ES_ALW, { { &es_name_edit_enable }, &es_name_edit_enable_data } },
    { ES_ALW, { { &es_key_group }, &es_key_group_data } },
    { ES_ALW, { { &es_key_list }, &stdlisttext_data } },
    { ES_ALW, { { &es_key_list_enable }, &es_key_list_enable_data } },

    { ES_ALW, { { &es_name_ok }, &es_ok_data } }
};

/******************************************************************************
*
* character style - fonts, attributes, colour
*
******************************************************************************/

static const DIALOG_CONTROL
es_fs_group =
{
    ES_FS_ID_GROUP, ES_ID_RHS_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_fs_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_FS), { FRAMED_BOX_GROUP } };

/*
typeface 'buddies'
*/

static const DIALOG_CONTROL
es_fs_typeface_enable =
{
    ES_FS_ID_TYPEFACE_ENABLE, ES_FS_ID_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, ES_FS_ID_TYPEFACE },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, 0, DIALOG_STDCHECK_V },
    { DRT(LTRT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_fs_typeface_enable_data = { { { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_FS_TYPEFACE) }, ES_FS_ID_TYPEFACE };

static const DIALOG_CONTROL
es_fs_typeface_list =
{
    ES_FS_ID_TYPEFACE, ES_FS_ID_GROUP,
#if RISCOS && 1
    { ES_FS_ID_TYPEFACE_ENABLE, ES_FS_ID_TYPEFACE_ENABLE, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, ES_FS_TYPEFACE_H, ES_FS_TYPEFACE_V }, /* just drops below subscript button */
    { DRT(LBLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
#elif RISCOS
    { ES_FS_ID_TYPEFACE_ENABLE, ES_FS_ID_TYPEFACE_ENABLE, DIALOG_CONTROL_SELF, ES_FS_ID_SUBSCRIPT },
    { 0, DIALOG_STDSPACING_V, ES_FS_TYPEFACE_H }, /* <<< unused ES_FS_TYPEFACE_V */
    { DRT(LBLB, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
#else
    { ES_FS_ID_TYPEFACE_ENABLE, ES_FS_ID_TYPEFACE_ENABLE, ES_FS_ID_HEIGHT_UNITS, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, ES_FS_TYPEFACE_V },
    { DRT(LBRT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
#endif
};

static const DIALOG_CONTROL
es_fs_hw_caption_group =
{
    ES_FS_ID_HW_CAPTION_GROUP, ES_FS_ID_GROUP,
    { ES_FS_ID_TYPEFACE, ES_FS_ID_HEIGHT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0 },
    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

/*
height 'buddies'
*/

static const DIALOG_CONTROL
es_fs_height_enable =
{
    ES_FS_ID_HEIGHT_ENABLE, ES_FS_ID_HW_CAPTION_GROUP,
    { ES_FS_ID_TYPEFACE, ES_FS_ID_HEIGHT, DIALOG_CONTROL_SELF, ES_FS_ID_HEIGHT },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_fs_height_enable_data = { { { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_FS_HEIGHT) }, ES_FS_ID_HEIGHT };

static const DIALOG_CONTROL
es_fs_height =
{
    ES_FS_ID_HEIGHT, ES_FS_ID_GROUP,
    { ES_FS_ID_HW_CAPTION_GROUP, ES_FS_ID_TYPEFACE },
    { ES_PRECTL_H, DIALOG_STDSPACING_V, ES_FS_SIZE_FIELDS_H, DIALOG_STDBUMP_V },
    { DRT(RBLT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
es_fs_height_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &es_fs_height_points_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
es_fs_height_units =
{
    ES_FS_ID_HEIGHT_UNITS, ES_FS_ID_GROUP,
    { ES_FS_ID_HEIGHT, ES_FS_ID_HEIGHT, DIALOG_CONTROL_SELF, ES_FS_ID_HEIGHT },
    { DIALOG_BUMPUNITSGAP_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

#define es_fs_height_units_data measurement_points_data

/*
width 'buddies'
*/

static const DIALOG_CONTROL
es_fs_width_enable =
{
    ES_FS_ID_WIDTH_ENABLE, ES_FS_ID_HW_CAPTION_GROUP,
    { ES_FS_ID_HEIGHT_ENABLE, ES_FS_ID_WIDTH, DIALOG_CONTROL_SELF, ES_FS_ID_WIDTH },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_fs_width_enable_data = { { { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_FS_WIDTH) }, ES_FS_ID_WIDTH };

static const DIALOG_CONTROL
es_fs_width =
{
    ES_FS_ID_WIDTH, ES_FS_ID_GROUP,
    { ES_FS_ID_HEIGHT, ES_FS_ID_HEIGHT, ES_FS_ID_HEIGHT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
es_fs_width_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &es_points_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
es_fs_width_units =
{
    ES_FS_ID_WIDTH_UNITS, ES_FS_ID_GROUP,
    { ES_FS_ID_HEIGHT_UNITS, ES_FS_ID_WIDTH, ES_FS_ID_HEIGHT_UNITS, ES_FS_ID_WIDTH },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

#define es_fs_width_units_data measurement_points_data

/*
bold 'buddies'
*/

static const DIALOG_CONTROL
es_fs_bold_enable =
{
    ES_FS_ID_BOLD_ENABLE, ES_FS_ID_GROUP,
    { ES_FS_ID_TYPEFACE, ES_FS_ID_TYPEFACE_ENABLE, DIALOG_CONTROL_SELF, ES_FS_ID_TYPEFACE_ENABLE },
    { DIALOG_STDSPACING_H, 0, DIALOG_STDCHECK_H },
    { DRT(RTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_fs_bold =
{
    ES_FS_ID_BOLD, ES_FS_ID_GROUP,
    { ES_FS_ID_BOLD_ENABLE, ES_FS_ID_BOLD_ENABLE },
    { DIALOG_CHECKGAP_H, +(ES_FS_TYPE_V - DIALOG_STDCHECK_V) / 2, ES_FS_TYPE_H, ES_FS_TYPE_V },
    { DRT(RTLT, CHECKPICTURE), 1 /*tabstop*/ }
};

/*
italic 'buddies'
*/

static const DIALOG_CONTROL
es_fs_italic_enable =
{
    ES_FS_ID_ITALIC_ENABLE, ES_FS_ID_GROUP,
    { ES_FS_ID_BOLD_ENABLE, ES_FS_ID_ITALIC, ES_FS_ID_BOLD_ENABLE },
    { 0, -(ES_FS_TYPE_V - DIALOG_STDCHECK_V) / 2, 0, DIALOG_STDCHECK_V },
    { DRT(LTRT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_fs_italic =
{
    ES_FS_ID_ITALIC, ES_FS_ID_GROUP,
    { ES_FS_ID_BOLD, ES_FS_ID_BOLD, ES_FS_ID_BOLD },
    { 0, ES_FS_BIU_SPACING_V, 0, ES_FS_TYPE_V },
    { DRT(LBRT, CHECKPICTURE), 1 /*tabstop*/ }
};

/*
underline 'buddies'
*/

static const DIALOG_CONTROL
es_fs_underline_enable =
{
    ES_FS_ID_UNDERLINE_ENABLE, ES_FS_ID_GROUP,
    { ES_FS_ID_ITALIC_ENABLE, ES_FS_ID_UNDERLINE, ES_FS_ID_ITALIC_ENABLE },
    { 0, -(ES_FS_TYPE_V - DIALOG_STDCHECK_V) / 2, 0, DIALOG_STDCHECK_V },
    { DRT(LTRT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_fs_underline =
{
    ES_FS_ID_UNDERLINE, ES_FS_ID_GROUP,
    { ES_FS_ID_ITALIC, ES_FS_ID_ITALIC, ES_FS_ID_ITALIC },
    { 0, ES_FS_BIU_SPACING_V, 0, ES_FS_TYPE_V },
    { DRT(LBRT, CHECKPICTURE), 1 /*tabstop*/ }
};

static const RESOURCE_BITMAP_ID
es_fs_underline_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_UNDERLINE };

static const DIALOG_CONTROL_DATA_CHECKPICTURE
es_fs_underline_data = { { 0 }, &es_fs_underline_bitmap };

/*
superscript 'buddies'
*/

static const DIALOG_CONTROL
es_fs_superscript_enable =
{
    ES_FS_ID_SUPERSCRIPT_ENABLE, ES_FS_ID_GROUP,
    { ES_FS_ID_UNDERLINE_ENABLE, ES_FS_ID_SUPERSCRIPT, ES_FS_ID_UNDERLINE_ENABLE },
    { 0, -(ES_FS_TYPE_V - DIALOG_STDCHECK_V) / 2, 0, DIALOG_STDCHECK_V },
    { DRT(LTRT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_fs_superscript =
{
    ES_FS_ID_SUPERSCRIPT, ES_FS_ID_GROUP,
    { ES_FS_ID_UNDERLINE, ES_FS_ID_UNDERLINE, ES_FS_ID_UNDERLINE },
    { 0, ES_FS_BIU_SPACING_V, 0, ES_FS_TYPE_V },
    { DRT(LBRT, CHECKPICTURE), 1 /*tabstop*/ }
};

static const RESOURCE_BITMAP_ID
es_fs_superscript_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_SUPERSCRIPT };

static const DIALOG_CONTROL_DATA_CHECKPICTURE
es_fs_superscript_data = { { 0 }, &es_fs_superscript_bitmap };

/*
subscript 'buddies'
*/

static const DIALOG_CONTROL
es_fs_subscript_enable =
{
    ES_FS_ID_SUBSCRIPT_ENABLE, ES_FS_ID_GROUP,
    { ES_FS_ID_SUPERSCRIPT_ENABLE, ES_FS_ID_SUBSCRIPT, ES_FS_ID_SUPERSCRIPT_ENABLE },
    { 0, -(ES_FS_TYPE_V - DIALOG_STDCHECK_V) / 2, 0, DIALOG_STDCHECK_V },
    { DRT(LTRT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_fs_subscript =
{
    ES_FS_ID_SUBSCRIPT, ES_FS_ID_GROUP,
    { ES_FS_ID_SUPERSCRIPT, ES_FS_ID_SUPERSCRIPT, ES_FS_ID_SUPERSCRIPT },
    { 0, ES_FS_BIU_SPACING_V, 0, ES_FS_TYPE_V },
    { DRT(LBRT, CHECKPICTURE), 1 /*tabstop*/ }
};

static const RESOURCE_BITMAP_ID
es_fs_subscript_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_SUBSCRIPT };

static const DIALOG_CONTROL_DATA_CHECKPICTURE
es_fs_subscript_data = { { 0 }, &es_fs_subscript_bitmap };

/*
fore group
*/

static const DIALOG_CONTROL
es_fs_colour_group_main =
{
    ES_FS_ID_COLOUR_GROUP_MAIN, ES_ID_RHS_GROUP,
    { ES_FS_ID_GROUP, ES_FS_ID_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL
es_fs_colour_group_enable =
{
    ES_FS_ID_COLOUR_GROUP_ENABLE, ES_FS_ID_COLOUR_GROUP_MAIN,
    { DIALOG_CONTROL_PARENT, ES_FS_ID_COLOUR_TX_R, DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_TX_R },
    { DIALOG_STDGROUP_LM, 0, DIALOG_STDCHECK_H },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_fs_colour_group =
{
    ES_FS_ID_COLOUR_GROUP, ES_FS_ID_COLOUR_GROUP_MAIN,
    { ES_FS_ID_COLOUR_GROUP_ENABLE, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_SMALLSPACING_H, DIALOG_STDGROUP_TM },
    { DRT(RTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
es_fs_colour_tx[3] =
{
    {
        ES_FS_ID_COLOUR_TX_R, ES_FS_ID_COLOUR_GROUP,
        { ES_FS_ID_COLOUR_GROUP, ES_FS_ID_COLOUR_R, DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_R },
        { 0, 0, RGB_TX_H },
        { DRT(LTLB, TEXTLABEL) }
    },
    {
        ES_FS_ID_COLOUR_TX_G, ES_FS_ID_COLOUR_GROUP,
        { ES_FS_ID_COLOUR_TX_R, ES_FS_ID_COLOUR_G, ES_FS_ID_COLOUR_TX_R, ES_FS_ID_COLOUR_G },
        { 0 },
        { DRT(LTRB, TEXTLABEL) }
    },
    {
        ES_FS_ID_COLOUR_TX_B, ES_FS_ID_COLOUR_GROUP,
        { ES_FS_ID_COLOUR_TX_G, ES_FS_ID_COLOUR_B, ES_FS_ID_COLOUR_TX_G, ES_FS_ID_COLOUR_B },
        { 0 },
        { DRT(LTRB, TEXTLABEL) }
    }
};

static const DIALOG_CONTROL
es_fs_colour_field[3] =
{
    {
        ES_FS_ID_COLOUR_R, ES_FS_ID_COLOUR_GROUP,
        { ES_FS_ID_COLOUR_TX_R, ES_FS_ID_COLOUR_GROUP },
        { DIALOG_LABELGAP_H, 0, RGB_FIELDS_H, DIALOG_STDBUMP_V },
        { DRT(RTLT, BUMP_S32), 1 /*tabstop*/ }
    },
    {
        ES_FS_ID_COLOUR_G, ES_FS_ID_COLOUR_GROUP,
        { ES_FS_ID_COLOUR_R, ES_FS_ID_COLOUR_R, ES_FS_ID_COLOUR_R },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
        { DRT(LBRT, BUMP_S32), 1 /*tabstop*/ }
    },
    {
        ES_FS_ID_COLOUR_B, ES_FS_ID_COLOUR_GROUP,
        { ES_FS_ID_COLOUR_G, ES_FS_ID_COLOUR_G, ES_FS_ID_COLOUR_G },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
        { DRT(LBRT, BUMP_S32), 1 /*tabstop*/ }
    }
};

static const DIALOG_CONTROL
es_fs_colour_patch =
{
    ES_FS_ID_COLOUR_PATCH, ES_FS_ID_COLOUR_GROUP,
    { ES_FS_ID_COLOUR_GROUP_MAIN, ES_FS_ID_COLOUR_B },
    { -DIALOG_STDGROUP_LM, DIALOG_STDSPACING_V, 8 * RGB_PATCHES_H, RGB_PATCHES_V },
    { DRT(LBLT, USER) }
};

static const DIALOG_CONTROL
es_fs_colour_button =
{
    ES_FS_ID_COLOUR_BUTTON, ES_FS_ID_COLOUR_GROUP,
    { ES_FS_ID_COLOUR_PATCH, ES_FS_ID_COLOUR_PATCH, ES_FS_ID_COLOUR_PATCH, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_fs_colour[16] =
{
    { ES_FS_ID_COLOUR_0, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_7, ES_FS_ID_COLOUR_1, ES_FS_ID_COLOUR_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_1, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_7, ES_FS_ID_COLOUR_2, ES_FS_ID_COLOUR_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_2, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_7, ES_FS_ID_COLOUR_3, ES_FS_ID_COLOUR_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_3, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_7, ES_FS_ID_COLOUR_4, ES_FS_ID_COLOUR_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_4, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_7, ES_FS_ID_COLOUR_5, ES_FS_ID_COLOUR_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_5, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_7, ES_FS_ID_COLOUR_6, ES_FS_ID_COLOUR_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_6, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_7, ES_FS_ID_COLOUR_7, ES_FS_ID_COLOUR_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_7, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOURS_RELATIVE, ES_FS_ID_COLOURS_RELATIVE, DIALOG_CONTROL_SELF },
  { RGB_PATCHES_H, DIALOG_STDSPACING_V, 0, RGB_PATCHES_V }, { DRT(RBRT, USER) } },
    { ES_FS_ID_COLOUR_8, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_15, ES_FS_ID_COLOUR_9, ES_FS_ID_COLOUR_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_9, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_15, ES_FS_ID_COLOUR_10, ES_FS_ID_COLOUR_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_10, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_15, ES_FS_ID_COLOUR_11, ES_FS_ID_COLOUR_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_11, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_15, ES_FS_ID_COLOUR_12, ES_FS_ID_COLOUR_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_12, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_15, ES_FS_ID_COLOUR_13, ES_FS_ID_COLOUR_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_13, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_15, ES_FS_ID_COLOUR_14, ES_FS_ID_COLOUR_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_14, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_15, ES_FS_ID_COLOUR_15, ES_FS_ID_COLOUR_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_FS_ID_COLOUR_15, ES_FS_ID_COLOUR_GROUP, { DIALOG_CONTROL_SELF, ES_FS_ID_COLOUR_7, ES_FS_ID_COLOUR_7, DIALOG_CONTROL_SELF },
  { RGB_PATCHES_H, 0, 0, RGB_PATCHES_V }, { DRT(RBRT, USER) } }
};

/*
ok
*/

#if RISCOS

static const DIALOG_CONTROL
es_fs_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF, ES_ID_RHS_GROUP, ES_ID_RHS_GROUP },
    { DIALOG_CONTENTS_CALC, DIALOG_DEFPUSHBUTTON_V, 0, 0 },
    { DRT(RBRB, PUSHBUTTON), 1 /*tabstop*/ }
};

#else
#define es_fs_ok es_ok
#endif

static const ES_DIALOG_CTL_CREATE
es_fs_ctl_create[] =
{
    { ES_ALW, { { &es_fs_group }, &es_fs_group_data } },
    { ES_ALW, { { &es_fs_typeface_list }, &stdlisttext_data } },
    { ES_ALW, { { &es_fs_typeface_enable }, &es_fs_typeface_enable_data } },
    { ES_ALW, { { &es_fs_hw_caption_group }, NULL } },
    { ES_ALW, { { &es_fs_height }, &es_fs_height_data } },
    { ES_ALW, { { &es_fs_height_units }, &es_fs_height_units_data } },
    { ES_ALW, { { &es_fs_height_enable }, &es_fs_height_enable_data } },
    { ES_ALW, { { &es_fs_width }, &es_fs_width_data } },
    { ES_ALW, { { &es_fs_width_units }, &es_fs_width_units_data } },
    { ES_ALW, { { &es_fs_width_enable }, &es_fs_width_enable_data } },
    { ES_ALW, { { &es_fs_bold }, &style_text_bold_data } },
    { ES_ALW, { { &es_fs_bold_enable }, &es_fs_bold_enable_data } },
    { ES_ALW, { { &es_fs_italic }, &style_text_italic_data } },
    { ES_ALW, { { &es_fs_italic_enable }, &es_fs_italic_enable_data } },
    { ES_ALW, { { &es_fs_underline }, &es_fs_underline_data } },
    { ES_ALW, { { &es_fs_underline_enable }, &es_fs_underline_enable_data } },
    { ES_ALW, { { &es_fs_superscript }, &es_fs_superscript_data } },
    { ES_ALW, { { &es_fs_superscript_enable }, &es_fs_superscript_enable_data } },
    { ES_ALW, { { &es_fs_subscript }, &es_fs_subscript_data } },
    { ES_ALW, { { &es_fs_subscript_enable }, &es_fs_subscript_enable_data } },

    { ES_ALW, { { &es_fs_colour_group_main }, &rgb_group_data } },
    { ES_ALW, { { &es_fs_colour_group }, NULL } },
    { ES_ALW, { { &es_fs_colour_patch }, &rgb_patch_data } }, /* patch must be created before first field */
    { ES_ALW, { { &es_fs_colour_tx[RGB_TX_IX_R] }, &rgb_tx_data[RGB_TX_IX_R] } },
    { ES_ALW, { { &es_fs_colour_field[RGB_TX_IX_R] }, &rgb_bump_data } },
    { ES_ALW, { { &es_fs_colour_tx[RGB_TX_IX_G] }, &rgb_tx_data[RGB_TX_IX_G] } },
    { ES_ALW, { { &es_fs_colour_field[RGB_TX_IX_G] }, &rgb_bump_data } },
    { ES_ALW, { { &es_fs_colour_tx[RGB_TX_IX_B] }, &rgb_tx_data[RGB_TX_IX_B] } },
    { ES_ALW, { { &es_fs_colour_field[RGB_TX_IX_B] }, &rgb_bump_data } },
    { ES_NRO, { { &es_fs_colour_button }, &rgb_button_data } },
    { ES_ALW, { { &es_fs_colour[0] }, &rgb_patches_data[0] } },
    { ES_ALW, { { &es_fs_colour[1] }, &rgb_patches_data[1] } },
    { ES_ALW, { { &es_fs_colour[2] }, &rgb_patches_data[2] } },
    { ES_ALW, { { &es_fs_colour[3] }, &rgb_patches_data[3] } },
    { ES_ALW, { { &es_fs_colour[4] }, &rgb_patches_data[4] } },
    { ES_ALW, { { &es_fs_colour[5] }, &rgb_patches_data[5] } },
    { ES_ALW, { { &es_fs_colour[6] }, &rgb_patches_data[6] } },
    { ES_ALW, { { &es_fs_colour[7] }, &rgb_patches_data[7] } },
    { ES_ALW, { { &es_fs_colour[8] }, &rgb_patches_data[8] } },
    { ES_ALW, { { &es_fs_colour[9] }, &rgb_patches_data[9] } },
    { ES_ALW, { { &es_fs_colour[10] }, &rgb_patches_data[10] } },
    { ES_ALW, { { &es_fs_colour[11] }, &rgb_patches_data[11] } },
    { ES_ALW, { { &es_fs_colour[12] }, &rgb_patches_data[12] } },
    { ES_ALW, { { &es_fs_colour[13] }, &rgb_patches_data[13] } },
    { ES_ALW, { { &es_fs_colour[14] }, &rgb_patches_data[14] } },
    { ES_ALW, { { &es_fs_colour[15] }, &rgb_patches_data[15] } },
    { ES_ALW, { { &es_fs_colour_group_enable }, &es_fs_colour_group_enable_data } },

    { ES_ALW, { { &es_fs_ok }, &es_ok_data } }
};

/******************************************************************************
*
* ruler data
*
******************************************************************************/

static const DIALOG_CONTROL
es_cs_group =
{
    ES_CS_ID_GROUP, ES_ID_RHS_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_cs_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_CS), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
es_cs_col_caption_group =
{
    ES_CS_ID_COL_CAPTION_GROUP, ES_CS_ID_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDGROUP_LM, 0, 0, 0 },
    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

/*
width 'buddies'
*/

static const DIALOG_CONTROL
es_cs_width_enable =
{
    ES_CS_ID_WIDTH_ENABLE, ES_CS_ID_COL_CAPTION_GROUP,
    { DIALOG_CONTROL_PARENT, ES_CS_ID_WIDTH, DIALOG_CONTROL_SELF, ES_CS_ID_WIDTH },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_cs_width_enable_data = { { { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_CS_WIDTH) }, ES_CS_ID_WIDTH };

static const DIALOG_CONTROL
es_cs_width =
{
    ES_CS_ID_WIDTH, ES_CS_ID_GROUP,
    { ES_PS_ID_MARGINS_CAPTION_GROUP, DIALOG_CONTROL_PARENT },
    { DIALOG_STDSPACING_H, DIALOG_STDGROUP_TM, ES_CS_FIELDS_H, DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
es_cs_width_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &es_h_units_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
es_cs_width_units =
{
    ES_CS_ID_WIDTH_UNITS, ES_CS_ID_GROUP,
    { ES_CS_ID_WIDTH, ES_CS_ID_WIDTH, DIALOG_CONTROL_SELF, ES_CS_ID_WIDTH },
    { DIALOG_BUMPUNITSGAP_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
es_cs_width_units_data = { { /*poked*/ UI_TEXT_TYPE_NONE }, { 1 /*left_text*/ } };

/*
column numform name 'buddies'
*/

static const DIALOG_CONTROL
es_cs_col_name_enable =
{
    ES_CS_ID_COL_NAME_ENABLE, ES_CS_ID_COL_CAPTION_GROUP,
    { ES_CS_ID_WIDTH_ENABLE, ES_CS_ID_COL_NAME, DIALOG_CONTROL_SELF, ES_CS_ID_COL_NAME },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_cs_col_name_enable_data = { { { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_CS_COLUMN_NAME) }, ES_CS_ID_COL_NAME };

static const DIALOG_CONTROL
es_cs_col_name =
{
    ES_CS_ID_COL_NAME, ES_CS_ID_GROUP,
    { ES_CS_ID_WIDTH, ES_CS_ID_WIDTH, ES_CS_ID_WIDTH_UNITS },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V },
    { DRT(LBRT, EDIT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_EDIT
es_cs_col_name_data = { { { FRAMED_BOX_EDIT } }, /*EDIT_XX*/ { UI_TEXT_TYPE_NONE } /* UI_TEXT state */ };

/*
horizontal justify group
*/

static const DIALOG_CONTROL
es_ps_horz_justify_group_main =
{
    ES_PS_ID_HORZ_JUSTIFY_GROUP_MAIN, ES_ID_RHS_GROUP,
    { ES_CS_ID_GROUP, ES_CS_ID_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_ps_horz_justify_group_main_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_HORZ_JUSTIFY), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
es_ps_horz_justify_group_enable =
{
    ES_PS_ID_HORZ_JUSTIFY_GROUP_ENABLE, ES_PS_ID_HORZ_JUSTIFY_GROUP_MAIN,
    { DIALOG_CONTROL_PARENT, ES_PS_ID_HORZ_JUSTIFY_GROUP, DIALOG_CONTROL_SELF, ES_PS_ID_HORZ_JUSTIFY_GROUP },
    { DIALOG_STDGROUP_LM, 0, DIALOG_STDCHECK_H },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_ps_horz_justify_group =
{
    ES_PS_ID_HORZ_JUSTIFY_GROUP, ES_PS_ID_HORZ_JUSTIFY_GROUP_MAIN,
    { ES_PS_ID_HORZ_JUSTIFY_GROUP_ENABLE, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDSPACING_H, DIALOG_STDGROUP_TM },
    { DRT(RTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

/*
* LEFT
*/

static const DIALOG_CONTROL
es_ps_horz_justify_left =
{
    ES_PS_ID_HORZ_JUSTIFY_LEFT, ES_PS_ID_HORZ_JUSTIFY_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, ES_PS_JUSTIFY_TYPE_H, ES_PS_JUSTIFY_TYPE_V },
    { DRT(LTLT, RADIOPICTURE), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const RESOURCE_BITMAP_ID
es_ps_horz_justify_left_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_J_LEFT };

static const DIALOG_CONTROL_DATA_RADIOPICTURE
es_ps_horz_justify_left_data = { { 0 }, SF_JUSTIFY_LEFT, &es_ps_horz_justify_left_bitmap };

/*
* CENTRE
*/

static const DIALOG_CONTROL
es_ps_horz_justify_centre =
{
    ES_PS_ID_HORZ_JUSTIFY_CENTRE, ES_PS_ID_HORZ_JUSTIFY_GROUP,
    { ES_PS_ID_HORZ_JUSTIFY_LEFT, ES_PS_ID_HORZ_JUSTIFY_LEFT, DIALOG_CONTROL_SELF, ES_PS_ID_HORZ_JUSTIFY_LEFT },
    { 0, 0, ES_PS_JUSTIFY_TYPE_H },
    { DRT(RTLB, RADIOPICTURE) }
};

static const RESOURCE_BITMAP_ID
es_ps_horz_justify_centre_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_J_CENTRE };

static const DIALOG_CONTROL_DATA_RADIOPICTURE
es_ps_horz_justify_centre_data = { { 0 }, SF_JUSTIFY_CENTRE, &es_ps_horz_justify_centre_bitmap };

/*
* RIGHT
*/

static const DIALOG_CONTROL
es_ps_horz_justify_right =
{
    ES_PS_ID_HORZ_JUSTIFY_RIGHT, ES_PS_ID_HORZ_JUSTIFY_GROUP,
    { ES_PS_ID_HORZ_JUSTIFY_CENTRE, ES_PS_ID_HORZ_JUSTIFY_CENTRE, DIALOG_CONTROL_SELF, ES_PS_ID_HORZ_JUSTIFY_CENTRE },
    { 0, 0, ES_PS_JUSTIFY_TYPE_H },
    { DRT(RTLB, RADIOPICTURE) }
};

static const RESOURCE_BITMAP_ID
es_ps_horz_justify_right_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_J_RIGHT };

static const DIALOG_CONTROL_DATA_RADIOPICTURE
es_ps_horz_justify_right_data = { { 0 }, SF_JUSTIFY_RIGHT, &es_ps_horz_justify_right_bitmap };

/*
* BOTH
*/

static const DIALOG_CONTROL
es_ps_horz_justify_both =
{
    ES_PS_ID_HORZ_JUSTIFY_BOTH, ES_PS_ID_HORZ_JUSTIFY_GROUP,
    { ES_PS_ID_HORZ_JUSTIFY_RIGHT, ES_PS_ID_HORZ_JUSTIFY_RIGHT, DIALOG_CONTROL_SELF, ES_PS_ID_HORZ_JUSTIFY_RIGHT },
    { 0, 0, ES_PS_JUSTIFY_TYPE_H },
    { DRT(RTLB, RADIOPICTURE) }
};

static const RESOURCE_BITMAP_ID
es_ps_horz_justify_both_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_J_FULL };

static const DIALOG_CONTROL_DATA_RADIOPICTURE
es_ps_horz_justify_both_data = { { 0 }, SF_JUSTIFY_BOTH, &es_ps_horz_justify_both_bitmap };

/*
vertical justify group
*/

static const DIALOG_CONTROL
es_ps_vert_justify_group_main =
{
    ES_PS_ID_VERT_JUSTIFY_GROUP_MAIN, ES_ID_RHS_GROUP,
    { ES_PS_ID_HORZ_JUSTIFY_GROUP_MAIN, ES_PS_ID_HORZ_JUSTIFY_GROUP_MAIN, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_ps_vert_justify_group_main_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_VERT_JUSTIFY), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
es_ps_vert_justify_group_enable =
{
    ES_PS_ID_VERT_JUSTIFY_GROUP_ENABLE, ES_PS_ID_VERT_JUSTIFY_GROUP_MAIN,
    { DIALOG_CONTROL_PARENT, ES_PS_ID_VERT_JUSTIFY_GROUP, DIALOG_CONTROL_SELF, ES_PS_ID_VERT_JUSTIFY_GROUP },
    { DIALOG_STDGROUP_LM, 0, DIALOG_STDCHECK_H },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_ps_vert_justify_group =
{
    ES_PS_ID_VERT_JUSTIFY_GROUP, ES_PS_ID_VERT_JUSTIFY_GROUP_MAIN,
    { ES_PS_ID_VERT_JUSTIFY_GROUP_ENABLE, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDSPACING_H, DIALOG_STDGROUP_TM },
    { DRT(RTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

/*
* TOP
*/

static const DIALOG_CONTROL
es_ps_vert_justify_top =
{
    ES_PS_ID_VERT_JUSTIFY_TOP, ES_PS_ID_VERT_JUSTIFY_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, ES_PS_JUSTIFY_TYPE_H, ES_PS_JUSTIFY_TYPE_V },
    { DRT(LTLT, RADIOPICTURE), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const RESOURCE_BITMAP_ID
es_ps_vert_justify_top_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_VJ_TOP };

static const DIALOG_CONTROL_DATA_RADIOPICTURE
es_ps_vert_justify_top_data = { { 0 }, SF_JUSTIFY_V_TOP, &es_ps_vert_justify_top_bitmap };

/*
* CENTRE
*/

static const DIALOG_CONTROL
es_ps_vert_justify_centre =
{
    ES_PS_ID_VERT_JUSTIFY_CENTRE, ES_PS_ID_VERT_JUSTIFY_GROUP,
    { ES_PS_ID_VERT_JUSTIFY_TOP, ES_PS_ID_VERT_JUSTIFY_TOP, DIALOG_CONTROL_SELF, ES_PS_ID_VERT_JUSTIFY_TOP },
    { 0, 0, ES_PS_JUSTIFY_TYPE_H },
    { DRT(RTLB, RADIOPICTURE) }
};

static const RESOURCE_BITMAP_ID
es_ps_vert_justify_centre_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_VJ_CENTRE };

static const DIALOG_CONTROL_DATA_RADIOPICTURE
es_ps_vert_justify_centre_data = {  { 0 }, SF_JUSTIFY_V_CENTRE, &es_ps_vert_justify_centre_bitmap };

/*
* BOTTOM
*/

static const DIALOG_CONTROL
es_ps_vert_justify_bottom =
{
    ES_PS_ID_VERT_JUSTIFY_BOTTOM, ES_PS_ID_VERT_JUSTIFY_GROUP,
    { ES_PS_ID_VERT_JUSTIFY_CENTRE, ES_PS_ID_VERT_JUSTIFY_CENTRE, DIALOG_CONTROL_SELF, ES_PS_ID_VERT_JUSTIFY_CENTRE },
    { 0, 0, ES_PS_JUSTIFY_TYPE_H },
    { DRT(RTLB, RADIOPICTURE) }
};

static const RESOURCE_BITMAP_ID
es_ps_vert_justify_bottom_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_VJ_BOTTOM };

static const DIALOG_CONTROL_DATA_RADIOPICTURE
es_ps_vert_justify_bottom_data = { { 0 }, SF_JUSTIFY_V_BOTTOM, &es_ps_vert_justify_bottom_bitmap };

/*
margins group
*/

static const DIALOG_CONTROL
es_ps_margins_group =
{
    ES_PS_ID_MARGINS_GROUP, ES_ID_RHS_GROUP,
    { DIALOG_CONTROL_PARENT, ES_CS_ID_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_ps_margins_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_MARGINS), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
es_ps_margins_caption_group =
{
    ES_PS_ID_MARGINS_CAPTION_GROUP, ES_PS_ID_MARGINS_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDGROUP_LM, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

/*
left margin 'buddies'
*/

static const DIALOG_CONTROL
es_ps_margin_left_enable =
{
    ES_PS_ID_MARGIN_LEFT_ENABLE, ES_PS_ID_MARGINS_CAPTION_GROUP,
    { DIALOG_CONTROL_PARENT, ES_PS_ID_MARGIN_LEFT, DIALOG_CONTROL_SELF, ES_PS_ID_MARGIN_LEFT },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_ps_margin_left_enable_data = { { { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_MARGIN_LEFT) }, ES_PS_ID_MARGIN_LEFT };

static const DIALOG_CONTROL
es_ps_margin_left =
{
    ES_PS_ID_MARGIN_LEFT, ES_PS_ID_MARGINS_GROUP,
    { ES_PS_ID_MARGINS_CAPTION_GROUP, DIALOG_CONTROL_PARENT },
    { ES_PRECTL_H, DIALOG_STDGROUP_TM, ES_PS_MARGIN_FIELDS_H, DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
es_ps_margin_left_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &es_h_units_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
es_ps_margin_left_units =
{
    ES_PS_ID_MARGIN_LEFT_UNITS, ES_PS_ID_MARGINS_GROUP,
    { ES_PS_ID_MARGIN_LEFT, ES_PS_ID_MARGIN_LEFT, DIALOG_CONTROL_SELF, ES_PS_ID_MARGIN_LEFT },
    { DIALOG_BUMPUNITSGAP_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
es_ps_margin_left_units_data = { { /*poked*/ UI_TEXT_TYPE_NONE }, { 1 /*left_text*/ } };

/*
para margin 'buddies'
*/

static const DIALOG_CONTROL
es_ps_margin_para_enable =
{
    ES_PS_ID_MARGIN_PARA_ENABLE, ES_PS_ID_MARGINS_CAPTION_GROUP,
    { ES_PS_ID_MARGIN_LEFT_ENABLE, ES_PS_ID_MARGIN_PARA, DIALOG_CONTROL_SELF, ES_PS_ID_MARGIN_PARA },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_ps_margin_para_enable_data = { { { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_MARGIN_PARA) }, ES_PS_ID_MARGIN_PARA };

static const DIALOG_CONTROL
es_ps_margin_para =
{
    ES_PS_ID_MARGIN_PARA, ES_PS_ID_MARGINS_GROUP,
    { ES_PS_ID_MARGIN_LEFT, ES_PS_ID_MARGIN_LEFT, ES_PS_ID_MARGIN_LEFT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
es_ps_margin_para_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &es_ps_margin_para_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
es_ps_margin_para_units =
{
    ES_PS_ID_MARGIN_PARA_UNITS, ES_PS_ID_MARGINS_GROUP,
    { ES_PS_ID_MARGIN_LEFT_UNITS, ES_PS_ID_MARGIN_PARA, ES_PS_ID_MARGIN_LEFT_UNITS, ES_PS_ID_MARGIN_PARA },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
es_ps_margin_para_units_data = { { /*poked*/ UI_TEXT_TYPE_NONE }, { 1 /*left_text*/ } };

/*
right margin 'buddies'
*/

static /*poked*/ DIALOG_CONTROL
es_ps_margin_right_enable =
{
    ES_PS_ID_MARGIN_RIGHT_ENABLE, ES_PS_ID_MARGINS_CAPTION_GROUP,
    { ES_PS_ID_MARGIN_LEFT_ENABLE, ES_PS_ID_MARGIN_RIGHT, DIALOG_CONTROL_SELF, ES_PS_ID_MARGIN_RIGHT }, /* LEFT ought to be PARA but PARA is optional */
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_ps_margin_right_enable_data = { { { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_MARGIN_RIGHT) }, ES_PS_ID_MARGIN_RIGHT };

static /*poked*/ DIALOG_CONTROL
es_ps_margin_right =
{
    ES_PS_ID_MARGIN_RIGHT, ES_PS_ID_MARGINS_GROUP,
    { ES_PS_ID_MARGIN_LEFT, ES_PS_ID_MARGIN_PARA, ES_PS_ID_MARGIN_LEFT, DIALOG_CONTROL_SELF }, /* LEFT ought to be PARA but PARA is optional */
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
es_ps_margin_right_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &es_h_units_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
es_ps_margin_right_units =
{
    ES_PS_ID_MARGIN_RIGHT_UNITS, ES_PS_ID_MARGINS_GROUP,
    { ES_PS_ID_MARGIN_LEFT_UNITS, ES_PS_ID_MARGIN_RIGHT, /* LEFT ought to be PARA but PARA is optional */
      ES_PS_ID_MARGIN_LEFT_UNITS, ES_PS_ID_MARGIN_RIGHT },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
es_ps_margin_right_units_data = { { /*poked*/ UI_TEXT_TYPE_NONE }, { 1 /*left_text*/ } };

/*
tab list 'buddies'
*/

static /*poked*/ DIALOG_CONTROL
es_ps_tab_list_group =
{
    ES_PS_ID_TAB_LIST_GROUP, ES_ID_RHS_GROUP,
    { ES_PS_ID_HORZ_JUSTIFY_GROUP_MAIN, ES_PS_ID_VERT_JUSTIFY_GROUP_MAIN, ES_PS_ID_HORZ_JUSTIFY_GROUP_MAIN, ES_PS_ID_MARGINS_GROUP },
    { 0, DIALOG_STDSPACING_V, 0, 0 },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_ps_tab_list_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_TAB_LIST), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
es_ps_tab_list_enable =
{
    ES_PS_ID_TAB_LIST_ENABLE, ES_PS_ID_TAB_LIST_GROUP,
    { DIALOG_CONTROL_PARENT, ES_PS_ID_TAB_LIST, DIALOG_CONTROL_SELF, ES_PS_ID_TAB_LIST },
    { DIALOG_STDGROUP_LM, 0, DIALOG_STDCHECK_H, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_ps_tab_list_enable_data = { { { 0, 1 /*move_focus*/ }, { UI_TEXT_TYPE_NONE } }, ES_PS_ID_TAB_LIST };

static const DIALOG_CONTROL
es_ps_tab_list =
{
    ES_PS_ID_TAB_LIST, ES_PS_ID_TAB_LIST_GROUP,
    { ES_PS_ID_TAB_LIST_ENABLE, DIALOG_CONTROL_PARENT, ES_PS_ID_TAB_LIST_UNITS, DIALOG_CONTROL_PARENT },
    { DIALOG_STDSPACING_H, DIALOG_STDGROUP_TM, DIALOG_SMALLSPACING_H, DIALOG_STDGROUP_BM },
    { DRT(RTLB, EDIT), 1 /*tabstop*/ }
};

static BITMAP(es_ps_tab_list_validation, 256);

static const DIALOG_CONTROL_DATA_EDIT
es_ps_tab_list_data = { { { FRAMED_BOX_EDIT, 0, 0, 1 /*multiline*/ }, es_ps_tab_list_validation }, /* EDIT_XX */ { UI_TEXT_TYPE_NONE } /* UI_TEXT state */ };

static const DIALOG_CONTROL
es_ps_tab_list_units =
{
    ES_PS_ID_TAB_LIST_UNITS, ES_PS_ID_TAB_LIST_GROUP,
    { DIALOG_CONTROL_SELF, ES_PS_ID_TAB_LIST, DIALOG_CONTROL_PARENT, ES_PS_ID_TAB_LIST },
    { DIALOG_CONTENTS_CALC, 0, DIALOG_STDGROUP_RM, 0 },
    { DRT(RTRB, STATICTEXT) }
};

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
es_ps_tab_list_units_data = { { /*poked*/ UI_TEXT_TYPE_NONE }, { 1 /*left_text*/ } };

/*
ok
*/

#define es_ps2_ok es_ok

static const ES_DIALOG_CTL_CREATE
es_ps2_ctl_create[] =
{
    { ES_ALW, { { &es_cs_group }, &es_cs_group_data } },
    { ES_ALW, { { &es_cs_col_caption_group }, NULL } },
    { ES_ALW, { { &es_cs_width }, &es_cs_width_data } },
    { ES_ALW, { { &es_cs_width_units }, &es_cs_width_units_data } },
    { ES_ALW, { { &es_cs_width_enable }, &es_cs_width_enable_data } },
    { ES_NUM, { { &es_cs_col_name }, &es_cs_col_name_data } },
    { ES_NUM, { { &es_cs_col_name_enable }, &es_cs_col_name_enable_data } },

    { ES_ALW, { { &es_ps_horz_justify_group_main }, &es_ps_horz_justify_group_main_data } },
    { ES_ALW, { { &es_ps_horz_justify_group }, NULL } },
    { ES_ALW, { { &es_ps_horz_justify_left }, &es_ps_horz_justify_left_data } },
    { ES_ALW, { { &es_ps_horz_justify_centre }, &es_ps_horz_justify_centre_data } },
    { ES_ALW, { { &es_ps_horz_justify_right }, &es_ps_horz_justify_right_data } },
    { ES_ATX, { { &es_ps_horz_justify_both }, &es_ps_horz_justify_both_data } },
    { ES_ALW, { { &es_ps_horz_justify_group_enable }, &es_ps_horz_justify_group_enable_data } },

    { ES_ALW, { { &es_ps_vert_justify_group_main }, &es_ps_vert_justify_group_main_data } },
    { ES_ALW, { { &es_ps_vert_justify_group }, NULL /*&es_ps_vert_justify_group_data*/ } },
    { ES_ALW, { { &es_ps_vert_justify_top }, &es_ps_vert_justify_top_data } },
    { ES_ALW, { { &es_ps_vert_justify_centre }, &es_ps_vert_justify_centre_data } },
    { ES_ALW, { { &es_ps_vert_justify_bottom }, &es_ps_vert_justify_bottom_data } },
    { ES_ALW, { { &es_ps_vert_justify_group_enable }, &es_ps_vert_justify_group_enable_data } },

    { ES_ALW, { { &es_ps_margins_group }, &es_ps_margins_group_data } },
    { ES_ALW, { { &es_ps_margins_caption_group }, NULL } },
    { ES_ALW, { { &es_ps_margin_left }, &es_ps_margin_left_data } },
    { ES_ALW, { { &es_ps_margin_left_units }, &es_ps_margin_left_units_data } },
    { ES_ALW, { { &es_ps_margin_left_enable }, &es_ps_margin_left_enable_data } },
    { ES_ATX, { { &es_ps_margin_para }, &es_ps_margin_para_data } },
    { ES_ATX, { { &es_ps_margin_para_units }, &es_ps_margin_para_units_data } },
    { ES_ATX, { { &es_ps_margin_para_enable }, &es_ps_margin_para_enable_data } },
    { ES_ALW, { { &es_ps_margin_right }, &es_ps_margin_right_data } },
    { ES_ALW, { { &es_ps_margin_right_units }, &es_ps_margin_right_units_data } },
    { ES_ALW, { { &es_ps_margin_right_enable }, &es_ps_margin_right_enable_data } },

    { ES_ATX, { { &es_ps_tab_list_group }, &es_ps_tab_list_group_data } },
    { ES_ATX, { { &es_ps_tab_list }, &es_ps_tab_list_data } },
    { ES_ATX, { { &es_ps_tab_list_units }, &es_ps_tab_list_units_data } },
    { ES_ATX, { { &es_ps_tab_list_enable }, &es_ps_tab_list_enable_data } },

    { ES_ALW, { { &es_ps2_ok }, &es_ok_data } }
};

/******************************************************************************
*
* ps1: border/grid/background
*
******************************************************************************/

/*
border group
*/

static const DIALOG_CONTROL
es_ps_border_group =
{
    ES_PS_ID_BORDER_GROUP, ES_ID_RHS_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_ps_border_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_BORDERS), { FRAMED_BOX_GROUP } };

/*
border RGB group
*/

static const DIALOG_CONTROL
es_ps_border_rgb_group_enable =
{
    ES_PS_ID_BORDER_RGB_GROUP_ENABLE, ES_PS_ID_BORDER_GROUP,
    { DIALOG_CONTROL_PARENT, ES_PS_ID_BORDER_TX_R, DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_TX_R },
    { DIALOG_STDGROUP_LM, 0, DIALOG_STDCHECK_H /*ES_BORDER_ENABLE_H*/ },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_ps_border_rgb_group =
{
    ES_PS_ID_BORDER_RGB_GROUP, ES_PS_ID_BORDER_GROUP,
    { ES_PS_ID_BORDER_RGB_GROUP_ENABLE, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_SMALLSPACING_H, DIALOG_STDGROUP_TM },
    { DRT(RTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
es_ps_border_tx[3] =
{
    {
        ES_PS_ID_BORDER_TX_R, ES_PS_ID_BORDER_RGB_GROUP,
        { ES_PS_ID_BORDER_RGB_GROUP, ES_PS_ID_BORDER_R, DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_R },
        { 0, 0, RGB_TX_H },
        { DRT(LTLB, TEXTLABEL) }
    },
    {
        ES_PS_ID_BORDER_TX_G, ES_PS_ID_BORDER_RGB_GROUP,
        { ES_PS_ID_BORDER_TX_R, ES_PS_ID_BORDER_G, ES_PS_ID_BORDER_TX_R, ES_PS_ID_BORDER_G },
        { 0 }, { DRT(LTRB, TEXTLABEL) }
    },
    {
        ES_PS_ID_BORDER_TX_B, ES_PS_ID_BORDER_RGB_GROUP,
        { ES_PS_ID_BORDER_TX_G, ES_PS_ID_BORDER_B, ES_PS_ID_BORDER_TX_G, ES_PS_ID_BORDER_B },
        { 0 }, { DRT(LTRB, TEXTLABEL) }
    }
};

static const DIALOG_CONTROL
es_ps_border_field[3] =
{
    {
        ES_PS_ID_BORDER_R, ES_PS_ID_BORDER_RGB_GROUP,
        { ES_PS_ID_BORDER_TX_R, ES_PS_ID_BORDER_RGB_GROUP },
        { DIALOG_LABELGAP_H, 0, RGB_FIELDS_H, DIALOG_STDBUMP_V },
        { DRT(RTLT, BUMP_S32), 1 /*tabstop*/ }
    },
    {
        ES_PS_ID_BORDER_G, ES_PS_ID_BORDER_RGB_GROUP,
        { ES_PS_ID_BORDER_R, ES_PS_ID_BORDER_R, ES_PS_ID_BORDER_R },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
        { DRT(LBRT, BUMP_S32), 1 /*tabstop*/ }
    },
    {
        ES_PS_ID_BORDER_B, ES_PS_ID_BORDER_RGB_GROUP,
        { ES_PS_ID_BORDER_G, ES_PS_ID_BORDER_G, ES_PS_ID_BORDER_G },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
        { DRT(LBRT, BUMP_S32), 1 /*tabstop*/ }
    }
};

static const DIALOG_CONTROL
es_ps_border_patch =
{
    ES_PS_ID_BORDER_PATCH, ES_PS_ID_BORDER_RGB_GROUP,
    { ES_PS_ID_BORDER_GROUP, ES_PS_ID_BORDER_B },
    { -DIALOG_STDGROUP_LM, DIALOG_STDSPACING_V, 8 * RGB_PATCHES_H, RGB_PATCHES_V },
    { DRT(LBLT, USER) }
};

static const DIALOG_CONTROL
es_ps_border_button =
{
    ES_PS_ID_BORDER_BUTTON, ES_PS_ID_BORDER_RGB_GROUP,
    { ES_PS_ID_BORDER_PATCH, ES_PS_ID_BORDER_PATCH, ES_PS_ID_BORDER_PATCH, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_ps_border[16] =
{
    { ES_PS_ID_BORDER_0, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_7, ES_PS_ID_BORDER_1, ES_PS_ID_BORDER_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_1, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_7, ES_PS_ID_BORDER_2, ES_PS_ID_BORDER_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_2, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_7, ES_PS_ID_BORDER_3, ES_PS_ID_BORDER_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_3, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_7, ES_PS_ID_BORDER_4, ES_PS_ID_BORDER_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_4, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_7, ES_PS_ID_BORDER_5, ES_PS_ID_BORDER_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_5, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_7, ES_PS_ID_BORDER_6, ES_PS_ID_BORDER_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_6, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_7, ES_PS_ID_BORDER_7, ES_PS_ID_BORDER_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_7, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDERS_RELATIVE, ES_PS_ID_BORDERS_RELATIVE, DIALOG_CONTROL_SELF },
  { RGB_PATCHES_H, DIALOG_STDSPACING_V, 0, RGB_PATCHES_V }, { DRT(RBRT, USER) } },
    { ES_PS_ID_BORDER_8, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_15, ES_PS_ID_BORDER_9, ES_PS_ID_BORDER_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_9, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_15, ES_PS_ID_BORDER_10, ES_PS_ID_BORDER_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_10, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_15, ES_PS_ID_BORDER_11, ES_PS_ID_BORDER_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_11, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_15, ES_PS_ID_BORDER_12, ES_PS_ID_BORDER_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_12, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_15, ES_PS_ID_BORDER_13, ES_PS_ID_BORDER_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_13, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_15, ES_PS_ID_BORDER_14, ES_PS_ID_BORDER_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_14, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_15, ES_PS_ID_BORDER_15, ES_PS_ID_BORDER_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_BORDER_15, ES_PS_ID_BORDER_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_7, ES_PS_ID_BORDER_7, DIALOG_CONTROL_SELF },
  { RGB_PATCHES_H, 0, 0, RGB_PATCHES_V }, { DRT(RBRT, USER) } }
};

/*
border line style group
*/

static const DIALOG_CONTROL
es_ps_border_line_group_enable =
{
    ES_PS_ID_BORDER_LINE_GROUP_ENABLE, ES_PS_ID_BORDER_GROUP,
    { ES_PS_ID_BORDER_RGB_GROUP_ENABLE, ES_PS_ID_BORDER_RGB_GROUP, ES_PS_ID_BORDER_RGB_GROUP },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCHECK_V },
    { DRT(LBRT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
es_ps_border_line_group_enable_data = { { 0 }, UI_TEXT_INIT_RESID(SKEL_SPLIT_MSG_DIALOG_BOX_LINE_STYLE) };

static const DIALOG_CONTROL
es_ps_border_line_group =
{
    ES_PS_ID_BORDER_LINE_GROUP, ES_PS_ID_BORDER_GROUP,
    { ES_PS_ID_BORDER_LINE_GROUP_ENABLE, ES_PS_ID_BORDER_LINE_GROUP_ENABLE, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V },
    { DRT(LBRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
es_ps_border_line[5] =
{
    {
        ES_PS_ID_BORDER_LINE_0, ES_PS_ID_BORDER_LINE_GROUP, { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
        { 0, 0, ES_PS_BORDER_LINE_H, ES_PS_BORDER_LINE_V }, { DRT(LTLT, RADIOPICTURE), 1 /*tabstop*/, 1 /*logical_group*/ }
    },
    {
        ES_PS_ID_BORDER_LINE_1, ES_PS_ID_BORDER_LINE_GROUP, { ES_PS_ID_BORDER_LINE_0, ES_PS_ID_BORDER_LINE_0, ES_PS_ID_BORDER_LINE_0, DIALOG_CONTROL_SELF },
        { 0, 0, 0, ES_PS_BORDER_LINE_V }, { DRT(LBRT, RADIOPICTURE) }
    },
    {
        ES_PS_ID_BORDER_LINE_2, ES_PS_ID_BORDER_LINE_GROUP, { ES_PS_ID_BORDER_LINE_1, ES_PS_ID_BORDER_LINE_1, DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_LINE_1 },
        { 0, 0, ES_PS_BORDER_LINE_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    },
    {
        ES_PS_ID_BORDER_LINE_3, ES_PS_ID_BORDER_LINE_GROUP, { ES_PS_ID_BORDER_LINE_1, ES_PS_ID_BORDER_LINE_1, ES_PS_ID_BORDER_LINE_1, DIALOG_CONTROL_SELF },
        { 0, 0, 0, ES_PS_BORDER_LINE_V }, { DRT(LBRT, RADIOPICTURE) }
    },
    {
        ES_PS_ID_BORDER_LINE_4, ES_PS_ID_BORDER_LINE_GROUP, { ES_PS_ID_BORDER_LINE_3, ES_PS_ID_BORDER_LINE_3, DIALOG_CONTROL_SELF, ES_PS_ID_BORDER_LINE_3 },
        { 0, 0, ES_PS_BORDER_LINE_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    }
};

/*
grid group
*/

static const DIALOG_CONTROL
es_ps_grid_group =
{
    ES_PS_ID_GRID_GROUP, ES_ID_RHS_GROUP,
    { ES_PS_ID_BORDER_GROUP, ES_PS_ID_BORDER_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_ps_grid_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_GRID), { FRAMED_BOX_GROUP } };

/*
grid RGB group
*/

static const DIALOG_CONTROL
es_ps_grid_rgb_group_enable =
{
    ES_PS_ID_GRID_RGB_GROUP_ENABLE, ES_PS_ID_GRID_GROUP,
    { DIALOG_CONTROL_PARENT, ES_PS_ID_GRID_TX_R, DIALOG_CONTROL_SELF, ES_PS_ID_GRID_TX_R },
    { DIALOG_STDGROUP_LM, 0, DIALOG_STDCHECK_H },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_ps_grid_rgb_group =
{
    ES_PS_ID_GRID_RGB_GROUP, ES_PS_ID_GRID_GROUP,
    { ES_PS_ID_GRID_RGB_GROUP_ENABLE, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_SMALLSPACING_H, DIALOG_STDGROUP_TM },
    { DRT(RTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
es_ps_grid_tx[3] =
{
    {
        ES_PS_ID_GRID_TX_R, ES_PS_ID_GRID_RGB_GROUP,
        { ES_PS_ID_GRID_RGB_GROUP, ES_PS_ID_GRID_R, DIALOG_CONTROL_SELF, ES_PS_ID_GRID_R },
        { 0, 0, RGB_TX_H },
        { DRT(LTLB, TEXTLABEL) }
    },
    {
        ES_PS_ID_GRID_TX_G, ES_PS_ID_GRID_RGB_GROUP,
        { ES_PS_ID_GRID_TX_R, ES_PS_ID_GRID_G, ES_PS_ID_GRID_TX_R, ES_PS_ID_GRID_G },
        { 0 },
        { DRT(LTRB, TEXTLABEL) }
    },
    {
        ES_PS_ID_GRID_TX_B, ES_PS_ID_GRID_RGB_GROUP,
        { ES_PS_ID_GRID_TX_G, ES_PS_ID_GRID_B, ES_PS_ID_GRID_TX_G, ES_PS_ID_GRID_B },
        { 0 },
        { DRT(LTRB, TEXTLABEL) }
    }
};

static const DIALOG_CONTROL
es_ps_grid_field[3] =
{
    {
        ES_PS_ID_GRID_R, ES_PS_ID_GRID_RGB_GROUP,
        { ES_PS_ID_GRID_TX_R, ES_PS_ID_GRID_RGB_GROUP },
        { DIALOG_LABELGAP_H, 0, RGB_FIELDS_H, DIALOG_STDBUMP_V },
        { DRT(RTLT, BUMP_S32), 1 /*tabstop*/ }
    },
    {
        ES_PS_ID_GRID_G, ES_PS_ID_GRID_RGB_GROUP,
        { ES_PS_ID_GRID_R, ES_PS_ID_GRID_R, ES_PS_ID_GRID_R },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
        { DRT(LBRT, BUMP_S32), 1 /*tabstop*/ }
    },
    {
        ES_PS_ID_GRID_B, ES_PS_ID_GRID_RGB_GROUP,
        { ES_PS_ID_GRID_G, ES_PS_ID_GRID_G, ES_PS_ID_GRID_G },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
        { DRT(LBRT, BUMP_S32), 1 /*tabstop*/ }
    }
};

static const DIALOG_CONTROL
es_ps_grid_patch =
{
    ES_PS_ID_GRID_PATCH, ES_PS_ID_GRID_RGB_GROUP,
    { ES_PS_ID_GRID_GROUP, ES_PS_ID_GRID_B },
    { -DIALOG_STDGROUP_LM, DIALOG_STDSPACING_V, 8 * RGB_PATCHES_H, RGB_PATCHES_V },
    { DRT(LBLT, USER) }
};

static const DIALOG_CONTROL
es_ps_grid_button =
{
    ES_PS_ID_GRID_BUTTON, ES_PS_ID_GRID_RGB_GROUP,
    { ES_PS_ID_GRID_PATCH, ES_PS_ID_GRID_PATCH, ES_PS_ID_GRID_PATCH, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_ps_grid[16] =
{
    { ES_PS_ID_GRID_0, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_7, ES_PS_ID_GRID_1, ES_PS_ID_GRID_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_1, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_7, ES_PS_ID_GRID_2, ES_PS_ID_GRID_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_2, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_7, ES_PS_ID_GRID_3, ES_PS_ID_GRID_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_3, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_7, ES_PS_ID_GRID_4, ES_PS_ID_GRID_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_4, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_7, ES_PS_ID_GRID_5, ES_PS_ID_GRID_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_5, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_7, ES_PS_ID_GRID_6, ES_PS_ID_GRID_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_6, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_7, ES_PS_ID_GRID_7, ES_PS_ID_GRID_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_7, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRIDS_RELATIVE, ES_PS_ID_GRIDS_RELATIVE, DIALOG_CONTROL_SELF },
  { RGB_PATCHES_H, DIALOG_STDSPACING_V, 0, RGB_PATCHES_V }, { DRT(RBRT, USER) } },
    { ES_PS_ID_GRID_8, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_15, ES_PS_ID_GRID_9, ES_PS_ID_GRID_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_9, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_15, ES_PS_ID_GRID_10, ES_PS_ID_GRID_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_10, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_15, ES_PS_ID_GRID_11, ES_PS_ID_GRID_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_11, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_15, ES_PS_ID_GRID_12, ES_PS_ID_GRID_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_12, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_15, ES_PS_ID_GRID_13, ES_PS_ID_GRID_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_13, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_15, ES_PS_ID_GRID_14, ES_PS_ID_GRID_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_14, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_15, ES_PS_ID_GRID_15, ES_PS_ID_GRID_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_GRID_15, ES_PS_ID_GRID_RGB_GROUP, { DIALOG_CONTROL_SELF, ES_PS_ID_GRID_7, ES_PS_ID_GRID_7, DIALOG_CONTROL_SELF },
  { RGB_PATCHES_H, 0, 0, RGB_PATCHES_V }, { DRT(RBRT, USER) } }
};

/*
grid line style group
*/

static const DIALOG_CONTROL
es_ps_grid_line_group_enable =
{
    ES_PS_ID_GRID_LINE_GROUP_ENABLE, ES_PS_ID_GRID_GROUP,
    { ES_PS_ID_GRID_RGB_GROUP_ENABLE, ES_PS_ID_GRID_RGB_GROUP, ES_PS_ID_GRID_RGB_GROUP },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCHECK_V },
    { DRT(LBRT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
es_ps_grid_line_group_enable_data = { { 0 }, UI_TEXT_INIT_RESID(SKEL_SPLIT_MSG_DIALOG_BOX_LINE_STYLE) };

static const DIALOG_CONTROL
es_ps_grid_line_group =
{
    ES_PS_ID_GRID_LINE_GROUP, ES_PS_ID_GRID_GROUP,
    { ES_PS_ID_GRID_LINE_GROUP_ENABLE, ES_PS_ID_GRID_LINE_GROUP_ENABLE, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V },
    { DRT(LBRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
es_ps_grid_line[5] =
{
    {
        ES_PS_ID_GRID_LINE_0, ES_PS_ID_GRID_LINE_GROUP,
        { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
        { 0, 0, ES_PS_GRID_LINE_H, ES_PS_GRID_LINE_V }, { DRT(LTLT, RADIOPICTURE), 1 /*tabstop*/, 1 /*logical_group*/ }
    },
    {
        ES_PS_ID_GRID_LINE_1, ES_PS_ID_GRID_LINE_GROUP,
        { ES_PS_ID_GRID_LINE_0, ES_PS_ID_GRID_LINE_0, ES_PS_ID_GRID_LINE_0, DIALOG_CONTROL_SELF },
        { 0, 0, 0, ES_PS_GRID_LINE_V }, { DRT(LBRT, RADIOPICTURE) }
    },
    {
        ES_PS_ID_GRID_LINE_2, ES_PS_ID_GRID_LINE_GROUP,
        { ES_PS_ID_GRID_LINE_1, ES_PS_ID_GRID_LINE_1, DIALOG_CONTROL_SELF, ES_PS_ID_GRID_LINE_1 },
        { 0, 0, ES_PS_GRID_LINE_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    },
    {
        ES_PS_ID_GRID_LINE_3, ES_PS_ID_GRID_LINE_GROUP,
        { ES_PS_ID_GRID_LINE_1, ES_PS_ID_GRID_LINE_1, ES_PS_ID_GRID_LINE_1, DIALOG_CONTROL_SELF },
        { 0, 0, 0, ES_PS_GRID_LINE_V }, { DRT(LBRT, RADIOPICTURE) }
    },
    {
        ES_PS_ID_GRID_LINE_4, ES_PS_ID_GRID_LINE_GROUP,
        { ES_PS_ID_GRID_LINE_3, ES_PS_ID_GRID_LINE_3, DIALOG_CONTROL_SELF, ES_PS_ID_GRID_LINE_3 },
        { 0, 0, ES_PS_GRID_LINE_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    }
};

/*
back colour group
*/

static const DIALOG_CONTROL
es_ps_back_group =
{
    ES_PS_ID_BACK_GROUP, ES_ID_RHS_GROUP,
    { ES_PS_ID_GRID_GROUP, ES_PS_ID_GRID_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_ps_back_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_RGB_BACK), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
es_ps_rgb_back_group_enable =
{
    ES_PS_ID_RGB_BACK_GROUP_ENABLE, ES_PS_ID_BACK_GROUP,
    { DIALOG_CONTROL_PARENT, ES_PS_ID_RGB_BACK_TX_R, DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_TX_R },
    { DIALOG_STDGROUP_LM, 0, DIALOG_STDCHECK_H },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_ps_rgb_back_group =
{
    ES_PS_ID_RGB_BACK_GROUP, ES_PS_ID_BACK_GROUP,
    { ES_PS_ID_RGB_BACK_GROUP_ENABLE, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_SMALLSPACING_H, DIALOG_STDGROUP_TM },
    { DRT(RTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
es_ps_rgb_back_group_inner =
{
    ES_PS_ID_RGB_BACK_GROUP_INNER, ES_PS_ID_RGB_BACK_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0 },
    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
es_ps_rgb_back_tx[3] =
{
    {
        ES_PS_ID_RGB_BACK_TX_R, ES_PS_ID_RGB_BACK_GROUP_INNER,
        { DIALOG_CONTROL_PARENT, ES_PS_ID_RGB_BACK_R, DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_R },
        { 0, 0, RGB_TX_H },
        { DRT(LTLB, TEXTLABEL) }
    },
    {
        ES_PS_ID_RGB_BACK_TX_G, ES_PS_ID_RGB_BACK_GROUP_INNER,
        { ES_PS_ID_RGB_BACK_TX_R, ES_PS_ID_RGB_BACK_G, ES_PS_ID_RGB_BACK_TX_R, ES_PS_ID_RGB_BACK_G },
        { 0 },
        { DRT(LTRB, TEXTLABEL) }
    },
    {
        ES_PS_ID_RGB_BACK_TX_B, ES_PS_ID_RGB_BACK_GROUP_INNER,
        { ES_PS_ID_RGB_BACK_TX_G, ES_PS_ID_RGB_BACK_B, ES_PS_ID_RGB_BACK_TX_G, ES_PS_ID_RGB_BACK_B },
        { 0 },
        { DRT(LTRB, TEXTLABEL) }
    }
};

static const DIALOG_CONTROL
es_ps_rgb_back_field[3] =
{
    {
        ES_PS_ID_RGB_BACK_R, ES_PS_ID_RGB_BACK_GROUP_INNER,
        { ES_PS_ID_RGB_BACK_TX_R, DIALOG_CONTROL_PARENT },
        { DIALOG_LABELGAP_H, 0, RGB_FIELDS_H, DIALOG_STDBUMP_V },
        { DRT(RTLT, BUMP_S32), 1 /*tabstop*/ }
    },
    {
        ES_PS_ID_RGB_BACK_G, ES_PS_ID_RGB_BACK_GROUP_INNER,
        { ES_PS_ID_RGB_BACK_R, ES_PS_ID_RGB_BACK_R, ES_PS_ID_RGB_BACK_R },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
        { DRT(LBRT, BUMP_S32), 1 /*tabstop*/ }
    },
    {
        ES_PS_ID_RGB_BACK_B, ES_PS_ID_RGB_BACK_GROUP_INNER,
        { ES_PS_ID_RGB_BACK_G, ES_PS_ID_RGB_BACK_G, ES_PS_ID_RGB_BACK_G },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
        { DRT(LBRT, BUMP_S32), 1 /*tabstop*/ }
    }
};

static const DIALOG_CONTROL
es_ps_rgb_back_patch =
{
    ES_PS_ID_RGB_BACK_PATCH, ES_PS_ID_RGB_BACK_GROUP_INNER,
    { ES_PS_ID_BACK_GROUP, ES_PS_ID_RGB_BACK_B },
    { -DIALOG_STDGROUP_LM, DIALOG_STDSPACING_V, 8 * RGB_PATCHES_H, RGB_PATCHES_V },
    { DRT(LBLT, USER) }
};

static const DIALOG_CONTROL
es_ps_rgb_back_button =
{
    ES_PS_ID_RGB_BACK_BUTTON, ES_PS_ID_RGB_BACK_GROUP_INNER,
    { ES_PS_ID_RGB_BACK_PATCH, ES_PS_ID_RGB_BACK_PATCH, ES_PS_ID_RGB_BACK_PATCH, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_ps_rgb_back[16] =
{
    { ES_PS_ID_RGB_BACK_0, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_7, ES_PS_ID_RGB_BACK_1, ES_PS_ID_RGB_BACK_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_1, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_7, ES_PS_ID_RGB_BACK_2, ES_PS_ID_RGB_BACK_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_2, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_7, ES_PS_ID_RGB_BACK_3, ES_PS_ID_RGB_BACK_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_3, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_7, ES_PS_ID_RGB_BACK_4, ES_PS_ID_RGB_BACK_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_4, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_7, ES_PS_ID_RGB_BACK_5, ES_PS_ID_RGB_BACK_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_5, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_7, ES_PS_ID_RGB_BACK_6, ES_PS_ID_RGB_BACK_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_6, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_7, ES_PS_ID_RGB_BACK_7, ES_PS_ID_RGB_BACK_7 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_7, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACKS_RELATIVE, ES_PS_ID_RGB_BACKS_RELATIVE, DIALOG_CONTROL_SELF },
  { RGB_PATCHES_H, DIALOG_STDSPACING_V, 0, RGB_PATCHES_V }, { DRT(RBRT, USER) } },
    { ES_PS_ID_RGB_BACK_8, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_15, ES_PS_ID_RGB_BACK_9, ES_PS_ID_RGB_BACK_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_9, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_15, ES_PS_ID_RGB_BACK_10, ES_PS_ID_RGB_BACK_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_10, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_15, ES_PS_ID_RGB_BACK_11, ES_PS_ID_RGB_BACK_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_11, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_15, ES_PS_ID_RGB_BACK_12, ES_PS_ID_RGB_BACK_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_12, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_15, ES_PS_ID_RGB_BACK_13, ES_PS_ID_RGB_BACK_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_13, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_15, ES_PS_ID_RGB_BACK_14, ES_PS_ID_RGB_BACK_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_14, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_15, ES_PS_ID_RGB_BACK_15, ES_PS_ID_RGB_BACK_15 },
  { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { ES_PS_ID_RGB_BACK_15, ES_PS_ID_RGB_BACK_GROUP_INNER, { DIALOG_CONTROL_SELF, ES_PS_ID_RGB_BACK_7, ES_PS_ID_RGB_BACK_7, DIALOG_CONTROL_SELF },
  { RGB_PATCHES_H, 0, 0, RGB_PATCHES_V }, { DRT(RBRT, USER) } }
};

static const DIALOG_CONTROL
es_ps_rgb_back_t =
{
    ES_PS_ID_RGB_BACK_T, ES_PS_ID_RGB_BACK_GROUP,
    { ES_PS_ID_RGB_BACK_8, ES_PS_ID_RGB_BACK_8 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

/*
ok
*/

#if RISCOS

static const DIALOG_CONTROL
es_ps1_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF, ES_ID_RHS_GROUP, ES_ID_RHS_GROUP /*bodge for pretty*/ },
    { DIALOG_CONTENTS_CALC, DIALOG_DEFPUSHBUTTON_V, 0, 0 },
    { DRT(RBRB, PUSHBUTTON), 1 /*tabstop*/ }
};

#else
#define es_ps1_ok es_ok
#endif

static ES_DIALOG_CTL_CREATE
es_ps1_ctl_create[] =
{
/*
border
*/

    { ES_ALW, { { &es_ps_border_group }, &es_ps_border_group_data } },
    { ES_ALW, { { &es_ps_border_rgb_group }, NULL } },
    { ES_ALW, { { &es_ps_border_patch }, &rgb_patch_data } }, /* patch must be created before first field */
    { ES_ALW, { { &es_ps_border_tx[RGB_TX_IX_R] }, &rgb_tx_data[RGB_TX_IX_R] } },
    { ES_ALW, { { &es_ps_border_field[RGB_TX_IX_R] }, &rgb_bump_data } },
    { ES_ALW, { { &es_ps_border_tx[RGB_TX_IX_G] }, &rgb_tx_data[RGB_TX_IX_G] } },
    { ES_ALW, { { &es_ps_border_field[RGB_TX_IX_G] }, &rgb_bump_data } },
    { ES_ALW, { { &es_ps_border_tx[RGB_TX_IX_B] }, &rgb_tx_data[RGB_TX_IX_B] } },
    { ES_ALW, { { &es_ps_border_field[RGB_TX_IX_B] }, &rgb_bump_data } },
    { ES_NRO, { { &es_ps_border_button }, &rgb_button_data } },
    { ES_ALW, { { &es_ps_border[0] }, &rgb_patches_data[0] } },
    { ES_ALW, { { &es_ps_border[1] }, &rgb_patches_data[1] } },
    { ES_ALW, { { &es_ps_border[2] }, &rgb_patches_data[2] } },
    { ES_ALW, { { &es_ps_border[3] }, &rgb_patches_data[3] } },
    { ES_ALW, { { &es_ps_border[4] }, &rgb_patches_data[4] } },
    { ES_ALW, { { &es_ps_border[5] }, &rgb_patches_data[5] } },
    { ES_ALW, { { &es_ps_border[6] }, &rgb_patches_data[6] } },
    { ES_ALW, { { &es_ps_border[7] }, &rgb_patches_data[7] } },
    { ES_ALW, { { &es_ps_border[8] }, &rgb_patches_data[8] } },
    { ES_ALW, { { &es_ps_border[9] }, &rgb_patches_data[9] } },
    { ES_ALW, { { &es_ps_border[10] }, &rgb_patches_data[10] } },
    { ES_ALW, { { &es_ps_border[11] }, &rgb_patches_data[11] } },
    { ES_ALW, { { &es_ps_border[12] }, &rgb_patches_data[12] } },
    { ES_ALW, { { &es_ps_border[13] }, &rgb_patches_data[13] } },
    { ES_ALW, { { &es_ps_border[14] }, &rgb_patches_data[14] } },
    { ES_ALW, { { &es_ps_border[15] }, &rgb_patches_data[15] } },
    { ES_ALW, { { &es_ps_border_rgb_group_enable }, &es_ps_border_rgb_group_enable_data } },

    { ES_ALW, { { &es_ps_border_line_group }, NULL } },
    { ES_ALW, { { &es_ps_border_line[0] }, &line_style_data[0] } },
    { ES_ALW, { { &es_ps_border_line[1] }, &line_style_data[1] } },
    { ES_ALW, { { &es_ps_border_line[2] }, &line_style_data[2] } },
    { ES_ALW, { { &es_ps_border_line[3] }, &line_style_data[3] } },
    { ES_ALW, { { &es_ps_border_line[4] }, &line_style_data[4] } },
    { ES_ALW, { { &es_ps_border_line_group_enable }, &es_ps_border_line_group_enable_data } },

/*
grid
*/

    { ES_ALW, { { &es_ps_grid_group }, &es_ps_grid_group_data } },
    { ES_ALW, { &es_ps_grid_rgb_group } },
    { ES_ALW, { { &es_ps_grid_patch }, &rgb_patch_data } }, /* patch must be created before first field */
    { ES_ALW, { { &es_ps_grid_tx[RGB_TX_IX_R] },&rgb_tx_data[RGB_TX_IX_R] } },
    { ES_ALW, { { &es_ps_grid_field[RGB_TX_IX_R] }, &rgb_bump_data } },
    { ES_ALW, { { &es_ps_grid_tx[RGB_TX_IX_G] }, &rgb_tx_data[RGB_TX_IX_G] } },
    { ES_ALW, { { &es_ps_grid_field[RGB_TX_IX_G] }, &rgb_bump_data } },
    { ES_ALW, { { &es_ps_grid_tx[RGB_TX_IX_B] }, &rgb_tx_data[RGB_TX_IX_B] } },
    { ES_ALW, { { &es_ps_grid_field[RGB_TX_IX_B] }, &rgb_bump_data } },
    { ES_NRO, { { &es_ps_grid_button }, &rgb_button_data } },
    { ES_ALW, { { &es_ps_grid[0] }, &rgb_patches_data[0] } },
    { ES_ALW, { { &es_ps_grid[1] }, &rgb_patches_data[1] } },
    { ES_ALW, { { &es_ps_grid[2] }, &rgb_patches_data[2] } },
    { ES_ALW, { { &es_ps_grid[3] }, &rgb_patches_data[3] } },
    { ES_ALW, { { &es_ps_grid[4] }, &rgb_patches_data[4] } },
    { ES_ALW, { { &es_ps_grid[5] }, &rgb_patches_data[5] } },
    { ES_ALW, { { &es_ps_grid[6] }, &rgb_patches_data[6] } },
    { ES_ALW, { { &es_ps_grid[7] }, &rgb_patches_data[7] } },
    { ES_ALW, { { &es_ps_grid[8] }, &rgb_patches_data[8] } },
    { ES_ALW, { { &es_ps_grid[9] }, &rgb_patches_data[9] } },
    { ES_ALW, { { &es_ps_grid[10] }, &rgb_patches_data[10] } },
    { ES_ALW, { { &es_ps_grid[11] }, &rgb_patches_data[11] } },
    { ES_ALW, { { &es_ps_grid[12] }, &rgb_patches_data[12] } },
    { ES_ALW, { { &es_ps_grid[13] }, &rgb_patches_data[13] } },
    { ES_ALW, { { &es_ps_grid[14] }, &rgb_patches_data[14] } },
    { ES_ALW, { { &es_ps_grid[15] }, &rgb_patches_data[15] } },
    { ES_ALW, { { &es_ps_grid_rgb_group_enable }, &es_ps_grid_rgb_group_enable_data } },

    { ES_ALW, { &es_ps_grid_line_group } },
    { ES_ALW, { { &es_ps_grid_line[0] }, &line_style_data[0] } },
    { ES_ALW, { { &es_ps_grid_line[1] }, &line_style_data[1] } },
    { ES_ALW, { { &es_ps_grid_line[2] }, &line_style_data[2] } },
    { ES_ALW, { { &es_ps_grid_line[3] }, &line_style_data[3] } },
    { ES_ALW, { { &es_ps_grid_line[4] }, &line_style_data[4] } },
    { ES_ALW, { { &es_ps_grid_line_group_enable }, &es_ps_grid_line_group_enable_data } },

/*
background
*/

    { ES_ALW, { { &es_ps_back_group }, &es_ps_back_group_data } },
    { ES_ALW, { &es_ps_rgb_back_group } },
    { ES_ALW, { &es_ps_rgb_back_group_inner } },
    { ES_ALW, { { &es_ps_rgb_back_patch }, &rgb_patch_data } }, /* patch must be created before first field */
    { ES_ALW, { { &es_ps_rgb_back_tx[RGB_TX_IX_R] }, &rgb_tx_data[RGB_TX_IX_R] } },
    { ES_ALW, { { &es_ps_rgb_back_field[RGB_TX_IX_R] }, &rgb_bump_data } },
    { ES_ALW, { { &es_ps_rgb_back_tx[RGB_TX_IX_G] }, &rgb_tx_data[RGB_TX_IX_G] } },
    { ES_ALW, { { &es_ps_rgb_back_field[RGB_TX_IX_G] }, &rgb_bump_data } },
    { ES_ALW, { { &es_ps_rgb_back_tx[RGB_TX_IX_B] }, &rgb_tx_data[RGB_TX_IX_B] } },
    { ES_ALW, { { &es_ps_rgb_back_field[RGB_TX_IX_B] }, &rgb_bump_data } },
    { ES_NRO, { { &es_ps_rgb_back_button }, &rgb_button_data } },
    { ES_ALW, { { &es_ps_rgb_back[0] }, &rgb_patches_data[0] } },
    { ES_ALW, { { &es_ps_rgb_back[1] }, &rgb_patches_data[1] } },
    { ES_ALW, { { &es_ps_rgb_back[2] }, &rgb_patches_data[2] } },
    { ES_ALW, { { &es_ps_rgb_back[3] }, &rgb_patches_data[3] } },
    { ES_ALW, { { &es_ps_rgb_back[4] }, &rgb_patches_data[4] } },
    { ES_ALW, { { &es_ps_rgb_back[5] }, &rgb_patches_data[5] } },
    { ES_ALW, { { &es_ps_rgb_back[6] }, &rgb_patches_data[6] } },
    { ES_ALW, { { &es_ps_rgb_back[7] }, &rgb_patches_data[7] } },
    { ES_ALW, { { &es_ps_rgb_back[8] }, &rgb_patches_data[8] } },
    { ES_ALW, { { &es_ps_rgb_back[9] }, &rgb_patches_data[9] } },
    { ES_ALW, { { &es_ps_rgb_back[10] }, &rgb_patches_data[10] } },
    { ES_ALW, { { &es_ps_rgb_back[11] }, &rgb_patches_data[11] } },
    { ES_ALW, { { &es_ps_rgb_back[12] }, &rgb_patches_data[12] } },
    { ES_ALW, { { &es_ps_rgb_back[13] }, &rgb_patches_data[13] } },
    { ES_ALW, { { &es_ps_rgb_back[14] }, &rgb_patches_data[14] } },
    { ES_ALW, { { &es_ps_rgb_back[15] }, &rgb_patches_data[15] } },
    { ES_ALW, { { &es_ps_rgb_back_t }, &es_tx_t_data } },
    { ES_ALW, { { &es_ps_rgb_back_group_enable }, &es_ps_rgb_back_group_enable_data } },

    { ES_ALW, { { &es_ps1_ok }, &es_ok_data } }
};

/******************************************************************************
*
* para/line spacing
*
******************************************************************************/

/*
paragraph spacing group
*/

static const DIALOG_CONTROL
es_ps_para_space_group =
{
    ES_PS_ID_PARA_SPACE_GROUP, ES_ID_RHS_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_ps_para_space_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_PARA), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
es_ps_para_space_caption_group =
{
    ES_PS_ID_PARA_SPACE_CAPTION_GROUP, ES_PS_ID_PARA_SPACE_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0 },
    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

/*
para start 'buddies'
*/

static const DIALOG_CONTROL
es_ps_para_start_enable =
{
    ES_PS_ID_PARA_START_ENABLE, ES_PS_ID_PARA_SPACE_CAPTION_GROUP,
    { DIALOG_CONTROL_PARENT, ES_PS_ID_PARA_START, DIALOG_CONTROL_SELF, ES_PS_ID_PARA_START },
    { DIALOG_STDGROUP_LM, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_ps_para_start_enable_data = { { { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_PARA_START) }, ES_PS_ID_PARA_START };

static const DIALOG_CONTROL
es_ps_para_start =
{
    ES_PS_ID_PARA_START, ES_PS_ID_PARA_SPACE_GROUP,
    { ES_PS_ID_PARA_SPACE_CAPTION_GROUP, DIALOG_CONTROL_PARENT },
    { ES_PRECTL_H, DIALOG_STDGROUP_TM, ES_PS_SPACING_FIELDS_H, DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
es_ps_para_start_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &es_v_units_fine_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
es_ps_para_start_units =
{
    ES_PS_ID_PARA_START_UNITS, ES_PS_ID_PARA_SPACE_GROUP,
    { ES_PS_ID_PARA_START, ES_PS_ID_PARA_START, DIALOG_CONTROL_SELF, ES_PS_ID_PARA_START },
    { DIALOG_BUMPUNITSGAP_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
es_ps_para_start_units_data = { { /*poked*/ UI_TEXT_TYPE_NONE }, { 1 /*left_text*/ } };

/*
para end 'buddies'
*/

static const DIALOG_CONTROL
es_ps_para_end_enable =
{
    ES_PS_ID_PARA_END_ENABLE, ES_PS_ID_PARA_SPACE_CAPTION_GROUP,
    { ES_PS_ID_PARA_START_ENABLE, ES_PS_ID_PARA_END, DIALOG_CONTROL_SELF, ES_PS_ID_PARA_END },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_ps_para_end_enable_data = { {  { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_PARA_END) }, ES_PS_ID_PARA_END };

static const DIALOG_CONTROL
es_ps_para_end =
{
    ES_PS_ID_PARA_END, ES_PS_ID_PARA_SPACE_GROUP,
    { ES_PS_ID_PARA_START, ES_PS_ID_PARA_START, ES_PS_ID_PARA_START },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
es_ps_para_end_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &es_v_units_fine_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
es_ps_para_end_units =
{
    ES_PS_ID_PARA_END_UNITS, ES_PS_ID_PARA_SPACE_GROUP,
    { ES_PS_ID_PARA_START_UNITS, ES_PS_ID_PARA_END, ES_PS_ID_PARA_START_UNITS, ES_PS_ID_PARA_END },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
es_ps_para_end_units_data = { { /*poked*/ UI_TEXT_TYPE_NONE }, { 1 /*left_text*/ } };

/*
line space group
*/

static const DIALOG_CONTROL
es_ps_line_space_group_main =
{
    ES_PS_ID_LINE_SPACE_GROUP_MAIN, ES_ID_RHS_GROUP,
    { ES_PS_ID_PARA_SPACE_GROUP, ES_PS_ID_PARA_SPACE_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_ps_line_space_group_main_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_LINE), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
es_ps_line_space_group_enable =
{
    ES_PS_ID_LINE_SPACE_GROUP_ENABLE, ES_PS_ID_LINE_SPACE_GROUP_MAIN,
    { DIALOG_CONTROL_PARENT, ES_PS_ID_LINE_SPACE_GROUP, DIALOG_CONTROL_SELF, ES_PS_ID_LINE_SPACE_GROUP },
    { DIALOG_STDGROUP_LM, 0, DIALOG_STDCHECK_H },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
es_ps_line_space_group =
{
    ES_PS_ID_LINE_SPACE_GROUP, ES_PS_ID_LINE_SPACE_GROUP_MAIN,
    { ES_PS_ID_LINE_SPACE_GROUP_ENABLE, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDSPACING_H, DIALOG_STDGROUP_TM },
    { DRT(RTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

/*
* LINE_SPACE_SINGLE
*/

static const DIALOG_CONTROL
es_ps_line_space_single =
{
    ES_PS_ID_LINE_SPACE_SINGLE, ES_PS_ID_LINE_SPACE_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, ES_PS_LINE_SPACE_TYPE_H, ES_PS_LINE_SPACE_TYPE_V },
    { DRT(LTLT, RADIOPICTURE), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const RESOURCE_BITMAP_ID
es_ps_line_space_single_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_PS_SINGLE };

static const DIALOG_CONTROL_DATA_RADIOPICTURE
es_ps_line_space_single_data = { { 0 }, SF_LINE_SPACE_SINGLE, &es_ps_line_space_single_bitmap };

/*
* LINE_SPACE_ONEP5
*/

static const DIALOG_CONTROL
es_ps_line_space_onep5 =
{
    ES_PS_ID_LINE_SPACE_ONEP5, ES_PS_ID_LINE_SPACE_GROUP,
    { ES_PS_ID_LINE_SPACE_SINGLE, ES_PS_ID_LINE_SPACE_SINGLE, DIALOG_CONTROL_SELF, ES_PS_ID_LINE_SPACE_SINGLE },
    { 0, 0, ES_PS_LINE_SPACE_TYPE_H },
    { DRT(RTLB, RADIOPICTURE) }
};

static const RESOURCE_BITMAP_ID
es_ps_line_space_onep5_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_PS_ONEP5 };

static const DIALOG_CONTROL_DATA_RADIOPICTURE
es_ps_line_space_onep5_data = { { 0 }, SF_LINE_SPACE_ONEP5, &es_ps_line_space_onep5_bitmap };

/*
* LINE_SPACE_DOUBLE
*/

static const DIALOG_CONTROL
es_ps_line_space_double =
{
    ES_PS_ID_LINE_SPACE_DOUBLE, ES_PS_ID_LINE_SPACE_GROUP,
    { ES_PS_ID_LINE_SPACE_ONEP5, ES_PS_ID_LINE_SPACE_ONEP5, DIALOG_CONTROL_SELF, ES_PS_ID_LINE_SPACE_ONEP5 },
    { 0, 0, ES_PS_LINE_SPACE_TYPE_H },
    { DRT(RTLB, RADIOPICTURE) }
};

static const RESOURCE_BITMAP_ID
es_ps_line_space_double_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_PS_DOUBLE };

static const DIALOG_CONTROL_DATA_RADIOPICTURE
es_ps_line_space_double_data = { { 0 }, SF_LINE_SPACE_DOUBLE, &es_ps_line_space_double_bitmap };

/*
* LINE_SPACE_DOUBLE
*/

static const DIALOG_CONTROL
es_ps_line_space_n =
{
    ES_PS_ID_LINE_SPACE_N, ES_PS_ID_LINE_SPACE_GROUP,
    { ES_PS_ID_LINE_SPACE_DOUBLE, ES_PS_ID_LINE_SPACE_DOUBLE, DIALOG_CONTROL_SELF, ES_PS_ID_LINE_SPACE_DOUBLE },
    { 0, 0, ES_PS_LINE_SPACE_TYPE_H },
    { DRT(RTLB, RADIOPICTURE) }
};

static const RESOURCE_BITMAP_ID
es_ps_line_space_n_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_PS_N };

static const DIALOG_CONTROL_DATA_RADIOPICTURE
es_ps_line_space_n_data = { { 0 }, SF_LINE_SPACE_SET, &es_ps_line_space_n_bitmap };

static const DIALOG_CONTROL
es_ps_line_space_n_val =
{
    ES_PS_ID_LINE_SPACE_N_VAL, ES_PS_ID_LINE_SPACE_GROUP,
    { ES_PS_ID_LINE_SPACE_SINGLE, ES_PS_ID_LINE_SPACE_SINGLE },
    { 0, DIALOG_STDSPACING_V, ES_PS_LINE_SPACE_FIELDS_H, DIALOG_STDBUMP_V },
    { DRT(LBLT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
es_ps_line_space_n_val_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &es_points_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
es_ps_line_space_n_val_units =
{
    ES_PS_ID_LINE_SPACE_N_VAL_UNITS, ES_PS_ID_LINE_SPACE_GROUP,
    { ES_PS_ID_LINE_SPACE_N_VAL, ES_PS_ID_LINE_SPACE_N_VAL, DIALOG_CONTROL_SELF, ES_PS_ID_LINE_SPACE_N_VAL },
    { DIALOG_BUMPUNITSGAP_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

#define es_ps_line_space_n_val_units_data measurement_points_data

/*
ok
*/

#define es_ps3_ok es_ok

static ES_DIALOG_CTL_CREATE
es_ps3_ctl_create[] =
{
    { ES_ATX, { { &es_ps_para_space_group }, &es_ps_para_space_group_data } },
    { ES_ATX, { { &es_ps_para_space_caption_group }, NULL } },
    { ES_ATX, { { &es_ps_para_start }, &es_ps_para_start_data } },
    { ES_ATX, { { &es_ps_para_start_units }, &es_ps_para_start_units_data } },
    { ES_ATX, { { &es_ps_para_start_enable }, &es_ps_para_start_enable_data } },
    { ES_ATX, { { &es_ps_para_end }, &es_ps_para_end_data } },
    { ES_ATX, { { &es_ps_para_end_units }, &es_ps_para_end_units_data } },
    { ES_ATX, { { &es_ps_para_end_enable }, &es_ps_para_end_enable_data } },
    { ES_ATX, { { &es_ps_line_space_group_main }, &es_ps_line_space_group_main_data } },
    { ES_ATX, { { &es_ps_line_space_group }, NULL } },
    { ES_ATX, { { &es_ps_line_space_single }, &es_ps_line_space_single_data } },
    { ES_ATX, { { &es_ps_line_space_onep5 }, &es_ps_line_space_onep5_data } },
    { ES_ATX, { { &es_ps_line_space_double }, &es_ps_line_space_double_data } },
    { ES_ATX, { { &es_ps_line_space_n }, &es_ps_line_space_n_data } },
    { ES_ATX, { { &es_ps_line_space_n_val }, &es_ps_line_space_n_val_data } },
    { ES_ATX, { { &es_ps_line_space_n_val_units }, &es_ps_line_space_n_val_units_data } },
    { ES_ATX, { { &es_ps_line_space_group_enable }, &es_ps_line_space_group_enable_data } },

    { ES_ALW, { { &es_ps3_ok }, &es_ok_data } }
};

/******************************************************************************
*
* numform/new object
*
******************************************************************************/

static const DIALOG_CONTROL
es_ps_numform_group =
{
    ES_PS_ID_NUMFORM_GROUP, ES_ID_RHS_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_ps_numform_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_NUMFORM_GROUP), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
es_ps_numform_list_nu_enable =
{
    ES_PS_ID_NUMFORM_LIST_NU_ENABLE, ES_PS_ID_NUMFORM_GROUP,
    { DIALOG_CONTROL_PARENT, ES_PS_ID_NUMFORM_LIST_NU, DIALOG_CONTROL_SELF, ES_PS_ID_NUMFORM_LIST_NU },
    { DIALOG_STDGROUP_LM, 0, DIALOG_STDCHECK_H + DIALOG_CHECKGAP_H + MAX(DIALOG_SYSCHARS_H("umber") + DIALOG_FATCHAR_H, DIALOG_SYSCHARS_H("Date")), 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
es_ps_numform_list_nu_enable_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_NUMFORM_NU) };

static const DIALOG_CONTROL
es_ps_numform_list_nu =
{
    ES_PS_ID_NUMFORM_LIST_NU, ES_PS_ID_NUMFORM_GROUP,
    { ES_PS_ID_NUMFORM_LIST_NU_ENABLE, DIALOG_CONTROL_PARENT },
    { DIALOG_SMALLSPACING_H, DIALOG_STDGROUP_TM, ES_PS_NUMFORM_LIST_H, DIALOG_STDCOMBO_V },
    { DRT(RTLT, COMBO_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL_DATA_COMBO_TEXT
es_ps_numform_list_nu_data =
{
  {/*combo_xx*/

    {/*edit_xx*/ {/*bits*/ FRAMED_BOX_EDIT /*bits*/}, NULL /*edit_xx*/},
    {/*list_xx*/ { 0 /*force_v_scroll*/, 0 /*disable_double*/, 0 /*tab_position*/} /*list_xx*/},
        NULL,
        ES_PS_NUMFORM_LIST_DROP_V /*dropdown_size*/

#if RISCOS
        , &es_ps_numform_list_nu_enable_data.caption
#endif

      /*combo_xx*/},
    { UI_TEXT_TYPE_NONE } /*state*/
};

static const DIALOG_CONTROL
es_ps_numform_list_dt_enable =
{
    ES_PS_ID_NUMFORM_LIST_DT_ENABLE, ES_PS_ID_NUMFORM_GROUP,
    { ES_PS_ID_NUMFORM_LIST_NU_ENABLE, ES_PS_ID_NUMFORM_LIST_DT, ES_PS_ID_NUMFORM_LIST_NU_ENABLE, ES_PS_ID_NUMFORM_LIST_DT },
    { 0 },
    { DRT(LTRB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
es_ps_numform_list_dt_enable_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_NUMFORM_DT) };

static const DIALOG_CONTROL
es_ps_numform_list_dt =
{
    ES_PS_ID_NUMFORM_LIST_DT, ES_PS_ID_NUMFORM_GROUP,
    { ES_PS_ID_NUMFORM_LIST_NU, ES_PS_ID_NUMFORM_LIST_NU, ES_PS_ID_NUMFORM_LIST_NU },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCOMBO_V },
    { DRT(LBRT, COMBO_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

#if RISCOS
static const UI_TEXT
es_ps_numform_list_dt_dropdown_text = UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_NUMFORM_DT_DROPDOWN);
#endif

static const DIALOG_CONTROL_DATA_COMBO_TEXT
es_ps_numform_list_dt_data =
{
  {/*combo_xx*/

    {/*edit_xx*/ {/*bits*/ FRAMED_BOX_EDIT /*bits*/}, NULL /*edit_xx*/},
    {/*list_xx*/ { 0 /*force_v_scroll*/, 0 /*disable_double*/, 0 /*tab_position*/} /*list_xx*/},
        NULL,
        ES_PS_NUMFORM_LIST_DROP_V /*dropdown_size*/

#if RISCOS
        , &es_ps_numform_list_dt_dropdown_text
#endif

      /*combo_xx*/},
    { UI_TEXT_TYPE_NONE } /*state*/
};

static const DIALOG_CONTROL
es_ps_numform_list_se_enable =
{
    ES_PS_ID_NUMFORM_LIST_SE_ENABLE, ES_PS_ID_NUMFORM_GROUP,
    { ES_PS_ID_NUMFORM_LIST_DT_ENABLE, ES_PS_ID_NUMFORM_LIST_SE, ES_PS_ID_NUMFORM_LIST_DT_ENABLE, ES_PS_ID_NUMFORM_LIST_SE },
    { 0 },
    { DRT(LTRB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
es_ps_numform_list_se_enable_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_NUMFORM_SE) };

static const DIALOG_CONTROL
es_ps_numform_list_se =
{
    ES_PS_ID_NUMFORM_LIST_SE, ES_PS_ID_NUMFORM_GROUP,
    { ES_PS_ID_NUMFORM_LIST_DT, ES_PS_ID_NUMFORM_LIST_DT, ES_PS_ID_NUMFORM_LIST_DT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCOMBO_V },
    { DRT(LBRT, COMBO_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

#if RISCOS
static const UI_TEXT
es_ps_numform_list_se_dropdown_text = UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_NUMFORM_SE_DROPDOWN);
#endif

static const DIALOG_CONTROL_DATA_COMBO_TEXT
es_ps_numform_list_se_data =
{
  {/*combo_xx*/

    {/*edit_xx*/ {/*bits*/ FRAMED_BOX_EDIT /*bits*/}, NULL /*edit_xx*/},
    {/*list_xx*/ { 0 /*force_v_scroll*/, 0 /*disable_double*/, 0 /*tab_position*/} /*list_xx*/},
        NULL,
        ES_PS_NUMFORM_LIST_DROP_V /*dropdown_size*/

#if RISCOS
        , &es_ps_numform_list_se_dropdown_text
#endif

      /*combo_xx*/},
    { UI_TEXT_TYPE_NONE } /*state*/
};

static const DIALOG_CONTROL
es_ps_new_object_group =
{
    ES_PS_ID_NEW_OBJECT_GROUP, ES_ID_RHS_GROUP,
    { ES_PS_ID_NUMFORM_GROUP, ES_PS_ID_NUMFORM_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_ps_new_object_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_NEW_OBJECT), { FRAMED_BOX_GROUP } };

/*
new object list 'buddies'
*/

static const DIALOG_CONTROL
es_ps_new_object_list_enable =
{
    ES_PS_ID_NEW_OBJECT_LIST_ENABLE, ES_PS_ID_NEW_OBJECT_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_STDCHECK_H, DIALOG_STDCOMBO_V },
    { DRT(LTLT, CHECKBOX), 1 /*tabstop*/ }
};

#if 1

static const DIALOG_CONTROL
es_ps_new_object_list =
{
    ES_PS_ID_NEW_OBJECT_LIST, ES_PS_ID_NEW_OBJECT_GROUP,
    { ES_PS_ID_NEW_OBJECT_LIST_ENABLE, ES_PS_ID_NEW_OBJECT_LIST_ENABLE },
#if RISCOS
    { DIALOG_STDSPACING_H, 0, ES_PS_NEW_OBJECT_LIST_H, DIALOG_STDLISTOVH_V + 3 * DIALOG_STDLISTITEM_V },
#else
    { DIALOG_STDSPACING_H, 0, ES_PS_NEW_OBJECT_LIST_H, DIALOG_STDLISTOVH_V + 2 * DIALOG_STDLISTITEM_V },
#endif
    { DRT(RTLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL_DATA_LIST_TEXT
es_ps_new_object_list_data =
{
    {/*list_xx*/ { 0 /*force_v_scroll*/, 0 /*disable_double*/, 0 /*tab_position*/} /*list_xx*/},
    { UI_TEXT_TYPE_NONE } /*state*/
};

#else

static const DIALOG_CONTROL
es_ps_new_object_list =
{
    ES_PS_ID_NEW_OBJECT_LIST, ES_PS_ID_NEW_OBJECT_GROUP,
    { ES_PS_ID_NEW_OBJECT_LIST_ENABLE, ES_PS_ID_NEW_OBJECT_LIST_ENABLE },
    { DIALOG_STDSPACING_H, 0, ES_PS_NEW_OBJECT_LIST_H, DIALOG_STDCOMBO_V },
    { DRT(RTLT, COMBO_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

#if RISCOS
static const UI_TEXT
es_ps_new_object_list_data_caption = UI_TEXT_INIT_RESID(SKEL_SPLIT_MSG_NEW_OBJECT_CAPTION);
#endif

static const DIALOG_CONTROL_DATA_COMBO_TEXT
es_ps_new_object_list_data =
{
  {/*combo_xx*/

    {/*edit_xx*/ {/*bits*/ FRAMED_BOX_EDIT_READ_ONLY, 1 /*read_only*/ /*bits*/}, NULL /*edit_xx*/},
    {/*list_xx*/ { 0 /*force_v_scroll*/, 0 /*disable_double*/, 0 /*tab_position*/} /*list_xx*/},
        NULL,
        ES_PS_NEW_OBJECT_LIST_DROP_V /*dropdown_size*/

#if RISCOS
        , &es_ps_new_object_list_data_caption
#endif

      /*combo_xx*/},
    { UI_TEXT_TYPE_NONE } /*state*/
};

#endif

/*
protection
*/

static const DIALOG_CONTROL
es_ps_protection_enable =
{
    ES_PS_ID_PROTECTION_ENABLE, ES_ID_RHS_GROUP,
    { ES_PS_ID_NEW_OBJECT_LIST_ENABLE, ES_PS_ID_PROTECTION, DIALOG_CONTROL_SELF, ES_PS_ID_PROTECTION },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_ps_protection_enable_data = { { { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS_PROTECTION) }, ES_PS_ID_PROTECTION };

static const DIALOG_CONTROL
es_ps_protection =
{
    ES_PS_ID_PROTECTION, ES_ID_RHS_GROUP,
    { ES_PS_ID_PROTECTION_ENABLE, ES_PS_ID_NEW_OBJECT_GROUP },
    { DIALOG_STDSPACING_H, DIALOG_GROUPSPACING_V, DIALOG_STDCHECK_H, DIALOG_STDCHECK_V },
    { DRT(RBLT, CHECKBOX), 1 /*tabstop*/ }
};

/*
ok
*/

#if RISCOS
#define es_ps4_ok es_fs_ok
#else
#define es_ps4_ok es_ok
#endif

static ES_DIALOG_CTL_CREATE
es_ps4_ctl_create[] =
{
    { ES_ALW, { { &es_ps_numform_group }, &es_ps_numform_group_data } },
    { ES_NUM, { { &es_ps_numform_list_nu }, &es_ps_numform_list_nu_data } },
    { ES_NUM, { { &es_ps_numform_list_nu_enable }, &es_ps_numform_list_nu_enable_data } },
    { ES_NUM, { { &es_ps_numform_list_dt }, &es_ps_numform_list_dt_data } },
    { ES_NUM, { { &es_ps_numform_list_dt_enable }, &es_ps_numform_list_dt_enable_data } },
    { ES_NUM, { { &es_ps_numform_list_se }, &es_ps_numform_list_se_data } },
    { ES_NUM, { { &es_ps_numform_list_se_enable }, &es_ps_numform_list_se_enable_data } },
    { ES_ALW, { { &es_ps_new_object_group }, &es_ps_new_object_group_data } },
    { ES_ALW, { { &es_ps_new_object_list }, &es_ps_new_object_list_data } },
    { ES_ALW, { { &es_ps_new_object_list_enable }, &es_ps_new_object_list_enable_data } },
    { ES_NUM, { { &es_ps_protection }, &es_ps_protection_data } },
    { ES_NUM, { { &es_ps_protection_enable }, &es_ps_protection_enable_data } },

    { ES_ALW, { { &es_ps4_ok }, &es_ok_data } }
};

/******************************************************************************
*
* row info
*
******************************************************************************/

static const DIALOG_CONTROL
es_rs_group =
{
    ES_RS_ID_GROUP, ES_ID_RHS_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
es_rs_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_RS), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
es_rs_caption_group =
{
    ES_RS_ID_CAPTION_GROUP, ES_RS_ID_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0 },
    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

/*
height 'buddies'
*/

static const DIALOG_CONTROL
es_rs_height_enable =
{
    ES_RS_ID_HEIGHT_ENABLE, ES_RS_ID_CAPTION_GROUP,
    { DIALOG_CONTROL_PARENT, ES_RS_ID_HEIGHT, DIALOG_CONTROL_SELF, ES_RS_ID_HEIGHT },
    { DIALOG_STDGROUP_LM, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_rs_height_enable_data = { { { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_RS_HEIGHT) }, ES_RS_ID_HEIGHT };

static const DIALOG_CONTROL
es_rs_height =
{
    ES_RS_ID_HEIGHT, ES_RS_ID_GROUP,
    { ES_RS_ID_CAPTION_GROUP, DIALOG_CONTROL_PARENT },
    { DIALOG_STDSPACING_H, DIALOG_STDGROUP_TM, ES_RS_FIELDS_H, DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
es_rs_height_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &es_v_units_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
es_rs_height_units =
{
    ES_RS_ID_HEIGHT_UNITS, ES_RS_ID_GROUP,
    { ES_RS_ID_HEIGHT, ES_RS_ID_HEIGHT, DIALOG_CONTROL_SELF, ES_RS_ID_HEIGHT },
    { DIALOG_BUMPUNITSGAP_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
es_rs_height_units_data = { { /*poked*/ UI_TEXT_TYPE_NONE }, { 1 /*left_text*/ } };

/*
height fixed 'buddies'
*/

static const DIALOG_CONTROL
es_rs_height_fixed_enable =
{
    ES_RS_ID_HEIGHT_FIXED_ENABLE, ES_RS_ID_CAPTION_GROUP,
    { ES_RS_ID_HEIGHT_ENABLE, ES_RS_ID_HEIGHT_FIXED, DIALOG_CONTROL_SELF, ES_RS_ID_HEIGHT_FIXED },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
es_rs_height_fixed_enable_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_RS_HEIGHT_FIXED) };

static const DIALOG_CONTROL
es_rs_height_fixed =
{
    ES_RS_ID_HEIGHT_FIXED, ES_RS_ID_GROUP,
    { ES_RS_ID_HEIGHT, ES_RS_ID_HEIGHT },
#if RISCOS
    { 0, DIALOG_STDSPACING_V, DIALOG_STDCHECK_H, DIALOG_STDEDIT_V /*NB*/},
#else
    { 0, DIALOG_STDSPACING_V, DIALOG_STDCHECK_H, DIALOG_STDCHECK_V},
#endif
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

/*
unbreakable 'buddies'
*/

static const DIALOG_CONTROL
es_rs_unbreakable_enable =
{
    ES_RS_ID_UNBREAKABLE_ENABLE, ES_RS_ID_CAPTION_GROUP,
    { ES_RS_ID_HEIGHT_FIXED_ENABLE, ES_RS_ID_UNBREAKABLE, DIALOG_CONTROL_SELF, ES_RS_ID_UNBREAKABLE },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
es_rs_unbreakable_enable_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_RS_UNBREAKABLE) };

static const DIALOG_CONTROL
es_rs_unbreakable =
{
    ES_RS_ID_UNBREAKABLE, ES_RS_ID_GROUP,
    { ES_RS_ID_HEIGHT_FIXED, ES_RS_ID_HEIGHT_FIXED },
#if RISCOS
    { 0, DIALOG_STDSPACING_V, DIALOG_STDCHECK_H, DIALOG_STDEDIT_V /*NB*/},
#else
    { 0, DIALOG_STDSPACING_V, DIALOG_STDCHECK_H, DIALOG_STDCHECK_V},
#endif
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

/*
row numform name 'buddies'
*/

static const DIALOG_CONTROL
es_rs_row_name_enable =
{
    ES_RS_ID_ROW_NAME_ENABLE, ES_RS_ID_CAPTION_GROUP,
    { ES_RS_ID_UNBREAKABLE_ENABLE, ES_RS_ID_ROW_NAME, DIALOG_CONTROL_SELF, ES_RS_ID_ROW_NAME },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOXF
es_rs_row_name_enable_data = { { { 0, 1 /*move_focus*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_RS_ROW_NAME) }, ES_RS_ID_ROW_NAME };

static const DIALOG_CONTROL
es_rs_row_name =
{
    ES_RS_ID_ROW_NAME, ES_RS_ID_GROUP,
    { ES_RS_ID_UNBREAKABLE, ES_RS_ID_UNBREAKABLE, ES_RS_ID_HEIGHT_UNITS },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V },
    { DRT(LBRT, EDIT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_EDIT
es_rs_row_name_data = { { { FRAMED_BOX_EDIT } }, /*EDIT_XX*/ { UI_TEXT_TYPE_NONE } /* UI_TEXT state */ };

/*
ok
*/

#define es_rs_ok es_ok

static const ES_DIALOG_CTL_CREATE
es_rs_ctl_create[] =
{
    { ES_ALW, { { &es_rs_group }, &es_rs_group_data } },
    { ES_ALW, { { &es_rs_caption_group }, NULL } },
    { ES_ALW, { { &es_rs_height }, &es_rs_height_data } },
    { ES_ALW, { { &es_rs_height_units }, &es_rs_height_units_data } },
    { ES_ALW, { { &es_rs_height_enable }, &es_rs_height_enable_data } },
    { ES_ALW, { { &es_rs_height_fixed }, &es_rs_height_fixed_data } },
    { ES_ALW, { { &es_rs_height_fixed_enable }, &es_rs_height_fixed_enable_data } },
    { ES_ALW, { { &es_rs_unbreakable }, &es_rs_unbreakable_data } },
    { ES_ALW, { { &es_rs_unbreakable_enable }, &es_rs_unbreakable_enable_data } },
    { ES_ALW, { { &es_rs_row_name }, &es_rs_row_name_data } },
    { ES_ALW, { { &es_rs_row_name_enable }, &es_rs_row_name_enable_data } },

    { ES_ALW, { { &es_rs_ok }, &es_ok_data } }
};

static const
struct ES_DIALOG
{
    PC_ES_DIALOG_CTL_CREATE p_es_dialog_ctl_create;
    U32 n_es_dialog_ctl_create_bytes;
    STATUS help_topic_resource_id;
}
es_subdialogs[ES_SUBDIALOG_MAX] =
{
#if    ES_SUBDIALOG_NAME != 0
#error ES_SUBDIALOG_NAME != 0
#endif
    { es_name_ctl_create, sizeof32(es_name_ctl_create), SKEL_SPLIT_MSG_DIALOG_ES_STYLE_HELP_TOPIC },
#if    ES_SUBDIALOG_FS != 1
#error ES_SUBDIALOG_FS != 1
#endif
    { es_fs_ctl_create, sizeof32(es_fs_ctl_create), SKEL_SPLIT_MSG_DIALOG_ES_FS_HELP_TOPIC },
#if    ES_SUBDIALOG_PS2 != 2
#error ES_SUBDIALOG_PS2 != 2
#endif
    { es_ps2_ctl_create, sizeof32(es_ps2_ctl_create), SKEL_SPLIT_MSG_DIALOG_ES_PS2_HELP_TOPIC },
#if    ES_SUBDIALOG_PS1 != 3
#error ES_SUBDIALOG_PS1 != 3
#endif
    { es_ps1_ctl_create, sizeof32(es_ps1_ctl_create), SKEL_SPLIT_MSG_DIALOG_ES_PS1_HELP_TOPIC },
#if    ES_SUBDIALOG_PS3 != 4
#error ES_SUBDIALOG_PS3 != 4
#endif
    { es_ps3_ctl_create, sizeof32(es_ps3_ctl_create), SKEL_SPLIT_MSG_DIALOG_ES_PS3_HELP_TOPIC },
#if    ES_SUBDIALOG_PS4 != 5
#error ES_SUBDIALOG_PS4 != 5
#endif
    { es_ps4_ctl_create, sizeof32(es_ps4_ctl_create), SKEL_SPLIT_MSG_DIALOG_ES_PS4_HELP_TOPIC },
#if    ES_SUBDIALOG_RS != 6
#error ES_SUBDIALOG_RS != 6
#endif
    { es_rs_ctl_create, sizeof32(es_rs_ctl_create), SKEL_SPLIT_MSG_DIALOG_ES_RS_HELP_TOPIC },
#if    ES_SUBDIALOG_MAX != 7
#error ES_SUBDIALOG_MAX != 7
#endif
};

/*
lhs radio buttons and lights common to all subdialogs
*/

#define ES_LIGHT(control_id) ( \
    (control_id) + (ES_ID_LIGHT_STT - ES_ID_RADIO_STT) )

#define ES_LIGHT_CONTROL_BUNDLE(control_id) \
    { DIALOG_CONTROL_PARENT, (control_id), DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF }

#define ES_LIGHT_OFFSET_BUNDLE \
    { 0, -ES_LIGHT_V_OFFSET, ES_LIGHT_H, ES_LIGHT_V }

static const DIALOG_CONTROL
es_category_group =
{
    ES_ID_CATEGORY_GROUP, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_PARENT, ES_ID_RHS_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0 },
    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

/*
name
*/

static const DIALOG_CONTROL
es_name =
{
    ES_ID_NAME, ES_ID_CATEGORY_GROUP,
    { ES_LIGHT(ES_ID_NAME), DIALOG_CONTROL_PARENT },
    { ES_LIGHTMAINSPACING_H, 0 /*-DIALOG_STDGROUP_TM*/, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(RTLT, RADIOBUTTON), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
es_name_data = { { 0 }, ES_ID_NAME, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_NAME) };

static const DIALOG_CONTROL
es_light_name =
{
    ES_LIGHT(ES_ID_NAME), ES_ID_CATEGORY_GROUP,
    ES_LIGHT_CONTROL_BUNDLE(ES_ID_NAME), ES_LIGHT_OFFSET_BUNDLE, { DRT(LTLT, USER) }
};

static const DIALOG_CONTROL_DATA_USER
es_light_name_data = { 0, { FRAMED_BOX_BUTTON_OUT, 1 /*state_is_rgb*/ } };

/*
fonts
*/

static const DIALOG_CONTROL
es_fs =
{
    ES_ID_FS, ES_ID_CATEGORY_GROUP,
    { ES_ID_NAME, ES_ID_NAME },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
es_fs_data = { { 0 }, ES_ID_FS, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_FS) };

static const DIALOG_CONTROL
es_light_fs =
{
    ES_LIGHT(ES_ID_FS), ES_ID_CATEGORY_GROUP,
    ES_LIGHT_CONTROL_BUNDLE(ES_ID_FS), ES_LIGHT_OFFSET_BUNDLE, { DRT(LTLT, USER) }
};

static const DIALOG_CONTROL_DATA_USER
es_light_fs_data = { 0, { FRAMED_BOX_BUTTON_OUT, 1 /*state_is_rgb*/ } };

/*
paragraph
*/

static const DIALOG_CONTROL
es_ps2 =
{
    ES_ID_PS2, ES_ID_CATEGORY_GROUP,
    { ES_ID_FS, ES_ID_FS },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static /*poked*/ DIALOG_CONTROL_DATA_RADIOBUTTON
es_ps2_data = { { 0 }, ES_ID_PS2, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS2) };

static const DIALOG_CONTROL
es_light_ps2 =
{
    ES_LIGHT(ES_ID_PS2), ES_ID_CATEGORY_GROUP,
    ES_LIGHT_CONTROL_BUNDLE(ES_ID_PS2), ES_LIGHT_OFFSET_BUNDLE, { DRT(LTLT, USER) }
};

static const DIALOG_CONTROL_DATA_USER
es_light_ps2_data = { 0, { FRAMED_BOX_BUTTON_OUT, 1 /*state_is_rgb*/ } };

static const DIALOG_CONTROL
es_ps1 =
{
    ES_ID_PS1, ES_ID_CATEGORY_GROUP,
    { ES_ID_PS2, ES_ID_PS2 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
es_ps1_data = { { 0 }, ES_ID_PS1, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS1) };

static const DIALOG_CONTROL
es_light_ps1 =
{
    ES_LIGHT(ES_ID_PS1), ES_ID_CATEGORY_GROUP,
    ES_LIGHT_CONTROL_BUNDLE(ES_ID_PS1), ES_LIGHT_OFFSET_BUNDLE, { DRT(LTLT, USER) }
};

static const DIALOG_CONTROL_DATA_USER
es_light_ps1_data = { 0, { FRAMED_BOX_BUTTON_OUT, 1 /*state_is_rgb*/ } };

static const DIALOG_CONTROL
es_ps3 =
{
    ES_ID_PS3, ES_ID_CATEGORY_GROUP,
    { ES_ID_PS1, ES_ID_PS1 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
es_ps3_data = { { 0 }, ES_ID_PS3, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS3) };

static const DIALOG_CONTROL
es_light_ps3 =
{
    ES_LIGHT(ES_ID_PS3), ES_ID_CATEGORY_GROUP,
    ES_LIGHT_CONTROL_BUNDLE(ES_ID_PS3), ES_LIGHT_OFFSET_BUNDLE, { DRT(LTLT, USER) }
};

static const DIALOG_CONTROL_DATA_USER
es_light_ps3_data = { 0, { FRAMED_BOX_BUTTON_OUT, 1 /*state_is_rgb*/ } };

static /*poked*/ DIALOG_CONTROL
es_ps4 =
{
    ES_ID_PS4, ES_ID_CATEGORY_GROUP,
    { ES_ID_PS3, ES_ID_PS3 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
es_ps4_data = { { 0 }, ES_ID_PS4, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_PS4) };

static const DIALOG_CONTROL
es_light_ps4 =
{
    ES_LIGHT(ES_ID_PS4), ES_ID_CATEGORY_GROUP,
    ES_LIGHT_CONTROL_BUNDLE(ES_ID_PS4), ES_LIGHT_OFFSET_BUNDLE, { DRT(LTLT, USER) }
};

static const DIALOG_CONTROL_DATA_USER
es_light_ps4_data = { 0, { FRAMED_BOX_BUTTON_OUT, 1 /*state_is_rgb*/ } };

/*
row info
*/

static /*poked*/ DIALOG_CONTROL
es_rs =
{
    ES_ID_RS, ES_ID_CATEGORY_GROUP,
    { ES_ID_PS4, ES_ID_PS4 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
es_rs_data = { { 0 }, ES_ID_RS, UI_TEXT_INIT_RESID(MSG_DIALOG_ES_RS) };

static const DIALOG_CONTROL
es_light_rs =
{
    ES_LIGHT(ES_ID_RS), ES_ID_CATEGORY_GROUP,
    ES_LIGHT_CONTROL_BUNDLE(ES_ID_RS), ES_LIGHT_OFFSET_BUNDLE, { DRT(LTLT, USER) }
};

static const DIALOG_CONTROL_DATA_USER
es_light_rs_data = { 0, { FRAMED_BOX_BUTTON_OUT, 1 /*state_is_rgb*/ } };

static const ES_DIALOG_CTL_CREATE
es_ctl_1_create[] =
{
    { ES_ALW, { { &es_rhs_group }, NULL } }
};

static const ES_DIALOG_CTL_CREATE
es_ctl_2_create[] =
{
    { ES_ALW, { { &es_cancel }, &es_cancel_data } }, /* always just after the OK button */

    { ES_ALW, { { &es_category_group }, NULL } },

    { ES_ALW, { { &es_name }, &es_name_data } },
    { ES_ALW, { { &es_light_name }, &es_light_name_data } },

    { ES_ALW, { { &es_fs }, &es_fs_data } },
    { ES_ALW, { { &es_light_fs }, &es_light_fs_data } },

    { ES_ALW, { { &es_ps2 }, &es_ps2_data } },
    { ES_ALW, { { &es_light_ps2 }, &es_light_ps2_data } },

    { ES_ALW, { { &es_ps1 }, &es_ps1_data } },
    { ES_ALW, { { &es_light_ps1 }, &es_light_ps1_data } },

    { ES_ATX, { { &es_ps3 }, &es_ps3_data } },
    { ES_ATX, { { &es_light_ps3 }, &es_light_ps3_data } },

    { ES_NUM, { { &es_ps4 }, &es_ps4_data } },
    { ES_NUM, { { &es_light_ps4 }, &es_light_ps4_data } },

    { ES_ALW, { { &es_rs }, &es_rs_data } },
    { ES_ALW, { { &es_light_rs }, &es_light_rs_data } }
};

/*extern*/ S32
es_subdialog_current = ES_SUBDIALOG_FS;

static void
user_unit_setup(
    _DocuRef_   P_DOCU p_docu,
    P_ES_USER_UNIT_INFO p_user_unit_info,
    _InVal_     UINT idx)
{
    SCALE_INFO scale_info;
    DISPLAY_UNIT_INFO display_unit_info;
    STATUS numform_resource_id_normal;
    STATUS numform_resource_id_fine;

    scale_info_from_docu(p_docu, (idx != IDX_VERT), &scale_info);

    /* allow bumping to finest usable resolution */
    p_user_unit_info->normal.user_unit_multiple = ((F64) scale_info.coarse_div * scale_info.fine_div) / scale_info.numbered_units_multiplier;

    display_unit_info_from_display_unit(&display_unit_info, scale_info.display_unit);

    p_user_unit_info->fp_pixits_per_user_unit = display_unit_info.fp_pixits_per_unit;

    switch(scale_info.display_unit)
    {
    default: default_unhandled();
#if CHECKING
    case DISPLAY_UNIT_CM:
#endif
        p_user_unit_info->user_unit_resource_id = MSG_DIALOG_UNITS_CM;
        numform_resource_id_normal = MSG_DIALOG_STYLE_NUMFORM_CM;
        break;

    case DISPLAY_UNIT_MM:
        p_user_unit_info->user_unit_resource_id = MSG_DIALOG_UNITS_MM;
        numform_resource_id_normal = MSG_DIALOG_STYLE_NUMFORM_MM;
        break;

    case DISPLAY_UNIT_INCHES:
        p_user_unit_info->user_unit_resource_id = MSG_DIALOG_UNITS_INCHES;
        numform_resource_id_normal = MSG_DIALOG_STYLE_NUMFORM_INCHES;
        break;

    case DISPLAY_UNIT_POINTS:
        p_user_unit_info->user_unit_resource_id = MSG_DIALOG_UNITS_POINTS;
        numform_resource_id_normal = MSG_DIALOG_STYLE_NUMFORM_POINTS;
        break;
    }

    resource_lookup_ustr_buffer(ustr_bptr(p_user_unit_info->normal.user_unit_numform_ustr_buf), elemof32(p_user_unit_info->normal.user_unit_numform_ustr_buf), numform_resource_id_normal);

    p_user_unit_info->fine = p_user_unit_info->normal;

    switch(scale_info.display_unit)
    {
    default: default_unhandled();
#if CHECKING
    case DISPLAY_UNIT_CM:
#endif
        p_user_unit_info->fine.user_unit_multiple = p_user_unit_info->normal.user_unit_multiple * 10;
        numform_resource_id_fine = MSG_DIALOG_STYLE_NUMFORM_CM_FINE;
        break;

    case DISPLAY_UNIT_MM:
        p_user_unit_info->fine.user_unit_multiple = p_user_unit_info->normal.user_unit_multiple * 10;
        numform_resource_id_fine = MSG_DIALOG_STYLE_NUMFORM_MM_FINE;
        break;

    case DISPLAY_UNIT_INCHES:
    case DISPLAY_UNIT_POINTS:
        numform_resource_id_fine = 0; /* same as normal case */
        break;
    }

    if(0 != numform_resource_id_fine)
        resource_lookup_ustr_buffer(ustr_bptr(p_user_unit_info->fine.user_unit_numform_ustr_buf), elemof32(p_user_unit_info->fine.user_unit_numform_ustr_buf), numform_resource_id_fine);
}

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
    _InVal_     STYLE_HANDLE style_handle_being_modified /*i.e. not effects*/,
    _InVal_     S32 subdialog /*-1 -> whichever was last */)
{
    STATUS status;
    ES_CALLBACK escb; zero_struct_fn(escb);

    style_init(p_style_out);

    escb.atx = 1; /*advanced_text_processing*/

    escb.num = 1; /*switch(g_product_id)*/

    escb.has_colour_picker = g_has_colour_picker;

    escb.hwnd_parent = hwnd_parent;
    escb.style_handle_being_modified = style_handle_being_modified;

    es_tweak_style_init(&escb);

    style_init(&escb.committed_style);

    if(NULL != p_style_modified)
        style_selector_clear(p_style_modified);

    if(NULL != p_style_selector_modified)
        style_selector_clear(p_style_selector_modified);

    if(NULL != p_prohibited_enabler)
        style_selector_copy(&escb.prohibited_enabler, p_prohibited_enabler);

    if(NULL != p_prohibited_enabler_2)
        style_selector_copy(&escb.prohibited_enabler_2, p_prohibited_enabler_2);

    /* duplicate the style we are editing */
    status_return(style_duplicate(&escb.committed_style, p_style_in, &p_style_in->selector));

    /* initially have as enabled all that we currently own and are editing - see below */
    void_style_selector_and(&escb.committed_style_selector, &escb.committed_style.selector, p_style_selector);

    /* nothing modified so far */
    style_selector_clear(&escb.committed_style_modified);
    style_selector_clear(&escb.committed_style_selector_modified);

    if(status_ok(status = ui_text_copy(&escb.caption, p_caption)))
    {
        /* decide which to go for, default being what we have now */
        if(subdialog != -1)
            es_subdialog_current = subdialog;

        if(!es_subdialog_style_selector_test(&escb.committed_style_selector, es_subdialog_current))
        {
            UINT i;

            for(i = 1; i < ES_SUBDIALOG_MAX; ++i)
                if(es_subdialog_style_selector_test(&escb.committed_style_selector, i))
                {
                    es_subdialog_current = i;
                    break;
                }
        }

        escb.subdialog_current = es_subdialog_current;

        status = es_subdialog_process(p_docu, &escb);

        if(status_ok(status))
        {
            /* donate style to caller, first freeing resources that we currently own but won't be giving back */
            STYLE_SELECTOR selector;

            void_style_selector_not(&selector, &escb.committed_style_selector);

            style_free_resources(&escb.committed_style, &selector);

            *p_style_out = escb.committed_style;
            style_selector_copy(&p_style_out->selector, &escb.committed_style_selector);

            /* if he's interested, tell client which bits changed */
            if(NULL != p_style_modified)
                style_selector_copy(p_style_modified,          &escb.committed_style_modified);

            if(NULL != p_style_selector_modified)
                style_selector_copy(p_style_selector_modified, &escb.committed_style_selector_modified);

            style_init(&escb.committed_style);
        }

        ui_text_dispose(&escb.caption);
    }

    style_free_resources_all(&escb.committed_style);

    return(status);
}

_Check_return_
static STATUS
es_subdialog_process(
    _DocuRef_   P_DOCU p_docu,
    P_ES_CALLBACK p_es_callback)
{
    /* biggest so far is the colours box, ca. 3*33 + ca. 20 */
#if RISCOS && 0
    static
#endif
    DIALOG_CTL_CREATE es_subdialog_ctl_create[120 /*increase me if controls overflow - watch for assert() going bang*/];

    STATUS status;

    resource_lookup_ustr_buffer(ustr_bptr(es_points_numform_ustr_buf), elemof32(es_points_numform_ustr_buf), MSG_NUMFORM_2_DP);

    for(;;)
    {
        S32 n_ctls = 0;

        style_init(&p_es_callback->style);

        /* style copied in as needed during individual control creation */
        style_selector_clear(&p_es_callback->style_selector);

        {
        DIALOG_CTL_CREATE * p_dialog_ctl_create = es_subdialog_ctl_create;
        U32 i;

        /* reflect current measurement systems in dialog */
        for(i = 0; i < elemof32(p_es_callback->info); ++i)
            user_unit_setup(p_docu, &p_es_callback->info[i], i);

        for(i = 0; i < elemof32(es_ctl_1_create); ++i)
        {
            PC_ES_DIALOG_CTL_CREATE p_es_dialog_ctl_create = &es_ctl_1_create[i];

            p_dialog_ctl_create->p_dialog_control = p_es_dialog_ctl_create->dialog_ctl_create.p_dialog_control;
            p_dialog_ctl_create->p_dialog_control_data = p_es_dialog_ctl_create->dialog_ctl_create.p_dialog_control_data;

            ++p_dialog_ctl_create;
            ++n_ctls;
        }

        for(i = 0; i < (U32) es_subdialogs[p_es_callback->subdialog_current].n_es_dialog_ctl_create_bytes / sizeof32(ES_DIALOG_CTL_CREATE); ++i)
        {
            PC_ES_DIALOG_CTL_CREATE p_es_dialog_ctl_create = &es_subdialogs[p_es_callback->subdialog_current].p_es_dialog_ctl_create[i];

            if(p_es_dialog_ctl_create->flags & ES_ATX)
                if(!p_es_callback->atx)
                    continue;

            if(p_es_dialog_ctl_create->flags & ES_NUM)
                if(!p_es_callback->num)
                    continue;

            if(p_es_dialog_ctl_create->flags & ES_NRO)
                if(!p_es_callback->has_colour_picker)
                    continue;

            p_dialog_ctl_create->p_dialog_control = p_es_dialog_ctl_create->dialog_ctl_create.p_dialog_control;
            p_dialog_ctl_create->p_dialog_control_data = p_es_dialog_ctl_create->dialog_ctl_create.p_dialog_control_data;

            ++p_dialog_ctl_create;
            ++n_ctls;
        }

        for(i = 0; i < elemof32(es_ctl_2_create); ++i)
        {
            PC_ES_DIALOG_CTL_CREATE p_es_dialog_ctl_create = &es_ctl_2_create[i];

            if(p_es_dialog_ctl_create->flags & ES_ATX)
                if(!p_es_callback->atx)
                    continue;

            if(p_es_dialog_ctl_create->flags & ES_NUM)
                if(!p_es_callback->num)
                    continue;

            if(p_es_dialog_ctl_create->flags & ES_NRO)
                if(!p_es_callback->has_colour_picker)
                    continue;

            p_dialog_ctl_create->p_dialog_control = p_es_dialog_ctl_create->dialog_ctl_create.p_dialog_control;
            p_dialog_ctl_create->p_dialog_control_data = p_es_dialog_ctl_create->dialog_ctl_create.p_dialog_control_data;

            ++p_dialog_ctl_create;
            ++n_ctls;
        }

        es_ps2_data.caption.text.resource_id = p_es_callback->atx ? MSG_DIALOG_ES_PS2 : MSG_DIALOG_ES_PS2_CELL;

        /* unfortunate explicit knowledge to remove ps3 and/or ps4 groups entirely */
        es_ps4.relative_dialog_control_id[0] =
        es_ps4.relative_dialog_control_id[1] =
            (DIALOG_CONTROL_ID) (p_es_callback->atx ? ES_ID_PS3 : es_ps3.relative_dialog_control_id[0]);

        es_rs.relative_dialog_control_id[0] =
        es_rs.relative_dialog_control_id[1] =
            (DIALOG_CONTROL_ID) (p_es_callback->num ? ES_ID_PS4 : es_ps4.relative_dialog_control_id[0]);

        /* and also for individual controls */
        es_ps_margin_right.relative_dialog_control_id[1] = ES_PS_ID_MARGIN_PARA;
        if(!p_es_callback->atx)
            es_ps_margin_right.relative_dialog_control_id[1] = ES_PS_ID_MARGIN_LEFT;

        es_ps_tab_list_group.relative_dialog_control_id[1] = ES_PS_ID_VERT_JUSTIFY_GROUP_MAIN;
        if(!p_es_callback->atx)
            es_ps_tab_list_group.relative_dialog_control_id[1] = ES_PS_ID_HORZ_JUSTIFY_GROUP_MAIN;
        } /*block*/

        assert(n_ctls <= elemof32(es_subdialog_ctl_create));
        {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup_ui_text(&dialog_cmd_process_dbox, es_subdialog_ctl_create, n_ctls, &p_es_callback->caption);
        dialog_cmd_process_dbox.help_topic_resource_id = es_subdialogs[p_es_callback->subdialog_current].help_topic_resource_id;
        dialog_cmd_process_dbox.bits.note_position = 1;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_tweak_style;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_es_callback;
        es_tweak_style_precreate(p_es_callback, &dialog_cmd_process_dbox);
#if RISCOS
        if(g_has_colour_picker)
        {   /* this needs to be addressed */
            dialog_cmd_process_dbox.bits.use_riscos_menu = 1;
            dialog_cmd_process_dbox.riscos.menu = DIALOG_RISCOS_NOT_MENU;
        }
#endif
#if WINDOWS
        if(p_es_callback->hwnd_parent)
        {
            dialog_cmd_process_dbox.windows.hwnd = p_es_callback->hwnd_parent;
            dialog_cmd_process_dbox.bits.use_windows_hwnd = 1;
        }
#endif
        status_break(status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
        }

        if(status == ES_ID_REVERT)
        {
            /* local cancel, reprocess once again at this level */
            style_free_resources_all(&p_es_callback->style);
            continue;
        }

        /* don't copy back empty style names */
        if(style_selector_bit_test(&p_es_callback->style_selector, STYLE_SW_NAME))
        {
            PCTSTR tstr = array_tstr(&p_es_callback->style.h_style_name_tstr);

            if(!tstr || !*tstr)
            {
                /* effects, however, generally do have blank names */
                if(p_es_callback->style_handle_being_modified)
                    reperr_null(ERR_STYLE_MUST_HAVE_NAME);

                style_selector_bit_clear(&p_es_callback->style_modified, STYLE_SW_NAME);
            }

            /* also check for clash */
            if(p_es_callback->style_handle_being_modified)
            {
                STYLE_HANDLE style_handle = style_handle_from_name(p_docu, tstr);

                if((0 != style_handle) && (style_handle != p_es_callback->style_handle_being_modified))
                {
                    reperr_null(ERR_STYLE_WITH_SAME_NAME);

                    style_selector_bit_clear(&p_es_callback->style_modified, STYLE_SW_NAME);
                }
            }
        }

        /* don't copy back unselected shortcuts */
        if(style_selector_bit_test(&p_es_callback->style_selector, STYLE_SW_KEY))
        {
            if(!p_es_callback->style.style_key)
            {
                assert0();
                style_selector_bit_clear(&p_es_callback->style.selector, STYLE_SW_KEY);
                style_selector_bit_clear(&p_es_callback->style_selector, STYLE_SW_KEY);
            }
        }

        /* replace bits in committed style with those that have been modified in subdialog edit
         * (NB. this is a donation - don't dispose of this now!)
        */
        style_free_resources(&p_es_callback->committed_style, &p_es_callback->style_modified);
        style_copy(&p_es_callback->committed_style, &p_es_callback->style, &p_es_callback->style_modified);

        /* edited style no longer owns those donated resources; free the ones we didn't modify */
        void_style_selector_bic(&p_es_callback->style.selector, &p_es_callback->style.selector, &p_es_callback->style_modified);
        style_free_resources_all(&p_es_callback->style);

        /* committed style now has had these bits modified */
        void_style_selector_or(&p_es_callback->committed_style_modified, &p_es_callback->committed_style_modified, &p_es_callback->style_modified);

        { /* replace bits in committed style enable with those that have been modified in subdialog edit */
        STYLE_SELECTOR selector;
        void_style_selector_bic(&p_es_callback->committed_style_selector, &p_es_callback->committed_style_selector, &p_es_callback->style_selector_modified);
        void_style_selector_and(&selector, &p_es_callback->style_selector, &p_es_callback->style_selector_modified);
        void_style_selector_or(&p_es_callback->committed_style_selector, &p_es_callback->committed_style_selector, &selector);
        } /*block*/

        /* committed style selector now has had these bits modified */
        void_style_selector_or(&p_es_callback->committed_style_selector_modified, &p_es_callback->committed_style_selector_modified, &p_es_callback->style_selector_modified);

        if(status == ES_ID_SET)
            /* process the same subdialog again once we have got back to this level */
            continue;

        /* bog standard completion? */
        if((status <= 1) || (status == ES_ID_ADJUST_APPLY))
            break;

        status -= ES_ID_RADIO_STT;

        /* process another subdialog once we have got back to this level */
        es_subdialog_current = p_es_callback->subdialog_current = (S32) status;
    }

    if(status_fail(status))
    {
        style_free_resources_all(&p_es_callback->style);

        if(status == STATUS_FAIL)
            status = STATUS_OK;
    }

    return(status);
}

/* end of ui_styl2.c */
