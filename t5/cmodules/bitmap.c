/* bitmap.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Bitmap handling module */

/* MRJC January 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/******************************************************************************
*
* AND two bitmaps together
*
******************************************************************************/

/*ncr*/
extern BOOL /* any bits set in result */
bitmap_and(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap_result,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    N_BITS_ARG_DECL(n_bits))
{
    BIT_NUMBER bit_number;
    BOOL any = FALSE;

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(n_words_for_n_bits));

    for(bit_number = 0; bit_number < n_bits; bit_number += BITMAP_BPW)
        if((*p_bitmap_result++ = (BITMAP_WORD) (*p_bitmap_1++ & *p_bitmap_2++)) != 0)
            any = TRUE;

    return(any);
}

/******************************************************************************
*
* any bits set in this bitmap ?
*
******************************************************************************/

_Check_return_
extern BOOL
bitmap_any(
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap,
    N_BITS_ARG_DECL(n_bits))
{
    BIT_NUMBER bit_number;

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(n_words_for_n_bits));

    for(bit_number = 0; bit_number < n_bits; bit_number += BITMAP_BPW)
        if(*p_bitmap++ != 0)
            return(TRUE);

    return(FALSE);
}

/******************************************************************************
*
* BIC two bitmaps together
*
******************************************************************************/

/*ncr*/
extern BOOL /* any bits set in result */
bitmap_bic(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap_result,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    N_BITS_ARG_DECL(n_bits))
{
    BIT_NUMBER bit_number;
    BOOL any = FALSE;

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(n_words_for_n_bits));

    for(bit_number = 0; bit_number < n_bits; bit_number += BITMAP_BPW)
        if((*p_bitmap_result++ = (BITMAP_WORD) (*p_bitmap_1++ & ~*p_bitmap_2++)) != 0)
            any = TRUE;

    return(any);
}

/******************************************************************************
*
* clear bit number n in this bitmap
*
******************************************************************************/

/* see header */

/******************************************************************************
*
* copy bit number n into this bitmap from another bitmap
*
******************************************************************************/

extern void
bitmap_bit_copy(
    _Inout_updates_(n_words_for_n_bits) P_BITMAP p_bitmap_mod,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap,
    _InVal_     BIT_NUMBER bit_number,
    N_BITS_ARG_DECL(n_bits))
{
    const U32 word = (U32) bit_number >> BITMAP_BPW_SHIFT;
    const int bit = ((int) bit_number) & (BITMAP_BPW - 1);
    const BITMAP_WORD mask = (BITMAP_WORD) (ONE_BIT << bit);

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(n_words_for_n_bits));
    assert(bit_number < n_bits);
    if(bit_number < n_bits)
    {
        if(p_bitmap[word] & mask)
            p_bitmap_mod[word] |=  mask;
        else
            p_bitmap_mod[word] &= ~mask;
    }
}

/******************************************************************************
*
* set bit number n in this bitmap
*
******************************************************************************/

/* see header */

/******************************************************************************
*
* test bit number n in this bitmap
*
******************************************************************************/

/* see header */

/******************************************************************************
*
* compare two bitmaps for equality
*
* --out--
* = 0 bitmaps same
* !=0 bitmaps different
*
******************************************************************************/

_Check_return_
extern BOOL
bitmap_compare(
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    N_BITS_ARG_DECL(n_bits))
{
    BIT_NUMBER bit_number;

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(n_words_for_n_bits));

    for(bit_number = 0; bit_number < n_bits; bit_number += BITMAP_BPW)
        if(*p_bitmap_1++ != *p_bitmap_2++)
            return(TRUE);

    return(FALSE);
}

/******************************************************************************
*
* count the number of bits set in a bitmap
*
******************************************************************************/

/* see header */

/******************************************************************************
*
* given a starting bit, return the next bit number set in a bitmap;
* returns -1 if no more bits set
*
* used to have a failure case: if enum started at a non word offset in a zero
* byte then it added BITMAP_BPW to the bit_number, thereby ignoring some bits
* of the second word. new code safer
*
******************************************************************************/

_Check_return_
extern BIT_NUMBER /* <0 no more bits set */
bitmap_next_bit(
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap,
    _In_        BIT_NUMBER bit_number,
    N_BITS_ARG_DECL(n_bits))
{
    UINT word_idx = (UINT) (bit_number >> BITMAP_BPW_SHIFT);
    UINT bit_idx = ((UINT) bit_number) & (BITMAP_BPW - 1);
#if ((BITMAP_BPW == 8) || (BITMAP_BPW == 16)) && 1
    /* SKS and PMF 21apr95 */
    U32 mask = (U32) 1 << bit_idx; /* limiting condition: mask must be bigger size than BITMAP_WORD */

    while(bit_number < n_bits)
    {
        BITMAP_WORD word = p_bitmap[word_idx++];

        /* combines both word == 0 and mask ended cases where we've shifted the mask into the next word */
        while(mask <= word)
        {
            if(mask & word)
                return(bit_number);

            mask <<= 1;
            bit_number += 1;
        }

        mask = 1;

        bit_number = (U32) word_idx << BITMAP_BPW_SHIFT; /* due to early exit from loop */
    }
#else
    BITMAP_WORD mask = (BITMAP_WORD) (ONE_BIT << bit_idx);

    while(bit_number < n_bits)
    {
        BITMAP_WORD word = p_bitmap[word_idx++];

        if(word)
        {
            do  {
                if(mask & word)
                    return(bit_number);

                mask <<= 1;
                bit_number += 1;
            }
            while(mask);
        }

        mask = 1;

        bit_number = (U32) word_idx << BITMAP_BPW_SHIFT; /* for safety */
    }
#endif

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(n_words_for_n_bits));

    return(-1);
}

_Check_return_
extern BIT_NUMBER /* <0 no more bits set */
bitmap_next_bit_in_both(
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    _In_        BIT_NUMBER bit_number,
    N_BITS_ARG_DECL(n_bits))
{
    UINT word_idx = (UINT) (bit_number >> BITMAP_BPW_SHIFT);
    UINT bit_idx = ((UINT) bit_number) & (BITMAP_BPW - 1);
#if ((BITMAP_BPW == 8) || (BITMAP_BPW == 16)) && 1
    /* SKS and PMF 21apr95 */
    U32 mask = (U32) 1 << bit_idx;

    while(bit_number < n_bits)
    {
        BITMAP_WORD word = (BITMAP_WORD) (p_bitmap_1[word_idx] & p_bitmap_2[word_idx]);

        ++word_idx;

        /* combine both p_bitmap[i] == 0 and mask ended cases where we've shifted the mask into the next word */
        while(mask <= word)
        {
            if(mask & word)
                return(bit_number);

            mask <<= 1;
            bit_number += 1;
        }

        mask = 1;

        bit_number = (U32) word_idx << BITMAP_BPW_SHIFT; /* due to early exit from loop */
    }
#else
    BITMAP_WORD mask = (BITMAP_WORD) (ONE_BIT << bit_idx);

    while(bit_number < n_bits)
    {
        BITMAP_WORD word = p_bitmap_1[word_idx] & p_bitmap_2[word_idx];

        ++word_idx;

        if(word)
        {
            do  {
                if(mask & word)
                    return(bit_number);

                mask <<= 1;
                bit_number += 1;
            }
            while(mask);
        }

        mask = 1;

        bit_number = (U32) word_idx << BITMAP_BPW_SHIFT; /* for safety */
    }
#endif

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(n_words_for_n_bits));

    return(-1);
}

/******************************************************************************
*
* complement a bitmap
*
******************************************************************************/

extern void
bitmap_not(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap_result,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap,
    N_BITS_ARG_DECL(n_bits))
{
    const BITMAP_SIZE bitmap_size = BITMAP_SIZE_FROM_BITS(n_bits);
    const BIT_NUMBER size_bits = BITMAP_BITS_FROM_SIZE(bitmap_size); /* round back up, >= n_bits */
    BIT_NUMBER bit_number;

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(n_words_for_n_bits));

    for(bit_number = 0; bit_number < n_bits; bit_number += BITMAP_BPW)
        *p_bitmap_result++ = (BITMAP_WORD) ~(*p_bitmap++);

    if(size_bits > n_bits)
    {
        /* clear out top bits of end word again */
        P_BITMAP p = --p_bitmap_result;

#if BITMAP_BPW == 8
        BITMAP_WORD mask = 0xFF;
#elif BITMAP_BPW == 16
        BITMAP_WORD mask = 0xFFFFU;
#elif BITMAP_BPW == 32
        BITMAP_WORD mask = 0xFFFFFFFFU;
#elif BITMAP_BPW == 64
        BITMAP_WORD mask = 0xFFFFFFFFFFFFFFFFU;
#endif
        mask >>= (size_bits - n_bits); /* guaranteed to clear top bits as BITMAP_WORD is an unsigned type */

        *p &= mask;
    }
}

/******************************************************************************
*
* OR together two bitmaps
*
******************************************************************************/

extern void
bitmap_or(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap_result,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    N_BITS_ARG_DECL(n_bits))
{
    BIT_NUMBER bit_number;

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(n_words_for_n_bits));

    for(bit_number = 0; bit_number < n_bits; bit_number += BITMAP_BPW)
        *p_bitmap_result++ = (BITMAP_WORD) (*p_bitmap_1++ | *p_bitmap_2++);
}

/******************************************************************************
*
* SET a bitmap to all ones, padding end word with zeros
*
******************************************************************************/

extern void
bitmap_set(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap,
    N_BITS_ARG_DECL(n_bits))
{
#if defined(CODE_ANALYSIS)
    const BITMAP_SIZE bitmap_size = n_words_for_n_bits;
#else
    const BITMAP_SIZE bitmap_size = BITMAP_SIZE_FROM_BITS(n_bits);
#endif
    const BIT_NUMBER size_bits = BITMAP_BITS_FROM_SIZE(bitmap_size); /* round back up, >= n_bits */
    BITMAP_WORD word = 0xFFFFFFFFU;
    U32 word_idx = 0;

    assert(0 != bitmap_size);
    if(0 != bitmap_size)
    {
        while(word_idx < (bitmap_size - 1))
            p_bitmap[word_idx++] = word;
            
        if(size_bits > n_bits)
            /* need to clear out top bits of the end word */
            word = (BITMAP_WORD) (word >> (size_bits - n_bits)); /* guaranteed to clear top bits as BITMAP_WORD is an unsigned type */

        p_bitmap[word_idx++] = word;
    }
}

/******************************************************************************
*
* TST two bitmaps together for early exit
*
******************************************************************************/

/* see header */

/******************************************************************************
*
* XOR together two bitmaps
*
******************************************************************************/

extern void
bitmap_xor(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap_result,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    N_BITS_ARG_DECL(n_bits))
{
    BIT_NUMBER bit_number;

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(n_words_for_n_bits));

    for(bit_number = 0; bit_number < n_bits; bit_number += BITMAP_BPW)
        *p_bitmap_result++ = (BITMAP_WORD) (*p_bitmap_1++ ^ *p_bitmap_2++);
}

/* end of bitmap.c */
