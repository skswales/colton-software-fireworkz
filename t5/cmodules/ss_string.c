/* ss_string.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Fireworkz string constant handling */

#include "common/gflags.h"

/******************************************************************************
*
* allocate space for a string
*
* NB MUST cater for zero-length strings
*
******************************************************************************/

_Check_return_
extern STATUS
ss_string_allocate(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     U32 len)
{
    //RETURN_STATUS_CHECK_IF_PTR_NONE_OR_NULL(p_ss_data);

    if(0 == len)
    {
        p_ss_data->arg.string.uchars = (PC_UCHARS) ustr_empty_string;

        p_ss_data->local_data = 0;
    }
    else
    {
        STATUS status;

        if(NULL == (p_ss_data->arg.string.uchars = al_ptr_alloc_bytes(P_UCHARS, len, &status)))
            return(ss_data_set_error(p_ss_data, status));

        p_ss_data->local_data = 1;
    }

    ss_data_set_data_id(p_ss_data, DATA_ID_STRING);
    p_ss_data->arg.string_wr.size = len;

    return(STATUS_OK);
}

/******************************************************************************
*
* duplicate a string
*
******************************************************************************/

_Check_return_
extern STATUS
ss_string_dup(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA p_ss_data_src)
{
    PTR_ASSERT(p_ss_data_src);
    assert(ss_data_is_string(p_ss_data_src));
    PTR_ASSERT(p_ss_data_out);

    return(ss_string_make_uchars(p_ss_data_out, ss_data_get_string(p_ss_data_src), ss_data_get_string_size(p_ss_data_src)));
}

/******************************************************************************
*
* make a string
*
******************************************************************************/

_Check_return_
extern STATUS
ss_string_make_uchars(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_reads_opt_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n)
{
    PTR_ASSERT(p_ss_data);
    assert((S32) uchars_n >= 0);
    assert(uchars_n <= EV_MAX_STRING_LEN); /* sometimes we get copied willy-nilly into buffers! */

    status_return(ss_string_allocate(p_ss_data, uchars_n)); /* NB caters for zero-length strings */

    if(0 != uchars_n)
    {
        if(NULL == uchars)
            PtrPutByte(p_ss_data->arg.string_wr.uchars, CH_NULL); /* allows append (like ustr_set_n()) */
        else
            memcpy32(p_ss_data->arg.string_wr.uchars, uchars, uchars_n);
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
ss_string_make_ustr(
    _OutRef_    P_SS_DATA p_ss_data,
    _In_z_      PC_USTR ustr)
{
    return(ss_string_make_uchars(p_ss_data, ustr, ustrlen32(ustr)));
}

/******************************************************************************
*
* is a string blank ?
*
******************************************************************************/

_Check_return_
extern BOOL
ss_string_is_blank(
    _InRef_     PC_SS_DATA p_ss_data)
{
    PTR_ASSERT(p_ss_data);
    assert(ss_data_is_string(p_ss_data));

    if(0 == ss_data_get_string_size(p_ss_data))
        return(TRUE);

    PTR_ASSERT(ss_data_get_string(p_ss_data));

    return(ss_data_get_string_size(p_ss_data) == ss_string_skip_leading_whitespace(p_ss_data));
}

_Check_return_
extern U32
ss_string_skip_leading_whitespace(
    _InRef_     PC_SS_DATA p_ss_data)
{
    assert(ss_data_is_string(p_ss_data));

    return(ss_string_skip_leading_whitespace_uchars(ss_data_get_string(p_ss_data), ss_data_get_string_size(p_ss_data)));
}

/* test character for 'space' in a spreadsheet text string - not necessarily same as Unicode classification, for instance */

_Check_return_
static inline BOOL
ss_string_ucs4_is_space(
    _InVal_     UCS4 ucs4)
{
    if(ucs4 > CH_SPACE)
        return(FALSE);

    switch(ucs4)
    {  /* just these are as defined as 'space' by OpenFormula */
    case UCH_CHARACTER_TABULATION:
    case UCH_LINE_FEED:
    case UCH_CARRIAGE_RETURN:
    case UCH_SPACE:
        return(TRUE);

    default:
        return(FALSE);
    }
}

_Check_return_
extern U32
ss_string_skip_leading_whitespace_uchars(
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InRef_     U32 uchars_n)
{
    return(ss_string_skip_internal_whitespace_uchars(uchars, uchars_n, 0U));
}

_Check_return_
extern U32
ss_string_skip_internal_whitespace_uchars(
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InVal_     U32 uchars_idx)
{
    U32 buf_idx = uchars_idx;
    U32 wss;

    assert( (0 == uchars_n) || PTR_NOT_NULL_OR_NONE(uchars) );

    while(buf_idx < uchars_n)
    {
        const U8 u8 = PtrGetByteOff(uchars, buf_idx);

        if(!ss_string_ucs4_is_space(u8))
            break;

        ++buf_idx;
    }

    wss = buf_idx - uchars_idx;
    return(wss);
}

_Check_return_
extern U32
ss_string_skip_trailing_whitespace_uchars(
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n)
{
    U32 buf_idx = uchars_n;
    U32 wss;

    assert ((0 == uchars_n) || PTR_NOT_NULL_OR_NONE(uchars) );

    while(0 != buf_idx)
    {
        const U8 u8 = PtrGetByteOff(uchars, buf_idx-1);

        if(!ss_string_ucs4_is_space(u8))
            break;

        --buf_idx;
    }

    wss = uchars_n - buf_idx;
    return(wss);
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
    _OutRef_    P_SS_DATA p_ss_data,
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
        if(NULL == (p_ss_data->arg.string_wr.uchars = al_ptr_alloc_bytes(P_UCHARS, len, &status)))
            return(status);
        p_ss_data->local_data = 1;
    }
    else
    {
        p_ss_data->arg.string.uchars = uchars_empty_string;
        p_ss_data->local_data = 0;
    }

    ss_data_set_data_id(p_ss_data, DATA_ID_STRING);
    p_ss_data->arg.string_wr.size = len;
    } /*block*/

    {
    P_U8 co = (P_U8) p_ss_data->arg.string_wr.uchars;
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

/* end of ss_string.c */
