/* im_cache.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that handles image file cache */

/* SKS 12-Sep-1991 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef          __im_cache_h
#include "cmodules/im_cache.h"
#endif

#ifndef          __gr_diag_h
#include "cmodules/gr_diag.h"
#endif

#include "cmodules/collect.h"

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"

#define PicConvert_BMPtoFF9 0x00048E00
#endif

#if WINDOWS
#include "external/Dial_Solutions/drawfile.h"
#endif

#if WINDOWS
#include "ob_skel/ho_gdip_image.h"
#endif

#if !defined(_tremove)
#define _tremove    remove
#endif

/*
internal structure
*/

/*
entries in the image file cache
*/

typedef struct IMAGE_CACHE_DRAWFILE_ENTRY
{
    UBF no_autokill : 1;
    UBF refs : sizeof(int)*8 - 1;

    STATUS error;

    T5_FILETYPE t5_filetype_original;   /* original image data format file type */

    ARRAY_HANDLE h_data_original;       /* original data, whatever the image data format */

    ARRAY_HANDLE h_data_draw;           /* only ever contains a Draw file */

#if WINDOWS
    GdipImage gdip_image;
#endif /* OS */

    /* name follows here at (p_image_cache + 1) */
}
IMAGE_CACHE, * P_IMAGE_CACHE;

_Check_return_
static inline LIST_ITEMNO
list_itemno_from_image_cache_handle(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle)
{
    return((LIST_ITEMNO) (uintptr_t) (image_cache_handle));
}

/*
internal functions
*/

#if WINDOWS

static void
image_cache_add_win_font_table(
    _InoutRef_  P_ARRAY_HANDLE p_h_data_draw);

#endif /* OS */

_Check_return_
_Ret_maybenull_
static P_IMAGE_CACHE
image_cache_entry_new(
    _In_z_      PCTSTR name,
    _InVal_     T5_FILETYPE t5_filetype,
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _OutRef_    P_STATUS p_status);

_Check_return_
static STATUS
image_cache_load(
    P_IMAGE_CACHE p_image_cache);

_Check_return_
static STATUS
image_cache_load_file_to_handle(
    _OutRef_    P_ARRAY_HANDLE p_h_data,
    _In_z_      PCTSTR filename,
    _OutRef_    P_T5_FILETYPE p_t5_filetype);

_Check_return_
static STATUS
image_cache_process_loaded_or_embedded_data(
    P_IMAGE_CACHE p_image_cache,
    _In_        PC_ARRAY_HANDLE p_h_data, /* args may mutate */
    _In_        U32 n_bytes,
    _In_        T5_FILETYPE t5_filetype);

static void
image_cache_rebind(
    _InoutRef_  P_ARRAY_HANDLE p_h_data_draw /*inout, certainly poked*/);

#if RISCOS

static void
image_cache_spot_draw_bmp_object(
    _InoutRef_  P_ARRAY_HANDLE p_h_data_draw);

#elif WINDOWS

static void
image_cache_spot_draw_jpeg_object(
    _InoutRef_  P_IMAGE_CACHE p_image_cache);

#endif /* OS */

#define HIMETRIC_PER_INCH 2540 /* 25.4 mm/inch * 100.0 hm/mm */

#define DrawUnitsFromHiMetric(hm) ( \
    muldiv64_ceil((hm), GR_RISCDRAW_PER_INCH, HIMETRIC_PER_INCH) )

/*
the cache descriptor list
*/

static LIST_BLOCK
image_cache;

_Check_return_
_Ret_maybenull_
static P_IMAGE_CACHE
image_cache_goto_item(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle)
{
    return(collect_goto_item(IMAGE_CACHE, &image_cache, list_itemno_from_image_cache_handle(image_cache_handle)));
}

_Check_return_
_Ret_maybenull_
static P_IMAGE_CACHE
image_cache_first_item(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle)
{
    return(collect_first(IMAGE_CACHE, &image_cache, (P_LIST_ITEMNO) p_image_cache_handle));
}

_Check_return_
_Ret_maybenull_
static P_IMAGE_CACHE
image_cache_next_item(
    _InoutRef_  P_IMAGE_CACHE_HANDLE p_image_cache_handle)
{
    return(collect_next(IMAGE_CACHE, &image_cache, (P_LIST_ITEMNO) p_image_cache_handle));
}

/******************************************************************************
*
* query of RISC OS filetypes that might sensibly be loaded
*
******************************************************************************/

_Check_return_
extern BOOL
image_cache_can_load(
    _InVal_     T5_FILETYPE t5_filetype)
{
    switch(t5_filetype)
    {
    case FILETYPE_DRAW:
    case FILETYPE_VECTOR:
    case FILETYPE_POSTER:
    case FILETYPE_SPRITE:
    case FILETYPE_JPEG:
#if WINDOWS
    case FILETYPE_ICO:
    case FILETYPE_GIF:
    case FILETYPE_BMP:
    case FILETYPE_WMF:
    case FILETYPE_PNG:
    case FILETYPE_TIFF:
    case FILETYPE_WINDOWS_EMF:
#endif /* OS */
        return(TRUE);

    default:
        return(FALSE);
    }
}

_Check_return_
extern BOOL
image_cache_can_import_with_image_convert(
    _InVal_     T5_FILETYPE t5_filetype)
{
    if(image_cache_can_load(t5_filetype))
        return(TRUE);

    if(image_convert_can_convert(t5_filetype))
        return(TRUE);

    return(FALSE);
}

#if WINDOWS

_Check_return_
static BOOL
image_cache_convert_data_for_wrap(
    P_IMAGE_CACHE p_image_cache,
    _OutRef_    P_ARRAY_HANDLE p_h_data,
    _OutRef_    P_T5_FILETYPE p_t5_filetype)
{
    BOOL ok = TRUE;
    PCTSTR tempname;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, BUF_MAX_PATHSTRING);
    quick_tblock_with_buffer_setup(quick_tblock);

    *p_h_data = 0;
    *p_t5_filetype = FILETYPE_UNDETERMINED;

    if(status_fail(file_tempname_null(TEXT("cv"), FILE_EXT_SEP_TSTR TEXT("bmp"), 0, &quick_tblock)))
        ok = FALSE;

    tempname = quick_tblock_tstr(&quick_tblock);

    if(ok)
        ok = GdipImage_SaveAs_BMP(p_image_cache->gdip_image, _wstr_from_tstr(tempname));

    if(ok)
        if(status_fail(image_cache_load_file_to_handle(p_h_data, tempname, p_t5_filetype)))
            ok = FALSE;

    quick_tblock_dispose(&quick_tblock);

    if(ok)
        if(FILETYPE_UNDETERMINED == *p_t5_filetype)
            *p_t5_filetype = FILETYPE_BMP;

    return(ok);
}

#endif /* OS */

/******************************************************************************
*
* --out--
* -ve = error
*   0 = same as an existing entry's original data (data not stolen)
*   1 = entry created (data stolen)
*
******************************************************************************/

_Check_return_
extern STATUS
image_cache_embedded(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _InoutRef_  P_ARRAY_HANDLE p_h_data /*inout;maybe stolen, certainly poked*/,
    _InVal_     T5_FILETYPE t5_filetype_original)
{
    const U32 n_bytes_original = array_elements32(p_h_data);
    IMAGE_CACHE_HANDLE image_cache_handle;
    P_IMAGE_CACHE p_image_cache;
    T5_FILETYPE t5_filetype = t5_filetype_original;
    U32 n_bytes = n_bytes_original;
    PTSTR converted_name = NULL;
    ARRAY_HANDLE h_data_converted = 0;
    STATUS status;

    for(p_image_cache = image_cache_first_item(&image_cache_handle);
        p_image_cache;
        p_image_cache = image_cache_next_item(&image_cache_handle))
    {
        /* always compare original data */
        if(n_bytes != array_elements32(&p_image_cache->h_data_original))
            continue;

        if(0 == memcmp32(array_rangec(&p_image_cache->h_data_original, BYTE, 0, n_bytes),
                         array_rangec(p_h_data, BYTE, 0, n_bytes),
                         n_bytes))
        {   /* bingo! a direct hit. tell the caller which one his data matched */
            *p_image_cache_handle = image_cache_handle;
            return(STATUS_OK);
        }
    }

    if(NULL == (p_image_cache = image_cache_entry_new(tstr_empty_string, t5_filetype, p_image_cache_handle, &status)))
        return(status);

    for(;;) /* loop for structure */
    {
        if(image_cache_can_load(t5_filetype_original))
            break;

        if(!image_convert_can_convert(t5_filetype_original))
        {
            trace_1(TRACE_MODULE_GR_CHART, TEXT("image_cache_embedded: cannot convert: type 0x%x"), t5_filetype_original);
            status = status_check();
            break;
        }

        if(status_fail(status = image_convert_do_convert_data(&converted_name, &t5_filetype, array_rangec(p_h_data, BYTE, 0, n_bytes), n_bytes, t5_filetype_original)))
        {
            trace_1(TRACE_MODULE_GR_CHART, TEXT("image_cache_embedded: failed to convert: type 0x%x"), t5_filetype_original);
            break;
        }

        { /* read the converted file */
        T5_FILETYPE t5_filetype_loaded;

        if(status_fail(status = image_cache_load_file_to_handle(&h_data_converted, converted_name, &t5_filetype_loaded)))
            break;

        if(FILETYPE_UNDETERMINED != t5_filetype_loaded)
        {
            assert(t5_filetype == t5_filetype_loaded);
            t5_filetype = t5_filetype_loaded;
        }
        } /*block*/

        n_bytes = array_elements32(&h_data_converted);

        break; /* out of loop for structure */
        /*NOTREACHED*/
    }

    /* iff converted, n_bytes and t5_filetype now pertain to h_converted_data; otherwise to h_data_original */
    if(status_ok(status))
        status = image_cache_process_loaded_or_embedded_data(p_image_cache, (0 != h_data_converted) ? &h_data_converted : p_h_data, n_bytes, t5_filetype);

    if(status_ok(status))
        status = STATUS_DONE;

    al_array_dispose(&h_data_converted);

    if(NULL != converted_name)
    {
        // <<< (void) _tremove(converted_name);
        tstr_clr(&converted_name);
    }

    if(status_fail(status))
    {   /* don't retain handle to cache entry for failed embedded data */
        image_cache_entry_remove(*p_image_cache_handle);
        *p_image_cache_handle = IMAGE_CACHE_HANDLE_NONE;
        return(status);
    }

    /* that all worked, so steal the original data */
    p_image_cache->h_data_original = *p_h_data;
    *p_h_data = 0;

    return(status);
}

/******************************************************************************
*
* scans cache list for identical entry
*
* if a match is found, this entry is trashed and the caller's ref updated
*
******************************************************************************/

/*ncr*/
extern STATUS
image_cache_embedded_updating_entry(
    _InoutRef_  P_IMAGE_CACHE_HANDLE p_image_cache_handle)
{
    P_IMAGE_CACHE this_dcep = image_cache_goto_item(*p_image_cache_handle);
    U32 n_bytes;
    IMAGE_CACHE_HANDLE image_cache_handle;
    P_IMAGE_CACHE p_image_cache;

    if(NULL == this_dcep)
    {
        PTR_ASSERT(this_dcep);
        return(status_check());
    }

    n_bytes = array_elements32(&this_dcep->h_data_original);

    for(p_image_cache = image_cache_first_item(&image_cache_handle);
        p_image_cache;
        p_image_cache = image_cache_next_item(&image_cache_handle))
    {
        if(this_dcep == p_image_cache)
            continue;

        {
        PCTSTR entryname = (PCTSTR) (p_image_cache + 1);

        if(CH_NULL != *entryname)
            continue;
        } /*block*/

        /* always compare original data */
        if(n_bytes != array_elements32(&p_image_cache->h_data_original))
            continue;

        if(0 == memcmp32(array_rangec(&p_image_cache->h_data_original, BYTE, 0, n_bytes),
                         array_rangec(&this_dcep->h_data_original, BYTE, 0, n_bytes),
                         n_bytes))
        {   /* bingo! a direct hit. blow the caller's entry away, replacing with what we found */
            image_cache_entry_remove(*p_image_cache_handle);
            *p_image_cache_handle = image_cache_handle;
            return(STATUS_OK);
        }
    }

    status_return(image_cache_entry_rename(*p_image_cache_handle, NULL));

    return(STATUS_DONE);
}

/******************************************************************************
*
* create an entry for this in the cache
*
* --out--
* -ve = error
*
******************************************************************************/

_Check_return_
extern STATUS
image_cache_entry_create(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _In_z_      PCTSTR name,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status;

    if(NULL != image_cache_entry_new(name, t5_filetype, p_image_cache_handle, &status))
        return(status);

    return(STATUS_DONE);
}

/******************************************************************************
*
* given the details of a Draw file, make sure an entry for it is in the cache
*
* --out--
* -ve = error
*   1 = entry exists
*
******************************************************************************/

_Check_return_
extern STATUS
image_cache_entry_ensure(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _In_z_      PCTSTR name,
    _InVal_     T5_FILETYPE t5_filetype)
{
    if(image_cache_entry_query(p_image_cache_handle, name))
        return(STATUS_DONE);

    return(image_cache_entry_create(p_image_cache_handle, name, t5_filetype));
}

/******************************************************************************
*
* create a list entry
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_IMAGE_CACHE
image_cache_entry_new(
    _In_z_      PCTSTR name,
    _InVal_     T5_FILETYPE t5_filetype,
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _OutRef_    P_STATUS p_status)
{
    static LIST_ITEMNO cahkey_gen = 0x42516000; /* NB. not tbs! */

    LIST_ITEMNO key = cahkey_gen++;
    U32 namlenp1 = tstrlen32p1(name);
    P_IMAGE_CACHE p_image_cache;

    *p_image_cache_handle = IMAGE_CACHE_HANDLE_NONE;

    if(NULL != (p_image_cache = collect_add_entry_bytes(IMAGE_CACHE, &image_cache, P_DATA_NONE, sizeof32(*p_image_cache) + (sizeof32(*name) * namlenp1), key, p_status)))
    {
        zero_struct_ptr(p_image_cache);

        p_image_cache->t5_filetype_original = t5_filetype;

        memcpy32((p_image_cache + 1), name, (sizeof32(*name) * namlenp1));

        *p_image_cache_handle = (IMAGE_CACHE_HANDLE) key;
    }

    return(p_image_cache);
}

/******************************************************************************
*
* given the details of a Draw file, query whether an entry for it is in the cache
*
******************************************************************************/

_Check_return_
extern BOOL
image_cache_entry_query(
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _In_z_      PCTSTR name)
{
    BOOL rooted = file_is_rooted(name);
    PCTSTR leaf = file_leafname(name);
    IMAGE_CACHE_HANDLE image_cache_handle;
    P_IMAGE_CACHE p_image_cache;

    /* search for the file on the list */
    *p_image_cache_handle = IMAGE_CACHE_HANDLE_NONE;

    for(p_image_cache = image_cache_first_item(&image_cache_handle);
        p_image_cache;
        p_image_cache = image_cache_next_item(&image_cache_handle))
    {
        PCTSTR testname  = name;
        PCTSTR entryname = (PCTSTR) (p_image_cache + 1);

        if(!rooted)
        {
            testname  = leaf;
            entryname = file_leafname(entryname);
        }

        if(0 == tstricmp(testname, entryname))
        {
            *p_image_cache_handle = (IMAGE_CACHE_HANDLE) image_cache_handle;

            trace_2(TRACE_MODULE_GR_CHART, TEXT("image_cache_entry_query found file, handle: ") ENUM_XTFMT TEXT(" ") S32_TFMT TEXT(", in list"), *p_image_cache_handle, image_cache_handle);
            trace_0(TRACE_MODULE_GR_CHART, TEXT("image_cache_entry_query yields TRUE"));
            return(TRUE);
        }
    }

    trace_0(TRACE_MODULE_GR_CHART, TEXT("image_cache_entry_query yields FALSE"));
    return(FALSE);
}

static void
image_cache_entry_data_remove(
    P_IMAGE_CACHE p_image_cache)
{
#if WINDOWS
    GdipImage_Dispose(&p_image_cache->gdip_image);
#endif /* OS */

    p_image_cache->t5_filetype_original = FILETYPE_UNDETERMINED;

    al_array_dispose(&p_image_cache->h_data_original);

    al_array_dispose(&p_image_cache->h_data_draw);
}

extern void
image_cache_entry_remove(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle)
{
    P_IMAGE_CACHE p_image_cache;

    if(NULL == (p_image_cache = image_cache_goto_item(image_cache_handle)))
    {
        assert0();
        return;
    }

    image_cache_entry_data_remove(p_image_cache);

    collect_subtract_entry(&image_cache, list_itemno_from_image_cache_handle(image_cache_handle));
    collect_compress(&image_cache);
}

_Check_return_
extern STATUS
image_cache_entry_rename(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle,
    _In_opt_z_  PCTSTR name)
{
    P_IMAGE_CACHE p_image_cache;

    trace_2(TRACE_MODULE_GR_CHART, TEXT("image_cache_entry_rename(") ENUM_XTFMT TEXT(", %s)"), image_cache_handle, report_tstr(name));

    if(NULL == (p_image_cache = image_cache_goto_item(image_cache_handle)))
        return(0);

    /* if room to fit new name simply reuse else reallocate */
    if(NULL != name)
    {
        U32 namlenp1 = tstrlen32p1(name);

        if(tstrlen32p1((PCTSTR) (p_image_cache + 1)) < namlenp1)
        {
            const LIST_ITEMNO key = list_itemno_from_image_cache_handle(image_cache_handle);
            STATUS status;

            collect_subtract_entry(&image_cache, key); /* SKS 24feb2012 was missing */

            if(NULL == (p_image_cache = collect_add_entry_bytes(IMAGE_CACHE, &image_cache, P_DATA_NONE, sizeof32(IMAGE_CACHE) + (sizeof32(*name) * namlenp1), key, &status)))
                return(status);
        }

        memcpy32((p_image_cache + 1), name, (sizeof32(*name) * namlenp1));
    }
    else
    {
        * (PTSTR) (p_image_cache + 1) = CH_NULL;
    }

    return(1);
}

#if defined(UNUSED_KEEP_ALIVE)

/******************************************************************************
*
* set state such that entry is removed when refs go to zero
*
******************************************************************************/

_Check_return_
extern STATUS
image_cache_entry_clear_autokill(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle)
{
    const P_IMAGE_CACHE p_image_cache = image_cache_goto_item(image_cache_handle);

    if(NULL != p_image_cache)
    {
        p_image_cache->no_autokill = 1;

        return(1);
    }

    return(0);
}

#endif /* UNUSED_KEEP_ALIVE */

_Check_return_
extern STATUS
image_cache_error_query(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle)
{
    const P_IMAGE_CACHE p_image_cache = image_cache_goto_item(image_cache_handle);
    STATUS err = 0;

    if(NULL != p_image_cache)
        err = p_image_cache->error;

    return(err);
}

_Check_return_
extern STATUS
image_cache_error_set(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle,
    _InVal_     STATUS err)
{
    const P_IMAGE_CACHE p_image_cache = image_cache_goto_item(image_cache_handle);

    if(NULL != p_image_cache)
    {
        p_image_cache->error = err;

        if(err)
            image_cache_entry_data_remove(p_image_cache);
    }

    return(err);
}

/******************************************************************************
*
* ensure that the cache has loaded data for this handle
*
******************************************************************************/

extern ARRAY_HANDLE
image_cache_loaded_ensure(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle)
{
    const P_IMAGE_CACHE p_image_cache = image_cache_goto_item(image_cache_handle);

    if(NULL != p_image_cache)
    {
        if(0 == p_image_cache->h_data_draw)
        {
            /* let caller query any errors from this load */
            if(image_cache_load(p_image_cache) <= 0)
                return(0);
        }

        return(p_image_cache->h_data_draw);
    }

    return(0);
}

_Check_return_
extern BOOL
image_cache_name_query(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer)
{
    const P_IMAGE_CACHE p_image_cache = image_cache_goto_item(image_cache_handle);

    if(NULL != p_image_cache)
    {
        tstr_xstrkpy(tstr_buf, elemof_buffer, (PCTSTR) (p_image_cache + 1));
        return(TRUE);
    }

    tstr_buf[0] = CH_NULL;
    return(FALSE);
}

/******************************************************************************
*
* add / remove ref for Draw file
*
******************************************************************************/

extern void
image_cache_ref(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle,
    _InVal_     BOOL add_ref)
{
    P_IMAGE_CACHE p_image_cache;

    trace_2(TRACE_MODULE_GR_CHART, TEXT("image_cache_ref(") ENUM_XTFMT TEXT(", %s"), image_cache_handle, report_boolstring(add_ref));

    if(IMAGE_CACHE_HANDLE_NONE == image_cache_handle)
        return;

    if(NULL == (p_image_cache = image_cache_goto_item(image_cache_handle)))
    {
        assert0();
        return;
    }

    if(add_ref)
    {
        ++p_image_cache->refs;
        trace_2(TRACE_MODULE_GR_CHART, TEXT("image_cache_ref: ") ENUM_XTFMT TEXT(" refs up to ") S32_TFMT, image_cache_handle, p_image_cache->refs);
    }
    else if(0 != p_image_cache->refs)
    {
        --p_image_cache->refs;
        trace_2(TRACE_MODULE_GR_CHART, TEXT("image_cache_ref: ") ENUM_XTFMT TEXT(" refs down to ") S32_TFMT, image_cache_handle, p_image_cache->refs);

        if(0 == p_image_cache->refs)
        {
            if(!p_image_cache->no_autokill)
            {
                trace_2(TRACE_MODULE_GR_CHART, TEXT("image_cache_ref: ") ENUM_XTFMT TEXT(" refs down to 0, so free diagram ") S32_TFMT TEXT(" (removing the entry)"), image_cache_handle, p_image_cache->h_data_draw);
                image_cache_entry_remove(image_cache_handle);
            }
            else
            {
                trace_2(TRACE_MODULE_GR_CHART, TEXT("image_cache_ref: ") ENUM_XTFMT TEXT(" refs down to 0, so free diagram ") S32_TFMT TEXT(" (leave the entry around)"), image_cache_handle, p_image_cache->h_data_draw);
                image_cache_entry_data_remove(p_image_cache);
            }
        }
    }
    else
        assert0(); /* trying to decrement refs past zero */
}

extern void
image_cache_reref(
    _InoutRef_  P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _InVal_     IMAGE_CACHE_HANDLE new_cah)
{
    IMAGE_CACHE_HANDLE old_cah = *p_image_cache_handle;

    /* changing reference? */
    if(old_cah != new_cah)
    {
        /* remove a prior ref if there was one */
        if(old_cah != IMAGE_CACHE_HANDLE_NONE)
            image_cache_ref(old_cah, 0);

        /* add new ref if there is one */
        if(new_cah != IMAGE_CACHE_HANDLE_NONE)
            image_cache_ref(new_cah, 1);

        /* poke the picture ref */
        *p_image_cache_handle = new_cah;
    }
}

/******************************************************************************
*
* search Draw file cache for a file using a key
*
******************************************************************************/

_Check_return_
extern ARRAY_HANDLE
image_cache_search(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle,
    _InVal_     BOOL fOriginal)
{
    const P_IMAGE_CACHE p_image_cache = image_cache_goto_item(image_cache_handle);
    ARRAY_HANDLE h_data = 0;

    trace_2(TRACE_MODULE_GR_CHART, TEXT("image_cache_search(") ENUM_XTFMT TEXT(", %s)"), image_cache_handle, report_boolstring(fOriginal));

    if(NULL != p_image_cache)
        h_data = fOriginal ? p_image_cache->h_data_original : p_image_cache->h_data_draw;

    trace_1(TRACE_MODULE_GR_CHART, TEXT("image_cache_search yields %d"), h_data);
    return(h_data);
}

#if WINDOWS

_Check_return_
_Ret_maybenull_
extern GdipImage
image_cache_search_gdip_image(
    _InVal_     IMAGE_CACHE_HANDLE image_cache_handle,
    _OutRef_    P_ARRAY_HANDLE p_array_handle)
{
    const P_IMAGE_CACHE p_image_cache = image_cache_goto_item(image_cache_handle);
    ARRAY_HANDLE h_data = 0;
    GdipImage gdip_image = NULL;

    if(NULL != p_image_cache)
    {
        h_data = p_image_cache->h_data_draw;
        gdip_image = p_image_cache->gdip_image;
    }

    trace_2(TRACE_MODULE_GR_CHART, TEXT("image_cache_search_gdip_image(") ENUM_XTFMT TEXT(") yields ") PTR_XTFMT, image_cache_handle, gdip_image);
    *p_array_handle = h_data;
    return(gdip_image);
}

#endif /* OS */

extern void
image_cache_trash(void)
{
    IMAGE_CACHE_HANDLE image_cache_handle;
    P_IMAGE_CACHE p_image_cache;

    for(p_image_cache = image_cache_first_item(&image_cache_handle);
        p_image_cache;
        p_image_cache = image_cache_next_item(&image_cache_handle))
    {
        image_cache_entry_data_remove(p_image_cache);
    }

    collect_delete(&image_cache);
}

/******************************************************************************
*
* ensure Draw file for given entry is loaded
*
******************************************************************************/

static void
image_cache_load_setup_foreign_diagram(
    _InRef_     PC_ARRAY_HANDLE p_h_data_draw,
    _In_z_      PC_SBSTR szCreatorName)
{
    __pragma(warning(disable:6260)) /* sizeof * sizeof is usually wrong. Did you intend to use a character count or a byte count? */
    gr_riscdiag_diagram_init((P_DRAW_FILE_HEADER) array_range(p_h_data_draw, BYTE, 0, sizeof32(DRAW_FILE_HEADER)), szCreatorName);

    {
    DRAW_OBJECT_OPTIONS options;

    zero_struct(options); /* NB bounding box of options object must be ignored */

    options.type = DRAW_OBJECT_TYPE_OPTIONS;
    options.size = sizeof32(options);

    options.paper_size = (4 + 1) << 8; /* A4 */
    options.paper_limits.defaults = 1;

    writeval_F64_as_ARM(&options.grid_spacing[0], 1.0);
    options.grid_division = 2;
    options.grid_units = 1; /* cm */

    options.zoom_multiplier = 1;
    options.zoom_divider = 1;

    options.toolbox_presence = 1;

    options.initial_entry_mode.select = 1;

    options.undo_buffer_bytes = 5000;

    memcpy32(array_range(p_h_data_draw, BYTE, sizeof32(DRAW_FILE_HEADER), sizeof32(DRAW_OBJECT_OPTIONS)), &options, sizeof32(DRAW_OBJECT_OPTIONS));
    } /*block*/
}

_Check_return_
static STATUS
image_cache_process_loaded_or_embedded_data(
    P_IMAGE_CACHE p_image_cache,
    _In_        PC_ARRAY_HANDLE p_h_data /*may mutate*/,
    _In_        U32 n_bytes /*may mutate*/,
    _In_        T5_FILETYPE t5_filetype /*may mutate*/)
{
    STATUS status = STATUS_DONE;
    P_BYTE readp = NULL;
    BOOL fCreateDrawfile = TRUE;
#if WINDOWS
    BOOL fCreateGdipImage = FALSE;
    ARRAY_HANDLE h_converted_for_wrap = 0; /* if non-zero, function parameters mutate */
#endif /* OS */
    U32 drawlength = n_bytes;
    U32 spritelength = 0; /* keep dataflower happy */
    U32 object_offset = 0;
    U32 object_data_offset = 0;

#if WINDOWS
    switch(t5_filetype)
    {
    default:
        break;

    case FILETYPE_BMP:
        /* Needed even on Windows as we may be caching these for inclusion in a chart Draw file! */
        fCreateDrawfile = TRUE;
        fCreateGdipImage = TRUE;
        break;

    case FILETYPE_ICO:
    case FILETYPE_GIF:
    case FILETYPE_WMF:
    case FILETYPE_PNG:
    case FILETYPE_JPEG: /* Yep, fails to render on Windows */
    case FILETYPE_TIFF:
    case FILETYPE_WINDOWS_EMF:
        /* No direct means of wrapping these types in a Draw file - may be rendered with GDI+ */
        fCreateDrawfile = FALSE;
        fCreateGdipImage = TRUE;
        break;
    }
#endif /* OS */

    for(;;) /* loop for structure */
    {
#if WINDOWS
        if(fCreateGdipImage)
        {
            status = GdipImage_New(&p_image_cache->gdip_image);

            if(status_ok(status))
            {   /* NB This takes its own copy of the data for GDI+ */
                BOOL ok;

                assert(NULL != p_image_cache->gdip_image);

                ok = GdipImage_Load_Memory(p_image_cache->gdip_image, array_range(p_h_data, BYTE, 0, n_bytes), n_bytes, t5_filetype);

                if(ok)
                {
                    switch(t5_filetype)
                    {
                    default:
                        /* No direct means of wrapping these types in a Draw file - convert a copy to BMP for wrapping */
                        ok = image_cache_convert_data_for_wrap(p_image_cache, &h_converted_for_wrap, &t5_filetype);

                        if(ok)
                        {
                            p_h_data = &h_converted_for_wrap;
                            n_bytes = array_elements32(p_h_data);
                            fCreateDrawfile = TRUE;
                        }
                        break;

                    case FILETYPE_BMP:
                        break;
                    }
                }
            }
        }
#endif /* OS */

        if(fCreateDrawfile)
        {   /* create a Draw file in memory from this data */
            static /*poked*/ ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(BYTE), FALSE);
#if WINDOWS
            /* use appropriate allocator - WILL be needed for Dial Solutions Draw file manipulation routines */
            array_init_block.use_alloc = ALLOC_USE_DS_ALLOC;
#endif

            switch(t5_filetype)
            {
            default: default_unhandled();
#if CHECKING
                drawlength = n_bytes;
                break;

            case FILETYPE_DRAW:
            case FILETYPE_VECTOR:
            case FILETYPE_POSTER:
#endif
                /* round up size to be paranoid */
                drawlength = round_up(n_bytes, 4);
                assert(drawlength == n_bytes); /* should already be a multiple of 4 */ /* NB PipeDream chart files are not padded */
                break;

            case FILETYPE_BMP:
                /* BMP files have a BITMAPFILEHEADER on the front */
                spritelength = n_bytes - sizeof_BITMAPFILEHEADER;

                /* round up size for packaging in a Draw object */
                spritelength = round_up(spritelength, 4);

                object_offset = sizeof32(DRAW_FILE_HEADER) + sizeof32(DRAW_OBJECT_OPTIONS);
                object_data_offset = object_offset + sizeof32(DRAW_OBJECT_HEADER);

                drawlength = object_data_offset + spritelength;
                break;

            case FILETYPE_JPEG:
                /* round up size for packaging in a Draw object */
                spritelength = round_up(n_bytes, 4);

                object_offset = sizeof32(DRAW_FILE_HEADER) + sizeof32(DRAW_OBJECT_OPTIONS);
                object_data_offset = object_offset + sizeof32(DRAW_OBJECT_JPEG);

                drawlength = object_data_offset + spritelength;
                break;

            case FILETYPE_SPRITE:
                /* sprite files have a SPRITE_FILE_HEADER on the front (sprite area without the length word) */
                spritelength = n_bytes - sizeof_SPRITE_FILE_HEADER;

                /* round up size for packaging in a Draw object */
                spritelength = round_up(spritelength, 4);

                object_offset = sizeof32(DRAW_FILE_HEADER) + sizeof32(DRAW_OBJECT_OPTIONS);
                object_data_offset = object_offset + sizeof32(DRAW_OBJECT_HEADER);

                drawlength = object_data_offset + spritelength;
                break;
            }

            reportf(TEXT("image_cache_load: file length: ") U32_TFMT TEXT(", Draw file length: ") S32_TFMT, n_bytes, drawlength);

            if(NULL == (readp = al_array_alloc_BYTE(&p_image_cache->h_data_draw, drawlength, &array_init_block, &p_image_cache->error)))
                break; /* out of loop */

            /* Adjust the position where data is to be loaded */
            readp += object_data_offset;

            switch(t5_filetype)
            {
            default: default_unhandled();
#if CHECKING
            case FILETYPE_DRAW:
            case FILETYPE_VECTOR:
            case FILETYPE_POSTER:
            case FILETYPE_JPEG:
#endif
                break;

            case FILETYPE_BMP:
                /* need to strip file header too */
                assert(  sizeof_BITMAPFILEHEADER <= object_data_offset);
                readp -= sizeof_BITMAPFILEHEADER;
                break;

            case FILETYPE_SPRITE:
                /* need to strip file header too */
                assert(  sizeof_SPRITE_FILE_HEADER <= object_data_offset);
                readp -= sizeof_SPRITE_FILE_HEADER;
                break;
            }

            /* copy all of the original (or converted) data to this position */
            memcpy32(readp, array_range(p_h_data, BYTE, 0, n_bytes), n_bytes);

            /* now got the data loaded in p_image_cache->h_data_draw at the desired offset */

            switch(t5_filetype)
            {
            default:
                break;

            case FILETYPE_DRAW:
            case FILETYPE_VECTOR:
            case FILETYPE_POSTER:
#if RISCOS
                image_cache_spot_draw_bmp_object(&p_image_cache->h_data_draw); /* convert any OBJDIBs to SPRITEs on RISC OS */
#elif WINDOWS
                assert(NULL == p_image_cache->gdip_image);
                image_cache_spot_draw_jpeg_object(p_image_cache); /* may create GdipImage */
                image_cache_add_win_font_table(&p_image_cache->h_data_draw);
#endif /* OS */
                break;
            }

#if RISCOS 
            switch(t5_filetype)
            {
            default:
                break;

            case FILETYPE_BMP:
                {
                /* Use the PicConvert module to convert it to a sprite
                   R0 = address of BMPFILEHEADER
                   R1 = 0 for enquiry call, otherwise address for output sprite
                   R2 = options for use with 24bit BMPs: use 0 for lowest o/p
                */
                STATUS status;
                _kernel_swi_regs rs;
                int options = 3; /* options => 24 bpp BMP -> 24 bpp sprite */
                S32 newlength;

                if(host_os_version_query() < RISCOS_3_5)
                    options = 0; /* 24 bpp BMP -> 8 bpp,default palette sprite */

                /* Do Enq call */
                rs.r[0] = (int) readp;
                rs.r[1] = 0;
                rs.r[2] = options;

                status_break(image_convert_ensure_PicConvert());

                if(_kernel_swi(PicConvert_BMPtoFF9 /*0x48E00*/, &rs, &rs))
                {   /* enquiry failed - bind as BMP object into Drawfile regardless */
                    break;
                }

                newlength = rs.r[1];

                if(0 == newlength)
                {   /* no conversion - bind as BMP object into Drawfile regardless */
                    break;
                }

                {
                ARRAY_HANDLE new_h_data_draw;
                P_BYTE readp_new;

                spritelength = newlength - sizeof_SPRITE_FILE_HEADER;

                newlength = object_data_offset + spritelength;

                if(NULL == (readp_new = al_array_alloc_BYTE(&new_h_data_draw, newlength, &array_init_block_byte, &status)))
                    /* alloc failed - bind as BMP object into Draw file regardless */
                    break;

                readp_new += object_data_offset - sizeof_SPRITE_FILE_HEADER;

                /* Do Conversion call */
                rs.r[0] = (int) readp;
                rs.r[1] = (int) readp_new;
                rs.r[2] = options;

                if(NULL != _kernel_swi(PicConvert_BMPtoFF9 /*0x48E00*/, &rs, &rs))
                {   /* conversion failed, discard new memory - bind as BMP object into Draw file regardless */
                    al_array_dispose(&new_h_data_draw);
                    break;
                }

                /* throw away the previous copy and use the new one */
                al_array_dispose(&p_image_cache->h_data_draw);

                p_image_cache->h_data_draw = new_h_data_draw;

                drawlength = newlength;

                readp = readp_new;

                t5_filetype = FILETYPE_SPRITE;

                /* object_offset is unchanged */
                } /*block*/

                break;
                }
            }
#endif /* RISCOS */

            switch(t5_filetype)
            {
            default:
                break;

            case FILETYPE_BMP:
                {
                P_DRAW_OBJECT_HEADER pObject;
                S32 size;

                image_cache_load_setup_foreign_diagram(&p_image_cache->h_data_draw, "IMC_DIB");

                /* bitmap is the first and only real object in this diagram */
                pObject = (P_DRAW_OBJECT_HEADER) array_ptr(&p_image_cache->h_data_draw, BYTE, object_offset);

                pObject->type = DRAW_OBJECT_TYPE_DS_DIB; /* FILETYPE_BMP aka DIB */

                size  = sizeof32(*pObject);

                size += spritelength; /* actual size */

                pObject->size = size;

                /* force duff bbox so riscdiag_object_reset_bbox will calculate default size, not retain */
                pObject->bbox.x0 =  0; /* but bl positioned at 0,0 for the moment */
                pObject->bbox.x1 = -1;
                pObject->bbox.y0 =  0;
                pObject->bbox.y1 = -1;

                image_cache_rebind(&p_image_cache->h_data_draw);

                break;
                }

            case FILETYPE_JPEG:
                {
                U8 density_type = readp[13];
                U32 density_x = (U32) readval_U16_BE(&readp[14]);
                U32 density_y = (U32) readval_U16_BE(&readp[16]);
                P_DRAW_OBJECT_JPEG pJpegObject;
                S32 size;

                image_cache_load_setup_foreign_diagram(&p_image_cache->h_data_draw, "IMC_JFIF");

                /* JPEG image is the first and only real object in this diagram */
                pJpegObject = (P_DRAW_OBJECT_JPEG) array_ptr(&p_image_cache->h_data_draw, BYTE, object_offset);

                pJpegObject->type = DRAW_OBJECT_TYPE_JPEG;

                size  = sizeof32(*pJpegObject);

                size += spritelength; /* actual size */

                pJpegObject->size = size;

                /* force duff bbox so riscdiag_object_reset_bbox will calculate default size, not retain */
                pJpegObject->bbox.x0 =  0; /* but bl positioned at 0,0 for the moment */
                pJpegObject->bbox.x1 = -1;
                pJpegObject->bbox.y0 =  0;
                pJpegObject->bbox.y1 = -1;

                pJpegObject->width  = GR_RISCDRAW_PER_INCH;
                pJpegObject->height = GR_RISCDRAW_PER_INCH;

                switch(density_type)
                {
                default: default_unhandled();
                case 0: /* aspect ratio - convert to something 'reasonable' as pixels per inch */
                    if((density_x == 1) && (density_y == 1))
                        density_x = density_y = 90;

#if 0
                    while((density_x < 64) && (density_y < 64))
                    {
                        density_x *= 2;
                        density_y *= 2;
                    }
#endif

                    /*FALLTRHU*/

                case 1: /* pixels per inch */
                    pJpegObject->dpi_x = density_x;
                    pJpegObject->dpi_y = density_y;
                    break;

                case 2: /* pixels per cm */
                    pJpegObject->dpi_x = (S32) (density_x / 2.54);
                    pJpegObject->dpi_y = (S32) (density_y / 2.54);
                    break;
                }

                pJpegObject->trfm.a = GR_SCALE_ONE;
                pJpegObject->trfm.b = GR_SCALE_ZERO;
                pJpegObject->trfm.c = GR_SCALE_ZERO;
                pJpegObject->trfm.d = GR_SCALE_ONE;
                pJpegObject->trfm.e = GR_SCALE_ZERO;
                pJpegObject->trfm.f = GR_SCALE_ZERO;

                pJpegObject->len = (int) n_bytes;

#if WINDOWS
                assert(NULL != p_image_cache->gdip_image); /* should already have been created */
                if(NULL != p_image_cache->gdip_image)
                {
                    SIZE image_size;

                    GdipImage_GetImageSize(p_image_cache->gdip_image, &image_size); /* TWIPS */

                    /* fill in rest of JPEG header */
                    pJpegObject->width  = GR_RISCDRAW_PER_PIXIT * image_size.cx;
                    pJpegObject->height = GR_RISCDRAW_PER_PIXIT * image_size.cy;
                    reportf(TEXT("GdipImage_GetImageSize: w=%d, h=%d (twips), dpi x=%d,y=%d"), pJpegObject->width, pJpegObject->height, pJpegObject->dpi_x, pJpegObject->dpi_y);
                }
#endif /* OS */

#if RISCOS
                { /* fill in rest of JPEG header */
                _kernel_swi_regs rs;
                rs.r[0] = 1; /* return dimensions */
                rs.r[1] = (int) (pJpegObject+ 1); /* point to JPEG image */
                rs.r[2] = (int) n_bytes;
                if(NULL == _kernel_swi(/*JPEG_Info*/ 0x49980, &rs, &rs))
                {
                  /*if(rs.r[0] & (1<<2))
                        reportf(TEXT("JPEG_Info: simple ratio"));*/
                    pJpegObject->width  = rs.r[2]; /* pixels */
                    pJpegObject->height = rs.r[3];
                  /*pJpegObject->dpi_x  = rs.r[4];*/
                  /*pJpegObject->dpi_y  = rs.r[5];*/
                    reportf(TEXT("JPEG_Info: w=%d, h=%d (px), dpi x=%d,y=%d"), pJpegObject->width, pJpegObject->height, pJpegObject->dpi_x, pJpegObject->dpi_y);

                    pJpegObject->width  = (pJpegObject->width  * GR_RISCDRAW_PER_INCH) / pJpegObject->dpi_x; /* convert from pixels to Draw units */
                    pJpegObject->height = (pJpegObject->height * GR_RISCDRAW_PER_INCH) / pJpegObject->dpi_y;
                }
                } /*block*/

#endif /* OS */
                reportf(TEXT("JPEG: w=%d, h=%d Draw units"), pJpegObject->width, pJpegObject->height);

#if 0 /* Seems to barf Acorn DrawFile renderer */
                /* Ensure that the picture fits on an A4 portrait page */
                while( (pJpegObject->width  > DrawUnitsFromHiMetric(21000/*hm*/)) ||
                       (pJpegObject->height > DrawUnitsFromHiMetric(29700/*hm*/)) )
                {
                    pJpegObject->width  /= 2;
                    pJpegObject->height /= 2;
                    pJpegObject->dpi_x  *= 2;
                    pJpegObject->dpi_y  *= 2;
                    reportf(TEXT("JPEG: w=%d, h=%d Draw units"), pJpegObject->width, pJpegObject->height);
                }
#endif

                pJpegObject->bbox.x1 = pJpegObject->width;
                pJpegObject->bbox.y1 = pJpegObject->height;
                /*reportf(TEXT("JPEG: bbox %d,%d %d,%d"), pJpegObject->bbox.x0, pJpegObject->bbox.y0, pJpegObject->bbox.x1, pJpegObject->bbox.y1);*/

                image_cache_rebind(&p_image_cache->h_data_draw);

                /*reportf(TEXT("JPEG: bbox %d,%d %d,%d"), pJpegObject->bbox.x0, pJpegObject->bbox.y0, pJpegObject->bbox.x1, pJpegObject->bbox.y1);*/

                break;
                }

            case FILETYPE_SPRITE:
                {
                P_DRAW_OBJECT_SPRITE pSpriteObject;

                image_cache_load_setup_foreign_diagram(&p_image_cache->h_data_draw, "IMC_SPRITE");

                /* sprite is the first and only real object in this diagram */
                pSpriteObject = (P_DRAW_OBJECT_SPRITE) array_ptr(&p_image_cache->h_data_draw, BYTE, object_offset);

                pSpriteObject->type = DRAW_OBJECT_TYPE_SPRITE;
                pSpriteObject->size = sizeof32(DRAW_OBJECT_HEADER) + pSpriteObject->sprite.next; /* actual size */
                pSpriteObject->sprite.next = 0;

                /* force duff bbox so riscdiag_object_reset_bbox will calculate default size, not retain */
                pSpriteObject->bbox.x0 =  0; /* but bl positioned at 0,0 for the moment */
                pSpriteObject->bbox.x1 = -1;
                pSpriteObject->bbox.y0 =  0;
                pSpriteObject->bbox.y1 = -1;

                { /* chuck away trailing sprites and reduce diagram size */
                U32 required = object_offset + pSpriteObject->size;
                S32 excess   = array_elements(&p_image_cache->h_data_draw) - (S32) required;
                myassert2x(!excess, TEXT("about to strip ") S32_TFMT TEXT(" bytes after offset ") S32_TFMT TEXT(" in loaded sprite file"), excess, required);
                al_array_shrink_by(&p_image_cache->h_data_draw, -excess);
                } /*block*/

                image_cache_rebind(&p_image_cache->h_data_draw);

                break;
                }

            } /* end of switch */

#if 0
            {
            P_DRAW_FILE_HEADER p_draw_file_header = (P_DRAW_FILE_HEADER) array_base(&p_image_cache->h_data_draw, BYTE);
            reportf(TEXT("Draw: bbox %d,%d %d,%d"), p_draw_file_header->bbox.x0, p_draw_file_header->bbox.y0, p_draw_file_header->bbox.x1, p_draw_file_header->bbox.y1);
            } /*block*/
#endif
        } /*fi*/

        break; /* end of loop for structure */
        /*NOTREACHED*/
    }

#if WINDOWS
    al_array_dispose(&h_converted_for_wrap);
#endif

    return(status);
}

_Check_return_
static STATUS
image_cache_load(
    P_IMAGE_CACHE p_image_cache)
{
    STATUS status = STATUS_DONE;
    const PCTSTR original_name = (PCTSTR) (p_image_cache + 1);
    T5_FILETYPE t5_filetype_original, t5_filetype;
    U32 n_bytes = 0;
    PTSTR converted_name = NULL;
    ARRAY_HANDLE h_data_converted = 0;

    /* reset error each time we try to load */
    p_image_cache->error = 0;

    if(0 != p_image_cache->h_data_original)
    {   /* already loaded; a representation will already be converted as best as possible */
        return(STATUS_DONE);
    }

    { /* read the existing original file */
    T5_FILETYPE t5_filetype_loaded;

    if(status_fail(status = image_cache_load_file_to_handle(&p_image_cache->h_data_original, original_name, &t5_filetype_loaded)))
    {
        p_image_cache->error = status;
        return(status);
    }

    if(FILETYPE_UNDETERMINED != t5_filetype_loaded)
    {
        assert(p_image_cache->t5_filetype_original == t5_filetype_loaded);
        p_image_cache->t5_filetype_original = t5_filetype_loaded;
    }

    t5_filetype = t5_filetype_original = p_image_cache->t5_filetype_original;
    } /*block*/

    n_bytes = array_elements32(&p_image_cache->h_data_original);

    for(;;) /* loop for structure */
    {
        if(image_cache_can_load(t5_filetype_original))
            break;

        if(!image_convert_can_convert(t5_filetype_original))
        {
            trace_1(TRACE_MODULE_GR_CHART, TEXT("image_cache_load: cannot convert: %s"), original_name);
            status = STATUS_FAIL;
            break;
        }

        if(status_fail(status = image_convert_do_convert_file(&converted_name, &t5_filetype, original_name, t5_filetype_original)))
        {
            trace_1(TRACE_MODULE_GR_CHART, TEXT("image_cache_load: failed to convert: %s"), original_name);
            break;
        }

        {
        T5_FILETYPE t5_filetype_loaded;

        status_break(status = image_cache_load_file_to_handle(&h_data_converted, converted_name, &t5_filetype_loaded));

        if(FILETYPE_UNDETERMINED != t5_filetype_loaded)
        {
            assert(t5_filetype == t5_filetype_loaded);
            t5_filetype = t5_filetype_loaded;
        }
        } /*block*/

        n_bytes = array_elements32(&h_data_converted);

        break; /* end of loop for structure */
        /*NOTREACHED*/
    }

    /* iff converted, n_bytes and t5_filetype now pertain to h_converted_data; otherwise to h_data_original */
    if(status_ok(status))
        status = image_cache_process_loaded_or_embedded_data(p_image_cache, (0 != h_data_converted) ? &h_data_converted : &p_image_cache->h_data_original, n_bytes, t5_filetype);

    if(status_ok(status))
        status = STATUS_DONE;

    al_array_dispose(&h_data_converted);

    if(NULL != converted_name)
    {
        // <<< (void) _tremove(converted_name);
        tstr_clr(&converted_name);
    }

    return(status);
}

_Check_return_
static STATUS
image_cache_load_file_to_handle(
    _OutRef_    P_ARRAY_HANDLE p_h_data,
    _In_z_      PCTSTR filename,
    _OutRef_    P_T5_FILETYPE p_t5_filetype)
{
    U32 n_bytes = 0;
    FILE_HANDLE fin;
    STATUS status;

    *p_h_data = 0;
    *p_t5_filetype = FILETYPE_UNDETERMINED;

    if(status_fail(status = t5_file_open(filename, file_open_read, &fin, TRUE)))
    {
        reportf(TEXT("image_cache_load_file_to_handle: cannot open: %s"), filename);
        return(status);
    }

    for(;;) /* loop for structure */
    {
        P_BYTE p_data;
        filelength_t x_filelength;

        if(status_fail(status = file_length(fin, &x_filelength)))
            break;

        assert(      x_filelength.u.words.hi == 0);
        assert((S32) x_filelength.u.words.lo >= 0);
        n_bytes = (U32) x_filelength.u.words.lo;

        reportf(TEXT("image_cache_load_file_to_handle: %s file length: ") U32_TFMT, filename, n_bytes);

        {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(BYTE), FALSE);
        if(NULL == (p_data = al_array_alloc_BYTE(p_h_data, n_bytes, &array_init_block, &status)))
            break;
        } /*block*/

        /* load all of the file */
        if(status_fail(status = file_read_bytes_requested(p_data, n_bytes, fin)))
        {
            al_array_dispose(p_h_data);
            break;
        }

        /* what did we convert the file to? */
#if RISCOS
        *p_t5_filetype = (T5_FILETYPE) file_get_risc_os_filetype(fin);
#endif

        break; /* end of loop for structure */
        /*NOTREACHED*/
    }

    status_assert(t5_file_close(&fin));

    return(status);
}

#if WINDOWS

static void
image_cache_add_win_font_table(
    _InoutRef_  P_ARRAY_HANDLE p_h_data_draw /*inout, certainly poked*/)
{
    draw_diag d;
    draw_error de;
    zero_struct(de);
    d.data = array_base(p_h_data_draw, void);
    d.length = array_elements32(p_h_data_draw);
    PTR_ASSERT(d.data);
    assert(0 != d.length);
    consume_bool(Draw_VerifyDiag(NULL, &d, FALSE /*rebind*/, TRUE /*AddWinFontTable*/, TRUE /*UpdateFontMap*/, &de));
    al_array_resized_ptr(p_h_data_draw, d.data, d.length);
}

#endif /* OS */

static void
image_cache_rebind(
    _InoutRef_  P_ARRAY_HANDLE p_h_data_draw /*inout, certainly poked*/)
{
    trace_0(TRACE_MODULE_GR_CHART, TEXT("image_cache_rebind: file loaded, now verify"));

#if TRACE_ALLOWED
    { /* get box in Draw units - NB array of BYTE */
    PC_DRAW_FILE_HEADER pDrawFileHdr = (PC_DRAW_FILE_HEADER) array_rangec(p_h_data_draw, BYTE, 0, sizeof32(DRAW_FILE_HEADER));
    DRAW_BOX box = pDrawFileHdr->bbox;
    trace_5(TRACE_MODULE_GR_CHART, TEXT("image_cache_rebind: diagram bbox prior to verify ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(" (Draw), length ") S32_TFMT, box.x0, box.y0, box.x1, box.y1, array_elements(p_h_data_draw));
    } /*block*/
#endif

    { /* MUST do this even when we call Draw_VerifyDiag afterwards */
    GR_RISCDIAG gr_riscdiag;
    GR_RISCDIAG_PROCESS_T process;
    gr_riscdiag_diagram_setup_from_data(&gr_riscdiag, array_base(p_h_data_draw, BYTE), array_elements32(p_h_data_draw));
    * (int *) &process = 0;
    process.recurse    = 1;
    process.recompute  = 1;
    gr_riscdiag_diagram_reset_bbox(&gr_riscdiag, process);
    } /*block*/

#if TRACE_ALLOWED
    { /* get box in Draw units - NB array of BYTE */
    PC_DRAW_FILE_HEADER pDrawFileHdr = (PC_DRAW_FILE_HEADER) array_rangec(p_h_data_draw, BYTE, 0, sizeof32(DRAW_FILE_HEADER));
    DRAW_BOX box = pDrawFileHdr->bbox;
    trace_5(TRACE_MODULE_GR_CHART, TEXT("image_cache_rebind: diagram bbox after verify ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(" (Draw), length ") S32_TFMT, box.x0, box.y0, box.x1, box.y1, array_elements(p_h_data_draw));
    } /*block*/
#endif
}

#if RISCOS

/* Given a group object, add it to the list if the array */

static void
image_cache_activate_group(
    _InVal_     S32 offset,
    P_BYTE p_base,
    P_ARRAY_HANDLE p_array_handle)
{
    STATUS status;
    P_S32 p_new = al_array_extend_by(p_array_handle, S32, 1, PC_ARRAY_INIT_BLOCK_NONE, &status);
    UNREFERENCED_PARAMETER(p_base);
    PTR_ASSERT(p_new);
    *p_new = offset;
}

static void
image_cache_kill_groups(
    _InVal_     S32 offset,
    P_BYTE p_base,
    _InRef_     PC_ARRAY_HANDLE p_array_handle)
{
    /* kill any item in the list which ENDs earlier than the given offset */
    const ARRAY_INDEX n_elements = array_elements(p_array_handle);
    ARRAY_INDEX index;

    for(index = 0; index < n_elements; index++)
    {
        P_S32 p_item = array_ptr(p_array_handle, S32, index);

        if(*p_item != -1) /* Is the object alive? */
        {
            P_DRAW_OBJECT_HEADER pObject = PtrAddBytes(P_DRAW_OBJECT_HEADER, p_base, *p_item); /* got the object */

            if(((S32) pObject->size + *p_item) <= offset)
                /* this object is now dead */
                *p_item = -1;
        }
    }
}

/* alter the size of all active groups by delta */

static void
image_cache_patch_groups(
    _InVal_     S32 offset,
    P_BYTE p_base,
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InVal_     S32 delta)
{
    const ARRAY_INDEX n_elements = array_elements(p_array_handle);
    ARRAY_INDEX index;

    UNREFERENCED_PARAMETER_InVal_(offset);

    if(delta == 0)
        return;

    for(index = 0; index < n_elements; index++)
    {
        P_S32 p_item = array_ptr(p_array_handle, S32, index);

        if(*p_item != -1) /* Is the object alive? */
        {
            P_DRAW_OBJECT_HEADER pObject = PtrAddBytes(P_DRAW_OBJECT_HEADER, p_base, *p_item); /* got the object */

            pObject->size += delta;
        }
    }
}

/* We have a BITMAPINFO here NOT a bitmapfileheader - shame */

static S32
image_cache_convert_bmp_object(
    _InoutRef_  P_ARRAY_HANDLE p_h_data_draw,
    P_DRAW_OBJECT_HEADER pObject)
{
    S32 delta = 0;
    ARRAY_HANDLE newhandle = 0;
    DRAW_BOX copy_box = pObject->bbox; /* make a copy of the bbox 'cos we need to make a BITMAPFILEHEADER over it */
    P_BITMAPINFO p_bitmapinfo = (P_BITMAPINFO) (pObject + 1);
    P_BYTE p_bitmapfileheader = (P_BYTE) p_bitmapinfo - sizeof_BITMAPFILEHEADER; /* point back enough before BITMAPINFO */
    S32 size;
    S32 offset = sizeof_BITMAPFILEHEADER + sizeof32(BITMAPINFOHEADER);
    U32 bpp = (U32) p_bitmapinfo->bmiHeader.biBitCount * p_bitmapinfo->bmiHeader.biPlanes;

    size = pObject->size;
    size -= sizeof32(DRAW_OBJECT_HEADER);
    size += sizeof_BITMAPFILEHEADER;

    if(bpp <= 8)
        offset += (4 << bpp);

__pragma(warning(push)) __pragma(warning(disable:4548)) /* expression before comma has no effect; expected expression with side-effect */
    /* Make a BITMAPFILEHEADER */
    p_bitmapfileheader[0] = 'B';
    p_bitmapfileheader[1] = 'M';
    writeval_S32(p_bitmapfileheader +  2, size);
    writeval_S16(p_bitmapfileheader +  6, 0);
    writeval_S16(p_bitmapfileheader +  8, 0);
    writeval_S32(p_bitmapfileheader + 10, offset);
__pragma(warning(pop))

    for(;;) /* loop for structure - we must restore the copy of the bbox before exiting and get rid of any temp core allocated */
    {
        STATUS status;
        _kernel_swi_regs rs;
        int options = 3; /* 24 bpp BMP -> 24 bpp sprite */
        S32 newlength;
        P_BYTE readp_new;
        S32 required_size;
        S32 current_size;

        if(host_os_version_query() < RISCOS_3_5)
            options = 0; /* 24 bpp BMP -> 8 bpp,default palette sprite */

        /* Do Enq call */
        rs.r[0] = (int) p_bitmapfileheader;
        rs.r[1] = 0;
        rs.r[2] = options;

        status_break(image_convert_ensure_PicConvert());

        if(_kernel_swi(PicConvert_BMPtoFF9 /*0x48E00*/, &rs, &rs))
            break;

        if(0 == (newlength = rs.r[1]))
            break;

        if(NULL == (readp_new = al_array_alloc_BYTE(&newhandle, newlength, &array_init_block_byte, &status)))
            break;

        /* Do Conversion call */
        rs.r[0] = (int) p_bitmapfileheader;
        rs.r[1] = (int) readp_new;
        rs.r[2] = options;

        if(_kernel_swi(PicConvert_BMPtoFF9 /*0x48E00*/, &rs, &rs))
            break;

        required_size = newlength - sizeof32(SAH) + sizeof32(DRAW_OBJECT_HEADER);
        current_size  = pObject->size;
        delta         = required_size-current_size;

        if(delta) /* otherwise you got impossibly lucky! */
        {
            P_BYTE p_from = PtrAddBytes(P_BYTE, pObject, current_size);
            S32 move_size = (S32) array_elements32(p_h_data_draw) - (p_from - array_base(p_h_data_draw, BYTE));
            S32 offset_pObject = (P_BYTE) pObject - array_base(p_h_data_draw, BYTE);

            if(delta > 0)
            {
                /* Getting bigger so realloc then move */
                if(NULL == al_array_extend_by_BYTE(p_h_data_draw, delta, PC_ARRAY_INIT_BLOCK_NONE, &status))
                {
                    delta = 0;
                    break;
                }

                pObject = (P_DRAW_OBJECT_HEADER) array_ptr(p_h_data_draw, BYTE, offset_pObject);

                p_from = (P_BYTE) pObject + current_size;

                memmove32(p_from + delta, p_from, move_size);
            }
            else
            {
                /* Getting smaller so move then realloc */
                p_from = PtrAddBytes(P_BYTE, pObject, current_size);

                memmove32(p_from + delta, p_from, move_size);

                al_array_shrink_by(p_h_data_draw, delta);

                pObject = (P_DRAW_OBJECT_HEADER) array_ptr(p_h_data_draw, BYTE, offset_pObject);
            }

            /* Fixup the header */
            pObject->size = required_size;
        }

        { /* Copy new data into place */
        P_BYTE p_to = (P_BYTE) (pObject + 1); /* sprite */
        PC_BYTE p_from = array_basec(&newhandle, BYTE) + sizeof32(SAH);
        U32 n_bytes = (U32) newlength - sizeof32(SAH);
        memcpy32(p_to, p_from, n_bytes);
        } /*block*/

        pObject->type = DRAW_OBJECT_TYPE_SPRITE; /* Change the tag */
    }

    al_array_dispose(&newhandle);

    pObject->bbox = copy_box;

    return(delta);
}

#endif /* RISCOS */

#if RISCOS

static void
image_cache_spot_draw_bmp_object(
    _InoutRef_  P_ARRAY_HANDLE p_h_data_draw)
{
    GR_RISCDIAG gr_riscdiag;
    P_GR_RISCDIAG p_gr_riscdiag = &gr_riscdiag;
    P_DRAW_OBJECT_HEADER pObject;
    DRAW_DIAG_OFFSET sttObject = DRAW_DIAG_OFFSET_FIRST /* first object */;
    DRAW_DIAG_OFFSET endObject = DRAW_DIAG_OFFSET_LAST  /* last object */;
    ARRAY_HANDLE array_handle_groups;
    SC_ARRAY_INIT_BLOCK array_init_block_groups = aib_init(1, sizeof32(S32), FALSE);

    if(status_fail(al_array_alloc_zero(&array_handle_groups, &array_init_block_groups)))
        return;

    /* scan the diagram for BMP objects - no need to set hglobal */
    gr_riscdiag_diagram_setup_from_data(&gr_riscdiag, array_base(p_h_data_draw, BYTE), array_elements32(p_h_data_draw));

    if(gr_riscdiag_object_first(p_gr_riscdiag, &sttObject, &endObject, (P_P_BYTE) &pObject, TRUE))
    {
        do  {
            if(pObject->type == DRAW_OBJECT_TYPE_GROUP) /* This group goes active */
                image_cache_activate_group((P_BYTE) pObject - array_base(p_h_data_draw, BYTE), array_base(p_h_data_draw, BYTE), &array_handle_groups);

            /* remove any 'dead' groups */
            image_cache_kill_groups((P_BYTE) pObject - array_base(p_h_data_draw, BYTE), array_base(p_h_data_draw, BYTE), &array_handle_groups);

            /* convert all DIB objects in place (to sprite) */
            if(pObject->type == DRAW_OBJECT_TYPE_DS_DIB)
            {
                S32 delta = image_cache_convert_bmp_object(p_h_data_draw, pObject);

                if(delta)
                {
                    gr_riscdiag_diagram_setup_from_data(&gr_riscdiag, array_base(p_h_data_draw, BYTE), array_elements32(p_h_data_draw));

                    endObject = p_gr_riscdiag->draw_diag.length;

                    image_cache_patch_groups((P_BYTE) pObject - array_base(p_h_data_draw, BYTE), array_base(p_h_data_draw, BYTE), &array_handle_groups, delta);
                }
            }
        }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &sttObject, &endObject, (P_P_BYTE) &pObject, TRUE));
    }

    al_array_dispose(&array_handle_groups);
}

#endif /* OS */

#if WINDOWS

static void
image_cache_spot_draw_jpeg_object(
    _InoutRef_  P_IMAGE_CACHE p_image_cache)
{
    const P_ARRAY_HANDLE p_h_data_draw = &p_image_cache->h_data_draw;
    GR_RISCDIAG gr_riscdiag;
    P_GR_RISCDIAG p_gr_riscdiag = &gr_riscdiag;
    P_DRAW_OBJECT_HEADER pObject;
    DRAW_DIAG_OFFSET sttObject = DRAW_DIAG_OFFSET_FIRST /* first object */;
    DRAW_DIAG_OFFSET endObject = DRAW_DIAG_OFFSET_LAST  /* last object */;
    PC_BYTE jpeg_ptr = NULL;
    U32 jpeg_length = 0;
    U32 nHarmlessOptions = 0;
    U32 nOtherOptions = 0;

    /* scan the diagram for JPEG objects */
    gr_riscdiag_diagram_setup_from_data(&gr_riscdiag, array_base(p_h_data_draw, BYTE), array_elements32(p_h_data_draw));

    if(gr_riscdiag_object_first(p_gr_riscdiag, &sttObject, &endObject, (P_BYTE *) &pObject, TRUE))
    {
        /* see if the whole Draw file is composed of harmless options plus a single JPEG */
        do  {
            if(pObject->type == DRAW_OBJECT_TYPE_JPEG)
            {
                P_DRAW_OBJECT_JPEG pJpegObject = (P_DRAW_OBJECT_JPEG) pObject;
                jpeg_length = pJpegObject->len;
                jpeg_ptr = (PC_BYTE) (pJpegObject + 1);
                break;
            }

            switch(pObject->type)
            {
            default:
                nOtherOptions++;
                break;

            case DRAW_OBJECT_TYPE_GROUP:
                break;

            case DRAW_OBJECT_TYPE_OPTIONS:
            case DRAW_OBJECT_TYPE_DS_WINFONTLIST:
                nHarmlessOptions++;
                break;
            }

            if(nOtherOptions != 0)
                break;
        }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &sttObject, &endObject, (P_BYTE *) &pObject, TRUE));

        if((jpeg_length != 0) && (nOtherOptions == 0))
        {
            STATUS status;
            status = GdipImage_New(&p_image_cache->gdip_image);
            if(status_ok(status))
            {
                assert(NULL != p_image_cache->gdip_image);
                consume_bool(GdipImage_Load_Memory(p_image_cache->gdip_image, jpeg_ptr, jpeg_length, FILETYPE_JPEG));
            }
        }
    }
}

#endif /* OS */

/* end of im_cache.c */
