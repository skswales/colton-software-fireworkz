/* gr_diag.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Representation building */

/* SKS July 1991 */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h"
#endif

/*
internal functions
*/

_Check_return_
static inline GR_DIAG_OFFSET
gr_diag_normalise_stt(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET sttObject_in)
{
    GR_DIAG_OFFSET sttObject = sttObject_in;
    UNREFERENCED_PARAMETER_InRef_(p_gr_diag);

    myassert0x(p_gr_diag && array_elements32(&p_gr_diag->handle), TEXT("gr_diag_normalise_stt has no diagram"));

    if(sttObject == GR_DIAG_OBJECT_FIRST)
        sttObject = sizeof32(GR_DIAG_DIAGHEADER);

    myassert1x(sttObject >= sizeof32(GR_DIAG_DIAGHEADER), TEXT("gr_diag_normalise_stt has sttObject ") U32_XTFMT TEXT(" < sizeof-gr_diag_diagheader"), sttObject);
    myassert2x(sttObject <= array_elements32(&p_gr_diag->handle), TEXT("gr_diag_normalise_stt has sttObject ") U32_XTFMT TEXT(" > array_elements() ") U32_XTFMT, sttObject, array_elements32(&p_gr_diag->handle));

    return(sttObject);
}

_Check_return_
static inline GR_DIAG_OFFSET
gr_diag_normalise_end(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET endObject_in)
{
    GR_DIAG_OFFSET endObject = endObject_in;

    myassert0x(p_gr_diag && array_elements32(&p_gr_diag->handle), TEXT("gr_diag_normalise_end has no diagram"));
    UNREFERENCED_PARAMETER_InRef_(p_gr_diag);

    if(endObject == GR_DIAG_OBJECT_LAST)
        endObject = array_elements32(&p_gr_diag->handle);

    myassert1x(endObject >= sizeof32(GR_DIAG_DIAGHEADER), TEXT("gr_diag_normalise_end has endObject ") U32_XTFMT TEXT(" < sizeof-gr_diag_diagheader"), endObject);
    myassert2x(endObject <= array_elements32(&p_gr_diag->handle), TEXT("gr_diag_normalise_end has endObject ") U32_XTFMT TEXT(" > array_elements() ") U32_XTFMT, endObject, array_elements32(&p_gr_diag->handle));

    return(endObject);
}

_Check_return_
static STATUS
gr_diag_create_riscdiag_between(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in);

_Check_return_
static STATUS
gr_diag_create_riscdiag_font_tables_between(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    _OutRef_    P_ARRAY_HANDLE p_array_handleR,
    _OutRef_    P_ARRAY_HANDLE p_array_handleW);

_Check_return_
static BOOL
gr_diag_line_hit(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET testObject,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size);

_Check_return_
static BOOL
gr_diag_piesector_hit(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET testObject,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size);

_Check_return_
static BOOL
gr_diag_quadrilateral_hit(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET testObject,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size);

/******************************************************************************
*
* round to ± infinity at multiples of a given number
* (occasionally need to round to real pixels etc.)
* (oh, for function overloading...)
*
******************************************************************************/

static GR_PIXIT
gr_round_pixit_to_ceil(
    _InVal_     GR_PIXIT a,
    _InVal_     S32 b)
{
    return( (GR_PIXIT)  ((a <= 0) ? a : (a + b - 1)) / b );
}

static GR_PIXIT
gr_round_pixit_to_floor(
    _InVal_     GR_PIXIT a,
    _InVal_     S32 b)
{
    return( (GR_PIXIT)  ((a >= 0) ? a : (a - b + 1)) / b );
}

/* diagrams are built as a pair of representations:
 * i) the mostly-system independent diagram
 * ii) a system-specific representation e.g. RISC OS Draw file
 * there being stored in (i) offsets to the corresponding objects in (ii)
 * to enable correlations, clipping etc.
*/

#ifndef GR_DIAG_SIZE_INIT
#define GR_DIAG_SIZE_INIT 1024
#endif

#ifndef GR_DIAG_SIZE_INCR
#define GR_DIAG_SIZE_INCR 512
#endif

/******************************************************************************
*
* run over this diagram forming the system-dependent representation
*
******************************************************************************/

_Check_return_
static STATUS
gr_diag_create_riscdiag(
    _InoutRef_  P_GR_DIAG p_gr_diag)
{
    P_GR_DIAG_DIAGHEADER pDiagHdr;
    ARRAY_HANDLE font_table_array_handleR;
    ARRAY_HANDLE font_table_array_handleW;
    STATUS status;

    myassert2x(p_gr_diag && p_gr_diag->handle, TEXT("gr_diag_create_riscdiag has no diagram ") PTR_XTFMT TEXT("->") U32_TFMT, p_gr_diag, p_gr_diag ? p_gr_diag->handle : 0);

    gr_riscdiag_diagram_dispose(&p_gr_diag->p_gr_riscdiag);

    pDiagHdr = array_base(&p_gr_diag->handle, GR_DIAG_DIAGHEADER);

    status_return(gr_diag_create_riscdiag_font_tables_between(p_gr_diag, GR_DIAG_OBJECT_FIRST, GR_DIAG_OBJECT_LAST, &font_table_array_handleR, &font_table_array_handleW));

    if(status_ok(status = gr_riscdiag_diagram_new(&p_gr_diag->p_gr_riscdiag, _sbstr_from_tstr(pDiagHdr->szCreatorName), font_table_array_handleR, font_table_array_handleW, TRUE /*options*/)))
        status = gr_diag_create_riscdiag_between(p_gr_diag, GR_DIAG_OBJECT_FIRST, GR_DIAG_OBJECT_LAST);

    if(status_ok(status))
    {
        gr_riscdiag_diagram_end(p_gr_diag->p_gr_riscdiag);
        status = STATUS_OK; /* paranoia */
    }
    else
        gr_riscdiag_diagram_dispose(&p_gr_diag->p_gr_riscdiag);

    al_array_dispose(&font_table_array_handleR);
    al_array_dispose(&font_table_array_handleW);

    return(status);
}

_Check_return_
static STATUS
gr_diag_create_riscdiag_between(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in)
{
    P_GR_RISCDIAG p_gr_riscdiag = p_gr_diag->p_gr_riscdiag;
    GR_DIAG_OFFSET thisObject = gr_diag_normalise_stt(p_gr_diag, sttObject_in);
    const GR_DIAG_OFFSET endObject = gr_diag_normalise_end(p_gr_diag, endObject_in);
    STATUS status = STATUS_OK;

    myassert2x(p_gr_diag && p_gr_diag->handle, TEXT("gr_diag_create_riscdiag_between has no diagram ") PTR_XTFMT TEXT("->") U32_TFMT, p_gr_diag, p_gr_diag ? p_gr_diag->handle : 0);

    while(thisObject < endObject)
    {
        P_BYTE pObject;
        GR_DIAG_OBJHDR objhdr;

        pObject = array_ptr(&p_gr_diag->handle, BYTE, thisObject);

        memcpy32(&objhdr, pObject, sizeof32(objhdr));

        /*trace_3(0, TEXT("gr_diag_create_riscdiag_between(") U32_XTFMT TEXT(",") U32_XTFMT TEXT(") now at ") U32_XTFMT, sttObject_in, endObject, thisObject);*/

        myassert2x(objhdr.n_bytes >= sizeof32(objhdr),
                  TEXT("gr_diag_create_riscdiag_between object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" > sizeof-objhdr"), thisObject, objhdr.n_bytes);
        myassert3x((thisObject + objhdr.n_bytes) <= endObject,
                  TEXT("gr_diag_create_riscdiag_between object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" goes beyond end ") U32_XTFMT, thisObject, objhdr.n_bytes, endObject);

        objhdr.sys_off = DRAW_DIAG_OFFSET_NONE; /* just in case */

        switch(objhdr.type)
        {
        case GR_DIAG_OBJTYPE_GROUP:
            {
            /* recurse unconditionally creating group contents.
             * could proceed linearly if we had a patchup list - but it don't go that deep
            */
            status_break(status = gr_riscdiag_group_new(p_gr_riscdiag, &objhdr.sys_off, "" /**pObject.group->name ? pObject.group->name :*/));

            status = gr_diag_create_riscdiag_between(p_gr_diag, thisObject + sizeof32(GR_DIAG_OBJGROUP), thisObject + objhdr.n_bytes);

            /* fix up the group even if error */
            gr_riscdiag_group_end(p_gr_riscdiag, objhdr.sys_off);

            break;
            }

        case GR_DIAG_OBJTYPE_TEXT:
            {
            GR_DIAG_OBJTEXT text;
            DRAW_POINT draw_point;
            GR_POINT gr_point;
            GR_COLOUR fg;
            int segments = 0;

            memcpy32(&text, pObject, sizeof32(text));

            gr_point = text.pos;
            fg = text.textstyle.fg;

            { /* count number of segments to ouput */
            PC_USTR textp = PtrAddBytes(PC_USTR, pObject, sizeof32(text));

            for(;;)
            {
                PC_USTR segendp = textp;
                U32 seglen = 0;
                U8 ch;

                while(((ch = PtrGetByte(segendp)) != CH_NULL) && (ch != 10))
                {
                    ustr_IncByte(segendp);
                    ++seglen;
                }

                if((CH_NULL == ch) && (0 == seglen))
                    break;

                ++segments; /* SKS 15feb96 */

                if(CH_NULL == ch)
                    break;

                textp = ustr_AddBytes(segendp, 1); /* skip LF to next segment */
            }
            } /*block*/

            if(segments >= 2)
                status_break(status = gr_riscdiag_group_new(p_gr_riscdiag, &objhdr.sys_off, "MultiLineTxt"));

            draw_point_from_gr_point(&draw_point, &gr_point);

            {
            PC_USTR textp = PtrAddBytes(PC_USTR, pObject, sizeof32(text));

            for(;;)
            {
                DRAW_DIAG_OFFSET sys_off;
                PC_USTR segendp = textp;
                U32 seglen = 0;
                U8 ch;

                while(((ch = PtrGetByte(segendp)) != CH_NULL) && (ch != 10))
                {
                    ustr_IncByte(segendp);
                    ++seglen;
                }

                if((CH_NULL == ch) && (0 == seglen))
                    break;

                status_break(status = gr_riscdiag_string_new_uchars(p_gr_riscdiag, &sys_off, &draw_point, textp, seglen, &text.textstyle, fg, NULL, p_gr_riscdiag));

                if(segments < 2)
                    objhdr.sys_off = sys_off;

                draw_point.y -= gr_riscDraw_from_pixit((text.textstyle.size_y * 12) / 10);

                if(CH_NULL == ch)
                    break;

                textp = ustr_AddBytes(segendp, 1); /* skip LF to next segment */
            }
            } /*block*/

            /* fix up the group even if error */
            if(segments >=2 )
                gr_riscdiag_group_end(p_gr_riscdiag, objhdr.sys_off);

            break;
            }

        case GR_DIAG_OBJTYPE_RECTANGLE:
            {
            GR_DIAG_OBJRECTANGLE rect;
            DRAW_POINT draw_point;
            DRAW_SIZE draw_size;

            memcpy32(&rect, pObject, sizeof32(rect));

            draw_point_from_gr_point(&draw_point, &rect.pos);

            draw_size.cx = gr_riscDraw_from_pixit(rect.size.cx);
            draw_size.cy = gr_riscDraw_from_pixit(rect.size.cy);

            status = gr_riscdiag_rectangle_new(p_gr_riscdiag, &objhdr.sys_off, &draw_point, &draw_size, &rect.linestyle, &rect.fillstylec);

            break;
            }

        case GR_DIAG_OBJTYPE_LINE:
            {
            GR_DIAG_OBJLINE line;
            DRAW_POINT draw_point, draw_offset;

            memcpy32(&line, pObject, sizeof32(line));

            draw_point_from_gr_point(&draw_point, &line.pos);
            draw_point_from_gr_point(&draw_offset, &line.offset);

            status = gr_riscdiag_line_new(p_gr_riscdiag, &objhdr.sys_off, &draw_point, &draw_offset, &line.linestyle);

            break;
            }

        case GR_DIAG_OBJTYPE_QUADRILATERAL:
            {
            GR_DIAG_OBJQUADRILATERAL quad;
            DRAW_POINT draw_point, draw_offset1, draw_offset2, draw_offset3;

            memcpy32(&quad, pObject, sizeof32(quad));

            draw_point_from_gr_point(&draw_point,  &quad.pos);
            draw_point_from_gr_point(&draw_offset1, &quad.offset1);
            draw_point_from_gr_point(&draw_offset2, &quad.offset2);
            draw_point_from_gr_point(&draw_offset3, &quad.offset3);

            status = gr_riscdiag_quadrilateral_new(p_gr_riscdiag, &objhdr.sys_off, &draw_point, &draw_offset1, &draw_offset2, &draw_offset3, &quad.linestyle, &quad.fillstylec);

            break;
            }

        case GR_DIAG_OBJTYPE_PIESECTOR:
            {
            GR_DIAG_OBJPIESECTOR pie;
            DRAW_POINT draw_point;
            DRAW_COORD draw_radius;

            memcpy32(&pie, pObject, sizeof32(pie));

            draw_point_from_gr_point(&draw_point, &pie.pos);
            draw_radius = gr_riscDraw_from_pixit(pie.radius);

            status = gr_riscdiag_piesector_new(p_gr_riscdiag, &objhdr.sys_off, &draw_point, draw_radius, pie.alpha, pie.beta, &pie.linestyle, &pie.fillstylec);

            break;
            }

        case GR_DIAG_OBJTYPE_PICTURE:
            {
            GR_DIAG_OBJPICTURE pict;
            DRAW_BOX draw_box;
            IMAGE_CACHE_HANDLE picture;
            ARRAY_HANDLE array_handle;

            memcpy32(&pict, pObject, sizeof32(pict));

            /* a representation of 'nothing' */

            draw_point_from_gr_point((P_DRAW_POINT) &draw_box.x0, &pict.pos);

            draw_box.x1 = draw_box.x0 + gr_riscDraw_from_pixit(pict.size.cx);
            draw_box.y1 = draw_box.y0 + gr_riscDraw_from_pixit(pict.size.cy);

            picture = pict.picture;

            array_handle = image_cache_loaded_ensure(picture);

            if(array_elements(&array_handle))
            {
                const U32 n_bytes = array_elements(&array_handle);
                status = gr_riscdiag_scaled_diagram_add(p_gr_riscdiag, &objhdr.sys_off, &draw_box, array_rangec(&array_handle, BYTE, 0, n_bytes), n_bytes, &pict.fillstyleb, &pict.fillstylec);
            }

            break;
            }

        default: default_unhandled();
            break;
        }

        status_break(status);

        /* poke diagram with offset of corresponding object in RISC OS diagram */
        memcpy32(pObject + offsetof32(GR_DIAG_OBJHDR, sys_off), &objhdr.sys_off, sizeof32(objhdr.sys_off));

        thisObject += objhdr.n_bytes;
    }

    return(status);
}

_Check_return_
static STATUS
gr_diag_ensure_riscdiag_font_tableR_entry(
    _InRef_     PC_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY p_f,
    _InoutRef_  P_ARRAY_HANDLE p_array_handleR)
{
    { /* check for existing entry */
    const ARRAY_INDEX n_elements = array_elements(p_array_handleR);
    ARRAY_INDEX i;
    PC_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY p_f_test = array_rangec(p_array_handleR, GR_RISCDIAG_RISCOS_FONTLIST_ENTRY, 0, n_elements);
    for(i = 0; i < n_elements; ++i, ++p_f_test)
    {
        if(0 == memcmp32(p_f_test, p_f, sizeof32(*p_f)))
            return(i + 1); /* already in table */
    }
    } /*block*/

    { /* add new entry at end */
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_f), FALSE);
    status_return(al_array_add(p_array_handleR, GR_RISCDIAG_RISCOS_FONTLIST_ENTRY, 1, &array_init_block, p_f));
    } /*block*/

    return(array_elements(p_array_handleR));
}

_Check_return_
static STATUS
gr_diag_ensure_riscdiag_font_tableR_entry_for_TEXT(
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle,
    _InoutRef_  P_ARRAY_HANDLE p_array_handleR)
{
    HOST_FONT_SPEC host_font_spec;
    STATUS status;

    status_return(status = gr_riscdiag_host_font_spec_riscos_from_textstyle(&host_font_spec, p_gr_textstyle));

    if(array_elements(&host_font_spec.h_host_name_tstr))
    {
        const PC_SBSTR szHostFontName = _sbstr_from_tstr(array_tstr(&host_font_spec.h_host_name_tstr));
        GR_RISCDIAG_RISCOS_FONTLIST_ENTRY f;

        zero_struct(f);
        xstrkpy(f.szHostFontName, sizeof32(f.szHostFontName), szHostFontName);

        host_font_spec_dispose(&host_font_spec);

        status = gr_diag_ensure_riscdiag_font_tableR_entry(&f, p_array_handleR);
    }

    return(status);
}

_Check_return_
static STATUS
gr_diag_ensure_riscdiag_font_tableR_entries_for_PICTURE(
    _In_reads_(diag_len) PC_BYTE p_diag,
    _InVal_     U32 diag_len,
    _InoutRef_  P_ARRAY_HANDLE p_array_handleR)
{
    STATUS status = STATUS_OK;
    GR_RISCDIAG gr_riscdiag;
    P_GR_RISCDIAG p_gr_riscdiag = &gr_riscdiag;

    /* scan the diagram for font tables - no need to set hglobal */
    gr_riscdiag_diagram_setup_from_data(&gr_riscdiag, p_diag, diag_len);

    gr_riscdiag_fontlist_scan(p_gr_riscdiag, DRAW_DIAG_OFFSET_FIRST, DRAW_DIAG_OFFSET_LAST);

    if(p_gr_riscdiag->dd_fontListR)
    {
        const DRAW_DIAG_OFFSET fontListR = p_gr_riscdiag->dd_fontListR;
        P_DRAW_OBJECT_FONTLIST pFontListObject = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, p_gr_riscdiag, fontListR);
        DRAW_DIAG_OFFSET nextObject = fontListR + pFontListObject->size;
        DRAW_DIAG_OFFSET thisOffset = fontListR + sizeof32(*pFontListObject);
        PC_DRAW_FONTLIST_ELEM pFontListElemR = gr_riscdiag_getoffptr(DRAW_FONTLIST_ELEM, p_gr_riscdiag, thisOffset);

        /* actual end of RISC OS font list object data may not be word aligned */
        while((nextObject - thisOffset) >= 4)
        {
            const DRAW_DIAG_OFFSET thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + strlen32p1(pFontListElemR->szHostFontName); /*CH_NULL*/
            GR_RISCDIAG_RISCOS_FONTLIST_ENTRY f;

            zero_struct(f);
            xstrkpy(f.szHostFontName, sizeof32(f.szHostFontName), pFontListElemR->szHostFontName);

            status_break(status = gr_diag_ensure_riscdiag_font_tableR_entry(&f, p_array_handleR));

            thisOffset += thislen;

            pFontListElemR = PtrAddBytes(PC_DRAW_FONTLIST_ELEM, pFontListElemR, thislen);
        }
    }

    return(status);
}

#if WINDOWS && 0

_Check_return_
static STATUS
gr_diag_ensure_riscdiag_font_tableW_entry(
    _InRef_     PC_DRAW_DS_WINDOWS_LOGFONT p_draw_ds_windows_logfont,
    _InoutRef_  P_ARRAY_HANDLE p_array_handleW)
{
    { /* check for existing entry */
    const ARRAY_INDEX n_elements = array_elements(p_array_handleW);
    ARRAY_INDEX i;
    PC_DRAW_DS_WINDOWS_LOGFONT p_draw_ds_windows_logfont_test = array_rangec(p_array_handleW, DRAW_DS_WINDOWS_LOGFONT, 0, n_elements);
    for(i = 0; i < n_elements; ++i, ++p_draw_ds_windows_logfont_test)
    {
        if(0 == draw_ds_windows_logfont_compare(p_draw_ds_windows_logfont_test, p_draw_ds_windows_logfont))
            return(i + 1); /* already in table */
    }
    } /*block*/

    { /* add new entry at end */
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_draw_ds_windows_logfont), FALSE);
    status_return(al_array_add(p_array_handleW, DRAW_DS_WINDOWS_LOGFONT, 1, &array_init_block, p_draw_ds_windows_logfont));
    } /*block*/

    return(array_elements(p_array_handleW));
}

_Check_return_
static STATUS
gr_diag_ensure_riscdiag_font_tableW_entry_for_TEXT(
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle,
    _InoutRef_  P_ARRAY_HANDLE p_array_handleW,
    _InVal_     STATUS expected_font_ref)
{
    HOST_FONT_SPEC host_font_spec;
    STATUS status = gr_riscdiag_host_font_spec_from_textstyle(&host_font_spec, p_gr_textstyle);

    if(status_ok(status && array_elements(&host_font_spec.h_host_name_tstr)))
    {
        const PC_SBSTR szHostFontName = _sbstr_from_tstr(array_tstr(&host_font_spec.h_host_name_tstr));
        DRAW_DS_WINDOWS_LOGFONT draw_ds_windows_logfont;

        draw_ds_windows_logfont_from_textstyle(&draw_ds_windows_logfont, p_gr_textstyle, szHostFontName);

        host_font_spec_dispose(&host_font_spec);

        status_return(status = gr_diag_ensure_riscdiag_font_tableW_entry(&draw_ds_windows_logfont, p_array_handleW));

#if CHECKING
        if(status_done(expected_font_ref))
            if(status != expected_font_ref)
                assert(status == expected_font_ref);
#endif
    }

    return(status);
}

_Check_return_
static STATUS
gr_diag_ensure_riscdiag_font_tableW_entries_for_PICTURE(
    _In_reads_(diag_len) PC_BYTE p_diag,
    _InVal_     U32 diag_len,
    _InoutRef_  P_ARRAY_HANDLE p_array_handleW,
    _InVal_     STATUS expected_font_ref)
{
    STATUS status = STATUS_OK;
    GR_RISCDIAG gr_riscdiag;
    P_GR_RISCDIAG p_gr_riscdiag = &gr_riscdiag;

    /* scan the diagram for font tables - no need to set hglobal */
    gr_riscdiag_diagram_setup_from_data(&gr_riscdiag, p_diag, diag_len);

    gr_riscdiag_fontlist_scan(p_gr_riscdiag, DRAW_DIAG_OFFSET_FIRST, DRAW_DIAG_OFFSET_LAST);

    if(p_gr_riscdiag->dd_fontListW)
    {
        const DRAW_DIAG_OFFSET fontListW = p_gr_riscdiag->dd_fontListW;
        P_DRAW_OBJECT_FONTLIST pFontListObject;
        DRAW_DIAG_OFFSET nextObject, thisOffset;

        pFontListObject = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, p_gr_riscdiag, fontListW);

        nextObject = fontListW + pFontListObject->size;
        thisOffset = fontListW + sizeof32(*pFontListObject);

        /* actual end of Windows font list object data may not be word aligned (in this case, paranoia) */
        while((nextObject - thisOffset) >= 4)
        {
            PC_DRAW_DS_WINFONTLIST_ELEM pFontListElemW = gr_riscdiag_getoffptr(DRAW_DS_WINFONTLIST_ELEM, p_gr_riscdiag, thisOffset);

            status_break(status = gr_diag_ensure_riscdiag_font_tableW_entry(&pFontListElemW->draw_ds_windows_logfont, p_array_handleW));

            thisOffset += sizeof32(*pFontListElemW);
        }

        status_return(status);
    }

#if CHECKING
    if(status_ok(status) && status_done(expected_font_ref))
        if(status != expected_font_ref)
            assert(status == expected_font_ref);
#endif

    return(status);
}

#endif /* WINDOWS */

_Check_return_
static STATUS
gr_diag_ensure_riscdiag_font_table_entry_for_TEXT(
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle,
    _InoutRef_  P_ARRAY_HANDLE p_array_handleR,
    _InoutRef_  P_ARRAY_HANDLE p_array_handleW)
{
    STATUS status;

    status = gr_diag_ensure_riscdiag_font_tableR_entry_for_TEXT(p_gr_textstyle, p_array_handleR);

#if WINDOWS && 0
    if(status_ok(status))
        status = gr_diag_ensure_riscdiag_font_tableW_entry_for_TEXT(p_gr_textstyle, p_array_handleW, status);
#else
    UNREFERENCED_PARAMETER_InoutRef_(p_array_handleW);
#endif

    return(status);
}

_Check_return_
static STATUS
gr_diag_ensure_riscdiag_font_table_entries_for_PICTURE(
    _In_reads_(diag_len) PC_BYTE p_diag,
    _InVal_     U32 diag_len,
    _InoutRef_  P_ARRAY_HANDLE p_array_handleR,
    _InoutRef_  P_ARRAY_HANDLE p_array_handleW)
{
    STATUS status;

    status = gr_diag_ensure_riscdiag_font_tableR_entries_for_PICTURE(p_diag, diag_len, p_array_handleR);

#if WINDOWS && 0
    if(status_ok(status))
        status = gr_diag_ensure_riscdiag_font_tableW_entries_for_PICTURE(p_diag, diag_len, p_array_handleW, status);
#else
    UNREFERENCED_PARAMETER_InoutRef_(p_array_handleW);
#endif

    return(status);
}

_Check_return_
static STATUS
gr_diag_create_riscdiag_font_tables_between(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    _OutRef_    P_ARRAY_HANDLE p_array_handleR,
    _OutRef_    P_ARRAY_HANDLE p_array_handleW)
{
    GR_DIAG_OFFSET thisObject = sttObject_in;
    GR_DIAG_OFFSET endObject = endObject_in;
    STATUS status = STATUS_OK;
    P_BYTE pObject;

    *p_array_handleR = 0;
    *p_array_handleW = 0;

    myassert2x(p_gr_diag && p_gr_diag->handle, TEXT("gr_diag_create_riscdiag_between has no diagram ") PTR_XTFMT TEXT("->") U32_TFMT, p_gr_diag, p_gr_diag ? p_gr_diag->handle : 0);

    /* first add font table entries for all text objects */
    if(gr_diag_object_first(p_gr_diag, &thisObject, &endObject, &pObject))
    {
        do  {
            switch(*DIAG_OBJHDR(GR_DIAG_OBJTYPE, pObject, type))
            {
            case GR_DIAG_OBJTYPE_TEXT:
                {
                GR_DIAG_OBJTEXT text;

                memcpy32(&text, pObject, sizeof32(text));

                if(status_ok(status))
                    status = gr_diag_ensure_riscdiag_font_table_entry_for_TEXT(&text.textstyle, p_array_handleR, p_array_handleW);

                break;
                }

            default:
                break;
            }

            status_break(status);
        }
        while(gr_diag_object_next(p_gr_diag, &thisObject, endObject, &pObject));
    }

    status_return(status);

    /* do pictures in a separate pass for debugging */
    thisObject = sttObject_in;
    endObject = endObject_in;

    if(gr_diag_object_first(p_gr_diag, &thisObject, &endObject, &pObject))
    {
        do  {
            switch(*DIAG_OBJHDR(GR_DIAG_OBJTYPE, pObject, type))
            {
            case GR_DIAG_OBJTYPE_PICTURE:
                {
                GR_DIAG_OBJPICTURE pict;
                IMAGE_CACHE_HANDLE picture;
                ARRAY_HANDLE array_handle;

                memcpy32(&pict, pObject, sizeof32(pict));

                picture = pict.picture;

                array_handle = image_cache_loaded_ensure(picture);

                if(array_elements(&array_handle))
                {
                    const U32 n_bytes = array_elements(&array_handle);
                    status = gr_diag_ensure_riscdiag_font_table_entries_for_PICTURE(array_rangec(&array_handle, BYTE, 0, n_bytes), n_bytes, p_array_handleR, p_array_handleW);
                }

                break;
                }

            default:
                break;
            }

            status_break(status);
        }
        while(gr_diag_object_next(p_gr_diag, &thisObject, endObject, &pObject));
    }

    return(status);
}

/******************************************************************************
*
* dispose of a diagram
*
******************************************************************************/

extern void
gr_diag_diagram_dispose(
    _InoutRef_ /*never NULL*/ P_P_GR_DIAG p_p_gr_diag)
{
    P_GR_DIAG p_gr_diag;

    PTR_ASSERT(p_p_gr_diag);

    if(NULL != (p_gr_diag = *p_p_gr_diag))
    {
        /* remove system-dependent representation too */
        gr_riscdiag_diagram_dispose(&p_gr_diag->p_gr_riscdiag);

        al_array_dispose(&p_gr_diag->handle);

        al_ptr_dispose(P_P_ANY_PEDANTIC(p_p_gr_diag));
    }
}

/******************************************************************************
*
* end adding data to a diagram
*
******************************************************************************/

extern GR_DIAG_OFFSET
gr_diag_diagram_end(
    _InoutRef_  P_GR_DIAG p_gr_diag)
{
    GR_DIAG_PROCESS_T process;

    myassert2x(p_gr_diag && p_gr_diag->handle, TEXT("gr_diag_diagram_end has no diagram ") PTR_XTFMT TEXT("->") U32_TFMT, p_gr_diag, p_gr_diag ? p_gr_diag->handle : 0);

    /* kill old one */
    gr_riscdiag_diagram_dispose(&p_gr_diag->p_gr_riscdiag);

    /* build new system-dependent representation */
    status_assert(gr_diag_create_riscdiag(p_gr_diag));

    * (int *) &process = 0;
    process.recurse = 1;
    process.recompute = 1;
    process.severe_recompute = 0;
    gr_diag_diagram_reset_bbox(p_gr_diag, process);

    return(array_elements32(&p_gr_diag->handle));
}

/******************************************************************************
*
* start a diagram: allocate descriptor and initial chunk & initialise
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_GR_DIAG
gr_diag_diagram_new(
    _In_z_      PCTSTR szCreatorName,
    _OutRef_    P_STATUS p_status)
{
    P_GR_DIAG p_gr_diag = al_ptr_calloc_elem(GR_DIAG, 1, p_status);

    if(NULL != p_gr_diag)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(512, sizeof32(U8), 1 /*zero allocation*/);
        P_GR_DIAG_DIAGHEADER pDiagHdr;
        const U32 n_bytes = sizeof32(*pDiagHdr);

        if(NULL != (pDiagHdr = (P_GR_DIAG_DIAGHEADER) al_array_alloc_BYTE(&p_gr_diag->handle, n_bytes, &array_init_block, p_status)))
        {
            tstr_xstrkpy(pDiagHdr->szCreatorName, elemof32(pDiagHdr->szCreatorName), szCreatorName);
            gr_box_make_bad(&pDiagHdr->bbox);
        }
        else
        {
            al_ptr_dispose(P_P_ANY_PEDANTIC(&p_gr_diag));
        }
    }

    return(p_gr_diag);
}

/******************************************************************************
*
* reset a diagram's bbox
*
******************************************************************************/

extern void
gr_diag_diagram_reset_bbox(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_PROCESS_T process)
{
    P_GR_DIAG_DIAGHEADER pDiagHdr;
    GR_BOX diag_box;

    /* reset_bbox needs info from riscdiag */
    if(NULL != gr_riscdiag_diagram_lock(p_gr_diag->p_gr_riscdiag))
    {
        gr_diag_object_reset_bbox_between(p_gr_diag, &diag_box, GR_DIAG_OBJECT_FIRST, GR_DIAG_OBJECT_LAST, process);

        gr_riscdiag_diagram_unlock(p_gr_diag->p_gr_riscdiag);
    }
    else
        gr_box_make_bad(&diag_box);

    pDiagHdr = array_base(&p_gr_diag->handle, GR_DIAG_DIAGHEADER);

    memcpy32(&pDiagHdr->bbox, &diag_box, sizeof32(diag_box));
}

/******************************************************************************
*
* return amount of space to be allocated for the header of this object
*
******************************************************************************/

_Check_return_
extern U32
gr_diag_object_base_size(
    _InVal_     GR_DIAG_OBJTYPE objectType)
{
    switch(objectType)
    {
    case GR_DIAG_OBJTYPE_GROUP:
        return(sizeof32(GR_DIAG_OBJGROUP));

    case GR_DIAG_OBJTYPE_TEXT:
        return(sizeof32(GR_DIAG_OBJTEXT));

    case GR_DIAG_OBJTYPE_LINE:
        return(sizeof32(GR_DIAG_OBJLINE));

    case GR_DIAG_OBJTYPE_RECTANGLE:
        return(sizeof32(GR_DIAG_OBJRECTANGLE));

    case GR_DIAG_OBJTYPE_PIESECTOR:
        return(sizeof32(GR_DIAG_OBJPIESECTOR));

    case GR_DIAG_OBJTYPE_QUADRILATERAL:
        return(sizeof32(GR_DIAG_OBJQUADRILATERAL));

    case GR_DIAG_OBJTYPE_PICTURE:
        return(sizeof32(GR_DIAG_OBJPICTURE));

    default:
        myassert1(TEXT("gr_diag_object_base_size of objectType ") U32_TFMT, objectType);
        return(sizeof32(GR_DIAG_OBJHDR));
    }
}

/******************************************************************************
*
* run backwards over range of objects looking for top object that clips
*
* Hierarchy does help here too!
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_object_correlate_between(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size,
    _Inout_updates_(recursion_limit) P_GR_DIAG_OFFSET pHitObject /*[]out*/,
    _InVal_     U32 recursion_limit,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in)
{
    const GR_DIAG_OFFSET sttObject = gr_diag_normalise_stt(p_gr_diag, sttObject_in);
    const GR_DIAG_OFFSET endObject = gr_diag_normalise_end(p_gr_diag, endObject_in);
    GR_DIAG_OFFSET thisObject;
    BOOL hit;

    /* always say nothing hit at this level so caller can scan hitObject list and find a terminator */
    pHitObject[0] = GR_DIAG_OBJECT_NONE;

    myassert2x(p_gr_diag && p_gr_diag->handle, TEXT("gr_diag_object_correlate_between has no diagram ") PTR_XTFMT TEXT("->") U32_TFMT, p_gr_diag, p_gr_diag ? p_gr_diag->handle : 0);

    /* start with current object off end of range */
    thisObject = endObject;

    /* whenever current object becomes same as start object we've missed */
    while(thisObject > sttObject)
    {
        GR_DIAG_OBJHDR objhdr;

        { /* block to reduce recursion stack overhead */
        /* find next object to process: scan up from start till we meet current object */
        GR_DIAG_OFFSET findObject = sttObject;
        PC_BYTE pObject = array_ptrc(&p_gr_diag->handle, BYTE, findObject);

        for(;;)
        {
            memcpy32(&objhdr, pObject, sizeof32(objhdr));

            myassert2x(objhdr.n_bytes >= sizeof32(objhdr),
                       TEXT("gr_diag_object_correlate_between object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" > sizeof-objhdr"), findObject, objhdr.n_bytes);
            myassert3x(findObject + objhdr.n_bytes <= endObject,
                       TEXT("gr_diag_object_correlate_between object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" goes beyond end ") U32_XTFMT, findObject, objhdr.n_bytes, endObject);

            if(findObject + objhdr.n_bytes >= thisObject)
            {
                /* leave lock on pObject.hdr for bbox scan - loop ALWAYS terminates through here */
                /* also moves thisObject down */
                thisObject = findObject;
                break;
            }

            findObject += objhdr.n_bytes;
            pObject += objhdr.n_bytes;
        }

        /* refine hit where this is easy enough to do */
        switch(objhdr.type)
        {
        case GR_DIAG_OBJTYPE_LINE:
            hit = gr_diag_line_hit(p_gr_diag, findObject, point, size);
            break;

        case GR_DIAG_OBJTYPE_PIESECTOR:
            hit = gr_diag_piesector_hit(p_gr_diag, findObject, point, size);
            break;

        case GR_DIAG_OBJTYPE_QUADRILATERAL:
            hit = gr_diag_quadrilateral_hit(p_gr_diag, findObject, point, size);
            break;

        default:
            /* simple test to see whether we hit this object */
            hit = gr_box_hit(&objhdr.bbox, point, size);
            break;
        }

        if(!hit)
            continue; /* loop for next object */
        } /*block*/

        pHitObject[0] = thisObject;
        pHitObject[1] = GR_DIAG_OBJECT_NONE; /* keep terminated */

        /* did we hit hierarchy that needs searching at a finer level
         * and that we are allowed to search into?
        */
        if((objhdr.type == GR_DIAG_OBJTYPE_GROUP)  &&  (recursion_limit != 0))
        {
            /* find a new version of hit as we may have hit a group
             * at this level but miss all its components at the next
             * so groups are not found as leaves when recursing
             *
             * SKS after PD 4.12 12feb92 - must limit search to the part of this group spanned by stt...end
            */
            hit = gr_diag_object_correlate_between(p_gr_diag, point, size,
                                                   pHitObject + 1, /* will be poked at a finer level */
                                                   recursion_limit - 1,
                                                   thisObject + sizeof32(GR_DIAG_OBJGROUP),
                                                   MIN(thisObject + objhdr.n_bytes, endObject));

            if(!hit)
            {   /* kill group hit, keep terminated */
                pHitObject[0] = GR_DIAG_OBJECT_NONE;
                continue;
            }
        }

        return(hit);
    }

    return(FALSE);
}

/******************************************************************************
*
* end an object: go back and patch its size field
*
******************************************************************************/

extern U32
gr_diag_object_end(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InRef_     PC_GR_DIAG_OFFSET pObjectStart)
{
    P_BYTE pObject;
    U32 n_bytes;

    myassert2x(p_gr_diag && p_gr_diag->handle, TEXT("gr_diag_object_end has no diagram ") PTR_XTFMT TEXT("->") U32_TFMT, p_gr_diag, p_gr_diag ? p_gr_diag->handle : 0);

    pObject = array_ptr(&p_gr_diag->handle, BYTE, *pObjectStart);

    n_bytes = array_elements32(&p_gr_diag->handle) - *pObjectStart;

    if(n_bytes == gr_diag_object_base_size(*DIAG_OBJHDR(GR_DIAG_OBJTYPE, pObject, type)))
    {
        /* destroy object and contents if nothing in it */
        al_array_shrink_by(&p_gr_diag->handle, - (S32) n_bytes);
        trace_3(0 /*TRACE_OUT | TRACE_ANY*/, TEXT("object_end: destroying type ") U32_TFMT TEXT(" n_bytes ") U32_XTFMT TEXT(" at ") U32_XTFMT, * DIAG_OBJHDR(GR_DIAG_OBJTYPE, pObject, type), n_bytes, *pObjectStart);
        n_bytes = 0;
    }
    else
        /* update object size */
        *DIAG_OBJHDR(U32, pObject, n_bytes) = n_bytes;

    return(n_bytes);
}

/******************************************************************************
*
* diagram scanning: loop over objects
*
******************************************************************************/

_Check_return_
extern BOOL
gr_diag_object_first(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InoutRef_  P_GR_DIAG_OFFSET pSttObject,
    _InoutRef_  P_GR_DIAG_OFFSET pEndObject,
    _OutRef_    P_P_BYTE ppObject)
{
    P_BYTE pObject;

    *pSttObject = gr_diag_normalise_stt(p_gr_diag, *pSttObject);
    *pEndObject = gr_diag_normalise_end(p_gr_diag, *pEndObject);

    pObject = array_ptr(&p_gr_diag->handle, BYTE, *pSttObject);

#if CHECKING
    {
    const U32 n_bytes = *DIAG_OBJHDR(U32, pObject, n_bytes); /* even if group */
    myassert2x(n_bytes >= sizeof32(GR_DIAG_OBJHDR),
              TEXT("gr_diag_object_first object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" < sizeof-GR_DIAG_OBJHDR"), *pSttObject, n_bytes);
    myassert3x(*pSttObject + n_bytes <= *pEndObject,
              TEXT("gr_diag_object_first object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" goes beyond end ") U32_XTFMT, *pSttObject, n_bytes, *pEndObject);
    } /*block*/
#endif /* CHECKING */

    if(*pSttObject >= *pEndObject)
    {
        *pSttObject = GR_DIAG_OBJECT_FIRST;
        pObject = NULL;
    }

    /* maintain current lock as initial, now caller's responsibility */
    *ppObject = pObject;

    return(*pSttObject != GR_DIAG_OBJECT_FIRST); /* note above repoke^^^ on end */
}

/******************************************************************************
*
* start an object: init size, leave bbox bad, to be patched later
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_object_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _OutRef_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _Out_opt_   P_P_BYTE ppObject,
    P_ANY p_gr_diag_objhdr /*size poked*/,
    _InVal_     U32 extraBytes)
{
    P_U8 pObject_in = (P_U8) p_gr_diag_objhdr;
    U32 baseBytes = gr_diag_object_base_size(*DIAG_OBJHDR(GR_DIAG_OBJTYPE, pObject_in, type));
    U32 allocBytes = baseBytes + extraBytes;
    STATUS status = STATUS_OK;
    P_BYTE pObject;

    *DIAG_OBJHDR(U32, pObject_in, n_bytes) = allocBytes;
    gr_box_make_bad(PtrAddBytes(P_GR_BOX, pObject_in, offsetof32(GR_DIAG_OBJHDR, bbox))); /* everything has bbox */

    myassert2x(p_gr_diag && p_gr_diag->handle, TEXT("gr_diag_object_new has no diagram ") PTR_XTFMT TEXT("->") U32_TFMT, p_gr_diag, p_gr_diag ? p_gr_diag->handle : 0);

    if(NULL != pObjectStart)
        *pObjectStart = array_elements32(&p_gr_diag->handle);

    if(NULL != (pObject = al_array_extend_by_BYTE(&p_gr_diag->handle, allocBytes, PC_ARRAY_INIT_BLOCK_NONE, &status)))
    {
        memcpy32(pObject, pObject_in, baseBytes);
        trace_4(0 /*TRACE_OUT | TRACE_ANY*/, TEXT("object_new(type ") S32_TFMT TEXT(" baseBytes ") U32_XTFMT TEXT(" allocBytes ") U32_XTFMT TEXT(") at ") U32_XTFMT,* (P_S32) pObject, baseBytes, allocBytes, array_elements(&p_gr_diag->handle) - allocBytes);
    }

    if(NULL != ppObject)
        *ppObject = pObject;

    return(status);
}

/******************************************************************************
*
* diagram scanning: loop over objects
*
******************************************************************************/

_Check_return_
extern BOOL
gr_diag_object_next(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InoutRef_  P_GR_DIAG_OFFSET pSttObject,
    _InVal_     GR_DIAG_OFFSET endObject,
    _OutRef_    P_P_BYTE ppObject)
{
    P_BYTE pObject;
    U32 n_bytes;

    assert(array_index_is_valid(&p_gr_diag->handle, *pSttObject));
    pObject = array_ptr(&p_gr_diag->handle, BYTE, *pSttObject);
    n_bytes = *DIAG_OBJHDR(U32, pObject, n_bytes);

    myassert2x(n_bytes >= sizeof32(GR_DIAG_OBJHDR),
               TEXT("gr_diag_object_next object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" < sizeof-GR_DIAG_OBJHDR"), *pSttObject, n_bytes);
    myassert3x(*pSttObject + n_bytes <= endObject,
               TEXT("gr_diag_object_next object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" goes beyond end ") U32_XTFMT, *pSttObject, n_bytes, endObject);

    /* force gr_diag scans to be linear, not recursive */
    if(*DIAG_OBJHDR(GR_DIAG_OBJTYPE, pObject, type) == GR_DIAG_OBJTYPE_GROUP)
        n_bytes = sizeof32(GR_DIAG_OBJGROUP);

    *pSttObject += n_bytes;

    if(*pSttObject >= endObject)
    {
        *pSttObject = GR_DIAG_OBJECT_FIRST;
        pObject = NULL;
    }

    *ppObject = pObject;

    return(*pSttObject != GR_DIAG_OBJECT_FIRST); /* note above repoke^^^ on end */
}

/******************************************************************************
*
* reset a bbox over a range of objects in the diagram
*
******************************************************************************/

extern void
gr_diag_object_reset_bbox_between(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    /*out*/ P_GR_BOX pBox,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    _InVal_     GR_DIAG_PROCESS_T process)
{
    GR_DIAG_OFFSET thisObject = gr_diag_normalise_stt(p_gr_diag, sttObject_in);
    const GR_DIAG_OFFSET endObject = gr_diag_normalise_end(p_gr_diag, endObject_in);
    P_BYTE pObject;

    myassert2x(p_gr_diag && p_gr_diag->handle, TEXT("gr_diag_object_reset_bbox_between has no diagram ") PTR_XTFMT TEXT("->") U32_TFMT, p_gr_diag, p_gr_diag ? p_gr_diag->handle : 0);

    /* loop over constituent objects and extract bbox */

    gr_box_make_bad(pBox);

    /* keep pObject valid through loop, dropping and reloading where necessary */
    pObject = array_ptr(&p_gr_diag->handle, BYTE, thisObject);

    while(thisObject < endObject)
    {
        GR_DIAG_OBJHDR objhdr;

        memcpy32(&objhdr, pObject, sizeof32(objhdr));

        myassert2x(objhdr.n_bytes >= sizeof32(objhdr),
                  TEXT("gr_diag_object_reset_bbox_between object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" < sizeof-objhdr"), thisObject, objhdr.n_bytes);
        myassert3x(thisObject + objhdr.n_bytes <= endObject,
                  TEXT("gr_diag_object_reset_bbox_between object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" goes beyond end ") U32_XTFMT, thisObject, objhdr.n_bytes, endObject);

        /* fix up hierarchy recursively */
        if(objhdr.type == GR_DIAG_OBJTYPE_GROUP)
        {
            if(process.recurse)
            {
                gr_diag_object_reset_bbox_between(p_gr_diag, &objhdr.bbox, thisObject + sizeof32(GR_DIAG_OBJGROUP), thisObject + objhdr.n_bytes, process);

                if(process.recompute)
                    pObject = array_ptr(&p_gr_diag->handle, BYTE, thisObject);

                memcpy32(pObject + offsetof32(GR_DIAG_OBJHDR, bbox), &objhdr.bbox, sizeof32(objhdr.bbox));
            }
        }
        else if(process.recompute)
        {
#if 0
            if(process.severe_recompute)
            {
                /* get corresponding object recomputed */

                /* probably no need for this yet, but ... */
                /* pObject.p_any = array_ptr(&p_gr_diag->handle, BYTE, thisObject); */
            }
#endif

            if((NULL != p_gr_diag->p_gr_riscdiag) && p_gr_diag->p_gr_riscdiag->draw_diag.length)
            {
                /* merely copy over bbox info from corresponding object in Draw file */
                const PC_BYTE pObjectR = gr_riscdiag_getoffptr(BYTE, p_gr_diag->p_gr_riscdiag, objhdr.sys_off);
                DRAW_OBJECT_HEADER objhdrR;
                S32 div_x, div_y, mul_x, mul_y;

                assert((objhdr.sys_off >= sizeof32(DRAW_FILE_HEADER)) && (objhdr.sys_off + sizeof32(DRAW_OBJECT_HEADER) <= p_gr_diag->p_gr_riscdiag->draw_diag.length));

                memcpy32(&objhdrR, pObjectR, sizeof32(objhdrR));

                /* pixits from riscDraw ain't exact so take care - even go via pixels (on RISC OS we can use screen, for Windows assume 90x90) */
                div_x = GR_RISCDRAW_PER_RISCOS;
                div_y = GR_RISCDRAW_PER_RISCOS;
                mul_x = GR_PIXITS_PER_RISCOS;
                mul_y = GR_PIXITS_PER_RISCOS;
#if RISCOS
                div_x <<= host_modevar_cache_current.XEigFactor;
                div_y <<= host_modevar_cache_current.YEigFactor;
                mul_x <<= host_modevar_cache_current.XEigFactor;
                mul_y <<= host_modevar_cache_current.YEigFactor;
#else
                div_x *= 2;
                div_y *= 2;
                mul_x *= 2;
                mul_y *= 2;
#endif

                objhdr.bbox.x0 = gr_round_pixit_to_floor(objhdrR.bbox.x0, div_x) * mul_x;
                objhdr.bbox.y0 = gr_round_pixit_to_floor(objhdrR.bbox.y0, div_y) * mul_y;
                objhdr.bbox.x1 = gr_round_pixit_to_ceil( objhdrR.bbox.x1, div_x) * mul_x;
                objhdr.bbox.y1 = gr_round_pixit_to_ceil( objhdrR.bbox.y1, div_y) * mul_y;
            }
            else
                gr_box_make_bad(&objhdr.bbox);

            memcpy32(pObject + offsetof32(GR_DIAG_OBJHDR, bbox), &objhdr.bbox, sizeof32(objhdr.bbox));
        }

        gr_box_union(pBox, pBox, &objhdr.bbox);

        thisObject += objhdr.n_bytes;
        pObject += objhdr.n_bytes;
    }
}

/******************************************************************************
*
* scan a range of objects in the diagram for the given id
*
******************************************************************************/

extern GR_DIAG_OFFSET
gr_diag_object_search_between(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InVal_     GR_DIAG_OFFSET sttObject_in,
    _InVal_     GR_DIAG_OFFSET endObject_in,
    _InVal_     GR_DIAG_PROCESS_T process)
{
    GR_DIAG_OFFSET thisObject = gr_diag_normalise_stt(p_gr_diag, sttObject_in);
    const GR_DIAG_OFFSET endObject = gr_diag_normalise_end(p_gr_diag, endObject_in);
    P_BYTE pObject;
    GR_DIAG_OFFSET hitObject = GR_DIAG_OBJECT_NONE;

    myassert2x(p_gr_diag && p_gr_diag->handle, TEXT("gr_diag_object_search_between has no diagram ") PTR_XTFMT TEXT("->") U32_TFMT, p_gr_diag, p_gr_diag ? p_gr_diag->handle : 0);

    /* loop over constituent objects */

    /* keep pObject valid through loop, dropping and reloading where necessary */
    pObject = array_ptr(&p_gr_diag->handle, BYTE, thisObject);

    while(thisObject < endObject)
    {
        GR_DIAG_OBJHDR objhdr;

        memcpy32(&objhdr, pObject, sizeof32(objhdr));

        myassert2x(objhdr.n_bytes >= sizeof32(objhdr),
                   TEXT("gr_diag_object_search_between object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" < sizeof-objhdr"), thisObject, objhdr.n_bytes);
        myassert3x(thisObject + objhdr.n_bytes <= endObject,
                   TEXT("gr_diag_object_search_between object ") U32_XTFMT TEXT(" size ") U32_XTFMT TEXT(" goes beyond end") U32_XTFMT, thisObject, objhdr.n_bytes, endObject);

        if(gr_chart_objid_equal(&objid, &objhdr.objid))
        {
            hitObject = thisObject;
            break;
        }

        if(process.find_children)
        {
            GR_DIAG_OBJID_T id = objhdr.objid;

            consume_bool(gr_chart_objid_find_parent_for_search(&id));

            if(gr_chart_objid_equal(&objid, &id))
            {
                hitObject = thisObject;
                break;
            }
        }

        if(objhdr.type == GR_DIAG_OBJTYPE_GROUP)
            if(process.recurse)
            {
                /* search hierarchy recursively */
                hitObject = gr_diag_object_search_between(p_gr_diag, objid, thisObject + sizeof32(GR_DIAG_OBJGROUP), thisObject + objhdr.n_bytes, process);

                if(hitObject != GR_DIAG_OBJECT_NONE)
                    break;
            }

        thisObject += objhdr.n_bytes;
        pObject += objhdr.n_bytes;
    }

    return(hitObject);
}

/******************************************************************************
*
* end a group: go back and patch its size field (leave bbox till diag end)
*
******************************************************************************/

extern U32
gr_diag_group_end(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InRef_     PC_GR_DIAG_OFFSET pGroupStart)
{
    return(gr_diag_object_end(p_gr_diag, pGroupStart));
}

/******************************************************************************
*
* start a group: leave size & bbox empty to be patched later
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_group_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pGroupStart,
    _InVal_     GR_DIAG_OBJID_T objid)
{
    GR_DIAG_OBJGROUP group;

    group.type = GR_DIAG_OBJTYPE_GROUP;
    group.objid = objid;

    return(gr_diag_object_new(p_gr_diag, pGroupStart, NULL, &group, 0));
}

/******************************************************************************
*
* add a line as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_line_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InRef_     PC_GR_POINT pOffset,
    _InRef_     PC_GR_LINESTYLE linestyle)
{
    GR_DIAG_OBJLINE line;

    line.type = GR_DIAG_OBJTYPE_LINE;
    line.objid = objid;

    line.pos = *pPos;

    line.offset = *pOffset;

    line.linestyle = *linestyle;

    return(gr_diag_object_new(p_gr_diag, pObjectStart, NULL, &line, 0));
}

/* NB this refining test presumes the bounding box test has passed */

_Check_return_
static BOOL
line_hit(
    _InRef_     PC_GR_POINT line_p1,
    _InRef_     PC_GR_POINT line_p2,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    const GR_COORD Tx = point->x;
    const GR_COORD Ty = point->y;
    const GR_COORD Ex = size->cx;
    const GR_COORD Ey = size->cy;
    /* x3 = Tx + Ex; x4 = Tx - Ex; */
    /* y3 = Ty + Ey; y4 = Ty - Ey; */
    const GR_COORD x1 = line_p1->x;
    const GR_COORD y1 = line_p1->y;
    const GR_COORD x2 = line_p2->x;
    const GR_COORD y2 = line_p2->y;
    const GR_COORD dx = x1 - x2;
    const GR_COORD dy = y1 - y2;
    GR_COORD Px, Py;

    /* where does the line (x1,y1) (x2,y2) pass through the horizontal line defined by Ty? */
    if(0 == dy)
    {   /* this line is horizontal */
        Py = y1;

        if(Py < (Ty - Ey))
            return(FALSE);
        if(Py > (Ty + Ey))
            return(FALSE);
    }
    else
    {
        Px = ((Ty * dx) - (x1 * y2 - y1 * x2)) / dy;

        if(Px < (Tx - Ex))
            return(FALSE);
        if(Px > (Tx + Ex))
            return(FALSE);
    }

    /* where does the line (x1,y1) (x2,y2) pass through the vertical line defined by Tx? */
    if(0 == dx)
    {   /* this line is vertical */
        /* if dx == dy == 0 it is a point, but these combined tests effectively do point-in-box test */
        Px = x1;

        if(Px < (Tx - Ex))
            return(FALSE);
        if(Px > (Tx + Ex))
            return(FALSE);
    }
    else
    {
        Py = ((Tx * dy) + (x1 * y2 - y1 * x2)) / dx;

        if(Py < (Ty - Ey))
            return(FALSE);
        if(Py > (Ty + Ey))
            return(FALSE);
    }

    return(TRUE);
}

_Check_return_
static BOOL
gr_diag_line_hit_refine(
    const GR_DIAG_OBJLINE * const line,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    GR_POINT line_p1;
    GR_POINT line_p2 = line->pos;

    line_p1.x = line->pos.x + line->offset.x;
    line_p1.y = line->pos.y + line->offset.y;

    return(line_hit(&line_p1, &line_p2, point, size));
}

_Check_return_
static BOOL
gr_diag_line_hit(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET testObject,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    const PC_BYTE pObject = array_ptrc(&p_gr_diag->handle, BYTE, testObject);
    const GR_DIAG_OBJLINE * const line = (const GR_DIAG_OBJLINE *) pObject;

    /* simple test to see whether we hit this object */
    if(!gr_box_hit(&line->bbox, point, size))
        return(FALSE);

    return(gr_diag_line_hit_refine(line, point, size));
}

/******************************************************************************
*
* add a parallelogram as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_parallelogram_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InRef_     PC_GR_POINT pOffset1,
    _InRef_     PC_GR_POINT pOffset2,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec)
{
    GR_DIAG_OBJQUADRILATERAL quad;

    quad.type = GR_DIAG_OBJTYPE_QUADRILATERAL;
    quad.objid = objid;

    quad.pos = *pPos;

    quad.offset1 = *pOffset1;
    quad.offset2 = *pOffset2;

    quad.offset3.x = pOffset2->x - pOffset1->x;
    quad.offset3.y = pOffset2->y - pOffset1->y;

    quad.linestyle = *linestyle;
    quad.fillstylec = *fillstylec;

    return(gr_diag_object_new(p_gr_diag, pObjectStart, NULL, &quad, 0));
}

/******************************************************************************
*
* add a piesector as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_piesector_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InVal_     GR_PIXIT radius,
    _InVal_     F64 alpha,
    _InVal_     F64 beta,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec)
{
    GR_DIAG_OBJPIESECTOR pie;

    pie.type = GR_DIAG_OBJTYPE_PIESECTOR;
    pie.objid = objid;

    pie.pos = *pPos;

    pie.radius = radius;
    pie.alpha = alpha;
    pie.beta = beta;

    pie.linestyle = *linestyle;
    pie.fillstylec = *fillstylec;

    return(gr_diag_object_new(p_gr_diag, pObjectStart, NULL, &pie, 0));
}

_Check_return_
static BOOL
piesector_hit(
    _InRef_     PC_GR_POINT pPos,
    _InVal_     GR_PIXIT radius,
    _InVal_     F64 alpha,
    _InVal_     F64 beta,
    _InRef_     PC_GR_POINT point)
{
    const GR_PIXIT dx = point->x - pPos->x;
    const GR_PIXIT dy = point->y - pPos->y;

    { /* trivial no-trig check - is point within the bounding circle? */
    const S32 dx_2_plus_dy_2 = (dx * dx) + (dy * dy);
    const S32 radius_squared = radius * radius;
    if(dx_2_plus_dy_2 > radius_squared)
        return(FALSE);
    } /*block*/

    {
    F64 theta = atan2(dy, dx);
    assert(alpha <= beta);

    /*reportf(TEXT("piesector_hit 1: test %g < %g < %g"), alpha, theta, beta);*/
    if( (theta >= alpha) && (theta <= beta) )
        return(TRUE);

    if(theta < 0.0)
    {
        theta += _two_pi;
    }
    else
    {
        theta -= _two_pi;
    }

    /*reportf(TEXT("piesector_hit 2: test %g < %g < %g"), alpha, theta, beta);*/
    if( (theta >= alpha) && (theta <= beta) )
        return(TRUE);
    } /*block*/

    return(FALSE);
}

_Check_return_
static BOOL
gr_diag_piesector_hit_refine(
    const GR_DIAG_OBJPIESECTOR * const pie,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    UNREFERENCED_PARAMETER_InRef_(size);

    return(piesector_hit(&pie->pos, pie->radius, pie->alpha, pie->beta, point));
}

_Check_return_
static BOOL
gr_diag_piesector_hit(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET testObject,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    const PC_BYTE pObject = array_ptrc(&p_gr_diag->handle, BYTE, testObject);
    const GR_DIAG_OBJPIESECTOR * const pie = (const GR_DIAG_OBJPIESECTOR *) pObject;

    /* simple test to see whether we hit this object */
    if(!gr_box_hit(&pie->bbox, point, size))
        return(FALSE);

    return(gr_diag_piesector_hit_refine(pie, point, size));
}

/******************************************************************************
*
* add a quadrilateral as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_quadrilateral_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InRef_     PC_GR_POINT pOffset1,
    _InRef_     PC_GR_POINT pOffset2,
    _InRef_     PC_GR_POINT pOffset3,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec)
{
    GR_DIAG_OBJQUADRILATERAL quad;

    quad.type = GR_DIAG_OBJTYPE_QUADRILATERAL;
    quad.objid = objid;

    quad.pos = *pPos;

    quad.offset1 = *pOffset1;
    quad.offset2 = *pOffset2;
    quad.offset3 = *pOffset3;

    quad.linestyle = *linestyle;
    quad.fillstylec = *fillstylec;

    return(gr_diag_object_new(p_gr_diag, pObjectStart, NULL, &quad, 0));
}

_Check_return_
static BOOL
gr_diag_quadrilateral_hit_refine(
    const GR_DIAG_OBJQUADRILATERAL * const quad,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    GR_POINT points[4 + 1];

    UNREFERENCED_PARAMETER_InRef_(size);

    points[0] = quad->pos;

    points[1].x = quad->pos.x + quad->offset1.x;
    points[1].y = quad->pos.y + quad->offset1.y;

    points[2].x = quad->pos.x + quad->offset2.x;
    points[2].y = quad->pos.y + quad->offset2.y;

    points[3].x = quad->pos.x + quad->offset3.x;
    points[3].y = quad->pos.y + quad->offset3.y;

    points[4] = points[0];

    if(0 != wn_PnPoly(point, points, elemof32(points))) /* NB NOT n */
        return(TRUE);

    { /* might be very thin, line-like, so test for proximity to edges */
    U32 i;
    for(i = 0; i < 4; ++i)
    {
        if(line_hit(&points[i], &points[i+1], point, size))
            return(TRUE);
    }
    } /*block*/

    return(FALSE);
}

_Check_return_
static BOOL
gr_diag_quadrilateral_hit(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OFFSET testObject,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size)
{
    const PC_BYTE pObject = array_ptrc(&p_gr_diag->handle, BYTE, testObject);
    const GR_DIAG_OBJQUADRILATERAL * const quad = (const GR_DIAG_OBJQUADRILATERAL *) pObject;

    /* simple test to see whether we hit this object */
    if(!gr_box_hit(&quad->bbox, point, size))
        return(FALSE);

    return(gr_diag_quadrilateral_hit_refine(quad, point, size));
}

/******************************************************************************
*
* add a rectangle as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_rectangle_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_BOX pBox,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec)
{
    GR_DIAG_OBJRECTANGLE rect;

    rect.type = GR_DIAG_OBJTYPE_RECTANGLE;
    rect.objid = objid;

    rect.pos.x = pBox->x0;
    rect.pos.y = pBox->y0;

    rect.size.cx = pBox->x1 - pBox->x0;
    rect.size.cy = pBox->y1 - pBox->y0;

    rect.linestyle = *linestyle;
    rect.fillstylec = *fillstylec;

    return(gr_diag_object_new(p_gr_diag, pObjectStart, NULL, &rect, 0));
}

/******************************************************************************
*
* add a picture (scaled into a rectangle) as a named object in the diagram
*
******************************************************************************/

_Check_return_
extern STATUS
gr_diag_scaled_picture_add(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_BOX pBox,
    _InVal_     IMAGE_CACHE_HANDLE picture,
    _InRef_     PC_GR_FILLSTYLEB fillstyleb,
    _InRef_opt_ PC_GR_FILLSTYLEC fillstylec)
{
    GR_DIAG_OBJPICTURE pict;

    pict.type = GR_DIAG_OBJTYPE_PICTURE;
    pict.objid = objid;

    pict.pos.x = pBox->x0;
    pict.pos.y = pBox->y0;

    pict.size.cx = pBox->x1 - pBox->x0;
    pict.size.cy = pBox->y1 - pBox->y0;

    pict.picture = picture;

    pict.fillstyleb = *fillstyleb;

    if(fillstylec)
        pict.fillstylec = *fillstylec;
    else
        zero_struct(pict.fillstylec);

    return(gr_diag_object_new(p_gr_diag, pObjectStart, NULL, &pict, 0));
}

_Check_return_
extern STATUS
gr_diag_text_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_BOX pBox,
    _In_z_      PC_USTR ustr,
    _InRef_     PC_GR_TEXTSTYLE textstyle)
{
    GR_DIAG_OBJTEXT text;
    U32 size = ustrlen32p1(ustr) /*CH_NULL*/;
    P_BYTE pObject;

    text.type = GR_DIAG_OBJTYPE_TEXT;
    text.objid = objid;

    text.pos.x = pBox->x0;
    text.pos.y = pBox->y0;

    text.size.cx = pBox->x1 - pBox->x0;
    text.size.cy = pBox->y1 - pBox->y0;

    text.textstyle = *textstyle;

    status_return(gr_diag_object_new(p_gr_diag, pObjectStart, &pObject, &text, round_up(size, 4) /* round up to output word boundary */));

    memcpy32(pObject + sizeof32(text), ustr, size);

    return(STATUS_OK);
}

/* end of gr_diag.c */
