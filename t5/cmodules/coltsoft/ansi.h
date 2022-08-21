/* ansi.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Standard includes */

#ifndef __ansi_h
#define __ansi_h

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>

#if WINDOWS

#if defined(_MSC_VER) && (_MSC_VER > 1500) /* Needs VS2010 or later */

#include <stdint.h> /* C99 header */

#else /* _MSC_VER */

/* Define from MSVC's internal types */
typedef __int32 int32_t;
#define INT32_MAX _I32_MAX
#define INT32_MIN _I32_MIN

typedef unsigned __int32 uint32_t;
#define UINT64_MAX _UI64_MAX

typedef __int64 int64_t;
#define INT64_MAX _I64_MAX
#define INT64_MIN _I64_MIN

typedef unsigned __int64 uint64_t;
#define UINT64_MAX _UI64_MAX

#endif /* _MSC_VER */

#else /* OS */

#include <stdint.h> /* C99 header */

#ifndef __CC_NORCROFT
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

#endif /* OS */

#if defined(_MSC_VER)
#define _PRId64 "I64d"
#define _PRIu64 "I64u" /* NB %llu doesn't work on Windows 2000 runtime */
#define _PRIx64 "I64x"
#elif 0
#define _PRId64 "lld"
#define _PRIu64 "llu"
#define _PRIx64 "llx"
#elif defined(PRId64)
#define _PRId64 PRId64
#define _PRIu64 PRIu64
#define _PRIx64 PRIx64
#endif

__pragma(warning(push)) __pragma(warning(disable:4255)) /* no function prototype given: converting '()' to '(void)' */
#include <stdio.h>
__pragma(warning(pop))

#include <string.h>

#if WINDOWS
__pragma(warning(push)) __pragma(warning(disable:4820)) /* padding added after data member */
#include <tchar.h>
__pragma(warning(pop))
#endif /* OS */

#include <ctype.h>
#include <limits.h>

#include <float.h>

__pragma(warning(push)) __pragma(warning(disable:4514)) /* unreferenced inline function has been removed (for VC2010 headers) */
#include <math.h>
__pragma(warning(pop))

#include <time.h>
#include <locale.h>
#include <errno.h>

#endif /* __ansi_h */

/* end of ansi.h */
