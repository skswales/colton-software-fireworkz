/* ui_remov.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Style region editing UI */

/* MRJC December 1992 */

#include "common/gflags.h"

#include "ob_skspt/ui_styin.h"

#if RISCOS
#include "ob_dlg/xp_dlgr.h"
#endif

enum REGION_EDIT_CONTROL_IDS
{
    REGION_EDIT_ID_IN = 348,
    REGION_EDIT_ID_OUT,
    REGION_EDIT_ID_REMOVE
};

enum REGION_EDIT_COMPLETION_CODE
{
    REGION_EDIT_COMPLETION_IN = 456,
    REGION_EDIT_COMPLETION_OUT,
    REGION_EDIT_COMPLETION_REMOVE,
    REGION_EDIT_COMPLETION_REMOVE_PERSIST
};

#if RISCOS
#define REGION_EDIT_BUTTONS_H DIALOG_STDCANCEL_H
#else
#define REGION_EDIT_BUTTONS_H DIALOG_STDPUSHBUTTON_H
#endif

/*
next
*/

static const DIALOG_CONTROL
remove_query_in =
{
    REGION_EDIT_ID_IN, DIALOG_CONTROL_WINDOW,

    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },

    { 0, 0, REGION_EDIT_BUTTONS_H, DIALOG_STDPUSHBUTTON_V },

    { DRT(LTLT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTONR
remove_query_in_data = { { REGION_EDIT_COMPLETION_IN, 0, 0, 1 /*alternate_right*/ }, UI_TEXT_INIT_RESID(MSG_IN), REGION_EDIT_COMPLETION_OUT };

/*
replace
*/

static const DIALOG_CONTROL
remove_query_out =
{
    REGION_EDIT_ID_OUT, DIALOG_CONTROL_WINDOW,

    { REGION_EDIT_ID_IN, REGION_EDIT_ID_IN, REGION_EDIT_ID_IN },

    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },

    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTONR
remove_query_out_data = { { REGION_EDIT_COMPLETION_OUT, 0, 0, 1 /*alternate_right*/ }, UI_TEXT_INIT_RESID(MSG_OUT), REGION_EDIT_COMPLETION_IN };

/*
replace all
*/

static const DIALOG_CONTROL
remove_query_remove =
{
    REGION_EDIT_ID_REMOVE, DIALOG_CONTROL_WINDOW,

    { REGION_EDIT_ID_OUT, REGION_EDIT_ID_OUT, REGION_EDIT_ID_OUT },

    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },

    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTONR
remove_query_remove_data = { { REGION_EDIT_COMPLETION_REMOVE, 0, 0, 1 /*alternate_right*/ }, UI_TEXT_INIT_RESID(MSG_DELETE), REGION_EDIT_COMPLETION_REMOVE_PERSIST };

/*
cancel
*/

static const DIALOG_CONTROL
remove_query_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,

    { REGION_EDIT_ID_REMOVE, REGION_EDIT_ID_REMOVE, REGION_EDIT_ID_REMOVE },

    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },

    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CTL_CREATE
remove_query_ctl_create[] =
{
    { &remove_query_in,      &remove_query_in_data      },
    { &remove_query_out,     &remove_query_out_data     },
    { &remove_query_remove,  &remove_query_remove_data  },
    { &remove_query_cancel,  &stdbutton_cancel_data }
};

typedef struct UI_REMOVE_CALLBACK
{
    BOOL in, out, removable;
}
UI_REMOVE_CALLBACK, * P_UI_REMOVE_CALLBACK;

_Check_return_
static STATUS
dialog_ui_remove_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_UI_REMOVE_CALLBACK p_ui_remove_callback = (P_UI_REMOVE_CALLBACK) p_dialog_msg_process_start->client_handle;

    ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, REGION_EDIT_ID_IN, p_ui_remove_callback->in);
    ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, REGION_EDIT_ID_OUT, p_ui_remove_callback->out);
    ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, REGION_EDIT_ID_REMOVE, p_ui_remove_callback->removable);

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_ui_remove)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_ui_remove_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(extern, t5_cmd_style_region_edit)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX ix = -1, top_ix = -1, bottom_ix = -1;
    enum STYLE_DOCU_AREA_CHOOSE_ACTION action = STYLE_CHOOSE_DOWN;
    S32 stop_edit = 0;
    P_DOCU_AREA p_docu_area = NULL;
    P_POSITION p_position = NULL;
    DOCU_AREA docu_area;
    P_STYLE_DOCU_AREA p_style_docu_area = NULL;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    /* select starting point */
    if(p_docu->mark_info_cells.h_markers)
    {
        docu_area_from_markers_first(p_docu, &docu_area);
        p_docu_area = &docu_area;
    }
    else
        p_position = &p_docu->cur;

    while(status_ok(status) && !stop_edit)
    {
        S32 new_ix;

        if((new_ix = style_docu_area_choose(&p_docu->h_style_docu_area, p_docu_area, p_position, action, ix)) >= 0)
        {
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
            quick_ublock_with_buffer_setup(quick_ublock);

            p_style_docu_area = array_ptr(&p_docu->h_style_docu_area, STYLE_DOCU_AREA, new_ix);

            status_assert(object_call_id(OBJECT_ID_CELLS, p_docu, T5_MSG_SELECTION_MAKE, &p_style_docu_area->docu_area));

            if(status_ok(status = style_text_convert(p_docu,
                                                     &quick_ublock,
                                                     p_style_from_docu_area(p_docu, p_style_docu_area),
                                                     &style_selector_all)))
                status_line_setf(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, MSG_STYLE_TEXT_CONVERT_REGION, quick_ublock_ustr(&quick_ublock));

            quick_ublock_dispose(&quick_ublock);

            if(p_style_docu_area->base)
                bottom_ix = new_ix;

            status_break(status);
        }
        else if(action == STYLE_CHOOSE_DOWN)
        {
            static const UI_TEXT ui_text = UI_TEXT_INIT_RESID(MSG_STYLE_TEXT_CONVERT_NO_MORE);
            bottom_ix = ix;
            status_line_set(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, &ui_text);
        }

        if(NULL == p_style_docu_area)
            break;
        else
        {
            STATUS remove_action;

            ix = new_ix >= 0 ? new_ix : ix;
            top_ix = top_ix >= 0 ? top_ix : new_ix;

            {
            UI_REMOVE_CALLBACK ui_remove_callback;
            DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;

            ui_remove_callback.in = ix > bottom_ix;
            ui_remove_callback.out = top_ix > 0 && ix < top_ix;
            ui_remove_callback.removable = !p_style_docu_area->base;

            dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, remove_query_ctl_create, elemof32(remove_query_ctl_create), SKEL_SPLIT_MSG_DIALOG_REGION_EDIT_HELP_TOPIC);
            /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
            dialog_cmd_process_dbox.caption.text.resource_id = SKEL_SPLIT_MSG_DIALOG_REGION_EDIT_CAPTION;
            dialog_cmd_process_dbox.bits.note_position = 1;
            dialog_cmd_process_dbox.p_proc_client = dialog_event_ui_remove;
            dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &ui_remove_callback;
            remove_action = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
            } /*block*/

            switch(remove_action)
            {
            default:
                status = remove_action;
                stop_edit = 1;
                break;

            case STATUS_FAIL:
                status = STATUS_OK;
                stop_edit = 1;
                break;

            case REGION_EDIT_COMPLETION_IN:
                action = STYLE_CHOOSE_DOWN;
                break;

            case REGION_EDIT_COMPLETION_OUT:
                action = STYLE_CHOOSE_UP;
                break;

            case REGION_EDIT_COMPLETION_REMOVE:
            case REGION_EDIT_COMPLETION_REMOVE_PERSIST:
                docu_modify(p_docu);
                style_docu_area_remove(p_docu, &p_docu->h_style_docu_area, ix);
                if(remove_action == REGION_EDIT_COMPLETION_REMOVE_PERSIST)
                {
                    top_ix -= 1;
                    action = STYLE_CHOOSE_DOWN;
                }
                else
                    stop_edit = 1;
                break;
            }
        }
    }

    status_line_auto_clear(p_docu);
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    return(status);
}

/******************************************************************************
*
* box dialog box
*
******************************************************************************/

typedef struct BOX_PERSISTENT_STATE
{
    S32  line_style;
    BOOL all;
    BOOL outside;
    BOOL V_all;
    BOOL V_l;
    BOOL V_r;
    BOOL H_all;
    BOOL H_t;
    BOOL H_b;
    RGB  rgb;
}
BOX_PERSISTENT_STATE;

static BOX_PERSISTENT_STATE
g_box_persistent_state = { SF_BORDER_STANDARD /*line_style*/, 1 /*all*/ };


enum BOX_CONTROL_IDS
{
    BOX_ID_ALL = 100,
    BOX_ID_OUTSIDE,

    BOX_ID_V_GROUP = 110,
    BOX_ID_V_ALL,
    BOX_ID_V_L,
    BOX_ID_V_R,

    BOX_ID_H_GROUP = 120,
    BOX_ID_H_ALL,
    BOX_ID_H_T,
    BOX_ID_H_B,

    BOX_ID_LINE_GROUP = 250,
    BOX_ID_LINE_0,
    BOX_ID_LINE_1,
    BOX_ID_LINE_2,
    BOX_ID_LINE_3,
    BOX_ID_LINE_4
};

static const DIALOG_CONTROL_ID
box_ok_data_argmap[] =
{
    BOX_ID_ALL,
    BOX_ID_OUTSIDE,
    BOX_ID_V_ALL,
    BOX_ID_V_L,
    BOX_ID_V_R,
    BOX_ID_H_ALL,
    BOX_ID_H_T,
    BOX_ID_H_B,
    BOX_ID_LINE_GROUP,
    DIALOG_ID_RGB_PATCH
};

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
box_ok_command = { T5_CMD_BOX, OBJECT_ID_SKEL, NULL, box_ok_data_argmap, { 0, 0, 0, 1 /*lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
box_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_OK), &box_ok_command };

/*
main 'group'
*/

static const DIALOG_CONTROL
box_all =
{
    BOX_ID_ALL, DIALOG_COL1_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
box_all_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_BOX_ALL), DIALOG_BUTTONSTATE_ON /*state*/ };

static const DIALOG_CONTROL
box_outside =
{
    BOX_ID_OUTSIDE, DIALOG_COL1_GROUP,
    { BOX_ID_ALL, BOX_ID_ALL, DIALOG_CONTROL_SELF, BOX_ID_ALL },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
box_outside_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_BOX_OUTSIDE) };

static const DIALOG_CONTROL
box_V_group =
{
    BOX_ID_V_GROUP, DIALOG_COL1_GROUP,
    { BOX_ID_ALL, BOX_ID_ALL, BOX_ID_V_R, BOX_ID_V_R },
    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
box_V_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_BOX_V), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
box_V_all =
{
    BOX_ID_V_ALL, BOX_ID_V_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
box_V_all_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_BOX_ALL) };

static const DIALOG_CONTROL
box_V_l =
{
    BOX_ID_V_L, BOX_ID_V_GROUP,
    { BOX_ID_V_ALL, BOX_ID_V_ALL, DIALOG_CONTROL_SELF, BOX_ID_V_ALL },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
box_V_l_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_BOX_L) };

static const DIALOG_CONTROL
box_V_r =
{
    BOX_ID_V_R, BOX_ID_V_GROUP,
    { BOX_ID_V_L, BOX_ID_V_L, DIALOG_CONTROL_SELF, BOX_ID_V_L },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
box_V_r_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_BOX_R) };

static const DIALOG_CONTROL
box_H_group =
{
    BOX_ID_H_GROUP, DIALOG_COL1_GROUP,
    { BOX_ID_V_GROUP, BOX_ID_V_GROUP, BOX_ID_H_B, BOX_ID_H_B },
    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
box_H_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_BOX_H), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
box_H_all =
{
    BOX_ID_H_ALL, BOX_ID_H_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
box_H_all_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_BOX_ALL) };

static const DIALOG_CONTROL
box_H_t =
{
    BOX_ID_H_T, BOX_ID_H_GROUP,
    { BOX_ID_H_ALL, BOX_ID_H_ALL, DIALOG_CONTROL_SELF, BOX_ID_H_ALL },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
box_H_t_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_BOX_T) };

static const DIALOG_CONTROL
box_H_b =
{
    BOX_ID_H_B, BOX_ID_H_GROUP,
    { BOX_ID_H_T, BOX_ID_H_T, DIALOG_CONTROL_SELF, BOX_ID_H_T },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
box_H_b_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_BOX_B) };

/*
RGB container
*/

static const DIALOG_CONTROL
rgb_group =
{
    DIALOG_ID_RGB_GROUP, DIALOG_COL2_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

/*
line style group
*/

static const DIALOG_CONTROL
box_line_group =
{
    BOX_ID_LINE_GROUP, DIALOG_COL1_GROUP,
    { BOX_ID_H_GROUP, BOX_ID_H_GROUP, BOX_ID_LINE_4, BOX_ID_LINE_4 },
    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
box_line_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_BOX_LINE_STYLE), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
box_line[5] =
{
    {
        BOX_ID_LINE_0, BOX_ID_LINE_GROUP, { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
        { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, LINE_STYLE_H, LINE_STYLE_V }, { DRT(LTLT, RADIOPICTURE), 1 /*tabstop*/, 1 /*logical_group*/ }
    },

    {
        BOX_ID_LINE_1, BOX_ID_LINE_GROUP, { BOX_ID_LINE_0, BOX_ID_LINE_0, DIALOG_CONTROL_SELF, BOX_ID_LINE_0 },
        { 0, 0, LINE_STYLE_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    },


    {
        BOX_ID_LINE_2, BOX_ID_LINE_GROUP, { BOX_ID_LINE_1, BOX_ID_LINE_1, DIALOG_CONTROL_SELF, BOX_ID_LINE_1 },
        { 0, 0, LINE_STYLE_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    },

    {
        BOX_ID_LINE_3, BOX_ID_LINE_GROUP, { BOX_ID_LINE_1, BOX_ID_LINE_1, BOX_ID_LINE_1 },
        { 0, 0, 0, LINE_STYLE_V }, { DRT(LBRT, RADIOPICTURE) }
    },

    {
        BOX_ID_LINE_4, BOX_ID_LINE_GROUP, { BOX_ID_LINE_3, BOX_ID_LINE_3, DIALOG_CONTROL_SELF, BOX_ID_LINE_3 },
        { 0, 0, LINE_STYLE_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    }
};

static const DIALOG_CTL_CREATE
box_ctl_create[] =
{
    { &dialog_main_group },

    { &dialog_col1_group },

    { &box_all,        &box_all_data },
    { &box_outside,    &box_outside_data },
    { &box_V_group,    &box_V_group_data },
    { &box_V_all,      &box_V_all_data },
    { &box_V_l,        &box_V_l_data },
    { &box_V_r,        &box_V_r_data },
    { &box_H_group,    &box_H_group_data },
    { &box_H_all,      &box_H_all_data },
    { &box_H_t,        &box_H_t_data },
    { &box_H_b,        &box_H_b_data },

    { &box_line_group, &box_line_group_data },
    { &box_line[0], &line_style_data[0] },
    { &box_line[1], &line_style_data[1] },
    { &box_line[2], &line_style_data[2] },
    { &box_line[3], &line_style_data[3] },
    { &box_line[4], &line_style_data[4] },

    { &dialog_col2_group },

    { &rgb_group,  &rgb_group_data },
    { &rgb_group_inner },

    { &rgb_tx[RGB_TX_IX_R], &rgb_tx_data[RGB_TX_IX_R] },
    { &rgb_bump[RGB_TX_IX_R], &rgb_bump_data },
    { &rgb_tx[RGB_TX_IX_G], &rgb_tx_data[RGB_TX_IX_G] },
    { &rgb_bump[RGB_TX_IX_G], &rgb_bump_data },
    { &rgb_tx[RGB_TX_IX_B], &rgb_tx_data[RGB_TX_IX_B] },
    { &rgb_bump[RGB_TX_IX_B], &rgb_bump_data },
    { &rgb_patch,      &rgb_patch_data },

    { &rgb_button,      &rgb_button_data },

    { &rgb_patches[0],  &rgb_patches_data[0] },
    { &rgb_patches[1],  &rgb_patches_data[1] },
    { &rgb_patches[2],  &rgb_patches_data[2] },
    { &rgb_patches[3],  &rgb_patches_data[3] },
    { &rgb_patches[4],  &rgb_patches_data[4] },
    { &rgb_patches[5],  &rgb_patches_data[5] },
    { &rgb_patches[6],  &rgb_patches_data[6] },
    { &rgb_patches[7],  &rgb_patches_data[7] },
    { &rgb_patches[8],  &rgb_patches_data[8] },
    { &rgb_patches[9],  &rgb_patches_data[9] },
    { &rgb_patches[10], &rgb_patches_data[10] },
    { &rgb_patches[11], &rgb_patches_data[11] },
    { &rgb_patches[12], &rgb_patches_data[12] },
    { &rgb_patches[13], &rgb_patches_data[13] },
    { &rgb_patches[14], &rgb_patches_data[14] },
    { &rgb_patches[15], &rgb_patches_data[15] },
    /* no transparent */

    { &defbutton_ok,     &box_ok_data },
    { &stdbutton_cancel, &stdbutton_cancel_data}
};

_Check_return_
static STATUS
clear_down(
    _InVal_     H_DIALOG h_dialog,
    _In_reads_(n_control_id) const PC_DIALOG_CTL_ID p_dialog_control_id,
    _InVal_     U32 n_control_id)
{
    STATUS status = STATUS_OK;
    U32 control_idx = 0;
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = h_dialog;
    dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_DONT_MSG;
    dialog_cmd_ctl_state_set.state.checkbox = DIALOG_BUTTONSTATE_OFF;

    for(control_idx = 0; control_idx < n_control_id; ++control_idx)
    {
        dialog_cmd_ctl_state_set.dialog_control_id = p_dialog_control_id[control_idx];
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
    }

    return(status);
}

static const DIALOG_CONTROL_ID
clear_all[]     = {             BOX_ID_OUTSIDE, BOX_ID_V_ALL, BOX_ID_V_L, BOX_ID_V_R, BOX_ID_H_ALL, BOX_ID_H_T, BOX_ID_H_B };

static const DIALOG_CONTROL_ID
clear_outside[] = { BOX_ID_ALL,                 BOX_ID_V_ALL, BOX_ID_V_L, BOX_ID_V_R, BOX_ID_H_ALL, BOX_ID_H_T, BOX_ID_H_B };

static const DIALOG_CONTROL_ID
clear_V_all[]   = { BOX_ID_ALL, BOX_ID_OUTSIDE,               BOX_ID_V_L, BOX_ID_V_R,                                      };

static const DIALOG_CONTROL_ID
clear_V_lr[]    = { BOX_ID_ALL, BOX_ID_OUTSIDE, BOX_ID_V_ALL,                                                              };

static const DIALOG_CONTROL_ID
clear_H_all[]   = { BOX_ID_ALL, BOX_ID_OUTSIDE,                                                     BOX_ID_H_T, BOX_ID_H_B };

static const DIALOG_CONTROL_ID
clear_H_tb[]    = { BOX_ID_ALL, BOX_ID_OUTSIDE,                                       BOX_ID_H_ALL,                        };

_Check_return_
static STATUS
rgb_patch_set(
    _InVal_     H_DIALOG h_dialog,
    _InRef_     PC_RGB p_rgb)
{
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = h_dialog;
    dialog_cmd_ctl_state_set.dialog_control_id = DIALOG_ID_RGB_PATCH;
    dialog_cmd_ctl_state_set.bits = 0;
    dialog_cmd_ctl_state_set.state.user.rgb = *p_rgb;
    return(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
}

_Check_return_
static STATUS
box_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case BOX_ID_ALL:     p_dialog_msg_ctl_create_state->state_set.state.checkbox = (U8) g_box_persistent_state.all;     break;
    case BOX_ID_OUTSIDE: p_dialog_msg_ctl_create_state->state_set.state.checkbox = (U8) g_box_persistent_state.outside; break;
    case BOX_ID_V_ALL:   p_dialog_msg_ctl_create_state->state_set.state.checkbox = (U8) g_box_persistent_state.V_all;   break;
    case BOX_ID_V_L:     p_dialog_msg_ctl_create_state->state_set.state.checkbox = (U8) g_box_persistent_state.V_l;     break;
    case BOX_ID_V_R:     p_dialog_msg_ctl_create_state->state_set.state.checkbox = (U8) g_box_persistent_state.V_r;     break;
    case BOX_ID_H_ALL:   p_dialog_msg_ctl_create_state->state_set.state.checkbox = (U8) g_box_persistent_state.H_all;   break;
    case BOX_ID_H_T:     p_dialog_msg_ctl_create_state->state_set.state.checkbox = (U8) g_box_persistent_state.H_t;     break;
    case BOX_ID_H_B:     p_dialog_msg_ctl_create_state->state_set.state.checkbox = (U8) g_box_persistent_state.H_b;     break;

    case BOX_ID_LINE_GROUP:
        p_dialog_msg_ctl_create_state->state_set.state.radiobutton = g_box_persistent_state.line_style;
        break;

    case DIALOG_ID_RGB_R:
    case DIALOG_ID_RGB_G:
    case DIALOG_ID_RGB_B:
    case DIALOG_ID_RGB_PATCH:
        p_dialog_msg_ctl_create_state->processed = 1; /* go no further */
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
box_ctl_pushbutton(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case DIALOG_ID_RGB_BUTTON:
        {
        MSG_UISTYLE_COLOUR_PICKER msg_uistyle_colour_picker;
        msg_uistyle_colour_picker.h_dialog = p_dialog_msg_ctl_pushbutton->h_dialog;
        msg_uistyle_colour_picker.rgb_dialog_control_id = DIALOG_ID_RGB_PATCH;
        msg_uistyle_colour_picker.button_dialog_control_id = DIALOG_ID_RGB_BUTTON;
        return(object_call_id(OBJECT_ID_SKEL_SPLIT, p_docu, T5_MSG_UISTYLE_COLOUR_PICKER, &msg_uistyle_colour_picker));
        }

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
box_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    status_return(rgb_patch_set(p_dialog_msg_process_start->h_dialog, (PC_RGB) p_dialog_msg_process_start->client_handle));
    ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, DIALOG_ID_RGB_BUTTON, g_has_colour_picker);
    return(STATUS_OK);
}

_Check_return_
static STATUS
box_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    STATUS status = STATUS_OK;
    const H_DIALOG h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
    PC_DIALOG_CTL_ID p_clear = NULL;
    U32 n_clear = 0;
    U32 shift = 0;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case BOX_ID_ALL:     p_clear = clear_all;     n_clear = elemof32(clear_all);     break;
    case BOX_ID_OUTSIDE: p_clear = clear_outside; n_clear = elemof32(clear_outside); break;
    case BOX_ID_V_ALL:   p_clear = clear_V_all;   n_clear = elemof32(clear_V_all);   break;
    case BOX_ID_H_ALL:   p_clear = clear_H_all;   n_clear = elemof32(clear_H_all);   break;
    case BOX_ID_V_L:
    case BOX_ID_V_R:     p_clear = clear_V_lr;    n_clear = elemof32(clear_V_lr);    break;
    case BOX_ID_H_T:
    case BOX_ID_H_B:     p_clear = clear_H_tb;    n_clear = elemof32(clear_H_tb);    break;

    case DIALOG_ID_RGB_B:
        shift += 8;

        /*FALLTHRU*/

    case DIALOG_ID_RGB_G:
        shift += 8;

        /*FALLTHRU*/

    case DIALOG_ID_RGB_R:
        {
        union DIALOG_CONTROL_DATA_USER_STATE user;

        user.u32 =
            (* (PC_U32) &g_box_persistent_state.rgb & ~(0x000000FFU << shift))
          | ((p_dialog_msg_ctl_state_change->new_state.bump_s32 & 0x000000FFU) << shift);

        return(rgb_patch_set(p_dialog_msg_ctl_state_change->h_dialog, &user.rgb));
        }

    case DIALOG_ID_RGB_PATCH:
        {
        g_box_persistent_state.rgb = p_dialog_msg_ctl_state_change->new_state.user.rgb;

        {
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;

        dialog_cmd_ctl_state_set.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
        dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_DONT_MSG;

        dialog_cmd_ctl_state_set.dialog_control_id = DIALOG_ID_RGB_R;
        dialog_cmd_ctl_state_set.state.bump_s32 = g_box_persistent_state.rgb.r;
        status_return(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));

        dialog_cmd_ctl_state_set.dialog_control_id = DIALOG_ID_RGB_G;
        dialog_cmd_ctl_state_set.state.bump_s32 = g_box_persistent_state.rgb.g;
        status_return(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));

        dialog_cmd_ctl_state_set.dialog_control_id = DIALOG_ID_RGB_B;
        dialog_cmd_ctl_state_set.state.bump_s32 = g_box_persistent_state.rgb.b;
        status_return(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
        } /*block*/

        return(STATUS_OK);
        }

    default:
        return(STATUS_OK);
    }

    if(p_dialog_msg_ctl_state_change->new_state.checkbox == DIALOG_BUTTONSTATE_ON)
        status = clear_down(h_dialog, p_clear, n_clear);

    return(status);
}

_Check_return_
static STATUS
box_ctl_user_mouse(
    _InRef_     PC_DIALOG_MSG_CTL_USER_MOUSE p_dialog_msg_ctl_user_mouse)
{
    const DIALOG_CONTROL_ID dialog_control_id = p_dialog_msg_ctl_user_mouse->dialog_control_id;

#if RISCOS
    if(p_dialog_msg_ctl_user_mouse->click != DIALOG_MSG_USER_MOUSE_CLICK_LEFT_SINGLE)
        return(STATUS_OK);
#endif

    if((dialog_control_id >= DIALOG_ID_RGB_0) && (dialog_control_id <= DIALOG_ID_RGB_15))
    {
        return(rgb_patch_set(p_dialog_msg_ctl_user_mouse->h_dialog, &rgb_stash[dialog_control_id - DIALOG_ID_RGB_0]));
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
box_ctl_user_redraw(
    _InRef_     PC_DIALOG_MSG_CTL_USER_REDRAW p_dialog_msg_ctl_user_redraw)
{
    RGB rgb = RGB_INIT_BLACK;

    switch(p_dialog_msg_ctl_user_redraw->dialog_control_id)
    {
    case DIALOG_ID_RGB_PATCH:
        {
        DIALOG_CMD_CTL_STATE_QUERY dialog_cmd_ctl_state_query;
        msgclr(dialog_cmd_ctl_state_query);
        dialog_cmd_ctl_state_query.h_dialog = p_dialog_msg_ctl_user_redraw->h_dialog;
        dialog_cmd_ctl_state_query.dialog_control_id = p_dialog_msg_ctl_user_redraw->dialog_control_id;
        dialog_cmd_ctl_state_query.bits = 0;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
        rgb = dialog_cmd_ctl_state_query.state.user.rgb;
        break;
        }

    default:
        return(STATUS_OK);
    }

#if RISCOS
    if(host_setbgcolour(&rgb))
        host_clg();
#endif

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_box)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(box_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(box_ctl_pushbutton(p_docu, (P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(box_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(box_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_CTL_USER_MOUSE:
        return(box_ctl_user_mouse((PC_DIALOG_MSG_CTL_USER_MOUSE) p_data));

    case DIALOG_MSG_CODE_CTL_USER_REDRAW:
        return(box_ctl_user_redraw((PC_DIALOG_MSG_CTL_USER_REDRAW) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* create a box over an area of a document
*
******************************************************************************/

T5_CMD_PROTO(extern, t5_cmd_box_intro)
{
    RGB rgb = g_box_persistent_state.rgb;
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, box_ctl_create, elemof32(box_ctl_create), MSG_DIALOG_BOX_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_BOX_CAPTION;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_box;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &rgb;
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

/******************************************************************************
*
* apply the results of the box dialog box to a given region list
*
******************************************************************************/

/* this is how the box command remembers its state from one time to the next! */

static void
box_setup_from_handle(
    _InRef_     PC_ARGLIST_HANDLE p_arglist_handle,
    _Inout_     BOX_PERSISTENT_STATE * const p_box_persistent_state)
{
    if(0 != *p_arglist_handle)
    {
        P_ARGLIST_ARG p_arg;

        if(arg_present(p_arglist_handle, 0, &p_arg))
            p_box_persistent_state->all = p_arg->val.fBool;

        if(arg_present(p_arglist_handle, 1, &p_arg))
            p_box_persistent_state->outside = p_arg->val.fBool;

        if(arg_present(p_arglist_handle, 2, &p_arg))
            p_box_persistent_state->V_all = p_arg->val.fBool;

        if(arg_present(p_arglist_handle, 3, &p_arg))
            p_box_persistent_state->V_l = p_arg->val.fBool;

        if(arg_present(p_arglist_handle, 4, &p_arg))
            p_box_persistent_state->V_r = p_arg->val.fBool;

        if(arg_present(p_arglist_handle, 5, &p_arg))
            p_box_persistent_state->H_all = p_arg->val.fBool;

        if(arg_present(p_arglist_handle, 6, &p_arg))
            p_box_persistent_state->H_t = p_arg->val.fBool;

        if(arg_present(p_arglist_handle, 7, &p_arg))
            p_box_persistent_state->H_b = p_arg->val.fBool;

        if(arg_present(p_arglist_handle, 8, &p_arg))
            p_box_persistent_state->line_style = p_arg->val.s32;

        if(arg_present(p_arglist_handle, 9, &p_arg))
            p_box_persistent_state->rgb = * (P_RGB) &p_arg->val.x32;
    }
}

T5_MSG_PROTO(extern, t5_msg_box_apply, P_BOX_APPLY p_box_apply)
{
    const PC_DOCU_AREA p_docu_area_in = &p_box_apply->docu_area;
    DOCU_AREA docu_area;
    BOX_PERSISTENT_STATE box_persistent_state;
    STYLE style;
    STYLE_SELECTOR empty_style_selector;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    style_init(&style);

    style_selector_clear(&empty_style_selector);

    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
    style_docu_area_add_parm.p_array_handle = p_box_apply->p_array_handle;
    style_docu_area_add_parm.data_space = p_box_apply->data_space;

    zero_struct(box_persistent_state);
    box_persistent_state.line_style = SF_BORDER_STANDARD;
    box_persistent_state.all = 1;

    box_setup_from_handle(&p_box_apply->arglist_handle, &box_persistent_state);

    g_box_persistent_state = box_persistent_state;

    style.para_style.rgb_grid_left   = box_persistent_state.rgb;
    style.para_style.rgb_grid_top    = box_persistent_state.rgb;
    style.para_style.rgb_grid_right  = box_persistent_state.rgb;
    style.para_style.rgb_grid_bottom = box_persistent_state.rgb;

    style.para_style.grid_left   =
    style.para_style.grid_right  =
    style.para_style.grid_top    =
    style.para_style.grid_bottom = (U8) box_persistent_state.line_style;

    if(box_persistent_state.all)
    {
        docu_area = *p_docu_area_in;

        style_selector_copy(&style.selector, &empty_style_selector);
        style_bit_set(&style, STYLE_SW_PS_RGB_GRID_LEFT);
        style_bit_set(&style, STYLE_SW_PS_RGB_GRID_RIGHT);
        style_bit_set(&style, STYLE_SW_PS_RGB_GRID_TOP);
        style_bit_set(&style, STYLE_SW_PS_RGB_GRID_BOTTOM);
        style_bit_set(&style, STYLE_SW_PS_GRID_LEFT);
        style_bit_set(&style, STYLE_SW_PS_GRID_RIGHT);
        style_bit_set(&style, STYLE_SW_PS_GRID_TOP);
        style_bit_set(&style, STYLE_SW_PS_GRID_BOTTOM);
        status_assert(status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
    }
    else if(box_persistent_state.outside)
    {
        UINT i;

        for(i = 0; i <= 3; ++i)
        {
            docu_area = *p_docu_area_in;

            style_selector_copy(&style.selector, &empty_style_selector);

            switch(i)
            {
            case 0:
                style_bit_set(&style, STYLE_SW_PS_RGB_GRID_LEFT);
                style_bit_set(&style, STYLE_SW_PS_GRID_LEFT);
                docu_area.br.slr.col = docu_area.tl.slr.col + 1;
                break;

            case 1:
                style_bit_set(&style, STYLE_SW_PS_RGB_GRID_RIGHT);
                style_bit_set(&style, STYLE_SW_PS_GRID_RIGHT);
                docu_area.tl.slr.col = docu_area.br.slr.col - 1;
                break;

            case 2:
                style_bit_set(&style, STYLE_SW_PS_RGB_GRID_TOP);
                style_bit_set(&style, STYLE_SW_PS_GRID_TOP);
                docu_area.br.slr.row = docu_area.tl.slr.row + 1;
                break;

            case 3:
                style_bit_set(&style, STYLE_SW_PS_RGB_GRID_BOTTOM);
                style_bit_set(&style, STYLE_SW_PS_GRID_BOTTOM);
                docu_area.tl.slr.row = docu_area.br.slr.row - 1;
                break;
            }

            status_assert(status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
        }
    }
    else
    {
        if(box_persistent_state.V_all)
        {
            docu_area = *p_docu_area_in;

            style_selector_copy(&style.selector, &empty_style_selector);
            style_bit_set(&style, STYLE_SW_PS_RGB_GRID_LEFT);
            style_bit_set(&style, STYLE_SW_PS_RGB_GRID_RIGHT);
            style_bit_set(&style, STYLE_SW_PS_GRID_LEFT);
            style_bit_set(&style, STYLE_SW_PS_GRID_RIGHT);
            status_assert(status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
        }
        else
        {
            if(box_persistent_state.V_l)
            {
                docu_area = *p_docu_area_in;

                style_selector_copy(&style.selector, &empty_style_selector);
                style_bit_set(&style, STYLE_SW_PS_RGB_GRID_LEFT);
                style_bit_set(&style, STYLE_SW_PS_GRID_LEFT);
                docu_area.br.slr.col = docu_area.tl.slr.col + 1;
                status_assert(status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
            }

            if(box_persistent_state.V_r)
            {
                docu_area = *p_docu_area_in;

                style_selector_copy(&style.selector, &empty_style_selector);
                style_bit_set(&style, STYLE_SW_PS_RGB_GRID_RIGHT);
                style_bit_set(&style, STYLE_SW_PS_GRID_RIGHT);
                docu_area.tl.slr.col = docu_area.br.slr.col - 1;
                status_assert(status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
            }
        }

        if(box_persistent_state.H_all)
        {
            docu_area = *p_docu_area_in;

            style_selector_copy(&style.selector, &empty_style_selector);
            style_bit_set(&style, STYLE_SW_PS_RGB_GRID_TOP);
            style_bit_set(&style, STYLE_SW_PS_RGB_GRID_BOTTOM);
            style_bit_set(&style, STYLE_SW_PS_GRID_TOP);
            style_bit_set(&style, STYLE_SW_PS_GRID_BOTTOM);
            status_assert(status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
        }
        else
        {
            if(box_persistent_state.H_t)
            {
                docu_area = *p_docu_area_in;

                style_selector_copy(&style.selector, &empty_style_selector);
                style_bit_set(&style, STYLE_SW_PS_RGB_GRID_TOP);
                style_bit_set(&style, STYLE_SW_PS_GRID_TOP);
                docu_area.br.slr.row = docu_area.tl.slr.row + 1;
                status_assert(status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
            }

            if(box_persistent_state.H_b)
            {
                docu_area = *p_docu_area_in;

                style_selector_copy(&style.selector, &empty_style_selector);
                style_bit_set(&style, STYLE_SW_PS_RGB_GRID_BOTTOM);
                style_bit_set(&style, STYLE_SW_PS_GRID_BOTTOM);
                docu_area.tl.slr.row = docu_area.br.slr.row - 1;
                status_assert(status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
            }
        }
    }

    return(status);
}

/* end of ui_remov.c */
