/* ev_arith.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Split out from ev_help.c */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

/******************************************************************************
*
* p_ss_data_res = error iff either p_ss_data_1 or p_ss_data_2 are error
*
******************************************************************************/

_Check_return_ _Success_(return)
static bool
two_nums_bop_propagate_errors(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    PC_SS_DATA p_ss_data_err = NULL;

    if(ss_data_is_error(p_ss_data_2))
        p_ss_data_err = p_ss_data_2;
    if(ss_data_is_error(p_ss_data_1))   /* no else - eliminates a branch */
        p_ss_data_err = p_ss_data_1;    /* any error from data_1 has priority */

    if(NULL == p_ss_data_err)
        return(false);

    *p_ss_data_res = *p_ss_data_err;
    return(true);
}

/******************************************************************************
*
* go via 64 bits to handle potential integer overflow
*
******************************************************************************/

_Check_return_ _Success_(return)
static inline bool
ss_data_try_int64_as_integer(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     int64_t int64)
{
#if 1
    if(0 != ((S32) (int64 >> 32) - (((S32) int64) >> 31)))
    {
        /* top word and MSB of low word of result all zeros (+ve) or all ones (-ve) -> still fits in widest integer */
        ss_data_set_integer(p_ss_data_res, (S32) int64);
        return(true);
    }

    return(false);
#else
    switch((S32) (int64 >> 31))
    {
    case -1:
    case 0:
        /* top word and MSB of low word of result all zeros (+ve) or all ones (-ve) -> still fits in widest integer */
        ss_data_set_integer(p_ss_data_res, (S32) int64);
        return(true);

    default:
        return(false);
    }
#endif
}

/******************************************************************************
*
* p_ss_data_res = p_ss_data_1 + p_ss_data_2
*
******************************************************************************/

_Check_return_
static bool
two_nums_add_try_two_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    ss_data_set_real(p_ss_data_res, ss_data_get_real(p_ss_data_1) + ss_data_get_real(p_ss_data_2));

    return(true);
}

_Check_return_
static inline bool
two_nums_add_try_two_integers(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    ss_data_set_integer(p_ss_data_res, ss_data_get_integer(p_ss_data_1) + ss_data_get_integer(p_ss_data_2));

    return(true);
}

_Check_return_
static inline bool
two_nums_add_try_two_integers_WORD32(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    INT64_WITH_INT32_OVERFLOW result;

    ss_data_set_integer(p_ss_data_res, int32_add_check_overflow(ss_data_get_integer(p_ss_data_1), ss_data_get_integer(p_ss_data_2), &result));

    /* result would overflow widest integer? */
    if(result.f_overflow)
        ss_data_set_real(p_ss_data_res, (F64) result.int64_result);

    return(true);
}

_Check_return_ _Success_(return)
extern bool
two_nums_add_try(
    _OutRef_    P_SS_DATA p_ss_data_res, /* does not propagate errors, that's up to the caller */
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    const enum two_nums_type_match_result match = two_nums_type_match(p_ss_data_1, p_ss_data_2);

    if(TWO_INTEGERS == match)
        return(two_nums_add_try_two_integers(p_ss_data_res, p_ss_data_1, p_ss_data_2));

    if(TWO_INTEGERS_WORD32 == match)
        return(two_nums_add_try_two_integers_WORD32(p_ss_data_res, p_ss_data_1, p_ss_data_2)); /* handle potential overflow */

    if(TWO_REALS == match)
        return(two_nums_add_try_two_reals(p_ss_data_res, p_ss_data_1, p_ss_data_2));

    return(false);
}

_Check_return_ _Success_(return)
extern bool
two_nums_add_propagate_error(
    _OutRef_    P_SS_DATA p_ss_data_res, /* propagate errors */
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    if(two_nums_add_try(p_ss_data_res, p_ss_data_1, p_ss_data_2))
        return(true);

    return(two_nums_bop_propagate_errors(p_ss_data_res, p_ss_data_1, p_ss_data_2));
}

/* see also ss_data_add() */

/******************************************************************************
*
* p_ss_data_res = p_ss_data_1 - p_ss_data_2
*
******************************************************************************/

_Check_return_
static bool
two_nums_subtract_try_two_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    ss_data_set_real(p_ss_data_res, ss_data_get_real(p_ss_data_1) - ss_data_get_real(p_ss_data_2));

    return(true);
}

_Check_return_
static inline bool
two_nums_subtract_try_two_integers(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    ss_data_set_integer(p_ss_data_res, ss_data_get_integer(p_ss_data_1) - ss_data_get_integer(p_ss_data_2));

    return(true);
}

_Check_return_
static inline bool
two_nums_subtract_try_two_integers_WORD32( /* deal with potential overflow */
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    INT64_WITH_INT32_OVERFLOW result;

    ss_data_set_integer(p_ss_data_res, int32_subtract_check_overflow(ss_data_get_integer(p_ss_data_1), ss_data_get_integer(p_ss_data_2), &result));

    /* result would overflow widest integer? */
    if(result.f_overflow)
        ss_data_set_real(p_ss_data_res, (F64) result.int64_result);

    return(true);
}

_Check_return_ _Success_(return)
extern bool
two_nums_subtract_try(
    _OutRef_    P_SS_DATA p_ss_data_res, /* does not propagate errors, that's up to the caller */
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    const enum two_nums_type_match_result match = two_nums_type_match(p_ss_data_1, p_ss_data_2);

    if(TWO_INTEGERS == match)
        return(two_nums_subtract_try_two_integers(p_ss_data_res, p_ss_data_1, p_ss_data_2));

    if(TWO_INTEGERS_WORD32 == match)
        return(two_nums_subtract_try_two_integers_WORD32(p_ss_data_res, p_ss_data_1, p_ss_data_2));

    if(TWO_REALS == match)
        return(two_nums_subtract_try_two_reals(p_ss_data_res, p_ss_data_1, p_ss_data_2));

    return(false);
}

_Check_return_ _Success_(return)
extern bool
two_nums_subtract_propagate_error(
    _OutRef_    P_SS_DATA p_ss_data_res, /* propagate errors */
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    if(two_nums_subtract_try(p_ss_data_res, p_ss_data_1, p_ss_data_2))
        return(true);

    return(two_nums_bop_propagate_errors(p_ss_data_res, p_ss_data_1, p_ss_data_2));
}

/* see also ss_data_subtract() */

/******************************************************************************
*
* p_ss_data_res = p_ss_data_1 * p_ss_data_2
*
******************************************************************************/

_Check_return_
static bool
two_nums_multiply_try_two_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    ss_data_set_real(p_ss_data_res, ss_data_get_real(p_ss_data_1) * ss_data_get_real(p_ss_data_2));

    return(true);
}

_Check_return_
static inline bool
two_nums_multiply_try_two_integers(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    ss_data_set_integer(p_ss_data_res, ss_data_get_integer(p_ss_data_1) * ss_data_get_integer(p_ss_data_2));

    return(true);
}

_Check_return_
static inline bool
two_nums_multiply_try_two_integers_WORD32( /* deal with potential overflow */
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    INT64_WITH_INT32_OVERFLOW result;

    ss_data_set_integer(p_ss_data_res, int32_multiply_check_overflow(ss_data_get_integer(p_ss_data_1), ss_data_get_integer(p_ss_data_2), &result));

    /* result would overflow widest integer? */
    if(result.f_overflow)
#if 1
        /* seems OK, but watch out for any inexact exceptions as we might need 61 bits of mantissa! */
        ss_data_set_real(p_ss_data_res, (F64) result.int64_result);
#else
        /* safer alternative */
        ss_data_set_real(p_ss_data_res, (F64) ss_data_get_integer(p_ss_data_1) * ss_data_get_integer(p_ss_data_2));
#endif

    return(true);
}

_Check_return_ _Success_(return)
extern bool
two_nums_multiply_try(
    _OutRef_    P_SS_DATA p_ss_data_res, /* does not propagate errors, that's up to the caller */
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    const enum two_nums_type_match_result match = two_nums_type_match(p_ss_data_1, p_ss_data_2);

    if(TWO_INTEGERS == match)
        return(two_nums_multiply_try_two_integers(p_ss_data_res, p_ss_data_1, p_ss_data_2));

    if(TWO_INTEGERS_WORD32 == match)
        return(two_nums_multiply_try_two_integers_WORD32(p_ss_data_res, p_ss_data_1, p_ss_data_2)); /* handle potential overflow */

    if(TWO_REALS == match)
        return(two_nums_multiply_try_two_reals(p_ss_data_res, p_ss_data_1, p_ss_data_2));

    return(false);
}

_Check_return_ _Success_(return)
extern bool
two_nums_multiply_propagate_error(
    _OutRef_    P_SS_DATA p_ss_data_res, /* propagate errors */
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    if(two_nums_multiply_try(p_ss_data_res, p_ss_data_1, p_ss_data_2))
        return(true);

    return(two_nums_bop_propagate_errors(p_ss_data_res, p_ss_data_1, p_ss_data_2));
}

/* see also ss_data_multiply() */

/******************************************************************************
*
* p_ss_data_res = p_ss_data_1 / p_ss_data_2
*
******************************************************************************/

_Check_return_
static bool inline
f64_is_p_divisor_too_small(
    _InRef_     PC_F64 p_divisor)
{
    if(isgreater(fabs(*p_divisor), F64_MIN))
        return(false);/* attempt the division, should be OK */

    if(global_preferences.ss_calc_try_IEEE_maths)
        return(false); /* attempt the division, cope with ±inf or NaN result */

    return(true); /* don't attempt the division, flag a DIV/0 error */
}

_Check_return_
extern bool
f64_is_divisor_too_small(
    _InVal_     F64 divisor)
{
    return(f64_is_p_divisor_too_small(&divisor));
}

_Check_return_
static bool
calc_divide_by_zero(
    _OutRef_    P_SS_DATA p_ss_data_res)
{
    ss_data_set_error(p_ss_data_res, EVAL_ERR_DIVIDEBY0);
    return(true);
}

#define FASTER_REAL_DIVIDE 1

#if defined(FASTER_REAL_DIVIDE)

_Check_return_
static bool
calc_divide_two_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     PC_F64 p_dividend,
    _InVal_     PC_F64 p_divisor)
{
    if(f64_is_p_divisor_too_small(p_divisor))
        return(calc_divide_by_zero(p_ss_data_res));

    ss_data_set_real(p_ss_data_res, *p_dividend / *p_divisor);
    return(true);
}

#define two_nums_divide_try_two_reals(p_ss_data_res, pc_ss_data_1, pc_ss_data_2) \
    calc_divide_two_reals(p_ss_data_res, ss_data_get_pc_real(pc_ss_data_1), ss_data_get_pc_real(pc_ss_data_2))

#else /* !FASTER_REAL_DIVIDE */

_Check_return_
static bool
calc_divide_two_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     F64 dividend,
    _InVal_     F64 divisor)
{
    if(f64_is_divisor_too_small(divisor))
        return(calc_divide_by_zero(p_ss_data_res));

    ss_data_set_real(p_ss_data_res, dividend / divisor);
    return(true);
}

_Check_return_
static bool
two_nums_divide_try_two_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    const F64 dividend = ss_data_get_real(p_ss_data_1);
    const F64 divisor  = ss_data_get_real(p_ss_data_2);

    return(calc_divide_two_reals(p_ss_data_res, dividend, divisor));
}

#endif /* FASTER_REAL_DIVIDE */

_Check_return_
static bool
two_nums_divide_try_two_integers_as_reals(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     S32 s32_dividend,
    _InVal_     S32 s32_divisor)
{
    const F64 dividend = (F64) s32_dividend;
    const F64 divisor  = (F64) s32_divisor;

#if defined(FASTER_REAL_DIVIDE)
    return(calc_divide_two_reals(p_ss_data_res, &dividend, &divisor));
#else
    return(calc_divide_two_reals(p_ss_data_res, dividend, divisor));
#endif /* FASTER_REAL_DIVIDE */
}

_Check_return_
static bool
two_nums_divide_try_two_integers(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    const S32 dividend = ss_data_get_integer(p_ss_data_1);
    const S32 divisor  = ss_data_get_integer(p_ss_data_2);

    if(0 != divisor)
    {
        const S32 quotient  = dividend / divisor;
        const S32 remainder = dividend % divisor;

        if(0 == remainder)
        {
            ss_data_set_integer(p_ss_data_res, quotient);
            return(true);
        }
    }

    return(two_nums_divide_try_two_integers_as_reals(p_ss_data_res, dividend, divisor));
}

_Check_return_ _Success_(return)
extern bool
two_nums_divide_try(
    _OutRef_    P_SS_DATA p_ss_data_res, /* does not propagate errors, that's up to the caller */
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    const enum two_nums_type_match_result match = two_nums_type_match(p_ss_data_1, p_ss_data_2); /* the result is always smaller if integer args - no potential overflow */

    if( (TWO_INTEGERS == match) || (TWO_INTEGERS_WORD32 == match) )
        return(two_nums_divide_try_two_integers(p_ss_data_res, p_ss_data_1, p_ss_data_2));

    if(TWO_REALS == match)
        return(two_nums_divide_try_two_reals(p_ss_data_res, p_ss_data_1, p_ss_data_2));

    return(false);
}

_Check_return_ _Success_(return)
extern bool
two_nums_divide_propagate_error(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    if(two_nums_divide_try(p_ss_data_res, p_ss_data_1, p_ss_data_2))
        return(true);

    return(two_nums_bop_propagate_errors(p_ss_data_res, p_ss_data_1, p_ss_data_2));
}

/* see also ss_data_divide() */

/* end of ev_arith.c */
