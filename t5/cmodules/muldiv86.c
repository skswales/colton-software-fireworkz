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
* result = (dividend * multiplier) / divisor with 64-bit intermediate calculation
*
* notes:
*   1   Will give INT 0 trap if divisor == 0
*   2   divisor must be +ve
*
*/

/* rounds towards -inf, remainder always +ve */

_Check_return_
extern S32
muldiv64(
    _InVal_     S32 dividend,
    _InVal_     S32 multiplier,
    _InVal_     S32 divisor)
{
    int64_t product_s64;
    int64_t quotient_s64;
    S32 quotient_s32;

    muldiv64_statics.remainder = 0;
    muldiv64_statics.overflow = 0;

#if 1
    product_s64 = __emul(dividend, multiplier); /* __int64 mul of two 32-bit values wasteful */
#else
    product_s64 = (S64) dividend * multiplier;
#endif

    /* like idiv_floor */
    if(product_s64 >= 0)
        quotient_s64 = product_s64                   / divisor;
    else
        quotient_s64 = (product_s64 - (divisor - 1)) / divisor;

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
#if 1
        muldiv64_statics.remainder = (S32) (product_s64 - __emul(quotient_s32, divisor)); /* always +ve */
#else
        muldiv64_statics.remainder = (S32) (product_s64 - ((S64) quotient_s32 * divisor)); /* always +ve */
#endif
        assert(muldiv64_statics.remainder >= 0);
    }

    return(quotient_s32);
}

/*
  round result towards +inf as ceil()

  result =
    (sgn(a*b*divisor) -ve)
      ?  (a*b)                / divisor
      : ((a*b) + (divisor-1)) / divisor
*/

_Check_return_
extern S32
muldiv64_ceil(
    _InVal_     S32 dividend,
    _InVal_     S32 multiplier,
    _InVal_     S32 divisor)
{
    S32 res = muldiv64(dividend, multiplier, divisor);

    if(muldiv64_statics.remainder != 0)
    {
        const U32 sign_bit = (dividend ^ multiplier ^ divisor) & 0x80000000U;
        assert(0 == muldiv64_statics.overflow);
        assert(muldiv64_statics.remainder > 0);
        muldiv64_statics.remainder = 0;
        if(sign_bit)
        {   /*EMPTY*/
        }
        else
        {
            assert(res != S32_MAX);
            res += 1;
        }
    }

    return(res);
}

/*
  round result towards -inf as floor()

  result =
    (sgn(a*b*divisor) +ve)
      ?  (a*b)                / divisor
      : ((a*b) - (divisor-1)) / divisor
*/

_Check_return_
extern S32
muldiv64_floor(
    _InVal_     S32 dividend,
    _InVal_     S32 multiplier,
    _InVal_     S32 divisor)
{
    S32 res = muldiv64(dividend, multiplier, divisor);

    if(muldiv64_statics.remainder != 0)
    {
        const U32 sign_bit = (dividend ^ multiplier ^ divisor) & 0x80000000U;
        assert(0 == muldiv64_statics.overflow);
        assert(muldiv64_statics.remainder > 0);
        muldiv64_statics.remainder = 0;
        if(sign_bit)
        {
            assert(res != S32_MIN);
            res -= 1;
        }
        else
        {   /*EMPTY*/
        }
    }

    return(res);
}

/*
  round result towards -inf

  result =
    (sgn(a*b*divisor) +ve)
      ? ((a*b) + divisor/2) / divisor
      : ((a*b) - divisor/2) / divisor
*/

_Check_return_
extern S32
muldiv64_round_floor(
    _InVal_     S32 dividend,
    _InVal_     S32 multiplier,
    _InVal_     S32 divisor)
{
    S32 res = muldiv64(dividend, multiplier, divisor);

    if(muldiv64_statics.remainder != 0)
    {
        const U32 sign_bit = (dividend ^ multiplier ^ divisor) & 0x80000000U;
        const S32 threshold = divisor / 2;
        assert(0 == muldiv64_statics.overflow);
        assert(muldiv64_statics.remainder > 0);
        if(muldiv64_statics.remainder >= threshold)
        {
            if(sign_bit)
            {
                assert(res != S32_MIN);
                res -= 1;
            }
            else
            {
                assert(res != S32_MAX);
                res += 1;
            }
        }
        muldiv64_statics.remainder = 0;
    }

    return(res);
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
