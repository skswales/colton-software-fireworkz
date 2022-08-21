/* resource.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

typedef struct RISCOS_BOUND_MSGS
{
    PC_U8 msgs_block;
    PC_U8 lookup_ptr;
}
RISCOS_BOUND_MSGS, * P_RISCOS_BOUND_MSGS;

typedef struct RISCOS_LOADED_SPRITES
{
    UBF loaded_c  : 8;
    UBF loaded_22 : 8;
    UBF loaded_24 : 8;
    UBF reserved  : 8;
    P_SAH s_c;
    P_SAH s_22;
    P_SAH s_24;
}
RISCOS_LOADED_SPRITES, * P_RISCOS_LOADED_SPRITES;

#endif /* RISCOS */

typedef struct RESOURCE_STATICS
{
    PCTSTR p_str_dll_store;
    ARRAY_HANDLE bitmap_index;

#if RISCOS
    struct RESOURCE_STATICS_WINDOWS
    {
        RISCOS_BOUND_MSGS mangled_msgs[MAX_OBJECTS];
        RISCOS_LOADED_SPRITES sprites[MAX_OBJECTS];
    } riscos;
#elif WINDOWS
    struct RESOURCE_STATICS_WINDOWS
    {
        LIST_BLOCK bitmap_cache[MAX_OBJECTS];
        #if defined(NOT_ALL_IN_ONE)
        HINSTANCE library_handle[MAX_OBJECTS];
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
riscos_msg_lookup_in(
    PCTSTR tag_and_default,
    /*inout*/ P_RISCOS_BOUND_MSGS p_msgs);

#endif /* RISCOS */

/******************************************************************************
*
* load a new copy of a named bitmap from resource - must be freed later
*
******************************************************************************/

_Check_return_
extern RESOURCE_BITMAP_HANDLE
resource_bitmap_find(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id)
{
    RESOURCE_BITMAP_HANDLE h_bitmap;
    OBJECT_ID object_id;

    h_bitmap.i = 0;

    if(NULL == p_resource_bitmap_id)
        return(h_bitmap);

    object_id = p_resource_bitmap_id->object_id;

    if(OBJECT_ID_ENUM_START == object_id) /* scan all currently loaded objects */
    {
        RESOURCE_BITMAP_ID resource_bitmap_id = *p_resource_bitmap_id;

        resource_bitmap_id.object_id = OBJECT_ID_ENUM_START;

        while(status_ok(object_next(&resource_bitmap_id.object_id)))
        {
            h_bitmap = resource_bitmap_find(&resource_bitmap_id);

            if(h_bitmap.i)
                break;
        }

        return(h_bitmap);
    }

    myassert1x(IS_OBJECT_ID_VALID(object_id), TEXT("resource_bitmap_find(INVALID OBJECT_ID ") S32_TFMT TEXT(")"), object_id);

    if(!object_present(object_id))
        return(h_bitmap);

    {
#if RISCOS
    _kernel_swi_regs  rs;
    _kernel_oserror * p_kernel_oserror;
    P_SAH area;
    int scan_hi_res = (host_modevar_cache_current.XEig < 2) && (host_modevar_cache_current.YEig < 2);

    /* can't really use system-defined bitmaps here? */

    if(scan_hi_res)
    {
        if(NULL != (area = resource_statics.riscos.sprites[object_id].s_22))
        {
            rs.r[0] = 24 + 256; /* sprite_select in area */
            rs.r[1] = (int) area;
            rs.r[2] = (int) p_resource_bitmap_id->bitmap_name;

            if(NULL == (p_kernel_oserror = _kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs)))
            {
                h_bitmap.i = rs.r[2];
                return(h_bitmap);
            }
        }
    }

    if(NULL != (area = resource_statics.riscos.sprites[object_id].s_24))
    {
        rs.r[0] = 24 + 256; /* sprite_select in area */
        rs.r[1] = (int) area;
        rs.r[2] = (int) p_resource_bitmap_id->bitmap_name;

        if(NULL == (p_kernel_oserror = _kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs)))
        {
            h_bitmap.i = rs.r[2];
            return(h_bitmap);
        }
    }

    if(NULL != (area = resource_statics.riscos.sprites[object_id].s_c))
    {
        rs.r[0] = 24 + 256; /* sprite_select in area */
        rs.r[1] = (int) area;
        rs.r[2] = (int) p_resource_bitmap_id->bitmap_name;

        if(NULL == (p_kernel_oserror = _kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs)))
            h_bitmap.i = rs.r[2];
    }
#elif WINDOWS
    /* scan this object's cache for bitmap */
    STATUS status;
    const P_LIST_BLOCK p_list_block = &resource_statics.windows.bitmap_cache[object_id];
    const LIST_ITEMNO item = (LIST_ITEMNO) p_resource_bitmap_id->bitmap_id;
    HBITMAP * p_hBitmap = collect_goto_item(HBITMAP, p_list_block, item);
    /*STATUS status;*/
    if(NULL != p_hBitmap)
    {
        h_bitmap.i = *p_hBitmap;
    }
    else if(NULL != (p_hBitmap = collect_add_entry_elem(HBITMAP, p_list_block, P_DATA_NONE, item, &status)))
    {
        HINSTANCE hInstance = resource_get_object_resources(object_id);

        h_bitmap.i = *p_hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(p_resource_bitmap_id->bitmap_id));
    }
#endif /* OS */
    } /*block*/

    return(h_bitmap);
}

#if WINDOWS

_Check_return_
extern BOOL
resource_bitmap_find_new(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id,
    _OutRef_    P_RESOURCE_BITMAP_HANDLE p_resource_bitmap_handle,
    _OutRef_    P_GDI_SIZE p_bm_grid_size,
    _OutRef_    PUINT p_index)
{
    RESOURCE_BITMAP_ID resource_bitmap_id;

    resource_bitmap_id.object_id = p_resource_bitmap_id->object_id;
    resource_bitmap_id.bitmap_id = p_resource_bitmap_id->bitmap_id & ~T5_RESOURCE_BTTNCUR_MASK;

    p_bm_grid_size->cx = 0;
    p_bm_grid_size->cy = 0;

    if(p_resource_bitmap_id->bitmap_id & T5_RESOURCE_COMMON_BMP_BIT) /* index into one of our multi-image stores? */
    {
        *p_index = resource_bitmap_id.bitmap_id >> 8;
        resource_bitmap_id.bitmap_id &= 0xFF;

        if(tdd.uDPI >= 120)
        {
            resource_bitmap_id.bitmap_id += 1; /* try for high-dpi variant */
            *p_resource_bitmap_handle = resource_bitmap_find(&resource_bitmap_id);
            if(0 == p_resource_bitmap_handle->i)
                resource_bitmap_id.bitmap_id -= 1; /* stick with normal-dpi variant */
        }

        *p_resource_bitmap_handle = resource_bitmap_find(&resource_bitmap_id);

        if(0 != p_resource_bitmap_handle->i)
            status_assert(resource_bitmap_tool_size_query(&resource_bitmap_id, p_bm_grid_size));

        return(0 != p_resource_bitmap_handle->i);
    }

    *p_index = 0;
    *p_resource_bitmap_handle = resource_bitmap_find(&resource_bitmap_id);

    if(p_resource_bitmap_handle->i)
    {
        BITMAP bitmap;
        GetObject(p_resource_bitmap_handle->i, sizeof32(bitmap), &bitmap);
        p_bm_grid_size->cx = bitmap.bmWidth;
        p_bm_grid_size->cy = bitmap.bmHeight;
    }

    return(0 != p_resource_bitmap_handle->i);
}

#endif /* OS */

#if RISCOS

_Check_return_
extern RESOURCE_BITMAP_HANDLE
resource_bitmap_find_in_area(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id,
    _InVal_     S32 area_id)
{
    RESOURCE_BITMAP_HANDLE h_bitmap;
    OBJECT_ID object_id;

    h_bitmap.i = 0;

    if(NULL == p_resource_bitmap_id)
        return(h_bitmap);

    object_id = p_resource_bitmap_id->object_id;

    myassert1x(IS_OBJECT_ID_VALID(object_id), TEXT("resource_bitmap_find(INVALID OBJECT_ID ") S32_TFMT TEXT(")"), object_id);

    if(!object_present(object_id))
        return(h_bitmap);

    {
    P_SAH area;

    switch(area_id)
    {
    case RESOURCE_BITMAP_AREA_HI_RES:
        area = resource_statics.riscos.sprites[object_id].s_22;
        break;

    case RESOURCE_BITMAP_AREA_LO_RES:
        area = resource_statics.riscos.sprites[object_id].s_24;
        break;

    default: default_unhandled();
#if CHECKING
    case RESOURCE_BITMAP_AREA_STANDARD:
#endif
        area = resource_statics.riscos.sprites[object_id].s_c;
        break;
    }

    if(area)
    {
        _kernel_swi_regs  rs;
        _kernel_oserror * p_kernel_oserror;

        rs.r[0] = 24 + 256; /* sprite_select in area */
        rs.r[1] = (int) area;
        rs.r[2] = (int) p_resource_bitmap_id->bitmap_name;

        if(NULL == (p_kernel_oserror = _kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs)))
            h_bitmap.i = rs.r[2];
    }
    } /*block*/

    return(h_bitmap);
}

#endif /* OS */

_Check_return_
extern RESOURCE_BITMAP_HANDLE
resource_bitmap_find_defaulting(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id)
{
    RESOURCE_BITMAP_HANDLE h_bitmap;

    h_bitmap = resource_bitmap_find(p_resource_bitmap_id);

#if RISCOS
    if(!h_bitmap.i)
    {
        h_bitmap = resource_bitmap_find_system(p_resource_bitmap_id);

        if(h_bitmap.i)
        {   /* return a bodged pointer to the name rather than an address */
            h_bitmap.p_u8 = de_const_cast(P_U8, p_resource_bitmap_id->bitmap_name);
            h_bitmap.i |= RESOURCE_BITMAP_HANDLE_RISCOS_BODGE;
        }
    }
#endif /* OS */

    return(h_bitmap);
}

#if RISCOS

_Check_return_
extern RESOURCE_BITMAP_HANDLE
resource_bitmap_find_system(
    _InRef_     PC_RESOURCE_BITMAP_ID p_resource_bitmap_id)
{
    RESOURCE_BITMAP_HANDLE h_bitmap;
    _kernel_swi_regs rs;
    _kernel_oserror * p_kernel_oserror;
    SAH * area[2];
    U32 area_idx;

    h_bitmap.i = 0;

    /* have a look in Window Manager's back pockets */
    void_WrapOsErrorChecking(_kernel_swi(/*Wimp_BaseOfSprites*/ 0x000400EA, &rs, &rs));

    area[0] = (P_ANY) rs.r[1]; /* RAM */
    area[1] = (P_ANY) rs.r[0]; /* ROM */

    for(area_idx = 0; area_idx <= 1; ++area_idx)
    {
        if(area[area_idx])
        {
            rs.r[0] = 24 + 256; /* sprite_select in area */
            rs.r[1] = (int) area[area_idx];
            rs.r[2] = (int) p_resource_bitmap_id->bitmap_name;

            if(NULL == (p_kernel_oserror = _kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs)))
            {
                h_bitmap.i = rs.r[2];
                break;
            }
        }
    }

    return(h_bitmap);
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
    /*inout*/ P_RESOURCE_BITMAP_HANDLE p_h_bitmap)
{
    if(0 != p_h_bitmap->i)
    {
#if RISCOS
        /* Don't need to throw away on RISC OS */
#elif WINDOWS
        /* Decrement cache ref count if we can be bothered */
#endif
        p_h_bitmap->i = 0;
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
        rs.r[0] = 40 + 256;
        rs.r[1] = 1; /* Use Window Manager's Sprite Pools */
        rs.r[2] = resource_bitmap_handle.i & ~RESOURCE_BITMAP_HANDLE_RISCOS_BODGE;

        ok = (NULL == _kernel_swi(/*Wimp_SpriteOp*/ 0x000400E9, &rs, &rs));
    }
    else
    {
        rs.r[0] = 40 + 512;
        rs.r[1] = (int) 0x89ABFEDC; /* kill the OS or any twerp who dares to access this! */
        rs.r[2] = resource_bitmap_handle.i;

        ok = (NULL == _kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs));
    }

    if(!ok)
    {
        p_size->cx = p_size->cy = 0;
        return;
    }

    { /* read info on defining screen mode */
    S32 XEigFactor, YEigFactor;
    host_modevar_cache_query_eigs(rs.r[6], &XEigFactor, &YEigFactor);
    p_size->cx = (GDI_COORD) rs.r[3] << XEigFactor;
    p_size->cy = (GDI_COORD) rs.r[4] << YEigFactor;
    } /*block*/
}

#endif /* OS */

_Check_return_
extern STATUS
resource_close(
    _InVal_     OBJECT_ID object_id)
{
    assert(IS_OBJECT_ID_VALID(object_id));

#if RISCOS
    if(resource_statics.riscos.sprites[object_id].loaded_24)
    {
        resource_statics.riscos.sprites[object_id].loaded_24 = 0;
        al_ptr_dispose(P_P_ANY_PEDANTIC(&resource_statics.riscos.sprites[object_id].s_24));
    }

    if(resource_statics.riscos.sprites[object_id].loaded_22)
    {
        resource_statics.riscos.sprites[object_id].loaded_22 = 0;
        al_ptr_dispose(P_P_ANY_PEDANTIC(&resource_statics.riscos.sprites[object_id].s_22));
    }

    if(resource_statics.riscos.sprites[object_id].loaded_c)
    {
        resource_statics.riscos.sprites[object_id].loaded_c = 0;
        al_ptr_dispose(P_P_ANY_PEDANTIC(&resource_statics.riscos.sprites[object_id].s_c));
    }
#elif WINDOWS
    if(collect_has_data(&resource_statics.windows.bitmap_cache[object_id]))
    {   /* blow away cached bitmaps or else Mr GDI gets all upset */
        const P_LIST_BLOCK p_list_block = &resource_statics.windows.bitmap_cache[object_id];
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
    if(resource_statics.windows.library_handle[object_id])
    {
        FreeLibrary(resource_statics.windows.library_handle[object_id]); /* now kosher 'cos only resources out there */
        resource_statics.windows.library_handle[object_id] = 0;
    }
#endif
#endif /* OS */

    /* remove all entries that belong to this object from the resource_bitmap_index array */
    {
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

    assert(IS_OBJECT_ID_VALID(object_id));

    consume_int(tstr_xsnprintf(name, elemof32(name), resource_statics.p_str_dll_store, (U32) object_id));

    if(file_find_on_path(tstr_buf, elemof_buffer, name) > 0)
        return(STATUS_OK);

    tstr_buf[0] = CH_NULL;
    return(STATUS_FAIL);
}

extern void
resource_dll_free(
    _InVal_     OBJECT_ID object_id)
{
#if WINDOWS && defined(NOT_ALL_IN_ONE)
    if(resource_statics.windows.library_handle[object_id])
    {
        FreeLibrary(resource_statics.windows.library_handle[object_id]);
        resource_statics.windows.library_handle[object_id] = 0;
    }
#else
    IGNOREPARM_InVal_(object_id);
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
    P_SAH * p_p_src = &resource_statics.riscos.sprites[object_id_src].s_22;
    P_SAH * p_p_dst = &resource_statics.riscos.sprites[object_id_dst].s_22;
    if(*p_p_src)
    {
        *p_p_dst = *p_p_src;
        *p_p_src = NULL;
    }
    } /*block*/
#else
    IGNOREPARM_InVal_(object_id_dst);
    IGNOREPARM_InVal_(object_id_src);
#endif
}

#endif /* UNUSED_KEEP_ALIVE */

_Check_return_
extern STATUS
resource_init(
    _InVal_     OBJECT_ID object_id,
    _In_opt_    PC_U8 * const p_u8_bound_msg,
    _InRef_opt_ PC_BOUND_RESOURCES p_bound)
{
    assert(IS_OBJECT_ID_VALID(object_id));

#if RISCOS
    assert(NULL != p_u8_bound_msg);
    resource_statics.riscos.mangled_msgs[object_id].msgs_block = (PC_U8) p_u8_bound_msg;

    if(NULL != p_bound)
    {
        /* load common sprites */
        if(p_bound->sprite_area_c)
            resource_statics.riscos.sprites[object_id].s_c = p_bound->sprite_area_c;

        /* load mode-dependent sprites */
        if(p_bound->sprite_area_22)
            resource_statics.riscos.sprites[object_id].s_22 = p_bound->sprite_area_22;

        if(p_bound->sprite_area_24)
            resource_statics.riscos.sprites[object_id].s_24 = p_bound->sprite_area_24;
    }
#elif WINDOWS
    IGNOREPARM_CONST(p_u8_bound_msg);
    assert(MSG_WEAK == p_u8_bound_msg);

#if defined(NOT_ALL_IN_ONE)
    if(NULL == resource_statics.windows.library_handle[object_id])
    {
        TCHARZ resource_file[BUF_MAX_PATHSTRING];

        if(NULL != p_bound)
        {
            if(status_ok(resource_dll_find(object_id, resource_file, elemof32(resource_file))))
            {
                resource_statics.windows.library_handle[object_id] = LoadLibrary(resource_file);

                if((UINT) resource_statics.windows.library_handle[object_id] <= (UINT) HINSTANCE_ERROR)
                {
                    resource_statics.windows.library_handle[object_id] = NULL;   /* just-in-case */
                    return(STATUS_MODULE_NOT_FOUND);
                }
            }
        }
    }
#else
    /* never load any DLL resources */
    IGNOREPARM_InRef_(p_bound);
    IGNOREPARM_InVal_(object_id);
#endif

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
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    *p_p_sah = NULL;

    rs.r[0] = 5;
    rs.r[1] = (int) filename;
    void_WrapOsErrorChecking(_kernel_swi(OS_File, &rs, &rs));

    if(rs.r[0] == 1)
    {
        int arealen = rs.r[4] + 4; /* file is sprite area without the length word */

        if(NULL != (*p_p_sah = al_ptr_alloc_bytes(P_SAH, arealen, &status)))
        {
            (*p_p_sah)->area_size = arealen; /* first we must initialise it ourselves */
            (*p_p_sah)->offset_to_first = 16;

            rs.r[0] = 9 + 256; /* sprite area initialise */
            rs.r[1] = (int) *p_p_sah;
            rs.r[2] = arealen;

            void_WrapOsErrorChecking(_kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs));

            rs.r[0] = 10 + 256; /* sprite area load */
            rs.r[1] = (int) *p_p_sah;
            rs.r[2] = (int) filename;

            if(NULL != (e = _kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs)))
                al_ptr_dispose((void **) p_p_sah);
        }
    }

    return(status);
}

_Check_return_
extern STATUS
resource_load_sprites(
    _InVal_     OBJECT_ID object_id,
    _In_        UINT which)
{
    TCHARZ name[BUF_MAX_PATHSTRING];
    TCHARZ resource_file[BUF_MAX_PATHSTRING];
    STATUS status = STATUS_OK;

    switch(which)
    {
    default:
        break;

    case 24:
        /* same as below if you can be bothered, just replace 22 by 24 */
        break;

    case 22:
        strcat(strcpy(resource_file, resource_statics.p_str_dll_store), "_22"); /* abuse this convenient store */

        consume_int(snprintf(name, elemof32(name), resource_file, (S32) object_id));

        if(file_find_on_path(resource_file, elemof32(resource_file), name) > 0)
        {
            riscos_sprite_readfile_into(resource_file, &resource_statics.riscos.sprites[object_id].s_22);
            resource_statics.riscos.sprites[object_id].loaded_22 = 1;
            status = STATUS_DONE;
        }

        break;
    }

    return(status);
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
resource_startup(
    _In_z_      PCTSTR p_str_dll_store)
{
    resource_statics.p_str_dll_store = p_str_dll_store;
}

#if WINDOWS

_Check_return_
extern HINSTANCE
resource_get_object_resources(
    _InVal_     OBJECT_ID object_id)
{
    HINSTANCE hInstance;

    assert(IS_OBJECT_ID_VALID(object_id));

#if defined(NOT_ALL_IN_ONE)
    hInstance = resource_statics.windows.library_handle[object_id];
#else
    IGNOREPARM_InVal_(object_id);

    hInstance = GetInstanceHandle();
#endif

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
    BOOL is_err;

    if(0 == status)
        return(NULL);

    is_err = resource_split_status(status, &status_offset, &object_id);

    if(IS_OBJECT_ID_VALID(object_id));
    {
        TCHARZ tagbuffer[sizeof32("e32767")];
        PC_SBSTR str;

        consume_int(tstr_xsnprintf(tagbuffer, elemof32(tagbuffer), is_err ? "e%u" : "%u", (unsigned int) status_offset));

        str = riscos_msg_lookup_in(tagbuffer, &resource_statics.riscos.mangled_msgs[object_id]);

        if(str != tagbuffer)
            return((PTSTR) str);
    }

    /* lookup failed */
    assert(IS_OBJECT_ID_VALID(object_id));
    return(NULL);
}

#elif WINDOWS

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
static BOOL
resource_lookup_tstr_buffer_no_default(
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     STATUS status)
{
    OBJECT_ID object_id;
    U16 status_offset;
    BOOL is_err;
    UINT uint;

    is_err = resource_split_status(status, &status_offset, &object_id);

    assert(IS_OBJECT_ID_VALID(object_id));

    if(is_err)
        status_offset += 0x8000U;

    PTR_ASSERT(tstr_buf);
    assert(0 != elemof_buffer);

    switch(object_id)
    {
    case 0:
        if(0 == status)
        {
            tstr_buf[0] = CH_NULL;
            return(FALSE);
        }
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
#define N_DEFAULT_MSGS 128
        uint = 0x8000U;
        uint -= N_DEFAULT_MSGS * MAX_OBJECTS;
        uint += N_DEFAULT_MSGS * (UINT) object_id;
        uint += status_offset;
        break;
    }

    if(!LoadString(GetInstanceHandle(), uint, tstr_buf, elemof_buffer-1))
    {
        /* lookup failed */
        tstr_buf[0] = CH_NULL;
        return(FALSE);
    }

    return(TRUE);
}

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
    PCTSTR tstr = resource_lookup_tstr_no_default(status);

    if(NULL == tstr)
        return(STATUS_OK);

    status_return(quick_tblock_tstr_add(p_quick_tblock, tstr));

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
        _kernel_swi_regs rs;

        consume_int(xsnprintf(sbstr_buf, elemof_buffer, "file_%.3x", t5_filetype));

        /* now to check if the sprite exists: do a sprite_select on the Window Manager sprite pool */
        rs.r[0] = 24;
        rs.r[2] = (int) sbstr_buf;
        if(NULL == _kernel_swi(/*Wimp_SpriteOp*/ 0x000400E9, &rs, &rs))
            return;

        /* the sprite does not exist: use general don't-know icon. */
        } /*block*/

        /*FALLTHRU*/

    case FILETYPE_UNDETERMINED:
    case FILETYPE_UNTYPED:
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
    return(resource_statics.riscos.sprites[object_id].s_c);
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

#define INVALID_MSGS_BLOCK ((void *) 1)

#define msgs_TAG_MAX  10 /*max length of a tag in characters*/

_Check_return_
_Ret_z_
static PCTSTR
riscos_msg_lookup_in(
    PCTSTR tag_and_default,
    /*inout*/ P_RISCOS_BOUND_MSGS p_msgs)
{
    PCTSTR tag = tag_and_default;
    U32 tag_length;
    char tag_buffer[msgs_TAG_MAX + 1];
    PCTSTR default_ptr;
    PCTSTR init_lookup_ptr;

    if(NULL == (default_ptr = tstrchr(tag_and_default, TAG_DELIMITER)))
        tag_length = tstrlen32(tag);
    else
    {   /* need to copy to buffer, stopping at delimiter */
        tag_length = default_ptr - tag_and_default;
        xstrnkpy(tag_buffer, elemof32(tag_buffer), tag_and_default, tag_length);
        default_ptr++;
        tag = tag_buffer;
    }

    /* if there ain't a message block or it's bad then we can't do lookup */
    if(p_msgs->msgs_block  &&  (p_msgs->msgs_block != INVALID_MSGS_BLOCK))
    {
        init_lookup_ptr = p_msgs->lookup_ptr;

        for(;;)
        {
            /* try resuming search at last position or at start */
            if(!p_msgs->lookup_ptr)
                p_msgs->lookup_ptr = p_msgs->msgs_block;

            while(*p_msgs->lookup_ptr)
            {
                U32 lookup_length = tstrlen32(p_msgs->lookup_ptr);

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

            if(!init_lookup_ptr)
                break;
        }
    }

    /* return the default, or if that fails, the tag */
    return(default_ptr ? default_ptr : tag_and_default);
}

_Check_return_
_Ret_z_
extern PCTSTR
string_for_object(
    _In_z_      PCTSTR tag_and_default,
    _InVal_     OBJECT_ID object_id)
{
    return(riscos_msg_lookup_in(tag_and_default, &resource_statics.riscos.mangled_msgs[object_id]));
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
