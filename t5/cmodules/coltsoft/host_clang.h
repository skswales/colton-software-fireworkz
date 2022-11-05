/* host_clang.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2019-2022 Stuart Swales */

#ifndef __host_clang_h
#define __host_clang_h

#ifndef __CHAR_UNSIGNED__
#error  __CHAR_UNSIGNED__ must be set (use -funsigned-char switch)
#endif

#undef  F64_IS_64_BIT_ALIGNED
#define F64_IS_64_BIT_ALIGNED 1

#define ___assert(e, s) 0 /* Norcroft specific */

#pragma clang diagnostic ignored "-Wchar-subscripts"
#pragma clang diagnostic ignored "-Wmissing-braces"
#pragma clang diagnostic ignored "-Wnonportable-include-path"
#pragma clang diagnostic ignored "-Wnonportable-system-include-path"
#pragma clang diagnostic ignored "-Wreserved-id-macro"

#if defined(__cplusplus)
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant" /* NULL gets defined as zero by vcruntime.h */
#endif

#endif /* __host_clang_h */

/* end of host_clang.h */
