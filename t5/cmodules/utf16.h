/* utf16.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2006-2016 Stuart Swales */

#ifndef __utf16_h
#define __utf16_h

/*
UTF-16 functions
*/

_Check_return_
static inline BOOL
WCHAR_is_utf16_high_surrogate(
    _InVal_     WCHAR high_surrogate)
{
    return((high_surrogate >= UCH_SURROGATE_HIGH) && (high_surrogate <= UCH_SURROGATE_HIGH_END));
}

_Check_return_
static inline BOOL
WCHAR_is_utf16_low_surrogate(
    _InVal_     WCHAR low_surrogate)
{
    return((low_surrogate >= UCH_SURROGATE_LOW) && (low_surrogate <= UCH_SURROGATE_LOW_END));
}

_Check_return_
extern UCS4
utf16_char_decode_surrogates(
    _InVal_     WCHAR high_surrogate,
    _InVal_     WCHAR low_surrogate);

extern void
utf16_char_encode_surrogates(
    _OutRef_    PWCH p_high_surrogate,
    _OutRef_    PWCH p_low_surrogate,
    _InVal_     UCS4 ucs4);

_Check_return_
extern U32 /* number of *bytes* (not chars) */
utf16_char_encode(
    _Out_writes_bytes_(bytesof_buffer) P_ANY buffer /*filled*/,
    _InVal_     U32 bytesof_buffer,
    _InVal_     UCS4 ucs4);

_Check_return_
extern U32 /* number of bytes */
utf16_bytes_of_char_encoding(
    _InVal_     UCS4 ucs4);

#endif /* __utf16_h */

/* end of utf16.h */
