/* ev_matb.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2016 Stuart Swales */

/* More mathematical function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#if __STDC_VERSION__ < 199901L

#if WINDOWS

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h" /* for _log2_e */
#endif

_Check_return_
static double
log2(_InVal_ double d)
{
    return(log(d) * _log2_e);
}

#endif /* OS */

#endif /* __STDC_VERSION__ */

/******************************************************************************
*
* More mathematical functions for OpenDocument / Microsoft Excel support
*
******************************************************************************/

/******************************************************************************
*
* NUMBER factdouble(n)
*
******************************************************************************/

static void
factdouble_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return fp or error */
    _InVal_     S32 n)
{
    if(n <= 3)
    {
        if(n < -1)
        {
            ev_data_set_error(p_ev_data_out, EVAL_ERR_ARGRANGE);
            return;
        }

        ev_data_set_real(p_ev_data_out, (n > 0) ? n : 1); /* 3!!=3, 2!!=2, 1!!=1, 0!!=1, -1!!=1 */
        return;
    }

    if(0 == (n & 1)) /* is_even */
    {   /* where n = 2k, n!! = 2^k * k! */
        const S32 k = n >> 1;

        factorial_calc(p_ev_data_out, k); /* may return integer or fp or error */

        switch(p_ev_data_out->did_num)
        {
        case RPN_DAT_WORD32:
            ev_data_set_real(p_ev_data_out, (F64) p_ev_data_out->arg.integer); /* now go to fp for the single multiply */

            /*FALLTHRU*/

        case RPN_DAT_REAL:
            p_ev_data_out->arg.fp *= pow(2.0, k);
            break;

        default:
        case RPN_DAT_ERROR:
            break;
        }
    }
    else /* is_odd */
    {   /* n!! = n! / (n - 1)!! */
        EV_DATA ev_data_numer;
        EV_DATA ev_data_denom;

        factorial_calc(&ev_data_numer, n); /* may return integer or fp or error */

        factdouble_calc(&ev_data_denom, n - 1); /* may return fp or error */

        if(!two_nums_divide_try(p_ev_data_out, &ev_data_numer, &ev_data_denom, TRUE /*propogate_errors*/))
            ev_data_set_error(p_ev_data_out, EVAL_ERR_CALC_FAILURE);
    }
}

PROC_EXEC_PROTO(c_factdouble)
{
    const S32 n = args[0]->arg.integer;

    exec_func_ignore_parms();

    factdouble_calc(p_ev_data_res, n); /* may return fp or error */

    consume_bool(real_to_integer_try(p_ev_data_res));
}

/******************************************************************************
*
* NUMBER odf.int(number)
*
* NB OpenDocument and Microsoft Excel truncate towards -infinity, unlike Fireworkz, PipeDream and Lotus 1-2-3
*
******************************************************************************/

PROC_EXEC_PROTO(c_odf_int)
{
    exec_func_ignore_parms();

    if(RPN_DAT_REAL == args[0]->did_num)
    {
        ev_data_set_real_ti(p_ev_data_res, real_floor(args[0]->arg.fp));
        return;
    }

    /* all other types can be handled by our friend */
    c_int(args, n_args, p_ev_data_res, p_cur_slr);
}

/******************************************************************************
*
* REAL log(x:number {, base:number=10})
*
******************************************************************************/

PROC_EXEC_PROTO(c_log)
{
    const F64 x = args[0]->arg.fp;
    const F64 b = (n_args > 1) ? args[1]->arg.fp : 10.0;
    F64 log_result;

    exec_func_ignore_parms();

    errno = 0;

    if(b == 1.0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
        return;
    }
    else if(b == 2.0)
    {
        log_result = log2(x);
    }
    else if(b == 10.0)
    {
        log_result = log10(x);
    }
    else
    {
        const F64 log2_x = log2(x);
        const F64 log2_b = log2(b);

        log_result = log2_x / log2_b;
    }

    ev_data_set_real(p_ev_data_res, log_result);

    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_LOG);
}

/******************************************************************************
*
* NUMBER ceiling(n {, multiple})
*
* NUMBER floor(n {, multiple})
*
* NUMBER mround(n, multiple)
*
* NUMBER round(n {, decimals})
*
* NUMBER rounddown(n, decimals)
*
* NUMBER roundup(n, decimals)
*
* NUMBER trunc(n {, decimals})
*
******************************************************************************/

extern void
round_common(
    P_EV_DATA args[EV_MAX_ARGS],
    _InVal_     S32 n_args,
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InVal_     U32 rpn_did_num)
{
    BOOL negate_result = FALSE;
    F64 f64, multiplier;

    switch(rpn_did_num)
    {
    case RPN_FNV_CEILING:
    case RPN_FNV_FLOOR:
        {
        f64 = args[0]->arg.fp;

        if(n_args <= 1)
        {
            multiplier = 1.0;
        }
        else
        {
            F64 multiple =  args[1]->arg.fp;

            if(multiple < 0.0) /* Handle the Excel way of doing these */
            {
                multiple = fabs(multiple);

                if(f64 > 0.0)
                {
                    ev_data_set_error(p_ev_data_res, EVAL_ERR_MIXED_SIGNS);
                    return;
                }

                /* rounds towards (or away from) zero, so operate on positive number */
                if(f64 < 0.0)
                {
                    f64 = -f64; /* both are now positive */
                    negate_result = TRUE;
                }
            }

            /* check for divide by zero about to trap */
            if(multiple < F64_MIN)
            {
                ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
                return;
            }

            multiplier = 1.0 / multiple; 
        }

        break;
        }

    case RPN_FNF_MROUND:
        {
        F64 multiple = fabs(args[1]->arg.fp); /* Unlike Excel, we allow mixed signs for MROUND */

        f64 = args[0]->arg.fp;

        /* rounds towards (or away from) zero, so operate on positive number */
        if(f64 < 0.0)
        {
            f64 = -f64;
            negate_result = TRUE;
        }

        /* check for divide by zero about to trap */
        if(multiple < F64_MIN)
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
            return;
        }

        multiplier = 1.0 / multiple; 
        break;
        }

    default: default_unhandled();
#if CHECKING
    case RPN_FNV_ROUND:
    case RPN_FNF_ROUNDDOWN:
    case RPN_FNF_ROUNDUP:
    case RPN_FNV_TRUNC:
#endif
        {
        S32 decimal_places = 2;

        if(n_args > 1)
            decimal_places = MIN(15, args[1]->arg.integer);
        else if(RPN_FNV_TRUNC == rpn_did_num)
            decimal_places = 0;

        switch(args[0]->did_num)
        {
        case RPN_DAT_BOOL8:
        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            if(decimal_places >= 0)
            {   /* if we have an integer number to be rounded at, or to the right of, the decimal, it's already there */
                /* NOP */
                *p_ev_data_res = *(args[0]);
                return;
            }

            f64 = (F64) args[0]->arg.integer; /* have to do it like this */
            break;

        default:
            f64 = args[0]->arg.fp;
            break;
        }

        /* these ones all round towards (or away from) zero, so operate on positive number */
        if(f64 < 0.0)
        {
            f64 = -f64;
            negate_result = TRUE;
        }

        multiplier = pow(10.0, (F64) decimal_places);
        break;
        }
    }

    f64 = f64 * multiplier;

    switch(rpn_did_num)
    {
    case RPN_FNF_MROUND:
    case RPN_FNV_ROUND:
        f64 = floor(f64 + 0.5); /* got a positive number here */
        break;

    default: default_unhandled();
#if CHECKING
    case RPN_FNV_FLOOR:
    case RPN_FNF_ROUNDDOWN:
    case RPN_FNV_TRUNC:
#endif
        f64 = floor(f64);
        break;

    case RPN_FNV_CEILING:
    case RPN_FNF_ROUNDUP:
        f64 = ceil(f64);
        break;
    }

    f64 = f64 / multiplier;

    ev_data_set_real(p_ev_data_res, negate_result ? -f64 : f64);
}

PROC_EXEC_PROTO(c_ceiling)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ev_data_res, RPN_FNV_CEILING);

    consume_bool(real_to_integer_try(p_ev_data_res));
}

PROC_EXEC_PROTO(c_floor)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ev_data_res, RPN_FNV_FLOOR);

    consume_bool(real_to_integer_try(p_ev_data_res));
}

PROC_EXEC_PROTO(c_mround)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ev_data_res, RPN_FNF_MROUND);

    consume_bool(real_to_integer_try(p_ev_data_res));
}

PROC_EXEC_PROTO(c_round)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ev_data_res, RPN_FNV_ROUND);

    consume_bool(real_to_integer_try(p_ev_data_res));
}

PROC_EXEC_PROTO(c_rounddown)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ev_data_res, RPN_FNF_ROUNDDOWN);

    consume_bool(real_to_integer_try(p_ev_data_res));
}

PROC_EXEC_PROTO(c_roundup)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ev_data_res, RPN_FNF_ROUNDUP);

    consume_bool(real_to_integer_try(p_ev_data_res));
}

PROC_EXEC_PROTO(c_trunc)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ev_data_res, RPN_FNV_TRUNC);

    consume_bool(real_to_integer_try(p_ev_data_res));
}

/******************************************************************************
*
* NUMBER quotient(numerator:number, denominator:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_quotient)
{
    exec_func_ignore_parms();

    if(!two_nums_divide_try(p_ev_data_res, args[0], args[1], FALSE))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_CALC_FAILURE);
        return;
    }

    if(RPN_DAT_REAL == p_ev_data_res->did_num)
    {
        F64 quotient_result;

        /* lose the fractional part */
        (void) modf(p_ev_data_res->arg.fp, &quotient_result);

        ev_data_set_real_ti(p_ev_data_res, quotient_result);
    }
}

/******************************************************************************
*
* NUMBER seriessum(x:number, n:number, m:number, coefficients:array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_seriessum)
{
    const F64 x = args[0]->arg.fp;
    const F64 n = args[1]->arg.fp;
    const F64 m = args[2]->arg.fp;
    const PC_EV_DATA array_coefficients = args[3];
    S32 x_size, y_size;
    S32 ix, iy;
    F64 seriessum_result = 0.0;
    F64 power = n;
    F64 x_to_power = pow(x, power);

    exec_func_ignore_parms();

    /* get x and y sizes */
    array_range_sizes(array_coefficients, &x_size, &y_size);

    for(ix = 0; ix < x_size; ++ix)
    {
        for(iy = 0; iy < y_size; ++iy)
        {
            F64 term;
            EV_DATA ev_data_coefficient;

            (void) array_range_index(&ev_data_coefficient, array_coefficients, ix, iy, EM_REA);

            term = ev_data_coefficient.arg.fp * x_to_power;

            seriessum_result += term;

            /* advance */
            power += m;
            if(m == 1.0)
                x_to_power *= x;
            else if(m == 2.0)
                x_to_power *= (x * x);
            else
                x_to_power = pow(x, power);
        }
    }

    ev_data_set_real(p_ev_data_res, seriessum_result);
}

/******************************************************************************
*
* NUMBER sumproduct(array_1, array_2, ...)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sumproduct)
{
    F64 sum = 0.0;
    S32 x_size, y_size;
    S32 i;

    exec_func_ignore_parms();

    /* ensure all arrays are the same size */
    array_range_sizes(args[0], &x_size, &y_size);

    for(i = 1; i < n_args; ++i)
    {
        const PC_EV_DATA array_i = args[i];
        S32 x_size_i, y_size_i;
        array_range_sizes(array_i, &x_size_i, &y_size_i);
        if((x_size != x_size_i) || (y_size != y_size_i))
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_VALUE);
            return;
        }
    }

    { /* for each element in all arrays, calculate the product */
    S32 ix, iy;

    for(iy = 0; iy < y_size; ++iy)
    {
        for(ix = 0; ix < x_size; ++ix)
        {
            F64 product = 1.0;

            for(i = 0; i < n_args; ++i)
            {
                const PC_EV_DATA array_i = args[i];
                EV_DATA ev_data;

                (void) array_range_index(&ev_data, array_i, ix, iy, EM_REA);

                product *= ev_data.arg.fp;
            }

            /* all products of {arrays}[i][j] are added to the total */
            sum += product;
        }
    }
    } /*block*/

    ev_data_set_real_ti(p_ev_data_res, sum);
}

/******************************************************************************
*
* NUMBER sum_x2my2(array_x, array_y)
*
* NUMBER sum_x2py2(array_x, array_y)
*
******************************************************************************/

static void
sumx2opy2_common(
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InRef_     PC_EV_DATA array_x,
    _InRef_     PC_EV_DATA array_y,
    _InVal_     BOOL subtract_y2)
{
    STATUS status = STATUS_OK;
    S32 x_size[2];
    S32 y_size[2];

    array_range_sizes(array_x, &x_size[0], &y_size[0]);
    array_range_sizes(array_y, &x_size[1], &y_size[1]);

    if((x_size[0] != x_size[1]) || (y_size[0] != y_size[1]))
        status = EVAL_ERR_ODF_NA;
    else if((0 == x_size[0]) || (0 == y_size[0]))
        status = EVAL_ERR_NO_VALID_DATA;
    else
    {
        F64 sum = 0.0;
        S32 ix, iy;

        for(iy = 0; iy < y_size[0]; ++iy)
        {
            for(ix = 0; ix < x_size[0]; ++ix)
            {
                EV_DATA ev_data_x, ev_data_y;
                F64 x2, y2;

                (void) array_range_index(&ev_data_x, array_x, ix, iy, EM_REA);
                (void) array_range_index(&ev_data_y, array_y, ix, iy, EM_REA);

                x2 = ev_data_x.arg.fp*ev_data_x.arg.fp;
                y2 = ev_data_y.arg.fp*ev_data_y.arg.fp;

                if(subtract_y2)
                    y2 = -y2;

                sum += (x2 + y2);
            }
        }

        ev_data_set_real_ti(p_ev_data_res, sum);
    }

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
}

PROC_EXEC_PROTO(c_sum_x2my2)
{
    const PC_EV_DATA array_x = args[0];
    const PC_EV_DATA array_y = args[1];

    exec_func_ignore_parms();

    sumx2opy2_common(p_ev_data_res, array_x, array_y, TRUE); /* sum(x^2 - y^2) */
}

PROC_EXEC_PROTO(c_sum_x2py2)
{
    const PC_EV_DATA array_x = args[0];
    const PC_EV_DATA array_y = args[1];

    exec_func_ignore_parms();

    sumx2opy2_common(p_ev_data_res, array_x, array_y, TRUE); /* sum(x^2 + y^2) */
}

/******************************************************************************
*
* NUMBER sum_xmy2(array_x, array_y)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sum_xmy2)
{
    STATUS status = STATUS_OK;
    const PC_EV_DATA array_x = args[0];
    const PC_EV_DATA array_y = args[1];
    S32 x_size[2];
    S32 y_size[2];

    exec_func_ignore_parms();

    array_range_sizes(array_x, &x_size[0], &y_size[0]);
    array_range_sizes(array_y, &x_size[1], &y_size[1]);

    if((x_size[0] != x_size[1]) || (y_size[0] != y_size[1]))
        status = EVAL_ERR_ODF_NA;
    else if((0 == x_size[0]) || (0 == y_size[0]))
        status = EVAL_ERR_NO_VALID_DATA;
    else
    {
        F64 sum = 0.0;
        S32 ix, iy;

        for(iy = 0; iy < y_size[0]; ++iy)
        {
            for(ix = 0; ix < x_size[0]; ++ix)
            {
                EV_DATA ev_data_x, ev_data_y;
                F64 difference;

                (void) array_range_index(&ev_data_x, array_x, ix, iy, EM_REA);
                (void) array_range_index(&ev_data_y, array_y, ix, iy, EM_REA);

                difference = ev_data_x.arg.fp - ev_data_y.arg.fp;

                sum += (difference*difference);
            }
        }

        ev_data_set_real_ti(p_ev_data_res, sum);
    }

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
}

/* end of ev_matb.c */
