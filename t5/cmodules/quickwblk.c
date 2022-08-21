/* quickwblk.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/******************************************************************************
*
* quick_block allocation module (WCHAR-based)
*
* efficient routines for handling temporary buffers
*
* you supply a 'static' buffer; if the item to be added to the buffer
* would overflow the supplied buffer, a handle-based buffer is allocated
*
* SKS Jul 2014 derived from quicktblk.c
*
******************************************************************************/

#include "common/gflags.h"

#ifndef          __utf16_h
#include "cmodules/utf16.h"
#endif

#ifndef          __quickwblk_h
#include "cmodules/quickwblk.h"
#endif

/******************************************************************************
*
* WCHAR-based quick block
*
******************************************************************************/

/* dispose of a quick_wblock is inline */

/******************************************************************************
*
* dispose of a quick_wblock of data but leave as much text in place as possible (see charts and xvtsnprintf)
*
******************************************************************************/

extern void
quick_wblock_dispose_leaving_buffer_valid(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*disposed*/)
{
    /* have we overflowed the buffer and gone into handle allocation? */
    if(0 != quick_wblock_array_handle_ref(p_quick_wblock))
    {   /* copy as much stuff as possible back down before deleting the handle */
        const U32 wchars_n = MIN(p_quick_wblock->wb_static_buffer_elem, array_elements32(&quick_wblock_array_handle_ref(p_quick_wblock)));
        PCWCH p_data = array_rangec(&quick_wblock_array_handle_ref(p_quick_wblock), WCHAR, 0, wchars_n);

        memcpy32(p_quick_wblock->wb_p_static_buffer, p_data, wchars_n * sizeof32(WCHAR));

        al_array_dispose(&quick_wblock_array_handle_ref(p_quick_wblock));
    }

    p_quick_wblock->wb_static_buffer_used = 0;
}

/******************************************************************************
*
* empty a quick_wblock of data for efficient reuse
*
******************************************************************************/

extern void
quick_wblock_empty(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*emptied*/)
{
    if(0 != quick_wblock_array_handle_ref(p_quick_wblock))
    {
#if CHECKING_QUICK_WBLOCK
        /* trash buffer on empty when CHECKING_QUICK_WBLOCK */
        const U32 wchars_n = array_elements32(&quick_wblock_array_handle_ref(p_quick_wblock));
        _do_wqb_fill(array_range(&quick_wblock_array_handle_ref(p_quick_wblock), WCHAR, 0, wchars_n), wchars_n);
#endif
        al_array_empty(&quick_wblock_array_handle_ref(p_quick_wblock));
    }

    /* trash buffer on empty when CHECKING_QUICK_WBLOCK */
    _do_wqb_fill(p_quick_wblock->wb_p_static_buffer, p_quick_wblock->wb_static_buffer_elem);
    p_quick_wblock->wb_static_buffer_used = 0;
}

/******************************************************************************
*
* efficient routine for allocating temporary buffers of WCHAR
*
* you supply a buffer; if the item is bigger than this,
* an indirect buffer is allocated
*
* remember to call quick_wblock_dispose afterwards!
*
******************************************************************************/

_Check_return_
_Ret_writes_maybenull_(extend_by) /* may be NULL */
extern PWCH
quick_wblock_extend_by(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*extended*/,
    _InVal_     U32 extend_by,
    _OutRef_    P_STATUS p_status)
{
    ARRAY_INIT_BLOCK array_init_block;
    S32 size_increment;
    PWCH p_output;

    *p_status = STATUS_OK;

    if(0 == extend_by) /* realloc by 0 is OK, but return a non-NULL rubbish pointer */
        return((PWCH) (uintptr_t) 2);

    assert(extend_by < 0xF0000000U); /* real world use always +ve; check possible -ve client */
    assert(extend_by < 0x80000000U); /* sanity for debug */

#if 0
    if((S32) extend_by < 0)
    {
        quick_wblock_shrink_by(p_quick_wblock, (S32) extend_by);
        return((PWCH) (uintptr_t) 2);
    }
#endif

    /* does the request fit in the current buffer? */
    if(extend_by <= (p_quick_wblock->wb_static_buffer_elem - p_quick_wblock->wb_static_buffer_used))
    {
        p_output = p_quick_wblock->wb_p_static_buffer + p_quick_wblock->wb_static_buffer_used;
        p_quick_wblock->wb_static_buffer_used += extend_by;
        return(p_output);
    }

    if(0 != quick_wblock_array_handle_ref(p_quick_wblock))
        return(al_array_extend_by_WCHAR(&quick_wblock_array_handle_ref(p_quick_wblock), extend_by, PC_ARRAY_INIT_BLOCK_NONE, p_status));

    /* transition from static buffer to array handle */
    size_increment = p_quick_wblock->wb_static_buffer_elem >> 2; /* / 4 */

    if( size_increment < 4)
        size_increment = 4;

    array_init_block_setup(&array_init_block, size_increment, sizeof32(WCHAR), FALSE);

    if(NULL != (p_output = al_array_alloc_WCHAR(&quick_wblock_array_handle_ref(p_quick_wblock),
                                                p_quick_wblock->wb_static_buffer_used + extend_by, &array_init_block, p_status)))
    {
        memcpy32(p_output, p_quick_wblock->wb_p_static_buffer, p_quick_wblock->wb_static_buffer_used * sizeof32(WCHAR));

        p_output += p_quick_wblock->wb_static_buffer_used;

        p_quick_wblock->wb_static_buffer_used = p_quick_wblock->wb_static_buffer_elem; /* indicate static buffer full for fast add etc. */

        /* trash buffer once copied when CHECKING_QUICK_WBLOCK */
        _do_wqb_fill(p_quick_wblock->wb_p_static_buffer, p_quick_wblock->wb_static_buffer_elem);
    }

    return(p_output);
}

extern void
quick_wblock_shrink_by(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*shrunk*/,
    _InVal_     S32 shrink_by)
{
    U32 shrink_by_pos;

    assert(shrink_by <= 0);

    if(shrink_by >= 0)
        return;

    if(0 != quick_wblock_array_handle_ref(p_quick_wblock))
    {
        al_array_shrink_by(&quick_wblock_array_handle_ref(p_quick_wblock), shrink_by);
        return;
    }

    shrink_by_pos = (U32) -(shrink_by);

    if(p_quick_wblock->wb_static_buffer_used >= shrink_by_pos)
    {
        p_quick_wblock->wb_static_buffer_used -= shrink_by_pos;
        /* trash invalidated section when CHECKING_QUICK_WBLOCK */
        _do_wqb_fill(p_quick_wblock->wb_p_static_buffer + p_quick_wblock->wb_static_buffer_used, shrink_by_pos);
    }
    else
    {
        assert(p_quick_wblock->wb_static_buffer_used >= shrink_by_pos);
        p_quick_wblock->wb_static_buffer_used = 0;
        /* trash invalidated section when CHECKING_QUICK_WBLOCK */
        _do_wqb_fill(p_quick_wblock->wb_p_static_buffer, p_quick_wblock->wb_static_buffer_elem);
    }
}

/******************************************************************************
*
* add a WCHAR to a quick_wblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_wblock_wchar_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _InVal_     WCHAR wchar)
{
    STATUS status;
    PWCH pwch;

    if(NULL != (pwch = quick_wblock_extend_by(p_quick_wblock, 1, &status)))
        *pwch = wchar;

    return(STATUS_OK);
}

/******************************************************************************
*
* add some WCHARs to a quick_wblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_wblock_wchars_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_reads_(wchars_n) PCWCH pwch_in,
    _InVal_     U32 wchars_n)
{
    if(0 != wchars_n) /* adding 0 WCHARs is OK */
    {
        STATUS status;
        PWCH pwch_out;

        if(NULL == (pwch_out = quick_wblock_extend_by(p_quick_wblock, wchars_n, &status)))
            return(status);

        memcpy32(pwch_out, pwch_in, wchars_n * sizeof32(*pwch_in)); /* OK if pwch_in is unaligned */
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* add a WCHARZ string to a quick_wblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_wblock_wstr_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PCWSTR wstr)
{
    return(quick_wblock_wstr_add_n(p_quick_wblock, wstr, strlen_without_NULLCH));
}

_Check_return_
extern STATUS
quick_wblock_wstr_add_n(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PCWSTR wstr,
    _InVal_     U32 wchars_n /*strlen_with,without_NULLCH*/)
{
    return(quick_wblock_wchars_add(p_quick_wblock, wstr, wstrlen32_n(wstr, wchars_n)));
}

/******************************************************************************
*
* CH_NULL terminate the given quick_wblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_wblock_nullch_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/)
{
    return(quick_wblock_wchar_add(p_quick_wblock, CH_NULL));
}

/******************************************************************************
*
* ensure that the given quick_wblock is not CH_NULL-terminated
*
******************************************************************************/

extern void
quick_wblock_nullch_strip(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*stripped*/)
{
    const U32 wchars_n = quick_wblock_chars(p_quick_wblock);

    if(0 != wchars_n)
    {
        PCWCH pwch = quick_wblock_wchars(p_quick_wblock);

        if(CH_NULL == pwch[wchars_n-1])
            quick_wblock_shrink_by(p_quick_wblock, -1);
    }
}

/******************************************************************************
*
* sprintf into quick_wblock
*
* NB. does NOT add terminating CH_NULL
*
******************************************************************************/

_Check_return_
extern STATUS __cdecl
quick_wblock_printf(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_ _Printf_format_string_ PCWSTR format,
    /**/        ...)
{
    va_list args;
    STATUS status;

    va_start(args, format);
    status = quick_wblock_vprintf(p_quick_wblock, format, args);
    va_end(args);

    return(status);
}

/******************************************************************************
*
* vsprintf into quick_wblock
*
* NB. does NOT add terminating CH_NULL
*
******************************************************************************/

_Check_return_
extern STATUS
quick_wblock_vprintf(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_ _Printf_format_string_ PCWSTR format,
    /**/        va_list args)
{
    PCWSTR wstr;
    STATUS status = STATUS_OK;
#if WINDOWS
    WCHAR preceding;
    WCHAR conversion;
#endif

    assert(0 == (1 & (uintptr_t) format)); /* OK on x86 but not on ARM */

    /* loop finding format specifications */
    while(NULL != (wstr = wstrchr(format, CH_PERCENT_SIGN)))
    {
        /* can't use L"" as wchar_t is int on RISC OS */
        const WCHAR conversion_chars[] = { 'd', 'i', 'o', 'u', 'x', 'X', 'c', 's', 'f', 'e', 'E', 'g', 'G', 'p', 'n', 'C', 'S', CH_NULL }; /*L"diouxXcsfeEgGpnCS"*/

        /* output what we have so far */
        if(wstr - format)
            status_break(status = quick_wblock_wchars_add(p_quick_wblock, format, PtrDiffElemU32(wstr, format)));

        format = wstr + 1; /* skip the % */

        if(CH_PERCENT_SIGN == *format)
        {
            status_break(status = quick_wblock_wchar_add(p_quick_wblock, CH_PERCENT_SIGN));
            format++; /* skip the escaped % too */
            continue;
        }

        /* skip goop till conversion found */
        if(NULL == (wstr = wstrpbrk(format, conversion_chars)))
        {
            assert0();
            break;
        }

        /* skip over conversion */
#if WINDOWS
        preceding = wstr[-1];
        conversion = *wstr++;
#else
        wstr++;
#endif

        {
#if WINDOWS
        PCWSTR begin = format - 1;
        WCHARZ buffer[256]; /* hopefully big enough for a single conversion */
        WCHARZ buffer_format[32];
        S32 len;

        wstr_xstrnkpy(buffer_format, elemof32(buffer_format), begin, PtrDiffElemU32(wstr, begin));

        len = _vsnwprintf_s(buffer, elemof32(buffer), _TRUNCATE, buffer_format, args /* NOT updated - see below */);
        if(-1 == len)
            len = wstrlen32(buffer); /* limit transfer to what actually was achieved */

        status_break(status = quick_wblock_wchars_add(p_quick_wblock, buffer, len));
#elif 0
        assert0(); /* no implementation */
        buffer[0] = CH_NULL;
        len = 0;
#else /* C99 CRT */
        PCWSTR begin = format - 1;
        UCHARZ buffer[256]; /* hopefully big enough for a single conversion */
        UCHARZ buffer_format[32];
        S32 len;

        xstrnkpy(buffer_format, elemof32(buffer_format), _tstr_from_wstr(begin), PtrDiffElemU32(wstr, begin));

        len = vsnprintf(buffer, elemof32(buffer), buffer_format, args /* will be updated accordingly */);
        if(len >= elemof32(buffer))
            len = strlen32(buffer); /* limit transfer to what actually was achieved */

        status_break(status = quick_wblock_sbchars_add(p_quick_wblock, buffer, len));
#endif

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
                volatile long double ld = va_arg(args, long double); UNREFERENCED_LOCAL_VARIABLE(ld);
            }
            else /* NB floats are promoted to double when passed to variadic functions */
            {
                volatile double d = va_arg(args, double); UNREFERENCED_LOCAL_VARIABLE(d);
            }
            break;

        case 's':
        case 'S':
        case 'p':
        case 'n':
            {
            volatile char * p = va_arg(args, char *); UNREFERENCED_LOCAL_VARIABLE(p);
            break;
            }

        case 'c':
        case 'C':
            { /* chars are widened to int when passed as parameters */
            volatile int ci = va_arg(args, int); UNREFERENCED_LOCAL_VARIABLE(ci);
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
                volatile long li = va_arg(args, long); UNREFERENCED_LOCAL_VARIABLE(li);
            }
            else /* NB shorts are promoted to int when passed to variadic functions */
            {
                volatile int i = va_arg(args, int); UNREFERENCED_LOCAL_VARIABLE(i);
            }
            break;
        }
#endif /* WINDOWS */
        } /*block*/

        format = wstr; /* skip whole conversion sequence */
    }

    /* output trailing fragment */
    if(*format && status_ok(status))
        status = quick_wblock_wstr_add(p_quick_wblock, format);

    return(status);
}

/* appending with conversion */

/******************************************************************************
*
* add a UCS-4 character encoded as WCHAR(s) to a quick_wblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_wblock_ucs4_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _InVal_     UCS4 ucs4)
{
    WCHAR buffer[2];
    U32 wchars_n;

    if(ucs4 <= U16_MAX)
    {
        buffer[0] = (WCHAR) ucs4;
        wchars_n = 1;
    }
    else
    {
        const U32 n_bytes = utf16_char_encode(buffer, /*NB*/ sizeof32(buffer), ucs4);
        wchars_n = n_bytes / sizeof32(WCHAR);
    }

    return(quick_wblock_wchars_add(p_quick_wblock, buffer, wchars_n));
}

/******************************************************************************
*
* add some WCHARs to a quick_wblock, converting from SBCHARs
*
******************************************************************************/

_Check_return_
extern STATUS
quick_wblock_sbchars_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_reads_(sbchars_n) PC_SBCHARS sbchars,
    _InVal_     U32 sbchars_n)
{
    if(0 != sbchars_n) /* adding 0 SBCHARs is OK */
    {
        STATUS status;
        PWCH pwch_out;
        U32 offset;

        if(NULL == (pwch_out = quick_wblock_extend_by(p_quick_wblock, sbchars_n * sizeof32(WCHAR), &status)))
            return(status);

        for(offset = 0; offset < sbchars_n; ++offset)
            pwch_out[offset] = sbchars[offset];
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
quick_wblock_sbstr_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PC_SBSTR sbstr)
{
    return(quick_wblock_sbstr_add_n(p_quick_wblock, sbstr, strlen_without_NULLCH));
}

_Check_return_
extern STATUS
quick_wblock_sbstr_add_n(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PC_SBSTR sbstr,
    _InVal_     U32 sbchars_n /*strlen_with,without_NULLCH*/)
{
    return(quick_wblock_sbchars_add(p_quick_wblock, sbstr, strlen32_n(sbstr, sbchars_n)));
}

/******************************************************************************
*
* add some WCHARs to a quick_wblock, converting from UCHARs
*
******************************************************************************/

_Check_return_
extern STATUS
quick_wblock_uchars_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n)
{
#if USTR_IS_SBSTR
    return(quick_wblock_sbchars_add(p_quick_wblock, uchars, uchars_n));
#else
    return(quick_wblock_utf8_add(p_quick_wblock, uchars, uchars_n));
#endif
}

_Check_return_
extern STATUS
quick_wblock_ustr_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PC_USTR ustr)
{
    return(quick_wblock_ustr_add_n(p_quick_wblock, ustr, strlen_without_NULLCH));
}

_Check_return_
extern STATUS
quick_wblock_ustr_add_n(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PC_USTR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/)
{
    return(quick_wblock_uchars_add(p_quick_wblock, ustr, ustrlen32_n(ustr, uchars_n)));
}

/******************************************************************************
*
* add some WCHARs to a quick_wblock, converting from UTF-8
*
******************************************************************************/

_Check_return_
extern STATUS
quick_wblock_utf8_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n)
{
    STATUS status = STATUS_OK;

    if(0 == uchars_n)
        return(STATUS_OK);

    assert(strlen_without_NULLCH > uchars_n);

#if CHECKING_UCHARS
    /* Check that string we are converting for append is valid */
    status_return(uchars_validate(TEXT("quick_wblock_utf8_add arg"), uchars, uchars_n));
#endif

#if TSTR_IS_SBSTR
    { /* manually widen */
    U32 offset = 0;
    while(offset < uchars_n)
    {
        U32 bytes_of_char;
        UCS4 ucs4 = utf8_char_decode_off(uchars, offset, bytes_of_char);
        status_return(status = quick_wblock_ucs4_add(p_quick_wblock, ucs4));
        offset += bytes_of_char;
    }
    } /*block*/
#elif WINDOWS
    {
    U32 avail;
    int wchars_n;
    PWSTR dstptr;

    avail = p_quick_wblock->wb_static_buffer_elem - p_quick_wblock->wb_static_buffer_used;

    /* try reading into static buffer if there is some (sufficient?) space there*/
    if(avail >= uchars_n /* NB this can only be approximate! */)
    {
        dstptr = &p_quick_wblock->wb_p_static_buffer[p_quick_wblock->wb_static_buffer_used];

        wchars_n =
            MultiByteToWideChar(CP_UTF8 /*SourceCodePage*/, 0 /*dwFlags*/,
                                (PCSTR) uchars, (int) uchars_n,
                                dstptr, (int) avail);

        if(wchars_n > 0)
        {   /* converted fully into static buffer, mark as used */
            p_quick_wblock->wb_static_buffer_used += wchars_n;
            return(STATUS_OK);
        }

        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return(STATUS_FAIL);
    }

    /* either no static buffer or insufficient space there */

    /* count number of WCHARs needed */
    wchars_n =
        MultiByteToWideChar(CP_UTF8 /*SourceCodePage*/, 0 /*dwFlags*/,
                            (PCSTR) uchars, (int) uchars_n,
                            NULL, 0);

    if(0 == wchars_n)
    {   /* nothing to add */
        return(STATUS_OK);
    }

    /* add the requisite number of WCHARs to the quick_wblock */
    avail = wchars_n;
    dstptr = quick_wblock_extend_by(p_quick_wblock, wchars_n, &status);

    if(NULL != dstptr)
    {
        wchars_n =
            MultiByteToWideChar(CP_UTF8 /*SourceCodePage*/, 0 /*dwFlags*/,
                                (PCSTR) uchars, (int) uchars_n,
                                dstptr, (int) avail);
        if(0 == wchars_n)
            return(STATUS_FAIL);
    }
    } /*block*/
#else
#error No implementation of quick_wblock_utf8_add
#endif /* OS */

    return(status);
}

_Check_return_
extern STATUS
quick_wblock_utf8str_add(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PC_UTF8STR ustr)
{
    return(quick_wblock_utf8str_add_n(p_quick_wblock, ustr, strlen_without_NULLCH));
}

_Check_return_
extern STATUS
quick_wblock_utf8str_add_n(
    _InoutRef_  P_QUICK_WBLOCK p_quick_wblock /*appended*/,
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/)
{
    return(quick_wblock_utf8_add(p_quick_wblock, ustr, utf8str_strlen32_n(ustr, uchars_n)));
}

/* end of quickwblk.c */
