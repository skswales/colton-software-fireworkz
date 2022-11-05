/* bins.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Binary searching routines */

/* MRJC July 1993 */

#include "common/gflags.h"

#include "ob_skel/flags.h"

#include "cmodules/bins.h"

extern ARRAY_INDEX /* <0 not found, >= 0 element found */
binary_tree_search(
    P_BINARY_TREE_BLOCK p_binary_tree_block,
    P_ANY p_key,
    P_ANY p_table,
    S32 entry_size,
    P_PROC_BSEARCH p_proc_bsearch,
    S32 offset_links)
{
    ARRAY_INDEX i = 0, found = -1;

    p_binary_tree_block->insert_element = -1;

    if(p_table)
    {
        do  {
            S32 res;
            P_ANY p_entry;

            p_entry = (P_U8)p_table + i * entry_size;
            if((res = (*p_proc_bsearch)(p_key, p_entry)) == 0)
            {
                found = i;
                break;
            }

            p_binary_tree_block->insert_element = i;

            {
            P_BINARY_TREE_LINKS p_binary_tree_links = (P_ANY) ((P_U8) p_entry + offset_links);

            if(res < 0)
            {
                i = p_binary_tree_links->link_lo;
                p_binary_tree_block->use_link_lo = 1;
            }
            else
            {
                i = p_binary_tree_links->link_hi;
                p_binary_tree_block->use_link_lo = 0;
            }
            } /*block*/

        }
        while(i);
    }

    return(found);
}

extern void
binary_tree_insert(
    P_BINARY_TREE_BLOCK p_binary_tree_block,
    P_ANY p_table,
    S32 entry_size,
    S32 offset_links,
    ARRAY_INDEX new_element)
{
    if(p_binary_tree_block->insert_element >= 0)
    {
        P_BINARY_TREE_LINKS p_binary_tree_links = (P_BINARY_TREE_LINKS) (((P_U8) p_table
                                                                              + p_binary_tree_block->insert_element * entry_size)
                                                                              + offset_links);

        if(p_binary_tree_block->use_link_lo)
            p_binary_tree_links->link_lo = new_element;
        else
            p_binary_tree_links->link_hi = new_element;
    }
}

/* end of bins.c */
