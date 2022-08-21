/* windows/ho_dll.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* David De Vorchik (diz) 9-Dec-93 */

#include "common/gflags.h"

#if WINDOWS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/ho_dll.h"

#if defined(LOADS_CODE_MODULES)

#define PROC_EVENT_ORDINAL 0x0002

/* Flags and instance information used to inform the world about
 * loaded objects and modules
 */

static HINSTANCE dll_instances[MAX_OBJECTS];

static BITMAP(dlls_loaded_at_runtime, MAX_OBJECTS);

static void
host_discard_loaded_modules(void);

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_dll);

_Check_return_
static STATUS
ho_dll_msg_exit2(void)
{
    host_discard_loaded_modules();

    return(STATUS_OK);
}

_Check_return_
static STATUS
maeve_services_ho_dll_msg_initclose(
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose)
{
    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__EXIT2:
        return(ho_dll_msg_exit2());

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_dll)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_ho_dll_msg_initclose((PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/* Load a module: this is quite a simple process under Windows as they are
 * simple DLLs.  The 2nd ordinal function is the entry point used
 * by the app to pass main events through.  This code simply loads
 * the library and then sets up the structure to contain something
 * sensible.
 *
 * A bitmap is maintained that describes which objects have been
 * loaded to allow them to be diposed of.
 */

_Check_return_
extern STATUS
host_load_module(
    _In_z_      PCTSTR filename,
    P_RUNTIME_INFO p_runtime_info,
    _InVal_     OBJECT_ID object_id)
{
    zero_struct_ptr(p_runtime_info);

    if(!IS_OBJECT_ID_VALID(object_id))
        return(status_check());

    p_runtime_info->object_id = object_id;

    /* Load the library and find the entry */
    dll_instances[p_runtime_info->object_id] = LoadLibrary(filename);
    if(dll_instances[p_runtime_info->object_id] <= (HINSTANCE) (UINT_PTR) HINSTANCE_ERROR)
    {
        dll_instances[p_runtime_info->object_id] = NULL;   /* just-in-case */
        return(STATUS_MODULE_NOT_FOUND);
    }

    __pragma(warning(push)) __pragma(warning(disable:4191)) /* 'type cast' : unsafe conversion from 'FARPROC' to 'P_PROC_EVENT' */
    p_runtime_info->p_module_entry = (void (__cdecl *)(void))
        GetProcAddress(dll_instances[p_runtime_info->object_id], (LPCSTR) (UINT_PTR) (0x0000000UL | PROC_EVENT_ORDINAL));
    __pragma(warning(pop))
    if(NULL == p_runtime_info->p_module_entry)
        return(STATUS_MODULE_NOT_FOUND);

    /* Record the object information, and the fact it was loaded */
    bitmap_bit_set(dlls_loaded_at_runtime, p_runtime_info->object_id, N_BITS_ARG(MAX_OBJECTS));

    return(STATUS_OK);
}

/* Discard a loaded object given its runtime info structure.
 */

extern void
host_discard_module(
    P_RUNTIME_INFO p_runtime_info)
{
    if(bitmap_bit_test(dlls_loaded_at_runtime, p_runtime_info->object_id, N_BITS_ARG(MAX_OBJECTS)))
    {
        FreeLibrary(dll_instances[p_runtime_info->object_id]);
        bitmap_bit_clear(dlls_loaded_at_runtime, p_runtime_info->object_id, N_BITS_ARG(MAX_OBJECTS));
    }

    return;
}

/* This code will discard all installed DLLs within the system.
 * It should be called during one of the exit handlers for the application.
 *
 * This is done by stepping through the loaded modules bitmap,
 * if a bit is set then the relevant handle is released.
 */

static void
host_discard_loaded_modules(void)
{
    OBJECT_ID object_id;

    for(object_id = OBJECT_ID_FIRST; object_id < OBJECT_ID_MAX; OBJECT_ID_INCR(object_id))
    {
        if(bitmap_bit_test(dlls_loaded_at_runtime, object_id, N_BITS_ARG(MAX_OBJECTS)))
        {
            bitmap_bit_clear(dlls_loaded_at_runtime, object_id, N_BITS_ARG(MAX_OBJECTS));
            FreeLibrary(dll_instances[object_id]);
            dll_instances[object_id] = NULL;
        }
    }
}

#else /*defined(LOADS_CODE_MODULES) */
__pragma(warning(disable:4206)) /* nonstandard extension used : translation unit is empty */
#endif /*defined(LOADS_CODE_MODULES) */

#endif /* WINDOWS */

/* end of windows/ho_dll.c */
