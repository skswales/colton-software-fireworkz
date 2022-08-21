/* pragma.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS 15-Jan-1991 */

/* Designed for multiple inclusion */

#if RISCOS

#if CROSS_COMPILE && defined(HOST_WINDOWS) /* TARGET_RISCOS */

/* incompatible pragmas */

#else /* NOT CROSS_COMPILE */

/* Can't handle MSVC pragmas anyway so define out of the way */
#ifndef __pragma_defined
#define __pragma_defined 1
#define __pragma(x) /* nothing */
#endif /* __pragma_defined */

#include "cmodules/coltsoft/pragmari.h"

#endif /* CROSS_COMPILE */

#elif WINDOWS

#if CROSS_COMPILE /* TARGET_WINDOWS */

/* incompatible pragmas */
#ifndef __pragma_defined
#define __pragma_defined 1
#define __pragma(x) /* nothing */
#endif /* __pragma_defined */

#else /* NOT CROSS_COMPILE */

#include "cmodules/coltsoft/pragmams.h"

#endif /* CROSS_COMPILE */

#endif /* OS */

/* end of pragma.h */
