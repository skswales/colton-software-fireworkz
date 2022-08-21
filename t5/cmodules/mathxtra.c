/* mathxtra.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Additional math routines */

/* SKS July 1991 */

/* ANSI C library provides:        sin        cos        tan
 *                                asin       acos       atan
 *
 *                                 sinh       cosh       tanh
 *
 *                                sqrt
 *
 *                                rand
 *
 * this module provides          cosec        sec        cot
 *                              acosec       asec       acot
 *
 *                               cosech       sech       coth
 *                              acosech      asech      acoth
 *
 *                                asinh      acosh      atanh
 *
 *                              square      hypot
*/

#include "common/gflags.h"

#include "cmodules/mathxtri.h"

#ifndef          __mathxtra_h
#include "cmodules/mathxtra.h"
#endif

#ifndef                    _mathnums_h
#include "cmodules/coltsoft/mathnums.h"
#endif

_Check_return_
static inline F64
mathxtra_domain_error(void)
{
    errno = EDOM;
    return(+F64_HUGE_VAL);
}

_Check_return_
static inline F64
mathxtra_range_error(
    _InVal_     F64 x)
{
    errno = ERANGE;
    return((x < 0) ? -F64_HUGE_VAL : +F64_HUGE_VAL);
}

#if RISCOS
#define F64_MIN_RECIP F64_MIN
#elif WINDOWS
#define F64_MIN_RECIP F64_ONE / F64_MAX
#endif

/******************************************************************************
*
*                T R I G O N O M E T R I C   F U N C T I O N S
*
******************************************************************************/

/* cosec(x) = 1 / sin(x); |cosec(x)| >= 1 (odd) */

_Check_return_
extern F64
mx_cosec(
    _InVal_     F64 x)
{
    /* trivial implementation */
    F64 y;

    y = sin(x);

    /* about to divide result by (near) zero? */
    if(fabs(y) < F64_MIN_RECIP)
        return(mathxtra_range_error(x));

    return(F64_ONE / y);
}

/* cot(x) = 1 / tan(x) (odd) */

_Check_return_
extern F64
mx_cot(
    _InVal_     F64 x)
{
    /* trivial implementation */
    F64 y;

    y = tan(x);

    /* about to divide result by (near) zero? */
    if(fabs(y) < F64_MIN_RECIP)
        return(mathxtra_range_error(x));

    return(F64_ONE / y);
}

/* sec(x) = 1 / cos(x); |sec(x)| >= 1 (even) */

_Check_return_
extern F64
mx_sec(
    _InVal_     F64 x)
{
    /* trivial implementation */
    F64 y;

    y = cos(x);

    /* about to divide result by (near) zero? */
    if(fabs(y) < F64_MIN_RECIP)
        return(mathxtra_range_error(x));

    return(F64_ONE / y);
}

/******************************************************************************
*
*        I N V E R S E   T R I G O N O M E T R I C   F U N C T I O N S
*
******************************************************************************/

/* acosec(x) = asin(1 / x); -pi/2 <= acosec(x) <= +pi/2 */

/* NB. |cosec(x)| >= 1 */

_Check_return_
extern F64
mx_acosec(
    _InVal_     F64 x)
{
    F64 y, ax;
    BOOL negative;

    negative = (x < 0);

    ax = (negative) ? -x : +x;

    if(ax == F64_ONE)
        return((negative) ? -_pi_div_two : +_pi_div_two);

    if(ax < F64_ONE)
        return(mathxtra_domain_error());

    y = asin(F64_ONE / x);

    return(y);
}

/* 1. acot(x) = atan(1 / x); -pi/2 <= acot(x) <= +pi/2 */

/* 2. acot(|x|) = pi/2 - atan(|x|) (symmetry of cot/tan about pi/4) */

_Check_return_
extern F64
mx_acot(
    _InVal_     F64 x)
{
    F64 y, ax;
    BOOL negative;

    negative = (x < 0);

    ax = (negative) ? -x : +x;

    if(ax == F64_ZERO)
        return(_pi_div_two);

    if(ax < F64_MIN_RECIP)
        return((negative) ? -_pi_div_two : +_pi_div_two);

    y = atan(F64_ONE / x);

    return(y);
}

/* asec(x) = acos(1 / x); 0 <= asec(x) <= +pi */

/* NB. |sec(x)| >= 1 */

_Check_return_
extern F64
mx_asec(
    _InVal_     F64 x)
{
    F64 y, ax;
    BOOL negative;

    negative = (x < 0);

    ax = (negative) ? -x : +x;

    if(ax == F64_ONE)
        return(F64_ZERO);

    if(ax < F64_ONE)
        return(mathxtra_domain_error());

    y = acos(F64_ONE / x);

    return(y);
}

/******************************************************************************
*
*                  H Y P E R B O L I C   F U N C T I O N S
*
******************************************************************************/

/******************************************************************************
*
* computes the hyperbolic cosecant of x.
*
* A domain error occurs if x is zero.
* A range error occurs if the magnitude of x is too small.
*
* Returns: the hyperbolic cosecant value.
*          if domain error; returns F64_HUGE_VAL.
*          if range error; returns -F64_HUGE_VAL or F64_HUGE_VAL depending
*          on the sign of the argument.
*
******************************************************************************/

_Check_return_
extern F64
mx_cosech(
    _InVal_     F64 x)
{
    F64 y, ax;
    BOOL negative;

    if(x == F64_ZERO)
        return(mathxtra_domain_error());

    negative = (x < F64_ZERO);

    ax = (negative) ? -x : x;

    if(ax < F64_MIN)
        return(mathxtra_range_error(x));

    y = F64_ONE / sinh(ax);

    return((negative) ? -y : +y);
}

/******************************************************************************
*
* computes the hyperbolic cotangent of x.
*
* A domain error occurs if x is zero.
* A range error occurs if the magnitude of x is too small.
*
* Returns: the hyperbolic cotangent value.
*          if domain error; returns F64_HUGE_VAL.
*          if range error; returns -F64_HUGE_VAL or F64_HUGE_VAL depending
*          on the sign of the argument.
*
******************************************************************************/

_Check_return_
extern F64
mx_coth(
    _InVal_     F64 x)
{
    F64 y, ax;
    BOOL negative;

    if(x == F64_ZERO)
        return(mathxtra_domain_error());

    negative = (x < F64_ZERO);

    ax = (negative) ? -x : x;

    if(ax < F64_MIN)
        return(mathxtra_range_error(x));

    y = F64_ONE / tanh(ax);

    return((negative) ? -y : +y);
}

/******************************************************************************
*
* computes the hyperbolic secant of x.
*
* Returns: the hyperbolic secant value.
*
******************************************************************************/

/* 1 / cosh(x) == 2 / (exp(x) + exp(-x)) */

/* symmetric about x=0, result always 0..+1 */

#define __sech_simple_arg ((F64_MANT_DIG / 2) * __LN_RADIX_BASE)

_Check_return_
extern F64
mx_sech(
    _InVal_     F64 x)
{
    F64 y, ax;
    BOOL negative;

    negative = (x < F64_ZERO);

    /* equality test can probably now use result from above */
    if(x == F64_ZERO)
        return(F64_ONE);

    ax = (negative) ? -x : x;

#ifdef __sech_simple_arg
    if(ax > __sech_simple_arg)
        /* small, nearing zero result, exp will set ERANGE on underflow? */
        y = F64_TWO * exp(-ax);
    else
#endif
        y = F64_ONE / cosh(ax);

    return(y);
}

/******************************************************************************
*
*         I N V E R S E   H Y P E R B O L I C   F U N C T I O N S
*
******************************************************************************/

/******************************************************************************
*
* computes the principal value of the inverse hyperbolic cosine of x.
*
* A domain error occurs for arguments not in the range 1 to +inf.
* A range error occurs if the magnitude of x is too large.
*
* Returns: the inverse hyperbolic cosine in the range 0 to +inf.
*          if domain error; returns -F64_HUGE_VAL or F64_HUGE_VAL depending
*          on the sign of the argument.
*          if range error; returns F64_HUGE_VAL
*
******************************************************************************/

/* ln(x + sqrt(x^2 - 1)) */

#define __acosh_max_arg F64_MAX_SQUAREABLE

_Check_return_
extern F64
mx_acosh(
    _InVal_     F64 x)
{
    if(x <= F64_ONE)
    {
        if(x == F64_ONE)
            return(F64_ZERO);

        return(mathxtra_domain_error());
    }

    if(x > __acosh_max_arg)
        return(mathxtra_range_error(x));

    /* another test could be done to test significance to just do log(2x) */
#ifdef __acosh_max_sig_arg
    if(x > __acosh_max_sig_arg)
#ifdef _ln_two
        return(log(x) + _ln_two);
#else
        return(log(TWO * x));
#endif
#endif

    return(log(x + sqrt((x * x) - F64_ONE)));
}

/******************************************************************************
*
* computes the inverse hyperbolic cosecant of x.
*
* A domain error occurs if x is zero.
* A range error occurs if the magnitude of x is too small.
*
* Returns: the inverse hyperbolic cosecant value.
*          if domain error; returns F64_HUGE_VAL.
*          if range error; returns -F64_HUGE_VAL or F64_HUGE_VAL depending
*          on the sign of the argument.
*
******************************************************************************/

/* ln((1/x) + sqrt((1/x)^2 + 1)) for |x| > 0 */

/* mustn't let (1/x)^2 overflow */
#define __acosech_min_arg (F64_MIN)
/* guess for now, will be something like FLT_RADIX**(FLT_MANT_DIG/2) */

_Check_return_
extern F64
mx_acosech(
    _InVal_     F64 x)
{
    F64 ax, y, invax;
    BOOL negative;

    negative = (x < F64_ZERO);

    ax = (negative) ? -x : x;

    if(ax < __acosech_min_arg)
    {
        if(ax == F64_ZERO)
            return(mathxtra_domain_error());

        return(mathxtra_range_error(x));
    }

    invax = F64_ONE / ax;

    /* another test could be done to test significance to just do log(2*invax) */

    y = log(invax + sqrt(invax * invax + F64_ONE));

    return((negative) ? -y : +y);
}

/******************************************************************************
*
* computes the inverse hyperbolic cotangent of x.
*
* A domain error occurs if the magnitude of x is less than one.
* A range error occurs if the magnitude of x is too small.
*
* Returns: the inverse hyperbolic cotangent value.
*          if domain error; returns -F64_HUGE_VAL or F64_HUGE_VAL depending
*          on the sign of the argument.
*          if range error; returns -F64_HUGE_VAL or F64_HUGE_VAL depending
*          on the sign of the argument.
*
******************************************************************************/

/* (1/2) * ln((x+1) / (x-1)) for |x| > 1 */

#define __acoth_min_arg (F64_ONE + F64_EPSILON)

_Check_return_
extern F64
mx_acoth(
    _InVal_     F64 x)
{
    F64 y, ax;
    BOOL negative;

    negative = (x < F64_ZERO);

    ax = (negative) ? -x : x;

    if(ax < __acoth_min_arg)
    {
        if(ax < F64_ONE)
            return(mathxtra_domain_error());

        return(mathxtra_range_error(x));
    }

    y = F64_HALF * log((ax + F64_ONE) / (ax - F64_ONE));

    return((negative) ? -y : +y);
}

/******************************************************************************
*
* computes the inverse hyperbolic secant of x.
*
* A domain error occurs if x is less than zero or greater than one.
* A range error occurs if x is zero or too small.
*
* Returns: the principal (+ve) inverse hyperbolic secant value.
*          if domain error; returns -F64_HUGE_VAL or F64_HUGE_VAL depending
*          on the sign of the argument.
*          if range error; returns F64_HUGE_VAL
*
******************************************************************************/

/* ln((1/x) ± sqrt((1/x)^2-1)) for 0 < x <= 1 */

/* two-valued symmetric about y=0, yield +ve value */

/* mustn't let (1/x)^2 overflow */
#define __asech_min_arg (F64_MIN)

_Check_return_
extern F64
mx_asech(
    _InVal_     F64 x)
{
    F64 y, invax;

    if(x >= F64_ONE)
    {
        if(x == F64_ONE)
            return(F64_ZERO);

        return(mathxtra_domain_error());
    }

    if(x < __asech_min_arg)
    {
        /* rationale for exact zero similar to that for log, but debatable */
        if(x < F64_ZERO)
            return(mathxtra_domain_error());

        return(mathxtra_range_error(x));
    }

    invax = F64_ONE / x;

    /* another test could be done to test significance to just do log(2x) */

    y = log(invax + sqrt(invax * invax - F64_ONE));

    return(y);
}

/******************************************************************************
*
* computes the value of the inverse hyperbolic sine of x.
*
* A range error occurs if the magnitude of x is too large.
*
* Returns: the inverse hyperbolic sine in the range -inf to +inf.
*          if range error; returns -F64_HUGE_VAL or F64_HUGE_VAL depending
*          on the sign of the argument.
*
******************************************************************************/

/* ln(x + sqrt(x^2 + 1)) */

#define __asinh_max_arg F64_MAX_SQUAREABLE

_Check_return_
extern F64
mx_asinh(
    _InVal_     F64 x)
{
    if(x == F64_ZERO)
        return(F64_ZERO);

    if(fabs(x) > __asinh_max_arg)
        return(mathxtra_range_error(x));

    /* another test could be done to test significance to just do log(2x) */
#ifdef __asinh_max_sig_arg
    if(x > __asinh_max_sig_arg)
#ifdef _ln_two
        return(log(x) + _ln_two);
#else
        return(log(TWO * x));
#endif
#endif

    return(log(x + sqrt((x * x) + F64_ONE)));
}

/******************************************************************************
*
* computes the inverse hyperbolic tangent of x.
*
* A domain error occurs if the magnitude of x is greater than one.
*
* Returns: the inverse hyperbolic tangent value.
*          if domain error; returns -F64_HUGE_VAL or F64_HUGE_VAL depending
*          on the sign of the argument.
*          if range error; returns -F64_HUGE_VAL or F64_HUGE_VAL depending
*          on the sign of the argument.
*
******************************************************************************/

/* (1/2) * ln((1+x) / (1-x)) */

#define __atanh_max_absarg (F64_ONE - F64_EPSILON)

_Check_return_
extern F64
mx_atanh(
    _InVal_     F64 x)
{
    F64 y, ax;
    BOOL negative;

    if(x == F64_ZERO)
        return(F64_ZERO);

    negative = (x < F64_ZERO);

    ax = (negative) ? -x : x;

    /* tanh quickly approaches +/-1 at ends of range */
    if(ax > __atanh_max_absarg)
    {
        if(ax >= F64_ONE)
            return(mathxtra_domain_error());

        return(mathxtra_range_error(x));
    }

    y = F64_HALF * log((F64_ONE + ax) / (F64_ONE - ax));

    return((negative) ? -y : +y);
}

/******************************************************************************
*
* carefully evaluate the hypotenuse of a triangle
*
******************************************************************************/

_Check_return_
extern F64
mx_fhypot(
    _InVal_     F64 x,
    _InVal_     F64 y)
{
    const F64 abs_x = fabs(x);
    const F64 abs_y = fabs(y);

    if(abs_x == F64_ZERO)
        return(abs_y);

    if(abs_y == F64_ZERO)
        return(abs_x);

    if(abs_x >= abs_y)
        return(abs_x * sqrt(F64_ONE + mx_fsquare(abs_y / abs_x)));
    else
        return(abs_y * sqrt(F64_ONE + mx_fsquare(abs_x / abs_y)));
}

/* end of mathxtra.c */
