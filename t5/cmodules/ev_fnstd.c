/* ev_fnstd.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2021 Stuart Swales */

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
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
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
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
    _InVal_     S32 n,
    _InVal_     F64 p,
    _InVal_     S32 k)
{
    S32 j;

    if( (n < 0) || (k < 0) || (k > n) ||
        (p < 0.0) || (p > 1.0) )
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    if(k == n)
    {
        ss_data_set_real(p_ss_data_out, 1.0);
        return;
    }

    if(n > 12)
    {   /* can save time using incomplete beta function */
        const F64 one_minus_p = 1.0 - p;
        const S32 n_minus_k = n - k;
        const S32 k_plus_one = k + 1;
        ss_data_set_real(p_ss_data_out, mx_Ix_beta(one_minus_p, n_minus_k, k_plus_one));
        return;
    }

    *p_ss_data_out = ss_data_real_zero;

    /* sum of PMF over [0..k] */
    for(j = 0; j <= k; ++j)
    {
        SS_DATA term;

        binom_dist_pmf_calc(&term, n, p, j); /* may return real or error */

        if(!two_nums_add_propagate_error(p_ss_data_out, p_ss_data_out, &term))
            ss_data_set_error(p_ss_data_out, EVAL_ERR_CALC_FAILURE);

        if(ss_data_is_error(p_ss_data_out))
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
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
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

    ss_data_set_real(p_ss_data_out, exp(ln_term));

    /* exp() overflowed? - don't test for underflow case */
    if(ss_data_get_real(p_ss_data_out) == F64_HUGE_VAL)
        ss_data_set_error(p_ss_data_out, EVAL_ERR_OUTOFRANGE);
}

static void
binom_dist_pmf_calc(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
    _InVal_     S32 n,
    _InVal_     F64 p,
    _InVal_     S32 k)
{
    SS_DATA binomial_term, power_term_k, power_term_n_minus_k;
    const S32 n_minus_k = (n - k);

    if( (n < 0) || (k < 0) || (k > n) ||
        (p < 0.0) || (p > 1.0) )
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    if(n > 1000)
    {   /* binomial term near to overflow and power term product near to underflow */
        alternate_binom_dist_pmf_calc(p_ss_data_out, n, p, k); /* may return real or error */
        return;
    }

    binomial_coefficient_calc(&binomial_term, n, k); /* may return integer or real or error */

    errno = 0;

    ss_data_set_real(&power_term_k, pow(p, k));
    ss_data_set_real(&power_term_n_minus_k, pow(1.0 - p, n_minus_k));

    if(errno /* == EDOM */)
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    if( !two_nums_multiply_propagate_error(p_ss_data_out, &power_term_k, &power_term_n_minus_k) ||
        !two_nums_multiply_propagate_error(p_ss_data_out, p_ss_data_out, &binomial_term) )
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_CALC_FAILURE);
        return;
    }
}

static void
binom_dist_range_calc(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
    _InVal_     S32 n,
    _InVal_     F64 p,
    _InVal_     S32 S,
    _InVal_     S32 S2)
{
    S32 k;

    if(S == 0)
    {   /* sum(PMF[0..S2]) is the CDF */
        binom_dist_cdf_calc(p_ss_data_out, n, p, S2); /* may return real or error */
        return;
    }

    if( (n < 0) || (S < 0) || (S2 < 0) ||
        (S > S2) || (S2 > n) ||
        (p < 0.0) || (p > 1.0) )
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    if((S2 == n) && (n > 12))
    {   /* sum(PMF[S..n]) */ /* can save time using incomplete beta function */
        S32 n_minus_k_plus_one;
        k = S;
        n_minus_k_plus_one = n - k + 1;
        ss_data_set_real(p_ss_data_out, mx_Ix_beta(p, k, n_minus_k_plus_one));
        return;
    }

    *p_ss_data_out = ss_data_real_zero;

    /* sum of PMF over [S..S2] */
    for(k = S; k <= S2; ++k)
    {
        SS_DATA term;

        binom_dist_pmf_calc(&term, n, p, k); /* may return real or error */

        if(!two_nums_add_propagate_error(p_ss_data_out, p_ss_data_out, &term))
            ss_data_set_error(p_ss_data_out, EVAL_ERR_CALC_FAILURE);

        if(ss_data_is_error(p_ss_data_out))
            break;
    }
}

PROC_EXEC_PROTO(c_binom_dist)
{
    const S32 S = ss_data_get_integer(args[0]); /*number_success*/
    const S32 n = ss_data_get_integer(args[1]); /*number_trials*/
    const F64 p = ss_data_get_real(args[2]); /*probability_success*/
    const bool cumulative = ss_data_get_logical(args[3]);

    exec_func_ignore_parms();

    if(cumulative)
        binom_dist_cdf_calc(p_ss_data_res, n, p, S); /* may return real or error */
    else
        binom_dist_pmf_calc(p_ss_data_res, n, p, S); /* may return real or error */
}

PROC_EXEC_PROTO(c_binom_dist_range)
{
    const S32 n = ss_data_get_integer(args[0]); /*number_trials*/
    const F64 p = ss_data_get_real(args[1]); /*probability_success*/
    const S32 S = ss_data_get_integer(args[2]);
    const S32 S2 = (n_args > 3) ? ss_data_get_integer(args[3]) : S;

    exec_func_ignore_parms();

    if(S == S2)
    {   /* just PMF at S */
        binom_dist_pmf_calc(p_ss_data_res, n, p, S); /* may return real or error */
        return;
    }

    binom_dist_range_calc(p_ss_data_res, n, p, S, S2); /* may return real or error */
}

/******************************************************************************
*
* NUMBER binom.inv(number_trials, probability_success, alpha)
*
******************************************************************************/

static void
binom_inv_calc(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
    _InVal_     S32 N, /*number_trials*/
    _InVal_     F64 P, /*probability_success*/
    _InVal_     F64 alpha)
{
    S32 k;

    assert((P >= 0.0) && (P <= 1.0));
    assert((alpha >= 0.0) && (alpha <= 1.0));

    //reportf(TEXT("binom_inv_calc(N=%d, P=%f, alpha=%f)"), N, P, alpha);
    for(k = 0; k <= N; ++k)
    {
        SS_DATA ss_data;
        F64 CDF_k;

        /* could improve using bisection */
        binom_dist_cdf_calc(&ss_data, N, P, k); /* may return real or error */

        if(ss_data_is_error(&ss_data))
        {
            *p_ss_data_out = ss_data;
            return;
        }

        assert(ss_data_is_real(&ss_data));
        CDF_k = ss_data_get_real(&ss_data);

        if(CDF_k >= alpha)
        {
            //reportf(TEXT("binom_inv_calc(N=%d, P=%f, alpha=%f) returns k=%d"), N, P, alpha, k);
            ss_data_set_integer(p_ss_data_out, k);
            return;
        }
    }

    ss_data_set_error(p_ss_data_out, EVAL_ERR_ARGRANGE);
}

PROC_EXEC_PROTO(c_binom_inv)
{
    const S32 N = ss_data_get_integer(args[0]); /*number_trials*/
    const F64 P = ss_data_get_real(args[1]); /*probability_success*/
    const F64 alpha = ss_data_get_real(args[2]);

    exec_func_ignore_parms();

    binom_inv_calc(p_ss_data_res, N, P, alpha); /* may return real or error */
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
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
    _InVal_     S32 s,
    _InVal_     S32 n,
    _InVal_     S32 M,
    _InVal_     S32 N,
    _InVal_     F64 ln_denominator)
{
    const F64 ln_numerator_lhs = mx_ln_binomial_coefficient(M, s);
    const F64 ln_numerator_rhs = mx_ln_binomial_coefficient(N - M, n - s);
    const F64 ln_numerator = ln_numerator_lhs + ln_numerator_rhs;
    const F64 ln_term = ln_numerator - ln_denominator;

    ss_data_set_real(p_ss_data_out, exp(ln_term));

    /* exp() overflowed? - don't test for underflow case */
    if(ss_data_get_real(p_ss_data_out) == F64_HUGE_VAL)
        ss_data_set_error(p_ss_data_out, EVAL_ERR_OUTOFRANGE);
}

static void
alternate_hypgeom_dist_calc(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
    _InVal_     S32 x /*sample_successes*/,
    _InVal_     S32 n /*number_sample*/,
    _InVal_     S32 M /*population_successes*/,
    _InVal_     S32 N /*number_population*/,
    _InVal_     bool cumulative)
{
    F64 ln_denominator;
    S32 i;

    *p_ss_data_out = ss_data_real_zero;

    errno = 0;

    ln_denominator = mx_ln_binomial_coefficient(N, n);

    /* CDF is the sum from the terms over [0..x]; PMF is just the single term at x  */
    for(i = cumulative ? 0 : x; i <= x; ++i)
    {
        SS_DATA ss_data_term;

        alternate_hypgeom_dist_pdf_calc(&ss_data_term, i, n, M, N, ln_denominator); /* may return real or error */

        if(!two_nums_add_propagate_error(p_ss_data_out, p_ss_data_out, &ss_data_term))
            ss_data_set_error(p_ss_data_out, EVAL_ERR_CALC_FAILURE);

        if(ss_data_is_error(p_ss_data_out))
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
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return integer or real or error */
    _InVal_     S32 s,
    _InVal_     S32 n,
    _InVal_     S32 M,
    _InVal_     S32 N,
    _InRef_     P_SS_DATA denominator)
{
    SS_DATA numerator_lhs, numerator_rhs, numerator;

    binomial_coefficient_calc(&numerator_lhs, M, s);            /* may return integer or real or error */
    binomial_coefficient_calc(&numerator_rhs, N - M, n - s);    /* may return integer or real or error */

    if( !two_nums_multiply_propagate_error(&numerator, &numerator_lhs, &numerator_rhs) ||
        !two_nums_divide_propagate_error(p_ss_data_out, &numerator, denominator) )
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_CALC_FAILURE);
    }
}

static void
hypgeom_dist_calc(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
    _InVal_     S32 x /*sample_successes*/,
    _InVal_     S32 n /*number_sample*/,
    _InVal_     S32 M /*population_successes*/,
    _InVal_     S32 N /*number_population*/,
    _InVal_     bool cumulative)
{
    SS_DATA denominator;
    S32 i;

    if(n > 170)
    {
        alternate_hypgeom_dist_calc(p_ss_data_out, x, n, M, N, cumulative); /* may return real or error */
        return;
    }

    binomial_coefficient_calc(&denominator, N, n); /* may return integer or real or error */

    *p_ss_data_out = ss_data_real_zero;

    /* CDF is the sum from the terms over [0..x]; PMF is just the single term at x  */
    for(i = cumulative ? 0 : x; i <= x; ++i)
    {
        SS_DATA term;

        hypgeom_dist_pdf_calc(&term, i, n, M, N, &denominator); /* may return integer or real or error */

        if(!two_nums_add_propagate_error(p_ss_data_out, p_ss_data_out, &term))
            ss_data_set_error(p_ss_data_out, EVAL_ERR_CALC_FAILURE);

        if(ss_data_is_error(p_ss_data_out))
            break;
    }
}

PROC_EXEC_PROTO(c_hypgeom_dist)
{
    const S32 x = ss_data_get_integer(args[0]); /*sample_successes*/
    const S32 n = ss_data_get_integer(args[1]); /*number_sample*/
    const S32 M = ss_data_get_integer(args[2]); /*population_successes*/
    const S32 N = ss_data_get_integer(args[3]); /*number_population*/
    const bool cumulative = (n_args > 4) ? ss_data_get_logical(args[4]) : false;

    exec_func_ignore_parms();

    if( (x < 0) || (n < 0) || (M < 0) || (N < 0) ||
        (n > N) ||
        (M > N) ||
        (x > MIN(n, M)) ||
        (x < MAX(0, n - N + M)) )
    {
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
    }

    hypgeom_dist_calc(p_ss_data_res, x, n, M, N, cumulative); /* may return real or error */
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
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
    _InVal_     S32 k,
    _InVal_     S32 r,
    _InVal_     F64 p)
{
    SS_DATA binomial_term, power_term_r, power_term_k;

    binomial_coefficient_calc(&binomial_term, k + r - 1, k); /* may return integer or real or error */

    errno = 0;

    ss_data_set_real(&power_term_r, pow(p, r));
    ss_data_set_real(&power_term_k, pow(1.0 - p, k));

    if(errno /* == EDOM */)
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    if( !two_nums_multiply_propagate_error(p_ss_data_out, &power_term_r, &power_term_k) ||
        !two_nums_multiply_propagate_error(p_ss_data_out, p_ss_data_out, &binomial_term) )
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_CALC_FAILURE);
        return;
    }
}

static void
negbinom_dist_calc(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
    _InVal_     S32 x,
    _InVal_     S32 r,
    _InVal_     F64 p,
    _InVal_     bool cumulative)
{
    S32 k;

    if( ((x + r - 1) <= 0) ||
        (p < 0.0) || (p > 1.0) )
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    *p_ss_data_out = ss_data_real_zero;

    /* CDF is the sum from the terms over [0..x]; PMF is just the single term at x */
    for(k = cumulative ? 0 : x; k <= x; ++k)
    {
        SS_DATA term;

        negbinom_dist_pdf_calc(&term, k, r, p); /* may return real or error */

        if(!two_nums_add_propagate_error(p_ss_data_out, p_ss_data_out, &term))
            ss_data_set_error(p_ss_data_out, EVAL_ERR_CALC_FAILURE);

        if(ss_data_is_error(p_ss_data_out))
            break;
    }
}

PROC_EXEC_PROTO(c_negbinom_dist)
{
    const S32 x = ss_data_get_integer(args[0]); /*number of failures*/
    const S32 r = ss_data_get_integer(args[1]); /*threshold number of successes*/
    const F64 p = ss_data_get_real(args[2]); /*probability of success*/
    const bool cumulative = (n_args > 3) ? ss_data_get_logical(args[3]) : false;

    exec_func_ignore_parms();

    negbinom_dist_calc(p_ss_data_res, x, r, p, cumulative); /* may return real or error */
}

/******************************************************************************
*
* NUMBER poisson.dist(x:number, lambda:number {, cumulative:Logical=TRUE})
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
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
    _InVal_     S32 x,
    _InVal_     F64 lambda,
    _InVal_     bool cumulative)
{
    const F64 ln_exp_term = -lambda; /* this exp() won't overflow, ignore underflow */
    const bool more_logs = (x > 20);
    S32 k;

    *p_ss_data_out = ss_data_real_zero;

    /* CDF is the sum from the terms over [0..x]; PMF is just the single term at x */
    for(k = cumulative ? 0 : x; k <= x; ++k)
    {
        const F64 ln_power_term = log(lambda) * k;
        const F64 ln_numerator = ln_power_term + ln_exp_term;
        SS_DATA term;

        if(more_logs)
        {
            const F64 ln_denominator = mx_ln_factorial(k);

            ss_data_set_real(&term, exp(ln_numerator - ln_denominator));
        }
        else
        {
            SS_DATA numerator, denominator;

            ss_data_set_real(&numerator, exp(ln_numerator));

            factorial_calc(&denominator, k); /* may return integer or real or error */

            if(!two_nums_divide_propagate_error(&term, &numerator, &denominator))
                ss_data_set_error(&term, EVAL_ERR_CALC_FAILURE);
        }

        if(!two_nums_add_propagate_error(p_ss_data_out, p_ss_data_out, &term))
            ss_data_set_error(p_ss_data_out, EVAL_ERR_CALC_FAILURE);

        if(ss_data_is_error(p_ss_data_out))
            break;
    }
}

PROC_EXEC_PROTO(c_poisson_dist)
{
    const S32 x = ss_data_get_integer(args[0]);
    const F64 lambda = ss_data_get_real(args[1]);
    const bool cumulative = (n_args > 2) ? ss_data_get_logical(args[2]) : true;

    exec_func_ignore_parms();

    if( (x < 0) || (lambda <= 0.0) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    poisson_dist_calc(p_ss_data_res, x, lambda, cumulative); /* may return real or error */
}

/* end of ev_fnstd.c */
