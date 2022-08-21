/* quickwblk.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header file for quickwblk.c (WCHAR-based quick block) */

/* SKS Jul 2014 derived from quicktblk.h */

#ifndef __quickwblk_h
#define __quickwblk_h

#if CHECKING && 1
#define CHECKING_QUICK_WBLOCK 1
#else
#define CHECKING_QUICK_WBLOCK 0
#endif

typedef struct QUICK_WBLOCK
{
    ARRAY_HANDLE wb_h_array_buffer;
    U32 wb_static_buffer_elem; /* NB in WCHARs */
    U32 wb_static_buffer_used; /* NB in WCHARs */
    PWCH wb_p_static_buffer; /* SKS 20141210 reordered for easier MSVC debugging */
}
QUICK_WBLOCK, * P_QUICK_WBLOCK; typedef const QUICK_WBLOCK * PC_QUICK_WBLOCK;

#define P_QUICK_WBLOCK_NONE _P_DATA_NONE(P_QUICK_WBLOCK)

#define QUICK_WBLOCK_WITH_BUFFER(name, elem) \
    QUICK_WBLOCK name; WCHAR name ## _buffer[elem]

/* NB buffer ^^^ immediately follows QB in a structure for subsequent fixup */

#define QUICK_WBLOCK_INIT_NULL() \
    { 0,, 0, 0 NULL }

#define quick_wblock_array_handle_ref(p_quick_wblock) \
    (p_quick_wblock)->wb_h_array_buffer /* really only internal use but needed for QB sort, no brackets */

/*
functions as macros
*/

#if CHECKING_QUICK_WBLOCK || CHECKING_FOR_CODE_ANALYSIS

static __forceinline void
_do_wqb_fill(
    _Out_writes_all_(wbuf_elem) PWCH wbuf,
    _InVal_     U32 wbuf_elem)
{
    (void) memset(wbuf, 0xEE, wbuf_elem * sizeof(WCHAR));
}

#else
#define _do_wqb_fill(wbuf, elemof_wbuf) /*nothing*/
#endif /* CHECKING_QUICK_WBLOCK */

static inline void
_quick_wblock_setup(
    _OutRef_    P_QUICK_WBLOCK p_quick_wblock /*set up*/,
    _Out_writes_all_(wbuf_elem) PWCH wbuf,
    _InVal_     U32 wbuf_elem)
{
    p_quick_wblock->wb_h_array_buffer     = 0;
    p_quick_wblock->wb_static_buffer_elem = wbuf_elem;
    p_quick_wblock->wb_static_buffer_used = 0;
    p_quick_wblock->wb_p_static_buffer    = wbuf;

    _do_wqb_fill(p_quick_wblock->wb_p_static_buffer, p_quick_wblock->wb_static_buffer_elem);
}

#define quick_wblock_with_buffer_setup(name /*ref*/) \
    _quick_wblock_setup(&name, name ## _buffer, elemof32(name ## _buffer))

#define quick_wblock_setup(p_quick_wblock, wbuf /*ref*/) \
    _quick_wblock_setup(p_quick_wblock, wbuf, elemof32(wbuf))

/* set up a quick_wblock from an existing buffer (no _do_wqb_fill - see wstr_xvsnprintf), not cleared for CHECKING_QUICK_WBLOCK */
#define quick_wblock_setup_without_clearing_wbuf(p_quick_wblock, wbuf, wbuf_elem) (     \
    (p_quick_wblock)->wb_h_array_buffer     = 0,                                        \
    (p_quick_wblock)->wb_static_buffer_elem = (wbuf_elem),                              \
    (p_quick_wblock)->wb_static_buffer_used = 0,                                        \
    (p_quick_wblock)->wb_p_static_buffer    = (wbuf)                                    )

/* set up a quick_wblock from an existing array handle, not cleared for CHECKING_QUICK_WBLOCK */
#define quick_wblock_setup_using_array(p_quick_wblock, handle) (                        \
    (p_quick_wblock)->wb_h_array_buffer     = (handle),                                 \
    (p_quick_wblock)->wb_static_buffer_elem = 0,                                        \
    (p_quick_wblock)->wb_static_buffer_used = 0,                                        \
    (p_quick_wblock)->wb_p_static_buffer    = NULL                                      )

/* set up a quick_wblock from an existing buffer, not cleared for CHECKING_QUICK_WBLOCK */
#define quick_wblock_setup_fill_from_wbuf(p_quick_wblock, wbuf, wbuf_elem) (            \
    (p_quick_wblock)->wb_h_array_buffer     = 0,                                        \
    (p_quick_wblock)->wb_static_buffer_elem = (wbuf_elem),                              \
    (p_quick_wblock)->wb_static_buffer_used = (p_quick_wblock)->wb_static_buffer_elem,  \
    (p_quick_wblock)->wb_p_static_buffer    = (wbuf)                                    )

/* set up a quick_wblock from an existing const buffer, not cleared for CHECKING_QUICK_WBLOCK */
#define quick_wblock_setup_fill_from_const_wbuf(p_quick_wblock, wbuf, wbuf_elem) (      \
    (p_quick_wblock)->wb_h_array_buffer     = 0,                                        \
    (p_quick_wblock)->wb_static_buffer_elem = (wbuf_elem),                              \
    (p_quick_wblock)->wb_static_buffer_used = (p_quick_wblock)->wb_static_buffer_elem,  \
    (p_quick_wblock)->wb_p_static_buffer    = de_const_cast(PWCH, (wbuf))               )

/*
exported routines
*/

_Check_return_
_Ret_writes_maybenull_(extend_by) /* may be NULL */
extern PWCH
quick_wblock_extend_by(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*extended*/,
    _InVal_     U32 extend_by,
    _OutRef_    P_STATUS p_status);

extern void
quick_wblock_shrink_by(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*shrunk*/,
    _InVal_     S32 extend_by); /* NB -ve */

extern void
quick_wblock_dispose_leaving_buffer_valid(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*disposed*/);

extern void
quick_wblock_empty(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*emptied*/);

_Check_return_
extern STATUS
quick_wblock_wchar_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _InVal_     WCHAR wchar);

_Check_return_
extern STATUS
quick_wblock_wchars_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_reads_(wchars_n) PCWCH pwch,
    _InVal_     U32 wchars_n);

_Check_return_
extern STATUS
quick_wblock_wstr_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PCWSTR wstr);

_Check_return_
extern STATUS
quick_wblock_wstr_add_n(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PCWSTR wstr,
    _InVal_     U32 wchars_n /*strlen_with,without_NULLCH*/);

_Check_return_
extern STATUS
quick_wblock_nullch_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/);

extern void
quick_wblock_nullch_strip(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*stripped*/);

_Check_return_
extern STATUS __cdecl
quick_wblock_printf(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_ _Printf_format_string_ PCWSTR format,
    /**/        ...);

_Check_return_
extern STATUS
quick_wblock_vprintf(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_ _Printf_format_string_ PCWSTR format,
    /**/        va_list args);

static inline void
quick_wblock_dispose(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*disposed*/)
{
    if(0 != quick_wblock_array_handle_ref(p_quick_wblock))
    {
#if CHECKING_QUICK_WBLOCK
        /* trash handle on dispose when CHECKING_QUICK_WBLOCK */
        const U32 wchars_n = array_elements32(&quick_wblock_array_handle_ref(p_quick_wblock));
        _do_wqb_fill(array_range(&quick_wblock_array_handle_ref(p_quick_wblock), WCHAR, 0, wchars_n), wchars_n);
#endif
        al_array_dispose(&quick_wblock_array_handle_ref(p_quick_wblock));
    }

    /* trash buffer on dispose when CHECKING_QUICK_WBLOCK */
    _do_wqb_fill(p_quick_wblock->wb_p_static_buffer, p_quick_wblock->wb_static_buffer_elem);
    p_quick_wblock->wb_static_buffer_used = 0;
}

_Check_return_
static inline U32
quick_wblock_chars(
    _InRef_     PC_QUICK_WBLOCK p_quick_wblock)
{
    if(0 != quick_wblock_array_handle_ref(p_quick_wblock))
        return(array_elements32(&quick_wblock_array_handle_ref(p_quick_wblock)));

    return(p_quick_wblock->wb_static_buffer_used);
}

_Check_return_
_Ret_notnull_
static inline PCWCH
quick_wblock_wchars(
    _InRef_     PC_QUICK_WBLOCK p_quick_wblock)
{
    if(0 != quick_wblock_array_handle_ref(p_quick_wblock))
    {
        assert(array_handle_is_valid(&quick_wblock_array_handle_ref(p_quick_wblock)));
        return(array_basec_no_checks(&quick_wblock_array_handle_ref(p_quick_wblock), WCHAR));
    }

    return(p_quick_wblock->wb_p_static_buffer);
}

_Check_return_
_Ret_notnull_
static inline PWCH
quick_wblock_wchars_wr(
    _InRef_     P_QUICK_WBLOCK p_quick_wblock)
{
    if(0 != quick_wblock_array_handle_ref(p_quick_wblock))
    {
        assert(array_handle_is_valid(&quick_wblock_array_handle_ref(p_quick_wblock)));
        return(array_base_no_checks(&quick_wblock_array_handle_ref(p_quick_wblock), WCHAR));
    }

    return(p_quick_wblock->wb_p_static_buffer);
}

_Check_return_
_Ret_z_
static inline PCWSTR
quick_wblock_wstr(
    _InRef_     PC_QUICK_WBLOCK p_quick_wblock)
{
    if(0 != quick_wblock_array_handle_ref(p_quick_wblock))
    {
        assert(array_handle_is_valid(&quick_wblock_array_handle_ref(p_quick_wblock)));
        return(array_basec_no_checks(&quick_wblock_array_handle_ref(p_quick_wblock), WCHAR));
    }

    return(p_quick_wblock->wb_p_static_buffer);
}

/* appending with conversion */

_Check_return_
extern STATUS
quick_wblock_ucs4_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _InVal_     UCS4 ucs4);

_Check_return_
extern STATUS
quick_wblock_sbchars_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_reads_(sbchars_n) PC_SBCHARS sbchars,
    _InVal_     U32 sbchars_n);

_Check_return_
extern STATUS
quick_wblock_sbstr_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PC_SBSTR sbstr);

_Check_return_
extern STATUS
quick_wblock_sbstr_add_n(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PC_SBSTR sbstr,
    _InVal_     U32 sbchars_n /*strlen_with,without_NULLCH*/);

_Check_return_
extern STATUS
quick_wblock_uchars_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n);

_Check_return_
extern STATUS
quick_wblock_ustr_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PC_USTR ustr);

_Check_return_
extern STATUS
quick_wblock_ustr_add_n(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PC_USTR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/);

_Check_return_
extern STATUS
quick_wblock_utf8_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n);

_Check_return_
extern STATUS
quick_wblock_utf8str_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PC_UTF8STR ustr);

_Check_return_
extern STATUS
quick_wblock_utf8str_add_n(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/);

#endif /* __quickwblk_h */

/* end of quickwblk.h */
