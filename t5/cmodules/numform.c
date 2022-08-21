/* numform.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Formats data in an ss_data structure */

/* James A. Dixon 07-Jan-1992; SKS April 1993 major hack */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
internal structure
*/

#define SECTION_MAX 200
#define BUF_SECTION_MAX 201

#define LIST_SEPARATOR CH_COMMA

#define ROMAN_MAX 3999

typedef struct NUMFORM_INFO
{
    S32 type; /* conversion type being performed */

    P_QUICK_UBLOCK p_quick_ublock; /*appended*/

    SS_DATA ss_data;

    struct NUMFORM_INFO_NUMBER
    {
        U8 zero;
        U8 negative;
        U8 insert_minus_sign;
        U8 nan;
        U8 infinity;
    }
    number;

    struct NUMFORM_INFO_DATE
    {
        BOOL valid;

        S32 day, month, year;
    }
    date;

    struct NUMFORM_INFO_TIME
    {
        BOOL valid;

        S32 hours, minutes, seconds;
    }
    time;

    S32 integer_places_format;
    S32 decimal_places_format;
    S32 exponent_places_format;

    P_USTR ustr_integer_section;
    S32 elemof_integer_section;
    S32 integer_places_actual;

    PC_USTR ustr_decimal_section;
    S32 decimal_places_actual;
    S32 decimal_section_spaces;

    P_USTR ustr_exponent_section;
    S32 exponent_places_actual;
    S32 exponent_section_spaces;
    U8 exponent_sign_actual;
    U8 _spare[3];

    PC_USTR ustr_engineering_section;

    P_USTR ustr_lookup_section;
    P_USTR ustr_style_name;

    U8  decimal_pt;
    U8  engineering;
    U8  exponential;
    U8  exponent_sign;
    U8  roman;
    U8  spreadsheet;
    U8  thousands_sep;

    U8  integer_section_result_output;

    U8  decimal_section_has_hash;
    U8  decimal_section_has_zero;

    U8  base_basechar;
    S32 base;
}
NUMFORM_INFO, * P_NUMFORM_INFO;

/*
internal routines
*/

_Check_return_
static STATUS
numform_output(
    _InRef_     PC_NUMFORM_CONTEXT p_numform_context,
    _InRef_     P_NUMFORM_INFO p_numform_info,
    _In_z_      PC_USTR ustr_numform_section);

_Check_return_
static STATUS
numform_output_date_fields(
    P_NUMFORM_INFO p_numform_info,
    _InRef_     PC_NUMFORM_CONTEXT p_numform_context,
    _InoutRef_  P_PC_USTR p_ustr_numform_section);

_Check_return_
static STATUS
numform_output_number_fields(
    P_NUMFORM_INFO p_numform_info,
    _InVal_     U8 ch);

_Check_return_
static STATUS
numform_output_style_name(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock_style /*appended,terminated*/,
    _In_z_      PC_USTR ustr_style_name);

static void
numform_section_extract_datetime(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_datetime);

static void
numform_section_extract_numeric(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_numeric,
    _Out_writes_z_(elemof_numeric_buffer) P_USTR ustr_numeric_buf,
    _InVal_     U32 elemof_numeric_buffer);

static void
numform_section_extract_texterror(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_texterror);

_Check_return_
_Ret_z_ /*opt_*/
static PC_USTR
numform_section_scan_next(
    _In_z_      PC_USTR ustr_section);

#if USTR_IS_SBSTR

#define ustr_GetByteInc(ustr) \
    *(ustr++)

#define ustr_GetByteInc_wr(ustr) \
    *(ustr++)

#else

_Check_return_
static inline U8
_ustr_GetByteInc(P_PC_USTR p_ustr)
{
    U8 u8 = PtrGetByte(*p_ustr);
    *p_ustr = ustr_AddBytes(*p_ustr, 1);
    return(u8);
}

#if defined(__cplusplus)

_Check_return_
static inline U8
_ustr_GetByteInc(P_P_USTR p_ustr)
{
    U8 u8 = PtrGetByte(*p_ustr);
    *p_ustr = ustr_AddBytes_wr(*p_ustr, 1);
    return(u8);
}

#endif /* __cplusplus */

#define ustr_GetByteInc(ustr) \
    _ustr_GetByteInc(&ustr)

_Check_return_
static inline U8
_ustr_GetByteInc_wr(P_P_USTR p_ustr)
{
    U8 u8 = PtrGetByte(*p_ustr);
    *p_ustr = ustr_AddBytes_wr(*p_ustr, 1);
    return(u8);
}

#define ustr_GetByteInc_wr(ustr_wr) \
    _ustr_GetByteInc_wr(&ustr_wr)

#endif

/*
internal variables
*/

static U8 numform_output_datetime_last_field = CH_NULL; /* Last field output */

static U32
convert_digit_roman(
    P_USTR ustr_num_buf,
    _InVal_     U8 ch,
    _In_z_      PC_U8Z p_ones_fives_tens)
{
    const U8 ones = p_ones_fives_tens[0];
    U32 n_bytes;

    switch(ch)
    {
    default: default_unhandled();
#if CHECKING
    case CH_DIGIT_ZERO:
#endif
        n_bytes = 0;
        break;

    case '1':
        PtrPutByteOff(ustr_num_buf, 0, ones);
        n_bytes = 1;
        break;

    case '2':
        PtrPutByteOff(ustr_num_buf, 0, ones);
        PtrPutByteOff(ustr_num_buf, 1, ones);
        n_bytes = 2;
        break;

    case '3':
        PtrPutByteOff(ustr_num_buf, 0, ones);
        PtrPutByteOff(ustr_num_buf, 1, ones);
        PtrPutByteOff(ustr_num_buf, 2, ones);
        n_bytes = 3;
        break;

    case '4':
        PtrPutByteOff(ustr_num_buf, 0, ones);
        PtrPutByteOff(ustr_num_buf, 1, p_ones_fives_tens[1]);
        n_bytes = 2;
        break;

    case '5':
        PtrPutByteOff(ustr_num_buf, 0, p_ones_fives_tens[1]);
        n_bytes = 1;
        break;

    case '6':
        PtrPutByteOff(ustr_num_buf, 0, p_ones_fives_tens[1]);
        PtrPutByteOff(ustr_num_buf, 1, ones);
        n_bytes = 2;
        break;

    case '7':
        PtrPutByteOff(ustr_num_buf, 0, p_ones_fives_tens[1]);
        PtrPutByteOff(ustr_num_buf, 1, ones);
        PtrPutByteOff(ustr_num_buf, 2, ones);
        n_bytes = 3;
        break;

    case '8':
        PtrPutByteOff(ustr_num_buf, 0, p_ones_fives_tens[1]);
        PtrPutByteOff(ustr_num_buf, 1, ones);
        PtrPutByteOff(ustr_num_buf, 2, ones);
        PtrPutByteOff(ustr_num_buf, 3, ones);
        n_bytes = 4;
        break;

    case CH_DIGIT_NINE:
        PtrPutByteOff(ustr_num_buf, 0, ones);
        PtrPutByteOff(ustr_num_buf, 1, p_ones_fives_tens[2]);
        n_bytes = 2;
        break;
    }

    return(n_bytes);
}

/******************************************************************************
*
* Output engineering number to num_buf
* Must also set integer_places_actual
*
******************************************************************************/

static void
convert_number_engineering(
    P_NUMFORM_INFO p_numform_info)
{
    F64 work_value = ss_data_get_number(&p_numform_info->ss_data);
    S32 remainder = 0;
    S32 decimal_places;
    P_USTR ustr;

    if(work_value >= F64_MIN)
    {
        F64 log10_work_value = log10(work_value);
        F64 f64_exponent;
        F64 mantissa = modf(log10_work_value, &f64_exponent);
        S32 exponent = (S32) f64_exponent;

        /* allow for logs of numbers (especially those not in same base as FP representation) becoming imprecise */
#define LOG_SIG_EPS 1E-8

        /* NB. consider numbers such as log10(0.2) ~ -0.698 = (-1.0 exp) + (0.30103 man) */
        if(mantissa < 0.0)
        {
            mantissa += 1.0;
            exponent -= 1;
        }

        /* watch for logs going awry */
        if(mantissa < LOG_SIG_EPS)
            /* keep rounded down */
            mantissa = 0.0;

        else if((1.0 - mantissa) < LOG_SIG_EPS)
            /* round up */
        {
            mantissa = 0.0;
            exponent += 1;
        }

        remainder = exponent % 3;

        if(remainder < 0)
            remainder += 3;

        exponent -= remainder;

        switch(exponent)
        {
        /* NB X-U and x-u are the Jeff K. Aronson 1990s proposal and are vaguely sensible! */
        case +36: p_numform_info->ustr_engineering_section = USTR_TEXT("U"); break; /* udeka */
        case +33: p_numform_info->ustr_engineering_section = USTR_TEXT("V"); break; /* vendeka */
        case +30: p_numform_info->ustr_engineering_section = USTR_TEXT("W"); break; /* wekta */
        case +27: p_numform_info->ustr_engineering_section = USTR_TEXT("X"); break; /* xenta */
        case +24: p_numform_info->ustr_engineering_section = USTR_TEXT("Y"); break; /* yotta */
        case +21: p_numform_info->ustr_engineering_section = USTR_TEXT("Z"); break; /* zetta */
        case +18: p_numform_info->ustr_engineering_section = USTR_TEXT("E"); break; /* exa */
        case +15: p_numform_info->ustr_engineering_section = USTR_TEXT("P"); break; /* peta */
        case +12: p_numform_info->ustr_engineering_section = USTR_TEXT("T"); break; /* tera */
        case  +9: p_numform_info->ustr_engineering_section = USTR_TEXT("G"); break; /* giga */
        case  +6: p_numform_info->ustr_engineering_section = USTR_TEXT("M"); break; /* mega */
        case  +3: p_numform_info->ustr_engineering_section = USTR_TEXT("k"); break; /* kilo */
        case   0: break;
        case  -3: p_numform_info->ustr_engineering_section = USTR_TEXT("m"); break; /* milli */
        case  -6: p_numform_info->ustr_engineering_section = USTR_TEXT("u"); break; /* micro */
        case  -9: p_numform_info->ustr_engineering_section = USTR_TEXT("n"); break; /* nano */
        case -12: p_numform_info->ustr_engineering_section = USTR_TEXT("p"); break; /* pico */
        case -15: p_numform_info->ustr_engineering_section = USTR_TEXT("f"); break; /* femto */
        case -18: p_numform_info->ustr_engineering_section = USTR_TEXT("a"); break; /* atto */
        case -21: p_numform_info->ustr_engineering_section = USTR_TEXT("z"); break; /* zepto */
        case -24: p_numform_info->ustr_engineering_section = USTR_TEXT("y"); break; /* yocto */
        case -27: p_numform_info->ustr_engineering_section = USTR_TEXT("x"); break; /* xenno */
        case -30: p_numform_info->ustr_engineering_section = USTR_TEXT("w"); break; /* weko */
        case -33: p_numform_info->ustr_engineering_section = USTR_TEXT("v"); break; /* vendeko */
        /*case -36: p_numform_info->ustr_engineering_section = USTR_TEXT("uu"); break;*/ /* udeko */

        default:
            {
            static UCHARZ exponent_buffer[16];
            consume_int(xsnprintf(exponent_buffer, sizeof32(exponent_buffer),
                                  "%c" S32_FMT,
                                  (p_numform_info->engineering == 'g') ? 'e': 'E', exponent));
            p_numform_info->ustr_engineering_section = ustr_bptr(exponent_buffer);
            break;
            }
        }

        work_value = pow(10.0, remainder + mantissa);
    }

    decimal_places = (p_numform_info->integer_places_format + p_numform_info->decimal_places_format - 1) /*- remainder*/;

    consume_int(ustr_xsnprintf(p_numform_info->ustr_integer_section, p_numform_info->elemof_integer_section,
                               USTR_TEXT("%#.*f"), /* NB # flag forces f conversion to always have decimal point */
                               (int) decimal_places, work_value));
    trace_1(TRACE_MODULE_NUMFORM, TEXT("numform engfmt : %s"), report_ustr(p_numform_info->ustr_integer_section));

    ustr = p_numform_info->ustr_integer_section;

    while(ustr_GetByteInc_wr(ustr) != CH_FULL_STOP)
    { /*EMPTY*/ }
    p_numform_info->ustr_decimal_section = ustr;

    ustr_DecByte_wr(ustr);
    PtrPutByte(ustr, CH_NULL); /* terminate integer section */
    p_numform_info->integer_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_integer_section);

    /* strip trailing zeros behind dp */
    ustr_IncByte_wr(ustr); /* skip dp */
    ustr_IncBytes_wr(ustr, ustrlen32(ustr));
    while(PtrGetByteOff(ustr, -1) == CH_DIGIT_ZERO)
        ustr_DecByte_wr(ustr);
    PtrPutByte(ustr, CH_NULL); /* terminate decimal section */
    p_numform_info->decimal_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_decimal_section);

    trace_4(TRACE_MODULE_NUMFORM, TEXT("numform engfmt > %s ") S32_TFMT TEXT("; %s ") S32_TFMT,
            report_ustr(p_numform_info->ustr_integer_section), p_numform_info->integer_places_actual,
            report_ustr(p_numform_info->ustr_decimal_section), p_numform_info->decimal_places_actual);
}

/******************************************************************************
*
* Output exponential number to num_buf
* Must also set integer_places_actual
*
******************************************************************************/

static void
convert_number_exponential(
    P_NUMFORM_INFO p_numform_info)
{
    F64 work_value = ss_data_get_number(&p_numform_info->ss_data);
    P_USTR ustr;

    consume_int(ustr_xsnprintf(p_numform_info->ustr_integer_section, p_numform_info->elemof_integer_section,
                               USTR_TEXT("%#.*e"), /* NB # flag forces e conversion to always have decimal point */
                               (int) p_numform_info->decimal_places_format, work_value));
    trace_1(TRACE_MODULE_NUMFORM, TEXT("numform expfmt : %s"), report_ustr(p_numform_info->ustr_integer_section));

    ustr = p_numform_info->ustr_integer_section;

    while(PtrGetByte(ustr) != CH_FULL_STOP)
        ustr_IncByte_wr(ustr);
    p_numform_info->ustr_decimal_section = ustr_AddBytes_wr(ustr, 1);
    PtrPutByte(ustr, CH_NULL); /* terminate integer section */
    p_numform_info->integer_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_integer_section);

    assert(p_numform_info->integer_places_actual == 1);

    /* strip trailing zeros behind dp and before 'e' */
    /*ustr_IncByte_wr(ustr);*/ /* skip dp (loop will do this anyhow) */
    while(PtrGetByte(ustr) != 'e')
        ustr_IncByte_wr(ustr);
    p_numform_info->ustr_exponent_section = ustr_AddBytes_wr(ustr, 1); /* point to exp sign */

    while(PtrGetByteOff(ustr, -1) == CH_DIGIT_ZERO)
        ustr_DecByte_wr(ustr);

    PtrPutByte(ustr, CH_NULL); /* terminate decimal section, overwriting 'e' or a CH_DIGIT_ZERO */
    p_numform_info->decimal_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_decimal_section);

    /* strip sign and leading zero(es) from exponent */
    p_numform_info->exponent_sign_actual = ustr_GetByteInc_wr(p_numform_info->ustr_exponent_section);
    assert((p_numform_info->exponent_sign_actual == CH_PLUS_SIGN) || (p_numform_info->exponent_sign_actual == CH_MINUS_SIGN__BASIC));

    ustr = p_numform_info->ustr_exponent_section;
    while(PtrGetByte(ustr) == CH_DIGIT_ZERO)
    {
        /* don't strip last zero? */
        if(CH_NULL == PtrGetByteOff(ustr, 1))
            break;

        ustr_IncByte_wr(ustr);
    }
    p_numform_info->ustr_exponent_section = ustr;

    p_numform_info->exponent_places_actual = ustrlen32(p_numform_info->ustr_exponent_section);

    trace_6(TRACE_MODULE_NUMFORM, TEXT("numform expfmt > %s ") S32_TFMT TEXT("; %s ") S32_TFMT TEXT("; %s ") S32_TFMT,
            report_ustr(p_numform_info->ustr_integer_section), p_numform_info->integer_places_actual,
            report_ustr(p_numform_info->ustr_decimal_section), p_numform_info->decimal_places_actual,
            report_ustr(p_numform_info->ustr_exponent_section), p_numform_info->exponent_places_actual);
}

/******************************************************************************
*
* Output roman numerals to num_buf
* Must also set integer_places_actual
*
******************************************************************************/

_Check_return_
static BOOL
convert_number_roman(
    P_NUMFORM_INFO p_numform_info)
{
    static const U8 roman_numerals_lc[] = { 'i', 'v', 'x', 'l', 'c', 'd', 'm', '-', '-' };
    static const U8 roman_numerals_uc[] = { 'I', 'V', 'X', 'L', 'C', 'D', 'M', '-', '-' };

    PC_U8 roman_numerals = (p_numform_info->roman == 'R') ? roman_numerals_uc : roman_numerals_lc;
    U8Z num_string[9];
    P_U8Z p_num_string = num_string;
    S32 digit = 0;
    P_USTR ustr;

    /* bounds check argument (always positive) for given output format */
    assert(ss_data_is_number(&p_numform_info->ss_data));
    if(ss_data_is_real(&p_numform_info->ss_data))
    {
        if(ss_data_get_real(&p_numform_info->ss_data) > (F64) ROMAN_MAX)
            p_numform_info->roman = 0;
    }
    else
    {
        if(ss_data_get_integer(&p_numform_info->ss_data) > ROMAN_MAX)
            p_numform_info->roman = 0;
    }

    if(!p_numform_info->roman)
        return(FALSE);

    if(ss_data_is_real(&p_numform_info->ss_data))
        digit = xsnprintf(num_string, sizeof32(num_string),
                          "%.0f",  ss_data_get_real(&p_numform_info->ss_data)); /* precision of 0 -> no decimal point */
    else
        digit = xsnprintf(num_string, sizeof32(num_string),
                          S32_FMT, ss_data_get_integer(&p_numform_info->ss_data));

    p_num_string = num_string;
    ustr = p_numform_info->ustr_integer_section;

    while(--digit >= 0)
        ustr_IncBytes_wr(ustr, convert_digit_roman(ustr, *p_num_string++, &roman_numerals[digit * 2]));

    p_numform_info->integer_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_integer_section);

    p_numform_info->ustr_decimal_section = ustr_AddBytes(p_numform_info->ustr_integer_section, p_numform_info->integer_places_actual); /* point to CH_NULL */

    trace_4(TRACE_MODULE_NUMFORM, TEXT("numform rmnfmt > %s ") S32_TFMT TEXT("; %s ") S32_TFMT,
            report_ustr(p_numform_info->ustr_integer_section), p_numform_info->integer_places_actual,
            report_ustr(p_numform_info->ustr_decimal_section), p_numform_info->decimal_places_actual);

    return(TRUE);
}

/******************************************************************************
*
* Output spreadsheet number to num_buf
* Must also set integer_places_actual
*
******************************************************************************/

_Check_return_
static BOOL
convert_number_spreadsheet(
    P_NUMFORM_INFO p_numform_info)
{
    S32 work_value;

    /* bounds check argument (always positive) for given output format */
    assert(ss_data_is_number(&p_numform_info->ss_data));
    if(ss_data_is_real(&p_numform_info->ss_data))
    {
        if(ss_data_get_real(&p_numform_info->ss_data) > (F64) S32_MAX)
            p_numform_info->spreadsheet = 0;
        else
            ss_data_set_integer(&p_numform_info->ss_data, (S32) real_floor(ss_data_get_real(&p_numform_info->ss_data) + 0.5)); /* round prior to force */ /* already range checked so should convert OK */
    }

    /* all integer args are representable in spreadsheet format */

    if(!p_numform_info->spreadsheet)
        return(FALSE);

    work_value = ss_data_get_integer(&p_numform_info->ss_data);

    p_numform_info->integer_places_actual = xtos_ustr_buf(p_numform_info->ustr_integer_section, p_numform_info->elemof_integer_section, work_value - 1, (p_numform_info->spreadsheet == 'X'));

    p_numform_info->ustr_decimal_section = ustr_AddBytes(p_numform_info->ustr_integer_section, p_numform_info->integer_places_actual); /* point to CH_NULL */

    return(TRUE);
}

/******************************************************************************
*
* lookup value in table
*
******************************************************************************/

static void
convert_number_lookup(
    P_NUMFORM_INFO p_numform_info,
    P_USTR p_buffer)
{
    U8 ch;
    S32 index = 0;
    P_USTR ustr_string = p_numform_info->ustr_lookup_section;
    P_USTR ustr_last_string = ustr_string;
    S32 work_value;

    PTR_ASSERT(ustr_string);

    work_value = ss_data_is_real(&p_numform_info->ss_data) ? (S32) (ss_data_get_real(&p_numform_info->ss_data) + 0.5) : ss_data_get_integer(&p_numform_info->ss_data);

    while((index < work_value) && ((ch = ustr_GetByteInc_wr(ustr_string)) != CH_RIGHT_CURLY_BRACKET) && (ch != CH_NULL))
    {
        if(ch == LIST_SEPARATOR)
        {
            index++;
            ustr_last_string = ustr_string;
        }
        if(ch == CH_LEFT_SQUARE_BRACKET) /* skip style names */
        {
            while(((ch = ustr_GetByteInc_wr(ustr_string)) != CH_RIGHT_SQUARE_BRACKET) && (ch != CH_RIGHT_CURLY_BRACKET) && (ch != CH_NULL))
            { /*EMPTY*/ }
            if(ch != CH_RIGHT_SQUARE_BRACKET)
                ustr_DecByte_wr(ustr_string); /* point back to terminator */
        }
    }
    ustr_string = p_numform_info->ustr_integer_section;
    while(((ch = ustr_GetByteInc_wr(ustr_last_string)) != LIST_SEPARATOR) && (ch != CH_RIGHT_CURLY_BRACKET) && (ch != CH_NULL))
    {
        if(ch == CH_LEFT_SQUARE_BRACKET)
        {
            p_numform_info->ustr_style_name = ustr_last_string;
            while(((ch = ustr_GetByteInc_wr(ustr_last_string)) != CH_RIGHT_SQUARE_BRACKET) && (ch != CH_RIGHT_CURLY_BRACKET) && (ch != CH_NULL) && (ch != LIST_SEPARATOR))
            { /*EMPTY*/ }
            if(ch != CH_RIGHT_SQUARE_BRACKET)
                ustr_DecByte_wr(ustr_last_string);
        }
        else
        {
            PtrPutByte(ustr_string,  ch);
            ustr_IncByte_wr(ustr_string);
        }
    }
    PtrPutByte(ustr_string, CH_NULL);

    p_numform_info->integer_places_actual = ustrlen32(p_numform_info->ustr_integer_section);

    p_numform_info->ustr_decimal_section = ustr_AddBytes(p_numform_info->ustr_integer_section, p_numform_info->integer_places_actual); /* point to CH_NULL */

    if(CH_NULL == PtrGetByte(p_buffer))
    {
        PtrPutByteOff(p_buffer, 0, CH_NUMBER_SIGN);
        PtrPutByteOff(p_buffer, 1, CH_NULL);
    }
}

/******************************************************************************
*
* Output standard number to num_buf
* Must also set integer_places_actual
*
******************************************************************************/

static void
convert_number_standard(
    P_NUMFORM_INFO p_numform_info)
{
    if(ss_data_is_real(&p_numform_info->ss_data))
    {
        P_USTR ustr;

        consume_int(ustr_xsnprintf(p_numform_info->ustr_integer_section, p_numform_info->elemof_integer_section,
                                   USTR_TEXT("%#.*f"), /* NB # flag forces f conversion to always have decimal point */
                                   (int) p_numform_info->decimal_places_format, ss_data_get_real(&p_numform_info->ss_data)));
        trace_1(TRACE_MODULE_NUMFORM, TEXT("numform stdfmt(f) : %s"), report_ustr(p_numform_info->ustr_integer_section));

        if(PtrGetByte(p_numform_info->ustr_integer_section) == CH_DIGIT_ZERO)
            ustr_IncByte_wr(p_numform_info->ustr_integer_section);

        ustr = p_numform_info->ustr_integer_section;

        while(PtrGetByte(ustr) != CH_FULL_STOP)
            ustr_IncByte_wr(ustr);
        p_numform_info->ustr_decimal_section = ustr_AddBytes_wr(ustr, 1);
        PtrPutByte(ustr, CH_NULL); /* terminate integer section */
        p_numform_info->integer_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_integer_section);

        /* strip trailing zeros behind dp */
        ustr_IncByte_wr(ustr); /* skip dp */
        ustr_IncBytes_wr(ustr, ustrlen32(ustr));
        while(PtrGetByteOff(ustr, -1) == CH_DIGIT_ZERO)
            ustr_DecByte_wr(ustr);

        while(PtrDiffBytesS32(ustr, p_numform_info->ustr_decimal_section) > p_numform_info->decimal_places_format)
            ustr_DecByte_wr(ustr); /* converted too many places */

        PtrPutByte(ustr, CH_NULL); /* terminate decimal section */
        p_numform_info->decimal_places_actual = PtrDiffBytesS32(ustr, p_numform_info->ustr_decimal_section);
    }
    else
    {
        consume_int(ustr_xsnprintf(p_numform_info->ustr_integer_section, p_numform_info->elemof_integer_section,
                                   USTR_TEXT(S32_FMT),
                                   ss_data_get_integer(&p_numform_info->ss_data)));
        trace_1(TRACE_MODULE_NUMFORM, TEXT("numform stdfmt(i) : %s"), report_ustr(p_numform_info->ustr_integer_section));

        if(PtrGetByte(p_numform_info->ustr_integer_section) == CH_DIGIT_ZERO)
            ustr_IncByte_wr(p_numform_info->ustr_integer_section);

        p_numform_info->integer_places_actual = ustrlen32(p_numform_info->ustr_integer_section);

        p_numform_info->ustr_decimal_section = ustr_AddBytes(p_numform_info->ustr_integer_section, p_numform_info->integer_places_actual); /* point to CH_NULL */
    }

    trace_4(TRACE_MODULE_NUMFORM, TEXT("numform stdfmt > %s ") S32_TFMT TEXT("; %s ") S32_TFMT,
            report_ustr(p_numform_info->ustr_integer_section), p_numform_info->integer_places_actual,
            report_ustr(p_numform_info->ustr_decimal_section), p_numform_info->decimal_places_actual);
}

static void
convert_to_floating_point_hexadecimal(
    _InoutRef_  P_NUMFORM_INFO p_numform_info)
{
    F64 f64 = ss_data_get_number(&p_numform_info->ss_data);

    /* NB with DBL_MANT_DIG 53 there are 52 bits actually stored, divided by 4 -> 13 hex digits after the point */
    assert(DBL_MANT_DIG == 53);
    consume_int(ustr_xsnprintf(p_numform_info->ustr_integer_section, p_numform_info->elemof_integer_section,
                               ucs4_is_uppercase(p_numform_info->base_basechar) ? USTR_TEXT("%.13A") : USTR_TEXT("%.13a"),
                               f64));

    p_numform_info->integer_places_actual = ustrlen32(p_numform_info->ustr_integer_section);

    p_numform_info->ustr_decimal_section = ustr_AddBytes(p_numform_info->ustr_integer_section, p_numform_info->integer_places_actual); /* point to CH_NULL */

    trace_4(TRACE_MODULE_NUMFORM, TEXT("numform hexfmt > %s ") S32_TFMT TEXT("; %s ") S32_TFMT,
            report_ustr(p_numform_info->ustr_integer_section), p_numform_info->integer_places_actual,
            report_ustr(p_numform_info->ustr_decimal_section), p_numform_info->decimal_places_actual);
}

_Check_return_
static inline U8
base_character(
    _InVal_     S32 digit_in_base,
    _InVal_     U8 base_char)
{
    assert(digit_in_base >= 0);

    if(digit_in_base < 10)
        return((U8) (CH_DIGIT_ZERO + digit_in_base));

    return((U8) (base_char + digit_in_base - 10));
}

static void
convert_number_base(
    _InoutRef_  P_NUMFORM_INFO p_numform_info)
{
    U8Z buffer[1024+4]; /* should cover all numbers! (consider DBL_MAX in base 2) */
    P_U8Z p_buffer = buffer + elemof32(buffer);

    if(S32_MIN == p_numform_info->base)
    {
        convert_to_floating_point_hexadecimal(p_numform_info);
        return;
    }

    *--p_buffer = CH_NULL; /* digits are output in reverse order, appearing in buffer in the correct order */

    if(ss_data_is_real(&p_numform_info->ss_data))
    {
        F64 work_value = real_floor(ss_data_get_real(&p_numform_info->ss_data));

        do  { /* consider zero */
            const S32 digit_in_base = (S32) fmod(work_value, p_numform_info->base);
            work_value = work_value / p_numform_info->base;
            *--p_buffer = base_character(digit_in_base, p_numform_info->base_basechar);
            assert(buffer != p_buffer);
        }
        while(work_value >= 1.0);
    }
    else
    {
        S32 work_value = ss_data_get_integer(&p_numform_info->ss_data);

        do  { /* consider zero */
            const S32 digit_in_base = work_value % p_numform_info->base;
            work_value = work_value / p_numform_info->base;
            *--p_buffer = base_character(digit_in_base, p_numform_info->base_basechar);
            assert(buffer != p_buffer);
        }
        while(work_value >= 1);
    }

    /* copy the accumulated digits to the output */
    ustr_xstrkpy(p_numform_info->ustr_integer_section, p_numform_info->elemof_integer_section, p_buffer);

    if(PtrGetByte(p_numform_info->ustr_integer_section) == CH_DIGIT_ZERO)
        ustr_IncByte_wr(p_numform_info->ustr_integer_section);

    p_numform_info->integer_places_actual = ustrlen32(p_numform_info->ustr_integer_section);

    p_numform_info->ustr_decimal_section = ustr_AddBytes(p_numform_info->ustr_integer_section, p_numform_info->integer_places_actual); /* point to CH_NULL */

    trace_4(TRACE_MODULE_NUMFORM, TEXT("numform basfmt > %s ") S32_TFMT TEXT("; %s ") S32_TFMT,
            report_ustr(p_numform_info->ustr_integer_section), p_numform_info->integer_places_actual,
            report_ustr(p_numform_info->ustr_decimal_section), p_numform_info->decimal_places_actual);
}

/*ncr*/
static BOOL
check_real_nan_inf(
    _InoutRef_  P_NUMFORM_INFO p_numform_info)
{
    F64 f64;

    assert(ss_data_is_real(&p_numform_info->ss_data));
    f64 = ss_data_get_real(&p_numform_info->ss_data);

    if(!isfinite(f64)) /* test for NaN and infinity */
    {
        const F64 test = copysign(1.0, f64); /* try to handle -nan and -inf */

        if(test < 0.0)
            p_numform_info->number.negative = 1;

        if(isnan(f64))
            p_numform_info->number.nan = 1;
        else
            p_numform_info->number.infinity = 1;

        p_numform_info->ss_data = ss_data_real_zero;

        return(TRUE);
    }

    return(FALSE);
}

static void
quickly_check_numform_numeric_for_date(
    _InoutRef_  P_NUMFORM_INFO p_numform_info,
    _In_z_      PC_USTR ustr_numform_numeric)
{
    F64 f64;
    SS_DATE ss_date;

    if(NULL == ustr_numform_numeric)
        return; /* e.g. during autoformat() */

    /* just test the first character */
    switch(sbchar_tolower(PtrGetByte(ustr_numform_numeric)))
    {
    default:
        return;

    case 'd':
    case 'h':
    case 'm':
    case 'n':
    case 's':
    case 'y':
        /* try to mutate to a date then! */
        break;
    }

    f64 = ss_data_get_number(&p_numform_info->ss_data);

    if(status_ok(ss_serial_number_to_date(&ss_date, f64)))
        ss_data_set_date(&p_numform_info->ss_data, ss_date.date, ss_date.time);
}

/******************************************************************************

Recognised Symbols:

\    - next character literal
".." - literal text
;    - Section separator
-:() - quotes unnecessary (also CH_FULL_STOP and CH_COMMA in non-numeric fields)
#    - Digit, nothing if absent
0    - Digit, zero if absent
_    - Digit, space if absent
.    - decimal point indicator (next char)
,    - thousands sep indicator (next char)
eE   - exponent (followed by +, - or nothing)
<    - shift left (* 1000)
>    - shift right (/ 1000)
%    - % char and *100
gG   - Engineering notation (afpnum.kMGTPE)
rR   - Roman numerals (if < 4999)
xX   - Spreadsheet (1 -> A etc.)
hH   - hours
nN   - minutes
sS   - seconds
dD   - days
mM   - months, unless immediately after hH, then minutes
yY   - years
@    - Text / error
[]   - encloses a style name
{}   - lookup list, starting from 0
bB   - base number
******************************************************************************/

static const NUMFORM_CONTEXT
default_numform_context =
{
    {
    USTR_TEXT("january"),
    USTR_TEXT("february"),
    USTR_TEXT("march"),
    USTR_TEXT("april"),
    USTR_TEXT("may"),
    USTR_TEXT("june"),
    USTR_TEXT("july"),
    USTR_TEXT("august"),
    USTR_TEXT("september"),
    USTR_TEXT("october"),
    USTR_TEXT("november"),
    USTR_TEXT("december")
    },

    {
    USTR_TEXT("sunday"),
    USTR_TEXT("monday"),
    USTR_TEXT("tuesday"),
    USTR_TEXT("wednesday"),
    USTR_TEXT("thursday"),
    USTR_TEXT("friday"),
    USTR_TEXT("saturday")
    },

    {
/*              0                1                2                3                4                5                6                7                8                9 */
                     USTR_TEXT("st"), USTR_TEXT("nd"), USTR_TEXT("rd"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"),
    USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"),
    USTR_TEXT("th"), USTR_TEXT("st"), USTR_TEXT("nd"), USTR_TEXT("rd"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"), USTR_TEXT("th"),
    USTR_TEXT("th"), USTR_TEXT("st")
    }
};

_Check_return_
extern STATUS
numform(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _InoutRef_opt_ P_QUICK_TBLOCK p_quick_tblock_style /*appended,terminated*/,
    _InRef_     PC_SS_DATA p_ss_data,
    _InRef_     PC_NUMFORM_PARMS p_numform_parms)
{
    PC_NUMFORM_CONTEXT p_numform_context = p_numform_parms->p_numform_context;
    NUMFORM_INFO numform_info = { 0 };
    UCHARZ own_numform[BUF_SECTION_MAX]; /* buffer into which numform section is extracted */
#if 1 /* consider DBL_MAX in base 2 */
    UCHARZ num_ustr_buf[1 /*sign*/ + (1+DBL_MAX_EXP) /*digits*/ + 1 /*paranoia*/ + 1 /*CH_NULL*/]; /* Contains the number to be output */
#else
    UCHARZ num_ustr_buf[1 /*sign*/ + (1+DBL_MAX_10_EXP) /*digits*/ + 1 /*paranoia*/ + 1 /*CH_NULL*/]; /* Contains the number to be output */
#endif

    if(IS_P_DATA_NONE(p_numform_context))
        p_numform_context = &default_numform_context;

    numform_info.p_quick_ublock = p_quick_ublock;
    numform_info.ustr_style_name = NULL;

    /* Get a hard copy of the ss_data */
    numform_info.ss_data = *p_ss_data;

    /* just show array tl */
    if(ss_data_is_array(&numform_info.ss_data))
        ss_array_element_read(&numform_info.ss_data, &numform_info.ss_data, 0, 0);

    if(ss_data_is_number(&numform_info.ss_data))
        quickly_check_numform_numeric_for_date(&numform_info, p_numform_parms->ustr_numform_numeric);

    /* note conversion type */
    numform_info.type = ss_data_get_data_id(&numform_info.ss_data);

    /*numform_info.negative = 0; already clear */
    /*numform_info.zero = 0;*/

    own_numform[0] = CH_NULL;

    switch(ss_data_get_data_id(&numform_info.ss_data))
    {
    case DATA_ID_REAL:
        if(check_real_nan_inf(&numform_info))
        {   /*EMPTY*/ /*nan/infinity*/
        }
        else if(ss_data_get_real(&numform_info.ss_data) == 0.0)
        {
            numform_info.ss_data = ss_data_real_zero; /* NB We DO get -0.0 !!! which screws up rest of printing code ... */
            numform_info.number.zero = 1;
        }
        else if(ss_data_get_real(&numform_info.ss_data) < 0.0)
        {
            ss_data_set_real(&numform_info.ss_data, 0.0 - ss_data_get_real(&numform_info.ss_data));
            numform_info.number.negative = 1;
        }

        if(!IS_PTR_NULL_OR_NONE(p_numform_parms->ustr_numform_numeric))
            numform_section_extract_numeric(&numform_info, ustr_bptr(own_numform), sizeof32(own_numform), p_numform_parms->ustr_numform_numeric, ustr_bptr(num_ustr_buf), sizeof32(num_ustr_buf));
        break;

    case DATA_ID_LOGICAL:
        /*break;*/

    case DATA_ID_WORD8:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        numform_info.type = DATA_ID_REAL;

        if(ss_data_get_integer(&numform_info.ss_data) == 0)
        {
            numform_info.number.zero = 1;
        }
        else if(ss_data_get_integer(&numform_info.ss_data) < 0)
        {
            ss_data_set_integer(&numform_info.ss_data, 0 - ss_data_get_integer(&numform_info.ss_data));
            assert(ss_data_get_integer(&numform_info.ss_data) > 0); /* watch out for S32_MIN - that should never have got into an integer value */
            numform_info.number.negative = 1;
        }

        if(!IS_PTR_NULL_OR_NONE(p_numform_parms->ustr_numform_numeric))
            numform_section_extract_numeric(&numform_info, ustr_bptr(own_numform), sizeof32(own_numform), p_numform_parms->ustr_numform_numeric, ustr_bptr(num_ustr_buf), sizeof32(num_ustr_buf));
        break;

    case DATA_ID_DATE:
        numform_output_datetime_last_field = CH_NULL;

        numform_info.date.valid = status_ok(ss_dateval_to_ymd(ss_data_get_date(&numform_info.ss_data)->date, &numform_info.date.year,  &numform_info.date.month,   &numform_info.date.day));
        numform_info.time.valid = status_ok(ss_timeval_to_hms(ss_data_get_date(&numform_info.ss_data)->time, &numform_info.time.hours, &numform_info.time.minutes, &numform_info.time.seconds));

        if(!IS_PTR_NULL_OR_NONE(p_numform_parms->ustr_numform_datetime))
            numform_section_extract_datetime(&numform_info, ustr_bptr(own_numform), sizeof32(own_numform), p_numform_parms->ustr_numform_datetime);
        break;

    default:
#if CHECKING
        default_unhandled();
        /*FALLTHRU*/
    case RPN_DAT_NEXT_NUMBER:
        assert0();

        /*FALLTHRU*/

    case DATA_ID_STRING:
    case DATA_ID_BLANK:
    case DATA_ID_ERROR:
#endif
        numform_info.type = DATA_ID_STRING;

        if(!IS_PTR_NULL_OR_NONE(p_numform_parms->ustr_numform_texterror))
            numform_section_extract_texterror(&numform_info, ustr_bptr(own_numform), sizeof32(own_numform), p_numform_parms->ustr_numform_texterror);
        break;

    case DATA_ID_ARRAY:
        return(create_error(EVAL_ERR_UNEXARRAY));
    }

    status_return(numform_output(p_numform_context, &numform_info, ustr_bptr(own_numform)));

    /* get style name if requested and if one exists */
    if(P_QUICK_TBLOCK_NONE != p_quick_tblock_style)
    {
        if(NULL != numform_info.ustr_style_name)
            status_return(numform_output_style_name(p_quick_tblock_style, numform_info.ustr_style_name));
        else
            status_return(quick_tblock_nullch_add(p_quick_tblock_style));
    }

    return(STATUS_OK);
}

static S32
numform_numeric_section_copy_and_parse(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_section)
{
    const P_U8 buffer = (P_U8) ustr_buf;
    U32 dst_idx = 0;
    S32 shift_value = 0;
    U8 ch;

    assert(0 != elemof_buffer); /* needs CH_NULL-termination */

    p_numform_info->integer_places_format  = 0;
    p_numform_info->decimal_places_format  = 0;
    p_numform_info->exponent_places_format = 0;

    p_numform_info->integer_places_actual  = 0;
    p_numform_info->decimal_places_actual  = 0;
    p_numform_info->exponent_places_actual = 0;

    p_numform_info->exponent_sign          = CH_PLUS_SIGN;
    p_numform_info->thousands_sep          = 0;
    p_numform_info->decimal_pt             = 0;

    p_numform_info->engineering            = 0;
    p_numform_info->exponential            = 0;
    p_numform_info->roman                  = 0;
    p_numform_info->spreadsheet            = 0;
    p_numform_info->base                   = 0;

    p_numform_info->decimal_section_has_hash = 0;
    p_numform_info->decimal_section_has_zero = 0;
    p_numform_info->decimal_section_spaces   = 0;

    p_numform_info->integer_section_result_output = 0;

    p_numform_info->ustr_engineering_section = NULL;
    p_numform_info->ustr_lookup_section = NULL;
    p_numform_info->ustr_style_name = NULL;

    for(;;)
    {
        ch = ustr_GetByteInc(ustr_numform_section);

        switch(ch)
        {
        case CH_QUOTATION_MARK:
            for(;;)
            {
                if(dst_idx >= elemof_buffer) break;
                buffer[dst_idx++] = ch;

                ch = ustr_GetByteInc(ustr_numform_section);

                if((CH_QUOTATION_MARK == ch) || (CH_NULL == ch))
                    break;
            }

            if(CH_NULL == ch)
                break;

            if(dst_idx >= elemof_buffer) break;
            buffer[dst_idx++] = ch;
            break;

        case CH_BACKWARDS_SLASH:
            if(dst_idx >= elemof_buffer) break;
            buffer[dst_idx++] = ch;

            ch = ustr_GetByteInc(ustr_numform_section);

            if(CH_NULL == ch)
                break;

            if(dst_idx >= elemof_buffer) break;
            buffer[dst_idx++] = ch;
            break;

        case CH_LEFT_CURLY_BRACKET:
            p_numform_info->ustr_lookup_section = de_const_cast(P_USTR, ustr_numform_section);

            for(;;)
            {
                ch = ustr_GetByteInc(ustr_numform_section);

                if((CH_RIGHT_CURLY_BRACKET == ch) || (CH_NULL == ch))
                    break;
            }

            /* may be CH_NULL */
            break;

        case CH_LEFT_SQUARE_BRACKET:
            p_numform_info->ustr_style_name = de_const_cast(P_USTR, ustr_numform_section);

            for(;;)
            {
                ch = ustr_GetByteInc(ustr_numform_section);

                if((CH_RIGHT_SQUARE_BRACKET == ch) || (CH_NULL == ch))
                    break;
            }

            /* may be CH_NULL */
            break;

        case CH_FULL_STOP:
        case CH_COMMA:
            {
            P_U8 p_char = (CH_FULL_STOP == ch)
                        ? &p_numform_info->decimal_pt
                        : &p_numform_info->thousands_sep;

            if(CH_NULL == *p_char)
            {
                if(CH_FULL_STOP == ch)
                {
                    if(dst_idx >= elemof_buffer) break;
                    buffer[dst_idx++] = ch;
                }

                switch(PtrGetByte(ustr_numform_section))
                {
                case CH_NULL:
                case CH_UNDERSCORE:
                case CH_QUESTION_MARK: /* as Excel */
                case CH_DIGIT_ZERO:
                case CH_NUMBER_SIGN:
                case CH_QUOTATION_MARK:
                    /* dot/comma followed by one of these chars specifies default decimal point/thousands sep char */
                    *p_char = ch;
                    break;

                default:
                    /* dot/comma followed by char specifies decimal point/thousands sep char */
                    *p_char = ustr_GetByteInc(ustr_numform_section);
                    break;
                }
            }

            break;
            }

        case CH_UNDERSCORE:
        case CH_QUESTION_MARK: /* as Excel */
            if(dst_idx >= elemof_buffer) break;
            buffer[dst_idx++] = ch;

            if(p_numform_info->exponential)
                p_numform_info->exponent_section_spaces += 1;
            else if(p_numform_info->decimal_pt)
                p_numform_info->decimal_section_spaces += 1;
            else
                p_numform_info->integer_places_format++;

            break;

        case CH_DIGIT_ZERO:
            if(dst_idx >= elemof_buffer) break;
            buffer[dst_idx++] = ch;

            if(p_numform_info->exponential)
                p_numform_info->exponent_places_format++;
            else if(p_numform_info->decimal_pt)
            {
                p_numform_info->decimal_places_format++;
                p_numform_info->decimal_section_has_zero = 1;
            }
            else
                p_numform_info->integer_places_format++;

            break;

        case CH_NUMBER_SIGN:
            if(dst_idx >= elemof_buffer) break;
            buffer[dst_idx++] = ch;

            if(p_numform_info->exponential)
                p_numform_info->exponent_places_format++;
            else if(p_numform_info->decimal_pt)
            {
                p_numform_info->decimal_places_format++;
                p_numform_info->decimal_section_has_hash = 1;
            }
            else
                p_numform_info->integer_places_format++;

            break;

        case 'e':
        case 'E':
            p_numform_info->exponential = ch;

            ch = ustr_GetByteInc(ustr_numform_section);
            if((ch == CH_PLUS_SIGN) || (ch == CH_MINUS_SIGN__BASIC))
                p_numform_info->exponent_sign = ch;
            else
                ustr_DecByte(ustr_numform_section);
            break;

        case 'a':
        case 'A':
            {
            p_numform_info->base = S32_MIN;
            break;
            }

        case 'b':
        case 'B':
            {
            p_numform_info->base_basechar = (U8) (ch - 1); /* a or A */

            if(!sbchar_isdigit(PtrGetByte(ustr_numform_section)))
                p_numform_info->base = 0;  /* turn conversion off again */
            else
            {
                p_numform_info->base = fast_ustrtoul(ustr_numform_section, &ustr_numform_section);

                if((p_numform_info->base < 2) || (p_numform_info->base > 36))
                    p_numform_info->base = 0;
            }
            break;
            }

        case 'g':
        case 'G':
            p_numform_info->engineering = ch;
            break;

        case 'r':
        case 'R':
            p_numform_info->roman = ch;
            break;

        case 'x':
        case 'X':
            p_numform_info->spreadsheet = ch;
            break;

        case CH_LESS_THAN_SIGN: /* SKS 19dec94 removed dross code */
            shift_value += 3;
            break;

        case CH_GREATER_THAN_SIGN:
            shift_value -= 3;
            break;

        case CH_PERCENT_SIGN:
            shift_value += 2;
            if(dst_idx >= elemof_buffer) break;
            buffer[dst_idx++] = ch;
            break;

        case CH_SEMICOLON:
            ch = CH_NULL;
            break;

        case CH_NULL:
            break;

        default:
            if(dst_idx >= elemof_buffer) break;
            buffer[dst_idx++] = ch;
            break;
        }

        if(CH_NULL == ch)
            break;
    }

    /* CH_NULL-terminate */
    if(dst_idx >= elemof_buffer)
        dst_idx = elemof_buffer - 1;
    buffer[dst_idx++] = CH_NULL;

    if(!p_numform_info->decimal_pt)
        p_numform_info->decimal_pt = CH_FULL_STOP;

    trace_6(TRACE_MODULE_NUMFORM, TEXT("%c ths; ") S32_TFMT TEXT(" ip; %c dpc; ") S32_TFMT TEXT(" dp; ") S32_TFMT TEXT(" ep; %c es"),
            p_numform_info->thousands_sep ? p_numform_info->thousands_sep : CH_SPACE,
            p_numform_info->integer_places_format,
            p_numform_info->decimal_pt,
            p_numform_info->decimal_places_format,
            p_numform_info->exponent_places_format,
            p_numform_info->exponent_sign ? p_numform_info->exponent_sign : CH_SPACE);

    return(shift_value);
}

/* loop outputting chars from data under control of format */

_Check_return_
static STATUS
numform_output(
    _InRef_     PC_NUMFORM_CONTEXT p_numform_context,
    _InRef_     P_NUMFORM_INFO p_numform_info,
    _In_z_      PC_USTR ustr_numform_section)
{
    const P_QUICK_UBLOCK p_quick_ublock = p_numform_info->p_quick_ublock;
    U8 ch;

    for(;;)
    {
        ch = ustr_GetByteInc(ustr_numform_section);

        switch(ch)
        {
        case CH_QUOTATION_MARK:
            /* literal string */
            for(;;)
            {
                ch = ustr_GetByteInc(ustr_numform_section);

                if((CH_QUOTATION_MARK == ch) || (CH_NULL == ch))
                    break;

                status_return(quick_ublock_ucs4_add(p_quick_ublock, ch));
            }

            if(CH_NULL == ch)
                break;

            continue;

        case CH_BACKWARDS_SLASH:
            /* next char literal */
            ch = ustr_GetByteInc(ustr_numform_section);

            if(CH_NULL == ch)
                break;

            status_return(quick_ublock_ucs4_add(p_quick_ublock, ch));
            continue;

        /* Any characters to be recognised as literal text without backslash or double quotes */
        case CH_COMMA:
        case CH_COLON:
        case CH_LEFT_PARENTHESIS:
        case CH_RIGHT_PARENTHESIS:
        case CH_MINUS_SIGN__BASIC:
            status_return(quick_ublock_ucs4_add(p_quick_ublock, ch));
            continue;

        /* All other characters may be type-dependent */
        default:
            break;
        }

        if(CH_NULL == ch)
            break;

        switch(p_numform_info->type)
        {
        default: default_unhandled();
#if CHECKING
        case DATA_ID_REAL:
#endif
            switch(ch)
            {
            case CH_FULL_STOP:
            case CH_UNDERSCORE:
            case CH_QUESTION_MARK: /* as Excel */
            case CH_DIGIT_ZERO:
            case CH_NUMBER_SIGN:
                if(p_numform_info->number.negative)
                {
                    p_numform_info->number.negative = 0;

                    if(p_numform_info->number.insert_minus_sign)
                    {   /* for negative numbers, insert a minus sign in as needed just prior to the first bit of numeric output */
                        status_return(quick_ublock_a7char_add(p_quick_ublock, CH_MINUS_SIGN__BASIC));
                    }
                }

                status_return(numform_output_number_fields(p_numform_info, ch));

                /* nasty bodge to cope with engineering format giving us more after the decimal places than we bargained for */
                if(p_numform_info->ustr_engineering_section && !p_numform_info->decimal_places_format)
                {
                    status_return(quick_ublock_ustr_add(p_quick_ublock, p_numform_info->ustr_engineering_section));
                    p_numform_info->ustr_engineering_section = NULL;
                }

                continue;

            default:
                break;
            }

            break;

        case DATA_ID_DATE:
            ustr_DecByte(ustr_numform_section);
            status_return(numform_output_date_fields(p_numform_info, p_numform_context, &ustr_numform_section));
            continue;

        case DATA_ID_STRING:
            if(CH_COMMERCIAL_AT == ch)
            {
                if(ss_data_is_string(&p_numform_info->ss_data))
                {
                    status_return(quick_ublock_uchars_add(p_quick_ublock, ss_data_get_string(&p_numform_info->ss_data), ss_data_get_string_size(&p_numform_info->ss_data)));
                    continue;
                }

                if(ss_data_is_blank(&p_numform_info->ss_data))
                    continue;

                if(ss_data_is_error(&p_numform_info->ss_data))
                {
                    status_return(resource_lookup_quick_ublock(p_quick_ublock, p_numform_info->ss_data.arg.ss_error.status));
                    continue;
                }
            }

            break;
        }

        /* otherwise just append the character */
        status_return(quick_ublock_ucs4_add(p_quick_ublock, ch));
    }

    return(quick_ublock_nullch_add(p_quick_ublock));
}

/******************************************************************************
*
* Checks the next character in the passed numform_section for any
* characters relevant to time / date field processing
*
******************************************************************************/

_Check_return_
static STATUS
numform_output_invalid_date_field(
    P_QUICK_UBLOCK p_quick_ublock,
    int count)
{
    int i;

    for(i = 0; i < count; ++i)
        status_return(quick_ublock_a7char_add(p_quick_ublock, CH_ASTERISK));

    return(STATUS_OK);
}

_Check_return_
static STATUS
numform_output_date_fields(
    P_NUMFORM_INFO p_numform_info,
    _InRef_     PC_NUMFORM_CONTEXT p_numform_context,
    _InoutRef_  P_PC_USTR p_ustr_numform_section)
{
    P_QUICK_UBLOCK p_quick_ublock = p_numform_info->p_quick_ublock;
    PC_USTR ustr_numform_section = *p_ustr_numform_section;
    int count = 0;
    U8 section_ch, next_section_ch;
    S32 arg = 0; /* keep dataflower happy */

    STATUS status = STATUS_OK;

    switch(section_ch = ustr_GetByteInc(ustr_numform_section))
    {
    default:
        *p_ustr_numform_section = ustr_numform_section;
        return(quick_ublock_ucs4_add(p_quick_ublock, section_ch)); /* don't set last_field */

    case 'h':
    case 'H':
        do { ++count; } while(((next_section_ch = ustr_GetByteInc(ustr_numform_section)) == 'h') || (next_section_ch == 'H'));
        ustr_DecByte(ustr_numform_section);

        arg = p_numform_info->time.hours;

        status = quick_ublock_printf(p_quick_ublock, USTR_TEXT("%.*" S32_FMT_POSTFIX), count, arg);
        break;

    case 'n':
    case 'N':
        do { ++count; } while(((next_section_ch = ustr_GetByteInc(ustr_numform_section)) == 'n') || (next_section_ch == 'N'));
        ustr_DecByte(ustr_numform_section);

        arg = p_numform_info->time.minutes;

        status = quick_ublock_printf(p_quick_ublock, USTR_TEXT("%.*" S32_FMT_POSTFIX), count, arg);
        break;

    case 's':
    case 'S':
        do { ++count; } while(((next_section_ch = ustr_GetByteInc(ustr_numform_section)) == 's') || (next_section_ch == 'S'));
        ustr_DecByte(ustr_numform_section);

        arg = p_numform_info->time.seconds;

        status = quick_ublock_printf(p_quick_ublock, USTR_TEXT("%.*" S32_FMT_POSTFIX), count, arg);
        break;

    case 'd':
    case 'D':
        do { ++count; } while(((next_section_ch = ustr_GetByteInc(ustr_numform_section)) == 'd') || (next_section_ch == 'D'));
        ustr_DecByte(ustr_numform_section);

        if(!p_numform_info->date.valid)
        {
            status = numform_output_invalid_date_field(p_quick_ublock, count);
            break;
        }

        arg = p_numform_info->date.day;

        if(count <= 2)
            status = quick_ublock_printf(p_quick_ublock, USTR_TEXT("%.*" S32_FMT_POSTFIX), count, arg);
        else
        {
            PC_USTR ustr_suffix = ustr_empty_string;
            PC_USTR ustr = p_numform_context->day_endings[p_numform_info->date.day - 1];

            if(NULL != ustr)
                ustr_suffix = ustr;

            status = quick_ublock_printf(p_quick_ublock, USTR_TEXT("%" S32_FMT_POSTFIX "%s"), arg, ustr_suffix);
        }
        break;

    case 'm':
    case 'M':
        do { ++count; } while(((next_section_ch = ustr_GetByteInc(ustr_numform_section)) == 'm') || (next_section_ch == 'M'));
        ustr_DecByte(ustr_numform_section);

        if((numform_output_datetime_last_field == 'h') || (numform_output_datetime_last_field == 'H'))
        {
            /* minutes, not months iff immediately after hours */
            arg = p_numform_info->time.minutes;

            status = quick_ublock_printf(p_quick_ublock, USTR_TEXT("%.*" S32_FMT_POSTFIX), count, arg);
            break;
        }

        if(!p_numform_info->date.valid)
        {
            status = numform_output_invalid_date_field(p_quick_ublock, MIN(3, count));
            break;
        }

        if(count <= 2)
        {
            arg = p_numform_info->date.month;

            status = quick_ublock_printf(p_quick_ublock, USTR_TEXT("%.*" S32_FMT_POSTFIX), count, arg);
        }
        else
        {
            PC_USTR ustr_month_name = p_numform_context->month_names[p_numform_info->date.month - 1];
            U32 len = ustr_month_name ? ustrlen32(ustr_month_name) : 0;
            U32 offset;
            U32 limit = U32_MAX;
            U32 i;
            BOOL upper_case_flag = FALSE;

            if(count == 3)
                limit = 3; /* restrict output to just first 3 chars of the monthname */
            else if(count == 5)
                limit = 1; /* restrict output to just first char of the monthname */

            for(offset = 0, i = 0; (offset < len) && (i < limit); ++i)
            {
                /* NB we use a -ve index here as we've skipped over specifier chars when counting */
                S32 specifier_index = (S32) i - (S32) count;
                U32 bytes_of_char;
                UCS4 ucs4 = ustr_char_decode_off(ustr_month_name, offset, /*ref*/bytes_of_char);

                /* uppercase corresponding chars of the monthname */
                /* remembering last specifier char if overflow */
                if(specifier_index < 0)
                    upper_case_flag = (PtrGetByteOff(ustr_numform_section, specifier_index) == 'M');

                if(upper_case_flag)
                    ucs4 = t5_ucs4_uppercase(ucs4);

                status_break(status = quick_ublock_ucs4_add(p_quick_ublock, ucs4));

                offset += bytes_of_char;
            }
        }
        break;

    case 'y':
    case 'Y':
        {
        PC_USTR ustr_format;
        BOOL ad_format, ce_format;

        do { ++count; } while(((next_section_ch = ustr_GetByteInc(ustr_numform_section)) == 'y') || (next_section_ch == 'Y'));

        ad_format = ((next_section_ch == 'a') || (next_section_ch == 'A'));
        ce_format = ((next_section_ch == 'c') || (next_section_ch == 'C'));

        if(!ad_format && !ce_format)
           ustr_DecByte(ustr_numform_section);

        if(!p_numform_info->date.valid)
        {
            status = numform_output_invalid_date_field(p_quick_ublock, count);
            break;
        }

        arg = p_numform_info->date.year;

        switch(count)
        {
        case 1:
        case 2:
            /* BCE dates must always be output as four digit numbers */
            if(arg >= 0)
            {
                arg = arg - ((arg / 100) * 100);
                status = quick_ublock_printf(p_quick_ublock, USTR_TEXT("%.*" S32_FMT_POSTFIX), count, arg);
                break;
            }

            /*FALLTHRU*/

        default:
            if(ad_format)
            {
                BOOL bc_date = FALSE;

                if(arg <= 0)
                {
                    arg = 1 - arg; /* NB. year 0000 -> 1BC, -0001 -> 2BC etc. */
                    bc_date = TRUE;
                    count = 4;
                }

                ustr_format = bc_date ? USTR_TEXT("%.*" S32_FMT_POSTFIX "BC") : USTR_TEXT("%.*" S32_FMT_POSTFIX "AD");
            }
            else if(ce_format)
            {
                BOOL bce_date = FALSE;

                if(arg <= 0)
                {
                    arg = 1 - arg; /* NB. year 0000 -> 1BCE, -0001 -> 2BCE etc. */
                    bce_date = TRUE;
                    count = 4;
                }

                ustr_format = bce_date ? USTR_TEXT("%.*" S32_FMT_POSTFIX "BCE") : USTR_TEXT("%.*" S32_FMT_POSTFIX "CE");
            }
            else
            {
                /* BCE dates must always be output as four digit numbers */
                if(arg <= 0)
                    count = 4;

                ustr_format = USTR_TEXT("%.*" S32_FMT_POSTFIX);
            }

            status = quick_ublock_printf(p_quick_ublock, ustr_format, count, arg);
            break;
        }

        break;
        }
    }

    numform_output_datetime_last_field = section_ch;

    *p_ustr_numform_section = ustr_numform_section;
    return(status);
}

_Check_return_
static STATUS
numform_output_number_fields(
    P_NUMFORM_INFO p_numform_info,
    _InVal_     U8 ch)
{
    STATUS status = STATUS_OK;

    if(ch == CH_FULL_STOP)
    {
        U8 ch_out = p_numform_info->decimal_pt;

        /* supress output of decimal point char sometimes st. 2.0 output under 0.## will give 2, but under 0. will give 2. */
        if(p_numform_info->decimal_places_actual == 0)
            if(p_numform_info->decimal_section_has_hash)
                if(!p_numform_info->decimal_section_has_zero && !p_numform_info->decimal_section_spaces)
                    ch_out = CH_NULL;

        /* i.e. a format such as .### will display just the fractional part of a number */
        p_numform_info->integer_places_actual = 0;

        if(ch_out)
            status_return(quick_ublock_ucs4_add(p_numform_info->p_quick_ublock, ch_out));

        return(status);
    }

    /* Integer length overflows or matches specifier */
    while(p_numform_info->integer_places_format < p_numform_info->integer_places_actual)
    {
        status_return(quick_ublock_ucs4_add(p_numform_info->p_quick_ublock, ustr_GetByteInc_wr(p_numform_info->ustr_integer_section)));
        p_numform_info->integer_places_actual--;
        p_numform_info->integer_section_result_output = 2;

        if(p_numform_info->thousands_sep && (p_numform_info->integer_places_actual > 0))
            if(p_numform_info->integer_places_actual % 3 == 0)
                status_return(quick_ublock_ucs4_add(p_numform_info->p_quick_ublock, p_numform_info->thousands_sep));
    }

    if(p_numform_info->integer_places_format > 0)
    {
        if(p_numform_info->integer_places_format > p_numform_info->integer_places_actual)
        {
            /* Specifier overflows number, so pad up */
            U8 ch_out;

            switch(ch)
            {
            case CH_UNDERSCORE:
            case CH_QUESTION_MARK: /* as Excel */
                ch_out = CH_SPACE;
                if(!p_numform_info->integer_section_result_output)
                    p_numform_info->integer_section_result_output = 1; /* put spaces at thousand positions between padding spaces */
                break;

            case CH_DIGIT_ZERO:
                ch_out = CH_DIGIT_ZERO;
                p_numform_info->integer_section_result_output = 2; /* put thousands separators between padding zeros */
                break;

            default:
                ch_out = CH_DIGIT_ZERO;
                if(!p_numform_info->integer_section_result_output)
                    ch_out = CH_NULL;
                break;
            }

            if(ch_out)
                 status_return(quick_ublock_ucs4_add(p_numform_info->p_quick_ublock, ch_out));
        }
        else
        {
            assert(p_numform_info->integer_places_format == p_numform_info->integer_places_actual);
            status_return(quick_ublock_ucs4_add(p_numform_info->p_quick_ublock, ustr_GetByteInc_wr(p_numform_info->ustr_integer_section)));
            p_numform_info->integer_places_actual--;

            p_numform_info->integer_section_result_output = 2;
        }

        p_numform_info->integer_places_format--;

        if(p_numform_info->thousands_sep && (p_numform_info->integer_places_format > 0) && p_numform_info->integer_section_result_output)
            if(p_numform_info->integer_places_format % 3 == 0)
            {
                U8 th_ch = CH_SPACE;
                if(p_numform_info->integer_section_result_output != 1)
                    th_ch = p_numform_info->thousands_sep;
                status = quick_ublock_ucs4_add(p_numform_info->p_quick_ublock, th_ch);
            }

        return(status);
    }

    /* got any trailing spaces in the decimal section? */
    if((CH_UNDERSCORE == ch) || (CH_QUESTION_MARK == ch))
    {
        if(p_numform_info->decimal_section_spaces)
        {
            status = quick_ublock_a7char_add(p_numform_info->p_quick_ublock, CH_SPACE);
            p_numform_info->decimal_section_spaces--;
            return(status);
        }
    }

    if(p_numform_info->decimal_places_format > 0)
    {
        if(p_numform_info->decimal_places_actual > 0)
        {
            status = quick_ublock_ucs4_add(p_numform_info->p_quick_ublock, ustr_GetByteInc(p_numform_info->ustr_decimal_section));
            p_numform_info->decimal_places_actual--;
        }
        else
        {
            /* run out of digits for the format - pad with zeros as appropriate */
            switch(ch)
            {
            case CH_DIGIT_ZERO:
                status = quick_ublock_a7char_add(p_numform_info->p_quick_ublock, CH_DIGIT_ZERO);
                break;

            default:
                break;
            }
        }

        p_numform_info->decimal_places_format--;

        return(status);
    }

    if(p_numform_info->exponent_sign)
    {
        status_return(quick_ublock_ucs4_add(p_numform_info->p_quick_ublock, p_numform_info->exponential));

        if( (p_numform_info->exponent_sign_actual == CH_MINUS_SIGN__BASIC) ||
            (p_numform_info->exponent_sign_actual == CH_PLUS_SIGN && p_numform_info->exponent_sign == CH_PLUS_SIGN))
            status_return(quick_ublock_ucs4_add(p_numform_info->p_quick_ublock, p_numform_info->exponent_sign_actual));

        p_numform_info->exponent_sign = CH_NULL;
    }

    /* Exponent length overflows or matches specifier */
    while(p_numform_info->exponent_places_format < p_numform_info->exponent_places_actual)
    {
        assert(sbchar_isdigit(PtrGetByte(p_numform_info->ustr_exponent_section)));
        status_return(quick_ublock_ucs4_add(p_numform_info->p_quick_ublock, ustr_GetByteInc_wr(p_numform_info->ustr_exponent_section)));
        p_numform_info->exponent_places_actual--;
    }

    /* got any trailing spaces in the exponent section? */
    if((CH_UNDERSCORE == ch) || (CH_QUESTION_MARK == ch))
    {
        if(p_numform_info->exponent_section_spaces)
        {
            status = quick_ublock_a7char_add(p_numform_info->p_quick_ublock, CH_SPACE);
            p_numform_info->exponent_section_spaces--;
            return(status);
        }
    }

    if(p_numform_info->exponent_places_format > 0)
    {
        if(p_numform_info->exponent_places_format > p_numform_info->exponent_places_actual)
        {
            /* Specifier overflows number */
            switch(ch)
            {
            case CH_DIGIT_ZERO:
                status_return(quick_ublock_a7char_add(p_numform_info->p_quick_ublock, CH_DIGIT_ZERO));
                break;

            default:
                break;
            }
        }
        else
        {
            assert(p_numform_info->exponent_places_format == p_numform_info->exponent_places_actual);
            assert(sbchar_isdigit(PtrGetByte(p_numform_info->ustr_exponent_section)));
            status_return(quick_ublock_ucs4_add(p_numform_info->p_quick_ublock, ustr_GetByteInc_wr(p_numform_info->ustr_exponent_section)));
            p_numform_info->exponent_places_actual--;
        }

        p_numform_info->exponent_places_format--;
    }

    return(status);
}

_Check_return_
static STATUS
numform_output_style_name(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock_style /*appended,terminated*/,
    _In_z_      PC_USTR ustr_style_name)
{
    U8 ch;

    /* terminated by CH_RIGHT_SQUARE_BRACKET hopefully */
    for(;;)
    {
        ch = ustr_GetByteInc(ustr_style_name);
        
        if((CH_RIGHT_SQUARE_BRACKET == ch) || (CH_NULL == ch))
            break;

       status_return(quick_tblock_tchar_add(p_quick_tblock_style, ch));
    }

    return(quick_tblock_nullch_add(p_quick_tblock_style));
}

static void
numform_section_copy(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_section)
{
    const P_U8 buffer = (P_U8) ustr_buf;
    U32 dst_idx = 0;
    U8 ch;

    assert(0 != elemof_buffer); /* needs CH_NULL-termination */

    for(;;)
    {
        ch = ustr_GetByteInc(ustr_section);

        switch(ch)
        {
        case CH_QUOTATION_MARK:
            for(;;)
            {
                if(dst_idx >= elemof_buffer) break;
                buffer[dst_idx++] = ch;

                ch = ustr_GetByteInc(ustr_section);

                if((CH_QUOTATION_MARK == ch) || (CH_NULL == ch))
                    break;
            }
            break;

        case CH_BACKWARDS_SLASH:
            /* next character literal */
            if(dst_idx >= elemof_buffer) break;
            buffer[dst_idx++] = ch;

            ch = ustr_GetByteInc(ustr_section);
            break;

        case CH_SEMICOLON:
            ch = CH_NULL;
            break;

        default:
            break;
        }

        if(CH_NULL == ch)
            break;

        if(dst_idx >= elemof_buffer) break;
        buffer[dst_idx++] = ch;
    }

    /* CH_NULL-terminate */
    if(dst_idx >= elemof_buffer)
        dst_idx = elemof_buffer - 1;
    buffer[dst_idx++] = CH_NULL;
}

/* 2 section format is for time_only_am,time_only_pm */
/* 3 section format is for generic,date_only,time_only */
/* 4 section format is for generic,date_only,time_only_am,time_only_pm */

/*           1 section  2 sections  3 sections  4 sections  */
/* date_only    1st         1st         2nd         2nd     */
/* time_only    1st         1/2         3rd         3/4     */
/* both         1st         1st         1st         1st     */

static void
numform_section_extract_datetime(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_datetime)
{
    BOOL has_date = p_numform_info->date.valid;
    BOOL has_time = p_numform_info->time.valid;
    BOOL date_only = has_date && !has_time;
#if SS_DATE_NULL != 0
    BOOL time_only = has_time && !has_date;
#else
    BOOL time_only = /*has_time &&*/ !has_date; /*SKS 10feb97 for 00:00:00 time with no date*/
#endif
    PC_USTR ustr_section = ustr_numform_datetime;
    PC_USTR ustr_second_section = numform_section_scan_next(ustr_section);
    PC_USTR ustr_third_section =
        (P_USTR_NONE != ustr_second_section)
            ? numform_section_scan_next(ustr_second_section)
            : P_USTR_NONE;

    if(date_only)
    {
        if(P_USTR_NONE != ustr_third_section)
            ustr_section = ustr_second_section;
    }
    else if(time_only)
    {
        if(P_USTR_NONE != ustr_third_section)
        {
            BOOL is_pm = (p_numform_info->time.hours >= 12);

            ustr_section = ustr_third_section;

            /* if we have time >= 1200, look for an optional fourth section */
            if(is_pm)
            {
                PC_USTR ustr_fourth_section = numform_section_scan_next(ustr_third_section);

                if(P_USTR_NONE != ustr_fourth_section)
                {
                    if(p_numform_info->time.hours >= 12)
                        p_numform_info->time.hours -= 12;

                    if(p_numform_info->time.hours == 0) /* err, 0:30pm is silly SKS 07feb96 */
                        p_numform_info->time.hours = 12;

                    ustr_section = ustr_fourth_section;
                }
            }
        }
        else if(P_USTR_NONE != ustr_second_section) /* 2 section am/pm format? */
        {
            BOOL is_pm = (p_numform_info->time.hours >= 12);

            if(is_pm)
            {
                if(p_numform_info->time.hours >= 12)
                    p_numform_info->time.hours -= 12;

                if(p_numform_info->time.hours == 0) /* err, 0:30pm is silly SKS 07feb96 */
                    p_numform_info->time.hours = 12;

                ustr_section = ustr_second_section;
            }
        }
    }
    else
    {
        if((P_USTR_NONE != ustr_second_section) && (P_USTR_NONE == ustr_third_section)) /* 2 section am/pm format, possibly with date bits? */
        {
            BOOL is_pm = (p_numform_info->time.hours >= 12);

            if(is_pm)
            {
                if(p_numform_info->time.hours >= 12)
                    p_numform_info->time.hours -= 12;

                if(p_numform_info->time.hours == 0) /* err, 0:30pm is silly SKS 07feb96 */
                    p_numform_info->time.hours = 12;

                ustr_section = ustr_second_section;
            }
        }
    }

    numform_section_copy(ustr_buf, elemof_buffer, ustr_section);
}

static void
numform_section_extract_numeric(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_numeric,
    _Out_writes_z_(elemof_numeric_buffer) P_USTR ustr_numeric_buf,
    _InVal_     U32 elemof_numeric_buffer)
{
    PC_USTR ustr_section = ustr_numform_numeric;
    PC_USTR ustr_next_section;
    S32 shift_value;

    /* if we have a negative number, search for optional second section */
    /* if second section is not present, we must flag to insert a minus sign where needed */
    if(p_numform_info->number.negative)
    {
        if(P_USTR_NONE != (ustr_next_section = numform_section_scan_next(ustr_section)))
            ustr_section = ustr_next_section;
        else
            p_numform_info->number.insert_minus_sign = 1; /* no second section, insert a minus sign where needed */
    }

    /* if we have a zero number, search for optional third section (after any second section) */
    /* it doesn't matter if third section is not present */
    if(p_numform_info->number.zero)
    {
        if(P_USTR_NONE != (ustr_next_section = numform_section_scan_next(ustr_section)))
            if(P_USTR_NONE != (ustr_next_section = numform_section_scan_next(ustr_next_section)))
                ustr_section = ustr_next_section;
    }

    trace_1(TRACE_MODULE_NUMFORM, TEXT("numform buffer 1 %s"), report_ustr(ustr_section));
    shift_value = numform_numeric_section_copy_and_parse(p_numform_info, ustr_buf, elemof_buffer, ustr_section);
    trace_1(TRACE_MODULE_NUMFORM, TEXT("numform buffer 2 %s"), report_ustr(ustr_buf));

    if(shift_value)
    {
        F64 multiplier = pow(10.0, shift_value);
        F64 work_value = ss_data_get_number(&p_numform_info->ss_data);
        ss_data_set_real(&p_numform_info->ss_data, work_value * multiplier);
        (void) check_real_nan_inf(p_numform_info); /* recheck for nan/infinity in this case */
    }

    p_numform_info->ustr_integer_section = ustr_numeric_buf;
    p_numform_info->elemof_integer_section = elemof_numeric_buffer;

    /* now handle exceptional stuff */
    if(p_numform_info->number.nan || p_numform_info->number.infinity)
    {
        p_numform_info->number.insert_minus_sign = 0; /* as we are forcing the format */

        (void) numform_numeric_section_copy_and_parse(p_numform_info, ustr_buf, elemof_buffer,
                                                      p_numform_info->number.negative ? USTR_TEXT("-#") : USTR_TEXT("#"));

        consume_int(ustr_xsnprintf(p_numform_info->ustr_integer_section, p_numform_info->elemof_integer_section,
                                   USTR_TEXT("%s"),
                                   (p_numform_info->number.nan) ? "nan" : "inf"));

        p_numform_info->integer_places_actual = ustrlen32(p_numform_info->ustr_integer_section);
        return;
    }

    if(p_numform_info->roman)
    {
        if(convert_number_roman(p_numform_info))
            return;
    }

    if(p_numform_info->spreadsheet)
    {
        if(convert_number_spreadsheet(p_numform_info))
            return;
    }

    if(p_numform_info->ustr_lookup_section)
    {
        convert_number_lookup(p_numform_info, ustr_buf /* if empty, supply a default */);
        return;
    }

    if(p_numform_info->engineering)
    {
        convert_number_engineering(p_numform_info);
        return;
    }

    if(p_numform_info->exponential)
    {
        convert_number_exponential(p_numform_info);
        return;
    }

    if(p_numform_info->base)
    {
        convert_number_base(p_numform_info);
        return;
    }

    convert_number_standard(p_numform_info);
}

static void
numform_section_extract_texterror(
    P_NUMFORM_INFO p_numform_info,
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR ustr_numform_texterror)
{
    PC_USTR ustr_section = ustr_numform_texterror;
    PC_USTR ustr_next_section;

    /* maybe search for optional second section */
    switch(ss_data_get_data_id(&p_numform_info->ss_data))
    {
    case DATA_ID_BLANK:
    case DATA_ID_ERROR:
        if(P_USTR_NONE != (ustr_next_section = numform_section_scan_next(ustr_section)))
        {
            /* maybe search for optional third section */
            if(ss_data_is_blank(&p_numform_info->ss_data))
            {
                if(P_USTR_NONE != (ustr_next_section = numform_section_scan_next(ustr_next_section)))
                    ustr_section = ustr_next_section;
                /* otherwise stick with first section */
            }
            else
            {
                ustr_section = ustr_next_section;
            }
        }
        break;

    default:
        break;
    }

    numform_section_copy(ustr_buf, elemof_buffer, ustr_section);
}

_Check_return_
_Ret_z_ /*opt_*/
static PC_USTR
numform_section_scan_next(
    _In_z_      PC_USTR ustr_section)
{
    U8 ch;

    do  {
        switch(ch = ustr_GetByteInc(ustr_section))
        {
        case CH_BACKWARDS_SLASH:
            /* next character literal */
            ch = ustr_GetByteInc(ustr_section);
            break;

        case CH_QUOTATION_MARK:
            do  {
                ch = ustr_GetByteInc(ustr_section);
            }
            while(ch && (ch != CH_QUOTATION_MARK));
            break;

        case CH_SEMICOLON:
            return(ustr_section);

        default:
            break;
        }
    }
    while(ch);

    return(P_USTR_NONE);
}

/* end of numform.c */
