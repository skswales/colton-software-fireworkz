/* ho_gdip.cpp */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2013-2020 Stuart Swales */

/* Remember to disable use of precompiled headers for this file */

#define CPLUSPLUSINTERFACE

#include "common/gflags.h"

#pragma warning(disable:4263) /* 'x' : member function does not override any base class virtual member function */
#pragma warning(disable:4264) /* 'x' : no override available for virtual member function from base 'y'; function is hidden */

#if _MSC_VER >= 1910 /* VS2017 or later */
#pragma warning(disable:5039) /* 'function' : pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception */
#pragma warning(disable:4596) /* Illegal qualified name in member declaration */
#endif

#if _MSC_VER >= 1900 /* VS2015 or later */
#pragma warning(disable:4458) /* declaration of 'x' hides class member */
#endif

#if _MSC_VER < 1900 /* < VS2015 */
#pragma warning(disable:4738) /* storing 32-bit float result in memory, possible loss of performance */
#endif

#include <gdiplus.h>
/* using namespace Gdiplus; prefer to be explict */

#pragma comment(lib, "Gdiplus.lib")

static ULONG_PTR g_gdiplusToken;

extern "C" void
gdiplus_startup(void)
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;

    const Gdiplus::Status status = Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    if(Gdiplus::Ok != status)
    {
    }
}

extern "C" void
gdiplus_shutdown(void)
{
    Gdiplus::GdiplusShutdown(g_gdiplusToken);
}

extern "C" HBITMAP
gdiplus_load_bitmap_from_file(
    _In_z_      PCTSTR filename)
{
    PCWSTR wstr_filename = _wstr_from_tstr(filename);
    Gdiplus::Bitmap mBitmap(wstr_filename, false);
    HBITMAP result;
    mBitmap.GetHBITMAP(0x00000000, &result);
    return(result);
}

/*
* GdipImage 'class' that uses GDI+ Image and Graphics classes to render images
*/

#include "ob_skel/ho_gdip_image.h"

struct GdipImage_struct
{
    HGLOBAL hGlobal; /* GDI+ needs a copy to be available at all times */
    IStream * stream;

    Gdiplus::Bitmap * bitmap;
    Gdiplus::Metafile * metafile;
};
/* GdipImage_implementation */

/******************************************************************************
*
* Create a new GdipImage 'object'
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern "C" STATUS
GdipImage_New(
    _Out_       GdipImage * const p_p_new)
{
    STATUS status;
    GdipImage p_this = al_ptr_calloc_bytes(GdipImage, sizeof32(GdipImage_struct), &status);
    *p_p_new = p_this;
    return(status);
}

/******************************************************************************
*
* Delete a GdipImage 'object'
*
******************************************************************************/

static void
GdipImage_Dispose_data(
    _InoutRef_  GdipImage p_this)
{
    if(NULL != p_this->metafile)
    {
        delete p_this->metafile;
        p_this->metafile = NULL;
    }

    if(NULL != p_this->bitmap)
    {
        delete p_this->bitmap;
        p_this->bitmap = NULL;
    }

    /* can only free these resources once their users have gone */
    if(NULL != p_this->stream)
    {
        p_this->stream->Release();
        p_this->stream = NULL;
    }

    if(NULL != p_this->hGlobal)
    {
        ::GlobalUnlock(p_this->hGlobal);
        ::GlobalFree(p_this->hGlobal);
        p_this->hGlobal = NULL;
    }
}

extern "C" void
GdipImage_Dispose(
    _Inout_     GdipImage * const p_p_this)
{
    GdipImage p_this = *p_p_this;

    if(NULL == p_this)
        return;

    GdipImage_Dispose_data(p_this);

    al_ptr_dispose(P_P_ANY_PEDANTIC(p_p_this));
}

/******************************************************************************
*
* Load GdipImage 'object' from file
*
******************************************************************************/

_Check_return_
extern "C" BOOL
GdipImage_Load_File(
    _InoutRef_  GdipImage p_this,
    _In_z_      PCWSTR wstr_filename,
    _InVal_     T5_FILETYPE t5_filetype)
{
    BOOL bRet = FALSE;

    GdipImage_Dispose_data(p_this);

    switch(t5_filetype)
    {
    default:
        p_this->bitmap = new Gdiplus::Bitmap(wstr_filename);
        bRet = (NULL != p_this->bitmap);
        if(bRet)
            bRet = (Gdiplus::Ok == p_this->bitmap->GetLastStatus());
        break;

    case FILETYPE_WMF:
    case FILETYPE_WINDOWS_EMF:
        p_this->metafile = new Gdiplus::Metafile(wstr_filename);
        bRet = (NULL != p_this->metafile);
        if(bRet)
            bRet = (Gdiplus::Ok == p_this->metafile->GetLastStatus());
        break;
    }

    if(!bRet)
        GdipImage_Dispose_data(p_this);

    return(bRet);
}

/******************************************************************************
*
* Load GdipImage 'object' from memory
*
******************************************************************************/

_Check_return_
extern "C" BOOL
GdipImage_Load_Memory(
    _InoutRef_  GdipImage p_this,
    _In_reads_(n_bytes) PC_BYTE p_data,
    _InVal_     U32 n_bytes,
    _InVal_     T5_FILETYPE t5_filetype)
{
    BOOL bRet = FALSE;

    GdipImage_Dispose_data(p_this);

    p_this->hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, n_bytes);
    if(NULL == p_this->hGlobal)
        return(FALSE);

    void * pBuffer = ::GlobalLock(p_this->hGlobal);
    PTR_ASSERT(pBuffer);
    ::CopyMemory(pBuffer, p_data, n_bytes);

    /* Create IStream* from global memory */
    HRESULT hr = ::CreateStreamOnHGlobal(p_this->hGlobal, FALSE, &p_this->stream);
    if(SUCCEEDED(hr))
    { /*EMPTY*/ }
    else
    { p_this->stream = NULL;  }

    if(NULL != p_this->stream)
    {
        switch(t5_filetype)
        {
        default:
            p_this->bitmap = new Gdiplus::Bitmap(p_this->stream);
            bRet = (NULL != p_this->bitmap);
            if(bRet)
                bRet = (Gdiplus::Ok == p_this->bitmap->GetLastStatus());
            break;

        case FILETYPE_WMF:
        case FILETYPE_WINDOWS_EMF:
            p_this->metafile = new Gdiplus::Metafile(p_this->stream);
            bRet = (NULL != p_this->metafile);
            if(bRet)
                bRet = (Gdiplus::Ok == p_this->metafile->GetLastStatus());
            break;
        }
    }

    if(!bRet)
        GdipImage_Dispose_data(p_this);

    return(bRet);
}

/******************************************************************************
*
* Get GdipImage 'object' image size in TWIPS
*
******************************************************************************/

extern "C" void
GdipImage_GetImageSize(
    _InRef_     GdipImage p_this,
    _OutRef_    PSIZE p_size /* TWIPS */)
{
    Gdiplus::Image * image = NULL;
    
    if(NULL != p_this->bitmap)
        image = p_this->bitmap;
    else if(NULL != p_this->metafile)
        image = p_this->metafile;

    p_size->cx = 0;
    p_size->cy = 0;

    if((NULL != image) && (Gdiplus::Ok == image->GetLastStatus()))
    {
        const UINT x = image->GetWidth();
        const UINT y = image->GetHeight();
        const F64 h_dpi = image->GetHorizontalResolution();
        const F64 v_dpi = image->GetVerticalResolution();

        p_size->cx = (LONG) ceil(((F64) x * PIXITS_PER_INCH) / h_dpi);
        p_size->cy = (LONG) ceil(((F64) y * PIXITS_PER_INCH) / v_dpi);
    }
}

/******************************************************************************
*
* Render GdipImage 'object' to device context
*
******************************************************************************/

_Check_return_
static BOOL
GdipImage_Render_bitmap(
    _InRef_     GdipImage p_this,
    _HdcRef_    HDC hDC,
    _InRef_     PCRECT p_rect)
{
    Gdiplus::Bitmap * bitmap = p_this->bitmap;

    const INT x = p_rect->left;
    const INT y = p_rect->top;
    const INT width  = p_rect->right - p_rect->left;
    const INT height = p_rect->bottom - p_rect->top;

    Gdiplus::Graphics graphics(hDC);
    const Gdiplus::Status status = graphics.DrawImage(bitmap, x, y, width, height);
    return(Gdiplus::Ok == status);
}

_Check_return_
static BOOL
GdipImage_Render_metafile(
    _InRef_     GdipImage p_this,
    _HdcRef_    HDC hDC,
    _InRef_     PCRECT p_rect)
{
    Gdiplus::Metafile * metafile = p_this->metafile;

    const INT x = p_rect->left;
    const INT y = p_rect->top;
    const INT width  = p_rect->right - p_rect->left;
    const INT height = p_rect->bottom - p_rect->top;

    Gdiplus::Graphics graphics(hDC);
    const Gdiplus::Status status = graphics.DrawImage(metafile, x, y, width, height);
    return(Gdiplus::Ok == status);
}

_Check_return_
extern "C" BOOL
GdipImage_Render(
    _InRef_     GdipImage p_this,
    _HdcRef_    HDC hDC,
    _InRef_     PCRECT p_rect)
{
    if(NULL != p_this->bitmap)
        return(GdipImage_Render_bitmap(p_this, hDC, p_rect));

    if(NULL != p_this->metafile)
        return(GdipImage_Render_metafile(p_this, hDC, p_rect));

    return(FALSE);
}

// This helper function from MSDN 'Retrieving the Class Identifier for an Encoder'

static int
GetEncoderClsid(
    const WCHAR * format,
    CLSID * pClsid)
{
    UINT num = 0;   // number of image encoders
    UINT size = 0;  // size of the image encoder array in bytes

    Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

    if((Gdiplus::Ok != Gdiplus::GetImageEncodersSize(&num, &size)) || (size == 0))
        return -1;  // Failure

    pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if(NULL == pImageCodecInfo)
        return -1;  // Failure

    if(Gdiplus::Ok == Gdiplus::GetImageEncoders(num, size, pImageCodecInfo))
    {
        for(UINT j = 0; j < num; ++j)
        {
            if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
            {
                *pClsid = pImageCodecInfo[j].Clsid;
                free(pImageCodecInfo);
                return j;   // Success
            }    
        }
    }

    free(pImageCodecInfo);
    return -1;  // Failure
}

_Check_return_
extern "C" BOOL
GdipImage_SaveAs_BMP(
    _InoutRef_  GdipImage p_this,
    _In_z_      PCWSTR wstr_filename)
{
    BOOL bRet = FALSE;
    Gdiplus::Image * image = NULL;
    
    if(NULL != p_this->bitmap)
        image = p_this->bitmap;
    else if(NULL != p_this->metafile)
        image = p_this->metafile;

    if(NULL == image)
        return(FALSE);

    CLSID clsidEncoder;

    if(-1 == GetEncoderClsid(L"image/bmp", &clsidEncoder))
        return(FALSE);

    /* bizarre workaround for some PNG files that gave Win32Error when saved directly */
    Gdiplus::Bitmap * dest = new Gdiplus::Bitmap(image->GetWidth(), image->GetHeight(), PixelFormat32bppARGB);

    /* create a graphics from the dest image */
    Gdiplus::Graphics * g = Gdiplus::Graphics::FromImage(dest);

    /* draw the source image into the desired format dest image */
    g->DrawImage(image, Gdiplus::Point(0, 0));

    const Gdiplus::Status status = dest->Save(wstr_filename, &clsidEncoder);

    if(Gdiplus::Ok == status)
    {
        bRet = TRUE;
    }
    else
    {
        (void) dest->GetLastStatus();
        bRet = FALSE;
    }

    delete g;
    delete dest;

    return(bRet);
}

/* end of ho_gdip.cpp */
