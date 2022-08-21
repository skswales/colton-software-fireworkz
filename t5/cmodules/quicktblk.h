/* quicktblk.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header file for quicktblk.c (TCHAR-based quick block) */

/* SKS May 2014 split off from quickblk.h */

#ifndef __quicktblk_h
#define __quicktblk_h

#if CHECKING && 1
#define CHECKING_QUICK_TBLOCK 1
#else
#define CHECKING_QUICK_TBLOCK 0
#endif

typedef struct QUICK_TBLOCK
{
    ARRAY_HANDLE tb_h_array_buffer;
    U32 tb_static_buffer_elem; /* NB in TCHARs */
    U32 tb_static_buffer_used; /* NB in TCHARs */
    PTCH tb_p_static_buffer; /* SKS 20141210 reordered for easier MSVC debugging */
}
QUICK_TBLOCK, * P_QUICK_TBLOCK; typedef const QUICK_TBLOCK * PC_QUICK_TBLOCK;

#define P_QUICK_TBLOCK_NONE _P_DATA_NONE(P_QUICK_TBLOCK)

#define QUICK_TBLOCK_WITH_BUFFER(name, elem) \
    QUICK_TBLOCK name; TCHAR name ## _buffer[elem]

/* NB buffer ^^^ immediately follows QB in a structure for subsequent fixup */

#define QUICK_TBLOCK_INIT_NULL() \
    { 0, 0, 0, NULL }

#define quick_tblock_array_handle_ref(p_quick_tblock) \
    (p_quick_tblock)->tb_h_array_buffer /* really only internal use but needed for QB sort, no brackets */

/*
functions as macros
*/

#if CHECKING_QUICK_TBLOCK || CHECKING_FOR_CODE_ANALYSIS

static __forceinline void
_do_tqb_fill(
    _Out_writes_all_(tbuf_elem) PTCH tbuf,
    _InVal_     U32 tbuf_elem)
{
    (void) memset(tbuf, 0xEE, tbuf_elem * sizeof(TCHAR));
}

#else
#define _do_tqb_fill(tbuf, elemof_tbuf) /*nothing*/
#endif /* CHECKING_QUICK_TBLOCK */

static inline void
_quick_tblock_setup(
    _OutRef_    P_QUICK_TBLOCK p_quick_tblock /*set up*/,
    _Out_writes_all_(tbuf_elem) PTCH tbuf,
    _InVal_     U32 tbuf_elem)
{
    p_quick_tblock->tb_h_array_buffer     = 0;
    p_quick_tblock->tb_static_buffer_elem = tbuf_elem;
    p_quick_tblock->tb_static_buffer_used = 0;
    p_quick_tblock->tb_p_static_buffer    = tbuf;

    _do_tqb_fill(p_quick_tblock->tb_p_static_buffer, p_quick_tblock->tb_static_buffer_elem);
}

#define quick_tblock_with_buffer_setup(name /*ref*/) \
    _quick_tblock_setup(&name, name ## _buffer, elemof32(name ## _buffer))

#define quick_tblock_setup(p_quick_tblock, tbuf /*ref*/) \
    _quick_tblock_setup(p_quick_tblock, tbuf, elemof32(tbuf))

/* set up a quick_tblock from an existing buffer (no _do_tqb_fill - see tstr_xvsnprintf), not cleared for CHECKING_QUICK_TBLOCK */
#define quick_tblock_setup_without_clearing_tbuf(p_quick_tblock, tbuf, tbuf_elem) (     \
    (p_quick_tblock)->tb_h_array_buffer     = 0,                                        \
    (p_quick_tblock)->tb_static_buffer_elem = (tbuf_elem),                              \
    (p_quick_tblock)->tb_static_buffer_used = 0,                                        \
    (p_quick_tblock)->tb_p_static_buffer    = (tbuf)                                    )

/* set up a quick_tblock from an existing array handle, not cleared for CHECKING_QUICK_TBLOCK */
#define quick_tblock_setup_using_array(p_quick_tblock, handle) (                        \
    (p_quick_tblock)->tb_h_array_buffer     = (handle),                                 \
    (p_quick_tblock)->tb_static_buffer_elem = 0,                                        \
    (p_quick_tblock)->tb_static_buffer_used = 0,                                        \
    (p_quick_tblock)->tb_p_static_buffer    = NULL                                      )

/* set up a quick_tblock from an existing buffer, not cleared for CHECKING_QUICK_TBLOCK */
#define quick_tblock_setup_fill_from_tbuf(p_quick_tblock, tbuf, tbuf_elem) (            \
    (p_quick_tblock)->tb_h_array_buffer     = 0,                                        \
    (p_quick_tblock)->tb_static_buffer_elem = (tbuf_elem),                              \
    (p_quick_tblock)->tb_static_buffer_used = (p_quick_tblock)->tb_static_buffer_elem,  \
    (p_quick_tblock)->tb_p_static_buffer    = (tbuf)                                    )

/* set up a quick_tblock from an existing const buffer, not cleared for CHECKING_QUICK_TBLOCK */
#define quick_tblock_setup_fill_from_const_tbuf(p_quick_tblock, tbuf, tbuf_elem) (      \
    (p_quick_tblock)->tb_h_array_buffer     = 0,                                        \
    (p_quick_tblock)->tb_static_buffer_elem = (tbuf_elem),                              \
    (p_quick_tblock)->tb_static_buffer_used = (p_quick_tblock)->tb_static_buffer_elem,  \
    (p_quick_tblock)->tb_p_static_buffer    = de_const_cast(PTCH, (tbuf))               )

/*
exported routines
*/

_Check_return_
_Ret_writes_maybenull_(extend_by) /* may be NULL */
extern PTCH
quick_tblock_extend_by(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*extended*/,
    _InVal_     U32 extend_by,
    _OutRef_    P_STATUS p_status);

extern void
quick_tblock_shrink_by(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*shrunk*/,
    _InVal_     S32 extend_by); /* NB -ve */

extern void
quick_tblock_dispose_leaving_buffer_valid(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*disposed*/);

extern void
quick_tblock_empty(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*emptied*/);

_Check_return_
extern STATUS
quick_tblock_tchar_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InVal_     TCHAR tchar);

_Check_return_
extern STATUS
quick_tblock_tchars_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_reads_(tchars_n) PCTCH ptch,
    _InVal_     U32 tchars_n);

_Check_return_
extern STATUS
quick_tblock_tstr_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_      PCTSTR tstr);

_Check_return_
extern STATUS
quick_tblock_tstr_add_n(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_      PCTSTR tstr,
    _InVal_     U32 tchars_n /*strlen_with,without_NULLCH*/);

_Check_return_
extern STATUS
quick_tblock_nullch_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/);

extern void
quick_tblock_nullch_strip(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*stripped*/);

_Check_return_
extern STATUS __cdecl
quick_tblock_printf(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...);

_Check_return_
extern STATUS
quick_tblock_vprintf(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list args);

static inline void
quick_tblock_dispose(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*disposed*/)
{
    if(0 != quick_tblock_array_handle_ref(p_quick_tblock))
    {
#if CHECKING_QUICK_TBLOCK
        /* trash handle on dispose when CHECKING_QUICK_TBLOCK */
        const U32 tchars_n = array_elements32(&quick_tblock_array_handle_ref(p_quick_tblock));
        _do_tqb_fill(array_range(&quick_tblock_array_handle_ref(p_quick_tblock), TCHAR, 0, tchars_n), tchars_n);
#endif
        al_array_dispose(&quick_tblock_array_handle_ref(p_quick_tblock));
    }

    /* trash buffer on dispose when CHECKING_QUICK_TBLOCK */
    _do_tqb_fill(p_quick_tblock->tb_p_static_buffer, p_quick_tblock->tb_static_buffer_elem);
    p_quick_tblock->tb_static_buffer_used = 0;
}

_Check_return_
static inline U32
quick_tblock_chars(
    _InRef_     PC_QUICK_TBLOCK p_quick_tblock)
{
    if(0 != quick_tblock_array_handle_ref(p_quick_tblock))
        return(array_elements32(&quick_tblock_array_handle_ref(p_quick_tblock)));

    return(p_quick_tblock->tb_static_buffer_used);
}

_Check_return_
_Ret_notnull_
static inline PCTCH
quick_tblock_tchars(
    _InRef_     PC_QUICK_TBLOCK p_quick_tblock)
{
    if(0 != quick_tblock_array_handle_ref(p_quick_tblock))
    {
        assert(array_handle_is_valid(&quick_tblock_array_handle_ref(p_quick_tblock)));
        return(array_basec_no_checks(&quick_tblock_array_handle_ref(p_quick_tblock), TCHAR));
    }

    return(p_quick_tblock->tb_p_static_buffer);
}

_Check_return_
_Ret_notnull_
static inline PTCH
quick_tblock_tchars_wr(
    _InRef_     P_QUICK_TBLOCK p_quick_tblock)
{
    if(0 != quick_tblock_array_handle_ref(p_quick_tblock))
    {
        assert(array_handle_is_valid(&quick_tblock_array_handle_ref(p_quick_tblock)));
        return(array_base_no_checks(&quick_tblock_array_handle_ref(p_quick_tblock), TCHAR));
    }

    return(p_quick_tblock->tb_p_static_buffer);
}

_Check_return_
_Ret_z_
static inline PCTSTR
quick_tblock_tstr(
    _InRef_     PC_QUICK_TBLOCK p_quick_tblock)
{
    if(0 != quick_tblock_array_handle_ref(p_quick_tblock))
    {
        assert(array_handle_is_valid(&quick_tblock_array_handle_ref(p_quick_tblock)));
        return(array_basec_no_checks(&quick_tblock_array_handle_ref(p_quick_tblock), TCHAR));
    }

    return(p_quick_tblock->tb_p_static_buffer);
}

/* appending with conversion */

_Check_return_
extern STATUS
quick_tblock_ucs4_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InVal_     UCS4 ucs4);

_Check_return_
extern STATUS
quick_tblock_uchars_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n);

_Check_return_
extern STATUS
quick_tblock_ustr_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_      PC_USTR ustr);

_Check_return_
extern STATUS
quick_tblock_ustr_add_n(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_      PC_USTR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/);

_Check_return_
extern STATUS
quick_tblock_utf8_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n);

_Check_return_
extern STATUS
quick_tblock_utf8str_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_      PC_UTF8STR ustr);

_Check_return_
extern STATUS
quick_tblock_utf8str_add_n(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/);

#endif /* __quicktblk_h */

/* end of quicktblk.h */
