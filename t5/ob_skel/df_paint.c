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
    static GR_LINESTYLE gr_linestyle; // init to zero
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

    if(status_fail(status = gr_riscdiag_rectangle_new(p_redraw_context->p_gr_riscdiag, &rect_start /*filled*/, &draw_box, &gr_linestyle, &gr_fillstylec)))
        return;
}

/******************************************************************************
*
* search diagram's font list object for given font, returning its fontref
*
******************************************************************************/

static DRAW_FONT_REF16
fonty_gr_riscdiag_fontlist_lookup_textstyle(
    P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET fontListR,
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle,
    _OutRef_    P_BOOL p_found)
{
    DRAW_FONT_REF16 fontRefNum = 0; /* will have to use System font unless request matched */
    HOST_FONT_SPEC host_font_spec;
    STATUS status = gr_riscdiag_host_font_spec_riscos_from_textstyle(&host_font_spec, p_gr_textstyle);

    *p_found = FALSE;

    if(status_ok(status) && fontListR && array_elements(&host_font_spec.h_host_name_tstr))
    {
        PCTSTR tstrHostFontName = array_tstr(&host_font_spec.h_host_name_tstr);
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

        host_font_spec_dispose(&host_font_spec);
    }

    return(fontRefNum);
}

_Check_return_
static STATUS
ensure_textstyle_in_font_object(
    _InoutRef_  P_GR_RISCDIAG lookup_gr_riscdiag,
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle)
{
    /* search for the text style using lookup_gr_riscdiag */
    BOOL found;
    DRAW_FONT_REF16 fontRef = fonty_gr_riscdiag_fontlist_lookup_textstyle(lookup_gr_riscdiag, lookup_gr_riscdiag->dd_fontListR, p_gr_textstyle, &found);

    if(!found)
    {
        /* not present - best add it, one ref# beyond what's already there */
        HOST_FONT_SPEC host_font_spec;
        STATUS status = gr_riscdiag_host_font_spec_riscos_from_textstyle(&host_font_spec, p_gr_textstyle);
        if(status_ok(status) && array_elements(&host_font_spec.h_host_name_tstr))
        {
            const PC_SBSTR szHostFontName = _sbstr_from_tstr(array_tstr(&host_font_spec.h_host_name_tstr));
            const S32 namelen_p1 = strlen32p1(szHostFontName); /*CH_NULL*/
            const S32 thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + namelen_p1;
            const S32 pad_bytes = ((thislen + (4-1)) & ~(4-1)) - thislen;
            P_BYTE p_u8;
            /* we have to allocate multiples of 4 */
            if(NULL != (p_u8 = gr_riscdiag_ensure(BYTE, lookup_gr_riscdiag, thislen + pad_bytes, &status)))
            {
                /* give the remainder back! */
                lookup_gr_riscdiag->draw_diag.length -= pad_bytes;
                {
                P_DRAW_FONTLIST_ELEM pFontListElemR = (P_DRAW_FONTLIST_ELEM) p_u8;
                pFontListElemR->fontref8 = (U8) (fontRef + 1);
                memcpy32(pFontListElemR->szHostFontName, szHostFontName, namelen_p1);
                } /*block*/
                /* adjust the font object header too */
                p_u8 = gr_riscdiag_getoffptr(BYTE, lookup_gr_riscdiag, lookup_gr_riscdiag->dd_fontListR);
                ((P_DRAW_OBJECT_HEADER_NO_BBOX) p_u8)->size += thislen;
            }
            host_font_spec_dispose(&host_font_spec);
        }
        status_assert(status);
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

    textstyle.width    = font_spec_adjust.size_x;
    textstyle.height   = font_spec_adjust.size_y;
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

    if(status_fail(status = ensure_textstyle_in_font_object(p_redraw_context->lookup_gr_riscdiag, &textstyle)))
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
    _InVal_     S32 thickness,
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
    DRAW_PATH_MOVE move;    /* move to bottom left  */
    DRAW_PATH_LINE line;    /* line to bottom right */
    DRAW_PATH_TERM term;
};

extern void
drawfile_paint_line(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InVal_     S32 thickness,
    _InRef_opt_ PC_DRAW_DASH_HEADER dash_pattern,
    _InRef_     PC_RGB p_rgb)
{
    DRAW_BOX draw_box;
    struct gr_riscdiag_line_guts line;
    DRAW_DIAG_OFFSET line_start;
    P_BYTE pLine;
    STATUS status;

    draw_box_from_pixit_rect_and_context(&draw_box, p_pixit_rect, p_redraw_context);

    line.move.tag = DRAW_PATH_TYPE_MOVE;
    line.move.pt.x = MIN(draw_box.x0, draw_box.x1);
    line.move.pt.y = MIN(draw_box.y0, draw_box.y1);

    line.line.tag = DRAW_PATH_TYPE_LINE;
    line.line.pt.x = MAX(draw_box.x0, draw_box.x1);
    line.line.pt.y = MAX(draw_box.y0, draw_box.y1);

    line.term.tag = DRAW_PATH_TYPE_TERM;

    if(NULL != (pLine = gr_riscdiag_path_new_raw(p_redraw_context->p_gr_riscdiag, &line_start, thickness, dash_pattern, p_rgb, sizeof32(line), &status)))
        memcpy32(pLine, &line, sizeof32(line));

    if(status_fail(status))
        return;
}

#endif /* OS */

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

        status = gr_riscdiag_scaled_diagram_add(p_redraw_context->p_gr_riscdiag, &drawfile_start, &draw_box, (P_BYTE) p_draw_file_header, p_draw_diag->length, &fillstyleb, NULL /*fillstylec*/);
    }
    } /*block*/

    if(status_fail(status))
        return;
}

/* end of ob_skel/df_paint.c */
