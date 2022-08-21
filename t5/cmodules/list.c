/* list.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* List manager derived from handlist for fixed memory */

/* MRJC December 1991 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#define FILLSIZE (sizeof32(union LIST_ITEM_GUTS))

#ifdef LIST_CACHE

/*
list cache definition
*/

typedef struct LIST_CACHE_ENTRY
{
    LIST_ITEMNO list_itemno;
    P_LIST_ITEM p_list_item;
}
LIST_CACHE_ENTRY, * P_LIST_CACHE_ENTRY;

#endif /* LIST_CACHE */

/*
internal functions
*/

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
addafter(
    _InoutRef_  P_LIST_BLOCK lp,
    _InVal_     S32 size,
    _In_        LIST_ITEMNO adjust);

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
addbefore(
    _InoutRef_  P_LIST_BLOCK lp,
    _InVal_     S32 size,
    _In_        LIST_ITEMNO adjust);

#ifdef LIST_CACHE

static void
cache_dispose(
    _InoutRef_  P_LIST_BLOCK p_list_block);

#endif

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
convofftoptr(
    _InoutRef_  P_LIST_BLOCK lp,
    _InVal_     S32 poolix,
    _In_        OFF_TYPE off);

static LIST_ITEMNO
deallocitem(
    _InoutRef_  P_LIST_BLOCK lp,
    P_LIST_ITEM it);

static void
deallocpool(
    _InoutRef_  P_LIST_BLOCK lp,
    _InVal_     S32 poolix);

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
fillafter(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO itemfill,
    _In_        LIST_ITEMNO adjust);

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
fillbefore(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO itemfill,
    _In_        LIST_ITEMNO adjust);

#ifdef LIST_CACHE

_Check_return_
_Ret_notnull_
static P_LIST_ITEM
gotoitem_opt(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item);

#endif

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
gotoitem_i(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item);

#define pooldesc_ptr(lp, ix) \
    array_ptr(&(lp)->h_pooldesc, POOLDESC, (ix))

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
reallocitem(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        S32 new_size,
    P_LIST_ITEM it,
    _In_        LIST_ITEMNO adjust);

_Check_return_
static STATUS
splitpool(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        OFF_TYPE size_needed,
    _In_        LIST_ITEMNO adjust);

static void
updatepoolitems(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO change);

/*

static data
*/

/******************************************************************************
*
* create an item for a particular list and item of a given size
*
* the size must include any extra space required apart from the item overhead itself:
* a size of 0 means a blank item; a size of 1 leaves space for a delimiter.
*
******************************************************************************/

#define FILLSET 1

_Check_return_
_Ret_writes_bytes_to_maybenull_(size, 0)
extern P_LIST_ITEM
list_createitem(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item,
    _In_        S32 size,
    _In_        S32 fill)
{
    LIST_ITEMNO itemcur, newfill;
    P_LIST_ITEM newitem, it;
    S32 poolsave;
    OFF_TYPE offsetsave;

    /* check not past maximum conceivable size */
    assert(size >= 0);
    if((OFF_TYPE) size >= MAX_ITEM)
        return(NULL);

    it = NULL;

    newitem = list_gotoitem_opt(lp, item);

#ifdef LIST_CACHE
    cache_dispose(lp);
#endif

    /* save parameters for address reconstruction */
    poolsave = lp->ix_pooldesc;
    offsetsave = lp->offsetc;

    if(NULL == newitem)
    {
        trace_4(TRACE_MODULE_LIST, TEXT("list_createitem *: lp: ") PTR_XTFMT TEXT(", lp->offsetc: ") S32_TFMT TEXT(", item: ") S32_TFMT TEXT(", atitem: ") S32_TFMT,
                lp, (S32) lp->offsetc, (S32) item, (S32) list_atitem(lp));

        it = gotoitem_i(lp, (itemcur = list_atitem(lp)));

        /* save parameters for address reconstruction */
        poolsave = lp->ix_pooldesc;
        offsetsave = lp->offsetc;

        if(it && it->fill)
        {
            if((itemcur + it->i.itemfill) > item)
            {
                /* check for creating hole */
                if(!size && !fill)
                    return(gotoitem_i(lp, itemcur));

                /* need to split filler block; newfill becomes
                the size of the lower filler block */
                if((newfill = (itemcur + it->i.itemfill -
                                item - FILLSET)) != 0)
                {
                    /* adjust higher filler for new filler */
                    it->i.itemfill -= newfill;

                    if(!fillafter(lp, newfill, newfill))
                    {
                        /* replace earlier state */
                        P_LIST_ITEM it_earlier =
                        convofftoptr(lp,
                                     poolsave,
                                     offsetsave);
                        PTR_ASSERT(it_earlier);
                        it_earlier->i.itemfill += newfill;
                        return(NULL);
                    }

                    /* re-load higher fill pointer */
                    it = gotoitem_i(lp, itemcur);
                    PTR_ASSERT(it);
                }

                /* if filler is only 1 big, cause
                new item to overwrite it */
                if(it->i.itemfill == 1)
                {
                    newitem = it;
                    it = NULL;
                }
                /* otherwise make filler 1 smaller
                anticipating item being created */
                else
                    it->i.itemfill -= FILLSET;
            }
            else
                /* found a trailing filler - need to enlarge */
            {
                it->i.itemfill += item - (itemcur + it->i.itemfill);
                it = NULL;

                /* update numitem record */
                if( lp->numitem < item + 1)
                    lp->numitem = item + 1;
            }
        }
        /* adding past end of list - filler item needed */
        else if((newfill = (it ? item - (itemcur + list_leapnext(it))
                               : item - itemcur)) > 0)
        {
            if(!fillafter(lp, newfill, (LIST_ITEMNO) 0))
                return(NULL);
            it = NULL;

            /* update numitem record */
            if(item >= lp->numitem)
                /* in case where addafter of actual item fails (below), and we
                 * added a trailing filler to an existing list, numitem was 1
                 * too big
                 * MRJC 23.9.91
                 */
                lp->numitem = itemcur + newfill + 1;
        }
        else
            it = NULL;
    }

    if(fill || !size)
    {
        fill = TRUE;
        size = FILLSIZE;
    }

    if(!newitem)
    {
        /* allocate actual item */
        if(NULL == (newitem = addafter(lp, size, (LIST_ITEMNO) FILLSET)))
        {
            /* correct higher filler since we couldn't insert */
            if(it)
                convofftoptr(lp, poolsave, offsetsave)->i.itemfill += FILLSET;
            return(NULL);
        }

        /* update numitem record */
        if( lp->numitem < item + 1)
            lp->numitem = item + 1;
    }
    else
    {
        /* item exists - ensure the same size */
        if(NULL == (newitem = reallocitem(lp, size, newitem, 0)))
            return(NULL);
    }

    /* set filler flag if it is a filler or a hole */
    if(fill)
    {
        newitem->fill = 1;
        newitem->i.itemfill = FILLSET;
    }
    else
        newitem->fill = 0;

    return(newitem);
}

/******************************************************************************
*
* delete an item from the list
*
******************************************************************************/

extern void
list_deleteitems(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item,
    _In_        LIST_ITEMNO numdel)
{
    P_LIST_ITEM it;
    LIST_ITEMNO removed;

#ifdef LIST_CACHE
    cache_dispose(lp);
#endif

    do  {
        if(NULL == (it = gotoitem_i(lp, item)))
            return;

        if(it->fill && (list_leapnext(it) > numdel))
        {
            it->i.itemfill -= numdel;
            updatepoolitems(lp, -numdel);
            return;
        }

        /* give up if we're about to delete the wrong item */
        if(list_atitem(lp) != item)
            return;

        removed = deallocitem(lp, it);
        numdel -= removed;

        trace_2(TRACE_MODULE_LIST, TEXT("list_deleteitems before update numitem: ") S32_TFMT TEXT(", removed: ") S32_TFMT, (S32) lp->numitem, (S32) removed);

        updatepoolitems(lp, -removed);

        trace_1(TRACE_MODULE_LIST, TEXT("list_deleteitems after update numitem: numitem: ") S32_TFMT, (S32) lp->numitem);
    }
    while(numdel);
}

#if defined(UNUSED_KEEP_ALIVE)

/******************************************************************************
*
* ensure we have a item break at the specified position - insert or split filler if necessary
*
******************************************************************************/

_Check_return_
extern STATUS
list_ensureitem(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item)
{
    if(NULL != list_gotoitem_opt(lp, item))
        return(STATUS_OK);

    if(NULL == list_createitem(lp, item, FILLSIZE, TRUE))
        return(status_nomem());

    return(STATUS_OK);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* return the size in bytes of an entry
*
* the number returned may be bigger than the size originally allocated since it may have been rounded up for alignment
*
******************************************************************************/

extern S32
list_entsize(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item)
{
    P_LIST_ITEM it;
    P_POOLDESC p_pooldesc;

    if(NULL == (it = list_gotoitem_opt(lp, item)))
        return(STATUS_FAIL);

    if(it->offsetn)
        return((OFF_TYPE) it->offsetn - (OFF_TYPE) LIST_ITEMOVH);

    p_pooldesc = pooldesc_ptr(lp, lp->ix_pooldesc);

    return(array_elements(&p_pooldesc->h_pool) - PtrDiffBytesS32(it, array_base(&p_pooldesc->h_pool, U8)) - LIST_ITEMOVH);
}

/******************************************************************************
*
* free a whole list
*
******************************************************************************/

extern void
list_free(
    _InoutRef_  P_LIST_BLOCK lp)
{
    if(lp)
    {
        ARRAY_INDEX i = array_elements(&lp->h_pooldesc);

        while(--i >= 0)
            deallocpool(lp, i);

        lp->numitem = 0;
        lp->offsetc = 0;
        lp->itemc   = 0;

#ifdef LIST_CACHE
        al_array_dispose(&lp->h_list_cache);
#endif
    }
}

/******************************************************************************
*
* garbage collect for a list, removing adjacent filler blocks
*
******************************************************************************/

extern S32
list_garbagecollect(
    _InoutRef_  P_LIST_BLOCK lp)
{
    LIST_ITEMNO item = 0;
    S32 res = 0;

#ifdef LIST_CACHE
    cache_dispose(lp);
#endif

    while(item < list_numitem(lp))
    {
        P_LIST_ITEM it;

        if(NULL == (it = gotoitem_i(lp, item)))
            break;
        else
        {
            LIST_ITEMNO item_next;

            item_next = item + list_leapnext(it);

            if(it->fill && item == list_atitem(lp))
            {
                P_LIST_ITEM it_next;

                it_next = gotoitem_i(lp, item_next);

                if(it_next &&
                   it_next->fill &&
                   ((item_next == list_atitem(lp)) || (item_next == list_numitem(lp)))
                  )
                {
                    LIST_ITEMNO fill = deallocitem(lp, it_next);

                    updatepoolitems(lp, -fill);

                    if( (NULL != (it = gotoitem_i(lp, item))) &&
                        it->fill
                      )
                    {
                        it->i.itemfill += fill;
                        updatepoolitems(lp, fill);
                    }

                    res += 1;
                    item_next = item;

                    trace_0(TRACE_MODULE_LIST, TEXT("Filler recovered"));
                }
            }

            item = item_next;
        }
    }

    trace_1(TRACE_MODULE_LIST, S32_TFMT TEXT(" fillers recovered"), res);
    return(res);
}

/******************************************************************************
*
* external veneer to gotoitem
*
******************************************************************************/

#ifdef LIST_CACHE

_Check_return_
_Ret_maybenull_
extern P_LIST_ITEM
list_gotoitem_opt(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _In_        LIST_ITEMNO list_itemno)
{
    P_LIST_ITEM p_list_item;

    {
    if(p_list_block && p_list_block->h_pooldesc && p_list_block->itemc == list_itemno)
    {
        P_POOLDESC p_pooldesc = pooldesc_ptr(p_list_block, p_list_block->ix_pooldesc);
        if(p_pooldesc->h_pool)
        {
            p_list_item = (P_LIST_ITEM) array_ptr(&p_pooldesc->h_pool, U8, p_list_block->offsetc);
            return(p_list_item->fill ? NULL : p_list_item);
        }
    }
    } /*block*/

    {
    ARRAY_INDEX n_cache = array_elements(&p_list_block->h_list_cache);

    if(n_cache)
    {
        ARRAY_INDEX i;
        P_LIST_CACHE_ENTRY p_list_cache_entry = array_range(&p_list_block->h_list_cache, 0, n_cache);

        for(i = 0; i < n_cache; ++i, ++p_list_cache_entry)
            if(p_list_cache_entry->list_itemno == list_itemno)
                return(p_list_cache_entry->p_list_item);
    }

    p_list_item = gotoitem_ptr(p_list_block, list_itemno);

    if(n_cache)
    {
        ARRAY_INDEX replace_entry = host_rand_between(0, n_cache);
        P_LIST_CACHE_ENTRY p_list_cache_entry;

        p_list_cache_entry = array_ptr(&p_list_block->h_list_cache, LIST_CACHE_ENTRY, replace_entry);
        p_list_cache_entry->list_itemno = list_itemno;
        p_list_cache_entry->p_list_item = p_list_item;
    }
    } /*block*/

    return(p_list_item);
}

#endif /* LIST_CACHE */

/******************************************************************************
*
* travel to a particular item
*
* due to the structure used, where a item may conceptually exist,
* but an explicit entry in the list chain does not exist, travel()
* will return a NULL pointer for a item that exists but has no entry
* of its own in the structure
*
* the function list_atitem() must then be called to return the actual item that was achieved
*
******************************************************************************/

#ifdef LIST_CACHE

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
gotoitem_opt(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item)

#else

_Check_return_
_Ret_maybenull_
extern P_LIST_ITEM
list_gotoitem_opt(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item)

#endif
{
    LIST_ITEMNO i, t;
    P_LIST_ITEM it;
    P_POOLDESC p_pooldesc;

    PTR_ASSERT(lp);

    if(!lp->h_pooldesc)
        return(NULL);

    /* get descriptor pointer */
    p_pooldesc = pooldesc_ptr(lp, lp->ix_pooldesc);

    /* check there is a pool */
    if(!p_pooldesc->h_pool)
        return(NULL);

    it = (P_LIST_ITEM) array_ptr(&p_pooldesc->h_pool, U8, lp->offsetc);

    if((i = lp->itemc) == item)
        return(!it->fill ? it : NULL);

    /* skip backwards to pool if necessary */
    if(item < p_pooldesc->poolitem)
    {
        do
        {
            --lp->ix_pooldesc;
            --p_pooldesc;
        }
        while(item < p_pooldesc->poolitem);

        i = p_pooldesc->poolitem;
        it = (P_LIST_ITEM) array_ptr(&p_pooldesc->h_pool, U8, 0);
    }

    if(item > i)
    {
        for(;;)
        {
            /* go down list */
            while(it->offsetn)
            {
                if((i + (t = list_leapnext(it))) > item)
                    goto there;
                it = (P_LIST_ITEM) (((P_U8) it) + it->offsetn);
                if((i += t) == item)
                    goto there;
            }

            if((lp->ix_pooldesc + 1 < array_elements(&lp->h_pooldesc)) &&
               (item >= (p_pooldesc + 1)->poolitem))
            {
                do
                {
                    ++lp->ix_pooldesc;
                    ++p_pooldesc;
                }
                while((lp->ix_pooldesc + 1 < array_elements(&lp->h_pooldesc)) &&
                      (item >= (p_pooldesc + 1)->poolitem));

                i = p_pooldesc->poolitem;
                it = array_base(&p_pooldesc->h_pool, LIST_ITEM);
            }
            else
                break;
        }
    }
    else if(item < i)
        /* go up the list */
    {
        do  {
            it = (P_LIST_ITEM) (((P_U8) it) - it->offsetp);
        } while((i -= list_leapnext(it)) > item);
    }

there:

    lp->itemc = i;
    lp->offsetc = (OFF_TYPE) ((PC_BYTE) it - array_ptrc(&p_pooldesc->h_pool, BYTE, 0));
    return((i == item) && !it->fill ? it : NULL);
}

_Check_return_
_Ret_notnull_
extern P_LIST_ITEM
list_gotoitem(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item)
{
    P_LIST_ITEM p_list_item = list_gotoitem_opt(lp, item);
    PTR_ASSERT(p_list_item);
    return(p_list_item);
}

/******************************************************************************
*
* list block may have data?
* use to check whether it is worth enumerating the list for deletion of item contents
*
******************************************************************************/

_Check_return_
extern BOOL
list_has_data(
    _InRef_     P_LIST_BLOCK p_list_block)
{
    if(NULL == p_list_block)
        return(FALSE);

    return(0 != array_elements(&p_list_block->h_pooldesc));
}

/******************************************************************************
*
* initialise a list block
*
******************************************************************************/

extern void
list_init(
    _OutRef_    P_LIST_BLOCK p_list_block)
{
    p_list_block->offsetc = 0;
    p_list_block->itemc = 0;
    p_list_block->ix_pooldesc = 0;
    p_list_block->h_pooldesc = 0;
    p_list_block->numitem = 0;
#ifdef LIST_CACHE
    p_list_block->h_list_cache = 0;
#else
    p_list_block->spare = 0;
#endif
}

/******************************************************************************
*
* set up a cache for a list
*
******************************************************************************/

#ifdef LIST_CACHE

_Check_return_
extern STATUS
list_init_cache(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InVal_     S32 n_items)
{
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(LIST_CACHE_ENTRY), TRUE)
    STATUS status;

    if(NULL == al_array_extend_by(&p_list_block->h_list_cache, LIST_CACHE_ENTRY, n_items, &array_init_block, &status))
        return(status);

    cache_dispose(p_list_block);

    return(STATUS_OK);
}

#endif /* LIST_CACHE */

/******************************************************************************
*
* initialise a sequence
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_LIST_ITEM
list_initseq(
    _InoutRef_  P_LIST_BLOCK lp,
    _InoutRef_  P_LIST_ITEMNO p_itemno)
{
    P_LIST_ITEM it;

#ifdef SPARSE_SEQ_DEBUG
    trace_3(TRACE_MODULE_LIST, TEXT("list_initseq(") PTR_XTFMT TEXT(", ") PTR_XTFMT TEXT(" (current ") S32_TFMT TEXT("))"), lp, p_itemno, (S32) *p_itemno);
#endif

    if(NULL == (it = gotoitem_i(lp, *p_itemno)))
        return(NULL);

    if(!it->fill)
    {
        *p_itemno = list_atitem(lp);
        return(it);
    }

    return(list_nextseq(lp, p_itemno));
}

/******************************************************************************
*
* insert items into a list
*
******************************************************************************/

_Check_return_
extern STATUS
list_insertitems(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item,
    _In_        LIST_ITEMNO numins)
{
    P_LIST_ITEM it, psl;

    /* if there's no item there, do nothing */
    if( (NULL == (it = gotoitem_i(lp, item))) ||
        (item >= list_numitem(lp)) )
        return(STATUS_OK);

    /* if we have a filler item either side,
     * add to the filler item to insert
    */
    if(it->fill)
    {
        it->i.itemfill += numins;
        updatepoolitems(lp, numins);
        return(STATUS_OK);
    }

    psl = (item == 0) ? NULL : gotoitem_i(lp, item - 1);
    if(psl && psl->fill)
    {
        psl->i.itemfill += numins;
        updatepoolitems(lp, numins);
        return(STATUS_OK);
    }

    /* create filler at insert position */
    (void) gotoitem_i(lp, item);

    if(!fillbefore(lp, numins, (LIST_ITEMNO) 0))
        return(status_nomem());

    updatepoolitems(lp, numins);

    return(STATUS_OK);
}

/******************************************************************************
*
* get next item in sequence
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_LIST_ITEM
list_nextseq(
    _InoutRef_  P_LIST_BLOCK lp,
    _InoutRef_  P_LIST_ITEMNO p_itemno)
{
    P_LIST_ITEM it;
    P_POOLDESC p_pooldesc;

#ifdef SPARSE_SEQ_DEBUG
    trace_4(TRACE_MODULE_LIST, TEXT("list_nextseq(") PTR_XTFMT TEXT(", ") PTR_XTFMT TEXT(" (current ") S32_TFMT TEXT(")): numitem = ") S32_TFMT, lp, p_itemno, (S32) *p_itemno, (S32) list_numitem(lp));
#endif

    /* get current item pointer */
    if( (lp->itemc != *p_itemno) ||
       !(lp->h_pooldesc)        ||
        ((p_pooldesc = pooldesc_ptr(lp, lp->ix_pooldesc))->h_pool == 0)
      )
    {
        if(NULL == (it = gotoitem_i(lp, *p_itemno)))
            return(NULL);
    }
    else
        it = (P_LIST_ITEM) array_ptr(&p_pooldesc->h_pool, U8, lp->offsetc);

    trace_1(TRACE_MODULE_LIST, TEXT("list_nextseq: it = ") PTR_XTFMT, it);

    /* skip over fillers to next real row */
    do  {
        /* work out next row boundary and move to it */
        *p_itemno = lp->itemc + list_leapnext(it);

        trace_1(TRACE_MODULE_LIST, TEXT("list_nextseq: *p_itemno := ") S32_TFMT, (S32) *p_itemno);

        if(*p_itemno >= list_numitem(lp))
            return(NULL);
        it = gotoitem_i(lp, *p_itemno);
        PTR_ASSERT(it);
    }
    while(it->fill);

    trace_1(TRACE_MODULE_LIST, TEXT("list_nextseq: returning it = ") PTR_XTFMT, it);

    return(it);
}

/******************************************************************************
*
* get next item in sequence
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_LIST_ITEM
list_prevseq(
    _InoutRef_  P_LIST_BLOCK lp,
    _InoutRef_  P_LIST_ITEMNO p_itemno)
{
    P_LIST_ITEM it;
    LIST_ITEMNO item;

    if((item = *p_itemno) == 0)
        return(NULL);

    do
    {
        it = gotoitem_i(lp, item - 1);
        item = list_atitem(lp);
        PTR_ASSERT(it);
    }
    while(it->fill && item);

    *p_itemno = item;

    if(it->fill)
        return(NULL);

    return(it);
}

/******************************************************************************
*
* work out the memory used in a list
*
******************************************************************************/

#if TRACE_ALLOWED

extern void
list_size(
    _InRef_     P_LIST_BLOCK p_list_block,
    _OutRef_    P_U32 p_used,
    _OutRef_    P_U32 p_size,
    _OutRef_    P_U32 p_pools)
{
    ARRAY_INDEX i;
    P_POOLDESC p_pooldesc;

    *p_used = *p_size = *p_pools = 0;

    for(i = 0, p_pooldesc = array_range(&p_list_block->h_pooldesc, POOLDESC, 0, array_elements(&p_list_block->h_pooldesc));
        i < array_elements(&p_list_block->h_pooldesc);
        ++i, ++p_pooldesc)
    {
        if(p_pooldesc->h_pool)
        {
            *p_pools += 1;
            *p_used += array_elements32(&p_pooldesc->h_pool);
            *p_size += array_size32(&p_pooldesc->h_pool);
        }
    }
}

#endif /* TRACE_ALLOWED */

/******************************************************************************
*
* add in item to list after current position
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
addafter(
    _InoutRef_  P_LIST_BLOCK lp,
    _InVal_     S32 size,
    _In_        LIST_ITEMNO adjust)
{
    P_LIST_ITEM it;
    BOOL old_offsetc_set; /* SKS 05nov93 for WINDOWS */
    OFF_TYPE old_offsetc;
    LIST_ITEMNO old_itemc;
    P_POOLDESC p_pooldesc;

    if(lp->h_pooldesc && (p_pooldesc = pooldesc_ptr(lp, lp->ix_pooldesc))->h_pool)
    {
        old_offsetc_set = 1;
        old_offsetc = lp->offsetc;
        old_itemc = lp->itemc;

        it = (P_LIST_ITEM) array_ptr(&p_pooldesc->h_pool, U8, lp->offsetc);
        if(it->offsetn)
            lp->offsetc += (OFF_TYPE) it->offsetn;
        else
            lp->offsetc = (OFF_TYPE) array_elements(&p_pooldesc->h_pool);
        lp->itemc += list_leapnext(it);
    }
    else
    {
        old_offsetc_set = 0;
        old_offsetc = 0;
        old_itemc = 0;
    }

    it = addbefore(lp, size, adjust);

    if(!it && old_offsetc_set)
    {
        lp->offsetc = old_offsetc;
        lp->itemc = old_itemc;
    }

    return(it);
}

/******************************************************************************
*
* add in item to list before current position
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
addbefore(
    _InoutRef_  P_LIST_BLOCK lp,
    _InVal_     S32 size,
    _In_        LIST_ITEMNO adjust)
{
    P_LIST_ITEM it;

    /* allocate a new item */
    if(NULL != (it = reallocitem(lp, size, NULL, adjust)))
        it->fill = 0;

    return(it);
}

/******************************************************************************
*
* allocate a memory pool
*
******************************************************************************/

_Check_return_
static STATUS
allocpool(
    _InoutRef_  P_LIST_BLOCK lp,
    _InVal_     S32 poolix,
    _In_        OFF_TYPE size)
{
    P_POOLDESC p_pooldesc;
    S32 i;
    S32 pool_inc;
    ARRAY_HANDLE h_pool;
    static /*poked*/ ARRAY_INIT_BLOCK pool_init = aib_init(0, sizeof32(U8), FALSE);
    STATUS status;
    SC_ARRAY_INIT_BLOCK pooldesc_init = aib_init(POOLDBLKSIZEINC, sizeof32(POOLDESC), FALSE);

    /* try to allocate new pool
     * we use the initial size of the pool as the pool increment
     * i.e. (typically) the size of the first item in the pool
     * which seems to be as good a number as any
     */
    pool_inc = MIN(MAX_POOL, size * 5);
    pool_init.size_increment = pool_inc;

    if(NULL == al_array_alloc_U8(&h_pool, size, &pool_init, &status))
        return(status);

    al_array_auto_compact_set(&h_pool);

    /* get room in pool descriptor */
    if(NULL == (p_pooldesc = al_array_extend_by(&lp->h_pooldesc, POOLDESC, 1, &pooldesc_init, &status)))
    {
        al_array_dispose(&h_pool);
        return(status);
    }

    al_array_auto_compact_set(&lp->h_pooldesc);

    /* insert new pool into pool descriptor block */
    for(i = array_elements(&lp->h_pooldesc) - 1; i > poolix; --i, --p_pooldesc)
        p_pooldesc[0] = p_pooldesc[-1];

    /* initialise new pool descriptor */
    PTR_ASSERT(p_pooldesc); /* VC2008 /analyze [-1] whinge */
    p_pooldesc->poolitem = 0;
    p_pooldesc->h_pool = 0;

    /* adjust current index for insert */
    if(lp->ix_pooldesc >= poolix && lp->ix_pooldesc != array_elements(&lp->h_pooldesc) - 1)
        ++lp->ix_pooldesc;

    p_pooldesc->h_pool = h_pool;

    return(STATUS_OK);
}

/******************************************************************************
*
* dispose of any cache entries
*
******************************************************************************/

#ifdef LIST_CACHE

static void
cache_dispose(
    _InoutRef_  P_LIST_BLOCK p_list_block)
{
    ARRAY_INDEX i;
    P_LIST_CACHE_ENTRY p_list_cache_entry = array_range(&p_list_block->h_list_cache, LIST_CACHE_ENTRY, 0, array_elements(&p_list_block->h_list_cache));

    for(i = 0; i < array_elements(&p_list_block->h_list_cache); ++i, ++p_list_cache_entry)
    {
        p_list_cache_entry->list_itemno = -1;
    }
}

#endif /* LIST_CACHE */

/******************************************************************************
*
* extract item pointer from list block
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
convofftoptr(
    _InoutRef_  P_LIST_BLOCK lp,
    _InVal_     S32 poolix,
    _In_        OFF_TYPE off)
{
    P_POOLDESC p_pooldesc;

    if(!lp->h_pooldesc)
        return(NULL);

    p_pooldesc = pooldesc_ptr(lp, poolix);

    if(!p_pooldesc->h_pool)
        return(NULL);

    return((P_LIST_ITEM) array_ptr(&p_pooldesc->h_pool, U8, off));
}

/******************************************************************************
*
* deallocate an item
*
******************************************************************************/

static LIST_ITEMNO /* number of items removed */
deallocitem(
    _InoutRef_  P_LIST_BLOCK lp,
    P_LIST_ITEM it)
{
    OFF_TYPE size;
    OFF_TYPE itoff;
    P_LIST_ITEM nextsl, prevsl;
    P_POOLDESC p_pooldesc;
    LIST_ITEMNO removed;

    p_pooldesc = pooldesc_ptr(lp, lp->ix_pooldesc);

    removed = list_leapnext(it);

    /* work out size */
    itoff = (OFF_TYPE) ((PC_BYTE) it - array_ptrc(&p_pooldesc->h_pool, BYTE, 0));

    if(it->offsetn)
        size = (OFF_TYPE) it->offsetn;
    else
    {
        if(it->offsetp)
            size = (OFF_TYPE) array_elements(&p_pooldesc->h_pool) - itoff;
        else
        {
            deallocpool(lp, lp->ix_pooldesc);
            return(removed);
        }
    }

    /* sort out item links */
    if(it->offsetn)
    {
        nextsl = (P_LIST_ITEM) (((P_U8) it) + it->offsetn);

        /* check for special case of trailing filler item */
        if(!it->offsetp &&
           !nextsl->offsetn &&
           nextsl->fill &&
           ((lp->ix_pooldesc + 1) == array_elements(&lp->h_pooldesc)))
        {
            removed += list_leapnext(nextsl);
            deallocpool(lp, lp->ix_pooldesc);
            return(removed);
        }

        nextsl->offsetp = it->offsetp;
    }
    else
    {
        /* step back to previous item so we don't hang in space */
        prevsl = (P_LIST_ITEM) (((P_U8) it) - it->offsetp);
        lp->offsetc -= (OFF_TYPE) it->offsetp;
        lp->itemc -= list_leapnext(prevsl);
        prevsl->offsetn = 0;
    }

    memmove32(it, ((P_U8) it) + size, array_elements(&p_pooldesc->h_pool) - size - itoff);

    /* free the space in the pool */
    al_array_shrink_by(&p_pooldesc->h_pool, - (S32) size);

    return(removed);
}

/******************************************************************************
*
* deallocate pool and remove from pool descriptor block
*
******************************************************************************/

static void
deallocpool(
    _InoutRef_  P_LIST_BLOCK lp,
    _InVal_     S32 poolix)
{
    P_POOLDESC p_pooldesc = pooldesc_ptr(lp, poolix);

    /* the pool is dead, kill the pool */
    al_array_dispose(&p_pooldesc->h_pool);

    /* remove descriptor */
    al_array_delete_at(&lp->h_pooldesc, -1, poolix);

    if(!array_elements(&lp->h_pooldesc))
    {
        al_array_dispose(&lp->h_pooldesc);
        lp->ix_pooldesc = 0;
    }
    else if(lp->ix_pooldesc >= array_elements(&lp->h_pooldesc))
    {
        /* make sure not pointing past the end */
        --lp->ix_pooldesc;
        lp->itemc = pooldesc_ptr(lp, lp->ix_pooldesc)->poolitem;
        lp->offsetc = 0;
    }

    return;
}

/******************************************************************************
*
* create filler item after current position in list
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
fillafter(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO itemfill,
    _In_        LIST_ITEMNO adjust)
{
    P_LIST_ITEM it;

    it = addafter(lp, FILLSIZE, adjust);
    if(!it)
        return(NULL);

    it->fill = 1;
    it->i.itemfill = itemfill;
    return(it);
}

/******************************************************************************
*
* create filler item in list before specified item
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
fillbefore(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO itemfill,
    _In_        LIST_ITEMNO adjust)
{
    P_LIST_ITEM it;

    it = addbefore(lp, FILLSIZE, adjust);
    if(!it)
        return(NULL);

    it->fill = 1;
    it->i.itemfill = itemfill;
    return(it);
}

/******************************************************************************
*
* internal gotoitem
*
* igotoitem only returns a null pointer if there is no item at all in
* the structure; it may not get to the item specified, in which case
* it returns the nearest item BEFORE the one asked for. this may be
* a filler item, of course. this routine is for use by the internal
* structure management only; generally list_gotoitem() is the one to use
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
gotoitem_i(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item)
{
    P_LIST_ITEM it = list_gotoitem_opt(lp, item);

    return((NULL != it) ? it : lp ? convofftoptr(lp, lp->ix_pooldesc, lp->offsetc) : NULL);
}

/******************************************************************************
*
* reallocate memory for a item
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_LIST_ITEM
reallocitem(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        S32 new_size,
    P_LIST_ITEM it,
    _In_        LIST_ITEMNO adjust)
{
    P_POOLDESC p_pooldesc;
    int extra_space; /* SKS 14dec93 - must be signed */
    OFF_TYPE old_size, end_of_data;
    P_U8 p_pool;
    P_LIST_ITEM new_it;
    STATUS status;

    /* check for a delete */
    if(!new_size)
    {
        LIST_ITEMNO removed;
        removed = deallocitem(lp, it);
        updatepoolitems(lp, -removed);
        return(NULL);
    }

    /* add item overhead */
    new_size += LIST_ITEMOVH;

#if SIZEOF_ALIGN_T > 1
    /* round up size if necessary */
    if(new_size & (SIZEOF_ALIGN_T - 1))
    {
        new_size += SIZEOF_ALIGN_T;
        new_size &= ~(SIZEOF_ALIGN_T - 1);
    }
#endif /* SIZEOF_ALIGN_T */

    /* do we have a pool right now ? */
    /* create a pool if we have none */
    if(!lp->h_pooldesc)
    {
        if(status_fail(allocpool(lp, lp->ix_pooldesc, (OFF_TYPE) new_size)))
            return(NULL);
        extra_space = 0;
        old_size = 0;
        end_of_data = 0;
    }
    else
    {
        p_pooldesc = pooldesc_ptr(lp, lp->ix_pooldesc);
        end_of_data = (OFF_TYPE) array_elements(&p_pooldesc->h_pool);

        /* do we have an existing item ? */
        if(it)
        {
            lp->offsetc = (OFF_TYPE) ((PC_BYTE) it - array_ptrc(&p_pooldesc->h_pool, BYTE, 0));

            /* work out old size */
            if(it->offsetn)
                old_size = (OFF_TYPE) it->offsetn;
            else
                old_size = end_of_data - lp->offsetc;
        }
        else
            old_size = 0;

        extra_space = (int) new_size - (int) old_size;

        if(extra_space > 0)
        {
            /* check if we need to split pool */
            if(array_elements(&p_pooldesc->h_pool) + extra_space > MAX_POOL)
            {
                trace_0(TRACE_MODULE_LIST, TEXT("reallocitem - splitting pool"));

                if(status_fail(splitpool(lp, (OFF_TYPE) extra_space, adjust)))
                    return(NULL);

                p_pooldesc = pooldesc_ptr(lp, lp->ix_pooldesc);
                end_of_data = (OFF_TYPE) (array_elements(&p_pooldesc->h_pool) - extra_space);
            }
            else
                /* make extra room in pool */
                if(NULL == al_array_extend_by_U8(&p_pooldesc->h_pool, extra_space, PC_ARRAY_INIT_BLOCK_NONE, &status))
                    return(NULL);
        }
    }

    p_pooldesc = pooldesc_ptr(lp, lp->ix_pooldesc);
    p_pool = array_ptr(&p_pooldesc->h_pool, U8, 0);

    if(old_size)
    {
        /* we are adjusting an existing item */
        OFF_TYPE item_end_old = lp->offsetc + old_size;
        memmove32((p_pool + item_end_old) + extra_space, p_pool + item_end_old, end_of_data - item_end_old);

        /* store new item size in item, & update link in next item */
        new_it = (P_LIST_ITEM) (p_pool + lp->offsetc);
        if(new_it->offsetn)
        {
            new_it->offsetn = (LINK_TYPE) (old_size + extra_space);
            ((P_LIST_ITEM) (((P_U8) new_it) + new_it->offsetn))->offsetp = (LINK_TYPE) (old_size + extra_space);
        }

        /* release extra space */
        if(extra_space < 0)
            al_array_shrink_by(&p_pooldesc->h_pool, extra_space);
    }
    else
    {
        /* we are allocating a new item */
        P_LIST_ITEM nextsl, prevsl;

        /* insert space into pool */
        new_it = (P_LIST_ITEM) (p_pool + lp->offsetc);
        nextsl = (P_LIST_ITEM) (p_pool + lp->offsetc + new_size);

        /* is there an item after this one ? */
        if(end_of_data > lp->offsetc)
        {
            memmove32(nextsl, new_it, end_of_data - lp->offsetc);
            prevsl = NULL;
            if(nextsl->offsetp)
                prevsl = (P_LIST_ITEM) (((P_U8) new_it) - nextsl->offsetp);
            nextsl->offsetp = new_it->offsetn = (LINK_TYPE) new_size;
        }
        else
        {
            new_it->offsetn = 0;
            if(lp->offsetc)
            {
                /* find previous item, painfully, by working down */
                prevsl = (P_LIST_ITEM) p_pool;
                while(prevsl->offsetn)
                    prevsl = (P_LIST_ITEM) (((P_U8) prevsl) + prevsl->offsetn);
            }
            else
                prevsl = NULL;
        }

        /* is there an item before this one ? */
        if(prevsl)
            new_it->offsetp = prevsl->offsetn = PtrDiffBytesU32(new_it, prevsl);
        else
            new_it->offsetp = 0;
    }

    return(new_it);
}

/******************************************************************************
*
* split a memory pool
*
* after split, current pool contains the extra space needed
*
******************************************************************************/

_Check_return_
static STATUS
splitpool(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        OFF_TYPE size_needed,
    _In_        LIST_ITEMNO adjust)
{
    P_U8 newpp;
    S32 justadd, pool2;
    OFF_TYPE new_size;
    P_POOLDESC p_pooldesc, newp_pooldesc;
    P_LIST_ITEM it;
    LIST_ITEMNO poolitem;

    trace_0(TRACE_MODULE_LIST, TEXT("*** splitting pool"));

    p_pooldesc = pooldesc_ptr(lp, lp->ix_pooldesc);

    trace_1(TRACE_MODULE_LIST, TEXT("Existing p_pooldesc: ") PTR_XTFMT, p_pooldesc);

    /* if adding to the end of the pool, don't split, just add a new pool onto the end */
    justadd = (lp->offsetc >= (OFF_TYPE) array_elements(&p_pooldesc->h_pool));

    /* are we adding the item to the end of the pool? */
    if(!justadd)
    {
        /* calculate a split point */
        OFF_TYPE offset, split;

        poolitem = p_pooldesc->poolitem;
        it = array_base(&p_pooldesc->h_pool, LIST_ITEM);
        offset = 0;

        /* try for half the pool */
        split = (OFF_TYPE) (array_size32(&p_pooldesc->h_pool) / 2);

        while(offset < split)
        {
            poolitem += list_leapnext(it);
            offset += (OFF_TYPE) it->offsetn;
            it = (P_LIST_ITEM) (((P_U8) it) + it->offsetn);
            if(adjust && (poolitem > lp->itemc))
            {
                poolitem += adjust;
                adjust = 0;
            }
        }

        trace_2(TRACE_MODULE_LIST, TEXT("offset: ") S32_TFMT TEXT(", split: ") S32_TFMT, (S32) offset, (S32) split);
        new_size = (OFF_TYPE) array_elements(&p_pooldesc->h_pool) - offset;

        /* work out into which pool new item will go */
        if(lp->offsetc >= offset)
        {
            pool2 = 1;
            new_size += size_needed;
        }
        else
            pool2 = 0;
    }
    else
    {
        it = NULL;
        new_size = size_needed;
        pool2 = 1;
        poolitem = lp->itemc;
    }

    /* allocate new pool one past current descriptor position */
    status_return(allocpool(lp, lp->ix_pooldesc + 1, new_size));

    /* get pointer to start of new pool */
    newp_pooldesc = pooldesc_ptr(lp, lp->ix_pooldesc + 1);
    newpp = array_ptr(&newp_pooldesc->h_pool, U8, 0);

    trace_2(TRACE_MODULE_LIST, TEXT("newpp: ") PTR_XTFMT TEXT(", newp_pooldesc: ") PTR_XTFMT, newpp, newp_pooldesc);

    /* re-load pointer */
    p_pooldesc = pooldesc_ptr(lp, lp->ix_pooldesc);

    if(!justadd)
    {
        /* copy second half into new pool */
        P_LIST_ITEM psl = (P_LIST_ITEM) (((P_U8) it) - it->offsetp);
        OFF_TYPE n_bytes;

        psl->offsetn = it->offsetp = 0;
        n_bytes = (OFF_TYPE) (array_elements(&p_pooldesc->h_pool) - PtrDiffBytesS32(it, array_base(&p_pooldesc->h_pool, U8)));

        trace_3(TRACE_MODULE_LIST, TEXT("New p_pool: ") PTR_XTFMT TEXT(", it: ") PTR_XTFMT TEXT(", bytes: ") S32_TFMT, newpp, it, n_bytes);

        memmove32(newpp, it, n_bytes);

        /* shrink giving pool */
        assert(((- (S32) n_bytes) + (pool2 ? 0 : size_needed)) < 0);
        al_array_shrink_by(&p_pooldesc->h_pool, (- (S32) n_bytes) + (pool2 ? 0 : size_needed));

        newp_pooldesc->poolitem = poolitem;

        /* if item in next block, update item offset and pool descriptor */
        if(pool2)
        {
            lp->offsetc -= (OFF_TYPE) array_elements(&p_pooldesc->h_pool);
            ++lp->ix_pooldesc;
        }
    }
    else
    {
        lp->offsetc = 0;
        ++lp->ix_pooldesc;
        newp_pooldesc->poolitem = poolitem;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* update the item counts in pools below the current pool after an insert or a delete
*
******************************************************************************/

static void
updatepoolitems(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO change)
{
    if(lp->h_pooldesc)
    {
        P_POOLDESC p_pooldesc = pooldesc_ptr(lp, lp->ix_pooldesc);
        ARRAY_INDEX i;

        for(i = lp->ix_pooldesc; i < array_elements(&lp->h_pooldesc); ++i, ++p_pooldesc)
            if(p_pooldesc->poolitem > lp->itemc)
                p_pooldesc->poolitem += change;
    }

    if(lp->numitem)
    {
        lp->numitem += change;
        trace_2(TRACE_MODULE_LIST, TEXT("updatepoolitems lp: ") PTR_XTFMT TEXT(", numitem: ") S32_TFMT, lp, (S32) lp->numitem);
    }
}

/* end of list.c */
