/* sk_slot.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Cell (slot) routines for Fireworkz */

/* MRJC January 1992 */

#include "common/gflags.h"

#ifndef    __cells_ob_cells_h
#include "ob_cells/ob_cells.h"
#endif

/*
internal routines
*/

#define p_list_block_from_col_in_range(p_docu, col) \
    array_ptr(&(p_docu)->h_col_list, LIST_BLOCK, (col)) \

#define p_list_block_from_col(p_docu, col) ( \
    ((col) < n_cols_physical(p_docu)) \
        ? p_list_block_from_col_in_range(p_docu, col) \
        : NULL )

_Check_return_
static BOOL
cells_object_at_end(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_POSITION p_position);

_Check_return_
static STATUS
slr_blank_make_no_uref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr);

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_slot);

_Check_return_
static STATUS
sk_slot_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    COL col;

    for(col = 0; col < n_cols_physical(p_docu); ++col)
    {
        const P_LIST_BLOCK p_list_block = p_list_block_from_col_in_range(p_docu, col);

        list_free(p_list_block);
    }

    al_array_dispose(&p_docu->h_col_list);

    /* flag document no longer has data */
    p_docu->flags.has_data = 0;

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_sk_slot_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__CLOSE1:
        return(sk_slot_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_slot)
{
    IGNOREPARM_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_slot_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* is there a splittable object at the given position ?
*
******************************************************************************/

_Check_return_
static STATUS /* FAIL = can never split, OK = at start, DONE = moved to start */
slot_object_splittable(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_POSITION p_position)
{
    OBJECT_POSITION_SET object_position_set;
    STATUS status = STATUS_FAIL;

    if(cell_data_from_position(p_docu, &object_position_set.object_data, p_position))
    {
        object_position_set.action = OBJECT_POSITION_SET_START;
        status = cell_call_id(object_position_set.object_data.object_id, p_docu, T5_MSG_OBJECT_POSITION_SET, &object_position_set, NO_CELLS_EDIT);
    }

    return(status);
}

/******************************************************************************
*
* lose trailing blank logical columns
*
******************************************************************************/

static void
cols_logical_minimise(
    _DocuRef_   P_DOCU p_docu,
    _In_        COL cols_n)
{
    COL col = p_docu->cols_logical;

    while(--col >= 0 && cols_n)
    {
        if(!cells_block_is_blank(p_docu, col, col + 1, 0, n_rows(p_docu)))
            break;

        p_docu->cols_logical = MAX(p_docu->cols_logical - 1, 1);

        page_x_extent_set(p_docu);

        cols_n -= 1;
    }
}

/******************************************************************************
*
* lose trailing blank physical columns
*
******************************************************************************/

static void
cols_phys_minimise(
    _DocuRef_   P_DOCU p_docu)
{
    COL col = n_cols_physical(p_docu);

    while(--col >= 0)
    {
        if(!cells_block_is_blank(p_docu, col, col + 1, 0, n_rows(p_docu)))
            break;
        else
        {
            const P_LIST_BLOCK p_list_block = p_list_block_from_col_in_range(p_docu, col);

            list_free(p_list_block);

            al_array_shrink_by(&p_docu->h_col_list, -1);
        }
    }
}

#if defined(UNUSED_KEEP_ALIVE)

/******************************************************************************
*
* extract next cell in a docu_area - scans down columns
*
******************************************************************************/

_Check_return_
extern S32 /* docu_area scan code */
docu_area_next_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_POSITION p_position,
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_in)
{
    *p_docu_area_out = *p_docu_area_in;

    if(p_docu_area_out->whole_row)
    {
        p_docu_area_out->tl.slr.col = 0;
        p_docu_area_out->br.slr.col = n_cols_logical(p_docu);
        p_docu_area_out->whole_row = 0;
    }

    if(p_docu_area_out->whole_col)
    {
        p_docu_area_out->tl.slr.row = 0;
        p_docu_area_out->br.slr.row = n_rows(p_docu);
        p_docu_area_out->whole_col = 0;
    }

    *p_position = p_docu_area_out->tl;

    return(DOCU_AREA_FIRST_CELL);
}

_Check_return_
extern S32 /* docu_area scan code */
docu_area_next_cell(
    _OutRef_    P_POSITION p_position,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    p_position->slr.row += 1;
    p_position->object_position.object_id = OBJECT_ID_NONE;

    if(p_position->slr.row >= p_docu_area->br.slr.row)
    {
        p_position->slr.row = p_docu_area->tl.slr.row;
        p_position->slr.col += 1;

        if(p_position->slr.col >= p_docu_area->br.slr.col)
            return(DOCU_AREA_END);
    }

    if(!slr_last_in_docu_area(p_docu_area, &p_position->slr))
        return(DOCU_AREA_MIDDLE_CELL);

    p_position->object_position = p_docu_area->br.object_position;

    return(DOCU_AREA_LAST_CELL);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* what is the size of the contents of this cell ?
*
******************************************************************************/

_Check_return_
static S32
object_size_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    const P_LIST_BLOCK p_list_block = p_list_block_from_col(p_docu, p_slr->col);

    if(NULL != p_list_block)
    {
        const S32 size = list_entsize(p_list_block, (LIST_ITEMNO) p_slr->row); /* NB may 'fail' */
        if(size >= (S32) CELL_OVH)
            return(size - (S32) CELL_OVH);
    }

    return(0);
}

/******************************************************************************
*
* return a pointer to a cell
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_CELL
p_cell_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    P_LIST_BLOCK p_list_block;

    assert(p_slr->col >= 0);
    assert(p_slr->row >= 0);

    if(NULL == (p_list_block = p_list_block_from_col(p_docu, p_slr->col)))
        return(NULL);

    return(list_gotoitemcontents_opt(CELL, p_list_block, (LIST_ITEMNO) p_slr->row));
}

/******************************************************************************
*
* initialise scan block for scan of whole document, going across rows
*
******************************************************************************/

_Check_return_
extern STATUS
cells_scan_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SCAN_BLOCK p_scan_block,
    _InVal_     enum SCAN_INIT_DIRECTION scan_direction,
    _InVal_     enum SCAN_INIT_WHAT scan_what,
    P_DOCU_AREA p_docu_area,
    _InVal_     OBJECT_ID object_id)
{
    STATUS status = STATUS_DONE;

    p_scan_block->state.scan_what = UBF_PACK(scan_what);

    switch(scan_what)
    {
    default: default_unhandled();
#if CHECKING
    case SCAN_WHOLE:
#endif
        position_init_from_col_row(&p_scan_block->docu_area.tl, 0, 0);
        position_init_from_col_row(&p_scan_block->docu_area.br,
                                   scan_direction == SCAN_MATRIX ? n_cols_physical(p_docu) : n_cols_logical(p_docu),
                                   n_rows(p_docu));
        p_scan_block->h_markers = 0;
        if(p_scan_block->docu_area.tl.slr.col >= p_scan_block->docu_area.br.slr.col)
            status = STATUS_OK; /* SKS 26may95 stops completely empty documents going gaga */
        break;

    case SCAN_MARKERS:
        /* mutiple marker scan code disabled at the moment..
         * just take a copy of the current markers
         */
         p_docu_area = p_docu_area_from_markers_first(p_docu);
         p_scan_block->state.scan_what = SCAN_AREA;

        /*FALLTHRU*/

    case SCAN_AREA:
        docu_area_normalise_phys(p_docu, &p_scan_block->docu_area, p_docu_area);
        p_scan_block->h_markers = 0;
        break;

    case SCAN_FROM_CUR:
        docu_area_init(&p_scan_block->docu_area);
        p_scan_block->docu_area.tl = p_docu->cur;
        position_init_from_col_row(&p_scan_block->docu_area.br,
                                   scan_direction == SCAN_MATRIX ? n_cols_physical(p_docu) : n_cols_logical(p_docu),
                                   n_rows(p_docu));
        p_scan_block->h_markers = 0;
        break;
    }

    p_scan_block->ix_block = 0;
    p_scan_block->state.new_block = 1;
    p_scan_block->state.scan_direction = UBF_PACK(scan_direction);
    p_scan_block->object_id = object_id;

    return(status);
}

/******************************************************************************
*
* scan the next cell from a scan block
*
******************************************************************************/

_Check_return_
extern STATUS /* STATUS_OK = end, STATUS_DONE = have result */
cells_scan_next(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_DATA p_object_data,
    P_SCAN_BLOCK p_scan_block)
{
    STATUS status = STATUS_OK;

    while(!status_done(status))
    {
        /* initialise a block */
        if(p_scan_block->state.new_block)
        {
            if(p_scan_block->h_markers)
            {
                P_MARKERS p_markers;

                if(p_scan_block->ix_block >= array_elements(&p_scan_block->h_markers))
                {
                    /* end of markers in list */
                    al_array_dispose(&p_scan_block->h_markers);
                    break;
                }

                /* start new marker block */
                p_markers = array_ptr(&p_scan_block->h_markers, MARKERS, p_scan_block->ix_block);
                docu_area_normalise_phys(p_docu, &p_scan_block->docu_area, &p_markers->docu_area);
            }
            else if(p_scan_block->ix_block)
                break;

            /* initialise first cell */
            p_scan_block->slr = p_scan_block->docu_area.tl.slr;

            /* check first cell is OK */
            if(!slr_in_docu_area(&p_scan_block->docu_area, &p_scan_block->slr))
                break;

            p_scan_block->ix_block += 1;
            p_scan_block->state.new_block = 0;
        }
        else
        {
            switch(p_scan_block->state.scan_direction)
            {
            case SCAN_ACROSS:
                p_scan_block->slr.col += 1;

                if(p_scan_block->slr.col >= p_scan_block->docu_area.br.slr.col)
                {
                    if(p_scan_block->state.scan_what == SCAN_FROM_CUR) /* SKS 05feb93 fix for next match */
                        p_scan_block->slr.col = 0;
                    else
                        p_scan_block->slr.col = p_scan_block->docu_area.tl.slr.col;
                    p_scan_block->slr.row += 1;
                    if(p_scan_block->slr.row >= p_scan_block->docu_area.br.slr.row)
                        p_scan_block->state.new_block = 1;
                }
                break;

            case SCAN_DOWN:
                p_scan_block->slr.row += 1;

                if(p_scan_block->slr.row >= p_scan_block->docu_area.br.slr.row)
                {
                    p_scan_block->slr.row = p_scan_block->docu_area.tl.slr.row;
                    p_scan_block->slr.col += 1;
                    if(p_scan_block->slr.col >= p_scan_block->docu_area.br.slr.col)
                        p_scan_block->state.new_block = 1;
                }
                break;

            case SCAN_MATRIX:
                if(p_scan_block->slr.col < n_cols_physical(p_docu))
                {
                    if(!list_nextseq(array_ptr(&p_docu->h_col_list, LIST_BLOCK, p_scan_block->slr.col),
                                               (P_LIST_ITEMNO) &p_scan_block->slr.row) ||
                       p_scan_block->slr.row >= p_scan_block->docu_area.br.slr.row)
                    {
                        p_scan_block->slr.row = p_scan_block->docu_area.tl.slr.row;
                        p_scan_block->slr.col += 1;
                        if(p_scan_block->slr.col >= p_scan_block->docu_area.br.slr.col)
                            p_scan_block->state.new_block = 1;
                    }
                }
                else
                    p_scan_block->state.new_block = 1;
                break;

            default: default_unhandled(); break;
            } /* switch */
        }

        if(!p_scan_block->state.new_block)
        {
            status_consume(object_data_from_slr(p_docu, p_object_data, &p_scan_block->slr));

            /* have we got the object we're looking for ? */
            if((OBJECT_ID_NONE == p_scan_block->object_id) || (p_scan_block->object_id == p_object_data->object_id))
                status = STATUS_DONE;

            /* at the start of the block ? */
            if(slr_equal(&p_scan_block->slr, &p_scan_block->docu_area.tl.slr))
                p_object_data->object_position_start = p_scan_block->docu_area.tl.object_position;

            /* have we hit the end of the block ? */
            if(p_scan_block->slr.col == p_scan_block->docu_area.br.slr.col - 1 &&
               p_scan_block->slr.row == p_scan_block->docu_area.br.slr.row - 1)
                p_object_data->object_position_end = p_scan_block->docu_area.br.object_position;
        }
    }

    return(status);
}

/******************************************************************************
*
* calculate percentage through block
*
******************************************************************************/

_Check_return_
extern S32
cells_scan_percent(
    P_SCAN_BLOCK p_scan_block)
{
    COL col = p_scan_block->slr.col - p_scan_block->docu_area.tl.slr.col;
    ROW row = p_scan_block->slr.row - p_scan_block->docu_area.tl.slr.row;
    COL cols_n = p_scan_block->docu_area.br.slr.col - p_scan_block->docu_area.tl.slr.col;
    ROW rows_n = p_scan_block->docu_area.br.slr.row - p_scan_block->docu_area.tl.slr.row;
    S32 percent;

    if(p_scan_block->state.new_block)
        return(0);

    switch(p_scan_block->state.scan_direction)
    {
    default: default_unhandled();
#if CHECKING
    case SCAN_ACROSS:
#endif
        percent = (row * 100) / rows_n;

        if(rows_n < 100)
        {
            /* column number varying can make some useful (>=1%) contribution */
            S32 contribution = (col * 100) / (cols_n * rows_n);
            percent += contribution;
        }

        break;

    case SCAN_DOWN:
    case SCAN_MATRIX:
        percent = (col * 100) / cols_n;

        if(cols_n < 100)
        {
            /* row number varying can make some useful (>=1%) contribution */
            S32 contribution = (row * 100) / (cols_n * rows_n);
            percent += contribution;
        }

        break;
    }

    return(percent);
}

/******************************************************************************
*
* blank out a single cell - be careful to minimise reformat
*
******************************************************************************/

_Check_return_
extern STATUS
cells_blank_make(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    STATUS status = STATUS_OK;

    { /* tell dependents about overwrite */
    UREF_PARMS uref_parms;
    region_from_two_slrs(&uref_parms.source.region, p_slr, p_slr, TRUE);
    uref_event(p_docu, T5_MSG_UREF_OVERWRITE, &uref_parms);
    } /*block*/

    status = slr_blank_make_no_uref(p_docu, p_slr);

    if(status_ok(status))
        status = format_object_size_set(p_docu, NULL, NULL, p_slr, FALSE);

    return(status);
}

/******************************************************************************
*
* is block blank ?
*
******************************************************************************/

_Check_return_
extern BOOL
cells_block_is_blank(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_s,
    _InVal_     COL col_e,
    _InVal_     ROW row_s,
    _InVal_     S32 rows_n)
{
    const COL col_e_limited = MIN(col_e, n_cols_physical(p_docu));
    const ROW row_e_limited = MIN((row_s + rows_n), n_rows(p_docu));
    SLR slr;

    for(slr.col = col_s; slr.col < col_e_limited; ++slr.col)
    {
        for(slr.row = row_s; slr.row < row_e_limited; ++slr.row)
        {
            if(!slr_is_blank(p_docu, &slr))
                return(FALSE);
        }
    }

    return(TRUE);
}

/******************************************************************************
*
* blank out a block
*
******************************************************************************/

_Check_return_
extern STATUS
cells_block_blank_make(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_s,
    _InVal_     COL col_e,
    _InVal_     ROW row_s,
    _InVal_     S32 rows_n)
{
    STATUS status = STATUS_OK;

    { /* tell dependents about overwrite */
    UREF_PARMS uref_parms;
    uref_parms.source.region.tl.col = col_s;
    uref_parms.source.region.tl.row = row_s;
    uref_parms.source.region.br.col = col_e;
    uref_parms.source.region.br.row = row_s + (ROW) rows_n;
    uref_parms.source.region.whole_col =
    uref_parms.source.region.whole_row = 0;
    uref_event(p_docu, T5_MSG_UREF_OVERWRITE, &uref_parms);
    } /*block*/

    {
    const COL col_e_limited = MIN(col_e, n_cols_physical(p_docu));
    const ROW row_e_limited = MIN((row_s + (ROW) rows_n), n_rows(p_docu));
    SLR slr;

    for(slr.col = col_s; slr.col < col_e_limited; ++slr.col)
    {
        for(slr.row = row_s; slr.row < row_e_limited; ++slr.row)
        {
            status_break(status = slr_blank_make_no_uref(p_docu, &slr));
        }

        status_break(status);
    }
    } /*block*/

    reformat_from_row(p_docu, row_s, REFORMAT_Y);

    return(status);
}

/******************************************************************************
*
* copy a block of cells to a new place
*
******************************************************************************/

_Check_return_
extern STATUS
cells_block_copy_no_uref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr_to,
    _InRef_     PC_SLR p_slr_from_s,
    _InRef_     PC_SLR p_slr_from_e)
{
    STATUS status = STATUS_OK;

    {
    OBJECT_COPY object_copy;

    for(object_copy.slr_from.col = p_slr_from_s->col;
        object_copy.slr_from.col < p_slr_from_e->col;
        object_copy.slr_from.col += 1)
    {
        for(object_copy.slr_from.row = p_slr_from_s->row;
            object_copy.slr_from.row < p_slr_from_e->row;
            object_copy.slr_from.row += 1)
        {
            object_copy.slr_to.col = object_copy.slr_from.col - p_slr_from_s->col + p_slr_to->col;
            object_copy.slr_to.row = object_copy.slr_from.row - p_slr_from_s->row + p_slr_to->row;

            if(status_fail(status))
                status_consume(slr_blank_make_no_uref(p_docu, &object_copy.slr_to));
            else
            {
                if(status_ok(status = slr_blank_make_no_uref(p_docu, &object_copy.slr_to)))
                    status = object_call_from_slr(p_docu, T5_MSG_OBJECT_COPY, &object_copy, &object_copy.slr_from);
            }
        }
    }
    } /*block*/

    return(status);
}

/******************************************************************************
*
* delete a block of cells
*
******************************************************************************/

extern void
cells_block_delete(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_s,
    _InVal_     COL col_e,
    _InVal_     ROW at_row,
    _InVal_     S32 rows_n)
{
    { /* tell dependents about delete */
    UREF_PARMS uref_parms;
    uref_parms.source.region.tl.col = col_s;
    uref_parms.source.region.tl.row = at_row;
    uref_parms.source.region.br.col = col_e;
    uref_parms.source.region.br.row = at_row + (ROW) rows_n;
    uref_parms.source.region.whole_col =
    uref_parms.source.region.whole_row = 0;
    if(col_s == 0 && col_e >= n_cols_logical(p_docu))
        uref_parms.source.region.whole_row = 1;
    uref_event(p_docu, T5_MSG_UREF_DELETE, &uref_parms);
    } /*block*/

    {
    const COL col_e_limited = MIN(col_e, n_cols_physical(p_docu));
    SLR slr;
    slr.row = at_row;
    for(slr.col = col_s; slr.col < col_e_limited; ++slr.col)
    {
        const P_LIST_BLOCK p_list_block = p_list_block_from_col_in_range(p_docu, slr.col);
        ROW row;

        /* done in single steps to avoid having to
         * do a list_ensureitem, which may fail
         * and make things very messy
        */
        for(row = at_row; row < at_row + rows_n; ++row)
            list_deleteitems(p_list_block, (LIST_ITEMNO) slr.row, 1);
    }
    } /*block*/

    { /* update dependencies for the block shifted upwards as a result of the delete */
    UREF_PARMS uref_parms;
    uref_parms.source.region.tl.col = col_s;
    uref_parms.source.region.tl.row = at_row + (ROW) rows_n;
    uref_parms.source.region.br.col = col_e;
    uref_parms.source.region.br.row = MAX_ROW;
    uref_parms.source.region.whole_col =
    uref_parms.source.region.whole_row = 0;
    if(col_s == 0 && col_e >= n_cols_logical(p_docu))
        uref_parms.source.region.whole_row = 1;

    uref_parms.target.slr.col = 0;
    uref_parms.target.slr.row = - (ROW) rows_n;
    uref_parms.add_col = 0;
    uref_parms.add_row = 0;

    uref_event(p_docu, T5_MSG_UREF_UREF, &uref_parms);
    } /*block*/

    /* lop off some rows */
    if(!col_s && col_e >= n_cols_logical(p_docu))
    {
        ROW rows_n_new = n_rows(p_docu) - (ROW) rows_n;
        status_assert(n_rows_set(p_docu, MAX(1, rows_n_new)));
    }

    reformat_from_row(p_docu, at_row, REFORMAT_Y);
}

/******************************************************************************
*
* insert a block of cells
*
******************************************************************************/

_Check_return_
extern STATUS
cells_block_insert(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_s,
    _InVal_     COL col_e,
    _InVal_     ROW at_row,
    _InVal_     S32 rows_n,
    _InVal_     S32 add)
{
    STATUS status = STATUS_OK;
    SLR slr;
    ROW row_add_shift = add ? 1 : 0;

    for(slr.col = col_s, slr.row = at_row;
        (slr.col < col_e) && (slr.col < n_cols_physical(p_docu));
        ++slr.col)
    {
        const P_LIST_BLOCK p_list_block = p_list_block_from_col_in_range(p_docu, slr.col);

        status_break(status = list_insertitems(p_list_block, (LIST_ITEMNO) (slr.row + row_add_shift), (LIST_ITEMNO) rows_n));
    }

    /* update number of rows in document */
    if(status_ok(status))
    {
        ROW row_max = 0;

        if(col_s > 0 || col_e < n_cols_logical(p_docu))
        {
            /* work out bottom of block being inserted into */
            COL col;

            for(col = col_s; col < col_e; col += 1)
            {
                const P_LIST_BLOCK p_list_block = p_list_block_from_col(p_docu, col);

                if(NULL != p_list_block)
                    row_max = MAX(row_max, list_numitem(p_list_block));
            }
        }
        else
            row_max = n_rows(p_docu);

        if(row_max + rows_n > n_rows(p_docu))
            status = n_rows_set(p_docu, row_max + (ROW) rows_n);
    }

    /* clear up after a mess */
    if(status_fail(status))
    {
        COL upto;

        for(upto = slr.col, slr.col = col_s; slr.col < upto; ++slr.col)
        {
            const P_LIST_BLOCK p_list_block = p_list_block_from_col(p_docu, slr.col);

            if(NULL != p_list_block)
                list_deleteitems(p_list_block, (LIST_ITEMNO) (slr.row + row_add_shift), (LIST_ITEMNO) rows_n);
        }
    }
    else
    {
        {
        UREF_PARMS uref_parms;
        uref_parms.source.region.tl.col = col_s;
        uref_parms.source.region.tl.row = at_row + row_add_shift;
        uref_parms.source.region.br.col = col_e;
        uref_parms.source.region.br.row = MAX_ROW;
        uref_parms.source.region.whole_col =
        uref_parms.source.region.whole_row = 0;
        if(col_s == 0 && col_e >= n_cols_logical(p_docu))
            uref_parms.source.region.whole_row = 1;
        uref_parms.target.slr.col = 0;
        uref_parms.target.slr.row = (ROW) rows_n;
        uref_parms.add_col = 0;
        uref_parms.add_row = UBF_PACK(add);

        uref_event(p_docu, T5_MSG_UREF_UREF, &uref_parms);
        } /*block*/

        reformat_from_row(p_docu, at_row, REFORMAT_Y);
    }

    return(status);
}

/******************************************************************************
*
* move object contents from one place to another
*
******************************************************************************/

_Check_return_
static STATUS
slot_object_move_no_uref(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_COPY p_object_copy)
{
    STATUS status = STATUS_OK;
    S32 size = object_size_from_slr(p_docu, &p_object_copy->slr_from);

    if(0 != size)
    {
        P_CELL p_cell_from = p_cell_from_slr(p_docu, &p_object_copy->slr_from);
        P_CELL p_cell_to;
        PTR_ASSERT(p_cell_from);

        if(status_ok(status = slr_realloc(p_docu, &p_cell_to, &p_object_copy->slr_to, object_id_from_cell(p_cell_from), size)))
        {
            p_cell_from = p_cell_from_slr(p_docu, &p_object_copy->slr_from); /* reload */
            PTR_ASSERT(p_cell_from);
            memcpy32(p_cell_to, p_cell_from, size + CELL_OVH);

            status = slr_blank_make_no_uref(p_docu, &p_object_copy->slr_from);
        }
    }
    else
    {
        status = slr_blank_make_no_uref(p_docu, &p_object_copy->slr_to);
    }

    return(status);
}

/******************************************************************************
*
* delete columns from a block
*
******************************************************************************/

_Check_return_
extern STATUS
cells_column_delete(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL at_col,
    _InVal_     COL cols_n,
    _InVal_     ROW row_s,
    _InVal_     ROW row_e)
{
    STATUS status = STATUS_OK;

    {
    UREF_PARMS uref_parms;
    uref_parms.source.region.tl.col = at_col;
    uref_parms.source.region.tl.row = row_s;
    uref_parms.source.region.br.col = at_col + cols_n;
    uref_parms.source.region.br.row = row_e;
    uref_parms.source.region.whole_col =
    uref_parms.source.region.whole_row = 0;
    if(row_s == 0 && row_e >= n_rows(p_docu))
        uref_parms.source.region.whole_col = 1;

    uref_event(p_docu, T5_MSG_UREF_DELETE, &uref_parms);
    } /*block*/

    {
    OBJECT_COPY object_copy;

    for(object_copy.slr_to.col = at_col; object_copy.slr_to.col < n_cols_physical(p_docu); ++object_copy.slr_to.col)
    {
        for(object_copy.slr_to.row = row_s; object_copy.slr_to.row < row_e; ++object_copy.slr_to.row)
        {
            object_copy.slr_from.col = object_copy.slr_to.col + cols_n;
            object_copy.slr_from.row = object_copy.slr_to.row;

            status_break(status = slot_object_move_no_uref(p_docu, &object_copy));
        }

        status_break(status);
    }
    } /*block*/

    cols_logical_minimise(p_docu, cols_n);
    cols_phys_minimise(p_docu);

    {
    UREF_PARMS uref_parms;
    uref_parms.source.region.tl.col = at_col + cols_n;
    uref_parms.source.region.br.col = MAX_COL;
    uref_parms.source.region.tl.row = row_s;
    uref_parms.source.region.br.row = row_e;
    uref_parms.source.region.whole_col =
    uref_parms.source.region.whole_row = 0;
    if(row_s == 0 && row_e >= n_rows(p_docu))
        uref_parms.source.region.whole_col = 1;

    uref_parms.target.slr.col = -cols_n;
    uref_parms.target.slr.row = 0;
    uref_parms.add_col = 0;
    uref_parms.add_row = 0;

    uref_event(p_docu, T5_MSG_UREF_UREF, &uref_parms);
    } /*block*/

    reformat_from_row(p_docu, row_s, REFORMAT_XY);

    return(status);
}

/******************************************************************************
*
* insert columns in a block
*
******************************************************************************/

_Check_return_
extern STATUS
cells_column_insert(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL at_col,
    _InVal_     COL cols_n,
    _InVal_     ROW row_s,
    _InVal_     ROW row_e,
    _InVal_     S32 add)
{
    STATUS status = STATUS_OK;
    COL col_add_shift = add ? 1 : 0;

    {
    OBJECT_COPY object_copy;
    COL old_logical = p_docu->cols_logical;

    for(object_copy.slr_to.col = n_cols_physical(p_docu) - 1 + cols_n;
        object_copy.slr_to.col >= (at_col + cols_n + col_add_shift);
        --object_copy.slr_to.col)
    {
        for(object_copy.slr_to.row = row_s; object_copy.slr_to.row < row_e; ++object_copy.slr_to.row)
        {
            object_copy.slr_from.col = object_copy.slr_to.col - cols_n;
            object_copy.slr_from.row = object_copy.slr_to.row;

            status_break(status = slot_object_move_no_uref(p_docu, &object_copy));
        }

        status_break(status);
    }

    /* if moving data didn't make enough columns, make 'virtual' ones anyway */
    p_docu->cols_logical += cols_n - (p_docu->cols_logical - old_logical);

    page_x_extent_set(p_docu);
    } /*block*/

    {
    UREF_PARMS uref_parms;
    uref_parms.source.region.tl.col = at_col + col_add_shift;
    uref_parms.source.region.br.col = MAX_COL;
    uref_parms.source.region.tl.row = row_s;
    uref_parms.source.region.br.row = row_e;
    uref_parms.source.region.whole_col =
    uref_parms.source.region.whole_row = 0;
    if(row_s == 0 && row_e >= n_rows(p_docu))
        uref_parms.source.region.whole_col = 1;

    uref_parms.target.slr.col = cols_n;
    uref_parms.target.slr.row = 0;
    uref_parms.add_col = UBF_PACK(add);
    uref_parms.add_row = 0;

    uref_event(p_docu, T5_MSG_UREF_UREF, &uref_parms);
    } /*block*/

    reformat_from_row(p_docu, row_s, REFORMAT_XY);

    return(status);
}

/******************************************************************************
*
* delete an area from a document
*
******************************************************************************/

_Check_return_
extern STATUS
cells_docu_area_delete(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InVal_     BOOL delete_whole_rows,
    _InVal_     BOOL before_and_after)
{
    STATUS status = STATUS_OK;
    COL col_s, col_e;
    ROW row_s, row_e;
    BOOL start_frag, end_frag, single_cell_frag, end_frag_deleted = 0;
    OBJECT_DELETE_SUB object_delete_sub;

    limits_from_docu_area(p_docu, &col_s, &col_e, &row_s, &row_e, p_docu_area);

    /* fragment in a single cell ? */
    start_frag = docu_area_frag_start(p_docu_area);
    end_frag = docu_area_frag_end(p_docu_area);

    if(slr_last_in_docu_area(p_docu_area, &p_docu_area->tl.slr) && start_frag && end_frag)
        single_cell_frag = 1;
    else
        single_cell_frag = 0;

    /* delete start fragment */
    if(start_frag)
    {
        (void) cell_data_from_docu_area_tl(p_docu, &object_delete_sub.object_data, p_docu_area);
        object_delete_sub.save_data = 0;
        status_consume(cell_call_id(object_delete_sub.object_data.object_id, p_docu, T5_MSG_OBJECT_DELETE_SUB, &object_delete_sub, NO_CELLS_EDIT));

        if(single_cell_frag)
            end_frag_deleted = 1;

        row_s += 1;
    }

    /* delete end fragment */
    if(!end_frag_deleted && end_frag)
    {
        (void) cell_data_from_docu_area_br(p_docu, &object_delete_sub.object_data, p_docu_area);
        object_delete_sub.save_data = 0;
        status_consume(cell_call_id(object_delete_sub.object_data.object_id, p_docu, T5_MSG_OBJECT_DELETE_SUB, &object_delete_sub, NO_CELLS_EDIT));

        if(!start_frag)
            row_e -= 1;
    }

    /* if it's not a single fragment in a single cell, clean up fragment left
     * in cell at end of block and stick it in the cell above the block
     */
    if(!single_cell_frag && start_frag && end_frag)
    {
        SLR slr_end;

        /* work out reference of end cell */
        slr_end.col = col_e - 1;
        slr_end.row = row_e - 1;

        if(cell_data_from_slr(p_docu, &object_delete_sub.object_data, &slr_end))
        {
            POSITION position;
            OBJECT_POSITION_SET object_position_set;

            /* this is a special position of ID,0 so that we are saving a
             * fragment, not just a whole cell, so that it is treated
             * as a fragment
             */
            object_position_set_start(&object_delete_sub.object_data.object_position_start, object_delete_sub.object_data.object_id);
            object_delete_sub.save_data = 1;
            status_consume(cell_call_id(object_delete_sub.object_data.object_id, p_docu, T5_MSG_OBJECT_DELETE_SUB, &object_delete_sub, NO_CELLS_EDIT));

            position = p_docu_area->tl;
            if(cell_data_from_position(p_docu, &object_position_set.object_data, &p_docu_area->tl))
            {
                assert(object_position_set.object_data.object_id == object_delete_sub.object_data.object_id);
                object_position_set.action = OBJECT_POSITION_SET_END;
                status_consume(cell_call_id(object_position_set.object_data.object_id,
                                            p_docu,
                                            T5_MSG_OBJECT_POSITION_SET,
                                            &object_position_set,
                                            NO_CELLS_EDIT));
                position.object_position = object_position_set.object_data.object_position_start;
            }
            else
                position.object_position.object_id = OBJECT_ID_NONE;

            status = load_ownform_from_array_handle(p_docu, &object_delete_sub.h_data_del, &position, FALSE);

            /* free deleted data */
            al_array_dispose(&object_delete_sub.h_data_del);

            {
            DOCU_REFORMAT docu_reformat;
            docu_reformat.action = REFORMAT_Y;
            docu_reformat.data_type = REFORMAT_DOCU_AREA;
            docu_reformat.data_space = DATA_SLOT;
            docu_reformat.data.docu_area = *p_docu_area;
            status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
            } /*block*/
        }
    }

    if(status_ok(status))
    {
        S32 rows_n = row_e - row_s;

        if(rows_n)
        {
            if(before_and_after)
                cur_change_before(p_docu);

            if(!delete_whole_rows)
            {
                if((col_s == 0) && (col_e >= n_cols_max(p_docu, row_s, row_e)))
                {
                    cells_block_delete(p_docu, 0, all_cols(p_docu), row_s, rows_n);
                }
                else
                {
                    status = cells_block_blank_make(p_docu, col_s, col_e, row_s, rows_n);
                }
            }
            else
            {
                /* try to delete across document to maintain alignment */
                if( cells_block_is_blank(p_docu,     0,            col_s, row_s, rows_n) &&
                    cells_block_is_blank(p_docu, col_e, all_cols(p_docu), row_s, rows_n) )
                {
                    cells_block_delete(p_docu, 0, all_cols(p_docu), row_s, rows_n);
                }
                else
                {
                    cells_block_delete(p_docu, col_s, col_e, row_s, rows_n);

                    status = cells_block_insert(p_docu, col_s, col_e, row_s, rows_n, FALSE);
                }
            }

            if(before_and_after)
                cur_change_after(p_docu);
        }
    }

    return(status);
}

/******************************************************************************
*
* insert an area into a document
*
* final output position gives insert position adjusted for fragments etc. (optional)
*
******************************************************************************/

_Check_return_ _Success_(return >= 0)
extern STATUS
cells_docu_area_insert(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_POSITION p_position,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _OutRef_    P_POSITION p_position_out)
{
    DOCU_AREA docu_area;
    STATUS status = STATUS_OK, splittable;
    S32 cols_n, rows_n;

    /* calculate number of rows to insert */
    docu_area_normalise_phys(p_docu, &docu_area, p_docu_area);
    cols_n = docu_area.br.slr.col - docu_area.tl.slr.col;
    rows_n = docu_area.br.slr.row - docu_area.tl.slr.row;

    if(cols_n && rows_n)
    {
        ROW at_row = p_position->slr.row;
        S32 start_frag = (OBJECT_ID_NONE != docu_area.tl.object_position.object_id) && (0 != docu_area.tl.object_position.data);
        S32 end_frag   = (OBJECT_ID_NONE != docu_area.br.object_position.object_id);
        BOOL single_cell_frag = 0, middle_insert = 0, end_insert = 0;

        if( slr_last_in_docu_area(p_docu_area, &docu_area.tl.slr) &&
            ((OBJECT_ID_NONE != docu_area.tl.object_position.object_id) || (OBJECT_ID_NONE != docu_area.br.object_position.object_id)) )
        {
            start_frag = 1;
            end_frag = 0;
            single_cell_frag = 1;
        }

        rows_n -= start_frag + end_frag;

        if(cells_object_at_end(p_docu, p_position))
            end_insert = 1;

        /* are we inserting into the middle of an object ? */
        if((OBJECT_ID_NONE != p_position->object_position.object_id) &&
           (!object_position_at_start(&p_position->object_position) &&
            !end_insert))
            middle_insert = 1;

        if(end_insert)
            at_row += 1;

        splittable = slot_object_splittable(p_docu, p_position);

        if(single_cell_frag)
            rows_n = 0;
        else if(middle_insert && status_ok(splittable))
        {
            status_return(cells_object_split(p_docu, p_position));
            at_row += 1;
        }

        if(!status_done(splittable))
            rows_n += start_frag + end_frag;
        else if(!single_cell_frag)
        {
            if(object_position_at_start(&p_position->object_position))
                rows_n += start_frag;
        }

        *p_position_out = *p_position;

        if(!single_cell_frag)
        {
            if( (middle_insert || end_insert)
                &&
                ( (!status_ok(splittable) &&  start_frag) ||
                  ( status_ok(splittable) && !start_frag) )
                )
            {
                p_position_out->slr.row += 1;
                p_position_out->object_position.object_id = OBJECT_ID_NONE;
            }
        }

        if(rows_n)
        {
            if(!cells_block_is_blank(p_docu, p_position->slr.col, p_position->slr.col + (COL) cols_n, p_position->slr.row, rows_n))
            {
                status = cells_block_insert(p_docu, 0, all_cols(p_docu), at_row, rows_n, FALSE);
            }
            else
            {

                {
                UREF_PARMS uref_parms;
                uref_parms.source.region.tl.col = p_position->slr.col;
                uref_parms.source.region.tl.row = p_position->slr.row;
                uref_parms.source.region.br.col = p_position->slr.col + (COL) cols_n;
                uref_parms.source.region.br.row = p_position->slr.row + (ROW) rows_n;
                uref_parms.source.region.whole_col =
                uref_parms.source.region.whole_row = 0;

                uref_event(p_docu, T5_MSG_UREF_OVERWRITE, &uref_parms);
                } /*block*/

                {
                ROW rows_n_now = n_rows(p_docu);
                /* ensure we have enough rows in document */
                status = n_rows_set(p_docu, MAX(rows_n_now, at_row + (ROW) rows_n));
                } /*block*/

                reformat_from_row(p_docu, p_position->slr.row, REFORMAT_Y);
            }
        }
    }
    else
    {   /* can be inserting a file into an empty document */
        *p_position_out = *p_position;
    }

    return(status);
}

/******************************************************************************
*
* given a position, check to see if it's at the end of the relevant object
*
******************************************************************************/

_Check_return_
static BOOL
cells_object_at_end(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_POSITION p_position)
{
    OBJECT_POSITION_SET object_position_set;
    BOOL res = 0;

    if( (OBJECT_ID_NONE != p_position->object_position.object_id)
        &&
        cell_data_from_position(p_docu, &object_position_set.object_data, p_position) )
    {
        object_position_set.action = OBJECT_POSITION_SET_END;
        if(status_done(cell_call_id(object_position_set.object_data.object_id,
                                    p_docu,
                                    T5_MSG_OBJECT_POSITION_SET,
                                    &object_position_set,
                                    NO_CELLS_EDIT)) &&
           !object_position_compare(&object_position_set.object_data.object_position_start,
                                    &p_position->object_position,
                                    OBJECT_POSITION_COMPARE_PP))
            res = 1;
    }

    return(res);
}

/******************************************************************************
*
* split (try to) the object at the given position
*
******************************************************************************/

_Check_return_
extern STATUS
cells_object_split(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_POSITION p_position)
{
    STATUS status = STATUS_OK;
    BOOL start_frag;

    if(object_position_at_start(&p_position->object_position))
        start_frag = FALSE;
    else
        start_frag = TRUE;

    /* insert row across document;
     * if start_frag, we insert row after,
     * if start_frag, we insert row before
     */
    status_return(cells_block_insert(p_docu,
                                     0,
                                     all_cols(p_docu),
                                     start_frag ? p_position->slr.row + 1
                                                : p_position->slr.row,
                                     1,
                                     FALSE));

    if(start_frag)
    {
        OBJECT_DELETE_SUB object_delete_sub;

        consume_bool(cell_data_from_position(p_docu, &object_delete_sub.object_data, p_position));
        assert(OBJECT_ID_NONE != object_delete_sub.object_data.object_id);
        object_delete_sub.save_data = 1;

        /* status is not returned from DELETE_SUB call - object may not be splittable; this is not an error */
        if(status_ok(cell_call_id(object_delete_sub.object_data.object_id, p_docu, T5_MSG_OBJECT_DELETE_SUB, &object_delete_sub, NO_CELLS_EDIT))
           &&
           object_delete_sub.h_data_del)
        {
            POSITION position;

            position = *p_position;
            position.slr.row += 1;
            position.object_position.object_id = OBJECT_ID_NONE;

            cur_change_before(p_docu);
            status = load_ownform_from_array_handle(p_docu, &object_delete_sub.h_data_del, &position, FALSE);
            cur_change_after(p_docu);

            /* free deleted data */
            al_array_dispose(&object_delete_sub.h_data_del);
        }
    }

    return(status);
}

/******************************************************************************
*
* make a cell blank
*
******************************************************************************/

_Check_return_
static STATUS
slr_blank_make_no_uref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    P_CELL p_cell;
    return(slr_realloc(p_docu, &p_cell, p_slr, OBJECT_ID_NONE, 0));
}

/******************************************************************************
*
* set the object size of a cell - creating the cell if necessary;
* an object size of 0 creates a hole
*
******************************************************************************/

_Check_return_
extern STATUS
slr_realloc(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_P_CELL p_p_cell,
    _InRef_     PC_SLR p_slr,
    _InVal_     OBJECT_ID object_id,
    _InVal_     S32 object_size)
{
    STATUS status = STATUS_OK;

    *p_p_cell = NULL;

    /* does the column exist ? */
    if(object_size && (p_slr->col >= n_cols_physical(p_docu)))
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(6, sizeof32(LIST_BLOCK), TRUE);
        COL start_col, icol;
        P_LIST_BLOCK p_list_block;

        start_col = n_cols_physical(p_docu);

        if(NULL == (p_list_block = al_array_extend_by(&p_docu->h_col_list, LIST_BLOCK, (p_slr->col - start_col) + 1, &array_init_block, &status)))
        {
            myassert1(TEXT("slr_realloc failed to realloc col list by ") S32_TFMT, (S32) (p_slr->col - start_col) + 1);
            return(status);
        }

        al_array_auto_compact_set(&p_docu->h_col_list);

        for(icol = start_col; icol <= p_slr->col; ++icol, ++p_list_block)
            list_init(p_list_block);

        if(p_slr->col >= p_docu->cols_logical)
        {
            p_docu->cols_logical = p_slr->col + 1;

            page_x_extent_set(p_docu);
        }

        /* flag document has data */
        p_docu->flags.has_data = 1;
    }

    /* make sure the row exists */
    if((p_slr->row + 1) > n_rows(p_docu))
        status_return(n_rows_set(p_docu, p_slr->row + 1));

    { /* create an object */
    P_LIST_ITEM p_list_item;
    const P_CELL p_cell_old = p_cell_from_slr(p_docu, p_slr);
    P_CELL p_cell = NULL;

    if(object_size)
    {
        const P_LIST_BLOCK p_list_block = p_list_block_from_col_in_range(p_docu, p_slr->col);

        if(NULL == (p_list_item = list_createitem(p_list_block, (LIST_ITEMNO) p_slr->row, CELL_OVH + object_size, FALSE)))
        {
            myassert1(TEXT("slr_realloc failed to create cell of size ") S32_TFMT, CELL_OVH + object_size);
            status = status_nomem();
        }
        else
        {
            p_cell = list_itemcontents(CELL, p_list_item);
            p_cell->packed_object_id = OBJECT_ID_PACK(PACKED_OBJECT_ID, object_id);
            if(NULL == p_cell_old)
            {
                p_cell->cell_at_row_bottom = 0;
                p_cell->cell_new = 1;
            }
        }
    }
    else if(NULL != p_cell_old)
    {
        /* create a hole */
        const P_LIST_BLOCK p_list_block = p_list_block_from_col_in_range(p_docu, p_slr->col);

        if(NULL == list_createitem(p_list_block, (LIST_ITEMNO) p_slr->row, 0, FALSE))
        {
            myassert0(TEXT("slr_realloc failed to create a hole at existing cell!"));
            status = status_nomem();
        }
    }

    *p_p_cell = p_cell;
    } /*block*/

    return(status);
}

/******************************************************************************
*
* cell swapping routines
*
******************************************************************************/

typedef struct CELL_SWAP
{
    SLR slr;
    P_CELL p_cell;
    S32 object_size;
    PACKED_OBJECT_ID packed_object_id;
    U8 _padding[3];
}
CELL_SWAP, * P_CELL_SWAP;

_Check_return_
static STATUS
cell_swap(
    _DocuRef_   P_DOCU p_docu,
    P_CELL_SWAP p_cell_swap_1,
    P_CELL_SWAP p_cell_swap_2)
{
    STATUS status = STATUS_OK;
    P_BYTE p_byte;
    QUICK_BLOCK_WITH_BUFFER(quick_block, 500);
    quick_block_with_buffer_setup(quick_block);

    if(NULL != (p_byte = quick_block_extend_by(&quick_block, p_cell_swap_2->object_size, &status)))
    {
        if(status_ok(status))
        {
            memcpy32(p_byte, &p_cell_swap_2->p_cell->object[0], p_cell_swap_2->object_size);
            status = slr_realloc(p_docu,
                                 &p_cell_swap_2->p_cell,
                                 &p_cell_swap_2->slr,
                                 OBJECT_ID_UNPACK(p_cell_swap_1->packed_object_id),
                                 p_cell_swap_1->object_size);
        }

        if(status_ok(status))
        {
            p_cell_swap_1->p_cell = p_cell_from_slr(p_docu, &p_cell_swap_1->slr);
            assert(p_cell_swap_1->p_cell);
            memcpy32(&p_cell_swap_2->p_cell->object[0], &p_cell_swap_1->p_cell->object[0], p_cell_swap_1->object_size);
            status = slr_realloc(p_docu,
                                 &p_cell_swap_1->p_cell,
                                 &p_cell_swap_1->slr,
                                 OBJECT_ID_UNPACK(p_cell_swap_2->packed_object_id),
                                 p_cell_swap_2->object_size);
        }

        if(status_ok(status))
            memcpy32(&p_cell_swap_1->p_cell->object[0], p_byte, p_cell_swap_2->object_size);
    }

    quick_block_dispose(&quick_block);

    return(status);
}

_Check_return_
extern STATUS
cells_swap_rows(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REGION p_region)
{
    STATUS status = STATUS_OK;
    COL col, col_s, col_e;
    CELL_SWAP cell_swap_1, cell_swap_2;
    QUICK_BLOCK_WITH_BUFFER(quick_block, 500);
    quick_block_with_buffer_setup(quick_block);

    limits_from_region(p_docu, &col_s, &col_e, &cell_swap_1.slr.row, &cell_swap_2.slr.row, p_region);

    for(col = col_s; col < col_e; col += 1)
    {
        cell_swap_1.slr.col = cell_swap_2.slr.col = col;
        if(NULL != (cell_swap_1.p_cell = p_cell_from_slr(p_docu, &cell_swap_1.slr)))
        {
            cell_swap_1.object_size = object_size_from_slr(p_docu, &cell_swap_1.slr);
            cell_swap_1.packed_object_id = cell_swap_1.p_cell->packed_object_id;
        }
        else
            cell_swap_1.object_size = 0;

        if(NULL != (cell_swap_2.p_cell = p_cell_from_slr(p_docu, &cell_swap_2.slr)))
        {
            cell_swap_2.object_size = object_size_from_slr(p_docu, &cell_swap_2.slr);
            cell_swap_2.packed_object_id = cell_swap_2.p_cell->packed_object_id;
        }
        else
            cell_swap_2.object_size = 0;

        if(cell_swap_1.object_size > cell_swap_2.object_size)
            status = cell_swap(p_docu, &cell_swap_1, &cell_swap_2);
        else if(cell_swap_1.object_size < cell_swap_2.object_size)
            status = cell_swap(p_docu, &cell_swap_2, &cell_swap_1);
        else if(cell_swap_1.object_size != 0)
        {
            P_BYTE p_byte = quick_block_extend_by(&quick_block, cell_swap_1.object_size, &status);

            if(NULL != p_byte)
            {
                memcpy32(p_byte, &cell_swap_2.p_cell->object[0],                         cell_swap_1.object_size);
                memcpy32(&cell_swap_2.p_cell->object[0], &cell_swap_1.p_cell->object[0], cell_swap_1.object_size);
                memcpy32(&cell_swap_1.p_cell->object[0], p_byte,                         cell_swap_1.object_size);
                cell_swap_1.p_cell->packed_object_id = cell_swap_2.packed_object_id;
                cell_swap_2.p_cell->packed_object_id = cell_swap_1.packed_object_id;
            }

            quick_block_dispose(&quick_block);
        }

        status_break(status);
    }

    {
    UREF_PARMS uref_parms;
    uref_parms.source.region = *p_region;
    uref_event(p_docu, T5_MSG_UREF_SWAP_ROWS, &uref_parms);
    } /*block*/

    return(status);
}

/* end of sk_slot.c */
