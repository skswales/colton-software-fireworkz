/* allocblk.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module to allocate one-shot blocks of memory, disposed of en masse when owning DOCU is freed */

/* SKS December 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*extern*/ P_ALLOCBLOCK global_string_alloc_block;

/*
macro used to align allocations as desired
*/

#define ALLOCBLOCK_ROUNDUP(x) ( \
    ((x) + (sizeof32(U32) - 1U)) & ~(sizeof32(U32) - 1U) )

/******************************************************************************
*
* start off the chain of core with an initial allocation
*
******************************************************************************/

_Check_return_
extern STATUS
alloc_block_create(
    _OutRef_    P_P_ALLOCBLOCK lplpAllocBlock,
    _InVal_     U32 n_bytes_wanted)
{
    STATUS status;
    P_ALLOCBLOCK lpAllocBlock;
    const U32 n_bytes_alloc = ALLOCBLOCK_ROUNDUP(n_bytes_wanted);

    if(NULL != (*lplpAllocBlock = lpAllocBlock = al_ptr_alloc_bytes(P_ALLOCBLOCK, n_bytes_alloc, &status)))
    {
        lpAllocBlock->next = NULL;
        lpAllocBlock->hwm = ALLOCBLOCK_ROUNDUP(sizeof32(*lpAllocBlock));
        lpAllocBlock->size = n_bytes_alloc;
    }

    return(status);
}

/******************************************************************************
*
* blow away the chain of core
*
******************************************************************************/

__pragma(warning(push)) __pragma(warning(disable:6001)) /* Using uninitialized memory '*lpAllocBlock' */

extern void
alloc_block_dispose(
    _InoutRef_  P_P_ALLOCBLOCK lplpAllocBlock)
{
    P_ALLOCBLOCK lpAllocBlock = *lplpAllocBlock;

    *lplpAllocBlock = NULL;

    while(NULL != lpAllocBlock)
    {
        P_ALLOCBLOCK next_lpAllocBlock = lpAllocBlock->next;
        al_ptr_free(lpAllocBlock);
        lpAllocBlock = next_lpAllocBlock;
    }
}

__pragma(warning(pop))

/******************************************************************************
*
* attempt to allocate some memory within the current block of core
*
* if this fails, try adding some more core to the chain
* this does in fact waste a little at the end of each block
* but that's the price you pay for simplicity
*
******************************************************************************/

_Check_return_
_Ret_writes_to_maybenull_(n_bytes_requested, 0) /* may be NULL */
extern P_BYTE
alloc_block_malloc(
    _InoutRef_  P_P_ALLOCBLOCK lplpAllocBlock,
    _InVal_     U32 n_bytes_requested,
    _OutRef_    P_STATUS p_status)
{
    P_ALLOCBLOCK lpAllocBlock = *lplpAllocBlock;
    const U32 n_bytes_alloc = ALLOCBLOCK_ROUNDUP(n_bytes_requested);
    U32 bytes_left = lpAllocBlock->size - lpAllocBlock->hwm;

    if(bytes_left < n_bytes_alloc)
    {
        /* try to get another chunk of core from the system and add it to the HEAD of the list (which is why we need the inout parameter) */
        U32 new_block_size = lpAllocBlock->size;
        P_ALLOCBLOCK new_lpAllocBlock;

        if(NULL == (new_lpAllocBlock = al_ptr_alloc_bytes(P_ALLOCBLOCK, new_block_size, p_status)))
            return(NULL);

        new_lpAllocBlock->next = lpAllocBlock;
        new_lpAllocBlock->hwm = ALLOCBLOCK_ROUNDUP(sizeof32(*new_lpAllocBlock));
        new_lpAllocBlock->size = new_block_size;

        *lplpAllocBlock = new_lpAllocBlock;
        lpAllocBlock = new_lpAllocBlock;
    }

    { /* trivial allocation within current block */
    P_BYTE p = (P_BYTE) lpAllocBlock;
    p += lpAllocBlock->hwm;
    lpAllocBlock->hwm += n_bytes_alloc;
    *p_status = STATUS_OK;
    return(p);
    } /*block*/
}

/******************************************************************************
*
* do a string assignment without having to think about cleanup
*
******************************************************************************/

_Check_return_
extern STATUS
alloc_block_ustr_set(
    _OutRef_    P_P_USTR aa,
    _In_z_      PC_USTR b,
    _InoutRef_  P_P_ALLOCBLOCK lplpAllocBlock)
{
    STATUS status;
    P_USTR a;
    U32 l;

    *aa = NULL;

    if(NULL == b)
        return(STATUS_OK);

    l = ustrlen32p1(b);

#if CHECKING
    if(contains_inline(b, l - 1))
    {
        assert0();
        /* "<<alloc_block_ustr_set - CONTAINS INLINES>>" */
        l = ustr_inline_strlen32p1((PC_USTR_INLINE) b); /*CH_NULL*/
    }
#endif

    if(!*lplpAllocBlock)
        status_return(alloc_block_create(lplpAllocBlock, 0x0800 - sizeof32(ALLOCBLOCK)));

    if(NULL == (*aa = a = (P_USTR) alloc_block_malloc(lplpAllocBlock, l, &status)))
        return(status);

    memcpy32(a, b, l);
    return(STATUS_DONE);
}

_Check_return_
extern STATUS
alloc_block_tstr_set(
    _OutRef_    P_PTSTR aa,
    _In_z_      PCTSTR b,
    _InoutRef_  P_P_ALLOCBLOCK lplpAllocBlock)
{
    STATUS status;
    PTSTR a;
    U32 l;

    *aa = NULL;

    if(NULL == b)
        return(STATUS_OK);

    l = tstrlen32p1(b);

    if(!*lplpAllocBlock)
        status_return(alloc_block_create(lplpAllocBlock, 0x0800 - sizeof32(ALLOCBLOCK)));

    if(NULL == (*aa = a = (PTCH) alloc_block_malloc(lplpAllocBlock, l * sizeof32(*a), &status)))
        return(status);

    memcpy32(a, b, l * sizeof32(*a));
    return(STATUS_DONE);
}

/* end of allocblk.c */
