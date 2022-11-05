/* mathxcpx.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Complex number functions */

/* Split off from ev_mcpx.c December 2019 */

#ifndef __mathxcpx_h
#define __mathxcpx_h

/*
declare complex number type for internal usage
*/

#if (__STDC_VERSION__ < 199901L) || defined(__STDC_NO_COMPLEX__) || defined(USE_OWN_COMPLEX_IMPL) || 0

#if !defined(USE_OWN_COMPLEX_IMPL)
#define USE_OWN_COMPLEX_IMPL 1 /* for subsequent testing */
#endif

#endif

#if !defined(USE_OWN_COMPLEX_IMPL)

#include <complex.h>

typedef
#if defined(_MSC_VER)
    _Dcomplex /* still not C99! */
#else
    double _Complex
#endif
COMPLEX, * P_COMPLEX; typedef const COMPLEX * PC_COMPLEX;

#if defined(CMPLX)
#define COMPLEX_INIT(real_value, imag_value) CMPLX(real_value, imag_value)
#elif defined(_MSC_VER)
#define COMPLEX_INIT(real_value, imag_value) { (real_value), (imag_value) } /* yuk */
#else
#define COMPLEX_INIT(real_value, imag_value) ((real_value) + I*(imag_value))
#endif

#define complex_get(z) \
    (*(z))

#define complex_set(z, complex_value) \
    (*(z)) = (complex_value)

_Check_return_
static inline double
complex_real(
    _InRef_ PC_COMPLEX z)
{
    return(creal(*z));
}

_Check_return_
static inline double
complex_imag(
    _InRef_ PC_COMPLEX z)
{
    return(cimag(*z));
}

static inline void
complex_set_ri(
    _OutRef_ P_COMPLEX z,
    _InVal_ double real_value,
    _InVal_ double imag_value)
{
#if defined(CMPLX)
    complex_set(z, CMPLX(real_value, imag_value));
#elif defined(_MSC_VER)
    complex_set(z, _DCOMPLEX_(real_value, imag_value));
#else
    complex_set(z, real_value + I*imag_value);
#endif
}

#define inline_if_native_complex inline

#else /* defined(USE_OWN_COMPLEX_IMPL) */

typedef struct COMPLEX
{
    double r, i;
}
COMPLEX, * P_COMPLEX; typedef const COMPLEX * PC_COMPLEX;

#define COMPLEX_INIT(real_value, imag_value) { (real_value), (imag_value) }

_Check_return_
static inline double
complex_real(
    _InRef_ PC_COMPLEX z)
{
    return(z->r);
}

_Check_return_
static inline double
complex_imag(
    _InRef_ PC_COMPLEX z)
{
    return(z->i);
}

/* NB this must not be a macro! (so we can use output as input) */
static inline void
complex_set_ri(
    _OutRef_    P_COMPLEX z,
    _InVal_     double real_value,
    _InVal_     double imag_value)
{
    z->r = real_value;
    z->i = imag_value;
}

#define inline_if_native_complex /*nothing*/

#endif/* defined(USE_OWN_COMPLEX_IMPL) */

/*
exported data
*/

extern const COMPLEX
complex_zero;

extern const COMPLEX
complex_Re_one;

extern const COMPLEX
complex_Re_one_half;

/*
exported functions
*/

/* z = x + iy = r.e^i@ */

_Check_return_
extern double
complex_modulus( /* |z| */
    _InRef_     PC_COMPLEX z);

_Check_return_
extern double
complex_argument( /* arg(z) */
    _InRef_     PC_COMPLEX z);

/* basic complex arithmetic */

extern void
complex_add( /* z1 + z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2,
    _OutRef_    P_COMPLEX z_out);

extern void
complex_subtract( /* z1 - z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2,
    _OutRef_    P_COMPLEX z_out);

extern void
complex_multiply( /* z1 * z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_divide( /* z1 / z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2,
    _OutRef_    P_COMPLEX z_out);

#define complex_reciprocal(z, z_out) \
    complex_divide(&complex_Re_one, z, z_out)

extern void
complex_conjugate( /* conj(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

/* complex exponent, logarithms, powers */

extern void
complex_exp( /* e^z */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_log_e( /* log_e(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_log_2( /* log_2(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_log_10( /* log_10(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_power( /* z1^z2 */
    _InRef_     PC_COMPLEX z1,
    _InRef_     PC_COMPLEX z2,
    _OutRef_    P_COMPLEX z_out);

extern void
complex_square_root( /* square root : sqrt(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

/* complex trig functions */

extern void
complex_cosine( /* cosine : cos(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

extern void
complex_sine( /* sine : sin(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_tangent( /* tangent : tan(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_secant( /* secant : sec(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_cosecant( /* cosecant : csc(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_cotangent( /* cotangent : cot(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

/* complex inverse trig functions */

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_arc_cosine( /* arc cosine : arccos(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_arc_sine( /* arc sine : arcsin(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_arc_tangent( /* arc tangent : arctan(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_arc_secant( /* arc secant : arcsec(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_arc_cosecant( /* arc cosecant : arccsc(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_arc_cotangent( /* arc cotangent : arccot(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

/* complex hyperbolic functions */

extern void
complex_hyperbolic_cosine( /* hyperbolic cosine : cosh(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

extern void
complex_hyperbolic_sine( /* hyperbolic sine : sinh(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_hyperbolic_tangent( /* hyperbolic tangent : tanh(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_hyperbolic_secant( /* hyperbolic secant : sech(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_hyperbolic_cosecant( /* hyperbolic cosecantt : csch(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_hyperbolic_cotangent( /* hyperbolic cotangent : coth(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

/* complex inverse hyperbolic functions */

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_inverse_hyperbolic_cosine( /* inverse hyperbolic cosine : acosh(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_inverse_hyperbolic_sine( /* inverse hyperbolic sine : asinh(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_inverse_hyperbolic_tangent( /* inverse hyperbolic tangent : atanh(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_inverse_hyperbolic_secant( /* inverse hyperbolic secant : asech(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_inverse_hyperbolic_cosecant( /* inverse hyperbolic cosecant : acsch(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

_Check_return_ _Success_(status_ok(return))
extern STATUS
complex_inverse_hyperbolic_cotangent( /* inverse hyperbolic cotangent : acoth(z) */
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX z_out);

#endif /* __mathxcpx_h */

/* end of mathxcpx.h  */
