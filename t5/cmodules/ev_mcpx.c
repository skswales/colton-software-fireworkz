/* ev_mcpx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Complex number functions for evaluator */

/* MRJC May 1991; SKS May 2014 split off from ev_math */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "cmodules/mathxtra.h"

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h" /* for _log10_e */
#endif

#if __STDC_VERSION__ < 199901L

#if WINDOWS

_Check_return_
static double
log2(_InVal_ double d)
{
    return(log(d) * _log2_e);
}

#endif /* OS */

#endif /* __STDC_VERSION__ */

/*
declare complex number type for internal usage
*/

typedef struct COMPLEX
{
    F64 r, i;
}
COMPLEX, * P_COMPLEX; typedef const COMPLEX * PC_COMPLEX;

/*
internal functions
*/

static void
complex_result_reals(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part);

#if defined(COMPLEX_STRING)

static void
complex_result_reals_string(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part,
    _InVal_     U8 imag_suffix_char);

#endif

/******************************************************************************
*
* the constant one as a complex number
*
******************************************************************************/

static const COMPLEX
complex_unity = { 1.0, 0.0 };

/******************************************************************************
*
* the constant one half as a complex number
*
******************************************************************************/

static const COMPLEX
complex_one_half = { 0.5, 0.0 };

/******************************************************************************
*
* check that arg is a 2 by 1 array of two real numbers (or one real)
*
* --out--
* <0  arg data was unsuitable
* >=0 n->r and n->i set up with numbers
*
******************************************************************************/

_Check_return_
static STATUS
complex_check_arg_array(
    /**/        P_EV_DATA p_ev_data_res,
    _OutRef_    P_COMPLEX n,
    _InRef_     P_EV_DATA p_ev_data_in)
{
    if(RPN_DAT_REAL == p_ev_data_in->did_num)
    {
        n->r = p_ev_data_in->arg.fp;
        n->i = 0.0;
        return(STATUS_OK);
    }

    if(RPN_DAT_ARRAY == p_ev_data_in->did_num)
    {
        EV_DATA ev_data1, ev_data2;

        /* extract elements from the array */
        const EV_IDNO t1 = array_range_index(&ev_data1, p_ev_data_in, 0, 0, EM_REA);
        const EV_IDNO t2 = array_range_index(&ev_data2, p_ev_data_in, 1, 0, EM_REA);

        if((RPN_DAT_REAL == t1) && (RPN_DAT_REAL == t2))
        {
            n->r = ev_data1.arg.fp;
            n->i = ev_data2.arg.fp;
            return(STATUS_OK);
        }
    }

    n->r = 0.0;
    n->i = 0.0;
    return(ev_data_set_error(p_ev_data_res, EVAL_ERR_BADCOMPLEX));
}

#if defined(COMPLEX_STRING)

/******************************************************************************
*
* check that a string is suitable for use as Excel-style complex number
*
* --out--
* <0  string was unsuitable
* >=0 n->r and n->i set up with numbers
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
    _InoutRef_  P_COMPLEX n,
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
            n->r = real_part;
            n->i = 0.0;
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
        n->r = 0.0;
        n->i = imag_part;
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
        n->r = real_part;
        n->i = imag_part;
        return(STATUS_OK);
    }

    return(EVAL_ERR_BADCOMPLEX);
}

_Check_return_
static STATUS
complex_check_arg_string(
    /**/        P_EV_DATA p_ev_data_res,
    _OutRef_    P_COMPLEX n,
    _OutRef_    P_U8 p_imag_suffix_char,
    _InRef_     P_EV_DATA p_ev_data_in)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 50);
    quick_ublock_with_buffer_setup(quick_ublock);

    n->r = 0.0;
    n->i = 0.0;
    *p_imag_suffix_char = CH_NULL;

    if(RPN_DAT_STRING != p_ev_data_in->did_num)
        return(ev_data_set_error(p_ev_data_res, EVAL_ERR_BADCOMPLEX));

    /* need a CH_NULL-terminated string to parse */
    if(status_ok(status = quick_ublock_uchars_add(&quick_ublock, p_ev_data_in->arg.string.uchars, p_ev_data_in->arg.string.size)))
        status = quick_ublock_nullch_add(&quick_ublock);

    if(status_ok(status))
        status = complex_check_arg_ustr(n, p_imag_suffix_char, quick_ublock_ustr(&quick_ublock));

    quick_ublock_dispose(&quick_ublock);

    if(status_fail(status))
        return(ev_data_set_error(p_ev_data_res, status));

    return(STATUS_OK);
}

_Check_return_
static STATUS
complex_check_arg_string_pair(
    /**/        P_EV_DATA p_ev_data_res,
    _OutRef_    P_COMPLEX n1,
    _OutRef_    P_COMPLEX n2,
    _OutRef_    P_U8 p_imag_suffix_char,
    _InRef_     P_EV_DATA p_ev_data_1,
    _InRef_     P_EV_DATA p_ev_data_2)
{
    U8 imag_suffix_char_1, imag_suffix_char_2;

    CODE_ANALYSIS_ONLY((n2->r = 0.0, n2->i = 0.0));

    *p_imag_suffix_char = CH_NULL;

    status_return(complex_check_arg_string(p_ev_data_res, n1, &imag_suffix_char_1, p_ev_data_1));
    status_return(complex_check_arg_string(p_ev_data_res, n2, &imag_suffix_char_2, p_ev_data_2));

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
        return(ev_data_set_error(p_ev_data_res, EVAL_ERR_MIXED_SUFFIXES));
#endif

    return(STATUS_OK);
}

#endif /* COMPLEX_STRING */

_Check_return_
static STATUS
complex_check_arg(
    /**/        P_EV_DATA p_ev_data_res,
    _OutRef_    P_COMPLEX n,
    _OutRef_    P_U8 p_imag_suffix_char,
    _InRef_     P_EV_DATA p_ev_data)
{
#if defined(COMPLEX_STRING)
    if(RPN_DAT_STRING == p_ev_data->did_num)
        return(complex_check_arg_string(p_ev_data_res, n, p_imag_suffix_char, p_ev_data));
#endif /* COMPLEX_STRING */

    *p_imag_suffix_char = CH_NULL;

    return(complex_check_arg_array(p_ev_data_res, n, p_ev_data));
}

_Check_return_
static STATUS
complex_check_arg_pair(
    /**/        P_EV_DATA p_ev_data_res,
    _OutRef_    P_COMPLEX n1,
    _OutRef_    P_COMPLEX n2,
    _OutRef_    P_U8 p_imag_suffix_char,
    _InRef_     P_EV_DATA p_ev_data_1,
    _InRef_     P_EV_DATA p_ev_data_2)
{
#if defined(COMPLEX_STRING)
    /* easy if both are strings */
    if((RPN_DAT_STRING == p_ev_data_1->did_num) && (RPN_DAT_STRING == p_ev_data_2->did_num))
        return(complex_check_arg_string_pair(p_ev_data_res, n1, n2, p_imag_suffix_char, p_ev_data_1, p_ev_data_2));

    *p_imag_suffix_char = CH_NULL;

    CODE_ANALYSIS_ONLY((n2->r = 0.0, n2->i = 0.0));

    if(RPN_DAT_STRING == p_ev_data_1->did_num)
        status_return(complex_check_arg_string(p_ev_data_res, n1, p_imag_suffix_char, p_ev_data_1));
    else
        status_return(complex_check_arg_array(p_ev_data_res, n1, p_ev_data_1));

    if(RPN_DAT_STRING == p_ev_data_2->did_num)
        return(complex_check_arg_string(p_ev_data_res, n2, p_imag_suffix_char, p_ev_data_2));
    else
        return(complex_check_arg_array(p_ev_data_res, n2, p_ev_data_2));
#else
    *p_imag_suffix_char = CH_NULL;

    CODE_ANALYSIS_ONLY((n2->r = 0.0, n2->i = 0.0));

    status_return(complex_check_arg_array(p_ev_data_res, n1, p_ev_data_1));
           return(complex_check_arg_array(p_ev_data_res, n2, p_ev_data_2));
#endif /* COMPLEX_STRING */
}

/******************************************************************************
*
* find ln of complex number for internal routines
*
******************************************************************************/

/* NB z = r.e^i.theta -> ln(z) = ln(r) + i.theta */

_Check_return_
static STATUS
complex_lnz(
    _InRef_     PC_COMPLEX in,
    _OutRef_    P_COMPLEX out)
{
    const F64 r_squared = mx_fsquare(in->r) + mx_fsquare(in->i);
    STATUS status = STATUS_OK;

    errno = 0;

    out->r = log(r_squared) * 0.5; /* x saves a sqrt() */

    if(errno /* == ERANGE */ /*can't be EDOM here*/)
        status = EVAL_ERR_BAD_LOG;

    out->i = atan2(in->i, in->r);

    return(status);
}

/******************************************************************************
*
* make a complex number result from an internal complex number type
*
******************************************************************************/

static inline void
complex_result_complex(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_COMPLEX z,
    _InVal_     U8 imag_suffix_char)
{
    if(CH_NULL != imag_suffix_char)
    {
#if defined(COMPLEX_STRING)
        complex_result_reals_string(p_ev_data, z->r, z->i, imag_suffix_char);
#endif
        return;
    }

    complex_result_reals(p_ev_data, z->r, z->i);
}

/******************************************************************************
*
* make a complex number result from the given complex number
*
******************************************************************************/

static void
complex_result_reals(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     F64 real_part,
    _InVal_     F64 imag_part)
{
    P_EV_DATA elep;

    if(status_fail(ss_array_make(p_ev_data, 2, 1)))
        return;

    elep = ss_array_element_index_wr(p_ev_data, 0, 0);
    ev_data_set_real(elep, real_part);

    elep = ss_array_element_index_wr(p_ev_data, 1, 0);
    ev_data_set_real(elep, imag_part);
}

#if defined(COMPLEX_STRING)

static inline void
complex_result_complex_string(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_COMPLEX z,
    _InVal_     U8 imag_suffix_char)
{
    complex_result_reals_string(p_ev_data, z->r, z->i, imag_suffix_char);
}

static U32 /* bytes currently in output */
decode_fp(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     F64 f64)
{
    U32 len = ustr_xsnprintf(ustr_buf, elemof_buffer, USTR_TEXT("%.15g"), f64);
    P_U8 exp;

    /* search for exponent and remove leading zeros because they are confusing */
    /* also remove the + for good measure */
    if(NULL != (exp = strchr((char *) ustr_buf, 'e')))
    {
        U8 sign = *(++exp);
        P_U8 exps;

        if(CH_MINUS_SIGN__BASIC == sign)
            ++exp;

        exps = exp;

        if(CH_PLUS_SIGN == sign)
            ++exp;

        while(CH_DIGIT_ZERO == *exp)
            ++exp;

        if(exp != exps)
        {
            memmove32(exps, exp, strlen32p1(exp) /*CH_NULL*/);

            len -= PtrDiffBytesU32(exp, exps);
        }
    }

    return(len);
}

static void
complex_result_reals_string(
    _OutRef_    P_EV_DATA p_ev_data,
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
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 50);
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
        ev_data_set_error(p_ev_data, status);
    else
        status_assert(ss_string_make_uchars(p_ev_data, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock)));

    quick_ublock_dispose(&quick_ublock);
}

#endif /* COMPLEX_STRING */

/******************************************************************************
*
* find w*ln(z) for internal routines
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
static STATUS
complex_wlnz(
    _InRef_     PC_COMPLEX w,
    _InRef_     PC_COMPLEX z,
    _OutRef_    P_COMPLEX out)
{
    COMPLEX lnz;

    status_return(complex_lnz(z, &lnz));

    out->r = w->r * lnz.r  -  w->i * lnz.i;
    out->i = w->r * lnz.i  +  w->i * lnz.r;

    return(STATUS_OK);
}

/******************************************************************************
*
* add two complex numbers
* (a+ib) + (c+id) = (a+c) + i(b+d)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_add)
{
    U8 imag_suffix_char;
    COMPLEX in1, in2, add_result;

    exec_func_ignore_parms();

    /* check the input is a pair of suitable complex numbers */
    if(status_fail(complex_check_arg_pair(p_ev_data_res, &in1, &in2, &imag_suffix_char, args[0], args[1])))
        return;

    add_result.r = in1.r + in2.r;
    add_result.i = in1.i + in2.i;

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &add_result, imag_suffix_char);
}

/******************************************************************************
*
* subtract second complex number from first
* (a+ib) - (c+id) = (a-c) + i(b-d)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sub)
{
    U8 imag_suffix_char;
    COMPLEX in1, in2, sub_result;

    exec_func_ignore_parms();

    /* check the input is a pair of suitable complex numbers */
    if(status_fail(complex_check_arg_pair(p_ev_data_res, &in1, &in2, &imag_suffix_char, args[0], args[1])))
        return;

    sub_result.r = in1.r - in2.r;
    sub_result.i = in1.i - in2.i;

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &sub_result, imag_suffix_char);
}

/******************************************************************************
*
* multiply two complex numbers
* (a+ib)*(c+id) = (ac-bd) + i(bc+ad)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_mul)
{
    U8 imag_suffix_char;
    COMPLEX in1, in2, mul_result;

    exec_func_ignore_parms();

    /* check the input is a pair of suitable complex numbers */
    if(status_fail(complex_check_arg_pair(p_ev_data_res, &in1, &in2, &imag_suffix_char, args[0], args[1])))
        return;

    mul_result.r = in1.r * in2.r - in1.i * in2.i;
    mul_result.i = in1.i * in2.r + in1.r * in2.i;

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &mul_result, imag_suffix_char);
}

/******************************************************************************
*
* divide two complex numbers
* (a+ib)/(c+id) = ((ac+bd) + i(bc-ad)) / (c*c + d*d)
*
******************************************************************************/

_Check_return_ _Success_(return)
static BOOL
do_complex_divide(
    /**/        P_EV_DATA p_ev_data_res,
    _InRef_     PC_COMPLEX in1,
    _InRef_     PC_COMPLEX in2,
    _OutRef_    P_COMPLEX out)
{
    /* c*c + d*d */
    const F64 divisor = mx_fsquare(in2->r) + mx_fsquare(in2->i);

    /* check for divide by zero about to trap */
    if(divisor < F64_MIN)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
        return(FALSE);
    }

    out->r = (in1->r * in2->r + in1->i * in2->i) / divisor;
    out->i = (in1->i * in2->r - in1->r * in2->i) / divisor;

    return(TRUE);
}

PROC_EXEC_PROTO(c_c_div)
{
    U8 imag_suffix_char;
    COMPLEX in1, in2, div_result;

    exec_func_ignore_parms();

    /* check the input is a pair of suitable complex numbers */
    if(status_fail(complex_check_arg_pair(p_ev_data_res, &in1, &in2, &imag_suffix_char, args[0], args[1])))
        return;

    if(!do_complex_divide(p_ev_data_res, &in1, &in2, &div_result))
        return;

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &div_result, imag_suffix_char);
}

/******************************************************************************
*
* COMPLEX c_complex(real_part:number, imag_part:number {, suffix:string})
*
* STRING odf.complex(real_part:number, imag_part:number {, suffix:string})
*
******************************************************************************/

static inline void
c_c_complex_common(
    _In_reads_(n_args) P_EV_DATA args[],
    _InVal_     S32 n_args,
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InVal_     U8 default_imag_suffix_char)
{
    COMPLEX complex_result;
    U8 imag_suffix_char = default_imag_suffix_char;

    assert(n_args > 0);
    complex_result.r = args[0]->arg.fp;

    complex_result.i = (n_args > 1) ? args[1]->arg.fp : 0.0;

    if(n_args > 2)
    {
        PC_UCHARS uchars = args[2]->arg.string.uchars;

        if( (1 != args[2]->arg.string.size) || ((PtrGetByte(uchars) != 'i') && (PtrGetByte(uchars) != 'j')) )
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_BADCOMPLEX);
            return;
        }

        imag_suffix_char = PtrGetByte(uchars);
    }

    complex_result_complex(p_ev_data_res, &complex_result, imag_suffix_char);
}

PROC_EXEC_PROTO(c_c_complex)
{
    exec_func_ignore_parms();

    c_c_complex_common(args, n_args, p_ev_data_res, CH_NULL /* only a string result if suffix supplied */);
}

PROC_EXEC_PROTO(c_odf_complex)
{
    exec_func_ignore_parms();

    c_c_complex_common(args, n_args, p_ev_data_res, 'i' /* always a string result, may be overridden */);
}

/******************************************************************************
*
* COMPLEX find complex conjugate of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_conjugate)
{
    U8 imag_suffix_char;
    COMPLEX in, conjugate_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    conjugate_result.r =   in.r;
    conjugate_result.i = - in.i;

    complex_result_complex(p_ev_data_res, &conjugate_result, imag_suffix_char);
}

/******************************************************************************
*
* REAL find real part of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_real)
{
    U8 imag_suffix_char;
    COMPLEX in;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    ev_data_set_real(p_ev_data_res, in.r);
}

/******************************************************************************
*
* REAL find imaginary part of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_imaginary)
{
    U8 imag_suffix_char;
    COMPLEX in;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    ev_data_set_real(p_ev_data_res, in.i);
}

/******************************************************************************
*
* REAL find radius of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_radius)
{
    U8 imag_suffix_char;
    COMPLEX in;
    F64 radius_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    radius_result = mx_fhypot(in.r, in.i); /* SKS does carefully */

    ev_data_set_real(p_ev_data_res, radius_result);
}

/******************************************************************************
*
* REAL find theta of complex number
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_theta)
{
    U8 imag_suffix_char;
    COMPLEX in;
    F64 theta_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    theta_result = atan2(in.i, in.r); /* note silly C library ordering */

    ev_data_set_real(p_ev_data_res, theta_result);
}

/******************************************************************************
*
* round(complex number {, decimal_places:number=2})
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_round)
{
    U8 imag_suffix_char;
    COMPLEX in, round_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    { /* round components separately to the desired number of decimal places */
    EV_DATA number, decimal_places;
    P_EV_DATA local_args[2];

    local_args[0] = &number;
    local_args[1] = &decimal_places;

    if(n_args > 1)
        ev_data_set_integer(&decimal_places, MIN(15, args[1]->arg.integer));
    else
        ev_data_set_integer(&decimal_places, 2);

    ev_data_set_real(&number, in.r);
    round_common(local_args, 2, p_ev_data_res, RPN_FNV_ROUND);
    assert(RPN_DAT_REAL == p_ev_data_res->did_num);
    round_result.r = p_ev_data_res->arg.fp;

    ev_data_set_real(&number, in.i);
    round_common(local_args, 2, p_ev_data_res, RPN_FNV_ROUND);
    assert(RPN_DAT_REAL == p_ev_data_res->did_num);
    round_result.i = p_ev_data_res->arg.fp;
    } /*block*/

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &round_result, imag_suffix_char);
}

/******************************************************************************
*
* ln(complex number)
* ln(a+ib) = ln(a*a + b*b)/2 + i(atan2(b,a))
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_ln)
{
    U8 imag_suffix_char;
    COMPLEX in, ln_result;
    STATUS status;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    if(status_fail(status = complex_lnz(&in, &ln_result)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &ln_result, imag_suffix_char);
}

/******************************************************************************
*
* log_10(complex number)
*
* z = r.e^i.theta -> ln(z) = ln(r) + i.theta
*
* log.a(z) = log.a(r) + (i.theta) . log.a(e)
*
******************************************************************************/

_Check_return_
static inline STATUS
complex_log10_z(
    _InRef_     PC_COMPLEX in,
    _OutRef_    P_COMPLEX out)
{
    const F64 r_squared = mx_fsquare(in->r) + mx_fsquare(in->i);
    STATUS status = STATUS_OK;

    errno = 0;

    out->r = log10(r_squared) * 0.5; /* x saves a sqrt() */

    if(errno /* == ERANGE */ /*can't be EDOM here*/)
        status = EVAL_ERR_BAD_LOG;

    out->i = atan2(in->i, in->r) * _log10_e; /* rotate */

    return(status);
}

PROC_EXEC_PROTO(c_c_log_10)
{
    U8 imag_suffix_char;
    COMPLEX in, log10_result;
    STATUS status;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    if(status_fail(status = complex_log10_z(&in, &log10_result)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &log10_result, imag_suffix_char);
}

/******************************************************************************
*
* log_2(complex number)
*
******************************************************************************/

_Check_return_
static inline STATUS
complex_log2_z(
    _InRef_     PC_COMPLEX in,
    _OutRef_    P_COMPLEX out)
{
    const F64 r_squared = mx_fsquare(in->r) + mx_fsquare(in->i);
    STATUS status = STATUS_OK;

    errno = 0;

    out->r = log2(r_squared) * 0.5; /* x saves a sqrt() */

    if(errno /* == ERANGE */ /*can't be EDOM here*/)
        status = EVAL_ERR_BAD_LOG;

    out->i = atan2(in->i, in->r) * _log2_e; /* rotate */

    return(status);
}

PROC_EXEC_PROTO(c_c_log_2)
{
    U8 imag_suffix_char;
    COMPLEX in, log2_result;
    STATUS status;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    if(status_fail(status = complex_log2_z(&in, &log2_result)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &log2_result, imag_suffix_char);
}

/******************************************************************************
*
* complex z^w
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
static STATUS
do_complex_power(
    _InRef_     PC_COMPLEX in1,
    _InRef_     PC_COMPLEX in2,
    _OutRef_    P_COMPLEX out)
{
    COMPLEX wlnz;

    /* find and check wlnz */
    status_return(complex_wlnz(in2, in1, &wlnz));

    out->r = exp(wlnz.r) * cos(wlnz.i);
    out->i = exp(wlnz.r) * sin(wlnz.i);

    return(STATUS_OK);
}

PROC_EXEC_PROTO(c_c_power)
{
    U8 imag_suffix_char;
    COMPLEX in1, in2, power_result;
    STATUS status;

    exec_func_ignore_parms();

    /* check the input is a pair of suitable complex numbers */
    if(status_fail(complex_check_arg_pair(p_ev_data_res, &in1, &in2, &imag_suffix_char, args[0], args[1])))
        return;

    if(status_fail(status = do_complex_power(&in1, &in2, &power_result)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &power_result, imag_suffix_char);
}

PROC_EXEC_PROTO(c_c_sqrt)
{
    U8 imag_suffix_char;
    COMPLEX in, power_result;
    STATUS status;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    if(status_fail(status = do_complex_power(&in, &complex_one_half, &power_result)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &power_result, imag_suffix_char);
}

/******************************************************************************
*
* exp(complex number)
* exp(a+ib) = exp(a) * cos(b) + i(exp(a) * sin(b))
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_exp)
{
    U8 imag_suffix_char;
    COMPLEX in, exp_result;
    F64 ea;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    /* make exp(a) */
    ea = exp(in.r);

    exp_result.r = ea * cos(in.i);
    exp_result.i = ea * sin(in.i);

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &exp_result, imag_suffix_char);
}

/******************************************************************************
*
* sin(complex number)
* sin(a+ib) = (exp(-b)+exp(b))sin(a)/2 + i((exp(b)-exp(-b))cos(a)/2)
*
******************************************************************************/

static void
do_complex_sin(
    _InRef_     PC_COMPLEX in,
    _OutRef_    P_COMPLEX out)
{
    /* make exp(b) and exp(-b) */
    const F64 eb = exp(in->i);
    const F64 emb = 1.0 / eb;

    out->r = (eb + emb) * sin(in->r) * 0.5;
    out->i = (eb - emb) * cos(in->r) * 0.5;
}

PROC_EXEC_PROTO(c_c_sin)
{
    U8 imag_suffix_char;
    COMPLEX in, sin_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    do_complex_sin(&in, &sin_result);

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &sin_result, imag_suffix_char);
}

/******************************************************************************
*
* cos(complex number)
* cos(a+ib) = (exp(-b)+exp(b))cos(a)/2 + i((exp(-b)-exp(b))sin(a)/2)
*
******************************************************************************/

static void
do_complex_cos(
    _InRef_     PC_COMPLEX in,
    _OutRef_    P_COMPLEX out)
{
    /* make exp(b) and exp(-b) */
    const F64 eb = exp(in->i);
    const F64 emb = 1.0 / eb;

    out->r = (emb + eb) * cos(in->r) * 0.5;
    out->i = (emb - eb) * sin(in->r) * 0.5;
}

PROC_EXEC_PROTO(c_c_cos)
{
    U8 imag_suffix_char;
    COMPLEX in, cos_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    do_complex_cos(&in, &cos_result);

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &cos_result, imag_suffix_char);
}

/******************************************************************************
*
* tan(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_tan)
{
    U8 imag_suffix_char;
    COMPLEX in, sin, cos, tan_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    do_complex_sin(&in, &sin);
    do_complex_cos(&in, &cos);

    if(!do_complex_divide(p_ev_data_res, &sin, &cos, &tan_result))
        return;

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &tan_result, imag_suffix_char);
}

/******************************************************************************
*
* cosec(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cosec)
{
    U8 imag_suffix_char;
    COMPLEX in, temp, cosec_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    do_complex_sin(&in, &temp);

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &temp, &cosec_result))
        return;

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &cosec_result, imag_suffix_char);
}

/******************************************************************************
*
* sec(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sec)
{
    U8 imag_suffix_char;
    COMPLEX in, temp, sec_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    do_complex_cos(&in, &temp);

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &temp, &sec_result))
        return;

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &sec_result, imag_suffix_char);
}

/******************************************************************************
*
* cot(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cot)
{
    U8 imag_suffix_char;
    COMPLEX in, sin, cos, cot_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    do_complex_sin(&in, &sin);
    do_complex_cos(&in, &cos);

    if(!do_complex_divide(p_ev_data_res, &cos, &sin, &cot_result))
        return;

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &cot_result, imag_suffix_char);
}

/******************************************************************************
*
* sinh(complex number)
* sinh(a+ib) = (exp(a)-exp(-a))cos(b)/2 + i((exp(a)+exp(-a))sin(b)/2)
*
******************************************************************************/

static void
do_complex_sinh(
    _InRef_     PC_COMPLEX in,
    _OutRef_    P_COMPLEX out)
{
    /* make exp(a) and exp(-a) */
    const F64 ea = exp(in->r);
    const F64 ema = 1.0 / ea;

    out->r = (ea - ema) * cos(in->i) * 0.5;
    out->i = (ea + ema) * sin(in->i) * 0.5;
}

PROC_EXEC_PROTO(c_c_sinh)
{
    U8 imag_suffix_char;
    COMPLEX in, sinh_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    do_complex_sinh(&in, &sinh_result);

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &sinh_result, imag_suffix_char);
}

/******************************************************************************
*
* cosh(complex number)
* cosh(a+ib) = (exp(a)+exp(-a))cos(b)/2 + i((exp(a)-exp(-a))sin(b)/2)
*
******************************************************************************/

static void
do_complex_cosh(
    _InRef_     PC_COMPLEX in,
    _OutRef_    P_COMPLEX out)
{
    /* make exp(a) and exp(-a) */
    const F64 ea = exp(in->r);
    const F64 ema = 1.0 / ea;

    out->r = (ea + ema) * cos(in->i) * 0.5;
    out->i = (ea - ema) * sin(in->i) * 0.5;
}

PROC_EXEC_PROTO(c_c_cosh)
{
    U8 imag_suffix_char;
    COMPLEX in, cosh_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    do_complex_cosh(&in, &cosh_result);

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &cosh_result, imag_suffix_char);
}

/******************************************************************************
*
* tanh(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_tanh)
{
    U8 imag_suffix_char;
    COMPLEX in, sin, cos, tanh_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    do_complex_sinh(&in, &sin);
    do_complex_cosh(&in, &cos);

    if(!do_complex_divide(p_ev_data_res, &sin, &cos, &tanh_result))
        return;

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &tanh_result, imag_suffix_char);
}

/******************************************************************************
*
* cosech(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_cosech)
{
    U8 imag_suffix_char;
    COMPLEX in, temp, cosech_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    do_complex_sinh(&in, &temp);

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &temp, &cosech_result))
        return;

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &cosech_result, imag_suffix_char);
}

/******************************************************************************
*
* sech(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_sech)
{
    U8 imag_suffix_char;
    COMPLEX in, temp, sech_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    do_complex_cosh(&in, &temp);

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &temp, &sech_result))
        return;

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &sech_result, imag_suffix_char);
}

/******************************************************************************
*
* coth(complex number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_c_coth)
{
    U8 imag_suffix_char;
    COMPLEX in, sin, cos, coth_result;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    do_complex_sinh(&in, &sin);
    do_complex_cosh(&in, &cos);

    if(!do_complex_divide(p_ev_data_res, &cos, &sin, &coth_result))
        return;

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &coth_result, imag_suffix_char);
}

/******************************************************************************
*
* do the work for arccosh and arcsinh and their many relations
*
* arccosh(z) = ln(z + (z*z - 1) ^.5)
* arcsinh(z) = ln(z + (z*z + 1) ^.5)
*
* rob thinks the following apply, from the expansions given further on
* arccos(z) = -i arccosh(z)
* arcsin(z) =  i arcsinh(-iz)
*
*   arccosh(z) = +- i arccos(z)
* i arccosh(z) =      arccos(z)
*
*   arcsinh(z)   = -i arcsin(iz)
* i arcsinh(z)   =    arcsin(iz)
* i arcsinh(z/i) =    arcsin(z)
* i arcsinh(-iz) =    arcsin(z)
*
*   arctanh(z)   = -i arctan(iz)
*   arctanh(-iz) = -i arctan(z)
* i arctanh(-iz) =    arctan(z)
*
******************************************************************************/

#define C_COSH 1
#define C_SINH 2
#define C_TANH 3

static void
do_arc_cosh_sinh_tanh(
    P_EV_DATA p_ev_data_res,
    _InVal_     S32 type,
    _InoutRef_  P_COMPLEX z,
    _InVal_     U8 imag_suffix_char,
    _InRef_opt_ PC_F64 mult_z_by_i,
    _InRef_opt_ PC_F64 mult_res_by_i)
{
    COMPLEX out;
    COMPLEX half;
    COMPLEX temp;
    STATUS status;

    /* maybe preprocess z
        multiply the input by   i * mult_z_by_i
         i(a + ib) = -b + ia
        -i(a + ib) =  b - ia
        mult_z_by_i is 1 to multiply by i, -1 to multiply by -i
    */
    if(NULL != mult_z_by_i)
    {
        F64 t = z->r;

        z->r = z->i * -(*mult_z_by_i);
        z->i = t    *  (*mult_z_by_i);
    }

    if(type == C_TANH)
    {
        /* do temp = (1+z)/(1-z) */
        COMPLEX in1, in2;

        in1.r = 1.0 + z->r;
        in1.i =   z->i;
        in2.r = 1.0 - z->r;
        in2.i = - z->i;

        if(!do_complex_divide(p_ev_data_res, &in1, &in2, &temp))
            return;
    }
    else
    {
        /* find z*z */
        out.r = z->r * z->r - z->i * z->i;
        out.i = z->r * z->i * 2.0;

        /* z*z + add_in_middle */
        out.r += (type == C_COSH ? -1.0 : +1.0);

        /* sqrt it into temp */
        half.r = 0.5;
        half.i = 0.0;
        if(status_fail(status = do_complex_power(&out, &half, &temp)))
        {
            ev_data_set_error(p_ev_data_res, status);
            return;
        }

        /* z + it  */
        temp.r += z->r;
        temp.i += z->i;
    }

    /* ln it to out */
    if(status_fail(status = complex_lnz(&temp, &out)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    /* now its in out, halve it for arctans */
    if(type == C_TANH)
    {
        /* halve it */
        out.r /= 2.0;
        out.i /= 2.0;
    }

    /* maybe postprocess out
        multiply the output by   i * mult_res_by_i
         i(a+ ib)  = -b + ia
        -i(a + ib) =  b - ia
        mult_res_by_i is 1 to multiply by i, -1 to multiply by -i
    */
    if(NULL != mult_res_by_i)
    {
        F64 t = out.r;

        out.r = out.i  * -(*mult_res_by_i);
        out.i = t      *  (*mult_res_by_i);
    }

    /* output a complex number */
    complex_result_complex(p_ev_data_res, &out, imag_suffix_char);
}

/*
complex arctan
*/

PROC_EXEC_PROTO(c_c_atan)
{
    U8 imag_suffix_char;
    COMPLEX z;

    static const F64 c_catan_z   = -1.0;
    static const F64 c_catan_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &z, &imag_suffix_char, args[0])))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_TANH, &z, imag_suffix_char, &c_catan_z, &c_catan_res);
}

/*
complex arctanh
*/

PROC_EXEC_PROTO(c_c_atanh)
{
    U8 imag_suffix_char;
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &z, &imag_suffix_char, args[0])))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_TANH, &z, imag_suffix_char, NULL, NULL);
}

/*
complex arccos
*/

PROC_EXEC_PROTO(c_c_acos)
{
    U8 imag_suffix_char;
    COMPLEX z;

    static const F64 c_cacos_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &z, &imag_suffix_char, args[0])))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_COSH, &z, imag_suffix_char, NULL, &c_cacos_res);
}

/*
complex arccosh
*/

PROC_EXEC_PROTO(c_c_acosh)
{
    U8 imag_suffix_char;
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &z, &imag_suffix_char, args[0])))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_COSH, &z, imag_suffix_char, NULL, NULL);
}

/*
complex arcsin
*/

PROC_EXEC_PROTO(c_c_asin)
{
    U8 imag_suffix_char;
    COMPLEX z;

    static const F64 c_asin_z   = -1.0;
    static const F64 c_asin_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &z, &imag_suffix_char, args[0])))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_SINH, &z, imag_suffix_char, &c_asin_z, &c_asin_res);
}

/*
complex arcsinh
*/

PROC_EXEC_PROTO(c_c_asinh)
{
    U8 imag_suffix_char;
    COMPLEX z;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &z, &imag_suffix_char, args[0])))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_SINH, &z, imag_suffix_char, NULL, NULL);
}

/*
complex arccot
*/

PROC_EXEC_PROTO(c_c_acot)
{
    U8 imag_suffix_char;
    COMPLEX in, z;

    static const F64 c_cacot_z   = -1.0;
    static const F64 c_cacot_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_TANH, &z, imag_suffix_char, &c_cacot_z, &c_cacot_res);
}

/*
complex arccoth
*/

PROC_EXEC_PROTO(c_c_acoth)
{
    U8 imag_suffix_char;
    COMPLEX in, z;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_TANH, &z, imag_suffix_char, NULL, NULL);
}

/*
complex arcsec
*/

PROC_EXEC_PROTO(c_c_asec)
{
    U8 imag_suffix_char;
    COMPLEX in, z;

    static const F64 c_casec_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_COSH, &z, imag_suffix_char, NULL, &c_casec_res);
}

/*
complex arcsech
*/

PROC_EXEC_PROTO(c_c_asech)
{
    U8 imag_suffix_char;
    COMPLEX in, z;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_COSH, &z, imag_suffix_char, NULL, NULL);
}

/*
complex arccosec
*/

PROC_EXEC_PROTO(c_c_acosec)
{
    U8 imag_suffix_char;
    COMPLEX in, z;

    static const F64 c_acosec_z   = -1.0;
    static const F64 c_acosec_res = +1.0;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_SINH, &z, imag_suffix_char, &c_acosec_z, &c_acosec_res);
}

/*
complex arccosech
*/

PROC_EXEC_PROTO(c_c_acosech)
{
    U8 imag_suffix_char;
    COMPLEX in, z;

    exec_func_ignore_parms();

    /* check the input is a suitable complex number */
    if(status_fail(complex_check_arg(p_ev_data_res, &in, &imag_suffix_char, args[0])))
        return;

    if(!do_complex_divide(p_ev_data_res, &complex_unity, &in, &z))
        return;

    do_arc_cosh_sinh_tanh(p_ev_data_res, C_SINH, &z, imag_suffix_char, NULL, NULL);
}

/* [that's enough complex algebra - Ed] */

/* end of ev_mcpx.c */
