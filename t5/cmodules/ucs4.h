/* ucs4.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2006-2016 Stuart Swales */

#ifndef __ucs4_h
#define __ucs4_h

/*
UCS-4 functions
*/

_Check_return_
extern STATUS /* STATUS_OK, error if not */
ucs4_validate(
    _InVal_     UCS4 ucs4);

/*
UCD access functions
*/

_Check_return_
extern BOOL
ucs4_is_alphabetic(
    _InVal_     UCS4 ucs4);

_Check_return_
extern BOOL
ucs4_is_decimal_digit(
    _InVal_     UCS4 ucs4);

_Check_return_
extern BOOL
ucs4_is_grapheme_extend(
    _InVal_     UCS4 ucs4);

_Check_return_
extern BOOL
ucs4_is_lowercase(
    _InVal_     UCS4 ucs4);

_Check_return_
extern BOOL
ucs4_is_uppercase(
    _InVal_     UCS4 ucs4);

_Check_return_
extern BOOL
ucs4_is_XID_continue(
    _InVal_     UCS4 ucs4);

_Check_return_
extern BOOL
ucs4_is_XID_start(
    _InVal_     UCS4 ucs4);

_Check_return_
extern UCS4
ucs4_case_fold_simple(
    _InVal_     UCS4 ucs4);

_Check_return_
extern S32
ucs4_decimal_digit_value(
    _InVal_     UCS4 ucs4);

_Check_return_
extern UCS4
ucs4_lowercase(
    _InVal_     UCS4 ucs4);

_Check_return_
extern UCS4
ucs4_uppercase(
    _InVal_     UCS4 ucs4);

/*
UCS-4 character conversion functions
*/

#define SBCHAR_CODEPAGE_WINDOWS_1250    1250U       /* Windows-1250 */
#define SBCHAR_CODEPAGE_WINDOWS_1252    1252U       /* Windows-1252 */
#define SBCHAR_CODEPAGE_WINDOWS_28591   28591U      /* Windows-28591 */ /* ISO 8859-1 */
#define SBCHAR_CODEPAGE_WINDOWS_28592   28592U      /* Windows-28592 */ /* ISO 8859-2 */
#define SBCHAR_CODEPAGE_WINDOWS_28593   28593U      /* Windows-28593 */ /* ISO 8859-3 */
#define SBCHAR_CODEPAGE_WINDOWS_28594   28594U      /* Windows-28594 */ /* ISO 8859-4 */
#define SBCHAR_CODEPAGE_WINDOWS_28605   28605U      /* Windows-28605 */ /* ISO 8859-15 */

#define SBCHAR_CODEPAGE_ALPHABET_LATIN1 0x0A100000U /* Alphabet-Latin1 */
#define SBCHAR_CODEPAGE_ALPHABET_LATIN2 0x0A200000U /* Alphabet-Latin2 */
#define SBCHAR_CODEPAGE_ALPHABET_LATIN3 0x0A300000U /* Alphabet-Latin3 */
#define SBCHAR_CODEPAGE_ALPHABET_LATIN4 0x0A400000U /* Alphabet-Latin4 */
#define SBCHAR_CODEPAGE_ALPHABET_LATIN9 0x0A900000U /* Alphabet-Latin9 */

_Check_return_
extern SBCHAR_CODEPAGE
get_system_codepage(void);

_Check_return_
extern UCS4
ucs4_from_sbchar_with_codepage(
    _InVal_     SBCHAR sbchar,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage);

_Check_return_
extern UCS4
ucs4_to_sbchar_force_with_codepage(
    _InVal_     UCS4 ucs4_in,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage,
    _InVal_     UCS4 ucs4_default);

_Check_return_
extern UCS4
ucs4_to_sbchar_try_with_codepage(
    _InVal_     UCS4 ucs4,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage);

#endif /* __ucs4_h */

/* end of ucs4.h */
