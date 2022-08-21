/* muldiv86.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#if defined(_WINDOWS)

#pragma warning(push)
#pragma warning(disable: 4255) /* '__getcallerseflags' : no function prototype given: converting '()' to '(void)' */
#pragma warning(disable: 4668) /* '_M_IA64' is not defined as a preprocessor macro */
#include <intrin.h>
#pragma warning(pop)

#pragma intrinsic(__emul)

static
struct MULDIV64_STATICS
{
    S32 remainder;
    S32 overflow;
}
muldiv64_statics;

_Check_return_
extern S32
muldiv64_overflow(void)
{
    return(muldiv64_statics.overflow);
}

_Check_return_
extern S32
muldiv64_remainder(void)
{
    return(muldiv64_statics.remainder);
}

/*
* result = dividend * numerator / denominator with 64-bit intermediate calculation
*
* notes:
*   1   Will give INT 0 trap if denominator == 0
*   2   denominator must be +ve
*
*/

/* rounds towards -inf, remainder always +ve */

_Check_return_
extern S32
muldiv64(
    _InRef_     S32 dividend,
    _InVal_     S32 numerator,
    _InVal_     S32 denominator)
{
    __int64 product_s64;
    __int64 quotient_s64;
    S32 quotient_s32;

    muldiv64_statics.remainder = 0;
    muldiv64_statics.overflow = 0;

#if 1
    product_s64 = __emul(dividend, numerator); /* __int64 mul of two 32-bit values wasteful */
#else
    product_s64 = (S64) dividend * numerator;
#endif

    /* like idiv_floor */
    quotient_s64 = ((product_s64 >= 0) ? product_s64 : (product_s64 - denominator + 1)) / denominator;

    if(quotient_s64 < S32_MIN)
    {
        quotient_s32 = S32_MIN;
        muldiv64_statics.overflow = -1;
    }
    else if(quotient_s64 > S32_MAX)
    {
        quotient_s32 = S32_MAX;
        muldiv64_statics.overflow = 1;
    }
    else
    {
        quotient_s32 = (S32) quotient_s64;
        muldiv64_statics.remainder = (S32) (product_s64 - __emul(quotient_s32, denominator)); /* always +ve */
        assert(muldiv64_statics.remainder >= 0);
    }

    return(quotient_s32);
}

/* round result towards +inf */

_Check_return_
extern S32
muldiv64_ceil(
    _InRef_     S32 dividend,
    _InVal_     S32 numerator,
    _InVal_     S32 denominator)
{
    S32 res = muldiv64(dividend, numerator, denominator);

    if(muldiv64_statics.remainder != 0)
    {
        assert(muldiv64_statics.remainder > 0);
        assert(0 == muldiv64_statics.overflow);
        muldiv64_statics.remainder = 0;
        assert(res != S32_MAX);
        res += 1;
    }

    return(res);
}

/* round result towards -inf */

_Check_return_
extern S32
muldiv64_floor(
    _InRef_     S32 dividend,
    _InVal_     S32 numerator,
    _InVal_     S32 denominator)
{
    S32 res = muldiv64(dividend, numerator, denominator);

    if(muldiv64_statics.remainder != 0)
    {
        assert(muldiv64_statics.remainder > 0);
        assert(0 == muldiv64_statics.overflow);
        muldiv64_statics.remainder = 0;
    }

    return(res);
}

_Check_return_
extern S32
muldiv64_round_floor(
    _InRef_     S32 dividend,
    _InVal_     S32 numerator,
    _InVal_     S32 denominator)
{
    S32 res = muldiv64(dividend, numerator, denominator);

    if(muldiv64_statics.remainder != 0)
    {
        const S32 threshold = denominator / 2;
        assert(muldiv64_statics.remainder > 0);
        assert(0 == muldiv64_statics.overflow);
        if(muldiv64_statics.remainder >= threshold)
        {
            assert(res != S32_MAX);
            res += 1;
        }
        muldiv64_statics.remainder = 0;
    }

    return(res);
}

extern void
umul64(
    _InVal_     U32 a,
    _InVal_     U32 b,
    _OutRef_    P_UMUL64_RESULT result)
{
    U64 u64 = (U64) a * b;
    result->LowPart  = (U32) ( u64        & 0xFFFFFFFFU);
    result->HighPart = (U32) ((u64 >> 32) & 0xFFFFFFFFU);
}

#else

/* keep compilers happy */

extern void
muldiv86(void);

extern void
muldiv86(void)
{
    return;
}

#endif /*WINDOWS*/

/* end of muldiv86.c */
