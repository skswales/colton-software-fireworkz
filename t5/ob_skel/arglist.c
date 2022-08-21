/* arglist.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS December 1991 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

typedef struct ARGLIST_CACHE_ENTRY
{
    S32 free_idx; /* -2 if alloced, -1 at end of chain */
    ARGLIST_HANDLE arglist_handle;
}
ARGLIST_CACHE_ENTRY, * P_ARGLIST_CACHE_ENTRY;

static
struct ARGLIST_CACHE
{
    ARRAY_HANDLE handle;
    S32 entries_in_use;
    S32 free_idx; /* -1 at end of chain */
}
arglist_cache =
{
    0,
    0,
    -1
};

/******************************************************************************
*
* allocate a string arg in an arglist
*
******************************************************************************/

_Check_return_
extern STATUS
arg_alloc_ustr(
    _InRef_     PC_ARGLIST_HANDLE p_h_args /*data modified*/,
    _InVal_     U32 arg_idx,
    _In_opt_z_  PC_USTR ustr_inline) /* poss inline */
{
    STATUS status;
    P_ARGLIST_ARG p_arg;
    U32 n_bytes;

    if(NULL == ustr_inline)
        ustr_inline = ustr_empty_string;

    n_bytes = ustr_inline_strlen32p1((PC_USTR_INLINE) ustr_inline); /*CH_NULL*/

#if CHECKING
    assert(!contains_inline(ustr_inline, n_bytes - 1));

    if(!array_index_is_valid(p_h_args, arg_idx))
    {
        myassert4(TEXT("arg_alloc_ustr: arg_idx ") S32_TFMT TEXT(" < n_arglist_args(p_h_args ") PTR_XTFMT TEXT("->") S32_TFMT TEXT(") ") S32_TFMT, (S32) arg_idx, p_h_args, *p_h_args, n_arglist_args(p_h_args));
        return(status_nomem());
    }
#endif

    p_arg = p_arglist_arg(p_h_args, arg_idx);

    assert(ARG_TYPE_USTR == (p_arg->type & ARG_TYPE_MASK));
    assert(0 == (p_arg->type & ARG_ALLOC));

    if(NULL != (p_arg->val.ustr_wr = al_ptr_alloc_bytes(P_USTR, n_bytes, &status))) /* NB can't ustr_set() due to inlines */
    {
        memcpy32(p_arg->val.ustr_wr, ustr_inline, n_bytes);

        p_arg->type = (ARG_TYPE) (ARG_TYPE_USTR | ARG_ALLOC);
    }

    return(status);
}

_Check_return_
extern STATUS
arg_alloc_tstr(
    _InRef_     PC_ARGLIST_HANDLE p_h_args /*data modified*/,
    _InVal_     U32 arg_idx,
    _In_opt_z_  PCTSTR tstr) /* can't be inline */
{
    P_ARGLIST_ARG p_arg;

    if(NULL == tstr)
        tstr = tstr_empty_string;

#if CHECKING
    if(!array_offset_is_valid(p_h_args, arg_idx))
    {
        myassert4(TEXT("arg_alloc_tstr: arg_idx ") S32_TFMT TEXT(" < n_arglist_args(p_h_args ") PTR_XTFMT TEXT("->") S32_TFMT TEXT(") ") S32_TFMT, (S32) arg_idx, p_h_args, *p_h_args, n_arglist_args(p_h_args));
        return(status_check());
    }
#endif

    p_arg = p_arglist_arg(p_h_args, arg_idx);

    assert(ARG_TYPE_TSTR == (p_arg->type & ARG_TYPE_MASK));
    assert(0 == (p_arg->type & ARG_ALLOC));

    status_return(tstr_set(&p_arg->val.tstr_wr, tstr));

    p_arg->type = (ARG_TYPE) (ARG_TYPE_TSTR | ARG_ALLOC);

    return(STATUS_OK);
}

/******************************************************************************
*
* dispose of an arg in an arglist
*
******************************************************************************/

extern void
arg_dispose(
    _InRef_     PC_ARGLIST_HANDLE p_h_args /*data modified*/,
    _InVal_     U32 arg_idx)
{
    P_ARGLIST_ARG p_arg;

    if(0 == array_elements(p_h_args))
        return;

#if CHECKING
    if(!array_offset_is_valid(p_h_args, arg_idx))
    {
        myassert4(TEXT("arg_dispose: arg_idx ") S32_TFMT TEXT(" < n_arglist_args(p_h_args ") PTR_XTFMT TEXT("->") S32_TFMT TEXT(") ") S32_TFMT, (S32) arg_idx, p_h_args, *p_h_args, n_arglist_args(p_h_args));
        return;
    }
#endif

    p_arg = p_arglist_arg(p_h_args, arg_idx);

    if(p_arg->type & ARG_ALLOC)
    {
        p_arg->type &= ~ARG_ALLOC;

        switch(p_arg->type & ARG_TYPE_MASK)
        {
        default:
            break;

        case ARG_TYPE_RAW:
        case ARG_TYPE_RAW_DS:
            al_array_dispose(&p_arg->val.raw);
            break;

        case ARG_TYPE_USTR:
            ustr_clr(&p_arg->val.ustr_wr);
            break;

#if defined(ARG_TYPE_TSTR_DISTINCT)
        case ARG_TYPE_TSTR:
            tstr_clr(&p_arg->val.tstr_wr);
            break;
#endif /* ARG_TYPE_TSTR_DISTINCT */
        }
    }

    p_arg->type = ARG_TYPE_NONE;
}

/******************************************************************************
*
* dispose of an non-fixed arg's value in an arglist
*
******************************************************************************/

extern void
arg_dispose_val(
    _InRef_     PC_ARGLIST_HANDLE p_h_args /*data modified*/,
    _InVal_     U32 arg_idx)
{
    P_ARGLIST_ARG p_arg;

    if(0 == array_elements(p_h_args))
        return;

#if CHECKING
    if(!array_offset_is_valid(p_h_args, arg_idx))
    {
        myassert4(TEXT("arg_dispose_val: arg_idx ") S32_TFMT TEXT(" < n_arglist_args(p_h_args ") PTR_XTFMT TEXT("->") S32_TFMT TEXT(") ") S32_TFMT, (S32) arg_idx, p_h_args, *p_h_args, n_arglist_args(p_h_args));
        return;
    }
#endif

    p_arg = p_arglist_arg(p_h_args, arg_idx);

    if(p_arg->type & ARG_ALLOC)
    {
        p_arg->type &= ~ARG_ALLOC;

        switch(p_arg->type & ARG_TYPE_MASK)
        {
        default:
            break;

        case ARG_TYPE_RAW:
        case ARG_TYPE_RAW_DS:
            al_array_dispose(&p_arg->val.raw);
            break;

        case ARG_TYPE_USTR:
            ustr_clr(&p_arg->val.ustr_wr);
            break;

#if defined(ARG_TYPE_TSTR_DISTINCT)
        case ARG_TYPE_TSTR:
            tstr_clr(&p_arg->val.tstr_wr);
            break;
#endif /* ARG_TYPE_TSTR_DISTINCT */
        }
    }
}

/* trash the cache? */

extern void
arglist_cache_reduce(void)
{
    if(!arglist_cache.entries_in_use)
    {
        ARRAY_INDEX i;

        trace_0(TRACE_APP_ARGLIST, TEXT("arglist_cache TRASH"));

        assert(!arglist_cache.entries_in_use);

        arglist_cache.free_idx = -1; /* prepare for reuse */

        for(i = 0; i < array_elements(&arglist_cache.handle); ++i)
        {
            P_ARGLIST_CACHE_ENTRY p_arglist_cache_entry = array_ptr(&arglist_cache.handle, ARGLIST_CACHE_ENTRY, i);

            al_array_dispose((P_ARRAY_HANDLE) &p_arglist_cache_entry->arglist_handle);
        }

        al_array_dispose(&arglist_cache.handle);
    }
}

/******************************************************************************
*
* dispose of all args in an arglist and the arglist itself
*
******************************************************************************/

extern void
arglist_dispose(
    _InoutRef_  P_ARGLIST_HANDLE p_h_args)
{
    if(NULL != p_h_args)
    {
#if 1
        ARRAY_INDEX i;
        ARGLIST_HANDLE arglist_handle = *p_h_args;

        trace_1(TRACE_APP_ARGLIST, TEXT("arglist_dispose(") S32_TFMT TEXT(")"), arglist_handle);

        arglist_dispose_after(p_h_args, U32_MAX /*all args*/);

        for(i = 0; i < array_elements(&arglist_cache.handle); ++i)
        {
            P_ARGLIST_CACHE_ENTRY p_arglist_cache_entry = array_ptr(&arglist_cache.handle, ARGLIST_CACHE_ENTRY, i);

            if(arglist_handle == p_arglist_cache_entry->arglist_handle)
            {
                assert(p_arglist_cache_entry->free_idx == -2 /* alloced */);

                /* add at head of free chain */
                p_arglist_cache_entry->free_idx = arglist_cache.free_idx;
                arglist_cache.free_idx = i;

                --arglist_cache.entries_in_use;

#if 0
                arglist_cache_reduce();
#endif
            }
        }

        *p_h_args = 0;

#else
        arglist_dispose_after(p_h_args, U32_MAX /*all args*/);

        al_array_dispose(p_h_args);
#endif
    }
}

/******************************************************************************
*
* dispose of all args after a given index in an arglist
*
******************************************************************************/

extern void
arglist_dispose_after(
    _InRef_     PC_ARGLIST_HANDLE p_h_args,
    _In_        U32 arg_idx /* U32_MAX -> all args */)
{
    const U32 n_args = n_arglist_args(p_h_args);

    assert((U32_MAX == arg_idx) || (array_offset_is_valid(p_h_args, arg_idx)));

    while(++arg_idx < n_args)
    {
        const P_ARGLIST_ARG p_arg = p_arglist_arg(p_h_args, arg_idx);

        if(p_arg->type & ARG_ALLOC)
        {
            p_arg->type &= ~ARG_ALLOC;

            switch(p_arg->type & ARG_TYPE_MASK)
            {
            default:
                break;

            case ARG_TYPE_RAW:
            case ARG_TYPE_RAW_DS:
                al_array_dispose(&p_arg->val.raw);
                break;

            case ARG_TYPE_USTR:
                ustr_clr(&p_arg->val.ustr_wr);
                break;

#if defined(ARG_TYPE_TSTR_DISTINCT)
            case ARG_TYPE_TSTR:
                tstr_clr(&p_arg->val.tstr_wr);
                break;
#endif /* ARG_TYPE_TSTR_DISTINCT */
            }
        }

        p_arg->type = ARG_TYPE_NONE;
    }
}

/******************************************************************************
*
* duplicate an arglist and all its args
*
******************************************************************************/

_Check_return_
static STATUS
arglist_duplicate_fail(
    _InoutRef_  P_ARGLIST_HANDLE p_h_dst_args,
    _In_        U32 arg_idx)
{
    const U32 n_args = n_arglist_args(p_h_dst_args);
    const P_ARGLIST_ARG p_dst_args = p_arglist_args(p_h_dst_args, n_args);

    /* zap all args we don't have */
    for(; arg_idx < n_args; ++arg_idx)
        p_dst_args[arg_idx].type = ARG_TYPE_NONE;

    arglist_dispose(p_h_dst_args);

    return(status_nomem());
}

_Check_return_
extern STATUS
arglist_duplicate(
    _OutRef_    P_ARGLIST_HANDLE p_h_dst_args,
    _InRef_     PC_ARGLIST_HANDLE p_h_src_args)
{
    STATUS status;
    U32 n_args, arg_idx;
    P_ARGLIST_ARG p_dst_arg, p_src_arg;

    *p_h_dst_args = 0;

    status_return(status = al_array_duplicate(p_h_dst_args, p_h_src_args));

    n_args    = n_arglist_args(p_h_src_args);
    p_src_arg = p_arglist_args(p_h_src_args, n_args);
    p_dst_arg = p_arglist_args(p_h_dst_args, n_args);

    for(arg_idx = 0; arg_idx < n_args; ++arg_idx)
    {
        switch(p_dst_arg[arg_idx].type & ARG_TYPE_MASK)
        {
        default:
            continue;

        case ARG_TYPE_RAW:
        case ARG_TYPE_RAW_DS:
            if(0 != p_src_arg[arg_idx].val.raw)
                if(status_ok(status = al_array_duplicate(&p_dst_arg[arg_idx].val.raw, &p_src_arg[arg_idx].val.raw)))
                    p_dst_arg[arg_idx].type |= ARG_ALLOC;
            break;

        /* The Horror!!! Doesn't cater for inlines */
        case ARG_TYPE_USTR:
            if(NULL != p_src_arg[arg_idx].val.ustr)
                if(status_ok(status = ustr_set(&p_dst_arg[arg_idx].val.ustr_wr, p_src_arg[arg_idx].val.ustr)))
                    p_dst_arg[arg_idx].type |= ARG_ALLOC;
            break;

#if defined(ARG_TYPE_TSTR_DISTINCT)
        case ARG_TYPE_TSTR:
            if(NULL != p_src_arg[arg_idx].val.tstr)
                if(status_ok(status = tstr_set(&p_dst_arg[arg_idx].val.tstr_wr, p_src_arg[arg_idx].val.tstr)))
                    p_dst_arg[arg_idx].type |= ARG_ALLOC;
            break;
#endif /* ARG_TYPE_TSTR_DISTINCT */
        }

        if(status_fail(status))
            return(arglist_duplicate_fail(p_h_dst_args, arg_idx));
    }

    return(status);
}

/******************************************************************************
*
* make an arglist ready to receive the given kinds of data
* NB. not including allocating all fixed args
*
******************************************************************************/

_Check_return_
extern STATUS
arglist_prepare(
    _OutRef_    P_ARGLIST_HANDLE p_h_args,
    _InRef_     PC_ARG_TYPE p_arg_type /*[], terminator is ARG_TYPE_NONE*/)
{
    STATUS status = STATUS_OK;

    *p_h_args = 0;

    if(NULL != p_arg_type)
    {
        PC_ARG_TYPE p_il;
        ARG_TYPE il;
        U32 n_args;

        /* count the number of args required */
        p_il = p_arg_type;

        while(*p_il++ != ARG_TYPE_NONE)
        { /*EMPTY*/ }

        n_args = PtrDiffElemS32((--p_il), p_arg_type); /* ignore the ARG_TYPE_NONE arg */

        if(n_args)
        {
            P_ARGLIST_ARG p_arg = NULL;

#if 1
            /* scan free chain for a free arglist of correct size */
            P_S32 p_free_idx = &arglist_cache.free_idx;

            for(;;)
            {
                P_ARGLIST_CACHE_ENTRY p_arglist_cache_entry;
                S32 free_idx = *p_free_idx;

                if(free_idx == -1)
                {
                    /* end of chain; create new entry and arglist */
                    SC_ARRAY_INIT_BLOCK array_init_block_c = aib_init(1, sizeof32(*p_arglist_cache_entry), FALSE);
                    SC_ARRAY_INIT_BLOCK array_init_block_a = aib_init(1, sizeof32(*p_arg), FALSE);

                    if(NULL == (p_arglist_cache_entry = al_array_extend_by(&arglist_cache.handle, ARGLIST_CACHE_ENTRY, 1, &array_init_block_c, &status)))
                        break; /* p_arg == NULL also */

                    if(NULL == (p_arg = al_array_alloc((P_ARRAY_HANDLE) p_h_args, ARGLIST_ARG, n_args, &array_init_block_a, &status)))
                    {
                        al_array_shrink_by(&arglist_cache.handle, -1);
                        status = STATUS_OK;
                        break;
                    }

                    p_arglist_cache_entry->free_idx = -2; /* alloced */
                    p_arglist_cache_entry->arglist_handle = *p_h_args;

                    ++arglist_cache.entries_in_use;

                    trace_2(TRACE_APP_ARGLIST, TEXT("arglist_cache MISS: alloced ") S32_TFMT TEXT(", ") S32_TFMT TEXT(" args"), *p_h_args, n_arglist_args(p_h_args));

                    break;
                }

                p_arglist_cache_entry = array_ptr(&arglist_cache.handle, ARGLIST_CACHE_ENTRY, free_idx);

                assert(p_arglist_cache_entry->free_idx != -2 /* mustn't be alloced if on free chain! */);

                if((p_arglist_cache_entry->free_idx >= 0) && (n_arglist_args(&p_arglist_cache_entry->arglist_handle) == n_args))
                {
                    /* patch up free chain */
                    *p_free_idx = p_arglist_cache_entry->free_idx;
                    p_arglist_cache_entry->free_idx = -2; /* alloced */

                    *p_h_args = p_arglist_cache_entry->arglist_handle;
                    p_arg = p_arglist_arg(p_h_args, 0);

                    ++arglist_cache.entries_in_use;

                    trace_2(TRACE_APP_ARGLIST, TEXT("arglist_cache HIT: using ") S32_TFMT TEXT(", ") S32_TFMT TEXT(" args"), *p_h_args, n_arglist_args(p_h_args));

                    break;
                }

                p_free_idx = &p_arglist_cache_entry->free_idx;
            }

            if(NULL == p_arg)
                status = status_nomem();
#else
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_arg), TRUE);

            p_arg = al_array_alloc((P_ARRAY_HANDLE) p_h_args, ARGLIST_ARG, n_args, &array_init_block, &status);
#endif

            /* copy over arg types from initialiser to arg type fields, leaving arg val 0 */
            if(NULL != p_arg)
            {
                p_il = p_arg_type;

                while((il = *p_il++) != ARG_TYPE_NONE)
                {
                    p_arg->type = il;
                    p_arg->val.w.w0 = 0;
                    p_arg->val.w.w1 = 0;

                    p_arg++;
                }
            }
        }
    }

    return(status);
}

#if TRACE_ALLOWED

extern void
arglist_trace(
    _InRef_     PC_ARGLIST_HANDLE p_h_args,
    _In_z_      PCTSTR caller)
{
    const U32 n_args = n_arglist_args(p_h_args);
    const PC_ARGLIST_ARG p_args = pc_arglist_args(p_h_args, n_args);
    U32 arg_idx;

    if_constant(!tracing(TRACE_APP_ARGLIST))
        return;

    trace_4(TRACE_APP_ARGLIST, TEXT("%s arglist(") PTR_XTFMT TEXT("->") ARRAY_HANDLE_TFMT TEXT(") - ") U32_TFMT TEXT(" args:"), caller, p_h_args, *p_h_args, n_args);

    for(arg_idx = 0; arg_idx < n_args; ++arg_idx)
    {
        PC_ARGLIST_ARG p_arg = &p_args[arg_idx];

        trace_1(TRACE_APP_ARGLIST, TEXT("  arg ") U32_TFMT TEXT(" |"), arg_idx);

        switch(p_arg->type & ARG_TYPE_MASK)
        {
        default: default_unhandled();
        case ARG_TYPE_NONE:
            break;

        case ARG_TYPE_U8C:
            trace_1(TRACE_APP_ARGLIST, TEXT("|U8C ") U8_TFMT, p_arg->val.u8c);
            break;

        case ARG_TYPE_U8N:
            trace_1(TRACE_APP_ARGLIST, TEXT("|U8N ") S32_TFMT, (S32) p_arg->val.u8n);
            break;

        case ARG_TYPE_BOOL:
            trace_1(TRACE_APP_ARGLIST, TEXT("|BOOL ") S32_TFMT, (S32) p_arg->val.fBool);
            break;

        case ARG_TYPE_S32:
            trace_1(TRACE_APP_ARGLIST, TEXT("|S32 ") S32_TFMT, p_arg->val.s32);
            break;

        case ARG_TYPE_X32:
            trace_1(TRACE_APP_ARGLIST, TEXT("|X32 ") U32_XTFMT, p_arg->val.x32);
            break;

        case ARG_TYPE_COL:
            trace_1(TRACE_APP_ARGLIST, TEXT("|COL ") COL_TFMT, p_arg->val.col);
            break;

        case ARG_TYPE_ROW:
            trace_1(TRACE_APP_ARGLIST, TEXT("|ROW ") ROW_TFMT, p_arg->val.row);
            break;

        case ARG_TYPE_F64:
            trace_1(TRACE_APP_ARGLIST, TEXT("|F64 ") F64_TFMT, p_arg->val.f64);
            break;

        case ARG_TYPE_RAW:
        case ARG_TYPE_RAW_DS:
            if(0 == array_elements(&p_arg->val.raw))
                trace_0(TRACE_APP_ARGLIST, TEXT("|RAW <missing>"));
            else
                trace_1(TRACE_APP_ARGLIST, TEXT("|RAW ") S32_TFMT TEXT(" bytes"), array_elements(&p_arg->val.raw));
            break;

        case ARG_TYPE_USTR:
            if(NULL == p_arg->val.ustr)
                trace_0(TRACE_APP_ARGLIST, TEXT("|USTR <missing>"));
            else if(contains_inline(p_arg->val.ustr, ustrlen32(p_arg->val.ustr)))
                trace_2(TRACE_APP_ARGLIST, TEXT("|USTR (") U32_TFMT TEXT(") %s"), ustr_inline_strlen32(p_arg->val.ustr_inline), report_ustr_inline(p_arg->val.ustr_inline));
            else
                trace_2(TRACE_APP_ARGLIST, TEXT("|USTR (") U32_TFMT TEXT(") %s"), ustrlen32(p_arg->val.ustr), report_ustr(p_arg->val.ustr));
            break;

#if defined(ARG_TYPE_TSTR_DISTINCT)
        case ARG_TYPE_TSTR:
            if(NULL == p_arg->val.tstr)
                trace_0(TRACE_APP_ARGLIST, TEXT("|TSTR <missing>"));
            else if(contains_inline(p_arg->val.tstr, sizeof32(TCHAR) * tstrlen32(p_arg->val.tstr)))
                trace_2(TRACE_APP_ARGLIST, TEXT("|TSTR (") U32_TFMT TEXT(") %s"), tstrlen32(p_arg->val.tstr), TEXT("CONTAINS INLINES"));
            else
                trace_2(TRACE_APP_ARGLIST, TEXT("|TSTR (") U32_TFMT TEXT(") %s"), tstrlen32(p_arg->val.tstr), report_tstr(p_arg->val.tstr));
            break;
#endif /* ARG_TYPE_TSTR_DISTINCT */
        }
    }
}

#endif /* TRACE_ALLOWED */

/* end of arglist.c */
