/* target_riscos_host_gccsdk.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2015 Stuart Swales */

/* SKS 2012 */

#ifndef __target_riscos_host_gccsdk_h
#define __target_riscos_host_gccsdk_h

#ifndef _CHAR_UNSIGNED
#if 1
#define _CHAR_UNSIGNED 1
#else
#error  _CHAR_UNSIGNED must be set (use -funsigned-char switch)
#endif
#endif

#ifndef _In_reads_
#include "cmodules/coltsoft/no-sal.h"
#endif

#undef  F64_IS_64_BIT_ALIGNED
#define F64_IS_64_BIT_ALIGNED 1

#define __swi(inline_swi_number) /* Norcroft specific */

#define ___assert(e, s) 0 /* Norcroft specific */

#endif /* __target_riscos_host_gccsdk_h */

/* end of target_riscos_host_gccsdk.h */
