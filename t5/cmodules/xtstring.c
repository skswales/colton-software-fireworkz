/* xtstring.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Library module for TCHAR-based string handling */

/* SKS Nov 2006 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef          __xtstring_h
#include "cmodules/xtstring.h"
#endif

/******************************************************************************
*
* append to an aligator TCHAR-based string
*
******************************************************************************/

_Check_return_
extern STATUS
al_tstr_append(
    _InoutRef_  P_ARRAY_HANDLE_TSTR p_array_handle_tstr,
    _In_z_      PCTSTR tstr)
{
    const U32 tchars_n = tstrlen32p1(tstr); /*CH_NULL*/
    U32 nullch_char = 0;
    PTSTR tstr_wr;
    STATUS status;

    if(0 != array_elements32(p_array_handle_tstr))
        if(CH_NULL == *array_ptrc(p_array_handle_tstr, TCHAR, array_elements32(p_array_handle_tstr) - 1))
            nullch_char = 1;

    if(NULL == (tstr_wr = al_array_extend_by_TCHAR(p_array_handle_tstr, tchars_n - nullch_char, &array_init_block_tchar, &status)))
        return(status);

    tstr_wr -= nullch_char; /* retract pointer over the CH_NULL iff present */

    memcpy32(tstr_wr, tstr, tchars_n * sizeof32(*tstr));

    return(STATUS_OK);
}

/******************************************************************************
*
* do an aligator TCHAR-based string assignment
*
******************************************************************************/

_Check_return_
extern STATUS
al_tstr_set(
    _OutRef_    P_ARRAY_HANDLE_TSTR p_array_handle_tstr,
    _In_z_      PCTSTR tstr)
{
    const U32 tchars_n = tstrlen32p1(tstr); /*CH_NULL*/

    *p_array_handle_tstr = 0;

    return(al_array_add(p_array_handle_tstr, TCHAR, tchars_n, &array_init_block_tchar, tstr));
}

/******************************************************************************
*
* do a TCHAR-based string assignment
*
******************************************************************************/

_Check_return_
extern STATUS
tstr_set(
    _OutRef_    P_PTSTR aa,
    _In_opt_z_  PCTSTR tstr)
{
    STATUS status;
    PTSTR tstr_wr;
    U32 tchars_n;

    if(NULL == tstr)
    {
        *aa = NULL;
        return(STATUS_OK);
    }

    tchars_n = tstrlen32p1(tstr) /*CH_NULL*/;

    if(NULL == (*aa = tstr_wr = al_ptr_alloc_elem(TCHARZ, tchars_n, &status)))
        return(status);

    memcpy32(tstr_wr, tstr, tchars_n * sizeof32(*tstr));

    return(STATUS_DONE);
}

_Check_return_
extern STATUS
tstr_set_n(
    _OutRef_    P_PTSTR aa,
    _In_reads_opt_(tchars_n) PCTCH tchars,
    _InVal_     U32 tchars_n)
{
    STATUS status;
    PTSTR tstr_wr;

    if(0 == tchars_n)
    {
        *aa = NULL;
        return(STATUS_OK);
    }

    if(NULL == (*aa = tstr_wr = al_ptr_alloc_elem(TCHARZ, (tchars_n + 1 /*CH_NULL*/), &status)))
        return(status);

    if(NULL == tchars)
    {   /* NULL == tchars allows client to allocate for a string of tchars_n characters (and the CH_NULL) */
        tstr_wr[0] = CH_NULL; /* allows append e.g. tstr_xstrkat() */
        tstr_wr[tchars_n] = CH_NULL; /* in case client forgets it */
    }
    else
    {
        memcpy32(tstr_wr, tchars, tchars_n * sizeof32(*tchars));
        tstr_wr[tchars_n] = CH_NULL;
        assert(tchars_n <= tstrlen32(tstr_wr));
    }

    return(STATUS_DONE);
}

/******************************************************************************
*
* case sensitive lexical comparison of leading chars of two TCHAR-based strings
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
tstr_compare(
    _In_z_      PCTSTR tstr_a,
    _In_z_      PCTSTR tstr_b)
{
    return(tstr_compare_n2(tstr_a, strlen_without_NULLCH, tstr_b, strlen_without_NULLCH));
}

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
tstr_compare_equals(
    _In_z_      PCTSTR tstr_a,
    _In_z_      PCTSTR tstr_b)
{
    return(0 == tstr_compare_n2(tstr_a, strlen_without_NULLCH, tstr_b, strlen_without_NULLCH));
}

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
tstr_compare_n2(
    _In_/*tchars_n_a*/ PCTSTR tstr_a,
    _InVal_     U32 tchars_n_a /*strlen_with,without_NULLCH*/,
    _In_/*tchars_n_b*/ PCTSTR tstr_b,
    _InVal_     U32 tchars_n_b /*strlen_with,without_NULLCH*/)
{
    int res;
    U32 limit = MIN(tchars_n_a, tchars_n_b);
    U32 i;
    U32 remain_a, remain_b;

    profile_ensure_frame();

    /* no worry about limiting to strlen as this function demands CH_NULL-terminated strings */
    for(i = 0; i < limit; ++i)
    {
        int c_a = *tstr_a++;
        int c_b = *tstr_b++;

        res = c_a - c_b; /* NB can't overflow given valid UCS-2 range */

        if(0 != res)
            return(res);

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal */
    }

    /* matched up to the comparison limit */

    /* which string has the greater number of chars left over? */
    remain_a = tchars_n_a - limit;
    remain_b = tchars_n_b - limit;

    if(remain_a == remain_b)
        return(0); /* ended together at the specified finite lengths -> equal */

    /* sort out any string length residuals */
    assert((tchars_n_a != strlen_with_NULLCH) || (tchars_n_b == strlen_with_NULLCH)); /* an admixture wouldn't be useful */

    if(tchars_n_a >= strlen_without_NULLCH)
        remain_a = tstrlen32_n(tstr_a, tchars_n_a);

    if(tchars_n_b >= strlen_without_NULLCH)
        remain_b = tstrlen32_n(tstr_b, tchars_n_b);

    /*if(remain_a == remain_b)
        return(0);*/ /* ended together at the determined finite lengths -> equal */

    res = (int) remain_a - (int) remain_b;

    return(res);
}

/******************************************************************************
*
* case insensitive lexical comparison of leading chars of two TCHAR-based strings
*
* uses sbchar_sortbyte() for collation
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
tstr_compare_nocase(
    _In_z_      PCTSTR tstr_a,
    _In_z_      PCTSTR tstr_b)
{
    return(tstr_compare_n2_nocase(tstr_a, strlen_without_NULLCH, tstr_b, strlen_without_NULLCH));
}

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
tstr_compare_equals_nocase(
    _In_z_      PCTSTR tstr_a,
    _In_z_      PCTSTR tstr_b)
{
    return(0 == tstr_compare_n2_nocase(tstr_a, strlen_without_NULLCH, tstr_b, strlen_without_NULLCH));
}

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
tstr_compare_n2_nocase(
    _In_/*tchars_n_a*/ PCTSTR tstr_a,
    _InVal_     U32 tchars_n_a /*strlen_with,without_NULLCH*/,
    _In_/*tchars_n_b*/ PCTSTR tstr_b,
    _InVal_     U32 tchars_n_b /*strlen_with,without_NULLCH*/)
{
    int res;
    U32 limit = MIN(tchars_n_a, tchars_n_b);
    U32 i;
    U32 remain_a, remain_b;

    profile_ensure_frame();

    /* no worry about limiting to strlen as this function demands CH_NULL-terminated strings */
    for(i = 0; i < limit; ++i)
    {
        int c_a = *tstr_a++;
        int c_b = *tstr_b++;

        res = c_a - c_b; /* NB can't overflow given valid UCS-2 range */

        if(0 != res)
        {   /* retry with case folding */
#if TSTR_IS_SBSTR
            c_a = sbchar_sortbyte(c_a);
            c_b = sbchar_sortbyte(c_b);
#else
            if((c_a < 256) && (c_b < 256))
            {
                c_a = sbchar_sortbyte(c_a);
                c_b = sbchar_sortbyte(c_b);
            }
#endif /* TSTR_IS_SBSTR */

            res = c_a - c_b; /* NB can't overflow given valid UCS-2 range */

            if(0 != res)
                return(res);
        }

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal */
    }

    /* matched up to the comparison limit */

    /* which string has the greater number of chars left over? */
    remain_a = tchars_n_a - limit;
    remain_b = tchars_n_b - limit;

    if(remain_a == remain_b)
        return(0); /* ended together at the specified finite lengths -> equal */

    /* sort out any string length residuals */
    assert((tchars_n_a != strlen_with_NULLCH) || (tchars_n_b == strlen_with_NULLCH)); /* an admixture wouldn't be useful */

    if(tchars_n_a >= strlen_without_NULLCH)
        remain_a = tstrlen32_n(tstr_a, tchars_n_a);

    if(tchars_n_b >= strlen_without_NULLCH)
        remain_b = tstrlen32_n(tstr_b, tchars_n_b);

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
extern U32 /* TCHARs currently in output */
tstr_xstrkat(
    _Inout_updates_z_(dst_n) PTSTR dst,
    _InVal_     U32 dst_n,
    _In_z_      PCTSTR src)
{
#if TSTR_IS_SBSTR
    return(xstrkat(dst, dst_n, src));
#else
    U32 dst_idx = 0;
    TCHAR ch;

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

    return(dst_idx); /* TCHARs currently in output */
#endif /* TSTR_IS_SBSTR */
}

/*
append up to src_n characters from src (or fewer, if CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* TCHARs currently in output */
tstr_xstrnkat(
    _Inout_updates_z_(dst_n) PTSTR dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PCTCH src,
    _InVal_     U32 src_n)
{
#if TSTR_IS_SBSTR
    return(xstrnkat(dst, dst_n, src, src_n));
#else
    U32 dst_idx = 0;
    U32 src_idx = 0;
    TCHAR ch;

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

    return(dst_idx); /* TCHARs currently in output */
#endif /* TSTR_IS_SBSTR */
}

/*
copy characters from src (until CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* TCHARs currently in output */
tstr_xstrkpy(
    _Out_writes_z_(dst_n) PTSTR dst,
    _InVal_     U32 dst_n,
    _In_z_      PCTSTR src)
{
#if TSTR_IS_SBSTR
    return(xstrkpy(dst, dst_n, src));
#else
    U32 dst_idx = 0;
    TCHAR ch;

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

    return(dst_idx); /* TCHARs currently in output */
#endif /* TSTR_IS_SBSTR */
}

/*
copy up to src_n characters from src (or fewer, if CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* TCHARs currently in output */
tstr_xstrnkpy(
    _Out_writes_z_(dst_n) PTSTR dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PCTCH src,
    _InVal_     U32 src_n)
{
#if TSTR_IS_SBSTR
    return(xstrnkpy(dst, dst_n, src, src_n));
#else
    U32 dst_idx = 0;
    U32 src_idx = 0;
    TCHAR ch;

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

    return(dst_idx); /* TCHARs currently in output */
#endif /* TSTR_IS_SBSTR */
}

/*
a portable but inexact replacement for (v)snprintf(), which Microsoft CRT doesn't have... also ensures CH_NULL termination
*/

_Check_return_
extern int __cdecl
tstr_xsnprintf(
    _Out_writes_z_(dst_n) PTSTR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = tstr_xvsnprintf(dst, dst_n, format, args);
    va_end(args);

    return(ret);
}

_Check_return_
extern int __cdecl
tstr_xvsnprintf(
    _Out_writes_z_(dst_n) PTSTR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list args)
{
    int ret;

#if TSTR_IS_SBSTR

    ret = xvsnprintf(dst, dst_n, format, args);

#elif WINDOWS

    if(0 == dst_n)
        return(0);

    ret = _vsntprintf_s(dst, dst_n, _TRUNCATE, format, args);

    if(-1 == ret) /* limit the answer */
        ret = tstrlen32(dst);

#else /* fallback implementation */

    STATUS status;
    U32 used;
    QUICK_TBLOCK quick_tblock;

    if(0 == dst_n)
        return(0);

    quick_tblock_setup_without_tqb_fill(&quick_tblock, dst, dst_n); /* don't splurge on the remains of the buffer */

    status = quick_tblock_vprintf(&quick_tblock, format, args);

    /* have we overflowed the buffer and gone into handle allocation? NB if so, don't undo our good work!!! */
    if(0 != quick_tblock_array_handle_ref(&quick_tblock))
    {   /* copy as much stuff as possible back down before deleting the handle */
        quick_tblock_dispose_leaving_buffer_valid(&quick_tblock);
        used = dst_n;
    }
    else
        used = quick_tblock.tb_static_buffer_used;

    status_assert(status);

    /* ensure dst buffer is CH_NULL-terminated */
    if(dst_n == used)
        used -= 1; /* retract to make room */
    dst[used] = CH_NULL;

    ret = (int) used;

#endif /* TSTR_IS_SBSTR || OS */

    return(ret);
}

/*
low-lifetime conversion routines
*/

#if TSTR_IS_SBSTR

/* defined as macros as no conversion is required */

#else /* TSTR_IS_SBSTR */

/* host-specific implementation */

#if WINDOWS

static struct sbstr_from_tstr_statics
{
    PC_SBSTR last;
    U32 used;
    U8 buffer[4 * 1024];
}
sbstr_from_tstr_statics;

_Check_return_
_Ret_z_ /* never NULL */
extern PC_SBSTR /*low-lifetime*/
_sbstr_from_tstr(
    _In_z_      PCTSTR tstr)
{
    const UINT mbchars_CodePage = GetACP();
    P_U8Z dstptr;
    BOOL fUsedDefaultChar = FALSE;
    int multi_n;
    int pass = 1;

#if CHECKING
    if(NULL == tstr)
        return(("<<sbstr_from_tstr - NULL>>"));

    if(PTR_IS_NONE(tstr))
        return(("<<sbstr_from_tstr - NONE>>"));
#endif

    do {
        U32 avail = elemof32(sbstr_from_tstr_statics.buffer) - sbstr_from_tstr_statics.used; /* NB may be zero! */

#if CHECKING && 1
        if(avail < 200)
            avail = 0;
#endif

        dstptr = sbstr_from_tstr_statics.buffer + sbstr_from_tstr_statics.used;

        multi_n =
            WideCharToMultiByte(mbchars_CodePage, 0 /*dwFlags*/,
                                tstr, -1 /*strlen_with_NULLCH*/,
                                (PSTR) dstptr, (int) avail,
                                NULL /*lpDefaultChar*/, &fUsedDefaultChar);

        if((0 != avail) && (0 != multi_n))
        {
            assert(CH_NULL == PtrGetByteOff(dstptr, multi_n-1));
            assert(!fUsedDefaultChar);
            sbstr_from_tstr_statics.last = dstptr;
            break;
        }

        /* retry with whole buffer */
        sbstr_from_tstr_statics.used = 0;
    }
    while(++pass <= 2);

    if(multi_n > 0)
        sbstr_from_tstr_statics.used += multi_n;

    return(dstptr);
}

static struct tstr_from_sbstr_statics
{
    PCTSTR last;
    U32 used;
    TCHAR buffer[4 * 1024];
}
tstr_from_sbstr_statics;

_Check_return_
_Ret_z_ /* never NULL */
extern PCTSTR /*low-lifetime*/
_tstr_from_sbstr(
    _In_z_      PC_SBSTR sbstr)
{
    const UINT mbchars_CodePage = GetACP();
    PTSTR dstptr;
    int wchars_n;
    int pass = 1;

#if CHECKING
    if(NULL == sbstr)
        return(TEXT("<<tstr_from_sbstr - NULL>>"));

    if(PTR_IS_NONE(sbstr))
        return(TEXT("<<tstr_from_sbstr - NONE>>"));

    if(contains_inline(sbstr, strlen32(sbstr)))
    {
        assert0();
        return(TEXT("<<tstr_from_sbstr - CONTAINS INLINES>>"));
    }
#endif

    do  {
        U32 avail = elemof32(tstr_from_sbstr_statics.buffer) - tstr_from_sbstr_statics.used; /* NB may be zero! */

#if CHECKING && 1
        if(avail < 200)
            avail = 0;
#endif

        dstptr = tstr_from_sbstr_statics.buffer + tstr_from_sbstr_statics.used;

        wchars_n =
            MultiByteToWideChar(mbchars_CodePage, 0 /*dwFlags*/,
                                (PCSTR) sbstr, -1 /*strlen_with_NULLCH*/,
                                dstptr, (int) avail);

        if((0 != avail) && (0 != wchars_n))
        {
            assert(CH_NULL == dstptr[wchars_n-1]);
            tstr_from_sbstr_statics.last = dstptr;
            break;
        }

        /* retry with whole buffer */
        tstr_from_sbstr_statics.used = 0;
    }
    while(++pass <= 2);

    if(wchars_n > 0)
        tstr_from_sbstr_statics.used += wchars_n;

    return(dstptr);
}

#endif /* WINDOWS */

#endif /* TSTR_IS_SBSTR */

#if !TSTR_IS_SBSTR

/* defined as macros as no conversion is required */

#else /* TSTR_IS_SBSTR */

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

/* host-specific implementation */

#if WINDOWS /* seems to be no use case on RISC OS */

static struct tstr_from_wstr_statics
{
    PCTSTR last;
    U32 used;
    TCHAR buffer[4 * 1024];
}
tstr_from_wstr_statics;

_Check_return_
_Ret_z_ /* never NULL */
extern PCTSTR /*low-lifetime*/
_tstr_from_wstr(
    _In_z_      PCWSTR wstr)
{
#if WINDOWS
    const UINT mbchars_CodePage = GetACP();
#endif
    PTSTR dstptr;
    int multi_n;
    int pass = 1;

    assert(0 == (1 & (uintptr_t) wstr)); /* OK on x86 but not on ARM */

    do  {
        U32 avail = elemof32(tstr_from_wstr_statics.buffer) - tstr_from_wstr_statics.used; /* NB may be zero! */

#if CHECKING && 1
        if(avail < 200)
            avail = 0;
#endif

        dstptr = tstr_from_wstr_statics.buffer + tstr_from_wstr_statics.used;

#if WINDOWS
        multi_n =
            WideCharToMultiByte(mbchars_CodePage, 0 /*dwFlags*/,
                                wstr, -1 /*strlen_with_NULLCH*/,
                                dstptr, (int) avail,
                                NULL, NULL);

        if((0 != avail) && (0 != multi_n))
        {
            assert(CH_NULL == dstptr[multi_n-1]);
            tstr_from_wstr_statics.last = dstptr;
            break;
        }
#else
        {
        U32 offset = 0;

        for(;;)
        {
            const WCHAR this_wch = GetWcharOff(wstr, offset);

            if(offset <= avail)
                dstptr[offset] = ucs4_is_sbchar(this_wch) ? (TCHAR) this_wch : CH_QUESTION_MARK;

            ++offset;

            if(CH_NULL == this_wch)
            {
                multi_n = (int) offset; /*strlen_with_NULLCH*/
                break;
            }
        }
        } /*block*/

        if((U32) multi_n <= avail)
        {
            assert(CH_NULL == dstptr[multi_n-1]);
            tstr_from_wstr_statics.last = dstptr;
            break;
        }
#endif /* OS */

        /* retry with whole buffer */
        tstr_from_wstr_statics.used = 0;
    }
    while(++pass <= 2);

    if(multi_n > 0)
        tstr_from_wstr_statics.used += multi_n;

    return(dstptr);
}

#endif /* OS */

#if WINDOWS /* seems to be no use case on RISC OS */

static struct wstr_from_tstr_statics
{
    PCWSTR last;
    U32 used;
    WCHAR buffer[4 * 1024];
}
wstr_from_tstr_statics;

_Check_return_
_Ret_z_ /* never NULL */
extern PCWSTR /*low-lifetime*/
_wstr_from_tstr(
    _In_z_      PCTSTR tstr)
{
#if WINDOWS
    const UINT mbchars_CodePage = GetACP();
#endif
    PWSTR dstptr;
    int wchars_n;
    int pass = 1;

    do  {
        U32 avail = elemof32(wstr_from_tstr_statics.buffer) - wstr_from_tstr_statics.used; /* NB may be zero! */

        dstptr = wstr_from_tstr_statics.buffer + wstr_from_tstr_statics.used;

#if WINDOWS
        wchars_n =
            MultiByteToWideChar(mbchars_CodePage, 0 /*dwFlags*/,
                                tstr, -1 /*strlen_with_NULLCH*/,
                                dstptr, (int) avail);

        if((0 != avail) && (0 != wchars_n))
        {
            assert(CH_NULL == dstptr[wchars_n-1]);
            wstr_from_tstr_statics.last = dstptr;
            break;
        }
#else
        {
        U32 offset = 0;

        for(;;)
        {
            const TCHAR this_tch = tstr[offset];

            if(offset <= avail)
                dstptr[offset] = (WCHAR) this_tch;

            ++offset;

            if(CH_NULL == this_tch)
            {
                wchars_n = (int) offset; /*strlen_with_NULLCH*/
                break;
            }
        }
        } /*block*/

        if((U32) wchars_n <= avail)
        {
            assert(CH_NULL == dstptr[wchars_n-1]);
            wstr_from_tstr_statics.last = dstptr;
            break;
        }
#endif /* OS */

        /* retry with whole buffer */
        wstr_from_tstr_statics.used = 0;
    }
    while(++pass <= 2);

    if(wchars_n > 0)
        wstr_from_tstr_statics.used += wchars_n;

    return(dstptr);
}

#endif /* OS */

#endif /* TSTR_IS_SBSTR */

/* end of xtstring.c */
