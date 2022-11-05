/* target_riscos_host_gccsdk.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2022 Stuart Swales */

#ifndef __target_riscos_host_gccsdk_h
#define __target_riscos_host_gccsdk_h

#if !defined(__CHAR_UNSIGNED__)
#error  __CHAR_UNSIGNED__ must be set (use -funsigned-char switch)
#endif

#include "cmodules/coltsoft/ns-sal.h"

#undef  F64_IS_64_BIT_ALIGNED
#define F64_IS_64_BIT_ALIGNED 1

#define ___assert(e, s) 0 /* Norcroft specific */

#if defined(__clang__)
//#pragma clang diagnostic ignored "-Wchar-subscripts"

#if defined(CODE_ANALYSIS)
#define INTRINSIC_MEMCMP 1
#define INTRINSIC_MEMCPY 1
#define INTRINSIC_MEMSET 1
#endif /* CODE_ANALYSIS */
#endif /* __clang__ */

#define USE_OWN_COMPLEX_IMPL 1 /* only for CROSS_COMPILE - see RISC OS complex.h */

#endif /* __target_riscos_host_gccsdk_h */

/* end of target_riscos_host_gccsdk.h */
