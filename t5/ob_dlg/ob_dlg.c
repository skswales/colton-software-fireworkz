/* ob_dlg.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* dialog UI handling */

/* SKS April 1992 */

#include "common/gflags.h"

#include "ob_dlg/ui_dlgin.h"

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_DIALOG)
extern PC_U8 rb_dlg_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_DIALOG &rb_dlg_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_DIALOG LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_DIALOG DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_DIALOG DONT_LOAD_RESOURCES

/*
internal routines
*/

static void
dialog_ictl_enable_here(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl);

static void
dialog_ictl_enable_in_tree(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl);

static void
dialog_ictl_enable_suppressed_changes_in(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_ictl_group);

_Check_return_
static STATUS
dialog_ictl_encode_here(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    _InVal_     S32 bits);

/* ------------------------------------------------------------------------------------ */

/*
instance of dialog statics shared by ob_dlg modules
*/

DIALOG_STATICS dialog_statics;

_Check_return_
static STATUS
dialog_cmd_ctl_enable(
    P_DIALOG_CMD_CTL_ENABLE p_dialog_cmd_ctl_enable)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;
    S32 enabled;

    assert(p_dialog_cmd_ctl_enable->dialog_control_id != DIALOG_CONTROL_INVALID);
    assert(p_dialog_cmd_ctl_enable->enabled           != (S32) 0xBCBCBCBC);

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_enable->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if(NULL == (p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_cmd_ctl_enable->dialog_control_id)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

    enabled = (p_dialog_cmd_ctl_enable->enabled != 0);

    if(p_dialog_ictl->bits.enabled != UBF_PACK(enabled))
    {
        p_dialog_ictl->bits.enabled = UBF_PACK(enabled);

        /* reflect enabled state (taking suppression into account) and propagate down tree if a group */
        dialog_ictl_enable_in_tree(p_dialog, p_dialog_ictl);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_cmd_ctl_enable_query(
    P_DIALOG_CMD_CTL_ENABLE_QUERY p_dialog_cmd_ctl_enable_query)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;

    assert(p_dialog_cmd_ctl_enable_query->dialog_control_id != DIALOG_CONTROL_INVALID);

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_enable_query->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if(NULL == (p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_cmd_ctl_enable_query->dialog_control_id)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

    p_dialog_cmd_ctl_enable_query->enabled = p_dialog_ictl->bits.enabled;
    p_dialog_cmd_ctl_enable_query->enable_suppressed = p_dialog_ictl->bits.enable_suppressed;

    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_CTL_ENCODE
*
* ensure encode state is reflected
*
******************************************************************************/

_Check_return_
static STATUS
dialog_cmd_ctl_encode(
    P_DIALOG_CMD_CTL_ENCODE p_dialog_cmd_ctl_encode)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;

    assert(p_dialog_cmd_ctl_encode->dialog_control_id != DIALOG_CONTROL_INVALID);

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_encode->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if(HOST_WND_NONE == p_dialog->hwnd)
        return(STATUS_OK);

    if(NULL == (p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_cmd_ctl_encode->dialog_control_id)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

    switch(p_dialog_ictl->dialog_control_type)
    {
    default:
        status_return(dialog_ictl_encode_here(p_dialog, p_dialog_ictl, p_dialog_cmd_ctl_encode->bits));
        break;

    case DIALOG_CONTROL_GROUPBOX:
        status_return(dialog_ictls_encode_in(p_dialog, &p_dialog_ictl->data.groupbox.ictls, p_dialog_cmd_ctl_encode->bits));
        break;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_CTL_FOCUS_SET
*
* ensure the input focus is set to this control
*
******************************************************************************/

_Check_return_
static STATUS
dialog_cmd_ctl_focus_set(
    P_DIALOG_CMD_CTL_FOCUS_SET p_dialog_cmd_ctl_focus_set)
{
    P_DIALOG p_dialog;

    assert(p_dialog_cmd_ctl_focus_set->dialog_control_id != DIALOG_CONTROL_INVALID);

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_focus_set->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if(p_dialog_cmd_ctl_focus_set->dialog_control_id)
    {
        P_DIALOG_ICTL p_dialog_ictl;

        if(NULL == (p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_cmd_ctl_focus_set->dialog_control_id)))
            return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

        /* is this control enabled? */
        if(!p_dialog_ictl->bits.enabled)
            return(STATUS_FAIL);
    }

    dialog_current_set(p_dialog, p_dialog_cmd_ctl_focus_set->dialog_control_id, 0);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_cmd_ctl_new_source(
    P_DIALOG_CMD_CTL_NEW_SOURCE p_dialog_cmd_ctl_new_source)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;

    assert(p_dialog_cmd_ctl_new_source->dialog_control_id != DIALOG_CONTROL_INVALID);

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_new_source->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if(NULL == (p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_cmd_ctl_new_source->dialog_control_id)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

    switch(p_dialog_ictl->dialog_control_type)
    {
    default:
        return(STATUS_OK);

    case DIALOG_CONTROL_LIST_TEXT:
    case DIALOG_CONTROL_LIST_S32:
#if RISCOS
        if(p_dialog_ictl->data.list_xx.list_xx.riscos.lbox)
            ri_lbox_source_modified(p_dialog_ictl->data.list_xx.list_xx.riscos.lbox);
#else
        dialog_windows_build_list(p_dialog, p_dialog_ictl);
#endif
        break;

    case DIALOG_CONTROL_COMBO_TEXT:
    case DIALOG_CONTROL_COMBO_S32:
#if RISCOS
        if(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox)
            ri_lbox_source_modified(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox);
#else
        dialog_windows_build_list(p_dialog, p_dialog_ictl);
#endif
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_cmd_ctl_nobble(
    P_DIALOG_CMD_CTL_NOBBLE p_dialog_cmd_ctl_nobble)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;
    S32 nobbled;

    assert(p_dialog_cmd_ctl_nobble->dialog_control_id != DIALOG_CONTROL_INVALID);
    assert(p_dialog_cmd_ctl_nobble->nobbled           != 0xBCBCBCBC);

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_nobble->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if(NULL == (p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_cmd_ctl_nobble->dialog_control_id)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

    nobbled = (p_dialog_cmd_ctl_nobble->nobbled != 0);

    if(p_dialog_ictl->bits.nobbled != UBF_PACK(nobbled))
    {
        p_dialog_ictl->bits.nobbled = UBF_PACK(nobbled);

        /* no state to reflect, this only takes effect later */
    }

    return(STATUS_OK);
}

static inline void
dialog_cmd_ctl_ui_control_BUMP_S32(
    P_DIALOG_CMD_CTL_UI_CONTROL p_dialog_cmd_ctl_ui_control,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    PC_UI_CONTROL_S32 p_ui_control_s32 = (PC_UI_CONTROL_S32) p_dialog_ictl->p_dialog_control_data.bump_s32->bump_xx.p_uic;
    UI_CONTROL_S32 * p_ui_control_s32_wr;

    if(PTR_IS_NONE(p_ui_control_s32))
        return;

    p_ui_control_s32_wr = de_const_cast(UI_CONTROL_S32 *, p_ui_control_s32);

    switch(p_dialog_cmd_ctl_ui_control->what)
    {
    case DIALOG_CMD_CTL_UI_CONTROL_UIC:
        de_const_cast(DIALOG_CONTROL_DATA_BUMP_S32 *, p_dialog_ictl->p_dialog_control_data.bump_s32)->bump_xx.p_uic =
            p_dialog_cmd_ctl_ui_control->data.p_ui_control_s32;
        break;

    case DIALOG_CMD_CTL_UI_CONTROL_MAX:
        p_ui_control_s32_wr->max_val = p_dialog_cmd_ctl_ui_control->data.s32;
        break;

    case DIALOG_CMD_CTL_UI_CONTROL_MIN:
        p_ui_control_s32_wr->min_val = p_dialog_cmd_ctl_ui_control->data.s32;
        break;

    case DIALOG_CMD_CTL_UI_CONTROL_BUMP:
        p_ui_control_s32_wr->bump_val = p_dialog_cmd_ctl_ui_control->data.s32;
        break;

    default:
        break;
    }
}

static inline void
dialog_cmd_ctl_ui_control_BUMP_F64(
    P_DIALOG_CMD_CTL_UI_CONTROL p_dialog_cmd_ctl_ui_control,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    const PC_UI_CONTROL_F64 p_ui_control_f64 = (PC_UI_CONTROL_F64) p_dialog_ictl->p_dialog_control_data.bump_f64->bump_xx.p_uic;
    UI_CONTROL_F64 * p_ui_control_f64_wr;

    if(PTR_IS_NONE(p_ui_control_f64))
        return;

    p_ui_control_f64_wr = de_const_cast(UI_CONTROL_F64 *, p_ui_control_f64);

    switch(p_dialog_cmd_ctl_ui_control->what)
    {
    case DIALOG_CMD_CTL_UI_CONTROL_UIC:
        de_const_cast(DIALOG_CONTROL_DATA_BUMP_F64 *, p_dialog_ictl->p_dialog_control_data.bump_f64)->bump_xx.p_uic =
            p_dialog_cmd_ctl_ui_control->data.p_ui_control_f64;
        break;

    case DIALOG_CMD_CTL_UI_CONTROL_MAX:
        p_ui_control_f64_wr->max_val = p_dialog_cmd_ctl_ui_control->data.f64;
        break;

    case DIALOG_CMD_CTL_UI_CONTROL_MIN:
        p_ui_control_f64_wr->min_val = p_dialog_cmd_ctl_ui_control->data.f64;
        break;

    case DIALOG_CMD_CTL_UI_CONTROL_BUMP:
        p_ui_control_f64_wr->bump_val = p_dialog_cmd_ctl_ui_control->data.f64;
        break;

    default:
        break;
    }
}

_Check_return_
static STATUS
dialog_cmd_ctl_ui_control(
    P_DIALOG_CMD_CTL_UI_CONTROL p_dialog_cmd_ctl_ui_control)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;

    assert(p_dialog_cmd_ctl_ui_control->dialog_control_id != DIALOG_CONTROL_INVALID);
    assert(p_dialog_cmd_ctl_ui_control->what              != 0xBCBCBCBC);

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_ui_control->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if(NULL == (p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_cmd_ctl_ui_control->dialog_control_id)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

    switch(p_dialog_ictl->dialog_control_type)
    {
    default:
        return(create_error(DIALOG_ERR_CANT_UI_CONTROL));

    case DIALOG_CONTROL_BUMP_S32:
        dialog_cmd_ctl_ui_control_BUMP_S32(p_dialog_cmd_ctl_ui_control, p_dialog_ictl);
        break;

    case DIALOG_CONTROL_BUMP_F64:
        dialog_cmd_ctl_ui_control_BUMP_F64(p_dialog_cmd_ctl_ui_control, p_dialog_ictl);
        break;
    }

    /* no state to reflect, this only takes effect later */

    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_CTL_PARENT_QUERY
*
******************************************************************************/

_Check_return_
static STATUS
dialog_cmd_ctl_parent_query(
    P_DIALOG_CMD_CTL_PARENT_QUERY p_dialog_cmd_ctl_parent_query)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;
    P_DIALOG_ICTL_GROUP p_parent_ictls;

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_parent_query->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if(NULL == (p_dialog_ictl = p_dialog_ictl_from_control_id_in(&p_dialog->ictls, p_dialog_cmd_ctl_parent_query->dialog_control_id, &p_parent_ictls)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

    UNREFERENCED_PARAMETER(p_dialog_ictl);

    PTR_ASSERT(p_parent_ictls);

    p_dialog_cmd_ctl_parent_query->parent_dialog_control_id = dialog_control_id_of_group(p_dialog, p_parent_ictls);

#if RISCOS
    p_dialog_cmd_ctl_parent_query->hwnd = p_dialog->hwnd;
#endif

    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_CTL_POSN_QUERY
*
* inform caller of position of a control
*
******************************************************************************/

_Check_return_
static STATUS
dialog_cmd_ctl_posn_query(
    P_DIALOG_CMD_CTL_POSN_QUERY p_dialog_cmd_ctl_posn_query)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;
    GDI_BOX gdi_box;
    FRAMED_BOX_STYLE b = FRAMED_BOX_NONE;

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_posn_query->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if(NULL == (p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_cmd_ctl_posn_query->dialog_control_id)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

    dialog_control_rect(p_dialog, p_dialog_cmd_ctl_posn_query->dialog_control_id, NULL);

    p_dialog_cmd_ctl_posn_query->outer_posn.x = p_dialog_ictl->pixit_rect.tl.x;
    p_dialog_cmd_ctl_posn_query->outer_posn.y = p_dialog_ictl->pixit_rect.tl.y;

    gdi_box.x0 = p_dialog_cmd_ctl_posn_query->outer_posn.x / PIXITS_PER_RISCOS;
    gdi_box.y0 = p_dialog_cmd_ctl_posn_query->outer_posn.y / PIXITS_PER_RISCOS;
    gdi_box.x1 = 0;
    gdi_box.y1 = 0;

    switch(p_dialog_ictl->dialog_control_type)
    {
    default: default_unhandled();
        break;

    /* add cases as required */

    case DIALOG_CONTROL_USER:
        b = p_dialog_ictl->data.user.border_style;
        break;
    }

#if RISCOS || 0 /* <<< fix me */
    host_framed_box_trim_frame(&gdi_box, b);
#endif

    p_dialog_cmd_ctl_posn_query->inner_posn.x = gdi_box.x0 * PIXITS_PER_RISCOS;
    p_dialog_cmd_ctl_posn_query->inner_posn.y = gdi_box.y0 * PIXITS_PER_RISCOS;

    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_CTL_SET_DEFAULT
*
******************************************************************************/

_Check_return_
static STATUS
dialog_cmd_ctl_set_default(
    P_DIALOG_CMD_CTL_SET_DEFAULT p_dialog_cmd_ctl_set_default)
{
    P_DIALOG p_dialog;
    DIALOG_CONTROL_ID old_default_dialog_control_id;

    assert(p_dialog_cmd_ctl_set_default->dialog_control_id != DIALOG_CONTROL_INVALID);

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_set_default->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if(p_dialog_cmd_ctl_set_default->dialog_control_id == p_dialog->default_dialog_control_id)
        return(STATUS_OK);

    if(p_dialog_cmd_ctl_set_default->dialog_control_id)
        if(NULL == p_dialog_ictl_from_control_id(p_dialog, p_dialog_cmd_ctl_set_default->dialog_control_id))
            return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

    old_default_dialog_control_id = p_dialog->default_dialog_control_id;
    p_dialog->default_dialog_control_id = p_dialog_cmd_ctl_set_default->dialog_control_id;

    if(p_dialog->default_dialog_control_id)
    {
        DIALOG_CMD_CTL_ENCODE dialog_cmd_ctl_encode;
        msgclr(dialog_cmd_ctl_encode);
        dialog_cmd_ctl_encode.h_dialog = p_dialog->h_dialog;
        dialog_cmd_ctl_encode.dialog_control_id = p_dialog->default_dialog_control_id;
        dialog_cmd_ctl_encode.bits = DIALOG_ENCODE_UPDATE_NOW;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_ENCODE, &dialog_cmd_ctl_encode));
    }

    if(old_default_dialog_control_id)
    {
        DIALOG_CMD_CTL_ENCODE dialog_cmd_ctl_encode;
        msgclr(dialog_cmd_ctl_encode);
        dialog_cmd_ctl_encode.h_dialog = p_dialog->h_dialog;
        dialog_cmd_ctl_encode.dialog_control_id = old_default_dialog_control_id;
        dialog_cmd_ctl_encode.bits = DIALOG_ENCODE_UPDATE_NOW;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_ENCODE, &dialog_cmd_ctl_encode));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_CTL_SIZE_ESTIMATE
*
* have a stab at making 'reasonable' sized controls
*
******************************************************************************/

static inline void
dialog_cmd_ctl_size_estimate_GROUPBOX(
    _InoutRef_  P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate,
    _InoutRef_  P_PIXIT_POINT p_est_size)
{
    const PC_DIALOG_CONTROL_DATA p_dialog_control_data = p_dialog_cmd_ctl_size_estimate->p_dialog_control_data;

    if(p_dialog_cmd_ctl_size_estimate->p_dialog_control->bits.logical_group)
    {   /* no intrinsic size */
        return;
    }

    p_est_size->x = DIALOG_STDGROUP_LM + DIALOG_STDGROUP_RM;
    p_est_size->y = DIALOG_STDGROUP_TM + DIALOG_STDGROUP_BM;
#if RISCOS
    p_est_size->y = 120 * PIXITS_PER_RISCOS;
#endif /* OS */

    if(NULL != p_dialog_control_data.groupbox)
    {
        PIXIT width = ui_width_from_p_ui_text(&p_dialog_control_data.groupbox->caption);

        if(FRAMED_BOX_GROUP == p_dialog_control_data.groupbox->bits.border_style)
        {
#if RISCOS
            width += 8 * PIXITS_PER_RISCOS;
#elif WINDOWS
            width += 6 * PIXITS_PER_WDU_H;
#endif /* OS */
        }

        p_est_size->x = MAX(p_est_size->x, width);
    }
}

static inline void
dialog_cmd_ctl_size_estimate_TEXTxx(
    _InoutRef_  P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate,
    _InoutRef_  P_PIXIT_POINT p_est_size)
{
    const PC_DIALOG_CONTROL_DATA p_dialog_control_data = p_dialog_cmd_ctl_size_estimate->p_dialog_control_data;
    PIXIT width = ui_width_from_p_ui_text(&p_dialog_control_data.statictext->caption);

    assert_EQ(offsetof32(DIALOG_CONTROL_DATA_STATICTEXT, caption), offsetof32(DIALOG_CONTROL_DATA_TEXTLABEL, caption));
    assert_EQ(offsetof32(DIALOG_CONTROL_DATA_STATICTEXT, caption), offsetof32(DIALOG_CONTROL_DATA_TEXTFRAME, caption));

    if(0 != width)
    {
#if RISCOS
        width += (4 * PIXITS_PER_RISCOS);
#elif WINDOWS
        width += (4 * PIXITS_PER_WDU_H); /* needs some bodging too */

        if(DIALOG_CONTROL_TEXTLABEL == UBF_UNPACK(DIALOG_CONTROL_TYPE, p_dialog_cmd_ctl_size_estimate->p_dialog_control->bits.packed_dialog_control_type))
        {
            static PIXIT colon_width = 0;
            if(0 == colon_width)
                colon_width = ui_width_from_tstr(TEXT(":"));
            if(!p_dialog_control_data.textlabel->bits.windows_no_colon)
                width += colon_width;
        }
#endif
    }

    p_est_size->x = width;
    p_est_size->y = DIALOG_STDTEXT_V;
}

static inline void
dialog_cmd_ctl_size_estimate_RADIOBUTTON(
    _InoutRef_  P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate,
    _InoutRef_  P_PIXIT_POINT p_est_size)
{
    const PC_DIALOG_CONTROL_DATA p_dialog_control_data = p_dialog_cmd_ctl_size_estimate->p_dialog_control_data;
    PIXIT width = ui_width_from_p_ui_text(&p_dialog_control_data.radiobutton->caption);

    if(0 != width)
        width += DIALOG_RADIOGAP_H; /* small gap between button and label */

    p_est_size->x = DIALOG_STDRADIO_H + width;
    p_est_size->y = DIALOG_STDRADIO_V;
}

static inline void
dialog_cmd_ctl_size_estimate_CHECKBOX(
    _InoutRef_  P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate,
    _InoutRef_  P_PIXIT_POINT p_est_size)
{
    const PC_DIALOG_CONTROL_DATA p_dialog_control_data = p_dialog_cmd_ctl_size_estimate->p_dialog_control_data;
    PIXIT width = ui_width_from_p_ui_text(&p_dialog_control_data.checkbox->caption);

#ifdef DIALOG_HAS_TRISTATE
    assert(offsetof32(DIALOG_CONTROL_DATA_CHECKBOX, caption) == offsetof32(DIALOG_CONTROL_DATA_TRISTATE, caption));
#endif

    if(0 != width)
        width += DIALOG_CHECKGAP_H; /* small gap between button and label */

    p_est_size->x = DIALOG_STDCHECK_H + width;
    p_est_size->y = DIALOG_STDCHECK_V;
}

static inline void
dialog_cmd_ctl_size_estimate_RADIOPICTURE(
    _InoutRef_  P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate,
    _InoutRef_  P_PIXIT_POINT p_est_size)
{
    UNREFERENCED_PARAMETER_InoutRef_(p_dialog_cmd_ctl_size_estimate);

    p_est_size->x = DIALOG_STDRADIO_H;
    p_est_size->y = DIALOG_STDRADIO_V;
}

static inline void
dialog_cmd_ctl_size_estimate_PUSHBUTTON(
    _InoutRef_  P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate,
    _InoutRef_  P_PIXIT_POINT p_est_size)
{
    const PC_DIALOG_CONTROL_DATA p_dialog_control_data = p_dialog_cmd_ctl_size_estimate->p_dialog_control_data;
    PIXIT width = ui_width_from_p_ui_text(&p_dialog_control_data.pushbutton->caption);

    p_est_size->x = DIALOG_PUSHBUTTONOVH_H + width;
    p_est_size->y = DIALOG_STDPUSHBUTTON_V;

    if( (IDOK == p_dialog_cmd_ctl_size_estimate->p_dialog_control->dialog_control_id) || (p_dialog_control_data.pushbutton->push_xx.def_pushbutton) )
    {
        p_est_size->x += 2 * DIALOG_DEFPUSHEXTRA_H;
        p_est_size->y += 2 * DIALOG_DEFPUSHEXTRA_V;
    }
}

static inline void
dialog_cmd_ctl_size_estimate_EDIT(
    _InoutRef_  P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate,
    _InoutRef_  P_PIXIT_POINT p_est_size)
{
    UNREFERENCED_PARAMETER_InoutRef_(p_dialog_cmd_ctl_size_estimate);

    p_est_size->x = 180 * PIXITS_PER_RISCOS;
    p_est_size->y = DIALOG_STDEDIT_V;
}

static inline void
dialog_cmd_ctl_size_estimate_BUMP_XX(
    _InoutRef_  P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate,
    _InoutRef_  P_PIXIT_POINT p_est_size)
{
    PIXIT width = 120 * PIXITS_PER_RISCOS; /* edit_xx */

    if(DIALOG_CONTROL_BUMP_F64 == UBF_UNPACK(DIALOG_CONTROL_TYPE, p_dialog_cmd_ctl_size_estimate->p_dialog_control->bits.packed_dialog_control_type))
        width += 60 * PIXITS_PER_RISCOS;

    p_est_size->x = width + (60 * PIXITS_PER_RISCOS) /* inc */ + (60 * PIXITS_PER_RISCOS) /* dec */;
    p_est_size->y = DIALOG_STDBUMP_V;
}

static inline void
dialog_cmd_ctl_size_estimate_LIST_XX(
    _InoutRef_  P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate,
    _InoutRef_  P_PIXIT_POINT p_est_size)
{
    const PC_DIALOG_CONTROL_DATA p_dialog_control_data = p_dialog_cmd_ctl_size_estimate->p_dialog_control_data;

    p_est_size->x = DIALOG_STDLISTOVH_H;
    p_est_size->y = DIALOG_STDLISTOVH_V;

    if( (NULL != p_dialog_control_data.list_text) && !p_dialog_control_data.list_text->list_xx.bits.force_v_scroll )
        p_est_size->x = DIALOG_MINLISTOVH_H;

#if RISCOS
    p_est_size->x += (PIXITS_PER_RISCOS * 2) << host_modevar_cache_current.XEigFactor; /* thin border left and right */
    p_est_size->y += (PIXITS_PER_RISCOS * 2) << host_modevar_cache_current.YEigFactor; /* thin border top and bottom */
#endif
}

static inline void
dialog_cmd_ctl_size_estimate_COMBO_XX(
    _InoutRef_  P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate,
    _InoutRef_  P_PIXIT_POINT p_est_size)
{
    PIXIT add_width;

#if RISCOS
    add_width = (PIXITS_PER_RISCOS * 2) << host_modevar_cache_current.XEigFactor; /* thin border either side */

    if(p_dialog_cmd_ctl_size_estimate->p_dialog_control_data.combo_text->combo_xx.edit_xx.bits.read_only)
        add_width = (4 * PIXITS_PER_RISCOS) * 2; /* fixed sized border either side */
#elif WINDOWS
    UNREFERENCED_PARAMETER_InoutRef_(p_dialog_cmd_ctl_size_estimate);

    {
    const int vscroll_pixels_cx = GetSystemMetrics(SM_CXVSCROLL);
    const int edge_pixels_cx = GetSystemMetrics(SM_CXEDGE);
    GDI_SIZE PixelsPerInch;

    host_get_pixel_size(NULL /*screen*/, &PixelsPerInch.cx, NULL /*cy*/); /* Get current pixel size for a dialog e.g. 96 or 120 */

    /* +2 is a bit hacky for Windows Classic theme on XP */ /* DPI-aware */
    add_width = idiv_ceil_u((vscroll_pixels_cx + 2 * edge_pixels_cx + 2) * PIXITS_PER_PIXEL * 96, PixelsPerInch.cx);
    } /*block*/
#endif

    p_est_size->x = DIALOG_STDCOMBOOVH_H + add_width;
    p_est_size->y = DIALOG_STDCOMBO_V;
}

_Check_return_
static STATUS
dialog_cmd_ctl_size_estimate(
    _InoutRef_  P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate)
{
    PIXIT_POINT est_size;

    est_size.x = 0;
    est_size.y = 0;

    PTR_ASSERT(p_dialog_cmd_ctl_size_estimate->p_dialog_control);

    switch(UBF_UNPACK(DIALOG_CONTROL_TYPE, p_dialog_cmd_ctl_size_estimate->p_dialog_control->bits.packed_dialog_control_type))
    {
    default:
        break;

    case DIALOG_CONTROL_GROUPBOX:
        dialog_cmd_ctl_size_estimate_GROUPBOX(p_dialog_cmd_ctl_size_estimate, &est_size);
        break;

    case DIALOG_CONTROL_STATICTEXT:
    case DIALOG_CONTROL_TEXTLABEL:
    case DIALOG_CONTROL_TEXTFRAME:
        dialog_cmd_ctl_size_estimate_TEXTxx(p_dialog_cmd_ctl_size_estimate, &est_size);
        break;

    case DIALOG_CONTROL_RADIOBUTTON:
        dialog_cmd_ctl_size_estimate_RADIOBUTTON(p_dialog_cmd_ctl_size_estimate, &est_size);
        break;

    case DIALOG_CONTROL_CHECKBOX:
#ifdef DIALOG_HAS_TRISTATE
    case DIALOG_CONTROL_TRISTATE:
#endif
        dialog_cmd_ctl_size_estimate_CHECKBOX(p_dialog_cmd_ctl_size_estimate, &est_size);
        break;

    case DIALOG_CONTROL_RADIOPICTURE:
    case DIALOG_CONTROL_CHECKPICTURE:
#ifdef DIALOG_HAS_TRISTATE
    case DIALOG_CONTROL_TRIPICTURE:
#endif
        dialog_cmd_ctl_size_estimate_RADIOPICTURE(p_dialog_cmd_ctl_size_estimate, &est_size);
        break;

    case DIALOG_CONTROL_PUSHBUTTON:
        dialog_cmd_ctl_size_estimate_PUSHBUTTON(p_dialog_cmd_ctl_size_estimate, &est_size);
        break;

    case DIALOG_CONTROL_EDIT:
        dialog_cmd_ctl_size_estimate_EDIT(p_dialog_cmd_ctl_size_estimate, &est_size);
        break;

    case DIALOG_CONTROL_BUMP_S32:
    case DIALOG_CONTROL_BUMP_F64:
        dialog_cmd_ctl_size_estimate_BUMP_XX(p_dialog_cmd_ctl_size_estimate, &est_size);
        break;

    case DIALOG_CONTROL_LIST_S32:
    case DIALOG_CONTROL_LIST_TEXT:
        dialog_cmd_ctl_size_estimate_LIST_XX(p_dialog_cmd_ctl_size_estimate, &est_size);
        break;

    case DIALOG_CONTROL_COMBO_TEXT:
    case DIALOG_CONTROL_COMBO_S32:
        dialog_cmd_ctl_size_estimate_COMBO_XX(p_dialog_cmd_ctl_size_estimate, &est_size);
        break;
    }

    p_dialog_cmd_ctl_size_estimate->size = est_size;

    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_CTL_SIZE_QUERY
*
* inform caller of size of a control
*
******************************************************************************/

_Check_return_
static STATUS
dialog_cmd_ctl_size_query(
    P_DIALOG_CMD_CTL_SIZE_QUERY p_dialog_cmd_ctl_size_query)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;
    GDI_BOX gdi_box;
    FRAMED_BOX_STYLE b = FRAMED_BOX_NONE;

    if((p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_size_query->h_dialog)) == NULL)
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if((p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_cmd_ctl_size_query->dialog_control_id)) == NULL)
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

    dialog_control_rect(p_dialog, p_dialog_cmd_ctl_size_query->dialog_control_id, NULL);

    p_dialog_cmd_ctl_size_query->outer_size.x = pixit_rect_width(&p_dialog_ictl->pixit_rect);
    p_dialog_cmd_ctl_size_query->outer_size.y = pixit_rect_height(&p_dialog_ictl->pixit_rect);

    gdi_box.x0 = 0;
    gdi_box.y0 = 0;
    gdi_box.x1 = p_dialog_cmd_ctl_size_query->outer_size.x / PIXITS_PER_RISCOS;
    gdi_box.y1 = p_dialog_cmd_ctl_size_query->outer_size.y / PIXITS_PER_RISCOS;

    switch(p_dialog_ictl->dialog_control_type)
    {
    default: default_unhandled();
        break;

    /* add cases as required */

    case DIALOG_CONTROL_USER:
        b = p_dialog_ictl->data.user.border_style;
        break;
    }

#if RISCOS || 0 /* <<< fix me */
    host_framed_box_trim_frame(&gdi_box, b);
#endif

    p_dialog_cmd_ctl_size_query->inner_size.x = (gdi_box.x1 - gdi_box.x0) * PIXITS_PER_RISCOS;
    p_dialog_cmd_ctl_size_query->inner_size.y = (gdi_box.y1 - gdi_box.y0) * PIXITS_PER_RISCOS;

    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_CTL_SIZE_SET
*
* resize a control
*
******************************************************************************/

_Check_return_
static STATUS
dialog_cmd_ctl_size_set(
    _InRef_     P_DIALOG_CMD_CTL_SIZE_SET p_dialog_cmd_ctl_size_set)
{
    /* no longer required */
    UNREFERENCED_PARAMETER_InRef_(p_dialog_cmd_ctl_size_set);
    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_CTL_STATE_QUERY
*
* caller owns returned state and may steal it or dispose of it
*
******************************************************************************/

_Check_return_
static STATUS
dialog_cmd_ctl_state_query(
    P_DIALOG_CMD_CTL_STATE_QUERY p_dialog_cmd_ctl_state_query)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;
    STATUS status = STATUS_OK;

    assert(p_dialog_cmd_ctl_state_query->dialog_control_id != DIALOG_CONTROL_INVALID);
    assert(p_dialog_cmd_ctl_state_query->bits              != 0xBCBCBCBC);

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_state_query->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if(NULL == (p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_cmd_ctl_state_query->dialog_control_id)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

    p_dialog_cmd_ctl_state_query->dialog_control_type = p_dialog_ictl->dialog_control_type;

    if(p_dialog_cmd_ctl_state_query->bits & DIALOG_STATE_QUERY_ALTERNATE)
    {
        switch(p_dialog_ictl->dialog_control_type)
        {
        default: default_unhandled();
            break;

        case DIALOG_CONTROL_LIST_S32:
        case DIALOG_CONTROL_LIST_TEXT:
            /* return the selected item */
            p_dialog_cmd_ctl_state_query->state.list_text.itemno  = p_dialog_ictl->state.list_text.itemno;
            break;

        case DIALOG_CONTROL_COMBO_S32:
        case DIALOG_CONTROL_COMBO_TEXT:
            /* return the selected item */
            p_dialog_cmd_ctl_state_query->state.combo_text.itemno = p_dialog_ictl->state.combo_text.itemno;
            break;
        }
    }
    else
    {
        switch(p_dialog_ictl->dialog_control_type)
        {
        case DIALOG_CONTROL_STATICPICTURE:
        case DIALOG_CONTROL_PUSHPICTURE:
            break;

        default:
#if CHECKING
        case DIALOG_CONTROL_RADIOBUTTON:
        case DIALOG_CONTROL_RADIOPICTURE:
            assert0();

            /*FALLTHRU*/

        case DIALOG_CONTROL_GROUPBOX:
        case DIALOG_CONTROL_CHECKBOX:
        case DIALOG_CONTROL_CHECKPICTURE:
        case DIALOG_CONTROL_BUMP_S32:
        case DIALOG_CONTROL_BUMP_F64:
        case DIALOG_CONTROL_LIST_S32:
        case DIALOG_CONTROL_COMBO_S32:
        case DIALOG_CONTROL_USER:
#ifdef DIALOG_HAS_TRISTATE
        case DIALOG_CONTROL_TRISTATE:
        case DIALOG_CONTROL_TRIPICTURE:
#endif
#endif
            p_dialog_cmd_ctl_state_query->state = p_dialog_ictl->state;
            break;

        case DIALOG_CONTROL_STATICTEXT:
        case DIALOG_CONTROL_TEXTLABEL:
        case DIALOG_CONTROL_TEXTFRAME:
        case DIALOG_CONTROL_PUSHBUTTON:
            status = ui_text_copy(&p_dialog_cmd_ctl_state_query->state.pushbutton, &p_dialog_ictl->state.pushbutton);
            break;

        case DIALOG_CONTROL_EDIT:
        case DIALOG_CONTROL_LIST_TEXT:
        case DIALOG_CONTROL_COMBO_TEXT:
            p_dialog_cmd_ctl_state_query->state.list_text.itemno = p_dialog_ictl->state.list_text.itemno;
            status = ui_text_copy(&p_dialog_cmd_ctl_state_query->state.list_text.ui_text, &p_dialog_ictl->state.list_text.ui_text);
            break;
        }
    }

    return(status);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_CTL_STATE_QUERY_DISPOSE
*
* caller wants to dispose of a state returned from query
*
******************************************************************************/

_Check_return_
static STATUS
dialog_cmd_ctl_state_query_dispose(
    P_DIALOG_CMD_CTL_STATE_QUERY p_dialog_cmd_ctl_state_query_dispose)
{
    assert(p_dialog_cmd_ctl_state_query_dispose->bits                != 0xBCBCBCBC);
    assert(p_dialog_cmd_ctl_state_query_dispose->dialog_control_type != 0xBCBCBCBC);

    if(p_dialog_cmd_ctl_state_query_dispose->bits & DIALOG_STATE_QUERY_ALTERNATE)
    {
#if 0
        switch(p_dialog_cmd_ctl_state_query_dispose->dialog_control_type)
        {
        default:
            break;
        }
#endif
    }
    else
    {
        switch(p_dialog_cmd_ctl_state_query_dispose->dialog_control_type)
        {
        default:
#if CHECKING
        case DIALOG_CONTROL_RADIOBUTTON:
        case DIALOG_CONTROL_RADIOPICTURE:
            assert0();

            /*FALLTHRU*/

        case DIALOG_CONTROL_STATICPICTURE:
        case DIALOG_CONTROL_PUSHPICTURE:

        case DIALOG_CONTROL_GROUPBOX:
        case DIALOG_CONTROL_CHECKBOX:
        case DIALOG_CONTROL_CHECKPICTURE:
        case DIALOG_CONTROL_BUMP_S32:
        case DIALOG_CONTROL_BUMP_F64:
        case DIALOG_CONTROL_LIST_S32:
        case DIALOG_CONTROL_COMBO_S32:
        case DIALOG_CONTROL_USER:
#ifdef DIALOG_HAS_TRISTATE
        case DIALOG_CONTROL_TRISTATE:
        case DIALOG_CONTROL_TRIPICTURE:
#endif
#endif
            break;

        case DIALOG_CONTROL_STATICTEXT:
        case DIALOG_CONTROL_TEXTLABEL:
        case DIALOG_CONTROL_TEXTFRAME:
        case DIALOG_CONTROL_PUSHBUTTON:
            ui_text_dispose(&p_dialog_cmd_ctl_state_query_dispose->state.pushbutton);
            break;

        case DIALOG_CONTROL_EDIT:
        case DIALOG_CONTROL_LIST_TEXT:
        case DIALOG_CONTROL_COMBO_TEXT:
            ui_text_dispose(&p_dialog_cmd_ctl_state_query_dispose->state.list_text.ui_text);
            break;
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_CTL_STATE_SET
*
* dialog always copies state
*
******************************************************************************/

static void
dialog_cmd_ctl_state_set_list_s32(
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InRef_     P_DIALOG_CMD_CTL_STATE_SET p_dialog_cmd_ctl_state_set,
    _InoutRef_  P_BOOL p_changed)
{
    const UI_DATA_TYPE ui_data_type = UI_DATA_TYPE_S32;
    S32 n_items, itemno;

    *p_changed = (p_dialog_ictl->state.list_s32.s32 != p_dialog_cmd_ctl_state_set->state.list_s32.s32);

    if(!*p_changed)
        return;

    n_items = ui_data_n_items_query(ui_data_type, p_dialog_ictl->data.list_xx.list_xx.p_ui_source);

    p_dialog_ictl->state.list_s32.s32 = p_dialog_cmd_ctl_state_set->state.list_s32.s32;

    /* lookup this new state in source to find selected item */
    p_dialog_ictl->state.list_s32.itemno = DIALOG_CTL_STATE_LIST_ITEM_NONE;

    for(itemno = 0; itemno < n_items; ++itemno)
    {
        UI_DATA_TYPE this_ui_data_type = ui_data_type;
        UI_DATA ui_data;

        status_assert(ui_data_query(&this_ui_data_type, p_dialog_ictl->data.list_xx.list_xx.p_ui_source, itemno, &ui_data));
        assert(this_ui_data_type == ui_data_type);

        if(ui_data.s32 == p_dialog_ictl->state.list_s32.s32)
        {
            p_dialog_ictl->state.list_s32.itemno = itemno;
            break;
        }
    }
}

_Check_return_
static STATUS
dialog_cmd_ctl_state_set_list_text(
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InRef_     P_DIALOG_CMD_CTL_STATE_SET p_dialog_cmd_ctl_state_set,
    _InoutRef_  P_BOOL p_changed)
{
    const UI_DATA_TYPE ui_data_type = UI_DATA_TYPE_TEXT;
    S32 n_items, itemno;

    status_return(ui_text_state_change(&p_dialog_ictl->state.list_text.ui_text, &p_dialog_cmd_ctl_state_set->state.list_text.ui_text, p_changed));

    if(!*p_changed)
        return(STATUS_OK);

    n_items = ui_data_n_items_query(ui_data_type, p_dialog_ictl->data.list_xx.list_xx.p_ui_source);

    /* lookup this new state in source to find selected item */
    p_dialog_ictl->state.list_text.itemno = DIALOG_CTL_STATE_LIST_ITEM_NONE;

    for(itemno = 0; itemno < n_items; ++itemno)
    {
        UI_DATA_TYPE this_ui_data_type = ui_data_type;
        UI_DATA ui_data;

        status_assert(ui_data_query(&this_ui_data_type, p_dialog_ictl->data.list_xx.list_xx.p_ui_source, itemno, &ui_data));
        assert(this_ui_data_type == ui_data_type);

        if(0 == ui_text_compare(&ui_data.text, &p_dialog_ictl->state.list_text.ui_text, 0, 0))
        {
            p_dialog_ictl->state.list_text.itemno = itemno;
            break;
        }
    }

    return(STATUS_OK);
}

static void
dialog_cmd_ctl_state_set_combo_s32(
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InRef_     P_DIALOG_CMD_CTL_STATE_SET p_dialog_cmd_ctl_state_set,
    _InoutRef_  P_BOOL p_changed)
{
    const UI_DATA_TYPE ui_data_type = UI_DATA_TYPE_S32;
    S32 n_items, itemno;

    *p_changed = (p_dialog_ictl->state.combo_s32.s32 != p_dialog_cmd_ctl_state_set->state.combo_s32.s32);

    if(!*p_changed)
        return;

    n_items = ui_data_n_items_query(ui_data_type, p_dialog_ictl->data.combo_xx.list_xx.p_ui_source);

    p_dialog_ictl->state.combo_s32.s32 = p_dialog_cmd_ctl_state_set->state.combo_s32.s32;

    /* lookup this new state in source to find selected item */
    p_dialog_ictl->state.combo_s32.itemno = DIALOG_CTL_STATE_LIST_ITEM_OTHER;

    for(itemno = 0; itemno < n_items; ++itemno)
    {
        UI_DATA_TYPE this_ui_data_type = ui_data_type;
        UI_DATA ui_data;

        status_assert(ui_data_query(&this_ui_data_type, p_dialog_ictl->data.combo_xx.list_xx.p_ui_source, itemno, &ui_data));
        assert(this_ui_data_type == ui_data_type);

        if(ui_data.s32 == p_dialog_ictl->state.combo_s32.s32)
        {
            p_dialog_ictl->state.combo_s32.itemno = itemno;
            break;
        }
    }
}

_Check_return_
static STATUS
dialog_cmd_ctl_state_set_combo_text(
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InRef_     P_DIALOG_CMD_CTL_STATE_SET p_dialog_cmd_ctl_state_set,
    _InoutRef_  P_BOOL p_changed)
{
    const UI_DATA_TYPE ui_data_type = UI_DATA_TYPE_TEXT;
    S32 n_items, itemno;

    status_return(ui_text_state_change(&p_dialog_ictl->state.combo_text.ui_text, &p_dialog_cmd_ctl_state_set->state.combo_text.ui_text, p_changed));

    if(!*p_changed)
        return(STATUS_OK);

    n_items = ui_data_n_items_query(ui_data_type, p_dialog_ictl->data.combo_xx.list_xx.p_ui_source);

    /* lookup this new state in source to find selected item */
    p_dialog_ictl->state.combo_text.itemno = DIALOG_CTL_STATE_LIST_ITEM_OTHER;

    for(itemno = 0; itemno < n_items; ++itemno)
    {
        UI_DATA_TYPE this_ui_data_type = ui_data_type;
        UI_DATA ui_data;

        status_assert(ui_data_query(&this_ui_data_type, p_dialog_ictl->data.combo_xx.list_xx.p_ui_source, itemno, &ui_data));
        assert(this_ui_data_type == ui_data_type);

        if(0 == ui_text_compare(&ui_data.text, &p_dialog_ictl->state.combo_text.ui_text, 0, 0))
        {
            p_dialog_ictl->state.combo_text.itemno = itemno;
            break;
        }
    }

    return(STATUS_OK);
}

static void
dialog_cmd_ctl_state_set_list_xx_alternate(
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InRef_     P_DIALOG_CMD_CTL_STATE_SET p_dialog_cmd_ctl_state_set,
    _InoutRef_  P_BOOL p_changed)
{
    /* setting by selected_item */
    *p_changed = (p_dialog_ictl->state.list_text.itemno != p_dialog_cmd_ctl_state_set->state.list_text.itemno);

    if(!*p_changed)
        return;

    /* lookup 'normal' state using this itemno iff it's kosher */
    p_dialog_ictl->state.list_text.itemno = p_dialog_cmd_ctl_state_set->state.list_text.itemno;

    if(p_dialog_ictl->state.list_text.itemno >= 0)
    {
        UI_DATA ui_data;
        UI_DATA_TYPE ui_data_type = (p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_LIST_S32)
                                    ? UI_DATA_TYPE_S32
                                    : UI_DATA_TYPE_TEXT;
        UI_DATA_TYPE this_ui_data_type = ui_data_type;

        status_assert(ui_data_query(&this_ui_data_type, p_dialog_ictl->data.list_xx.list_xx.p_ui_source, p_dialog_ictl->state.list_text.itemno, &ui_data));

        if(p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_LIST_S32)
        {
            /* this_ui_data_type == ui_data_type fails if item doesn't exist */
            if(this_ui_data_type == ui_data_type)
                p_dialog_ictl->state.list_s32.s32 = ui_data.s32;
        }
        else
        {
            /* steal this */
            ui_text_dispose(&p_dialog_ictl->state.list_text.ui_text);

            /* this_ui_data_type == ui_data_type fails if item doesn't exist */
            if(this_ui_data_type == ui_data_type)
                p_dialog_ictl->state.list_text.ui_text = ui_data.text;
        }
    }
}

static void
dialog_cmd_ctl_state_set_combo_xx_alternate(
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InRef_     P_DIALOG_CMD_CTL_STATE_SET p_dialog_cmd_ctl_state_set,
    _InoutRef_  P_BOOL p_changed)
{
    /* setting by selected_item */
    *p_changed = (p_dialog_ictl->state.combo_text.itemno != p_dialog_cmd_ctl_state_set->state.combo_text.itemno);

    if(!*p_changed)
        return;

    /* lookup 'normal' state using this itemno iff it's kosher */
    if((p_dialog_ictl->state.combo_text.itemno = p_dialog_cmd_ctl_state_set->state.combo_text.itemno) >= 0)
    {
        UI_DATA ui_data;
        UI_DATA_TYPE ui_data_type = (p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_COMBO_S32)
                                    ? UI_DATA_TYPE_S32
                                    : UI_DATA_TYPE_TEXT;
        UI_DATA_TYPE this_ui_data_type = ui_data_type;

        status_assert(ui_data_query(&this_ui_data_type, p_dialog_ictl->data.combo_xx.list_xx.p_ui_source, p_dialog_ictl->state.combo_text.itemno, &ui_data));
        assert(this_ui_data_type == ui_data_type);

        if(p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_COMBO_S32)
            p_dialog_ictl->state.combo_s32.s32 = ui_data.s32;
        else
        {
            /* steal this */
            ui_text_dispose(&p_dialog_ictl->state.combo_text.ui_text);
            p_dialog_ictl->state.combo_text.ui_text = ui_data.text;
        }
    }
}

_Check_return_
static STATUS
dialog_cmd_ctl_state_set(
    P_DIALOG_CMD_CTL_STATE_SET p_dialog_cmd_ctl_state_set)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL_GROUP p_parent_ictls;
    P_DIALOG_ICTL p_dialog_ictl, message_p_dialog_ictl;
    DIALOG_CONTROL_ID encode_control_id;
    BOOL changed;
    BOOL force_update;

    assert(p_dialog_cmd_ctl_state_set->dialog_control_id != DIALOG_CONTROL_INVALID);

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_ctl_state_set->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    encode_control_id = p_dialog_cmd_ctl_state_set->dialog_control_id;

    if(NULL == (p_dialog_ictl = p_dialog_ictl_from_control_id_in(&p_dialog->ictls, encode_control_id, &p_parent_ictls)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

    force_update = p_dialog_ictl->bits.force_update;

    message_p_dialog_ictl = p_dialog_ictl;

    if(p_dialog_cmd_ctl_state_set->bits & DIALOG_STATE_SET_ALTERNATE)
    {
        switch(p_dialog_ictl->dialog_control_type)
        {
        default: default_unhandled();
            return(STATUS_OK);

        case DIALOG_CONTROL_LIST_S32:
        case DIALOG_CONTROL_LIST_TEXT:
            dialog_cmd_ctl_state_set_list_xx_alternate(p_dialog_ictl, p_dialog_cmd_ctl_state_set, &changed);
            break;

        case DIALOG_CONTROL_COMBO_S32:
        case DIALOG_CONTROL_COMBO_TEXT:
            dialog_cmd_ctl_state_set_combo_xx_alternate(p_dialog_ictl, p_dialog_cmd_ctl_state_set, &changed);
            break;
        }
    }
    else
    {
        switch(p_dialog_ictl->dialog_control_type)
        {
        default: default_unhandled();
            return(STATUS_OK);

        case DIALOG_CONTROL_STATICTEXT:
        case DIALOG_CONTROL_TEXTLABEL:
        case DIALOG_CONTROL_TEXTFRAME:
            status_return(ui_text_state_change(&p_dialog_ictl->state.statictext, &p_dialog_cmd_ctl_state_set->state.statictext, &changed));

            if(changed)
            {
#if RISCOS
                tstr_clr(&p_dialog_ictl->riscos.caption);
                status_assert(dialog_riscos_remove_escape(p_dialog_ictl, &p_dialog_ictl->state.statictext));
#endif
            }
            break;

        case DIALOG_CONTROL_PUSHBUTTON:
            status_return(ui_text_state_change(&p_dialog_ictl->state.pushbutton, &p_dialog_cmd_ctl_state_set->state.pushbutton, &changed));

            if(changed)
            {
#if RISCOS
                tstr_clr(&p_dialog_ictl->riscos.caption);
                status_assert(dialog_riscos_remove_escape(p_dialog_ictl, &p_dialog_ictl->state.pushbutton));
#endif
            }
            break;

        case DIALOG_CONTROL_RADIOBUTTON:
        case DIALOG_CONTROL_RADIOPICTURE:
            assert(p_parent_ictls != &p_dialog->ictls);
            assert0();

            /*FALLTHRU*/

        case DIALOG_CONTROL_GROUPBOX:
            {
            /* master copy of radiobutton state held in containing group */
            P_S32 p_state;

            if(p_dialog_ictl->dialog_control_type != DIALOG_CONTROL_GROUPBOX)
            {
                P_U8 p_u8 = (P_U8) p_parent_ictls;
                p_u8 -= (offsetof32(DIALOG_ICTL, data) + offsetof32(union DIALOG_ICTL_DATA, groupbox) + offsetof32(struct DIALOG_ICTL_DATA_GROUPBOX, ictls));
                p_u8 += (offsetof32(DIALOG_ICTL, state) + offsetof32(DIALOG_CTL_STATE, radiobutton));
                p_state = (P_S32) p_u8;
            }
            else
                p_state = &p_dialog_ictl->state.radiobutton;

            changed = (*p_state != p_dialog_cmd_ctl_state_set->state.radiobutton);

            /* note that it is valid to set up a state that is not representable by the radio buttons */
            if(changed)
            {
                ARRAY_INDEX i;

                *p_state = p_dialog_cmd_ctl_state_set->state.radiobutton;

                if(p_dialog_ictl->dialog_control_type != DIALOG_CONTROL_GROUPBOX)
                    message_p_dialog_ictl = NULL;
                else
                    p_parent_ictls = &p_dialog_ictl->data.groupbox.ictls;

                encode_control_id = 0;

                /* find old button (if at all) and reencode just that one and the new active one */
                for(i = 0; i < n_ictls_from_group(p_parent_ictls); ++i)
                {
                    P_DIALOG_ICTL this_p_dialog_ictl = p_dialog_ictl_from(p_parent_ictls, i);

                    if( (this_p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_RADIOBUTTON) ||
                        (this_p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_RADIOPICTURE) )
                    {
                        BOOL old_active = this_p_dialog_ictl->bits.radiobutton_active;

                        /* inform only the first button of group if radiobutton set */
                        if(!message_p_dialog_ictl)
                            message_p_dialog_ictl = this_p_dialog_ictl;

                        /* replicate state out amongst our siblings */
                        this_p_dialog_ictl->state.radiobutton = p_dialog_cmd_ctl_state_set->state.radiobutton;

                        this_p_dialog_ictl->bits.radiobutton_active = (this_p_dialog_ictl->state.radiobutton ==
                                                                       this_p_dialog_ictl->p_dialog_control_data.radiobutton->activate_state);

                        if(this_p_dialog_ictl->bits.radiobutton_active)
                            encode_control_id = this_p_dialog_ictl->dialog_control_id;

                        if(old_active)
                        {
                            DIALOG_CMD_CTL_ENCODE dialog_cmd_ctl_encode;
                            msgclr(dialog_cmd_ctl_encode);
                            dialog_cmd_ctl_encode.h_dialog = p_dialog->h_dialog;
                            dialog_cmd_ctl_encode.dialog_control_id = this_p_dialog_ictl->dialog_control_id;
                            dialog_cmd_ctl_encode.bits = 0;
                            status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_ENCODE, &dialog_cmd_ctl_encode));
                        }
                    }
                }
            }

            break;
            }

        case DIALOG_CONTROL_CHECKBOX:
        case DIALOG_CONTROL_CHECKPICTURE:
            myassert1x((p_dialog_cmd_ctl_state_set->state.checkbox == DIALOG_BUTTONSTATE_OFF) ||
                        (p_dialog_cmd_ctl_state_set->state.checkbox == DIALOG_BUTTONSTATE_ON),
                      TEXT("p_dialog_cmd_ctl_state_set->state.checkbox = ") U32_XTFMT, p_dialog_cmd_ctl_state_set->state.checkbox);
            changed = (p_dialog_ictl->state.checkbox != p_dialog_cmd_ctl_state_set->state.checkbox);
            p_dialog_ictl->state.checkbox = p_dialog_cmd_ctl_state_set->state.checkbox;
            break;

#ifdef DIALOG_HAS_TRISTATE
        case DIALOG_CONTROL_TRISTATE:
        case DIALOG_CONTROL_TRIPICTURE:
            myassert1x((p_dialog_cmd_ctl_state_set->state.tristate == DIALOG_TRISTATE_OFF)       ||
                      (p_dialog_cmd_ctl_state_set->state.tristate == DIALOG_TRISTATE_ON)        ||
                      (p_dialog_cmd_ctl_state_set->state.tristate == DIALOG_TRISTATE_DONT_CARE),
                      TEXT("p_dialog_cmd_ctl_state_set->state.tristate = ") U32_XTFMT, p_dialog_cmd_ctl_state_set->state.tristate);
            changed = (p_dialog_ictl->state.tristate != p_dialog_cmd_ctl_state_set->state.tristate);
            p_dialog_ictl->state.tristate = p_dialog_cmd_ctl_state_set->state.tristate;
            break;
#endif

        case DIALOG_CONTROL_USER:
            changed = (p_dialog_ictl->state.user.u32 != p_dialog_cmd_ctl_state_set->state.user.u32);
            p_dialog_ictl->state.user = p_dialog_cmd_ctl_state_set->state.user;
            break;

        case DIALOG_CONTROL_EDIT:
            status_return(ui_text_state_change(&p_dialog_ictl->state.edit.ui_text, &p_dialog_cmd_ctl_state_set->state.edit.ui_text, &changed));
            break;

        case DIALOG_CONTROL_BUMP_S32:
            changed = (p_dialog_ictl->state.bump_s32 != p_dialog_cmd_ctl_state_set->state.bump_s32);
            p_dialog_ictl->state.bump_s32 = p_dialog_cmd_ctl_state_set->state.bump_s32;
            break;

        case DIALOG_CONTROL_BUMP_F64:
            changed = (p_dialog_ictl->state.bump_f64 != p_dialog_cmd_ctl_state_set->state.bump_f64);
            p_dialog_ictl->state.bump_f64 = p_dialog_cmd_ctl_state_set->state.bump_f64;
            break;

        case DIALOG_CONTROL_LIST_S32:
            dialog_cmd_ctl_state_set_list_s32(p_dialog_ictl, p_dialog_cmd_ctl_state_set, &changed);
            break;

        case DIALOG_CONTROL_LIST_TEXT:
            status_return(dialog_cmd_ctl_state_set_list_text(p_dialog_ictl, p_dialog_cmd_ctl_state_set, &changed));
            break;

        case DIALOG_CONTROL_COMBO_S32:
            dialog_cmd_ctl_state_set_combo_s32(p_dialog_ictl, p_dialog_cmd_ctl_state_set, &changed);
            break;

        case DIALOG_CONTROL_COMBO_TEXT:
            status_return(dialog_cmd_ctl_state_set_combo_text(p_dialog_ictl, p_dialog_cmd_ctl_state_set, &changed));
            break;
        }
    }

    if(changed || force_update)
    {
        if(encode_control_id)
        {
            /* common exit reflects new state into all views of control */
            DIALOG_CMD_CTL_ENCODE dialog_cmd_ctl_encode;
            msgclr(dialog_cmd_ctl_encode);
            dialog_cmd_ctl_encode.h_dialog = p_dialog->h_dialog;
            dialog_cmd_ctl_encode.dialog_control_id = encode_control_id;
            dialog_cmd_ctl_encode.bits = p_dialog_cmd_ctl_state_set->bits & DIALOG_STATE_SET_UPDATE_NOW;
            status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_ENCODE, &dialog_cmd_ctl_encode));
        }
    }

    if(p_dialog_cmd_ctl_state_set->bits & DIALOG_STATE_SET_DONT_MSG)
        message_p_dialog_ictl = NULL;

    if(message_p_dialog_ictl)
        if(changed || (p_dialog_cmd_ctl_state_set->bits & DIALOG_STATE_SET_ALWAYS_MSG))
        {   /* inform client of control's changed state */
            P_PROC_DIALOG_EVENT p_proc_client;
            DIALOG_MSG_CTL_STATE_CHANGE dialog_msg_ctl_state_change;
            msgclr(dialog_msg_ctl_state_change);

            if(NULL != (p_proc_client = dialog_find_handler(p_dialog, encode_control_id, &dialog_msg_ctl_state_change.client_handle)))
            {
                DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_state_change, p_dialog, message_p_dialog_ictl);
                /* give them a cheap and nasty replica, not a full copy. If they want that, they can ask for it! */
                dialog_msg_ctl_state_change.new_state = message_p_dialog_ictl->state;
                status_assert(dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_STATE_CHANGE, &dialog_msg_ctl_state_change, p_proc_client));
            }
        }

    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_DEFPUSHBUTTON
*
* activate the default pushbutton
*
******************************************************************************/

_Check_return_
static STATUS
dialog_cmd_defpushbutton(
    P_DIALOG_CMD_DEFPUSHBUTTON p_dialog_cmd_defpushbutton)
{
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_defpushbutton->h_dialog);

    if(NULL == p_dialog)
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    if(p_dialog->default_dialog_control_id)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog->default_dialog_control_id);

        if(NULL != p_dialog_ictl)
            return(dialog_click_pushbutton(p_dialog, p_dialog_ictl, 0, p_dialog_cmd_defpushbutton->double_dialog_control_id));
    }

    host_bleep();

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_cmd_note_position_trash(void)
{
    dialog_statics.note_position = FALSE;
    dialog_statics.noted_position = FALSE;
    dialog_statics.noted_gdi_tl.x = 0;
    dialog_statics.noted_gdi_tl.y = 0;
    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_cmd_hwnd_query(
    P_DIALOG_CMD_HWND_QUERY p_dialog_cmd_hwnd_query)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL_GROUP p_parent_ictls;
    P_DIALOG_ICTL p_dialog_ictl;
    DIALOG_CONTROL_ID dialog_control_id;

    assert(p_dialog_cmd_hwnd_query->dialog_control_id != DIALOG_CONTROL_INVALID);

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_cmd_hwnd_query->h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    p_dialog_cmd_hwnd_query->hwnd = p_dialog->hwnd;

    if(0 != (dialog_control_id = p_dialog_cmd_hwnd_query->dialog_control_id))
    {
        if(NULL == (p_dialog_ictl = p_dialog_ictl_from_control_id_in(&p_dialog->ictls, dialog_control_id, &p_parent_ictls)))
            return(create_error(DIALOG_ERR_UNKNOWN_DIALOG_CONTROL));

        /* otherwise WHAT? */
        UNREFERENCED_PARAMETER(p_dialog_ictl);
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* DIALOG_CMD_CODE_NULL_EVENT
*
* a dialog has got a null event
*
******************************************************************************/

_Check_return_
static STATUS
dialog_cmd_null_event(
    P_NULL_EVENT_BLOCK p_null_event_block)
{
    const H_DIALOG h_dialog = (H_DIALOG) (intptr_t) p_null_event_block->client_handle;
    P_DIALOG p_dialog;

    if(NULL == (p_dialog = p_dialog_from_h_dialog(h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

#if RISCOS
    dialog_riscos_null_event(p_dialog);
#endif

    return(STATUS_OK);
}

static void
dialog_ictl_enable_suppress_clear_changes_in(
    P_DIALOG_ICTL_GROUP p_ictl_group)
{
    const ARRAY_INDEX n_ictls = n_ictls_from_group(p_ictl_group);
    ARRAY_INDEX i;
    P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictls_from_group(p_ictl_group, n_ictls);

    for(i = 0; i < n_ictls; ++i, ++p_dialog_ictl)
    {
        p_dialog_ictl->bits.enable_suppress_change = 0;

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            dialog_ictl_enable_suppress_clear_changes_in(&p_dialog_ictl->data.groupbox.ictls);
            break;
        }
    }
}

/******************************************************************************
*
* propagate a suppression state down the tree
*
******************************************************************************/

static UINT
dialog_ictl_enable_suppress_in(
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     BOOL suppress)
{
    ARRAY_INDEX i;
    UINT nsc = 0;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        if(p_dialog_ictl->bits.enable_suppressed != UBF_PACK(suppress))
        {
            p_dialog_ictl->bits.enable_suppressed = UBF_PACK(suppress);
            p_dialog_ictl->bits.enable_suppress_change = 1;
            ++nsc;
        }

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            /* recurse into subgroups */
            nsc += dialog_ictl_enable_suppress_in(&p_dialog_ictl->data.groupbox.ictls, suppress);
            break;
        }
    }

    return(nsc);
}

/******************************************************************************
*
* reflect the enabled state on a control
*
******************************************************************************/

static void
dialog_ictl_enable_in_tree(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    UINT nsc = 0;

    switch(p_dialog_ictl->dialog_control_type)
    {
    default:
        break;

    case DIALOG_CONTROL_GROUPBOX:
        /* have to make this pass before repr is updated to mark suppression change flags in children */
        {  /* this group may itself be suppressed from a higher level */
        BOOL suppress = !p_dialog_ictl->bits.enabled || p_dialog_ictl->bits.enable_suppressed;
        nsc = dialog_ictl_enable_suppress_in(&p_dialog_ictl->data.groupbox.ictls, suppress);
        break;
        }
    }

#if RISCOS
    if(nsc)
        dialog_riscos_big_mods(p_dialog, 1);
#endif

    dialog_ictl_enable_here(p_dialog, p_dialog_ictl);

    if(nsc)
    {
        /* enabled state of group has suppressed enabled state of its children */
        assert(p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_GROUPBOX);
        dialog_ictl_enable_suppressed_changes_in(p_dialog, &p_dialog_ictl->data.groupbox.ictls);
    }

#if RISCOS
    if(nsc)
        dialog_riscos_big_mods(p_dialog, 0);
#endif

    if(nsc)
    {
        assert(p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_GROUPBOX);
        dialog_ictl_enable_suppress_clear_changes_in(&p_dialog_ictl->data.groupbox.ictls);
    }
}

/******************************************************************************
*
* reflect the enabled state on a control
*
******************************************************************************/

static void
dialog_ictl_enable_here(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    p_dialog_ictl->bits.disabled = !p_dialog_ictl->bits.enabled || p_dialog_ictl->bits.enable_suppressed;

#if RISCOS
    dialog_riscos_ictl_enable_here(p_dialog, p_dialog_ictl);
#elif WINDOWS
    dialog_windows_ictl_enable_here(p_dialog, p_dialog_ictl);
#endif

    if(p_dialog_ictl->bits.disabled)
    {
        if(p_dialog_ictl->dialog_control_id == p_dialog->current_dialog_control_id)
        {
            /* remove focus from control */
            DIALOG_CMD_CTL_FOCUS_SET dialog_cmd_ctl_focus_set;
            msgclr(dialog_cmd_ctl_focus_set);
            dialog_cmd_ctl_focus_set.h_dialog = p_dialog->h_dialog;
            dialog_cmd_ctl_focus_set.dialog_control_id = 0;
            status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_FOCUS_SET, &dialog_cmd_ctl_focus_set));
        }
    }
}

static void
dialog_ictl_enable_suppressed_changes_in(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_ictl_group)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        if(p_dialog_ictl->bits.enable_suppress_change)
        {
            /* if we have suppressed/unsuppressed something that was/should be enabled
             * then we must reflect the change else leave disabled
            */
            if(p_dialog_ictl->bits.enabled)
                dialog_ictl_enable_here(p_dialog, p_dialog_ictl);
        }

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            /* recurse into subgroups */
            dialog_ictl_enable_suppressed_changes_in(p_dialog, &p_dialog_ictl->data.groupbox.ictls);
            break;
        }
    }
}

/******************************************************************************
*
* reflect the encoded state on a control
*
******************************************************************************/

_Check_return_
static STATUS
dialog_ictl_edit_xx_encode_here_SetText(
    _InoutRef_  P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    _InRef_     PC_UI_TEXT p_ui_text)
{
    STATUS status = STATUS_OK;

#if RISCOS

#if defined(EDIT_XX_SINGLE_LINE_WIMP) || defined(EDIT_XX_SINGLE_LINE_WIMP_RO)
    if(0 != p_dialog_ictl_edit_xx->riscos.slec_buffer_size)
    {
        const P_DIALOG_WIMP_I p_i = &p_dialog_ictl->riscos.dwi[0];
        /*WimpIconBlockWithBitset icon;*/

/* >>> More work on moving carets - see PipeDream! */

        ui_text_copy_as_sbstr_buf(p_dialog_ictl_edit_xx->riscos.slec_buffer, p_dialog_ictl_edit_xx->riscos.slec_buffer_size, p_ui_text);

        /*status_assert(dialog_riscos_icon_recreate_prepare(p_dialog, &icon, p_i));*/
        /*dialog_riscos_icon_text_setup(&icon, p_dialog_ictl_edit_xx->riscos.slec_buffer);*/
        /*status_assert(dialog_riscos_icon_recreate(p_dialog, &icon, p_i));*/
#if 1
    {
    WimpSetIconStateBlock set_icon_state_block;
    set_icon_state_block.window_handle = p_dialog->hwnd;
    set_icon_state_block.icon_handle = p_i->icon_handle;
    set_icon_state_block.EOR_word = 0; /* poke it for redraw */
    set_icon_state_block.clear_word = 0;
    (void) (wimp_set_icon_state(&set_icon_state_block));
    } /*block*/
#else
        dialog_riscos_icon_redraw_for_encode(p_dialog, p_i, FRAMED_BOX_EDIT, 0);
#endif
    }
#endif

    if(NULL != p_dialog_ictl_edit_xx->riscos.mlec)
        status_assert(status = mlec_SetText(p_dialog_ictl_edit_xx->riscos.mlec, ui_text_ustr(p_ui_text)));

    UNREFERENCED_PARAMETER_InRef_(p_dialog);

#elif WINDOWS

    BOOL combo_type = FALSE;

    switch(p_dialog_ictl->dialog_control_type)
    {
    default:
        break;

    case DIALOG_CONTROL_COMBO_S32:
    case DIALOG_CONTROL_COMBO_TEXT:
        combo_type = TRUE;
        break;
    }

    if(HOST_WND_NONE != p_dialog->hwnd)
    {
        const HWND hwnd = GetDlgItem(p_dialog->hwnd, p_dialog_ictl->windows.wid);
        const PCTSTR tstr = ui_text_tstr(p_ui_text);

        if(combo_type)
        {
            ComboBox_SetText(hwnd, tstr);
        }
        else
        {
            if( p_dialog_ictl_edit_xx->multiline && (NULL != tstrchr(tstr, LF)) )
            {
                PCTSTR tstr_here = tstr;
                PCTSTR tstr_test;
                QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 32);
                quick_tblock_with_buffer_setup(quick_tblock);
                while(NULL != (tstr_test = tstrchr(tstr_here, LF)))
                {
                    static const TCHAR cr_lf[]  = { CR, LF };
                    status_break(status = quick_tblock_tchars_add(&quick_tblock, tstr_here, PtrDiffElemU32(tstr_test, tstr_here)));
                    status_break(status = quick_tblock_tchars_add(&quick_tblock, cr_lf, elemof32(cr_lf)));
                    tstr_here = tstr_test + 1; /* point past the LF */
                }
                if(status_ok(status))
                if(status_ok(status = quick_tblock_tstr_add_n(&quick_tblock, tstr_here, strlen_with_NULLCH)))
                    Edit_SetText(hwnd, quick_tblock_tstr(&quick_tblock));
                quick_tblock_dispose(&quick_tblock);
            }
            else
            {
                Edit_SetText(hwnd, tstr);
            }
        }
    }

#endif /* OS */

    return(status);
}

_Check_return_
static STATUS
dialog_ictl_edit_xx_encode_here(
    _InoutRef_  P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx)
{
    STATUS status = STATUS_OK;
    const void /*UI_CONTROL*/ * p_ui_control = NULL;
    UI_DATA_TYPE ui_data_type = UI_DATA_TYPE_NONE;
    UI_DATA ui_data;
    UI_TEXT ui_text;
    BOOL kill_ui_text = FALSE;

    /* obtain the text to be set into the control */
    switch(p_dialog_ictl->dialog_control_type)
    {
    default: default_unhandled();
#if CHECKING
    case DIALOG_CONTROL_EDIT:
#endif
        ui_text = p_dialog_ictl->state.edit.ui_text;
        break;

    case DIALOG_CONTROL_BUMP_S32:
        p_ui_control = p_dialog_ictl->p_dialog_control_data.bump_s32->bump_xx.p_uic;
        ui_data.s32 = p_dialog_ictl->state.bump_s32;
        ui_data_type = UI_DATA_TYPE_S32;
        break;

    case DIALOG_CONTROL_BUMP_F64:
        p_ui_control = p_dialog_ictl->p_dialog_control_data.bump_f64->bump_xx.p_uic;
        ui_data.f64 = p_dialog_ictl->state.bump_f64;
        ui_data_type = UI_DATA_TYPE_F64;
        break;

    case DIALOG_CONTROL_COMBO_S32:
        p_ui_control= p_dialog_ictl->data.combo_xx.list_xx.p_ui_control_s32;
        ui_data.s32 = p_dialog_ictl->state.combo_s32.s32;
        ui_data_type = UI_DATA_TYPE_S32;
        break;

    case DIALOG_CONTROL_COMBO_TEXT:
        ui_text = p_dialog_ictl->state.combo_text.ui_text;
        break;
    }

    if(ui_data_type != UI_DATA_TYPE_NONE)
    {
        UI_SOURCE ui_source;
        ui_source.type = UI_SOURCE_TYPE_SINGLE;
        ui_source.source.single_p_ui_data = &ui_data;
        kill_ui_text = 1;
        status_assert(status = ui_data_query_as_text(ui_data_type, &ui_source, 0 /*itemno*/, p_ui_control, &ui_text));
        status_return(status);
    }

    /* set the text into the control, with guards */
    p_dialog_ictl->bits.in_update += 1;
    dialog_ictl_edit_xx_encode_here_SetText(p_dialog, p_dialog_ictl, p_dialog_ictl_edit_xx, &ui_text);
    p_dialog_ictl->bits.in_update -= 1;

    if(kill_ui_text)
        ui_text_dispose(&ui_text);

    status_return(status);

    return(STATUS_OK);
}

#if RISCOS

static inline void
dialog_riscos_icon_change_bitmap(
    _InRef_     PC_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_WIMP_I p_i,
    _InVal_     RESOURCE_BITMAP_HANDLE resource_bitmap_handle)
{
    WimpIconBlockWithBitset icon;
    status_assert(dialog_riscos_icon_recreate_prepare(p_dialog, &icon, p_i));
    status_assert(dialog_riscos_icon_recreate_with(p_dialog, &icon, p_i, resource_bitmap_handle));
}

static inline void
dialog_riscos_icon_set_text_from_caption(
    _InRef_     P_DIALOG p_dialog,
    _InRef_     PC_DIALOG_ICTL p_dialog_ictl,
    _InVal_     S32 bits,
    _InoutRef_  P_DIALOG_WIMP_I p_i,
    _InVal_     FRAMED_BOX_STYLE b)
{
    WimpIconBlockWithBitset icon;
    status_assert(dialog_riscos_icon_recreate_prepare(p_dialog, &icon, p_i));
    dialog_riscos_icon_text_setup(&icon, p_dialog_ictl->riscos.caption);
    status_assert(dialog_riscos_icon_recreate(p_dialog, &icon, p_i));
    dialog_riscos_icon_redraw_for_encode(p_dialog, p_i, b, bits);
}

#endif /* OS */

_Check_return_
static STATUS
dialog_ictl_encode_here(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    _InVal_     S32 bits)
{
#if WINDOWS
    UNREFERENCED_PARAMETER_InVal_(bits);
#endif

    {
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);
    if(NULL != p_dialog_ictl_edit_xx)
    {
        if(p_dialog_ictl->bits.in_update && !p_dialog_ictl->bits.force_update)
            return(STATUS_OK);

        status_return(dialog_ictl_edit_xx_encode_here(p_dialog, p_dialog_ictl, p_dialog_ictl_edit_xx));
    }
    } /*block*/

    {
#if RISCOS
    const P_DIALOG_WIMP_I p_i = &p_dialog_ictl->riscos.dwi[0];
#elif WINDOWS
    const HWND hwnd = (p_dialog->hwnd && p_dialog_ictl->windows.wid) ? GetDlgItem(p_dialog->hwnd, p_dialog_ictl->windows.wid) : HOST_WND_NONE;
#endif

    switch(p_dialog_ictl->dialog_control_type)
    {
    default: default_unhandled();
#if CHECKING
        break;

    case DIALOG_CONTROL_GROUPBOX:
    case DIALOG_CONTROL_STATICPICTURE:
    case DIALOG_CONTROL_PUSHPICTURE:
#endif
        break;

#if RISCOS
    case DIALOG_CONTROL_STATICTEXT:
    case DIALOG_CONTROL_TEXTLABEL:
        {
        dialog_riscos_icon_set_text_from_caption(p_dialog, p_dialog_ictl, bits, p_i, FRAMED_BOX_NONE);
        break;
        }
#elif WINDOWS
    case DIALOG_CONTROL_STATICTEXT:
        {
        if(HOST_WND_NONE != hwnd)
            Static_SetText(hwnd, ui_text_tstr(&p_dialog_ictl->state.statictext));
        break;
        }

    case DIALOG_CONTROL_TEXTLABEL:
        {
        if(HOST_WND_NONE != hwnd)
        {
            if(p_dialog_ictl->p_dialog_control_data.textlabel->bits.windows_no_colon)
                Static_SetText(hwnd, ui_text_tstr(&p_dialog_ictl->state.textlabel));
            else
            {
                QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 128);
                quick_tblock_with_buffer_setup(quick_tblock);

                status_assert(ui_text_read(&quick_tblock, &p_dialog_ictl->state.textlabel));
                if(0 != quick_tblock_chars(&quick_tblock))
                    status_assert(quick_tblock_tchar_add(&quick_tblock, CH_COLON));
                status_assert(quick_tblock_nullch_add(&quick_tblock));
                Static_SetText(hwnd, quick_tblock_tstr(&quick_tblock));

                quick_tblock_dispose(&quick_tblock);
            }
        }
        break;
        }
#endif

    case DIALOG_CONTROL_TEXTFRAME:
        {
#if RISCOS
        dialog_riscos_icon_set_text_from_caption(p_dialog, p_dialog_ictl, bits, p_i, p_dialog_ictl->p_dialog_control_data.textframe->bits.border_style);
#elif WINDOWS
        if(HOST_WND_NONE != hwnd)
            Static_SetText(hwnd, ui_text_tstr(&p_dialog_ictl->state.textframe));
#endif
        break;
        }

    case DIALOG_CONTROL_PUSHBUTTON:
        {
#if RISCOS
        dialog_riscos_icon_set_text_from_caption(p_dialog, p_dialog_ictl, bits, p_i, FRAMED_BOX_NONE);
#elif WINDOWS
        if(HOST_WND_NONE != hwnd)
            Button_SetText(hwnd, ui_text_tstr(&p_dialog_ictl->state.pushbutton));
#endif
        break;
        }

    case DIALOG_CONTROL_RADIOBUTTON:
        {
#if RISCOS
        dialog_riscos_icon_change_bitmap(p_dialog, p_i,
            p_dialog_ictl->bits.radiobutton_active
                ? p_dialog->riscos.bitmap_radio_on.resource_bitmap_handle
                : p_dialog->riscos.bitmap_radio_off.resource_bitmap_handle);
#elif WINDOWS
        if(HOST_WND_NONE != hwnd)
            Button_SetCheck(hwnd, p_dialog_ictl->bits.radiobutton_active);
#endif
        break;
        }

    case DIALOG_CONTROL_RADIOPICTURE:
        {
#if RISCOS
        dialog_riscos_icon_change_bitmap(p_dialog, p_i,
            p_dialog_ictl->bits.radiobutton_active
                ? p_dialog_ictl->data.radiopicture.riscos.resource_bitmap_handle_on
                : p_dialog_ictl->data.radiopicture.riscos.resource_bitmap_handle_off);
#elif WINDOWS
        if(HOST_WND_NONE != hwnd)
            InvalidateRect(hwnd, NULL, FALSE);
#endif
        break;
        }

    case DIALOG_CONTROL_CHECKBOX:
        {
#if RISCOS
        RESOURCE_BITMAP_HANDLE resource_bitmap_handle;

        switch(p_dialog_ictl->state.checkbox)
        {
        default: default_unhandled();
#if CHECKING
        case DIALOG_BUTTONSTATE_OFF:
#endif
            resource_bitmap_handle = p_dialog->riscos.bitmap_check_off.resource_bitmap_handle;
            break;

        case DIALOG_BUTTONSTATE_ON:
            resource_bitmap_handle = p_dialog->riscos.bitmap_check_on.resource_bitmap_handle;
            break;
            }

        dialog_riscos_icon_change_bitmap(p_dialog, p_i, resource_bitmap_handle);
#elif WINDOWS
        if(HOST_WND_NONE != hwnd)
            Button_SetCheck(hwnd, (p_dialog_ictl->state.checkbox != DIALOG_BUTTONSTATE_OFF));
#endif
        break;
        }

    case DIALOG_CONTROL_CHECKPICTURE:
        {
#if RISCOS
        RESOURCE_BITMAP_HANDLE resource_bitmap_handle;

        switch(p_dialog_ictl->state.checkbox)
        {
        default: default_unhandled();
#if CHECKING
        case DIALOG_BUTTONSTATE_OFF:
#endif
            resource_bitmap_handle = p_dialog_ictl->data.checkpicture.riscos.resource_bitmap_handle_off;
            break;

        case DIALOG_BUTTONSTATE_ON:
            resource_bitmap_handle = p_dialog_ictl->data.checkpicture.riscos.resource_bitmap_handle_on;
            break;
            }

        dialog_riscos_icon_change_bitmap(p_dialog, p_i, resource_bitmap_handle);
#elif WINDOWS
        if(HOST_WND_NONE != hwnd)
            InvalidateRect(hwnd, NULL, FALSE);
#endif
        break;
    }

#ifdef DIALOG_HAS_TRISTATE
    case DIALOG_CONTROL_TRISTATE:
        {
#if RISCOS
        RESOURCE_BITMAP_HANDLE resource_bitmap_handle;

        switch(p_dialog_ictl->state.tristate)
        {
        default: default_unhandled();
#if CHECKING
        case DIALOG_TRISTATE_OFF:
#endif
            resource_bitmap_handle = p_dialog->riscos.bitmap_tristate_off.resource_bitmap_handle;
            break;

        case DIALOG_TRISTATE_ON:
            resource_bitmap_handle = p_dialog->riscos.bitmap_tristate_on.resource_bitmap_handle;
            break;

        case DIALOG_TRISTATE_DONT_CARE:
            resource_bitmap_handle = p_dialog->riscos.bitmap_tristate_dont_care.resource_bitmap_handle;
            break;
        }

        dialog_riscos_icon_change_bitmap(p_dialog, p_i, resource_bitmap_handle);
#elif WINDOWS
        if(HOST_WND_NONE != hwnd)
            Button_SetCheck(hwnd, p_dialog_ictl->state.tristate);
#endif
        break;
        }

    case DIALOG_CONTROL_TRIPICTURE:
        {
#if RISCOS
        RESOURCE_BITMAP_HANDLE resource_bitmap_handle;

        switch(p_dialog_ictl->state.tristate)
        {
        default: default_unhandled();
#if CHECKING
        case DIALOG_TRISTATE_OFF:
#endif
            resource_bitmap_handle = p_dialog_ictl->data.tripicture.riscos.resource_bitmap_handle_off;
            break;

        case DIALOG_TRISTATE_ON:
            resource_bitmap_handle = p_dialog_ictl->data.tripicture.riscos.resource_bitmap_handle_on;
            break;

        case DIALOG_TRISTATE_DONT_CARE:
            resource_bitmap_handle = p_dialog_ictl->data.tripicture.riscos.resource_bitmap_handle_dont_care;
            break;
        }

        dialog_riscos_icon_change_bitmap(p_dialog, p_i, resource_bitmap_handle);
#elif WINDOWS
        if(HOST_WND_NONE != hwnd)
            InvalidateRect(hwnd, NULL, FALSE);
#endif
        break;
    }
#endif

    case DIALOG_CONTROL_LIST_S32:
    case DIALOG_CONTROL_LIST_TEXT:
#if RISCOS
        if(p_dialog_ictl->data.list_xx.list_xx.riscos.lbox)
            (void) ri_lbox_selection_set_from_itemno(p_dialog_ictl->data.list_xx.list_xx.riscos.lbox,
                                                     (p_dialog_ictl->state.list_text.itemno >= 0)
                                                        ? p_dialog_ictl->state.list_text.itemno
                                                        : RI_LBOX_SELECTION_NONE);
#elif WINDOWS
        if(HOST_WND_NONE != hwnd)
            ListBox_SetCurSel(hwnd, (p_dialog_ictl->state.list_text.itemno >= 0) ? p_dialog_ictl->state.list_text.itemno : -1);
#endif
        break;

    case DIALOG_CONTROL_COMBO_S32:
    case DIALOG_CONTROL_COMBO_TEXT:
#if RISCOS
        if(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox)
            (void) ri_lbox_selection_set_from_itemno(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox,
                                                     (p_dialog_ictl->state.combo_text.itemno >= 0)
                                                        ? p_dialog_ictl->state.combo_text.itemno
                                                        : RI_LBOX_SELECTION_NONE);
#elif WINDOWS
        if(HOST_WND_NONE != hwnd)
            ComboBox_SetCurSel(hwnd, (p_dialog_ictl->state.combo_text.itemno >= 0) ? p_dialog_ictl->state.combo_text.itemno : -1);
#endif
        break;

    case DIALOG_CONTROL_EDIT:
       break;

    case DIALOG_CONTROL_BUMP_S32:
    case DIALOG_CONTROL_BUMP_F64:
        break;

    case DIALOG_CONTROL_USER:
#if RISCOS
        dialog_riscos_icon_redraw_for_encode(p_dialog, p_i, p_dialog_ictl->data.user.border_style, bits);
#elif WINDOWS
        if(HOST_WND_NONE != hwnd)
            InvalidateRect(hwnd, NULL, FALSE);
#endif
        break;
    }

    return(STATUS_OK);
    } /*block*/
}

/******************************************************************************
*
* run over a group of controls ensuring that they have enable state shown correctly
*
******************************************************************************/

_Check_return_
extern STATUS
dialog_ictls_enable_in(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_ictl_group)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        dialog_ictl_enable_here(p_dialog, p_dialog_ictl);

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            /* recurse into subgroups */
            status_return(dialog_ictls_enable_in(p_dialog, &p_dialog_ictl->data.groupbox.ictls));
            break;
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* run over a group of controls ensuring that they have encode state shown correctly
*
******************************************************************************/

_Check_return_
extern STATUS
dialog_ictls_encode_in(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     S32 bits)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        status_return(dialog_ictl_encode_here(p_dialog, p_dialog_ictl, bits));

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            /* recurse into subgroups */
            status_return(dialog_ictls_encode_in(p_dialog, &p_dialog_ictl->data.groupbox.ictls, bits));
            break;
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* dialog system initialisation
*
******************************************************************************/

_Check_return_
static STATUS
dialog_init(void)
{
    BIT_NUMBER bit_number;

    /* initialise class default validation acceptance bitmaps */

    /* U8 accepts 0-9 only */
    bitmap_clear(dialog_statics.bitmap_validation_u8, N_BITS_ARG(256));
    for(bit_number = CH_DIGIT_ZERO; bit_number <= CH_DIGIT_NINE; ++bit_number)
        bitmap_bit_set(dialog_statics.bitmap_validation_u8, bit_number, N_BITS_ARG(256));

    /* S32 based on U8, but accepts +- too */
    bitmap_copy(   dialog_statics.bitmap_validation_s32, dialog_statics.bitmap_validation_u8, N_BITS_ARG(256));
    bitmap_bit_set(dialog_statics.bitmap_validation_s32, CH_PLUS_SIGN, N_BITS_ARG(256));
    bitmap_bit_set(dialog_statics.bitmap_validation_s32, CH_MINUS_SIGN__BASIC, N_BITS_ARG(256));

    /* F64 based on S32, but accepts eE. too */
    bitmap_copy(   dialog_statics.bitmap_validation_f64, dialog_statics.bitmap_validation_s32, N_BITS_ARG(256));
    bitmap_bit_set(dialog_statics.bitmap_validation_f64, 'e', N_BITS_ARG(256));
    bitmap_bit_set(dialog_statics.bitmap_validation_f64, 'E', N_BITS_ARG(256));
    bitmap_bit_set(dialog_statics.bitmap_validation_f64, CH_FULL_STOP, N_BITS_ARG(256)); /* always accept dot for punters who know what the real world is all about */
    bitmap_bit_set(dialog_statics.bitmap_validation_f64, get_ss_recog_context_alt(decimal_point_char), N_BITS_ARG(256));

    /* general edit accepts all bar control chars */
    bitmap_set(dialog_statics.bitmap_validation_edit, N_BITS_ARG(256));
    for(bit_number = 0; bit_number <= 31; ++bit_number)
        bitmap_bit_clear(dialog_statics.bitmap_validation_edit, bit_number, N_BITS_ARG(256));

    /* multiline edit based on edit, but accepts line separators too */
    bitmap_copy(   dialog_statics.bitmap_validation_edit_multiline, dialog_statics.bitmap_validation_edit, N_BITS_ARG(256));
    bitmap_bit_set(dialog_statics.bitmap_validation_edit_multiline, 10, N_BITS_ARG(256));

    /* initialise handle generators and structure allocators */
    dialog_statics.handle_gen = 0x29000000;

    array_init_block_setup(&dialog_statics.ictls_init_block, 8, sizeof32(DIALOG_ICTL), 1);

#if WINDOWS
    dialog_windows_ui_len_init(); /* ensure we know all about which font to use etc. */
#endif

    return(STATUS_OK);
}

static void
dialog_exit(void)
{
#if WINDOWS
    if(dialog_statics.windows.hfont)
    {
        DeleteFont(dialog_statics.windows.hfont);
        dialog_statics.windows.hfont = NULL;
    }
#endif
    al_array_dispose(&dialog_statics.handles);
}

#define DIALOG_MESSAGE_CMD_OFFSET(dialog_message) ( \
    ((U32) (dialog_message) - (U32) DIALOG_MESSAGE_STT) )

_Check_return_
extern STATUS
dialog_event(
    _InVal_     DIALOG_MESSAGE dialog_message,
    P_ANY p_data)
{
    switch(DIALOG_MESSAGE_CMD_OFFSET(dialog_message))
    {
    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_COMPLETE_DBOX):
        return(dialog_cmd_complete_dbox((P_DIALOG_CMD_COMPLETE_DBOX) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_DISPOSE_DBOX):
        return(dialog_cmd_dispose_dbox((P_DIALOG_CMD_DISPOSE_DBOX) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_NOTE_POSITION_TRASH):
        return(dialog_cmd_note_position_trash());

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_HWND_QUERY):
        return(dialog_cmd_hwnd_query((P_DIALOG_CMD_HWND_QUERY) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_NULL_EVENT):
        return(dialog_cmd_null_event((P_NULL_EVENT_BLOCK) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_DEFPUSHBUTTON):
        return(dialog_cmd_defpushbutton((P_DIALOG_CMD_DEFPUSHBUTTON) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_ENABLE):
        return(dialog_cmd_ctl_enable((P_DIALOG_CMD_CTL_ENABLE) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_ENABLE_QUERY):
        return(dialog_cmd_ctl_enable_query((P_DIALOG_CMD_CTL_ENABLE_QUERY) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_ENCODE):
        return(dialog_cmd_ctl_encode((P_DIALOG_CMD_CTL_ENCODE) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_NOBBLE):
        return(dialog_cmd_ctl_nobble((P_DIALOG_CMD_CTL_NOBBLE) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_UI_CONTROL):
        return(dialog_cmd_ctl_ui_control((P_DIALOG_CMD_CTL_UI_CONTROL) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_SET_DEFAULT):
        return(dialog_cmd_ctl_set_default((P_DIALOG_CMD_CTL_SET_DEFAULT) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_FOCUS_SET):
        return(dialog_cmd_ctl_focus_set((P_DIALOG_CMD_CTL_FOCUS_SET) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_SIZE_ESTIMATE):
        return(dialog_cmd_ctl_size_estimate((P_DIALOG_CMD_CTL_SIZE_ESTIMATE) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_STATE_QUERY):
        return(dialog_cmd_ctl_state_query((P_DIALOG_CMD_CTL_STATE_QUERY) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_STATE_QUERY_DISPOSE):
        return(dialog_cmd_ctl_state_query_dispose((P_DIALOG_CMD_CTL_STATE_QUERY) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_STATE_SET):
        return(dialog_cmd_ctl_state_set((P_DIALOG_CMD_CTL_STATE_SET) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_NEW_SOURCE):
        return(dialog_cmd_ctl_new_source((P_DIALOG_CMD_CTL_NEW_SOURCE) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_SIZE_QUERY):
        return(dialog_cmd_ctl_size_query((P_DIALOG_CMD_CTL_SIZE_QUERY) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_SIZE_SET):
        return(dialog_cmd_ctl_size_set((P_DIALOG_CMD_CTL_SIZE_SET) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_POSN_QUERY):
        return(dialog_cmd_ctl_posn_query((P_DIALOG_CMD_CTL_POSN_QUERY) p_data));

    case DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_CTL_PARENT_QUERY):
        return(dialog_cmd_ctl_parent_query((P_DIALOG_CMD_CTL_PARENT_QUERY) p_data));

    default:
#if RISCOS
        if(((U32) dialog_message - (U32) DIALOG_RISCOS_EVENT_CODE_STT) < ((U32) DIALOG_MESSAGE_END - (U32) DIALOG_RISCOS_EVENT_CODE_STT))
            return(dialog_riscos_event(dialog_message, p_data));
#endif /* RISCOS */
        default_unhandled();
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, dialog_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP_SERVICES:
        status_return(resource_init(OBJECT_ID_DIALOG, P_BOUND_MESSAGES_OBJECT_ID_DIALOG, P_BOUND_RESOURCES_OBJECT_ID_DIALOG));
        return(dialog_init());

    case T5_MSG_IC__SERVICES_EXIT1:
        dialog_exit();
        return(resource_close(OBJECT_ID_DIALOG));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_dialog);
OBJECT_PROTO(extern, object_dialog)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(dialog_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        if(((U32) t5_message - (U32) DIALOG_MESSAGE_STT) >= ((U32) DIALOG_MESSAGE_END - (U32) DIALOG_MESSAGE_STT))
            return(STATUS_OK);

        if(DIALOG_MESSAGE_CMD_OFFSET(DIALOG_CMD_CODE_PROCESS_DBOX) == DIALOG_MESSAGE_CMD_OFFSET(t5_message)) /* this one needs p_docu */
            return(dialog_dbox_process(p_docu, t5_message, (P_DIALOG_CMD_PROCESS_DBOX) p_data));

        return(dialog_event((const DIALOG_MESSAGE) t5_message, p_data));
    }
}

/* end of ob_dlg.c */
