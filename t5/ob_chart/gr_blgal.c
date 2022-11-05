/* gr_blgal.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Chart galleries for Fireworkz */

#include "common/gflags.h"

#include "ob_ss/xp_ss.h"

#include "ob_chart/ob_chart.h"

_Check_return_
static STATUS
chart_add_note(
    P_CHART_HEADER p_chart_header,
    P_CHART_SHAPEDESC p_chart_shapedesc)
{
    const P_DOCU p_docu = p_docu_from_docno(p_chart_header->docno);
    NOTE_INFO note_info;

    zero_struct_fn(note_info);

    note_info.layer = LAYER_CELLS_AREA_ABOVE; /* always insert into front layer */

    note_info.note_pinning = NOTE_PIN_CELLS_SINGLE; /* and pinned to a cell */

    /* try to position chart below the data if that cell could exist else position in bottom cell */
    note_info.region.tl.col = p_chart_shapedesc->region.tl.col;
    note_info.region.tl.row = p_chart_shapedesc->region.br.row;

    note_info.region.tl.row -= 1; /* SKS 04dec94 give in to pressure about note positioning - fixes silly when in letter anyway */

    if(p_docu->flags.next_chart_unpinned)
    {
        p_docu->flags.next_chart_unpinned = 0;

        note_info.note_pinning = NOTE_UNPINNED;

        /* try to position chart alongside the data */
        note_info.region.tl.col = p_chart_shapedesc->region.br.col;
        note_info.region.tl.row = p_chart_shapedesc->region.tl.row;
    }

    {
    ROW_ENTRY row_entry;
    row_entry_from_row(p_docu, &row_entry, note_info.region.tl.row);
    note_info.offset_tl.y = (row_entry.rowtab.edge_bot.pixit - row_entry.rowtab.edge_top.pixit) * 3 / 4;
    } /*block*/

    region_from_two_slrs(&note_info.region, &note_info.region.tl, &note_info.region.tl, 1);

    {
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_header->ch);

    if( (p_chart_shapedesc->bits.label_first_item  && (cp->axes[0].chart_type != GR_CHART_TYPE_PIE))
    ||  (p_chart_shapedesc->bits.label_first_range && (cp->axes[0].chart_type == GR_CHART_TYPE_PIE)) )
        /* SKS 15sep93 after 1.04 pies take legend defaults from category, not series headings */
    {
        cp->core.layout.margins.right = 1296; /* wow 0.9in */

        cp->core.layout.width = cp->core.layout.size.x + (cp->core.layout.margins.left + cp->core.layout.margins.right);

        cp->legend.bits.on = 1;
    }

    note_info.pixit_size.cx = cp->core.layout.width  + CHART_NOTE_SIZE_FUDGE_X;
    note_info.pixit_size.cy = cp->core.layout.height + CHART_NOTE_SIZE_FUDGE_Y;
    } /*block*/

    note_info.gr_scale_pair.x = GR_SCALE_ONE;
    note_info.gr_scale_pair.y = GR_SCALE_ONE;

    {
    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;

    skel_point_from_slr_tl(p_docu, &skel_rect.tl, &note_info.region.tl);
    skel_rect.tl.pixit_point.x += note_info.offset_tl.x;
    skel_rect.tl.pixit_point.y += note_info.offset_tl.y;
    skel_point_normalise(p_docu, &skel_rect.tl, UPDATE_PANE_CELLS_AREA);

    skel_rect.br = skel_rect.tl;
    skel_rect.br.pixit_point.x += note_info.pixit_size.cx;
    skel_rect.br.pixit_point.y += note_info.pixit_size.cy;
    skel_point_normalise(p_docu, &skel_rect.br, UPDATE_PANE_CELLS_AREA);

    RECT_FLAGS_CLEAR(rect_flags);
    view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);
    } /*block*/

    note_info.object_id = OBJECT_ID_CHART;
    note_info.object_data_ref = p_chart_header;

    return(object_call_id_load(p_docu, T5_MSG_NOTE_NEW, &note_info, OBJECT_ID_NOTE));
}

_Check_return_
extern STATUS
gr_chart_add_more(
    _DocuRef_   P_DOCU p_docu,
    P_CHART_HEADER p_chart_header)
{
    STATUS status = STATUS_OK;
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_header->ch);
    CHART_SHAPEDESC chart_shapedesc;

    if(!p_docu->mark_info_cells.h_markers)
        return(STATUS_OK);

    chart_shapedesc.docno = p_chart_header->docno;
    chart_shapedesc.bits.chart_type = cp->axes[cp->axes_idx_max].chart_type; /* as good a guess as any */
    chart_shapedesc.bits.label_first_range = 0;
    chart_shapedesc.bits.label_first_item = p_chart_header->label_first_item;
    chart_shapedesc.bits.range_over_columns = p_chart_header->range_over_columns;
    chart_shapedesc.bits.range_over_manual = 1;

    {
    DOCU_AREA docu_area;
    docu_area_normalise(p_docu, &docu_area, p_docu_area_from_markers_first(p_docu));
    status_return(chart_shape_start(p_docu, &chart_shapedesc, &docu_area));
    } /*block*/

    for(;;)
    {
        if((chart_shapedesc.bits.chart_type == GR_CHART_TYPE_SCAT) && !chart_shapedesc.bits.some_number_cells)
            status_break(status = create_error(CHART_ERR_NO_DATA));

        chart_shape_labels(&chart_shapedesc);

        chart_shape_continue(&chart_shapedesc);

        if(!chart_shapedesc.n_ranges)
            status_break(status = create_error(CHART_ERR_NO_DATA));

        if(status_ok(status = chart_add(p_chart_header, &chart_shapedesc)))
            chart_modify_docu(p_chart_header);

        break; /* out of loop for structure */
        /*NOTREACHED*/
    }

    chart_shape_end(&chart_shapedesc);

    return(status);
}

typedef struct CHART_BL_GALLERY_CALLBACK
{
    GR_CHART_TYPE chart_type;
    CHART_SHAPEDESC chart_shapedesc;
    P_S32 p_selected_pict;
    F64 pie_explode_value;
    F64 pie_start_heading_degrees;
    BOOL pie_anticlockwise;
    BOOL auto_pict;
    BOOL solid_if_auto_pict;
    S32 encode_level;
}
CHART_BL_GALLERY_CALLBACK, * P_CHART_BL_GALLERY_CALLBACK;

static U8
bl_3d_state = DIALOG_BUTTONSTATE_OFF;

static S32
pie_gallery_selected_explode_pict = 1;

_Check_return_
static STATUS
dialog_bl_gallery_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_CHART_BL_GALLERY_CALLBACK p_chart_bl_gallery_callback = (P_CHART_BL_GALLERY_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;
    STATUS status = STATUS_OK;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case BL_GALLERY_ID_SERIES_GROUP:
        {
        CHART_SHAPEDESC chart_shapedesc = p_chart_bl_gallery_callback->chart_shapedesc;

        /* grok it again, but in a more manual fashion (only difference is behaviour wrt tl cell) */
        chart_shapedesc.bits.range_over_manual = 1;
        chart_shapedesc.bits.range_over_columns = (U8) (p_dialog_msg_ctl_state_change->new_state.radiobutton != 0);
        chart_shape_labels(&chart_shapedesc);

        if(chart_shapedesc.bits.chart_type == GR_CHART_TYPE_SCAT)
            chart_shapedesc.bits.label_first_range = 1;

        if(status_ok(status = ui_dlg_set_radio(p_dialog_msg_ctl_state_change->h_dialog, BL_GALLERY_ID_FIRST_SERIES_GROUP, chart_shapedesc.bits.label_first_range)))
                     status = ui_dlg_set_radio(p_dialog_msg_ctl_state_change->h_dialog, BL_GALLERY_ID_FIRST_CATEGORY_GROUP, chart_shapedesc.bits.label_first_item);

        break;
        }

    case PIE_GALLERY_ID_START_POSITION_ANGLE_VALUE:
        {
        if(!p_chart_bl_gallery_callback->encode_level)
        {
            p_chart_bl_gallery_callback->encode_level += 1;
            status = ui_dlg_set_radio(p_dialog_msg_ctl_state_change->h_dialog, PIE_GALLERY_ID_START_POSITION_GROUP, (S32) p_dialog_msg_ctl_state_change->new_state.bump_f64);
            p_chart_bl_gallery_callback->encode_level -= 1;
        }

        break;
        }

    case PIE_GALLERY_ID_START_POSITION_GROUP:
        {
        if(!p_chart_bl_gallery_callback->encode_level)
        {
            const F64 f64 = p_dialog_msg_ctl_state_change->new_state.radiobutton;
            p_chart_bl_gallery_callback->encode_level += 1;
            status = ui_dlg_set_f64(p_dialog_msg_ctl_state_change->h_dialog, PIE_GALLERY_ID_START_POSITION_ANGLE_VALUE, f64);
            p_chart_bl_gallery_callback->encode_level -= 1;
        }

        break;
        }

    case PIE_GALLERY_ID_EXPLODE_GROUP:
        {
        if(p_dialog_msg_ctl_state_change->new_state.radiobutton != 1)
        {
            F64 f64 = ui_dlg_get_f64(p_dialog_msg_ctl_state_change->h_dialog, PIE_GALLERY_ID_EXPLODE_BY_VALUE);

            if(f64 == 0.0)
            {
                f64 = 25.0;

                status = ui_dlg_set_f64(p_dialog_msg_ctl_state_change->h_dialog, PIE_GALLERY_ID_EXPLODE_BY_VALUE, f64);
            }
        }

        break;
        }

    default:
        break;
    }

    return(status);
}

_Check_return_
static STATUS
dialog_bl_gallery_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_CHART_BL_GALLERY_CALLBACK p_chart_bl_gallery_callback = (P_CHART_BL_GALLERY_CALLBACK) p_dialog_msg_process_start->client_handle;
    const H_DIALOG h_dialog = p_dialog_msg_process_start->h_dialog;

    status_return(ui_dlg_set_radio(h_dialog, BL_GALLERY_ID_PICT_GROUP, *p_chart_bl_gallery_callback->p_selected_pict));

    switch(p_chart_bl_gallery_callback->chart_type)
    {
    case GR_CHART_TYPE_BAR:
    case GR_CHART_TYPE_LINE:
    case GR_CHART_TYPE_OVER_BL:
        status_return(ui_dlg_set_check(h_dialog, BL_GALLERY_ID_3D, bl_3d_state));
        break;

    case GR_CHART_TYPE_PIE:
        {
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;

        status_return(ui_dlg_set_radio(h_dialog, PIE_GALLERY_ID_EXPLODE_GROUP, pie_gallery_selected_explode_pict));

        msgclr(dialog_cmd_ctl_state_set);
        dialog_cmd_ctl_state_set.h_dialog = h_dialog;
        dialog_cmd_ctl_state_set.dialog_control_id = PIE_GALLERY_ID_START_POSITION_ANGLE_VALUE;
        dialog_cmd_ctl_state_set.state.bump_f64 = 0.0;
        dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_ALWAYS_MSG; /*NB*/
        status_return(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
        break;
        }

    default:
        break;
    }

    { /* make an initial stab at orientation in automatic mode; might actually change labelling accordingly in proc */
    CHART_SHAPEDESC chart_shapedesc = p_chart_bl_gallery_callback->chart_shapedesc;
    chart_shapedesc.bits.range_over_manual = 0;
    chart_shape_labels(&chart_shapedesc);
    status_return(ui_dlg_set_radio(h_dialog, BL_GALLERY_ID_SERIES_GROUP, chart_shapedesc.bits.range_over_columns));
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_bl_gallery_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_CHART_BL_GALLERY_CALLBACK p_chart_bl_gallery_callback = (P_CHART_BL_GALLERY_CALLBACK) p_dialog_msg_process_end->client_handle;
        const H_DIALOG h_dialog = p_dialog_msg_process_end->h_dialog;

        p_chart_bl_gallery_callback->chart_shapedesc.bits.range_over_columns = (U8) (ui_dlg_get_radio(h_dialog, BL_GALLERY_ID_SERIES_GROUP) != 0);
        p_chart_bl_gallery_callback->chart_shapedesc.bits.label_first_range  = (U8) (ui_dlg_get_radio(h_dialog, BL_GALLERY_ID_FIRST_SERIES_GROUP) != 0);
        p_chart_bl_gallery_callback->chart_shapedesc.bits.label_first_item   = (U8) (ui_dlg_get_radio(h_dialog, BL_GALLERY_ID_FIRST_CATEGORY_GROUP) != 0);

        *p_chart_bl_gallery_callback->p_selected_pict = ui_dlg_get_radio(h_dialog, BL_GALLERY_ID_PICT_GROUP);

        switch(p_chart_bl_gallery_callback->chart_type)
        {
        case GR_CHART_TYPE_BAR:
        case GR_CHART_TYPE_LINE:
        case GR_CHART_TYPE_OVER_BL:
            bl_3d_state = (U8) ui_dlg_get_check(h_dialog, BL_GALLERY_ID_3D);
            break;

        case GR_CHART_TYPE_PIE:
            pie_gallery_selected_explode_pict = ui_dlg_get_radio(h_dialog, PIE_GALLERY_ID_EXPLODE_GROUP);
            p_chart_bl_gallery_callback->pie_start_heading_degrees = ui_dlg_get_f64(h_dialog, PIE_GALLERY_ID_START_POSITION_ANGLE_VALUE);
            p_chart_bl_gallery_callback->pie_explode_value = ui_dlg_get_f64(h_dialog, PIE_GALLERY_ID_EXPLODE_BY_VALUE);
            p_chart_bl_gallery_callback->pie_anticlockwise = ui_dlg_get_check(h_dialog, PIE_GALLERY_ID_ANTICLOCKWISE);
            break;

        default:
            break;
        }
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_bl_gallery)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_bl_gallery_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_bl_gallery_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_bl_gallery_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
gr_chart_bl_gallery(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     GR_CHART_TYPE chart_type)
{
    CHART_BL_GALLERY_CALLBACK chart_bl_gallery_callback;
    P_CHART_HEADER p_chart_header = NULL;
    STATUS status = STATUS_OK;

    if(!p_docu->mark_info_cells.h_markers)
        return(STATUS_OK);

    chart_bl_gallery_callback.chart_shapedesc.docno = docno_from_p_docu(p_docu);
    chart_bl_gallery_callback.chart_shapedesc.bits.chart_type = chart_type;

    chart_bl_gallery_callback.encode_level = 0;
    chart_bl_gallery_callback.chart_type = chart_type;

    {
    DOCU_AREA docu_area;
    docu_area_normalise(p_docu, &docu_area, p_docu_area_from_markers_first(p_docu));
    status_return(chart_shape_start(p_docu, &chart_bl_gallery_callback.chart_shapedesc, &docu_area));
    } /*block*/

    for(;;)
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;

        chart_bl_gallery_callback.pie_anticlockwise = FALSE;
        chart_bl_gallery_callback.auto_pict = FALSE;
        chart_bl_gallery_callback.solid_if_auto_pict = TRUE;

        if((chart_bl_gallery_callback.chart_shapedesc.bits.chart_type == GR_CHART_TYPE_SCAT) && !chart_bl_gallery_callback.chart_shapedesc.bits.some_number_cells)
            status_break(status = create_error(CHART_ERR_NO_DATA));

        {
        T5_MSG_CHART_GALLERY_DATA t5_msg_chart_gallery_data;
        msgclr(t5_msg_chart_gallery_data);
        t5_msg_chart_gallery_data.chart_type = chart_type;
        status_break(status = object_call_id(OBJECT_ID_CHART, P_DOCU_NONE, T5_MSG_CHART_GALLERY, &t5_msg_chart_gallery_data));
        chart_bl_gallery_callback.p_selected_pict = t5_msg_chart_gallery_data.p_selected_pict;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, t5_msg_chart_gallery_data.p_ctl_create, t5_msg_chart_gallery_data.n_ctls, t5_msg_chart_gallery_data.resource_id);
        dialog_cmd_process_dbox.help_topic_resource_id = t5_msg_chart_gallery_data.help_topic_resource_id;
        } /*block*/

        dialog_cmd_process_dbox.p_proc_client = dialog_event_bl_gallery;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &chart_bl_gallery_callback;
        status_break(status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));

        /* restrict initial penguin chart */
        if(chart_bl_gallery_callback.chart_shapedesc.bits.chart_type == GR_CHART_TYPE_BAR)
            if((*chart_bl_gallery_callback.p_selected_pict == 5) || (*chart_bl_gallery_callback.p_selected_pict == 6))
                chart_bl_gallery_callback.chart_shapedesc.allowed_numbers = 1;

        chart_shape_continue(&chart_bl_gallery_callback.chart_shapedesc);

        if(!chart_bl_gallery_callback.chart_shapedesc.n_ranges)
            status_break(status = create_error(CHART_ERR_NO_DATA));

        if(status_ok(status = chart_new(&p_chart_header, 1, 1)))
        {
            p_docu->last_chart_edited = p_chart_header->chartdatakey;

            p_chart_header->docno = chart_bl_gallery_callback.chart_shapedesc.docno;

            p_chart_header->label_first_item = chart_bl_gallery_callback.chart_shapedesc.bits.label_first_item;
            p_chart_header->range_over_columns = chart_bl_gallery_callback.chart_shapedesc.bits.range_over_columns;

            if(status_ok(status = chart_add(p_chart_header, &chart_bl_gallery_callback.chart_shapedesc)))
            {
                const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_header->ch);

                cp->axes[0].chart_type = chart_bl_gallery_callback.chart_shapedesc.bits.chart_type;

                switch(chart_bl_gallery_callback.chart_shapedesc.bits.chart_type)
                {
                case GR_CHART_TYPE_BAR:
                    {
                    cp->d3.bits.on = (bl_3d_state == DIALOG_BUTTONSTATE_ON);

                    switch(*chart_bl_gallery_callback.p_selected_pict)
                    {
                    default:
                    case 1:
                        cp->axes[0].sertype = GR_CHART_SERIES_PLAIN;
                        break;

                    case 2:
                        cp->axes[0].sertype = GR_CHART_SERIES_PLAIN;
                        cp->axes[0].bits.vary_by_point = 1;
                        break;

                    case 3:
                        cp->axes[0].sertype = GR_CHART_SERIES_PLAIN;
                        cp->axes[0].bits.stacked = 1;
                        break;

                    case 4:
                        cp->axes[0].sertype = GR_CHART_SERIES_PLAIN;
                        cp->axes[0].bits.stacked = 1;
                        cp->axes[0].bits.stacked_pct = 1;
                        break;

                    case 5:
                        cp->axes[0].style.barch.bits.pictures_stacked = 1;

                        /*FALLTHRU*/

                    case 6:
                        cp->axes[0].sertype = GR_CHART_SERIES_PLAIN;
                        chart_bl_gallery_callback.auto_pict = TRUE;
                        chart_bl_gallery_callback.solid_if_auto_pict = FALSE;
                        break;

                    case 7:
                        cp->axes[0].sertype = GR_CHART_SERIES_PLAIN_ERROR1;
                        break;

                    case 8:
                        cp->axes[0].sertype = GR_CHART_SERIES_PLAIN;
                        cp->axes[0].bits.cumulative = 1;
                        break;
                    }

                    break;
                    }

                case GR_CHART_TYPE_LINE:
                    {
                    cp->d3.bits.on = (bl_3d_state == DIALOG_BUTTONSTATE_ON);

                    switch(*chart_bl_gallery_callback.p_selected_pict)
                    {
                    case 2:
                        chart_bl_gallery_callback.auto_pict = TRUE;

                        /*FALLTHRU*/

                    default:
                    case 1:
                        cp->axes[0].sertype = GR_CHART_SERIES_PLAIN;
                        break;

                    case 4:
                        chart_bl_gallery_callback.auto_pict = TRUE;

                        /*FALLTHRU*/

                    case 3:
                        cp->axes[0].bits.fill_to_axis = 1;
                        cp->axes[0].sertype = GR_CHART_SERIES_PLAIN;
                        break;

                    case 6:
                        chart_bl_gallery_callback.auto_pict = TRUE;

                        /*FALLTHRU*/

                    case 5:
                        cp->axes[0].bits.stacked = 1;
                        cp->axes[0].bits.fill_to_axis = 1;
                        cp->axes[0].sertype = GR_CHART_SERIES_PLAIN;
                        break;

                    case 8:
                        chart_bl_gallery_callback.auto_pict = TRUE;

                        /*FALLTHRU*/

                    case 7:
                        cp->axes[0].sertype = GR_CHART_SERIES_PLAIN_ERROR1;
                        break;
                    }

                    break;
                    }

                case GR_CHART_TYPE_OVER_BL:
                    {
                    cp->d3.bits.on = (bl_3d_state == DIALOG_BUTTONSTATE_ON);

                    cp->axes_idx_max = 1;

                    cp->axes[0].sertype = GR_CHART_SERIES_PLAIN;
                    cp->axes[1].sertype = GR_CHART_SERIES_PLAIN;

                    switch(*chart_bl_gallery_callback.p_selected_pict)
                    {
                    default:
                    case 1:
                        cp->axes[0].chart_type = GR_CHART_TYPE_BAR;
                        cp->axes[1].chart_type = GR_CHART_TYPE_BAR;
                        break;

                    case 2:
                        cp->axes[0].chart_type = GR_CHART_TYPE_LINE;
                        cp->axes[1].chart_type = GR_CHART_TYPE_BAR;
                        break;

                    case 3:
                        cp->axes[0].chart_type = GR_CHART_TYPE_LINE; /* SKS 08sep93 after 1.04 was BAR */
                        cp->axes[1].chart_type = GR_CHART_TYPE_LINE;
                        break;
                    }

                    break;
                    }

                case GR_CHART_TYPE_PIE:
                    {
                    cp->axes[0].sertype = GR_CHART_SERIES_PLAIN;

                    cp->axes[0].bits.vary_by_point = 1;

                    cp->axes[0].style.piechlabel.bits.manual = 1;

                    switch(*chart_bl_gallery_callback.p_selected_pict) /* labelling */
                    {
                    default:
                    case 1:
                        cp->axes[0].style.piechlabel.bits.label_leg = 0;
                        cp->axes[0].style.piechlabel.bits.label_val = 0;
                        cp->axes[0].style.piechlabel.bits.label_pct = 0;
                        break;

                    case 2:
                        cp->axes[0].style.piechlabel.bits.label_leg = 1;
                        cp->axes[0].style.piechlabel.bits.label_val = 0;
                        cp->axes[0].style.piechlabel.bits.label_pct = 0;

                        cp->core.layout.width += PIXITS_PER_INCH / 2; /* make it wider to accomodate longer text labels */
                        break;

                    case 3:
                        cp->axes[0].style.piechlabel.bits.label_leg = 0;
                        cp->axes[0].style.piechlabel.bits.label_val = 1;
                        cp->axes[0].style.piechlabel.bits.label_pct = 0;
                        break;

                    case 4:
                        cp->axes[0].style.piechlabel.bits.label_leg = 0;
                        cp->axes[0].style.piechlabel.bits.label_val = 0;
                        cp->axes[0].style.piechlabel.bits.label_pct = 1;
                        break;
                    }

                    break; /* SKS 16sep93 after 1.04 was missing! */
                    }

                default: default_unhandled();
#if CHECKING
                case GR_CHART_TYPE_SCAT:
#endif
                    {
                    cp->axes[0].axis[X_AXIS_IDX].bits.incl_zero = 0;
                    cp->axes[0].axis[Y_AXIS_IDX].bits.incl_zero = 0;

                    /* yes these look really wierd but it's a sad overload meaning XYY or XYXY */
                    switch(*chart_bl_gallery_callback.p_selected_pict)
                    {
                    default:
                    case 1:
                    case 4:
                    case 7:
                        cp->axes[0].sertype = chart_bl_gallery_callback.chart_shapedesc.bits.label_first_range ? GR_CHART_SERIES_PLAIN : GR_CHART_SERIES_POINT;
                        break;

                    case 2:
                    case 5:
                    case 8:
                        cp->axes[0].sertype = chart_bl_gallery_callback.chart_shapedesc.bits.label_first_range ? GR_CHART_SERIES_PLAIN_ERROR1 : GR_CHART_SERIES_POINT_ERROR1;
                        break;

                    case 3:
                    case 6:
                    case 9:
                        cp->axes[0].sertype = chart_bl_gallery_callback.chart_shapedesc.bits.label_first_range ? GR_CHART_SERIES_PLAIN_ERROR2 : GR_CHART_SERIES_POINT_ERROR2;
                        break;
                    }

                    switch(*chart_bl_gallery_callback.p_selected_pict)
                    {
                    default:
                        chart_bl_gallery_callback.auto_pict = TRUE;
                        break;

                    case 1:
                    case 2:
                    case 3:
                        break;
                    }

                    switch(*chart_bl_gallery_callback.p_selected_pict)
                    {
                    default:
                        break;

                    case 4:
                    case 5:
                    case 6:
                        cp->axes[0].style.scatch.bits.lines_off = 1;
                        break;
                    }

                    break;
                    }
                }

#if RISCOS
                /* round chart size out to worst case pixels */
                cp->core.layout.margins.top    = idiv_ceil(cp->core.layout.margins.top,    4 * PIXITS_PER_RISCOS) * 4 * PIXITS_PER_RISCOS;
                cp->core.layout.margins.bottom = idiv_ceil(cp->core.layout.margins.bottom, 4 * PIXITS_PER_RISCOS) * 4 * PIXITS_PER_RISCOS;
                cp->core.layout.margins.left   = idiv_ceil(cp->core.layout.margins.left,   2 * PIXITS_PER_RISCOS) * 2 * PIXITS_PER_RISCOS;
                cp->core.layout.margins.right  = idiv_ceil(cp->core.layout.margins.right,  2 * PIXITS_PER_RISCOS) * 2 * PIXITS_PER_RISCOS;
                cp->core.layout.width          = idiv_ceil(cp->core.layout.width,          2 * PIXITS_PER_RISCOS) * 2 * PIXITS_PER_RISCOS;
                cp->core.layout.height         = idiv_ceil(cp->core.layout.height,         4 * PIXITS_PER_RISCOS) * 4 * PIXITS_PER_RISCOS;
#endif

                status = gr_chart_realloc_series(cp);

                if(chart_bl_gallery_callback.auto_pict)
                {
                    GR_SERIES_IDX series_idx;

                    for(series_idx = cp->axes[0].series.stt_idx; series_idx < cp->axes[0].series.end_idx; ++series_idx)
                    {
                        const P_GR_SERIES serp = getserp(cp, series_idx);

                        serp->style.point_fillb.bits.manual = 1;
                        serp->style.point_fillb.bits.pattern = 1; /* auto-pattern 'cos will have NULL handle */
                        serp->style.point_fillb.bits.notsolid = !chart_bl_gallery_callback.solid_if_auto_pict;
                        serp->style.point_fillb.bits.norecolour = !chart_bl_gallery_callback.solid_if_auto_pict;
                    }
                }

                if(chart_type == GR_CHART_TYPE_PIE)
                {
                    const P_GR_SERIES serp = getserp(cp, 0);
                    GR_CHART_OBJID modifying_id;
                    GR_PIECHDISPLSTYLE gr_piechdisplstyle;

                    gr_chart_objid_from_series_idx(cp, 0, &modifying_id);

                    serp->style.pie_start_heading_degrees = chart_bl_gallery_callback.pie_start_heading_degrees;
                    serp->bits.pie_anticlockwise = chart_bl_gallery_callback.pie_anticlockwise;

                    switch(pie_gallery_selected_explode_pict)
                    {
                    default:
                    case 1:
                    case 3:
                        break;

                    case 2:
                        modifying_id.name = GR_CHART_OBJNAME_POINT;
                        modifying_id.has_subno = 1;
                        modifying_id.subno = 1;
                        break;
                    }

                    gr_piechdisplstyle.bits.manual = 1;

                    switch(pie_gallery_selected_explode_pict)
                    {
                    default:
                    case 1:
                        gr_piechdisplstyle.radial_displacement = 0.0;
                        break;

                    case 2:
                    case 3:
                        gr_piechdisplstyle.radial_displacement = chart_bl_gallery_callback.pie_explode_value;
                        break;
                    }

                    if(status_ok(status))
                        status = gr_chart_objid_piechdisplstyle_set(cp, modifying_id, &gr_piechdisplstyle);
                }

                if(status_ok(status))
                    status = chart_add_note(p_chart_header, &chart_bl_gallery_callback.chart_shapedesc);
            }

            if(status_fail(status))
                chart_dispose(&p_chart_header);
            else
            {
                chart_modify_docu(p_chart_header);
                chart_rebuild_after_modify(p_chart_header);
            }
        }

        break; /* out of loop for structure */
        /*NOTREACHED*/
    }

    chart_shape_end(&chart_bl_gallery_callback.chart_shapedesc);

    return(status);
}

/* end of gr_blgal.c */
