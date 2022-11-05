/* ev_mtri.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Trigonometrical function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "cmodules/mathxtra.h"

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h" /* for _pi */
#endif

/******************************************************************************
*
* Trigonometrical functions
*
******************************************************************************/

/******************************************************************************
*
* REAL acos(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acos)
{
    F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, acos(number));

    /* input of magnitude greater than 1 invalid */
    if(errno /* == EDOM */)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
}

/******************************************************************************
*
* REAL acosec(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acosec)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, mx_acosec(number));

    /* input of magnitude less than 1 invalid */
    if(errno /* == EDOM */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL acosech(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acosech)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, mx_acosech(number));

    /* input of zero invalid */
    /* input of magnitude near zero causes overflow */
    if(errno /* == EDOM, ERANGE */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL acosh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acosh)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, acosh(number));

    /* input less than 1 invalid */
    /* large positive input causes overflow ***in current algorithm*** */
    if(errno /* == EDOM, ERANGE */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL acot(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acot)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    ss_data_set_real(p_ss_data_res, mx_acot(number));

    /* no error cases */
}

/******************************************************************************
*
* REAL acoth(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acoth)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, mx_acoth(number));

    /* input of magnitude less than 1 invalid */
    /* input of magnitude near 1 causes overflow */
    if(errno /* == EDOM */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL asec(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_asec)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, mx_asec(number));

    /* input of values less than 1 invalid */
    if(errno /* == EDOM */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL asech(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_asech)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, mx_asech(number));

    /* negative input or positive values greater than one invalid */
    /* input of zero or small positive value causes overflow */
    if(errno /* == EDOM, ERANGE */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL asin(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_asin)
{
    F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, asin(number));

    /* input of magnitude greater than 1 invalid */
    if(errno /* == EDOM */)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
}

/******************************************************************************
*
* REAL asinh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_asinh)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    ss_data_set_real(p_ss_data_res, asinh(number));

    /* no error case */
}

/******************************************************************************
*
* REAL atan(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_atan)
{
    F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    ss_data_set_real(p_ss_data_res, atan(number));

    /* no error cases */
}

/******************************************************************************
*
* REAL atanh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_atanh)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, atanh(number));

    /* input of magnitude 1 or greater invalid */
    /* input of near magnitude 1 causes overflow */
    if(errno /* == EDOM, ERANGE */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL atan_2(a, b)
*
******************************************************************************/

PROC_EXEC_PROTO(c_atan_2)
{
    const F64 a = ss_data_get_real(args[0]);
    const F64 b = ss_data_get_real(args[1]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, atan2(b, a));

    /* both input args zero? */
    if(errno /* == EDOM */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL cos(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cos)
{
    F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, cos(number));

    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ACCURACY_LOST);
}

/******************************************************************************
*
* REAL cosec(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cosec)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, mx_cosec(number));

    /* various periodic input yields infinity or overflows */
    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL cosech(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cosech)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, mx_cosech(number));

    /* zero | small magnitude input -> infinity | overflows */
    if(errno /* == EDOM, ERANGE */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL cosh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cosh)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, cosh(number));

    /* large magnitude input causes overflow */
    if(errno /* == ERANGE */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL cot(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cot)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, mx_cot(number));

    /* various periodic input yields infinity or overflows */
    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL coth(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_coth)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, mx_coth(number));

    /* zero | small magnitude input -> infinity | overflows */
    if(errno /* == EDOM, ERANGE */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL deg(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_deg)
{
    F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    ss_data_set_real(p_ss_data_res, number * _degrees_per_radian);
}

/******************************************************************************
*
* REAL pi
*
******************************************************************************/

PROC_EXEC_PROTO(c_pi)
{
    exec_func_ignore_parms();

    UNREFERENCED_PARAMETER(args);

    ss_data_set_real(p_ss_data_res, _pi);
}

/******************************************************************************
*
* REAL rad(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rad)
{
    F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    ss_data_set_real(p_ss_data_res, number * _radians_per_degree);
}

/******************************************************************************
*
* REAL sec(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sec)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, mx_sec(number));

    /* various periodic input yields infinity or overflows */
    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL sech(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sech)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    ss_data_set_real(p_ss_data_res, mx_sech(number));

    /* no error cases */
}

/******************************************************************************
*
* REAL sin(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sin)
{
    F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, sin(number));

    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ACCURACY_LOST);
}

/******************************************************************************
*
* REAL sinh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sinh)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, sinh(number));

    /* large magnitude input causes overflow */
    if(errno /* == ERANGE */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL tan(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_tan)
{
    F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, tan(number));

    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ACCURACY_LOST);
}

/******************************************************************************
*
* REAL tanh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_tanh)
{
    const F64 number = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    ss_data_set_real(p_ss_data_res, tanh(number));

    /* no error cases */
}

/* end of ev_mtri.c */
