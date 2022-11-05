/* ev_help.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

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
    _InoutRef_  P_SS_DATA p_ss_data,
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
static STATUS /* error */
arg_normalise_set_error_ARGRANGE(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    return(ss_data_set_error(p_ss_data, EVAL_ERR_ARGRANGE));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_real_as_integer( /* try to obtain integer value from this real arg */
    _InoutRef_  P_SS_DATA p_ss_data)
{
    const EV_IDNO data_id = ss_data_real_to_integer_force(p_ss_data);

    if(DATA_ID_CONVERSION_FAILED != data_id)
        return(data_id);

    return(arg_normalise_set_error_ARGRANGE(p_ss_data));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_real_as_date( /* try to obtain date value from this real arg */
    _InoutRef_  P_SS_DATA p_ss_data)
{
    SS_DATE ss_date;

    if(status_ok(ss_serial_number_to_date(&ss_date, ss_data_get_real(p_ss_data))))
    {
        ss_data_set_date(p_ss_data, ss_date.date, ss_date.time);
        assert(ss_data_is_date(p_ss_data));
        return(DATA_ID_DATE);
    }

    return(arg_normalise_set_error_ARGRANGE(p_ss_data));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_real(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_REA)
        return(DATA_ID_REAL); /* preferred */

    if(type_flags & EM_INT)
        /* try to obtain integer value from this real arg */
        return(arg_normalise_real_as_integer(p_ss_data));

    if(type_flags & EM_DAT)
        /* try to obtain date value from this real arg */
        return(arg_normalise_real_as_date(p_ss_data));

    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXNUMBER));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_integer_as_real( /* promote this integer arg to real value */
    _InoutRef_  P_SS_DATA p_ss_data)
{
    return(ss_data_set_real_rid(p_ss_data, (F64) ss_data_get_integer(p_ss_data)));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_integer_as_date( /* try to obtain date value from this integer arg */
    _InoutRef_  P_SS_DATA p_ss_data)
{
    SS_DATE ss_date;

    if(status_ok(ss_serial_number_to_date(&ss_date, (F64) ss_data_get_integer(p_ss_data))))
    {
        ss_data_set_date(p_ss_data, ss_date.date, ss_date.time);
        assert(ss_data_is_date(p_ss_data));
        return(DATA_ID_DATE);
    }

    return(arg_normalise_set_error_ARGRANGE(p_ss_data));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_integer(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_INT)
        return(ss_data_get_data_id(p_ss_data)); /* preferred */

    if(type_flags & EM_REA)
        /* promote this integer arg to real value */
        return(arg_normalise_integer_as_real(p_ss_data));

    if(type_flags & EM_DAT)
        /* try to obtain date value from this integer arg */
        return(arg_normalise_integer_as_date(p_ss_data));

    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXNUMBER));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_date_as_real(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    return(ss_data_set_real_rid(p_ss_data, ss_date_to_serial_number(ss_data_get_date(p_ss_data))));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_date_as_integer( /* for EM_INT args ignore any time component */
    _InoutRef_  P_SS_DATA p_ss_data)
{
    if(SS_DATE_NULL == ss_data_get_date(p_ss_data)->date)
    {   /* this is a pure time value */
        assert(SS_TIME_NULL != ss_data_get_date(p_ss_data)->time);
        return(ss_data_set_integer_rid(p_ss_data, 0));
    }

    return(ss_data_set_integer_rid(p_ss_data, ss_dateval_to_serial_number(ss_data_get_date(p_ss_data)->date)));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_date(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_DAT)
        return(DATA_ID_DATE); /* preferred */

    /* coerce dates to Excel-compatible serial number if a number is acceptable */
    if(type_flags & EM_REA)
        return(arg_normalise_date_as_real(p_ss_data));

    if(type_flags & EM_INT)
        /* for EM_INT args ignore any time component */
        return(arg_normalise_date_as_integer(p_ss_data));

    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXDATE));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_string(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_STR)
        return(DATA_ID_STRING); /* preferred */

    /* can't do anything with this string arg, best free it */
    ss_data_free_resources(p_ss_data);
    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXSTRING));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_blank_as_string( /* map blank arg to empty string */
    _InoutRef_  P_SS_DATA p_ss_data)
{
    p_ss_data->arg.string.uchars = uchars_empty_string;
    p_ss_data->arg.string.size = 0;
    ss_data_set_data_id(p_ss_data, DATA_ID_STRING);
    p_ss_data->local_data = 0;
    return(DATA_ID_STRING);
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_blank(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_BLK)
        return(DATA_ID_BLANK); /* preferred */

    if(type_flags & EM_STR)
        /* map blank arg to empty string */
        return(arg_normalise_blank_as_string(p_ss_data));

    /* otherwise map blank arg to zero and retry */
    ss_data_set_integer(p_ss_data, 0);
    return(arg_normalise_integer(p_ss_data, type_flags));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_error(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_ERR)
        return(DATA_ID_ERROR); /* preferred */

    return(p_ss_data->arg.ss_error.status);
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_a_r_f_stack_dbase( /* array or range or field */
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y,
    P_STACK_DBASE p_stack_dbase)
{
    SS_DATA ss_data;
    dbase_array_index(&ss_data, p_ss_data, p_stack_dbase, type_flags);
    ss_data_free_resources(p_ss_data);
    *p_ss_data = ss_data;
    return(arg_normalise(p_ss_data, type_flags, p_max_x, p_max_y, NULL));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_array_range_field(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y,
    P_STACK_DBASE p_stack_dbase)
{
    if(NULL != p_stack_dbase)
        return(arg_normalise_a_r_f_stack_dbase(p_ss_data, type_flags, p_max_x, p_max_y, p_stack_dbase));

    if(type_flags & EM_ARY)
        return(ss_data_get_data_id(p_ss_data)); /* preferred */ /* ARRAY/RANGE/FIELD */

    if( (NULL != p_max_x) && (NULL != p_max_y) )
    {
        S32 x_size, y_size;
        array_range_sizes(p_ss_data, &x_size, &y_size);
        *p_max_x = MAX(*p_max_x, (S32) x_size);
        *p_max_y = MAX(*p_max_y, (S32) y_size);
        return(ss_data_get_data_id(p_ss_data));
    }

    ss_data_free_resources(p_ss_data);

    return(ss_data_set_error(p_ss_data, EVAL_ERR_UNEXARRAY));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_slr(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y,
    P_STACK_DBASE p_stack_dbase)
{
    if(type_flags & EM_SLR)
        return(DATA_ID_SLR); /* preferred */

    /* function doesn't want SLRs, so dereference them and retry */
    return(arg_normalise(ev_slr_deref(p_ss_data, &p_ss_data->arg.slr), type_flags, p_max_x, p_max_y, p_stack_dbase));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_name(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y,
    P_STACK_DBASE p_stack_dbase)
{
    /* no function handles NAME args, so dereference them and retry */
    return(arg_normalise(name_deref(p_ss_data, p_ss_data->arg.h_name), type_flags, p_max_x, p_max_y, p_stack_dbase));
}

_Check_return_
static STATUS /* EV_IDNO or error */
arg_normalise_cond(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(type_flags & EM_CDX)
        return(ss_data_get_data_id(p_ss_data)); /* preferred */

    return(ss_data_set_error(p_ss_data, EVAL_ERR_BADEXPR));
}

_Check_return_
extern STATUS /* EV_IDNO or error */
arg_normalise(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y,
    P_STACK_DBASE p_stack_dbase)
{
    /* what have we currently got? */
    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_REAL:
        return(arg_normalise_real(p_ss_data, type_flags));

    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        return(arg_normalise_integer(p_ss_data, type_flags));

    case DATA_ID_DATE:
        return(arg_normalise_date(p_ss_data, type_flags));

    case DATA_ID_STRING:
        return(arg_normalise_string(p_ss_data, type_flags));

    case DATA_ID_BLANK:
        return(arg_normalise_blank(p_ss_data, type_flags));

    case DATA_ID_ERROR:
        return(arg_normalise_error(p_ss_data, type_flags));

    case DATA_ID_ARRAY:
        return(arg_normalise_array_range_field(p_ss_data, type_flags, p_max_x, p_max_y, p_stack_dbase));

    case DATA_ID_SLR:
        return(arg_normalise_slr(p_ss_data, type_flags, p_max_x, p_max_y, p_stack_dbase));

    case DATA_ID_RANGE:
    case DATA_ID_FIELD:
        return(arg_normalise_array_range_field(p_ss_data, type_flags, p_max_x, p_max_y, p_stack_dbase));

    case DATA_ID_NAME:
        return(arg_normalise_name(p_ss_data, type_flags, p_max_x, p_max_y, p_stack_dbase));

    case RPN_FRM_COND:
        return(arg_normalise_cond(p_ss_data, type_flags));

    default:
        return(ss_data_get_data_id(p_ss_data));
    }
}

/******************************************************************************
*
* copy data element at p_ss_data into array element given by ap, x, y
*
******************************************************************************/

_Check_return_
static STATUS
array_element_copy(
    P_SS_DATA p_ss_data_array,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InRef_     PC_SS_DATA p_ss_data_from)
{
    const P_SS_DATA p_ss_data = ss_array_element_index_wr(p_ss_data_array, ix, iy);

    switch(ss_data_get_data_id(p_ss_data_from))
    {
    case DATA_ID_STRING:
        return(ss_string_dup(p_ss_data, p_ss_data_from));

    default:
        *p_ss_data = *p_ss_data_from;
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
    P_SS_DATA p_ss_data_to,
    P_SS_DATA p_ss_data_from)
{
    STATUS status = STATUS_OK;
    S32 x_to, y_to, x_from, y_from, ix, iy;

    array_range_sizes(p_ss_data_from, &x_from, &y_from);

    x_to = p_ss_data_to->arg.ss_array.x_size;
    y_to = p_ss_data_to->arg.ss_array.y_size;

    if( (x_to < x_from) || (y_to < y_from) )
        status = ss_array_element_make(p_ss_data_to, x_from - 1, y_from - 1);

    if(status_ok(status))
    {
        for(iy = 0; iy < y_from; ++iy)
        {
            for(ix = 0; ix < x_from; ++ix)
            {
                SS_DATA ss_data;

                (void) array_range_index(&ss_data, p_ss_data_from, ix, iy, EM_ANY);

                status = array_element_copy(p_ss_data_to, ix, iy, &ss_data);

                ss_data_free_resources(&ss_data);

                status_break(status);
            }

            status_break(status);
        }
    }
    else
    {
        ss_data_free_resources(p_ss_data_to);
        ss_data_set_error(p_ss_data_to, status);
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
    P_SS_DATA p_ss_data,
    _InVal_     S32 max_x,
    _InVal_     S32 max_y)
{
    STATUS status = STATUS_OK;
    S32 old_x, old_y;

    array_range_sizes(p_ss_data, &old_x, &old_y);

    /* if array/range is already big enough, give up */
    if(max_x > old_x || max_y > old_y)
    {
        SS_DATA new_array;

        for(;;)
        {
            S32 ix, iy;

            /* make a new array of the required size */
            status_break(status = ss_array_make(&new_array, max_x, max_y));

            /* copy in existing data */
            if(data_is_array_range(p_ss_data))
                status_break(status = array_copy(&new_array, p_ss_data));

            if( (old_x == 1 && old_y == 1) ||
                (old_x == 0 && old_y == 0) )
            {
                SS_DATA ss_data;

                if(old_x)
                    (void) array_range_index(&ss_data, p_ss_data, 0, 0, EM_ANY);
                else
                    ss_data = *p_ss_data;

                /* replicate across and down */
                for(iy = 0; iy < max_y && status_ok(status); ++iy)
                {
                    for(ix = 0; ix < max_x; ++ix)
                    {
                        if(old_x && (!ix && !iy))
                            continue;

                        status_break(status = array_element_copy(&new_array, ix, iy, &ss_data));
                    }
                }

                ss_data_free_resources(&ss_data);
            }
            else if((max_x > old_x) && (old_y == max_y))
            {
                /* replicate across */
                for(iy = 0; iy < max_y && status_ok(status); ++iy)
                {
                    SS_DATA ss_data;

                    (void) array_range_index(&ss_data, p_ss_data, old_x - 1, iy, EM_ANY);

                    for(ix = 1; ix < max_x; ++ix)
                        status_break(status = array_element_copy(&new_array, ix, iy, &ss_data));

                    ss_data_free_resources(&ss_data);
                }
            }
            else if((max_y > old_y) && (old_x == max_x))
            {
                /* replicate down */
                for(ix = 0; ix < max_x && status_ok(status); ++ix)
                {
                    SS_DATA ss_data;

                    (void) array_range_index(&ss_data, p_ss_data, ix, old_y - 1, EM_ANY);

                    for(iy = 1; iy < max_y && status_ok(status); ++iy)
                        status_break(status = array_element_copy(&new_array, ix, iy, &ss_data));

                    ss_data_free_resources(&ss_data);
                }
            }
            else
            {
                /* fill new elements with blanks */
                SS_DATA ss_data;

                ss_data_set_blank(&ss_data);

                for(iy = 0; iy < old_y && status_ok(status); ++iy)
                    for(ix = old_x; ix < max_x; ++ix)
                        status_break(status = array_element_copy(&new_array, ix, iy, &ss_data));

                for(iy = old_y; iy < max_y && status_ok(status); ++iy)
                    for(ix = 0; ix < max_x; ++ix)
                        status_break(status = array_element_copy(&new_array, ix, iy, &ss_data));
            }

            break;
            /*NOTREACHED*/
        }

        ss_data_free_resources(p_ss_data);
        *p_ss_data = new_array;
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
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InVal_     EV_TYPE type_flags)
{
    p_ss_data_out->local_data = 0;

    switch(ss_data_get_data_id(p_ss_data_in))
    {
    case DATA_ID_ARRAY:
        ss_array_element_read(p_ss_data_out, p_ss_data_in, ix, iy);
        break;

    case DATA_ID_RANGE:
        p_ss_data_out->arg.slr = p_ss_data_in->arg.range.s;
        p_ss_data_out->arg.slr.col += EV_COL_PACK(ix);
        p_ss_data_out->arg.slr.row += (EV_ROW)    iy;
        ss_data_set_data_id(p_ss_data_out, DATA_ID_SLR);
        break;

    case DATA_ID_FIELD:
        assert(ix == 0);
        ev_field_data_read(p_ss_data_out, p_ss_data_in->arg.h_name, iy);
        break;

    default: default_unhandled();
        ss_data_set_error(p_ss_data_out, EVAL_ERR_NO_VALID_DATA);
        break;
    }

    status_assert(arg_normalise(p_ss_data_out, type_flags, NULL, NULL, NULL));

    return(ss_data_get_data_id(p_ss_data_out));
}

/*ncr*/
extern EV_IDNO
array_range_index(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InVal_     EV_TYPE type_flags)
{
    S32 x_size, y_size;

    array_range_sizes(p_ss_data_in, &x_size, &y_size);

    if( ((U32) ix >= (U32) x_size) ||
        ((U32) iy >= (U32) y_size) ) /* caters for negative indexing */
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_SUBSCRIPT);
        return(ss_data_get_data_id(p_ss_data_out));
    }

    p_ss_data_out->local_data = 0;

    switch(ss_data_get_data_id(p_ss_data_in))
    {
    case DATA_ID_ARRAY:
        ss_array_element_read(p_ss_data_out, p_ss_data_in, ix, iy);
        break;

    case DATA_ID_RANGE:
        p_ss_data_out->arg.slr = p_ss_data_in->arg.range.s;
        p_ss_data_out->arg.slr.col += EV_COL_PACK(ix);
        p_ss_data_out->arg.slr.row += (EV_ROW)    iy;
        ss_data_set_data_id(p_ss_data_out, DATA_ID_SLR);
        break;

    case DATA_ID_FIELD:
        assert(ix == 0);
        ev_field_data_read(p_ss_data_out, p_ss_data_in->arg.h_name, iy);
        break;

    default: default_unhandled();
        ss_data_set_error(p_ss_data_out, EVAL_ERR_NO_VALID_DATA);
        break;
    }

    status_assert(arg_normalise(p_ss_data_out, type_flags, NULL, NULL, NULL));

    return(ss_data_get_data_id(p_ss_data_out));
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
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in,
    _InVal_     S32 mono_ix,
    _InVal_     EV_TYPE type_flags)
{
    S32 x_size, y_size;

    array_range_sizes(p_ss_data_in, &x_size, &y_size);

    if( (0 == x_size) || (0 == y_size) )
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_SUBSCRIPT);
        return(ss_data_get_data_id(p_ss_data_out));
    }

    {
    const S32 iy  =       mono_ix / x_size;
    const S32 ix  = mono_ix - (iy * x_size);

    return(array_range_index_common(p_ss_data_out, p_ss_data_in, ix, iy, type_flags));
    } /*block*/
}

/******************************************************************************
*
* return x and y sizes of arrays or ranges
*
******************************************************************************/

extern void
array_range_sizes(
    _InRef_     PC_SS_DATA p_ss_data_in,
    _OutRef_    P_S32 p_x_size,
    _OutRef_    P_S32 p_y_size)
{
    switch(ss_data_get_data_id(p_ss_data_in))
    {
    case DATA_ID_ARRAY:
        *p_x_size = p_ss_data_in->arg.ss_array.x_size;
        *p_y_size = p_ss_data_in->arg.ss_array.y_size;
        break;

    case DATA_ID_RANGE:
        *p_x_size = (S32) ev_slr_col(&p_ss_data_in->arg.range.e) - (S32) ev_slr_col(&p_ss_data_in->arg.range.s);
        *p_y_size = (S32) ev_slr_row(&p_ss_data_in->arg.range.e) - (S32) ev_slr_row(&p_ss_data_in->arg.range.s);
        break;

    case DATA_ID_FIELD:
        *p_x_size = 1;
        status_consume(ev_field_n_records(p_ss_data_in->arg.h_name, p_y_size));
        break;

    default:
        *p_x_size = *p_y_size = 0;
        break;
    }
}

extern void
array_range_mono_size(
    _InRef_     PC_SS_DATA p_ss_data_in,
    _OutRef_    P_S32 p_mono_size)
{
    S32 x_size, y_size;

    array_range_sizes(p_ss_data_in, &x_size, &y_size);

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
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(p_array_scan_block->x_pos >= p_array_scan_block->x_size)
    {
        p_array_scan_block->x_pos  = 0;
        p_array_scan_block->y_pos += 1;

        if(p_array_scan_block->y_pos >= p_array_scan_block->y_size)
        {
            CODE_ANALYSIS_ONLY(ss_data_set_blank(p_ss_data));
            return(RPN_FRM_END);
        }
    }

    ss_array_element_read(p_ss_data, &p_array_scan_block->ss_data, p_array_scan_block->x_pos, p_array_scan_block->y_pos);

    p_array_scan_block->x_pos += 1;

    return(arg_normalise(p_ss_data, type_flags, NULL, NULL, NULL));
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
    _InRef_     PC_SS_DATA p_ss_data)
{
    assert(ss_data_is_array(p_ss_data));

    p_array_scan_block->ss_data = *p_ss_data;
    p_array_scan_block->x_size = p_ss_data->arg.ss_array.x_size;
    p_array_scan_block->y_size = p_ss_data->arg.ss_array.y_size;

    p_array_scan_block->x_pos = p_array_scan_block->y_pos = 0;

    return(STATUS_OK);
}

/******************************************************************************
*
* locals for array sorting
*
******************************************************************************/

#if WINDOWS && 0

PROC_QSORT_S_PROTO(static, proc_array_sort, U32, SS_DATA)
{
    U32 x_index_context = * (PC_U32) context;

#else

static U32 x_index_static; /* no qsort() context */

PROC_QSORT_PROTO(static, proc_array_sort, SS_DATA)
{
    const U32 x_index_context = x_index_static;

#endif /* OS */

    QSORT_ARG1_VAR_DECL(PC_SS_DATA, p_ss_data_row_1);
    QSORT_ARG2_VAR_DECL(PC_SS_DATA, p_ss_data_row_2);

    const PC_SS_DATA p_ss_data_1 = p_ss_data_row_1 + x_index_context;
    const PC_SS_DATA p_ss_data_2 = p_ss_data_row_2 + x_index_context;

    return((int) ss_data_compare(p_ss_data_1, p_ss_data_2, FALSE, FALSE));
}

/******************************************************************************
*
* sort an array
*
******************************************************************************/

_Check_return_
extern STATUS
array_sort(
    P_SS_DATA p_ss_data,
    _InVal_     U32 x_index)
{
    STATUS status = STATUS_OK;

    assert(ss_data_is_array(p_ss_data));

    {
    const U32 row_size = (U32) p_ss_data->arg.ss_array.x_size * sizeof32(SS_DATA);

    if(x_index < (U32) p_ss_data->arg.ss_array.x_size)
    {
        x_index_static = x_index;
        qsort(array_base(&p_ss_data->arg.ss_array.elements, SS_DATA), (U32) p_ss_data->arg.ss_array.y_size, row_size, proc_array_sort);
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
    _InoutRef_  P_SS_DATA p_ss_data)
{
    if(DATA_ID_FIELD == ss_data_get_data_id(p_ss_data))
    {
        SS_DATA array_out;
        S32 x_size, y_size;

        array_range_sizes(p_ss_data, &x_size, &y_size);

        assert(x_size == 1);

        if(status_ok(ss_array_make(&array_out, x_size, y_size)))
        {
            S32 iy;
            STATUS status = STATUS_OK;

            for(iy = 0; iy < y_size; ++iy)
            {
                P_SS_DATA p_ss_data_i = ss_array_element_index_wr(&array_out, 0, iy);
                ev_field_data_read(p_ss_data_i, p_ss_data->arg.h_name, iy);
                if(ss_data_is_error(p_ss_data_i))
                {
                    status = p_ss_data_i->arg.ss_error.status;
                    break;
                }
            }

            if(status_fail(status))
            {
                ss_data_free_resources(&array_out);
                ss_data_set_error(&array_out, status);
            }
        }

        *p_ss_data = array_out;
    }
}

/******************************************************************************
*
* make a constant array from a range
*
******************************************************************************/

static void
data_constant_array_from_range(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    if(DATA_ID_RANGE == ss_data_get_data_id(p_ss_data))
    {
        SS_DATA array_out;
        S32 x_size, y_size;

        array_range_sizes(p_ss_data, &x_size, &y_size);

        if(status_ok(ss_array_make(&array_out, x_size, y_size)))
        {
            S32 ix, iy;
            EV_SLR slr;
            STATUS status = STATUS_OK;

            ev_slr_flags_init(&slr);
            ev_slr_docno_set(&slr, p_ss_data->arg.range.s.docno);

            for(iy = 0, slr.row = p_ss_data->arg.range.s.row; iy < y_size; ++iy, ++slr.row)
            {
                for(ix = 0, slr.col = p_ss_data->arg.range.s.col; ix < x_size; ++ix, ++slr.col)
                {
                    P_SS_DATA p_ss_data_i = ss_array_element_index_wr(&array_out, ix, iy);
                    ev_slr_deref(p_ss_data_i, &slr);
                    if(ss_data_is_error(p_ss_data_i)) /* may have a range containing error values */
                        continue;
                    data_ensure_constant_sub(p_ss_data_i, TRUE);
                    if(ss_data_is_error(p_ss_data_i))
                    {
                        status = p_ss_data_i->arg.ss_error.status;
                        break;
                    }
                }

                status_break(status);
            }

            if(status_fail(status))
            {
                ss_data_free_resources(&array_out);
                ss_data_set_error(&array_out, status);
            }
        }

        *p_ss_data = array_out;
    }
}

/******************************************************************************
*
* limit the types in a data item to those that are allowed in a result
*
******************************************************************************/

extern void
data_ensure_constant(
    P_SS_DATA p_ss_data)
{
    data_ensure_constant_sub(p_ss_data, FALSE);
}

static void
data_ensure_constant_sub(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     S32 array)
{
    switch(ss_data_get_data_id(p_ss_data))
    {
    /* these types need no attention */
    case DATA_ID_REAL:
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
    case DATA_ID_DATE:
    case DATA_ID_STRING:
    case DATA_ID_BLANK:
    case DATA_ID_ERROR:
        break;

    /* scan array and remove embedded cockroaches */
    case DATA_ID_ARRAY:
        if(array)
            ss_data_set_error(p_ss_data, EVAL_ERR_NESTEDARRAY);
        else
        {
            S32 ix, iy;
            STATUS status = STATUS_OK;

            assert(p_ss_data->local_data);

            for(iy = 0; iy < p_ss_data->arg.ss_array.y_size; ++iy)
            {
                for(ix = 0; ix < p_ss_data->arg.ss_array.x_size; ++ix)
                {
                    P_SS_DATA p_ss_data_i = ss_array_element_index_wr(p_ss_data, ix, iy);
                    if(ss_data_is_error(p_ss_data_i)) /* may have an array containing error values */
                        continue;
                    data_ensure_constant_sub(p_ss_data_i, TRUE);
                    if(ss_data_is_error(p_ss_data_i))
                    {
                        status = p_ss_data_i->arg.ss_error.status;
                        break;
                    }
                }

                status_break(status);
            }

            if(status_fail(status)) /* we have broken this data item */
            {
                ss_data_free_resources(p_ss_data);
                ss_data_set_error(p_ss_data, status);
            }
        }
        break;

    case DATA_ID_SLR:
        data_ensure_constant_sub(ev_slr_deref(p_ss_data, &p_ss_data->arg.slr), array);
        break;

    /* range is converted to array of constants */
    case DATA_ID_RANGE:
        if(array)
            ss_data_set_error(p_ss_data, EVAL_ERR_UNEXRANGE);
        else
            data_constant_array_from_range(p_ss_data);
        break;

    /* field is converted to array of constants */
    case DATA_ID_FIELD:
        if(array)
            ss_data_set_error(p_ss_data, EVAL_ERR_UNEXARRAY);
        else
            data_constant_array_from_field(p_ss_data);
        break;

    case DATA_ID_NAME:
        data_ensure_constant_sub(name_deref(p_ss_data, p_ss_data->arg.h_name), array);
        break;

    /* duff result types */
    default:
        ss_data_set_error(p_ss_data, EVAL_ERR_INTERNAL);
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
    _InRef_     PC_SS_DATA p_ss_data)
{
    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_ARRAY:
    case DATA_ID_RANGE:
    case DATA_ID_FIELD:
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
    SS_DATA ss_data;

    /* do binary copy */
    memcpy32(p_ev_cell_out, p_ev_cell_in, ev_len(p_ev_cell_in));

    /* copy cell results */
    ss_data_from_ev_cell(&ss_data, p_ev_cell_in);
    ev_cell_constant_from_data(p_ev_cell_out, &ss_data);
}

/******************************************************************************
*
* adjust the refs and ranges in an expression when the expression is copied
* cell/range refs are updated:
*   p_ev_slr_offset->col/row contain offset
*   p_ev_slr_offset->doc contains target document
*   docno_from contains target document
*
******************************************************************************/

extern void
ev_exp_refs_adjust(
    _InoutRef_  P_EV_CELL p_ev_cell,
    _InRef_     PC_EV_SLR p_ev_slr_offset /* adjustment to slrs */)
{
    /* loop over slrs in cell */
    if(p_ev_cell->ev_parms.slr_n)
    {
        P_EV_SLR p_ev_slr = p_ev_slr_from_ev_cell(p_ev_cell, 0);
        UBF i;

        for(i = 0; i < p_ev_cell->ev_parms.slr_n; ++i, ++p_ev_slr)
            slr_offset_add(p_ev_slr, p_ev_slr_offset, NULL, TRUE /* use_abs */, FALSE /* end_coord */);
    }

    /* loop over ranges in cell */
    if(p_ev_cell->ev_parms.range_n)
    {
        P_EV_RANGE p_ev_range = p_ev_range_from_ev_cell(p_ev_cell, 0);
        UBF i;

        for(i = 0; i < p_ev_cell->ev_parms.range_n; ++i, ++p_ev_range)
        {
            slr_offset_add(&p_ev_range->s, p_ev_slr_offset, NULL, TRUE /* use_abs */, FALSE /* end_coord */);
            slr_offset_add(&p_ev_range->e, p_ev_slr_offset, NULL, TRUE /* use_abs */, TRUE /* end_coord */);
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
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(p_array_scan_block->y_pos >= p_array_scan_block->y_size)
    {
        CODE_ANALYSIS_ONLY(ss_data_set_blank(p_ss_data));
        return(RPN_FRM_END);
    }

    ev_field_data_read(p_ss_data, p_array_scan_block->ss_data.arg.h_name, p_array_scan_block->y_pos);

    p_array_scan_block->y_pos += 1;

    return(arg_normalise(p_ss_data, type_flags, NULL, NULL, NULL));
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
    _InRef_     PC_SS_DATA p_ss_data)
{
    assert(DATA_ID_FIELD == ss_data_get_data_id(p_ss_data));

    p_array_scan_block->ss_data = *p_ss_data;
    array_range_sizes(p_ss_data, &p_array_scan_block->x_size, &p_array_scan_block->y_size);
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
    _InoutRef_  P_SS_DATA p_ss_data)
{
    SS_DATA ss_data_temp_1, ss_data_temp_2;

    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_SLR:
        ev_slr_deref(&ss_data_temp_1, &p_ss_data->arg.slr);
        break;

    case DATA_ID_NAME:
        ev_cell_constant_from_data(p_ev_cell, name_deref(&ss_data_temp_1, p_ss_data->arg.h_name));
        return;

    default:
       /* claim ownership of input data */
        ss_data_temp_1 = *p_ss_data;
        p_ss_data->local_data = 0;
        break;
    }

    if(!ss_data_temp_1.local_data)
        status_assert(ss_data_resource_copy(&ss_data_temp_2, &ss_data_temp_1));
    else
        ss_data_temp_2 = ss_data_temp_1;

    data_ensure_constant(&ss_data_temp_2);

    p_ev_cell->ev_parms.data_id = UBF_PACK(ss_data_get_data_id(&ss_data_temp_2));

    p_ev_cell->ss_constant = ss_data_temp_2.arg.ss_constant;
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
    switch(UBF_UNPACK(EV_IDNO, p_ev_cell->ev_parms.data_id))
    {
    case DATA_ID_STRING:
        if(p_ev_cell->ss_constant.string.size)
        /* SKS 18apr96 only free strings that really exist. Pity 'tis an SS_CONSTANT not an SS_DATA then we'd know for sure */
            al_ptr_dispose(P_P_ANY_PEDANTIC(&p_ev_cell->ss_constant.string_wr.uchars));
        p_ev_cell->ev_parms.data_id = UBF_PACK(DATA_ID_BLANK);
        break;

    case DATA_ID_ARRAY:
        {
        SS_DATA ss_data;
        ss_data_from_ev_cell(&ss_data, p_ev_cell);
        ss_data.local_data = 1;
        ss_data_free_resources(&ss_data);
        p_ev_cell->ev_parms.data_id = UBF_PACK(DATA_ID_BLANK);
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
ss_data_from_ev_cell(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_EV_CELL p_ev_cell)
{
    ss_data_set_data_id(p_ss_data, UBF_UNPACK(EV_IDNO, p_ev_cell->ev_parms.data_id)); /* limited range stored in cell's bitfield */
    p_ss_data->local_data = 0;
    p_ss_data->arg.ss_constant = p_ev_cell->ss_constant;
}

/******************************************************************************
*
* dereference an slr
*
* p_ss_data can be the same data structure as contains the slr
*
******************************************************************************/

/*ncr*/ _Ret_notnull_
extern P_SS_DATA
ev_slr_deref(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_EV_SLR p_ev_slr)
{
    P_EV_CELL p_ev_cell;
    EV_SLR ev_slr = *p_ev_slr;

    if(ev_slr.ext_ref && ev_doc_error(ev_slr_docno(&ev_slr)))
        ss_data_set_error(p_ss_data, ev_doc_error(ev_slr_docno(&ev_slr)));
    else
    {
        S32 res = ev_travel(&p_ev_cell, &ev_slr);

         /* it's external data */
        if(res <= 0)
            ev_external_data(p_ss_data, &ev_slr);
        /* it's an evaluator cell */
        else
        {
            ss_data_from_ev_cell(p_ss_data, p_ev_cell);

            switch(ss_data_get_data_id(p_ss_data))
            {
            case DATA_ID_STRING:
                if(ss_string_is_blank(p_ss_data))
                {
                    ss_data_free_resources(p_ss_data);
                    ss_data_set_blank(p_ss_data);
                }
                break;

            case DATA_ID_ERROR:
                if(p_ss_data->arg.ss_error.type != ERROR_PROPAGATED)
                {
                    p_ss_data->arg.ss_error.type = ERROR_PROPAGATED;
                    p_ss_data->arg.ss_error.docno = ev_slr.docno; /* equivalent UBF */
                    p_ss_data->arg.ss_error.col = ev_slr.col; /* equivalent SBF */
                    p_ss_data->arg.ss_error.row = ev_slr.row;
                }
                break;
            }
        }
    }

    return(p_ss_data);
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

/*ncr*/ _Ret_notnull_
extern P_SS_DATA
name_deref(
    P_SS_DATA p_ss_data,
    _InVal_     EV_HANDLE h_name)
{
    const ARRAY_INDEX name_num = name_def_from_handle(h_name);
    BOOL got_def = FALSE;

    if(name_num >= 0)
    {
        const PC_EV_NAME p_ev_name = array_ptr(&name_def_deptable.h_table, EV_NAME, name_num);

        if(!p_ev_name->flags.undefined)
        {
            got_def = TRUE;
            status_assert(ss_data_resource_copy(p_ss_data, &p_ev_name->def_data));
        }
    }

    if(!got_def)
        ss_data_set_error(p_ss_data, EVAL_ERR_NAMEUNDEF);

    return(p_ss_data);
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
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     EV_TYPE type_flags)
{
    if(ev_slr_col(&p_range_scan_block->pos) >= ev_slr_col(&p_range_scan_block->range.e))
    {
        p_range_scan_block->pos.col  = p_range_scan_block->range.s.col; /* equivalent SBF */
        p_range_scan_block->pos.row += 1;

        if(ev_slr_row(&p_range_scan_block->pos) >= ev_slr_row(&p_range_scan_block->range.e))
        {
            CODE_ANALYSIS_ONLY(ss_data_set_blank(p_ss_data));
            return(RPN_FRM_END);
        }
    }

    ev_slr_deref(p_ss_data, &p_range_scan_block->pos);
    p_range_scan_block->slr_of_result = p_range_scan_block->pos;

    p_range_scan_block->pos.col += 1;

    return(arg_normalise(p_ss_data, type_flags, NULL, NULL, NULL));
}

/******************************************************************************
*
* add an offset to an EV_SLR when it is moved (maybe take account of abs bits)
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

extern void
slr_offset_add_simple(
    _InoutRef_  P_EV_SLR p_ev_slr,
    _InRef_     PC_EV_SLR p_ev_slr_offset)
{
    /* adjust column reference if it is relative */
    if(!p_ev_slr->abs_col)
    {
        p_ev_slr->col += p_ev_slr_offset->col;

        /* has adjustment made it overflow? */
        if((U32) p_ev_slr->col >= (U32) EV_MAX_COL)
        {
            p_ev_slr->col = 0;
            p_ev_slr->bad_ref = 1;
        }
    }

    /* adjust row reference if it is relative */
    if(!p_ev_slr->abs_row)
    {
        p_ev_slr->row += p_ev_slr_offset->row;

        /* has adjustment made it overflow? */
        if((U32) p_ev_slr->row >= (U32) EV_MAX_ROW)
        {
            p_ev_slr->row = 0;
            p_ev_slr->bad_ref = 1;
        }
    }
}

extern void
slr_offset_add_harder(
    _InoutRef_  P_EV_SLR p_ev_slr,
    _InRef_     PC_EV_SLR p_ev_slr_offset,
    _InRef_opt_ PC_EV_RANGE p_ev_range_scope,
    _InVal_     BOOL end_coord)
{
    BOOL adjust_col, adjust_row;

    if(end_coord)
    {
        p_ev_slr->col -= 1;
        p_ev_slr->row -= 1;
    }

    /* adjust column reference if it is relative */
    if(!p_ev_slr->abs_col)
        adjust_col = TRUE;
    /* column reference is fixed. adjust only if there is a source block and the reference was in that block ('move') */
    else if((NULL != p_ev_range_scope) && ev_slr_in_range(p_ev_range_scope, p_ev_slr))
        adjust_col = TRUE;
    else
        adjust_col = FALSE;

    if(adjust_col)
        p_ev_slr->col += p_ev_slr_offset->col;

    /* adjust row reference if it is relative */
    if(!p_ev_slr->abs_row)
        adjust_row = TRUE;
    /* row reference is fixed. adjust only if there is a source block and the reference was in that block ('move') */
    else if((NULL != p_ev_range_scope) && ev_slr_in_range(p_ev_range_scope, p_ev_slr))
        adjust_row = TRUE;
    else
        adjust_row = FALSE;

    if(adjust_row)
        p_ev_slr->row += p_ev_slr_offset->row;

    /* adjust_docno */
    if((NULL != p_ev_range_scope) && (ev_slr_docno(p_ev_slr) == ev_slr_docno(&p_ev_range_scope->s)))
        p_ev_slr->docno = p_ev_slr_offset->docno;

    if(end_coord)
    {
        p_ev_slr->col += 1;
        p_ev_slr->row += 1;
    }

    /* has adjustment made it overflow? */
    if((U32) p_ev_slr->col >= (U32) EV_MAX_COL)
    {
        p_ev_slr->col = 0;
        p_ev_slr->bad_ref = 1;
    }

    /* has adjustment made it overflow? */
    if((U32) p_ev_slr->row >= (U32) EV_MAX_ROW)
    {
        p_ev_slr->row = 0;
        p_ev_slr->bad_ref = 1;
    }
}

/* end of ev_help.c */
