/* xstring.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Library module for string handling */

/* SKS May 1990 */

#ifndef __xstring_h
#define __xstring_h

#define strlen_with_NULLCH      ((U32) (-1))
#define strlen_without_NULLCH   ((U32) (-2))

#if RISCOS && CROSS_COMPILE
#define u8_from_int(i) ((U8) (i))
#else
#define u8_from_int(i) (i)
#endif

/* skip spaces on byte-oriented string (either U8 or UTF-8, but not TSTR) */

#define PtrSkipSpaces(__ptr_type, ptr__ref) /*void*/ \
    while(CH_SPACE == PtrGetByte(ptr__ref)) \
        PtrIncByte(__ptr_type, ptr__ref)

/* skip spaces on characted-oriented string (either U8 or TSTR, but not UTF-8) */

#define StrSkipSpaces(ptr__ref) /*void*/ \
    while(CH_SPACE == *(ptr__ref)) \
        ++(ptr__ref)

/*
exported functions
*/

_Check_return_
_Ret_writes_bytes_maybenull_(entry_size)
extern P_ANY
_bfind(
    _In_reads_bytes_(entry_size) PC_ANY key,
    _In_reads_bytes_x_(entry_size * n_entries) PC_ANY p_start,
    _InVal_     S32 n_entries,
    _InVal_     U32 entry_size,
    _InRef_     P_PROC_BSEARCH p_proc_bsearch,
    _OutRef_    P_BOOL p_hit);

#define bfind(key, p_start, n_entries, entry_size, __base_type, p_proc_bsearch, p_hit) ( \
    (__base_type *) _bfind(key, p_start, n_entries, entry_size, p_proc_bsearch, p_hit) )

_Check_return_
_Ret_writes_bytes_maybenull_(entry_size)
extern P_ANY
xlsearch(
    _In_reads_bytes_(entry_size) PC_ANY key,
    _In_reads_bytes_x_(entry_size * n_entries) PC_ANY p_start,
    _InVal_     S32 n_entries,
    _InVal_     U32 entry_size,
    _InRef_     P_PROC_BSEARCH p_proc_bsearch);

#define lsearch(key, p_start, n_entries, entry_size, __base_type, p_proc_bsearch) ( \
    (__base_type *) xlsearch(key, p_start, n_entries, entry_size, p_proc_bsearch) )

extern void __cdecl /* declared as qsort replacement */
check_sorted(
    const void * base,
    size_t num,
    size_t width,
    int (__cdecl * p_proc_compare) (
        const void * a1,
        const void * a2));

_Check_return_
extern S32
fast_strtol(
    _In_z_      PC_U8Z p_u8_in, /* NB NOT USTR */
    _OutRef_opt_ P_P_U8Z endptr);

_Check_return_
extern U32
fast_strtoul(
    _In_z_      PC_U8Z p_u8_in, /* NB NOT USTR */
    _OutRef_opt_ P_P_U8Z endptr);

extern void
memrev32(
    _Inout_updates_bytes_x_(n_elements * element_width) P_ANY p,
    _InVal_     U32 n_elements,
    _InVal_     U32 element_width);

_Check_return_
_Ret_writes_bytes_maybenull_(size_b)
extern P_ANY
memstr32(
    _In_reads_bytes_(size_a) PC_ANY a,
    _InVal_     U32 size_a,
    _In_reads_bytes_(size_b) PC_ANY b,
    _InVal_     U32 size_b);

extern void
memswap32(
    _Inout_updates_bytes_(n_bytes) P_ANY p1,
    _Inout_updates_bytes_(n_bytes) P_ANY p2,
    _InVal_     U32 n_bytes);

#if WINDOWS

#define C_stricmp(a, b)     _stricmp(a, b)
#define C_strnicmp(a, b, n) _strnicmp(a, b, n)

#else

_Check_return_
extern int
C_stricmp(
    _In_z_      PC_U8Z a,
    _In_z_      PC_U8Z b);

_Check_return_
extern int
C_strnicmp(
    _In_z_/*n*/ PC_U8Z a,
    _In_z_/*n*/ PC_U8Z b,
    _InVal_     U32 n);

#endif /* OS */

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
sbstr_compare(
    _In_z_      PC_SBSTR sbstr_a,
    _In_z_      PC_SBSTR sbstr_b);

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
sbstr_compare_equals(
    _In_z_      PC_SBSTR sbstr_a,
    _In_z_      PC_SBSTR sbstr_b);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
sbstr_compare_n2(
    _In_z_/*sbchars_n_a*/ PC_SBSTR sbstr_a,
    _InVal_     U32 sbchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_/*sbchars_n_b*/ PC_SBSTR sbstr_b,
    _InVal_     U32 sbchars_n_b /*strlen_with,without_NULLCH*/);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
sbstr_compare_nocase(
    _In_z_      PC_SBSTR sbstr_a,
    _In_z_      PC_SBSTR sbstr_b);

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
sbstr_compare_equals_nocase(
    _In_z_      PC_SBSTR sbstr_a,
    _In_z_      PC_SBSTR sbstr_b);

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
sbstr_compare_n2_nocase(
    _In_z_/*sbchars_n_a*/ PC_SBSTR sbstr_a,
    _InVal_     U32 sbchars_n_a /*strlen_with,without_NULLCH*/,
    _In_z_/*sbchars_n_b*/ PC_SBSTR sbstr_b,
    _InVal_     U32 sbchars_n_b /*strlen_with,without_NULLCH*/);

/*
strcpy / _s() etc. replacements that ensure CH_NULL-termination
*/

/*ncr*/
extern U32 /* bytes currently in output */
xstrkat(
    _Inout_updates_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_U8Z src);

/*ncr*/
extern U32 /* bytes currently in output */
xstrnkat(
    _Inout_updates_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PC_U8 src,
    _InVal_     U32 src_n);

/*ncr*/
extern U32 /* bytes currently in output */
xstrkpy(
    _Out_writes_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_z_      PC_U8Z src);

/*ncr*/
extern U32 /* bytes currently in output */
xstrnkpy(
    _Out_writes_z_(dst_n) P_U8Z dst,
    _InVal_     U32 dst_n,
    _In_reads_or_z_(src_n) PC_U8 src,
    _InVal_     U32 src_n);

_Check_return_
extern int __cdecl
xsnprintf(
    _Out_writes_z_(dst_n) char * dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ const char * format,
    /**/        ...);

_Check_return_
extern int __cdecl
xvsnprintf(
    _Out_writes_z_(dst_n) char * dst,
    _InVal_     U32 dst_n,
    _In_z_ _Printf_format_string_ const char * format,
    /**/        va_list args);

/* 32-bit (rather than size_t) sized strlen() */
#define strlen32(str) \
    ((U32) strlen(str))

#define strlen32p1(str) \
    (1U /*CH_NULL*/ + strlen32(str))

_Check_return_
static inline U32
strlen32_n(
    _In_z_      PC_U8Z str,
    _InVal_     U32 n /*strlen_with,without_NULLCH*/)
{
    if(strlen_without_NULLCH == n)
        return(strlen32(str));

    if(strlen_with_NULLCH == n)
        return(strlen32p1(str));

    return(n);
}

#endif /* __xstring_h */

/* end of xstring.h */
