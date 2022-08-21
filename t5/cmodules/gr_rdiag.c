/* gr_rdiag.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RISC OS Draw file creation */

/* SKS August 1991 */

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

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

#if RISCOS
#define EXPOSE_RISCOS_FLEX 1
#define EXPOSE_RISCOS_FONT 1
#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"
#endif

#if WINDOWS
#include "external/Dial_Solutions/drawfile.h"
#endif

#ifndef GR_RISCDIAG_SIZE_INIT
#define GR_RISCDIAG_SIZE_INIT 0x1000
#endif

#ifndef GR_RISCDIAG_SIZE_INCR
#define GR_RISCDIAG_SIZE_INCR 0x1000
#endif

/*
transforms
*/

/*
scaling matrix for riscDraw units to pixits
*/

const GR_XFORMMATRIX
gr_riscdiag_pixit_from_riscDraw_xformer =
{
    /* &0000.0800 == 1/32 (inexact) */
    GR_SCALE_ONE / GR_RISCDRAW_PER_PIXIT, GR_SCALE_ZERO,

    GR_SCALE_ZERO, GR_SCALE_ONE / GR_RISCDRAW_PER_PIXIT,

    GR_SCALE_ZERO, GR_SCALE_ZERO
};

/*
scaling matrix for riscDraw units to OS units
*/

const GR_XFORMMATRIX
gr_riscdiag_riscos_from_riscDraw_xformer =
{
    /* &0000.0100 == 1/256 (inexact) */
    (GR_SCALE_ONE * GR_RISCOS_PER_INCH) / GR_RISCDRAW_PER_INCH, GR_SCALE_ZERO,

    GR_SCALE_ZERO, (GR_SCALE_ONE * GR_RISCOS_PER_INCH) / GR_RISCDRAW_PER_INCH,

    GR_SCALE_ZERO, GR_SCALE_ZERO
};

/*
scaling matrix for pixits to riscDraw units
*/

const GR_XFORMMATRIX
gr_riscdiag_riscDraw_from_pixit_xformer =
{
    /* &0020.0000 == 32 */
    GR_SCALE_ONE * GR_RISCDRAW_PER_PIXIT, GR_SCALE_ZERO,

    GR_SCALE_ZERO, GR_SCALE_ONE * GR_RISCDRAW_PER_PIXIT,

    GR_SCALE_ZERO, GR_SCALE_ZERO
};

/*
scaling matrix for millipoints to riscDraw units
*/

const GR_XFORMMATRIX
gr_riscdiag_riscDraw_from_mp_xformer =
{
    /* &0000.AD37 ~= 640/1000 (inexact) */
    (GR_SCALE_ONE * GR_RISCDRAW_PER_POINT) / GR_MILLIPOINTS_PER_POINT, GR_SCALE_ZERO,

    GR_SCALE_ZERO, (GR_SCALE_ONE * GR_RISCDRAW_PER_POINT) / GR_MILLIPOINTS_PER_POINT,

    GR_SCALE_ZERO, GR_SCALE_ZERO
};

/******************************************************************************
*
* delete a diagram
*
******************************************************************************/

extern void
gr_riscdiag_diagram_dispose(
    _InoutRef_  P_P_GR_RISCDIAG p_p_gr_riscdiag)
{
    P_GR_RISCDIAG p_gr_riscdiag;

    if(NULL != (p_gr_riscdiag = *p_p_gr_riscdiag))
    {
        *p_p_gr_riscdiag = NULL;

#if RISCOS
        flex_dispose((flex_ptr) &p_gr_riscdiag->draw_diag.data);
#else
        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_gr_riscdiag->draw_diag.data));
#endif /* OS */
        p_gr_riscdiag->draw_diag.length = 0;
        p_gr_riscdiag->dd_allocsize = 0;
    }
}

/******************************************************************************
*
* end adding data to a diagram
*
******************************************************************************/

extern U32
gr_riscdiag_diagram_end(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag)
{
    U32 n_bytes;
    GR_RISCDIAG_PROCESS_T process;

    /* close root group (if there is one) */
    if(0 != p_gr_riscdiag->dd_rootGroupStart)
        gr_riscdiag_group_end(p_gr_riscdiag, p_gr_riscdiag->dd_rootGroupStart);

    n_bytes = p_gr_riscdiag->draw_diag.length;

    /* reduce size of diagram to final size (never zero) */
    if(n_bytes != p_gr_riscdiag->dd_allocsize)
    {
        /* shrink */
#if RISCOS
        (void) flex_realloc((flex_ptr) &p_gr_riscdiag->draw_diag.data, (int) n_bytes);
        p_gr_riscdiag->dd_allocsize = n_bytes;
#elif WINDOWS
        /* SKS 26.03.2006 trimming a draw_diag seems to render it unexpandable
         * beyond that size EVEN AFTER BEING FREED and REALLOCED !!!
         * DSAFF.DLL does not appear to use GlobalSize() so I believe that we're ok ...
         */
#endif
    }

    /* always set sensible bbox */
    * (int *) &process = 0;
    process.recurse = 1;
    process.recompute = 1;
    gr_riscdiag_diagram_reset_bbox(p_gr_riscdiag, process);

#if WINDOWS
    /* SKS 09.05.2006 I think the trim problem may be caused by leaving it locked
     * as we still get problems eg with Gerald damped oscillation charts
     */
    gr_riscdiag_diagram_unlock(p_gr_riscdiag);

    if(p_gr_riscdiag->dd_fontListR && !p_gr_riscdiag->dd_fontListW)
    {
#if 0 /* get wrong size object added */
        draw_diag d;
        draw_error de;
        zero_struct(de);
        d.data = p_gr_riscdiag->draw_diag.hglobal;
        d.length = p_gr_riscdiag->draw_diag.length;
        consume_bool(Draw_VerifyDiag(NULL, &d, FALSE /*rebind*/, TRUE /*AddWinFontTable*/, TRUE /*UpdateFontMap*/, &de));
        assert(d.data == p_gr_riscdiag->draw_diag.hglobal);
        if(d.length != p_gr_riscdiag->draw_diag.length)
            n_bytes = p_gr_riscdiag->draw_diag.length = d.length;
#endif
    }
#endif

    return(n_bytes);
}

extern void
gr_riscdiag_diagram_init(
    _OutRef_    P_DRAW_FILE_HEADER pDrawFileHdr,
    _In_z_      PC_SBSTR szCreatorName)
{
    /* create the header manually */
    U32 i;

    CODE_ANALYSIS_ONLY(zero_struct_ptr(pDrawFileHdr));

    pDrawFileHdr->title[0] = 'D';
    pDrawFileHdr->title[1] = 'r';
    pDrawFileHdr->title[2] = 'a';
    pDrawFileHdr->title[3] = 'w';

    pDrawFileHdr->major_stamp = 201;
    pDrawFileHdr->minor_stamp = 0;

    for(i = 0; i < sizeofmemb32(DRAW_FILE_HEADER, creator_id); ++i)
    {
        U8 ch = *szCreatorName;
        if(CH_NULL != ch)
            szCreatorName++;
        else
            ch = CH_SPACE; /* NB program identification string must NOT be CH_NULL-terminated */
        pDrawFileHdr->creator_id[i] = ch;
    }

    draw_box_make_bad(&pDrawFileHdr->bbox);
}

_Check_return_
_Ret_maybenull_
extern P_DRAW_FILE_HEADER
gr_riscdiag_diagram_lock(
    _InoutRef_opt_ P_GR_RISCDIAG p_gr_riscdiag)
{
    if(NULL == p_gr_riscdiag)
        return(NULL);

    return((P_DRAW_FILE_HEADER) p_gr_riscdiag->draw_diag.data);
}

extern void
gr_riscdiag_diagram_unlock(
    _InoutRef_opt_ P_GR_RISCDIAG p_gr_riscdiag)
{
    if(NULL == p_gr_riscdiag)
        return;
}

/******************************************************************************
*
* start a diagram: allocate descriptor and initial chunk & initialise
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_diagram_new(
    _OutRef_    P_P_GR_RISCDIAG p_p_gr_riscdiag,
    _In_z_      PC_SBSTR szCreatorName,
    _InVal_     ARRAY_HANDLE array_handleR,
    _InVal_     ARRAY_HANDLE array_handleW,
    _InVal_     BOOL create_options_object)
{
    STATUS status;
    P_GR_RISCDIAG p_gr_riscdiag;
    P_DRAW_FILE_HEADER pDrawFileHdr;

    if(NULL == (*p_p_gr_riscdiag = p_gr_riscdiag = al_ptr_calloc_elem(GR_RISCDIAG, 1, &status)))
        return(status);

    p_gr_riscdiag->draw_diag.data = NULL;
    p_gr_riscdiag->draw_diag.length = 0;
    p_gr_riscdiag->dd_allocsize = 0;

    if(NULL == (pDrawFileHdr = gr_riscdiag_ensure(DRAW_FILE_HEADER, p_gr_riscdiag, sizeof32(DRAW_FILE_HEADER), &status)))
        return(status);

    gr_riscdiag_diagram_init(pDrawFileHdr, szCreatorName);

    status = STATUS_OK;

    /* create font list object(s) in top level - seems best for Draw renderers */
    if(array_elements32(&array_handleR) && status_ok(status))
        status = gr_riscdiag_fontlist_new(p_gr_riscdiag, &array_handleR, &array_handleW);

    /* create options object in top level - ditto */
    if(create_options_object && status_ok(status))
        status = gr_riscdiag_options_new(p_gr_riscdiag, &p_gr_riscdiag->dd_options);

    /* create root group */
    if(status_ok(status))
        status = gr_riscdiag_group_new(p_gr_riscdiag, &p_gr_riscdiag->dd_rootGroupStart, "RootGroup");

    /* clean up mess if needed */
    if(status_fail(status))
        gr_riscdiag_diagram_dispose(p_p_gr_riscdiag);

    return(status);
}

/******************************************************************************
*
* reset a diagram's bbox
*
******************************************************************************/

extern void
gr_riscdiag_diagram_reset_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     GR_RISCDIAG_PROCESS_T process)
{
    P_DRAW_FILE_HEADER pDrawFileHdr;
    DRAW_BOX diag_box;

    gr_riscdiag_object_reset_bbox_between(p_gr_riscdiag, &diag_box, DRAW_DIAG_OFFSET_FIRST /* first object */, DRAW_DIAG_OFFSET_LAST /* last object */, process);

    pDrawFileHdr = gr_riscdiag_getoffptr(DRAW_FILE_HEADER, p_gr_riscdiag, 0);

    pDrawFileHdr->bbox = diag_box;
}

/******************************************************************************
*
* save out a diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_diagram_save(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _In_z_      PCTSTR filename)
{
    FILE_HANDLE file_handle;
    STATUS status;

    status_return(status = t5_file_open(filename, file_open_write, &file_handle, TRUE));

#if RISCOS
    status_assert(file_set_risc_os_filetype(file_handle, FILETYPE_DRAW));
#endif

    status = gr_riscdiag_diagram_save_into(p_gr_riscdiag, file_handle);

    status_accumulate(status, t5_file_close(&file_handle));

    return(status_fail(status) ? status : STATUS_DONE);
}

/******************************************************************************
*
* save out a Draw file into an opened file
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_diagram_save_into(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InoutRef_  FILE_HANDLE file_handle)
{
    P_DRAW_FILE_HEADER pDrawFileHdr;
    STATUS status = STATUS_OK;

    if(NULL != (pDrawFileHdr = gr_riscdiag_diagram_lock(p_gr_riscdiag)))
    {
        status = file_write_bytes(pDrawFileHdr, p_gr_riscdiag->draw_diag.length, file_handle);

        gr_riscdiag_diagram_unlock(p_gr_riscdiag);
    }

    return(status);
}

/******************************************************************************
*
* set up a diagram from some data (usually for font scanning and such like)
*
******************************************************************************/

extern void
gr_riscdiag_diagram_setup_from_data(
    _OutRef_    P_GR_RISCDIAG p_gr_riscdiag,
    _In_reads_(diag_len) PC_BYTE p_diag,
    _InVal_     U32 diag_len)
{
    zero_struct_ptr(p_gr_riscdiag);

    p_gr_riscdiag->draw_diag.data = de_const_cast(P_BYTE, p_diag);

    p_gr_riscdiag->dd_allocsize = p_gr_riscdiag->draw_diag.length = diag_len;
}

/******************************************************************************
*
* ensure enough memory for bytes to be added to diagram
*
* --out--
*
*  -ve allocation error
*  +ve ok
*
******************************************************************************/

_Check_return_
_Ret_writes_maybenull_(size)
extern P_BYTE
_gr_riscdiag_ensure(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     U32 size,
    _OutRef_    P_STATUS p_status)
{
    P_BYTE result;

    *p_status = STATUS_OK;

    myassert0x(p_gr_riscdiag, TEXT("gr_riscdiag_ensure has no diagram"));

    assert((size & 3) == 0);

    /* need to get more space for the diag? */
    if(size > (p_gr_riscdiag->dd_allocsize - p_gr_riscdiag->draw_diag.length))
    {
        U32 requiredsize, allocsize, allocdelta;
        P_BYTE mp;

        if(0 != p_gr_riscdiag->draw_diag.length)
            requiredsize = p_gr_riscdiag->draw_diag.length + size;
        else
            requiredsize = MAX(size, GR_RISCDIAG_SIZE_INIT);

        allocsize = round_up(requiredsize, GR_RISCDIAG_SIZE_INCR);

        allocdelta = allocsize - p_gr_riscdiag->dd_allocsize;

#if RISCOS
        if(!flex_realloc((flex_ptr) &p_gr_riscdiag->draw_diag.data, (int) allocsize))
        {
            /* Try for only what's actually been requested rather than the rounded-up version */
            allocsize = requiredsize;

            allocdelta = allocsize - p_gr_riscdiag->dd_allocsize;

            if(!flex_realloc((flex_ptr) &p_gr_riscdiag->draw_diag.data, (int) allocsize))
                *p_status = status_nomem();
        }
#else
        {
        P_BYTE p_data;

        if(NULL != p_gr_riscdiag->draw_diag.data)
            p_data = al_ptr_realloc_bytes(P_BYTE, p_gr_riscdiag->draw_diag.data, allocsize, p_status);
        else
            p_data = al_ptr_alloc_bytes(P_BYTE, allocsize, p_status);

        if(NULL != p_data)
            p_gr_riscdiag->draw_diag.data = p_data;
        } /*block*/
#endif /* OS */

        if(status_fail(*p_status))
            return(NULL);

        /* point to new allocation; clear to zero.
         * depends on objects being discarded emptying their space for reuse
        */
        mp = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, p_gr_riscdiag->dd_allocsize);

        memset32(mp, 0, allocdelta);

        /* return pointer to base of ensured allocation and skip it */
        result = mp - p_gr_riscdiag->dd_allocsize + p_gr_riscdiag->draw_diag.length;

        p_gr_riscdiag->dd_allocsize = allocsize;
    }
    else
    {   /* return pointer to base of ensured allocation and skip it */
        result = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, p_gr_riscdiag->draw_diag.length);
    }

    p_gr_riscdiag->draw_diag.length += size;

    return(result);
}

/******************************************************************************
*
* create font list object(s) in the diagram
*
******************************************************************************/

_Check_return_
static STATUS
gr_riscdiag_fontlistR_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InRef_     PC_ARRAY_HANDLE p_array_handleR)
{
    DRAW_DIAG_OFFSET fontListStart = gr_riscdiag_query_offset(p_gr_riscdiag);

    if(0 == array_elements(p_array_handleR))
        return(STATUS_OK);

    {
    const ARRAY_INDEX n_elements = array_elements(p_array_handleR);
    ARRAY_INDEX i;
    P_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY p = array_range(p_array_handleR, GR_RISCDIAG_RISCOS_FONTLIST_ENTRY, 0, n_elements);
    U32 extraBytes = 0;

    for(i = 0; i < n_elements; ++i, ++p)
    {
        const DRAW_DIAG_OFFSET lenp1 = strlen32p1(p->szHostFontName); /*CH_NULL*/
        const DRAW_DIAG_OFFSET thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + lenp1;

        extraBytes += thislen;
    }

    /* round up size to 32bit boundary */
    extraBytes = (extraBytes + (4-1)) & ~(4-1);

    {
    STATUS status;
    P_BYTE pObject;
    DRAW_OBJECT_HEADER_NO_BBOX objhdr;

    objhdr.type = DRAW_OBJECT_TYPE_FONTLIST;
    objhdr.size = sizeof32(objhdr) + extraBytes;
    /* this doesn't have a bbox */

    if(NULL == (pObject = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, objhdr.size, &status)))
        return(status);

    memcpy32(pObject, &objhdr, sizeof32(objhdr));
    } /*block*/
    } /*block*/

    {
    const ARRAY_INDEX n_elements = array_elements(p_array_handleR);
    ARRAY_INDEX i;
    PC_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY p = array_range(p_array_handleR, GR_RISCDIAG_RISCOS_FONTLIST_ENTRY, 0, n_elements);
    DRAW_DIAG_OFFSET fontListPos = fontListStart + sizeof32(DRAW_OBJECT_HEADER_NO_BBOX);

    for(i = 0; i < n_elements; ++i, ++p)
    {
        const DRAW_DIAG_OFFSET lenp1 = strlen32p1(p->szHostFontName); /*CH_NULL*/
        const DRAW_DIAG_OFFSET thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + lenp1;
        const P_DRAW_FONTLIST_ELEM pFontListElemR = gr_riscdiag_getoffptr(DRAW_FONTLIST_ELEM, p_gr_riscdiag, fontListPos);

        pFontListElemR->fontref8 = (U8) (i + 1);

        memcpy32(pFontListElemR->szHostFontName, p->szHostFontName, lenp1);

        fontListPos += thislen;
    }

    { /* pad up to 3 bytes with 0 */
    P_BYTE pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, fontListPos);
    while((uintptr_t) pObject & (4-1))
        *pObject++ = CH_NULL;
    } /*block*/
    } /*block*/

    p_gr_riscdiag->dd_fontListR = fontListStart;

    return(STATUS_OK);
}

_Check_return_
static STATUS
gr_riscdiag_fontlistW_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InRef_     PC_ARRAY_HANDLE p_array_handleW)
{
    DRAW_DIAG_OFFSET fontListStart = gr_riscdiag_query_offset(p_gr_riscdiag);

    if(0 == array_elements(p_array_handleW))
        return(STATUS_OK);

    {
    U32 n = array_elements32(p_array_handleW);
    U32 extraBytes = sizeof32(DRAW_DS_WINFONTLIST_ELEM) * n;
    U32 allocBytes = extraBytes;
    STATUS status;
    P_BYTE pObject;
    DRAW_OBJECT_HEADER objhdr;

    zero_struct(objhdr); /* NB bounding box of DS windows font list object must be ignored */

    objhdr.type = DRAW_OBJECT_TYPE_DS_WINFONTLIST;
    objhdr.size = sizeof32(objhdr) + allocBytes;

    objhdr.bbox.x0 = (DRAW_COORD) 0x7FFFFFFF; /* match closely what Oak Draw emits */
    objhdr.bbox.y0 = (DRAW_COORD) 0x7FFFFFFF;
    objhdr.bbox.x1 = (DRAW_COORD) 0x80000000;
    objhdr.bbox.y1 = (DRAW_COORD) 0x80000000;

    if(NULL == (pObject = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, objhdr.size, &status)))
        return(status);

    memcpy32(pObject, &objhdr, sizeof32(objhdr));
    } /*block*/

    {
    const ARRAY_INDEX n_elements = array_elements(p_array_handleW);
    ARRAY_INDEX i;
    PC_DRAW_DS_WINDOWS_LOGFONT p_draw_ds_windows_logfont = array_range(p_array_handleW, DRAW_DS_WINDOWS_LOGFONT, 0, n_elements);
    DRAW_DIAG_OFFSET fontListPos = fontListStart + sizeof32(DRAW_OBJECT_HEADER);

    for(i = 0; i < n_elements; ++i, ++p_draw_ds_windows_logfont)
    {
        const P_DRAW_DS_WINFONTLIST_ELEM pFontListElemW = gr_riscdiag_getoffptr(DRAW_DS_WINFONTLIST_ELEM, p_gr_riscdiag, fontListPos);

        pFontListElemW->draw_font_ref16 = (DRAW_FONT_REF16) (i + 1);

        pFontListElemW->draw_ds_windows_logfont = *p_draw_ds_windows_logfont;

        fontListPos += sizeof32(*pFontListElemW);
    }
    } /*block*/

    p_gr_riscdiag->dd_fontListW = fontListStart;

    return(STATUS_OK);
}

_Check_return_
extern STATUS
gr_riscdiag_fontlist_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InRef_     PC_ARRAY_HANDLE p_array_handleR,
    _InRef_     PC_ARRAY_HANDLE p_array_handleW)
{
    BOOL usable_W = FALSE;

    if(array_elements(p_array_handleR))
    {
        status_return(gr_riscdiag_fontlistR_new(p_gr_riscdiag, p_array_handleR));

        /* must be the same size else how could we get matching font refs!? */
        if(array_elements(p_array_handleR) == array_elements(p_array_handleW))
            usable_W = TRUE;

        if(usable_W)
            status_return(gr_riscdiag_fontlistW_new(p_gr_riscdiag, p_array_handleW));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* search top level only of part of diagram for font list object(s)
* note that it sets it(them) too
*
******************************************************************************/

extern void
gr_riscdiag_fontlist_scan(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET sttObject_in,
    _InVal_     DRAW_DIAG_OFFSET endObject_in)
{
    DRAW_DIAG_OFFSET sttObject = sttObject_in;
    DRAW_DIAG_OFFSET endObject = endObject_in;
    P_BYTE pObject;

    p_gr_riscdiag->dd_fontListR = 0;
    p_gr_riscdiag->dd_fontListW = 0;

    if(gr_riscdiag_object_first(p_gr_riscdiag, &sttObject, &endObject, &pObject, FALSE)) /* flat scan good enough for what I want */
    {
        do  {
            switch(*DRAW_OBJHDR(U32, pObject, type))
            {
            default:
                break;

            case DRAW_OBJECT_TYPE_FONTLIST:
                p_gr_riscdiag->dd_fontListR = sttObject;
                break;

            case DRAW_OBJECT_TYPE_DS_WINFONTLIST:
                p_gr_riscdiag->dd_fontListW = sttObject;
                break;
            }
        }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &sttObject, &endObject, &pObject, FALSE));
    }
}

/******************************************************************************
*
* return the font ref that pertains to the given font list(s)
* and the (possibly external) fontlistelem(s)
*
******************************************************************************/

_Check_return_
extern DRAW_FONT_REF16
gr_riscdiag_fontlist_lookup_direct(
    _InRef_     PC_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET fontListR,
    _InVal_     DRAW_DIAG_OFFSET fontListW,
    _InRef_opt_ PC_DRAW_FONTLIST_ELEM pFontListElemR_lookup,
    _InRef_opt_ PC_DRAW_DS_WINFONTLIST_ELEM pFontListElemW_lookup)
{
    DRAW_FONT_REF16 fontRefNum = 0; /* will have to use System font unless request matched */

/*reportf(TEXT("%s(") PTR_XTFMT TEXT(", %d, %d)"), __Tfunc__, p_gr_riscdiag, fontListR, fontListW);*/
/*if(pFontListElemR_lookup) reportf(TEXT("R: %s"), report_sbstr(pFontListElemR_lookup->szHostFontName));*/
    if(fontListR && pFontListElemR_lookup)
    {
        const U32 lookup_lenp1 = strlen32p1(pFontListElemR_lookup->szHostFontName); /*CH_NULL*/
        P_DRAW_OBJECT_FONTLIST pFontListObject = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, p_gr_riscdiag, fontListR);
        DRAW_DIAG_OFFSET nextObject = fontListR + pFontListObject->size;
        DRAW_DIAG_OFFSET thisOffset = fontListR + sizeof32(*pFontListObject);
        PC_DRAW_FONTLIST_ELEM pFontListElemR_test = gr_riscdiag_getoffptr(DRAW_FONTLIST_ELEM, p_gr_riscdiag, thisOffset);

        /* actual end of RISC OS font list object data may not be word aligned */
        while((nextObject - thisOffset) >= 4)
        {
            const DRAW_DIAG_OFFSET test_lenp1 = strlen32p1(pFontListElemR_test->szHostFontName); /*CH_NULL*/
            const DRAW_DIAG_OFFSET thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + test_lenp1;

/*reportf(TEXT("fontListR %d, %s"), pFontListElemR_test->fontref8, report_sbstr(pFontListElemR_test->szHostFontName));*/
            if((test_lenp1 == lookup_lenp1) && (0 == C_stricmp(pFontListElemR_test->szHostFontName, pFontListElemR_lookup->szHostFontName)))
            {
                fontRefNum = (DRAW_FONT_REF16) pFontListElemR_test->fontref8;
                break;
            }

            thisOffset += thislen;

            pFontListElemR_test = PtrAddBytes(PC_DRAW_FONTLIST_ELEM, pFontListElemR_test, thislen);
        }
    }

/*if(pFontListElemW_lookup) reportf(TEXT("W: %s"), report_sbstr(pFontListElemW_lookup->draw_ds_windows_logfont.lfFaceName));*/
    if(fontListW && pFontListElemW_lookup && (0 == fontRefNum))
    {
        P_DRAW_OBJECT_FONTLIST pFontListObject;
        DRAW_DIAG_OFFSET nextObject, thisOffset;

        pFontListObject = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, p_gr_riscdiag, fontListW);

        nextObject = fontListW + pFontListObject->size;
        thisOffset = fontListW + sizeof32(*pFontListObject);

        /* actual end of Windows font list object data may not be word aligned (in this case, paranoia) */
        while((nextObject - thisOffset) >= 4)
        {
            PC_DRAW_DS_WINFONTLIST_ELEM pFontListElemW_test = gr_riscdiag_getoffptr(DRAW_DS_WINFONTLIST_ELEM, p_gr_riscdiag, thisOffset);

            if(0 == draw_ds_windows_logfont_compare(&pFontListElemW_test->draw_ds_windows_logfont, &pFontListElemW_lookup->draw_ds_windows_logfont))
            {
                fontRefNum = pFontListElemW_test->draw_font_ref16;
                break;
            }

            thisOffset += sizeof32(*pFontListElemW_test);
        }
    }

/*reportf(TEXT("%s returns fontref %d"), __Tfunc__, fontRefNum);*/
    return(fontRefNum);
}

/******************************************************************************
*
* return the fontlistelem(s) for a font ref that pertains to the given font list(s)
*
******************************************************************************/

extern void
gr_riscdiag_fontlist_lookup_fontref(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET fontListR,
    _InVal_     DRAW_DIAG_OFFSET fontListW,
    _OutRef_    P_DRAW_DIAG_OFFSET pFoundOffsetR,
    _OutRef_    P_DRAW_DIAG_OFFSET pFoundOffsetW,
    _InVal_     DRAW_FONT_REF16 fontRef)
{
/*reportf(TEXT("%s(") PTR_XTFMT TEXT(", %d, %d, %d)"), __Tfunc__, p_gr_riscdiag, fontListR, fontListW, fontRef);*/

    *pFoundOffsetR = 0;
    *pFoundOffsetW = 0;

    if(0 == fontRef)
        return;

    if(fontListR && (fontRef < 256))
    {
        P_DRAW_OBJECT_FONTLIST pFontListObject = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, p_gr_riscdiag, fontListR);
        DRAW_DIAG_OFFSET nextObject = fontListR + pFontListObject->size;
        DRAW_DIAG_OFFSET thisOffset = fontListR + sizeof32(*pFontListObject);
        PC_DRAW_FONTLIST_ELEM pFontListElemR = gr_riscdiag_getoffptr(DRAW_FONTLIST_ELEM, p_gr_riscdiag, thisOffset);

        /* actual unpadded end of RISC OS font list object data may not be word aligned */
        while((nextObject - thisOffset) >= 4)
        {
            const DRAW_DIAG_OFFSET lenp1 = strlen32p1(pFontListElemR->szHostFontName); /*CH_NULL*/
            const DRAW_DIAG_OFFSET thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + lenp1;

/*reportf(TEXT("%s: %d %s"), __Tfunc__, pFontListElemR->fontref8, report_sbstr(pFontListElemR->szHostFontName));*/
            if((DRAW_FONT_REF16) pFontListElemR->fontref8 == fontRef)
            {
/*reportf(TEXT("%s returns offsetR %d"), __Tfunc__, thisOffset);*/
                *pFoundOffsetR = thisOffset;
                return;
            }

            thisOffset += thislen;

            pFontListElemR = PtrAddBytes(PC_DRAW_FONTLIST_ELEM, pFontListElemR, thislen);
        }
    }

    if(fontListW)
    {
        P_DRAW_OBJECT_FONTLIST pFontListObject = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, p_gr_riscdiag, fontListW);
        DRAW_DIAG_OFFSET nextObject = fontListW + pFontListObject->size;
        DRAW_DIAG_OFFSET thisOffset = fontListW + sizeof32(*pFontListObject);

        /* actual unpadded end of Windows font list object data may not be word aligned (in this case, paranoia) */
        while((nextObject - thisOffset) >= 4)
        {
            PC_DRAW_DS_WINFONTLIST_ELEM pFontListElemW = gr_riscdiag_getoffptr(DRAW_DS_WINFONTLIST_ELEM, p_gr_riscdiag, thisOffset);

            if(pFontListElemW->draw_font_ref16 == fontRef)
            {
                *pFoundOffsetW = thisOffset;
                return;
            }

            thisOffset += sizeof32(*pFontListElemW);
        }
    }
}

/* DsFntMap.ini:
 * [Fonts]
 * nFonts=N
 * Font1=...
 * ...
 * FontN=RISC_OS_Full_Name,lfWeight(400|700),lfItalic(0|255),lfUnderline(0|1),lfStrikeOut(0|1),lfCharSet(0..255),lfPitchAndFamily(eg.32),lfFaceName
 */

_Check_return_
extern int
draw_ds_windows_logfont_compare(
    _InRef_     PC_DRAW_DS_WINDOWS_LOGFONT p_draw_ds_windows_logfont_1,
    _InRef_     PC_DRAW_DS_WINDOWS_LOGFONT p_draw_ds_windows_logfont_2)
{
    /* Ignores lfHeight, lfWidth, lfEscapement, lfOrientation, lfOutPrecision, lfClipPrecision, lfQuality */
    if(p_draw_ds_windows_logfont_1->lfWeight != p_draw_ds_windows_logfont_2->lfWeight)
        return(1);

    if(p_draw_ds_windows_logfont_1->lfItalic != p_draw_ds_windows_logfont_2->lfItalic)
        return(1);

    if(p_draw_ds_windows_logfont_1->lfUnderline != p_draw_ds_windows_logfont_2->lfUnderline)
        return(1);

    if(p_draw_ds_windows_logfont_1->lfStrikeOut != p_draw_ds_windows_logfont_2->lfStrikeOut)
        return(1);

    if(p_draw_ds_windows_logfont_1->lfCharSet != p_draw_ds_windows_logfont_2->lfCharSet)
        return(1);

    if(p_draw_ds_windows_logfont_1->lfPitchAndFamily != p_draw_ds_windows_logfont_2->lfPitchAndFamily)
        return(1);

    /* NB case-sensitive, "C" */
    if(0 != /*"C"*/strcmp(p_draw_ds_windows_logfont_1->lfFaceName, p_draw_ds_windows_logfont_2->lfFaceName))
        return(1);

    return(0);
}

extern void
draw_ds_windows_logfont_from_textstyle(
    _OutRef_    P_DRAW_DS_WINDOWS_LOGFONT p_draw_ds_windows_logfont,
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle,
    _In_z_      PC_U8Z szHostFontName)
{
    zero_struct_ptr(p_draw_ds_windows_logfont);
    p_draw_ds_windows_logfont->lfWeight = 400 /*FW_NORMAL*/;
    if(p_gr_textstyle->bold) p_draw_ds_windows_logfont->lfWeight = 700 /*FW_BOLD*/;
    p_draw_ds_windows_logfont->lfItalic = (p_gr_textstyle->italic ? 0xFFU : 0x00U);
#if WINDOWS
    p_draw_ds_windows_logfont->lfCharSet = ANSI_CHARSET;
    p_draw_ds_windows_logfont->lfOutPrecision = OUT_STROKE_PRECIS;
    p_draw_ds_windows_logfont->lfClipPrecision = CLIP_STROKE_PRECIS;
    p_draw_ds_windows_logfont->lfQuality = DRAFT_QUALITY;
    p_draw_ds_windows_logfont->lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;
#else
    /* the rest of these are already zero */
#endif
    xstrkpy(p_draw_ds_windows_logfont->lfFaceName, sizeof32(p_draw_ds_windows_logfont->lfFaceName), szHostFontName);
}

/******************************************************************************
*
* end a group: go back and patch its size field (leave bbox till diag end)
*
******************************************************************************/

extern U32
gr_riscdiag_group_end(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET groupStart)
{
    return(gr_riscdiag_object_end(p_gr_riscdiag, groupStart));
}

/******************************************************************************
*
* start a group: leave size & bbox empty to be patched later
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_group_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pGroupStart,
    _In_z_      PC_SBSTR pGroupName)
{
    DRAW_OBJECT_GROUP group;
    U32 i;
    STATUS status;
    P_BYTE pObject;

    group.type = DRAW_OBJECT_TYPE_GROUP;
    group.size = sizeof32(group);
    draw_box_make_bad(&group.bbox);

    for(i = 0; i < sizeof32(group.name); ++i)
    {
        U8 ch = *pGroupName;
        if(CH_NULL != ch)
            pGroupName++;
        else
            ch = CH_SPACE; /* NB group name in Draw diagram must NOT be CH_NULL-terminated */
        group.name[i] = ch;
    }

    *pGroupStart = gr_riscdiag_query_offset(p_gr_riscdiag);

    if(NULL != (pObject = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, group.size, &status)))
        memcpy32(pObject, &group, sizeof32(group));

    return(status);
}

/******************************************************************************
*
* ensure start and end object offsets are good for scan or whatever
*
******************************************************************************/

_Check_return_
extern DRAW_DIAG_OFFSET
gr_riscdiag_normalise_stt(
    _InRef_     P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET sttObject_in)
{
    DRAW_DIAG_OFFSET sttObject = sttObject_in;

    myassert0x(p_gr_riscdiag && p_gr_riscdiag->draw_diag.length, TEXT("gr_riscdiag_normalise_stt has no diagram"));
    IGNOREPARM_InRef_(p_gr_riscdiag);

    if(sttObject == DRAW_DIAG_OFFSET_FIRST)
        sttObject = sizeof32(DRAW_FILE_HEADER);

    myassert1x(sttObject >= sizeof32(DRAW_FILE_HEADER), TEXT("gr_riscdiag_normalise_stt has sttObject ") U32_XTFMT TEXT(" < sizeof-DRAW_FILE_HEADER"), sttObject);
    myassert2x(sttObject <= p_gr_riscdiag->draw_diag.length, TEXT("gr_riscdiag_normalise_stt has sttObject ") U32_XTFMT TEXT(" > p_gr_riscdiag->draw_diag.length ") U32_XTFMT, sttObject, p_gr_riscdiag->draw_diag.length);

    return(sttObject);
}

_Check_return_
extern DRAW_DIAG_OFFSET
gr_riscdiag_normalise_end(
    _InRef_     P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET endObject_in)
{
    DRAW_DIAG_OFFSET endObject = endObject_in;

    myassert0x(p_gr_riscdiag && p_gr_riscdiag->draw_diag.length, TEXT("gr_riscdiag_normalise_end has no diagram"));

    if(endObject == DRAW_DIAG_OFFSET_LAST)
        endObject = p_gr_riscdiag->draw_diag.length;

    myassert1x(endObject >= sizeof32(DRAW_FILE_HEADER), TEXT("gr_riscdiag_normalise_end has endObject ") U32_XTFMT TEXT(" < sizeof-DRAW_FILE_HEADER"), endObject);
    myassert2x(endObject <= p_gr_riscdiag->draw_diag.length, TEXT("gr_riscdiag_normalise_end has endObject ") U32_XTFMT TEXT(" > p_gr_riscdiag->draw_diag.length ") U32_XTFMT, endObject, p_gr_riscdiag->draw_diag.length);

    return(endObject);
}

/******************************************************************************
*
* return amount of space to be allocated for the header of this object
*
******************************************************************************/

extern U32
gr_riscdiag_object_base_size(
    _InVal_     U32 type)
{
    switch(type)
    {
    case DRAW_OBJECT_TYPE_FONTLIST: return(sizeof32(DRAW_OBJECT_HEADER_NO_BBOX));

    case DRAW_OBJECT_TYPE_TEXT:     return(sizeof32(DRAW_OBJECT_TEXT));
    case DRAW_OBJECT_TYPE_PATH:     return(sizeof32(DRAW_OBJECT_PATH));
    case DRAW_OBJECT_TYPE_GROUP:    return(sizeof32(DRAW_OBJECT_GROUP));

    default:                        return(sizeof32(DRAW_OBJECT_HEADER));
    }
}

/******************************************************************************
*
* end an object: go back and patch its size field
*
******************************************************************************/

extern U32
gr_riscdiag_object_end(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET objectStart)
{
    P_BYTE pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, objectStart);
    U32 n_bytes = p_gr_riscdiag->draw_diag.length - objectStart;

    /*trace_4(0, TEXT("gr_riscdiag_object_end(") PTR_XTFMT TEXT("): started ") U32_XTFMT " ended ") U32_XTFMT TEXT(" n_bytes ") U32_XTFMT, p_gr_riscdiag, *pObjectStart, p_gr_riscdiag->draw_diag.length, n_bytes);*/

    if(n_bytes == gr_riscdiag_object_base_size(*DRAW_OBJHDR(U32, pObject, type)))
    {
        /*trace_0(0, TEXT("gr_riscdiag_object_end: destroy object and zero contents (for reuse) as nothing in it"));*/
        memset32(pObject, 0, n_bytes);
        p_gr_riscdiag->draw_diag.length = objectStart;
        n_bytes = 0;
    }
    else
    {   /* update object size */
        *DRAW_OBJHDR(U32, pObject, size) = n_bytes;
    }

    return(n_bytes);
}

/******************************************************************************
*
* diagram scanning: loop over objects
*
******************************************************************************/

_Check_return_
extern BOOL
gr_riscdiag_object_first(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InoutRef_  P_DRAW_DIAG_OFFSET pSttObject,
    _InoutRef_  P_DRAW_DIAG_OFFSET pEndObject,
    _OutRef_    P_P_BYTE ppObject,
    _InVal_     BOOL recurse)
{
    P_BYTE pObject;
    DRAW_DIAG_OFFSET thisObject;

    *pSttObject = gr_riscdiag_normalise_stt(p_gr_riscdiag, *pSttObject);
    *pEndObject = gr_riscdiag_normalise_end(p_gr_riscdiag, *pEndObject);

    thisObject = *pSttObject;

    pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, thisObject);

#if CHECKING
    {
    /* see comments in gr_riscdiag_object_next regarding recurse option */
    const U32 objectType = *DRAW_OBJHDR(U32, pObject, type);
    const U32 objectSize = ((objectType == DRAW_OBJECT_TYPE_GROUP) && recurse)
                             ? sizeof32(DRAW_OBJECT_GROUP)
                             : *DRAW_OBJHDR(U32, pObject, size);

    myassert2x((objectSize >= sizeof32(DRAW_OBJECT_HEADER)) ||
              ((objectType == DRAW_OBJECT_TYPE_FONTLIST) && (objectSize >= sizeof32(DRAW_OBJECT_FONTLIST))),
              TEXT("gr_riscdiag_object_first object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" < sizeof-DRAW_OBJECT_HEADER"), thisObject, objectSize);
    myassert3x(thisObject + objectSize <= *pEndObject,
              TEXT("gr_riscdiag_object_first object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" goes beyond end ") U32_XTFMT, thisObject, objectSize, *pEndObject);
    } /*block*/
#else
    IGNOREPARM_InVal_(recurse);
#endif /* CHECKING */

    /* stay at this first object */

    if(*pSttObject >= *pEndObject)
    {
        *pSttObject = DRAW_DIAG_OFFSET_FIRST;
        pObject = NULL;
    }

    *ppObject = pObject;

    return(*pSttObject != DRAW_DIAG_OFFSET_FIRST); /* NOT DRAW_FILE_HEADER!!! note above repoke^^^ on end */
}

/******************************************************************************
*
* diagram scanning: loop over objects
*
******************************************************************************/

_Check_return_
extern BOOL
gr_riscdiag_object_next(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InoutRef_  P_DRAW_DIAG_OFFSET pSttObject,
    _InRef_     PC_DRAW_DIAG_OFFSET pEndObject,
    _OutRef_    P_P_BYTE ppObject,
    _InVal_     BOOL recurse)
{
    P_BYTE pObject;
    DRAW_DIAG_OFFSET thisObject = *pSttObject;
    U32 objectType;
    U32 objectSize;

    assert(thisObject < p_gr_riscdiag->draw_diag.length);
    pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, thisObject);

    /* note that the recurse flag merely allows entry to group objects - there is no actual recursion */
    objectType = *DRAW_OBJHDR(U32, pObject, type);
    objectSize = ((objectType == DRAW_OBJECT_TYPE_GROUP) && recurse)
                             ? sizeof32(DRAW_OBJECT_GROUP)
                             : *DRAW_OBJHDR(U32, pObject, size);

    myassert2x((objectSize >= sizeof32(DRAW_OBJECT_HEADER)) ||
              ((objectType == DRAW_OBJECT_TYPE_FONTLIST) && (objectSize >= sizeof32(DRAW_OBJECT_FONTLIST))),
              TEXT("gr_riscdiag_object_next object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" < sizeof-DRAW_OBJECT_HEADER"), thisObject, objectSize);

    if(thisObject + objectSize <= *pEndObject)
    {
        /* skip current object, move to next */

        *pSttObject = thisObject + objectSize;
        pObject += objectSize;

        if(*pSttObject >= *pEndObject)
        {
            *pSttObject = DRAW_DIAG_OFFSET_FIRST;
            pObject = NULL;
        }
    }
    else
    {
        /* SKS 07may95 moved from being a simple assertion to a real safety net */
        myassert3x(thisObject + objectSize <= *pEndObject,
                  TEXT("gr_riscdiag_object_next object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" goes beyond end ") U32_XTFMT, thisObject, objectSize, *pEndObject);

        *pSttObject = DRAW_DIAG_OFFSET_FIRST;
        pObject = NULL;
    }

    *ppObject = pObject;

    return(*pSttObject != DRAW_DIAG_OFFSET_FIRST); /* NOT DRAW_FILE_HEADER!!! note above repoke^^^ on end */
}

/******************************************************************************
*
* recompute the bbox of an object - fairly limited
*
******************************************************************************/

extern void
gr_riscdiag_object_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET objectStart)
{
    PC_BYTE pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, objectStart);

    switch(*DRAW_OBJHDR(U32, pObject, type))
    {
    case DRAW_OBJECT_TYPE_TEXT:
        gr_riscdiag_string_recompute_bbox(p_gr_riscdiag, objectStart);
        break;

    case DRAW_OBJECT_TYPE_PATH:
        gr_riscdiag_path_recompute_bbox(p_gr_riscdiag, objectStart);
        break;

    case DRAW_OBJECT_TYPE_SPRITE:
        /* sprite bbox defines its size not vice versa */
        /* however it may be forced bad, in which case we shall make default size */
        gr_riscdiag_sprite_recompute_bbox(p_gr_riscdiag, objectStart);
        break;

    case DRAW_OBJECT_TYPE_JPEG:
        /* sprite comments apply too */
        gr_riscdiag_jpeg_recompute_bbox(p_gr_riscdiag, objectStart);
        break;

    case DRAW_OBJECT_TYPE_DS_DIB:
        /* sprite comments apply too */
        gr_riscdiag_dib_recompute_bbox(p_gr_riscdiag, objectStart);
        break;

    default:
        break;
    }
}

/******************************************************************************
*
* reset a bbox over a range of objects
*
******************************************************************************/

extern void
gr_riscdiag_object_reset_bbox_between(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_BOX pBox,
    _InVal_     DRAW_DIAG_OFFSET sttObject_in,
    _InVal_     DRAW_DIAG_OFFSET endObject_in,
    _InVal_     GR_RISCDIAG_PROCESS_T process)
{
    P_BYTE pObject;
    DRAW_DIAG_OFFSET thisObject = gr_riscdiag_normalise_stt(p_gr_riscdiag, sttObject_in);
    const DRAW_DIAG_OFFSET endObject = gr_riscdiag_normalise_end(p_gr_riscdiag, endObject_in);

    myassert1x(p_gr_riscdiag && p_gr_riscdiag->draw_diag.length, TEXT("gr_riscdiag_object_reset_bbox_between has no diagram ") PTR_XTFMT, p_gr_riscdiag);

    /* loop over constituent objects and extract bbox */

    draw_box_make_bad(pBox);

    /* keep pObject valid for as long as we can, dropping and reloading where needed */
    pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, thisObject);

    while(thisObject < endObject)
    {
        DRAW_OBJECT_HEADER objhdr;

        memcpy32(&objhdr, pObject, sizeof32(objhdr));

        myassert2x((objhdr.size >= sizeof32(DRAW_OBJECT_HEADER)) ||
                  ((objhdr.type == DRAW_OBJECT_TYPE_FONTLIST) && (objhdr.size >= sizeof32(DRAW_OBJECT_FONTLIST))),
                  TEXT("gr_riscdiag_object_reset_bbox_between object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" < sizeof-DRAW_OBJECT_HEADER"), thisObject, objhdr.size);
        myassert3x(thisObject + objhdr.size <= endObject,
                  TEXT("gr_riscdiag_object_reset_bbox_between object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" goes beyond end ") U32_XTFMT, thisObject, objhdr.size, endObject);

        switch(objhdr.type)
        {
        case DRAW_OBJECT_TYPE_FONTLIST: /* font tables do not have bounding boxes - beware! */
            p_gr_riscdiag->dd_fontListR = thisObject;
            break;

        case DRAW_OBJECT_TYPE_DS_WINFONTLIST: /* these have an invalid bounding box (usually all-zeros from us, crossed max from Oak Draw) */
            p_gr_riscdiag->dd_fontListW = thisObject;
            break;

        case DRAW_OBJECT_TYPE_OPTIONS: /* these have an invalid bounding box (usually all-zeros) */
            p_gr_riscdiag->dd_options = thisObject;
            break;

        case DRAW_OBJECT_TYPE_GROUP:
            /* NB. groups skipped wholesale (assumed correct unless recursing) */
            if(process.recurse)
            {
                gr_riscdiag_object_reset_bbox_between(p_gr_riscdiag, &objhdr.bbox, thisObject + sizeof32(DRAW_OBJECT_GROUP), thisObject + objhdr.size, process);

                if(process.recompute)
                {
                    pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, thisObject);

                    memcpy32(pObject + offsetof32(DRAW_OBJECT_HEADER, bbox), &objhdr.bbox, sizeof32(objhdr.bbox));
                }
            }

            draw_box_union(pBox, pBox, &objhdr.bbox);
            break;

        case DRAW_OBJECT_TYPE_TAG:
            {
            /* can't do anything with tagged object goop bar skip it - for bboxing,
             * take bbox of tagged object (not the tag), skipping header - but see RCM
             * SKS 03jun93 removed RISC OS 2 !Draw tagged file support 'cos there never were any files like this
            */
            U32 tagHdrSize = sizeof32(DRAW_OBJECT_TAG);

            draw_box_union(pBox, pBox, PtrAddBytes(PC_DRAW_BOX, pObject, tagHdrSize + offsetof32(DRAW_OBJECT_HEADER, bbox)));

            break;
            }

        default:
            if(process.recompute)
            {
                gr_riscdiag_object_recompute_bbox(p_gr_riscdiag, thisObject);

                pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, thisObject);

                memcpy32(&objhdr.bbox, pObject + offsetof32(DRAW_OBJECT_HEADER, bbox), sizeof32(objhdr.bbox));
            }

            draw_box_union(pBox, pBox, &objhdr.bbox);
            break;
        }

        thisObject += objhdr.size;
        pObject += objhdr.size;
    }
}

/******************************************************************************
*
* create an options object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_options_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pOptionsStart)
{
    DRAW_OBJECT_OPTIONS options;
    P_BYTE pObject;
    STATUS status;

    zero_struct(options); /* NB bounding box of options object must be ignored */

    options.type = DRAW_OBJECT_TYPE_OPTIONS;
    options.size = sizeof32(options);

    options.paper_size = (4 + 1) << 8; /* A4 */
    options.paper_limits.defaults = 1;

    writeval_F64_as_ARM(&options.grid_spacing[0], 1.0);
    options.grid_division = 10;
    options.grid_units = 1; /* cm */

    options.zoom_multiplier = 1;
    options.zoom_divider = 1;

    options.initial_entry_mode.select = 1;

    options.undo_buffer_bytes = 5000;

    *pOptionsStart = gr_riscdiag_query_offset(p_gr_riscdiag);

    if(NULL != (pObject = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, options.size, &status)))
        memcpy32(pObject, &options, sizeof32(options));

    return(status);
}

#if RISCOS

static S32
gr_round_to_ceil(
    _InVal_     S32 a,
    _InVal_     S32 b)
{
    return( (S32)  ((a <= 0) ? a : (a + b - 1)) / b );
}

static S32
gr_round_to_floor(
    _InVal_     S32 a,
    _InVal_     S32 b)
{
    return( (S32)  ((a >= 0) ? a : (a - b + 1)) / b );
}

#endif

/******************************************************************************
*
* recompute a path object's bbox
*
******************************************************************************/

extern void
gr_riscdiag_path_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET pathStart)
{
    DRAW_OBJECT_PATH path;
    P_BYTE pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, pathStart);
    P_BYTE path_seq = pObject + sizeof32(path);  /* path really starts here (unless dashed) */
    P_BYTE line_dash_pattern = NULL;
    DRAW_BOX bbox;
#if RISCOS
    DRAW_MODULE_CAP_JOIN_SPEC cjspec;
#else
    DS_DRAW_CAP_JOIN_SPEC cjspec;
#endif

    /* compute bbox of path object by asking the Draw module */
    memcpy32(&path, pObject, sizeof32(path));

    if(path.pathstyle.flags & DRAW_PS_DASH_PACK_MASK)
    {
        line_dash_pattern = path_seq;

        path_seq = line_dash_pattern + sizeof32(DRAW_DASH_HEADER)
                 + sizeof32(S32) * (* (P_U32) (line_dash_pattern + offsetof32(DRAW_DASH_HEADER, dashcount)));
    }

    {
    U32 temp; /* eliminate wierd compiler winges */
    temp = ((U32) path.pathstyle.flags & (U32) DRAW_PS_JOIN_PACK_MASK    ) >> ((U32) DRAW_PS_JOIN_PACK_SHIFT);
    cjspec.join_style           = (unsigned char) temp;
    temp = ((U32) path.pathstyle.flags & (U32) DRAW_PS_STARTCAP_PACK_MASK) >> ((U32) DRAW_PS_STARTCAP_PACK_SHIFT);
    cjspec.leading_cap_style    = (unsigned char) temp;
    temp = ((U32) path.pathstyle.flags & (U32) DRAW_PS_ENDCAP_PACK_MASK  ) >> ((U32) DRAW_PS_ENDCAP_PACK_SHIFT);
    cjspec.trailing_cap_style   = (unsigned char) temp;
    cjspec.reserved             = 0;
    cjspec.mitre_limit = 0x000A0000; /* 10.0 "PostScript default" from DrawFiles doc'n */
#if RISCOS
    cjspec.leading_tricap_width     = (U16) (path.pathstyle.tricap_w * path.pathwidth) / 16;
    cjspec.leading_tricap_height    = (U16) (path.pathstyle.tricap_h * path.pathwidth) / 16;
    cjspec.trailing_tricap_width    = cjspec.leading_tricap_width;
    cjspec.trailing_tricap_height   = cjspec.leading_tricap_height;
#else
    cjspec.leading_tricap_width     = (U16) (path.pathstyle.tricap_w * path.pathwidth) / 16;
    cjspec.leading_tricap_height    = (U16) (path.pathstyle.tricap_h * path.pathwidth) / 16;
    cjspec.trailing_tricap_width    = cjspec.leading_tricap_width;
    cjspec.trailing_tricap_height   = cjspec.leading_tricap_height;
#endif
    } /*block*/

    {
    int fill_style = DMFT_PATH_Reflatten | DMFT_PATH_Thicken | DMFT_PATH_Flatten | DMFT_PLOT_Bint | DMFT_PLOT_NonBint | DMFT_PLOT_Bext;
#if RISCOS
    _kernel_swi_regs rs;

    rs.r[0] = (int)  path_seq;
    rs.r[1] =        fill_style;
    rs.r[2] =        NULL; /* xform matrix */
    rs.r[3] =        2 * GR_RISCDRAW_PER_RISCOS; /* flatness (no DrawFiles recommendation. Draw module recommends 2 OS units) */
    rs.r[4] = (int)  path.pathwidth; /* thickness */
    rs.r[5] = (int) &cjspec;
    rs.r[6] = (int)  line_dash_pattern;
    rs.r[7] = (int) &bbox | 0x80000000; /* where to put bbox */

    void_WrapOsErrorChecking(_kernel_swi(/*Draw_ProcessPath*/ 0x040700, &rs, &rs));
#elif WINDOWS
    draw_error de;
    zero_struct(de);

    consume_bool(
        Draw_PathBbox(
            (HPLONG) path_seq,
            fill_style,
            path.pathwidth /* thickness */,
            (draw_dashstr *) line_dash_pattern,
            (draw_capjoinspec *) &cjspec,
            (HPDRAWBOX) &bbox,
            &de));
#endif
    } /*block*/

    if((bbox.x0 > bbox.x1) || (bbox.y0 > bbox.y1))
    {
        /* SKS after 1.05 25oct93 desperate fix for Ola Lind bug - how can Draw_ProcessPath return such shite? */
        assert((bbox.x0 <= bbox.x1) || (bbox.y0 <= bbox.y1));
        assert(((PC_S32) path_seq)[0] == 2);
        bbox.x0 = bbox.x1 = ((PC_S32) path_seq)[1];
        bbox.y0 = bbox.y1 = ((PC_S32) path_seq)[2];
    }
    else
    {
        /* SKS 22aug93 add half a worst-case pixel all round */
        bbox.x0 -= (2 * RISCDRAW_PER_RISCOS) / 2;
        bbox.y0 -= (4 * RISCDRAW_PER_RISCOS) / 2;
        bbox.x1 += (2 * RISCDRAW_PER_RISCOS) / 2;
        bbox.y1 += (4 * RISCDRAW_PER_RISCOS) / 2;
    }

    memcpy32(pObject + offsetof32(DRAW_OBJECT_HEADER, bbox), &bbox, sizeof32(bbox));
}

/******************************************************************************
*
* recompute a sprite object's bbox
*
******************************************************************************/

#if WINDOWS

_Check_return_
static int
windows_sprite_bpp(
    _InVal_     U32 mode)
{
    U32 t = (mode >> 27) & 0x1F;

    if(t)
    { /* new format sprites are easy */
        switch(t)
        {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            return(1<< (t - 1));

         case 7:
            return(32); /*cmyk */

         case 8:
            return(24);

        default:
            return(1);
        }
    }

    switch(mode)
    {
    case 0:
    case 4:
    case 18:
    case 23:
    case 25:
    case 29:
    case 33:
    case 37:
    case 41:
    case 44:
        return(1);

    case 1:
    case 3:
    case 5:
    case 6:
    case 8:
    case 11:
    case 19:
    case 26:
    case 30:
    case 34:
    case 38:
    case 42:
    case 45:
        return(2);

    default:
        /* unknown mode */
    case 2:
    case 7:
    case 9:
    case 12:
    case 14:
    case 16:
    case 17:
    case 20:
    case 27:
    case 31:
    case 35:
    case 39:
    case 43:
    case 46:
        return(4);

    case 10:
    case 13:
    case 15:
    case 21:
    case 24:
    case 28:
    case 36:
    case 40:
        return(8);
    }
}

static void
windows_sprite_dpi(
    _InVal_     S32 mode,
    _OutRef_    P_GDI_SIZE dpi)
{
    if((mode >> 27) != 0)
    { /* new format sprites are easy */
        dpi->cx = (mode >>  1) & 0xFFF;
        dpi->cy = (mode >> 14) & 0xFFF;
        return;
    }

    switch(mode)
    {
    case 2:
    case 10:
        dpi->cx = 22;
        dpi->cy = 45;
        return;

    case 1:
    case 4:
    case 9:
    case 13:
        dpi->cx = 45;
        dpi->cy = 45;
        return;

    case 0:
    case 8:
    case 12:
    case 15:
    case 16:
    case 17:
    case 24:
        dpi->cx = 90;
        dpi->cy = 45;
        return;

    default:
        /* unknown mode */
    case 18:
    case 19:
    case 20:
    case 21:
    case 25:
    case 26:
    case 27:
    case 28:
        dpi->cx = 90;
        dpi->cy = 90;
        return;
    }
}

#endif /* WINDOWS */

extern void
gr_riscdiag_sprite_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET spriteStart)
{
    P_BYTE pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, spriteStart);

    /* only recalculate (and therefore set default size) if deliberately made bad */
    if(*DRAW_OBJBOX(pObject, x0) >= *DRAW_OBJBOX(pObject, x1))
    {
        PC_BYTE p_sprite = pObject + sizeof32(DRAW_OBJECT_HEADER);
        PC_SCB p_scb = (PC_SCB) p_sprite;
        S32 x1, y1;
#if RISCOS
        SPRITE_MODE_WORD sprite_mode_word;
        _kernel_swi_regs rs;

        sprite_mode_word.u.u32 = (U32) p_scb->mode;

        rs.r[0] = 40 + 512;
        rs.r[1] = (int) 0x89ABFEDC; /* kill the OS or any twerp who dares to access this! */
        rs.r[2] = (int) p_sprite;

        void_WrapOsErrorChecking(_kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs));

        if((sprite_mode_word.u.u32 < 256U) || (0 == (sprite_mode_word.u.riscos_3_5.mode_word_bit)))
        {   /* Mode Number or Mode Specifier */
            S32 XEigFactor, YEigFactor;
            host_modevar_cache_query_eigs(p_scb->mode, &XEigFactor, &YEigFactor);
            x1 = (rs.r[3] << XEigFactor) * GR_RISCDRAW_PER_RISCOS;
            y1 = (rs.r[4] << YEigFactor) * GR_RISCDRAW_PER_RISCOS;
        }
        else if(SPRITE_TYPE_RO5_WORD == sprite_mode_word.u.riscos_3_5.type)
        {   /* RISC OS 5 style Mode Word */
            x1 = (rs.r[3] * GR_RISCDRAW_PER_INCH) / (180 >> sprite_mode_word.u.riscos_5.x_eig);
            y1 = (rs.r[4] * GR_RISCDRAW_PER_INCH) / (180 >> sprite_mode_word.u.riscos_5.y_eig);
        }
        else
        {   /* RISC OS 3.5 style Mode Word */
            x1 = (rs.r[3] * GR_RISCDRAW_PER_INCH) / sprite_mode_word.u.riscos_3_5.h_dpi;
            y1 = (rs.r[4] * GR_RISCDRAW_PER_INCH) / sprite_mode_word.u.riscos_3_5.v_dpi;
        }
#elif WINDOWS
        int bpp = windows_sprite_bpp(p_scb->mode);
        GDI_SIZE pixels, dpi;
        pixels.cx = ((p_scb->lwidth + 1) /* width in 32-bit words */ * 32) / bpp;
        pixels.cy = (p_scb->lheight + 1); /* number of scan lines */
        windows_sprite_dpi(p_scb->mode, &dpi);
        x1 = muldiv64(pixels.cx, GR_RISCDRAW_PER_INCH, dpi.cx);
        y1 = muldiv64(pixels.cy, GR_RISCDRAW_PER_INCH, dpi.cy);
#endif

        /* assumes bbox.x0, bbox.y0 correct */
        *DRAW_OBJBOX(pObject, x1) = *DRAW_OBJBOX(pObject, x0) + x1;
        *DRAW_OBJBOX(pObject, y1) = *DRAW_OBJBOX(pObject, y0) + y1;
    }
}

/******************************************************************************
*
* recompute a dib object's bbox
*
******************************************************************************/

extern void
gr_riscdiag_dib_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET dibStart)
{
    P_BYTE pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, dibStart);

    /* only recalculate (and therefore set default size) if deliberately made bad */

    if(*DRAW_OBJBOX(pObject, x0) >= *DRAW_OBJBOX(pObject, x1))
    {
        P_BYTE p_bitmapinfoheader = pObject + sizeof32(DRAW_OBJECT_HEADER);
        BITMAPINFOHEADER bitmapinfoheader;
        S32 x1, y1;

        memcpy32(&bitmapinfoheader, p_bitmapinfoheader, sizeof32(bitmapinfoheader));

        /* SKS 16oct95 cope with crap converters. 23nov95 do for RISCOS too */
        if(0 == bitmapinfoheader.biXPelsPerMeter)
            x1 = muldiv64(bitmapinfoheader.biWidth,  GR_RISCDRAW_PER_INCH, 96);
        else
            x1 = muldiv64(bitmapinfoheader.biWidth,  GR_RISCDRAW_PER_INCH * INCHES_PER_METRE_MUL, bitmapinfoheader.biXPelsPerMeter * INCHES_PER_METRE_DIV);

        if(0 == bitmapinfoheader.biYPelsPerMeter)
            y1 = muldiv64(bitmapinfoheader.biHeight, GR_RISCDRAW_PER_INCH, 96);
        else
            y1 = muldiv64(bitmapinfoheader.biHeight, GR_RISCDRAW_PER_INCH * INCHES_PER_METRE_MUL, bitmapinfoheader.biYPelsPerMeter * INCHES_PER_METRE_DIV);

        /* assumes bbox.x0, bbox.y0 correct */
        *DRAW_OBJBOX(pObject, x1) = *DRAW_OBJBOX(pObject, x0) + x1;
        *DRAW_OBJBOX(pObject, y1) = *DRAW_OBJBOX(pObject, y0) + y1;
    }
}

/******************************************************************************
*
* recompute a jpeg object's bbox
*
******************************************************************************/

extern void
gr_riscdiag_jpeg_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET jpegStart)
{
    P_DRAW_OBJECT_JPEG pJpegObject = gr_riscdiag_getoffptr(DRAW_OBJECT_JPEG, p_gr_riscdiag, jpegStart);

    /* only recalculate (and therefore set default size) if deliberately made bad */

    if(pJpegObject->bbox.x0 >= pJpegObject->bbox.x1)
    {
        S32 x1, y1;

        x1 = pJpegObject->width;  //muldiv64(pJpegObject->width,  GR_RISCDRAW_PER_INCH, pJpegObject->dpi_x);
        y1 = pJpegObject->height; //muldiv64(pJpegObject->height, GR_RISCDRAW_PER_INCH, pJpegObject->dpi_y);

        /* assumes bbox.x0, bbox.y0 correct */
        pJpegObject->bbox.x1 = pJpegObject->bbox.x0 + x1;
        pJpegObject->bbox.y1 = pJpegObject->bbox.y0 + y1;
    }
}

/******************************************************************************
*
* recompute a string's bbox
*
******************************************************************************/

extern void
gr_riscdiag_string_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET textStart)
{
    P_BYTE pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, textStart);
    DRAW_OBJECT_TEXT text;

    memcpy32(&text, pObject, sizeof32(text));

#if RISCOS

    {
    const PC_SBSTR sbstr = PtrAddBytes(PC_SBSTR, pObject, sizeof32(text));
    HOST_FONT host_font = HOST_FONT_NONE;
    DRAW_DIAG_OFFSET foundOffsetR, foundOffsetW;
    _kernel_swi_regs rs;
    _kernel_oserror * p_kernel_oserror;

    gr_riscdiag_fontlist_lookup_fontref(p_gr_riscdiag, p_gr_riscdiag->dd_fontListR, p_gr_riscdiag->dd_fontListW, &foundOffsetR, &foundOffsetW, text.textstyle.fontref16);

    if(foundOffsetR)
    {
        PC_DRAW_FONTLIST_ELEM pFontListElemR = gr_riscdiag_getoffptr(DRAW_FONTLIST_ELEM, p_gr_riscdiag, foundOffsetR);

        /* structure font sizes stored in 1/640th point; RISC OS FontManager requires 1/16 point */
        S32 x16_font_size_y =                ((16 * text.fsize_y) / 640);
        S32 x16_font_size_x = text.fsize_x ? ((16 * text.fsize_x) / 640) : x16_font_size_y;

        rs.r[1] = (int) pFontListElemR->szHostFontName;
        rs.r[2] = x16_font_size_x;
        rs.r[3] = x16_font_size_y;
        rs.r[4] = 0;
        rs.r[5] = 0;

        if(NULL == (p_kernel_oserror = WrapOsErrorChecking(_kernel_swi(/*Font_FindFont*/ 0x040081, &rs, &rs))))
            host_font = (HOST_FONT) rs.r[0];
    }

    if(HOST_FONT_NONE != host_font)
    {
        (void) font_SetFont(host_font);

        rs.r[1] = (int) sbstr;
        void_WrapOsErrorChecking(_kernel_swi(/*Font_StringBBox*/ 0x40097, &rs, &rs));
        /* NB. result in mp */

        /* scale bbox to riscDraw units - imprecise, but slightly better than a gr_box_xform() */
        text.bbox.x0 = (DRAW_COORD) gr_round_to_floor(rs.r[1] * GR_RISCDRAW_PER_POINT, GR_MILLIPOINTS_PER_POINT);
        text.bbox.y0 = (DRAW_COORD) gr_round_to_floor(rs.r[2] * GR_RISCDRAW_PER_POINT, GR_MILLIPOINTS_PER_POINT);
        text.bbox.x1 = (DRAW_COORD) gr_round_to_ceil( rs.r[3] * GR_RISCDRAW_PER_POINT, GR_MILLIPOINTS_PER_POINT);
        text.bbox.y1 = (DRAW_COORD) gr_round_to_ceil( rs.r[4] * GR_RISCDRAW_PER_POINT, GR_MILLIPOINTS_PER_POINT);

        WrapOsErrorReporting(font_LoseFont(host_font));
    }
    else
    {
        /* have a stab at VDU 5 System font bboxing
         * (I'd wondered why 1/640th point; its GR_POINTS_PER_RISCDRAW!)
         * note that Draw expects standard System font to be 12.80pt x 6.40pt
        */
        text.bbox.x0 = 0;
        text.bbox.y0 = 0;
        text.bbox.x1 = text.fsize_x * strlen(sbstr);
        text.bbox.y1 = text.fsize_y;
    }
    } /*block*/

    /* move box to have its origin at string baseline origin (text.coord) */
    draw_box_translate(&text.bbox, &text.bbox, &text.coord);

#else

    /* zero the object's bbox (paranoid) */
    text.bbox.x0 = 0;
    text.bbox.x1 = 0;
    text.bbox.y0 = 0;
    text.bbox.y1 = 0;
    memcpy32(pObject + offsetof32(DRAW_OBJECT_HEADER, bbox), &text.bbox, sizeof32(text.bbox));

    {
    draw_matrix tx, inv_tx;
    draw_diag d;

    d.data = p_gr_riscdiag->draw_diag.data;
    d.length = p_gr_riscdiag->draw_diag.length;

    tx.a = 0x00010000;
    tx.b = 0x00000000;
    tx.c = 0x00000000;
    tx.d = 0x00010000;
    tx.e = 0;
    tx.f = 0;

    inv_tx = tx; /*Draw_InverseTransform(&tx, &inv_tx);*/

    Draw_GetAccurateTextBbox(NULL, &d, textStart, &tx, &inv_tx, (draw_box *) &text.bbox);
    } /*block*/

    /* NB box has already been computed with its origin at string baseline origin (text.coord) */

#endif /* OS */

    /* update the object's bbox with that just calculated */
    memcpy32(pObject + offsetof32(DRAW_OBJECT_HEADER, bbox), &text.bbox, sizeof32(text.bbox));
}

/* end of gr_rdiag.c */
