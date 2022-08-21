/* collect.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Provides a layer for easy list management on top of list.c */

/* MRJC December 1991 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/collect.h"

/******************************************************************************
*
* add entry to list
* existing entry is overwritten if exists
* use key of -1 to add to end of list
*
* --out--
* = NULL failed to add entry
* <>NULL pointer to new entry
*
******************************************************************************/

_Check_return_
_Ret_writes_maybenull_(size)
extern P_BYTE
_collect_add_entry(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _In_reads_bytes_opt_(size) PC_ANY p_data,
    _InVal_     S32 size,
    _InVal_     LIST_ITEMNO key,
    _OutRef_    P_STATUS p_status)
{
    LIST_ITEMNO item = (key >= 0) ? key : list_numitem(p_list_block);
    P_LIST_ITEM it = NULL;
    P_BYTE p_data_new;

    *p_status = STATUS_OK;

    if(0 == size) /* if empty item, return NULL */
        return(NULL);

    myassert1x(key >= -1, TEXT("collect_add_entry key(") LIST_ITEMNO_TFMT TEXT(") negative and not -1"), key);

    if(NULL == (it = list_createitem(p_list_block, item, size, FALSE)))
    {
        *p_status = status_nomem();
        return(NULL);
    }

    p_data_new = list_itemcontents(BYTE, it);

    if(!IS_P_DATA_NONE(p_data))
        memcpy32(p_data_new, p_data, size);

    return(p_data_new);
}

/******************************************************************************
*
* compress a list, coalescing fillers and removing if empty
*
******************************************************************************/

extern void
collect_compress(
    _InoutRef_  P_LIST_BLOCK p_list_block)
{
    /* ensure any fillers get zapped */
    list_garbagecollect(p_list_block);

    if(0 == list_numitem(p_list_block))
        collect_delete(p_list_block);
}

/******************************************************************************
*
* copy one list into another
*
* --out--
* status indicating success or failure
*
******************************************************************************/

_Check_return_
extern STATUS
collect_copy(
    _InoutRef_  P_LIST_BLOCK new_p_list_block,
    _InoutRef_  P_LIST_BLOCK old_p_list_block)
{
    LIST_ITEMNO key = 0;
    P_BYTE old_entp;

    if(NULL != (old_entp = collect_first(BYTE, old_p_list_block, &key)))
    {
        do  {
            STATUS status;
            S32 entry_size = list_entsize(old_p_list_block, list_atitem(old_p_list_block));
            P_BYTE new_entp = _collect_add_entry(new_p_list_block, old_entp, entry_size, key, &status);

            if(NULL == new_entp)
            {
                collect_delete(new_p_list_block);
                return(status);
            }
        }
        while(NULL != (old_entp = collect_next(BYTE, old_p_list_block, &key)));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* delete a collection altogether and free storage
*
******************************************************************************/

extern void
collect_delete(
    _InoutRef_  P_LIST_BLOCK p_list_block)
{
    list_free(p_list_block);
}

#if defined(UNUSED_KEEP_ALIVE)

/******************************************************************************
*
* delete an item from the list; key numbers greater
* than the given item are decremented by one
* (complementary to collect_insert_entry)
*
******************************************************************************/

extern void
collect_delete_entry(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InVal_     LIST_ITEMNO key)
{
    myassert1x(key >= 0, TEXT("collect_delete_entry key(") LIST_ITEMNO_TFMT TEXT(") negative"), key);

    if(NULL != list_gotoitemcontents_opt(void, p_list_block, key))
        list_deleteitems(p_list_block, key, (LIST_ITEMNO) 1);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* initialise list for sequence and
* return the very first element in the list
*
* --out--
* = NULL nothing in collection
* <>NULL pointer to first entry contents
*
******************************************************************************/

_Check_return_
_Ret_writes_maybenull_(bytesof_elem)
extern P_BYTE
_collect_first(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _OutRef_    P_LIST_ITEMNO p_key
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem))
{
    P_LIST_ITEM it;
    LIST_ITEMNO item = 0;

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(bytesof_elem));

    it = list_initseq(p_list_block, &item);

    if(p_key)
        *p_key = item;

    return(it ? list_itemcontents(BYTE, it) : NULL);
}

/******************************************************************************
*
* initialise list for sequence and
* return the first element in the list
* starting at the given key (may be previous
* element if key is in a hole or off end)
*
* --out--
* = NULL nothing in collection
* <>NULL pointer to first entry contents
*
******************************************************************************/

_Check_return_
_Ret_writes_maybenull_(bytesof_elem)
extern P_BYTE
_collect_first_from(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InoutRef_  P_LIST_ITEMNO p_key
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem))
{
    P_LIST_ITEM it;
    LIST_ITEMNO item = *p_key;

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(bytesof_elem));
    myassert1x(item >= 0, TEXT("collect_first key ") LIST_ITEMNO_TFMT TEXT(" negative"), item);

    it = list_initseq(p_list_block, &item);

    *p_key = item;

    return(it ? list_itemcontents(BYTE, it) : NULL);
}

_Check_return_
extern BOOL
collect_has_data(
    _InRef_     P_LIST_BLOCK p_list_block)
{
    return(list_has_data(p_list_block));
}

#if defined(UNUSED_KEEP_ALIVE)

/******************************************************************************
*
* insert an entry in the list at the given item number
* items with a keynumber greater than the given key
* are incremented by one
*
* --out--
* = NULL failed to insert entry
* <>NULL pointer to contents of new entry
*
******************************************************************************/

_Check_return_
_Ret_writes_to_maybenull_(size, 0)
extern P_BYTE
_collect_insert_entry(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InVal_     S32 size,
    _InVal_     LIST_ITEMNO key,
    _OutRef_    P_STATUS p_status)
{
    P_LIST_ITEM it = NULL;

    *p_status = STATUS_OK;

    if(0 == size)
        return(NULL);

    myassert1x(key >= 0, TEXT("collect_insert_entry key(") LIST_ITEMNO_TFMT TEXT(") negative"), key);

    trace_1(TRACE_MODULE_ALLOC,
            TEXT("collect_insert_entry key: ") S32_TFMT,
            key);

    /* insert an item at this position if not off end of list */
    if(key < list_numitem(p_list_block))
        if(status_fail(*p_status = list_insertitems(p_list_block, key, 1)))
            return(NULL);

    /* create an item at the given position */
    if(NULL == (it = list_createitem(p_list_block, key, size, FALSE)))
    {
        *p_status = status_nomem();
        return(NULL);
    }

    return(list_itemcontents(BYTE, it));
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* return the next element in the list
*
* --out--
* = NULL nothing more in collection
* <>NULL pointer to next entry
*
******************************************************************************/

_Check_return_
_Ret_writes_maybenull_(bytesof_elem)
extern P_BYTE
_collect_next(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InoutRef_opt_ P_LIST_ITEMNO p_key
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem))
{
    P_LIST_ITEM it;
    LIST_ITEMNO item;

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(bytesof_elem));

    if(p_key)
    {
        item = *p_key;
        myassert1x(item >= 0, TEXT("collect_next key ") LIST_ITEMNO_TFMT TEXT(" negative"), item);
    }
    else
        item = list_atitem(p_list_block);

    it = list_nextseq(p_list_block, &item);

    if(p_key)
        *p_key = item;

    return(it ? list_itemcontents(BYTE, it) : NULL);
}

#if defined(UNUSED_KEEP_ALIVE)

/******************************************************************************
*
* return the previous element in the list
*
* --out--
* = NULL nothing more in collection
* <>NULL pointer to next entry
*
******************************************************************************/

_Check_return_
_Ret_writes_maybenull_(bytesof_elem)
extern P_BYTE
_collect_prev(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InoutRef_  P_LIST_ITEMNO p_key
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem))
{
    P_LIST_ITEM it;
    LIST_ITEMNO item = *p_key;

    CODE_ANALYSIS_ONLY(UNREFERENCED_PARAMETER_InVal_(bytesof_elem));
    myassert1x(item >= 0, TEXT("collect_prev key ") LIST_ITEMNO_TFMT TEXT(" negative"), item);

    it = list_prevseq(p_list_block, &item);

    *p_key = item;

    return(it ? list_itemcontents(BYTE, it) : NULL);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* remove an item from the list; items with key numbers
* greater than the removed item remain unaffected
* (complementary to collect_add_entry)
*
******************************************************************************/

extern void
collect_subtract_entry(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InVal_     LIST_ITEMNO key)
{
    myassert1x(key >= 0, TEXT("collect_subtract_entry key(") LIST_ITEMNO_TFMT TEXT(") negative"), key);

    /* create filler entry */
    if(NULL == list_createitem(p_list_block, key, 0, FALSE))
        assert("failed to create filler");
}

/* end of collect.c */
