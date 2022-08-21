/* ev_fnstr.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* String function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "cmodules/unicode/u20A0.h" /* for UCH_EURO_CURRENCY_SIGN */

/******************************************************************************
*
* String functions
*
******************************************************************************/

/******************************************************************************
*
* STRING char(n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_char)
{
    UCS4 ucs4 = (UCS4) ss_data_get_integer(args[0]);

    exec_func_ignore_parms();

    if( (ucs4 >= 256U) || (CH_INLINE == ucs4) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    {
    UCHARB uchar_buffer[8];
#if USTR_IS_SBSTR
    U32 bytes_of_char = 1;
    uchar_buffer[0] = (U8) ucs4;
#else
    U32 bytes_of_char = uchars_char_encode(uchars_bptr(uchar_buffer), sizeof32(uchar_buffer), ucs4);
#endif
    status_assert(ss_string_make_uchars(p_ss_data_res, uchars_bptr(uchar_buffer), bytes_of_char));
    } /*block*/
}

/******************************************************************************
*
* STRING clean("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_clean)
{
    U32 bytes_of_buffer = 0;
    const PC_UCHARS uchars = ss_data_get_string(args[0]);
    const U32 uchars_n = ss_data_get_string_size(args[0]);
    int pass = 1;

    exec_func_ignore_parms();

    do  {
        U32 o_idx = 0;
        U32 i_idx = 0;

        /* clean the string of unprintable characters during transfer */
        while(i_idx < uchars_n)
        {
            U32 bytes_of_char;
            UCS4 ucs4 = uchars_char_decode_off(uchars, i_idx, /*ref*/bytes_of_char);

            i_idx += bytes_of_char;

#if !USTR_IS_SBSTR
            if(!ucs4_validate(ucs4))
                continue;
#endif

            if(ucs4_is_sbchar(ucs4) && !t5_isprint((U8) ucs4))
                continue;

            if(pass == 2)
            {
                assert((o_idx + bytes_of_char) <= bytes_of_buffer);
                (void) uchars_char_encode_off(p_ss_data_res->arg.string_wr.uchars, bytes_of_buffer, o_idx, ucs4);
            }

            o_idx += bytes_of_char;
        }

        if(pass == 1)
        {
            bytes_of_buffer = o_idx;

            if(status_fail(ss_string_allocate(p_ss_data_res, bytes_of_buffer)))
                return;
        }
    }
    while(++pass <= 2);
}

/******************************************************************************
*
* INTEGER code("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_code)
{
    S32 code_result = 0; /* SKS 20160801 allow empty string */
    const PC_UCHARS uchars = ss_data_get_string(args[0]);
    const U32 uchars_n = ss_data_get_string_size(args[0]);

    exec_func_ignore_parms();

    if(0 != uchars_n)
        code_result = (S32) PtrGetByte(uchars);

    ss_data_set_integer(p_ss_data_res, code_result);
}

/******************************************************************************
*
* STRING dollar(number {, decimals})
*
* a bit like our old STRING() function but with a currency symbol and thousands commas (Excel)
*
******************************************************************************/

_Check_return_
static inline S32
get_decimal_places(
    _InRef_     PC_SS_DATA p_ss_data)
{
    S32 decimal_places = ss_data_get_integer(p_ss_data);
    decimal_places = MIN(127, decimal_places); /* can be large for formatting */
    decimal_places = MAX(  0, decimal_places);
    return(decimal_places);
}

_Check_return_
static STATUS
dollar_number_format_string(
    _InRef_     P_QUICK_UBLOCK p_quick_ublock,
    _InVal_     S32 decimal_places,
    _InVal_     bool euro_suffix)
{
    STATUS status = STATUS_OK;
    S32 i;

    if(!euro_suffix)
        status = quick_ublock_ucs4_add(p_quick_ublock, UCH_POUND_SIGN); /* FIXME */

    if(status_ok(status))
        status = quick_ublock_ustr_add(p_quick_ublock, USTR_TEXT("#,"));

    if(status_ok(status))
        status = quick_ublock_a7char_add(p_quick_ublock, get_ss_recog_context_alt(thousands_char));

    if(status_ok(status))
        status = quick_ublock_ustr_add(p_quick_ublock, USTR_TEXT("###0"));

    if(decimal_places > 0)
    {
        if(status_ok(status))
            status = quick_ublock_a7char_add(p_quick_ublock, get_ss_recog_context_alt(decimal_point_char));

        for(i = 0; (i < decimal_places) && status_ok(status); ++i)
            status = quick_ublock_a7char_add(p_quick_ublock, CH_DIGIT_ZERO);
    }

    if(euro_suffix)
    {
        if(status_ok(status))
            status = quick_ublock_a7char_add(p_quick_ublock, CH_SPACE);

#if USTR_IS_SBSTR
        if(status_ok(status))
            status = quick_ublock_ucs4_add(p_quick_ublock, ucs4_to_sbchar_force_with_codepage(UCH_EURO_CURRENCY_SIGN, get_system_codepage(), UCH_SPACE));
#else
        if(status_ok(status))
            status = quick_ublock_ucs4_add(p_quick_ublock, UCH_EURO_CURRENCY_SIGN);
#endif
    }

    return(status);
}

PROC_EXEC_PROTO(c_dollar)
{
    STATUS status = STATUS_OK;
    const S32 decimal_places = (n_args > 1) ? get_decimal_places(args[1]) : 2; /* keep signed - can use negative decimal_places to round before decimal point */
    bool euro_suffix = false;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 64);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 64);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    round_common(args, n_args, p_ss_data_res, RPN_FNV_ROUND); /* ROUND: uses n_args, args[0] and {args[1]}, yields DATA_ID_REAL (or an integer type) */

    euro_suffix = (CH_COMMA == get_ss_recog_context_alt(decimal_point_char)); /* FIXME */

    /* positive format */
    status = dollar_number_format_string(&quick_ublock_format, decimal_places, euro_suffix);

    /* followed by negative format (in parentheses) */
    if(status_ok(status))
        status = quick_ublock_sbstr_add(&quick_ublock_format, "_;(");

    if(status_ok(status))
        status = dollar_number_format_string(&quick_ublock_format, decimal_places, euro_suffix);

    if(status_ok(status))
        status = quick_ublock_a7char_add(&quick_ublock_format, CH_RIGHT_PARENTHESIS);

    if(status_ok(status))
        status = quick_ublock_nullch_add(&quick_ublock_format);

    if(status_ok(status)) /* ev_numform() is happy with anything from round_common() */
        status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), p_ss_data_res);

    if(status_ok(status))
        status_assert(ss_string_make_ustr(p_ss_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);

    exec_func_status_return(p_ss_data_res, status);
}

/******************************************************************************
*
* LOGICAL exact("text1", "text2")
*
******************************************************************************/

PROC_EXEC_PROTO(c_exact)
{
    bool exact_result = false;

    exec_func_ignore_parms();

    if( ss_data_get_string_size(args[0]) == ss_data_get_string_size(args[1]) )
        if(0 == memcmp32(ss_data_get_string(args[0]), ss_data_get_string(args[1]), ss_data_get_string_size(args[0])))
            exact_result = true;

    ss_data_set_logical(p_ss_data_res, exact_result);
}

/******************************************************************************
*
* INTEGER find("find_text", "in_text" {, start_n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_find)
{
    S32 find_result = 0;
    const S32 find_len = (S32) ss_data_get_string_size(args[1]);
    S32 start_n = (n_args > 2) ? ss_data_get_integer(args[2]) : 1;
    PC_UCHARS uchars;

    exec_func_ignore_parms();

    if(start_n <= 0)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    start_n -= 1; /* SKS 12apr95 could have caused exception with find("str", "") as find_len == 0 */

    if( start_n > find_len)
        start_n = find_len;

    if(NULL != (uchars = (PC_UCHARS) memstr32(uchars_AddBytes(ss_data_get_string(args[1]), start_n), find_len - start_n, ss_data_get_string(args[0]), ss_data_get_string_size(args[0])))) /*strstr replacement*/
        find_result = 1 + PtrDiffBytesS32(uchars, ss_data_get_string(args[1]));

    ss_data_set_integer(p_ss_data_res, find_result);
}

/******************************************************************************
*
* STRING fixed(number {, decimals {, no_commas}})
*
* a bit like our old STRING() function but with thousands commas (Excel)
*
******************************************************************************/

PROC_EXEC_PROTO(c_fixed)
{
    STATUS status;
    const S32 decimal_places = (n_args > 1) ? get_decimal_places(args[1]) : 2; /* keep signed - can use negative decimal_places to round before decimal point */
    const bool no_commas = (n_args > 2) ? ss_data_get_logical(args[2]) : false;
    S32 i;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 64);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 64);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    round_common(args, n_args, p_ss_data_res, RPN_FNV_ROUND); /* ROUND: uses n_args, args[0] and {args[1]}, yields DATA_ID_REAL (or an integer type) */

    status = quick_ublock_ustr_add(&quick_ublock_format, no_commas ? USTR_TEXT("0") : USTR_TEXT("#,,###0"));

    if((decimal_places > 0) && status_ok(status))
        status = quick_ublock_a7char_add(&quick_ublock_format, CH_FULL_STOP);

    for(i = 0; (i < decimal_places) && status_ok(status); ++i)
        status = quick_ublock_a7char_add(&quick_ublock_format, CH_DIGIT_ZERO);

    if(status_ok(status))
        status = quick_ublock_nullch_add(&quick_ublock_format);

    if(status_ok(status)) /* ev_numform() is happy with anything from round_common() */
        status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), p_ss_data_res);

    if(status_ok(status))
        status_assert(ss_string_make_ustr(p_ss_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);

    exec_func_status_return(p_ss_data_res, status);
}

/******************************************************************************
*
* STRING formula_text(ref)
*
******************************************************************************/

PROC_EXEC_PROTO(c_formula_text)
{
    P_EV_CELL p_ev_cell;

    exec_func_ignore_parms();

    if(ev_travel(&p_ev_cell, &args[0]->arg.slr) > 0)
    {
        STATUS status;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 128);
        quick_ublock_with_buffer_setup(quick_ublock);

        /* decode cell contents as it would appear in the formula line i.e. with alternate / foreign UI if wanted */
        status = ev_cell_decode_ui(&quick_ublock, p_ev_cell, ev_slr_docno(&args[0]->arg.slr));

        if(status_ok(status))
            status_assert(ss_string_make_uchars(p_ss_data_res, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock)));

        quick_ublock_dispose(&quick_ublock);

        exec_func_status_return(p_ss_data_res, status);
    }
}

/******************************************************************************
*
* STRING join(string1 {, stringn ...})
*
******************************************************************************/

PROC_EXEC_PROTO(c_join)
{
    S32 arg_idx;
    uint64_t len = 0;
    P_UCHARS uchars;

    exec_func_ignore_parms();

    for(arg_idx = 0; arg_idx < n_args; ++arg_idx)
    {
        const U32 arg_uchars_n = ss_data_get_string_size(args[arg_idx]);
        len += arg_uchars_n;
        if(len > SS_STRING_BYTE_LIMIT)
            exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
    }

    if(status_fail(ss_string_allocate(p_ss_data_res, (U32) len)))
        return;

    if(0 == len)
        return;

    uchars = p_ss_data_res->arg.string_wr.uchars;

    for(arg_idx = 0; arg_idx < n_args; ++arg_idx)
    {
        const PC_UCHARS arg_uchars = ss_data_get_string(args[arg_idx]);
        const U32 arg_uchars_n = ss_data_get_string_size(args[arg_idx]);
        memcpy32(uchars, arg_uchars, arg_uchars_n);
        uchars_IncBytes_wr(uchars, arg_uchars_n);
    }
}

/******************************************************************************
*
* STRING left(string {, n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_left)
{
    const PC_UCHARS uchars = ss_data_get_string(args[0]);
    const U32 uchars_n = ss_data_get_string_size(args[0]);
    S32 n = 1;

    exec_func_ignore_parms();

    if(n_args > 1)
    {
        n = ss_data_get_integer(args[1]);

        if(n < 0)
            exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
    }

    status_assert(ss_string_make_uchars(p_ss_data_res, uchars, MIN((U32) n, uchars_n)));
}

/******************************************************************************
*
* INTEGER length(string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_length)
{
    S32 length_result = ss_data_get_string_size(args[0]);

    exec_func_ignore_parms();

    ss_data_set_integer(p_ss_data_res, length_result);
}

/******************************************************************************
*
* STRING lower("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_lower)
{
    exec_func_ignore_parms();

    if(status_ok(ss_string_dup(p_ss_data_res, args[0])))
    {
        P_UCHARS uchars = p_ss_data_res->arg.string_wr.uchars;
        const U32 len = p_ss_data_res->arg.string_wr.size;
        U32 offset = 0;

        while(offset < len)
        {
            U32 bytes_of_char;
            const UCS4 ucs4 = uchars_char_decode_off(uchars, offset, /*ref*/bytes_of_char);
            const UCS4 ucs4_x = t5_ucs4_lowercase(ucs4);

            if(ucs4_x != ucs4)
            {
                const U32 new_bytes_of_char = uchars_bytes_of_char_encoding(ucs4_x);
                assert(new_bytes_of_char == bytes_of_char);
                if(new_bytes_of_char == bytes_of_char)
                    (void) uchars_char_encode_off(uchars, len, offset, ucs4_x);
            }

            offset += bytes_of_char;
            assert(offset <= len);
        }
    }
}

/******************************************************************************
*
* STRING mid(string, start, n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_mid)
{
    const PC_UCHARS uchars = ss_data_get_string(args[0]);
    const S32 len = (S32) ss_data_get_string_size(args[0]);
    S32 n = ss_data_get_integer(args[2]);
    S32 start;

    exec_func_ignore_parms();

    if( (ss_data_get_integer(args[1]) <= 0) || (n < 0) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    start = ss_data_get_integer(args[1]) - 1; /* one-based API: ensured that this will not go -ve */
    start = MIN(start, len);
    n = MIN(n, len - start);
    status_assert(ss_string_make_uchars(p_ss_data_res, uchars_AddBytes(uchars, start), n));
}

/******************************************************************************
*
* VALUE n
*
******************************************************************************/

PROC_EXEC_PROTO(c_n)
{
    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_LOGICAL:
        ss_data_set_integer(p_ss_data_res, (S32) ss_data_get_logical(args[0]));
        break;

    case DATA_ID_DATE:
        /* might have just a date component, which is representable as an integer */
        ss_data_set_real_try_integer(p_ss_data_res, ss_date_to_serial_number(ss_data_get_date(args[0])));
        break;

    case DATA_ID_STRING:
        /* can't discriminate here between strings and text cells */
        ss_data_set_integer(p_ss_data_res, 0);
        break;

    default:
        status_assert(ss_data_resource_copy(p_ss_data_res, args[0]));
        break;
    }
}

/******************************************************************************
*
* STRING proper("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_proper)
{
    exec_func_ignore_parms();

    if(status_ok(ss_string_dup(p_ss_data_res, args[0])))
    {
        const U32 len = p_ss_data_res->arg.string_wr.size;
        P_UCHARS uchars = p_ss_data_res->arg.string_wr.uchars;
        U32 offset = 0;
        U32 offset_in_word = 0;

        while(offset < len)
        {
            U32 bytes_of_char;
            const UCS4 ucs4 = uchars_char_decode_off(uchars, offset, /*ref*/bytes_of_char);

            if(!t5_ucs4_is_alphabetic(ucs4) && !t5_ucs4_is_decimal_digit(ucs4))
                offset_in_word = 0;
            else
            {
                UCS4 ucs4_x;

                if(0 == offset_in_word)
                    ucs4_x = t5_ucs4_uppercase(ucs4);
                else
                    ucs4_x = t5_ucs4_lowercase(ucs4);

                if(ucs4_x != ucs4)
                {
                    const U32 new_bytes_of_char = uchars_bytes_of_char_encoding(ucs4_x);
                    assert(new_bytes_of_char == bytes_of_char);
                    if(new_bytes_of_char == bytes_of_char)
                        (void) uchars_char_encode_off(uchars, len, offset, ucs4_x);
                }

                ++offset_in_word;
            }

            offset += bytes_of_char;
            assert(offset <= len);
        }
    }
}

/******************************************************************************
*
* STRING replace("text", start_n, chars_n, "with_text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_replace)
{
    STATUS status = STATUS_OK;
    const S32 len = ss_data_get_string_size(args[0]);
    S32 start_n_chars;
    S32 excise_n_chars = ss_data_get_integer(args[2]);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 64);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if((ss_data_get_integer(args[1]) <= 0) || (excise_n_chars < 0))
        status = EVAL_ERR_ARGRANGE;
    else
    {
        start_n_chars = ss_data_get_integer(args[1]) - 1; /* one-based API */

        if( start_n_chars > len)
            start_n_chars = len;

        if(0 != start_n_chars)
            status = quick_ublock_uchars_add(&quick_ublock_result, ss_data_get_string(args[0]), start_n_chars);

        /* NB even if 0 == excise_n_chars; that's just an insert operation */
        if(status_ok(status))
            status = quick_ublock_uchars_add(&quick_ublock_result, ss_data_get_string(args[3]), ss_data_get_string_size(args[3]));

        if(status_ok(status) && ((start_n_chars + excise_n_chars) < len))
        {
            S32 end_n_chars = len - (start_n_chars + excise_n_chars);

            status = quick_ublock_uchars_add(&quick_ublock_result, uchars_AddBytes(ss_data_get_string(args[0]), (start_n_chars + excise_n_chars)), end_n_chars);
        }

        if(status_ok(status))
            status_assert(ss_string_make_uchars(p_ss_data_res, quick_ublock_uchars(&quick_ublock_result), quick_ublock_bytes(&quick_ublock_result)));
    }

    quick_ublock_dispose(&quick_ublock_result);

    exec_func_status_return(p_ss_data_res, status);
}

/******************************************************************************
*
* STRING rept("text", n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rept)
{
    const PC_UCHARS uchars_in = ss_data_get_string(args[0]);
    const U32 len = ss_data_get_string_size(args[0]);
    S32 n = ss_data_get_integer(args[1]);
    P_UCHARS uchars_out;
    S32 uchars_n;
    INT64_WITH_INT32_OVERFLOW int64_with_int32_overflow;
    S32 x;

    exec_func_ignore_parms();

    uchars_n = int32_multiply_check_overflow(n, len, &int64_with_int32_overflow);

    if( (uchars_n < 0) || int64_with_int32_overflow.f_overflow )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    if(status_fail(ss_string_allocate(p_ss_data_res, (U32) uchars_n)))
        return;

    if(0 == uchars_n)
        return;

    uchars_out = p_ss_data_res->arg.string_wr.uchars;

    for(x = 0; x < n; ++x)
    {
        memcpy32(uchars_out, uchars_in, len);
        uchars_IncBytes_wr(uchars_out, len);
    }
}

/******************************************************************************
*
* STRING reverse("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_reverse)
{
    exec_func_ignore_parms();

    if(status_ok(ss_string_dup(p_ss_data_res, args[0])))
    {
        U32 x;
        const U32 half_len = p_ss_data_res->arg.string_wr.size / 2;
        P_U8 ptr_lo = (P_U8) p_ss_data_res->arg.string_wr.uchars;
        P_U8 ptr_hi = ptr_lo + p_ss_data_res->arg.string_wr.size;

        for(x = 0; x < half_len; ++x)
        {
            U8 c = *--ptr_hi; /* SKS 04jul96 this was complete twaddle before */
            *ptr_hi = *ptr_lo;
            *ptr_lo++ = c;
        }
    }
}

/******************************************************************************
*
* STRING right(string {, n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_right)
{
    const PC_UCHARS uchars = ss_data_get_string(args[0]);
    const S32 len = (S32) ss_data_get_string_size(args[0]);
    S32 n = 1;

    exec_func_ignore_parms();

    if(n_args > 1)
    {
        n = ss_data_get_integer(args[1]);

        if(n < 0)
            exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
    }

    n = MIN(n, len);
    status_assert(ss_string_make_uchars(p_ss_data_res, uchars_AddBytes(uchars, (len - n)), n));
}

/******************************************************************************
*
* STRING string(n {, x})
*
******************************************************************************/

PROC_EXEC_PROTO(c_string)
{
    const S32 decimal_places = (n_args > 1) ? get_decimal_places(args[1]) : 2; /* keep signed - can use negative decimal_places to round before decimal point */
    F64 rounded_value;
    U8Z buffer[127 + 1 + 32]; /* somewhat bigger than the MAX in get_decimal_places() */

    exec_func_ignore_parms();

    round_common(args, n_args, p_ss_data_res, RPN_FNV_ROUND); /* ROUND: uses n_args, args[0] and {args[1]}, yields DATA_ID_REAL (or an integer type) */

    rounded_value = ss_data_get_number(p_ss_data_res); /* take care now we can get integer types back */

    consume_int(xsnprintf(buffer, elemof32(buffer), "%.*f", (int) decimal_places, rounded_value));

    status_assert(ss_string_make_ustr(p_ss_data_res, ustr_bptr(buffer)));
}

/******************************************************************************
*
* STRING substitute(text, old_text, new_text {, instance_num})
*
******************************************************************************/

PROC_EXEC_PROTO(c_substitute)
{
    STATUS status = STATUS_OK;
    U32 text_size = ss_data_get_string_size(args[0]);
    PC_UCHARS text_uchars = ss_data_get_string(args[0]);
    U32 old_text_size = ss_data_get_string_size(args[1]);
    PC_UCHARS old_text_uchars = ss_data_get_string(args[1]);
    U32 new_text_size = ss_data_get_string_size(args[2]);
    PC_UCHARS new_text_uchars = ss_data_get_string(args[2]);
    U32 instance_to_replace = (n_args > 3) ? ss_data_get_integer(args[3]) : 0;

    exec_func_ignore_parms();

    if(0 == old_text_size)
    {   /* nothing to be matched for substitution - just copy all the current text over to the result */
        status_assert(ss_string_make_uchars(p_ss_data_res, text_uchars, text_size));
    }
    else
    {
        U32 instance = 1;
        U32 text_i = 0;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 64);
        quick_ublock_with_buffer_setup(quick_ublock_result);

        while(text_i < text_size)
        {
            const U32 text_size_remain = text_size - text_i;
            PC_UCHARS text_to_test;
            U32 bytes_of_char;

            if(text_size_remain < old_text_size)
                break;

            text_to_test = uchars_AddBytes(text_uchars, text_i);

            if( ((instance == instance_to_replace) || (0 == instance_to_replace)) &&
                (0 == memcmp32(text_to_test, old_text_uchars, old_text_size)) )
            {   /* found an instance of old_text in text to replace */

                /* copy the replacement string (which may be empty) to the output */
                status_break(status = quick_ublock_uchars_add(&quick_ublock_result, new_text_uchars, new_text_size));

                /* skip over all the matched characters in the current string */
                text_i += old_text_size;

                ++instance;

                continue;
            }

            /* otherwise copy the character from the current string to the output */
            bytes_of_char = uchars_bytes_of_char(text_to_test);

            status_break(status = quick_ublock_uchars_add(&quick_ublock_result, text_to_test, bytes_of_char));

            text_i += bytes_of_char;

            assert(text_i <= text_size);
        }

        if(status_ok(status))
            status_assert(ss_string_make_uchars(p_ss_data_res, quick_ublock_uchars(&quick_ublock_result), quick_ublock_bytes(&quick_ublock_result)));

        quick_ublock_dispose(&quick_ublock_result);
    }
}

/******************************************************************************
*
* STRING t(value)
*
******************************************************************************/

PROC_EXEC_PROTO(c_t)
{
    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    default:
        status_consume(ss_string_allocate(p_ss_data_res, 0));
        break;

    case DATA_ID_STRING:
        status_consume(ss_string_dup(p_ss_data_res, args[0]));
        break;
    }
}

/******************************************************************************
*
* STRING text(n, numform_string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_text)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 64);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 64);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if(status_ok(status = quick_ublock_uchars_add(&quick_ublock_format, ss_data_get_string(args[1]), ss_data_get_string_size(args[1]))))
        if(status_ok(status = quick_ublock_nullch_add(&quick_ublock_format)))
            status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), args[0]);

    if(status_ok(status))
        status_assert(ss_string_make_ustr(p_ss_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);

    exec_func_status_return(p_ss_data_res, status);
}

/******************************************************************************
*
* STRING trim("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_trim)
{
    PC_UCHARS s_ptr = ss_data_get_string(args[0]);
    PC_UCHARS e_ptr = uchars_AddBytes(s_ptr, ss_data_get_string_size(args[0]));
    P_UCHARS o_buf = P_UCHARS_NONE;
    int pass = 1;

    exec_func_ignore_parms();

    while((s_ptr < e_ptr) && (PtrGetByte(s_ptr) == CH_SPACE)) /* skip leading spaces */
        uchars_IncByte(s_ptr);

    while((e_ptr > s_ptr) && (PtrGetByteOff(e_ptr, -1) == CH_SPACE)) /* skip trailing spaces */
        uchars_DecByte(e_ptr);

    do  {
        PC_UCHARS i_ptr;
        U32 o_idx = 0;
        BOOL gap = FALSE;

        /* crunge runs of multiple spaces to a single space during transfer */
        for(i_ptr = s_ptr; i_ptr < e_ptr; uchars_IncByte(i_ptr))
        {
            if(CH_SPACE == PtrGetByte(i_ptr))
            {
                if(gap)
                    continue;
                else
                    gap = TRUE;
            }
            else
            {
                gap = FALSE;
            }

            if(1 == pass)
                ++o_idx; /* sizing pass */
            else
                o_buf[o_idx++] = PtrGetByte(i_ptr);
        }

        if(1 == pass)
        {
            status_break(ss_string_allocate(p_ss_data_res, o_idx));

            o_buf = p_ss_data_res->arg.string_wr.uchars;
        }
    }
    while(++pass <= 2);
}

/******************************************************************************
*
* STRING upper("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_upper)
{
    exec_func_ignore_parms();

    if(status_ok(ss_string_dup(p_ss_data_res, args[0])))
    {
        P_UCHARS uchars = p_ss_data_res->arg.string_wr.uchars;
        const U32 len = p_ss_data_res->arg.string_wr.size;
        U32 offset = 0;

        while(offset < len)
        {
            U32 bytes_of_char;
            const UCS4 ucs4 = uchars_char_decode_off(uchars, offset, /*ref*/bytes_of_char);
            const UCS4 ucs4_x = t5_ucs4_uppercase(ucs4);

            if(ucs4_x != ucs4)
            {
                const U32 new_bytes_of_char = uchars_bytes_of_char_encoding(ucs4_x);
                assert(new_bytes_of_char == bytes_of_char);
                if(new_bytes_of_char == bytes_of_char)
                    (void) uchars_char_encode_off(uchars, len, offset, ucs4_x);
            }

            offset += bytes_of_char;
            assert(offset <= len);
        }
    }
}

/******************************************************************************
*
* NUMBER value("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_value)
{
    PC_UCHARS s_ptr = ss_data_get_string(args[0]);
    U32 len = ss_data_get_string_size(args[0]);

    exec_func_ignore_parms();

    while((0 != len) && (PtrGetByte(s_ptr) == CH_SPACE)) /* skip leading spaces prior to copy */
    {
        uchars_IncByte(s_ptr);
        --len;
    }

    if(0 == len)
    {
        ss_data_set_integer(p_ss_data_res, 0); /* SKS 08sep97 now behaves as documented */
    }
    else
    {
        PC_UCHARS e_ptr = uchars_AddBytes(s_ptr, len);
        STATUS status;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
        quick_ublock_with_buffer_setup(quick_ublock);

        /* we are guaranteed here to have at least one byte to transfer */
        while(/*(e_ptr > s_ptr) &&*/ (PtrGetByteOff(e_ptr, -1) == CH_SPACE)) /* skip trailing spaces prior to copy */
        {
            uchars_DecByte(e_ptr);
            --len;
        }

        /* just copy the trimmed string and CH_NULL-terminate */
        if(status_ok(status = quick_ublock_uchars_add(&quick_ublock, s_ptr, len)))
        if(status_ok(status = quick_ublock_nullch_add(&quick_ublock)))
        {
#if 0
            ev_recog_constant_using_autoformat(p_ss_data_res, ev_slr_docno(p_cur_slr), quick_ublock_ustr(&quick_ublock));
#elif 1
            PC_USTR ustr = quick_ublock_ustr(&quick_ublock);
            SS_RECOG_CONTEXT ss_recog_context;

            ss_recog_context_push(&ss_recog_context); /* recognise constants from cells as if typed by user with current UI settings */

            { /* quick hack removing leading common currency symbols, thousands separators before a decimal point */
            PC_USTR ustr_in = ustr;
            P_USTR ustr_out = de_const_cast(P_USTR, ustr);
            U8 ch = *ustr_in;
            bool had_decimal_point = false;

            for( ; CH_NULL != ch; ustr_IncByte(ustr_in))
            {
                ch = PtrGetByte(ustr_in);

                switch(ch)
                {
                case UCH_POUND_SIGN:
                case UCH_DOLLAR_SIGN:
#if RISCOS || WINDOWS
                case 0x80: /* UCH_EURO_CURRENCY_SIGN in both Acorn Extended Latin-1 and Windows-1252 */
#endif
                // case UCH_EURO_CURRENCY_SIGN:
                    if(ustr_in == ustr)
                        continue;
                    break;

                default:
                    if(ss_recog_context.decimal_point_char == ch)
                        had_decimal_point = true;

                    if( (ss_recog_context.thousands_char == ch) && !had_decimal_point )
                        continue;

                    break;
                }

                PtrPutByte(ustr_out, ch); ustr_IncByte_wr(ustr_out);
            }
            } /*block */

            if( !status_done(ss_recog_constant(p_ss_data_res, ustr)) ||
                ss_data_is_string(p_ss_data_res) )
            {
                ss_data_set_blank(p_ss_data_res); /* don't leave half-recognised things lying around, or strings (numbers, dates, errors, arrays OK) */
            }

            ss_recog_context_pull(&ss_recog_context);
#else
            PC_USTR ptr;
            PC_USTR buffer = quick_ublock_ustr(&quick_ublock);
            F64 f64 = ui_strtod(ustr_bptrc(buffer), &ptr);

            if(0 != PtrDiffBytesU32(ptr, buffer))
                ss_data_set_real_try_integer(p_ss_data_res, f64);
#endif
        }

        if(ss_data_is_blank(p_ss_data_res))
            ss_data_set_integer(p_ss_data_res, 0); /* SKS 08sep97 now behaves as documented */

        quick_ublock_dispose(&quick_ublock);

        exec_func_status_return(p_ss_data_res, status);
    }
}

/* end of ev_fnstr.c */
