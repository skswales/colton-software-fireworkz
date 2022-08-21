/* allocblk.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header file for allocblk.c */

/* SKS December 1992 */

#ifndef __allocblk_h
#define __allocblk_h

typedef struct ALLOCBLOCK /* really private to implementation, but stop old ARM compiler barfing over debug info */
{
    struct ALLOCBLOCK * next;
    U32 hwm;
    U32 size;
#if !defined(_WIN64)
    U32 _padding;
#endif
}
ALLOCBLOCK, * P_ALLOCBLOCK, ** P_P_ALLOCBLOCK;

#define ALLOCBLOCK_OVH 16 /* sizeof32(ALLOCBLOCK) */

/* each DOCU has a general_string_alloc_block but there are a few *limited* occasions where this is useful, e.g. fontmap, fileutil */
extern P_ALLOCBLOCK global_string_alloc_block;

_Check_return_
extern STATUS
alloc_block_create(
    _OutRef_    P_P_ALLOCBLOCK lplpAllocBlock,
    _InVal_     U32 n_bytes);

extern void
alloc_block_dispose(
    _InoutRef_  P_P_ALLOCBLOCK lplpAllocBlock);

_Check_return_
_Ret_writes_to_maybenull_(n_bytes_requested, 0) /* may be NULL */
extern P_BYTE
alloc_block_malloc(
    _InoutRef_  P_P_ALLOCBLOCK lplpAllocBlock,
    _InVal_     U32 n_bytes_requested,
    _OutRef_    P_STATUS p_status);

_Check_return_
extern STATUS
alloc_block_ustr_set(
    _OutRef_    P_P_USTR aa,
    _In_z_      PC_USTR b,
    _InoutRef_  P_P_ALLOCBLOCK lplpAllocBlock);

_Check_return_
extern STATUS
alloc_block_tstr_set(
    _OutRef_    P_PTSTR aa,
    _In_z_      PCTSTR b,
    _InoutRef_  P_P_ALLOCBLOCK lplpAllocBlock);

#endif /* __allocblk_h */

/* end of allocblk.h */
