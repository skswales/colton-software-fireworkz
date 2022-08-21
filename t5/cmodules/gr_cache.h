/* gr_cache.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Draw file cache */

/* SKS May 1991 */

#ifndef __gr_cache_h
#define __gr_cache_h

/*
exports from gr_cache.c
*/

typedef struct DRAW_DIAG
{
    P_BYTE data;
    U32 length;
}
DRAW_DIAG, * P_DRAW_DIAG; typedef const DRAW_DIAG * PC_DRAW_DIAG;

/*
32-bit cache handle for export (was abstract)
*/

typedef enum GR_CACHE_HANDLE
{
    GR_CACHE_HANDLE_NONE = 0
}
GR_CACHE_HANDLE, * P_GR_CACHE_HANDLE;

/*
gr_cache.c
*/

_Check_return_
extern STATUS
gr_cache_can_import(
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern STATUS
gr_cache_embedded(
    _OutRef_    P_GR_CACHE_HANDLE cahp,
    _InoutRef_  P_ARRAY_HANDLE p_h_data /*inout;maybe stolen, certainly poked*/);

/*ncr*/
extern STATUS
gr_cache_embedded_updating_entry(
    _InoutRef_  P_GR_CACHE_HANDLE cahp);

_Check_return_
extern STATUS
gr_cache_ensure_PicConvert(void);

_Check_return_
extern STATUS
gr_cache_entry_ensure(
    _OutRef_    P_GR_CACHE_HANDLE cahp,
    _In_z_      PCTSTR name,
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern STATUS
gr_cache_entry_query(
    _OutRef_    P_GR_CACHE_HANDLE cahp,
    _In_opt_z_  PCTSTR name);

extern void
gr_cache_entry_remove(
    _InVal_     GR_CACHE_HANDLE cah);

_Check_return_
extern STATUS
gr_cache_entry_rename(
    _InVal_     GR_CACHE_HANDLE cah,
    _In_opt_z_  PCTSTR name);

_Check_return_
extern STATUS
gr_cache_entry_set_autokill(
    _InVal_     GR_CACHE_HANDLE cah);

_Check_return_
extern STATUS
gr_cache_error_query(
    _InVal_     GR_CACHE_HANDLE cah);

_Check_return_
extern STATUS
gr_cache_error_set(
    _InVal_     GR_CACHE_HANDLE cah,
    _InVal_     STATUS err);

extern ARRAY_HANDLE
gr_cache_loaded_ensure(
    _InVal_     GR_CACHE_HANDLE cah);

_Check_return_
extern BOOL
gr_cache_name_query(
    _InVal_     GR_CACHE_HANDLE cah,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer);

extern void
gr_cache_ref(
    _InVal_     GR_CACHE_HANDLE cah,
    _InVal_     BOOL add_ref);

extern void
gr_cache_reref(
    _InoutRef_  P_GR_CACHE_HANDLE cahp,
    _InVal_     GR_CACHE_HANDLE new_cah);

extern ARRAY_HANDLE
gr_cache_search(
    _InVal_     GR_CACHE_HANDLE cah);

#if WINDOWS

_Check_return_
_Ret_maybenull_
extern P_ANY /*CPicture*/
gr_cache_search_cpicture(
    _InVal_     GR_CACHE_HANDLE cah,
    _OutRef_    P_ARRAY_HANDLE p_array_handle);

#endif /* OS */

extern void
gr_cache_trash(void);

/*
end of exports from gr_cache.c
*/

/* offset in our internal object/group/diagram (can be large) */
typedef U32 GR_DIAG_OFFSET;  typedef GR_DIAG_OFFSET * P_GR_DIAG_OFFSET; typedef const GR_DIAG_OFFSET * PC_GR_DIAG_OFFSET;

typedef struct GR_RISCDIAG
{
    DRAW_DIAG draw_diag;

    U32 dd_allocsize;

    DRAW_DIAG_OFFSET dd_fontListR;
    DRAW_DIAG_OFFSET dd_fontListW;
    DRAW_DIAG_OFFSET dd_options;
    DRAW_DIAG_OFFSET dd_rootGroupStart;
}
GR_RISCDIAG, * P_GR_RISCDIAG, ** P_P_GR_RISCDIAG; typedef const GR_RISCDIAG * PC_GR_RISCDIAG;

#endif /* __gr_cache_h */

/* end of gr_cache.h */
