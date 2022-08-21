/* utf16.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2006-2020 Stuart Swales */

/* Library module for UTF-16 character handling */

/* SKS Oct 2006 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

#ifndef          __utf16_h
#include "cmodules/utf16.h"
#endif

/******************************************************************************
*
* return number of bytes needed to encode a UCS-4 character to its UTF-16 representation
*
******************************************************************************/

_Check_return_
extern U32 /* number of bytes */
utf16_bytes_of_char_encoding(
    _InVal_     UCS4 ucs4)
{
    assert(status_ok(ucs4_validate(ucs4)));

    if(ucs4 < UCH_UCS2_INVALID)
    {   /* U+0000 : U+FFFF (16 bits) */
        return(2);
    }

    if(ucs4 < UCH_UNICODE_INVALID)
    {   /* U+010000 : U+10FFFF (20 bits after Unicode surrogate furtling) */
        return(4);
    }

    myassert0(TEXT("invalid UCS-4"));
    return(0);
}

_Check_return_
extern UCS4
utf16_char_decode_surrogates(
    _InVal_     WCHAR high_surrogate,
    _InVal_     WCHAR low_surrogate)
{
    U16 high_ten_bits = (U16) (high_surrogate & 0x03FFU);
    U16 low_ten_bits  = (U16) (low_surrogate  & 0x03FFU);
    U32 twenty_bits = ((U32) high_ten_bits << 10) | ((U32) low_ten_bits << 0);
    UCS4 ucs4 = twenty_bits + 0x10000U;
    assert(WCHAR_is_utf16_high_surrogate(high_surrogate));
    assert(WCHAR_is_utf16_low_surrogate(low_surrogate));
    assert(status_ok(ucs4_validate(ucs4)));
    return(ucs4);
}

extern void
utf16_char_encode_surrogates(
    _OutRef_    PWCH p_high_surrogate,
    _OutRef_    PWCH p_low_surrogate,
    _InVal_     UCS4 ucs4)
{
    U32 twenty_bits = ucs4 - 0x10000U;
    U16 high_ten_bits = (U16) ((twenty_bits >> 10) & 0x03FFU);
    U16 low_ten_bits  = (U16) ((twenty_bits >>  0) & 0x03FFU);
    *p_high_surrogate = (WCHAR) (UCH_SURROGATE_HIGH | high_ten_bits);
    *p_low_surrogate  = (WCHAR) (UCH_SURROGATE_LOW  | low_ten_bits);
    assert(status_ok(ucs4_validate(ucs4)));
    assert(WCHAR_is_utf16_high_surrogate(*p_high_surrogate));
    assert(WCHAR_is_utf16_low_surrogate(*p_low_surrogate));
}

/******************************************************************************
*
* encode a UCS-4 character to its native UTF-16 representation in a buffer
*
* buffer is unmolested if bytesof_buffer doesn't fit the entire UTF-16 sequence
*
* call with bytesof_buffer == 0 to count encoding space required
*
******************************************************************************/

_Check_return_
extern U32 /* number of bytes */
utf16_char_encode(
    _Out_writes_bytes_(bytesof_buffer) P_ANY buffer /*filled*/,
    _InVal_     U32 bytesof_buffer,
    _InVal_     UCS4 ucs4)
{
    if(ucs4 < UCH_UCS2_INVALID)
    {   /* U+0000 : U+FFFF (16 bits) */
        WCHAR wchar = (WCHAR) ucs4;

        /* should never be encoding a surrogate character */
        assert(!WCHAR_is_utf16_high_surrogate(wchar));
        assert(!WCHAR_is_utf16_low_surrogate(wchar));
        assert(status_ok(ucs4_validate(ucs4)));

        if(bytesof_buffer >= 2)
        {
            writeval_U16(buffer, wchar);
        }
        return(2);
    }

    assert(status_ok(ucs4_validate(ucs4)));

    if(ucs4 < UCH_UNICODE_INVALID)
    {   /* U+010000 : U+10FFFF (20 bits after Unicode surrogate furtling) */
        if(bytesof_buffer >= 4)
        {   /* character needs to be encoded as a surrogate pair */
            WCHAR high_surrogate, low_surrogate;

            utf16_char_encode_surrogates(&high_surrogate, &low_surrogate, ucs4);

            writeval_U16(buffer, high_surrogate);
            writeval_U16(PtrAddBytes(P_BYTE, buffer, 2), low_surrogate);
        }
        return(4);
    }

    myassert0(TEXT("invalid UCS-4"));
    return(0);
}

/* end of utf16.c */
