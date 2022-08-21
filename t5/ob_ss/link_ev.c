/* link_ev.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Link routines required and used by evaluator module */

/* MRJC January 1991 / May 1992 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

H_DIALOG h_alert_dialog = 0;
S32 alert_result = ALERT_RESULT_NONE;

H_DIALOG h_input_dialog = 0;
S32 input_result = ALERT_RESULT_NONE;
UI_TEXT ui_text_input;

_Check_return_
extern STATUS
ev_alert(
    _InVal_     EV_DOCNO ev_docno,
    _InRef_     PC_EV_STRINGC p_ev_string_message,
    _InRef_     PC_EV_STRINGC p_ev_string_but_1,
    _InRef_     PC_EV_STRINGC p_ev_string_but_2)
{
    SS_INPUT_EXEC ss_input_exec;
    ss_input_exec.ev_docno = ev_docno;
    ss_input_exec.p_ev_string_message = p_ev_string_message;
    ss_input_exec.p_ev_string_but_1 = p_ev_string_but_1;
    ss_input_exec.p_ev_string_but_2 = p_ev_string_but_2;
    return(object_call_id_load(P_DOCU_NONE, T5_MSG_SS_ALERT_EXEC, &ss_input_exec, OBJECT_ID_SS_SPLIT));
}

/******************************************************************************
*
* close alert dialog
*
******************************************************************************/

extern void
ev_alert_close(void)
{
    DIALOG_CMD_DISPOSE_DBOX dialog_cmd_dispose_dbox;
    dialog_cmd_dispose_dbox.h_dialog = h_alert_dialog;
    h_alert_dialog = 0;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_DISPOSE_DBOX, &dialog_cmd_dispose_dbox));
}

/******************************************************************************
*
* ask about status of alert dialog box
*
******************************************************************************/

_Check_return_
extern STATUS
ev_alert_poll(void)
{
    STATUS status = STATUS_FAIL;

    switch(alert_result)
    {
    default: default_unhandled();
#if CHECKING
    case ALERT_RESULT_NONE:
#endif
        break;

    case ALERT_RESULT_CLOSE:
        status = 0;
        break;

    case ALERT_RESULT_BUT_1:
        status = STATUS_DONE;
        break;

    case ALERT_RESULT_BUT_2:
        status = 2;
        break;
    }

    return(status);
}

_Check_return_
extern STATUS
ev_input(
    _InVal_     EV_DOCNO ev_docno,
    _InRef_     PC_EV_STRINGC p_ev_string_message,
    _InRef_     PC_EV_STRINGC p_ev_string_but_1,
    _InRef_     PC_EV_STRINGC p_ev_string_but_2)
{
    SS_INPUT_EXEC ss_input_exec;
    ss_input_exec.ev_docno = ev_docno;
    ss_input_exec.p_ev_string_message = p_ev_string_message;
    ss_input_exec.p_ev_string_but_1 = p_ev_string_but_1;
    ss_input_exec.p_ev_string_but_2 = p_ev_string_but_2;
    return(object_call_id_load(P_DOCU_NONE, T5_MSG_SS_INPUT_EXEC, &ss_input_exec, OBJECT_ID_SS_SPLIT));
}

/******************************************************************************
*
* close input dialog
*
******************************************************************************/

extern void
ev_input_close(void)
{
    DIALOG_CMD_DISPOSE_DBOX dialog_cmd_dispose_dbox;
    ui_text_dispose(&ui_text_input);
    dialog_cmd_dispose_dbox.h_dialog = h_input_dialog;
    h_input_dialog = 0;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_DISPOSE_DBOX, &dialog_cmd_dispose_dbox));
}

/******************************************************************************
*
* ask about status of input dialog box
*
******************************************************************************/

_Check_return_
extern STATUS
ev_input_poll(
    _Out_writes_z_(elemof_buffer) P_USTR buffer,
    _InVal_     U32 elemof_buffer)
{
    STATUS status = STATUS_FAIL;

    assert(0 != elemof_buffer);
    PtrPutByte(buffer, CH_NULL);

    switch(input_result)
    {
    default: default_unhandled();
#if CHECKING
    case ALERT_RESULT_NONE:
#endif
        break;

    case ALERT_RESULT_CLOSE:
        status = 0;
        break;

    case ALERT_RESULT_BUT_1:
    case ALERT_RESULT_BUT_2:
        ustr_xstrkpy(buffer, elemof_buffer, ui_text_ustr(&ui_text_input));
        status = (input_result == ALERT_RESULT_BUT_2) ? 2 : STATUS_DONE;
        break;
    }

    return(status);
}

extern void
ev_current_cell(
    _OutRef_    P_EV_SLR p_ev_slr)
{
    const EV_DOCNO ev_docno = ev_current_docno();

    ev_slr_init(p_ev_slr);

    if(DOCNO_NONE != ev_docno)
    {
        const PC_DOCU p_docu = p_docu_from_ev_docno(ev_docno);

        ev_slr_docno_set(p_ev_slr, ev_docno);
        ev_slr_col_set(p_ev_slr, p_docu->cur.slr.col);
        ev_slr_row_set(p_ev_slr, p_docu->cur.slr.row);

        return;
    }
}

_Check_return_
extern EV_DOCNO
ev_current_docno(void)
{
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
    {
        const PC_DOCU p_docu = p_docu_from_docno_valid(docno);

        if(p_docu->flags.is_current)
            return((EV_DOCNO) docno);
    }

    return(DOCNO_NONE);
}

extern void
ev_double_click(
    _OutRef_    P_EV_SLR p_ev_slr_out,
    _InRef_     PC_EV_SLR p_ev_slr_in)
{
    const P_DOCU p_docu = p_docu_from_ev_docno(ev_slr_docno(p_ev_slr_in));

    *p_ev_slr_out = p_object_instance_data_SS(p_docu)->ev_slr_double_click;
}

_Check_return_
extern BOOL
ev_event_occurred(
    _InVal_     EV_DOCNO ev_docno,
    _InVal_     EVENT_TYPE event_type)
{
    BOOL res = FALSE;
    const ARRAY_INDEX n_events = array_elements(&event_use_deptable.h_table);
    ARRAY_INDEX event_idx;
    PC_EVENT_USE p_event_use = array_rangec(&event_use_deptable.h_table, EVENT_USE, 0, n_events);

    for(event_idx = 0; event_idx < n_events; ++event_idx, ++p_event_use)
    {
        if(p_event_use->flags.tobedel)
            continue;

        if(p_event_use->event_type != event_type)
            continue;

        if(ev_slr_docno(&p_event_use->slr_by) == ev_docno)
        {
            ev_todo_add_slr(&p_event_use->slr_by);
            res = TRUE;
        }
    }

    return(res);
}

/******************************************************************************
*
* close down evaluator, freeing resources
*
******************************************************************************/

extern void
ev_exit(void)
{
    stack_free();
    todo_exit();
#if 0
    al_array_dispose(&h_event_list);
#endif
}

/******************************************************************************
*
* given a docu_name (which may be blank), establish a document number
*
******************************************************************************/

_Check_return_
extern EV_DOCNO
ev_establish_docno_from_docu_name(
    _InoutRef_  P_DOCU_NAME p_docu_name,
    _InVal_     EV_DOCNO ev_docno_from)
{
    EV_DOCNO ev_docno = ev_docno_from;

    if(!name_is_blank(p_docu_name))
    {
        const P_DOCU p_docu_from = p_docu_from_ev_docno(ev_docno_from);

        status_assert(name_ensure_path(p_docu_name, &p_docu_from->docu_name));

        ev_docno = docno_establish_docno_from_name(p_docu_name);
    }

    return(ev_docno);
}

/******************************************************************************
*
* return data from another object type to the spreadsheet object
*
******************************************************************************/

extern void
ev_external_data(
    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_SLR p_ev_slr)
{
    const P_DOCU p_docu = p_docu_from_ev_docno(ev_slr_docno(p_ev_slr));

    ev_data_set_blank(p_ev_data_out);

    if(p_docu->flags.has_data)
    {
        SLR slr;
        OBJECT_DATA_READ object_data_read;
        slr.col = ev_slr_col(p_ev_slr);
        slr.row = ev_slr_row(p_ev_slr);
        status_consume(object_data_from_slr(p_docu, &object_data_read.object_data, &slr));
        if(status_ok(object_call_id(object_data_read.object_data.object_id,
                                    p_docu,
                                    T5_MSG_OBJECT_DATA_READ,
                                    &object_data_read)))
        {
            if(RPN_DAT_STRING == object_data_read.ev_data.did_num)
            {
                if(ss_string_is_blank(&object_data_read.ev_data))
                {
                    ss_data_free_resources(&object_data_read.ev_data);
                    ev_data_set_blank(&object_data_read.ev_data);
                }
            }

            *p_ev_data_out = object_data_read.ev_data;
        }
    }
}

/******************************************************************************
*
* read some data from a field id
*
******************************************************************************/

static inline void
field_data_query_init(
    _OutRef_    P_FIELD_DATA_QUERY p_field_data_query,
    _InVal_     ARRAY_INDEX name_num,
    _InVal_     S32 record_no)
{
    P_EV_NAME p_ev_name = array_ptr(&name_def.h_table, EV_NAME, name_num);

    p_field_data_query->p_compound_name = p_ev_name->ustr_name_id;
    p_field_data_query->record_no = record_no;
    p_field_data_query->docno = (DOCNO) ev_slr_docno(&p_ev_name->owner);
    ev_data_set_blank(&p_field_data_query->ev_data);
}

extern void
ev_field_data_read(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InVal_     EV_HANDLE ev_handle,
    _InVal_     S32 iy)
{
    ARRAY_INDEX name_num = name_def_find(ev_handle);
    FIELD_DATA_QUERY field_data_query;

    if(name_num < 0)
    {
        ev_data_set_error(p_ev_data_out, create_error(EVAL_ERR_DATABASE));
        return;
    }

    field_data_query_init(&field_data_query, name_num, iy);

    if(status_fail(object_call_id(OBJECT_ID_REC, P_DOCU_NONE, T5_MSG_FIELD_DATA_READ, &field_data_query)))
    {
        ev_data_set_error(p_ev_data_out, create_error(EVAL_ERR_DATABASE));
        return;
    }

    *p_ev_data_out = field_data_query.ev_data;
}

/******************************************************************************
*
* say how many records there are for a given field
*
******************************************************************************/

_Check_return_
extern STATUS
ev_field_n_records(
    _InVal_     EV_HANDLE ev_handle,
    _OutRef_    P_S32 p_n_records_out)
{
    ARRAY_INDEX name_num = name_def_find(ev_handle);
    FIELD_DATA_QUERY field_data_query;

    *p_n_records_out = 0;

    if(name_num < 0)
        return(create_error(EVAL_ERR_DATABASE));

    field_data_query_init(&field_data_query, name_num, 0);

    if(status_fail(object_call_id(OBJECT_ID_REC, P_DOCU_NONE, T5_MSG_FIELD_DATA_N_RECORDS, &field_data_query)))
        return(create_error(EVAL_ERR_DATABASE));

    *p_n_records_out = field_data_query.record_no;

    return(STATUS_OK);
}

/******************************************************************************
*
* look thru all names for references to database fields
*
******************************************************************************/

extern void
ev_field_names_check(
    P_NAME_UREF p_name_uref)
{
    const ARRAY_INDEX n_elements = array_elements(&name_def.h_table);
    ARRAY_INDEX i;
    P_EV_NAME p_ev_name = array_range(&name_def.h_table, EV_NAME, 0, n_elements);

    for(i = 0; i < n_elements; ++i, ++p_ev_name)
    {
        if(p_ev_name->flags.tobedel)
            continue;

        if((PtrGetByte(p_ev_name->ustr_name_id) == CH_QUESTION_MARK)
           &&
           (p_name_uref->docno == ev_slr_docno(&p_ev_name->owner)))
        {
            DATA_REF_NAME_COMPARE data_ref_name_compare;
            data_ref_name_compare.docno = p_name_uref->docno;
            data_ref_name_compare.data_ref = p_name_uref->data_ref;
            data_ref_name_compare.p_compound_name = p_ev_name->ustr_name_id;
            if(status_ok(object_call_id(OBJECT_ID_REC, P_DOCU_NONE, T5_MSG_DATA_REF_NAME_COMPARE, &data_ref_name_compare)))
                if(!data_ref_name_compare.result)
                    ev_todo_add_name_dependents(p_ev_name->handle);
        }
    }
}

/******************************************************************************
*
* make a cell for the evaluator and store the given result in it
*
******************************************************************************/

_Check_return_
extern STATUS
ev_make_cell(
    _InRef_     PC_EV_SLR p_ev_slr,
    P_EV_DATA p_ev_data)
{
    STATUS status = STATUS_OK;
    const P_DOCU p_docu = p_docu_from_ev_docno(ev_slr_docno(p_ev_slr));
    SLR slr;
    S32 travel_res;

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_MODULE_EVAL))
    {
        TCHARZ tstr_buf[32 + BUF_EV_LONGNAMLEN];
        ev_trace_slr_tstr_buf(tstr_buf, elemof32(tstr_buf), TEXT("ev_make_cell $$"), p_ev_slr);
        trace_v0(TRACE_MODULE_EVAL, tstr_buf);
    }
#endif

    slr_from_ev_slr(&slr, p_ev_slr);

    if(slr.col >= n_cols_logical(p_docu) || slr.row >= n_rows(p_docu))
        return(create_error(EVAL_ERR_BEYONDEXTENTS));

#if 0
    /* tell dependents about it
     * can't do this - blows up evaluator (stack_zap etc)
     * and starts again resulting in endless loop
     */
    {
    UREF_PARMS uref_parms;
    region_from_two_slrs(&uref_parms.source.region, &slr, &slr, TRUE);
    uref_event(p_docu, T5_MSG_UREF_OVERWRITE, &uref_parms);
    } /*block*/
#endif

    {
    P_EV_CELL p_ev_cell;

    travel_res = ev_travel(&p_ev_cell, p_ev_slr);

    /* make the object non-null for now */
    if(status_ok(status = object_realloc(p_docu, (P_P_ANY) &p_ev_cell, &slr, OBJECT_ID_SS, OVH_EV_CELL)))
    {
        zero_struct(p_ev_cell->parms);
        ev_cell_constant_from_data(p_ev_cell, p_ev_data);
        p_ev_cell->parms.data_only = 1;
        docu_modify(p_docu);
    }
    } /*block*/

    if(travel_res < 0)
        reformat_from_row(p_docu, slr.row, REFORMAT_Y);

    return(status);
}

/******************************************************************************
*
* return number of cols in a doc
*
******************************************************************************/

_Check_return_
extern EV_COL
ev_numcol(
    _InVal_     EV_DOCNO ev_docno)
{
    const PC_DOCU p_docu = p_docu_from_ev_docno(ev_docno);

    if(p_docu->flags.has_data)
        return((EV_COL) n_cols_logical(p_docu));

    return(0);
}

/******************************************************************************
*
* return number of cols in a doc
*
******************************************************************************/

_Check_return_
extern EV_COL
ev_numcol_phys(
    _InVal_     EV_DOCNO ev_docno)
{
    const PC_DOCU p_docu = p_docu_from_ev_docno(ev_docno);

    if(p_docu->flags.has_data)
        return((EV_COL) n_cols_physical(p_docu));

    return(0);
}

/******************************************************************************
*
* convert a value to a string
*
******************************************************************************/

_Check_return_
extern STATUS
ev_numform(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _In_z_      PC_USTR ustr_format,
    _InRef_     PC_EV_DATA p_ev_data)
{
    NUMFORM_PARMS numform_parms;

    /*zero_struct(numform_parms);*/
    numform_parms.ustr_numform_numeric =
    numform_parms.ustr_numform_datetime =
    numform_parms.ustr_numform_texterror = ustr_format;
    numform_parms.p_numform_context = get_p_numform_context(P_DOCU_NONE);

    return(numform(p_quick_ublock, P_QUICK_TBLOCK_NONE, p_ev_data, &numform_parms));
}

/******************************************************************************
*
* return number of rows for given doc, col
*
******************************************************************************/

_Check_return_
extern EV_ROW
ev_numrow(
    _InVal_     EV_DOCNO ev_docno)
{
    const PC_DOCU p_docu = p_docu_from_ev_docno(ev_docno);

    if(p_docu->flags.has_data)
        return(n_rows(p_docu));

    return(0);
}

/******************************************************************************
*
* get pointer to spreadsheet instance data
*
******************************************************************************/

_Check_return_
_Ret_maybenone_
extern P_SS_DOC
ev_p_ss_doc_from_docno(
    _InVal_     EV_DOCNO ev_docno)
{
    const P_DOCU p_docu = p_docu_from_ev_docno(ev_docno); /* SKS 06jan95 this could well have been it !!! */

    if(IS_DOCU_NONE(p_docu))
        return(P_SS_DOC_NONE);

    return(&p_object_instance_data_SS(p_docu)->ss_doc);
}

/******************************************************************************
*
* return the last page numbers
*
******************************************************************************/

_Check_return_
extern S32
ev_page_last(
    _InVal_     EV_DOCNO ev_docno,
    _InVal_     BOOL xy)
{
    const P_DOCU p_docu = p_docu_from_ev_docno(ev_docno);

    return(xy ? last_page_y(p_docu) : last_page_x(p_docu));
}

/******************************************************************************
*
* return the page number of a cell
*
******************************************************************************/

_Check_return_
extern STATUS
ev_page_slr(
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     BOOL xy)
{
    const EV_DOCNO ev_docno = ev_slr_docno(p_ev_slr);

    if( (ev_slr_col(p_ev_slr) < ev_numcol(ev_docno)) &&
        (ev_slr_row(p_ev_slr) < ev_numrow(ev_docno)) )
    {
        const P_DOCU p_docu = p_docu_from_ev_docno(ev_docno);
        SLR slr;
        SKEL_POINT skel_point;

        slr_from_ev_slr(&slr, p_ev_slr);
        skel_point_from_slr_tl(p_docu, &skel_point, &slr);
        return(xy ? skel_point.page_num.y : skel_point.page_num.x);
    }

    return(EVAL_ERR_OUTOFRANGE);
}

/******************************************************************************
*
* switch on recalc NULL events
*
******************************************************************************/

#define RECALC_NULL_CLIENT_HANDLE ((CLIENT_HANDLE) 0x00000005)

BOOL g_ev_recalc_started = FALSE;

PROC_EVENT_PROTO(static, null_event_recalc)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM(p_data);

#if CHECKING
    switch(t5_message)
    {
    default: default_unhandled();
        return(STATUS_OK);

    case T5_EVENT_NULL: /* null events for recalc come here */
#else
    IGNOREPARM_InVal_(t5_message);
    {
#endif
        ev_recalc();
        return(STATUS_OK);
    }
}

extern void
ev_recalc_start(
    _InVal_     BOOL must)
{
    if(g_ev_recalc_started)
        return;

    if(!(must || ev_doc_auto_calc()))
        return;

    trace_1(TRACE_OUT | TRACE_ANY, TEXT("ev_recalc_start(must=%d) - *** null_events_start(DOCNO_NONE)"), (int) must);
    if(status_ok(status_wrap(null_events_start(DOCNO_NONE, T5_EVENT_NULL, null_event_recalc, RECALC_NULL_CLIENT_HANDLE))))
        g_ev_recalc_started = TRUE;
}

/******************************************************************************
*
* tell our friends what's going on
*
******************************************************************************/

extern void
ev_recalc_status(
    _InVal_     STATUS to_calc)
{
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
    {
        const P_DOCU p_docu = p_docu_from_docno_valid(docno);

        if(p_docu->flags.is_current)
        {
            if(to_calc > 0)
                status_line_setf(p_docu, STATUS_LINE_LEVEL_SS_RECALC, SS_MSG_STATUS_RECALC_N, to_calc);
            else if(to_calc == 0)
                status_line_clear(p_docu, STATUS_LINE_LEVEL_SS_RECALC);
            else
            {
                UI_TEXT ui_text;
                ui_text.type = UI_TEXT_TYPE_RESID;
                ui_text.text.resource_id = to_calc;
                status_line_set(p_docu, STATUS_LINE_LEVEL_SS_RECALC, &ui_text);
            }

            /*break; SKS after 1.05 20oct93 - messages could be left stranded by current docu change */
        }
        else
            status_line_clear(p_docu, STATUS_LINE_LEVEL_SS_RECALC); /* SKS after 1.05 20oct93 - messages could be left stranded by current docu change */
    }
}

/******************************************************************************
*
* switch off recalc NULL events
*
******************************************************************************/

extern void
ev_recalc_stop(void)
{
    if(!g_ev_recalc_started)
        return;

    g_ev_recalc_started = FALSE;

    trace_0(TRACE_OUT | TRACE_ANY, TEXT("ev_recalc_stop() - *** null_events_stop(DOCNO_NONE)"));
    null_events_stop(DOCNO_NONE, T5_EVENT_NULL, null_event_recalc, RECALC_NULL_CLIENT_HANDLE);

    status_assert(maeve_service_event(P_DOCU_NONE, T5_MSG_RECALCED, P_DATA_NONE));

    {
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
    {
        const P_DOCU p_docu = p_docu_from_docno_valid(docno);

        if(!p_docu->flags.is_current)
            continue;

        status_assert(maeve_event(p_docu, T5_MSG_RECALCED, P_DATA_NONE));
        break;
    }
    } /*block*/
}

/******************************************************************************
*
* redraw cells
*
******************************************************************************/

extern void
ev_redraw_slot_range(
    _InRef_     PC_EV_RANGE p_ev_range)
{
    if(DOCNO_NONE != ev_slr_docno(&p_ev_range->s))
    {
        const P_DOCU p_docu = p_docu_from_ev_docno(ev_slr_docno(&p_ev_range->s));
        REGION region;

        slr_from_ev_slr(&region.tl, &p_ev_range->s);
        slr_from_ev_slr(&region.br, &p_ev_range->e);
        region.whole_col = region.whole_row = FALSE;

        status_consume(object_skel(p_docu, T5_MSG_REDRAW_REGION, &region));
    }
}

/******************************************************************************
*
* broadcast a change to a name
*
******************************************************************************/

_Check_return_
extern BOOL /* found a use */
ev_tell_name_clients(
    _InVal_     EV_HANDLE ev_handle,
    _InVal_     BOOL changed)
{
    SS_NAME_CHANGE ss_name_change;
    ss_name_change.ev_handle = ev_handle;
    ss_name_change.found_use = 0;
    ss_name_change.name_changed = changed;
    status_assert(maeve_service_event(P_DOCU_NONE, T5_MSG_SS_NAME_CHANGE, &ss_name_change));
    return(ss_name_change.found_use);
}

/******************************************************************************
*
* travel to a cell and return an evaluator type pointer to the cell
*
* --out--
* <0 non-evaluator cell
* =0 no cell found
* >0 evaluator cell
*
******************************************************************************/

_Check_return_
extern S32
ev_travel(
    _OutRef_    P_P_EV_CELL p_p_ev_cell,
    _InRef_     PC_EV_SLR p_ev_slr)
{
    const P_DOCU p_docu = p_docu_from_ev_docno(ev_slr_docno(p_ev_slr));
    const COL col = ev_slr_col(p_ev_slr);
    const ROW row = ev_slr_row(p_ev_slr);
    S32 res = 0;

    *p_p_ev_cell = NULL;

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_MODULE_EVAL))
    {
        TCHARZ tstr_buf[32 + BUF_EV_LONGNAMLEN];
        ev_trace_slr_tstr_buf(tstr_buf, elemof32(tstr_buf), TEXT("ev_travel $$"), p_ev_slr);
        trace_v0(TRACE_MODULE_EVAL, tstr_buf);
    }
#endif

    if(!p_docu->flags.has_data)
        return(create_error(DOCNO_ERR_CANTEXTREF));

    if(col < n_cols_physical(p_docu))
    {
        const P_CELL p_cell = list_gotoitemcontents_opt(CELL, array_ptr(&p_docu->h_col_list, LIST_BLOCK, col), (LIST_ITEMNO) row);

        if(NULL != p_cell)
        {
            if(OBJECT_ID_SS == object_id_from_cell(p_cell))
            {
                *p_p_ev_cell = (P_EV_CELL) &p_cell->object[0];
                res = 1;
            }
            else
            {
                res = -1;
            }
        }
    }

    return(res);
}

/******************************************************************************
*
* send around uref change message for a cell
*
******************************************************************************/

extern void
ev_uref_change_range(
    _InRef_     PC_EV_RANGE p_ev_range)
{
    if(DOCNO_NONE != ev_slr_docno(&p_ev_range->s))
    {
        UREF_PARMS uref_parms;
        const P_DOCU p_docu = p_docu_from_ev_docno(ev_slr_docno(&p_ev_range->s));

        slr_from_ev_slr(&uref_parms.source.region.tl, &p_ev_range->s);
        slr_from_ev_slr(&uref_parms.source.region.br, &p_ev_range->e);
        uref_parms.source.region.whole_col = uref_parms.source.region.whole_row = 0;

        uref_event(p_docu, T5_MSG_UREF_CHANGE, &uref_parms);
    }
}

/******************************************************************************
*
* call uref service to update an evaluator range
*
******************************************************************************/

_Check_return_
extern S32
ev_uref_match_range(
    _InoutRef_  P_EV_RANGE p_ev_range,
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_UREF_EVENT_BLOCK p_uref_event_block)
{
    S32 res = DEP_NONE;

    DOCU_ASSERT(p_docu);

    if(ev_slr_docno(&p_ev_range->s) == p_docu->docno)
    {
        REGION region;

        region.tl.col = p_ev_range->s.col;
        region.tl.row = p_ev_range->s.row;
        region.br.col = p_ev_range->e.col;
        region.br.row = p_ev_range->e.row;
        region.whole_col = region.whole_row = FALSE;

        if((res = uref_match_region(&region, t5_message, p_uref_event_block)) != DEP_NONE)
        {
            p_ev_range->s.col = EV_COL_PACK(region.tl.col);
            p_ev_range->s.row = (EV_ROW)    region.tl.row;
            p_ev_range->e.col = EV_COL_PACK(region.br.col);
            p_ev_range->e.row = (EV_ROW)    region.br.row;

            if( (res == DEP_DELETE)
                &&
                (t5_message != T5_MSG_UREF_CLOSE1)
                &&
                (t5_message != T5_MSG_UREF_CLOSE2) )
            {
                p_ev_range->s.bad_ref = p_ev_range->e.bad_ref = 1;
            }
        }
    }

    return(res);
}

/******************************************************************************
*
* call uref service to update an evaluator cell reference
*
******************************************************************************/

_Check_return_
extern S32
ev_uref_match_slr(
    _InoutRef_  P_EV_SLR p_ev_slr,
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_UREF_EVENT_BLOCK p_uref_event_block)
{
    S32 res = DEP_NONE;

    DOCU_ASSERT(p_docu);

    if(ev_slr_docno(p_ev_slr) == p_docu->docno)
    {
        SLR slr;

        slr.col = ev_slr_col(p_ev_slr);
        slr.row = ev_slr_row(p_ev_slr);

        if(DEP_NONE != (res = uref_match_slr(&slr, t5_message, p_uref_event_block)))
        {
            p_ev_slr->col = EV_COL_PACK(slr.col);
            p_ev_slr->row = (EV_ROW)    slr.row;

            if( (res == DEP_DELETE)
                &&
                (t5_message != T5_MSG_UREF_CLOSE1)
                &&
                (t5_message != T5_MSG_UREF_CLOSE2) )
            {
                p_ev_slr->bad_ref = 1;
            }
        }
    }

    return(res);
}

/******************************************************************************
*
* write out document name in square brackets;
* pathname common to docno_from is removed from output
*
******************************************************************************/

/*ncr*/
extern U32 /* number of chars output */
ev_write_docname_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     EV_DOCNO ev_docno_to,
    _InVal_     EV_DOCNO ev_docno_from)
{
    U32 len = 0;

    PtrPutByteOff(ustr_buf, len++, CH_LEFT_SQUARE_BRACKET);
    len += docno_name_write_ustr_buf(ustr_AddBytes_wr(ustr_buf, len), elemof_buffer - 2, (DOCNO) ev_docno_to, (DOCNO) ev_docno_from);
    PtrPutByteOff(ustr_buf, len++, CH_RIGHT_SQUARE_BRACKET);
    PtrPutByteOff(ustr_buf, len,   CH_NULL);

    return(len);
}

/* end of link_ev.c */
