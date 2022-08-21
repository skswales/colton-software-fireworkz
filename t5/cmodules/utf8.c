/* utf8.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2006-2016 Stuart Swales */

/* Library module for UTF-8 character & characters handling */

/* SKS Oct 2006 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef          __utf8_h
#include "cmodules/utf8.h"
#endif

/******************************************************************************
*
* return the number of bytes for a given UTF-8 character encoding sequence
*
******************************************************************************/

_Check_return_
extern U32
utf8__bytes_of_char(
    _In_        PC_UTF8 uchars)
{
    U8 u8 = PtrGetByte(uchars);
    U8 mask;

    mask = 0x80;
    if(0x00 == (u8 & mask))
        return(1); /* U+0000 : U+007F (7 bits) */

    mask = 0xE0;
    if(0xC0 == (u8 & mask))
        return(2); /* U+0080 : U+07FF (11 bits) */

    mask = 0xF0;
    if(0xE0 == (u8 & mask))
        return(3); /* U+0800 : U+FFFF (16 bits) */

    mask = 0xF8;
    if(0xF0 == (u8 & mask))
        return(4); /* U+010000 : U+1FFFFF (21 bits) */

    myassert1(TEXT("byte 0x%.2X is not a valid UTF-8 encoding"), u8);
    return(1); /* so that caller doesn't get stuck... */
}

/******************************************************************************
*
* return the number of bytes for a given UTF-8 character encoding sequence
* going backwards
*
* uchars_start may be NULL but why wouldn't you have called the _NS() function?
*
******************************************************************************/

_Check_return_
extern U32
utf8__bytes_prev_of_char(
    _InRef_     PC_UTF8 uchars_start,
    _InRef_     PC_UTF8 uchars)
{
    PC_U8 uchars_prev = (PC_U8) uchars;
    U32 trail_bytes = 0;
    U32 bytes_of_char = 0;
    U8 ch;

    /* simply skip backwards over all UTF-8 trail bytes */
    while((PC_UTF8) uchars_prev != uchars_start)
    {
        ch = *--uchars_prev;

        if(u8_is_utf8_trail_byte(ch))
        {   /* UTF-8 trail byte */
            bytes_of_char++;
            trail_bytes++;
            assert(trail_bytes <= 3);
            continue;
        }

        /* UTF-8 lead byte or simple ASCII-7 */
        bytes_of_char++;
        break;
    }

    /* if we've had UTF-8 trail chars we must now be on a UTF-8 lead byte */
    assert((trail_bytes == 0) || u8_is_utf8_lead_byte(PtrGetByte(uchars_prev)));

    return(bytes_of_char);
}

/*
as above, but without start limit (for inline_b_bytecount_off() during transition when not skipping grapheme clusters)
*/

_Check_return_
extern U32
utf8__bytes_prev_of_char_NS(
    _InRef_     PC_UTF8 uchars)
{
    PC_U8 uchars_prev = (PC_U8) uchars;
    U32 trail_bytes = 0;
    U32 bytes_of_char = 0;
    U8 ch;

    /* simply skip backwards over all UTF-8 trail bytes */
    for(;;)
    {
        ch = *--uchars_prev;

        if(u8_is_utf8_trail_byte(ch))
        {   /* UTF-8 trail byte */
            bytes_of_char++;
            trail_bytes++;
            assert(trail_bytes <= 3);
            continue;
        }

        /* UTF-8 lead byte or simple ASCII-7 */
        bytes_of_char++;
        break;
    }

    /* if we've had UTF-8 trail chars we must now be on a UTF-8 lead byte */
    assert((trail_bytes == 0) || u8_is_utf8_lead_byte(PtrGetByte(uchars_prev)));

    return(bytes_of_char);
}

/******************************************************************************
*
* return the number of bytes occupied as UTF-8
* by the first n_chars characters
*
******************************************************************************/

_Check_return_
extern U32
utf8_bytes_of_chars(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n,
    _InVal_     U32 n_chars)
{
    U32 bytes_so_far = 0;
    U32 chars_so_far;

    for(chars_so_far = 0; (bytes_so_far < uchars_n) && (chars_so_far < n_chars); ++chars_so_far) /* eot -> character sequence break */
    {
        U32 this_n_bytes = utf8_bytes_of_char(utf8_AddBytes(uchars, bytes_so_far));
        U32 new_bytes_so_far = bytes_so_far + this_n_bytes;

        if(new_bytes_so_far > uchars_n)
        {
            myassert0(TEXT("character sequence spills off the end of counted character sequence"));
            break;
        }

        bytes_so_far = new_bytes_so_far;
    }

    return(bytes_so_far);
}

/******************************************************************************
*
* return the current character in a UTF-8 character sequence as a UCS-4,
* also returning the number of bytes it occupies as UTF-8.
*
******************************************************************************/

_Check_return_
extern UCS4
utf8__char_decode(
    _In_        PC_UTF8 uchars,
    _Out_opt_   P_U32 p_bytes_of_char)
{
    PC_U8 p_u8 = (PC_U8) uchars;
    U8 u8 = *p_u8++;
    U8 mask;
    UCS4 ucs4 = 0;
    U32 bytes_of_char;

    for(;;) /* loop for structure */
    {
        mask = 0x80;
        if(0x00 == (u8 & mask))
        {   /* U+0000 : U+007F (7 bits) */
            ucs4 = u8;
            bytes_of_char = 1;
            break;
        }

        mask = 0xE0;
        if(0xC0 == (u8 & mask))
        {   /* U+0080 : U+07FF (11 bits) */
            bytes_of_char = 2;
            break;
        }

        mask = 0xF0;
        if(0xE0 == (u8 & mask))
        {   /* U+0800 : U+FFFF (16 bits) */
            bytes_of_char = 3;
            break;
        }

        mask = 0xF8;
        if(0xF0 == (u8 & mask))
        {   /* U+010000 : U+1FFFFF (21 bits) */
            bytes_of_char = 4;
            break;
        }

        myassert1(TEXT("byte 0x%.2X is not valid UTF-8 encoding"), u8);
        bytes_of_char = 1; /* so that caller doesn't get stuck... */
        break; /* end of loop for structure */ /*NOTREACHED*/
    }

    if(bytes_of_char > 1)
    {
        U32 shift = (bytes_of_char - 1) * 6;

        ucs4 = ((UCS4) (u8 & ~mask)) << shift; /* N bits */

        do  {
            shift -= 6;
            u8 = *p_u8++;
            assert(u8_is_utf8_trail_byte(u8));
            ucs4 |= ((UCS4) (u8 & ~0xC0)) << shift; /* 6 bits */
        }
        while(shift != 0);

        if(status_fail(ucs4_validate(ucs4)))
        {
            myassert3(TEXT("Invalid UCS-4 U+%.4X decoded from %p:%u bytes"), ucs4, PtrSubBytes(PC_ANY, p_u8, bytes_of_char), bytes_of_char);
            ucs4 = UCH_REPLACEMENT_CHARACTER;
        }
    }

    if(NULL != p_bytes_of_char)
        *p_bytes_of_char = bytes_of_char;

    return(ucs4);
}

/******************************************************************************
*
* encode a UCS-4 character to its UTF-8 representation at given offset in a buffer
*
* buffer is unmolested if elemof_buffer doesn't fit the entire UTF-8 character sequence
*
* call with elemof_buffer == 0 and encode_offset == 0 to count encoding space required
*
******************************************************************************/

_Check_return_
extern U32 /* number of bytes */
utf8__char_encode_off(
    _Out_cap_(elemof_buffer) P_UTF8 utf8_buf /*filled*/,
    _InVal_     U32 elemof_buffer,
    _InVal_     U32 encode_offset,
    _In_        UCS4 ucs4)
{
    if(ucs4 < 0x00000080U)
    {   /* U+0000 : U+007F (7 bits) */
        if(elemof_buffer >= (1 + encode_offset))
        {
            PtrPutByteOff(utf8_buf, encode_offset, (U8) (ucs4));
        }
        return(1);
    }

    if(ucs4 < 0x00000800U)
    {   /* U+0080 : U+07FF (11 bits) */
        if(elemof_buffer >= (2 + encode_offset))
        {
            PtrPutByteOff(utf8_buf, encode_offset + 0, (U8) (0xC0 | (0x1F & (ucs4 >>  6))) /* first 5 bits */ );
            PtrPutByteOff(utf8_buf, encode_offset + 1, (U8) (0x80 | (0x3F & (ucs4 >>  0))) /*  last 6 bits */ );
        }
        return(2);
    }

#if CHECKING_UTF8
    assert(status_ok(ucs4_validate(ucs4)));
#endif

    if(ucs4 < 0x00010000U)
    {   /* U+0800 : U+FFFF (16 bits) */
        if(elemof_buffer >= (3 + encode_offset))
        {
            PtrPutByteOff(utf8_buf, encode_offset + 0, (U8) (0xE0 | (0x0F & (ucs4 >> 12))) /* first 4 bits */ );
            PtrPutByteOff(utf8_buf, encode_offset + 1, (U8) (0x80 | (0x3F & (ucs4 >>  6))) /*  next 6 bits */ );
            PtrPutByteOff(utf8_buf, encode_offset + 2, (U8) (0x80 | (0x3F & (ucs4 >>  0))) /*  last 6 bits */ );
        }
        return(3);
    }

    if(ucs4 < UCH_UNICODE_INVALID)
    {   /* U+010000 : U+1FFFFF (21 bits, but range limited much further by the Unicode Standard to U+10FFFF) */
        if(elemof_buffer >= (4 + encode_offset))
        {
            PtrPutByteOff(utf8_buf, encode_offset + 0, (U8) (0xF0 | (0x07 & (ucs4 >> 18))) /* first 3 bits */ );
            PtrPutByteOff(utf8_buf, encode_offset + 1, (U8) (0x80 | (0x3F & (ucs4 >> 12))) /*  next 6 bits */ );
            PtrPutByteOff(utf8_buf, encode_offset + 2, (U8) (0x80 | (0x3F & (ucs4 >>  6))) /*  next 6 bits */ );
            PtrPutByteOff(utf8_buf, encode_offset + 3, (U8) (0x80 | (0x3F & (ucs4 >>  0))) /*  last 6 bits */ );
        }
        return(4);
    }

    myassert0(TEXT("invalid UCS-4"));
    return(0);
}

/******************************************************************************
*
* return number of bytes needed to encode a UCS-4 character to its UTF-8 representation
*
******************************************************************************/

_Check_return_
extern U32 /* number of bytes */
utf8__bytes_of_char_encoding(
    _In_        UCS4 ucs4)
{
    if(ucs4 < 0x00000080U)
        /* U+0000 : U+007F (7 bits) */
        return(1);

    if(ucs4 < 0x00000800U)
        /* U+0080 : U+07FF (11 bits) */
        return(2);

    if(ucs4 < 0x00010000U)
        /* U+0800 : U+FFFF (16 bits) */
        return(3);

    if(ucs4 < UCH_UNICODE_INVALID)
        /* U+010000 : U+1FFFFF (21 bits, but range limited much further by the Unicode Standard to U+10FFFF) */
        return(4);

    myassert0(TEXT("invalid UCS-4"));
    return(0);
}

/******************************************************************************
*
* find a UCS-4 character in a UTF-8 character encoding sequence
*
******************************************************************************/

_Check_return_
extern P_UTF8
utf8_char_find(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n,
    _In_        UCS4 search_ucs4)
{
    U32 n_bytes_so_far = 0;

#if CHECKING_UTF8
    if(status_fail(utf8_validate(TEXT("utf8_char_find arg"), uchars, uchars_n)))
        return(NULL);
#endif

    while(n_bytes_so_far < uchars_n)
    {
        PC_UTF8 this_p_char = utf8_AddBytes(uchars, n_bytes_so_far);
        U32 this_n_bytes;
        UCS4 this_ucs4 = utf8_char_decode(this_p_char, /*ref*/this_n_bytes);

        if(this_ucs4 == search_ucs4) /* NB allows search for CH_NULL */
        {
            return(de_const_cast(P_UTF8, this_p_char));
        }

        if(this_ucs4 == CH_NULL) /* allows n_bytes == strlen_with,without_NULLCH */
            break;

        n_bytes_so_far += this_n_bytes;
    }

    return(NULL);
}

/******************************************************************************
*
* find a UCS-4 character in a UTF-8 character encoding sequence going backwards
*
* uchars_start may be NULL
*
******************************************************************************/

_Check_return_
extern P_UTF8
utf8_char_find_rev(
    _In_reads_(uchars_n) PC_UTF8 uchars_start,
    _InVal_     U32 uchars_n,
    _In_        UCS4 search_ucs4)
{
    PC_UTF8 uchars = utf8_AddBytes(uchars_start, uchars_n);
    U32 n_bytes_so_far = 0;

#if CHECKING_UTF8
    if(status_fail(utf8_validate(TEXT("utf8_char_find_rev arg"), uchars_start, uchars_n)))
        return(NULL);
#endif

    while(n_bytes_so_far < uchars_n)
    {
        PC_UTF8 uchars_prev;
        UCS4 this_ucs4 = utf8_char_prev(uchars_start, uchars, &uchars_prev);
        U32 this_n_bytes = PtrDiffBytesU32(uchars, uchars_prev);

        uchars = uchars_prev;
        n_bytes_so_far += this_n_bytes;

        if(this_ucs4 == search_ucs4)
        {
            return(de_const_cast(P_UTF8, uchars));
        }
    }

    return(NULL);
}

/******************************************************************************
*
* return the current UCS-4 character in a UTF-8 character sequence,
* returning pointer to the start of the next character encoding.
*
******************************************************************************/

_Check_return_
extern UCS4
utf8_char_next(
    _In_        PC_UTF8 uchars,
    _OutRef_    P_PC_UTF8 p_uchars_next)
{
    U32 bytes_of_char;
    UCS4 ucs4 = utf8_char_decode(uchars, /*ref*/bytes_of_char);

    /* return pointer to byte at start of the next UTF-8 character sequence */
    /*RETURN_VALUE_IF_NULL_OR_BAD_POINTER(p_uchars_next, 0)*/
    *p_uchars_next = utf8_AddBytes(uchars, bytes_of_char);

    return(ucs4);
}

/******************************************************************************
*
* return the previous UCS-4 character in a UTF-8 character sequence,
* returning pointer to the start of that character encoding.
*
* uchars_start may be NULL only for uchars__bytes_prev_of_grapheme_cluster_NS()
*
******************************************************************************/

_Check_return_
extern UCS4
utf8_char_prev(
    _In_opt_    PC_UTF8 uchars_start,
    _In_        PC_UTF8 uchars,
    _OutRef_    P_PC_UTF8 p_uchars_prev)
{
    U8 ch;
    U32 trail_bytes = 0;

#if CHECKING_UTF8
    if((NULL != uchars_start) && status_fail(utf8_validate(TEXT("utf8_char_prev arg"), uchars_start, PtrDiffBytesU32(uchars, uchars_start))))
    {
        *p_uchars_prev = NULL;
        return(0);
    }
#endif

    /* simply skip backwards over all UTF-8 trail bytes, then decode a char */
    for(;;)
    {
        if(NULL != uchars_start)
            if(uchars == uchars_start)
                break;

        PtrDecBytes(PC_UTF8, uchars, 1);

        ch = PtrGetByte(uchars);

        if(u8_is_utf8_trail_byte(ch))
        {   /* UTF-8 trail byte */
            trail_bytes++;
            assert(trail_bytes <= 3);
            continue;
        }

        /* return pointer to byte at start of this UTF-8 character sequence */
        *p_uchars_prev = de_const_cast(P_UTF8, uchars); /*de-const for return*/

        /* if we've had trail bytes we must now be on a lead byte */
        assert((trail_bytes == 0) || u8_is_utf8_lead_byte(PtrGetByte(uchars)));

        return(utf8_char_decode_NULL(uchars));
    }

    *p_uchars_prev = NULL;

    return(CH_NULL);
}

/******************************************************************************
*
* return the number of UCS-4 characters represented by uchars_n bytes of UTF-8 encoding
*
******************************************************************************/

_Check_return_
extern U32 /* n_chars */
utf8_chars_of_bytes(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n)
{
    U32 n_bytes_so_far = 0;
    U32 n_chars;

#if CHECKING_UTF8
    if(status_fail(utf8_validate(TEXT("utf8_chars_of_bytes arg"), uchars, uchars_n)))
        return(0);
#endif

    for(n_chars = 0; n_bytes_so_far < uchars_n; ++n_chars)
    {
        PC_UTF8 this_p_char = utf8_AddBytes(uchars, n_bytes_so_far);
        U32 this_n_bytes = utf8_bytes_of_char(this_p_char);
        U32 new_bytes_so_far = n_bytes_so_far + this_n_bytes;

        if(new_bytes_so_far > uchars_n)
        {
            myassert0(TEXT("character sequence spills off the end of counted character sequence"));
            break;
        }

        n_bytes_so_far = new_bytes_so_far;
    }

    return(n_chars);
}

/******************************************************************************
*
* return the number of bytes for a given UTF-8 grapheme cluster encoding sequence
*
******************************************************************************/

_Check_return_
extern U32
utf8__bytes_of_grapheme_cluster(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n)
{
    U32 bytes_so_far = 0;

    while(bytes_so_far < uchars_n) /* eot -> grapheme cluster break */
    {
        PC_UTF8 this_p_char = utf8_AddBytes(uchars, bytes_so_far);
        U32 this_n_bytes;
        UCS4 ucs4 = utf8_char_decode(this_p_char, /*ref*/this_n_bytes);
        U32 new_bytes_so_far;

        /* if sot, or is Grapheme_Extend, do not break at this character */
        if((0 != bytes_so_far) && !ucs4_is_grapheme_extend(ucs4))
            break;

        new_bytes_so_far = bytes_so_far + this_n_bytes;

        if(new_bytes_so_far > uchars_n)
        {
            myassert0(TEXT("grapheme cluster spills off the end of counted character sequence"));
            break;
        }

        bytes_so_far = new_bytes_so_far;
    }

    return(bytes_so_far);
}

/*
as above, but without counted UTF-8 character sequence
*/

_Check_return_
extern U32
utf8__bytes_of_grapheme_cluster_NC(
    _In_        PC_UTF8 uchars)
{
    U32 bytes_so_far = 0;

    for(;;)
    {
        PC_UTF8 this_p_char = utf8_AddBytes(uchars, bytes_so_far);
        U32 this_n_bytes;
        UCS4 ucs4 = utf8_char_decode(this_p_char, /*ref*/this_n_bytes);

        /* if sot, or is Grapheme_Extend, do not break at this character */
        if((0 != bytes_so_far) && !ucs4_is_grapheme_extend(ucs4))
            break;

        bytes_so_far += this_n_bytes;
    }

    return(bytes_so_far);
}

/******************************************************************************
*
* return the number of bytes for a given UTF-8 grapheme cluster encoding sequence
* going backwards
*
* uchars_start may be NULL but why wouldn't you have called the _NS() function?
*
******************************************************************************/

_Check_return_
extern U32
utf8__bytes_prev_of_grapheme_cluster(
    _InRef_     PC_UTF8 uchars_start,
    _InRef_     PC_UTF8 uchars)
{
    PC_UTF8 uchars_prev = uchars;

    /* simply skip backwards over all Grapheme_Extend characters */
    while(uchars != uchars_start)
    {
        UCS4 this_ucs4 = utf8_char_prev(uchars_start, uchars_prev, &uchars_prev);

        if(!ucs4_is_grapheme_extend(this_ucs4))
            break;
    }

    return(PtrDiffBytesU32(uchars, uchars_prev));
}

/*
as above, but without start limit (for inline_b_bytecount_off())
*/

_Check_return_
extern U32
utf8__bytes_prev_of_grapheme_cluster_NS(
    _InRef_     PC_UTF8 uchars)
{
    PC_UTF8 uchars_prev = uchars;

    /* simply skip backwards over all Grapheme_Extend characters */
    for(;;)
    {
        UCS4 this_ucs4 = utf8_char_prev(NULL, uchars_prev, &uchars_prev);

        if(!ucs4_is_grapheme_extend(this_ucs4))
            break;
    }

    return(PtrDiffBytesU32(uchars, uchars_prev));
}

/******************************************************************************
*
* return the number of bytes occupied as UTF-8
* by the first n_grapheme_clusters grapheme clusters in uchars_n bytes of UTF-8 encoding
*
******************************************************************************/

_Check_return_
extern U32
utf8_bytes_of_grapheme_clusters(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n,
    _InVal_     U32 n_grapheme_clusters)
{
    U32 bytes_so_far = 0;
    U32 grapheme_clusters_so_far;

    for(grapheme_clusters_so_far = 0; (bytes_so_far < uchars_n) && (grapheme_clusters_so_far < n_grapheme_clusters); ++grapheme_clusters_so_far) /* eot -> grapheme cluster sequence break */
    {
        U32 this_n_bytes = utf8_bytes_of_grapheme_cluster(utf8_AddBytes(uchars, bytes_so_far), uchars_n - bytes_so_far);
        U32 new_bytes_so_far = bytes_so_far + this_n_bytes;

        if(new_bytes_so_far > uchars_n)
        {
            myassert0(TEXT("grapheme cluster sequence spills off the end of counted character sequence"));
            break;
        }

        bytes_so_far = new_bytes_so_far;
    }

    return(bytes_so_far);
}

/******************************************************************************
*
* return the current character in a UTF-8 character sequence as a UCS-4,
* also returning the number of bytes it occupies as UTF-8,
* also returning the length of the whole grapheme cluster.
*
******************************************************************************/

_Check_return_
extern UCS4
utf8__grapheme_cluster_decode(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n,
    _OutRef_    P_U32 p_bytes_of_char,
    _OutRef_    P_U32 p_bytes_of_grapheme_cluster)
{
    PC_U8 p_u8 = (PC_U8) uchars;
    UCS4 ucs4 = 0;
    U32 bytes_of_char = 1; /* so that we don't get stuck... */
    U8 u8;
    U8 mask;

    assert(0 != uchars_n);

    for(;;) /* loop for structure */
    {
        u8 = *p_u8++;

        mask = 0x80;
        if((u8 & mask) == 0x00)
        {   /* U+0000 : U+007F (7 bits) */
            ucs4 = u8;
            bytes_of_char = 1;
            break;
        }

        mask = 0xE0;
        if((u8 & mask) == 0xC0)
        {   /* U+0080 : U+07FF (11 bits) */
            bytes_of_char = 2;
            break;
        }

        mask = 0xF0;
        if((u8 & mask) == 0xE0)
        {   /* U+0800 : U+FFFF (16 bits) */
            bytes_of_char = 3;
            break;
        }

        mask = 0xF8;
        if((u8 & mask) == 0xF0)
        {   /* U+010000 : U+1FFFFF (21 bits) */
            bytes_of_char = 4;
            break;
        }

        assert(!u8); /* not UTF-8 encoding */

        break; /* end of loop for structure */ /*NOTREACHED*/
    }

    assert(bytes_of_char <= uchars_n);

    if(bytes_of_char > 1)
    {
        U32 shift = (bytes_of_char - 1) * 6;

        ucs4 = ((UCS4) (u8 & ~mask)) << shift; /* N bits */

        do  {
            shift -= 6;
            u8 = *p_u8++;
            assert(u8_is_utf8_trail_byte(u8));
            ucs4 |= ((UCS4) (u8 & ~0xC0)) << shift; /* 6 bits */
        }
        while(shift != 0);
    }

    if(NULL != p_bytes_of_char) *p_bytes_of_char = bytes_of_char;

#if CHECKING_UTF8
    assert(status_ok(ucs4_validate(ucs4)));
#endif

    assert(p_bytes_of_grapheme_cluster);

    *p_bytes_of_grapheme_cluster = bytes_of_char;

    while(*p_bytes_of_grapheme_cluster < uchars_n)
    {
        PC_UTF8 this_p_char = utf8_AddBytes(uchars, *p_bytes_of_grapheme_cluster);
        UCS4 this_ucs4 = utf8_char_decode(this_p_char, /*ref*/bytes_of_char);

#if CHECKING_UTF8
        if(status_fail(ucs4_validate(this_ucs4)))
        {
            assert(status_ok(ucs4_validate(this_ucs4)));
            break;
        }
#endif

        if(!ucs4_is_grapheme_extend(this_ucs4))
            break;

        *p_bytes_of_grapheme_cluster += bytes_of_char;
    }

    return(ucs4);
}

/******************************************************************************
*
* return the number of grapheme clusters represented by uchars_n bytes of UTF-8 encoding
*
******************************************************************************/

_Check_return_
extern U32 /* n_chars */
utf8_grapheme_clusters_of_bytes(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n)
{
    U32 n_bytes_so_far = 0;
    U32 grapheme_clusters_so_far;

#if CHECKING_UTF8
    if(status_fail(utf8_validate(TEXT("utf8_grapheme_clusters_of_bytes arg"), uchars, uchars_n)))
        return(0);
#endif

    for(grapheme_clusters_so_far = 0; n_bytes_so_far < uchars_n; ++grapheme_clusters_so_far)
    {
        PC_UTF8 this_p_grapheme_cluster = utf8_AddBytes(uchars, n_bytes_so_far);
        U32 this_n_bytes = utf8_bytes_of_grapheme_cluster(this_p_grapheme_cluster, uchars_n - n_bytes_so_far);
        U32 new_n_bytes_so_far = n_bytes_so_far + this_n_bytes;

        if(new_n_bytes_so_far > uchars_n)
        {
            myassert0(TEXT("grapheme cluster sequence spills off the end of counted character sequence"));
            break;
        }

        n_bytes_so_far = new_n_bytes_so_far;
    }

    return(grapheme_clusters_so_far);
}

/******************************************************************************
*
* validate a UTF-8 character sequence
*
* variant including inline processing may be used
* e.g. needed for recursive construct processing during file load
*
******************************************************************************/

_Check_return_
extern STATUS /* STATUS_OK, error if not */
utf8_validate(
    _In_z_      PCTSTR func,
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n)
{
    PC_U8 p_u8_in = (PC_U8) uchars;
    PC_U8 p_u8 = p_u8_in;
    U32 n_bytes_so_far = 0;

    UNREFERENCED_PARAMETER_CONST(func);

    assert(strlen_without_NULLCH > uchars_n);

    while(n_bytes_so_far < uchars_n)
    {
        U32 bytes_of_char = 0;
        U8 u8;
        U8 mask;
        UCS4 ucs4 = 0;
        U8 second_byte_lower_limit = 0x80;
        U8 second_byte_upper_limit = 0xBF;

        for(;;) /* loop for structure */
        {
            u8 = *p_u8++;

            assert(CH_INLINE != u8);

            mask = 0x80;
            if((u8 & mask) == 0x00)
            {   /* U+0000 : U+007F (7 bits) */
                ucs4 = u8;
                bytes_of_char = 1;
                break;
            }

            mask = 0xE0;
            if((u8 & mask) == 0xC0)
            {   /* U+0080 : U+07FF (11 bits) */
                bytes_of_char = 2;
                if(u8 <= 0xC1)
                {
                    myassert5(TEXT("%s: Not Well Formed UTF-8 - first byte fails in sequence ") PTR_XTFMT TEXT("(%d) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
                    return(ERR_INVALID_UTF8_ENCODING);
                }
                break;
            }

            mask = 0xF0;
            if((u8 & mask) == 0xE0)
            {   /* U+0800 : U+FFFF (16 bits) */
                bytes_of_char = 3;
                if(u8 == 0xE0)
                    second_byte_lower_limit = 0xA0;
                else if(u8 == 0xED)
                    second_byte_upper_limit = 0x9F;
                break;
            }

            mask = 0xF8;
            if((u8 & mask) == 0xF0)
            {   /* U+010000 : U+1FFFFF (21 bits) */
                bytes_of_char = 4;
                if(u8 == 0xF0)
                    second_byte_lower_limit = 0x90;
                else if(u8 == 0xF5)
                    second_byte_upper_limit = 0x8F;
                else if(u8 >= 0xF5)
                {
                    myassert5(TEXT("%s: Not Well Formed UTF-8 - first byte fails in sequence ") PTR_XTFMT TEXT("(%u) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
                    return(ERR_INVALID_UTF8_ENCODING);
                }
                break;
            }

            /* not UTF-8 encoding */
            myassert5(TEXT("%s: Not Well Formed UTF-8 - first byte fails in sequence ") PTR_XTFMT TEXT("(%u) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
            return(ERR_INVALID_UTF8_ENCODING); /* end of loop for structure */ /*NOTREACHED*/
        }

        if((n_bytes_so_far + bytes_of_char) > uchars_n)
        {   /* UTF-8 lead byte indicates character encoding too long for this sequence */
            myassert4(TEXT("%s: UTF-8 char extends beyond sequence limits ") PTR_XTFMT TEXT("(%u) @ %u"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in));
            return(ERR_INVALID_UTF8_ENCODING);
        }

        if(bytes_of_char > 1)
        {
            U32 shift = (bytes_of_char - 1) * 6;

            ucs4 = ((UCS4) (u8 & ~mask)) << shift; /* N bits */

            /* always a second byte at this point - check that it is OK for well-formed UTF-8 */
            shift -= 6;
            u8 = *p_u8++;
            if(!u8_is_utf8_trail_byte(u8))
            {
                myassert5(TEXT("%s: Not UTF-8 trail byte in sequence ") PTR_XTFMT TEXT("(%u) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
                return(ERR_INVALID_UTF8_ENCODING);
            }
            if((u8 < second_byte_lower_limit) || (u8 > second_byte_upper_limit))
            {
                myassert5(TEXT("%s: Not Well Formed UTF-8 - second byte fails in sequence ") PTR_XTFMT TEXT("(%u) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
                return(ERR_INVALID_UTF8_ENCODING);
            }
            ucs4 |= ((UCS4) (u8 & ~0xC0)) << shift; /* 6 bits */

            while(shift != 0)
            {
                shift -= 6;
                u8 = *p_u8++;
                if(!u8_is_utf8_trail_byte(u8))
                {
                    myassert5(TEXT("%s: Not UTF-8 trail byte in sequence ") PTR_XTFMT TEXT("(%u) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
                    return(ERR_INVALID_UTF8_ENCODING);
                }
                ucs4 |= ((UCS4) (u8 & ~0xC0)) << shift; /* 6 bits */
            }
        }

        (void) ucs4_validate(ucs4);

        n_bytes_so_far += bytes_of_char;
    }

    return(STATUS_OK);
}

#if !defined(uchars_inline_validate)

_Check_return_
extern STATUS /* STATUS_OK, error if not */
uchars_inline_validate(
    _In_z_      PCTSTR func,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline,
    _InVal_     U32 uchars_n)
{
    /*const*/ BOOL allow_inlines = TRUE;
    PC_U8 p_u8_in = (PC_U8) uchars_inline;
    PC_U8 p_u8 = p_u8_in;
    U32 n_bytes_so_far = 0;

    UNREFERENCED_PARAMETER_CONST(func);

    assert(strlen_without_NULLCH > uchars_n);

    while(n_bytes_so_far < uchars_n)
    {
        U32 bytes_of_char = 0;
        U8 u8;
        U8 mask;
        UCS4 ucs4 = 0;
        U8 second_byte_lower_limit = 0x80;
        U8 second_byte_upper_limit = 0xBF;

        /* skip inlines (any inline contents are not validated!) during general Fireworkz UTF-8 character sequence validation */
        if(allow_inlines && is_inline(p_u8) && ((n_bytes_so_far + INLINE_OVH) <= uchars_n))
        {
            bytes_of_char = inline_bytecount((PC_UCHARS_INLINE) p_u8);
            assert(bytes_of_char == (U32) inline_b_bytecount(uchars_inline_AddBytes(p_u8, bytes_of_char)));
            assert(inline_code(p_u8) == inline_b_code(uchars_inline_AddBytes(p_u8, bytes_of_char)));
            if(IL_TYPE_USTR == inline_code(p_u8))
            {
                PC_USTR ustr = inline_data_ptr(PC_USTR, p_u8);
                status_consume(ustr_validate(TEXT("uchars_inline_validate USTR "), ustr));
            }
            p_u8 += bytes_of_char;
            n_bytes_so_far += bytes_of_char;
            assert(n_bytes_so_far <= uchars_n); /* check for overlong inline */
            continue;
        }

        for(;;) /* loop for structure */
        {
            u8 = *p_u8++;

            mask = 0x80;
            if((u8 & mask) == 0x00)
            {   /* U+0000 : U+007F (7 bits) */
                ucs4 = u8;
                bytes_of_char = 1;
                if(CH_INLINE == u8)
                {
                    myassert5(TEXT("%s: CH_INLINE byte in sequence ") PTR_XTFMT TEXT("(%u) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
                    return(ERR_INVALID_UTF8_ENCODING);
                }
                break;
            }

            mask = 0xE0;
            if((u8 & mask) == 0xC0)
            {   /* U+0080 : U+07FF (11 bits) */
                bytes_of_char = 2;
                if(u8 <= 0xC1)
                {
                    myassert5(TEXT("%s: Not Well Formed UTF-8 - first byte fails in sequence ") PTR_XTFMT TEXT("(%u) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
                    return(ERR_INVALID_UTF8_ENCODING);
                }
                break;
            }

            mask = 0xF0;
            if((u8 & mask) == 0xE0)
            {   /* U+0800 : U+FFFF (16 bits) */
                bytes_of_char = 3;
                if(u8 == 0xE0)
                    second_byte_lower_limit = 0xA0;
                else if(u8 == 0xED)
                    second_byte_upper_limit = 0x9F;
                break;
            }

            mask = 0xF8;
            if((u8 & mask) == 0xF0)
            {   /* U+010000 : U+1FFFFF (21 bits) */
                bytes_of_char = 4;
                if(u8 == 0xF0)
                    second_byte_lower_limit = 0x90;
                else if(u8 == 0xF5)
                    second_byte_upper_limit = 0x8F;
                else if(u8 >= 0xF5)
                {
                    myassert5(TEXT("%s: Not Well Formed UTF-8 - first byte fails in sequence ") PTR_XTFMT TEXT("(%u) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
                    return(ERR_INVALID_UTF8_ENCODING);
                }
                break;
            }

            /* not UTF-8 encoding */
            myassert5(TEXT("%s: Not UTF-8 lead byte in sequence ") PTR_XTFMT TEXT("(%u) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
            return(ERR_INVALID_UTF8_ENCODING); /* end of loop for structure */ /*NOTREACHED*/
        }

        if((n_bytes_so_far + bytes_of_char) > uchars_n)
        {   /* UTF-8 lead byte indicates character encoding too long for this sequence */
            myassert4(TEXT("%s: UTF-8 char extends beyond sequence limits ") PTR_XTFMT TEXT("(%u) @ %u"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in));
            return(ERR_INVALID_UTF8_ENCODING);
        }

        if(bytes_of_char > 1)
        {
            U32 shift = (bytes_of_char - 1) * 6;

            ucs4 = ((UCS4) (u8 & ~mask)) << shift; /* N bits */

            /* always a second byte at this point - check that it is OK for well-formed UTF-8 */
            shift -= 6;
            u8 = *p_u8++;
            if(!u8_is_utf8_trail_byte(u8))
            {
                myassert5(TEXT("%s: Not UTF-8 trail byte in sequence ") PTR_XTFMT TEXT("(%u) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
                return(ERR_INVALID_UTF8_ENCODING);
            }
            if((u8 < second_byte_lower_limit) || (u8 > second_byte_upper_limit))
            {
                myassert5(TEXT("%s: Not Well Formed UTF-8 - second byte fails in sequence ") PTR_XTFMT TEXT("(%u) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
                return(ERR_INVALID_UTF8_ENCODING);
            }
            ucs4 |= ((UCS4) (u8 & ~0xC0)) << shift; /* 6 bits */

            while(shift != 0)
            {
                shift -= 6;
                u8 = *p_u8++;
                if(!u8_is_utf8_trail_byte(u8))
                {
                    myassert5(TEXT("%s: Not UTF-8 trail byte in sequence ") PTR_XTFMT TEXT("(%u) @ %u:0x%.2X"), func, p_u8_in, uchars_n, PtrDiffBytesU32((p_u8 - 1), p_u8_in), u8);
                    return(ERR_INVALID_UTF8_ENCODING);
                }
                ucs4 |= ((UCS4) (u8 & ~0xC0)) << shift; /* 6 bits */
            }
            while(shift != 0);
        }

        (void) ucs4_validate(ucs4);

        n_bytes_so_far += bytes_of_char;
    }

    return(STATUS_OK);
}

#endif /* !defined(uchars_inline_validate) */

/******************************************************************************
*
* case sensitive (binary) comparison of two counted UTF-8 character sequences
*
******************************************************************************/

#if TRACE_ALLOWED && 1
_Check_return_
static int
_utf8_compare(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b);

_Check_return_
extern int
utf8_compare(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b)
{
    int res = _utf8_compare(uchars_a, uchars_n_a, uchars_b, uchars_n_b);

    {
    TCHARZ tstr_buf_a[256];
    TCHARZ tstr_buf_b[256];

    U32 n_tstr_buf_a = tstr_buf_from_utf8str(tstr_buf_a, elemof32(tstr_buf_a), uchars_a, uchars_n_a);
    U32 n_tstr_buf_b = tstr_buf_from_utf8str(tstr_buf_b, elemof32(tstr_buf_b), uchars_b, uchars_n_b);

    tracef(TRACE_ANY, TEXT("utf8_compare %.*s(%d) %c %.*s(%d)"),
           (int) n_tstr_buf_a, tstr_buf_a, (int) uchars_n_a,
           ((res > 0) ? CH_GREATER_THAN_SIGN : (res == 0) ? CH_EQUALS_SIGN : CH_LESS_THAN_SIGN),
           (int) n_tstr_buf_b, tstr_buf_b, (int) uchars_n_b);
    } /*block*/

    return(res);
}

_Check_return_
static int
_utf8_compare(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a_in,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b_in,
    _InVal_     U32 uchars_n_b)
#else
_Check_return_
extern int
utf8_compare(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a_in,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b_in,
    _InVal_     U32 uchars_n_b)
#endif
{
    int res;
#if 1
    PC_U8 uchars_a = (PC_U8) uchars_a_in;
    PC_U8 uchars_b = (PC_U8) uchars_b_in;
    U32 limit = MIN(uchars_n_a, uchars_n_b);
    U32 i;
#else
    PC_UCHARS uchars_a = uchars_a_in;
    PC_UCHARS uchars_b = uchars_b_in;
    U32 i_a = 0;
    U32 i_b = 0;
#endif
    U32 remain_a, remain_b;

    profile_ensure_frame();

    assert(strlen_without_NULLCH > uchars_n_a); /* we will read to the bitter end - these are not valid */
    assert(strlen_without_NULLCH > uchars_n_b);

#if CHECKING_UTF8
    if(status_fail(utf8_validate(TEXT("utf8_compare arga"), uchars_a_in, uchars_n_a))) return(0);
    if(status_fail(utf8_validate(TEXT("utf8_compare argb"), uchars_b_in, uchars_n_b))) return(0);
#endif

#if 1 /* NB UTF-8 encoding has the very useful property that simple binary comparison works !!! */
    for(i = 0; i < limit; ++i)
    {
        U8 c_a = *uchars_a++;
        U8 c_b = *uchars_b++;

        res = (int) c_a - (int) c_b;

        if(0 != res)
            return(res);
    }

    /* matched up to the comparison limit */

    /* which counted sequence has the greater number of bytes left over? */
    remain_a = uchars_n_a - limit;
    remain_b = uchars_n_b - limit;
#else
    while((i_a < uchars_n_a) && (i_b < uchars_n_b))
    {
        UCS4 c_a = utf8_char_next(uchars_a, &uchars_a);
        UCS4 c_b = utf8_char_next(uchars_b, &uchars_b);

        i_a = PtrDiffBytesU32(uchars_a, uchars_a_in);
        i_b = PtrDiffBytesU32(uchars_b, uchars_b_in);

        assert(i_a <= uchars_n_a);
        assert(i_b <= uchars_n_b);

        res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-4 range */

        if(0 != res)
            return(res);
    }

    /* matched up to the comparison limit */

    /* which counted sequence has the greater number of bytes left over? */
    remain_a = uchars_n_a - i_a;
    remain_b = uchars_n_b - i_b;
#endif /* UTF-8 binary comparison */

    /*if(remain_a == remain_b)
        return(0);*/ /* ended together at the specified finite lengths -> equal */

    res = (int) remain_a - (int) remain_b;

    return(res);
}

/******************************************************************************
*
* case insensitive lexical comparison of two counted UTF-8 character sequences
*
******************************************************************************/

#if TRACE_ALLOWED && 1
_Check_return_
static int
_utf8_compare_nocase(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b);

_Check_return_
extern int
utf8_compare_nocase(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b)
{
    int res = _utf8_compare_nocase(uchars_a, uchars_n_a, uchars_b, uchars_n_b);

    {
    TCHARZ tstr_buf_a[256];
    TCHARZ tstr_buf_b[256];

    U32 n_tstr_buf_a = tstr_buf_from_utf8str(tstr_buf_a, elemof32(tstr_buf_a), uchars_a, uchars_n_a);
    U32 n_tstr_buf_b = tstr_buf_from_utf8str(tstr_buf_b, elemof32(tstr_buf_b), uchars_b, uchars_n_b);

    tracef(TRACE_ANY, TEXT("utf8_compare_nocase %.*s(%d) %c %.*s(%d)"),
           (int) n_tstr_buf_a, tstr_buf_a, (int) uchars_n_a,
           ((res > 0) ? CH_GREATER_THAN_SIGN : (res == 0) ? CH_EQUALS_SIGN : CH_LESS_THAN_SIGN),
           (int) n_tstr_buf_b, tstr_buf_b, (int) uchars_n_b);
    } /*block*/

    return(res);
}

_Check_return_
static int
_utf8_compare_nocase(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b)
#else
_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
utf8_compare_nocase(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b)
#endif
{
    int res;
    U32 i_a = 0;
    U32 i_b = 0;
    U32 remain_a, remain_b;

    profile_ensure_frame();

    assert(strlen_without_NULLCH > uchars_n_a); /* we will read to the bitter end - these are not valid */
    assert(strlen_without_NULLCH > uchars_n_b);

#if CHECKING_UTF8
    if(status_fail(utf8_validate(TEXT("utf8_compare_nocase arga"), uchars_a, uchars_n_a))) return(0);
    if(status_fail(utf8_validate(TEXT("utf8_compare_nocase argb"), uchars_b, uchars_n_b))) return(0);
#endif

    while((i_a < uchars_n_a) && (i_b < uchars_n_b))
    {
        U32 bytes_of_char_a, bytes_of_grapheme_cluster_a;
        UCS4 c_a;

        U32 bytes_of_char_b, bytes_of_grapheme_cluster_b;
        UCS4 c_b;

#if 1 /* quick speedup for runs of equal ASCII-7 characters */
        {
        U8 u8_c_a, u8_c_b;

        u8_c_a = PtrGetByte(uchars_a);

        if(u8_is_ascii7(u8_c_a) && (u8_c_a == (u8_c_b = PtrGetByte(uchars_b))))
        {
            ++i_a;
            ++i_b;
            utf8_IncBytes(uchars_a, 1);
            utf8_IncBytes(uchars_b, 1);
            continue;
        }
        } /*block*/
#endif /* quick speedup */

        c_a = utf8_grapheme_cluster_decode(uchars_a, uchars_n_a - i_a, /*ref*/bytes_of_char_a, /*ref*/bytes_of_grapheme_cluster_a);
        c_b = utf8_grapheme_cluster_decode(uchars_b, uchars_n_b - i_b, /*ref*/bytes_of_char_b, /*ref*/bytes_of_grapheme_cluster_b);

        utf8_IncBytes(uchars_a, bytes_of_grapheme_cluster_a);
        i_a += bytes_of_grapheme_cluster_a;

        utf8_IncBytes(uchars_b, bytes_of_grapheme_cluster_b);
        i_b += bytes_of_grapheme_cluster_b;

        assert(i_a <= uchars_n_a);
        assert(i_b <= uchars_n_b);

        res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-4 range */

        if(0 != res)
        {   /* retry with case folding */
            c_a = ucs4_case_fold_simple(c_a);
            c_b = ucs4_case_fold_simple(c_b);

            res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-4 range */

            if(0 != res)
            {   /* a pedant writes - here we need to consider sort order! */
                /* but for now this will have to do... */
                return(res);
            }
        }
    }

    /* matched up to the comparison limit */

    /* which counted sequence has the greater number of bytes left over? */
    remain_a = uchars_n_a - i_a;
    remain_b = uchars_n_b - i_b;

    /*if(remain_a == remain_b)
        return(0);*/ /* ended together at the specified finite lengths -> equal (even if matched length differs) */

    res = (int) remain_a - (int) remain_b;

    return(res);
}

/*
simpler variant that doesn't require result to reflect sort order, just inequality
*/

_Check_return_
extern BOOL /* FALSE:NEQ, TRUE:EQ */
utf8_compare_equals_nocase(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b)
{
    BOOL res;
    U32 i_a = 0;
    U32 i_b = 0;
    U32 remain_a, remain_b;

    profile_ensure_frame();

    assert(strlen_without_NULLCH > uchars_n_a); /* we will read to the bitter end - these are not valid */
    assert(strlen_without_NULLCH > uchars_n_b);

#if CHECKING_UTF8
    if(status_fail(utf8_validate(TEXT("utf8_compare_nocase_neq arga"), uchars_a, uchars_n_a))) return(0);
    if(status_fail(utf8_validate(TEXT("utf8_compare_nocase_neq argb"), uchars_b, uchars_n_b))) return(0);
#endif

    while((i_a < uchars_n_a) && (i_b < uchars_n_b))
    {
        U32 bytes_of_char_a, bytes_of_grapheme_cluster_a;
        UCS4 c_a = utf8_grapheme_cluster_decode(uchars_a, uchars_n_a - i_a, /*ref*/bytes_of_char_a, /*ref*/bytes_of_grapheme_cluster_a);

        U32 bytes_of_char_b, bytes_of_grapheme_cluster_b;
        UCS4 c_b = utf8_grapheme_cluster_decode(uchars_b, uchars_n_b - i_b, /*ref*/bytes_of_char_b, /*ref*/bytes_of_grapheme_cluster_b);

        utf8_IncBytes(uchars_a, bytes_of_grapheme_cluster_a);
        i_a += bytes_of_grapheme_cluster_a;

        utf8_IncBytes(uchars_b, bytes_of_grapheme_cluster_b);
        i_b += bytes_of_grapheme_cluster_b;

        res = (c_a != c_b);

        if(!res)
        {   /* retry with case folding */
            c_a = ucs4_case_fold_simple(c_a);
            c_b = ucs4_case_fold_simple(c_b);

            res = (c_a != c_b);

            if(!res)
                return(FALSE); /* not equal character */
        }
    }

    /* matched up to the comparison limit */

    /* unequal number of bytes left over? */
    remain_a = uchars_n_a - i_a;
    remain_b = uchars_n_b - i_b;

    /*if(remain_a == remain_b)
        return(TRUE);*/ /* ended together at the specified finite lengths -> equal (even if matched length differs) */

    res = (remain_a == remain_b);

    return(res);
}

/******************************************************************************
*
* case insensitive lexical comparison of
* a case folded counted UCS-4 character sequence and
* a counted UTF-8 character sequence
*
******************************************************************************/

_Check_return_
extern int
utf8_compare_nocase_CF1(
    _In_reads_(uchars_n_a) PC_UCS4 p_ucs4_a,
    _InVal_     U32 uchars_n_a /*elements*/,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b /*bytes*/)
{
    int res;
    U32 i_a = 0;
    U32 i_b = 0;
    U32 remain_a, remain_b;

    profile_ensure_frame();

    assert(strlen_without_NULLCH > uchars_n_a); /* we will read to the bitter end - these are not valid */
    assert(strlen_without_NULLCH > uchars_n_b);

#if CHECKING_UTF8
    if(status_fail(utf8_validate(TEXT("utf8_compare_nocase_CF1 argb"), uchars_b, uchars_n_b))) return(0);
#endif

    while((i_a < uchars_n_a) && (i_b < uchars_n_b))
    {
        UCS4 c_a = p_ucs4_a[i_a++];

        U32 bytes_of_char_b, bytes_of_grapheme_cluster_b;
        UCS4 c_b = utf8_grapheme_cluster_decode(uchars_b, uchars_n_b - i_b, /*ref*/bytes_of_char_b, /*ref*/bytes_of_grapheme_cluster_b);

        utf8_IncBytes(uchars_b, bytes_of_grapheme_cluster_b);
        i_b += bytes_of_grapheme_cluster_b;

        res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-4 range */

        if(0 != res)
        {   /* retry with case folding */
            c_b = ucs4_case_fold_simple(c_b);

            res = (int) c_a - (int) c_b; /* NB can't overflow given valid UCS-4 range */

            if(0 != res)
            {   /* a pedant writes - here we need to consider sort order! */
                /* but for now this will have to do... */
                return(res);
            }
        }
    }

    /* matched up to the comparison limit */

    /* which counted sequence has the greater number of elements / bytes left over? */
    remain_a = uchars_n_a - i_a;
    remain_b = uchars_n_b - i_b;

    /*if(remain_a == remain_b)
        return(0);*/ /* ended together at the specified finite lengths -> equal (even if matched length differs) */

    res = (int) remain_a - (int) remain_b;

    return(res);
}

/******************************************************************************
*
* this routine is a quick hack from PD3 stringcmp
* uses wild ^? single and ^# multiple, ^^ == ^
* needs rewriting long-term for generality!
*
* leading and trailing spaces are insignificant
*
******************************************************************************/

_Check_return_
extern int
utf8_compare_nocase_wild(
    _In_reads_(uchars_n_a) PC_UTF8 uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UTF8 uchars_b,
    _InVal_     U32 uchars_n_b)
{
    PC_U8 ptr1 = (PC_U8) uchars_a;
    PC_U8 ptr2 = (PC_U8) uchars_b;
    PC_U8 end_y = ptr1 + uchars_n_a;
    PC_U8 end_x = ptr2 + uchars_n_b;
    PC_U8 x;
    PC_U8 y;
    PC_U8 ox;
    PC_U8 oy;
    U8 ch;
    int wild_x;
    int pos_res;

    profile_ensure_frame();

    assert(strlen_without_NULLCH > uchars_n_a); /* we will read to the bitter end - these are not valid */
    assert(strlen_without_NULLCH > uchars_n_b);

#if CHECKING_UTF8
    if(status_fail(utf8_validate(TEXT("utf8_compare_nocase_wild arga"), uchars_a, uchars_n_a))) return(0);
    if(status_fail(utf8_validate(TEXT("utf8_compare_nocase_wild argb"), uchars_b, uchars_n_b))) return(0);
#endif

    /* skip leading and trailing spaces */
    while((ptr1 < end_y) && (CH_SPACE == PtrGetByte(ptr1)))
        PtrIncByte(PC_U8, ptr1);

    while((end_y > ptr1) && (PtrGetByteOff(end_y, -1) == CH_SPACE))
        PtrDecByte(PC_U8, end_y);

    y = ptr1;

    /* skip leading and trailing spaces in template sequence */
    while((ptr2 < end_x) && (CH_SPACE == PtrGetByte(ptr2)))
        PtrIncByte(PC_U8, ptr2);

    while((end_x > ptr2) && (PtrGetByteOff(end_x, -1) == CH_SPACE))
        PtrDecByte(PC_U8, end_x);

    /* must skip leading hilites in template sequence for final rejection */
    x = ptr2 - 1;

STAR:
    /* skip a char in template sequence */
    ch = *++x;
    /*trace_1(0, "utf8_compare_nocase_wild STAR (x skipped): x -> '%s'", x);*/

    wild_x = (ch == CH_CIRCUMFLEX_ACCENT);
    if(wild_x)
        ++x;

    oy = y;

    /* loop1: */
    for(;;)
    {
        /* skip a char in second sequence */
        ch = *++oy;
        /*trace_1(0, "utf8_compare_nocase_wild loop1 (oy skipped): oy -> '%s'", oy);*/

        ox = x;

        /* loop3: */
        for(;;)
        {
            if(wild_x)
                switch(*x)
                {
                case CH_NUMBER_SIGN:
                    /*trace_0(0, "utf8_compare_nocase_wild loop3: ^# found in first sequence: goto STAR to skip it & hilites");*/
                    goto STAR;

                case CH_CIRCUMFLEX_ACCENT:
                    /*trace_0(0, "utf8_compare_nocase_wild loop3: ^^ found in first sequence: match as ^");*/
                    wild_x = FALSE;

                default:
                    break;
                }

            /*trace_3(0, "utf8_compare_nocase_wild loop3: x -> '%s', y -> '%s', wild_x %s", x, y, report_boolstring(wild_x));*/

            /* are we at end of y sequence? */
            if(y == end_y)
            {
                /*trace_1(0, "utf8_compare_nocase_wild: end of y sequence: returns " S32_FMT, (*x == CH_NULL) ? 0 : -1);*/
                if(x == end_x)
                    return(0);       /* equal */
                else
                    return(-1);      /* first is bigger */
            }

            /* see if characters at x and y match */
            if(u8_is_ascii7(*y) && u8_is_ascii7(*x))
                pos_res = (int) /*ascii7*/sbchar_sortbyte(*y) - (int) /*ascii7*/sbchar_sortbyte(*x);
            else
                pos_res = (int) *y - (int) *x;

            if(pos_res)
            {   /* single character wildcard at x? */
                if(!wild_x  ||  (*x != CH_QUOTATION_MARK)  ||  (*y == CH_SPACE))
                {
                    y = oy;
                    x = ox;

                    if(x == ptr2)
                    {
                        /*trace_1(0, "utf8_compare_nocase_wild: returns " S32_FMT, pos_res);*/
                        return(pos_res);
                    }

                    /*trace_0(0, "utf8_compare_nocase_wild: chars differ: restore ptrs & break to loop1");*/
                    break;
                }
            }

            /* characters at x and y match, so increment x and y */
            /*trace_0(0, "utf8_compare_nocase_wild: chars at x & y match: ++x, ++y & keep in loop3");*/
            ch = *++x;

            wild_x = (ch == CH_CIRCUMFLEX_ACCENT);
            if(wild_x)
                ++x;

            ch = *++y;
        }
    }
}

#if !USTR_IS_SBSTR

_Check_return_
extern STATUS
utf8_case_fold(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n)
{
    STATUS status = STATUS_OK;
    U32 n_bytes_so_far = 0;

#if CHECKING_UTF8
    status_return(utf8_validate(TEXT("utf8_case_fold arg"), uchars, uchars_n));
#endif

    assert(strlen_without_NULLCH >uchars_n);

    while(n_bytes_so_far < uchars_n)
    {
        PC_UTF8 this_p_grapheme_cluster = utf8_AddBytes(uchars, n_bytes_so_far);
        U32 bytes_of_char, this_bytes_of_grapheme_cluster;
        UCS4 ucs4_in = utf8_grapheme_cluster_decode(this_p_grapheme_cluster, uchars_n - n_bytes_so_far, /*ref*/bytes_of_char, /*ref*/this_bytes_of_grapheme_cluster);
        UCS4 ucs4_out = ucs4_case_fold_simple(ucs4_in);
        UTF8B utf8_buffer[8];

        bytes_of_char = utf8_char_encode(utf8_bptr(utf8_buffer), elemof32(utf8_buffer), ucs4_out);

        status_break(status = quick_ublock_uchars_add(p_quick_ublock, uchars_bptr(utf8_buffer), bytes_of_char));

        n_bytes_so_far += this_bytes_of_grapheme_cluster;
    }

    return(status);
}

#endif /* USTR_IS_SBSTR */

/******************************************************************************
*
* convert to UTF-8 character sequence buffer from a counted TCHARs sequence
*
* NB not necessarily CH_NULL-terminated
*
******************************************************************************/

/*ncr*/
extern U32
utf8_from_tchars(
    _Out_opt_cap_(elemof_buffer) P_UTF8 uchars_buf,
    _InVal_     U32 elemof_buffer,
    _In_reads_(tchars_n) PCTCH tchars,
    _InVal_     U32 tchars_n)
{
    U32 bytes_output = 0;

#if TSTR_IS_SBSTR
    UTF8B utf8_buffer[8];
    U32 bytes_remain = elemof_buffer;
    U32 i;

    for(i = 0; i < tchars_n; ++i)
    {
        UCS4 ucs4 = (UCS4) tchars[i];
        U32 bytes_of_char = utf8_char_encode(utf8_bptr(utf8_buffer), sizeof32(utf8_buffer), ucs4);
        if(bytes_remain < bytes_of_char)
            break;
        bytes_remain -= bytes_of_char;
        utf8_char_copy(utf8_AddBytes_wr(uchars_buf, bytes_output), utf8_bptr(utf8_buffer), bytes_of_char);
        bytes_output += bytes_of_char;
    }
#elif WINDOWS
    int multi_n;
    assert(strlen_with_NULLCH != tchars_n); /* wouldn't work in the TSTR_IS_SBSTR case above */

    multi_n =
        WideCharToMultiByte(CP_UTF8 /*DestCodePage*/, 0 /*dwFlags*/,
                            tchars, (int) tchars_n,
                            (PSTR) uchars_buf, (int) elemof_buffer,
                            NULL /*pDefaultChar*/, NULL /*pUsedDefaultChar*/);
    assert(multi_n >= 0);
    if(0 == multi_n) return(0);

    bytes_output = multi_n;
#else
#error No implementation of utf8_from_tchars_convert
#endif /* OS */

#if CHECKING_UTF8
    if((NULL != uchars_buf) && status_fail(utf8_validate(TEXT("utf8_from_tchars_convert buf"), uchars_buf, bytes_output)))
        return(0);
#endif

    return(bytes_output);
}

_Check_return_
extern U32
sbchars_from_utf8_bytes_needed(
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n,
    _OutRef_    P_BOOL is_pure_ascii7)
{
    U32 bytes_so_far = 0;
    U32 bytes_needed = 0;

    *is_pure_ascii7 = TRUE;

    while(bytes_so_far < uchars_n)
    {
        U32 bytes_of_char;
        U8 u8 = PtrGetByteOff(uchars, bytes_so_far);

        if(u8_is_ascii7(u8))
        {
            bytes_of_char = 1;

            bytes_needed++;

            /* DO include any CH_NULL found on a counting pass, DON'T stop */
        }
        else
        {
            bytes_of_char = utf8_bytes_of_char_off(uchars, bytes_so_far);

            *is_pure_ascii7 = FALSE; /* conversion will be needed */

            bytes_needed++; /* single byte mapping forced */
        }

        bytes_so_far += bytes_of_char;
    }

    return(bytes_needed);
}

_Check_return_
extern U32
sbchars_from_utf8(
    _Out_writes_opt_(elemof_buffer) P_SBCHARS sbchars_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage,
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n)
{
    U32 bytes_so_far = 0;
    U32 sbchars_output = 0;

    if(NULL == sbchars_buf)
    {
        BOOL is_pure_ascii7;
        return(sbchars_from_utf8_bytes_needed(uchars, uchars_n, &is_pure_ascii7));
    }

    while(bytes_so_far < uchars_n)
    {
        U32 bytes_of_char;
        U8 u8 = PtrGetByteOff(uchars, bytes_so_far);

        if(u8_is_ascii7(u8))
        {
            bytes_of_char = 1;

            sbchars_buf[sbchars_output++] = u8;

            /* DO include any CH_NULL found on a conversion pass, DON'T stop */
        }
        else
        {
            PC_UTF8 p_char = utf8_AddBytes(uchars, bytes_so_far);
            UCS4 ucs4 = utf8_char_decode(p_char, /*ref*/bytes_of_char);

            if(ucs4_is_ascii7(ucs4))
                u8 = (U8) ucs4;
            else
            {
                assert(!ucs4_is_C1(ucs4)); /* hopefully no C1 to complicate things */
                u8 = (U8) ucs4_to_sbchar_force_with_codepage(ucs4, sbchar_codepage, CH_QUESTION_MARK);
            }

            sbchars_buf[sbchars_output++] = u8;
        }

        if(sbchars_output >= elemof_buffer)
        {   /* run out of space (NB no explicit CH_NULL-termination) */
            assert(sbchars_output == elemof_buffer);
            return(elemof_buffer); /* SBCHARs currently in output */
        }

        bytes_so_far += bytes_of_char;
    }

    return(sbchars_output); /* SBCHARs currently in output */
}

/* end of utf8.c */
