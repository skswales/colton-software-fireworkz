/* coltsoft/filetype.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __filetype_h
#define __filetype_h

/*
RISC OS file types
*/

typedef enum T5_FILETYPE
{
    FILETYPE_WV_V10      = 0x004,
    FILETYPE_WV_V12      = 0x006,
    FILETYPE_ICO         = 0x132,
    FILETYPE_TSTEP_128W  = 0x300,
    FILETYPE_RAYSHADE    = 0x371,
    FILETYPE_CCIR_601    = 0x601,
    FILETYPE_TCLEAR      = 0x690,
    FILETYPE_DEGAS       = 0x691,
    FILETYPE_GIF         = 0x695,
    FILETYPE_PCX         = 0x697,
    FILETYPE_QRT         = 0x698,
    FILETYPE_MTV         = 0x699,
    FILETYPE_BMP         = 0x69C, /* As Allocated by Acorn PMF */
    FILETYPE_TGA         = 0x69D,
    FILETYPE_CUR         = 0x6A5,
    FILETYPE_TSTEP_800S  = 0x7A0,
    FILETYPE_FWP         = 0xAF8,
    FILETYPE_DRAW        = 0xAFF,
    FILETYPE_PNG         = 0xB60,
    FILETYPE_MS_XLS      = 0xBA6,
    FILETYPE_WMF         = 0xB2F,
    FILETYPE_T5_FIREWORKZ= 0xBDF,
    FILETYPE_T5_RECORDZ  = 0xBE0,
    FILETYPE_T5_RESULTZ  = 0xBE1,
    FILETYPE_PCD         = 0xBE8,
    FILETYPE_T5_WORDZ    = 0xC1C,
    FILETYPE_T5_TEMPLATE = 0xC1D,
    FILETYPE_T5_COMMAND  = 0xC1E,
    FILETYPE_DATAPOWER   = 0xC27,
    FILETYPE_DATAPOWERGPH= 0xC28,
    FILETYPE_RTF         = 0xC32,
    FILETYPE_VECTOR      = 0xC56,
    FILETYPE_POSTER      = 0xCC3,
    FILETYPE_SID         = 0xC7D,
    FILETYPE_JPEG        = 0xC85,
    FILETYPE_PDMACRO     = 0xD21,
    FILETYPE_LOTUS123    = 0xDB0, /* .WK1 */
    FILETYPE_PIPEDREAM   = 0xDDE,
    FILETYPE_PROARTISAN  = 0xDE2,
    FILETYPE_WATFORD_DFA = 0xDFA,
    FILETYPE_CSV         = 0xDFE,
    FILETYPE_WAP         = 0xF8F,
    FILETYPE_DOS         = 0xFE4,
    FILETYPE_VIEW        = 0xFE9,
    FILETYPE_TIFF        = 0xFF0,
    FILETYPE_PRINTOUT    = 0xFF4,
    FILETYPE_SPRITE      = 0xFF9,
    FILETYPE_DATA        = 0xFFD,
    FILETYPE_TEXT        = 0xFFF,
    FILETYPE_ASCII       = 0xFFF,
    FILETYPE_DTP         = 0xFFF, /* NGardner says use this */

    FILETYPE_DIRECTORY   = 0x1000,
    FILETYPE_APPLICATION = 0x2000,
    FILETYPE_UNTYPED     = 0x3000,

    FILETYPE_T5_PRIVATE  = (-65536), /*FFFF0000*/
    FILETYPE_WINDOWS_EMF = FILETYPE_T5_PRIVATE - 1, /*FFFF0001*/

    FILETYPE_UNDETERMINED = -1
}
T5_FILETYPE, * P_T5_FILETYPE;

#endif /* __filetype_h */

/* end of coltsoft/filetype.h */
