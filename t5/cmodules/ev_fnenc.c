/* ev_fnenc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2019-2022 Stuart Swales */

/* Engineering function routines (base conversion) for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

/******************************************************************************
*
* Engineering functions - conversion
*
******************************************************************************/

#define DEFAULT_PLACES INT_MIN

/*
parse input
*/

_Check_return_
static STATUS
check_trailing_garbage(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _In_z_      const char * end_ptr)
{
    StrSkipSpaces(end_ptr);

    if(CH_NULL != *end_ptr)
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ODF_NUM));

    return(STATUS_OK);
}

_Check_return_
static STATUS
from_binary_decode_real(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _In_        F64 f64,
    _OutRef_    P_S32 p_s32)
{
    char buffer[16];
    char * end_ptr;
    unsigned long ul;

    f64 = real_floor(f64);

    *p_s32 = 0;

    if((f64 < 0.0) || (f64 > 1111111111.0)) /* range check input, ten decimal digits max (NB fp just for consistency with others) */
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

    consume_int(xsnprintf(buffer, sizeof(buffer), "%.0f", f64));

    ul = strtoul(buffer, &end_ptr, 2); /* read as desired base */

    status_return(check_trailing_garbage(p_ss_data_res, end_ptr));

    if(ULONG_MAX == ul)
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

    if(ul >= (1UL << 10)) /* (paranoid) range check output, ten binary digits max */
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

    *p_s32 = (S32) ul; /* always positive */

    return(STATUS_OK);
}

_Check_return_
static STATUS
from_binary_decode_string(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _OutRef_    P_S32 p_s32)
{
    const U32 radix = 2;
    const U32 shift = 1;
    U32 buf_idx;
    S32 s32 = 0;

    *p_s32 = 0;

    for(buf_idx = 0; buf_idx < uchars_n; ++buf_idx)
    {
        const U8 u8 = PtrGetByteOff(uchars, buf_idx);
        U32 digit;

        if(ucs4_is_decimal_digit(u8))
        {
            digit = ucs4_decimal_digit_value(u8);
        }
        else
        {
            break;
        }

        if(digit >= radix)
            break;

        /* before appending this binary digit check that we will not overflow ten bits (i.e anything in bit 9) */
        if(0 != (s32 >> (10U - shift)))
            return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

        s32 = (s32 << shift) | digit;
    }

    if( (0 == buf_idx) /* nothing read? */ || (buf_idx < uchars_n) /* trailing garbage? */ )
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ODF_NUM));

    *p_s32 = s32;

    return(STATUS_OK);
}

_Check_return_
static STATUS
from_binary(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data,
    _OutRef_    P_S32 p_s32)
{
    S32 s32;

    *p_s32 = 0;

    if(ss_data_is_real(p_ss_data))
        status_return(from_binary_decode_real(p_ss_data_res, ss_data_get_real(p_ss_data), &s32));
    else
        status_return(from_binary_decode_string(p_ss_data_res, ss_data_get_string(p_ss_data), ss_data_get_string_size(p_ss_data), &s32));

    /* sign extend using the tenth bit (shift left so bit 9 is at bit 31, then ASR back) */
    s32 = s32 << (31 - 9);
    s32 = s32 >> (31 - 9);

    *p_s32 = s32;

    return(STATUS_OK);
}

_Check_return_
static STATUS
from_decimal_decode_string(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _OutRef_    P_F64 p_f64)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
    quick_ublock_with_buffer_setup(quick_ublock);

    *p_f64 = 0.0;

    /* need a CH_NULL-terminated string to parse */
    if(status_ok(status = quick_ublock_uchars_add(&quick_ublock, uchars, uchars_n)))
        status = quick_ublock_nullch_add(&quick_ublock);

    if(status_ok(status))
    {
        PC_USTR end_ptr;

        *p_f64 = real_floor(ustrtod(quick_ublock_ustr(&quick_ublock), &end_ptr));

        status = check_trailing_garbage(p_ss_data_res, (const char *) end_ptr);
    }

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

static inline STATUS
from_decimal(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data,
    _OutRef_    P_F64 p_f64)
{
    if(ss_data_is_real(p_ss_data))
        *p_f64 = real_floor(ss_data_get_real(p_ss_data));
    else
        status_return(from_decimal_decode_string(p_ss_data_res, ss_data_get_string(p_ss_data), ss_data_get_string_size(p_ss_data), p_f64));

    return(STATUS_OK);
}

_Check_return_
static STATUS
from_hexadecimal_decode_real(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _In_        F64 f64,
    _OutRef_    P_S64 p_s64)
{
    char buffer[64];
    char * end_ptr;
    unsigned long long ull;

    f64 = real_floor(f64);

    *p_s64 = 0;

    if((f64 < 0.0) || (f64 > 9999999999.0)) /* range check input, ten decimal digits max (NB decimal representation of largest doesn't fit in 32 bits) */
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

    consume_int(xsnprintf(buffer, sizeof(buffer), "%.0f", f64));

    ull = strtoull(buffer, &end_ptr, 16); /* read as desired base */

    status_return(check_trailing_garbage(p_ss_data_res, end_ptr));

    if(ULLONG_MAX == ull)
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

    if(ull >= (1ULL << 40)) /* (paranoid) range check output, ten hexadecimal digits max */
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

    *p_s64 = (S64) ull; /* always positive */

    return(STATUS_OK);
}

_Check_return_
static STATUS
from_hexadecimal_decode_string(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _OutRef_    P_S64 p_s64)
{
    const U32 radix = 16;
    const U32 shift = 4;
    U32 buf_idx;
    S64 s64 = 0;

    *p_s64 = 0;

    for(buf_idx = 0; buf_idx < uchars_n; ++buf_idx)
    {
        const U8 u8 = PtrGetByteOff(uchars, buf_idx);
        U32 digit;

        if(ucs4_is_hexadecimal_digit(u8))
        {
            digit = ucs4_hexadecimal_digit_value(u8);
        }
        else
        {
            break;
        }

        /* if(digit >= radix) break; */
        assert(digit < radix);
        UNREFERENCED_LOCAL_VARIABLE(radix);

        /* before appending this hexadecimal digit check that we will not overflow forty bits (i.e anything in bits 39..36) */
        if(0 != (s64 >> (40U - shift)))
            return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

        s64 = (s64 << shift) | digit;
    }

    if( (0 == buf_idx) /* nothing read? */ || (buf_idx < uchars_n) /* trailing garbage? */ )
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ODF_NUM));

    *p_s64 = s64;

    return(STATUS_OK);
}

_Check_return_
static STATUS
from_hexadecimal(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data,
    _OutRef_    P_S64 p_s64)
{
    S64 s64;

    *p_s64 = 0;

    if(ss_data_is_real(p_ss_data))
        status_return(from_hexadecimal_decode_real(p_ss_data_res, ss_data_get_real(p_ss_data), &s64));
    else
        status_return(from_hexadecimal_decode_string(p_ss_data_res, ss_data_get_string(p_ss_data), ss_data_get_string_size(p_ss_data), &s64));

    /* sign extend using the fortieth bit (shift left so bit 39 is at bit 63, then ASR back) */
    s64 = s64 << (63 - 39);
    s64 = s64 >> (63 - 39);

    *p_s64 = s64;

    return(STATUS_OK);
}

_Check_return_
static STATUS
from_octal_decode_real(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _In_        F64 f64,
    _OutRef_    P_S32 p_s32)
{
    char buffer[16];
    char * end_ptr;
    unsigned long ul;

    f64 = real_floor(f64);

    *p_s32 = 0;

    if((f64 < 0.0) || (f64 > 7777777777.0)) /* range check input, ten decimal digits max (NB decimal representation of largest doesn't fit in 32 bits) */
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

    consume_int(xsnprintf(buffer, sizeof(buffer), "%.0f", f64));

    ul = strtoul(buffer, &end_ptr, 8); /* read as desired base */

    status_return(check_trailing_garbage(p_ss_data_res, end_ptr));

    if(ULONG_MAX == ul)
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

    if(ul >= (1UL << 30)) /* (paranoid) range check output, ten octal digits max */
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

    *p_s32 = (S32) ul; /* always positive */

    return(STATUS_OK);
}

_Check_return_
static STATUS
from_octal_decode_string(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _OutRef_    P_S32 p_s32)
{
    const U32 radix = 8;
    const U32 shift = 3;
    S32 s32 = 0;
    U32 buf_idx;

    *p_s32 = 0;

    for(buf_idx = 0; buf_idx < uchars_n; ++buf_idx)
    {
        const U8 u8 = PtrGetByteOff(uchars, buf_idx);
        U32 digit;

        if(ucs4_is_decimal_digit(u8))
        {
            digit = ucs4_decimal_digit_value(u8);
        }
        else
        {
            break;
        }

        if(digit >= radix)
            break;

        /* before appending this octal digit check that we will not overflow thirty bits (i.e anything in bits 29..27) */
        if(0 != (s32 >> (30U - shift)))
            return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE));

        s32 = (s32 << shift) | digit;
    }

    if( (0 == buf_idx) /* nothing read? */ || (buf_idx < uchars_n) /* trailing garbage? */ )
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_ODF_NUM));

    *p_s32 = s32;

    return(STATUS_OK);
}

_Check_return_
static STATUS
from_octal(
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_SS_DATA p_ss_data,
    _OutRef_    P_S32 p_s32)
{
    S32 s32;

    *p_s32 = 0;

    if(ss_data_is_real(p_ss_data))
        status_return(from_octal_decode_real(p_ss_data_res, ss_data_get_real(p_ss_data), &s32));
    else
        status_return(from_octal_decode_string(p_ss_data_res, ss_data_get_string(p_ss_data), ss_data_get_string_size(p_ss_data), &s32));

    /* sign extend using the thirtieth bit (shift left so bit 29 is at bit 31, then ASR back) */
    s32 = s32 << (31 - 29);
    s32 = s32 >> (31 - 29);

    *p_s32 = s32;

    return(STATUS_OK);
}

/*
format output
*/

static void
to_binary(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     S64 s64,
    _In_        S32 places)
{
    S32 s32;

    /* limit acceptable range to ten bits, including sign */
    if((s64 > ((1 << (10-1))-1)) || (s64 < (-(1 << (10-1)))))
    {
        ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    s32 = (S32) s64;

    if(DEFAULT_PLACES != places)
    {
        if((places <= 0) || (places > 10))
        {
            ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
            return;
        }

        if(s32 < 0)
        {   /* override: negative numbers will be fully padded to ten digits */
            places = DEFAULT_PLACES;
        }
    }

    {
    U8Z buffer[32];
    U32 buf_idx;
    S32 bit_number = 10; /* output all [9:0] */
    S32 leading_zeros = 0;

    for(buf_idx = 0; --bit_number >= 0; ++buf_idx)
    {
        const U8 bit = (s32 >> bit_number) & 1;

        buffer[buf_idx] = bit ? CH_DIGIT_ONE : CH_DIGIT_ZERO;
    }

    buffer[buf_idx] = CH_NULL;

    /* count leading zeros in the converted number */
    for(buf_idx = 0; buf_idx < 10-1; ++buf_idx) /* always leave last zero to be output */
    {
        if(buffer[buf_idx] != CH_DIGIT_ZERO)
            break;

        ++leading_zeros;
    }

    buf_idx = leading_zeros;

    if(DEFAULT_PLACES != places)
    {
        if(strlen32(buffer + leading_zeros) > (U32) places)
        {   /* converted number did not fit in the specified places */
            ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
            return;
        }

        /* skip some of the leading zeros to pad the width to the specified places */
        buf_idx = 10 - places;
    }

    status_assert(ss_string_make_ustr(p_ss_data_res, ustr_bptr(buffer + buf_idx)));
    } /*block*/
}

static void
to_hexadecimal(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     S64 s64,
    _In_        S32 places)
{
    /* limit acceptable range to forty bits, including sign */
    if((s64 > ((1LL << (40-1))-1)) || (s64 < (-(1LL << (40-1)))))
    {
        ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    if(DEFAULT_PLACES != places)
    {
        if((places <= 0) || (places > 10))
        {
            ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
            return;
        }

        if(s64 < 0)
        {   /* override: negative numbers will be fully padded to sixteen digits, then truncated to ten digits */
            places = DEFAULT_PLACES;
        }
    }

    {
    U8Z buffer[32];
    U32 buf_idx = 0;

    if(DEFAULT_PLACES != places)
    {   /* output as 'places' hexadecimal digits field. number always positive, leading zero padding as needed */
        consume_int(xsnprintf(buffer, sizeof32(buffer), "%0*llX", (int) places, s64));
        if(strlen32(buffer) > (U32) places)
        {   /* converted number did not fit in the specified places */
            ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
            return;
        }
    }
    else
    {   /* default output format */
        consume_int(xsnprintf(buffer, sizeof32(buffer), "%llX", s64));

        /* NB. negative numbers will have fully padded to sixteen digits with no option of truncation here, so skip on copying below */
        if(s64 < 0)
        {   /* (64 - 40) / 4  =  16 - 10  =  6 */
            buf_idx = 6;
        }
    }

    status_assert(ss_string_make_ustr(p_ss_data_res, ustr_bptr(buffer + buf_idx)));
    } /*block*/
}

static void
to_decimal(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     S64 s64)
{
    ss_data_set_real_try_integer(p_ss_data_res, (F64) s64);
}

static void
to_octal(
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     S64 s64,
    _In_        S32 places)
{
    S32 s32;

    /* limit acceptable range to thirty bits, including sign */
    if((s64 > ((1 << (30-1))-1)) || (s64 < (-(1 << (30-1)))))
    {
        ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    s32 = (S32) s64;

    if(DEFAULT_PLACES != places)
    {
        if((places <= 0) || (places > 10))
        {
            ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
            return;
        }

        if(s32 < 0)
        {   /* override: negative numbers will be fully padded to ten digits */
            places = DEFAULT_PLACES;
        }
    }

    {
    U8Z buffer[32];
    U32 buf_idx = 0;

    if(DEFAULT_PLACES != places)
    {   /* output as 'places' octal digits field. number always positive, leading zero padding as needed */
        consume_int(xsnprintf(buffer, sizeof32(buffer), "%0*o", (int) places, s32));
        if(strlen32(buffer) > (U32) places)
        {   /* converted number did not fit in the specified places */
            ss_data_set_error(p_ss_data_res, EVAL_ERR_ARGRANGE);
            return;
        }
    }
    else
    {   /* default output format */
        consume_int(xsnprintf(buffer, sizeof32(buffer), "%o", s32));

        /*NB. negative numbers will have fully padded to eleven digits with no option of truncation here, so skip on copying below */
        if(s32 < 0)
        {   /* e.g -1 -> 37777777777, so to form our 30 bit result we discard the top two bits (i.e. the first '3') */
            buf_idx = 1;
        }
    }

    status_assert(ss_string_make_ustr(p_ss_data_res, ustr_bptr(buffer + buf_idx)));
    } /*block*/
}

/******************************************************************************
*
* NUMBER bin2dec(number_string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_bin2dec)
{
    S32 s32;

    if(status_fail(from_binary(p_ss_data_res, args[0], &s32)))
        return;

    exec_func_ignore_parms();

    to_decimal(p_ss_data_res, s32);
}

/******************************************************************************
*
* STRING bin2hex(number_string, places)
*
******************************************************************************/

PROC_EXEC_PROTO(c_bin2hex)
{
    S32 s32;
    const S32 places = (n_args > 1) ? ss_data_get_integer(args[1]) : DEFAULT_PLACES;

    if(status_fail(from_binary(p_ss_data_res, args[0], &s32)))
        return;

    exec_func_ignore_parms();

    to_hexadecimal(p_ss_data_res, s32, places);
}

/******************************************************************************
*
* STRING bin2oct(number_string, places)
*
******************************************************************************/

PROC_EXEC_PROTO(c_bin2oct)
{
    S32 s32;
    const S32 places = (n_args > 1) ? ss_data_get_integer(args[1]) : DEFAULT_PLACES;

    if(status_fail(from_binary(p_ss_data_res, args[0], &s32)))
        return;

    exec_func_ignore_parms();

    to_octal(p_ss_data_res, s32, places);
}

/******************************************************************************
*
* STRING dec2bin(number {, places})
*
******************************************************************************/

PROC_EXEC_PROTO(c_dec2bin)
{
    F64 f64;
    S32 s32;
    const S32 places = (n_args > 1) ? ss_data_get_integer(args[1]) : DEFAULT_PLACES;

    status_consume(from_decimal(p_ss_data_res, args[0], &f64));

    exec_func_ignore_parms();

    if((f64 > S32_MAX) || (f64 < S32_MIN))
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    s32 = (S32) f64;

    to_binary(p_ss_data_res, s32, places);
}

/******************************************************************************
*
* STRING dec2hex(number {, places})
*
******************************************************************************/

PROC_EXEC_PROTO(c_dec2hex)
{
    F64 f64;
    S64 s64;
    const S32 places = (n_args > 1) ? ss_data_get_integer(args[1]) : DEFAULT_PLACES;

    status_consume(from_decimal(p_ss_data_res, args[0], &f64));

    exec_func_ignore_parms();

    if((f64 > ((1LL << (40-1))-1)) || (f64 < (-(1LL << (40-1)))))
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    s64 = (S64) f64;

    to_hexadecimal(p_ss_data_res, s64, places);
}

/******************************************************************************
*
* STRING dec2oct(number {, places})
*
******************************************************************************/

PROC_EXEC_PROTO(c_dec2oct)
{
    F64 f64;
    S32 s32;
    const S32 places = (n_args > 1) ? ss_data_get_integer(args[1]) : DEFAULT_PLACES;

    status_consume(from_decimal(p_ss_data_res, args[0], &f64));

    exec_func_ignore_parms();

    if((f64 > S32_MAX) || (f64 < S32_MIN))
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    s32 = (S32) f64;

    to_octal(p_ss_data_res, s32, places);
}

/******************************************************************************
*
* STRING hex2bin(number_string, places)
*
******************************************************************************/

PROC_EXEC_PROTO(c_hex2bin)
{
    S64 s64;
    const S32 places = (n_args > 1) ? ss_data_get_integer(args[1]) : DEFAULT_PLACES;

    if(status_fail(from_hexadecimal(p_ss_data_res, args[0], &s64)))
        return;

    exec_func_ignore_parms();

    to_binary(p_ss_data_res, s64, places);
}

/******************************************************************************
*
* NUMBER hex2dec(number_string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_hex2dec)
{
    S64 s64;

    if(status_fail(from_hexadecimal(p_ss_data_res, args[0], &s64)))
        return;

    exec_func_ignore_parms();

    to_decimal(p_ss_data_res, s64);
}

/******************************************************************************
*
* STRING hex2oct(number_string, places)
*
******************************************************************************/

PROC_EXEC_PROTO(c_hex2oct)
{
    S64 s64;
    const S32 places = (n_args > 1) ? ss_data_get_integer(args[1]) : DEFAULT_PLACES;

    if(status_fail(from_hexadecimal(p_ss_data_res, args[0], &s64)))
        return;

    exec_func_ignore_parms();

    to_octal(p_ss_data_res, s64, places);
}

/******************************************************************************
*
* STRING oct2bin(number_string, places)
*
******************************************************************************/

PROC_EXEC_PROTO(c_oct2bin)
{
    S32 s32;
    const S32 places = (n_args > 1) ? ss_data_get_integer(args[1]) : DEFAULT_PLACES;

    if(status_fail(from_octal(p_ss_data_res, args[0], &s32)))
        return;

    exec_func_ignore_parms();

    to_binary(p_ss_data_res, s32, places);
}

/******************************************************************************
*
* NUMBER oct2dec(number_string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_oct2dec)
{
    S32 s32;

    if(status_fail(from_octal(p_ss_data_res, args[0], &s32)))
        return;

    exec_func_ignore_parms();

    to_decimal(p_ss_data_res, s32);
}

/******************************************************************************
*
* STRING oct2hex(number_string, places)
*
******************************************************************************/

PROC_EXEC_PROTO(c_oct2hex)
{
    S32 s32;
    const S32 places = (n_args > 1) ? ss_data_get_integer(args[1]) : DEFAULT_PLACES;

    if(status_fail(from_octal(p_ss_data_res, args[0], &s32)))
        return;

    exec_func_ignore_parms();

    to_hexadecimal(p_ss_data_res, s32, places);
}

/* end of ev_fnenc.c */
