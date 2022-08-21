/* ev_mcpx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Complex number functions for evaluator */

/* MRJC May 1991; SKS May 2014 split off from ev_math; SKS December 2019 split off complex core functions */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "cmodules/mathxcpx.h"

#include "cmodules/mathxtra.h"

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h" /* for _log10_e */
#endif

/*
internal functions
*/

static void
complex_result_reals(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part);

#if defined(COMPLEX_STRING)

static void
complex_result_reals_string(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part,
    _InVal_     U8 imag_suffix_char);

#endif

/******************************************************************************
*
* check that arg is a 2 by 1 array of two real numbers (or one real)
*
* --out--
* <0  arg data was unsuitable
* >=0 COMPLEX z was set up with numbers
*
******************************************************************************/

_Check_return_
static STATUS
complex_check_arg_array(
    /**/        P_SS_DATA p_ss_data_res,
    _OutRef_    P_COMPLEX z,
    _InRef_     P_SS_DATA p_ss_data_in)
{
    if(ss_data_is_real(p_ss_data_in))
    {
        complex_set_ri(z, ss_data_get_real(p_ss_data_in), 0.0);
        return(STATUS_OK);
    }

    if(ss_data_is_array(p_ss_data_in))
    {   /* extract elements from the array */
        SS_DATA ss_data_1, ss_data_2;
        const EV_IDNO t1 = array_range_index(&ss_data_1, p_ss_data_in, 0, 0, EM_REA);
        const EV_IDNO t2 = array_range_index(&ss_data_2, p_ss_data_in, 1, 0, EM_REA);

        if( (DATA_ID_REAL == t1) && (DATA_ID_REAL == t2) )
        {
            complex_set_ri(z, ss_data_get_real(&ss_data_1), ss_data_get_real(&ss_data_2));
            return(STATUS_OK);
        }
    }

    *z = complex_zero;
    return(ss_data_set_error(p_ss_data_res, EVAL_ERR_BADCOMPLEX));
}

#if defined(COMPLEX_STRING)

/******************************************************************************
*
* check that a string is suitable for use as Excel-style complex number
*
* --out--
* <0  string was unsuitable
* >=0 COMPLEX z was set up with numbers
*
* parse <real>[+|-]<imag>[i|j], which may be just
* <real> or
* <imag>[i|j] or even
* {[+|-]}[i|j]
*
******************************************************************************/

_Check_return_
static STATUS
complex_check_arg_ustr(
    _InoutRef_  P_COMPLEX z,
    _InoutRef_  P_U8 p_imag_suffix_char,
    _In_z_      PC_USTR ustr)
{
    F64 real_part = 0.0;
    F64 imag_part = 0.0;
    BOOL negate = FALSE;
    F64 f64;
    U32 uchars_read;
    PC_USTR new_ustr;

    /* read a number */
    f64 = ustrtod(ustr, &new_ustr);
    uchars_read = PtrDiffBytesU32(new_ustr, ustr);

    if(0 == uchars_read)
    {   /* no number read - but we don't need a number when fed just "i" or "-i" */
        if(CH_MINUS_SIGN__BASIC == PtrGetByte(ustr))
        {
            negate = TRUE;
            ustr_IncByte(ustr);
        }
        else if(CH_PLUS_SIGN == PtrGetByte(ustr))
        {
            negate = FALSE;
            ustr_IncByte(ustr);
        }
        else
            negate = FALSE;
    }
    else
    {   /* read a number OK */

        if(CH_NULL == PtrGetByteOff(ustr, uchars_read))
        {   /* early exit, got just a real part */
            real_part = f64;
            complex_set_ri(z, real_part, 0.0);
            return(STATUS_OK);
        }

        ustr = new_ustr;
    }

    if((PtrGetByte(ustr) == 'i') || (PtrGetByte(ustr) == 'j'))
    {
        if(CH_NULL != PtrGetByteOff(ustr, 1))
            return(EVAL_ERR_BADCOMPLEX);
        *p_imag_suffix_char = PtrGetByte(ustr);
        if(0 != uchars_read)
            imag_part = f64; /* <imag>[i|j] */
        else
            imag_part = negate ? -1.0 : 1.0; /* {[+|-]}[i|j] */
        complex_set_ri(z, 0.0, imag_part);
        return(STATUS_OK);
    }

    if(0 == uchars_read)
        return(EVAL_ERR_BADCOMPLEX);

    /* read a number OK - that number must be the real part then */
    real_part = f64;

    /* and must be followed by [+|-]{number}i */
    if(CH_MINUS_SIGN__BASIC == PtrGetByte(ustr))
        negate = TRUE;
    else if(CH_PLUS_SIGN == PtrGetByte(ustr))
        negate = FALSE;
    else
        return(EVAL_ERR_BADCOMPLEX);

    /* read another number */
    f64 = ustrtod(ustr, &new_ustr);
    uchars_read = PtrDiffBytesU32(new_ustr, ustr);

    if(0 == uchars_read)
    {   /* no number read - but we don't need another number when fed just "<real>[+|]i" */
        if(CH_MINUS_SIGN__BASIC == PtrGetByte(ustr))
        {
            negate = TRUE;
            ustr_IncByte(ustr);
        }
        else if(CH_PLUS_SIGN == PtrGetByte(ustr))
        {
            negate = FALSE;
            ustr_IncByte(ustr);
        }
        else
            return(EVAL_ERR_BADCOMPLEX);
    }
    else
    {   /* read another number OK - that number must be the imag part then */
        ustr = new_ustr;
    }

    if((PtrGetByte(ustr) == 'i') || (PtrGetByte(ustr) == 'j'))
    {
        if(CH_NULL != PtrGetByteOff(ustr, 1))
            return(EVAL_ERR_BADCOMPLEX);
        *p_imag_suffix_char = PtrGetByte(ustr);
        if(0 != uchars_read)
            imag_part = f64; /* <real>[+|-]<imag>[i|j] */
        else
            imag_part = negate ? -1.0 : 1.0; /* <real>[+|-][i|j] */
        complex_set_ri(z, real_part, imag_part);
        return(STATUS_OK);
    }

    return(EVAL_ERR_BADCOMPLEX);
}

_Check_return_
static STATUS
complex_check_arg_string(
    /**/        P_SS_DATA p_ss_data_res,
    _OutRef_    P_COMPLEX z,
    _OutRef_    P_U8 p_imag_suffix_char,
    _InRef_     P_SS_DATA p_ss_data_in)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
    quick_ublock_with_buffer_setup(quick_ublock);

    *z = complex_zero;
    *p_imag_suffix_char = CH_NULL;

    if(!ss_data_is_string(p_ss_data_in))
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_BADCOMPLEX));

    /* need a CH_NULL-terminated string to parse */
    if(status_ok(status = quick_ublock_uchars_add(&quick_ublock, ss_data_get_string(p_ss_data_in), ss_data_get_string_size(p_ss_data_in))))
        status = quick_ublock_nullch_add(&quick_ublock);

    if(status_ok(status))
        status = complex_check_arg_ustr(z, p_imag_suffix_char, quick_ublock_ustr(&quick_ublock));

    quick_ublock_dispose(&quick_ublock);

    if(status_fail(status))
        return(ss_data_set_error(p_ss_data_res, status));

    return(STATUS_OK);
}

_Check_return_
static STATUS
complex_check_arg_string_pair(
    /**/        P_SS_DATA p_ss_data_res,
    _OutRef_    P_COMPLEX z1,
    _OutRef_    P_COMPLEX z2,
    _OutRef_    P_U8 p_imag_suffix_char,
    _InRef_     P_SS_DATA p_ss_data_1,
    _InRef_     P_SS_DATA p_ss_data_2)
{
    U8 imag_suffix_char_1, imag_suffix_char_2;

    CODE_ANALYSIS_ONLY(*z2 = complex_zero);

    *p_imag_suffix_char = CH_NULL;

    status_return(complex_check_arg_string(p_ss_data_res, z1, &imag_suffix_char_1, p_ss_data_1));
    status_return(complex_check_arg_string(p_ss_data_res, z2, &imag_suffix_char_2, p_ss_data_2));

    if((CH_NULL == imag_suffix_char_1) && (CH_NULL == imag_suffix_char_2))
         *p_imag_suffix_char = 'i';
    else if(imag_suffix_char_1 == imag_suffix_char_2)
        *p_imag_suffix_char = imag_suffix_char_1;
    else if(CH_NULL == imag_suffix_char_1)
        *p_imag_suffix_char = imag_suffix_char_2;
    else if(CH_NULL == imag_suffix_char_2)
        *p_imag_suffix_char = imag_suffix_char_1;
    else /* (imag_suffix_char_1 != imag_suffix_char_2) */
#if 1
        *p_imag_suffix_char = 'i';
#else
        return(ss_data_set_error(p_ss_data_res, EVAL_ERR_MIXED_SUFFIXES));
#endif

    return(STATUS_OK);
}

#endif /* COMPLEX_STRING */

_Check_return_
static STATUS
complex_check_arg(
    /**/        P_SS_DATA p_ss_data_res,
    _OutRef_    P_COMPLEX z,
    _OutRef_    P_U8 p_imag_suffix_char,
    _InRef_     P_SS_DATA p_ss_data)
{
#if defined(COMPLEX_STRING)
    if(ss_data_is_string(p_ss_data))
        return(complex_check_arg_string(p_ss_data_res, z, p_imag_suffix_char, p_ss_data));
#endif /* COMPLEX_STRING */

    *p_imag_suffix_char = CH_NULL;

    return(complex_check_arg_array(p_ss_data_res, z, p_ss_data));
}

_Check_return_
static STATUS
complex_check_arg_pair(
    /**/        P_SS_DATA p_ss_data_res,
    _OutRef_    P_COMPLEX z1,
    _OutRef_    P_COMPLEX z2,
    _OutRef_    P_U8 p_imag_suffix_char,
    _InRef_     P_SS_DATA p_ss_data_1,
    _InRef_     P_SS_DATA p_ss_data_2)
{
#if defined(COMPLEX_STRING)
    /* easy if both are strings */
    if( ss_data_is_string(p_ss_data_1) && ss_data_is_string(p_ss_data_2) )
        return(complex_check_arg_string_pair(p_ss_data_res, z1, z2, p_imag_suffix_char, p_ss_data_1, p_ss_data_2));

    *p_imag_suffix_char = CH_NULL;

    CODE_ANALYSIS_ONLY(*z2 = complex_zero);

    if(ss_data_is_string(p_ss_data_1))
        status_return(complex_check_arg_string(p_ss_data_res, z1, p_imag_suffix_char, p_ss_data_1));
    else
        status_return(complex_check_arg_array(p_ss_data_res, z1, p_ss_data_1));

    if(ss_data_is_string(p_ss_data_2))
        return(complex_check_arg_string(p_ss_data_res, z2, p_imag_suffix_char, p_ss_data_2));
    else
        return(complex_check_arg_array(p_ss_data_res, z2, p_ss_data_2));
#else
    *p_imag_suffix_char = CH_NULL;

    CODE_ANALYSIS_ONLY(*z2 = complex_zero);

    status_return(complex_check_arg_array(p_ss_data_res, z1, p_ss_data_1));
           return(complex_check_arg_array(p_ss_data_res, z2, p_ss_data_2));
#endif /* COMPLEX_STRING */
}

/******************************************************************************
*
* make a complex number result from an internal complex number type
*
******************************************************************************/

static void
complex_result_complex(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_COMPLEX z,
    _InVal_     U8 imag_suffix_char)
{
    if(CH_NULL != imag_suffix_char)
    {
#if defined(COMPLEX_STRING)
        complex_result_reals_string(p_ss_data, complex_real(z), complex_imag(z), imag_suffix_char);
#endif
        return;
    }

    complex_result_reals(p_ss_data, complex_real(z), complex_imag(z));
}

/******************************************************************************
*
* make a complex number result from the given complex number
*
******************************************************************************/

static void
complex_result_reals(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part)
{
    P_SS_DATA elep;

    if(status_fail(ss_array_make(p_ss_data, 2, 1)))
        return;

    elep = ss_array_element_index_wr(p_ss_data, 0, 0);
    ss_data_set_real(elep, real_part);

    elep = ss_array_element_index_wr(p_ss_data, 1, 0);
    ss_data_set_real(elep, imag_part);
}

#if defined(COMPLEX_STRING)

#if defined(UNUSED_KEEP_ALIVE)

static inline void
complex_result_complex_string(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_COMPLEX z,
    _InVal_     U8 imag_suffix_char)
{
    complex_result_reals_string(p_ss_data, complex_real(z), complex_imag(z), imag_suffix_char);
}

#endif /* UNUSED */

static inline U32 /* bytes currently in output */
decode_fp(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     F64 f64)
{
    return(/*len*/ ustr_xsnprintf(ustr_buf, elemof_buffer, USTR_TEXT("%.16G"), f64));
}

static void
complex_result_reals_string(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part,
    _InVal_     U8 imag_suffix_char)
{
    STATUS status = STATUS_OK;
    BOOL real_zero = real_part == 0.0;
  /*BOOL real_negative = real_part < 0.0;*/
  /*F64 real_abs = fabs(real_part);*/
  /*U8 real_sign_char = real_negative ? CH_MINUS_SIGN__BASIC : CH_PLUS_SIGN;*/
    BOOL imag_zero = imag_part == 0.0;
    BOOL imag_negative = imag_part < 0.0;
    F64 imag_abs = fabs(imag_part);
    U8 imag_sign_char = imag_negative ? CH_MINUS_SIGN__BASIC : CH_PLUS_SIGN;
    UCHARZ fp_buffer[64];
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
    quick_ublock_with_buffer_setup(quick_ublock);

    /* always output real part first if it is non-zero */
    if(!real_zero)
    {
        const U32 len = decode_fp(ustr_bptr(fp_buffer), elemof32(fp_buffer), real_part); /* including sign of real part */
        status = quick_ublock_uchars_add(&quick_ublock, ustr_bptr(fp_buffer), len);
    }

    if(imag_zero)
    {
        if(real_zero)
            status = quick_ublock_a7char_add(&quick_ublock, CH_DIGIT_ZERO);
    }
    else if(1.0 == imag_abs)
    {
        /* append [+|-][i|j] */
        if(status_ok(status))
            status = quick_ublock_a7char_add(&quick_ublock, imag_sign_char);
        if(status_ok(status))
            status = quick_ublock_a7char_add(&quick_ublock, imag_suffix_char);
    }
    else
    {
        /* append [+|-]<imag_abs>[i|j] */
        if(status_ok(status))
            status = quick_ublock_a7char_add(&quick_ublock, imag_sign_char);
        if(status_ok(status))
        {
            const U32 len = decode_fp(ustr_bptr(fp_buffer), elemof32(fp_buffer), imag_abs);
            status = quick_ublock_uchars_add(&quick_ublock, ustr_bptr(fp_buffer), len);
        }
        if(status_ok(status))
            status = quick_ublock_a7char_add(&quick_ublock, imag_suffix_char);
    }

    if(status_fail(status))
        ss_data_set_error(p_ss_data, status);
    else
        status_assert(ss_string_make_uchars(p_ss_data, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock)));

    quick_ublock_dispose(&quick_ublock);
}

#endif /* COMPLEX_STRING */

/******************************************************************************
*
* add two complex numbers
*
* (a+ib) + (c+id) = (a+c) + i(b+d)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_add)
{
    U8 imag_suffix_char;
    COMPLEX z1, z2, add_result;

    exec_func_ignore_parms();

    /* check the input is a pair of suitable complex numbers */
    if(status_fail(complex_check_arg_pair(p_ss_data_res, &z1, &z2, &imag_suffix_char, args[0], args[1])))
        return;

    complex_add(&z1, &z2, &add_result);

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &add_result, imag_suffix_char);
}

/******************************************************************************
*
* subtract second complex number from first
*
* (a+ib) - (c+id) = (a-c) + i(b-d)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sub)
{
    U8 imag_suffix_char;
    COMPLEX z1, z2, sub_result;

    exec_func_ignore_parms();

    /* check the input is a pair of suitable complex numbers */
    if(status_fail(complex_check_arg_pair(p_ss_data_res, &z1, &z2, &imag_suffix_char, args[0], args[1])))
        return;

    complex_subtract(&z1, &z2, &sub_result);

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &sub_result, imag_suffix_char);
}

/******************************************************************************
*
* multiply two complex numbers
*
* (a+ib) * (c+id) = (ac-bd) + i(bc+ad)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_mul)
{
    U8 imag_suffix_char;
    COMPLEX z1, z2, mul_result;

    exec_func_ignore_parms();

    /* check the input is a pair of suitable complex numbers */
    if(status_fail(complex_check_arg_pair(p_ss_data_res, &z1, &z2, &imag_suffix_char, args[0], args[1])))
        return;

    complex_multiply(&z1, &z2, &mul_result);

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &mul_result, imag_suffix_char);
}

/******************************************************************************
*
* divide two complex numbers
*
* (a+ib) / (c+id) = ((ac+bd) + i(bc-ad)) / (c*c + d*d)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_div)
{
    U8 imag_suffix_char;
    COMPLEX z1, z2, div_result;

    exec_func_ignore_parms();

    /* check the input is a pair of suitable complex numbers */
    if(status_fail(complex_check_arg_pair(p_ss_data_res, &z1, &z2, &imag_suffix_char, args[0], args[1])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_divide(&z1, &z2, &div_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &div_result, imag_suffix_char);
}

/******************************************************************************
*
* COMPLEX c_complex(real_part:number {, imag_part:number {, suffix:string}})
*
* STRING odf.complex(real_part:number, imag_part:number {, suffix:string})
*
******************************************************************************/

static void
c_c_complex_common(
    _In_reads_(n_args) P_SS_DATA args[],
    _InVal_     S32 n_args,
    _OutRef_    P_SS_DATA p_ss_data_res,
    _InVal_     U8 default_imag_suffix_char)
{
    COMPLEX complex_result;
    U8 imag_suffix_char = default_imag_suffix_char;

    assert(n_args >= 1);
    complex_set_ri(&complex_result, ss_data_get_real(args[0]), (n_args > 1) ? ss_data_get_real(args[1]) : 0.0);

    if(n_args > 2)
    {
        const PC_UCHARS uchars = ss_data_get_string(args[2]);

        if( (1 != ss_data_get_string_size(args[2])) || ((PtrGetByte(uchars) != 'i') && (PtrGetByte(uchars) != 'j')) )
        {
            ss_data_set_error(p_ss_data_res, EVAL_ERR_BADCOMPLEX);
            return;
        }

        imag_suffix_char = PtrGetByte(uchars);
    }

    complex_result_complex(p_ss_data_res, &complex_result, imag_suffix_char);
}

PROC_EXEC_PROTO(c_c_complex)
{
    exec_func_ignore_parms();

    c_c_complex_common(args, n_args, p_ss_data_res, CH_NULL /* only a string result if suffix supplied */);
}

PROC_EXEC_PROTO(c_odf_complex)
{
    exec_func_ignore_parms();

    c_c_complex_common(args, n_args, p_ss_data_res, 'i' /* always a string result, may be overridden */);
}

/******************************************************************************
*
* COMPLEX find complex conjugate of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_conjugate)
{
    U8 imag_suffix_char;
    COMPLEX z, conjugate_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    complex_conjugate(&z, &conjugate_result);

    complex_result_complex(p_ss_data_res, &conjugate_result, imag_suffix_char);
}

/******************************************************************************
*
* REAL find real part of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_real)
{
    U8 imag_suffix_char;
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    ss_data_set_real(p_ss_data_res, complex_real(&z));
}

/******************************************************************************
*
* REAL find imaginary part of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_imaginary)
{
    U8 imag_suffix_char;
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    ss_data_set_real(p_ss_data_res, complex_imag(&z));
}

/******************************************************************************
*
* REAL find radius of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_radius)
{
    U8 imag_suffix_char;
    COMPLEX z;
    F64 radius_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    radius_result = complex_modulus(&z);

    ss_data_set_real(p_ss_data_res, radius_result);
}

/******************************************************************************
*
* REAL find theta of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_theta)
{
    U8 imag_suffix_char;
    COMPLEX z;
    F64 theta_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    theta_result = complex_argument(&z);

    ss_data_set_real(p_ss_data_res, theta_result);
}

/******************************************************************************
*
* c_round(complex number {, decimal_places:number=2})
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_round)
{
    U8 imag_suffix_char;
    COMPLEX z, round_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    { /* round components separately to the desired number of decimal places */
    F64 round_result_real, round_result_imag;
    SS_DATA number, decimal_places;
    P_SS_DATA local_args[2];

    local_args[0] = &number;
    local_args[1] = &decimal_places;

    if(n_args > 1)
        ss_data_set_integer(&decimal_places, MIN(15, ss_data_get_integer(args[1])));
    else
        ss_data_set_integer(&decimal_places, 2);

    ss_data_set_real(&number, complex_real(&z));
    round_common(local_args, 2, p_ss_data_res, RPN_FNV_ROUND);
    assert(ss_data_is_real(p_ss_data_res));
    round_result_real = ss_data_get_real(p_ss_data_res);

    ss_data_set_real(&number, complex_imag(&z));
    round_common(local_args, 2, p_ss_data_res, RPN_FNV_ROUND);
    assert(ss_data_is_real(p_ss_data_res));
    round_result_imag = ss_data_get_real(p_ss_data_res);

    complex_set_ri(&round_result, round_result_real, round_result_imag);
    } /*block*/

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &round_result, imag_suffix_char);
}

/******************************************************************************
*
* c_exp(complex number) - complex exponent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_exp)
{
    U8 imag_suffix_char;
    COMPLEX z, exp_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    complex_exp(&z, &exp_result);

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &exp_result, imag_suffix_char);
}

/******************************************************************************
*
* c_ln(complex number) - complex natural logarithm
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_ln)
{
    U8 imag_suffix_char;
    COMPLEX z, ln_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_log_e(&z, &ln_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &ln_result, imag_suffix_char);
}

/******************************************************************************
*
* c_log_10(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_log_10)
{
    U8 imag_suffix_char;
    COMPLEX z, log10_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_log_10(&z, &log10_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &log10_result, imag_suffix_char);
}

/******************************************************************************
*
* c_log_2(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_log_2)
{
    U8 imag_suffix_char;
    COMPLEX z, log2_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_log_2(&z, &log2_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &log2_result, imag_suffix_char);
}

/******************************************************************************
*
* c_power(complex number) - complex z^w
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_power)
{
    U8 imag_suffix_char;
    COMPLEX z1, z2, power_result;

    exec_func_ignore_parms();

    /* check the input is a pair of suitable complex numbers */
    if(status_fail(complex_check_arg_pair(p_ss_data_res, &z1, &z2, &imag_suffix_char, args[0], args[1])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_power(&z1, &z2, &power_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &power_result, imag_suffix_char);
}

/******************************************************************************
*
* c_sqrt(complex number) - complex square root
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sqrt)
{
    U8 imag_suffix_char;
    COMPLEX z, sqrt_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    complex_square_root(&z, &sqrt_result);

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &sqrt_result, imag_suffix_char);
}

/******************************************************************************
*
* c_cos(complex number) - complex cosine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cos)
{
    U8 imag_suffix_char;
    COMPLEX z, cos_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    complex_cosine(&z, &cos_result);

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &cos_result, imag_suffix_char);
}

/******************************************************************************
*
* c_acos(complex number) - complex arc cosine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_acos)
{
    U8 imag_suffix_char;
    COMPLEX z, acos_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_arc_cosine(&z, &acos_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &acos_result, imag_suffix_char);
}

/******************************************************************************
*
* c_sin(complex number) - complex sine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sin)
{
    U8 imag_suffix_char;
    COMPLEX z, sin_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    complex_sine(&z, &sin_result);

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &sin_result, imag_suffix_char);
}

/******************************************************************************
*
* c_asin(complex number) - complex arc sine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_asin)
{
    U8 imag_suffix_char;
    COMPLEX z, asin_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_arc_sine(&z, &asin_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &asin_result, imag_suffix_char);
}

/******************************************************************************
*
* c_tan(complex number) - complex tangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_tan)
{
    U8 imag_suffix_char;
    COMPLEX z, tan_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_tangent(&z, &tan_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &tan_result, imag_suffix_char);
}

/******************************************************************************
*
* c_atan(complex number) - complex arc tangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_atan)
{
    U8 imag_suffix_char;
    COMPLEX z, atan_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_arc_tangent(&z, &atan_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &atan_result, imag_suffix_char);
}

/******************************************************************************
*
* c_cosec(complex number) - complex cosecant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cosec)
{
    U8 imag_suffix_char;
    COMPLEX z, cosec_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_cosecant(&z, &cosec_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &cosec_result, imag_suffix_char);
}

/******************************************************************************
*
* c_acosec(complex number) - complex arc cosecant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_acosec)
{
    U8 imag_suffix_char;
    COMPLEX z, acosec_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_arc_cosecant(&z, &acosec_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &acosec_result, imag_suffix_char);
}

/******************************************************************************
*
* c_sec(complex number) - complex secant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sec)
{
    U8 imag_suffix_char;
    COMPLEX z, sec_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_secant(&z, &sec_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &sec_result, imag_suffix_char);
}

/******************************************************************************
*
* c_asec(complex number) - complex arc secant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_asec)
{
    U8 imag_suffix_char;
    COMPLEX z, asec_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_arc_secant(&z, &asec_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &asec_result, imag_suffix_char);
}

/******************************************************************************
*
* c_cot(complex number) - complex cotangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cot)
{
    U8 imag_suffix_char;
    COMPLEX z, cot_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_cotangent(&z, &cot_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &cot_result, imag_suffix_char);
}

/******************************************************************************
*
* c_acot(complex number) - complex arc cotangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_acot)
{
    U8 imag_suffix_char;
    COMPLEX z, acot_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_arc_cotangent(&z, &acot_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &acot_result, imag_suffix_char);
}

/******************************************************************************
*
* c_cosh(complex number) - complex hyperbolic cosine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cosh)
{
    U8 imag_suffix_char;
    COMPLEX z, cosh_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    complex_hyperbolic_cosine(&z, &cosh_result);

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &cosh_result, imag_suffix_char);
}

/******************************************************************************
*
* c_acosh(complex number) - complex inverse hyperbolic cosine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_acosh)
{
    U8 imag_suffix_char;
    COMPLEX z, acosh_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_inverse_hyperbolic_cosine(&z, &acosh_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &acosh_result, imag_suffix_char);
}

/******************************************************************************
*
* c_sinh(complex number) - complex hyperbolic sine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sinh)
{
    U8 imag_suffix_char;
    COMPLEX z, sinh_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    complex_hyperbolic_sine(&z, &sinh_result);

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &sinh_result, imag_suffix_char);
}

/******************************************************************************
*
* c_asinh(complex number) - complex inverse hyperbolic sine
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_asinh)
{
    U8 imag_suffix_char;
    COMPLEX z, asinh_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_inverse_hyperbolic_sine(&z, &asinh_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &asinh_result, imag_suffix_char);
}

/******************************************************************************
*
* c_tanh(complex number) - complex hyperbolic tangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_tanh)
{
    U8 imag_suffix_char;
    COMPLEX z, tanh_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_hyperbolic_tangent(&z, &tanh_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &tanh_result, imag_suffix_char);
}

/******************************************************************************
*
* c_atanh(complex number) - complex inverse hyperbolic tangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_atanh)
{
    U8 imag_suffix_char;
    COMPLEX z, atanh_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_inverse_hyperbolic_tangent(&z, &atanh_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &atanh_result, imag_suffix_char);
}

/******************************************************************************
*
* c_cosech(complex number) - complex hyperbolic cosecant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cosech)
{
    U8 imag_suffix_char;
    COMPLEX z, cosech_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_hyperbolic_cosecant(&z, &cosech_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &cosech_result, imag_suffix_char);
}

/******************************************************************************
*
* c_acosech(complex number) - complex inverse hyperbolic cosecant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_acosech)
{
    U8 imag_suffix_char;
    COMPLEX z, acosech_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_inverse_hyperbolic_cosecant(&z, &acosech_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &acosech_result, imag_suffix_char);
}

/******************************************************************************
*
* c_sech(complex number) - complex hyperbolic secant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sech)
{
    U8 imag_suffix_char;
    COMPLEX z, sech_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_hyperbolic_secant(&z, &sech_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &sech_result, imag_suffix_char);
}

/******************************************************************************
*
* c_asech(complex number) - complex inverse hyperbolic secant
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_asech)
{
    U8 imag_suffix_char;
    COMPLEX z, asech_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_inverse_hyperbolic_secant(&z, &asech_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &asech_result, imag_suffix_char);
}

/******************************************************************************
*
* c_coth(complex number) - complex hyperbolic cotangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_coth)
{
    U8 imag_suffix_char;
    COMPLEX z, coth_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_hyperbolic_cotangent(&z, &coth_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &coth_result, imag_suffix_char);
}

/******************************************************************************
*
* c_acoth(complex number) - complex inverse hyperbolic cotangent
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_acoth)
{
    U8 imag_suffix_char;
    COMPLEX z, acoth_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ss_data_res, &z, &imag_suffix_char, args[0])))
        return;

    exec_func_status_return(p_ss_data_res,
        complex_inverse_hyperbolic_cotangent(&z, &acoth_result));

    /* output a complex number */
    complex_result_complex(p_ss_data_res, &acoth_result, imag_suffix_char);
}

/* end of ev_mcpx.c */
