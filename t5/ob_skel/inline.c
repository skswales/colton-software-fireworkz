/* inline.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Inline string routines */

/* MRJC January 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

/******************************************************************************
*
* return inline data from an inline
*
******************************************************************************/

extern void
data_from_inline(
    _Out_writes_bytes_(bytesof_buffer) P_ANY buffer,
    _InVal_     U32 bytesof_buffer,
    _In_reads_c_(INLINE_OVH) PC_USTR_INLINE ustr_inline,
    _InVal_     S32 type_expected)
{
    U32 n_bytes;

    if(!is_inline(ustr_inline))
    {
        assert(is_inline(ustr_inline));
        return;
    }

    if(inline_data_type(ustr_inline) != type_expected)
    {
        assert(inline_data_type(ustr_inline) == type_expected);
        return;
    }

    n_bytes = inline_data_size(ustr_inline);
    assert(n_bytes <= bytesof_buffer);
    if( n_bytes > bytesof_buffer)
        n_bytes = bytesof_buffer;

    memcpy32(buffer, inline_data_ptr(PC_BYTE, ustr_inline), n_bytes);
}

_Check_return_
extern S32
data_from_inline_s32(
    _In_reads_c_(INLINE_OVH + sizeof32(S32)) PC_USTR_INLINE ustr_inline)
{
    PC_BYTE p_data;
    S32 s32_result;

    if(!is_inline(ustr_inline))
    {
        assert(is_inline(ustr_inline));
        return(0);
    }

    assert(inline_data_type(ustr_inline) == IL_TYPE_S32);

    p_data = inline_data_ptr(PC_BYTE, ustr_inline);
    s32_result = readval_S32(p_data);
    return(s32_result);
}

/******************************************************************************
*
* append an inline created from an array of data to an array handle
*
******************************************************************************/

_Check_return_
extern STATUS /* size out */
inline_array_from_data(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle /*appended to*/,
    _InVal_     IL_CODE inline_id,
    _InVal_     IL_TYPE inline_type,
    _In_reads_bytes_opt_(size) PC_ANY p_data,
    _InVal_     U32 size)
{
    QUICK_UBLOCK quick_ublock;
    STATUS status;

    quick_ublock_setup_using_array(&quick_ublock, *p_array_handle); /* force use of handle (loan) */

    status = inline_quick_ublock_from_data(&quick_ublock, inline_id, inline_type, p_data, size);

    *p_array_handle = quick_ublock_array_handle_ref(&quick_ublock); /* retrieve loaned handle */

    /* no need to quick_block_dispose(); */

    return(status);
}

#if !defined(IL_OFF_COUNT_B)

/* return number of bytes in an in-line sequence (or character) going backwards from given offset */

_Check_return_
extern S32
inline_b_bytecount_off(
    _In_reads_(offset) PC_UCHARS_INLINE uchars,
    _InVal_     U32 offset)
{
    if(is_inline_b_off(uchars, offset))
    {   /* there is an end inline byte at (the byte before) offset
         * start looking for its matching start inline byte a minimum inline length away (backwards)
         */
        U32 test_offset = offset - (INLINE_OVH - 1);
        S32 found_bytecount = -1;

        for(;;)
        {
            S32 bytecount;

            if(0 == test_offset)
                break; /* ran out of string to test */

            --test_offset;

            bytecount = (S32) (offset - test_offset);

            if(bytecount >= 256) /* inlines are limited in length */
                break;

            if(!is_inline_off(uchars, test_offset))
                continue;

            if(inline_bytecount_off(uchars, test_offset) != bytecount)
                continue;

            /* this start inline matched up with the initial end inline */
            found_bytecount = bytecount;

            /* but keep searching backwards on the off chance that we may have just hit the right pattern in the inline sequence header or payload */
            continue;
        }

        if(found_bytecount < 0)
        {
            assert0(); /* no matching start inline byte found within plausible span */
            return(1); /* can only reverse over this byte */
        }

        return(found_bytecount);
    }

    return(uchars_bytes_prev_of_char_NS(uchars_AddBytes(uchars, offset)));
}

#endif /* IL_OFF_COUNT_B */

/******************************************************************************
*
* make an inline from an array of data
*
******************************************************************************/

_Check_return_
extern U32 /* size out */
inline_uchars_buf_from_data(
    _Out_writes_(elemof_buffer) P_UCHARS_INLINE uchars_inline,
    _InVal_     U32 elemof_buffer,
    _InVal_     S32 inline_id,
    _InVal_     S32 inline_type,
    _In_reads_bytes_opt_(size) PC_ANY p_data,
    _InVal_     U32 size /* only for TYPE_ANY */)
{
    U32 inline_size;
    U32 data_size;

    assert(inline_id != 0);

    switch(inline_type)
    {
    default: default_unhandled(); /*FALLTHRU*/
    case IL_TYPE_NONE:  data_size = 0;                              break;
    case IL_TYPE_USTR:  assert(p_data);
                        data_size = ustrlen32p1((PC_USTR) p_data);  break;
    case IL_TYPE_U8:    data_size = sizeof32(U8);                   break;
    case IL_TYPE_S32:   data_size = sizeof32(S32);                  break;
    case IL_TYPE_F64:   data_size = sizeof32(F64);                  break;
    case IL_TYPE_ANY:   data_size = size;                           break;
    }

    inline_size = data_size + INLINE_OVH;

    if(inline_size > U8_MAX)
    {
        assert(inline_size <= U8_MAX);
        return(0/*status_check()*/);
    }

    /* check space available */
    if(inline_size > elemof_buffer)
        return(0);

    PtrPutByte(   uchars_inline,               CH_INLINE);
#if defined(IL_OFF_NULL)
    PtrPutByteOff(uchars_inline, IL_OFF_NULL,  CH_NULL);
#endif
    PtrPutByteOff(uchars_inline, IL_OFF_COUNT, (BYTE) inline_size);
    PtrPutByteOff(uchars_inline, IL_OFF_CODE,  (BYTE) inline_id);
    PtrPutByteOff(uchars_inline, IL_OFF_TYPE,  (BYTE) inline_type);

    if(0 != data_size)
    {
        if(IS_PTR_NULL_OR_NONE(p_data))
            return(0/*status_check()*/);

        memcpy32(inline_data_ptr(P_BYTE, uchars_inline), p_data, data_size);
    }

#if defined(IL_OFF_COUNT_B)
    PtrPutByteOff(uchars_inline, inline_size - IL_OFF_COUNT_B, (BYTE) inline_size);
#endif
    PtrPutByteOff(uchars_inline, inline_size - IL_OFF_LEAD_B,  CH_INLINE);

    return(inline_size);
}

/******************************************************************************
*
* append an inline created from an array of data to a quick block
*
******************************************************************************/

_Check_return_
extern STATUS /* size out */
inline_quick_ublock_from_code(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     IL_CODE inline_id)
{
    STATUS status;
    const U32 inline_size = INLINE_OVH;
    P_USTR_INLINE uchars_inline;

    if(NULL == (uchars_inline = (P_UCHARS_INLINE) quick_ublock_extend_by(p_quick_ublock, inline_size, &status)))
        return(status);

    PtrPutByte(   uchars_inline,               CH_INLINE);
#if defined(IL_OFF_NULL)
    PtrPutByteOff(uchars_inline, IL_OFF_NULL,  CH_NULL);
#endif
    PtrPutByteOff(uchars_inline, IL_OFF_COUNT, (BYTE) inline_size);
    PtrPutByteOff(uchars_inline, IL_OFF_CODE,  (BYTE) inline_id);
    PtrPutByteOff(uchars_inline, IL_OFF_TYPE,  IL_TYPE_NONE);

    /* no payload */

#if defined(IL_OFF_COUNT_B)
    PtrPutByteOff(uchars_inline, inline_size - IL_OFF_COUNT_B, (BYTE) inline_size);
#endif
    PtrPutByteOff(uchars_inline, inline_size - IL_OFF_LEAD_B,  CH_INLINE);

    return((S32) inline_size);
}

_Check_return_
extern STATUS /* size out */
inline_quick_ublock_from_data(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     S32 inline_id,
    _InVal_     S32 inline_type,
    _In_reads_bytes_opt_(size) PC_ANY p_data,
    _InVal_     U32 size /* only for TYPE_ANY */)
{
    U32 inline_size;
    U32 data_size;
    P_USTR_INLINE uchars_inline;
    STATUS status;

    assert(inline_id != 0);

    switch(inline_type)
    {
    default: default_unhandled(); /*FALLTHRU*/
    case IL_TYPE_NONE:  data_size = 0;                              break;
    case IL_TYPE_USTR:  assert(p_data);
                        data_size = ustrlen32p1((PC_USTR) p_data);  break;
    case IL_TYPE_U8:    data_size = sizeof32(U8);                   break;
    case IL_TYPE_S32:   data_size = sizeof32(S32);                  break;
    case IL_TYPE_F64:   data_size = sizeof32(F64);                  break;
    case IL_TYPE_ANY:   data_size = size;                           break;
    }

    inline_size = data_size + INLINE_OVH;

    if(inline_size > U8_MAX)
    {
        assert(inline_size <= U8_MAX);
        return(status_check());
    }

    if(NULL == (uchars_inline = (P_UCHARS_INLINE) quick_ublock_extend_by(p_quick_ublock, inline_size, &status)))
        return(status);

    PtrPutByte(   uchars_inline,               CH_INLINE);
#if defined(IL_OFF_NULL)
    PtrPutByteOff(uchars_inline, IL_OFF_NULL,  CH_NULL);
#endif
    PtrPutByteOff(uchars_inline, IL_OFF_COUNT, (BYTE) inline_size);
    PtrPutByteOff(uchars_inline, IL_OFF_CODE,  (BYTE) inline_id);
    PtrPutByteOff(uchars_inline, IL_OFF_TYPE,  (BYTE) inline_type);

    if(0 != data_size)
    {
        if(IS_PTR_NULL_OR_NONE(p_data))
            return(status_check());

        memcpy32(inline_data_ptr(P_BYTE, uchars_inline), p_data, data_size);
    }

#if defined(IL_OFF_COUNT_B)
    PtrPutByteOff(uchars_inline, inline_size - IL_OFF_COUNT_B, (BYTE) inline_size);
#endif
    PtrPutByteOff(uchars_inline, inline_size - IL_OFF_LEAD_B,  CH_INLINE);

    return(inline_size);
}

_Check_return_
extern STATUS /* size out */
inline_quick_ublock_from_multiple_data(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     S32 inline_id,
    _InVal_     S32 inline_type,
    _In_reads_(n_elements) PC_MULTIPLE_DATA p_multiple_data,
    _InVal_     U32 n_elements)
{
    U32 inline_size;
    U32 data_size = 0;
    P_USTR_INLINE uchars_inline;
    STATUS status;

    assert(inline_id != 0);

    { /* sizing pass */
    U32 i;
    for(i = 0; i < n_elements; ++i)
    {
        const PC_ANY p_src = p_multiple_data[i].p_data;
        const U32 n_bytes = p_multiple_data[i].n_bytes;

        if(0 == n_bytes)
            continue;

        if(IS_PTR_NULL_OR_NONE(p_src))
            return(status_check());

        data_size += n_bytes;
    }
    } /*block*/

    inline_size = data_size + INLINE_OVH;

    if(inline_size > U8_MAX)
    {
        assert(inline_size <= U8_MAX);
        return(status_check());
    }

    if(NULL == (uchars_inline = (P_UCHARS_INLINE) quick_ublock_extend_by(p_quick_ublock, inline_size, &status)))
        return(status);

    PtrPutByte(   uchars_inline,               CH_INLINE);
#if defined(IL_OFF_NULL)
    PtrPutByteOff(uchars_inline, IL_OFF_NULL,  CH_NULL);
#endif
    PtrPutByteOff(uchars_inline, IL_OFF_COUNT, (BYTE) inline_size);
    PtrPutByteOff(uchars_inline, IL_OFF_CODE,  (BYTE) inline_id);
    PtrPutByteOff(uchars_inline, IL_OFF_TYPE,  (BYTE) inline_type);

    { /* copying pass */
    P_BYTE p_dst = inline_data_ptr(P_BYTE, uchars_inline);
    U32 i;
    for(i = 0; i < n_elements; ++i)
    {
        const PC_ANY p_src = p_multiple_data[i].p_data;
        const U32 n_bytes = p_multiple_data[i].n_bytes;

        if(0 == n_bytes)
            continue;

        memcpy32(p_dst, p_src, n_bytes);

        p_dst += n_bytes;
    }
    } /*block*/

#if defined(IL_OFF_COUNT_B)
    PtrPutByteOff(uchars_inline, inline_size - IL_OFF_COUNT_B, (BYTE) inline_size);
#endif
    PtrPutByteOff(uchars_inline, inline_size - IL_OFF_LEAD_B,  CH_INLINE);

    return(inline_size);
}

_Check_return_
extern STATUS /* size out */
inline_quick_ublock_from_ustr(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     IL_CODE inline_id,
    _In_z_      PC_USTR ustr)
{
    STATUS status;
    U32 inline_size;
    U32 data_size;
    P_USTR_INLINE uchars_inline;

    if(IS_PTR_NULL_OR_NONE(ustr))
        return(status_check());

#if CHECKING_UCHARS
    status_return(ustr_validate(TEXT("inline_quick_ublock_from_ustr arg"), ustr));
#endif

    data_size = ustrlen32p1(ustr);

    inline_size = data_size + INLINE_OVH;

    if(inline_size > U8_MAX)
    {
        assert(inline_size <= U8_MAX);
        return(status_check());
    }

    if(NULL == (uchars_inline = (P_UCHARS_INLINE) quick_ublock_extend_by(p_quick_ublock, inline_size, &status)))
        return(status);

    PtrPutByte(   uchars_inline,               CH_INLINE);
#if defined(IL_OFF_NULL)
    PtrPutByteOff(uchars_inline, IL_OFF_NULL,  CH_NULL);
#endif
    PtrPutByteOff(uchars_inline, IL_OFF_COUNT, (BYTE) inline_size);
    PtrPutByteOff(uchars_inline, IL_OFF_CODE,  (BYTE) inline_id);
    PtrPutByteOff(uchars_inline, IL_OFF_TYPE,  IL_TYPE_USTR);

    memcpy32(inline_data_ptr(P_BYTE, uchars_inline), ustr, data_size);

#if defined(IL_OFF_COUNT_B)
    PtrPutByteOff(uchars_inline, inline_size - IL_OFF_COUNT_B, (BYTE) inline_size);
#endif
    PtrPutByteOff(uchars_inline, inline_size - IL_OFF_LEAD_B,  CH_INLINE);

    return((S32) inline_size);
}

_Check_return_
extern STATUS /* size out */
inline_quick_ublock_IL_TAB(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/)
{
    return(inline_quick_ublock_from_code(p_quick_ublock, IL_TAB));
}

/******************************************************************************
*
* compare two inline strings for strcmp-like result
*
* NB -1/0/+1 normalised result
*
******************************************************************************/

_Check_return_
extern int
uchars_inline_compare_n2(
    _In_reads_(uchars_n_1) PC_UCHARS_INLINE uchars_inline_1,
    _InVal_     U32 uchars_n_1,
    _In_reads_(uchars_n_2) PC_UCHARS_INLINE uchars_inline_2,
    _InVal_     U32 uchars_n_2)
{
    int res = 0;
    STATUS status = STATUS_OK;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_1, 128);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_2, 128);
    quick_ublock_with_buffer_setup(quick_ublock_1);
    quick_ublock_with_buffer_setup(quick_ublock_2);

    if( status_ok(status = uchars_inline_plain_convert(&quick_ublock_1, uchars_inline_1, uchars_n_1)) &&
        status_ok(status = uchars_inline_plain_convert(&quick_ublock_2, uchars_inline_2, uchars_n_2)) )
    for(;;) /* loop for structure */
    {   /* SKS 15jul94 length limited comparison now no CH_NULL termination */
        PC_UCHARS a = quick_ublock_uchars(&quick_ublock_1);
        PC_UCHARS b = quick_ublock_uchars(&quick_ublock_2);
        U32 len_a = quick_ublock_bytes(&quick_ublock_1);
        U32 len_b = quick_ublock_bytes(&quick_ublock_2);
        U32 limit = MIN(len_a, len_b);
        U32 i = 0;
        U32 remain_a, remain_b;

        while(i < limit)
        {
            int c_a = PtrGetByteOff(a, i);
            int c_b = PtrGetByteOff(b, i);

            ++i;

            res = (c_a - c_b);

            if(0 != res)
            {   /* retry with case folding */
                c_a = t5_sortbyte(c_a);
                c_b = t5_sortbyte(c_b);

                res = (c_a - c_b);

                if(0 != res)
                    break;
            }
        }

        if(0 != res)
        {
            /* NB return normalised result */
            if(res < 0)
                res = -1;
            else
                res = 1;
            break; /* out of loop for structure */
        }

        /* matched up to the comparison limit */

        /* which counted sequence has the greater number of chars left over? */
        remain_a = (len_a - limit);
        remain_b = (len_b - limit);

        if(remain_a == remain_b)
        {
            res = 0; /* ended together at the specified finite lengths -> equal */
            break;
        }

        res = (int) remain_a - (int) remain_b;

        /* if a ended before b then
         *   a < b (-ve)
         * else
         *   a > b (+ve)
         */

        /* NB return normalised result */
        if(res < 0)
            res = -1;
        else
            res = 1;

        break; /* out of loop for structure */
    }

    status_assert(status);

    quick_ublock_dispose(&quick_ublock_1);
    quick_ublock_dispose(&quick_ublock_2);

    return(res);
}

/******************************************************************************
*
* copy an inline sequence but completely stripping inlines out
*
******************************************************************************/

/*ncr*/
extern U32
__pragma(warning(suppress: 6101)) /* Code Analysis not happy about zero return */
uchars_inline_copy_strip(
    _Out_writes_to_(elemof_buffer,return) P_UCHARS uchars_buf,
    _InVal_     U32 elemof_buffer,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline,
    _InVal_     U32 uchars_n)
{
    U32 offset = 0;
    U32 dst_idx = 0;

    PTR_ASSERT(uchars_buf);

    while(offset < uchars_n)
    {
        if(is_inline_off(uchars_inline, offset))
        {   /* skip inlines */
            offset += inline_bytecount_off(uchars_inline, offset);
        }
        else
        {
            U32 bytes_of_char = uchars_bytes_of_char_off((PC_UCHARS) uchars_inline, offset);

            /* is there room to put this whole character to destination buffer? */
            if((dst_idx + bytes_of_char) > elemof_buffer)
                break;

            uchars_char_copy(uchars_AddBytes_wr(uchars_buf, dst_idx), uchars_AddBytes(uchars_inline, offset), bytes_of_char);

            offset += bytes_of_char;
            dst_idx += bytes_of_char;
        }
    }

    assert(offset == uchars_n);

    assert(dst_idx <= elemof_buffer);

    return(dst_idx); /* number of bytes in output */
}

/******************************************************************************
*
* strcpy an inline string but completely stripping inlines out
*
******************************************************************************/

/*ncr*/
extern U32
ustr_inline_copy_strip(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR_INLINE ustr_inline)
{
    U32 offset = 0;
    U32 dst_idx = 0;

    PTR_ASSERT(ustr_buf);

    for(;;)
    {
        if(is_inline_off(ustr_inline, offset))
        {   /* skip inlines */
            offset += inline_bytecount_off(ustr_inline, offset);
        }
        else
        {
            U32 bytes_of_char = ustr_bytes_of_char_off((PC_USTR) ustr_inline, offset);

            if(CH_NULL == PtrGetByteOff(ustr_inline, offset))
                break;

            /* is there room to put both this whole character *and a CH_NULL* to destination buffer? */
            if((dst_idx + bytes_of_char) >= elemof_buffer)
                break;

            uchars_char_copy(uchars_AddBytes_wr(ustr_buf, dst_idx), uchars_AddBytes(ustr_inline, offset), bytes_of_char);

            dst_idx += bytes_of_char;
            offset += bytes_of_char;
        }
    }

    if(dst_idx >= elemof_buffer)
    {
        assert(dst_idx < elemof_buffer);
        if(0 == elemof_buffer)
            return(elemof_buffer);
        dst_idx = elemof_buffer - 1;
    }

    PtrPutByteOff(ustr_buf, dst_idx++, CH_NULL); /* ensure terminated */

    return(dst_idx); /* number of bytes in output including CH_NULL */
}

/******************************************************************************
*
* convert a raw inline sequence to plain text (UCHARs)
*
******************************************************************************/

_Check_return_
extern STATUS
uchars_inline_plain_convert(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline,
    _InVal_     U32 uchars_n)
{
    STATUS status = STATUS_OK;
    U32 offset = 0;

    while((offset < uchars_n) && status_ok(status))
    {
        if(is_inline_off(uchars_inline, offset))
        {
            switch(inline_code_off(uchars_inline, offset))
            {
            case IL_UTF8:
                {
                break;
                }

            default:
                status = quick_ublock_a7char_add(p_quick_ublock, CH_SPACE); /* all inlines simply get converted to spaces */
                break;
            }

            offset += inline_bytecount_off(uchars_inline, offset);
        }
        else
        {
            U32 bytes_of_char;
            UCS4 ucs4 = uchars_char_decode_off((PC_UCHARS) uchars_inline, offset, /*ref*/bytes_of_char);

            status = quick_ublock_ucs4_add(p_quick_ublock, ucs4);

            offset += bytes_of_char;
        }
    }

    assert(offset == uchars_n);

    /* SKS after 1.07 15jul94 now unterminated */
    return(status);
}

/******************************************************************************
*
* convert inline replace data from text into inline format
*
******************************************************************************/

_Check_return_
extern STATUS
ustr_inline_replace_convert(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_out,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock_in,
    _InVal_     S32 case_1,
    _InVal_     S32 case_2,
    _InVal_     S32 copy_capitals)
{
    STATUS status = STATUS_OK;
    const U32 limit = quick_ublock_bytes(p_quick_ublock_in);
    PC_UCHARS uchars_in_start = quick_ublock_uchars(p_quick_ublock_in);
    PC_UCHARS uchars_in = uchars_in_start;
    S32 charoffset = 0;

    while( (PtrDiffBytesU32(uchars_in, uchars_in_start) < limit) &&
           (CH_NULL != PtrGetByte(uchars_in)) )
    {
        UCS4 ucs4_replace = 0;
        IL_CODE il_code = IL_NONE;
        U32 bytecount = 1;

        if(CH_CIRCUMFLEX_ACCENT == PtrGetByte(uchars_in))
        {
            switch(PtrGetByteOff(uchars_in, 1))
            {
            case CH_NULL:
                status = STATUS_FAIL;
                break;

            default:
                bytecount = 1;
                ucs4_replace = PtrGetByte(uchars_in);
                break;

            case CH_CIRCUMFLEX_ACCENT:
                bytecount = 2;
                ucs4_replace = CH_CIRCUMFLEX_ACCENT; /* SKS 19apr93 after 1.03 - ^^T used to replace with ^ and TAB inline */
                break;

            case 'T':
            case 't':
                bytecount = 2;
                il_code = IL_TAB;
                status = inline_quick_ublock_from_code(p_quick_ublock_out, il_code);
                charoffset = 0;
                break;

            case 'R':
            case 'r':
                bytecount = 2;
                il_code = IL_RETURN;
                status = inline_quick_ublock_from_code(p_quick_ublock_out, il_code);
                charoffset = 0;
                break;

            case CH_HYPHEN_MINUS:
                bytecount = 2;
                il_code = IL_SOFT_HYPHEN;
                status = inline_quick_ublock_from_code(p_quick_ublock_out, il_code);
                charoffset = 0;
                break;
            }
        }
        else
        {
            ucs4_replace = uchars_char_decode(uchars_in, bytecount);
        }

        status_break(status);

        if(0 == il_code)
        {
            if(copy_capitals)
            {
                if((case_1 > 0) && (  (case_2 > 0) /* uppercasing all? */
                                   || (charoffset == 0) /* start of word? */) )
                {
                    ucs4_replace = t5_ucs4_uppercase(ucs4_replace);
                }
                else
                {
                    ucs4_replace = t5_ucs4_lowercase(ucs4_replace);
                }
            }

            status = quick_ublock_ucs4_add(p_quick_ublock_out, ucs4_replace);

            if((ucs4_replace == CH_SPACE) || (ucs4_replace == CH_HYPHEN_MINUS) || (ucs4_replace == CH_APOSTROPHE))
                charoffset = 0;
            else
                charoffset++;
        }

        status_break(status);

        uchars_IncBytes(uchars_in, bytecount);
    }

    return(status);
}

/******************************************************************************
*
* search an inline string
*
******************************************************************************/

_Check_return_
extern STATUS /* 0 == not found, 1 == found */
uchars_inline_search(
    _In_reads_(*p_end) PC_UCHARS_INLINE uchars_inline,
    _In_z_      PC_USTR ustr_search_for,
    _InoutRef_  P_S32 p_start,
    _InoutRef_  P_S32 p_end,
    _InVal_     BOOL ignore_capitals,
    _In_        BOOL whole_words)
{
    STATUS status = STATUS_OK;
    PC_UCHARS_INLINE uchars_inline_look_in_start;
    PC_UCHARS_INLINE uchars_inline_look_in_end;
    PC_UCHARS_INLINE uchars_inline_look_in;

    PTR_ASSERT(uchars_inline);
    PTR_ASSERT(p_start);
    PTR_ASSERT(p_end);

    if(CH_NULL == PtrGetByte(ustr_search_for))
        return(create_error(ERR_BAD_FIND_STRING));

    uchars_inline_look_in_start = uchars_inline_AddBytes(uchars_inline, *p_start);
    uchars_inline_look_in = uchars_inline_look_in_start;
    uchars_inline_look_in_end = uchars_inline_AddBytes(uchars_inline, *p_end);

    if(whole_words)
    {
        U32 bytes_of_char;
        UCS4 ucs4 = uchars_char_decode(ustr_search_for, bytes_of_char);
        if(!t5_ucs4_is_alphabetic(ucs4) && !t5_ucs4_is_decimal_digit(ucs4))
            whole_words = FALSE;
    }

    while(uchars_inline_look_in < uchars_inline_look_in_end)
    {
        PC_USTR ustr_look_for;

        if(whole_words)
        {
            if(PtrGetByte(uchars_inline_look_in) != CH_SPACE)
            {
                /* check if previous character is a space for word granularity */
                if((PtrDiffBytesS32(uchars_inline_look_in, uchars_inline) > *p_start) && (PtrGetByteOff(uchars_inline_look_in, -1) != CH_SPACE))
                    while((uchars_inline_look_in < uchars_inline_look_in_end) && (PtrGetByte(uchars_inline_look_in) != CH_SPACE))
                        inline_advance(PC_UCHARS_INLINE, uchars_inline_look_in);
            }

            /* skip spaces to find start of next word */
            while((uchars_inline_look_in < uchars_inline_look_in_end) && (PtrGetByte(uchars_inline_look_in) == CH_SPACE))
                inline_advance(PC_UCHARS_INLINE, uchars_inline_look_in);
        }

        uchars_inline_look_in_start = uchars_inline_look_in; /* remember for loop */

        ustr_look_for = ustr_search_for;

        while(uchars_inline_look_in <= uchars_inline_look_in_end)
        {
            UCS4 ch_look_for = 0, ch_look_in = 0;
            S32 inline_look_for = 0, inline_look_in = 0;

            if(CH_NULL == PtrGetByte(ustr_look_for))
            {
                UCS4 ucs4;
                if(!whole_words || (uchars_inline_look_in == uchars_inline_look_in_end))
                {
                    status = STATUS_DONE;
                    break;
                }
                ucs4 = uchars_char_decode_NULL((PC_UCHARS) uchars_inline_look_in);
                if(!t5_ucs4_is_alphabetic(ucs4) && !t5_ucs4_is_decimal_digit(ucs4))
                {
                    status = STATUS_DONE;
                    break;
                }
                break;
            }

            if(uchars_inline_look_in == uchars_inline_look_in_end)
                break;

            /* choose the character or inline that we are looking for */
            if(PtrGetByte(ustr_look_for) == CH_CIRCUMFLEX_ACCENT)
            {
                U32 bytes_of_char_look_for = 1;

                switch(PtrGetByteOff(ustr_look_for, 1))
                {
                case CH_NULL:
                    ch_look_for = 0;
                    status = create_error(ERR_BAD_FIND_STRING);
                    break;

                default:
                    ch_look_for = uchars_char_decode_off(ustr_look_for, 1, /*ref*/bytes_of_char_look_for);
                    break;

                case 'T':
                case 't':
                    inline_look_for = IL_TAB;
                    break;

                case 'R':
                case 'r':
                    inline_look_for = IL_RETURN;
                    break;

                case CH_HYPHEN_MINUS:
                    inline_look_for = IL_SOFT_HYPHEN;
                    break;
                }

                ustr_IncBytes(ustr_look_for, (1 + bytes_of_char_look_for));
            }
            else
            {
                U32 bytes_of_char_look_for;
                ch_look_for = uchars_char_decode(ustr_look_for, bytes_of_char_look_for);
                ustr_IncBytes(ustr_look_for, bytes_of_char_look_for);
            }

            status_break(status);

            if(is_inline(uchars_inline_look_in))
            {
                const IL_CODE il_code = inline_code(uchars_inline_look_in);

                switch(il_code)
                {
                case IL_TAB:
                case IL_RETURN:
                case IL_SOFT_HYPHEN:
                    inline_look_in = il_code;
                    break;

                default:
                    break;
                }

                uchars_inline_IncBytes(uchars_inline_look_in, inline_bytecount(uchars_inline_look_in));
            }
            else
            {
                U32 bytes_of_char_look_in;
                ch_look_in = uchars_char_decode((PC_UCHARS) uchars_inline_look_in, bytes_of_char_look_in);
                uchars_inline_IncBytes(uchars_inline_look_in, bytes_of_char_look_in);
            }

            if(inline_look_for != inline_look_in)
                break;

            if(0 == inline_look_for)
            {
                if(ch_look_for != ch_look_in)
                {   /* retry with case folding if allowed */
                    if(ignore_capitals)
                    {
                        ch_look_for = t5_ucs4_uppercase(ch_look_for);
                        ch_look_in  = t5_ucs4_uppercase(ch_look_in);
                    }

                    if(ch_look_for != ch_look_in)
                        break;
                }
            }
        } /* end of inner while */

        if(status_done(status))
        {
            *p_start = PtrDiffBytesU32(uchars_inline_look_in_start, uchars_inline);
            *p_end = PtrDiffBytesU32(uchars_inline_look_in, uchars_inline);
            break;
        }

        status_break(status);

        /* try looking at next position */
        uchars_inline_look_in = uchars_inline_AddBytes(uchars_inline_look_in_start, inline_bytecount(uchars_inline_look_in_start));
    } /* end of outer while */

    return(status);
}

/******************************************************************************
*
* convert a raw inline sequence to text for search
*
******************************************************************************/

_Check_return_
extern STATUS
uchars_inline_search_convert(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline,
    _InVal_     U32 uchars_n)
{
    STATUS status = STATUS_OK;
    U32 offset = 0;

    while((offset < uchars_n) && status_ok(status))
    {
        if(is_inline_off(uchars_inline, offset))
        {
            const IL_CODE il_code = inline_code_off(uchars_inline, offset);

            switch(il_code)
            {
            case IL_TAB:
            case IL_RETURN:
            case IL_SOFT_HYPHEN:
                {
                UCS4 char_out;

                switch(il_code)
                {
                default:
                case IL_TAB:
                    char_out = 't';
                    break;

                case IL_RETURN:
                    char_out = 'r';
                    break;

                case IL_SOFT_HYPHEN:
                    char_out = CH_HYPHEN_MINUS;
                    break;
                    }

                if(status_ok(status = quick_ublock_a7char_add(p_quick_ublock, CH_CIRCUMFLEX_ACCENT)))
                    status = quick_ublock_ucs4_add(p_quick_ublock, char_out);
                }
                break;

            /* ignore other inlines */
            default:
                break;
            }

            offset += inline_bytecount_off(uchars_inline, offset);
        }
        else
        {
            U32 bytes_of_char;
            UCS4 ucs4 = uchars_char_decode_off((PC_UCHARS) uchars_inline, offset, /*ref*/bytes_of_char);

            status = quick_ublock_ucs4_add(p_quick_ublock, ucs4);

            if((CH_CIRCUMFLEX_ACCENT == ucs4) && status_ok(status))
                status = quick_ublock_a7char_add(p_quick_ublock, CH_CIRCUMFLEX_ACCENT);

            offset += bytes_of_char;
        }
    }

    /* 17.3.95 MRJC: unterminated like all the others now are! */
    return(status);
}

/******************************************************************************
*
* return the total length in bytes of an inline string (excluding the CH_NULL)
*
******************************************************************************/

_Check_return_
extern U32
ustr_inline_strlen32(
    _In_z_      PC_USTR_INLINE ustr_inline)
{
#if defined(IL_OFF_NULL) /* can optimise using fast Norcroft strlen() when CH_NULL is used after CH_INLINE */
    PC_U8 p_u8 = (PC_U8) ustr_inline;

    if(CH_NULL == PtrGetByte(p_u8)) /* verifying string is not empty saves a test per loop */
        return(0);

    for(;;)
    {
        p_u8 = p_u8 + strlen(p_u8);

        /* p_u8 points at a NULLCH - either the real NULLCH or one in an inline header */

        assert(1 == IL_OFF_NULL);
        if(CH_INLINE == PtrGetByteOff(p_u8, -IL_OFF_NULL)) /* p_u8 is never == ustr_inline */
        {   /* the CH_NULL we found is at IL_OFF_NULL - keep going */
            p_u8 -= IL_OFF_NULL; /* retract to point at the leadin byte */

            p_u8 += inline_bytecount(p_u8); /* skip over the whole inline */

            /* might be an inline sequence right at the end, which would otherwise confuse */
            if(CH_NULL != PtrGetByte(p_u8))
                continue;

            /* p_u8 points at the real NULLCH */
        }

        return(PtrDiffBytesS32(p_u8, ustr_inline));
    }
#else
    PC_USTR_INLINE ustr_inline_cur = ustr_inline;

    while(CH_NULL != PtrGetByte(ustr_inline_cur))
        inline_advance(PC_USTR_INLINE, ustr_inline_cur);

    return(PtrDiffBytesU32(ustr_inline_cur, ustr_inline));
#endif
}

#if RISCOS
#define LOW_MEMORY_LIMIT  0x00008000U /* Don't look in zero page */
#define HIGH_MEMORY_LIMIT 0xFFFFFFFCU /* 32-bit RISC OS 5 */
#endif

_Check_return_
_Ret_z_
extern PCTSTR
report_ustr_inline(
    _In_opt_z_  PC_USTR_INLINE ustr_inline)
{
    if(NULL == ustr_inline)
        return(TEXT("<<NULL>>"));

#if CHECKING
    if(IS_PTR_NONE(ustr_inline))
        return(TEXT("<<NONE>>"));
#endif

    if(
#if RISCOS
        ((uintptr_t) ustr_inline < (uintptr_t) LOW_MEMORY_LIMIT ) ||
        ((uintptr_t) ustr_inline > (uintptr_t) HIGH_MEMORY_LIMIT) ||
#endif
        IS_BAD_POINTER(ustr_inline) )
    {
        static TCHARZ stringbuffer[16];
#if WINDOWS
        consume_int(_sntprintf_s(stringbuffer, elemof32(stringbuffer), _TRUNCATE, TEXT("<<") PTR_XTFMT TEXT(">>"), ustr_inline));
#else /* C99 CRT */
        consume_int(snprintf(stringbuffer, elemof32(stringbuffer), TEXT("<<") PTR_XTFMT TEXT(">>"), report_ptr_cast(ustr_inline)));
#endif
        return(stringbuffer);
    }

    /* Always watch out for inlines in USTR_INLINEs! */
    if(contains_inline(ustr_inline, ustr_inline_strlen32(ustr_inline)))
    {
        return(TEXT("<<CONTAINS INLINES>>"));
    }

    return(_tstr_from_ustr((PC_USTR) ustr_inline));
}

/* end of inline.c */
