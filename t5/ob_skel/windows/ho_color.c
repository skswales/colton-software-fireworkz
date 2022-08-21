/* windows/ho_color.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
helper code to map RISC OS like stuff under Windows
*/

HPALETTE g_hPalette = NULL;

UINT g_nColours = UINT_MAX;

typedef struct LOGPALETTE256
{
    WORD palVersion;
    WORD palNumberOfEntries;
    PALETTEENTRY palPalEntry[256];
}
LOGPALETTE256;

/* Clear the System Palette so that we can ensure an identity palette mapping for fast performance */

static void
ClearSystemPalette(void)
{
    LOGPALETTE256 LogPalette;
    const HDC ScreenDC = GetDC(NULL);
    U32 i;
    HPALETTE ClearPalette;

    /* Reset everything in the system palette to black */
    zero_struct(LogPalette);

    LogPalette.palVersion = 0x300;
    LogPalette.palNumberOfEntries = 256;

    for(i = 0; i < 256; ++i)
    {
      /*LogPalette.palPalEntry[i].peRed   = 0;*/
      /*LogPalette.palPalEntry[i].peGreen = 0;*/
      /*LogPalette.palPalEntry[i].peBlue  = 0;*/
        LogPalette.palPalEntry[i].peFlags = PC_NOCOLLAPSE;
    }

    /* Create, select, realize, deselect, and delete the palette */
    ClearPalette = CreatePalette((LOGPALETTE *) &LogPalette);

    if(NULL != ClearPalette)
    {
        UINT nMapped;
        HPALETTE hPalette = SelectPalette(ScreenDC, ClearPalette, FALSE);
        nMapped = RealizePalette(ScreenDC);
        consume(HPALETTE, SelectPalette(ScreenDC, hPalette, FALSE));
        DeletePalette(ClearPalette);
    }

    void_WrapOsBoolChecking(1 == ReleaseDC(NULL, ScreenDC));
}

#define PE_RGB(r, g, b) \
    { (r), (g), (b), PC_NOCOLLAPSE }

static const PALETTEENTRY
default_palette[6 + 8 + 6*6*6] = /* RISC OS colours, splash colours and 666 colour cube */
{
    /* 0, 81, 133, 177, 217, 255 greys will be set by colour cube */
    /*  1 */ PE_RGB(221, 221, 221), /* light grey */
    /*  2 */ PE_RGB(187, 187, 187),
    /*  3 */ PE_RGB(153, 153, 153),
    /*  4 */ PE_RGB(117, 117, 117),
    /*  5 */ PE_RGB( 85,  85,  85),
    /*  6 */ PE_RGB( 51,  51,  51), /* dark grey */

    /*  8 */ PE_RGB(  0, 136,  68), /* dark blue */
    /*  9 */ PE_RGB(255, 238,   0), /* mellow yellow */
    /* 10 */ PE_RGB(  0, 221,   0), /* light green */
    /* 11 */ PE_RGB(221,   0,   0), /* simply red */
    /* 12 */ PE_RGB(238, 238, 204), /* fresh cream */
    /* 13 */ PE_RGB( 68, 136,   0), /* dark green */
    /* 14 */ PE_RGB(255, 204,   0), /* orange */
    /* 15 */ PE_RGB(  0, 255, 255), /* light blue */

    /*PE_RGB(255, 128,  64),*/ /* splashy orange */

    PE_RGB(  0,   0,   0),  PE_RGB(  0,   0,  81),  PE_RGB(  0,   0, 133),  PE_RGB(  0,   0, 177),  PE_RGB(  0,   0, 217),  PE_RGB(  0,   0, 255),
    PE_RGB(  0,  81,   0),  PE_RGB(  0,  81,  81),  PE_RGB(  0,  81, 133),  PE_RGB(  0,  81, 177),  PE_RGB(  0,  81, 217),  PE_RGB(  0,  81, 255),
    PE_RGB(  0, 133,   0),  PE_RGB(  0, 133,  81),  PE_RGB(  0, 133, 133),  PE_RGB(  0, 133, 177),  PE_RGB(  0, 133, 217),  PE_RGB(  0, 133, 255),
    PE_RGB(  0, 177,   0),  PE_RGB(  0, 177,  81),  PE_RGB(  0, 177, 133),  PE_RGB(  0, 177, 177),  PE_RGB(  0, 177, 217),  PE_RGB(  0, 177, 255),
    PE_RGB(  0, 217,   0),  PE_RGB(  0, 217,  81),  PE_RGB(  0, 217, 133),  PE_RGB(  0, 217, 177),  PE_RGB(  0, 217, 217),  PE_RGB(  0, 217, 255),
    PE_RGB(  0, 255,   0),  PE_RGB(  0, 255,  81),  PE_RGB(  0, 255, 133),  PE_RGB(  0, 255, 177),  PE_RGB(  0, 255, 217),  PE_RGB(  0, 255, 255),

    PE_RGB( 81,   0,   0),  PE_RGB( 81,   0,  81),  PE_RGB( 81,   0, 133),  PE_RGB( 81,   0, 177),  PE_RGB( 81,   0, 217),  PE_RGB( 81,   0, 255),
    PE_RGB( 81,  81,   0),  PE_RGB( 81,  81,  81),  PE_RGB( 81,  81, 133),  PE_RGB( 81,  81, 177),  PE_RGB( 81,  81, 217),  PE_RGB( 81,  81, 255),
    PE_RGB( 81, 133,   0),  PE_RGB( 81, 133,  81),  PE_RGB( 81, 133, 133),  PE_RGB( 81, 133, 177),  PE_RGB( 81, 133, 217),  PE_RGB( 81, 133, 255),
    PE_RGB( 81, 177,   0),  PE_RGB( 81, 177,  81),  PE_RGB( 81, 177, 133),  PE_RGB( 81, 177, 177),  PE_RGB( 81, 177, 217),  PE_RGB( 81, 177, 255),
    PE_RGB( 81, 217,   0),  PE_RGB( 81, 217,  81),  PE_RGB( 81, 217, 133),  PE_RGB( 81, 217, 177),  PE_RGB( 81, 217, 217),  PE_RGB( 81, 217, 255),
    PE_RGB( 81, 255,   0),  PE_RGB( 81, 255,  81),  PE_RGB( 81, 255, 133),  PE_RGB( 81, 255, 177),  PE_RGB( 81, 255, 217),  PE_RGB( 81, 255, 255),

    PE_RGB(133,   0,   0),  PE_RGB(133,   0,  81),  PE_RGB(133,   0, 133),  PE_RGB(133,   0, 177),  PE_RGB(133,   0, 217),  PE_RGB(133,   0, 255),
    PE_RGB(133,  81,   0),  PE_RGB(133,  81,  81),  PE_RGB(133,  81, 133),  PE_RGB(133,  81, 177),  PE_RGB(133,  81, 217),  PE_RGB(133,  81, 255),
    PE_RGB(133, 133,   0),  PE_RGB(133, 133,  81),  PE_RGB(133, 133, 133),  PE_RGB(133, 133, 177),  PE_RGB(133, 133, 217),  PE_RGB(133, 133, 255),
    PE_RGB(133, 177,   0),  PE_RGB(133, 177,  81),  PE_RGB(133, 177, 133),  PE_RGB(133, 177, 177),  PE_RGB(133, 177, 217),  PE_RGB(133, 177, 255),
    PE_RGB(133, 217,   0),  PE_RGB(133, 217,  81),  PE_RGB(133, 217, 133),  PE_RGB(133, 217, 177),  PE_RGB(133, 217, 217),  PE_RGB(133, 217, 255),
    PE_RGB(133, 255,   0),  PE_RGB(133, 255,  81),  PE_RGB(133, 255, 133),  PE_RGB(133, 255, 177),  PE_RGB(133, 255, 217),  PE_RGB(133, 255, 255),

    PE_RGB(177,   0,   0),  PE_RGB(177,   0,  81),  PE_RGB(177,   0, 133),  PE_RGB(177,   0, 177),  PE_RGB(177,   0, 217), PE_RGB(177,   0, 255),
    PE_RGB(177,  81,   0),  PE_RGB(177,  81,  81),  PE_RGB(177,  81, 133),  PE_RGB(177,  81, 177),  PE_RGB(177,  81, 217), PE_RGB(177,  81, 255),
    PE_RGB(177, 133,   0),  PE_RGB(177, 133,  81),  PE_RGB(177, 133, 133),  PE_RGB(177, 133, 177),  PE_RGB(177, 133, 217), PE_RGB(177, 133, 255),
    PE_RGB(177, 177,   0),  PE_RGB(177, 177,  81),  PE_RGB(177, 177, 133),  PE_RGB(177, 177, 177),  PE_RGB(177, 177, 217), PE_RGB(177, 177, 255),
    PE_RGB(177, 217,   0),  PE_RGB(177, 217,  81),  PE_RGB(177, 217, 133),  PE_RGB(177, 217, 177),  PE_RGB(177, 217, 217), PE_RGB(177, 217, 255),
    PE_RGB(177, 255,   0),  PE_RGB(177, 255,  81),  PE_RGB(177, 255, 133),  PE_RGB(177, 255, 177),  PE_RGB(177, 255, 217), PE_RGB(177, 255, 255),

    PE_RGB(217,   0,   0),  PE_RGB(217,   0,  81),  PE_RGB(217,   0, 133),  PE_RGB(217,   0, 177),  PE_RGB(217,   0, 217),  PE_RGB(217,   0, 255),
    PE_RGB(217,  81,   0),  PE_RGB(217,  81,  81),  PE_RGB(217,  81, 133),  PE_RGB(217,  81, 177),  PE_RGB(217,  81, 217),  PE_RGB(217,  81, 255),
    PE_RGB(217, 133,   0),  PE_RGB(217, 133,  81),  PE_RGB(217, 133, 133),  PE_RGB(217, 133, 177),  PE_RGB(217, 133, 217),  PE_RGB(217, 133, 255),
    PE_RGB(217, 177,   0),  PE_RGB(217, 177,  81),  PE_RGB(217, 177, 133),  PE_RGB(217, 177, 177),  PE_RGB(217, 177, 217),  PE_RGB(217, 177, 255),
    PE_RGB(217, 217,   0),  PE_RGB(217, 217,  81),  PE_RGB(217, 217, 133),  PE_RGB(217, 217, 177),  PE_RGB(217, 217, 217),  PE_RGB(217, 217, 255),
    PE_RGB(217, 255,   0),  PE_RGB(217, 255,  81),  PE_RGB(217, 255, 133),  PE_RGB(217, 255, 177),  PE_RGB(217, 255, 217),  PE_RGB(217, 255, 255),

    PE_RGB(255,   0,   0),  PE_RGB(255,   0,  81),  PE_RGB(255,   0, 133),  PE_RGB(255,   0, 177),  PE_RGB(255,   0, 217),  PE_RGB(255,   0, 255),
    PE_RGB(255,  81,   0),  PE_RGB(255,  81,  81),  PE_RGB(255,  81, 133),  PE_RGB(255,  81, 177),  PE_RGB(255,  81, 217),  PE_RGB(255,  81, 255),
    PE_RGB(255, 133,   0),  PE_RGB(255, 133,  81),  PE_RGB(255, 133, 133),  PE_RGB(255, 133, 177),  PE_RGB(255, 133, 217),  PE_RGB(255, 133, 255),
    PE_RGB(255, 177,   0),  PE_RGB(255, 177,  81),  PE_RGB(255, 177, 133),  PE_RGB(255, 177, 177),  PE_RGB(255, 177, 217),  PE_RGB(255, 177, 255),
    PE_RGB(255, 217,   0),  PE_RGB(255, 217,  81),  PE_RGB(255, 217, 133),  PE_RGB(255, 217, 177),  PE_RGB(255, 217, 217),  PE_RGB(255, 217, 255),
    PE_RGB(255, 255,   0),  PE_RGB(255, 255,  81),  PE_RGB(255, 255, 133),  PE_RGB(255, 255, 177),  PE_RGB(255, 255, 217),  PE_RGB(255, 255, 255)
};

static void
load_default_palette(
    _HdcRef_    HDC hDC,
    _Out_       LOGPALETTE256 * const LogPalette)
{
    const int nStaticColors = GetDeviceCaps(hDC, NUMRESERVED);
    const int half_nStaticColors = nStaticColors / 2;
    int i, j;

    LogPalette->palVersion = 0x300;
    LogPalette->palNumberOfEntries = 256;

    void_WrapOsBoolChecking(GetSystemPaletteEntries(hDC, 0, 256, &LogPalette->palPalEntry[0]));

    /* Clear the peFlags of the lower and upper static colours */
    for(i = 0; i < half_nStaticColors; ++i)
        LogPalette->palPalEntry[i].peFlags = 0;

    for(i = 256 - half_nStaticColors; i < 256; ++i)
        LogPalette->palPalEntry[i].peFlags = 0;

    /* Only copy as much as we can in between the lower and upper static colours */
    for(i = half_nStaticColors, j = 0; (i < 256 - half_nStaticColors) && (j < elemof32(default_palette)); ++i, j += 3)
        LogPalette->palPalEntry[i] = default_palette[j];

    /* Any remaining entries are zapped */
    for( ; i < 256 - half_nStaticColors; ++i)
        LogPalette->palPalEntry[i] = default_palette[0];
}

extern void
host_create_default_palette(void)
{
    const HDC hDC = GetDC(NULL);
    TCHARZ buffer[16];

    host_destroy_default_palette(); /* default is not to do any palette mapping */

    g_nColours = UINT_MAX;

    if(0 != (RC_PALETTE & GetDeviceCaps(hDC, RASTERCAPS)))
        g_nColours = GetSystemPaletteEntries(hDC, 0, 0, NULL);

    /* SKS notes 15mar94 that you get very small values in palette entries with 16 and 24 bit modes */
    /* SKS notes 07jan96 that this is NOT true on NT3.51 or Win95(490) hence extra test */

    if((g_nColours != 0) && (g_nColours != UINT_MAX))
    {
        /* 16 color modes never allow hardware palette mapping, and certainly don't bother with > 256 colour modes! */
        if(0 != MyGetProfileString(TEXT("NumColours"), tstr_empty_string, buffer, elemof32(buffer))) /* SKS 14oct96 hack to allow no palette mucking about for debugging */
            g_nColours = _tstoi(buffer); /*atoi*/
    }

    if(g_nColours == 256)
    {
        LOGPALETTE256 LogPalette;

        load_default_palette(hDC, &LogPalette);

        ClearSystemPalette();

        g_hPalette = CreatePalette((LOGPALETTE *) &LogPalette);

#if 1
        if(NULL != g_hPalette)
        {
            UINT nMapped;
            HPALETTE hPalette = SelectPalette(hDC, g_hPalette, FALSE);
            nMapped = RealizePalette(hDC);
            consume(HPALETTE, SelectPalette(hDC, hPalette, FALSE));
        }
#endif
    }

    void_WrapOsBoolChecking(1 == ReleaseDC(NULL, hDC));
}

extern void
host_destroy_default_palette(void)
{
    if(NULL != g_hPalette)
    {
        DeletePalette(g_hPalette);
        g_hPalette = NULL;
    }
}

extern void
host_select_default_palette(
    _HdcRef_    HDC hdc,
    _Out_       HPALETTE * const p_h_palette)
{
    if(NULL != g_hPalette)
    {
        UINT nMapped;
        *p_h_palette = SelectPalette(hdc, g_hPalette, FALSE);
        nMapped = RealizePalette(hdc);
        return;
    }
    *p_h_palette = NULL;
}

extern void
host_select_old_palette(
    _HdcRef_    HDC hdc,
    _Inout_     HPALETTE * const p_h_palette)
{
    UNREFERENCED_PARAMETER_CONST(hdc);
    if(NULL != *p_h_palette)
    { /*EMPTY*/ } /*SelectPalette(hdc, *p_h_palette, FALSE)*/
    *p_h_palette = NULL;
}

_Check_return_
static COLORREF
host_wimp_colorref(
    _InVal_     UINT index)
{
    switch(index)
    {
    default: /* RISC OS Wimp colours:    R,   G,   B */

    case 0:     return(RGB(255, 255, 255)); /* white */
    case 1:     return(RGB(221, 221, 221));
    case 2:     return(RGB(187, 187, 187));
    case 3:     return(RGB(153, 153, 153)); /* greyscale */
    case 4:     return(RGB(117, 117, 117));
    case 5:     return(RGB( 85,  85,  85));
    case 6:     return(RGB( 51,  51,  51));
    case 7:     return(RGB(  0,   0,   0)); /* black */

    case 8:     return(RGB(  0, 136,  68)); /* dark blue */
    case 9:     return(RGB(255, 238,   0)); /* mellow yellow */
    case 10:    return(RGB(  0, 221,   0)); /* light green */
    case 11:    return(RGB(221,   0,   0)); /* simply red */
    case 12:    return(RGB(238, 238, 204)); /* fresh cream */
    case 13:    return(RGB( 68, 136,   0)); /* dark green */
    case 14:    return(RGB(255, 204,   0)); /* orange */
    case 15:    return(RGB(  0, 255, 255)); /* light blue */
    }
}

/* The RGB stash defines the RGB values used for painting the colour patches.
 * It also contains the RGB values of some important system colours used for painting, saving looking them up.
 */

RGB
rgb_stash[COLOUR_STASH_MAX];

/* Given a COLORREF setup the RGB pointed to contain something sensible.  Transparent is forced to be FALSE */

static inline void
host_set_rgb_from_colorref(
    _OutRef_    P_RGB p_rgb,
    _In_        COLORREF color_ref)
{
    rgb_set(p_rgb, GetRValue(color_ref), GetGValue(color_ref), GetBValue(color_ref));
}

extern void
host_rgb_stash_colour(
    _HdcRef_    HDC hDC,
    _In_        UINT index)
{
    COLORREF color_ref_1 = host_wimp_colorref(index);
    COLORREF color_ref_2 = (g_nColours >= 256) ? color_ref_1 : GetNearestColor(hDC, color_ref_1);
    host_set_rgb_from_colorref(&rgb_stash[index], color_ref_2);
}

extern void
host_rgb_stash(
    _HdcRef_    HDC hDC) /* SKS 23jan95 now caches using the given hDC/palette */
{
    UINT index;

    /* can presumably set up our favouite Wimp 16 colours and get almost sensible representations thereof */
    for(index = 0; index < 16; ++index)
        host_rgb_stash_colour(hDC, index);
}

#include "commdlg.h"

#include "cderr.h"

static COLORREF
choosecolor_custom_colours[16] = /* all white */
{
    RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
    RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
    RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
    RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF)
};

static void
choosecolor_custom_colours_init(void)
{
    static BOOL f_choosecolor_custom_colours_init = FALSE;

    UINT index;

    if(f_choosecolor_custom_colours_init)
        return;

    for(index = 0; index < 16; ++index)
        choosecolor_custom_colours[index] = RGB(rgb_stash[index].r, rgb_stash[index].g, rgb_stash[index].b);

    f_choosecolor_custom_colours_init = TRUE;
}

extern BOOL
windows_colour_picker(
    HOST_WND    parent_window_handle,
    _InoutRef_  P_RGB p_rgb)
{
    BOOL res;
    CHOOSECOLOR choosecolor;
    choosecolor.lStructSize = sizeof32(choosecolor);
    choosecolor.hwndOwner = parent_window_handle;
    choosecolor.hInstance = (HWND /* bug in commdlg.h! */) GetInstanceHandle();
    choosecolor.rgbResult = RGB(p_rgb->r, p_rgb->g, p_rgb->b); /* it's a COLORREF */
    choosecolor_custom_colours_init();
    choosecolor.lpCustColors = choosecolor_custom_colours;
    choosecolor.Flags = CC_RGBINIT | CC_FULLOPEN;
    choosecolor.lCustData = 0;
    choosecolor.lpfnHook = NULL;
    choosecolor.lpTemplateName = NULL;
    res = ChooseColor(&choosecolor);
    if(!res)
    {
        /* extended error sometime */
    }
    else
    {
        rgb_set(p_rgb, GetRValue(choosecolor.rgbResult), GetGValue(choosecolor.rgbResult), GetBValue(choosecolor.rgbResult));
    }
    return(res);
}

/* end of windows/ho_color.c */
