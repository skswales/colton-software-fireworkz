/* mathxtra.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS July 1991 */

#ifndef __mathxtra_h
#define __mathxtra_h

/*
exported functions
*/

#define PRAGMA_SIDE_EFFECTS_OFF
#include "coltsoft/pragma.h"
/* note that ANSI errno is volatile to enable this sort of CSE optimization */

_Check_return_
extern F64
mx_acosh(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_acosec(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_acosech(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_acot(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_acoth(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_asec(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_asech(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_asinh(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_atanh(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_cosec(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_cosech(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_cot(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_coth(
    _InVal_     F64 x);

/* return the square of a number (or more likely, hard expression) */

_Check_return_
static inline F64
mx_fsquare(
    _InVal_     F64 x)
{
    return(x * x);
}

_Check_return_
extern F64
mx_fhypot(
    _InVal_     F64 x,
    _InVal_     F64 y);

_Check_return_
extern F64
mx_sec(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_sech(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_gcd(
    _InVal_     F64 a,
    _InVal_     F64 b);

_Check_return_
extern F64
mx_lcm(
    _InVal_     F64 a,
    _InVal_     F64 b);

#define PRAGMA_SIDE_EFFECTS
#include "coltsoft/pragma.h"

#if defined(_MSC_VER)
#if _MSC_VER < 1800 /* < VS2013 */

extern double FreeBSD_erf(double);

#define erf FreeBSD_erf

extern double FreeBSD_erfc(double);

#define erfc FreeBSD_erfc

#endif
#endif /* _MSC_VER */

_Check_return_
extern double FreeBSD_jn(int, double);

#define bessel_jn(n, x) FreeBSD_jn(n, x)

_Check_return_
extern double FreeBSD_yn(int, double);

#define bessel_yn(n, x) FreeBSD_yn(n, x)


#if __STDC_VERSION__ < 199901L

#define acosh(d) mx_acosh(d)
#define asinh(d) mx_asinh(d)
#define atanh(d) mx_atanh(d)

_Check_return_
static inline bool
isunordered(_InVal_ double a, _InVal_ double b)
{
    return(isnan(a) || isnan(b));
}

_Check_return_
static inline bool
isgreater(_InVal_ double a, _InVal_ double b)
{
    if(isnan(a) || isnan(b))
        return(!isnan(b));

    return(a > b);
}

_Check_return_
static inline bool
isless(_InVal_ double a, _InVal_ double b)
{
    if(isnan(a) || isnan(b))
        return(!isnan(b));

    return(a < b);
}

_Check_return_
static inline double
fmax(_InVal_ double a, _InVal_ double b)
{
    if(isnan(a))
        return(b);

    return(isgreater(a, b) ? a : b);
}

_Check_return_
static inline double
fmin(_InVal_ double a, _InVal_ double b)
{
    if(isnan(a))
        return(b);

    return(isless(a, b) ? a : b);
}

_Check_return_
static inline double
exp2(_InVal_ double d)
{
    return(pow(2.0, d));
}

#if WINDOWS

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h" /* for _log2_e */
#endif

_Check_return_
static inline double
log2(_InVal_ double d)
{
    return(log(d) * _log2_e);
}

#endif /* OS */

#endif /* __STDC_VERSION__ */


#endif /* __mathxtra_h */

/* end of mathxtra.h  */
