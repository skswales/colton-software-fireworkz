/* quickblk.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/******************************************************************************
*
* quick_block allocation module (BYTE-based)
*
* efficient routines for handling temporary buffers
*
* you supply a 'static' buffer; if the item to be added to the buffer
* would overflow the supplied buffer, a handle-based buffer is allocated
*
* SKS November 2006 split out of aligator.c; May 2014 split off UCHARS/TCHARS
*
******************************************************************************/

#include "common/gflags.h"

#ifndef          __quickblk_h
#include "cmodules/quickblk.h"
#endif

/* dispose of a quick_block is inline */

/******************************************************************************
*
* dispose of a quick_block of data but leave as much text in place as possible (see xvsnprintf)
*
******************************************************************************/

extern void
quick_block_dispose_leaving_buffer_valid(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*disposed*/)
{
    /* have we overflowed the buffer and gone into handle allocation? */
    if(0 != quick_block_array_handle_ref(p_quick_block))
    {   /* copy as much stuff as possible back down before deleting the handle */
        const U32 n_bytes = MIN(p_quick_block->static_buffer_size, array_elements32(&quick_block_array_handle_ref(p_quick_block)));
        PC_BYTE p_data = array_rangec(&quick_block_array_handle_ref(p_quick_block), BYTE, 0, n_bytes);

        memcpy32(p_quick_block->p_static_buffer, p_data, n_bytes);

        al_array_dispose(&quick_block_array_handle_ref(p_quick_block));
    }

    p_quick_block->static_buffer_used = 0;
}

/******************************************************************************
*
* empty a quick_block of data for efficient reuse
*
******************************************************************************/

extern void
quick_block_empty(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*emptied*/)
{
    if(0 != quick_block_array_handle_ref(p_quick_block))
    {
#if CHECKING_QUICK_BLOCK
        /* trash handle on empty when CHECKING_QUICK_BLOCK */
        const U32 n_bytes = array_elements32(&quick_block_array_handle_ref(p_quick_block));
        _do_aqb_fill(array_range(&quick_block_array_handle_ref(p_quick_block), BYTE, 0, n_bytes), n_bytes);
#endif
        al_array_empty(&quick_block_array_handle_ref(p_quick_block));
    }

    /* trash buffer on empty when CHECKING_QUICK_BLOCK */
    _do_aqb_fill(p_quick_block->p_static_buffer, p_quick_block->static_buffer_size);
    p_quick_block->static_buffer_used = 0;
}

/******************************************************************************
*
* efficient routine for allocating temporary buffers
*
* you supply a buffer; if the item is bigger than this,
* an indirect buffer is allocated
*
* remember to call quick_block_dispose afterwards!
*
******************************************************************************/

_Check_return_
_Ret_writes_maybenull_(extend_by) /* may be NULL */
extern P_BYTE
quick_block_extend_by(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*extended*/,
    _InVal_     U32 extend_by,
    _OutRef_    P_STATUS p_status)
{
    ARRAY_INIT_BLOCK array_init_block;
    S32 size_increment;
    P_BYTE p_output;

    *p_status = STATUS_OK;

    if(0 == extend_by) /* realloc by 0 is OK, but return a non-NULL rubbish pointer */
        return((P_BYTE) (uintptr_t) 1);

    assert(extend_by < 0xF0000000U); /* real world use always +ve; check possible -ve client */
    assert(extend_by < 0x80000000U); /* sanity for debug */

#if 0
    if((S32) extend_by < 0)
    {
        quick_block_shrink_by(p_quick_block, (S32) extend_by);
        return((P_BYTE) (uintptr_t) 1);
    }
#endif

    /* does the request fit in the current buffer? */
    if(extend_by <= (p_quick_block->static_buffer_size - p_quick_block->static_buffer_used))
    {
        p_output = PtrAddBytes(P_BYTE, p_quick_block->p_static_buffer, p_quick_block->static_buffer_used);
        p_quick_block->static_buffer_used += extend_by;
        return(p_output);
    }

    if(0 != quick_block_array_handle_ref(p_quick_block))
        return(al_array_extend_by_BYTE(&quick_block_array_handle_ref(p_quick_block), extend_by, PC_ARRAY_INIT_BLOCK_NONE, p_status));

    /* transition from static buffer to array handle */
    size_increment = p_quick_block->static_buffer_size >> 2; /* / 4 */

    if( size_increment < 4)
        size_increment = 4;

    array_init_block_setup(&array_init_block, size_increment, sizeof32(BYTE), FALSE);

    if(NULL != (p_output = al_array_alloc_BYTE(&quick_block_array_handle_ref(p_quick_block),
                                               p_quick_block->static_buffer_used + extend_by, &array_init_block, p_status)))
    {
        memcpy32(p_output, p_quick_block->p_static_buffer, p_quick_block->static_buffer_used);

        p_output += p_quick_block->static_buffer_used;

        p_quick_block->static_buffer_used = p_quick_block->static_buffer_size; /* indicate static buffer full for fast add etc. */

        /* trash buffer once copied when CHECKING_QUICK_BLOCK */
        _do_aqb_fill(p_quick_block->p_static_buffer, p_quick_block->static_buffer_size);
    }

    return(p_output);
}

/******************************************************************************
*
* shrink quick_block by this many (NB -ve)
*
******************************************************************************/

extern void
quick_block_shrink_by(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*shrunk*/,
    _InVal_     S32 shrink_by)
{
    U32 shrink_by_pos;

    assert(shrink_by <= 0);

    if(shrink_by >= 0)
        return;

    if(0 != quick_block_array_handle_ref(p_quick_block))
    {
        al_array_shrink_by(&quick_block_array_handle_ref(p_quick_block), shrink_by);
        return;
    }

    shrink_by_pos = (U32) -(shrink_by);

    if(p_quick_block->static_buffer_used >= shrink_by_pos)
    {
        p_quick_block->static_buffer_used -= shrink_by_pos;
        /* trash invalidated section when CHECKING_QUICK_BLOCK */
        _do_aqb_fill(p_quick_block->p_static_buffer + p_quick_block->static_buffer_used, shrink_by_pos);
    }
    else
    {
        assert(p_quick_block->static_buffer_used >= shrink_by_pos);
        p_quick_block->static_buffer_used = 0;
        /* trash invalidated section when CHECKING_QUICK_BLOCK */
        _do_aqb_fill(p_quick_block->p_static_buffer, p_quick_block->static_buffer_size);
    }
}

/******************************************************************************
*
* add a byte to a quick_block
*
******************************************************************************/

_Check_return_
extern STATUS
quick_block_byte_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _InVal_     BYTE u8)
{
    STATUS status = STATUS_OK;
    P_BYTE p_byte;

    if(!quick_block_byte_add_fast(p_quick_block, u8))
        if(NULL != (p_byte = quick_block_extend_by(p_quick_block, 1, &status)))
            *p_byte = u8;

    return(status);
}

/******************************************************************************
*
* add some bytes to a quick_block
*
******************************************************************************/

_Check_return_
extern STATUS
quick_block_bytes_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _In_reads_bytes_(n_bytes) PC_ANY p_any,
    _InVal_     U32 n_bytes)
{
    if(0 != n_bytes) /* adding 0 bytes is OK */
    {
        STATUS status;
        P_BYTE p_byte;

        assert(n_bytes < 0xF0000000U); /* check possible -ve client */
        assert(n_bytes < 0x80000000U); /* sanity for debug */

        if(NULL == (p_byte = quick_block_extend_by(p_quick_block, n_bytes, &status)))
            return(status);

        memcpy32(p_byte, p_any, n_bytes);
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* CH_NULL terminate the given quick_block
*
******************************************************************************/

_Check_return_
extern STATUS
quick_block_nullch_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/)
{
    return(quick_block_byte_add(p_quick_block, CH_NULL));
}

/******************************************************************************
*
* ensure that the given quick_block is not CH_NULL-terminated
*
******************************************************************************/

extern void
quick_block_nullch_strip(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*stripped*/)
{
    const U32 n_bytes = quick_block_bytes(p_quick_block);

    if(0 != n_bytes)
    {
        PC_BYTE p_byte = quick_block_ptr_wr(p_quick_block);

        if(CH_NULL == p_byte[n_bytes-1])
            quick_block_shrink_by(p_quick_block, -1);
    }
}

/******************************************************************************
*
* sprintf into quick_block
*
* NB. does NOT add terminating CH_NULL
*
******************************************************************************/

_Check_return_
extern STATUS __cdecl
quick_block_printf(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _In_z_ _Printf_format_string_ PC_U8Z format,
    /**/        ...)
{
    va_list args;
    STATUS status;

    va_start(args, format);
    status = quick_block_vprintf(p_quick_block, format, args);
    va_end(args);

    return(status);
}

/******************************************************************************
*
* vsprintf into quick_block
*
* NB. does NOT add terminating CH_NULL
*
******************************************************************************/

_Check_return_
extern STATUS
quick_block_vprintf(
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended*/,
    _In_z_ _Printf_format_string_ PC_U8Z format,
    /**/        va_list args)
{
    PC_U8Z p_u8;
    STATUS status = STATUS_OK;
#if WINDOWS
    U8 preceding;
    U8 conversion;
#endif

    /* loop finding format specifications */
    while(NULL != (p_u8 = strchr(format, CH_PERCENT_SIGN)))
    {
        /* output what we have so far */
        if(p_u8 - format)
            status_break(status = quick_block_bytes_add(p_quick_block, format, PtrDiffBytesU32(p_u8, format)));

        format = p_u8 + 1; /* skip the % */

        if(CH_PERCENT_SIGN == *format)
        {
            status_break(status = quick_block_byte_add(p_quick_block, CH_PERCENT_SIGN));
            format++; /* skip the escaped % too */
            continue;
        }

        /* skip goop till conversion found */
        if(NULL == (p_u8 = strpbrk(format, "diouxXcsfeEgGpnCS")))
        {
            assert0();
            break;
        }

        /* skip over conversion */
#if WINDOWS
        preceding = p_u8[-1];
        conversion = *p_u8++;
#else
        p_u8++;
#endif

        {
        U8Z buffer[512]; /* hopefully big enough for a single conversion (was 256, but consider a f.p. number numform()ed to 308 digits...) */
        U8Z buffer_format[32];
        S32 len;

        xstrnkpy(buffer_format, elemof32(buffer_format), format - 1, PtrDiffBytesU32(p_u8, format) + 1);

#if WINDOWS
        len = _vsnprintf_s(buffer, elemof32(buffer), _TRUNCATE, buffer_format, args /* NOT updated - see below */);
        if(-1 == len)
            len = strlen32(buffer); /* limit transfer to what actually was achieved */
#else /* C99 CRT */
        len = vsnprintf(buffer, elemof32(buffer), buffer_format, args /* will be updated accordingly */);
        if(len >= elemof32(buffer))
            len = strlen32(buffer); /* limit transfer to what actually was achieved */
#endif

        status_break(status = quick_block_bytes_add(p_quick_block, buffer, len));

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
            else  /* NB floats are promoted to double when passed to variadic functions */
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

        format = p_u8; /* skip whole conversion sequence */
    }

    /* output trailing fragment */
    if(*format && status_ok(status))
        status = quick_block_bytes_add(p_quick_block, format, strlen32(format));

    return(status);
}

/* end of quickblk.c */
