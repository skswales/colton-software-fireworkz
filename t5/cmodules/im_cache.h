/* im_cache.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Image file cache */

/* SKS May 1991 */

#ifndef __im_cache_h
#define __im_cache_h

/*
exports from im_cache.c
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

typedef enum IMAGE_CACHE_HANDLE
{
    IMAGE_CACHE_HANDLE_NONE = 0
}
IMAGE_CACHE_HANDLE, * P_IMAGE_CACHE_HANDLE;

/*
im_cache.c
*/

_Check_return_
extern BOOL
image_cache_can_load(
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern BOOL
image_cache_can_import_with_image_convert(
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern STATUS
image_cache_embedded(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _InoutRef_  P_ARRAY_HANDLE p_h_data /*inout;maybe stolen, certainly poked*/,
    _InVal_     T5_FILETYPE t5_filetype);

/*ncr*/
extern STATUS
image_cache_embedded_updating_entry(
    _InoutRef_  P_IMAGE_CACHE_HANDLE p_image_cache_handle);

_Check_return_
extern STATUS
image_cache_entry_create(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _In_z_      PCTSTR name,
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern STATUS
image_cache_entry_ensure(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _In_z_      PCTSTR name,
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern BOOL
image_cache_entry_query(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _In_z_      PCTSTR name);

extern void
image_cache_entry_remove(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle);

_Check_return_
extern STATUS
image_cache_entry_rename(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle,
    _In_opt_z_  PCTSTR name);

_Check_return_
extern STATUS
image_cache_entry_set_autokill(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle);

_Check_return_
extern STATUS
image_cache_error_query(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle);

_Check_return_
extern STATUS
image_cache_error_set(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle,
    _InVal_     STATUS err);

extern ARRAY_HANDLE
image_cache_loaded_ensure(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle);

_Check_return_
extern BOOL
image_cache_name_query(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer);

extern void
image_cache_ref(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle,
    _InVal_     BOOL add_ref);

extern void
image_cache_reref(
    _InoutRef_  P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _InVal_     IMAGE_CACHE_HANDLE new_cah);

_Check_return_
extern ARRAY_HANDLE
image_cache_search(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle,
    _InVal_     BOOL fOriginal);

typedef struct GdipImage_struct * GdipImage; /* opaque */

#if WINDOWS

_Check_return_
_Ret_maybenull_
extern GdipImage
image_cache_search_gdip_image(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle,
    _OutRef_    P_ARRAY_HANDLE p_array_handle);

#endif /* OS */

extern void
image_cache_trash(void);

/*
end of exports from im_cache.c
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

/*
im_convert.c
*/

_Check_return_
extern BOOL
image_convert_can_convert(
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern STATUS
image_convert_do_convert_file(
    _OutRef_    P_PTSTR p_converted_name,
    _OutRef_    P_T5_FILETYPE p_t5_filetype_converted,
    _In_z_      PCTSTR source_file_name,
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern STATUS
image_convert_do_convert_data(
    _OutRef_    P_PTSTR p_converted_name,
    _OutRef_    P_T5_FILETYPE p_t5_filetype_converted,
    _In_reads_(n_bytes) PC_BYTE p_data,
    _InVal_     U32 n_bytes,
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern STATUS
image_convert_ensure_PicConvert(void);

#endif /* __im_cache_h */

/* end of im_cache.h */
