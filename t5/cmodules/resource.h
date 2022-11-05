/* resource.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS April 1992 */

#ifndef __resource_h
#define __resource_h

typedef struct BOUND_RESOURCES
{

#if RISCOS

#define RESOURCE_DLL_SUFFIX /*nothing*/

    P_ANY sprite_area_c; /* sprite_area * */
    P_ANY sprite_area_11;
    P_ANY sprite_area_22;
    P_ANY sprite_area_24;

#elif WINDOWS

/* all resources (per object) can be lumped into one file on Windows */

#define RESOURCE_DLL_SUFFIX TEXT(".dll")

    P_ANY really_abstract;

#endif /* OS */

}
BOUND_RESOURCES, * P_BOUND_RESOURCES; typedef const BOUND_RESOURCES * PC_BOUND_RESOURCES;

#define LOAD_RESOURCES ((P_BOUND_RESOURCES) (intptr_t) 1)
#define DONT_LOAD_RESOURCES NULL

#define LOAD_MESSAGES_FILE ((P_PC_U8) (intptr_t) 1)
#define DONT_LOAD_MESSAGES_FILE NULL

typedef union RESOURCE_BITMAP_HANDLE
{
#if RISCOS
    int i;
    P_SCB p_scb;
    P_U8 p_u8;
#elif WINDOWS
    HBITMAP i;
#endif /* OS */
}
RESOURCE_BITMAP_HANDLE, * P_RESOURCE_BITMAP_HANDLE;

#if RISCOS
#define RESOURCE_BITMAP_HANDLE_RISCOS_BODGE 0x00000001 /* knowing sprite_ptr is always word aligned */
#endif

/*
*/

typedef struct RESOURCE_BITMAP_ID
{
    OBJECT_ID object_id;
#if RISCOS
    PC_SBSTR bitmap_name; /* [12] including CH_NULL terminator for RISC OS: native, can't be UTF-8 encoded */
#elif WINDOWS
    UINT bitmap_id;
#endif
}
RESOURCE_BITMAP_ID, * P_RESOURCE_BITMAP_ID, ** P_P_RESOURCE_BITMAP_ID; typedef const RESOURCE_BITMAP_ID * PC_RESOURCE_BITMAP_ID;

typedef U32 T5_RESOURCE_BITMAP_ID_PACKED;

#define T5_RESOURCE_COMMON_BMP_BIT 0x8000
/* if resource_id & T5_RESOURCE_COMMON_BMP_BIT then use other top 6 bits as bitmap index, bottom byte as bitmap id */
/* otherwise all bits are bitmap id */
#define T5_RESOURCE_COMMON_BMP(id, index) ( \
    T5_RESOURCE_COMMON_BMP_BIT | ((index) << 8) | (id) )

/* use this to declare a multi-bitmap for 'ALL_IN_ONE' scheme
 * NB both object_signifier & id limited to 0..15
 */
#define T5_RESOURCE_COMMON_BMP_BASE(object_signifier, id) ( \
    ((object_signifier) << 4) | (id) )

#define RESOURCE_BITMAP_AREA_STANDARD 0
#define RESOURCE_BITMAP_AREA_LO_RES 1
#define RESOURCE_BITMAP_AREA_HI_RES 2
#define RESOURCE_BITMAP_AREA_UHI_RES 3
#define RESOURCE_BITMAP_AREA_COUNT 4

#define RESOURCE_BITMAP_AREA_ID int

_Check_return_
extern RESOURCE_BITMAP_HANDLE
resource_bitmap_find(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id);

#if WINDOWS

_Check_return_
extern BOOL
resource_bitmap_find_new(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id,
    _OutRef_    P_RESOURCE_BITMAP_HANDLE p_resource_bitmap_handle,
    _OutRef_    P_GDI_SIZE p_bm_grid_size,
    _OutRef_    PUINT p_index);

#endif /* WINDOWS */

_Check_return_
extern RESOURCE_BITMAP_HANDLE
resource_bitmap_find_in_area(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id,
    _InVal_     S32 area_id);

#if RISCOS

_Check_return_
extern RESOURCE_BITMAP_HANDLE
resource_bitmap_find_defaulting(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id);

_Check_return_
extern RESOURCE_BITMAP_HANDLE
resource_bitmap_find_system(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id);

#endif /* RISCOS */

#if WINDOWS

_Check_return_
extern STATUS /* STATUS_DONE and size set iff found, or STATUS_OK and zero size if not */
resource_bitmap_tool_size_query(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id,
    _OutRef_    P_GDI_SIZE p_size);

#endif /* WINDOWS */

_Check_return_
extern STATUS
resource_bitmap_tool_size_register(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id,
    _In_        int cx,
    _In_        int cy);

extern void
resource_bitmap_lose(
    /*inout*/ P_RESOURCE_BITMAP_HANDLE p_resource_bitmap_handle);

extern void
resource_bitmap_gdi_size_query(
    RESOURCE_BITMAP_HANDLE resource_bitmap_handle,
    _OutRef_    P_GDI_SIZE p_size);

_Check_return_
extern STATUS
resource_close(
    _InVal_     OBJECT_ID object_id);

_Check_return_
extern STATUS
resource_dll_find(
    _InVal_     OBJECT_ID object_id,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer);

extern void
resource_dll_free(
    _InVal_     OBJECT_ID object_id);

extern void
resource_donate(
    _InVal_     OBJECT_ID object_id_dst,
    _InVal_     OBJECT_ID object_id_src);

_Check_return_
extern STATUS
resource_init(
    _InVal_     OBJECT_ID object_id,
    _In_opt_    PC_U8 * const p_u8_bound_msg,
    _InRef_opt_ PC_BOUND_RESOURCES p_bound_resources);

#if RISCOS

_Check_return_
extern STATUS
resource_load_messages(
    _InVal_     OBJECT_ID object_id);

_Check_return_
extern STATUS
resource_load_sprites(
    _InVal_     OBJECT_ID object_id,
    _In_        UINT which);

_Check_return_
extern STATUS
resource_load_appropriate_sprites(
    _InVal_     OBJECT_ID object_id);

#endif

/*ncr*/
extern BOOL
resource_split_status(
    _In_        STATUS status,
    _OutRef_    P_U16 p_offset,
    _OutRef_    P_OBJECT_ID p_object_id);

extern void
resource_shutdown(void);

extern void
resource_startup(void);

#if WINDOWS

_Check_return_
extern HINSTANCE
resource_get_object_resources(
    _InVal_     OBJECT_ID object_id,
    _Out_       HINSTANCE * const p_hInstance_fallback);

#endif

#if RISCOS

extern void
riscos_filesprite(
    /*_Out_z_cap_c_(elemof_buffer)*/ P_SBSTR sbstr_buf /*filled*/,
    _InVal_     U32 elemof_buffer,
    _InVal_     T5_FILETYPE t5_filetype);

#endif /* OS */

_Check_return_
_Ret_z_
extern PC_USTR
resource_lookup_ustr(
    _InVal_     STATUS status);

extern void
resource_lookup_ustr_buffer(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     STATUS status);

_Check_return_
_Ret_maybenull_z_
extern PC_USTR
resource_lookup_ustr_no_default(
    _InVal_     STATUS status);

_Check_return_
extern STATUS
resource_lookup_quick_ublock(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     STATUS status);

_Check_return_
extern STATUS
resource_lookup_quick_ublock_no_default(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     STATUS status);

_Check_return_
_Ret_z_
extern PTSTR
resource_lookup_tstr(
    _InVal_     STATUS status);

extern void
resource_lookup_tstr_buffer(
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     STATUS status);

_Check_return_
_Ret_maybenull_z_
extern PTSTR
resource_lookup_tstr_no_default(
    _InVal_     STATUS status);

_Check_return_
extern STATUS
resource_lookup_quick_tblock(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InVal_     STATUS status);

_Check_return_
extern STATUS
resource_lookup_quick_tblock_no_default(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InVal_     STATUS status);

#endif /* __resource_h */

/* end of resource.h */
