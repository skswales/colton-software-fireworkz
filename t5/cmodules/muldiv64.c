/* muldiv64.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#pragma warning(push)
#pragma warning(disable: 4255) /* '__getcallerseflags' : no function prototype given: converting '()' to '(void)' */
#pragma warning(disable: 4668) /* '_M_IA64' is not defined as a preprocessor macro */
#include <intrin.h>
#pragma warning(pop)
#pragma intrinsic(__emul)
#endif /* _MSC_VER */

/*
* result := (dividend * multiplier) / divisor
* with 64-bit intermediate calculation
*
* notes:
*   1   Will give INT 0 trap if divisor == 0
*   2   Divisor must be +ve
*   3   Rounds towards -inf (smoother tranitions through zero for display)
*   4   Remainder is always +ve (unlike C99)
*/

#define PRAGMA_CHECK_STACK_OFF
#include "coltsoft/pragma.h"

_Check_return_
extern int32_t
muldiv64(
    _InVal_     int32_t dividend,
    _InVal_     int32_t multiplier,
    _InVal_     int32_t divisor)
{
    const int64_t divisor_i64 = divisor; // better for ARM
    int64_t numerator_i64;
    int64_t quotient_i64;
    const int64_t product_i64 =
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
        __emul(dividend, multiplier); /* __int64 mul of two 32-bit values wasteful */
#else
        (int64_t) multiplier * dividend; // better order for ARM
#endif

    /* like idiv_floor */
    numerator_i64 = product_i64;
    if(int64_is_negative(numerator_i64))
    {
#if 1 /* for Fireworkz, numerator is 99% positive so happy to make the odd function call */
        const int32_t divisor_m1 = divisor - 1;
#else
        const int64_t divisor_m1 = divisor - 1; /* can't avoid Norcroft C 'lower precision in wider context' warning except by mucking up the code produced */
#endif
        numerator_i64 -= divisor_m1;
    }

    quotient_i64 = numerator_i64 / divisor_i64;

    if(false == int64_would_overflow_int32(quotient_i64))
        return((int32_t) quotient_i64);

    return(int64_is_negative(quotient_i64) ? INT32_MIN : INT32_MAX);
}

#if 1

_Check_return_
static inline int32_t /*+ve*/
fixup_remainder(
    _InVal_     int32_t remainder_i32 /*-ve*/,
    _InVal_     int32_t divisor)
{
    if(divisor > 0)
        return(divisor + remainder_i32);

    return(remainder_i32 - divisor);
}

#endif

_Check_return_
extern int32_t
muldiv64_removf(
    _InVal_     int32_t dividend,
    _InVal_     int32_t multiplier,
    _InVal_     int32_t divisor,
    _OutRef_    P_MULDIV64_REMOVF p_muldiv64_removf)
{
    const int64_t divisor_i64 = divisor; // better for ARM
    int64_t numerator_i64;
    int64_t quotient_i64;
    int64_t remainder_i64;
    const int64_t product_i64 =
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
        __emul(dividend, multiplier); /* __int64 mul of two 32-bit values wasteful */
#else
        (int64_t) multiplier * dividend; // better order for ARM
#endif

    /* like idiv_floor */
    numerator_i64 = product_i64;
    if(int64_is_negative(numerator_i64))
    {
#if 1 /* for Fireworkz, numerator is 99% positive so happy to make the odd function call */
        const int32_t divisor_m1 = divisor - 1;
#else
        const int64_t divisor_m1 = divisor - 1; /* can't avoid Norcroft C 'lower precision in wider context' warning except by mucking up the code produced */
#endif
        numerator_i64 -= divisor_m1;
    }

    quotient_i64  = numerator_i64 / divisor_i64;
    remainder_i64 = numerator_i64 % divisor_i64; /* C99 remainder has sign of dividend */

    p_muldiv64_removf->remainder = (int32_t) remainder_i64; /* can't overflow! */
    p_muldiv64_removf->overflow = 0;

    /* ensure that remainder from muldiv64() is always +ve */
    if(p_muldiv64_removf->remainder < 0)
        p_muldiv64_removf->remainder = fixup_remainder(p_muldiv64_removf->remainder, divisor);

    if(false == int64_would_overflow_int32(quotient_i64))
    {
        const int32_t quotient_i32 = (int32_t) quotient_i64;
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
        p_muldiv64_removf->remainder = (int32_t) (product_i64 - __emul(quotient_i32, divisor)); /* always +ve */
#else
        //p_muldiv64_removf->remainder = (int32_t) (product_i64 - (quotient_i32 * (int64_t) divisor)); /* always +ve */
#endif
        assert(p_muldiv64_removf->remainder >= 0);
        return(quotient_i32);
    }

    p_muldiv64_removf->overflow = int64_is_negative(quotient_i64) ? -1 : 1;
    return(int64_is_negative(quotient_i64) ? INT32_MIN : INT32_MAX);
}

#define PRAGMA_CHECK_STACK_ON
#include "coltsoft/pragma.h"

/*
* round result towards +inf as ceil()
*
* result :=
*   (sgn(a*b*divisor) -ve)
*       ?  (a*b)                / divisor
*       : ((a*b) + (divisor-1)) / divisor
*/

_Check_return_
extern int32_t
muldiv64_ceil(
    _InVal_     int32_t dividend,
    _InVal_     int32_t multiplier,
    _InVal_     int32_t divisor)
{
    MULDIV64_REMOVF removf;
    int32_t res = muldiv64_removf(dividend, multiplier, divisor, &removf);

    if(removf.remainder == 0)
        return(res);

    {
    const uint32_t sign_bit = (dividend ^ multiplier ^ divisor) & 0x80000000U;
    assert(0 == removf.overflow);
    assert(removf.remainder > 0);
    //removf.remainder = 0;
    if(sign_bit)
    {   /*EMPTY*/
    }
    else
    {
        assert(res != INT32_MAX);
        res += 1;
    }

    return(res);
    } /*block*/
}

/*
* round result towards -inf as floor()
*
* result :=
*   (sgn(a*b*divisor) +ve)
*       ?  (a*b)                / divisor
*       : ((a*b) - (divisor-1)) / divisor
*/

_Check_return_
extern int32_t
muldiv64_floor(
    _InVal_     int32_t dividend,
    _InVal_     int32_t multiplier,
    _InVal_     int32_t divisor)
{
    MULDIV64_REMOVF removf;
    int32_t res = muldiv64_removf(dividend, multiplier, divisor, &removf);

    if(removf.remainder == 0)
        return(res);

    {
    const uint32_t sign_bit = (dividend ^ multiplier ^ divisor) & 0x80000000U;
    assert(0 == removf.overflow);
    assert(removf.remainder > 0);
    //removf.remainder = 0;
    if(sign_bit)
    {
        assert(res != INT32_MIN);
        res -= 1;
    }
    else
    {   /*EMPTY*/
    }

    return(res);
    } /*block*/
}

/*
* round result towards -inf
*
* result :=
*   (sgn(a*b*divisor) +ve)
*       ? ((a*b) + divisor/2) / divisor
*       : ((a*b) - divisor/2) / divisor
*/

_Check_return_
extern int32_t
muldiv64_round_floor(
    _InVal_     int32_t dividend,
    _InVal_     int32_t multiplier,
    _InVal_     int32_t divisor)
{
    MULDIV64_REMOVF removf;
    int32_t res = muldiv64_removf(dividend, multiplier, divisor, &removf);

    if(removf.remainder == 0)
        return(res);

    {
    const uint32_t sign_bit = (dividend ^ multiplier ^ divisor) & 0x80000000U;
    const int32_t threshold = divisor >> 1; // Pedants would say / 2 was needed for divisor of -1, but that wouldn't give remainder
    assert(0 == removf.overflow);
    assert(removf.remainder > 0);
    if(removf.remainder >= threshold)
    {
        if(sign_bit)
        {
            assert(res != INT32_MIN);
            res -= 1;
        }
        else
        {
            assert(res != INT32_MAX);
            res += 1;
        }
    }
    //removf.remainder = 0;

    return(res);
    } /*block*/
}

extern void
muldiv64_init(void)
{
    /* nothing to do for this implementation */
}

/* end of muldiv64.c */
