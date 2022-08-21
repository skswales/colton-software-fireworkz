/* riscos/ho_dll.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Dynamic Linking for Fireworkz - load a module and relocate it */

/* David De Vorchik 21-Jun-1993 */

/* -------------------------------------------------------------------------
 * 22-jun-93 diz Brused and abused into the skeleton
 * 14 jul 93 diz Made it use file_ stuff
 * 14-jul-93 diz Made it check for and load squashed module chunks
 * 12-aug-93 diz Added maeve handler to release memory on exit
 * 12-aug-93 diz Stub loading moved to this handler, passing NULL for stub table makes it use the global one
 * ------------------------------------------------------------------------- */

#include "common/gflags.h"

#if RISCOS

#if defined(LOADS_CODE_MODULES)

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/ho_dll.h"

#include "cmodules/riscos/ho_sqush.h"

#define EXPOSE_RISCOS_FLEX 1

#include "ob_skel/xp_skelr.h"

/*
internal structure
*/

/* Guard word that prefixs the module to ensure that we are reading
 * something semi-sensible.
 */

#define _MODULE_GUARD_WORD 0x50465350

/* The relocation section of the file contains lots of these
 * entries, these are used for fixing up the runtime
 * address and sorting out all external references.
 */

typedef struct OB_MODULE_RELOC
{
    S32 offset;                                 /* to perform relocation at within block */
    S32 value;                                  /* either PC relative or absolute */

    UBF relocation_type : 4;                    /* type of relocation required */
    UBF external_reference : 1;                 /* reference to an externally declared function */
    UBF PC_relative : 1;                        /* PC relative value (i.e. instruction to be relocated) */
    UBF internal_reloc : 1;                     /* add area base to value for non PC relative relocations */

    UBF absolute_value : 1;                     /* value is a constant and should not be relocated on loading */
}
OB_MODULE_RELOC, * P_OB_MODULE_RELOC;

enum RELOC_TYPES
{
    RELOC_TYPE_BYTE,                            /* byte value to be modified */
    RELOC_TYPE_WORD,                            /* word value (16 bits) to be modified */
    RELOC_TYPE_DOUBLE                           /* double word (32 bits) to be modified */
};

/* Define the chunk information, its type, compressed size
 * and expanded size.  The squashed bit allows for compressed
 * chunks within the file
 */

typedef struct OB_CHUNK_HEADER
{
    S32 type;                                   /* is it code, data, relocation */
    S32 size;                                   /* size of area within file */
    S32 expanded_size;                          /* size when expanded (if compressed) */

    UBF squashed_area : 1;                      /* area is squashed using the 'Squash' module */
}
OB_CHUNK_HEADER, * P_OB_CHUNK_HEADER;

enum AOF_AREA_TYPES
{
    _AREA_CODE,
    _AREA_RELOC
};

/* Module header, this prefixes the module and defines
 * the version information required, along with a
 * suitable guard word
 */

typedef struct OB_MODULE_HEADER
{
    U32 guard_word;
    S32 skeleton_version;
    S32 module_version;
    S32 module_id;
    U32 entry_offset;
}
OB_MODULE_HEADER, * P_OB_MODULE_HEADER;

/* The stubs file contains n entries as defined below.
 * Each entry has a value field, but is only valid if the undefined bit is zero.
 */

typedef struct OB_STUB
{
    S32 value;
    UBF absolute  : 1;                          /* =0 -> address, else constant value */
    UBF undefined : 1;
}
OB_STUB, * P_OB_STUB;

static STUB_DESC
global_stubs;

static PTSTR
global_stubs_filename;

_Check_return_
static STATUS
global_stubs_ensure_loaded(void);

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_dll);

_Check_return_
static STATUS
ho_dll_msg_startup(void)
{
    STATUS status = STATUS_OK;
    TCHARZ filename[BUF_MAX_PATHSTRING];

    /* Only try to load the Stubs once */
    if(status_done(file_find_on_path(filename, elemof32(filename), file_get_resources_path(), TEXT("Stubs"))))
        if(status_ok(tstr_set(&global_stubs_filename, filename))) /* NB. not status = !!! */
            status = global_stubs_ensure_loaded();

    return(status);
}

_Check_return_
static STATUS
ho_dll_msg_exit2(void)
{
    host_squash_dispose(&global_stubs.p_stub_data);

    tstr_clr(&global_stubs_filename);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_ho_dll_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(ho_dll_msg_startup());

    case T5_MSG_IC__EXIT2:
        return(ho_dll_msg_exit2());

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_dll)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_ho_dll_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/*
Load in the stubs table filling in the structure passed to contain a
suitable array handle.  This needs to be done before any stubs are loaded.
*/

_Check_return_
extern STATUS
host_load_stubs(
    _In_z_      PCTSTR filename,
    _OutRef_    P_STUB_DESC p_stub_desc)
{
    FILE_HANDLE stub_file_handle;
    STATUS status;

    zero_struct_ptr(p_stub_desc);

    if(status_done(status = t5_file_open(filename, file_open_read, &stub_file_handle, FALSE)))
    {
        status = host_squash_load_data_file(&p_stub_desc->p_stub_data, stub_file_handle);

        status_accumulate(status, t5_file_close(&stub_file_handle));
    }

    return(status);
}

_Check_return_
static STATUS
global_stubs_ensure_loaded(void)
{
    if(global_stubs.p_stub_data)
        return(STATUS_OK);

    if(NULL == global_stubs_filename)
        return(create_error(ERR_NO_STUBS));

    return(host_load_stubs(global_stubs_filename, &global_stubs));
}

/* -------------------------------------------------------------------------
 * Attempt to load a code module into memory and relocate it.  The code
 * handles decoding the headers, decompessing the areas of code and
 * general fix up of the environment around it.
 *
 * If the caller passes NULL for a ob_stub table then the table is assumed
 * to be the global one and that will be references as required.
 * ------------------------------------------------------------------------- */

_Check_return_
extern STATUS
host_load_module(
    _In_z_      PCTSTR filename,
    P_RUNTIME_INFO p_runtime_info,
    _InVal_     S32 host_version,
    P_ANY p_stub_table_in)
{
    P_OB_STUB p_stub_table = p_stub_table_in;
    FILE_HANDLE module_file;
    OB_MODULE_HEADER module_header;
    OB_CHUNK_HEADER chunk_header;
    STATUS status = STATUS_OK;

    if(NULL == p_stub_table)
       if(NULL == (p_stub_table = global_stubs.p_stub_data))
            return(create_error(ERR_NO_STUBS));

    zero_struct_ptr(p_runtime_info);

    status_return(t5_file_open(filename, file_open_read, &module_file, TRUE));

    if(status_fail(status = file_read_bytes_requested(&module_header, sizeof32(OB_MODULE_HEADER), module_file)))
    { /* EMPTY */ }
    else if(module_header.guard_word != _MODULE_GUARD_WORD)
        status = STATUS_NOT_A_MODULE; /* guard word does not contain a sensible value */
    else if(host_version < module_header.skeleton_version)
        status = STATUS_BAD_VERSION; /* skeleton is too old for this module */
    else
        while(status == STATUS_OK)
        {
            U32 bytesread;

            if(status_ok(status = file_read_bytes(&chunk_header, sizeof32(OB_CHUNK_HEADER), &bytesread, module_file)) && (bytesread == sizeof32(OB_CHUNK_HEADER)))
            {
                switch(chunk_header.type)
                {
                case _AREA_CODE:
                    {
                    p_runtime_info->code_size = chunk_header.expanded_size;

                    if(chunk_header.squashed_area)
                    {
                        status_break(status = host_squash_alloc(&p_runtime_info->p_code, chunk_header.expanded_size, TRUE));

                        if(status_fail(status = ensure_memory_froth()))
                            host_squash_dispose(&p_runtime_info->p_code);
                        else
                            status = host_squash_expand_from_file(p_runtime_info->p_code, module_file, chunk_header.size, chunk_header.expanded_size);
                    }
                    else
                    {
                        status_break(status = host_squash_alloc(&p_runtime_info->p_code, chunk_header.size, TRUE));

                        /* SKS after 1.20/50 - try to ensure loaded code will be able to initialise properly */
                        if(status_fail(status = ensure_memory_froth()))
                            host_squash_dispose(&p_runtime_info->p_code);
                        else if(status_ok(status = file_read_bytes_requested(p_runtime_info->p_code, chunk_header.size, module_file)))
                            status = STATUS_OK;
                    }

                    break;
                    }

                case _AREA_RELOC:
                    {
                    p_runtime_info->reloc_size = chunk_header.expanded_size;

                    if(!flex_alloc(&p_runtime_info->reloc_flex, (int) chunk_header.size))
                    {
                        status = status_nomem();
                    }
                    else if(status_ok(status = file_read_bytes_requested(p_runtime_info->reloc_flex, chunk_header.size, module_file)))
                    {
                        status = STATUS_OK;

                        if(chunk_header.squashed_area)
                        {
                            void * squash_workspace = NULL;
                            void * expansion_space  = NULL;

                            for(;;)
                            {
                                _kernel_swi_regs rs;

                                rs.r[0] = 0x8;

                                if(NULL != _kernel_swi(/*Squash_Decompress*/ 0x042701, &rs, &rs))
                                {
                                    status = create_error_from_kernel_oserror(_kernel_last_oserror());
                                    break;
                                }

                                if(!flex_alloc(&squash_workspace, rs.r[0]))
                                {
                                    status = status_nomem();
                                    break;
                                }

                                if(!flex_alloc(&expansion_space, (int) chunk_header.expanded_size))
                                {
                                    status = status_nomem();
                                    break;
                                }

                                rs.r[0] = 0;
                                rs.r[1] = (int) squash_workspace;
                                rs.r[2] = (int) p_runtime_info->reloc_flex;
                                rs.r[3] = (int) chunk_header.size;
                                rs.r[4] = (int) expansion_space;
                                rs.r[5] = (int) chunk_header.expanded_size;

                                if(NULL != _kernel_swi(/*Squash_Decompress*/ 0x042701, &rs, &rs))
                                {
                                    status = create_error_from_kernel_oserror(_kernel_last_oserror());
                                    break;
                                }

                                if(rs.r[3] == 0x0)
                                {
                                    flex_dispose(&p_runtime_info->reloc_flex);
                                    flex_give_away(&p_runtime_info->reloc_flex, &expansion_space);
                                    break;
                                }

                                status = STATUS_FAIL;
                                break;
                            }

                            flex_dispose(&squash_workspace);
                            flex_dispose(&expansion_space);
                        }
                    }

                    break;
                    }

                default:
                    status = STATUS_BAD_CHUNK;
                    break;
                }
            }
            else
            {
                if(status_ok(status))
                {
                    if(file_eof(module_file))
                        break; /* no error 'cos end of file when trying to read in new header */

                    status = FILE_ERR_CANTREADREQUESTED; /* failed to read in chunk header (no eof, just didn't read it all!) */
                }
            }
        }

    if((status == STATUS_OK) && (NULL != p_runtime_info->p_code) && (p_runtime_info->reloc_size != 0))
    {
        P_U8 p_code = p_runtime_info->p_code;
        P_OB_MODULE_RELOC p_reloc = (P_OB_MODULE_RELOC) p_runtime_info->reloc_flex;
        S32 relocations = p_runtime_info->reloc_size / sizeof32(OB_MODULE_RELOC);

        while((relocations-- > 0) && (status == STATUS_OK))
        {
            P_U8 address = p_code + p_reloc->offset;
            S32 value = p_reloc->value;

            if(p_reloc->external_reference)
            {
                P_OB_STUB p_stub_reference = p_stub_table +value;
                value = p_stub_reference->value;
            }
            else if(!p_reloc->absolute_value)
                value += (S32) p_code;

            if(p_reloc->PC_relative)
            {
                P_U32 p_instruction = (P_U32) address;
                U32 instruction = *p_instruction;

                switch(instruction & 0x0F000000)
                {
                case 0x0A000000: /* case for B and BL instruction */
                case 0x0B000000:
                  value -= (U32) (p_instruction +2);
                  value  = (instruction & 0xFF000000) | ((value >>2) & 0x00FFFFFF);
                  *p_instruction = value;
                  break;

                default:
                  status = STATUS_BAD_INSTRUCT;
                  break;
                }
            }
            else if(p_reloc->external_reference)
            {
                switch(p_reloc->relocation_type)
                {
                case RELOC_TYPE_BYTE:
                    {
                    P_U8 p_8value = (P_U8) address;
                    *p_8value = (U8) (*p_8value + value);
                    break;
                    }

                case RELOC_TYPE_WORD:
                    {
                    P_U16 p_16value = (P_U16) address;
                    *p_16value = (U16) (*p_16value + value);
                    break;
                    }

                case RELOC_TYPE_DOUBLE:
                    {
                    P_U32 p_32value = (P_U32) address;
                    *p_32value = (U32) (*p_32value + value);
                    break;
                    }
                }
            }
            else if(p_reloc->internal_reloc == 0)
            {
                switch(p_reloc->relocation_type)
                {
                case RELOC_TYPE_BYTE:
                    {
                    P_U8 p_8value = (P_U8) address;
                    *p_8value = (U8) (*p_8value + (U32) p_code);
                    break;
                    }

                case RELOC_TYPE_WORD:
                    {
                    P_U16 p_16value = (P_U16) address;
                    *p_16value = (U16) (*p_16value + (U32) p_code);
                    break;
                    }

                case RELOC_TYPE_DOUBLE:
                    {
                    P_U32 p_32value = (P_U32) address;
                    *p_32value = (U32) (*p_32value + (U32) p_code);
                    break;
                    }
                }
            }
            else
            {
                switch(p_reloc->relocation_type)
                {
                case RELOC_TYPE_BYTE:
                    {
                    P_U8 p_8value = (P_U8) address;
                    *p_8value = (U8) (*p_8value + value);
                    break;
                    }

                case RELOC_TYPE_WORD:
                    {
                    P_U16 p_16value = (P_U16) address;
                    *p_16value = (U16) (*p_16value + value);
                    break;
                    }

                case RELOC_TYPE_DOUBLE:
                    {
                    P_U32 p_32value = (P_U32) address;
                    *p_32value = (U32) (*p_32value + value);
                    break;
                    }
                }
            }

              p_reloc++;
          }

        if(host_platform_features_query() & PLATFEAT_SYNCH_CODE_AREAS)
        {
            /* flush StrongARM cache on this area */
            _kernel_swi_regs rs;
            rs.r[0] = 1; /* Address range to be synchronised */
            rs.r[1] = (int) p_runtime_info->p_code;
            rs.r[2] = (int) p_runtime_info->p_code + (int) p_runtime_info->code_size;
            rs.r[2] = ((rs.r[2] + 3) & ~3) - 4; /* inclusive address required, so round up then lose a word */
            WrapOsErrorReporting(_kernel_swi(OS_SynchroniseCodeAreas, &rs, &rs));
        }

        if(module_header.entry_offset != 0)
        {
            union keep_stuart_happy
            {
                PC_BYTE pc_byte;
                void (__cdecl * p_proc) (void);
            } keep_stuart_happy;

            keep_stuart_happy.pc_byte = PtrAddBytes(PC_BYTE, p_runtime_info->p_code, module_header.entry_offset);
            p_runtime_info->p_module_entry = keep_stuart_happy.p_proc;
        }
    }
    else if(status == STATUS_OK)
        status = STATUS_NOT_ALL_LOADED;

    status_assert(t5_file_close(&module_file));

    flex_dispose(&p_runtime_info->reloc_flex);

    if(status != STATUS_OK)
        host_discard_module(p_runtime_info);

    return(status);
}

/* -------------------------------------------------------------------------
 * Discard a loaded module.  This requires the runtime description
 * block to be passed.
 * ------------------------------------------------------------------------- */

extern void
host_discard_module(
    P_RUNTIME_INFO p_runtime_info)
{
    host_squash_dispose(&p_runtime_info->p_code);
}

#else /* defined(LOADS_CODE_MODULES) */
__pragma(warning(disable:4206)) /* nonstandard extension used : translation unit is empty */
#endif /* defined(LOADS_CODE_MODULES) */

#endif /* RISCOS */

/* end of riscos/ho_dll.c */
