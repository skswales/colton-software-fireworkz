/* ss_const.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Fireworkz constant handling */

/* MRJC April 1992 / August 1993 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"
#endif

const F64
g_real_zero = 0.0;

const SS_DATA
ss_data_real_zero =
{
#if defined(SS_DATA_HAS_EV_IDNO_IN_TOP_16_BITS)
    0, 0, DATA_ID_REAL,
#else
    DATA_ID_REAL, 0, 0,
#endif
    { 0.0 }
};

/******************************************************************************
*
* free an array - we have to check each element for strings which themselves need deallocating
*
******************************************************************************/

static void
array_free(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    S32 ix, iy;

    trace_2(TRACE_MODULE_EVAL, TEXT("ss_array_free x: ") S32_TFMT TEXT(", y: ") S32_TFMT, p_ss_data->arg.ss_array.x_size, p_ss_data->arg.ss_array.y_size);

    for(iy = 0; iy < p_ss_data->arg.ss_array.y_size; ++iy)
        for(ix = 0; ix < p_ss_data->arg.ss_array.x_size; ++ix)
            ss_data_free_resources(ss_array_element_index_wr(p_ss_data, ix, iy));

    al_array_dispose(&p_ss_data->arg.ss_array.elements);

    p_ss_data->local_data = 0;

    ss_data_set_blank(p_ss_data);
}

/******************************************************************************
*
* convert floating point number to string
*
******************************************************************************/

_Check_return_
static STATUS
decode_fp(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_        F64 fpval) /* Norcroft compiler barfs if const */
{
    UCHARZ ustr_buf[50];
    P_U8 exp;
    U32 len;

    if(!isfinite(fpval)) /* test for NaN and infinity */
    {
        const F64 test = copysign(1.0, fpval); /* try to handle -nan and -inf */

        if(isnan(fpval))
            len = ustr_xstrkpy(ustr_bptr(ustr_buf), elemof32(ustr_buf), (test < 0.0) ? USTR_TEXT("-nan") : USTR_TEXT("nan"));
        else
            len = ustr_xstrkpy(ustr_bptr(ustr_buf), elemof32(ustr_buf), (test < 0.0) ? USTR_TEXT("-inf") : USTR_TEXT("inf"));

        --len; /* ignore CH_NULL */

        return(quick_ublock_uchars_add(p_quick_ublock, uchars_bptr(ustr_buf), len));
    }

    len = ustr_xsnprintf(ustr_bptr(ustr_buf), elemof32(ustr_buf), USTR_TEXT("%.15g"), fpval);

    /* search for exponent and remove leading zeros because they are confusing */
    /* also remove the + for good measure */
    if(NULL != (exp = strchr(ustr_buf, 'e')))
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

    /* see what's happening with decimal points */
    if((NULL == exp) || (CH_FULL_STOP != g_ss_recog_context.decimal_point_char))
    {
        P_USTR ustr_dp = ustrchr(ustr_bptr(ustr_buf), CH_FULL_STOP);

        if(NULL != ustr_dp)
            PtrPutByte(ustr_dp, g_ss_recog_context.decimal_point_char);
        else if(NULL == exp)
        {
            /* not found a decimal point and not had an exponent */
            /* therefore make it clear that it is fp by tacking on a .0 */
            ustr_dp = ustr_AddBytes_wr(ustr_buf, ustrlen32(ustr_bptr(ustr_buf)));

            PtrPutByteOff(ustr_dp, 0, g_ss_recog_context.decimal_point_char);
            PtrPutByteOff(ustr_dp, 1, CH_DIGIT_ZERO);
            PtrPutByteOff(ustr_dp, 2, CH_NULL);

            len += 2;
        }
    }

    return(quick_ublock_uchars_add(p_quick_ublock, uchars_bptr(ustr_buf), len));
}

/******************************************************************************
*
* read an error
*
******************************************************************************/

_Check_return_ _Success_(return > 0)
static S32
recog_error(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_z_      PC_USTR in_ustr)
{
    PC_USTR ustr = in_ustr;
    PC_USTR ustr_end;
    S32 err;

    if(CH_NUMBER_SIGN != PtrGetByte(ustr))
        return(0);

    ustr_IncByte(ustr);

    err = - (S32) fast_ustrtoul(ustr, &ustr_end);

    if(ustr == ustr_end)
        return(0);

    if(err >= 0)
        err = STATUS_FAIL;

    ss_data_set_error(p_ss_data, err);
    p_ss_data->local_data = 1;

    return(PtrDiffBytesS32(ustr_end, in_ustr));
}

/******************************************************************************
*
* recognise a number, date, string and optionally an slr or range
*
******************************************************************************/

_Check_return_ _Success_(return > 0)
static STATUS
recog_date_number_string_error(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_z_      PC_USTR in_str)
{
    STATUS status;

    /* SKS 11apr95 moved to before ss_recog_number for foreign UI also for 1/2/96 recognition */
    if(0 != (status = ss_recog_date_time(p_ss_data, in_str)))
        return(status);

    if(0 != (status = ss_recog_number(p_ss_data, in_str)))
        return(status);

    if(0 != (status = ss_recog_string(p_ss_data, in_str)))
        return(status);

    if(0 != (status = ss_recog_logical(p_ss_data, in_str)))
        return(status);

    return(recog_error(p_ss_data, in_str));
}

/******************************************************************************
*
* recognise a row of an array
*
******************************************************************************/

_Check_return_
static STATUS
recog_array_row(
    P_SS_DATA p_ss_data_out,
    _In_z_      PC_USTR in_str,
    _InoutRef_  P_S32 p_ix,
    _InoutRef_  P_S32 p_iy)
{
    PC_USTR in_pos = in_str;
    STATUS status = STATUS_OK;

    do  {
        S32 len;
        SS_DATA ss_data;

        ss_data_set_blank(&ss_data);

        /* skip separator */
        ustr_IncByte(in_pos);

        /* skip blanks */
        while(CH_SPACE == PtrGetByte(in_pos))
            ustr_IncByte(in_pos);

        if((len = recog_date_number_string_error(&ss_data, in_pos)) <= 0)
        {
            status = len;
            break;
        }

        ustr_IncBytes(in_pos, len);

        if(*p_iy)
        {
            /* can't expand array on subsequent rows */
            if(*p_ix >= p_ss_data_out->arg.ss_array.x_size)
            {
                status = 0;
                break;
            }
        }

        /* put this data into array */
        if(status_ok(ss_array_element_make(p_ss_data_out, *p_ix, *p_iy)))
            *ss_array_element_index_wr(p_ss_data_out, *p_ix, *p_iy) = ss_data;
        else
        {
            status = status_nomem();
            break;
        }

        *p_ix += 1;
        status = 1;

        PtrSkipSpaces(PC_USTR, in_pos);
    }
    while(PtrGetByte(in_pos) == g_ss_recog_context.array_col_sep);

    return(status_done(status) ? PtrDiffBytesS32(in_pos, in_str) : status);
}

/******************************************************************************
*
* recognise a constant array
*
* --out--
* < 0 error
* >=0 characters processed
*
******************************************************************************/

_Check_return_
static STATUS
recog_array(
    P_SS_DATA p_ss_data,
    _In_z_      PC_USTR in_str)
{
    PC_USTR in_pos = in_str;
    S32 ix, iy;
    STATUS status = STATUS_OK;

    if(CH_LEFT_CURLY_BRACKET != PtrGetByte(in_pos))
        return(STATUS_OK);

    status_return(status = ss_array_make(p_ss_data, 0, 0));

    iy = 0;
    do  {
        ix = 0;
        if(status_done(status = recog_array_row(p_ss_data, in_pos, &ix, &iy))) /* SKS 22nov94 was status_ok so that {1;2,3} stuffed up big time */
        {
            ustr_IncBytes(in_pos, (U32) status);
            iy += 1;
        }
        else
        {
            ss_data_free_resources(p_ss_data);
            break;
        }
    }
    while(PtrGetByte(in_pos) == g_ss_recog_context.array_row_sep);

    /* check it's finished correctly */
    if(status_done(status))
    {
        if(CH_RIGHT_CURLY_BRACKET != PtrGetByte(in_pos))
            return(STATUS_OK);

        ustr_IncByte(in_pos);
    }

    return(status_done(status) ? PtrDiffBytesS32(in_pos, in_str) : status);
}

/******************************************************************************
*
* set error type into data element
*
******************************************************************************/

/*ncr*/
extern STATUS
ss_data_set_error(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     STATUS error)
{
    zero_struct_ptr(p_ss_data);
    ss_data_set_data_id(p_ss_data, DATA_ID_ERROR);
    p_ss_data->arg.ss_error.status = error;
    p_ss_data->arg.ss_error.type = ERROR_NORMAL;
    return(error);
}

/******************************************************************************
*
* floor() of real with possible ickle rounding
*
* rounding performed like real_trunc()
* so ((0.06-0.04)/0.01) coerced to integer is 2 not 1 etc.
*
******************************************************************************/

/* rounds at the given significant place before floor-ing */

_Check_return_
static inline_when_fast_fp F64
adjust_value_for_additional_rounding(
    _InVal_     F64 f64)
{
    int exponent;
    const F64 mantissa = frexp(f64, &exponent); /* yields mantissa in ±[0.5,1.0) */

    if(exponent < 0) /* no need to do more for negative exponents here */
        return(f64);

    {
    const int mantissa_digits_minus_n = DBL_MANT_DIG - 3;
    const int exponent_minus_mdmn = exponent - mantissa_digits_minus_n;
    const F64 rounding_value = ldexp(copysign(1.0, mantissa), exponent_minus_mdmn);
    const F64 adjusted_value = f64 + rounding_value; /* adjusted result */
    return(adjusted_value);
    } /*block*/
}

_Check_return_
static inline_when_fast_fp F64
real_floor_try_additional_rounding(
    _In_        F64 f64)
{
    if(isfinite(f64))
        return(floor(adjust_value_for_additional_rounding(f64)));

    return(f64);
}

_Check_return_
extern F64
real_floor(
    _InVal_     F64 f64)
{
    const F64 floor_value = floor(f64);

    /* first do the cheap step to see if we're already at an integer (or ±inf) */
    if(floor_value == f64)
        return(f64);

    if(!global_preferences.ss_calc_additional_rounding)
        return(floor_value); /* standard result */

    /* if not already an integer, and allowed, then do the more expensive bit */
    return(real_floor_try_additional_rounding(f64));
}

/******************************************************************************
*
* trunc() of real with possible ickle rounding
*
* SKS 06oct97 for INT() function try rounding an ickle bit
* so INT((0.06-0.04)/0.01) is 2 not 1
* and INT((0.06-0.04)/1E-6) is 20000 not 19999
* which is different to naive real_trunc()
*
******************************************************************************/

#if __STDC_VERSION__ < 199901L

static inline double
trunc(const double d)
{
    double trunc_result;
    (void) modf(d, &trunc_result);
    return(trunc_result);
}

#endif /* __STDC_VERSION__ */

/* rounds at the given significant place before truncating */

_Check_return_
static inline_when_fast_fp F64
real_trunc_try_additional_rounding(
    _In_        F64 f64)
{
    if(isfinite(f64))
        return(trunc(adjust_value_for_additional_rounding(f64)));

    return(f64);
}

_Check_return_
extern F64
real_trunc(
    _InVal_     F64 f64)
{
    const F64 trunc_value = trunc(f64);

    /* first do the cheap step to see if we're already at an integer (or ±inf) */
    if(trunc_value == f64)
        return(f64);

    if(!global_preferences.ss_calc_additional_rounding)
        return(trunc_value); /* standard result */

    /* if not already an integer, and allowed, then do the more expensive bit */
    return(real_trunc_try_additional_rounding(f64));
}

_Check_return_
extern F64
real_round(
    _InVal_     F64 f64)
{
    /* first do the cheap step to see if we're already at an integer (or ±inf) */
    if(floor(f64) == f64)
        return(f64);

    /* round away from zero */
    return(real_trunc(f64 + copysign(0.5, f64)));
}

/******************************************************************************
*
* force conversion of real to integer
*
* NB ss_data_real_to_integer_force uses real_floor() NOT real_trunc()
*
******************************************************************************/

_Check_return_
extern EV_IDNO /* DATA_ID_CONVERSION_FAILED iff not converted in range */
ss_data_real_to_integer_force(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    F64 f64;
    F64 floor_value;
    S32 s32;

    if(!ss_data_is_real(p_ss_data))
    {
        assert(ss_data_is_real(p_ss_data));
        return(ss_data_get_data_id(p_ss_data)); /* no conversion */
    }

    ss_data_copy_real(&f64, p_ss_data);

    floor_value = real_floor(f64);

    if( isgreater(fabs(floor_value), (F64) S32_MAX) || isnan(floor_value) ) /* test for NaN */
    {
        ss_data_set_integer(p_ss_data, isless(floor_value, 0.0) ? -S32_MAX : S32_MAX);
        return(DATA_ID_CONVERSION_FAILED); /* out of range */
    }

    s32 = (S32) floor_value;

    return(ss_data_set_integer_rid(p_ss_data, s32)); /* converted OK */
}

/******************************************************************************
*
* try to make an integer from a real if appropriate
*
* a bit like real_floor() but without the forcing
*
* returns TRUE if we produced an integer data value
*
******************************************************************************/

/*ncr*/
static BOOL
ss_data_real_to_integer_try_additional_rounding(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    F64 f64 = ss_data_get_real(p_ss_data);
    F64 floor_value;
    S32 s32;
    int exponent;
    const F64 mantissa = frexp(f64, &exponent); /* yields mantissa in ±[0.5,1.0) */

    if(exponent < 0) /* no need to do more for negative exponents here */
        return(FALSE); /* unmodified */

    {
    const int mantissa_digits_minus_n = DBL_MANT_DIG - 3;
    const int exponent_minus_mdmn = exponent - mantissa_digits_minus_n;
    const F64 rounding_value = ldexp(copysign(1.0, mantissa), exponent_minus_mdmn);
    const F64 adjusted_value = f64 + rounding_value;

    floor_value = floor(adjusted_value);

    if(floor_value != adjusted_value)
        return(FALSE); /* unmodified */

    if(isgreater(fabs(floor_value), (F64) S32_MAX))
    {   /* won't fit in S32 but we should hang on to this adjusted value */
        ss_data_set_real(p_ss_data, adjusted_value);
        return(FALSE); /* unmodified */
    }
    } /*block*/

    s32 = (S32) floor_value;
    ss_data_set_integer(p_ss_data, s32);
    return(TRUE); /* converted OK */
}

/*ncr*/
extern BOOL
ss_data_real_to_integer_try(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    F64 f64;
    F64 floor_value;
    S32 s32;

    if(!ss_data_is_real(p_ss_data))
    {
        assert(ss_data_is_real(p_ss_data));
        return(FALSE);
    }

    ss_data_copy_real(&f64, p_ss_data);

    /*if(!isfinite(f64))*/ /* test for NaN and infinity */ /* not needed */
    /*    return(FALSE);*/ /* unmodified */

    /* first do the cheap step to see if we're already at an integer */
    floor_value = floor(f64);

    if(floor_value != f64)
    {
        if(!global_preferences.ss_calc_additional_rounding)
            return(FALSE); /* unmodified */

        /* if not already an integer, and allowed, then do the more expensive bit */
        return(ss_data_real_to_integer_try_additional_rounding(p_ss_data));
    }

    if(isgreater(fabs(floor_value), (F64) S32_MAX))
        return(FALSE); /* unmodified */

    s32 = (S32) floor_value;
    ss_data_set_integer(p_ss_data, s32);
    return(TRUE); /* converted OK */
}

/******************************************************************************
*
* make a duplicate of an array
*
******************************************************************************/

_Check_return_
extern STATUS
ss_array_dup(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in)
{
    S32 x_size = p_ss_data_in->arg.ss_array.x_size;
    S32 y_size = p_ss_data_in->arg.ss_array.y_size;

    assert(ss_data_is_array(p_ss_data_in));

    status_return(ss_array_make(p_ss_data_out, x_size, y_size));

    {
    S32 iy;

    for(iy = 0; iy < y_size; ++iy)
    {
        S32 ix;

        for(ix = 0; ix < x_size; ++ix)
        {
            STATUS status = ss_data_resource_copy(ss_array_element_index_wr(p_ss_data_out, ix, iy),
                                                  ss_array_element_index_borrow(p_ss_data_in, ix, iy));

            if(status_fail(status))
            {
                ss_data_free_resources(p_ss_data_out);
                ss_data_set_error(p_ss_data_out, status);
                return(status);
            }
        }
    }
    } /*block*/

    return(STATUS_OK);
}

/******************************************************************************
*
* return pointer to array element data
*
******************************************************************************/

_Check_return_
_Ret_notnull_
extern P_SS_DATA
ss_array_element_index_wr(
    _InRef_     P_SS_DATA p_ss_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    const S32 element = (iy * p_ss_data_in->arg.ss_array.x_size) + ix;

    assert(ss_data_is_array(p_ss_data_in));
    assert(p_ss_data_in->local_data == 1);
    assert(((U32) ix < (U32) p_ss_data_in->arg.ss_array.x_size) && ((U32) iy < (U32) p_ss_data_in->arg.ss_array.y_size));

    return(array_ptr(&p_ss_data_in->arg.ss_array.elements, SS_DATA, element));
}

/******************************************************************************
*
* return pointer to array element data
* array doesn't need to be local
*
******************************************************************************/

_Check_return_
_Ret_valid_
extern PC_SS_DATA
ss_array_element_index_borrow(
    _InRef_     PC_SS_DATA p_ss_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    const S32 element = (iy * p_ss_data_in->arg.ss_array.x_size) + ix;

    assert(ss_data_is_array(p_ss_data_in));
    assert( ((U32) ix < (U32) p_ss_data_in->arg.ss_array.x_size) && ((U32) iy < (U32) p_ss_data_in->arg.ss_array.y_size) );
    assert(element >= 0);

    return(array_ptrc(&p_ss_data_in->arg.ss_array.elements, SS_DATA, element));
}

_Check_return_
_Ret_maybenone_
extern PC_SS_DATA
ss_array_element_index_borrow_check(
    _InRef_     PC_SS_DATA p_ss_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    const S32 element = (iy * p_ss_data_in->arg.ss_array.x_size) + ix;

    assert(ss_data_is_array(p_ss_data_in));
    assert( ((U32) ix < (U32) p_ss_data_in->arg.ss_array.x_size) && ((U32) iy < (U32) p_ss_data_in->arg.ss_array.y_size) );
    assert(element >= 0);

    if( ((U32) ix >= (U32) p_ss_data_in->arg.ss_array.x_size) || ((U32) iy >= (U32) p_ss_data_in->arg.ss_array.y_size) )
        return(_P_DATA_NONE(P_SS_DATA));

    return(array_ptrc(&p_ss_data_in->arg.ss_array.elements, SS_DATA, element));
}

/******************************************************************************
*
* ensure that a given array element exists
*
******************************************************************************/

_Check_return_
extern STATUS
ss_array_element_make(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(SS_DATA), TRUE);
    const S32 old_xs = p_ss_data->arg.ss_array.x_size;
    const S32 old_ys = p_ss_data->arg.ss_array.y_size;
    S32 new_xs;
    S32 new_ys;
    S32 old_size, new_size;
    STATUS status;

    if((ix < old_xs) && (iy < old_ys))
        return(STATUS_OK);

#if CHECKING
    /* SKS 22nov94 trap uncatered for resizing, whereby we'd have to delaminate the current array due to x growth with > 1 row */
    if(old_xs && old_ys)
        if(ix > old_xs)
            assert(old_ys <= 1);
#endif

    /* calculate number of extra elements needed */
    old_size = old_xs * old_ys;
    new_xs = MAX(ix + 1, old_xs);
    new_ys = MAX(iy + 1, old_ys);
    new_size = new_xs * new_ys;

    /* check not too many elements */
    if(new_size >= EV_MAX_ARRAY_ELES)
        return(STATUS_FAIL);

    if(NULL == al_array_extend_by(&p_ss_data->arg.ss_array.elements, SS_DATA, (new_size - old_size), &array_init_block, &status))
        return(status);

    trace_2(TRACE_MODULE_EVAL,
            TEXT("array realloced, now: ") S32_TFMT TEXT(" entries, ") S32_TFMT TEXT(" bytes"),
            new_xs * new_ys,
            new_size * (S32) sizeof32(SS_DATA));

    { /* set all new array elements to blank */
    P_SS_DATA p_ss_data_s, p_ss_data_e, p_ss_data_t;

    /* use old stored sizes to get pointer to end of old data */
    if( (old_xs < 1) || (old_ys < 1) )
        p_ss_data_s = array_base(&p_ss_data->arg.ss_array.elements, SS_DATA);
    else
        p_ss_data_s = ss_array_element_index_wr(p_ss_data, old_xs - 1, old_ys - 1) + 1;

    /* set up new sizes */
    p_ss_data->arg.ss_array.x_size = new_xs;
    p_ss_data->arg.ss_array.y_size = new_ys;

    /* use new stored size to get pointer to end of new array */
    p_ss_data_e = ss_array_element_index_wr(p_ss_data, new_xs - 1, new_ys - 1);

    for(p_ss_data_t = p_ss_data_s; p_ss_data_t <= p_ss_data_e; ++p_ss_data_t)
        ss_data_set_blank(p_ss_data_t);

    return(STATUS_OK);
    } /*block*/
}

/******************************************************************************
*
* read array element data
*
******************************************************************************/

extern void
ss_array_element_read(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_SS_DATA p_ss_data_src,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    const PC_SS_DATA p_ss_data_element = ss_array_element_index_borrow_check(p_ss_data_src, ix, iy);
    PTR_ASSERT(p_ss_data_element);
    if(PTR_IS_NONE(p_ss_data_element))
    {
        ss_data_set_blank(p_ss_data);
        return;
    }
    *p_ss_data = *p_ss_data_element;
    p_ss_data->local_data = 0;
}

/******************************************************************************
*
* make an array of a given size
*
******************************************************************************/

_Check_return_
extern STATUS
ss_array_make(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     S32 x_size,
    _InVal_     S32 y_size)
{
    STATUS status = STATUS_OK;

    ss_data_set_data_id(p_ss_data, DATA_ID_ARRAY);
    p_ss_data->local_data = 1;
    p_ss_data->arg.ss_array.x_size = 0;
    p_ss_data->arg.ss_array.y_size = 0;
    p_ss_data->arg.ss_array.elements = 0;

    if(x_size && y_size)
        if(status_fail(status = ss_array_element_make(p_ss_data, x_size - 1, y_size - 1)))
            ss_data_free_resources(p_ss_data);

    if(status_fail(status))
        ss_data_set_error(p_ss_data, status);

    return(status);
}

/******************************************************************************
*
* sets equal type numbers for equivalent types
*
******************************************************************************/

static void
type_equate_non_number(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     BOOL blanks_equal_zero)
{
    p_ss_data->local_data = 0;

    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_STRING:
        if(!ss_string_is_blank(p_ss_data))
            return;

        ss_data_set_blank(p_ss_data);
        /*FALLTHRU*/

    case DATA_ID_BLANK:
        if(blanks_equal_zero)
            ss_data_set_WORD32(p_ss_data, 0);
        return;
    }
}

static inline void
type_equate(
    _InoutRef_  P_SS_DATA p_ss_data,
    _InVal_     BOOL blanks_equal_zero)
{
    /* deal with number cases quickest (NB look at ARM DecAOF output in context) */
    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_REAL:
        return;

    case DATA_ID_WORD32:
        return;
    }

    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
        ss_data_set_data_id(p_ss_data, DATA_ID_WORD32); /* NB all integers returned from type_equate() are promoted to widest type */
        return;

    default:
        type_equate_non_number(p_ss_data, blanks_equal_zero);
        return;
    }
}

/******************************************************************************
*
* compare two data items
*
* NB string compare returns [-ve, 0, +ve] not [-1, 0, +1]
*
* --out--
* -1 p_ss_data_1 < p_ss_data_2
*  0 p_ss_data_1 = p_ss_data_2
* +1 p_ss_data_1 > p_ss_data_2
*
******************************************************************************/

_Check_return_
static S32
ss_data_compare_data_ids(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    S32 res;

    if(ss_data_get_data_id(p_ss_data_1) < ss_data_get_data_id(p_ss_data_2))
        res = -1;
    else if(ss_data_get_data_id(p_ss_data_1) > ss_data_get_data_id(p_ss_data_2))
        res = 1;
    else
        res = 0;

    return(res);
}

_Check_return_
static S32
ss_data_compare_reals_unordered(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    F64 f64_1;
    F64 f64_2 = ss_data_get_real(p_ss_data_2);

    /* This will stick NaNs up at the top of a sort ("To +infinity and beyond...") */
    if(isnan(f64_2))
    {
        f64_1 = ss_data_get_real(p_ss_data_1);

        if(isnan(f64_1))
            return(0); /* f1(NaN) '==' f2(NaN) */

     /* else */
            return(-1); /* f1 '<' f2(NaN) */
    }

 /* if(isnan(f64_1)) */
        return(1); /* f1(NaN) '>' f2 */
}

_Check_return_
static inline_when_fast_fp S32
ss_data_compare_reals(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    /* NB Do not muck about with this without watching the generated code ... */

    /* NaNs, being unordered, are never equal to each other or anything else */
    if(!isunordered(ss_data_get_real(p_ss_data_1), ss_data_get_real(p_ss_data_2)))
    {
        if(ss_data_get_real(p_ss_data_1) == ss_data_get_real(p_ss_data_2))
            return(0);

        /* Handle +/-inf */
        if(isless(ss_data_get_real(p_ss_data_1), ss_data_get_real(p_ss_data_2)))
            return(-1);

     /* if(isgreater(ss_data_get_real(p_ss_data_1), ss_data_get_real(p_ss_data_2))) */
            return(1);
    }

    return(ss_data_compare_reals_unordered(p_ss_data_1, p_ss_data_2));
}

_Check_return_
static inline S32
ss_data_compare_integers(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    if(p_ss_data_1->arg.integer == p_ss_data_2->arg.integer)
        return(0);

    if(p_ss_data_1->arg.integer < p_ss_data_2->arg.integer)
        return(-1);

 /* if(p_ss_data_1->arg.integer > p_ss_data_2->arg.integer) */
        return(1);
}

_Check_return_
static inline S32
ss_data_compare_dates(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    return(ss_date_compare(ss_data_get_date(p_ss_data_1), ss_data_get_date(p_ss_data_2)));
}

_Check_return_
static S32
ss_data_compare_strings(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2,
    _InVal_     BOOL allow_wild_match)
{
    S32 res = 0;

    if(allow_wild_match)
        res = uchars_compare_t5_nocase_wild(ss_data_get_string(p_ss_data_1), ss_data_get_string_size(p_ss_data_1), ss_data_get_string(p_ss_data_2), ss_data_get_string_size(p_ss_data_2));
    else
        res = uchars_compare_t5_nocase(ss_data_get_string(p_ss_data_1), ss_data_get_string_size(p_ss_data_1), ss_data_get_string(p_ss_data_2), ss_data_get_string_size(p_ss_data_2));

    if(res < 0)
        res = -1;
    else if(res > 0)
        res = 1;

    return(res);
}

_Check_return_
static S32
ss_data_compare_errors(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2)
{
    if(p_ss_data_1->arg.ss_error.status == p_ss_data_2->arg.ss_error.status)
        return(0);

    if(-(p_ss_data_1->arg.ss_error.status) < -(p_ss_data_2->arg.ss_error.status))
        return(-1);

 /* else */
        return(1);
}

_Check_return_
static S32
ss_data_compare_arrays(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2,
    _InVal_     BOOL blanks_equal_zero,
    _InVal_     BOOL allow_wild_match)
{
    S32 res = 0;
    S32 ix, iy;

    if(p_ss_data_1->arg.ss_array.y_size < p_ss_data_2->arg.ss_array.y_size)
        return(-1);

    if(p_ss_data_1->arg.ss_array.y_size > p_ss_data_2->arg.ss_array.y_size)
        return(1);

    if(p_ss_data_1->arg.ss_array.x_size < p_ss_data_2->arg.ss_array.x_size)
        return(-1);

    if(p_ss_data_1->arg.ss_array.x_size > p_ss_data_2->arg.ss_array.x_size)
        return(1);

    /* arrays are both the same size */
    for(iy = 0; iy < p_ss_data_1->arg.ss_array.y_size; ++iy)
    {
        for(ix = 0; ix < p_ss_data_1->arg.ss_array.x_size; ++ix)
        {
            res = ss_data_compare(ss_array_element_index_borrow(p_ss_data_1, ix, iy),
                                  ss_array_element_index_borrow(p_ss_data_2, ix, iy),
                                  blanks_equal_zero, allow_wild_match);

            if(0 != res)
                return(res);
        }
    }

    return(res);
}

_Check_return_
extern S32
ss_data_compare(
    _InRef_     PC_SS_DATA p_ss_data_1,
    _InRef_     PC_SS_DATA p_ss_data_2,
    _InVal_     BOOL blanks_equal_zero,
    _InVal_     BOOL allow_wild_match)
{
    /* take copies and then eliminate equivalent types */
    SS_DATA ss_data_1 = *p_ss_data_1;
    SS_DATA ss_data_2 = *p_ss_data_2;

    type_equate(&ss_data_1, blanks_equal_zero);
    type_equate(&ss_data_2, blanks_equal_zero);

    consume(enum two_nums_type_match_result, two_nums_type_match(&ss_data_1, &ss_data_2));

    if(ss_data_get_data_id(&ss_data_1) != ss_data_get_data_id(&ss_data_2))
        return(ss_data_compare_data_ids(&ss_data_1, &ss_data_2));

    switch(ss_data_get_data_id(&ss_data_1))
    {
    case DATA_ID_REAL:
        return(ss_data_compare_reals(&ss_data_1, &ss_data_2));

    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        return(ss_data_compare_integers(&ss_data_1, &ss_data_2));

    case DATA_ID_DATE:
        return(ss_data_compare_dates(&ss_data_1, &ss_data_2));

    case DATA_ID_STRING:
        return(ss_data_compare_strings(&ss_data_1, &ss_data_2, allow_wild_match));

    case DATA_ID_ERROR:
        return(ss_data_compare_errors(&ss_data_1, &ss_data_2));

    case DATA_ID_ARRAY:
        return(ss_data_compare_arrays(&ss_data_1, &ss_data_2, blanks_equal_zero, allow_wild_match));

    default: default_unhandled();
#if CHECKING
    case DATA_ID_BLANK:
#endif
        return(0);
    }
}

/******************************************************************************
*
* free resources owned by a data item
*
******************************************************************************/

extern void
ss_data_free_resources(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    /* NB p_ss_data->local_data might be uninitialised in all but cases that care about it */

    switch(ss_data_get_data_id(p_ss_data))
    {
    default:
        break;

    case DATA_ID_STRING:
        if(p_ss_data->local_data)
        {
            al_ptr_dispose(P_P_ANY_PEDANTIC(&p_ss_data->arg.string_wr.uchars));
            ss_data_set_blank(p_ss_data);
        }
        break;

    case DATA_ID_ARRAY:
        if(p_ss_data->local_data)
            array_free(p_ss_data);
        break;
    }

    /* SKS 05oct95 it's important for the above array_free that this is still set! */
    p_ss_data->local_data = 0;
}

/******************************************************************************
*
* given a data item, make a copy of it so we can be sure we own it
*
******************************************************************************/

_Check_return_
extern STATUS
ss_data_resource_copy(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_in)
{
    /* we get our own copy of handle based resources */
    switch(ss_data_get_data_id(p_ss_data_in))
    {
    case DATA_ID_STRING:
        return(ss_string_dup(p_ss_data_out, p_ss_data_in));

    case DATA_ID_ARRAY:
        return(ss_array_dup(p_ss_data_out, p_ss_data_in));

    default:
        *p_ss_data_out = *p_ss_data_in;
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* decode data (which must be a number type) as a Boolean value
*
******************************************************************************/

_Check_return_
extern bool
ss_data_get_logical(
    _InRef_     PC_SS_DATA p_ss_data)
{
    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_REAL:
        return(0.0 != ss_data_get_real(p_ss_data));

    case DATA_ID_LOGICAL:
        assert((p_ss_data->arg.logical_integer == 0U /*false*/) || (p_ss_data->arg.logical_integer == 1U /*true*/)); /* verify full width */
        return(p_ss_data->arg.logical_bool);

    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        return(0 != ss_data_get_integer(p_ss_data));

    default: default_unhandled();
        assert(ss_data_is_number(p_ss_data));
        return(FALSE);
    }
}

extern void
ss_data_set_logical(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     bool logical)
{
    ss_data_set_data_id(p_ss_data, DATA_ID_LOGICAL);
    p_ss_data->arg.logical_integer = 0U; /* set a full width integer in case bool is smaller */
    p_ss_data->arg.logical_bool = logical;
    assert((p_ss_data->arg.logical_integer == 0U /*false*/) || (p_ss_data->arg.logical_integer == 1U /*true*/)); /* verify full width */
}

/******************************************************************************
*
* decode data item to text
*
* NOTE: CH_NULL not added to output
*
******************************************************************************/

_Check_return_
static STATUS
ss_decode_constant_string(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_SS_DATA p_ss_data)
{
    STATUS status;
    PC_UCHARS uchars = ss_data_get_string(p_ss_data);
    const U32 len = ss_data_get_string_size(p_ss_data);

    status = quick_ublock_a7char_add(p_quick_ublock, CH_QUOTATION_MARK);

    if(status_ok(status))
    {
        if(NULL == memchr(uchars, CH_QUOTATION_MARK, len))
        {   /* no escaping needed - stuff it in the result */
            status = quick_ublock_uchars_add(p_quick_ublock, uchars, len);
        }
        else
        {
            U32 offset = 0;

            while(offset < len)
            {
                const U32 bytes_of_char = uchars_bytes_of_char_off(uchars, offset);

                if(CH_QUOTATION_MARK == PtrGetByteOff(uchars, offset))
                    status_break(status = quick_ublock_a7char_add(p_quick_ublock, CH_QUOTATION_MARK));

                status_break(status = quick_ublock_uchars_add(p_quick_ublock, uchars_AddBytes(uchars, offset), bytes_of_char));

                offset += bytes_of_char;
            }
        }
    }

    if(status_ok(status))
        status = quick_ublock_a7char_add(p_quick_ublock, CH_QUOTATION_MARK);

    return(status);
}

_Check_return_
static STATUS
ss_decode_constant_array(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_SS_DATA p_ss_data)
{
    STATUS status;

    status = quick_ublock_a7char_add(p_quick_ublock, CH_LEFT_CURLY_BRACKET);

    if(status_ok(status))
    {
        S32 iy;

        for(iy = 0; iy < p_ss_data->arg.ss_array.y_size; ++iy)
        {
            S32 ix;

            if(iy)
                status_break(status = quick_ublock_ucs4_add(p_quick_ublock, g_ss_recog_context.array_row_sep));

            for(ix = 0; ix < p_ss_data->arg.ss_array.x_size; ++ix)
            {
                SS_DATA ss_data;
                if(ix)
                    status_break(status = quick_ublock_ucs4_add(p_quick_ublock, g_ss_recog_context.array_col_sep));
                ss_array_element_read(&ss_data, p_ss_data, ix, iy);
                status_break(status = ss_decode_constant(p_quick_ublock, &ss_data));
                ss_data_free_resources(&ss_data);
            }

            status_break(status);
        }
    }

    if(status_ok(status))
        status = quick_ublock_a7char_add(p_quick_ublock, CH_RIGHT_CURLY_BRACKET);

    return(status);
}

_Check_return_
extern STATUS
ss_decode_constant(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_SS_DATA p_ss_data)
{
    STATUS status = STATUS_OK;

    switch(ss_data_get_data_id(p_ss_data))
    {
    case DATA_ID_REAL:
        status = decode_fp(p_quick_ublock, ss_data_get_real(p_ss_data));
        break;

    case DATA_ID_LOGICAL:
        status = quick_ublock_ustr_add(p_quick_ublock, ss_data_get_logical(p_ss_data) ? USTR_TEXT("TRUE") : USTR_TEXT("FALSE"));
        break;

    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        status = quick_ublock_printf(p_quick_ublock, USTR_TEXT(S32_FMT), ss_data_get_integer(p_ss_data));
        break;

    case DATA_ID_DATE:
        status = ss_date_decode(p_quick_ublock, ss_data_get_date(p_ss_data));
        break;

    case DATA_ID_STRING:
        status = ss_decode_constant_string(p_quick_ublock, p_ss_data);
        break;

    default: default_unhandled();
#if CHECKING
    case DATA_ID_BLANK:
#endif
        break;

    case DATA_ID_ERROR:
        status = quick_ublock_printf(p_quick_ublock, USTR_TEXT("#" S32_FMT), -(p_ss_data->arg.ss_error.status));
        break;

    case DATA_ID_ARRAY:
        status = ss_decode_constant_array(p_quick_ublock, p_ss_data);
        break;
    }

    return(status);
}

/******************************************************************************
*
* read a Boolean constant value (here ignoring true(), false() function calls)
*
* --out--
* =0 no constant found
* >0 constant found
*
******************************************************************************/

_Check_return_ _Success_(return > 0)
extern STATUS
ss_recog_logical(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_z_      PC_USTR in_str)
{
    PC_USTR pos = in_str;

    if(('F' == PtrGetByte(pos)) || ('f' == PtrGetByte(pos)))
    {   /* try FALSE */
        const U32 expected = 5;

        if( (0 == C_strnicmp(PtrAddBytes(const char *, pos, 1), /*f*/"alse", expected-1)) &&
            (CH_LEFT_PARENTHESIS != PtrGetByteOff(pos, expected)) &&
            !sbchar_isalnum(PtrGetByteOff(pos, expected)) )
        {
            ss_data_set_logical(p_ss_data, false);
            return((STATUS) expected);
        }

        return(STATUS_OK);
    }

    if(('T' == PtrGetByte(pos)) || ('t' == PtrGetByte(pos)))
    {   /* try TRUE */
        const U32 expected = 4;

        if( (0 == C_strnicmp(PtrAddBytes(const char *, pos, 1), /*t*/"rue", expected-1)) &&
            (CH_LEFT_PARENTHESIS != PtrGetByteOff(pos, expected)) &&
            !sbchar_isalnum(PtrGetByteOff(pos, expected)) )
        {
            ss_data_set_logical(p_ss_data, true);
            return((STATUS) expected);
        }

        return(STATUS_OK);
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* try to scan a constant:
* number, string or date
*
* --out--
* <0 error
* =0 nothing found
* >0 #characters scanned
*
******************************************************************************/

_Check_return_
extern STATUS
ss_recog_constant(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_z_      PC_USTR in_str)
{
    STATUS status = STATUS_OK;
    PC_USTR in_pos = in_str;
    S32 len;

    ss_data_set_blank(p_ss_data);

    ustr_SkipSpaces(in_pos);

    /* several callers will call us with raw input e.g. =1.2 */
    if(CH_EQUALS_SIGN == PtrGetByte(in_pos))
        ustr_IncByte(in_pos);

    if(CH_NULL == PtrGetByte(in_pos)) /* SKS 19apr95 check for early exit */
        return(STATUS_OK);

    if(((len = recog_date_number_string_error(p_ss_data, in_pos)) > 0)
       ||
       ((len = recog_array(p_ss_data, in_pos)) > 0)
      ) {
        ustr_IncBytes(in_pos, len);

        while((PtrGetByte(in_pos) == CH_SPACE) ||
              (PtrGetByte(in_pos) == LF)  ||
              (PtrGetByte(in_pos) == CR))
        {
            ustr_IncByte(in_pos);
        }

        /* check there's nothing following */
        if(CH_NULL == PtrGetByte(in_pos))
            status = STATUS_DONE;
        else
            ss_data_free_resources(p_ss_data);
    }
    else
        status = len;

    return((status > 0) ? PtrDiffBytesS32(in_pos, in_str) : status);
}

/******************************************************************************
*
* read a number
*
* --out--
* =0 no constant found
* >0 constant found
*
******************************************************************************/

_Check_return_ _Success_(return > 0)
extern STATUS
ss_recog_number(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_z_      PC_USTR in_str_in)
{
    /* SKS 15apr93 reworked to recognise .1 and 1E2 */
    /* SKS 11may93 helps out with large funny numbers too and gets round Acorn FP bug */
    S32 res = 0;
    BOOL negative;
    BOOL try_fp;
    PC_USTR in_str = in_str_in;
    PC_USTR epos;

    /* no need to skip leading spaces as this has been done */

    negative = (PtrGetByte(in_str) == CH_MINUS_SIGN__BASIC);

    if(negative || (PtrGetByte(in_str) == CH_PLUS_SIGN))
        ustr_IncByte(in_str);

    try_fp = (PtrGetByte(in_str) == g_ss_recog_context.decimal_point_char);

    if(!try_fp && sbchar_isdigit(PtrGetByte(in_str)))
    {
        U32 u32 = fast_ustrtoul(in_str, &epos);

        if((epos != in_str) && (PtrGetByte(epos) != g_ss_recog_context.time_sep_char))
        {
            try_fp = (PtrGetByte(epos) == g_ss_recog_context.decimal_point_char) || (PtrGetByte(epos) == 'e') || (PtrGetByte(epos) == 'E') || (u32 > (U32) S32_MAX) /*covers U32_MAX ERANGE case*/;

            if(!try_fp)
            {
                ss_data_set_integer(p_ss_data, (negative ? - (S32) u32 : (S32) u32));
                res = PtrDiffBytesS32(epos, in_str_in);
            }
        }
    }

    /* must have scanned something and not be a date or a real */

    if(try_fp)
    {
        F64 f64 = ui_strtod(in_str, &epos);

        /* must have scanned something and not be a date */
        if((epos != in_str) && (PtrGetByte(epos) != CH_FULL_STOP) && (PtrGetByte(epos) != g_ss_recog_context.time_sep_char))
        {
            ss_data_set_real(p_ss_data, negative ? -f64 : f64);
            /*ss_data_real_to_integer_try(p_ss_data);*//*SKS for 1.30 why the hell did it bother? means you can't preserve 1.0 typed in */
            res = PtrDiffBytesS32(epos, in_str_in);
        }
    }

    if(res)
        p_ss_data->local_data = 1;

    return(res);
}

/******************************************************************************
*
* given two numbers, reals or integers of various sizes,
* convert them so they are both compatible
*
******************************************************************************/

enum two_num_action
{
    TN_I,  /* both are integer */
    TN_IW, /* at least one is widest integer, take care with potential overflow */
    TN_RR, /* both are REAL */
    TN_R1, /* convert ss_data_1 to REAL */
    TN_R2, /* convert ss_data_2 to REAL */
    TN_RB, /* convert both ss_data_1 and ss_data_2 to REAL */
    TN_MIX
};

enum two_num_index
{
    TN_REAL,
    TN_LOGICAL,
    TN_WORD16,
    TN_WORD32,
    TN_OTHER
};

static const U8
tn_worry[5][5] =
{ /*  REA     L8      W16     W32     OTH */
    { TN_RR,  TN_R1,  TN_R1,  TN_R1,  TN_MIX }, /*REA*/
    { TN_R2,  TN_I,   TN_I,   TN_RB,  TN_MIX }, /*L8*/
    { TN_R2,  TN_I,   TN_I,   TN_RB,  TN_MIX }, /*W16*/
    { TN_R2,  TN_RB,  TN_RB,  TN_RB,  TN_MIX }, /*W32*/
    { TN_MIX, TN_MIX, TN_MIX, TN_MIX, TN_MIX }, /*OTH*/
};

static const U8
tn_no_worry[5][5] =
{ /*  REA     L8      W16     W32     OTH */
    { TN_RR,  TN_R1,  TN_R1,  TN_R1,  TN_MIX }, /*REA*/
    { TN_R2,  TN_I,   TN_I,   TN_IW,  TN_MIX }, /*L8*/
    { TN_R2,  TN_I,   TN_I,   TN_IW,  TN_MIX }, /*W16*/
    { TN_R2,  TN_IW,  TN_IW,  TN_IW,  TN_MIX }, /*W32*/
    { TN_MIX, TN_MIX, TN_MIX, TN_MIX, TN_MIX }, /*OTH*/
};

#if CHECKING

static void
check_tn(
    _InVal_     S32 did1,
    _InVal_     S32 did2)
{
    assert(DATA_ID_REAL == TN_REAL);
    assert(DATA_ID_LOGICAL == TN_LOGICAL);
    assert(DATA_ID_WORD16 == TN_WORD16);
    assert(DATA_ID_WORD32 == TN_WORD32);
    assert( (did1 >= TN_REAL) && ((U32) did1 <= (U32) TN_OTHER) );
    assert( (did2 >= TN_REAL) && ((U32) did2 <= (U32) TN_OTHER) );
}

#endif /* CHECKING */

/*ncr*/
static enum two_nums_type_match_result
two_nums_type_match_R1(
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    UNREFERENCED_PARAMETER_InoutRef_(p_ss_data_2);
    ss_data_set_real(p_ss_data_1, (F64) ss_data_get_integer(p_ss_data_1));
    return(TWO_REALS);
}

/*ncr*/
static enum two_nums_type_match_result
two_nums_type_match_R2(
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    UNREFERENCED_PARAMETER_InoutRef_(p_ss_data_1);
    ss_data_set_real(p_ss_data_2, (F64) ss_data_get_integer(p_ss_data_2));
    return(TWO_REALS);
}

/*ncr*/
static enum two_nums_type_match_result
two_nums_type_match_RB(
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    ss_data_set_real(p_ss_data_1, (F64) ss_data_get_integer(p_ss_data_1));
    ss_data_set_real(p_ss_data_2, (F64) ss_data_get_integer(p_ss_data_2));
    return(TWO_REALS);
}

/*ncr*/
extern enum two_nums_type_match_result
two_nums_type_match(
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2)
{
    const U32 did1 = MIN((U32) TN_OTHER, (U32) ss_data_get_data_id(p_ss_data_1)); /* collapse all higher (non-number and invalid) RPN values onto TN_OTHER */
    const U32 did2 = MIN((U32) TN_OTHER, (U32) ss_data_get_data_id(p_ss_data_2));

    CHECKING_ONLY(check_tn(did1, did2));

    switch(tn_no_worry[did2][did1])
    {
    case TN_I:
        return(TWO_INTEGERS);

    case TN_IW:
        return(TWO_INTEGERS_WORD32);

    case TN_RR:
        return(TWO_REALS);

    case TN_R1:
        return(two_nums_type_match_R1(p_ss_data_1, p_ss_data_2));

    case TN_R2:
        return(two_nums_type_match_R2(p_ss_data_1, p_ss_data_2));

    case TN_RB:
        return(two_nums_type_match_RB(p_ss_data_1, p_ss_data_2));

    default: default_unhandled();
#if CHECKING
    case TN_MIX:
#endif
        return(TWO_MIXED);
    }
}


/*ncr*/
extern enum two_nums_type_match_result
two_nums_type_match_maybe_worry(
    _InoutRef_  P_SS_DATA p_ss_data_1,
    _InoutRef_  P_SS_DATA p_ss_data_2,
    _InVal_     BOOL size_worry)
{
    const U32 did1 = MIN((U32) TN_OTHER, (U32) ss_data_get_data_id(p_ss_data_1)); /* collapse all higher (non-number and invalid) RPN values onto TN_OTHER */
    const U32 did2 = MIN((U32) TN_OTHER, (U32) ss_data_get_data_id(p_ss_data_2));
    const U8 (*p_tn_array)[5] = (size_worry) ? tn_worry : tn_no_worry;

    CHECKING_ONLY(check_tn(did1, did2));

    switch(p_tn_array[did2][did1])
    {
    case TN_I:
        return(TWO_INTEGERS);

    case TN_IW:
        assert(!size_worry);
        return(TWO_INTEGERS_WORD32);

    case TN_RR:
        return(TWO_REALS);

    case TN_R1:
        return(two_nums_type_match_R1(p_ss_data_1, p_ss_data_2));

    case TN_R2:
        return(two_nums_type_match_R2(p_ss_data_1, p_ss_data_2));

    case TN_RB:
        return(two_nums_type_match_RB(p_ss_data_1, p_ss_data_2));

    default: default_unhandled();
#if CHECKING
    case TN_MIX:
#endif
        return(TWO_MIXED);
    }
}

SS_DECOMPILER_OPTIONS
g_ss_decompiler_options =
{
    1,  /* lf */
    0,  /* cr */
    0,  /* initial_formula_equals */
    0,  /* range_colon_separator */
    0,  /* upper_case_function */
    0,  /* upper_case_slr */
    0   /* zero_args_function_parentheses */
};

SS_RECOG_CONTEXT
g_ss_recog_context =
{
    0,              /*ui_flag*/ /* 0 -> use only canonical representation for files */
    0,              /*alternate_function_flag*/
    CH_COMMA        /*function arg sep*/,
    CH_COMMA        /*array col sep*/,
    CH_SEMICOLON    /*array row sep*/,
    CH_COMMA        /*thousands char*/,
    CH_FULL_STOP    /*decimal point char*/,
    CH_COMMA        /*list sep char*/,
    CH_FULL_STOP    /*date sep char*/, /* stores dot in files */
    CH_NULL         /*alternate date sep char*/, /* only stores dot in files */
    CH_COLON        /*time sep char*/,
    0, 0, 0, 0, 0   /*spares*/
};

SS_RECOG_CONTEXT
g_ss_recog_context_alt =
{
    1,              /*ui_flag*/ /* 1 -> additional parsing (e.g. ISO dates) for stuff that user types in */
    0,              /*alternate_function_flag*/
    CH_COMMA        /*function arg sep*/,
    CH_COMMA        /*array col sep*/,
    CH_SEMICOLON    /*array row sep*/,
    CH_COMMA        /*thousands char*/,
    CH_FULL_STOP    /*decimal point char*/,
    CH_COMMA        /*list sep char*/,
    CH_FULL_STOP    /*date sep char*/, /* stores dot in files and is suggested in documentation */
    CH_FORWARDS_SLASH /*alternate date sep char*/, /* whereas slash is more typical of GB user */
    CH_COLON        /*time sep char*/,
    0, 0, 0, 0, 0   /*spares*/
};

extern void
ss_recog_context_pull(
    P_SS_RECOG_CONTEXT p_ss_recog_context /* const save area */)
{
    g_ss_recog_context = *p_ss_recog_context;
}

extern void
ss_recog_context_push(
    P_SS_RECOG_CONTEXT p_ss_recog_context /* save area, filled*/)
{
    *p_ss_recog_context = g_ss_recog_context;

    if(g_ss_recog_context_alt.function_arg_sep)
        g_ss_recog_context = g_ss_recog_context_alt; /* use alternate UI stuff iff set up */
}

_Check_return_
static F64
ui_strtod_harder(
    _In_z_      PC_USTR ustr,
    _Out_opt_   P_PC_USTR p_ustr)
{
    /* avoid poking source string like before! */
    P_USTR ustr_dp;
    UCHARZ ustr_buf[256];
    ustr_xstrkpy(ustr_bptr(ustr_buf), elemof32(ustr_buf), ustr);
    ustr_dp = ustrchr(ustr_bptr(ustr_buf), g_ss_recog_context.decimal_point_char);
    assert(NULL != ustr_dp);
    if(NULL != ustr_dp)
    {
        F64 f64;
        PtrPutByte(ustr_dp, CH_FULL_STOP);
        f64 = strtod((const char *) ustr_buf, (char **) p_ustr);
        if(NULL != p_ustr)
        {   /* adjust */
            U32 chars_read = PtrDiffBytesU32(*p_ustr, ustr_buf);
            *p_ustr = ustr_AddBytes(ustr, chars_read);
        }
        return(f64);
    }

    return(strtod((const char *) ustr, (char **) p_ustr));
}

_Check_return_
extern F64
ui_strtod(
    _In_z_      PC_USTR ustr,
    _Out_opt_   P_PC_USTR p_ustr)
{
    if( (CH_FULL_STOP == g_ss_recog_context.decimal_point_char) ||
        (NULL == ustrchr(ustr, g_ss_recog_context.decimal_point_char)) )
        return(strtod((const char *) ustr, (char **) p_ustr));

    return(ui_strtod_harder(ustr, p_ustr));
}

_Check_return_
extern S32
ui_strtol(
    _In_z_      PC_USTR ustr,
    _Out_opt_   P_P_USTR p_ustr,
    _In_        int radix)
{
    return((S32) strtol((const char *) ustr, (char **) p_ustr, radix));
}

/******************************************************************************
*
* add, subtract or multiply two 32-bit signed integers,
* checking for overflow and also returning
* a signed 64-bit result that the caller may consult
* e.g. to promote to fp
*
******************************************************************************/

_Check_return_
static inline int32_t
int32_from_int64_possible_overflow(
    _In_        const int64_t int64,
    _OutRef_    P_INT64_WITH_INT32_OVERFLOW p_int64_with_int32_overflow)
{
    p_int64_with_int32_overflow->int64_result = int64;

    /* if both the top word and the MSB of the low word of the result
     * are all zeros (+ve) or all ones (-ve) then
     * the result still fits in a 32-bit signed integer
     */

#if WINDOWS && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    /* try to stop Microsoft compiler generating a redundant generic shift by 32 call */
    if(false == (p_int64_with_int32_overflow->f_overflow = (
                (((const int32_t *) &int64)[1])  -  (((int32_t) int64) >> 31)
                ) ) )
    {
        return((int32_t) int64);
    }

    return((int64 < 0) ? INT32_MIN : INT32_MAX);
#elif RISCOS
    /* two instructions on ARM Norcroft C - SUBS r0, r0, r1 ASR #31; MOVNE r0, #1 */
    if(false == (p_int64_with_int32_overflow->f_overflow = (
                ((int32_t) (int64 >> 32))  -  (((int32_t) int64) >> 31)
                ) ) )
    {
        return((int32_t) int64);
    }
  
    /* just test sign bit of 64-bit result - single instruction TST r1 on ARM Norcroft C (compare does full subtraction) */
    return(((uint32_t) (int64 >> 32) & 0x80000000U) ? INT32_MIN : INT32_MAX);
#else
    /* portable version */
    if(false == (p_int64_with_int32_overflow->f_overflow = (
                ((int32_t) (int64 >> 32))  !=  (((int32_t) int64) >> 31)
                ) ) )
    {
        return((int32_t) int64);
    }

    return((int64 < 0) ? INT32_MIN : INT32_MAX);
#endif
}

_Check_return_
extern int32_t
int32_add_check_overflow(
    _In_        const int32_t addend_a,
    _In_        const int32_t addend_b,
    _OutRef_    P_INT64_WITH_INT32_OVERFLOW p_int64_with_int32_overflow)
{
    /* NB contorted order to save register juggling on ARM Norcroft C */
    const int64_t int64 = (int64_t) addend_b + addend_a;

    return(int32_from_int64_possible_overflow(int64, p_int64_with_int32_overflow));
}

_Check_return_
extern int32_t
int32_subtract_check_overflow(
    _In_        const int32_t minuend,
    _In_        const int32_t subtrahend,
    _OutRef_    P_INT64_WITH_INT32_OVERFLOW p_int64_with_int32_overflow)
{
    const int64_t int64 = (int64_t) minuend - subtrahend;

    return(int32_from_int64_possible_overflow(int64, p_int64_with_int32_overflow));
}

_Check_return_
extern int32_t
int32_multiply_check_overflow(
    _In_        const int32_t multiplicand_a,
    _In_        const int32_t multiplicand_b,
    _OutRef_    P_INT64_WITH_INT32_OVERFLOW p_int64_with_int32_overflow)
{
    /* NB contorted order to save register juggling on ARM Norcroft C */
    /* ARM Norcroft C should generate a call to _ll_mullss: "Create a 64-bit number by multiplying two int32_t numbers" */
    const int64_t int64 = (int64_t) multiplicand_b * multiplicand_a;

    return(int32_from_int64_possible_overflow(int64, p_int64_with_int32_overflow));
}

/******************************************************************************
*
* add, subtract or multiply two 32-bit unsigned integers,
* checking for overflow and also returning
* an unsigned 64-bit result that the caller may consult
* e.g. to promote to fp
*
******************************************************************************/

_Check_return_
static inline uint32_t
uint32_from_uint64_possible_overflow(
    _In_        const uint64_t uint64,
    _OutRef_    P_UINT64_WITH_UINT32_OVERFLOW p_uint64_with_uint32_overflow)
{
    p_uint64_with_uint32_overflow->uint64_result = uint64;

    /* if the top word is all zeros (+ve) then
     * the result still fits in a 32-bit unsigned integer
     */

    if(false == (p_uint64_with_uint32_overflow->f_overflow = (
                (0 != (uint64 >> 32))
                ) ) )
    {
        return((uint32_t) uint64);
    }

    return(UINT32_MAX);
}

_Check_return_
extern uint32_t
uint32_add_check_overflow(
    _In_        const uint32_t addend_a,
    _In_        const uint32_t addend_b,
    _OutRef_    P_UINT64_WITH_UINT32_OVERFLOW p_uint64_with_uint32_overflow)
{
    const uint64_t uint64 = (uint64_t) addend_b + addend_a;

    return(uint32_from_uint64_possible_overflow(uint64, p_uint64_with_uint32_overflow));
}

_Check_return_
extern uint32_t
uint32_subtract_check_overflow(
    _In_        const uint32_t minuend,
    _In_        const uint32_t subtrahend,
    _OutRef_    P_UINT64_WITH_UINT32_OVERFLOW p_uint64_with_uint32_overflow)
{
    const uint64_t uint64 = (uint64_t) minuend - subtrahend;

    return(uint32_from_uint64_possible_overflow(uint64, p_uint64_with_uint32_overflow));
}

_Check_return_
extern uint32_t
uint32_multiply_check_overflow(
    _In_        const uint32_t multiplicand_a,
    _In_        const uint32_t multiplicand_b,
    _OutRef_    P_UINT64_WITH_UINT32_OVERFLOW p_uint64_with_uint32_overflow)
{
    const uint64_t uint64 = (uint64_t) multiplicand_b * multiplicand_a;

    return(uint32_from_uint64_possible_overflow(uint64, p_uint64_with_uint32_overflow));
}

/* end of ss_const.c */
