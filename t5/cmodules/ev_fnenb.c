/* ev_fnenb.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2019-2020 Stuart Swales */

/* Engineering function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

/******************************************************************************
*
* Engineering functions - bitwise operations
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
static STATUS
getnumberforbitop(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     P_SS_DATA p_ss_data,
    _OutRef_    P_S64 p_s64)
{
    F64 f64 = ss_data_get_real(p_ss_data);

    *p_s64 = 0;

    if((f64 > ((1LL << 48)-1)) || (f64 < (-(1LL << 48))))
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

    *p_s64 = (S64) f64;

    return(STATUS_OK);
}

static void
setresultforbitop(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InVal_     S64 s64)
{
    /* check that we will not overflow forty-eight bits */
    if(0 != (s64 >> 48))
    {
        ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    ss_data_set_real_try_integer(p_ss_data_res, (F64) s64);
}

/******************************************************************************
*
* NUMBER bitand(number1, number2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_bitand)
{
    S64 s64_in_1, s64_in_2, s64_res;

    exec_func_ignore_parms();

    if(status_fail(getnumberforbitop(p_ss_data_res, args[0], &s64_in_1)))
        return;
    if(status_fail(getnumberforbitop(p_ss_data_res, args[1], &s64_in_2)))
        return;

    s64_res = s64_in_1 & s64_in_2;

    setresultforbitop(p_ss_data_res, s64_res);
}

/******************************************************************************
*
* NUMBER bitlshift(number, shift_amount)
*
******************************************************************************/

static void
bitlshift(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InVal_     F64 f64, /* already INT() */
    _InVal_     S32 shift_amount /*+/-*/)
{
    F64 multiplier;
    F64 bitlshift_result;

    if(isless(f64, 0.0))
    {
        ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    errno = 0;
    multiplier = ldexp(1.0, shift_amount);
    if(errno)
    {
        ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    bitlshift_result = f64 * multiplier;

    if(shift_amount < 0)
        bitlshift_result = real_floor(bitlshift_result);

    ss_data_set_real_try_integer(p_ss_data_res, bitlshift_result);
}

PROC_EXEC_PROTO(c_bitlshift)
{
    const F64 f64 = arg_get_real_INT(args[0]);
    const S32 shift_amount = ss_data_get_integer(args[1]);

    exec_func_ignore_parms();

    bitlshift(p_ss_data_res, f64, shift_amount);
}

/******************************************************************************
*
* NUMBER bitor(number1, number2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_bitor)
{
    S64 s64_in_1, s64_in_2, s64_res;

    exec_func_ignore_parms();

    if(status_fail(getnumberforbitop(p_ss_data_res, args[0], &s64_in_1)))
        return;
    if(status_fail(getnumberforbitop(p_ss_data_res, args[1], &s64_in_2)))
        return;

    s64_res = s64_in_1 | s64_in_2;

    setresultforbitop(p_ss_data_res, s64_res);
}

/******************************************************************************
*
* NUMBER bitrshift(number, shift_amount)
*
******************************************************************************/

PROC_EXEC_PROTO(c_bitrshift)
{
    const F64 f64 = arg_get_real_INT(args[0]);
    const S32 shift_amount = ss_data_get_integer(args[1]);

    exec_func_ignore_parms();

    bitlshift(p_ss_data_res, f64, -shift_amount);
}

/******************************************************************************
*
* NUMBER bitxor(number1, number2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_bitxor)
{
    S64 s64_in_1, s64_in_2, s64_res;

    exec_func_ignore_parms();

    if(status_fail(getnumberforbitop(p_ss_data_res, args[0], &s64_in_1)))
        return;
    if(status_fail(getnumberforbitop(p_ss_data_res, args[1], &s64_in_2)))
        return;

    s64_res = s64_in_1 ^ s64_in_2;

    setresultforbitop(p_ss_data_res, s64_res);
}

/* end of ev_fnenb.c */
