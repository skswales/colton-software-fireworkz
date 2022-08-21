/* ui_page.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1995-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Page editing UI for Fireworkz */

/* SKS April 1995 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

#if WINDOWS
#include "direct.h"
#endif

/******************************************************************************
*
* paper scale
*
******************************************************************************/

enum PAPER_SCALE_CONTROL_IDS
{
    PAPER_SCALE_ID_FIT = 993,
    PAPER_SCALE_ID_FIT_X,
    PAPER_SCALE_ID_100,
#if WINDOWS
#define PAPER_SCALE_BUTTON_H (DIALOG_STDCANCEL_H + DIALOG_STDSPACING_H + DIALOG_DEFOK_H)
#else
#define PAPER_SCALE_BUTTON_H (DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARS_H("Fit one page X"))
#endif

    PAPER_SCALE_ID_VALUE,
    PAPER_SCALE_ID_LABEL,
    PAPER_SCALE_ID_UNITS,

    PAPER_SCALE_ID_MAX
};

static const DIALOG_CONTROL
paper_scale_fit =
{
    PAPER_SCALE_ID_FIT, DIALOG_MAIN_GROUP,
#if RISCOS
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, PAPER_SCALE_ID_UNITS },
    { 0, 0, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LTRT, PUSHBUTTON), 1 /*tabstop*/ }
#else
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, PAPER_SCALE_BUTTON_H, DIALOG_STDPUSHBUTTON_V },
    { DRT(LTLT, PUSHBUTTON), 1 /*tabstop*/ }
#endif
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
paper_scale_fit_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_SCALE_FIT) };

static const DIALOG_CONTROL
paper_scale_fit_x =
{
    PAPER_SCALE_ID_FIT_X, DIALOG_MAIN_GROUP,
    { PAPER_SCALE_ID_FIT, PAPER_SCALE_ID_FIT, PAPER_SCALE_ID_FIT, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
paper_scale_fit_x_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_SCALE_FIT_X) };

static const DIALOG_CONTROL
paper_scale_100 =
{
    PAPER_SCALE_ID_100, DIALOG_MAIN_GROUP,
    { PAPER_SCALE_ID_FIT_X, PAPER_SCALE_ID_FIT_X, PAPER_SCALE_ID_FIT_X, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
paper_scale_100_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_SCALE_100) };

static const DIALOG_CONTROL
paper_scale_label =
{
    PAPER_SCALE_ID_LABEL, DIALOG_MAIN_GROUP,
    { PAPER_SCALE_ID_100, PAPER_SCALE_ID_VALUE, DIALOG_CONTROL_SELF, PAPER_SCALE_ID_VALUE },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
paper_scale_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_SCALE) };

static const DIALOG_CONTROL
paper_scale_value =
{
    PAPER_SCALE_ID_VALUE, DIALOG_MAIN_GROUP,
    { PAPER_SCALE_ID_LABEL, PAPER_SCALE_ID_100 },
    { DIALOG_LABELGAP_H, DIALOG_STDSPACING_V, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },
    { DRT(RBLT, BUMP_S32), 1 /*tabstop*/ }
};

static const UI_CONTROL_S32
paper_scale_value_control = { 5, 1000, 5 };

static const DIALOG_CONTROL_DATA_BUMP_S32
paper_scale_value_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &paper_scale_value_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
paper_scale_units =
{
    PAPER_SCALE_ID_UNITS, DIALOG_MAIN_GROUP,
    { PAPER_SCALE_ID_VALUE, PAPER_SCALE_ID_VALUE, DIALOG_CONTROL_SELF, PAPER_SCALE_ID_VALUE },
    { DIALOG_BUMPUNITSGAP_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
paper_scale_units_data = { UI_TEXT_INIT_RESID(MSG_PERCENT), { 1 /*left_text*/ } };

/*
ok
*/

static const DIALOG_CONTROL_ID
paper_scale_ok_data_argmap[] =
{
    PAPER_SCALE_ID_VALUE,
};

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
paper_scale_ok_command = { T5_CMD_PAPER_SCALE, OBJECT_ID_SKEL, NULL, paper_scale_ok_data_argmap, { 0 /*set_view*/, 0, 0, 1 /*lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
paper_scale_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_BUTTON_APPLY), &paper_scale_ok_command };

static const DIALOG_CTL_CREATE
paper_scale_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &paper_scale_fit },   &paper_scale_fit_data },
    { { &paper_scale_fit_x }, &paper_scale_fit_x_data },
    { { &paper_scale_100 },   &paper_scale_100_data },

    { { &paper_scale_label }, &paper_scale_label_data },
    { { &paper_scale_value }, &paper_scale_value_data },
    { { &paper_scale_units }, &paper_scale_units_data },

    { { &defbutton_ok },      &paper_scale_ok_data },
    { { &stdbutton_cancel },  &stdbutton_cancel_data }
};

_Check_return_
static STATUS
dialog_paper_scale_process_start(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    return(ui_dlg_set_s32(p_dialog_msg_process_start->h_dialog, PAPER_SCALE_ID_VALUE, p_docu->paper_scale));
}

_Check_return_
static STATUS
dialog_paper_scale_ctl_pushbutton_fit(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id)
{
    PIXIT_POINT paper_size, docu_size;
    HEADFOOT_BOTH headfoot_both;
    PAGE_DEF page_def = p_docu->page_def;
    S32 scale;

    page_def_from_phys_page_def(&page_def, &p_docu->phys_page_def, 100);
    page_def_validate(&page_def);

    paper_size.x = page_def.cells_usable_x;
    paper_size.y = page_def.cells_usable_y;

    paper_size.x += page_def.margin_row;
    paper_size.y += page_def.margin_col;

    docu_size.x = non_blank_width(p_docu);
    docu_size.y = non_blank_height(p_docu);

    /* SKS 10nov93 after 1.06 - don't set up really silly values when there's no real data */
    if((docu_size.x == 0) || (docu_size.y == 0))
        return(STATUS_OK);

    docu_size.x += page_def.margin_row;
    docu_size.y += page_def.margin_col;

    docu_size.x += page_def.grid_size;
    docu_size.y += page_def.grid_size;

    headfoot_both_from_page_y(p_docu, &headfoot_both, 0);
    docu_size.y += (headfoot_both.header.margin + headfoot_both.footer.margin);

    docu_size.x = MAX(docu_size.x, PIXITS_PER_INCH / 2);
    docu_size.y = MAX(docu_size.y, PIXITS_PER_INCH / 2);

#define SMALL_PIXIT_FUDGE_TO_ADD 2 /* always make bias scaling so that columns don't just fall off the right */

    scale = (100 * paper_size.x) / (docu_size.x + SMALL_PIXIT_FUDGE_TO_ADD);

    if(PAPER_SCALE_ID_FIT == dialog_control_id)
    {
        S32 scale_y = (100 * paper_size.y) / (docu_size.y + SMALL_PIXIT_FUDGE_TO_ADD);

        scale = MIN(scale, scale_y);
    }

    return(ui_dlg_set_s32(h_dialog, PAPER_SCALE_ID_VALUE, scale));
}

_Check_return_
static STATUS
dialog_paper_scale_ctl_pushbutton(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    const DIALOG_CONTROL_ID dialog_control_id = p_dialog_msg_ctl_pushbutton->dialog_control_id;

    switch(dialog_control_id)
    {
    case PAPER_SCALE_ID_FIT:
    case PAPER_SCALE_ID_FIT_X:
        return(dialog_paper_scale_ctl_pushbutton_fit(p_docu, p_dialog_msg_ctl_pushbutton->h_dialog, dialog_control_id));

    case PAPER_SCALE_ID_100:
        return(ui_dlg_set_s32(p_dialog_msg_ctl_pushbutton->h_dialog, PAPER_SCALE_ID_VALUE, 100));

    default:
        /* other things do come through here, so ignore them */
        return(STATUS_OK);
    }
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_paper_scale)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_paper_scale_process_start(p_docu, (PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_paper_scale_ctl_pushbutton(p_docu, (P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(extern, t5_cmd_paper_scale_intro)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, paper_scale_ctl_create, elemof32(paper_scale_ctl_create), MSG_DIALOG_PAPER_SCALE_CAPTION);
    dialog_cmd_process_dbox.help_topic_resource_id = MSG_DIALOG_PAPER_SCALE_HELP_TOPIC;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_paper_scale;
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

/******************************************************************************
*
* edit paper info
*
******************************************************************************/

static FP_PIXIT
paper_fp_pixits_per_user_unit = FP_PIXITS_PER_MM;

_Check_return_
static STATUS
paper_user_unit_resource_id = MSG_DIALOG_UNITS_MM;

static /*poked*/ UI_CONTROL_F64
paper_units_control;

static /*poked*/ UI_CONTROL_F64
paper_units_fine_control;

enum PAPER_CONTROL_IDS
{
    PAPER_ID_PAPER_GROUP = 110,

    PAPER_ID_PAPER_LABELS_GROUP, /* for sizing */

    PAPER_ID_NAME,
    PAPER_ID_NAME_LABEL,

    PAPER_ID_HEIGHT,
    PAPER_ID_HEIGHT_LABEL,
    PAPER_ID_HEIGHT_UNITS,

    PAPER_ID_WIDTH,
    PAPER_ID_WIDTH_LABEL,
    PAPER_ID_WIDTH_UNITS,
#define PAPER_LABELS_H (DIALOG_SYSCHARS_H("olumn margin") + DIALOG_FATCHAR_H)

    PAPER_ID_PAPER_MARGIN_GROUP = 130,

    PAPER_ID_PAPER_MARGIN_LABELS_GROUP, /* for sizing */

    PAPER_ID_MARGIN_LEFT,
    PAPER_ID_MARGIN_LEFT_LABEL,
    PAPER_ID_MARGIN_LEFT_UNITS,

    PAPER_ID_MARGIN_RIGHT,
    PAPER_ID_MARGIN_RIGHT_LABEL,
    PAPER_ID_MARGIN_RIGHT_UNITS,

    PAPER_ID_MARGIN_TOP,
    PAPER_ID_MARGIN_TOP_LABEL,
    PAPER_ID_MARGIN_TOP_UNITS,

    PAPER_ID_MARGIN_BOTTOM,
    PAPER_ID_MARGIN_BOTTOM_LABEL,
    PAPER_ID_MARGIN_BOTTOM_UNITS,

    PAPER_ID_BINDING_GROUP = 150,

    PAPER_ID_MARGIN_BIND,
    PAPER_ID_MARGIN_BIND_LABEL,
    PAPER_ID_MARGIN_BIND_UNITS,

    PAPER_ID_MARGIN_OE_SWAP,

    PAPER_ID_MARGIN_COL = 170,
    PAPER_ID_MARGIN_COL_LABEL,
    PAPER_ID_MARGIN_COL_UNITS,

    PAPER_ID_MARGIN_ROW,
    PAPER_ID_MARGIN_ROW_LABEL,
    PAPER_ID_MARGIN_ROW_UNITS,

    PAPER_ID_ORIENTATION_GROUP = 190,
    PAPER_ID_ORIENTATION_P,
    PAPER_ID_ORIENTATION_L,

    PAPER_ID_BUTTON_READ,
    PAPER_ID_BUTTON_NONE,
    PAPER_ID_BUTTON_1,
    PAPER_ID_BUTTON_2,
    PAPER_ID_BUTTON_3,
    PAPER_ID_BUTTON_4,
    PAPER_ID_BUTTON_5,
    PAPER_ID_BUTTON_6,

#define PAPER_BUTTONS_H (PIXITS_PER_INCH +  (PIXITS_PER_INCH * 4) / 16)

    PAPER_ID_GRID_GROUP,
    PAPER_ID_GRID_SIZE,
    PAPER_ID_GRID_SIZE_LABEL,
    PAPER_ID_GRID_SIZE_UNITS,
    PAPER_ID_GRID_FAINT,

    PAPER_ID_MAX
};

/*
paper margin group
*/

static const DIALOG_CONTROL
paper_paper_group =
{
    PAPER_ID_PAPER_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
paper_paper_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_PAPER), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
paper_paper_labels_group =
{
    PAPER_ID_PAPER_LABELS_GROUP, PAPER_ID_PAPER_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDGROUP_LM },
    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

/*
paper name 'buddies'
*/

static const DIALOG_CONTROL
paper_name_label =
{
    PAPER_ID_NAME_LABEL, PAPER_ID_PAPER_LABELS_GROUP,
    { DIALOG_CONTROL_PARENT, PAPER_ID_NAME, DIALOG_CONTROL_SELF, PAPER_ID_NAME },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
paper_name_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_NAME), { 1/*left_text*/ } }; /* left-aligned as we use vertical guideline here */

static const DIALOG_CONTROL
paper_name =
{
    PAPER_ID_NAME, PAPER_ID_PAPER_GROUP,
    { PAPER_ID_PAPER_MARGIN_LABELS_GROUP, DIALOG_CONTROL_PARENT, PAPER_ID_HEIGHT_UNITS },
    { DIALOG_LABELGAP_H, DIALOG_STDGROUP_TM, 0, DIALOG_STDEDIT_V },
    { DRT(RTRT, EDIT), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_EDIT
paper_name_data = { { { FRAMED_BOX_EDIT } }, /*EDIT_XX*/ { UI_TEXT_TYPE_NONE } /* UI_TEXT state */ };

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
paper_units_data = { { UI_TEXT_TYPE_NONE }, { 1 /*left_text*/ } };

/*
height 'buddies'
*/

static const DIALOG_CONTROL
paper_height_label =
{
    PAPER_ID_HEIGHT_LABEL, PAPER_ID_PAPER_LABELS_GROUP,
    { DIALOG_CONTROL_PARENT, PAPER_ID_HEIGHT, DIALOG_CONTROL_SELF, PAPER_ID_HEIGHT },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
paper_height_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_HEIGHT), { 1/*left_text*/ } };

static const DIALOG_CONTROL
paper_height =
{
    PAPER_ID_HEIGHT, PAPER_ID_PAPER_GROUP,
    { PAPER_ID_NAME, PAPER_ID_NAME },
    { 0, DIALOG_STDSPACING_V, DIALOG_BUMP_H(6), DIALOG_STDBUMP_V },
    { DRT(LBLT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
paper_height_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &paper_units_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
paper_height_units =
{
    PAPER_ID_HEIGHT_UNITS, PAPER_ID_PAPER_GROUP,
    { PAPER_ID_HEIGHT, PAPER_ID_HEIGHT, DIALOG_CONTROL_SELF, PAPER_ID_HEIGHT },
    { DIALOG_BUMPUNITSGAP_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

/*
width 'buddies'
*/

static const DIALOG_CONTROL
paper_width_label =
{
    PAPER_ID_WIDTH_LABEL, PAPER_ID_PAPER_LABELS_GROUP,
    { DIALOG_CONTROL_PARENT, PAPER_ID_WIDTH, DIALOG_CONTROL_SELF, PAPER_ID_WIDTH },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
paper_width_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_WIDTH), { 1/*left_text*/ } };

static const DIALOG_CONTROL
paper_width =
{
    PAPER_ID_WIDTH, PAPER_ID_PAPER_GROUP,
    { PAPER_ID_HEIGHT, PAPER_ID_HEIGHT, PAPER_ID_HEIGHT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
paper_width_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &paper_units_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
paper_width_units =
{
    PAPER_ID_WIDTH_UNITS, PAPER_ID_PAPER_GROUP,
    { PAPER_ID_HEIGHT_UNITS, PAPER_ID_WIDTH, PAPER_ID_HEIGHT_UNITS, PAPER_ID_WIDTH },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

/*
paper margin group
*/

static const DIALOG_CONTROL
paper_paper_margin_group =
{
    PAPER_ID_PAPER_MARGIN_GROUP, DIALOG_MAIN_GROUP,
    { PAPER_ID_PAPER_GROUP, PAPER_ID_PAPER_GROUP, PAPER_ID_PAPER_GROUP, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
paper_paper_margin_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_PAPER_MARGIN), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
paper_paper_margin_labels_group =
{
    PAPER_ID_PAPER_MARGIN_LABELS_GROUP, PAPER_ID_PAPER_MARGIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDGROUP_LM },
    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

/*
top margin 'buddies'
*/

static const DIALOG_CONTROL
paper_margin_top_label =
{
    PAPER_ID_MARGIN_TOP_LABEL, PAPER_ID_PAPER_MARGIN_LABELS_GROUP,
    { DIALOG_CONTROL_PARENT, PAPER_ID_MARGIN_TOP, DIALOG_CONTROL_SELF, PAPER_ID_MARGIN_TOP },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
paper_margin_top_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_MARGIN_TOP), { 1/*left_text*/ } };

static const DIALOG_CONTROL
paper_margin_top =
{
    PAPER_ID_MARGIN_TOP, PAPER_ID_PAPER_MARGIN_GROUP,
    { PAPER_ID_PAPER_MARGIN_LABELS_GROUP, DIALOG_CONTROL_PARENT, PAPER_ID_WIDTH },
    { DIALOG_LABELGAP_H, DIALOG_STDGROUP_TM, 0, DIALOG_STDBUMP_V },
    { DRT(RTRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
paper_margin_top_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &paper_units_fine_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
paper_margin_top_units =
{
    PAPER_ID_MARGIN_TOP_UNITS, PAPER_ID_PAPER_MARGIN_GROUP,
    { PAPER_ID_MARGIN_TOP, PAPER_ID_MARGIN_TOP, DIALOG_CONTROL_SELF, PAPER_ID_MARGIN_TOP },
    { DIALOG_BUMPUNITSGAP_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

/*
bottom margin 'buddies'
*/

static const DIALOG_CONTROL
paper_margin_bottom_label =
{
    PAPER_ID_MARGIN_BOTTOM_LABEL, PAPER_ID_PAPER_MARGIN_LABELS_GROUP,
    { DIALOG_CONTROL_PARENT, PAPER_ID_MARGIN_BOTTOM, DIALOG_CONTROL_SELF, PAPER_ID_MARGIN_BOTTOM },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
paper_margin_bottom_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_MARGIN_BOTTOM), { 1/*left_text*/ } };

static const DIALOG_CONTROL
paper_margin_bottom =
{
    PAPER_ID_MARGIN_BOTTOM, PAPER_ID_PAPER_MARGIN_GROUP,
    { PAPER_ID_MARGIN_TOP, PAPER_ID_MARGIN_TOP, PAPER_ID_MARGIN_TOP },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
paper_margin_bottom_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &paper_units_fine_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
paper_margin_bottom_units =
{
    PAPER_ID_MARGIN_BOTTOM_UNITS, PAPER_ID_PAPER_MARGIN_GROUP,
    { PAPER_ID_MARGIN_TOP_UNITS, PAPER_ID_MARGIN_BOTTOM, PAPER_ID_MARGIN_TOP_UNITS, PAPER_ID_MARGIN_BOTTOM },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

/*
left margin 'buddies'
*/

static const DIALOG_CONTROL
paper_margin_left_label =
{
    PAPER_ID_MARGIN_LEFT_LABEL, PAPER_ID_PAPER_MARGIN_LABELS_GROUP,
    { DIALOG_CONTROL_PARENT, PAPER_ID_MARGIN_LEFT, DIALOG_CONTROL_SELF, PAPER_ID_MARGIN_LEFT },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
paper_margin_left_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_MARGIN_LEFT), { 1/*left_text*/ } };

static const DIALOG_CONTROL
paper_margin_left =
{
    PAPER_ID_MARGIN_LEFT, PAPER_ID_PAPER_MARGIN_GROUP,
    { PAPER_ID_MARGIN_BOTTOM, PAPER_ID_MARGIN_BOTTOM, PAPER_ID_MARGIN_BOTTOM },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
paper_margin_left_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &paper_units_fine_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
paper_margin_left_units =
{
    PAPER_ID_MARGIN_LEFT_UNITS, PAPER_ID_PAPER_MARGIN_GROUP,
    { PAPER_ID_MARGIN_BOTTOM_UNITS, PAPER_ID_MARGIN_LEFT, PAPER_ID_MARGIN_BOTTOM_UNITS, PAPER_ID_MARGIN_LEFT },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

/*
right margin 'buddies'
*/

static const DIALOG_CONTROL
paper_margin_right_label =
{
    PAPER_ID_MARGIN_RIGHT_LABEL, PAPER_ID_PAPER_MARGIN_LABELS_GROUP,
    { DIALOG_CONTROL_PARENT, PAPER_ID_MARGIN_RIGHT, DIALOG_CONTROL_SELF, PAPER_ID_MARGIN_RIGHT },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
paper_margin_right_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_MARGIN_RIGHT), { 1/*left_text*/ } };

static const DIALOG_CONTROL
paper_margin_right =
{
    PAPER_ID_MARGIN_RIGHT, PAPER_ID_PAPER_MARGIN_GROUP,
    { PAPER_ID_MARGIN_LEFT, PAPER_ID_MARGIN_LEFT, PAPER_ID_MARGIN_LEFT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
paper_margin_right_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &paper_units_fine_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
paper_margin_right_units =
{
    PAPER_ID_MARGIN_RIGHT_UNITS, PAPER_ID_PAPER_MARGIN_GROUP,
    { PAPER_ID_MARGIN_LEFT_UNITS, PAPER_ID_MARGIN_RIGHT, PAPER_ID_MARGIN_LEFT_UNITS, PAPER_ID_MARGIN_RIGHT },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

/*
orientation group
*/

static const DIALOG_CONTROL
paper_orientation_group =
{
    PAPER_ID_ORIENTATION_GROUP, DIALOG_MAIN_GROUP,
    { PAPER_ID_PAPER_GROUP, PAPER_ID_PAPER_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, 0, 0 },
    { DRT(RTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static S32
paper_orientation_pl_current;

static const DIALOG_CONTROL
paper_orientation_p =
{
    PAPER_ID_ORIENTATION_P, PAPER_ID_ORIENTATION_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
paper_orientation_p_data = { { 0 }, 0 /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_PORTRAIT)};

static const DIALOG_CONTROL
paper_orientation_l =
{
    PAPER_ID_ORIENTATION_L, PAPER_ID_ORIENTATION_GROUP,
    { PAPER_ID_ORIENTATION_P, PAPER_ID_ORIENTATION_P, DIALOG_CONTROL_SELF, PAPER_ID_ORIENTATION_P },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
paper_orientation_l_data = { { 0 }, 1 /* activate_state */ , UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_LANDSCAPE) };

static const DIALOG_CONTROL
paper_button_read =
{
    PAPER_ID_BUTTON_READ, DIALOG_MAIN_GROUP,
    { PAPER_ID_ORIENTATION_GROUP, PAPER_ID_ORIENTATION_GROUP },
    { 0, DIALOG_STDSPACING_V, PAPER_BUTTONS_H, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBLT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
paper_button_read_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_BUTTON_READ) };

static const DIALOG_CONTROL
paper_button_none =
{
    PAPER_ID_BUTTON_NONE, DIALOG_MAIN_GROUP,
    { PAPER_ID_BUTTON_READ, PAPER_ID_BUTTON_READ },
    { DIALOG_STDSPACING_H, 0, PAPER_BUTTONS_H, DIALOG_STDPUSHBUTTON_V },
    { DRT(RTLT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
paper_button_none_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_BUTTON_NONE) };

static const DIALOG_CONTROL
paper_button[6] =
{
    {
        PAPER_ID_BUTTON_1, DIALOG_MAIN_GROUP, { PAPER_ID_BUTTON_READ, PAPER_ID_BUTTON_READ, PAPER_ID_BUTTON_READ },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V }, { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
    },

    {
        PAPER_ID_BUTTON_2, DIALOG_MAIN_GROUP, { PAPER_ID_BUTTON_NONE, PAPER_ID_BUTTON_NONE, PAPER_ID_BUTTON_NONE },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V }, { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
    },

    {
        PAPER_ID_BUTTON_3, DIALOG_MAIN_GROUP, { PAPER_ID_BUTTON_1, PAPER_ID_BUTTON_1, PAPER_ID_BUTTON_1 },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V }, { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
    },

    {
        PAPER_ID_BUTTON_4, DIALOG_MAIN_GROUP, { PAPER_ID_BUTTON_2, PAPER_ID_BUTTON_2, PAPER_ID_BUTTON_2 },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V }, { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
    },

    {
        PAPER_ID_BUTTON_5, DIALOG_MAIN_GROUP, { PAPER_ID_BUTTON_3, PAPER_ID_BUTTON_3, PAPER_ID_BUTTON_3 },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V }, { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
    },

    {
        PAPER_ID_BUTTON_6, DIALOG_MAIN_GROUP, { PAPER_ID_BUTTON_4, PAPER_ID_BUTTON_4, PAPER_ID_BUTTON_4 },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V }, { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
    }
};

static /*poked*/ DIALOG_CONTROL_DATA_PUSHBUTTON
paper_button_data[6] =
{
    { { 0 }, { UI_TEXT_TYPE_NONE } },
    { { 0 }, { UI_TEXT_TYPE_NONE } },
    { { 0 }, { UI_TEXT_TYPE_NONE } },
    { { 0 }, { UI_TEXT_TYPE_NONE } },
    { { 0 }, { UI_TEXT_TYPE_NONE } },
    { { 0 }, { UI_TEXT_TYPE_NONE } }
};

/*
binding group
*/

static const DIALOG_CONTROL
paper_binding_group =
{
    PAPER_ID_BINDING_GROUP, DIALOG_MAIN_GROUP,
    { PAPER_ID_BUTTON_3, PAPER_ID_PAPER_MARGIN_GROUP /*BUTTON_3*/, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0/*DIALOG_STDSPACING_V*/, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
paper_binding_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_BINDING), { FRAMED_BOX_GROUP } };

/*
bind margin 'buddies'
*/

static const DIALOG_CONTROL
paper_margin_bind_label =
{
    PAPER_ID_MARGIN_BIND_LABEL, PAPER_ID_BINDING_GROUP,
    { DIALOG_CONTROL_PARENT, PAPER_ID_MARGIN_BIND, DIALOG_CONTROL_SELF, PAPER_ID_MARGIN_BIND },
    { DIALOG_STDGROUP_LM, 0, PAPER_LABELS_H, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
paper_margin_bind_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_MARGIN_BIND), { 1/*left_text*/ } };

static const DIALOG_CONTROL
paper_margin_bind =
{
    PAPER_ID_MARGIN_BIND, PAPER_ID_BINDING_GROUP,
    { PAPER_ID_MARGIN_BIND_LABEL, DIALOG_CONTROL_PARENT },
    { DIALOG_LABELGAP_H, DIALOG_STDGROUP_TM, DIALOG_BUMP_H(5), DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
paper_margin_bind_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &paper_units_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
paper_margin_bind_units =
{
    PAPER_ID_MARGIN_BIND_UNITS, PAPER_ID_BINDING_GROUP,
    { PAPER_ID_MARGIN_BIND, PAPER_ID_MARGIN_BIND, DIALOG_CONTROL_SELF, PAPER_ID_MARGIN_BIND },
    { DIALOG_BUMPUNITSGAP_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

static const DIALOG_CONTROL
paper_margin_oe_swap =
{
    PAPER_ID_MARGIN_OE_SWAP, PAPER_ID_BINDING_GROUP,
    { PAPER_ID_MARGIN_BIND_LABEL, PAPER_ID_MARGIN_BIND, PAPER_ID_MARGIN_BIND },
    { 0, DIALOG_STDSPACING_V, -DIALOG_STDCHECK_H, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
paper_margin_oe_swap_data = { { 1 /*left_text*/ }, UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_MARGIN_OE_SWAP) };

/*
col margin 'buddies'
*/

static const DIALOG_CONTROL
paper_margin_col_label =
{
    PAPER_ID_MARGIN_COL_LABEL, DIALOG_MAIN_GROUP,
    { PAPER_ID_MARGIN_BIND_LABEL, PAPER_ID_MARGIN_COL, PAPER_ID_MARGIN_BIND_LABEL, PAPER_ID_MARGIN_COL },
    { 0 },
    { DRT(LTRB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
paper_margin_col_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_MARGIN_COL), { 1/*left_text*/ } };

static const DIALOG_CONTROL
paper_margin_col =
{
    PAPER_ID_MARGIN_COL, DIALOG_MAIN_GROUP,
    { PAPER_ID_MARGIN_BIND, PAPER_ID_BINDING_GROUP, PAPER_ID_MARGIN_BIND },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
paper_margin_col_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &paper_units_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
paper_margin_col_units =
{
    PAPER_ID_MARGIN_COL_UNITS, DIALOG_MAIN_GROUP,
    { PAPER_ID_MARGIN_BIND_UNITS, PAPER_ID_MARGIN_COL, PAPER_ID_MARGIN_BIND_UNITS, PAPER_ID_MARGIN_COL },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

/*
row margin 'buddies'
*/

static const DIALOG_CONTROL
paper_margin_row_label =
{
    PAPER_ID_MARGIN_ROW_LABEL, DIALOG_MAIN_GROUP,
    { PAPER_ID_MARGIN_COL_LABEL, PAPER_ID_MARGIN_ROW, PAPER_ID_MARGIN_COL_LABEL, PAPER_ID_MARGIN_ROW },
    { 0 },
    { DRT(LTRB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
paper_margin_row_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_MARGIN_ROW), { 1/*left_text*/ } };

static const DIALOG_CONTROL
paper_margin_row =
{
    PAPER_ID_MARGIN_ROW, DIALOG_MAIN_GROUP,
    { PAPER_ID_MARGIN_COL, PAPER_ID_MARGIN_COL, PAPER_ID_MARGIN_COL },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
paper_margin_row_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &paper_units_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
paper_margin_row_units =
{
    PAPER_ID_MARGIN_ROW_UNITS, DIALOG_MAIN_GROUP,
    { PAPER_ID_MARGIN_COL_UNITS, PAPER_ID_MARGIN_ROW, PAPER_ID_MARGIN_COL_UNITS, PAPER_ID_MARGIN_ROW },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

/*
grid group
*/

static const DIALOG_CONTROL
paper_grid_group =
{
    PAPER_ID_GRID_GROUP, DIALOG_MAIN_GROUP,
    { PAPER_ID_PAPER_MARGIN_GROUP, PAPER_ID_PAPER_MARGIN_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_GROUPSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
paper_grid_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_GRID), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
paper_grid_size_label =
{
    PAPER_ID_GRID_SIZE_LABEL, PAPER_ID_GRID_GROUP,
    { DIALOG_CONTROL_PARENT, PAPER_ID_GRID_SIZE, DIALOG_CONTROL_SELF, PAPER_ID_GRID_SIZE },
    { DIALOG_STDGROUP_LM, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
paper_grid_size_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_GRID_SIZE) };

static const DIALOG_CONTROL
paper_grid_size =
{
    PAPER_ID_GRID_SIZE, PAPER_ID_GRID_GROUP,
    { PAPER_ID_PAPER_MARGIN_LABELS_GROUP, DIALOG_CONTROL_PARENT },
    { DIALOG_LABELGAP_H, DIALOG_STDGROUP_TM, DIALOG_BUMP_H(3), DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_F64), 1 /*tabstop*/ }
};

static UCHARZ
paper_grid_size_numform_ustr_buf[6]; /* must fit "0.,##" - display more precision */

static const UI_CONTROL_F64
paper_grid_size_control = { 0.0, 1000.0, 0.1, (P_USTR) paper_grid_size_numform_ustr_buf, 10.0 };

static const DIALOG_CONTROL_DATA_BUMP_F64
paper_grid_size_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &paper_grid_size_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
paper_grid_size_units =
{
    PAPER_ID_GRID_SIZE_UNITS, PAPER_ID_GRID_GROUP,
    { PAPER_ID_GRID_SIZE, PAPER_ID_GRID_SIZE, DIALOG_CONTROL_SELF, PAPER_ID_GRID_SIZE },
    { DIALOG_BUMPUNITSGAP_H, 0, DIALOG_CONTENTS_CALC },
    { DRT(RTLB, STATICTEXT) }
};

#define paper_grid_size_units_data measurement_points_data

static const DIALOG_CONTROL
paper_grid_faint =
{
    PAPER_ID_GRID_FAINT, PAPER_ID_GRID_GROUP,
    { PAPER_ID_GRID_SIZE_LABEL, PAPER_ID_GRID_SIZE },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
paper_grid_faint_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_GRID_FAINT) };

/*
ok
*/

static const DIALOG_CONTROL_ID
paper_ok_data_argmap[] =
{
    PAPER_ID_NAME,
    PAPER_ID_ORIENTATION_GROUP,
    PAPER_ID_HEIGHT,
    PAPER_ID_WIDTH,
    PAPER_ID_MARGIN_TOP,
    PAPER_ID_MARGIN_BOTTOM,
    PAPER_ID_MARGIN_LEFT,
    PAPER_ID_MARGIN_RIGHT,
    PAPER_ID_MARGIN_BIND,
    PAPER_ID_MARGIN_OE_SWAP,
    PAPER_ID_MARGIN_COL,
    PAPER_ID_MARGIN_ROW,
    PAPER_ID_GRID_SIZE,
    0, /* ARG_PAPER_LOADED_ID : NB. only present in config file (so far) */
    PAPER_ID_GRID_FAINT
};

#if RISCOS

static const DIALOG_CONTROL
paper_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF, DIALOG_MAIN_GROUP, DIALOG_MAIN_GROUP },
    { DIALOG_CONTENTS_CALC, DIALOG_DEFPUSHBUTTON_V, 0, DIALOG_STDSPACING_V },
    { DRT(RBRB, PUSHBUTTON), 1 /*tabstop*/ }
};

#else
#define paper_ok defbutton_ok
#endif

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
paper_ok_command = { T5_CMD_PAPER, OBJECT_ID_SKEL, NULL, paper_ok_data_argmap, { 0 /*set_view*/, 0, 0, 1 /*lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
paper_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_BUTTON_APPLY), &paper_ok_command };

static /*poked*/ DIALOG_CTL_CREATE
paper_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &paper_paper_group },          &paper_paper_group_data },
    { { &paper_paper_labels_group },   NULL },
    { { &paper_name_label },           &paper_name_label_data },
    { { &paper_name },                 &paper_name_data },
    { { &paper_height_label },         &paper_height_label_data },
    { { &paper_height },               &paper_height_data },
    { { &paper_height_units },         &paper_units_data },
    { { &paper_width_label },          &paper_width_label_data },
    { { &paper_width },                &paper_width_data },
    { { &paper_width_units },          &paper_units_data },

    { { &paper_paper_margin_group },   &paper_paper_margin_group_data },
    { { &paper_paper_margin_labels_group }, NULL },
    { { &paper_margin_top_label },     &paper_margin_top_label_data },
    { { &paper_margin_top },           &paper_margin_top_data },
    { { &paper_margin_top_units },     &paper_units_data },
    { { &paper_margin_bottom_label },  &paper_margin_bottom_label_data },
    { { &paper_margin_bottom },        &paper_margin_bottom_data },
    { { &paper_margin_bottom_units },  &paper_units_data },
    { { &paper_margin_left_label },    &paper_margin_left_label_data },
    { { &paper_margin_left },          &paper_margin_left_data },
    { { &paper_margin_left_units },    &paper_units_data },
    { { &paper_margin_right_label },   &paper_margin_right_label_data },
    { { &paper_margin_right },         &paper_margin_right_data },
    { { &paper_margin_right_units },   &paper_units_data },

    { { &paper_orientation_group },    NULL },
    { { &paper_orientation_p },        &paper_orientation_p_data },
    { { &paper_orientation_l },        &paper_orientation_l_data },

    { { &paper_button_read },          &paper_button_read_data },
    { { &paper_button_none },          &paper_button_none_data },
    { { &paper_button[0] },            &paper_button_data[0] },
    { { &paper_button[1] },            &paper_button_data[1] },
    { { &paper_button[2] },            &paper_button_data[2] },
    { { &paper_button[3] },            &paper_button_data[3] },
    { { &paper_button[4] },            &paper_button_data[4] },
    { { &paper_button[5] },            &paper_button_data[5] },

    { { &paper_binding_group },        &paper_binding_group_data },

    { { &paper_margin_bind_label },    &paper_margin_bind_label_data },
    { { &paper_margin_bind },          &paper_margin_bind_data },
    { { &paper_margin_bind_units },    &paper_units_data },

    { { &paper_margin_oe_swap },       &paper_margin_oe_swap_data },

    { { &paper_margin_col_label },     &paper_margin_col_label_data },
    { { &paper_margin_col },           &paper_margin_col_data },
    { { &paper_margin_col_units },     &paper_units_data },
    { { &paper_margin_row_label },     &paper_margin_row_label_data },
    { { &paper_margin_row },           &paper_margin_row_data },
    { { &paper_margin_row_units },     &paper_units_data },

    { { &paper_grid_group },           &paper_grid_group_data },
    { { &paper_grid_size_label },      &paper_grid_size_label_data },
    { { &paper_grid_size },            &paper_grid_size_data },
    { { &paper_grid_size_units },      &paper_grid_size_units_data },
    { { &paper_grid_faint },           &paper_grid_faint_data },

    { { &paper_ok }, &paper_ok_data },
    { { &stdbutton_cancel }, &stdbutton_cancel_data }
};

static void
paper_precreate_button(
    _InRef_     P_DIALOG_CTL_CREATE p_dialog_ctl_create)
{
    const PC_DOCU p_docu_config = p_docu_from_config();
    const DIALOG_CONTROL_ID dialog_control_id = p_dialog_ctl_create->p_dialog_control.p_dialog_control->dialog_control_id;
    const ARRAY_INDEX control_index = (ARRAY_INDEX) dialog_control_id - PAPER_ID_BUTTON_1;

    if(array_index_is_valid(&p_docu_config->loaded_phys_page_defs, control_index))
    {
        P_PHYS_PAGE_DEF p_phys_page_def = array_ptr_no_checks(&p_docu_config->loaded_phys_page_defs, PHYS_PAGE_DEF, control_index);
        PC_DIALOG_CONTROL_DATA_PUSHBUTTON p_dialog_control_data_pushbutton = p_dialog_ctl_create->p_dialog_control_data.pushbutton;
        P_UI_TEXT p_ui_text = de_const_cast(P_UI_TEXT, &p_dialog_control_data_pushbutton->caption);
        /* a bold assignment follows ... I assert these will not move (at least not during the paper box!) */
        p_ui_text->type = UI_TEXT_TYPE_USTR_PERM;
        p_ui_text->text.ustr = ustr_bptr(p_phys_page_def->ustr_name);
    }
    else
    {   /* don't add this control */
        * (PC_DIALOG_CONTROL *) &p_dialog_ctl_create->p_dialog_control.p_dialog_control = NULL;
    }
}

static void
paper_precreate(
    _DocuRef_   P_DOCU p_docu,
    P_DIALOG_CMD_PROCESS_DBOX p_dialog_cmd_process_dbox)
{
    static UCHARZ paper_units_numform_ustr_buf[7]; /* must fit "0.,###" higher precision */

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    resource_lookup_ustr_buffer(ustr_bptr(paper_grid_size_numform_ustr_buf), elemof32(paper_grid_size_numform_ustr_buf), MSG_NUMFORM_2_DP);

#if 0
    paper_user_unit_resource_id = MSG_DIALOG_UNITS_MM;
#else
   { /* reflect current measurement system in dialogue box */
    SCALE_INFO scale_info;

    scale_info_from_docu(p_docu, TRUE, &scale_info);

    switch(scale_info.display_unit)
    {
    default: default_unhandled();
#if CHECKING
    case DISPLAY_UNIT_CM:
#endif
        paper_user_unit_resource_id = MSG_DIALOG_UNITS_CM;
        break;

    case DISPLAY_UNIT_MM:
        paper_user_unit_resource_id = MSG_DIALOG_UNITS_MM;
        break;

    case DISPLAY_UNIT_INCHES:
        paper_user_unit_resource_id = MSG_DIALOG_UNITS_INCHES;
        break;

    case DISPLAY_UNIT_POINTS:
        paper_user_unit_resource_id = MSG_DIALOG_UNITS_POINTS;
        break;
    }
    } /*block*/
#endif

    {
    const F64 max_inches = 1000.0;
    UI_CONTROL_F64 * p_ui_control_f64 =  &paper_units_control;
    STATUS numform_resource_id;

    p_ui_control_f64->min_val = 0.0;

    switch(paper_user_unit_resource_id)
    {
    default: default_unhandled();
#if CHECKING
    case MSG_DIALOG_UNITS_CM:
#endif
        paper_fp_pixits_per_user_unit = FP_PIXITS_PER_CM;
        numform_resource_id = MSG_NUMFORM_2_DP0;
        p_ui_control_f64->inc_dec_round = 10.0;
        p_ui_control_f64->max_val = (max_inches * PIXITS_PER_INCH) / PIXITS_PER_CM;
        break;

    case MSG_DIALOG_UNITS_MM:
        paper_fp_pixits_per_user_unit = FP_PIXITS_PER_MM;
        numform_resource_id = MSG_NUMFORM_1_DP0;
        p_ui_control_f64->inc_dec_round = 1.0;
        p_ui_control_f64->max_val = (max_inches * PIXITS_PER_INCH) / PIXITS_PER_MM;
        break;

    case MSG_DIALOG_UNITS_INCHES:
        paper_fp_pixits_per_user_unit = PIXITS_PER_INCH;
        numform_resource_id = MSG_NUMFORM_3_DP0;
        p_ui_control_f64->inc_dec_round = 10.0;
        p_ui_control_f64->max_val = max_inches;
        break;

    case MSG_DIALOG_UNITS_POINTS:
        paper_fp_pixits_per_user_unit = PIXITS_PER_POINT;
        numform_resource_id = MSG_NUMFORM_1_DP0;
        p_ui_control_f64->inc_dec_round = 1.0;
        p_ui_control_f64->max_val = (max_inches * PIXITS_PER_INCH) / PIXITS_PER_POINT;
        break;
    }

    resource_lookup_ustr_buffer(ustr_bptr(paper_units_numform_ustr_buf), elemof32(paper_units_numform_ustr_buf), numform_resource_id);

    p_ui_control_f64->ustr_numform = ustr_bptr(paper_units_numform_ustr_buf);

    p_ui_control_f64->bump_val = 1.0 / p_ui_control_f64->inc_dec_round;
    } /*block*/

    {
    const F64 max_inches_fine = 30.0;
    UI_CONTROL_F64 * p_ui_control_f64 = &paper_units_fine_control;

    *p_ui_control_f64 = paper_units_control; /* this one is much like the other */

    switch(paper_user_unit_resource_id)
    {
    default: default_unhandled();
#if CHECKING
    case MSG_DIALOG_UNITS_CM:
#endif
        p_ui_control_f64->max_val = (max_inches_fine * PIXITS_PER_CM) / PIXITS_PER_POINT;
        break;

    case MSG_DIALOG_UNITS_MM:
        p_ui_control_f64->inc_dec_round = 10.0; /* smaller increments for critical units! */
        p_ui_control_f64->max_val = (max_inches_fine * PIXITS_PER_MM) / PIXITS_PER_POINT;
        break;

    case MSG_DIALOG_UNITS_INCHES:
        p_ui_control_f64->max_val = max_inches_fine;
        break;

    case MSG_DIALOG_UNITS_POINTS:
        p_ui_control_f64->max_val = (max_inches_fine * PIXITS_PER_INCH) / PIXITS_PER_POINT;
        break;
    }

    p_ui_control_f64->bump_val = 1.0 / p_ui_control_f64->inc_dec_round;
    } /*block*/

    paper_units_data.caption.type = UI_TEXT_TYPE_RESID;
    paper_units_data.caption.text.resource_id = paper_user_unit_resource_id;

    { /* quick pre-create pass */
    U32 i;

    assert(NULL != p_dialog_cmd_process_dbox->p_ctl_create);

    for(i = 0; i < p_dialog_cmd_process_dbox->n_ctls; ++i)
    {
        const P_DIALOG_CTL_CREATE p_dialog_ctl_create = &p_dialog_cmd_process_dbox->p_ctl_create[i];
        DIALOG_CONTROL_ID dialog_control_id;

        if(NULL == p_dialog_ctl_create->p_dialog_control.p_dialog_control)
            continue;

        dialog_control_id = p_dialog_ctl_create->p_dialog_control.p_dialog_control->dialog_control_id;

        switch(dialog_control_id)
        {
        case PAPER_ID_BUTTON_1:
        case PAPER_ID_BUTTON_2:
        case PAPER_ID_BUTTON_3:
        case PAPER_ID_BUTTON_4:
        case PAPER_ID_BUTTON_5:
        case PAPER_ID_BUTTON_6:
            paper_precreate_button(p_dialog_ctl_create);
            break;

        default:
            break;
        }
    }
    } /*block*/
}

_Check_return_
static STATUS
paper_set_dlg_as_unit(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     PIXIT value)
{
    const F64 f64 = /*FP_USER_UNIT*/ ((FP_PIXIT) value / paper_fp_pixits_per_user_unit);
    return(ui_dlg_set_f64(h_dialog, dialog_control_id, f64));
}

static const DIALOG_CONTROL_ID
button_control_id[6] =
{
    PAPER_ID_HEIGHT,
    PAPER_ID_WIDTH,
    PAPER_ID_MARGIN_LEFT,
    PAPER_ID_MARGIN_TOP,
    PAPER_ID_MARGIN_RIGHT,
    PAPER_ID_MARGIN_BOTTOM
};

static void
paper_read_vals(
    _InVal_     H_DIALOG h_dialog,
    P_FP_PIXIT p_data /*[6]*/)
{
    STATUS i;
    P_FP_PIXIT p[6];

    if(paper_orientation_pl_current == 1)
    {
        p[1] = &p_data[0];
        p[0] = &p_data[1];

        p[5] = &p_data[2];
        p[2] = &p_data[3];
        p[3] = &p_data[4];
        p[4] = &p_data[5];
    }
    else
    {
        for(i = 0; i < 6; ++i)
            p[i] = &p_data[i];
    }

    for(i = 0; i < 6; ++i)
    {
        const F64 f64 = ui_dlg_get_f64(h_dialog, button_control_id[i]); /*FP_USER_UNIT*/
        *p[i] = f64 * paper_fp_pixits_per_user_unit;
    }
}

_Check_return_
static STATUS
paper_set_vals(
    _InVal_     H_DIALOG h_dialog,
    PC_FP_PIXIT p_data /*[6]*/)
{
    BOOL landscape;
    STATUS i;
    PC_FP_PIXIT p[6];

    landscape = (ui_dlg_get_radio(h_dialog, PAPER_ID_ORIENTATION_GROUP) != 0);

    if(landscape)
    {
        p[1] = &p_data[0];
        p[0] = &p_data[1];

        p[5] = &p_data[2];
        p[2] = &p_data[3];
        p[3] = &p_data[4];
        p[4] = &p_data[5];
    }
    else
    {
        for(i = 0; i < 6; ++i)
            p[i] = &p_data[i];
    }

    for(i = 0; i < 6; ++i)
    {
        const F64 f64 = /*FP_USER_UNIT*/ (*p[i] / paper_fp_pixits_per_user_unit);
        status_return(ui_dlg_set_f64(h_dialog, button_control_id[i], f64));
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_paper_intro_ctl_create(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE p_dialog_msg_ctl_create)
{
    switch(p_dialog_msg_ctl_create->dialog_control_id)
    {
    case PAPER_ID_BUTTON_READ:
        {
        PAPER paper;

        if(status_fail(host_read_printer_paper_details(&paper)))
             /* disable this control if no printer is available */
             ui_dlg_ctl_enable(p_dialog_msg_ctl_create->h_dialog, p_dialog_msg_ctl_create->dialog_control_id, 0);

        break;
        }

    default:
        /* other things do come through here, so ignore them */
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_paper_intro_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    const PC_S32 p_s32 = (PC_S32) p_dialog_msg_ctl_create_state->client_handle;

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case PAPER_ID_ORIENTATION_GROUP:
        p_dialog_msg_ctl_create_state->state_set.state.radiobutton = *p_s32 /*paper_orientation_pl_current*/;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_paper_intro_process_start(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const H_DIALOG h_dialog = p_dialog_msg_process_start->h_dialog;

    status_return(paper_set_dlg_as_unit(h_dialog, PAPER_ID_HEIGHT, p_docu->phys_page_def.size_y));
    status_return(paper_set_dlg_as_unit(h_dialog, PAPER_ID_WIDTH,  p_docu->phys_page_def.size_x));

    status_return(paper_set_dlg_as_unit(h_dialog, PAPER_ID_MARGIN_TOP,    p_docu->phys_page_def.margin_top));
    status_return(paper_set_dlg_as_unit(h_dialog, PAPER_ID_MARGIN_BOTTOM, p_docu->phys_page_def.margin_bottom));
    status_return(paper_set_dlg_as_unit(h_dialog, PAPER_ID_MARGIN_LEFT,   p_docu->phys_page_def.margin_left));
    status_return(paper_set_dlg_as_unit(h_dialog, PAPER_ID_MARGIN_RIGHT,  p_docu->phys_page_def.margin_right));

    status_return(paper_set_dlg_as_unit(h_dialog, PAPER_ID_MARGIN_BIND, p_docu->phys_page_def.margin_bind));
    status_return(paper_set_dlg_as_unit(h_dialog, PAPER_ID_MARGIN_COL,  p_docu->page_def.margin_col));
    status_return(paper_set_dlg_as_unit(h_dialog, PAPER_ID_MARGIN_ROW,  p_docu->page_def.margin_row));

    status_return(ui_dlg_set_check(h_dialog, PAPER_ID_MARGIN_OE_SWAP, p_docu->phys_page_def.margin_oe_swap));

    {
    const F64 f64 = /*FP_POINTS*/ ((FP_PIXIT) p_docu->page_def.grid_size / PIXITS_PER_POINT);
    status_return(ui_dlg_set_f64(h_dialog, PAPER_ID_GRID_SIZE, f64));
    } /*block*/

    return(ui_dlg_set_check(h_dialog, PAPER_ID_GRID_FAINT, p_docu->flags.faint_grid));
}

_Check_return_
static STATUS
dialog_paper_intro_preprocess_command(
    _InoutRef_  P_DIALOG_MSG_PREPROCESS_COMMAND p_dialog_msg_preprocess_command)
{
    const U32 n_args = n_arglist_args(&p_dialog_msg_preprocess_command->arglist_handle);
    const P_ARGLIST_ARG p_args = p_arglist_args(&p_dialog_msg_preprocess_command->arglist_handle, n_args);
    U32 arg_idx;

    for(arg_idx = 0; arg_idx < n_args; ++arg_idx)
    {
        if(!arg_is_present(p_args, arg_idx))
            continue;

        switch(arg_idx)
        {
        default: default_unhandled();
#if CHECKING
        case ARG_PAPER_HEIGHT:
        case ARG_PAPER_WIDTH:

        case ARG_PAPER_MARGIN_TOP:
        case ARG_PAPER_MARGIN_BOTTOM:
        case ARG_PAPER_MARGIN_LEFT:
        case ARG_PAPER_MARGIN_RIGHT:

        case ARG_PAPER_MARGIN_BIND:
        case ARG_PAPER_MARGIN_COL:
        case ARG_PAPER_MARGIN_ROW:
#endif
            p_args[arg_idx].val.fp_pixit = (FP_PIXIT) ((PIXIT) (/*FP_USER_UNIT*/ p_args[arg_idx].val.f64 * paper_fp_pixits_per_user_unit + 0.5));
            break;

        case ARG_PAPER_GRID_SIZE:
            p_args[arg_idx].val.fp_pixit = (FP_PIXIT) ((PIXIT) (/*FP_POINTS*/ p_args[arg_idx].val.f64 * PIXITS_PER_POINT + 0.5));
            break;

        case ARG_PAPER_NAME:
        case ARG_PAPER_ORIENTATION:
        case ARG_PAPER_MARGIN_OE_SWAP:
        case ARG_PAPER_GRID_FAINT:
            break;
        }
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_paper_intro_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_S32 p_s32 = (P_S32) p_dialog_msg_ctl_state_change->client_handle;
    STATUS status = STATUS_OK;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case PAPER_ID_ORIENTATION_GROUP:
        if(*p_s32 /*paper_orientation_pl_current*/ != p_dialog_msg_ctl_state_change->new_state.radiobutton)
        {
            /* swap values iff changed */
            FP_PIXIT data[6];

            /* read ***current*** state out, swapping as needed */
            paper_read_vals(p_dialog_msg_ctl_state_change->h_dialog, data);

            *p_s32 /*paper_orientation_pl_current*/ = p_dialog_msg_ctl_state_change->new_state.radiobutton;

            status = paper_set_vals(p_dialog_msg_ctl_state_change->h_dialog, data);
        }

        break;

    default:
        /* other things do come through here, so ignore them */
        break;
    }

    return(status);
}

_Check_return_
static STATUS
dialog_paper_intro_ctl_pushbutton_NONE(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    FP_PIXIT data[6];
    static const UI_TEXT ui_text = UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_BUTTON_NONE);

    status_return(ui_dlg_set_edit(p_dialog_msg_ctl_pushbutton->h_dialog, PAPER_ID_NAME, &ui_text));

    /* set 'infinite' paper size */
    data[0] = FP_PIXITS_PER_METRE * 5.0;
    data[1] = FP_PIXITS_PER_METRE * 5.0;
    data[2] = 0.0;
    data[3] = 0.0;
    data[4] = 0.0;
    data[5] = 0.0;

    return(paper_set_vals(p_dialog_msg_ctl_pushbutton->h_dialog, data));
}

_Check_return_
static STATUS
dialog_paper_intro_ctl_pushbutton_READ(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    /* read corresponding paper name and dimensions from current printer into dialog controls */
    PAPER paper;

    if(status_ok(host_read_printer_paper_details(&paper)))
    {
        FP_PIXIT data[6];
        static const UI_TEXT ui_text = UI_TEXT_INIT_RESID(MSG_DIALOG_PAPER_BUTTON_FROM_PRINTER);

        status_return(ui_dlg_set_edit(p_dialog_msg_ctl_pushbutton->h_dialog, PAPER_ID_NAME, &ui_text));

        data[0] = (FP_PIXIT) paper.y_size;
        data[1] = (FP_PIXIT) paper.x_size;
        data[2] = (FP_PIXIT) paper.lm;
        data[3] = (FP_PIXIT) paper.tm;
        data[4] = (FP_PIXIT) paper.rm;
        data[5] = (FP_PIXIT) paper.bm;

        return(paper_set_vals(p_dialog_msg_ctl_pushbutton->h_dialog, data));
    }

    /* don't disturb current settings - should never get here 'cos should be greyed out if not available */
    assert0();
    host_bleep();
    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_paper_intro_ctl_pushbutton_N(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    STATUS status = STATUS_OK;
    FP_PIXIT data[6];
    UI_TEXT ui_text;
    P_PHYS_PAGE_DEF p_phys_page_def;
    const DIALOG_CONTROL_ID dialog_control_id = p_dialog_msg_ctl_pushbutton->dialog_control_id;
    ARRAY_INDEX control_index = (ARRAY_INDEX) dialog_control_id - PAPER_ID_BUTTON_1;
    const PC_DOCU p_docu_config = p_docu_from_config();

    if(control_index >= array_elements(&p_docu_config->loaded_phys_page_defs))
        return(status);

    p_phys_page_def = array_ptr(&p_docu_config->loaded_phys_page_defs, PHYS_PAGE_DEF, control_index);

    ui_text.type = UI_TEXT_TYPE_USTR_TEMP;
    ui_text.text.ustr = ustr_bptr(p_phys_page_def->ustr_name);
    status = ui_dlg_set_edit(p_dialog_msg_ctl_pushbutton->h_dialog, PAPER_ID_NAME, &ui_text);

    data[0] = (FP_PIXIT) p_phys_page_def->size_y;
    data[1] = (FP_PIXIT) p_phys_page_def->size_x;
    data[2] = (FP_PIXIT) p_phys_page_def->margin_left;
    data[3] = (FP_PIXIT) p_phys_page_def->margin_top;
    data[4] = (FP_PIXIT) p_phys_page_def->margin_right;
    data[5] = (FP_PIXIT) p_phys_page_def->margin_bottom;

    if(status_ok(status))
        status = paper_set_vals(p_dialog_msg_ctl_pushbutton->h_dialog, data);

    return(status);
}

_Check_return_
static STATUS
dialog_paper_intro_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case PAPER_ID_BUTTON_NONE:
        return(dialog_paper_intro_ctl_pushbutton_NONE(p_dialog_msg_ctl_pushbutton));

    case PAPER_ID_BUTTON_READ:
        return(dialog_paper_intro_ctl_pushbutton_READ(p_dialog_msg_ctl_pushbutton));

    case PAPER_ID_BUTTON_1:
    case PAPER_ID_BUTTON_2:
    case PAPER_ID_BUTTON_3:
    case PAPER_ID_BUTTON_4:
    case PAPER_ID_BUTTON_5:
    case PAPER_ID_BUTTON_6:
        return(dialog_paper_intro_ctl_pushbutton_N(p_dialog_msg_ctl_pushbutton));

    default:
        /* other things do come through here, so ignore them */
        return(STATUS_OK);
    }
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_paper_intro)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_CREATE:
        return(dialog_paper_intro_ctl_create((P_DIALOG_MSG_CTL_CREATE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_paper_intro_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_paper_intro_process_start(p_docu, (PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PREPROCESS_COMMAND:
        return(dialog_paper_intro_preprocess_command((P_DIALOG_MSG_PREPROCESS_COMMAND) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_paper_intro_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_paper_intro_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(extern, t5_cmd_paper_intro)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    paper_name_data.state.type = UI_TEXT_TYPE_USTR_TEMP;
    paper_name_data.state.text.ustr = ustr_bptr(p_docu->phys_page_def.ustr_name);

    paper_orientation_pl_current = p_docu->phys_page_def.landscape;

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, paper_ctl_create, elemof32(paper_ctl_create), MSG_DIALOG_PAPER_CAPTION);
    dialog_cmd_process_dbox.help_topic_resource_id = MSG_DIALOG_PAPER_HELP_TOPIC;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &paper_orientation_pl_current;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_paper_intro;
    paper_precreate(p_docu, &dialog_cmd_process_dbox);
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
    } /*block*/
}

/* end of ui_page.c */
