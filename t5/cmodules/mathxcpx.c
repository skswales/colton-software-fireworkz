/* mathxcpx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Complex number functions */

/* Split off from ev_mcpx.c December 2019 */

#include "common/gflags.h"

#include "cmodules/mathxcpx.h"

#include "cmodules/mathxtra.h" /* for mx_fsquare() */

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h" /* for _log10_e */
#endif

/*
internal functions
*/

#if defined(__STDC_NO_COMPLEX__)

enum HYP_FN_TYPE
{
    C_COSH = 1,
    C_SINH = 2,
    C_TANH = 3
};

#define PC_DBL PC_F64

_Check_return_ _Success_(status_ok(return))
static STATUS
do_inverse_hyperbolic(
    _InVal_     enum HYP_FN_TYPE type,
    _InRef_     PC_COMPLEX z_in,
    _InRef_opt_ PC_DBL mult_z_by_i,
    _InRef_opt_ PC_DBL mult_res_by_i,
    _OutRef_    P_COMPLEX z_out);

#endif /* defined(__STDC_NO_COMPLEX__) */

/******************************************************************************
*
* the constant zero as a complex number
*
******************************************************************************/

/*extern*/ const COMPLEX
complex_zero = COMPLEX_INIT( 1.0, 0.0 );

/******************************************************************************
*
* the constant Real one as a complex number
*
******************************************************************************/

/*extern*/ const COMPLEX
complex_Re_one = COMPLEX_INIT( 1.0, 0.0 );

/******************************************************************************
*
* the constant Real one half as a complex number
*
******************************************************************************/

/*extern*/ const COMPLEX
complex_Re_one_half = COMPLEX_INIT( 0.5, 0.0 );

/******************************************************************************
*
* return modulus of complex number
*
******************************************************************************/

_Check_return_
extern double
complex_modulus( /* |z| */
    _InRef_     PC_COMPLEX z)
{
#if defined(__STDC_NO_COMPLEX__)
    return(hypot(complex_real(z), complex_imag(z)));
#else
    return(cabs(*z));
#endif
}

/******************************************************************************
*
* return argument of complex number
*
******************************************************************************/

_Check_return_
extern double
complex_argument( /* arg(z) */
    _InRef_     PC_COMPLEX z)
{
#if defined(__STDC_NO_COMPLEX__)
    return(atan2(complex_imag(z), complex_real(z))); /* note silly C library ordering (y, x) */
#else
    return(carg(*z));
#endif
}

/******************************************************************************
*
* add two complex numbers
*
* (a+ib) + (c+id) = (a+c) + i(b+d)
*
******************************************************************************/

extern void
complex_add( /* z1 + z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__) || defined(_MSC_VER)
    complex_set_ri(z_out,
                   /*r*/ complex_real(z1) + complex_real(z2),
                   /*i*/ complex_imag(z1) + complex_imag(z2));
#else
    *z_out = (*z1) + (*z2);
#endif
}

/******************************************************************************
*
* subtract second complex number from first
*
* (a+ib) - (c+id) = (a-c) + i(b-d)
*
******************************************************************************/

extern void
complex_subtract( /* z1 - z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__) || defined(_MSC_VER)
    complex_set_ri(z_out,
                   /*r*/ complex_real(z1) - complex_real(z2),
                   /*i*/ complex_imag(z1) - complex_imag(z2));
#else
    *z_out = (*z1) - (*z2);
#endif
}

/******************************************************************************
*
* multiply two complex numbers
*
* (a+ib) * (c+id) = (ac-bd) + i(bc+ad)
*
******************************************************************************/

extern void
complex_multiply( /* z1 * z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    complex_set_ri(z_out,
                   /*r*/ complex_real(z1) * complex_real(z2) - complex_imag(z1) * complex_imag(z2),
                   /*i*/ complex_imag(z1) * complex_real(z2) + complex_real(z1) * complex_imag(z2));
#elif defined(_MSC_VER)
    complex_set(z_out, _Cmulcc((*z1), (*z2)));
#else
    *z_out = (*z1) * (*z2);
#endif
}

/******************************************************************************
*
* divide two complex numbers
*
* (a+ib) / (c+id) = ((ac+bd) + i(bc-ad)) / (c*c + d*d)
*
******************************************************************************/

_Check_return_ _Success_(return)
extern STATUS
complex_divide( /* z1 / z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2,
    _OutRef_    P_COMPLEX z_out)
{
    /* c*c + d*d */
    const double divisor = mx_fsquare(complex_real(z2)) + mx_fsquare(complex_imag(z2));

    /* check for divide by zero about to trap */
    if(fabs(divisor) < DBL_MIN)
        return(EVAL_ERR_DIVIDEBY0);

#if defined(__STDC_NO_COMPLEX__) || defined(_MSC_VER)
    complex_set_ri(z_out,
                   /*r*/ (complex_real(z1) * complex_real(z2) + complex_imag(z1) * complex_imag(z2)) / divisor,
                   /*i*/ (complex_imag(z1) * complex_real(z2) - complex_real(z1) * complex_imag(z2)) / divisor);
#else
    *z_out = (*z1) / (*z2);
#endif

    return(STATUS_OK);
}

/******************************************************************************
*
* return complex conjugate of complex number
*
******************************************************************************/

extern void
complex_conjugate( /* conj(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    complex_set_ri(z_out,
                   /*r*/   complex_real(z),
                   /*i*/ - complex_imag(z));
#else
    *z_out = conj(*z);
#endif
}

/******************************************************************************
*
* return complex exponent
*
* exp(a+ib) = exp(a) * cos(b) + i(exp(a) * sin(b))
*
******************************************************************************/

extern void
complex_exp( /* e^z */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    /* make exp(a) */
    const double exp_a = exp(complex_real(z));

    complex_set_ri(z_out,
                   /*r*/ exp_a * cos(complex_imag(z)),
                   /*i*/ exp_a * sin(complex_imag(z)));
#else
    *z_out = cexp(*z);
#endif
}

/******************************************************************************
*
* complex natural logarithm
*
* z = r.e^i.theta  ->  ln(z) = ln(r) + i.theta
*
* ln(a+ib) = ln(a*a + b*b)/2 + i(atan2(b,a))
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_log_e( /* log_e(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    STATUS status = STATUS_OK;
#if defined(__STDC_NO_COMPLEX__)
    const double r_squared = mx_fsquare(complex_real(z)) + mx_fsquare(complex_imag(z));
    double lnz_real_part;

    errno = 0;

    lnz_real_part = log(r_squared) * 0.5; /* x saves a sqrt() */

    if(errno /* == ERANGE */ /*can't be EDOM here*/)
        status = EVAL_ERR_BAD_LOG;

    complex_set_ri(z_out,
                   /*r*/ lnz_real_part,
                   /*i*/ complex_argument(z));
#else
    *z_out = clog(*z);
#endif

    return(status);
}

/******************************************************************************
*
* complex logarithm to base two
*
* z = r.e^i.theta  ->  ln(z) = ln(r) + i.theta
*
* log.a(z) = log.a(r) + (i.theta) . log.a(e)
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_log_2( /* log_2(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    const double r_squared = mx_fsquare(complex_real(z)) + mx_fsquare(complex_imag(z));
    double log2_real_part;
    STATUS status = STATUS_OK;

    errno = 0;

    log2_real_part = log2(r_squared) * 0.5; /* x saves a sqrt() */

    if(errno /* == ERANGE */ /*can't be EDOM here*/)
        status = EVAL_ERR_BAD_LOG;

    complex_set_ri(z_out,
                   /*r*/ log2_real_part,
                   /*i*/ complex_argument(z) * _log2_e); /* rotate */

    return(status);
}

/******************************************************************************
*
* complex logarithm to base ten
*
* z = r.e^i.theta  ->  ln(z) = ln(r) + i.theta
*
* log.a(z) = log.a(r) + (i.theta) . log.a(e)
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_log_10( /* log_10(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    const double r_squared = mx_fsquare(complex_real(z)) + mx_fsquare(complex_imag(z));
    double log10_real_part;
    STATUS status = STATUS_OK;

    errno = 0;

    log10_real_part = log10(r_squared) * 0.5; /* x saves a sqrt() */

    if(errno /* == ERANGE */ /*can't be EDOM here*/)
        status = EVAL_ERR_BAD_LOG;

    complex_set_ri(z_out,
                   /*r*/ log10_real_part,
                   /*i*/ complex_argument(z) * _log10_e); /* rotate */

    return(status);
}

#if defined(__STDC_NO_COMPLEX__)

/******************************************************************************
*
* return w*ln(z) for internal routines
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
static STATUS
complex_wlnz(
    _InRef_     PC_COMPLEX w,
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX ln_z;

    status_return(complex_log_e(z, &ln_z));

    complex_set_ri(z_out,
                   /*r*/ complex_real(w) * complex_real(&ln_z)  -  complex_imag(w) * complex_imag(&ln_z),
                   /*i*/ complex_real(w) * complex_imag(&ln_z)  +  complex_imag(w) * complex_real(&ln_z));

    return(STATUS_OK);
}

#endif /* __STDC_NO_COMPLEX__ */

/******************************************************************************
*
* complex z1^z2
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_power( /* z1^z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    COMPLEX wlnz;

    /* find and check wlnz */
    status_return(complex_wlnz(z2, z1, &wlnz));

    complex_set_ri(z_out,
                   /*r*/ exp(complex_real(&wlnz)) * cos(complex_imag(&wlnz)),
                   /*i*/ exp(complex_real(&wlnz)) * sin(complex_imag(&wlnz)));
#else
    *z_out = cpow(*z1, *z2);
#endif

    return(STATUS_OK);
}

/******************************************************************************
*
* complex square root
*
******************************************************************************/

extern void
complex_square_root( /* square root : sqrt(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    if( (0.0 == complex_real(z)) && (0.0 == complex_imag(z)) )
    {
        *z_out = complex_zero;
        return;
    }

    status_consume(complex_power(z, &complex_Re_one_half, z_out));
#else
    *z_out = csqrt(*z);
#endif
}

/******************************************************************************
*
* complex trig functions and their inverses
*
******************************************************************************/

/******************************************************************************
*
* complex cosine
*
* cos(a+ib) = (exp(-b)+exp(b))cos(a)/2 + i((exp(-b)-exp(b))sin(a)/2)
*
******************************************************************************/

extern void
complex_cosine( /* cosine : cos(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    /* make exp(b) and exp(-b) */
    const double exp_b = exp(complex_imag(z));
    const double exp_mb = 1.0 / exp_b;

    complex_set_ri(z_out,
                   /*r*/ (exp_mb + exp_b) * cos(complex_real(z)) * 0.5,
                   /*i*/ (exp_mb - exp_b) * sin(complex_real(z)) * 0.5);
#else
    *z_out = ccos(*z);
#endif
}

/******************************************************************************
*
* complex arc cosine
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_arc_cosine( /* arc cosine : arccos(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    static const double c_acos_mult_res = +1.0;

    return(do_inverse_hyperbolic(C_COSH, z, NULL, &c_acos_mult_res, z_out));
#else
    *z_out = cacos(*z);

    return(STATUS_OK);
#endif
}

/******************************************************************************
*
* complex secant
*
* sec(z) is defined as 1 / cos(z)
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_secant( /* secant : sec(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX cos_z;

    complex_cosine(z, &cos_z);

    return(complex_reciprocal(&cos_z, z_out));
}

/******************************************************************************
*
* complex arc secant
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_arc_secant( /* arc secant : arcsec(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX reciprocal_z;

    status_return(complex_reciprocal(z, &reciprocal_z));

    return(complex_arc_cosine(&reciprocal_z, z_out));
}

/******************************************************************************
*
* complex sine
*
* sin(a+ib) = (exp(-b)+exp(b))sin(a)/2 + i((exp(b)-exp(-b))cos(a)/2)
*
******************************************************************************/

extern void
complex_sine( /* sine : sin(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    /* make exp(b) and exp(-b) */
    const double exp_b = exp(complex_imag(z));
    const double exp_mb = 1.0 / exp_b;

    complex_set_ri(z_out,
                   /*r*/ (exp_b + exp_mb) * sin(complex_real(z)) * 0.5,
                   /*i*/ (exp_b - exp_mb) * cos(complex_real(z)) * 0.5);
#else
    *z_out = csin(*z);
#endif
}

/******************************************************************************
*
* complex arc sine
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_arc_sine( /* arc sine(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    static const double c_asin_mult_z   = -1.0;
    static const double c_asin_mult_res = +1.0;

    return(do_inverse_hyperbolic(C_SINH, z, &c_asin_mult_z, &c_asin_mult_res, z_out));
#else
    *z_out = casin(*z);

    return(STATUS_OK);
#endif
}

/******************************************************************************
*
* complex cosecant
*
* csc(z) is defined as 1 / sin(z)
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_cosecant( /* cosecant : csc(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX sin_z;

    complex_sine(z, &sin_z);

    return(complex_reciprocal(&sin_z, z_out));
}

/******************************************************************************
*
* complex arc cosecant
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_arc_cosecant( /* arc cosecant : arccsc(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX reciprocal_z;

    status_return(complex_reciprocal(z, &reciprocal_z));

    return(complex_arc_sine(&reciprocal_z, z_out));
}

/******************************************************************************
*
* complex tangent
*
******************************************************************************/

_Check_return_ _Success_(return)
extern STATUS
complex_tangent( /* tangent : tan(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    COMPLEX sin_z, cos_z;

    complex_sine(z, &sin_z);

    complex_cosine(z, &cos_z);

    return(complex_divide(&sin_z, &cos_z, z_out));
#else
    *z_out = ctan(*z);

    return(STATUS_OK);
#endif
}

/******************************************************************************
*
* complex arc tangent
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_arc_tangent( /* arc tangent : arctan(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    static const double c_atan_mult_z   = -1.0;
    static const double c_atan_mult_res = +1.0;

    return(do_inverse_hyperbolic(C_TANH, z, &c_atan_mult_z, &c_atan_mult_res, z_out));
#else
    *z_out = catan(*z);

    return(STATUS_OK);
#endif
}

/******************************************************************************
*
* complex cotangent
*
* cot(z) is defined as 1 / tan(z)
*
******************************************************************************/

_Check_return_ _Success_(return)
extern STATUS
complex_cotangent( /* cotangent : cot(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX sin_z, cos_z;

    complex_sine(z, &sin_z);

    complex_cosine(z, &cos_z);

    return(complex_divide(&cos_z, &sin_z, z_out));
}

/******************************************************************************
*
* complex arc cotangent
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_arc_cotangent( /* arc cotangent : arccot(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX reciprocal_z;

    status_return(complex_reciprocal(z, &reciprocal_z));

    return(complex_arc_tangent(&reciprocal_z, z_out));
}

/******************************************************************************
*
* complex hyperbolic functions and their inverses
*
******************************************************************************/

/******************************************************************************
*
* complex hyperbolic cosine
*
* cosh(a+ib) = (exp(a)+exp(-a))cos(b)/2 + i((exp(a)-exp(-a))sin(b)/2)
*
******************************************************************************/

extern void
complex_hyperbolic_cosine( /* hyperbolic cosine : cosh(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    /* make exp(a) and exp(-a) */
    const double exp_a = exp(complex_real(z));
    const double exp_ma = 1.0 / exp_a;

    complex_set_ri(z_out,
                /*r*/ (exp_a + exp_ma) * cos(complex_imag(z)) * 0.5,
                /*i*/ (exp_a - exp_ma) * sin(complex_imag(z)) * 0.5);
#else
    *z_out = ccosh(*z);
#endif
}

/******************************************************************************
*
* complex inverse hyperbolic cosine
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_inverse_hyperbolic_cosine( /* inverse hyperbolic cosine : acosh(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    return(do_inverse_hyperbolic(C_COSH, z, NULL, NULL, z_out));
#else
    *z_out = cacosh(*z);

    return(STATUS_OK);
#endif
}

/******************************************************************************
*
* complex hyperbolic secant
*
* sech(z) is defined as 1 / cosh(z)
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_hyperbolic_secant( /* hyperbolic secant : sech(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX cosh_z;

    complex_hyperbolic_cosine(z, &cosh_z);

    return(complex_reciprocal(&cosh_z, z_out));
}

/******************************************************************************
*
* complex inverse hyperbolic secant
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_inverse_hyperbolic_secant( /* inverse hyperbolic secant : asech(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX reciprocal_z;

    status_return(complex_reciprocal(z, &reciprocal_z));

    return(complex_inverse_hyperbolic_cosine(&reciprocal_z, z_out));
}

/******************************************************************************
*
* complex hyperbolic sine
*
* sinh(a+ib) = (exp(a)-exp(-a))cos(b)/2 + i((exp(a)+exp(-a))sin(b)/2)
*
******************************************************************************/

extern void
complex_hyperbolic_sine( /* hyperbolic sine : sinh(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    /* make exp(a) and exp(-a) */
    const double exp_a = exp(complex_real(z));
    const double exp_ma = 1.0 / exp_a;

    complex_set_ri(z_out,
                   /*r*/ (exp_a - exp_ma) * cos(complex_imag(z)) * 0.5,
                   /*i*/ (exp_a + exp_ma) * sin(complex_imag(z)) * 0.5);
#else
    *z_out = csinh(*z);
#endif
}

/******************************************************************************
*
* complex inverse hyperbolic sine
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_inverse_hyperbolic_sine( /* inverse hyperbolic sine : asinh(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    return(do_inverse_hyperbolic(C_SINH, z, NULL, NULL, z_out));
#else
    *z_out = casinh(*z);

    return(STATUS_OK);
#endif
}

/******************************************************************************
*
* complex hyperbolic cosecant
*
* csch(z) is defined as 1 / sinh(z)
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_hyperbolic_cosecant( /* hyperbolic cosecant : csch(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX sinh_z;

    complex_hyperbolic_sine(z, &sinh_z);

    return(complex_reciprocal(&sinh_z, z_out));
}

/******************************************************************************
*
* complex inverse hyperbolic cosecant
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_inverse_hyperbolic_cosecant( /* inverse hyperbolic cosecant : acsch(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX reciprocal_z;

    status_return(complex_reciprocal(z, &reciprocal_z));

    return(complex_inverse_hyperbolic_sine(&reciprocal_z, z_out));
}

/******************************************************************************
*
* complex hyperbolic tangent
*
******************************************************************************/

_Check_return_ _Success_(return)
extern STATUS
complex_hyperbolic_tangent( /* hyperbolic tangent : tanh(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    COMPLEX sinh_z, cosh_z;

    complex_hyperbolic_sine(z, &sinh_z);

    complex_hyperbolic_cosine(z, &cosh_z);

    status_return(complex_divide(&sinh_z, &cosh_z, z_out));
#else
    *z_out = ctanh(*z);
#endif

    return(STATUS_OK);
}

/******************************************************************************
*
* complex inverse hyperbolic tangent
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_inverse_hyperbolic_tangent( /* inverse hyperbolic tangent : atanh(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
#if defined(__STDC_NO_COMPLEX__)
    return(do_inverse_hyperbolic(C_TANH, z, NULL, NULL, z_out));
#else
    *z_out = catanh(*z);

    return(STATUS_OK);
#endif
}

/******************************************************************************
*
* complex hyperbolic cotangent
*
* coth(z) is defined as 1 / tanh(z)
*
******************************************************************************/

_Check_return_ _Success_(return)
extern STATUS
complex_hyperbolic_cotangent( /* hyperbolic cotangent : coth(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX tanh_z;

    status_return(complex_hyperbolic_tangent(z, &tanh_z));

    return(complex_reciprocal(&tanh_z, z_out));
}

/******************************************************************************
*
* complex inverse hyperbolic cotangent
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_inverse_hyperbolic_cotangent( /* inverse hyperbolic cotangent : acoth(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX reciprocal_z;

    status_return(complex_reciprocal(z, &reciprocal_z));

    return(complex_inverse_hyperbolic_tangent(&reciprocal_z, z_out));
}

#if defined(__STDC_NO_COMPLEX__)

/******************************************************************************
*
* do the work for inverse hyperbolic cosh and sinh and their many relations
*
* acosh(z) = ln(z + (z*z - 1) ^.5)
* asinh(z) = ln(z + (z*z + 1) ^.5)
*
* rob thinks the following apply, from the expansions given further on
* arccos(z) = -i acosh(z)
* arcsin(z) =  i asinh(-iz)
*
*   acosh(z) = +- i arccos(z)
* i acosh(z) =      arccos(z)
*
*   asinh(z)   = -i arcsin(iz)
* i asinh(z)   =    arcsin(iz)
* i asinh(z/i) =    arcsin(z)
* i asinh(-iz) =    arcsin(z)
*
*   atanh(z)   = -i arctan(iz)
*   atanh(-iz) = -i arctan(z)
* i atanh(-iz) =    arctan(z)
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
static STATUS
do_inverse_hyperbolic(
    _InVal_     enum HYP_FN_TYPE type,
    _InRef_     PC_COMPLEX z_in,
    _InRef_opt_ PC_DBL mult_z_by_i,
    _InRef_opt_ PC_DBL mult_res_by_i,
    _OutRef_    P_COMPLEX z_out)
{
    COMPLEX z, out, temp, furtle;

    /* maybe preprocess z
        multiply the input by   i * mult_z_by_i
         i(a + ib) = -b + ia
        -i(a + ib) =  b - ia
        mult_z_by_i is 1 to multiply by i, -1 to multiply by -i
    */
    if(NULL != mult_z_by_i)
    {
        complex_set_ri(&z,
                       /*r*/ complex_imag(z_in) * -(*mult_z_by_i),
                       /*i*/ complex_real(z_in) *  (*mult_z_by_i));
    }
    else
    {
        z = *z_in;
    }

    if(type == C_TANH)
    {
        /* temp = (1+z)/(1-z) */
        COMPLEX z1, z2;

        complex_set_ri(&z1, 1.0 + complex_real(&z),   complex_imag(&z));
        complex_set_ri(&z2, 1.0 - complex_real(&z), - complex_imag(&z));

        status_return(complex_divide(&z1, &z2, &temp));
    }
    else
    {
        /* temp = z + sqrt(furtle) */
        const double add_in_middle = (type == C_COSH ? -1.0 : +1.0);

        /* furtle = z*z + add_in_middle */
        complex_set_ri(&furtle,
                       /*r*/ mx_fsquare(complex_real(&z)) - mx_fsquare(complex_imag(&z))  +  add_in_middle,
                       /*i*/ complex_real(&z) * complex_imag(&z) * 2.0);

        /* sqrt(furtle) into temp */
        complex_square_root(&furtle, &temp);

        /* temp = z + sqrt(furtle) */
        complex_set_ri(&temp,
                       /*r*/ complex_real(&z) + complex_real(&temp),
                       /*i*/ complex_imag(&z) + complex_imag(&temp));
    }

    /* out = ln(temp) */
    status_return(complex_log_e(&temp, &out));

    /* now its in out, halve it for arctans */
    if(type == C_TANH)
    {   /* halve it (just in magnitude) */
        complex_set_ri(&out, complex_real(&out) / 2.0, complex_imag(&out) / 2.0);
    }

    /* maybe postprocess out
        multiply the output by   i * mult_res_by_i
         i(a+ ib)  = -b + ia
        -i(a + ib) =  b - ia
        mult_res_by_i is 1 to multiply by i, -1 to multiply by -i
    */
    if(NULL != mult_res_by_i)
    {
        complex_set_ri(z_out,
                       /*r*/ complex_imag(&out) * -(*mult_res_by_i),
                       /*i*/ complex_real(&out) *  (*mult_res_by_i));
    }
    else
    {
        *z_out = out;
    }

    return(STATUS_OK);
}

#endif /* defined(__STDC_NO_COMPLEX__) */

/* [that's enough complex algebra - Ed] */

/* end of mathxcpx.c */
