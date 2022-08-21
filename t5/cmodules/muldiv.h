/* muldiv.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS July 1991 */

#ifndef __muldiv_h
#define __muldiv_h

#if defined(__cplusplus)
extern "C" {
#endif

/*
exported structure
*/

typedef struct UMUL64_RESULT
{
    U32 LowPart;
    U32 HighPart;
}
UMUL64_RESULT, * P_UMUL64_RESULT;

typedef struct MYRAND_SEED
{
    U32 seed_word;
    U32 seed_bit; /* bottom bit is used */
}
MYRAND_SEED, * P_MYRAND_SEED;

/*
exported functions
*/

extern void
muldiv64_init(void);

/* careful 32-bit * 32-bit / 32-bit a la BCPL */

_Check_return_
extern S32
muldiv64(
    _InRef_     S32 a,
    _InRef_     S32 b,
    _InRef_     S32 c);

_Check_return_
extern S32
muldiv64_ceil(
    _InRef_     S32 a,
    _InRef_     S32 b,
    _InRef_     S32 c);

_Check_return_
extern S32
muldiv64_floor(
    _InRef_     S32 a,
    _InRef_     S32 b,
    _InRef_     S32 c);

_Check_return_
extern S32
muldiv64_round_floor(
    _InRef_     S32 a,
    _InRef_     S32 b,
    _InRef_     S32 c);

/* ditto, but limit against +/-S32_MAX on overflows */

_Check_return_
extern S32
muldiv64_limiting(
    _InRef_     S32 a,
    _InRef_     S32 b,
    _InRef_     S32 c);

/* the overflow from a prior muldiv64() */

_Check_return_
extern S32
muldiv64_overflow(void);

/* the remainder from a prior muldiv64() */

_Check_return_
extern S32
muldiv64_remainder(void);

extern void
umul64(
    _InVal_     U32 a,
    _InVal_     U32 b,
    _OutRef_    P_UMUL64_RESULT result);

_Check_return_
extern U32
myrand(
    _InoutRef_  P_MYRAND_SEED p_myrand_seed,
    _InVal_     U32 n /*exclusive*/);

#if defined(__cplusplus)
}
#endif

#endif /* __muldiv_h */

/* end of muldiv.h */
