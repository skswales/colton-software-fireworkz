/* quickblk.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/******************************************************************************
*
* quick_block allocation module (UCHARS-based)
*
* efficient routines for handling temporary buffers
*
* you supply a 'static' buffer; if the item to be added to the buffer
* would overflow the supplied buffer, a handle-based buffer is allocated
*
* SKS May 2014 split off from quickblk.c
*
******************************************************************************/

#include "common/gflags.h"

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

#ifndef          __utf16_h
#include "cmodules/utf16.h"
#endif

#ifndef          __quickublk_h
#include "cmodules/quickublk.h"
#endif

/******************************************************************************
*
* UCHARS-based quick block
*
******************************************************************************/

/* dispose of a quick_block is inline */

/******************************************************************************
*
* dispose of a quick_ublock of data but leave as much text in place as possible (see charts)
*
******************************************************************************/

extern void
quick_ublock_dispose_leaving_buffer_valid(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*disposed*/)
{
    /* have we overflowed the buffer and gone into handle allocation? */
    if(0 != quick_ublock_array_handle_ref(p_quick_ublock))
    {   /* copy as much stuff as possible back down before deleting the handle */
        const U32 uchars_n = MIN(p_quick_ublock->ub_static_buffer_size, array_elements32(&quick_ublock_array_handle_ref(p_quick_ublock)));
        PC_BYTE p_data = array_rangec(&quick_ublock_array_handle_ref(p_quick_ublock), BYTE, 0, uchars_n);

        memcpy32(p_quick_ublock->ub_p_static_buffer, p_data, uchars_n);

        al_array_dispose(&quick_ublock_array_handle_ref(p_quick_ublock));
    }

    p_quick_ublock->ub_static_buffer_used = 0;
}

/******************************************************************************
*
* empty a quick_ublock of data for efficient reuse
*
******************************************************************************/

extern void
quick_ublock_empty(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*emptied*/)
{
    if(0 != quick_ublock_array_handle_ref(p_quick_ublock))
    {
#if CHECKING_QUICK_UBLOCK
        /* trash handle on empty when CHECKING_QUICK_UBLOCK */
        const U32 n_bytes = array_elements32(&quick_ublock_array_handle_ref(p_quick_ublock));
        _do_uqb_fill(uchars_bptr(array_range(&quick_ublock_array_handle_ref(p_quick_ublock), UCHARB, 0, n_bytes)), n_bytes);
#endif
        al_array_empty(&quick_ublock_array_handle_ref(p_quick_ublock));
    }

    /* trash buffer on empty when CHECKING_QUICK_UBLOCK */
    _do_uqb_fill(p_quick_ublock->ub_p_static_buffer, p_quick_ublock->ub_static_buffer_size);
    p_quick_ublock->ub_static_buffer_used = 0;
}

/******************************************************************************
*
* efficient routine for allocating temporary buffers
*
* you supply a buffer; if the item is bigger than this,
* an indirect buffer is allocated
*
* remember to call quick_ublock_dispose afterwards!
*
******************************************************************************/

_Check_return_
_Ret_writes_maybenull_(extend_by) /* may be NULL */
extern P_UCHARS
quick_ublock_extend_by(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*extended*/,
    _InVal_     U32 extend_by,
    _OutRef_    P_STATUS p_status)
{
    ARRAY_INIT_BLOCK array_init_block;
    S32 size_increment;
    P_UCHARS p_output;

    *p_status = STATUS_OK;

    if(0 == extend_by) /* realloc by 0 is OK, but return a non-NULL rubbish pointer */
        return(BAD_POINTER_X(P_UCHARS, 0));

    if(extend_by <= (p_quick_ublock->ub_static_buffer_size - p_quick_ublock->ub_static_buffer_used))
    {
        P_UCHARS p_output = uchars_AddBytes_wr(p_quick_ublock->ub_p_static_buffer, p_quick_ublock->ub_static_buffer_used);
        p_quick_ublock->ub_static_buffer_used += extend_by;
        return(p_output);
    }

    if(0 != quick_ublock_array_handle_ref(p_quick_ublock))
        return(al_array_extend_by(&quick_ublock_array_handle_ref(p_quick_ublock), _UCHARS, extend_by, PC_ARRAY_INIT_BLOCK_NONE, p_status));

    /* transition from static buffer to array handle */
    size_increment = p_quick_ublock->ub_static_buffer_size >> 2; /* / 4 */

    if( size_increment < 4)
        size_increment = 4;

    array_init_block_setup(&array_init_block, size_increment, sizeof32(UCHARB), FALSE);

    if(NULL != (p_output = al_array_alloc(&quick_ublock_array_handle_ref(p_quick_ublock), _UCHARS,
                                          p_quick_ublock->ub_static_buffer_used + extend_by, &array_init_block, p_status)))
    {
        memcpy32(p_output,
                 p_quick_ublock->ub_p_static_buffer,
                 p_quick_ublock->ub_static_buffer_used * sizeof32(UCHARB));

        uchars_IncBytes_wr(p_output, p_quick_ublock->ub_static_buffer_used);

        p_quick_ublock->ub_static_buffer_used = p_quick_ublock->ub_static_buffer_size; /* indicate static buffer full for fast add etc. */

        /* trash buffer once copied when CHECKING_QUICK_UBLOCK */
        _do_uqb_fill(p_quick_ublock->ub_p_static_buffer, p_quick_ublock->ub_static_buffer_size);
    }

    return(p_output);
}

/******************************************************************************
*
* shrink quick_ublock by this many (NB -ve)
*
******************************************************************************/

extern void
quick_ublock_shrink_by(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*shrunk*/,
    _InVal_     S32 shrink_by)
{
    U32 shrink_by_pos;

    assert(shrink_by <= 0);

    if(shrink_by >= 0)
        return;

    if(0 != quick_ublock_array_handle_ref(p_quick_ublock))
    {
        al_array_shrink_by(&quick_ublock_array_handle_ref(p_quick_ublock), shrink_by);
        return;
    }

    shrink_by_pos = (U32) -(shrink_by);

    if(p_quick_ublock->ub_static_buffer_used >= shrink_by_pos)
    {
        p_quick_ublock->ub_static_buffer_used -= shrink_by_pos;
        /* trash invalidated section when CHECKING_QUICK_UBLOCK */
        _do_uqb_fill(ustr_AddBytes_wr(p_quick_ublock->ub_p_static_buffer, p_quick_ublock->ub_static_buffer_used), shrink_by_pos);
    }
    else
    {
        assert(p_quick_ublock->ub_static_buffer_used >= shrink_by_pos);
        p_quick_ublock->ub_static_buffer_used = 0;
        /* trash invalidated section when CHECKING_QUICK_UBLOCK */
        _do_uqb_fill(p_quick_ublock->ub_p_static_buffer, p_quick_ublock->ub_static_buffer_size);
    }
}

/******************************************************************************
*
* add a UCHARS string to a quick_ublock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_ublock_uchars_add_slow(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n)
{
    STATUS status;
    P_UCHARS dstptr;

    assert(strlen_without_NULLCH > uchars_n);

#if CHECKING_UCHARS
    /* Check that string we are appending is valid */
    status_return(uchars_inline_validate(TEXT("quick_ublock_uchars_add arg"), (PC_UCHARS_INLINE) uchars, uchars_n));
#endif

    if(NULL == (dstptr = quick_ublock_extend_by(p_quick_ublock, uchars_n, &status)))
        return(status);

    memcpy32(dstptr, uchars, uchars_n);

#if CHECKING_UCHARS
    /* Check that whole quick_ublock is valid after appending string */
    status_return(uchars_inline_validate(TEXT("quick_ublock_uchars_add ublock"), quick_ublock_uchars_inline(p_quick_ublock), quick_ublock_bytes(p_quick_ublock)));
#endif

    return(STATUS_OK);
}

_Check_return_
extern STATUS
quick_ublock_uchars_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n)
{
    assert(strlen_without_NULLCH > uchars_n);

#if CHECKING_UCHARS
    /* Check that string we are appending is valid */
    status_return(uchars_inline_validate(TEXT("quick_ublock_uchars_add arg"), (PC_UCHARS_INLINE) uchars, uchars_n));
#endif

    if(quick_ublock_uchars_add_fast(p_quick_ublock, uchars, uchars_n))
    {
#if CHECKING_UCHARS
        /* Check that whole quick_ublock is valid after appending string */
        status_return(uchars_inline_validate(TEXT("quick_ublock_uchars_add ublock"), quick_ublock_uchars_inline(p_quick_ublock), quick_ublock_bytes(p_quick_ublock)));
#endif
        return(STATUS_OK);
    }

    return(quick_ublock_uchars_add_slow(p_quick_ublock, uchars, uchars_n));
}

_Check_return_
extern STATUS
quick_ublock_ustr_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_USTR ustr)
{
    return(quick_ublock_ustr_add_n(p_quick_ublock, ustr, strlen_without_NULLCH));
}

_Check_return_
extern STATUS
quick_ublock_ustr_add_n(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_USTR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/)
{
    return(quick_ublock_uchars_add(p_quick_ublock, ustr, ustrlen32_n(ustr, uchars_n)));
}

/******************************************************************************
*
* CH_NULL terminate the given quick_ublock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_ublock_nullch_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/)
{
    return(quick_ublock_a7char_add(p_quick_ublock, CH_NULL));
}

/******************************************************************************
*
* ensure that the given quick_ublock is not CH_NULL-terminated
*
******************************************************************************/

extern void
quick_ublock_nullch_strip(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*stripped*/)
{
    U32 uchars_n = quick_ublock_bytes(p_quick_ublock);

    if(0 != uchars_n)
    {
        PC_UCHARS uchars = quick_ublock_uchars(p_quick_ublock);

        if(CH_NULL == PtrGetByteOff(uchars, uchars_n-1))
            quick_ublock_shrink_by(p_quick_ublock, -1);
    }
}

/******************************************************************************
*
* sprintf into quick_ublock
*
* NB. does NOT add terminating CH_NULL
*
******************************************************************************/

_Check_return_
extern STATUS __cdecl
quick_ublock_printf(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_ _Printf_format_string_ PC_USTR ustr_format,
    /**/        ...)
{
    va_list args;
    STATUS status;

    va_start(args, ustr_format);

    status = quick_ublock_vprintf(p_quick_ublock, ustr_format, args);

    va_end(args);

    return(status);
}

/******************************************************************************
*
* vprintf into quick_ublock
*
* NB. does NOT add a terminating CH_NULL
*
******************************************************************************/

_Check_return_
extern STATUS
quick_ublock_vprintf(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_ _Printf_format_string_ PC_USTR ustr_format,
    /**/        va_list args)
{
    PC_USTR ustr;
    STATUS status = STATUS_OK;
#if WINDOWS
    U8 preceding;
    U8 conversion;
#endif

    while(NULL != (ustr = ustrchr(ustr_format, CH_PERCENT_SIGN)))
    {
        /* output what we have so far */
        if(ustr != ustr_format)
            status_break(status = quick_ublock_ustr_add_n(p_quick_ublock, ustr_format, PtrDiffBytesU32(ustr, ustr_format)));

        ustr_format = ustr_AddBytes(ustr, 1); /* skip percent character */

        if(CH_PERCENT_SIGN == PtrGetByte(ustr_format))
        {
            status_break(status = quick_ublock_a7char_add(p_quick_ublock, CH_PERCENT_SIGN));
            ustr_IncByte(ustr_format); /* skip the escaped % too */
            continue;
        }

        /* skip goop till conversion found */
        if(NULL == (ustr = ustrpbrk(ustr_format, "diouxXcsfeEgGpnCS")))
        {
            myassert1(TEXT("conversion %%%s unhandled"), report_ustr(ustr_format));
            break;
        }

        /* skip over conversion */
#if WINDOWS
        preceding = PtrGetByteOff(ustr, -1);
        conversion = PtrGetByte(ustr);
#endif

        ustr_IncByte(ustr);

        {
        UCHARZ buffer_format[32];
        UCHARZ buffer[256];
        S32 len;

        ustr_xstrnkpy(ustr_bptr(buffer_format), elemof32(buffer_format),
                      PtrSubBytes(PC_USTR, ustr_format, 1),
                      PtrDiffBytesU32(ustr, ustr_format) + 1);

#if WINDOWS
        len = _vsnprintf_s((char *) buffer, elemof32(buffer), _TRUNCATE, (const char *) buffer_format, args /* NOT updated - see below */);
        if(-1 == len)
            len = ustrlen32(ustr_bptr(buffer)); /* limit transfer to what actually was achieved */
#else /* C99 CRT */
        len = vsnprintf((char *) buffer, elemof32(buffer), (const char *) buffer_format, args /* will be updated accordingly */);
        if(len >= elemof32(buffer))
            len = ustrlen32(ustr_bptr(buffer)); /* limit transfer to what actually was achieved */
#endif

        status_break(status = quick_ublock_uchars_add(p_quick_ublock, uchars_bptr(buffer), len));

#if WINDOWS /* Microsoft seems to pass args by value not reference so that args ain't updated like Norcroft compiler */
        switch(conversion)
        {
        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
            if(preceding == 'L')
            {
                volatile long double ld = va_arg(args, long double); ld=ld;
            }
            else /* NB floats are promoted to double when passed to variadic functions */
            {
                volatile double d = va_arg(args, double); d=d;
            }
            break;

        case 's':
        case 'S':
        case 'p':
        case 'n':
            {
            volatile char * p = va_arg(args, char *); p=p;
            break;
            }

        case 'c':
        case 'C':
            { /* chars are promoted to int when passed as parameters */
            volatile int ci = va_arg(args, int); ci=ci;
            break;
            }

        default:
        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
            if(preceding == 'l')
            {
                volatile long li = va_arg(args, long); li=li;
            }
            else /* NB shorts are promoted to int when passed to variadic functions */
            {
                volatile int i = va_arg(args, int); i=i;
            }
            break;
        }
#endif /* WINDOWS */
        } /*block*/

        ustr_format = ustr; /* skip whole conversion sequence */
    }

    /* output trailing fragment */
    if(status_ok(status) && (PtrGetByte(ustr_format) != CH_NULL))
        status = quick_ublock_ustr_add(p_quick_ublock, ustr_format);

    return(status);
}

/* appending with conversion */

/******************************************************************************
*
* add a ASCII 7-bit character to a quick_ublock (NB no conversion ever needed)
*
******************************************************************************/

_Check_return_
static STATUS
quick_ublock_a7char_add_slow(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     A7CHAR a7char)
{
    UCHARB uchar_buffer[1];
    
    assert(0 == (0x80 & a7char));

    uchar_buffer[0] = (UCHARB) (a7char & 0x7F);

    return(quick_ublock_uchars_add_slow(p_quick_ublock, uchars_bptr(uchar_buffer), 1));
}

_Check_return_
extern STATUS
quick_ublock_a7char_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     A7CHAR a7char)
{
    assert(0 == (0x80 & a7char));

    if(_quick_ublock_uchar1_add_fast(p_quick_ublock, (a7char & 0x7F)))
        return(STATUS_OK);

    return(quick_ublock_a7char_add_slow(p_quick_ublock, a7char));
}

/******************************************************************************
*
* add a UCS-4 character to a quick_ublock, converting from UCS4 to UCHAR
*
******************************************************************************/

_Check_return_
static STATUS
quick_ublock_ucs4_add_slow(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     UCS4 ucs4)
{
    UCHARB uchar_buffer[8];

#if USTR_IS_SBSTR
    if(ucs4_is_sbchar(ucs4))
    {
        uchar_buffer[0] = (UCHARB) (ucs4 & 0xFF);
    }
    else
    {   /* out-of-range -> substitute */
        assert(ucs4_is_sbchar(ucs4));
        uchar_buffer[0] = CH_QUESTION_MARK;
    }

    return(quick_ublock_uchars_add_slow(p_quick_ublock, uchars_bptr(uchar_buffer), 1));
#else
    const U32 len = uchars_char_encode(uchars_bptr(uchar_buffer), elemof32(uchar_buffer), ucs4);

    if(quick_ublock_uchars_add_fast(p_quick_ublock, uchars_bptr(uchar_buffer), len))
        return(STATUS_OK);

    return(quick_ublock_uchars_add_slow(p_quick_ublock, uchars_bptr(uchar_buffer), len));
#endif /* USTR_IS_SBSTR */
}

_Check_return_
extern STATUS
quick_ublock_ucs4_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     UCS4 ucs4)
{
    if(ucs4_is_ascii7(ucs4))
    {
        if(_quick_ublock_uchar1_add_fast(p_quick_ublock, (ucs4 & 0x7F)))
            return(STATUS_OK);
    }

    return(quick_ublock_ucs4_add_slow(p_quick_ublock, ucs4));
}

/******************************************************************************
*
* add some SBCHARs to a quick_ublock, converting to UCHAR from SBCHAR as needed
*
******************************************************************************/

_Check_return_
extern STATUS
quick_ublock_sbchars_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(sbchars_n) PC_SBCHARS sbchars_in,
    _InVal_     U32 sbchars_n)
{
#if USTR_IS_SBSTR
    return(quick_ublock_uchars_add(p_quick_ublock, (PC_UCHARS) sbchars_in, sbchars_n));
#else
    PC_SBCHARS sbchars = sbchars_in;
    BOOL is_pure_ascii7 = TRUE;
    U32 i;

    for(i = 0; i < sbchars_n; i++)
    {
        SBCHAR sbchar = *sbchars++;

        if(!u8_is_ascii7(sbchar))
        {
            is_pure_ascii7 = FALSE; /* conversion will be needed */
            --sbchars;
            break; /* leave i referring to the failed byte, p_u8 pointing at it */
        }
    }

    /* there probably is a run of pure ASCII-7 to start with (or full string!) */
    if(0 != i)
    {
        status_return(quick_ublock_uchars_add(p_quick_ublock, (PC_UCHARS) sbchars_in, i));
    }

    if(!is_pure_ascii7)
    {   /* need to convert the rest one at a time */
        const SBCHAR_CODEPAGE sbchar_codepage = get_system_codepage();
        for(; i < sbchars_n; i++)
        {
            SBCHAR sbchar = *sbchars++;
            UCS4 ucs4 = ucs4_from_sbchar_with_codepage(sbchar, sbchar_codepage);
            status_return(quick_ublock_ucs4_add(p_quick_ublock, ucs4));
        }
    }

#if CHECKING_UCHARS
    /* Check that whole quick_ublock is valid after appending converted string */
    status_return(uchars_inline_validate(TEXT("quick_ublock_sbchars_add ublock"), quick_ublock_uchars_inline(p_quick_ublock), quick_ublock_bytes(p_quick_ublock)));
#endif

    return(STATUS_OK);
#endif /* USTR_IS_SBSTR */
}

_Check_return_
extern STATUS
quick_ublock_sbstr_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_SBSTR sbstr)
{
    return(quick_ublock_sbstr_add_n(p_quick_ublock, sbstr, strlen_without_NULLCH));
}

_Check_return_
extern STATUS
quick_ublock_sbstr_add_n(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_SBSTR sbstr,
    _InVal_     U32 sbchars_n /*strlen_with,without_NULLCH*/)
{
    return(quick_ublock_sbchars_add(p_quick_ublock, sbstr, strlen32_n(sbstr, sbchars_n)));
}

_Check_return_
extern STATUS
quick_ublock_sbchars_add_with_codepage(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(sbchars_n) PC_SBCHARS sbchars_in,
    _InVal_     U32 sbchars_n,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage)
{
    PC_SBCHARS sbchars = sbchars_in;
    BOOL is_pure_ascii7 = TRUE;
    U32 i;

    for(i = 0; i < sbchars_n; i++)
    {
        SBCHAR sbchar = *sbchars++;

        if(!ucs4_is_ascii7(sbchar))
        {
            is_pure_ascii7 = FALSE; /* conversion may be needed */
            --sbchars;
            break; /* leave i referring to the failed byte, p_u8 pointing at it */
        }
    }

    /* there probably is a run of pure ASCII-7 to start with (or full string!) */
    if(0 != i)
    {
        status_return(quick_ublock_uchars_add(p_quick_ublock, (PC_UCHARS) sbchars_in, i));
    }

    if(!is_pure_ascii7)
    {   /* need to convert the rest one at a time */
        for(; i < sbchars_n; i++)
        {
            SBCHAR sbchar = *sbchars++;
            UCS4 ucs4 = ucs4_from_sbchar_with_codepage(sbchar, sbchar_codepage);
            status_return(quick_ublock_ucs4_add(p_quick_ublock, ucs4));
        }
    }

#if CHECKING_UCHARS
    /* Check that whole quick_ublock is valid after appending converted string */
    status_return(uchars_inline_validate(TEXT("quick_ublock_sbchars_add_with_codepage ublock"), quick_ublock_uchars_inline(p_quick_ublock), quick_ublock_bytes(p_quick_ublock)));
#endif

    return(STATUS_OK);
}

/******************************************************************************
*
* add some TCHARs to a quick_ublock, converting to UCHARs from TCHARs
*
******************************************************************************/

_Check_return_
extern STATUS
quick_ublock_tchars_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(tchars_n) PCTCH tchars,
    _InVal_     U32 tchars_n)
{
#if TSTR_IS_SBSTR
    return(quick_ublock_sbchars_add(p_quick_ublock, (PC_SBCHARS) tchars, tchars_n));
#else /* TSTR_IS_SBSTR */
    return(quick_ublock_wchars_add(p_quick_ublock, tchars, tchars_n));
#endif /* TSTR_IS_SBSTR */
}

_Check_return_
extern STATUS
quick_ublock_tstr_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PCTSTR tstr)
{
    return(quick_ublock_tstr_add_n(p_quick_ublock, tstr, strlen_without_NULLCH));
}

_Check_return_
extern STATUS
quick_ublock_tstr_add_n(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PCTSTR tstr,
    _InVal_     U32 tchars_n /*strlen_with,without_NULLCH*/)
{
    return(quick_ublock_tchars_add(p_quick_ublock, tstr, tstrlen32_n(tstr, tchars_n)));
}

/******************************************************************************
*
* add some WCHARs to a quick_ublock, converting to UCHARs from WCHARs
*
* NB pwch may be unaligned e.g. in Excel file content
*
******************************************************************************/

_Check_return_
extern STATUS
quick_ublock_wchars_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(wchars_n) PCWCH pwch,
    _InVal_     U32 wchars_n)
{
#if TSTR_IS_SBSTR
    U32 offset = 0;

    assert(0 == (1 & (uintptr_t) pwch)); /* OK on x86 but not on ARM */

    assert(strlen_without_NULLCH > wchars_n);

    for(;;)
    {
        UCS4 ucs4;
        WCHAR ch;

        if(offset == wchars_n) /* adding 0 WCHARs is OK */
            break;

        ch = (WCHAR) readval_U16(pwch + offset); offset++;

        if(CH_NULL == ch)
            break; /* unexpected end of string */

        /* consider surrogate pair in UTF-16 */
        if(WCHAR_is_utf16_high_surrogate(ch))
        {
            WCHAR high_surrogate = ch;
            WCHAR low_surrogate;

            if(offset == wchars_n)
            {   /* no low surrogate available */
                assert0();
                break;
            }

            low_surrogate = (WCHAR) readval_U16(pwch + offset); offset++;
            assert(WCHAR_is_utf16_low_surrogate(low_surrogate));

            ucs4 = utf16_char_decode_surrogates(high_surrogate, low_surrogate);
        }
        else
        {
            assert(!WCHAR_is_utf16_low_surrogate(ch));
            ucs4 = ch;
        }

        if(status_fail(ucs4_validate(ucs4)))
        {
            assert(status_ok(ucs4_validate(ucs4)));
            ucs4 = CH_QUESTION_MARK;
        }

#if USTR_IS_SBSTR
        if(!ucs4_is_sbchar(ucs4))
        {   /* out-of-range -> substitute */
            assert(ucs4_is_sbchar(ucs4));
            status_return(quick_ublock_a7char_add(p_quick_ublock, CH_QUESTION_MARK));
            continue;
        }
#endif

        status_return(quick_ublock_ucs4_add(p_quick_ublock, ucs4));
    }

    return(STATUS_OK);

#else /* NOT TSTR_IS_SBSTR */

#if USTR_IS_SBSTR
    const UINT mbchars_CodePage = GetACP();
    BOOL fUsedDefaultChar = FALSE;
    P_BOOL p_fUsedDefaultChar = &fUsedDefaultChar;
#else
    const UINT mbchars_CodePage = CP_UTF8;
    P_BOOL p_fUsedDefaultChar = NULL;
#endif
    STATUS status = STATUS_OK;
    P_UCHARS dstptr;
    U32 avail;
    int multi_n;

    assert(strlen_without_NULLCH > wchars_n);

    if(0 == wchars_n)
        return(STATUS_OK);

    avail = p_quick_ublock->ub_static_buffer_size - p_quick_ublock->ub_static_buffer_used;

    /* try reading into static buffer if there is some (sufficient?) space there*/
    if(avail >= wchars_n /* NB this can only be approximate! */)
    {
        dstptr = uchars_AddBytes_wr(p_quick_ublock->ub_p_static_buffer, p_quick_ublock->ub_static_buffer_used);

#if WINDOWS
        multi_n =
            WideCharToMultiByte(mbchars_CodePage, 0 /*dwFlags*/,
                                pwch, (int) wchars_n,
                                (PSTR) dstptr, (int) avail,
                                NULL /*lpDefaultChar*/, p_fUsedDefaultChar);
#if USTR_IS_SBSTR
        assert(!fUsedDefaultChar);
#endif
#endif

        if(multi_n > 0)
        {   /* converted fully into static buffer, mark as used */
            p_quick_ublock->ub_static_buffer_used += multi_n;
            return(STATUS_OK);
        }

#if WINDOWS
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return(STATUS_FAIL);
#endif
    }

    /* either no static buffer or insufficient space there */

    /* count number of bytes needed */
#if WINDOWS
    multi_n =
        WideCharToMultiByte(mbchars_CodePage, 0 /*dwFlags*/,
                            pwch, (int) wchars_n,
                            NULL, 0,
                            NULL /*lpDefaultChar*/, p_fUsedDefaultChar);
#if USTR_IS_SBSTR
    assert(!fUsedDefaultChar);
#endif
#endif /* OS */

    if(0 == multi_n)
    {   /* nothing to add */
        return(STATUS_OK);
    }

    /* add the requisite number of bytes to the quick_ublock */
    avail = multi_n;
    dstptr = quick_ublock_extend_by(p_quick_ublock, multi_n, &status);

    if(NULL != dstptr)
    {
#if WINDOWS
        multi_n =
            WideCharToMultiByte(mbchars_CodePage, 0 /*dwFlags*/,
                                pwch, (int) wchars_n,
                                (PSTR) dstptr, (int) avail,
                                NULL /*lpDefaultChar*/, p_fUsedDefaultChar);
#if USTR_IS_SBSTR
        assert(!fUsedDefaultChar);
#endif
#endif /* OS */

        if(0 == multi_n)
            return(STATUS_FAIL);

#if CHECKING_UCHARS
        /* Check that whole quick_ublock is valid after appending converted string */
        status_return(uchars_inline_validate(TEXT("quick_ublock_wchars_add ublock"), quick_ublock_uchars_inline(p_quick_ublock), quick_ublock_bytes(p_quick_ublock)));
#endif
    }

    return(STATUS_OK);

#endif /* TSTR_IS_SBSTR */
}

#if USTR_IS_SBSTR

/******************************************************************************
*
* add a UCS-4 character to a quick_ublock, converting from UCS4 to UCHAR
*
* NB if out-of-range, insert the corresponding IL_UTF8 instead,
* which not all callers of quick_ublock_ucs4_add() can cope with,
* just the ones where it ends up in a Fireworkz text object
*
******************************************************************************/

_Check_return_
static inline STATUS /* size out */
inline_quick_ublock_IL_UTF8(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     UCS4 ucs4)
{
    UTF8B utf8_buffer[8];
    U32 bytes_of_char = utf8_char_encode(utf8_bptr(utf8_buffer), elemof32(utf8_buffer), ucs4);
    return(inline_quick_ublock_from_data(p_quick_ublock, IL_UTF8, IL_TYPE_ANY, utf8_buffer, bytes_of_char));
}

_Check_return_
extern STATUS
quick_ublock_ucs4_add_aiu(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     UCS4 ucs4)
{
    if(ucs4_is_sbchar(ucs4))
    {
        UCHARB uchar_buffer;

        uchar_buffer = (UCHARB) (ucs4 & 0xFF);

        if(_quick_ublock_uchar1_add_fast(p_quick_ublock, uchar_buffer))
            return(STATUS_OK);

        return(quick_ublock_uchars_add_slow(p_quick_ublock, uchars_bptr(&uchar_buffer), 1));
    }

#if USTR_IS_SBSTR /* try hard not to make inlines */
    {
    UCS4 ucs4_try = ucs4_to_sbchar_try_with_codepage(ucs4, get_system_codepage());

    if(ucs4_is_sbchar(ucs4_try))
        return(quick_ublock_ucs4_add_aiu(p_quick_ublock, ucs4_try));
    } /*block*/
#endif

    /* out-of-range -> substitute with IL_UTF8 */
    return(inline_quick_ublock_IL_UTF8(p_quick_ublock, ucs4));
}

#endif /* USTR_IS_SBSTR */

/* end of quickublk.c */
