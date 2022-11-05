/* sk_uref.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Routines for Fireworkz to handle reference dependencies */

/* MRJC March 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/******************************************************************************

for copying into things:

case Uref_Msg_Uref:
case Uref_Msg_Delete:
case Uref_Msg_Overwrite:
case Uref_Msg_Change:
case Uref_Msg_Swap_Rows:
case Uref_Msg_CLOSE1:
case Uref_Msg_CLOSE2:

******************************************************************************/

/******************************************************************************
*
* NOTE:
*
* the uref helper routines in here are configured for
* updating dependent references (i.e. refto, not refby) - thus
* a Uref_Msg_Overwrite will never generate a Uref_Dep_Delete even
* if the entire range / cell is spanned, since the dependent is
* not deleted; Old ev_uref muddled the by/to cases for deleting
* references contained by a REPLACEd area; an extra flag or
* routine will be needed for this case
*
******************************************************************************/

/******************************************************************************
*
* UREF messages:
*
*    Uref_Msg_Uref
*       issued when part of a document is moved: all refs
*       pointing to the moved part of the document are updated
*       by the move distance; issued after physical operation
*
*    Uref_Msg_Delete
*       issued when part of a document is deleted: all refs
*       pointing to the deleted part of the document are
*       marked bad; all refs from the deleted part are removed
*       from the tree; issued before physical operation
*
*    Uref_Msg_Overwrite
*       issued when part of a document is deleted: all refs from
*       the deleted part are removed from the tree; same as DELETE
*       but refs to the block are not marked bad; issued before
*       physical operation
*
*    Uref_Msg_Change
*       issued when part of a document has changed; typically follows
*       a REPLACE, but is issued after the physical operation is complete;
*       dependents use the CHANGE message to read the up-to-date data
*       e.g. after a cell has been recalced
*
*    Uref_Msg_Swap_Rows
*       issued when two rows are swapped in a sort; refs pointing
*       two the two rows are updated for the swap
*
*    T5_MSG_CLOSE1
*       initial closedown message, followed by CLOSE2, when document
*       is being destroyed; all refs from the document are removed
*
*    T5_MSG_CLOSE2
*       closedown message following CLOSE1
*
******************************************************************************/

/*
uref table entry
*/

typedef struct UREF_ENTRY
{
    UREF_ID uref_id;
    P_PROC_UREF_EVENT p_proc_uref_event;
    U8 is_deleted;
    U8 _spare[3];
}
UREF_ENTRY, * P_UREF_ENTRY;

#define P_UREF_ENTRY_NONE _P_DATA_NONE(P_UREF_ENTRY)

static UREF_HANDLE next_uref_handle = (UREF_HANDLE) 1; /* our next handle */

/******************************************************************************
*
* uref the start and end points of a region
*
******************************************************************************/

enum START_END_FLAGS
{
    REGION_TL,
    REGION_BR
};

static UREF_COMMS /* reason code out */
uref_region_ends(
    _InoutRef_  P_REGION p_region,
    _InVal_     enum START_END_FLAGS start_end_flags,
    _InVal_     UREF_MESSAGE uref_message,
    P_UREF_PARMS p_uref_parms,
    _InVal_     BOOL add_col,
    _InVal_     BOOL add_row)
{
    UREF_COMMS res = Uref_Dep_None;
    P_SLR p_slr;

    switch(start_end_flags)
    {
    case REGION_TL:
        p_slr = &p_region->tl;
        break;

    case REGION_BR:
        p_slr = &p_region->br;
        /* adjust slr */
        if(!add_col)
            p_slr->col -= 1;
        if(!add_row)
            p_slr->row -= 1;
        break;

    default: default_unhandled();
        return(Uref_Dep_None);
    }

    switch(uref_message)
    {
    case Uref_Msg_Uref:
        if( (p_region->whole_col || row_in_region(&p_uref_parms->source.region, p_slr->row))
            &&
            (p_region->whole_row || col_in_region(&p_uref_parms->source.region, p_slr->col)) )
        {
            if(!p_region->whole_row)
            {
                p_slr->col += p_uref_parms->target.slr.col;
                p_slr->col = MAX(0, p_slr->col);
                res = Uref_Dep_Update;
            }

            if(!p_region->whole_col)
            {
                p_slr->row += p_uref_parms->target.slr.row;
                p_slr->row = MAX(0, p_slr->row);
                res = Uref_Dep_Update;
            }
        }
        break;

    case Uref_Msg_Delete:
        if( (p_region->whole_col || row_in_region(&p_uref_parms->source.region, p_slr->row))
            &&
            (p_region->whole_row || col_in_region(&p_uref_parms->source.region, p_slr->col)) )
             res = Uref_Dep_Delete;
         break;

    case Uref_Msg_Overwrite:
    case Uref_Msg_Change:
         if( (p_region->whole_col || row_in_region(&p_uref_parms->source.region, p_slr->row))
             &&
             (p_region->whole_row || col_in_region(&p_uref_parms->source.region, p_slr->col)) )
             res = Uref_Dep_Inform;
         break;

    case Uref_Msg_Swap_Rows:
        if(col_in_region(&p_uref_parms->source.region, p_slr->col))
        {
            if(p_slr->row == p_uref_parms->source.region.tl.row)
            {
                p_slr->row = p_uref_parms->source.region.br.row;
                res = Uref_Dep_Update;
            }
            else if(p_slr->row == p_uref_parms->source.region.br.row)
            {
                p_slr->row = p_uref_parms->source.region.tl.row;
                res = Uref_Dep_Update;
            }
        }
        break;

    case Uref_Msg_CLOSE1:
    case Uref_Msg_CLOSE2:
        res = Uref_Dep_Delete;
        break;

    default: default_unhandled();
        break;
    }

    switch(start_end_flags)
    {
    case REGION_BR:
        /* readjust slr */
        if(!add_col)
            p_slr->col += 1;
        if(!add_row)
            p_slr->row += 1;
        break;

    case REGION_TL:
    default:
        break;
    }

    return(res);
}

/******************************************************************************
*
* add a dependent to the document
*
******************************************************************************/

_Check_return_
extern STATUS
uref_add_dependency(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REGION p_region,
    _InRef_     P_PROC_UREF_EVENT p_proc_uref_event /* uref event routine */,
    _InVal_     CLIENT_HANDLE client_handle /* clients handle in */,
    _OutRef_    P_UREF_HANDLE p_uref_handle  /* urefs handle out */,
    _InVal_     BOOL simply_allocate)
{
    P_UREF_ENTRY p_uref_entry = P_UREF_ENTRY_NONE;

    *p_uref_handle = UREF_HANDLE_INVALID;

    /* SKS 19feb97 allow for speed up loading loads of regions */
    if(!simply_allocate)
    {   /* look for a spare entry */
        const ARRAY_INDEX n_regions = array_elements(&p_docu->h_uref_table);
        ARRAY_INDEX uref_ix;
        P_UREF_ENTRY p_uref_entry_i = array_range(&p_docu->h_uref_table, UREF_ENTRY, 0, n_regions);

        for(uref_ix = 0; uref_ix < n_regions; ++uref_ix, ++p_uref_entry_i)
        {
            if(p_uref_entry_i->is_deleted)
            {
                p_uref_entry = p_uref_entry_i;
                p_uref_entry->is_deleted = 0;
                break;
            }
        }
    }

    if(IS_P_DATA_NONE(p_uref_entry))
    {
        STATUS status;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(UREF_ENTRY), TRUE);
        if(NULL == (p_uref_entry = al_array_extend_by(&p_docu->h_uref_table, UREF_ENTRY, 1, &array_init_block, &status)))
            return(status);
        al_array_auto_compact_set(&p_docu->h_uref_table);
    }

    p_uref_entry->uref_id.region = *p_region;
    p_uref_entry->uref_id.client_handle = client_handle;
    p_uref_entry->uref_id.uref_handle = next_uref_handle++;
    p_uref_entry->p_proc_uref_event = p_proc_uref_event;

    *p_uref_handle = p_uref_entry->uref_id.uref_handle;

    return(STATUS_OK);
}

/******************************************************************************
*
* change a client handle
*
******************************************************************************/

_Check_return_
extern STATUS
uref_change_handle(
    _InVal_     DOCNO docno,
    _InVal_     UREF_HANDLE uref_handle,
    _InVal_     CLIENT_HANDLE client_handle)
{
    const P_DOCU p_docu = p_docu_from_docno(docno);
    const ARRAY_INDEX n_regions = array_elements(&p_docu->h_uref_table);
    ARRAY_INDEX region_ix;
    P_UREF_ENTRY p_uref_entry = array_range(&p_docu->h_uref_table, UREF_ENTRY, 0, n_regions);
    STATUS status = STATUS_FAIL;

    for(region_ix = 0; region_ix < n_regions; ++region_ix, ++p_uref_entry)
    {
        if(p_uref_entry->is_deleted)
            continue;

        if(p_uref_entry->uref_id.uref_handle == uref_handle)
        {
            p_uref_entry->uref_id.client_handle = client_handle;
            status = STATUS_OK;
            break;
        }
    }

    return(status);
}

/******************************************************************************
*
* delete a dependency from a document
*
******************************************************************************/

extern void
uref_del_dependency(
    _InVal_     DOCNO docno,
    _InVal_     UREF_HANDLE uref_handle)
{
    const P_DOCU p_docu = p_docu_from_docno(docno);
    const ARRAY_INDEX n_regions = array_elements(&p_docu->h_uref_table);
    ARRAY_INDEX region_ix;
    P_UREF_ENTRY p_uref_entry = array_range(&p_docu->h_uref_table, UREF_ENTRY, 0, n_regions);
    BOOL found_it = FALSE;

    for(region_ix = 0; region_ix < n_regions; ++region_ix, ++p_uref_entry)
    {
        if(p_uref_entry->is_deleted)
            continue;

        if(p_uref_entry->uref_id.uref_handle == uref_handle)
        {
            p_uref_entry->is_deleted = 1;
            found_it = TRUE;
            break;
        }
    }

    assert(found_it);
}

/******************************************************************************
*
* remove all traces of a file when it is closed and the handle becomes invalid
*
******************************************************************************/

PROC_ELEMENT_IS_DELETED_PROTO(static, uref_element_is_deleted)
{
    const P_UREF_ENTRY p_uref_entry = (P_UREF_ENTRY) p_any;

    return(p_uref_entry->is_deleted);
}

static void
uref_event_after_msg_close2(
    _DocuRef_   P_DOCU closing_p_docu)
{
    /* for all loaded thunks, garbage collect and free uref tables if possible */
    DOCNO docno = DOCNO_NONE;

    UNREFERENCED_PARAMETER_DocuRef_(closing_p_docu);

    while(DOCNO_NONE != (docno = docno_enum_thunks(docno)))
    {
        const P_DOCU this_p_docu = p_docu_from_docno(docno);
        AL_GARBAGE_FLAGS al_garbage_flags;
        AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
        al_garbage_flags.remove_deleted = 1;
        al_garbage_flags.shrink = 1;
        al_garbage_flags.may_dispose = 1; /* and hopefully does so if it is this document being closed... */
        consume(S32, al_array_garbage_collect(&this_p_docu->h_uref_table, 0, uref_element_is_deleted, al_garbage_flags));
        assert((0 == this_p_docu->h_uref_table) || (this_p_docu != closing_p_docu));
    }
}

/******************************************************************************
*
* main event handler for uref events
*
* see comments on individual cases for argument usage (passed in *p_uref_event_block)
*
******************************************************************************/

/*ncr*/
extern STATUS
uref_event(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    /*_Inout_*/ P_ANY p_data)
{
    UREF_EVENT_BLOCK uref_event_block;

    if(IS_DOCU_NONE(p_docu))
        return(STATUS_OK);

    switch(uref_message)
    {
    case Uref_Msg_Change:
    case Uref_Msg_Delete:
    case Uref_Msg_Overwrite:
    case Uref_Msg_Uref:
    case Uref_Msg_Swap_Rows:
        {
        P_UREF_PARMS p_uref_parms = (P_UREF_PARMS) p_data;

        uref_event_block.uref_parms = *p_uref_parms; /* details of operation are in source, target */
        }

        /*FALLTHRU*/

    case Uref_Msg_CLOSE1: /* document being closed */
    case Uref_Msg_CLOSE2:
        {
        const ARRAY_INDEX n_regions = array_elements(&p_docu->h_uref_table);
        ARRAY_INDEX region_ix;
        P_UREF_ENTRY p_uref_entry = array_range(&p_docu->h_uref_table, UREF_ENTRY, 0, n_regions);

        /* call hangers on */
        for(region_ix = 0; region_ix < n_regions; ++region_ix, ++p_uref_entry)
        {
            UREF_COMMS res;

            if(p_uref_entry->is_deleted)
                continue;

            res = uref_match_region(&p_uref_entry->uref_id.region, uref_message, &uref_event_block);

            if(Uref_Dep_None != res)
            {
                uref_event_block.reason.code = UBF_PACK(res);
                uref_event_block.uref_id = p_uref_entry->uref_id;
                (* p_uref_entry->p_proc_uref_event) (p_docu, uref_message, &uref_event_block);
            }
        }

        if(Uref_Msg_CLOSE2 == uref_message)
            uref_event_after_msg_close2(p_docu);

        return(STATUS_OK);
        }

    /* ignore messages in general */
    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* check a uref event against a region and derive a reason code which can be passed to hangers on
*
******************************************************************************/

extern UREF_COMMS /* reason code out */
uref_match_region(
    _InoutRef_  P_REGION p_region,
    _InVal_     UREF_MESSAGE uref_message,
    P_UREF_EVENT_BLOCK p_uref_event_block)
{
    UREF_COMMS res = Uref_Dep_None;

    switch(uref_message)
    {
    case Uref_Msg_Uref:
        {
        BOOL colspan, rowspan, do_uref = FALSE;
        UREF_COMMS res_s = Uref_Dep_None, res_e = Uref_Dep_None;
        BOOL add_col = FALSE, add_row = FALSE;

        region_span(&colspan, &rowspan, p_region, &p_uref_event_block->uref_parms.source.region);

        /* if all cols and rows spanned, we can move anywhere;
         * if all cols spanned and only moving up or down, OK;
         * if all rows spanned and only moving left or right, OK
         */
        if(colspan && rowspan)
            do_uref = TRUE;
        else if(colspan && p_uref_event_block->uref_parms.target.slr.row && !p_uref_event_block->uref_parms.target.slr.col)
            do_uref = TRUE;
        else if(rowspan && p_uref_event_block->uref_parms.target.slr.col && !p_uref_event_block->uref_parms.target.slr.row)
            do_uref = TRUE;

        /* when doing an add operation and region is greater than
         * a single unit in size, then stretch the region
         */
        if(p_uref_event_block->uref_parms.add_col && (p_region->whole_row || ((p_region->br.col - p_region->tl.col) > 1)))
            add_col = TRUE;
        if(p_uref_event_block->uref_parms.add_row && (p_region->whole_col || ((p_region->br.row - p_region->tl.row) > 1)))
            add_row = TRUE;

        if(do_uref)
        {
            res_s = uref_region_ends(p_region, REGION_TL, uref_message, &p_uref_event_block->uref_parms, add_col, add_row);
            res_e = uref_region_ends(p_region, REGION_BR, uref_message, &p_uref_event_block->uref_parms, add_col, add_row);
        }

        if( (Uref_Dep_Delete == res_s) || (Uref_Dep_Delete == res_e) )
            res = Uref_Dep_Delete;
        else if( (Uref_Dep_Update == res_s) || (Uref_Dep_Update == res_e) )
        {
            if(region_empty(p_region))
                res = Uref_Dep_Delete;
            else
                res = Uref_Dep_Update;
        }
        else if(region_intersect_region(p_region, &p_uref_event_block->uref_parms.source.region))
            res = Uref_Dep_Inform;
        break;
        }

    case Uref_Msg_Delete:
        {
        BOOL colspan, rowspan;

        region_span(&colspan, &rowspan, p_region, &p_uref_event_block->uref_parms.source.region);

        /* range completely deleted ? */
        if(colspan && rowspan)
            res = Uref_Dep_Delete;
        else if(colspan && !p_region->whole_col)
        {
            /* lop off some rows of the dependency */
            if(Uref_Dep_Delete == uref_region_ends(p_region, REGION_TL, uref_message, &p_uref_event_block->uref_parms, FALSE, FALSE))
            {
                p_region->tl.row = p_uref_event_block->uref_parms.source.region.br.row;
                res = Uref_Dep_Update;
            }
            else if(Uref_Dep_Delete == uref_region_ends(p_region, REGION_BR, uref_message, &p_uref_event_block->uref_parms, FALSE, FALSE))
            {
                p_region->br.row = p_uref_event_block->uref_parms.source.region.tl.row;
                res = Uref_Dep_Update;
            }
        }
        else if(rowspan && !p_region->whole_row)
        {
            /* lop off some columns of the dependency */
            if(Uref_Dep_Delete == uref_region_ends(p_region, REGION_TL, uref_message, &p_uref_event_block->uref_parms, FALSE, FALSE))
            {
                p_region->tl.col = p_uref_event_block->uref_parms.source.region.br.col;
                res = Uref_Dep_Update;
            }
            else if(Uref_Dep_Delete == uref_region_ends(p_region, REGION_BR, uref_message, &p_uref_event_block->uref_parms, FALSE, FALSE))
            {
                p_region->br.col = p_uref_event_block->uref_parms.source.region.tl.col;
                res = Uref_Dep_Update;
            }
        }

        /* if the area affected and the range intersect
         * at all, then the dependents must be told
         */
        if( (Uref_Dep_None == res) && region_intersect_region(p_region, &p_uref_event_block->uref_parms.source.region) )
            res = Uref_Dep_Inform;
        break;
        }

    /* inform if regions intersect */
    case Uref_Msg_Overwrite:
    case Uref_Msg_Change:
        if(region_intersect_region(p_region, &p_uref_event_block->uref_parms.source.region))
            res = Uref_Dep_Inform;
        break;

    case Uref_Msg_Swap_Rows:
        {
        BOOL colspan_1, rowspan_1, colspan_2, rowspan_2;
        REGION region_1, region_2;

        region_1 = p_uref_event_block->uref_parms.source.region;
        region_1.br.row = region_1.tl.row + 1;
        region_span(&colspan_1, &rowspan_1, p_region, &region_1);

        region_2 = p_uref_event_block->uref_parms.source.region;
        region_2.tl.row = region_2.br.row;
        region_2.br.row = region_2.tl.row + 1;
        region_span(&colspan_2, &rowspan_2, p_region, &region_2);

        /* we can only swap when the entire region fits into the area being swapped
         */
        if(colspan_1 && colspan_2 && (rowspan_1 || rowspan_2))
        {
            uref_region_ends(p_region, REGION_TL, uref_message, &p_uref_event_block->uref_parms, FALSE, FALSE);
            uref_region_ends(p_region, REGION_BR, uref_message, &p_uref_event_block->uref_parms, FALSE, FALSE);
            res = Uref_Dep_Update;
        }
        else if(region_intersect_region(p_region, &region_1))
            res = Uref_Dep_Inform;
        else if(region_intersect_region(p_region, &region_2))
            res = Uref_Dep_Inform;
        break;
        }

    case Uref_Msg_CLOSE1:
    case Uref_Msg_CLOSE2:
        res = Uref_Dep_Delete;
        break;

    default: default_unhandled();
        break;
    }

    return(res);
}

/******************************************************************************
*
* perform uref on a docu_area
* (ignores sub-object positions)
*
******************************************************************************/

extern UREF_COMMS /* reason code out */
uref_match_docu_area(
    _InoutRef_  P_DOCU_AREA p_docu_area,
    _InVal_     UREF_MESSAGE uref_message,
    P_UREF_EVENT_BLOCK p_uref_event_block)
{
    REGION region;
    UREF_COMMS res;

    region_from_docu_area_max(&region, p_docu_area);

    trace_0(TRACE_APP_UREF, TEXT("uref_match_docu_area 0"));

    if(Uref_Dep_None == (res = uref_match_region(&region, uref_message, p_uref_event_block)))
        return(Uref_Dep_None);

    p_docu_area->tl.slr = region.tl;
    p_docu_area->br.slr = region.br;

    return(res);
}

/******************************************************************************
*
* check a uref event against an slr and derive
* a reason code for hangers on
*
******************************************************************************/

extern UREF_COMMS /* reason code out */
uref_match_slr(
    _InoutRef_  P_SLR p_slr,
    _InVal_     UREF_MESSAGE uref_message,
    P_UREF_EVENT_BLOCK p_uref_event_block)
{
    REGION region;
    UREF_COMMS res;

    region_from_two_slrs(&region, p_slr, p_slr, FALSE);

    res = uref_region_ends(&region, REGION_TL, uref_message, &p_uref_event_block->uref_parms, FALSE, FALSE);

    *p_slr = region.tl;

    return(res);
}

_Check_return_
_Ret_z_
extern PCTSTR
uref_report_message(
    _InVal_     UREF_MESSAGE uref_message)
{
    switch(uref_message)
    {
    case Uref_Msg_Uref:      return(TEXT("Uref_Msg_Uref"));
    case Uref_Msg_Delete:    return(TEXT("Uref_Msg_Delete"));
    case Uref_Msg_Swap_Rows: return(TEXT("Uref_Msg_Swap_Rows"));
    case Uref_Msg_Change:    return(TEXT("Uref_Msg_Change"));
    case Uref_Msg_Overwrite: return(TEXT("Uref_Msg_Overwrite"));
    case Uref_Msg_CLOSE1:    return(TEXT("Uref_Msg_CLOSE1"));
    case Uref_Msg_CLOSE2:    return(TEXT("Uref_Msg_CLOSE2"));
    default:                 return(TEXT("Uref_Msg_Unknown"));
    }
}

#if TRACE_ALLOWED

/******************************************************************************
*
* trace uref reason code
*
******************************************************************************/

extern void
uref_trace_reason(
    _InVal_     UREF_COMMS reason_code,
    _In_z_      PCTSTR tstr)
{
    switch(reason_code)
    {
    case Uref_Dep_Delete: trace_1(TRACE_APP_UREF, TEXT("%s UREF Uref_Dep_Delete"), tstr); break;
    case Uref_Dep_Update: trace_1(TRACE_APP_UREF, TEXT("%s UREF Uref_Dep_Update"), tstr); break;
    case Uref_Dep_Inform: trace_1(TRACE_APP_UREF, TEXT("%s UREF Uref_Dep_Inform"), tstr); break;
    case Uref_Dep_None:   trace_1(TRACE_APP_UREF, TEXT("%s UREF Uref_Dep_None"), tstr); break;
    }
}

#endif

/*
old swap cell code
*/

#if 0

/*
from uref_region_ends
*/

case Uref_Msg_SWAPSLOT:
    if(slr_equal(ref, &p_uref_parms->source.region.tl))
        res = uref_swapslot(ref, &p_uref_parms->source.region.br);
    else if(slr_equal(ref, &p_uref_parms->source.region.br))
        res = uref_swapslot(ref, &p_uref_parms->source.region.tl);
    break;

/*
from uref_match_region
*/

        /* (range may be a cell after all) */
        case Uref_Msg_SWAPSLOT:
            if(!(p_region->tl.flags & SLR_ALL_REF))
            {
                /* check if the whole range can be moved */
                if(p_region->br.col == p_region->tl.col + 1 &&
                   p_region->br.row == p_region->tl.row + 1)
                {
                    if(slr_equal(&p_region->s, &p_uref_event_block->uref_parms.slr_source1))
                    {
                        p_region->s = p_uref_event_block->uref_parms.slr_source2;
                        res = Uref_Dep_Update;
                    }
                    else if(slr_equal(&p_region->s, &p_uref_event_block->uref_parms.slr_source2))
                    {
                        p_region->s = p_uref_event_block->uref_parms.slr_source1;
                        res = Uref_Dep_Update;
                    }

                    if(Uref_Dep_Update == res)
                    {
                        p_region->e = p_region->s;
                        p_region->br.col += 1;
                        p_region->br.row += 1;
                    }
                }
            }

            /* tell people who have ranges affected */
            if(Uref_Dep_Update != res &&
                (slr_in_range(p_region, &p_uref_event_block->uref_parms.slr_source1) ||
                 slr_in_range(p_region, &p_uref_event_block->uref_parms.slr_source2)))
                res = Uref_Dep_Inform;
            break;

/******************************************************************************
*
* update reference when cells are swapped
*
******************************************************************************/

static int
uref_swapslot(
    ev_slrp ref,
    ev_slrp slrp)
{
    *ref = *slrp;
    return(Uref_Dep_Update);
}

#endif

/* end of sk_uref.c */
