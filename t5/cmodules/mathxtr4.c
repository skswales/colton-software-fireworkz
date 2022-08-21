/* mathxtr4.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2021 Stuart Swales */

/* Additional math routines */

/* SKS Dec 2014 */

#include "common/gflags.h"

#include "cmodules/mathxtr4.h"

/* Beta(a,b)
 *  = Gamma(a) * Gamma(b) / Gamma(a+b)
 *
 * Special Values:
 * Beta(a,1) = 1/a
 * Beta(1,b) = 1/b
 */

/* log(e) Beta(a,b) */

_Check_return_
extern F64
mx_ln_beta(
    _InVal_     F64 a,
    _InVal_     F64 b)
{
    if(a == 1.0)
        return(-log(b));

    if(b == 1.0)
        return(-log(a));

    {
    const F64 ln_gamma_a = lgamma(a);
    const F64 ln_gamma_b = lgamma(b);
    const F64 ln_gamma_s = lgamma(a + b);

    return((ln_gamma_a + ln_gamma_b) - ln_gamma_s);
    } /*block*/
}

/* Regularized incomplete beta function Ix(alpha,beta) */

_Check_return_
static inline F64
incomplete_beta_cf_d_odd(
    _InVal_     F64 x,
    _InVal_     F64 p,
    _InVal_     F64 q,
    _InVal_     S32 n) /* d(2n+1) */
{
    const F64 two_n = 2.0 * n;
    const F64 numerator = (p + n) * (p + q + n);
    const F64 denominator = (p + two_n) * (p + two_n + 1);

    return(-x * (numerator / denominator));
}

_Check_return_
static inline F64
incomplete_beta_cf_d_even(
    _InVal_     F64 x,
    _InVal_     F64 p,
    _InVal_     F64 q,
    _InVal_     S32 n) /* d(2n+2) */
{
    const F64 two_n = 2.0 * n;
    const F64 n_plus_1 = (n + 1.0);
    const F64 numerator = n_plus_1 * (q - n_plus_1);
    const F64 denominator = (p + two_n + 1) * (p + two_n + 2);

    return(+x * (numerator / denominator));
}

/* Calculate the continued fraction term using the modified Lentz algorithm.
 * f = b0 +   (a1 / (b1 +   a2 / (b2 +   a3 / (b3 +  a4 / (b4 +   ... )))))
 * See 'Numerical Methods for Special Functions' for the outline of this method.
 *
 * A&S 26.5.8 gives the expansion of the c.f. term in the incomplete beta function as
 * f =  0 +   ( 1 / ( 1 +   d1 / ( 1 +   d2 / ( 1 +  d3 / ( 1 +   ... )))))
 * an = d(n-1,x); d0 = 1
 * b0 = 0
 * bn = 1
 */

_Check_return_
static F64
mx_incomplete_beta_continued_fraction(
    _InVal_     F64 x, /* value now standardized */
    _InVal_     F64 alpha, /* alpha and beta are shape parameters */
    _InVal_     F64 beta)
{
    const F64 tiny = 1E-30;
    const F64 epsilon = 1E-16;
    F64 a;
  /*const F64 b0 = 0.0;*/
    const F64 b = 1.0;
    F64 Cn, Dn, En, Hn;
    F64 delta_n;
    S32 n;

    assert((x >= 0.0) && (x <= 1.0));
    assert((alpha > 0.0) && (beta > 0.0));

    Cn = epsilon; /*b0*/ /*(fabs(b0) < tiny) ? epsilon : b0;*/
    Dn = 0.0;
    En = Cn;

    /* do the first step (d0==1) */
    a = 1.0;
    //reportf(TEXT("ibcf:d%d %+f"),0,a);

    Dn = b + a * Dn/*n-1*/;
    En = b + a / En/*n-1*/;

    if(fabs(Dn) < tiny) Dn = epsilon;
    if(fabs(En) < tiny) En = epsilon;

    Dn = 1.0 / Dn;
    Hn = En * Dn;
    Cn = Cn/*n-1*/ * Hn;
    //reportf(TEXT("ibcf:C%d %+f"),0,Cn);

    for(n = 0; n < 100; ++n)
    {
        /* NB all bn are 1 */

        /* do the odd step (d2n+1) */
        a = incomplete_beta_cf_d_odd(x, alpha, beta, n);
        //reportf(TEXT("ibcf:d%d %+f"),2*n+1,a);

        Dn = b + a * Dn/*n-1*/;
        En = b + a / En/*n-1*/;

        if(fabs(Dn) < tiny) Dn = epsilon;
        if(fabs(En) < tiny) En = epsilon;

        Dn = 1.0 / Dn;
        Hn = En * Dn;
        Cn = Cn/*n-1*/ * Hn;
        //reportf(TEXT("ibcf:C%d %+f"),2*n+1,Cn);

        /* close enough? */
        delta_n = fabs(Hn - 1.0);
        if(delta_n < 1e-9) 
            break;

        /* do the even step (d2n+2) */
        a = incomplete_beta_cf_d_even(x, alpha, beta, n);
        //reportf(TEXT("ibcf:d%d %+f"),2*n+2,a);

        Dn = b + a * Dn/*n-1*/;
        En = b + a / En/*n-1*/;

        if(fabs(Dn) < tiny) Dn = epsilon;
        if(fabs(En) < tiny) En = epsilon;

        Dn = 1.0 / Dn;
        Hn = En * Dn;
        Cn = Cn/*n-1*/ * Hn;
        //reportf(TEXT("ibcf:C%d %+f"),2*n+1,Cn);

        /* close enough? */
        delta_n = fabs(Hn - 1.0);
        if(delta_n < 1e-9) 
            break;
    }

    //reportf(TEXT("ibcf:res %+f after %d"),Cn,n);
    return(Cn);
}

/* See A&S 26.5.1 */

_Check_return_
extern F64
mx_Ix_beta(
    _InVal_     F64 x, /* in [0..1] */
    _InVal_     F64 alpha, /* alpha and beta are shape parameters, both > 0 */
    _InVal_     F64 beta)
{
    if((alpha <= 0.0) || (beta <= 0.0))
    {
        errno = EDOM;
        return(0.0);
    }

    if(x <= 0.0)
    {
        if((x != 0.0) || (alpha < 1.0)) /* avoid pole at zero for alpha < 1 */
            errno = EDOM;
        return(0.0);
    }

    if(x >= 1.0)
    {
        if((x != 1.0) || (beta < 1.0)) /* avoid pole at one for beta < 1 */
            errno = EDOM;
        return(1.0);
    }

    /* These special cases are cheap and easy */
    if(alpha == 1.0)
        return(1.0 - pow(1.0 - x, beta));

    if(beta == 1.0)
        return(pow(x, alpha));

    {
    const F64 switchover = ((alpha - 1) / (alpha + beta - 2));
    const F64 ln_reciprocal_term = mx_ln_reciprocal_beta(alpha, beta);
    const F64 ln_alpha_power_term = log(x) * alpha;
    const F64 ln_beta_power_term = log(1 - x) * beta;
    const F64 common_term = exp(ln_reciprocal_term + ln_alpha_power_term + ln_beta_power_term);
    F64 continued_fraction_term;
    F64 I_beta_result;

    if(x <= switchover)
    {
        continued_fraction_term = mx_incomplete_beta_continued_fraction(x, alpha, beta);
        I_beta_result = common_term * (continued_fraction_term / alpha);
    }
    else
    {   /* Use A&S 26.5.2 relation */
        continued_fraction_term = mx_incomplete_beta_continued_fraction(1.0 - x, beta, alpha);
        I_beta_result = 1.0 - (common_term * (continued_fraction_term / beta));
    }

    return(I_beta_result);
    } /*block*/
}

/* log(e) (n k) */

_Check_return_
extern F64
mx_ln_binomial_coefficient(
    _InVal_     S32 n,
    _InVal_     S32 k)
{
    F64 ln_numerator, ln_denominator;

    if((n < 0) || (k < 0) || ((n - k) < 0))
    {
        errno = EDOM;
        return(-HUGE_VAL);
    }

    ln_numerator = mx_ln_factorial(n);

    if(errno)
        return(HUGE_VAL);

    ln_denominator = mx_ln_factorial(n - k) + mx_ln_factorial(k);

    if(errno)
        return(HUGE_VAL);

    return(ln_numerator - ln_denominator);
}

/* Regularized gamma function P(s,x)
 *  = (1 / (G(s)) . y(s,x)
 *    where y(s,x) is the lower incomplete gamma function
 *
 * For s,x that wouldn't converge well with the series sum,
 * use
 * Q(s,x) = 1 - P(s,x)
 * to evaluate using the continued fraction expansion.
 */

_Check_return_
extern F64
mx_P_gamma(
    _InVal_     F64 s,
    _InVal_     F64 x)
{
    if(x < (s + 1.0))
    {   /* calculate using the series sum */
        const F64 P_numerator = mx_y_gamma(s, x);
        const F64 P_denominator = tgamma(s);
        const F64 P = P_numerator / P_denominator;

#if 0 /* can cross-check against continued fraction if we want to */
        const F64 Q_numerator = mx_Gu_gamma(s, x);
        const F64 Q_denominator = tgamma(s);
        const F64 Q = (Q_numerator / Q_denominator);
        const F64 P_Gu = 1.0 - Q;
        //reportf(TEXT("P_gamma(s=%f,x=%f): result=%f (result=%f (CF))"), s, x, P, P_Gu);
#else
        //reportf(TEXT("P_gamma(s=%f,x=%f): result=%f"), s, x, P);
#endif

        return(P);
    }
    else
    {   /* calculate using the continued fraction */
        const F64 Q_numerator = mx_Gu_gamma(s, x);
        const F64 Q_denominator = tgamma(s);
        const F64 Q = (Q_numerator / Q_denominator);
        const F64 P = 1.0 - Q;
        //reportf(TEXT("P_gamma(s=%f,x=%f): result=%f (CF)"), s, x, P);

        return(P);
    }
}

/* Regularized gamma function Q(s,x)
 *  = (1 / (G(s)) . Gu(s,x)
 *    where Gu(s,x) is the upper incomplete gamma function
 *
 * For s,x that would converge well with the series sum,
 * use
 * Q(s,x) = 1 - P(s,x)
 * otherwise evaluate using the continued fraction expansion.
 */

_Check_return_
extern F64
mx_Q_gamma(
    _InVal_     F64 s,
    _InVal_     F64 x)
{
    if(x < (s + 1.0))
    {   /* calculate using the series sum */
        const F64 P_numerator = mx_y_gamma(s, x);
        const F64 P_denominator = tgamma(s);
        const F64 P = (P_numerator / P_denominator);
        const F64 Q = 1.0 - P;

        return(Q);
    }
    else
    {   /* calculate using the continued fraction */
        const F64 Q_numerator = mx_Gu_gamma(s, x);
        const F64 Q_denominator = tgamma(s);
        const F64 Q = (Q_numerator / Q_denominator);

        return(Q);
    }
}

/* Lower incomplete gamma function y(s,x) (Little gamma)
 *  = integral from t=0 to t=x of (t ^ (s - 1) . exp(-t)) dt
 *
 * Series sum is
 * x^s . G(s) . exp(-x) . sum(k=0..+inf; x^k / G(s+k+1))
 */

_Check_return_
extern F64
mx_y_gamma(
    _InVal_     F64 s,
    _InVal_     F64 x)
{
    F64 y_gamma_result;
#if 1
    const F64 ln_power_term = s * log(x);
    const F64 ln_exp_term = -x;
    const F64 ln_gamma_term = lgamma(s);
    const F64 common_product_term = exp(ln_power_term + ln_exp_term + ln_gamma_term);
#else
    const F64 power_term = pow(x, s);
    const F64 exp_term = exp(-x);
    const F64 gamma_term = tgamma(s);
    const F64 common_product_term = power_term * exp_term * gamma_term;
#endif
    F64 sum = 0.0;
    S32 k;
    F64 term = 1.0 / tgamma(s + 1);

    for(k = 0; ; ++k)
    {
        sum += term;

        if(term < 1e-12)
            break;

        term *= (x / (s + k + 1)); /* NB G(t + 1) = t . G(t) */

        if(!isfinite(term))
        {
            //reportf(TEXT("non-finite term in mx_y_gamma(s=%f,x=%f): k=%d"), s, x, k);
            break;
        }
    }

    y_gamma_result = common_product_term * sum;

    //reportf(TEXT("mx_y_gamma(s=%f,x=%f): result=%f after k=%d"), s, x, y_gamma_result, k);
    return(y_gamma_result);
}

/* Calculate the continued fraction term using the modified Lentz algorithm.
 * f = b0 +   (a1 / (b1 +    a2 / (b2 +    a3 / (b3 +    a4 / (b4 +   ... )))))
 * See 'Numerical Methods for Special Functions' for the outline of this method.
 *
 * NMSF 6.31 gives the expansion of the c.f. term in the upper incomplete gamma function as
 * f =  0 +   ( 1 / ( 1 + alp.1 / ( 1 + alp.2 / ( 1 + alp.3 / ( 1 +   ... )))))
 * an = alp.(n-1,x,a); alp.0 = 1
 * b0 = 0
 * bn = 1
 */

_Check_return_
static inline F64
incomplete_Gu_cf_alpha_n(
    _InVal_     F64 x,
    _InVal_     F64 a,
    _InVal_     S32 n)
{
    const F64 numerator = n * (a - n);
    const F64 common = (x + 2.0*n - a);
    const F64 denominator = (common - 1.0) * (common + 1.0);
    const F64 alpha_n = numerator / denominator;

    return(alpha_n);
}

_Check_return_
static F64
mx_incomplete_Gu_continued_fraction(
    _InVal_     F64 s,
    _InVal_     F64 x)
{
    const F64 tiny = 1E-30;
    const F64 epsilon = 1E-16;
    F64 a;
  /*const F64 b0 = 0.0;*/
    const F64 b = 1.0;
    F64 Cn, Dn, En, Hn;
    F64 delta_n;
    S32 n;

    Cn = epsilon; /*b0*/ /*(fabs(b0) < tiny) ? epsilon : b0;*/
    Dn = 0.0;
    En = Cn;

    /* do the first step (alp.0==1) */
    a = 1.0;
    //reportf(TEXT("iGucf:alp.%d %+f"),0,a);

    Dn = b + a * Dn/*n-1*/;
    En = b + a / En/*n-1*/;

    if(fabs(Dn) < tiny) Dn = epsilon;
    if(fabs(En) < tiny) En = epsilon;

    Dn = 1.0 / Dn;
    Hn = En * Dn;
    Cn = Cn/*n-1*/ * Hn;
    //reportf(TEXT("iGucf:C%d %+f"),0,Cn);

    for(n = 1; n < 100; ++n)
    {
        /* NB all bn are 1 */

        a = incomplete_Gu_cf_alpha_n(x, s, n); /* get alp.n */
        //reportf(TEXT("iGucf:alp.%d %+f"),n,a);

        Dn = b + a * Dn/*n-1*/;
        En = b + a / En/*n-1*/;

        if(fabs(Dn) < tiny) Dn = epsilon;
        if(fabs(En) < tiny) En = epsilon;

        Dn = 1.0 / Dn;
        Hn = En * Dn;
        Cn = Cn/*n-1*/ * Hn;
        //reportf(TEXT("iGucf:C%d %+f"),n,Cn);

        /* close enough? */
        delta_n = fabs(Hn - 1.0);
        if(delta_n < 1e-9) 
            break;
    }

    //reportf(TEXT("iGucf:res %+f after %d"),Cn,n);
    return(Cn);
}

/* Upper incomplete gamma function Gu(s,x) (Big gamma)
 *  = integral from t=x to t=+inf of (t ^ (s - 1) . exp(-t)) dt
 */

_Check_return_
extern F64
mx_Gu_gamma(
    _InVal_     F64 s,
    _InVal_     F64 x)
{
    const F64 common_term = exp(-x) * pow(x, s) / (x + 1 - s);
    const F64 continued_fraction_term = mx_incomplete_Gu_continued_fraction(s, x);
    F64 Gu_gamma_result = common_term * continued_fraction_term;

    //reportf(TEXT("mx_Gu_gamma(s=%f,x=%f): result=%f"), s, x, Gu_gamma_result);
    return(Gu_gamma_result);
}

/* end of mathxtr4.c */
