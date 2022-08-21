/* xwstring.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Library module for WCHAR-based string handling */

/* SKS Jul 2014 derived from xtstring.h */

#ifndef __xwstring_h
#define __xwstring_h

/*
exported functions
*/

_Check_return_
extern STATUS
al_wstr_realloc(
    _InoutRef_  P_ARRAY_HANDLE_WSTR p_array_handle_wstr,
    _In_z_      PCWSTR wstr);

_Check_return_
static inline STATUS
al_wstr_alloc(
    _OutRef_    P_ARRAY_HANDLE_WSTR p_array_handle_wstr,
    _In_z_      PCWSTR wstr)
{
    *p_array_handle_wstr = 0;

    return(al_wstr_realloc(p_array_handle_wstr, wstr));
}

static inline void
wstr_clr(
    _Inout_opt_ P_PWSTR aa)
{
    al_ptr_dispose(P_P_ANY_PEDANTIC(aa));
}

_Check_return_
extern STATUS
wstr_set(
    _OutRef_    P_PWSTR aa,
    _In_opt_z_  PCWSTR wstr);

_Check_return_
extern STATUS
wstr_set_n(
    _OutRef_    P_PWSTR aa,
    _In_reads_opt_(wchars_n) PCWCH wchars,
    _InVal_     U32 wchars_n);

_Check_return_
extern STATUS
wstr_set_from_ustr(
    _OutRef_    P_PWSTR aa,
    _In_opt_z_  PC_USTR ustr);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
wstr_compare(
    _In_z_      PCWSTR wstr_a,
    _In_z_      PCWSTR wstr_b);

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
wstr_compare_equals(
    _In_z_      PCWSTR wstr_a,
    _In_z_      PCWSTR wstr_b);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
wstr_compare_n2(
    _In_/*wchars_n_a*/ PCWSTR wstr_a,
    _InVal_     U32 wchars_n_a /*strlen_with,without_NULLCH*/,
    _In_/*wchars_n_b*/ PCWSTR wstr_b,
    _InVal_     U32 wchars_n_b /*strlen_with,without_NULLCH*/);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
wstr_compare_nocase(
    _In_z_      PCWSTR wstr_a,
    _In_z_      PCWSTR wstr_b);

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
wstr_compare_equals_nocase(
    _In_z_      PCWSTR wstr_a,
    _In_z_      PCWSTR wstr_b);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
wstr_compare_n2_nocase(
    _In_/*wchars_n_a*/ PCWSTR wstr_a,
    _InVal_     U32 wchars_n_a /*strlen_with,without_NULLCH*/,
    _In_/*wchars_n_b*/ PCWSTR wstr_b,
    _InVal_     U32 wchars_n_b /*strlen_with,without_NULLCH*/);

/*
strcpy / _s() etc. replacements that ensure CH_NULL-termination
*/

/*ncr*/
extern U32 /* WCHARs currently in output */
wstr_xstrkat(
    _Inout_updates_z_(dst_n) PWSTR dst,
    _InVal_     U32 dst_n,
    _In_z_      PCWSTR src);

/*ncr*/
extern U32 /* WCHARs currently in output */
wstr_xstrnkat(
    _Inout_updates_z_(dst_n) PWSTR dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PCWCH src,
    _InVal_     U32 src_n);

/*ncr*/
extern U32 /* WCHARs currently in output */
wstr_xstrkpy(
    _Out_writes_z_(dst_n) PWSTR dst,
    _InVal_     U32 dst_n,
    _In_z_      PCWSTR src);

/*ncr*/
extern U32 /* WCHARs currently in output */
wstr_xstrnkpy(
    _Out_writes_z_(dst_n) PWSTR dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PCWCH src,
    _InVal_     U32 src_n);

_Check_return_
extern int __cdecl
wstr_xsnprintf(
    _Out_writes_z_(dst_n) PWSTR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PCWSTR format,
    /**/        ...);

_Check_return_
extern int __cdecl
wstr_xvsnprintf(
    _Out_writes_z_(dst_n) PWSTR,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PCWSTR format,
    /**/        va_list args);

#if WINDOWS && 0

/* preferably our own controlled string copy fns: safe_wstr{kpy,nkpy,kat,nkat} etc. */

#define wstrlen(s)                  wcslen(s)

/*#define wstrcmp(s1, s2)             wcscmp(s1, s2)*/
/*#define wstricmp(s1, s2)            wcsicmp(s1, s2)*/
/*#define wstrncmp(s1, s2, n)         wcsncmp(s1, s2, n)*/
/*#define wstrnicmp(s1, s2, n)        wcsnicmp(s1, s2, n)*/

#define wstrchr(s, c)               wcschr(s, c)
#define wstrrchr(s, c)              wcsrchr(s, c)
#define wstrstr(s1, s2)             wcsstr(s1, s2)
#define wstrpbrk(s1, s2)            wcspbrk(s1, s2)

#define wstrtod(p, pp)              wcstod(p, pp)
#define wstrtol(p, pp, r)           wcstol(p, pp, r)
#define wstrtoul(p, pp, r)          wcstoul(p, pp, r)

#else /* OS */

_Check_return_
extern size_t
wstrlen(
    _In_z_      PCWSTR wstr);

_Check_return_
extern PWSTR
wstrchr(
    _In_z_      PCWSTR wstr,
    _InVal_     WCHAR wch);

_Check_return_
extern PWSTR
wstrpbrk(
    _In_z_      PCWSTR wstr,
    _In_z_      PCWSTR wstr_control);

/*#define wstrchr(s, c)               strchr(s, c)*/
/*#define wstrrchr(s, c)              strrchr(s, c)*/
/*#define wstrstr(s1, s2)             strstr(s1, s2)*/
/*#define wstrpbrk(s1, s2)            strpbrk(s1, s2)*/

/*#define wstrtod(p, pp)              strtod(p, pp)*/
/*#define wstrtol(p, pp, r)           strtol(p, pp, r)*/
/*#define wstrtoul(p, pp, r)          strtoul(p, pp, r)*/

#endif /* OS */

#define wstrcmp(s1, s2)             wstr_compare(s1, s2)
#define wstrncmp(s1, s2, n)         wstr_compare_n2(s1, n, s2, n)
#define wstricmp(s1, s2)            wstr_compare_nocase(s1, s2)
#define wstrnicmp(s1, s2, n)        wstr_compare_n2_nocase(s1, n, s2, n)

/* 32-bit (rather than size_t) sized strlen() */
#define wstrlen32(wstr) \
    ((U32) wstrlen(wstr))

#define wstrlen32p1(wstr) \
    (1U /*CH_NULL*/ + wstrlen32(wstr))

_Check_return_
static inline U32
wstrlen32_n(
    _In_z_      PCWSTR wstr,
    _InVal_     U32 wchars_n /*strlen_with,without_NULLCH*/)
{
    if(strlen_without_NULLCH == wchars_n)
        return(wstrlen32(wstr));

    if(strlen_with_NULLCH == wchars_n)
        return(wstrlen32p1(wstr));

    return(wchars_n);
}

#if WINDOWS && 0

#define C_wcsicmp(a, b)     _wcsicmp(a, b)
#define C_wcsnicmp(a, b, n) _wcsnicmp(a, b, n)

#else

_Check_return_
extern int
C_wcsicmp(
    _In_z_      PCWSTR wstr_a,
    _In_z_      PCWSTR wstr_b);

_Check_return_
extern int
C_wcsnicmp(
    _In_z_/*wchars_n*/ PCWSTR wstr_a,
    _In_z_/*wchars_n*/ PCWSTR wstr_b,
    _InVal_     U32 wchars_n);

#endif /* OS */

#endif /* __xwstring_h */

/* end of xwstring.h */
