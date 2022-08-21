/* quicktblk.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/******************************************************************************
*
* quick_block allocation module (TCHAR-based)
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

#ifndef          __utf16_h
#include "cmodules/utf16.h"
#endif

#ifndef          __quicktblk_h
#include "cmodules/quicktblk.h"
#endif

/* dispose of a quick_tblock is inline */

/******************************************************************************
*
* dispose of a quick_tblock of data but leave as much text in place as possible (see charts and xvtsnprintf)
*
******************************************************************************/

extern void
quick_tblock_dispose_leaving_buffer_valid(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*disposed*/)
{
    /* have we overflowed the buffer and gone into handle allocation? */
    if(0 != quick_tblock_array_handle_ref(p_quick_tblock))
    {   /* copy as much stuff as possible back down before deleting the handle */
        const U32 tchars_n = MIN(p_quick_tblock->tb_static_buffer_elem, array_elements32(&quick_tblock_array_handle_ref(p_quick_tblock)));
        PCTCH p_data = array_rangec(&quick_tblock_array_handle_ref(p_quick_tblock), TCHAR, 0, tchars_n);

        memcpy32(p_quick_tblock->tb_p_static_buffer, p_data, tchars_n * sizeof32(TCHAR));

        al_array_dispose(&quick_tblock_array_handle_ref(p_quick_tblock));
    }

    p_quick_tblock->tb_static_buffer_used = 0;
}

/******************************************************************************
*
* empty a quick_tblock of data for efficient reuse
*
******************************************************************************/

extern void
quick_tblock_empty(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*emptied*/)
{
    if(0 != quick_tblock_array_handle_ref(p_quick_tblock))
    {
#if CHECKING_QUICK_TBLOCK
        /* trash buffer on empty when CHECKING_QUICK_TBLOCK */
        const U32 tchars_n = array_elements32(&quick_tblock_array_handle_ref(p_quick_tblock));
        _do_tqb_fill(array_range(&quick_tblock_array_handle_ref(p_quick_tblock), TCHAR, 0, tchars_n), tchars_n);
#endif
        al_array_empty(&quick_tblock_array_handle_ref(p_quick_tblock));
    }

    /* trash buffer on empty when CHECKING_QUICK_TBLOCK */
    _do_tqb_fill(p_quick_tblock->tb_p_static_buffer, p_quick_tblock->tb_static_buffer_elem);
    p_quick_tblock->tb_static_buffer_used = 0;
}

/******************************************************************************
*
* efficient routine for allocating temporary buffers of TCHAR
*
* you supply a buffer; if the item is bigger than this,
* an indirect buffer is allocated
*
* remember to call quick_tblock_dispose afterwards!
*
******************************************************************************/

_Check_return_
_Ret_writes_maybenull_(extend_by) /* may be NULL */
extern PTCH
quick_tblock_extend_by(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*extended*/,
    _InVal_     U32 extend_by,
    _OutRef_    P_STATUS p_status)
{
    ARRAY_INIT_BLOCK array_init_block;
    S32 size_increment;
    PTCH p_output;

    *p_status = STATUS_OK;

    if(0 == extend_by) /* realloc by 0 is OK, but return a non-NULL rubbish pointer */
        return((PTCH) (uintptr_t) 1);

    assert(extend_by < 0xF0000000U); /* real world use always +ve; check possible -ve client */
    assert(extend_by < 0x80000000U); /* sanity for debug */

#if 0
    if((S32) extend_by < 0)
    {
        quick_tblock_shrink_by(p_quick_tblock, (S32) extend_by);
        return((PTCH) (uintptr_t) 1);
    }
#endif

    /* does the request fit in the current buffer? */
    if(extend_by <= (p_quick_tblock->tb_static_buffer_elem - p_quick_tblock->tb_static_buffer_used))
    {
        p_output = p_quick_tblock->tb_p_static_buffer + p_quick_tblock->tb_static_buffer_used;
        p_quick_tblock->tb_static_buffer_used += extend_by;
        return(p_output);
    }

    if(0 != quick_tblock_array_handle_ref(p_quick_tblock))
        return(al_array_extend_by_TCHAR(&quick_tblock_array_handle_ref(p_quick_tblock), extend_by, PC_ARRAY_INIT_BLOCK_NONE, p_status));

    /* transition from static buffer to array handle */
    size_increment = p_quick_tblock->tb_static_buffer_elem >> 2; /* / 4 */

    if( size_increment < 4)
        size_increment = 4;

    array_init_block_setup(&array_init_block, size_increment, sizeof32(TCHAR), FALSE);

    if(NULL != (p_output = al_array_alloc_TCHAR(&quick_tblock_array_handle_ref(p_quick_tblock),
                                                p_quick_tblock->tb_static_buffer_used + extend_by, &array_init_block, p_status)))
    {
        memcpy32(p_output, p_quick_tblock->tb_p_static_buffer, p_quick_tblock->tb_static_buffer_used * sizeof32(TCHAR));

        p_output += p_quick_tblock->tb_static_buffer_used;

        p_quick_tblock->tb_static_buffer_used = p_quick_tblock->tb_static_buffer_elem; /* indicate static buffer full for fast add etc. */

        /* trash buffer once copied when CHECKING_QUICK_TBLOCK */
        _do_tqb_fill(p_quick_tblock->tb_p_static_buffer, p_quick_tblock->tb_static_buffer_elem);
    }

    return(p_output);
}

extern void
quick_tblock_shrink_by(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*shrunk*/,
    _InVal_     S32 shrink_by)
{
    U32 shrink_by_pos;

    assert(shrink_by <= 0);

    if(shrink_by >= 0)
        return;

    if(0 != quick_tblock_array_handle_ref(p_quick_tblock))
    {
        al_array_shrink_by(&quick_tblock_array_handle_ref(p_quick_tblock), shrink_by);
        return;
    }

    shrink_by_pos = (U32) -(shrink_by);

    if(p_quick_tblock->tb_static_buffer_used >= shrink_by_pos)
    {
        p_quick_tblock->tb_static_buffer_used -= shrink_by_pos;
        /* trash invalidated section when CHECKING_QUICK_TBLOCK */
        _do_tqb_fill(p_quick_tblock->tb_p_static_buffer + p_quick_tblock->tb_static_buffer_used, shrink_by_pos);
    }
    else
    {
        assert(p_quick_tblock->tb_static_buffer_used >= shrink_by_pos);
        p_quick_tblock->tb_static_buffer_used = 0;
        /* trash invalidated section when CHECKING_QUICK_TBLOCK */
        _do_tqb_fill(p_quick_tblock->tb_p_static_buffer, p_quick_tblock->tb_static_buffer_elem);
    }
}

/******************************************************************************
*
* add a TCHAR to a quick_tblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_tblock_tchar_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InVal_     TCHAR tchar)
{
    STATUS status;
    PTCH ptch;

    if(NULL != (ptch = quick_tblock_extend_by(p_quick_tblock, 1, &status)))
        *ptch = tchar;

    return(STATUS_OK);
}

/******************************************************************************
*
* add some TCHARs to a quick_tblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_tblock_tchars_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_reads_(tchars_n) PCTCH ptch_in,
    _InVal_     U32 tchars_n)
{
    if(0 != tchars_n) /* adding 0 TCHARs is OK */
    {
        STATUS status;
        PTCH ptch_out;

        if(NULL == (ptch_out = quick_tblock_extend_by(p_quick_tblock, tchars_n, &status)))
            return(status);

        memcpy32(ptch_out, ptch_in, tchars_n * sizeof32(*ptch_in));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* add a TCHARZ string to a quick_tblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_tblock_tstr_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_      PCTSTR tstr)
{
    return(quick_tblock_tstr_add_n(p_quick_tblock, tstr, strlen_without_NULLCH));
}

_Check_return_
extern STATUS
quick_tblock_tstr_add_n(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_      PCTSTR tstr,
    _InVal_     U32 tchars_n /*strlen_with,without_NULLCH*/)
{
    return(quick_tblock_tchars_add(p_quick_tblock, tstr, tstrlen32_n(tstr, tchars_n)));
}

/******************************************************************************
*
* CH_NULL terminate the given quick_tblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_tblock_nullch_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/)
{
    return(quick_tblock_tchar_add(p_quick_tblock, CH_NULL));
}

/******************************************************************************
*
* ensure that the given quick_tblock is not CH_NULL-terminated
*
******************************************************************************/

extern void
quick_tblock_nullch_strip(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*stripped*/)
{
    const U32 tchars_n = quick_tblock_chars(p_quick_tblock);

    if(0 != tchars_n)
    {
        PCTCH ptch = quick_tblock_tchars(p_quick_tblock);

        if(CH_NULL == ptch[tchars_n-1])
            quick_tblock_shrink_by(p_quick_tblock, -1);
    }
}

/******************************************************************************
*
* sprintf into quick_tblock
*
* NB. does NOT add terminating CH_NULL
*
******************************************************************************/

_Check_return_
extern STATUS __cdecl
quick_tblock_printf(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...)
{
    va_list args;
    STATUS status;

    va_start(args, format);
    status = quick_tblock_vprintf(p_quick_tblock, format, args);
    va_end(args);

    return(status);
}

/******************************************************************************
*
* vsprintf into quick_tblock
*
* NB. does NOT add terminating CH_NULL
*
******************************************************************************/

_Check_return_
extern STATUS
quick_tblock_vprintf(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list args)
{
    PCTSTR tstr;
    STATUS status = STATUS_OK;
#if WINDOWS
    TCHAR preceding;
    TCHAR conversion;
#endif

    /* loop finding format specifications */
    while(NULL != (tstr = tstrchr(format, CH_PERCENT_SIGN)))
    {
        /* output what we have so far */
        if(tstr - format)
            status_break(status = quick_tblock_tchars_add(p_quick_tblock, format, PtrDiffElemU32(tstr, format)));

        format = tstr + 1; /* skip the % */

        if(CH_PERCENT_SIGN == *format)
        {
            status_break(status = quick_tblock_tchar_add(p_quick_tblock, CH_PERCENT_SIGN));
            format++; /* skip the escaped % too */
            continue;
        }

        /* skip goop till conversion found */
        if(NULL == (tstr = tstrpbrk(format, TEXT("diouxXcsfeEgGpnCS"))))
        {
            assert0();
            break;
        }

        /* skip over conversion */
#if WINDOWS
        preceding = tstr[-1];
        conversion = *tstr++;
#else
        tstr++;
#endif

        {
        TCHARZ buffer[256]; /* hopefully big enough for a single conversion */
        TCHARZ buffer_format[32];
        S32 len;

        tstr_xstrnkpy(buffer_format, elemof32(buffer_format), format - 1, PtrDiffElemU32(tstr, format) + 1);

#if WINDOWS
        len = _vsntprintf_s(buffer, elemof32(buffer), _TRUNCATE, buffer_format, args /* NOT updated - see below */);
        if(-1 == len)
            len = (S32) tstrlen32(buffer); /* limit transfer to what actually was achieved */
#else /* C99 CRT */
        len = vsnprintf(buffer, elemof32(buffer), buffer_format, args /* will be updated accordingly */);
        if( len >= (S32) elemof32(buffer) )
            len =  (S32) tstrlen32(buffer); /* limit transfer to what actually was achieved */
#endif

        status_break(status = quick_tblock_tchars_add(p_quick_tblock, buffer, len));

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
            { /* chars are promoted to int when passed as parameters */
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

        format = tstr; /* skip whole conversion sequence */
    }

    /* output trailing fragment */
    if(*format && status_ok(status))
        status = quick_tblock_tstr_add(p_quick_tblock, format);

    return(status);
}

/* appending with conversion */

/******************************************************************************
*
* add a UCS-4 character encoded as TCHAR(s) to a quick_tblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_tblock_ucs4_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InVal_     UCS4 ucs4)
{
    TCHAR buffer[2];
    U32 tchars_n;

#if TSTR_IS_SBSTR
    if(ucs4_is_sbchar(ucs4))
    {
        buffer[0] = (TCHAR) ucs4;
        tchars_n = 1;
    }
    else
    {
        assert0();
        tchars_n = 0;
    }
#else /* NOT TSTR_IS_SBSTR */
    if(ucs4 <= U16_MAX)
    {
        buffer[0] = (TCHAR) ucs4;
        tchars_n = 1;
    }
    else
    {
        const U32 n_bytes = utf16_char_encode(buffer, /*NB*/ sizeof32(buffer), ucs4);
        tchars_n = n_bytes / sizeof32(TCHAR);
    }
#endif /* TSTR_IS_SBSTR  */

    return(quick_tblock_tchars_add(p_quick_tblock, buffer, tchars_n));
}

/******************************************************************************
*
* add some TCHARs to a quick_tblock, converting from UCHARs
*
******************************************************************************/

_Check_return_
extern STATUS
quick_tblock_uchars_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n)
{
#if TSTR_IS_SBSTR && USTR_IS_SBSTR
    return(quick_tblock_tchars_add(p_quick_tblock, uchars, uchars_n));
#else
    return(quick_tblock_utf8_add(p_quick_tblock, uchars, uchars_n));
#endif
}

_Check_return_
extern STATUS
quick_tblock_ustr_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_      PC_USTR ustr)
{
    return(quick_tblock_ustr_add_n(p_quick_tblock, ustr, strlen_without_NULLCH));
}

_Check_return_
extern STATUS
quick_tblock_ustr_add_n(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_      PC_USTR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/)
{
    return(quick_tblock_uchars_add(p_quick_tblock, ustr, ustrlen32_n(ustr, uchars_n)));
}

/******************************************************************************
*
* add some TCHARs to a quick_tblock, converting from UTF-8
*
******************************************************************************/

_Check_return_
extern STATUS
quick_tblock_utf8_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_reads_(uchars_n) PC_UTF8 uchars,
    _InVal_     U32 uchars_n)
{
    STATUS status = STATUS_OK;

    if(0 == uchars_n)
        return(STATUS_OK);

    assert(strlen_without_NULLCH > uchars_n);

#if CHECKING_UCHARS
    /* Check that string we are converting for append is valid */
    status_return(uchars_validate(TEXT("quick_tblock_utf8_add arg"), uchars, uchars_n));
#endif

#if TSTR_IS_SBSTR
    { /* manually narrow to native codepage */
    BOOL is_pure_ascii7;
    U32 sbchars_n = sbchars_from_utf8_bytes_needed(uchars, uchars_n, &is_pure_ascii7);
    PTSTR dstptr = quick_tblock_extend_by(p_quick_tblock, sbchars_n, &status);

    if(NULL != dstptr)
        sbchars_from_utf8(dstptr, sbchars_n, get_system_codepage(), uchars, uchars_n);
    } /*block*/
#elif WINDOWS
    {
    U32 avail;
    int wchars_n;
    PTSTR dstptr;

    avail = p_quick_tblock->tb_static_buffer_elem - p_quick_tblock->tb_static_buffer_used;

    /* try reading into static buffer if there is some (sufficient?) space there*/
    if(avail >= uchars_n /* NB this can only be approximate! */)
    {
        dstptr = &p_quick_tblock->tb_p_static_buffer[p_quick_tblock->tb_static_buffer_used];

        wchars_n =
            MultiByteToWideChar(CP_UTF8 /*SourceCodePage*/, 0 /*dwFlags*/,
                                (PCSTR) uchars, (int) uchars_n,
                                dstptr, (int) avail);

        if(wchars_n > 0)
        {   /* converted fully into static buffer, mark as used */
            p_quick_tblock->tb_static_buffer_used += wchars_n;
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

    /* add the requisite number of WCHARs to the quick_tblock */
    avail = wchars_n;
    dstptr = quick_tblock_extend_by(p_quick_tblock, wchars_n, &status);

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
#error No implementation of quick_tblock_utf8_add
#endif /* TSTR_IS_SBSTR */

    return(status);
}

_Check_return_
extern STATUS
quick_tblock_utf8str_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_      PC_UTF8STR ustr)
{
    return(quick_tblock_utf8str_add_n(p_quick_tblock, ustr, strlen_without_NULLCH));
}

_Check_return_
extern STATUS
quick_tblock_utf8str_add_n(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _In_z_      PC_UTF8STR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/)
{
    return(quick_tblock_utf8_add(p_quick_tblock, ustr, utf8str_strlen32_n(ustr, uchars_n)));
}

/* end of quicktblk.c */
