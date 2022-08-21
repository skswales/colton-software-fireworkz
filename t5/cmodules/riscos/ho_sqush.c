/* riscos/ho_sqush.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Simplistic interfaces to host specific decompression functions */

/* David De Vorchik 14 jul 93 */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/riscos/ho_sqush.h"

#undef malloc
extern void *
malloc(
    size_t size);

#undef free
extern void
free(
    void * ptr);

#define EXPOSE_RISCOS_FLEX 1

#include "ob_skel/xp_skelr.h"

/* Guard word used to flag squashed resources */

#define _SQUASH_GUARD_WORD 0x48535153

/*
what the file contains
*/

typedef struct SQUASH_HEADER
{
    U32 guard_word;
    U32 expanded_size;
    U32 load_address;
    U32 exec_address;
    U32 reserved;
}
SQUASH_HEADER, * P_SQUASH_HEADER;

typedef void (* P_FREE_PROC) (
    P_ANY core);

typedef struct SQUASH_ALLOC_BLOCK
{
    P_FREE_PROC p_proc_free;
}
SQUASH_ALLOC_BLOCK, * P_SQUASH_ALLOC_BLOCK;

/* -------------------------------------------------------------------------
 * Given a file handle and the amount to be expanded and the expected size
 * then read the data into the flex area and expand it.
 * The file pointer will have been advanced.
 * ------------------------------------------------------------------------- */

_Check_return_
extern STATUS
host_squash_expand_from_file(
    _Out_writes_bytes_(expand_size) P_ANY p_any /*filled*/,
    _InoutRef_  FILE_HANDLE file_handle,
    _InVal_     U32 compress_size,
    _InVal_     U32 expand_size)
{
    STATUS status;
    void * compressed_data  = NULL;
    void * squash_workspace = NULL;
    _kernel_swi_regs rs;

    for(;;)
    {
        rs.r[0] = 0x8;

        if(NULL != _kernel_swi(/*Squash_Decompress*/ 0x042701, &rs, &rs))
        {
            status = STATUS_FAIL;
            break;
        }

        if(!flex_alloc(&squash_workspace, rs.r[0]))
        {
            status = status_nomem();
            break;
        }

        if(!flex_alloc(&compressed_data, (int) compress_size))
        {
            status = status_nomem();
            break;
        }

        status_break(status = file_read_bytes_requested(compressed_data, compress_size, file_handle));

        rs.r[0] = 0;
        rs.r[1] = (int) squash_workspace;
        rs.r[2] = (int) compressed_data;
        rs.r[3] = (int) compress_size;
        rs.r[4] = (int) p_any;
        rs.r[5] = (int) expand_size;

        if((NULL != _kernel_swi(/*Squash_Decompress*/ 0x042701, &rs, &rs)) || (rs.r[3] != 0))
        {
            status = STATUS_FAIL;
            break;
        }

        status = STATUS_OK;

        break;
        /*NOTREACHED*/
    }

    flex_dispose(&compressed_data);
    flex_dispose(&squash_workspace);

    return(status);
}

/* -------------------------------------------------------------------------
 * Load an entire file into memory expanding it if required.
 * This code checks to see if the file is bigger than a squash header, if it
 * is then it loads the header and checks to see if the file is squashed (guard word valid).
 *
 * If the file is squashed then the compressed copy is loaded into a flex
 * block and the squash module is granted some workspace (in another flex block).
 *
 * We then allocate enough store for the original data and expand it
 * out into there, tidying and returning a suitable status code.
 *
 * If the file is not compressed then it is just loaded directly into memory.
 * If the call returns a failure then the buffer will have been disposed of.
 * ------------------------------------------------------------------------- */

_Check_return_
extern STATUS
host_squash_load_data_file(
    /*out*/ P_P_ANY p_p_any,
    _InoutRef_  FILE_HANDLE file_handle)
{
    STATUS status;
    filelength_t filelength;
    S32 length;
    SQUASH_HEADER squash_header;

    status_return(file_length(file_handle, &filelength));

    assert(0 == filelength.u.words.hi);
    assert((S32) filelength.u.words.lo >= 0);
    length = (S32) filelength.u.words.lo;

    if(length > sizeof32(SQUASH_HEADER))
    {
        status_return(file_read_bytes_requested(&squash_header, sizeof32(SQUASH_HEADER), file_handle));

        if(squash_header.guard_word == _SQUASH_GUARD_WORD)
        {
            const U32 expand_size = squash_header.expanded_size;

            if(status_ok(status = host_squash_alloc(p_p_any, expand_size, FALSE)))
            {
                length -= sizeof32(SQUASH_HEADER);

                if(status_fail(status = host_squash_expand_from_file(*p_p_any, file_handle, length, expand_size)))
                    host_squash_dispose(p_p_any);
            }

            return(status);
        }

        status_return(file_rewind(file_handle));
    }

    status_return(host_squash_alloc(p_p_any, length, FALSE));

    if(status_fail(status = file_read_bytes_requested(*p_p_any, length, file_handle)))
    {
        host_squash_dispose(p_p_any);
        status = STATUS_FAIL;
    }
    else
        status = STATUS_OK;

    return(status);
}

_Check_return_
extern STATUS
host_squash_alloc(
    /*out*/ P_P_ANY p_p_any,
    _InVal_     U32 size,
    _InVal_     BOOL may_be_code)
{
    STATUS status = STATUS_OK;
    P_SQUASH_ALLOC_BLOCK p_squash_alloc_block;
    P_FREE_PROC p_proc_free = NULL;
    U32 alloced_size = size + sizeof32(*p_squash_alloc_block);

    if(may_be_code && (0 != alloc_dynamic_area_query()))
    {
        /* allocate from low heap as we can't execute code up there in the dynamic areas */
        if(NULL != (p_squash_alloc_block = malloc(alloced_size)))
            p_proc_free = free;
        else
            status = status_nomem();
    }
    else
    {
        if(NULL != (p_squash_alloc_block = al_ptr_alloc_bytes(P_SQUASH_ALLOC_BLOCK, alloced_size, &status)))
            p_proc_free = al_ptr_free;
    }

    if(NULL != p_squash_alloc_block)
    {
        p_squash_alloc_block->p_proc_free = p_proc_free;
        p_squash_alloc_block += 1;
        *p_p_any = p_squash_alloc_block;
    }

    return(status);
}

extern void
host_squash_dispose(
    /*inout*/ P_P_ANY p_p_any)
{
    P_SQUASH_ALLOC_BLOCK p_squash_alloc_block = *p_p_any;

    *p_p_any = NULL;

    if(NULL != p_squash_alloc_block)
    {
        p_squash_alloc_block -= 1;
        (* p_squash_alloc_block->p_proc_free) (p_squash_alloc_block);
    }
}

#endif /* RISCOS */

/* end of riscos/ho_sqsh.c */
