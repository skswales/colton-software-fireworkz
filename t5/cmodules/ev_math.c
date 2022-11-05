/* ev_math.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

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
    _InoutRef_  P_SS_DATA p_ss_data_res, /* denotes integer or real; may return integer or real. NB must contain 'start' value as integer or real */
  /*_InVal_     S32 start,*/
    _InVal_     S32 end)
{
    S32 i;

    if(ss_data_is_real(p_ss_data_res))
    {
        const F64 f_start = ss_data_get_real(p_ss_data_res);

        i = (S32) f_start + 1;
    }
    else
    {
        const S32 i_start = ss_data_get_integer(p_ss_data_res);

        i = i_start + 1;

        if(1 == i_start) /*even for end==0*/
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

            assert(end >= 0);
            if((U32) end < elemof32(factorial_n))
            {
                p_ss_data_res->arg.integer = factorial_n[end];
                return;
            }

            i = elemof32(factorial_n) - 1; /* last element is useful to save many multiplications */
            p_ss_data_res->arg.integer = factorial_n[i];
            ++i;
        }

        /* keep product in integer where possible */
        while(i <= end)
        {   /* yes, the casts are important */
            /* ARM Norcroft C should generate a call to _ll_mulluu: "Create a 64-bit number by multiplying two uint32_t numbers" */
            const uint64_t u64 = (uint64_t) (uint32_t) p_ss_data_res->arg.integer * (uint32_t) i;

            if(((uint64_t) (int32_t) u64) == u64)
            {   /* want top word and MSB of low word to both be zero - if so, result still fits in signed integer */
                p_ss_data_res->arg.integer = (S32) (U32) u64;
                ++i;
                continue;
            }

            /* have to go to fp now result has outgrown integer and retry the multiply */
            ss_data_set_real(p_ss_data_res, (F64) ss_data_get_integer(p_ss_data_res));
            break;
        }

        /* NB leave p_ss_data_res' data_id undisturbed if not DATA_ID_REAL for caller to sort out */
        if(!ss_data_is_real(p_ss_data_res))
            return;
    }

    /* finish off in fp if needed */
    assert(ss_data_is_real(p_ss_data_res));
    while(i <= end)
    {
        ss_data_set_real(p_ss_data_res, ss_data_get_real(p_ss_data_res) * (F64) i);
        ++i;
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

    if(ss_data_is_real(args[0]))
        ss_data_set_real(p_ss_data_res, fabs(ss_data_get_real(args[0])));
    else
        ss_data_set_integer(p_ss_data_res, abs(ss_data_get_integer(args[0])));
}

/******************************************************************************
*
* REAL exp(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_exp)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    ss_data_set_real(p_ss_data_res, exp(number));

    /* exp() overflowed? - don't test for underflow case */
    if(ss_data_get_real(p_ss_data_res) == HUGE_VAL)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_OUTOFRANGE);
}

/******************************************************************************
*
* NUMBER fact(n)
*
******************************************************************************/

extern void
factorial_calc(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return integer or real or error */
    _InVal_     S32 n)
{
    if( (n < 0) || (n > 170) ) /* SKS maximum factorial that will fit in F64 */
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    /* assume result will be (widest) integer to start with; function will go to fp as necessary */
    ss_data_set_WORD32(p_ss_data_out, 1); /* start */
    product_between_calc(p_ss_data_out, /*1,*/ n); /* n==0 is handled by that */ /* may return integer or real */
}

PROC_EXEC_PROTO(c_fact)
{
    const S32 n = ss_data_get_integer(args[0]);

    exec_func_ignore_parms();

    factorial_calc(p_ss_data_res, n); /* may return integer or real or error */

    if(ss_data_is_integer(p_ss_data_res))
        ss_data_set_integer_size(p_ss_data_res);
}

/******************************************************************************
*
* NUMBER int(number)
*
* NB truncates towards zero - this is unlike OpenDocument and Microsoft Excel
* but is what Lotus 1-2-3 did and what PipeDream does too
*
******************************************************************************/

PROC_EXEC_PROTO(c_int)
{
    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_REAL:
        ss_data_set_real_try_integer(p_ss_data_res, real_trunc(ss_data_get_real(args[0])));
        return;

    case DATA_ID_DATE:
        /* Convert just the date component to a largely Excel-compatible serial number */
        if(SS_DATE_NULL != ss_data_get_date(args[0])->date)
            ss_data_set_integer(p_ss_data_res, ss_dateval_to_serial_number(ss_data_get_date(args[0])->date)); /* ignore any time component */
        else
            ss_data_set_integer(p_ss_data_res, 0); /* ignore any time component, and this is a pure time value */
        return;

    default:
        *p_ss_data_res = *(args[0]); /*NOP*/
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
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, log(number));

    if(errno /* == EDOM, ERANGE */)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_BAD_LOG);
}

/******************************************************************************
*
* NUMBER mod(a, b)
*
******************************************************************************/

static void
calc_mod_two_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     F64 f64_a,
    _InVal_     F64 f64_b)
{
    F64 f64_mod_result;

    errno = 0;

    f64_mod_result = fmod(f64_a, f64_b); /* remainder has the same sign as the dividend */

    ss_data_set_real_try_integer(p_ss_data_res, f64_mod_result);

    /* would have divided by zero? */
    if(!global_preferences.ss_calc_try_IEEE_maths)
        if(errno /* == EDOM */)
            ss_data_set_error(p_ss_data_res, EVAL_ERR_DIVIDEBY0);
}

static void
c_mod_two_integers(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA arg0,
    _InRef_     PC_SS_DATA arg1)
{
    const S32 s32_a = ss_data_get_integer(arg0);
    const S32 s32_b = ss_data_get_integer(arg1);
    S32 s32_mod_result;

    if(0 != s32_b)
    {
        s32_mod_result = (s32_a % s32_b); /* remainder has the same sign as the dividend (C99) */

        ss_data_set_integer(p_ss_data_res, s32_mod_result);
        return;
    }

    calc_mod_two_reals(p_ss_data_res, (F64) s32_a, (F64) s32_b);
}

static void
c_mod_two_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA arg0,
    _InRef_     PC_SS_DATA arg1)
{
    const F64 f64_a = ss_data_get_real(arg0);
    const F64 f64_b = ss_data_get_real(arg1);

    calc_mod_two_reals(p_ss_data_res, f64_a, f64_b);
}

PROC_EXEC_PROTO(c_mod)
{
    exec_func_ignore_parms();

    switch(two_nums_type_match(args[0], args[1]))
    {
    case TWO_INTEGERS:
    case TWO_INTEGERS_WORD32: /* NB the result is always smaller - no potential overflow worries */
        c_mod_two_integers(p_ss_data_res, args[0], args[1]);
        return;

    case TWO_REALS:
        c_mod_two_reals(p_ss_data_res, args[0], args[1]);
        return;

    default: default_unhandled();
        return;
    }
}

/******************************************************************************
*
* NUMBER sgn(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sgn)
{
    const F64 number = ss_data_get_real(args[0]);
    S32 sgn_result;

    exec_func_ignore_parms();

    if(number > F64_MIN)
        sgn_result = 1;
    else if(number < -F64_MIN)
        sgn_result = -1;
    else
        sgn_result = 0;

    ss_data_set_integer(p_ss_data_res, sgn_result);
}

/******************************************************************************
*
* REAL sqr(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sqr)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, sqrt(number));

    if(errno /* == EDOM */)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_NEG_ROOT);
}

/* end of ev_math.c */
