/* coltsoft.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Definition of standard types and source rules */

/* MRJC September 1989 / December 1991 */

#ifndef __coltsoft_h
#define __coltsoft_h

#if RISCOS

/*
Stuff defined on Windows
*/

/*
"crtdefs.h"
*/

#define errno_t int

#ifndef EINVAL
#define EINVAL -1
#endif

/*
"stdio.h"
*/

#define sscanf_s sscanf /* This is OK for non-%s, %c - stop winges about deprecated function */

/*
"windef.h"
*/

typedef /*unsigned*/ char  BYTE; /* NB char IS unsigned on Norcroft - stop winges about pointer type diffs */
typedef unsigned short WORD;
typedef unsigned long  DWORD;

typedef int BOOL;
typedef unsigned int UINT;

#define LOBYTE(w)   ((BYTE) ( ((U32)(w))       & 0xFF))
#define HIBYTE(w)   ((BYTE) ((((U32)(w)) >> 8) & 0xFF))

/*
"winnt.h"
*/

typedef void * HANDLE;
typedef void * HGLOBAL;

typedef unsigned short WCHAR; /* 16-bit UTF-16 character (NOT wchar_t) */
typedef         WCHAR *    PWCH;
typedef        PWCH   *  P_PWCH;
typedef   const WCHAR *   PCWCH;
typedef       PCWCH   * P_PCWCH;

/* CH_NULL-terminated variants */
#define         WCHARZ       WCHAR
typedef         WCHAR  *    PWSTR;
typedef        PWSTR   *  P_PWSTR;
typedef   const WCHAR  *   PCWSTR;
typedef       PCWSTR   * P_PCWSTR;

#define __TEXT(s) s /* No wide character constants */
#define TEXT(s) __TEXT(s)

/*
"tchar.h"
*/

#define     TCHAR       U8
#define    PTCH       P_U8
#define   PCTCH      PC_U8
#define P_PCTCH    P_PC_U8

/* CH_NULL-terminated variants */

#define     TCHARZ      U8Z
#define    PTSTR      P_U8Z
#define  P_PTSTR    P_P_U8Z
#define   PCTSTR     PC_U8Z
#define P_PCTSTR   P_PC_U8Z

#endif /* RISCOS */

#define PTSTR_NONE _P_DATA_NONE(PTSTR)

#define tstr_empty_string TEXT("")

/*
Never use a native C type unless dealing with a native API
Where you might have used an int, use an S32

Never use a '*' in a variable definition/declaration;
'*' is OK in two places: typedefs and pointer dereferences

All types are upper case

Structures are defined thus:

    typedef struct FRED
    {
    }
    FRED;

and the 'struct' word should only rarely appear outside a typedef

Pointer types are derived by prefixing P_ to base type:

    typedef FRED * P_FRED;

or where it is a pointer to const data, PC_:

    typedef const FRED * PC_FRED;

Where possible, instances of a type should be lower case versions of the type name:

    P_FRED p_fred;

When you define a constant for the size of a buffer in bytes
e.g. STRING_MAX, always define two constants, the second being
larger by 1 and prefixed with BUF_:

#define STRING_MAX 10
#define BUF_STRING_MAX 11

Where the parameters of a function include a source and a destination,
the destination should appear first in the parameter list,
as it does for example xstrkpy(dst, elemof_dst, src)

When you have start and end pointers / indexes they must be
inclusive, exclusive whether they point to memory, cells, arrays or anything
*/

/*
Our additions to SAL
*/

/*
Most non-trivial pointer args here (i.e. P_XXX or PC_XXX) are really references
to a single instance of an 'object' so annotate as such -
the callee function shouldn't be modifying the pointer,
just referencing/setting/updating the contents referred to
*/

#define _InRef_                 _In_                const
#define _InRef_opt_             _In_opt_            const
#define _InRef_maybenone_       _In_maybenone_      const

#define _InoutRef_              _Inout_             const
#define _InoutRef_opt_          _Inout_opt_         const
#define _InoutRef_maybenone_    _Inout_maybenone_   const

#define _OutRef_                _Out_               const
#define _OutRef_opt_            _Out_opt_           const
#define _OutRef_maybenone_      _Out_maybenone_     const

/*
Simple, or derived, type value args that the callee function shouldn't be modifying
*/

#define _InVal_ /*_In_*/ const

/*
void
*/

typedef void * P_ANY, ** P_P_ANY; typedef const void * PC_ANY;
#if defined(__cplusplus) || defined(__CC_NORCROFT) || defined(__GNUC__) || defined(__clang__)
#define P_P_ANY_PEDANTIC(pp) ((P_P_ANY) (pp)) /* needs cast */
#else
#define P_P_ANY_PEDANTIC(pp) (pp)
#endif

#define    P_DATA_NONE       _P_DATA_NONE(P_ANY)
#define IS_P_DATA_NONE(p) _IS_P_DATA_NONE(PC_ANY, p)

/*
BYTE
*/

typedef BYTE * P_BYTE, ** P_P_BYTE; typedef const BYTE * PC_BYTE;

#define P_BYTE_NONE _P_DATA_NONE(P_BYTE)

/*
U8 char
*/

#if !defined(__CHAR_UNSIGNED__)
#error       __CHAR_UNSIGNED__ is not defined
typedef unsigned char    U8;
#else
typedef          char    U8;
#endif
typedef U8 * P_U8, ** P_P_U8; typedef const U8 * PC_U8; typedef PC_U8 * P_PC_U8;

#define P_U8_NONE _P_DATA_NONE(P_U8)

/* Z suffix signifying these to be known to be CH_NULL-terminated */

#if WINDOWS && 1 /* Distinct (but convertible) where possible for usage clarity and Code Analysis and IntelliSense hover */
typedef       U8         U8Z;
typedef       U8Z *    P_U8Z; /*_Null_terminated_*/
typedef       U8Z ** P_P_U8Z; /*_Null_terminated_*/
typedef const U8Z *   PC_U8Z; /*_Null_terminated_*/
typedef    PC_U8Z * P_PC_U8Z;
#elif 1 /* Distinct (but convertible) where possible for usage clarity and IntelliSense hover */
typedef      U8      U8Z;
typedef    P_U8    P_U8Z; /*_Null_terminated_*/
typedef  P_P_U8  P_P_U8Z;
typedef   PC_U8   PC_U8Z; /*_Null_terminated_*/
typedef P_PC_U8 P_PC_U8Z;
#else
#define      U8Z      U8
#define    P_U8Z    P_U8 /*_Null_terminated_*/
#define  P_P_U8Z  P_P_U8
#define   PC_U8Z   PC_U8 /*_Null_terminated_*/
#define P_PC_U8Z P_PC_U8
#endif

#define P_U8Z_NONE _P_DATA_NONE(P_U8Z)

#define empty_string ""

#define u8_is_ascii7(u8) ( \
    (u8) < 0x80)

typedef   int8_t  S8; typedef S8 * P_S8, ** P_P_S8;

typedef  int16_t S16; typedef S16 * P_S16, ** P_P_S16; typedef const S16 * PC_S16;

typedef uint16_t U16; typedef U16 * P_U16, ** P_P_U16; typedef const U16 * PC_U16;

typedef  int32_t S32; typedef S32 * P_S32, ** P_P_S32; typedef const S32 * PC_S32;

typedef uint32_t U32; typedef U32 * P_U32, ** P_P_U32; typedef const U32 * PC_U32;

#define P_S32_NONE _P_DATA_NONE(P_S32)

typedef  int64_t S64; typedef S64 * P_S64, ** P_P_S64; typedef const S64 * PC_S64;

typedef uint64_t U64; typedef U64 * P_U64, ** P_P_U64; typedef const U64 * PC_U64;

typedef double   F64; typedef F64 * P_F64, ** P_P_F64; typedef const F64 * PC_F64;

typedef BOOL * P_BOOL; typedef const BOOL * PC_BOOL;

typedef uintptr_t CLIENT_HANDLE; typedef CLIENT_HANDLE * P_CLIENT_HANDLE;

/*
a UCS-4 32-bit character (ISO 10646-1)
*/

typedef          U32      UCS4;
typedef         UCS4 *  P_UCS4;
typedef   const UCS4 * PC_UCS4;

#if defined(__CC_NORCROFT) /* these are better done as macros on Norcroft */

#define ucs4_is_ascii7(ucs4) ( \
    /*(UCS4)*/ (ucs4) < (UCS4) 0x00000080U )

#define ucs4_is_sbchar(ucs4) (\
    /*(UCS4)*/ (ucs4) < (UCS4) 0x00000100U )

#else /* COMPILER */

_Check_return_
static inline BOOL
ucs4_is_ascii7(
    _InVal_     UCS4 ucs4)
{
    return((ucs4) < 0x00000080U);
}

_Check_return_
static inline BOOL
ucs4_is_sbchar(
    _InVal_     UCS4 ucs4)
{
    return((ucs4) < 0x00000100U);
}

#endif /* COMPILER */

_Check_return_
static inline BOOL
ucs4_is_C1(
    _InVal_     UCS4 ucs4)
{
    return(((ucs4) >= 0x00000080U) && ((ucs4) <= 0x0000009FU));
}

/*
A7CHAR: an ASCII 7-bit character (subset of SBCHAR where top bit is zero)
Useful for things that we know are limited e.g. spreadsheet TYPE()
and key names that go in files for mapping. Also RTF files.
*/

#if 1 /* Distinct (but convertible) where possible for usage clarity and IntelliSense hover */
typedef      U8       A7CHAR;
typedef    P_U8     P_A7CHARS;
typedef  P_P_U8   P_P_A7CHARS;
typedef   PC_U8    PC_A7CHARS;
typedef P_PC_U8  P_PC_A7CHARS;
#else
#define      A7CHAR       U8
#define    P_A7CHARS    P_U8
#define  P_P_A7CHARS  P_P_U8
#define   PC_A7CHARS   PC_U8
#define P_PC_A7CHARS P_PC_U8
#endif

#if WINDOWS && 1 /* Distinct (but convertible) where possible for usage clarity and Code Analysis and IntelliSense hover */
typedef       U8Z            A7CHARZ;
typedef       A7CHARZ *    P_A7STR; /*_Null_terminated_*/
typedef       A7CHARZ ** P_P_A7STR; /*_Null_terminated_*/
typedef const A7CHARZ *   PC_A7STR; /*_Null_terminated_*/
typedef    PC_A7STR   * P_PC_A7STR;
#elif 1 /* Distinct (but convertible) where possible for usage clarity and IntelliSense hover */
typedef      U8Z      A7CHARZ;
typedef    P_U8Z    P_A7STR; /*_Null_terminated_*/
typedef  P_P_U8Z  P_P_A7STR;
typedef   PC_U8Z   PC_A7STR; /*_Null_terminated_*/
typedef P_PC_U8Z P_PC_A7STR;
#else
#define      A7CHARZ      U8Z
#define    P_A7STR      P_U8Z /*_Null_terminated_*/
#define  P_P_A7STR    P_P_U8Z
#define   PC_A7STR     PC_U8Z /*_Null_terminated_*/
#define P_PC_A7STR   P_PC_U8Z
#endif

/*
SBCHAR: a Latin-N 8-bit Single Byte character (may have top-bit-set)
ISO 8859-N / Acorn Extended Latin-N / Windows-<AnsiCodePage>
*/

#if 1 /* Distinct (but convertible) where possible for usage clarity and IntelliSense hover */
typedef      U8       SBCHAR;
typedef    P_U8     P_SBCHARS;
typedef  P_P_U8   P_P_SBCHARS;
typedef   PC_U8    PC_SBCHARS;
typedef P_PC_U8  P_PC_SBCHARS;
#else
#define      SBCHAR       U8
#define    P_SBCHARS    P_U8
#define  P_P_SBCHARS  P_P_U8
#define   PC_SBCHARS   PC_U8
#define P_PC_SBCHARS P_PC_U8
#endif

#if WINDOWS && 1 /* Distinct (but convertible) where possible for usage clarity and Code Analysis and IntelliSense hover */
typedef       U8Z            SBCHARZ;
typedef       SBCHARZ *    P_SBSTR; /*_Null_terminated_*/
typedef       SBCHARZ ** P_P_SBSTR; /*_Null_terminated_*/
typedef const SBCHARZ *   PC_SBSTR; /*_Null_terminated_*/
typedef    PC_SBSTR   * P_PC_SBSTR;
#elif 1 /* Distinct (but convertible) where possible for usage clarity and IntelliSense hover */
typedef      U8Z      SBCHARZ;
typedef    P_U8Z    P_SBSTR; /*_Null_terminated_*/
typedef  P_P_U8Z  P_P_SBSTR;
typedef   PC_U8Z   PC_SBSTR; /*_Null_terminated_*/
typedef P_PC_U8Z P_PC_SBSTR;
#else
#define      SBCHARZ      U8Z
#define    P_SBSTR      P_U8Z /*_Null_terminated_*/
#define  P_P_SBSTR    P_P_U8Z
#define   PC_SBSTR     PC_U8Z /*_Null_terminated_*/
#define P_PC_SBSTR   P_PC_U8Z
#endif

#define SBSTR_TEXT(text) ((PC_SBSTR) (text)) /* akin to the TEXT() macro */

typedef U32 SBCHAR_CODEPAGE; /* for translation between code pages */

/*
RISC OS non-Unicode build
TCHAR/TSTR is Latin-N
UCHARS/USTR is the same
*/

/*
RISC OS Unicode build
TCHAR/TSTR is still Latin-N
UCHARS/USTR is still Latin-N and should become UTF-8
*/

/*
Windows non-Unicode build
TCHAR/TSTR is Latin-N (Windows-1252 usually)
UCHARS/USTR is the same
*/

/*
Windows Unicode build
TCHAR/TSTR is WCHAR (UTF-16)
UCHARS/USTR is still Latin-N and should become UTF-8
*/

#if APP_UNICODE

#if 0

/* UTF-8 encoding is used in USTR/UCHAR */
#define USTR_IS_SBSTR 0
/* TCHAR is WCHAR (UTF-16, was simply UCS-2 on NT 4.0) */
#define TSTR_IS_SBSTR 0

#else

/* Do NOT use UTF-8 encoding in USTR/UCHAR - alias USTR/UCHAR as SBSTR/SBCHAR U8 Latin-N throughout */
#define USTR_IS_SBSTR 1
/* But TCHAR is WCHAR (UTF-16, was simply UCS-2 on NT 4.0) */
#define TSTR_IS_SBSTR 0

#endif

#else /* NOT APP_UNICODE */

/* Do NOT use UTF-8 encoding in USTR/UCHAR - alias USTR/UCHAR as SBSTR/SBCHAR U8 Latin-N throughout */
#define USTR_IS_SBSTR 1
/* Also TSTR/TCHAR is SBSTR/SBCHAR U8 Latin-N */
#define TSTR_IS_SBSTR 1

#endif /* APP_UNICODE */

/*
UTF-8 is a sequence of bytes encoding a UCS-4 32-bit character
(ISO 10646-1:2000 Annex D, also described in RFC 3629,
also section 3.9 of the Unicode 4.0 standard).
See http://www.cl.cam.ac.uk/~mgk25/unicode.html
*/

#if ((!RELEASED && 1) || defined(CODE_ANALYSIS))
#define STRUCT_UTF8_IS_UTF8B 1 /* NB this is really useful for debugging (and is needed for Code Analysis) */
#endif

typedef U8 UTF8B; /* this is needed to make arrays of them on the stack */

#if defined(STRUCT_UTF8_IS_UTF8B)
#define _UTF8 UTF8B
#else /* NOT STRUCT_UTF8_IS_U8 */
/* make it so that we cannot dereference / obtain size of pointer to UTF8 */
#define _UTF8 struct __UTF8
#endif /* STRUCT_UTF8_IS_U8 */

typedef       _UTF8 *    P_UTF8;
typedef      P_UTF8 *  P_P_UTF8;
typedef const _UTF8 *   PC_UTF8;
typedef     PC_UTF8 * P_PC_UTF8;

#define P_UTF8_NONE _P_DATA_NONE(P_UTF8)

#if defined(STRUCT_UTF8_IS_UTF8B)

/* ease single-step debugging */
#define utf8_bptr(buf)    buf

#else

/* using inline function can spot any bad buffer-to-pointer casts that would otherwise happen silently */
_Check_return_
_Ret_notnull_
static __forceinline P_UTF8
utf8_bptr(UTF8B buf[])
{
    return((P_UTF8) buf);
}

#endif /* STRUCT_UTF8_IS_UTF8B */

/*
UTF8STR: signifying that these are known to be CH_NULL-terminated
*/

#define _UTF8STR _UTF8

typedef       _UTF8STR *    P_UTF8STR; /*_Null_terminated_*/
typedef       _UTF8STR ** P_P_UTF8STR; /*_Null_terminated_*/
typedef const _UTF8STR *   PC_UTF8STR; /*_Null_terminated_*/
typedef     PC_UTF8STR * P_PC_UTF8STR;

#define P_UTF8STR_NONE _P_DATA_NONE(P_UTF8STR)

typedef U8Z UTF8STRB; /* this is needed to make arrays of them on the stack */

#if defined(STRUCT_UTF8_IS_UTF8B)

/* ease single-step debugging */
#define utf8str_bptr(buf)    buf
#define utf8str_bptrc(buf)   buf

#else

/* using inline function can spot any bad buffer-to-pointer casts that would otherwise happen silently */
_Check_return_
_Ret_notnull_
static __forceinline P_UTF8STR
utf8str_bptr(UTF8STRB buf[])
{
    return((P_UTF8STR) buf);
}

_Check_return_
_Ret_notnull_
static __forceinline PC_UTF8STR
utf8str_bptrc(const UTF8STRB buf[])
{
    return((PC_UTF8STR) buf);
}

#endif /* STRUCT_UTF8_IS_UTF8B */

#define UTF8STR_TEXT(text) ((PC_UTF8STR) (text)) /* akin to the TEXT() macro */

/*
UCHARS/USTR
*/

#if USTR_IS_SBSTR
#define _UCHARS  U8
#define _UCHARZ  U8Z
#else
#define _UCHARS _UTF8
#define _UCHARZ _UTF8STR
#endif /* USTR_IS_SBSTR */

typedef       _UCHARS *    P_UCHARS;
typedef const _UCHARS *   PC_UCHARS;
typedef      P_UCHARS *  P_P_UCHARS;
typedef     PC_UCHARS * P_PC_UCHARS;

#define P_UCHARS_NONE _P_DATA_NONE(P_UCHARS)

typedef       _UCHARZ *    P_USTR; /*_Null_terminated_*/
typedef const _UCHARZ *   PC_USTR; /*_Null_terminated_*/
typedef       _UCHARZ ** P_P_USTR; /*_Null_terminated_*/
typedef     PC_USTR   * P_PC_USTR;

#define P_USTR_NONE _P_DATA_NONE(P_USTR)

/* this is needed to make arrays of them on the stack */
#if USTR_IS_SBSTR
typedef U8       UCHARB;
typedef U8Z      UCHARZ;
#else
typedef UTF8B    UCHARB;
typedef UTF8STRB UCHARZ;
#endif /* USTR_IS_SBSTR */

#if !((RELEASED && 0) || defined(CODE_ANALYSIS))

#if USTR_IS_SBSTR
/* ease single-step debugging */
#define uchars_bptr(buf)    buf
#define ustr_bptr(buf)      buf
#define ustr_bptrc(buf)     buf
#else
#define uchars_bptr(buf)    utf8_bptr(buf)
#define ustr_bptr(buf)      utf8str_bptr(buf)
#define ustr_bptrc(buf)     utf8str_bptrc(buf)
#endif

#else

/* using inline function can spot any bad buffer-to-pointer casts that would otherwise happen silently */
_Check_return_
_Ret_notnull_
static __forceinline P_UCHARS
uchars_bptr(UCHARB buf[])
{
    return((P_UCHARS) buf);
}

_Check_return_
_Ret_notnull_
static __forceinline P_USTR
ustr_bptr(UCHARZ buf[])
{
    return((P_USTR) buf);
}

_Check_return_
_Ret_notnull_
static __forceinline PC_USTR
ustr_bptrc(const UCHARZ buf[])
{
    return((PC_USTR) buf);
}

#endif /* RELEASED */

#define uchars_empty_string ((PC_UCHARS) "")

#define USTR_TEXT(text) ((PC_USTR) (text)) /* akin to the TEXT() macro - sadly can't concatenate them */

#define ustr_empty_string USTR_TEXT("")

/*
printf format strings
*/

#define  U8_FMT         "%c"
#define  S8_FMT         "%c"
#define S16_FMT        "%hd"
#define U16_FMT        "%hu"
#define U16_XFMT       "%hX"
#define INT_FMT         "%d"
#define UINT_FMT        "%u"
#define S32_FMT         "%d"
#define S32_FMT_POSTFIX  "d"
#define U32_FMT         "%u"
#define U32_XFMT        "0x%X"
#define S64_FMT         "%" PRId64
#define U64_FMT         "%" PRIu64
#define U64_XFMT        "%" PRIx64
#define F64_FMT         "%g"
#define PTR_FMT         "%p"

#define  U8_TFMT         TEXT("%c")
#define  S8_TFMT         TEXT("%c")
#define S16_TFMT        TEXT("%hd")
#define U16_TFMT        TEXT("%hu")
#define U16_XTFMT     TEXT("0x%hX")
#define INT_TFMT         TEXT("%d")
#define UINT_TFMT        TEXT("%u")
#define UINT_XTFMT     TEXT("0x%X")
#define S32_TFMT         TEXT("%d")
#define S32_TFMT_POSTFIX  TEXT("d")
#define U32_TFMT         TEXT("%u")
#define U32_XTFMT      TEXT("0x%X")
#if defined(_MSC_VER)
#define S64_TFMT         TEXT("%lld")
#define U64_TFMT         TEXT("%llu")
#define U64_XTFMT      TEXT("0x%llx")
#else
#define S64_TFMT         TEXT("%") PRId64
#define U64_TFMT         TEXT("%") PRIu64
#define U64_XTFMT      TEXT("0x%") PRIx64
#endif /* _MSC_VER */
#define F64_TFMT         TEXT("%g")
#define PTR_TFMT         TEXT("%p")
#define PTR_XTFMT      TEXT("0x%p")

#define ENUM_XTFMT      U32_XTFMT

#define DWORD_TFMT       TEXT("%lu")
#define DWORD_XTFMT    TEXT("0x%lX")

#if defined(_WIN64)
#define UINT3264_TFMT   U64_TFMT
#define UINT3264_XTFMT  U64_XTFMT
#else
#define UINT3264_TFMT   U32_TFMT
#define UINT3264_XTFMT  U32_XTFMT
#endif /* _WIN64 */

#define UINTPTR_TFMT    UINT3264_TFMT
#define UINTPTR_XTFMT   UINT3264_XTFMT

/*
type limits
*/

#define  U8_MAX   UCHAR_MAX
#define  S8_MAX   SCHAR_MAX
#define U16_MAX   USHRT_MAX
#define S16_MAX    SHRT_MAX
#define U32_MAX    UINT_MAX
#define S32_MAX     INT_MAX
#define F64_MAX     DBL_MAX

#define  S8_MIN   SCHAR_MIN
#define S16_MIN    SHRT_MIN
#define S32_MIN     INT_MIN /* but you might not want to use this! */
#define F64_MIN     DBL_MIN

/*
buffer sizes for printf conversions
*/

#define BUF_MAX_U8_FMT  (1+ 3)  /* 255*/
#define BUF_MAX_S8_FMT  (1+ 4)  /*-128*/
#define BUF_MAX_S16_FMT (1+ 6)  /*-32768*/
#define BUF_MAX_U16_FMT (1+ 5)  /* 65535*/
#define BUF_MAX_S32_FMT (1+ 11) /*-2147483648*/
#define BUF_MAX_U32_FMT (1+ 10) /* 4294967296*/
#define BUF_MAX_F64_FMT (1+ 1 + 1 + 1 + 15 + 1 + 3 + 4) /*approx e.g. -1.2345678901234566e-128, 4 for good luck */

/* stringize(THING_MAYBE_DEFINED_AS_MACRO) may be used to extract its value as a string at preprocessor time that can then be concatenated e.g. "%." stringize(DBL_DECIMAL_DIG) "g" */
#define stringize_helper(value) #value
#define stringize(value) stringize_helper(value)

/* get byte from pointer (plus offset) / put byte at pointer (plus offset) */

#define PtrGetByte(ptr) /*byte*/ \
    PtrGetByteOff(ptr, 0)

#define PtrGetByteOff(ptr, off) /*byte*/ \
    (((PC_BYTE) (ptr))[off])

#define PtrPutByte(ptr, b) /*void*/ \
    PtrPutByteOff(ptr, 0, b)

#define PtrPutByteOff(ptr, off, b) /*void*/ \
    (((P_BYTE) (ptr))[off]) = (b)

/* increment / decrement pointer by one bytes (or N bytes) */

#define PtrIncByte(__ptr_type, ptr__ref) \
    PtrIncBytes(__ptr_type, ptr__ref, 1)

#define PtrIncBytes(__ptr_type, ptr__ref, add) \
    (ptr__ref) = ((__ptr_type) (((uintptr_t)(ptr__ref)) + (add)))

#define PtrDecByte(__ptr_type, ptr__ref) \
    PtrDecBytes(__ptr_type, ptr__ref, 1)

#define PtrDecBytes(__ptr_type, ptr__ref, sub) \
    (ptr__ref) = ((__ptr_type) (((uintptr_t)(ptr__ref)) - (sub)))

/* add bytes to pointer / subtract bytes from pointer */

#define PtrAddBytes(__ptr_type, ptr, add) /*ptr*/ \
    ((__ptr_type) (((uintptr_t)(ptr)) + (add)))

#define PtrSubBytes(__ptr_type, ptr, sub) /*ptr*/ \
    ((__ptr_type) (((uintptr_t)(ptr)) - (sub)))

/* 32-bit difference between two pointers in bytes */

#define PtrDiffBytesU32(ptr, base) /*U32 num*/ \
    ((U32) (((uintptr_t)(ptr)) - ((uintptr_t)(base))))

#define PtrDiffBytesS32(ptr, base) /*S32 num*/ \
    ((S32) (((uintptr_t)(ptr)) - ((uintptr_t)(base))))

/* 32-bit difference between two pointers in elements */

#define PtrDiffElemU32(ptr, base) /*U32 num*/ \
    ((U32) ((ptr) - (base)))

#define PtrDiffElemS32(ptr, base) /*S32 num*/ \
    ((S32) ((ptr) - (base)))

/* size_t difference between two pointers in elements */

#define PtrDiffElem(ptr, base) /*size_t num*/ \
    ((size_t) ((ptr) - (base)))

/*
remove const from pointer
*/

#if defined(__cplusplus)
#define de_const_cast(__type, __expr) (const_cast < __type > ( __expr ))
#else
#define de_const_cast(__type, __expr) (( /*de-const*/ __type ) ( __expr ))
#endif

/*
bit field qualifiers
*/

#define UBF  unsigned int
#define SBF    signed int

#define UBF_PACK(value) \
    ((UBF) (value))

#define UBF_UNPACK(__unpacked_type, packed_value) \
    ((__unpacked_type) (packed_value))

#define SBF_PACK(value) \
    ((SBF) (value))

#define SBF_UNPACK(__unpacked_type, packed_value) \
    ((__unpacked_type) (packed_value))

/*
pointers to procedures are derived by adding P_PROC_
*/

/*
atexit()
*/

typedef void (__cdecl * P_PROC_ATEXIT) (void);

#define PROC_ATEXIT_PROTO(_e_s, _proc_atexit) \
_e_s void __cdecl _proc_atexit(void)

/*
bsearch() / bfind()
*/

typedef /*_Check_return_*/ int (__cdecl * P_PROC_BSEARCH) (
    _Pre_valid_ const void * _key,
    _Pre_valid_ const void * _datum);

#define PROC_BSEARCH_PROTO(_e_s, _proc_bsearch, __key_base_type, __datum_base_type) \
_Check_return_ \
_e_s int __cdecl _proc_bsearch( \
    _In_reads_bytes_c_(sizeof(__key_base_type))   const void * _key, \
    _In_reads_bytes_c_(sizeof(__datum_base_type)) const void * _datum)

#define BSEARCH_KEY(__key_ptr_type) ((__key_ptr_type) \
    _key)

#define BSEARCH_KEY_VAR_DECL(__key_ptr_type, __var_name) \
    const __key_ptr_type __var_name = BSEARCH_KEY(__key_ptr_type)

#define BSEARCH_DATUM(__datum_ptr_type) ((__datum_ptr_type) \
    _datum)

#define BSEARCH_DATUM_VAR_DECL(__datum_ptr_type, __var_name) \
    const __datum_ptr_type __var_name = BSEARCH_DATUM(__datum_ptr_type)

/* for CH_NULL-terminated string lookup */
/* Removing z_ avoids the C6510 'Invalid annotation: 'NullTerminated'' warning */
#define PROC_BSEARCH_PROTO_Z(_e_s, _proc_bsearch, __key_base_type, __datum_base_type) \
_Check_return_ \
_e_s int __cdecl _proc_bsearch( \
    _Pre_valid_ /*_In_z_*/ /*__key_base_type unused*/ const void * _key, \
    _In_reads_bytes_c_(sizeof(__datum_base_type)) const void * _datum)

/*
qsort()
*/

typedef int (__cdecl * P_PROC_QSORT) (
    const void * _arg1,
    const void * _arg2);

#define PROC_QSORT_PROTO(_e_s, _proc_qsort, __arg_base_type) \
_e_s int  __cdecl _proc_qsort( \
    _In_reads_bytes_c_(sizeof(__arg_base_type)) const void * _arg1, \
    _In_reads_bytes_c_(sizeof(__arg_base_type)) const void * _arg2)

#define QSORT_ARG1_VAR_DECL(__arg_ptr_type, __var_name) \
    const __arg_ptr_type __var_name = (__arg_ptr_type) _arg1

#define QSORT_ARG2_VAR_DECL(__arg_ptr_type, __var_name) \
    const __arg_ptr_type __var_name = (__arg_ptr_type) _arg2

/*
qsort_s()
*/

typedef int (__cdecl * P_PROC_QSORT_S) (
    _In_        void *context,
    _In_        const void * _arg1,
    _In_        const void * _arg2);

/*    _In_reads_bytes_c_(sizeof(__ctx_base_type)) void *context, \ */

#define PROC_QSORT_S_PROTO(_e_s_, _proc_qsort_s, __ctx_base_type, __arg_base_type) \
_e_s_ int __cdecl _proc_qsort_s( \
    _In_        void * context, \
    _In_reads_bytes_c_(sizeof(__arg_base_type)) const void * _arg1, \
    _In_reads_bytes_c_(sizeof(__arg_base_type)) const void * _arg2)

#if RISCOS
typedef int HOST_WND; /* really wimp_w but don't tell everyone */
#define HOST_WND_XTFMT U32_XTFMT
#define HOST_WND_NONE ((HOST_WND) 0)
#elif WINDOWS
#define HOST_WND HWND
#define HOST_WND_XTFMT PTR_XTFMT
#define HOST_WND_NONE ((HOST_WND) NULL)
#endif

#if RISCOS
#define _HwndRef_ _InVal_
#define _HwndRef_opt_ _InVal_ /* may be 0 */

#define UNREFERENCED_PARAMETER_HwndRef_ UNREFERENCED_PARAMETER_InVal_
#elif WINDOWS
#define _HwndRef_ _InRef_
#define _HwndRef_opt_ _InRef_opt_ /* may be NULL */

#define UNREFERENCED_PARAMETER_HwndRef_ UNREFERENCED_PARAMETER_InRef_
#endif

/*
GDI coordinates are large signed things (device units: OS units on RISC OS)
*/

typedef S32 GDI_COORD; typedef GDI_COORD * P_GDI_COORD; typedef const GDI_COORD * PC_GDI_COORD;

#define GDI_COORD_MAX S32_MAX

#define GDI_COORD_TFMT TEXT("%d")

/*
points, or simply pairs of coordinates
*/

typedef struct GDI_POINT
{
    GDI_COORD x, y;
}
GDI_POINT, * P_GDI_POINT; typedef const GDI_POINT * PC_GDI_POINT;

#define GDI_POINT_TFMT \
    TEXT("x = ") GDI_COORD_TFMT TEXT(", y = ") GDI_COORD_TFMT

#define GDI_POINT_ARGS(gdi_point__ref) \
    (gdi_point__ref).x, \
    (gdi_point__ref).y

typedef struct GDI_SIZE
{
    GDI_COORD cx, cy;
}
GDI_SIZE, * P_GDI_SIZE; typedef const GDI_SIZE * PC_GDI_SIZE;

/*
boxes, or simply pairs of points
*/

typedef struct GDI_BOX
{
    GDI_COORD x0, y0, x1, y1;
}
GDI_BOX, * P_GDI_BOX; typedef const GDI_BOX * PC_GDI_BOX;

#define GDI_BOX_TFMT \
    TEXT("x0 = ") GDI_COORD_TFMT TEXT(", y0 = ") GDI_COORD_TFMT TEXT("; ") \
    TEXT("x1 = ") GDI_COORD_TFMT TEXT(", y1 = ") GDI_COORD_TFMT

#define GDI_BOX_ARGS(gdi_box__ref) \
    (gdi_box__ref).x0, \
    (gdi_box__ref).y0, \
    (gdi_box__ref).x1, \
    (gdi_box__ref).y1

/*
ordered rectangles
*/

typedef struct GDI_RECT
{
    GDI_POINT tl, br;
}
GDI_RECT, * P_GDI_RECT; typedef const GDI_RECT * PC_GDI_RECT;

#define GDI_RECT_TFMT \
    TEXT("tl = ") GDI_COORD_TFMT TEXT(",") GDI_COORD_TFMT TEXT("; ") \
    TEXT("br = ") GDI_COORD_TFMT TEXT(",") GDI_COORD_TFMT

#define GDI_RECT_ARGS(gdi_rect__ref) \
    (gdi_rect__ref).tl.x, \
    (gdi_rect__ref).tl.y, \
    (gdi_rect__ref).br.x, \
    (gdi_rect__ref).br.y

/*
S32 variants where we have neither GDI nor PIXIT units
e.g. pixits-per-pixel values during scaling
*/

typedef struct S32_POINT
{
    S32 x, y;
}
S32_POINT, * P_S32_POINT; typedef const S32_POINT * PC_S32_POINT;

typedef struct S32_BOX
{
    S32 x0, y0, x1, y1;
}
S32_BOX, * P_S32_BOX; typedef const S32_BOX * PC_S32_BOX;

typedef struct S32_RECT
{
    S32_POINT tl, br;
}
S32_RECT, * P_S32_RECT; typedef const S32_RECT * PC_S32_RECT;

/* NB suppressing C4127: conditional expression is constant */
#define while_constant(c) \
    __pragma(warning(push)) __pragma(warning(disable:4127)) while(c) __pragma(warning(pop))

/* NB suppressing C4127: conditional expression is constant */
#define if_constant(expr) \
    __pragma(warning(push)) __pragma(warning(disable:4127)) if(expr) __pragma(warning(pop))

#if defined(CODE_ANALYSIS)
#define CODE_ANALYSIS_ONLY(stmt)      stmt
#else
#define CODE_ANALYSIS_ONLY(stmt)      /* stmt omitted in normal build */
#endif

#if defined(CODE_ANALYSIS)
#define CODE_ANALYSIS_ONLY_ARG(arg)   , arg
#else
#define CODE_ANALYSIS_ONLY_ARG(arg)   /* no arg in normal build */
#endif

#if RISCOS && 0

/* Norcroft produces invalid instructions in RTL loader for the other set! */

#define short_memcmp32(src_1, src_2, n_bytes) \
    memcmp(src_1, src_2, n_bytes)

#else

static __forceinline int
short_memcmp32(
    _In_reads_bytes_(n_bytes) PC_ANY src_any_1,
    _In_reads_bytes_(n_bytes) PC_ANY src_any_2,
    _InVal_     U32 n_bytes)
{
    PC_BYTE src_1 = (PC_BYTE) src_any_1;
    PC_BYTE src_2 = (PC_BYTE) src_any_2;
    const PC_BYTE end_src_2 = src_2 + n_bytes;

    while(src_2 < end_src_2)
    {
        const int byte_a = *src_1++;
        const int byte_b = *src_2++;
        if(byte_a != byte_b)
            return(byte_a - byte_b);
    }

    return(0);
}

#endif

#if !defined(INTRINSIC_MEMCMP)

static __forceinline int
memcmp32(
    _In_reads_bytes_(n_bytes) PC_ANY src_1,
    _In_reads_bytes_(n_bytes) PC_ANY src_2,
    _InVal_     U32 n_bytes)
{
    return(memcmp(src_1, src_2, n_bytes));
}

#else

#define memcmp32(src_1, src_2, n_bytes) \
    memcmp(src_1, src_2, n_bytes)

#endif /* INTRINSIC_MEMCMP */

#if !defined(INTRINSIC_MEMCPY)

static __forceinline void
memcpy32(
    _Out_writes_bytes_all_(n_bytes) P_ANY dst,
    _In_reads_bytes_(n_bytes) PC_ANY src,
    _InVal_     U32 n_bytes)
{
    (void) memcpy(dst, src, n_bytes);
}

static __forceinline void
memmove32(
    _Out_writes_bytes_all_(n_bytes) P_ANY dst,
    _In_reads_bytes_(n_bytes) PC_ANY src,
    _InVal_     U32 n_bytes)
{
    (void) memmove(dst, src, n_bytes);
}

#else

#define memcpy32(dst, src, n_bytes) \
    (void) memcpy(dst, src, n_bytes)

#define memmove32(dst, src, n_bytes) \
    (void) memmove(dst, src, n_bytes)

#endif /* INTRINSIC_MEMCPY */

#if !defined(INTRINSIC_MEMSET)

static __forceinline void
memset32(
    _Out_writes_bytes_all_(n_bytes) P_ANY dst,
    _InVal_     int byteval,
    _InVal_     U32 n_bytes)
{
    (void) memset(dst, byteval, n_bytes);
}

#else

#define memset32(dst, byteval, n_bytes) \
    (void) memset(dst, byteval, (n_bytes))

#endif /* INTRINSIC_MEMSET */

#if RISCOS

static __forceinline void
SecureZeroMemory(
    _Out_writes_bytes_all_(n_bytes) P_ANY ptr,
    U32 n_bytes)
{
    volatile char * vptr = (volatile char *) ptr;

    while(0 != n_bytes)
    {
        *vptr++ = 0;
        n_bytes--;
    }
}

#endif /* OS */

/* slower way to clear memory, but it saves space when speed is not important */

extern void
memclr32(
    _Out_writes_bytes_(n_bytes) P_ANY ptr,
    _InVal_     U32 n_bytes);

#define zero_array(_array) \
    (void) memset(_array, 0, sizeof(_array))

#define zero_array_fn(_array) \
    memclr32(_array, sizeof32(_array))

#define zero_struct(_struct) \
    (void) memset(&_struct, 0, sizeof(_struct))

#define zero_struct_fn(_struct) \
    memclr32(&_struct, sizeof32(_struct))

#define zero_struct_ptr(_ptr) \
    (void) memset(_ptr, 0, sizeof(*(_ptr)))

#define zero_struct_ptr_fn(_ptr) \
    memclr32(_ptr, sizeof32(*(_ptr)))

#define zero_32(_struct) \
    * (P_U32) (&_struct) = 0

#define zero_32_ptr(_ptr) \
    * (P_U32) (_ptr) = 0

/******************************************************************************

where possible, functions should return a STATUS
STATUS is a system-wide error code:

    >= 0 no error, function dependent success code
    <  0 error, system wide error code

helper macros are given:

BOOL    status_fail(STATUS)         TRUE if status is not OK
BOOL    status_ok(STATUS)           TRUE if status is OK
BOOL    status_done(STATUS)         TRUE if action taken: i.e. +ve
void    status_break(STATUS)        breaks on error condition
void    status_consume(STATUS)      executes expression, discards result
void    status_return(STATUS)       returns from current function with STATUS error code
void    status_accumulate(S1, S2)   executes expression S2, accumulating error into S1
void    status_assert(STATUS)       executes expression, discards result, also reports error in debug version
STATUS  status_wrap(STATUS)         executes expression, returns result, also reports error in debug version

******************************************************************************/

#if 1
typedef /*_Return_type_success_(return >= 0)*/ S32 STATUS;
#define STATUS_TFMT S32_TFMT
#else
/*#pragma warning(disable:4820)*/
typedef __int64 STATUS;
#define STATUS_TFMT TEXT("%I64d")
#endif
typedef STATUS * P_STATUS;

#define STATUS_MSG_INCREMENT ((STATUS) ((STATUS) 1 << 16))
#define STATUS_ERR_INCREMENT (-STATUS_MSG_INCREMENT)

/* +ve status return values are good */
#define STATUS_DONE         ((STATUS) (+1))
#define STATUS_OK           ((STATUS)  (0))

/* -ve status return values are errors */
#define STATUS_ERROR_RQ     ((STATUS) (-1))
#define STATUS_FAIL         ((STATUS) (-2))
#define STATUS_NOMEM        ((STATUS) (-3))
#define STATUS_CANCEL       ((STATUS) (-4))
#define STATUS_CHECK        ((STATUS) (-5))

/* TRUE if status is not OK, i.e. -ve */
#define status_fail(status) ( \
    (status)  < STATUS_OK )

/* TRUE if status is OK, i.e. +ve or zero */
#define status_ok(status) ( \
    (status) >= STATUS_OK )

/* TRUE if action taken: i.e. +ve, non-zero */
#define status_done(status) ( \
    (status)  > STATUS_OK )

/* breaks out of loop on error condition */
#define status_break(status) \
    { \
    if(status_fail(status)) \
        break; \
    }

#define status_consume(expr) \
    consume(STATUS, expr)

/* returns from current function with STATUS error code */
#define status_return(expr) \
    do { \
    STATUS status_e = (expr); \
    if(status_fail(status_e)) \
        return(status_e); \
    } while_constant(0)

/* accumulate status failure from expression into status_a
 * NB this will preserve an initial STATUS_DONE if expression returns STATUS_OK
 */
#define status_accumulate(status_a, expr) \
    do { \
    STATUS status_e = (expr); \
    if(status_fail(status_e) && status_ok(status_a)) \
        status_a = status_e; \
    } while_constant(0)

/* standard constants */
#ifndef FALSE
#define FALSE false
#endif

#ifndef TRUE
#define TRUE true
#endif

#define INDETERMINATE 2

/*
Trivial character definitions
*/

#ifndef LF
#define LF 10
#endif

#ifndef CR
#define CR 13
#endif

/*
UCS-4 character definitions
*/

/*
0000..001F C0 Controls
*/

#define CH_NULL                     0x00
#define CH_TAB                      0x09
#define CH_INLINE                   0x15    /* Fireworkz inline identifier code */ /* used to be 0x07 - 0x15 is RISC OS Font Manager Comment sequence (see fonty.c) */
#define CH_ESCAPE                   0x1B

/*
0020..007F Basic Latin
*/

#define CH_SPACE                    0x20    /*   */
#define CH_EXCLAMATION_MARK         0x21    /* ! */
#define CH_QUOTATION_MARK           0x22    /* " */
#define CH_NUMBER_SIGN              0x23    /* # */ /*hash*/
#define CH_HASH                     CH_NUMBER_SIGN
#define CH_DOLLAR_SIGN              0x24    /* $ */
#define CH_PERCENT_SIGN             0x25    /* % */
#define CH_AMPERSAND                0x26    /* & */
#define CH_APOSTROPHE               0x27    /* ' */
#define CH_LEFT_PARENTHESIS         0x28    /* ( */
#define CH_RIGHT_PARENTHESIS        0x29    /* ) */
#define CH_ASTERISK                 0x2A    /* * */
#define CH_PLUS_SIGN                0x2B    /* + */
#define CH_COMMA                    0x2C    /* , */
#define CH_HYPHEN_MINUS             0x2D    /* - */
#define CH_MINUS_SIGN__BASIC        CH_HYPHEN_MINUS
#define CH_FULL_STOP                0x2E    /* . */
#define CH_SOLIDUS                  0x2F    /* / */ /*slash*/
#define CH_FORWARDS_SLASH           CH_SOLIDUS

#define CH_DIGIT_ZERO               0x30    /* 0 */
#define CH_DIGIT_ONE                0x31    /* 1 */
#define CH_DIGIT_SEVEN              0x37    /* 7 */
#define CH_DIGIT_NINE               0x39    /* 9 */
#define CH_COLON                    0x3A    /* : */
#define CH_SEMICOLON                0x3B    /* ; */
#define CH_LESS_THAN_SIGN           0x3C    /* < */
#define CH_EQUALS_SIGN              0x3D    /* = */
#define CH_GREATER_THAN_SIGN        0x3E    /* > */
#define CH_QUESTION_MARK            0x3F    /* ? */

#define CH_COMMERCIAL_AT            0x40    /* @ */

#define CH_LEFT_SQUARE_BRACKET      0x5B    /* [ */
#define CH_REVERSE_SOLIDUS          0x5C    /* \ */ /*backslash*/
#define CH_BACKWARDS_SLASH          CH_REVERSE_SOLIDUS
#define CH_RIGHT_SQUARE_BRACKET     0x5D    /* ] */
#define CH_CIRCUMFLEX_ACCENT        0x5E    /* ^ */
#define CH_LOW_LINE                 0x5F    /* _ */ /*underscore*/
#define CH_UNDERSCORE               CH_LOW_LINE

#define CH_LEFT_CURLY_BRACKET       0x7B    /* { */
#define CH_VERTICAL_LINE            0x7C    /* | */
#define CH_RIGHT_CURLY_BRACKET      0x7D    /* } */
#define CH_TILDE                    0x7E    /* ~ */
#define CH_DELETE                   0x7F

#include "cmodules/unicode/u0000.h" /* 0000..007F Basic Latin */
#include "cmodules/unicode/u0080.h" /* 0080..00FF Latin-1 Supplement */

/*
D800..DB7F High Surrogates
DB80..DBFF High Private Use Surrogates
DC00..DFFF Low Surrogates
These are used to obtain characters >= U+10000 under UTF-16
This requires Windows 2000 or later as WCHAR was simply UCS-2 on NT 4.0
*/

#define UCH_SURROGATE_HIGH          0xD800U
#define UCH_SURROGATE_HIGH_END      0xDBFFU
#define UCH_SURROGATE_LOW           0xDC00U
#define UCH_SURROGATE_LOW_END       0xDFFFU

/*
UCS-4 Noncharacters
*/

#define UCH_NONCHARACTER_STT        0xFDD0U
#define UCH_NONCHARACTER_END        0xFDEFU

/*
UCS-4 Specials
*/

#define UCH_REPLACEMENT_CHARACTER   0xFFFDU
#define UCH_NONCHARACTER_XXFFFE     0xFFFEU
#define UCH_NONCHARACTER_XXFFFF     0xFFFFU

#define UCH_UCS2_END                0xFFFFU     /* only UCS-2 values <= this code point are valid */
#define UCH_UCS2_INVALID            0x10000U    /* only UCS-2 values <  this code point are valid (easier on the ARM) */

#define UCH_UNICODE_END             0x10FFFFU   /* only UCS-4 values <= this code point are valid */
#define UCH_UNICODE_INVALID         0x110000U   /* only UCS-4 values <  this code point are valid (easier on the ARM) */

/* type for worst case alignment */
#if RISCOS
typedef int align_t;
#define SIZEOF_ALIGN_T 4
#elif WINDOWS && 0 /* only really intended to pack so hard for low-memory DOS systems */
typedef char align_t;
#define SIZEOF_ALIGN_T 1
#else
typedef int align_t;
#define SIZEOF_ALIGN_T 4
#endif

/* useful max/min macros (NB watch out for multiple-evaluation side-effects) */

#ifndef MAX
#define MAX(A,B) ((A) > (B) ? (A) : (B))
#endif
#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

/* useful division a/b macros to round result to +/- infinity rather than towards zero (NB watch out for multiple-evaluation side-effects) */

#define idiv_ceil_u(a, b) ( \
    ((a) + (b) - 1) / (b) ) /* for unsigned a */

#define idiv_ceil(a, b) ( \
    (((a) >  0) ? ((a) + (b) - 1) : (a)) / (b) )

#define idiv_floor_u(a, b) ( \
    (a) / (b) ) /* for unsigned a */

#define idiv_floor(a, b) ( \
    (((a) >= 0) ? (a) : ((a) - (b) + 1)) / (b) )

/* functions for where it can't be avoided */

_Check_return_
static inline S32
idiv_ceil_fn(
    _InVal_     S32 a,
    _InVal_     S32 b)
{
    return(idiv_ceil(a, b));
}

_Check_return_
static inline S32
idiv_floor_fn(
    _InVal_     S32 a,
    _InVal_     S32 b)
{
    return(idiv_floor(a, b));
}

#if defined(UNREFERENCED_PARAMETER) && ((defined(__clang__) && !defined(_PREFAST_)) || (0 /*defined(_MSC_VER) && defined(_PREFAST_)*/))
#undef UNREFERENCED_PARAMETER
#endif
#if !defined(UNREFERENCED_PARAMETER)
#define UNREFERENCED_PARAMETER(p)           (p)=(p)
#endif
#define UNREFERENCED_PARAMETER_CONST(p)     (void)(p)
#define UNREFERENCED_PARAMETER_InRef_(p)    (void)(p)
#define UNREFERENCED_PARAMETER_InoutRef_(p) (void)(p)
#define UNREFERENCED_PARAMETER_OutRef_(p)   (void)(p)
#define UNREFERENCED_PARAMETER_InVal_(p)    (void)(p)

#define UNREFERENCED_LOCAL_VARIABLE(v)      (void)(v)

#define consume(__base_type, expr) \
    do { \
        const __base_type __consume_v = (expr); \
        UNREFERENCED_LOCAL_VARIABLE(__consume_v); \
    } while_constant(0)

#define consume_ptr(expr) consume(PC_ANY, expr)

#define consume_bool(expr) consume(BOOL, expr)

#define consume_int(expr) consume(int, expr) /* quite useful for printf() etc. */

#define PTR_IS_NULL(/*ANY*/ p) ( \
    NULL == /*(PC_ANY)*/ (p) )

#define PTR_NOT_NULL(/*ANY*/ p) ( \
    NULL != /*(PC_ANY)*/ (p) )

/*
a pointer that will hopefully give a trap when dereferenced even when not CHECKING
*/

#if RISCOS

/* doesn't seem to be anything at 3GB */

#define BAD_POINTER_X(__ptr_type, X) ((__ptr_type) \
    (uintptr_t) (0xC0000000U | (X)))

#define BAD_POINTER_X_RANGE ( \
    (uintptr_t) (0x0000001FU))

#elif defined(_WIN64)

#define BAD_POINTER_X(__ptr_type, X) ((__ptr_type) \
    (uintptr_t) (0x0000000000000000U | (X)))

#define BAD_POINTER_X_RANGE ( \
    (uintptr_t) (0x000000000000001FU))

#else

#define BAD_POINTER_X(__ptr_type, X) ((__ptr_type) \
    (uintptr_t) (0x00000000U | (X)))

#define BAD_POINTER_X_RANGE ( \
    (uintptr_t) (0x0000001FU))

#endif /* _WIN64 */

#define PTR_IS_BAD_POINTER(p) ( \
    ((uintptr_t) (p) - BAD_POINTER_X(uintptr_t, 0)) <= BAD_POINTER_X_RANGE )

#define PTR_NOT_BAD_POINTER(p) ( \
    ((uintptr_t) (p) - BAD_POINTER_X(uintptr_t, 0)) >  BAD_POINTER_X_RANGE )

/*
a more limited range of values for strictly typed pointers when CHECKING
*/

#if CHECKING || (CHECKING_FOR_CODE_ANALYSIS && 1)

#define PTR_NONE_X(__ptr_type, X) \
    BAD_POINTER_X(__ptr_type, X)

#define PTR_NONE_X_RANGE ( \
    (uintptr_t) (0x1FU))

#define PTR_IS_NONE(/*ANY*/ p) ( \
    ((uintptr_t) (p) - PTR_NONE_X(uintptr_t, 0)) <= PTR_NONE_X_RANGE )

#define PTR_NOT_NONE(/*ANY*/ p) ( \
    ((uintptr_t) (p) - PTR_NONE_X(uintptr_t, 0)) >  PTR_NONE_X_RANGE )

#define PTR_IS_NONE_X(__ptr_type, X, p) ( \
    PTR_NONE_X(__ptr_type, X) == (p) )

#define PTR_NOT_NONE_X(__ptr_type, X, p) ( \
    PTR_NONE_X(__ptr_type, X) != (p) )

#define PTR_IS_NULL_OR_NONE(/*ANY*/ p) ( \
    PTR_IS_NULL(p) || PTR_IS_NONE(p) )

#define PTR_NOT_NULL_OR_NONE(/*ANY*/ p) ( \
    PTR_NOT_NULL(p) && PTR_NOT_NONE(p) )

#define PTR_IS_NULL_OR_NONE_X(__ptr_type, X, p) ( \
    PTR_IS_NULL(p) || PTR_IS_NONE_X(__ptr_type, X, p) )

#define PTR_NOT_NULL_OR_NONE_X(__ptr_type, X, p) ( \
    PTR_NOT_NULL(p) && PTR_NOT_NONE_X(__ptr_type, X, p) )

/* Some SAL to reflect the fact that PTR_NONE is not NULL in this state */

#define _In_maybenone_          _In_
#define _Inout_maybenone_       _Inout_
#define _Out_maybenone_         _Out_

#define _In_reads_bytes_maybenone_  _In_reads_bytes_ /*-_opt_*/

#define _Ret_notnone_           _Ret_notnull_
#define _Ret_maybenone_         _Ret_notnull_

#define _Ret_writes_maybenone_  _Ret_writes_

#else /* NOT CHECKING */

/* paranoid compatibility - all PTR_NONE == NULL */

#define PTR_NONE_X(__ptr_type, X) ( \
    (__ptr_type) NULL)

#define PTR_IS_NONE(/*ANY*/ p) ( \
    PTR_IS_NULL(p) )

#define PTR_NOT_NONE(/*ANY*/ p) ( \
    PTR_NOT_NULL(p) )

#define PTR_IS_NONE_X(__ptr_type, X, p) ( \
    (PTR_NONE_X(__ptr_type, X) == (p)) )

#define PTR_NOT_NONE_X(__ptr_type, X, p) ( \
    (PTR_NONE_X(__ptr_type, X) != (p)) )

#define PTR_IS_NULL_OR_NONE(/*ANY*/ p) ( \
    /* PTR_IS_NULL(p) || */ PTR_IS_NONE(p) )

#define PTR_NOT_NULL_OR_NONE(/*ANY*/ p) ( \
    /* PTR_NOT_NULL(p) && */ PTR_NOT_NONE(p) )

#define PTR_IS_NULL_OR_NONE_X(__ptr_type, X, p) ( \
    /* PTR_IS_NULL(p) || */ PTR_IS_NONE_X(__ptr_type, X, p) )

#define PTR_NOT_NULL_OR_NONE_X(__ptr_type, X, p) ( \
    /* PTR_NOT_NULL(p) && */ PTR_NOT_NONE_X(__ptr_type, X, p) )

/* Some SAL to reflect the fact that PTR_NONE is NULL in this state */

#define _In_maybenone_          _In_opt_
#define _Inout_maybenone_       _Inout_opt_
#define _Out_maybenone_         _Out_opt_

#define _In_reads_bytes_maybenone_  _In_reads_bytes_opt_

#define _Ret_notnone_           _Ret_notnull_
#define _Ret_maybenone_         _Ret_maybenull_

#define _Ret_writes_maybenone_  _Ret_writes_maybenull_

#endif /* CHECKING */

#define PTR_ASSERT(p) \
    assert(PTR_NOT_NULL_OR_NONE(p))

#define _P_DATA_NONE(__ptr_type) ( \
    PTR_NONE_X(__ptr_type, 1))

#define _IS_P_DATA_NONE(__ptr_type, p) ( \
    PTR_IS_NONE_X(__ptr_type, 1, p))

/*
round up v if needed to the next r (r must be a power of 2)
*/

#define round_up(v, r) ( \
    ((v) + ((r) - 1)) & ~((r) - 1))

#define sizeof32(__base_type) \
    ((U32) sizeof(__base_type))

/* sizeof the type obtained by dereferencing the pointer type */
#define sizeof_deref(__ptr_type) \
    sizeof((__ptr_type) 0)

#define sizeof32_deref(__ptr_type) \
    sizeof32((__ptr_type) 0)

#define offsetof32(__base_type, _member) \
    ((U32) offsetof(__base_type, _member))

/*
deviant of sizeof{32}(exp) formed from offsetof{32}(type, member)
*/

#define sizeofmemb(__base_type, _member) \
    sizeof(((__base_type *) 0)->_member)

#define sizeofmemb32(__base_type, _member) \
    sizeof32(((__base_type *) 0)->_member)
   /*
    * expands to an integral constant expression that has type size_t/U32, the
    * value of which is the size in bytes of a member designated by the
    * identifier member of a structure designated by type (if the
    * specified member is a bit-field, the behaviour is undefined).
    */

/*
number of elements in an array (use rather than sizeof()/sizeof32() in many instances)
*/

#define elemof(_array) \
    (sizeof(_array)/sizeof(_array[0]))

#define elemof32(_array) \
    (sizeof32(_array)/sizeof32(_array[0]))

/* is a (zero-based) index valid for this array? */
#define IS_ARRAY_INDEX_VALID(_idx, _elemof32_array) \
    ((U32) (_idx) < _elemof32_array)

/*
Enumeration handling
*/

#define ENUM_NEXT(__enum_type, value) \
    ((__enum_type) ((int) (value) + 1))

#define ENUM_PREV(__enum_type, value) \
    ((__enum_type) ((int) (value) - 1))

#define ENUM_INCR(__enum_type, value__ref) \
    (value__ref = ENUM_NEXT(__enum_type, (value__ref)))

#define ENUM_DECR(__enum_type, value__ref) \
    (value__ref = ENUM_PREV(__enum_type, (value__ref)))

/* enumerations are sometimes stored in unsigned bit fields or other shorter types */
#define ENUM_PACK(__packed_type, value) \
    ((__packed_type) (value))

#define ENUM_UNPACK(__enum_type, packed_value) \
    ((__enum_type) (packed_value))

#if CHECKING
#define CHECKING_ONLY(stmt)     stmt
#else
#define CHECKING_ONLY(stmt)     /* stmt omitted in normal build */
#endif

#if CHECKING
#define CHECKING_ONLY_ARG(arg) , arg
#else
#define CHECKING_ONLY_ARG(arg) /* no arg */
#endif

#if (defined(PROFILING) || defined(FULL_FRAMES)) && RISCOS
extern void
profile_ensure_frame(void); /* ensure procedure gets a stack frame we can trace out of at the cost of two branches */
#else
#define profile_ensure_frame() /* nothing */
#endif

#if __STDC_VERSION__ < 199901L
#include "cmodules/mathxtra.h" /* for isless() etc when not C99 */
#endif

#endif /* __coltsoft_h */

/* end of coltsoft.h */
