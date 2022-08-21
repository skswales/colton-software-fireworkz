/* target_riscos_host_windows.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2019 Stuart Swales */

/* SKS 2012 */

#ifndef __target_riscos_host_windows_h
#define __target_riscos_host_windows_h

#if !defined(_MSC_VER)
#define _MSC_VER 1500 /* VC2008 */
#else
//#error You need to undefine all compiler defines with /u
/* Now we can survive OK with them - particularly needed for SAL */
#endif

#if !defined(__STDC_VERSION__)
#if _MSC_VER >= 1800 /* VS2013 or later */
#define __STDC_VERSION__ 199901L /* MSVC is still not quite C99 but pretend that it is */
#elif _MSC_VER >= 1400 /* VS2005 or later */
#define __STDC_VERSION__ 199901L /* MSVC is still not quite C99 but pretend that it is for inttypes.h */
#else
#define __STDC_VERSION__ 0L /* MSVC is still not C99 */
#endif
#endif

#if !defined(_CHAR_UNSIGNED)
#if 1
#define _CHAR_UNSIGNED 1 /* We may compile with switches that undefine all MS' macros! */
#else
#error 
#endif
#endif

#if !defined(__CHAR_UNSIGNED__)
#define __CHAR_UNSIGNED__ 1
#endif

/* preempt definition by sourceannotations.h */
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

/* sorry, RISC OS headers ... */
#ifndef __wchar_t
#  define __wchar_t 1
#endif

#if _MSC_VER >= 1600 /* VS2010 or later */
/* __func__ is defined */
#else /* _MSC_VER */
#define __func__ __FUNCTION__
#endif /* _MSC_VER */

#define __TFILE__ TEXT(__FILE__)

#if !defined(__INTELLISENSE__)
#define __Tfunc__ TEXT(__FUNCTION__)
#else
#define __Tfunc__ TEXT("__FUNCTION__")
#endif

#pragma warning(push)
#pragma warning(disable:4820) /* 'x' bytes padding added after data member */

#ifndef SAL_SUPP_H /* try to suppress inclusion as we get macro redefinition warnings when using Visual C++ 11.0 with the Windows 7.1 SDK */
#define SAL_SUPP_H 1
#endif

#if 1 /* turn on for a big surprise with MSVC /analyze ! */
#define _USE_DECLSPECS_FOR_SAL  0
#define _USE_ATTRIBUTES_FOR_SAL 1
/* Ideally #include "%VCINSTALLDIR%\Include\sal.h" */
#if _MSC_VER == 1900 /* VS2015 */
#include "C:\Program Files\Microsoft Visual Studio 14.0\VC\Include\sal.h"
#elif _MSC_VER == 1800 /* VS2013 */
#include "C:\Program Files\Microsoft Visual Studio 12.0\VC\Include\sal.h"
#elif _MSC_VER == 1500 /* VS2008 */
#include "C:\Program Files\Microsoft Visual Studio 9.0\VC\Include\sal.h"
#else
/* VC2005 sal.h is different */
#endif
#endif

#pragma warning(pop)

#include "cmodules/coltsoft/ns-sal.h"

__pragma(warning(disable:4514)) /* 'x' : unreferenced inline function has been removed */
/*__pragma(warning(disable:4548))*/ /* expression before comma has no effect; expected expression with side-effect */
__pragma(warning(disable:4710)) /* 'x' : function not inlined */
__pragma(warning(disable:4711)) /* function 'x' selected for automatic inline expansion */ /* /Ob2 */
__pragma(warning(disable:4820)) /* padding added after data member */

#undef  F64_IS_64_BIT_ALIGNED
#define F64_IS_64_BIT_ALIGNED 1

#ifdef _WIN64
#error Just don't even think about it!
#endif

#endif /* __target_riscos_host_windows_h */

/* end of target_riscos_host_windows.h */
