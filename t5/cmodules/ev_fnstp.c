/* ev_fnstp.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2019 Stuart Swales */

/* Statistical function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "cmodules/mathxtra.h" /* for mx_fsquare() */

/******************************************************************************
*
* Statistical functions (newer paired statistics)
*
******************************************************************************/

/* don't force data to numeric values for many statistics functions - caller tests for non-numeric */

#define statistics_array_range_index(p_ev_data_out, p_ev_data_in, ix, iy) \
    array_range_index(p_ev_data_out, p_ev_data_in, ix, iy, EM_REA | EM_STR | EM_BLK)

/* only returns a pair of numeric values that the caller can use */

_Check_return_
extern BOOL
statistics_paired_values_next(
    _OutRef_    P_EV_DATA p_ev_data_out_a,
    _OutRef_    P_EV_DATA p_ev_data_out_b,
    _InRef_     PC_EV_DATA p_ev_data_in_a,
    _InRef_     PC_EV_DATA p_ev_data_in_b,
    _InoutRef_  P_S32 p_ix,
    _InoutRef_  P_S32 p_iy,
    _InRef_     S32 x_size,
    _InRef_     S32 y_size)
{
    EV_DATA ev_data;
    S32 ix = *p_ix;
    S32 iy = *p_iy;

    if((ix < 0) || (ix >= x_size))
    {
        ix = -1; /* prepare for increment to 0,0 */
        iy = 0;
    }

    for(;;)
    {
        ++ix; /* across columns first */
        if(ix >= x_size)
        {
            ix = 0;
            ++iy; /* then down rows */
            if(iy >= y_size)
            {
                ev_data_set_blank(p_ev_data_out_a);
                ev_data_set_blank(p_ev_data_out_b);
                *p_ix = x_size;
                *p_iy = y_size;
                return(FALSE);
            }
        }

        if(RPN_DAT_REAL != statistics_array_range_index(&ev_data, p_ev_data_in_a, ix, iy))
        {   /* ignore non-numeric values */
            ss_data_free_resources(&ev_data);
            continue;
        }

        *p_ev_data_out_a = ev_data;

        if(RPN_DAT_REAL != statistics_array_range_index(&ev_data, p_ev_data_in_b, ix, iy))
        {   /* ignore non-numeric values */
            ss_data_free_resources(&ev_data);
            continue;
        }

        *p_ev_data_out_b = ev_data;

        /* only TRUE if both arrays have usable values at (ix,iy) */
        *p_ix = ix;
        *p_iy = iy;
        return(TRUE);
    }
}

typedef struct STATISTICS_PAIRED
{
    struct STATISTICS_PAIRED_SINGLE
    {
        S32 n;
        F64 sum;
        F64 mean;
        F64 sum_of_delta;
        F64 sum_of_delta2;
    } x, y;

    F64 sum_of_product_of_deltas;
}
STATISTICS_PAIRED, * P_STATISTICS_PAIRED;

_Check_return_
static STATUS
statistics_paired_calc(
    _InoutRef_  P_STATISTICS_PAIRED p_statistics_paired,
    _InRef_     PC_EV_DATA array_x,
    _InRef_     PC_EV_DATA array_y)
{
    S32 x_size[2]; /* [0]:array_x, [1]:array_y */
    S32 y_size[2];
    S32 ix, iy;
    EV_DATA ev_data_x, ev_data_y;

    p_statistics_paired->x.n = 0;
    p_statistics_paired->y.n = 0;

    p_statistics_paired->x.sum = 0.0;
    p_statistics_paired->y.sum = 0.0;

    p_statistics_paired->x.mean = 0.0;
    p_statistics_paired->y.mean = 0.0;

    p_statistics_paired->x.sum_of_delta = 0.0;
    p_statistics_paired->y.sum_of_delta = 0.0;

    p_statistics_paired->sum_of_product_of_deltas = 0.0;
 
    array_range_sizes(array_x, &x_size[0], &y_size[0]);
    array_range_sizes(array_y, &x_size[1], &y_size[1]);

    if((x_size[0] != x_size[1]) || (y_size[0] != y_size[1]))
        return(EVAL_ERR_ODF_NA);

    /* first phase calculates the number of usable elements and the means of each array of data */
    ix = -1; iy = 0;

    while(statistics_paired_values_next(&ev_data_x, &ev_data_y, array_x, array_y, &ix, &iy, x_size[0], y_size[0]))
    {
        p_statistics_paired->x.sum += ev_data_x.arg.fp;
        p_statistics_paired->x.n++;

        p_statistics_paired->y.sum += ev_data_y.arg.fp;
        p_statistics_paired->y.n++;
    }

    if(0 == p_statistics_paired->x.n)
        return(EVAL_ERR_NO_VALID_DATA);

    p_statistics_paired->x.mean = p_statistics_paired->x.sum / p_statistics_paired->x.n;
    p_statistics_paired->y.mean = p_statistics_paired->y.sum / p_statistics_paired->y.n;

    /* second phase calculates the independent, and paired, statistics based on the difference of each value from its mean */
    ix = -1; iy = 0;

    for(;;)
    {
        F64 delta_x, delta_y;

        if(!statistics_paired_values_next(&ev_data_x, &ev_data_y, array_x, array_y, &ix, &iy, x_size[0], y_size[0]))
        {
            break;
        }

        delta_x = ev_data_x.arg.fp - p_statistics_paired->x.mean; /* difference of value from array_x from its mean */
        p_statistics_paired->x.sum_of_delta += delta_x;
        p_statistics_paired->x.sum_of_delta2 += mx_fsquare(delta_x);

        delta_y = ev_data_y.arg.fp - p_statistics_paired->y.mean; /* difference of value from array_y from its mean */
        p_statistics_paired->y.sum_of_delta += delta_y;
        p_statistics_paired->y.sum_of_delta2 += mx_fsquare(delta_y);

        p_statistics_paired->sum_of_product_of_deltas += (delta_x * delta_y);
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* NUMBER covariance.p(array_x, array_y)
*
* NUMBER covariance.s(array_x, array_y)
*
******************************************************************************/

_Check_return_
static STATUS
covariance_calc(
    _InoutRef_  P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA array_x,
    _InRef_     PC_EV_DATA array_y,
    _InVal_     BOOL population_statistics)
{
    STATUS status = STATUS_OK;
    STATISTICS_PAIRED statistics_paired;
    zero_struct(statistics_paired);

    if(status_ok(status = statistics_paired_calc(&statistics_paired, array_x, array_y)))
    {
        /* the covariance is the average of the products of the deviations */
        F64 n = statistics_paired.x.n;
        F64 covariance = statistics_paired.sum_of_product_of_deltas / (population_statistics ? n : (n - 1.0));

        ev_data_set_real(p_ev_data_out, covariance);
    }

    if(status_fail(status))
        ev_data_set_error(p_ev_data_out, status);

    return(status);
}

PROC_EXEC_PROTO(c_covariance_p)
{
    const PC_EV_DATA array_x = args[0];
    const PC_EV_DATA array_y = args[1];
    STATUS status;

    exec_func_ignore_parms();

    status = covariance_calc(p_ev_data_res, array_x, array_y, TRUE);

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
}

PROC_EXEC_PROTO(c_covariance_s)
{
    const PC_EV_DATA array_x = args[0];
    const PC_EV_DATA array_y = args[1];
    STATUS status;

    exec_func_ignore_parms();

    status = covariance_calc(p_ev_data_res, array_x, array_y, FALSE);

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
}

/******************************************************************************
*
* NUMBER forecast(x:number, known_y:array,known_x:array)
*
* NUMBER intercept(known_y:array,known_x:array)
*
* NUMBER slope(known_y:array,known_x:array)
*
******************************************************************************/

static void
slope_or_forecast_calc(
    _InoutRef_  P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA known_y,
    _InRef_     PC_EV_DATA known_x,
    _InRef_opt_ PC_F64 p_x)
{
    STATUS status = STATUS_OK;
    STATISTICS_PAIRED statistics_paired;
    zero_struct(statistics_paired);

    if(status_ok(status = statistics_paired_calc(&statistics_paired, known_x, known_y)))
    {
        F64 slope_numerator =
            statistics_paired.sum_of_product_of_deltas;

        F64 slope_denominator =
            statistics_paired.x.sum_of_delta2;

        F64 slope =
            slope_numerator
            /
            slope_denominator;

        if(p_x)
        {
            F64 forecast = statistics_paired.y.mean + slope * (*p_x - statistics_paired.x.mean);

            ev_data_set_real(p_ev_data_out, forecast);
        }
        else
        {
            ev_data_set_real(p_ev_data_out, slope);
        }
    }

    if(status_fail(status))
        ev_data_set_error(p_ev_data_out, status);
}

PROC_EXEC_PROTO(c_forecast)
{
    const F64 x = args[0]->arg.fp;
    const PC_EV_DATA known_y = args[1];
    const PC_EV_DATA known_x = args[2];

    exec_func_ignore_parms();

    slope_or_forecast_calc(p_ev_data_res, known_y, known_x, &x);
}

PROC_EXEC_PROTO(c_intercept)
{
    const PC_EV_DATA known_y = args[0];
    const PC_EV_DATA known_x = args[1];
    const F64 x = 0.0;

    exec_func_ignore_parms();

    slope_or_forecast_calc(p_ev_data_res, known_y, known_x, &x);
}

PROC_EXEC_PROTO(c_slope)
{
    const PC_EV_DATA known_y = args[0];
    const PC_EV_DATA known_x = args[1];

    exec_func_ignore_parms();

    slope_or_forecast_calc(p_ev_data_res, known_y, known_x, FALSE);
}

/******************************************************************************
*
* NUMBER correl(array_x, array_y)
*
* NUMBER pearson(array_x, array_y)
*
* Same as Excel CORREL so better than early Excel PEARSON
*
******************************************************************************/

PROC_EXEC_PROTO(c_correl)
{
    /* call my friend to do the hard work */
    c_pearson(args, n_args, p_ev_data_res, p_cur_slr);
}

PROC_EXEC_PROTO(c_pearson)
{
    const PC_EV_DATA array_x = args[0];
    const PC_EV_DATA array_y = args[1];
    STATUS status = STATUS_OK;
    STATISTICS_PAIRED statistics_paired;
    zero_struct(statistics_paired);

    exec_func_ignore_parms();

    if(status_ok(status = statistics_paired_calc(&statistics_paired, array_x, array_y)))
    {
        /* the Pearson r correlation coefficient is the average of the products of the deviations divided by the product of the standard deviations */
        F64 pearson_r =
            statistics_paired.sum_of_product_of_deltas
            /
            sqrt(statistics_paired.x.sum_of_delta2 * statistics_paired.y.sum_of_delta2);

        ev_data_set_real(p_ev_data_res, pearson_r);
    }

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
}

/******************************************************************************
*
* NUMBER prob(x_data:array, prob_data:array, lower_limit:number {, upper_limit:number})
*
******************************************************************************/

PROC_EXEC_PROTO(c_prob)
{
    const PC_EV_DATA array_x = args[0];
    const PC_EV_DATA array_prob = args[1];
    const F64 lower_limit = args[2]->arg.fp;
    const F64 upper_limit = (n_args > 3) ? args[3]->arg.fp : lower_limit;
    STATUS status = STATUS_OK;
    S32 total_size, idx;
    S32 total_size_prob;
    F64 prob_result = 0.0;

    exec_func_ignore_parms();

    array_range_mono_size(array_x, &total_size);
    array_range_mono_size(array_prob, &total_size_prob);

    if(total_size != total_size_prob)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NA);
        return;
    }

    { /* check dimensions match and check the sum of the prob array == 1.0 */
    F64 sum = 0.0;

    for(idx = 0; idx < total_size; ++idx)
    {
        EV_DATA ev_data;

        if(RPN_DAT_REAL != array_range_mono_index(&ev_data, array_x, idx, EM_REA | EM_STR | EM_BLK))
        {   /* non-numeric values are error */
            ss_data_free_resources(&ev_data);
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NUM);
            return;
        }

        ss_data_free_resources(&ev_data);

        if(RPN_DAT_REAL != array_range_mono_index(&ev_data, array_prob, idx, EM_REA | EM_STR | EM_BLK))
        {   /* non-numeric values are error */
            ss_data_free_resources(&ev_data);
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NUM);
            return;
        }

        sum += ev_data.arg.fp;

        ss_data_free_resources(&ev_data);
    }

    if(1.0 != sum)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NUM);
        return;
    }
    } /*block*/

    if(upper_limit >= lower_limit)
    {
        for(idx = 0; idx < total_size; ++idx)
        {
            EV_DATA ev_data;
            BOOL in_range = FALSE;

            if(RPN_DAT_REAL != array_range_mono_index(&ev_data, array_x, idx, EM_REA | EM_STR | EM_BLK))
            {   /* non-numeric values are error */
                ss_data_free_resources(&ev_data);
                ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NUM);
                continue;
            }

            if((lower_limit <= ev_data.arg.fp) && (ev_data.arg.fp <= upper_limit))
                in_range = TRUE;

            ss_data_free_resources(&ev_data);

            if(!in_range)
                continue;

            if(RPN_DAT_REAL != array_range_mono_index(&ev_data, array_prob, idx, EM_REA | EM_STR | EM_BLK))
            {   /* non-numeric values are error */
                ss_data_free_resources(&ev_data);
                ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NUM);
                continue;
            }

            prob_result += ev_data.arg.fp;

            ss_data_free_resources(&ev_data);
        }
    }
    /* else (upper_limit >= lower_limit) -> zero probability, OK */

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
    else
        ev_data_set_real(p_ev_data_res, prob_result);
}

/******************************************************************************
*
* NUMBER rsq(array_x, array_y)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rsq)
{
    /* call my friend to do the hard work */
    c_pearson(args, n_args, p_ev_data_res, p_cur_slr);

    if(!ev_data_is_error(p_ev_data_res))
    {
        const F64 r2 = mx_fsquare(p_ev_data_res->arg.fp);
        ev_data_set_real(p_ev_data_res, r2);
    }
}

/******************************************************************************
*
* NUMBER steyx(known_y:array,known_x:array)
*
******************************************************************************/

static void
steyx_calc(
    _InoutRef_  P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA known_y,
    _InRef_     PC_EV_DATA known_x)
{
    STATUS status = STATUS_OK;
    STATISTICS_PAIRED statistics_paired;
    zero_struct(statistics_paired);

    if(status_ok(status = statistics_paired_calc(&statistics_paired, known_x, known_y)))
    {
        S32 n = statistics_paired.x.n;

        F64 slope_numerator =
            statistics_paired.sum_of_product_of_deltas;

        F64 slope_denominator =
            statistics_paired.x.sum_of_delta2;

        F64 rh_term =
            (slope_numerator * slope_numerator)
            /
            slope_denominator;

        F64 lh_term =
            statistics_paired.y.sum_of_delta2;

        F64 steyx2_numerator = (lh_term - rh_term);
        S32 steyx2_denominator = ((n - 2));

        F64 steyx = sqrt(steyx2_numerator / steyx2_denominator);

        ev_data_set_real(p_ev_data_out, steyx);
    }

    if(status_fail(status))
        ev_data_set_error(p_ev_data_out, status);
}

PROC_EXEC_PROTO(c_steyx)
{
    const PC_EV_DATA known_y = args[0];
    const PC_EV_DATA known_x = args[1];

    exec_func_ignore_parms();

    steyx_calc(p_ev_data_res, known_y, known_x);
}

/* end of ev_fnstp.c */
