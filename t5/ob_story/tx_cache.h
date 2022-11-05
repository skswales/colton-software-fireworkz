/* tx_cache.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Text formatting for Fireworkz */

/* MRJC September 1992 */

/*
structure of a cache entry
*/

typedef struct TEXT_CACHE
{
    DATA_REF data_ref;
    FORMATTED_TEXT formatted_text;
    DOCNO docno;
    U8 used;
    U8 _spare[2];
    UREF_HANDLE uref_handle;
    CLIENT_HANDLE client_handle;
}
TEXT_CACHE, * P_TEXT_CACHE;

/*
exported routines
*/

_Check_return_
_Ret_maybenull_
extern P_TEXT_CACHE
p_text_cache_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_FORMAT_INFO p_text_format_info,
    P_INLINE_OBJECT p_inline_object);

#if TRACE_ALLOWED

extern void
text_cache_size(
    _DocuRef_   P_DOCU p_docu);

#endif

/* end of tx_cache.h */
