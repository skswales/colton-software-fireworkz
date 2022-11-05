/* debug.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS/MRJC February and June 1991 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/* Allow TRACE_ALLOWED even in RELEASED code if really wanted */
#undef  TRACE_ALLOWED
#define TRACE_ALLOWED 1

#undef tracing /* Keeps VS2012 quiet */

#include "cmodules/debug.h"

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

/*extern*/ S32 trace__count = 0;

static BOOL trace__enabled     = 1;
static BOOL trace__initialised = 0;

/******************************************************************************
*
* tracef routine
*
******************************************************************************/

extern void __cdecl
tracef(
    _InVal_     U32 mask,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...)
{
    va_list args;

    if(!tracing(mask) || !trace__enabled)
        return;

    va_start(args, format);
    vreportf(format, args);
    va_end(args);
}

extern void
trace_disable(void)
{
    while(trace__count)
        trace_off();

    trace__enabled = FALSE;
}

_Check_return_
extern BOOL
trace_is_on(void)
{
    return(0 != trace__count);
}

extern void
trace_off(void)
{
    if(0 == trace__count)
        return;

    --trace__count;
}

extern void
trace_on(void)
{
    if(!trace__enabled)
        return;

    if(trace__count++ != 0)
        return;

    if(trace__initialised)
        return;

#if RISCOS
    setbuf(stderr, NULL);
#endif

    trace__initialised = TRUE;
}

#if defined(TRACE_LIST)

#define writetodebugf vreportf

extern void
trace_list(
    _InVal_     U32 mask,
    P_ANY /*P_LIST_BLOCK*/ e_p_list_block)
{
    P_LIST_BLOCK p_list_block = (P_LIST_BLOCK) e_p_list_block;
    ARRAY_INDEX i;
    LIST_ITEMNO high_itemno = -1;
    LIST_ITEMNO last_itemno = -1;

    if(!tracing(mask))
        return;

    writetodebugf(TEXT("list ") PTR_XTFMT TEXT(", numitem ") U32_XTFMT TEXT(", offsetc ") U16_XTFMT TEXT(", itemc ") U16_XTFMT TEXT(", ix_pooldesc ") U32_XTFMT TEXT(", h_pooldesc ") U32_XTFMT TEXT(", n_pooldesc ") U32_XTFMT,
                  p_list_block, (S32) p_list_block->numitem,
                  (U16) p_list_block->offsetc, (U16) p_list_block->itemc,
                  (S32) p_list_block->ix_pooldesc, (S32) p_list_block->h_pooldesc,
                  array_elements32(&p_list_block->h_pooldesc));

    for(i = 0; i < array_elements(&p_list_block->h_pooldesc); ++i)
    {
        P_POOLDESC p_pooldesc = array_ptr(&p_list_block->h_pooldesc, POOLDESC, i);

        writetodebugf(TEXT("  pool ") U32_XTFMT TEXT(" %s, ") PTR_XTFMT TEXT(", h_pool ") U32_XTFMT TEXT(", pool item ") U32_XTFMT TEXT(", pool bytes ") U32_XTFMT,
                      (S32) i, ((i == p_list_block->ix_pooldesc) ? "(*current*) " : ""),
                      p_pooldesc, (S32) p_pooldesc->h_pool,
                      (S32) p_pooldesc->poolitem, array_elements32(&p_pooldesc->h_pool));

        if(0 != array_elements(&p_pooldesc->h_pool))
        {
            P_LIST_ITEM base_it = array_base(&p_pooldesc->h_pool, LIST_ITEM);
            P_LIST_ITEM it      = base_it;
            P_LIST_ITEM end_it  = PtrAddBytes(P_LIST_ITEM, base_it, array_elements32(&p_pooldesc->h_pool);
            LIST_ITEMNO itemno  = p_pooldesc->poolitem;

            if(itemno < last_itemno)
                writetodebugf(TEXT("*** list corrupt ***: itemno ") U32_XTFMT TEXT(" < last pool itemno ") U32_XTFMT, (S32) itemno, (S32) last_itemno);

            while(it != end_it)
            {
                OFF_TYPE itemsize;

                if(it == base_it)
                    if(it->offsetp)
                        writetodebugf(TEXT("*** pool corrupt ***: base it ") PTR_XTFMT TEXT(" has non-zero offsetp ") U16_XTFMT, it, (U16) it->offsetp);

                if(it->offsetn)
                    itemsize = (OFF_TYPE) it->offsetn;
                else
                    itemsize = (OFF_TYPE) ((PC_U8) end_it - (PC_U8) it);

                if(it->fill)
                {
                    writetodebugf(TEXT("    item ") U32_XTFMT TEXT(" ") PTR_XTFMT TEXT(", filler ") U32_XTFMT, (S32) itemno, it, (S32) it->i.itemfill);

                    if(itemsize != sizeof32(*it))
                        writetodebugf(TEXT("*** filler corrupt ***: size ") U16_XTFMT TEXT(" > sizeof-LIST_ITEM ") U16_XTFMT, (U16) itemsize, (U16) sizeof32(*it));
                }
                else
                {
                    writetodebugf(TEXT("    item ") U32_XTFMT TEXT(" ") PTR_XTFMT TEXT(", contents ") PTR_XTFMT TEXT(", size ") U16_XTFMT, (S32) itemno, it, list_itemcontents(void, it), itemsize - offsetof32(LIST_ITEM, i));
                    high_itemno = MAX(high_itemno, itemno);
                }

                itemno += list_leapnext(it);

                if(itemno > list_numitem(p_list_block))
                    writetodebugf(TEXT("*** list corrupt ***: item ") U32_XTFMT TEXT(" > numitem ") U32_XTFMT, (S32) itemno, (S32) list_numitem(p_list_block));

                it = (P_ANY) ((P_U8) it + itemsize);

                if(it > end_it)
                {
                    writetodebugf(TEXT("  *** pool corrupt ***: it ") PTR_XTFMT TEXT(" > end_it ") PTR_XTFMT, it, end_it);
                    break;
                }
            }

            writetodebugf(TEXT("  pool end, itemno ") U32_XTFMT, (S32) itemno);
            last_itemno = itemno;
        }
    }

    writetodebugf(TEXT("highest non-filler itemno ") U32_XTFMT TEXT(", highest ") U32_XTFMT TEXT(", numitem ") U32_XTFMT, (S32) high_itemno, (S32) last_itemno, (S32) list_numitem(p_list_block));
}

#endif /* TRACE_LIST */

/* end of debug.c */
