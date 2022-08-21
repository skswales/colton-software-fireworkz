/* quickublk.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header file for quickublk.c (UCHARS-based quick block) */

/* SKS May 2014 split off from quickblk.h */

#ifndef __quickublk_h
#define __quickublk_h

#if CHECKING && 1
#define CHECKING_QUICK_UBLOCK 1
#else
#define CHECKING_QUICK_UBLOCK 0
#endif

typedef struct QUICK_UBLOCK
{
    ARRAY_HANDLE ub_h_array_buffer;
    U32 ub_static_buffer_size; /* NB in bytes */
    U32 ub_static_buffer_used; /* NB in bytes */
    P_UCHARS ub_p_static_buffer; /* SKS 20141210 reordered for easier MSVC debugging */
}
QUICK_UBLOCK, * P_QUICK_UBLOCK; typedef const QUICK_UBLOCK * PC_QUICK_UBLOCK;

#define P_QUICK_UBLOCK_NONE _P_DATA_NONE(P_QUICK_UBLOCK)

#define QUICK_UBLOCK_WITH_BUFFER(name, size) \
    QUICK_UBLOCK name; UCHARB name ## _buffer[size]

/* NB buffer ^^^ immediately follows QB in a structure for subsequent fixup */

#define QUICK_UBLOCK_INIT_NULL() \
    { 0, 0, 0, NULL }

#define quick_ublock_array_handle_ref(p_quick_ublock) \
    (p_quick_ublock)->ub_h_array_buffer /* really only internal use but needed for QB sort, no brackets */

/*
functions as macros
*/

#if CHECKING_QUICK_UBLOCK || CHECKING_FOR_CODE_ANALYSIS

static __forceinline void
_do_uqb_fill(
    _Out_writes_all_(ubuf_size) P_UCHARS ubuf,
    _InVal_     U32 ubuf_size)
{
    (void) memset(ubuf, 0x7F, ubuf_size); /* something not top-bit-set */
}

#else
#define _do_uqb_fill(ubuf, ubuf_size) /*nothing*/
#endif /* CHECKING_QUICK_UBLOCK */

static inline void
_quick_ublock_setup(
    _OutRef_    P_QUICK_UBLOCK p_quick_ublock /*set up*/,
    _Out_writes_all_(ubuf_size) P_UCHARS ubuf,
    _InVal_     U32 ubuf_size)
{
    p_quick_ublock->ub_h_array_buffer     = 0;
    p_quick_ublock->ub_static_buffer_size = ubuf_size;
    p_quick_ublock->ub_static_buffer_used = 0;
    p_quick_ublock->ub_p_static_buffer    = ubuf;

    _do_uqb_fill(p_quick_ublock->ub_p_static_buffer, p_quick_ublock->ub_static_buffer_size);
}

/* NB buffer for setup must be a UCHARB buffer[] */

#define quick_ublock_with_buffer_setup(name /*ref*/) \
    _quick_ublock_setup(&name, uchars_bptr(name ## _buffer), sizeof32(name ## _buffer))

#define quick_ublock_setup(p_quick_ublock, ubuf) \
    _quick_ublock_setup((p_quick_ublock), uchars_bptr(ubuf), sizeof32(ubuf))

/* set up a quick_ublock from an existing buffer (no _do_uqb_fill - see csv_quick_ublock_setup), not cleared for CHECKING_QUICK_UBLOCK */
#define quick_ublock_setup_without_clearing_ubuf(p_quick_ublock, ubuf, ubuf_size) (     \
    (p_quick_ublock)->ub_h_array_buffer     = 0,                                        \
    (p_quick_ublock)->ub_static_buffer_size = (ubuf_size),                              \
    (p_quick_ublock)->ub_static_buffer_used = 0,                                        \
    (p_quick_ublock)->ub_p_static_buffer    = (ubuf)                                    )

/* set up a quick_ublock from an existing array handle, not cleared for CHECKING_QUICK_UBLOCK */
#define quick_ublock_setup_using_array(p_quick_ublock, handle) (                        \
    (p_quick_ublock)->ub_h_array_buffer     = (handle),                                 \
    (p_quick_ublock)->ub_static_buffer_size = 0,                                        \
    (p_quick_ublock)->ub_static_buffer_used = 0,                                        \
    (p_quick_ublock)->ub_p_static_buffer    = NULL                                      )

/* set up a quick_ublock from an existing buffer, not cleared for CHECKING_QUICK_UBLOCK */
#define quick_ublock_setup_fill_from_ubuf(p_quick_ublock, ubuf, ubuf_size) (            \
    (p_quick_ublock)->ub_h_array_buffer     = 0,                                        \
    (p_quick_ublock)->ub_static_buffer_size = (ubuf_size),                              \
    (p_quick_ublock)->ub_static_buffer_used = (p_quick_ublock)->ub_static_buffer_size,  \
    (p_quick_ublock)->ub_p_static_buffer    = (ubuf)                                    )

/* set up a quick_ublock from an existing const buffer, not cleared for CHECKING_QUICK_UBLOCK */
#define quick_ublock_setup_fill_from_const_ubuf(p_quick_ublock, ubuf, ubuf_size) (      \
    (p_quick_ublock)->ub_h_array_buffer     = 0,                                        \
    (p_quick_ublock)->ub_static_buffer_size = (ubuf_size),                              \
    (p_quick_ublock)->ub_static_buffer_used = (p_quick_ublock)->ub_static_buffer_size,  \
    (p_quick_ublock)->ub_p_static_buffer    = de_const_cast(P_UCHARS, (ubuf))           )

/*
exported routines
*/

_Check_return_
_Ret_writes_maybenull_(extend_by) /* may be NULL */
extern P_UCHARS
quick_ublock_extend_by(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*extended*/,
    _InVal_     U32 extend_by,
    _OutRef_    P_STATUS p_status);

extern void
quick_ublock_shrink_by(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*shrunk*/,
    _InVal_     S32 shrink_by); /* NB -ve */

extern void
quick_ublock_dispose_leaving_buffer_valid(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*disposed*/);

extern void
quick_ublock_empty(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*emptied*/);

_Check_return_
extern STATUS
quick_ublock_uchars_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n);

_Check_return_
extern STATUS
quick_ublock_uchars_add_slow(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n);

_Check_return_
extern STATUS
quick_ublock_ustr_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_USTR ustr);

_Check_return_
extern STATUS
quick_ublock_ustr_add_n(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_USTR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/);

_Check_return_
extern STATUS
quick_ublock_nullch_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/);

extern void
quick_ublock_nullch_strip(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*stripped*/);

_Check_return_
extern STATUS __cdecl
quick_ublock_printf(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_ _Printf_format_string_ PC_USTR ustr_format,
    /**/        ...);

_Check_return_
extern STATUS
quick_ublock_vprintf(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_ _Printf_format_string_ PC_USTR ustr_format,
    /**/        va_list args);

static inline void
quick_ublock_dispose(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*IN*/ /*OUT=0*/)
{
    if(0 != quick_ublock_array_handle_ref(p_quick_ublock))
    {
#if CHECKING_QUICK_UBLOCK
        /* trash handle on dispose when CHECKING_QUICK_UBLOCK */
        const U32 uchars_n = array_elements32(&quick_ublock_array_handle_ref(p_quick_ublock));
        _do_uqb_fill(uchars_bptr(array_range(&quick_ublock_array_handle_ref(p_quick_ublock), UCHARB, 0, uchars_n)), uchars_n);
#endif
        al_array_dispose(&quick_ublock_array_handle_ref(p_quick_ublock));
    }

    /* trash buffer on dispose when CHECKING_QUICK_UBLOCK */
    _do_uqb_fill(p_quick_ublock->ub_p_static_buffer, p_quick_ublock->ub_static_buffer_size);
    p_quick_ublock->ub_static_buffer_used = 0;
}

_Check_return_
static inline U32
quick_ublock_bytes(
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock)
{
    if(0 != quick_ublock_array_handle_ref(p_quick_ublock))
        return(array_elements32(&quick_ublock_array_handle_ref(p_quick_ublock)));

    return(p_quick_ublock->ub_static_buffer_used);
}

_Check_return_
_Ret_notnull_
static inline PC_UCHARS
quick_ublock_uchars(
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock)
{
    if(0 != quick_ublock_array_handle_ref(p_quick_ublock))
    {
        assert(array_handle_is_valid(&quick_ublock_array_handle_ref(p_quick_ublock)));
        return(array_basec_no_checks(&quick_ublock_array_handle_ref(p_quick_ublock), _UCHARS));
    }

    return(p_quick_ublock->ub_p_static_buffer);
}

_Check_return_
_Ret_notnull_
static inline P_UCHARS
quick_ublock_uchars_wr(
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock)
{
    if(0 != quick_ublock_array_handle_ref(p_quick_ublock))
    {
        assert(array_handle_is_valid(&quick_ublock_array_handle_ref(p_quick_ublock)));
        return(array_base_no_checks(&quick_ublock_array_handle_ref(p_quick_ublock), _UCHARS));
    }

    return(p_quick_ublock->ub_p_static_buffer);
}

_Check_return_
static inline BOOL
quick_ublock_uchars_add_fast(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n)
{
    const U32 static_buffer_used = p_quick_ublock->ub_static_buffer_used;
    const BOOL fast_possible = (1 == uchars_n) && (p_quick_ublock->ub_static_buffer_size != static_buffer_used);

    if(fast_possible)
    {
        p_quick_ublock->ub_static_buffer_used = static_buffer_used + 1;
        PtrPutByteOff(p_quick_ublock->ub_p_static_buffer, static_buffer_used, PtrGetByte(uchars));
    }

    return(fast_possible);
}

_Check_return_
static __forceinline BOOL
_quick_ublock_uchar1_add_fast(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     U8 u8)
{
    const U32 static_buffer_used = p_quick_ublock->ub_static_buffer_used;
    const BOOL fast_possible = (p_quick_ublock->ub_static_buffer_size != static_buffer_used);

    if(fast_possible)
    {
        p_quick_ublock->ub_static_buffer_used = static_buffer_used + 1;
        PtrPutByteOff(p_quick_ublock->ub_p_static_buffer, static_buffer_used, u8);
    }

    return(fast_possible);
}

_Check_return_
_Ret_z_
static inline PC_USTR
quick_ublock_ustr(
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock)
{
    if(0 != quick_ublock_array_handle_ref(p_quick_ublock))
    {
        assert(array_handle_is_valid(&quick_ublock_array_handle_ref(p_quick_ublock)));
        return(array_basec_no_checks(&quick_ublock_array_handle_ref(p_quick_ublock), _UCHARZ));
    }

    return(p_quick_ublock->ub_p_static_buffer);
}

/* appending with conversion */

_Check_return_
extern STATUS
quick_ublock_a7char_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     A7CHAR a7char);

_Check_return_
extern STATUS
quick_ublock_ucs4_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     UCS4 ucs4);

_Check_return_
extern STATUS
quick_ublock_sbchars_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(sbchars_n) PC_SBCHARS sbchars,
    _InVal_     U32 sbchars_n);

_Check_return_
extern STATUS
quick_ublock_sbstr_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_SBSTR sbstr);

_Check_return_
extern STATUS
quick_ublock_sbstr_add_n(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_SBSTR sbstr,
    _InVal_     U32 sbchars_n /*strlen_with,without_NULLCH*/);

_Check_return_
extern STATUS
quick_ublock_sbchars_add_with_codepage(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(sbchars_n) PC_SBCHARS sbchars_in,
    _InVal_     U32 sbchars_n,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage);

_Check_return_
extern STATUS
quick_ublock_tchars_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(tchars_n) PCTCH tchars,
    _InVal_     U32 tchars_n);

_Check_return_
extern STATUS
quick_ublock_tstr_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PCTSTR tstr);

_Check_return_
extern STATUS
quick_ublock_tstr_add_n(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PCTSTR tstr,
    _InVal_     U32 tchars_n /*strlen_with,without_NULLCH*/);

_Check_return_
extern STATUS
quick_ublock_wchars_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(wchars_n) PCWCH pwch,
    _InVal_     U32 wchars_n);

#if USTR_IS_SBSTR

_Check_return_
extern STATUS
quick_ublock_ucs4_add_aiu(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     UCS4 ucs4);

#else /* NOT USTR_IS_SBSTR */

#define quick_ublock_ucs4_add_aiu(p_quick_ublock, ucs4) \
    quick_ublock_ucs4_add(p_quick_ublock, ucs4)

#endif /* USTR_IS_SBSTR */

#endif /* __quickublk_h */

/* end of quickublk.h */
