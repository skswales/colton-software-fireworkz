/* ansi.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Standard includes */

#ifndef __ansi_h
#define __ansi_h

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)

/* C99 headers */
#include <inttypes.h> /* includes <stdint.h> */
#include <stdbool.h>

#elif defined(_MSC_VER)

/* Define from MSVC's internal types */
typedef signed char int8_t;
#define INT8_MAX _I8_MAX
#define INT8_MIN _I8_MIN

typedef unsigned char uint8_t;
#define UINT8_MAX _UI8_MAX

typedef __int16 int16_t;
#define INT16_MAX _I16_MAX
#define INT16_MIN _I16_MIN

typedef unsigned __int16 uint16_t;
#define UINT16_MAX _UI16_MAX

typedef __int32 int32_t;
#define INT32_MAX _I32_MAX
#define INT32_MIN _I32_MIN

typedef unsigned __int32 uint32_t;
#define UINT32_MAX _UI32_MAX

typedef __int64 int64_t;
#define INT64_MAX _I64_MAX
#define INT64_MIN _I64_MIN

typedef unsigned __int64 uint64_t;
#define UINT64_MAX _UI64_MAX

#define _PFX_64 "ll"

#endif /* __STDC_VERSION__ */

#if RISCOS && !defined(__CC_NORCROFT)
/* DO need 64-bit integer base types even though cross ain't C99 */

#ifndef INT64_MAX
typedef __int64 int64_t;
#define INT64_MAX _I64_MAX
#define INT64_MIN _I64_MIN
#endif

#ifndef UINT64_MAX
typedef unsigned __int64 uint64_t;
#define UINT64_MAX _UI64_MAX
#endif

#endif /* __CC_NORCROFT */

#if !defined(PRId64)
#define PRId64 "lld"
#define PRIu64 "llu"
#define PRIx64 "llx"
#endif

__pragma(warning(push))
#if WINDOWS && defined(_MSC_VER)
#if _MSC_VER <= 1500 /* VC2008 */
__pragma(warning(disable:4255)) /* no function prototype given: converting '()' to '(void)' (VS2008) */
#endif
#endif
#include <stdio.h>
__pragma(warning(pop))

#include <string.h>

#if WINDOWS
#include <tchar.h>
#endif /* OS */

/* #include <ctype.h> - Fireworkz uses replacement functions */

#include <limits.h>

#include <float.h>
#if !defined(DBL_DECIMAL_DIG)
#if defined(_MSC_VER)
#define DBL_DECIMAL_DIG 17 /* MS float.h gives too short DECIMAL_DIG for < VS2015 */
#elif defined(DECIMAL_DIG)
#define DBL_DECIMAL_DIG DECIMAL_DIG
#else
#error Need a definition of DBL_DECIMAL_DIG for this platform
#endif
#endif

#include <math.h>
#if RISCOS && defined(__CC_NORCROFT)
#undef fma /* Norcroft is currently broken w.r.t. __d_fma inline */
#endif

#include <errno.h>

#endif /* __ansi_h */

/* end of ansi.h */
