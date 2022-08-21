/* bitmap.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __bitmap_h
#define __bitmap_h

/* bitmap bits per word */
#if 0
/*#define BITMAP_BPW 8*/    /* rubbish 16-bit processors can't cope */
#elif RISCOS || (WINDOWS && 1)
#define BITMAP_BPW 32
#define FAST_BITMAP_OPS_32 1 /* fast 32-bit aligned bitmap ops */
#elif WINDOWS
#define BITMAP_BPW 64
#define FAST_BITMAP_OPS_64 1 /* fast 64-bit aligned bitmap ops */
#endif

#if     BITMAP_BPW    == 8
#define BITMAP_BPW_SHIFT 3
typedef U8               BITMAP_WORD;
#elif   BITMAP_BPW    == 16
#define BITMAP_BPW_SHIFT 4
typedef U16              BITMAP_WORD;
#elif   BITMAP_BPW    == 32
#define BITMAP_BPW_SHIFT 5
typedef U32              BITMAP_WORD;
#elif   BITMAP_BPW    == 64
#define BITMAP_BPW_SHIFT 6
typedef U64              BITMAP_WORD;
#endif

#define ONE_BIT          ((BITMAP_WORD) 1U)

typedef BITMAP_WORD * P_BITMAP; typedef const BITMAP_WORD * PC_BITMAP;

#define P_BITMAP_NONE _P_DATA_NONE(P_BITMAP)

typedef U32 BITMAP_SIZE;
typedef S32 BIT_NUMBER;

/* macro to define/declare a bitmap given number of bits required */
#define BITMAP(name, n_bits) \
    BITMAP_WORD name[BITMAP_SIZE_FROM_BITS(n_bits)]

/* macro to calculate size in bitmap words from bit count */
#define BITMAP_SIZE_FROM_BITS(n_bits) ( \
    ((BITMAP_SIZE) (n_bits) + (BITMAP_SIZE) (BITMAP_BPW - 1)) >> BITMAP_BPW_SHIFT )

#define BITMAP_BITS_FROM_SIZE(bitmap_size) ( \
    (BIT_NUMBER) (bitmap_size) << BITMAP_BPW_SHIFT )

#define N_BITS_ARG(n_bits) \
    n_bits \
    CODE_ANALYSIS_ONLY_ARG(BITMAP_SIZE_FROM_BITS(n_bits))

#define N_BITS_ARG_DECL(n_bits) \
    _InVal_ BIT_NUMBER n_bits \
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 n_words_for_n_bits)

/*ncr*/
extern BOOL
bitmap_and(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap_result,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    N_BITS_ARG_DECL(n_bits));

_Check_return_
extern BOOL
bitmap_any(
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap,
    N_BITS_ARG_DECL(n_bits));

/*ncr*/
extern BOOL
bitmap_bic(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap_result,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    N_BITS_ARG_DECL(n_bits));

static inline void
bitmap_bit_clear(
    _Inout_updates_(n_words_for_n_bits) P_BITMAP p_bitmap,
    _InVal_     BIT_NUMBER bit_number,
    N_BITS_ARG_DECL(n_bits))
{
    const U32 word = (U32) bit_number >> BITMAP_BPW_SHIFT;
    const int bit = ((int) bit_number) & (BITMAP_BPW - 1);
    const BITMAP_WORD mask = (BITMAP_WORD) (ONE_BIT << bit);

    CODE_ANALYSIS_ONLY(IGNOREPARM_InVal_(n_words_for_n_bits));
    assert(bit_number < n_bits);
    if(bit_number < n_bits)
        p_bitmap[word] &= ~mask;
}

extern void
bitmap_bit_copy(
    _Inout_updates_(n_words_for_n_bits) P_BITMAP p_bitmap_mod,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap,
    _InVal_     BIT_NUMBER bit_number,
    N_BITS_ARG_DECL(n_bits));

static inline void
bitmap_bit_set(
    _Inout_updates_(n_words_for_n_bits) P_BITMAP p_bitmap,
    _InVal_     BIT_NUMBER bit_number,
    N_BITS_ARG_DECL(n_bits))
{
    const U32 word = (U32) bit_number >> BITMAP_BPW_SHIFT;
    const int bit = ((int) bit_number) & (BITMAP_BPW - 1);
    const BITMAP_WORD mask = (BITMAP_WORD) (ONE_BIT << bit);

    CODE_ANALYSIS_ONLY(IGNOREPARM_InVal_(n_words_for_n_bits));
    assert(bit_number < n_bits);
    if(bit_number < n_bits)
        p_bitmap[word] |= mask;
}

_Check_return_
static inline BOOL
bitmap_bit_test(
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap,
    _InVal_     BIT_NUMBER bit_number,
    N_BITS_ARG_DECL(n_bits))
{
    /* refinement to avoid signed divide */
    const U32 word = (U32) bit_number >> BITMAP_BPW_SHIFT;
    const int bit = ((int) bit_number) & (BITMAP_BPW - 1);
    const BITMAP_WORD mask = (BITMAP_WORD) (ONE_BIT << bit);

    CODE_ANALYSIS_ONLY(IGNOREPARM_InVal_(n_words_for_n_bits));
    assert(bit_number < n_bits);
    if(bit_number < n_bits)
    {
        BITMAP_WORD result = (BITMAP_WORD) (p_bitmap[word] & mask);

        return((result != 0));
    }

    return(FALSE);
}

static inline void
bitmap_clear(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap,
    N_BITS_ARG_DECL(n_bits))
{
    CODE_ANALYSIS_ONLY(IGNOREPARM_InVal_(n_words_for_n_bits));

    memset32(p_bitmap, 0, BITMAP_SIZE_FROM_BITS(n_bits) * sizeof32(BITMAP_WORD));
}

_Check_return_
extern BOOL
bitmap_compare(
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    N_BITS_ARG_DECL(n_bits));

static inline void
bitmap_copy(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap_result,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap,
    N_BITS_ARG_DECL(n_bits))
{
    CODE_ANALYSIS_ONLY(IGNOREPARM_InVal_(n_words_for_n_bits));

    memcpy32(p_bitmap_result, p_bitmap, BITMAP_SIZE_FROM_BITS(n_bits) * sizeof32(BITMAP_WORD));
}

_Check_return_
static inline U32
bitmap_count(
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap,
    N_BITS_ARG_DECL(n_bits))
{
    BIT_NUMBER bit_number;
    U32 count = 0;

    CODE_ANALYSIS_ONLY(IGNOREPARM_InVal_(n_words_for_n_bits));

    for(bit_number = 0; bit_number < n_bits; bit_number += BITMAP_BPW)
    {
        BITMAP_WORD w = *p_bitmap++;

        while(w)
        {
            if(w & 1)
                ++count;

            w >>= 1;
        }
    }

    return(count);
}

_Check_return_
extern BIT_NUMBER /* <0 no more bits set */
bitmap_next_bit(
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap,
    _In_        BIT_NUMBER bit_number,
    N_BITS_ARG_DECL(n_bits));

_Check_return_
extern BIT_NUMBER /* <0 no more bits set */
bitmap_next_bit_in_both(
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    _In_        BIT_NUMBER bit_number,
    N_BITS_ARG_DECL(n_bits));

extern void
bitmap_not(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap_result,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap,
    N_BITS_ARG_DECL(n_bits));

extern void
bitmap_or(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap_result,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    N_BITS_ARG_DECL(n_bits));

extern void
bitmap_set(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap,
    N_BITS_ARG_DECL(n_bits));

_Check_return_
static inline BOOL
bitmap_test(
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    N_BITS_ARG_DECL(n_bits))
{
    BIT_NUMBER bit_number;

    CODE_ANALYSIS_ONLY(IGNOREPARM_InVal_(n_words_for_n_bits));

    for(bit_number = 0; bit_number < n_bits; bit_number += BITMAP_BPW)
        if((*p_bitmap_1++ & *p_bitmap_2++) != 0)
            return(TRUE);

    return(FALSE);
}

extern void
bitmap_xor(
    _Out_writes_all_(n_words_for_n_bits) P_BITMAP p_bitmap_result,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_1,
    _In_reads_(n_words_for_n_bits) PC_BITMAP p_bitmap_2,
    N_BITS_ARG_DECL(n_bits));

#endif /* __bitmap.h */

/* end of bitmap.h */
