/* ss_const.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

#define SECS_IN_24 ((S32) 60 * (S32) 60 * (S32) 24)

/******************************************************************************
*
* free an array - we have to check each element for strings which themselves need deallocating
*
******************************************************************************/

static void
array_free(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    S32 ix, iy;

    trace_2(TRACE_MODULE_EVAL, TEXT("ss_array_free x: ") S32_TFMT TEXT(", y: ") S32_TFMT, p_ev_data->arg.ev_array.x_size, p_ev_data->arg.ev_array.y_size);

    for(iy = 0; iy < p_ev_data->arg.ev_array.y_size; ++iy)
        for(ix = 0; ix < p_ev_data->arg.ev_array.x_size; ++ix)
            ss_data_free_resources(ss_array_element_index_wr(p_ev_data, ix, iy));

    al_array_dispose(&p_ev_data->arg.ev_array.elements);

    p_ev_data->local_data = 0;

    ev_data_set_blank(p_ev_data);
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
    _OutRef_    P_EV_DATA p_ev_data,
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

    ev_data_set_error(p_ev_data, err);
    p_ev_data->local_data = 1;

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
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR in_str)
{
    STATUS status;

    /* SKS 11apr95 moved to before ss_recog_number for foreign UI also for 1/2/96 recognition */
    if(0 != (status = ss_recog_date_time(p_ev_data, in_str)))
        return(status);

    if(0 != (status = ss_recog_number(p_ev_data, in_str)))
        return(status);

    if(0 != (status = ss_recog_string(p_ev_data, in_str)))
        return(status);

    if(0 != (status = ss_recog_boolean(p_ev_data, in_str)))
        return(status);

    return(recog_error(p_ev_data, in_str));
}

/******************************************************************************
*
* recognise a row of an array
*
******************************************************************************/

_Check_return_
static STATUS
recog_array_row(
    P_EV_DATA p_ev_data_out,
    _In_z_      PC_USTR in_str,
    _InoutRef_  P_S32 p_ix,
    _InoutRef_  P_S32 p_iy)
{
    PC_USTR in_pos = in_str;
    STATUS status = STATUS_OK;

    do  {
        S32 len;
        EV_DATA ev_data;

        ev_data_set_blank(&ev_data);

        /* skip separator */
        ustr_IncByte(in_pos);

        /* skip blanks */
        while(CH_SPACE == PtrGetByte(in_pos))
            ustr_IncByte(in_pos);

        if((len = recog_date_number_string_error(&ev_data, in_pos)) <= 0)
        {
            status = len;
            break;
        }

        ustr_IncBytes(in_pos, len);

        if(*p_iy)
        {
            /* can't expand array on subsequent rows */
            if(*p_ix >= p_ev_data_out->arg.ev_array.x_size)
            {
                status = 0;
                break;
            }
        }

        /* put this data into array */
        if(status_ok(ss_array_element_make(p_ev_data_out, *p_ix, *p_iy)))
            *ss_array_element_index_wr(p_ev_data_out, *p_ix, *p_iy) = ev_data;
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
    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR in_str)
{
    PC_USTR in_pos = in_str;
    S32 ix, iy;
    STATUS status = STATUS_OK;

    if(CH_LEFT_CURLY_BRACKET != PtrGetByte(in_pos))
        return(STATUS_OK);

    status_return(status = ss_array_make(p_ev_data, 0, 0));

    iy = 0;
    do  {
        ix = 0;
        if(status_done(status = recog_array_row(p_ev_data, in_pos, &ix, &iy))) /* SKS 22nov94 was status_ok so that {1;2,3} stuffed up big time */
        {
            ustr_IncBytes(in_pos, (U32) status);
            iy += 1;
        }
        else
        {
            ss_data_free_resources(p_ev_data);
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
ev_data_set_error(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     STATUS error)
{
    zero_struct_ptr(p_ev_data);
    p_ev_data->did_num = RPN_DAT_ERROR;
    p_ev_data->arg.ev_error.status = error;
    p_ev_data->arg.ev_error.type = ERROR_NORMAL;
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

#if 1

/* this one rounds at the given significant place before floor-ing */

_Check_return_
extern F64
real_floor(
    _InVal_     F64 f64)
{
    F64 floor_value;

    if(global_preferences.ss_calc_additional_rounding)
    {
        int exponent;
        F64 mantissa = frexp(f64, &exponent); /* yields mantissa in �[0.5,1.0) */
        const int mantissa_digits_minus_n = DBL_MANT_DIG - 3;
        const int exponent_minus_mdmn = exponent - mantissa_digits_minus_n;

        if(exponent >= 0) /* no need to do more for negative exponents here */
        {
            const F64 rounding_value = copysign(pow(2.0, exponent_minus_mdmn), mantissa);
            const F64 adjusted_value = f64 + rounding_value;

            /* adjusted result */
            floor_value = floor(adjusted_value);
            return(floor_value);
        }
    }

    /* standard result */
    floor_value = floor(f64);
    return(floor_value);
}

#else

/* this one rounds at the given number of decimals before floor-ing which is not the same as at the significant place */

_Check_return_
extern F64
real_floor(
    _InVal_     F64 f64)
{
    /* first do the naive desired step */
    F64 floor_value = floor(f64);

    if(global_preferences.ss_calc_additional_rounding && (floor_value != f64))
    {   /* if not already an integer, and allowed, then do the more expensive bit */
        F64 trunc_value;
        F64 fractional_part = modf(f64, &trunc_value);

        if(fabs(fractional_part) > (+1.0 - 1E-14))
        {   /* close enough to an integer */
            trunc_value += copysign(1.0, trunc_value);

            /* adjusted result */
            floor_value = trunc_value;
        }
    }

    return(floor_value);
}

#endif

/******************************************************************************
*
* trunc() of real with possible ickle rounding
*
* SKS 06oct97 for INT() function try rounding an ickle bit
* so INT((0.06-0.04)/0.01) is 2 not 1
* and now dec14 INT((0.06-0.02)/1E-6) is 20000 not 19999
* which is different to naive real_trunc()
*
******************************************************************************/

#if 1

/* this one rounds at the given significant place before truncating */

_Check_return_
extern F64
real_trunc(
    _InVal_     F64 f64)
{
    F64 trunc_value;

    if(global_preferences.ss_calc_additional_rounding)
    {
        int exponent;
        F64 mantissa = frexp(f64, &exponent); /* yields mantissa in �[0.5,1.0) */
        const int mantissa_digits_minus_n = DBL_MANT_DIG - 3;
        const int exponent_minus_mdmn = exponent - mantissa_digits_minus_n;

        if(exponent >= 0) /* no need to do more for negative exponents here */
        {
            const F64 rounding_value = copysign(pow(2.0, exponent_minus_mdmn), mantissa);
            const F64 adjusted_value = f64 + rounding_value;

            /* adjusted result */
            (void) modf(adjusted_value, &trunc_value);
            return(trunc_value);
        }
    }

    /* standard result */
    (void) modf(f64, &trunc_value);
    return(trunc_value);
}

#else

/* this one rounds at the given number of decimals before truncating which is not the same as at the significant place */

_Check_return_
extern F64
real_trunc(
    _InVal_     F64 f64)
{
    /* first do the naive desired step */
    F64 trunc_value;
    F64 fractional_part = modf(f64, &trunc_value);

    if(global_preferences.ss_calc_additional_rounding && (trunc_value != f64))
    {   /* if not already an integer, and allowed, then do the more expensive bit */
        if(fabs(fractional_part) > (+1.0 - 1E-14))
        {   /* close enough to an integer */
            trunc_value += copysign(1.0, trunc_value);
            /* adjusted result */
        }
    }

    return(trunc_value);
}

#endif

_Check_return_
static inline F64
real_adjust_if_near_power_of_2(
    _InVal_     F64 f64_in,
    _OutRef_    P_BOOL p_adjusted)
{
    int exponent;
    F64 mantissa = frexp(f64_in, &exponent); /* yields mantissa in �[0.5,1.0) */

    if(fabs(mantissa) > (+1.0 - 1E-14))
    {
        F64 f64;

        *p_adjusted = TRUE;

        f64 = ldexp(copysign(1.0, mantissa), exponent);

        return(f64);
    }

    *p_adjusted = FALSE;

    return(f64_in);
}

/******************************************************************************
*
* force conversion of real to integer
*
* NB real_to_integer_force uses real_floor() NOT real_trunc()
*
******************************************************************************/

_Check_return_
extern STATUS /* DONE iff converted in range */
real_to_integer_force(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    F64 floor_value;
    S32 s32;

    assert(RPN_DAT_REAL == p_ev_data->did_num);
    if(RPN_DAT_REAL != p_ev_data->did_num)
        return(STATUS_OK); /* no conversion */

    if(p_ev_data->arg.fp > (F64) S32_MAX)
    {
        ev_data_set_integer(p_ev_data, S32_MAX);
        return(STATUS_OK); /* out of range */
    }
    else if(p_ev_data->arg.fp < (F64) -S32_MAX) /* NB NOT S32_MIN */
    {
        ev_data_set_integer(p_ev_data, -S32_MAX);
        return(STATUS_OK); /* out of range */
    }
    else
    {
        floor_value = real_floor(p_ev_data->arg.fp);

        s32 = (S32) floor_value;

        if(s32 == S32_MIN)
        {
            ev_data_set_integer(p_ev_data, -S32_MAX);
            return(STATUS_OK); /* out of range */
        }
    }

    ev_data_set_integer(p_ev_data, s32);
    return(STATUS_DONE); /* converted OK */
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
extern BOOL
real_to_integer_try(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    F64 f64;
    F64 floor_value;
    S32 s32;

    if(RPN_DAT_REAL != p_ev_data->did_num)
        return(FALSE);

    f64 = p_ev_data->arg.fp;

    /* first do the cheap step to see if we're already at an integer */
    floor_value = floor(f64);

    if(floor_value == f64)
    {
        if( (floor_value > (F64)  S32_MAX) ||
            (floor_value < (F64) -S32_MAX) ) /* NB NOT S32_MIN */
            return(FALSE);

        s32 = (S32) floor_value;
        ev_data_set_integer(p_ev_data, s32);
        return(TRUE);
    }

    if(global_preferences.ss_calc_additional_rounding)
    {   /* if not already an integer, and allowed, then do the more expensive bit */
#if 1
        int exponent;
        F64 mantissa = frexp(f64, &exponent); /* yields mantissa in �[0.5,1.0) */
        const int mantissa_digits_minus_n = DBL_MANT_DIG - 3;
        const int exponent_minus_mdmn = exponent - mantissa_digits_minus_n;

        if(exponent >= 0) /* no need to do more for negative exponents here */
        {
            const F64 rounding_value = copysign(pow(2.0, exponent_minus_mdmn), mantissa);
            const F64 adjusted_value = f64 + rounding_value;

            floor_value = floor(adjusted_value);

            if(floor_value == adjusted_value)
            {
                if( (floor_value > (F64)  S32_MAX) ||
                    (floor_value < (F64) -S32_MAX) ) /* NB NOT S32_MIN */
                {   /* won't fit in S32 but we should hang on to this adjusted value */
                    p_ev_data->arg.fp = adjusted_value;
                    return(FALSE);
                }

                s32 = (S32) floor_value;
                ev_data_set_integer(p_ev_data, s32);
                return(TRUE); /* converted OK */
            }
        }
#else
        F64 trunc_value;
        F64 fractional_part = modf(f64, &trunc_value);

        if(fabs(fractional_part) > (+1.0 - 1E-14))
        {   /* close enough to an integer */
            trunc_value += copysign(1.0, trunc_value);

            if( (trunc_value > (F64)  S32_MAX) ||
                (trunc_value < (F64) -S32_MAX) ) /* NB NOT S32_MIN */
            {   /* won't fit in S32 but we should hang on to this adjusted value */
                p_ev_data->arg.fp = trunc_value;
                return(FALSE);
            }

            s32 = (S32) trunc_value;
            ev_data_set_integer(p_ev_data, s32);
            return(TRUE); /* converted OK */
        }
#endif
    }

    return(FALSE); /* unmodified */
}

/******************************************************************************
*
* make a duplicate of an array
*
******************************************************************************/

_Check_return_
extern STATUS
ss_array_dup(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in)
{
    S32 x_size = p_ev_data_in->arg.ev_array.x_size;
    S32 y_size = p_ev_data_in->arg.ev_array.y_size;

    assert(RPN_DAT_ARRAY == p_ev_data_in->did_num);

    status_return(ss_array_make(p_ev_data_out, x_size, y_size));

    {
    S32 iy;

    for(iy = 0; iy < y_size; ++iy)
    {
        S32 ix;

        for(ix = 0; ix < x_size; ++ix)
        {
            STATUS status = ss_data_resource_copy(ss_array_element_index_wr(p_ev_data_out, ix, iy),
                                                  ss_array_element_index_borrow(p_ev_data_in, ix, iy));

            if(status_fail(status))
            {
                ss_data_free_resources(p_ev_data_out);
                ev_data_set_error(p_ev_data_out, status);
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
extern P_EV_DATA
ss_array_element_index_wr(
    _InRef_     P_EV_DATA p_ev_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    const S32 element = (iy * p_ev_data_in->arg.ev_array.x_size) + ix;

    assert(RPN_DAT_ARRAY == p_ev_data_in->did_num);
    assert(p_ev_data_in->local_data == 1);
    assert(((U32) ix < (U32) p_ev_data_in->arg.ev_array.x_size) && ((U32) iy < (U32) p_ev_data_in->arg.ev_array.y_size));

    return(array_ptr(&p_ev_data_in->arg.ev_array.elements, EV_DATA, element));
}

/******************************************************************************
*
* return pointer to array element data
* array doesn't need to be local
*
******************************************************************************/

_Check_return_
_Ret_valid_
extern PC_EV_DATA
ss_array_element_index_borrow(
    _InRef_     PC_EV_DATA p_ev_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    const S32 element = (iy * p_ev_data_in->arg.ev_array.x_size) + ix;

    assert(RPN_DAT_ARRAY == p_ev_data_in->did_num);
    assert(((U32) ix < (U32) p_ev_data_in->arg.ev_array.x_size) && ((U32) iy < (U32) p_ev_data_in->arg.ev_array.y_size));

    return(array_ptrc(&p_ev_data_in->arg.ev_array.elements, EV_DATA, element));
}

/******************************************************************************
*
* ensure that a given array element exists
*
******************************************************************************/

_Check_return_
extern STATUS
ss_array_element_make(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(EV_DATA), TRUE);
    const S32 old_xs = p_ev_data->arg.ev_array.x_size;
    const S32 old_ys = p_ev_data->arg.ev_array.y_size;
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

    if(NULL == al_array_extend_by(&p_ev_data->arg.ev_array.elements, EV_DATA, (new_size - old_size), &array_init_block, &status))
        return(status);

    trace_2(TRACE_MODULE_EVAL,
            TEXT("array realloced, now: ") S32_TFMT TEXT(" entries, ") S32_TFMT TEXT(" bytes"),
            new_xs * new_ys,
            new_size * sizeof32(EV_DATA));

    { /* set all new array elements to blank */
    P_EV_DATA p_ev_data_s, p_ev_data_e, p_ev_data_t;

    /* use old stored sizes to get pointer to end of old data */
    if(old_xs < 1 || old_ys < 1)
        p_ev_data_s = array_base(&p_ev_data->arg.ev_array.elements, EV_DATA);
    else
        p_ev_data_s = ss_array_element_index_wr(p_ev_data, old_xs - 1, old_ys - 1) + 1;

    /* set up new sizes */
    p_ev_data->arg.ev_array.x_size = new_xs;
    p_ev_data->arg.ev_array.y_size = new_ys;

    /* use new stored size to get pointer to end of new array */
    p_ev_data_e = ss_array_element_index_wr(p_ev_data, new_xs - 1, new_ys - 1);

    for(p_ev_data_t = p_ev_data_s; p_ev_data_t <= p_ev_data_e; ++p_ev_data_t)
        ev_data_set_blank(p_ev_data_t);

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
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_DATA p_ev_data_src,
    _InVal_     S32 ix,
    _InVal_     S32 iy)
{
    *p_ev_data = *ss_array_element_index_borrow(p_ev_data_src, ix, iy);
    p_ev_data->local_data = 0;
}

/******************************************************************************
*
* make an array of a given size
*
******************************************************************************/

_Check_return_
extern STATUS
ss_array_make(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     S32 x_size,
    _InVal_     S32 y_size)
{
    STATUS status = STATUS_OK;

    p_ev_data->did_num = RPN_DAT_ARRAY;
    p_ev_data->local_data = 1;
    p_ev_data->arg.ev_array.x_size = 0;
    p_ev_data->arg.ev_array.y_size = 0;
    p_ev_data->arg.ev_array.elements = 0;

    if(x_size && y_size)
        if(status_fail(status = ss_array_element_make(p_ev_data, x_size - 1, y_size - 1)))
            ss_data_free_resources(p_ev_data);

    if(status_fail(status))
        ev_data_set_error(p_ev_data, status);

    return(status);
}

/******************************************************************************
*
* copies input data item to out, setting equal type numbers for equivalent types
*
******************************************************************************/

static void
type_equate(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in,
    _InVal_     BOOL blanks_equal_zero)
{
    *p_ev_data_out = *p_ev_data_in;

    p_ev_data_out->local_data = 0;

    switch(p_ev_data_out->did_num)
    {
    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        p_ev_data_out->did_num = RPN_DAT_WORD32; /* NB all integers returned from type_equate() are promoted to widest type */
        break;

    case RPN_DAT_STRING:
        if(ss_string_is_blank(p_ev_data_out))
        {
            if(blanks_equal_zero)
                ev_data_set_WORD32(p_ev_data_out, 0);
            else
                ev_data_set_blank(p_ev_data_out);
        }
        break;

    case RPN_DAT_BLANK:
        if(blanks_equal_zero)
            ev_data_set_WORD32(p_ev_data_out, 0);
        break;
    }
}

/******************************************************************************
*
* compare two results
*
* NB string compare return -ve, 0, +ve not -1, 0, +1
*
* --out--
* <0 p_ev_data1 < p_ev_data2
* =0 p_ev_data1 = p_ev_data2
* >0 p_ev_data1 > p_ev_data2
*
******************************************************************************/

_Check_return_
extern S32
ss_data_compare(
    _InRef_     PC_EV_DATA p_ev_data1,
    _InRef_     PC_EV_DATA p_ev_data2,
    _InVal_     BOOL blanks_equal_zero,
    _InVal_     BOOL allow_wild_match)
{
    EV_DATA ev_data1;
    EV_DATA ev_data2;
    S32 res = 0;

    /* eliminate equivalent types */
    type_equate(&ev_data1, p_ev_data1, blanks_equal_zero);
    type_equate(&ev_data2, p_ev_data2, blanks_equal_zero);

    consume(S32, two_nums_type_match(&ev_data1, &ev_data2, FALSE));

    if(ev_data1.did_num != ev_data2.did_num)
    {
        if(ev_data1.did_num < ev_data2.did_num)
            res = -1;
        else
            res = 1;
    }
    else
    {
        switch(ev_data1.did_num)
        {
        case RPN_DAT_REAL:
            if(ev_data1.arg.fp == ev_data2.arg.fp)
                res = 0;
            else if(ev_data1.arg.fp < ev_data2.arg.fp)
                res = -1;
            else
                res = 1;
            break;

        case RPN_DAT_BOOL8:
        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            if(ev_data1.arg.integer == ev_data2.arg.integer)
                res = 0;
            else if(ev_data1.arg.integer < ev_data2.arg.integer)
                res = -1;
            else
                res = 1;
            break;

        case RPN_DAT_STRING:
            if(allow_wild_match)
                res = uchars_compare_t5_nocase_wild(ev_data1.arg.string.uchars, ev_data1.arg.string.size, ev_data2.arg.string.uchars, ev_data2.arg.string.size);
            else
                res = uchars_compare_t5_nocase(ev_data1.arg.string.uchars, ev_data1.arg.string.size, ev_data2.arg.string.uchars, ev_data2.arg.string.size);
            if(res < 0)
                res = -1;
            else if(res > 0)
                res = 1;
            break;

        case RPN_DAT_ARRAY:
            {
            if(     ev_data1.arg.ev_array.y_size < ev_data2.arg.ev_array.y_size)
                res = -1;
            else if(ev_data1.arg.ev_array.y_size > ev_data2.arg.ev_array.y_size)
                res = 1;
            else if(ev_data1.arg.ev_array.x_size < ev_data2.arg.ev_array.x_size)
                res = -1;
            else if(ev_data1.arg.ev_array.x_size > ev_data2.arg.ev_array.x_size)
                res = 1;
            else /* same sizes */
            {
                S32 ix, iy;
                for(iy = 0; !res && iy < ev_data1.arg.ev_array.y_size; ++iy)
                    for(ix = 0; !res && ix < ev_data1.arg.ev_array.x_size; ++ix)
                        res = ss_data_compare(ss_array_element_index_borrow(&ev_data1, ix, iy),
                                              ss_array_element_index_borrow(&ev_data2, ix, iy),
                                              blanks_equal_zero, allow_wild_match);
            }
            break;
            }

        case RPN_DAT_DATE:
            if(ev_data1.arg.ev_date.date == ev_data2.arg.ev_date.date)
            {   /* both dates valid or invalid but equal - compare times */
                /* here 31/12/2015 00:00:00 == 31/12/2015 */
                EV_DATE_TIME time_1 = (EV_TIME_NULL != ev_data1.arg.ev_date.time) ? ev_data1.arg.ev_date.time : 0;
                EV_DATE_TIME time_2 = (EV_TIME_NULL != ev_data2.arg.ev_date.time) ? ev_data2.arg.ev_date.time : 0;
                if(time_1 == time_2)
                    res = 0;
                else if(time_1 < time_2)
                    res = -1;
                else
                    res = 1;
            }
            else if(EV_DATE_NULL == ev_data2.arg.ev_date.date)
                res = -1; /* date 1 valid, date 2 invalid */
            else if(EV_DATE_NULL == ev_data1.arg.ev_date.date)
                res = 1; /* date 1 valid, date 2 invalid */
            else if(ev_data1.arg.ev_date.date < ev_data2.arg.ev_date.date) /* both dates valid */
                res = -1;
            else
                res = 1;
            break;

        case RPN_DAT_ERROR:
            if(ev_data1.arg.ev_error.status == ev_data2.arg.ev_error.status)
                res = 0;
            else if(-(ev_data1.arg.ev_error.status) < -(ev_data2.arg.ev_error.status))
                res = -1;
            else
                res = 1;
            break;

        case RPN_DAT_BLANK:
            res = 0;
            break;
        }
    }

    return(res);
}

/******************************************************************************
*
* free resources owned by a data item
*
******************************************************************************/

extern void
ss_data_free_resources(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    if(!p_ev_data->local_data)
        return;

    switch(p_ev_data->did_num)
    {
    case RPN_DAT_STRING:
        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_ev_data->arg.string_wr.uchars));
        ev_data_set_blank(p_ev_data);
        break;

    case RPN_DAT_ARRAY:
        array_free(p_ev_data);
        break;
    }

    /* SKS 05oct95 it's important for the above array_free that this is still set! */
    p_ev_data->local_data = 0;
}

/******************************************************************************
*
* given a data item, make a copy of
* it so we can be sure we own it
*
******************************************************************************/

_Check_return_
extern STATUS
ss_data_resource_copy(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in)
{
    /* we get our own copy of handle based resources */
    switch(p_ev_data_in->did_num)
    {
    case RPN_DAT_STRING:
        return(ss_string_dup(p_ev_data_out, p_ev_data_in));

    case RPN_DAT_ARRAY:
        return(ss_array_dup(p_ev_data_out, p_ev_data_in));

    default:
        *p_ev_data_out = *p_ev_data_in;
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* decode data item to text
*
* NOTE: CH_NULL not added to output
*
******************************************************************************/

_Check_return_
extern STATUS
ss_decode_constant(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_EV_DATA p_ev_data)
{
    STATUS status = STATUS_OK;

    switch(p_ev_data->did_num)
    {
    case RPN_DAT_REAL:
        status = decode_fp(p_quick_ublock, p_ev_data->arg.fp);
        break;

    case RPN_DAT_BOOL8:
        status = quick_ublock_ustr_add(p_quick_ublock, (p_ev_data->arg.integer != 0) ? USTR_TEXT("TRUE") : USTR_TEXT("FALSE"));
        break;

    default: default_unhandled();
#if CHECKING
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
#endif
        status = quick_ublock_printf(p_quick_ublock, USTR_TEXT(S32_FMT), p_ev_data->arg.integer);
        break;

    case RPN_DAT_STRING:
        {
        PC_UCHARS uchars = p_ev_data->arg.string.uchars;

        status = quick_ublock_a7char_add(p_quick_ublock, CH_QUOTATION_MARK);

        if(status_ok(status))
        {
            const U32 len = p_ev_data->arg.string.size;

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
        break;
        }

    case RPN_DAT_ARRAY:
        {
        if(status_ok(status = quick_ublock_a7char_add(p_quick_ublock, CH_LEFT_CURLY_BRACKET)))
        {
            S32 iy;

            for(iy = 0; iy < p_ev_data->arg.ev_array.y_size; ++iy)
            {
                S32 ix;

                if(iy)
                    status_break(status = quick_ublock_ucs4_add(p_quick_ublock, g_ss_recog_context.array_row_sep));

                for(ix = 0; ix < p_ev_data->arg.ev_array.x_size; ++ix)
                {
                    EV_DATA ev_data;
                    if(ix)
                        status_break(status = quick_ublock_ucs4_add(p_quick_ublock, g_ss_recog_context.array_col_sep));
                    ss_array_element_read(&ev_data, p_ev_data, ix, iy);
                    status_break(status = ss_decode_constant(p_quick_ublock, &ev_data));
                    ss_data_free_resources(&ev_data);
                }

                status_break(status);
            }

            if(status_ok(status))
                status = quick_ublock_a7char_add(p_quick_ublock, CH_RIGHT_CURLY_BRACKET);
        }

        break;
        }

    case RPN_DAT_DATE:
        status = ss_date_decode(p_quick_ublock, &p_ev_data->arg.ev_date);
        break;

    case RPN_DAT_BLANK:
        break;

    case RPN_DAT_ERROR:
        status = quick_ublock_printf(p_quick_ublock, USTR_TEXT("#" S32_FMT), -(p_ev_data->arg.ev_error.status));
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
ss_recog_boolean(
    _OutRef_    P_EV_DATA p_ev_data,
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
            ev_data_set_boolean(p_ev_data, FALSE);
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
            ev_data_set_boolean(p_ev_data, TRUE);
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
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR in_str)
{
    STATUS status = STATUS_OK;
    PC_USTR in_pos = in_str;
    S32 len;

    ev_data_set_blank(p_ev_data);

    ustr_SkipSpaces(in_pos);

    /* several callers will call us with raw input e.g. =1.2 */
    if(CH_EQUALS_SIGN == PtrGetByte(in_pos))
        ustr_IncByte(in_pos);

    if(CH_NULL == PtrGetByte(in_pos)) /* SKS 19apr95 check for early exit */
        return(STATUS_OK);

    if(((len = recog_date_number_string_error(p_ev_data, in_pos)) > 0)
       ||
       ((len = recog_array(p_ev_data, in_pos)) > 0)
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
            ss_data_free_resources(p_ev_data);
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
    _OutRef_    P_EV_DATA p_ev_data,
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
                ev_data_set_integer(p_ev_data, (negative ? - (S32) u32 : (S32) u32));
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
            ev_data_set_real(p_ev_data, negative ? -f64 : f64);
            /*real_to_integer_try(p_ev_data);*//*SKS for 1.30 why the hell did it bother? means you can't preserve 1.0 typed in */
            res = PtrDiffBytesS32(epos, in_str_in);
        }
    }

    if(res)
        p_ev_data->local_data = 1;

    return(res);
}

/******************************************************************************
*
* read a string
*
* <0 error
* =0 no string found
* >0 #chars read
*
******************************************************************************/

_Check_return_ _Success_(return > 0)
extern STATUS
ss_recog_string(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR in_str)
{
    STATUS status = STATUS_OK;

    /* check for valid start character */
    if(PtrGetByte(in_str) != CH_QUOTATION_MARK)
        return(STATUS_OK);

    {
    S32 len = 0;
    PC_U8Z ci = (PC_U8Z) in_str + 1;

    while(CH_NULL != *ci)
    {
        if(CH_QUOTATION_MARK == *ci)
        {
            ci += 1;

            if(*ci != CH_QUOTATION_MARK)
                break;
        }

        ci += 1;
        len += 1;
    }

    /* allocate memory for string */
    if(0 != len)
    {
        if(NULL == (p_ev_data->arg.string_wr.uchars = al_ptr_alloc_bytes(P_UCHARS, len, &status)))
            return(status);
        p_ev_data->local_data = 1;
    }
    else
    {
        p_ev_data->arg.string.uchars = uchars_empty_string;
        p_ev_data->local_data = 0;
    }

    p_ev_data->did_num = RPN_DAT_STRING;
    p_ev_data->arg.string_wr.size = len;
    } /*block*/

    {
    P_U8 co = (P_U8) p_ev_data->arg.string_wr.uchars;
    PC_U8Z ci = (PC_U8Z) in_str + 1;

    while(CH_NULL != *ci)
    {
        if(CH_QUOTATION_MARK == *ci)
        {
            ci += 1;

            if(*ci != CH_QUOTATION_MARK)
                break;
        }

        *co++ = *ci;
        ci += 1;
    }

    return(PtrDiffBytesS32(ci, in_str));
    } /*block*/
}

/******************************************************************************
*
* is a string blank ?
*
******************************************************************************/

_Check_return_
extern BOOL
ss_string_is_blank(
    _InRef_     PC_EV_DATA p_ev_data)
{
    assert(RPN_DAT_STRING == p_ev_data->did_num);

    if(0 == p_ev_data->arg.string.size)
        return(TRUE);

    PTR_ASSERT(p_ev_data->arg.string.uchars);

    return(p_ev_data->arg.string.size == ss_string_skip_leading_whitespace(p_ev_data));
}

/******************************************************************************
*
* duplicate a string
*
******************************************************************************/

_Check_return_
extern STATUS
ss_string_dup(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_src)
{
    PTR_ASSERT(p_ev_data_src);
    assert(RPN_DAT_STRING == p_ev_data_src->did_num);
    PTR_ASSERT(p_ev_data_out);

    return(ss_string_make_uchars(p_ev_data_out, p_ev_data_src->arg.string.uchars, p_ev_data_src->arg.string.size));
}

/******************************************************************************
*
* make a string
*
******************************************************************************/

_Check_return_
extern STATUS
ss_string_make_uchars(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_reads_opt_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n)
{
    const U32 actual_len = uchars_n;

    PTR_ASSERT(p_ev_data);
    assert((S32) uchars_n >= 0);

    if(0 != actual_len)
    {
        STATUS status;
        if(NULL == (p_ev_data->arg.string_wr.uchars = al_ptr_alloc_bytes(P_UCHARS, actual_len, &status)))
        {
            ev_data_set_error(p_ev_data, status);
            return(status);
        }
        assert(actual_len <= EV_MAX_STRING_LEN); /* sometimes we get copied willy-nilly into buffers! */
        if(NULL == uchars)
            PtrPutByte(p_ev_data->arg.string_wr.uchars, CH_NULL); /* allows append (like ustr_set_n()) */
        else
            memcpy32(p_ev_data->arg.string_wr.uchars, uchars, actual_len);
        p_ev_data->local_data = 1;
    }
    else
    {
        p_ev_data->arg.string.uchars = uchars_empty_string;
        p_ev_data->local_data = 0;
    }

    p_ev_data->did_num = RPN_DAT_STRING;
    p_ev_data->arg.string_wr.size = actual_len;

    return(STATUS_OK);
}

_Check_return_
extern STATUS
ss_string_make_ustr(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR ustr)
{
    return(ss_string_make_uchars(p_ev_data, ustr, ustrlen32(ustr)));
}

_Check_return_
extern U32
ss_string_skip_leading_whitespace(
    _InRef_     PC_EV_DATA p_ev_data)
{
    assert(RPN_DAT_STRING == p_ev_data->did_num);

    return(str_hlp_skip_leading_whitespace(p_ev_data->arg.string.uchars, p_ev_data->arg.string.size));
}

_Check_return_
extern U32
str_hlp_skip_leading_whitespace(
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InRef_     U32 uchars_n)
{
    U32 wss = 0;

    assert((0 == uchars_n) || (!IS_PTR_NULL_OR_NONE(uchars)));

    while(wss < uchars_n)
    {
        switch(PtrGetByteOff(uchars, wss))
        {
        case CH_TAB:
        case LF:
        case CR:
        case CH_SPACE:
            wss++;
            continue;

        default:
            break;
        }

        break;
    }

    return(wss);
}

/******************************************************************************
*
* given two numbers, reals or integers of various
* sizes, convert them so they are both compatible
*
******************************************************************************/

enum two_num_action
{
    TN_NOP,
    TN_R1, /* convert ev_data1 to REAL */
    TN_R2, /* convert ev_data2 to REAL */
    TN_RB, /* convert both ev_data1 and ev_data2 to REAL */
    TN_I,
    TN_MIX
};

enum two_num_index
{
    TN_REAL,
    TN_BOOL8,
    TN_WORD8,
    TN_WORD16,
    TN_WORD32,
    TN_OTHER
};

static const U8
tn_worry[6][6] =
{   /*   REA     B8      W8      W16     W32     OTH */
    { TN_NOP, TN_R1,  TN_R1,  TN_R1,  TN_R1,  TN_MIX }, /*REA*/
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_RB,  TN_MIX }, /*B8*/
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_RB,  TN_MIX }, /*W8*/
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_RB,  TN_MIX }, /*W16*/
    { TN_R2,  TN_RB,  TN_RB,  TN_RB,  TN_RB,  TN_MIX }, /*W32*/
    { TN_MIX, TN_MIX, TN_MIX, TN_MIX, TN_MIX, TN_MIX }, /*OTH*/
};

static const U8
tn_no_worry[6][6] =
{   /*   REA     B8      W8      W16     W32     OTH */
    { TN_NOP, TN_R1,  TN_R1,  TN_R1,  TN_R1,  TN_MIX }, /*REA*/
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_I,   TN_MIX }, /*B8*/
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_I,   TN_MIX }, /*W8*/
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_I,   TN_MIX }, /*W16*/
    { TN_R2,  TN_I,   TN_I,   TN_I,   TN_I,   TN_MIX }, /*W32*/
    { TN_MIX, TN_MIX, TN_MIX, TN_MIX, TN_MIX, TN_MIX }, /*OTH*/
};

#if CHECKING

static void
check_tn(
    _InVal_     S32 did1,
    _InVal_     S32 did2)
{
    assert(RPN_DAT_REAL == TN_REAL);
    assert(RPN_DAT_BOOL8 == TN_BOOL8);
    assert(RPN_DAT_WORD8 == TN_WORD8);
    assert(RPN_DAT_WORD16 == TN_WORD16);
    assert(RPN_DAT_WORD32 == TN_WORD32);
    assert((did1 >= TN_REAL) && ((U32) did1 <= (U32) TN_OTHER));
    assert((did2 >= TN_REAL) && ((U32) did2 <= (U32) TN_OTHER));
}

#endif /* CHECKING */

/*ncr*/
extern S32
two_nums_type_match(
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2,
    _InVal_     BOOL size_worry)
{
    const U32 did1 = MIN((U32) TN_OTHER, (U32) p_ev_data1->did_num); /* collapse all higher (non-number and invalid) RPN values onto TN_OTHER */
    const U32 did2 = MIN((U32) TN_OTHER, (U32) p_ev_data2->did_num);
    const U8 (*p_tn_array)[6] = (size_worry) ? tn_worry : tn_no_worry;

    CHECKING_ONLY(check_tn(did1, did2));

    switch(p_tn_array[did2][did1])
    {
    case TN_NOP:
        return(TWO_REALS);

    case TN_R1:
        ev_data_set_real(p_ev_data1, (F64) p_ev_data1->arg.integer);
        return(TWO_REALS);

    case TN_R2:
        ev_data_set_real(p_ev_data2, (F64) p_ev_data2->arg.integer);
        return(TWO_REALS);

    case TN_RB:
        ev_data_set_real(p_ev_data1, (F64) p_ev_data1->arg.integer);
        ev_data_set_real(p_ev_data2, (F64) p_ev_data2->arg.integer);
        return(TWO_REALS);

    case TN_I:
        return(TWO_INTS);

    default:
    case TN_MIX:
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
extern F64
ui_strtod(
    _In_z_      PC_USTR ustr,
    _Out_opt_   P_PC_USTR p_ustr)
{
    if(CH_FULL_STOP == g_ss_recog_context.decimal_point_char)
        return(strtod((const char *) ustr, (char **) p_ustr));

    if(NULL != ustrchr(ustr, g_ss_recog_context.decimal_point_char))
    {   /* avoid poking source string like before! */
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
    }

    return(strtod((const char *) ustr, (char **) p_ustr));
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

const EV_DATA
ev_data_real_zero =
{ RPN_DAT_REAL, 0, 0, { 0.0 } };

/* end of ss_const.c */
