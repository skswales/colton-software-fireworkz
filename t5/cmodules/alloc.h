/* alloc.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Allocation in an extensible flex block */

/* SKS 23-Aug-1989 */

#ifndef __alloc_h
#define __alloc_h

/*
types
*/

typedef struct ALLOC_FUNCTION_SET
{
    P_ANY (* calloc_fn)   (U32, U32);
    void  (* free_fn)     (_Pre_maybenull_ _Post_invalid_ P_ANY);
    P_ANY (* malloc_fn)   (U32);
    P_ANY (* realloc_fn)  (_Pre_maybenull_ _Post_invalid_ P_ANY, U32);
    U32   (* size_fn)     (P_ANY);
    void  (* validate_fn) (P_ANY, _In_z_ PCTSTR);
}
ALLOC_FUNCTION_SET;

/* *** NB. these functions may move flex blocks when used *** */

/*
exported functions
*/

/*
main alloc function set - export via vector (see above defs)
*/

extern ALLOC_FUNCTION_SET alloc_main;

#define alloc_calloc   alloc_main.calloc_fn
#define alloc_free     alloc_main.free_fn
#define alloc_malloc   alloc_main.malloc_fn
#define alloc_realloc  alloc_main.realloc_fn
#define alloc_size     alloc_main.size_fn
#define alloc_validate alloc_main.validate_fn

extern U32 alloc_main_heap_minsize;

extern U32 g_dynamic_area_limit;

_Check_return_
extern STATUS
alloc_ensure_froth(
    _In_        U32 froth_size);

_Check_return_
extern STATUS
alloc_init(void);

extern void
alloc_tidy_up(void);

extern void
alloc_track_stop(void);

_Check_return_
extern int
alloc_dynamic_area_query(void);

#if TRACE_ALLOWED

/*
exported functions for TRACE_ALLOWED
*/

extern void
alloc_traversefree(
    _In_        int which);

#endif /* TRACE_ALLOWED */

#ifdef EXPORT_FIXED_ALLOCS
/* functions that use the old allocation system
 * and therefore do not move memory when used
*/

/*
fixed alloc function set - export via vector (see above defs)
*/

extern ALLOC_FUNCTION_SET alloc_fixed;

#define fixed_calloc  alloc_fixed.calloc_fn
#define fixed_free    alloc_fixed.free_fn
#define fixed_malloc  alloc_fixed.malloc_fn
#define fixed_realloc alloc_fixed.realloc_fn
#define fixed_size    alloc_fixed.size_fn

extern U32 alloc_fixed_heap_size;

#endif /* EXPORT_FIXED_ALLOCS */

#ifndef REALLY_DONT_REDIRECT_ALLOCS

/*
fixed alloc function set - export via vector (see above defs)
*/

extern ALLOC_FUNCTION_SET alloc_barf;

#define calloc     alloc_barf.calloc_fn
#define free(a)    alloc_barf.free_fn(a)
#define malloc     alloc_barf.malloc_fn
#define realloc    alloc_barf.realloc_fn

#endif /* REALLY_DONT_REDIRECT_ALLOCS */

#endif /* __alloc_h */

/* end of alloc.h */
