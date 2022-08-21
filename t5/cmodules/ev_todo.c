/* ev_todo.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Routines to manage the todo list */

/* MRJC April 1993 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

/*
local data
*/

ARRAY_HANDLE h_todo_list = 0;

static U8 todo_sort_order = TODO_SORTED_NOT;

ARRAY_HANDLE h_needs_recalc = 0;

/******************************************************************************
*
* needs_recalc map handling
*
******************************************************************************/

typedef struct NR_MAP_BLOCK
{
    ARRAY_HANDLE h_flags;
    S32 rows;
    S32 cols;
}
NR_MAP_BLOCK, * P_NR_MAP_BLOCK; typedef const NR_MAP_BLOCK * PC_NR_MAP_BLOCK;

static ARRAY_HANDLE h_nr_maps = 0;

_Check_return_
static STATUS
needs_recalc_add_to_maps(
    _InRef_     PC_EV_SLR p_ev_slr)
{
    STATUS status = STATUS_OK;
    const EV_DOCNO ev_docno = ev_slr_docno(p_ev_slr);

    if(!ev_doc_check_custom(ev_docno))
    {
        const ARRAY_INDEX n_nr_maps = array_elements(&h_nr_maps);

        if(n_nr_maps < ((ARRAY_INDEX) ev_docno + 1))
        {
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(5, sizeof32(NR_MAP_BLOCK), TRUE);
            consume_ptr(al_array_extend_by(&h_nr_maps, NR_MAP_BLOCK, ((ARRAY_INDEX) ev_docno + 1) - n_nr_maps, &array_init_block, &status));
        }

        if(status_ok(status))
        {
            const P_NR_MAP_BLOCK p_nr_map_block = array_ptr(&h_nr_maps, NR_MAP_BLOCK, ev_docno);

            if(ev_slr_row(p_ev_slr) >= p_nr_map_block->rows)
            {
                SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(U8), TRUE);

                if(!p_nr_map_block->h_flags)
                    p_nr_map_block->cols = ev_numcol_phys(ev_docno);

                if(NULL != al_array_extend_by_U8(&p_nr_map_block->h_flags, p_nr_map_block->cols * (ev_slr_row(p_ev_slr) - p_nr_map_block->rows + 1), &array_init_block, &status))
                    p_nr_map_block->rows = ev_slr_row(p_ev_slr) + (S32) 1;

#if TRACE_ALLOWED
                if_constant(tracing(TRACE_APP_MEMORY_USE))
                {
                    TCHARZ tstr_buf[64 + BUF_EV_LONGNAMLEN];
                    ev_trace_slr_tstr_buf(tstr_buf, elemof32(tstr_buf), TEXT("needs_recalc_add_to_maps adding: $$, now ") S32_TFMT TEXT(" bytes"), p_ev_slr);
                    trace_v1(TRACE_APP_MEMORY_USE, tstr_buf, array_elements(&p_nr_map_block->h_flags) * sizeof32(U8));
                }
#endif
            }

            if(status_ok(status) && (ev_slr_row(p_ev_slr) < p_nr_map_block->rows))
            {
                P_U8 p_u8 = array_ptr(&p_nr_map_block->h_flags, U8, (p_nr_map_block->cols * ev_slr_row(p_ev_slr)) + ev_slr_col(p_ev_slr));
                *p_u8 = 1;
            }
        }
    }

    return(status);
}

static void
needs_recalc_remove(
    _InRef_     PC_EV_SLR p_ev_slr)
{
    const EV_DOCNO ev_docno = ev_slr_docno(p_ev_slr);

    if(array_offset_is_valid(&h_nr_maps, ev_docno))
    {
        const PC_NR_MAP_BLOCK p_nr_map_block = array_ptrc(&h_nr_maps, NR_MAP_BLOCK, ev_docno);

        if(ev_slr_row(p_ev_slr) < p_nr_map_block->rows)
        {
            P_U8 p_u8 = array_ptr(&p_nr_map_block->h_flags, U8, (p_nr_map_block->cols * ev_slr_row(p_ev_slr)) + ev_slr_col(p_ev_slr));
            *p_u8 = 0;
        }
    }
}

/******************************************************************************
*
* compare routine for todo node sort
*
******************************************************************************/

PROC_QSORT_PROTO(static, todo_compare_node, P_TODO_ENTRY)
{
    QSORT_ARG1_VAR_DECL(P_TODO_ENTRY, p_todo_entry_1);
    QSORT_ARG2_VAR_DECL(P_TODO_ENTRY, p_todo_entry_2);

    if(p_todo_entry_1->node_distance > p_todo_entry_2->node_distance)
        return(-1);
    if(p_todo_entry_1->node_distance < p_todo_entry_2->node_distance)
        return(1);

    return(0);
}

/******************************************************************************
*
* sort todo table in reverse order of node distance
*
******************************************************************************/

static void
todo_sort_nodes(void)
{
    if(todo_sort_order != TODO_SORTED_NODE)
    {
        ARRAY_INDEX n_todo = array_elements(&h_todo_list);

        al_array_qsort(&h_todo_list, todo_compare_node);

        todo_sort_order = TODO_SORTED_NODE;

        /* remove duds (marked as -1) */
        if(0 != n_todo)
        {
            ARRAY_INDEX todo_ix = (n_todo - 1);
            P_TODO_ENTRY p_todo_entry = array_ptr(&h_todo_list, TODO_ENTRY, todo_ix);

            while(todo_ix && p_todo_entry[0].node_distance < 0)
            {
                --todo_ix;
                --p_todo_entry;
            }

            al_array_shrink_by(&h_todo_list, todo_ix - (n_todo - 1));

#if TRACE_ALLOWED
            if(n_todo - todo_ix > 1)
                trace_1(TRACE_MODULE_EVAL, TEXT("todo_sort_nodes ") S32_TFMT TEXT(" to do"), array_elements(&h_todo_list));
#endif
        }
    }
}

/******************************************************************************
*
* mark all dependents (users of a custom) for recalc
*
******************************************************************************/

/*ncr*/
extern BOOL
ev_todo_add_custom_dependents(
    _InVal_     EV_HANDLE h_custom)
{
    BOOL found_use = 0;
    const ARRAY_INDEX custom_num = custom_def_find(h_custom);

    if(custom_num >= 0)
    {
        DOCNO owner_docno = (DOCNO) ev_slr_docno(&array_ptrc(&custom_def.h_table, EV_CUSTOM, custom_num)->owner);
        ARRAY_INDEX custom_ix = search_for_custom_use(h_custom);

        if(custom_ix >= 0)
        {
            const ARRAY_INDEX custom_use_elements = array_elements(&custom_use_deptable.h_table);
            PC_CUSTOM_USE p_custom_use = array_ptr(&custom_use_deptable.h_table, CUSTOM_USE, custom_ix);

            for( ; custom_ix < custom_use_elements; ++custom_ix, ++p_custom_use)
            {
                if(p_custom_use->custom_to != h_custom)
                    break;

                if(p_custom_use->flags.to_be_deleted)
                    continue;

                if(ev_slr_docno(&p_custom_use->slr_by) != owner_docno)
                    ev_todo_add_slr(&p_custom_use->slr_by);
            }

            found_use = 1;
        }
    }

    return(found_use);
}

/******************************************************************************
*
* look for names which refer to a given cell and add their dependents to be done
*
* set ev_docno to avoid self-dependencies
*
******************************************************************************/

static void
todo_add_name_deps_of_slr(
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     EV_DOCNO ev_docno)
{
    const ARRAY_INDEX n_elements = array_elements(&name_def.h_table);
    ARRAY_INDEX i;
    P_EV_NAME p_ev_name = array_range(&name_def.h_table, EV_NAME, 0, n_elements);

    /* look for name references */
    for(i = 0; i < n_elements; ++i, ++p_ev_name)
    {
        if(p_ev_name->flags.to_be_deleted)
            continue;

        if(ev_slr_docno(&p_ev_name->owner) != ev_docno)
        {
            BOOL got_ref = FALSE;

            switch(ss_data_get_data_id(&p_ev_name->def_data))
            {
            case DATA_ID_SLR:
                if(ev_slr_equal(&p_ev_name->def_data.arg.slr, p_ev_slr))
                    got_ref = TRUE;
                break;

            case DATA_ID_RANGE:
                if(ev_slr_in_range(&p_ev_name->def_data.arg.range, p_ev_slr))
                    got_ref = TRUE;
                break;
            }

            /* mark all uses of name */
            if(got_ref)
                ev_todo_add_name_dependents(p_ev_name->handle);
        }
    }
}

/******************************************************************************
*
* mark all dependents of the given docno for recalc
*
******************************************************************************/

extern void
ev_todo_add_doc_dependents(
    _InVal_     EV_DOCNO ev_docno)
{
    {
    const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_docno);

    if(P_DATA_NONE != p_ss_doc)
    {
        { /* look thru range dependencies */
        ARRAY_INDEX range_table_elements = array_elements(&p_ss_doc->range_table.h_table);
        ARRAY_INDEX range_ix;
        P_RANGE_USE p_range_use;

        for(range_ix = 0, p_range_use = p_range_use_from_p_ss_doc(p_ss_doc, range_ix);
            range_ix < range_table_elements;
            ++range_ix, ++p_range_use)
        {
            if(p_range_use->flags.to_be_deleted)
                continue;

            if(ev_slr_docno(&p_range_use->slr_by) != ev_docno)
                ev_todo_add_slr(&p_range_use->slr_by);
        }
        } /*block*/

        { /* add slr dependencies */
        const ARRAY_INDEX slr_table_elements = array_elements(&p_ss_doc->slr_table.h_table);
        ARRAY_INDEX slr_ix;
        P_SLR_USE p_slr_use;

        for(slr_ix = 0, p_slr_use = p_slr_use_from_p_ss_doc(p_ss_doc, slr_ix);
            slr_ix < slr_table_elements;
            ++slr_ix, ++p_slr_use)
        {
            if(p_slr_use->flags.to_be_deleted)
                continue;

            if(ev_slr_docno(&p_slr_use->slr_by) != ev_docno)
                ev_todo_add_slr(&p_slr_use->slr_by);
        }
        } /*block*/
    }
    } /*block*/

    { /* add dependents of names referencing this document */
    EV_SLR slr;
    const ARRAY_INDEX custom_table_elements = array_elements(&custom_def.h_table);
    ARRAY_INDEX i;
    P_EV_CUSTOM p_ev_custom;

    slr.docno = EV_DOCNO_PACK(ev_docno);

    todo_add_name_deps_of_slr(&slr, ev_docno);

    for(i = 0, p_ev_custom = array_ptr(&custom_def.h_table, EV_CUSTOM, i);
        i < custom_table_elements;
        ++i, ++p_ev_custom)
    {
        if(p_ev_custom->flags.to_be_deleted)
            continue;

        if(p_ev_custom->flags.undefined)
            continue;

        if(ev_slr_docno(&p_ev_custom->owner) == ev_docno)
            ev_todo_add_custom_dependents(p_ev_custom->handle);
    }
    } /*block*/
}

/******************************************************************************
*
* mark all dependents (users of a name) for recalc
*
******************************************************************************/

/*ncr*/
extern BOOL /* found a use */
ev_todo_add_name_dependents(
    _InVal_     EV_HANDLE h_name)
{
    BOOL found_use = 0;
    ARRAY_INDEX name_ix = search_for_name_use(h_name);

    if(name_ix >= 0)
    {
        const ARRAY_INDEX name_use_elements = array_elements(&name_use_deptable.h_table);
        P_NAME_USE p_name_use = array_ptr(&name_use_deptable.h_table, NAME_USE, name_ix);

        for( ; name_ix < name_use_elements; ++name_ix, ++p_name_use)
        {
            if(p_name_use->name_to.h_name != h_name)
                break;

            if(p_name_use->flags.to_be_deleted)
                continue;

            ev_todo_add_slr(&p_name_use->slr_by);
        }

        found_use = 1;
    }

    if(ev_tell_name_clients(h_name, TRUE /* changed */))
        found_use = 1;

    return(found_use);
}

/******************************************************************************
*
* add slr to list of modified cells
*
******************************************************************************/

extern void
ev_todo_add_slr(
    _InRef_     PC_EV_SLR p_ev_slr)
{
    const EV_DOCNO ev_docno = ev_slr_docno(p_ev_slr);
    P_NEEDS_RECALC p_needs_recalc;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(200, sizeof32(NEEDS_RECALC), FALSE);
    STATUS status;

    if(ev_doc_check_custom(ev_docno))
        return;

    assert(ev_slr_col(p_ev_slr) <= ev_numcol(ev_docno));
    assert(ev_slr_row(p_ev_slr) <= ev_numrow(ev_docno));

    if(NULL == (p_needs_recalc = al_array_extend_by(&h_needs_recalc, NEEDS_RECALC, 1, &array_init_block, &status)))
    {
        status_assert(status);
        return;
    }

    p_needs_recalc->ev_slr = *p_ev_slr;
    p_needs_recalc->ev_slr.bad_ref = 0;

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_MODULE_EVAL))
    {
        TCHARZ tstr_buf[64 + BUF_EV_LONGNAMLEN];
        ev_trace_slr_tstr_buf(tstr_buf, elemof32(tstr_buf), TEXT("ev_todo_add_slr added: $$, ") S32_TFMT TEXT(" entries"), &p_needs_recalc->ev_slr);
        trace_v1(TRACE_MODULE_EVAL, tstr_buf, array_elements(&h_needs_recalc));
    }
#endif

    /* switch on background recalc */
    if(!g_ev_recalc_started)
        ev_recalc_start(FALSE);
}

typedef struct DEP_ADD_STACK
{
    S32 state;
    EV_SLR slr;
    S32 index;
    union
    {
        EV_HANDLE h_name;
        S32 index_circ;
    } arg;
}
DEP_ADD_STACK, * P_DEP_ADD_STACK, ** P_P_DEP_ADD_STACK;

enum DEP_ADD_CODES
{
    DEP_ADD_SLR,
    DEP_ADD_RANGE,
    DEP_ADD_NAME,
    DEP_ADD_NAME_DEP
};

static ARRAY_HANDLE h_dep_add_stack = 0;

static P_DEP_ADD_STACK p_dep_add_stack = NULL;

static S32 node_distance = 0;

/******************************************************************************
*
* add a cell to the todo list
* this is called when a cell is modified;
* all the dependents of the cell are added to
* the list at this time; the flag indicates
* whether the cell itself should be added
* to the list
*
******************************************************************************/

_Check_return_
static STATUS
todo_add_to_list(
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     S32 node_gap)
{
    STATUS status = STATUS_OK;
    P_TODO_ENTRY p_todo_entry;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(200, sizeof32(TODO_ENTRY), FALSE);

    if(NULL != (p_todo_entry = al_array_extend_by(&h_todo_list, TODO_ENTRY, 1, &array_init_block, &status)))
    {
        p_todo_entry->slr = *p_ev_slr;
        p_todo_entry->node_distance = MAX(0, node_gap);
        if(node_gap < 0)
            p_todo_entry->slr.circ = 1;
        todo_sort_order = TODO_SORTED_NOT;

#if TRACE_ALLOWED
        if_constant(tracing(TRACE_MODULE_EVAL))
        {
            TCHARZ tstr_buf[64 + BUF_EV_LONGNAMLEN];
            ev_trace_slr_tstr_buf(tstr_buf, elemof32(tstr_buf), TEXT("todo_add_to_list added: $$, ") S32_TFMT TEXT(" entries, ") S32_TFMT TEXT(" bytes, node: ") S32_TFMT, p_ev_slr);
            trace_v3(TRACE_MODULE_EVAL,
                    tstr_buf,
                    array_elements(&h_todo_list),
                    array_elements(&h_todo_list) * sizeof32(TODO_ENTRY),
                    p_todo_entry->node_distance);
        }
#endif
    }

    return(status);
}

typedef struct TODO_MAP_BLOCK
{
    ARRAY_HANDLE h_nodes;
    S32 rows;
    S32 cols;
}
TODO_MAP_BLOCK, * P_TODO_MAP_BLOCK; typedef const TODO_MAP_BLOCK * PC_TODO_MAP_BLOCK;

static ARRAY_HANDLE h_todo_maps = 0;

_Check_return_
static STATUS /* node gap was bigger */
todo_add_to_maps(
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     S32 node_gap)
{
    const EV_DOCNO ev_docno = ev_slr_docno(p_ev_slr);
    STATUS status = STATUS_OK;

    if(!ev_doc_check_custom(ev_docno))
    {
        const P_ARRAY_HANDLE p_h_todo_maps = &h_todo_maps;
        ARRAY_INDEX n_todo_maps = array_elements(p_h_todo_maps);

        if(n_todo_maps < ((ARRAY_INDEX) ev_docno + 1))
        {
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(5, sizeof32(TODO_MAP_BLOCK), TRUE);
            consume_ptr(al_array_extend_by(p_h_todo_maps, TODO_MAP_BLOCK, ((ARRAY_INDEX) ev_docno + 1) - n_todo_maps, &array_init_block, &status));
        }

        if(status_ok(status))
        {
            const P_TODO_MAP_BLOCK p_todo_map_block = array_ptr(p_h_todo_maps, TODO_MAP_BLOCK, ev_docno);

            if(ev_slr_row(p_ev_slr) >= p_todo_map_block->rows)
            {
                SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(S16), TRUE);

                if(!p_todo_map_block->h_nodes)
                    p_todo_map_block->cols = ev_numcol_phys(ev_docno);

                if(NULL != al_array_extend_by(&p_todo_map_block->h_nodes, S16, p_todo_map_block->cols * (ev_slr_row(p_ev_slr) - p_todo_map_block->rows + 1), &array_init_block, &status))
                    p_todo_map_block->rows = ev_slr_row(p_ev_slr) + 1;

#if TRACE_ALLOWED
                if_constant(tracing(TRACE_APP_MEMORY_USE))
                {
                    TCHARZ tstr_buf[64 + BUF_EV_LONGNAMLEN];
                    ev_trace_slr_tstr_buf(tstr_buf, elemof32(tstr_buf), TEXT("todo_add_to_maps adding: $$, now ") S32_TFMT TEXT(" bytes"), p_ev_slr);
                    trace_v1(TRACE_APP_MEMORY_USE, tstr_buf, array_elements(&p_todo_map_block->h_nodes) * sizeof32(S16));
                }
#endif
            }

#if 0
            if(ev_slr_row(p_ev_slr) == 57)
            {
                int x = 1;
                x = 0;
            }
#endif

            if(status_ok(status) && (ev_slr_row(p_ev_slr) < p_todo_map_block->rows))
            {
                P_S16 p_s16 = array_ptr(&p_todo_map_block->h_nodes, S16, (p_todo_map_block->cols * ev_slr_row(p_ev_slr)) + ev_slr_col(p_ev_slr));
                if((node_gap + 1 > *p_s16) || p_ev_slr->circ)
                {
                    if(p_ev_slr->circ)
                        *p_s16 = -1;
                    else
                        *p_s16 = (S16) (node_gap + 1);

                    status = 1;
                }
            }
        }
    }

    if(status_ok(status))
        needs_recalc_remove(p_ev_slr);

    return(status);
}

_Check_return_
static STATUS /* node gap was bigger */
todo_find_in_maps(
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     S32 node_gap)
{
    STATUS status = STATUS_OK;
    const P_ARRAY_HANDLE p_h_todo_maps = &h_todo_maps;
    const ARRAY_INDEX n_todo_maps = array_elements(p_h_todo_maps);
    const EV_DOCNO ev_docno = ev_slr_docno(p_ev_slr);

    if(n_todo_maps > (ARRAY_INDEX) ev_docno)
    {
        const PC_TODO_MAP_BLOCK p_todo_map_block = array_ptrc(p_h_todo_maps, TODO_MAP_BLOCK, ev_docno);

        if(ev_slr_row(p_ev_slr) < p_todo_map_block->rows)
        {
            P_S16 p_s16 = array_ptr(&p_todo_map_block->h_nodes, S16, (p_todo_map_block->cols * ev_slr_row(p_ev_slr)) + ev_slr_col(p_ev_slr));
            if(node_gap + 1 <= *p_s16)
                status = STATUS_DONE;
        }
    }

    return(status);
}

_Check_return_
static STATUS
todo_list_from_map_block(
    _InRef_     PC_TODO_MAP_BLOCK p_todo_map_block,
    _InVal_     ARRAY_INDEX ev_docno)
{
    PC_S16 p_s16 = array_base(&p_todo_map_block->h_nodes, S16);
    EV_SLR ev_slr;

    ev_slr_flags_init(&ev_slr);
    ev_slr_docno_set(&ev_slr, ev_docno);

    for(ev_slr.row = 0; ev_slr_row(&ev_slr) < p_todo_map_block->rows; ++ev_slr.row)
    {
        const S32 n_map_block_cols = p_todo_map_block->cols;
        EV_COL ev_col;

        for(ev_col = 0; ev_col < n_map_block_cols; ++ev_col, ++p_s16)
        {
            const S16 node_gap = *p_s16;

            if(0 == node_gap)
                continue;

            ev_slr_col_set(&ev_slr, ev_col);

            status_return(todo_add_to_list(&ev_slr, node_gap));
        }
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
todo_list_from_maps(void)
{
    STATUS status = STATUS_OK;
    const P_ARRAY_HANDLE p_h_todo_maps = &h_todo_maps;
    const ARRAY_INDEX n_todo_maps = array_elements(p_h_todo_maps);
    ARRAY_INDEX i;

    for(i = DOCNO_FIRST; i < n_todo_maps; ++i)
    {
        const P_TODO_MAP_BLOCK p_todo_map_block = array_ptr(p_h_todo_maps, TODO_MAP_BLOCK, i);

        if(0 == p_todo_map_block->h_nodes)
            continue;

        status = todo_list_from_map_block(p_todo_map_block, i);

        al_array_dispose(&p_todo_map_block->h_nodes);

        status_break(status);
    }

    al_array_dispose(p_h_todo_maps);

    return(status);
}

/*
set the circular bit for all cells in a loop
*/

static void
dep_add_circ_unravel(
    _InRef_     PC_EV_SLR p_ev_slr)
{
    P_DEP_ADD_STACK p_dep_add_stack_base = array_base(&h_dep_add_stack, DEP_ADD_STACK), p_dep_add_stack_t = p_dep_add_stack;

    p_dep_add_stack_t->slr.circ = 1;
    do  {
        --p_dep_add_stack_t;
        if(p_dep_add_stack_t->slr.circ == 1)
            break;
        p_dep_add_stack_t->slr.circ = 1;
    }
    while(!ev_slr_equal(&p_dep_add_stack_t->slr, p_ev_slr) && p_dep_add_stack_t > p_dep_add_stack_base);
}

static void
dep_add_stack_dec(void)
{
    al_array_shrink_by(&h_dep_add_stack, -1);

    {
    ARRAY_INDEX n_stack = array_elements(&h_dep_add_stack);
    if(n_stack)
        p_dep_add_stack = array_ptr(&h_dep_add_stack, DEP_ADD_STACK, n_stack - 1);
    } /*block*/
}

_Check_return_ _Success_(return > 0)
static STATUS /* <0 error, =0 not added, >0 added to stack */
dep_add_stack_inc(
    _OutRef_    P_P_DEP_ADD_STACK p_p_dep_add_stack,
    _InVal_     S32 state,
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     S32 node_gap)
{
    STATUS status = 0;

    if(todo_find_in_maps(p_ev_slr, node_gap) <= 0)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(50, sizeof32(DEP_ADD_STACK), FALSE);
        S32 n_sub = 0;

        if(0 != h_dep_add_stack)
            n_sub = 1;

        if(NULL == (p_dep_add_stack = al_array_extend_by(&h_dep_add_stack, DEP_ADD_STACK, 1, &array_init_block, &status)))
        {
            __analysis_assume(status < 0);
            return(status);
        }

        {
#if TRACE_ALLOWED
            {
            static ARRAY_INDEX max_stack = 0;
            ARRAY_INDEX old_max = max_stack;
            max_stack = MAX(max_stack, array_elements(&h_dep_add_stack));
            if(max_stack > old_max)
                trace_1(TRACE_MODULE_EVAL, TEXT("todo_add_slr stack now ") S32_TFMT TEXT(" elements"), max_stack);
            } /*block*/
#endif

            p_dep_add_stack->state = state;
            p_dep_add_stack->slr = *p_ev_slr;
            p_dep_add_stack->index = -1;
            p_dep_add_stack->arg.index_circ = -1;

            *p_p_dep_add_stack = p_dep_add_stack - n_sub;
            status = 1;
        } /*block*/
    }

    return(status);
}

_Check_return_
static STATUS
dep_add_name(void)
{
    STATUS status = STATUS_OK;
    PC_EV_NAME p_ev_name;
    ARRAY_INDEX name_ix;

    { /* look for references by a name to the cell */
    const ARRAY_INDEX n_elements = array_elements(&name_def.h_table); /* SKS 19apr95 goes big document speed tweaky */

    for(name_ix = MAX(p_dep_add_stack->index, 0), p_ev_name = array_ptr(&name_def.h_table, EV_NAME, name_ix);
        name_ix < n_elements;
        ++name_ix, ++p_ev_name)
    {
        if(!p_ev_name->flags.to_be_deleted)
        {
            BOOL got_ref = FALSE;

            switch(ss_data_get_data_id(&p_ev_name->def_data))
            {
            case DATA_ID_SLR:
                if(ev_slr_equal(&p_ev_name->def_data.arg.slr, &p_dep_add_stack->slr))
                    got_ref = TRUE;
                break;

            case DATA_ID_RANGE:
                if(ev_slr_in_range(&p_ev_name->def_data.arg.range, &p_dep_add_stack->slr))
                    got_ref = TRUE;
                break;
            }

            /* mark all uses of name */
            if(got_ref)
            {
                P_DEP_ADD_STACK p_dep_add_stack_old;
                if((status = dep_add_stack_inc(&p_dep_add_stack_old, DEP_ADD_NAME_DEP, &p_dep_add_stack->slr, node_distance)) > 0)
                {
                    p_dep_add_stack_old->index = name_ix + 1;
                    p_dep_add_stack->arg.h_name = p_ev_name->handle;
                    return(status);
                }
                else if(status_fail(status)) /* SKS 19apr95 moved from loop condition for Mr Speedy */
                    break;
                assert(n_elements == array_elements(&name_def.h_table));
            }
        }
    }
    } /*block*/

    /* all dependents now added - add ourselves */
    if(status_ok(status))
        status = todo_add_to_maps(&p_dep_add_stack->slr, node_distance);

    if(status_ok(status))
    {
        dep_add_stack_dec();
        node_distance -= 1;
    }

    return(status);
}

_Check_return_
static STATUS
dep_add_name_dep(void)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX name_use_ix = p_dep_add_stack->index;

    if(name_use_ix < 0)
        name_use_ix = search_for_name_use(p_dep_add_stack->arg.h_name);

    if(name_use_ix >= 0)
    {
        P_NAME_USE p_name_use = array_ptr(&name_use_deptable.h_table, NAME_USE, name_use_ix);

        if(p_dep_add_stack->index > 0)
            array_ptr(&name_use_deptable.h_table, NAME_USE, p_dep_add_stack->index - 1)->flags.circ = 0;

        while(status_ok(status)
              &&
              name_use_ix < array_elements(&name_use_deptable.h_table)
              &&
              p_name_use->name_to.h_name == p_dep_add_stack->arg.h_name)
        {
            if(!p_name_use->flags.to_be_deleted)
            {
                if(p_name_use->flags.circ)
                    dep_add_circ_unravel(&p_name_use->slr_by);
                else if(!p_name_use->name_to.no_dep)
                {
                    P_DEP_ADD_STACK p_dep_add_stack_old;
                    if((status = dep_add_stack_inc(&p_dep_add_stack_old, DEP_ADD_SLR, &p_name_use->slr_by, node_distance + 1)) > 0)
                    {
                        p_name_use->flags.circ = 1;
                        p_dep_add_stack_old->index = name_use_ix + 1;
                        node_distance += 1;
                        return(status);
                    }
                }
            }

            ++p_name_use;
            ++name_use_ix;
        }
    }

    /* must revert to DEP_ADD_NAME state */
    dep_add_stack_dec();

    return(status);
}

_Check_return_
static STATUS
dep_add_range(void)
{
    STATUS status = STATUS_OK;
    const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_slr_docno(&p_dep_add_stack->slr));

    if(P_DATA_NONE == p_ss_doc)
    {
        assert0();
        return(status);
    }

    if(p_ss_doc->h_range_cols && p_ss_doc->h_range_rows)
    {
        P_ARRAY_HANDLE p_array_handle_col, p_array_handle_row;
        ARRAY_INDEX n_col, n_row;

        assert(ev_slr_col(&p_dep_add_stack->slr) < array_elements(&p_ss_doc->h_range_cols));
        assert(ev_slr_row(&p_dep_add_stack->slr) < array_elements(&p_ss_doc->h_range_rows));

        p_array_handle_col = array_ptr(&p_ss_doc->h_range_cols, ARRAY_HANDLE, ev_slr_col(&p_dep_add_stack->slr));
        p_array_handle_row = array_ptr(&p_ss_doc->h_range_rows, ARRAY_HANDLE, ev_slr_row(&p_dep_add_stack->slr));
        n_col = array_elements(p_array_handle_col);
        n_row = array_elements(p_array_handle_row);

        if(n_col && n_row)
        {
            P_RANGE_INDEX p_range_loop, p_range_search;
            ARRAY_INDEX n_loop, n_search, index = MAX(0, p_dep_add_stack->index);

            if(n_col > n_row)
            {
                p_range_search = array_base(p_array_handle_col, RANGE_INDEX);
                p_range_loop = array_ptr(p_array_handle_row, RANGE_INDEX, index);
                n_loop = n_row;
                n_search = n_col;
            }
            else
            {
                p_range_search = array_base(p_array_handle_row, RANGE_INDEX);
                p_range_loop = array_ptr(p_array_handle_col, RANGE_INDEX, index);
                n_loop = n_col;
                n_search = n_row;
            }

            if(p_dep_add_stack->arg.index_circ >= 0)
                p_range_use_from_p_ss_doc(p_ss_doc, p_dep_add_stack->arg.index_circ)->flags.circ = 0;

            while(status_ok(status) && index < n_loop)
            {
                P_RANGE_USE p_range_use = p_range_use_from_p_ss_doc(p_ss_doc, (ARRAY_INDEX) *p_range_loop);

                if(node_distance > p_range_use->node_distance
                   &&
                   bsearch(p_range_loop, p_range_search, (U32) n_search, sizeof(RANGE_INDEX), compare_range_index))
                {
                    if(p_range_use->flags.circ)
                        dep_add_circ_unravel(&p_range_use->slr_by);
                    else if(!p_range_use->range_to.s.no_dep)
                    {
                        P_DEP_ADD_STACK p_dep_add_stack_old;
                        if((status = dep_add_stack_inc(&p_dep_add_stack_old, DEP_ADD_SLR, &p_range_use->slr_by, node_distance + 1)) > 0)
                        {
                            p_range_use->flags.circ = 1;
                            p_range_use->node_distance = node_distance;
                            p_dep_add_stack_old->index = index + 1;
                            p_dep_add_stack_old->arg.index_circ = *p_range_loop;
                            node_distance += 1;
                            return(status);
                        }
                    }
                }

                index += 1;
                p_range_loop += 1;
            }
        }
    }
    else
        /* code to cope when we couldn't allocate range lookup tables */
    {
        ARRAY_INDEX range_ix = MAX(0, p_dep_add_stack->index);
        ARRAY_INDEX n_ranges = array_elements(&p_ss_doc->range_table.h_table);
        P_RANGE_USE p_range_use = p_range_use_from_p_ss_doc(p_ss_doc, range_ix);

        if(p_dep_add_stack->arg.index_circ >= 0)
            p_range_use_from_p_ss_doc(p_ss_doc, p_dep_add_stack->arg.index_circ)->flags.circ = 0;

        while(status_ok(status) && range_ix < n_ranges)
        {
            if(node_distance > p_range_use->node_distance
               &&
               ev_slr_in_range(&p_range_use->range_to, &p_dep_add_stack->slr))
            {
                if(p_range_use->flags.circ)
                    dep_add_circ_unravel(&p_range_use->slr_by);
                else if(!p_range_use->range_to.s.no_dep)
                {
                    P_DEP_ADD_STACK p_dep_add_stack_old;
                    if((status = dep_add_stack_inc(&p_dep_add_stack_old, DEP_ADD_SLR, &p_range_use->slr_by, node_distance + 1)) > 0)
                    {
                        p_range_use->flags.circ = 1;
                        p_dep_add_stack_old->index = range_ix + 1;
                        p_dep_add_stack_old->arg.index_circ = range_ix;
                        node_distance += 1;
                        return(status);
                    }
                }
            }
            ++range_ix;
            ++p_range_use;
        }
    }

    p_dep_add_stack->state = DEP_ADD_NAME;
    p_dep_add_stack->index = -1;
    p_dep_add_stack->arg.index_circ = -1;

    return(status);
}

_Check_return_
static STATUS
dep_add_slr(void)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX slr_ix = p_dep_add_stack->index;

    if(slr_ix < 0)
        slr_ix = search_for_slr_use(&p_dep_add_stack->slr);

    if(slr_ix >= 0)
    {
        const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_slr_docno(&p_dep_add_stack->slr));

        if(P_DATA_NONE != p_ss_doc)
        {
            P_SLR_USE p_slr_use = p_slr_use_from_p_ss_doc(p_ss_doc, slr_ix);
            ARRAY_INDEX n_slrs = array_elements(&p_ss_doc->slr_table.h_table);

            if(p_dep_add_stack->arg.index_circ >= 0)
                p_slr_use_from_p_ss_doc(p_ss_doc, p_dep_add_stack->arg.index_circ)->flags.circ = 0;

            while(status_ok(status)
                  &&
                  slr_ix < n_slrs
                  &&
                  ev_slr_equal(&p_slr_use->slr_to, &p_dep_add_stack->slr))
            {
                if(p_slr_use->flags.circ)
                    dep_add_circ_unravel(&p_slr_use->slr_by);
                else if(!p_slr_use->slr_to.no_dep)
                {
                    P_DEP_ADD_STACK p_dep_add_stack_old;
                    if((status = dep_add_stack_inc(&p_dep_add_stack_old, DEP_ADD_SLR, &p_slr_use->slr_by, node_distance + 1)) > 0)
                    {
                        p_slr_use->flags.circ = 1;
                        p_dep_add_stack_old->index = slr_ix + 1;
                        p_dep_add_stack_old->arg.index_circ = slr_ix;
                        node_distance += 1;
                        return(status);
                    }
                }

                ++p_slr_use;
                ++slr_ix;
            }
        }
        else
        {
            assert0();
        }
    }

    p_dep_add_stack->state = DEP_ADD_RANGE;
    p_dep_add_stack->index = -1;
    p_dep_add_stack->arg.index_circ = -1;

    return(status);
}

_Check_return_
static STATUS
todo_add_slr_and_deps(
    _InRef_     PC_EV_SLR p_ev_slr)
{
    STATUS status = STATUS_OK;
    P_DEP_ADD_STACK p_dep_add_stack_old;

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_MODULE_EVAL))
    {
        TCHARZ tstr_buf[64 + BUF_EV_LONGNAMLEN];
        ev_trace_slr_tstr_buf(tstr_buf, elemof32(tstr_buf), TEXT("todo_add_slr_and_deps: $$"), p_ev_slr);
        trace_v0(TRACE_MODULE_EVAL, tstr_buf);
    }
#endif

    node_distance = 0;

    if((status = dep_add_stack_inc(&p_dep_add_stack_old, DEP_ADD_SLR, p_ev_slr, node_distance)) > 0)
    {
        do  {
            switch(p_dep_add_stack->state)
            {
            case DEP_ADD_SLR: /* find slr dependents */
                status_accumulate(status, dep_add_slr());
                break;

            case DEP_ADD_RANGE:
                status_accumulate(status, dep_add_range());
                break;

            case DEP_ADD_NAME:
                status_accumulate(status, dep_add_name());
                break;

            case DEP_ADD_NAME_DEP:
                status = dep_add_name_dep();
                break;
            }
        }
        while(status_ok(status) && array_elements(&h_dep_add_stack));
    }

    al_array_dispose(&h_dep_add_stack);

    return(status);
}

/*
dispose of all todo resources
*/

extern void
todo_exit(void)
{
    al_array_dispose(&h_needs_recalc);
    al_array_dispose(&h_todo_list);
}

/******************************************************************************
*
* take the list of cells needing recalc and make a todo list
*
******************************************************************************/

_Check_return_
static STATUS
todo_from_needs_recalc_map_block(
    _InRef_     PC_NR_MAP_BLOCK p_nr_map_block,
    _InVal_     ARRAY_INDEX ev_docno)
{
    PC_U8 p_u8 = array_basec(&p_nr_map_block->h_flags, U8);
    EV_SLR ev_slr;

    ev_slr_flags_init(&ev_slr);
    ev_slr_docno_set(&ev_slr, ev_docno);

    for(ev_slr.row = 0; ev_slr_row(&ev_slr) < p_nr_map_block->rows; ++ev_slr.row)
    {
        const S32 n_nr_map_block_cols = p_nr_map_block->cols;
        EV_COL ev_col;

        for(ev_col = 0; ev_col < n_nr_map_block_cols; ++ev_col, ++p_u8)
        {
            if(0 == *p_u8)
                continue;

            ev_slr_col_set(&ev_slr, ev_col);

            status_return(todo_add_slr_and_deps(&ev_slr));
        }
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
todo_from_needs_recalc(void)
{
    STATUS status = STATUS_OK;

    if(0 == h_needs_recalc)
        return(STATUS_OK);

    { /* first step is to make a needs_recalc map from the needs_recalc list */
    const ARRAY_INDEX n_need_recalc = array_elements(&h_needs_recalc);
    ARRAY_INDEX i;

    for(i = 0; i < n_need_recalc; ++i)
    {
        P_NEEDS_RECALC p_needs_recalc = array_ptr(&h_needs_recalc, NEEDS_RECALC, i);

        if(!p_needs_recalc->ev_slr.bad_ref)
            status_break(status = needs_recalc_add_to_maps(&p_needs_recalc->ev_slr));
    }

    al_array_dispose(&h_needs_recalc);
    } /*block*/

    if(status_ok(status))
    {   /* next step is to loop over needs_recalc map to make todo_map */
        const ARRAY_INDEX n_nr_maps = array_elements(&h_nr_maps);
        ARRAY_INDEX i;

        for(i = DOCNO_FIRST; i < n_nr_maps; ++i)
        {
            const P_NR_MAP_BLOCK p_nr_map_block = array_ptr(&h_nr_maps, NR_MAP_BLOCK, i);

            if(0 == p_nr_map_block->h_flags)
                continue;

            status_break(status = todo_from_needs_recalc_map_block(p_nr_map_block, i));

            al_array_dispose(&p_nr_map_block->h_flags);
            p_nr_map_block->rows = 0;
        }

        al_array_dispose(&h_nr_maps);
    }

    /* now make a todo list from the todo map, sorted on node distance */
    if(status_ok(status))
        status = todo_list_from_maps();

    if(status_ok(status))
        todo_sort_nodes();

    return(status);
}

/******************************************************************************
*
* extract a cell todo
*
******************************************************************************/

_Check_return_ _Success_(return > 0)
extern STATUS /* +ve == got one */
todo_next_slr(
    _OutRef_    P_EV_SLR p_ev_slr)
{
    status_return(todo_from_needs_recalc());

    {
    ARRAY_INDEX n_todo = array_elements(&h_todo_list);

    if(n_todo)
    {
        *p_ev_slr = array_ptr(&h_todo_list, TODO_ENTRY, n_todo - 1)->slr;
        return(1);
    }
    else
    {
        /* switches off recalc etc. */
        (void)todo_remove_slr();
        return(0);
    }
    } /*block*/
}

/*ncr*/
extern S32 /* any left ? */
todo_remove_slr(void)
{
    ARRAY_INDEX n_todo = array_elements(&h_todo_list);

    if(0 != n_todo)
    {
        al_array_shrink_by(&h_todo_list, -1);
        n_todo -= 1;
    }

    if(!n_todo)
        al_array_dispose(&h_todo_list);

    return(n_todo);
}

/* end of ev_todo.c */
