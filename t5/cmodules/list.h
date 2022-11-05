/* list.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Defines the external interface to the list manager */

/* MRJC December 1991 */

#ifndef __list_h
#define __list_h

/******************************************************************************

Maximum size of items and pools is 32k; this can be increased on machines
where the default integer type is more than 2 bytes by altering LINK_TYPE.
This adds extra overhead for every item stored by handlist - the overhead is
now 5 bytes; using 32 bit links would increase the overhead to 9 bytes.
OFF_TYPE must be large enough to hold 1 bit more information than MAX_POOL -
to allow for overflow during calculations and pool splitting

******************************************************************************/

/* define to hold biggest item no. */
typedef S32 LIST_ITEMNO; typedef LIST_ITEMNO * P_LIST_ITEMNO;
#define LIST_ITEMNO_TFMT S32_TFMT

#ifdef SHORT_INT /* Was for 16-bit Windows; left in as example of use */

typedef U16 OFF_TYPE;
typedef UBF LINK_TYPE;
#define MAX_POOL 32767

#else

typedef S32 OFF_TYPE;       /* type for pool sizes and offsets */
typedef UBF LINK_TYPE;      /* type for inter-item links */
#define MAX_POOL 32767      /* maximum size of pool */

#endif /* SHORT_INT */

/*
block increment sizes
*/

#define POOLDBLKSIZEINC 5

/*
structure of entry in pool block array
*/

typedef struct POOLDESC
{
    LIST_ITEMNO poolitem;       /* item number of pool */
    ARRAY_HANDLE h_pool;
}
POOLDESC, * P_POOLDESC;  /* pool descriptor pointer */

typedef struct LIST_BLOCK
{
    OFF_TYPE offsetc;           /* offset to current item */
    LIST_ITEMNO itemc;          /* current item number */

    S32 ix_pooldesc;            /* index to current pool descriptor */
    ARRAY_HANDLE h_pooldesc;    /* handle of pool descriptor array */

    LIST_ITEMNO numitem;        /* number of items in this list */

#ifdef LIST_CACHE
    ARRAY_HANDLE h_list_cache;
#else
    S32 spare;
#endif
}
LIST_BLOCK, * P_LIST_BLOCK;

/* old style compatibility */
#define list_block LIST_BLOCK

typedef union LIST_ITEM_GUTS
{
    LIST_ITEMNO itemfill; /* fill count */
    U8 inside[1];         /* contents of the item */
}
LIST_ITEM_GUTS;

typedef struct LIST_ITEM
{
    UBF fill : 1;
    UBF _spare : 1;
    UBF offsetn : 15; /* offset to next item */
    UBF offsetp : 15; /* offset to previous item */

    LIST_ITEM_GUTS i;
}
LIST_ITEM, * P_LIST_ITEM, ** P_P_LIST_ITEM;

/* overhead per allocated item */
#define LIST_ITEMOVH    offsetof32(LIST_ITEM, i)

/* maximum size of an item */
#define MAX_ITEM        (MAX_POOL - LIST_ITEMOVH - SIZEOF_ALIGN_T)

/*
functions as macros
*/

#define list_atitem(lp) ( \
    (lp) ? (lp)->itemc : 0 )

#define list_itemcontents(__base_type, it) ( \
    (__base_type *) ((it)->i.inside) )

#define list_leapnext(it) ( \
    (it)->fill ? (it)->i.itemfill : 1 )

#define list_numitem(lp) ( \
    (lp) ? (lp)->numitem : 0 )

/*
exported functions
*/

_Check_return_
_Ret_writes_bytes_to_maybenull_(size, 0)
extern P_LIST_ITEM
list_createitem(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item,
    _In_        S32 size,
    _In_        S32 fill);

extern void
list_deleteitems(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item,
    _In_        LIST_ITEMNO numdel);

_Check_return_
extern STATUS
list_ensureitem(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item);

extern S32
list_entsize(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item);

extern void
list_free(
    _InoutRef_  P_LIST_BLOCK lp);

extern S32
list_garbagecollect(
    _InoutRef_  P_LIST_BLOCK lp);

_Check_return_
_Ret_notnull_
extern P_LIST_ITEM
list_gotoitem(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item);

/* return a pointer to the given item's contents */

_Check_return_
_Ret_notnull_
static inline P_ANY
_list_gotoitemcontents(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item)
{
    P_LIST_ITEM it = list_gotoitem(lp, item);

    return(list_itemcontents(void, it));
}

#define list_gotoitemcontents(__base_type, p_list_block, item) ( \
    ((__base_type *) _list_gotoitemcontents(p_list_block, item)) )

_Check_return_
_Ret_maybenull_
extern P_LIST_ITEM
list_gotoitem_opt(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item);

_Check_return_
_Ret_maybenull_
static inline P_ANY
_list_gotoitemcontents_opt(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item)
{
    P_LIST_ITEM it = list_gotoitem_opt(lp, item);

    return(it ? list_itemcontents(void, it) : NULL);
}

#define list_gotoitemcontents_opt(__base_type, p_list_block, item) ( \
    ((__base_type *) _list_gotoitemcontents_opt(p_list_block, item)) )

_Check_return_
extern BOOL
list_has_data(
    _InRef_     P_LIST_BLOCK p_list_block);

extern void
list_init(
    _OutRef_    P_LIST_BLOCK p_list_block);

_Check_return_
extern STATUS
list_init_cache(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _In_        S32 n_items);

_Check_return_
_Ret_maybenull_
extern P_LIST_ITEM
list_initseq(
    _InoutRef_  P_LIST_BLOCK lp,
    _InoutRef_  P_LIST_ITEMNO p_itemno);

_Check_return_
extern STATUS
list_insertitems(
    _InoutRef_  P_LIST_BLOCK lp,
    _In_        LIST_ITEMNO item,
    _In_        LIST_ITEMNO numins);

_Check_return_
_Ret_maybenull_
extern P_LIST_ITEM
list_nextseq(
    _InoutRef_  P_LIST_BLOCK lp,
    _InoutRef_  P_LIST_ITEMNO p_itemno);

_Check_return_
_Ret_maybenull_
extern P_LIST_ITEM
list_prevseq(
    _InoutRef_  P_LIST_BLOCK lp,
    _InoutRef_  P_LIST_ITEMNO p_itemno);

#if TRACE_ALLOWED

extern void
list_size(
    _InRef_     P_LIST_BLOCK p_list_block,
    _OutRef_    P_U32 p_used,
    _OutRef_    P_U32 p_size,
    _OutRef_    P_U32 p_pools);

#endif

#endif /* __list_h */

/* end of list.h */
