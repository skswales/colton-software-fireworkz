/* ucs4conv.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2006-2019 Stuart Swales */

/* Library module for UCS-4 character conversion */

/* SKS Oct 2006 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/collect.h"

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

#include "cmodules/unicode/u0000.h" /* 0000..007F Basic Latin */
#include "cmodules/unicode/u0080.h" /* 0080..00FF Latin-1 Supplement */
#include "cmodules/unicode/u0100.h" /* 0100..017F Latin Extended-A */
#include "cmodules/unicode/u0180.h" /* 0180..024F Latin Extended-B */
#include "cmodules/unicode/u02B0.h" /* 02B0..02FF Spacing Modifier Letters */
#include "cmodules/unicode/u2000.h" /* 2000..206F General Punctuation */
#include "cmodules/unicode/u20A0.h" /* 20A0..20CF Currency Symbols */
#include "cmodules/unicode/u2100.h" /* 2100..214F Letterlike Symbols */
#include "cmodules/unicode/uFB00.h" /* FB00..FB4F Alphabetic Presentation Forms */

/*
2200..22FF Mathematical Operators
*/

#define UCH_MINUS_SIGN__UNICODE     0x2212U

_Check_return_
extern SBCHAR_CODEPAGE
get_system_codepage(void)
{
#if WINDOWS

    return((U32) GetACP());

#elif RISCOS

    static SBCHAR_CODEPAGE cached_system_codepage;

    if(0 == cached_system_codepage)
    {
        switch(host_query_alphabet_number())
        {
        default: default_unhandled();
        case 101:
            cached_system_codepage = SBCHAR_CODEPAGE_ALPHABET_LATIN1;
            break;

        case 102:
            cached_system_codepage = SBCHAR_CODEPAGE_ALPHABET_LATIN2;
            break;

        case 103:
            cached_system_codepage = SBCHAR_CODEPAGE_ALPHABET_LATIN3;
            break;

        case 104:
            cached_system_codepage = SBCHAR_CODEPAGE_ALPHABET_LATIN4;
            break;

        case 112:
            cached_system_codepage = SBCHAR_CODEPAGE_ALPHABET_LATIN9;
            break;
        }
    }

    return(cached_system_codepage);

#endif /* OS */
}

/********************************
*
* conversion from SBCHAR to UCS-4
*
********************************/

/**********************
*
* ISO 8859-1 & variants
*
**********************/

/*
ISO 8859-1
*/

_Check_return_
static UCS4
ucs4_from_sbchar_ISO_8859_1(
    _InVal_     SBCHAR sbchar)
{
    UCS4 ucs4 = (UCS4) sbchar;

    /* all code points in 0000..00FF in ISO 8859-1 are Unicode code points */

    return(ucs4);
}

/*
Windows-28591
*/

/* Windows-28591 is ISO 8859-1 */
#define ucs4_from_sbchar_Windows_28591(sbchar) ( \
    ucs4_from_sbchar_ISO_8859_1(sbchar) )

/*
Windows-1252
*/

static const UCS4
ucs4_from_sbchar_Windows_1252_C1[0x20] =
{           /* Windows-1252 extensions are all in C1 */
/*0x80*/    UCH_EURO_CURRENCY_SIGN,
/*0x81*/    0x0081,                 /* should not be present */
/*0x82*/    UCH_SINGLE_LOW_9_QUOTATION_MARK,
/*0x83*/    UCH_LATIN_SMALL_LETTER_F_WITH_HOOK,
/*0x84*/    UCH_DOUBLE_LOW_9_QUOTATION_MARK,
/*0x85*/    UCH_HORIZONTAL_ELLIPSIS,
/*0x86*/    UCH_DAGGER,
/*0x87*/    UCH_DOUBLE_DAGGER,
/*0x88*/    UCH_MODIFIER_LETTER_CIRCUMFLEX_ACCENT,
/*0x89*/    UCH_PER_MILLE_SIGN,
/*0x8A*/    UCH_LATIN_CAPITAL_LETTER_S_WITH_CARON,
/*0x8B*/    UCH_SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK,
/*0x8C*/    UCH_LATIN_CAPITAL_LIGATURE_OE,
/*0x8D*/    0x008D,                 /* should not be present */
/*0x8E*/    UCH_LATIN_CAPITAL_LETTER_Z_WITH_CARON,
/*0x8F*/    0x008F,                 /* should not be present */

/*0x90*/    0x0090,                 /* should not be present */
/*0x91*/    UCH_LEFT_SINGLE_QUOTATION_MARK,
/*0x92*/    UCH_RIGHT_SINGLE_QUOTATION_MARK,
/*0x93*/    UCH_LEFT_DOUBLE_QUOTATION_MARK,
/*0x94*/    UCH_RIGHT_DOUBLE_QUOTATION_MARK,
/*0x95*/    UCH_BULLET,
/*0x96*/    UCH_EN_DASH,
/*0x97*/    UCH_EM_DASH,
/*0x98*/    UCH_SMALL_TILDE,
/*0x99*/    UCH_TRADE_MARK_SIGN,
/*0x9A*/    UCH_LATIN_SMALL_LETTER_S_WITH_CARON,
/*0x9B*/    UCH_SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK,
/*0x9C*/    UCH_LATIN_SMALL_LIGATURE_OE,
/*0x9D*/    0x009D,                 /* should not be present */
/*0x9E*/    UCH_LATIN_SMALL_LETTER_Z_WITH_CARON,
/*0x9F*/    UCH_LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS
};

_Check_return_
static UCS4
ucs4_from_sbchar_Windows_1252(
    _InVal_     SBCHAR sbchar)
{
    UCS4 ucs4 = (UCS4) sbchar;

    if(ucs4_is_C1(ucs4)) /* Windows-1252 extensions are all in C1 */
        ucs4 = ucs4_from_sbchar_Windows_1252_C1[ucs4 - 0x80];

    /* all other code points in 0000..00FF in Windows-1252 are Unicode code points */

    return(ucs4);
}

/*
Acorn Extended Latin-1
*/

static const UCS4
ucs4_from_sbchar_Alphabet_Latin1_C1[0x20] =
{           /* Acorn Extended Latin-1 extensions are all in C1 */
/*0x80*/    UCH_EURO_CURRENCY_SIGN,
/*0x81*/    UCH_LATIN_CAPITAL_LETTER_W_WITH_CIRCUMFLEX,
/*0x82*/    UCH_LATIN_SMALL_LETTER_W_WITH_CIRCUMFLEX,
/*0x83*/    0x0083,                 /* should not be present */
/*0x84*/    0x0084,                 /* should not be present */
/*0x85*/    UCH_LATIN_CAPITAL_LETTER_Y_WITH_CIRCUMFLEX,
/*0x86*/    UCH_LATIN_SMALL_LETTER_Y_WITH_CIRCUMFLEX,
/*0x87*/    0x0087,                 /* should not be present */
/*0x88*/    0x0088,                 /* should not be present */
/*0x89*/    0x0089,                 /* should not be present */
/*0x8A*/    0x008A,                 /* should not be present */
/*0x8B*/    0x008B,                 /* should not be present */
/*0x8C*/    UCH_HORIZONTAL_ELLIPSIS,
/*0x8D*/    UCH_TRADE_MARK_SIGN,
/*0x8E*/    UCH_PER_MILLE_SIGN,
/*0x8F*/    UCH_BULLET,

/*0x90*/    UCH_LEFT_SINGLE_QUOTATION_MARK,
/*0x91*/    UCH_RIGHT_SINGLE_QUOTATION_MARK,
/*0x92*/    UCH_SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK,
/*0x93*/    UCH_SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK,
/*0x94*/    UCH_LEFT_DOUBLE_QUOTATION_MARK,
/*0x95*/    UCH_RIGHT_DOUBLE_QUOTATION_MARK,
/*0x96*/    UCH_DOUBLE_LOW_9_QUOTATION_MARK,
/*0x97*/    UCH_EN_DASH,
/*0x98*/    UCH_EM_DASH,
/*0x99*/    UCH_MINUS_SIGN__UNICODE,
/*0x9A*/    UCH_LATIN_CAPITAL_LIGATURE_OE,
/*0x9B*/    UCH_LATIN_SMALL_LIGATURE_OE,
/*0x9C*/    UCH_DAGGER,
/*0x9D*/    UCH_DOUBLE_DAGGER,
/*0x9E*/    UCH_LATIN_SMALL_LIGATURE_FI,
/*0x9F*/    UCH_LATIN_SMALL_LIGATURE_FL
};

_Check_return_
static UCS4
ucs4_from_sbchar_Alphabet_Latin1(
    _InVal_     SBCHAR sbchar)
{
    UCS4 ucs4 = (UCS4) sbchar;

    if(ucs4_is_C1(ucs4)) /* Acorn Extended Latin-1 extensions are all in C1 */
        ucs4 = ucs4_from_sbchar_Alphabet_Latin1_C1[ucs4 - 0x80];

    /* all other code points in 0000..00FF in Acorn Extended Latin-1 are Unicode code points */

    return(ucs4);
}

/**********************
*
* ISO 8859-2 & variants
*
**********************/

/*
ISO 8859-2 0xA0..0xFF range
*/

static const UCS4
ucs4_from_sbchar_ISO_8859_2_A0_FF[0x60] =
{
/*0xA0*/    UCH_NO_BREAK_SPACE,
/*0xA1*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_OGONEK,
/*0xA2*/    UCH_BREVE,
/*0xA3*/    UCH_LATIN_CAPITAL_LETTER_L_WITH_STROKE,
/*0xA4*/    UCH_CURRENCY_SIGN,
/*0xA5*/    UCH_LATIN_CAPITAL_LETTER_L_WITH_CARON,
/*0xA6*/    UCH_LATIN_CAPITAL_LETTER_S_WITH_ACUTE,
/*0xA7*/    UCH_SECTION_SIGN,
/*0xA8*/    UCH_DIAERESIS,
/*0xA9*/    UCH_LATIN_CAPITAL_LETTER_S_WITH_CARON,
/*0xAA*/    UCH_LATIN_CAPITAL_LETTER_S_WITH_CEDILLA,
/*0xAB*/    UCH_LATIN_CAPITAL_LETTER_T_WITH_CARON,
/*0xAC*/    UCH_LATIN_CAPITAL_LETTER_Z_WITH_ACUTE,
/*0xAD*/    UCH_SOFT_HYPHEN,
/*0xAE*/    UCH_LATIN_CAPITAL_LETTER_Z_WITH_CARON,
/*0xAF*/    UCH_LATIN_CAPITAL_LETTER_Z_WITH_DOT_ABOVE,

/*0xB0*/    UCH_DEGREE_SIGN,
/*0xB1*/    UCH_LATIN_SMALL_LETTER_A_WITH_OGONEK,
/*0xB2*/    UCH_OGONEK,
/*0xB3*/    UCH_LATIN_SMALL_LETTER_L_WITH_STROKE,
/*0xB4*/    UCH_ACUTE_ACCENT,
/*0xB5*/    UCH_LATIN_SMALL_LETTER_L_WITH_CARON,
/*0xB6*/    UCH_LATIN_SMALL_LETTER_S_WITH_ACUTE,
/*0xB7*/    UCH_CARON,
/*0xB8*/    UCH_CEDILLA,
/*0xB9*/    UCH_LATIN_SMALL_LETTER_S_WITH_CARON,
/*0xBA*/    UCH_LATIN_SMALL_LETTER_S_WITH_CEDILLA,
/*0xBB*/    UCH_LATIN_SMALL_LETTER_T_WITH_CARON,
/*0xBC*/    UCH_LATIN_SMALL_LETTER_Z_WITH_ACUTE,
/*0xBD*/    UCH_DOUBLE_ACUTE_ACCENT,
/*0xBE*/    UCH_LATIN_SMALL_LETTER_Z_WITH_CARON,
/*0xBF*/    UCH_LATIN_SMALL_LETTER_Z_WITH_DOT_ABOVE,

/*0xC0*/    UCH_LATIN_CAPITAL_LETTER_R_WITH_ACUTE,
/*0xC1*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_ACUTE,
/*0xC2*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX,
/*0xC3*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_BREVE,
/*0xC4*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_DIAERESIS,
/*0xC5*/    UCH_LATIN_CAPITAL_LETTER_L_WITH_ACUTE,
/*0xC6*/    UCH_LATIN_CAPITAL_LETTER_C_WITH_ACUTE,
/*0xC7*/    UCH_LATIN_CAPITAL_LETTER_C_WITH_CEDILLA,
/*0xC8*/    UCH_LATIN_CAPITAL_LETTER_C_WITH_CARON,
/*0xC9*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_ACUTE,
/*0xCA*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_OGONEK,
/*0xCB*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS,
/*0xCC*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_CARON,
/*0xCD*/    UCH_LATIN_CAPITAL_LETTER_I_WITH_ACUTE,
/*0xCE*/    UCH_LATIN_CAPITAL_LETTER_I_WITH_CIRCUMFLEX,
/*0xCF*/    UCH_LATIN_CAPITAL_LETTER_D_WITH_CARON,

/*0xD0*/    UCH_LATIN_CAPITAL_LETTER_D_WITH_STROKE,
/*0xD1*/    UCH_LATIN_CAPITAL_LETTER_N_WITH_ACUTE,
/*0xD2*/    UCH_LATIN_CAPITAL_LETTER_N_WITH_CARON,
/*0xD3*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_ACUTE,
/*0xD4*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_CIRCUMFLEX,
/*0xD5*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_DOUBLE_ACUTE,
/*0xD6*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_DIAERESIS,
/*0xD7*/    UCH_MULTIPLICATION_SIGN,
/*0xD8*/    UCH_LATIN_CAPITAL_LETTER_R_WITH_CARON,
/*0xD9*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_RING_ABOVE,
/*0xDA*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_ACUTE,
/*0xDB*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_DOUBLE_ACUTE,
/*0xDC*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_DIAERESIS,
/*0xDD*/    UCH_LATIN_CAPITAL_LETTER_Y_WITH_ACUTE,
/*0xDE*/    UCH_LATIN_CAPITAL_LETTER_T_WITH_CEDILLA,
/*0xDF*/    UCH_LATIN_SMALL_LETTER_SHARP_S,

/*0xE0*/    UCH_LATIN_SMALL_LETTER_R_WITH_ACUTE,
/*0xE1*/    UCH_LATIN_SMALL_LETTER_A_WITH_ACUTE,
/*0xE2*/    UCH_LATIN_SMALL_LETTER_A_WITH_CIRCUMFLEX,
/*0xE3*/    UCH_LATIN_SMALL_LETTER_A_WITH_BREVE,
/*0xE4*/    UCH_LATIN_SMALL_LETTER_A_WITH_DIAERESIS,
/*0xE5*/    UCH_LATIN_SMALL_LETTER_L_WITH_ACUTE,
/*0xE6*/    UCH_LATIN_SMALL_LETTER_C_WITH_ACUTE,
/*0xE7*/    UCH_LATIN_SMALL_LETTER_C_WITH_CEDILLA,
/*0xE8*/    UCH_LATIN_SMALL_LETTER_C_WITH_CARON,
/*0xE9*/    UCH_LATIN_SMALL_LETTER_E_WITH_ACUTE,
/*0xEA*/    UCH_LATIN_SMALL_LETTER_E_WITH_OGONEK,
/*0xEB*/    UCH_LATIN_SMALL_LETTER_E_WITH_DIAERESIS,
/*0xEC*/    UCH_LATIN_SMALL_LETTER_E_WITH_CARON,
/*0xED*/    UCH_LATIN_SMALL_LETTER_I_WITH_ACUTE,
/*0xEE*/    UCH_LATIN_SMALL_LETTER_I_WITH_CIRCUMFLEX,
/*0xEF*/    UCH_LATIN_SMALL_LETTER_D_WITH_CARON,

/*0xF0*/    UCH_LATIN_SMALL_LETTER_D_WITH_STROKE,
/*0xF1*/    UCH_LATIN_SMALL_LETTER_N_WITH_ACUTE,
/*0xF2*/    UCH_LATIN_SMALL_LETTER_N_WITH_CARON,
/*0xF3*/    UCH_LATIN_SMALL_LETTER_O_WITH_ACUTE,
/*0xF4*/    UCH_LATIN_SMALL_LETTER_O_WITH_CIRCUMFLEX,
/*0xF5*/    UCH_LATIN_SMALL_LETTER_O_WITH_DOUBLE_ACUTE,
/*0xF6*/    UCH_LATIN_SMALL_LETTER_O_WITH_DIAERESIS,
/*0xF7*/    UCH_DIVISION_SIGN,
/*0xF8*/    UCH_LATIN_SMALL_LETTER_R_WITH_CARON,
/*0xF9*/    UCH_LATIN_SMALL_LETTER_U_WITH_RING_ABOVE,
/*0xFA*/    UCH_LATIN_SMALL_LETTER_U_WITH_ACUTE,
/*0xFB*/    UCH_LATIN_SMALL_LETTER_U_WITH_DOUBLE_ACUTE,
/*0xFC*/    UCH_LATIN_SMALL_LETTER_U_WITH_DIAERESIS,
/*0xFD*/    UCH_LATIN_SMALL_LETTER_Y_WITH_ACUTE,
/*0xFE*/    UCH_LATIN_SMALL_LETTER_T_WITH_CEDILLA,
/*0xFF*/    UCH_DOT_ABOVE
};

/*
ISO 8859-2
*/

_Check_return_
static UCS4
ucs4_from_sbchar_ISO_8859_2(
    _InVal_     SBCHAR sbchar)
{
    UCS4 ucs4 = (UCS4) sbchar;

    if((ucs4 >= 0xA0U) && (ucs4 <= 0xFFU))
        ucs4 = ucs4_from_sbchar_ISO_8859_2_A0_FF[ucs4 - 0xA0U];

    /* all other code points in 0000..00FF in ISO 8859-2 are Unicode code points */

    return(ucs4);
}

/*
Windows-28592
*/

/* Windows-28592 is ISO 8859-2 */
#define ucs4_from_sbchar_Windows_28592(sbchar) ( \
    ucs4_from_sbchar_ISO_8859_2(sbchar) )

/*
Windows-1250
*/

/*
0080..009F Windows-1250 Extensions
00A0..00BF Windows-1252 Changes
*/

static const UCS4
ucs4_from_sbchar_Windows_1250_80_BF[0x40] =
{
/*0x80*/    UCH_EURO_CURRENCY_SIGN,
/*0x81*/    0x0081,                 /* should not be present */
/*0x82*/    UCH_SINGLE_LOW_9_QUOTATION_MARK,
/*0x83*/    0x0083,                 /* should not be present */
/*0x84*/    UCH_DOUBLE_LOW_9_QUOTATION_MARK,
/*0x85*/    UCH_HORIZONTAL_ELLIPSIS,
/*0x86*/    UCH_DAGGER,
/*0x87*/    UCH_DOUBLE_DAGGER,
/*0x88*/    0x0088,                 /* should not be present */
/*0x89*/    UCH_PER_MILLE_SIGN,
/*0x8A*/    UCH_LATIN_CAPITAL_LETTER_S_WITH_CARON,
/*0x8B*/    UCH_SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK,
/*0x8C*/    UCH_LATIN_CAPITAL_LETTER_S_WITH_ACUTE,
/*0x8D*/    UCH_LATIN_CAPITAL_LETTER_T_WITH_CARON,
/*0x8E*/    UCH_LATIN_CAPITAL_LETTER_Z_WITH_CARON,
/*0x8F*/    UCH_LATIN_CAPITAL_LETTER_Z_WITH_ACUTE,

/*0x90*/    0x0090,                 /* should not be present */
/*0x91*/    UCH_LEFT_SINGLE_QUOTATION_MARK,
/*0x92*/    UCH_RIGHT_SINGLE_QUOTATION_MARK,
/*0x93*/    UCH_LEFT_DOUBLE_QUOTATION_MARK,
/*0x94*/    UCH_RIGHT_DOUBLE_QUOTATION_MARK,
/*0x95*/    UCH_BULLET,
/*0x96*/    UCH_EN_DASH,
/*0x97*/    UCH_EM_DASH,
/*0x98*/    0x0098,                 /* should not be present */
/*0x99*/    UCH_TRADE_MARK_SIGN,
/*0x9A*/    UCH_LATIN_SMALL_LETTER_S_WITH_CARON,
/*0x9B*/    UCH_SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK,
/*0x9C*/    UCH_LATIN_SMALL_LETTER_S_WITH_ACUTE,
/*0x9D*/    UCH_LATIN_SMALL_LETTER_T_WITH_CARON,
/*0x9E*/    UCH_LATIN_SMALL_LETTER_Z_WITH_CARON,
/*0x9F*/    UCH_LATIN_SMALL_LETTER_Z_WITH_ACUTE,

/*0xA0*/    UCH_NO_BREAK_SPACE,
/*0xA1*/    UCH_CARON,
/*0xA2*/    UCH_BREVE,
/*0xA3*/    UCH_LATIN_CAPITAL_LETTER_L_WITH_STROKE,
/*0xA4*/    UCH_CURRENCY_SIGN,
/*0xA5*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_OGONEK,
/*0xA6*/    UCH_BROKEN_BAR,
/*0xA7*/    UCH_SECTION_SIGN,
/*0xA8*/    UCH_DIAERESIS,
/*0xA9*/    UCH_COPYRIGHT_SIGN,
/*0xAA*/    UCH_LATIN_CAPITAL_LETTER_S_WITH_CEDILLA,
/*0xAB*/    UCH_LEFT_POINTING_DOUBLE_ANGLE_QUOTATION_MARK,
/*0xAC*/    UCH_NOT_SIGN,
/*0xAD*/    UCH_SOFT_HYPHEN,
/*0xAE*/    UCH_REGISTERED_SIGN,
/*0xAF*/    UCH_LATIN_CAPITAL_LETTER_Z_WITH_DOT_ABOVE,

/*0xB0*/    UCH_DEGREE_SIGN,
/*0xB1*/    UCH_PLUS_MINUS_SIGN,
/*0xB2*/    UCH_OGONEK,
/*0xB3*/    UCH_LATIN_SMALL_LETTER_L_WITH_STROKE,
/*0xB4*/    UCH_ACUTE_ACCENT,
/*0xB5*/    UCH_MICRO_SIGN,
/*0xB6*/    UCH_PILCROW_SIGN,
/*0xB7*/    UCH_MIDDLE_DOT,
/*0xB8*/    UCH_CEDILLA,
/*0xB9*/    UCH_LATIN_SMALL_LETTER_A_WITH_OGONEK,
/*0xBA*/    UCH_LATIN_SMALL_LETTER_S_WITH_CEDILLA,
/*0xBB*/    UCH_RIGHT_POINTING_DOUBLE_ANGLE_QUOTATION_MARK,
/*0xBC*/    UCH_LATIN_CAPITAL_LETTER_L_WITH_CARON,
/*0xBD*/    UCH_DOUBLE_ACUTE_ACCENT,
/*0xBE*/    UCH_LATIN_SMALL_LETTER_L_WITH_CARON,
/*0xBF*/    UCH_LATIN_SMALL_LETTER_Z_WITH_DOT_ABOVE
};

_Check_return_
static UCS4
ucs4_from_sbchar_Windows_1250(
    _InVal_     SBCHAR sbchar)
{
    UCS4 ucs4 = (UCS4) sbchar;

    if((ucs4 >= 0x80U) && (ucs4 <= 0xBFU)) /* Windows-1250 extensions spill upwards out of C1 */
        ucs4 = ucs4_from_sbchar_Windows_1250_80_BF[ucs4 - 0x80U];
    else if((ucs4 >= 0xA0U) && (ucs4 <= 0xFFU))
        ucs4 = ucs4_from_sbchar_ISO_8859_2_A0_FF[ucs4 - 0xA0U];

    /* all other code points in 0000..00FF in Windows-1250 are Unicode code points */

    return(ucs4);
}

/*
Acorn Extended Latin-2
*/

static const UCS4
ucs4_from_sbchar_Alphabet_Latin2_C1[0x20] =
{           /* Acorn Extended Latin-2 extensions are all in C1 */
/*0x80*/    UCH_EURO_CURRENCY_SIGN,
/*0x81*/    0x0081,                 /* should not be present */
/*0x82*/    0x0082,                 /* should not be present */
/*0x83*/    0x0083,                 /* should not be present */
/*0x84*/    0x0084,                 /* should not be present */
/*0x85*/    0x0085,                 /* should not be present */
/*0x86*/    0x0086,                 /* should not be present */
/*0x87*/    0x0087,                 /* should not be present */
/*0x88*/    0x0088,                 /* should not be present */
/*0x89*/    0x0089,                 /* should not be present */
/*0x8A*/    0x008A,                 /* should not be present */
/*0x8B*/    0x008B,                 /* should not be present */
/*0x8C*/    UCH_HORIZONTAL_ELLIPSIS,
/*0x8D*/    UCH_TRADE_MARK_SIGN,
/*0x8E*/    UCH_PER_MILLE_SIGN,
/*0x8F*/    UCH_BULLET,

/*0x90*/    UCH_LEFT_SINGLE_QUOTATION_MARK,
/*0x91*/    UCH_RIGHT_SINGLE_QUOTATION_MARK,
/*0x92*/    UCH_SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK,
/*0x93*/    UCH_SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK,
/*0x94*/    UCH_LEFT_DOUBLE_QUOTATION_MARK,
/*0x95*/    UCH_RIGHT_DOUBLE_QUOTATION_MARK,
/*0x96*/    UCH_DOUBLE_LOW_9_QUOTATION_MARK,
/*0x97*/    UCH_EN_DASH,
/*0x98*/    UCH_EM_DASH,
/*0x99*/    UCH_MINUS_SIGN__UNICODE,
/*0x9A*/    UCH_LATIN_CAPITAL_LIGATURE_OE,
/*0x9B*/    UCH_LATIN_SMALL_LIGATURE_OE,
/*0x9C*/    UCH_DAGGER,
/*0x9D*/    UCH_DOUBLE_DAGGER,
/*0x9E*/    UCH_LATIN_SMALL_LIGATURE_FI,
/*0x9F*/    UCH_LATIN_SMALL_LIGATURE_FL
};

_Check_return_
static UCS4
ucs4_from_sbchar_Alphabet_Latin2(
    _InVal_     SBCHAR sbchar)
{
    UCS4 ucs4 = (UCS4) sbchar;

    if(ucs4_is_C1(ucs4)) /* Acorn Extended Latin-2 extensions are all in C1 */
        ucs4 = ucs4_from_sbchar_Alphabet_Latin2_C1[ucs4 - 0x80U];
    else if((ucs4 >= 0xA0U) && (ucs4 <= 0xFFU))
        ucs4 = ucs4_from_sbchar_ISO_8859_2_A0_FF[ucs4 - 0xA0U];

    /* all other code points in 0000..00FF in Acorn Extended Latin-2 are Unicode code points */

    return(ucs4);
}

/**********************
*
* ISO 8859-3 & variants
*
**********************/

/*
ISO 8859-3 0xA0..0xFF range
*/

static const UCS4
ucs4_from_sbchar_ISO_8859_3_A0_FF[0x60] =
{
/*0xA0*/    UCH_NO_BREAK_SPACE,
/*0xA1*/    UCH_LATIN_CAPITAL_LETTER_H_WITH_STROKE,
/*0xA2*/    UCH_BREVE,
/*0xA3*/    UCH_POUND_SIGN,
/*0xA4*/    UCH_CURRENCY_SIGN,
/*0xA5*/    0x00A5,                 /* should not be present */
/*0xA6*/    UCH_LATIN_CAPITAL_LETTER_H_WITH_CIRCUMFLEX,
/*0xA7*/    UCH_SECTION_SIGN,
/*0xA8*/    UCH_DIAERESIS,
/*0xA9*/    UCH_LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE,
/*0xAA*/    UCH_LATIN_CAPITAL_LETTER_S_WITH_CEDILLA,
/*0xAB*/    UCH_LATIN_CAPITAL_LETTER_G_WITH_BREVE,
/*0xAC*/    UCH_LATIN_CAPITAL_LETTER_J_WITH_CIRCUMFLEX,
/*0xAD*/    UCH_SOFT_HYPHEN,
/*0xAE*/    0x00AE,                 /* should not be present */
/*0xAF*/    UCH_LATIN_CAPITAL_LETTER_Z_WITH_DOT_ABOVE,

/*0xB0*/    UCH_DEGREE_SIGN,
/*0xB1*/    UCH_LATIN_SMALL_LETTER_H_WITH_STROKE,
/*0xB2*/    UCH_SUPERSCRIPT_TWO,
/*0xB3*/    UCH_SUPERSCRIPT_THREE,
/*0xB4*/    UCH_ACUTE_ACCENT,
/*0xB5*/    UCH_MICRO_SIGN,
/*0xB6*/    UCH_LATIN_SMALL_LETTER_H_WITH_CIRCUMFLEX,
/*0xB7*/    UCH_MIDDLE_DOT,
/*0xB8*/    UCH_CEDILLA,
/*0xB9*/    UCH_LATIN_SMALL_LETTER_DOTLESS_I,
/*0xBA*/    UCH_LATIN_SMALL_LETTER_S_WITH_CEDILLA,
/*0xBB*/    UCH_LATIN_SMALL_LETTER_G_WITH_BREVE,
/*0xBC*/    UCH_LATIN_SMALL_LETTER_J_WITH_CIRCUMFLEX,
/*0xBD*/    UCH_VULGAR_FRACTION_ONE_HALF,
/*0xBE*/    0x00BE,                 /* should not be present */
/*0xBF*/    UCH_LATIN_SMALL_LETTER_Z_WITH_DOT_ABOVE,

/*0xC0*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_GRAVE,
/*0xC1*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_ACUTE,
/*0xC2*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX,
/*0xC3*/    0x00C3,                 /* should not be present */
/*0xC4*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_DIAERESIS,
/*0xC5*/    UCH_LATIN_CAPITAL_LETTER_C_WITH_DOT_ABOVE,
/*0xC6*/    UCH_LATIN_CAPITAL_LETTER_C_WITH_CIRCUMFLEX,
/*0xC7*/    UCH_LATIN_CAPITAL_LETTER_C_WITH_CEDILLA,
/*0xC8*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_GRAVE,
/*0xC9*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_ACUTE,
/*0xCA*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_CIRCUMFLEX,
/*0xCB*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS,
/*0xCC*/    UCH_LATIN_CAPITAL_LETTER_I_WITH_GRAVE,
/*0xCD*/    UCH_LATIN_CAPITAL_LETTER_I_WITH_ACUTE,
/*0xCE*/    UCH_LATIN_CAPITAL_LETTER_I_WITH_CIRCUMFLEX,
/*0xCF*/    UCH_LATIN_CAPITAL_LETTER_I_WITH_DIAERESIS,

/*0xD0*/    0x00D0,                 /* should not be present */
/*0xD1*/    UCH_LATIN_CAPITAL_LETTER_N_WITH_TILDE,
/*0xD2*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_GRAVE,
/*0xD3*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_ACUTE,
/*0xD4*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_CIRCUMFLEX,
/*0xD5*/    UCH_LATIN_CAPITAL_LETTER_G_WITH_DOT_ABOVE,
/*0xD6*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_DIAERESIS,
/*0xD7*/    UCH_MULTIPLICATION_SIGN,
/*0xD8*/    UCH_LATIN_CAPITAL_LETTER_G_WITH_CIRCUMFLEX,
/*0xD9*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_GRAVE,
/*0xDA*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_ACUTE,
/*0xDB*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_CIRCUMFLEX,
/*0xDC*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_DIAERESIS,
/*0xDD*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_BREVE,
/*0xDE*/    UCH_LATIN_CAPITAL_LETTER_S_WITH_CIRCUMFLEX,
/*0xDF*/    UCH_LATIN_SMALL_LETTER_SHARP_S,

/*0xE0*/    UCH_LATIN_SMALL_LETTER_A_WITH_GRAVE,
/*0xE1*/    UCH_LATIN_SMALL_LETTER_A_WITH_ACUTE,
/*0xE2*/    UCH_LATIN_SMALL_LETTER_A_WITH_CIRCUMFLEX,
/*0xE3*/    0x00E3,                 /* should not be present */
/*0xE4*/    UCH_LATIN_SMALL_LETTER_A_WITH_DIAERESIS,
/*0xE5*/    UCH_LATIN_SMALL_LETTER_C_WITH_DOT_ABOVE,
/*0xE6*/    UCH_LATIN_SMALL_LETTER_C_WITH_CIRCUMFLEX,
/*0xE7*/    UCH_LATIN_SMALL_LETTER_C_WITH_CEDILLA,
/*0xE8*/    UCH_LATIN_SMALL_LETTER_E_WITH_GRAVE,
/*0xE9*/    UCH_LATIN_SMALL_LETTER_E_WITH_ACUTE,
/*0xEA*/    UCH_LATIN_SMALL_LETTER_E_WITH_CIRCUMFLEX,
/*0xEB*/    UCH_LATIN_SMALL_LETTER_E_WITH_DIAERESIS,
/*0xEC*/    UCH_LATIN_SMALL_LETTER_I_WITH_GRAVE,
/*0xED*/    UCH_LATIN_SMALL_LETTER_I_WITH_ACUTE,
/*0xEE*/    UCH_LATIN_SMALL_LETTER_I_WITH_CIRCUMFLEX,
/*0xEF*/    UCH_LATIN_SMALL_LETTER_I_WITH_DIAERESIS,

/*0xF0*/    0x00F0,                 /* should not be present */
/*0xF1*/    UCH_LATIN_SMALL_LETTER_N_WITH_TILDE,
/*0xF2*/    UCH_LATIN_SMALL_LETTER_O_WITH_GRAVE,
/*0xF3*/    UCH_LATIN_SMALL_LETTER_O_WITH_ACUTE,
/*0xF4*/    UCH_LATIN_SMALL_LETTER_O_WITH_CIRCUMFLEX,
/*0xF5*/    UCH_LATIN_SMALL_LETTER_G_WITH_DOT_ABOVE,
/*0xF6*/    UCH_LATIN_SMALL_LETTER_O_WITH_DIAERESIS,
/*0xF7*/    UCH_DIVISION_SIGN,
/*0xF8*/    UCH_LATIN_SMALL_LETTER_G_WITH_CIRCUMFLEX,
/*0xF9*/    UCH_LATIN_SMALL_LETTER_U_WITH_GRAVE,
/*0xFA*/    UCH_LATIN_SMALL_LETTER_U_WITH_ACUTE,
/*0xFB*/    UCH_LATIN_SMALL_LETTER_U_WITH_CIRCUMFLEX,
/*0xFC*/    UCH_LATIN_SMALL_LETTER_U_WITH_DIAERESIS,
/*0xFD*/    UCH_LATIN_SMALL_LETTER_U_WITH_BREVE,
/*0xFE*/    UCH_LATIN_SMALL_LETTER_S_WITH_CIRCUMFLEX,
/*0xFF*/    UCH_DOT_ABOVE
};

/*
ISO 8859-3
*/

_Check_return_
static UCS4
ucs4_from_sbchar_ISO_8859_3(
    _InVal_     SBCHAR sbchar)
{
    UCS4 ucs4 = (UCS4) sbchar;

    if((ucs4 >= 0xA0U) && (ucs4 <= 0xFFU))
        ucs4 = ucs4_from_sbchar_ISO_8859_3_A0_FF[ucs4 - 0xA0U];

    /* all other code points in 0000..00FF in ISO 8859-3 are Unicode code points */

    return(ucs4);
}

/*
Windows-28593
*/

/* Windows-28593 is ISO 8859-3 */
#define ucs4_from_sbchar_Windows_28593(sbchar) ( \
    ucs4_from_sbchar_ISO_8859_3(sbchar) )


/*
Acorn Extended Latin-3
*/

_Check_return_
static UCS4
ucs4_from_sbchar_Alphabet_Latin3(
    _InVal_     SBCHAR sbchar)
{
    UCS4 ucs4 = (UCS4) sbchar;

    if(ucs4_is_C1(ucs4)) /* Acorn Extended Latin-3 extensions are all in C1 */
        ucs4 = ucs4_from_sbchar_Alphabet_Latin2_C1[ucs4 - 0x80U]; /* and are the same as Acorn Extended Latin-2 */
    else if((ucs4 >= 0xA0U) && (ucs4 <= 0xFFU))
        ucs4 = ucs4_from_sbchar_ISO_8859_3_A0_FF[ucs4 - 0xA0U];

    /* all other code points in 0000..00FF in Acorn Extended Latin-3 are Unicode code points */

    return(ucs4);
}

/**********************
*
* ISO 8859-4 & variants
*
**********************/

/*
ISO 8859-4 0xA0..0xFF range
*/

static const UCS4
ucs4_from_sbchar_ISO_8859_4_A0_FF[0x60] =
{
/*0xA0*/    UCH_NO_BREAK_SPACE,
/*0xA1*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_OGONEK,
/*0xA2*/    UCH_LATIN_SMALL_LETTER_KRA,
/*0xA3*/    UCH_LATIN_CAPITAL_LETTER_R_WITH_CEDILLA,
/*0xA4*/    UCH_CURRENCY_SIGN,
/*0xA5*/    UCH_LATIN_CAPITAL_LETTER_I_WITH_TILDE,
/*0xA6*/    UCH_LATIN_CAPITAL_LETTER_L_WITH_CEDILLA,
/*0xA7*/    UCH_SECTION_SIGN,
/*0xA8*/    UCH_DIAERESIS,
/*0xA9*/    UCH_LATIN_CAPITAL_LETTER_S_WITH_CARON,
/*0xAA*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_MACRON,
/*0xAB*/    UCH_LATIN_CAPITAL_LETTER_G_WITH_CEDILLA,
/*0xAC*/    UCH_LATIN_CAPITAL_LETTER_T_WITH_STROKE,
/*0xAD*/    UCH_SOFT_HYPHEN,
/*0xAE*/    UCH_LATIN_CAPITAL_LETTER_Z_WITH_CARON,
/*0xAF*/    UCH_MACRON,

/*0xB0*/    UCH_DEGREE_SIGN,
/*0xB1*/    UCH_LATIN_SMALL_LETTER_A_WITH_OGONEK,
/*0xB2*/    UCH_OGONEK,
/*0xB3*/    UCH_LATIN_SMALL_LETTER_R_WITH_CEDILLA,
/*0xB4*/    UCH_ACUTE_ACCENT,
/*0xB5*/    UCH_LATIN_SMALL_LETTER_I_WITH_TILDE,
/*0xB6*/    UCH_LATIN_SMALL_LETTER_L_WITH_CEDILLA,
/*0xB7*/    UCH_CARON,
/*0xB8*/    UCH_CEDILLA,
/*0xB9*/    UCH_LATIN_SMALL_LETTER_S_WITH_CARON,
/*0xBA*/    UCH_LATIN_SMALL_LETTER_E_WITH_MACRON,
/*0xBB*/    UCH_LATIN_SMALL_LETTER_G_WITH_CEDILLA,
/*0xBC*/    UCH_LATIN_SMALL_LETTER_T_WITH_STROKE,
/*0xBD*/    UCH_LATIN_CAPITAL_LETTER_ENG,
/*0xBE*/    UCH_LATIN_SMALL_LETTER_Z_WITH_CARON,
/*0xBF*/    UCH_LATIN_SMALL_LETTER_ENG,

/*0xC0*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_MACRON,
/*0xC1*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_ACUTE,
/*0xC2*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX,
/*0xC3*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_TILDE,
/*0xC4*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_DIAERESIS,
/*0xC5*/    UCH_LATIN_CAPITAL_LETTER_A_WITH_RING_ABOVE,
/*0xC6*/    UCH_LATIN_CAPITAL_LETTER_AE,
/*0xC7*/    UCH_LATIN_CAPITAL_LETTER_I_WITH_OGONEK,
/*0xC8*/    UCH_LATIN_CAPITAL_LETTER_C_WITH_CARON,
/*0xC9*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_ACUTE,
/*0xCA*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_OGONEK,
/*0xCB*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS,
/*0xCC*/    UCH_LATIN_CAPITAL_LETTER_E_WITH_DOT_ABOVE,
/*0xCD*/    UCH_LATIN_CAPITAL_LETTER_I_WITH_ACUTE,
/*0xCE*/    UCH_LATIN_CAPITAL_LETTER_I_WITH_CIRCUMFLEX,
/*0xCF*/    UCH_LATIN_CAPITAL_LETTER_I_WITH_MACRON,

/*0xD0*/    UCH_LATIN_CAPITAL_LETTER_D_WITH_STROKE,
/*0xD1*/    UCH_LATIN_CAPITAL_LETTER_N_WITH_CEDILLA,
/*0xD2*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_MACRON,
/*0xD3*/    UCH_LATIN_CAPITAL_LETTER_K_WITH_CEDILLA,
/*0xD4*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_CIRCUMFLEX,
/*0xD5*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_TILDE,
/*0xD6*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_DIAERESIS,
/*0xD7*/    UCH_MULTIPLICATION_SIGN,
/*0xD8*/    UCH_LATIN_CAPITAL_LETTER_O_WITH_STROKE,
/*0xD9*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_OGONEK,
/*0xDA*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_ACUTE,
/*0xDB*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_CIRCUMFLEX,
/*0xDC*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_DIAERESIS,
/*0xDD*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_TILDE,
/*0xDE*/    UCH_LATIN_CAPITAL_LETTER_U_WITH_MACRON,
/*0xDF*/    UCH_LATIN_SMALL_LETTER_SHARP_S,

/*0xE0*/    UCH_LATIN_SMALL_LETTER_A_WITH_MACRON,
/*0xE1*/    UCH_LATIN_SMALL_LETTER_A_WITH_ACUTE,
/*0xE2*/    UCH_LATIN_SMALL_LETTER_A_WITH_CIRCUMFLEX,
/*0xE3*/    UCH_LATIN_SMALL_LETTER_A_WITH_TILDE,
/*0xE4*/    UCH_LATIN_SMALL_LETTER_A_WITH_DIAERESIS,
/*0xE5*/    UCH_LATIN_SMALL_LETTER_A_WITH_RING_ABOVE,
/*0xE6*/    UCH_LATIN_SMALL_LETTER_AE,
/*0xE7*/    UCH_LATIN_SMALL_LETTER_I_WITH_OGONEK,
/*0xE8*/    UCH_LATIN_SMALL_LETTER_C_WITH_CARON,
/*0xE9*/    UCH_LATIN_SMALL_LETTER_E_WITH_ACUTE,
/*0xEA*/    UCH_LATIN_SMALL_LETTER_E_WITH_OGONEK,
/*0xEB*/    UCH_LATIN_SMALL_LETTER_E_WITH_DIAERESIS,
/*0xEC*/    UCH_LATIN_SMALL_LETTER_E_WITH_DOT_ABOVE,
/*0xED*/    UCH_LATIN_SMALL_LETTER_I_WITH_ACUTE,
/*0xEE*/    UCH_LATIN_SMALL_LETTER_I_WITH_CIRCUMFLEX,
/*0xEF*/    UCH_LATIN_SMALL_LETTER_I_WITH_MACRON,

/*0xF0*/    UCH_LATIN_SMALL_LETTER_D_WITH_STROKE,
/*0xF1*/    UCH_LATIN_SMALL_LETTER_N_WITH_CEDILLA,
/*0xF2*/    UCH_LATIN_SMALL_LETTER_O_WITH_MACRON,
/*0xF3*/    UCH_LATIN_SMALL_LETTER_K_WITH_CEDILLA,
/*0xF4*/    UCH_LATIN_SMALL_LETTER_O_WITH_CIRCUMFLEX,
/*0xF5*/    UCH_LATIN_SMALL_LETTER_O_WITH_TILDE,
/*0xF6*/    UCH_LATIN_SMALL_LETTER_O_WITH_DIAERESIS,
/*0xF7*/    UCH_DIVISION_SIGN,
/*0xF8*/    UCH_LATIN_SMALL_LETTER_O_WITH_STROKE,
/*0xF9*/    UCH_LATIN_SMALL_LETTER_U_WITH_OGONEK,
/*0xFA*/    UCH_LATIN_SMALL_LETTER_U_WITH_ACUTE,
/*0xFB*/    UCH_LATIN_SMALL_LETTER_U_WITH_CIRCUMFLEX,
/*0xFC*/    UCH_LATIN_SMALL_LETTER_U_WITH_DIAERESIS,
/*0xFD*/    UCH_LATIN_SMALL_LETTER_U_WITH_TILDE,
/*0xFE*/    UCH_LATIN_SMALL_LETTER_U_WITH_MACRON,
/*0xFF*/    UCH_DOT_ABOVE
};

/*
ISO 8859-4
*/

_Check_return_
static UCS4
ucs4_from_sbchar_ISO_8859_4(
    _InVal_     SBCHAR sbchar)
{
    UCS4 ucs4 = (UCS4) sbchar;

    if((ucs4 >= 0xA0U) && (ucs4 <= 0xFFU))
        ucs4 = ucs4_from_sbchar_ISO_8859_4_A0_FF[ucs4 - 0xA0U];

    /* all other code points in 0000..00FF in ISO 8859-4 are Unicode code points */

    return(ucs4);
}

/*
Windows-28594
*/

/* Windows-28594 is ISO 8859-4 */
#define ucs4_from_sbchar_Windows_28594(sbchar) ( \
    ucs4_from_sbchar_ISO_8859_4(sbchar) )

/*
Acorn Extended Latin-4
*/

_Check_return_
static UCS4
ucs4_from_sbchar_Alphabet_Latin4(
    _InVal_     SBCHAR sbchar)
{
    UCS4 ucs4 = (UCS4) sbchar;

    if(ucs4_is_C1(ucs4)) /* Acorn Extended Latin-3 extensions are all in C1 */
        ucs4 = ucs4_from_sbchar_Alphabet_Latin2_C1[ucs4 - 0x80U]; /* and are the same as Acorn Extended Latin-2 */
    else if((ucs4 >= 0xA0U) && (ucs4 <= 0xFFU))
        ucs4 = ucs4_from_sbchar_ISO_8859_4_A0_FF[ucs4 - 0xA0U];

    /* all other code points in 0000..00FF in Acorn Extended Latin-4 are Unicode code points */

    return(ucs4);
}

/***********************
*
* ISO 8859-15 & variants
*
***********************/

/*
ISO 8859-15
*/

_Check_return_
static UCS4
ucs4_from_sbchar_ISO_8859_15(
    _InVal_     SBCHAR sbchar)
{
    UCS4 ucs4 = (UCS4) sbchar;

    switch(ucs4)
    {
    /* 00A0..00FF ISO 8859-15 */

    case 0xA4:
        ucs4 = UCH_EURO_CURRENCY_SIGN;
        break;

    case 0xA6:
        ucs4 = UCH_LATIN_CAPITAL_LETTER_S_WITH_CARON;
        break;

    case 0xA8:
        ucs4 = UCH_LATIN_SMALL_LETTER_S_WITH_CARON;
        break;

    case 0xB4:
        ucs4 = UCH_LATIN_CAPITAL_LETTER_Z_WITH_CARON;
        break;

    case 0xB8:
        ucs4 = UCH_LATIN_SMALL_LETTER_Z_WITH_CARON;
        break;

    case 0xBC:
        ucs4 = UCH_LATIN_CAPITAL_LIGATURE_OE;
        break;

    case 0xBD:
        ucs4 = UCH_LATIN_SMALL_LIGATURE_OE;
        break;

    case 0xBE:
        ucs4 = UCH_LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS;
        break;

    default:
        /* all other code points in 0000..00FF in ISO 8859-15 are Unicode code points */
        break;
    }

    return(ucs4);
}

/*
Windows-28605
*/

/* Windows-28605 is ISO 8859-15 */
#define ucs4_from_sbchar_Windows_28605(sbchar) ( \
    ucs4_from_sbchar_ISO_8859_15(sbchar) )

/*
Acorn Extended Latin-9
*/

static const UCS4
ucs4_from_sbchar_Alphabet_Latin9_C1[0x20] =
{           /* Acorn Extended Latin-9 extensions are all in C1 */
/*0x80*/    0x0080,                 /* should not be present */
/*0x81*/    UCH_LATIN_CAPITAL_LETTER_W_WITH_CIRCUMFLEX,
/*0x82*/    UCH_LATIN_SMALL_LETTER_W_WITH_CIRCUMFLEX,
/*0x83*/    0x0083,                 /* should not be present */
/*0x84*/    0x0084,                 /* should not be present */
/*0x85*/    UCH_LATIN_CAPITAL_LETTER_Y_WITH_CIRCUMFLEX,
/*0x86*/    UCH_LATIN_SMALL_LETTER_Y_WITH_CIRCUMFLEX,
/*0x87*/    0x0087,                 /* should not be present */
/*0x88*/    0x0088,                 /* should not be present */
/*0x89*/    0x0089,                 /* should not be present */
/*0x8A*/    0x008A,                 /* should not be present */
/*0x8B*/    0x008B,                 /* should not be present */
/*0x8C*/    UCH_HORIZONTAL_ELLIPSIS,
/*0x8D*/    UCH_TRADE_MARK_SIGN,
/*0x8E*/    UCH_PER_MILLE_SIGN,
/*0x8F*/    UCH_BULLET,

/*0x90*/    UCH_LEFT_SINGLE_QUOTATION_MARK,
/*0x91*/    UCH_RIGHT_SINGLE_QUOTATION_MARK,
/*0x92*/    UCH_SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK,
/*0x93*/    UCH_SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK,
/*0x94*/    UCH_LEFT_DOUBLE_QUOTATION_MARK,
/*0x95*/    UCH_RIGHT_DOUBLE_QUOTATION_MARK,
/*0x96*/    UCH_DOUBLE_LOW_9_QUOTATION_MARK,
/*0x97*/    UCH_EN_DASH,
/*0x98*/    UCH_EM_DASH,
/*0x99*/    UCH_MINUS_SIGN__UNICODE,
/*0x9A*/    UCH_LATIN_CAPITAL_LIGATURE_OE,
/*0x9B*/    UCH_LATIN_SMALL_LIGATURE_OE,
/*0x9C*/    UCH_DAGGER,
/*0x9D*/    UCH_DOUBLE_DAGGER,
/*0x9E*/    UCH_LATIN_SMALL_LIGATURE_FI,
/*0x9F*/    UCH_LATIN_SMALL_LIGATURE_FL
};

_Check_return_
static UCS4
ucs4_from_sbchar_Alphabet_Latin9(
    _InVal_     SBCHAR sbchar)
{
    UCS4 ucs4 = (UCS4) sbchar;

    switch(ucs4)
    {
    /* 00A0..00FF ISO 8859-15 */

    case 0xA4:
        ucs4 = UCH_EURO_CURRENCY_SIGN;
        break;

    case 0xA6:
        ucs4 = UCH_LATIN_CAPITAL_LETTER_S_WITH_CARON;
        break;

    case 0xA8:
        ucs4 = UCH_LATIN_SMALL_LETTER_S_WITH_CARON;
        break;

    case 0xB4:
        ucs4 = UCH_LATIN_CAPITAL_LETTER_Z_WITH_CARON;
        break;

    case 0xB8:
        ucs4 = UCH_LATIN_SMALL_LETTER_Z_WITH_CARON;
        break;

    case 0xBC:
        ucs4 = UCH_LATIN_CAPITAL_LIGATURE_OE;
        break;

    case 0xBD:
        ucs4 = UCH_LATIN_SMALL_LIGATURE_OE;
        break;

    case 0xBE:
        ucs4 = UCH_LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS;
        break;

    default:
        if(ucs4_is_C1(ucs4)) /* Acorn Extended Latin-9 extensions are all in C1 */
            ucs4 = ucs4_from_sbchar_Alphabet_Latin9_C1[ucs4 - 0x80];
        /* all other code points in 0000..00FF in Acorn Extended Latin-9 are Unicode code points */
        break;
    }

    return(ucs4);
}

_Check_return_
extern UCS4
ucs4_from_sbchar_with_codepage(
    _InVal_     SBCHAR sbchar,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage)
{
    switch(sbchar_codepage)
    {
#if WINDOWS
    default: default_unhandled();
#endif
    case SBCHAR_CODEPAGE_WINDOWS_1252:
        return(ucs4_from_sbchar_Windows_1252(sbchar));

    case SBCHAR_CODEPAGE_WINDOWS_1250:
        return(ucs4_from_sbchar_Windows_1250(sbchar));

    case SBCHAR_CODEPAGE_WINDOWS_28591:
        return(ucs4_from_sbchar_Windows_28591(sbchar));

    case SBCHAR_CODEPAGE_WINDOWS_28592:
        return(ucs4_from_sbchar_Windows_28592(sbchar));

    case SBCHAR_CODEPAGE_WINDOWS_28593:
        return(ucs4_from_sbchar_Windows_28593(sbchar));

    case SBCHAR_CODEPAGE_WINDOWS_28594:
        return(ucs4_from_sbchar_Windows_28594(sbchar));

    case SBCHAR_CODEPAGE_WINDOWS_28605:
        return(ucs4_from_sbchar_Windows_28605(sbchar));

#if RISCOS
    default: default_unhandled();
#endif
    case SBCHAR_CODEPAGE_ALPHABET_LATIN1:
        return(ucs4_from_sbchar_Alphabet_Latin1(sbchar));

    case SBCHAR_CODEPAGE_ALPHABET_LATIN2:
        return(ucs4_from_sbchar_Alphabet_Latin2(sbchar));

    case SBCHAR_CODEPAGE_ALPHABET_LATIN3:
        return(ucs4_from_sbchar_Alphabet_Latin3(sbchar));

    case SBCHAR_CODEPAGE_ALPHABET_LATIN4:
        return(ucs4_from_sbchar_Alphabet_Latin4(sbchar));

    case SBCHAR_CODEPAGE_ALPHABET_LATIN9:
        return(ucs4_from_sbchar_Alphabet_Latin9(sbchar));
    }
}

/********************************
*
* conversion from UCS-4 to SBCHAR
*
********************************/

_Check_return_
extern UCS4
ucs4_to_sbchar_force_with_codepage(
    _InVal_     UCS4 ucs4_in,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage,
    _InVal_     UCS4 ucs4_default)
{
    UCS4 ucs4 = ucs4_in;

    if(ucs4_is_ascii7(ucs4))
        return(ucs4);

    /* try mapping */
    ucs4 = ucs4_to_sbchar_try_with_codepage(ucs4, sbchar_codepage);

    if(ucs4_is_sbchar(ucs4))
        return(ucs4);

    /* substitute */
    /*assert(ucs4_is_sbchar(ucs4));*/
    return(ucs4_default);
}

/**********************
*
* ISO 8859-1 & variants
*
**********************/

/*
ISO 8859-1
*/

_Check_return_
static UCS4
ucs4_to_sbchar_try_with_ISO_8859_1(
    _InVal_     UCS4 ucs4_in)
{
    UCS4 ucs4 = ucs4_in;

    /* nothing else can be mapped down; all other Unicode code points in 0000..00FF are ISO 8859-1 code points */

    return(ucs4);
}

/*
Windows-28591
*/

/* Windows-28591 is ISO 8859-1 */
#define ucs4_to_sbchar_try_with_Windows_28591(ucs4_in) ( \
    ucs4_to_sbchar_try_with_ISO_8859_1(ucs4_in) )

/*
Windows-1252
*/

_Check_return_
static UCS4
ucs4_to_sbchar_try_with_Windows_1252(
    _InVal_     UCS4 ucs4_in)
{
    UCS4 ucs4 = ucs4_in;

    switch(ucs4)
    {
    /* Unicode code points which map down into 0080..009F in Windows-1252 Extensions */

    case UCH_EURO_CURRENCY_SIGN:
        ucs4 = 0x80;
        break;
        /* nothing maps to 0x81 */
    case UCH_SINGLE_LOW_9_QUOTATION_MARK:
        ucs4 = 0x82;
        break;
    case UCH_LATIN_SMALL_LETTER_F_WITH_HOOK:
        ucs4 = 0x83;
        break;
    case UCH_DOUBLE_LOW_9_QUOTATION_MARK:
        ucs4 = 0x84;
        break;
    case UCH_HORIZONTAL_ELLIPSIS:
        ucs4 = 0x85;
        break;
    case UCH_DAGGER:
        ucs4 = 0x86;
        break;
    case UCH_DOUBLE_DAGGER:
        ucs4 = 0x87;
        break;
    case UCH_MODIFIER_LETTER_CIRCUMFLEX_ACCENT:
        ucs4 = 0x88;
        break;
    case UCH_PER_MILLE_SIGN:
        ucs4 = 0x89;
        break;
    case UCH_LATIN_CAPITAL_LETTER_S_WITH_CARON:
        ucs4 = 0x8A;
        break;
    case UCH_SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK:
        ucs4 = 0x8B;
        break;
    case UCH_LATIN_CAPITAL_LIGATURE_OE:
        ucs4 = 0x8C;
        break;
        /* nothing maps to 0x8D */
    case UCH_LATIN_CAPITAL_LETTER_Z_WITH_CARON:
        ucs4 = 0x8E;
        break;
        /* nothing maps to 0x8F */

        /* nothing maps to 0x90 */
    case UCH_LEFT_SINGLE_QUOTATION_MARK:
        ucs4 = 0x91;
        break;
    case UCH_RIGHT_SINGLE_QUOTATION_MARK:
        ucs4 = 0x92;
        break;
    case UCH_LEFT_DOUBLE_QUOTATION_MARK:
        ucs4 = 0x93;
        break;
    case UCH_RIGHT_DOUBLE_QUOTATION_MARK:
        ucs4 = 0x94;
        break;
    case UCH_BULLET:
        ucs4 = 0x95;
        break;
    case UCH_EN_DASH:
        ucs4 = 0x96;
        break;
    case UCH_EM_DASH:
        ucs4 = 0x97;
        break;
    case UCH_SMALL_TILDE:
        ucs4 = 0x98;
        break;
    case UCH_TRADE_MARK_SIGN:
        ucs4 = 0x99;
        break;
    case UCH_LATIN_SMALL_LETTER_S_WITH_CARON:
        ucs4 = 0x9A;
        break;
    case UCH_SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK:
        ucs4 = 0x9B;
        break;
    case UCH_LATIN_SMALL_LIGATURE_OE:
        ucs4 = 0x9C;
        break;
        /* nothing maps to 0x9D */
    case UCH_LATIN_SMALL_LETTER_Z_WITH_CARON:
        ucs4 = 0x9E;
        break;
    case UCH_LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS:
        ucs4 = 0x9F;
        break;

    default:
        /* nothing else can be mapped down; all other Unicode code points in 0000..00FF are ISO 8859-1 code points */
        break;
    }

    return(ucs4);
}

/*
Acorn Extended Latin-1
*/

_Check_return_
static UCS4
ucs4_to_sbchar_try_with_Alphabet_Latin1(
    _InVal_     UCS4 ucs4_in)
{
    UCS4 ucs4 = ucs4_in;

    switch(ucs4)
    {
    /* Unicode code points which map down into 0080..009F in Acorn Extended Latin-1 */

    case UCH_EURO_CURRENCY_SIGN:
        ucs4 = 0x80;
        break;
    case UCH_LATIN_CAPITAL_LETTER_W_WITH_CIRCUMFLEX:
        ucs4 = 0x81;
        break;
    case UCH_LATIN_SMALL_LETTER_W_WITH_CIRCUMFLEX:
        ucs4 = 0x82;
        break;
        /* nothing maps to 0x83 */
        /* nothing maps to 0x84 */
    case UCH_LATIN_CAPITAL_LETTER_Y_WITH_CIRCUMFLEX:
        ucs4 = 0x85;
        break;
    case UCH_LATIN_SMALL_LETTER_Y_WITH_CIRCUMFLEX:
        ucs4 = 0x86;
        break;
        /* nothing maps to 0x87 */
        /* nothing maps to 0x88 */
        /* nothing maps to 0x89 */
        /* nothing maps to 0x8A */
        /* nothing maps to 0x8B */
    case UCH_HORIZONTAL_ELLIPSIS:
        ucs4 = 0x8C;
        break;
    case UCH_TRADE_MARK_SIGN:
        ucs4 = 0x8D;
        break;
    case UCH_PER_MILLE_SIGN:
        ucs4 = 0x8E;
        break;
    case UCH_BULLET:
        ucs4 = 0x8F;
        break;

    case UCH_LEFT_SINGLE_QUOTATION_MARK:
        ucs4 = 0x90;
        break;
    case UCH_RIGHT_SINGLE_QUOTATION_MARK:
        ucs4 = 0x91;
        break;
    case UCH_SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK:
        ucs4 = 0x92;
        break;
    case UCH_SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK:
        ucs4 = 0x93;
        break;
    case UCH_LEFT_DOUBLE_QUOTATION_MARK:
        ucs4 = 0x94;
        break;
    case UCH_RIGHT_DOUBLE_QUOTATION_MARK:
        ucs4 = 0x95;
        break;
    case UCH_DOUBLE_LOW_9_QUOTATION_MARK:
        ucs4 = 0x96;
        break;
    case UCH_EN_DASH:
        ucs4 = 0x97;
        break;
    case UCH_EM_DASH:
        ucs4 = 0x98;
        break;
    case UCH_MINUS_SIGN__UNICODE:
        ucs4 = 0x99;
        break;
    case UCH_LATIN_CAPITAL_LIGATURE_OE:
        ucs4 = 0x9A;
        break;
    case UCH_LATIN_SMALL_LIGATURE_OE:
        ucs4 = 0x9B;
        break;
    case UCH_DAGGER:
        ucs4 = 0x9C;
        break;
    case UCH_DOUBLE_DAGGER:
        ucs4 = 0x9D;
        break;
    case UCH_LATIN_SMALL_LIGATURE_FI:
        ucs4 = 0x9E;
        break;
    case UCH_LATIN_SMALL_LIGATURE_FL:
        ucs4 = 0x9F;
        break;

    default:
        /* nothing else can be mapped down; all other Unicode code points in 0000..00FF are ISO 8859-1 code points */
        break;
    }

    return(ucs4);
}

/**********************
*
* ISO 8859-2 & variants
*
**********************/

/*
ISO 8859-2
*/

_Check_return_
static UCS4
ucs4_to_sbchar_try_with_ISO_8859_2(
    _InVal_     UCS4 ucs4_in)
{
    UCS4 ucs4 = ucs4_in;

    switch(ucs4)
    {
    case UCH_NO_BREAK_SPACE:
        ucs4 = 0xA0;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_OGONEK:
        ucs4 = 0xA1;
        break;
    case UCH_BREVE:
        ucs4 = 0xA2;
        break;
    case UCH_LATIN_CAPITAL_LETTER_L_WITH_STROKE:
        ucs4 = 0xA3;
        break;
    case UCH_CURRENCY_SIGN:
        ucs4 = 0xA4;
        break;
    case UCH_LATIN_CAPITAL_LETTER_L_WITH_CARON:
        ucs4 = 0xA5;
        break;
    case UCH_LATIN_CAPITAL_LETTER_S_WITH_ACUTE:
        ucs4 = 0xA6;
        break;
    case UCH_SECTION_SIGN:
        ucs4 = 0xA7;
        break;
    case UCH_DIAERESIS:
        ucs4 = 0xA8;
        break;
    case UCH_LATIN_CAPITAL_LETTER_S_WITH_CARON:
        ucs4 = 0xA9;
        break;
    case UCH_LATIN_CAPITAL_LETTER_S_WITH_CEDILLA:
        ucs4 = 0xAA;
        break;
    case UCH_LATIN_CAPITAL_LETTER_T_WITH_CARON:
        ucs4 = 0xAB;
        break;
    case UCH_LATIN_CAPITAL_LETTER_Z_WITH_ACUTE:
        ucs4 = 0xAC;
        break;
    case UCH_SOFT_HYPHEN:
        ucs4 = 0xAD;
        break;
    case UCH_LATIN_CAPITAL_LETTER_Z_WITH_CARON:
        ucs4 = 0xAE;
        break;
    case UCH_LATIN_CAPITAL_LETTER_Z_WITH_DOT_ABOVE:
        ucs4 = 0xAF;
        break;

    case UCH_DEGREE_SIGN:
        ucs4 = 0xB0;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_OGONEK:
        ucs4 = 0xB1;
        break;
    case UCH_OGONEK:
        ucs4 = 0xB2;
        break;
    case UCH_LATIN_SMALL_LETTER_L_WITH_STROKE:
        ucs4 = 0xB3;
        break;
    case UCH_ACUTE_ACCENT:
        ucs4 = 0xB4;
        break;
    case UCH_LATIN_SMALL_LETTER_L_WITH_CARON:
        ucs4 = 0xB5;
        break;
    case UCH_LATIN_SMALL_LETTER_S_WITH_ACUTE:
        ucs4 = 0xB6;
        break;
    case UCH_CARON:
        ucs4 = 0xB7;
        break;
    case UCH_CEDILLA:
        ucs4 = 0xB8;
        break;
    case UCH_LATIN_SMALL_LETTER_S_WITH_CARON:
        ucs4 = 0xB9;
        break;
    case UCH_LATIN_SMALL_LETTER_S_WITH_CEDILLA:
        ucs4 = 0xBA;
        break;
    case UCH_LATIN_SMALL_LETTER_T_WITH_CARON:
        ucs4 = 0xBB;
        break;
    case UCH_LATIN_SMALL_LETTER_Z_WITH_ACUTE:
        ucs4 = 0xBC;
        break;
    case UCH_DOUBLE_ACUTE_ACCENT:
        ucs4 = 0xBD;
        break;
    case UCH_LATIN_SMALL_LETTER_Z_WITH_CARON:
        ucs4 = 0xBE;
        break;
    case UCH_LATIN_SMALL_LETTER_Z_WITH_DOT_ABOVE:
        ucs4 = 0xBF;
        break;

    case UCH_LATIN_CAPITAL_LETTER_R_WITH_ACUTE: ucs4 = 0xC0;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_ACUTE: ucs4 = 0xC1;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX: ucs4 = 0xC2;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_BREVE: ucs4 = 0xC3;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_DIAERESIS: ucs4 = 0xC4;
        break;
    case UCH_LATIN_CAPITAL_LETTER_L_WITH_ACUTE: ucs4 = 0xC5;
        break;
    case UCH_LATIN_CAPITAL_LETTER_C_WITH_ACUTE: ucs4 = 0xC6;
        break;
    case UCH_LATIN_CAPITAL_LETTER_C_WITH_CEDILLA: ucs4 = 0xC7;
        break;
    case UCH_LATIN_CAPITAL_LETTER_C_WITH_CARON: ucs4 = 0xC8;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_ACUTE: ucs4 = 0xC9;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_OGONEK: ucs4 = 0xCA;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS: ucs4 = 0xCB;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_CARON: ucs4 = 0xCC;
        break;
    case UCH_LATIN_CAPITAL_LETTER_I_WITH_ACUTE: ucs4 = 0xCD;
        break;
    case UCH_LATIN_CAPITAL_LETTER_I_WITH_CIRCUMFLEX: ucs4 = 0xCE;
        break;
    case UCH_LATIN_CAPITAL_LETTER_D_WITH_CARON: ucs4 = 0xCF;
        break;

    case UCH_LATIN_CAPITAL_LETTER_D_WITH_STROKE: ucs4 = 0xD0;
        break;
    case UCH_LATIN_CAPITAL_LETTER_N_WITH_ACUTE: ucs4 = 0xD1;
        break;
    case UCH_LATIN_CAPITAL_LETTER_N_WITH_CARON: ucs4 = 0xD2;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_ACUTE: ucs4 = 0xD3;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_CIRCUMFLEX: ucs4 = 0xD4;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_DOUBLE_ACUTE: ucs4 = 0xD5;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_DIAERESIS: ucs4 = 0xD6;
        break;
    case UCH_MULTIPLICATION_SIGN: ucs4 = 0xD7;
        break;
    case UCH_LATIN_CAPITAL_LETTER_R_WITH_CARON: ucs4 = 0xD8;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_RING_ABOVE: ucs4 = 0xD9;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_ACUTE: ucs4 = 0xDA;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_DOUBLE_ACUTE: ucs4 = 0xDB;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_DIAERESIS: ucs4 = 0xDC;
        break;
    case UCH_LATIN_CAPITAL_LETTER_Y_WITH_ACUTE: ucs4 = 0xDD;
        break;
    case UCH_LATIN_CAPITAL_LETTER_T_WITH_CEDILLA: ucs4 = 0xDE;
        break;
    case UCH_LATIN_SMALL_LETTER_SHARP_S: ucs4 = 0xDF;
        break;

    case UCH_LATIN_SMALL_LETTER_R_WITH_ACUTE:
        ucs4 = 0xE0;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_ACUTE:
        ucs4 = 0xE1;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_CIRCUMFLEX:
        ucs4 = 0xE2;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_BREVE:
        ucs4 = 0xE3;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_DIAERESIS:
        ucs4 = 0xE4;
        break;
    case UCH_LATIN_SMALL_LETTER_L_WITH_ACUTE:
        ucs4 = 0xE5;
        break;
    case UCH_LATIN_SMALL_LETTER_C_WITH_ACUTE:
        ucs4 = 0xE6;
        break;
    case UCH_LATIN_SMALL_LETTER_C_WITH_CEDILLA:
        ucs4 = 0xE7;
        break;
    case UCH_LATIN_SMALL_LETTER_C_WITH_CARON:
        ucs4 = 0xE8;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_ACUTE:
        ucs4 = 0xE9;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_OGONEK:
        ucs4 = 0xEA;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_DIAERESIS:
        ucs4 = 0xEB;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_CARON:
        ucs4 = 0xEC;
        break;
    case UCH_LATIN_SMALL_LETTER_I_WITH_ACUTE:
        ucs4 = 0xED;
        break;
    case UCH_LATIN_SMALL_LETTER_I_WITH_CIRCUMFLEX:
        ucs4 = 0xEE;
        break;
    case UCH_LATIN_SMALL_LETTER_D_WITH_CARON:
        ucs4 = 0xEF;
        break;

    case UCH_LATIN_SMALL_LETTER_D_WITH_STROKE:
        ucs4 = 0xF0;
        break;
    case UCH_LATIN_SMALL_LETTER_N_WITH_ACUTE:
        ucs4 = 0xF1;
        break;
    case UCH_LATIN_SMALL_LETTER_N_WITH_CARON:
        ucs4 = 0xF2;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_ACUTE:
        ucs4 = 0xF3;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_CIRCUMFLEX:
        ucs4 = 0xF4;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_DOUBLE_ACUTE:
        ucs4 = 0xF5;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_DIAERESIS:
        ucs4 = 0xF6;
        break;
    case UCH_DIVISION_SIGN:
        ucs4 = 0xF7;
        break;
    case UCH_LATIN_SMALL_LETTER_R_WITH_CARON:
        ucs4 = 0xF8;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_RING_ABOVE:
        ucs4 = 0xF9;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_ACUTE:
        ucs4 = 0xFA;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_DOUBLE_ACUTE:
        ucs4 = 0xFB;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_DIAERESIS:
        ucs4 = 0xFC;
        break;
    case UCH_LATIN_SMALL_LETTER_Y_WITH_ACUTE:
        ucs4 = 0xFD;
        break;
    case UCH_LATIN_SMALL_LETTER_T_WITH_CEDILLA:
        ucs4 = 0xFE;
        break;
    case UCH_DOT_ABOVE:
        ucs4 = 0xFF;
        break;

    default:
        /* nothing else can be mapped down; all other Unicode code points in 0000..00FF are ISO 8859-2 code points */
        break;
    }

    return(ucs4);
}

/*
Windows-28592
*/

/* Windows-28592 is ISO 8859-2 */
#define ucs4_to_sbchar_try_with_Windows_28592(ucs4_in) ( \
    ucs4_to_sbchar_try_with_ISO_8859_2(ucs4_in) )

/*
Windows-1250
*/

_Check_return_
static UCS4
ucs4_to_sbchar_try_with_Windows_1250(
    _InVal_     UCS4 ucs4_in)
{
    UCS4 ucs4 = ucs4_in;

    switch(ucs4)
    {
    /* Unicode code points which map down into 0080..009F in Windows-1250 Extensions */

    case UCH_EURO_CURRENCY_SIGN:
        ucs4 = 0x80;
        break;
        /* nothing maps to 0x81 */
    case UCH_SINGLE_LOW_9_QUOTATION_MARK:
        ucs4 = 0x82;
        break;
        /* nothing maps to 0x83 */
    case UCH_DOUBLE_LOW_9_QUOTATION_MARK:
        ucs4 = 0x84;
        break;
    case UCH_HORIZONTAL_ELLIPSIS:
        ucs4 = 0x85;
        break;
    case UCH_DAGGER:
        ucs4 = 0x86;
        break;
    case UCH_DOUBLE_DAGGER:
        ucs4 = 0x87;
        break;
        /* nothing maps to 0x88 */
    case UCH_PER_MILLE_SIGN:
        ucs4 = 0x89;
        break;
    case UCH_LATIN_CAPITAL_LETTER_S_WITH_CARON:
        ucs4 = 0x8A;
        break;
    case UCH_SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK:
        ucs4 = 0x8B;
        break;
    case UCH_LATIN_CAPITAL_LETTER_S_WITH_ACUTE:
        ucs4 = 0x8C;
        break;
    case UCH_LATIN_CAPITAL_LETTER_T_WITH_CARON:
        ucs4 = 0x8D;
        break;
    case UCH_LATIN_CAPITAL_LETTER_Z_WITH_CARON:
        ucs4 = 0x8E;
        break;
    case UCH_LATIN_CAPITAL_LETTER_Z_WITH_ACUTE:
        ucs4 = 0x8F;
        break;

        /* nothing maps to 0x90 */
    case UCH_LEFT_SINGLE_QUOTATION_MARK:
        ucs4 = 0x91;
        break;
    case UCH_RIGHT_SINGLE_QUOTATION_MARK:
        ucs4 = 0x92;
        break;
    case UCH_LEFT_DOUBLE_QUOTATION_MARK:
        ucs4 = 0x93;
        break;
    case UCH_RIGHT_DOUBLE_QUOTATION_MARK:
        ucs4 = 0x94;
        break;
    case UCH_BULLET:
        ucs4 = 0x95;
        break;
    case UCH_EN_DASH:
        ucs4 = 0x96;
        break;
    case UCH_EM_DASH:
        ucs4 = 0x97;
        break;
        /* nothing maps to 0x98 */
    case UCH_TRADE_MARK_SIGN:
        ucs4 = 0x99;
        break;
    case UCH_LATIN_SMALL_LETTER_S_WITH_CARON:
        ucs4 = 0x9A;
        break;
    case UCH_SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK:
        ucs4 = 0x9B;
        break;
    case UCH_LATIN_SMALL_LETTER_S_WITH_ACUTE:
        ucs4 = 0x9C;
        break;
    case UCH_LATIN_SMALL_LETTER_T_WITH_CARON:
        ucs4 = 0x9D;
        break;
    case UCH_LATIN_SMALL_LETTER_Z_WITH_CARON:
        ucs4 = 0x9E;
        break;
    case UCH_LATIN_SMALL_LETTER_Z_WITH_ACUTE:
        ucs4 = 0x9F;
        break;

    default:
        ucs4 = ucs4_to_sbchar_try_with_ISO_8859_2(ucs4);
        /* nothing else can be mapped down; all other Unicode code points in 0000..007F are ISO 8859-2 code points */
        break;
    }

    return(ucs4);
}

/*
Acorn Extended Latin-2
*/

_Check_return_
static UCS4
ucs4_to_sbchar_try_with_Alphabet_Latin2(
    _InVal_     UCS4 ucs4_in)
{
    UCS4 ucs4 = ucs4_in;

    switch(ucs4)
    {
    /* Unicode code points which map down into 0080..009F in Acorn Extended Latin-2 */

    case UCH_EURO_CURRENCY_SIGN:
        ucs4 = 0x80;
        break;
        /* nothing maps to 0x81 */
        /* nothing maps to 0x82 */
        /* nothing maps to 0x83 */
        /* nothing maps to 0x84 */
        /* nothing maps to 0x85 */
        /* nothing maps to 0x86 */
        /* nothing maps to 0x87 */
        /* nothing maps to 0x88 */
        /* nothing maps to 0x89 */
        /* nothing maps to 0x8A */
        /* nothing maps to 0x8B */
    case UCH_HORIZONTAL_ELLIPSIS:
        ucs4 = 0x8C;
        break;
    case UCH_TRADE_MARK_SIGN:
        ucs4 = 0x8D;
        break;
    case UCH_PER_MILLE_SIGN:
        ucs4 = 0x8E;
        break;
    case UCH_BULLET:
        ucs4 = 0x8F;
        break;

    case UCH_LEFT_SINGLE_QUOTATION_MARK:
        ucs4 = 0x90;
        break;
    case UCH_RIGHT_SINGLE_QUOTATION_MARK:
        ucs4 = 0x91;
        break;
    case UCH_SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK:
        ucs4 = 0x92;
        break;
    case UCH_SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK:
        ucs4 = 0x93;
        break;
    case UCH_LEFT_DOUBLE_QUOTATION_MARK:
        ucs4 = 0x94;
        break;
    case UCH_RIGHT_DOUBLE_QUOTATION_MARK:
        ucs4 = 0x95;
        break;
    case UCH_DOUBLE_LOW_9_QUOTATION_MARK:
        ucs4 = 0x96;
        break;
    case UCH_EN_DASH:
        ucs4 = 0x97;
        break;
    case UCH_EM_DASH:
        ucs4 = 0x98;
        break;
    case UCH_MINUS_SIGN__UNICODE:
        ucs4 = 0x99;
        break;
    case UCH_LATIN_CAPITAL_LIGATURE_OE:
        ucs4 = 0x9A;
        break;
    case UCH_LATIN_SMALL_LIGATURE_OE:
        ucs4 = 0x9B;
        break;
    case UCH_DAGGER:
        ucs4 = 0x9C;
        break;
    case UCH_DOUBLE_DAGGER:
        ucs4 = 0x9D;
        break;
    case UCH_LATIN_SMALL_LIGATURE_FI:
        ucs4 = 0x9E;
        break;
    case UCH_LATIN_SMALL_LIGATURE_FL:
        ucs4 = 0x9F;
        break;

    default:
        ucs4 = ucs4_to_sbchar_try_with_ISO_8859_2(ucs4);
        /* nothing else can be mapped down; all other Unicode code points in 0000..007F are ISO 8859-2 code points */
        break;
    }

    return(ucs4);
}

/**********************
*
* ISO 8859-3 & variants
*
**********************/

/*
ISO 8859-3
*/

_Check_return_
static UCS4
ucs4_to_sbchar_try_with_ISO_8859_3(
    _InVal_     UCS4 ucs4_in)
{
    UCS4 ucs4 = ucs4_in;

    switch(ucs4)
    {
    case UCH_NO_BREAK_SPACE:
        ucs4 = 0xA0;
        break;
    case UCH_LATIN_CAPITAL_LETTER_H_WITH_STROKE:
        ucs4 = 0xA1;
        break;
    case UCH_BREVE:
        ucs4 = 0xA2;
        break;
    case UCH_POUND_SIGN:
        ucs4 = 0xA3;
        break;
    case UCH_CURRENCY_SIGN:
        ucs4 = 0xA4;
        break;
        /* nothing maps to 0xA5*/
    case UCH_LATIN_CAPITAL_LETTER_H_WITH_CIRCUMFLEX:
        ucs4 = 0xA6;
        break;
    case UCH_SECTION_SIGN:
        ucs4 = 0xA7;
        break;
    case UCH_DIAERESIS:
        ucs4 = 0xA8;
        break;
    case UCH_LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE:
        ucs4 = 0xA9;
        break;
    case UCH_LATIN_CAPITAL_LETTER_S_WITH_CEDILLA:
        ucs4 = 0xAA;
        break;
    case UCH_LATIN_CAPITAL_LETTER_G_WITH_BREVE:
        ucs4 = 0xAB;
        break;
    case UCH_LATIN_CAPITAL_LETTER_J_WITH_CIRCUMFLEX:
        ucs4 = 0xAC;
        break;
    case UCH_SOFT_HYPHEN:
        ucs4 = 0xAD;
        break;
        /* nothing maps to 0xAE */
    case UCH_LATIN_CAPITAL_LETTER_Z_WITH_DOT_ABOVE:
        ucs4 = 0xAF;
        break;

    case UCH_DEGREE_SIGN:
        ucs4 = 0xB0;
        break;
    case UCH_LATIN_SMALL_LETTER_H_WITH_STROKE:
        ucs4 = 0xB1;
        break;
    case UCH_SUPERSCRIPT_TWO:
        ucs4 = 0xB2;
        break;
    case UCH_SUPERSCRIPT_THREE:
        ucs4 = 0xB3;
        break;
    case UCH_ACUTE_ACCENT:
        ucs4 = 0xB4;
        break;
    case UCH_MICRO_SIGN:
        ucs4 = 0xB5;
        break;
    case UCH_LATIN_SMALL_LETTER_H_WITH_CIRCUMFLEX:
        ucs4 = 0xB6;
        break;
    case UCH_MIDDLE_DOT:
        ucs4 = 0xB7;
        break;
    case UCH_CEDILLA:
        ucs4 = 0xB8;
        break;
    case UCH_LATIN_SMALL_LETTER_DOTLESS_I:
        ucs4 = 0xB9;
        break;
    case UCH_LATIN_SMALL_LETTER_S_WITH_CEDILLA:
        ucs4 = 0xBA;
        break;
    case UCH_LATIN_SMALL_LETTER_G_WITH_BREVE:
        ucs4 = 0xBB;
        break;
    case UCH_LATIN_SMALL_LETTER_J_WITH_CIRCUMFLEX:
        ucs4 = 0xBC;
        break;
    case UCH_VULGAR_FRACTION_ONE_HALF:
        ucs4 = 0xBD;
        break;
        /* nothing maps to 0xBE */
    case UCH_LATIN_SMALL_LETTER_Z_WITH_DOT_ABOVE:
        ucs4 = 0xBF;
        break;

    case UCH_LATIN_CAPITAL_LETTER_A_WITH_GRAVE:
        ucs4 = 0xC0;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_ACUTE:
        ucs4 = 0xC1;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX:
        ucs4 = 0xC2;
        break;
        /* nothing maps to 0xC3 */
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_DIAERESIS:
        ucs4 = 0xC4;
        break;
    case UCH_LATIN_CAPITAL_LETTER_C_WITH_DOT_ABOVE:
        ucs4 = 0xC5;
        break;
    case UCH_LATIN_CAPITAL_LETTER_C_WITH_CIRCUMFLEX:
        ucs4 = 0xC6;
        break;
    case UCH_LATIN_CAPITAL_LETTER_C_WITH_CEDILLA:
        ucs4 = 0xC7;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_GRAVE:
        ucs4 = 0xC8;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_ACUTE:
        ucs4 = 0xC9;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_CIRCUMFLEX:
        ucs4 = 0xCA;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS:
        ucs4 = 0xCB;
        break;
    case UCH_LATIN_CAPITAL_LETTER_I_WITH_GRAVE:
        ucs4 = 0xCC;
        break;
    case UCH_LATIN_CAPITAL_LETTER_I_WITH_ACUTE:
        ucs4 = 0xCD;
        break;
    case UCH_LATIN_CAPITAL_LETTER_I_WITH_CIRCUMFLEX:
        ucs4 = 0xCE;
        break;
    case UCH_LATIN_CAPITAL_LETTER_I_WITH_DIAERESIS:
        ucs4 = 0xCF;
        break;

        /* nothing maps to 0xD0 */
    case UCH_LATIN_CAPITAL_LETTER_N_WITH_TILDE:
        ucs4 = 0xD1;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_GRAVE:
        ucs4 = 0xD2;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_ACUTE:
        ucs4 = 0xD3;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_CIRCUMFLEX:
        ucs4 = 0xD4;
        break;
    case UCH_LATIN_CAPITAL_LETTER_G_WITH_DOT_ABOVE:
        ucs4 = 0xD5;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_DIAERESIS:
        ucs4 = 0xD6;
        break;
    case UCH_MULTIPLICATION_SIGN:
        ucs4 = 0xD7;
        break;
    case UCH_LATIN_CAPITAL_LETTER_G_WITH_CIRCUMFLEX:
        ucs4 = 0xD8;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_GRAVE:
        ucs4 = 0xD9;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_ACUTE:
        ucs4 = 0xDA;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_CIRCUMFLEX:
        ucs4 = 0xDB;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_DIAERESIS:
        ucs4 = 0xDC;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_BREVE:
        ucs4 = 0xDD;
        break;
    case UCH_LATIN_CAPITAL_LETTER_S_WITH_CIRCUMFLEX:
        ucs4 = 0xDE;
        break;
    case UCH_LATIN_SMALL_LETTER_SHARP_S:
        ucs4 = 0xDF;
        break;

    case UCH_LATIN_SMALL_LETTER_A_WITH_GRAVE:
        ucs4 = 0xE0;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_ACUTE:
        ucs4 = 0xE1;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_CIRCUMFLEX:
        ucs4 = 0xE2;
        break;
        /* nothing maps to 0xE3 */
    case UCH_LATIN_SMALL_LETTER_A_WITH_DIAERESIS:
        ucs4 = 0xE4;
        break;
    case UCH_LATIN_SMALL_LETTER_C_WITH_DOT_ABOVE:
        ucs4 = 0xE5;
        break;
    case UCH_LATIN_SMALL_LETTER_C_WITH_CIRCUMFLEX:
        ucs4 = 0xE6;
        break;
    case UCH_LATIN_SMALL_LETTER_C_WITH_CEDILLA:
        ucs4 = 0xE7;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_GRAVE:
        ucs4 = 0xE8;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_ACUTE:
        ucs4 = 0xE9;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_CIRCUMFLEX:
        ucs4 = 0xEA;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_DIAERESIS:
        ucs4 = 0xEB;
        break;
    case UCH_LATIN_SMALL_LETTER_I_WITH_GRAVE:
        ucs4 = 0xEC;
        break;
    case UCH_LATIN_SMALL_LETTER_I_WITH_ACUTE:
        ucs4 = 0xED;
        break;
    case UCH_LATIN_SMALL_LETTER_I_WITH_CIRCUMFLEX:
        ucs4 = 0xEE;
        break;
    case UCH_LATIN_SMALL_LETTER_I_WITH_DIAERESIS:
        ucs4 = 0xEF;
        break;

    case UCH_REPLACEMENT_CHARACTER:
        ucs4 = 0xF0;
        break;
    case UCH_LATIN_SMALL_LETTER_N_WITH_TILDE:
        ucs4 = 0xF1;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_GRAVE:
        ucs4 = 0xF2;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_ACUTE:
        ucs4 = 0xF3;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_CIRCUMFLEX:
        ucs4 = 0xF4;
        break;
    case UCH_LATIN_SMALL_LETTER_G_WITH_DOT_ABOVE:
        ucs4 = 0xF5;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_DIAERESIS:
        ucs4 = 0xF6;
        break;
    case UCH_DIVISION_SIGN:
        ucs4 = 0xF7;
        break;
    case UCH_LATIN_SMALL_LETTER_G_WITH_CIRCUMFLEX:
        ucs4 = 0xF8;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_GRAVE:
        ucs4 = 0xF9;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_ACUTE:
        ucs4 = 0xFA;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_CIRCUMFLEX:
        ucs4 = 0xFB;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_DIAERESIS:
        ucs4 = 0xFC;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_BREVE:
        ucs4 = 0xFD;
        break;
    case UCH_LATIN_SMALL_LETTER_S_WITH_CIRCUMFLEX:
        ucs4 = 0xFE;
        break;
    case UCH_DOT_ABOVE:
        ucs4 = 0xFF;
        break;

    default:
        /* nothing else can be mapped down; all other Unicode code points in 0000..00FF are ISO 8859-1 code points */
        break;
    }

    return(ucs4);
}

/*
Windows-28593
*/

/* Windows-28593 is ISO 8859-3 */
#define ucs4_to_sbchar_try_with_Windows_28593(ucs4_in) ( \
    ucs4_to_sbchar_try_with_ISO_8859_3(ucs4_in) )

/*
Acorn Extended Latin-3
*/

_Check_return_
static UCS4
ucs4_to_sbchar_try_with_Alphabet_Latin3(
    _InVal_     UCS4 ucs4_in)
{
    UCS4 ucs4 = ucs4_in;

    switch(ucs4)
    {
    /* Unicode code points which map down into 0080..009F in Acorn Extended Latin-3 */

    case UCH_EURO_CURRENCY_SIGN:
        ucs4 = 0x80;
        break;
        /* nothing maps to 0x81 */
        /* nothing maps to 0x82 */
        /* nothing maps to 0x83 */
        /* nothing maps to 0x84 */
        /* nothing maps to 0x85 */
        /* nothing maps to 0x86 */
        /* nothing maps to 0x87 */
        /* nothing maps to 0x88 */
        /* nothing maps to 0x89 */
        /* nothing maps to 0x8A */
        /* nothing maps to 0x8B */
    case UCH_HORIZONTAL_ELLIPSIS:
        ucs4 = 0x8C;
        break;
    case UCH_TRADE_MARK_SIGN:
        ucs4 = 0x8D;
        break;
    case UCH_PER_MILLE_SIGN:
        ucs4 = 0x8E;
        break;
    case UCH_BULLET:
        ucs4 = 0x8F;
        break;

    case UCH_LEFT_SINGLE_QUOTATION_MARK:
        ucs4 = 0x90;
        break;
    case UCH_RIGHT_SINGLE_QUOTATION_MARK:
        ucs4 = 0x91;
        break;
    case UCH_SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK:
        ucs4 = 0x92;
        break;
    case UCH_SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK:
        ucs4 = 0x93;
        break;
    case UCH_LEFT_DOUBLE_QUOTATION_MARK:
        ucs4 = 0x94;
        break;
    case UCH_RIGHT_DOUBLE_QUOTATION_MARK:
        ucs4 = 0x95;
        break;
    case UCH_DOUBLE_LOW_9_QUOTATION_MARK:
        ucs4 = 0x96;
        break;
    case UCH_EN_DASH:
        ucs4 = 0x97;
        break;
    case UCH_EM_DASH:
        ucs4 = 0x98;
        break;
    case UCH_MINUS_SIGN__UNICODE:
        ucs4 = 0x99;
        break;
    case UCH_LATIN_CAPITAL_LIGATURE_OE:
        ucs4 = 0x9A;
        break;
    case UCH_LATIN_SMALL_LIGATURE_OE:
        ucs4 = 0x9B;
        break;
    case UCH_DAGGER:
        ucs4 = 0x9C;
        break;
    case UCH_DOUBLE_DAGGER:
        ucs4 = 0x9D;
        break;
    case UCH_LATIN_SMALL_LIGATURE_FI:
        ucs4 = 0x9E;
        break;
    case UCH_LATIN_SMALL_LIGATURE_FL:
        ucs4 = 0x9F;
        break;

    default:
        ucs4 = ucs4_to_sbchar_try_with_ISO_8859_3(ucs4);
        /* nothing else can be mapped down; all other Unicode code points in 0000..007F are ISO 8859-3 code points */
        break;
    }

    return(ucs4);
}


/**********************
*
* ISO 8859-4 & variants
*
**********************/

/*
ISO 8859-4
*/

_Check_return_
static UCS4
ucs4_to_sbchar_try_with_ISO_8859_4(
    _InVal_     UCS4 ucs4_in)
{
    UCS4 ucs4 = ucs4_in;

    switch(ucs4)
    {
    case UCH_NO_BREAK_SPACE:
        ucs4 = 0xA0;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_OGONEK:
        ucs4 = 0xA1;
        break;
    case UCH_LATIN_SMALL_LETTER_KRA:
        ucs4 = 0xA2;
        break;
    case UCH_LATIN_CAPITAL_LETTER_R_WITH_CEDILLA:
        ucs4 = 0xA3;
        break;
    case UCH_CURRENCY_SIGN:
        ucs4 = 0xA4;
        break;
    case UCH_LATIN_CAPITAL_LETTER_I_WITH_TILDE:
        ucs4 = 0xA5;
        break;
    case UCH_LATIN_CAPITAL_LETTER_L_WITH_CEDILLA:
        ucs4 = 0xA6;
        break;
    case UCH_SECTION_SIGN:
        ucs4 = 0xA7;
        break;
    case UCH_DIAERESIS:
        ucs4 = 0xA8;
        break;
    case UCH_LATIN_CAPITAL_LETTER_S_WITH_CARON:
        ucs4 = 0xA9;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_MACRON:
        ucs4 = 0xAA;
        break;
    case UCH_LATIN_CAPITAL_LETTER_G_WITH_CEDILLA:
        ucs4 = 0xAB;
        break;
    case UCH_LATIN_CAPITAL_LETTER_T_WITH_STROKE:
        ucs4 = 0xAC;
        break;
    case UCH_SOFT_HYPHEN:
        ucs4 = 0xAD;
        break;
    case UCH_LATIN_CAPITAL_LETTER_Z_WITH_CARON:
        ucs4 = 0xAE;
        break;
    case UCH_MACRON:
        ucs4 = 0xAF;
        break;

    case UCH_DEGREE_SIGN:
        ucs4 = 0xB0;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_OGONEK:
        ucs4 = 0xB1;
        break;
    case UCH_OGONEK:
        ucs4 = 0xB2;
        break;
    case UCH_LATIN_SMALL_LETTER_R_WITH_CEDILLA:
        ucs4 = 0xB3;
        break;
    case UCH_ACUTE_ACCENT:
        ucs4 = 0xB4;
        break;
    case UCH_LATIN_SMALL_LETTER_I_WITH_TILDE:
        ucs4 = 0xB5;
        break;
    case UCH_LATIN_SMALL_LETTER_L_WITH_CEDILLA:
        ucs4 = 0xB6;
        break;
    case UCH_CARON:
        ucs4 = 0xB7;
        break;
    case UCH_CEDILLA:
        ucs4 = 0xB8;
        break;
    case UCH_LATIN_SMALL_LETTER_S_WITH_CARON:
        ucs4 = 0xB9;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_MACRON:
        ucs4 = 0xBA;
        break;
    case UCH_LATIN_SMALL_LETTER_G_WITH_CEDILLA:
        ucs4 = 0xBB;
        break;
    case UCH_LATIN_SMALL_LETTER_T_WITH_STROKE:
        ucs4 = 0xBC;
        break;
    case UCH_LATIN_CAPITAL_LETTER_ENG:
        ucs4 = 0xBD;
        break;
    case UCH_LATIN_SMALL_LETTER_Z_WITH_CARON:
        ucs4 = 0xBE;
        break;
    case UCH_LATIN_SMALL_LETTER_ENG:
        ucs4 = 0xBF;
        break;

    case UCH_LATIN_CAPITAL_LETTER_A_WITH_MACRON:
        ucs4 = 0xC0;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_ACUTE:
        ucs4 = 0xC1;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX:
        ucs4 = 0xC2;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_TILDE:
        ucs4 = 0xC3;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_DIAERESIS:
        ucs4 = 0xC4;
        break;
    case UCH_LATIN_CAPITAL_LETTER_A_WITH_RING_ABOVE:
        ucs4 = 0xC5;
        break;
    case UCH_LATIN_CAPITAL_LETTER_AE:
        ucs4 = 0xC6;
        break;
    case UCH_LATIN_CAPITAL_LETTER_I_WITH_OGONEK:
        ucs4 = 0xC7;
        break;
    case UCH_LATIN_CAPITAL_LETTER_C_WITH_CARON:
        ucs4 = 0xC8;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_ACUTE:
        ucs4 = 0xC9;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_OGONEK:
        ucs4 = 0xCA;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS:
        ucs4 = 0xCB;
        break;
    case UCH_LATIN_CAPITAL_LETTER_E_WITH_DOT_ABOVE:
        ucs4 = 0xCC;
        break;
    case UCH_LATIN_CAPITAL_LETTER_I_WITH_ACUTE:
        ucs4 = 0xCD;
        break;
    case UCH_LATIN_CAPITAL_LETTER_I_WITH_CIRCUMFLEX:
        ucs4 = 0xCE;
        break;
    case UCH_LATIN_CAPITAL_LETTER_I_WITH_MACRON:
        ucs4 = 0xCF;
        break;

    case UCH_LATIN_CAPITAL_LETTER_D_WITH_STROKE:
        ucs4 = 0xD0;
        break;
    case UCH_LATIN_CAPITAL_LETTER_N_WITH_CEDILLA:
        ucs4 = 0xD1;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_MACRON:
        ucs4 = 0xD2;
        break;
    case UCH_LATIN_CAPITAL_LETTER_K_WITH_CEDILLA:
        ucs4 = 0xD3;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_CIRCUMFLEX:
        ucs4 = 0xD4;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_TILDE:
        ucs4 = 0xD5;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_DIAERESIS:
        ucs4 = 0xD6;
        break;
    case UCH_MULTIPLICATION_SIGN:
        ucs4 = 0xD7;
        break;
    case UCH_LATIN_CAPITAL_LETTER_O_WITH_STROKE:
        ucs4 = 0xD8;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_OGONEK:
        ucs4 = 0xD9;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_ACUTE:
        ucs4 = 0xDA;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_CIRCUMFLEX:
        ucs4 = 0xDB;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_DIAERESIS:
        ucs4 = 0xDC;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_TILDE:
        ucs4 = 0xDD;
        break;
    case UCH_LATIN_CAPITAL_LETTER_U_WITH_MACRON:
        ucs4 = 0xDE;
        break;
    case UCH_LATIN_SMALL_LETTER_SHARP_S:
        ucs4 = 0xDF;
        break;

    case UCH_LATIN_SMALL_LETTER_A_WITH_MACRON:
        ucs4 = 0xE0;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_ACUTE:
        ucs4 = 0xE1;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_CIRCUMFLEX:
        ucs4 = 0xE2;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_TILDE:
        ucs4 = 0xE3;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_DIAERESIS:
        ucs4 = 0xE4;
        break;
    case UCH_LATIN_SMALL_LETTER_A_WITH_RING_ABOVE:
        ucs4 = 0xE5;
        break;
    case UCH_LATIN_SMALL_LETTER_AE:
        ucs4 = 0xE6;
        break;
    case UCH_LATIN_SMALL_LETTER_I_WITH_OGONEK:
        ucs4 = 0xE7;
        break;
    case UCH_LATIN_SMALL_LETTER_C_WITH_CARON:
        ucs4 = 0xE8;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_ACUTE:
        ucs4 = 0xE9;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_OGONEK:
        ucs4 = 0xEA;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_DIAERESIS:
        ucs4 = 0xEB;
        break;
    case UCH_LATIN_SMALL_LETTER_E_WITH_DOT_ABOVE:
        ucs4 = 0xEC;
        break;
    case UCH_LATIN_SMALL_LETTER_I_WITH_ACUTE:
        ucs4 = 0xED;
        break;
    case UCH_LATIN_SMALL_LETTER_I_WITH_CIRCUMFLEX:
        ucs4 = 0xEE;
        break;
    case UCH_LATIN_SMALL_LETTER_I_WITH_MACRON:
        ucs4 = 0xEF;
        break;

    case UCH_LATIN_SMALL_LETTER_D_WITH_STROKE:
        ucs4 = 0xF0;
        break;
    case UCH_LATIN_SMALL_LETTER_N_WITH_CEDILLA:
        ucs4 = 0xF1;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_MACRON:
        ucs4 = 0xF2;
        break;
    case UCH_LATIN_SMALL_LETTER_K_WITH_CEDILLA:
        ucs4 = 0xF3;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_CIRCUMFLEX:
        ucs4 = 0xF4;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_TILDE:
        ucs4 = 0xF5;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_DIAERESIS:
        ucs4 = 0xF6;
        break;
    case UCH_DIVISION_SIGN:
        ucs4 = 0xF7;
        break;
    case UCH_LATIN_SMALL_LETTER_O_WITH_STROKE:
        ucs4 = 0xF8;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_OGONEK:
        ucs4 = 0xF9;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_ACUTE:
        ucs4 = 0xFA;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_CIRCUMFLEX:
        ucs4 = 0xFB;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_DIAERESIS:
        ucs4 = 0xFC;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_TILDE:
        ucs4 = 0xFD;
        break;
    case UCH_LATIN_SMALL_LETTER_U_WITH_MACRON:
        ucs4 = 0xFE;
        break;
    case UCH_DOT_ABOVE:
        ucs4 = 0xFF;
        break;

    default:
        /* nothing else can be mapped down; all other Unicode code points in 0000..00FF are ISO 8859-1 code points */
        break;
    }

    return(ucs4);
}

/*
Windows-28594
*/

/* Windows-28594 is ISO 8859-4 */
#define ucs4_to_sbchar_try_with_Windows_28594(ucs4_in) ( \
    ucs4_to_sbchar_try_with_ISO_8859_4(ucs4_in) )

/*
Acorn Extended Latin-4
*/

_Check_return_
static UCS4
ucs4_to_sbchar_try_with_Alphabet_Latin4(
    _InVal_     UCS4 ucs4_in)
{
    UCS4 ucs4 = ucs4_in;

    switch(ucs4)
    {
    /* Unicode code points which map down into 0080..009F in Acorn Extended Latin-4 */

    case UCH_EURO_CURRENCY_SIGN:
        ucs4 = 0x80;
        break;
        /* nothing maps to 0x81 */
        /* nothing maps to 0x82 */
        /* nothing maps to 0x83 */
        /* nothing maps to 0x84 */
        /* nothing maps to 0x85 */
        /* nothing maps to 0x86 */
        /* nothing maps to 0x87 */
        /* nothing maps to 0x88 */
        /* nothing maps to 0x89 */
        /* nothing maps to 0x8A */
        /* nothing maps to 0x8B */
    case UCH_HORIZONTAL_ELLIPSIS:
        ucs4 = 0x8C;
        break;
    case UCH_TRADE_MARK_SIGN:
        ucs4 = 0x8D;
        break;
    case UCH_PER_MILLE_SIGN:
        ucs4 = 0x8E;
        break;
    case UCH_BULLET:
        ucs4 = 0x8F;
        break;

    case UCH_LEFT_SINGLE_QUOTATION_MARK:
        ucs4 = 0x90;
        break;
    case UCH_RIGHT_SINGLE_QUOTATION_MARK:
        ucs4 = 0x91;
        break;
    case UCH_SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK:
        ucs4 = 0x92;
        break;
    case UCH_SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK:
        ucs4 = 0x93;
        break;
    case UCH_LEFT_DOUBLE_QUOTATION_MARK:
        ucs4 = 0x94;
        break;
    case UCH_RIGHT_DOUBLE_QUOTATION_MARK:
        ucs4 = 0x95;
        break;
    case UCH_DOUBLE_LOW_9_QUOTATION_MARK:
        ucs4 = 0x96;
        break;
    case UCH_EN_DASH:
        ucs4 = 0x97;
        break;
    case UCH_EM_DASH:
        ucs4 = 0x98;
        break;
    case UCH_MINUS_SIGN__UNICODE:
        ucs4 = 0x99;
        break;
    case UCH_LATIN_CAPITAL_LIGATURE_OE:
        ucs4 = 0x9A;
        break;
    case UCH_LATIN_SMALL_LIGATURE_OE:
        ucs4 = 0x9B;
        break;
    case UCH_DAGGER:
        ucs4 = 0x9C;
        break;
    case UCH_DOUBLE_DAGGER:
        ucs4 = 0x9D;
        break;
    case UCH_LATIN_SMALL_LIGATURE_FI:
        ucs4 = 0x9E;
        break;
    case UCH_LATIN_SMALL_LIGATURE_FL:
        ucs4 = 0x9F;
        break;

    default:
        ucs4 = ucs4_to_sbchar_try_with_ISO_8859_4(ucs4);
        /* nothing else can be mapped down; all other Unicode code points in 0000..007F are ISO 8859-4 code points */
        break;
    }

    return(ucs4);
}

/***********************
*
* ISO 8859-15 & variants
*
***********************/

/*
ISO 8859-15
*/

_Check_return_
static UCS4
ucs4_to_sbchar_try_with_ISO_8859_15(
    _InVal_     UCS4 ucs4_in)
{
    UCS4 ucs4 = ucs4_in;

    switch(ucs4)
    {
    /* Unicode code points which map down into 00A0..00FF in ISO 8859-15 */

    case UCH_EURO_CURRENCY_SIGN:
        ucs4 = 0xA4;
        break;

    case UCH_LATIN_CAPITAL_LETTER_S_WITH_CARON:
        ucs4 = 0xA6;
        break;

    case UCH_LATIN_SMALL_LETTER_S_WITH_CARON:
        ucs4 = 0xA8;
        break;

    case UCH_LATIN_CAPITAL_LETTER_Z_WITH_CARON:
        ucs4 = 0xB4;
        break;

    case UCH_LATIN_SMALL_LETTER_Z_WITH_CARON:
        ucs4 = 0xB8;
        break;

    case UCH_LATIN_CAPITAL_LIGATURE_OE:
        ucs4 = 0xBC;
        break;

    case UCH_LATIN_SMALL_LIGATURE_OE:
        ucs4 = 0xBC;
        break;

    case UCH_LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS:
        ucs4 = 0xBE;
        break;

    default:
        /* nothing else can be mapped down; all other Unicode code points in 0000..00FF are ISO 8859-15 code points */
        break;
    }

    return(ucs4);
}

/*
Windows-28605
*/

/* Windows-28605 is ISO 8859-15 */
#define ucs4_to_sbchar_try_with_Windows_28605(ucs4_in) ( \
    ucs4_to_sbchar_try_with_ISO_8859_15(ucs4_in) )

/*
Acorn Extended Latin-9
*/

_Check_return_
static UCS4
ucs4_to_sbchar_try_with_Alphabet_Latin9(
    _InVal_     UCS4 ucs4_in)
{
    UCS4 ucs4 = ucs4_in;

    switch(ucs4)
    {
    /* Unicode code points which map down into 0080..009F in Acorn Extended Latin-2 */

    /* Unicode code points which map down into 00A0..00FF in ISO 8859-15 */

    case UCH_EURO_CURRENCY_SIGN:
        ucs4 = 0xA4;
        break;

    case UCH_LATIN_CAPITAL_LETTER_S_WITH_CARON:
        ucs4 = 0xA6;
        break;

    case UCH_LATIN_SMALL_LETTER_S_WITH_CARON:
        ucs4 = 0xA8;
        break;

    case UCH_LATIN_CAPITAL_LETTER_Z_WITH_CARON:
        ucs4 = 0xB4;
        break;

    case UCH_LATIN_SMALL_LETTER_Z_WITH_CARON:
        ucs4 = 0xB8;
        break;

    case UCH_LATIN_CAPITAL_LIGATURE_OE:
        ucs4 = 0xBC;
        break;

    case UCH_LATIN_SMALL_LIGATURE_OE:
        ucs4 = 0xBD;
        break;

    case UCH_LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS:
        ucs4 = 0xBE;
        break;

    default:
        /* nothing else can be mapped down; all other Unicode code points in 0000..00FF are ISO 8859-15 code points */
        break;
    }

    return(ucs4);
}

_Check_return_
extern UCS4
ucs4_to_sbchar_try_with_codepage(
    _InVal_     UCS4 ucs4,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage)
{
    switch(sbchar_codepage)
    {
    case 0: /* no mapping */
        return(ucs4);

#if WINDOWS
    default: default_unhandled();
#endif
    case SBCHAR_CODEPAGE_WINDOWS_1252:
        return(ucs4_to_sbchar_try_with_Windows_1252(ucs4));

    case SBCHAR_CODEPAGE_WINDOWS_1250:
        return(ucs4_to_sbchar_try_with_Windows_1250(ucs4));

    case SBCHAR_CODEPAGE_WINDOWS_28591:
        return(ucs4_to_sbchar_try_with_Windows_28591(ucs4));

    case SBCHAR_CODEPAGE_WINDOWS_28592:
        return(ucs4_to_sbchar_try_with_Windows_28592(ucs4));

    case SBCHAR_CODEPAGE_WINDOWS_28593:
        return(ucs4_to_sbchar_try_with_Windows_28593(ucs4));

    case SBCHAR_CODEPAGE_WINDOWS_28605:
        return(ucs4_to_sbchar_try_with_Windows_28605(ucs4));

#if RISCOS
    default: default_unhandled();
#endif
    case SBCHAR_CODEPAGE_ALPHABET_LATIN1:
        return(ucs4_to_sbchar_try_with_Alphabet_Latin1(ucs4));

    case SBCHAR_CODEPAGE_ALPHABET_LATIN2:
        return(ucs4_to_sbchar_try_with_Alphabet_Latin2(ucs4));

    case SBCHAR_CODEPAGE_ALPHABET_LATIN3:
        return(ucs4_to_sbchar_try_with_Alphabet_Latin3(ucs4));

    case SBCHAR_CODEPAGE_ALPHABET_LATIN4:
        return(ucs4_to_sbchar_try_with_Alphabet_Latin4(ucs4));

    case SBCHAR_CODEPAGE_ALPHABET_LATIN9:
        return(ucs4_to_sbchar_try_with_Alphabet_Latin9(ucs4));
    }
}

/* end of ucs4conv.c */
