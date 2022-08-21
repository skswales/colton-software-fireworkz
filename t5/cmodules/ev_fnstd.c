/* ev_fnstd.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2019 Stuart Swales */

/* Statistical function routines (distributions etc) for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "cmodules/mathxtr4.h" /* for mx_ln_binomial_coefficient() */

#include "cmodules/mathxtri.h"

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h"
#endif

/******************************************************************************
*
* Statistical functions - Discrete distributions
*
******************************************************************************/

static void
binom_dist_pmf_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 n,
    _InVal_     F64 p,
    _InVal_     S32 k);

/******************************************************************************
*
* NUMBER binom.dist()
*
* NUMBER binom.dist.range()
*
* See OpenDocument 1.2 definition of BINOMDIST and BINOM.DIST.RANGE
*
* A&S 26.1.20
*
******************************************************************************/

/* Binomial is a discrete distribution with
 *
 * CDF(k;n,p)
 *  = I(x; n-k, k+1)
 *    where Ix(alpha,beta) is the regularized incomplete beta function
 *    and x = 1-p
 */

static void
binom_dist_cdf_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 n,
    _InVal_     F64 p,
    _InVal_     S32 k)
{
    S32 j;

    if( (n < 0) || (k < 0) || (k > n) ||
        (p < 0.0) || (p > 1.0) )
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    if(k == n)
    {
        ev_data_set_real(p_ev_data_out, 1.0);
        return;
    }

    if(n > 12)
    {   /* can save time using incomplete beta function */
        const F64 one_minus_p = 1.0 - p;
        const S32 n_minus_k = n - k;
        const S32 k_plus_one = k + 1;
        ev_data_set_real(p_ev_data_out, mx_Ix_beta(one_minus_p, n_minus_k, k_plus_one));
        return;
    }

    *p_ev_data_out = ev_data_real_zero;

    /* sum of PMF over [0..k] */
    for(j = 0; j <= k; ++j)
    {
        EV_DATA term;

        binom_dist_pmf_calc(&term, n, p, j); /* may return fp or error */

        if(!two_nums_add_try(p_ev_data_out, p_ev_data_out, &term, TRUE /*propogate_errors*/))
            ev_data_set_error(p_ev_data_out, EVAL_ERR_CALC_FAILURE);

        if(ev_data_is_error(p_ev_data_out))
            break;
    }
}

/* Binomial is a discrete distribution with
 *
 * PMF(k;n,p)
 *  = (n k) . p^k . (1-p)^(n-k)
 */

static void
alternate_binom_dist_pmf_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 n,
    _InVal_     F64 p,
    _InVal_     S32 k)
{
    /* do in logs */
    F64 ln_binomial_term, ln_power_term_k, ln_power_term_n_minus_k, ln_term;
    const S32 n_minus_k = (n - k);

    errno = 0;

    ln_binomial_term = mx_ln_binomial_coefficient(n, k);

    ln_power_term_k = log(p) * k;

    ln_power_term_n_minus_k = log(1.0 - p) * n_minus_k;

    ln_term = ln_binomial_term + ln_power_term_k + ln_power_term_n_minus_k;

    ev_data_set_real(p_ev_data_out, exp(ln_term));

    /* exp() overflowed? - don't test for underflow case */
    if(p_ev_data_out->arg.fp == F64_HUGE_VAL)
        ev_data_set_error(p_ev_data_out, EVAL_ERR_OUTOFRANGE);
}

static void
binom_dist_pmf_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 n,
    _InVal_     F64 p,
    _InVal_     S32 k)
{
    EV_DATA binomial_term, power_term_k, power_term_n_minus_k;
    const S32 n_minus_k = (n - k);

    if( (n < 0) || (k < 0) || (k > n) ||
        (p < 0.0) || (p > 1.0) )
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    if(n > 1000)
    {   /* binomial term near to overflow and power term product near to underflow */
        alternate_binom_dist_pmf_calc(p_ev_data_out, n, p, k); /* may return fp or error */
        return;
    }

    binomial_coefficient_calc(&binomial_term, n, k); /* may return integer or fp or error */

    errno = 0;

    ev_data_set_real(&power_term_k, pow(p, k));
    ev_data_set_real(&power_term_n_minus_k, pow(1.0 - p, n_minus_k));

    if(errno /* == EDOM */)
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    if( !two_nums_multiply_try(p_ev_data_out, &power_term_k, &power_term_n_minus_k, TRUE /*propogate_errors*/) ||
        !two_nums_multiply_try(p_ev_data_out, p_ev_data_out, &binomial_term, TRUE /*propogate_errors*/) )
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_CALC_FAILURE);
        return;
    }
}

static void
binom_dist_range_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 n,
    _InVal_     F64 p,
    _InVal_     S32 S,
    _InVal_     S32 S2)
{
    S32 k;

    if(S == 0)
    {   /* sum(PMF[0..S2]) is the CDF */
        binom_dist_cdf_calc(p_ev_data_out, n, p, S2); /* may return fp or error */
        return;
    }

    if( (n < 0) || (S < 0) || (S2 < 0) ||
        (S > S2) || (S2 > n) ||
        (p < 0.0) || (p > 1.0) )
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    if((S2 == n) && (n > 12))
    {   /* sum(PMF[S..n]) */ /* can save time using incomplete beta function */
        S32 n_minus_k_plus_one;
        k = S;
        n_minus_k_plus_one = n - k + 1;
        ev_data_set_real(p_ev_data_out, mx_Ix_beta(p, k, n_minus_k_plus_one));
        return;
    }

    *p_ev_data_out = ev_data_real_zero;

    /* sum of PMF over [S..S2] */
    for(k = S; k <= S2; ++k)
    {
        EV_DATA term;

        binom_dist_pmf_calc(&term, n, p, k); /* may return fp or error */

        if(!two_nums_add_try(p_ev_data_out, p_ev_data_out, &term, TRUE /*propogate_errors*/))
            ev_data_set_error(p_ev_data_out, EVAL_ERR_CALC_FAILURE);

        if(ev_data_is_error(p_ev_data_out))
            break;
    }
}

PROC_EXEC_PROTO(c_binom_dist)
{
    const S32 S = args[0]->arg.integer; /*number_success*/
    const S32 n = args[1]->arg.integer; /*number_trials*/
    const F64 p = args[2]->arg.fp; /*probability_success*/
    const BOOL cumulative = args[3]->arg.boolean;

    exec_func_ignore_parms();

    if(cumulative)
        binom_dist_cdf_calc(p_ev_data_res, n, p, S); /* may return fp or error */
    else
        binom_dist_pmf_calc(p_ev_data_res, n, p, S); /* may return fp or error */
}

PROC_EXEC_PROTO(c_binom_dist_range)
{
    const S32 n = args[0]->arg.integer; /*number_trials*/
    const F64 p = args[1]->arg.fp; /*probability_success*/
    const S32 S = args[2]->arg.integer;
    const S32 S2 = (n_args > 3) ? args[3]->arg.integer : S;

    exec_func_ignore_parms();

    if(S == S2)
    {   /* just PMF at S */
        binom_dist_pmf_calc(p_ev_data_res, n, p, S); /* may return fp or error */
        return;
    }

    binom_dist_range_calc(p_ev_data_res, n, p, S, S2); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER binom.inv(number_trials, probability_success, alpha)
*
******************************************************************************/

static void
binom_inv_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 N, /*number_trials*/
    _InVal_     F64 P, /*probability_success*/
    _InVal_     F64 alpha)
{
    S32 k;

    assert((P >= 0.0) && (P <= 1.0));
    assert((alpha >= 0.0) && (alpha <= 1.0));

    reportf(TEXT("binom_inv_calc(N=%d, P=%f, alpha=%f)"), N, P, alpha);
    for(k = 0; k <= N; ++k)
    {
        EV_DATA ev_data;
        F64 CDF_k;

        /* could improve using bisection */
        binom_dist_cdf_calc(&ev_data, N, P, k); /* may return fp or error */

        if(ev_data_is_error(&ev_data))
        {
            *p_ev_data_out = ev_data;
            return;
        }

        assert(RPN_DAT_REAL == ev_data.did_num);
        CDF_k = ev_data.arg.fp;

        if(CDF_k >= alpha)
        {
            reportf(TEXT("binom_inv_calc(N=%d, P=%f, alpha=%f) returns k=%d"), N, P, alpha, k);
            ev_data_set_integer(p_ev_data_out, k);
            return;
        }
    }

    ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
}

PROC_EXEC_PROTO(c_binom_inv)
{
    const S32 N = args[0]->arg.integer; /*number_trials*/
    const F64 P = args[1]->arg.fp; /*probability_success*/
    const F64 alpha = args[2]->arg.fp;

    exec_func_ignore_parms();

    binom_inv_calc(p_ev_data_res, N, P, alpha); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER hypgeom.dist()
*
* See OpenDocument 1.2 definition of HYPGEOMDIST
*
* A&S 26.1.21
*
******************************************************************************/

static void
alternate_hypgeom_dist_pdf_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 s,
    _InVal_     S32 n,
    _InVal_     S32 M,
    _InVal_     S32 N,
    _InRef_     PC_F64 ln_denominator)
{
    F64 ln_numerator_lhs, ln_numerator_rhs, ln_numerator, ln_term;

    ln_numerator_lhs = mx_ln_binomial_coefficient(M, s);
    ln_numerator_rhs = mx_ln_binomial_coefficient(N - M, n - s);

    ln_numerator = ln_numerator_lhs + ln_numerator_rhs;

    ln_term = ln_numerator - *ln_denominator;

    ev_data_set_real(p_ev_data_out, exp(ln_term));

    /* exp() overflowed? - don't test for underflow case */
    if(p_ev_data_out->arg.fp == F64_HUGE_VAL)
        ev_data_set_error(p_ev_data_out, EVAL_ERR_OUTOFRANGE);
}

static void
alternate_hypgeom_dist_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 x /*sample_successes*/,
    _InVal_     S32 n /*number_sample*/,
    _InVal_     S32 M /*population_successes*/,
    _InVal_     S32 N /*number_population*/,
    _InVal_     BOOL cumulative)
{
    F64 ln_denominator;
    S32 i;

    errno = 0;

    ln_denominator = mx_ln_binomial_coefficient(N, n);

    *p_ev_data_out = ev_data_real_zero;

    /* CDF is the sum from the terms over [0..x]; PMF is just the single term at x  */
    for(i = cumulative ? 0 : x; i <= x; ++i)
    {
        EV_DATA term;

        alternate_hypgeom_dist_pdf_calc(&term, i, n, M, N, &ln_denominator); /* may return fp or error */

        if(!two_nums_add_try(p_ev_data_out, p_ev_data_out, &term, TRUE /*propogate_errors*/))
            ev_data_set_error(p_ev_data_out, EVAL_ERR_CALC_FAILURE);

        if(ev_data_is_error(p_ev_data_out))
            break;
    }
}

/* Hypergeometric is a discrete distribution with
 *
 * PMF(s,N1,N2,n)
 * = (N1 s) . (N2 n-s) / (N1+N2 n)
 */

static void
hypgeom_dist_pdf_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return integer or fp or error */
    _InVal_     S32 s,
    _InVal_     S32 n,
    _InVal_     S32 M,
    _InVal_     S32 N,
    _InRef_     P_EV_DATA denominator)
{
    EV_DATA numerator_lhs, numerator_rhs, numerator;

    binomial_coefficient_calc(&numerator_lhs, M, s);            /* may return integer or fp or error */
    binomial_coefficient_calc(&numerator_rhs, N - M, n - s);    /* may return integer or fp or error */

    if( !two_nums_multiply_try(&numerator, &numerator_lhs, &numerator_rhs, TRUE /*propogate_errors*/) ||
        !two_nums_divide_try(p_ev_data_out, &numerator, denominator, TRUE /*propogate_errors*/) )
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_CALC_FAILURE);
    }
}

static void
hypgeom_dist_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 x /*sample_successes*/,
    _InVal_     S32 n /*number_sample*/,
    _InVal_     S32 M /*population_successes*/,
    _InVal_     S32 N /*number_population*/,
    _InVal_     BOOL cumulative)
{
    EV_DATA denominator;
    S32 i;

    if(n > 170)
    {
        alternate_hypgeom_dist_calc(p_ev_data_out, x, n, M, N, cumulative); /* may return fp or error */
        return;
    }

    binomial_coefficient_calc(&denominator, N, n); /* may return integer or fp or error */

    *p_ev_data_out = ev_data_real_zero;

    /* CDF is the sum from the terms over [0..x]; PMF is just the single term at x  */
    for(i = cumulative ? 0 : x; i <= x; ++i)
    {
        EV_DATA term;

        hypgeom_dist_pdf_calc(&term, i, n, M, N, &denominator); /* may return integer or fp or error */

        if(!two_nums_add_try(p_ev_data_out, p_ev_data_out, &term, TRUE /*propogate_errors*/))
            ev_data_set_error(p_ev_data_out, EVAL_ERR_CALC_FAILURE);

        if(ev_data_is_error(p_ev_data_out))
            break;
    }
}

PROC_EXEC_PROTO(c_hypgeom_dist)
{
    const S32 x = args[0]->arg.integer; /*sample_successes*/
    const S32 n = args[1]->arg.integer; /*number_sample*/
    const S32 M = args[2]->arg.integer; /*population_successes*/
    const S32 N = args[3]->arg.integer; /*number_population*/
    const BOOL cumulative = (n_args > 4) ? args[4]->arg.boolean : FALSE;

    exec_func_ignore_parms();

    if( (x < 0) || (n < 0) || (M < 0) || (N < 0) ||
        (n > N) ||
        (M > N) ||
        (x > MIN(n, M)) ||
        (x < MAX(0, n - N + M)) )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    hypgeom_dist_calc(p_ev_data_res, x, n, M, N, cumulative); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER negbinom.dist()
*
* See OpenDocument 1.2 definition of NEGBINOMDIST
*
* A&S 26.1.23
*
******************************************************************************/

/* Negative binomial is a discrete distribution with
 *
 * PMF(k;r,p)
 *  = (k+r-1 k) . p^r . (1-p)^k
 */

static void
negbinom_dist_pdf_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 k,
    _InVal_     S32 r,
    _InVal_     F64 p)
{
    EV_DATA binomial_term, power_term_r, power_term_k;

    binomial_coefficient_calc(&binomial_term, k + r - 1, k); /* may return integer or fp or error */

    errno = 0;

    ev_data_set_real(&power_term_r, pow(p, r));
    ev_data_set_real(&power_term_k, pow(1.0 - p, k));

    if(errno /* == EDOM */)
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    if( !two_nums_multiply_try(p_ev_data_out, &power_term_r, &power_term_k, TRUE /*propogate_errors*/) ||
        !two_nums_multiply_try(p_ev_data_out, p_ev_data_out, &binomial_term, TRUE /*propogate_errors*/) )
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_CALC_FAILURE);
        return;
    }
}

static void
negbinom_dist_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 x,
    _InVal_     S32 r,
    _InVal_     F64 p,
    _InVal_     BOOL cumulative)
{
    S32 k;

    if( ((x + r - 1) <= 0) ||
        (p < 0.0) || (p > 1.0) )
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    *p_ev_data_out = ev_data_real_zero;

    /* CDF is the sum from the terms over [0..x]; PMF is just the single term at x */
    for(k = cumulative ? 0 : x; k <= x; ++k)
    {
        EV_DATA term;

        negbinom_dist_pdf_calc(&term, k, r, p); /* may return fp or error */

        if(!two_nums_add_try(p_ev_data_out, p_ev_data_out, &term, TRUE /*propogate_errors*/))
            ev_data_set_error(p_ev_data_out, EVAL_ERR_CALC_FAILURE);

        if(ev_data_is_error(p_ev_data_out))
            break;
    }
}

PROC_EXEC_PROTO(c_negbinom_dist)
{
    const S32 x = args[0]->arg.integer; /*number of failures*/
    const S32 r = args[1]->arg.integer /*threshold number of successes*/;
    const F64 p = args[2]->arg.fp; /*probability of success*/
    const BOOL cumulative = (n_args > 3) ? args[3]->arg.boolean : FALSE;

    exec_func_ignore_parms();

    negbinom_dist_calc(p_ev_data_res, x, r, p, cumulative); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER poisson.dist(x:number, lambda:number {, cumulative:Boolean=TRUE})
*
* See OpenDocument 1.2 definition of POISSON
*
* A&S 26.1.22
*
******************************************************************************/

/* Poisson is a discrete distribution with
 *
 * PMF(k;lambda)
 *  = lambda^k . exp(-lambda) / k!
 */

static void
poisson_dist_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 x,
    _InVal_     F64 lambda,
    _InVal_     BOOL cumulative)
{
    const F64 ln_exp_term = -lambda; /* this exp() won't overflow, ignore underflow */
    const BOOL more_logs = (x > 20);
    S32 k;

    *p_ev_data_out = ev_data_real_zero;

    /* CDF is the sum from the terms over [0..x]; PMF is just the single term at x */
    for(k = cumulative ? 0 : x; k <= x; ++k)
    {
        const F64 ln_power_term = log(lambda) * k;
        const F64 ln_numerator = ln_power_term + ln_exp_term;
        EV_DATA term;

        if(more_logs)
        {
            const F64 ln_denominator = mx_ln_factorial(k);

            ev_data_set_real(&term, exp(ln_numerator - ln_denominator));
        }
        else
        {
            EV_DATA numerator, denominator;

            ev_data_set_real(&numerator, exp(ln_numerator));

            factorial_calc(&denominator, k); /* may return integer or fp or error */

            if(!two_nums_divide_try(&term, &numerator, &denominator, TRUE /*propogate_errors*/))
                ev_data_set_error(&term, EVAL_ERR_CALC_FAILURE);
        }

        if(!two_nums_add_try(p_ev_data_out, p_ev_data_out, &term, TRUE /*propogate_errors*/))
            ev_data_set_error(p_ev_data_out, EVAL_ERR_CALC_FAILURE);

        if(ev_data_is_error(p_ev_data_out))
            break;
    }
}

PROC_EXEC_PROTO(c_poisson_dist)
{
    const S32 x = args[0]->arg.integer;
    const F64 lambda = args[1]->arg.fp;
    const BOOL cumulative = (n_args > 2) ? args[2]->arg.boolean : TRUE;

    exec_func_ignore_parms();

    if( (x < 0) ||
        (lambda <= 0.0) )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    poisson_dist_calc(p_ev_data_res, x, lambda, cumulative); /* may return fp or error */
}

/* end of ev_fnstd.c */
