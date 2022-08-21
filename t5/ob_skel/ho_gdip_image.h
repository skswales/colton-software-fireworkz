/* ho_gdip_image.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2015-2019 Stuart Swales */

/* SKS Dec 2015 */

#ifndef __ho_gdip_image_h
#define __ho_gdip_image_h

#if defined(__cplusplus)
extern "C" {
#endif

//typedef struct GdipImage_struct * GdipImage; // opaque

_Check_return_ _Success_(status_ok(return))
extern STATUS
GdipImage_New(
    _Out_       GdipImage * const p_p_new);

extern void
GdipImage_Dispose(
    _Inout_     GdipImage * const p_p_this);

_Check_return_
extern BOOL
GdipImage_Load_File(
    _InoutRef_  GdipImage p_this,
    _In_z_      PCWSTR wstr_filename,
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern BOOL
GdipImage_Load_Memory(
    _InoutRef_  GdipImage p_this,
    _In_reads_(n_bytes) PC_BYTE p_data,
    _InVal_     U32 n_bytes,
    _InVal_     T5_FILETYPE t5_filetype);

extern void
GdipImage_GetImageSize(
    _InRef_     GdipImage p_this,
    _OutRef_    PSIZE p_size /* PIXITS */);

_Check_return_
extern BOOL
GdipImage_Render(
    _InRef_     GdipImage p_this,
    _HdcRef_    HDC hDC,
    _InRef_     PCRECT p_rect);

_Check_return_
extern BOOL
GdipImage_SaveAs_BMP(
    _InoutRef_  GdipImage p_this,
    _In_z_      PCWSTR wstr_filename);

#if defined(__cplusplus)
}
#endif

#endif /* ho_gdip_image_h */

/* end of ho_gdip_image.h */
