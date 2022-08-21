/* df_paint.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2001-2015 R W Colton */

/* Rendering to Drawfile graphics routines for Fireworkz */

/* SKS July 2001 / May 2006 extracted here */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef __gr_diag_h
#include "cmodules/gr_diag.h"
#endif

#ifndef __gr_rdia3_h
#include "cmodules/gr_rdia3.h"
#endif

static inline void
draw_point_from_pixit_point_and_context(
    _OutRef_    P_DRAW_POINT p_draw_point,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT_POINT pixit_point = *p_pixit_point;

    pixit_point.x += p_redraw_context->pixit_origin.x;
    pixit_point.y += p_redraw_context->pixit_origin.y;

    pixit_point.x += p_redraw_context->page_pixit_origin_draw.x;
    pixit_point.y -= p_redraw_context->page_pixit_origin_draw.y; /* Draw file coordinate system is the right way up */

    p_draw_point->x = +muldiv64(/*+*/pixit_point.x, GR_RISCDRAW_PER_PIXIT * p_redraw_context->host_xform.scale.t.x, p_redraw_context->host_xform.scale.b.x);
    p_draw_point->y = -muldiv64(/*-*/pixit_point.y, GR_RISCDRAW_PER_PIXIT * p_redraw_context->host_xform.scale.t.y, p_redraw_context->host_xform.scale.b.y);

    /*p_draw_point->x -= p_redraw_context->gdi_org.x;*/
    /*p_draw_point->y -= p_redraw_context->gdi_org.y;*/
}

static inline void
draw_box_from_pixit_rect_and_context(
    _OutRef_    P_DRAW_BOX p_draw_box,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT_RECT pixit_rect = *p_pixit_rect;

    pixit_rect.tl.x += p_redraw_context->pixit_origin.x;
    pixit_rect.tl.y += p_redraw_context->pixit_origin.y;
    pixit_rect.br.x += p_redraw_context->pixit_origin.x;
    pixit_rect.br.y += p_redraw_context->pixit_origin.y;

    pixit_rect.tl.x += p_redraw_context->page_pixit_origin_draw.x;
    pixit_rect.tl.y -= p_redraw_context->page_pixit_origin_draw.y; /* Draw file coordinate system is the right way up */
    pixit_rect.br.x += p_redraw_context->page_pixit_origin_draw.x;
    pixit_rect.br.y -= p_redraw_context->page_pixit_origin_draw.y;

    p_draw_box->x0 = +muldiv64(/*+*/pixit_rect.tl.x, GR_RISCDRAW_PER_PIXIT * p_redraw_context->host_xform.scale.t.x, p_redraw_context->host_xform.scale.b.x);
    p_draw_box->y0 = -muldiv64(/*-*/pixit_rect.br.y, GR_RISCDRAW_PER_PIXIT * p_redraw_context->host_xform.scale.t.y, p_redraw_context->host_xform.scale.b.y);
    p_draw_box->x1 = +muldiv64(/*+*/pixit_rect.br.x, GR_RISCDRAW_PER_PIXIT * p_redraw_context->host_xform.scale.t.x, p_redraw_context->host_xform.scale.b.x);
    p_draw_box->y1 = -muldiv64(/*-*/pixit_rect.tl.y, GR_RISCDRAW_PER_PIXIT * p_redraw_context->host_xform.scale.t.y, p_redraw_context->host_xform.scale.b.y);

    /*p_draw_box->x0 -= p_redraw_context->gdi_org.x;*/
    /*p_draw_box->y0 -= p_redraw_context->gdi_org.y;*/
    /*p_draw_box->x1 -= p_redraw_context->gdi_org.x;*/
    /*p_draw_box->y1 -= p_redraw_context->gdi_org.y;*/
}

extern void
drawfile_paint_rectangle_filled(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_RGB p_rgb)
{
    DRAW_BOX draw_box;
    DRAW_POINT draw_point;
    DRAW_SIZE draw_size;
    static GR_LINESTYLE gr_linestyle; /* init to zero */
    GR_FILLSTYLEC gr_fillstylec;
    DRAW_DIAG_OFFSET rect_start;
    STATUS status;

    //gr_linestyle.pattern = GR_LINE_PATTERN_NONE;
    * (P_S32) &gr_fillstylec.fg = 0;
    gr_fillstylec.fg.visible = 1;
    gr_fillstylec.fg.red   = p_rgb->r;
    gr_fillstylec.fg.green = p_rgb->g;
    gr_fillstylec.fg.blue  = p_rgb->b;

    draw_box_from_pixit_rect_and_context(&draw_box, p_pixit_rect, p_redraw_context);

    draw_point.x = draw_box.x0;
    draw_point.y = draw_box.y0;
    draw_size.cx = draw_box.x1 - draw_box.x0;
    draw_size.cy = draw_box.y1 - draw_box.y0;

    if(status_fail(status = gr_riscdiag_rectangle_new(p_redraw_context->p_gr_riscdiag, &rect_start /*filled*/, &draw_point, &draw_size, &gr_linestyle, &gr_fillstylec)))
        return;
}

/******************************************************************************
*
* search diagram's font list object for given font, returning its fontref
*
******************************************************************************/

static DRAW_FONT_REF16
fonty_gr_riscdiag_fontlist_lookup_host_font_name(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET fontListR,
    _InVal_     PCTSTR tstrHostFontName,
    _OutRef_    P_BOOL p_found)
{
    DRAW_FONT_REF16 fontRefNum = 0; /* System font */

    *p_found = FALSE;

    if(0 != fontListR)
    {
        P_DRAW_OBJECT_FONTLIST pFontListObject = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, p_gr_riscdiag, fontListR);
        DRAW_DIAG_OFFSET nextObject = fontListR + pFontListObject->size;
        DRAW_DIAG_OFFSET thisOffset = fontListR + sizeof32(*pFontListObject);
        P_DRAW_FONTLIST_ELEM pFontListElemR = gr_riscdiag_getoffptr(DRAW_FONTLIST_ELEM, p_gr_riscdiag, thisOffset);

        /* actual end of RISC OS font list object data may not be word aligned */
        while((nextObject - thisOffset) >= 4)
        {
            const DRAW_DIAG_OFFSET thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + strlen32p1(pFontListElemR->szHostFontName); /*CH_NULL*/

            fontRefNum = (DRAW_FONT_REF16) pFontListElemR->fontref8;

            if(0 == tstricmp(_tstr_from_sbstr(pFontListElemR->szHostFontName), tstrHostFontName))
            {
                *p_found = TRUE;
                break;
            }

            thisOffset += thislen;

            pFontListElemR = PtrAddBytes(P_DRAW_FONTLIST_ELEM, pFontListElemR, thislen);
        }
    }

    return(fontRefNum); /* returns the matched font reference, or the last font reference in the list */
}

static DRAW_FONT_REF16
fonty_gr_riscdiag_fontlist_lookup_textstyle(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET fontListR,
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle,
    _OutRef_    P_BOOL p_found)
{
    DRAW_FONT_REF16 fontRefNum = 0; /* System font */

    *p_found = FALSE;

    if(0 != fontListR)
    {
        HOST_FONT_SPEC host_font_spec;
        STATUS status = gr_riscdiag_host_font_spec_riscos_from_textstyle(&host_font_spec, p_gr_textstyle);

        if( status_ok(status) && (0 != array_elements(&host_font_spec.h_host_name_tstr)) )
            fontRefNum = fonty_gr_riscdiag_fontlist_lookup_host_font_name(p_gr_riscdiag, fontListR, array_tstr(&host_font_spec.h_host_name_tstr), p_found);

        host_font_spec_dispose(&host_font_spec);
    }

    return(fontRefNum);
}

_Check_return_
static STATUS
add_host_font_name_to_lookup_fontlist_object(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag_lookup,
    _InVal_     PCTSTR tstrHostFontName,
    _InVal_     DRAW_FONT_REF16 fontRefNum)
{
    const PC_SBSTR szHostFontName = _sbstr_from_tstr(tstrHostFontName);
    const S32 namelen_p1 = strlen32p1(szHostFontName); /*CH_NULL*/
    const S32 thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + namelen_p1;
    const S32 pad_bytes = ((thislen + (4-1)) & ~(4-1)) - thislen;
    STATUS status;
    P_BYTE p_u8;

    assert((fontRefNum != 0) && (fontRefNum < 256));

    /* we have to allocate multiples of 4 */
    if(NULL != (p_u8 = gr_riscdiag_ensure(BYTE, p_gr_riscdiag_lookup, thislen + pad_bytes, &status)))
    {
        {
        P_DRAW_FONTLIST_ELEM pFontListElemR = (P_DRAW_FONTLIST_ELEM) p_u8;
        pFontListElemR->fontref8 = (U8) fontRefNum;
        memcpy32(pFontListElemR->szHostFontName, szHostFontName, namelen_p1);
        } /*block*/

        /* adjust the font object header. NB size may not be a multiple of 4 */
        p_u8 = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag_lookup, p_gr_riscdiag_lookup->dd_fontListR);
        ((P_DRAW_OBJECT_HEADER_NO_BBOX) p_u8)->size += thislen;

        /* give the remainder back! NB length may not be a multiple of 4 */
        p_gr_riscdiag_lookup->draw_diag.length -= pad_bytes;
    }

    return(status);
}

_Check_return_
static STATUS
ensure_textstyle_in_lookup_fontlist_object(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag_lookup,
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle)
{
    /* search for the text style using p_gr_riscdiag_lookup */
    BOOL found;
    DRAW_FONT_REF16 fontRefNum = fonty_gr_riscdiag_fontlist_lookup_textstyle(p_gr_riscdiag_lookup, p_gr_riscdiag_lookup->dd_fontListR, p_gr_textstyle, &found);

    if(!found)
    {
        /* not present - best add it, one ref# beyond what's already there */
        HOST_FONT_SPEC host_font_spec;
        STATUS status = gr_riscdiag_host_font_spec_riscos_from_textstyle(&host_font_spec, p_gr_textstyle);

        if( status_ok(status) && (0 != array_elements(&host_font_spec.h_host_name_tstr)) )
        {
            status = add_host_font_name_to_lookup_fontlist_object(p_gr_riscdiag_lookup, array_tstr(&host_font_spec.h_host_name_tstr), fontRefNum + 1);

            host_font_spec_dispose(&host_font_spec);
        }

        return(status);
    }

    return(STATUS_OK);
}

static void
drawfile_adjust_font_size(
    _InoutRef_  P_FONT_SPEC p_font_spec,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    if( (p_redraw_context->host_xform.scale.t.x != p_redraw_context->host_xform.scale.b.x) &&
        (p_redraw_context->host_xform.scale.t.y != p_redraw_context->host_xform.scale.b.y) )
    {
        p_font_spec->size_x = muldiv64(p_font_spec->size_x, p_redraw_context->host_xform.scale.t.x, p_redraw_context->host_xform.scale.b.x);
        p_font_spec->size_y = muldiv64(p_font_spec->size_y, p_redraw_context->host_xform.scale.t.y, p_redraw_context->host_xform.scale.b.y);
    }

    if(p_font_spec->superscript || p_font_spec->subscript)
    {
        if(0 != p_font_spec->size_x)
        {
            p_font_spec->size_x *= 7;
            p_font_spec->size_x /= 12;
        }
        p_font_spec->size_y *= 7;
        p_font_spec->size_y /= 12;
    }
}

extern void
drawfile_paint_uchars(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_RGB p_rgb_back,
    _InRef_     PC_FONT_CONTEXT p_font_context)
{
    FONT_SPEC font_spec_adjust = p_font_context->font_spec;
    DRAW_POINT draw_point;
    GR_TEXTSTYLE textstyle;
    GR_COLOUR bg;
    DRAW_DIAG_OFFSET text_start;
    STATUS status;

    drawfile_adjust_font_size(&font_spec_adjust, p_redraw_context);

    textstyle.size_x   = font_spec_adjust.size_x;
    textstyle.size_y   = font_spec_adjust.size_y;
    textstyle.bold     = font_spec_adjust.bold;
    textstyle.italic   = font_spec_adjust.italic;
    tstr_xstrkpy(textstyle.tstrFontName, elemof32(textstyle.tstrFontName), array_tstr(&p_font_context->font_spec.h_app_name_tstr));

    zero_32(textstyle.fg);
    textstyle.fg.visible = 1;
    textstyle.fg.red   = font_spec_adjust.colour.r;
    textstyle.fg.green = font_spec_adjust.colour.g;
    textstyle.fg.blue  = font_spec_adjust.colour.b;

    zero_32(bg);
    bg.visible = !p_rgb_back->transparent;
    bg.red   = p_rgb_back->r;
    bg.green = p_rgb_back->g;
    bg.blue  = p_rgb_back->b;

    draw_point_from_pixit_point_and_context(&draw_point, p_pixit_point, p_redraw_context);

    if(status_fail(status = ensure_textstyle_in_lookup_fontlist_object(p_redraw_context->lookup_gr_riscdiag, &textstyle)))
        return;

    if(status_fail(status = gr_riscdiag_string_new_uchars(p_redraw_context->p_gr_riscdiag, &text_start /*filled*/, &draw_point, uchars, uchars_n, &textstyle, textstyle.fg, &bg, p_redraw_context->lookup_gr_riscdiag)))
        return;
}

#if WINDOWS

static const DRAW_PATH_STYLE
gr_riscdiag_path_style_default =
{
    0, /* flags     */
    0, /* reserved  */
    0, /* tricap_w  */
    0, /* tricap_h  */
};

_Check_return_
_Ret_writes_maybenull_(extraBytes)
static P_BYTE
gr_riscdiag_path_new_raw(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pPathStart,
    _InVal_     DRAW_COORD thickness,
    _InRef_opt_ PC_DRAW_DASH_HEADER dash_pattern,
    _InRef_     PC_RGB p_rgb,
    _InVal_     U32 extraBytes,
    _OutRef_    P_STATUS p_status)
{
    U32 nDashBytes = 0;
    DRAW_OBJECT_PATH path;
    P_BYTE pObject;

    if(NULL != dash_pattern)
        nDashBytes = sizeof32(*dash_pattern) + sizeof32(S32) * dash_pattern->dashcount;

    path.type = DRAW_OBJECT_TYPE_PATH;
    path.size = sizeof32(path) + nDashBytes + extraBytes;
    draw_box_make_bad(&path.bbox);

    path.fillcolour = DRAW_COLOUR_Transparent;
    {
    GR_COLOUR gr_colour;
    * (P_S32) &gr_colour = 0;
    gr_colour.visible = 1;
    gr_colour.red   = p_rgb->r;
    gr_colour.green = p_rgb->g;
    gr_colour.blue  = p_rgb->b;
    path.pathcolour = gr_colour_to_riscDraw(gr_colour);
    } /*block*/
    path.pathwidth = thickness;
    path.pathstyle = gr_riscdiag_path_style_default;

    if(NULL != dash_pattern)
        path.pathstyle.flags |= DRAW_PS_DASH_PACK_MASK;

    *pPathStart = gr_riscdiag_query_offset(p_gr_riscdiag);

    if(NULL == (pObject = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, path.size, p_status)))
        return(NULL);

    memcpy32(pObject, &path, sizeof32(path));

    if(NULL != dash_pattern)
        memcpy32(pObject + sizeof32(path), dash_pattern, nDashBytes);

    return(pObject + sizeof32(path) + nDashBytes);
}

#endif /* WINDOWS */

#if WINDOWS

struct gr_riscdiag_line_guts
{
    DRAW_PATH_MOVE  pos;    /* move to first point */
    DRAW_PATH_LINE  lineto; /* line to second point */
    DRAW_PATH_TERM  term;
};

extern void
drawfile_paint_line(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InVal_     DRAW_COORD thickness,
    _InRef_opt_ PC_DRAW_DASH_HEADER dash_pattern,
    _InRef_     PC_RGB p_rgb)
{
    DRAW_BOX draw_box;
    struct gr_riscdiag_line_guts rd_line;
    DRAW_DIAG_OFFSET line_start;
    P_BYTE pLine;
    STATUS status;
    const DRAW_COORD half_width = thickness >> 1;

    draw_box_from_pixit_rect_and_context(&draw_box, p_pixit_rect, p_redraw_context);

    rd_line.pos.tag = DRAW_PATH_TYPE_MOVE;
    rd_line.pos.pt.x = MIN(draw_box.x0, draw_box.x1);
    rd_line.pos.pt.y = MIN(draw_box.y0, draw_box.y1);

    rd_line.lineto.tag = DRAW_PATH_TYPE_LINE;
    rd_line.lineto.pt.x = MAX(draw_box.x0, draw_box.x1);
    rd_line.lineto.pt.y = MAX(draw_box.y0, draw_box.y1);

    rd_line.term.tag = DRAW_PATH_TYPE_TERM;

    /* put the line down the middle of the rectangle that the line lies in */
    if(rd_line.pos.pt.y == rd_line.lineto.pt.y)
    {   /* horizontal line */
        rd_line.pos.pt.y    -= half_width;
        rd_line.lineto.pt.y -= half_width;
    }
    else if(rd_line.pos.pt.x == rd_line.lineto.pt.x)
    {   /* vertical line */
        rd_line.pos.pt.x    += half_width;
        rd_line.lineto.pt.x += half_width;
    }

    if(NULL != (pLine = gr_riscdiag_path_new_raw(p_redraw_context->p_gr_riscdiag, &line_start, thickness, dash_pattern, p_rgb, sizeof32(rd_line), &status)))
        memcpy32(pLine, &rd_line, sizeof32(rd_line));

    if(status_fail(status))
        return;
}

#endif /* OS */

_Check_return_
static STATUS
ensure_diagram_fontlistR_in_lookup_fontlist_object(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag_lookup,
    _InRef_     PC_GR_RISCDIAG p_gr_riscdiag_source)
{
    const DRAW_DIAG_OFFSET fontListR = p_gr_riscdiag_source->dd_fontListR;
    P_DRAW_OBJECT_FONTLIST pFontListObject = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, p_gr_riscdiag_source, fontListR);
    DRAW_DIAG_OFFSET nextObject = fontListR + pFontListObject->size;
    DRAW_DIAG_OFFSET thisOffset = fontListR + sizeof32(*pFontListObject);
    assert(0 != p_gr_riscdiag_source->dd_fontListR);

    /* actual end of RISC OS font list object data may not be word aligned */
    while((nextObject - thisOffset) >= 4)
    {
        const PC_DRAW_FONTLIST_ELEM pFontListElemR = gr_riscdiag_getoffptr(DRAW_FONTLIST_ELEM, p_gr_riscdiag_source, thisOffset);
        const DRAW_DIAG_OFFSET thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + strlen32p1(pFontListElemR->szHostFontName); /*CH_NULL*/
        const PCTSTR tstrHostFontName = _tstr_from_sbstr(pFontListElemR->szHostFontName);
        BOOL found;
        DRAW_FONT_REF16 fontRefNum = fonty_gr_riscdiag_fontlist_lookup_host_font_name(p_gr_riscdiag_lookup, p_gr_riscdiag_lookup->dd_fontListR, tstrHostFontName, &found);

        if(!found)
        {   /* not present - best add it, one ref# beyond what's already there */
            status_return(add_host_font_name_to_lookup_fontlist_object(p_gr_riscdiag_lookup, tstrHostFontName, fontRefNum + 1));
        }

        thisOffset += thislen;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
ensure_diagram_fonts_in_lookup_fontlist_object(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag_lookup,
    _In_reads_(diag_len) PC_BYTE p_diag,
    _InVal_     U32 diag_len)
{
    GR_RISCDIAG source_gr_riscdiag;
    U32 diagLength = diag_len;

    if(NULL == p_diag)
        return(STATUS_FAIL);

    if(diagLength <= sizeof32(DRAW_FILE_HEADER))
        return(status_check());

    /* scan the diagram to be copied for font tables - no need to set hglobal */
    gr_riscdiag_diagram_setup_from_data(&source_gr_riscdiag, p_diag, diag_len);

    {
    DRAW_DIAG_OFFSET sttObject = DRAW_DIAG_OFFSET_FIRST;
    DRAW_DIAG_OFFSET endObject = DRAW_DIAG_OFFSET_LAST;
    P_BYTE pObject;

    if(gr_riscdiag_object_first(&source_gr_riscdiag, &sttObject, &endObject, &pObject, FALSE)) /* flat scan good enough for what I want */
    {
        do {
            switch(*DRAW_OBJHDR(U32, pObject, type))
            {
            /* these objects are never grouped */
            case DRAW_OBJECT_TYPE_FONTLIST:
                source_gr_riscdiag.dd_fontListR = sttObject;
/*reportf(TEXT("edf: ") PTR_XTFMT TEXT(" fontListR at %d"), &source_gr_riscdiag, sttObject);*/
                status_return(ensure_diagram_fontlistR_in_lookup_fontlist_object(p_gr_riscdiag_lookup, &source_gr_riscdiag));
                break;

            case DRAW_OBJECT_TYPE_DS_WINFONTLIST:
                source_gr_riscdiag.dd_fontListW = sttObject;
/*reportf(TEXT("edf: ") PTR_XTFMT TEXT(" fontListW at %d"), &source_gr_riscdiag, sttObject);*/
                break;

            default:
                break;
            }
        }
        while(gr_riscdiag_object_next(&source_gr_riscdiag, &sttObject, &endObject, &pObject, FALSE));
    }
    } /*block*/

    return(STATUS_OK);
}

extern void
drawfile_paint_drawfile(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_GR_SCALE_PAIR p_gr_scale_pair,
    _InRef_     PC_DRAW_DIAG p_draw_diag)
{
    DRAW_POINT draw_point;
    GR_SCALE_PAIR gr_scale_pair; /* scale for the diagram being added, not the overall scale */
    GR_FILLSTYLEB fillstyleb;
    DRAW_DIAG_OFFSET drawfile_start;
    STATUS status = STATUS_OK;

    gr_scale_pair.x = muldiv64(p_gr_scale_pair->x, p_redraw_context->host_xform.scale.t.x, p_redraw_context->host_xform.scale.b.x);
    gr_scale_pair.y = muldiv64(p_gr_scale_pair->y, p_redraw_context->host_xform.scale.t.y, p_redraw_context->host_xform.scale.b.y);

    zero_struct(fillstyleb);
    /*fillstyleb.bits.isotropic = 0;*/ /* zero in order not to square and centre */
    fillstyleb.bits.norecolour = 1;

    draw_point_from_pixit_point_and_context(&draw_point, p_pixit_point, p_redraw_context);

    {
    PC_DRAW_FILE_HEADER p_draw_file_header;

    if(NULL != (p_draw_file_header = (PC_DRAW_FILE_HEADER) p_draw_diag->data))
    {
        const DRAW_BOX bbox = p_draw_file_header->bbox;
        DRAW_BOX draw_box;

        draw_box.x0 = draw_point.x;
        draw_box.y1 = draw_point.y;
        draw_box.x1 = draw_box.x0 + draw_coord_scale(bbox.x1 - bbox.x0, gr_scale_pair.x);
        draw_box.y0 = draw_box.y1 - draw_coord_scale(bbox.y1 - bbox.y0, gr_scale_pair.y);

        if(status_ok(status = ensure_diagram_fonts_in_lookup_fontlist_object(p_redraw_context->lookup_gr_riscdiag, (PC_BYTE) p_draw_file_header, p_draw_diag->length)))
            status = gr_riscdiag_scaled_diagram_add(p_redraw_context->p_gr_riscdiag, &drawfile_start, &draw_box, (PC_BYTE) p_draw_file_header, p_draw_diag->length, &fillstyleb, NULL /*fillstylec*/, p_redraw_context->lookup_gr_riscdiag);
    }
    } /*block*/

    if(status_fail(status))
        return;
}

/* end of ob_skel/df_paint.c */
