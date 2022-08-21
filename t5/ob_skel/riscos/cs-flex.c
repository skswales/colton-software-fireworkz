/* cs-flex.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
internal definitions
*/

#define EXPOSE_RISCOS_FLEX 1
#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"

/* the well-known implementation of flex has allocated block preceded by: */

typedef struct FLEX_BLOCK
{
    flex_ptr    anchor;
    int         size;           /* exact size of allocation in bytes */
}
FLEX_BLOCK, * P_FLEX_BLOCK;

#if defined(REPORT_FLEX)
#define flex_reportf(arglist) reportf arglist
#else
#define flex_reportf(arglist) /* reportf arglist */
#endif

/* like flex_free(), but caters for already freed / not yet allocated */

extern void
flex_dispose(
    flex_ptr anchor)
{
    flex_reportf(("flex_dispose(%p)", report_ptr_cast(anchor)));
    flex_reportf(("flex_dispose(%p->%p)", report_ptr_cast(anchor), *anchor));

    if(NULL == *anchor)
        return;

    flex_free(anchor);
}

/* give flex memory to another owner */

extern void
flex_give_away(
    flex_ptr new_anchor,
    flex_ptr old_anchor)
{
    P_FLEX_BLOCK p;

    trace_4(TRACE_MODULE_ALLOC, TEXT("flex_give_away(") PTR_XTFMT TEXT(":=") PTR_XTFMT TEXT("->") PTR_XTFMT TEXT(" (size %d))"), new_anchor, old_anchor, *old_anchor, flex_size(old_anchor));
    flex_reportf((TEXT("flex_give_away(") PTR_XTFMT TEXT(":=") PTR_XTFMT TEXT("->") PTR_XTFMT TEXT(" (size %d))"), report_ptr_cast(new_anchor), report_ptr_cast(old_anchor), *old_anchor, flex_size_maybe_null(old_anchor)));

    p = (P_FLEX_BLOCK) *old_anchor;

    *old_anchor = NULL;
    *new_anchor = p;

    if(NULL == p--)
        return;

    p->anchor = new_anchor;
}

_Check_return_
extern BOOL
flex_realloc(
    flex_ptr anchor,
    int newsize)
{
    trace_3(TRACE_MODULE_ALLOC, TEXT("flex_realloc(") PTR_XTFMT TEXT(" -> ") PTR_XTFMT TEXT(", %d)"), anchor, *anchor, newsize);
    flex_reportf(("flex_realloc(%p)", report_ptr_cast(anchor)));
    flex_reportf(("flex_realloc(%p->%p, size %d)", report_ptr_cast(anchor), *anchor, newsize));

    if(NULL != *anchor)
    {
        if(0 == newsize)
        {
            flex_dispose(anchor);
            return(FALSE);
        }

        return(flex_extend(anchor, newsize));
    }

    return(flex_alloc(anchor, newsize));
}

/* like flex_size(), but caters for already freed / not yet allocated */

_Check_return_
extern int
flex_size_maybe_null(
    flex_ptr anchor)
{
    P_FLEX_BLOCK p;

    flex_reportf(("flex_size_maybe_null(%p)", report_ptr_cast(anchor)));

    p = (P_FLEX_BLOCK) *anchor;

    if(NULL == p--)
    {
        flex_reportf(("flex_size_maybe_null(%p->NULL): size 0 returned", report_ptr_cast(anchor)));
        return(0);
    }

    flex_reportf(("flex_size_maybe_null(%p->%p): size %d", report_ptr_cast(anchor), *anchor, p->size));
    return(p->size);
}

#if defined(REPORT_FLEX) || 1 /* easiest if we keep them so we don't have to recompile DataPower's WimpLib */

#undef flex_alloc

_Check_return_
extern BOOL
report_flex_alloc(
    flex_ptr anchor,
    int size)
{
#if defined(REPORT_FLEX)
    flex_reportf(("report_flex_alloc(%p)", report_ptr_cast(anchor)));

    /*if(0 == size)*/
        /*flex_reportf(("report_flex_alloc(%p->%p, size %d): size 0 will stuff up", report_ptr_cast(anchor), *anchor, size));*/

    if(*anchor)
        flex_reportf(("report_flex_alloc(%p->%p, size %d): anchor not NULL - will discard data without freeing", report_ptr_cast(anchor), *anchor, size));
    else
        flex_reportf(("report_flex_alloc(%p->NULL, size %d)", report_ptr_cast(anchor), size));
#endif /* REPORT_FLEX */

    return(flex_alloc(anchor, size));
}

#undef flex_extend

_Check_return_
extern BOOL
report_flex_extend(
    flex_ptr anchor,
    int newsize)
{
#if defined(REPORT_FLEX)
    flex_reportf(("report_flex_extend(%p)", report_ptr_cast(anchor)));

    if(0 == newsize)
        flex_reportf(("report_flex_extend(%p->%p, newsize %d): size 0 will stuff up", report_ptr_cast(anchor), *anchor, newsize));

    if(*anchor)
        flex_reportf(("report_flex_extend(%p->%p, newsize %d) (cursize %d)", report_ptr_cast(anchor), *anchor, newsize, (flex_size)(anchor)));
    else
        flex_reportf(("report_flex_extend(%p->NULL, newsize %d): anchor NULL - will fail", report_ptr_cast(anchor), newsize));
#endif /* REPORT_FLEX */

    return(flex_extend(anchor, newsize));
}

#undef flex_free

extern void
report_flex_free(
    flex_ptr anchor)
{
#if defined(REPORT_FLEX)
   flex_reportf(("report_flex_free(%p)", report_ptr_cast(anchor)));

    if(*anchor)
        flex_reportf(("report_flex_free(%p->%p) (size %d)", report_ptr_cast(anchor), *anchor, (flex_size)(anchor)));
    else
        flex_reportf(("report_flex_free(%p->NULL): anchor NULL - will fail", report_ptr_cast(anchor)));
#endif /* REPORT_FLEX */

    (flex_free)(anchor);
}

#undef flex_size

_Check_return_
extern int
report_flex_size(
    flex_ptr anchor)
{
    int size = 0;

    flex_reportf(("report_flex_size(%p)", report_ptr_cast(anchor)));

    if(*anchor)
    {
        size = (flex_size)(anchor);
        flex_reportf(("report_flex_size(%p->%p): size %d", report_ptr_cast(anchor), *anchor, size));
    }
    else
    {
        flex_reportf(("report_flex_size(%p->NULL): anchor NULL - will fail", report_ptr_cast(anchor)));
        size = (flex_size)(anchor);
    }

    return(size);
}

#endif /* REPORT_FLEX */

int flex_granularity = 0x8000;     /* must be a power-of-two size or zero (exported) */

static inline int
flex_granularity_ceil(int n)
{
  if(flex_granularity)
  {
    int mask = flex_granularity - 1; /* flex_granularity must be a power-of-two */
    n = (n + mask) & ~mask;
  }
  return n;
}

static inline int
flex_granularity_floor(int n)
{
  if(flex_granularity)
  {
    int mask = flex_granularity - 1; /* flex_granularity must ve a power-of-two */
    n = n & ~mask;
  }
  return n;
}

#if defined(TBOXLIBS_FLEX)

/* can get better code for loading structure members on ARM Norcroft */

static struct flex_
{
    char *          start;          /* start of flex memory */
    char *          freep;          /* free flex memory */
    char *          limit;          /* limit of flex memory */

    int             area_num;       /* dynamic area handle */

    BOOL            shrink_forbidden;
} flex_;

/*ncr*/
extern BOOL
flex_forbid_shrink(
    _InVal_     BOOL forbid)
{
    BOOL res = flex_.shrink_forbidden;

#if defined(REPORT_FLEX)
    flex_reportf(("flex_forbid_shrink(%s)", report_boolstring(forbid)));
#endif

    flex_.shrink_forbidden = forbid;

    return(res);
}

extern int
flex_set_budge(int newstate)
{
    if(flex_.area_num)
        return(-1);

    UNREFERENCED_PARAMETER_InVal_(newstate);
    assert(0 == newstate);
    return(0);
}

/******************************************************************************
*
* how much store do we have unused at the end of the flex area?
*
******************************************************************************/

_Check_return_
extern int
flex_storefree(void)
{
#if defined(REPORT_FLEX) && 0
    flex_reportf(("flex_storefree(): flex_.limit = %p, flex_.freep = %p, => free = %d",
                 report_ptr_cast(flex_.limit), report_ptr_cast(flex_.freep), flex_.limit - flex_.freep));
#endif
    return(flex_.limit - flex_.freep);
}

#define flex__base     flex_.start
#define flex__start    flex_.start
#define flex__freep    flex_.freep
#define flex__lim      flex_.limit

#define flex__area_num flex_.area_num

#define flex__check() /*EMPTY*/

static void
flex__fail(int i);

static const char * /* only called here */
flex_msgs_lookup(
    const char * tag_and_default)
{
    return(string_for_object(tag_and_default, OBJECT_ID_SKEL));
}

#define SKS_ACW 1 /* make it easy to share with PipeDream's modifications */

#undef TRACE /* don't mess with devices:parallel ! */

#define DefaultSize 0 /* for the Dynamic Area */

#include "tb-flex.c"

static void
flex__fail(int i)
{
   UNREFERENCED_PARAMETER_InVal_(i);
   flex_werr(FALSE, flex_msgs_lookup(MSGS_flex1)); /* don't abort */
}

#else

/* can get better code for loading structure members on ARM Norcroft */

static struct flex_
{
    char *      start;          /* start of flex memory */
    char *      freep;          /* free flex memory */
    char *      limit;          /* limit of flex memory */

    int         area_num;       /* dynamic area handle */
}
flex_;

/* macros for improved code clarity */

#define flex_roundup(i) ( \
    (i + 3) & ~3 )

#define flex_innards(p) ( \
    (void *) (p + 1) )

#define flex_next_block(p) ( \
    (flex__rec *) (void *) ((char *) flex_innards(p) + flex_roundup(p->size)) )

/*
nd: 08-07-1996
*/

/* Creates a dynamic area and returns the handle of it */

static os_error *
dynamicarea_create(
    _Out_       int * hand,
    _InVal_     int dynamic_size,
    _In_z_      const char * name)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = 0;
    rs.r[1] = -1;
    rs.r[2] = 0;
    rs.r[3] = -1;
    rs.r[4] = (1 << 7);
    rs.r[5] = dynamic_size;
    rs.r[6] = 0;
    rs.r[7] = 0;
    rs.r[8] = (int) name;

    if(NULL == (e = _kernel_swi(OS_DynamicArea, &rs, &rs)))
        *hand = rs.r[1];
    else
        *hand = 0;

    return(e);
}

/* Kills off an existing dynamic area */

static os_error *
dynamicarea_kill(
    _In_        int hand)
{
    _kernel_swi_regs rs;

    rs.r[0] = 1;
    rs.r[1] = hand;

    return(_kernel_swi(OS_DynamicArea, &rs, &rs));
}

/* Reads info about a dynamic area */

static os_error *
dynamicarea_read(
    _In_        int hand,
    _Out_       int * p_size,
    _Out_       int * p_base)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = 2;
    rs.r[1] = hand;

    if(NULL != (e = _kernel_swi(OS_DynamicArea, &rs, &rs)))
    {
        rs.r[2] = 0;
        rs.r[3] = 0;
    }

    *p_size = rs.r[2];
    *p_base = rs.r[3];

    return(e);
}

/* Changes the size of a dynamic area */

static os_error *
dynamicarea_change(
    _In_        int hand,
    _In_        int size)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = hand;

    if(NULL != (e = _kernel_swi(OS_ReadDynamicArea, &rs, &rs)))
        return(e);

    rs.r[1] = size - rs.r[1];
    rs.r[0] = hand;

    return(_kernel_swi(OS_ChangeDynamicArea, &rs, &rs));
}

/* Reads the size of the current task's slot */

static _kernel_oserror *
wimp_currentslot_read(
    _Out_       int * p_size)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = -1;
    rs.r[1] = -1;

    if(NULL != (e = _kernel_swi(Wimp_SlotSize, &rs, &rs)))
        rs.r[0] = 0;

    *p_size = rs.r[0];

    return(e);
}

/* Changes the size of the current task's slot */

static _kernel_oserror *
wimp_currentslot_change(
    _Inout_     int * p_size)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = *p_size;
    rs.r[1] = -1;

    if(NULL != (e = _kernel_swi(Wimp_SlotSize, &rs, &rs)))
        return(e);

    *p_size = rs.r[0];

    return(NULL);
}

/* Write the top of available memory */

static _kernel_oserror *
flex__area_change(
    _Inout_     int * p_top)
{
    _kernel_oserror * e;
    int top = *p_top;
    int base = 0x8000;
    int size;

    if(flex_.area_num)
    {
        base = (int) flex_.start;
        size = top - base;
        e = dynamicarea_change(flex_.area_num, size);
    }
    else
    {
        size = top - base;
        e = wimp_currentslot_change(&size);
    }

    if(NULL == e)
        *p_top = base + size;

    return(e);
}

/* Read the top of available memory */

static void inline
flex__area_read(
    _Out_       int * p_top)
{
    _kernel_oserror * e;
    int base = 0x8000;
    int size = 0;
    
    if(flex_.area_num)
        e = dynamicarea_read(flex_.area_num, &size, &base);
    else
        e = wimp_currentslot_read(&size);

    if(NULL != e)
    {
        base = 0;
        size = 0;
    }

    *p_top = base + size;
}

static void
flex_kill(void)
{
    if(flex_.area_num)
    {
        dynamicarea_kill(flex_.area_num);
        flex_.area_num = 0;
    }
}

static void
flex_atexit(void)
{
    flex_kill();
}

extern int
flex_init(
    char *program_name,
    int *error_fd,
    int dynamic_size)
{
    UNREFERENCED_PARAMETER(error_fd);

    {
    _kernel_swi_regs rs;
    void_WrapOsErrorChecking(_kernel_swi(OS_ReadMemMapInfo, &rs, &rs));
    flex_granularity = rs.r[0]; /* page size */
    } /*block*/

    if(flex_granularity < 0x4000)
        /* SKS says don't page violently on RISC PC (allow 2MB A3000 etc. to get away with 16KB pages though) */
        flex_granularity = 0x8000;

#if defined(SHAKE_HEAP_VIOLENTLY)
    flex_granularity = 0x0080;
#endif
    trace_1(TRACE_MODULE_ALLOC, TEXT("flex_init: flex_granularity = %d"), flex_granularity);

    if((dynamic_size != 0) && (NULL == dynamicarea_create(&flex_.area_num, dynamic_size, program_name))) /* call should fail if OS_DynamicArea not supported */
    {
        atexit(flex_atexit);
    }

    { /* Read current top of memory (either Window Manager current slot or Dynamic area just created) */
    int top;
    flex__area_read(&top);
    flex_.start = flex_.freep = flex_.limit = (char *) top;
    trace_1(TRACE_MODULE_ALLOC, TEXT("flex_.limit = ") PTR_XTFMT, flex_.limit);
    } /*block*/

    {
    void * a;
    if(flex_alloc(&a, 1))
    {
        flex_free(&a);
        return(flex_.area_num);
    }
    } /*block*/

    flex_kill();

    return(-1);
}

extern int
flex_set_budge(int newstate)
{
    if(flex_.area_num)
        return(-1);

    UNREFERENCED_PARAMETER_InVal_(newstate);
    assert(0 == newstate);
    return(0);
}

/******************************************************************************
*
* how much store do we have unused at the end of the flex area?
*
******************************************************************************/

_Check_return_
extern int
flex_storefree(void)
{
    return(flex_.limit - flex_.freep);
}

#include "flex.c"

#endif /* TBOXLIBS_FLEX */

#endif /* RISCOS */

/* end of cs-flex.c */
