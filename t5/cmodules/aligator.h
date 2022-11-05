/* aligator.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header file for aligator.c */

/* MRJC December 1991 */

#ifndef __aligator_h
#define __aligator_h

#if defined(ALIGATOR_USE_ALLOC) && !defined(__alloc_h)
#include "cmodules/alloc.h"
#endif

/*
prototypes for full-event clients
*/

typedef U32 (* P_PROC_AL_FULL_EVENT) (
    _InVal_     U32 n_bytes);

_Check_return_
extern STATUS
al_full_event_client_register(
    _In_        P_PROC_AL_FULL_EVENT proc);

/*
malloc() replacement
*/

_Check_return_
_Ret_writes_to_maybenull_(n_bytes, 0) /* may be NULL */
extern P_BYTE
_al_ptr_alloc(
    _InVal_     U32 n_bytes,
    _OutRef_    P_STATUS p_status);

#define al_ptr_alloc_bytes(__ptr_type, n_bytes, p_status) ( \
    (__ptr_type) _al_ptr_alloc((n_bytes), p_status) )

#define al_ptr_alloc_elem(__base_type, n_elem, p_status) ( \
    (__base_type *) _al_ptr_alloc((n_elem) * sizeof(__base_type), p_status) )

#if WINDOWS

_Check_return_
_Ret_writes_to_maybenull_(n_bytes, 0) /* may be NULL */
extern P_BYTE
_dsapplib_ptr_alloc(
    _InVal_     U32 n_bytes,
    _OutRef_    P_STATUS p_status);

#endif /* WINDOWS */

/*
calloc() replacement
*/

_Check_return_
_Ret_writes_maybenull_(n_bytes) /* may be NULL */
extern P_BYTE
_al_ptr_calloc(
    _InVal_     U32 n_bytes,
    _OutRef_    P_STATUS p_status);

#define al_ptr_calloc_bytes(__ptr_type, n_bytes, p_status) ( \
    (__ptr_type) _al_ptr_calloc((n_bytes), p_status) )

#define al_ptr_calloc_elem(__base_type, n_elem, p_status) ( \
    (__base_type *) _al_ptr_calloc((n_elem) * sizeof(__base_type), p_status) )
/* NB different args and arg order to calloc() */

/*
free() replacement
*/

extern void
al_ptr_free(
    _Pre_maybenull_ _Post_invalid_ P_ANY p_any);

static inline void
al_ptr_dispose(
    _InoutRef_opt_ P_P_ANY p_p_any)
{
    if(NULL != p_p_any)
    {
        P_ANY p_any = *p_p_any;

        if(NULL != p_any)
        {
            *p_p_any = NULL;
            al_ptr_free(p_any);
        }
    }
}

#if WINDOWS

extern void
dsapplib_ptr_free(
    _Pre_maybenull_ _Post_invalid_ P_ANY p_any);

#endif /* WINDOWS */

/*
realloc() replacement
*/

_Check_return_
_Ret_writes_to_maybenull_(n_bytes, 0) /* may be NULL */
extern P_BYTE
_al_ptr_realloc(
    _Pre_maybenull_ _Post_invalid_ P_ANY p_any,
    _InVal_     U32 n_bytes,
    _OutRef_    P_STATUS p_status);

#define al_ptr_realloc_bytes(__ptr_type, p_any, n_bytes, p_status) ( \
    (__ptr_type) _al_ptr_realloc(p_any, (n_bytes), p_status) )

#define al_ptr_realloc_elem(__base_type, p_any, n_elem, p_status) ( \
    (__base_type *) _al_ptr_realloc(p_any, (n_elem) * sizeof(__base_type), p_status) )

#if WINDOWS

_Check_return_
_Ret_writes_to_maybenull_(n_bytes, 0) /* may be NULL */
extern P_BYTE
_dsapplib_ptr_realloc(
    _Pre_maybenull_ _Post_invalid_ P_ANY p_any,
    _InVal_     U32 n_bytes,
    _OutRef_    P_STATUS p_status);

#endif /* WINDOWS */

/*
definition of al_array_xxx index type
*/

typedef S32 ARRAY_INDEX; typedef ARRAY_INDEX * P_ARRAY_INDEX;

#define ARRAY_INDEX_TFMT S32_TFMT

typedef U32 ARRAY_HANDLE; typedef ARRAY_HANDLE * P_ARRAY_HANDLE, ** P_P_ARRAY_HANDLE; typedef const ARRAY_HANDLE * PC_ARRAY_HANDLE;

#define P_ARRAY_HANDLE_NONE _P_DATA_NONE(P_ARRAY_HANDLE)

#define ARRAY_HANDLE_TFMT TEXT("h:") U32_TFMT

/*
structure of array space allocator
*/

typedef struct ARRAY_BLOCK_PARMS
{
    /* private to aligator - export only for macros */
    UBF auto_compact            : 1;
    UBF compact_off             : 1;
    UBF entry_free              : 1;
    UBF clear_new_block         : 1;
#define ALLOC_USE_ALLOC         0 /* use standard allocator */
#if WINDOWS
#define ALLOC_USE_GLOBAL_ALLOC  1 /* use GlobalAlloc()/GlobalLock()/GlobalFree() for HANDLE based allocation (e.g. clipboard) */
#define ALLOC_USE_DS_ALLOC      2 /* use Dial Solutions DSAppLib allocator for Draw files */
    UBF use_alloc               : 2;
    UBF _spare                  : (1+14)-2;
    UBF packed_size_increment   : 13;
#else
    UBF _spare                  : 1;
    UBF packed_element_size     : 14; /* needed visible for array_element_size32() */
    UBF packed_size_increment   : 13;
    /* SKS after 1.06 09nov93 reordered to give ARM compiler a good chance to use those wonderful shift operators */
#endif
}
ARRAY_BLOCK_PARMS;

typedef struct ARRAY_BLOCK
{
    /* private to aligator - export only for macros */
    P_BYTE              p_data;
    ARRAY_INDEX         used_elements;
    ARRAY_INDEX         size;
    ARRAY_BLOCK_PARMS   parms;

#if WINDOWS /* SKS 24feb2012 unpacked */
    U32                 element_size;
#endif /* WINDOWS */
}
ARRAY_BLOCK, * P_ARRAY_BLOCK; typedef const ARRAY_BLOCK * PC_ARRAY_BLOCK;

#define P_ARRAY_BLOCK_NONE _P_DATA_NONE(P_ARRAY_BLOCK)

#if WINDOWS
/* return size of an element in array (with valid block) */
#define array_block_element_size(p_array_block) \
    (p_array_block->element_size)
#else
/* return size of an element in array (with valid block) */
#define array_block_element_size(p_array_block) \
    ((U32) p_array_block->parms.packed_element_size)
#endif /* OS */

#define array_block_size_increment(p_array_block) \
    ((U32) p_array_block->parms.packed_size_increment)

/*
different typedef for root allocation allows us to see better in debugger
e.g. watch and expand
    array_root.p_array_block[<array_handle>]
and it also makes implementation in aligator.c simpler
*/

typedef struct ARRAY_ROOT_BLOCK
{
    /* private to aligator - export only for macros */
    PC_ARRAY_BLOCK      p_array_block; /* NB const makes for safer access outside of aligator */
    U32                 used_handles; /* same as 'used_elements' in ARRAY_BLOCK */
    U32                 size;
    ARRAY_BLOCK_PARMS   parms;

#if WINDOWS /* SKS 24feb2012 unpacked */
    U32                 element_size;
#endif /* WINDOWS */
}
ARRAY_ROOT_BLOCK;

/* NB. SKS 1.03 19mar93 made handle zero info kosher - i.e. NULL pointer, zero size, zero element size etc. */

/*
functions as macros
*/

/* NB These ones are for aligator internal use only (when handle has been validated) */

/* return pointer to first array element */
#define array_base_no_checks(pc_array_handle, __base_type) ( \
    ((__base_type *) (array_blockc_no_checks(pc_array_handle)->p_data)) )

/* return const pointer to first array element */
#define array_basec_no_checks(pc_array_handle, __base_type) ( \
    ((const __base_type *) (array_blockc_no_checks(pc_array_handle)->p_data)) )

/* return const pointer to array block for given handle - NB. for internal use only */
#define array_blockc_no_checks(pc_array_handle) ( \
    array_root.p_array_block + *(pc_array_handle) )

/* return number of used elements in array */
#define array_elements_no_checks(pc_array_handle) ( \
    array_blockc_no_checks(pc_array_handle)->used_elements )

/* return number of used elements in array (unsigned 32-bit) */
#define array_elements32_no_checks(pc_array_handle) \
    (U32) array_elements_no_checks(pc_array_handle)

/* return size of an element in array */
#define array_element_size32_no_checks(pc_array_handle) (  \
    array_block_element_size(array_blockc_no_checks(pc_array_handle)) )

/* return pointer to array element - NB. pc_array_handle must point to a valid handle */
#define array_ptr_no_checks(pc_array_handle, __base_type, ele_index) ( \
    ((__base_type *) array_blockc_no_checks(pc_array_handle)->p_data) + (ele_index) )

#define array_ptr32_no_checks(pc_array_handle, __base_type, ele_index) ( \
    ((__base_type *) array_blockc_no_checks(pc_array_handle)->p_data) + (ele_index) )

/* return const pointer to array element */
#define array_ptrc_no_checks(pc_array_handle, __base_type, ele_offset) ( \
    ((const __base_type *) array_blockc_no_checks(pc_array_handle)->p_data) + (ele_offset) )

#define array_ptr32c_no_checks(pc_array_handle, __base_type, ele_offset) ( \
    ((const __base_type *) array_blockc_no_checks(pc_array_handle)->p_data) + (ele_offset) )

#if CHECKING

/* return pointer to first array element */

_Check_return_
_Ret_/*opt_*/
extern P_BYTE /* may be P_BYTE_NONE */
array_base_check(
    _InRef_     PC_ARRAY_HANDLE pc_array_handle);

#define array_base(pc_array_handle, __base_type) \
    ((__base_type *) array_base_check(pc_array_handle))

#define array_basec(pc_array_handle, __base_type) \
    ((const __base_type *) array_base_check(pc_array_handle))

/* return pointer to array block for given handle - NB. for internal use only */

_Check_return_
_Ret_ /* may be P_ARRAY_BLOCK_NONE */
extern PC_ARRAY_BLOCK
array_block_check(
    _InRef_     PC_ARRAY_HANDLE pc_array_handle);

#define array_blockc(pc_array_handle) \
    array_block_check(pc_array_handle)

/* return number of used elements in array */

_Check_return_
extern ARRAY_INDEX
array_elements_check(
    _InRef_     PC_ARRAY_HANDLE pc_array_handle);

#define array_elements(pc_array_handle) \
    array_elements_check(pc_array_handle)

#define array_elements32(pc_array_handle) \
    (U32) array_elements_check(pc_array_handle)

/* return pointer to array element */

_Check_return_
_Ret_writes_(ele_size)
extern P_BYTE /* may be P_BYTE_NONE */
array_ptr_check(
    _InRef_     PC_ARRAY_HANDLE pc_array_handle,
    _InVal_     ARRAY_INDEX ele_index,
    _InVal_     U32 ele_size);

#define array_ptr(pc_array_handle, __base_type, ele_index) ( \
    ((__base_type *) array_ptr_check(pc_array_handle, (ele_index), sizeof32(__base_type))) )

#define array_ptrc(pc_array_handle, __base_type, ele_index) ( \
    ((const __base_type *) array_ptr_check(pc_array_handle, (ele_index), sizeof32(__base_type))) )

_Check_return_
_Ret_writes_(ele_size)
extern P_BYTE /* may be P_BYTE_NONE */
array_ptr32_check(
    _InRef_     PC_ARRAY_HANDLE pc_array_handle,
    _InVal_     U32 ele_offset,
    _InVal_     U32 ele_size);

#define array_ptr32(pc_array_handle, __base_type, ele_offset) ( \
    ((__base_type *) array_ptr32_check(pc_array_handle, (ele_offset), sizeof32(__base_type))) )

#define array_ptr32c(pc_array_handle, __base_type, ele_offset) ( \
    ((const __base_type *) array_ptr32_check(pc_array_handle, (ele_offset), sizeof32(__base_type))) )

/* return pointer to n array elements of specified ele_size at given offset */
_Check_return_
_Ret_writes_(total_n_bytes) /* may be P_BYTE_NONE */
extern P_BYTE
array_range_check(
    _InRef_     PC_ARRAY_HANDLE pc_array_handle,
    _InVal_     ARRAY_INDEX ele_index,
    _InVal_     U32 ele_size
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 total_n_bytes) );

_Check_return_
_Ret_writes_(total_n_bytes) /* may be P_BYTE_NONE */
extern PC_BYTE
array_rangec_check(
    _InRef_     PC_ARRAY_HANDLE pc_array_handle,
    _InVal_     ARRAY_INDEX ele_index,
    _InVal_     U32 ele_size
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 total_n_bytes) );

/* return pointer to n array elements at given offset - NB. p_handle must point to a valid handle */
#define array_range(pc_array_handle, __base_type, ele_index, n_elements) \
    ((__base_type *) array_range_check(pc_array_handle, (ele_index), sizeof32(__base_type) CODE_ANALYSIS_ONLY_ARG(sizeof32(__base_type) * (n_elements))))

#define array_rangec(pc_array_handle, __base_type, ele_index, n_elements) \
    ((const __base_type *) array_rangec_check(pc_array_handle, (ele_index), sizeof32(__base_type) CODE_ANALYSIS_ONLY_ARG(sizeof32(__base_type) * (n_elements))))

#define array_range_generic(pc_array_handle, ele_size, ele_index, n_elements) \
    ((P_ANY) array_range_check(pc_array_handle, (ele_index), ele_size CODE_ANALYSIS_ONLY_ARG(ele_size * (n_elements))))

/* return pointer to n bytes at given offset */
_Check_return_
_Ret_writes_(n_bytes) /* may be P_BYTE_NONE */
extern P_BYTE
array_range_bytes_check(
    _InRef_     PC_ARRAY_HANDLE pc_array_handle,
    _InVal_     U32 byte_offset
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 n_bytes) );

#define array_range_bytes(pc_array_handle, __ptr_type, byte_offset, n_bytes) \
    ((__ptr_type) array_range_bytes_check(pc_array_handle, (byte_offset) CODE_ANALYSIS_ONLY_ARG(n_bytes)))

#else /* NOT CHECKING */

/* return pointer to first array element */
#define array_base(pc_array_handle, __base_type) ( \
    ((__base_type *) (array_blockc(pc_array_handle)->p_data)) )

#define array_basec(pc_array_handle, __base_type) ( \
    ((const __base_type *) (array_blockc(pc_array_handle)->p_data)) )

/* return const pointer to array block for given handle - NB. for internal use only */
#define array_blockc(pc_array_handle) ( \
    array_root.p_array_block + *(pc_array_handle) )

/* return number of used elements in array */
#define array_elements(pc_array_handle) ( \
    array_blockc(pc_array_handle)->used_elements )

/* return number of used elements in array (unsigned 32-bit) */
#define array_elements32(pc_array_handle) ( \
    (U32) array_blockc(pc_array_handle)->used_elements )

/* return pointer to array element - NB. pc_array_handle must point to a valid handle */
#define array_ptr(pc_array_handle, __base_type, ele_index) ( \
    ((__base_type *) array_blockc(pc_array_handle)->p_data) + (ele_index) )

#define array_ptrc(pc_array_handle, __base_type, ele_index) ( \
    ((const __base_type *) array_blockc(pc_array_handle)->p_data) + (ele_index) )

#define array_ptr32(pc_array_handle, __base_type, ele_offset) ( \
    ((__base_type *) array_blockc(pc_array_handle)->p_data) + (ele_offset) )

#define array_ptr32c(pc_array_handle, __base_type, ele_offset) ( \
    ((const __base_type *) array_blockc(pc_array_handle)->p_data) + (ele_offset) )

/* return pointer to n array elements at given offset - NB. p_handle must point to a valid handle */
#define array_range(pc_array_handle, __base_type, ele_index, n_elements) \
    array_ptr(pc_array_handle, __base_type, ele_index)

#define array_rangec(pc_array_handle, __base_type, ele_index, n_elements) \
    array_ptrc(pc_array_handle, __base_type, ele_index)

#define array_range_generic(p_handle, ele_size, ele_index, n_elements) \
    PtrAddBytes(P_ANY, array_blockc(pc_array_handle)->p_data, (ele_index) * (ele_size))

#define array_range_bytes(pc_array_handle, __ptr_type, byte_offset, n_bytes) \
    PtrAddBytes(__ptr_type, array_blockc(pc_array_handle)->p_data, byte_offset)

#endif /* CHECKING */

/* return the size of an element in array */
#define array_element_size32(pc_array_handle) (  \
    array_block_element_size(array_blockc(pc_array_handle)) )

/* return whether given array handle is valid (NB doesn't check for handle zero) */
#define array_handle_is_valid(pc_array_handle) ( \
    (U32) *(pc_array_handle) < array_root.used_handles )

/* return whether given index is valid in array */
#define array_index_is_valid(pc_array_handle, ele_index) ( \
    (U32) (ele_index) < array_elements32(pc_array_handle) )

#define array_offset_is_valid(pc_array_handle, ele_offset) ( \
    (U32) (ele_offset) < array_elements32(pc_array_handle) )

/* return the element index of a pointer to an element in an array */
#define array_indexof_element(p_handle, __base_type, ptr) ( \
    (ARRAY_INDEX) ((ptr) - array_base(p_handle, __base_type)) )

/* return the size of an array (NB includes unallocated elements; you probably want array_elements) */
#define array_size32(pc_array_handle) ( \
    (U32) array_blockc(pc_array_handle)->size )

/*
block passed to al_array_alloc/realloc
*/

typedef struct ARRAY_INIT_BLOCK
{
    ARRAY_INDEX size_increment;     /* number of array elements to allocate at a time */
    U32         element_size;       /* sizeof32() the type stored */
    U8          clear_new_block;    /* boolean; zeros allocated chunks */
    U8          use_alloc;          /* see ARRAY_BLOCK_PARMS */
    U8          _spare[2];
}
ARRAY_INIT_BLOCK; typedef const ARRAY_INIT_BLOCK * PC_ARRAY_INIT_BLOCK;

#define PC_ARRAY_INIT_BLOCK_NONE _P_DATA_NONE(PC_ARRAY_INIT_BLOCK)

#define SC_ARRAY_INIT_BLOCK static const ARRAY_INIT_BLOCK /* an ARRAY_INIT_BLOCK out in const area */

#define aib_init(size_increment, element_size, clear_new_block) \
    { (size_increment), (element_size), (clear_new_block), 0, { 0, 0 } }

#define array_init_block_setup(block, a, b, c) ( \
    (block)->size_increment   = (a), \
    (block)->element_size     = (b), \
    (block)->clear_new_block  = (c), \
    (block)->use_alloc        = ALLOC_USE_ALLOC )

/*
remove deleted info
*/

typedef BOOL (* P_PROC_ELEMENT_IS_DELETED) (
    P_ANY p_any);

#define PROC_ELEMENT_IS_DELETED_PROTO(_e_s, _proc_name) \
_e_s BOOL \
_proc_name( \
    P_ANY p_any)

typedef struct AL_GARBAGE_FLAGS
{
    UBF remove_deleted : 1;
    UBF shrink : 1;
    UBF may_dispose : 1;
    UBF spare_1 : 8 - (3*1);
    UBF spare_2 : 8;
    UBF spare_3 : 8;
    UBF spare_4 : 8;
}
AL_GARBAGE_FLAGS, * P_AL_GARBAGE_FLAGS;

#define AL_GARBAGE_FLAGS_INIT \
    { 0, 0, 0,  0, 0, 0, 0 } /* this aggregate initialiser gives poor code on Norcroft */

/* better to do by hand on Norcroft */
#define AL_GARBAGE_FLAGS_CLEAR(flags) \
    zero_32(flags)

/*
exported data
*/

extern ARRAY_ROOT_BLOCK array_root;

/*
exported routines
*/

_Check_return_
extern STATUS
aligator_init(void);

extern void
aligator_exit(void);

_Check_return_
extern STATUS
_al_array_add(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InVal_     U32 num_elements,
    _InRef_maybenone_ PC_ARRAY_INIT_BLOCK p_array_init_block,
    _In_reads_bytes_(bytesof_elem_x_num_elem) PC_ANY p_data_in /*copied*/
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem_x_num_elem));

#define al_array_add(p_array_handle, __base_type, num_elements, p_array_init_block, p_data_in) \
    _al_array_add(p_array_handle, num_elements, p_array_init_block, p_data_in \
    CODE_ANALYSIS_ONLY_ARG((num_elements) * sizeof32(__base_type)))

_Check_return_
_Ret_writes_maybenull_(bytesof_elem_x_num_elem)
extern P_BYTE
_al_array_alloc(
    _OutRef_    P_ARRAY_HANDLE p_array_handle,
    _InVal_     U32 num_elements,
    _InRef_     PC_ARRAY_INIT_BLOCK p_array_init_block,
    _OutRef_    P_STATUS p_status
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem_x_num_elem));

#define al_array_alloc(p_array_handle, __base_type, num_elements, p_array_init_block, p_status) ( \
    (__base_type *) _al_array_alloc(p_array_handle, num_elements, p_array_init_block, p_status \
    CODE_ANALYSIS_ONLY_ARG((num_elements) * sizeof32(__base_type))) )

#define AL_ARRAY_ALLOC_PROTO(_e_s, __base_type) \
_Check_return_ \
_Ret_writes_maybenull_(num_elements)  /* may be NULL */ \
_e_s __base_type * /* pointer to new allocation if ok, NULL if failed */ \
al_array_alloc_ ## __base_type( \
    _OutRef_    P_ARRAY_HANDLE p_array_handle, \
    _InVal_     U32 num_elements, \
    _InRef_     PC_ARRAY_INIT_BLOCK p_array_init_block, \
    _OutRef_    P_STATUS p_status)

#define AL_ARRAY_ALLOC_IMPL(_e_s, __base_type) \
__pragma(warning(push)) /*__pragma(warning(disable:6386))*/ /* mask the 'return value' Buffer overrun warning */ \
_Check_return_ \
_Ret_writes_maybenull_(num_elements)  /* may be NULL */ \
_e_s __base_type * /* pointer to new allocation if ok, NULL if failed */ \
al_array_alloc_ ## __base_type( \
    _OutRef_    P_ARRAY_HANDLE p_array_handle, \
    _InVal_     U32 num_elements, \
    _InRef_     PC_ARRAY_INIT_BLOCK p_array_init_block, \
    _OutRef_    P_STATUS p_status) \
{ \
    return((__base_type *) _al_array_alloc(p_array_handle, num_elements, p_array_init_block, p_status \
    CODE_ANALYSIS_ONLY_ARG((num_elements) & sizeof32(__base_type)) \
    )); \
} \
__pragma(warning(pop))

_Check_return_
extern STATUS
al_array_alloc_zero(
    _OutRef_    P_ARRAY_HANDLE p_array_handle,
    _InRef_     PC_ARRAY_INIT_BLOCK p_array_init_block);

_Check_return_
extern STATUS
al_array_preallocate_zero(
    _OutRef_    P_ARRAY_HANDLE p_array_handle,
    _InRef_     PC_ARRAY_INIT_BLOCK p_array_init_block);

extern void
al_array_auto_compact_set(
    _InRef_     PC_ARRAY_HANDLE p_array_handle);

_Check_return_
extern ARRAY_INDEX
_al_array_bfind(
    _In_reads_bytes_(bytesof_elem) PC_ANY key,
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _In_        P_PROC_BSEARCH p_proc_bsearch,
    _OutRef_    P_BOOL p_hit
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem));

#define al_array_bfind(key, p_array_handle, __base_type, p_proc_bsearch, p_hit) ( \
    _al_array_bfind(key, p_array_handle, p_proc_bsearch, p_hit \
    CODE_ANALYSIS_ONLY_ARG(sizeof32(__base_type))) )

_Check_return_
_Ret_writes_maybenone_(bytesof_elem)
extern P_BYTE
_al_array_bsearch(
    _In_        PC_ANY key,
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _In_        P_PROC_BSEARCH p_proc_bsearch
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem));

#define al_array_bsearch(key, p_array_handle, __base_type, p_proc_bsearch) ( \
    (__base_type *) _al_array_bsearch(key, p_array_handle, p_proc_bsearch \
    CODE_ANALYSIS_ONLY_ARG(sizeof32(__base_type))) )

_Check_return_
_Ret_writes_maybenone_(bytesof_elem)
extern P_BYTE
_al_array_lsearch(
    _In_        PC_ANY key,
    _InRef_     P_ARRAY_HANDLE p_array_handle,
    _In_        P_PROC_BSEARCH p_proc_bsearch
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem));

#define al_array_lsearch(key, p_array_handle, __base_type, p_proc_bsearch) ( \
    (__base_type *) _al_array_lsearch(key, p_array_handle, p_proc_bsearch \
    CODE_ANALYSIS_ONLY_ARG(sizeof32(__base_type))) )

extern void
_al_array_dispose(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle);

static inline void
al_array_dispose(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle)
{
    if(0 != *p_array_handle)
        _al_array_dispose(p_array_handle);
}

_Check_return_
extern STATUS
al_array_duplicate(
    _OutRef_    P_ARRAY_HANDLE p_dup_array_handle,
    _InRef_     PC_ARRAY_HANDLE pc_src_array_handle);

extern void
al_array_empty(
    _InRef_     PC_ARRAY_HANDLE p_array_handle);

/*ncr*/
extern S32 /* number of elements remaining */
al_array_garbage_collect(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InVal_     ARRAY_INDEX element_start,
    _In_opt_    P_PROC_ELEMENT_IS_DELETED p_proc_element_is_deleted,
    _InVal_     AL_GARBAGE_FLAGS al_garbage_flags);

_Check_return_
extern S32 /* number of allocated handles */
al_array_handle_check(
    _InVal_     S32 whinge /* whinge about allocated handles */);

extern void
al_array_delete_at(
    _InRef_     P_ARRAY_HANDLE p_array_handle,
    _InVal_     S32 num_elements, /* NB num_elements still -ve */
    _InVal_     ARRAY_INDEX delete_at);

_Check_return_
_Ret_writes_maybenull_(bytesof_elem_x_num_elem)
extern P_BYTE
_al_array_insert_before(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InVal_     S32 num_elements,
    _InRef_maybenone_ PC_ARRAY_INIT_BLOCK p_array_init_block,
    _OutRef_    P_STATUS p_status,
    _InVal_     ARRAY_INDEX insert_before
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem_x_num_elem));

#define al_array_insert_before(p_array_handle, __base_type, num_elements, p_array_init_block, p_status, insert_before) ( \
    (__base_type *) _al_array_insert_before(p_array_handle, num_elements, p_array_init_block, p_status, insert_before \
    CODE_ANALYSIS_ONLY_ARG((num_elements) * sizeof32(__base_type))) )

#define AL_ARRAY_INSERT_BEFORE_PROTO(_e_s, __base_type) \
_Check_return_ \
_Ret_writes_maybenull_(num_elements) /* may be NULL */ \
_e_s __base_type * /* pointer to new allocation if ok, NULL if failed */ \
al_array_insert_before_ ## __base_type( \
    _InoutRef_  P_ARRAY_HANDLE p_array_handle, \
    _InVal_     S32 num_elements, \
    _InRef_maybenone_ PC_ARRAY_INIT_BLOCK p_array_init_block, \
    _OutRef_    P_STATUS p_status, \
    _InVal_     ARRAY_INDEX insert_before)

#define AL_ARRAY_INSERT_BEFORE_IMPL(_e_s, __base_type) \
__pragma(warning(push)) /*__pragma(warning(disable:6386))*/ /* mask the 'return value' Buffer overrun warning */ \
_Check_return_ \
_Ret_writes_maybenull_(num_elements) /* may be NULL */ \
_e_s __base_type * /* pointer to new allocation if ok, NULL if failed */ \
al_array_insert_before_ ## __base_type( \
    _InoutRef_  P_ARRAY_HANDLE p_array_handle, \
    _InVal_     S32 num_elements, \
    _InRef_maybenone_ PC_ARRAY_INIT_BLOCK p_array_init_block, \
    _OutRef_    P_STATUS p_status, \
    _InVal_     ARRAY_INDEX insert_before) \
{ \
    return( (__base_type *) \
        _al_array_insert_before(p_array_handle, num_elements, p_array_init_block, p_status, insert_before \
        CODE_ANALYSIS_ONLY_ARG((U32) (num_elements) * sizeof32(__base_type)) \
        )); \
} \
__pragma(warning(pop))

extern void
al_array_qsort(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _In_        P_PROC_QSORT p_proc_compare_qsort);

extern void /* declared as al_array_qsort replacement */
al_array_check_sorted(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _In_        P_PROC_QSORT p_proc_compare_qsort);

_Check_return_
_Ret_writes_maybenull_(bytesof_elem_x_num_elem)
extern P_BYTE
_al_array_extend_by(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InVal_     U32 add_elements,
    _InRef_maybenone_ PC_ARRAY_INIT_BLOCK p_array_init_block,
    _OutRef_    P_STATUS p_status
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem_x_num_elem));

#define al_array_extend_by(p_array_handle, __base_type, add_elements, p_array_init_block, p_status) ( \
    (__base_type *) _al_array_extend_by(p_array_handle, add_elements, p_array_init_block, p_status \
    CODE_ANALYSIS_ONLY_ARG((add_elements) * sizeof32(__base_type))) )

#define AL_ARRAY_EXTEND_BY_PROTO(_e_s, __base_type) \
_Check_return_ \
_Ret_writes_maybenull_(add_elements) /* may be NULL */ \
_e_s __base_type * /* pointer to new allocation if ok, NULL if failed */ \
al_array_extend_by_ ## __base_type( \
    _InoutRef_  P_ARRAY_HANDLE p_array_handle, \
    _InVal_     U32 add_elements, \
    _InRef_maybenone_ PC_ARRAY_INIT_BLOCK p_array_init_block, \
    _OutRef_    P_STATUS p_status)

#define AL_ARRAY_EXTEND_BY_IMPL(_e_s, __base_type) \
__pragma(warning(push)) /*__pragma(warning(disable:6386))*/ /* mask the 'return value' Buffer overrun warning */ \
_Check_return_ \
_Ret_writes_maybenull_(add_elements) /* may be NULL */ \
_e_s __base_type * /* pointer to new allocation if ok, NULL if failed */ \
al_array_extend_by_ ## __base_type( \
    _InoutRef_  P_ARRAY_HANDLE p_array_handle, \
    _InVal_     U32 add_elements, \
    _InRef_maybenone_ PC_ARRAY_INIT_BLOCK p_array_init_block, \
    _OutRef_    P_STATUS p_status) \
{ \
    return( (__base_type *) \
        _al_array_extend_by(p_array_handle, add_elements, p_array_init_block, p_status \
        CODE_ANALYSIS_ONLY_ARG((add_elements) * sizeof32(__base_type)) \
        )); \
} \
__pragma(warning(pop))

extern void
al_array_shrink_by(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InVal_     S32 num_elements); /* NB num_elements still -ve */

#if WINDOWS

extern void
al_array_resized_hglobal(
    _InRef_     P_ARRAY_HANDLE p_array_handle,
    _In_        HGLOBAL hglobal,
    _InVal_     U32 size);

extern void
al_array_resized_ptr(
    _InRef_     P_ARRAY_HANDLE p_array_handle,
    _In_        P_ANY p_any,
    _InVal_     U32 size);

_Check_return_
_Ret_maybenull_
extern HGLOBAL
al_array_steal_hglobal(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle);

#endif

extern void
al_array_trim(
    _InRef_     PC_ARRAY_HANDLE p_array_handle);

#if WINDOWS

_Check_return_
_Ret_writes_to_maybenull_(dwBytes, 0) /* may be NULL */
extern P_BYTE
GlobalAllocAndLock(
    _InVal_     UINT uFlags,
    _InVal_     U32 dwBytes,
    _Out_       HGLOBAL * const p_hglobal);

#endif /* OS */

#if TRACE_ALLOWED

extern void
al_list_big_handles(
    _InVal_     U32 over_n_bytes);

#endif

/*
templated exported data & routines
*/

extern const ARRAY_INIT_BLOCK
array_init_block_u8;

AL_ARRAY_ALLOC_PROTO(extern, U8);
AL_ARRAY_EXTEND_BY_PROTO(extern, U8);
AL_ARRAY_INSERT_BEFORE_PROTO(extern, U8);

AL_ARRAY_ALLOC_PROTO(extern, BYTE);
AL_ARRAY_EXTEND_BY_PROTO(extern, BYTE);

#define array_init_block_byte array_init_block_u8

#define array_init_block_uchars array_init_block_u8

extern const ARRAY_INIT_BLOCK
array_init_block_tchar;

AL_ARRAY_ALLOC_PROTO(extern, TCHAR);
AL_ARRAY_EXTEND_BY_PROTO(extern, TCHAR);

extern const ARRAY_INIT_BLOCK
array_init_block_wchar;

AL_ARRAY_ALLOC_PROTO(extern, WCHAR);
AL_ARRAY_EXTEND_BY_PROTO(extern, WCHAR);

AL_ARRAY_ALLOC_PROTO(extern, ARRAY_HANDLE);
AL_ARRAY_EXTEND_BY_PROTO(extern, ARRAY_HANDLE);

/*
simply-typed variants of ARRAY_HANDLE
*/

#define    ARRAY_HANDLE_SBSTR     ARRAY_HANDLE
#define  P_ARRAY_HANDLE_SBSTR   P_ARRAY_HANDLE

#define    ARRAY_HANDLE_UCHARS    ARRAY_HANDLE
#define  P_ARRAY_HANDLE_UCHARS  P_ARRAY_HANDLE
#define PC_ARRAY_HANDLE_UCHARS PC_ARRAY_HANDLE

#define uchars_from_h_uchars(pc_array_handle_uchars) \
    array_basec(pc_array_handle_uchars, _UCHARS)

#define    ARRAY_HANDLE_USTR     ARRAY_HANDLE_UCHARS
#define  P_ARRAY_HANDLE_USTR   P_ARRAY_HANDLE_UCHARS
#define PC_ARRAY_HANDLE_USTR  PC_ARRAY_HANDLE_UCHARS

#define ustr_from_h_ustr(pc_array_handle_ustr) \
    array_basec(pc_array_handle_ustr, _UCHARZ)

#define array_ustr(pc_array_handle_ustr) \
    ustr_from_h_ustr(pc_array_handle_ustr)

#define    ARRAY_HANDLE_TSTR     ARRAY_HANDLE
#define  P_ARRAY_HANDLE_TSTR   P_ARRAY_HANDLE
#define PC_ARRAY_HANDLE_TSTR  PC_ARRAY_HANDLE

#define tstr_from_h_tstr(pc_array_handle_tstr) \
    array_basec(pc_array_handle_tstr, TCHARZ)

#define array_tstr(pc_array_handle_tstr) \
    tstr_from_h_tstr(pc_array_handle_tstr)

#define    ARRAY_HANDLE_WSTR     ARRAY_HANDLE
#define  P_ARRAY_HANDLE_WSTR   P_ARRAY_HANDLE
#define PC_ARRAY_HANDLE_WSTR  PC_ARRAY_HANDLE

#define wstr_from_h_wstr(pc_array_handle_wstr) \
    array_basec(pc_array_handle_wstr, WCHARZ)

#define array_wstr(pc_array_handle_wstr) \
    wstr_from_h_wstr(pc_array_handle_wstr)

#endif /* __aligator_h */

/* end of aligator.h */
