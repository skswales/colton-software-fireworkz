/* ev_uref.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Evaluator uref service */

/* MRJC February 1991 / March 1993 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

/* look through the needs_recalc list */

static void
ev_uref_process_needs_recalc(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block,
    _InVal_     ARRAY_INDEX needs_recalc_n)
{
    ARRAY_INDEX needs_recalc_ix;
    P_NEEDS_RECALC p_needs_recalc = array_range(&h_needs_recalc, NEEDS_RECALC, 0, needs_recalc_n);

    for(needs_recalc_ix = 0; needs_recalc_ix < needs_recalc_n; ++needs_recalc_ix, ++p_needs_recalc)
    {
        S32 res;

        if(p_needs_recalc->ev_slr.bad_ref)
            continue;

        res = ev_uref_match_slr(&p_needs_recalc->ev_slr, p_docu, t5_message, p_uref_event_block);

        if(res == DEP_DELETE)
            p_needs_recalc->ev_slr.bad_ref = 1;
    }
}

/* look through the todo list */

static void
ev_uref_process_todo(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block,
    _InVal_     ARRAY_INDEX todo_n)
{
    ARRAY_INDEX todo_ix;
    P_TODO_ENTRY p_todo_entry = array_range(&h_todo_list, TODO_ENTRY, 0, todo_n);

    for(todo_ix = 0; todo_ix < todo_n; ++todo_ix, ++p_todo_entry)
    {
        S32 res;

        if(p_todo_entry->node_distance < 0)
            continue;

        res = ev_uref_match_slr(&p_todo_entry->slr, p_docu, t5_message, p_uref_event_block);

        if(res == DEP_DELETE)
            p_todo_entry->node_distance = -1;
    }
}

PROC_UREF_EVENT_PROTO(extern, proc_uref_event_ev_uref)
{
    STATUS status = STATUS_OK;

    trace_0(TRACE_MODULE_UREF, TEXT("ev_uref_event in -- |"));

#if TRACE_ALLOWED
    switch(t5_message)
    {
    case T5_MSG_UREF_CHANGE:    trace_0(TRACE_MODULE_UREF, TEXT("|T5_MSG_UREF_CHANGE |")); break;
    case T5_MSG_UREF_UREF:      trace_0(TRACE_MODULE_UREF, TEXT("|T5_MSG_UREF_UREF |")); break;
    case T5_MSG_UREF_DELETE:    trace_0(TRACE_MODULE_UREF, TEXT("|T5_MSG_UREF_DELETE |")); break;
    case T5_MSG_UREF_SWAP_ROWS: trace_0(TRACE_MODULE_UREF, TEXT("|T5_MSG_UREF_SWAP_ROWS |")); break;
    case T5_MSG_UREF_OVERWRITE: trace_0(TRACE_MODULE_UREF, TEXT("|T5_MSG_UREF_OVERWRITE |")); break;
    case T5_MSG_UREF_CLOSE1:    trace_0(TRACE_MODULE_UREF, TEXT("|T5_MSG_UREF_CLOSE1 |")); break;
    case T5_MSG_UREF_CLOSE2:    trace_0(TRACE_MODULE_UREF, TEXT("|T5_MSG_UREF_CLOSE2 |")); break;
    default: default_unhandled(); break;
    }
#endif

    /* blow up the evaluator when things move under its feet */
    switch(t5_message)
    {
    case T5_MSG_UREF_CHANGE:
        break;

    default: default_unhandled();
#if CHECKING
    case T5_MSG_UREF_OVERWRITE:
    case T5_MSG_UREF_UREF:
    case T5_MSG_UREF_DELETE:
    case T5_MSG_UREF_SWAP_ROWS:
    case T5_MSG_UREF_CLOSE1:
    case T5_MSG_UREF_CLOSE2:
#endif
        stack_zap();
        break;
    }

    /* look through the needs_recalc list */
    switch(t5_message)
    {
    case T5_MSG_UREF_CHANGE:
    case T5_MSG_UREF_OVERWRITE:
        break;

    default: default_unhandled();
#if CHECKING
    case T5_MSG_UREF_UREF:
    case T5_MSG_UREF_DELETE:
    case T5_MSG_UREF_SWAP_ROWS:
    case T5_MSG_UREF_CLOSE1:
    case T5_MSG_UREF_CLOSE2:
#endif
        {
        const ARRAY_INDEX needs_recalc_n = array_elements(&h_needs_recalc);

        if(needs_recalc_n != 0)
            ev_uref_process_needs_recalc(p_docu, t5_message, p_uref_event_block, needs_recalc_n);

        break;
        }
    }

    /* look through the todo list */
    switch(t5_message)
    {
    case T5_MSG_UREF_CHANGE:
    case T5_MSG_UREF_OVERWRITE:
        break;

    default: default_unhandled();
#if CHECKING
    case T5_MSG_UREF_UREF:
    case T5_MSG_UREF_DELETE:
    case T5_MSG_UREF_SWAP_ROWS:
    case T5_MSG_UREF_CLOSE1:
    case T5_MSG_UREF_CLOSE2:
#endif
        {
        const ARRAY_INDEX todo_n = array_elements(&h_todo_list);

        if(todo_n != 0)
            ev_uref_process_todo(p_docu, t5_message, p_uref_event_block, todo_n);

        break;
        }
    }

    { /* look through the range and slr use tables for each document */
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_thunks(docno)))
    {
        const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(docno);

        if(P_DATA_NONE != p_ss_doc)
        {
            /* look through the range tree */
            switch(t5_message)
            {
            case T5_MSG_UREF_CHANGE:
                break;

            default: default_unhandled();
#if CHECKING
            case T5_MSG_UREF_UREF:
            case T5_MSG_UREF_DELETE:
            case T5_MSG_UREF_SWAP_ROWS:
            case T5_MSG_UREF_OVERWRITE:
            case T5_MSG_UREF_CLOSE1:
            case T5_MSG_UREF_CLOSE2:
#endif
                {
                P_RANGE_USE p_range_use;
                ARRAY_INDEX range_ix;

                for(range_ix = 0, p_range_use = p_range_use_from_p_ss_doc(p_ss_doc, range_ix);
                    range_ix < array_elements(&p_ss_doc->range_table.h_table);
                    ++range_ix, ++p_range_use)
                {
                    S32 res;

                    if(p_range_use->flags.tobedel)
                        continue;

                    /* refs contained by deleted area must be removed */
                    if((res = ev_uref_match_slr(&p_range_use->slr_by, p_docu, t5_message, p_uref_event_block)) != DEP_NONE)
                        if(res == DEP_DELETE || t5_message == T5_MSG_UREF_OVERWRITE)
                        {
                            p_range_use->flags.tobedel = 1;
                            p_ss_doc->range_table.flags.tobedel = 1;
                            p_ss_doc->range_table.mindel = MIN(p_ss_doc->range_table.mindel, range_ix);
                            continue;
                        }

                    /* if reference to matched, find the reference in the compiled string and update that */
                    if((res = ev_uref_match_range(&p_range_use->range_to, p_docu, t5_message, p_uref_event_block)) != DEP_NONE)
                    {
                        /* mark for recalc */
                        ev_todo_add_slr(&p_range_use->slr_by);

                        if(res != DEP_INFORM)
                        {
                            P_EV_CELL p_ev_cell;

                            if(ev_travel(&p_ev_cell, &p_range_use->slr_by) > 0)
                            {
                                (void) ev_uref_match_range(
                                            p_ev_range_from_ev_cell(p_ev_cell, p_range_use->by_index),
                                            p_docu,
                                            t5_message,
                                            p_uref_event_block);

                                if(res == DEP_UPDATE)
                                    ev_doc_modify(ev_slr_docno(&p_range_use->slr_by));
                            }

                            p_ss_doc->range_table.sorted = 0;
                        }
                    }
                }

                break;
                }
            }

            /* look through slr tree */
            switch(t5_message)
            {
            case T5_MSG_UREF_CHANGE:
                break;

            default: default_unhandled();
#if CHECKING
            case T5_MSG_UREF_UREF:
            case T5_MSG_UREF_DELETE:
            case T5_MSG_UREF_SWAP_ROWS:
            case T5_MSG_UREF_OVERWRITE:
            case T5_MSG_UREF_CLOSE1:
            case T5_MSG_UREF_CLOSE2:
#endif
                {
                P_SLR_USE p_slr_use;
                ARRAY_INDEX slr_ix;

                for(slr_ix = 0, p_slr_use = p_slr_use_from_p_ss_doc(p_ss_doc, slr_ix);
                    slr_ix < array_elements(&p_ss_doc->slr_table.h_table);
                    ++slr_ix, ++p_slr_use)
                {
                    S32 res;

                    if(p_slr_use->flags.tobedel)
                        continue;

                    /* refs contained by deleted area must be removed */
                    if((res = ev_uref_match_slr(&p_slr_use->slr_by, p_docu, t5_message, p_uref_event_block)) != DEP_NONE)
                    {
                        if(res == DEP_DELETE || t5_message == T5_MSG_UREF_OVERWRITE)
                        {
                            p_slr_use->flags.tobedel = 1;
                            p_ss_doc->slr_table.flags.tobedel = 1;
                            p_ss_doc->slr_table.mindel = MIN(p_ss_doc->slr_table.mindel, slr_ix);
                            continue;
                        }
                    }

                    /* if ref_to matched, find the reference in the compiled string and update that */
                    if((res = ev_uref_match_slr(&p_slr_use->slr_to, p_docu, t5_message, p_uref_event_block)) != DEP_NONE)
                    {
                        /* mark for recalc */
                        ev_todo_add_slr(&p_slr_use->slr_by);

                        if(res != DEP_INFORM)
                        {
                            P_EV_CELL p_ev_cell;

                            if(ev_travel(&p_ev_cell, &p_slr_use->slr_by) > 0)
                            {
                                (void) ev_uref_match_slr(
                                            p_ev_slr_from_ev_cell(p_ev_cell, p_slr_use->by_index),
                                            p_docu,
                                            t5_message,
                                            p_uref_event_block);

                                if(res == DEP_UPDATE)
                                    ev_doc_modify(ev_slr_docno(&p_slr_use->slr_by));
                            }

                            p_ss_doc->slr_table.sorted = 0;
                        }
                    }
                }

                break;
                }
            }
        }
    }
    } /*block*/

    /* look through the name use table */
    switch(t5_message)
    {
    case T5_MSG_UREF_CHANGE:
        break;

    default: default_unhandled();
#if CHECKING
    case T5_MSG_UREF_UREF:
    case T5_MSG_UREF_DELETE:
    case T5_MSG_UREF_SWAP_ROWS:
    case T5_MSG_UREF_OVERWRITE:
    case T5_MSG_UREF_CLOSE1:
    case T5_MSG_UREF_CLOSE2:
#endif
        {
        P_NAME_USE p_name_use;
        ARRAY_INDEX name_ix;
        BOOL check_use = 0, un_sort = 0;

        name_list_sort();

        for(name_ix = 0, p_name_use = array_ptr(&name_use_deptable.h_table, NAME_USE, name_ix);
            name_ix < array_elements(&name_use_deptable.h_table);
            ++name_ix, ++p_name_use)
        {
            S32 res;

            if(p_name_use->flags.tobedel)
                continue;

            /* refs contained by deleted area must be removed */
            if((res = ev_uref_match_slr(&p_name_use->slr_by, p_docu, t5_message, p_uref_event_block)) != DEP_NONE)
            {
                if(res == DEP_DELETE || t5_message == T5_MSG_UREF_OVERWRITE)
                {
                    p_name_use->flags.tobedel = 1;
                    name_use_deptable.flags.tobedel = 1;
                    name_use_deptable.mindel = MIN(name_use_deptable.mindel, name_ix);
                    check_use = 1;
                }
                else
                    un_sort = 1;
            }

            /* set these flags at end to avoid sorts in the middle of our fuddle */
            if(check_use)
                name_def.flags.checkuse = 1;
            if(un_sort)
                name_use_deptable.sorted = 0;
        }

        break;
        }
    }

    /* look through name definition table */
    switch(t5_message)
    {
    case T5_MSG_UREF_CHANGE:
        break;

    default: default_unhandled();
#if CHECKING
    case T5_MSG_UREF_UREF:
    case T5_MSG_UREF_DELETE:
    case T5_MSG_UREF_SWAP_ROWS:
    case T5_MSG_UREF_OVERWRITE:
    case T5_MSG_UREF_CLOSE1:
    case T5_MSG_UREF_CLOSE2:
#endif
        {
        ARRAY_INDEX name_ix;
        P_EV_NAME p_ev_name;

        for(name_ix = 0, p_ev_name = array_ptr(&name_def.h_table, EV_NAME, name_ix);
            name_ix < array_elements(&name_def.h_table);
            ++name_ix, ++p_ev_name)
        {
            S32 res = DEP_NONE;

            if(p_ev_name->flags.tobedel)
                continue;

            /* check for name definition in the area */
            if(t5_message != T5_MSG_UREF_CHANGE
               &&
               (res = ev_uref_match_slr(&p_ev_name->owner, p_docu, t5_message, p_uref_event_block)) != DEP_NONE)
            {
                if(res == DEP_DELETE || t5_message == T5_MSG_UREF_OVERWRITE)
                {
                    /* delete name definition
                     * if there are no dependents
                     */
                    name_free_resources(p_ev_name);

                    if(!ev_todo_add_name_dependents(p_ev_name->handle))
                    {
                        P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_slr_docno(&p_ev_name->owner));

                        if(P_DATA_NONE != p_ss_doc)
                            p_ss_doc->nam_ref_count -= 1;

                        p_ev_name->flags.tobedel = 1;
                        name_def.flags.tobedel = 1;
                        name_def.mindel = MIN(name_def.mindel, name_ix);

                        trace_1(TRACE_MODULE_UREF, TEXT("uref deleting name: %s"), report_ustr(ustr_bptr(p_ev_name->ustr_name_id)));
                    }

                    /* it's definitely undefined now, anyway */
                    p_ev_name->flags.undefined = 1;
                }
            }

            /* check for the definition of the name
             * itself being in the uref area
             */
            if(!p_ev_name->flags.undefined)
            {
                switch(p_ev_name->def_data.did_num)
                {
                case RPN_DAT_SLR:
                    if(ev_uref_match_slr(&p_ev_name->def_data.arg.slr, p_docu, t5_message, p_uref_event_block) != DEP_NONE)
                    {
                        if(t5_message != T5_MSG_UREF_CHANGE)
                        {
                            if(res == DEP_UPDATE)
                                ev_doc_modify(ev_slr_docno(&p_ev_name->owner));
                        }
                        ev_todo_add_name_dependents(p_ev_name->handle);
                    }
                    break;

                case RPN_DAT_RANGE:
                    if(ev_uref_match_range(&p_ev_name->def_data.arg.range, p_docu, t5_message, p_uref_event_block) != DEP_NONE)
                    {
                        if(t5_message != T5_MSG_UREF_CHANGE)
                        {
                            if(res == DEP_UPDATE)
                                ev_doc_modify(ev_slr_docno(&p_ev_name->owner));
                        }
                        ev_todo_add_name_dependents(p_ev_name->handle);
                    }
                    break;
                }
            }
        }

        break;
        }
    }

    /* look through the event use table */
    switch(t5_message)
    {
    case T5_MSG_UREF_CHANGE:
        break;

    default: default_unhandled();
#if CHECKING
    case T5_MSG_UREF_UREF:
    case T5_MSG_UREF_DELETE:
    case T5_MSG_UREF_SWAP_ROWS:
    case T5_MSG_UREF_OVERWRITE:
    case T5_MSG_UREF_CLOSE1:
    case T5_MSG_UREF_CLOSE2:
#endif
        {
        P_EVENT_USE p_event_use;
        ARRAY_INDEX event_ix;

        tree_sort_events();

        for(event_ix = 0, p_event_use = array_ptr(&event_use_deptable.h_table, EVENT_USE, event_ix);
            event_ix < array_elements(&event_use_deptable.h_table);
            ++event_ix, ++p_event_use)
        {
            S32 res;

            if(p_event_use->flags.tobedel)
                continue;

            /* event uses contained by deleted area must be removed */
            if((res = ev_uref_match_slr(&p_event_use->slr_by, p_docu, t5_message, p_uref_event_block)) != DEP_NONE)
            {
                if(res == DEP_DELETE || t5_message == T5_MSG_UREF_OVERWRITE)
                {
                    p_event_use->flags.tobedel = 1;
                    event_use_deptable.flags.tobedel = 1;
                    event_use_deptable.mindel = MIN(event_use_deptable.mindel, event_ix);
                }
            }
        }

        break;
        }
    }

    /* look through the custom function use table */
    switch(t5_message)
    {
    case T5_MSG_UREF_CHANGE:
        break;

    default: default_unhandled();
#if CHECKING
    case T5_MSG_UREF_UREF:
    case T5_MSG_UREF_DELETE:
    case T5_MSG_UREF_SWAP_ROWS:
    case T5_MSG_UREF_OVERWRITE:
    case T5_MSG_UREF_CLOSE1:
    case T5_MSG_UREF_CLOSE2:
#endif
        {
        P_CUSTOM_USE p_custom_use;
        ARRAY_INDEX custom_ix;
        BOOL check_use = 0, un_sort = 0;

        custom_list_sort();

        for(custom_ix = 0, p_custom_use = array_ptr(&custom_use_deptable.h_table, CUSTOM_USE, custom_ix);
            custom_ix < array_elements(&custom_use_deptable.h_table);
            ++custom_ix, ++p_custom_use)
        {
            S32 res;

            if(p_custom_use->flags.tobedel)
                continue;

            /* macro uses contained by deleted area must be removed */
            if((res = ev_uref_match_slr(&p_custom_use->slr_by, p_docu, t5_message, p_uref_event_block)) != DEP_NONE)
            {
                if(res == DEP_DELETE || t5_message == T5_MSG_UREF_OVERWRITE)
                {
                    p_custom_use->flags.tobedel = 1;
                    custom_use_deptable.flags.tobedel = 1;
                    custom_use_deptable.mindel = MIN(custom_use_deptable.mindel, custom_ix);
                    check_use = 1;
                }
                else
                    un_sort = 1;
            }

            /* set these flags at end to avoid sorts
             * in the middle of our fuddle
             */
            if(check_use)
                custom_def.flags.checkuse = 1;
            if(un_sort)
                custom_use_deptable.sorted = 0;
        }

        break;
        }
    }

    /* update custom function definition table */
    switch(t5_message)
    {
    case T5_MSG_UREF_CHANGE:
        break;

    default: default_unhandled();
#if CHECKING
    case T5_MSG_UREF_UREF:
    case T5_MSG_UREF_DELETE:
    case T5_MSG_UREF_SWAP_ROWS:
    case T5_MSG_UREF_OVERWRITE:
    case T5_MSG_UREF_CLOSE1:
    case T5_MSG_UREF_CLOSE2:
#endif
        {
        ARRAY_INDEX custom_ix;
        P_EV_CUSTOM p_ev_custom;

        for(custom_ix = 0, p_ev_custom = array_ptr(&custom_def.h_table, EV_CUSTOM, custom_ix);
            custom_ix < array_elements(&custom_def.h_table);
            ++custom_ix, ++p_ev_custom)
        {
            S32 res;

            if(p_ev_custom->flags.tobedel)
                continue;

            /* check for macro definition in the area */
            if((res = ev_uref_match_slr(&p_ev_custom->owner, p_docu, t5_message, p_uref_event_block)) != DEP_NONE)
            {
                if(res == DEP_DELETE || t5_message == T5_MSG_UREF_OVERWRITE)
                {
                    /* delete macro table entry
                     * if there are no dependents
                    */
                    if(!ev_todo_add_custom_dependents(p_ev_custom->handle))
                    {
                        P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_slr_docno(&p_ev_custom->owner));

                        if(P_DATA_NONE != p_ss_doc)
                            p_ss_doc->custom_ref_count -= 1;

                        p_ev_custom->flags.tobedel = 1;
                        custom_def.flags.tobedel = 1;
                        custom_def.mindel = MIN(custom_def.mindel, custom_ix);

                        trace_1(TRACE_MODULE_UREF, TEXT("uref deleting custom: %s"), report_ustr(ustr_bptr(p_ev_custom->ustr_custom_id)));
                    }

                    /* it's definitely undefined now, anyway */
                    p_ev_custom->owner.col = EV_COL_PACK(EV_MAX_COL - 1);
                    p_ev_custom->owner.row =            (EV_MAX_ROW - 1);
                    p_ev_custom->flags.undefined = 1;
                }
            }
        }

        break;
        }
    }

    /* try clearing up documents */
    if(T5_MSG_UREF_CLOSE2 == t5_message)
    {
        DOCNO docno = DOCNO_NONE;

        tree_sort_all();

        while(DOCNO_NONE != (docno = docno_enum_thunks(docno)))
        {
            const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(docno);

            if(P_DATA_NONE == p_ss_doc)
                continue;

            if(p_ss_doc->uref_handle && ev_doc_is_thunk(docno))
            {
                if(!p_ss_doc->slr_table.h_table   &&
                   !p_ss_doc->range_table.h_table &&
                   !p_ss_doc->nam_ref_count       &&
                   !p_ss_doc->custom_ref_count)
                {
                    uref_del_dependency(docno, p_ss_doc->uref_handle);
                    p_ss_doc->uref_handle = 0;
                }
            }
        }
    }

    trace_0(TRACE_MODULE_UREF, TEXT("| -- out"));

    return(status);
}

/* end of ev_uref.c */
