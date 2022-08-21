/* xwstring.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Library module for WCHAR-based string handling */

/* SKS Jul 2014 derived from xtstring.c */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef          __xwstring_h
#include "cmodules/xwstring.h"
#endif

/******************************************************************************
*
* realloc an aligator WCHAR-based string
*
******************************************************************************/

_Check_return_
extern STATUS
al_wstr_realloc(
    _InoutRef_  P_ARRAY_HANDLE_WSTR p_array_handle_wstr,
    _In_z_      PCWSTR wstr)
{
    const U32 wchars_n = wstrlen32p1(wstr); /*CH_NULL*/
    U32 nullch_char = 0;
    PWSTR wstr_wr;
    STATUS status;

    if(0 != array_elements32(p_array_handle_wstr))
        if(CH_NULL == *array_ptr(p_array_handle_wstr, PCWCH, array_elements32(p_array_handle_wstr) - 1))
            nullch_char = 1;

    if(NULL == (wstr_wr = al_array_extend_by_WCHAR(p_array_handle_wstr, wchars_n - nullch_char, &array_init_block_wchar, &status)))
        return(status);

    if(nullch_char)
        wstr_wr -= 1; /* retract pointer over the CH_NULL */

    memcpy32(wstr_wr, wstr, wchars_n * sizeof32(*wstr)); /* OK if wstr is unaligned */

    return(STATUS_OK);
}

/******************************************************************************
*
* do a WCHAR-based string assignment
*
******************************************************************************/

_Check_return_
extern STATUS
wstr_set(
    _OutRef_    P_PWSTR aa,
    _In_opt_z_  PCWSTR wstr)
{
    STATUS status;
    PWSTR wstr_wr;
    U32 wchars_n;

    if(NULL == wstr)
    {
        *aa = NULL;
        return(STATUS_OK);
    }

    wchars_n = wstrlen32p1(wstr) /*CH_NULL*/;

    if(NULL == (*aa = wstr_wr = al_ptr_alloc_elem(WCHARZ, wchars_n, &status)))
        return(status);

    memcpy32(wstr_wr, wstr, wchars_n * sizeof32(*wstr)); /* OK if wstr is unaligned */

    return(STATUS_DONE);
}

_Check_return_
extern STATUS
wstr_set_n(
    _OutRef_    P_PWSTR aa,
    _In_reads_opt_(wchars_n) PCWCH wchars,
    _InVal_     U32 wchars_n)
{
    STATUS status;
    PWSTR wstr_wr;

    if(0 == wchars_n)
    {
        *aa = NULL;
        return(STATUS_OK);
    }

    if(NULL == (*aa = wstr_wr = al_ptr_alloc_elem(WCHARZ, (wchars_n + 1 /*CH_NULL*/), &status)))
        return(status);

    if(NULL == wchars)
    {   /* NULL == wchars allows client to allocate for a string of wchars_n characters (and the CH_NULL) */
        wstr_wr[0] = CH_NULL; /* allows append e.g. wstr_xstrkat() */
        wstr_wr[wchars_n] = CH_NULL; /* in case client forgets it */
    }
    else
    {
        memcpy32(wstr_wr, wchars, wchars_n * sizeof32(*wchars)); /* OK if wchars is unaligned */
        wstr_wr[wchars_n] = CH_NULL;
        assert(wchars_n <= wstrlen32(wstr_wr));
    }

    return(STATUS_DONE);
}

/******************************************************************************
*
* case sensitive lexical comparison of leading chars of two WCHAR-based strings
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
wstr_compare(
    _In_z_      PCWSTR wstr_a,
    _In_z_      PCWSTR wstr_b)
{
    return(wstr_compare_n2(wstr_a, strlen_without_NULLCH, wstr_b, strlen_without_NULLCH));
}

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
wstr_compare_equals(
    _In_z_      PCWSTR wstr_a,
    _In_z_      PCWSTR wstr_b)
{
    return(0 == wstr_compare_n2(wstr_a, strlen_without_NULLCH, wstr_b, strlen_without_NULLCH));
}

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
wstr_compare_n2(
    _In_/*wchars_n_a*/ PCWSTR wstr_a,
    _InVal_     U32 wchars_n_a /*strlen_with,without_NULLCH*/,
    _In_/*wchars_n_b*/ PCWSTR wstr_b,
    _InVal_     U32 wchars_n_b /*strlen_with,without_NULLCH*/)
{
    int res;
    U32 limit = MIN(wchars_n_a, wchars_n_b);
    U32 i;
    U32 remain_a, remain_b;

    profile_ensure_frame();

    assert(0 == (1 & (uintptr_t) wstr_a)); /* OK on x86 but not on ARM */
    assert(0 == (1 & (uintptr_t) wstr_b)); /* OK on x86 but not on ARM */

    /* no worry about limiting to strlen as this function demands CH_NULL-terminated strings */
    for(i = 0; i < limit; ++i)
    {
        WCHAR c_a = *wstr_a++;
        WCHAR c_b = *wstr_b++;

        res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-2 range */

        if(0 != res)
            return(res);

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal */
    }

    /* matched up to the comparison limit */

    /* which string has the greater number of chars left over? */
    remain_a = wchars_n_a - limit;
    remain_b = wchars_n_b - limit;

    if(remain_a == remain_b)
        return(0); /* ended together at the specified finite lengths -> equal */

    /* sort out any string length residuals */
    assert((wchars_n_a != strlen_with_NULLCH) || (wchars_n_b == strlen_with_NULLCH)); /* an admixture wouldn't be useful */

    if(wchars_n_a >= strlen_without_NULLCH)
        remain_a = wstrlen32_n(wstr_a, wchars_n_a);

    if(wchars_n_b >= strlen_without_NULLCH)
        remain_b = wstrlen32_n(wstr_b, wchars_n_b);

    /*if(remain_a == remain_b)
        return(0);*/ /* ended together at the determined finite lengths -> equal */

    res = (int) remain_a - (int) remain_b;

    return(res);
}

/******************************************************************************
*
* case insensitive lexical comparison of leading chars of two WCHAR-based strings
*
* uses sbchar_sortbyte() for collation
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
wstr_compare_nocase(
    _In_z_      PCWSTR wstr_a,
    _In_z_      PCWSTR wstr_b)
{
    return(wstr_compare_n2_nocase(wstr_a, strlen_without_NULLCH, wstr_b, strlen_without_NULLCH));
}

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
wstr_compare_equals_nocase(
    _In_z_      PCWSTR wstr_a,
    _In_z_      PCWSTR wstr_b)
{
    return(0 == wstr_compare_n2_nocase(wstr_a, strlen_without_NULLCH, wstr_b, strlen_without_NULLCH));
}

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
wstr_compare_n2_nocase(
    _In_/*wchars_n_a*/ PCWSTR wstr_a,
    _InVal_     U32 wchars_n_a /*strlen_with,without_NULLCH*/,
    _In_/*wchars_n_b*/ PCWSTR wstr_b,
    _InVal_     U32 wchars_n_b /*strlen_with,without_NULLCH*/)
{
    int res;
    U32 limit = MIN(wchars_n_a, wchars_n_b);
    U32 i;
    U32 remain_a, remain_b;

    profile_ensure_frame();

    assert(0 == (1 & (uintptr_t) wstr_a)); /* OK on x86 but not on ARM */
    assert(0 == (1 & (uintptr_t) wstr_b)); /* OK on x86 but not on ARM */

    /* no worry about limiting to strlen as this function demands CH_NULL-terminated strings */
    for(i = 0; i < limit; ++i)
    {
        WCHAR c_a = *wstr_a++;
        WCHAR c_b = *wstr_b++;

        res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-2 range */

        if(0 != res)
        {   /* retry with case folding */
            if((c_a < 256) && (c_b < 256))
            {
                c_a = (WCHAR) sbchar_sortbyte(c_a);
                c_b = (WCHAR) sbchar_sortbyte(c_b);
            }

            res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-2 range */

            if(0 != res)
                return(res);
        }

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal */
    }

    /* matched up to the comparison limit */

    /* which string has the greater number of chars left over? */
    remain_a = wchars_n_a - limit;
    remain_b = wchars_n_b - limit;

    if(remain_a == remain_b)
        return(0); /* ended together at the specified finite lengths -> equal */

    /* sort out any string length residuals */
    assert((wchars_n_a != strlen_with_NULLCH) || (wchars_n_b == strlen_with_NULLCH)); /* an admixture wouldn't be useful */

    if(wchars_n_a >= strlen_without_NULLCH)
        remain_a = wstrlen32_n(wstr_a, wchars_n_a);

    if(wchars_n_b >= strlen_without_NULLCH)
        remain_b = wstrlen32_n(wstr_b, wchars_n_b);

    /*if(remain_a == remain_b)
        return(0);*/ /* ended together at the determined finite lengths -> equal */

    res = (int) remain_a - (int) remain_b;

    return(res);
}

/*
portable string copy functions that ensure CH_NULL termination without buffer overflow

strcpy(), strncat() etc. and even their _s() variants are all a bit 'wonky'

copying to dst buffer is limited by dst_n characters

dst buffer is always then CH_NULL-terminated within dst_n characters limit
*/

/*
append up characters from src (until CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* WCHARs currently in output */
wstr_xstrkat(
    _Inout_updates_z_(dst_n) PWSTR dst,
    _InVal_     U32 dst_n,
    _In_z_      PCWSTR src)
{
    U32 dst_idx = 0;
    WCHAR ch;

    assert(0 == (1 & (uintptr_t) dst)); /* OK on x86 but not on ARM */
    assert(0 == (1 & (uintptr_t) src)); /* OK on x86 but not on ARM */

    /* find out where in dst buffer to append the string */
    for(;;)
    {
        /* watch out for unterminated dst buffer */
        if(dst_idx == dst_n)
        {
            assert(dst_idx != dst_n);
            /* keep our promise to terminate if that's possible */
            if(0 != dst_n)
                dst[dst_n - 1] = CH_NULL;
            return(dst_n);
        }

        ch = dst[dst_idx];

        /* append here? */
        if(CH_NULL == ch)
            break;

        ++dst_idx;
    }

    /* copy src to this point */
    for(;;)
    {
        ch = *src++;

        /* finished source string before reaching source limit? */
        if(CH_NULL == ch)
            break;

        /* is there room to put both this character *and a CH_NULL* to destination buffer? */
        if((dst_idx + 1) >= dst_n)
            break;

        dst[dst_idx++] = ch;
    }

    /* NB watch out for zero dst_n (and cockups) */
    if(dst_idx >= dst_n)
    {
        assert(dst_idx < dst_n);
        if(0 == dst_n)
            return(dst_n);
        dst_idx = dst_n - 1;
    }

    dst[dst_idx++] = CH_NULL; /* ensure terminated */

    return(dst_idx); /* WCHARs currently in output */
}

/*
append up to src_n characters from src (or fewer, if CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* WCHARs currently in output */
wstr_xstrnkat(
    _Inout_updates_z_(dst_n) PWSTR dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PCWCH src,
    _InVal_     U32 src_n)
{
    U32 dst_idx = 0;
    U32 src_idx = 0;
    WCHAR ch;

    assert(0 == (1 & (uintptr_t) dst)); /* OK on x86 but not on ARM */
    assert(0 == (1 & (uintptr_t) src)); /* OK on x86 but not on ARM */

    /* find out where in dst buffer to append the string */
    for(;;)
    {
        /* watch out for unterminated dst buffer */
        if(dst_idx == dst_n)
        {
            assert(dst_idx != dst_n);
            /* keep our promise to terminate if that's possible */
            if(0 != dst_n)
                dst[dst_n - 1] = CH_NULL;
            return(dst_n);
        }

        ch = dst[dst_idx];

        /* append here? */
        if(CH_NULL == ch)
            break;

        ++dst_idx;
    }

    /* copy src to this point */
    while(src_idx < src_n)
    {
        ch = src[src_idx++];

        /* finished source string before reaching source limit? */
        if(CH_NULL == ch)
            break;

        /* is there room to put both this character *and a CH_NULL* to destination buffer? */
        if((dst_idx + 1) >= dst_n)
            break;

        dst[dst_idx++] = ch;
    }

    /* NB watch out for zero dst_n (and cockups) */
    if(dst_idx >= dst_n)
    {
        assert(dst_idx < dst_n);
        if(0 == dst_n)
            return(dst_n);
        dst_idx = dst_n - 1;
    }

    dst[dst_idx++] = CH_NULL; /* ensure terminated */

    return(dst_idx); /* WCHARs currently in output */
}

/*
copy characters from src (until CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* WCHARs currently in output */
wstr_xstrkpy(
    _Out_writes_z_(dst_n) PWSTR dst,
    _InVal_     U32 dst_n,
    _In_z_      PCWSTR src)
{
    U32 dst_idx = 0;
    WCHAR ch;

    assert(0 == (1 & (uintptr_t) dst)); /* OK on x86 but not on ARM */
    assert(0 == (1 & (uintptr_t) src)); /* OK on x86 but not on ARM */

    /* copy src to this point */
    for(;;)
    {
        ch = *src++;

        /* finished source string? */
        if(CH_NULL == ch)
            break;

        /* is there room to put both this character *and a CH_NULL* to destination buffer? */
        if((dst_idx + 1) >= dst_n)
            break;

        dst[dst_idx++] = ch;
    }

    /* NB watch out for zero dst_n (and cockups) */
    if(dst_idx >= dst_n)
    {
        assert(dst_idx < dst_n);
        if(0 == dst_n)
            return(dst_n);
        dst_idx = dst_n - 1;
    }

    dst[dst_idx++] = CH_NULL; /* ensure terminated */

    return(dst_idx); /* WCHARs currently in output */
}

/*
copy up to src_n characters from src (or fewer, if CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* WCHARs currently in output */
wstr_xstrnkpy(
    _Out_writes_z_(dst_n) PWSTR dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PCWCH src,
    _InVal_     U32 src_n)
{
    U32 dst_idx = 0;
    U32 src_idx = 0;
    WCHAR ch;

    assert(0 == (1 & (uintptr_t) dst)); /* OK on x86 but not on ARM */
    assert(0 == (1 & (uintptr_t) src)); /* OK on x86 but not on ARM */

    /* copy src to this point */
    while(src_idx < src_n)
    {
        ch = src[src_idx++];

        /* finished source string before reaching source limit? */
        if(CH_NULL == ch)
            break;

        /* is there room to put both this character *and a CH_NULL* to destination buffer? */
        if((dst_idx + 1) >= dst_n)
            break;

        dst[dst_idx++] = ch;
    }

    /* NB watch out for zero dst_n (and cockups) */
    if(dst_idx >= dst_n)
    {
        assert(dst_idx < dst_n);
        if(0 == dst_n)
            return(dst_n);
        dst_idx = dst_n - 1;
    }

    dst[dst_idx++] = CH_NULL; /* ensure terminated */

    return(dst_idx); /* WCHARs currently in output */
}

/*
a portable but inexact replacement for (v)snprintf(), which Microsoft CRT doesn't have... also ensures CH_NULL termination
*/

_Check_return_
extern int __cdecl
wstr_xsnprintf(
    _Out_writes_z_(dst_n) PWSTR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PCWSTR format,
    /**/        ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = wstr_xvsnprintf(dst, dst_n, format, args);
    va_end(args);

    return(ret);
}

_Check_return_
extern int __cdecl
wstr_xvsnprintf(
    _Out_writes_z_(dst_n) PWSTR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PCWSTR format,
    /**/        va_list args)
{
    int ret;

#if WINDOWS

    if(0 == dst_n)
        return(0);

    ret = _vsnwprintf_s(dst, dst_n, _TRUNCATE, format, args);

    if(-1 == ret) /* limit the answer */
        ret = wstrlen32(dst);

#else /* fallback implementation */

    STATUS status;
    U32 used;
    QUICK_WBLOCK quick_wblock;

    if(0 == dst_n)
        return(0);

    assert(0 == (1 & (uintptr_t) dst)); /* OK on x86 but not on ARM */
    assert(0 == (1 & (uintptr_t) format)); /* OK on x86 but not on ARM */

    quick_wblock_setup_without_clearing_wbuf(&quick_wblock, dst, dst_n); /* don't splurge on the remains of the buffer */

    status = quick_wblock_vprintf(&quick_wblock, format, args);

    /* have we overflowed the buffer and gone into handle allocation? NB if so, don't undo our good work!!! */
    if(0 != quick_wblock_array_handle_ref(&quick_wblock))
    {   /* copy as much stuff as possible back down before deleting the handle */
        quick_wblock_dispose_leaving_buffer_valid(&quick_wblock);
        used = dst_n;
    }
    else
        used = quick_wblock.wb_static_buffer_used;

    status_assert(status);

    /* ensure dst buffer is CH_NULL-terminated */
    if(dst_n == used)
        used -= 1; /* retract to make room */
    dst[used] = CH_NULL;

    ret = (int) used;

#endif /* OS */

    return(ret);
}

#if RISCOS || 1

#if 1
#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif
#define GetWchar(pwch) readval_U16(pwch) /* paranoia about unaligned data */
#define GetWcharOff(pwch, off) readval_U16(PtrAddBytes(PCWCH, (pwch), (off) * sizeof32(WCHAR))) /* paranoia about unaligned data */
#else
#define GetWchar(pwch) *(pwch)
#define GetWcharOff(pwch, off) (pwch)[(off)]
#endif

/******************************************************************************
*
* return length of WCHAR-based strings in WCHARs
*
******************************************************************************/

_Check_return_
extern size_t
wstrlen(
    _In_z_      PCWSTR wstr_in)
{
    PCWSTR wstr = wstr_in;

    assert(0 == (1 & (uintptr_t) wstr)); /* OK on x86 but not on ARM */

    for(;;)
    {
        const WCHAR this_wch = GetWchar(wstr);

        if(CH_NULL == this_wch)
            return((size_t) (wstr - wstr_in));

        ++wstr;
    }
}

/******************************************************************************
*
* return position of character in WCHAR-based string
*
******************************************************************************/

_Check_return_
extern PWSTR
wstrchr(
    _In_z_      PCWSTR wstr,
    _InVal_     WCHAR wch)
{
    assert(0 == (1 & (uintptr_t) wstr)); /* OK on x86 but not on ARM */

    for(;;)
    {
        const WCHAR this_wch = GetWchar(wstr);

        if(this_wch == wch) /* we can search for CH_NULL */
            return(de_const_cast(PWSTR, wstr));

        if(CH_NULL == this_wch)
            return(NULL);

        ++wstr;
    }
}

/******************************************************************************
*
* return position of one of a set of characters in WCHAR-based string
*
******************************************************************************/

_Check_return_
extern PWSTR
wstrpbrk(
    _In_z_      PCWSTR wstr,
    _In_z_      PCWSTR wstr_control_in)
{
    assert(0 == (1 & (uintptr_t) wstr)); /* OK on x86 but not on ARM */
    assert(0 == (1 & (uintptr_t) wstr_control_in)); /* OK on x86 but not on ARM */

    for(;;)
    {
        const WCHAR this_wch = GetWchar(wstr);
        PCWSTR wstr_control;

        for(wstr_control = wstr_control_in; ; ++wstr_control)
        {
            const WCHAR test_wch = GetWchar(wstr_control);

            if(CH_NULL == test_wch)
                break;

            if(this_wch == test_wch)
                return(de_const_cast(PWSTR, wstr));
        }

        if(CH_NULL == this_wch)
            return(NULL);

        ++wstr;
    }
}

/******************************************************************************
*
* case insensitive lexical comparison of two WCHAR-based strings
*
* must be lowercase compare in "C" locale
*
******************************************************************************/

_Check_return_
extern int
C_wcsicmp(
    _In_z_      PCWSTR wstr_a,
    _In_z_      PCWSTR wstr_b)
{
    int res;

    profile_ensure_frame();

    assert(0 == (1 & (uintptr_t) wstr_a)); /* OK on x86 but not on ARM */
    assert(0 == (1 & (uintptr_t) wstr_b)); /* OK on x86 but not on ARM */

    for(;;)
    {
        WCHAR c_a = GetWchar(wstr_a);
        WCHAR c_b = GetWchar(wstr_b);

        ++wstr_a;
        ++wstr_b;

        res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-2 range */

        if(0 != res)
        {   /* retry with case folding */
            if((c_a < 256) && (c_b < 256))
            {
                c_a = (WCHAR) /*"C"*/tolower(c_a); /* ASCII, no remapping */
                c_b = (WCHAR) /*"C"*/tolower(c_b);
            }

            res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-2 range */

            if(0 != res)
                return(res);
        }

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal */
    }
}

/******************************************************************************
*
* case insensitive lexical comparison of leading chars of two WCHAR-based strings
*
* must be lowercase compare in "C" locale
*
******************************************************************************/

_Check_return_
extern int
C_wcsnicmp(
    _In_z_/*wchars_n*/ PCWSTR wstr_a,
    _In_z_/*wchars_n*/ PCWSTR wstr_b,
    _InVal_     U32 wchars_n)
{
    int res;
    U32 wchars_n_remain = wchars_n;

    profile_ensure_frame();

    assert(0 == (1 & (uintptr_t) wstr_a)); /* OK on x86 but not on ARM */
    assert(0 == (1 & (uintptr_t) wstr_b)); /* OK on x86 but not on ARM */

    while(wchars_n_remain-- != 0)
    {
        WCHAR c_a = GetWchar(wstr_a);
        WCHAR c_b = GetWchar(wstr_b);

        ++wstr_a;
        ++wstr_b;

        res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-2 range */

        if(0 != res)
        {   /* retry with case folding */
            if((c_a < 256) && (c_b < 256))
            {
                c_a = (WCHAR) /*"C"*/tolower(c_a); /* ASCII, no remapping */
                c_b = (WCHAR) /*"C"*/tolower(c_b);
            }

            res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-2 range */

            if(0 != res)
                return(res);
        }

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal */
    }

    /* matched up to the comparison limit */

    return(0); /* span of n leading chars is equal */
}

#endif /* RISCOS */

/* end of xwstring.c */
