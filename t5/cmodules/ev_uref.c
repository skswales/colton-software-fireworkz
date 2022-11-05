/* ev_uref.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Evaluator uref service */

/* MRJC February 1991 / March 1993 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#if TRACE_ALLOWED

static void
ev_uref_event_trace_begin(_InVal_ UREF_MESSAGE uref_message)
{
    trace_0(TRACE_MODULE_UREF, TEXT("ev_uref_uref_event in -- |"));

    switch(uref_message)
    {
    case Uref_Msg_Change:    trace_0(TRACE_MODULE_UREF, TEXT("|Uref_Msg_Change |")); break;
    case Uref_Msg_Uref:      trace_0(TRACE_MODULE_UREF, TEXT("|Uref_Msg_Uref |")); break;
    case Uref_Msg_Delete:    trace_0(TRACE_MODULE_UREF, TEXT("|Uref_Msg_Delete |")); break;
    case Uref_Msg_Swap_Rows: trace_0(TRACE_MODULE_UREF, TEXT("|Uref_Msg_Swap_Rows |")); break;
    case Uref_Msg_Overwrite: trace_0(TRACE_MODULE_UREF, TEXT("|Uref_Msg_Overwrite |")); break;
    case Uref_Msg_CLOSE1:    trace_0(TRACE_MODULE_UREF, TEXT("|Uref_Msg_CLOSE1 |")); break;
    case Uref_Msg_CLOSE2:    trace_0(TRACE_MODULE_UREF, TEXT("|Uref_Msg_CLOSE2 |")); break;
    default:                 default_unhandled(); break;
    }
}

#endif /* TRACE_ALLOWED */

/* blow up the evaluator when things move under its feet */

static inline void
ev_uref_event_test_blow_up(
    _InVal_     UREF_MESSAGE uref_message)
{
    switch(uref_message)
    {
    case Uref_Msg_Change:
        return;

    default: default_unhandled();
#if CHECKING
    case Uref_Msg_Overwrite:
    case Uref_Msg_Uref:
    case Uref_Msg_Delete:
    case Uref_Msg_Swap_Rows:
    case Uref_Msg_CLOSE1:
    case Uref_Msg_CLOSE2:
#endif
        break;
    }

    stack_zap();
}

/* look through the needs_recalc list */

static inline void
ev_uref_event_scan_needs_recalc(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block)
{
    switch(uref_message)
    {
    case Uref_Msg_Change:
    case Uref_Msg_Overwrite:
        return;

    default: default_unhandled();
#if CHECKING
    case Uref_Msg_Uref:
    case Uref_Msg_Delete:
    case Uref_Msg_Swap_Rows:
    case Uref_Msg_CLOSE1:
    case Uref_Msg_CLOSE2:
#endif
        break;
    }

    {
    const ARRAY_INDEX needs_recalc_n = array_elements(&h_needs_recalc);
    ARRAY_INDEX needs_recalc_ix;
    P_NEEDS_RECALC p_needs_recalc = array_range(&h_needs_recalc, NEEDS_RECALC, 0, needs_recalc_n);

    for(needs_recalc_ix = 0; needs_recalc_ix < needs_recalc_n; ++needs_recalc_ix, ++p_needs_recalc)
    {
        UREF_COMMS res;

        if(p_needs_recalc->ev_slr.bad_ref)
            continue;

        res = ev_uref_match_slr(&p_needs_recalc->ev_slr, p_docu, uref_message, p_uref_event_block);

        if(Uref_Dep_Delete == res)
            p_needs_recalc->ev_slr.bad_ref = 1;
    }
    } /*block*/
}

/* look through the todo list */

static inline void
ev_uref_event_scan_todo(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block)
{
    switch(uref_message)
    {
    case Uref_Msg_Change:
    case Uref_Msg_Overwrite:
        return;

    default: default_unhandled();
#if CHECKING
    case Uref_Msg_Uref:
    case Uref_Msg_Delete:
    case Uref_Msg_Swap_Rows:
    case Uref_Msg_CLOSE1:
    case Uref_Msg_CLOSE2:
#endif
        break;
    }

    {
    const ARRAY_INDEX todo_n = array_elements(&h_todo_list);
    ARRAY_INDEX todo_ix;
    P_TODO_ENTRY p_todo_entry = array_range(&h_todo_list, TODO_ENTRY, 0, todo_n);

    for(todo_ix = 0; todo_ix < todo_n; ++todo_ix, ++p_todo_entry)
    {
        UREF_COMMS res;

        if(p_todo_entry->node_distance < 0)
            continue;

        res = ev_uref_match_slr(&p_todo_entry->slr, p_docu, uref_message, p_uref_event_block);

        if(Uref_Dep_Delete == res)
            p_todo_entry->node_distance = -1;
    }
    } /*block*/
}

/* look through the range tree */

static inline void
ev_uref_event_scan_this_document_range_tree(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block,
    _InRef_     P_SS_DOC p_ss_doc)
{
    ARRAY_INDEX range_ix;
    P_RANGE_USE p_range_use;

    switch(uref_message)
    {
    case Uref_Msg_Change:
        return;

    default: default_unhandled();
#if CHECKING
    case Uref_Msg_Uref:
    case Uref_Msg_Delete:
    case Uref_Msg_Swap_Rows:
    case Uref_Msg_Overwrite:
    case Uref_Msg_CLOSE1:
    case Uref_Msg_CLOSE2:
#endif
        break;
    }

    for(range_ix = 0, p_range_use = p_range_use_from_p_ss_doc(p_ss_doc, range_ix);
        range_ix < array_elements(&p_ss_doc->range_table.h_table);
        ++range_ix, ++p_range_use)
    {
        UREF_COMMS res;

        if(p_range_use->flags.to_be_deleted)
            continue;

        /* refs contained by deleted area must be removed */
        if(Uref_Dep_None != (res = ev_uref_match_slr(&p_range_use->slr_by, p_docu, uref_message, p_uref_event_block)))
        {
            if( (Uref_Dep_Delete == res) || (Uref_Msg_Overwrite == uref_message) )
            {
                p_range_use->flags.to_be_deleted = 1;
                p_ss_doc->range_table.flags.to_be_deleted = 1;
                p_ss_doc->range_table.mindel = MIN(p_ss_doc->range_table.mindel, range_ix);
                continue;
            }
        }

        /* if reference to matched, find the reference in the compiled string and update that */
        if(Uref_Dep_None != (res = ev_uref_match_range(&p_range_use->range_to, p_docu, uref_message, p_uref_event_block)))
        {
            /* mark for recalc */
            ev_todo_add_slr(&p_range_use->slr_by);

            if(Uref_Dep_Inform != res)
            {
                P_EV_CELL p_ev_cell;

                if(ev_travel(&p_ev_cell, &p_range_use->slr_by) > 0)
                {
                    consume(UREF_COMMS,
                        ev_uref_match_range(
                                p_ev_range_from_ev_cell(p_ev_cell, p_range_use->by_index),
                                p_docu,
                                uref_message,
                                p_uref_event_block));

                    if(Uref_Dep_Update == res)
                        ev_doc_modify(ev_slr_docno(&p_range_use->slr_by));
                }

                p_ss_doc->range_table.sorted = 0;
            }
        }
    }
}

/* look through the slr tree */

static inline void
ev_uref_event_scan_this_document_slr_tree(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block,
    _InRef_     P_SS_DOC p_ss_doc)
{
    ARRAY_INDEX slr_ix;
    P_SLR_USE p_slr_use;

    switch(uref_message)
    {
    case Uref_Msg_Change:
        return;

    default: default_unhandled();
#if CHECKING
    case Uref_Msg_Uref:
    case Uref_Msg_Delete:
    case Uref_Msg_Swap_Rows:
    case Uref_Msg_Overwrite:
    case Uref_Msg_CLOSE1:
    case Uref_Msg_CLOSE2:
#endif
        break;
    }

    for(slr_ix = 0, p_slr_use = p_slr_use_from_p_ss_doc(p_ss_doc, slr_ix);
        slr_ix < array_elements(&p_ss_doc->slr_table.h_table);
        ++slr_ix, ++p_slr_use)
    {
        UREF_COMMS res;

        if(p_slr_use->flags.to_be_deleted)
            continue;

        /* refs contained by deleted area must be removed */
        if(Uref_Dep_None != (res = ev_uref_match_slr(&p_slr_use->slr_by, p_docu, uref_message, p_uref_event_block)))
        {
            if( (Uref_Dep_Delete == res) || (Uref_Msg_Overwrite == uref_message) )
            {
                p_slr_use->flags.to_be_deleted = 1;
                p_ss_doc->slr_table.flags.to_be_deleted = 1;
                p_ss_doc->slr_table.mindel = MIN(p_ss_doc->slr_table.mindel, slr_ix);
                continue;
            }
        }

        /* if ref_to matched, find the reference in the compiled string and update that */
        if(Uref_Dep_None != (res = ev_uref_match_slr(&p_slr_use->slr_to, p_docu, uref_message, p_uref_event_block)))
        {
            /* mark for recalc */
            ev_todo_add_slr(&p_slr_use->slr_by);

            if(Uref_Dep_Inform != res)
            {
                P_EV_CELL p_ev_cell;

                if(ev_travel(&p_ev_cell, &p_slr_use->slr_by) > 0)
                {
                    consume(UREF_COMMS,
                        ev_uref_match_slr(
                                p_ev_slr_from_ev_cell(p_ev_cell, p_slr_use->by_index),
                                p_docu,
                                uref_message,
                                p_uref_event_block));

                    if(Uref_Dep_Update == res)
                        ev_doc_modify(ev_slr_docno(&p_slr_use->slr_by));
                }

                p_ss_doc->slr_table.sorted = 0;
            }
        }
    }
}

/* look through the range and slr use tables for each document */

static inline void
ev_uref_event_scan_all_documents(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block)
{
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_thunks(docno)))
    {
        const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(docno);

        if(P_SS_DOC_NONE == p_ss_doc)
            continue;

        /* look through the range and slr use tables for this document */
        ev_uref_event_scan_this_document_range_tree(p_docu, uref_message, p_uref_event_block, p_ss_doc);
        ev_uref_event_scan_this_document_slr_tree(p_docu, uref_message, p_uref_event_block, p_ss_doc);
    }
}

/* look through the name use table */

static inline void
ev_uref_event_scan_name_use_table(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block)
{
    switch(uref_message)
    {
    case Uref_Msg_Change:
        return;

    default: default_unhandled();
#if CHECKING
    case Uref_Msg_Uref:
    case Uref_Msg_Delete:
    case Uref_Msg_Swap_Rows:
    case Uref_Msg_Overwrite:
    case Uref_Msg_CLOSE1:
    case Uref_Msg_CLOSE2:
#endif
        break;
    }

    if(0 == array_elements(&name_use_deptable.h_table))
        return;

    name_list_sort();

    {
    BOOL check_use = FALSE;
    BOOL un_sort = FALSE;
    ARRAY_INDEX name_use_idx;

    for(name_use_idx = 0; name_use_idx < array_elements(&name_use_deptable.h_table); ++name_use_idx)
    {
        const P_NAME_USE p_name_use = array_ptr(&name_use_deptable.h_table, NAME_USE, name_use_idx);
        UREF_COMMS res;

        if(p_name_use->flags.to_be_deleted)
            continue;

        /* refs contained by deleted area must be removed */
        if(Uref_Dep_None != (res = ev_uref_match_slr(&p_name_use->slr_by, p_docu, uref_message, p_uref_event_block)))
        {
            if( (Uref_Dep_Delete == res) || (Uref_Msg_Overwrite == uref_message) )
            {
                p_name_use->flags.to_be_deleted = 1;
                name_use_deptable.flags.to_be_deleted = 1;
                name_use_deptable.mindel = MIN(name_use_deptable.mindel, name_use_idx);
                check_use = TRUE;
            }
            else
                un_sort = TRUE;
        }
    }

    /* set these flags at end to avoid sorts in the middle of our fuddle */
    if(check_use)
        name_def_deptable.flags.checkuse = 1;
    if(un_sort)
        name_use_deptable.sorted = 0;

    } /*block*/
}

/* look through the name definition table */

static inline void
ev_uref_event_scan_name_definition_table(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block)
{
    switch(uref_message)
    {
    case Uref_Msg_Change:
        return;

    default: default_unhandled();
#if CHECKING
    case Uref_Msg_Uref:
    case Uref_Msg_Delete:
    case Uref_Msg_Swap_Rows:
    case Uref_Msg_Overwrite:
    case Uref_Msg_CLOSE1:
    case Uref_Msg_CLOSE2:
#endif
        break;
    }

    if(0 == array_elements(&name_def_deptable.h_table))
        return;

    name_list_sort();

    {
    BOOL name_def_to_be_deleted = FALSE;
    ARRAY_INDEX name_def_mindel = name_def_deptable.mindel;
    ARRAY_INDEX name_def_idx;

    CHECKING_ONLY(const ARRAY_INDEX initial_n_elements = array_elements(&name_def_deptable.h_table));

    for(name_def_idx = 0; name_def_idx < array_elements(&name_def_deptable.h_table); ++name_def_idx)
    {
        const P_EV_NAME p_ev_name = array_ptr(&name_def_deptable.h_table, EV_NAME, name_def_idx);
        UREF_COMMS res = Uref_Dep_None;

        if(p_ev_name->flags.to_be_deleted)
            continue;

        /* check for name definition in the area */
        if( (Uref_Msg_Change != uref_message)
            &&
            (Uref_Dep_None != (res = ev_uref_match_slr(&p_ev_name->owner, p_docu, uref_message, p_uref_event_block))) )
        {
            if( (Uref_Dep_Delete == res) || (Uref_Msg_Overwrite == uref_message) )
            {
                name_free_resources(p_ev_name);

                /* delete name definition if there are no dependents */
                if(!ev_todo_add_name_dependents(p_ev_name->handle))
                {
                    CHECKING_ONLY(assert(initial_n_elements == array_elements(&name_def_deptable.h_table)));

                    trace_1(TRACE_MODULE_UREF, TEXT("uref deleting name: %s"), report_ustr(ustr_bptr(p_ev_name->ustr_name_id)));

                    {
                    const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_slr_docno(&p_ev_name->owner));

                    if(P_SS_DOC_NONE != p_ss_doc)
                        p_ss_doc->name_ref_count -= 1;
                    } /*block*/

                    p_ev_name->flags.to_be_deleted = 1;
                    /*name_def_deptable.flags.to_be_deleted = 1;*/
                    /*name_def_deptable.mindel = MIN(name_def_deptable.mindel, name_def_idx);*/
                    name_def_to_be_deleted = TRUE;
                    name_def_mindel = MIN(name_def_mindel, name_def_idx);
                }

                /* it's definitely undefined now, anyway */
                p_ev_name->flags.undefined = 1;
            }
        }

        /* check for the definition of the name itself being in the uref area */
        if(!p_ev_name->flags.undefined)
        {
            switch(ss_data_get_data_id(&p_ev_name->def_data))
            {
            case DATA_ID_SLR:
                if(Uref_Dep_None != ev_uref_match_slr(&p_ev_name->def_data.arg.slr, p_docu, uref_message, p_uref_event_block))
                {
                    if(Uref_Msg_Change != uref_message)
                    {
                        if(Uref_Dep_Update == res)
                            ev_doc_modify(ev_slr_docno(&p_ev_name->owner));
                    }
                    ev_todo_add_name_dependents(p_ev_name->handle);
                }
                break;

            case DATA_ID_RANGE:
                if(Uref_Dep_None != ev_uref_match_range(&p_ev_name->def_data.arg.range, p_docu, uref_message, p_uref_event_block))
                {
                    if(Uref_Msg_Change != uref_message)
                    {
                        if(Uref_Dep_Update == res)
                            ev_doc_modify(ev_slr_docno(&p_ev_name->owner));
                    }
                    ev_todo_add_name_dependents(p_ev_name->handle);
                }
                break;
            }
        }
    }

    /* set these at end to avoid sorts in the middle of our fuddle */
    if(name_def_to_be_deleted)
    {
        name_def_deptable.flags.to_be_deleted = 1;
        name_def_deptable.mindel = MIN(name_def_deptable.mindel, name_def_mindel);

        name_list_sort();
    }

    } /*block*/
}

/* look through the event use table */

static inline void
ev_uref_event_scan_event_use_table(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block)
{
    switch(uref_message)
    {
    case Uref_Msg_Change:
        return;

    default: default_unhandled();
#if CHECKING
    case Uref_Msg_Uref:
    case Uref_Msg_Delete:
    case Uref_Msg_Swap_Rows:
    case Uref_Msg_Overwrite:
    case Uref_Msg_CLOSE1:
    case Uref_Msg_CLOSE2:
#endif
        break;
    }

    if(0 == array_elements(&event_use_deptable.h_table))
        return;

    tree_sort_event_use();

    {
    ARRAY_INDEX event_use_idx;

    for(event_use_idx = 0; event_use_idx < array_elements(&event_use_deptable.h_table); ++event_use_idx)
    {
        const P_EVENT_USE p_event_use = array_ptr(&event_use_deptable.h_table, EVENT_USE, event_use_idx);
        UREF_COMMS res;

        if(p_event_use->flags.to_be_deleted)
            continue;

        /* event uses contained by deleted area must be removed */
        if(Uref_Dep_None != (res = ev_uref_match_slr(&p_event_use->slr_by, p_docu, uref_message, p_uref_event_block)))
        {
            if( (Uref_Dep_Delete == res) || (Uref_Msg_Overwrite == uref_message) )
            {
                p_event_use->flags.to_be_deleted = 1;
                event_use_deptable.flags.to_be_deleted = 1;
                event_use_deptable.mindel = MIN(event_use_deptable.mindel, event_use_idx);
            }
        }
    }
    } /*block*/
}

/* look through the custom function use table */

static inline void
ev_uref_event_scan_custom_use_table(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block)
{
    switch(uref_message)
    {
    case Uref_Msg_Change:
        return;

    default: default_unhandled();
#if CHECKING
    case Uref_Msg_Uref:
    case Uref_Msg_Delete:
    case Uref_Msg_Swap_Rows:
    case Uref_Msg_Overwrite:
    case Uref_Msg_CLOSE1:
    case Uref_Msg_CLOSE2:
#endif
        break;
    }

    if(0 == array_elements(&custom_use_deptable.h_table))
        return;

    custom_list_sort();

    {
    BOOL check_use = FALSE;
    BOOL un_sort = FALSE;
    ARRAY_INDEX custom_use_idx;

    for(custom_use_idx = 0; custom_use_idx < array_elements(&custom_use_deptable.h_table); ++custom_use_idx)
    {
        const P_CUSTOM_USE p_custom_use = array_ptr(&custom_use_deptable.h_table, CUSTOM_USE, custom_use_idx);
        UREF_COMMS res;

        if(p_custom_use->flags.to_be_deleted)
            continue;

        /* custom function uses contained by deleted area must be removed */
        if(Uref_Dep_None != (res = ev_uref_match_slr(&p_custom_use->slr_by, p_docu, uref_message, p_uref_event_block)))
        {
            if( (Uref_Dep_Delete == res) || (Uref_Msg_Overwrite == uref_message) )
            {
                p_custom_use->flags.to_be_deleted = 1;
                custom_use_deptable.flags.to_be_deleted = 1;
                custom_use_deptable.mindel = MIN(custom_use_deptable.mindel, custom_use_idx);
                check_use = TRUE;
            }
            else
                un_sort = TRUE;
        }
    }

    /* set these flags at end to avoid sorts in the middle of our fuddle */
    if(check_use)
        custom_def_deptable.flags.checkuse = 1;
    if(un_sort)
        custom_use_deptable.sorted = 0;

    } /*block*/
}

/* look through the custom function definition table */

static inline void
ev_uref_event_scan_custom_definition_table(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block)
{
    switch(uref_message)
    {
    case Uref_Msg_Change:
        return;

    default: default_unhandled();
#if CHECKING
    case Uref_Msg_Uref:
    case Uref_Msg_Delete:
    case Uref_Msg_Swap_Rows:
    case Uref_Msg_Overwrite:
    case Uref_Msg_CLOSE1:
    case Uref_Msg_CLOSE2:
#endif
        break;
    }

    if(0 == array_elements(&custom_def_deptable.h_table))
        return;

    custom_list_sort();

    {
    BOOL custom_def_to_be_deleted = FALSE;
    ARRAY_INDEX custom_def_mindel = custom_def_deptable.mindel;
    ARRAY_INDEX custom_def_idx;

    CHECKING_ONLY(const ARRAY_INDEX initial_n_elements = array_elements(&custom_def_deptable.h_table));

    for(custom_def_idx = 0; custom_def_idx < array_elements(&custom_def_deptable.h_table); ++custom_def_idx)
    {
        const P_EV_CUSTOM p_ev_custom = array_ptr(&custom_def_deptable.h_table, EV_CUSTOM, custom_def_idx);
        UREF_COMMS res;

        if(p_ev_custom->flags.to_be_deleted)
            continue;

        /* check for custom function definition in the area */
        if(Uref_Dep_None != (res = ev_uref_match_slr(&p_ev_custom->owner, p_docu, uref_message, p_uref_event_block)))
        {
            if( (Uref_Dep_Delete == res) || (Uref_Msg_Overwrite == uref_message) )
            {
                /* delete custom function table entry if there are no dependents */
                if(!ev_todo_add_custom_dependents(p_ev_custom->handle))
                {
                    CHECKING_ONLY(assert(initial_n_elements == array_elements(&custom_def_deptable.h_table)));

                    trace_1(TRACE_MODULE_UREF, TEXT("uref deleting custom: %s"), report_ustr(ustr_bptr(p_ev_custom->ustr_custom_id)));

                    {
                    const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_slr_docno(&p_ev_custom->owner));

                    if(P_SS_DOC_NONE != p_ss_doc)
                        p_ss_doc->custom_ref_count -= 1;
                    } /*block*/

                    p_ev_custom->flags.to_be_deleted = 1;
                    /*custom_def_deptable.flags.to_be_deleted = 1;*/
                    /*custom_def_deptable.mindel = MIN(custom_def_deptable.mindel, custom_def_idx);*/
                    custom_def_to_be_deleted = TRUE;
                    custom_def_mindel = MIN(custom_def_mindel, custom_def_idx);
                }

                /* it's definitely undefined now, anyway */
                p_ev_custom->owner.col = EV_COL_PACK(EV_MAX_COL - 1);
                p_ev_custom->owner.row =            (EV_MAX_ROW - 1);
                p_ev_custom->flags.undefined = 1;
            }
        }
    }

    /* set these at end to avoid sorts in the middle of our fuddle */
    if(custom_def_to_be_deleted)
    {
        custom_def_deptable.flags.to_be_deleted = 1;
        custom_def_deptable.mindel = MIN(custom_def_deptable.mindel, custom_def_mindel);

        custom_list_sort();
    }

    } /*block*/
}

/* try clearing up documents on CLOSE2 */

static inline void
ev_uref_event_tidy_on_msg_close2(void)
{
    DOCNO docno = DOCNO_NONE;

    tree_sort_all();

    while(DOCNO_NONE != (docno = docno_enum_thunks(docno)))
    {
        const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(docno);

        if(!ev_doc_is_thunk(docno))
            continue;

        PTR_ASSERT(p_ss_doc);
        if(P_SS_DOC_NONE == p_ss_doc)
            continue;

        if(0 != p_ss_doc->uref_handle)
        {
            if( (0 == p_ss_doc->slr_table.h_table)   &&
                (0 == p_ss_doc->range_table.h_table) &&
                (0 == p_ss_doc->name_ref_count)      &&
                (0 == p_ss_doc->custom_ref_count)    )
            {
                uref_del_dependency(docno, p_ss_doc->uref_handle);
                p_ss_doc->uref_handle = 0;
            }
        }
    }
}

PROC_UREF_EVENT_PROTO(extern, ev_uref_uref_event)
{
    STATUS status = STATUS_OK;

#if TRACE_ALLOWED
    ev_uref_event_trace_begin(uref_message);
#endif

    /* blow up the evaluator when things move under its feet */
    ev_uref_event_test_blow_up(uref_message);

    /* look through the needs_recalc list */
    ev_uref_event_scan_needs_recalc(p_docu, uref_message, p_uref_event_block);

    /* look through the todo list */
    ev_uref_event_scan_todo(p_docu, uref_message, p_uref_event_block);

    /* look through the range and slr use tables for each document */
    ev_uref_event_scan_all_documents(p_docu, uref_message, p_uref_event_block);

    /* look through the name use table */
    ev_uref_event_scan_name_use_table(p_docu, uref_message, p_uref_event_block);

    /* look through the name definition table */
    ev_uref_event_scan_name_definition_table(p_docu, uref_message, p_uref_event_block);

    /* look through the event use table */
    ev_uref_event_scan_event_use_table(p_docu, uref_message, p_uref_event_block);

    /* look through the custom function use table */
    ev_uref_event_scan_custom_use_table(p_docu, uref_message, p_uref_event_block);

    /* update custom function definition table */
    ev_uref_event_scan_custom_definition_table(p_docu, uref_message, p_uref_event_block);

    /* try clearing up documents on CLOSE2 */
    if(Uref_Msg_CLOSE2 == uref_message)
        ev_uref_event_tidy_on_msg_close2();

    trace_0(TRACE_MODULE_UREF, TEXT("| -- out"));

    return(status);
}

/* end of ev_uref.c */
