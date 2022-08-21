/* gr_cache.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that handles Draw file cache */

/* SKS 12-Sep-1991 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef          __gr_cache_h
#include "cmodules/gr_cache.h"
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
#endif

#if WINDOWS
#include "external/Dial_Solutions/drawfile.h"

#include "ob_skel/ho_cpicture.h"
#endif

/*
internal structure
*/

/*
entries in the Draw file cache
*/

typedef struct GR_CACHE_DRAWFILE_ENTRY
{
    UBF no_autokill : 1;
    UBF refs : sizeof(int)*8 - 1;

    T5_FILETYPE t5_filetype;

    STATUS error;

    ARRAY_HANDLE h_data;

#if WINDOWS
    CPicture cpicture;
#endif

    /* name follows here at (p_gr_cache + 1) */
}
GR_CACHE, * P_GR_CACHE;

_Check_return_
static inline LIST_ITEMNO
list_itemno_from_handle(
    _InVal_     GR_CACHE_HANDLE cah)
{
    return((LIST_ITEMNO) (uintptr_t) (cah));
}

/*
internal functions
*/

static void
gr_cache_add_win_font_table(
    /*inout*/ P_ARRAY_HANDLE p_h_data);

_Check_return_
_Ret_maybenull_
static P_GR_CACHE
gr_cache_entry_new(
    _In_z_      PCTSTR name,
    _InVal_     T5_FILETYPE t5_filetype,
    _OutRef_    P_GR_CACHE_HANDLE cahp,
    _OutRef_    P_STATUS p_status);

_Check_return_
static STATUS
gr_cache_load(
    P_GR_CACHE p_gr_cache);

static void
gr_cache_rebind(
    P_ARRAY_HANDLE p_h_data /*inout, certainly poked*/);

static void
gr_cache_spot_draw_bmp_object(
    P_ARRAY_HANDLE p_h_data);

static void
gr_cache_spot_draw_jpeg_object(
    P_GR_CACHE p_gr_cache);

/*
the cache descriptor list
*/

static LIST_BLOCK
gr_cache_drawfiles;

_Check_return_
_Ret_maybenull_
static inline P_GR_CACHE
gr_cache_goto_item(
    _InVal_     GR_CACHE_HANDLE cah)
{
    return(collect_goto_item(GR_CACHE, &gr_cache_drawfiles, list_itemno_from_handle(cah)));
}

/******************************************************************************
*
* query of RISC OS filetypes that might sensibly be loaded
*
******************************************************************************/

_Check_return_
extern STATUS
gr_cache_can_import(
    _InVal_     T5_FILETYPE t5_filetype)
{
    switch(t5_filetype)
    {
    case FILETYPE_DRAW:
    case FILETYPE_SPRITE:
    case FILETYPE_WINDOWS_BMP:
#if 1
    case FILETYPE_JPEG:
#endif
        return(1);

    default:
        return(0);
    }
}

/******************************************************************************
*
* --out--
* -ve = error
*   0 = same as an existing entry
*   1 = entry exists
*
******************************************************************************/

_Check_return_
extern STATUS
gr_cache_embedded(
    _OutRef_    P_GR_CACHE_HANDLE cahp,
    _InoutRef_  P_ARRAY_HANDLE p_h_data /*inout;maybe stolen, certainly poked*/)
{
    LIST_ITEMNO key;
    P_GR_CACHE p_gr_cache;
    STATUS status;

    /* always rebind/verify the Draw file prior to comparison */
    /*gr_cache_rebind(p_h_data);*/

    gr_cache_add_win_font_table(p_h_data); /* SKS 20131001 like loading */
    gr_cache_spot_draw_bmp_object(p_h_data); /* convert any OBJDIBs on RISC OS */
    /*gr_cache_spot_draw_jpeg_object(p_gr_cache);*/ /* see below */

    for(p_gr_cache = collect_first(GR_CACHE, &gr_cache_drawfiles, &key);
        p_gr_cache;
        p_gr_cache = collect_next(GR_CACHE, &gr_cache_drawfiles, &key))
    {
        const U32 n_bytes = array_elements32(p_h_data);

        if(n_bytes != array_elements32(&p_gr_cache->h_data))
            continue;

        if(0 == memcmp32(array_rangec(p_h_data, BYTE, 0, n_bytes),
                         array_rangec(&p_gr_cache->h_data, BYTE, 0, n_bytes),
                         n_bytes))
        {
            /* bingo! a direct hit. tell the caller which one his data matched */
            *cahp = (GR_CACHE_HANDLE) key;
            return(STATUS_OK);
        }
    }

    if(NULL == (p_gr_cache = gr_cache_entry_new(tstr_empty_string, FILETYPE_DRAW, cahp, &status)))
        return(status);

    /* steal the transmogrified data */
    p_gr_cache->h_data = *p_h_data;
    *p_h_data = 0;

    gr_cache_spot_draw_jpeg_object(p_gr_cache); /* see if it is just an OBJJPEG */

    return(STATUS_DONE);
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
gr_cache_embedded_updating_entry(
    _InoutRef_  P_GR_CACHE_HANDLE cahp)
{
    P_GR_CACHE this_dcep = gr_cache_goto_item(*cahp);
    LIST_ITEMNO key;
    P_GR_CACHE p_gr_cache;

    PTR_ASSERT(this_dcep);

    for(p_gr_cache = collect_first(GR_CACHE, &gr_cache_drawfiles, &key);
        p_gr_cache;
        p_gr_cache = collect_next(GR_CACHE, &gr_cache_drawfiles, &key))
    {
        const U32 n_bytes = array_elements32(&this_dcep->h_data);

        if(this_dcep == p_gr_cache)
            continue;

        {
        PCTSTR entryname = (PCTSTR) (p_gr_cache + 1);

        if(*entryname)
            continue;
        } /*block*/

        if(n_bytes != array_elements32(&p_gr_cache->h_data))
            continue;

        if(0 == memcmp32(array_rangec(&this_dcep->h_data, BYTE, 0, n_bytes),
                         array_rangec(&p_gr_cache->h_data, BYTE, 0, n_bytes),
                         n_bytes))
        {
            /* bingo! a direct hit. blow the caller's entry away, replacing with what we found */
            gr_cache_entry_remove(*cahp);

            *cahp = (GR_CACHE_HANDLE) key;

            return(STATUS_OK);
        }
    }

    status_return(gr_cache_entry_rename(*cahp, NULL));

    return(STATUS_DONE);
}

/******************************************************************************
*
* given the details of a draw file, make
* sure an entry for it is in the cache
*
* --out--
* -ve = error
*   1 = entry exists
*
******************************************************************************/

_Check_return_
extern STATUS
gr_cache_entry_ensure(
    _OutRef_    P_GR_CACHE_HANDLE ecahp,
    _In_z_      PCTSTR name,
    _InVal_     T5_FILETYPE t5_filetype)
{
    GR_CACHE_HANDLE cah;
    P_GR_CACHE_HANDLE cahp = ecahp ? ecahp : &cah; /* export if viable */
    STATUS status;

    if(gr_cache_entry_query(cahp, name))
        return(STATUS_DONE);

    if(NULL != gr_cache_entry_new(name, t5_filetype, cahp, &status))
        return(status);

    return(STATUS_DONE);
}

/******************************************************************************
*
* create a list entry
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_GR_CACHE
gr_cache_entry_new(
    _In_z_      PCTSTR name,
    _InVal_     T5_FILETYPE t5_filetype,
    _OutRef_    P_GR_CACHE_HANDLE cahp,
    _OutRef_    P_STATUS p_status)
{
    static LIST_ITEMNO cahkey_gen = 0x42516000; /* NB. not tbs! */

    LIST_ITEMNO key = cahkey_gen++;
    U32 namlenp1 = tstrlen32p1(name);
    P_GR_CACHE p_gr_cache;

    *cahp = GR_CACHE_HANDLE_NONE;

    if(NULL != (p_gr_cache = collect_add_entry_bytes(GR_CACHE, &gr_cache_drawfiles, P_DATA_NONE, sizeof32(*p_gr_cache) + (sizeof32(*name) * namlenp1), key, p_status)))
    {
        zero_struct_ptr(p_gr_cache);

        p_gr_cache->t5_filetype = t5_filetype;

        memcpy32((p_gr_cache + 1), name, (sizeof32(*name) * namlenp1));

        *cahp = (GR_CACHE_HANDLE) key;
    }

    return(p_gr_cache);
}

/******************************************************************************
*
* given the details of a draw file, query whether an entry for it is in the cache
*
******************************************************************************/

_Check_return_
extern STATUS
gr_cache_entry_query(
    _OutRef_    P_GR_CACHE_HANDLE ecahp,
    _In_opt_z_  PCTSTR name)
{
    GR_CACHE_HANDLE cah;
    P_GR_CACHE_HANDLE cahp = ecahp ? ecahp : &cah; /* export if viable */

    /* search for the file on the list */
    *cahp = GR_CACHE_HANDLE_NONE;

    if(name)
    {
        BOOL rooted = file_is_rooted(name);
        PCTSTR leaf = file_leafname(name);
        LIST_ITEMNO key;
        P_GR_CACHE p_gr_cache;

        for(p_gr_cache = collect_first(GR_CACHE, &gr_cache_drawfiles, &key);
            p_gr_cache;
            p_gr_cache = collect_next(GR_CACHE, &gr_cache_drawfiles, &key))
        {
            PCTSTR testname  = name;
            PCTSTR entryname = (PCTSTR) (p_gr_cache + 1);

            if(!rooted)
            {
                testname  = leaf;
                entryname = file_leafname(entryname);
            }

            if(0 == tstricmp(testname, entryname))
            {
                *cahp = (GR_CACHE_HANDLE) key;

                trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_cache_entry_query found file, keys: ") ENUM_XTFMT TEXT(" ") S32_TFMT TEXT(", in list"), *cahp, key);

                trace_0(TRACE_MODULE_GR_CHART, TEXT("gr_cache_entry_query yields 1"));
                return(1);
            }
        }
    }

    trace_0(TRACE_MODULE_GR_CHART, TEXT("gr_cache_entry_query yields 0"));
    return(0);
}

static void
gr_cache_entry_data_remove(
    P_GR_CACHE p_gr_cache)
{
    al_array_dispose(&p_gr_cache->h_data);

#if WINDOWS
    if(NULL != p_gr_cache->cpicture)
        CPicture_Dispose(&p_gr_cache->cpicture);
#endif
}

extern void
gr_cache_entry_remove(
    _InVal_     GR_CACHE_HANDLE cah)
{
    LIST_ITEMNO key = list_itemno_from_handle(cah);
    P_GR_CACHE p_gr_cache;

    if(NULL == (p_gr_cache = gr_cache_goto_item(cah)))
    {
        assert0();
        return;
    }

    gr_cache_entry_data_remove(p_gr_cache);

    collect_subtract_entry(&gr_cache_drawfiles, key);
    collect_compress(&gr_cache_drawfiles);
}

_Check_return_
extern STATUS
gr_cache_entry_rename(
    _InVal_     GR_CACHE_HANDLE cah,
    _In_opt_z_  PCTSTR name)
{
    LIST_ITEMNO key = list_itemno_from_handle(cah);
    P_GR_CACHE p_gr_cache;

    trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_cache_entry_rename(") ENUM_XTFMT TEXT(", %s)"), cah, report_tstr(name));

    if(NULL != (p_gr_cache = gr_cache_goto_item(cah)))
    {
        /* if room to fit new name simply reuse else reallocate */
        if(NULL != name)
        {
            U32 namlenp1 = tstrlen32p1(name);

            if(tstrlen32p1((PCTSTR) (p_gr_cache + 1)) < namlenp1)
            {
                STATUS status;

                collect_subtract_entry(&gr_cache_drawfiles, key); /* SKS 24feb2012 was missing */

                if(NULL == (p_gr_cache = collect_add_entry_bytes(GR_CACHE, &gr_cache_drawfiles, P_DATA_NONE, sizeof32(GR_CACHE) + (sizeof32(*name) * namlenp1), key, &status)))
                    return(status);
            }

            memcpy32((p_gr_cache + 1), name, (sizeof32(*name) * namlenp1));
        }
        else
        {
            * (PTSTR) (p_gr_cache + 1) = CH_NULL;
        }

        return(1);
    }

    return(0);
}

#if defined(UNUSED_KEEP_ALIVE)

/******************************************************************************
*
* set state such that entry is removed when refs go to zero
*
******************************************************************************/

_Check_return_
extern STATUS
gr_cache_entry_clear_autokill(
    _InVal_     GR_CACHE_HANDLE cah)
{
    const P_GR_CACHE p_gr_cache = gr_cache_goto_item(cah);

    if(NULL != p_gr_cache)
    {
        p_gr_cache->no_autokill = 1;

        return(1);
    }

    return(0);
}

#endif /* UNUSED_KEEP_ALIVE */

_Check_return_
extern STATUS
gr_cache_error_query(
    _InVal_     GR_CACHE_HANDLE cah)
{
    const P_GR_CACHE p_gr_cache = gr_cache_goto_item(cah);
    STATUS err = 0;

    if(NULL != p_gr_cache)
        err = p_gr_cache->error;

    return(err);
}

_Check_return_
extern STATUS
gr_cache_error_set(
    _InVal_     GR_CACHE_HANDLE cah,
    _InVal_     STATUS err)
{
    const P_GR_CACHE p_gr_cache = gr_cache_goto_item(cah);

    if(NULL != p_gr_cache)
    {
        p_gr_cache->error = err;

        if(err)
            gr_cache_entry_data_remove(p_gr_cache);
    }

    return(err);
}

/******************************************************************************
*
* ensure that the cache has loaded data for this handle
*
******************************************************************************/

extern ARRAY_HANDLE
gr_cache_loaded_ensure(
    _InVal_     GR_CACHE_HANDLE cah)
{
    const P_GR_CACHE p_gr_cache = gr_cache_goto_item(cah);

    if(NULL != p_gr_cache)
    {
        if(!p_gr_cache->h_data)
        {
            /* let caller query any errors from this load */
            if(gr_cache_load(p_gr_cache) <= 0)
                return(0);
        }

        return(p_gr_cache->h_data);
    }

    return(0);
}

_Check_return_
extern BOOL
gr_cache_name_query(
    _InVal_     GR_CACHE_HANDLE cah,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer)
{
    const P_GR_CACHE p_gr_cache = gr_cache_goto_item(cah);

    if(NULL != p_gr_cache)
    {
        tstr_xstrkpy(tstr_buf, elemof_buffer, (PCTSTR) (p_gr_cache + 1));
        return(TRUE);
    }

    tstr_buf[0] = CH_NULL;
    return(FALSE);
}

/******************************************************************************
*
* add / remove ref for draw file
*
******************************************************************************/

extern void
gr_cache_ref(
    _InVal_     GR_CACHE_HANDLE cah,
    _InVal_     BOOL add_ref)
{
    P_GR_CACHE p_gr_cache;

    trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_cache_ref(") ENUM_XTFMT TEXT(", %s"), cah, report_boolstring(add_ref));

    if(GR_CACHE_HANDLE_NONE == cah)
        return;

    if(NULL == (p_gr_cache = gr_cache_goto_item(cah)))
    {
        assert0();
        return;
    }

    if(add_ref)
    {
        ++p_gr_cache->refs;
        trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_cache_ref: ") ENUM_XTFMT TEXT(" refs up to ") S32_TFMT, cah, p_gr_cache->refs);
    }
    else if(0 != p_gr_cache->refs)
    {
        --p_gr_cache->refs;
        trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_cache_ref: ") ENUM_XTFMT TEXT(" refs down to ") S32_TFMT, cah, p_gr_cache->refs);

        if(0 == p_gr_cache->refs)
        {
            if(!p_gr_cache->no_autokill)
            {
                trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_cache_ref: ") ENUM_XTFMT TEXT(" refs down to 0, so free diagram ") S32_TFMT TEXT(" (removing the entry)"), cah, p_gr_cache->h_data);
                gr_cache_entry_remove(cah);
            }
            else
            {
                trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_cache_ref: ") ENUM_XTFMT TEXT(" refs down to 0, so free diagram ") S32_TFMT TEXT(" (leave the entry around)"), cah, p_gr_cache->h_data);
                gr_cache_entry_data_remove(p_gr_cache);
            }
        }
    }
    else
        assert0(); /* trying to decrement refs past zero */
}

extern void
gr_cache_reref(
    _InoutRef_  P_GR_CACHE_HANDLE cahp,
    _InVal_     GR_CACHE_HANDLE new_cah)
{
    GR_CACHE_HANDLE old_cah = *cahp;

    /* changing reference? */
    if(old_cah != new_cah)
    {
        /* remove a prior ref if there was one */
        if(old_cah != GR_CACHE_HANDLE_NONE)
            gr_cache_ref(old_cah, 0);

        /* add new ref if there is one */
        if(new_cah != GR_CACHE_HANDLE_NONE)
            gr_cache_ref(new_cah, 1);

        /* poke the picture ref */
        *cahp = new_cah;
    }
}

/******************************************************************************
*
* search draw file cache for a file using a key
*
******************************************************************************/

extern ARRAY_HANDLE
gr_cache_search(
    _InVal_     GR_CACHE_HANDLE cah)
{
    const P_GR_CACHE p_gr_cache = gr_cache_goto_item(cah);
    ARRAY_HANDLE h_data = 0;

    trace_1(TRACE_MODULE_GR_CHART, TEXT("gr_cache_search(") ENUM_XTFMT TEXT(")"), cah);

    if(NULL != p_gr_cache)
        h_data = p_gr_cache->h_data;

    trace_1(TRACE_MODULE_GR_CHART, TEXT("gr_cache_search yields %d"), h_data);
    return(h_data);
}

#if WINDOWS

_Check_return_
_Ret_maybenull_
extern P_ANY /*CPicture*/
gr_cache_search_cpicture(
    _InVal_     GR_CACHE_HANDLE cah,
    _OutRef_    P_ARRAY_HANDLE p_array_handle)
{
    const P_GR_CACHE p_gr_cache = gr_cache_goto_item(cah);
    ARRAY_HANDLE h_data = 0;
    CPicture cpicture = NULL;

    if(NULL != p_gr_cache)
    {
        h_data = p_gr_cache->h_data;
        cpicture = p_gr_cache->cpicture;
    }

    trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_cache_search_cpicture(") ENUM_XTFMT TEXT(") yields ") PTR_XTFMT, cah, cpicture);
    *p_array_handle = h_data;
    return(cpicture);
}

#endif /* OS */

extern void
gr_cache_trash(void)
{
    LIST_ITEMNO key;
    P_GR_CACHE p_gr_cache;

    for(p_gr_cache = collect_first(GR_CACHE, &gr_cache_drawfiles, &key);
        p_gr_cache;
        p_gr_cache = collect_next(GR_CACHE, &gr_cache_drawfiles, &key))
    {
        gr_cache_entry_data_remove(p_gr_cache);
    }

    collect_delete(&gr_cache_drawfiles);
}

/******************************************************************************
*
* ensure Draw file for given entry is loaded
*
******************************************************************************/

static void
gr_cache_load_setup_foreign_diagram(
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _In_z_      PC_SBSTR szCreatorName)
{
    __pragma(warning(disable:6260)) /* sizeof * sizeof is usually wrong. Did you intend to use a character count or a byte count? */
    gr_riscdiag_diagram_init((P_DRAW_FILE_HEADER) array_range(p_h_data, BYTE, 0, sizeof32(DRAW_FILE_HEADER)), szCreatorName);

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

    memcpy32(array_range(p_h_data, BYTE, sizeof32(DRAW_FILE_HEADER), sizeof32(DRAW_OBJECT_OPTIONS)), &options, sizeof32(DRAW_OBJECT_OPTIONS));
    } /*block*/
}

#define HIMETRIC_PER_INCH 2540 /* 25.4 mm/inch * 100.0 hm/mm */

#define DrawUnitsFromHiMetric(hm) ( \
    muldiv64_ceil((hm), GR_RISCDRAW_PER_INCH, HIMETRIC_PER_INCH) )

_Check_return_
static STATUS
gr_cache_load(
    P_GR_CACHE p_gr_cache)
{
    FILE_HANDLE fin;
    P_BYTE readp;
    STATUS res = 1;
#if WINDOWS
    HGLOBAL hGlobal = NULL;
#endif

    /* reset error each time we try to load */
    p_gr_cache->error = 0;

    for(;;) /* loop for structure */
    {
        if(p_gr_cache->h_data)
        {
            /* already loaded */
            res = 1;
            break;
        }

        if(status_fail(res = t5_file_open((PCTSTR) (p_gr_cache + 1), file_open_read, &fin, TRUE)))
        {
            trace_1(TRACE_MODULE_GR_CHART, TEXT("gr_cache_load: cannot open: %s"), (PCTSTR) (p_gr_cache + 1));
            p_gr_cache->error = res;
            break;
        }

        /* another loop for structure */
        for(;;)
        {
            T5_FILETYPE t5_filetype = p_gr_cache->t5_filetype;
            U32 filelength;
            U32 drawlength;
            U32 spritelength = 0; /* keep dataflower happy */
            U32 object_offset = 0;
            U32 object_data_offset = 0;

            {
            filelength_t x_filelength;

            if(status_fail(res = file_length(fin, &x_filelength)))
            {
                p_gr_cache->error = res;
                break;
            }

            assert(      x_filelength.u.words.hi == 0);
            assert((S32) x_filelength.u.words.lo >= 0);
            filelength = (U32) x_filelength.u.words.lo;
            } /*block*/

            switch(t5_filetype) /* now kosher */
            {
            default: default_unhandled();
#if CHECKING
            case FILETYPE_DRAW:
#endif
                /* round up size to be paranoid */
                drawlength = round_up(filelength, 4);
                assert(drawlength == filelength); /* should already be a multiple of 4 */
                break;

            case FILETYPE_SPRITE:
                /* sprite files have a sprite area bound on the front without the length word */
                spritelength = filelength - (sizeof32(SAH) - 4);

                /* round up size for packaging in a Draw object */
                spritelength = round_up(spritelength, 4);

                object_offset = sizeof32(DRAW_FILE_HEADER) + sizeof32(DRAW_OBJECT_OPTIONS);
                object_data_offset = object_offset + sizeof32(DRAW_OBJECT_HEADER);

                drawlength = object_data_offset + spritelength;
                break;

            case FILETYPE_WINDOWS_BMP:
                /* BMP files have a BITMAPFILEHEADER on the front */
                spritelength = filelength - sizeof_BITMAPFILEHEADER;

                /* round up size for packaging in a Draw object */
                spritelength = round_up(spritelength, 4);

                object_offset = sizeof32(DRAW_FILE_HEADER) + sizeof32(DRAW_OBJECT_OPTIONS);
                object_data_offset = object_offset + sizeof32(DRAW_OBJECT_HEADER);

                drawlength = object_data_offset + spritelength;
                break;

            case FILETYPE_JPEG:
                /* round up size for packaging in a Draw object */
                spritelength = round_up(filelength, 4);

                object_offset = sizeof32(DRAW_FILE_HEADER) + sizeof32(DRAW_OBJECT_OPTIONS);
                object_data_offset = object_offset + sizeof32(DRAW_OBJECT_JPEG);

                drawlength = object_data_offset + spritelength;
                break;
            }

            reportf(TEXT("gr_cache_load: file length: ") U32_TFMT TEXT(", Draw file length: ") S32_TFMT, filelength, drawlength);

            {
            static /*poked*/ ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(BYTE), FALSE);
#if WINDOWS && 1
            /* use appropriate allocator - WILL be needed for Dial Solutions Draw file manipulation routines */
            array_init_block.use_alloc = ALLOC_USE_DS_ALLOC;
#elif WINDOWS && 0
            /* use appropriate allocator - may be needed for Dial Solutions Draw file manipulation routines */
            array_init_block.use_alloc = (FILETYPE_DRAW == t5_filetype) ? ALLOC_USE_DS_ALLOC : ALLOC_USE_ALLOC;
#endif
            if(NULL == (readp = al_array_alloc_BYTE(&p_gr_cache->h_data, drawlength, &array_init_block, &p_gr_cache->error)))
                break; /* out of another loop */
            } /*block*/

            /* Adjust the position where data is to be loaded */
            readp += object_data_offset;

            switch(t5_filetype)
            {
            default: default_unhandled();
            case FILETYPE_DRAW:
            case FILETYPE_JPEG:
                break;

            case FILETYPE_SPRITE:
                /* need to strip header too */
                assert(  (sizeof32(SAH) - 4) <= object_data_offset);
                readp -= (sizeof32(SAH) - 4);
                break;

            case FILETYPE_WINDOWS_BMP:
                /* need to strip header too */
                assert(  (sizeof_BITMAPFILEHEADER) <= object_data_offset);
                readp -= (sizeof_BITMAPFILEHEADER);
                break;
            }

            /* Load all of the file at this position */
            if(status_fail(res = file_read_bytes_requested(readp, filelength, fin)))
            {
                al_array_dispose(&p_gr_cache->h_data);
                p_gr_cache->error = res;
                break; /* out of another loop */
            }

            /* Got the file loaded in p_gr_cache->h_data */

            switch(t5_filetype)
            {
            case FILETYPE_DRAW:
                gr_cache_add_win_font_table(&p_gr_cache->h_data); /* SKS 12sep97 */
                gr_cache_spot_draw_bmp_object(&p_gr_cache->h_data); /* convert any OBJDIBs on RISC OS */
                gr_cache_spot_draw_jpeg_object(p_gr_cache);
                break;

            default: default_unhandled();
            case FILETYPE_SPRITE:
                break;

            case FILETYPE_WINDOWS_BMP:
                {
#if RISCOS
                /* Use the PicConvert module to convert it to a sprite
                   R0 = address of BMPFILEHEADER
                   R1 = 0 for enquiry call, otherwise address for output sprite
                   R2 = options for use with 24bit BMPs: use 0 for lowest o/p
                */
                STATUS status;
                _kernel_swi_regs rs;
                int options = 0; /* options => 24 bpp BMP -> 8 bpp,default palette sprite */
                S32 newlength;

                if(host_os_version_query() >= RISCOS_3_5)
                    options = 3; /* 24 bpp BMP -> 24 bpp sprite */

                /* Do Enq call */
                rs.r[0] = (int) readp;
                rs.r[1] = 0;
                rs.r[2] = options;

#define PicConvert_BMPtoFF9 0x00048E00

                if(_kernel_swi(/*PicConvert_BMPtoFF9*/ 0x48E00, &rs, &rs))
                {
                    status_break(gr_cache_ensure_PicConvert());

                    if(_kernel_swi(/*PicConvert_BMPtoFF9*/ 0x48E00, &rs, &rs))
                        /* failed so bind as BMP object into Drawfile regardless */
                        break;
                }

                newlength = rs.r[1];

                if(0 != newlength)
                {
                    ARRAY_HANDLE newhandle;
                    P_BYTE readpnew;

                    spritelength = newlength - (sizeof32(SAH) - 4);

                    newlength = object_data_offset + spritelength;

                    if(NULL != (readpnew = al_array_alloc_BYTE(&newhandle, newlength, &array_init_block_byte, &status)))
                        readpnew += object_data_offset - (sizeof32(SAH) - 4);

                    /* Do Conversion call */
                    rs.r[0] = (int) readp;
                    rs.r[1] = (int) readpnew;
                    rs.r[2] = options;

                    if((NULL == readpnew) || (NULL != _kernel_swi(/*PicConvert_BMPtoFF9*/ 0x48E00, &rs, &rs)))
                    {   /* failed - discard new memory and bind as BMP object into Draw file regardless */
                        al_array_dispose(&newhandle);
                    }
                    else
                    {
                        /* Throw away the original & use the new */
                        al_array_dispose(&p_gr_cache->h_data);

                        p_gr_cache->h_data = newhandle;

                        drawlength = newlength;

                        readp = readpnew;

                        t5_filetype = FILETYPE_SPRITE;

                        /* object_offset is unchanged */
                    }
                }
#endif
                break;
                }

            case FILETYPE_JPEG:
                {
#if WINDOWS
                /* Also copy the loaded data into a HGLOBAL */
                P_BYTE ptr = GlobalAllocAndLock(GMEM_MOVEABLE, filelength, &hGlobal);
                if(NULL != ptr)
                {
                    memcpy32(ptr, readp, filelength);
                    GlobalUnlock(hGlobal);
                }
#endif
                break;
                }
            }

            switch(t5_filetype)
            {
            case FILETYPE_SPRITE:
                {
                P_DRAW_OBJECT_SPRITE pSpriteObject;

                gr_cache_load_setup_foreign_diagram(&p_gr_cache->h_data, "FZC_SPRITE");

                /* sprite is the first and only real object in this diagram */
                pSpriteObject = (P_DRAW_OBJECT_SPRITE) array_ptr(&p_gr_cache->h_data, BYTE, object_offset);

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
                S32 excess   = array_elements(&p_gr_cache->h_data) - (S32) required;
                myassert2x(!excess, TEXT("about to strip ") S32_TFMT TEXT(" bytes after offset ") S32_TFMT TEXT(" in loaded sprite file"), excess, required);
                al_array_shrink_by(&p_gr_cache->h_data, -excess);
                } /*block*/

                gr_cache_rebind(&p_gr_cache->h_data);

                break;
                }

            case FILETYPE_WINDOWS_BMP:
                {
                P_DRAW_OBJECT_HEADER pObject;
                S32 size;

                gr_cache_load_setup_foreign_diagram(&p_gr_cache->h_data, "FZC_DIB");

                /* bitmap is the first and only real object in this diagram */
                pObject = (P_DRAW_OBJECT_HEADER) array_ptr(&p_gr_cache->h_data, BYTE, object_offset);

                pObject->type = DRAW_OBJECT_TYPE_DS_DIB; /* FILETYPE_WINDOWS_BMP aka DIB */

                size  = sizeof32(*pObject);

                size += spritelength; /* actual size */

                pObject->size = size;

                /* force duff bbox so riscdiag_object_reset_bbox will calculate default size, not retain */
                pObject->bbox.x0 =  0; /* but bl positioned at 0,0 for the moment */
                pObject->bbox.x1 = -1;
                pObject->bbox.y0 =  0;
                pObject->bbox.y1 = -1;

                gr_cache_rebind(&p_gr_cache->h_data);

                break;
                }

            case FILETYPE_JPEG:
                {
                U8 density_type = readp[13];
                U32 density_x = (U32) readval_U16_BE(&readp[14]);
                U32 density_y = (U32) readval_U16_BE(&readp[16]);
                P_DRAW_OBJECT_JPEG pJpegObject;
                S32 size;

                gr_cache_load_setup_foreign_diagram(&p_gr_cache->h_data, "FZC_JFIF");

                /* JPEG image is the first and only real object in this diagram */
                pJpegObject = (P_DRAW_OBJECT_JPEG) array_ptr(&p_gr_cache->h_data, BYTE, object_offset);

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

                pJpegObject->len = (int) filelength;

#if WINDOWS
                p_gr_cache->cpicture = CPicture_New(&res);

                if(NULL != p_gr_cache->cpicture)
                {
                    SIZE image_size;

                    consume_bool(CPicture_Load_HGlobal(p_gr_cache->cpicture, hGlobal));
                    /* Once it is loaded, we can free the hGlobal (done at the bottom) */

                    CPicture_GetImageSize(p_gr_cache->cpicture, &image_size); /* HIMETRIC (0.01mm) */

                    /* fill in rest of JPEG header */
                    pJpegObject->width  = image_size.cx;
                    pJpegObject->height = image_size.cy;
                    reportf(TEXT("CPicture_GetImageSize: w=%d, h=%d (hm), dpi x=%d,y=%d"), pJpegObject->width, pJpegObject->height, pJpegObject->dpi_x, pJpegObject->dpi_y);

                    pJpegObject->width  = DrawUnitsFromHiMetric(pJpegObject->width); /* convert from HIMETRIC to Draw units */
                    pJpegObject->height = DrawUnitsFromHiMetric(pJpegObject->height);
                }
#endif

#if RISCOS
                { /* fill in rest of JPEG header */
                _kernel_swi_regs rs;
                rs.r[0] = 1; /* return dimensions */
                rs.r[1] = (int) (pJpegObject+ 1); /* point to JPEG image */
                rs.r[2] = (int) filelength;
                if(NULL == _kernel_swi(/*JPEG_Info*/ 0x49980, &rs, &rs))
                {
                  /*if(rs.r[0] & (1<<2))
                        reportf("JPEG_Info: simple ratio");*/
                    pJpegObject->width  = rs.r[2]; /* pixels */
                    pJpegObject->height = rs.r[3];
                  /*pJpegObject->dpi_x  = rs.r[4];*/
                  /*pJpegObject->dpi_y  = rs.r[5];*/
                    reportf(TEXT("JPEG_Info: w=%d, h=%d (px), dpi x=%d,y=%d"), pJpegObject->width, pJpegObject->height, pJpegObject->dpi_x, pJpegObject->dpi_y);

                    pJpegObject->width  = (pJpegObject->width  * GR_RISCDRAW_PER_INCH) / pJpegObject->dpi_x; /* convert from pixels to Draw units */
                    pJpegObject->height = (pJpegObject->height * GR_RISCDRAW_PER_INCH) / pJpegObject->dpi_y;
                }
                } /*block*/

#endif
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

                gr_cache_rebind(&p_gr_cache->h_data);

                break;
                }

            } /* end of switch */

            break; /* end of another loop for structure */
            /*NOTREACHED*/
        }

        status_assert(t5_file_close(&fin)); /* don't mind so long as we've got the data loaded ... */

        break; /* end of loop for structure */
        /*NOTREACHED*/
    }

#if WINDOWS
    if(NULL != hGlobal)
    {
        GlobalFree(hGlobal);
        hGlobal = NULL;
    }
#endif

    res = p_gr_cache->error ? p_gr_cache->error : STATUS_DONE;

    return(res);
}

static void
gr_cache_add_win_font_table(
    P_ARRAY_HANDLE p_h_data /*inout, certainly poked*/)
{
#if WINDOWS
    draw_diag d;
    draw_error de;
    zero_struct(de);
    d.data = array_base(p_h_data, void);
    d.length = array_elements32(p_h_data);
    PTR_ASSERT(d.data);
    assert(0 != d.length);
    consume_bool(Draw_VerifyDiag(NULL, &d, FALSE /*rebind*/, TRUE /*AddWinFontTable*/, TRUE /*UpdateFontMap*/, &de));
    al_array_resized_ptr(p_h_data, d.data, d.length);
#else /* OS */
    IGNOREPARM(p_h_data);
#endif /* OS */
}

static void
gr_cache_rebind(
    P_ARRAY_HANDLE p_h_data /*inout, certainly poked*/)
{
    trace_0(TRACE_MODULE_GR_CHART, TEXT("gr_cache_rebind: file loaded, now verify"));

#if TRACE_ALLOWED
    { /* get box in Draw units - NB array of BYTE */
    PC_DRAW_FILE_HEADER pDrawFileHdr = (PC_DRAW_FILE_HEADER) array_rangec(p_h_data, BYTE, 0, sizeof32(DRAW_FILE_HEADER));
    DRAW_BOX box = pDrawFileHdr->bbox;
    trace_5(TRACE_MODULE_GR_CHART, TEXT("gr_cache_rebind: diagram bbox prior to verify ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(" (Draw), length ") S32_TFMT, box.x0, box.y0, box.x1, box.y1, array_elements(p_h_data));
    } /*block*/
#endif

    { /* MUST do this even when we call Draw_VerifyDiag afterwards */
    GR_RISCDIAG gr_riscdiag;
    GR_RISCDIAG_PROCESS_T process;
    gr_riscdiag_diagram_setup_from_data(&gr_riscdiag, array_base(p_h_data, BYTE), array_elements32(p_h_data));
    * (int *) &process = 0;
    process.recurse    = 1;
    process.recompute  = 1;
    gr_riscdiag_diagram_reset_bbox(&gr_riscdiag, process);
    } /*block*/

#if TRACE_ALLOWED
    { /* get box in Draw units - NB array of BYTE */
    PC_DRAW_FILE_HEADER pDrawFileHdr = (PC_DRAW_FILE_HEADER) array_rangec(p_h_data, BYTE, 0, sizeof32(DRAW_FILE_HEADER));
    DRAW_BOX box = pDrawFileHdr->bbox;
    trace_5(TRACE_MODULE_GR_CHART, TEXT("gr_cache_rebind: diagram bbox after verify ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(" (Draw), length ") S32_TFMT, box.x0, box.y0, box.x1, box.y1, array_elements(p_h_data));
    } /*block*/
#endif
}

#if RISCOS

/* Given a group object, add it to the list if the array */

static void
gr_cache_activate_group(
    _InVal_     S32 offset,
    P_BYTE p_base,
    P_ARRAY_HANDLE p_array_handle)
{
    STATUS status;
    P_S32 p_new = al_array_extend_by(p_array_handle, S32, 1, PC_ARRAY_INIT_BLOCK_NONE, &status);
    IGNOREPARM(p_base);
    PTR_ASSERT(p_new);
    *p_new = offset;
}

static void
gr_cache_kill_groups(
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
gr_cache_patch_groups(
    _InVal_     S32 offset,
    P_BYTE p_base,
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InVal_     S32 delta)
{
    const ARRAY_INDEX n_elements = array_elements(p_array_handle);
    ARRAY_INDEX index;

    IGNOREPARM_InVal_(offset);

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
gr_cache_convert_bmp_object(
    /*inout*/ P_ARRAY_HANDLE p_h_data,
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
        int options = 0;
        S32 newlength;
        P_BYTE readpnew;
        S32 required_size;
        S32 current_size;

        if(host_os_version_query() >= RISCOS_3_5)
            options = 3; /* 24 bpp BMP -> 24 bpp sprite */

        /* Do Enq call */
        rs.r[0] = (int) p_bitmapfileheader;
        rs.r[1] = 0;
        rs.r[2] = options;

        if(_kernel_swi(/*PicConvert_BMPtoFF9*/ 0x48E00, &rs, &rs))
        {
            status_break(gr_cache_ensure_PicConvert());

            if(_kernel_swi(/*PicConvert_BMPtoFF9*/ 0x48E00, &rs, &rs))
                break;
        }

        if(0 == (newlength = rs.r[1]))
            break;

        if(NULL == (readpnew = al_array_alloc_BYTE(&newhandle, newlength, &array_init_block_byte, &status)))
            break;

        /* Do Conversion call */
        rs.r[0] = (int) p_bitmapfileheader;
        rs.r[1] = (int) readpnew;
        rs.r[2] = options;

        if(_kernel_swi(/*PicConvert_BMPtoFF9*/ 0x48E00, &rs, &rs))
            break;

        required_size = newlength - sizeof32(SAH) + sizeof32(DRAW_OBJECT_HEADER);
        current_size  = pObject->size;
        delta         = required_size-current_size;

        if(delta) /* otherwise you got impossibly lucky! */
        {
            P_BYTE p_from = PtrAddBytes(P_BYTE, pObject, current_size);
            S32 move_size = (S32) array_elements32(p_h_data) - (p_from - array_base(p_h_data, BYTE));
            S32 offset_pObject = (P_BYTE) pObject - array_base(p_h_data, BYTE);

            if(delta > 0)
            {
                /* Getting bigger so realloc then move */
                if(NULL == al_array_extend_by_BYTE(p_h_data, delta, PC_ARRAY_INIT_BLOCK_NONE, &status))
                {
                    delta = 0;
                    break;
                }

                pObject = (P_DRAW_OBJECT_HEADER) array_ptr(p_h_data, BYTE, offset_pObject);

                p_from = (P_BYTE) pObject + current_size;

                memmove32(p_from + delta, p_from, move_size);
            }
            else
            {
                /* Getting smaller so move then realloc */
                p_from = PtrAddBytes(P_BYTE, pObject, current_size);

                memmove32(p_from + delta, p_from, move_size);

                al_array_shrink_by(p_h_data, delta);

                pObject = (P_DRAW_OBJECT_HEADER) array_ptr(p_h_data, BYTE, offset_pObject);
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

static void
gr_cache_spot_draw_bmp_object(
    /*inout*/ P_ARRAY_HANDLE p_h_data)
{
#if RISCOS
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
    gr_riscdiag_diagram_setup_from_data(&gr_riscdiag, array_base(p_h_data, BYTE), array_elements32(p_h_data));

    if(gr_riscdiag_object_first(p_gr_riscdiag, &sttObject, &endObject, (P_P_BYTE) &pObject, TRUE))
    {
        do  {
            if(pObject->type == DRAW_OBJECT_TYPE_GROUP) /* This group goes active */
                gr_cache_activate_group((P_BYTE) pObject - array_base(p_h_data, BYTE), array_base(p_h_data, BYTE), &array_handle_groups);

            /* remove any 'dead' groups */
            gr_cache_kill_groups((P_BYTE) pObject - array_base(p_h_data, BYTE), array_base(p_h_data, BYTE), &array_handle_groups);

            /* convert any DIB objects in place (to sprite) */
            if(pObject->type == DRAW_OBJECT_TYPE_DS_DIB)
            {
                S32 delta = gr_cache_convert_bmp_object(p_h_data, pObject);

                if(delta)
                {
                    gr_riscdiag_diagram_setup_from_data(&gr_riscdiag, array_base(p_h_data, BYTE), array_elements32(p_h_data));

                    endObject = p_gr_riscdiag->draw_diag.length;

                    gr_cache_patch_groups((P_BYTE) pObject - array_base(p_h_data, BYTE), array_base(p_h_data, BYTE), &array_handle_groups, delta);
                }
            }
        }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &sttObject, &endObject, (P_P_BYTE) &pObject, TRUE));
    }

    al_array_dispose(&array_handle_groups);
#else
    IGNOREPARM(p_h_data);
#endif
}

static void
gr_cache_spot_draw_jpeg_object(
    P_GR_CACHE p_gr_cache)
{
#if WINDOWS
    P_ARRAY_HANDLE p_h_data = &p_gr_cache->h_data;
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
    gr_riscdiag_diagram_setup_from_data(&gr_riscdiag, array_base(p_h_data, BYTE), array_elements32(p_h_data));

    if(gr_riscdiag_object_first(p_gr_riscdiag, &sttObject, &endObject, (P_BYTE *) &pObject, TRUE))
    {
        /* See if the whole draw file is composed of harmless options plus a JPEG */
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
            case DRAW_OBJECT_TYPE_OPTIONS:
                nHarmlessOptions++;
                break;
            }

            if(nOtherOptions != 0)
                break;
        }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &sttObject, &endObject, (P_BYTE *) &pObject, TRUE));

        if((jpeg_length != 0) && (nOtherOptions == 0))
        {
            /* Copy the data into a HGLOBAL */
            HGLOBAL hGlobal;
            P_BYTE ptr = GlobalAllocAndLock(GMEM_MOVEABLE, jpeg_length, &hGlobal);
            if(NULL != ptr)
            {
                STATUS status;

                memcpy32(ptr, jpeg_ptr, jpeg_length);
                GlobalUnlock(hGlobal);

                /* Create a CPicture */
                p_gr_cache->cpicture = CPicture_New(&status);
                if(NULL != p_gr_cache->cpicture)
                {
                    consume_bool(CPicture_Load_HGlobal(p_gr_cache->cpicture, hGlobal));
                }
                status_consume(status);

                GlobalFree(hGlobal);
            }
        }
    }
#else
    IGNOREPARM(p_gr_cache);
#endif
}

#if RISCOS

/* try loading the module - just the once, mind (remember the error too) */

_Check_return_
extern STATUS
gr_cache_ensure_PicConvert(void)
{
    static STATUS status = STATUS_OK;

    TCHARZ command_buffer[BUF_MAX_PATHSTRING];
    PTSTR tstr = command_buffer;

    if(STATUS_OK != status)
        return(status);

    tstr_xstrkpy(tstr, elemof32(command_buffer), "%RMLoad ");
    tstr += tstrlen32(tstr);

    status_return(status = file_find_on_path(tstr, elemof32(command_buffer) - (tstr - command_buffer), TEXT("RISC_OS.PicConvert")));

    if(STATUS_OK == status)
        return(status = create_error(FILE_ERR_NOTFOUND));

    if(_kernel_ERROR == _kernel_oscli(command_buffer))
        return(status = status_nomem());

    return(status = STATUS_DONE);
}

#endif /* RISCOS */

/* end of gr_cache.c */
