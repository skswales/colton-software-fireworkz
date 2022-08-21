/* utf8str.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2006-2016 Stuart Swales */

#ifndef __utf8str_h
#define __utf8str_h

#ifndef          __utf8_h
#include "cmodules/utf8.h"
#endif

/*
CH_NULL-terminated UTF-8 character string functions
*/

/*
exported functions
*/

#define utf8str_IncBytes(ustr, add) \
    PtrIncBytes(PC_UTF8STR, ustr, add)

#define utf8str_IncBytes_wr(ustr, add) \
    PtrIncBytes(P_UTF8STR, ustr, add)

#define utf8str_AddBytes(ustr, add) \
    PtrAddBytes(PC_UTF8STR, ustr, add)

#define utf8str_AddBytes_wr(ustr, add) \
    PtrAddBytes(P_UTF8STR, ustr, add)

/*
number of bytes of UTF-8 encoding of the first character in this USTR
*/

/* single ASCII-7 bytes retrieved without function call overhead */

/* NL variant is not required as total length is encoded in the first byte */

#define utf8str_bytes_of_char(ustr) /*num*/ (   \
    u8_is_ascii7(PtrGetByte(ustr))              \
    ? 1                                         \
    : utf8str__bytes_of_char(ustr)              )

_Check_return_
static inline U32
utf8str__bytes_of_char(
    _In_z_      PC_UTF8STR ustr)
{
    return(utf8__bytes_of_char(ustr));
}

#define utf8str_bytes_of_char_off(ustr, off) /*num*/ (  \
    u8_is_ascii7(PtrGetByteOff(ustr, off))              \
    ? 1                                                 \
    : utf8str__bytes_of_char_off(ustr, off)             )

_Check_return_
static inline U32
utf8str__bytes_of_char_off(
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 offset)
{
    return(utf8__bytes_of_char_off(ustr, offset));
}

/* single ASCII-7 bytes retrieved without function call overhead */
#define utf8str_bytes_prev_of_char(ustr_start, ustr) /*num*/ (  \
    (   (ustr_start != ustr)                    &&              \
        u8_is_ascii7(PtrGetByteOff(ustr, -1))   )               \
        ? 1                                                     \
        : utf8str__bytes_prev_of_char(ustr_start, ustr)         )

_Check_return_
static inline U32
utf8str__bytes_prev_of_char(
    _In_z_      PC_UTF8STR ustr_start,
    _In_z_      PC_UTF8STR ustr)
{
    return(utf8__bytes_prev_of_char(ustr_start, ustr));
}

/*
decode UCS-4 character from the first character in this USTR, with number of bytes
*/

/* single ASCII-7 bytes retrieved without function call overhead */
#define utf8str_char_decode(ustr, bytes_of_char__ref) /*UCS4*/ (    \
    u8_is_ascii7(PtrGetByte(ustr))                                  \
        ? ( (bytes_of_char__ref) = 1, (UCS4) PtrGetByte(ustr) )     \
        : utf8str__char_decode(ustr, &(bytes_of_char__ref))         )

#define utf8str_char_decode_off(ustr, off, bytes_of_char__ref) /*UCS4*/ (   \
    u8_is_ascii7(PtrGetByteOff(ustr, off))                                  \
        ? ( (bytes_of_char__ref) = 1, (UCS4) PtrGetByteOff(ustr, off) )     \
        : utf8str__char_decode_off(ustr, off, &(bytes_of_char__ref))        )

#define utf8str_char_decode_NULL(ustr) /*UCS4*/ (   \
    u8_is_ascii7(PtrGetByte(ustr))                  \
        ? (UCS4) PtrGetByte(ustr)                   \
        : utf8str__char_decode(ustr, NULL)          )

#define utf8str_char_decode_off_NULL(ustr, off) /*UCS4*/ (  \
    u8_is_ascii7(PtrGetByteOff(ustr, off))                  \
        ? (UCS4) PtrGetByteOff(ustr, off)                   \
        : utf8str__char_decode_off(ustr, off, NULL)         )

_Check_return_
static inline UCS4
utf8str__char_decode(
    _In_z_      PC_UTF8STR ustr,
    _Out_opt_   P_U32 p_bytes_of_char)
{
    return(utf8__char_decode(ustr, p_bytes_of_char));
}

_Check_return_
static inline UCS4
utf8str__char_decode_off(
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 offset,
    _Out_opt_   P_U32 p_bytes_of_char)
{
    return(utf8__char_decode(utf8str_AddBytes(ustr, offset), p_bytes_of_char));
}

_Check_return_
extern UCS4
utf8str_char_next(
    _In_z_      PC_UTF8STR ustr,
    _OutRef_    P_PC_UTF8STR p_ustr_next);

/*
case-sensitive string comparision
*/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
utf8str_compare(
    _In_z_      PC_UTF8STR ustr_a,
    _In_z_      PC_UTF8STR ustr_b);

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
utf8str_compare_equals(
    _In_z_      PC_UTF8STR ustr_a,
    _In_z_      PC_UTF8STR ustr_b);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
utf8str_compare_n2(
    _In_z_      PC_UTF8STR ustr_a,
    _InVal_     U32 uchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_      PC_UTF8STR ustr_b,
    _InVal_     U32 uchars_n_b /*strlen_with,without_NULLCH*/);

/*
case-insensitive string comparision
*/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
utf8str_compare_nocase(
    _In_z_      PC_UTF8STR ustr_a,
    _In_z_      PC_UTF8STR ustr_b);

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
utf8str_compare_equals_nocase(
    _In_z_      PC_UTF8STR ustr_a,
    _In_z_      PC_UTF8STR ustr_b);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
utf8str_compare_n2_nocase(
    _In_z_      PC_UTF8STR ustr_a,
    _InVal_     U32 uchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_      PC_UTF8STR ustr_b,
    _InVal_     U32 uchars_n_b /*strlen_with,without_NULLCH*/);

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
utf8str_compare_equals_n2_nocase(
    _In_z_      PC_UTF8STR ustr_a,
    _InVal_     U32 uchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_      PC_UTF8STR ustr_b,
    _InVal_     U32 uchars_n_b /*strlen_with,without_NULLCH*/);

#if !USTR_IS_SBSTR

_Check_return_
extern STATUS
utf8str_case_fold(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/);

#endif /* !USTR_IS_SBSTR */

/*
replacement functions help verify that all is type-correct
*/

_Check_return_
static inline U32
utf8str_strlen32(
    _In_z_      PC_UTF8STR ustr)
{
    /* NB Don't validate input string here as this is used during, and alongside many calls to, utf8str_validate */
    return(strlen32((const char *) ustr));
}

_Check_return_
static inline U32
utf8str_strlen32p1(
    _In_z_      PC_UTF8STR ustr)
{
    return(strlen32p1((const char *) ustr));
}

_Check_return_
static inline U32
utf8str_strlen32_n(
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/)
{
    if(strlen_without_NULLCH == uchars_n)
        return(utf8str_strlen32(ustr));

    if(strlen_with_NULLCH == uchars_n)
        return(utf8str_strlen32p1(ustr));

    return(uchars_n);
}

_Check_return_
_Ret_maybenull_z_ /* may be NULL */
extern P_UTF8STR
utf8str_strchr(
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U8 ch);

/*ncr*/
extern U32 /* bytes currently in output */
utf8str_xstrkat(
    _Inout_updates_z_(dst_n) P_UTF8STR dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_UTF8STR src);

/*ncr*/
extern U32 /* bytes currently in output */
utf8str_xstrnkat(
    _Inout_updates_z_(dst_n) P_UTF8STR dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PC_UTF8STR src,
    _InVal_     U32 src_n);

/*ncr*/
extern U32 /* bytes currently in output */
utf8str_xstrkpy(
    _Out_writes_z_(dst_n) P_UTF8STR dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_UTF8STR src);

/*ncr*/
extern U32 /* bytes currently in output */
utf8str_xstrnkpy(
    _Out_writes_z_(dst_n) P_UTF8STR dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PC_UTF8STR src,
    _InVal_     U32 src_n);

_Check_return_
_Ret_maybenull_z_ /* may be NULL */
extern P_UTF8STR
utf8str_strpbrk(
    _In_z_      PC_UTF8STR ustr,
    _In_z_      const char *test);

_Check_return_
extern int __cdecl
utf8str_xsnprintf(
    _Out_writes_z_(dst_n) P_UTF8STR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PC_UTF8STR ustr_format,
    /**/        ...);

_Check_return_
extern int __cdecl
utf8str_xvsnprintf(
    _Out_writes_z_(dst_n) P_UTF8STR dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ PC_UTF8STR ustr_format,
    /**/        va_list args);

_Check_return_
extern STATUS /* STATUS_OK, error if not */
utf8str_validate(
    _In_z_      PCTSTR func,
    _In_z_      PC_UTF8STR ustr);

_Check_return_
extern STATUS /* STATUS_OK, error if not */
utf8str_validate_n(
    _In_z_      PCTSTR func,
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/);

/*
for SBCHAR (U8 Latin-N) text, e.g. for RISC OS Font Manager, Draw file text
*/

_Check_return_
_Ret_z_ /* never NULL */
extern PC_SBSTR /*low-lifetime*/
_sbstr_from_utf8str(
    _In_z_      PC_UTF8STR ustr);

/*ncr*/
extern U32
sbstr_buf_from_utf8str(
    _Out_writes_opt_z_(elemof_buffer) P_SBSTR sbstr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage,
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/);

/*ncr*/
extern U32
tstr_buf_from_utf8str(
    _Out_writes_opt_z_(elemof_buffer) PTSTR tstr_buf, /*NULL->count*/
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/);

/*ncr*/
extern U32
utf8str_buf_from_tstr(
    _Out_writes_opt_z_(elemof_buffer) P_UTF8STR ustr_buf, /*NULL->count*/ /* [] of UTF-8, CH_NULL-terminated */
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR tstr,
    _InVal_     U32 tchars_n /*strlen_with,without_NULLCH*/);

/*
USTR to UTF8STR transitional macros
*/

#if USTR_IS_SBSTR

/* macros are defined in xustring.h */

#else /* NOT USTR_IS_SBSTR */

#define _sbstr_from_ustr(ustr)      _sbstr_from_utf8str(ustr)
/* _ustr_from_sbstr(sbstr) */ /* appears to be unused */

#define ustrlen32(ustr) /*U32*/ \
    utf8str_strlen32(ustr)

#define ustrlen32p1(ustr) /*U32*/ \
    utf8str_strlen32p1(ustr)

#define ustrlen32_n(ustr, n_bytes) /*U32*/ \
    utf8str_strlen32_n(ustr, n_bytes)

#define ustrchr(s, c)               utf8str_strchr(s, c)
#define ustrpbrk(s1, s2)            utf8str_strpbrk(s1, s2)

#define ustr_xstrkat(dst, dst_n, src) /*U32*/ \
    utf8str_xstrkat(dst, dst_n, src)

#define ustr_xstrnkat(dst, dst_n, src, src_n) /*U32*/ \
    utf8str_xstrnkat(dst, dst_n, src, src_n)

#define ustr_xstrkpy(dst, dst_n, src) /*U32*/ \
    utf8str_xstrkpy(dst, dst_n, src) \

#define ustr_xstrnkpy(dst, dst_n, src, src_n) /*U32*/ \
    utf8str_xstrnkpy(dst, dst_n, src, src_n)

#define ustr_compare(ustr_a, ustr_b) /*int*/ \
    utf8str_compare(ustr_a, ustr_b)

#define ustr_compare_equals(ustr_a, ustr_b) /*BOOL*/ \
    utf8str_compare_equals(ustr_a, ustr_b)

#define ustr_compare_n2(ustr_a, uchars_n_a, ustr_b, uchars_n_b) /*int*/ \
    utf8str_compare_n2(ustr_a, uchars_n_a, ustr_b, uchars_n_b)

#define ustr_compare_nocase(ustr_a, ustr_b) /*int*/ \
    utf8str_compare_nocase(ustr_a, ustr_b)

#define ustr_compare_equals_nocase(ustr_a, ustr_b) /*BOOL*/ \
    utf8str_compare_equals_nocase(ustr_a, ustr_b)

#define ustr_compare_n2_nocase(ustr_a, uchars_n_a, ustr_b, uchars_n_b) /*int*/ \
    utf8str_compare_n2_nocase(ustr_a, uchars_n_a, ustr_b, uchars_n_b)

#define ustr_xsnprintf \
    utf8str_xsnprintf

#define ustr_xvsnprintf \
    utf8str_xvsnprintf

#define ustr_validate(func, ustr) \
    utf8str_validate(func, ustr)

#define ustr_validate_n(func, ustr, uchars_n) \
    utf8str_validate_n(func, ustr, uchars_n)

#define tstr_buf_from_ustr(buffer, elemof_buffer, ustr, uchars_n) \
    tstr_buf_from_utf8str(buffer, elemof_buffer, ustr, uchars_n)

#endif /* USTR_IS_SBSTR */

/*
number of bytes of USTR encoding of the character pointed to
*/

#if USTR_IS_SBSTR

#define ustr_bytes_of_char(ustr) /*num*/ \
    1

#define ustr_bytes_of_char_off(ustr, off) /*num*/ \
    1

#else /* NOT USTR_IS_SBSTR */

#define ustr_bytes_of_char(ustr) /*num*/ \
    utf8str_bytes_of_char(ustr)

#define ustr_bytes_of_char_off(uchars, off) /*num*/ \
    utf8str_bytes_of_char_off(uchars, off)

#endif /* USTR_IS_SBSTR */

#if USTR_IS_SBSTR

#define ustr_bytes_prev_of_char(ustr_start, ustr) /*num*/ \
    ( (ustr_start != ustr) ? 1 : 0 )

#else /* NOT USTR_IS_SBSTR */

#define ustr_bytes_prev_of_char(ustr_start, ustr) /*num*/ \
    utf8str_bytes_prev_of_char(ustr_start, ustr)

#endif /* USTR_IS_SBSTR */

/*
decode UCS-4 character from USTR character encoding, with number of bytes
*/

#if USTR_IS_SBSTR

#define ustr_char_decode(ustr, bytes_of_char__ref) /*UCS4*/ ( \
    (bytes_of_char__ref) = 1, \
    (UCS4) PtrGetByte(ustr) )

#define ustr_char_decode_off(ustr, off, bytes_of_char__ref) /*UCS4*/ ( \
    (bytes_of_char__ref) = 1, \
    (UCS4) PtrGetByteOff(ustr, off) )

#define ustr_char_decode_NULL(ustr) /*UCS4*/ \
    (UCS4) PtrGetByte(ustr)

#define ustr_char_decode_off_NULL(ustr, off) /*UCS4*/ \
    (UCS4) PtrGetByteOff(ustr, off)

#else /* NOT USTR_IS_SBSTR */

#define ustr_char_decode(ustr, bytes_of_char__ref) /*UCS4*/ \
    utf8str_char_decode(ustr, bytes_of_char__ref)

#define ustr_char_decode_off(ustr, off, bytes_of_char__ref) /*UCS4*/ \
    utf8str_char_decode_off(ustr, off, bytes_of_char__ref)

#define ustr_char_decode_NULL(ustr) /*UCS4*/ \
    utf8str_char_decode_NULL(ustr)

#define ustr_char_decode_off_NULL(ustr, off) /*UCS4*/ \
    utf8str_char_decode_off_NULL(ustr, off)

#endif /* USTR_IS_SBSTR */

/*
* return the current UCS-4 character in a USTR character string,
* returning pointer to the start of the next character encoding.
*/

#if USTR_IS_SBSTR

_Check_return_
static inline UCS4
ustr_char_next(
    _In_z_      PC_USTR ustr,
    _OutRef_    P_PC_USTR p_ustr_next)
{
    U32 bytes_of_char;
    UCS4 ucs4 = ustr_char_decode(ustr, /*ref*/bytes_of_char);

    /* return pointer to byte at start of the next UCHARS sequence */
    if(CH_NULL == ucs4) /* don't wander beyond the end of the string */
        *p_ustr_next = ustr;
    else
        *p_ustr_next = ustr_AddBytes(ustr, bytes_of_char);

    return(ucs4);
}

#else /* NOT USTR_IS_SBSTR */

#define ustr_char_next(ustr, p_ustr_next) /*UCS4*/ \
    utf8str_char_next(ustr, p_ustr_next)

#endif /* USTR_IS_SBSTR */

#endif /* __utf8str_h */

/* end of utf8str.h */
