/* ev_fnstc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2015 Stuart Swales */

/* Statistical function routines (distributions etc) for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "cmodules/mathxtra.h" /* for mx_fsquare() */
#include "cmodules/mathxtr4.h" /* for mx_Ix_beta() */

#include "cmodules/mathxtri.h"

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h"
#endif

/******************************************************************************
*
* Statistical functions - Continuous distributions
*
******************************************************************************/

extern double erf(double); /* for normal-based CDFs */

_Check_return_
static F64
calc_gamma_cdf(
    _InVal_     F64 x,
    _InVal_     F64 shape,
    _InVal_     F64 scale);

_Check_return_
static F64
calc_gamma_pdf(
    _InVal_     F64 x,
    _InVal_     F64 shape,
    _InVal_     F64 scale);

static void
gamma_inv_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     F64 p,
    _InVal_     F64 shape,
    _InVal_     F64 scale);

_Check_return_
static inline F64
calc_norm_std_cdf(
    _InVal_     F64 z)
{
    const F64 erf_term = erf(z / _sqrt_two);
    const F64 cdf = 0.5 + (0.5 * erf_term);

    return(cdf);
}

_Check_return_
static inline F64
calc_norm_std_pdf(
    _InVal_     F64 z)
{
    const F64 z2 = (z * z);
    const F64 exp_term = exp(-z2 * 0.5);
    static const F64 reciprocal_denominator = (1.0 / _sqrt_two_pi);
    const F64 pdf = exp_term * reciprocal_denominator;

    return(pdf);
}

static void
norm_std_inv_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     F64 p);

static void
t_inv_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     F64 p,
    _InVal_     F64 k);

/******************************************************************************
*
* NUMBER beta.dist(x, alpha, beta {, a {, b {, cumulative}}})
*
* See OpenDocument 1.2 definition of BETADIST
*
* A&S 26.1.33
*
******************************************************************************/

/* beta is a continuous distribution with
 *
 * CDF(x;alpha,beta)
 *  = I(x;alpha,beta)
 *    where Ix(alpha,beta) is the regularized incomplete beta function
 */

_Check_return_
static inline F64
calc_beta_std_cdf(
    _InVal_     F64 x, /* in [0..1] */
    _InVal_     F64 alpha, /* alpha and beta are shape parameters */
    _InVal_     F64 beta)
{
    errno = 0;

    /* OpenDocument compatibility for x outwith [0,1] */
    if(x <= 0.0)
        return(0.0); /* PDF is zero everywhere below 0, so CDF is zero */

    if(x >= 1.0)
        return(1.0); /* PDF is zero everywhere above 1, so CDF is one */

    return(mx_Ix_beta(x, alpha, beta));
}

_Check_return_
static F64
calc_beta_cdf(
    _InVal_     F64 x_value, /* in [a..b] */
    _InVal_     F64 alpha, /* alpha and beta are shape parameters */
    _InVal_     F64 beta,
    _InVal_     F64 a, /* a and b translate and scale */
    _InVal_     F64 b)
{
    assert((b - a) > 0.0);

    {
    const F64 scale = b - a;
    const F64 x = (x_value - a) / scale; /* value now standardized */

    return(calc_beta_std_cdf(x, alpha, beta));
    } /*block*/
}

/* beta is a continuous distribution with
 *
 * PDF(x;alpha,beta)
 *  = (G(alpha + beta) / G(alpha) . G(beta)) 
 *    x ^ (alpha - 1) .
 *    (1 - x) ^ (beta - 1)
 *    for x E [0,1] and
 *    where G(x) is the gamma function
 *
 * PDF(x;alpha,beta,a,b)
 *  = (G(alpha + beta) / G(alpha) . G(beta)) 
 *    ((x - a) / (b - a)) ^ (alpha - 1) .
 *    (1 - ((x - a) / (b - a))) ^ (beta - 1) .
 *    (1 / (b - a))
 *    for x E [a,b]
 *
 * You will often see the last three terms written as
 *    (x - a) ^ (alpha - 1) .
 *    (b - x) ^ (beta - 1) .
 *    (b - a) ^ -(alpha + beta - 1)
 * but the former is verifiably the same and easier to relate the the std PDF
 */

_Check_return_
static F64
calc_beta_std_pdf(
    _InVal_     F64 x, /* in [0..1] */
    _InVal_     F64 alpha, /* alpha and beta are shape parameters */
    _InVal_     F64 beta)
{
    assert((alpha > 0.0) && (beta > 0.0));

    /* Care not to take log of zero; also OpenDocument compatibility for x outwith [0,1] */
    if(x <= 0.0)
    {
        if(x == 0.0)
            return((alpha == 1.0) ? beta : (alpha > 1.0) ? 0.0 : HUGE_VAL /* pole at zero for alpha < 1 */);

        return(0.0); /* PDF is zero everywhere outwith [0,1] */
    }

    if(x >= 1.0)
    {
        if(x == 1.0)
            return((beta == 1.0) ? alpha : (beta > 1.0) ? 0.0 : HUGE_VAL /* pole at one for beta < 1 */);

        return(0.0); /* PDF is zero everywhere outwith [0,1] */
    }

    {
    const F64 ln_reciprocal_beta_term = mx_ln_reciprocal_beta(alpha, beta);
    const F64 ln_alpha_power_term = log(x) * (alpha - 1);
    const F64 ln_beta_power_term = log(1.0 - x) * (beta - 1);
    const F64 pdf = exp(ln_reciprocal_beta_term + ln_alpha_power_term + ln_beta_power_term);

    return(pdf);
    } /*block*/
}

_Check_return_
static F64
calc_beta_pdf(
    _InVal_     F64 x_value, /* in [a..b] */
    _InVal_     F64 alpha, /* alpha and beta are shape parameters */
    _InVal_     F64 beta,
    _InVal_     F64 a, /* a and b translate and scale */
    _InVal_     F64 b)
{
    assert((b - a) > 0.0);

    {
    const F64 scale = b - a;
    const F64 x = (x_value - a) / scale; /* value now standardized */
    const F64 std_pdf = calc_beta_std_pdf(x, alpha, beta);
    const F64 pdf = std_pdf / scale;

    return(pdf);
    } /*block*/
}

static void
beta_dist_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     F64 x,
    _InVal_     F64 alpha, /* alpha and beta are shape parameters */
    _InVal_     F64 beta,
    _InVal_     BOOL cumulative,
    _InVal_     F64 a, /* a and b translate and scale from standard distribution on [0,1] with (alpha, beta) */
    _InVal_     F64 b)
{
    F64 beta_dist_result;

    if( (alpha <= 0.0) ||
        (beta <= 0.0)  ||
        (a >= b)       )
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    if(cumulative)
    {
        beta_dist_result = calc_beta_cdf(x, alpha, beta, a, b);
    }
    else
    {
        if( ((x == a) && (alpha < 1.0)) ||
            ((x == b) && (beta  < 1.0)) )
        {
            ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
            return;
        }

        beta_dist_result = calc_beta_pdf(x, alpha, beta, a, b);
    }

    ev_data_set_real(p_ev_data_out, beta_dist_result);
}

PROC_EXEC_PROTO(c_beta_dist)
{
    const F64 x = args[0]->arg.fp;
    const F64 alpha = args[1]->arg.fp;
    const F64 beta = args[2]->arg.fp;
    const BOOL cumulative = (n_args > 3) ? args[3]->arg.boolean : TRUE;
    const F64 a = (n_args > 4) ? args[4]->arg.fp : 0.0;
    const F64 b = (n_args > 5) ? args[5]->arg.fp : 1.0;

    exec_func_ignore_parms();

    beta_dist_calc(p_ev_data_res, x, alpha, beta, cumulative, a, b); /* may return fp or error */
}

PROC_EXEC_PROTO(c_odf_betadist)
{
    const F64 x = args[0]->arg.fp;
    const F64 alpha = args[1]->arg.fp;
    const F64 beta = args[2]->arg.fp;
    const F64 a = (n_args > 3) ? args[3]->arg.fp : 0.0;
    const F64 b = (n_args > 4) ? args[4]->arg.fp : 1.0;
    const BOOL cumulative = (n_args > 5) ? args[5]->arg.boolean : TRUE;

    exec_func_ignore_parms();

    beta_dist_calc(p_ev_data_res, x, alpha, beta, cumulative, a, b); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER beta.inv(p:number, alpha:number, beta:number {, a:number, b:number})
*
******************************************************************************/

/* Newton-Raphson method - solve for F(x) = 0 where F(x;a,b) = CDF(x;a,b) - p */

static void
beta_std_inv_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     F64 p,
    _InVal_     F64 alpha,
    _InVal_     F64 beta)
{
    F64 x;
    F64 lower_bracket = 0.0; /* result is in [0,1] */
    F64 upper_bracket = 1.0;
    U32 iteration_count;

    /* there is guaranteed to be a unique solution for p in [0,1] */
    /* or (0,1] | [0,1) | (0,1) if alpha < 1 | beta < 1 | both */
    assert((p >= 0.0) && (p <= 1.0));
    assert((alpha > 0.0) && (beta > 0.0));

    if(alpha < 1.0) /* avoid pole at zero */
        lower_bracket += F64_EPSILON;

    if(beta < 1.0) /* avoid pole at one */
        upper_bracket -= F64_EPSILON;

    if(p == 0.0)
    {
        x = lower_bracket;
        ev_data_set_real(p_ev_data_out, x);
        return;
    }

    if(p == 1.0)
    {
        x = upper_bracket;
        ev_data_set_real(p_ev_data_out, x);
        return;
    }

    /* for 1 < alpha < beta */
    /* mode <= median <= mean */
    /* for 1 < beta < alpha */
    /* mean <= median <= mode */
    /* where the mode is (alpha - 1)/(alpha + beta - 2) and the mean is alpha/(alpha + beta) */
    /* NB for a < 1 there is no mode */

    /* choose an initial guess the right side of the median if we can */
    if(alpha == beta)
    {   /* symmetric */
        /*median = 0.5;*/
        x = ((p < 0.5) ? 0.25 : 0.75);
    }
    else if(alpha == 1.0)
    {   /* these are all well over to the left of [0,1] for beta > 1 */
        F64 median = 1.0 - pow(2.0, -1.0/beta);
        x = median;
        if(p < 0.5)
            x /= 2.0;
    }
    else if(beta == 1.0)
    {   /* these are all well over to the right of [0,1] for alpha > 1 */
        F64 median = pow(2.0, -1.0/alpha);
        x = median;
        if(p < 0.5)
            x /= 2.0;
    }
    else if((alpha >= 1.0) && (beta >= 1.0))
    {   /* a very useful new approximation: Kerman (2011) (arXiv:1111.0433v1), error < 4% */
        F64 mode = (alpha - 1.0) / (alpha + beta - 2.0);
        F64 mean = alpha / (alpha + beta);
        F64 median_est = (alpha - 1.0/3.0) / (alpha + beta - 2.0/3.0);
        x = median_est;
        if(p < 0.5)
            x = (alpha < beta) ? mode : mean;
        else if(p > 0.5)
            x = (alpha < beta) ? mean : mode;
    }
    else
    {   /* no idea */
        /*median = 0.5;*/
        x = ((p < 0.5) ? 0.25 : 0.75);
    }

    reportf(TEXT("beta_std_inv_calc(p=%f, alpha=%f, beta=%f): initial_x=%f"), p, alpha, beta, x);
    for(iteration_count = 0; iteration_count < 100; ++iteration_count)
    {
        const F64 CDF_x = calc_beta_std_cdf(x, alpha, beta);
        const F64 F_x = CDF_x - p;
        const F64 d_F_x = calc_beta_std_pdf(x, alpha, beta); /* derivative of the CDF is the PDF */
        F64 delta_x, x1;

        if(d_F_x == 0.0)
            break;

        /* we can improve the bracketing constraints on the solution at each step */
        if(F_x < 0.0)
        {
            //reportf("refine [,] from [l=%f to [l=%f", lower_bracket, x);
            lower_bracket = x;
        }
        else
        {
            //reportf("refine [,] from u=%f] to u=%f]", upper_bracket, x);
            upper_bracket = x;
        }

        delta_x = - (F_x / d_F_x);
        x1 = x + delta_x;

        if(x1 < lower_bracket)
        {   /* constrain using bisection in [lower,x] */
            F64 new_delta_x = (lower_bracket - x) * 0.5; /*-ve*/
            reportf(TEXT("%u: CDF(x=%f;a,b)-p=%+f, CDF'=%f, dx=%+f (bisect [l=%f,x]), new_dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x, lower_bracket, new_delta_x);
            delta_x = new_delta_x;
            x1 = x + delta_x;
        }
        else if(x1 > upper_bracket)
        {   /* constrain using bisection in [x,upper] */
            F64 new_delta_x = (upper_bracket - x) * 0.5; /*+ve*/
            reportf(TEXT("%u: CDF(x=%f;a,b)-p=%+f, CDF'=%f, dx=%+f (bisect [x,u=%f]), new_dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x, upper_bracket, new_delta_x);
            delta_x = new_delta_x;
            x1 = x + delta_x;
        }
        else
        {
            reportf(TEXT("%u: CDF(x=%f;a,b)-p=%+f, CDF'=%f, dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x);
        }

        x = x1;

        if(fabs(delta_x) <= 3.0E-7)
        {
            ev_data_set_real(p_ev_data_out, x);
            return;
        }
    }

    ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
}

PROC_EXEC_PROTO(c_beta_inv)
{
    const F64 p = args[0]->arg.fp;
    const F64 alpha = args[1]->arg.fp;
    const F64 beta = args[2]->arg.fp;
    const F64 a = (n_args > 3) ? args[3]->arg.fp : 0.0;
    const F64 b = (n_args > 4) ? args[4]->arg.fp : 1.0;

    exec_func_ignore_parms();

    if((p < 0.0) || (p > 1.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if( (alpha <= 0.0) ||
        (beta <= 0.0)  ||
        (a >= b)       )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    beta_std_inv_calc(p_ev_data_res, p, alpha, beta); /* may return fp or error */

    if(!ev_data_is_error(p_ev_data_res) && ((a != 0.0) || (b != 1.0)))
    {   /* scale and shift standardized result from [0..1] into [a..b] */
        const F64 scale = b - a;
        p_ev_data_res->arg.fp = (p_ev_data_res->arg.fp * scale) + a;
    }
}

/******************************************************************************
*
* NUMBER chisq.dist(x, k:integer {, cumulative})
*
* See OpenDocument 1.2 definition of CHISQDIST
*
* A&S 26.4
*
******************************************************************************/

/* chi-squared is a continuous distribution with
 *
 * CDF(x;k)
 *  = P(k / 2, x / 2)
 *    where P(s,x) is a regularized gamma function
 */

#if 0

_Check_return_
static F64
calc_chisq_cdf(
    _InVal_     F64 x,
    _InVal_     S32 k)
{
    const F64 half_x = x * 0.5;
    const F64 half_k = k * 0.5;
    const F64 cdf = mx_P_gamma(half_k, half_x);

    return(cdf);
}

#endif

/* chi-squared is a continuous distribution with
 *
 * PDF(x;k)
 *  = (1 / (G(k/2) . (2 ^ (k/2))) .
 *    x ^ ((k/2) - 1) .
 *    exp(-(x/2))
 *    where G(x) is the gamma function
 */

#if 0

_Check_return_
static F64
calc_chisq_pdf(
    _InVal_     F64 x,
    _InVal_     S32 k)
{
    const F64 half_x = x * 0.5;
    const F64 half_k = k * 0.5;
    const F64 numerator_power_term = pow(x, half_k - 1.0);
    const F64 numerator_exp_term = exp(-half_x);
    const F64 denominator_gamma_term = tgamma(half_k);
    const F64 denominator_power_term = pow(2.0, half_k);
    const F64 pdf = (numerator_power_term * numerator_exp_term) / (denominator_gamma_term * denominator_power_term);

    return(pdf);
}

#endif

PROC_EXEC_PROTO(c_chisq_dist)
{
    const F64 x = args[0]->arg.fp;
    const S32 k = args[1]->arg.integer;
    const BOOL cumulative = (n_args > 2) ? args[2]->arg.boolean : TRUE;
    const F64 shape = (k * 0.5);
    const F64 scale = 2.0;
    F64 chisq_dist_result;

    exec_func_ignore_parms();

    if(k <= 0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if(cumulative)
        chisq_dist_result = calc_gamma_cdf(x, shape, scale);
    else
        chisq_dist_result = calc_gamma_pdf(x, shape, scale);

    ev_data_set_real(p_ev_data_res, chisq_dist_result);
}

/******************************************************************************
*
* NUMBER chisq.dist.rt(x, k:integer)
*
* See OpenDocument 1.2 definition of LEGACY.CHIDIST
*
******************************************************************************/

PROC_EXEC_PROTO(c_chisq_dist_rt)
{
    const F64 x = args[0]->arg.fp;
    const S32 k = args[1]->arg.integer;
    const F64 shape = (k * 0.5);
    const F64 scale = 2.0;
    F64 chisq_dist_rt_result;

    exec_func_ignore_parms();

    if(k <= 0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    chisq_dist_rt_result = 1.0 - calc_gamma_cdf(x, shape, scale);

    ev_data_set_real(p_ev_data_res, chisq_dist_rt_result);
}

/******************************************************************************
*
* NUMBER chisq.inv(p:number, k:integer)
*
******************************************************************************/

PROC_EXEC_PROTO(c_chisq_inv)
{
    const F64 p = args[0]->arg.fp;
    const S32 k = args[1]->arg.integer;
    const F64 shape = (k * 0.5);
    const F64 scale = 2.0;

    exec_func_ignore_parms();

    if((p < 0.0) || (p > 1.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if( (shape <= 0.0) ||
        (scale <= 0.0) )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    gamma_inv_calc(p_ev_data_res, p, shape, scale); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER chisq.inv.rt(p:number, k:integer)
*
******************************************************************************/

PROC_EXEC_PROTO(c_chisq_inv_rt)
{
    const F64 p = args[0]->arg.fp;
    const S32 k = args[1]->arg.integer;
    const F64 shape = (k * 0.5);
    const F64 scale = 2.0;

    exec_func_ignore_parms();

    if((p < 0.0) || (p > 1.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if( (shape <= 0.0) ||
        (scale <= 0.0) )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    gamma_inv_calc(p_ev_data_res, 1.0 - p, shape, scale); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER chisq.test(actual:array, expected:array)
*
* See OpenDocument 1.2 definition of LEGACY.CHITEST
*
******************************************************************************/

static void
chisq_test_calc(
    _InoutRef_  P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA array_a,
    _InRef_     PC_EV_DATA array_e)
{
    EV_DATA ev_data_a, ev_data_e;
    S32 x_size[2];
    S32 y_size[2];
    S32 ix, iy;
    EV_DATA chi_squared_statistic, degrees_of_freedom;

    array_range_sizes(array_a, &x_size[0], &y_size[0]);
    array_range_sizes(array_e, &x_size[1], &y_size[1]);

    if((x_size[0] != x_size[1]) || (y_size[0] != y_size[1]))
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ODF_NA);
        return;
    }

    chi_squared_statistic = ev_data_real_zero;

    ix = -1; iy = 0;

    while(statistics_paired_values_next(&ev_data_a, &ev_data_e, array_a, array_e, &ix, &iy, x_size[0], y_size[0]))
    {
        const F64 delta = ev_data_a.arg.fp - ev_data_e.arg.fp;

        chi_squared_statistic.arg.fp += mx_fsquare(delta) / ev_data_e.arg.fp;
    }

    if((y_size[0] > 1) && (x_size[0] > 1))
        ev_data_set_integer(&degrees_of_freedom, (y_size[0] - 1) * (x_size[0] - 1));
    else
        ev_data_set_integer(&degrees_of_freedom, (y_size[0] * x_size[0]) - 1);

    { /* call my friend to do the hard work */
    static const EV_SLR dummy_slr = EV_SLR_INIT;
    P_EV_DATA args[2];
    args[0] = &chi_squared_statistic;
    args[1] = &degrees_of_freedom;
    c_chisq_dist_rt(args, 2, p_ev_data_out, &dummy_slr);
    } /*block*/
}

PROC_EXEC_PROTO(c_chisq_test)
{
    const PC_EV_DATA array_actual = args[0];
    const PC_EV_DATA array_expected = args[1];

    exec_func_ignore_parms();

    chisq_test_calc(p_ev_data_res, array_actual, array_expected);
}

/******************************************************************************
*
* NUMBER confidence.norm(alpha:number, standard_deviation:number, size:number)
*
* See OpenDocument 1.2 definition of CONFIDENCE
*
******************************************************************************/

PROC_EXEC_PROTO(c_confidence_norm)
{
    const F64 alpha = args[0]->arg.fp;
    const F64 sigma = args[1]->arg.fp;
    const F64 size = floor(args[2]->arg.fp);

    exec_func_ignore_parms();

    if( (sigma <= 0.0) ||
        (size <= 0.0)  )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    norm_std_inv_calc(p_ev_data_res, 1.0 - (alpha * 0.5)); /* may return fp or error */

    if(!ev_data_is_error(p_ev_data_res))
    {
        const F64 z = p_ev_data_res->arg.fp;
        const F64 scale = sigma / sqrt(size);
        const F64 confidence_norm_result = z * scale;

        ev_data_set_real(p_ev_data_res, confidence_norm_result);
    }
}

/******************************************************************************
*
* NUMBER confidence.t(alpha:number, standard_deviation:number, size:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_confidence_t)
{
    const F64 alpha = args[0]->arg.fp;
    const F64 sigma = args[1]->arg.fp;
    const F64 size = floor(args[2]->arg.fp);

    exec_func_ignore_parms();

    if( (sigma <= 0.0) ||
        (size <= 1.0)  )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    t_inv_calc(p_ev_data_res, 1.0 - (alpha * 0.5), size - 1.0); /* may return fp or error */

    if(!ev_data_is_error(p_ev_data_res))
    {
        const F64 t = p_ev_data_res->arg.fp;
        const F64 scale = sigma / sqrt(size);
        const F64 confidence_t_result = t * scale;

        ev_data_set_real(p_ev_data_res, confidence_t_result);
    }
}

/******************************************************************************
*
* NUMBER expon.dist(x:number, lambda:number {, cumulative:Boolean=TRUE})
*
* See OpenDocument 1.2 definition of EXPONDIST
*
******************************************************************************/

PROC_EXEC_PROTO(c_expon_dist)
{
    const F64 x = args[0]->arg.fp;
    const F64 lambda = args[1]->arg.fp;
    const BOOL cumulative = (n_args > 2) ? args[2]->arg.boolean : TRUE;
    F64 expon_dist_result;

    exec_func_ignore_parms();

    if(x < 0)
    {
        expon_dist_result = 0; /* Excel would error here */
    }
    else
    {
        const F64 exp_term = exp(-lambda * x); /* this exp() won't overflow, ignore underflow */

        if(cumulative)
            expon_dist_result = 1.0 - exp_term;
        else
            expon_dist_result = lambda * exp_term;
    }

    ev_data_set_real(p_ev_data_res, expon_dist_result);
}

/******************************************************************************
*
* NUMBER f.dist(x, d1:number, d2:number {, cumulative})
*
* See OpenDocument 1.2 definition of FDIST
*
******************************************************************************/

/* F is a continuous distribution with
 *
 * CDF(x;d1,d2)
 *  = Iz(h1, h2)
 *    where Iz(a,b) is a regularized incomplete beta function and
 *    z = (x * d1) / ((x * d1) + d2)
 *    h1 = d1 / 2
 *    h2 = d2 / 2
 */

_Check_return_
static F64
calc_F_cdf(
    _InVal_     F64 x,
    _InVal_     F64 d1,
    _InVal_     F64 d2)
{
    if(x <= 0.0)
        return(0.0); /* Excel would error here for x < 0.0 */

    {
    const F64 h1 = d1 * 0.5;
    const F64 h2 = d2 * 0.5;
    const F64 x_d1 = x * d1;
    const F64 z = x_d1 / (x_d1 + d2);
    const F64 cdf = mx_Ix_beta(z, h1, h2);

    return(cdf);
    } /*block*/
}

/* F is a continuous distribution with
 *
 * PDF(x;d1,d2)
 *  = (1 / (B(h1, h2)) .
 *    (D ^ h1) .
 *    (x ^ (h1 - 1) .
 *    (1 + (x * D) ^ (-(h1 + h2))))
 *    where B(a,b) is the beta function and
 *    h1 = d1 / 2
 *    h2 = d2 / 2
 *    D = d1 / d2
 */

_Check_return_
static F64
calc_F_pdf(
    _InVal_     F64 x,
    _InVal_     F64 d1,
    _InVal_     F64 d2)
{
    if(x <= 0.0)
    {
        if((x == 0.0) && (d1 == 1.0)) /* pole at zero for d1 == 1 */
        {
            errno = EDOM;
            return(HUGE_VAL);
        }
        return(0.0);
    }

    {
    const F64 h1 = d1 * 0.5;
    const F64 h2 = d2 * 0.5;
    const F64 D = d1 / d2;
    const F64 ln_reciprocal_beta_term = mx_ln_reciprocal_beta(h1, h2);
    const F64 ln_numerator_power_term_1 = log(D) * h1;
    const F64 ln_numerator_power_term_2 = log(x) * (h1 - 1.0);
    const F64 ln_numerator_power_term_3 = log(1.0 + (x * D)) * (-(h1 + h2));
    const F64 pdf = exp(ln_reciprocal_beta_term + ln_numerator_power_term_1 + ln_numerator_power_term_2 + ln_numerator_power_term_3);

    return(pdf);
    } /*block*/
}

PROC_EXEC_PROTO(c_F_dist)
{
    const F64 x = args[0]->arg.fp;
    const F64 d1 = floor(args[1]->arg.fp);
    const F64 d2 = floor(args[2]->arg.fp);
    const BOOL cumulative = (n_args > 3) ? args[3]->arg.boolean : TRUE;
    F64 F_dist_result;

    exec_func_ignore_parms();

    if((d1 <= 0.0) || (d2 <= 0.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if(cumulative)
        F_dist_result = calc_F_cdf(x, d1, d2);
    else
    {
        if((d1 == 1.0) && (x == 0.0))
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
            return;
        }

        F_dist_result = calc_F_pdf(x, d1, d2);
    }

    ev_data_set_real(p_ev_data_res, F_dist_result);
}

/******************************************************************************
*
* NUMBER f.dist.rt(x, d1:number, d2:number)
*
* See OpenDocument 1.2 definition of LEGACY.FDIST
*
******************************************************************************/

PROC_EXEC_PROTO(c_F_dist_rt)
{
    const F64 x = args[0]->arg.fp;
    const F64 d1 = floor(args[1]->arg.fp);
    const F64 d2 = floor(args[2]->arg.fp);
    F64 F_dist_rt_result;

    exec_func_ignore_parms();

    if((d1 <= 0) || (d2 <= 0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    F_dist_rt_result = 1.0 - calc_F_cdf(x, d1, d2);

    ev_data_set_real(p_ev_data_res, F_dist_rt_result);
}

/******************************************************************************
*
* NUMBER f.inv(p:number, d1:number, d2:number)
*
******************************************************************************/

static void
F_inv_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     F64 p,
    _InVal_     F64 d1,
    _InVal_     F64 d2)
{
    F64 x;
    F64 lower_bracket = 0.0; /* result is in [0,+inf) */
    F64 upper_bracket = +F64_MAX;
    U32 iteration_count;

    /* there is guaranteed to be a unique solution for p in [0,1] */
    /* or in (0,1] when d1 == 0 */
    assert((p >= 0.0) && (p <= 1.0));
    assert((d1 > 0.0) && (d2 > 0.0));

    if(d1 == 1.0) /* avoid pole at zero when d1 == 1 */
        lower_bracket += F64_EPSILON;

    if(p == 0.0)
    {
        x = lower_bracket;
        ev_data_set_real(p_ev_data_out, x);
        return;
    }

    if(d2 > 2.0)
    {   /* try the mean */
        x = d2 / (d2 - 2.0);
    }
    else if(d1 > 2.0)
    {   /* try the mode */
        x = ((d1 - 2.0) / d1) * (d2 / (d2 + 2.0));
    }
    else
    {   /* no idea what to guess */
        /* looks like one is pretty close to the median for most values of similar d1,d2 */
        x = 1.0;
    }

    reportf(TEXT("F_inv_calc(p=%f, d1=%f, d2=%f): initial_x=%f"), p, d1, d2, x);
    for(iteration_count = 0; iteration_count < 100; ++iteration_count)
    {
        const F64 CDF_x = calc_F_cdf(x, d1, d2);
        const F64 F_x = CDF_x - p;
        const F64 d_F_x = calc_F_pdf(x, d1, d2); /* derivative of the CDF is the PDF */
        F64 delta_x, x1;

        if(d_F_x == 0.0)
            break;

        /* we can improve the bracketing constraints on the solution at each step */
        if(F_x < 0.0)
        {
            //reportf("refine [,] from [l=%g to [l=%f", lower_bracket, x);
            lower_bracket = x;
        }
        else
        {
            //reportf("refine [,] from u=%g] to u=%f]", upper_bracket, x);
            upper_bracket = x;
        }

        delta_x = - (F_x / d_F_x);
        x1 = x + delta_x;

        if(x1 < lower_bracket)
        {   /* constrain using bisection in [lower,x] */
            F64 new_delta_x = (lower_bracket - x) * 0.5; /*-ve*/
            reportf(TEXT("%u: CDF(x=%f;a,b)-p=%+f, CDF'=%f, dx=%+f (bisect [l=%f,x]), new_dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x, lower_bracket, new_delta_x);
            delta_x = new_delta_x;
            x1 = x + delta_x;
        }
        else if(x1 > upper_bracket)
        {   /* constrain using bisection in [x,upper] */
            F64 new_delta_x = (upper_bracket - x) * 0.5; /*+ve*/
            reportf(TEXT("%u: CDF(x=%f;a,b)-p=%+f, CDF'=%f, dx=%+f (bisect [x,u=%f]), new_dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x, upper_bracket, new_delta_x);
            delta_x = new_delta_x;
            x1 = x + delta_x;
        }
        else
        {
            reportf(TEXT("%u: CDF(x=%f;a,b)-p=%+f, CDF'=%f, dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x);
        }

        x = x1;

        if(fabs(delta_x) <= 3.0E-7)
        {
            ev_data_set_real(p_ev_data_out, x);
            return;
        }
    }

    ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
}

PROC_EXEC_PROTO(c_F_inv)
{
    const F64 p = args[0]->arg.fp;
    const F64 d1 = floor(args[1]->arg.fp);
    const F64 d2 = floor(args[2]->arg.fp);

    exec_func_ignore_parms();

    if((p < 0.0) || (p > 1.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if((d1 <= 0.0) || (d2 <= 0.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    F_inv_calc(p_ev_data_res, p, d1, d2); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER f.inv.rt(p:number, d1:number, d2:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_F_inv_rt)
{
    const F64 p = args[0]->arg.fp;
    const F64 d1 = floor(args[1]->arg.fp);
    const F64 d2 = floor(args[2]->arg.fp);

    exec_func_ignore_parms();

    if((p < 0.0) || (p > 1.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if((d1 <= 0.0) || (d2 <= 0.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    F_inv_calc(p_ev_data_res, 1.0 - p, d1, d2); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER f.test(array_1, array_2)
*
* See OpenDocument 1.2 definition of FTEST
*
******************************************************************************/

static void
statistics_array_variance(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InRef_     PC_EV_DATA array_data,
    _OutRef_    P_F64 p_mean,
    _OutRef_    P_S32 p_n)
{
    F64 sample_mean, sample_variance;
    S32 total_size, idx;
    S32 n;

    *p_mean = 0.0;
    *p_n = 0;

    array_range_mono_size(array_data, &total_size);

    { /* calculate the mean of this sample */
    F64 sum = 0.0;

    n = 0;

    for(idx = 0; idx < total_size; ++idx)
    {
        EV_DATA ev_data;

        if(RPN_DAT_REAL != array_range_mono_index(&ev_data, array_data, idx, EM_REA | EM_STR | EM_BLK))
        {   /* ignore non-numeric values */
            ss_data_free_resources(&ev_data);
            continue;
        }

        sum += ev_data.arg.fp;

        ++n;

        ss_data_free_resources(&ev_data);
    }

    if(2 > n)
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ODF_DIV0);
        return;
    }

    sample_mean = sum / n;

    *p_mean = sample_mean;
    } /*block*/

    { /* calculate the variance of this sample */
    F64 sum_delta2 = 0.0;

    n = 0;

    for(idx = 0; idx < total_size; ++idx)
    {
        EV_DATA ev_data;
        F64 delta;

        if(RPN_DAT_REAL != array_range_mono_index(&ev_data, array_data, idx, EM_REA | EM_STR | EM_BLK))
        {   /* ignore non-numeric values */
            ss_data_free_resources(&ev_data);
            continue;
        }

        delta = (ev_data.arg.fp - sample_mean);

        sum_delta2 += mx_fsquare(delta);

        ++n;

        ss_data_free_resources(&ev_data);
    }

    *p_n = n;

    if(2 > n)
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ODF_DIV0);
        return;
    }

    sample_variance = (sum_delta2 / (n - 1.0));

    ev_data_set_real(p_ev_data_out, sample_variance);
    } /*block*/
}

static void
F_test_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InRef_     PC_EV_DATA array_1,
    _InRef_     PC_EV_DATA array_2)
{
    EV_DATA variance_1, variance_2;
    F64 mean_1, mean_2;
    S32 n1, n2;
    S32 d1, d2;
    F64 x;

    statistics_array_variance(&variance_1, array_1, &mean_1, &n1); /* may return fp or error */

    if(ev_data_is_error(&variance_1))
    {
        *p_ev_data_out = variance_1;
        return;
    }

    statistics_array_variance(&variance_2, array_2, &mean_2, &n2); /* may return fp or error */

    if(ev_data_is_error(&variance_2))
    {
        *p_ev_data_out = variance_2;
        return;
    }

    x = variance_1.arg.fp / variance_2.arg.fp;
    d1 = n1 - 1;
    d2 = n2 - 1;

    if(variance_1.arg.fp > variance_2.arg.fp)
    {   /* twice the right-tail */
        F64 F_dist_rt_result = 1.0 - calc_F_cdf(x, d1, d2);
        ev_data_set_real(p_ev_data_out, 2.0 * F_dist_rt_result);
    }
    else
    {   /* twice the left-tail */
        F64 F_dist_result = calc_F_cdf(x, d1, d2);
        ev_data_set_real(p_ev_data_out, 2.0 * F_dist_result);
    }
}

PROC_EXEC_PROTO(c_F_test)
{
    const PC_EV_DATA array_1 = args[0];
    const PC_EV_DATA array_2 = args[1];

    exec_func_ignore_parms();

    F_test_calc(p_ev_data_res, array_1, array_2); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER gamma.dist(x, shape, scale {, cumulative:Boolean=TRUE})
*
* See OpenDocument 1.2 definition of GAMMADIST
*
* A&S 26.1.32
*
******************************************************************************/

/* gamma is a continuous distribution with
 *
 * CDF(x;shape,scale)
 *  = P(shape, x / scale)
 *    where P(s,x) is a regularized gamma function
 */

_Check_return_
static F64
calc_gamma_cdf(
    _InVal_     F64 x,
    _InVal_     F64 shape,
    _InVal_     F64 scale)
{
    const F64 cdf = mx_P_gamma(shape, x / scale);

    return(cdf);
}

/* gamma is a continuous distribution with
 *
 * PDF(x;shape,scale)
 *  = (1 / (G(shape) . (scale ^ shape)) .
 *    x ^ (shape - 1) .
 *    exp(-(x/scale))
 *    where G(x) is the gamma function
 */

_Check_return_
static F64
calc_gamma_pdf(
    _InVal_     F64 x,
    _InVal_     F64 shape,
    _InVal_     F64 scale)
{
    const F64 ln_numerator_power_term = log(x) * (shape - 1.0);
    const F64 ln_numerator_exp_term = (-(x / scale));
    const F64 ln_denominator_gamma_term = lgamma(shape);
    const F64 ln_denominator_power_term = log(scale) * shape;
    const F64 pdf = exp(ln_numerator_power_term + ln_numerator_exp_term - ln_denominator_gamma_term - ln_denominator_power_term);

    return(pdf);
}

PROC_EXEC_PROTO(c_gamma_dist)
{
    const F64 x = args[0]->arg.fp;
    const F64 shape = args[1]->arg.fp; /*XLS:alpha*/
    const F64 scale = args[2]->arg.fp; /*XLS:beta*/
    const BOOL cumulative = (n_args > 3) ? args[3]->arg.boolean : TRUE;
    F64 gamma_dist_result;

    exec_func_ignore_parms();

    if( (x < 0.0)      ||
        (shape <= 0.0) ||
        (scale <= 0.0) )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if(cumulative)
        gamma_dist_result = calc_gamma_cdf(x, shape, scale);
    else
        gamma_dist_result = calc_gamma_pdf(x, shape, scale);

    ev_data_set_real(p_ev_data_res, gamma_dist_result);
}

/******************************************************************************
*
* NUMBER gamma.inv(p:number, shape:number, scale:number)
*
******************************************************************************/

/* a good guess saves a few iterations */

_Check_return_
static F64
gamma_inv_guess_initial_x(
    _InVal_     F64 p,
    _InVal_     F64 shape,
    _InVal_     F64 scale)
{
    F64 initial_x;
    F64 mean = (shape * scale);

    if(shape >= 1.0)
    {   /* See Wikipedia */
        F64 median_est = mean * (shape - 0.8) / (shape + 0.2);
        initial_x = median_est * (((p < 0.5) ? 2.0 : 3.0) / 5.0);
    }
    else
    {
        /* no closed form for the median */
        /* so choose a guess the right side of the mean */
        F64 CDF_mean = calc_gamma_cdf(mean, shape, scale);
        initial_x = mean * (((p < CDF_mean) ? 2.0 : 3.0) / 5.0);
    }

    return(initial_x);
}

/* Newton-Raphson method - solve for F(z) = 0 where F(x;a,b) = CDF(x;a,b) - p */

static void
gamma_inv_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     F64 p,
    _InVal_     F64 shape,
    _InVal_     F64 scale)
{
    F64 x = gamma_inv_guess_initial_x(p, shape, scale);
    F64 lower_bracket = 0.0; /* result is in [0,+inf) */
    F64 upper_bracket = +F64_MAX;
    U32 iteration_count;

    /* there is guaranteed to be a unique solution for p in [0,1] */
    assert((p >= 0.0) && (p <= 1.0));
    assert((shape > 0.0) && (scale > 0.0));

    if(p == 0.0)
    {
        x = lower_bracket;
        ev_data_set_real(p_ev_data_out, x);
        return;
    }

    reportf(TEXT("gamma_inv_calc(p=%f, shape=%f, scale=%f): initial_x=%f"), p, shape, scale, x);
    for(iteration_count = 0; iteration_count < 100; ++iteration_count)
    {
        const F64 CDF_x = calc_gamma_cdf(x, shape, scale);
        const F64 F_x = CDF_x - p;
        const F64 d_F_x = calc_gamma_pdf(x, shape, scale); /* derivative of the CDF is the PDF */
        F64 delta_x, x1;

        if(d_F_x == 0.0)
            break;

        /* we can improve the bracketing constraints on the solution at each step */
        if(F_x < 0.0)
        {
            //reportf("refine [,] from [l=%g to [l=%f", lower_bracket, x);
            lower_bracket = x;
        }
        else
        {
            //reportf("refine [,] from u=%g] to u=%f]", upper_bracket, x);
            upper_bracket = x;
        }

        delta_x = - (F_x / d_F_x);
        x1 = x + delta_x;

        if(x1 < lower_bracket)
        {   /* constrain using bisection in [lower,x] */
            F64 new_delta_x = (lower_bracket - x) * 0.5; /*-ve*/
            reportf(TEXT("%u: CDF(x=%f;a,b)-p=%+f, CDF'=%f, dx=%+f (bisect [l=%f,x]), new_dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x, lower_bracket, new_delta_x);
            delta_x = new_delta_x;
            x1 = x + delta_x;
        }
        else if(x1 > upper_bracket)
        {   /* constrain using bisection in [x,upper] */
            F64 new_delta_x = (upper_bracket - x) * 0.5; /*+ve*/
            reportf(TEXT("%u: CDF(x=%f;a,b)-p=%+f, CDF'=%f, dx=%+f (bisect [x,u=%f]), new_dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x, upper_bracket, new_delta_x);
            delta_x = new_delta_x;
            x1 = x + delta_x;
        }
        else
        {
            reportf(TEXT("%u: CDF(x=%f;a,b)-p=%+f, CDF'=%f, dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x);
        }

        x = x1;

        if(fabs(delta_x) <= 3.0E-7)
        {
            ev_data_set_real(p_ev_data_out, x);
            return;
        }
    }

    ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
}

PROC_EXEC_PROTO(c_gamma_inv)
{
    const F64 p = args[0]->arg.fp;
    const F64 shape = args[1]->arg.fp; /*XLS:alpha*/
    const F64 scale = args[2]->arg.fp; /*XLS:beta*/

    exec_func_ignore_parms();

    if((p < 0.0) || (p > 1.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if( (shape <= 0.0) ||
        (scale <= 0.0) )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    gamma_inv_calc(p_ev_data_res, p, shape, scale); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER lognorm.dist(x:number, mean:number, sigma:number {, cumulative:Boolean})
*
******************************************************************************/

PROC_EXEC_PROTO(c_lognorm_dist)
{
    const F64 x = args[0]->arg.fp;
    const F64 mean = args[1]->arg.fp;
    const F64 sigma = args[2]->arg.fp;
    const BOOL cumulative = (n_args > 3) ? args[3]->arg.boolean : TRUE;
    F64 ln_x;
    F64 z;
    F64 lognorm_dist_result;

    exec_func_ignore_parms();

    if(sigma == 0.0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
        return;
    }

    if(x <= 0.0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    errno = 0;

    ln_x = log(x);

    if(errno /* == EDOM, ERANGE */)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_LOG);
        return;
    }

    z = (ln_x - mean) / sigma; /* standardize */

    if(cumulative)
        lognorm_dist_result = calc_norm_std_cdf(z);
    else
        lognorm_dist_result = calc_norm_std_pdf(z) / x;

    ev_data_set_real(p_ev_data_res, lognorm_dist_result);
}

/******************************************************************************
*
* NUMBER lognorm.inv(p:number, mean:number, sigma:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_lognorm_inv)
{
    const F64 p = args[0]->arg.fp;
  /*const F64 mean = args[1]->arg.fp;*/
  /*const F64 sigma = args[2]->arg.fp;*/
    F64 x;

    exec_func_ignore_parms();

    if((p < 0.0) || (p > 1.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    norm_std_inv_calc(p_ev_data_res, p); /* may return fp or error */

    /* scale and shift standardized result */
    if( !two_nums_multiply_try(p_ev_data_res, p_ev_data_res, args[2] /*sigma*/, TRUE /*propogate_errors*/) ||
        !two_nums_add_try(     p_ev_data_res, p_ev_data_res, args[1] /*mean*/,  TRUE /*propogate_errors*/) )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_CALC_FAILURE);
    }

    if(ev_data_is_error(p_ev_data_res))
        return;

    x = exp(p_ev_data_res->arg.fp);

    /* exp() overflowed? - don't test for underflow case */
    if(F64_HUGE_VAL == x)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    ev_data_set_real(p_ev_data_res, x);
}

/******************************************************************************
*
* NUMBER norm.s.dist(z:number, {, cumulative:Boolean})
*
* NUMBER phi(z:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_norm_s_dist)
{
    const F64 z = args[0]->arg.fp;
    const BOOL cumulative = (n_args > 1) ? args[1]->arg.boolean : TRUE;
    F64 norm_s_dist_result;

    exec_func_ignore_parms();

    if(cumulative)
        norm_s_dist_result = calc_norm_std_cdf(z);
    else
        norm_s_dist_result = calc_norm_std_pdf(z);

    ev_data_set_real(p_ev_data_res, norm_s_dist_result);
}

PROC_EXEC_PROTO(c_phi)
{
    const F64 z = args[0]->arg.fp;
    F64 phi_result;

    exec_func_ignore_parms();

    phi_result = calc_norm_std_pdf(z);

    ev_data_set_real(p_ev_data_res, phi_result);
}

/******************************************************************************
*
* NUMBER norm.s.inv(p:number)
*
******************************************************************************/

/* See "Approximations to Inverse Error Functions", Dyer (2008) */

/* Seems good to about 1% in (0.01..0.45) */

_Check_return_
static inline F64
approximate_norm_std_inv(
    _InVal_     F64 p)
{
    const F64 t = sqrt(-log(p));
    const F64 x = (1.758 - (2.257 * t) + (0.1661 * t * t));

    return(x);
}

/* a good guess saves a few iterations */

_Check_return_
static F64
norm_std_inv_guess_initial_x(
    _InVal_     F64 p)
{
    if(p < 0.47)
    {
        return(approximate_norm_std_inv(p));
    }
    else if(p > (1.0 - 0.47))
    {
        return(-approximate_norm_std_inv(1.0 - p));
    }
    else
    {
        F64 p_minus_half = (p - 0.5);
        F64 abs_pmh = fabs(p_minus_half); /* symmetry of PHI(z) */
        F64 initial_x =
            (abs_pmh >= 0.4938) ? 2.5 : /* segment around 2.5s */
            (abs_pmh >= 0.4332) ? 1.5 : /* segment around 1.5s */
            (abs_pmh >= 0.1915) ? 1.0 : /* segment around 0.5s */
            0.0;
        return((p_minus_half < 0.0) ? -initial_x : initial_x);
    }
}

/* Newton-Raphson method - solve for F(z) = 0 where F(z) = PHI(z) - p */

static void
norm_std_inv_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     F64 p)
{
    F64 x = norm_std_inv_guess_initial_x(p);
    U32 iteration_count;

    /* there is guaranteed to be a unique solution for p in [0,1] */

    reportf(TEXT("norm_std_inv_calc(p=%f): initial_x=%+f"), p, x);
    for(iteration_count = 0; iteration_count < 100; ++iteration_count)
    {
        const F64 PHI_x = calc_norm_std_cdf(x);
        const F64 F_x = PHI_x - p;
        const F64 d_F_x = calc_norm_std_pdf(x); /* derivative of the CDF is the PDF */
        F64 delta_x;

        if(d_F_x == 0.0)
            break;

        delta_x = - (F_x / d_F_x);
        reportf(TEXT("%u: Fz=CDF(x=%+f)-p=%+f, CDF'=%f, dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x);

        x += delta_x;

        if(fabs(delta_x) <= 3.0E-7)
        {
            ev_data_set_real(p_ev_data_out, x);
            return;
        }
    }

    ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
}

PROC_EXEC_PROTO(c_norm_s_inv)
{
    const F64 p = args[0]->arg.fp;

    exec_func_ignore_parms();

    if((p < 0.0) || (p > 1.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    norm_std_inv_calc(p_ev_data_res, p); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER norm.dist(x:number, mean:number, sigma:number {, cumulative:Boolean})
*
******************************************************************************/

_Check_return_
static inline F64
calc_norm_cdf(
    _InVal_     F64 x,
    _InVal_     F64 mean,
    _InVal_     F64 sigma)
{
    const F64 z = ((x - mean) / sigma);
    const F64 cdf = calc_norm_std_cdf(z);

    return(cdf);
}

_Check_return_
static inline F64
calc_norm_pdf(
    _InVal_     F64 x,
    _InVal_     F64 mean,
    _InVal_     F64 sigma)
{
    const F64 z = ((x - mean) / sigma);
    const F64 std_pdf = calc_norm_std_pdf(z);
    const F64 pdf = std_pdf / sigma;

    return(pdf);
}

PROC_EXEC_PROTO(c_norm_dist)
{
    const F64 x = args[0]->arg.fp;
    const F64 mean = args[1]->arg.fp;
    const F64 sigma = args[2]->arg.fp;
    const BOOL cumulative = (n_args > 3) ? args[3]->arg.boolean : TRUE;
    F64 norm_dist_result;

    exec_func_ignore_parms();

    if(sigma == 0.0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
        return;
    }

    if(cumulative)
        norm_dist_result = calc_norm_cdf(x, mean, sigma);
    else
        norm_dist_result = calc_norm_pdf(x, mean, sigma);

    ev_data_set_real(p_ev_data_res, norm_dist_result);
}

/******************************************************************************
*
* NUMBER norm.inv(p:number, mean:number, sigma:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_norm_inv)
{
    const F64 p = args[0]->arg.fp;
  /*const F64 mean = args[1]->arg.fp;*/
    const F64 sigma = args[2]->arg.fp;

    exec_func_ignore_parms();

    if((p < 0.0) || (p > 1.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if(sigma == 0.0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
        return;
    }

    norm_std_inv_calc(p_ev_data_res, p); /* may return fp or error */

    /* scale and shift standardized result */
    if( !two_nums_multiply_try(p_ev_data_res, p_ev_data_res, args[2] /*sigma*/, TRUE /*propogate_errors*/) ||
        !two_nums_add_try(     p_ev_data_res, p_ev_data_res, args[1] /*mean*/,  TRUE /*propogate_errors*/) )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_CALC_FAILURE);
    }
}

/******************************************************************************
*
* NUMBER t.dist(x, k:number {, cumulative})
*
******************************************************************************/

/* Student's t is a continuous distribution with
 *
 * CDF(t,nu)
 *  = 1 - (1/2) x Ix(h1, h2) (for t > 0, else use symmetry)
 *    where Iz(a,b) is a regularized incomplete beta function and
 *    x = nu / (t^2 + nu)
 *    h1 = nu / 2
 *    h2 =  1 / 2
 */

_Check_return_
static F64
calc_t_cdf(
    _InVal_     F64 t,
    _InVal_     F64 nu)
{
    const F64 t2 = t * t;
    const F64 x = nu / (t2 + nu);
    const F64 h1 = nu * 0.5;
    const F64 h2 =      0.5;
    const F64 half_Ix = mx_Ix_beta(x, h1, h2) * 0.5;
    const F64 cdf = (t > 0.0) ? (1.0 - half_Ix) : half_Ix;

    return(cdf);
}

/* Student's t is a continuous distribution with
 *
 * PDF(t;nu)
 *  = (1 / (B(h1, h2)) .
 *    (1 / sqrt(nu)) .
 *    (x ^ (h1 + h2))
 *    where B(a,b) is the beta function and
 *    x = t^2 + nu
 *    h1 = nu / 2
 *    h2 =  1 / 2
 */

_Check_return_
static F64
calc_t_pdf(
    _InVal_     F64 t,
    _InVal_     F64 nu)
{
    const F64 t2 = t * t;
    const F64 x = nu / (t2 + nu);
    const F64 h1 = nu * 0.5;
    const F64 h2 =      0.5;
    const F64 ln_reciprocal_beta_term = mx_ln_reciprocal_beta(h1, h2);
    const F64 denominator_sqrt_term = sqrt(nu);
    const F64 ln_numerator_power_term = log(x) * (h1 + h2);
    const F64 pdf = exp(ln_reciprocal_beta_term + ln_numerator_power_term) / denominator_sqrt_term;

    return(pdf);
}

PROC_EXEC_PROTO(c_t_dist)
{
    const F64 t = args[0]->arg.fp;
    const F64 nu = floor(args[1]->arg.fp);
    const BOOL cumulative = (n_args > 2) ? args[2]->arg.boolean : TRUE;
    F64 t_dist_result;

    exec_func_ignore_parms();

    if(nu <= 0.0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if(cumulative)
        t_dist_result = calc_t_cdf(t, nu);
    else
        t_dist_result = calc_t_pdf(t, nu);

    ev_data_set_real(p_ev_data_res, t_dist_result);
}

/******************************************************************************
*
* NUMBER t.dist.2t(x, k:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_t_dist_2t)
{
    const F64 t = args[0]->arg.fp;
    const F64 nu = floor(args[1]->arg.fp);
    F64 t_dist_2t_result;

    exec_func_ignore_parms();

    if(nu <= 0.0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    t_dist_2t_result = 2.0 * (1.0 - calc_t_cdf(fabs(t), nu));

    ev_data_set_real(p_ev_data_res, t_dist_2t_result);
}

/******************************************************************************
*
* NUMBER t.dist.rt(x, k:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_t_dist_rt)
{
    const F64 t = args[0]->arg.fp;
    const F64 nu = floor(args[1]->arg.fp);
    F64 t_dist_rt_result;

    exec_func_ignore_parms();

    if(nu <= 0.0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    t_dist_rt_result = 1.0 - calc_t_cdf(t, nu);

    ev_data_set_real(p_ev_data_res, t_dist_rt_result);
}

/******************************************************************************
*
* NUMBER odf.tdist(x, k:number, n:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_odf_tdist)
{
    const F64 t = args[0]->arg.fp;
    const F64 nu = floor(args[1]->arg.fp);
    const F64 n = floor(args[2]->arg.fp);
    F64 odf_tdist_result;

    exec_func_ignore_parms();

    if(nu <= 0.0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if((n != 1.0) && (n != 2.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    odf_tdist_result = n * (1.0 - calc_t_cdf(fabs(t), nu));

    ev_data_set_real(p_ev_data_res, odf_tdist_result);
}

/******************************************************************************
*
* NUMBER t.inv(p:number, k:number)
*
******************************************************************************/

static void
t_inv_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     F64 p,
    _InVal_     F64 k)
{
    F64 x;
    F64 lower_bracket = -F64_MAX; /* result is in (-inf,+inf) */
    F64 upper_bracket = +F64_MAX;
    U32 iteration_count;

    /* there is guaranteed to be a unique solution for p in [0,1] */
    /* or in (0,1] when d1 == 0 */
    assert((p >= 0.0) && (p <= 1.0));
    assert(k > 0.0);

    /* Student's t approaches normal as d.f. -> +inf so that guess probably isn't far off */
    x = norm_std_inv_guess_initial_x(p);

    reportf(TEXT("t_inv_calc(p=%f, d.f.=%f): initial_x=%f"), p, k, x);
    for(iteration_count = 0; iteration_count < 100; ++iteration_count)
    {
        const F64 CDF_x = calc_t_cdf(x, k);
        const F64 F_x = CDF_x - p;
        const F64 d_F_x = calc_t_pdf(x, k); /* derivative of the CDF is the PDF */
        F64 delta_x, x1;

        if(d_F_x == 0.0)
            break;

        /* we can improve the bracketing constraints on the solution at each step */
        if(F_x < 0.0)
        {
            //reportf("refine [,] from [l=%g to [l=%f", lower_bracket, x);
            lower_bracket = x;
        }
        else
        {
            //reportf("refine [,] from u=%g] to u=%f]", upper_bracket, x);
            upper_bracket = x;
        }

        delta_x = - (F_x / d_F_x);
        x1 = x + delta_x;

        if(x1 < lower_bracket)
        {   /* constrain using bisection in [lower,x] */
            F64 new_delta_x = (lower_bracket - x) * 0.5; /*-ve*/
            reportf(TEXT("%u: CDF(x=%f;a,b)-p=%+f, CDF'=%f, dx=%+f (bisect [l=%f,x]), new_dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x, lower_bracket, new_delta_x);
            delta_x = new_delta_x;
            x1 = x + delta_x;
        }
        else if(x1 > upper_bracket)
        {   /* constrain using bisection in [x,upper] */
            F64 new_delta_x = (upper_bracket - x) * 0.5; /*+ve*/
            reportf(TEXT("%u: CDF(x=%f;a,b)-p=%+f, CDF'=%f, dx=%+f (bisect [x,u=%f]), new_dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x, upper_bracket, new_delta_x);
            delta_x = new_delta_x;
            x1 = x + delta_x;
        }
        else
        {
            reportf(TEXT("%u: CDF(x=%f;a,b)-p=%+f, CDF'=%f, dx=%+f"), iteration_count, x, F_x, d_F_x, delta_x);
        }

        x = x1;

        if(fabs(delta_x) <= 3.0E-7)
        {
            ev_data_set_real(p_ev_data_out, x);
            return;
        }
    }

    ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
}

PROC_EXEC_PROTO(c_t_inv)
{
    const F64 p = args[0]->arg.fp;
    const F64 k = floor(args[1]->arg.fp);

    exec_func_ignore_parms();

    if((p < 0.0) || (p > 1.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if(k <= 0.0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    t_inv_calc(p_ev_data_res, p, k); /* may return fp or error */
}

/******************************************************************************
*
* NUMBER t.inv.2t(p:number, k:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_t_inv_2t)
{
    const F64 p = args[0]->arg.fp;
    const F64 k = floor(args[1]->arg.fp);

    exec_func_ignore_parms();

    if((p < 0.0) || (p > 1.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if(k <= 0.0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    { /* NB t_dist_2t_result = 2.0 * (1.0 - calc_t_cdf(fabs(t), nu)); */
    const F64 lookup_p = 1.0 - (0.5 * p);
    t_inv_calc(p_ev_data_res, lookup_p, k); /* may return fp or error */
    } /*block*/
}

/******************************************************************************
*
* NUMBER t.test(array_1, array_2, tails:number, type:number)
*
* See OpenDocument 1.2 definition of TTEST
*
******************************************************************************/

static void
t_test_paired_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InRef_     PC_EV_DATA array_1,
    _InRef_     PC_EV_DATA array_2)
{
    EV_DATA variance_1, variance_2;
    F64 mean_1, mean_2;
    S32 n1, n2;

    statistics_array_variance(&variance_1, array_1, &mean_1, &n1); /* may return fp or error */

    if(ev_data_is_error(&variance_1))
    {
        *p_ev_data_out = variance_1;
        return;
    }

    statistics_array_variance(&variance_2, array_2, &mean_2, &n2); /* may return fp or error */

    if(ev_data_is_error(&variance_2))
    {
        *p_ev_data_out = variance_2;
        return;
    }

    if(n1 != n2)
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ODF_NA);
        return;
    }

    { /* calculate the paired variance of this pair of samples */
    S32 total_size, idx;
    S32 n;
    F64 sum = 0.0;

    array_range_mono_size(array_1, &total_size);

    n = 0;

    for(idx = 0; idx < total_size; ++idx)
    {
        EV_DATA ev_data;
        F64 delta_1, delta_2, paired_delta;

        if(RPN_DAT_REAL != array_range_mono_index(&ev_data, array_1, idx, EM_REA | EM_STR | EM_BLK))
        {   /* ignore non-numeric values */
            ss_data_free_resources(&ev_data);
            continue;
        }

        delta_1 = (ev_data.arg.fp - mean_1);

        ss_data_free_resources(&ev_data);

        if(RPN_DAT_REAL != array_range_mono_index(&ev_data, array_2, idx, EM_REA | EM_STR | EM_BLK))
        {   /* ignore non-numeric values */
            ss_data_free_resources(&ev_data);
            continue;
        }

        ss_data_free_resources(&ev_data);

        delta_2 = (ev_data.arg.fp - mean_2);

        paired_delta = delta_1 - delta_2;

        sum += mx_fsquare(paired_delta);

        ++n;
    }

    if((2 > n) || (n1 != n))
    {
        ev_data_set_error(p_ev_data_out, EVAL_ERR_ODF_DIV0);
        return;
    }

    {
    const S32 n_minus_1 = n - 1;
    const F64 paired_variance = sum / n_minus_1;
    const F64 t = fabs(mean_1 - mean_2) * sqrt(n) / sqrt(paired_variance);
    const F64 t_dist_rt_result = 1.0 - calc_t_cdf(t, n_minus_1);

    ev_data_set_real(p_ev_data_out, t_dist_rt_result);
    } /*block*/
    } /*block*/
}

/* two sample equal variance */

static void
t_test_homoscedastic_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InRef_     PC_EV_DATA array_1,
    _InRef_     PC_EV_DATA array_2)
{
    EV_DATA variance_1, variance_2;
    F64 mean_1, mean_2;
    S32 n1, n2;

    statistics_array_variance(&variance_1, array_1, &mean_1, &n1); /* may return fp or error */

    if(ev_data_is_error(&variance_1))
    {
        *p_ev_data_out = variance_1;
        return;
    }

    statistics_array_variance(&variance_2, array_2, &mean_2, &n2); /* may return fp or error */

    if(ev_data_is_error(&variance_2))
    {
        *p_ev_data_out = variance_2;
        return;
    }

    {
    const S32 n1_minus_1 = n1 - 1;
    const S32 n2_minus_1 = n2 - 1;
    const S32 equal_sample_n = (n1_minus_1 + n2_minus_1);
    const F64 v1 = variance_1.arg.fp;
    const F64 v2 = variance_2.arg.fp;
    const F64 equal_variance = ((n1_minus_1 * v1) + (n2_minus_1 * v2)) / equal_sample_n;
    const F64 t = fabs(mean_1 - mean_2) / sqrt(equal_variance * ((1.0 / n1) + (1.0 / n2)));
    const F64 t_dist_rt_result = 1.0 - calc_t_cdf(t, equal_sample_n);

    ev_data_set_real(p_ev_data_out, t_dist_rt_result);
    } /*block*/
}

/* two sample unequal variance */

static void
t_test_heteroscedastic_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InRef_     PC_EV_DATA array_1,
    _InRef_     PC_EV_DATA array_2)
{
    EV_DATA variance_1, variance_2;
    F64 mean_1, mean_2;
    S32 n1, n2;

    statistics_array_variance(&variance_1, array_1, &mean_1, &n1); /* may return fp or error */

    if(ev_data_is_error(&variance_1))
    {
        *p_ev_data_out = variance_1;
        return;
    }

    statistics_array_variance(&variance_2, array_2, &mean_2, &n2); /* may return fp or error */

    if(ev_data_is_error(&variance_2))
    {
        *p_ev_data_out = variance_2;
        return;
    }

    {
    const S32 n1_minus_1 = n1 - 1;
    const S32 n2_minus_1 = n2 - 1;
    const F64 v1 = variance_1.arg.fp;
    const F64 v2 = variance_2.arg.fp;
    const F64 v1_div_n1 = v1 / n1;
    const F64 v2_div_n2 = v2 / n2;
    const F64 t = fabs(mean_1 - mean_2) / sqrt(v1_div_n1 + v2_div_n2);
    const F64 nu_numerator = mx_fsquare(v1_div_n1 + v2_div_n2);
    const F64 nu_denominator = (mx_fsquare(v1_div_n1) / n1_minus_1) + (mx_fsquare(v2_div_n2) / n2_minus_1);
    const F64 nu = nu_numerator / nu_denominator;
    const F64 t_dist_rt_result = 1.0 - calc_t_cdf(t, nu);

    ev_data_set_real(p_ev_data_out, t_dist_rt_result);
    } /*block*/
}

PROC_EXEC_PROTO(c_t_test)
{
    const PC_EV_DATA array_1 = args[0];
    const PC_EV_DATA array_2 = args[1];
    const F64 tails = args[2]->arg.fp;
    const S32 type = (S32) floor(args[3]->arg.fp);

    exec_func_ignore_parms();

    if((tails != 1.0) && (tails != 2.0))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NUM);
        return;
    }

    switch(type)
    {
    default:
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NUM);
        return;

    case 1:
        t_test_paired_calc(p_ev_data_res, array_1, array_2); /* may return fp or error */
        break;

    case 2:
        t_test_homoscedastic_calc(p_ev_data_res, array_1, array_2); /* may return fp or error */
        break;

    case 3:
        t_test_heteroscedastic_calc(p_ev_data_res, array_1, array_2); /* may return fp or error */
        break;
    }

    if(!ev_data_is_error(p_ev_data_res) && (2.0 == tails))
        ev_data_set_real(p_ev_data_res, p_ev_data_res->arg.fp * 2.0);
}

/******************************************************************************
*
* NUMBER weibull.dist(x:number, shape:number, scale:number, cumulative:Boolean)
*
* See OpenDocument 1.2 definition of WEIBULL
*
******************************************************************************/

PROC_EXEC_PROTO(c_weibull_dist)
{
    const F64 x = args[0]->arg.fp;
    const F64 shape = args[1]->arg.fp;
    const F64 scale = args[2]->arg.fp;
    const BOOL cumulative = args[3]->arg.boolean;
    F64 weibull_dist_result;

    exec_func_ignore_parms();

    if( (x < 0) ||
        (shape <= 0) ||
        (scale <= 0) )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    {
    const F64 exp_term = exp(-pow((x / scale), shape));

    if(cumulative)
        weibull_dist_result = 1.0 - exp_term;
    else
        weibull_dist_result = (shape / scale) * pow((x / scale), (shape - 1)) * exp_term;
    } /*block*/

    ev_data_set_real(p_ev_data_res, weibull_dist_result);
}

/******************************************************************************
*
* NUMBER z.test(array, mean:number {, sigma:number})
*
******************************************************************************/

PROC_EXEC_PROTO(c_z_test)
{
    const PC_EV_DATA array_data = args[0];
    const F64 mean = args[1]->arg.fp;
    F64 sample_mean, sample_variance;
    F64 sigma;
    F64 z;
    S32 total_size, idx;
    S32 n;
    F64 z_test_result = 0.0;

    exec_func_ignore_parms();

    array_range_mono_size(array_data, &total_size);

    { /* calculate the mean of this sample */
    F64 sum = 0.0;

    n = 0;

    for(idx = 0; idx < total_size; ++idx)
    {
        EV_DATA ev_data;

        if(RPN_DAT_REAL != array_range_mono_index(&ev_data, array_data, idx, EM_REA | EM_STR | EM_BLK))
        {   /* ignore non-numeric values */
            ss_data_free_resources(&ev_data);
            continue;
        }

        sum += ev_data.arg.fp;

        ++n;

        ss_data_free_resources(&ev_data);
    }

    if(2 > n)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NA);
        return;
    }

    sample_mean = sum / n;
    } /*block*/

    if(n_args > 2)
    {
        sigma = args[2]->arg.fp;
    }
    else
    {   /* calculate the variance of this sample */
        F64 sum_delta2 = 0.0;

        n = 0;

        for(idx = 0; idx < total_size; ++idx)
        {
            EV_DATA ev_data;
            F64 delta;

            if(RPN_DAT_REAL != array_range_mono_index(&ev_data, array_data, idx, EM_REA | EM_STR | EM_BLK))
            {   /* ignore non-numeric values */
                ss_data_free_resources(&ev_data);
                continue;
            }

            delta = (ev_data.arg.fp - sample_mean);

            sum_delta2 += mx_fsquare(delta);

            ++n;

            ss_data_free_resources(&ev_data);
        }

        if(2 > n)
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NA);
            return;
        }

        sample_variance = (sum_delta2 / (n - 1.0));

        sigma = sqrt(sample_variance);
    }

    z = ((sample_mean - mean) * sqrt(n)) / sigma;

    z_test_result = 1.0 - calc_norm_std_cdf(z);

    ev_data_set_real(p_ev_data_res, z_test_result);
}

/* end of ev_fnstc.c */
