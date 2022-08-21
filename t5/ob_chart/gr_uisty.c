/* gr_uisty.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Chart style UI for Fireworkz */

/* SKS June 1993 */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

#include "ob_skspt/xp_uisty.h"

#if RISCOS
#include "ob_dlg/xp_dlgr.h"
#endif

/*
internal routines
*/

#define HEIGHT_WIDTH_MAX_VAL 1000

#define LINE_THICKNESS_MAX_VAL 1000

typedef struct FLT_CALLBACK
{
    P_CHART_HEADER p_chart_header;
    GR_CHART_OBJID id;

    RGB rgb;
    BOOL rgb_modified;
    BOOL other_modified;
    BOOL has_fillstyleb;

    GR_FILLSTYLEB gr_fillstyleb;

    GR_FILLSTYLEC gr_fillstylec;

    GR_LINESTYLE gr_linestyle;

    GR_TEXTSTYLE gr_textstyle;
}
FLT_CALLBACK, * P_FLT_CALLBACK;

/******************************************************************************
*
* text style
*
******************************************************************************/

enum TS_FS_ID
{
    STYLE_ID_TEXT_GROUP = 500,
    STYLE_ID_TEXT_TYPEFACE_LIST,
    STYLE_ID_TEXT_TYPEFACE_TEXT,
    STYLE_ID_TEXT_HEIGHT,
    STYLE_ID_TEXT_HEIGHT_TEXT,
    STYLE_ID_TEXT_HEIGHT_UNITS,
    STYLE_ID_TEXT_WIDTH,
    STYLE_ID_TEXT_WIDTH_TEXT,
    STYLE_ID_TEXT_WIDTH_UNITS,
#if RISCOS
#define TS_FS_TYPEFACE_H    (420 * PIXITS_PER_RISCOS)
#else
#define TS_FS_TYPEFACE_H    ((20 * PIXITS_PER_INCH) / 10)
#endif
#define TS_FS_TYPEFACE_V    ((8 + 2) * PIXITS_PER_INCH / 8)
#define TS_FS_SIZE_TEXT_H   (/*DIALOG_STDCHECK_H + DIALOG_CHECKGAP_H +*/ DIALOG_SYSCHARS_H("Height"))
#define TS_FS_SIZE_FIELDS_H DIALOG_BUMP_H(4)

    STYLE_ID_TEXT_BOLD = 350,
    STYLE_ID_TEXT_ITALIC,
#if RISCOS && 0
#define TS_FS_TYPE_H    ((44 * PIXITS_PER_RISCOS) + DIALOG_PUSHPICTUREOVH_H)
#define TS_FS_TYPE_V    ((44 * PIXITS_PER_RISCOS) + DIALOG_PUSHPICTUREOVH_V)
#else
#define TS_FS_TYPE_H    PIXITS_PER_STDTOOL_H
#define TS_FS_TYPE_V    PIXITS_PER_STDTOOL_V
#endif

    STYLE_ID_LINE_PATTERN_GROUP = 600,
    STYLE_ID_LINE_LINE_0,
    STYLE_ID_LINE_LINE_1,
    STYLE_ID_LINE_LINE_2,
    STYLE_ID_LINE_LINE_3,
    STYLE_ID_LINE_LINE_4,
    STYLE_ID_LINE_LINE_5,
    STYLE_ID_LINE_LINE_6,

    STYLE_ID_LINE_THICKNESS,
    STYLE_ID_LINE_THICKNESS_UNITS,

    STYLE_ID_FILL_GROUP,
    STYLE_ID_FILL_SOLID,
    STYLE_ID_FILL_PICTURE,
    STYLE_ID_FILL_ASPECT,
    STYLE_ID_FILL_RECOLOUR
};

/*
common structures
*/

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
rgb_class_handler_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    U32 shift = 0;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case DIALOG_ID_RGB_B:
        shift += 8;

        /*FALLTHRU*/

    case DIALOG_ID_RGB_G:
        shift += 8;

        /*FALLTHRU*/

    case DIALOG_ID_RGB_R:
        {
        const P_FLT_CALLBACK p_flt_callback = (P_FLT_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;
        union DIALOG_CONTROL_DATA_USER_STATE user;

        user.u32 =
            (* (PC_U32) &p_flt_callback->rgb & ~(0x000000FFU << shift))
          | (((U32) p_dialog_msg_ctl_state_change->new_state.bump_s32 & 0x000000FFU) << shift);

        p_flt_callback->rgb_modified = 1;

        return(rgb_patch_set(p_dialog_msg_ctl_state_change->h_dialog, &user.rgb));
        }

    case DIALOG_ID_RGB_PATCH:
        {
        const P_FLT_CALLBACK p_flt_callback = (P_FLT_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;

        p_flt_callback->rgb_modified = 1;

        p_flt_callback->rgb = p_dialog_msg_ctl_state_change->new_state.user.rgb;

        msgclr(dialog_cmd_ctl_state_set);
        dialog_cmd_ctl_state_set.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
        dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_DONT_MSG;

        dialog_cmd_ctl_state_set.dialog_control_id = DIALOG_ID_RGB_R;
        dialog_cmd_ctl_state_set.state.bump_s32 = p_flt_callback->rgb.r;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));

        dialog_cmd_ctl_state_set.dialog_control_id = DIALOG_ID_RGB_G;
        dialog_cmd_ctl_state_set.state.bump_s32 = p_flt_callback->rgb.g;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));

        dialog_cmd_ctl_state_set.dialog_control_id = DIALOG_ID_RGB_B;
        dialog_cmd_ctl_state_set.state.bump_s32 = p_flt_callback->rgb.b;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));

        /*ui_dlg_set_check(h_dialog, DIALOG_ID_RGB_AUTOMATIC, 0);*/

        break;
        }

    case DIALOG_ID_RGB_T:
        {
        const P_FLT_CALLBACK p_flt_callback = (P_FLT_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;

        p_flt_callback->rgb_modified = 1;

        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, DIALOG_ID_RGB_GROUP_INNER, (p_dialog_msg_ctl_state_change->new_state.checkbox != DIALOG_BUTTONSTATE_ON));

        /*ui_dlg_set_check(p_dialog_msg_ctl_state_change->h_dialog, DIALOG_ID_RGB_AUTOMATIC, 0);*/
        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
rgb_class_handler_ctl_user_mouse(
    _InRef_     PC_DIALOG_MSG_CTL_USER_MOUSE p_dialog_msg_ctl_user_mouse)
{
    const DIALOG_CONTROL_ID dialog_control_id = p_dialog_msg_ctl_user_mouse->dialog_control_id;

    if(p_dialog_msg_ctl_user_mouse->click != DIALOG_MSG_USER_MOUSE_CLICK_LEFT_SINGLE)
        return(STATUS_OK);

    if((dialog_control_id >= DIALOG_ID_RGB_0) && (dialog_control_id <= DIALOG_ID_RGB_15))
    {
        return(rgb_patch_set(p_dialog_msg_ctl_user_mouse->h_dialog, &rgb_stash[dialog_control_id - DIALOG_ID_RGB_0]));
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
rgb_class_handler_ctl_user_redraw(
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

_Check_return_
static STATUS
rgb_class_handler(
    _InVal_     DIALOG_MESSAGE dialog_message,
    /*_Inout_*/ P_ANY p_data)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(rgb_class_handler_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_CTL_USER_MOUSE:
        return(rgb_class_handler_ctl_user_mouse((PC_DIALOG_MSG_CTL_USER_MOUSE) p_data));

    case DIALOG_MSG_CODE_CTL_USER_REDRAW:
        return(rgb_class_handler_ctl_user_redraw((PC_DIALOG_MSG_CTL_USER_REDRAW) p_data));

    default: default_unhandled();
        return(STATUS_OK);
    }
}

static void
control_data_setup(
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _Inout_     UI_CONTROL_F64 * const p_ui_control_f64)
{
    static UCHARZ control_data_points_numform_ustr_buf[16];

    resource_lookup_ustr_buffer(ustr_bptr(control_data_points_numform_ustr_buf), elemof32(control_data_points_numform_ustr_buf), MSG_DIALOG_STYLE_NUMFORM_POINTS);

    p_ui_control_f64->ustr_numform  = ustr_bptr(control_data_points_numform_ustr_buf);
    p_ui_control_f64->inc_dec_round = 1.0;
    if(STYLE_ID_LINE_THICKNESS == dialog_control_id)
        p_ui_control_f64->inc_dec_round = 10.0;
    p_ui_control_f64->bump_val      = 1.0 / p_ui_control_f64->inc_dec_round;
    p_ui_control_f64->min_val       = 0.0;
    if(STYLE_ID_TEXT_HEIGHT == dialog_control_id)
        p_ui_control_f64->min_val   = 1.0;
    if(STYLE_ID_LINE_THICKNESS == dialog_control_id)
        p_ui_control_f64->max_val   = (F64) LINE_THICKNESS_MAX_VAL;
    else
        p_ui_control_f64->max_val   = (F64) HEIGHT_WIDTH_MAX_VAL;
}

/******************************************************************************
*
* fill style
*
******************************************************************************/

static const DIALOG_CONTROL
style_rgb_group =
{
    DIALOG_ID_RGB_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
style_fill_colour_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_STYLE_FILL_COLOUR), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
style_fill_group =
{
    STYLE_ID_FILL_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_ID_RGB_GROUP, DIALOG_ID_RGB_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
style_fill_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_STYLE_FILL), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
style_fill_solid =
{
    STYLE_ID_FILL_SOLID, STYLE_ID_FILL_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_STDCHECK_H + DIALOG_CHECKGAP_H + 2 * PIXITS_PER_INCH, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
style_fill_solid_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_STYLE_FILL_SOLID) };

static const DIALOG_CONTROL
style_fill_picture =
{
    STYLE_ID_FILL_PICTURE, STYLE_ID_FILL_GROUP,
    { STYLE_ID_FILL_SOLID, STYLE_ID_FILL_SOLID, STYLE_ID_FILL_SOLID },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCHECK_V },
    { DRT(LBRT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
style_fill_picture_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_STYLE_FILL_PICTURE) };

static const DIALOG_CONTROL
style_fill_aspect =
{
    STYLE_ID_FILL_ASPECT, STYLE_ID_FILL_GROUP,
    { STYLE_ID_FILL_PICTURE, STYLE_ID_FILL_PICTURE, STYLE_ID_FILL_PICTURE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCHECK_V },
    { DRT(LBRT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
style_fill_aspect_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_STYLE_FILL_ASPECT) };

static const DIALOG_CONTROL
style_fill_recolour =
{
    STYLE_ID_FILL_RECOLOUR, STYLE_ID_FILL_GROUP,
    { STYLE_ID_FILL_ASPECT, STYLE_ID_FILL_ASPECT, STYLE_ID_FILL_ASPECT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCHECK_V },
    { DRT(LBRT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
style_fill_recolour_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_STYLE_FILL_RECOLOUR) };

static const DIALOG_CTL_CREATE
style_fill_ctl_create[] =
{
    { &dialog_main_group },
    { &style_rgb_group, &style_fill_colour_group_data },
    { &rgb_group_inner },
    { &rgb_tx[RGB_TX_IX_R], &rgb_tx_data[RGB_TX_IX_R] },
    { &rgb_bump[RGB_TX_IX_R], &rgb_bump_data },
    { &rgb_tx[RGB_TX_IX_G], &rgb_tx_data[RGB_TX_IX_G] },
    { &rgb_bump[RGB_TX_IX_G], &rgb_bump_data },
    { &rgb_tx[RGB_TX_IX_B], &rgb_tx_data[RGB_TX_IX_B] },
    { &rgb_bump[RGB_TX_IX_B], &rgb_bump_data },
    { &rgb_patch, &rgb_patch_data },
    { &rgb_button, &rgb_button_data },
    { &rgb_patches[0], &rgb_patches_data[0] },
    { &rgb_patches[1], &rgb_patches_data[1] },
    { &rgb_patches[2], &rgb_patches_data[2] },
    { &rgb_patches[3], &rgb_patches_data[3] },
    { &rgb_patches[4], &rgb_patches_data[4] },
    { &rgb_patches[5], &rgb_patches_data[5] },
    { &rgb_patches[6], &rgb_patches_data[6] },
    { &rgb_patches[7], &rgb_patches_data[7] },
    { &rgb_patches[8], &rgb_patches_data[8] },
    { &rgb_patches[9], &rgb_patches_data[9] },
    { &rgb_patches[10], &rgb_patches_data[10] },
    { &rgb_patches[11], &rgb_patches_data[11] },
    { &rgb_patches[12], &rgb_patches_data[12] },
    { &rgb_patches[13], &rgb_patches_data[13] },
    { &rgb_patches[14], &rgb_patches_data[14] },
    { &rgb_patches[15], &rgb_patches_data[15] },
    { &rgb_transparent, &rgb_transparent_data },

    { &style_fill_group, &style_fill_group_data },
    { &style_fill_solid, &style_fill_solid_data },
    { &style_fill_picture, &style_fill_picture_data },
    { &style_fill_aspect, &style_fill_aspect_data },
    { &style_fill_recolour, &style_fill_recolour_data },

    { &defbutton_ok, &defbutton_ok_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

static const DIALOG_CTL_CREATE
style_fill_pie_ctl_create[] =
{
    { &dialog_main_group },
    { &style_rgb_group, &style_fill_colour_group_data },
    { &rgb_group_inner },
    { &rgb_tx[RGB_TX_IX_R], &rgb_tx_data[RGB_TX_IX_R] },
    { &rgb_bump[RGB_TX_IX_R], &rgb_bump_data },
    { &rgb_tx[RGB_TX_IX_G], &rgb_tx_data[RGB_TX_IX_G] },
    { &rgb_bump[RGB_TX_IX_G], &rgb_bump_data },
    { &rgb_tx[RGB_TX_IX_B], &rgb_tx_data[RGB_TX_IX_B] },
    { &rgb_bump[RGB_TX_IX_B], &rgb_bump_data },
    { &rgb_patch, &rgb_patch_data },
    { &rgb_button, &rgb_button_data },
    { &rgb_patches[0], &rgb_patches_data[0] },
    { &rgb_patches[1], &rgb_patches_data[1] },
    { &rgb_patches[2], &rgb_patches_data[2] },
    { &rgb_patches[3], &rgb_patches_data[3] },
    { &rgb_patches[4], &rgb_patches_data[4] },
    { &rgb_patches[5], &rgb_patches_data[5] },
    { &rgb_patches[6], &rgb_patches_data[6] },
    { &rgb_patches[7], &rgb_patches_data[7] },
    { &rgb_patches[8], &rgb_patches_data[8] },
    { &rgb_patches[9], &rgb_patches_data[9] },
    { &rgb_patches[10], &rgb_patches_data[10] },
    { &rgb_patches[11], &rgb_patches_data[11] },
    { &rgb_patches[12], &rgb_patches_data[12] },
    { &rgb_patches[13], &rgb_patches_data[13] },
    { &rgb_patches[14], &rgb_patches_data[14] },
    { &rgb_patches[15], &rgb_patches_data[15] },
    { &rgb_transparent, &rgb_transparent_data },

    { &defbutton_ok, &defbutton_ok_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

_Check_return_
static STATUS
dialog_style_fill_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_FLT_CALLBACK p_flt_callback = (P_FLT_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case STYLE_ID_FILL_ASPECT:
    case STYLE_ID_FILL_RECOLOUR:
    case STYLE_ID_FILL_SOLID:
        {
        p_flt_callback->other_modified = 1;
        break;
        }

    case STYLE_ID_FILL_PICTURE:
        {
        BOOL enabled = (p_dialog_msg_ctl_state_change->new_state.checkbox == DIALOG_BUTTONSTATE_ON);
        p_flt_callback->other_modified = 1;
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, STYLE_ID_FILL_ASPECT, enabled);
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, STYLE_ID_FILL_RECOLOUR, enabled);
        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_style_fill_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    STATUS status = STATUS_OK;

    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case DIALOG_ID_RGB_BUTTON:
        {
        MSG_UISTYLE_COLOUR_PICKER msg_uistyle_colour_picker;
        msg_uistyle_colour_picker.h_dialog = p_dialog_msg_ctl_pushbutton->h_dialog;
        msg_uistyle_colour_picker.rgb_dialog_control_id = DIALOG_ID_RGB_PATCH;
        msg_uistyle_colour_picker.button_dialog_control_id = DIALOG_ID_RGB_BUTTON;
        status = object_call_id_load(P_DOCU_NONE, T5_MSG_UISTYLE_COLOUR_PICKER, &msg_uistyle_colour_picker, OBJECT_ID_SKEL_SPLIT);
        break;
        }

    default:
        break;
    }

    return(status);
}

_Check_return_
static STATUS
dialog_style_fill_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_FLT_CALLBACK p_flt_callback = (P_FLT_CALLBACK) p_dialog_msg_process_start->client_handle;
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_flt_callback->p_chart_header->ch);

    if(p_flt_callback->has_fillstyleb)
        gr_chart_objid_fillstyleb_query(cp, p_flt_callback->id, &p_flt_callback->gr_fillstyleb);

    gr_chart_objid_fillstylec_query(cp, p_flt_callback->id, &p_flt_callback->gr_fillstylec);

    {
    RGB rgb;
    rgb_set(&rgb, (U8) p_flt_callback->gr_fillstylec.fg.red, (U8) p_flt_callback->gr_fillstylec.fg.green, (U8) p_flt_callback->gr_fillstylec.fg.blue);
    status_return(rgb_patch_set(p_dialog_msg_process_start->h_dialog, &rgb));
    } /*block*/

    status_return(ui_dlg_set_check_forcing(p_dialog_msg_process_start->h_dialog, DIALOG_ID_RGB_T, !p_flt_callback->gr_fillstylec.fg.visible));

    /* ui_dlg_set_check_forcing(p_dialog_msg_process_start->h_dialog, DIALOG_ID_RGB_AUTOMATIC, !p_flt_callback->gr_fillstylec.fg.manual); */

    if(p_flt_callback->has_fillstyleb)
    {
        status_return(ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, STYLE_ID_FILL_SOLID, !p_flt_callback->gr_fillstyleb.bits.notsolid));
        status_return(ui_dlg_set_check_forcing(p_dialog_msg_process_start->h_dialog, STYLE_ID_FILL_PICTURE, p_flt_callback->gr_fillstyleb.bits.pattern));
        status_return(ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, STYLE_ID_FILL_ASPECT, p_flt_callback->gr_fillstyleb.bits.isotropic));
        status_return(ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, STYLE_ID_FILL_RECOLOUR, !p_flt_callback->gr_fillstyleb.bits.norecolour));
    }

    p_flt_callback->rgb_modified = 0;
    p_flt_callback->other_modified = 0;

    ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, DIALOG_ID_RGB_BUTTON, g_has_colour_picker);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_style_fill_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_FLT_CALLBACK p_flt_callback = (P_FLT_CALLBACK) p_dialog_msg_process_end->client_handle;

        if(p_flt_callback->rgb_modified)
        {
            p_flt_callback->gr_fillstylec.fg.manual = 1;

            p_flt_callback->gr_fillstylec.fg.red = p_flt_callback->rgb.r;
            p_flt_callback->gr_fillstylec.fg.green = p_flt_callback->rgb.g;
            p_flt_callback->gr_fillstylec.fg.blue = p_flt_callback->rgb.b;

            p_flt_callback->gr_fillstylec.fg.visible = !ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, DIALOG_ID_RGB_T);

            status_return(gr_chart_objid_fillstylec_set(p_gr_chart_from_chart_handle(p_flt_callback->p_chart_header->ch), p_flt_callback->id, &p_flt_callback->gr_fillstylec));
        }

        if(p_flt_callback->other_modified)
        {
            if(p_flt_callback->has_fillstyleb)
            {
                p_flt_callback->gr_fillstyleb.bits.manual = 1; /* SKS after 1.05 25oct93 - how come nobody found this one? */

                p_flt_callback->gr_fillstyleb.bits.notsolid = !ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, STYLE_ID_FILL_SOLID);
                p_flt_callback->gr_fillstyleb.bits.pattern  =  ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, STYLE_ID_FILL_PICTURE);
                if(!p_flt_callback->gr_fillstyleb.bits.pattern)
                    p_flt_callback->gr_fillstyleb.pattern = GR_FILL_PATTERN_NONE; /* SKS after 1.05 10oct93 */
                p_flt_callback->gr_fillstyleb.bits.isotropic  =  ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, STYLE_ID_FILL_ASPECT);
                p_flt_callback->gr_fillstyleb.bits.norecolour = !ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, STYLE_ID_FILL_RECOLOUR);

                status_return(gr_chart_objid_fillstyleb_set(p_gr_chart_from_chart_handle(p_flt_callback->p_chart_header->ch), p_flt_callback->id, &p_flt_callback->gr_fillstyleb));
            }
        }

        chart_modify_docu(p_flt_callback->p_chart_header);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_style_fill)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        status_return(dialog_style_fill_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

        /*FALLTHRU*/

    case DIALOG_MSG_CODE_CTL_USER_MOUSE:
    case DIALOG_MSG_CODE_CTL_USER_REDRAW:
        return(rgb_class_handler(dialog_message, p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_style_fill_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_style_fill_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_style_fill_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
gr_chart_style_fill_process(
    P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id)
{
    FLT_CALLBACK flt_callback;
    STATUS status;

    zero_struct(flt_callback);
    flt_callback.p_chart_header = p_chart_header;
    flt_callback.id = id;

    {
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_header->ch);
    if(cp->axes[0].chart_type != GR_CHART_TYPE_PIE)
        flt_callback.has_fillstyleb = TRUE;
    } /*block*/

    for(;;)
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        UCHARZ buffer[BUF_MAX_GR_CHART_OBJID_REPR];
        gr_chart_object_name_from_id(p_gr_chart_from_chart_handle(p_chart_header->ch), id, ustr_bptr(buffer), elemof32(buffer));
        if(flt_callback.has_fillstyleb)
            dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, style_fill_ctl_create, elemof32(style_fill_ctl_create), CHART_MSG_DIALOG_STYLE_FILL_HELP_TOPIC);
        else
            dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, style_fill_pie_ctl_create, elemof32(style_fill_pie_ctl_create), CHART_MSG_DIALOG_STYLE_FILL_HELP_TOPIC);
        dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_USTR_TEMP;
        dialog_cmd_process_dbox.caption.text.ustr = ustr_bptr(buffer);
        dialog_cmd_process_dbox.p_proc_client = dialog_event_style_fill;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &flt_callback;
        status = object_call_DIALOG_with_docu(p_docu_from_docno(p_chart_header->docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        if(status != DIALOG_COMPLETION_OK_PERSIST)
            break;
    }

    return(status);
}

/******************************************************************************
*
* line style
*
******************************************************************************/

/*
line style group
*/

static const DIALOG_CONTROL_DATA_GROUPBOX
style_line_colour_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_STYLE_LINE_COLOUR), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
style_line_group =
{
    STYLE_ID_LINE_PATTERN_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_ID_RGB_GROUP, DIALOG_ID_RGB_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
style_line_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_BOX_LINE_STYLE), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
style_line_line[7] =
{
    { STYLE_ID_LINE_LINE_0, STYLE_ID_LINE_PATTERN_GROUP, { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },  { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, LINE_STYLE_H, LINE_STYLE_V }, { DRT(LTLT, RADIOPICTURE), 1 /*tabstop*/ } },
    { STYLE_ID_LINE_LINE_1, STYLE_ID_LINE_PATTERN_GROUP, { STYLE_ID_LINE_LINE_0, STYLE_ID_LINE_LINE_0, STYLE_ID_LINE_LINE_0, DIALOG_CONTROL_SELF },  { 0, 0, 0, LINE_STYLE_V }, { DRT(LBRT, RADIOPICTURE) } },
    { STYLE_ID_LINE_LINE_2, STYLE_ID_LINE_PATTERN_GROUP, { STYLE_ID_LINE_LINE_1, STYLE_ID_LINE_LINE_1, STYLE_ID_LINE_LINE_1, DIALOG_CONTROL_SELF },  { 0, 0, 0, LINE_STYLE_V }, { DRT(LBRT, RADIOPICTURE) } },
    { STYLE_ID_LINE_LINE_3, STYLE_ID_LINE_PATTERN_GROUP, { STYLE_ID_LINE_LINE_0, STYLE_ID_LINE_LINE_0, DIALOG_CONTROL_SELF, STYLE_ID_LINE_LINE_0 },  { 0, 0, LINE_STYLE_H, 0 }, { DRT(RTLB, RADIOPICTURE) } },
    { STYLE_ID_LINE_LINE_4, STYLE_ID_LINE_PATTERN_GROUP, { STYLE_ID_LINE_LINE_3, STYLE_ID_LINE_LINE_3, STYLE_ID_LINE_LINE_3, DIALOG_CONTROL_SELF },  { 0, 0, 0, LINE_STYLE_V }, { DRT(LBRT, RADIOPICTURE) } },
    { STYLE_ID_LINE_LINE_5, STYLE_ID_LINE_PATTERN_GROUP, { STYLE_ID_LINE_LINE_4, STYLE_ID_LINE_LINE_4, STYLE_ID_LINE_LINE_4, DIALOG_CONTROL_SELF },  { 0, 0, 0, LINE_STYLE_V }, { DRT(LBRT, RADIOPICTURE) } },
    { STYLE_ID_LINE_LINE_6, STYLE_ID_LINE_PATTERN_GROUP, { STYLE_ID_LINE_LINE_5, STYLE_ID_LINE_LINE_5, STYLE_ID_LINE_LINE_5, DIALOG_CONTROL_SELF },  { 0, 0, 0, LINE_STYLE_V }, { DRT(LBRT, RADIOPICTURE) } }
};

static const RESOURCE_BITMAP_ID
line_style_bitmap[7] =
{
    { OBJECT_ID_SKEL,  SKEL_ID_BM_LINE_NONE },
    { OBJECT_ID_SKEL,  SKEL_ID_BM_LINE_THIN },
    { OBJECT_ID_SKEL,  SKEL_ID_BM_LINE_STD },
    { OBJECT_ID_CHART, CHART_ID_BM_LDASH },
    { OBJECT_ID_CHART, CHART_ID_BM_LDOT },
    { OBJECT_ID_CHART, CHART_ID_BM_LDASHDOT },
    { OBJECT_ID_CHART, CHART_ID_BM_LDADODO },
};

static const DIALOG_CONTROL_DATA_RADIOPICTURE
chart_line_style_data[7] =
{
    { { 0 }, (S32) GR_LINE_PATTERN_NONE, &line_style_bitmap[0] },
    { { 0 }, (S32) GR_LINE_PATTERN_THIN, &line_style_bitmap[1] },
    { { 0 }, (S32) GR_LINE_PATTERN_SOLID, &line_style_bitmap[2] },
    { { 0 }, (S32) GR_LINE_PATTERN_DASH, &line_style_bitmap[3] },
    { { 0 }, (S32) GR_LINE_PATTERN_DOT, &line_style_bitmap[4] },
    { { 0 }, (S32) GR_LINE_PATTERN_DASH_DOT, &line_style_bitmap[5] },
    { { 0 }, (S32) GR_LINE_PATTERN_DASH_DOT_DOT, &line_style_bitmap[6] }
};

static const DIALOG_CONTROL
style_line_thickness =
{
    STYLE_ID_LINE_THICKNESS, STYLE_ID_LINE_PATTERN_GROUP,
    { STYLE_ID_LINE_LINE_0, STYLE_ID_LINE_LINE_6 },
    { 0, DIALOG_STDSPACING_V, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },
    { DRT(LBLT, BUMP_F64), 1 /*tabstop*/ }
};

static /*poked*/ UI_CONTROL_F64
style_line_thickness_control;

static const DIALOG_CONTROL_DATA_BUMP_F64
style_line_thickness_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &style_line_thickness_control } /* BUMP_XX */ };

static const DIALOG_CONTROL
style_line_thickness_units =
{
    STYLE_ID_LINE_THICKNESS_UNITS, STYLE_ID_LINE_PATTERN_GROUP,
    { STYLE_ID_LINE_THICKNESS, STYLE_ID_LINE_THICKNESS, DIALOG_CONTROL_SELF, STYLE_ID_LINE_THICKNESS },
    { DIALOG_LABELGAP_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

#define style_line_thickness_units_data measurement_points_data

static const DIALOG_CTL_CREATE
style_line_ctl_create[] =
{
    { &dialog_main_group },
    { &style_rgb_group, &style_line_colour_group_data },
    { &rgb_group_inner },
    { &rgb_tx[RGB_TX_IX_R], &rgb_tx_data[RGB_TX_IX_R] },
    { &rgb_bump[RGB_TX_IX_R], &rgb_bump_data },
    { &rgb_tx[RGB_TX_IX_G], &rgb_tx_data[RGB_TX_IX_G] },
    { &rgb_bump[RGB_TX_IX_G], &rgb_bump_data },
    { &rgb_tx[RGB_TX_IX_B], &rgb_tx_data[RGB_TX_IX_B] },
    { &rgb_bump[RGB_TX_IX_B], &rgb_bump_data },
    { &rgb_patch, &rgb_patch_data },
    { &rgb_button, &rgb_button_data },
    { &rgb_patches[0], &rgb_patches_data[0] },
    { &rgb_patches[1], &rgb_patches_data[1] },
    { &rgb_patches[2], &rgb_patches_data[2] },
    { &rgb_patches[3], &rgb_patches_data[3] },
    { &rgb_patches[4], &rgb_patches_data[4] },
    { &rgb_patches[5], &rgb_patches_data[5] },
    { &rgb_patches[6], &rgb_patches_data[6] },
    { &rgb_patches[7], &rgb_patches_data[7] },
    { &rgb_patches[8], &rgb_patches_data[8] },
    { &rgb_patches[9], &rgb_patches_data[9] },
    { &rgb_patches[10], &rgb_patches_data[10] },
    { &rgb_patches[11], &rgb_patches_data[11] },
    { &rgb_patches[12], &rgb_patches_data[12] },
    { &rgb_patches[13], &rgb_patches_data[13] },
    { &rgb_patches[14], &rgb_patches_data[14] },
    { &rgb_patches[15], &rgb_patches_data[15] },
    { &rgb_transparent, &rgb_transparent_data },

    { &style_line_group, &style_line_group_data },
    { &style_line_line[0], &chart_line_style_data[0] },
    { &style_line_line[1], &chart_line_style_data[1] },
    { &style_line_line[2], &chart_line_style_data[2] },
    { &style_line_line[3], &chart_line_style_data[3] },
    { &style_line_line[4], &chart_line_style_data[4] },
    { &style_line_line[5], &chart_line_style_data[5] },
    { &style_line_line[6], &chart_line_style_data[6] },

    { &style_line_thickness, &style_line_thickness_data },
    { &style_line_thickness_units, &style_line_thickness_units_data },

    { &defbutton_ok, &defbutton_ok_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

_Check_return_
static STATUS
dialog_style_line_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_FLT_CALLBACK p_flt_callback = (P_FLT_CALLBACK) p_dialog_msg_process_start->client_handle;

    gr_chart_objid_linestyle_query(p_gr_chart_from_chart_handle(p_flt_callback->p_chart_header->ch), p_flt_callback->id, &p_flt_callback->gr_linestyle);

    {
    RGB rgb;
    rgb_set(&rgb, (U8) p_flt_callback->gr_linestyle.fg.red, (U8) p_flt_callback->gr_linestyle.fg.green, (U8) p_flt_callback->gr_linestyle.fg.blue);
    status_return(rgb_patch_set(p_dialog_msg_process_start->h_dialog, &rgb));
    } /*block*/

    status_return(ui_dlg_set_check_forcing(p_dialog_msg_process_start->h_dialog, DIALOG_ID_RGB_T, !p_flt_callback->gr_linestyle.fg.visible));

    /*status_return(ui_dlg_set_check_forcing(p_dialog_msg_process_start->h_dialog, DIALOG_ID_RGB_AUTOMATIC, !gr_linestyle.fg.manual));*/

    { /* translate in UI to what punter expects to see, in this case points, not pixits or units */
    F64 f64 = ((FP_PIXIT) p_flt_callback->gr_linestyle.width) / PIXITS_PER_POINT;
    status_return(ui_dlg_set_f64(p_dialog_msg_process_start->h_dialog, STYLE_ID_LINE_THICKNESS, &f64));
    } /*block*/

    status_return(ui_dlg_set_radio(p_dialog_msg_process_start->h_dialog, STYLE_ID_LINE_PATTERN_GROUP, (S32) p_flt_callback->gr_linestyle.pattern));

    p_flt_callback->rgb_modified = 0;
    p_flt_callback->other_modified = 0;

    ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, DIALOG_ID_RGB_BUTTON, g_has_colour_picker);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_style_line_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_FLT_CALLBACK p_flt_callback = (P_FLT_CALLBACK) p_dialog_msg_process_end->client_handle;

        p_flt_callback->gr_linestyle.fg.manual = 1;

        p_flt_callback->gr_linestyle.fg.red = p_flt_callback->rgb.r;
        p_flt_callback->gr_linestyle.fg.green = p_flt_callback->rgb.g;
        p_flt_callback->gr_linestyle.fg.blue = p_flt_callback->rgb.b;

        p_flt_callback->gr_linestyle.fg.visible = !ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, DIALOG_ID_RGB_T);

        { /* translate from what punter expects to see, in this case points, not pixits or units */
        F64 multiplier = PIXITS_PER_POINT;
        F64 f64;
        ui_dlg_get_f64(p_dialog_msg_process_end->h_dialog, STYLE_ID_LINE_THICKNESS, &f64);
        p_flt_callback->gr_linestyle.width = ui_dlg_s32_from_f64(&f64, &multiplier, 0, LINE_THICKNESS_MAX_VAL * PIXITS_PER_POINT);
        } /*block*/

        p_flt_callback->gr_linestyle.pattern = (GR_LINE_PATTERN) ui_dlg_get_radio(p_dialog_msg_process_end->h_dialog, STYLE_ID_LINE_PATTERN_GROUP);

        status_return(gr_chart_objid_linestyle_set(p_gr_chart_from_chart_handle(p_flt_callback->p_chart_header->ch), p_flt_callback->id, &p_flt_callback->gr_linestyle));

        chart_modify_docu(p_flt_callback->p_chart_header);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_style_line)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
    case DIALOG_MSG_CODE_CTL_USER_MOUSE:
    case DIALOG_MSG_CODE_CTL_USER_REDRAW:
        return(rgb_class_handler(dialog_message, p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_style_line_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_style_line_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
gr_chart_style_line_process(
    P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id)
{
    FLT_CALLBACK flt_callback;
    STATUS status;

    zero_struct(flt_callback);
    flt_callback.p_chart_header = p_chart_header;
    flt_callback.id = id;

    control_data_setup(STYLE_ID_LINE_THICKNESS, (UI_CONTROL_F64 *) style_line_thickness_data.bump_xx.p_uic);

    for(;;)
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        UCHARZ buffer[BUF_MAX_GR_CHART_OBJID_REPR];
        gr_chart_object_name_from_id(p_gr_chart_from_chart_handle(p_chart_header->ch), id, ustr_bptr(buffer), elemof32(buffer));
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, style_line_ctl_create, elemof32(style_line_ctl_create), CHART_MSG_DIALOG_STYLE_LINE_HELP_TOPIC);
        dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_USTR_TEMP;
        dialog_cmd_process_dbox.caption.text.ustr = ustr_bptr(buffer);
        dialog_cmd_process_dbox.p_proc_client = dialog_event_style_line;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &flt_callback;
        status = object_call_DIALOG_with_docu(p_docu_from_docno(p_chart_header->docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        if(status != DIALOG_COMPLETION_OK_PERSIST)
            break;
    }

    return(status);
}

/******************************************************************************
*
* text style
*
******************************************************************************/

/*
font group
*/

static const DIALOG_CONTROL
style_text_group =
{
    STYLE_ID_TEXT_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
style_text_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_FS), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
style_text_typeface_text =
{
    STYLE_ID_TEXT_TYPEFACE_TEXT, STYLE_ID_TEXT_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, STYLE_ID_TEXT_TYPEFACE_LIST },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, 0, DIALOG_STDCHECK_V },
    { DRT(LTRT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
style_text_typeface_text_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_FS_TYPEFACE), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
style_text_typeface_list =
{
    STYLE_ID_TEXT_TYPEFACE_LIST, STYLE_ID_TEXT_GROUP,
    { STYLE_ID_TEXT_TYPEFACE_TEXT, STYLE_ID_TEXT_TYPEFACE_TEXT, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, TS_FS_TYPEFACE_H, TS_FS_TYPEFACE_V },
    { DRT(LBLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

/*
height 'group'
*/

static const DIALOG_CONTROL
style_text_height_text =
{
    STYLE_ID_TEXT_HEIGHT_TEXT, STYLE_ID_TEXT_GROUP,
    { STYLE_ID_TEXT_TYPEFACE_LIST, STYLE_ID_TEXT_HEIGHT, DIALOG_CONTROL_SELF, STYLE_ID_TEXT_HEIGHT },
    { 0, 0, TS_FS_SIZE_TEXT_H },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
style_text_height_text_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_FS_HEIGHT), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
style_text_height =
{
    STYLE_ID_TEXT_HEIGHT, STYLE_ID_TEXT_GROUP,
    { STYLE_ID_TEXT_HEIGHT_TEXT, STYLE_ID_TEXT_TYPEFACE_LIST },
    { DIALOG_LABELGAP_H, DIALOG_STDSPACING_V, TS_FS_SIZE_FIELDS_H, DIALOG_STDBUMP_V },
    { DRT(RBLT, BUMP_F64), 1 /*tabstop*/ }
};

static /*poked*/ UI_CONTROL_F64
style_text_height_control = { 1.0, 255.0, 1.0 };

static const DIALOG_CONTROL_DATA_BUMP_F64
style_text_height_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &style_text_height_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
style_text_height_units =
{
    STYLE_ID_TEXT_HEIGHT_UNITS, STYLE_ID_TEXT_GROUP,
    { STYLE_ID_TEXT_HEIGHT, STYLE_ID_TEXT_HEIGHT, DIALOG_CONTROL_SELF, STYLE_ID_TEXT_HEIGHT },
    { DIALOG_LABELGAP_H, 0, DIALOG_CONTENTS_CALC },
    { DRT(RTLB, STATICTEXT) }
};

#define style_text_height_units_data measurement_points_data

/*
width 'group'
*/

static const DIALOG_CONTROL
style_text_width_text =
{
    STYLE_ID_TEXT_WIDTH_TEXT, STYLE_ID_TEXT_GROUP,
    { STYLE_ID_TEXT_HEIGHT_TEXT, STYLE_ID_TEXT_WIDTH, STYLE_ID_TEXT_HEIGHT_TEXT, STYLE_ID_TEXT_WIDTH },
    { 0, DIALOG_STDSPACING_V },
    { DRT(LTRB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
style_text_width_text_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_ES_FS_WIDTH), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
style_text_width =
{
    STYLE_ID_TEXT_WIDTH, STYLE_ID_TEXT_GROUP,
    { STYLE_ID_TEXT_HEIGHT, STYLE_ID_TEXT_HEIGHT, STYLE_ID_TEXT_HEIGHT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static /*poked*/ UI_CONTROL_F64
style_text_width_control = { 0.0, 255.0, 1.0 };

static const DIALOG_CONTROL_DATA_BUMP_F64
style_text_width_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &style_text_width_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
style_text_width_units =
{
    STYLE_ID_TEXT_WIDTH_UNITS, STYLE_ID_TEXT_GROUP,
    { STYLE_ID_TEXT_HEIGHT_UNITS, STYLE_ID_TEXT_WIDTH, STYLE_ID_TEXT_HEIGHT_UNITS, STYLE_ID_TEXT_WIDTH },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

#define style_text_width_units_data measurement_points_data

/*
bold 'group'
*/

static const DIALOG_CONTROL
style_text_bold =
{
    STYLE_ID_TEXT_BOLD, STYLE_ID_TEXT_GROUP,
    { STYLE_ID_TEXT_TYPEFACE_LIST, STYLE_ID_TEXT_TYPEFACE_LIST },
    { DIALOG_STDSPACING_H, 0, TS_FS_TYPE_H, TS_FS_TYPE_V },
    { DRT(RTLT, CHECKPICTURE), 1 /*tabstop*/ }
};

/*
italic 'group'
*/

static const DIALOG_CONTROL
style_text_italic =
{
    STYLE_ID_TEXT_ITALIC, STYLE_ID_TEXT_GROUP,
    { STYLE_ID_TEXT_BOLD, STYLE_ID_TEXT_BOLD, STYLE_ID_TEXT_BOLD },
    { 0, DIALOG_STDSPACING_V, 0, TS_FS_TYPE_V },
    { DRT(LBRT, CHECKPICTURE), 1 /*tabstop*/ }
};

/*
fore group
*/

static const DIALOG_CONTROL
style_text_colour_group =
{
    DIALOG_ID_RGB_GROUP, DIALOG_MAIN_GROUP,
    { STYLE_ID_TEXT_GROUP, STYLE_ID_TEXT_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CTL_CREATE
style_text_ctl_create[] =
{
    { &dialog_main_group },
    { &style_text_group, &style_text_group_data },
    { &style_text_typeface_text, &style_text_typeface_text_data },
    { &style_text_typeface_list, &stdlisttext_data },
    { &style_text_height_text, &style_text_height_text_data },
    { &style_text_height, &style_text_height_data },
    { &style_text_height_units, &style_text_height_units_data },
    { &style_text_width_text, &style_text_width_text_data },
    { &style_text_width, &style_text_width_data },
    { &style_text_width_units, &style_text_width_units_data },
    { &style_text_bold, &style_text_bold_data },
    { &style_text_italic, &style_text_italic_data },

    { &style_text_colour_group, &rgb_group_data },
    { &rgb_group_inner },
    { &rgb_tx[RGB_TX_IX_R], &rgb_tx_data[RGB_TX_IX_R] },
    { &rgb_bump[RGB_TX_IX_R], &rgb_bump_data },
    { &rgb_tx[RGB_TX_IX_G], &rgb_tx_data[RGB_TX_IX_G] },
    { &rgb_bump[RGB_TX_IX_G], &rgb_bump_data },
    { &rgb_tx[RGB_TX_IX_B], &rgb_tx_data[RGB_TX_IX_B] },
    { &rgb_bump[RGB_TX_IX_B], &rgb_bump_data },
    { &rgb_patch, &rgb_patch_data },
    { &rgb_button, &rgb_button_data },
    { &rgb_patches[0], &rgb_patches_data[0] },
    { &rgb_patches[1], &rgb_patches_data[1] },
    { &rgb_patches[2], &rgb_patches_data[2] },
    { &rgb_patches[3], &rgb_patches_data[3] },
    { &rgb_patches[4], &rgb_patches_data[4] },
    { &rgb_patches[5], &rgb_patches_data[5] },
    { &rgb_patches[6], &rgb_patches_data[6] },
    { &rgb_patches[7], &rgb_patches_data[7] },
    { &rgb_patches[8], &rgb_patches_data[8] },
    { &rgb_patches[9], &rgb_patches_data[9] },
    { &rgb_patches[10], &rgb_patches_data[10] },
    { &rgb_patches[11], &rgb_patches_data[11] },
    { &rgb_patches[12], &rgb_patches_data[12] },
    { &rgb_patches[13], &rgb_patches_data[13] },
    { &rgb_patches[14], &rgb_patches_data[14] },
    { &rgb_patches[15], &rgb_patches_data[15] },
    { &rgb_transparent, &rgb_transparent_data },

    { &defbutton_ok, &defbutton_ok_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

static UI_SOURCE
typeface_source;

_Check_return_
static STATUS
dialog_style_text_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case STYLE_ID_TEXT_TYPEFACE_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &typeface_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_style_text_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_FLT_CALLBACK p_flt_callback = (P_FLT_CALLBACK) p_dialog_msg_process_start->client_handle;

    gr_chart_objid_textstyle_query(p_gr_chart_from_chart_handle(p_flt_callback->p_chart_header->ch), p_flt_callback->id, &p_flt_callback->gr_textstyle);

    {
    RGB rgb;
    rgb_set(&rgb, (U8) p_flt_callback->gr_textstyle.fg.red, (U8) p_flt_callback->gr_textstyle.fg.green, (U8) p_flt_callback->gr_textstyle.fg.blue);
    status_return(rgb_patch_set(p_dialog_msg_process_start->h_dialog, &rgb));
    } /*block*/

    status_return(ui_dlg_set_check_forcing(p_dialog_msg_process_start->h_dialog, DIALOG_ID_RGB_T, !p_flt_callback->gr_textstyle.fg.visible));

    { /* translate in UI to what punter expects to see on the host system */
    ARRAY_HANDLE_TSTR h_host_font_name_tstr;
    if(status_ok(fontmap_host_base_name_from_app_font_name(&h_host_font_name_tstr, p_flt_callback->gr_textstyle.tstrFontName)))
    {
        /* borrow the handle to some text */
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
        msgclr(dialog_cmd_ctl_state_set);
        dialog_cmd_ctl_state_set.h_dialog = p_dialog_msg_process_start->h_dialog;
        dialog_cmd_ctl_state_set.dialog_control_id = STYLE_ID_TEXT_TYPEFACE_LIST;
        dialog_cmd_ctl_state_set.bits = 0;
        dialog_cmd_ctl_state_set.state.list_text.ui_text.type = UI_TEXT_TYPE_TSTR_ARRAY;
        dialog_cmd_ctl_state_set.state.list_text.ui_text.text.array_handle_tstr = h_host_font_name_tstr;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
        ui_text_dispose(&dialog_cmd_ctl_state_set.state.list_text.ui_text);
    }
    } /*block*/

    status_return(ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, STYLE_ID_TEXT_BOLD, p_flt_callback->gr_textstyle.bold));
    status_return(ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, STYLE_ID_TEXT_ITALIC, p_flt_callback->gr_textstyle.italic));

    { /* translate in UI to what punter expects to see, in this case points, not pixits or units */
    F64 f64;
    f64 = ((FP_PIXIT) p_flt_callback->gr_textstyle.size_y) / PIXITS_PER_POINT;
    status_return(ui_dlg_set_f64(p_dialog_msg_process_start->h_dialog, STYLE_ID_TEXT_HEIGHT, &f64));
    f64 = ((FP_PIXIT) p_flt_callback->gr_textstyle.size_x) / PIXITS_PER_POINT;
    status_return(ui_dlg_set_f64(p_dialog_msg_process_start->h_dialog, STYLE_ID_TEXT_WIDTH, &f64));
    }

    p_flt_callback->rgb_modified = 0;
    p_flt_callback->other_modified = 0;

    ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, DIALOG_ID_RGB_BUTTON, g_has_colour_picker);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_style_text_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_FLT_CALLBACK p_flt_callback = (P_FLT_CALLBACK) p_dialog_msg_process_end->client_handle;

        chart_modify_docu(p_flt_callback->p_chart_header);

        p_flt_callback->gr_textstyle.fg.manual = 1;

        p_flt_callback->gr_textstyle.fg.red = p_flt_callback->rgb.r;
        p_flt_callback->gr_textstyle.fg.green = p_flt_callback->rgb.g;
        p_flt_callback->gr_textstyle.fg.blue = p_flt_callback->rgb.b;

        p_flt_callback->gr_textstyle.fg.visible = !ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, DIALOG_ID_RGB_T);

        { /* translate from what punter expects to see on the host system */
        UI_TEXT ui_text;
        FONT_SPEC font_spec;
        STATUS status;

        ui_dlg_get_list_text(p_dialog_msg_process_end->h_dialog, STYLE_ID_TEXT_TYPEFACE_LIST, &ui_text);

        if(status_ok(status = create_error(fontmap_font_spec_from_host_base_name(&font_spec, ui_text_tstr(&ui_text)))))
        {
            tstr_xstrkpy(p_flt_callback->gr_textstyle.tstrFontName, elemof32(p_flt_callback->gr_textstyle.tstrFontName), array_tstr(&font_spec.h_app_name_tstr));
            font_spec_dispose(&font_spec);
        }

        ui_text_dispose(&ui_text);

        if(status_fail(status) && (status != STATUS_FAIL))
            return(status);
        } /*block*/

        p_flt_callback->gr_textstyle.bold   = ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, STYLE_ID_TEXT_BOLD);
        p_flt_callback->gr_textstyle.italic = ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, STYLE_ID_TEXT_ITALIC);

        { /* translate from what punter expects to see, in this case points, not pixits or units */
        F64 multiplier = PIXITS_PER_POINT;
        F64 f64;
        ui_dlg_get_f64(p_dialog_msg_process_end->h_dialog, STYLE_ID_TEXT_HEIGHT, &f64);
        p_flt_callback->gr_textstyle.size_y = (PIXIT) ui_dlg_s32_from_f64(&f64, &multiplier, 1 * PIXITS_PER_POINT, (S32) HEIGHT_WIDTH_MAX_VAL * PIXITS_PER_POINT);
        ui_dlg_get_f64(p_dialog_msg_process_end->h_dialog, STYLE_ID_TEXT_WIDTH, &f64);
        p_flt_callback->gr_textstyle.size_x = (PIXIT) ui_dlg_s32_from_f64(&f64, &multiplier, 0 * PIXITS_PER_POINT, (S32) HEIGHT_WIDTH_MAX_VAL * PIXITS_PER_POINT);
        } /*block*/

        status_return(gr_chart_objid_textstyle_set(p_gr_chart_from_chart_handle(p_flt_callback->p_chart_header->ch), p_flt_callback->id, &p_flt_callback->gr_textstyle));
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_style_text)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_style_text_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
    case DIALOG_MSG_CODE_CTL_USER_MOUSE:
    case DIALOG_MSG_CODE_CTL_USER_REDRAW:
        return(rgb_class_handler(dialog_message, p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_style_text_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_style_text_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
gr_chart_style_text_process(
    P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id)
{
    FLT_CALLBACK flt_callback;
    STATUS status;
    ARRAY_HANDLE typeface_handle;

    status_return(ui_list_create_typeface(&typeface_handle, &typeface_source));

    zero_struct(flt_callback);
    flt_callback.p_chart_header = p_chart_header;
    flt_callback.id = id;

    control_data_setup(STYLE_ID_TEXT_HEIGHT, (UI_CONTROL_F64 *) style_text_height_data.bump_xx.p_uic);
    control_data_setup(STYLE_ID_TEXT_WIDTH,  (UI_CONTROL_F64 *) style_text_width_data.bump_xx.p_uic);

    for(;;)
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        UCHARZ buffer[BUF_MAX_GR_CHART_OBJID_REPR];
        gr_chart_object_name_from_id(p_gr_chart_from_chart_handle(p_chart_header->ch), id, ustr_bptr(buffer), elemof32(buffer));
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, style_text_ctl_create, elemof32(style_text_ctl_create), CHART_MSG_DIALOG_CHART_LEGEND_HELP_TOPIC);
        dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_USTR_TEMP;
        dialog_cmd_process_dbox.caption.text.ustr = ustr_bptr(buffer);
        dialog_cmd_process_dbox.p_proc_client = dialog_event_style_text;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &flt_callback;
        status = object_call_DIALOG_with_docu(p_docu_from_docno(p_chart_header->docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        if(status != DIALOG_COMPLETION_OK_PERSIST)
            break;
    }

    ui_lists_dispose_tb(&typeface_handle, &typeface_source, offsetof32(UI_LIST_ENTRY_TYPEFACE, quick_tblock));

    return(status);
}

/******************************************************************************
*
* chart margins
*
******************************************************************************/

typedef struct CHART_USER_UNIT_INFO
{
    F64         user_unit_multiple;
    UCHARZ      user_unit_numform_ustr_buf[32];
    STATUS      user_unit_resource_id;
    FP_PIXIT    fp_pixits_per_user_unit;
}
CHART_USER_UNIT_INFO, * P_CHART_USER_UNIT_INFO;

typedef struct CHART_MARGIN_STATICS
{
#define IDX_HORZ 0
#define IDX_VERT 1
    CHART_USER_UNIT_INFO info[2];
}
CHART_MARGIN_STATICS;

static CHART_MARGIN_STATICS
chart_margins_;

enum CHART_CONTROL_IDS
{
#define CHART_TEXTS1_H DIALOG_SYSCHARS_H("Height")
#define CHART_FIELDS_H DIALOG_BUMP_H(7)

    CHART_ID_MARGIN_LEFT = 130,
    CHART_ID_MARGIN_LEFT_TEXT,
    CHART_ID_MARGIN_LEFT_UNITS,

    CHART_ID_MARGIN_RIGHT,
    CHART_ID_MARGIN_RIGHT_TEXT,
    CHART_ID_MARGIN_RIGHT_UNITS,

    CHART_ID_MARGIN_TOP,
    CHART_ID_MARGIN_TOP_TEXT,
    CHART_ID_MARGIN_TOP_UNITS,

    CHART_ID_MARGIN_BOTTOM,
    CHART_ID_MARGIN_BOTTOM_TEXT,
    CHART_ID_MARGIN_BOTTOM_UNITS,

    CHART_ID_MAX
};

/*
paper margin group
*/

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
chart_margin_units_data[2] =
{
    { { UI_TEXT_TYPE_NONE }, { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } },
    { { UI_TEXT_TYPE_NONE }, { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } }
};

static /*poked*/ UI_CONTROL_F64
chart_margin_control[2];

/*
top margin 'group'
*/

static const DIALOG_CONTROL
chart_margin_top_text =
{
    CHART_ID_MARGIN_TOP_TEXT, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, CHART_ID_MARGIN_TOP, DIALOG_CONTROL_SELF, CHART_ID_MARGIN_TOP },
    { 0, 0, CHART_TEXTS1_H, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
chart_margin_top_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_CHART_MARGIN_TOP), { 1/*left_text*/ } };

static const DIALOG_CONTROL
chart_margin_top =
{
    CHART_ID_MARGIN_TOP, DIALOG_MAIN_GROUP,
    { CHART_ID_MARGIN_TOP_TEXT, DIALOG_CONTROL_PARENT },
    { DIALOG_LABELGAP_H, 0, CHART_FIELDS_H, DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
chart_margin_top_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &chart_margin_control[IDX_VERT] } /*BUMP_XX*/ };

static const DIALOG_CONTROL
chart_margin_top_units =
{
    CHART_ID_MARGIN_TOP_UNITS, DIALOG_MAIN_GROUP,
    { CHART_ID_MARGIN_TOP, CHART_ID_MARGIN_TOP, DIALOG_CONTROL_SELF, CHART_ID_MARGIN_TOP },
    { DIALOG_LABELGAP_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

/*
bottom margin 'group'
*/

static const DIALOG_CONTROL
chart_margin_bottom_text =
{
    CHART_ID_MARGIN_BOTTOM_TEXT, DIALOG_MAIN_GROUP,
    { CHART_ID_MARGIN_TOP_TEXT, CHART_ID_MARGIN_BOTTOM, CHART_ID_MARGIN_TOP_TEXT, CHART_ID_MARGIN_BOTTOM },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
chart_margin_bottom_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_CHART_MARGIN_BOTTOM), { 1/*left_text*/ } };

static const DIALOG_CONTROL
chart_margin_bottom =
{
    CHART_ID_MARGIN_BOTTOM, DIALOG_MAIN_GROUP,
    { CHART_ID_MARGIN_TOP, CHART_ID_MARGIN_TOP, CHART_ID_MARGIN_TOP },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
chart_margin_bottom_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &chart_margin_control[IDX_VERT] } /*BUMP_XX*/ };

static const DIALOG_CONTROL
chart_margin_bottom_units =
{
    CHART_ID_MARGIN_BOTTOM_UNITS, DIALOG_MAIN_GROUP,
    { CHART_ID_MARGIN_TOP_UNITS, CHART_ID_MARGIN_BOTTOM, CHART_ID_MARGIN_TOP_UNITS, CHART_ID_MARGIN_BOTTOM },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

/*
left margin 'group'
*/

static const DIALOG_CONTROL
chart_margin_left_text =
{
    CHART_ID_MARGIN_LEFT_TEXT, DIALOG_MAIN_GROUP,
    { CHART_ID_MARGIN_BOTTOM_TEXT, CHART_ID_MARGIN_LEFT, CHART_ID_MARGIN_BOTTOM_TEXT, CHART_ID_MARGIN_LEFT },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
chart_margin_left_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_CHART_MARGIN_LEFT), { 1/*left_text*/ } };

static const DIALOG_CONTROL
chart_margin_left =
{
    CHART_ID_MARGIN_LEFT, DIALOG_MAIN_GROUP,
    { CHART_ID_MARGIN_BOTTOM, CHART_ID_MARGIN_BOTTOM, CHART_ID_MARGIN_BOTTOM },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
chart_margin_left_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &chart_margin_control[IDX_HORZ] } /*BUMP_XX*/ };

static const DIALOG_CONTROL
chart_margin_left_units =
{
    CHART_ID_MARGIN_LEFT_UNITS, DIALOG_MAIN_GROUP,
    { CHART_ID_MARGIN_BOTTOM_UNITS, CHART_ID_MARGIN_LEFT, CHART_ID_MARGIN_BOTTOM_UNITS, CHART_ID_MARGIN_LEFT },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

/*
right margin 'group'
*/

static const DIALOG_CONTROL
chart_margin_right_text =
{
    CHART_ID_MARGIN_RIGHT_TEXT, DIALOG_MAIN_GROUP,
    { CHART_ID_MARGIN_LEFT_TEXT, CHART_ID_MARGIN_RIGHT, CHART_ID_MARGIN_LEFT_TEXT, CHART_ID_MARGIN_RIGHT },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
chart_margin_right_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_CHART_MARGIN_RIGHT), { 1/*left_text*/ } };

static const DIALOG_CONTROL
chart_margin_right =
{
    CHART_ID_MARGIN_RIGHT, DIALOG_MAIN_GROUP,
    { CHART_ID_MARGIN_LEFT, CHART_ID_MARGIN_LEFT, CHART_ID_MARGIN_LEFT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_F64
chart_margin_right_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &chart_margin_control[IDX_HORZ] } /*BUMP_XX*/ };

static const DIALOG_CONTROL
chart_margin_right_units =
{
    CHART_ID_MARGIN_RIGHT_UNITS, DIALOG_MAIN_GROUP,
    { CHART_ID_MARGIN_LEFT_UNITS, CHART_ID_MARGIN_RIGHT, CHART_ID_MARGIN_LEFT_UNITS, CHART_ID_MARGIN_RIGHT },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

static const DIALOG_CTL_CREATE
chart_margins_ctl_create[] =
{
    { &dialog_main_group },

    { &chart_margin_top_text,      &chart_margin_top_text_data },
    { &chart_margin_top,           &chart_margin_top_data },
    { &chart_margin_top_units,     &chart_margin_units_data[IDX_VERT] },
    { &chart_margin_bottom_text,   &chart_margin_bottom_text_data },
    { &chart_margin_bottom,        &chart_margin_bottom_data },
    { &chart_margin_bottom_units,  &chart_margin_units_data[IDX_VERT] },
    { &chart_margin_left_text,     &chart_margin_left_text_data },
    { &chart_margin_left,          &chart_margin_left_data },
    { &chart_margin_left_units,    &chart_margin_units_data[IDX_HORZ] },
    { &chart_margin_right_text,    &chart_margin_right_text_data },
    { &chart_margin_right,         &chart_margin_right_data },
    { &chart_margin_right_units,   &chart_margin_units_data[IDX_HORZ] },

    { &defbutton_ok, &defbutton_ok_persist_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

static void
user_unit_setup(
    _InVal_     DOCNO docno,
    P_CHART_USER_UNIT_INFO p_user_unit_info,
    _InVal_     U32 idx)
{
    SCALE_INFO scale_info;
    DISPLAY_UNIT_INFO display_unit_info;
    STATUS numform_resource_id;

    scale_info_from_docu(p_docu_from_docno(docno), (idx != IDX_VERT), &scale_info);

    /* allow bumping to finest usable resolution */
    p_user_unit_info->user_unit_multiple = ((F64) scale_info.coarse_div * scale_info.fine_div) / scale_info.numbered_units_multiplier;

    display_unit_info_from_display_unit(&display_unit_info, scale_info.display_unit);

    p_user_unit_info->fp_pixits_per_user_unit = display_unit_info.fp_pixits_per_unit;

    switch(scale_info.display_unit)
    {
    default: default_unhandled();
#if CHECKING
    case DISPLAY_UNIT_CM:
#endif
        p_user_unit_info->user_unit_resource_id = MSG_DIALOG_UNITS_CM;
        numform_resource_id = MSG_DIALOG_STYLE_NUMFORM_CM;
        break;

    case DISPLAY_UNIT_MM:
        p_user_unit_info->user_unit_resource_id = MSG_DIALOG_UNITS_MM;
        numform_resource_id = MSG_DIALOG_STYLE_NUMFORM_MM;
        break;

    case DISPLAY_UNIT_INCHES:
        p_user_unit_info->user_unit_resource_id = MSG_DIALOG_UNITS_INCHES;
        numform_resource_id = MSG_DIALOG_STYLE_NUMFORM_INCHES;
        break;

    case DISPLAY_UNIT_POINTS:
        p_user_unit_info->user_unit_resource_id = MSG_DIALOG_UNITS_POINTS;
        numform_resource_id = MSG_DIALOG_STYLE_NUMFORM_POINTS;
        break;
    }

    resource_lookup_ustr_buffer(ustr_bptr(p_user_unit_info->user_unit_numform_ustr_buf), sizeof32(p_user_unit_info->user_unit_numform_ustr_buf), numform_resource_id);
}

/* reflect current measurement systems in dialog */

static void
chart_margins_precreate(
    _InVal_     DOCNO docno)
{
    U32 i;

    for(i = 0; i < elemof32(chart_margins_.info); ++i) /* don't care here what they are, just how many */
    {
        P_CHART_USER_UNIT_INFO p_chart_user_unit_info = &chart_margins_.info[i];
        UI_CONTROL_F64 * p_ui_control_f64 = &chart_margin_control[i];
        P_UI_TEXT p_ui_text = &chart_margin_units_data[i].caption;

        user_unit_setup(docno, p_chart_user_unit_info, i);

        p_ui_control_f64->ustr_numform  = ustr_bptr(p_chart_user_unit_info->user_unit_numform_ustr_buf);
        p_ui_control_f64->inc_dec_round = p_chart_user_unit_info->user_unit_multiple;
        p_ui_control_f64->bump_val      = 1.0 / p_ui_control_f64->inc_dec_round;
        p_ui_control_f64->min_val       = 0.0;
        p_ui_control_f64->max_val       = (10.0 * PIXITS_PER_INCH) / p_chart_user_unit_info->fp_pixits_per_user_unit;

        p_ui_text->type = UI_TEXT_TYPE_RESID;
        p_ui_text->text.resource_id = p_chart_user_unit_info->user_unit_resource_id;
    }
}

typedef struct CHART_MARGINS_CALLBACK
{
    P_CHART_HEADER p_chart_header;
}
CHART_MARGINS_CALLBACK, * P_CHART_MARGINS_CALLBACK;

_Check_return_
static STATUS
dialog_chart_margins_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_CHART_MARGINS_CALLBACK p_chart_margins_callback = (P_CHART_MARGINS_CALLBACK) p_dialog_msg_process_start->client_handle;
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_margins_callback->p_chart_header->ch); /* paranoia */
    STATUS status = STATUS_OK;

    if(status_ok(status))
    {
        F64 f64 = cp->core.layout.margins.top   / chart_margins_.info[IDX_VERT].fp_pixits_per_user_unit;
        status = ui_dlg_set_f64(p_dialog_msg_process_start->h_dialog, CHART_ID_MARGIN_TOP, &f64);
    }

    if(status_ok(status))
    {
        F64 f64 = cp->core.layout.margins.bottom / chart_margins_.info[IDX_VERT].fp_pixits_per_user_unit;
        status = ui_dlg_set_f64(p_dialog_msg_process_start->h_dialog, CHART_ID_MARGIN_BOTTOM, &f64);
    }

    if(status_ok(status))
    {
        F64 f64 = cp->core.layout.margins.left   / chart_margins_.info[IDX_HORZ].fp_pixits_per_user_unit;
        status = ui_dlg_set_f64(p_dialog_msg_process_start->h_dialog, CHART_ID_MARGIN_LEFT, &f64);
    }

    if(status_ok(status))
    {
        F64 f64 = cp->core.layout.margins.right  / chart_margins_.info[IDX_HORZ].fp_pixits_per_user_unit;
        status = ui_dlg_set_f64(p_dialog_msg_process_start->h_dialog, CHART_ID_MARGIN_RIGHT, &f64);
    }

    return(status);
}

_Check_return_
static STATUS
dialog_chart_margins_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_CHART_MARGINS_CALLBACK p_chart_margins_callback = (P_CHART_MARGINS_CALLBACK) p_dialog_msg_process_end->client_handle;
        const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_margins_callback->p_chart_header->ch); /* paranoia */
        F64 f64;

        ui_dlg_get_f64(p_dialog_msg_process_end->h_dialog, CHART_ID_MARGIN_TOP, &f64);
        cp->core.layout.margins.top    = ui_dlg_s32_from_f64(&f64, &chart_margins_.info[IDX_VERT].fp_pixits_per_user_unit, 0, S32_MAX);
        ui_dlg_get_f64(p_dialog_msg_process_end->h_dialog, CHART_ID_MARGIN_BOTTOM, &f64);
        cp->core.layout.margins.bottom = ui_dlg_s32_from_f64(&f64, &chart_margins_.info[IDX_VERT].fp_pixits_per_user_unit, 0, S32_MAX);
        ui_dlg_get_f64(p_dialog_msg_process_end->h_dialog, CHART_ID_MARGIN_LEFT, &f64);
        cp->core.layout.margins.left   = ui_dlg_s32_from_f64(&f64, &chart_margins_.info[IDX_HORZ].fp_pixits_per_user_unit, 0, S32_MAX);
        ui_dlg_get_f64(p_dialog_msg_process_end->h_dialog, CHART_ID_MARGIN_RIGHT, &f64);
        cp->core.layout.margins.right  = ui_dlg_s32_from_f64(&f64, &chart_margins_.info[IDX_HORZ].fp_pixits_per_user_unit, 0, S32_MAX);

        chart_modify_docu(p_chart_margins_callback->p_chart_header);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_chart_margins)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_chart_margins_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_chart_margins_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
gr_chart_margins_process(
    P_CHART_HEADER p_chart_header)
{
    CHART_MARGINS_CALLBACK chart_margins_callback;
    STATUS status;

    chart_margins_callback.p_chart_header = p_chart_header;

    for(;;)
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, chart_margins_ctl_create, elemof32(chart_margins_ctl_create), CHART_MSG_DIALOG_CHART_MARGINS_HELP_TOPIC);
        /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
        dialog_cmd_process_dbox.caption.text.resource_id = CHART_MSG_DIALOG_CHART_MARGINS_CAPTION;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_chart_margins;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &chart_margins_callback;
        chart_margins_precreate(p_chart_header->docno);
        status = object_call_DIALOG_with_docu(p_docu_from_docno(p_chart_header->docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        if(status != DIALOG_COMPLETION_OK_PERSIST)
            break;
    }

    return(status);
}

enum CHART_LEGEND_IDS
{
    CHART_ID_LEGEND_ON = 190,
    CHART_ID_LEGEND_HORZ,

    CHART_ID_LEGEND_MAX
};

static const DIALOG_CONTROL
chart_legend_on =
{
    CHART_ID_LEGEND_ON, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
#if WINDOWS
    { 0, 0, DIALOG_STDWIDTH_MIN, DIALOG_STDCHECK_V },
#else
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
#endif
    { DRT(LTLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
chart_legend_on_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_CHART_LEGEND_ON) };

static const DIALOG_CONTROL
chart_legend_horz =
{
    CHART_ID_LEGEND_HORZ, DIALOG_MAIN_GROUP,
    { CHART_ID_LEGEND_ON, CHART_ID_LEGEND_ON },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
chart_legend_horz_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_CHART_LEGEND_HORZ) };

static const DIALOG_CTL_CREATE
chart_legend_ctl_create[] =
{
    { &dialog_main_group },

    { &chart_legend_on, &chart_legend_on_data },
    { &chart_legend_horz, &chart_legend_horz_data },

    { &defbutton_ok, &defbutton_ok_persist_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

typedef struct CHART_LEGEND_CALLBACK
{
    P_CHART_HEADER p_chart_header;
}
CHART_LEGEND_CALLBACK, * P_CHART_LEGEND_CALLBACK;

_Check_return_
static STATUS
dialog_chart_legend_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_CHART_LEGEND_CALLBACK p_chart_legend_callback = (P_CHART_LEGEND_CALLBACK) p_dialog_msg_process_start->client_handle;
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_legend_callback->p_chart_header->ch);

    status_return(ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, CHART_ID_LEGEND_ON, cp->legend.bits.on));
    return(ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, CHART_ID_LEGEND_HORZ, cp->legend.bits.in_rows));
}

_Check_return_
static STATUS
dialog_chart_legend_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_CHART_LEGEND_CALLBACK p_chart_legend_callback = (P_CHART_LEGEND_CALLBACK) p_dialog_msg_process_end->client_handle;
        const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_legend_callback->p_chart_header->ch);

        cp->legend.bits.on = ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, CHART_ID_LEGEND_ON);
        cp->legend.bits.in_rows = ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, CHART_ID_LEGEND_HORZ);

        chart_modify_docu(p_chart_legend_callback->p_chart_header);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_chart_legend)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_chart_legend_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_chart_legend_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
gr_chart_legend_process(
    P_CHART_HEADER p_chart_header)
{
    CHART_LEGEND_CALLBACK chart_legend_callback;
    STATUS status;

    chart_legend_callback.p_chart_header = p_chart_header;

    for(;;)
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, chart_legend_ctl_create, elemof32(chart_legend_ctl_create), CHART_MSG_DIALOG_CHART_LEGEND_HELP_TOPIC);
        /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
        dialog_cmd_process_dbox.caption.text.resource_id = CHART_MSG_DIALOG_CHART_LEGEND_CAPTION;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_chart_legend;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &chart_legend_callback;
        status = object_call_DIALOG_with_docu(p_docu_from_docno(p_chart_header->docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        if(status != DIALOG_COMPLETION_OK_PERSIST)
            break;
    }

    return(status);
}

/* end of gr_uisty.c */
