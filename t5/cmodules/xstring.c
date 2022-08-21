/* xstring.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Library module for string handling */

/* SKS May 1990 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/******************************************************************************
*
* like bsearch but always return a pointer even if not perfect match
*
******************************************************************************/

_Check_return_
_Ret_writes_bytes_maybenull_(entry_size)
extern P_ANY
_bfind(
    _In_reads_bytes_(entry_size) PC_ANY key,
    _In_reads_bytes_x_(entry_size * n_entries) PC_ANY p_start,
    _InVal_     S32 n_entries,
    _InVal_     U32 entry_size,
    _InRef_     P_PROC_BSEARCH p_proc_bsearch,
    _OutRef_    P_BOOL p_hit)
{
    PC_ANY p_any = p_start;

    *p_hit = FALSE;

    if(n_entries)
    {
        S32 e = n_entries;
        S32 s = 0;
        int res = 0;

        for(;;)
        {
            S32 n = (e - s) / 2;
            S32 t = s + n;

            p_any = PtrAddBytes(PC_ANY, p_start, t * entry_size);

            if((res = p_proc_bsearch(key, p_any)) == 0)
            {
                *p_hit = TRUE;
                return(de_const_cast(P_ANY, p_any));
            }

            if(res > 0)
                s = t;
            else
                e = t;

            if(!n)
                break;
        }

        /* always return pointer to element to insert before */
        if(res > 0)
            p_any = PtrAddBytes(PC_ANY, p_any, entry_size);
    }

    return(de_const_cast(P_ANY, p_any));
}

/******************************************************************************
*
* like bsearch but always return a pointer even if not perfect match
*
******************************************************************************/

_Check_return_
_Ret_writes_bytes_maybenull_(entry_size)
extern P_ANY
xlsearch(
    _In_reads_bytes_(entry_size) PC_ANY key,
    _In_reads_bytes_x_(entry_size * n_entries) PC_ANY p_start,
    _InVal_     S32 n_entries,
    _InVal_     U32 entry_size,
    _InRef_     P_PROC_BSEARCH p_proc_bsearch)
{
    int res = 0;
    S32 t;

    for(t = 0; t < n_entries; ++t)
    {
        PC_ANY p_any = PtrAddBytes(PC_ANY, p_start, t * entry_size);

        if((res = p_proc_bsearch(key, p_any)) == 0)
            return(de_const_cast(P_ANY, p_any));
    }

    return(NULL);
}

extern void __cdecl /* declared as qsort replacement */
check_sorted(
    const void * base,
    size_t num,
    size_t width,
    int (__cdecl * p_proc_compare) (
        const void * a1,
        const void * a2))
{
    const char * p = (const char *) base;
    size_t i;
    assert((S32)num > 0);
    if(num > 1)
        for(i = 0; i < num - 1; ++i, p += width)
        {
            int res = (* p_proc_compare) (p, p + width);

            if(res == 0)
            {
                messagef(TEXT("check_sorted: table ") PTR_XTFMT TEXT(" has identical members at index %d, %d"), base, i, i + 1);
                break;
            }
            else if(res > 0)
            {
                messagef(TEXT("check_sorted: table ") PTR_XTFMT TEXT(" members compare incorrectly at index %d, %d"), base, i, i + 1);
                break;
            }
        }
}

/* strtol, strtoul without whitespace stripping and base rubbish */

_Check_return_
extern S32
fast_strtol(
    _In_z_      PC_U8Z p_u8_in,
    _OutRef_opt_ P_P_U8Z endptr)
{
    PC_U8Z p_u8 = p_u8_in;
    BOOL negative = FALSE;
    P_U8Z this_endptr;
    U32 u32;

    switch(*p_u8)
    {
    case CH_PLUS_SIGN:  ++p_u8;                  break;
    case CH_MINUS_SIGN__BASIC: ++p_u8; negative = TRUE; break;
    default:                                     break;
    }

    u32 = fast_strtoul(p_u8, &this_endptr);

    if(endptr)
        *endptr = ((PC_U8Z) this_endptr != p_u8) ? this_endptr : (P_U8Z) p_u8_in;

    if(u32 > S32_MAX)
    {
        errno = ERANGE;
        u32 = S32_MAX;
    }

    if(negative)
        return(- (S32) u32);

    return(+ (S32) u32);
}

_Check_return_
extern U32
fast_strtoul(
    _In_z_      PC_U8Z p_u8_in,
    _OutRef_opt_ P_P_U8Z endptr)
{
    PC_U8Z p_u8 = p_u8_in;
    int c = *p_u8++;
    BOOL ok = FALSE;
    BOOL overflowed = FALSE;
    U32 hi = 0;
    U32 lo = 0;

    profile_ensure_frame();

    for(;;)
    {
        int digit;
        if(!sbchar_isdigit(c))
            break;
        digit = c - CH_DIGIT_ZERO;
        lo *= 10;
        lo += digit;
        hi *= 10;
        hi += (lo >> 16);
        lo &= 0x0000FFFFU;
        if(hi >= 0x000010000U)
            overflowed = TRUE;
        ok = TRUE;
        c = *p_u8++;
    }

    if(endptr)
        *endptr = ok ? (P_U8Z) (p_u8 - 1) : (P_U8Z) p_u8_in;

    if(overflowed)
    {
        errno = ERANGE;
        return(U32_MAX);
    }

    return((hi << 16) | lo);
}

/******************************************************************************
*
* find first occurence of b in a
*
******************************************************************************/

_Check_return_
_Ret_writes_bytes_maybenull_(size_b)
extern P_ANY
memstr32(
    _In_reads_bytes_(size_a) PC_ANY a,
    _InVal_     U32 size_a,
    _In_reads_bytes_(size_b) PC_ANY b,
    _InVal_     U32 size_b)
{
    if(size_a >= size_b)
    {
        U32 n = size_a - size_b;
        U32 i;

        for(i = 0; i <= n; ++i) /* always at least one comparison to do */
        {
            PC_BYTE t = ((PC_BYTE) a) + i;

            if(0 == memcmp32(t, b, size_b))
                return((P_ANY) de_const_cast(P_BYTE, t));
        }
    }

    return(NULL);
}

#if RISCOS
#define SWAPTYPE U32
#elif WINDOWS
#define SWAPTYPE U32
#endif

#ifdef SWAPTYPE
typedef SWAPTYPE * P_SWAPTYPE;
#endif

extern void
memswap32(
    _Inout_updates_bytes_(n_bytes) P_ANY p1,
    _Inout_updates_bytes_(n_bytes) P_ANY p2,
    _InVal_     U32 n_bytes)
{
    U32 n_bytes_remaining = n_bytes;
    P_U8 c1;
    P_U8 c2;
    U8 c;

    profile_ensure_frame();

#ifdef SWAPTYPE
    { /* copy aligned words at a time if possible */
    P_SWAPTYPE s1 = (P_SWAPTYPE) p1;
    P_SWAPTYPE s2 = (P_SWAPTYPE) p2;
    SWAPTYPE s;

    if( (n_bytes_remaining > sizeof32(s))                &&
        (((uintptr_t) s1 & (sizeof32(SWAPTYPE)-1)) == 0) &&
        (((uintptr_t) s2 & (sizeof32(SWAPTYPE)-1)) == 0) )
    {
        do  {
             s    = *s1;
            *s1++ = *s2;
            *s2++ =  s;
            n_bytes_remaining -= sizeof32(s);
        }
        while(n_bytes_remaining > sizeof32(s));
    }

    c1 = (P_U8) s1;
    c2 = (P_U8) s2;
    } /*block*/
#else
    c1 = p1;
    c2 = p2;
#endif

    while(n_bytes_remaining-- != 0)
    {
         c    = *c1;
        *c1++ = *c2;
        *c2++ =  c;
    }
}

#ifdef SWAPTYPE
#undef SWAPTYPE
#endif

/*
reverse an array
*/

extern void
memrev32(
    _Inout_updates_bytes_x_(n_elements * element_width) P_ANY p,
    _InVal_     U32 n_elements,
    _InVal_     U32 element_width)
{
    P_BYTE a = (P_BYTE) p;
    P_BYTE b = a + (n_elements * element_width); /* point beyond the last element */
    const U32 half_n_elements = n_elements / 2; /* only need to go halfway */
    U32 i;

    for(i = 0; i < half_n_elements; ++i)
    {
        b -= element_width;
        memswap32(a, b, element_width);
        a += element_width;
    }
}

#if !WINDOWS

/******************************************************************************
*
* case insensitive lexical comparison of two strings
*
* must be lowercase compare in "C" locale
*
******************************************************************************/

_Check_return_
extern int
C_stricmp(
    _In_z_      PC_U8Z a,
    _In_z_      PC_U8Z b)
{
    int res;

    profile_ensure_frame();

    for(;;)
    {
        int c_a = *a++;
        int c_b = *b++;

        res = c_a - c_b;

        if(0 != res)
        {   /* retry with case folding */
            c_a = /*"C"*/tolower(c_a); /* ASCII, no remapping */
            c_b = /*"C"*/tolower(c_b);

            res = c_a - c_b;

            if(0 != res)
                return(res);
        }

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator -> equal */
    }
}

/******************************************************************************
*
* case insensitive lexical comparison of leading chars of two strings
*
* must be lowercase compare in "C" locale
*
******************************************************************************/

_Check_return_
extern int
C_strnicmp(
    _In_z_/*n*/ PC_U8Z a,
    _In_z_/*n*/ PC_U8Z b,
    _InVal_     U32 n)
{
    int res;
    U32 n_remain = n;

    profile_ensure_frame();

    while(n_remain-- != 0)
    {
        int c_a = *a++;
        int c_b = *b++;

        res = c_a - c_b;

        if(0 != res)
        {   /* retry with case folding */
            c_a = /*"C"*/tolower(c_a); /* ASCII, no remapping */
            c_b = /*"C"*/tolower(c_b);

            res = c_a - c_b;

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

/******************************************************************************
*
* case sensitive (binary) comparison of leading chars of two SBCHAR strings
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
sbstr_compare(
    _In_z_      PC_SBSTR sbstr_a,
    _In_z_      PC_SBSTR sbstr_b)
{
    return(sbstr_compare_n2(sbstr_a, strlen_without_NULLCH, sbstr_b, strlen_without_NULLCH));
}

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
sbstr_compare_equals(
    _In_z_      PC_SBSTR sbstr_a,
    _In_z_      PC_SBSTR sbstr_b)
{
    return(0 == sbstr_compare_n2(sbstr_a, strlen_without_NULLCH, sbstr_b, strlen_without_NULLCH));
}

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
sbstr_compare_n2(
    _In_z_/*sbchars_n_a*/ PC_SBSTR sbstr_a,
    _InVal_     U32 sbchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_/*sbchars_n_b*/ PC_SBSTR sbstr_b,
    _InVal_     U32 sbchars_n_b /*strlen_with,without_NULLCH*/)
{
    int res;
    U32 limit = MIN(sbchars_n_a, sbchars_n_b);
    U32 i;
    U32 remain_a, remain_b;

    profile_ensure_frame();

    /* no worry about limiting to strlen as this function demands CH_NULL-terminated strings */
    for(i = 0; i < limit; ++i)
    {
        int c_a = *sbstr_a++;
        int c_b = *sbstr_b++;

        res = c_a - c_b;

        if(0 != res)
            return(res);

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal */
    }

    /* matched up to the comparison limit */

    /* which string has the greater number of chars left over? */
    remain_a = sbchars_n_a - limit;
    remain_b = sbchars_n_b - limit;

    if(remain_a == remain_b)
        return(0); /* ended together at the specified finite lengths -> equal */

    /* sort out any string length residuals */
    assert((sbchars_n_a != strlen_with_NULLCH) || (sbchars_n_b == strlen_with_NULLCH)); /* an admixture wouldn't be useful */

    if(sbchars_n_a >= strlen_without_NULLCH)
        remain_a = strlen32_n(sbstr_a, sbchars_n_a);

    if(sbchars_n_b >= strlen_without_NULLCH)
        remain_a = strlen32_n(sbstr_b, sbchars_n_b);

    /*if(remain_a == remain_b)
        return(0);*/ /* ended together at the determined finite lengths -> equal */

    res = (int) remain_a - (int) remain_b;

    return(res);
}

/******************************************************************************
*
* case insensitive lexical comparison of leading chars of two SBCHAR (U8 Latin-N) strings
*
* uses sbchar_sortbyte() for collation
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
sbstr_compare_nocase(
    _In_z_      PC_SBSTR sbstr_a,
    _In_z_      PC_SBSTR sbstr_b)
{
    return(sbstr_compare_n2_nocase(sbstr_a, strlen_without_NULLCH, sbstr_b, strlen_without_NULLCH));
}

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
sbstr_compare_equals_nocase(
    _In_z_      PC_SBSTR sbstr_a,
    _In_z_      PC_SBSTR sbstr_b)
{
    return(0 == sbstr_compare_n2_nocase(sbstr_a, strlen_without_NULLCH, sbstr_b, strlen_without_NULLCH));
}

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
sbstr_compare_n2_nocase(
    _In_z_/*sbchars_n_a*/ PC_SBSTR sbstr_a,
    _InVal_     U32 sbchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_/*sbchars_n_b*/ PC_SBSTR sbstr_b,
    _InVal_     U32 sbchars_n_b /*strlen_with,without_NULLCH*/)
{
    int res;
    U32 limit = MIN(sbchars_n_a, sbchars_n_b);
    U32 i;
    U32 remain_a, remain_b;

    profile_ensure_frame();

    /* no worry about limiting to strlen as this function demands CH_NULL-terminated strings */
    for(i = 0; i < limit; ++i)
    {
        int c_a = *sbstr_a++;
        int c_b = *sbstr_b++;

        res = c_a - c_b;

        if(0 != res)
        {   /* retry with case folding */
            c_a = sbchar_sortbyte(c_a);
            c_b = sbchar_sortbyte(c_b);

            res = c_a - c_b;

            if(0 != res)
                return(res);
        }

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal */
    }

    /* matched up to the comparison limit */

    /* which string has the greater number of chars left over? */
    remain_a = sbchars_n_a - limit;
    remain_b = sbchars_n_b - limit;

    if(remain_a == remain_b)
        return(0); /* ended together at the specified finite lengths -> equal */

    /* sort out any string length residuals */
    assert((sbchars_n_a != strlen_with_NULLCH) || (sbchars_n_b == strlen_with_NULLCH)); /* an admixture wouldn't be useful */

    if(sbchars_n_a >= strlen_without_NULLCH)
        remain_a = strlen32_n(sbstr_a, sbchars_n_a);

    if(sbchars_n_b >= strlen_without_NULLCH)
        remain_a = strlen32_n(sbstr_b, sbchars_n_b);

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
extern U32 /* bytes currently in output */
xstrkat(
    _Inout_updates_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_U8Z src)
{
    U32 dst_idx = 0;
    U8 ch;

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

    return(dst_idx); /* bytes currently in output */
}

/*
append up to src_n characters from src (or fewer, if CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* bytes currently in output */
xstrnkat(
    _Inout_updates_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PC_U8 src,
    _InVal_     U32 src_n)
{
    U32 dst_idx = 0;
    U32 src_idx = 0;
    U8 ch;

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

    return(dst_idx); /* bytes currently in output */
}

/*
copy characters from src (until CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* bytes currently in output */
xstrkpy(
    _Out_writes_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_U8Z src)
{
    U32 dst_idx = 0;
    U8 ch;

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

    return(dst_idx); /* bytes currently in output */
}

/*
copy up to src_n characters from src (or fewer, if CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* bytes currently in output */
xstrnkpy(
    _Out_writes_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PC_U8 src,
    _InVal_     U32 src_n)
{
    U32 dst_idx = 0;
    U32 src_idx = 0;
    U8 ch;

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

    return(dst_idx); /* bytes currently in output */
}

/*
a portable but inexact replacement for (v)snprintf(), which Microsoft CRT doesn't have... also ensures CH_NULL termination
*/

_Check_return_
extern int __cdecl
xsnprintf(
    _Out_writes_z_(dst_n) char * dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ const char * format,
    /**/        ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = xvsnprintf(dst, dst_n, format, args);
    va_end(args);

    return(ret);
}

_Check_return_
extern int __cdecl
xvsnprintf(
    _Out_writes_z_(dst_n) char * dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ const char * format,
    /**/        va_list args)
{
    int ret;

#if WINDOWS

    if(0 == dst_n)
        return(0);

    ret = _vsnprintf_s(dst, dst_n, _TRUNCATE, format, args);

    if(-1 == ret) /* limit the answer */
        ret = strlen32(dst);

#elif 1 /* C99 CRT */

    if(0 == dst_n)
        return(0);

    ret = vsnprintf(dst, dst_n, format, args);

    if(ret >= (int) dst_n) /* limit the answer */
        ret = strlen32(dst);

#else /* fallback implementation */

    STATUS status;
    U32 used;
    QUICK_BLOCK quick_block;

    if(0 == dst_n)
        return(0);

    quick_block_setup_without_aqb_fill(&quick_block, dst, dst_n); /* don't splurge on the remains of the buffer */

    status = quick_block_vprintf(&quick_block, format, args);

    /* have we overflowed the buffer and gone into handle allocation? NB if so, don't undo our good work!!! */
    if(0 != quick_block_array_handle_ref(&quick_block))
    {   /* copy as much stuff as possible back down before deleting the handle */
        quick_block_dispose_leaving_buffer_valid(&quick_block);
        used = dst_n;
    }
    else
        used = quick_block.static_buffer_used;

    status_assert(status);

    /* ensure dst buffer is CH_NULL-terminated */
    if(dst_n == used)
        used -= 1; /* retract to make room */
    dst[used] = CH_NULL;

    ret = (int) used;

#endif /* OS */

    return(ret);
}

/* end of xstring.c */
