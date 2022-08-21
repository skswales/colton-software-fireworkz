/* ho_cpicture.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2006-2020 Stuart Swales */

/* SKS Feb 2006 */

#ifndef __ho_cpicture_h
#define __ho_cpicture_h

#if WINDOWS

typedef struct CPicture_struct * CPicture; // opaque

_Check_return_
_Ret_maybenull_
extern CPicture
CPicture_New(
    _OutRef_    P_STATUS p_status);

extern void
CPicture_Dispose(
    _Inout_     CPicture * const p_this);

_Check_return_
extern BOOL
CPicture_Load_File(
    _InoutRef_  CPicture p_this,
    _In_z_      PCTSTR lpszFileName);

_Check_return_
extern BOOL
CPicture_Load_HGlobal(
    _InoutRef_  CPicture p_this,
    _In_        HGLOBAL hGlobal);

extern void
CPicture_GetImageSize(
    _InRef_     CPicture p_this,
    _OutRef_    LPSIZE p_size /* HIMETRIC */);

_Check_return_
extern BOOL
CPicture_Render(
    _InRef_     CPicture p_this,
    _HdcRef_    HDC hDC,
    _InRef_     LPCRECT p_rect,
    LPCRECT prcMFBounds);

#endif /* OS */

#endif /* ho_cpicture_h */

/* end of ho_cpicture.h */
