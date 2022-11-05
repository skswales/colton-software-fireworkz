/* gr_uiaxi.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Axis editing UI for Fireworkz */

/* SKS June 1993 */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

static /*poked*/ UI_CONTROL_F64
val_axis_major_control = { 0.0, 1.0E307 };

static /*poked*/ UI_CONTROL_F64
val_axis_major_control_under_log_labels = { 0.0, 1.0E307, 10.0 };

static /*poked*/ UI_CONTROL_F64
val_axis_minor_control = { 0.0, 1.0E307 };

static /*poked*/ UI_CONTROL_F64
val_axis_minor_control_under_log_labels = { 0.0, 1.0E307, 10.0 };

/******************************************************************************
*
* edit selected axis attributes
*
******************************************************************************/

_Check_return_
static STATUS
gr_chart_axis_process_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    STATUS status = STATUS_OK;

    /* load current settings into structure */
    const P_GR_CHART_AXIS_STATE p_state = (P_GR_CHART_AXIS_STATE) p_dialog_msg_process_start->client_handle;
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_state->p_chart_header->ch);
    const P_GR_AXES p_axes = &cp->axes[p_state->modifying_axes_idx];
    const P_GR_AXIS p_axis = &p_axes->axis[p_state->modifying_axis_idx];

    p_state->axis.lzr = p_axis->bits.lzr;
    p_state->axis.arf = p_axis->bits.arf;

    p_state->major.automatic = !p_axis->major.bits.manual;
    p_state->major.grid      =  p_axis->major.bits.grid;
    p_state->major.tick      =  p_axis->major.bits.tick;

    p_state->minor.automatic = !p_axis->minor.bits.manual;
    p_state->minor.grid      =  p_axis->minor.bits.grid;
    p_state->minor.tick      =  p_axis->minor.bits.tick;

    {
    DIALOG_CMD_CTL_UI_CONTROL dialog_cmd_ctl_ui_control;
    msgclr(dialog_cmd_ctl_ui_control);

    dialog_cmd_ctl_ui_control.h_dialog = p_dialog_msg_process_start->h_dialog;

    if(p_state->processing_cat)
    {
        p_state->major.cat_value = (S32) (p_axis->major.bits.manual ? p_axis->major.punter : p_axis->major.current);
        p_state->minor.cat_value = (S32) (p_axis->minor.bits.manual ? p_axis->minor.punter : p_axis->minor.current);

        dialog_cmd_ctl_ui_control.dialog_control_id = GEN_AXIS_ID_MAJOR_SPACING;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_BUMP;
        dialog_cmd_ctl_ui_control.data.s32 = (S32) gr_lin_major(p_state->major.cat_value);
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control));

        dialog_cmd_ctl_ui_control.dialog_control_id = GEN_AXIS_ID_MINOR_SPACING;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_BUMP;
        dialog_cmd_ctl_ui_control.data.s32 = (S32) gr_lin_major(p_state->minor.cat_value);
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control));

        dialog_cmd_ctl_ui_control.dialog_control_id = GEN_AXIS_ID_MAJOR_SPACING;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_MAX;
        dialog_cmd_ctl_ui_control.data.s32 = (S32) p_state->major.cat_value;
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control));
    }
    else
    {
        p_state->val_axis.automatic = !p_axis->bits.manual;
        p_state->val_axis.minimum = p_axis->bits.manual ? p_axis->punter.min : p_axis->current.min;
        p_state->val_axis.maximum = p_axis->bits.manual ? p_axis->punter.max : p_axis->current.max;
        p_state->val_axis.include_zero = p_axis->bits.incl_zero;
        p_state->val_axis.logarithmic = p_axis->bits.log_scale;
        p_state->val_axis.logarithmic_modified = 0;
        p_state->val_axis.log_labels = p_axis->bits.log_label;

        p_state->major.val_value = p_axis->major.bits.manual ? p_axis->major.punter : p_axis->major.current;
        p_state->minor.val_value = p_axis->minor.bits.manual ? p_axis->minor.punter : p_axis->minor.current;

        dialog_cmd_ctl_ui_control.dialog_control_id = VAL_AXIS_ID_SCALING_MAXIMUM;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_BUMP;
        dialog_cmd_ctl_ui_control.data.f64 = p_state->major.val_value;
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control));

        dialog_cmd_ctl_ui_control.dialog_control_id = VAL_AXIS_ID_SCALING_MINIMUM;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_BUMP;
        dialog_cmd_ctl_ui_control.data.f64 = p_state->major.val_value;
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control));

        if(p_state->val_axis.maximum == p_state->val_axis.minimum)
            p_state->val_axis.maximum += p_state->major.val_value;

        dialog_cmd_ctl_ui_control.dialog_control_id = GEN_AXIS_ID_MAJOR_SPACING;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_UIC;
        dialog_cmd_ctl_ui_control.data.p_ui_control_f64 = p_state->val_axis.log_labels
                                                        ? &val_axis_major_control_under_log_labels
                                                        : &val_axis_major_control;
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control));

        dialog_cmd_ctl_ui_control.dialog_control_id = GEN_AXIS_ID_MAJOR_SPACING;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_BUMP;
        dialog_cmd_ctl_ui_control.data.f64 = p_state->minor.val_value;
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control));

        dialog_cmd_ctl_ui_control.dialog_control_id = GEN_AXIS_ID_MINOR_SPACING;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_UIC;
        dialog_cmd_ctl_ui_control.data.p_ui_control_f64 = p_state->val_axis.log_labels
                                                        ? &val_axis_minor_control_under_log_labels
                                                        : &val_axis_minor_control;
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control));

        dialog_cmd_ctl_ui_control.dialog_control_id = GEN_AXIS_ID_MINOR_SPACING;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_BUMP;
        dialog_cmd_ctl_ui_control.data.f64 = gr_lin_major(2.0 * p_state->minor.val_value);
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control));

        dialog_cmd_ctl_ui_control.dialog_control_id = GEN_AXIS_ID_MINOR_SPACING;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_MAX;
        dialog_cmd_ctl_ui_control.data.f64 = p_state->major.val_value;
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control));

        * (int *) &p_state->val_series = 0;
        p_state->val_series.cumulative    = p_axes->bits.cumulative;
        p_state->val_series.cumulative_modified = 0;
        p_state->val_series.vary_by_point = p_axes->bits.vary_by_point;
        p_state->val_series.best_fit      = p_axes->bits.best_fit;
        p_state->val_series.fill_to_axis  = p_axes->bits.fill_to_axis;
        p_state->val_series.stacked       = p_axes->bits.stacked;
        p_state->val_series.stacked_modified = 0;
    }
    } /*block*/

    p_state->level += 1;

    if(status_ok(status))
    if(status_ok(status = ui_dlg_set_radio(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_POSITION_LZR_GROUP, p_state->axis.lzr)))
                 status = ui_dlg_set_radio(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_POSITION_ARF_GROUP, p_state->axis.arf);

    /*ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_POSITION_ARF_GROUP, p_state->cp->d3.bits.use);*/

    if(status_ok(status))
    {
        if(p_state->processing_cat)
            status = ui_dlg_set_s32(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_MAJOR_SPACING, p_state->major.cat_value);
        else
            status = ui_dlg_set_f64(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_MAJOR_SPACING, p_state->major.val_value);
    }

    if(status_ok(status))
    if(status_ok(status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_MAJOR_AUTO, p_state->major.automatic)))
    if(status_ok(status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_MAJOR_GRID, p_state->major.grid)))
                 status = ui_dlg_set_radio(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_MAJOR_GROUP, p_state->major.tick);

    if(status_ok(status))
    {
        if(p_state->processing_cat)
            status = ui_dlg_set_s32(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_MINOR_SPACING, p_state->minor.cat_value);
        else
            status = ui_dlg_set_f64(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_MINOR_SPACING, p_state->minor.val_value);
    }

    if(status_ok(status))
    if(status_ok(status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_MINOR_AUTO, p_state->minor.automatic)))
    if(status_ok(status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_MINOR_GRID, p_state->minor.grid)))
                 status = ui_dlg_set_radio(p_dialog_msg_process_start->h_dialog, GEN_AXIS_ID_MINOR_GROUP, p_state->minor.tick);

    if(status_ok(status))
    {
        if(!p_state->processing_cat)
        {
            /* scale */
            if(status_ok(status = ui_dlg_set_f64(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SCALING_MINIMUM, p_state->val_axis.minimum)))
            if(status_ok(status = ui_dlg_set_f64(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SCALING_MAXIMUM, p_state->val_axis.maximum)))
            if(status_ok(status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SCALING_AUTO, p_state->val_axis.automatic)))
            if(status_ok(status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SCALING_INCLUDE_ZERO, p_state->val_axis.include_zero)))
            if(status_ok(status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SCALING_LOGARITHMIC, p_state->val_axis.logarithmic)))
                         status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SCALING_LOG_LABELS, p_state->val_axis.log_labels);

            /* series defaults */
            if(status_ok(status))
            if(status_ok(status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SERIES_CUMULATIVE, p_state->val_series.cumulative)))
            if(status_ok(status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SERIES_VARY_BY_POINT, p_state->val_series.vary_by_point)))
            if(status_ok(status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SERIES_BEST_FIT, p_state->val_series.best_fit)))
                         status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SERIES_FILL_TO_AXIS, p_state->val_series.fill_to_axis);

            /* series immediate */
            if(status_ok(status))
                status = ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SERIES_STACK, p_state->val_series.stacked);
        }
    }

    p_state->level -= 1;

    return(status);
}

_Check_return_
static STATUS
gr_chart_axis_process_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    STATUS status = STATUS_OK;

    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        /* set chart from modified structure */
        const P_GR_CHART_AXIS_STATE p_state = (P_GR_CHART_AXIS_STATE) p_dialog_msg_process_end->client_handle;
        const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_state->p_chart_header->ch);
        const P_GR_AXES p_axes = &cp->axes[p_state->modifying_axes_idx];
        const P_GR_AXIS p_axis = &p_axes->axis[p_state->modifying_axis_idx];

        p_axis->bits.lzr = p_state->axis.lzr;
        p_axis->bits.arf = p_state->axis.arf;

        p_axis->major.bits.manual = !p_state->major.automatic;
        p_axis->major.bits.tick   =  p_state->major.tick;
        p_axis->major.bits.grid   =  p_state->major.grid;

        p_axis->minor.bits.manual = !p_state->minor.automatic;
        p_axis->minor.bits.tick   =  p_state->minor.tick;
        p_axis->minor.bits.grid   =  p_state->minor.grid;

        if(p_axis->major.bits.manual)
            p_axis->major.punter = p_state->processing_cat ? (F64) p_state->major.cat_value : p_state->major.val_value;
        if(p_axis->minor.bits.manual)
            p_axis->minor.punter = p_state->processing_cat ? (F64) p_state->minor.cat_value : p_state->minor.val_value;

        if(!p_state->processing_cat)
        {
            /* set chart from modified structure */
            GR_SERIES_IDX series_idx;

            p_axis->bits.manual = !p_state->val_axis.automatic;
            if(p_axis->bits.manual)
            {
                p_axis->punter.min = MIN(p_state->val_axis.minimum, p_state->val_axis.maximum);
                p_axis->punter.max = MAX(p_state->val_axis.minimum, p_state->val_axis.maximum);
            }
            p_axis->bits.incl_zero = p_state->val_axis.include_zero;
            p_axis->bits.log_scale = p_state->val_axis.logarithmic;
            p_axis->bits.log_label = p_state->val_axis.log_labels;

            p_axes->bits.cumulative    = p_state->val_series.cumulative;
            p_axes->bits.vary_by_point = p_state->val_series.vary_by_point;
            p_axes->bits.best_fit      = p_state->val_series.best_fit;
            p_axes->bits.fill_to_axis  = p_state->val_series.fill_to_axis;
            p_axes->bits.stacked       = p_state->val_series.stacked;

            /* reflect into series on these axes */
            for(series_idx = p_axes->series.stt_idx; series_idx < p_axes->series.end_idx; ++series_idx)
            {
                const P_GR_SERIES serp = getserp(cp, series_idx);

                if( p_state->val_series.stacked_modified     ||
                    p_state->val_series.cumulative_modified  ||
                    p_state->val_axis.logarithmic_modified     )
                    * (int *) &serp->valid = 0;
            }
        }

        chart_modify_docu(p_state->p_chart_header);
    }

    return(status);
}

_Check_return_
static STATUS
gr_chart_axis_process_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    STATUS status = STATUS_OK;
    P_GR_CHART_AXIS_STATE p_state = (P_GR_CHART_AXIS_STATE) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case GEN_AXIS_ID_POSITION_LZR_GROUP:
        p_state->axis.lzr = UBF_PACK(p_dialog_msg_ctl_state_change->new_state.radiobutton);
        break;

    case GEN_AXIS_ID_POSITION_ARF_GROUP:
        p_state->axis.arf = UBF_PACK(p_dialog_msg_ctl_state_change->new_state.radiobutton);
        break;

    case GEN_AXIS_ID_MAJOR_AUTO:
        p_state->major.automatic = p_dialog_msg_ctl_state_change->new_state.checkbox;

        if(p_state->major.automatic)
        {
            const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_state->p_chart_header->ch);
            const P_GR_AXES p_axes = &cp->axes[p_state->modifying_axes_idx];
            const P_GR_AXIS p_axis = &p_axes->axis[p_state->modifying_axis_idx];

            p_state->level += 1;
            if(p_state->processing_cat)
                status = ui_dlg_set_s32(p_dialog_msg_ctl_state_change->h_dialog, GEN_AXIS_ID_MAJOR_SPACING, (S32) p_axis->major.current);
            else
                status = ui_dlg_set_f64(p_dialog_msg_ctl_state_change->h_dialog, GEN_AXIS_ID_MAJOR_SPACING,       p_axis->major.current);
            p_state->level -= 1;
        }
        break;

    case GEN_AXIS_ID_MINOR_AUTO:
        p_state->minor.automatic = p_dialog_msg_ctl_state_change->new_state.checkbox;

        if(p_state->minor.automatic)
        {
            const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_state->p_chart_header->ch);
            const P_GR_AXES p_axes = &cp->axes[p_state->modifying_axes_idx];
            const P_GR_AXIS p_axis = &p_axes->axis[p_state->modifying_axis_idx];

            p_state->level += 1;
            if(p_state->processing_cat)
                status = ui_dlg_set_s32(p_dialog_msg_ctl_state_change->h_dialog, GEN_AXIS_ID_MINOR_SPACING, (S32) p_axis->minor.current);
            else
                status = ui_dlg_set_f64(p_dialog_msg_ctl_state_change->h_dialog, GEN_AXIS_ID_MINOR_SPACING,       p_axis->minor.current);
            p_state->level -= 1;
        }
        break;

    case GEN_AXIS_ID_MAJOR_SPACING:
        {
        DIALOG_CMD_CTL_UI_CONTROL dialog_cmd_ui_control;
        msgclr(dialog_cmd_ui_control);

        dialog_cmd_ui_control.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;

        if(p_state->processing_cat)
        {
            p_state->major.cat_value = p_dialog_msg_ctl_state_change->new_state.bump_s32;

            dialog_cmd_ui_control.dialog_control_id = GEN_AXIS_ID_MINOR_SPACING;
            dialog_cmd_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_MAX;
            dialog_cmd_ui_control.data.s32 = p_state->major.cat_value;
            status = object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ui_control);
        }
        else
        {
            p_state->major.val_value = p_dialog_msg_ctl_state_change->new_state.bump_f64;

            dialog_cmd_ui_control.dialog_control_id = GEN_AXIS_ID_MINOR_SPACING;
            dialog_cmd_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_MAX;
            dialog_cmd_ui_control.data.f64 = p_state->major.val_value;
            status = object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ui_control);

            dialog_cmd_ui_control.dialog_control_id = VAL_AXIS_ID_SCALING_MAXIMUM;
            dialog_cmd_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_BUMP;
            dialog_cmd_ui_control.data.f64 = p_state->major.val_value;
            status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ui_control));

            dialog_cmd_ui_control.dialog_control_id = VAL_AXIS_ID_SCALING_MINIMUM;
            dialog_cmd_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_BUMP;
            dialog_cmd_ui_control.data.f64 = p_state->major.val_value;
            status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ui_control));
        }

        if(!p_state->level)
            status_accumulate(status, ui_dlg_set_check(p_dialog_msg_ctl_state_change->h_dialog, GEN_AXIS_ID_MAJOR_AUTO, 0));

        break;
        }

    case GEN_AXIS_ID_MINOR_SPACING:
        if(p_state->processing_cat)
            p_state->minor.cat_value = p_dialog_msg_ctl_state_change->new_state.bump_s32;
        else
            p_state->minor.val_value = p_dialog_msg_ctl_state_change->new_state.bump_f64;

        if(!p_state->level)
            status = ui_dlg_set_check(p_dialog_msg_ctl_state_change->h_dialog, GEN_AXIS_ID_MINOR_AUTO, 0);
        break;

    case GEN_AXIS_ID_MAJOR_GRID:
        p_state->major.grid = p_dialog_msg_ctl_state_change->new_state.checkbox;
        break;

    case GEN_AXIS_ID_MINOR_GRID:
        p_state->minor.grid = p_dialog_msg_ctl_state_change->new_state.checkbox;
        break;

    case GEN_AXIS_ID_MAJOR_GROUP:
        p_state->major.tick = UBF_PACK(p_dialog_msg_ctl_state_change->new_state.radiobutton);
        break;

    case GEN_AXIS_ID_MINOR_GROUP:
        p_state->minor.tick = UBF_PACK(p_dialog_msg_ctl_state_change->new_state.radiobutton);
        break;

    case VAL_AXIS_ID_SCALING_AUTO:
        p_state->val_axis.automatic = p_dialog_msg_ctl_state_change->new_state.checkbox;

        if(p_state->val_axis.automatic)
        {
            const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_state->p_chart_header->ch);
            const P_GR_AXES p_axes = &cp->axes[p_state->modifying_axes_idx];
            const P_GR_AXIS p_axis = &p_axes->axis[p_state->modifying_axis_idx];

            p_state->level += 1;
            if(status_ok(status = ui_dlg_set_f64(p_dialog_msg_ctl_state_change->h_dialog, VAL_AXIS_ID_SCALING_MINIMUM, p_axis->current.min)))
                         status = ui_dlg_set_f64(p_dialog_msg_ctl_state_change->h_dialog, VAL_AXIS_ID_SCALING_MAXIMUM, p_axis->current.max);
            p_state->level -= 1;
        }
        break;

    case VAL_AXIS_ID_SCALING_MINIMUM:
    case VAL_AXIS_ID_SCALING_MAXIMUM:
        {
        F64 major;
        DIALOG_CMD_CTL_UI_CONTROL dialog_cmd_ctl_ui_control;
        msgclr(dialog_cmd_ctl_ui_control);

        * ((p_dialog_msg_ctl_state_change->dialog_control_id == VAL_AXIS_ID_SCALING_MINIMUM)
           ? &p_state->val_axis.minimum
           : &p_state->val_axis.maximum)
           = p_dialog_msg_ctl_state_change->new_state.bump_f64;

        major = gr_lin_major(p_state->val_axis.maximum - p_state->val_axis.minimum);

        dialog_cmd_ctl_ui_control.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
        dialog_cmd_ctl_ui_control.dialog_control_id = VAL_AXIS_ID_SCALING_MAXIMUM;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_BUMP;
        dialog_cmd_ctl_ui_control.data.f64 = major;
        status = object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control);

        dialog_cmd_ctl_ui_control.dialog_control_id = VAL_AXIS_ID_SCALING_MINIMUM;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_BUMP;
        dialog_cmd_ctl_ui_control.data.f64 = major;
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control));

        if(!p_state->level)
            status_accumulate(status, ui_dlg_set_check(p_dialog_msg_ctl_state_change->h_dialog, VAL_AXIS_ID_SCALING_AUTO, 0));

        break;
        }

    case VAL_AXIS_ID_SCALING_INCLUDE_ZERO:
        p_state->val_axis.include_zero = p_dialog_msg_ctl_state_change->new_state.checkbox;
        break;

    case VAL_AXIS_ID_SCALING_LOGARITHMIC:
        p_state->val_axis.logarithmic = p_dialog_msg_ctl_state_change->new_state.checkbox;
        p_state->val_axis.logarithmic_modified = 1;
        break;

    case VAL_AXIS_ID_SCALING_LOG_LABELS:
        {
        DIALOG_CMD_CTL_UI_CONTROL dialog_cmd_ctl_ui_control;
        msgclr(dialog_cmd_ctl_ui_control);

        p_state->val_axis.log_labels = p_dialog_msg_ctl_state_change->new_state.checkbox;

        dialog_cmd_ctl_ui_control.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
        dialog_cmd_ctl_ui_control.dialog_control_id = GEN_AXIS_ID_MAJOR_SPACING;
        dialog_cmd_ctl_ui_control.what = DIALOG_CMD_CTL_UI_CONTROL_UIC;
        dialog_cmd_ctl_ui_control.data.p_ui_control_f64 = p_state->val_axis.log_labels
                                                        ? &val_axis_major_control_under_log_labels
                                                        : &val_axis_major_control;
        status = object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control);

        dialog_cmd_ctl_ui_control.dialog_control_id = GEN_AXIS_ID_MINOR_SPACING;
        dialog_cmd_ctl_ui_control.data.p_ui_control_f64 = p_state->val_axis.log_labels
                                                        ? &val_axis_minor_control_under_log_labels
                                                        : &val_axis_minor_control;
        status_accumulate(status, object_call_DIALOG(DIALOG_CMD_CODE_CTL_UI_CONTROL, &dialog_cmd_ctl_ui_control));

        break;
        }

    case VAL_AXIS_ID_SERIES_CUMULATIVE:
        p_state->val_series.cumulative = p_dialog_msg_ctl_state_change->new_state.checkbox;
        p_state->val_series.cumulative_modified = 1;
        break;

    case VAL_AXIS_ID_SERIES_VARY_BY_POINT:
        p_state->val_series.vary_by_point = p_dialog_msg_ctl_state_change->new_state.checkbox;
        break;

    case VAL_AXIS_ID_SERIES_BEST_FIT:
        p_state->val_series.best_fit = p_dialog_msg_ctl_state_change->new_state.checkbox;
        break;

    case VAL_AXIS_ID_SERIES_FILL_TO_AXIS:
        p_state->val_series.fill_to_axis = p_dialog_msg_ctl_state_change->new_state.checkbox;
        break;

    case VAL_AXIS_ID_SERIES_STACK:
        p_state->val_series.stacked = p_dialog_msg_ctl_state_change->new_state.checkbox;
        p_state->val_series.stacked_modified = 1;
        break;

    default:
        break;
    }

    return(status);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_gr_chart_axis_process)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PROCESS_START:
        return(gr_chart_axis_process_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(gr_chart_axis_process_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(gr_chart_axis_process_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    default:
        return(STATUS_OK);
    }
}

static inline void
gr_chart_axis_process_init(
    _InoutRef_  P_CHART_HEADER p_chart_header,
    _In_        GR_CHART_OBJID id,
    _OutRef_    P_GR_CHART_AXIS_STATE p_state)
{
    p_state->p_chart_header = p_chart_header;
    p_state->processing_cat = 0;
    p_state->level = 0;

    {
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_header->ch);

    switch(id.name)
    {
    case GR_CHART_OBJNAME_AXIS:
        break;

    case GR_CHART_OBJNAME_AXISGRID:
    case GR_CHART_OBJNAME_AXISTICK:
        id.name = GR_CHART_OBJNAME_AXIS;
        id.has_subno = 0; /* clear the crud */
        id.subno = 0;
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_DROPSERIES:
    case GR_CHART_OBJNAME_BESTFITSER:
    case GR_CHART_OBJNAME_POINT:
    case GR_CHART_OBJNAME_DROPPOINT:
        {
        GR_SERIES_IDX series_idx = gr_series_idx_from_external(cp, id.no);
        GR_AXES_IDX axes_idx = gr_axes_idx_from_series_idx(cp, series_idx);

        gr_chart_objid_clear(&id);
        id.name = GR_CHART_OBJNAME_AXIS;
        id.no = UBF_PACK(gr_axes_external_from_idx(cp, axes_idx, Y_AXIS_IDX));

        break;
        }

    default:
        gr_chart_objid_clear(&id);
        id.name = GR_CHART_OBJNAME_AXIS;
        id.no = UBF_PACK(gr_axes_external_from_idx(cp, 0, Y_AXIS_IDX));
        break;
    }

    p_state->modifying_axis_idx = gr_axes_idx_from_external(cp, id.no, &p_state->modifying_axes_idx);

    if((GR_CHART_TYPE_SCAT != cp->axes[p_state->modifying_axes_idx].chart_type) && (GR_CHART_TYPE_SCAT != cp->axes[0].chart_type))
        if(p_state->modifying_axis_idx == X_AXIS_IDX)
            p_state->processing_cat = 1; /* give him a hand with id processing */
    } /*block*/

    p_state->id = id;
}

_Check_return_
static STATUS
gr_chart_axis_process_dialog(
    _InoutRef_  P_CHART_HEADER p_chart_header,
    _InoutRef_  P_GR_CHART_AXIS_STATE p_state)
{
    static UI_TEXT caption = { UI_TEXT_TYPE_USTR_TEMP };
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    UCHARZ buffer[BUF_MAX_GR_CHART_OBJID_REPR];
    gr_chart_object_name_from_id(p_gr_chart_from_chart_handle(p_chart_header->ch), p_state->id, ustr_bptr(buffer), elemof32(buffer));
    caption.text.ustr = ustr_bptr(buffer);
    {
    T5_MSG_CHART_DIALOG_DATA t5_msg_chart_dialog_data;
    msgclr(t5_msg_chart_dialog_data);
    t5_msg_chart_dialog_data.reason = p_state->processing_cat ? CHART_DIALOG_AXIS_CAT : CHART_DIALOG_AXIS_VAL;
    t5_msg_chart_dialog_data.modifying_axis_idx = p_state->modifying_axis_idx;
    status_return(object_call_id(OBJECT_ID_CHART, P_DOCU_NONE, T5_MSG_CHART_DIALOG, &t5_msg_chart_dialog_data));
    dialog_cmd_process_dbox_setup_ui_text(&dialog_cmd_process_dbox, t5_msg_chart_dialog_data.p_ctl_create, t5_msg_chart_dialog_data.n_ctls, &caption);
    dialog_cmd_process_dbox.help_topic_resource_id = t5_msg_chart_dialog_data.help_topic_resource_id;
    } /*block*/
    dialog_cmd_process_dbox.p_proc_client = dialog_event_gr_chart_axis_process;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_state;
    return(object_call_DIALOG_with_docu(p_docu_from_docno(p_chart_header->docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

_Check_return_
extern STATUS
gr_chart_axis_process(
    P_CHART_HEADER p_chart_header,
    _In_        GR_CHART_OBJID id)
{
    GR_CHART_AXIS_STATE state;
    STATUS status;

    gr_chart_axis_process_init(p_chart_header, id, &state);

    for(;;)
    {
        status = gr_chart_axis_process_dialog(p_chart_header, &state);

        if(status != DIALOG_COMPLETION_OK_PERSIST)
            break;
    }

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    return(status);
}

/******************************************************************************
*
* series processing can share a great deal with axis series defaults processing
*
******************************************************************************/

typedef struct GR_CHART_SERIES_STATE
{
    P_CHART_HEADER p_chart_header;

    GR_SERIES_IDX modifying_series_idx;
    GR_CHART_OBJID modifying_id;

    BOOL cumulative;
    BOOL cumulative_modified;

    BOOL vary_by_point;
    BOOL vary_by_point_modified;

    BOOL best_fit;
    BOOL best_fit_modified;

    BOOL fill_to_axis;
    BOOL fill_to_axis_modified;

    BOOL in_overlay;
}
GR_CHART_SERIES_STATE, * P_GR_CHART_SERIES_STATE;

_Check_return_
static STATUS
dialog_gr_chart_series_process_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_GR_CHART_SERIES_STATE p_state = (P_GR_CHART_SERIES_STATE) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case VAL_AXIS_ID_SERIES_CUMULATIVE:
        p_state->cumulative = p_dialog_msg_ctl_state_change->new_state.checkbox;
        p_state->cumulative_modified = 1;
        break;

    case VAL_AXIS_ID_SERIES_VARY_BY_POINT:
        p_state->vary_by_point = p_dialog_msg_ctl_state_change->new_state.checkbox;
        p_state->vary_by_point_modified = 1;
        break;

    case VAL_AXIS_ID_SERIES_BEST_FIT:
        p_state->best_fit = p_dialog_msg_ctl_state_change->new_state.checkbox;
        p_state->best_fit_modified = 1;
        break;

    case VAL_AXIS_ID_SERIES_FILL_TO_AXIS:
        p_state->fill_to_axis = p_dialog_msg_ctl_state_change->new_state.checkbox;
        p_state->fill_to_axis_modified = 1;
        break;

    case SERIES_ID_IN_OVERLAY:
        p_state->in_overlay = p_dialog_msg_ctl_state_change->new_state.checkbox;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

/* load current settings into structure */

_Check_return_
static STATUS
dialog_gr_chart_series_process_process_start(
    _InoutRef_  PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_GR_CHART_SERIES_STATE p_state = (P_GR_CHART_SERIES_STATE) p_dialog_msg_process_start->client_handle;
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_state->p_chart_header->ch);
    const P_GR_SERIES serp = getserp(cp, p_state->modifying_series_idx);
    const P_GR_AXES p_gr_axes = gr_axesp_from_series_idx(cp, p_state->modifying_series_idx);

    p_state->cumulative = serp->bits.cumulative_manual ? serp->bits.cumulative : p_gr_axes->bits.cumulative;
    status_return(ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SERIES_CUMULATIVE, p_state->cumulative));

    p_state->vary_by_point = serp->bits.vary_by_point_manual ? serp->bits.vary_by_point : p_gr_axes->bits.vary_by_point;
    status_return(ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SERIES_VARY_BY_POINT, p_state->vary_by_point));

    p_state->best_fit = serp->bits.best_fit_manual ? serp->bits.best_fit : p_gr_axes->bits.best_fit;
    status_return(ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SERIES_BEST_FIT, p_state->best_fit));

    p_state->fill_to_axis = serp->bits.fill_to_axis_manual ? serp->bits.fill_to_axis : p_gr_axes->bits.fill_to_axis;
    status_return(ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, VAL_AXIS_ID_SERIES_FILL_TO_AXIS, p_state->fill_to_axis));

    p_state->in_overlay = (p_state->modifying_series_idx >= cp->axes[1].series.stt_idx);
    status_return(ui_dlg_set_check(p_dialog_msg_process_start->h_dialog, SERIES_ID_IN_OVERLAY, p_state->in_overlay));
    ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, SERIES_ID_IN_OVERLAY, (cp->axes_idx_max == 1));

    p_state->cumulative_modified = 0;
    p_state->vary_by_point_modified = 0;
    p_state->fill_to_axis_modified = 0;
    p_state->best_fit_modified = 0;

    return(STATUS_OK);
}

/* set chart from modified structure */

_Check_return_
static STATUS
dialog_gr_chart_series_process_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_GR_CHART_SERIES_STATE p_state = (P_GR_CHART_SERIES_STATE) p_dialog_msg_process_end->client_handle;
        const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_state->p_chart_header->ch);
        const P_GR_SERIES serp = getserp(cp, p_state->modifying_series_idx);

        if(p_state->cumulative_modified)
        {
            serp->bits.cumulative = p_state->cumulative;
            serp->bits.cumulative_manual = 1;

            * (int *) &serp->valid = 0;
        }

        if(p_state->vary_by_point_modified)
        {
            serp->bits.vary_by_point = p_state->vary_by_point;
            serp->bits.vary_by_point_manual = 1;
        }

        if(p_state->best_fit_modified)
        {
            serp->bits.best_fit = p_state->best_fit;
            serp->bits.best_fit_manual = 1;
        }

        if(p_state->fill_to_axis_modified)
        {
            serp->bits.fill_to_axis = p_state->fill_to_axis;
            serp->bits.fill_to_axis_manual = 1;
        }

        chart_modify_docu(p_state->p_chart_header);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_gr_chart_series_process)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_gr_chart_series_process_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_gr_chart_series_process_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_gr_chart_series_process_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

static inline void
gr_chart_series_process_init(
    _InoutRef_  P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id_in,
    _OutRef_    P_GR_CHART_SERIES_STATE p_state)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_header->ch);

    p_state->p_chart_header = p_chart_header;

    switch(id_in.name)
    {
    default:
        p_state->modifying_series_idx = 0;
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_DROPSERIES:
    case GR_CHART_OBJNAME_BESTFITSER:
    case GR_CHART_OBJNAME_POINT:
    case GR_CHART_OBJNAME_DROPPOINT:
        p_state->modifying_series_idx = gr_series_idx_from_external(cp, id_in.no);
        break;
    }

    gr_chart_objid_from_series_idx(cp, p_state->modifying_series_idx, &p_state->modifying_id);
}

_Check_return_
static STATUS
gr_chart_series_process_dialog(
    _InoutRef_  P_CHART_HEADER p_chart_header,
    _InoutRef_  P_GR_CHART_SERIES_STATE p_state)
{
    static UI_TEXT caption = { UI_TEXT_TYPE_USTR_TEMP };
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    UCHARZ buffer[BUF_MAX_GR_CHART_OBJID_REPR];
    gr_chart_object_name_from_id(p_gr_chart_from_chart_handle(p_chart_header->ch), p_state->modifying_id, ustr_bptr(buffer), elemof32(buffer));
    caption.text.ustr = ustr_bptr(buffer);
    {
    T5_MSG_CHART_DIALOG_DATA t5_msg_chart_dialog_data;
    msgclr(t5_msg_chart_dialog_data);
    t5_msg_chart_dialog_data.reason = CHART_DIALOG_SERIES;
    status_return(object_call_id(OBJECT_ID_CHART, P_DOCU_NONE, T5_MSG_CHART_DIALOG, &t5_msg_chart_dialog_data));
    dialog_cmd_process_dbox_setup_ui_text(&dialog_cmd_process_dbox, t5_msg_chart_dialog_data.p_ctl_create, t5_msg_chart_dialog_data.n_ctls, &caption);
    dialog_cmd_process_dbox.help_topic_resource_id = t5_msg_chart_dialog_data.help_topic_resource_id;
    } /*block*/
    dialog_cmd_process_dbox.p_proc_client = dialog_event_gr_chart_series_process;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_state;
    return(object_call_DIALOG_with_docu(p_docu_from_docno(p_chart_header->docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

_Check_return_
extern STATUS
gr_chart_series_process(
    P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id_in)
{
    GR_CHART_SERIES_STATE state;
    STATUS status;

    gr_chart_series_process_init(p_chart_header, id_in, &state);

    for(;;)
    {
        status = gr_chart_series_process_dialog(p_chart_header, &state);

        if(status != DIALOG_COMPLETION_OK_PERSIST)
            break;
    }

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    return(status);
}

/* end of gr_uiaxi.c */
