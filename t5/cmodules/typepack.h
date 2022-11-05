/* typepack.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* External header file for module typepack.c */

/* MRJC November 1989 */

#ifndef __typepack_h
#define __typepack_h

/*#define TYPEPACK_EXPORT_ONLY_FUNCTIONS 1*/
/*#define TYPEPACK_EXPORT_ONLY_MACROS 1*/

#ifdef  TYPEPACK_EXPORT_ONLY_FUNCTIONS
#undef  TYPEPACK_EXPORT_ONLY_MACROS
#endif

#ifndef TYPEPACK_EXPORT_ONLY_MACROS

/*
exported functions
*/

/*
reading from memory (LE/BE/native)
*/

_Check_return_
extern F64
readval_F64_from_8087(
    _In_reads_bytes_c_(sizeof(F64)) PC_ANY from);

_Check_return_
extern F64
readval_F64_from_ARM(
    _In_reads_bytes_c_(sizeof(F64)) PC_ANY from);

_Check_return_
extern U16
readval_U16(
    _In_reads_bytes_c_(sizeof(U16)) PC_ANY from);

_Check_return_
extern U16
readval_U16_BE(
    _In_reads_bytes_c_(sizeof(U16)) PC_ANY from);

_Check_return_
extern U16
readval_U16_LE(
    _In_reads_bytes_c_(sizeof(U16)) PC_ANY from);

_Check_return_
extern U32
readval_U32(
    _In_reads_bytes_c_(sizeof(U32)) PC_ANY from);

_Check_return_
extern U32
readval_U32_BE(
    _In_reads_bytes_c_(sizeof(U32)) PC_ANY from);

_Check_return_
extern U32
readval_U32_LE(
    _In_reads_bytes_c_(sizeof(U32)) PC_ANY from);

_Check_return_
extern S16
readval_S16(
    _In_reads_bytes_c_(sizeof(S16)) PC_ANY from);

_Check_return_
extern S16
readval_S16_BE(
    _In_reads_bytes_c_(sizeof(S16)) PC_ANY from);

_Check_return_
extern S16
readval_S16_LE(
    _In_reads_bytes_c_(sizeof(S16)) PC_ANY from);

_Check_return_
extern S32
readval_S32(
    _In_reads_bytes_c_(sizeof(S32)) PC_ANY from);

_Check_return_
extern S32
readval_S32_BE(
    _In_reads_bytes_c_(sizeof(S32)) PC_ANY from);

_Check_return_
extern S32
readval_S32_LE(
    _In_reads_bytes_c_(sizeof(S32)) PC_ANY from);

/*
writing to memory (LE/BE/native)
*/

extern void
writeval_F64_as_8087(
    _Out_bytecapcount_x_(sizeof(F64)) P_ANY to,
    _InVal_     F64 f64);

extern void
writeval_F64_as_ARM(
    _Out_bytecapcount_x_(sizeof(F64)) P_ANY to,
    _InVal_     F64 f64);

extern void
writeval_U16(
    _Out_bytecapcount_x_(sizeof(U16)) P_ANY to,
    _InVal_     U16 u16);

extern void
writeval_U16_BE(
    _Out_bytecapcount_x_(sizeof(U16)) P_ANY to,
    _InVal_     U16 u16);

extern void
writeval_U16_LE(
    _Out_bytecapcount_x_(sizeof(U16)) P_ANY to,
    _InVal_     U16 u16);

extern void
writeval_U32(
    _Out_bytecapcount_x_(sizeof(U32)) P_ANY to,
    _InVal_     U32 u32);

extern void
writeval_U32_BE(
    _Out_bytecapcount_x_(sizeof(U32)) P_ANY to,
    _InVal_     U32 u32);

extern void
writeval_U32_LE(
    _Out_bytecapcount_x_(sizeof(U32)) P_ANY to,
    _InVal_     U32 u32);

extern void
writeval_S16(
    _Out_bytecapcount_x_(sizeof(S16)) P_ANY to,
    _InVal_     S16 s16);

extern void
writeval_S16_BE(
    _Out_bytecapcount_x_(sizeof(S16)) P_ANY to,
    _InVal_     S16 s16);

extern void
writeval_S16_LE(
    _Out_bytecapcount_x_(sizeof(S16)) P_ANY to,
    _InVal_     S16 s16);

extern void
writeval_S32(
    _Out_bytecapcount_x_(sizeof(S32)) P_ANY to,
    _InVal_     S32 s32);

extern void
writeval_S32_BE(
    _Out_bytecapcount_x_(sizeof(S32)) P_ANY to,
    _InVal_     S32 s32);

extern void
writeval_S32_LE(
    _Out_bytecapcount_x_(sizeof(S32)) P_ANY to,
    _InVal_     S32 s32);

#endif /* TYPEPACK_EXPORT_ONLY_MACROS */

#ifndef TYPEPACK_EXPORT_ONLY_FUNCTIONS

/*
functions as macros
*/

/*
reading from memory - assume unaligned access works, even if slower
*/

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define readval_U16_BE(from) (* (PC_U16) (from))
#define readval_U32_BE(from) (* (PC_U32) (from))
#define readval_S16_BE(from) (* (PC_S16) (from))
#define readval_S32_BE(from) (* (PC_S32) (from))

#define readval_U16(from) readval_U16_BE(from)
#define readval_U32(from) readval_U32_BE(from)
#define readval_S16(from) readval_S16_BE(from)
#define readval_S32(from) readval_S32_BE(from)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define readval_U16_LE(from) (* (PC_U16) (from))
#define readval_U32_LE(from) (* (PC_U32) (from))
#define readval_S16_LE(from) (* (PC_S16) (from))
#define readval_S32_LE(from) (* (PC_S32) (from))

#define readval_U16(from) readval_U16_LE(from)
#define readval_U32(from) readval_U32_LE(from)
#define readval_S16(from) readval_S16_LE(from)
#define readval_S32(from) readval_S32_LE(from)
#endif

#if RISCOS

/* Norcroft C short loading is poor; relies on current memory access and word/halfword alignment */

static inline U16
__inline_readval_U16_LE(
    _In_reads_bytes_c_(2) PC_ANY from)
{
    return(
        ((U16) (* (((PC_U8) (from)) + 0)) <<  0) |
        ((U16) (* (((PC_U8) (from)) + 1)) <<  8) );
}

#undef  readval_U16_LE
#define readval_U16_LE(from) __inline_readval_U16_LE(from)

static inline U16
__inline_readval_U16_BE(
    _In_reads_bytes_c_(2) PC_ANY from)
{
    return(
        ((U16) (* (((PC_U8) (from)) + 0)) <<  8) |
        ((U16) (* (((PC_U8) (from)) + 1)) <<  0) );
}

#undef  readval_U16_BE
#define readval_U16_BE(from) __inline_readval_U16_BE(from)

#undef  readval_U32_LE
#define readval_U32_LE(from) ( (U32) ( \
    ((U32) (* (((PC_U8) (from)) + 0)) <<  0) | \
    ((U32) (* (((PC_U8) (from)) + 1)) <<  8) | \
    ((U32) (* (((PC_U8) (from)) + 2)) << 16) | \
    ((U32) (* (((PC_U8) (from)) + 3)) << 24)   ) )

#undef  readval_U32_BE
#define readval_U32_BE(from) ( (U32) ( \
    ((U32) (* (((PC_U8) (from)) + 0)) << 24) | \
    ((U32) (* (((PC_U8) (from)) + 1)) << 16) | \
    ((U32) (* (((PC_U8) (from)) + 2)) <<  8) | \
    ((U32) (* (((PC_U8) (from)) + 3)) <<  0)   ) )

/* sign-extension required */
#undef  readval_S16_LE
#define readval_S16_LE(from) ( (S16) ( \
    ( (S32) (* (((PC_U8) (from)) + 0)) <<  0)        | \
    (((S32) (* (((PC_U8) (from)) + 1)) << 24) >> 16)   ) )

#undef  readval_S16_BE
#define readval_S16_BE(from) ( (S16) ( \
    (((S32) (* (((PC_U8) (from)) + 0)) << 24) >> 16) | \
    ( (S32) (* (((PC_U8) (from)) + 1)) <<  0)          ) )

#undef  readval_S32_LE
#define readval_S32_LE(from) ( (S32) ( \
    ((U32) (* (((PC_U8) (from)) + 0)) <<  0) | \
    ((U32) (* (((PC_U8) (from)) + 1)) <<  8) | \
    ((U32) (* (((PC_U8) (from)) + 2)) << 16) | \
    ((U32) (* (((PC_U8) (from)) + 3)) << 24)   ) )

#undef  readval_S32_BE
#define readval_S32_BE(from) ( (S32) ( \
    ((U32) (* (((PC_U8) (from)) + 0)) << 24) | \
    ((U32) (* (((PC_U8) (from)) + 1)) << 18) | \
    ((U32) (* (((PC_U8) (from)) + 2)) <<  8) | \
    ((U32) (* (((PC_U8) (from)) + 3)) <<  0)   ) )

#endif /* RISCOS */

/*
writing to memory - assume unaligned access works, even if slower
*/

/* used below for things that work ok by simple pointer op */
#define __writeval_generic(to, val, type) ( \
    * (type *) (to) = (val) )

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define writeval_U16_BE(to, u16) __writeval_generic(to, u16, U16)
#define writeval_U32_BE(to, u32) __writeval_generic(to, u32, U32)
#define writeval_S16_BE(to, s16) __writeval_generic(to, s16, S16)
#define writeval_S32_BE(to, s32) __writeval_generic(to, s32, S32)

#define writeval_U16(to, u16) writeval_U16_BE(to, u16)
#define writeval_U32(to, u32) writeval_U32_BE(to, u32)
#define writeval_S16(to, s16) writeval_S16_BE(to, s16)
#define writeval_S32(to, s32) writeval_S32_BE(to, s32)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define writeval_U16_LE(to, u16) __writeval_generic(to, u16, U16)
#define writeval_U32_LE(to, u32) __writeval_generic(to, u32, U32)
#define writeval_S16_LE(to, s16) __writeval_generic(to, s16, S16)
#define writeval_S32_LE(to, s32) __writeval_generic(to, s32, S32)

#define writeval_U16(to, u16) writeval_U16_LE(to, u16)
#define writeval_U32(to, u32) writeval_U32_LE(to, u32)
#define writeval_S16(to, s16) writeval_S16_LE(to, s16)
#define writeval_S32(to, s32) writeval_S32_LE(to, s32)
#endif

#if RISCOS

/* Norcroft C writes unaligned shorts OK on ARM using STRB,SHR,STRB */

/* Norcroft C can't yet be persuaded to write unaligned longs on ARM */

/* NB prevent sign extension */
struct __writeword32str
{
    U8 b0;
    U8 b1;
    U8 b2;
    U8 b3;
};

static inline void
__inline_writeval_U32_LE(
    _Out_bytecapcount_x_(sizeof(U32)) P_ANY to,
    _InVal_     U32 u32)
{
    ((struct __writeword32str *) to)->b0 = (U8) ((u32) >>  0);
    ((struct __writeword32str *) to)->b1 = (U8) ((u32) >>  8);
    ((struct __writeword32str *) to)->b2 = (U8) ((u32) >> 16);
    ((struct __writeword32str *) to)->b3 = (U8) ((u32) >> 24);
}

#undef  writeval_U32_LE
#define writeval_U32_LE __inline_writeval_U32_LE

static inline void
__inline_writeval_U32_BE(
    _Out_bytecapcount_x_(sizeof(U32)) P_ANY to,
    _InVal_     U32 u32)
{
    ((struct __writeword32str *) to)->b0 = (U8) ((u32) >> 24);
    ((struct __writeword32str *) to)->b1 = (U8) ((u32) >> 16);
    ((struct __writeword32str *) to)->b2 = (U8) ((u32) >>  8);
    ((struct __writeword32str *) to)->b3 = (U8) ((u32) >>  0);
}

#undef  writeval_U32_BE
#define writeval_U32_BE __inline_writeval_U32_BE

static inline void
__inline_writeval_S32_LE(
    _Out_bytecapcount_x_(sizeof(S32)) P_ANY to,
    _InVal_     S32 s32)
{
    ((struct __writeword32str *) to)->b0 = (U8) ((U32) (s32) >>  0);
    ((struct __writeword32str *) to)->b1 = (U8) ((U32) (s32) >>  8);
    ((struct __writeword32str *) to)->b2 = (U8) ((U32) (s32) >> 16);
    ((struct __writeword32str *) to)->b3 = (U8) ((U32) (s32) >> 24);
}

#undef  writeval_S32_LE
#define writeval_S32_LE __inline_writeval_S32_LE

static inline void
__inline_writeval_S32_BE(
    _Out_bytecapcount_x_(sizeof(S32)) P_ANY to,
    _InVal_     S32 s32)
{
    ((struct __writeword32str *) to)->b0 = (U8) ((U32) (s32) >> 24);
    ((struct __writeword32str *) to)->b1 = (U8) ((U32) (s32) >> 16);
    ((struct __writeword32str *) to)->b2 = (U8) ((U32) (s32) >>  8);
    ((struct __writeword32str *) to)->b3 = (U8) ((U32) (s32) >>  0);
}

#undef  writeval_S32_BE
#define writeval_S32_BE __inline_writeval_S32_BE

#endif /* RISCOS */

#endif /* TYPEPACK_EXPORT_ONLY_FUNCTIONS */

#endif /* __typepack_h */

/* end of typepack.h */
