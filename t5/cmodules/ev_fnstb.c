/* ev_fnstb.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2022 Stuart Swales */

/* Statistical function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "cmodules/mathxtr2.h" /* for linest() */
#include "cmodules/mathxtr3.h" /* for uniform_distribution() */
#include "cmodules/mathxtr4.h" /* for mx_ln_beta() */

/******************************************************************************
*
* Statistical functions
*
******************************************************************************/

/* don't force data to numeric values for many statistics functions - caller tests for non-numeric */

#define statistics_array_range_index(p_ss_data_out, p_ss_data_in, ix, iy) \
    array_range_index(p_ss_data_out, p_ss_data_in, ix, iy, EM_REA | EM_STR | EM_BLK)

/* flatten so we can use functions on horizontal and rectangular ranges as well as vertical, excluding strings and blanks */

_Check_return_
static STATUS
statistics_array_flatten_copy(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in)
{
    S32 x_size, y_size, total_size;
    S32 iy_out = 0;

    array_range_sizes(p_ss_data_in, &x_size, &y_size);

    total_size = x_size * y_size;

    status_return(ss_array_make(p_ss_data_out, 1, total_size));

    {
    S32 iy;

    for(iy = 0; iy < y_size; ++iy)
    {
        S32 ix;

        for(ix = 0; ix < x_size; ++ix)
        {
            SS_DATA ss_data;

            if(DATA_ID_REAL != statistics_array_range_index(&ss_data, p_ss_data_in, ix, iy))
            {   /* ignore non-numeric values such as strings and blank cells */
                if(ss_data_is_error(&ss_data))
                {
                    ss_data_free_resources(p_ss_data_out);
                    *p_ss_data_out = ss_data;
                    return(ss_data.arg.ss_error.status);
                }

                ss_data_free_resources(&ss_data);
                continue;
            }

            *ss_array_element_index_wr(p_ss_data_out, 0, iy_out) = ss_data; /* steal that copy */
            ++iy_out;
        }
    }
    } /*block*/

    p_ss_data_out->arg.ss_array.y_size = iy_out; /* hacky resize - don't free any memory */

    return(STATUS_OK);
}

/* only returns numeric values that the caller can use */

_Check_return_
extern BOOL
statistics_value_next(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in,
    _InoutRef_  P_S32 p_ix,
    _InoutRef_  P_S32 p_iy,
    _InRef_     S32 x_size,
    _InRef_     S32 y_size)
{
    SS_DATA ss_data;
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
                ss_data_set_blank(p_ss_data_out);
                *p_ix = x_size;
                *p_iy = y_size;
                return(FALSE);
            }
        }

        if(DATA_ID_REAL != statistics_array_range_index(&ss_data, p_ss_data_in, ix, iy))
        {   /* ignore non-numeric values */
            ss_data_free_resources(&ss_data);
            continue;
        }

        *p_ss_data_out = ss_data;
        *p_ix = ix;
        *p_iy = iy;
        return(TRUE);
    }
}

/******************************************************************************
*
* NUMBER combina(n, m)
*
******************************************************************************/

PROC_EXEC_PROTO(c_combina)
{
    const S32 n = ss_data_get_integer(args[0]);
    const S32 m = ss_data_get_integer(args[1]);

    exec_func_ignore_parms();

    binomial_coefficient_calc(p_ss_data_res, n + m - 1, n - 1); /* may return integer or real or error */
}

/******************************************************************************
*
* NUMBER fisher(x:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_fisher)
{
    const F64 x = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    errno = 0;

    ss_data_set_real(p_ss_data_res, atanh(x));

    /* input of magnitude 1 or greater invalid */
    /* input of near magnitude 1 causes overflow */
    if(errno /* == EDOM, ERANGE */)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* NUMBER fisherinv(y:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_fisherinv)
{
    const F64 y = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    ss_data_set_real(p_ss_data_res, tanh(y));

    /* no error cases */
}

/******************************************************************************
*
* REAL gamma(n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_gamma)
{
    const F64 number = ss_data_get_real(args[0]);
    F64 gamma_result;

    exec_func_ignore_parms();

    errno = 0;

    gamma_result = tgamma(number);

    ss_data_set_real(p_ss_data_res, gamma_result);

    if(errno)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* NUMBER large(array, k:integer)
*
* NUMBER small(array, k:integer)
*
* Unfortunately we can't do these with ARRAY_RANGE_XXX processing
* as they require the k parameter after the array
*
******************************************************************************/

static void
large_small_common(
    _InRef_     PC_SS_DATA arg0,
    _InRef_     PC_SS_DATA arg1,
    _InoutRef_  P_SS_DATA p_ss_data_out,
    _InVal_     bool fLarge)
{
    const S32 k_idx = ss_data_get_integer(arg1) - 1;
    SS_DATA ss_data_temp_array;
    S32 x_size, y_size;

    /* can trivially index by sorting a flattened copy of the source */
    status_assert(statistics_array_flatten_copy(&ss_data_temp_array, arg0));

    if(ss_data_is_error(&ss_data_temp_array))
    {
        *p_ss_data_out = ss_data_temp_array;
        return;
    }

    array_range_sizes(&ss_data_temp_array, &x_size, &y_size);

    if(0 == y_size)
    {
        ss_data_free_resources(&ss_data_temp_array);
        ss_data_set_error(p_ss_data_out, EVAL_ERR_NO_VALID_DATA);
        return;
    }

    if( (k_idx < 0) || (k_idx >= y_size) )
    {
        ss_data_free_resources(&ss_data_temp_array);
        ss_data_set_error(p_ss_data_out, EVAL_ERR_OUTOFRANGE);
        return;
    }

    status_consume(array_sort(&ss_data_temp_array, 0));

    {
    SS_DATA ss_data;
    S32 iy;

    if(fLarge /* RPN_FNF_LARGE */)
        iy = y_size - k_idx;
    else /* RPN_FNF_SMALL */
        iy = k_idx;

    (void) array_range_index(&ss_data, &ss_data_temp_array, 0, iy, EM_CONST);
    status_assert(ss_data_resource_copy(p_ss_data_out, &ss_data));
    consume_bool(ss_data_real_to_integer_try(p_ss_data_out));
    } /*block*/

    ss_data_free_resources(&ss_data_temp_array);
}

PROC_EXEC_PROTO(c_large)
{
    exec_func_ignore_parms();

    large_small_common(args[0], args[1], p_ss_data_res, true /*RPN_FNF_LARGE*/);
}

PROC_EXEC_PROTO(c_small)
{
    exec_func_ignore_parms();

    large_small_common(args[0], args[1], p_ss_data_res, false /*RPN_FNF_SMALL*/);
}

/******************************************************************************
*
* NUMBER median(list)
*
******************************************************************************/

/* now performed by ARRAY_RANGE_MEDIAN */

/* calculate the median of a span of data in a one-dimensional vertical array - we need to use non-zero start for Q3 calculation */

_Check_return_
extern F64
median_calc_span(
    _InRef_     PC_SS_DATA p_ss_data,
    _InVal_     S32 y_start /*incl*/,
    _InVal_     S32 y_end   /*excl*/)
{
    const S32 ys_effective = (y_end - y_start);
    const S32 ys_half = ys_effective / 2;
    SS_DATA ss_data;
    F64 median;

    (void) array_range_index(&ss_data, p_ss_data, 0, y_start + ys_half, EM_REA);
    median = ss_data_get_real(&ss_data);

    if(0 == (ys_effective & 1))
    {   /* there are an even number of elements in this span, so the median is the mean of the two middle ones */
        (void) array_range_index(&ss_data, p_ss_data, 0, y_start + ys_half - 1, EM_REA);
        median = (median + ss_data_get_real(&ss_data)) * 0.5;
    }

    return(median);
}

/******************************************************************************
*
* NUMBER mode.sngl(array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_mode_sngl)
{
    const PC_SS_DATA array_data = args[0];
    STATUS status = STATUS_OK;
    S32 total_size, idx;
    S32 current_mode_count = 0;
    F64 mode_result = 0.0;

    exec_func_ignore_parms();

    array_range_mono_size(array_data, &total_size);

    for(idx = 0; idx < total_size; ++idx)
    {
        SS_DATA ss_data;
        S32 count;
        S32 test_idx;

        if(DATA_ID_REAL != array_range_mono_index(&ss_data, array_data, idx, EM_REA | EM_STR | EM_BLK))
        {   /* ignore non-numeric values */
            ss_data_free_resources(&ss_data);
            continue;
        }

        count = 1; /* there is always one instance of the value we just read */

        for(test_idx = 0; test_idx < total_size; ++test_idx)
        {
            if(test_idx != idx)
            {
                SS_DATA ss_data_test;
                S32 res;

                if(DATA_ID_REAL != array_range_mono_index(&ss_data_test, array_data, test_idx, EM_REA | EM_STR | EM_BLK))
                {   /* ignore non-numeric values */
                    ss_data_free_resources(&ss_data_test);
                    continue;
                }

                res = ss_data_compare(&ss_data, &ss_data_test, FALSE, FALSE);

                ss_data_free_resources(&ss_data_test);

                if(0 == res)
                    ++count;
            }
        }

        if(current_mode_count < count)
        {
            current_mode_count = count;
            mode_result = ss_data_get_real(&ss_data);
        }

        ss_data_free_resources(&ss_data);
    }

    if(0 == current_mode_count)
        status = EVAL_ERR_NO_VALID_DATA;

    exec_func_status_return(p_ss_data_res, status);

    ss_data_set_real(p_ss_data_res, mode_result);
}

/******************************************************************************
*
* NUMBER percentile.inc(array, percentile)
*
* NUMBER percentile.exc(array, percentile)
*
* Unfortunately we can't do this with ARRAY_RANGE_XXX processing
* as it requires the percentile parameter after the array
*
******************************************************************************/

/* calculate the percentile of a span of data in a one-dimensional vertical array - reused for quartile */

/* Try estimating percentile using an Excel-compatible method */
/* Credit: https://en.wikipedia.org/wiki/Percentile#Microsoft_Excel_method */

_Check_return_
static F64
percentile_calc(
    _InRef_     PC_SS_DATA p_ss_data,
    _InVal_     S32 y_size, /* N */
    _InVal_     F64 percentile, /* (0.0..1.0) */
    _InVal_     BOOL exclusive)
{
    const S32 alpha = exclusive ? 0 : 1;
    const S32 Npm1 = (y_size + 1 - 2*alpha);
    F64 n = (percentile * Npm1) + alpha;

    /* decompose n into integer and fractional parts */
    S32 k = (S32) n;
    F64 d = n - k;

    SS_DATA ss_data;
    F64 v_k, v_kp1;
    F64 v_p;

    /* NB Data indices of algorithm are one-based; our array_range indices are zero-based */

    if(exclusive && (k == 0))
    {   /* exclusive: bottom end-stop */
        (void) array_range_index(&ss_data, p_ss_data, 0, (1) - 1, EM_REA);
        v_p = ss_data_get_real(&ss_data);
    }
    else if(exclusive && (k >= y_size))
    {   /* exclusive: top end-stop */
        (void) array_range_index(&ss_data, p_ss_data, 0, (y_size) - 1, EM_REA);
        v_p = ss_data_get_real(&ss_data);
    }
    else
    {   /* linear interpolation between values */
        (void) array_range_index(&ss_data, p_ss_data, 0, (k + 0) - 1, EM_REA);
        v_k = ss_data_get_real(&ss_data);

        if(0.0 == d)
            v_p = v_k;
        else
        {
            (void) array_range_index(&ss_data, p_ss_data, 0, (k + 1) - 1, EM_REA);
            v_kp1 = ss_data_get_real(&ss_data);

            v_p = v_k + d * (v_kp1 - v_k);
        }
    }

    return(v_p);
}

static void
percentile_inc_exc_common(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return integer or real or error */
    _InRef_     PC_SS_DATA array_data,
    _InVal_     F64 percentile_fraction,
    _InVal_     BOOL exclusive)
{
    STATUS status = STATUS_OK;
    SS_DATA ss_data_temp_array;
    S32 x_size, y_size;

    if( (percentile_fraction < 0.0) || (percentile_fraction > 1.0) )
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    ss_data_set_blank(&ss_data_temp_array);

    /* find percentile by sorting a flattened copy of the source (as we did for median) */
    status_assert(statistics_array_flatten_copy(&ss_data_temp_array, array_data));

    if(ss_data_is_error(&ss_data_temp_array))
    {
        /*ss_data_free_resources(p_ss_data_out);*/
        *p_ss_data_out = ss_data_temp_array;
        return;
    }

    array_range_sizes(&ss_data_temp_array, &x_size, &y_size);

    if(0 == y_size)
        status = EVAL_ERR_NO_VALID_DATA;
    else
    {
        F64 percentile_result;

        status_consume(array_sort(&ss_data_temp_array, 0));

        percentile_result = percentile_calc(&ss_data_temp_array, y_size, percentile_fraction, exclusive);

        ss_data_set_real_try_integer(p_ss_data_out, percentile_result);
    }

    ss_data_free_resources(&ss_data_temp_array);

    if(status_fail(status))
    {
        /*ss_data_free_resources(p_ss_data_out);*/
        ss_data_set_error(p_ss_data_out, status);
    }
}

PROC_EXEC_PROTO(c_percentile_exc)
{
    const PC_SS_DATA array_data = args[0];
    const F64 percentile_fraction = ss_data_get_real(args[1]);

    exec_func_ignore_parms();

    percentile_inc_exc_common(p_ss_data_res, array_data, percentile_fraction, TRUE/*exclusive*/); /* may return integer or real or error */
}

PROC_EXEC_PROTO(c_percentile_inc)
{
    const PC_SS_DATA array_data = args[0];
    const F64 percentile_fraction = ss_data_get_real(args[1]);

    exec_func_ignore_parms();

    percentile_inc_exc_common(p_ss_data_res, array_data, percentile_fraction, FALSE/*exclusive*/); /* may return integer or real or error */
}

/******************************************************************************
*
* NUMBER percentrank.inc(array, x {, significance})
*
******************************************************************************/

_Check_return_
static inline F64
interpolate_fraction_between(
    _InVal_     F64 lower,
    _InVal_     F64 upper,
    _InVal_     F64 value)
{
    F64 full_span = upper - lower;
    F64 this_span = value - lower;

    return(this_span / full_span); /* 0.0..1.0 */
}

static void
percentrank_inc_exc_common(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return real or error */
    _InRef_     PC_SS_DATA array_data,
    _InRef_     PC_SS_DATA number,
    _InVal_     S32 decimal_places,
    _InVal_     BOOL exclusive)
{
    F64 percentrank_result = 0.0;
    const S32 alpha = exclusive ? 0 : 1;

    { /* first off, get the position using code cribbed from rank.eq */
    SS_DATA ss_data_temp_array;
    S32 x_size, y_size;
    const S32 order = 1;
    S32 position = 1;
    BOOL found = FALSE;
    S32 iy;

    /* perform rank.eq by sorting a flattened copy of the source (as we did for median) */
    status_assert(statistics_array_flatten_copy(&ss_data_temp_array, array_data));

    if(ss_data_is_error(&ss_data_temp_array))
    {
        *p_ss_data_out = ss_data_temp_array;
        return;
    }

    array_range_sizes(&ss_data_temp_array, &x_size, &y_size);

    status_consume(array_sort(&ss_data_temp_array, 0));

    for(iy = 0; iy < y_size; ++iy)
    {
        SS_DATA ss_data_t;
        S32 res;

        (void) array_range_index(&ss_data_t, &ss_data_temp_array, 0, iy, EM_REA);

        res = ss_data_compare(number, &ss_data_t, FALSE, FALSE);

        ss_data_free_resources(&ss_data_t);

        if(0 == res)
        {
            const S32 divisor = (y_size + 1 - 2*alpha);
            percentrank_result = ((F64) position - alpha) / divisor;
            /* [1,y_size] -> [0,1] for .INC */
            /* [1,y_size] -> (0,1) for .EXC */
            found = TRUE;
            break;
        }
        else
        {
            if(0 != order)
                res = -res; /* reverse order */
            if(res < 0)
                position += 1;
            else
            {   /* found a larger value - we can stop as we are operating on sorted data */
                /* need to interpolate between this and the previous one (if there is one) */
                if(iy != 0)
                {
                    const S32 divisor = (y_size + 1 - 2*alpha);
                    F64 this_value = ss_data_get_real(&ss_data_t);
                    F64 fraction_beween;
                    (void) array_range_index(&ss_data_t, &ss_data_temp_array, 0, iy - 1, EM_REA);
                    fraction_beween = interpolate_fraction_between(ss_data_get_real(&ss_data_t), this_value, ss_data_get_real(number));
                    percentrank_result = ((((F64) position - 1) - alpha) + fraction_beween) / divisor;
                    /* [1,y_size] -> [0,1] for .INC */
                    /* [1,y_size] -> (0,1) for .EXC */
                    found = TRUE;
                }
                break;
            }
        }
    }

    ss_data_free_resources(&ss_data_temp_array);

    if(!found)
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_OUTOFRANGE);
        return;
    }

    } /*block*/

    { /* round as desired */
    F64 multiplier = pow(10.0, (F64) decimal_places);
    percentrank_result *= multiplier;
    percentrank_result = (int) (percentrank_result + 0.5);
    percentrank_result /= multiplier;
    } /*block*/

    ss_data_set_real(p_ss_data_out, percentrank_result);
}

PROC_EXEC_PROTO(c_percentrank_exc)
{
    const PC_SS_DATA array_data = args[0];
    const PC_SS_DATA number = args[1];
    const S32 decimal_places = (n_args > 2) ? ss_data_get_integer(args[2]) : 3;

    exec_func_ignore_parms();

    percentrank_inc_exc_common(p_ss_data_res, array_data, number, decimal_places, TRUE/*exclusive*/); /* may return real or error */
}

PROC_EXEC_PROTO(c_percentrank_inc)
{
    const PC_SS_DATA array_data = args[0];
    const PC_SS_DATA number = args[1];
    const S32 decimal_places = (n_args > 2) ? ss_data_get_integer(args[2]) : 3;

    exec_func_ignore_parms();

    percentrank_inc_exc_common(p_ss_data_res, array_data, number, decimal_places, FALSE/*exclusive*/); /* may return real or error */
}

/******************************************************************************
*
* NUMBER quartile.inc(array, quartile)
*
* NUMBER quartile.exc(array, quartile)
*
* Unfortunately we can't do this with ARRAY_RANGE_XXX processing
* as it requires the quartile parameter after the array
*
******************************************************************************/

#if 1

/* Try estimating quartile using an Excel-compatible method */

PROC_EXEC_PROTO(c_quartile_exc)
{
    const PC_SS_DATA array_data = args[0];
    const S32 quartile = ss_data_get_integer(args[1]);
    const F64 percentile_fraction = quartile / 4.0;

    exec_func_ignore_parms();

    percentile_inc_exc_common(p_ss_data_res, array_data, percentile_fraction, TRUE/*exclusive*/); /* may return integer or real or error */
}

PROC_EXEC_PROTO(c_quartile_inc)
{
    const PC_SS_DATA array_data = args[0];
    const S32 quartile = ss_data_get_integer(args[1]);
    const F64 percentile_fraction = quartile / 4.0;

    exec_func_ignore_parms();

    percentile_inc_exc_common(p_ss_data_res, array_data, percentile_fraction, FALSE/*exclusive*/); /* may return integer or real or error */
}

#else

/* IMHO A better method but not Excel-compatible */

PROC_EXEC_PROTO(c_quartile_inc)
{
    STATUS status = STATUS_OK;
    SS_DATA ss_data_temp_array;
    S32 x_size, y_size;
    const S32 quartile = ss_data_get_integer(args[1]);

    exec_func_ignore_parms();

    if( (quartile < 0) || (quartile > 4) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    ss_data_set_blank(&ss_data_temp_array);

    /* find quartile by sorting a flattened copy of the source (as we did for median) */
    status_assert(statistics_array_flatten_copy(&ss_data_temp_array, args[0]));

    if(ss_data_is_error(&ss_data_temp_array))
    {
        ss_data_free_resources(p_ss_data_res);
        *p_ss_data_res = ss_data_temp_array;
        return;
    }

    array_range_sizes(&ss_data_temp_array, &x_size, &y_size);

    if(0 == y_size)
        status = EVAL_ERR_NO_VALID_DATA;
    else
    {
        F64 quartile_result;

        status_consume(array_sort(&ss_data_temp_array, 0));

        { /* A better method but not Excel-compatible */
        SS_DATA ss_data;

        /* NB Data indices of algorithm are one-based; our array_range indices are zero-based */

        switch(quartile)
        {
        default: default_unhandled();
        case 0: /* minimum value */
            (void) array_range_index(&ss_data, &ss_data_temp_array, 0, 0, EM_REA);
            quartile_result = ss_data_get_real(&ss_data);
            break;

        case 1: /* Q1 */
            {
            S32 n = y_size >> 2; /* y_size / 4 */

            switch(y_size & 3)
            {
            default: default_unhandled();
            case 0: /* there are an even number of elements, so Q1 is the median of the lower half */
            case 2:
                quartile_result = median_calc_span(&ss_data_temp_array, 0, n << 1);
                break;

            case 1: /* 4n+1 */
                (void) array_range_index(&ss_data, &ss_data_temp_array, 0, (n + 0) - 1, EM_REA); /* (n)th value of the ordered data set */
                quartile_result  = 0.25 * ss_data_get_real(&ss_data);
                (void) array_range_index(&ss_data, &ss_data_temp_array, 0, (n + 1) - 1, EM_REA); /* (n+1)th value */
                quartile_result += 0.75 * ss_data_get_real(&ss_data);
                break;

            case 3: /* 4n+3 */
                (void) array_range_index(&ss_data, &ss_data_temp_array, 0, (n + 1) - 1, EM_REA); /* (n+1)th value of the ordered data set */
                quartile_result  = 0.75 * ss_data_get_real(&ss_data);
                (void) array_range_index(&ss_data, &ss_data_temp_array, 0, (n + 2) - 1, EM_REA); /* (n+2)th value */
                quartile_result += 0.25 * ss_data_get_real(&ss_data);
                break;
            }

            break;
            }

        case 2: /* Q2 = median value */
            quartile_result = median_calc_span(&ss_data_temp_array, 0, y_size);
            break;
 
        case 3: /* Q3 */
            {
            S32 n = y_size >> 2; /* y_size / 4 */

            switch(y_size & 3)
            {
            default: default_unhandled();
            case 0: /* there are an even number of elements, so Q1 is the median of the upper half */
            case 2:
                quartile_result = median_calc_span(&ss_data_temp_array, n << 1, y_size);
                break;

            case 1: /* 4n+1 */
                (void) array_range_index(&ss_data, &ss_data_temp_array, 0, (3*n + 1) - 1, EM_REA); /* (3n+1)th value of the ordered data set */
                quartile_result  = 0.75 * ss_data_get_real(&ss_data);
                (void) array_range_index(&ss_data, &ss_data_temp_array, 0, (3*n + 2) - 1, EM_REA); /* (3n+2)th value */
                quartile_result += 0.25 * ss_data_get_real(&ss_data);
                break;

            case 3: /* 4n+3 */
                (void) array_range_index(&ss_data, &ss_data_temp_array, 0, (3*n + 2) - 1, EM_REA); /* (3n+2)th value of the ordered data set */
                quartile_result  = 0.25 * ss_data_get_real(&ss_data);
                (void) array_range_index(&ss_data, &ss_data_temp_array, 0, (3*n + 3) - 1, EM_REA); /* (3n+3)th value */
                quartile_result += 0.75 * ss_data_get_real(&ss_data);
                break;
            }

            break;
            }

        case 4: /* maximum value */
            (void) array_range_index(&ss_data, &ss_data_temp_array, 0, y_size - 1, EM_REA);
            quartile_result = ss_data_get_real(&ss_data);
            break;
        }
        } /*block*/

        ss_data_set_real_try_integer(p_ss_data_res, quartile_result);
    }

    ss_data_free_resources(&ss_data_temp_array);

    if(status_fail(status))
    {
        ss_data_free_resources(p_ss_data_res);
        ss_data_set_error(p_ss_data_res, status);
    }
}

#endif

/******************************************************************************
*
* NUMBER randbetween(bottom, top)
*
******************************************************************************/

PROC_EXEC_PROTO(c_randbetween)
{
    F64 bottom = ss_data_get_real(args[0]); /* NB these need not be integers */
    F64 top = ss_data_get_real(args[1]);
    F64 lower_inc = ceil(bottom);
    F64 upper_exc = floor(top + 1.0); /* extend range so that we are sure we can possibly return top */
    F64 randbetween_result;

    exec_func_ignore_parms();

    if(bottom > top)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_OUTOFRANGE);

    consume_bool(uniform_distribution_test_seeded(true /*ensure*/));

    if(lower_inc == upper_exc)
    {
        randbetween_result = lower_inc;
    }
    else
    {
#if 0 /* test harness for h.r.b. */
        if(upper_exc < 64.0)
        {
            F64 f64 = host_rand_between((U32) lower_inc, (U32) upper_exc);
            randbetween_result = f64;
        }
        else
#endif
        for(;;)
        {
            F64 f64 = uniform_distribution(); /* generates a random number on [0,1) */
            f64 *= (upper_exc - lower_inc);
            f64 += lower_inc;
            randbetween_result = floor(f64);
            if(randbetween_result < upper_exc)
                break;
            /* retry for range failure */
        }
    }

    ss_data_set_real_try_integer(p_ss_data_res, randbetween_result);
}

/******************************************************************************
*
* INTEGER rank.eq(number, array {, order:number})
*
******************************************************************************/

PROC_EXEC_PROTO(c_rank_eq)
{
    const PC_SS_DATA number = args[0];
    const PC_SS_DATA array_data = args[1];
    const S32 order = (n_args > 2) ? ss_data_get_integer(args[2]) : 0;
    SS_DATA ss_data_temp_array;
    S32 x_size, y_size;
    S32 position = 1;
    S32 equal = 0;
    S32 iy;

    exec_func_ignore_parms();

    /* perform rank by sorting a flattened copy of the source (as we did for median) */
    status_assert(statistics_array_flatten_copy(&ss_data_temp_array, array_data));

    if(ss_data_is_error(&ss_data_temp_array))
    {
        ss_data_free_resources(p_ss_data_res);
        *p_ss_data_res = ss_data_temp_array;
        return;
    }

    array_range_sizes(&ss_data_temp_array, &x_size, &y_size);

    status_consume(array_sort(&ss_data_temp_array, 0));

    for(iy = 0; iy < y_size; ++iy)
    {
        SS_DATA ss_data_t;
        S32 res;

        (void) array_range_index(&ss_data_t, &ss_data_temp_array, 0, iy, EM_REA);

        res = ss_data_compare(number, &ss_data_t, FALSE, FALSE);

        ss_data_free_resources(&ss_data_t);

        if(0 == res)
            equal += 1;
        else
        {
            if(0 != order)
                res = -res; /* reverse order */
            if(res < 0)
                position += 1;
            else
            {   /* found a larger value - we can stop as we are operating on sorted data */
                break;
            }
        }
    }

    ss_data_free_resources(&ss_data_temp_array);

    ss_data_set_integer(p_ss_data_res, position);
}

/******************************************************************************
*
* REAL standardize(x:number, mean:number, sigma:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_standardize)
{
    F64 standardize_result;
    const F64 x = ss_data_get_real(args[0]);
    const F64 mean = ss_data_get_real(args[1]);
    const F64 sigma = ss_data_get_real(args[2]);

    exec_func_ignore_parms();

    standardize_result = (x - mean) / sigma;

    ss_data_set_real(p_ss_data_res, standardize_result);
}

/******************************************************************************
*
* NUMBER trimmean(array, fraction)
*
* Unfortunately we can't do this with ARRAY_RANGE_XXX processing
* as it requires the fraction parameter after the array
*
******************************************************************************/

PROC_EXEC_PROTO(c_trimmean)
{
    STATUS status = STATUS_OK;
    SS_DATA ss_data_temp_array;
    S32 x_size, y_size;
    const F64 fraction = ss_data_get_real(args[1]);

    exec_func_ignore_parms();

    ss_data_set_blank(&ss_data_temp_array);

    if( (fraction < 0.0) || (fraction > 1.0) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    /* perform trimmean by sorting a flattened copy of the source (as we did for median) */
    status_assert(statistics_array_flatten_copy(&ss_data_temp_array, args[0]));

    if(ss_data_is_error(&ss_data_temp_array))
    {
        ss_data_free_resources(p_ss_data_res);
        *p_ss_data_res = ss_data_temp_array;
        return;
    }

    array_range_sizes(&ss_data_temp_array, &x_size, &y_size);

    if(0 == y_size)
        status = EVAL_ERR_NO_VALID_DATA;
    else
    {
        F64 mean = 0.0;
        S32 lower_incl, upper_excl;

        status_consume(array_sort(&ss_data_temp_array, 0));

        lower_incl = (int) (y_size * (fraction * 0.5));
        upper_excl = y_size - lower_incl;

        if(lower_incl < upper_excl)
        {
            const S32 range = upper_excl - lower_incl;
            S32 iy;

            for(iy = lower_incl; iy < upper_excl; ++iy)
            {
                SS_DATA ss_data;

                (void) array_range_index(&ss_data, &ss_data_temp_array, 0, iy, EM_REA);

                mean += ss_data_get_real(&ss_data);
            }

            mean /= range;
        }

        ss_data_set_real_try_integer(p_ss_data_res, mean);
    }

    ss_data_free_resources(&ss_data_temp_array);

    if(status_fail(status))
    {
        ss_data_free_resources(p_ss_data_res);
        ss_data_set_error(p_ss_data_res, status);
    }
}

/* end of ev_fnstb.c */
