/* gr_blpro.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Handle chart process boxes */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

typedef struct BL_PROCESS_CALLBACK
{
    BOOL line_chart;

    BOOL slot_width_modified;
    BOOL slot_depth_modified;

    P_CHART_HEADER p_chart_header;
    GR_CHART_OBJID id;

    GR_BARCHSTYLE gr_barchstyle;
    GR_BARLINECHSTYLE gr_barlinechstyle;
    GR_LINECHSTYLE gr_linechstyle;
}
BL_PROCESS_CALLBACK, * P_BL_PROCESS_CALLBACK;

enum BL_CONTROL_IDS
{
    BL_ID_BL_GROUP = 242,
    BL_ID_BL_1,
    BL_ID_BL_1_TEXT,
    BL_ID_BL_2,
    BL_ID_BL_2_TEXT,
    BL_ID_BL_3,
    BL_ID_BL_3_TEXT,

    BL_ID_3D_GROUP,
    BL_ID_3D_ON,
    BL_ID_3D_GROUP_I,
    BL_ID_3D_PITCH,
    BL_ID_3D_PITCH_TEXT,
    BL_ID_3D_ROLL,
    BL_ID_3D_ROLL_TEXT
};

static const DIALOG_CONTROL
bl_process_bl_group =
{
    BL_ID_BL_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL
bl_process_bl_1 =
{
    BL_ID_BL_1, BL_ID_BL_GROUP,

    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },

    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },

    { DRT(LTLT, BUMP_F64) }
};

static const UI_CONTROL_F64
bl_process_bl_1_control = { 0.0, 100.0, 5.0 };

static const DIALOG_CONTROL_DATA_BUMP_F64
bl_process_bl_1_data = { { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/, &bl_process_bl_1_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
bl_process_bl_1_text =
{
    BL_ID_BL_1_TEXT, BL_ID_BL_GROUP,
    { BL_ID_BL_1, BL_ID_BL_1, DIALOG_CONTROL_SELF, BL_ID_BL_1 },
    { DIALOG_SMALLSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

static const DIALOG_CONTROL
bl_process_bl_2 =
{
    BL_ID_BL_2, BL_ID_BL_GROUP,
    { BL_ID_BL_1, BL_ID_BL_1, BL_ID_BL_1 },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64) }
};

static const UI_CONTROL_F64
bl_process_bl_2_control = { 0.0, 100.0, 5.0 };

static const DIALOG_CONTROL_DATA_BUMP_F64
bl_process_bl_2_data = { { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/, &bl_process_bl_2_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
bl_process_bl_2_text =
{
    BL_ID_BL_2_TEXT, BL_ID_BL_GROUP,
    { BL_ID_BL_1_TEXT, BL_ID_BL_2, DIALOG_CONTROL_SELF, BL_ID_BL_2 },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL
bl_process_bl_3 =
{
    BL_ID_BL_3, BL_ID_BL_GROUP,
    { BL_ID_BL_2, BL_ID_BL_2, BL_ID_BL_2 },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64) }
};

static const UI_CONTROL_F64
bl_process_bl_3_control = { 0.0, 100.0, 5.0 };

static const DIALOG_CONTROL_DATA_BUMP_F64
bl_process_bl_3_data = { { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/, &bl_process_bl_3_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
bl_process_bl_3_text =
{
    BL_ID_BL_3_TEXT, BL_ID_BL_GROUP,
    { BL_ID_BL_2_TEXT, BL_ID_BL_3, DIALOG_CONTROL_SELF, BL_ID_BL_3 },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

/*
3D group
*/

static const DIALOG_CONTROL
bl_process_3d_group =
{
    BL_ID_3D_GROUP, DIALOG_MAIN_GROUP,
    { BL_ID_BL_GROUP, BL_ID_BL_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
bl_process_3d_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_PROCESS_3D), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
bl_process_3d_on =
{
    BL_ID_3D_ON, BL_ID_3D_GROUP,
    { DIALOG_CONTROL_PARENT, BL_ID_3D_GROUP_I, DIALOG_CONTROL_SELF, BL_ID_3D_GROUP_I },
    { DIALOG_STDGROUP_LM, 0, DIALOG_STDCHECK_H, 0 },
    { DRT(LTLB, CHECKBOX) }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
bl_process_3d_on_data = { { 0 }, { UI_TEXT_TYPE_NONE } };

static const DIALOG_CONTROL
bl_process_3d_group_i =
{
    BL_ID_3D_GROUP_I, BL_ID_3D_GROUP,
    { BL_ID_3D_ON, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_CHECKGAP_H, DIALOG_STDGROUP_TM, 0, 0 },
    { DRT(RTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
bl_process_3d_roll =
{
    BL_ID_3D_ROLL, BL_ID_3D_GROUP_I,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },
    { DRT(LTLT, BUMP_F64) }
};

static const UI_CONTROL_F64
bl_process_3d_roll_control = { 0.0, 80.0, 5.0 };

static const DIALOG_CONTROL_DATA_BUMP_F64
bl_process_3d_roll_data = { { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/, &bl_process_3d_roll_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
bl_process_3d_roll_text =
{
    BL_ID_3D_ROLL_TEXT, BL_ID_3D_GROUP_I,
    { BL_ID_3D_ROLL, BL_ID_3D_ROLL, DIALOG_CONTROL_SELF, BL_ID_3D_ROLL },
    { DIALOG_SMALLSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
bl_process_3d_roll_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_PROCESS_3D_ROLL), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

static const DIALOG_CONTROL
bl_process_3d_pitch =
{
    BL_ID_3D_PITCH, BL_ID_3D_GROUP_I,
    { BL_ID_3D_ROLL, BL_ID_3D_ROLL, BL_ID_3D_ROLL },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64) }
};

static const UI_CONTROL_F64
bl_process_3d_pitch_control = { 0.0, 80.0, 5.0 };

static const DIALOG_CONTROL_DATA_BUMP_F64
bl_process_3d_pitch_data = { { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/, &bl_process_3d_pitch_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
bl_process_3d_pitch_text =
{
    BL_ID_3D_PITCH_TEXT, BL_ID_3D_GROUP_I,
    { BL_ID_3D_ROLL_TEXT, BL_ID_3D_PITCH, DIALOG_CONTROL_SELF, BL_ID_3D_PITCH },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
bl_process_3d_pitch_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_PROCESS_3D_PITCH), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

static const DIALOG_CONTROL_DATA_GROUPBOX
bar_process_bar_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BAR_PROCESS), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL_DATA_STATICTEXT
bar_process_1_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BAR_PROCESS_1), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

static const DIALOG_CONTROL_DATA_STATICTEXT
bar_process_2_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BAR_PROCESS_2), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

static const DIALOG_CONTROL_DATA_STATICTEXT
bar_process_3_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BAR_PROCESS_3), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

static const DIALOG_CONTROL
bar_process_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF, BL_ID_3D_GROUP, BL_ID_BL_GROUP },
    { DIALOG_CONTENTS_CALC, DIALOG_DEFPUSHBUTTON_V, 0, 0 },
    { DRT(RBRB, PUSHBUTTON), 1 }
};

static const DIALOG_CTL_CREATE
bar_process_ctl_create[] =
{
    { &dialog_main_group },

    { &bl_process_bl_group, &bar_process_bar_group_data },
    { &bl_process_bl_1, &bl_process_bl_1_data },
    { &bl_process_bl_1_text, &bar_process_1_text_data },
    { &bl_process_bl_2, &bl_process_bl_2_data },
    { &bl_process_bl_2_text, &bar_process_2_text_data },
    { &bl_process_bl_3, &bl_process_bl_3_data },
    { &bl_process_bl_3_text, &bar_process_3_text_data },

    { &bl_process_3d_group, &bl_process_3d_group_data },
    { &bl_process_3d_on, &bl_process_3d_on_data },
    { &bl_process_3d_group_i, NULL },
    { &bl_process_3d_roll, &bl_process_3d_roll_data },
    { &bl_process_3d_roll_text, &bl_process_3d_roll_text_data },
    { &bl_process_3d_pitch, &bl_process_3d_pitch_data },
    { &bl_process_3d_pitch_text, &bl_process_3d_pitch_text_data },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &bar_process_ok, &defbutton_ok_persist_data }
};

_Check_return_
static STATUS
dialog_bl_process_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_BL_PROCESS_CALLBACK p_bl_process_callback = (P_BL_PROCESS_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case BL_ID_3D_ON:
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, BL_ID_3D_GROUP_I, (p_dialog_msg_ctl_state_change->new_state.checkbox == DIALOG_BUTTONSTATE_ON));
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, BL_ID_BL_3, (p_dialog_msg_ctl_state_change->new_state.checkbox == DIALOG_BUTTONSTATE_ON));
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, BL_ID_BL_3_TEXT,  (p_dialog_msg_ctl_state_change->new_state.checkbox == DIALOG_BUTTONSTATE_ON));
        break;

    case BL_ID_BL_1:
        p_bl_process_callback->slot_width_modified = TRUE;
        break;

    case BL_ID_BL_3:
        p_bl_process_callback->slot_width_modified = TRUE;
        p_bl_process_callback->slot_depth_modified = TRUE;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_bl_process_msg_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_BL_PROCESS_CALLBACK p_bl_process_callback = (P_BL_PROCESS_CALLBACK) p_dialog_msg_process_start->client_handle;
    const H_DIALOG h_dialog = p_dialog_msg_process_start->h_dialog;
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_bl_process_callback->p_chart_header->ch);

    if(p_bl_process_callback->line_chart)
        gr_chart_objid_linechstyle_query(cp, p_bl_process_callback->id, &p_bl_process_callback->gr_linechstyle);
    else
        gr_chart_objid_barchstyle_query(cp, p_bl_process_callback->id, &p_bl_process_callback->gr_barchstyle);

    gr_chart_objid_barlinechstyle_query(cp, p_bl_process_callback->id, &p_bl_process_callback->gr_barlinechstyle);

    status_return(ui_dlg_set_f64(h_dialog, BL_ID_BL_1, (p_bl_process_callback->line_chart ? &p_bl_process_callback->gr_linechstyle.slot_width_percentage : &p_bl_process_callback->gr_barchstyle.slot_width_percentage)));
    status_return(ui_dlg_set_f64(h_dialog, BL_ID_BL_2, &cp->barch.slot_overlap_percentage));
    status_return(ui_dlg_set_f64(h_dialog, BL_ID_BL_3, &p_bl_process_callback->gr_barlinechstyle.slot_depth_percentage));

    status_return(ui_dlg_set_check_forcing(h_dialog, BL_ID_3D_ON, cp->d3.bits.on));

    status_return(ui_dlg_set_f64(h_dialog, BL_ID_3D_ROLL, &cp->d3.roll));
    status_return(ui_dlg_set_f64(h_dialog, BL_ID_3D_PITCH, &cp->d3.pitch));

    p_bl_process_callback->slot_width_modified = TRUE; /* wot de fook are dese says SKS */
    p_bl_process_callback->slot_depth_modified = TRUE;

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_bl_process_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_BL_PROCESS_CALLBACK p_bl_process_callback = (P_BL_PROCESS_CALLBACK) p_dialog_msg_process_end->client_handle;
        const H_DIALOG h_dialog = p_dialog_msg_process_end->h_dialog;
        const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_bl_process_callback->p_chart_header->ch);

        chart_modify_docu(p_bl_process_callback->p_chart_header);

        ui_dlg_get_f64(h_dialog, BL_ID_BL_1, &p_bl_process_callback->gr_linechstyle.slot_width_percentage);

        ui_dlg_get_f64(h_dialog, BL_ID_BL_1, &p_bl_process_callback->gr_barchstyle.slot_width_percentage);
        ui_dlg_get_f64(h_dialog, BL_ID_BL_2, &cp->barch.slot_overlap_percentage);
        ui_dlg_get_f64(h_dialog, BL_ID_BL_3, &p_bl_process_callback->gr_barlinechstyle.slot_depth_percentage);

        if(p_bl_process_callback->slot_width_modified)
        {
            p_bl_process_callback->gr_barchstyle.bits.manual = 1;
            status_return(gr_chart_objid_barchstyle_set(cp, p_bl_process_callback->id, &p_bl_process_callback->gr_barchstyle));
        }

        if(p_bl_process_callback->slot_depth_modified)
        {
            p_bl_process_callback->gr_barlinechstyle.bits.manual = 1;
            status_return(gr_chart_objid_barlinechstyle_set(cp, p_bl_process_callback->id, &p_bl_process_callback->gr_barlinechstyle));
        }

        if((cp->d3.bits.on = ui_dlg_get_check(h_dialog, BL_ID_3D_ON)) != FALSE)
        {
            ui_dlg_get_f64(h_dialog, BL_ID_3D_ROLL, &cp->d3.roll);
            ui_dlg_get_f64(h_dialog, BL_ID_3D_PITCH, &cp->d3.pitch);
        }
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_bl_process)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_bl_process_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_bl_process_msg_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_bl_process_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
gr_chart_bar_process(
    P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id)
{
    BL_PROCESS_CALLBACK bl_process_callback;
    STATUS status;

    bl_process_callback.p_chart_header = p_chart_header;
    bl_process_callback.id = id;
    bl_process_callback.line_chart = FALSE;

    for(;;)
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        UCHARZ buffer[BUF_MAX_GR_CHART_OBJID_REPR];
        gr_chart_object_name_from_id(p_gr_chart_from_chart_handle(p_chart_header->ch), id, ustr_bptr(buffer), elemof32(buffer));
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, bar_process_ctl_create, elemof32(bar_process_ctl_create), CHART_MSG_DIALOG_BAR_PROCESS_HELP_TOPIC);
        dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_USTR_TEMP;
        dialog_cmd_process_dbox.caption.text.ustr = ustr_bptr(buffer);
        dialog_cmd_process_dbox.p_proc_client = dialog_event_bl_process;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &bl_process_callback;
        status = call_dialog_with_docu(p_docu_from_docno(p_chart_header->docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        if(status != DIALOG_COMPLETION_OK_PERSIST)
            break;
    }

    return(status);
}

static const DIALOG_CONTROL_DATA_GROUPBOX
line_process_line_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_LINE_PROCESS), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL_DATA_STATICTEXT
line_process_1_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_LINE_PROCESS_1), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

static const DIALOG_CONTROL_DATA_STATICTEXT
line_process_2_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_LINE_PROCESS_2), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

static const DIALOG_CONTROL_DATA_STATICTEXT
line_process_3_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_LINE_PROCESS_3), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

static const DIALOG_CONTROL
line_process_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF, BL_ID_3D_GROUP, BL_ID_BL_GROUP },
    { DIALOG_CONTENTS_CALC, DIALOG_DEFPUSHBUTTON_V, 0, 0 },
    { DRT(RBRB, PUSHBUTTON), 1 }
};

static const DIALOG_CTL_CREATE
line_process_ctl_create[] =
{
    { &dialog_main_group },

    { &bl_process_bl_group, &line_process_line_group_data },
    { &bl_process_bl_1, &bl_process_bl_1_data },
    { &bl_process_bl_1_text, &line_process_1_text_data },
    { &bl_process_bl_2, &bl_process_bl_2_data },
    { &bl_process_bl_2_text, &line_process_2_text_data },
    { &bl_process_bl_3, &bl_process_bl_3_data },
    { &bl_process_bl_3_text, &line_process_3_text_data },

    { &bl_process_3d_group, &bl_process_3d_group_data },
    { &bl_process_3d_on, &bl_process_3d_on_data },
    { &bl_process_3d_group_i, NULL },
    { &bl_process_3d_roll, &bl_process_3d_roll_data },
    { &bl_process_3d_roll_text, &bl_process_3d_roll_text_data },
    { &bl_process_3d_pitch, &bl_process_3d_pitch_data },
    { &bl_process_3d_pitch_text, &bl_process_3d_pitch_text_data },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &line_process_ok, &defbutton_ok_persist_data }
};

_Check_return_
static STATUS
gr_chart_line_process(
    P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id)
{
    BL_PROCESS_CALLBACK bl_process_callback;
    STATUS status;

    bl_process_callback.p_chart_header = p_chart_header;
    bl_process_callback.id = id;
    bl_process_callback.line_chart = TRUE;

    for(;;)
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        UCHARZ buffer[BUF_MAX_GR_CHART_OBJID_REPR];
        gr_chart_object_name_from_id(p_gr_chart_from_chart_handle(p_chart_header->ch), id, ustr_bptr(buffer), elemof32(buffer));
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, line_process_ctl_create, elemof32(line_process_ctl_create), CHART_MSG_DIALOG_LINE_PROCESS_HELP_TOPIC);
        dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_USTR_TEMP;
        dialog_cmd_process_dbox.caption.text.ustr = ustr_bptr(buffer);
        dialog_cmd_process_dbox.p_proc_client = dialog_event_bl_process;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &bl_process_callback;
        status = call_dialog_with_docu(p_docu_from_docno(p_chart_header->docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        if(status != DIALOG_COMPLETION_OK_PERSIST)
            break;
    }

    return(status);
}

_Check_return_
static STATUS
gr_chart_pie_process(
    P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id)
{
    IGNOREPARM(p_chart_header);
    IGNOREPARM_InVal_(id);
    host_bleep();
    return(STATUS_OK);
}

/*
SCATTER chart process
*/

typedef struct SCAT_PROCESS_CALLBACK
{
    P_CHART_HEADER p_chart_header;
    GR_CHART_OBJID id;

    GR_SCATCHSTYLE gr_scatchstyle;
}
SCAT_PROCESS_CALLBACK, * P_SCAT_PROCESS_CALLBACK;

static const DIALOG_CONTROL_DATA_GROUPBOX
scat_process_point_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_SCAT_PROCESS), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL_DATA_STATICTEXT
scat_process_1_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_SCAT_PROCESS_1), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

static const DIALOG_CONTROL
scat_process_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, BL_ID_BL_GROUP, BL_ID_BL_GROUP, DIALOG_CONTROL_SELF },
    { DIALOG_CONTENTS_CALC, 0, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CTL_CREATE
scat_process_ctl_create[] =
{
    { &dialog_main_group },

    { &bl_process_bl_group, &scat_process_point_group_data },
    { &bl_process_bl_1, &bl_process_bl_1_data },
    { &bl_process_bl_1_text, &scat_process_1_text_data },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &scat_process_ok, &defbutton_ok_persist_data }
};

_Check_return_
static STATUS
dialog_scat_process_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_SCAT_PROCESS_CALLBACK p_scat_process_callback = (P_SCAT_PROCESS_CALLBACK) p_dialog_msg_process_start->client_handle;
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_scat_process_callback->p_chart_header->ch);
    gr_chart_objid_scatchstyle_query(cp, p_scat_process_callback->id, &p_scat_process_callback->gr_scatchstyle);
    return(ui_dlg_set_f64(p_dialog_msg_process_start->h_dialog, BL_ID_BL_1, &p_scat_process_callback->gr_scatchstyle.width_percentage));
}

_Check_return_
static STATUS
dialog_scat_process_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    STATUS status = STATUS_OK;

    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_SCAT_PROCESS_CALLBACK p_scat_process_callback = (P_SCAT_PROCESS_CALLBACK) p_dialog_msg_process_end->client_handle;
        const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_scat_process_callback->p_chart_header->ch);

        chart_modify_docu(p_scat_process_callback->p_chart_header);

        ui_dlg_get_f64(p_dialog_msg_process_end->h_dialog, BL_ID_BL_1, &p_scat_process_callback->gr_scatchstyle.width_percentage);
        p_scat_process_callback->gr_scatchstyle.bits.manual = 1;
        status = gr_chart_objid_scatchstyle_set(cp, p_scat_process_callback->id, &p_scat_process_callback->gr_scatchstyle);
    }

    return(status);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_scat_process)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_scat_process_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_scat_process_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
gr_chart_scat_process(
    P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id)
{
    SCAT_PROCESS_CALLBACK scat_process_callback;
    STATUS status;

    scat_process_callback.p_chart_header = p_chart_header;
    scat_process_callback.id = id;

    for(;;)
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        UCHARZ buffer[BUF_MAX_GR_CHART_OBJID_REPR];
        gr_chart_object_name_from_id(p_gr_chart_from_chart_handle(p_chart_header->ch), id, ustr_bptr(buffer), elemof32(buffer));
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, scat_process_ctl_create, elemof32(scat_process_ctl_create), CHART_MSG_DIALOG_SCAT_PROCESS_HELP_TOPIC);
        dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_USTR_TEMP;
        dialog_cmd_process_dbox.caption.text.ustr = ustr_bptr(buffer);
        dialog_cmd_process_dbox.p_proc_client = dialog_event_scat_process;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &scat_process_callback;
        status = call_dialog_with_docu(p_docu_from_docno(p_chart_header->docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        if(status != DIALOG_COMPLETION_OK_PERSIST)
            break;
    }

    return(status);
}

_Check_return_
extern STATUS
gr_chart_process(
    P_CHART_HEADER p_chart_header,
    _In_        GR_CHART_OBJID id)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_header->ch);
    GR_CHART_TYPE chart_type;

    if(cp->axes[0].chart_type == GR_CHART_TYPE_PIE)
        return(gr_chart_pie_process(p_chart_header, id));

    if(cp->axes[0].chart_type == GR_CHART_TYPE_SCAT)
        return(gr_chart_scat_process(p_chart_header, id));

    switch(id.name)
    {
    case GR_CHART_OBJNAME_AXIS:
    case GR_CHART_OBJNAME_AXISGRID:
    case GR_CHART_OBJNAME_AXISTICK:
        {
        GR_AXES_IDX axes_idx;
        GR_AXIS_IDX axis_idx = gr_axes_idx_from_external(cp, id.no, &axes_idx);

        chart_type = cp->axes[axes_idx].chart_type;

        gr_chart_objid_from_axes_idx(cp, axes_idx, axis_idx, &id);

        break;
        }

    case GR_CHART_OBJNAME_DROPSERIES:
    case GR_CHART_OBJNAME_BESTFITSER:
        id.name = GR_CHART_OBJNAME_SERIES;

        /*FALLTHRU*/

    case GR_CHART_OBJNAME_SERIES:
        chart_type = gr_seriesp_from_external(cp, id.no)->chart_type;
        break;

    case GR_CHART_OBJNAME_DROPPOINT:
        id.name = GR_CHART_OBJNAME_POINT;

        /*FALLTHRU*/

    case GR_CHART_OBJNAME_POINT:
        chart_type = gr_seriesp_from_external(cp, id.no)->chart_type;
        break;

    default:
        chart_type = cp->axes[0].chart_type;
        gr_chart_objid_from_axes_idx(cp, 0, 0, &id);
        break;
    }

    if(chart_type == GR_CHART_TYPE_BAR)
        return(gr_chart_bar_process(p_chart_header, id));

    return(gr_chart_line_process(p_chart_header, id));
}

/* end of gr_blpro.c */
