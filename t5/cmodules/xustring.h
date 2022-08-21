/* xustring.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Library module for UCHAR-based string handling */

/* SKS Apr 2014 */

#ifndef __xustring_h
#define __xustring_h

/*
exported functions
*/

_Check_return_
extern STATUS
al_ustr_append(
    _InoutRef_  P_ARRAY_HANDLE_USTR p_array_handle_ustr,
    _In_z_      PC_USTR ustr);

static inline void
al_ustr_clr(
    _InoutRef_  P_ARRAY_HANDLE_USTR p_array_handle_ustr)
{
    al_array_dispose(p_array_handle_ustr);
}

_Check_return_
extern STATUS
al_ustr_set(
    _OutRef_    P_ARRAY_HANDLE_USTR p_array_handle_ustr,
    _In_z_      PC_USTR ustr);

static inline void
ustr_clr(
    _Inout_opt_ P_P_USTR aa)
{
    al_ptr_dispose(P_P_ANY_PEDANTIC(aa));
}

_Check_return_
extern STATUS
ustr_set(
    _OutRef_    P_P_USTR aa,
    _In_opt_z_  PC_USTR b);

_Check_return_
extern STATUS
ustr_set_n(
    _OutRef_    P_P_USTR aa,
    _In_reads_opt_(uchars_n) PC_UCHARS b,
    _InVal_     U32 uchars_n);

_Check_return_
extern S32
stox(
    _In_z_      PC_USTR ustr,
    _OutRef_    P_S32 p_col);

/*ncr*/
extern U32
xtos_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     S32 x,
    _InVal_     BOOL upper_case);

#if USTR_IS_SBSTR

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
uchars_compare_t5_nocase(
    _In_reads_(uchars_n_a) PC_UCHARS uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UCHARS uchars_b,
    _InVal_     U32 uchars_n_b);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
uchars_compare_t5_nocase_wild(
    _In_reads_(uchars_n_a) PC_UCHARS uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UCHARS uchars_b,
    _InVal_     U32 uchars_n_b);

#endif /* USTR_IS_SBSTR */

/*ncr*/
extern U32
sbstr_buf_from_ustr(
    _Out_writes_z_(elemof_buffer) P_SBSTR buffer,
    _InVal_     U32 elemof_buffer,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage,
    _In_z_      PC_USTR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/);

#define uchars_IncByte(uchars__ref) \
    PtrIncByte(PC_UCHARS, uchars__ref)

#define uchars_IncByte_wr(uchars_wr__ref) \
    PtrIncByte(P_UCHARS, uchars_wr__ref)

#define uchars_IncBytes(uchars__ref, add) \
    PtrIncBytes(PC_UCHARS, uchars__ref, add)

#define uchars_IncBytes_wr(uchars_wr__ref, add) \
    PtrIncBytes(P_UCHARS, uchars_wr__ref, add)

#define uchars_DecByte(uchars__ref) \
    PtrDecByte(PC_UCHARS, uchars__ref)

#define uchars_AddBytes(uchars, add) \
    PtrAddBytes(PC_UCHARS, uchars, add)

#define uchars_AddBytes_wr(uchars_wr, add) \
    PtrAddBytes(P_UCHARS, uchars_wr, add)

#if USTR_IS_SBSTR

#define uchars_validate(func, uchars, n_bytes) /*nothing*/

#define tstr_buf_from_ustr(buffer, elemof_buffer, ustr, n_bytes) \
    sbstr_buf_from_ustr(buffer, elemof_buffer, get_system_codepage(), ustr, n_bytes)

#else /* NOT USTR_IS_SBSTR */

/* UCHARS to UTF8 transitional macros defined in utf8.h */

#endif /* USTR_IS_SBSTR */

#define ustr_IncByte(ustr__ref) \
    PtrIncBytes(PC_USTR, ustr__ref, 1)

#define ustr_IncByte_wr(ustr_wr__ref) \
    PtrIncBytes(P_USTR, ustr_wr__ref, 1)

#define ustr_IncBytes(ustr__ref, add) \
    PtrIncBytes(PC_USTR, ustr__ref, add)

#define ustr_IncBytes_wr(ustr_wr__ref, add) \
    PtrIncBytes(P_USTR, ustr_wr__ref, add)

#define ustr_DecByte(ustr__ref) \
    PtrDecBytes(PC_USTR, ustr__ref, 1)

#define ustr_DecByte_wr(ustr__ref) \
    PtrDecBytes(P_USTR, ustr__ref, 1)

#define ustr_AddBytes(ustr, add) \
    PtrAddBytes(PC_USTR, ustr, add)

#define ustr_AddBytes_wr(ustr_wr, add) \
    PtrAddBytes(P_USTR, ustr_wr, add)

#define ustr_SkipSpaces(ustr__ref) \
    PtrSkipSpaces(PC_USTR, ustr__ref)

#if USTR_IS_SBSTR

#define _sbstr_from_ustr(ustr)      ((PC_SBSTR) (ustr)) /* no conversion is required */
/* #define _ustr_from_sbstr(sbstr)     ((PC_USTR) (sbstr)) */ /* appears to be unused */

#define ustrlen32(ustr) /*U32*/ \
    strlen32(ustr)

#define ustrlen32p1(ustr) /*U32*/ \
    strlen32p1(ustr)

#define ustrlen32_n(ustr, n_bytes) /*U32*/ \
    strlen32_n(ustr, n_bytes)

#define ustrchr(s, c)               strchr(s, c)
#define ustrpbrk(s1, s2)            strpbrk(s1, s2)

#define ustr_xstrkat(dst, dst_n, src) /*U32*/ \
    xstrkat(dst, dst_n, src)

#define ustr_xstrnkat(dst, dst_n, src, src_n) /*U32*/ \
    xstrnkat(dst, dst_n, src, src_n)

#define ustr_xstrkpy(dst, dst_n, src) /*U32*/ \
    xstrkpy(dst, dst_n, src) \

#define ustr_xstrnkpy(dst, dst_n, src, src_n) /*U32*/ \
    xstrnkpy(dst, dst_n, src, src_n)

#define ustr_compare(ustr_a, ustr_b) /*int*/ \
    sbstr_compare(ustr_a, ustr_b)

#define ustr_compare_equals(ustr_a, ustr_b) /*BOOL*/ \
    sbstr_compare_equals(ustr_a, ustr_b)

#define ustr_compare_n2(ustr_a, uchars_n_a, ustr_b, uchars_n_b) /*int*/ \
    sbstr_compare_n2(ustr_a, uchars_n_a, ustr_b, uchars_n_b)

#define ustr_compare_nocase(ustr_a, ustr_b) /*int*/ \
    sbstr_compare_nocase(ustr_a, ustr_b)

#define ustr_compare_equals_nocase(ustr_a, ustr_b) /*BOOL*/ \
    sbstr_compare_equals_nocase(ustr_a, ustr_b)

#define ustr_compare_n2_nocase(ustr_a, uchars_n_a, ustr_b, uchars_n_b) /*int*/ \
    sbstr_compare_n2_nocase(ustr_a, uchars_n_a, ustr_b, uchars_n_b)

#define ustr_xsnprintf \
    xsnprintf

#define ustr_xvsnprintf \
    xvsnprintf

#define ustr_validate(func, ustr) /*nothing*/
#define ustr_validate_n(func, ustr_inline, uchars_n) /*nothing*/

#else /* NOT USTR_IS_SBSTR */

/* USTR to UTF8STR transitional macros defined in utf8str.h */

#endif /* USTR_IS_SBSTR */

#if (TSTR_IS_SBSTR && USTR_IS_SBSTR)

#define _tstr_from_ustr(ustr) ((PCTSTR) (ustr)) /* no conversion is required */
#define _ustr_from_tstr(tstr) ((PC_USTR) (tstr))

#else /* TSTR_IS_SBSTR && USTR_IS_SBSTR */

_Check_return_
_Ret_z_ /* never NULL */
extern PCTSTR /*low-lifetime*/
_tstr_from_ustr(
    _In_z_      PC_USTR ustr);

_Check_return_
_Ret_z_ /* never NULL */
extern PC_USTR /*low-lifetime*/
_ustr_from_tstr(
    _In_z_      PCTSTR tstr);

#endif /* TSTR_IS_SBSTR && USTR_IS_SBSTR */

#endif /* __xustring_h */

/* end of xustring.h */
