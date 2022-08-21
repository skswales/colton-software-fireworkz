/* bins.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MRJC July 1993 */

#ifndef __bins_h
#define __bins_h

typedef struct _binary_tree_links
{
    ARRAY_INDEX link_hi;
    ARRAY_INDEX link_lo;
}
BINARY_TREE_LINKS;

typedef BINARY_TREE_LINKS * P_BINARY_TREE_LINKS;

typedef struct _binary_tree_block
{
    ARRAY_INDEX insert_element;
    S32 use_link_lo;
}
BINARY_TREE_BLOCK;

typedef BINARY_TREE_BLOCK * P_BINARY_TREE_BLOCK;

/*
external functions
*/

extern void
binary_tree_insert(
    P_BINARY_TREE_BLOCK p_binary_tree_block,
    P_ANY p_table,
    S32 entry_size,
    S32 offset_links,
    ARRAY_INDEX new_element);

extern ARRAY_INDEX /* <0 not found, >= 0 element found */
binary_tree_search(
    P_BINARY_TREE_BLOCK p_binary_tree_block,
    P_ANY p_key,
    P_ANY p_table,
    S32 entry_size,
    P_PROC_BSEARCH p_proc_bsearch,
    S32 offset_links);

#endif /* __bins_h */

/* end of bins.h */
