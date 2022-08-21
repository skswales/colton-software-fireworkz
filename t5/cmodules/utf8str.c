/* utf8str.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2006-2019 Stuart Swales */

/* Library module for UTF-8 character string handling */

/* SKS Oct 2006 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef          __utf8_h
#include "cmodules/utf8.h"
#endif

#ifndef          __utf8str_h
#include "cmodules/utf8str.h"
#endif

#if CHECKING_UTF8
static void
_s_raise(
    _In_        errno_t e)
{
    (void) (e);
}
#endif

/******************************************************************************
*
* return the current UCS-4 character in a UTF-8 character string,
* returning pointer to the start of the next character encoding.
*
******************************************************************************/

_Check_return_
extern UCS4
utf8str_char_next(
    _In_z_      PC_UTF8STR ustr,
    _OutRef_    P_PC_UTF8STR p_ustr_next)
{
    U32 bytes_of_char;
    UCS4 ucs4 = utf8str_char_decode(ustr, /*ref*/bytes_of_char);

    /* return pointer to byte at start of the next UTF-8 sequence */
    /*RETURN_VALUE_IF_NULL_OR_BAD_POINTER(p_ustr_next, 0)*/
    if(CH_NULL == ucs4) /* don't wander beyond the end of the string */
        *p_ustr_next = ustr;
    else
        *p_ustr_next = utf8str_AddBytes(ustr, bytes_of_char);

    return(ucs4);
}

/******************************************************************************
*
* validate a UTF-8 character string
*
* variant including inline processing may be used
* e.g. needed for recursive construct processing during file load
*
******************************************************************************/

_Check_return_
extern STATUS /* STATUS_OK, error if not */
utf8str_validate(
    _In_z_      PCTSTR func,
    _In_z_      PC_UTF8STR ustr)
{
    return(utf8str_validate_n(func, ustr, strlen_without_NULLCH));
}

_Check_return_
extern STATUS /* STATUS_OK, error if not */
utf8str_validate_n(
    _In_z_      PCTSTR func,
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/)
{
    return(utf8_validate(func, ustr, utf8str_strlen32_n(ustr, uchars_n)));
}

#if !defined(ustr_inline_validate)

_Check_return_
extern STATUS /* STATUS_OK, error if not */
ustr_inline_validate(
    _In_z_      PCTSTR func,
    _In_z_      PC_USTR_INLINE ustr_inline)
{
    return(ustr_inline_validate_n(func, ustr_inline, strlen_with_NULLCH));
}

_Check_return_
extern STATUS /* STATUS_OK, error if not */
ustr_inline_validate_n(
    _In_z_      PCTSTR func,
    _In_z_      PC_USTR_INLINE ustr_inline,
    _In_        U32 uchars_n /*strlen_with,without_NULLCH*/)
{
    if(strlen_without_NULLCH == uchars_n)
        uchars_n = ustr_inline_strlen32(ustr_inline);
    else if(strlen_with_NULLCH == uchars_n)
        uchars_n = ustr_inline_strlen32p1(ustr_inline);
    else
        assert(uchars_n <= ustr_inline_strlen32p1(ustr_inline));

    return(uchars_inline_validate(func, ustr_inline, uchars_n));
}

#endif /* !defined(ustr_inline_validate) */

/******************************************************************************
*
* case sensitive (binary) comparison of two UTF-8 CH_NULL-terminated strings
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
utf8str_compare(
    _In_z_      PC_UTF8STR ustr_a,
    _In_z_      PC_UTF8STR ustr_b)
{
    return(utf8str_compare_n2(ustr_a, strlen_with_NULLCH, ustr_b, strlen_with_NULLCH));
}

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
utf8str_compare_equals(
    _In_z_      PC_UTF8STR ustr_a,
    _In_z_      PC_UTF8STR ustr_b)
{
    return(0 == utf8str_compare_n2(ustr_a, strlen_with_NULLCH, ustr_b, strlen_with_NULLCH));
}

/******************************************************************************
*
* case sensitive (binary) comparison of the leading bytes of two UTF-8 CH_NULL-terminated strings
*
******************************************************************************/

#if TRACE_ALLOWED && 1
_Check_return_
static int
_utf8str_compare_n2(
    _In_z_      PC_UTF8STR ustr_a_in,
    _InVal_     U32 uchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_      PC_UTF8STR ustr_b_in,
    _InVal_     U32 uchars_n_b /*strlen_with,without_NULLCH*/);

_Check_return_
extern int
utf8str_compare_n2(
    _In_z_      PC_UTF8STR ustr_a,
    _InVal_     U32 uchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_      PC_UTF8STR ustr_b,
    _InVal_     U32 uchars_n_b /*strlen_with,without_NULLCH*/)
{
    int res = _utf8str_compare_n2(ustr_a, uchars_n_a, ustr_b, uchars_n_b);

    {
    TCHARZ tstr_buf_a[256];
    TCHARZ tstr_buf_b[256];

    U32 n_tstr_buf_a = tstr_buf_from_utf8str(tstr_buf_a, elemof32(tstr_buf_a), ustr_a, uchars_n_a);
    U32 n_tstr_buf_b = tstr_buf_from_utf8str(tstr_buf_b, elemof32(tstr_buf_b), ustr_b, uchars_n_b);

    tracef(TRACE_ANY, TEXT("utf8str_compare_n2 %.*s(%d) %c %.*s(%d)"),
           (int) n_tstr_buf_a, tstr_buf_a, (int) uchars_n_a,
           ((res > 0) ? CH_GREATER_THAN_SIGN : (res == 0) ? CH_EQUALS_SIGN : CH_LESS_THAN_SIGN),
           (int) n_tstr_buf_b, tstr_buf_b, (int) uchars_n_b);
    } /*block*/

    return(res);
}

_Check_return_
static int
_utf8str_compare_n2(
    _In_z_      PC_UTF8STR ustr_a_in,
    _InVal_     U32 uchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_      PC_UTF8STR ustr_b_in,
    _InVal_     U32 uchars_n_b /*strlen_with,without_NULLCH*/)
#else
_Check_return_
extern int
utf8str_compare_n2(
    _In_z_      PC_UTF8STR ustr_a_in,
    _InVal_     U32 uchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_      PC_UTF8STR ustr_b_in,
    _InVal_     U32 uchars_n_b /*strlen_with,without_NULLCH*/)
#endif
{
    int res;
    PC_UTF8STR ustr_a = ustr_a_in;
    PC_UTF8STR ustr_b = ustr_b_in;
#if 1
    PC_U8Z u8str_a = (PC_U8) ustr_a_in;
    PC_U8Z u8str_b = (PC_U8) ustr_b_in;
    U32 limit = MIN(uchars_n_a, uchars_n_b);
    U32 i;
#else
    U32 i_a = 0;
    U32 i_b = 0;
#endif
    U32 remain_a, remain_b;

    profile_ensure_frame();

#if CHECKING_UTF8
    if(status_fail(utf8str_validate_n(TEXT("utf8str_compare_n2 arga"), ustr_a_in, uchars_n_a))) return(0);
    if(status_fail(utf8str_validate_n(TEXT("utf8str_compare_n2 argb"), ustr_b_in, uchars_n_b))) return(0);
#endif

#if 1 /* NB UTF-8 encoding has the very useful property that simple binary comparison works !!! */
    /* no worry about limiting to strlen as this function demands CH_NULL-terminated strings */
    for(i = 0; i < limit; ++i)
    {
        U8 c_a = *u8str_a++;
        U8 c_b = *u8str_b++;

        res = (int) c_a - (int) c_b;

        if(0 != res)
            return(res);

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal */
    }

    /* matched up to the comparison limit */

    /* which string has the greater number of bytes left over? */
    remain_a = uchars_n_a - limit;
    remain_b = uchars_n_b - limit;

    ustr_a = (PC_UTF8STR) u8str_a;
    ustr_b = (PC_UTF8STR) u8str_b;
#else
    /* no worry about limiting to strlen as this function demands CH_NULL-terminated strings */
    while((i_a < uchars_n_a) && (i_b < uchars_n_b))
    {
        UCS4 c_a = utf8str_char_next(ustr_a, &ustr_a);
        UCS4 c_b = utf8str_char_next(ustr_b, &ustr_b);

        i_a = PtrDiffBytesU32(ustr_a, ustr_a_in);
        i_b = PtrDiffBytesU32(ustr_b, ustr_b_in);

        assert(i_a <= uchars_n_a);
        assert(i_b <= uchars_n_b);

        res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-4 range */

        if(0 != res)
            return(res);

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal */
    }

    /* matched up to the comparison limit */

    /* which string has the greater number of bytes left over? */
    remain_a = uchars_n_a - i_a;
    remain_b = uchars_n_b - i_b;
#endif /* UTF-8 binary comparison */

    if(remain_a == remain_b)
        return(0); /* ended together at the specified finite lengths -> equal */

    /* sort out any string length residuals */
    assert((uchars_n_a != strlen_with_NULLCH) || (uchars_n_b == strlen_with_NULLCH)); /* an admixture wouldn't be useful */

    if(uchars_n_a >= strlen_without_NULLCH)
        remain_a = utf8str_strlen32_n(ustr_a, uchars_n_a);

    if(uchars_n_b >= strlen_without_NULLCH)
        remain_b = utf8str_strlen32_n(ustr_b, uchars_n_b);

    /*if(remain_a == remain_b)
        return(0);*/ /* ended together at the determined finite lengths -> equal */

    res = (int) remain_a - (int) remain_b;

    return(res);
}

/******************************************************************************
*
* case insensitive lexical comparison of two UTF-8 CH_NULL-terminated strings
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
utf8str_compare_nocase(
    _In_z_      PC_UTF8STR ustr_a,
    _In_z_      PC_UTF8STR ustr_b)
{
    return(utf8str_compare_n2_nocase(ustr_a, strlen_without_NULLCH, ustr_b, strlen_without_NULLCH));
}

/******************************************************************************
*
* case insensitive lexical comparison of the leading bytes of two UTF-8 CH_NULL-terminated strings
*
******************************************************************************/

#if TRACE_ALLOWED && 1
_Check_return_
static int
_utf8str_compare_n2_nocase(
    _In_z_      PC_UTF8STR ustr_a,
    _InVal_     U32 uchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_      PC_UTF8STR ustr_b,
    _InVal_     U32 uchars_n_b /*strlen_with,without_NULLCH*/);

_Check_return_
extern int
utf8str_compare_n2_nocase(
    _In_z_      PC_UTF8STR ustr_a,
    _InVal_     U32 uchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_      PC_UTF8STR ustr_b,
    _InVal_     U32 uchars_n_b /*strlen_with,without_NULLCH*/)
{
    int res = _utf8str_compare_n2_nocase(ustr_a, uchars_n_a, ustr_b, uchars_n_b);

    {
    TCHARZ tstr_buf_a[256];
    TCHARZ tstr_buf_b[256];

    U32 n_tstr_buf_a = tstr_buf_from_utf8str(tstr_buf_a, elemof32(tstr_buf_a), ustr_a, uchars_n_a);
    U32 n_tstr_buf_b = tstr_buf_from_utf8str(tstr_buf_b, elemof32(tstr_buf_b), ustr_b, uchars_n_b);

    tracef(TRACE_ANY, TEXT("utf8str_compare_n2_nocase %.*s(%d) %c %.*s(%d)"),
           (int) n_tstr_buf_a, tstr_buf_a, (int) uchars_n_a,
           ((res > 0) ? CH_GREATER_THAN_SIGN : (res == 0) ? CH_EQUALS_SIGN : CH_LESS_THAN_SIGN),
           (int) n_tstr_buf_b, tstr_buf_b, (int) uchars_n_b);
    } /*block*/

    return(res);
}

_Check_return_
static int
_utf8str_compare_n2_nocase(
    _In_z_      PC_UTF8STR ustr_a,
    _InVal_     U32 uchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_      PC_UTF8STR ustr_b,
    _InVal_     U32 uchars_n_b /*strlen_with,without_NULLCH*/)
#else
_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
utf8str_compare_n2_nocase(
    _In_z_      PC_UTF8STR ustr_a,
    _InVal_     U32 uchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_      PC_UTF8STR ustr_b,
    _InVal_     U32 uchars_n_b /*strlen_with,without_NULLCH*/)
#endif
{
    int res;
    U32 i_a = 0;
    U32 i_b = 0;
    U32 remain_a, remain_b;

    profile_ensure_frame();

#if CHECKING_UTF8
    if(status_fail(utf8str_validate_n(TEXT("utf8str_compare_n2_nocase arga"), ustr_a, uchars_n_a))) return(0);
    if(status_fail(utf8str_validate_n(TEXT("utf8str_compare_n2_nocase argb"), ustr_b, uchars_n_b))) return(0);
#endif

    /* no worry about limiting to strlen as this function demands CH_NULL-terminated strings */
    while((i_a < uchars_n_a) && (i_b < uchars_n_b))
    {
        PC_UTF8 uchars_a = utf8_AddBytes(ustr_a, i_a);
        U32 bytes_of_char_a, bytes_of_grapheme_cluster_a;
        UCS4 c_a = utf8_grapheme_cluster_decode(uchars_a, uchars_n_a - i_a, /*ref*/bytes_of_char_a, /*ref*/bytes_of_grapheme_cluster_a);

        PC_UTF8 uchars_b = utf8_AddBytes(ustr_b, i_b);
        U32 bytes_of_char_b, bytes_of_grapheme_cluster_b;
        UCS4 c_b = utf8_grapheme_cluster_decode(uchars_b, uchars_n_b - i_b, /*ref*/bytes_of_char_b, /*ref*/bytes_of_grapheme_cluster_b);

        i_a += bytes_of_grapheme_cluster_a;
        i_b += bytes_of_grapheme_cluster_b;

        assert(i_a <= uchars_n_a);
        assert(i_b <= uchars_n_b);

        res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-4 range */

        if(0 != res)
        {   /* retry with case folding */
            c_a = ucs4_case_fold_simple(c_a);
            c_b = ucs4_case_fold_simple(c_b);

            res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-4 range */

            if(0 != res)
            {   /* a pedant writes - here we need to consider sort order! */
                /* but for now this will have to do... */
                return(res);
            }
        }

        if(CH_NULL == c_a)
            return(0); /* ended together at the terminator (before limit) -> equal (even if matched length differs) */
    }

    /* matched up to the comparison limit */

    /* which string has the greater number of bytes left over? */
    remain_a = uchars_n_a - i_a;
    remain_b = uchars_n_b - i_b;

    if(remain_a == remain_b)
        return(0); /* ended together at the specified finite lengths -> equal (even if matched length differs) */

    /* sort out any string length residuals */
    assert((uchars_n_a != strlen_with_NULLCH) || (uchars_n_b == strlen_with_NULLCH)); /* an admixture wouldn't be useful */

    if(uchars_n_a >= strlen_without_NULLCH)
        remain_a = utf8str_strlen32_n(utf8str_AddBytes(ustr_a, i_a), uchars_n_a);

    if(uchars_n_b >= strlen_without_NULLCH)
        remain_b = utf8str_strlen32_n(utf8str_AddBytes(ustr_b, i_b), uchars_n_b);

    /*if(remain_a == remain_b)
        return(0);*/ /* ended together at the determined finite lengths -> equal (even if matched length differs) */

    res = (int) remain_a - (int) remain_b;

    return(res);
}

/*
simpler variants that don't require result to reflect sort order, just inequality
*/

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
utf8str_compare_equals_nocase(
    _In_z_      PC_UTF8STR ustr_a,
    _In_z_      PC_UTF8STR ustr_b)
{
    return(utf8str_compare_equals_n2_nocase(ustr_a, strlen_without_NULLCH, ustr_b, strlen_without_NULLCH));
}

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
utf8str_compare_equals_n2_nocase(
    _In_z_      PC_UTF8STR ustr_a,
    _InVal_     U32 uchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_      PC_UTF8STR ustr_b,
    _InVal_     U32 uchars_n_b /*strlen_with,without_NULLCH*/)
{
    BOOL res;
    U32 i_a = 0;
    U32 i_b = 0;
    U32 remain_a, remain_b;

    profile_ensure_frame();

#if CHECKING_UTF8
    if(status_fail(utf8str_validate_n(TEXT("utf8str_compare_equals_n2_nocase arga"), ustr_a, uchars_n_a))) return(0);
    if(status_fail(utf8str_validate_n(TEXT("utf8str_compare_equals_n2_nocase argb"), ustr_b, uchars_n_b))) return(0);
#endif

    /* no worry about limiting to strlen as this function demands CH_NULL-terminated strings */
    while((i_a < uchars_n_a) && (i_b < uchars_n_b))
    {
        PC_UTF8 uchars_a = utf8_AddBytes(ustr_a, i_a);
        U32 bytes_of_char_a, bytes_of_grapheme_cluster_a;
        UCS4 c_a = utf8_grapheme_cluster_decode(uchars_a, uchars_n_a - i_a, /*ref*/bytes_of_char_a, /*ref*/bytes_of_grapheme_cluster_a);

        PC_UTF8 uchars_b = utf8_AddBytes(ustr_b, i_b);
        U32 bytes_of_char_b, bytes_of_grapheme_cluster_b;
        UCS4 c_b = utf8_grapheme_cluster_decode(uchars_b, uchars_n_b - i_b, /*ref*/bytes_of_char_b, /*ref*/bytes_of_grapheme_cluster_b);

        i_a += bytes_of_grapheme_cluster_a;
        i_b += bytes_of_grapheme_cluster_b;

        assert(i_a <= uchars_n_a);
        assert(i_b <= uchars_n_b);

        res = (c_a != c_b);

        if(!res)
        {   /* retry with case folding */
            c_a = ucs4_case_fold_simple(c_a);
            c_b = ucs4_case_fold_simple(c_b);

            res = (c_a != c_b);

            if(!res)
                return(FALSE); /* not equal character */
        }

        if(CH_NULL == c_a)
            return(TRUE); /* ended together at the terminator -> equal (even if matched length differs) */
    }

    /* matched up to the comparison limit */

    /* unequal number of bytes left over? */
    remain_a = uchars_n_a - i_a;
    remain_b = uchars_n_b - i_b;

    if(remain_a == remain_b)
        return(TRUE); /* ended together at the specified finite lengths -> equal (even if matched length differs) */

    /* sort out any string length residuals */
    assert((uchars_n_a != strlen_with_NULLCH) || (uchars_n_b == strlen_with_NULLCH)); /* an admixture wouldn't be useful */

    if(uchars_n_a >= strlen_without_NULLCH)
        remain_a = utf8str_strlen32_n(utf8str_AddBytes(ustr_a, i_a), uchars_n_a);

    if(uchars_n_b >= strlen_without_NULLCH)
        remain_b = utf8str_strlen32_n(utf8str_AddBytes(ustr_b, i_b), uchars_n_b);

    /*if(remain_a == remain_b)
        return(TRUE);*/ /* ended together at the determined finite lengths -> equal (even if matched length differs) */

    res = (remain_a == remain_b);

    return(res);
}

#if !USTR_IS_SBSTR

_Check_return_
extern STATUS
utf8str_case_fold(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/)
{
    return(utf8_case_fold(p_quick_ublock, (PC_UTF8) ustr, utf8str_strlen32_n(ustr, uchars_n)));
}

#endif /* USTR_IS_SBSTR */

/******************************************************************************
*
* replacement functions should help verify that all is type-correct
*
******************************************************************************/

_Check_return_
_Ret_maybenull_z_ /* may be NULL */
extern P_UTF8STR
utf8str_strchr(
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U8 ch)
{
#if CHECKING_UTF8
    /* validate input string */
    if(status_fail(utf8str_validate(TEXT("utf8str_strchr arg"), ustr)))
        return(NULL);
#endif

    assert(u8_is_ascii7(ch));

    return((P_UTF8STR) (strchr((const char *) ustr, ch)));
}

/* make sure we transfer whole UTF-8 characters */

/*
append up characters from src (until CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* bytes currently in output */
utf8str_xstrkat(
    _Inout_updates_z_(dst_n) P_UTF8STR dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_UTF8STR src)
{
    U32 dst_idx = 0;
    U32 src_idx = 0;

#if CHECKING_UTF8
    /* validate input string */
    if(status_fail(utf8str_validate(TEXT("utf8str_xstrkat src"), src)))
    {
        errno = EINVAL;
        _s_raise(errno);
        return(utf8str_strlen32p1(dst));
    }
#endif

    /* find out where in dst buffer to append the string */
    for(;;)
    {
        U8 u8;

        /* watch out for unterminated dst buffer */
        if(dst_idx == dst_n)
        {
            assert(dst_idx != dst_n);
            /* keep our promise to terminate if that's possible */
            if(0 != dst_n)
                PtrPutByteOff(dst, dst_n - 1, CH_NULL);
            return(dst_n);
        }

        u8 = PtrGetByteOff(dst, dst_idx);

        /* append here? */
        if(CH_NULL == u8)
            break;

        ++dst_idx;
    }

    /* copy src to this point */
    for(;;)
    {
        U8 u8 = PtrGetByteOff(src, src_idx);
        U32 bytes_of_char;

        if(u8_is_ascii7(u8))
        {
            /* finished source string? */
            if(CH_NULL == u8)
                break;

            bytes_of_char = 1;

            /* is there room to put both this whole character *and a CH_NULL* to destination buffer? */
            if((dst_idx + 1/*bytes_of_char*/) >= dst_n)
                break;

            PtrPutByteOff(dst, dst_idx, u8);
        }
        else
        {
            bytes_of_char = utf8_bytes_of_char_off(src, src_idx);

            /* is there room to put both this whole character *and a CH_NULL* to destination buffer? */
            if((dst_idx + bytes_of_char) >= dst_n)
                break;

            utf8_char_copy(utf8_AddBytes_wr(dst, dst_idx), utf8_AddBytes(src, src_idx), bytes_of_char);
        }

        src_idx += bytes_of_char;
        dst_idx += bytes_of_char;
    }

    /* NB watch out for zero dst_n (and cockups) */
    if(dst_idx >= dst_n)
    {
        assert(dst_idx < dst_n);
        if(0 == dst_n)
            return(dst_n);
        dst_idx = dst_n - 1;
    }

    PtrPutByteOff(dst, dst_idx++, CH_NULL); /* ensure terminated */

#if CHECKING_UTF8
    /* validate output string */
    status_assert(utf8str_validate(TEXT("utf8str_xstrkat dst"), dst));
#endif

    return(dst_idx); /* bytes currently in output */
}

/*
append up to src_n bytes from src (or fewer, if CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* bytes currently in output */
utf8str_xstrnkat(
    _Inout_updates_z_(dst_n) P_UTF8STR dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PC_UTF8STR src,
    _InVal_     U32 src_n)
{
    U32 dst_idx = 0;
    U32 src_idx = 0;

#if CHECKING_UTF8
    /* validate input string */
    if(status_fail(utf8str_validate_n(TEXT("utf8str_xstrnkat src"), src, src_n)))
    {
        errno = EINVAL;
        _s_raise(errno);
        return(utf8str_strlen32p1(dst));
    }
#endif

    /* find out where in dst buffer to append the string */
    for(;;)
    {
        U8 u8;

        /* watch out for unterminated dst buffer */
        if(dst_idx == dst_n)
        {
            assert(dst_idx != dst_n);
            /* keep our promise to terminate if that's possible */
            if(0 != dst_n)
                PtrPutByteOff(dst, dst_n - 1, CH_NULL);
            return(dst_n);
        }

        u8 = PtrGetByteOff(dst, dst_idx);

        /* append here? */
        if(CH_NULL == u8)
            break;

        ++dst_idx;
    }

    /* copy src to this point */
    while(src_idx < src_n)
    {
        U8 u8 = PtrGetByteOff(src, src_idx);
        U32 bytes_of_char;

        if(u8_is_ascii7(u8))
        {
            /* finished source string before reaching source limit? */
            if(CH_NULL == u8)
                break;

            bytes_of_char = 1;

            /* is there room to put both this whole character *and a CH_NULL* to destination buffer? */
            if((dst_idx + 1/*bytes_of_char*/) >= dst_n)
                break;

            PtrPutByteOff(dst, dst_idx, u8);
        }
        else
        {
            bytes_of_char = utf8_bytes_of_char_off(src, src_idx);

            /* is there room to put both this whole character *and a CH_NULL* to destination buffer? */
            if((dst_idx + bytes_of_char) >= dst_n)
                break;

            utf8_char_copy(utf8_AddBytes_wr(dst, dst_idx), utf8_AddBytes(src, src_idx), bytes_of_char);
        }

        dst_idx += bytes_of_char;
        src_idx += bytes_of_char;
    }

    /* NB watch out for zero dst_n (and cockups) */
    if(dst_idx >= dst_n)
    {
        assert(dst_idx < dst_n);
        if(0 == dst_n)
            return(dst_n);
        dst_idx = dst_n - 1;
    }

    PtrPutByteOff(dst, dst_idx++, CH_NULL); /* ensure terminated */

#if CHECKING_UTF8
    /* validate output string */
    status_assert(utf8str_validate(TEXT("utf8str_xstrnkat dst"), dst));
#endif

    return(dst_idx); /* bytes currently in output */
}

/*
copy characters from src (until CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* bytes currently in output */
utf8str_xstrkpy(
    _Out_writes_z_(dst_n) P_UTF8STR dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_UTF8STR src)
{
    U32 dst_idx = 0;
    U32 src_idx = 0;

#if CHECKING_UTF8
    /* validate input string */
    if(status_fail(utf8str_validate(TEXT("utf8str_xstrkpy src"), src)))
    {
        errno = EINVAL;
        _s_raise(errno);
        if(0 == dst_n)
            return(0);
        PtrPutByte(dst, CH_NULL);
        return(utf8str_strlen32p1(dst));
    }
#endif

    /* copy src to this point */
    for(;;)
    {
        U8 u8 = PtrGetByteOff(src, src_idx);
        U32 bytes_of_char;

        if(u8_is_ascii7(u8))
        {
            /* finished source string? */
            if(CH_NULL == u8)
                break;

            bytes_of_char = 1;

            /* is there room to put both this whole character *and a CH_NULL* to destination buffer? */
            if((dst_idx + 1/*bytes_of_char*/) >= dst_n)
                break;

            PtrPutByteOff(dst, dst_idx, u8);
        }
        else
        {
            bytes_of_char = utf8_bytes_of_char_off(src, src_idx);

            /* is there room to put both this whole character *and a CH_NULL* to destination buffer? */
            if((dst_idx + bytes_of_char) >= dst_n)
                break;

            utf8_char_copy(utf8_AddBytes_wr(dst, dst_idx), utf8_AddBytes(src, src_idx), bytes_of_char);
        }

        src_idx += bytes_of_char;
        dst_idx += bytes_of_char;
    }

    /* NB watch out for zero dst_n (and cockups) */
    if(dst_idx >= dst_n)
    {
        assert(dst_idx < dst_n);
        if(0 == dst_n)
            return(dst_n);
        dst_idx = dst_n - 1;
    }

    PtrPutByteOff(dst, dst_idx++, CH_NULL); /* ensure terminated */

#if CHECKING_UTF8
    /* validate output string */
    status_assert(utf8str_validate(TEXT("utf8str_xstrkpy dst"), dst));
#endif

    return(dst_idx); /* bytes currently in output */
}

/*
copy up to src_n bytes from src (or fewer, if CH_NULL found) to dst (subject to dst limit)
*/

/*ncr*/
extern U32 /* bytes currently in output */
utf8str_xstrnkpy(
    _Out_writes_z_(dst_n) P_UTF8STR dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PC_UTF8STR src,
    _InVal_     U32 src_n)
{
    U32 dst_idx = 0;
    U32 src_idx = 0;

#if CHECKING_UTF8
    /* validate input string */
    if(status_fail(utf8str_validate_n(TEXT("utf8str_xstrnkpy src"), src, src_n)))
    {
        errno = EINVAL;
        _s_raise(errno);
        if(0 == dst_n)
            return(0);
        PtrPutByte(dst, CH_NULL);
        return(utf8str_strlen32p1(dst));
    }
#endif

    /* copy src to this point */
    while(src_idx < src_n)
    {
        U8 u8 = PtrGetByteOff(src, src_idx);
        U32 bytes_of_char;

        if(u8_is_ascii7(u8))
        {
            /* finished source string before reaching source limit? */
            if(CH_NULL == u8)
                break;

            bytes_of_char = 1;

            /* is there room to put both this whole character *and a CH_NULL* to destination buffer? */
            if((dst_idx + 1/*bytes_of_char*/) >= dst_n)
                break;

            PtrPutByteOff(dst, dst_idx, u8);
        }
        else
        {
            bytes_of_char = utf8_bytes_of_char_off(src, src_idx);

            /* is there room to put both this character *and a CH_NULL* to destination buffer? */
            if((dst_idx + bytes_of_char) >= dst_n)
                break;

            utf8_char_copy(utf8_AddBytes_wr(dst, dst_idx), utf8_AddBytes(src, src_idx), bytes_of_char);
        }

        src_idx += bytes_of_char;
        dst_idx += bytes_of_char;
    }

    /* NB watch out for zero dst_n (and cockups) */
    if(dst_idx >= dst_n)
    {
        assert(dst_idx < dst_n);
        if(0 == dst_n)
            return(dst_n);
        dst_idx = dst_n - 1;
    }

    PtrPutByteOff(dst, dst_idx++, CH_NULL); /* ensure terminated */

#if CHECKING_UTF8
    /* validate output string */
    status_assert(utf8str_validate(TEXT("utf8str_xstrnkpy dst"), dst));
#endif

    return(dst_idx); /* bytes currently in output */
}

_Check_return_
_Ret_maybenull_z_ /* may be NULL */
extern P_UTF8STR
utf8str_strpbrk(
    _In_z_      PC_UTF8STR ustr,
    _In_z_      const char *test)
{
#if CHECKING_UTF8
    /* validate input string */
    if(status_fail(utf8str_validate(TEXT("utf8str_strpbrk arg"), ustr)))
        return(NULL);
#endif

#if CHECKING_UTF8
    { /* Also verify that 'test' does not contain any multi-byte UTF-8 character encodings */
    const char *ptr = test;

    while(CH_NULL != *ptr)
    {
        char ch = *ptr++;
        assert(u8_is_ascii7(ch));
    }
    } /*block*/
#endif

    return(de_const_cast(P_UTF8STR, strpbrk((const char *) ustr, test)));
}

_Check_return_
extern int __cdecl
utf8str_xsnprintf(
    _Out_writes_z_(dst_n) P_UTF8STR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PC_UTF8STR ustr_format,
    /**/        ...)
{
    va_list args;
    int ret;

    va_start(args, ustr_format);
    ret = utf8str_xvsnprintf(dst, dst_n, ustr_format, args);
    va_end(args);

    return(ret);
}

_Check_return_
extern int __cdecl
utf8str_xvsnprintf(
    _Out_writes_z_(dst_n) P_UTF8STR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PC_UTF8STR ustr_format,
    /**/        va_list args)
{
    int ret;

    if(0 == dst_n)
        return(0);

#if CHECKING_UTF8
    /* validate input string */
    if(status_fail(utf8str_validate(TEXT("utf8str_xvsnprintf format arg"), ustr_format)))
    {
        errno = EINVAL;
        _s_raise(errno);
        PtrPutByte(dst, CH_NULL);
        return(-1);
    }
#endif

#if WINDOWS
    ret = _vsnprintf_s((char *) dst, dst_n, _TRUNCATE, (const char *) ustr_format, args);
    if(-1 == ret) /* limit the answer */
        ret = utf8str_strlen32(dst);
#else /* C99 CRT */
    ret = vsnprintf((char *) dst, dst_n, (const char *) ustr_format, args);
    if(ret >= (int) dst_n) /* limit the answer */
        ret = utf8str_strlen32(dst);
#endif /* OS */

    if(0 != ret)
    {   /* verify that we don't have a truncated UTF-8 sequence at the end */
        U32 offset = (U32) ret;

        --offset; /* point to the last byte */

        if(u8_is_utf8_lead_byte(PtrGetByteOff(dst, offset)))
        {
            assert(!u8_is_utf8_lead_byte(PtrGetByteOff(dst, offset)));
            ret = (int) offset; /* broken - retract */
        }
        else if(u8_is_utf8_trail_byte(PtrGetByteOff(dst, offset)))
        {
            do
                --offset;
            while((offset != 0) && u8_is_utf8_trail_byte(PtrGetByteOff(dst, offset)));

            if(u8_is_utf8_lead_byte(PtrGetByteOff(dst, offset)))
            {
                U32 bytes_of_char = utf8_bytes_of_char_off(dst, offset);

                if(offset + bytes_of_char != (U32) ret)
                {
                    assert(offset + bytes_of_char == (U32) ret);
                    ret = (int) offset; /* broken - retract */
                }
                /* otherwise it is OK */
            }
            else
            {
                assert(u8_is_utf8_lead_byte(PtrGetByteOff(dst, offset)));
                ret = (int) offset; /* broken - retract */
            }
        }
        /* otherwise it is OK */
    }

#if CHECKING_UTF8
    /* validate output string */
    status_assert(utf8str_validate(TEXT("utf8str_xvsnprintf dst"), dst));
#endif

    return(ret);
}

/* used by RISC OS routines, spell module and Lotus 1-2-3 export */

static struct sbstr_from_utf8str_statics
{
    PC_SBSTR last;
    U32 used;
    U8 buffer[4 * 1024];
}
sbstr_from_utf8str_statics;

_Check_return_
_Ret_z_ /* never NULL */
extern PC_SBSTR /*low-lifetime*/
_sbstr_from_utf8str(
    _In_z_      PC_UTF8STR ustr)
{
    P_SBSTR dstptr;
    U32 avail;
    U32 n_bytes;
    BOOL is_pure_ascii7 = TRUE;
    U32 bytes_needed;

#if CHECKING_UTF8
    /* Check that UTF-8 string we are converting is valid UTF-8 */
    if(status_fail(utf8str_validate(TEXT("_sbstr_from_utf8str arg"), ustr)))
        return("Invalid UTF-8");
#endif

    n_bytes = utf8str_strlen32p1(ustr); /*CH_NULL*/

    /* perform sizing pass (including CH_NULL) */
    bytes_needed = sbchars_from_utf8_bytes_needed(ustr, n_bytes, &is_pure_ascii7);

    if(is_pure_ascii7)
        /* no conversion needed, don't waste any more time/space */
        return((PC_SBSTR) ustr);

    avail = elemof32(sbstr_from_utf8str_statics.buffer) - sbstr_from_utf8str_statics.used;

    if(bytes_needed > avail)
        /* retry with whole buffer */
        sbstr_from_utf8str_statics.used = 0;

    avail = elemof32(sbstr_from_utf8str_statics.buffer) - sbstr_from_utf8str_statics.used;

    dstptr = sbstr_from_utf8str_statics.buffer + sbstr_from_utf8str_statics.used;

    /* perform conversion pass (including CH_NULL) */
    assert(bytes_needed < avail);
    consume(U32, sbchars_from_utf8(dstptr, avail, get_system_codepage(), ustr, n_bytes));

    sbstr_from_utf8str_statics.last = dstptr;

    sbstr_from_utf8str_statics.used += bytes_needed;

    return(dstptr);
}

/******************************************************************************
*
* Convert to SBCHAR (U8 Latin-N) string buffer from a UTF-8 counted string
*
* e.g. for RISC OS Font Manager, Draw file text, Lotus 1-2-3 export
*
* NB always CH_NULL-terminated
*
* Returns number of bytes in the output buffer, including the CH_NULL
*
******************************************************************************/

/*ncr*/
extern U32
sbstr_buf_from_utf8str(
    _Out_writes_opt_z_(elemof_buffer) P_SBSTR sbstr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage,
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n_in /*strlen_with,without_NULLCH*/)
{
    U32 uchars_n;
    BOOL is_pure_ascii7 = TRUE;
    U32 bytes_needed;
    U32 sbchars_output;

#if CHECKING_UTF8
    if(status_fail(utf8str_validate_n(TEXT("sbstr_buf_from_utf8str_convert arg"), ustr, uchars_n_in)))
    {
        if(NULL != sbstr_buf)
        {
            assert(0 != elemof_buffer);
            sbstr_buf[0] = CH_NULL;
        }
        return(0);
    }
#endif

    uchars_n = utf8str_strlen32_n(ustr, uchars_n_in);

    /* perform sizing pass */
    bytes_needed = sbchars_from_utf8_bytes_needed(ustr, uchars_n, &is_pure_ascii7);

    if(NULL == sbstr_buf)
        return(bytes_needed);

    if(is_pure_ascii7)
        /* no conversion needed, don't waste any more time/space */
        return(xstrnkpy(sbstr_buf, elemof_buffer, (PC_U8) ustr, uchars_n));

    /* perform conversion pass */
    sbchars_output = sbchars_from_utf8(sbstr_buf, elemof_buffer, sbchar_codepage, ustr, uchars_n);

    /* NB ensure always CH_NULL-terminated */
    if(0 == elemof_buffer)
    { assert(0 != elemof_buffer); }
    else if(sbchars_output >= elemof_buffer)
    {
        sbchars_output = elemof_buffer - 1;
        sbstr_buf[sbchars_output++] = CH_NULL;
    }
    else if((0 == sbchars_output) || (CH_NULL != sbstr_buf[sbchars_output - 1]))
    {   /* terminating CH_NULL wasn't included in conversion so add it */
        sbstr_buf[sbchars_output++] = CH_NULL;
    }

    return(sbchars_output); /* SBCHARs currently in output */
}

/******************************************************************************
*
* convert to TCHAR string buffer from a UTF-8 counted string
*
* NB always CH_NULL-terminated
*
* Returns number of TCHARs in the output buffer, including the CH_NULL
*
******************************************************************************/

/*ncr*/
extern U32
tstr_buf_from_utf8str(
    _Out_writes_opt_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n_in /*strlen_with,without_NULLCH*/)
{
#if TSTR_IS_SBSTR
    return(sbstr_buf_from_utf8str((P_SBSTR) tstr_buf, elemof_buffer, get_system_codepage(), ustr, uchars_n_in));
#else
    const U32 uchars_n = utf8str_strlen32_n(ustr, uchars_n_in);
    U32 tchars_output;

#if CHECKING_UTF8
    if(status_fail(utf8str_validate_n(TEXT("tstr_buf_from_utf8str_convert arg"), ustr, uchars_n_in)))
    {
        if(NULL != tstr_buf)
        {
            assert(0 != elemof_buffer);
            tstr_buf[0] = CH_NULL;
        }
        return(0);
    }
#endif

#if WINDOWS
    {
    int wchars_n;

    wchars_n =
        MultiByteToWideChar(CP_UTF8 /*SourceCodePage*/, 0 /*dwFlags*/,
                            (PCSTR) ustr, (int) uchars_n,
                            tstr_buf, (int) elemof_buffer);

    if(wchars_n < 0) return(0);

    tchars_output = wchars_n;
    } /*block*/
#else
#error No implementation of tstr_buf_from_utf8str_convert
#endif /* OS */

    if(NULL == tstr_buf)
        return(tchars_output); /* TCHARs counted */

    /* NB ensure always CH_NULL-terminated */
    if(0 == elemof_buffer)
    { assert(0 != elemof_buffer); }
    else if(tchars_output >= elemof_buffer)
    {
        tchars_output = elemof_buffer - 1;
        tstr_buf[tchars_output++] = CH_NULL;
    }
    else if((0 == tchars_output) || (CH_NULL != tstr_buf[tchars_output - 1]))
    {   /* terminating CH_NULL wasn't included in conversion so add it */
        tstr_buf[tchars_output++] = CH_NULL;
    }

    return(tchars_output); /* TCHARs currently in output */
#endif /* TSTR_IS_SBSTR */
}

/******************************************************************************
*
* convert to UTF-8 character string buffer from a counted TCHARs string
*
* NB always CH_NULL-terminated
*
******************************************************************************/

/*ncr*/
extern U32
utf8str_buf_from_tstr(
    _Out_writes_opt_z_(elemof_buffer) P_UTF8STR ustr_buf, /*NULL->count*/ /* [] of UTF8, CH_NULL-terminated */
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR tstr,
    _InVal_     U32 tchars_n /*strlen_with,without_NULLCH*/)
{
    U32 uchars_output = utf8_from_tchars(ustr_buf, elemof_buffer, tstr, tstrlen32_n(tstr, tchars_n));

    if(NULL == ustr_buf)
        return(uchars_output); /* bytes counted */

    /* NB ensure always CH_NULL-terminated */
    if(0 == elemof_buffer)
    { assert(0 != elemof_buffer); }
    if(uchars_output >= elemof_buffer)
    {
        uchars_output = elemof_buffer - 1;
        PtrPutByteOff(ustr_buf, uchars_output++, CH_NULL);
    }
    else if((0 == uchars_output) || (CH_NULL != PtrGetByteOff(ustr_buf, uchars_output-1)))
    {   /* terminating CH_NULL wasn't included in conversion so add it */
        PtrPutByteOff(ustr_buf, uchars_output++, CH_NULL);
    }

#if CHECKING_UTF8
    if((NULL != ustr_buf) && status_fail(utf8str_validate(TEXT("utf8str_buf_from_tstr_convert buf"), ustr_buf)))
        return(0);
#endif

    return(uchars_output); /* bytes currently in output */
}

/* end of utf8str.c */
