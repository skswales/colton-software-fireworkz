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
* NUMBER unary plus
*
******************************************************************************/

PROC_EXEC_PROTO(c_uplus)
{
    exec_func_ignore_parms();

    if(RPN_DAT_REAL == args[0]->did_num)
        *p_ev_data_res = *(args[0]);
    else
        ev_data_set_integer(p_ev_data_res, args[0]->arg.integer);
}

/******************************************************************************
*
* NUMBER unary minus
*
******************************************************************************/

PROC_EXEC_PROTO(c_uminus)
{
    exec_func_ignore_parms();

    if(RPN_DAT_REAL == args[0]->did_num)
        ev_data_set_real(p_ev_data_res, -args[0]->arg.fp);
    else
        ev_data_set_integer(p_ev_data_res, -args[0]->arg.integer);
}

/******************************************************************************
*
* BOOLEAN unary not / not() function
*
******************************************************************************/

PROC_EXEC_PROTO(c_not)
{
    BOOL not_result = (args[0]->arg.integer == 0);

    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, not_result);
}

/******************************************************************************
*
* BOOLEAN res = a && b
*
******************************************************************************/

PROC_EXEC_PROTO(c_and)
{
    BOOL a = (args[0]->arg.integer != 0);
    BOOL b = (args[1]->arg.integer != 0);
    BOOL and_result = a && b;

    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, and_result);
}

/******************************************************************************
*
* VALUE res = a * b
*
******************************************************************************/

PROC_EXEC_PROTO(c_mul)
{
    exec_func_ignore_parms();

    if(!two_nums_multiply_try(p_ev_data_res, args[0], args[1], FALSE))
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGTYPE);
}

/******************************************************************************
*
* VALUE res = a + b
*
******************************************************************************/

#define SECS_IN_24 ((S32) 60 * (S32) 60 * (S32) 24)

static void
date_plus_real_number_calc(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InRef_     PC_EV_DATE p_ev_date,
    _InVal_     F64 f64)
{
    F64 floor_value = real_floor(f64);
    S32 s32;

    p_ev_data_res->did_num = RPN_DAT_DATE;
    p_ev_data_res->arg.ev_date = *p_ev_date;

    if(EV_DATE_NULL != p_ev_date->date)
    {   /* a la real_to_integer_force */
        if((floor_value > (F64) S32_MAX) || (floor_value < (F64) -S32_MAX))
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_DATE);
            return;
        }

        s32 = (S32) floor_value;

        p_ev_data_res->arg.ev_date.date += s32;

        if(f64 != floor_value)
        {   /* handle any fraction as time part */
            s32 = (S32) ((f64 - floor_value) * SECS_IN_24);

            if(EV_TIME_NULL != p_ev_date->time)
                p_ev_data_res->arg.ev_date.time += s32;
            else
                p_ev_data_res->arg.ev_date.time  = s32;

            ss_date_normalise(&p_ev_data_res->arg.ev_date);
        }
    }
    else if(EV_TIME_NULL != p_ev_date->time)
    {
        if((floor_value > (F64) S32_MAX) || (floor_value < (F64) -S32_MAX))
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_DATE);
            return;
        }

        s32 = (S32) floor_value;

        p_ev_data_res->arg.ev_date.time += s32;

        /* ignore any fraction */

        ss_date_normalise(&p_ev_data_res->arg.ev_date);
    }
}

/* date + number */

static void
date_plus_number_calc(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InRef_     PC_EV_DATE p_ev_date,
    _InRef_     PC_EV_DATA p_ev_data)
{
    p_ev_data_res->did_num = RPN_DAT_DATE;
    p_ev_data_res->arg.ev_date = *p_ev_date;

    if(EV_DATE_NULL != p_ev_date->date)
    {
        p_ev_data_res->arg.ev_date.date += p_ev_data->arg.integer;
    }
    else if(EV_TIME_NULL != p_ev_date->time)
    {
        p_ev_data_res->arg.ev_date.time += p_ev_data->arg.integer;

        ss_date_normalise(&p_ev_data_res->arg.ev_date);
    }
}

static void
date_plus_date_calc(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InRef_     PC_EV_DATE p_ev_date_1,
    _InRef_     PC_EV_DATE p_ev_date_2)
{
    p_ev_data_res->did_num = RPN_DAT_DATE;
    p_ev_data_res->arg.ev_date = *p_ev_date_1;

    if(EV_DATE_NULL != p_ev_date_2->date)
    {
        if(EV_DATE_NULL != p_ev_date_1->date)
            p_ev_data_res->arg.ev_date.date += p_ev_date_2->date;
        else
            p_ev_data_res->arg.ev_date.date  = p_ev_date_2->date;
    }

    if(EV_TIME_NULL != p_ev_date_2->time)
    {
        if(EV_TIME_NULL != p_ev_date_1->time)
            p_ev_data_res->arg.ev_date.time += p_ev_date_2->time;
        else
            p_ev_data_res->arg.ev_date.time  = p_ev_date_2->time;
    }

    ss_date_normalise(&p_ev_data_res->arg.ev_date);
}

PROC_EXEC_PROTO(c_add)
{
    exec_func_ignore_parms();

    if(two_nums_add_try(p_ev_data_res, args[0], args[1], FALSE))
        return;

    switch(args[0]->did_num)
    {
    case RPN_DAT_REAL:
        switch(args[1]->did_num)
        {
        case RPN_DAT_DATE:
            /* number + date */
            date_plus_real_number_calc(p_ev_data_res, &args[1]->arg.ev_date, args[0]->arg.fp);
            break;

        default:
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGTYPE);
            break;
        }
        break;

    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        switch(args[1]->did_num)
        {
        case RPN_DAT_DATE:
            /* number + date */
            date_plus_number_calc(p_ev_data_res, &args[1]->arg.ev_date, args[0]);
            break;

        default:
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGTYPE);
            break;
        }
        break;

    case RPN_DAT_DATE:
        switch(args[1]->did_num)
        {
        case RPN_DAT_REAL:
            date_plus_real_number_calc(p_ev_data_res, &args[0]->arg.ev_date, args[1]->arg.fp);
            break;

        case RPN_DAT_BOOL8:
        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            /* date + number */
            date_plus_number_calc(p_ev_data_res, &args[0]->arg.ev_date, args[1]);
            break;

        case RPN_DAT_DATE:
            /* date + date */
            date_plus_date_calc(p_ev_data_res, &args[0]->arg.ev_date, &args[1]->arg.ev_date);
            break;

        default:
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGTYPE);
            break;
        }

        break;

    default:
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGTYPE);
        break;
    }
}

/******************************************************************************
*
* VALUE res = a - b
*
******************************************************************************/

static void
date_minus_real_number_calc(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InRef_     PC_EV_DATE p_ev_date,
    _InVal_     F64 f64)
{
    F64 floor_value = real_floor(f64);
    S32 s32;

    p_ev_data_res->did_num = RPN_DAT_DATE;
    p_ev_data_res->arg.ev_date = *p_ev_date;

    if(EV_DATE_NULL != p_ev_date->date)
    {   /* a la real_to_integer_force */
        if((floor_value > (F64) S32_MAX) || (floor_value < (F64) -S32_MAX))
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_DATE);
            return;
        }

        s32 = (S32) floor_value;

        p_ev_data_res->arg.ev_date.date -= s32;

        if(f64 != floor_value)
        {   /* handle any fraction as time part */
            s32 = (S32) ((f64 - floor_value) * SECS_IN_24);

            if(EV_TIME_NULL != p_ev_date->time)
                p_ev_data_res->arg.ev_date.time -= s32;
            else
                p_ev_data_res->arg.ev_date.time  = s32;

            ss_date_normalise(&p_ev_data_res->arg.ev_date);
        }
    }
    else if(EV_TIME_NULL != p_ev_date->time)
    {
        if((floor_value> (F64) S32_MAX) || (floor_value < (F64) -S32_MAX))
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_DATE);
            return;
        }

        s32 = (S32) floor_value;

        p_ev_data_res->arg.ev_date.time -= s32;

        /* ignore any fraction */

        ss_date_normalise(&p_ev_data_res->arg.ev_date);
    }
}

/* date - number */

static void
date_minus_number_calc(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InRef_     PC_EV_DATE p_ev_date,
    _InRef_     PC_EV_DATA p_ev_data)
{
    p_ev_data_res->did_num = RPN_DAT_DATE;
    p_ev_data_res->arg.ev_date = *p_ev_date;

    if(EV_DATE_NULL != p_ev_date->date)
    {
        p_ev_data_res->arg.ev_date.date -= p_ev_data->arg.integer;
    }
    else if(EV_TIME_NULL != p_ev_date->time)
    {
        p_ev_data_res->arg.ev_date.time -= p_ev_data->arg.integer;

        ss_date_normalise(&p_ev_data_res->arg.ev_date);
    }
}

static void
date_minus_date_calc(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InRef_     PC_EV_DATE p_ev_date_1,
    _InRef_     PC_EV_DATE p_ev_date_2)
{
    /* date - number */
    if( (EV_DATE_NULL != p_ev_date_1->date) && (EV_TIME_NULL == p_ev_date_1->time) &&
        (EV_DATE_NULL != p_ev_date_2->date) && (EV_TIME_NULL == p_ev_date_2->time) )
    {   /* subtracting two pure dates */
        ev_data_set_integer(p_ev_data_res, p_ev_date_1->date - p_ev_date_2->date);
        return;
    }

    if( (EV_DATE_NULL == p_ev_date_1->date) && (EV_TIME_NULL != p_ev_date_1->time) &&
        (EV_DATE_NULL == p_ev_date_2->date) && (EV_TIME_NULL != p_ev_date_2->time) )
    {   /* subtracting two pure times */
        ev_data_set_integer(p_ev_data_res, p_ev_date_1->time - p_ev_date_2->time);
        return;
    }

    /* subtracting two mixed date/times */
    p_ev_data_res->did_num = RPN_DAT_DATE;
    p_ev_data_res->arg.ev_date = *p_ev_date_1;

    if(EV_DATE_NULL != p_ev_date_2->date)
    {
        if(EV_DATE_NULL != p_ev_date_1->date)
            p_ev_data_res->arg.ev_date.date -=  p_ev_date_2->date;
        else
            p_ev_data_res->arg.ev_date.date  = -p_ev_date_2->date;
    }

    if(EV_TIME_NULL != p_ev_date_2->time)
    {
        if(EV_TIME_NULL != p_ev_date_1->time)
            p_ev_data_res->arg.ev_date.time -=  p_ev_date_2->time;
        else
            p_ev_data_res->arg.ev_date.time  = -p_ev_date_2->time;
    }

    ss_date_normalise(&p_ev_data_res->arg.ev_date);
}

PROC_EXEC_PROTO(c_sub)
{
    exec_func_ignore_parms();

    if(two_nums_subtract_try(p_ev_data_res, args[0], args[1], FALSE))
        return;

    switch(args[0]->did_num)
    {
    case RPN_DAT_REAL:
    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        switch(args[1]->did_num)
        {
        /* number - date */
        case RPN_DAT_DATE:
            ev_data_set_error(p_ev_data_res, EVAL_ERR_UNEXDATE);
            break;

        default:
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGTYPE);
            break;
        }
        break;

    case RPN_DAT_DATE:
        switch(args[1]->did_num)
        {
        case RPN_DAT_REAL:
            date_minus_real_number_calc(p_ev_data_res, &args[0]->arg.ev_date, args[1]->arg.fp);
            break;

        case RPN_DAT_BOOL8:
        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            /* date - number */
            date_minus_number_calc(p_ev_data_res, &args[0]->arg.ev_date, args[1]);
            break;

        case RPN_DAT_DATE:
            /* date - date. SKS rationalised 11sep97 */
            date_minus_date_calc(p_ev_data_res, &args[0]->arg.ev_date, &args[1]->arg.ev_date);
            break;

        default:
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGTYPE);
            break;
        }

        break;

    default:
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGTYPE);
        break;
    }
}

/******************************************************************************
*
* VALUE res = a / b
*
******************************************************************************/

PROC_EXEC_PROTO(c_div)
{
    exec_func_ignore_parms();

    if(!two_nums_divide_try(p_ev_data_res, args[0], args[1], FALSE))
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGTYPE);
}

/******************************************************************************
*
* NUMBER res = a ^ b
*
* NUMBER power(a, b) is equivalent OpenDocument / Microsoft Excel function
*
******************************************************************************/

PROC_EXEC_PROTO(c_power)
{
    const F64 a = args[0]->arg.fp;
    const F64 b = args[1]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real_ti(p_ev_data_res, pow(a, b));

    if(errno /* == EDOM */)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);

        /* test for negative numbers to power of 1/odd number (odd roots are OK) */
        if((a < 0.0) && (b <= 1.0))
        {
            EV_DATA test;

            ev_data_set_real(&test, 1.0 / b);

            if(real_to_integer_try(&test))
                ev_data_set_real_ti(p_ev_data_res, -(pow(-(a), b)));
        }
    }
}

/******************************************************************************
*
* BOOLEAN res = a | b
*
******************************************************************************/

PROC_EXEC_PROTO(c_or)
{
    BOOL a = (args[0]->arg.integer != 0);
    BOOL b = (args[1]->arg.integer != 0);
    BOOL or_result = a || b;

    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, or_result);
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
    c_join(args, n_args, p_ev_data_res, p_cur_slr);
}

/******************************************************************************
*
* BOOLEAN res = a == b
*
******************************************************************************/

PROC_EXEC_PROTO(c_eq)
{
    BOOL eq_result = (ss_data_compare(args[0], args[1], TRUE, TRUE) == 0);

    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, eq_result);
}

/******************************************************************************
*
* BOOLEAN res = a > b
*
******************************************************************************/

PROC_EXEC_PROTO(c_gt)
{
    BOOL gt_result = (ss_data_compare(args[0], args[1], TRUE, TRUE) > 0);

    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, gt_result);
}

/******************************************************************************
*
* BOOLEAN res = a >= b
*
******************************************************************************/

PROC_EXEC_PROTO(c_gteq)
{
    BOOL gteq_result = (ss_data_compare(args[0], args[1], TRUE, TRUE) >= 0);

    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, gteq_result);
}

/******************************************************************************
*
* BOOLEAN res = a < b
*
******************************************************************************/

PROC_EXEC_PROTO(c_lt)
{
    BOOL lt_result = (ss_data_compare(args[0], args[1], TRUE, TRUE) < 0);

    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, lt_result);
}

/******************************************************************************
*
* BOOLEAN res = a <= b
*
******************************************************************************/

PROC_EXEC_PROTO(c_lteq)
{
    BOOL lteq_result = (ss_data_compare(args[0], args[1], TRUE, TRUE) <= 0);

    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, lteq_result);
}

/******************************************************************************
*
* BOOLEAN res = a != b
*
******************************************************************************/

PROC_EXEC_PROTO(c_neq)
{
    BOOL neq_result = (ss_data_compare(args[0], args[1], TRUE, TRUE) != 0);

    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, neq_result);
}

/******************************************************************************
*
* VALUE if(num, true_res, false_res)
*
******************************************************************************/

PROC_EXEC_PROTO(c_if)
{
    exec_func_ignore_parms();

    if(args[0]->arg.boolean)
        status_assert(ss_data_resource_copy(p_ev_data_res, args[1]));
    else if(n_args > 2)
        status_assert(ss_data_resource_copy(p_ev_data_res, args[2]));
    else
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NA);
}

/* end of ev_exec.c */
