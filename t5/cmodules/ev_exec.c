/* ev_exec.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Semantic routines for evaluator */

/* MRJC April 1991 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

/******************************************************************************
*
* LOGICAL unary not operator
*
******************************************************************************/

PROC_EXEC_PROTO(c_uop_not)
{
    const bool not_result = !ss_data_get_logical(args[0]);

    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, not_result);
}

/******************************************************************************
*
* NUMBER unary plus operator
*
******************************************************************************/

PROC_EXEC_PROTO(c_uop_plus)
{
    exec_func_ignore_parms();

    assert(ss_data_is_number(args[0]));
    *p_ss_data_res = *(args[0]);
}

/******************************************************************************
*
* NUMBER unary minus operator
*
******************************************************************************/

PROC_EXEC_PROTO(c_uop_minus)
{
    exec_func_ignore_parms();

    assert(ss_data_is_number(args[0]));
    if(ss_data_is_real(args[0]))
        ss_data_set_real(p_ss_data_res, -ss_data_get_real(args[0]));
    else /* ss_data_is_integer */
        ss_data_set_integer(p_ss_data_res, -ss_data_get_integer(args[0]));
}

/******************************************************************************
*
* VALUE res = a + b
*
******************************************************************************/

static void
ss_data_bop_err_argtype(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_a,
    _InRef_     PC_SS_DATA p_ss_data_b)
{
    UNREFERENCED_PARAMETER_InRef_(p_ss_data_a);
    UNREFERENCED_PARAMETER_InRef_(p_ss_data_b);

    ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGTYPE);
}

/*
date arithmetic helper functions
*/

/*ncr*/
static bool
date_date_part_add_integer(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     S32 s32)
{
    INT64_WITH_INT32_OVERFLOW result;
    const P_SS_DATE_DATE date_part = &p_ss_data->arg.ss_date.date;

    if(SS_DATE_NULL == *date_part)
    {
        *date_part = (SS_DATE_DATE) s32;
        return(true);
    }

    *date_part = (SS_DATE_DATE) int32_add_check_overflow(*date_part, s32, &result);

    /* result would overflow this type? */
    if(result.f_overflow)
    {
        ss_data_set_error(p_ss_data, EVAL_ERR_BAD_DATE);
        return(false);
    }

    return(true);
}

/*ncr*/
static bool
date_time_part_add_integer(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     S32 s32)
{
    INT64_WITH_INT32_OVERFLOW result;
    const P_SS_DATE_TIME time_part = &p_ss_data->arg.ss_date.time;

    if(SS_TIME_NULL == *time_part)
    {
        *time_part = (SS_DATE_TIME) s32;
        return(true);
    }

    *time_part = (SS_DATE_TIME) int32_add_check_overflow(*time_part, s32, &result);

    /* result would overflow this type? */
    if(result.f_overflow)
    {
        ss_data_set_error(p_ss_data, EVAL_ERR_BAD_DATE);
        return(false);
    }

    return(true);
}

static inline void
date_calc_normalise(
    _InoutRef_  P_SS_DATA p_ss_data_res)
{
    assert(ss_data_is_date(p_ss_data_res));
    ss_date_normalise(&p_ss_data_res->arg.ss_date);
}

/* date + number (real) */

static void
date_add_real_number_calc(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATE p_ss_date,
    _InVal_     F64 f64)
{
    F64 floor_value;
    S32 s32;

    ss_data_set_date(p_ss_data_res, p_ss_date->date, p_ss_date->time);

    /* a la ss_data_real_to_integer_force */
    floor_value = real_floor(f64);

    if(fabs(floor_value) > (F64) S32_MAX)
    {
        ss_data_set_error(p_ss_data_res, EVAL_ERR_BAD_DATE);
        return;
    }

    s32 = (S32) floor_value;

    if(SS_DATE_NULL != p_ss_date->date)
    {
        if(!date_date_part_add_integer(p_ss_data_res, s32))
            return;

        /* handle any fraction as time part */
        if(f64 != floor_value)
        {
            /* fractional part denotes hh:mm:ss bit (don't be tempted to round or you could add a day!) */
            s32 = (S32) /*trunc*/ real_trunc((f64 - floor_value) * FP_SECS_IN_24);

            if(!date_time_part_add_integer(p_ss_data_res, s32))
                return;

            date_calc_normalise(p_ss_data_res);
        }
    }
    else if(SS_TIME_NULL != p_ss_date->time)
    {
        if(!date_time_part_add_integer(p_ss_data_res, s32))
            return;

        /* ignore any fraction */

        date_calc_normalise(p_ss_data_res);
    }
    else
    {   /* malnourished date */
        ss_data_set_error(p_ss_data_res, EVAL_ERR_BAD_DATE);
        return;
    }
}

/* date + number (any) */

static void
date_add_number_calc(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATE p_ss_date,
    _InRef_     PC_SS_DATA p_ss_data_addend)
{
    date_add_real_number_calc(p_ss_data_res, p_ss_date, ss_data_get_number(p_ss_data_addend));
}

/* date + date */

static void
date_add_date_calc(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATE p_ss_date_addend_a,
    _InRef_     PC_SS_DATE p_ss_date_addend_b)
{
    ss_data_set_date(p_ss_data_res, p_ss_date_addend_a->date, p_ss_date_addend_a->time);

    if(SS_DATE_NULL != p_ss_date_addend_b->date)
    {
        if(!date_date_part_add_integer(p_ss_data_res, p_ss_date_addend_b->date))
            return;
    }

    if(SS_TIME_NULL != p_ss_date_addend_b->time)
    {
        if(!date_time_part_add_integer(p_ss_data_res, p_ss_date_addend_b->time))
            return;
    }

    date_calc_normalise(p_ss_data_res);
}

static void
ss_data_add_addend_a_is_number(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_addend_a,
    _InRef_     PC_SS_DATA p_ss_data_addend_b)
{
    switch(ss_data_get_data_id(p_ss_data_addend_b))
    {
    case DATA_ID_DATE:
        /* number (any) + date */
        date_add_number_calc(p_ss_data_res, ss_data_get_date(p_ss_data_addend_b), p_ss_data_addend_a); /* NB reordered */
        break;

    default:
        /* number (any) + other */
        ss_data_bop_err_argtype(p_ss_data_res, p_ss_data_addend_a, p_ss_data_addend_b);
        break;
    }
}

static void
ss_data_add_addend_a_is_date(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_addend_a,
    _InRef_     PC_SS_DATA p_ss_data_addend_b)
{
    if(ss_data_is_number(p_ss_data_addend_b))
    {
        date_add_number_calc(p_ss_data_res, ss_data_get_date(p_ss_data_addend_a), p_ss_data_addend_b);
        return;
    }

    switch(ss_data_get_data_id(p_ss_data_addend_b))
    {
    case DATA_ID_DATE:
        /* date + date */
        date_add_date_calc(p_ss_data_res, ss_data_get_date(p_ss_data_addend_a), ss_data_get_date(p_ss_data_addend_b));
        break;

    default:
        /* date + other */
        ss_data_bop_err_argtype(p_ss_data_res, p_ss_data_addend_a, p_ss_data_addend_b);
        break;
    }
}

PROC_EXEC_PROTO(c_bop_add)
{
    exec_func_ignore_parms();

    ss_data_add(p_ss_data_res, args[0], args[1]);
}

extern void
ss_data_add(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_addend_a,
    _InoutRef_  P_SS_DATA p_ss_data_addend_b)
{
    if(ss_data_is_number(p_ss_data_addend_a))
    {
        if(two_nums_add_try(p_ss_data_res, p_ss_data_addend_a, p_ss_data_addend_b))
            return;

        ss_data_add_addend_a_is_number(p_ss_data_res, p_ss_data_addend_a, p_ss_data_addend_b);
        return;
    }

    switch(ss_data_get_data_id(p_ss_data_addend_a))
    {
    case DATA_ID_DATE:
        ss_data_add_addend_a_is_date(p_ss_data_res, p_ss_data_addend_a, p_ss_data_addend_b);
        break;

    default:
        ss_data_bop_err_argtype(p_ss_data_res, p_ss_data_addend_a, p_ss_data_addend_b);
        break;
    }
}

/******************************************************************************
*
* VALUE res = a - b
*
******************************************************************************/

/* date - number (any) */

static void
date_subtract_number_calc(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATE p_ss_date_minuend,
    _InRef_     PC_SS_DATA p_ss_data_subtrahend)
{
    assert(ss_data_is_number(p_ss_data_subtrahend));

    date_add_real_number_calc(p_ss_data_res, p_ss_date_minuend, -ss_data_get_number(p_ss_data_subtrahend));
}

/* date - date */

static bool
date_subtract_date_as_integer_calc(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     S32 s32_minuend,
    _InVal_     S32 s32_subtrahend)
{
    INT64_WITH_INT32_OVERFLOW result;

    ss_data_set_integer(p_ss_data_res, int32_subtract_check_overflow(s32_minuend, s32_subtrahend, &result));

    /* result would overflow this type? */
    if(result.f_overflow)
    {
        ss_data_set_error(p_ss_data_res, EVAL_ERR_BAD_DATE);
        return(false);
    }

    return(true);
}

static void
date_subtract_date_calc(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATE p_ss_date_minuend,
    _InRef_     PC_SS_DATE p_ss_date_subtrahend)
{
    if( (SS_DATE_NULL != p_ss_date_minuend->date)    && (SS_TIME_NULL == p_ss_date_minuend->time)    &&
        (SS_DATE_NULL != p_ss_date_subtrahend->date) && (SS_TIME_NULL == p_ss_date_subtrahend->time) )
    {   /* subtracting two pure dates - result is number of days difference */
        consume_bool(date_subtract_date_as_integer_calc(p_ss_data_res, p_ss_date_minuend->date, p_ss_date_subtrahend->date));
        return;
    }

    if( (SS_DATE_NULL == p_ss_date_minuend->date)    && (SS_TIME_NULL != p_ss_date_minuend->time)    &&
        (SS_DATE_NULL == p_ss_date_subtrahend->date) && (SS_TIME_NULL != p_ss_date_subtrahend->time) )
    {   /* subtracting two pure times - result is number of seconds difference */
        consume_bool(date_subtract_date_as_integer_calc(p_ss_data_res, p_ss_date_minuend->time, p_ss_date_subtrahend->time));
        return;
    }

    /* subtracting two mixed date/times */
    ss_data_set_date(p_ss_data_res, p_ss_date_minuend->date, p_ss_date_minuend->time);

    if(SS_DATE_NULL != p_ss_date_subtrahend->date)
    {
        if(!date_date_part_add_integer(p_ss_data_res, -p_ss_date_subtrahend->date))
            return;
    }

    if(SS_TIME_NULL != p_ss_date_subtrahend->time)
    {
        if(!date_time_part_add_integer(p_ss_data_res, -p_ss_date_subtrahend->time))
            return;
    }

    date_calc_normalise(p_ss_data_res);
}

static void
ss_data_subtract_minuend_is_number(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_minuend,
    _InRef_     PC_SS_DATA p_ss_data_subtrahend)
{
    switch(ss_data_get_data_id(p_ss_data_subtrahend))
    {
    case DATA_ID_DATE:
        /* number (any) - date */
        ss_data_set_error(p_ss_data_res, EVAL_ERR_UNEXDATE);
        break;

    default:
        /* number (any) - other */
        ss_data_bop_err_argtype(p_ss_data_res, p_ss_data_minuend, p_ss_data_subtrahend);
        break;
    }
}

static void
ss_data_subtract_minuend_is_date(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_minuend,
    _InRef_     PC_SS_DATA p_ss_data_subtrahend)
{
    if(ss_data_is_number(p_ss_data_subtrahend))
    {
        date_subtract_number_calc(p_ss_data_res, ss_data_get_date(p_ss_data_minuend), p_ss_data_subtrahend);
        return;
    }

    switch(ss_data_get_data_id(p_ss_data_subtrahend))
    {
    case DATA_ID_DATE:
        /* date - date */
        date_subtract_date_calc(p_ss_data_res, ss_data_get_date(p_ss_data_minuend), ss_data_get_date(p_ss_data_subtrahend));
        break;

    default:
        /* date - other */
        ss_data_bop_err_argtype(p_ss_data_res, p_ss_data_minuend, p_ss_data_subtrahend);
        break;
    }
}

PROC_EXEC_PROTO(c_bop_sub)
{
    exec_func_ignore_parms();

    ss_data_subtract(p_ss_data_res, args[0], args[1]);
}

extern void
ss_data_subtract(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_minuend,
    _InoutRef_  P_SS_DATA p_ss_data_subtrahend)
{
    if(ss_data_is_number(p_ss_data_minuend))
    {
        if(two_nums_subtract_try(p_ss_data_res, p_ss_data_minuend, p_ss_data_subtrahend))
            return;

        ss_data_subtract_minuend_is_number(p_ss_data_res, p_ss_data_minuend, p_ss_data_subtrahend);
        return;
    }

    switch(ss_data_get_data_id(p_ss_data_minuend))
    {
    case DATA_ID_DATE:
        ss_data_subtract_minuend_is_date(p_ss_data_res, p_ss_data_minuend, p_ss_data_subtrahend);
        break;

    default:
        ss_data_bop_err_argtype(p_ss_data_res, p_ss_data_minuend, p_ss_data_subtrahend);
        break;
    }
}

/******************************************************************************
*
* VALUE res = a * b
*
******************************************************************************/

PROC_EXEC_PROTO(c_bop_mul)
{
    exec_func_ignore_parms();

    ss_data_multiply(p_ss_data_res, args[0], args[1]);
}

extern void
ss_data_multiply(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_multiplicand_a,
    _InoutRef_  P_SS_DATA p_ss_data_multiplicand_b)
{
 retry:;

    if(two_nums_multiply_try(p_ss_data_res, p_ss_data_multiplicand_a, p_ss_data_multiplicand_b))
        return;

    /* handle dates here by converting to Excel serial numbers (yuk) */
    if(ss_data_is_date(p_ss_data_multiplicand_a))
    {
        if(status_ok(arg_normalise(p_ss_data_multiplicand_a, EM_REA, NULL, NULL, NULL)))
            goto retry;
    }

    if(ss_data_is_date(p_ss_data_multiplicand_b))
    {
        if(status_ok(arg_normalise(p_ss_data_multiplicand_b, EM_REA, NULL, NULL, NULL)))
            goto retry;
    }

    ss_data_bop_err_argtype(p_ss_data_res, p_ss_data_multiplicand_a, p_ss_data_multiplicand_b);
}

/******************************************************************************
*
* VALUE res = a / b
*
******************************************************************************/

PROC_EXEC_PROTO(c_bop_div)
{
    exec_func_ignore_parms();

    ss_data_divide(p_ss_data_res, args[0], args[1]);
}

extern void
ss_data_divide(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_dividend,
    _InoutRef_  P_SS_DATA p_ss_data_divisor)
{
 retry:;

    if(two_nums_divide_try(p_ss_data_res, p_ss_data_dividend, p_ss_data_divisor))
        return;

    /* handle dates here by converting to Excel serial numbers (yuk) */
    if(ss_data_is_date(p_ss_data_dividend))
    {
        if(status_ok(arg_normalise(p_ss_data_dividend, EM_REA, NULL, NULL, NULL)))
            goto retry;
    }

    if(ss_data_is_date(p_ss_data_divisor))
    {
        if(status_ok(arg_normalise(p_ss_data_divisor, EM_REA, NULL, NULL, NULL)))
            goto retry;
    }

    ss_data_bop_err_argtype(p_ss_data_res, p_ss_data_dividend, p_ss_data_divisor);
}

/******************************************************************************
*
* NUMBER res = a ^ b
*
******************************************************************************/

PROC_EXEC_PROTO(c_bop_power)
{
    const F64 a = ss_data_get_real(args[0]);
    const F64 b = ss_data_get_real(args[1]);

    exec_func_ignore_parms();

    errno = 0;

    if(0.5 == b)
        ss_data_set_real_try_integer(p_ss_data_res, sqrt(a));
    else
        ss_data_set_real_try_integer(p_ss_data_res, pow(a, b));

    if(errno /* == EDOM */)
    {
        bool ok = false;

        /* test for negative numbers to power of 1/odd number (odd roots are OK) */
        if( (a < 0.0) && (b <= 1.0) )
        {
            SS_DATA test;

            ss_data_set_real(&test, 1.0 / b);

            if(ss_data_real_to_integer_try(&test))
            {
                ss_data_set_real_try_integer(p_ss_data_res, -(pow(-(a), b)));
                ok = true;
            }
        }

        if(!ok)
            exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
    }
}

/******************************************************************************
*
* LOGICAL res = a && b
*
******************************************************************************/

PROC_EXEC_PROTO(c_bop_and)
{
    const bool a = ss_data_get_logical(args[0]);
    const bool b = ss_data_get_logical(args[1]);
    const bool and_result = a && b;

    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, and_result);
}

/******************************************************************************
*
* LOGICAL res = a | b
*
******************************************************************************/

PROC_EXEC_PROTO(c_bop_or)
{
    const bool a = ss_data_get_logical(args[0]);
    const bool b = ss_data_get_logical(args[1]);
    const bool or_result = a || b;

    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, or_result);
}

/******************************************************************************
*
* STRING res = a && b (concatenate)
*
******************************************************************************/

PROC_EXEC_PROTO(c_bop_concatenate)
{
    exec_func_ignore_parms();

    /* call my friend to do the hard work */
    c_join(args, n_args, p_ss_data_res, p_cur_slr);
}

/******************************************************************************
*
* LOGICAL res = a == b
*
******************************************************************************/

PROC_EXEC_PROTO(c_rel_eq)
{
    const bool eq_result = (ss_data_compare(args[0], args[1], TRUE, TRUE) == 0);

    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, eq_result);
}

/******************************************************************************
*
* LOGICAL res = a > b
*
******************************************************************************/

PROC_EXEC_PROTO(c_rel_gt)
{
    const bool gt_result = (ss_data_compare(args[0], args[1], TRUE, TRUE) > 0);

    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, gt_result);
}

/******************************************************************************
*
* LOGICAL res = a >= b
*
******************************************************************************/

PROC_EXEC_PROTO(c_rel_gteq)
{
    const bool gteq_result = (ss_data_compare(args[0], args[1], TRUE, TRUE) >= 0);

    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, gteq_result);
}

/******************************************************************************
*
* LOGICAL res = a < b
*
******************************************************************************/

PROC_EXEC_PROTO(c_rel_lt)
{
    const bool lt_result = (ss_data_compare(args[0], args[1], TRUE, TRUE) < 0);

    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, lt_result);
}

/******************************************************************************
*
* LOGICAL res = a <= b
*
******************************************************************************/

PROC_EXEC_PROTO(c_rel_lteq)
{
    const bool lteq_result = (ss_data_compare(args[0], args[1], TRUE, TRUE) <= 0);

    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, lteq_result);
}

/******************************************************************************
*
* LOGICAL res = a != b
*
******************************************************************************/

PROC_EXEC_PROTO(c_rel_neq)
{
    const bool neq_result = (ss_data_compare(args[0], args[1], TRUE, TRUE) != 0);

    exec_func_ignore_parms();

    ss_data_set_logical(p_ss_data_res, neq_result);
}

/******************************************************************************
*
* VALUE if(num, true_res, false_res)
*
******************************************************************************/

PROC_EXEC_PROTO(c_if)
{
    exec_func_ignore_parms();

    if(ss_data_get_logical(args[0]))
        status_assert(ss_data_resource_copy(p_ss_data_res, args[1]));
    else if(n_args > 2)
        status_assert(ss_data_resource_copy(p_ss_data_res, args[2]));
    else
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ODF_NA);
}

/* end of ev_exec.c */
