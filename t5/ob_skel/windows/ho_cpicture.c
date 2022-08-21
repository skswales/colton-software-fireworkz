/* windows/ho_cpicture.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2006-2016 Stuart Swales */

/* SKS Feb 2006 */

/*
 * CPicture 'class' that uses IPicture to render JPEG objects
 */

#include "common/gflags.h"

#if WINDOWS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/ho_cpicture.h"

#pragma warning(push)
#pragma warning(disable:4820) /* padding added after data member */
#pragma warning(disable:4917) /* 'type' : a GUID can only be associated with a class, interface or namespace */
#include "OCIdl.h"              /* for IPicture, IStream */
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable:4201)   /* nonstandard extension used : nameless struct/union */
#include "OleCtl.h"             /* for OleLoadPicture */
#pragma warning(pop)

struct CPicture_struct
{
    IPicture *m_spIPicture;
};
/* CPicture_implementation */

/******************************************************************************
*
* Create a new CPicture 'object'
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern CPicture
CPicture_New(
    _OutRef_    P_STATUS p_status)
{
    CPicture p_this = al_ptr_calloc_elem(struct CPicture_struct, 1, p_status);

    if(NULL == p_this)
        return(NULL);

    p_this->m_spIPicture = NULL;

    return(p_this);
}

/******************************************************************************
*
* Delete a CPicture 'object'
*
******************************************************************************/

extern void
CPicture_Dispose(
    _Inout_     CPicture * const p_p_this)
{
    CPicture p_this = *p_p_this;

    if(NULL == p_this)
        return;

    if(NULL != p_this->m_spIPicture)
    {
        IPicture_Release(p_this->m_spIPicture);
        p_this->m_spIPicture = NULL;
    }

    al_ptr_dispose(P_P_ANY_PEDANTIC(p_p_this));
}

/******************************************************************************
*
* Load CPicture 'object' from path name
*
* This is the one that really does it: call OleLoadPicturePath to do the work
*
******************************************************************************/

_Check_return_
extern BOOL
CPicture_Load_File(
    _InoutRef_  CPicture p_this,
    _In_z_      PCTSTR pszPathName)
{
    HRESULT hr;
    BOOL bRet;
    WCHAR wszPath[MAX_PATH];

    if(NULL != p_this->m_spIPicture)
    {
        IPicture_Release(p_this->m_spIPicture);
        p_this->m_spIPicture = NULL;
    }

#if TSTR_IS_SBSTR
    { /* convert to WCHAR */
    const UINT mbchars_CodePage = GetACP();

    consume_int(MultiByteToWideChar(mbchars_CodePage, 0 /*dwFlags*/,
                                    pszPathName, -1 /*strlen_with_NULLCH*/,
                                    wszPath, MAX_PATH));
    } /*block*/
#else
    /* copy just in case OleLoadPicturePath() corrupts non-const */
    consume(errno_t, wcscpy_s(wszPath, elemof32(wszPath), pszPathName));
#endif

    hr =
        OleLoadPicturePath(
            wszPath, 0, 0, 0,
#ifndef __cplusplus
            & /* C++ has const IID & */
#endif
            IID_IPicture,
            (void **) &p_this->m_spIPicture);

    if(SUCCEEDED(hr) && (NULL != p_this->m_spIPicture)) { } else { }

    bRet = SUCCEEDED(hr) && (NULL != p_this->m_spIPicture);

    return(bRet);
}

/******************************************************************************
*
* Load CPicture 'object' from memory (HGLOBAL (via IStream))
*
* This is the one that really does it: call OleLoadPicture to do the work
*
******************************************************************************/

_Check_return_
extern BOOL
CPicture_Load_HGlobal(
    _InoutRef_  CPicture p_this,
    _In_        HGLOBAL hGlobal)
{
    HRESULT hr;
    LPSTREAM pstm = NULL;
    BOOL bRet = FALSE;

    if(NULL != p_this->m_spIPicture)
    {
        IPicture_Release(p_this->m_spIPicture);
        p_this->m_spIPicture = NULL;
    }

    /* Create IStream* from global memory */
    hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pstm);
    if(SUCCEEDED(hr))
    { /*EMPTY*/ }
    else
    { pstm = NULL;  }

    if(NULL != pstm)
    {
        hr =
            OleLoadPicture(
                pstm, 0, FALSE,
#ifndef __cplusplus
                & /* C++ has const IID & */
#endif
                IID_IPicture,
                (void **) &p_this->m_spIPicture);

        bRet = SUCCEEDED(hr) && (NULL != p_this->m_spIPicture);

        IStream_Release(pstm);
        pstm = NULL;
    }

    return(bRet);
}

/******************************************************************************
*
* Get CPicture 'object' image size in HIMETRIC
*
******************************************************************************/

extern void
CPicture_GetImageSize(
    _InRef_     CPicture p_this,
    _OutRef_    LPSIZE p_size /* HIMETRIC */)
{
    HRESULT hr;

    p_size->cx = 0;
    p_size->cy = 0;

    if(NULL != p_this->m_spIPicture)
    {
        OLE_XSIZE_HIMETRIC hmWidth = 0;
        OLE_YSIZE_HIMETRIC hmHeight = 0; /* HIMETRIC units */

        hr = IPicture_get_Width(p_this->m_spIPicture, &hmWidth);

        if(SUCCEEDED(hr))
            hr = IPicture_get_Height(p_this->m_spIPicture, &hmHeight);

        if(SUCCEEDED(hr))
        {
            p_size->cx = hmWidth;
            p_size->cy = hmHeight;
        }
    }
}

/******************************************************************************
*
* Render CPicture 'object' to device context
*
******************************************************************************/

_Check_return_
extern BOOL
CPicture_Render(
    _InRef_     CPicture p_this,
    _HdcRef_    HDC hDC,
    _InRef_     LPCRECT p_rect,
    LPCRECT prcMFBounds)
{
    HRESULT hr;
    OLE_XSIZE_HIMETRIC hmWidth = 0;
    OLE_YSIZE_HIMETRIC hmHeight = 0; /* HIMETRIC units */

    if(NULL != p_this->m_spIPicture)
    {
        hr = IPicture_get_Width(p_this->m_spIPicture, &hmWidth);

        if(SUCCEEDED(hr))
            hr = IPicture_get_Height(p_this->m_spIPicture, &hmHeight);

        if(SUCCEEDED(hr))
            hr =
                IPicture_Render(
                    p_this->m_spIPicture,
                    hDC,
                    p_rect->left, p_rect->top,
                    p_rect->right - p_rect->left, p_rect->bottom - p_rect->top,
                    0, hmHeight, hmWidth, -hmHeight,
                    prcMFBounds);
    }

    return(TRUE);
}

#endif /* WINDOWS */

/* end of windows/ho_cpicture.c */
