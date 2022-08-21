/* ucs4.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2006-2016 Stuart Swales */

/* Library module for UCS-4 character handling */

/* SKS Oct 2006 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/collect.h"

/*
UCS-4
*/

_Check_return_
extern STATUS
ucs4_validate(
    _InVal_     UCS4 ucs4)
{
    /* Check first against lowest invalid code point */
    if(ucs4 < UCH_SURROGATE_HIGH)
    {
        return(STATUS_OK);
    }

    /* Surrogate character? (High, Low) */
    if((ucs4 >= UCH_SURROGATE_HIGH) && (ucs4 <= UCH_SURROGATE_LOW_END))
    {
        return(ERR_UCS4_NONCHARACTER);
    }

    if((ucs4 >= UCH_NONCHARACTER_STT) && (ucs4 <= UCH_NONCHARACTER_END))
    {
        return(ERR_UCS4_NONCHARACTER);
    }

    /* Check for noncharacters (UCD 5.0.0 PropList) */
    switch(ucs4 & 0x0000FFFFU)
    {
    case UCH_NONCHARACTER_XXFFFE:
    case UCH_NONCHARACTER_XXFFFF:
        return(ERR_UCS4_NONCHARACTER);

    default:
        break;
    }

    /* Check Unicode character limit */
    if(ucs4 >= UCH_UNICODE_INVALID)
    {
        return(ERR_UCS4_NONCHARACTER);
    }

    return(STATUS_OK);
}

/*
UCD access functions
*/

#if 0 /* radically reduce for debugging list handling */
#define UCS4_TABLE_SIZE             0x00100
#else
#define UCS4_TABLE_SIZE             0x10000 /* Covers BMP: Basic Multilingual Plane */
#endif

/* two-level table covering a total of UCS4_TABLE_SIZE code points:
 * first level table is an static array of pointers to allocated
 * second level entries, each covering UCS4_TABLE_L2_SIZE code points.
 * Code points beyond UCS4_TABLE_SIZE are handled by sparse lists of
 * entries of UCS4_TABLE_L2_SIZE.
 */
#define UCS4_TABLE_L2_BITS          8
#define UCS4_TABLE_L2_SIZE          (1 << UCS4_TABLE_L2_BITS)
#define UCS4_TABLE_L2_MASK          (UCS4_TABLE_L2_SIZE - 1)
#define UCS4_TABLE_L2_BASE(ucs4)    ((ucs4) & ~UCS4_TABLE_L2_MASK)

#define UCS4_TABLE_L1_SIZE          (UCS4_TABLE_SIZE/UCS4_TABLE_L2_SIZE)

static BOOL read_failed_UnicodeData = 0;

typedef struct CASE_MAP
{
    UCS4 lowercase;
    UCS4 uppercase;
}
CASE_MAP, * P_CASE_MAP;

static P_CASE_MAP l1_table_case_map_simple[UCS4_TABLE_L1_SIZE];

static LIST_BLOCK list_block_case_map_simple;

_Check_return_
static STATUS
load_case_map_simple(
    _In_        UCS4 ucs4)
{
    UCS4 l2_base_ucs4 = UCS4_TABLE_L2_BASE(ucs4);
    STATUS status = STATUS_OK;

    if(!read_failed_UnicodeData)
    {   /* Read and Parse Unicode Character Database file UnicodeData.txt, adding entries for this range */
        TCHARZ filename[BUF_MAX_PATHSTRING];

        if(status_fail(status = file_find_on_path(filename, elemof32(filename), TEXT("UCD") FILE_DIR_SEP_TSTR TEXT("UnicodeData.txt"))))
        {
            read_failed_UnicodeData = 1;
        }
        else
        {
            FILE_HANDLE file_handle = 0;

            if(!status_done(status = t5_file_open(filename, file_open_read, &file_handle, FALSE))) /* fail silently */
            {
                read_failed_UnicodeData = 1;
            }
            else
            {
                P_CASE_MAP l2_table;

                if(ucs4 < UCS4_TABLE_SIZE)
                {
                    const U32 l2_size_elem = UCS4_TABLE_L2_SIZE;
                    l2_table = l1_table_case_map_simple[l2_base_ucs4 >> UCS4_TABLE_L2_BITS] = al_ptr_alloc_elem(CASE_MAP, l2_size_elem, &status);
                }
                else
                {
                    const U32 l2_size_bytes = UCS4_TABLE_L2_SIZE * sizeof32(*l2_table);
                    l2_table = collect_add_entry_bytes(CASE_MAP, &list_block_case_map_simple, NULL, l2_size_bytes, l2_base_ucs4, &status);
                }

                if(NULL == l2_table)
                {
                    read_failed_UnicodeData = 1;
                }
                else
                {
                    U8 buffer[256];

                    { /* Default mapping is from character to itself */
                    UCS4 code_point;

                    for(code_point = l2_base_ucs4; code_point < l2_base_ucs4 + UCS4_TABLE_L2_SIZE; ++code_point)
                    {
                        l2_table[code_point - l2_base_ucs4].lowercase = code_point;
                        l2_table[code_point - l2_base_ucs4].uppercase = code_point;
                    }

                    } /*block*/

                    /* The entries in this file are in the following machine-readable format: */
                    /* 0 <code>;
                     * 1 <Name>;
                     * 2 <General_Category>;
                     * 3 <Canonical_Combining_Class>;
                     * 4 <Bidi_Class>;
                     * 5 <Decomposition_Type/Decomposition_Mapping>;
                     * 6,7,8 <Numeric_Type/Numeric_Value>*3;
                     * 9 <Bidi_Mirrored>;
                     * 10 <Unicode_1_Name>;
                     * 11 <ISO_Comment>;
                     * 12 <Simple_Uppercase_Mapping>;
                     * 13 <Simple_Lowercase_Mapping>;
                     * 14 <Simple_Titlecase_Mapping>
                     */

                    while(status_ok(status = file_gets(buffer, elemof32(buffer), file_handle)))
                    {
                        char *ptr = buffer;
                        UCS4 code_point;
                        UCS4 entry_mapping_uc;
                        UCS4 entry_mapping_lc;
                        char *code_point_name;

                        if(EOF_READ == status)
                            break;

                        if((CH_NULL == *ptr) || (CH_NUMBER_SIGN == *ptr))
                            continue;

                        code_point = (UCS4) strtoul(ptr, &ptr, 16); /* field 0: code point */

                        if(CH_SEMICOLON != *ptr)
                            continue;

                        ++ptr; /* skip semicolon: field 1 */

                        if((code_point < l2_base_ucs4) || (code_point >= (l2_base_ucs4 + UCS4_TABLE_L2_SIZE)))
                            continue;

                        code_point_name = ptr;

                        while(CH_NULL != *ptr) /* skip field 1, semicolon */
                            if(CH_SEMICOLON == *ptr++)
                            {
                                ptr[-1] = CH_NULL; /* terminate code point name */
                                break;
                            }

                        while(CH_NULL != *ptr) /* skip field 2, semicolon */
                            if(CH_SEMICOLON == *ptr++)
                                break;

                        while(CH_NULL != *ptr) /* skip field 3, semicolon */
                            if(CH_SEMICOLON == *ptr++)
                                break;

                        while(CH_NULL != *ptr) /* skip field 4, semicolon */
                            if(CH_SEMICOLON == *ptr++)
                                break;

                        while(CH_NULL != *ptr) /* skip field 5, semicolon */
                            if(CH_SEMICOLON == *ptr++)
                                break;

                        while(CH_NULL != *ptr) /* skip field 6, semicolon */
                            if(CH_SEMICOLON == *ptr++)
                                break;

                        while(CH_NULL != *ptr) /* skip field 7, semicolon */
                            if(CH_SEMICOLON == *ptr++)
                                break;

                        while(CH_NULL != *ptr) /* skip field 8, semicolon */
                            if(CH_SEMICOLON == *ptr++)
                                break;

                        while(CH_NULL != *ptr) /* skip field 9, semicolon */
                            if(CH_SEMICOLON == *ptr++)
                                break;

                        while(CH_NULL != *ptr) /* skip field 10, semicolon */
                            if(CH_SEMICOLON == *ptr++)
                                break;

                        while(CH_NULL != *ptr) /* skip field 11, semicolon */
                            if(CH_SEMICOLON == *ptr++)
                                break;

                        /* field 12: simple uppercase mapping */

                        if(CH_SEMICOLON == *ptr)
                        {
                            entry_mapping_uc = code_point;
                        }
                        else
                        {
                            entry_mapping_uc = (UCS4) strtoul(ptr, &ptr, 16);

                            if(CH_SEMICOLON != *ptr)
                                continue;
                        }

                        ++ptr; /* skip semicolon */

                        /* field 13: simple lowercase mapping */

                        if(CH_SEMICOLON == *ptr)
                        {
                            entry_mapping_lc = code_point;
                        }
                        else
                        {
                            entry_mapping_lc = (UCS4) strtoul(ptr, &ptr, 16);

                            if(CH_SEMICOLON != *ptr)
                                continue;
                        }

                        ++ptr; /* skip semicolon */

                        /* field 14: simple titlecase mapping */

#if TRACE_ALLOWED && 1
                        tracef(TRACE_OUT | TRACE_ANY, TEXT("CaseMap: U+%.6X: 0x%.6X(U),0x%.6X(L) # %s\n"), code_point, entry_mapping_uc, entry_mapping_lc, report_sbstr(code_point_name));
#else
                        UNREFERENCED_PARAMETER(code_point_name);
#endif

                        l2_table[code_point - l2_base_ucs4].lowercase = entry_mapping_lc;
                        l2_table[code_point - l2_base_ucs4].uppercase = entry_mapping_uc;
                    }
                }

                status_assert(t5_file_close(&file_handle));

                if(NULL != l2_table)
                    return(STATUS_OK);
            }
        }
    }

    return(status);
}

/*
UnicodeData: Lowercase
*/

_Check_return_
extern UCS4
ucs4_lowercase(
    _InVal_     UCS4 ucs4)
{
    UCS4 l2_base_ucs4 = UCS4_TABLE_L2_BASE(ucs4);
    P_CASE_MAP l2_table;

    if(status_fail(ucs4_validate(ucs4)))
        return(UCH_REPLACEMENT_CHARACTER);

    for(;;)
    {
        if(ucs4 < UCS4_TABLE_SIZE)
        {
            if(NULL != (l2_table = l1_table_case_map_simple[l2_base_ucs4 >> UCS4_TABLE_L2_BITS]))
            {
                return(l2_table[ucs4 - l2_base_ucs4].lowercase);
            }
        }
        else
        {
            if(NULL != (l2_table = list_gotoitemcontents_opt(CASE_MAP, &list_block_case_map_simple, l2_base_ucs4)))
            {
                return(l2_table[ucs4 - l2_base_ucs4].lowercase);
            }
        }

        if(status_ok(load_case_map_simple(ucs4)))
            continue;

        return(ucs4);
    }
}

/*
UnicodeData: Uppercase
*/

_Check_return_
extern UCS4
ucs4_uppercase(
    _InVal_     UCS4 ucs4)
{
    UCS4 l2_base_ucs4 = UCS4_TABLE_L2_BASE(ucs4);
    P_CASE_MAP l2_table;

    if(status_fail(ucs4_validate(ucs4)))
        return(UCH_REPLACEMENT_CHARACTER);

    for(;;)
    {
        if(ucs4 < UCS4_TABLE_SIZE)
        {
            if(NULL != (l2_table = l1_table_case_map_simple[l2_base_ucs4 >> UCS4_TABLE_L2_BITS]))
            {
                return(l2_table[ucs4 - l2_base_ucs4].uppercase);
            }
        }
        else
        {
            if(NULL != (l2_table = list_gotoitemcontents_opt(CASE_MAP, &list_block_case_map_simple, l2_base_ucs4)))
            {
                return(l2_table[ucs4 - l2_base_ucs4].uppercase);
            }
        }

        if(status_ok(load_case_map_simple(ucs4)))
            continue;

        return(ucs4);
    }
}

/*
UnicodeData: Decimal Digit
*/

static const UCS4
DecimalDigitZeroCodePoint[] = /* General_Category=Nd, sorted by code point */
{
    0x0030, /*;DIGIT ZERO;Nd;0;EN;;0;0;0;N;;;;;*/
    0x0660, /*;ARABIC-INDIC DIGIT ZERO;Nd;0;AN;;0;0;0;N;;;;;*/
    0x06F0, /*;EXTENDED ARABIC-INDIC DIGIT ZERO;Nd;0;EN;;0;0;0;N;EASTERN ARABIC-INDIC DIGIT ZERO;;;;*/
    0x07C0, /*;NKO DIGIT ZERO;Nd;0;R;;0;0;0;N;;;;;*/
    0x0966, /*;DEVANAGARI DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x09E6, /*;BENGALI DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x0A66, /*;GURMUKHI DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x0AE6, /*;GUJARATI DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x0B66, /*;ORIYA DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x0BE6, /*;TAMIL DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x0C66, /*;TELUGU DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x0CE6, /*;KANNADA DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x0D66, /*;MALAYALAM DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x0E50, /*;THAI DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x0ED0, /*;LAO DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x0F20, /*;TIBETAN DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x1040, /*;MYANMAR DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x17E0, /*;KHMER DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x1810, /*;MONGOLIAN DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x1946, /*;LIMBU DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x19D0, /*;NEW TAI LUE DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x1B50, /*;BALINESE DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0xFF10, /*;FULLWIDTH DIGIT ZERO;Nd;0;EN;<wide> 0030;0;0;0;N;;;;;*/
    0x104A0, /*;OSMANYA DIGIT ZERO;Nd;0;L;;0;0;0;N;;;;;*/
    0x1D7CE, /*;MATHEMATICAL BOLD DIGIT ZERO;Nd;0;EN;<font> 0030;0;0;0;N;;;;;*/
    0x1D7D8, /*;MATHEMATICAL DOUBLE-STRUCK DIGIT ZERO;Nd;0;EN;<font> 0030;0;0;0;N;;;;;*/
    0x1D7E2, /*;MATHEMATICAL SANS-SERIF DIGIT ZERO;Nd;0;EN;<font> 0030;0;0;0;N;;;;;*/
    0x1D7EC, /*;MATHEMATICAL SANS-SERIF BOLD DIGIT ZERO;Nd;0;EN;<font> 0030;0;0;0;N;;;;;*/
    0x1D7F6  /*;MATHEMATICAL MONOSPACE DIGIT ZERO;Nd;0;EN;<font> 0030;0;0;0;N;;;;;*/
};

_Check_return_
extern BOOL
ucs4_is_decimal_digit(
    _InVal_     UCS4 ucs4)
{
    U32 i;

    for(i = 0; i < elemof32(DecimalDigitZeroCodePoint); i++)
    {
        UCS4 ucs4_digit_zero = DecimalDigitZeroCodePoint[i];

        if(ucs4 < ucs4_digit_zero)
            return(FALSE);

        if(ucs4 <= (ucs4_digit_zero + 9))
            return(TRUE);
    }

    assert(status_ok(ucs4_validate(ucs4)));

    return(FALSE);
}

_Check_return_
extern S32 /* -1 or 0..9 */
ucs4_decimal_digit_value(
    _InVal_     UCS4 ucs4)
{
    U32 i;

    for(i = 0; i < elemof32(DecimalDigitZeroCodePoint); i++)
    {
        UCS4 ucs4_digit_zero = DecimalDigitZeroCodePoint[i];

        if(ucs4 < ucs4_digit_zero)
            return(-1);

        if(ucs4 <= (ucs4_digit_zero + 9))
            return((S32) (ucs4 - ucs4_digit_zero));
    }

    assert(status_ok(ucs4_validate(ucs4)));

    return(-1);
}

/*
DerivedCoreProperties: (Boolean)
*/

static BOOL read_failed_DerivedCoreProperties = FALSE;

#define CODE_POINT_IS_ALPHABETIC        (1<<0)
#define CODE_POINT_IS_LOWERCASE         (1<<1)
#define CODE_POINT_IS_UPPERCASE         (1<<2)
#define CODE_POINT_IS_XID_START         (1<<3)
#define CODE_POINT_IS_XID_CONTINUE      (1<<4)
#define CODE_POINT_IS_GRAPHEME_EXTEND   (1<<5)

static P_U8 l1_table_DerivedCoreProperties[UCS4_TABLE_L1_SIZE];

static LIST_BLOCK list_block_DerivedCoreProperties;

_Check_return_
static STATUS
load_DerivedCoreProperties(
    _In_        UCS4 ucs4)
{
    UCS4 l2_base_ucs4 = UCS4_TABLE_L2_BASE(ucs4);
    STATUS status = STATUS_OK;

    if(!read_failed_DerivedCoreProperties)
    {   /* Read and Parse Unicode Character Database file DerivedCoreProperties.txt, adding entries for this new range */
        TCHARZ filename[BUF_MAX_PATHSTRING];

        if(status_fail(status = file_find_on_path(filename, elemof32(filename), TEXT("UCD") FILE_DIR_SEP_TSTR TEXT("DerivedCoreProperties.txt"))))
        {
            read_failed_DerivedCoreProperties = 1;
        }
        else
        {
            FILE_HANDLE file_handle = 0;

            if(!status_done(status = t5_file_open(filename, file_open_read, &file_handle, FALSE))) /*fail silently*/
            {
                read_failed_DerivedCoreProperties = 1;
            }
            else
            {
                P_U8 l2_table;
                const U32 l2_size_elem = UCS4_TABLE_L2_SIZE;
                const U32 l2_size_bytes = l2_size_elem * sizeof32(*l2_table);

                if(ucs4 < UCS4_TABLE_SIZE)
                {
                    l2_table = l1_table_DerivedCoreProperties[l2_base_ucs4 >> UCS4_TABLE_L2_BITS] = al_ptr_alloc_elem(U8, l2_size_elem, &status);
                }
                else
                {
                    l2_table = collect_add_entry_bytes(U8, &list_block_DerivedCoreProperties, NULL, l2_size_bytes, l2_base_ucs4, &status);
                }

                if(NULL == l2_table)
                {
                    read_failed_DerivedCoreProperties = 1;
                }
                else
                {
                    U8 buffer[256];

                    /* Default mapping is property=FALSE */
                    memset32(l2_table, 0, l2_size_bytes);

                    /* The entries in this file are in the following machine-readable format: */
                    /* <code>; <status>; <mapping>; # <name> */

                    while(status_ok(status = file_gets(buffer, elemof32(buffer), file_handle)))
                    {
                        char *ptr = buffer;
                        UCS4 code_point_stt;
                        UCS4 code_point_end;
                        UCS4 code_point;
                        U8 flags = 0;

                        if(EOF_READ == status)
                            break;

                        if((CH_NULL == *ptr) || (CH_NUMBER_SIGN == *ptr))
                            continue;

                        code_point_stt = (UCS4) strtoul(ptr, &ptr, 16);

                        if((CH_FULL_STOP == *ptr) && (CH_FULL_STOP == ptr[1]))
                            code_point_end = (UCS4) strtoul(ptr + 2, &ptr, 16);
                        else
                            code_point_end = code_point_stt;

                        if((code_point_end < l2_base_ucs4) || (code_point_stt >= (l2_base_ucs4 + UCS4_TABLE_L2_SIZE)))
                            continue;

                        while(*ptr++ == CH_SPACE)
                            continue;

                        if(CH_SEMICOLON != ptr[-1])
                            continue;

                        while(*ptr++ == CH_SPACE)
                            continue;

                        --ptr;

                        if(     0 == /*"C"*/strncmp(ptr, "Alphabetic ", elemof32("Alphabetic ")-1))
                            flags = CODE_POINT_IS_ALPHABETIC;
                        else if(0 == /*"C"*/strncmp(ptr, "Lowercase ", elemof32("Lowercase ")-1))
                            flags = CODE_POINT_IS_LOWERCASE;
                        else if(0 == /*"C"*/strncmp(ptr, "Uppercase ", elemof32("Uppercase ")-1))
                            flags = CODE_POINT_IS_UPPERCASE;
                        else if(0 == /*"C"*/strncmp(ptr, "XID_Start ", elemof32("XID_Start ")-1))
                            flags = CODE_POINT_IS_XID_START;
                        else if(0 == /*"C"*/strncmp(ptr, "XID_Continue ", elemof32("XID_Continue ")-1))
                            flags = CODE_POINT_IS_XID_CONTINUE;
                        else if(0 == /*"C"*/strncmp(ptr, "Grapheme_Extend ", elemof32("Grapheme_Extend ")-1))
                            flags = CODE_POINT_IS_GRAPHEME_EXTEND;
                        else
                            continue;

#if TRACE_ALLOWED && 1
                        if(code_point_stt == code_point_end)
                        {
                            tracef(TRACE_OUT | TRACE_ANY, TEXT("DCP: U+%.6X: %s\n"), code_point_stt, report_sbstr(ptr));
                        }
                        else
                        {
                            tracef(TRACE_OUT | TRACE_ANY, TEXT("DCP: U+%.6X..U+%.6X: %s\n"), code_point_stt, code_point_end, report_sbstr(ptr));
                        }
#endif

                        code_point_stt = MAX(code_point_stt, l2_base_ucs4);
                        code_point_end = MIN(code_point_end, (l2_base_ucs4 + UCS4_TABLE_L2_SIZE) - 1 /*excl->incl*/);

                        for(code_point = code_point_stt; code_point <= code_point_end; ++code_point)
                        {
                            l2_table[code_point - l2_base_ucs4] |= flags;
                        }
                    }
                }

                status_assert(t5_file_close(&file_handle));

                if(NULL != l2_table)
                    return(STATUS_OK);
            }
        }
    }

    return(status);
}

/*
DerivedCoreProperties: Alphabetic
Generated from: Lu+Ll+Lt+Lm+Lo+Nl + Other_Alphabetic
*/

_Check_return_
extern BOOL
ucs4_is_alphabetic(
    _InVal_     UCS4 ucs4)
{
    UCS4 l2_base_ucs4 = UCS4_TABLE_L2_BASE(ucs4);
    PC_U8 l2_table;

    if(status_fail(ucs4_validate(ucs4)))
        return(FALSE);

    for(;;)
    {
        if(ucs4 < UCS4_TABLE_SIZE)
        {
            if(NULL != (l2_table = l1_table_DerivedCoreProperties[l2_base_ucs4 >> UCS4_TABLE_L2_BITS]))
            {
                return(0 != (l2_table[ucs4 - l2_base_ucs4] & CODE_POINT_IS_ALPHABETIC));
            }
        }
        else
        {
            if(NULL != (l2_table = list_gotoitemcontents_opt(U8, &list_block_DerivedCoreProperties, l2_base_ucs4)))
            {
                return(0 != (l2_table[ucs4 - l2_base_ucs4] & CODE_POINT_IS_ALPHABETIC));
            }
        }

        if(status_ok(load_DerivedCoreProperties(ucs4)))
            continue;

        return(FALSE);
    }
}

/*
DerivedCoreProperties: Lowercase
Generated from: Ll + Other_Lowercase
*/

_Check_return_
extern BOOL
ucs4_is_lowercase(
    _InVal_     UCS4 ucs4)
{
    UCS4 l2_base_ucs4 = UCS4_TABLE_L2_BASE(ucs4);
    PC_U8 l2_table;

    if(status_fail(ucs4_validate(ucs4)))
        return(FALSE);

    for(;;)
    {
        if(ucs4 < UCS4_TABLE_SIZE)
        {
            if(NULL != (l2_table = l1_table_DerivedCoreProperties[l2_base_ucs4 >> UCS4_TABLE_L2_BITS]))
            {
                return(0 != (l2_table[ucs4 - l2_base_ucs4] & CODE_POINT_IS_LOWERCASE));
            }
        }
        else
        {
            if(NULL != (l2_table = list_gotoitemcontents_opt(U8, &list_block_DerivedCoreProperties, l2_base_ucs4)))
            {
                return(0 != (l2_table[ucs4 - l2_base_ucs4] & CODE_POINT_IS_LOWERCASE));
            }
        }

        if(status_ok(load_DerivedCoreProperties(ucs4)))
            continue;

        return(FALSE);
    }
}

/*
DerivedCoreProperties: Uppercase
Generated from: Lu + Other_Uppercase
*/

_Check_return_
extern BOOL
ucs4_is_uppercase(
    _InVal_     UCS4 ucs4)
{
    UCS4 l2_base_ucs4 = UCS4_TABLE_L2_BASE(ucs4);
    PC_U8 l2_table;

    if(status_fail(ucs4_validate(ucs4)))
        return(FALSE);

    for(;;)
    {
        if(ucs4 < UCS4_TABLE_SIZE)
        {
            if(NULL != (l2_table = l1_table_DerivedCoreProperties[l2_base_ucs4 >> UCS4_TABLE_L2_BITS]))
            {
                return(0 != (l2_table[ucs4 - l2_base_ucs4] & CODE_POINT_IS_UPPERCASE));
            }
        }
        else
        {
            if(NULL != (l2_table = list_gotoitemcontents_opt(U8, &list_block_DerivedCoreProperties, l2_base_ucs4)))
            {
                return(0 != (l2_table[ucs4 - l2_base_ucs4] & CODE_POINT_IS_UPPERCASE));
            }
        }

        if(status_ok(load_DerivedCoreProperties(ucs4)))
            continue;

        return(FALSE);
    }
}

/*
DerivedCoreProperties: XID_Start
ID_Start modified for closure under NFKx
Modified as described in UAX #15
NOTE: Does NOT remove the non-NFKx characters.
      Merely ensures that if isIdentifer(string) then isIdentifier(NFKx(string))
NOTE: See UAX #31 for more information
-where-
ID_Start: Characters that can start an identifier.
Generated from Lu+Ll+Lt+Lm+Lo+Nl+Other_ID_Start
*/

_Check_return_
extern BOOL
ucs4_is_XID_start(
    _InVal_     UCS4 ucs4)
{
    UCS4 l2_base_ucs4 = UCS4_TABLE_L2_BASE(ucs4);
    PC_U8 l2_table;

    if(status_fail(ucs4_validate(ucs4)))
        return(FALSE);

    for(;;)
    {
        if(ucs4 < UCS4_TABLE_SIZE)
        {
            if(NULL != (l2_table = l1_table_DerivedCoreProperties[l2_base_ucs4 >> UCS4_TABLE_L2_BITS]))
            {
                return(0 != (l2_table[ucs4 - l2_base_ucs4] & CODE_POINT_IS_XID_START));
            }
        }
        else
        {
            if(NULL != (l2_table = list_gotoitemcontents_opt(U8, &list_block_DerivedCoreProperties, l2_base_ucs4)))
            {
                return(0 != (l2_table[ucs4 - l2_base_ucs4] & CODE_POINT_IS_XID_START));
            }
        }

        if(status_ok(load_DerivedCoreProperties(ucs4)))
            continue;

        return(FALSE);
    }
}

/*
DerivedCoreProperties: XID_Continue
ID_Continue modified for closure under NFKx
Modified as described in UAX #15
NOTE: Cf characters should be filtered out.
NOTE: Does NOT remove the non-NFKx characters.
      Merely ensures that if isIdentifer(string) then isIdentifier(NFKx(string))
NOTE: See UAX #31 for more information
-where-
ID_Continue: Characters that can continue an identifier.
Generated from: ID_Start + Mn+Mc+Nd+Pc + Other_ID_Continue
*/

_Check_return_
extern BOOL
ucs4_is_XID_continue(
    _InVal_     UCS4 ucs4)
{
    UCS4 l2_base_ucs4 = UCS4_TABLE_L2_BASE(ucs4);
    PC_U8 l2_table;

    if(status_fail(ucs4_validate(ucs4)))
        return(FALSE);

    for(;;)
    {
        if(ucs4 < UCS4_TABLE_SIZE)
        {
            if(NULL != (l2_table = l1_table_DerivedCoreProperties[l2_base_ucs4 >> UCS4_TABLE_L2_BITS]))
            {
                return(0 != (l2_table[ucs4 - l2_base_ucs4] & CODE_POINT_IS_XID_CONTINUE));
            }
        }
        else
        {
            if(NULL != (l2_table = list_gotoitemcontents_opt(U8, &list_block_DerivedCoreProperties, l2_base_ucs4)))
            {
                return(0 != (l2_table[ucs4 - l2_base_ucs4] & CODE_POINT_IS_XID_CONTINUE));
            }
        }

        if(status_ok(load_DerivedCoreProperties(ucs4)))
            continue;

        return(FALSE);
    }
}

/*
DerivedCoreProperties: Grapheme_Extend
Generated from: Me + Mn + Other_Grapheme_Extend
Note: depending on an application's interpretation of Co (private use),
they may be either in Grapheme_Base, or in Grapheme_Extend, or in neither.
*/

_Check_return_
extern BOOL
ucs4_is_grapheme_extend(
    _InVal_     UCS4 ucs4)
{
    UCS4 l2_base_ucs4 = UCS4_TABLE_L2_BASE(ucs4);
    PC_U8 l2_table;

    if(status_fail(ucs4_validate(ucs4)))
        return(FALSE);

    for(;;)
    {
        if(ucs4 < UCS4_TABLE_SIZE)
        {
            if(NULL != (l2_table = l1_table_DerivedCoreProperties[l2_base_ucs4 >> UCS4_TABLE_L2_BITS]))
            {
                return(0 != (l2_table[ucs4 - l2_base_ucs4] & CODE_POINT_IS_GRAPHEME_EXTEND));
            }
        }
        else
        {
            if(NULL != (l2_table = list_gotoitemcontents_opt(U8, &list_block_DerivedCoreProperties, l2_base_ucs4)))
            {
                return(0 != (l2_table[ucs4 - l2_base_ucs4] & CODE_POINT_IS_GRAPHEME_EXTEND));
            }
        }

        if(status_ok(load_DerivedCoreProperties(ucs4)))
            continue;

        return(FALSE);
    }
}

/*
no use found in Fireworkz for
DerivedCoreProperties:
    Math,
    ID_Start,
    ID_Continue,
    Default_Ignorable_Code_Point
    Grapheme_Base,
    Grapheme_Link
*/

/*
CaseFolding: Case Fold Simple
*/

static BOOL read_failed_CaseFolding = 0;

static P_UCS4 l1_table_case_fold_simple[UCS4_TABLE_L1_SIZE];

static LIST_BLOCK list_block_case_fold_simple;

_Check_return_
static UCS4
load_case_fold_simple(
    _In_        UCS4 ucs4)
{
    UCS4 l2_base_ucs4 = UCS4_TABLE_L2_BASE(ucs4);
    STATUS status;

    if(!read_failed_CaseFolding)
    {   /* Read and Parse Unicode Character Database file CaseFolding.txt, adding entries for this range */
        TCHARZ filename[BUF_MAX_PATHSTRING];

        if(status_fail(status = file_find_on_path(filename, elemof32(filename), TEXT("UCD") FILE_DIR_SEP_TSTR TEXT("CaseFolding.txt"))))
        {
            read_failed_CaseFolding = 1;
        }
        else
        {
            FILE_HANDLE file_handle = 0;

            if(!status_done(status = t5_file_open(filename, file_open_read, &file_handle, FALSE))) /* fail silently */
            {
                read_failed_CaseFolding = 1;
            }
            else
            {
                P_UCS4 l2_table;

                if(ucs4 < UCS4_TABLE_SIZE)
                {
                    const U32 l2_size_elem = UCS4_TABLE_L2_SIZE;
                    l2_table = l1_table_case_fold_simple[l2_base_ucs4 >> UCS4_TABLE_L2_BITS] = al_ptr_alloc_elem(UCS4, l2_size_elem, &status);
                }
                else
                {
                    const U32 l2_size_bytes = UCS4_TABLE_L2_SIZE * sizeof32(*l2_table);
                    l2_table = collect_add_entry_bytes(UCS4, &list_block_case_fold_simple, NULL, l2_size_bytes, l2_base_ucs4, &status);
                }

                if(NULL == l2_table)
                {
                    read_failed_CaseFolding = 1;
                }
                else
                {
                    U8 buffer[256];

                    { /* Default mapping is from character to itself */
                    UCS4 code_point;

                    for(code_point = l2_base_ucs4; code_point < l2_base_ucs4 + UCS4_TABLE_L2_SIZE; ++code_point)
                    {
                        l2_table[code_point - l2_base_ucs4] = code_point;
                    }

                    } /*block*/

                    /* The entries in this file are in the following machine-readable format: */
                    /* <code>; <status>; <mapping>; # <name> */

                    while(status_ok(status = file_gets(buffer, elemof32(buffer), file_handle)))
                    {
                        char *ptr = buffer;
                        UCS4 code_point;
                        char entry_status;
                        UCS4 entry_mapping;

                        if(EOF_READ == status)
                            break;

                        if((CH_NULL == *ptr) || (CH_NUMBER_SIGN == *ptr))
                            continue;

                        code_point = (UCS4) strtoul(ptr, &ptr, 16);

                        if(CH_SEMICOLON != *ptr)
                            continue;

                        while(*++ptr == CH_SPACE)
                            continue;

                        if((code_point < l2_base_ucs4) || (code_point >= (l2_base_ucs4 + UCS4_TABLE_L2_SIZE)))
                            continue;

                        entry_status = *ptr++;

                        if(CH_SEMICOLON != *ptr)
                            continue;

                        while(*++ptr == CH_SPACE)
                            continue;

                        /* we can only handle C and S mappings (not F or T) - see the standard */
                        if(('C' != entry_status) && ('S' != entry_status))
                            continue;

                        entry_mapping = (UCS4) strtoul(ptr, &ptr, 16);

                        if(CH_SEMICOLON != *ptr)
                            continue;

                        while(*++ptr == CH_SPACE)
                            continue;

#if TRACE_ALLOWED && 1
                        tracef(TRACE_OUT | TRACE_ANY, TEXT("CF: U+%.6X->U+%.6X %s\n"), code_point, entry_mapping, report_sbstr(ptr));
#endif

                        l2_table[code_point - l2_base_ucs4] = entry_mapping;
                    }
                }

                status_assert(t5_file_close(&file_handle));

                if(NULL != l2_table)
                    return(l2_table[ucs4 - l2_base_ucs4]);
            }
        }
    }

    /* we can use default built-in table if no allocation has ever been done */
    if((NULL == l1_table_case_fold_simple[0]) && ucs4_is_sbchar(ucs4))
    {
        return(t5__ctype_sortbyte(t5__ctype_sbchar[ucs4]));
    }

    return(ucs4);
}

_Check_return_
extern UCS4
ucs4_case_fold_simple(
    _InVal_     UCS4 ucs4)
{
    UCS4 l2_base_ucs4 = UCS4_TABLE_L2_BASE(ucs4);
    PC_UCS4 p_ucs4_l2;

    if(status_fail(ucs4_validate(ucs4)))
        return(UCH_REPLACEMENT_CHARACTER);

    if(ucs4 < UCS4_TABLE_SIZE)
    {
        if(NULL != (p_ucs4_l2 = l1_table_case_fold_simple[l2_base_ucs4 >> UCS4_TABLE_L2_BITS]))
        {
            return(p_ucs4_l2[ucs4 - l2_base_ucs4]);
        }
    }
    else
    {
        if(NULL != (p_ucs4_l2 = list_gotoitemcontents_opt(UCS4, &list_block_case_fold_simple, l2_base_ucs4)))
        {
            return(p_ucs4_l2[ucs4 - l2_base_ucs4]);
        }
    }

    return(load_case_fold_simple(ucs4));
}

/* end of ucs4.c */
