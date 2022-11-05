/* mrofmun.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Automatic number format generator */

/* James A. Dixon 21-Jul-1994; MRJC November 1994 rewritten */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/mrofmun.h"

/******************************************************************************
*
* the task:
* split an input string into data, type of data
* and format string,
* given a list of possible format strings
*
* the algorithm:
* 1) remove all characters not in the set '0-9', CH_MINUS_SIGN__BASIC, CH_COLON, CH_FULL_STOP, making a 'data input string'
* 2) make a string containing all non-digit characters in the order they were encountered making a 'format string'
* 3) pass 'data input string' to ss_recog_constant, producing a constant and a data type
* 4) print constant through each supplied numform, then
*    a) split numform result into 'data input' and 'format' again
*    b) send 'data input' to ss_recog_constant giving constant and data type
*    c) compare constant, data_type and how many characters match in format strings to determine best match
*
******************************************************************************/

_Check_return_
static inline STATUS
mrofmun_add_to_both(
    _InoutRef_  P_QUICK_BLOCK p_quick_block_data,
    _InoutRef_  P_QUICK_BLOCK p_quick_block_format,
    _InVal_     U8 u8)
{
    status_return(quick_block_byte_add(p_quick_block_data,   u8));
           return(quick_block_byte_add(p_quick_block_format, u8));
}

_Check_return_
static STATUS
mrofmun_split_string(
    _InoutRef_  P_QUICK_BLOCK p_quick_block_data,
    _InoutRef_  P_QUICK_BLOCK p_quick_block_format,
    _In_z_      PC_U8Z p_u8_in,
    _InVal_     BOOL date_pass)
{
    STATUS status = STATUS_OK;
    PC_U8Z p_u8 = p_u8_in;

    StrSkipSpaces(p_u8); /* skip leading spaces */

    while(status_ok(status) && (CH_NULL != PtrGetByte(p_u8)))
    {
        U8 u8 = PtrGetByte(p_u8);

        if(date_pass && ((u8 == g_ss_recog_context.date_sep_char)           ||
                         (u8 == g_ss_recog_context.alternate_date_sep_char) ||
                         (u8 == CH_FORWARDS_SLASH)                          ||
                         (u8 == CH_HYPHEN_MINUS) /*ISO*/                    ) )
        {
            status = mrofmun_add_to_both(p_quick_block_data, p_quick_block_format, u8);
        }
        else if(date_pass && (u8 == g_ss_recog_context.time_sep_char))
        {
            status = mrofmun_add_to_both(p_quick_block_data, p_quick_block_format, u8);
        }
        else if(u8 == g_ss_recog_context.decimal_point_char)
        {   /* don't consider decimal point as part of format */
            status = quick_block_byte_add(p_quick_block_data, u8);
        }
        else if(u8 == g_ss_recog_context.thousands_char)
        { /*EMPTY*/ } /* ignore thousands separators */
        else
        {
            switch(u8)
            {
            /* characters which are allowed in the number data */
            case CH_MINUS_SIGN__BASIC:
            case CH_LEFT_PARENTHESIS:
            case CH_RIGHT_PARENTHESIS:
            case CH_PERCENT_SIGN:
                if(date_pass)
                    status = quick_block_byte_add(p_quick_block_format, u8);
                else
                    status = mrofmun_add_to_both(p_quick_block_data, p_quick_block_format, u8);
                break;

            /* spaces are significant in combined date time */
            case CH_SPACE:
            /* digits */
            case CH_DIGIT_ZERO:
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case CH_DIGIT_NINE:
                status = quick_block_byte_add(p_quick_block_data, u8);
                break;

            /* add other characters to format string */
            default:
                status = quick_block_byte_add(p_quick_block_format, u8);
                break;
            }
        }

        p_u8 += 1;
    }

    if(status_ok(status))
        status = quick_block_nullch_add(p_quick_block_data);

    if(status_ok(status))
        status = quick_block_nullch_add(p_quick_block_format);

    return(status);
}

static S32 /* match_count */
mrofmun_formats_compare(
    _In_z_      PC_U8Z p_format_input /* format characters from input string */,
    _In_z_      PC_U8Z p_format_now)
{
    S32 match = 0;
    PC_U8Z p_u8_input = p_format_input;
    PC_U8Z p_u8_now = p_format_now;

    while(p_u8_now[0] && p_u8_input[0])
    {
        int ch_input = p_u8_input[0];
        PC_U8Z p_u8_t;

        if(NULL == (p_u8_t = strchr(p_u8_now, ch_input)))
            break;

        match += 1;
        p_u8_now = p_u8_t + 1;
        p_u8_input += 1;
    }

    /* did we use up all the format in the input ? */
    if(p_u8_input[0] || p_u8_now[0])
        match = -1;

    trace_3(TRACE_APP_SKEL,
            TEXT("mrofmun_formats_compare in: {%s}, now: {%s}, match: ") S32_TFMT,
            report_sbstr(p_format_input),
            report_sbstr(p_format_now),
            match);

    return(match);
}

_Check_return_
static S32
mrofmun_recog_constant(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_z_      PC_U8Z p_u8_in,
    _InVal_     BOOL date_pass)
{
    STATUS status = STATUS_OK;
    PC_U8Z p_u8 = p_u8_in;
    S32 res = 0;
    BOOL negative = FALSE;
    BOOL percent = FALSE;
    BOOL must_be_date = FALSE;
    BOOL had_digit = FALSE;
    QUICK_BLOCK_WITH_BUFFER(quick_block, 32);
    quick_block_with_buffer_setup(quick_block);

    trace_1(TRACE_APP_SKEL, TEXT("mrofmun_recog_constant: %s"), report_ustr((PC_USTR) p_u8_in));

    CODE_ANALYSIS_ONLY(ss_data_set_blank(p_ss_data));

    while(status_ok(status) && (CH_NULL != PtrGetByte(p_u8)))
    {
        U8 u8 = PtrGetByte(p_u8);

        if(date_pass && ((u8 == g_ss_recog_context.date_sep_char)           ||
                         (u8 == g_ss_recog_context.alternate_date_sep_char) ||
                         (u8 == CH_FORWARDS_SLASH)                          ||
                         (u8 == CH_HYPHEN_MINUS) /*ISO*/                    ) )
        {
            if(!had_digit)
            {
                status = STATUS_FAIL;
                break;
            }

            /* we allow these characters through to ss_recog_constant */
            if(u8 == CH_FORWARDS_SLASH)
                u8 = g_ss_recog_context.date_sep_char;
            status = quick_block_byte_add(&quick_block, u8);
            must_be_date = TRUE;
        }
        else if(date_pass && (u8 == g_ss_recog_context.time_sep_char))
        {
            if(!had_digit)
            {
                status = STATUS_FAIL;
                break;
            }

            status = quick_block_byte_add(&quick_block, u8);
            must_be_date = TRUE;
        }
        else if(u8 == g_ss_recog_context.decimal_point_char)
        {
            had_digit = TRUE;
            status = quick_block_byte_add(&quick_block, g_ss_recog_context.decimal_point_char);
        }
        else if(u8 == g_ss_recog_context.thousands_char)
        {
            if(!had_digit)
            {
                status = STATUS_FAIL;
                break;
            }
        }
        else
        {
            switch(u8)
            {
            case CH_MINUS_SIGN__BASIC:
                if(had_digit)
                    status = STATUS_FAIL;
                else
                    negative = !negative;
                break;

            case CH_LEFT_PARENTHESIS:
                if(had_digit || negative)
                    status = STATUS_FAIL;
                else
                    negative = TRUE;
                break;

            case CH_RIGHT_PARENTHESIS:
                if(!had_digit || !negative)
                    status = STATUS_FAIL;
                break;

            case CH_PERCENT_SIGN:
                if(!had_digit || percent)
                    status = STATUS_FAIL;
                else
                    percent = TRUE;
                break;

            /* we allow these characters through to ss_recog_constant */
            case CH_COLON:
            case CH_SPACE:
                status = quick_block_byte_add(&quick_block, u8);
                break;

            case CH_DIGIT_ZERO:
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case CH_DIGIT_NINE:
                had_digit = TRUE;
                status = quick_block_byte_add(&quick_block, u8);
                break;

            default: default_unhandled();
                break;
            }
        }

        p_u8 += 1;
    }

    if(status_ok(status))
        status = quick_block_nullch_add(&quick_block);

    if(status_ok(status))
    {
        res = ss_recog_constant(p_ss_data, (PC_USTR) quick_block_str(&quick_block) /*, FALSE*/);

        if(negative)
        {
            switch(ss_data_get_data_id(p_ss_data))
            {
            default:
                break;

            /*case DATA_ID_LOGICAL:*/
            case DATA_ID_WORD16:
            case DATA_ID_WORD32:
                ss_data_set_integer(p_ss_data, -ss_data_get_integer(p_ss_data));
                break;

            case DATA_ID_REAL:
                ss_data_set_real(p_ss_data, -ss_data_get_real(p_ss_data));
                break;
            }
        }

        if(percent)
        {
            switch(ss_data_get_data_id(p_ss_data))
            {
            default:
                break;

            /*case DATA_ID_LOGICAL:*/
            case DATA_ID_WORD16:
            case DATA_ID_WORD32:
                ss_data_set_real(p_ss_data, (F64) ss_data_get_integer(p_ss_data));

                /*FALLTHRU*/

            case DATA_ID_REAL:
                ss_data_set_real(p_ss_data, ss_data_get_real(p_ss_data) / 100.0);
                break;
            }
        }

        /* check we got a date when necessary */
        if(must_be_date && !ss_data_is_date(p_ss_data))
            res = 0;
    }

    quick_block_dispose(&quick_block);

    return(res);
}

_Check_return_
extern STATUS
autoformat(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _OutRef_    P_STYLE_HANDLE p_style_handle_out,
    _In_z_      PC_USTR ustr,
    _InRef_     PC_ARRAY_HANDLE p_array_handle_mrofmuns_in) /* array of mrofmun_entries */
{
    STATUS status = STATUS_OK;
    UINT pass;

    *p_style_handle_out = STYLE_HANDLE_NONE;
    ss_data_set_blank(p_ss_data_out);

    for(pass = 1; pass <= 2; ++pass)
    {
        BOOL date_pass = (1 == pass); /* SKS 29apr95 make it do date detection! */
        SS_DATA ss_data;
        QUICK_BLOCK_WITH_BUFFER(quick_block_data, 32);
        QUICK_BLOCK_WITH_BUFFER(quick_block_format, 32);
        quick_block_with_buffer_setup(quick_block_data);
        quick_block_with_buffer_setup(quick_block_format);

        status_break(status = mrofmun_split_string(&quick_block_data, &quick_block_format, (PC_U8Z) ustr, date_pass));

        if( (quick_block_bytes(&quick_block_format) <= 1) /* empty format? */ ||
            (mrofmun_recog_constant(&ss_data, quick_block_str(&quick_block_data), date_pass) <= 0) )
        {
            ss_data_set_error(&ss_data, STATUS_FAIL);
        }
        else
        {
            ARRAY_INDEX i;
            S32 max_match = -1;

            for(i = 0; i < array_elements(p_array_handle_mrofmuns_in); ++i)
            {
                const PC_MROFMUN_ENTRY p_mrofmun_entry = array_ptrc(p_array_handle_mrofmuns_in, MROFMUN_ENTRY, i);
                QUICK_UBLOCK_WITH_BUFFER(quick_ublock_numform, 48);
                quick_ublock_with_buffer_setup(quick_ublock_numform);

                if(date_pass)
                {
                    if(NULL == p_mrofmun_entry->numform_parms.ustr_numform_datetime)
                        continue;
                }
                else
                {
                    if(NULL == p_mrofmun_entry->numform_parms.ustr_numform_numeric)
                        continue;
                }

                /* pass value to numform & compare with ustr */
                status = numform(&quick_ublock_numform, P_QUICK_TBLOCK_NONE, &ss_data, &p_mrofmun_entry->numform_parms);

                if(status_ok(status))
                {
                    if(quick_ublock_bytes(&quick_ublock_numform) > 1)
                    {
                        QUICK_BLOCK_WITH_BUFFER(quick_block_data_n, 32);
                        QUICK_BLOCK_WITH_BUFFER(quick_block_format_n, 32);
                        quick_block_with_buffer_setup(quick_block_data_n);
                        quick_block_with_buffer_setup(quick_block_format_n);

                        status = mrofmun_split_string(&quick_block_data_n,
                                                      &quick_block_format_n,
                                                      (PC_U8Z) quick_ublock_ustr(&quick_ublock_numform), date_pass);

                        if(status_ok(status))
                        {
                            SS_DATA ss_data_n;

                            if(mrofmun_recog_constant(&ss_data_n, quick_block_str(&quick_block_data_n), date_pass) > 0)
                            {
                                if(0 == ss_data_compare(&ss_data, &ss_data_n, TRUE, TRUE))
                                {
                                    S32 match_count = mrofmun_formats_compare(quick_block_str(&quick_block_format),
                                                                              quick_block_str(&quick_block_format_n));
                                    if(match_count > max_match)
                                    {
                                        *p_style_handle_out = p_mrofmun_entry->style_handle;
                                        max_match = match_count;
                                    }
                                }
                            }
                        }

                        quick_block_dispose(&quick_block_data_n);
                        quick_block_dispose(&quick_block_format_n);
                    }
                }

                quick_ublock_dispose(&quick_ublock_numform);
            }
        }

        quick_block_dispose(&quick_block_data);
        quick_block_dispose(&quick_block_format);

        if(STYLE_HANDLE_NONE != *p_style_handle_out)
        {
            *p_ss_data_out = ss_data;
            break;
        }
    }

    return(status);
}

/* end of mrofmun.c */
