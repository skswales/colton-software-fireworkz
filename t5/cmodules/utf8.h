/* utf8.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2006-2020 Stuart Swales */

#ifndef __utf8_h
#define __utf8_h

#if CHECKING && 1
#define CHECKING_UTF8 1
#else
#define CHECKING_UTF8 0
#endif

#if USTR_IS_SBSTR
#define CHECKING_UCHARS 0
#else
#define CHECKING_UCHARS CHECKING_UTF8
#endif

/*
exported functions
*/

#define utf8_IncBytes(ptr__ref, add) \
    PtrIncBytes(PC_UTF8, ptr__ref, add) \

#define utf8_IncBytes_wr(ptr_wr__ref, add) \
    PtrIncBytes(P_UTF8, ptr_wr__ref, add) \

#define utf8_AddBytes(uchars, add) \
    PtrAddBytes(PC_UTF8, uchars, add)

#define utf8_AddBytes_wr(uchars_wr, add) \
    PtrAddBytes(P_UTF8, uchars_wr, add)

/*
is a byte a UTF-8 lead byte?
*/

/* this one is used almost excusively for debugging */
_Check_return_
static inline BOOL
u8_is_utf8_lead_byte(
    _InVal_     U8 u8)
{
    return( ((u8 & 0xE0) == 0xC0) ||
            ((u8 & 0xF0) == 0xE0) ||
            ((u8 & 0xF8) == 0xF0) );
}

/*
is a byte a UTF-8 trail byte?
*/

/* this one is used all the time */
_Check_return_
static inline BOOL
u8_is_utf8_trail_byte(
    _InVal_     U8 u8)
{
    return( ((u8 & 0xC0) == 0x80) );
}

/*
number of bytes of UTF-8 encoding of the character pointed to
*/

/* single ASCII-7 bytes retrieved without function call overhead */

/* NL variant is not required as total length is encoded in the first byte */

#define utf8_bytes_of_char(uchars) /*U32 num*/ (    \
    u8_is_ascii7(PtrGetByte(uchars))                \
    ? 1U                                            \
    : utf8__bytes_of_char(uchars)                   )

#define utf8_bytes_of_char_off(uchars, off) /*U32 num*/ (   \
    u8_is_ascii7(PtrGetByteOff(uchars, off))                \
    ? 1U                                                    \
    : utf8__bytes_of_char_off(uchars, off)                  )

_Check_return_
extern U32
utf8__bytes_of_char(
    _In_        PC_UTF8 uchars);

_Check_return_
static inline U32
utf8__bytes_of_char_off(
    _In_        PC_UTF8 uchars,
    _InVal_     U32 offset)
{
    return(utf8__bytes_of_char(utf8_AddBytes(uchars, offset)));
}

/* single ASCII-7 bytes retrieved without function call overhead */
#define utf8_bytes_prev_of_char(uchars_start, uchars) /*U32 num*/ ( \
    (   (uchars_start != uchars)                    &&              \
        u8_is_ascii7(PtrGetByteOff(uchars, -1))   )                 \
        ? 1U                                                        \
        : utf8__bytes_prev_of_char(uchars_start, uchars)            )

#define utf8_bytes_prev_of_char_NS(uchars) /*U32 num*/ (        \
        u8_is_ascii7(PtrGetByteOff(uchars, -1))                 \
        ? 1U                                                    \
        : utf8__bytes_prev_of_char_NS(uchars)                   )

_Check_return_
extern U32
utf8__bytes_prev_of_char(
    _InRef_     PC_UTF8 uchars_start,
    _InRef_     PC_UTF8 uchars);

_Check_return_
extern U32
utf8__bytes_prev_of_char_NS(
    _InRef_     PC_UTF8 uchars);

/*
number of bytes
*/

_Check_return_
extern U32
utf8_bytes_of_chars(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n,
    _InVal_     U32 n_chars);

/*
1..4 bytes (more if grapheme cluster) of UTF-8 character encoding
can be copied without function call overhead
*/
static inline void
utf8_char_copy(
    _Out_writes_all_(uchars_n) P_UTF8 dst_uchars,
    _In_reads_(uchars_n) PC_UTF8 src_uchars,
    _InVal_     U32 uchars_n)
{
    P_BYTE dst_byte = (P_BYTE) dst_uchars;
    PC_BYTE src_byte = (PC_BYTE) src_uchars;
    U32 n_bytes_remain = uchars_n;
    assert(n_bytes_remain);
    while(n_bytes_remain--)
        *dst_byte++ = *src_byte++;
}

/*
decode UCS-4 character from UTF-8 character encoding, with number of bytes
*/

/* single ASCII-7 bytes retrieved without function call overhead */
#define utf8_char_decode(uchars, bytes_of_char__ref) /*UCS4*/ (     \
    u8_is_ascii7(PtrGetByte(uchars))                                \
        ? ( (bytes_of_char__ref) = 1U, (UCS4) PtrGetByte(uchars) )  \
        : utf8__char_decode(uchars, &(bytes_of_char__ref))          )

#define utf8_char_decode_off(uchars, off, bytes_of_char__ref) /*UCS4*/ (    \
    u8_is_ascii7(PtrGetByteOff(uchars, off))                                \
        ? ( (bytes_of_char__ref) = 1U, (UCS4) PtrGetByteOff(uchars, off) )  \
        : utf8__char_decode_off(uchars, off, &(bytes_of_char__ref))         )

#define utf8_char_decode_NULL(uchars) /*UCS4*/ (    \
    u8_is_ascii7(PtrGetByte(uchars))                \
        ? (UCS4) PtrGetByte(uchars)                 \
        : utf8__char_decode(uchars, NULL)           )

#define utf8_char_decode_off_NULL(uchars, off) /*UCS4*/ (   \
    u8_is_ascii7(PtrGetByteOff(uchars, off))                \
        ? (UCS4) PtrGetByteOff(uchars, off)                 \
        : utf8__char_decode_off(uchars, off, NULL)          )

_Check_return_
extern UCS4
utf8__char_decode(
    _In_        PC_UTF8 uchars,
    _Out_opt_   P_U32 p_bytes_of_char);

_Check_return_
static inline UCS4
utf8__char_decode_off(
    _In_        PC_UTF8 uchars,
    _InVal_     U32 offset,
    _Out_opt_   P_U32 p_bytes_of_char)
{
    return(utf8__char_decode(utf8_AddBytes(uchars, offset), p_bytes_of_char));
}

/*
encode UCS-4 character as UTF-8 character encoding
*/

/* single ASCII-7 bytes poked direct to output buffer without function call overhead */
#define utf8_char_encode(utf8_buf, elemof_buffer, ucs4) /*U32 num*/ (   \
    (ucs4_is_ascii7(ucs4) && (0 != elemof_buffer))                      \
        ? ( PtrPutByte(utf8_buf, (U8) (ucs4)), 1U )                     \
        : utf8__char_encode_off(utf8_buf, elemof_buffer, 0, ucs4)       )

/* single ASCII-7 bytes poked direct to given offset in output buffer without function call overhead */
#define utf8_char_encode_off(utf8_buf, elemof_buffer, encode_offset, ucs4) /*U32 num*/ (    \
    (ucs4_is_ascii7(ucs4) && (encode_offset < elemof_buffer))                               \
        ? ( PtrPutByteOff(utf8_buf, encode_offset, (U8) (ucs4)), 1U )                       \
        : utf8__char_encode_off(utf8_buf, elemof_buffer, encode_offset, ucs4)               )

_Check_return_
extern U32 /* number of bytes */
utf8__char_encode_off(
    _Out_writes_(elemof_buffer) P_UTF8 utf8_buf /*filled at encode_offset*/,
    _InVal_     U32 elemof_buffer,
    _InVal_     U32 encode_offset,
    _In_        UCS4 ucs4);

/*
number of bytes required to encode UCS-4 character as UTF-8 character encoding
*/

/* single ASCII-7 character sized without function call overhead */
#define utf8_bytes_of_char_encoding(ucs4) /*U32 num*/ ( \
    ucs4_is_ascii7(ucs4)                                \
    ? 1U                                                \
    : utf8__bytes_of_char_encoding(ucs4)                )

_Check_return_
extern U32 /* number of bytes */
utf8__bytes_of_char_encoding(
    _In_        UCS4 ucs4);

_Check_return_
extern P_UTF8
utf8_char_find(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n,
    _In_        UCS4 ucs4);

_Check_return_
extern P_UTF8
utf8_char_find_rev(
    _In_reads_(uchars_n) PC_UTF8 uchars_start,
    _InVal_     U32 uchars_n,
    _In_        UCS4 ucs4);

_Check_return_
extern UCS4
utf8_char_next(
    _In_        PC_UTF8 uchars,
    _OutRef_    P_PC_UTF8 p_uchars_next);

_Check_return_
extern UCS4
utf8_char_prev(
    _In_opt_    PC_UTF8 uchars_start, /* NULL only for utf8__bytes_prev_of_grapheme_cluster_NS() */
    _In_        PC_UTF8 uchars,
    _OutRef_    P_PC_UTF8 p_uchars_prev);

/*
number of UCS-4 characters represented by uchars_n bytes of UTF-8 encoding
*/

_Check_return_
extern U32 /*num*/
utf8_chars_of_bytes(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n);

/*
grapheme cluster functions
*/

/*
total number of bytes of UTF-8 encoding of the characters constituting grapheme cluster pointed to

uchars_n == strlen_without_NULLCH abused to mean don't limit
*/

/* this macro is optimised for 'normal' case where text is a number of bytes, mostly ASCII-7 */
/* not usually called with zero bytes as this is usually used in an outer byte count loop */

/* if more than one byte, we can read the next pair of bytes: if both are ASCII-7 then GrCluster size is one */
/* if just one byte, we can read the next single byte: if it is ASCII-7 then GrCluster size is one */
/* otherwise call the function to work it out */
#define utf8_bytes_of_grapheme_cluster(uchars, uchars_n) /*U32 num*/ (                          \
    (   ((uchars_n >  1) && u8_is_ascii7(PtrGetByte(uchars) | PtrGetByteOff(uchars, 1))) ||     \
        ((uchars_n == 1) && u8_is_ascii7(PtrGetByte(uchars)))                                )  \
        ? 1U                                                                                    \
        : utf8__bytes_of_grapheme_cluster(uchars, uchars_n)                                     )

/* simple version for CH_NULL-terminated strings, see notes */
#define utf8_bytes_of_grapheme_cluster_NC(uchars) /*U32 num*/ (             \
    (   (CH_NULL == PtrGetByte(uchars))                                ||   \
        (u8_is_ascii7(PtrGetByte(uchars) | PtrGetByteOff(uchars, 1)))     ) \
        ? 1U                                                                \
        : utf8__bytes_of_grapheme_cluster_NC(uchars)                        )

_Check_return_
extern U32
utf8__bytes_of_grapheme_cluster(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n);

_Check_return_
extern U32
utf8__bytes_of_grapheme_cluster_NC(
    _In_        PC_UTF8 uchars);

#define utf8_bytes_prev_of_grapheme_cluster(uchars_start, uchars) /*U32 num*/ ( \
    (   (uchars_start != uchars)                    &&                          \
        u8_is_ascii7(PtrGetByteOff(uchars, -1))     )                           \
        ? 1U                                                                    \
        : utf8__bytes_prev_of_grapheme_cluster(uchars_start, uchars)            )

#define utf8_bytes_prev_of_grapheme_cluster_NS(uchars) /*U32 num*/ (    \
    u8_is_ascii7(PtrGetByteOff(uchars, -1))                             \
        ? 1U                                                            \
        : utf8__bytes_prev_of_grapheme_cluster_NS(uchars)               )

_Check_return_
extern U32
utf8__bytes_prev_of_grapheme_cluster(
    _InRef_     PC_UTF8 uchars_start,
    _InRef_     PC_UTF8 uchars);

_Check_return_
extern U32
utf8__bytes_prev_of_grapheme_cluster_NS(
    _InRef_     PC_UTF8 uchars);

/*
number of bytes
*/

_Check_return_
extern U32
utf8_bytes_of_grapheme_clusters(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n,
    _InVal_     U32 n_grapheme_clusters);

#define utf8_grapheme_cluster_copy(dst, src, uchars_n) /*void*/ \
    utf8_char_copy(dst, src, uchars_n)

/* single ASCII-7 bytes retrieved without function call overhead */

/* this macro is optimised for 'normal' case where text is a number of bytes, mostly ASCII-7 */
/* not usually called with zero bytes as this is usually used in an outer byte count loop */

/* if more than one byte, we can read the next pair of bytes: if both are ASCII-7 then GrCluster size is one */
/* if just one byte, we can read the next single byte: if it is ASCII-7 then GrCluster size is one */
/* otherwise call the function to work it out */
#define utf8_grapheme_cluster_decode(uchars, uchars_n, bytes_of_char__ref, bytes_of_grapheme_cluster__ref) /*UCS4*/ (   \
    (   ((uchars_n >  1) && u8_is_ascii7(PtrGetByte(uchars) | PtrGetByteOff(uchars, 1))) ||                             \
        ((uchars_n == 1) && u8_is_ascii7(PtrGetByte(uchars)))                             )                             \
        ? ( (bytes_of_grapheme_cluster__ref) = (bytes_of_char__ref) = 1U, (UCS4) PtrGetByte(uchars) )                   \
        : utf8__grapheme_cluster_decode(uchars, uchars_n, &(bytes_of_char__ref), &(bytes_of_grapheme_cluster__ref))     )

_Check_return_
extern UCS4
utf8__grapheme_cluster_decode(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n,
    _OutRef_    P_U32 p_bytes_of_char,
    _OutRef_    P_U32 p_bytes_of_grapheme_cluster);

_Check_return_
extern U32 /* number of grapheme clusters represented by uchars_n bytes of UTF-8 encoding */
utf8_grapheme_clusters_of_bytes(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n);

/*
case-sensitive UTF-8 character sequence comparision
*/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
utf8_compare(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b);

/*
case-insensitive UTF-8 character sequence comparision
*/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
utf8_compare_nocase(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b);

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
utf8_compare_equals_nocase(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
utf8_compare_nocase_CF1(
    _In_reads_(uchars_n_a) PC_UCS4 p_ucs4_a,
    _InVal_     U32 uchars_n_a /*elements*/,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b /*bytes*/);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
utf8_compare_nocase_wild(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b);

#if !USTR_IS_SBSTR

_Check_return_
extern STATUS
utf8_case_fold(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n);

#endif /* !USTR_IS_SBSTR */

_Check_return_
extern STATUS /* STATUS_OK, error if not */
utf8_validate(
    _In_z_      PCTSTR func,
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n);

/*ncr*/
extern U32
utf8_from_tchars(
    _Out_writes_opt_(elemof_buffer) P_UTF8 uchars_buf, /*NULL->count*/
    _InVal_     U32 elemof_buffer,
    _In_reads_(tchars_n) PCTCH tchars,
    _InVal_     U32 tchars_n);

_Check_return_
extern U32
sbchars_from_utf8_bytes_needed(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n,
    _OutRef_    P_BOOL is_pure_ascii7);

_Check_return_
extern U32
sbchars_from_utf8(
    _Out_writes_opt_(elemof_buffer) P_SBCHARS sbchars_buf, /*NULL->count*/
    _InVal_     U32 elemof_buffer,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage,
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n);

#if WINDOWS

/*
ho_utf8.c
*/

_Check_return_
extern STATUS
utf8_ExtTextOut(
    _HdcRef_    HDC hdc,
    _In_        int x,
    _In_        int y,
    _In_        UINT options,
    _In_opt_    CONST RECT *pRect,
    _In_reads_opt_(uchars_n) PC_UTF8 pString,
    _InVal_     U32 uchars_n,
    _In_opt_    CONST INT *pDx);

_Check_return_
extern STATUS
utf8_GetTextExtentPoint32(
    _HdcRef_    HDC hdc,
    _In_reads_(uchars_n) PC_UTF8 pString,
    _InVal_     U32 uchars_n,
    _OutRef_    PSIZE pSize);

#endif /* OS */

/*
UCHARS to UTF8 transitional macros
*/

#if USTR_IS_SBSTR

/* corresponding macros are defined in xustring.h */

#else /* NOT USTR_IS_SBSTR */

#define uchars_compare_t5_nocase(uchars_a, uchars_n_a, uchars_b, uchars_n_b) \
    utf8_compare_nocase(uchars_a, uchars_n_a, uchars_b, uchars_n_b)

#define uchars_compare_t5_nocase_wild(uchars_a, uchars_n_a, uchars_b, uchars_n_b) \
    utf8_compare_nocase_wild(uchars_a, uchars_n_a, uchars_b, uchars_n_b)

#define uchars_validate(func, uchars, uchars_n) \
    utf8_validate(func, uchars, uchars_n)

#endif /* USTR_IS_SBSTR */

/*
number of bytes of UCHARS encoding of the character pointed to
*/

#if USTR_IS_SBSTR

#define uchars_bytes_of_char(uchars) /*U32 num*/ \
    1U

#define uchars_bytes_of_char_off(uchars, off) /*U32 num*/ \
    1U

#else /* NOT USTR_IS_SBSTR */

#define uchars_bytes_of_char(uchars) /*U32 num*/ \
    utf8_bytes_of_char(uchars)

#define uchars_bytes_of_char_off(uchars, off) /*U32 num*/ \
    utf8_bytes_of_char_off(uchars, off)

#endif /* USTR_IS_SBSTR */

#if USTR_IS_SBSTR

#define uchars_bytes_prev_of_char(uchars_start, uchars) /*U32 num*/ \
    ( (uchars_start != uchars) ? 1U : 0U )

#define uchars_bytes_prev_of_char_NS(uchars) /*U32 num*/ \
    1U

#else /* NOT USTR_IS_SBSTR */

#define uchars_bytes_prev_of_char(uchars_start, uchars) /*U32 num*/ \
    utf8_bytes_prev_of_char(uchars_start, uchars)

#define uchars_bytes_prev_of_char_NS(uchars) /*U32 num*/ \
    utf8_bytes_prev_of_char_NS(uchars)

#endif /* USTR_IS_SBSTR */

/*
number of bytes
*/

#if USTR_IS_SBSTR

#define uchars_bytes_of_chars(uchars, uchars_n, n_chars) /*U32 num*/ \
    min(uchars_n, n_chars)

#else /* NOT USTR_IS_SBSTR */

#define uchars_bytes_of_chars(uchars, uchars_n, n_chars) /*U32 num*/ \
    utf8_bytes_of_chars(uchars, uchars_n, n_chars)

#endif /* USTR_IS_SBSTR */

#if USTR_IS_SBSTR

static inline void
uchars_char_copy(
    _Out_writes_all_(uchars_n) P_UCHARS dst_uchars,
    _In_reads_(uchars_n) PC_UCHARS src_uchars,
    _InVal_     U32 uchars_n)
{
    UNREFERENCED_PARAMETER_InVal_(uchars_n);
    assert(1 == uchars_n);
    PtrPutByte(dst_uchars, PtrGetByte(src_uchars));
}

#else /* NOT USTR_IS_SBSTR */

#define uchars_char_copy(dst_uchars, src_uchars, uchars_n) /*void*/ \
    utf8_char_copy(dst_uchars, src_uchars, uchars_n)

#endif /* USTR_IS_SBSTR */

/*
decode UCS-4 character from UCHARS character encoding, with number of bytes
*/

#if USTR_IS_SBSTR

#define uchars_char_decode(uchars, bytes_of_char__ref) /*UCS4*/ \
    ( (bytes_of_char__ref) = 1U, (UCS4) PtrGetByte(uchars) )

#define uchars_char_decode_off(uchars, off, bytes_of_char__ref) /*UCS4*/ \
    ( (bytes_of_char__ref) = 1U, (UCS4) PtrGetByteOff(uchars, off) )

#define uchars_char_decode_NULL(uchars) /*UCS4*/ \
    (UCS4) PtrGetByte(uchars)

#define uchars_char_decode_off_NULL(uchars, off) /*UCS4*/ \
    (UCS4) PtrGetByteOff(uchars, off)

#else /* NOT USTR_IS_SBSTR */

#define uchars_char_decode(uchars, bytes_of_char__ref) /*UCS4*/ \
    utf8_char_decode(uchars, bytes_of_char__ref)

#define uchars_char_decode_off(uchars, off, bytes_of_char__ref) /*UCS4*/ \
    utf8_char_decode_off(uchars, off, bytes_of_char__ref)

#define uchars_char_decode_NULL(uchars) /*UCS4*/ \
    utf8_char_decode_NULL(uchars)

#define uchars_char_decode_off_NULL(uchars, off) /*UCS4*/ \
    utf8_char_decode_off_NULL(uchars, off)

#endif /* USTR_IS_SBSTR */

/*
encode UCS-4 character as UCHARS character encoding
*/

#if USTR_IS_SBSTR

/* single bytes poked direct to output buffer */
#define uchars_char_encode(uchars, elemof_buffer, ucs4) /*U32 num*/ (   \
    ((0 != elemof_buffer))                                              \
        ? ( PtrPutByte(uchars, (U8) (ucs4)), 1U )                       \
        : 1U                                                            )

/* single bytes poked direct to given offset in output buffer */
#define uchars_char_encode_off(uchars, elemof_buffer, encode_offset, ucs4) /*U32 num*/ (    \
    (encode_offset < elemof_buffer)                                                         \
        ? ( PtrPutByteOff(uchars, encode_offset, (U8) (ucs4)), 1U )                         \
        : 1U                                                                                )

#else /* NOT USTR_IS_SBSTR */

#define uchars_char_encode(uchars, elemof_buffer, ucs4) /*U32 num*/ \
    utf8_char_encode(uchars, elemof_buffer, ucs4)

#define uchars_char_encode_off(uchars, elemof_buffer, encode_offset, ucs4) /*U32 num*/ \
    utf8_char_encode_off(uchars, elemof_buffer, encode_offset, ucs4)

#endif /* USTR_IS_SBSTR */

/*
number of bytes required to encode UCS-4 character as UCHARS character encoding
*/

#if USTR_IS_SBSTR

#define uchars_bytes_of_char_encoding(ucs4) /*U32 num*/ \
    1U

#else /* NOT USTR_IS_SBSTR */

/* single ASCII-7 character sized without function call overhead */
#define uchars_bytes_of_char_encoding(ucs4) /*U32 num*/ \
    utf8_bytes_of_char_encoding(ucs4)

#endif /* USTR_IS_SBSTR */

/*
number of UCS-4 characters represented by uchars_n bytes of UCHARS encoding
*/

#if USTR_IS_SBSTR

#define uchars_chars_of_bytes(uchars, uchars_n) /*U32 num*/ \
    (uchars_n)

#else /* NOT USTR_IS_SBSTR */

#define uchars_chars_of_bytes(uchars, uchars_n) /*U32 num*/ \
    utf8_chars_of_bytes(uchars, uchars_n)

#endif /* USTR_IS_SBSTR */

/*
grapheme cluster functions
*/

/*
total number of bytes of UCHARS/USTR encoding of the characters constituting grapheme cluster pointed to

uchars_n == strlen_without_NULLCH abused to mean don't limit
*/

#if USTR_IS_SBSTR

#define uchars_bytes_of_grapheme_cluster_NC(uchars) /*U32 num*/ \
    1U

#define uchars_bytes_of_grapheme_cluster(uchars, uchars_n) /*U32 num*/ \
    1U

#define uchars_bytes_prev_of_grapheme_cluster_NC(uchars) /*U32 num*/ \
    1U

#define uchars_bytes_prev_of_grapheme_cluster(uchars_start, uchars) /*U32 num*/ \
    ( (uchars_start != uchars) ? 1U : 0U )

#else /* NOT USTR_IS_SBSTR */

#define uchars_bytes_of_grapheme_cluster_NC(uchars) /*U32 num*/ \
    utf8_bytes_of_grapheme_cluster_NC(uchars)

#define uchars_bytes_of_grapheme_cluster(uchars, uchars_n) /*U32 num*/ \
    utf8_bytes_of_grapheme_cluster(uchars, uchars_n)

#define uchars_bytes_prev_of_grapheme_cluster_NS(uchars) /*U32 num*/ \
    utf8_bytes_prev_of_grapheme_cluster_NS(uchars)

#define uchars_bytes_prev_of_grapheme_cluster(uchars_start, uchars) /*U32 num*/ \
    utf8_bytes_prev_of_grapheme_cluster(uchars_start, uchars)

#endif /* USTR_IS_SBSTR */

/*
number of bytes
*/

#if USTR_IS_SBSTR

#define uchars_bytes_of_grapheme_clusters(uchars, uchars_n, n_grapheme_clusters) /*U32 num*/ \
    min(uchars_n, n_grapheme_clusters)

#else /* NOT USTR_IS_SBSTR */

#define uchars_bytes_of_grapheme_clusters(uchars, uchars_n, n_grapheme_clusters) /*U32 num*/ \
    utf8_bytes_of_grapheme_clusters(uchars, uchars_n, n_grapheme_clusters) 

#endif /* USTR_IS_SBSTR */

#if USTR_IS_SBSTR

#define uchars_grapheme_cluster_copy(dst, src, uchars_n) /*void*/ \
    uchars_char_copy(dst, src, uchars_n)

#else /* NOT USTR_IS_SBSTR */

#define uchars_grapheme_cluster_copy(dst, src, uchars_n) /*void*/ \
    utf8_grapheme_cluster_copy(dst, src, uchars_n)

#endif /* USTR_IS_SBSTR */

#if USTR_IS_SBSTR

#define uchars_grapheme_cluster_decode(uchars, uchars_n, bytes_of_char__ref, bytes_of_grapheme_cluster__ref) /*UCS4*/ \
    ( (bytes_of_grapheme_cluster__ref) = (bytes_of_char__ref) = 1U, (UCS4) PtrGetByte(uchars) )

#else /* NOT USTR_IS_SBSTR */

#define uchars_grapheme_cluster_decode(uchars, uchars_n, bytes_of_char__ref, bytes_of_grapheme_cluster__ref) /*UCS4*/ \
    utf8_grapheme_cluster_decode(uchars, uchars_n, bytes_of_char__ref, bytes_of_grapheme_cluster__ref)

#endif /* USTR_IS_SBSTR */

#if USTR_IS_SBSTR

#define uchars_grapheme_clusters_of_bytes(uchars, uchars_n) /*U32 num*/ \
    (uchars_n)

#else /* NOT USTR_IS_SBSTR */

#define uchars_grapheme_clusters_of_bytes(uchars, uchars_n) /*U32 num*/ \
    utf8_grapheme_clusters_of_bytes(uchars, uchars_n)

#endif /* USTR_IS_SBSTR */

#if WINDOWS

#if USTR_IS_SBSTR

/* use uchars STATUS returning functions */

#else /* NOT USTR_IS_SBSTR */

#define uchars_ExtTextOut(hdc, x, y, options, lprect, lpString, uchars_n, lpDx) \
    utf8_ExtTextOut(hdc, x, y, options, lprect, lpString, uchars_n, lpDx)

#define uchars_GetTextExtentPoint32(hdc, pString, uchars_n, pSize) \
    utf8_GetTextExtentPoint32(hdc, pString, uchars_n, pSize)

#endif /* USTR_IS_SBSTR */

#endif /* OS */

#endif /* __utf8_h */

/* end of utf8.h */
