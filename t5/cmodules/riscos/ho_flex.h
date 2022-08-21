/* riscos/ho_flex.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header for flex API */

/* SKS 26-Aug-2014 Separated out for ease of use by WindLibC */

#ifndef __riscos_ho_flex_h
#define __riscos_ho_flex_h

#define TBOXLIBS_FLEX 1

#include "flex.h" /* C: tboxlibs */

/*#define REPORT_FLEX 1*/

#if defined(REPORT_FLEX) /* reporting shims */

#define flex_alloc(anchor, size) report_flex_alloc(anchor, size)
_Check_return_
extern BOOL report_flex_alloc(flex_ptr anchor, int size);

#define flex_extend(anchor, newsize) report_flex_extend(anchor, newsize)
_Check_return_
extern BOOL report_flex_extend(flex_ptr anchor, int newsize);

#define flex_free(anchor) report_flex_free(anchor)
extern void report_flex_free(flex_ptr anchor);

#define flex_size(anchor) report_flex_size(anchor)
_Check_return_
extern int report_flex_size(flex_ptr anchor);

#endif /* REPORT_FLEX */

/* granularity of flex allocation to use */

extern int flex_granularity;

/* like flex_free(), but caters for already freed / not yet allocated */

extern void
flex_dispose(
    flex_ptr anchor);

/*ncr*/
extern BOOL
flex_forbid_shrink(
    _InVal_     BOOL forbid);

/* give flex memory to another owner */

extern void
flex_give_away(
    flex_ptr new_anchor,
    flex_ptr old_anchor);

_Check_return_
extern BOOL
flex_realloc(
    flex_ptr anchor,
    int newsize);

/* like flex_size(), but caters for already freed / not yet allocated */

_Check_return_
extern int
flex_size_maybe_null(
    flex_ptr anchor);

/* A number to tell the user, ie. only approximate: number of bytes free. */

_Check_return_
extern int
flex_storefree(void);

#endif /* __riscos_ho_flex_h */

/* end of riscos/ho_flex.h */
