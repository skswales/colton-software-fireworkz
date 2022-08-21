/* ev_math.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Mathematical function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

/******************************************************************************
*
* computes the product of all integers in the range [start..end] inclusive
*
* start, end assumed to be +ve, non-zero and start <= end
*
******************************************************************************/

extern void
product_between_calc(
    _InoutRef_  P_EV_DATA p_ev_data_res, /* denotes integer or fp; may return integer or fp */
    _InVal_     S32 start,
    _InVal_     S32 end)
{
    S32 i = start + 1;

    if(RPN_DAT_REAL == p_ev_data_res->did_num)
    {
        p_ev_data_res->arg.fp = start;
    }
    else
    {
        /* a small lookup table saves some time for stats functions and handles the exceptions anyway */
        static const S32 factorial_n[] =
        {
        /*0!*/  1,
        /*1!*/  1,
        /*2!*/  2,
        /*3!*/  2*3,
        /*4!*/  2*3*4,
        /*5!*/  2*3*4*5,
        /*6!*/  2*3*4*5*6,
        /*7!*/  2*3*4*5*6*7,
        /*8!*/  2*3*4*5*6*7*8,
        /*9!*/  2*3*4*5*6*7*8*9,
        /*10!*/ 2*3*4*5*6*7*8*9*10,
        /*11!*/ 2*3*4*5*6*7*8*9*10*11,
        /*12!*/ 2*3*4*5*6*7*8*9*10*11*12
        };

        p_ev_data_res->arg.integer = start;

        if(1 == start) /*even for end==0*/
        {
            if((U32) end < elemof32(factorial_n))
            {
                assert(end >= 0);
                p_ev_data_res->arg.integer = factorial_n[end];
                return;
            }

            i = elemof32(factorial_n) - 1; /* last element is useful to save many multiplications */
            p_ev_data_res->arg.integer = factorial_n[i];
            ++i;
        }

        /* keep product in integer where possible */
        for(; i <= end; ++i)
        {
            UMUL64_RESULT result;

            umul64(p_ev_data_res->arg.integer, i, &result);

            if((0 == result.HighPart) && (0 == (result.LowPart & 0x80000000U)))
            {   /* result still fits in integer */
                p_ev_data_res->arg.integer = (S32) result.LowPart;
                continue;
            }

            /* have to go to fp now result has outgrown integer and retry the multiply */
            ev_data_set_real(p_ev_data_res, (F64) p_ev_data_res->arg.integer);
            break;
        }
    }

    /* NB leave p_ev_data_res->did_num undisturbed if not RPN_DAT_REAL for caller to sort out */
    if(RPN_DAT_REAL != p_ev_data_res->did_num)
        return;

    /* finish off in fp if needed */
    for(; i <= end; ++i)
    {
        p_ev_data_res->arg.fp *= i;
    }
}

_Check_return_
extern STATUS
status_from_errno(void)
{
    switch(errno)
    {
    default:        return(STATUS_OK);
    case EDOM:      return(EVAL_ERR_ARGRANGE);
    case ERANGE:    return(EVAL_ERR_OUTOFRANGE);
    }
}

/******************************************************************************
*
* Mathematical functions
*
******************************************************************************/

/******************************************************************************
*
* NUMBER abs(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_abs)
{
    exec_func_ignore_parms();

    if(RPN_DAT_REAL == args[0]->did_num)
        ev_data_set_real(p_ev_data_res, fabs(args[0]->arg.fp));
    else
        ev_data_set_integer(p_ev_data_res, abs(args[0]->arg.integer));
}

/******************************************************************************
*
* REAL exp(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_exp)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, exp(number));

    /* exp() overflowed? - don't test for underflow case */
    if(p_ev_data_res->arg.fp == HUGE_VAL)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_OUTOFRANGE);
}

/******************************************************************************
*
* NUMBER fact(n)
*
******************************************************************************/

extern void
factorial_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return integer or fp or error */
    _InVal_     S32 n)
{
    if((n < 0) || (n > 170)) /* SKS maximum factorial that will fit in F64 */
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    /* assume result will be integer to start with */
    p_ev_data_out->did_num = RPN_DAT_WORD32;

    /* function will go to fp as necessary */
    product_between_calc(p_ev_data_out, 1, n); /* n==0 is handled by that */ /* may return integer or fp */
}

PROC_EXEC_PROTO(c_fact)
{
    const S32 n = args[0]->arg.integer;

    exec_func_ignore_parms();

    factorial_calc(p_ev_data_res, n); /* may return integer or fp or error */

    if(RPN_DAT_WORD32 == p_ev_data_res->did_num)
        p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
}

/******************************************************************************
*
* NUMBER int(number)
*
* NB truncates towards zero - this is unlike OpenDocument and Microsoft Excel
* but is what Lotus 1-2-3 and PipeDream did
*
******************************************************************************/

/* SKS 06oct97 for INT() function try rounding an ickle bit
 * so INT((0.06-0.04)/0.01) is 2 not 1
 * and now dec14 INT((0.06-0.02)/2E-6) is 20000 not 19999
 * which is different to naive real_trunc()
 */

PROC_EXEC_PROTO(c_int)
{
    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_REAL:
        ev_data_set_real_ti(p_ev_data_res, real_trunc(args[0]->arg.fp));
        return;

    case RPN_DAT_DATE:
        /* Convert just the date component to a largely Excel-compatible serial number */
        if(EV_DATE_NULL != args[0]->arg.ev_date.date)
            ev_data_set_integer(p_ev_data_res, ss_dateval_to_serial_number(&args[0]->arg.ev_date.date)); /* ignore any time component */
        else
            ev_data_set_integer(p_ev_data_res, 0); /* ignore any time component, and this is a pure time value */
        return;

    default:
        *p_ev_data_res = *(args[0]); /*NOP*/
        return;
    }
}

/******************************************************************************
*
* REAL ln(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_ln)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, log(number));

    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_LOG);
}

/******************************************************************************
*
* NUMBER mod(a, b)
*
******************************************************************************/

PROC_EXEC_PROTO(c_mod)
{
    exec_func_ignore_parms();

    switch(two_nums_type_match(args[0], args[1], FALSE)) /* FALSE is OK as the result is always smaller if TWO_INTS */
    {
    case TWO_INTS:
        {
        const S32 s32_a = args[0]->arg.integer;
        const S32 s32_b = args[1]->arg.integer;
        div_t d;
        S32 s32_mod_result;

        /* SKS after PD 4.11 03feb92 - gave FP error not zero if trap taken */
        if(0 == s32_b)
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
            return;
        }

        d = div((int) s32_a, (int) s32_b);

        s32_mod_result = d.rem; /* remainder has the same sign as the dividend */

        ev_data_set_integer(p_ev_data_res, s32_mod_result);
        break;
        }

    case TWO_REALS:
        {
        const F64 f64_a = args[0]->arg.fp;
        const F64 f64_b = args[1]->arg.fp;
        F64 f64_mod_result;

        errno = 0;

        f64_mod_result = fmod(f64_a, f64_b); /* remainder has the same sign as the dividend */

        ev_data_set_real_ti(p_ev_data_res, f64_mod_result);

        /* would have divided by zero? */
        if(errno /* == EDOM */)
            ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);

        break;
        }
    }
}

/* a little note from SKS:

 * it seems that fmod(a, b) guarantees the sign of the result
 * to be the same sign as a, whereas integer ops give an
 * implementation defined sign for both a / b and a % b.
*/

/******************************************************************************
*
* NUMBER sgn(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sgn)
{
    const F64 number = args[0]->arg.fp;
    S32 sgn_result;

    exec_func_ignore_parms();

    if(number > F64_MIN)
        sgn_result = 1;
    else if(number < -F64_MIN)
        sgn_result = -1;
    else
        sgn_result = 0;

    ev_data_set_integer(p_ev_data_res, sgn_result);
}

/******************************************************************************
*
* REAL sqr(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sqr)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, sqrt(number));

    if(errno /* == EDOM */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NEG_ROOT);
}

/* end of ev_math.c */
