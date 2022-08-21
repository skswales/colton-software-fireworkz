/* defwin32.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Some target-specific definitions for Windows */

#ifndef __def_win32_h
#define __def_win32_h

/* Ensure that compiler switches are set correctly */

/* -J switch to compiler defines this */
#ifndef _CHAR_UNSIGNED
#error  _CHAR_UNSIGNED must be set (use -J switch)
#endif

#ifndef WINVER
#error WINVER not defined (try 0x0500)
#endif

#ifndef __STDC_VERSION__
#if _MSC_VER >= 1800 /* VS2103 */
#define __STDC_VERSION__ 199901L /* MSVC is still not quite C99 but pretend that it is */
#else
#define __STDC_VERSION__ 0L /* MSVC is still not C99 */
#endif
#endif

#if _MSC_VER > 1500 /* Needs VS2010 or later */
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

#if __STDC_VERSION__ < 199901L
/* fix some functions that we use that MSVC < 2013 provides with non-C99 names but compatible API */
#define copysign(_Number, _Sign)    _copysign(_Number, _Sign)
#define isfinite(X)                 _finite(X)
#define isnan(X)                    _isnan(X)
#endif /* __STDC_VERSION__ */

#if defined(CODE_ANALYSIS)
/* some functions we inline for arg type checking */
#else
/* let compiler intrinsics work */
#define INTRINSIC_MEMCMP 1
#define INTRINSIC_MEMCPY 1
#define INTRINSIC_MEMSET 1
#endif

#if defined(CODE_ANALYSIS) && 1
#define _PFT_SHOULD_CHECK_RETURN 1
#endif

#if defined(CODE_ANALYSIS) && 1
#define _PFT_SHOULD_CHECK_RETURN_WAT 1
#endif

#if (defined(CODE_ANALYSIS) || 0) && 1
/* Could allow sal.h to choose attribute or __declspec implementation - these override */
#define _USE_DECLSPECS_FOR_SAL  0
#define _USE_ATTRIBUTES_FOR_SAL 1
#elif 1
/* Enable expansion of SAL macros in non-Code Analysis mode for API conformance checking */
#define _USE_DECLSPECS_FOR_SAL  0
#define _USE_ATTRIBUTES_FOR_SAL 1
#else
/* Usually disable expansion of SAL macros in non-Code Analysis mode to improve compiler throughput */
#define _USE_DECLSPECS_FOR_SAL  0
#define _USE_ATTRIBUTES_FOR_SAL 0
#endif /* CODE_ANALYSIS */

#pragma warning(push)
#pragma warning(disable:4820) /* 'x' bytes padding added after data member */

#ifndef SAL_SUPP_H /* try to suppress inclusion as we get macro redefinition warnings when using Visual C++ 11.0 with the Windows 7.1 SDK */
//#define SAL_SUPP_H 1
#endif

//#if !defined(__INTELLISENSE__) /* Google "Troubleshooting Tips for IntelliSense Slowness" */
#include "sal.h"
//#endif /* __INTELLISENSE__ (VS2012 package manager crashes - does this help?) */

#pragma warning(pop)

#if defined(__ANNOTATION)
#define SAL_SUPP_H 1 /* try to suppress inclusion as we get macro redefinition warnings when using Visual C++ 12.0 with the Windows 7.1 SDK */
#endif

/* VC2012/2013 2.0 SAL; VC2008 1.1 SAL; VC2005 1.0 SAL - sal.h is different across these so fix up differences */

#ifndef _In_reads_
#include "cmodules/coltsoft/no-sal.h"
#endif

/* Now include (a limited set of) Windows header files */

#if _MSC_VER >= 1500
#include "WinSDKVer.h"
#endif

#define STRICT 1

#ifndef ISOLATION_AWARE_ENABLED
#define ISOLATION_AWARE_ENABLED 0 /* saves loads of warning push/pop */
#endif

#define WIN32_LEAN_AND_MEAN 1

/* from VC_EXTRALEAN (does still speed rebuild a little) */

/*#define WIN32_EXTRA_LEAN */
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOSOUND
#define NOCOMM
#define NOKANJI
#define NORPC
#define NOPROXYSTUB
#define NOIMAGE
#define NOTAPE

#define NOHELP 1 /* WinHelp API is now obsolete */

#define INC_OLE2 1

#ifndef CPLUSPLUSINTERFACE
#define CINTERFACE 1
#define COBJMACROS 1
#endif

#pragma warning(push)
#pragma warning(disable:4115) /* named type definition in parentheses */
#pragma warning(disable:4255) /* no function prototype given: converting '()' to '(void) */
#pragma warning(disable:4668) /* not defined as a preprocessor macro */
#pragma warning(disable:4820) /* padding added after data member */
#pragma warning(disable:4826) /* conversion from 'const void *' to 'void *' is sign-extended */ /* for Windows Server 2003 R2 SDK */
#include "windows.h"
#pragma warning(pop)

#undef  GetGValue           /* poor definition in WinGDI.h causes /RTCc failure */
#define GetGValue(rgb)      (LOBYTE((rgb) >> 8))

#ifndef NTDDI_VISTA
#define NTDDI_VISTA NTDDI_LONGHORN
#endif

/* these appear to be missing ... */
typedef const POINT * PCPOINT;
typedef const RECT * PCRECT;

/* coltsoft-style ones... */
typedef  PWSTR *  P_PWSTR;
typedef PCWSTR * P_PCWSTR;

/* and this is a useful alias meaning CH_NULL-terminated array of WCHAR */
#define WCHARZ WCHAR

typedef const TCHAR * PCTCH;

typedef  PTSTR *  P_PTSTR;
typedef PCTSTR * P_PCTSTR;

/* and this is a useful alias meaning CH_NULL-terminated array of TCHAR */
#define TCHARZ TCHAR

/*
macros that ought to be in windows.h but aren't
*/

#if !defined(WM_MOUSEHWHEEL)
#if (_WIN32_WINNT >= 0x0600)
#error Eh? Why isn't this defined?
#elif(_WIN32_WINNT >= 0x0400)
/* See the comment below */
#else
#error Oh come on! Define _WIN32_WINNT
#endif
// This value must be defined to handle the WM_MOUSEHWHEEL on
// Windows 2000 and Windows XP, it will be defined in the Longhorn SDK
// NB it is now defined with _WIN32_WINNT >= 0x0600
// WM_MOUSEHWHEEL messages MAY be emulated by IntelliType Pro
// or IntelliPoint (if implemented in future versions).
#define WM_MOUSEHWHEEL          0x020E
#endif

/*
shellapi.h must be included before windowsx.h for relevant message crackers
*/

#pragma warning(push)
#pragma warning(disable:4201) /* nonstandard extension used : nameless struct/union */
#pragma warning(disable:4255) /* 'x' : no function prototype given: converting '()' to '(void)' */ /* for SDK v6.0A */
#pragma warning(disable:4668) /* not defined as a preprocessor macro */
#pragma warning(disable:4820) /* padding added after data member */
#include "shellapi.h"
#pragma warning(pop)

#include "windowsx.h"

/* void Cls_OnMouseLeave(HWND hwnd) */
#define HANDLE_WM_MOUSELEAVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)

/*
* From the book Advanced Windows (Third Edition) by Jeffrey Richter:
*
* "When using message crackers with dialog boxes, you should not use the
* HANDLE_MSG macro from Microsoft's WINDOWSX.H header file. The reason to
* avoid this macro is that it doesn't return TRUE or FALSE to indicate
* whether a message was handled by the dialog box procedure."
*/

#ifndef HANDLE_DLGMSG
// the normal HANDLE_MSG macro in WINDOWSX.H does not work properly
// for dialog boxes because DlgProcs return a BOOL instead of an LRESULT
// (like WndProcs)
#define HANDLE_DLGMSG(hwnd, message, fn) \
    case (message): \
    return(SetDlgMsgResult(hwnd, message, \
            HANDLE_##message((hwnd), (wParam), (lParam), (fn))))
#endif

/*
Turn off various Level 4 warnings
*/

#pragma warning(disable:4514) /* 'x' : unreferenced inline function has been removed */
#pragma warning(disable:4710) /* 'x' : function not inlined */ /* <<< comment out this one to see what it's up to ... */
#pragma warning(disable:4711) /* function 'x' selected for automatic inline expansion */ /* /Ob2 */

#if defined(_WIN64)
#pragma warning(disable:4820) /* padding added after data member */
#else
/* drives me mad with /Wall... */
#pragma warning(disable:4820) /* padding added after data member */ /* <<< temp */
#endif /* _WIN64 */

#if defined(__cplusplus)
/* For test builds */
#pragma warning(disable:4061) /* enumerator 'x' in switch of enum 'y' is not explicitly handled by a case label */
#pragma warning(disable:4062) /* enumerator 'x' in switch of enum 'y' is not handled */
#pragma warning(disable:4063) /* case 'x' is not a valid value for switch of enum 'y' */
#pragma warning(disable:4065) /* switch statement contains 'default' but no 'case' labels */
#if 1
#pragma warning(disable:4018) /* 'expression' : signed/unsigned mismatch */
#pragma warning(disable:4365) /* signed/unsigned mismatch */
#pragma warning(disable:4389) /* 'operator' : signed/unsigned mismatch */
#endif
#endif /* __cplusplus */

/* Temporarily disable some /analyze warnings */
#pragma warning(disable:6246) /* Local declaration of 'x' hides declaration of the same name in outer scope. For additional information, see previous declaration at line 'y'  */
#pragma warning(disable:6385) /* Invalid data: accessing 'argument 2', the readable size is 'x' bytes, but 'y' bytes might be read */

#endif /* __def_win32_h */

/* end of defwin32.h */
