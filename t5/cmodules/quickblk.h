/* quickblk.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header file for quickblk.c (BYTE-based quick block - can contain any byte-based info) */

/* SKS November 2006 split out of aligator.h; May 2014 split off UCHARS/TCHARS */

#ifndef __quickblk_h
#define __quickblk_h

#if CHECKING && 1
#define CHECKING_QUICK_BLOCK 1
#else
#define CHECKING_QUICK_BLOCK 0
#endif

typedef struct QUICK_BLOCK
{
    ARRAY_HANDLE h_array_buffer;
    U32 static_buffer_size; /* NB in bytes */
    U32 static_buffer_used; /* NB in bytes */
    P_BYTE p_static_buffer; /* SKS 20141210 reordered for easier MSVC debugging */
}
QUICK_BLOCK, * P_QUICK_BLOCK; typedef const QUICK_BLOCK * PC_QUICK_BLOCK;

#define P_QUICK_BLOCK_NONE _P_DATA_NONE(P_QUICK_BLOCK)

#define QUICK_BLOCK_WITH_BUFFER(name, size) \
    QUICK_BLOCK name; BYTE name ## _buffer[size]

/* NB buffer ^^^ immediately follows QB in a structure for subsequent fixup */

#define QUICK_BLOCK_INIT_NULL() \
    { 0, NULL, 0, 0 }

#define quick_block_array_handle_ref(p_quick_block) \
    (p_quick_block)->h_array_buffer /* really only internal use but needed for QB sort, no brackets */

/*
functions as macros
*/

#if CHECKING_QUICK_BLOCK || CHECKING_FOR_CODE_ANALYSIS

static __forceinline void
_do_aqb_fill(
    _Out_writes_all_(elemof_buffer) P_BYTE buf,
    _InVal_     U32 elemof_buffer)
{
    (void) memset(buf, 0xEE, elemof_buffer);
}

#else
#define _do_aqb_fill(buf, elemof_buffer) /*nothing*/
#endif /* CHECKING_QUICK_BLOCK */

static inline void
_quick_block_setup(
    _OutRef_    P_QUICK_BLOCK p_quick_block /*set up*/,
    _Out_writes_all_(elemof_buffer) P_BYTE buf,
    _InVal_     U32 elemof_buffer)
{
    p_quick_block->h_array_buffer     = 0;
    p_quick_block->static_buffer_size = elemof_buffer;
    p_quick_block->static_buffer_used = 0;
    p_quick_block->p_static_buffer    = buf;

    _do_aqb_fill(p_quick_block->p_static_buffer, p_quick_block->static_buffer_size);
}

#define quick_block_with_buffer_setup(name /*ref*/) \
    _quick_block_setup(&name, name ## _buffer, sizeof32(name ## _buffer))

#define quick_block_setup(p_quick_block, buf /*ref*/) \
    _quick_block_setup(p_quick_block, buf, sizeof32(buf))

/* set up a quick_block from an existing buffer (no _do_aqb_fill - see xvsnprintf), not cleared for CHECKING_QUICK_BLOCK */
#define quick_block_setup_without_clearing_buf(p_quick_block, buf, buf_size) (      \
    (p_quick_block)->h_array_buffer     = 0,                                        \
    (p_quick_block)->static_buffer_size = (buf_size),                               \
    (p_quick_block)->static_buffer_used = 0,                                        \
    (p_quick_block)->p_static_buffer    = (buf)                                     )

/* set up a quick buf from an existing array handle, not cleared for CHECKING_QUICK_BLOCK */
#define quick_block_setup_using_array(p_quick_block, handle) (                      \
    (p_quick_block)->h_array_buffer     = (handle),                                 \
    (p_quick_block)->static_buffer_size = 0,                                        \
    (p_quick_block)->static_buffer_used = 0,                                        \
    (p_quick_block)->p_static_buffer    = NULL                                      )

/* set up a quick_block from an existing buffer, not cleared for CHECKING_QUICK_BLOCK */
#define quick_block_setup_fill_from_buf(p_quick_block, buf, buf_size) (             \
    (p_quick_block)->h_array_buffer     = 0,                                        \
    (p_quick_block)->static_buffer_size = (buf_size),                               \
    (p_quick_block)->static_buffer_used = (p_quick_block)->static_buffer_size,      \
    (p_quick_block)->p_static_buffer    = (buf)                                     )

/* set up a quick_block from an existing const buffer, not cleared for CHECKING_QUICK_BLOCK */
#define quick_block_setup_fill_from_const_buf(p_quick_block, buf, buf_size) (       \
    (p_quick_block)->h_array_buffer     = 0,                                        \
    (p_quick_block)->static_buffer_size = (buf_size),                               \
    (p_quick_block)->static_buffer_used = (p_quick_block)->static_buffer_size,      \
    (p_quick_block)->p_static_buffer    = de_const_cast(P_BYTE, (buf))              )

/*
exported routines
*/

_Check_return_
_Ret_writes_maybenull_(extend_by) /* may be NULL */
extern P_BYTE
quick_block_extend_by(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*extended*/,
    _InVal_     U32 extend_by,
    _OutRef_    P_STATUS p_status);

extern void
quick_block_shrink_by(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*shrunk*/,
    _InVal_     S32 shrink_by); /* NB -ve */

extern void
quick_block_dispose_leaving_buffer_valid(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*disposed*/);

extern void
quick_block_empty(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*emptied*/);

_Check_return_
extern STATUS
quick_block_byte_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _InVal_     BYTE u8);

_Check_return_
extern STATUS
quick_block_bytes_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _In_reads_bytes_(n_bytes) PC_ANY p_any,
    _InVal_     U32 n_bytes);

_Check_return_
extern STATUS
quick_block_nullch_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/);

extern void
quick_block_nullch_strip(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*stripped*/);

_Check_return_
extern STATUS __cdecl
quick_block_printf(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _In_z_ _Printf_format_string_ PC_U8Z format,
    /**/        ...);

_Check_return_
extern STATUS
quick_block_vprintf(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _In_z_ _Printf_format_string_ PC_U8Z format,
    /**/        va_list args);

static inline void
quick_block_dispose(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*disposed*/)
{
    if(0 != quick_block_array_handle_ref(p_quick_block))
    {
#if CHECKING_QUICK_BLOCK
        /* trash handle on dispose when CHECKING_QUICK_BLOCK */
        const U32 n_bytes = array_elements32(&quick_block_array_handle_ref(p_quick_block));
        _do_aqb_fill(array_range(&quick_block_array_handle_ref(p_quick_block), BYTE, 0, n_bytes), n_bytes);
#endif
        al_array_dispose(&quick_block_array_handle_ref(p_quick_block));
    }

    /* trash buffer on dispose when CHECKING_QUICK_BLOCK */
    _do_aqb_fill(p_quick_block->p_static_buffer, p_quick_block->static_buffer_size);
    p_quick_block->static_buffer_used = 0;
}

_Check_return_
static inline BOOL
quick_block_byte_add_fast(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _InVal_     BYTE u8)
{
    const U32 static_buffer_used = p_quick_block->static_buffer_used;
    const BOOL fast_possible = (p_quick_block->static_buffer_size != static_buffer_used);

    if(fast_possible)
    {
        p_quick_block->static_buffer_used = static_buffer_used + 1;
        PtrPutByteOff(p_quick_block->p_static_buffer, static_buffer_used, u8);
    }

    return(fast_possible);
}

_Check_return_
static inline U32
quick_block_bytes(
    _InRef_     PC_QUICK_BLOCK p_quick_block)
{
    if(0 != quick_block_array_handle_ref(p_quick_block))
        return(array_elements32(&quick_block_array_handle_ref(p_quick_block)));

    return(p_quick_block->static_buffer_used);
}

_Check_return_
_Ret_notnull_
static inline PC_BYTE
quick_block_ptr(
    _InRef_     PC_QUICK_BLOCK p_quick_block)
{
    if(0 != quick_block_array_handle_ref(p_quick_block))
    {
        assert(array_handle_valid(&quick_block_array_handle_ref(p_quick_block)));
        return(array_basec_no_checks(&quick_block_array_handle_ref(p_quick_block), BYTE));
    }

    return(p_quick_block->p_static_buffer);
}

_Check_return_
_Ret_notnull_
static inline P_BYTE
quick_block_ptr_wr(
    _InRef_     P_QUICK_BLOCK p_quick_block)
{
    if(0 != quick_block_array_handle_ref(p_quick_block))
    {
        assert(array_handle_valid(&quick_block_array_handle_ref(p_quick_block)));
        return(array_base_no_checks(&quick_block_array_handle_ref(p_quick_block), BYTE));
    }

    return(p_quick_block->p_static_buffer);
}

_Check_return_
_Ret_z_
static inline PC_U8Z
quick_block_str(
    _InRef_     PC_QUICK_BLOCK p_quick_block)
{
    return((PC_U8Z) quick_block_ptr(p_quick_block));
}

#endif /* __quickblk_h */

/* end of quickblk.h */
