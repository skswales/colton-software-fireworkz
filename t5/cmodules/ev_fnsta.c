/* ev_fnsta.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Statistical function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "cmodules/mathxtra.h" /* for mx_atanh() */
#include "cmodules/mathxtr2.h" /* for linest() */
#include "cmodules/mathxtr3.h" /* for uniform_distribution() */
#include "cmodules/mathxtr4.h" /* for mx_ln_beta() */

/******************************************************************************
*
* Statistical functions
*
******************************************************************************/

/*
Statistical functions calculated using ARRAY_RANGE_XXX processing:
AVEDEV()
AVG()
COUNT()
COUNTA()
DEVSQ()
GEOMEAN()
HARMEAN()
KURT()
MAX()
MEDIAN()
MIN()
SKEW()
SKEW.P()
STD()
STDP()
SUMSQ()
VAR()
VARP()
*/

/*
NB Excel categorises some functions as Maths & Trig functions:
RAND()
RANDBETWEEN()
*/

/******************************************************************************
*
* NUMBER beta(a, b)
*
******************************************************************************/

PROC_EXEC_PROTO(c_beta)
{
    const F64 a = ss_data_get_real(args[0]);
    const F64 b = ss_data_get_real(args[1]);
    F64 beta_result;

    exec_func_ignore_parms();

    errno = 0;

    beta_result = exp(mx_ln_beta(a, b));

    ss_data_set_real(p_ss_data_res, beta_result);

    if(errno)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* bin(array1, array2)
*
* frequency(array1, array2)
*
******************************************************************************/

_Check_return_
static STATUS
bin_and_frequency_calc_init(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InVal_     S32 y_size_bins)
{
    /* make result array */
    status_return(ss_array_make(p_ss_data_out, 1, y_size_bins + 1));

    { /* clear result array to zero as widest integers */
    S32 iy;

    for(iy = 0; iy < y_size_bins + 1; ++iy)
    {
        P_SS_DATA p_ss_data = ss_array_element_index_wr(p_ss_data_out, 0, iy);
        ss_data_set_WORD32(p_ss_data, 0);
    }
    } /*block*/

    return(STATUS_OK);
}

static void
bin_and_frequency_calc_process_element(
    _InoutRef_  P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA array_bins,
    _InVal_     S32 y_size_bins,
    _InVal_     BOOL ignore_blanks_and_strings,
    _InRef_     PC_SS_DATA p_ss_data)
{
    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_STRING:
        if(ignore_blanks_and_strings)
        {   /* ignore this data item */
            break;
        }

        /*FALLTHRU*/

    case DATA_ID_REAL:
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
    case DATA_ID_DATE:
        {
        S32 bin_iy, bin_out_iy = y_size_bins;

        for(bin_iy = 0; bin_iy < y_size_bins; ++bin_iy)
        {
            SS_DATA ss_data_bin;
            const EV_IDNO bin_id = array_range_index(&ss_data_bin, array_bins, 0, bin_iy, EM_REA | EM_INT | EM_DAT | EM_STR);
            S32 res;

            if(ignore_blanks_and_strings && (DATA_ID_STRING == bin_id))
            {   /* can't match anything against this bin - skip */
                ss_data_free_resources(&ss_data_bin);
                continue;
            }

            res = ss_data_compare(p_ss_data, &ss_data_bin, FALSE, FALSE);

            ss_data_free_resources(&ss_data_bin);

            if(res <= 0)
            {
                bin_out_iy = bin_iy;
                break;
            }
        }

        ss_array_element_index_wr(p_ss_data_out, 0, bin_out_iy)->arg.integer += 1;
        break;
        }

    default:
        break;
    }
}

static void
bin_and_frequency_calc(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA array_data,
    _InRef_     PC_SS_DATA array_bins,
    _InVal_     BOOL ignore_blanks_and_strings)
{
    S32 x_size_data, x_size_bins;
    S32 y_size_data, y_size_bins;

    array_range_sizes(array_data, &x_size_data, &y_size_data);
    array_range_sizes(array_bins, &x_size_bins, &y_size_bins);

    /* make result array and then clear to zero as widest integers */
    if(status_fail(bin_and_frequency_calc_init(p_ss_data_out, y_size_bins)))
        return;

    { /* put each item from array_data into a bin in array_bins */
    S32 ix, iy;

    for(ix = 0; ix < x_size_data; ++ix)
    {
        for(iy = 0; iy < y_size_data; ++iy)
        {
            SS_DATA ss_data;

            (void) array_range_index(&ss_data, array_data, ix, iy, EM_REA | EM_INT | EM_DAT | EM_STR); /* no need for EM_BLK */
            
            bin_and_frequency_calc_process_element(p_ss_data_out, array_bins, y_size_bins, ignore_blanks_and_strings, &ss_data);

            ss_data_free_resources(&ss_data);
        }
    }
    } /*block*/
}

PROC_EXEC_PROTO(c_bin)
{
    const PC_SS_DATA array_data = args[0];
    const PC_SS_DATA array_bins = args[1];

    exec_func_ignore_parms();

    bin_and_frequency_calc(p_ss_data_res, array_data, array_bins, FALSE);
}

PROC_EXEC_PROTO(c_frequency)
{
    const PC_SS_DATA array_data = args[0];
    const PC_SS_DATA array_bins = args[1];

    exec_func_ignore_parms();

    bin_and_frequency_calc(p_ss_data_res, array_data, array_bins, TRUE);
}

/******************************************************************************
*
* NUMBER combin(n, k)
*
******************************************************************************/

/* nCk = (n k) = (n.(n-1).(n-2)...(n-k+1)) / (k.(k-1).(k-2)...1) */

/* Mathematically nCk is zero for k>n but most spreadsheets avoid this */

/* For k<=n, as factorials, nCk = (n k) = n!/((n-k)!*k!) */

extern void
binomial_coefficient_calc(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return integer or real or error */
    _InVal_     S32 n,
    _InVal_     S32 k)
{
    STATUS err = STATUS_OK;

    if( (n < 0) || (k < 0) /*|| ((n - k) < 0)*/ )
    {
        err = EVAL_ERR_ARGRANGE;
    }
    else if(k > n)
    {
        ss_data_set_integer(p_ss_data_out, 0);
    }
    else if( (k == 0) || (k == n) )
    {
        ss_data_set_integer(p_ss_data_out, 1);
    }
    else if(n <= 170) /* SKS maximum factorial that will fit in F64 */
    {
        SS_DATA ss_data_divisor;

        /* assume result will be (widest) integer to start with; function will go to fp as necessary */
        ss_data_set_WORD32(p_ss_data_out, (n - k) + 1); /* start */
        product_between_calc(p_ss_data_out, /*(n - k) + 1,*/ n); /* may return integer or real */

        /* calculate divisor in same format as dividend (either still WORD32 or REAL)
         * NB divisor is always smaller than dividend so this is OK
         */
        ss_data_set_WORD32(&ss_data_divisor, 1); /* start */
        if(ss_data_is_real(p_ss_data_out))
            ss_data_set_real(&ss_data_divisor, 1.0); /* start */
        assert_EQ(ss_data_get_data_id(&ss_data_divisor), ss_data_get_data_id(p_ss_data_out));
        product_between_calc(&ss_data_divisor, /*1,*/ k); /* may return integer or real */
        assert_EQ(ss_data_get_data_id(&ss_data_divisor), ss_data_get_data_id(p_ss_data_out));

        if(ss_data_is_real(p_ss_data_out))
        {
            /* binomial coefficient always integer result - see if we can get one! (may be out of integer range) */
            F64 binomial_coefficient_result = floor((ss_data_get_real(p_ss_data_out) / ss_data_get_real(&ss_data_divisor)) + 0.5);

            ss_data_set_real_try_integer(p_ss_data_out, binomial_coefficient_result);
        }
        else
        {
            /* no worries about remainders - combination always integer result! */
            S32 binomial_coefficient_result_integer = ss_data_get_integer(p_ss_data_out) / ss_data_get_integer(&ss_data_divisor);

            ss_data_set_integer(p_ss_data_out, binomial_coefficient_result_integer);
        }
    }
    else
    {
        F64 ln_binomial_coefficient;
        F64 binomial_coefficient_result;

        errno = 0;

        ln_binomial_coefficient = mx_ln_binomial_coefficient(n, k);

        /* binomial coefficient always integer result - see if we can get one! */
        binomial_coefficient_result = floor(exp(ln_binomial_coefficient) + 0.5);

        ss_data_set_real_try_integer(p_ss_data_out, binomial_coefficient_result);

        if(errno)
            err = status_from_errno();
    }

    if(status_fail(err))
        ss_data_set_error(p_ss_data_out, err);
}

PROC_EXEC_PROTO(c_combin)
{
    const S32 n = ss_data_get_integer(args[0]);
    const S32 k = ss_data_get_integer(args[1]);

    exec_func_ignore_parms();

    binomial_coefficient_calc(p_ss_data_res, n, k); /* may return integer or real or error */
}

/******************************************************************************
*
* REAL gammaln(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_gammaln)
{
    const F64 number = ss_data_get_real(args[0]);
    F64 gammaln_result;

    exec_func_ignore_parms();

    errno = 0;

    gammaln_result = lgamma(number);

    ss_data_set_real(p_ss_data_res, gammaln_result);

    if(errno)
        exec_func_status_return(p_ss_data_res, status_from_errno());
}

/******************************************************************************
*
* REAL grand({mean {, sd}})
*
******************************************************************************/

PROC_EXEC_PROTO(c_grand)
{
    F64 s = 1.0;
    F64 m = 0.0;
    F64 r;

    exec_func_ignore_parms();

    switch(n_args)
    {
    case 2:
        if((s = ss_data_get_real(args[1])) < 0.0)
            s = -s;

        /*FALLTHRU*/

    case 1:
        m = ss_data_get_real(args[0]);
        break;

    default:
        break;
    }

    consume_bool(uniform_distribution_test_seeded(true /*ensure*/));

    r  = normal_distribution();

    r *= s;
    r += m;

    ss_data_set_real(p_ss_data_res, r);
}

/******************************************************************************
*
* ARRAY listcount(array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_listcount)
{
    STATUS status = STATUS_OK;
    SS_DATA ss_data_temp_array;

    exec_func_ignore_parms();

    ss_data_set_blank(&ss_data_temp_array);

    for(;;)
    {
        S32 x_size, y_size;
        S32 n_unique = 0;
        S32 iy = 0;

        status_assert(ss_data_resource_copy(&ss_data_temp_array, args[0]));
        data_ensure_constant(&ss_data_temp_array);

        if(ss_data_is_error(&ss_data_temp_array))
        {
            ss_data_free_resources(p_ss_data_res);
            *p_ss_data_res = ss_data_temp_array;
            return;
        }

        status_break(status = array_sort(&ss_data_temp_array, 0));

        status_break(status = ss_array_make(p_ss_data_res, 0, 0));

        array_range_sizes(&ss_data_temp_array, &x_size, &y_size);

        while((iy < y_size) && status_ok(status))
        {
            SS_DATA ss_data;
            F64 count = 0;

            (void) array_range_index(&ss_data, &ss_data_temp_array, 0, iy, EM_CONST);

            while(status_ok(status))
            {
                SS_DATA ss_data_t;
                S32 res;

                (void) array_range_index(&ss_data_t, &ss_data_temp_array, 0, iy, EM_CONST);

                res = ss_data_compare(&ss_data, &ss_data_t, FALSE, FALSE);

                ss_data_free_resources(&ss_data_t);

                if(0 != res)
                {
                    if(status_ok(status = ss_array_element_make(p_ss_data_res, 1, n_unique)))
                    {
                        P_SS_DATA p_ss_data = ss_array_element_index_wr(p_ss_data_res, 0, n_unique);
                        status_assert(ss_data_resource_copy(p_ss_data, &ss_data));
                        ss_data_set_real_try_integer(&p_ss_data[1], count);
                        n_unique += 1;
                    }
                    break;
                }
                else if(x_size > 1)
                {
                    SS_DATA ss_data_count;
                    /* ignore content of all columns between the first (data) and last (count) */
                    if(DATA_ID_REAL == array_range_index(&ss_data_count, &ss_data_temp_array, x_size-1, iy, EM_REA))
                        count += ss_data_get_real(&ss_data_count); /* SKS 19may97 was just using [1, iy] before */
                    ss_data_free_resources(&ss_data_count);
                }
                else
                    count += 1.0;

                ++iy;
            }

            ss_data_free_resources(&ss_data);
        }

        break;
        /*NOTREACHED*/
    }

    ss_data_free_resources(&ss_data_temp_array);

    if(status_fail(status))
    {
        ss_data_free_resources(p_ss_data_res);
        ss_data_set_error(p_ss_data_res, status);
    }
}

/******************************************************************************
*
* NUMBER permut(n, k)
*
******************************************************************************/

/* nPk = n.(n-1).(n-2)...(n-k+1) */

/* Mathematically nPk is zero for k>n but most spreadsheets avoid this */

/* For k<=n, as factorials, nPk = n!/(n-k)! */

static void
permut_calc(
    _OutRef_    P_SS_DATA p_ss_data_out, /* may return integer or real or error */
    _InVal_     S32 n,
    _InVal_     S32 k)
{
    STATUS err = STATUS_OK;

    if( (n < 0) || (k < 0) /*|| ((n - k) < 0)*/ )
    {
        err = EVAL_ERR_ARGRANGE;
    }
    else if(k > n)
    {
        ss_data_set_integer(p_ss_data_out, 0);
    }
    else if(k == 0)
    {
        ss_data_set_integer(p_ss_data_out, 1);
    }
    else if(n <= 170) /* SKS maximum factorial that will fit in F64 */
    {
        /* assume result will be (widest) integer to start with; function will go to fp as necessary */
        ss_data_set_WORD32(p_ss_data_out, (n - k) + 1); /* start */
        product_between_calc(p_ss_data_out, /*(n - k) + 1,*/ n); /* may return integer or real */

        if(ss_data_is_integer(p_ss_data_out))
            ss_data_set_integer_size(p_ss_data_out);
    }
    else
    {
        F64 ln_numerator, ln_denominator;
        F64 permut_result;

        errno = 0;

        ln_numerator = mx_ln_factorial(n);
        ln_denominator = mx_ln_factorial(n - k);

        /* nPk always integer result - see if we can get one! */
        permut_result = floor(exp(ln_numerator - ln_denominator) + 0.5);

        ss_data_set_real_try_integer(p_ss_data_out, permut_result);

        if(errno)
            err = status_from_errno();
    }

    if(status_fail(err))
        ss_data_set_error(p_ss_data_out, err);
}

PROC_EXEC_PROTO(c_permut)
{
    const S32 n = ss_data_get_integer(args[0]);
    const S32 k = ss_data_get_integer(args[1]);

    exec_func_ignore_parms();

    permut_calc(p_ss_data_res, n, k); /* may return integer or real or error */
}

/******************************************************************************
*
* REAL rand({n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_rand)
{
    exec_func_ignore_parms();

    if(!uniform_distribution_test_seeded(false /*test*/))
    {
        if((0 != n_args) && (arg_get_real_INT(args[0]) != 0.0))
            uniform_distribution_seed((unsigned int) arg_get_real_INT(args[0]));
        else
            uniform_distribution_test_seeded(true /*ensure*/);
    }

    ss_data_set_real(p_ss_data_res, uniform_distribution());
}

/******************************************************************************
*
* ARRAY rank(array {, spearflag})
*
******************************************************************************/

PROC_EXEC_PROTO(c_rank)
{
    const PC_SS_DATA array_data = args[0];
    const bool spearman_correct = (n_args > 1) ? ss_data_get_logical(args[1]) : false;
    S32 x_size, y_size;

    exec_func_ignore_parms();

    array_range_sizes(array_data, &x_size, &y_size);

    /* make result array */
    if(status_ok(ss_array_make(p_ss_data_res, 2, y_size)))
    {
        S32 iy;

        for(iy = 0; iy < y_size; ++iy)
        {
            S32 iy_t;
            S32 position = 1;
            S32 equal = 1;
            SS_DATA ss_data;

            (void) array_range_index(&ss_data, array_data, 0, iy, EM_CONST);

            for(iy_t = 0; iy_t < y_size; ++iy_t)
            {
                if(iy_t != iy)
                {
                    SS_DATA ss_data_t;
                    S32 res;

                    (void) array_range_index(&ss_data_t, array_data, 0, iy_t, EM_CONST);

                    res = ss_data_compare(&ss_data, &ss_data_t, FALSE, FALSE);

                    ss_data_free_resources(&ss_data_t);

                    if(0 == res)
                        equal += 1;
                    else if(res < 0)
                        position += 1;
                    /* else */
                        /* found a larger value - we can't stop as array_data must be assumed to be unsorted */
                }
            }

            {
            P_SS_DATA p_ss_data = ss_array_element_index_wr(p_ss_data_res, 0, iy);

            if(spearman_correct) /* SKS 12apr95 make suitable for passing to spearman with equal values */
                ss_data_set_real(p_ss_data, position + (equal - 1.0) * 0.5);
            else
                ss_data_set_integer(p_ss_data, position);

            p_ss_data = ss_array_element_index_wr(p_ss_data_res, 1, iy);
            ss_data_set_integer(p_ss_data, equal);
            } /*block*/

            ss_data_free_resources(&ss_data);
        }
    }
}

/******************************************************************************
*
* REAL spearman(array1, array2)
*
******************************************************************************/

/* don't force data to numeric values for many statistics functions - caller tests for non-numeric */

#define statistics_array_range_index(p_ss_data_out, p_ss_data_in, ix, iy) \
    array_range_index(p_ss_data_out, p_ss_data_in, ix, iy, EM_REA | EM_STR | EM_BLK)

PROC_EXEC_PROTO(c_spearman)
{
    F64 spearman_result;
    S32 x_size[2];
    S32 y_size[2];
    S32 limit;
    S32 iy;
    F64 sum_d_squared = 0.0;
    S32 n_counted = 0;

    exec_func_ignore_parms();

    array_range_sizes(args[0], &x_size[0], &y_size[0]);
    array_range_sizes(args[1], &x_size[1], &y_size[1]);

    limit = MAX(y_size[0], y_size[1]);

    for(iy = 0; iy < limit; ++iy)
    {
        SS_DATA ss_data[2];
        const EV_IDNO ev_idno_0 = statistics_array_range_index(&ss_data[0], args[0], 0, iy);
        const EV_IDNO ev_idno_1 = statistics_array_range_index(&ss_data[1], args[1], 0, iy);

        if( (DATA_ID_REAL == ev_idno_0) && (DATA_ID_REAL == ev_idno_1) ) /* ignore non-numeric values */
        {
            const F64 d = ss_data_get_real(&ss_data[1]) - ss_data_get_real(&ss_data[0]);
            const F64 d_squared = d * d;
            sum_d_squared += d_squared;
            n_counted += 1;
        }

        ss_data_free_resources(&ss_data[0]);
        ss_data_free_resources(&ss_data[1]);
    }

    if(0 == n_counted)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_NO_VALID_DATA);

    spearman_result = (1.0 - (6.0 * sum_d_squared) / ((F64) n_counted * ((F64) n_counted * (F64) n_counted - 1.0)));

    ss_data_set_real(p_ss_data_res, spearman_result);
}

/* end of ev_fnsta.c */
