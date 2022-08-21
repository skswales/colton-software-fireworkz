/* sk_save.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* File save routines for Fireworkz */

/* JAD Mar 1992; MRJC October 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/ff_io.h"

/*
internal structure
*/

typedef struct SKEL_SAVE_CELL
{
    PC_CONSTRUCT_TABLE  p_construct;
    ARGLIST_HANDLE      arglist_handle;
    OBJECT_DATA         object_data;
}
SKEL_SAVE_CELL, * P_SKEL_SAVE_CELL;

/*
internal routines
*/

_Check_return_
static STATUS
save_cell(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    P_SKEL_SAVE_CELL p_skel_save_cell);

_Check_return_
static STATUS
save_style_docu_area(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InRef_     PC_STYLE_DOCU_AREA p_style_docu_area);

_Check_return_
static STATUS
save_rowtable(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format);

PROC_EVENT_PROTO(static, scheduled_event_auto_save);

_Check_return_
extern STATUS
auto_save(
    _DocuRef_   P_DOCU p_docu,
    _In_        S32 auto_save_period_minutes)
{
    BOOL once_only = FALSE;
    STATUS status = STATUS_OK;

    if(-1 == auto_save_period_minutes) /* i.e. do it once only */
    {
        auto_save_period_minutes = 0;
        once_only = TRUE;
    }

    if(!once_only)
    {
        p_docu->auto_save_period_minutes = auto_save_period_minutes;

        /* cancel any outstanding autosaves for this document */
        trace_1(TRACE__SCHEDULED, TEXT("auto_save - *** scheduled_event_remove(docno=%d), cancel pending auto save"), docno_from_p_docu(p_docu));
        scheduled_event_remove(docno_from_p_docu(p_docu), T5_EVENT_TRY_AUTO_SAVE, scheduled_event_auto_save, 0);
    }

    if(auto_save_period_minutes || once_only)
    {   /* schedule me an AutoSave event for sometime in the future */
        const MONOTIMEDIFF after = MONOTIMEDIFF_VALUE_FROM_SECONDS(auto_save_period_minutes * (MONOTIMEDIFF) 60);
        trace_1(TRACE__SCHEDULED, TEXT("auto_save - *** do scheduled_event_after(docno=%d, n)"), docno_from_p_docu(p_docu));
        status = status_wrap(scheduled_event_after(docno_from_p_docu(p_docu), T5_EVENT_TRY_AUTO_SAVE, scheduled_event_auto_save, 0, after));
    }

    return(status);
}

_Check_return_
static STATUS
do_auto_save(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;

    if(p_docu->modified)
        status_consume(execute_command_reperr(p_docu, T5_CMD_SAVE_OWNFORM, _P_DATA_NONE(P_ARGLIST_HANDLE), OBJECT_ID_SKEL));

    /* and rechedule for another one sometime */
    if(p_docu->auto_save_period_minutes)
    {   /* schedule me an AutoSave event for sometime in the future */
        const MONOTIMEDIFF after = MONOTIMEDIFF_VALUE_FROM_SECONDS(p_docu->auto_save_period_minutes * (MONOTIMEDIFF) 60);
        trace_1(TRACE__SCHEDULED, TEXT("do_auto_save - *** do scheduled_event_after(docno=%d, n)"), docno_from_p_docu(p_docu));
        status = status_wrap(scheduled_event_after(docno_from_p_docu(p_docu), T5_EVENT_TRY_AUTO_SAVE, scheduled_event_auto_save, 0, after));
    }

    return(status);
}

PROC_EVENT_PROTO(static, scheduled_event_auto_save)
{
    UNREFERENCED_PARAMETER(p_data);

    switch(t5_message)
    {
    case T5_EVENT_TRY_AUTO_SAVE:
        /* P_SCHEDULED_EVENT_BLOCK p_scheduled_event_block = (P_SCHEDULED_EVENT_BLOCK) p_data; */
        return(do_auto_save(p_docu));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
save_auto_save(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    if(p_of_op_format->of_template.data_class >= DATA_SAVE_WHOLE_DOC)
    {
        if(p_docu->auto_save_period_minutes)
        {
            const OBJECT_ID object_id = OBJECT_ID_SKEL;
            PC_CONSTRUCT_TABLE p_construct_table;
            ARGLIST_HANDLE arglist_handle;

            if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_AUTO_SAVE, &p_construct_table)))
            {
                const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
                p_args[0].val.s32 = p_docu->auto_save_period_minutes;
                status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
                arglist_dispose(&arglist_handle);
            }
        }
    }

    return(status);
}

_Check_return_
static STATUS
save_base_single_col(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    if(p_docu->flags.base_single_col) /* only bother saving non-default value (i.e. non-zero) */
    /*if(p_of_op_format->of_template.data_class >= DATA_SAVE_CHARACTER)*/
    {
        const OBJECT_ID object_id = OBJECT_ID_SKEL;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE arglist_handle;

        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_OF_BASE_SINGLE_COL, &p_construct_table)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
            p_args[0].val.fBool = p_docu->flags.base_single_col;
            status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
            arglist_dispose(&arglist_handle);
        }
    }

    return(status);
}

/******************************************************************************
*
* save out block construct describing data
*
******************************************************************************/

_Check_return_
static STATUS
save_block(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    /*if(p_of_op_format->of_template.data_class >= DATA_SAVE_CHARACTER)*/
    {
        const OBJECT_ID object_id = OBJECT_ID_SKEL;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE arglist_handle;
        SAVE_CELL_OWNFORM save_cell_ownform_stt_frag; UCHARZ stt_frag_buffer[128];
        SAVE_CELL_OWNFORM save_cell_ownform_end_frag; UCHARZ end_frag_buffer[128];
        P_ARGLIST_ARG p_args;
        BOOL single_slot_frag = 0;
        COL col_s, col_e;
        ROW row_s, row_e;

        zero_struct(save_cell_ownform_stt_frag);
        zero_struct(save_cell_ownform_end_frag);

        status_return(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_OF_BLOCK, &p_construct_table));

        p_args = p_arglist_args(&arglist_handle, ARG_BLOCK_N_ARGS);

        quick_ublock_setup(&save_cell_ownform_stt_frag.contents_data_quick_ublock, stt_frag_buffer);
        quick_ublock_setup(&save_cell_ownform_end_frag.contents_data_quick_ublock, end_frag_buffer);

        limits_from_docu_area(p_docu, &col_s, &col_e, &row_s, &row_e, &p_of_op_format->save_docu_area);

        /* Top left region coordinate */
        p_args[ARG_BLOCK_TL_COL].val.s32 = col_s;
        p_args[ARG_BLOCK_TL_ROW].val.s32 = row_s;
        p_args[ARG_BLOCK_TL_OBJECT_ID].val.s32 = p_of_op_format->save_docu_area.tl.object_position.object_id;
        if(OBJECT_ID_NONE == p_of_op_format->save_docu_area.tl.object_position.object_id)
            arg_dispose(&arglist_handle, ARG_BLOCK_TL_DATA);
        else
            p_args[ARG_BLOCK_TL_DATA].val.s32 = p_of_op_format->save_docu_area.tl.object_position.data;

        /* Bottom right region coordinate */
        p_args[ARG_BLOCK_BR_COL].val.s32 = col_e;
        p_args[ARG_BLOCK_BR_ROW].val.s32 = row_e;
        p_args[ARG_BLOCK_BR_OBJECT_ID].val.s32 = p_of_op_format->save_docu_area.br.object_position.object_id;
        if(OBJECT_ID_NONE == p_of_op_format->save_docu_area.br.object_position.object_id)
            arg_dispose(&arglist_handle, ARG_BLOCK_BR_DATA);
        else
            p_args[ARG_BLOCK_BR_DATA].val.s32 = p_of_op_format->save_docu_area.br.object_position.data;

        /* document size */
        p_args[ARG_BLOCK_DOC_COLS].val.s32 = n_cols_logical(p_docu);
        p_args[ARG_BLOCK_DOC_ROWS].val.s32 = n_rows(p_docu);

        /* start fragment */
        if(!(p_of_op_format->save_docu_area.whole_col || p_of_op_format->save_docu_area.whole_row))
        {
            (void) cell_data_from_docu_area_tl(p_docu, &save_cell_ownform_stt_frag.object_data, &p_of_op_format->save_docu_area);

            if(OBJECT_ID_NONE != save_cell_ownform_stt_frag.object_data.object_position_start.object_id)
            {
                single_slot_frag = (OBJECT_ID_NONE != save_cell_ownform_stt_frag.object_data.object_position_end.object_id);

                p_args[ARG_BLOCK_FA_CS1].val.u8c = construct_id_from_object_id(save_cell_ownform_stt_frag.object_data.object_id);

                save_cell_ownform_stt_frag.p_of_op_format = p_of_op_format;

                status = object_call_id(save_cell_ownform_stt_frag.object_data.object_id,
                                        p_docu,
                                        T5_MSG_SAVE_CELL_OWNFORM,
                                        &save_cell_ownform_stt_frag);

                p_of_op_format->save_docu_area_no_frags.tl.slr.row += 1;
                p_of_op_format->save_docu_area_no_frags.tl.object_position.object_id = OBJECT_ID_NONE;
            }
        }

        if(status_ok(status))
        {
            /* end fragment */
            if(!(p_of_op_format->save_docu_area.whole_col || p_of_op_format->save_docu_area.whole_row))
            {
                (void) cell_data_from_docu_area_br(p_docu, &save_cell_ownform_end_frag.object_data, &p_of_op_format->save_docu_area);

                if(!single_slot_frag && (OBJECT_ID_NONE != p_of_op_format->save_docu_area.br.object_position.object_id))
                {
                    p_args[ARG_BLOCK_FB_CS1].val.u8c = construct_id_from_object_id(save_cell_ownform_end_frag.object_data.object_id);

                    save_cell_ownform_end_frag.p_of_op_format = p_of_op_format;

                    status = object_call_id(save_cell_ownform_end_frag.object_data.object_id,
                                            p_docu,
                                            T5_MSG_SAVE_CELL_OWNFORM,
                                            &save_cell_ownform_end_frag);

                    p_of_op_format->save_docu_area_no_frags.br.slr.row -= 1;
                    p_of_op_format->save_docu_area_no_frags.br.object_position.object_id = OBJECT_ID_NONE;
                }
            }
        }

        if(status_ok(status))
        {
            if(0 == quick_ublock_bytes(&save_cell_ownform_stt_frag.contents_data_quick_ublock))
            {
                p_args[ARG_BLOCK_FA].type = ARG_TYPE_NONE;
                p_args[ARG_BLOCK_FA_CS1].type = ARG_TYPE_NONE;
                p_args[ARG_BLOCK_FA_CS2].type = ARG_TYPE_NONE;
            }
            else if(status_ok(status = quick_ublock_nullch_add(&save_cell_ownform_stt_frag.contents_data_quick_ublock)))
            {
                U8 data_char;
                p_args[ARG_BLOCK_FA].val.ustr_inline = quick_ublock_ustr_inline(&save_cell_ownform_stt_frag.contents_data_quick_ublock);
                data_char = construct_id_from_data_type(save_cell_ownform_stt_frag.data_type);
                p_args[ARG_BLOCK_FA_CS2].val.u8c = data_char;
            }
        }

        if(status_ok(status))
        {
            if(0 == quick_ublock_bytes(&save_cell_ownform_end_frag.contents_data_quick_ublock))
            {
                p_args[ARG_BLOCK_FB].type = ARG_TYPE_NONE;
                p_args[ARG_BLOCK_FB_CS1].type = ARG_TYPE_NONE;
                p_args[ARG_BLOCK_FB_CS2].type = ARG_TYPE_NONE;
            }
            else if(status_ok(status = quick_ublock_nullch_add(&save_cell_ownform_end_frag.contents_data_quick_ublock)))
            {
                U8 data_char;
                p_args[ARG_BLOCK_FB].val.ustr_inline = quick_ublock_ustr_inline(&save_cell_ownform_end_frag.contents_data_quick_ublock);
                data_char = construct_id_from_data_type(save_cell_ownform_end_frag.data_type);
                p_args[ARG_BLOCK_FB_CS2].val.u8c = data_char;
            }
        }

        if(status_ok(status))
            status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

        quick_ublock_dispose(&save_cell_ownform_stt_frag.contents_data_quick_ublock);
        quick_ublock_dispose(&save_cell_ownform_end_frag.contents_data_quick_ublock);

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

/******************************************************************************
*
* cell saving is now done on SAVE_2 so objects can
* save out related data before and after cells
*
******************************************************************************/

_Check_return_
static STATUS
save_data_save_2(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    /* this only ever saves whole cells; the frags have been done by save_block() */
    if(p_of_op_format->of_template.data_class >= DATA_SAVE_MANY)
    {
        SCAN_BLOCK scan_block;
        SKEL_SAVE_CELL skel_save_cell;
        STATUS status = STATUS_OK;

        status_return(arglist_prepare_with_construct(&skel_save_cell.arglist_handle, OBJECT_ID_SKEL, T5_CMD_OF_CELL, &skel_save_cell.p_construct));

        if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_DOWN, SCAN_AREA, &p_of_op_format->save_docu_area_no_frags, OBJECT_ID_NONE)))
        {
            while(status_done(cells_scan_next(p_docu, &skel_save_cell.object_data, &scan_block)))
            {
                save_reflect_status(p_of_op_format, cells_scan_percent(&scan_block));

                status = save_cell(p_of_op_format, &skel_save_cell);

                /*p_of_op_format->offset++;*/

                p_docu = p_docu_from_docno(p_of_op_format->docno);

                status_break(status);
            }
        }

        arglist_dispose(&skel_save_cell.arglist_handle);

        status_return(status);
    }

    /* Check cell regions are within that supplied, and whether clipping is required */
    /*if(p_of_op_format->of_template.data_class >= DATA_SAVE_CHARACTER)*/
    {
        /* avoid saving CurrentCell style etc. in CHARACTER/MANY */
        const REGION_CLASS region_class = (p_of_op_format->of_template.data_class <= DATA_SAVE_MANY) ? REGION_UPPER : REGION_END;
        ARRAY_INDEX style_docu_area_ix;
        STATUS status = STATUS_OK;

        /* look through region list forwards */
        for(style_docu_area_ix = 0; style_docu_area_ix < array_elements(&p_docu->h_style_docu_area); ++style_docu_area_ix)
        {
            PC_STYLE_DOCU_AREA p_style_docu_area = array_ptr(&p_docu->h_style_docu_area, STYLE_DOCU_AREA, style_docu_area_ix);

            if(p_style_docu_area->deleted)
                continue;

            if(p_style_docu_area->region_class >= region_class)
                continue;

            if(style_save_docu_area_save_from_index(p_docu,
                                                    p_style_docu_area,
                                                    &p_of_op_format->save_docu_area,
                                                    p_of_op_format->of_template.data_class))
            {
                    status_break(status = save_style_docu_area(p_of_op_format, p_style_docu_area));
            }
        }

        status_return(status);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
save_style_docu_area(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InRef_     PC_STYLE_DOCU_AREA p_style_docu_area)
{
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;
    P_ARGLIST_ARG p_args;
    T5_MESSAGE t5_message;
    S32 style_arg_no;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 500);
    quick_ublock_with_buffer_setup(quick_ublock);

    if(OBJECT_ID_NONE != p_style_docu_area->object_message.object_id)
    {
        if(p_style_docu_area->base)
            t5_message = T5_CMD_OF_BASE_IMPLIED_REGION;
        else
            t5_message = T5_CMD_OF_IMPLIED_REGION;
    }
    else
    {
        if(p_style_docu_area->base)
            t5_message = T5_CMD_OF_BASE_REGION;
        else
            t5_message = T5_CMD_OF_REGION;
    }

    status_return(arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table));

    p_args = p_arglist_args(&arglist_handle, ARG_DOCU_AREA_N_ARGS);

    /* first fill in docu_area parameters */
    /* Top left */
    if(p_style_docu_area->docu_area.whole_row)
        arg_dispose(&arglist_handle, ARG_DOCU_AREA_TL_COL);
    else
        p_args[ARG_DOCU_AREA_TL_COL].val.s32 = p_style_docu_area->docu_area.tl.slr.col;

    if(p_style_docu_area->docu_area.whole_col)
        arg_dispose(&arglist_handle, ARG_DOCU_AREA_TL_ROW);
    else
        p_args[ARG_DOCU_AREA_TL_ROW].val.s32 = p_style_docu_area->docu_area.tl.slr.row;

    if( p_style_docu_area->docu_area.whole_row || p_style_docu_area->docu_area.whole_col ||
        (OBJECT_ID_NONE == p_style_docu_area->docu_area.tl.object_position.object_id) )
        arg_dispose(&arglist_handle, ARG_DOCU_AREA_TL_DATA);
    else
        p_args[ARG_DOCU_AREA_TL_DATA].val.s32 = p_style_docu_area->docu_area.tl.object_position.data;

    if(p_style_docu_area->docu_area.whole_row || p_style_docu_area->docu_area.whole_col)
        arg_dispose(&arglist_handle, ARG_DOCU_AREA_TL_OBJECT_ID);
    else
        p_args[ARG_DOCU_AREA_TL_OBJECT_ID].val.s32 = p_style_docu_area->docu_area.tl.object_position.object_id;

    /* Bottom right */
    if(p_style_docu_area->docu_area.whole_row)
        arg_dispose(&arglist_handle, ARG_DOCU_AREA_BR_COL);
    else
        p_args[ARG_DOCU_AREA_BR_COL].val.s32 = p_style_docu_area->docu_area.br.slr.col;

    if(p_style_docu_area->docu_area.whole_col)
        arg_dispose(&arglist_handle, ARG_DOCU_AREA_BR_ROW);
    else
        p_args[ARG_DOCU_AREA_BR_ROW].val.s32 = p_style_docu_area->docu_area.br.slr.row;

    if( p_style_docu_area->docu_area.whole_row || p_style_docu_area->docu_area.whole_col ||
        (OBJECT_ID_NONE == p_style_docu_area->docu_area.br.object_position.object_id) )
        arg_dispose(&arglist_handle, ARG_DOCU_AREA_BR_DATA);
    else
        p_args[ARG_DOCU_AREA_BR_DATA].val.s32 = p_style_docu_area->docu_area.br.object_position.data;

    if(p_style_docu_area->docu_area.whole_row || p_style_docu_area->docu_area.whole_col)
        arg_dispose(&arglist_handle, ARG_DOCU_AREA_BR_OBJECT_ID);
    else
        p_args[ARG_DOCU_AREA_BR_OBJECT_ID].val.s32 = p_style_docu_area->docu_area.br.object_position.object_id;

    p_args[ARG_DOCU_AREA_WHOLE_COL].val.fBool = p_style_docu_area->docu_area.whole_col;
    p_args[ARG_DOCU_AREA_WHOLE_ROW].val.fBool = p_style_docu_area->docu_area.whole_row;

    switch(t5_message)
    {
    case T5_CMD_OF_BASE_IMPLIED_REGION:
    case T5_CMD_OF_IMPLIED_REGION:
        p_args[ARG_IMPLIED_OBJECT_ID].val.s32 = p_style_docu_area->object_message.object_id;
        p_args[ARG_IMPLIED_MESSAGE].val.t5_message = p_style_docu_area->object_message.t5_message;
        p_args[ARG_IMPLIED_ARG].val.s32 = p_style_docu_area->arg;
        p_args[ARG_IMPLIED_REGION_CLASS].val.u8n = p_style_docu_area->region_class;
        style_arg_no = ARG_IMPLIED_STYLE;
        break;

    default:
        style_arg_no = ARG_DOCU_AREA_DEFN;
        break;
    }

    {
    STYLE style;
    style_from_docu_area_no_indirection(p_docu_from_docno(p_of_op_format->docno), &style, p_style_docu_area);
    if(status_ok(status = style_ustr_inline_from_struct(p_docu_from_docno(p_of_op_format->docno), &quick_ublock, &style)))
        p_args[style_arg_no].val.ustr_inline = quick_ublock_ustr_inline(&quick_ublock);
    } /*block*/

    if(status_ok(status))
        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

    arglist_dispose(&arglist_handle);

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

_Check_return_
static STATUS
save_flow_object(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    if(p_of_op_format->of_template.data_class >= DATA_SAVE_MANY)
    {
        const OBJECT_ID object_id = OBJECT_ID_SKEL;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE arglist_handle;

        if(p_docu->flags.flow_installed)
        {
            if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_OF_FLOW, &p_construct_table)))
            {
                const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
                p_args[0].val.s32 = p_docu->object_id_flow;
                status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
                arglist_dispose(&arglist_handle);
            }
        }
    }

    return(status);
}

_Check_return_
static STATUS
save_numform_context(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    if(!p_of_op_format->of_template.numform_context)
        return(STATUS_OK);

    /* not hard-wired default? */
    if(p_docu->p_numform_context)
    {
        const OBJECT_ID object_id = OBJECT_ID_SKEL;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE arglist_handle;

        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_NUMFORM_DATA, &p_construct_table)))
        {
            U32 pass;
            U32 size = 0;
            U32 item_size;
            P_USTR ustr_out = NULL; /* keep dataflower happy */

            for(pass = 1; pass <= 2; ++pass)
            {
                P_NUMFORM_CONTEXT p_numform_context = p_docu->p_numform_context;

                {
                U32 month;

                for(month = 0; month < elemof32(p_numform_context->month_names); ++month)
                {
                    PC_USTR ustr_month_name = p_numform_context->month_names[month];

                    item_size = ustrlen32(ustr_month_name);

                    if(NULL == ustr_out /*pass == 1*/)
                    {
                        size += item_size;
                        ++size; /* for separator or CH_NULL terminator */
                    }
                    else /*if(pass == 2)*/
                    {
                        memcpy32(ustr_out, ustr_month_name, item_size);
                        ustr_IncBytes_wr(ustr_out, item_size);
                        PtrPutByte(ustr_out, CH_SPACE); /* separator */
                        ustr_IncByte_wr(ustr_out);
                    }
                }
                } /*block*/

                {
                U32 day;

                for(day = 0; day < elemof32(p_numform_context->day_endings); ++day)
                {
                    PC_USTR ustr_day_ending = p_numform_context->day_endings[day];

                    item_size = ustrlen32(ustr_day_ending);

                    if(NULL == ustr_out /*pass == 1*/)
                    {
                        size += item_size;
                        ++size; /* for separator or terminator */
                    }
                    else /*if(pass == 2)*/
                    {
                        memcpy32(ustr_out, ustr_day_ending, item_size);
                        ustr_IncBytes_wr(ustr_out, item_size);
                        PtrPutByte(ustr_out, CH_SPACE); /* separator */
                        ustr_IncByte_wr(ustr_out);
                    }
                }
                } /*block*/

                if(NULL == ustr_out /*pass == 1*/)
                {
                    const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);

                    if(NULL == (ustr_out = p_args->val.ustr_wr = al_ptr_alloc_bytes(P_USTR, size, &status)))
                        break;

                    assert(p_args->type == ARG_TYPE_USTR);
                    p_args->type = (ARG_TYPE) (ARG_TYPE_USTR | ARG_ALLOC);
                }
                else /*if(pass == 2)*/
                {
                    ustr_DecByte_wr(ustr_out);
                    PtrPutByte(ustr_out, CH_NULL); /* ensure it is terminated */
                }
            }

            if(status_ok(status))
                status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

            arglist_dispose(&arglist_handle);
        }
    }

    status_return(status);

    /* save out list of loaded numforms */
    if(array_elements(&p_docu->numforms))
    {
        const OBJECT_ID object_id = OBJECT_ID_SKEL;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE arglist_handle;

        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_NUMFORM_LOAD, &p_construct_table)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 2);
            ARRAY_INDEX i;

            for(i = 0; i < array_elements(&p_docu->numforms); ++i)
            {
                const PC_UI_NUMFORM p_ui_numform = array_ptrc(&p_docu->numforms, UI_NUMFORM, i);

                p_args[0].val.s32 = p_ui_numform->numform_class;
                p_args[1].val.ustr = p_ui_numform->ustr_numform;

                status_break(status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format));
            }

            arglist_dispose(&arglist_handle);
        }
    }

    return(status);
}

_Check_return_
static STATUS
save_all_styles(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STYLE_HANDLE style_handle = STYLE_HANDLE_ENUM_START;
    P_STYLE p_style;

    while(style_enum_styles(p_docu, &p_style, &style_handle) >= 0)
    {
        status_return(skeleton_save_style_handle(p_of_op_format, style_handle, FALSE));
    }

    return(STATUS_OK);
}

/* ask around for which styles are used in the docu_area being saved */

_Check_return_
static STATUS
save_some_styles(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;
    STYLE_USE_QUERY style_use_query;
    STYLE_HANDLE style_handle = STYLE_HANDLE_ENUM_START;
    P_STYLE p_style;

    style_use_query.docu_area    = p_of_op_format->save_docu_area;
    style_use_query.h_style_use  = 0;
    style_use_query.data_class   = (U8) p_of_op_format->of_template.data_class;
    style_use_query.region_class = (p_of_op_format->of_template.data_class <= DATA_SAVE_MANY) ? REGION_UPPER : REGION_END; /* avoid saving CurrentCell style etc. in CHARACTER/MANY */

    while(style_enum_styles(p_docu, &p_style, &style_handle) >= 0)
    {
        P_STYLE_USE_QUERY_ENTRY p_style_use_query_entry;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(*p_style_use_query_entry), 0);

        if(NULL == (p_style_use_query_entry = al_array_extend_by(&style_use_query.h_style_use, STYLE_USE_QUERY_ENTRY, 1, &array_init_block, &status)))
            break;

        p_style_use_query_entry->style_handle = style_handle;
        p_style_use_query_entry->use          = 0;
    }

    if(status_ok(status))
    {
        ARRAY_INDEX index;

        status_assert(maeve_event(p_docu, T5_MSG_STYLE_USE_QUERY, &style_use_query));

        for(index = 0; index < array_elements(&style_use_query.h_style_use); ++index)
        {
            P_STYLE_USE_QUERY_ENTRY p_style_use_query_entry;

            p_style_use_query_entry = array_ptr(&style_use_query.h_style_use, STYLE_USE_QUERY_ENTRY, index);

            if( ( p_style_use_query_entry->use && p_of_op_format->of_template.used_styles)   ||
                (!p_style_use_query_entry->use && p_of_op_format->of_template.unused_styles) )
            {
                status_break(status = skeleton_save_style_handle(p_of_op_format, p_style_use_query_entry->style_handle, FALSE));
            }
        }
    }

    al_array_dispose(&style_use_query.h_style_use);

    return(status);
}

_Check_return_
static STATUS
save_styles(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    if(p_of_op_format->of_template.used_styles && p_of_op_format->of_template.unused_styles)
    {
        status = save_all_styles(p_docu, p_of_op_format);
    }
    else if(p_of_op_format->of_template.used_styles || p_of_op_format->of_template.unused_styles)
    {
        status = save_some_styles(p_docu, p_of_op_format);
    }

    return(status);
}

_Check_return_
static STATUS
save_pre_save_1(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    status_return(skel_save_version(p_of_op_format));

    status_return(save_base_single_col(p_docu, p_of_op_format));

    status_return(save_rowtable(p_docu, p_of_op_format));

    status_return(save_flow_object(p_docu, p_of_op_format));

    status_return(save_block(p_docu, p_of_op_format));

    status_return(save_auto_save(p_docu, p_of_op_format));

    status_return(save_numform_context(p_docu, p_of_op_format));

    status_return(save_styles(p_docu, p_of_op_format));

    return(STATUS_OK);
}

_Check_return_
static STATUS
save_rowtable(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    if(p_of_op_format->of_template.data_class >= DATA_SAVE_DOC)
    {
        const OBJECT_ID object_id = OBJECT_ID_SKEL;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE arglist_handle;

        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_OF_ROWTABLE, &p_construct_table)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
            p_args[0].val.fBool = p_docu->flags.virtual_row_table;
            status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
            arglist_dispose(&arglist_handle);
        }
    }

    return(status);
}

/******************************************************************************
*
* save cell data in ownform
*
******************************************************************************/

_Check_return_
static STATUS
save_cell(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    P_SKEL_SAVE_CELL p_skel_save_cell)
{
    SAVE_CELL_OWNFORM save_cell_ownform;
    UCHARZ contents_buffer[256];
    UCHARZ formula_buffer[256];
    PC_USTR_INLINE ustr_inline_contents = NULL;
    PC_USTR ustr_formula = NULL;
    STATUS status;

    zero_struct(save_cell_ownform);

    save_cell_ownform.object_data = p_skel_save_cell->object_data;
    save_cell_ownform.p_of_op_format = p_of_op_format;

    quick_ublock_setup(&save_cell_ownform.contents_data_quick_ublock, contents_buffer);
    quick_ublock_setup(&save_cell_ownform.formula_data_quick_ublock, formula_buffer);

    status = object_call_id(p_skel_save_cell->object_data.object_id,
                            p_docu_from_docno(p_of_op_format->docno),
                            T5_MSG_SAVE_CELL_OWNFORM,
                            &save_cell_ownform);

    /* If we are returned an empty cell (nothing at all or just CH_NULL terminator), ignore this cell save */
    if(status_ok(status) && (0 != quick_ublock_bytes(&save_cell_ownform.contents_data_quick_ublock)))
        if(status_ok(status = quick_ublock_nullch_add(&save_cell_ownform.contents_data_quick_ublock)))
            ustr_inline_contents = quick_ublock_ustr_inline(&save_cell_ownform.contents_data_quick_ublock);

    if(status_ok(status) && (0 != quick_ublock_bytes(&save_cell_ownform.formula_data_quick_ublock)))
        if(status_ok(status = quick_ublock_nullch_add(&save_cell_ownform.formula_data_quick_ublock)))
            ustr_formula = quick_ublock_ustr(&save_cell_ownform.formula_data_quick_ublock);

    if(status_ok(status))
    if((NULL != ustr_inline_contents) || (NULL != ustr_formula))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&p_skel_save_cell->arglist_handle, ARG_CELL_N_ARGS);

        /* Above object call should have updated data_type character from calling object to save cell contents */
        p_args[ARG_CELL_OWNER].val.u8c = construct_id_from_object_id(p_skel_save_cell->object_data.object_id);
        p_args[ARG_CELL_DATA_TYPE].val.u8c = construct_id_from_data_type(save_cell_ownform.data_type);

        p_args[ARG_CELL_COL].val.s32 = p_skel_save_cell->object_data.data_ref.arg.slr.col;
        p_args[ARG_CELL_ROW].val.s32 = p_skel_save_cell->object_data.data_ref.arg.slr.row;

        p_args[ARG_CELL_CONTENTS].val.ustr_inline = ustr_inline_contents;
        p_args[ARG_CELL_FORMULA ].val.ustr = ustr_formula;
        p_args[ARG_CELL_MROFMUN ].val.tstr = save_cell_ownform.tstr_mrofmun_style;

        status = ownform_save_arglist(p_skel_save_cell->arglist_handle, OBJECT_ID_SKEL, p_skel_save_cell->p_construct, p_of_op_format);
    }

    quick_ublock_dispose(&save_cell_ownform.contents_data_quick_ublock);
    quick_ublock_dispose(&save_cell_ownform.formula_data_quick_ublock);

    return(status);
}

_Check_return_
extern STATUS
skel_save_version(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    if(p_of_op_format->of_template.version)
    {
        const OBJECT_ID object_id = OBJECT_ID_SKEL;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE arglist_handle;

        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_OF_VERSION, &p_construct_table)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 6);
            TCHARZ version_buffer[64];
            TCHARZ user_buffer[128];
            TCHARZ encoding_buffer[64];
            PCTSTR tstr;
            
            tstr_xstrkpy(user_buffer, elemof32(user_buffer), user_id());

            tstr = user_organ_id();
            if(CH_NULL != tstr[0])
            {
                tstr_xstrkat(user_buffer, elemof32(user_buffer), TEXT(" - "));
                tstr_xstrkat(user_buffer, elemof32(user_buffer), tstr);
            }

            resource_lookup_tstr_buffer(version_buffer, elemof32(version_buffer), MSG_SKEL_VERSION);

            encoding_buffer[0] = CH_NULL;

            p_args[0].val.tstr = version_buffer;
            p_args[1].val.tstr = resource_lookup_tstr(MSG_SKEL_DATE);
            p_args[2].val.tstr = product_id();
            p_args[3].val.tstr = user_buffer;
            p_args[4].val.tstr = registration_number();
            p_args[5].val.tstr = encoding_buffer;

#if WINDOWS
            consume_int(tstr_xsnprintf(encoding_buffer, elemof(encoding_buffer), TEXT("Windows-") U32_TFMT, get_system_codepage())); /* e.g. Windows-1252 */
#elif RISCOS
            switch(get_system_codepage())
            {
            case SBCHAR_CODEPAGE_ALPHABET_LATIN1:
                tstr_xstrkpy(encoding_buffer, elemof(encoding_buffer), TEXT("Alphabet-Latin1"));
                break;

            case SBCHAR_CODEPAGE_ALPHABET_LATIN2:
                tstr_xstrkpy(encoding_buffer, elemof(encoding_buffer), TEXT("Alphabet-Latin2"));
                break;

            case SBCHAR_CODEPAGE_ALPHABET_LATIN3:
                tstr_xstrkpy(encoding_buffer, elemof(encoding_buffer), TEXT("Alphabet-Latin3"));
                break;

            case SBCHAR_CODEPAGE_ALPHABET_LATIN4:
                tstr_xstrkpy(encoding_buffer, elemof(encoding_buffer), TEXT("Alphabet-Latin4"));
                break;

            case SBCHAR_CODEPAGE_ALPHABET_LATIN9:
                tstr_xstrkpy(encoding_buffer, elemof(encoding_buffer), TEXT("Alphabet-Latin9"));
                break;

            default:
                break;
            }
#endif

            status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

            arglist_dispose(&arglist_handle);
        }
    }

    return(status);
}

_Check_return_
static STATUS
skeleton_save_construct_convert_IL_STYLE_HANDLE(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_SAVE_CONSTRUCT_OWNFORM p_save_inline_ownform,
    _InVal_     STYLE_HANDLE style_handle)
{
    STATUS status = STATUS_OK;
    STYLE_SELECTOR selector;
    STYLE style;
    PCTSTR style_name;

    style_selector_clear(&selector);
    style_selector_bit_set(&selector, STYLE_SW_NAME);

    style_init(&style);
    style_struct_from_handle(p_docu, &style, style_handle, &selector);

    style_name = array_tstr(&style.h_style_name_tstr);

    status = arg_alloc_tstr(&p_save_inline_ownform->arglist_handle, 0, style_name);

    style_dispose(&style);

    return(status);
}

_Check_return_
static STATUS
skeleton_save_construct_convert_IL_STYLE_PS_TAB_LIST(
    _InRef_     P_SAVE_CONSTRUCT_OWNFORM p_save_inline_ownform,
    _In_        PC_TAB_INFO buffer,
    _InVal_     U32 il_data_size)
{
    STATUS status = STATUS_OK;
    const P_ARGLIST_ARG p_args = p_arglist_args(&p_save_inline_ownform->arglist_handle, 1);
    const U32 n_tab_info = il_data_size / sizeof32(TAB_INFO);
    int pass;
    U32 size = 0; /* accumulate on first pass */
    P_USTR ustr_out = NULL; /* allocate at end of first pass */

    if(0 == n_tab_info)
        return(status);

    for(pass = 1; pass <= 2; ++pass)
    {
        U32 i = 0;

        do  {
            static const char ownform_tab_types[] = { 'L', 'C', 'R', 'D' };

            const PC_TAB_INFO info = &buffer[i];
            const TAB_TYPE tab_type = info->type;
            const S32 offset = info->offset;
            U8Z tab_buffer[32];
            U32 len;

            assert(info->type < elemof32(ownform_tab_types));
            len = xsnprintf(tab_buffer, elemof32(tab_buffer), "%c" S32_FMT " ", ownform_tab_types[tab_type], offset);

            if(NULL == ustr_out /*pass == 1*/)
                size += len;
            else
            {
                memcpy32(ustr_out, tab_buffer, len);
                ustr_IncBytes_wr(ustr_out, len);
            }
        }
        while(++i < n_tab_info);

        if(NULL == ustr_out /*pass == 1*/)
        {
            if(NULL == (ustr_out = al_ptr_alloc_bytes(P_USTR, size, &status)))
                break;

            p_args[0].type = (ARG_TYPE) (ARG_TYPE_USTR | ARG_ALLOC);
            p_args[0].val.ustr = ustr_out;
        }
        else
        {
            PtrPutByteOff(ustr_out, -1, CH_NULL); /* replace last separator with CH_NULL terminator */
        }
    }

    return(status);
}

static STATUS
skeleton_save_construct_convert_IL_UTF8(
    _InRef_     P_SAVE_CONSTRUCT_OWNFORM p_save_inline_ownform,
    _In_reads_bytes_(il_data_size) PC_UTF8 utf8,
    _InVal_     U32 il_data_size)
{
    STATUS status = STATUS_OK;
    const P_ARGLIST_ARG p_args = p_arglist_args(&p_save_inline_ownform->arglist_handle, 1);
    int pass = 1;
    U32 size = 0; /* accumulate on first pass */
    P_USTR ustr_out = NULL; /* allocate at end of first pass */

    if(0 == il_data_size)
        return(status);

    for(pass = 1; pass <= 2; ++pass)
    {
        U32 offset = 0;

        while(offset < il_data_size)
        {
            U32 bytes_of_char;
            const UCS4 ucs4 = utf8_char_decode_off(utf8, offset, bytes_of_char);
            U8Z out_buffer[32];
            U32 len;

            len = xsnprintf(out_buffer, elemof32(out_buffer), "%04X ", ucs4);

            if(NULL == ustr_out /*pass == 1*/)
                size += len;
            else
            {
                memcpy32(ustr_out, out_buffer, len);
                ustr_IncBytes_wr(ustr_out, len);
            }

            offset += bytes_of_char;
        }

        if(NULL == ustr_out /*pass == 1*/)
        {
            if(NULL == (ustr_out = al_ptr_alloc_bytes(P_USTR, size, &status)))
                break;

            p_args[0].type = (ARG_TYPE) (ARG_TYPE_USTR | ARG_ALLOC);
            p_args[0].val.ustr = ustr_out;
        }
        else
        {
            PtrPutByteOff(ustr_out, -1, CH_NULL); /* replace last separator with CH_NULL terminator */
        }
    }

    return(status);
}

_Check_return_
extern STATUS
skeleton_save_construct(
    _DocuRef_   P_DOCU p_docu,
    P_SAVE_CONSTRUCT_OWNFORM p_save_inline_ownform)
{
    STATUS status = STATUS_OK;
    PC_USTR_INLINE ustr_inline = p_save_inline_ownform->ustr_inline;
    const IL_CODE il_code = inline_code(ustr_inline);
    const U32 il_data_size = (U32) inline_data_size(ustr_inline);
    PC_CONSTRUCT_TABLE p_construct_table;
    union SAVE_CONVERSION_BUFFERS
    {
        LINE_SPACE   line_space;
        RGB          rgb;
        STYLE_HANDLE style_handle;
        TAB_INFO     tab_info[OF_DATA_MAX_BUF/sizeof32(TAB_INFO)];
        UTF8B        utf8[OF_DATA_MAX_BUF];
        S32          s32;
        U8           u8n;
        U8           u8[OF_DATA_MAX_BUF];
    } buffer;

    data_from_inline(&buffer, sizeof32(buffer), ustr_inline, inline_data_type(ustr_inline));

    status_return(arglist_prepare_with_construct(&p_save_inline_ownform->arglist_handle, OBJECT_ID_SKEL, (T5_MESSAGE) il_code, &p_construct_table));

    if(p_construct_table->bits.exceptional)
    {
        switch(il_code)
        {
        case IL_STYLE_HANDLE:
            status = skeleton_save_construct_convert_IL_STYLE_HANDLE(p_docu, p_save_inline_ownform, buffer.style_handle);
            break;

        case IL_STYLE_KEY:
            {
            PCTSTR key_name = key_of_name_from_key_code((KMAP_CODE) buffer.s32);
            status = arg_alloc_tstr(&p_save_inline_ownform->arglist_handle, 0, key_name ? key_name : tstr_empty_string);
            break;
            }

        case IL_STYLE_PS_TAB_LIST:
            status = skeleton_save_construct_convert_IL_STYLE_PS_TAB_LIST(p_save_inline_ownform, buffer.tab_info, il_data_size);
            break;

        case IL_STYLE_PS_LINE_SPACE:
            {
            const P_ARGLIST_ARG p_args = p_arglist_args(&p_save_inline_ownform->arglist_handle, 2);
            p_args[0].val.s32 = buffer.line_space.type;
            p_args[1].val.s32 = buffer.line_space.leading;
            break;
            }

        case IL_UTF8:
            status = skeleton_save_construct_convert_IL_UTF8(p_save_inline_ownform, utf8_bptr(buffer.utf8), il_data_size);
            break;

        default: default_unhandled();
#if CHECKING
        case IL_STYLE_FS_COLOUR:
        case IL_STYLE_PS_RGB_BACK:
        case IL_STYLE_PS_RGB_BORDER:
        case IL_STYLE_PS_RGB_GRID_LEFT:
        case IL_STYLE_PS_RGB_GRID_TOP:
        case IL_STYLE_PS_RGB_GRID_RIGHT:
        case IL_STYLE_PS_RGB_GRID_BOTTOM:
#endif
            {
            const P_ARGLIST_ARG p_args = p_arglist_args(&p_save_inline_ownform->arglist_handle, 2);

            p_args[0].val.s32 = buffer.rgb.r;
            p_args[1].val.s32 = buffer.rgb.g;
            p_args[2].val.s32 = buffer.rgb.b;

            if(buffer.rgb.transparent) /* possibly superfluous arg */
                p_args[3].val.s32 = buffer.rgb.transparent;
            else
                arg_dispose(&p_save_inline_ownform->arglist_handle, 3);

            break;
            }
        }
    }
    else
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&p_save_inline_ownform->arglist_handle, 1);

        switch(inline_data_type(ustr_inline))
        {
        default: default_unhandled();
#if CHECKING
            break;
        case IL_TYPE_USTR:
#endif
            status = arg_alloc_ustr(&p_save_inline_ownform->arglist_handle, 0, (PC_USTR) buffer.u8);
            break;

        case IL_TYPE_U8:
            p_args[0].val.u8n = buffer.u8n;
            break;

        case IL_TYPE_S32:
            p_args[0].val.s32 = buffer.s32;
            break;
        }
    }

    if(status_fail(status))
        arglist_dispose(&p_save_inline_ownform->arglist_handle);
    else
    {
        p_save_inline_ownform->p_construct = p_construct_table;
        p_save_inline_ownform->object_id = OBJECT_ID_SKEL;
    }

    return(status);
}

_Check_return_
extern STATUS
skeleton_save_style_handle(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     STYLE_HANDLE style_handle,
    _InVal_     BOOL part_save)
{
    const P_DOCU p_docu = p_docu_from_docno(p_of_op_format->docno);
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;
    STYLE style;

    style_init(&style);

    style_struct_from_handle(p_docu, &style, style_handle, &style_selector_all);

    assert(style_bit_test(&style, STYLE_SW_NAME));
    style_selector_bit_clear(&style.selector, STYLE_SW_NAME);

    {
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 500);
    quick_ublock_with_buffer_setup(quick_ublock);

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_OF_STYLE, &p_construct_table)))
    {
        if(status_ok(status = style_ustr_inline_from_struct(p_docu, &quick_ublock, &style)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 2);
            p_args[0].val.tstr = array_tstr(&style.h_style_name_tstr);
            p_args[1].val.ustr_inline = quick_ublock_ustr_inline(&quick_ublock);
            status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
        }

        quick_ublock_dispose(&quick_ublock);

        arglist_dispose(&arglist_handle);
    }

    status_return(status);
    } /*block*/

    if(p_of_op_format->of_template.data_class >= DATA_SAVE_DOC)
    {
        /* ************************* old file compatibility *********************** */

        /* save out base style construct */
        if(style_handle == style_handle_base(&p_docu->h_style_docu_area))
        {
            if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_OF_STYLE_BASE, &p_construct_table)))
            {
                const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
                p_args[0].val.tstr = array_tstr(&style.h_style_name_tstr); /* loan */
                status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
                p_args[0].val.tstr = NULL; /* reclaim */
                arglist_dispose(&arglist_handle);
            }
        }

        /* save out current style construct */
        if(style_handle == style_handle_current(&p_docu->h_style_docu_area))
        {
            if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_OF_STYLE_CURRENT, &p_construct_table)))
            {
                const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
                p_args[0].val.tstr = array_tstr(&style.h_style_name_tstr); /* loan */
                status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
                p_args[0].val.tstr = NULL; /* reclaim */
                arglist_dispose(&arglist_handle);
            }
        }

        /* ********************* save implied regions with a style ********************* */

        if(part_save)
        {
            const ARRAY_INDEX n_regions = array_elements(&p_docu->h_style_docu_area);
            ARRAY_INDEX style_docu_area_ix;

            for(style_docu_area_ix = n_regions - 1; style_docu_area_ix >= 0; --style_docu_area_ix)
            {
                const PC_STYLE_DOCU_AREA p_style_docu_area = array_ptrc(&p_docu->h_style_docu_area, STYLE_DOCU_AREA, style_docu_area_ix);

                if( (style_handle == p_style_docu_area->style_handle)
                    &&
                    (OBJECT_ID_NONE != p_style_docu_area->object_message.object_id))
                    status_break(status = save_style_docu_area(p_of_op_format, p_style_docu_area));
            }
        }

        /* ************************************************************************ */

    }

    return(status);
}

/******************************************************************************
*
* save clipboard data to file
*
******************************************************************************/

_Check_return_
extern STATUS
clip_data_save(
    _In_z_      PCTSTR filename,
    _InRef_     PC_ARRAY_HANDLE p_h_clip_data)
{
    const U32 n_bytes = array_elements32(p_h_clip_data);
    PC_BYTE p_data = array_rangec(p_h_clip_data, BYTE, 0, n_bytes);
    P_DOCU p_docu = P_DOCU_NONE;
    OF_OP_FORMAT of_op_format = OF_OP_FORMAT_INIT;
    STATUS status;

    if(0 == n_bytes)
        return(STATUS_OK);

    of_op_format.process_status.flags.foreground = 1;

    status_return(ownform_initialise_save(p_docu, &of_op_format, NULL, filename, FILETYPE_T5_FIREWORKZ, NULL));

    status = binary_write_bytes(&of_op_format.output, p_data, n_bytes);

    return(ownform_finalise_save(&p_docu, &of_op_format, status));
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_clipboard)
{
    const PC_ARGLIST_ARG p_args = p_arglist_args(&p_t5_cmd->arglist_handle, 1);
    PCTSTR filename = p_args[0].val.tstr;
    ARRAY_HANDLE h_local_clip_data = local_clip_data_query();

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    return(clip_data_save(filename, &h_local_clip_data));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_clipboard)

/*
main events
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_save);

_Check_return_
static STATUS
sk_save_msg_save(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_SAVE p_msg_save)
{
    switch(p_msg_save->t5_msg_save_message)
    {
    case T5_MSG_SAVE__PRE_SAVE_1:
        return(save_pre_save_1(p_docu, p_msg_save->p_of_op_format));

    case T5_MSG_SAVE__DATA_SAVE_2:
        return(save_data_save_2(p_docu, p_msg_save->p_of_op_format));

    default:
        return(STATUS_OK);
    }
}

MAEVE_EVENT_PROTO(static, maeve_event_sk_save)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_SAVE:
        return(sk_save_msg_save(p_docu, (PC_MSG_SAVE) p_data));

    default:
        return(STATUS_OK);
    }
}

/*
exported services hook
*/

T5_MSG_PROTO(static, sk_save_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT1:
        return(maeve_event_handler_add(p_docu, maeve_event_sk_save, (CLIENT_HANDLE) 0));

    case T5_MSG_IC__CLOSE1:
        maeve_event_handler_del(p_docu, maeve_event_sk_save, (CLIENT_HANDLE) 0);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_save)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(sk_save_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of sk_save.c */
