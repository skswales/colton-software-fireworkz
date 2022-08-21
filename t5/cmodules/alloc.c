/* alloc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Allocation in an extensible flex block for RISC OS */

/* SKS 23-Aug-1989 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS

/*
set consistent set of flags for alloc.h
*/

#if defined(TRACE_MAIN_ALLOCS) || defined(TRACE_FIXED_ALLOCS)
#if !defined(CHECK_ALLOCS)
#define  CHECK_ALLOCS
#endif
#if !defined(TRACE_ALLOCS)
#define  TRACE_ALLOCS
#endif
#endif

#if defined(VALIDATE_MAIN_ALLOCS) || defined(VALIDATE_FIXED_ALLOCS)
#if !defined(CHECK_ALLOCS)
#define  CHECK_ALLOCS
#endif
#endif

#if defined(CHECK_ALLOCS) || defined(TRACE_ALLOCS)
#undef   TRACE_ALLOWED
#define  TRACE_ALLOWED 1
#include "debug.h"
#endif

/* various options */
/*defined(startguardsize)*/
/*defined(endguardsize)*/
/*defined(ALLOC_CLEAR_FREE)*/
/*defined(ALLOC_NOISE_THRESHOLD)*/
/*defined(ALLOC_RELEASE_STORE_INFREQUENTLY)*/
/*defined(ALLOC_TRACK_PROCESS_USE)*/
/*defined(ALLOC_TRACK_USAGE)*/
/*defined(USE_HEAP_SWIS)*/
/*defined(TEST_REALLOC_RARE_PROCESSES)*/
/*defined(VALIDATE_MAIN_ALLOCS_START)*/

/* untested options */
/*defined(REDIRECT_RISCOS_KERNEL_ALLOCS)*/

/*
internal definitions
*/

#define EXPOSE_RISCOS_FLEX 1
#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"

#define alloc_heap_fixed_size               0x00000001
#define alloc_heap_dont_compact             0x00000002
#define alloc_heap_compact_disabled         0x00000004

#define alloc_trace_off                     0x00000000
#define alloc_trace_on                      0x00000010

#define alloc_validate_off                  0x00000000
#define alloc_validate_on                   0x00000020

#define alloc_validate_disabled             0x00000000
#define alloc_validate_enabled              0x00000040

#define alloc_validate_heap_blocks          0x00000080

#define alloc_validate_heap_before_free     0x00010000 /* dispose, free */
#define alloc_validate_heap_before_alloc    0x00020000 /* calloc, malloc, realloc */
#define alloc_validate_heap_on_size         0x00040000 /* size */
#define alloc_validate_heap_after_free      0x00080000 /* dispose, free */
#define alloc_validate_heap_after_alloc     0x00100000 /* calloc, malloc */
#define alloc_validate_heap_after_realloc   0x00200000 /* realloc */

#define alloc_validate_block_before_free    0x01000000 /* dispose, free */
#define alloc_validate_block_before_realloc 0x02000000 /* realloc */
#define alloc_validate_block_on_size        0x04000000 /* size */
#define alloc_validate_block_after_alloc    0x08000000 /* calloc, malloc */
#define alloc_validate_block_after_realloc  0x10000000 /* realloc */

#define ALLOC_HEAP_FLAGS int

typedef struct ALLOC_HEAP_DESC
{
    struct RISCOS_HEAP *    heap;
    U32                     minsize;
    U32                     increment;
    ALLOC_HEAP_FLAGS        flags;
}
ALLOC_HEAP_DESC, * P_ALLOC_HEAP_DESC; typedef const ALLOC_HEAP_DESC * PC_ALLOC_HEAP_DESC;

/* Interface to RISC OS Heap Manager */

enum RISCOS_HEAPREASONCODES
{
    HeapReason_Init          = 0,
    HeapReason_Desc          = 1,
    HeapReason_Get           = 2,
    HeapReason_Free          = 3,
    HeapReason_ExtendBlock   = 4,
    HeapReason_ExtendHeap    = 5,
    HeapReason_ReadBlockSize = 6
};

typedef struct RISCOS_HEAP
{
    U32 magic;  /* ID word */
    U32 free;   /* offset to first block on free list ***from this location*** */
    U32 hwm;    /* offset to first free location */
    U32 size;   /* size of heap, including header */

                /* rest of heap follows here ... */
}
RISCOS_HEAP, * P_RISCOS_HEAP;

typedef struct RISCOS_USED_BLOCK
{
    U32 size;   /* rounded size of used block */

                /* data follows here ... */
}
RISCOS_USED_BLOCK, * P_RISCOS_USED_BLOCK;

/*
pointer to the block in which this object is allocated
*/
#define blockhdrp(core) ( \
    ((P_RISCOS_USED_BLOCK) (core)) - 1 )

/*
amount of core allocated to this object
*/
#define blocksize(core) ( \
    blockhdrp(core)->size - sizeof32(RISCOS_USED_BLOCK) )

typedef struct RISCOS_FREE_BLOCK
{
    U32 free;   /* offset to next block on free list ***from this location*** */
    U32 size;   /* size of free block */

                /* free space follows here ... */
}
RISCOS_FREE_BLOCK, * P_RISCOS_FREE_BLOCK;

/*
RISC OS only maintains size field on used blocks
*/
#define HEAPMGR_OVERHEAD    sizeof32(RISCOS_USED_BLOCK)

/* Round to integral number of RISC OS heap manager granules
 * This size is given by the need to fit a freeblock header into
 * any space that might be freed or fragmented on allocation.
*/
#define round_heapmgr(n) ( \
    ((n) + (sizeof32(RISCOS_FREE_BLOCK)-1)) & ~(sizeof32(RISCOS_FREE_BLOCK)-1) )

#if defined(CHECK_ALLOCS)
#define SG_FILL_BYTE 0xDDU
#define SG_FILL_WORD ( \
    (((((SG_FILL_BYTE << 8) | SG_FILL_BYTE) << 8) | SG_FILL_BYTE) << 8) | SG_FILL_BYTE )
#define EG_FILL_BYTE 0xEEU
#define EG_FILL_WORD ( \
    (((((EG_FILL_BYTE << 8) | EG_FILL_BYTE) << 8) | EG_FILL_BYTE) << 8) | EG_FILL_BYTE )
#if !defined(startguardsize)
#define startguardsize 0x10
#endif
#if !defined(endguardsize)
#define endguardsize   0x10
#endif
#else
#define startguardsize 0
#define endguardsize   0
#endif

#if defined(TRACE_ALLOCS)
#define alloc_trace_on_do(ahp)  { if((ahp)->flags & alloc_trace_on) trace_on();  }
#define alloc_trace_off_do(ahp) { if((ahp)->flags & alloc_trace_on) trace_off(); }
#else
#define alloc_trace_on_do(ahp)
#define alloc_trace_off_do(ahp)
#endif

#if !defined(ALLOC_NOISE_THRESHOLD)
#define  ALLOC_NOISE_THRESHOLD 1024
#endif

/*
internal functions
*/

static P_ANY
alloc__calloc(
    _InVal_     U32 num,
    _InVal_     U32 size,
    _InRef_     PC_ALLOC_HEAP_DESC ahp);

static void
alloc__free(
    P_ANY usrcore,
    _InRef_     PC_ALLOC_HEAP_DESC ahp);

static P_ANY
alloc__malloc(
    _InVal_     U32 size,
    _InRef_     PC_ALLOC_HEAP_DESC ahp);

static P_ANY
alloc__realloc(
    P_ANY usrcore,
    _InVal_     U32 size,
    _InRef_     PC_ALLOC_HEAP_DESC ahp);

static U32
alloc__size(
    P_ANY usrcore,
    _InRef_     PC_ALLOC_HEAP_DESC ahp);

static void
alloc__validate(
    P_ANY usrcore,
    _In_z_      PCTSTR msg,
    _InoutRef_  P_ALLOC_HEAP_DESC ahp);

#if defined(FULL_ANSI)

static U32
alloc__ini_size(
    P_ANY a);

static void
alloc__ini_validate(
    P_ANY a,
    _In_z_      PCTSTR msg);

#endif

static P_ANY
alloc__main_calloc(
    _InVal_     U32 num,
    _InVal_     U32 size);

static void
alloc__main_free(
    P_ANY a);

static P_ANY
alloc__main_malloc(
    _InVal_     U32 size);

static P_ANY
alloc__main_realloc(
    P_ANY a,
    _InVal_     U32 size);

static U32
alloc__main_size(
    P_ANY a);

static void
alloc__main_validate(
    P_ANY a,
    _In_z_      PCTSTR msg);

#if defined(REDIRECT_RISCOS_KERNEL_ALLOCS)

static void
alloc__riscos_kernel_free(
    void * a);

static void *
alloc__riscos_kernel_malloc(
    _In_        size_t size);

#endif

#if TRACE_ALLOWED

static void
alloc_validate_heap(
    _InRef_     PC_ALLOC_HEAP_DESC ahp,
    _In_z_      PCTSTR routine,
    _In_        int set_guards);

static void
alloc_validate_block(
    _InRef_     PC_ALLOC_HEAP_DESC ahp,
    P_ANY usrcore,
    _In_z_      PCTSTR routine,
    _In_        int set_guards);

#endif

static P_ANY
riscos_ptr_alloc(
    _In_        U32 size,
    _InRef_     PC_ALLOC_HEAP_DESC ahp);

static void
riscos_ptr_free(
    P_RISCOS_USED_BLOCK p_used,
    _InRef_     PC_ALLOC_HEAP_DESC ahp);

static P_ANY
riscos_ptr_realloc_grow(
    P_ANY p_any,
    _InVal_     U32 new_blksize,
    _InVal_     U32 cur_blksize,
    _InRef_     PC_ALLOC_HEAP_DESC ahp);

static P_ANY
riscos_ptr_realloc_shrink(
    P_ANY p_any,
    _InVal_     U32 new_blksize,
    _InVal_     U32 cur_blksize,
    _InRef_     PC_ALLOC_HEAP_DESC ahp);

#define P_HEAP_HWM(p_heap) ( /* P_U8 */ \
    PtrAddBytes(P_U8, p_heap, p_heap->hwm) )

typedef union P_RISCOS_HEAP_DATA
{
    P_U8 c;
    P_RISCOS_FREE_BLOCK f;
    P_RISCOS_USED_BLOCK u;
    P_ANY v;
}
P_RISCOS_HEAP_DATA;

typedef union P_RISCOS_HEAP_FREE_DATA
{
    P_U8 c;
    P_RISCOS_FREE_BLOCK f;
    P_ANY v;
}
P_RISCOS_HEAP_FREE_DATA;

#define P_END_OF_FREE(p_free_block) ( /* P_U8 */ \
    p_free_block.c + p_free_block.f->size )

#define P_NEXT_FREE(p_free_block) ( /* P_U8 */ \
    p_free_block.c + p_free_block.f->free )

typedef union P_RISCOS_HEAP_USED_DATA
{
    P_U8 c;
    P_RISCOS_USED_BLOCK u;
    P_ANY v;
}
P_RISCOS_HEAP_USED_DATA;

#define P_END_OF_USED(p_used_block) ( /* P_U8 */ \
    p_used_block.c + p_used_block.u->size )

#if defined(ALLOC_CLEAR_FREE)
#define CLEAR_FREE(p_free_block) \
    memset32(p_free_block.f + 1, 'x', p_free_block.f->size - sizeof32(*p_free_block.f));
#else
#define CLEAR_FREE(p_free_block)
#endif

struct FLEX_USED_BLOCK
{
    void * anchor;
    U32 size;
};

#define RHM_SIZEOF_FLEX_USED_BLOCK \
    round_heapmgr(sizeof32(struct FLEX_USED_BLOCK))

/* ----------------------------------------------------------------------- */

U32
g_dynamic_area_limit = 0;

static int
alloc_dynamic_area_handle = 0;

_Check_return_
extern int
alloc_dynamic_area_query(void)
{
    return(alloc_dynamic_area_handle);
}

/*
the main alloc heap
*/

static ALLOC_HEAP_DESC
alloc_main_heap_desc =
{
    NULL,    /* heap */
    0,       /* minsize */
    0x8000,  /* increment (a 32K lump) */
#if defined(TRACE_MAIN_ALLOCS)
    alloc_trace_on |
#endif
#if defined(ALLOC_RELEASE_STORE_INFREQUENTLY)
    alloc_heap_compact_disabled |
#endif
#if defined(VALIDATE_MAIN_ALLOCS)
    alloc_validate_on |
    alloc_validate_heap_before_alloc |
    alloc_validate_heap_before_free |
    alloc_validate_heap_on_size |
  /*alloc_validate_block_before_realloc |*/
  /*alloc_validate_block_before_free |*/
  /*alloc_validate_block_on_size |*/
    alloc_validate_heap_blocks |
#endif
    0 /* flags */
};

U32
alloc_main_heap_minsize = 0x8000U - RHM_SIZEOF_FLEX_USED_BLOCK; /* 32KB (minus flex block overhead) */

/*
the main heap function set
*/
#if defined(FULL_ANSI)
ALLOC_FUNCTION_SET
alloc_main =
{
    calloc,
    free,
    malloc,
    realloc,
    alloc__ini_size,
    alloc__ini_validate
};
#else
ALLOC_FUNCTION_SET
alloc_main;
#endif

/*
the function set which alloc_main is redirected to on a main heap success
*/

static const ALLOC_FUNCTION_SET
alloc_main_redirected =
{
    alloc__main_calloc,
    alloc__main_free,
    alloc__main_malloc,
    alloc__main_realloc,
    alloc__main_size,
    alloc__main_validate
};

#if TRACE_ALLOWED && defined(USE_HEAP_SWIS)
static _kernel_oserror *
alloc_winge(
    _kernel_oserror * e,
    _In_z_      PCTSTR* routine)
{
    myassert2x(e == NULL, TEXT("alloc__%s error: %s"), routine, e->errmess);
    return(e);
}
#else
#define alloc_winge(e, r) (e)
#endif

static BOOL
alloc_initialise_heap(
    _InoutRef_  P_ALLOC_HEAP_DESC ahp)
{
#if defined(USE_HEAP_SWIS)
    _kernel_swi_regs rs;
#endif
    BOOL res;
    U32 new_size;
    P_RISCOS_HEAP heap;

    /* ensure minsize a multiple of heap granularity - it must already have space for the RISCOS_HEAP */
    ahp->minsize = round_heapmgr(ahp->minsize);

    new_size = ahp->minsize;

    if(ahp->increment)
    {
        new_size += RHM_SIZEOF_FLEX_USED_BLOCK;
        new_size  = div_round_ceil_u(new_size, ahp->increment) * ahp->increment;
        new_size -= RHM_SIZEOF_FLEX_USED_BLOCK;
    }

    /* must be first fixed block at start of flex, or second block if the first is a fixed heap */
    if(!flex_alloc((flex_ptr) &ahp->heap, new_size))
        return(FALSE);

    /* once alloc is running in a fixed block at start of flex, we must stop the C runtime from moving us */
    flex_set_budge(0);

    heap = ahp->heap;

    trace_4(TRACE_MODULE_ALLOC, TEXT("alloc_initialise_heap(ahp:") PTR_XTFMT TEXT(") allocated heap ") PTR_XTFMT TEXT(", size ") U32_TFMT TEXT(",") U32_XTFMT, report_ptr_cast(ahp), report_ptr_cast(heap), new_size, new_size);

#if defined(USE_HEAP_SWIS)
    rs.r[0] = HeapReason_Init;
    rs.r[1] = (int) heap;
    /* no r2 */
    rs.r[3] = new_size;
    res = (NULL == alloc_winge(_kernel_swi(OS_Heap, &rs, &rs), "initialise_heap"));
#else
    /* do the job by hand */
    heap->magic = 0x70616548; /* ID word ("Heap") */
    heap->free = 0;
    heap->hwm = sizeof32(*heap);
    heap->size = new_size;

    res = TRUE;
#endif /* USE_HEAP_SWIS */

#if TRACE_ALLOWED && (PERSONAL_TRACE_FLAGS & TRACE_MODULE_ALLOC) && defined(TEST_REALLOC_RARE_PROCESSES)
    { /* perform some self-tests, especially on realloc processes 3,4,7 */
    U32 xs = sizeof32(RISCOS_USED_BLOCK)+startguardsize+endguardsize;
    P_ANY a = alloc__malloc(32-xs, ahp);
    P_ANY b = alloc__malloc(32-xs, ahp);
    P_ANY c = alloc__malloc(32-xs, ahp);
    P_ANY d = alloc__malloc(32-xs, ahp);
    P_ANY e = alloc__malloc(32-xs, ahp);
    P_ANY f = alloc__malloc(32-xs, ahp);
    P_ANY g = alloc__malloc(32-xs, ahp);
    U32 bs = heap->size-heap->hwm;
    P_ANY h = alloc__malloc(bs-xs, ahp);
    assert(heap->free == 0);
    assert(heap->size-heap->hwm == 0);

    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[test realloc process 7"));
    alloc__free(a, ahp);
    assert(heap->free != 0);
    alloc__free(c, ahp);
    alloc__free(e, ahp);
    d = alloc__realloc(d, 3*32-xs, ahp); /* test realloc process 7 */

    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[test realloc process 3"));
    alloc__free(g, ahp);
    h = alloc__realloc(h, bs-xs+32, ahp); /* test realloc process 3 */
    alloc__free(b, ahp);
    alloc__free(d, ahp);
    alloc__free(f, ahp);
    alloc__free(h, ahp);

    assert(heap->free == 0);
    assert(heap->hwm == sizeof32(RISCOS_HEAP));
    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[heap cleared out"));

    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[test realloc process 8"));
    a = alloc__malloc(32-xs, ahp);
    b = alloc__malloc(32-xs, ahp);
    c = alloc__malloc(32-xs, ahp);
    d = alloc__malloc(32-xs, ahp);
    e = alloc__malloc(32-xs, ahp);
    f = alloc__malloc(32-xs, ahp);
    alloc__free(a, ahp);
    assert(heap->free != 0);
    alloc__free(c, ahp);
    alloc__free(e, ahp);
    d = alloc__realloc(d, 3*32-16-xs, ahp); /* test realloc process 8 */
    alloc__free(b, ahp);
    alloc__free(d, ahp);
    alloc__free(f, ahp);

    assert(heap->free == 0);
    assert(heap->hwm == sizeof32(RISCOS_HEAP));
    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[heap cleared out"));

    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[test realloc process 9"));
    a = alloc__malloc(32-xs, ahp);
    b = alloc__malloc(32-xs, ahp);
    c = alloc__malloc(32-xs, ahp);
    d = alloc__malloc(32-xs, ahp);
    e = alloc__malloc(32-xs, ahp);
    f = alloc__malloc(32-xs, ahp);
    alloc__free(a, ahp);
    assert(heap->free != 0);
    alloc__free(c, ahp);
    d = alloc__realloc(d, 2*32-xs, ahp); /* test realloc process 9 */
    alloc__free(b, ahp);
    alloc__free(d, ahp);
    alloc__free(e, ahp);
    alloc__free(f, ahp);

    assert(heap->free == 0);
    assert(heap->hwm == sizeof32(RISCOS_HEAP));
    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[heap cleared out"));

    bs = heap->size-heap->hwm - 64 - 32;
    a = alloc__malloc(64-xs, ahp);
    b = alloc__malloc(bs-xs, ahp);
    c = alloc__malloc(32-xs, ahp);
    alloc__free(a, ahp);
    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[test realloc process 14"));
    c = alloc__realloc(c, 64-xs, ahp); /* test realloc process 14 */
    alloc__free(b, ahp);
    alloc__free(c, ahp);

    a = alloc__malloc(64-xs, ahp);
    b = alloc__malloc(bs-xs, ahp);
    c = alloc__malloc(32-xs, ahp);
    alloc__free(a, ahp);
    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[test realloc process 4"));
    c = alloc__realloc(c, 32+4-xs, ahp); /* test realloc process 4 */
    alloc__free(b, ahp);
    alloc__free(c, ahp);

    assert(heap->free == 0);
    assert(heap->hwm == sizeof32(RISCOS_HEAP));
    tracef(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("[heap cleared out"));
    } /*block*/
#endif

    return(res);
}

/******************************************************************************
*
* ensure that a block of given size can be allocated,
* the heap being extended as necessary
*
******************************************************************************/

#if !RELEASED
#define PRAGMA_CHECK_STACK_OFF
#include "coltsoft/pragma.h"
#endif

_Check_return_
static int
alloc_needtoallocate(
    _InVal_     U32 need,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    const P_RISCOS_HEAP heap = ahp->heap;
    U32 spare = heap->size - heap->hwm;

    trace_8(TRACE_MODULE_ALLOC, TEXT("alloc_needtoallocate(") U32_TFMT TEXT(",") U32_XTFMT TEXT(" in heap ") PTR_XTFMT TEXT("): hwmp=") PTR_XTFMT TEXT(", size=") U32_TFMT TEXT(",") U32_XTFMT TEXT(", spare=") U32_TFMT TEXT(",") U32_XTFMT, need, need, report_ptr_cast(heap), report_ptr_cast(P_HEAP_HWM(heap)), heap->size, heap->size, spare, spare);

    assert((int) need > 0);
    if(need <= spare)
        return(TRUE);

    /* must not wiggle fixed size heaps */
    if(ahp->flags & alloc_heap_fixed_size)
        return(FALSE);

    {
    U32 new_size = heap->hwm + need;

    if(ahp->increment)
    {
        new_size += RHM_SIZEOF_FLEX_USED_BLOCK;
        new_size  = div_round_ceil_u(new_size, ahp->increment) * ahp->increment;
        new_size -= RHM_SIZEOF_FLEX_USED_BLOCK;
    }

    trace_on();
    trace_5(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT("extending heap ") PTR_XTFMT TEXT(" from size=") U32_TFMT TEXT(",") U32_XTFMT TEXT(" to ") U32_TFMT TEXT(",") U32_XTFMT, report_ptr_cast(heap), heap->size, heap->size, new_size, new_size);
    /*alloc_validate_heap(ahp, "pre heap extension", 0);*/

    if(!flex_realloc((flex_ptr) &ahp->heap, new_size))
    {
        trace_0(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT("*** heap extension failed - return FALSE"));
        trace_off();
        return(FALSE);
    }

    heap->size = flex_size((flex_ptr) &ahp->heap);
    } /*block*/

    trace_3(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT("heap ") PTR_XTFMT TEXT(" extended to size ") U32_TFMT TEXT(",") U32_XTFMT, report_ptr_cast(ahp->heap), heap->size, heap->size);
    trace_off();
    return(TRUE);
}

#if !RELEASED
#define PRAGMA_CHECK_STACK_ON
#include "coltsoft/pragma.h"
#endif

/******************************************************************************
*
* release the free store at the top of the
* heap and flex area back to the free pool
*
******************************************************************************/

static void
alloc_freeextrastore(
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    const P_RISCOS_HEAP heap = ahp->heap;

    /* wiggling prohibited? */
    if(ahp->flags & alloc_heap_compact_disabled)
        return;

    /* must not wiggle some heaps */
    if(ahp->flags & alloc_heap_dont_compact)
        return;

    {
    U32 new_size = heap->hwm;
    U32 spare;

    new_size += RHM_SIZEOF_FLEX_USED_BLOCK;
    new_size  = div_round_ceil_u(new_size, ahp->increment) * ahp->increment;
    new_size -= RHM_SIZEOF_FLEX_USED_BLOCK;

    /* don't let heap size fall too low */
    if(new_size <= ahp->minsize)
        new_size = ahp->minsize;

    if(new_size >= heap->size)
    {
        assert(new_size == heap->size);
        return;
    }

    spare = heap->size - new_size;

    trace_on();

    if((spare >= (U32) flex_granularity) || (spare + (U32) flex_storefree() >= (U32) flex_granularity))
    {
        trace_1(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT("alloc_freeextrastore: contracting heap ") PTR_XTFMT TEXT(" to free some space"), report_ptr_cast(heap));

        if(!flex_realloc((flex_ptr) &ahp->heap, new_size))
        {
            trace_0(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT("*** heap contraction failed"));
            trace_off();
            return;
        }

        heap->size = flex_size((flex_ptr) &ahp->heap);

        trace_3(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT("heap ") PTR_XTFMT TEXT(" contracted to size ") U32_TFMT TEXT(",") U32_XTFMT, report_ptr_cast(ahp->heap), heap->size, heap->size);
    }

    trace_off();
    } /*block*/
}

_Check_return_
extern STATUS
alloc_ensure_froth(
    _In_        U32 froth_size)
{
    assert(froth_size >= 0x1000);

    froth_size -= RHM_SIZEOF_FLEX_USED_BLOCK;

    if(!alloc_needtoallocate(froth_size, &alloc_main_heap_desc))
        return(status_nomem());

    return(STATUS_OK);
}

/******************************************************************************
*
* initialise allocators
*
******************************************************************************/

#if defined(USE_BOUND_LIBRARY)

extern void
__heap_checking_on_all_allocates(BOOL);

extern void
__heap_checking_on_all_deallocates(BOOL);

#endif /* USE_BOUND_LIBRARY */

_Check_return_
extern STATUS
alloc_init(void)
{
    STATUS status = STATUS_NOMEM;

#if defined(TRACE_ALLOCS)
    trace_on();
#endif

    trace_0(TRACE_MODULE_ALLOC, TEXT("alloc_init()"));

#if defined(USE_BOUND_LIBRARY)
    __heap_checking_on_all_allocates(TRUE);
    __heap_checking_on_all_deallocates(TRUE);
#endif

    for(;;) /* loop for structure */
    {
        /*reportf("g_dynamic_area_limit: %d", g_dynamic_area_limit);*/
        if((alloc_dynamic_area_handle = flex_init(de_const_cast(char *, g_dynamic_area_name), 0, g_dynamic_area_limit)) < 0)
            break;

        if(alloc_main_heap_minsize)
        {
            alloc_main_heap_desc.minsize = alloc_main_heap_minsize;

            /* SKS 27sep94 attempt to get memory back for 2MB owners. Made sensible for other systems 10nov96 */
            if(0x4000 == flex_granularity)
                alloc_main_heap_desc.increment = 0x2000;

            if(!alloc_initialise_heap(&alloc_main_heap_desc))
                break;

            /* redirect alloc main functions */
            alloc_main = alloc_main_redirected;

#if defined(VALIDATE_MAIN_ALLOCS_START)
            alloc_main_heap_desc.flags |= alloc_validate_enabled;
#endif

#if defined(REDIRECT_RISCOS_KERNEL_ALLOCS)
            _kernel_register_allocs(alloc__riscos_kernel_malloc, alloc__riscos_kernel_free);
#endif

#if 0
            {
            void * p_data = NULL;
            unsigned n_words = 8*1024;

            while(n_words >= 2*1024)
            {
                unsigned n_words_got = _kernel_alloc(n_words, &p_data);

                if(NULL != p_data)
                {
                    n_words = n_words_got;
                    break;
                }

                n_words -= 128; /* try 512 bytes less */
            }

#if defined(ALLOC_TRACK_USAGE)
            freopen("$.poo", "wb", stderr);

            fprintf(stderr, TEXT("alloc_init: malloc ") PTR_XTFMT TEXT(" ") U32_XTFMT TEXT(" words"), p_data, n_words);
#endif

            } /*block*/
#endif

        }

        status = STATUS_OK;

        break; /* always - loop only for structure */
        /*NOTREACHED*/
    }

#if defined(TRACE_ALLOCS)
    trace_off();
#endif

    return(status);
}

/******************************************************************************
*
* release surplus memory
*
******************************************************************************/

extern void
alloc_tidy_up(void)
{
    int heap_compact_disabled = alloc_main_heap_desc.flags & alloc_heap_compact_disabled;
    alloc_main_heap_desc.flags &= ~alloc_heap_compact_disabled;
    alloc_freeextrastore(&alloc_main_heap_desc);
    alloc_main_heap_desc.flags |= heap_compact_disabled;
}

extern void
alloc_track_stop(void)
{
#if defined(ALLOC_TRACK_USAGE)
    fclose(stderr);
#endif
}

#if TRACE_ALLOWED

/******************************************************************************
*
* TRACE_ALLOWED functions
*
******************************************************************************/

extern void
alloc_traversefree(
    _In_        int which)
{
    const PC_ALLOC_HEAP_DESC ahp = &alloc_main_heap_desc;

    IGNOREPARM(which);

    alloc_validate_heap(ahp, "traversefree", 0);
}

#endif /* TRACE_ALLOWED */

/******************************************************************************
*
* heap validation functions
*
******************************************************************************/

static void
alloc_validate_heap(
    _InRef_     PC_ALLOC_HEAP_DESC ahp,
    _In_z_      PCTSTR routine,
    _In_        int set_guards)
{
#if !TRACE_ALLOWED
    IGNOREPARM_InRef_(ahp);
    IGNOREPARM(routine);
    IGNOREPARM(set_guards);
#else
    const P_RISCOS_HEAP heap = ahp->heap;
    P_RISCOS_HEAP_DATA p;
    P_U8 freep;
    P_U8 hwmp;
    P_U8 endp;
    U32 current_hwm, current_size;
    P_U8 usrcore;
    U32 syssize, usrsize, offset;
    U32 largest_free = 0;
    U32 total_free = 0;

    /* validate heap */

    p.v          = heap + 1;
    offset       = heap->free;
    freep        = offset ? PtrAddBytes(P_U8, &heap->free, offset) : NULL;

    current_hwm  = heap->hwm;
    hwmp         = P_HEAP_HWM(heap);

    current_size = heap->size;
    endp         = PtrAddBytes(P_U8, heap, current_size);

    trace_5(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT(" heap ") PTR_XTFMT TEXT(" size=") U32_TFMT TEXT(",") U32_XTFMT TEXT(" endp=") PTR_XTFMT TEXT(" hwmp=") PTR_XTFMT TEXT("\n*** free/used blocks ***:"), report_ptr_cast(heap), current_size, current_size, report_ptr_cast(endp), report_ptr_cast(hwmp));

    if(current_hwm >= (U32) S32_MAX)
        myassert4(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(" has corrupt hwm ") U32_TFMT TEXT(",") U32_XTFMT,
                  routine, report_ptr_cast(heap),
                  current_hwm, current_hwm);

    if(current_size >= (U32) S32_MAX)
        myassert4(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(" has corrupt size ") U32_TFMT TEXT(",") U32_XTFMT,
                  routine, report_ptr_cast(heap),
                  current_size, current_size);

    if(current_hwm > current_size)
        myassert6(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(" has corrupt hwm ") U32_TFMT TEXT(",") U32_XTFMT TEXT(" > size ") U32_TFMT TEXT(",") U32_XTFMT,
                  routine, report_ptr_cast(heap),
                  current_hwm, current_hwm,
                  current_size, current_size);

    if(offset)
        if((offset > current_hwm)  ||  (offset < sizeof32(RISCOS_HEAP) - offsetof32(RISCOS_HEAP, size)))
            myassert6(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(" has corrupt initial free link ") U32_TFMT TEXT(",") U32_XTFMT TEXT(" (hwm ") U32_TFMT TEXT(",") U32_XTFMT TEXT(")"),
                      routine, report_ptr_cast(heap),
                      offset, offset,
                      current_hwm, current_hwm);

    do  {
        if(p.c == hwmp)
        {
            syssize = endp - hwmp;

            /*largest_free = MAX(largest_free, syssize);*/
            total_free += syssize;

            tracef(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT("  (free ") PTR_XTFMT TEXT(",%5u,") U32_XTFMT TEXT(" lge %5u,") U32_XTFMT TEXT(" tot %5u,") U32_XTFMT TEXT(" --- above hwm)"), p.v, syssize, syssize, largest_free, largest_free, total_free, total_free);

            p.c = endp;
        }
        else if(p.c == freep)
        {
            offset  = p.f->free;
            syssize = p.f->size;

            largest_free = MAX(largest_free, syssize);
            total_free += syssize;

            if(syssize >= ALLOC_NOISE_THRESHOLD)
                tracef(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT("  (free ") PTR_XTFMT TEXT(",%5u,") U32_XTFMT TEXT(" lge %5u,") U32_XTFMT TEXT(" tot %5u,") U32_XTFMT TEXT(")"), p.v, syssize, syssize, largest_free, largest_free, total_free, total_free);

            if(offset)
            {
                freep += offset;

                if((offset & 3) != 0)
                    myassert5(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(" free block ") PTR_XTFMT TEXT(" has non-word aligned next free block offset ") U32_TFMT TEXT(",") U32_XTFMT,
                              routine, report_ptr_cast(heap),
                              p.v, offset, offset);

                if((freep < p.c + syssize)  ||  (freep > hwmp))
                    myassert5(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(" free block ") PTR_XTFMT TEXT(" has corrupt next free block offset ") U32_TFMT TEXT(",") U32_XTFMT,
                              routine, report_ptr_cast(heap),
                              p.v, offset, offset);
            }
            else
                freep = NULL;

            if((syssize & 3) != 0)
                myassert5(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(" free block ") PTR_XTFMT TEXT(" has non-word aligned size ") U32_TFMT TEXT(",") U32_XTFMT,
                          routine, report_ptr_cast(heap),
                          p.v, syssize, syssize);

            if((p.c + syssize <= p.c)  ||  (p.c + syssize > hwmp))
                myassert5(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(" free block ") PTR_XTFMT TEXT(" has corrupt size ") U32_TFMT TEXT(",") U32_XTFMT,
                          routine, report_ptr_cast(heap),
                          p.v, syssize, syssize);

            p.c += syssize;
        }
        else
        {
            syssize = p.u->size;

            usrsize = syssize - sizeof32(*p.u) - (startguardsize + endguardsize);

            usrcore = PtrAddBytes(P_U8, (p.u + 1), startguardsize);

            if(syssize >= ALLOC_NOISE_THRESHOLD)
                trace_6(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT("  (used ") PTR_XTFMT TEXT(",%5u,") U32_XTFMT TEXT(" usr ") PTR_XTFMT TEXT(",%5u,") U32_XTFMT TEXT(")"), p.v, syssize, syssize, usrcore, usrsize, usrsize);

            if((syssize & 3) != 0)
                myassert8(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(" used block ") PTR_XTFMT TEXT(" ") PTR_XTFMT TEXT(" has non-word aligned size ") U32_TFMT TEXT(",") U32_XTFMT TEXT(" ") U32_TFMT TEXT(",") U32_XTFMT,
                          routine, report_ptr_cast(heap),
                          p.v, usrcore,
                          syssize, syssize,
                          usrsize, usrsize);

            if((p.c + syssize <= p.c)  ||  (p.c + syssize > hwmp))
                myassert8(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(" used block ") PTR_XTFMT TEXT(" ") PTR_XTFMT TEXT(" has corrupt size ") U32_TFMT TEXT(",") U32_XTFMT TEXT(" ") U32_TFMT TEXT(",") U32_XTFMT,
                          routine, report_ptr_cast(heap),
                          p.v, usrcore,
                          syssize, syssize,
                          usrsize, usrsize);

            if((ahp->flags & alloc_validate_heap_blocks) || set_guards)
                alloc_validate_block(ahp, usrcore, routine, set_guards);

            p.c += syssize;
        }
    }
    while(p.c != endp);

    trace_0(TRACE_MODULE_ALLOC | TRACE_APP_MEMORY_USE, TEXT("  -- heap validated"));

#endif /* TRACE_ALLOWED */
}

static void
alloc_validate_block(
    _InRef_     PC_ALLOC_HEAP_DESC ahp,
    P_ANY usrcore,
    _In_z_      PCTSTR routine,
    _In_        int set_guards)
{
#if !TRACE_ALLOWED
    IGNOREPARM_InRef_(ahp);
    IGNOREPARM(usrcore);
    IGNOREPARM(routine);
    IGNOREPARM(set_guards);
#else
    const P_RISCOS_HEAP heap = ahp->heap;
    P_U8 syscore;
    P_RISCOS_USED_BLOCK p_used_block;
    U32 blksize, syssize, usrsize;
    int valid_size = 1;
#if defined(CHECK_ALLOCS)
    U32 actualusrsize;
#else
    IGNOREPARM(set_guards);
#endif

    syscore      = PtrSubBytes(P_U8, usrcore, startguardsize);
    p_used_block = blockhdrp(syscore);

    if( ((uintptr_t) p_used_block <  (uintptr_t) heap)              ||
        ((uintptr_t) p_used_block >= (uintptr_t) heap + heap->size) )
    {
        myassert4(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(", blk(") PTR_XTFMT TEXT("), usr(") PTR_XTFMT TEXT(") block is not in heap"),
                  routine, report_ptr_cast(heap),
                  p_used_block, usrcore);

        return;
    }

    blksize = p_used_block->size;
    syssize = blksize - sizeof32(*p_used_block);
    usrsize = syssize - (startguardsize + endguardsize);

    if(((syssize & 3) != 0) || ((U32) syssize >= (U32) S32_MAX))
    {
        myassert8(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(", blk(") PTR_XTFMT TEXT(",") U32_TFMT TEXT(",") U32_XTFMT TEXT("), usr(") PTR_XTFMT TEXT(",") U32_TFMT TEXT(",") U32_XTFMT TEXT(") block has corrupt size (can't check endguard)"),
                  routine, report_ptr_cast(heap),
                  p_used_block, blksize, blksize,
                  usrcore, usrsize, usrsize);

        valid_size = 0;
    }

#if defined(CHECK_ALLOCS)
    {
    P_U32 end32p;
    P_U32 ptr32p;
    P_U32 start32p;
    P_U8 end;
    P_U8 ptr;
    P_U8 start;

    start32p = (P_U32) syscore;
    end32p   = PtrAddBytes(P_U32, syscore, startguardsize); /* give offsets relative to start of guts */
    ptr32p   = end32p;

    while(--ptr32p > start32p) /* omit first start guard word as it contains size */
    {
        if(set_guards)
            *ptr32p = SG_FILL_WORD;
        else if(*ptr32p != SG_FILL_WORD)
        {
            myassert12(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(", blk(") PTR_XTFMT TEXT(",") U32_TFMT TEXT(",") U32_XTFMT TEXT("), usr(") PTR_XTFMT TEXT(",") U32_TFMT TEXT(",") U32_XTFMT TEXT(") block has fault at startguard offset ") U32_TFMT TEXT(",") U32_XTFMT TEXT(": ") U32_XTFMT TEXT(" != SG_FILL_WORD ") U32_XTFMT,
                       routine, report_ptr_cast(heap),
                       p_used_block, blksize, blksize,
                       usrcore, usrsize, usrsize,
                       (end32p - ptr32p) * sizeof32(*ptr32p),
                       (end32p - ptr32p) * sizeof32(*ptr32p),
                       (int) *ptr32p, SG_FILL_WORD);

            *ptr32p = SG_FILL_WORD; /* repair, so we don't see it again */
        }
    }

    actualusrsize = 0; /* keep dataflower happy */

    /*CONSTANTCONDITION*/
    if_constant(0 == startguardsize)
        valid_size = 0;

    if(valid_size)
    {
        /* actual size stored in first start guard word */
        actualusrsize = * (P_U32) start32p;

        if(round_heapmgr(actualusrsize + (startguardsize + endguardsize) + HEAPMGR_OVERHEAD) != blksize)
        {
            myassert10(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(", blk(") PTR_XTFMT TEXT(",") U32_TFMT TEXT(",") U32_XTFMT TEXT("), usr(") PTR_XTFMT TEXT(",") U32_TFMT TEXT(",") U32_XTFMT TEXT(") block inconsistent with alloc size ") U32_TFMT TEXT(",") U32_XTFMT TEXT(" (can't check endguard)"),
                       routine, report_ptr_cast(heap),
                       p_used_block, blksize, blksize,
                       usrcore, usrsize, usrsize,
                       actualusrsize, actualusrsize);

            valid_size = 0;
        }
    }

    if(valid_size)
    {
        start = PtrAddBytes(P_U8, usrcore, actualusrsize); /* now tests from end of user's block, not rounding up */
        end   = PtrAddBytes(P_U8, p_used_block, blksize);  /* but all the way up to the top still */
        ptr   = start;

        do  {
            if(set_guards)
                *ptr = EG_FILL_BYTE;
            else if(*ptr != EG_FILL_BYTE)
            {
                myassert12(TEXT("alloc__%s: heap ") PTR_XTFMT TEXT(", blk(") PTR_XTFMT TEXT(",") U32_TFMT TEXT(",") U32_XTFMT TEXT("), usr(") PTR_XTFMT TEXT(",") U32_TFMT TEXT(",") U32_XTFMT TEXT(") block has fault at endguard offset ") U32_TFMT TEXT(",") U32_XTFMT TEXT(": 0x%.2X != FILL_BYTE 0x%.2X"),
                           routine, report_ptr_cast(heap),
                           p_used_block, blksize, blksize,
                           usrcore, usrsize, usrsize,
                           (ptr - start) * sizeof32(*ptr),
                           (ptr - start) * sizeof32(*ptr),
                           *ptr, EG_FILL_BYTE);

                *ptr = EG_FILL_BYTE; /* repair, so we don't see it again */
            }
        }
        while(++ptr < end);
    }
    } /*block*/
#endif /* CHECK_ALLOCS */

#endif /* TRACE_ALLOWED */
}

#if defined(FULL_ANSI)

static U32
alloc__ini_size(
    P_ANY usrcore)
{
    myassert1x(usrcore == (P_ANY) 1, TEXT("unable to yield size for block ") PTR_XTFMT, usrcore);
    return(0);
}

static void
alloc__ini_validate(
    P_ANY usrcore,
    _In_z_      PCTSTR msg)
{
    IGNOREPARM(usrcore);
    IGNOREPARM(msg);
}

#endif

/******************************************************************************
*
* these allocation routines cause flex blocks to move
*
******************************************************************************/

static P_ANY
alloc__main_calloc(
    _InVal_     U32 num,
    _InVal_     U32 size)
{
    return(alloc__calloc(num, size, &alloc_main_heap_desc));
}

static void
alloc__main_free(
    P_ANY a)
{
    alloc__free(a, &alloc_main_heap_desc);
}

static P_ANY
alloc__main_malloc(
    _InVal_     U32 size)
{
    return(alloc__malloc(size, &alloc_main_heap_desc));
}

static P_ANY
alloc__main_realloc(
    P_ANY a,
    _InVal_     U32 size)
{
    return(alloc__realloc(a, size, &alloc_main_heap_desc));
}

static U32
alloc__main_size(
    P_ANY a)
{
    return(alloc__size(a, &alloc_main_heap_desc));
}

static void
alloc__main_validate(
    P_ANY a,
    _In_z_      PCTSTR msg)
{
    alloc__validate(a, msg, &alloc_main_heap_desc);
}

/******************************************************************************
*
* allocates space for an array of nmemb objects, each of whose size is
* 'size'. The space is initialised to all bits zero.
*
* Returns: either a null pointer or a pointer to the allocated space.
*
******************************************************************************/

static P_ANY
alloc__calloc(
    _InVal_     U32 num,
    _InVal_     U32 size,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    U32 n_bytes;
    P_ANY a;

    /* not very good - could get overflow in calc */
    n_bytes = (num * size + (sizeof32(S32) - 1)) & ~(sizeof32(S32) - 1);

    a = alloc__malloc(n_bytes, ahp);

    if(a)
        memset32(a, 0, n_bytes);

    return(a);
}

/******************************************************************************
*
* causes the space pointed to by ptr to be deallocated (i.e., made
* available for further allocation). If ptr is a null pointer, no action
* occurs. Otherwise, if ptr does not match a pointer earlier returned by
* calloc, malloc or realloc or if the space has been deallocated by a call
* to free or realloc, the behaviour is undefined.
*
******************************************************************************/

static void
alloc__free(
    P_ANY usrcore,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    P_RISCOS_HEAP_USED_DATA b;

    if(NULL == usrcore)
        return;

    alloc_trace_on_do(ahp);

    trace_2(TRACE_MODULE_ALLOC, TEXT("alloc__free(") PTR_XTFMT TEXT(" in heap ") PTR_XTFMT TEXT(")"), usrcore, report_ptr_cast(ahp->heap));

    if(ahp->flags & alloc_validate_enabled)
    {
        if(ahp->flags & alloc_validate_block_before_free)
            alloc_validate_block(ahp, usrcore, "free BLK", 0);

        if(ahp->flags & alloc_validate_heap_before_free)
            alloc_validate_heap(ahp, "free", 0);
    }

    b.v = usrcore;
    b.c -= startguardsize;

    riscos_ptr_free(b.u - 1, ahp);

    if(ahp->flags & alloc_validate_enabled)
        if(ahp->flags & alloc_validate_heap_after_free)
            alloc_validate_heap(ahp, "AFTER_free", 0);

    alloc_freeextrastore(ahp);

    alloc_trace_off_do(ahp);
}

/******************************************************************************
*
* allocates space for an object whose size is specified by 'size' and whose
* value is indeterminate.
*
* Returns: either a null pointer or a pointer to the allocated space.
*
******************************************************************************/

#if !RELEASED
#define PRAGMA_CHECK_STACK_OFF
#include "coltsoft/pragma.h"
#endif

static P_ANY
alloc__malloc(
    _InVal_     U32 size,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    P_U8 syscore;
    P_U8 usrcore;
    U32 syssize, usrsize;

    alloc_trace_on_do(ahp);

    trace_3(TRACE_MODULE_ALLOC, TEXT("alloc__malloc(") U32_TFMT TEXT(",") U32_XTFMT TEXT(" in heap ") PTR_XTFMT TEXT(") |"), size, size, report_ptr_cast(ahp->heap));

    if(ahp->flags & alloc_validate_enabled)
        if(ahp->flags & alloc_validate_heap_before_alloc)
            alloc_validate_heap(ahp, "malloc", 0);

    if(0 == size)
    {
        trace_0(TRACE_MODULE_ALLOC, TEXT("|yields NULL because zero size"));
        alloc_trace_off_do(ahp);
        return(NULL);
    }

    usrsize = size;
    syssize = usrsize + (startguardsize + endguardsize);

    syscore = riscos_ptr_alloc(syssize, ahp);

    usrcore = syscore + startguardsize;

#if defined(CHECK_ALLOCS)
    if(ahp->flags & alloc_validate_enabled) /* no longer regardless of enable state; relies on enabling setting guards */
    {
        memset32(syscore, SG_FILL_BYTE, startguardsize);
        memset32(syscore + startguardsize, EG_FILL_BYTE, blocksize(syscore) - startguardsize);
    }

    /* store requested size in the first start guard word */
    /*CONSTANTCONDITION*/
    if_constant(startguardsize)
        * (P_U32) (void *) syscore = usrsize;
#endif

    if(ahp->flags & alloc_validate_enabled)
    {
        if(ahp->flags & alloc_validate_block_after_alloc)
            alloc_validate_block(ahp, usrcore, "AFTER_malloc BLK", 0);

        if(ahp->flags & alloc_validate_heap_after_alloc)
            alloc_validate_heap(ahp, "AFTER_malloc", 0);
    }

    trace_1(TRACE_MODULE_ALLOC, TEXT("|yields ") PTR_XTFMT, usrcore);

    alloc_trace_off_do(ahp);

    return(usrcore);
}

#if defined(REDIRECT_RISCOS_KERNEL_ALLOCS)

/* All this turns out to do is to redirect the procs used by
 * the kernel part of the C library away from malloc/free!
 * Useful for stack extension but nothing else.
*/

static void
alloc__riscos_kernel_free(
    void * a)
{
    alloc__free(a, &alloc_main_heap_desc);
}

static void *
alloc__riscos_kernel_malloc(
    _In_        size_t size)
{
    return(alloc__malloc(size, &alloc_main_heap_desc));
}

#endif

#if !RELEASED
#define PRAGMA_CHECK_STACK_ON
#include "coltsoft/pragma.h"
#endif

/******************************************************************************
*
* changes the size of the object pointed to by ptr to the size specified by
* size. The contents of the object shall be unchanged up to the lesser of
* the new and old sizes. If the new size is larger, the value of the newly
* allocated portion of the object is indeterminate. If ptr is a null
* pointer, the realloc function behaves like a call to malloc for the
* specified size. Otherwise, if ptr does not match a pointer earlier
* returned by calloc, malloc or realloc, or if the space has been
* deallocated by a call to free or realloc, the behaviour is undefined.
* If the space cannot be allocated, the object pointed to by ptr is
* unchanged. If size is zero and ptr is not a null pointer, the object it
* points to is freed.
*
* Returns: either a null pointer or a pointer to the possibly moved allocated space.
*
******************************************************************************/

static P_ANY
alloc__realloc(
    P_ANY usrcore,
    _InVal_     U32 size,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    P_U8 syscore;
    U32 syssize, usrsize, new_blksize, current_blksize;

    alloc_trace_on_do(ahp);

    /* shrinking to zero size? */

    if(0 == size)
    {
        if(NULL == usrcore)
        {
            trace_0(TRACE_MODULE_ALLOC, TEXT("alloc__realloc(NULL,0) yields NULL because zero size"));
            alloc_trace_off_do(ahp);
            return(NULL);
        }
        else
        {
            trace_0(TRACE_MODULE_ALLOC, TEXT("alloc__realloc(p,0) -> alloc__free(p) because zero size"));
            alloc_trace_off_do(ahp);
            alloc__free(usrcore, ahp);
            return(NULL);
        }
    }

    /* first-time allocation? */

    if(NULL == usrcore)
    {
        trace_0(TRACE_MODULE_ALLOC, TEXT("alloc__realloc(NULL,n) -> alloc__malloc(n) because NULL pointer"));
        alloc_trace_off_do(ahp);
        return(alloc__malloc(size, ahp));
    }

    /* a real realloc */

    trace_4(TRACE_MODULE_ALLOC, TEXT("alloc__realloc(") PTR_XTFMT TEXT(", ") U32_TFMT TEXT(",") U32_XTFMT TEXT(" in heap ") PTR_XTFMT TEXT(")) |"), usrcore, size, size, report_ptr_cast(ahp->heap));

    if(ahp->flags & alloc_validate_enabled)
    {
        if(ahp->flags & alloc_validate_block_before_realloc)
            alloc_validate_block(ahp, usrcore, "realloc BLK", 0);

        if(ahp->flags & alloc_validate_heap_before_alloc)
            alloc_validate_heap(ahp, "realloc", 0);
    }

    usrsize = size;
    syssize = usrsize + (startguardsize + endguardsize);

    syscore = PtrSubBytes(P_U8, usrcore, startguardsize);

    /* must use actual full block sizes */
    new_blksize     = round_heapmgr(syssize + HEAPMGR_OVERHEAD);
    current_blksize = blocksize(syscore) + HEAPMGR_OVERHEAD;

    /* block not changing allocated size? (trivial shrink/grow) */

    if(new_blksize == current_blksize)
    {
        trace_1(TRACE_MODULE_ALLOC, TEXT("|(not moved, same allocated size) yields ") PTR_XTFMT, usrcore);

#if defined(CHECK_ALLOCS)
        /* block not moved - just consider size update and endguard fill */
        /*CONSTANTCONDITION*/
        if_constant(startguardsize)
            * (P_U32) (void *) syscore = usrsize;

        if(ahp->flags & alloc_validate_enabled)
            memset32(PtrAddBytes(P_U8, usrcore, usrsize), EG_FILL_BYTE, (PtrAddBytes(P_U8, blockhdrp(syscore), new_blksize) - PtrAddBytes(P_U8, usrcore, usrsize)));
#endif

        if(ahp->flags & alloc_validate_enabled)
        {
            if(ahp->flags & alloc_validate_block_after_realloc)
                alloc_validate_block(ahp, usrcore, "AFTER_=_realloc BLK", 0);

            if(ahp->flags & alloc_validate_heap_after_realloc)
                alloc_validate_heap(ahp, "AFTER_=_realloc", 0);
        }

        alloc_trace_off_do(ahp);
        return(usrcore);
    }

    /* block shrinking? */

    if(new_blksize < current_blksize)
    {
        if(NULL == (syscore = riscos_ptr_realloc_shrink(syscore, new_blksize, current_blksize, ahp)))
        {
            alloc_trace_off_do(ahp);
            return(NULL);
        }

        alloc_freeextrastore(ahp);

        usrcore = syscore + startguardsize;

        trace_1(TRACE_MODULE_ALLOC, TEXT("|(shrunk) yields ") PTR_XTFMT, usrcore);

#if defined(CHECK_ALLOCS)
        /* startguard always copied safely on realloc - just consider size update and endguard fill */
        /*CONSTANTCONDITION*/
        if_constant(startguardsize)
            * (P_U32) syscore = usrsize;

        if(ahp->flags & alloc_validate_enabled)
            memset32(PtrAddBytes(P_U8, usrcore, usrsize), EG_FILL_BYTE, (PtrAddBytes(P_U8, blockhdrp(syscore), new_blksize) - PtrAddBytes(P_U8, usrcore, usrsize)));
#endif

        if(ahp->flags & alloc_validate_enabled)
        {
            if(ahp->flags & alloc_validate_block_after_realloc)
                alloc_validate_block(ahp, usrcore, "AFTER_-_realloc BLK", 0);

            if(ahp->flags & alloc_validate_heap_after_realloc)
                alloc_validate_heap(ahp, "AFTER_-_realloc", 0);
        }

        alloc_trace_off_do(ahp);
        return(usrcore);
    }

    /* block is growing */

    /* new_blksize > current_blksize */

    if(NULL == (syscore = riscos_ptr_realloc_grow(syscore, new_blksize, current_blksize, ahp)))
    {
        alloc_trace_off_do(ahp);
        return(NULL);
    }

    alloc_freeextrastore(ahp);

    usrcore = syscore + startguardsize;

    trace_1(TRACE_MODULE_ALLOC, TEXT("| (grown) yields ") PTR_XTFMT, usrcore);

#if defined(CHECK_ALLOCS)
    /* startguard always copied safely on realloc - just consider size update and endguard fill */
    /*CONSTANTCONDITION*/
    if_constant(startguardsize)
        * (P_U32) (void *) syscore = usrsize;

    if(ahp->flags & alloc_validate_enabled)
        memset32(PtrAddBytes(P_U8, usrcore, usrsize), EG_FILL_BYTE, (PtrAddBytes(P_U8, blockhdrp(syscore), new_blksize) - PtrAddBytes(P_U8, usrcore, usrsize)));
#endif

    if(ahp->flags & alloc_validate_enabled)
    {
        if(ahp->flags & alloc_validate_block_after_realloc)
            alloc_validate_block(ahp, usrcore, "AFTER_+_realloc BLK", 0);

        if(ahp->flags & alloc_validate_heap_after_realloc)
            alloc_validate_heap(ahp, "AFTER_+_realloc", 0);
    }

    alloc_trace_off_do(ahp);
    return(usrcore);
}

static U32
alloc__size(
    P_ANY usrcore,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    P_U8 syscore;
    U32 size;

    if(!usrcore)
        return(0);

    alloc_trace_on_do(ahp);

    if(ahp->flags & alloc_validate_enabled)
    {
        if(ahp->flags & alloc_validate_block_on_size)
            alloc_validate_block(ahp, usrcore, "size", 0);

        if(ahp->flags & alloc_validate_heap_on_size)
            alloc_validate_heap(ahp, "size", 0);
    }

    syscore = usrcore;
    syscore -= startguardsize;

    size = blocksize(syscore);
    size -= (startguardsize + endguardsize);

    alloc_trace_off_do(ahp);

    return(size);
}

static void
alloc__validate(
    P_ANY a,
    _In_z_      PCTSTR msg,
    _InoutRef_  P_ALLOC_HEAP_DESC ahp)
{
    switch((int) a)
    {
    case 0:
        if(ahp->flags & alloc_validate_enabled)
            alloc_validate_heap(ahp, msg, 0);
        break;

    case 1:
        if((ahp->flags & alloc_validate_enabled) == 0)
        {
            ahp->flags |= alloc_validate_enabled;

            alloc_validate_heap(ahp, msg, 1); /* ensure all currently allocated blocks get guards */
        }
        else
            alloc_validate_heap(ahp, msg, 0);
        break;

    case 2:
        ahp->flags &= ~alloc_validate_enabled;
        break;

    default:
        alloc_validate_block(ahp, a, msg, 0);
        break;
    }
}

#if !RELEASED
#define PRAGMA_CHECK_STACK_OFF
#include "coltsoft/pragma.h"
#endif

#if defined(USE_HEAP_SWIS)

static P_ANY
riscos_ptr_alloc(
    _InVal_     U32 size,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    const P_RISCOS_HEAP heap = ahp->heap;
    _kernel_swi_regs rs;
    P_ANY p_any;

    rs.r[0] = HeapReason_Get;
    rs.r[1] = (int) heap;
    /* no r2 */
    rs.r[3] =       size;
    p_any   = _kernel_swi(OS_Heap, &rs, &rs) ? NULL : (P_ANY) rs.r[2];

    if(NULL == p_any)
    {
        if(!alloc_needtoallocate(round_heapmgr(HEAPMGR_OVERHEAD + size), ahp))
        {
            trace_0(TRACE_MODULE_ALLOC, TEXT("|yields NULL because heap extension failed"));
            return(NULL);
        }

        rs.r[0] = HeapReason_Get;
        rs.r[1] = (int) heap;
        /* no r2 */
        rs.r[3] =       size;
        p_any   = alloc_winge(_kernel_swi(OS_Heap, &rs, &rs), "malloc 2") ? NULL : (P_ANY) rs.r[2];

        myassert1x(p_any, TEXT("alloc__malloc(%u) failed unexpectedly after heap extension"), size);
    }

    return(p_any);
}

static void
riscos_ptr_free(
    P_RISCOS_USED_BLOCK p_used,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    const P_RISCOS_HEAP heap = ahp->heap;
    _kernel_swi_regs rs;

    rs.r[0] = HeapReason_Free;
    rs.r[1] = (int) heap;
    rs.r[2] = (int) (p_used + 1);
    (void) alloc_winge(_kernel_swi(OS_Heap, &rs, &rs), "free");
}

static P_ANY
riscos_ptr_realloc_grow(
    P_ANY p_used,
    _InVal_     U32 new_blksize,
    _InVal_     U32 cur_blksize,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    const P_RISCOS_HEAP heap = ahp->heap;
    U32 size_diff = new_blksize - cur_blksize;
    _kernel_swi_regs rs;
    P_ANY p_any;

    rs.r[0] = HeapReason_ExtendBlock;
    rs.r[1] = (int) heap;
    rs.r[2] = (int) p_used;
    rs.r[3] =       size_diff;
    p_any   = _kernel_swi(OS_Heap, &rs, &rs) ? NULL : (P_ANY) rs.r[2];

    if(NULL == p_any)
    {
        if(!alloc_needtoallocate(new_blksize, ahp))
        {
            trace_0(TRACE_MODULE_ALLOC, TEXT("|yields NULL because heap extension failed"));
            return(NULL);
        }

        rs.r[0] = HeapReason_ExtendBlock;
        rs.r[1] = (int) heap;
        rs.r[2] = (int) p_used;
        rs.r[3] =       size_diff;
        p_any   = alloc_winge(_kernel_swi(OS_Heap, &rs, &rs), "realloc(growth) 2") ? NULL : (P_ANY) rs.r[2];

        myassert1x(p_any, TEXT("alloc__realloc(%u) failed unexpectedly after heap extension"), new_blksize - cur_blksize);
    }

    alloc_freeextrastore(ahp);

    return(p_any);
}

static P_ANY
riscos_ptr_realloc_shrink(
    P_ANY p_any,
    _InVal_     U32 new_blksize,
    _InVal_     U32 cur_blksize,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    const P_RISCOS_HEAP heap = ahp->heap;
    U32 size_diff = new_blksize - cur_blksize;
    _kernel_swi_regs rs;

    rs.r[0] = HeapReason_ExtendBlock;
    rs.r[1] = (int) heap;
    rs.r[2] = (int) p_any;
    rs.r[3] =       size_diff;
    p_any   = alloc_winge(_kernel_swi(OS_Heap, &rs, &rs), "realloc(shrink)") ? NULL : (P_ANY) rs.r[2];

    return(p_any);
}

#else /* USE_HEAP_SWIS */

#if TRACE_ALLOWED && (PERSONAL_TRACE_FLAGS & TRACE_MODULE_ALLOC) && defined(ALLOC_TRACK_PROCESS_USE)

static BITMAP(used_processes, 32);

static int used_process = 0;

static void
USED_PROCESS(
    _In_        int n)
{
    used_process = n;

    if(!bitmap_bit_test(used_processes, used_process, 32))
    {
        bitmap_bit_set(used_processes, used_process, 32);
        trace_1(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("used process %d"), used_process);
        host_bleep();
    }
}

#else

#define USED_PROCESS(n)

#endif

static P_ANY
riscos_ptr_alloc(
    _In_        U32 size,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    const P_RISCOS_HEAP heap = ahp->heap;
    P_RISCOS_HEAP_FREE_DATA p_free_block;
    P_RISCOS_HEAP_USED_DATA p_used_block;
    U32 * p_free_offset;
    U32   next_free_offset;

#if defined(ALLOC_TRACK_USAGE)
    fprintf(stderr, "alloc:%x %x", size, round_heapmgr(HEAPMGR_OVERHEAD + size));
#endif

    /* block needs to be this big */
    size = round_heapmgr(HEAPMGR_OVERHEAD + size);

    /* scan free list for first fit */
    p_free_block.v = p_free_offset = &heap->free;

    while(NULL != (next_free_offset = *p_free_offset))
    {
        assert(next_free_offset < heap->size);
        p_free_block.c += next_free_offset;

        if(size <= p_free_block.f->size)
        {
            /* will return this block (or part thereof) */
            p_used_block.v = p_free_block.v;

            if(size == p_free_block.f->size)
            {
                /* take free block away entirely */
                *p_free_offset = p_free_block.f->free ? *p_free_offset + p_free_block.f->free : 0;
            }
            else /* if(size < p_free_block.f->size) */
            {
                /* allocate core from front of free block */
                RISCOS_FREE_BLOCK F = *p_free_block.f;
                /* this free block is now further away than before */
                *p_free_offset += size;
                p_free_block.c += size;
                /* next free block after this one is now closer than before */
                p_free_block.f->free = F.free ? F.free - size : 0;
                p_free_block.f->size = F.size - size;
                CLEAR_FREE(p_free_block);
            }

            p_used_block.u->size = size;
            return(p_used_block.u + 1);
        }

        p_free_offset = &p_free_block.f->free;
    }

    /* trivial allocation at hwm */
    if(!alloc_needtoallocate(size, ahp))
    {
        trace_0(TRACE_MODULE_ALLOC, TEXT("|yields NULL because heap extension failed"));
        return(NULL);
    }

    p_used_block.c = P_HEAP_HWM(heap);
    heap->hwm += size;

    p_used_block.u->size = size;
    return(p_used_block.u + 1);
}

static void
riscos_ptr_free(
    P_RISCOS_USED_BLOCK p_used,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    const P_RISCOS_HEAP heap = ahp->heap;
    P_RISCOS_HEAP_FREE_DATA p_free_block;
    P_RISCOS_HEAP_USED_DATA p_used_block;
    U32 * p_free_offset;
    U32   next_free_offset;

    p_used_block.u = p_used;

    /* scan free list for insertion/coalescing */
    p_free_block.v = p_free_offset = &heap->free;

    while(NULL != (next_free_offset = *p_free_offset))
    {
        assert(next_free_offset < heap->size);
        p_free_block.c += next_free_offset;

        /* coalesce with lower free block? */
        if(P_END_OF_FREE(p_free_block) == p_used_block.c)
        {
            p_free_block.f->size += p_used_block.u->size;
            if(p_free_block.f->free)
            {
                /* coalesce with upper free block too? */
                P_RISCOS_HEAP_FREE_DATA p_upper_free_block;
                p_upper_free_block.c = P_NEXT_FREE(p_free_block);
                if(P_END_OF_FREE(p_free_block) == p_upper_free_block.c)
                {
                    p_free_block.f->size += p_upper_free_block.f->size;
                    p_free_block.f->free = p_upper_free_block.f->free ? P_NEXT_FREE(p_upper_free_block) - p_free_block.c : 0;
                    CLEAR_FREE(p_free_block);
                }
            }
            else
            {
                CLEAR_FREE(p_free_block);

                /* coalesce with end block too? */
                if(P_HEAP_HWM(heap) == P_END_OF_FREE(p_free_block))
                {
                    *p_free_offset = 0;
                    heap->hwm -= p_free_block.f->size;
                }
            }
            return;
        }

        if(p_free_block.c > p_used_block.c)
        {
            P_RISCOS_HEAP_FREE_DATA r;
            assert(p_free_block.c >= P_END_OF_USED(p_used_block));

            r.v = p_used_block.v;
            *p_free_offset = r.c - (P_U8) p_free_offset;
            r.f->size = p_used_block.u->size;

            if(P_END_OF_FREE(r) == p_free_block.c)
            {
                /* coalesce with upper free block */
                r.f->size += p_free_block.f->size;
                r.f->free = p_free_block.f->free ? P_NEXT_FREE(p_free_block) - r.c : 0;
            }
            else
                /* found position to insert at */
                r.f->free = p_free_block.c - (P_U8) &r.f->free;

            CLEAR_FREE(r);
            return;
        }

        p_free_offset = &p_free_block.f->free;
    }

    p_free_block.v = p_used_block.v;
    p_free_block.f->size = p_used_block.u->size;
    p_free_block.f->free = 0; /* at end of free list */

    CLEAR_FREE(p_free_block);

    /* try coalescing with end block */
    if(P_HEAP_HWM(heap) == P_END_OF_FREE(p_free_block))
    {
        heap->hwm -= p_free_block.f->size;
        return;
    }

    /* add to end of free list */
    *p_free_offset = p_free_block.c - (P_U8) p_free_offset;
}

static P_ANY
riscos_ptr_realloc_grow(
    P_ANY p_any,
    _InVal_     U32 new_blksize,
    _InVal_     U32 cur_blksize,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    const P_RISCOS_HEAP heap = ahp->heap;
    U32   size_diff = new_blksize - cur_blksize;
    P_RISCOS_HEAP_USED_DATA p_used_block, p_new_used_block;
    P_RISCOS_HEAP_FREE_DATA p_free_block, p_lower_free_block, p_upper_free_block, p_fit_free_block;
    U32 * p_free_offset;
    U32 * p_fit_free_offset;
    U32   next_free_offset;

#if defined(ALLOC_TRACK_USAGE)
    fprintf(stderr, "realloc:") PTR_XTFMT TEXT(", %x, %x", p_any, new_blksize, cur_blksize);
#endif

    p_used_block.v = p_any;
    p_used_block.u -= 1;

    p_lower_free_block.f = p_upper_free_block.f = NULL;

    p_fit_free_block.f = NULL;
    p_fit_free_offset = NULL; /* keep dataflower happy */

    p_free_block.v = p_free_offset = &heap->free;

    /* easiest case is if the block we are reallocing is adjacent to the hwm (also special case for heap growth parms) */
    if(P_HEAP_HWM(heap) == P_END_OF_USED(p_used_block))
    {
        if(heap->size - heap->hwm >= size_diff)
        { USED_PROCESS(1) /*EMPTY*/ }
        else
        {
            /* look to see if there is a free block immediately below too that we can use */
            for(;;)
            {
                if((next_free_offset = *p_free_offset) == 0)
                {
                    USED_PROCESS(2);
                    if(!alloc_needtoallocate(size_diff, ahp))
                    {
                        trace_0(TRACE_MODULE_ALLOC, TEXT("|yields NULL because heap extension failed (1a)"));
                        return(NULL);
                    }
                    break;
                }

                assert(next_free_offset < heap->size);
                p_free_block.c += next_free_offset;

                if(P_END_OF_FREE(p_free_block) == p_used_block.c)
                {
                    /* now try to use this lower free block: we need to ask for less core, but need to move the block */
                    USED_PROCESS(3);
                    assert(!p_free_block.f->free);
                    if(heap->size - (heap->hwm - p_free_block.f->size) < size_diff)
                        if(!alloc_needtoallocate(size_diff - p_free_block.f->size, ahp))
                        {
                            trace_0(TRACE_MODULE_ALLOC, TEXT("|yields NULL because heap extension failed (1b)"));
                            return(NULL);
                        }

                    heap->hwm -= p_free_block.f->size; /* hwm becomes invalid for a mo */
                    *p_free_offset = 0;
                    memmove32(p_free_block.f, p_used_block.u, p_used_block.u->size);
                    p_used_block.v = p_free_block.v;
                    break;
                }

                /* can we sneak in a first fit allocation instead? */
                if(new_blksize <= p_free_block.f->size)
                {
                    /* return this block (or part thereof) */
                    USED_PROCESS((new_blksize == p_free_block.f->size) ? 14 : 4);
                    p_fit_free_block.f = p_free_block.f;
                    p_fit_free_offset  = p_free_offset;
                    goto use_fit_free_block;
                }

                p_free_offset = &p_free_block.f->free;
            }
        }

        /* trivial allocation at hwm now kosher */
        heap->hwm += size_diff;
        p_used_block.u->size = new_blksize;
        return(p_used_block.u + 1);
    }

    /* look to see if there is a free block immediately below and/or above that we can use or one in the middle that would do for us */
    while((next_free_offset = *p_free_offset) != 0)
    {
        assert(next_free_offset < heap->size);
        p_free_block.c += next_free_offset;

        /* usable lower free block? */
        if(P_END_OF_FREE(p_free_block) == p_used_block.c)
        {
            p_lower_free_block.f = p_free_block.f;
            if(p_lower_free_block.f->free)
            {
                /* usable upper free block too? */
                p_upper_free_block.c = P_NEXT_FREE(p_lower_free_block);
                if(P_END_OF_USED(p_used_block) != p_upper_free_block.c)
                    p_upper_free_block.f = NULL;
            }

            break;
        }

        if(P_END_OF_USED(p_used_block) <= p_free_block.c)
        {
            /* usable p_upper_free_block block? */
            if(P_END_OF_USED(p_used_block) == p_free_block.c)
                p_upper_free_block.f = p_free_block.f;

            break;
        }

        /* could we sneak in a first fit allocation instead? */
        if(new_blksize <= p_free_block.f->size)
        {
            /* either a first fit or an overriding exact fit or a subsequent better fit */
            if(!p_fit_free_block.f
            || (new_blksize == p_free_block.f->size)
            || (p_fit_free_block.f->size > p_free_block.f->size))
            {
                p_fit_free_block.f = p_free_block.f;
                p_fit_free_offset  = p_free_offset;
            }
        }

        p_free_offset = &p_free_block.f->free;
    }

    if(p_upper_free_block.f)
    {
        if(p_upper_free_block.f->size >= size_diff)
        {
            /* if we have a lower free block too then this is the free chain link we must patch */
            if(p_lower_free_block.f)
                p_free_offset = &p_lower_free_block.f->free;

            if(p_upper_free_block.f->size == size_diff)
            {
                /* new allocation fits exactly using upper free block */
                USED_PROCESS(5);
                /* remove upper free block from list */
                *p_free_offset = p_upper_free_block.f->free ? P_NEXT_FREE(p_upper_free_block) - (P_U8) p_free_offset : 0;
            }
            else
            {
                /* new allocation fits completely using upper free block */
                RISCOS_FREE_BLOCK upper_F;
                USED_PROCESS(6);
                upper_F = *p_upper_free_block.f;
                /* upper free block now further away than before */
                *p_free_offset += size_diff;
                p_upper_free_block.c += size_diff;
                /* and is itself closer to the entry above it */
                p_upper_free_block.f->free = upper_F.free ? upper_F.free - size_diff : 0;
                p_upper_free_block.f->size = upper_F.size - size_diff;
                CLEAR_FREE(p_upper_free_block);
            }
            p_used_block.u->size = new_blksize;
            return(p_used_block.u + 1);
        }

        if(p_lower_free_block.f)
        {
            if(p_upper_free_block.f->size + p_lower_free_block.f->size >= size_diff)
            {
                if(p_upper_free_block.f->size + p_lower_free_block.f->size == size_diff)
                {
                    /* new allocation fits exactly using both lower and upper free blocks */
                    USED_PROCESS(7);
                    /* remove both lower and upper free blocks from list */
                    *p_free_offset = p_upper_free_block.f->free ? P_NEXT_FREE(p_upper_free_block) - (P_U8) p_free_offset : 0;
                    memmove32(p_lower_free_block.f, p_used_block.u, p_used_block.u->size);
                    p_used_block.v = p_lower_free_block.v;
                    p_used_block.u->size = new_blksize;
                }
                else
                {
                    /* new allocation fits completely using both lower and upper free blocks */
                    P_U8 p_next_free;
                    U32 lower_F_size;
                    U32 upper_F_size;
                    USED_PROCESS(8);
                    p_next_free = p_upper_free_block.f->free ? P_NEXT_FREE(p_upper_free_block) : NULL;
                    lower_F_size = p_lower_free_block.f->size;
                    upper_F_size = p_upper_free_block.f->size;
                    memmove32(p_lower_free_block.f, p_used_block.u, p_used_block.u->size);
                    p_used_block.v = p_lower_free_block.v;
                    p_used_block.u->size = new_blksize;
                    /* now just one free block, above the used block, at a completely different place */
                    p_upper_free_block.c = P_END_OF_USED(p_used_block);
                    *p_free_offset = p_upper_free_block.c - (P_U8) p_free_offset;
                    p_upper_free_block.f->free = p_next_free ? p_next_free - (P_U8) &p_upper_free_block.f->free : 0;
                    p_upper_free_block.f->size = lower_F_size + upper_F_size - size_diff;
                    CLEAR_FREE(p_upper_free_block);
                }
                return(p_used_block.u + 1);
            }
        }

        p_free_block.f = p_upper_free_block.f;
        p_free_offset = &p_free_block.f->free;
    }
    else if(p_lower_free_block.f)
    {
        if(p_lower_free_block.f->size >= size_diff)
        {
            if(p_lower_free_block.f->size == size_diff)
            {
                /* new allocation fits exactly using lower free block */
                USED_PROCESS(9);
                /* remove lower free block from list */
                *p_free_offset = p_lower_free_block.f->free ? P_NEXT_FREE(p_lower_free_block) - (P_U8) p_free_offset : 0;
                memmove32(p_lower_free_block.f, p_used_block.u, p_used_block.u->size);
                p_used_block.v = p_lower_free_block.v;
                p_used_block.u->size = new_blksize;
            }
            else
            {
                /* new allocation fits completely using lower free block */
                P_U8 p_next_free;
                U32 lower_F_size;
                USED_PROCESS(10);
                p_next_free = p_lower_free_block.f->free ? P_NEXT_FREE(p_lower_free_block) : NULL;
                lower_F_size = p_lower_free_block.f->size;
                memmove32(p_lower_free_block.f, p_used_block.u, p_used_block.u->size);
                p_used_block.v = p_lower_free_block.v;
                p_used_block.u->size = new_blksize;
                /* free block now above the used block, at a completely different place */
                p_upper_free_block.c = P_END_OF_USED(p_used_block);
                *p_free_offset = p_upper_free_block.c - (P_U8) p_free_offset;
                /* and is itself closer to the entry above it */
                p_upper_free_block.f->free = p_next_free ? p_next_free - (P_U8) &p_upper_free_block.f->free : 0;
                p_upper_free_block.f->size = lower_F_size - size_diff;
                CLEAR_FREE(p_upper_free_block);
            }
            return(p_used_block.u + 1);
        }

        p_free_block.f = p_lower_free_block.f;
        p_free_offset = &p_free_block.f->free;
    }
    else if(*p_free_offset != 0)
        p_free_offset = &p_free_block.f->free;

    /* if we didn't get a fit block, continue looping up free list, look for first fit */
    if(p_fit_free_block.f)
    { USED_PROCESS(11) /*EMPTY*/ }
    else
    {
        while((next_free_offset = *p_free_offset) != 0)
        {
            assert(next_free_offset < heap->size);
            p_free_block.c += next_free_offset;

            /* can we sneak in a first fit allocation instead? */
            if(new_blksize <= p_free_block.f->size)
            {
                /* return this block (or part thereof) */
                p_fit_free_block.f = p_free_block.f;
                p_fit_free_offset  = p_free_offset;
                USED_PROCESS((new_blksize == p_free_block.f->size) ? 15 : 12);
                break;
            }

            p_free_offset = &p_free_block.f->free;
        }
    }

    if(p_fit_free_block.f)
    {
    use_fit_free_block:;

        /* will return either all or part of this block */
        p_new_used_block.v = p_fit_free_block.v;

        if(new_blksize == p_fit_free_block.f->size)
        {
            /* take free block away entirely */
             *p_fit_free_offset = p_fit_free_block.f->free ? *p_fit_free_offset + p_fit_free_block.f->free : 0;
         }
        else /* if(new_blksize < p_fit_free_block.f->size) */
        {
            /* allocate core from front of free block */
            RISCOS_FREE_BLOCK F = *p_fit_free_block.f;
            /* this free block is now further away than before */
            *p_fit_free_offset += new_blksize;
            p_fit_free_block.c += new_blksize;
            /* next free block after this one is now closer than before */
            p_fit_free_block.f->free = F.free ? F.free - new_blksize : 0;
            p_fit_free_block.f->size = F.size - new_blksize;
            CLEAR_FREE(p_fit_free_block);
        }
    }
    else
    {
        /* trivial allocation at hwm  - so now we need to raise the hwm by new_blksize */
        USED_PROCESS(13);

        if(!alloc_needtoallocate(new_blksize, ahp))
        {
            trace_0(TRACE_MODULE_ALLOC, TEXT("|yields NULL because heap extension failed (2)"));
            return(NULL);
        }

        p_new_used_block.c = P_HEAP_HWM(heap);
        heap->hwm += new_blksize;
    }

    memmove32(p_new_used_block.u, p_used_block.u, p_used_block.u->size);
    p_new_used_block.u->size = new_blksize;

    riscos_ptr_free(p_used_block.u, ahp);

    return(p_new_used_block.u + 1);
}

static P_ANY
riscos_ptr_realloc_shrink(
    P_ANY p_any,
    _InVal_     U32 new_blksize,
    _InVal_     U32 cur_blksize,
    _InRef_     PC_ALLOC_HEAP_DESC ahp)
{
    P_RISCOS_HEAP_USED_DATA p_used_block;

    /* update size of block we're keeping */
    p_used_block.v = p_any;
    p_used_block.u -= 1;
    p_used_block.u->size = new_blksize;

    /* free the end bit */
    p_used_block.c += p_used_block.u->size;
    p_used_block.u->size = cur_blksize - new_blksize;

    riscos_ptr_free(p_used_block.u, ahp);

    return(p_any);
}

#endif /* USE_HEAP_SWIS */

#if !RELEASED
#define PRAGMA_CHECK_STACK_ON
#include "coltsoft/pragma.h"
#endif

/*
the barfing heap function set
*/

static P_ANY
alloc__barf_calloc(
    _InVal_     U32 num,
    _InVal_     U32 size)
{
    assert0();
    return(alloc__calloc(num, size, &alloc_main_heap_desc));
}

static void
alloc__barf_free(
    P_ANY a)
{
    assert0();
    alloc__free(a, &alloc_main_heap_desc);
}

static P_ANY
alloc__barf_malloc(
    _InVal_     U32 size)
{
    assert0();
    return(alloc__malloc(size, &alloc_main_heap_desc));
}

static P_ANY
alloc__barf_realloc(
    P_ANY a,
    _InVal_     U32 size)
{
    assert0();
    return(alloc__realloc(a, size, &alloc_main_heap_desc));
}

ALLOC_FUNCTION_SET
alloc_barf =
{
    alloc__barf_calloc,
    alloc__barf_free,
    alloc__barf_malloc,
    alloc__barf_realloc
};

#endif /* RISCOS */

/* end of alloc.c */
