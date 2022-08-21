/* mathxtra.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS July 1991 */

#ifndef __mathxtra_h
#define __mathxtra_h

/*
exported functions
*/

#define PRAGMA_SIDE_EFFECTS_OFF
#include "coltsoft/pragma.h"
/* note that ANSI errno is volatile to enable this sort of CSE optimization */

_Check_return_
extern F64
mx_acosh(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_acosec(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_acosech(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_acot(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_acoth(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_asec(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_asech(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_asinh(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_atanh(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_cosec(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_cosech(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_cot(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_coth(
    _InVal_     F64 x);

/* return the square of a number (or more likely, hard expression) */

_Check_return_
static inline F64
mx_fsquare(
    _InVal_     F64 x)
{
    return(x * x);
}

_Check_return_
extern F64
mx_fhypot(
    _InVal_     F64 x,
    _InVal_     F64 y);

_Check_return_
extern F64
mx_sec(
    _InVal_     F64 x);

_Check_return_
extern F64
mx_sech(
    _InVal_     F64 x);

#define PRAGMA_SIDE_EFFECTS
#include "coltsoft/pragma.h"

#endif /* __mathxtra_h */

/* end of mathxtra.h  */
