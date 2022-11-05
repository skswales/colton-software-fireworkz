/* resource.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS April 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"
#endif

#if WINDOWS
#include "cmodules/collect.h"

#include "external/Microsoft/InsideOLE2/BTTNCURP/bttncur.h"
/* NB If this #include fails, try
 * Build\w32\setup.cmd
 */
#endif

/*
internal structure
*/

typedef struct RESOURCE_BITMAP_INDEX
{
    RESOURCE_BITMAP_ID bitmap_id;
    GDI_SIZE size;
}
RESOURCE_BITMAP_INDEX; typedef const RESOURCE_BITMAP_INDEX * PC_RESOURCE_BITMAP_INDEX;

/*
internal functions
*/

#if RISCOS

typedef struct RISCOS_MSGS_INDEX
{
    PC_U8Z tag;
    U32 tag_length;
}
RISCOS_MSGS_INDEX, * P_RISCOS_MSGS_INDEX; typedef const RISCOS_MSGS_INDEX * PC_RISCOS_MSGS_INDEX;

typedef struct RISCOS_MSGS
{
    PC_U8 msgs_block;
    PC_U8 lookup_ptr;
    P_RISCOS_MSGS_INDEX msgs_index;
    U32 n_msgs;
}
RISCOS_MSGS, * P_RISCOS_MSGS;

typedef struct RISCOS_LOADED_SPRITES
{
    UBF loaded_c  : 8;
    UBF loaded_11 : 8;
    UBF loaded_22 : 8;
    UBF loaded_24 : 8;
    P_SAH s_c;
    P_SAH s_11;
    P_SAH s_22;
    P_SAH s_24;
}
RISCOS_LOADED_SPRITES, * P_RISCOS_LOADED_SPRITES;

#endif /* RISCOS */

typedef struct RESOURCES_PER_OBJECT
{
    /* nothing host-independent here */
#if RISCOS
    struct RESOURCES_PER_OBJECT_RISCOS
    {
        RISCOS_MSGS mangled_msgs;
        RISCOS_LOADED_SPRITES sprites;
    } riscos;
#elif WINDOWS
    struct RESOURCES_PER_OBJECT_WINDOWS
    {
        LIST_BLOCK bitmap_cache;
#if defined(NOT_ALL_IN_ONE)
        HINSTANCE library_handle[MAX_OBJECTS];
#endif
    } windows;
#endif /* OS */
}
* P_RESOURCES_PER_OBJECT;

typedef struct RESOURCE_STATICS
{
    ARRAY_HANDLE bitmap_index;

    P_RESOURCES_PER_OBJECT objects[MAX_OBJECTS]; /* allocated as needed */

#if RISCOS && 0 /* now empty */
    struct RESOURCE_STATICS_WINDOWS
    {
        int foo;
    } riscos;
#elif WINDOWS
    struct RESOURCE_STATICS_WINDOWS
    {
        #if !defined(NOT_ALL_IN_ONE)
        HINSTANCE library_handle; /* used for UI translation */
        #endif
    } windows;
#endif /* OS */

    union RESOURCE_STATICS_DEFAULT_MSG
    {
        TCHARZ tcharz[256]; /* [] of TCHAR, CH_NULL-terminated */
        UCHARZ ucharz[256]; /* [] of U8, CH_NULL-terminated */
    } default_msg;
}
RESOURCE_STATICS;

static RESOURCE_STATICS resource_statics;

#if RISCOS

_Check_return_
_Ret_z_
static PCTSTR
riscos_msg_lookup_in_without_default(
    _In_reads_(tag_length) PCTSTR tag,
    _InVal_     U32 tag_length,
    /*inout*/ P_RISCOS_MSGS p_msgs);

_Check_return_
static STATUS
riscos_msg_init(
    _InRef_     P_RISCOS_MSGS p_msgs);

#endif /* RISCOS */

/******************************************************************************
*
* load a new copy of a named bitmap from resource - must be freed later
*
******************************************************************************/

#if RISCOS

_Check_return_
static int
spriteop_select_sprite(
    _In_opt_    P_SAH sprite_area,
    _In_z_      PC_U8Z sprite_name)
{
    _kernel_swi_regs  rs;
    _kernel_oserror * p_kernel_oserror;

    if(NULL == sprite_area)
        return(0);

    rs.r[0] = 0x100 | 24; /* Select sprite (in area) */
    rs.r[1] = (int) sprite_area;
    rs.r[2] = (int) sprite_name;

    if(NULL == (p_kernel_oserror = _kernel_swi(OS_SpriteOp, &rs, &rs)))
        return(rs.r[2]);

    return(0);
}

typedef P_SAH * P_P_SAH;

_Check_return_
static int
spriteop_select_sprite_in_areas(
    _In_reads_opt_(n_areas) P_P_SAH sprite_areas,
    _InVal_     U32 n_areas,
    _In_z_      PC_U8Z sprite_name)
{
    U32 area_idx;
    int result = 0;

    for(area_idx = 0; area_idx < n_areas; ++area_idx)
    {
        if(0 != (result = spriteop_select_sprite(sprite_areas[area_idx], sprite_name)))
            break;
    }

    return(result);
}

#endif

_Check_return_
static RESOURCE_BITMAP_HANDLE
resource_bitmap_find_scan_objects(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id)
{
    RESOURCE_BITMAP_ID resource_bitmap_id = *p_resource_bitmap_id;
    RESOURCE_BITMAP_HANDLE resource_bitmap_handle;

    resource_bitmap_handle.i = 0;

    while(status_ok(object_next(&resource_bitmap_id.object_id)))
    {
        resource_bitmap_handle = resource_bitmap_find(&resource_bitmap_id);

        if(0 != resource_bitmap_handle.i)
            break;
    }

    return(resource_bitmap_handle);
}

_Check_return_
extern RESOURCE_BITMAP_HANDLE
resource_bitmap_find(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id)
{
    RESOURCE_BITMAP_HANDLE resource_bitmap_handle;
    OBJECT_ID object_id;
    P_RESOURCES_PER_OBJECT p_resources_per_object;

    resource_bitmap_handle.i = 0;

    if(NULL == p_resource_bitmap_id)
        return(resource_bitmap_handle);

    object_id = p_resource_bitmap_id->object_id;

    if(OBJECT_ID_ENUM_START == object_id) /* scan all currently loaded objects */
        return(resource_bitmap_find_scan_objects(p_resource_bitmap_id));

    myassert1x(IS_OBJECT_ID_VALID(object_id), TEXT("resource_bitmap_find(INVALID OBJECT_ID ") S32_TFMT TEXT(")"), object_id);

    if( !object_present(object_id) ||
        (NULL == (p_resources_per_object = resource_statics.objects[object_id])) )
        return(resource_bitmap_handle);

#if RISCOS
    {
    P_SAH sprite_areas[4];
    const BOOL scan_11 = (host_modevar_cache_current.XEigFactor < 1U) && (host_modevar_cache_current.YEigFactor < 1U);
    const BOOL scan_22 = (host_modevar_cache_current.XEigFactor < 2U) && (host_modevar_cache_current.YEigFactor < 2U);

    /* can't really use system-defined bitmaps here? */

    if(scan_11)
    {
        sprite_areas[0] = p_resources_per_object->riscos.sprites.s_11;
        sprite_areas[1] = p_resources_per_object->riscos.sprites.s_22;
        sprite_areas[2] = p_resources_per_object->riscos.sprites.s_24;
    }
    else if(scan_22)
    {
        sprite_areas[0] = p_resources_per_object->riscos.sprites.s_22;
        sprite_areas[1] = p_resources_per_object->riscos.sprites.s_11;
        sprite_areas[2] = p_resources_per_object->riscos.sprites.s_24;
    }
    else
    {
        sprite_areas[0] = p_resources_per_object->riscos.sprites.s_24;
        sprite_areas[1] = p_resources_per_object->riscos.sprites.s_22;
        sprite_areas[2] = p_resources_per_object->riscos.sprites.s_11;
    }

    sprite_areas[3] = p_resources_per_object->riscos.sprites.s_c;

    resource_bitmap_handle.i = spriteop_select_sprite_in_areas(sprite_areas, 4, p_resource_bitmap_id->bitmap_name);
    } /*block*/
#elif WINDOWS
    { /* scan this object's cache for bitmap */
    STATUS status;
    const P_LIST_BLOCK p_list_block = &p_resources_per_object->windows.bitmap_cache;
    const LIST_ITEMNO item = (LIST_ITEMNO) p_resource_bitmap_id->bitmap_id;
    HBITMAP * p_hBitmap = collect_goto_item(HBITMAP, p_list_block, item);
    /*STATUS status;*/
    if(NULL != p_hBitmap)
    {
        resource_bitmap_handle.i = *p_hBitmap;
    }
    else if(NULL != (p_hBitmap = collect_add_entry_elem(HBITMAP, p_list_block, P_DATA_NONE, item, &status)))
    {
        HINSTANCE hInstance_fallback;
        const HINSTANCE hInstance = resource_get_object_resources(object_id, &hInstance_fallback);

        if(NULL == (resource_bitmap_handle.i = (HBITMAP) LoadImage(hInstance, MAKEINTRESOURCE(p_resource_bitmap_id->bitmap_id), IMAGE_BITMAP, 0, 0, 0)))
            resource_bitmap_handle.i = (HBITMAP) LoadImage(hInstance_fallback, MAKEINTRESOURCE(p_resource_bitmap_id->bitmap_id), IMAGE_BITMAP, 0, 0, 0);

        /* cache this value */
        *p_hBitmap = resource_bitmap_handle.i;
    }
    } /*block*/
#endif /* OS */

    return(resource_bitmap_handle);
}

#if WINDOWS

_Check_return_
extern BOOL /*found*/
resource_bitmap_find_new(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id,
    _OutRef_    P_RESOURCE_BITMAP_HANDLE p_resource_bitmap_handle,
    _OutRef_    P_GDI_SIZE p_bm_grid_size,
    _OutRef_    PUINT p_index)
{
    RESOURCE_BITMAP_ID resource_bitmap_id;

    resource_bitmap_id.object_id = p_resource_bitmap_id->object_id;
    resource_bitmap_id.bitmap_id = p_resource_bitmap_id->bitmap_id & ~(T5_RESOURCE_COMMON_BMP_BIT);

    p_bm_grid_size->cx = 0;
    p_bm_grid_size->cy = 0;

    *p_index = 0;

    if(p_resource_bitmap_id->bitmap_id & T5_RESOURCE_COMMON_BMP_BIT) /* index into one of our multi-image stores? */
    {
        *p_index = resource_bitmap_id.bitmap_id >> 8;
        resource_bitmap_id.bitmap_id &= 0xFF;

        if(tdd.uDPI >= 120)
        {   /* try for high-dpi variant (at id+1) */
            /* NB this only applies to multi-image stores */
            RESOURCE_BITMAP_ID high_dpi_resource_bitmap_id;
            high_dpi_resource_bitmap_id.object_id = resource_bitmap_id.object_id;
            high_dpi_resource_bitmap_id.bitmap_id = resource_bitmap_id.bitmap_id + 1;
            *p_resource_bitmap_handle = resource_bitmap_find(&high_dpi_resource_bitmap_id);
            if(0 != p_resource_bitmap_handle->i)
            {
                status_assert(resource_bitmap_tool_size_query(&high_dpi_resource_bitmap_id, p_bm_grid_size));
                return(TRUE);
            }

            /* stick with normal-dpi variant */
        }

        *p_resource_bitmap_handle = resource_bitmap_find(&resource_bitmap_id);

        if(0 != p_resource_bitmap_handle->i)
        {
            status_assert(resource_bitmap_tool_size_query(&resource_bitmap_id, p_bm_grid_size));
            return(TRUE);
        }

        return(FALSE);
    }

    *p_resource_bitmap_handle = resource_bitmap_find(&resource_bitmap_id);

    if(0 != p_resource_bitmap_handle->i)
    {
        BITMAP bitmap;
        GetObject(p_resource_bitmap_handle->i, sizeof32(bitmap), &bitmap);
        p_bm_grid_size->cx = bitmap.bmWidth;
        p_bm_grid_size->cy = bitmap.bmHeight;
        return(TRUE);
    }

    return(FALSE);
}

#endif /* OS */

#if RISCOS && defined(UNUSED_KEEP_ALIVE)

_Check_return_
extern RESOURCE_BITMAP_HANDLE
resource_bitmap_find_in_area(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id,
    _InVal_     S32 area_id)
{
    RESOURCE_BITMAP_HANDLE resource_bitmap_handle;
    OBJECT_ID object_id;
    P_RESOURCES_PER_OBJECT p_resources_per_object;

    resource_bitmap_handle.i = 0;

    if(NULL == p_resource_bitmap_id)
        return(resource_bitmap_handle);

    object_id = p_resource_bitmap_id->object_id;

    myassert1x(IS_OBJECT_ID_VALID(object_id), TEXT("resource_bitmap_find(INVALID OBJECT_ID ") S32_TFMT TEXT(")"), object_id);

    if( !object_present(object_id) || (NULL == (p_resources_per_object = resource_statics.objects[object_id])) )
        return(resource_bitmap_handle);

    {
    P_SAH area;

    switch(area_id)
    {
    case RESOURCE_BITMAP_AREA_UHI_RES:
        area = p_resources_per_object->riscos.sprites.s_11;
        break;

    case RESOURCE_BITMAP_AREA_HI_RES:
        area = p_resources_per_object->riscos.sprites.s_22;
        break;

    case RESOURCE_BITMAP_AREA_LO_RES:
        area = p_resources_per_object->riscos.sprites.s_24;
        break;

    default: default_unhandled();
#if CHECKING
    case RESOURCE_BITMAP_AREA_STANDARD:
#endif
        area = p_resources_per_object->riscos.sprites.s_c;
        break;
    }

    resource_bitmap_handle.i = spriteop_select_sprite(area, p_resource_bitmap_id->bitmap_name);
    } /*block*/

    return(resource_bitmap_handle);
}

#endif /* OS */

#if RISCOS

_Check_return_
extern RESOURCE_BITMAP_HANDLE
resource_bitmap_find_defaulting(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id)
{
    RESOURCE_BITMAP_HANDLE resource_bitmap_handle;

    resource_bitmap_handle = resource_bitmap_find(p_resource_bitmap_id);

    if(0 == resource_bitmap_handle.i)
    {
        resource_bitmap_handle = resource_bitmap_find_system(p_resource_bitmap_id);

        if(0 != resource_bitmap_handle.i)
        {   /* return a bodged pointer to the name rather than an address */
            resource_bitmap_handle.p_u8 = de_const_cast(P_U8, p_resource_bitmap_id->bitmap_name);
            resource_bitmap_handle.i |= RESOURCE_BITMAP_HANDLE_RISCOS_BODGE;
        }
    }

    return(resource_bitmap_handle);
}

#endif /* RISCOS */

#if RISCOS

_Check_return_
extern RESOURCE_BITMAP_HANDLE
resource_bitmap_find_system(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id)
{
    RESOURCE_BITMAP_HANDLE resource_bitmap_handle;
    SAH * sprite_areas[2];

    { /* have a look in Window Manager's back pockets */
    _kernel_swi_regs rs;
    void_WrapOsErrorChecking(_kernel_swi(Wimp_BaseOfSprites, &rs, &rs));
    sprite_areas[0] = (P_ANY) rs.r[1]; /* RAM */
    sprite_areas[1] = (P_ANY) rs.r[0]; /* ROM */
    } /*block*/

    resource_bitmap_handle.i = spriteop_select_sprite_in_areas(sprite_areas, 2, p_resource_bitmap_id->bitmap_name);

    return(resource_bitmap_handle);
}

#endif /* RISCOS */

#if WINDOWS

_Check_return_
extern STATUS /* STATUS_DONE and size set iff found, or STATUS_OK and zero size if not */
resource_bitmap_tool_size_query(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id,
    _OutRef_    P_GDI_SIZE p_size)
{
    ARRAY_INDEX i;
    p_size->cx = p_size->cy = 0;
    for(i = 0; i < array_elements(&resource_statics.bitmap_index); ++i)
    {
        PC_RESOURCE_BITMAP_INDEX p_resource_bitmap_index = array_ptrc(&resource_statics.bitmap_index, RESOURCE_BITMAP_INDEX, i);
        if(p_resource_bitmap_index->bitmap_id.object_id != p_resource_bitmap_id->object_id)
            continue;
        if(p_resource_bitmap_index->bitmap_id.bitmap_id != p_resource_bitmap_id->bitmap_id)
            continue;
        *p_size = p_resource_bitmap_index->size;
        return(STATUS_DONE);
    }
    return(STATUS_OK);
}

#endif /* WINDOWS */

_Check_return_
extern STATUS
resource_bitmap_tool_size_register(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id,
    _In_        int cx,
    _In_        int cy)
{
    RESOURCE_BITMAP_INDEX resource_bitmap_index;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(resource_bitmap_index), 0);
    resource_bitmap_index.bitmap_id = *p_resource_bitmap_id;
    resource_bitmap_index.size.cx = cx;
    resource_bitmap_index.size.cy = cy;
    return(al_array_add(&resource_statics.bitmap_index, RESOURCE_BITMAP_INDEX, 1, &array_init_block, &resource_bitmap_index));
}

extern void
resource_bitmap_lose(
    /*inout*/ P_RESOURCE_BITMAP_HANDLE p_resource_bitmap_handle)
{
    if(0 != p_resource_bitmap_handle->i)
    {
#if RISCOS
        /* Don't need to throw away on RISC OS */
#elif WINDOWS
        /* Decrement cache ref count if we can be bothered */
#endif
        p_resource_bitmap_handle->i = 0;
    }
}

#if RISCOS

extern void
resource_bitmap_gdi_size_query(
    RESOURCE_BITMAP_HANDLE resource_bitmap_handle,
    _OutRef_    P_GDI_SIZE p_size)
{
    _kernel_swi_regs rs;
    BOOL ok;

    if(resource_bitmap_handle.i & RESOURCE_BITMAP_HANDLE_RISCOS_BODGE)
    {
        rs.r[0] = 40; /* Read sprite information */
        rs.r[2] = resource_bitmap_handle.i & ~RESOURCE_BITMAP_HANDLE_RISCOS_BODGE;

        ok = (NULL == _kernel_swi(Wimp_SpriteOp, &rs, &rs));
    }
    else
    {
        rs.r[0] = 0x200 | 40; /* Read sprite information */
        rs.r[1] = (int) 0x89ABFEDC; /* kill the OS or any twerp who dares to access this! */
        rs.r[2] = resource_bitmap_handle.i;

        ok = (NULL == _kernel_swi(OS_SpriteOp, &rs, &rs));
    }

    if(!ok)
    {
        p_size->cx = p_size->cy = 0;
        return;
    }

    { /* read info on defining screen mode */
    U32 XEigFactor, YEigFactor;
    host_modevar_cache_query_eig_factors(rs.r[6], &XEigFactor, &YEigFactor);
    p_size->cx = (GDI_COORD) rs.r[3] << XEigFactor;
    p_size->cy = (GDI_COORD) rs.r[4] << YEigFactor;
    } /*block*/
}

#endif /* OS */

static inline void
resource_close_per_object_resources(
    _InVal_     OBJECT_ID object_id)
{
    const P_RESOURCES_PER_OBJECT p_resources_per_object = resource_statics.objects[object_id];

    PTR_ASSERT(p_resources_per_object);

#if RISCOS
    if(p_resources_per_object->riscos.sprites.loaded_24)
    {
        p_resources_per_object->riscos.sprites.loaded_24 = 0;
        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_resources_per_object->riscos.sprites.s_24));
    }

    if(p_resources_per_object->riscos.sprites.loaded_22)
    {
        p_resources_per_object->riscos.sprites.loaded_22 = 0;
        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_resources_per_object->riscos.sprites.s_22));
    }

    if(p_resources_per_object->riscos.sprites.loaded_11)
    {
        p_resources_per_object->riscos.sprites.loaded_11 = 0;
        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_resources_per_object->riscos.sprites.s_11));
    }

    if(p_resources_per_object->riscos.sprites.loaded_c)
    {
        p_resources_per_object->riscos.sprites.loaded_c = 0;
        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_resources_per_object->riscos.sprites.s_c));
    }
#elif WINDOWS
    if(collect_has_data(&p_resources_per_object->windows.bitmap_cache))
    {   /* blow away cached bitmaps or else Mr GDI gets all upset */
        const P_LIST_BLOCK p_list_block = &p_resources_per_object->windows.bitmap_cache;
        LIST_ITEMNO item;
        HBITMAP * p_hBitmap;

        for(p_hBitmap = collect_first(HBITMAP, p_list_block, &item);
            p_hBitmap;
            p_hBitmap = collect_next(HBITMAP, p_list_block, &item))
        {
            if(*p_hBitmap)
                DeleteBitmap(*p_hBitmap);
        }

        collect_delete(p_list_block);
    }

#if defined(NOT_ALL_IN_ONE)
    if(NULL != p_resources_per_object->windows.library_handle)
    {
        FreeLibrary(p_resources_per_object->windows.library_handle); /* now kosher 'cos only resources out there */
        p_resources_per_object->windows.library_handle = 0;
    }
#endif /* NOT_ALL_IN_ONE */
#endif /* OS */
}

_Check_return_
extern STATUS
resource_close(
    _InVal_     OBJECT_ID object_id)
{
    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert1(TEXT("resource_close(INVALID OBJECT_ID ") S32_TFMT TEXT(")"), object_id);
        return(status_check());
    }

    if(NULL == resource_statics.objects[object_id])
        return(STATUS_OK);

    resource_close_per_object_resources(object_id);

#if WINDOWS && !defined(NOT_ALL_IN_ONE)
    if( (0 == object_id) && (NULL != resource_statics.windows.library_handle) )
    {
        FreeLibrary(resource_statics.windows.library_handle); /* now kosher 'cos only resources out there */
        resource_statics.windows.library_handle = 0;
    }
#endif /* NOT_ALL_IN_ONE */

    { /* remove all entries that belong to this object from the resource_bitmap_index array */
    ARRAY_INDEX i = 0;

    while(i < array_elements(&resource_statics.bitmap_index))
    {
        PC_RESOURCE_BITMAP_INDEX p_resource_bitmap_index = array_ptrc(&resource_statics.bitmap_index, RESOURCE_BITMAP_INDEX, i);

        if(p_resource_bitmap_index->bitmap_id.object_id != object_id)
        {
            ++i;
            continue;
        }

        /* compact down removing element, then loop at same index */
        al_array_delete_at(&resource_statics.bitmap_index, -1, i);
    }

    if(array_elements(&resource_statics.bitmap_index) == 0)
        al_array_dispose(&resource_statics.bitmap_index);
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
extern STATUS
resource_dll_find(
    _InVal_     OBJECT_ID object_id,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer)
{
    TCHARZ name[BUF_MAX_PATHSTRING];

    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert1(TEXT("resource_dll_find(INVALID OBJECT_ID ") S32_TFMT TEXT(")"), object_id);
        return(status_check());
    }

    consume_int(tstr_xsnprintf(name, elemof32(name), tstr_objects_dll_store, (U32) object_id));

    if(file_find_on_path(tstr_buf, elemof_buffer, file_get_resources_path(), name) > 0)
        return(STATUS_OK);

    tstr_buf[0] = CH_NULL;
    return(STATUS_FAIL);
}

#if WINDOWS

_Check_return_
static STATUS
resource_language_dll_find(
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer)
{
    if(file_find_on_path(tstr_buf, elemof_buffer, file_get_resources_path(), TEXT("LangRes.dll")) > 0)
        return(STATUS_OK);

    tstr_buf[0] = CH_NULL;
    return(STATUS_FAIL);
}

#endif /* WINDOWS */

extern void
resource_dll_free(
    _InVal_     OBJECT_ID object_id)
{
#if WINDOWS
#if defined(NOT_ALL_IN_ONE)
    const P_RESOURCES_PER_OBJECT p_resources_per_object = resource_statics.objects[object_id];

    if(p_resources_per_object->windows.library_handle)
    {
        FreeLibrary(p_resources_per_object->windows.library_handle);
        p_resources_per_object->windows.library_handle = 0;
    }
#else
    if( (0 == object_id) && (NULL != resource_statics.windows.library_handle) )
    {
        FreeLibrary(resource_statics.windows.library_handle);
        resource_statics.windows.library_handle = 0;
    }
#endif
#else
    UNREFERENCED_PARAMETER_InVal_(object_id);
#endif
}

#if defined(UNUSED_KEEP_ALIVE)

extern void
resource_donate(
    _InVal_     OBJECT_ID object_id_dst,
    _InVal_     OBJECT_ID object_id_src)
{
#if RISCOS
    assert(OBJECT_ID_SKEL == object_id_dst);
    {
    P_SAH * p_p_src = &resource_statics.objects[object_id_src]->riscos.sprites.s_22;
    P_SAH * p_p_dst = &resource_statics.objects[object_id_dst]->riscos.sprites.s_22;
    if(*p_p_src)
    {
        *p_p_dst = *p_p_src;
        *p_p_src = NULL;
    }
    } /*block*/
#else
    UNREFERENCED_PARAMETER_InVal_(object_id_dst);
    UNREFERENCED_PARAMETER_InVal_(object_id_src);
#endif
}

#endif /* UNUSED_KEEP_ALIVE */

#if RISCOS

_Check_return_
static STATUS
resource_init_riscos_msgs(
    _InVal_     OBJECT_ID object_id,
    _In_opt_    PC_U8 * const p_u8_bound_msg)
{
    const P_RESOURCES_PER_OBJECT p_resources_per_object = resource_statics.objects[object_id];

    if(DONT_LOAD_MESSAGES_FILE == p_u8_bound_msg)
        return(STATUS_OK);

    PTR_ASSERT(p_resources_per_object);
    p_resources_per_object->riscos.mangled_msgs.msgs_block = (PC_U8) p_u8_bound_msg;

    if(LOAD_MESSAGES_FILE == p_u8_bound_msg)
        return(resource_load_messages(object_id));

    riscos_msg_init(&p_resources_per_object->riscos.mangled_msgs);
    return(STATUS_OK);
}

_Check_return_
static STATUS
resource_init_riscos_bound_resources(
    _InVal_     OBJECT_ID object_id,
    _InRef_opt_ PC_BOUND_RESOURCES p_bound_resources)
{
    const P_RESOURCES_PER_OBJECT p_resources_per_object = resource_statics.objects[object_id];

    if(DONT_LOAD_RESOURCES == p_bound_resources)
        return(STATUS_OK);

    if(LOAD_RESOURCES == p_bound_resources)
        return(resource_load_appropriate_sprites(object_id));

    PTR_ASSERT(p_resources_per_object);

    /* load common sprites */
    if(p_bound_resources->sprite_area_c)
        p_resources_per_object->riscos.sprites.s_c = p_bound_resources->sprite_area_c;

    /* load EIG-dependent sprites */
    if(p_bound_resources->sprite_area_11)
        p_resources_per_object->riscos.sprites.s_11 = p_bound_resources->sprite_area_11;

    if(p_bound_resources->sprite_area_22)
        p_resources_per_object->riscos.sprites.s_22 = p_bound_resources->sprite_area_22;

    if(p_bound_resources->sprite_area_24)
        p_resources_per_object->riscos.sprites.s_24 = p_bound_resources->sprite_area_24;

    return(STATUS_OK);
}

#elif WINDOWS

_Check_return_
extern STATUS
resource_init_windows_bound_resources(
    _InVal_     OBJECT_ID object_id,
    _InRef_opt_ PC_BOUND_RESOURCES p_bound_resources)
{
#if defined(NOT_ALL_IN_ONE)
    const P_RESOURCES_PER_OBJECT p_resources_per_object = resource_statics.objects[object_id];
    TCHARZ resource_file[BUF_MAX_PATHSTRING];

    PTR_ASSERT(p_resources_per_object);

    if(NULL == p_resources_per_object->windows.library_handle)
    {
        if(NULL != p_bound_resources)
        {
            if(status_ok(resource_dll_find(object_id, resource_file, elemof32(resource_file))))
            {
                p_resources_per_object->windows.library_handle = LoadLibrary(resource_file);

                if((UINT) p_resources_per_object->windows.library_handle <= (UINT) HINSTANCE_ERROR)
                {
                    p_resources_per_object->windows.library_handle = NULL;   /* just-in-case */
                    return(STATUS_MODULE_NOT_FOUND);
                }
            }
        }
    }
#else
    /* don't load any further per-object resources */
    UNREFERENCED_PARAMETER_InVal_(object_id);
    UNREFERENCED_PARAMETER_InRef_(p_bound_resources);
#endif

    return(STATUS_OK);
}

#endif /* OS */

_Check_return_
extern STATUS
resource_init(
    _InVal_     OBJECT_ID object_id,
    _In_opt_    PC_U8 * const p_u8_bound_msg,
    _InRef_opt_ PC_BOUND_RESOURCES p_bound_resources)
{
    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert1(TEXT("resource_init(INVALID OBJECT_ID ") S32_TFMT TEXT(")"), object_id);
        return(status_check());
    }

    if(NULL == resource_statics.objects[object_id])
    {
        STATUS status;
        if(NULL == (resource_statics.objects[object_id] = al_ptr_calloc_elem(struct RESOURCES_PER_OBJECT, 1, &status)))
            return(status);
    }

#if RISCOS
    status_return(resource_init_riscos_msgs(object_id, p_u8_bound_msg));

    status_return(resource_init_riscos_bound_resources(object_id, p_bound_resources));
#elif WINDOWS
    UNREFERENCED_PARAMETER_CONST(p_u8_bound_msg);
    assert(DONT_LOAD_MESSAGES_FILE == p_u8_bound_msg);

    status_return(resource_init_windows_bound_resources(object_id, p_bound_resources));
#endif /* OS */

    return(STATUS_OK);
}

#if RISCOS

_Check_return_
static STATUS
riscos_sprite_readfile_into(
    _In_z_      PCTSTR filename,
    _Out_       P_SAH * p_p_sah)
{
    STATUS status = STATUS_OK;
    _kernel_osfile_block osfile_block;
    _kernel_oserror * e;

    *p_p_sah = NULL;

    zero_struct_fn(osfile_block);

    switch(_kernel_osfile(OSFile_ReadNoPath, filename, &osfile_block))
    {
    default: default_unhandled();
#if CHECKING
    case _kernel_ERROR:
    case OSFile_ObjectType_None:
    case OSFile_ObjectType_Dir:
    case OSFile_ObjectType_Image:
#endif
        break;

    case OSFile_ObjectType_File:
        {
        const U32 area_size = osfile_block.start /*R4*/ + 4; /* sprite file is sprite area without the length word */

        if(NULL != (*p_p_sah = al_ptr_alloc_bytes(P_SAH, area_size, &status)))
        {
            _kernel_swi_regs rs;

            (*p_p_sah)->area_size = area_size; /* first we must initialise it ourselves */
            (*p_p_sah)->offset_to_first = 16;

            rs.r[0] = 0x100 | 9; /* Initialise sprite area */
            rs.r[1] = (int) *p_p_sah;
            rs.r[2] = area_size;

            void_WrapOsErrorChecking(_kernel_swi(OS_SpriteOp, &rs, &rs));

            rs.r[0] = 0x100 | 10; /* Load sprite file */
            rs.r[1] = (int) *p_p_sah;
            rs.r[2] = (int) filename;

            if(NULL != (e = _kernel_swi(OS_SpriteOp, &rs, &rs)))
                al_ptr_dispose((void **) p_p_sah);
        }

        break;
        }
    }

    return(status);
}

_Check_return_
static STATUS
resource_find_sprites(
    _Out_writes_(elemof_buffer) PTSTR buffer,
    U32 elemof_buffer,
    _InVal_     OBJECT_ID object_id,
    _In_        UINT which)
{
    TCHARZ name[BUF_MAX_PATHSTRING];

    if(NULL == _kernel_getenv("Wimp$IconTheme", name, elemof32(name)))
    {
        const U32 used = strlen(name);

        consume_int(snprintf(name + used, elemof32(name) - used,
                             TEXT("RISC_OS") FILE_DIR_SEP_TSTR TEXT("Sprites%.2u") FILE_DIR_SEP_TSTR TEXT("Ob%.2u"),
                             (unsigned int) which, (unsigned int) object_id));

        if(file_find_on_path(buffer, elemof_buffer, file_get_resources_path(), name) > 0)
            return(STATUS_DONE);
    }

    consume_int(snprintf(name, elemof32(name),
                         TEXT("RISC_OS") FILE_DIR_SEP_TSTR TEXT("Sprites%.2u") FILE_DIR_SEP_TSTR TEXT("Ob%.2u"),
                         (unsigned int) which, (unsigned int) object_id));

    if(file_find_on_path(buffer, elemof_buffer, file_get_resources_path(), name) > 0)
        return(STATUS_DONE);

    return(STATUS_OK);
}

_Check_return_
extern STATUS
resource_load_sprites(
    _InVal_     OBJECT_ID object_id,
    _In_        UINT which)
{
    const P_RESOURCES_PER_OBJECT p_resources_per_object = resource_statics.objects[object_id];
    TCHARZ resource_file[BUF_MAX_PATHSTRING];
    STATUS status = STATUS_OK;

    switch(which)
    {
    default:
        break;

    case 11:
        if(status_done(resource_find_sprites(resource_file, elemof32(resource_file), object_id, which)))
        {
          /*reportf("resource_load_sprites(%u): load %s", object_id, resource_file);*/
            riscos_sprite_readfile_into(resource_file, &p_resources_per_object->riscos.sprites.s_11);
            p_resources_per_object->riscos.sprites.loaded_11 = 1;
            return(STATUS_DONE);
        }

        break;

    case 22:
        if(status_done(resource_find_sprites(resource_file, elemof32(resource_file), object_id, which)))
        {
          /*reportf("resource_load_sprites(%u): load %s", object_id, resource_file);*/
            riscos_sprite_readfile_into(resource_file, &p_resources_per_object->riscos.sprites.s_22);
            p_resources_per_object->riscos.sprites.loaded_22 = 1;
            return(STATUS_DONE);
        }

        break;

    case 24:
        if(status_done(resource_find_sprites(resource_file, elemof32(resource_file), object_id, which)))
        {
          /*reportf("resource_load_sprites(%u): load %s", object_id, resource_file);*/
            riscos_sprite_readfile_into(resource_file, &p_resources_per_object->riscos.sprites.s_24);
            p_resources_per_object->riscos.sprites.loaded_24 = 1;
            return(STATUS_DONE);
        }

        break;
    }

  /*reportf("resource_load_sprites(%u, %u): no match", object_id, which);*/
    return(status);
}

/* In fact this loads all sprites for current mode and lower resolutions
 * in case of mode change to lower resolution mode whilst running.
 * Doesn't waste space loading highest resolution sprites if not wanted!
 */

_Check_return_
extern STATUS
resource_load_appropriate_sprites(
    _InVal_     OBJECT_ID object_id)
{
    STATUS status;
    const BOOL load_res_11 = (host_modevar_cache_current.XEigFactor < 1U) && (host_modevar_cache_current.YEigFactor < 1U);
    const BOOL load_res_22 = (host_modevar_cache_current.XEigFactor < 2U) && (host_modevar_cache_current.YEigFactor < 2U);

    if(load_res_11)
        status_return(resource_load_sprites(object_id, 11));

    if(load_res_22)
        status_return(resource_load_sprites(object_id, 22));

    /* always needs to have lowest resolution sprites to register */
    status = resource_load_sprites(object_id, 24);

    if(STATUS_OK != status)
        return(status);

    return(ERR_NO_SPRITES);
}

#endif /* OS */

/******************************************************************************
*
* split a status into object_id and offset
*
******************************************************************************/

/*ncr*/
extern BOOL
resource_split_status(
    _In_        STATUS status,
    _OutRef_    P_U16 p_offset,
    _OutRef_    P_OBJECT_ID p_object_id)
{
    BOOL is_err = status_fail(status);

    if(is_err)
    {
        /* error number */
        status = -status;
        assert_EQ(-STATUS_ERR_INCREMENT, STATUS_MSG_INCREMENT);
    }

    /* module range */
    *p_offset = (U16) (status & U16_MAX); /* keep /RTCc happy */
    *p_object_id = (OBJECT_ID) ((U32) (status & U32_MAX) >> 16);

    return(is_err);
}

extern void
resource_shutdown(void)
{
    OBJECT_ID object_id = OBJECT_ID_MAX;

    do  {
        OBJECT_ID_DECR(object_id);

        (void) resource_close(object_id);
    }
    while(OBJECT_ID_FIRST != object_id);
}

extern void
resource_startup(void)
{
#if WINDOWS
    /* check for a language resource DLL */
    TCHARZ language_resource_file[BUF_MAX_PATHSTRING];

    assert(NULL == resource_statics.windows.library_handle);

    if(status_ok(resource_language_dll_find(language_resource_file, elemof32(language_resource_file))))
    {
        resource_statics.windows.library_handle = LoadLibrary(language_resource_file);

        if((UINT) resource_statics.windows.library_handle <= (UINT) HINSTANCE_ERROR)
        {
            resource_statics.windows.library_handle = NULL;   /* just-in-case */
        }
    }
#endif
}

#if WINDOWS

_Check_return_
extern HINSTANCE
resource_get_object_resources(
    _InVal_     OBJECT_ID object_id,
    _Out_       HINSTANCE * const p_hInstance_fallback)
{
    HINSTANCE hInstance = NULL;

    *p_hInstance_fallback = GetInstanceHandle();

    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert1(TEXT("resource_get_object_resources(INVALID OBJECT_ID ") S32_TFMT TEXT(")"), object_id);
        return(NULL);
    }

#if defined(NOT_ALL_IN_ONE)
    if(NULL != resource_statics.objects[object_id]->windows.library_handle)
        hInstance = resource_statics.objects[object_id]->windows.library_handle;
#else
    if( (0 == object_id) && (NULL != resource_statics.windows.library_handle) )
        hInstance = resource_statics.windows.library_handle;
#endif

    if(NULL == hInstance)
        hInstance = GetInstanceHandle();

    return(hInstance);
}

#endif /* WINDOWS */

#ifndef MSGS_LOOKUP_DELIMITER_STR
#define MSGS_LOOKUP_DELIMITER_STR "_"
#endif

_Check_return_
_Ret_z_
static PC_USTR
resource_lookup_ustr_default_msg(
    _InVal_     STATUS status)
{
    OBJECT_ID object_id;
    U16 status_offset;
    BOOL is_err = resource_split_status(status, &status_offset, &object_id);
    PC_USTR p = resource_lookup_ustr_no_default(is_err ? ERR_ERR_NOT_FOUND : ERR_MSG_NOT_FOUND);
    UCHARZ tmpbuf2[256];
    assert_EQ(TRUE, status_fail(ERR_ERR_NOT_FOUND));
    assert_EQ(TRUE, status_fail(ERR_MSG_NOT_FOUND));
    ustr_xstrkpy(ustr_bptr(tmpbuf2), elemof32(tmpbuf2), p ? p : USTR_TEXT("!!!Panic!!! object_id=%u:status_offset=%u"));
    consume_int(ustr_xsnprintf(ustr_bptr(resource_statics.default_msg.ucharz), elemof32(resource_statics.default_msg.ucharz),
                               (PC_UCHARS) tmpbuf2,
                               (unsigned int) object_id, (unsigned int) status_offset));
    return(ustr_bptr(resource_statics.default_msg.ucharz));
}

_Check_return_
_Ret_z_
static PTSTR
resource_lookup_tstr_default_msg(
    _InVal_     STATUS status)
{
#if TSTR_IS_SBSTR
    (void) resource_lookup_ustr_default_msg(status);
#else
    OBJECT_ID object_id;
    U16 status_offset;
    BOOL is_err = resource_split_status(status, &status_offset, &object_id);
    PCTSTR p = resource_lookup_tstr_no_default(is_err ? ERR_ERR_NOT_FOUND : ERR_MSG_NOT_FOUND);
    TCHARZ tmpbuf2[256];
    assert_EQ(TRUE, status_fail(ERR_ERR_NOT_FOUND));
    assert_EQ(TRUE, status_fail(ERR_MSG_NOT_FOUND));
    tstr_xstrkpy(tmpbuf2, elemof32(tmpbuf2), p ? p : TEXT("!!!Panic!!! object_id=%u:status_offset=%u"));
    consume_int(tstr_xsnprintf(resource_statics.default_msg.tcharz, elemof32(resource_statics.default_msg.tcharz),
                               tmpbuf2,
                               (unsigned int) object_id, (unsigned int) status_offset));
#endif /* TSTR_IS_SBSTR */
    return(resource_statics.default_msg.tcharz);
}

/******************************************************************************
*
* return a pointer to a low-lifetime string
* corresponding to the STATUS passed in
* +ve -> message string, -ve -> error string
*
******************************************************************************/

_Check_return_
_Ret_z_
extern PC_USTR /*ostensibly const*/
resource_lookup_ustr(
    _InVal_     STATUS status)
{
    PC_USTR ustr = resource_lookup_ustr_no_default(status);

    if(NULL != ustr)
        return(ustr);

    if(0 == status)
        return(ustr_empty_string);

    return(resource_lookup_ustr_default_msg(status));
}

extern void
resource_lookup_ustr_buffer(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     STATUS status)
{
    ustr_xstrkpy(ustr_buf, elemof_buffer, resource_lookup_ustr(status));
}

_Check_return_
_Ret_maybenull_z_
extern PC_USTR
resource_lookup_ustr_no_default(
    _InVal_     STATUS status)
{
    PCTSTR tstr = resource_lookup_tstr_no_default(status);

    if(NULL != tstr)
        return(_ustr_from_tstr(tstr));

    return(NULL);
}

_Check_return_
_Ret_z_
extern PTSTR /*ostensibly const*/
resource_lookup_tstr(
    _InVal_     STATUS status)
{
    PTSTR tstr = resource_lookup_tstr_no_default(status);

    if(NULL != tstr)
        return(tstr);

    if(0 == status)
        return(tstr_empty_string);

    return(resource_lookup_tstr_default_msg(status));
}

/* NB Native lookup on Windows is TCHAR; our RISC OS message files are Latin-1 encoded i.e. TCHAR */

#if RISCOS

extern void
resource_lookup_tstr_buffer(
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     STATUS status)
{
    tstr_xstrkpy(tstr_buf, elemof_buffer, resource_lookup_tstr(status));
}

_Check_return_
_Ret_maybenull_z_
extern PTSTR
resource_lookup_tstr_no_default(
    _InVal_     STATUS status)
{
    OBJECT_ID object_id;
    U16 status_offset;
    P_RESOURCES_PER_OBJECT p_resources_per_object;
    BOOL is_err;

    if(0 == status)
        return(NULL);

    is_err = resource_split_status(status, &status_offset, &object_id);

    if( !IS_OBJECT_ID_VALID(object_id) || (NULL == (p_resources_per_object = resource_statics.objects[object_id])) )
    {
        myassert1x(IS_OBJECT_ID_VALID(object_id), TEXT("resource_lookup_tstr_no_default(INVALID OBJECT_ID ") S32_TFMT TEXT(")"), object_id);
        return(NULL);
    }

    {
    TCHARZ tag_buffer[sizeof32("e32767")];
    PC_SBSTR str;
    U32 tag_length = tstr_xsnprintf(tag_buffer, elemof32(tag_buffer), is_err ? "e%u" : "%u", (unsigned int) status_offset);

    str = riscos_msg_lookup_in_without_default(tag_buffer, tag_length, &p_resources_per_object->riscos.mangled_msgs);

    return((PTSTR) str); /* may be NULL */
    } /*block*/
}

#elif WINDOWS

_Check_return_ _Ret_maybenone_
static PCWCH
resource_lookup_wchars_no_default(
    _OutRef_    P_U32 p_n_chars,
    _InVal_     STATUS status);

_Check_return_
static BOOL
resource_lookup_tstr_buffer_no_default(
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     STATUS status);

extern void
resource_lookup_tstr_buffer(
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     STATUS status)
{
    if(resource_lookup_tstr_buffer_no_default(tstr_buf, elemof_buffer, status))
        return;

    if(0 == status)
        return;

    {
    OBJECT_ID object_id;
    U16 status_offset;
    BOOL is_err = resource_split_status(status, &status_offset, &object_id);
    PCTSTR p = resource_lookup_tstr_no_default(is_err ? ERR_ERR_NOT_FOUND : ERR_MSG_NOT_FOUND);
    TCHARZ tmpbuf2[256];
    assert_EQ(TRUE, status_fail(ERR_ERR_NOT_FOUND));
    assert_EQ(TRUE, status_fail(ERR_MSG_NOT_FOUND));
    tstr_xstrkpy(tmpbuf2, elemof32(tmpbuf2), p ? p : TEXT("!!!Panic!!! object_id=%u:status_offset=%u"));
    consume_int(tstr_xsnprintf(tstr_buf, elemof_buffer,
                               tmpbuf2,
                               (unsigned int) object_id, (unsigned int) status_offset));
    } /*block*/
}

_Check_return_
_Ret_maybenull_z_
extern PTSTR
resource_lookup_tstr_no_default(
    _InVal_     STATUS status)
{
    if(resource_lookup_tstr_buffer_no_default(resource_statics.default_msg.tcharz, elemof32(resource_statics.default_msg.tcharz), status))
        return(resource_statics.default_msg.tcharz);

    return(NULL);
}

_Check_return_
static inline UINT
resource_id_from_status(
    _OutRef_    P_OBJECT_ID p_object_id,
    _InVal_     STATUS status)
{
    U16 status_offset;
    const BOOL is_err = resource_split_status(status, &status_offset, p_object_id);
    UINT uint;

    if(is_err)
    {
        /*assert(1 != status_offset);*/ /* shouldn't be asked for ERROR_RQ, but don't complain or else create_error(xxx_ERROR_RQ) hurts */
        status_offset += 0x8000U;
    }

    switch(*p_object_id)
    {
    case 0:
        uint = status_offset;
        break;
    case 1:
        uint = status_offset + 0x2000;
        break;
    case 6:
        uint = status_offset + 0x3000;
        break;
    case 35:
        uint = status_offset + 0x3500;
        break;
    default:
        myassert1x(IS_OBJECT_ID_VALID(*p_object_id), TEXT("resource_lookup_tstr_buffer_no_default(INVALID OBJECT_ID ") S32_TFMT TEXT(")"), *p_object_id);
#define N_DEFAULT_MSGS 128
        uint = 0x8000U;
        uint -= N_DEFAULT_MSGS * MAX_OBJECTS;
        uint += N_DEFAULT_MSGS * (UINT) *p_object_id;
        uint += status_offset;
        break;
    }

    return(uint);
}

#if TSTR_IS_SBSTR

_Check_return_
static BOOL
resource_lookup_tstr_buffer_no_default(
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     STATUS status)
{
    OBJECT_ID object_id;
    const UINT uint = resource_id_from_status(&object_id, status);
    HINSTANCE hInstance;

    if(0 == status)
    {
        tstr_buf[0] = CH_NULL;
        return(FALSE);
    }

    hInstance = resource_statics.windows.library_handle;
    if(NULL != hInstance)
        if(LoadString(hInstance, uint, tstr_buf, elemof_buffer-1))
            return(TRUE);

    /* fallback */
    hInstance = GetInstanceHandle();
    if(LoadString(hInstance, uint, tstr_buf, elemof_buffer-1))
        return(TRUE);

    /* lookup failed */
    tstr_buf[0] = CH_NULL;
    return(FALSE);
}

#else

/* can obtain a read-only pointer to the wide-char resource */

_Check_return_ _Ret_maybenone_
static PCWCH
resource_lookup_wchars_no_default(
    _OutRef_    P_U32 p_n_chars,
    _InVal_     STATUS status)
{
    OBJECT_ID object_id;
    const UINT uint = resource_id_from_status(&object_id, status);
    HINSTANCE hInstance;
    union _LOADSTRING_BUFFER
    {
        BYTE data[80];
        PCWCH pwch;
    } loadstring_buffer;

    if(0 == status)
    {
        *p_n_chars = 0;
        return(P_DATA_NONE);
    }

    hInstance = resource_statics.windows.library_handle;
    if(NULL != hInstance)
        if(0 != (*p_n_chars = LoadStringW(hInstance, uint, (LPWSTR) &loadstring_buffer.data[0], 0)))
            return(loadstring_buffer.pwch);

    /* fallback */
    hInstance = GetInstanceHandle();
    if(0 != (*p_n_chars = LoadStringW(hInstance, uint, (LPWSTR) &loadstring_buffer.data[0], 0)))
        return(loadstring_buffer.pwch);

    *p_n_chars = 0;
    return(P_DATA_NONE);
}

_Check_return_
static BOOL
resource_lookup_tstr_buffer_no_default(
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     STATUS status)
{
    U32 n_chars;
    const PCWCH wchars = resource_lookup_wchars_no_default(&n_chars, status);

    if(0 != n_chars)
    {   /* caller may get a truncated copy */
        assert(0 != elemof_buffer);
        if(n_chars >= elemof_buffer)
            n_chars = elemof_buffer - 1; /* ensure room for terminator */
        memcpy32(tstr_buf, wchars, n_chars*sizeof32(WCHAR));
        tstr_buf[n_chars] = CH_NULL;
        return(TRUE);
    }

    /* lookup failed */
    tstr_buf[0] = CH_NULL;
    return(FALSE);
}

#endif /* TSTR_IS_SBSTR */

#endif /* OS */

/******************************************************************************
*
* look up the string into a user-supplied quick_block
*
* NB. does NOT add terminating CH_NULL
*
******************************************************************************/

_Check_return_
extern STATUS
resource_lookup_quick_ublock(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     STATUS status)
{
    return(quick_ublock_ustr_add(p_quick_ublock, resource_lookup_ustr(status)));
}

_Check_return_
extern STATUS
resource_lookup_quick_ublock_no_default(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     STATUS status)
{
    PC_USTR ustr = resource_lookup_ustr_no_default(status);

    if(NULL == ustr)
        return(STATUS_OK);

    status_return(quick_ublock_ustr_add(p_quick_ublock, ustr));

    return(STATUS_DONE);
}

_Check_return_
extern STATUS
resource_lookup_quick_tblock(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InVal_     STATUS status)
{
    return(quick_tblock_tstr_add(p_quick_tblock, resource_lookup_tstr(status)));
}

_Check_return_
extern STATUS
resource_lookup_quick_tblock_no_default(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InVal_     STATUS status)
{
#if TSTR_IS_SBSTR
    PCTSTR tstr = resource_lookup_tstr_no_default(status);

    if(NULL == tstr)
        return(STATUS_OK);

    status_return(quick_tblock_tstr_add(p_quick_tblock, tstr));
#else
    U32 n_chars;
    const PCWCH wchars = resource_lookup_wchars_no_default(&n_chars, status);

    if(0 == n_chars)
        return(STATUS_OK);

    status_return(quick_tblock_tchars_add(p_quick_tblock, wchars, n_chars));
#endif

    return(STATUS_DONE);
}

#if RISCOS

/* obtain a named sprite (in the Window Manager Sprite areas) representing the given filetype */

extern void
riscos_filesprite(
    /*_Out_z_cap_c_(elemof_buffer)*/ P_SBSTR sbstr_buf /*filled*/,
    _InVal_     U32 elemof_buffer,
    _InVal_     T5_FILETYPE t5_filetype)
{
    switch(t5_filetype)
    {
    default:
        {
        if(FILETYPE_UNTYPED == t5_filetype)
            xstrkpy(sbstr_buf, elemof_buffer, "file_lxa");
        else
            consume_int(xsnprintf(sbstr_buf, elemof_buffer, "file_%.3x", t5_filetype));

        /* now to check if the sprite exists: do a sprite select on the Window Manager sprite pool */
#if defined(NORCROFT_INLINE_SWIX_NOT_YET) /* not yet handled */
        if( NULL ==
            _swix(Wimp_SpriteOp, _IN(0)|_IN(2),
            /*in*/  24, /* Select sprite */
                    (int) sbstr_buf) )
            return;
#else
        _kernel_swi_regs rs;
        rs.r[0] = 24; /* Select sprite */
        rs.r[2] = (int) sbstr_buf;
        if(NULL == _kernel_swi(Wimp_SpriteOp, &rs, &rs))
            return;
#endif

        /* the sprite does not exist: use general don't-know icon. */
        } /*block*/

        /*FALLTHRU*/

    case FILETYPE_UNDETERMINED:
        xstrkpy(sbstr_buf, elemof_buffer, "file_xxx");
        break;

    case FILETYPE_DIRECTORY:
    case FILETYPE_APPLICATION:
        xstrkpy(sbstr_buf, elemof_buffer, "directory");
        break;
    }
}

#if defined(UNUSED_KEEP_ALIVE)

_Check_return_
_Ret_maybenull_
extern P_SAH
riscos_sprite_area_lookup(
    _InVal_     OBJECT_ID object_id)
{
    const P_RESOURCES_PER_OBJECT p_resources_per_object = resource_statics.objects[object_id];

    return((NULL != p_resources_per_object) ? p_resources_per_object->riscos.sprites.s_c : NULL);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* compare first n characters from a and b backwards
* (useful for similar tag searches)
*
* --out--
*       number of leading characters NOT same compared backwards
*                 ~~~~~~~
******************************************************************************/

_Check_return_
static U32
tstrrncmp(
    _In_reads_(tchars_n) PCTSTR a,
    _In_reads_(tchars_n) PCTSTR b,
    _In_        U32 tchars_n)
{
    profile_ensure_frame();

    if(tchars_n)
    {
        a += tchars_n;
        b += tchars_n;

        do
            if(*--a != *--b)
                break;
        while(--tchars_n);
    }

    return(tchars_n);
}

/* This code written to replace the supplied RISC_OSLib code
 * as that had (i) too much overhead, was (ii) too slow in loading
 * and (iii) used file i/o without bothering about buffering
 *
 * This code also allows binding of messages into an image
 * using a weak external refernce to msgs_weak_block.
 */

#define COMMENT_CH    CH_NUMBER_SIGN
#define TAG_DELIMITER CH_COLON

typedef struct RISCOS_MSGS_SEARCH
{
    PCTSTR tag;
    U32 tag_length;
}
RISCOS_MSGS_SEARCH, * P_RISCOS_MSGS_SEARCH; typedef const RISCOS_MSGS_SEARCH * PC_RISCOS_MSGS_SEARCH;

#if RISCOS
#define riscos_msgs_tag_compare short_memcmp32
#else
#define riscos_msgs_tag_compare memcmp /* use native function */
#endif /* OS */

PROC_BSEARCH_PROTO(static, riscos_msgs_search_compare, RISCOS_MSGS_SEARCH, RISCOS_MSGS_INDEX)
{
    BSEARCH_KEY_VAR_DECL(PC_RISCOS_MSGS_SEARCH, key);
    BSEARCH_DATUM_VAR_DECL(PC_RISCOS_MSGS_INDEX, datum);

    PCTSTR tag_key;
    PCTSTR tag_datum;

    const U32 tag_length_key   = key->tag_length;
    const U32 tag_length_datum = datum->tag_length;

  /*reportf("lookup(%u:%s) tag:msg %u:%s", tag_length_key, key->tag, tag_length_datum, datum->tag);*/

    if(tag_length_key != tag_length_datum)
        return((int) tag_length_key - (int) tag_length_datum);

    tag_key = key->tag;
    tag_datum = datum->tag;

    return(riscos_msgs_tag_compare(tag_key, tag_datum, tag_length_key));
}

_Check_return_
_Ret_z_
static PCTSTR
riscos_msg_lookup_using_index(
    _In_reads_(tag_length) PCTSTR tag,
    _InVal_     U32 tag_length,
    /*inout*/ P_RISCOS_MSGS p_msgs)
{
    U32 msg_idx;
    RISCOS_MSGS_SEARCH riscos_msgs_search;

    riscos_msgs_search.tag = tag;
    riscos_msgs_search.tag_length = tag_length;

    for(msg_idx = 0; msg_idx < p_msgs->n_msgs; ++msg_idx)
    {
        const int res = riscos_msgs_search_compare(&riscos_msgs_search, &p_msgs->msgs_index[msg_idx]);

        if(0 == res)
            return(p_msgs->msgs_index[msg_idx].tag + tag_length + 1);
    }

    return(NULL);
}

_Check_return_
_Ret_z_
static PCTSTR
riscos_msg_lookup_in_without_default(
    _In_reads_(tag_length) PCTSTR tag,
    _InVal_     U32 tag_length,
    /*inout*/ P_RISCOS_MSGS p_msgs)
{
    PCTSTR init_lookup_ptr;

    /* if there ain't a message block then we can't do lookup */
    if(NULL == p_msgs->msgs_block)
        return(NULL);

    if(NULL != p_msgs->msgs_index)
        return(riscos_msg_lookup_using_index(tag, tag_length, p_msgs));

    init_lookup_ptr = p_msgs->lookup_ptr;

    for(;;)
    {
        /* try resuming search at last position or at start */
        if(NULL == p_msgs->lookup_ptr)
            p_msgs->lookup_ptr = p_msgs->msgs_block;

        while(CH_NULL != *p_msgs->lookup_ptr)
        {
            const U32 lookup_length = tstrlen32(p_msgs->lookup_ptr);

            if(lookup_length > tag_length)
                if(*(p_msgs->lookup_ptr + tag_length) == TAG_DELIMITER)
                    if(0 == tstrrncmp(tag, p_msgs->lookup_ptr, tag_length))
                        return(p_msgs->lookup_ptr + tag_length + 1);

            /* step to next message */
            p_msgs->lookup_ptr += lookup_length + 1;

            if(p_msgs->lookup_ptr == init_lookup_ptr)
            {
                init_lookup_ptr = NULL;
                break; /* out of both loops */
            }
        }

        p_msgs->lookup_ptr = NULL; /* ensure restart */

        if(NULL == init_lookup_ptr)
            break;
    }

    return(NULL);
}

#define msgs_TAG_MAX  10 /*max length of a tag in characters*/

_Check_return_
_Ret_z_
static PCTSTR
riscos_msg_lookup_in_with_default(
    PCTSTR tag_and_default,
    /*inout*/ P_RISCOS_MSGS p_msgs)
{
    PCTSTR tag = tag_and_default;
    U32 tag_length;
    char tag_buffer[msgs_TAG_MAX + 1];
    PCTSTR default_ptr;
    PCTSTR ptr;

    if(NULL == (default_ptr = tstrchr(tag_and_default, TAG_DELIMITER)))
        tag_length = tstrlen32(tag);
    else
    {   /* need to copy to buffer, stopping at delimiter */
        tag_length = default_ptr - tag_and_default;
        xstrnkpy(tag_buffer, elemof32(tag_buffer), tag_and_default, tag_length);
        default_ptr++;
        tag = tag_buffer;
    }

    if(NULL != (ptr = riscos_msg_lookup_in_without_default(tag, tag_length, p_msgs)))
        return(ptr);

    /* return the default, or if that fails, the tag */
    return((NULL != default_ptr) ? default_ptr : tag_and_default);
}

#define MSG_PARSE_STATE_IN_TAG 0
#define MSG_PARSE_STATE_IN_MESSAGE 1

_Check_return_
static STATUS
riscos_msg_init(
    _InRef_     P_RISCOS_MSGS p_msgs)
{
    STATUS status = STATUS_OK;
    UINT pass;

    if(NULL == p_msgs->msgs_block)
        return(status);

    /*reportf("riscos_msg_init %s", p_msgs->msgs_block);*/

    for(pass = 1; pass <= 2; ++pass)
    {
        U32 n_msgs = 0;
        int msg_parse_state = MSG_PARSE_STATE_IN_TAG;
        P_U8 p_u8;
        P_U8 tag;

        for(p_u8 = tag = de_const_cast(P_U8, p_msgs->msgs_block); ; ++p_u8)
        {
            if(*p_u8 < CH_SPACE)
            {
                if((2 == pass) && (CH_NULL != *p_u8)) /* avoid writing to preprocessed read-only area */
                    *p_u8++ = CH_NULL; /* replace any CtrlChar with CH_NULL-termination */
                else
                    p_u8++; /* skip CtrlChar */

                if(CH_NULL == *p_u8)
                {   /* ended this pass of this file of tag,msg pairs */
                    break;
                }

                tag = p_u8; /* first character after any newline character */
                msg_parse_state = MSG_PARSE_STATE_IN_TAG;
                /* obviously another newline will reset harmlessly */
                continue;
            }

            /* ignore ':' in message part */
            if((TAG_DELIMITER == *p_u8) && (MSG_PARSE_STATE_IN_TAG == msg_parse_state))
            {
                /*reportf("[%u] tag:msg %s", pass, report_sbstr(tag));*/
                if(2 == pass)
                {
                    p_msgs->msgs_index[n_msgs].tag = tag;
                    p_msgs->msgs_index[n_msgs].tag_length = PtrDiffBytesU32(p_u8, tag);
                }

                msg_parse_state = MSG_PARSE_STATE_IN_MESSAGE;

                ++n_msgs;
                continue;
            }
        }

        if(1 == pass)
        {
            /*U32 n_bytes = PtrDiffBytesU32(p_u8, p_msgs->msgs_block);*/
            /*reportf("n_msgs[1]: %u %u", n_msgs, n_bytes);*/
            if(NULL == (p_msgs->msgs_index = al_ptr_calloc_bytes(P_RISCOS_MSGS_INDEX, sizeof32(p_msgs->msgs_index[0]) * n_msgs, &status)))
                break;
            p_msgs->n_msgs = n_msgs;
        }
        else /* 2 == pass */
        {   /* sort at some point */
            /*reportf("n_msgs[2]: %u", n_msgs);*/
            if(n_msgs != p_msgs->n_msgs) { /*reportf("n_msgs[2] != n_msgs[1]");*/ break; }
        }
    }

    return(status);
}

_Check_return_
static STATUS
resource_find_messages(
    _Out_writes_(elemof_buffer) PTSTR buffer,
    _InVal_     U32 elemof_buffer,
    _InVal_     OBJECT_ID object_id)
{
    TCHARZ name[BUF_MAX_PATHSTRING];

    consume_int(snprintf(name, elemof32(name),
                         TEXT("RISC_OS") FILE_DIR_SEP_TSTR TEXT("Messages") FILE_DIR_SEP_TSTR TEXT("Ob%.2u"),
                         (unsigned int) object_id));

    if(file_find_on_path(buffer, elemof_buffer, file_get_resources_path(), name) > 0)
        return(STATUS_DONE);

    return(STATUS_OK);
}

_Check_return_
static STATUS
riscos_messages_readfile_into(
    _In_z_      PCTSTR filename,
    _Out_       P_P_U8 p_p_u8)
{
    STATUS status = STATUS_OK;
    _kernel_osfile_block osfile_block;

    *p_p_u8 = NULL;

    zero_struct_fn(osfile_block);

    switch(_kernel_osfile(OSFile_ReadNoPath, filename, &osfile_block))
    {
    default: default_unhandled();
#if CHECKING
    case _kernel_ERROR:
    case OSFile_ObjectType_None:
    case OSFile_ObjectType_Dir:
    case OSFile_ObjectType_Image:
#endif
        break;

    case OSFile_ObjectType_File:
        {
        const U32 file_length = (U32) osfile_block.start /*[R4]*/;
        P_BYTE p_u8;

        if(NULL != (p_u8 = al_ptr_alloc_bytes(P_U8, file_length + 1, &status)))
        {
            zero_struct_fn(osfile_block);
            osfile_block.load = (int) p_u8;

            if(_kernel_ERROR == _kernel_osfile(OSFile_LoadNoPath, filename, &osfile_block))
            {
                al_ptr_dispose(P_P_ANY_PEDANTIC(&p_u8));
            }
            else
            {
                p_u8[file_length] = CH_NULL;
                *p_p_u8 = p_u8;
                status = STATUS_DONE;
            }
        }

        break;
        }
    }

    return(status);
}

_Check_return_
extern STATUS
resource_load_messages(
    _InVal_     OBJECT_ID object_id)
{
    TCHARZ resource_file[BUF_MAX_PATHSTRING];
    STATUS status = STATUS_OK;

    if(status_done(resource_find_messages(resource_file, elemof32(resource_file), object_id)))
    {
        const P_RESOURCES_PER_OBJECT p_resources_per_object = resource_statics.objects[object_id];
        P_U8 msgs_block = NULL;
      /*reportf("resource_load_messages(%u): load %s", object_id, resource_file);*/
        status_return(riscos_messages_readfile_into(resource_file, &msgs_block));
        p_resources_per_object->riscos.mangled_msgs.msgs_block = msgs_block;
        riscos_msg_init(&p_resources_per_object->riscos.mangled_msgs);
        return(STATUS_DONE);
    }

  /*reportf("resource_load_messages(%u): no match", object_id);*/
    return(status);
}

_Check_return_
_Ret_z_
extern PCTSTR
string_for_object(
    _In_z_      PCTSTR tag_and_default,
    _InVal_     OBJECT_ID object_id)
{
    const P_RESOURCES_PER_OBJECT p_resources_per_object = resource_statics.objects[object_id];
    return(riscos_msg_lookup_in_with_default(tag_and_default, &p_resources_per_object->riscos.mangled_msgs));
}

extern void
msgs_init(void);

extern void
msgs_init(void)
{
    assert0();
}

#endif /* RISCOS */

/* end of resource.c */
