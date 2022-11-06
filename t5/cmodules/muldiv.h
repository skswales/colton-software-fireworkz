/* muldiv.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

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

typedef struct MULDIV64_REMOVF
{
    int32_t remainder;
    int32_t overflow;
}
MULDIV64_REMOVF, * P_MULDIV64_REMOVF;

/*
exported functions
*/

/* careful (32-bit * 32-bit) / 32-bit a la BCPL */

_Check_return_
extern int32_t
muldiv64(
    _InVal_     int32_t dividend,
    _InVal_     int32_t multiplier,
    _InVal_     int32_t divisor);

_Check_return_
extern int32_t
muldiv64_removf(
    _InVal_     int32_t dividend,
    _InVal_     int32_t multiplier,
    _InVal_     int32_t divisor,
    _OutRef_    P_MULDIV64_REMOVF p_muldiv64_removf);

_Check_return_
extern int32_t
muldiv64_ceil(
    _InVal_     int32_t dividend,
    _InVal_     int32_t multiplier,
    _InVal_     int32_t divisor);

_Check_return_
extern int32_t
muldiv64_floor(
    _InVal_     int32_t dividend,
    _InVal_     int32_t multiplier,
    _InVal_     int32_t divisor);

_Check_return_
extern int32_t
muldiv64_round_floor(
    _InVal_     int32_t dividend,
    _InVal_     int32_t multiplier,
    _InVal_     int32_t divisor);

#if RISCOS
extern void
muldiv64_init(void);
#endif

#if !defined(PORTABLE_MULDIV64)

/* the overflow from a prior muldiv64() */

_Check_return_
extern S32
muldiv64_overflow(void);

/* the remainder from a prior muldiv64() */

_Check_return_
extern S32
muldiv64_remainder(void);

#endif /* PORTABLE_MULDIV64 */

#if defined(__cplusplus)
}
#endif

#endif /* __muldiv_h */

/* end of muldiv.h */
