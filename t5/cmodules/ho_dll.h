/* ho_dll.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#if defined(LOADS_CODE_MODULES)

#if RISCOS

/* Structure used when loading a module into memory.
 * This defines the entry and code areas taken up by the loading.
 */

typedef struct RUNTIME_INFO
{
    void (__cdecl * p_module_entry) (void);

    S32 code_size;
    P_ANY p_code;

    S32 reloc_size;
    void * reloc_flex; /* flex_alloc'ed */
}
RUNTIME_INFO, * P_RUNTIME_INFO;

typedef struct STUB_DESC
{
    P_ANY p_stub_data;
}
STUB_DESC, * P_STUB_DESC;

_Check_return_
extern STATUS
host_load_module(
    _In_z_      PCTSTR filename,
    P_RUNTIME_INFO p_runtime_info,
    _InVal_     S32 host_version,
    _In_        P_ANY p_stub_table);

_Check_return_
extern STATUS
host_load_stubs(
    _In_z_      PCTSTR filename,
    _OutRef_    P_STUB_DESC p_stub_desc);

#elif WINDOWS

typedef struct RUNTIME_INFO
{
    void (__cdecl * p_module_entry) (void);

    OBJECT_ID object_id;
}
RUNTIME_INFO, * P_RUNTIME_INFO;

_Check_return_
extern STATUS
host_load_module(
    _In_z_      PCTSTR filename,
    P_RUNTIME_INFO p_runtime_info,
    _InVal_     OBJECT_ID object_id);

#endif /* OS */

extern void
host_discard_module(
    P_RUNTIME_INFO p_runtime_info);

#endif /* defined(LOADS_CODE_MODULES) */

/* end of ho_dll.h */
