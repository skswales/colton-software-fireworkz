/* ev_fnstr.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* String function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

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
    int pass = 1;

    exec_func_ignore_parms();

    do  {
        U32 o_idx = 0;
        U32 i_idx = 0;

        /* clean the string of unprintable chracters during transfer */
        while(i_idx < ss_data_get_string_size(args[0]))
        {
            U32 bytes_of_char;
            UCS4 ucs4 = uchars_char_decode_off(ss_data_get_string(args[0]), i_idx, /*ref*/bytes_of_char);

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

            if(status_fail(ss_string_make_uchars(p_ss_data_res, NULL, bytes_of_buffer)))
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

    exec_func_ignore_parms();

    if(0 != ss_data_get_string_size(args[0]))
        code_result = (S32) PtrGetByte(ss_data_get_string(args[0]));

    ss_data_set_integer(p_ss_data_res, code_result);
}

/******************************************************************************
*
* STRING dollar(number {, decimals})
*
* a bit like our old STRING() function but with a currency symbol and thousands commas (Excel)
*
******************************************************************************/

PROC_EXEC_PROTO(c_dollar)
{
    STATUS status;
    S32 decimal_places = 2;
    S32 i;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 50);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 50);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if(n_args > 1)
    {
        decimal_places = MIN(127, ss_data_get_integer(args[1])); /* can be larger for formatting */
        decimal_places = MAX(  0, decimal_places);
    }

    round_common(args, n_args, p_ss_data_res, RPN_FNV_ROUND); /* ROUND: uses n_args, args[0] and {args[1]}, yields DATA_ID_REAL (or an integer type) */

    status = quick_ublock_ucs4_add(&quick_ublock_format, UCH_POUND_SIGN); /* FIXME */

    if(status_ok(status))
        status = quick_ublock_ustr_add(&quick_ublock_format, USTR_TEXT("#,,###0"));

    if((decimal_places > 0) && status_ok(status))
        status = quick_ublock_a7char_add(&quick_ublock_format, CH_FULL_STOP);

    for(i = 0; (i < decimal_places) && status_ok(status); ++i)
        status = quick_ublock_a7char_add(&quick_ublock_format, CH_NUMBER_SIGN);

    if(status_ok(status))
        status = quick_ublock_nullch_add(&quick_ublock_format);

    if(status_ok(status)) /* ev_numform() is happy with anything from round_common() */
        status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), p_ss_data_res);

    exec_func_status_return(p_ss_data_res, status);

    status_assert(ss_string_make_ustr(p_ss_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);
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

    if(ss_data_get_string_size(args[0]) == ss_data_get_string_size(args[1]))
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
    S32 find_len = ss_data_get_string_size(args[1]);
    S32 start_n = 1;
    PC_UCHARS uchars;

    exec_func_ignore_parms();

    if(n_args > 2)
        start_n = ss_data_get_integer(args[2]);

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
    S32 decimal_places = 2;
    BOOL no_commas = FALSE;
    S32 i;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 50);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 50);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if(n_args > 1)
    {
        decimal_places = MIN(127, ss_data_get_integer(args[1])); /* can be larger for formatting */
        decimal_places = MAX(  0, decimal_places);
    }

    if(n_args > 2)
        no_commas = (0 != ss_data_get_integer(args[2]));

    round_common(args, n_args, p_ss_data_res, RPN_FNV_ROUND); /* ROUND: uses n_args, args[0] and {args[1]}, yields DATA_ID_REAL (or an integer type) */

    status = quick_ublock_ustr_add(&quick_ublock_format, no_commas ? USTR_TEXT("0") : USTR_TEXT("#,,###0"));

    if((decimal_places > 0) && status_ok(status))
        status = quick_ublock_a7char_add(&quick_ublock_format, CH_FULL_STOP);

    for(i = 0; (i < decimal_places) && status_ok(status); ++i)
        status = quick_ublock_a7char_add(&quick_ublock_format, CH_NUMBER_SIGN);

    if(status_ok(status))
        status = quick_ublock_nullch_add(&quick_ublock_format);

    if(status_ok(status)) /* ev_numform() is happy with anything from round_common() */
        status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), p_ss_data_res);

    exec_func_status_return(p_ss_data_res, status);

    status_assert(ss_string_make_ustr(p_ss_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);
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
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
        quick_ublock_with_buffer_setup(quick_ublock);

        /* decode cell contents as it would appear in the formula line i.e. with alternate / foreign UI if wanted */
        status = ev_cell_decode_ui(&quick_ublock, p_ev_cell, ev_slr_docno(&args[0]->arg.slr));
        exec_func_status_return(p_ss_data_res, status);

        status_assert(ss_string_make_uchars(p_ss_data_res, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock)));

        quick_ublock_dispose(&quick_ublock);
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
    U32 len = 0;
    P_UCHARS uchars;

    exec_func_ignore_parms();

    for(arg_idx = 0; arg_idx < n_args; ++arg_idx)
    {
        const U32 arg_len = ss_data_get_string_size(args[arg_idx]);
        len += arg_len;
    }

    if(status_fail(ss_string_make_uchars(p_ss_data_res, NULL, len)))
        return;

    if(0 == len)
        return;

    uchars = p_ss_data_res->arg.string_wr.uchars;

    for(arg_idx = 0; arg_idx < n_args; ++arg_idx)
    {
        const U32 arg_len = ss_data_get_string_size(args[arg_idx]);
        memcpy32(uchars, ss_data_get_string(args[arg_idx]), arg_len);
        uchars_IncBytes_wr(uchars, arg_len);
    }
}

/******************************************************************************
*
* STRING left(string {, n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_left)
{
    const S32 len = ss_data_get_string_size(args[0]);
    S32 n = 1;

    exec_func_ignore_parms();

    if(n_args > 1)
    {
        if((n = ss_data_get_integer(args[1])) < 0)
            exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
    }

    n = MIN(n, len);
    status_assert(ss_string_make_uchars(p_ss_data_res, ss_data_get_string(args[0]), n));
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
    S32 len = ss_data_get_string_size(args[0]);
    S32 start = ss_data_get_integer(args[1]) - 1; /* ensure that this will not go -ve */
    S32 n = ss_data_get_integer(args[2]);

    exec_func_ignore_parms();

    if( (ss_data_get_integer(args[1]) <= 0) || (n < 0) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    start = MIN(start, len);
    n = MIN(n, len - start);
    status_assert(ss_string_make_uchars(p_ss_data_res, uchars_AddBytes(ss_data_get_string(args[0]), start), n));
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
        ss_data_set_integer(p_ss_data_res, ss_data_get_logical(args[0]));
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
    S32 start_n_chars = ss_data_get_integer(args[1]) - 1; /* one-based API */
    S32 excise_n_chars = ss_data_get_integer(args[2]);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, BUF_EV_MAX_STRING_LEN);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if((start_n_chars < 0) || (excise_n_chars < 0))
        status = EVAL_ERR_ARGRANGE;
    else
    {
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

    exec_func_status_return(p_ss_data_res, status);

    quick_ublock_dispose(&quick_ublock_result);
}

/******************************************************************************
*
* STRING rept("text", n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rept)
{
    const U32 len = ss_data_get_string_size(args[0]);
    S32 n = ss_data_get_integer(args[1]);
    P_UCHARS uchars;
    S32 x;

    exec_func_ignore_parms();

    if(n < 0)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    if(status_fail(ss_string_make_uchars(p_ss_data_res, NULL, (U32) n * len)))
        return;

    if(0 == n * len)
        return;

    uchars = p_ss_data_res->arg.string_wr.uchars;

    for(x = 0; x < n; ++x)
    {
        memcpy32(uchars, ss_data_get_string(args[0]), len);
        uchars_IncBytes_wr(uchars, len);
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
    const S32 len = ss_data_get_string_size(args[0]);
    S32 n = 1;

    exec_func_ignore_parms();

    if(n_args > 1)
    {
        if((n = ss_data_get_integer(args[1])) < 0)
            exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
    }

    n = MIN(n, len);
    status_assert(ss_string_make_uchars(p_ss_data_res, uchars_AddBytes(ss_data_get_string(args[0]), (len - n)), n));
}

/******************************************************************************
*
* STRING string(n {, x})
*
******************************************************************************/

PROC_EXEC_PROTO(c_string)
{
    S32 decimal_places = 2;
    F64 rounded_value;
    U8Z buffer[BUF_EV_MAX_STRING_LEN];

    exec_func_ignore_parms();

    if(n_args > 1)
    {
        decimal_places = MIN(127, ss_data_get_integer(args[1])); /* can be large for formatting */
        decimal_places = MAX(  0, decimal_places);
    }

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
    U32 instance_to_replace = 0;

    exec_func_ignore_parms();

    if(n_args > 3)
        instance_to_replace = ss_data_get_integer(args[3]);

    if(0 == old_text_size)
    {   /* nothing to be matched for substitution - just copy all the current text over to the result */
        status_assert(ss_string_make_uchars(p_ss_data_res, text_uchars, text_size));
    }
    else
    {
        U32 instance = 1;
        U32 text_i = 0;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, BUF_EV_MAX_STRING_LEN);
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
        status_consume(ss_string_make_uchars(p_ss_data_res, NULL, 0));
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
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 50);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 50);
    quick_ublock_with_buffer_setup(quick_ublock_format);
    quick_ublock_with_buffer_setup(quick_ublock_result);

    exec_func_ignore_parms();

    if(status_ok(status = quick_ublock_uchars_add(&quick_ublock_format, ss_data_get_string(args[1]), ss_data_get_string_size(args[1]))))
        if(status_ok(status = quick_ublock_nullch_add(&quick_ublock_format)))
            status = ev_numform(&quick_ublock_result, quick_ublock_ustr(&quick_ublock_format), args[0]);

    exec_func_status_return(p_ss_data_res, status);

    status_assert(ss_string_make_ustr(p_ss_data_res, quick_ublock_ustr(&quick_ublock_result)));

    quick_ublock_dispose(&quick_ublock_format);
    quick_ublock_dispose(&quick_ublock_result);
}

/******************************************************************************
*
* STRING trim("text")
*
******************************************************************************/

PROC_EXEC_PROTO(c_trim)
{
    PC_UCHARS s_ptr;
    PC_UCHARS e_ptr;
    P_UCHARS o_buf = P_UCHARS_NONE;
    int pass = 1;

    exec_func_ignore_parms();

    s_ptr = ss_data_get_string(args[0]);
    e_ptr = uchars_AddBytes(s_ptr, ss_data_get_string_size(args[0]));

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
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 50);
        quick_ublock_with_buffer_setup(quick_ublock);
        PC_USTR ptr;

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
#else
            PC_USTR buffer = quick_ublock_ustr(&quick_ublock);

            ss_data_set_real_try_integer(p_ss_data_res, ui_strtod(ustr_bptr(buffer), &ptr));

            if(0 == PtrDiffBytesU32(ptr, buffer))
                ss_data_set_integer(p_ss_data_res, 0); /* SKS 08sep97 now behaves as documented */
#endif
        }

        quick_ublock_dispose(&quick_ublock);

        exec_func_status_return(p_ss_data_res, status);
    }
}

/* end of ev_fnstr.c */
