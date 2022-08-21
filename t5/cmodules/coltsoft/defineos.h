/* defineos.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Some target-specific definitions for the target machine and OS */

#ifndef __defineos_h
#define __defineos_h

/*
 * target machine definition
 */

#if defined(__INTELLISENSE__)
/* if you get into IntelliSense hell, delete the .ncb file */
#endif

#ifndef CROSS_COMPILE
#define CROSS_COMPILE 0
#endif

#if defined(WINDOWS) /* set up on command line */
#else
#if defined(TARGET_WINDOWS)
#define WINDOWS 1
#elif defined(_WIN32) /* defined by compiler (even for _WIN64) */
#define WINDOWS 1
#else
#define WINDOWS 0
#endif
#endif /* WINDOWS */

#if defined(RISCOS) /* set up on command line */
#else
#if defined(TARGET_RISCOS)
#define RISCOS 1
#if defined(HOST_GCCSDK)
#undef  CROSS_COMPILE
#define CROSS_COMPILE 1
#endif
#if defined(HOST_WINDOWS)
#undef  WINDOWS
#define WINDOWS 0
#undef  CROSS_COMPILE
#define CROSS_COMPILE 1
#endif
#elif defined(__acorn) && defined(__CC_NORCROFT) /* defined by Norcroft ARM compiler */
#define RISCOS 1
#else
#define RISCOS 0
#endif
#endif /* RISCOS */

#if !(WINDOWS || RISCOS)
#error Unable to compile code for this OS
#endif /* OS */

/* define something we can consistently test on */
#if (WINDOWS && defined(_UNICODE)) || (RISCOS && 0)
#define APP_UNICODE 1
#if WINDOWS
#define UNICODE 1 /* Many Windows headers still need this rather than the compiler-defined _UNICODE */
#endif
#else
#define APP_UNICODE 0
#endif

/* First time set-up */
#include "cmodules/coltsoft/pragma.h"

#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234

#if defined(__clang__)
#include "cmodules/coltsoft/host_clang.h"
#endif

#if RISCOS

#define BYTE_ORDER LITTLE_ENDIAN

#if !CROSS_COMPILE && defined(__CC_NORCROFT)

/* Norcroft compiler, target ARM / RISC OS */

#ifndef __CHAR_UNSIGNED__
#define __CHAR_UNSIGNED__ 1
#endif

#if __STDC_VERSION__ >= 199901L
#define __FUNCTION__ __func__
#else
#define __FUNCTION__ ""
#endif /* __STDC_VERSION__ */

#define __TFILE__ __FILE__
#define __Tfunc__ __FUNCTION__

/* let Norcroft intrinsics work */
#define INTRINSIC_MEMSET 1

#endif /* CROSS_COMPILE */

#define F64_IS_64_BIT_ALIGNED 0

#define inline_when_fast_fp /*nothing*/

#if CROSS_COMPILE && defined(HOST_GCCSDK) && defined(TARGET_RISCOS)
/*__pragma(message("CROSS_COMPILE: HOST_GCCSDK & TARGET_RISCOS"))*/
#include "cmodules/coltsoft/target_riscos_host_gccsdk.h"
#endif

#if CROSS_COMPILE && defined(HOST_WINDOWS) && defined(TARGET_RISCOS)
__pragma(message("CROSS_COMPILE: HOST_WINDOWS & TARGET_RISCOS"))
#include "cmodules/coltsoft/target_riscos_host_windows.h"
#endif

#ifndef _In_reads_c_
#include "cmodules/coltsoft/ns-sal.h"
#endif

#endif /* RISCOS */

#if WINDOWS

#define BYTE_ORDER LITTLE_ENDIAN

#include "cmodules/coltsoft/defwin32.h"

#endif /* WINDOWS */

#if APP_UNICODE

#if 1

/* UTF-8 encoding is used in USTR/UCHAR */
#define USTR_IS_SBSTR 0
/* TCHAR is WCHAR (UTF-16, was simply UCS-2 on NT 4.0) */
#define TSTR_IS_SBSTR 0

#else

/* Do NOT use UTF-8 encoding in USTR/UCHAR - alias USTR/UCHAR as SBSTR/SBCHAR U8 Latin-N throughout */
#define USTR_IS_SBSTR 1
/* But TCHAR is WCHAR (UTF-16, was simply UCS-2 on NT 4.0) */
#define TSTR_IS_SBSTR 0

#endif

#else /* NOT APP_UNICODE */

/* Do NOT use UTF-8 encoding in USTR/UCHAR - alias USTR/UCHAR as SBSTR/SBCHAR U8 Latin-N throughout */
#define USTR_IS_SBSTR 1
/* Also TSTR/TCHAR is SBSTR/SBCHAR U8 Latin-N */
#define TSTR_IS_SBSTR 1

#endif /* APP_UNICODE */

/* Nobble Microsoft extensions and definitions on sensible systems */

#if RISCOS

#if !CROSS_COMPILE
#define __cdecl
#endif
#define PASCAL

#if CROSS_COMPILE && defined(HOST_WINDOWS)
#define inline __inline /* for MSVC < C99 */
#else
#define __forceinline inline /* macro MSVC tuning keyword for Norcroft needed for rare uses e.g ustr_bptr() */
#endif

#endif /* RISCOS */

#if !defined(_MSC_VER)
#ifndef __assume
#define __assume(expr) /*EMPTY*/
#endif
#endif /* _MSC_VER */

#ifndef __analysis_assume
#define __analysis_assume(expr) __assume(expr)
#endif

/* much more of differences covered in coltsoft/coltsoft.h ... */

#endif /* __defineos_h */

/* end of defineos.h */
