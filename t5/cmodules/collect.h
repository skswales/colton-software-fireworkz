/* collect.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MRJC December 1991 */

#ifndef __collect_h
#define __collect_h

#include "cmodules/list.h"

#define COLLECT_ENUM_START ((LIST_ITEMNO) -1)

/*
exported functions
*/

_Check_return_
_Ret_writes_maybenull_(size)
extern P_BYTE
_collect_add_entry(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _In_reads_bytes_maybenone_(size) PC_ANY p_data,
    _InVal_     S32 size,
    _InVal_     LIST_ITEMNO key,
    _OutRef_    P_STATUS p_status);

#define collect_add_entry_bytes(__base_type, p_list_block, p_data, size, key, p_status) ( \
    (__base_type *) _collect_add_entry(p_list_block, p_data, size, key, p_status) )

#define collect_add_entry_elem(__base_type, p_list_block, p_data, key, p_status) ( \
    (__base_type *) _collect_add_entry(p_list_block, p_data, sizeof32(__base_type), key, p_status) )

extern void
collect_compress(
    _InoutRef_  P_LIST_BLOCK p_list_block);

_Check_return_
extern STATUS
collect_copy(
    _InoutRef_  P_LIST_BLOCK new_p_list_block,
    _InoutRef_  P_LIST_BLOCK old_p_list_block);

extern void
collect_delete(
    _InoutRef_  P_LIST_BLOCK p_list_block);

extern void
collect_delete_entry(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InVal_     LIST_ITEMNO key);

_Check_return_
_Ret_writes_maybenull_(bytesof_elem)
extern P_BYTE
_collect_first(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _OutRef_    P_LIST_ITEMNO p_key
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem));

#define collect_first(__base_type, p_list_block, p_key) ( \
    (__base_type *) _collect_first(p_list_block, p_key \
    CODE_ANALYSIS_ONLY_ARG(sizeof32(__base_type))) )

_Check_return_
_Ret_writes_maybenull_(bytesof_elem)
extern P_BYTE
_collect_first_from(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InoutRef_  P_LIST_ITEMNO p_key
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem));

#define collect_first_from(__base_type, p_list_block, p_key) ( \
    (__base_type *) _collect_first_from(p_list_block, p_key \
    CODE_ANALYSIS_ONLY_ARG(sizeof32(__base_type))) )

#define collect_goto_item(__base_type, p_list_block, p_key) \
    list_gotoitemcontents_opt(__base_type, p_list_block, p_key)

_Check_return_
extern BOOL
collect_has_data(
    _InRef_     P_LIST_BLOCK p_list_block);

#if defined(UNUSED_KEEP_ALIVE)

_Check_return_
_Ret_writes_to_maybenull_(size, 0)
extern P_BYTE
_collect_insert_entry(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InVal_     S32 size,
    _InVal_     LIST_ITEMNO key,
    _OutRef_    P_STATUS p_status);

#define collect_insert_entry_bytes(__base_type, p_list_block, size, key, p_status) ( \
    (__base_type *) _collect_insert_entry(p_list_block, size, key, p_status) )

#define collect_insert_entry_elem(__base_type, p_list_block, key, p_status) ( \
    (__base_type *) _collect_insert_entry(p_list_block, sizeof32(__base_type), key, p_status) )

#endif /* UNUSED_KEEP_ALIVE */

_Check_return_
_Ret_writes_maybenull_(bytesof_elem)
extern P_BYTE
_collect_next(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InoutRef_opt_ P_LIST_ITEMNO p_key
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem));

#define collect_next(__base_type, p_list_block, p_key) ( \
    (__base_type *) _collect_next(p_list_block, p_key \
    CODE_ANALYSIS_ONLY_ARG(sizeof32(__base_type))) )

#if defined(UNUSED_KEEP_ALIVE)

_Check_return_
_Ret_writes_maybenull_(bytesof_elem)
extern P_BYTE
_collect_prev(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InoutRef_  P_LIST_ITEMNO p_key
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem));

#define collect_prev(__base_type, p_list_block, p_key) ( \
    (__base_type *) _collect_prev(p_list_block, p_key \
    CODE_ANALYSIS_ONLY_ARG(sizeof32(__base_type))) )

#endif /* UNUSED_KEEP_ALIVE */

extern void
collect_subtract_entry(
    _InoutRef_  P_LIST_BLOCK p_list_block,
    _InVal_     LIST_ITEMNO key);

#endif /* __collect_h */

/* end of collect.h */
