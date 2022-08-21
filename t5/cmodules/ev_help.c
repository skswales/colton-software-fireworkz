/* ev_help.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Routines for processing various data and types */

/* MRJC May 1991 / May 1992 / 1993 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

/*
internal functions
*/

static void
data_ensure_constant_sub(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     S32 array);

/******************************************************************************
*
* given a data item and a set of data allowable flags,
* convert as many different data items as possible into acceptable types
*
* --out--
* <0  error returned
* >=0 rpn number of data
*
* NOTE: if you pass in an SLR that has to be dereferenced,
* you may get a local copy of a string that will need freeing!
*
******************************************************************************/

_Check_return_
extern STATUS /* EV_IDNO or error */
arg_normalise(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y,
    P_STACK_DBASE p_stack_dbase)
{
    /* what have we currently got? */
    switch(p_ev_data->did_num)
    {
    case RPN_DAT_REAL:
        {
        if(type_flags & EM_REA)
            break; /* preferred */

        if(type_flags & EM_INT)
        {   /* try to obtain integer value from this real arg */
            if(status_done(real_to_integer_force(p_ev_data)))
                break;

            return(ev_data_set_error(p_ev_data, EVAL_ERR_ARGRANGE));
        }

        if(type_flags & EM_DAT)
        {   /* try to obtain date value from this real arg */
            if(status_ok(ss_serial_number_to_date(&p_ev_data->arg.ev_date, p_ev_data->arg.fp)))
                break;

            return(ev_data_set_error(p_ev_data, EVAL_ERR_ARGRANGE));
        }

        return(ev_data_set_error(p_ev_data, EVAL_ERR_UNEXNUMBER));
        }

    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        {
        if(type_flags & EM_INT)
            break; /* preferred */

        if(type_flags & EM_REA)
        {   /* promote this integer arg to real value */
            ev_data_set_real(p_ev_data, (F64) p_ev_data->arg.integer);
            break;
        }

        if(type_flags & EM_DAT)
        {   /* try to obtain date value from this integer arg */
            if(status_ok(ss_serial_number_to_date(&p_ev_data->arg.ev_date, (F64) p_ev_data->arg.integer)))
                break;

            return(ev_data_set_error(p_ev_data, EVAL_ERR_ARGRANGE));
        }

        return(ev_data_set_error(p_ev_data, EVAL_ERR_UNEXNUMBER));
        }

    case RPN_DAT_STRING:
        {
        if(type_flags & EM_STR)
            break; /* preferred */

        /* can't do anything with this string arg, best free it */
        ss_data_free_resources(p_ev_data);
        return(ev_data_set_error(p_ev_data, EVAL_ERR_UNEXSTRING));
        }

    case RPN_DAT_ARRAY:
    case RPN_DAT_RANGE:
    case RPN_DAT_FIELD:
        {
        if(NULL != p_stack_dbase)
        {
            EV_DATA ev_data;
            dbase_array_index(&ev_data, p_ev_data, p_stack_dbase, type_flags);
            ss_data_free_resources(p_ev_data);
            *p_ev_data = ev_data;
            return(arg_normalise(p_ev_data, type_flags, p_max_x, p_max_y, NULL));
        }

        if(type_flags & EM_ARY)
            break; /* preferred */

        if((NULL != p_max_x) && (NULL != p_max_y))
        {
            S32 x_size, y_size;
            array_range_sizes(p_ev_data, &x_size, &y_size);
            *p_max_x = MAX(*p_max_x, (S32) x_size);
            *p_max_y = MAX(*p_max_y, (S32) y_size);
            break;
        }

        ss_data_free_resources(p_ev_data);
        return(ev_data_set_error(p_ev_data, EVAL_ERR_UNEXARRAY));
        }

    case RPN_DAT_DATE:
        {
        if(type_flags & EM_DAT)
            break; /* preferred */

        /* coerce dates to Excel-compatible serial number if a number is acceptable */
        if(type_flags & EM_REA)
        {
            ev_data_set_real(p_ev_data, ss_date_to_serial_number(&p_ev_data->arg.ev_date));
            break;
        }

        if(type_flags & EM_INT)
        {   /* for EM_INT args ignore any time component */
            if(EV_DATE_NULL != p_ev_data->arg.ev_date.date)
                ev_data_set_integer(p_ev_data, ss_dateval_to_serial_number(&p_ev_data->arg.ev_date.date));
            else
                ev_data_set_integer(p_ev_data, 0); /* this is a pure time value */

            break;
        }

        return(ev_data_set_error(p_ev_data, EVAL_ERR_UNEXDATE));
        }

    case RPN_DAT_BLANK:
        {
        if(type_flags & EM_BLK)
            break; /* preferred */

        if(type_flags & EM_STR)
        {   /* map blank arg to empty string */
            p_ev_data->arg.string.uchars = uchars_empty_string;
            p_ev_data->arg.string.size = 0;
            p_ev_data->did_num = RPN_DAT_STRING;
            p_ev_data->local_data = 0;
            break;
        }

        /* map blank arg to zero and retry */
        ev_data_set_integer(p_ev_data, 0);
        return(arg_normalise(p_ev_data, type_flags, p_max_x, p_max_y, p_stack_dbase));
        }

    case RPN_DAT_ERROR:
        {
        if(type_flags & EM_ERR)
            break; /* preferred */

        return(p_ev_data->arg.ev_error.status);
        }

    case RPN_DAT_SLR:
        {
        if(type_flags & EM_SLR)
            break; /* preferred */

        /* function doesn't want SLRs, so dereference them and retry */
        ev_slr_deref(p_ev_data, &p_ev_data->arg.slr);
        return(arg_normalise(p_ev_data, type_flags, p_max_x, p_max_y, p_stack_dbase));
        }

    case RPN_DAT_NAME:
        /* no function handles NAME args, so dereference them and retry */
        name_deref(p_ev_data, p_ev_data->arg.h_name);
        return(arg_normalise(p_ev_data, type_flags, p_max_x, p_max_y, p_stack_dbase));

    case RPN_FRM_COND:
        {
        if(type_flags & EM_CDX)
            break; /* preferred */

        return(ev_data_set_error(p_ev_data, EVAL_ERR_BADEXPR));
        }
    }

    return(p_ev_data->did_num);
}

/******************************************************************************
*
* copy data element at p_ev_data into array element given by ap, x, y
*
******************************************************************************/

_Check_return_
static STATUS
array_element_copy(
    P_EV_DATA p_ev_data_array,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InRef_     PC_EV_DATA p_ev_data_from)
{
    const P_EV_DATA p_ev_data = ss_array_element_index_wr(p_ev_data_array, ix, iy);

    switch(p_ev_data_from->did_num)
    {
    case RPN_DAT_STRING:
        return(ss_string_dup(p_ev_data, p_ev_data_from));

    default:
        *p_ev_data = *p_ev_data_from;
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* copy one array into another; the target array is not shrunk, only expanded
*
* this task is not as simple as appears at first sight since the array may
* contain handle based resources e.g. strings which need copies making
*
******************************************************************************/

_Check_return_
static STATUS
array_copy(
    P_EV_DATA p_ev_data_to,
    P_EV_DATA p_ev_data_from)
{
    STATUS status = STATUS_OK;
    S32 x_to, y_to, x_from, y_from, ix, iy;

    array_range_sizes(p_ev_data_from, &x_from, &y_from);

    x_to = p_ev_data_to->arg.ev_array.x_size;
    y_to = p_ev_data_to->arg.ev_array.y_size;

    if(x_to < x_from || y_to < y_from)
        status = ss_array_element_make(p_ev_data_to, x_from - 1, y_from - 1);

    if(status_ok(status))
    {
        for(iy = 0; iy < y_from; ++iy)
        {
            for(ix = 0; ix < x_from; ++ix)
            {
                EV_DATA ev_data;

                (void) array_range_index(&ev_data, p_ev_data_from, ix, iy, EM_ANY);

                status = array_element_copy(p_ev_data_to, ix, iy, &ev_data);

                ss_data_free_resources(&ev_data);

                status_break(status);
            }

            status_break(status);
        }
    }
    else
    {
        ss_data_free_resources(p_ev_data_to);
        ev_data_set_error(p_ev_data_to, status);
    }

    return(status);
}

/******************************************************************************
*
* expand an existing array or data element to the given x,y size
*
* note that max_x, max_y are sizes not indices; i.e. to store element 0,2 sizes must be 1,3
*
******************************************************************************/

_Check_return_
extern STATUS
array_expand(
    P_EV_DATA p_ev_data,
    _InVal_     S32 max_x,
    _InVal_     S32 max_y)
{
    STATUS status = STATUS_OK;
    S32 old_x, old_y;

    array_range_sizes(p_ev_data, &old_x, &old_y);

    /* if array/range is already big enough, give up */
    if(max_x > old_x || max_y > old_y)
    {
        EV_DATA new_array;

        for(;;)
        {
            S32 ix, iy;

            /* make a new array of the required size */
            status_break(status = ss_array_make(&new_array, max_x, max_y));

            /* copy in existing data */
            if(data_is_array_range(p_ev_data))
                status_break(status = array_copy(&new_array, p_ev_data));

            if( (old_x == 1 && old_y == 1) ||
                (old_x == 0 && old_y == 0) )
            {
                EV_DATA ev_data;

                if(old_x)
                    (void) array_range_index(&ev_data, p_ev_data, 0, 0, EM_ANY);
                else
                    ev_data = *p_ev_data;

                /* replicate across and down */
                for(iy = 0; iy < max_y && status_ok(status); ++iy)
                {
                    for(ix = 0; ix < max_x; ++ix)
                    {
                        if(old_x && (!ix && !iy))
                            continue;

                        status_break(status = array_element_copy(&new_array, ix, iy, &ev_data));
                    }
                }

                ss_data_free_resources(&ev_data);
            }
            else if((max_x > old_x) && (old_y == max_y))
            {
                /* replicate across */
                for(iy = 0; iy < max_y && status_ok(status); ++iy)
                {
                    EV_DATA ev_data;

                    (void) array_range_index(&ev_data, p_ev_data, old_x - 1, iy, EM_ANY);

                    for(ix = 1; ix < max_x; ++ix)
                        status_break(status = array_element_copy(&new_array, ix, iy, &ev_data));

                    ss_data_free_resources(&ev_data);
                }
            }
            else if((max_y > old_y) && (old_x == max_x))
            {
                /* replicate down */
                for(ix = 0; ix < max_x && status_ok(status); ++ix)
                {
                    EV_DATA ev_data;

                    (void) array_range_index(&ev_data, p_ev_data, ix, old_y - 1, EM_ANY);

                    for(iy = 1; iy < max_y && status_ok(status); ++iy)
                        status_break(status = array_element_copy(&new_array, ix, iy, &ev_data));

                    ss_data_free_resources(&ev_data);
                }
            }
            else
            {
                /* fill new elements with blanks */
                EV_DATA ev_data;

                ev_data_set_blank(&ev_data);

                for(iy = 0; iy < old_y && status_ok(status); ++iy)
                    for(ix = old_x; ix < max_x; ++ix)
                        status_break(status = array_element_copy(&new_array, ix, iy, &ev_data));

                for(iy = old_y; iy < max_y && status_ok(status); ++iy)
                    for(ix = 0; ix < max_x; ++ix)
                        status_break(status = array_element_copy(&new_array, ix, iy, &ev_data));
            }

            break;
            /*NOTREACHED*/
        }

        ss_data_free_resources(p_ev_data);
        *p_ev_data = new_array;
    }

    return(status);
}

/******************************************************************************
*
* given a pointer to a data item and an x,y offset,
* return the relevant data item in an array or range;
* if the subscripts are too large, a subscript error
* is produced
*
* NOTE: if you request strings you may get a local copy which will need freeing
*
******************************************************************************/

_Check_return_
static EV_IDNO
array_range_index_common(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InVal_     EV_TYPE types)
{
    p_ev_data_out->local_data = 0;

    switch(p_ev_data_in->did_num)
    {
    case RPN_DAT_ARRAY:
        ss_array_element_read(p_ev_data_out, p_ev_data_in, ix, iy);
        break;

    case RPN_DAT_RANGE:
        p_ev_data_out->arg.slr = p_ev_data_in->arg.range.s;
        p_ev_data_out->arg.slr.col += EV_COL_PACK(ix);
        p_ev_data_out->arg.slr.row += (EV_ROW)    iy;
        p_ev_data_out->did_num = RPN_DAT_SLR;
        break;

    case RPN_DAT_FIELD:
        assert(ix == 0);
        ev_field_data_read(p_ev_data_out, p_ev_data_in->arg.h_name, iy);
        break;

    default: default_unhandled(); break;
    }

    status_assert(arg_normalise(p_ev_data_out, types, NULL, NULL, NULL));

    return(p_ev_data_out->did_num);
}

/*ncr*/
extern EV_IDNO
array_range_index(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InVal_     EV_TYPE types)
{
    S32 x_size, y_size;

    array_range_sizes(p_ev_data_in, &x_size, &y_size);

    if( ((U32) ix >= (U32) x_size) ||
        ((U32) iy >= (U32) y_size) ) /* caters for negative indexing */
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_SUBSCRIPT);
        return(p_ev_data_out->did_num);
    }

    p_ev_data_out->local_data = 0;

    switch(p_ev_data_in->did_num)
    {
    case RPN_DAT_ARRAY:
        ss_array_element_read(p_ev_data_out, p_ev_data_in, ix, iy);
        break;

    case RPN_DAT_RANGE:
        p_ev_data_out->arg.slr = p_ev_data_in->arg.range.s;
        p_ev_data_out->arg.slr.col += EV_COL_PACK(ix);
        p_ev_data_out->arg.slr.row += (EV_ROW)    iy;
        p_ev_data_out->did_num = RPN_DAT_SLR;
        break;

    case RPN_DAT_FIELD:
        assert(ix == 0);
        ev_field_data_read(p_ev_data_out, p_ev_data_in->arg.h_name, iy);
        break;

    default: default_unhandled(); break;
    }

    status_assert(arg_normalise(p_ev_data_out, types, NULL, NULL, NULL));

    return(p_ev_data_out->did_num);
}

/******************************************************************************
*
* extract an element of array or range using single index (across rows first)
*
* NOTE: if you request strings you may get a local copy which will need freeing
*
******************************************************************************/

/*ncr*/
extern EV_IDNO
array_range_mono_index(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in,
    _InVal_     S32 mono_ix,
    _InVal_     EV_TYPE types)
{
    S32 x_size, y_size;

    array_range_sizes(p_ev_data_in, &x_size, &y_size);

    if((0 == x_size) || (0 == y_size))
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_SUBSCRIPT);
        return(p_ev_data_out->did_num);
    }

    {
    const S32 iy  =       mono_ix / x_size;
    const S32 ix  = mono_ix - (iy * x_size);

    return(array_range_index_common(p_ev_data_out, p_ev_data_in, ix, iy, types));
    } /*block*/
}

/******************************************************************************
*
* return x and y sizes of arrays or ranges
*
******************************************************************************/

extern void
array_range_sizes(
    _InRef_     PC_EV_DATA p_ev_data_in,
    _OutRef_    P_S32 p_x_size,
    _OutRef_    P_S32 p_y_size)
{
    switch(p_ev_data_in->did_num)
    {
    case RPN_DAT_ARRAY:
        *p_x_size = p_ev_data_in->arg.ev_array.x_size;
        *p_y_size = p_ev_data_in->arg.ev_array.y_size;
        break;

    case RPN_DAT_RANGE:
        *p_x_size = (S32) ev_slr_col(&p_ev_data_in->arg.range.e) - (S32) ev_slr_col(&p_ev_data_in->arg.range.s);
        *p_y_size = (S32) ev_slr_row(&p_ev_data_in->arg.range.e) - (S32) ev_slr_row(&p_ev_data_in->arg.range.s);
        break;

    case RPN_DAT_FIELD:
        *p_x_size = 1;
        status_consume(ev_field_n_records(p_ev_data_in->arg.h_name, p_y_size));
        break;

    default:
        *p_x_size = *p_y_size = 0;
        break;
    }
}

extern void
array_range_mono_size(
    _InRef_     PC_EV_DATA p_ev_data_in,
    _OutRef_    P_S32 p_mono_size)
{
    S32 x_size, y_size;

    array_range_sizes(p_ev_data_in, &x_size, &y_size);

    *p_mono_size = x_size * y_size;
}

/******************************************************************************
*
* return normalised array elements
*
* call array_scan_init to start process
*
******************************************************************************/

_Check_return_
extern S32
array_scan_element(
    _InoutRef_  P_ARRAY_SCAN_BLOCK p_array_scan_block,
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     EV_TYPE type_flags)
{
    if(p_array_scan_block->x_pos >= p_array_scan_block->x_size)
    {
        p_array_scan_block->x_pos  = 0;
        p_array_scan_block->y_pos += 1;

        if(p_array_scan_block->y_pos >= p_array_scan_block->y_size)
        {
            CODE_ANALYSIS_ONLY(ev_data_set_blank(p_ev_data));
            return(RPN_FRM_END);
        }
    }

    ss_array_element_read(p_ev_data, &p_array_scan_block->ev_data, p_array_scan_block->x_pos, p_array_scan_block->y_pos);

    p_array_scan_block->x_pos += 1;

    return(arg_normalise(p_ev_data, type_flags, NULL, NULL, NULL));
}

/******************************************************************************
*
* start array scanning
*
******************************************************************************/

_Check_return_
extern STATUS
array_scan_init(
    _OutRef_    P_ARRAY_SCAN_BLOCK p_array_scan_block,
    _InRef_     PC_EV_DATA p_ev_data)
{
    assert_EQ(p_ev_data->did_num, RPN_DAT_ARRAY);

    p_array_scan_block->ev_data = *p_ev_data;
    p_array_scan_block->x_size = p_ev_data->arg.ev_array.x_size;
    p_array_scan_block->y_size = p_ev_data->arg.ev_array.y_size;

    p_array_scan_block->x_pos = p_array_scan_block->y_pos = 0;

    return(STATUS_OK);
}

/******************************************************************************
*
* locals for array sorting
*
******************************************************************************/

#if WINDOWS && 0

PROC_QSORT_S_PROTO(static, proc_array_sort, U32, EV_DATA)
{
    U32 x_index_context = * (PC_U32) context;

#else

static U32 x_index_static; /* no qsort() context */

PROC_QSORT_PROTO(static, proc_array_sort, EV_DATA)
{
    const U32 x_index_context = x_index_static;

#endif /* OS */

    QSORT_ARG1_VAR_DECL(PC_EV_DATA, p_ev_data_row_1);
    QSORT_ARG2_VAR_DECL(PC_EV_DATA, p_ev_data_row_2);

    const PC_EV_DATA p_ev_data_1 = p_ev_data_row_1 + x_index_context;
    const PC_EV_DATA p_ev_data_2 = p_ev_data_row_2 + x_index_context;

    return((int) ss_data_compare(p_ev_data_1, p_ev_data_2, FALSE, FALSE));
}

/******************************************************************************
*
* sort an array
*
******************************************************************************/

_Check_return_
extern STATUS
array_sort(
    P_EV_DATA p_ev_data,
    _InVal_     U32 x_index)
{
    STATUS status = STATUS_OK;

    assert(RPN_DAT_ARRAY == p_ev_data->did_num);

    {
    const U32 row_size = (U32) p_ev_data->arg.ev_array.x_size * sizeof32(EV_DATA);

    if(x_index < (U32) p_ev_data->arg.ev_array.x_size)
    {
        x_index_static = x_index;
        qsort(array_base(&p_ev_data->arg.ev_array.elements, EV_DATA), (U32) p_ev_data->arg.ev_array.y_size, row_size, proc_array_sort);
    }
    else
        status = create_error(EVAL_ERR_OUTOFRANGE);
    } /*block*/

    return(status);
}

/******************************************************************************
*
* make a constant array from a range
*
******************************************************************************/

static void
data_constant_array_from_field(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    if(RPN_DAT_FIELD == p_ev_data->did_num)
    {
        EV_DATA array_out;
        S32 x_size, y_size;

        array_range_sizes(p_ev_data, &x_size, &y_size);

        assert(x_size == 1);

        if(status_ok(ss_array_make(&array_out, x_size, y_size)))
        {
            S32 iy;
            STATUS status = STATUS_OK;

            for(iy = 0; iy < y_size; ++iy)
            {
                P_EV_DATA p_ev_data_i = ss_array_element_index_wr(&array_out, 0, iy);
                ev_field_data_read(p_ev_data_i, p_ev_data->arg.h_name, iy);
                if(ev_data_is_error(p_ev_data_i))
                {
                    status = p_ev_data_i->arg.ev_error.status;
                    break;
                }
            }

            if(status_fail(status))
            {
                ss_data_free_resources(&array_out);
                ev_data_set_error(&array_out, status);
            }
        }

        *p_ev_data = array_out;
    }
}

/******************************************************************************
*
* make a constant array from a range
*
******************************************************************************/

static void
data_constant_array_from_range(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    if(RPN_DAT_RANGE == p_ev_data->did_num)
    {
        EV_DATA array_out;
        S32 x_size, y_size;

        array_range_sizes(p_ev_data, &x_size, &y_size);

        if(status_ok(ss_array_make(&array_out, x_size, y_size)))
        {
            S32 ix, iy;
            EV_SLR slr;
            STATUS status = STATUS_OK;

            ev_slr_flags_init(&slr);
            ev_slr_docno_set(&slr, p_ev_data->arg.range.s.docno);

            for(iy = 0, slr.row = p_ev_data->arg.range.s.row; iy < y_size; ++iy, ++slr.row)
            {
                for(ix = 0, slr.col = p_ev_data->arg.range.s.col; ix < x_size; ++ix, ++slr.col)
                {
                    P_EV_DATA p_ev_data_i = ss_array_element_index_wr(&array_out, ix, iy);
                    ev_slr_deref(p_ev_data_i, &slr);
                    if(ev_data_is_error(p_ev_data_i)) /* may have a range containing error values */
                        continue;
                    data_ensure_constant_sub(p_ev_data_i, TRUE);
                    if(ev_data_is_error(p_ev_data_i))
                    {
                        status = p_ev_data_i->arg.ev_error.status;
                        break;
                    }
                }

                status_break(status);
            }

            if(status_fail(status))
            {
                ss_data_free_resources(&array_out);
                ev_data_set_error(&array_out, status);
            }
        }

        *p_ev_data = array_out;
    }
}

/******************************************************************************
*
* limit the types in a data item to those that are allowed in a result
*
******************************************************************************/

extern void
data_ensure_constant(
    P_EV_DATA p_ev_data)
{
    data_ensure_constant_sub(p_ev_data, FALSE);
}

static void
data_ensure_constant_sub(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     S32 array)
{
    switch(p_ev_data->did_num)
    {
    /* these types need no attention */
    case RPN_DAT_REAL:
    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
    case RPN_DAT_DATE:
    case RPN_DAT_BLANK:
    case RPN_DAT_ERROR:
    case RPN_DAT_STRING:
        break;

    /* scan array and remove embedded cockroaches */
    case RPN_DAT_ARRAY:
        if(array)
            ev_data_set_error(p_ev_data, EVAL_ERR_NESTEDARRAY);
        else
        {
            S32 ix, iy;
            STATUS status = STATUS_OK;

            assert(p_ev_data->local_data);

            for(iy = 0; iy < p_ev_data->arg.ev_array.y_size; ++iy)
            {
                for(ix = 0; ix < p_ev_data->arg.ev_array.x_size; ++ix)
                {
                    P_EV_DATA p_ev_data_i = ss_array_element_index_wr(p_ev_data, ix, iy);
                    if(ev_data_is_error(p_ev_data_i)) /* may have an array containing error values */
                        continue;
                    data_ensure_constant_sub(p_ev_data_i, TRUE);
                    if(ev_data_is_error(p_ev_data_i))
                    {
                        status = p_ev_data_i->arg.ev_error.status;
                        break;
                    }
                }

                status_break(status);
            }

            if(status_fail(status)) /* we have broken this data item */
            {
                ss_data_free_resources(p_ev_data);
                ev_data_set_error(p_ev_data, status);
            }
        }
        break;

    case RPN_DAT_SLR:
        ev_slr_deref(p_ev_data, &p_ev_data->arg.slr);
        data_ensure_constant_sub(p_ev_data, array);
        break;

    case RPN_DAT_NAME:
        name_deref(p_ev_data, p_ev_data->arg.h_name);
        data_ensure_constant_sub(p_ev_data, array);
        break;

    /* range is converted to array of constants */
    case RPN_DAT_RANGE:
        if(array)
            ev_data_set_error(p_ev_data, EVAL_ERR_UNEXRANGE);
        else
            data_constant_array_from_range(p_ev_data);
        break;

    /* field is converted to array of constants */
    case RPN_DAT_FIELD:
        if(array)
            ev_data_set_error(p_ev_data, EVAL_ERR_UNEXARRAY);
        else
            data_constant_array_from_field(p_ev_data);
        break;

    /* duff result types */
    default:
        ev_data_set_error(p_ev_data, EVAL_ERR_INTERNAL);
        break;
    }
}

/******************************************************************************
*
* say whether data is an array or not
*
******************************************************************************/

_Check_return_
extern BOOL
data_is_array_range(
    _InRef_     PC_EV_DATA p_ev_data)
{
    switch(p_ev_data->did_num)
    {
    case RPN_DAT_RANGE:
    case RPN_DAT_ARRAY:
    case RPN_DAT_FIELD:
        return(TRUE);

    default:
        return(FALSE);
    }
}

/******************************************************************************
*
* make a copy of an evaluator cell
* indirect resources are copied
* cell refs/ranges are updated:
*   p_ev_slr_offset->col/row contain offset
*   p_ev_slr_offset->doc contains target document
*   docno_from contains target document
*
******************************************************************************/

extern void
ev_exp_copy(
    _OutRef_    P_EV_CELL p_ev_cell_out,
    _InRef_     PC_EV_CELL p_ev_cell_in)
{
    EV_DATA ev_data;

    /* do binary copy */
    memcpy32(p_ev_cell_out, p_ev_cell_in, ev_len(p_ev_cell_in));

    /* copy cell results */
    ev_data_from_ev_cell(&ev_data, p_ev_cell_in);
    ev_cell_constant_from_data(p_ev_cell_out, &ev_data);
}

/******************************************************************************
*
* adjust the refs and ranges in an expression
* when the expression is copied
* cell refs/ranges are updated:
*   p_ev_slr_offset->col/row contain offset
*   p_ev_slr_offset->doc contains target document
*   docno_from contains target document
*
******************************************************************************/

extern void
ev_exp_refs_adjust(
    P_EV_CELL p_ev_cell,
    _InRef_     PC_EV_SLR p_ev_slr_offset    /* adjustment to slrs */,
    _InRef_opt_ PC_EV_RANGE p_ev_range_scope /* source range */)
{
    /* loop over slrs in cell */
    if(p_ev_cell->parms.slr_n)
    {
        P_EV_SLR p_ev_slr = p_ev_slr_from_ev_cell(p_ev_cell, 0);
        UBF i;

        for(i = 0; i < p_ev_cell->parms.slr_n; ++i, ++p_ev_slr)
            slr_offset_add(p_ev_slr, p_ev_slr_offset, p_ev_range_scope, TRUE /* use_abs */, FALSE /* end_coord */);
    }

    /* loop over ranges in cell */
    if(p_ev_cell->parms.range_n)
    {
        P_EV_RANGE p_ev_range = p_ev_range_from_ev_cell(p_ev_cell, 0);
        UBF i;

        for(i = 0; i < p_ev_cell->parms.range_n; ++i, ++p_ev_range)
        {
            slr_offset_add(&p_ev_range->s, p_ev_slr_offset, p_ev_range_scope, TRUE /* use_abs */, FALSE /* end_coord */);
            slr_offset_add(&p_ev_range->e, p_ev_slr_offset, p_ev_range_scope, TRUE /* use_abs */, TRUE /* end_coord */);
        }
    }
}

/******************************************************************************
*
* return normalised data from field
*
* call field_scan_init to start process
*
******************************************************************************/

_Check_return_
extern S32
field_scan_element(
    _InoutRef_  P_ARRAY_SCAN_BLOCK p_array_scan_block,
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     EV_TYPE type_flags)
{
    if(p_array_scan_block->y_pos >= p_array_scan_block->y_size)
    {
        CODE_ANALYSIS_ONLY(ev_data_set_blank(p_ev_data));
        return(RPN_FRM_END);
    }

    ev_field_data_read(p_ev_data, p_array_scan_block->ev_data.arg.h_name, p_array_scan_block->y_pos);

    p_array_scan_block->y_pos += 1;

    return(arg_normalise(p_ev_data, type_flags, NULL, NULL, NULL));
}

/******************************************************************************
*
* start field scanning
*
******************************************************************************/

_Check_return_
extern STATUS
field_scan_init(
    _OutRef_    P_ARRAY_SCAN_BLOCK p_array_scan_block,
    _InRef_     PC_EV_DATA p_ev_data)
{
    assert(RPN_DAT_FIELD == p_ev_data->did_num);

    p_array_scan_block->ev_data = *p_ev_data;
    array_range_sizes(p_ev_data, &p_array_scan_block->x_size, &p_array_scan_block->y_size);
    p_array_scan_block->x_pos = p_array_scan_block->y_pos = 0;
    return(STATUS_OK);
}

/******************************************************************************
*
* convert a data item to a result type
*
******************************************************************************/

extern void
ev_cell_constant_from_data(
    _OutRef_    P_EV_CELL p_ev_cell,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    EV_DATA ev_data_temp1, ev_data_temp2;

    switch(p_ev_data->did_num)
    {
    case RPN_DAT_SLR:
        ev_slr_deref(&ev_data_temp1, &p_ev_data->arg.slr);
        break;

    case RPN_DAT_NAME:
        name_deref(&ev_data_temp1, p_ev_data->arg.h_name);
        ev_cell_constant_from_data(p_ev_cell, &ev_data_temp1);
        return;

    default:
       /* claim ownership of input data */
        ev_data_temp1 = *p_ev_data;
        p_ev_data->local_data = 0;
        break;
    }

    if(!ev_data_temp1.local_data)
        status_assert(ss_data_resource_copy(&ev_data_temp2, &ev_data_temp1));
    else
        ev_data_temp2 = ev_data_temp1;

    data_ensure_constant(&ev_data_temp2);

    p_ev_cell->parms.did_num = ev_data_temp2.did_num;
    p_ev_cell->ev_constant = ev_data_temp2.arg.ev_constant;
}

/******************************************************************************
*
* free resources stored in an expression result
*
******************************************************************************/

extern void
ev_cell_free_resources(
    P_EV_CELL p_ev_cell)
{
    switch(p_ev_cell->parms.did_num)
    {
    case RPN_DAT_STRING:
        if(p_ev_cell->ev_constant.string.size)
        /* SKS 18apr96 only free strings that really exist. Pity 'tis an EV_CONSTANT not an EV_DATA then we'd know for sure */
            al_ptr_dispose(P_P_ANY_PEDANTIC(&p_ev_cell->ev_constant.string_wr.uchars));
        p_ev_cell->parms.did_num = RPN_DAT_BLANK;
        break;

    case RPN_DAT_ARRAY:
        {
        EV_DATA ev_data;
        ev_data_from_ev_cell(&ev_data, p_ev_cell);
        ev_data.local_data = 1;
        ss_data_free_resources(&ev_data);
        p_ev_cell->parms.did_num = RPN_DAT_BLANK;
        break;
        }

    default:
        break;
    }
}

/******************************************************************************
*
* read data from a cell
*
******************************************************************************/

extern void
ev_data_from_ev_cell(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_CELL p_ev_cell)
{
    p_ev_data->did_num = (U8) p_ev_cell->parms.did_num;
    p_ev_data->local_data = 0;
    p_ev_data->arg.ev_constant = p_ev_cell->ev_constant;
}

/******************************************************************************
*
* dereference an slr
*
* p_ev_data can be the same data structure as contains the slr
*
******************************************************************************/

extern void
ev_slr_deref(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_SLR p_ev_slr)
{
    P_EV_CELL p_ev_cell;
    EV_SLR ev_slr = *p_ev_slr;

    if(ev_slr.ext_ref && ev_doc_error(ev_slr_docno(&ev_slr)))
        ev_data_set_error(p_ev_data, ev_doc_error(ev_slr_docno(&ev_slr)));
    else
    {
        S32 res = ev_travel(&p_ev_cell, &ev_slr);

         /* it's external data */
        if(res <= 0)
            ev_external_data(p_ev_data, &ev_slr);
        /* it's an evaluator cell */
        else
        {
            ev_data_from_ev_cell(p_ev_data, p_ev_cell);

            switch(p_ev_data->did_num)
            {
            case RPN_DAT_ERROR:
                if(p_ev_data->arg.ev_error.type != ERROR_PROPAGATED)
                {
                    p_ev_data->arg.ev_error.type = ERROR_PROPAGATED;
                    p_ev_data->arg.ev_error.docno = ev_slr.docno; /* equivalent UBF */
                    p_ev_data->arg.ev_error.col = ev_slr.col; /* equivalent SBF */
                    p_ev_data->arg.ev_error.row = ev_slr.row;
                }
                break;

            case RPN_DAT_STRING:
                if(ss_string_is_blank(p_ev_data))
                {
                    ss_data_free_resources(p_ev_data);
                    ev_data_set_blank(p_ev_data);
                }
                break;
            }
        }
    }
}

#if TRACE_ALLOWED

extern void
ev_trace_slr_tstr_buf(
    _Out_writes_z_(elemof_tstr_buf) PTSTR tstr_buf,
    _InVal_     U32 elemof_tstr_buf,
    _In_z_      PCTSTR tstr,
    _InRef_     PC_EV_SLR p_ev_slr)
{
    PCTSTR dollar;
    EV_SLR slr;

    if(NULL != (dollar = tstrstr(tstr, TEXT("$$"))))
    {
        /* Copy head segment */
        U32 len_chars = PtrDiffElemU32(dollar, tstr);
        tstr_xstrnkpy(tstr_buf, elemof_tstr_buf, tstr, len_chars);

        /* Append decoded SLR */
        slr = *p_ev_slr;
        slr.ext_ref = 1;
        {
        UCHARZ ustr_buf[32];
        (void) ev_dec_slr_ustr_buf(ustr_bptr(ustr_buf), elemof32(ustr_buf), ev_slr_docno(&slr), &slr);
        tstr_xstrkat(tstr_buf + len_chars, elemof_tstr_buf - len_chars, _tstr_from_ustr(ustr_bptr(ustr_buf)));
        } /*block*/

        /* Append tail segment */
        tstr_xstrkat(tstr_buf, elemof_tstr_buf, dollar + 2);
    }
    else
        tstr_xstrkpy(tstr_buf, elemof_tstr_buf, tstr);
}

#endif /* TRACE_ALLOWED */

/******************************************************************************
*
* dereference a name by copying its body into the supplied data structure
*
******************************************************************************/

extern void
name_deref(
    P_EV_DATA p_ev_data,
    _InVal_     EV_HANDLE h_name)
{
    ARRAY_INDEX name_num = name_def_find(h_name);
    BOOL got_def = FALSE;

    if(name_num >= 0)
    {
        const PC_EV_NAME p_ev_name = array_ptr(&name_def.h_table, EV_NAME, name_num);

        if(!p_ev_name->flags.undefined)
        {
            got_def = 1;
            status_assert(ss_data_resource_copy(p_ev_data, &p_ev_name->def_data));
        }
    }

    if(!got_def)
        ev_data_set_error(p_ev_data, EVAL_ERR_NAMEUNDEF);
}

/******************************************************************************
*
* return next slr in range without overscanning; scans row by row, then col by col
*
******************************************************************************/

_Check_return_
extern S32
range_next(
    _InRef_     PC_EV_RANGE p_ev_range,
    _InoutRef_  P_EV_SLR p_ev_slr_pos)
{
    if( (ev_slr_row(p_ev_slr_pos) >= ev_slr_row(&p_ev_range->e)) ||
        (ev_slr_row(p_ev_slr_pos) >= ev_numrow(ev_slr_docno(p_ev_slr_pos))) )
    {
        p_ev_slr_pos->row  = p_ev_range->s.row;
        p_ev_slr_pos->col += 1;

        /* hit the end of the range ? */
        if(ev_slr_col(p_ev_slr_pos) >= ev_slr_col(&p_ev_range->e))
            return(0);
    }
    else
        p_ev_slr_pos->row += 1;

    return(1);
}

/******************************************************************************
*
* initialise scanning of a range
*
******************************************************************************/

_Check_return_
extern STATUS
range_scan_init(
    _OutRef_    P_RANGE_SCAN_BLOCK p_range_scan_block,
    _InRef_     PC_EV_RANGE p_ev_range)
{
    STATUS status = STATUS_OK;

    p_range_scan_block->range = *p_ev_range;
    p_range_scan_block->col_size = ev_slr_col(&p_range_scan_block->range.e) - ev_slr_col(&p_range_scan_block->range.s);
    p_range_scan_block->row_size = ev_slr_row(&p_range_scan_block->range.e) - ev_slr_row(&p_range_scan_block->range.s);

    p_range_scan_block->pos = p_range_scan_block->range.s;
    p_range_scan_block->slr_of_result = p_range_scan_block->pos;

    if(p_range_scan_block->range.s.ext_ref)
        status = ev_doc_error(ev_slr_docno(&p_range_scan_block->range.s));

    return(status);
}

/******************************************************************************
*
* scan each element of a range
*
******************************************************************************/

_Check_return_
extern S32
range_scan_element(
    _InoutRef_  P_RANGE_SCAN_BLOCK p_range_scan_block,
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     EV_TYPE type_flags)
{
    if(ev_slr_col(&p_range_scan_block->pos) >= ev_slr_col(&p_range_scan_block->range.e))
    {
        p_range_scan_block->pos.col  = p_range_scan_block->range.s.col; /* equivalent SBF */
        p_range_scan_block->pos.row += 1;

        if(ev_slr_row(&p_range_scan_block->pos) >= ev_slr_row(&p_range_scan_block->range.e))
        {
            CODE_ANALYSIS_ONLY(ev_data_set_blank(p_ev_data));
            return(RPN_FRM_END);
        }
    }

    ev_slr_deref(p_ev_data, &p_range_scan_block->pos);
    p_range_scan_block->slr_of_result = p_range_scan_block->pos;

    p_range_scan_block->pos.col += 1;

    return(arg_normalise(p_ev_data, type_flags, NULL, NULL, NULL));
}

/******************************************************************************
*
* add an offset to an slr when the slr is moved (maybe take account of abs bits)
*
******************************************************************************/

extern void
slr_offset_add(
    _InoutRef_  P_EV_SLR p_ev_slr,
    _InRef_     PC_EV_SLR p_ev_slr_offset,
    _InRef_opt_ PC_EV_RANGE p_ev_range_scope,
    _InVal_     BOOL use_abs,
    _InVal_     BOOL end_coord)
{
    if(end_coord)
    {
        p_ev_slr->col -= 1;
        p_ev_slr->row -= 1;
    }

    if((NULL == p_ev_range_scope) || ev_slr_in_range(p_ev_range_scope, p_ev_slr))
    {
        if(!use_abs || !p_ev_slr->abs_col)
            p_ev_slr->col += p_ev_slr_offset->col;

        if(!use_abs || !p_ev_slr->abs_row)
            p_ev_slr->row += p_ev_slr_offset->row;
    }

    if((NULL != p_ev_range_scope) && (ev_slr_docno(p_ev_slr) == ev_slr_docno(&p_ev_range_scope->s)))
        p_ev_slr->docno = p_ev_slr_offset->docno;

    if(end_coord)
    {
        p_ev_slr->col += 1;
        p_ev_slr->row += 1;
    }

    if(p_ev_slr->col < 0)
    {
        p_ev_slr->col = 0;
        p_ev_slr->bad_ref = 1;
    }

    if(p_ev_slr->row < 0)
    {
        p_ev_slr->row = 0;
        p_ev_slr->bad_ref = 1;
    }
}

/******************************************************************************
*
* p_ev_data_res = error iff either p_ev_data1 or p_ev_data2 are error
*
******************************************************************************/

_Check_return_ _Success_(return)
static inline BOOL
two_nums_propogate_errors(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2)
{
    if(ev_data_is_error(p_ev_data1))
    {
        *p_ev_data_res = *p_ev_data1;
        return(TRUE);
    }

    if(ev_data_is_error(p_ev_data2))
    {
        *p_ev_data_res = *p_ev_data2;
        return(TRUE);
    }

    return(FALSE);
}

/******************************************************************************
*
* p_ev_data_res = p_ev_data1 + p_ev_data2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
two_nums_add_try(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2,
    _InVal_     BOOL propogate_errors)
{
    BOOL did_op = FALSE;

    switch(two_nums_type_match(p_ev_data1, p_ev_data2, TRUE))
    {
    case TWO_INTS:
        ev_data_set_integer(p_ev_data_res, p_ev_data1->arg.integer + p_ev_data2->arg.integer);
        did_op = TRUE;
        break;

    case TWO_REALS:
        ev_data_set_real(p_ev_data_res, p_ev_data1->arg.fp + p_ev_data2->arg.fp);
        did_op = TRUE;
        break;

    default: default_unhandled();
#if CHECKING
    case TWO_MIXED:
#endif
        if(propogate_errors)
            did_op = two_nums_propogate_errors(p_ev_data_res, p_ev_data1, p_ev_data2);
        break;
    }

    return(did_op);
}

/******************************************************************************
*
* p_ev_data_res = p_ev_data1 / p_ev_data2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
two_nums_divide_try(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2,
    _InVal_     BOOL propogate_errors)
{
    BOOL did_op = FALSE;

    switch(two_nums_type_match(p_ev_data1, p_ev_data2, FALSE)) /* FALSE is OK as the result is always smaller if TWO_INTS */
    {
    case TWO_INTS:
        if(p_ev_data2->arg.integer == 0)
            ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
        else
        {
            if(0 == (p_ev_data1->arg.integer % p_ev_data2->arg.integer))
                ev_data_set_integer(p_ev_data_res, p_ev_data1->arg.integer / p_ev_data2->arg.integer);
            else
                ev_data_set_real(p_ev_data_res, (F64) p_ev_data1->arg.integer / p_ev_data2->arg.integer);
        }

        did_op = TRUE;
        break;

    case TWO_REALS:
        if(p_ev_data2->arg.fp == 0.0)
            ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
        else
            ev_data_set_real(p_ev_data_res, p_ev_data1->arg.fp / p_ev_data2->arg.fp);

        did_op = TRUE;
        break;

    default: default_unhandled();
#if CHECKING
    case TWO_MIXED:
#endif
        if(propogate_errors)
            did_op = two_nums_propogate_errors(p_ev_data_res, p_ev_data1, p_ev_data2);
        break;
    }

    return(did_op);
}

/******************************************************************************
*
* p_ev_data_res = p_ev_data1 - p_ev_data2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
two_nums_subtract_try(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2,
    _InVal_     BOOL propogate_errors)
{
    BOOL did_op = FALSE;

    switch(two_nums_type_match(p_ev_data1, p_ev_data2, TRUE))
    {
    case TWO_INTS:
        ev_data_set_integer(p_ev_data_res, p_ev_data1->arg.integer - p_ev_data2->arg.integer);
        did_op = TRUE;
        break;

    case TWO_REALS:
        ev_data_set_real(p_ev_data_res, p_ev_data1->arg.fp - p_ev_data2->arg.fp);
        did_op = TRUE;
        break;

    default: default_unhandled();
#if CHECKING
    case TWO_MIXED:
#endif
        if(propogate_errors)
            did_op = two_nums_propogate_errors(p_ev_data_res, p_ev_data1, p_ev_data2);
        break;
    }

    return(did_op);
}

/******************************************************************************
*
* p_ev_data_res = p_ev_data1 * p_ev_data2
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
two_nums_multiply_try(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2,
    _InVal_     BOOL propogate_errors)
{
    BOOL did_op = FALSE;

    switch(two_nums_type_match(p_ev_data1, p_ev_data2, TRUE))
    {
    case TWO_INTS:
        ev_data_set_integer(p_ev_data_res, p_ev_data1->arg.integer * p_ev_data2->arg.integer);
        did_op = TRUE;
        break;

    case TWO_REALS:
        ev_data_set_real(p_ev_data_res, p_ev_data1->arg.fp * p_ev_data2->arg.fp);
        did_op = TRUE;
        break;

    default: default_unhandled();
#if CHECKING
    case TWO_MIXED:
#endif
        if(propogate_errors)
            did_op = two_nums_propogate_errors(p_ev_data_res, p_ev_data1, p_ev_data2);
        break;
    }

    return(did_op);
}

/* end of ev_help.c */
