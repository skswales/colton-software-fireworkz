/* ev_fneng.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2019 Stuart Swales */

/* Engineering function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "cmodules/mathxtra.h"

/******************************************************************************
*
* Engineering functions
*
******************************************************************************/

/******************************************************************************
*
* NUMBER besseli(x:number, n:number)
*
******************************************************************************/

/* Polynomial Approximations - See Abramowitz & Stegun Ch. 9 */

_Check_return_
static F64
bessel_i0(
    _In_        F64 x)
{
    F64 result;

    /* even-order functions are even functions: I(n,x) = I(n,-x) */
    x = fabs(x);

    if(x <= 3.75)
    {   /* A&S 9.8.1 */
        const F64 t = x / 3.75;
        const F64 t_term = t * t;

        /* Coefficients for approximation to I0 on [-3.75,+3.75] */
        static const F64 coeff[] =
        {
            +1.0000000, /*t^0*/
            +3.5156229, /*t^2*/
            +3.0899424, /*t^4*/
            +1.2067492, /*t^6*/
            +0.2659732, /*t^8*/
            +0.0360768, /*t^10*/
            +0.0045813  /*t^12*/
        };  /* |eps| < 1.6e-7 */

        F64 sum =       coeff[0]
            + t_term * (coeff[2/2]
            + t_term * (coeff[4/2]
            + t_term * (coeff[6/2]
            + t_term * (coeff[8/2]
            + t_term * (coeff[10/2]
            + t_term * (coeff[12/2]) )))));

        result = sum;
    }
    else
    {   /* A&S 9.8.2 */
        const F64 denominator = sqrt(x) * exp(-x);
        const F64 t = x / 3.75;
        const F64 t_term = 1.0 / t;

        /* Coefficients for approximation to I0 on [+3.75,+inf) */
        static const F64 coeff[] =
        {
            +0.39894228, /*t^ 0*/
            +0.01328592, /*t^-1*/
            +0.00225319, /*t^-2*/
            -0.00157565, /*t^-3*/ /*-*/
            +0.00916281, /*t^-4*/
            -0.02057706, /*t^-5*/ /*-*/
            +0.02635537, /*t^-6*/
            -0.01647633, /*t^-7*/ /*-*/
            +0.00392377  /*t^-8*/
        };  /* |eps| < 1.9e-7 */

        F64 sum =       coeff[0]
            + t_term * (coeff[1]
            + t_term * (coeff[2]
            + t_term * (coeff[3]
            + t_term * (coeff[4]
            + t_term * (coeff[5]
            + t_term * (coeff[6]
            + t_term * (coeff[7]
            + t_term * (coeff[8]) )))))));

        result = sum / denominator;
    }

    return(result);
}

_Check_return_
static F64
bessel_i1(
    _In_        F64 x)
{
    F64 result;
    BOOL negate_result = FALSE;

    /* odd-order functions are odd functions: -I(n,x) = I(n,-x) */
    if(x < 0)
    {
        x = fabs(x);
        negate_result = TRUE;
    }

    if(x <= 3.75)
    {   /* A&S 9.8.3 */
        const F64 numerator = x;
        const F64 t = x / 3.75;
        const F64 t_term = t * t;

        /* Coefficients for approximation to I1 on [-3.75,+3.75] */
        static const F64 coeff[] =
        {
            +0.50000000, /*t^0*/
            +0.87890594, /*t^2*/
            +0.51498869, /*t^4*/
            +0.15084934, /*t^6*/
            +0.02658733, /*t^8*/
            +0.00301532, /*t^10*/
            +0.00032411  /*t^12*/
        };  /* |eps| < 8e-9 */

        F64 sum =       coeff[0]
            + t_term * (coeff[2/2]
            + t_term * (coeff[4/2]
            + t_term * (coeff[6/2]
            + t_term * (coeff[8/2]
            + t_term * (coeff[10/2]
            + t_term * (coeff[12/2]) )))));

        result = sum * numerator;
    }
    else
    {   /* A&S 9.8.4 */
        const F64 denominator = sqrt(x) * exp(-x);
        const F64 t = x / 3.75;
        const F64 t_term = 1.0 / t;

        /* Coefficients for approximation to I1 on [+3.75,+inf) */
        static const F64 coeff[] =
        {
            +0.39894228, /*t^0*/
            -0.03988024, /*t^-1*/ /*-*/
            -0.00362018, /*t^-2*/ /*-*/
            +0.00163801, /*t^-3*/
            -0.01031555, /*t^-4*/ /*-*/
            +0.02282967, /*t^-5*/
            -0.02895312, /*t^-6*/ /*-*/
            +0.01787654, /*t^-7*/
            -0.00420059  /*t^-8*/ /*-*/
        };  /* |eps| < 2.2e-7 */

        F64 sum =       coeff[0]
            + t_term * (coeff[1]
            + t_term * (coeff[2]
            + t_term * (coeff[3]
            + t_term * (coeff[4]
            + t_term * (coeff[5]
            + t_term * (coeff[6]
            + t_term * (coeff[7]
            + t_term * (coeff[8]) )))))));

        result = sum / denominator;
    }

    if(negate_result)
        result = -result;

    return(result);
}

_Check_return_
static F64
bessel_in(
    _In_        S32 n,
    _InVal_     F64 x)
{
    F64 n_minus_1_term, n_minus_2_term;
    S32 n_minus_1;

    if(0 == n)
        return(bessel_i0(x));

    /* A&S 9.6.6 I(-n,z) = I(n,z) */
    n = abs(n);

    if(1 == n)
        return(bessel_i1(x));

/*
 * Recurrence relation - See Abramowitz & Stegun Ch. 9
 *
 * I(n+1,x) = -2n/x * I(n,x) + I(n-1,x)
 *
 * rewrite here as:
 * I(n,x) = -2(n_minus_1)/x * I(n_minus_1,x) + I(n_minus_1-1,x)
 *
 */

    n_minus_1 = (n - 1);
    n_minus_1_term = ((-2.0 * n_minus_1) / x) * bessel_in(n_minus_1, x);
    n_minus_2_term =                            bessel_in(n_minus_1 - 1, x);

    return(n_minus_1_term + n_minus_2_term);
}

PROC_EXEC_PROTO(c_besseli)
{
    const F64 x = args[0]->arg.fp;
    const F64 n = floor(args[1]->arg.fp);
    F64 besseli_result;

    exec_func_ignore_parms();

    besseli_result = bessel_in((S32) n, x);

    ev_data_set_real(p_ev_data_res, besseli_result);
}

/******************************************************************************
*
* NUMBER besselj(x:number, n:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_besselj)
{
    const F64 x = args[0]->arg.fp;
    const F64 n = floor(args[1]->arg.fp);
    F64 besselj_result;

    exec_func_ignore_parms();

    besselj_result = bessel_jn((S32) n, x);

    ev_data_set_real(p_ev_data_res, besselj_result);
}

/******************************************************************************
*
* NUMBER besselk(x:number, n:number)
*
******************************************************************************/

static const F64 zero = 0.0;
static const F64 one  = 1.0;

__pragma(warning(push))
__pragma(warning(disable:4723)) /* potential divide by 0 */

/* Polynomial Approximations - See Abramowitz & Stegun Ch. 9 */

_Check_return_
static F64
bessel_k0(
    _InVal_     F64 x)
{
    F64 result;

    if(x <= 0.0)
    {   /* +inf or +nan */
        result = ((x == 0.0) ? one : zero) / zero; /*C4273*/
    }
    else if(x <= 2.0)
    {   /* A&S 9.8.5 */
        const F64 t = x * 0.5;
        const F64 t_term = t * t;
        const F64 ln_term = -(log(t) * bessel_i0(x));

        /* Coefficients for approximation to K0 on (0,+2] */
        static const F64 coeff[] =
        {
            -0.57721566, /*t^0*/ /*-*/
            +0.42278420, /*t^2*/
            +0.23069756, /*t^4*/
            +0.03488590, /*t^6*/
            +0.00262698, /*t^8*/
            +0.00010750, /*t^10*/
            +0.00000740  /*t^12*/
        };  /* |eps| < 1e-8 */

        F64 sum =       ln_term
            +           coeff[0]
            + t_term * (coeff[2/2]
            + t_term * (coeff[4/2]
            + t_term * (coeff[6/2]
            + t_term * (coeff[8/2]
            + t_term * (coeff[10/2]
            + t_term * (coeff[12/2]) )))));

        result = sum;
    }
    else
    {   /* A&S 9.8.7 */
        const F64 denominator = sqrt(x) * exp(x);
      /*const F64 t = x * 0.5;*/
        const F64 t_term = 2.0 / x; /* i.e. (1.0 / t); */

        /* Coefficients for approximation to K0 on [+2,+inf) */
        static const F64 coeff[] =
        {
            +1.25331414, /*t^0*/
            -0.07832358, /*t^1*/ /*-*/
            +0.02189568, /*t^2*/
            -0.01062446, /*t^3*/ /*-*/
            +0.00587872, /*t^4*/
            -0.00251540, /*t^5*/ /*-*/
            +0.00053208  /*t^6*/
        };  /* |eps| < 1.9e-7 */

        F64 sum =       coeff[0]
            + t_term * (coeff[1]
            + t_term * (coeff[2]
            + t_term * (coeff[3]
            + t_term * (coeff[4]
            + t_term * (coeff[5]
            + t_term * (coeff[6]) )))));

        result = sum / denominator;
    }

    return(result);
}

_Check_return_
static F64
bessel_k1(
    _InVal_     F64 x)
{
    F64 result;

    if(x <= 0.0)
    {   /* +inf or +nan */
        result = ((x == 0.0) ? one : zero) / zero; /*C4273*/
    }
    else if(x <= 2.0)
    {   /* A&S 9.8.7 */
        const F64 denominator = x;
        const F64 t = x * 0.5;
        const F64 t_term = t * t;
        const F64 ln_term = x*(log(t) * bessel_i1(x));

        /* Coefficients for approximation to K1 on (0,+2] */
        static const F64 coeff[] =
        {
            +1.00000000, /*t^0*/
            +0.15443144, /*t^2*/
            -0.67278579, /*t^4*/ /*-*/
            -0.18156897, /*t^6*/ /*-*/
            -0.01919402, /*t^8*/ /*-*/
            -0.00110404, /*t^10*/ /*-*/
            -0.00004686  /*t^12*/ /*-*/
        };  /* |eps| < 8e-9 */

        F64 sum =       ln_term
            +           coeff[0]
            + t_term * (coeff[2/2]
            + t_term * (coeff[4/2]
            + t_term * (coeff[6/2]
            + t_term * (coeff[8/2]
            + t_term * (coeff[10/2]
            + t_term * (coeff[12/2]) )))));

        result = sum / denominator;
    }
    else
    {   /* A&S 9.8.8 */
        const F64 denominator = sqrt(x) * exp(x);
      /*const F64 t = x * 0.5;*/
        const F64 t_term = 2.0 / x; /* i.e. (1.0 / t); */

        /* Coefficients for approximation to K0 on [+2,+inf) */
        static const F64 coeff[] =
        {
            +1.25331414, /*t^0*/
            +0.23498619, /*t^1*/
            -0.03655620, /*t^2*/ /*-*/
            +0.01504268, /*t^3*/
            -0.00780353, /*t^4*/ /*-*/
            +0.00325614, /*t^5*/
            -0.00068245  /*t^6*/ /*-*/
        };  /* |eps| < 2.2e-7 */

        F64 sum =       coeff[0]
            + t_term * (coeff[1]
            + t_term * (coeff[2]
            + t_term * (coeff[3]
            + t_term * (coeff[4]
            + t_term * (coeff[5]
            + t_term * (coeff[6]) )))));

        result = sum / denominator;
    }

    return(result);
}

__pragma(warning(pop))

_Check_return_
static F64
bessel_kn(
    _In_        S32 n,
    _InVal_     F64 x)
{
    F64 n_minus_1_term, n_minus_2_term;
    S32 n_minus_1;

    if(0 == n)
        return(bessel_k0(x));

    /* A&S 9.6.6 K(-n,z) = K(n,z) */
    n = abs(n);

    if(1 == n)
        return(bessel_k1(x));

/*
 * Recurrence relation - See Abramowitz & Stegun Ch. 9
 *
 * K(n+1,x) = 2n/x * K(n,x) + K(n-1,x)
 *
 * rewrite here as:
 * K(n,x) = 2(n_minus_1)/x * K(n_minus_1,x) + K(n_minus_1-1,x)
 *
 */

    n_minus_1 = (n - 1);
    n_minus_1_term = ((+2.0 * n_minus_1) / x) * bessel_kn(n_minus_1, x);
    n_minus_2_term =                            bessel_kn(n_minus_1 - 1, x);

    return(n_minus_1_term + n_minus_2_term);
}

PROC_EXEC_PROTO(c_besselk)
{
    const F64 x = args[0]->arg.fp;
    const F64 n = floor(args[1]->arg.fp);
    F64 besselk_result;

    exec_func_ignore_parms();

    besselk_result = bessel_kn((S32) n, x);

    ev_data_set_real(p_ev_data_res, besselk_result);
}

/******************************************************************************
*
* NUMBER bessely(x:number, n:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_bessely)
{
    const F64 x = args[0]->arg.fp;
    const F64 n = floor(args[1]->arg.fp);
    F64 bessely_result;

    exec_func_ignore_parms();

    bessely_result = bessel_yn((S32) n, x);

    ev_data_set_real(p_ev_data_res, bessely_result);
}

/******************************************************************************
*
* NUMBER delta(number_1:number {, number_2:number=0})
*
******************************************************************************/

PROC_EXEC_PROTO(c_delta)
{
    const F64 number_1 = args[0]->arg.fp;
    const F64 number_2 = (n_args > 1) ? args[1]->arg.fp : 0.0;
    S32 delta_result;

    exec_func_ignore_parms();

    delta_result = (number_1 == number_2);

    ev_data_set_integer(p_ev_data_res, delta_result);
}

/******************************************************************************
*
* REAL erf(lower_limit:number {, upper_limit:number})
*
******************************************************************************/

PROC_EXEC_PROTO(c_erf)
{
    const F64 lower_limit = args[0]->arg.fp;
    F64 lower_erf;
    F64 erf_result;

    exec_func_ignore_parms();

    lower_erf = erf(lower_limit);
    erf_result = lower_erf;

    if(n_args > 1)
    {
        const F64 upper_limit = args[1]->arg.fp;
        F64 upper_erf;

        upper_erf = erf(upper_limit);
        erf_result = upper_erf - lower_erf;
    }

    ev_data_set_real(p_ev_data_res, erf_result);
}

/******************************************************************************
*
* REAL erfc(lower_limit:number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_erfc)
{
    const F64 lower_limit = args[0]->arg.fp;
    F64 lower_erf;
    F64 erfc_result;

    exec_func_ignore_parms();

    lower_erf = erfc(lower_limit);
    erfc_result = lower_erf;

    ev_data_set_real(p_ev_data_res, erfc_result);
}

/******************************************************************************
*
* NUMBER gestep(number {, step:number=0})
*
******************************************************************************/

PROC_EXEC_PROTO(c_gestep)
{
    const F64 number = args[0]->arg.fp;
    const F64 step = (n_args > 1) ? args[1]->arg.fp : 0.0;
    S32 gestep_result;

    exec_func_ignore_parms();

    gestep_result = (number >= step);

    ev_data_set_integer(p_ev_data_res, gestep_result);
}

/* end of ev_fneng.c */
