/* mathxtr4.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2018 Stuart Swales */

/* Additional math routines */

/* SKS Dec 2014 */

#ifndef __mathxtr4_h
#define __mathxtr4_h

/*
exported functions
*/

/* log(e) B(a,b) */

_Check_return_
extern F64
mx_ln_beta(
    _InVal_     F64 a,
    _InVal_     F64 b);

/* log(e) (1 / B(a,b)) */

_Check_return_
static inline F64
mx_ln_reciprocal_beta(
    _InVal_     F64 alpha,
    _InVal_     F64 beta)
{
    return(-mx_ln_beta(alpha, beta));
}

/* Ix(alpha, beta) is the regularized incomplete beta function */

_Check_return_
extern F64
mx_Ix_beta(
    _InVal_     F64 x, /* in [0..1] */
    _InVal_     F64 alpha, /* alpha and beta are shape parameters, both > 0 */
    _InVal_     F64 beta);

/* log(e) (n k) */

_Check_return_
extern F64
mx_ln_binomial_coefficient(
    _InVal_     S32 n,
    _InVal_     S32 k);

#if __STDC_VERSION__ < 199901L

#if WINDOWS

_Check_return_
extern double FreeBSD_lgamma_r(double, int *);

_Check_return_
static inline double lgamma(double z)
{
    int sign;
    return(FreeBSD_lgamma_r(z, &sign));
}

_Check_return_
static inline double tgamma(double z)
{
    int sign;
    F64 ln_gamma = FreeBSD_lgamma_r(z, &sign);
    F64 gamma = sign * exp(ln_gamma);
    return(gamma);
}

#endif /* OS */

#endif /* __STDC_VERSION__ */

/* log(e) n! */

_Check_return_
static inline F64
mx_ln_factorial(
    _InVal_     S32 n)
{
    return(lgamma(n + 1.0));
}

/* Regularized gamma function P(s,x) */

_Check_return_
extern F64
mx_P_gamma(
    _InVal_     F64 s,
    _InVal_     F64 x);

/* Regularized gamma function Q(s,x) */

_Check_return_
extern F64
mx_Q_gamma(
    _InVal_     F64 s,
    _InVal_     F64 x);

/* Lower incomplete gamma function y(s,x) (Little gamma) */

_Check_return_
extern F64
mx_y_gamma(
    _InVal_     F64 s,
    _InVal_     F64 x);

/* Upper incomplete gamma function Gu(s,x) (Big gamma) */

_Check_return_
extern F64
mx_Gu_gamma(
    _InVal_     F64 s,
    _InVal_     F64 x);

#endif /* __mathxtr4_h */

/* end of mathxtr4.h  */
