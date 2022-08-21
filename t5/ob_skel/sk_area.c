/* sk_area.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MRJC August 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/******************************************************************************
*
* compare two data_refs for equality
*
******************************************************************************/

_Check_return_
extern BOOL
data_refs_equal(
    _InRef_     PC_DATA_REF p_data_ref_1,
    _InRef_     PC_DATA_REF p_data_ref_2)
{
    profile_ensure_frame();

    if(p_data_ref_1->data_space == p_data_ref_2->data_space)
    {
        switch(p_data_ref_1->data_space)
        {
        default: default_unhandled();
#if CHECKING
        case DATA_NONE:
#endif
            return(TRUE);

        case DATA_HEADER_ODD:
        case DATA_FOOTER_ODD:
        case DATA_HEADER_EVEN:
        case DATA_FOOTER_EVEN:
        case DATA_HEADER_FIRST:
        case DATA_FOOTER_FIRST:
            return(p_data_ref_1->arg.row == p_data_ref_2->arg.row);

        case DATA_SLOT:
            return(slr_equal(&p_data_ref_1->arg.slr, &p_data_ref_2->arg.slr));

        case DATA_DB_FIELD:
            return((p_data_ref_1->arg.db_field.db_id    == p_data_ref_2->arg.db_field.db_id     ) &&
                   (p_data_ref_1->arg.db_field.record   == p_data_ref_2->arg.db_field.record    ) &&
                   (p_data_ref_1->arg.db_field.field_id == p_data_ref_2->arg.db_field.field_id  ) );

        case DATA_DB_TITLE:
            return((p_data_ref_1->arg.db_title.db_id    == p_data_ref_2->arg.db_title.db_id     ) &&
                   (p_data_ref_1->arg.db_title.record   == p_data_ref_2->arg.db_title.record    ) &&
                   (p_data_ref_1->arg.db_title.field_id == p_data_ref_2->arg.db_title.field_id  ) );
        }
    }

    if((p_data_ref_1->data_space == DATA_NONE) || (p_data_ref_2->data_space == DATA_NONE))
        return(TRUE);

    return(FALSE);
}

/******************************************************************************
*
* is the data ref inside the region ?
*
******************************************************************************/

_Check_return_
static BOOL
field_or_title_data_ref_in_region(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     DATA_SPACE data_space,
    _InRef_     PC_REGION p_region,
    _InRef_     PC_DATA_REF p_data_ref)
{
    DATA_REF_AND_SLR data_ref_and_slr;

    UNREFERENCED_PARAMETER_InVal_(data_space);

    data_ref_and_slr.data_ref = *p_data_ref;

    if(status_ok(object_call_id(OBJECT_ID_REC, p_docu, T5_MSG_DATA_REF_TO_SLR, &data_ref_and_slr)))
        return(slr_in_region(p_region, &data_ref_and_slr.slr));

    return(FALSE);
}

_Check_return_
extern BOOL
data_ref_in_region(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     DATA_SPACE data_space,
    _InRef_     PC_REGION p_region,
    _InRef_     PC_DATA_REF p_data_ref)
{
    if((data_space == DATA_NONE) || (p_data_ref->data_space == DATA_NONE))
        return(TRUE);

    if(data_space != p_data_ref->data_space)
        return(FALSE);

    switch(data_space)
    {
    case DATA_NONE:
        return(FALSE);

    default: default_unhandled();
#if CHECKING
    case DATA_HEADER_ODD:
    case DATA_FOOTER_ODD:
    case DATA_HEADER_EVEN:
    case DATA_FOOTER_EVEN:
    case DATA_HEADER_FIRST:
    case DATA_FOOTER_FIRST:
#endif
        return(row_in_region(p_region, p_data_ref->arg.row));

    case DATA_SLOT:
        return(slr_in_region(p_region, &p_data_ref->arg.slr));

    case DATA_DB_FIELD:
    case DATA_DB_TITLE:
        return(field_or_title_data_ref_in_region(p_docu, data_space, p_region, p_data_ref));
    }
}

/******************************************************************************
*
* call objects to adjust start/end positions
* when working out if docu_area is empty
*
******************************************************************************/

#if 0

/* MRJC: code extracted from docu_area_is_empty 12.10.94 and left to wither a bit */

extern void
docu_area_adjust(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_in)
{
    BOOL position_set = 0;
    S32 start = 0;
    OBJECT_POSITION_SET object_position_set;

    *p_docu_area_out = *p_docu_area_in;

    if(OBJECT_ID_NONE != p_docu_area_in->tl.object_position.object_id)
    {
        if(STATUS_DONE == object_data_from_docu_area_tl(p_docu, &object_position_set.object_data, p_docu_area_in))
        {
            start = p_docu_area_in->tl.object_position.data;
            position_set = 1;
        }
    }
    else if(OBJECT_ID_NONE != p_docu_area_in->br.object_position.object_id)
    {
        if(STATUS_DONE == object_data_from_docu_area_br(p_docu, &object_position_set.object_data, p_docu_area_in))
            position_set = 1;
    }

    if(position_set)
    {
        object_position_set.action = OBJECT_POSITION_SET_END;
        status_consume(object_call_id(object_position_set.object_data.object_id, p_docu, T5_MSG_OBJECT_POSITION_SET, &object_position_set));

        if(start >= object_position_set.object_data.object_position_start.data)
            return(TRUE);
    }
}

#endif

/******************************************************************************
*
* are two docu areas adjacent - can they be coalesced ?
*
******************************************************************************/

/*ncr*/
extern BOOL
docu_area_coalesce_docu_area_out(
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_1,
    _InRef_     PC_DOCU_AREA p_docu_area_2,
    _In_        COALESCE_FLAGS coalesce_flags)
{
    BOOL do_it = 0, object_positions = 0, rows_match, cols_match;
    DOCU_AREA docu_area_1, docu_area_2;

    /* whole bits must be identical */
    if((p_docu_area_1->whole_col != p_docu_area_2->whole_col) ||
       (p_docu_area_1->whole_row != p_docu_area_2->whole_row))
        return(FALSE);

    p_docu_area_out->whole_col = p_docu_area_1->whole_col;
    p_docu_area_out->whole_row = p_docu_area_1->whole_row;

    docu_area_1 = *p_docu_area_1;
    docu_area_2 = *p_docu_area_2;

    cols_match = docu_area_1.tl.slr.col == docu_area_2.tl.slr.col &&
                 docu_area_1.br.slr.col == docu_area_2.br.slr.col;

    rows_match = docu_area_1.tl.slr.row == docu_area_2.tl.slr.row &&
                 docu_area_1.br.slr.row == docu_area_2.br.slr.row;

    if(!docu_area_1.whole_col && !docu_area_2.whole_row
       &&
       (position_clear_object_positions(&docu_area_1.tl, FALSE) ||
        position_clear_object_positions(&docu_area_1.br, TRUE)))
        object_positions = 1;

    if(!docu_area_2.whole_col && !docu_area_2.whole_row
       &&
       (position_clear_object_positions(&docu_area_2.tl, FALSE) ||
        position_clear_object_positions(&docu_area_2.br, TRUE)))
        object_positions = 1;

    if(docu_area_1.whole_col
       ||
       rows_match)
    {
        /* regions cover the same rows */
        if(docu_area_intersect_docu_area(p_docu_area_1, p_docu_area_2))
            do_it = coalesce_flags.cols || (coalesce_flags.frags && cols_match);
        else if(docu_area_1.whole_row)
            do_it = coalesce_flags.cols;
        else if(!object_positions &&
                (docu_area_1.br.slr.col == docu_area_2.tl.slr.col ||
                 docu_area_1.tl.slr.col == docu_area_2.br.slr.col))
            do_it = coalesce_flags.cols;
        /* fragments adjacent in a single cell */
        else if(cols_match)
        {
            if( (OBJECT_ID_NONE != docu_area_1.tl.object_position.object_id) &&
                (OBJECT_ID_NONE != docu_area_2.br.object_position.object_id) &&
#if 1
                !object_position_compare(&docu_area_1.tl.object_position, &docu_area_2.br.object_position, OBJECT_POSITION_COMPARE_SE))
#else
                docu_area_1.tl.object_position.data == docu_area_2.br.object_position.data)
#endif
                do_it = coalesce_flags.frags;
            else if( (OBJECT_ID_NONE != docu_area_1.br.object_position.object_id) &&
                     (OBJECT_ID_NONE != docu_area_2.tl.object_position.object_id) &&
#if 1
                     !object_position_compare(&docu_area_2.tl.object_position, &docu_area_1.br.object_position, OBJECT_POSITION_COMPARE_SE))
#else
                     docu_area_1.br.object_position.data == docu_area_2.tl.object_position.data)
#endif
                do_it = coalesce_flags.frags;
        }
    }
    else if(docu_area_1.whole_row
            ||
            cols_match)
    {
        /* regions cover the same cols */
        if(docu_area_intersect_docu_area(p_docu_area_1, p_docu_area_2))
            do_it = coalesce_flags.rows;
        else if(docu_area_1.whole_col)
            do_it = coalesce_flags.rows;
        else if(docu_area_1.br.slr.row == docu_area_2.tl.slr.row)
        {
            if(!object_positions)
                do_it = coalesce_flags.rows;
            else if( (OBJECT_ID_NONE != docu_area_1.br.object_position.object_id) &&
                     (OBJECT_ID_NONE != docu_area_2.tl.object_position.object_id) &&
#if 1
                     !object_position_compare(&docu_area_2.tl.object_position, &docu_area_1.br.object_position, OBJECT_POSITION_COMPARE_SE))
#else
                     docu_area_1.br.object_position.data == docu_area_2.tl.object_position.data)
#endif
                do_it = coalesce_flags.rows;
        }
        else if(docu_area_1.tl.slr.row == docu_area_2.br.slr.row)
        {
            if(!object_positions)
                do_it = coalesce_flags.rows;
            else if( (OBJECT_ID_NONE != docu_area_1.tl.object_position.object_id) &&
                     (OBJECT_ID_NONE != docu_area_2.br.object_position.object_id) &&
#if 1
                     !object_position_compare(&docu_area_1.tl.object_position, &docu_area_2.br.object_position, OBJECT_POSITION_COMPARE_SE))
#else
                     docu_area_1.tl.object_position.data == docu_area_2.br.object_position.data)
#endif
                do_it = coalesce_flags.rows;
        }
    }

    if(do_it)
    {
        docu_area_union_docu_area_out(p_docu_area_out, p_docu_area_1, p_docu_area_2);
        return(TRUE);
    }

    return(FALSE);
}

/******************************************************************************
*
* clean up a docu area
*
******************************************************************************/

extern void
docu_area_clean_up(
    _InoutRef_  P_DOCU_AREA p_docu_area)
{
    if(p_docu_area->whole_col)
    {
        p_docu_area->tl.slr.row = -1;
        p_docu_area->br.slr.row = -1;
    }

    if(p_docu_area->whole_row)
    {
        p_docu_area->tl.slr.col = -1;
        p_docu_area->br.slr.col = -1;
    }

    if(p_docu_area->whole_col || p_docu_area->whole_row)
    {
        object_position_init(&p_docu_area->tl.object_position);
        object_position_init(&p_docu_area->br.object_position);
    }
    else
    {
        if(object_position_at_start(&p_docu_area->tl.object_position))
            object_position_init(&p_docu_area->tl.object_position);

        if(OBJECT_ID_NONE == p_docu_area->br.object_position.object_id)
            object_position_init(&p_docu_area->br.object_position);
        else if(object_position_at_start(&p_docu_area->br.object_position))
        {
            object_position_init(&p_docu_area->br.object_position);
            p_docu_area->br.slr.row -= 1;
        }
    }
}

/******************************************************************************
*
* is this an empty docu_area ?
*
* if p_docu is non NULL, the object
* positions may be checked
*
******************************************************************************/

/* split out 30sep14 to eliminate stack frame, fast out for non-whole-col, non-whole-row */

_Check_return_
static BOOL
docu_area_wcr_is_empty(
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    if(slr_last_in_docu_area(p_docu_area, &p_docu_area->tl.slr))
        if(p_docu_area->tl.object_position.object_id == p_docu_area->br.object_position.object_id)
            if(OBJECT_ID_NONE != p_docu_area->tl.object_position.object_id)
#if 1
                if(object_position_compare(&p_docu_area->tl.object_position,
                                           &p_docu_area->br.object_position,
                                           OBJECT_POSITION_COMPARE_SE) >= 0)
#else
                if(p_docu_area->tl.object_position.data >= p_docu_area->br.object_position.data)
#endif
                    return(TRUE);

    return(FALSE);
}

_Check_return_
extern BOOL
docu_area_is_empty(
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    if(!p_docu_area->whole_row)
        if(p_docu_area->tl.slr.col >= p_docu_area->br.slr.col)
            return(TRUE);

    if(!p_docu_area->whole_col)
        if(p_docu_area->tl.slr.row >= p_docu_area->br.slr.row)
            return(TRUE);

    return(docu_area_wcr_is_empty(p_docu_area));
}

/******************************************************************************
*
* are the two docu_areas identical ?
*
******************************************************************************/

_Check_return_
extern BOOL
docu_area_equal(
    _InRef_     PC_DOCU_AREA p_docu_area_1,
    _InRef_     PC_DOCU_AREA p_docu_area_2)
{
    if(p_docu_area_1->whole_row != p_docu_area_2->whole_row)
        return(FALSE);

    if(!p_docu_area_1->whole_row)
        if(p_docu_area_1->tl.slr.col != p_docu_area_2->tl.slr.col ||
           p_docu_area_1->br.slr.col != p_docu_area_2->br.slr.col)
            return(FALSE);

    if(p_docu_area_1->whole_col != p_docu_area_2->whole_col)
        return(FALSE);

    if(!p_docu_area_1->whole_col)
        if(p_docu_area_1->tl.slr.row != p_docu_area_2->tl.slr.row ||
           p_docu_area_1->br.slr.row != p_docu_area_2->br.slr.row)
            return(FALSE);

    if(!p_docu_area_1->whole_col &&
       !p_docu_area_1->whole_row &&
       (p_docu_area_1->tl.object_position.object_id != p_docu_area_2->tl.object_position.object_id) ||
       (p_docu_area_1->br.object_position.object_id != p_docu_area_2->br.object_position.object_id) ||
#if 1
       ((OBJECT_ID_NONE != p_docu_area_1->tl.object_position.object_id) &&
        (object_position_compare(&p_docu_area_1->tl.object_position,
                                 &p_docu_area_2->tl.object_position,
                                 OBJECT_POSITION_COMPARE_PP) != 0) ) ||
       ((OBJECT_ID_NONE != p_docu_area_1->br.object_position.object_id) &&
        (object_position_compare(&p_docu_area_1->br.object_position,
                                 &p_docu_area_2->br.object_position,
                                 OBJECT_POSITION_COMPARE_PP) != 0) )
#else
       ((OBJECT_ID_NONE != p_docu_area_1->tl.object_position.object_id) &&
        (p_docu_area_1->tl.object_position.data != p_docu_area_2->tl.object_position.data) ) ||
       ((OBJECT_ID_NONE != p_docu_area_1->br.object_position.object_id) &&
        (p_docu_area_1->br.object_position.data != p_docu_area_2->br.object_position.data) )
#endif
      )
        return(FALSE);

    return(TRUE);
}

/******************************************************************************
*
* make a docu_area from object data
*
******************************************************************************/

/*ncr*/
_Ret_valid_
extern P_DOCU_AREA
docu_area_from_object_data(
    _OutRef_    P_DOCU_AREA p_docu_area,
    _InRef_     PC_OBJECT_DATA p_object_data)
{
    docu_area_init(p_docu_area);

    if(p_object_data->data_ref.data_space == DATA_SLOT)
        p_docu_area->tl.slr = p_object_data->data_ref.arg.slr;

    p_docu_area->br.slr.col = p_docu_area->tl.slr.col + 1;
    p_docu_area->br.slr.row = p_docu_area->tl.slr.row + 1;

    p_docu_area->tl.object_position = p_object_data->object_position_start;
    p_docu_area->br.object_position = p_object_data->object_position_end;

    return(p_docu_area);
}

/******************************************************************************
*
* make a docu area covering a single cell
*
******************************************************************************/

/*ncr*/
_Ret_valid_
extern P_DOCU_AREA
docu_area_from_position(
    _OutRef_    P_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position)
{
    docu_area_init(p_docu_area);

    p_docu_area->tl = p_docu_area->br = *p_position;
    p_docu_area->br.slr.col += 1;
    p_docu_area->br.slr.row += 1;

    return(p_docu_area);
}

/******************************************************************************
*
* make a docu area covering a single cell
*
******************************************************************************/

/*ncr*/
_Ret_valid_
extern P_DOCU_AREA
docu_area_from_position_max(
    _OutRef_    P_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position)
{
    docu_area_init(p_docu_area);

    p_docu_area->tl.slr = p_docu_area->br.slr = p_position->slr;
    p_docu_area->br.slr.col += 1;
    p_docu_area->br.slr.row += 1;

    return(p_docu_area);
}

/******************************************************************************
*
* make a docu_area from a region
*
******************************************************************************/

/*ncr*/
_Ret_valid_
extern P_DOCU_AREA
docu_area_from_region(
    _OutRef_    P_DOCU_AREA p_docu_area,
    _InRef_     PC_REGION p_region)
{
    profile_ensure_frame();

    p_docu_area->tl.slr = p_region->tl;
    p_docu_area->tl.object_position.object_id = OBJECT_ID_NONE;

    p_docu_area->br.slr = p_region->br;
    p_docu_area->br.object_position.object_id = OBJECT_ID_NONE;

    p_docu_area->whole_col = p_region->whole_col;
    p_docu_area->whole_row = p_region->whole_row;

    return(p_docu_area);
}

/******************************************************************************
*
* make a docu area covering a single cell
*
******************************************************************************/

/*ncr*/
_Ret_valid_
extern P_DOCU_AREA
docu_area_from_slr(
    _OutRef_    P_DOCU_AREA p_docu_area,
    _InRef_     PC_SLR p_slr)
{
    docu_area_init(p_docu_area);

    p_docu_area->tl.slr = *p_slr;
    p_docu_area->br.slr.col = p_slr->col + 1;
    p_docu_area->br.slr.row = p_slr->row + 1;

    return(p_docu_area);
}

/******************************************************************************
*
* is docu_area_2 inside docu_area_1 ?
*
******************************************************************************/

_Check_return_
extern BOOL
docu_area_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area_1,
    _InRef_     PC_DOCU_AREA p_docu_area_2)
{
    if(!p_docu_area_1->whole_row)
    {
        if(p_docu_area_2->whole_row)
            return(FALSE);
        if(p_docu_area_2->tl.slr.col < p_docu_area_1->tl.slr.col ||
           p_docu_area_2->br.slr.col > p_docu_area_1->br.slr.col)
            return(FALSE);
    }

    if(!p_docu_area_1->whole_col)
    {
        if(p_docu_area_2->whole_col)
            return(FALSE);
        if(p_docu_area_2->tl.slr.row < p_docu_area_1->tl.slr.row ||
           p_docu_area_2->br.slr.row > p_docu_area_1->br.slr.row)
            return(FALSE);
    }

    if(!p_docu_area_1->whole_col && !p_docu_area_1->whole_row)
    {
        if( slr_equal(&p_docu_area_1->tl.slr, &p_docu_area_2->tl.slr) &&
            (OBJECT_ID_NONE != p_docu_area_1->tl.object_position.object_id) )
        {
#if 1
            if(object_position_compare(&p_docu_area_2->tl.object_position,
                                       &p_docu_area_1->tl.object_position,
                                       OBJECT_POSITION_COMPARE_PP) < 0)
#else
            S32 area_2_data = (OBJECT_ID_NONE == p_docu_area_2->tl.object_position.object_id)
                                ? 0
                                : p_docu_area_2->tl.object_position.data;

            if(area_2_data  < p_docu_area_1->tl.object_position.data)
#endif
                return(FALSE);
        }

        if( slr_equal(&p_docu_area_1->br.slr, &p_docu_area_2->br.slr) &&
            (OBJECT_ID_NONE != p_docu_area_1->br.object_position.object_id) )
        {
            if( (OBJECT_ID_NONE == p_docu_area_2->br.object_position.object_id)
                ||
#if 1
                (object_position_compare(&p_docu_area_2->br.object_position,
                                         &p_docu_area_1->br.object_position,
                                         OBJECT_POSITION_COMPARE_PP) > 0) )
#else
                p_docu_area_2->br.object_position.data > p_docu_area_1->br.object_position.data)
#endif
                return(FALSE);
        }
    }

    return(TRUE);
}

/******************************************************************************
*
* do two docu_areas intersect ?
*
******************************************************************************/

_Check_return_
extern BOOL /* do they intersect ? */
docu_area_intersect_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area_1,
    _InRef_     PC_DOCU_AREA p_docu_area_2)
{
    if(!(p_docu_area_1->whole_row || p_docu_area_2->whole_row))
        if(p_docu_area_1->tl.slr.col >= p_docu_area_2->br.slr.col ||
           p_docu_area_1->br.slr.col <= p_docu_area_2->tl.slr.col)
            return(FALSE);

    if(!(p_docu_area_1->whole_col || p_docu_area_2->whole_col))
        if(p_docu_area_1->tl.slr.row >= p_docu_area_2->br.slr.row ||
           p_docu_area_1->br.slr.row <= p_docu_area_2->tl.slr.row)
            return(FALSE);

    if(!(p_docu_area_1->whole_col || p_docu_area_1->whole_row) &&
       !(p_docu_area_2->whole_col || p_docu_area_2->whole_row))
    {
        if(slr_last_in_docu_area(p_docu_area_1, &p_docu_area_2->tl.slr) &&
           (OBJECT_ID_NONE != p_docu_area_1->br.object_position.object_id) &&
           (OBJECT_ID_NONE != p_docu_area_2->tl.object_position.object_id) &&
#if 1
           object_position_compare(&p_docu_area_1->br.object_position,
                                   &p_docu_area_2->tl.object_position,
                                   OBJECT_POSITION_COMPARE_PP) <= 0)
#else
           p_docu_area_1->br.object_position.data <= p_docu_area_2->tl.object_position.data)
#endif
             return(FALSE);

        if(slr_last_in_docu_area(p_docu_area_2, &p_docu_area_1->tl.slr) &&
           (OBJECT_ID_NONE != p_docu_area_1->tl.object_position.object_id) &&
           (OBJECT_ID_NONE != p_docu_area_2->br.object_position.object_id) &&
#if 1
           object_position_compare(&p_docu_area_1->tl.object_position,
                                   &p_docu_area_2->br.object_position,
                                   OBJECT_POSITION_COMPARE_PP) >= 0)
#else
           p_docu_area_1->tl.object_position.data >= p_docu_area_2->br.object_position.data)
#endif
             return(FALSE);
    }

    return(TRUE);
}

/******************************************************************************
*
* return the intersection between two docu_areas
*
******************************************************************************/

/*ncr*/
extern BOOL /* do they intersect ? */
docu_area_intersect_docu_area_out(
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_1,
    _InRef_     PC_DOCU_AREA p_docu_area_2)
{
    p_docu_area_out->whole_row = 0;
    p_docu_area_out->whole_col = 0;

    p_docu_area_out->tl.object_position.object_id =OBJECT_ID_NONE;
    p_docu_area_out->br.object_position.object_id = OBJECT_ID_NONE;

    if(p_docu_area_1->whole_row && p_docu_area_2->whole_row)
        p_docu_area_out->whole_row = 1;
    else if(!p_docu_area_1->whole_row && !p_docu_area_2->whole_row)
    {
        p_docu_area_out->tl.slr.col = MAX(p_docu_area_1->tl.slr.col, p_docu_area_2->tl.slr.col);
        p_docu_area_out->br.slr.col = MIN(p_docu_area_1->br.slr.col, p_docu_area_2->br.slr.col);
    }
    else if(!p_docu_area_1->whole_row)
    {
        p_docu_area_out->tl.slr.col = p_docu_area_1->tl.slr.col;
        p_docu_area_out->br.slr.col = p_docu_area_1->br.slr.col;
    }
    else
    {
        p_docu_area_out->tl.slr.col = p_docu_area_2->tl.slr.col;
        p_docu_area_out->br.slr.col = p_docu_area_2->br.slr.col;
    }

    if(p_docu_area_1->whole_col && p_docu_area_2->whole_col)
        p_docu_area_out->whole_col = 1;
    else if(!p_docu_area_1->whole_col && !p_docu_area_2->whole_col)
    {
        p_docu_area_out->tl.slr.row = MAX(p_docu_area_1->tl.slr.row, p_docu_area_2->tl.slr.row);
        p_docu_area_out->br.slr.row = MIN(p_docu_area_1->br.slr.row, p_docu_area_2->br.slr.row);
    }
    else if(!p_docu_area_1->whole_col)
    {
        p_docu_area_out->tl.slr.row = p_docu_area_1->tl.slr.row;
        p_docu_area_out->br.slr.row = p_docu_area_1->br.slr.row;
    }
    else
    {
        p_docu_area_out->tl.slr.row = p_docu_area_2->tl.slr.row;
        p_docu_area_out->br.slr.row = p_docu_area_2->br.slr.row;
    }

    if(!p_docu_area_out->whole_col && !p_docu_area_out->whole_row)
    {
        if(slr_equal(&p_docu_area_out->tl.slr, &p_docu_area_1->tl.slr))
            p_docu_area_out->tl.object_position = p_docu_area_1->tl.object_position;
        if(slr_equal(&p_docu_area_out->br.slr, &p_docu_area_1->br.slr))
            p_docu_area_out->br.object_position = p_docu_area_1->br.object_position;

        if(slr_equal(&p_docu_area_out->tl.slr, &p_docu_area_2->tl.slr))
        {
            if(OBJECT_ID_NONE == p_docu_area_out->tl.object_position.object_id)
                p_docu_area_out->tl.object_position = p_docu_area_2->tl.object_position;
            else if(OBJECT_ID_NONE != p_docu_area_2->tl.object_position.object_id)
#if 1
                object_position_max(&p_docu_area_out->tl.object_position,
                                    &p_docu_area_out->tl.object_position,
                                    &p_docu_area_2->tl.object_position);
#else
                p_docu_area_out->tl.object_position.data = MAX(p_docu_area_out->tl.object_position.data,
                                                               p_docu_area_2->tl.object_position.data);
#endif
            if((OBJECT_ID_NONE != p_docu_area_out->tl.object_position.object_id) &&
#if 1
               object_position_at_start(&p_docu_area_out->tl.object_position))
#else
               p_docu_area_out->tl.object_position.data == 0)
#endif
                p_docu_area_out->tl.object_position.object_id = OBJECT_ID_NONE;
        }
        if(slr_equal(&p_docu_area_out->br.slr, &p_docu_area_2->br.slr))
        {
            if(OBJECT_ID_NONE == p_docu_area_out->br.object_position.object_id)
                p_docu_area_out->br.object_position = p_docu_area_2->br.object_position;
            else if(OBJECT_ID_NONE != p_docu_area_2->br.object_position.object_id)
#if 1
                object_position_min(&p_docu_area_out->br.object_position,
                                    &p_docu_area_out->br.object_position,
                                    &p_docu_area_2->br.object_position);
#else
                p_docu_area_out->br.object_position.data = MIN(p_docu_area_out->br.object_position.data,
                                                               p_docu_area_2->br.object_position.data);
#endif
        }
    }

    docu_area_clean_up(p_docu_area_out);

    return(!docu_area_is_empty(p_docu_area_out));
}

/******************************************************************************
*
* is docu area a fragment ?
*
******************************************************************************/

_Check_return_
extern BOOL
docu_area_is_frag(
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    if( slr_last_in_docu_area(p_docu_area, &p_docu_area->tl.slr) &&
        ((OBJECT_ID_NONE != p_docu_area->tl.object_position.object_id) ||
         (OBJECT_ID_NONE != p_docu_area->br.object_position.object_id)) )
        return(TRUE);

    return(FALSE);
}

/******************************************************************************
*
* is docu area a fragment ?
*
******************************************************************************/

_Check_return_
extern BOOL
docu_area_is_cell_or_less(
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    return(slr_last_in_docu_area(p_docu_area, &p_docu_area->tl.slr));
}

/******************************************************************************
*
* normalise docu_area - i.e. ensure that
* tl and br slrs contain sensible numbers,
* and the whole_col/whole_row bits are cleared
*
******************************************************************************/

extern void
docu_area_normalise(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_in)
{
    *p_docu_area_out = *p_docu_area_in;

    if(p_docu_area_out->whole_row || p_docu_area_out->whole_col)
    {
        p_docu_area_out->tl.object_position.object_id = OBJECT_ID_NONE;
        p_docu_area_out->br.object_position.object_id = OBJECT_ID_NONE;
    }

    if(p_docu_area_out->whole_row)
    {
        p_docu_area_out->whole_row = 0;
        p_docu_area_out->tl.slr.col = 0;
        p_docu_area_out->br.slr.col = n_cols_logical(p_docu);
    }

    if(p_docu_area_out->whole_col)
    {
        p_docu_area_out->whole_col = 0;
        p_docu_area_out->tl.slr.row = 0;
        p_docu_area_out->br.slr.row = n_rows(p_docu);
    }
}

/******************************************************************************
*
* normalise docu_area - i.e. ensure that
* tl and br slrs contain sensible numbers,
* and the whole_col/whole_row bits are cleared
*
******************************************************************************/

extern void
docu_area_normalise_phys(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_in)
{
    *p_docu_area_out = *p_docu_area_in;

    if(p_docu_area_out->whole_row || p_docu_area_out->whole_col)
    {
        SLR size;

        p_docu_area_out->tl.object_position.object_id = OBJECT_ID_NONE;
        p_docu_area_out->br.object_position.object_id = OBJECT_ID_NONE;

        find_object_extent(p_docu, &size);

        if(p_docu_area_out->whole_row)
        {
            COL cols = n_cols_physical(p_docu);
            p_docu_area_out->whole_row = 0;
            p_docu_area_out->tl.slr.col = 0;
            p_docu_area_out->br.slr.col = MAX(cols, size.col);
        }

        if(p_docu_area_out->whole_col)
        {
            ROW rows = n_rows(p_docu);
            p_docu_area_out->whole_col = 0;
            p_docu_area_out->tl.slr.row = 0;
            p_docu_area_out->br.slr.row = MAX(rows, size.row);
        }
    }
}

/******************************************************************************
*
* adjust a docu_area by the difference
* between two positions
*
******************************************************************************/

extern void
docu_area_offset_to_position(
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position_now,
    _InRef_     PC_POSITION p_position_was)
{
    /* all fields default */
    *p_docu_area_out = *p_docu_area;

    /**************** top left *****************/
    if(!p_docu_area->whole_row)
    {
        p_docu_area_out->tl.slr.col = p_docu_area->tl.slr.col + p_position_now->slr.col - p_position_was->slr.col;
        p_docu_area_out->tl.slr.col = MAX(0, p_docu_area_out->tl.slr.col);
    }
    if(!p_docu_area->whole_col)
    {
        p_docu_area_out->tl.slr.row = p_docu_area->tl.slr.row + p_position_now->slr.row - p_position_was->slr.row;
        p_docu_area_out->tl.slr.row = MAX(0, p_docu_area_out->tl.slr.row);
    }

    if(!(p_docu_area->whole_row || p_docu_area->whole_col))
    {
        S32 data_now = (OBJECT_ID_NONE == p_position_now->object_position.object_id) ? 0 : p_position_now->object_position.data;
        S32 data_was = (OBJECT_ID_NONE == p_position_was->object_position.object_id) ? 0 : p_position_was->object_position.data;

        p_docu_area_out->tl.object_position.data = p_docu_area->tl.object_position.data + data_now - data_was;
        p_docu_area_out->tl.object_position.data = MAX(p_docu_area_out->tl.object_position.data, 0);
    }
    else
        p_docu_area_out->tl.object_position.data = 0;

    /**************** bottom right *****************/
    if(!p_docu_area->whole_row)
    {
        p_docu_area_out->br.slr.col = p_docu_area->br.slr.col + p_position_now->slr.col - p_position_was->slr.col;
        p_docu_area_out->br.slr.col = MAX(0, p_docu_area_out->br.slr.col);
    }
    if(!p_docu_area->whole_col)
    {
        p_docu_area_out->br.slr.row = p_docu_area->br.slr.row + p_position_now->slr.row - p_position_was->slr.row;
        p_docu_area_out->br.slr.row = MAX(0, p_docu_area_out->br.slr.row);
    }

    /* trailing position can be offset only if it is the end of a single fragment */
    if(!(p_docu_area->whole_row || p_docu_area->whole_col)
       &&
       (OBJECT_ID_NONE != p_docu_area->br.object_position.object_id)
       &&
       slr_last_in_docu_area(p_docu_area, &p_docu_area->tl.slr))
    {
        p_docu_area_out->br.object_position.data = p_docu_area->br.object_position.data +
                                                   p_docu_area_out->tl.object_position.data - p_docu_area->tl.object_position.data;
        p_docu_area_out->br.object_position.data = MAX(p_docu_area_out->br.object_position.data, 0);
    }

    docu_area_clean_up(p_docu_area_out);
}

/******************************************************************************
*
* work out the union of two docu_areas
*
******************************************************************************/

extern void
docu_area_union_docu_area_out(
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_1,
    _InRef_     PC_DOCU_AREA p_docu_area_2)
{
    p_docu_area_out->whole_col = p_docu_area_1->whole_col | p_docu_area_2->whole_col;
    p_docu_area_out->whole_row = p_docu_area_1->whole_row | p_docu_area_2->whole_row;

    if(!p_docu_area_out->whole_col)
    {
        p_docu_area_out->tl.slr.row = MIN(p_docu_area_1->tl.slr.row, p_docu_area_2->tl.slr.row);
        p_docu_area_out->br.slr.row = MAX(p_docu_area_1->br.slr.row, p_docu_area_2->br.slr.row);
    }

    if(!p_docu_area_out->whole_row)
    {
        p_docu_area_out->tl.slr.col = MIN(p_docu_area_1->tl.slr.col, p_docu_area_2->tl.slr.col);
        p_docu_area_out->br.slr.col = MAX(p_docu_area_1->br.slr.col, p_docu_area_2->br.slr.col);
    }

    if(!p_docu_area_out->whole_col || !p_docu_area_out->whole_row)
    {
        p_docu_area_out->tl.object_position.object_id = OBJECT_ID_NONE;
        p_docu_area_out->br.object_position.object_id = OBJECT_ID_NONE;

        if(slr_equal(&p_docu_area_1->tl.slr, &p_docu_area_out->tl.slr))
            p_docu_area_out->tl.object_position = p_docu_area_1->tl.object_position;

        if(slr_equal(&p_docu_area_2->tl.slr, &p_docu_area_out->tl.slr)
           &&
           (OBJECT_ID_NONE != p_docu_area_out->tl.object_position.object_id))
        {
            if(OBJECT_ID_NONE != p_docu_area_2->tl.object_position.object_id)
#if 1
                object_position_min(&p_docu_area_out->tl.object_position,
                                    &p_docu_area_out->tl.object_position,
                                    &p_docu_area_2->tl.object_position);
#else
                p_docu_area_out->tl.object_position.data = MIN(p_docu_area_out->tl.object_position.data,
                                                               p_docu_area_2->tl.object_position.data);
#endif
            else
                p_docu_area_out->tl.object_position = p_docu_area_2->tl.object_position;
        }

        if(slr_equal(&p_docu_area_1->br.slr, &p_docu_area_out->br.slr))
            p_docu_area_out->br.object_position = p_docu_area_1->br.object_position;

        if(slr_equal(&p_docu_area_2->br.slr, &p_docu_area_out->br.slr)
           &&
           (OBJECT_ID_NONE != p_docu_area_out->br.object_position.object_id))
        {
            if(OBJECT_ID_NONE != p_docu_area_2->br.object_position.object_id)
#if 1
                object_position_max(&p_docu_area_out->br.object_position,
                                    &p_docu_area_out->br.object_position,
                                    &p_docu_area_2->br.object_position);
#else
                p_docu_area_out->br.object_position.data = MAX(p_docu_area_out->br.object_position.data,
                                                               p_docu_area_2->br.object_position.data);
#endif
            else
                p_docu_area_out->br.object_position = p_docu_area_2->br.object_position;
        }
    }
}

/******************************************************************************
*
* extract col and row limits from a docu_area
*
******************************************************************************/

extern void
limits_from_docu_area(
    _DocuRef_   P_DOCU p_docu,
    _Out_opt_   P_COL p_col_s,
    _Out_opt_   P_COL p_col_e,
    _Out_opt_   P_ROW p_row_s,
    _Out_opt_   P_ROW p_row_e,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    if(NULL != p_col_s)
        *p_col_s = p_docu_area->whole_row ? 0 : p_docu_area->tl.slr.col;

    if(NULL != p_col_e)
    {
        COL cols = n_cols_logical(p_docu);
        *p_col_e = p_docu_area->whole_row ? cols : MIN(p_docu_area->br.slr.col, cols);
    }

    if(NULL != p_row_s)
        *p_row_s = p_docu_area->whole_col ? 0 : p_docu_area->tl.slr.row;

    if(NULL != p_row_e)
    {
        ROW rows = n_rows(p_docu);
        *p_row_e = p_docu_area->whole_col ? rows : MIN(p_docu_area->br.slr.row, rows);
    }
}

/******************************************************************************
*
* extract col and row limits from a region
*
******************************************************************************/

extern void
limits_from_region(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_COL p_col_s,
    _OutRef_    P_COL p_col_e,
    _OutRef_    P_ROW p_row_s,
    _OutRef_    P_ROW p_row_e,
    _InRef_     PC_REGION p_region)
{
    *p_col_s = p_region->whole_row ? 0 : p_region->tl.col;

    {
    COL cols = n_cols_logical(p_docu);
    *p_col_e = p_region->whole_row ? cols : MIN(p_region->br.col, cols);
    } /*block*/

    *p_row_s = p_region->whole_col ? 0 : p_region->tl.row;

    {
    ROW rows = n_rows(p_docu);
    *p_row_e = p_region->whole_col ? rows : MIN(p_region->br.row, rows);
    } /*block*/
}

_Check_return_
static BOOL
rec_object_position_at_start(
    _InRef_     PC_OBJECT_POSITION p_object_position)
{
    OBJECT_POSITION_COMPARE object_position_compare;
    object_position_compare.first = *p_object_position;
    object_position_compare.second.object_id = OBJECT_ID_NONE;

    if(status_ok(object_call_id(OBJECT_ID_REC, P_DOCU_NONE, T5_MSG_OBJECT_POSITION_AT_START, &object_position_compare)))
        return((BOOL) object_position_compare.resultant_order);

    return(FALSE);
}

_Check_return_
extern BOOL
object_position_at_start(
    _InRef_     PC_OBJECT_POSITION p_object_position)
{
    switch(p_object_position->object_id)
    {
    default:
        return(0 == p_object_position->data);

    case OBJECT_ID_NONE:
        return(TRUE);

    case OBJECT_ID_REC:
        return(rec_object_position_at_start(p_object_position));
    }
}

static void
rec_object_position_set_start(
    _OutRef_    P_OBJECT_POSITION p_object_position)
{
    status_consume(object_call_id(OBJECT_ID_REC, P_DOCU_NONE, T5_MSG_OBJECT_POSITION_SET_START, p_object_position));
}

extern void
object_position_set_start(
    _OutRef_    P_OBJECT_POSITION p_object_position,
    _InVal_     OBJECT_ID object_id)
{
    p_object_position->data = 0;
    p_object_position->more_data = -1; /* like object_position_init */
    p_object_position->object_id = object_id;

    switch(object_id)
    {
    default:
        break;

    case OBJECT_ID_REC:
        rec_object_position_set_start(p_object_position);
        break;
    }
}

/******************************************************************************
*
* compare the data portions of two object positions, assuming they're inline offsets
*
******************************************************************************/

_Check_return_
static inline S32 /* strcmp like result */
object_position_compare_data_ignore_id(
    _InRef_     PC_OBJECT_POSITION p_object_position_s,
    _InRef_     PC_OBJECT_POSITION p_object_position_e,
    _InVal_     BOOL end_point)
{
    S32 data_s;
    S32 data_e;

    switch(p_object_position_s->object_id)
    {
    case OBJECT_ID_REC:
        /* Er what ? */
    case OBJECT_ID_NONE:
        data_s = 0;
        break;

    default:
        data_s = p_object_position_s->data;
        break;
    }

    switch(p_object_position_e->object_id)
    {
    case OBJECT_ID_REC:
        /* Er what ? */
    case OBJECT_ID_NONE:
        data_e = end_point ? S32_MAX : 0;
        break;

    default:
        data_e = p_object_position_e->data;
        break;
    }

    if(data_s > data_e)
        return(1);

    if(data_s < data_e)
        return(-1);

    return(0);
}

_Check_return_
static S32 /* strcmp like result */
rec_object_position_compare(
    _InRef_     PC_OBJECT_POSITION p_object_position_s,
    _InRef_     PC_OBJECT_POSITION p_object_position_e,
    _InVal_     BOOL end_point)
{
    if(p_object_position_s->object_id == p_object_position_e->object_id)
    {
        OBJECT_POSITION_COMPARE object_position_compare;
        object_position_compare.first  = *p_object_position_s;
        object_position_compare.second = *p_object_position_e;
        if(status_ok(object_call_id(OBJECT_ID_REC, P_DOCU_NONE, T5_MSG_COMPARE_OBJECT_POSITIONS, &object_position_compare)))
            return(object_position_compare.resultant_order);

        return(0);
    }

    return(object_position_compare_data_ignore_id(p_object_position_s, p_object_position_e, end_point));
}

_Check_return_
extern S32 /* strcmp like result */
object_position_compare(
    _InRef_     PC_OBJECT_POSITION p_object_position_s,
    _InRef_     PC_OBJECT_POSITION p_object_position_e,
    _InVal_     BOOL end_point)
{
    switch(p_object_position_s->object_id)
    {
    default:
        return(object_position_compare_data_ignore_id(p_object_position_s, p_object_position_e, end_point));

    case OBJECT_ID_REC:
        return(rec_object_position_compare(p_object_position_s, p_object_position_e, end_point));
    }
}

extern void
object_position_max(
    _OutRef_    P_OBJECT_POSITION p_object_position_out,
    _InRef_     PC_OBJECT_POSITION p_object_position_1,
    _InRef_     PC_OBJECT_POSITION p_object_position_2)
{
    if(object_position_compare(p_object_position_1, p_object_position_2, OBJECT_POSITION_COMPARE_PP) >= 0)
        *p_object_position_out = *p_object_position_1;
    else
        *p_object_position_out = *p_object_position_2;
}

extern void
object_position_min(
    _OutRef_    P_OBJECT_POSITION p_object_position_out,
    _InRef_     PC_OBJECT_POSITION p_object_position_1,
    _InRef_     PC_OBJECT_POSITION p_object_position_2)
{
    if(object_position_compare(p_object_position_1, p_object_position_2, OBJECT_POSITION_COMPARE_PP) < 0)
        *p_object_position_out = *p_object_position_1;
    else
        *p_object_position_out = *p_object_position_2;
}

/******************************************************************************
*
* calculate offsets from object data
*
******************************************************************************/

extern void
offsets_from_object_data(
    _OutRef_    P_S32 p_start,
    _OutRef_    P_S32 p_end,
    _InRef_     PC_OBJECT_DATA p_object_data,
    _InVal_     S32 len)
{
    S32 start = ((OBJECT_ID_NONE != p_object_data->object_position_start.object_id) ? p_object_data->object_position_start.data : 0  );
    S32 end   = ((OBJECT_ID_NONE != p_object_data->object_position_end.object_id  ) ? p_object_data->object_position_end.data   : len);

    profile_ensure_frame();

    assert(len >= 0);
    assert(start >= 0);
    assert(start <= end);

    *p_start = MIN(len, start);
    *p_end   = MIN(len, end);
}

/******************************************************************************
*
* compare two positions; OBJECT_ID_NONE
* is assumed to be at the start of an object
*
******************************************************************************/

_Check_return_
extern S32 /* strcmp like result */
position_compare(
    _InRef_     PC_POSITION p_position_1,
    _InRef_     PC_POSITION p_position_2)
{
    S32 res = slr_compare(&p_position_1->slr, &p_position_2->slr);

    if(0 != res)
        return(res);

    return(object_position_compare(&p_position_1->object_position, &p_position_2->object_position, OBJECT_POSITION_COMPARE_PP));
}

/******************************************************************************
*
* try clearing object positions from a position
*
******************************************************************************/

_Check_return_
extern BOOL /* sub object_position? */
position_clear_object_positions(
    _InoutRef_  P_POSITION p_position,
    _InVal_     S32 end /* position is end-exclusive */)
{
#if 1
    if(object_position_at_start(&p_position->object_position))
    {
        object_position_init(&p_position->object_position);
#else
    if((OBJECT_ID_NONE != p_position->object_position.object_id) && (0 == p_position->object_position.data))
    {
        p_position->object_position.object_id = OBJECT_ID_NONE;
#endif

        if(end)
            p_position->slr.row -= 1;
    }

    return(OBJECT_ID_NONE != p_position->object_position.object_id);
}

/* SKS 18may95 add function to correctly go from DATA_REF to POSITION */

static void
field_or_title_position_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_POSITION p_position,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_OBJECT_POSITION p_object_position)
{
    DATA_REF_AND_POSITION data_ref_and_position;

    data_ref_and_position.data_ref = *p_data_ref;
    data_ref_and_position.position.object_position = *p_object_position;

    status_assert(object_call_id(OBJECT_ID_REC, p_docu, T5_MSG_DATA_REF_TO_POSITION, &data_ref_and_position));

    *p_position = data_ref_and_position.position;
}

extern void
position_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_POSITION p_position,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_OBJECT_POSITION p_object_position)
{
    switch(p_data_ref->data_space)
    {
    default:
        p_position->slr.col = 0;
        p_position->slr.row = 0;
        p_position->object_position = *p_object_position;
        break;

    case DATA_SLOT:
        p_position->slr = p_data_ref->arg.slr;
        p_position->object_position = *p_object_position;
        break;

    case DATA_DB_FIELD:
    case DATA_DB_TITLE:
        field_or_title_position_from_data_ref(p_docu, p_position, p_data_ref, p_object_position);
        break;
    }
}

/******************************************************************************
*
* is the col of the position inside the docu_area ?
*
******************************************************************************/

_Check_return_
extern BOOL
position_col_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position)
{
    if(!p_docu_area->whole_row)
        if((p_position->slr.col >= p_docu_area->br.slr.col) || (p_position->slr.col < p_docu_area->tl.slr.col))
            return(FALSE);

    return(TRUE);
}

/******************************************************************************
*
* is the row of the position inside the docu_area ?
*
******************************************************************************/

_Check_return_
extern BOOL
position_row_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position)
{
    profile_ensure_frame();

    if(!p_docu_area->whole_col)
        if((p_position->slr.row >= p_docu_area->br.slr.row) || (p_position->slr.row < p_docu_area->tl.slr.row))
            return(FALSE);

    return(TRUE);
}

/******************************************************************************
*
* is the slr of the position inside the docu_area ?
*
******************************************************************************/

_Check_return_
extern BOOL
position_slr_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position)
{
    profile_ensure_frame();

    if(!p_docu_area->whole_row)
        if((p_position->slr.col >= p_docu_area->br.slr.col) || (p_position->slr.col < p_docu_area->tl.slr.col))
            return(FALSE);

    if(!p_docu_area->whole_col)
        if((p_position->slr.row >= p_docu_area->br.slr.row) || (p_position->slr.row < p_docu_area->tl.slr.row))
            return(FALSE);

    return(TRUE);
}

/******************************************************************************
*
* is the position inside the docu_area ?
*
******************************************************************************/

_Check_return_
extern BOOL
position_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position)
{
    if(!p_docu_area->whole_row)
        if((p_position->slr.col >= p_docu_area->br.slr.col) || (p_position->slr.col < p_docu_area->tl.slr.col))
            return(FALSE);

    if(!p_docu_area->whole_col)
        if((p_position->slr.row >= p_docu_area->br.slr.row) || (p_position->slr.row < p_docu_area->tl.slr.row))
            return(FALSE);

    if(OBJECT_ID_NONE != p_docu_area->tl.object_position.object_id)
        if(slr_equal(&p_docu_area->tl.slr, &p_position->slr) &&
#if 1
           object_position_compare(&p_docu_area->tl.object_position,
                                   &p_position->object_position,
                                   OBJECT_POSITION_COMPARE_PP) > 0)
#else
           p_docu_area->tl.object_position.data > ((OBJECT_ID_NONE == p_position->object_position.object_id)
                                                   ? 0
                                                   : p_position->object_position.data))
#endif
            return(FALSE);

    if(OBJECT_ID_NONE != p_docu_area->br.object_position.object_id)
        if(slr_equal_end(&p_docu_area->br.slr, &p_position->slr) &&
           (OBJECT_ID_NONE != p_position->object_position.object_id) &&
#if 1
           object_position_compare(&p_docu_area->br.object_position,
                                   &p_position->object_position,
                                   OBJECT_POSITION_COMPARE_PP) <= 0)
#else
           p_docu_area->br.object_position.data <= p_position->object_position.data)
#endif
            return(FALSE);

    return(TRUE);
}

/******************************************************************************
*
* is the region empty ?
*
******************************************************************************/

_Check_return_
extern BOOL
region_empty(
    _InRef_     PC_REGION p_region)
{
    profile_ensure_frame();

    if(!p_region->whole_row)
        if(p_region->tl.col >= p_region->br.col)
            return(TRUE);

    if(!p_region->whole_col)
        if(p_region->tl.row >= p_region->br.row)
            return(TRUE);

    return(FALSE);
}

/******************************************************************************
*
* do the regions cover the same area ?
*
******************************************************************************/

#if defined(UNUSED_KEEP_ALIVE)

_Check_return_
extern BOOL
region_equal(
    _InRef_     PC_REGION p_region_1,
    _InRef_     PC_REGION p_region_2)
{
    if(p_region_1->whole_row != p_region_2->whole_row)
        return(FALSE);
    if(!p_region_1->whole_row &&
       (p_region_1->tl.col != p_region_2->tl.col ||
        p_region_1->br.col != p_region_2->br.col))
        return(FALSE);

    if(p_region_1->whole_col != p_region_2->whole_col)
        return(FALSE);
    if(!p_region_1->whole_col &&
       (p_region_1->tl.row != p_region_2->tl.row ||
        p_region_1->br.row != p_region_2->br.row))
        return(FALSE);

    return(TRUE);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* make a region from a docu_area
*
******************************************************************************/

/*ncr*/
extern BOOL /* TRUE == not_empty */
region_from_docu_area_min(
    _OutRef_    P_REGION p_region,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    p_region->tl = p_docu_area->tl.slr;
    p_region->br = p_docu_area->br.slr;

    p_region->whole_col = p_docu_area->whole_col;
    p_region->whole_row = p_docu_area->whole_row;

    if(docu_area_frag_start(p_docu_area))
        p_region->tl.row += 1;
    if(docu_area_frag_end(p_docu_area))
        p_region->br.row -= 1;

    return(!region_empty(p_region));
}

/******************************************************************************
*
* make a region from a docu_area
*
******************************************************************************/

/*ncr*/
_Ret_valid_
extern P_REGION
region_from_docu_area_max(
    _OutRef_    P_REGION p_region,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    profile_ensure_frame();

    p_region->tl = p_docu_area->tl.slr;
    p_region->br = p_docu_area->br.slr;

    p_region->whole_col = p_docu_area->whole_col;
    p_region->whole_row = p_docu_area->whole_row;

    return(p_region);
}

/******************************************************************************
*
* extract region from DOCU_REFORMAT
*
******************************************************************************/

/*ncr*/
_Ret_maybenull_ _Success_(NULL != return)
extern P_REGION
region_from_docu_reformat(
    _OutRef_    P_REGION p_region,
    _InRef_     PC_DOCU_REFORMAT p_docu_reformat)
{
    profile_ensure_frame();

    if(p_docu_reformat->data_space != DATA_SLOT)
        return(NULL);

    switch(p_docu_reformat->data_type)
    {
    default: default_unhandled();
#if CHECKING
    case REFORMAT_ALL:
#endif
        p_region->whole_col = 1;
        p_region->whole_row = 1;
        break;

    case REFORMAT_SLR:
        region_from_two_slrs(p_region, &p_docu_reformat->data.slr, &p_docu_reformat->data.slr, TRUE);
        break;

    case REFORMAT_DOCU_AREA:
        region_from_docu_area_max(p_region, &p_docu_reformat->data.docu_area);
        break;

    case REFORMAT_ROW:
        p_region->tl.row = p_docu_reformat->data.row;
        p_region->br.row = MAX_ROW;
        p_region->whole_col = 0;
        p_region->whole_row = 1;
        break;
    }

    return(p_region);
}

/******************************************************************************
*
* make a region from two SLRs
*
******************************************************************************/

/*ncr*/
_Ret_valid_
extern P_REGION
region_from_two_slrs(
    _OutRef_    P_REGION p_region,
    _InRef_     PC_SLR p_slr_1,
    _InRef_     PC_SLR p_slr_2,
    _InVal_     BOOL add_one_to_br)
{
    profile_ensure_frame();

    p_region->tl = *p_slr_1;
    p_region->br = *p_slr_2;

    if(add_one_to_br)
    {
        p_region->br.col += 1;
        p_region->br.row += 1;
    }

    p_region->whole_col = 0;
    p_region->whole_row = 0;

    return(p_region);
}

/******************************************************************************
*
* is region2 contained by region1 ?
*
******************************************************************************/

_Check_return_
extern BOOL
region_in_region(
    _InRef_     PC_REGION p_region_1,
    _InRef_     PC_REGION p_region_2)
{
    profile_ensure_frame();

    if(!p_region_1->whole_row)
    {
        if(p_region_2->whole_row)
            return(FALSE);
        if(p_region_2->tl.col < p_region_1->tl.col ||
           p_region_2->br.col > p_region_1->br.col)
            return(FALSE);
    }

    if(!p_region_1->whole_col)
    {
        if(p_region_2->whole_col)
            return(FALSE);
        if(p_region_2->tl.row < p_region_1->tl.row ||
           p_region_2->br.row > p_region_1->br.row)
            return(FALSE);
    }

    return(TRUE);
}

/******************************************************************************
*
* return the intersection between
* a region and a docu_area
*
******************************************************************************/

_Check_return_
extern BOOL /* do they intersect ? */
region_intersect_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_REGION p_region)
{
    DOCU_AREA docu_area;

    return(docu_area_intersect_docu_area(p_docu_area, docu_area_from_region(&docu_area, p_region)));
}

/******************************************************************************
*
* do the two regions intersect ?
*
******************************************************************************/

_Check_return_
extern BOOL
region_intersect_region(
    _InRef_     PC_REGION p_region_1,
    _InRef_     PC_REGION p_region_2)
{
    profile_ensure_frame();

    if(!(p_region_1->whole_row || p_region_2->whole_row))
        if(p_region_1->tl.col >= p_region_2->br.col ||
           p_region_1->br.col <= p_region_2->tl.col)
            return(FALSE);

    if(!(p_region_1->whole_col || p_region_2->whole_col))
        if(p_region_1->tl.row >= p_region_2->br.row ||
           p_region_1->br.row <= p_region_2->tl.row)
            return(FALSE);

    return(TRUE);
}

/******************************************************************************
*
* calculate intersection of two regions
*
******************************************************************************/

_Check_return_
extern BOOL
region_intersect_region_out(
    _OutRef_    P_REGION p_region_out,
    _InRef_     PC_REGION p_region_1,
    _InRef_     PC_REGION p_region_2)
{
    p_region_out->whole_row = p_region_out->whole_col = 0;

    if(p_region_1->whole_row && p_region_2->whole_row)
        p_region_out->whole_row = 1;
    else if(!p_region_1->whole_row && !p_region_2->whole_row)
    {
        p_region_out->tl.col = MAX(p_region_1->tl.col, p_region_2->tl.col);
        p_region_out->br.col = MIN(p_region_1->br.col, p_region_2->br.col);
    }
    else if(!p_region_1->whole_row)
    {
        p_region_out->tl.col = p_region_1->tl.col;
        p_region_out->br.col = p_region_1->br.col;
    }
    else
    {
        p_region_out->tl.col = p_region_2->tl.col;
        p_region_out->br.col = p_region_2->br.col;
    }

    if(p_region_1->whole_col && p_region_2->whole_col)
        p_region_out->whole_col = 1;
    else if(!p_region_1->whole_col && !p_region_2->whole_col)
    {
        p_region_out->tl.row = MAX(p_region_1->tl.row, p_region_2->tl.row);
        p_region_out->br.row = MIN(p_region_1->br.row, p_region_2->br.row);
    }
    else if(!p_region_1->whole_col)
    {
        p_region_out->tl.row = p_region_1->tl.row;
        p_region_out->br.row = p_region_1->br.row;
    }
    else
    {
        p_region_out->tl.row = p_region_2->tl.row;
        p_region_out->br.row = p_region_2->br.row;
    }

    return(!region_empty(p_region_out));
}

/******************************************************************************
*
* work out whether region2 spans the whole of
* region1's columns or rows
*
******************************************************************************/

extern void
region_span(
    _OutRef_    P_BOOL p_colspan,
    _OutRef_    P_BOOL p_rowspan,
    _InRef_     PC_REGION p_region1,
    _InRef_     PC_REGION p_region2)
{
    profile_ensure_frame();

    *p_colspan = *p_rowspan = 0;

    /* does the moved area span all the rows in the range ? */
    if(p_region2->whole_col)
        *p_rowspan = 1;
    else if(p_region1->whole_col)
    {
        if(p_region2->tl.row == 0 && p_region2->br.row >= MAX_ROW - 1)
            *p_rowspan = 1;
    }
    else if(p_region2->tl.row <= p_region1->tl.row &&
            p_region2->br.row >= p_region1->br.row)
        *p_rowspan = 1;

    /* does the moved area span all the cols in the range ? */
    if(p_region2->whole_row)
        *p_colspan = 1;
    else if(p_region1->whole_row)
    {
        if(p_region2->tl.col == 0 && p_region2->br.col >= MAX_COL - 1)
            *p_colspan = 1;
    }
    else if(p_region2->tl.col <= p_region1->tl.col &&
            p_region2->br.col >= p_region1->br.col)
        *p_colspan = 1;
}

/******************************************************************************
*
* make a skel_rect empty
*
******************************************************************************/

extern void
skel_rect_empty_set(
    _OutRef_    P_SKEL_RECT p_skel_rect)
{
    profile_ensure_frame();

    p_skel_rect->tl.pixit_point.x = p_skel_rect->tl.pixit_point.y = S32_MAX;
    p_skel_rect->tl.page_num.x = p_skel_rect->tl.page_num.y = MAX_PAGE;

    p_skel_rect->br.pixit_point.x = p_skel_rect->br.pixit_point.y = 0;
    p_skel_rect->br.page_num.x = p_skel_rect->br.page_num.y = 0;
}

/******************************************************************************
*
* is this skeleton rectangle empty ?
*
******************************************************************************/

_Check_return_
extern BOOL
skel_rect_empty(
    _InRef_     PC_SKEL_RECT p_skel_rect)
{
    if(skel_point_compare(&p_skel_rect->tl, &p_skel_rect->br, x) >= 0 ||
       skel_point_compare(&p_skel_rect->tl, &p_skel_rect->br, y) >= 0)
        return(TRUE);

    return(FALSE);
}

/******************************************************************************
*
* do two skeleton rectangles intersect?
*
******************************************************************************/

/*ncr*/
extern BOOL
skel_rect_intersect(
    _Out_opt_   P_SKEL_RECT p_intersect,
    _InRef_     PC_SKEL_RECT p_skel_rect1,
    _InRef_     PC_SKEL_RECT p_skel_rect2)
{
    SKEL_RECT skel_rect;

    skel_rect.tl.page_num.x = MAX(p_skel_rect1->tl.page_num.x, p_skel_rect2->tl.page_num.x);
    if(p_skel_rect1->tl.page_num.x == p_skel_rect2->tl.page_num.x)
        skel_rect.tl.pixit_point.x = MAX(p_skel_rect1->tl.pixit_point.x, p_skel_rect2->tl.pixit_point.x);
    else if(p_skel_rect1->tl.page_num.x > p_skel_rect2->tl.page_num.x)
        skel_rect.tl.pixit_point.x = p_skel_rect1->tl.pixit_point.x;
    else
        skel_rect.tl.pixit_point.x = p_skel_rect2->tl.pixit_point.x;
    skel_rect.tl.page_num.x = MAX(p_skel_rect1->tl.page_num.x, p_skel_rect2->tl.page_num.x);

    skel_rect.tl.page_num.y = MAX(p_skel_rect1->tl.page_num.y, p_skel_rect2->tl.page_num.y);
    if(p_skel_rect1->tl.page_num.y == p_skel_rect2->tl.page_num.y)
        skel_rect.tl.pixit_point.y = MAX(p_skel_rect1->tl.pixit_point.y, p_skel_rect2->tl.pixit_point.y);
    else if(p_skel_rect1->tl.page_num.y > p_skel_rect2->tl.page_num.y)
        skel_rect.tl.pixit_point.y = p_skel_rect1->tl.pixit_point.y;
    else
        skel_rect.tl.pixit_point.y = p_skel_rect2->tl.pixit_point.y;
    skel_rect.tl.page_num.y = MAX(p_skel_rect1->tl.page_num.y, p_skel_rect2->tl.page_num.y);

    skel_rect.br.page_num.x = MIN(p_skel_rect1->br.page_num.x, p_skel_rect2->br.page_num.x);
    if(p_skel_rect1->br.page_num.x == p_skel_rect2->br.page_num.x)
        skel_rect.br.pixit_point.x = MIN(p_skel_rect1->br.pixit_point.x, p_skel_rect2->br.pixit_point.x);
    else if(p_skel_rect1->br.page_num.x < p_skel_rect2->br.page_num.x)
        skel_rect.br.pixit_point.x = p_skel_rect1->br.pixit_point.x;
    else
        skel_rect.br.pixit_point.x = p_skel_rect2->br.pixit_point.x;
    skel_rect.br.page_num.x = MIN(p_skel_rect1->br.page_num.x, p_skel_rect2->br.page_num.x);

    skel_rect.br.page_num.y = MIN(p_skel_rect1->br.page_num.y, p_skel_rect2->br.page_num.y);
    if(p_skel_rect1->br.page_num.y == p_skel_rect2->br.page_num.y)
        skel_rect.br.pixit_point.y = MIN(p_skel_rect1->br.pixit_point.y, p_skel_rect2->br.pixit_point.y);
    else if(p_skel_rect1->br.page_num.y < p_skel_rect2->br.page_num.y)
        skel_rect.br.pixit_point.y = p_skel_rect1->br.pixit_point.y;
    else
        skel_rect.br.pixit_point.y = p_skel_rect2->br.pixit_point.y;
    skel_rect.br.page_num.y = MIN(p_skel_rect1->br.page_num.y, p_skel_rect2->br.page_num.y);

    if(NULL != p_intersect)
        *p_intersect = skel_rect;

    return(!skel_rect_empty(&skel_rect));
}

/******************************************************************************
*
* move a rectangle onto the next page
*
******************************************************************************/

extern void
skel_rect_move_to_next_page(
    _InoutRef_  P_SKEL_RECT p_skel_rect)
{
    PIXIT height;

    profile_ensure_frame();

    height = (p_skel_rect->tl.page_num.y == p_skel_rect->br.page_num.y)
                ? skel_rect_height(p_skel_rect)
                : 0;

    p_skel_rect->tl.pixit_point.y = 0;
    p_skel_rect->tl.page_num.y   += 1;

    p_skel_rect->br.pixit_point.y = p_skel_rect->tl.pixit_point.y + height;
    p_skel_rect->br.page_num.y    = p_skel_rect->tl.page_num.y;
}

/******************************************************************************
*
* return the union of two rectangles
*
******************************************************************************/

extern void
skel_rect_union(
    _OutRef_    P_SKEL_RECT p_union,
    _InRef_     PC_SKEL_RECT p_skel_rect1,
    _InRef_     PC_SKEL_RECT p_skel_rect2)
{
    profile_ensure_frame();

    p_union->tl.pixit_point.x = MIN(p_skel_rect1->tl.pixit_point.x, p_skel_rect2->tl.pixit_point.x);
    p_union->tl.pixit_point.y = MIN(p_skel_rect1->tl.pixit_point.y, p_skel_rect2->tl.pixit_point.y);
    p_union->tl.page_num.x = MIN(p_skel_rect1->tl.page_num.x, p_skel_rect2->tl.page_num.x);
    p_union->tl.page_num.y = MIN(p_skel_rect1->tl.page_num.y, p_skel_rect2->tl.page_num.y);

    p_union->br.pixit_point.x = MAX(p_skel_rect1->br.pixit_point.x, p_skel_rect2->br.pixit_point.x);
    p_union->br.pixit_point.y = MAX(p_skel_rect1->br.pixit_point.y, p_skel_rect2->br.pixit_point.y);
    p_union->br.page_num.x = MAX(p_skel_rect1->br.page_num.x, p_skel_rect2->br.page_num.x);
    p_union->br.page_num.y = MAX(p_skel_rect1->br.page_num.y, p_skel_rect2->br.page_num.y);
}

/******************************************************************************
*
* is this cell the first in the docu area ?
*
******************************************************************************/

_Check_return_
extern BOOL
slr_first_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_SLR p_slr)
{
    profile_ensure_frame();

    if(!p_docu_area->whole_col &&
       !p_docu_area->whole_row &&
       slr_equal(&p_docu_area->tl.slr, p_slr))
        return(TRUE);

    return(FALSE);
}

/******************************************************************************
*
* is this cell the last in the docu area ?
*
******************************************************************************/

_Check_return_
extern BOOL
slr_last_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_SLR p_slr)
{
    profile_ensure_frame();

    if(!p_docu_area->whole_col &&
       !p_docu_area->whole_row &&
       slr_equal_end(&p_docu_area->br.slr, p_slr))
        return(TRUE);

    return(FALSE);
}

/* end of sk_area.c */
