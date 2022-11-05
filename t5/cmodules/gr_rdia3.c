/* gr_rdia3.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RISC OS Draw file creation */

/* SKS July 1992 */

#include "common/gflags.h"

#define EXPOSE_RISCOS_FONT 1
#define EXPOSE_RISCOS_SWIS 1

#include "ob_chart/ob_chart.h"

#ifndef          __im_cache_h
#include "cmodules/im_cache.h"
#endif

#ifndef          __bezier_h
#include "cmodules/bezier.h"
#endif

extern GR_COLOUR
gr_colour_from_riscDraw(
    _InVal_     DRAW_COLOUR riscDraw)
{
    GR_COLOUR colour;

    * (int *) &colour = 0;

    if(riscDraw != DRAW_COLOUR_Transparent)
    {
        colour.visible  = 1;
        colour.red      = ((unsigned int) riscDraw >>  8) & 0xFF;
        colour.green    = ((unsigned int) riscDraw >> 16) & 0xFF;
        colour.blue     = ((unsigned int) riscDraw >> 24) & 0xFF;
    }

    return(colour);
}

extern DRAW_COLOUR
gr_colour_to_riscDraw(
    _InVal_     GR_COLOUR colour)
{
    if(!colour.visible)
        return(DRAW_COLOUR_Transparent); /* transparent (if it gets this far) */

    return((DRAW_COLOUR) (((((((U32) colour.blue) << 8) | (U32) colour.green) << 8) | (U32) colour.red) << 8));
}

/******************************************************************************
*
* perform font mapping for charts
*
******************************************************************************/

extern void
gr_riscdiag_host_font_dispose(
    _InoutRef_  P_HOST_FONT p_host_font)
{
    HOST_FONT host_font = *p_host_font;

    if(HOST_FONT_NONE != host_font)
    {
        *p_host_font = HOST_FONT_NONE;

#if RISCOS
        WrapOsErrorReporting(font_LoseFont(host_font));
#endif
    }
}

_Check_return_
extern HOST_FONT
gr_riscdiag_host_font_from_textstyle(
    _InRef_     PC_GR_TEXTSTYLE textstyle)
{
    HOST_FONT host_font = HOST_FONT_NONE;

#if RISCOS

    HOST_FONT_SPEC host_font_spec;

    status_assert(gr_riscdiag_host_font_spec_riscos_from_textstyle(&host_font_spec, textstyle));

    if(array_elements(&host_font_spec.h_host_name_tstr))
    {
        _kernel_swi_regs rs;
        _kernel_oserror * p_kernel_oserror;

        /* RISC OS FontManager needs 16x fontsize */
        S32 x16_font_size_y =                         ((16 * host_font_spec.size_y) / PIXITS_PER_POINT);
        S32 x16_font_size_x = host_font_spec.size_x ? ((16 * host_font_spec.size_x) / PIXITS_PER_POINT) : x16_font_size_y;

        rs.r[1] = (int) array_tstr(&host_font_spec.h_host_name_tstr);
        rs.r[2] = x16_font_size_x;
        rs.r[3] = x16_font_size_y;
        rs.r[4] = 0;
        rs.r[5] = 0;

        if(NULL == (p_kernel_oserror = (_kernel_swi(Font_FindFont, &rs, &rs))))
            host_font = (HOST_FONT) rs.r[0];
    }

    host_font_spec_dispose(&host_font_spec);

#else

    UNREFERENCED_PARAMETER_InRef_(textstyle);

#endif /* OS */

    return(host_font);
}

_Check_return_
extern STATUS
gr_riscdiag_host_font_spec_riscos_from_textstyle(
    _OutRef_    P_HOST_FONT_SPEC p_host_font_spec /* h_host_name_tstr as RISC OS e.g. Trinity.Bold.Italic */,
    _InRef_     PC_GR_TEXTSTYLE textstyle)
{
    STATUS status;
    FONT_SPEC font_spec;

    zero_struct_ptr(p_host_font_spec);

    zero_struct(font_spec);
    font_spec.size_x = textstyle->size_x;
    font_spec.size_y = textstyle->size_y;
    font_spec.bold = UBF_UNPACK(U8, textstyle->bold);
    font_spec.italic = UBF_UNPACK(U8, textstyle->italic);
    status_return(font_spec_name_alloc(&font_spec, textstyle->tstrFontName));

    status = fontmap_host_font_spec_riscos_from_font_spec(p_host_font_spec, &font_spec);

    font_spec_dispose(&font_spec);
    return(status);
}

#if defined(UNUSED_KEEP_ALIVE)

/******************************************************************************
*
* close the current subpath with a line
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_path_close(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag)
{
    STATUS status;
    P_U32 p_u32;

    if(NULL != (p_u32 = gr_riscdiag_ensure(U32, p_gr_riscdiag, sizeof32(DRAW_PATH_CLOSE), &status)))
        *p_u32 = DRAW_PATH_TYPE_CLOSE_WITH_LINE;

    return(status);
}

/******************************************************************************
*
* add a bezier line segment to current subpath
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_path_curveto(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_COORD cp1x,
    _InVal_     DRAW_COORD cp1y,
    _InVal_     DRAW_COORD cp2x,
    _InVal_     DRAW_COORD cp2y,
    _InVal_     DRAW_COORD endx,
    _InVal_     DRAW_COORD endy)
{
    DRAW_PATH_CURVE curve;
    STATUS status;
    P_BYTE pObject;

    curve.tag = DRAW_PATH_TYPE_CURVE;
    curve.cp1.x = cp1x; /* control point 1 */
    curve.cp1.y = cp1y;
    curve.cp2.x = cp2x; /* control point 2 */
    curve.cp2.y = cp2y;
    curve.end.x = endx; /* end point */
    curve.end.y = endy;

    if(NULL != (pObject = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, sizeof32(curve), &status)))
        memcpy32(pObject, &curve, sizeof32(curve));

    return(STATUS_OK);
}

/******************************************************************************
*
* add a straight line segment to current subpath
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_path_lineto(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_COORD x,
    _InVal_     DRAW_COORD y)
{
    STATUS status;
    P_U32 p_u32;

    if(NULL != (p_u32 = gr_riscdiag_ensure(U32, p_gr_riscdiag, sizeof32(DRAW_PATH_LINE), &status)))
    {
        *p_u32++ = DRAW_PATH_TYPE_LINE;
        *p_u32++ = x;
        *p_u32++ = y;
    }

    return(status);
}

/******************************************************************************
*
* start a new subpath
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_path_moveto(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_COORD x,
    _InVal_     DRAW_COORD y)
{
    STATUS status;
    P_U32 p_u32;

    if(NULL != (p_u32 = gr_riscdiag_ensure(U32, p_gr_riscdiag, sizeof32(DRAW_PATH_MOVE), &status)))
    {
        *p_u32++ = DRAW_PATH_TYPE_MOVE;
        *p_u32++ = x;
        *p_u32++ = y;
    }

    return(status);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* start a path object
*
******************************************************************************/

static const DRAW_PATH_STYLE
gr_riscdiag_path_style_default =
{
    (DRAW_PS_JOIN_MITRED      << DRAW_PS_JOIN_PACK_SHIFT    ) | /* NB. big mitres > width converted into bevels automagically */
    (DRAW_PS_CAP_BUTT         << DRAW_PS_ENDCAP_PACK_SHIFT  ) |
    (DRAW_PS_CAP_BUTT         << DRAW_PS_STARTCAP_PACK_SHIFT) |
    (DRAW_PS_WINDRULE_NONZERO << DRAW_PS_WINDRULE_PACK_SHIFT) |
    (DRAW_PS_DASH_ABSENT      << DRAW_PS_DASH_PACK_SHIFT    ) |
    0, /* flags     */
    0, /* reserved  */
    0, /* tricap_w  */
    0, /* tricap_h  */
};

/* keep consistent with CModules.UK.spr.gr_chart (sprites gr_ec_lstyp1..4, 0 is solid) */

static const struct linestyle_riscos_dash_2 { DRAW_DASH_HEADER hdr; S32 pattern[2]; }
gr_linestyle_riscos_dash =
{
    { gr_riscDraw_from_point(0) /* start distance */, 2 /* count */ },
    { gr_riscDraw_from_point(8), gr_riscDraw_from_point(8) }
};

static const struct linestyle_riscos_dash_2
gr_linestyle_riscos_dot =
{
    { gr_riscDraw_from_point(0) /* start distance */, 2 /* count */ },
    { gr_riscDraw_from_point(2), gr_riscDraw_from_point(2) }
};

static const struct linestyle_riscos_dash_4 { DRAW_DASH_HEADER hdr; S32 pattern[4]; }
gr_linestyle_riscos_dash_dot =
{
    { gr_riscDraw_from_point(0) /* start distance */, 4 /* count */ },
    { gr_riscDraw_from_point(8), gr_riscDraw_from_point(2), gr_riscDraw_from_point(2), gr_riscDraw_from_point(2) }
};

static const struct linestyle_riscos_dash_6 { DRAW_DASH_HEADER hdr; S32 pattern[6]; }
gr_linestyle_riscos_dash_dot_dot =
{
    { gr_riscDraw_from_point(0) /* start distance */, 6 /* count */ },
    { gr_riscDraw_from_point(8), gr_riscDraw_from_point(2), gr_riscDraw_from_point(2), gr_riscDraw_from_point(2), gr_riscDraw_from_point(2), gr_riscDraw_from_point(2) }
};

static const PC_DRAW_DASH_HEADER
gr_linestyle_riscos_dashes[] =
{
    NULL, /* SOLID */
    NULL, /* NONE */
    NULL, /* THIN */
    &gr_linestyle_riscos_dash.hdr,
    &gr_linestyle_riscos_dot.hdr,
    &gr_linestyle_riscos_dash_dot.hdr,
    &gr_linestyle_riscos_dash_dot_dot.hdr
};

_Check_return_
_Ret_maybenull_
static PC_DRAW_DASH_HEADER
gr_linestyle_to_riscDraw(
    _InRef_opt_ PC_GR_LINESTYLE linestyle)
{
    if(NULL == linestyle)
        return(NULL);

    assert(linestyle->pattern < elemof32(gr_linestyle_riscos_dashes));
    if(linestyle->pattern >= elemof32(gr_linestyle_riscos_dashes))
        return(NULL);

    return(gr_linestyle_riscos_dashes[linestyle->pattern]);
}

_Check_return_
_Ret_writes_maybenull_(extraBytes)
extern P_BYTE /* -> path guts */
gr_riscdiag_path_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pPathStart,
    _InRef_opt_ PC_GR_LINESTYLE linestyle,
    _InRef_opt_ PC_GR_FILLSTYLEC fillstylec,
    _InRef_opt_ PC_DRAW_PATH_STYLE pathstyle,
    _InVal_     U32 extraBytes,
    _OutRef_    P_STATUS p_status)
{
    PC_DRAW_DASH_HEADER dash_pattern = gr_linestyle_to_riscDraw(linestyle);
    U32 nDashBytes = 0;
    DRAW_OBJECT_PATH path;
    P_BYTE pObject;

    if(NULL != dash_pattern)
    {
        nDashBytes = sizeof32(*dash_pattern) + sizeof32(S32) * dash_pattern->dashcount;

#ifdef GR_CHART_LINE_FILL_INTERSTICES
        if(linestyle->bg.visible)
            /* hmm. consider at a later date allocating two paths: on path_end memcpy first to second and patch colour fields */
        { /*EMPTY*/
        }
#endif
    }

    path.type = DRAW_OBJECT_TYPE_PATH;
    path.size = sizeof32(path) + nDashBytes + extraBytes;
    draw_box_make_bad(&path.bbox);

    path.fillcolour = fillstylec ? gr_colour_to_riscDraw(fillstylec->fg) : DRAW_COLOUR_Transparent;
    path.pathcolour = (linestyle && (linestyle->pattern != GR_LINE_PATTERN_NONE))
        ? gr_colour_to_riscDraw(linestyle->fg)
        : DRAW_COLOUR_Transparent;
    path.pathwidth  = (linestyle && (linestyle->pattern != GR_LINE_PATTERN_THIN))
        ? gr_riscDraw_from_pixit(linestyle->width)
        : 0;
    path.pathstyle  = pathstyle ? *pathstyle : gr_riscdiag_path_style_default;

    if(NULL != dash_pattern)
        path.pathstyle.flags |= DRAW_PS_DASH_PACK_MASK;

    *pPathStart = gr_riscdiag_query_offset(p_gr_riscdiag);

    if(NULL == (pObject = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, path.size, p_status)))
        return(NULL);

    memcpy32(pObject, &path, sizeof32(path));

    if(NULL != dash_pattern)
        memcpy32(pObject + sizeof32(path), dash_pattern, nDashBytes);

    return(pObject + sizeof32(path) + nDashBytes); /* -> path guts */
}

/******************************************************************************
*
* return the start of the path data in a path object structure
*
******************************************************************************/

_Check_return_
_Ret_valid_
extern P_BYTE
gr_riscdiag_path_query_guts(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET pathStart)
{
    P_BYTE pObject = gr_riscdiag_getoffptr(BYTE, p_gr_riscdiag, pathStart);
    DRAW_OBJECT_PATH path;
    U32 extraBytes;

    memcpy32(&path, pObject, sizeof32(path));

    myassert2x(path.type == DRAW_OBJECT_TYPE_PATH, TEXT("gr_riscdiag_path_query_guts of a non-path object ") U32_XTFMT TEXT(" type ") U32_TFMT, pathStart, path.type);

    /* always skip path header */
    extraBytes = sizeof32(DRAW_OBJECT_PATH);

    if(path.pathstyle.flags & DRAW_PS_DASH_PACK_MASK)
        /* and skip dash if present */
        extraBytes += sizeof32(DRAW_DASH_HEADER)
                    + sizeof32(S32) * (PtrAddBytes(PC_DRAW_DASH_HEADER, pObject, sizeof32(DRAW_OBJECT_PATH)))->dashcount;

    pObject += extraBytes;

    return(pObject);
}

/******************************************************************************
*
* end the path
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_path_term(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag)
{
    STATUS status;
    P_U32 p_u32;

    if(NULL != (p_u32 = gr_riscdiag_ensure(U32, p_gr_riscdiag, sizeof32(DRAW_PATH_TERM), &status)))
        *p_u32 = DRAW_PATH_TYPE_TERM;

    return(status);
}

#if defined(UNUSED_KEEP_ALIVE)

/******************************************************************************
*
* circle object
*
******************************************************************************/

struct gr_riscdiag_circle_guts
{
    DRAW_PATH_MOVE  move;
    DRAW_PATH_CURVE curve[4];
    DRAW_PATH_CLOSE close;
    DRAW_PATH_TERM  term;
};

_Check_return_
extern STATUS
gr_riscdiag_circle_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pCircleStart,
    _InRef_     PC_DRAW_POINT pPos,
    _InVal_     DRAW_COORD radius,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec)
{
    DRAW_POINT bezCentre = *pPos;
    DRAW_COORD bezRadius = radius;
    DRAW_POINT bezPoints[13];
    STATUS status;
    P_BYTE pPathGuts;
    struct gr_riscdiag_circle_guts Circle;
    int i, j;

    bezier_arc_circle(&bezCentre, bezRadius, &bezPoints[0]);

    Circle.move.tag = DRAW_PATH_TYPE_MOVE;
    Circle.move.pt  = bezPoints[0];

    for(i = 0, j  = 1;
        i < 4;
        i++,   j += 3 /* cp1, cp2, end */)
    {
        Circle.curve[i].tag = DRAW_PATH_TYPE_CURVE;
        Circle.curve[i].cp1 = bezPoints[j + 0]; /* 1, 4, 7, 10 */
        Circle.curve[i].cp2 = bezPoints[j + 1]; /* 2, 5, 8, 11 */
        Circle.curve[i].end = bezPoints[j + 2]; /* 3, 6, 9, 12 */
    }

    Circle.close.tag = DRAW_PATH_TYPE_CLOSE_WITH_LINE;

    Circle.term.tag  = DRAW_PATH_TYPE_TERM;

    if(NULL != (pPathGuts = gr_riscdiag_path_new(p_gr_riscdiag, pCircleStart, linestyle, fillstylec, NULL, sizeof32(Circle), &status)))
        memcpy32(pPathGuts, &Circle, sizeof32(Circle));

    return(status);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* line object
*
******************************************************************************/

struct gr_riscdiag_line_guts
{
    DRAW_PATH_MOVE  pos;    /* move to first point  */
    DRAW_PATH_LINE  lineto; /* line to second point */
    DRAW_PATH_TERM  term;
};

_Check_return_
extern STATUS
gr_riscdiag_line_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pLineStart,
    _InRef_     PC_DRAW_POINT pPos,
    _InRef_     PC_DRAW_POINT pOffset,
    _InRef_     PC_GR_LINESTYLE linestyle)
{
    struct gr_riscdiag_line_guts Line;
    STATUS status;
    P_BYTE pPathGuts;

    Line.pos.tag        = DRAW_PATH_TYPE_MOVE;
    Line.pos.pt         = *pPos;

    Line.lineto.tag     = DRAW_PATH_TYPE_LINE;
    Line.lineto.pt.x    = Line.pos.pt.x + pOffset->x;
    Line.lineto.pt.y    = Line.pos.pt.y + pOffset->y;

    Line.term.tag       = DRAW_PATH_TYPE_TERM;

    if(NULL != (pPathGuts = gr_riscdiag_path_new(p_gr_riscdiag, pLineStart, linestyle, NULL, NULL, sizeof32(Line), &status)))
        memcpy32(pPathGuts, &Line, sizeof32(Line));

    return(status);
}

/******************************************************************************
*
* pie sector object
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_piesector_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pPieStart,
    _InRef_     PC_DRAW_POINT pPos,
    _InVal_     DRAW_COORD radius,
    _InVal_     F64 alpha,
    _InVal_     F64 beta,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec)
{
    DRAW_POINT bezCentre = *pPos;
    DRAW_COORD bezRadius = radius;
    DRAW_POINT bezStart, bezEnd, bezCP1, bezCP2;
    U32 n_segments, segment_id;
    STATUS status;
    P_BYTE pPathGuts;

    if(bezRadius <= 0)
    {
        *pPieStart = DRAW_DIAG_OFFSET_NONE;
        return(STATUS_OK);
    }

    n_segments = bezier_arc_start(&bezCentre, bezRadius, alpha, beta, &bezStart, &bezEnd, &bezCP1, &bezCP2);

    /* NB. variable number of segments precludes fixed structure */
    if(NULL == (pPathGuts =
        gr_riscdiag_path_new(p_gr_riscdiag, pPieStart,
                             linestyle, fillstylec, NULL,
                             sizeof32(DRAW_PATH_MOVE)  +
                             sizeof32(DRAW_PATH_LINE)  +
                             (sizeof32(DRAW_PATH_CURVE) * n_segments) +
                             sizeof32(DRAW_PATH_LINE)  +
                             sizeof32(DRAW_PATH_CLOSE) +
                             sizeof32(DRAW_PATH_TERM),
                             &status)))
        return(status);

    segment_id = 1;

    {
    DRAW_PATH_MOVE move;
    move.tag = DRAW_PATH_TYPE_MOVE;
    move.pt  = bezCentre;
    memcpy32(pPathGuts, &move, sizeof32(move));
    pPathGuts += sizeof32(move);
    } /*block*/

    {
    DRAW_PATH_LINE line;
    line.tag = DRAW_PATH_TYPE_LINE;
    line.pt  = bezStart;
    memcpy32(pPathGuts, &line, sizeof32(line));
    pPathGuts += sizeof32(line);
    } /*block*/

    /* accumulate curvy bits */
    do  {
        DRAW_PATH_CURVE curve;
        curve.tag = DRAW_PATH_TYPE_CURVE;
        curve.cp1 = bezCP1;
        curve.cp2 = bezCP2;
        curve.end = bezEnd;
        memcpy32(pPathGuts, &curve, sizeof32(curve));
        pPathGuts += sizeof32(curve);
    }
    while((segment_id = bezier_arc_segment(segment_id, &bezEnd, &bezCP1, &bezCP2)) != 0);

    {
    DRAW_PATH_LINE line;
    line.tag = DRAW_PATH_TYPE_LINE;
    line.pt  = bezCentre;
    memcpy32(pPathGuts, &line, sizeof32(line));
    pPathGuts += sizeof32(line);
    } /*block*/

    * (P_U32) pPathGuts = DRAW_PATH_TYPE_CLOSE_WITH_LINE; /* ok to poke single words */
    pPathGuts += sizeof32(DRAW_PATH_CLOSE);

    * (P_U32) pPathGuts = DRAW_PATH_TYPE_TERM;

    return(STATUS_OK);
}

/******************************************************************************
*
* quadrilateral object
*
******************************************************************************/

struct gr_riscdiag_quadrilateral_guts
{
    DRAW_PATH_MOVE  pos;    /* move to first point  */
    DRAW_PATH_LINE  second; /* line to second point */
    DRAW_PATH_LINE  third;  /* line to third point  */
    DRAW_PATH_LINE  fourth; /* line to fourth point */
#if WINDOWS
    DRAW_PATH_LINE  again;  /* line to first point  */
#endif
    DRAW_PATH_CLOSE close;
    DRAW_PATH_TERM  term;
};

_Check_return_
extern STATUS
gr_riscdiag_quadrilateral_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pQuadStart,
    _InRef_     PC_DRAW_POINT pPos,
    _InRef_     PC_DRAW_POINT pOffset1,
    _InRef_     PC_DRAW_POINT pOffset2,
    _InRef_     PC_DRAW_POINT pOffset3,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec)
{
    DRAW_PATH_STYLE pathstyle;
    struct gr_riscdiag_quadrilateral_guts Quad;
    STATUS status;
    P_BYTE pPathGuts;

    zero_struct(pathstyle);

    pathstyle.flags |= DRAW_PS_JOIN_ROUND;

#if WINDOWS
    {
    PC_DRAW_DASH_HEADER dash_pattern = gr_linestyle_to_riscDraw(linestyle);

    if(NULL == dash_pattern)
    {
        pathstyle.flags |= (
            (DRAW_PS_CAP_ROUND << DRAW_PS_ENDCAP_PACK_SHIFT  ) |
            (DRAW_PS_CAP_ROUND << DRAW_PS_STARTCAP_PACK_SHIFT) );
    }
    } /*block*/
#endif

    Quad.pos.tag        = DRAW_PATH_TYPE_MOVE;
    Quad.pos.pt         = *pPos;

    Quad.second.tag     = DRAW_PATH_TYPE_LINE;
    Quad.second.pt.x    = Quad.pos.pt.x + pOffset1->x;
    Quad.second.pt.y    = Quad.pos.pt.y + pOffset1->y;

    Quad.third.tag      = DRAW_PATH_TYPE_LINE;
    Quad.third.pt.x     = Quad.pos.pt.x + pOffset2->x;
    Quad.third.pt.y     = Quad.pos.pt.y + pOffset2->y;

    Quad.fourth.tag     = DRAW_PATH_TYPE_LINE;
    Quad.fourth.pt.x    = Quad.pos.pt.x + pOffset3->x;
    Quad.fourth.pt.y    = Quad.pos.pt.y + pOffset3->y;

#if WINDOWS
    Quad.again.tag      = DRAW_PATH_TYPE_LINE;
    Quad.again.pt       = *pPos;
#endif

    Quad.close.tag      = DRAW_PATH_TYPE_CLOSE_WITH_LINE;

    Quad.term.tag       = DRAW_PATH_TYPE_TERM;

    if(NULL != (pPathGuts = gr_riscdiag_path_new(p_gr_riscdiag, pQuadStart, linestyle, fillstylec, &pathstyle, sizeof32(Quad), &status)))
        memcpy32(pPathGuts, &Quad, sizeof32(Quad));

    return(status);
}

/******************************************************************************
*
* rectangle object
*
******************************************************************************/

struct gr_riscdiag_rectangle_guts
{
    DRAW_PATH_MOVE  bl;  /* move to bottom left  */
    DRAW_PATH_LINE  br;  /* line to bottom right */
    DRAW_PATH_LINE  tr;  /* line to top left     */
    DRAW_PATH_LINE  tl;  /* line to top right    */
#if WINDOWS
    DRAW_PATH_LINE  again;  /* line to first point  */
#endif
    DRAW_PATH_CLOSE close;
    DRAW_PATH_TERM  term;
};

_Check_return_
extern STATUS
gr_riscdiag_rectangle_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pRectStart,
    _InRef_     PC_DRAW_POINT pPos,
    _InRef_     PC_DRAW_SIZE pSize,
    _InRef_opt_ PC_GR_LINESTYLE linestyle,
    _InRef_opt_ PC_GR_FILLSTYLEC fillstylec)
{
    DRAW_PATH_STYLE pathstyle;
    struct gr_riscdiag_rectangle_guts Rect;
    STATUS status;
    P_BYTE pPathGuts;

    zero_struct(pathstyle);

#if WINDOWS
    pathstyle.flags |= DRAW_PS_JOIN_MITRED;

    {
    PC_DRAW_DASH_HEADER dash_pattern = gr_linestyle_to_riscDraw(linestyle);

    if(NULL == dash_pattern)
    {
        pathstyle.flags |= (
            (DRAW_PS_CAP_BUTT   << DRAW_PS_ENDCAP_PACK_SHIFT  ) |
            (DRAW_PS_CAP_SQUARE << DRAW_PS_STARTCAP_PACK_SHIFT) );
    }
    } /*block*/
#endif

    Rect.bl.tag    = DRAW_PATH_TYPE_MOVE;
    Rect.bl.pt.x   = pPos->x;
    Rect.bl.pt.y   = pPos->y;

    Rect.br.tag    = DRAW_PATH_TYPE_LINE;
    Rect.br.pt.x   = Rect.bl.pt.x + pSize->cx;
    Rect.br.pt.y   = Rect.bl.pt.y;

    Rect.tr.tag    = DRAW_PATH_TYPE_LINE;
    Rect.tr.pt.x   = Rect.br.pt.x;
    Rect.tr.pt.y   = Rect.br.pt.y + pSize->cy;

    Rect.tl.tag    = DRAW_PATH_TYPE_LINE;
    Rect.tl.pt.x   = Rect.bl.pt.x;
    Rect.tl.pt.y   = Rect.tr.pt.y;

#if WINDOWS
    /* horrific bodge needed due to Draw DLLs not rendering closing line with correct style */
    Rect.again.tag  = DRAW_PATH_TYPE_LINE;
    Rect.again.pt.x = pPos->x;
    Rect.again.pt.y = pPos->y;
#endif

    Rect.close.tag = DRAW_PATH_TYPE_CLOSE_WITH_LINE;

    Rect.term.tag  = DRAW_PATH_TYPE_TERM;

    if(NULL != (pPathGuts = gr_riscdiag_path_new(p_gr_riscdiag, pRectStart, linestyle, fillstylec, &pathstyle, sizeof32(Rect), &status)))
        memcpy32(pPathGuts, &Rect, sizeof32(Rect));

    return(status);
}

/******************************************************************************
*
* string object
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_string_new_sbchars(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pTextStart,
    _InRef_     PC_DRAW_POINT point,
    _In_reads_(sbchars_n) PC_SBCHARS sbchars,
    _InVal_     U32 sbchars_n,
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle,
    _InVal_     GR_COLOUR fg,
    _In_opt_    const GR_COLOUR * const bg,
    _InRef_     PC_GR_RISCDIAG lookup_gr_riscdiag)
{
    GR_PIXIT fsize_y = p_gr_textstyle->size_y;
    GR_PIXIT fsize_x = p_gr_textstyle->size_x; /* may be zero */
    U32 size = sbchars_n + 1 /*CH_NULL*/;
    DRAW_OBJECT_TEXT text;
    P_BYTE pObject;
    DRAW_FONT_REF16 fontRef;
    PIXIT_POINT fsize_mp;
    STATUS status;

    text.type = DRAW_OBJECT_TYPE_TEXT;
    text.size = sizeof32(text) + round_up(size, 4); /* round up to output word boundary */
    draw_box_make_bad(&text.bbox);

    text.textcolour = gr_colour_to_riscDraw(fg);

    if(bg && bg->visible)
        text.background = gr_colour_to_riscDraw(*bg); /* hint colour */
    else
        text.background = (DRAW_COLOUR) 0xFFFFFF00U; /* hint is white if unspecified */

    /* search for the text style using lookup_gr_riscdiag */
    fontRef = gr_riscdiag_fontlist_lookup_textstyle(lookup_gr_riscdiag, lookup_gr_riscdiag->dd_fontListR, lookup_gr_riscdiag->dd_fontListW, p_gr_textstyle);

    /* blat reserved fields in this 32-bit structure */
    * (P_U32) &text.textstyle = (U32) fontRef;

    /* NB. NOT DRAW UNITS!!! --- 1/640 point */
    fsize_mp.y = gr_mp_from_pixit(fsize_y);
    text.fsize_y = draw_fontsize_from_mp(fsize_mp.y);

    if(0 == fsize_x)
    { /* -> same as y */
        fsize_mp.x = fsize_mp.y;
        text.fsize_x = text.fsize_y;
    }
    else
    {
        fsize_mp.x = gr_mp_from_pixit(fsize_x);
        text.fsize_x = draw_fontsize_from_mp(fsize_mp.x);
    }

    /* baseline origin coords */
    text.coord = *point;

    if(fontRef == 0)
        /* Draw rendering positions System font strangely: quickly correct baseline */
        text.coord.y -= (gr_riscDraw_from_pixit(fsize_y) / 8);

    *pTextStart = gr_riscdiag_query_offset(p_gr_riscdiag);

    if(NULL != (pObject = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, text.size, &status)))
    {
        memcpy32(pObject, &text, sizeof32(text));

        /* SKS 01.07.01 only copy the amount requested! */
        memcpy32(pObject + sizeof32(text), sbchars, sbchars_n);
        PtrPutByteOff(pObject, sizeof32(text) + sbchars_n, CH_NULL); /* needs explicit termination */
    }

    return(status);
}

_Check_return_
extern STATUS
gr_riscdiag_string_new_uchars(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pTextStart,
    _InRef_     PC_DRAW_POINT point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle,
    _InVal_     GR_COLOUR fg,
    _In_opt_    const GR_COLOUR * const bg,
    _InRef_     PC_GR_RISCDIAG lookup_gr_riscdiag)
{
#if USTR_IS_SBSTR
    return(gr_riscdiag_string_new_sbchars(p_gr_riscdiag, pTextStart, point, uchars, uchars_n, p_gr_textstyle, fg, bg, lookup_gr_riscdiag));
#else
    GR_PIXIT fsize_y = p_gr_textstyle->size_y;
    GR_PIXIT fsize_x = p_gr_textstyle->size_x; /* may be zero */
    BOOL is_pure_ascii7;
    U32 sbchars_n = sbchars_from_utf8_bytes_needed(uchars, uchars_n, &is_pure_ascii7);
    U32 size = sbchars_n + 1 /*CH_NULL*/;
    DRAW_OBJECT_TEXT text;
    P_BYTE pObject;
    DRAW_FONT_REF16 fontRef;
    PIXIT_POINT fsize_mp;
    STATUS status;

    text.type = DRAW_OBJECT_TYPE_TEXT;
    text.size = sizeof32(text) + round_up(size, 4); /* round up to output word boundary */
    draw_box_make_bad(&text.bbox);

    text.textcolour = gr_colour_to_riscDraw(fg);

    if(bg && bg->visible)
        text.background = gr_colour_to_riscDraw(*bg); /* hint colour */
    else
        text.background = (DRAW_COLOUR) 0xFFFFFF00U; /* hint is white if unspecified */

    /* search for the text style using lookup_gr_riscdiag */
    fontRef = gr_riscdiag_fontlist_lookup_textstyle(lookup_gr_riscdiag, lookup_gr_riscdiag->dd_fontListR, lookup_gr_riscdiag->dd_fontListW, p_gr_textstyle);

    /* blat reserved fields in this 32-bit structure */
    * (P_U32) &text.textstyle = (U32) fontRef;

    /* NB. NOT DRAW UNITS!!! --- 1/640 point */
    fsize_mp.y = gr_mp_from_pixit(fsize_y);
    text.fsize_y = draw_fontsize_from_mp(fsize_mp.y);

    if(0 == fsize_x)
    { /* -> same as y */
        fsize_mp.x = fsize_mp.y;
        text.fsize_x = text.fsize_y;
    }
    else
    {
        fsize_mp.x = gr_mp_from_pixit(fsize_x);
        text.fsize_x = draw_fontsize_from_mp(fsize_mp.x);
    }

    /* baseline origin coords */
    text.coord = *point;

    if(fontRef == 0)
        /* Draw rendering positions System font strangely: quickly correct baseline */
        text.coord.y -= (gr_riscDraw_from_pixit(fsize_y) / 8);

    *pTextStart = gr_riscdiag_query_offset(p_gr_riscdiag);

    if(NULL != (pObject = gr_riscdiag_ensure(BYTE, p_gr_riscdiag, text.size, &status)))
    {
        memcpy32(pObject, &text, sizeof32(text));

        if(is_pure_ascii7)
        {
            assert(sbchars_n == uchars_n);
            memcpy32(PtrAddBytes(P_SBCHARS, pObject, sizeof32(text)), uchars, uchars_n);
        }
        else
        {
            bool_assert(sbchars_n ==
                sbchars_from_utf8(PtrAddBytes(P_SBCHARS, pObject, sizeof32(text)), sbchars_n, get_system_codepage(), uchars, uchars_n));
        }

        PtrPutByteOff(pObject, sizeof32(text) + sbchars_n, CH_NULL); /* needs explicit termination */
    }

    return(status);
#endif /* USTR_IS_SBSTR */
}

/******************************************************************************
*
* put a scaled copy of the contents of a diagram in as a group in the Draw file.
* aren't font list objects a pain in the ass? Also note that text column objects
* and tagged objects can NOT be scaled at this point
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_scaled_diagram_add(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pPictStart,
    _InRef_     PC_DRAW_BOX pBox,
    _In_reads_(diag_len) PC_BYTE p_diag,
    _InVal_     U32 diag_len,
    _InRef_     PC_GR_FILLSTYLEB fillstyleb,
    _InRef_opt_ PC_GR_FILLSTYLEC fillstylec,
    _InRef_opt_ PC_GR_RISCDIAG p_gr_riscdiag_lookup)
{
    GR_RISCDIAG source_gr_riscdiag;
    U32 diagLength = diag_len;
    U32 total_skip_size;
    DRAW_DIAG_OFFSET diagStart, groupStart;
    DRAW_DIAG_OFFSET thisObject, endObject;
    STATUS status;
    P_BYTE pObject;
    DRAW_POINT init_posn, init_size, posn, size;
    GR_SCALE_PAIR simple_scale;
    GR_SCALE simple_scaling;
    GR_XFORMMATRIX  scale_xform;
    BOOL isotropic =  fillstyleb->bits.isotropic;
    BOOL recolour  = !fillstyleb->bits.norecolour;

    trace_8(0, TEXT("gr_riscdiag_scaled_diagram_add(") PTR_XTFMT TEXT(", (") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT("), (") PTR_XTFMT TEXT(",") U32_TFMT TEXT("), iso=") S32_TFMT TEXT(")"),
        p_gr_riscdiag, pBox->x0, pBox->y0, pBox->x1, pBox->y1, p_diag, diag_len, isotropic);

    if(NULL == p_diag)
        return(STATUS_FAIL);

    if(diagLength <= sizeof32(DRAW_FILE_HEADER))
        return(status_check());

    status_return(gr_riscdiag_group_new(p_gr_riscdiag, &groupStart, "DrawDiagCopy"));

    /* note offset of where diagram body (skip file header and font table) is to be copied */
    diagStart = gr_riscdiag_query_offset(p_gr_riscdiag);

    /* scan the diagram to be copied for font tables - no need to set hglobal */
    gr_riscdiag_diagram_setup_from_data(&source_gr_riscdiag, p_diag, diag_len);

    { /* never put the file header, font lists or RISC OS 3 rubbish object in our diagram */
    P_U8 pDiagCopy = NULL; /* keep dataflower happy */
    UINT pass;

    total_skip_size = sizeof32(DRAW_FILE_HEADER);

    for(pass = 1; pass <= 2; ++pass)
    {
        DRAW_DIAG_OFFSET sttObject = gr_riscdiag_normalise_stt(&source_gr_riscdiag, DRAW_DIAG_OFFSET_FIRST);
        DRAW_DIAG_OFFSET endObject = gr_riscdiag_normalise_end(&source_gr_riscdiag, DRAW_DIAG_OFFSET_LAST);
        DRAW_DIAG_OFFSET s = sttObject; /* after normalisation */
        P_BYTE pObject;

        if(gr_riscdiag_object_first(&source_gr_riscdiag, &sttObject, &endObject, &pObject, FALSE)) /* flat scan good enough for what I want */
        {
            do {
                switch(*DRAW_OBJHDR(U32, pObject, type))
                {
                /* these skipped objects can't be grouped (which they never are) otherwise the group header(s) would need patching too! */
                case DRAW_OBJECT_TYPE_FONTLIST:
                    source_gr_riscdiag.dd_fontListR = sttObject;
/*reportf(TEXT("sda: ") PTR_XTFMT TEXT(" fontListR at %d"), &source_gr_riscdiag, sttObject);*/
                    goto skip_object;

                case DRAW_OBJECT_TYPE_DS_WINFONTLIST:
                    source_gr_riscdiag.dd_fontListW = sttObject;
/*reportf(TEXT("sda: ") PTR_XTFMT TEXT(" fontListW at %d"), &source_gr_riscdiag, sttObject);*/
                    goto skip_object;

                case DRAW_OBJECT_TYPE_OPTIONS:
                    source_gr_riscdiag.dd_options = sttObject;
/*reportf(TEXT("sda: ") PTR_XTFMT TEXT(" options at %d"), &source_gr_riscdiag, sttObject);*/
                skip_object:;
                    if(pass == 1)
                        total_skip_size += *DRAW_OBJHDR(U32, pObject, size);
                    else
                    {
                        /* flush out whatever we had so far */
                        U32 n_bytes = sttObject - s;
                        if(n_bytes)
                        {
                            memcpy32(pDiagCopy, gr_riscdiag_getoffptr(BYTE, &source_gr_riscdiag, s), n_bytes);
                            pDiagCopy += n_bytes;
                        }
                        s = sttObject + *DRAW_OBJHDR(U32, pObject, size); /* next object to be output starts here */
                    }
                    break;

                default:
                    break;
                }
            }
            while(gr_riscdiag_object_next(&source_gr_riscdiag, &sttObject, &endObject, &pObject, FALSE));

            if(pass == 1)
            {
                U32 n_bytes = diagLength - total_skip_size;
                if(0 == n_bytes)
                    break;
                if(NULL == (pDiagCopy = gr_riscdiag_ensure(U8, p_gr_riscdiag, n_bytes, &status)))
                    return(status);
            }
            else
            {
                /* flush out end objects */
                U32 n_bytes = diagLength - s;
                if(n_bytes)
                    memcpy32(pDiagCopy, gr_riscdiag_getoffptr(BYTE, &source_gr_riscdiag, s), n_bytes);
            }
        }
    }
    } /*block*/

    /* can now close the group */
    gr_riscdiag_group_end(p_gr_riscdiag, groupStart);

    /* run over all appropriate objects shifting and scaling */

    {
    PC_DRAW_FILE_HEADER pDrawFileHdr = (PC_DRAW_FILE_HEADER) p_diag;
    init_posn.x = pDrawFileHdr->bbox.x0;
    init_posn.y = pDrawFileHdr->bbox.y0;
    init_size.x = pDrawFileHdr->bbox.x1 - init_posn.x;
    init_size.y = pDrawFileHdr->bbox.y1 - init_posn.y;
    /*reportf("init_posn = %d,%d; init_size = %d,%d", init_posn.x, init_posn.y, init_size.x, init_size.y);*/
    } /*block*/

    posn.x = pBox->x0;
    posn.y = pBox->y0;
    size.x = pBox->x1 - posn.x;
    size.y = pBox->y1 - posn.y;

    if(isotropic)
    {
        /* make box square and reposition to centre */
        if(size.x > size.y)
        {
            posn.x += (size.x - size.y) / 2;
            size.x = size.y;
        }
        else if(size.x < size.y)
        {
            posn.x += (size.y - size.x) / 2;
            size.y = size.x;
        }
    }

    simple_scale.x = gr_scale_from_s32_pair(size.x, init_size.x);
    simple_scale.y = gr_scale_from_s32_pair(size.y, init_size.y);

    /* simple_scaling is (for want of a better value) the geometric mean of x and y scales */
    if(simple_scale.x == simple_scale.y)
        simple_scaling = simple_scale.x;
    else
        simple_scaling = gr_scale_from_f64(sqrt(gr_f64_from_scale(simple_scale.x) * gr_f64_from_scale(simple_scale.y)));

    /* now I have sufficient confidence, I do this as a combination xform: */
    /* i)   translation to get old origin at (0,0) */
    /* ii)  scaling about (0,0) */
    /* iii) translation to new origin */
    {
    GR_XFORMMATRIX temp_xform;
    static const GR_POINT origin = { 0, 0 };

    /* i)   translation to get old origin at (0,0) */
    init_posn.x = -init_posn.x;
    init_posn.y = -init_posn.y;
    gr_xform_make_translation(&temp_xform, (PC_GR_POINT) &init_posn);
    init_posn.x = -init_posn.x;
    init_posn.y = -init_posn.y;

    /* ii)  scaling about (0,0) */
    gr_xform_make_scale(&scale_xform, &origin, simple_scale.x, simple_scale.y);

    gr_xform_make_combination(&scale_xform, &temp_xform, &scale_xform);

    /* iii) translation to new origin */
    gr_xform_make_translation(&temp_xform, (PC_GR_POINT) &posn);

    gr_xform_make_combination(&scale_xform, &scale_xform, &temp_xform);
    } /*block*/

    thisObject = diagStart; /* may as well miss out the group header again! */
    endObject  = diagStart + (diagLength - total_skip_size);

    if(gr_riscdiag_object_first(p_gr_riscdiag, &thisObject, &endObject, &pObject, TRUE))
    {
        do  {
            /* note that there are no awkward font table or RO3 DRAW_OBJECT_TYPE_OPTIONS etc. in the copy */
            switch(*DRAW_OBJHDR(U32, pObject, type))
            {
            case DRAW_OBJECT_TYPE_TEXT:
                {
                DRAW_OBJECT_TEXT text;
                DRAW_DIAG_OFFSET foundOffsetR, foundOffsetW;

                memcpy32(&text, pObject, sizeof32(text));

                if(0 != text.textstyle.fontref16)
                {
/*reportf(TEXT("sda: old fr %d"), text.textstyle.fontref16);*/
                gr_riscdiag_fontlist_lookup_fontref(&source_gr_riscdiag, source_gr_riscdiag.dd_fontListR, source_gr_riscdiag.dd_fontListW, &foundOffsetR, &foundOffsetW, text.textstyle.fontref16);
/*reportf(TEXT("sda: old fr %d got %d,%d"), text.textstyle.fontref16, foundOffsetR, foundOffsetW);*/

                {
                PC_DRAW_FONTLIST_ELEM pFontListElemR_source = foundOffsetR ? gr_riscdiag_getoffptr(DRAW_FONTLIST_ELEM, &source_gr_riscdiag, foundOffsetR) : NULL;
                PC_DRAW_DS_WINFONTLIST_ELEM pFontListElemW_source = foundOffsetW ? gr_riscdiag_getoffptr(DRAW_DS_WINFONTLIST_ELEM, &source_gr_riscdiag, foundOffsetW) : NULL;
                if(NULL != p_gr_riscdiag_lookup)
                    text.textstyle.fontref16 = gr_riscdiag_fontlist_lookup_direct(p_gr_riscdiag_lookup, p_gr_riscdiag_lookup->dd_fontListR, p_gr_riscdiag_lookup->dd_fontListW, pFontListElemR_source, pFontListElemW_source);
                else
                    text.textstyle.fontref16 = gr_riscdiag_fontlist_lookup_direct(p_gr_riscdiag, p_gr_riscdiag->dd_fontListR, p_gr_riscdiag->dd_fontListW, pFontListElemR_source, pFontListElemW_source);
/*reportf(TEXT("sda: new fr %d"), text.textstyle.fontref16);*/
                if(0 == text.textstyle.fontref16)
                    text.textstyle.fontref16 = 1; /* bodge if not mapped */
                } /*block*/
                }

                /* note that we can't cope here with the destination diagram's font table growing as
                 * that'd bugger up all the sys_offs we've placed in the diagram! so if it uses
                 * a font that ain't used in any real text object then they've had it. so there.
                 * it's up to the caller to pre-populate the destination diagram's font table
                 * or supply a lookup font table that'll be inserted before export.
                */

                /* shift and scale and shift baseline origin */
                draw_point_xform(&text.coord, &text.coord, &scale_xform);

                /* can scale both width and height */
                text.fsize_x = gr_coord_scale(text.fsize_x, simple_scale.x);
                text.fsize_y = gr_coord_scale(text.fsize_y, simple_scale.y);

                memcpy32(pObject, &text, sizeof32(text));

                break;
                }

            case DRAW_OBJECT_TYPE_PATH:
                {
                DRAW_OBJECT_PATH path;
                P_BYTE p_path;

                memcpy32(&path, pObject, sizeof32(path));

                /* simply scale the line width if not Thin */
                if(path.pathwidth != 0)
                    path.pathwidth = gr_coord_scale(path.pathwidth, simple_scaling);

                if(recolour)
                {
                    DRAW_COLOUR colour = gr_colour_to_riscDraw(fillstylec->fg);

                    recolour = 0;

                    /* set both stroke and fill */
                    path.fillcolour = colour;
                    path.pathcolour = colour;
                }

                /* path ordinarily starts here */
                p_path = pObject + sizeof32(path);

                /* simply scale the dash lengths and start offset if present */
                if(path.pathstyle.flags & DRAW_PS_DASH_PACK_MASK)
                {
                    const S32 dashcount = ((PC_DRAW_DASH_HEADER) p_path)->dashcount;
                    S32 i;
                    P_S32 p_s32;

                    p_s32 = (P_S32) &((P_DRAW_DASH_HEADER) p_path)->dashstart;
                    *p_s32 = gr_coord_scale(*p_s32, simple_scaling);

                    for(i = 0; i < dashcount; ++i)
                    {
                        p_s32 = PtrAddBytes(P_S32, p_path, sizeof32(DRAW_DASH_HEADER) + sizeof32(S32) * i);
                        *p_s32 = gr_coord_scale(*p_s32, simple_scaling);
                    }

                    /* skip dash header and pattern */
                    p_path += sizeof32(DRAW_DASH_HEADER) + sizeof32(S32) * dashcount;
                }

                /* shift and scale the path coordinates */

                while(* (P_U32) p_path != DRAW_PATH_TYPE_TERM)
                    switch(* (P_U32) p_path)
                    {
                    case DRAW_PATH_TYPE_MOVE:
                    case DRAW_PATH_TYPE_LINE:
                        {
                        /* shift and scale and shift */
                        DRAW_PATH_LINE line;

                        memcpy32(&line, p_path, sizeof32(line));

                        draw_point_xform(&line.pt, &line.pt, &scale_xform);

                        memcpy32(p_path, &line, sizeof32(line));

                        p_path += sizeof32(line);
                        break;
                        }

                    case DRAW_PATH_TYPE_CURVE:
                        {
                        /* shift and scale and shift */
                        DRAW_PATH_CURVE curve;

                        memcpy32(&curve, p_path, sizeof32(curve));

                        draw_point_xform(&curve.cp1, &curve.cp1, &scale_xform);
                        draw_point_xform(&curve.cp2, &curve.cp2, &scale_xform);
                        draw_point_xform(&curve.end, &curve.end, &scale_xform);

                        memcpy32(p_path, &curve, sizeof32(curve));

                        p_path += sizeof32(curve);
                        break;
                        }

                    default:
#if CHECKING
                        myassert1(TEXT("unknown path object tag ") U32_TFMT, * (P_U32) p_path);

                        /*FALLTHRU*/

                    case DRAW_PATH_TYPE_CLOSE_WITH_LINE:
#endif
                        p_path += sizeof32(DRAW_PATH_CLOSE);
                        break;
                    }

                memcpy32(pObject, &path, sizeof32(path));

                break;
                }

            case DRAW_OBJECT_TYPE_JPEG:
                {
                DRAW_OBJECT_JPEG jpeg;

                memcpy32(&jpeg, pObject, sizeof32(jpeg));

                /* shift and scale and shift bbox (x1,y1 will need recomputing) */
                draw_box_xform(&jpeg.bbox, &jpeg.bbox, &scale_xform);

                /* scale both width and height */
                jpeg.width  = gr_coord_scale(jpeg.width,  simple_scale.x);
                jpeg.height = gr_coord_scale(jpeg.height, simple_scale.y);

                /* scale the transform matrix */
                jpeg.trfm.a = gr_coord_scale(jpeg.trfm.a, simple_scale.x);
                jpeg.trfm.b = 0; /*gr_coord_scale(jpeg.trfm.b, simple_scale.x);*/ /* only handles upright images */
                jpeg.trfm.c = 0; /*gr_coord_scale(jpeg.trfm.c, simple_scale.y);*/
                jpeg.trfm.d = gr_coord_scale(jpeg.trfm.d, simple_scale.y);

                /* update the translation component of the transform matrix to position this instance */
                jpeg.trfm.e = jpeg.bbox.x0;
                jpeg.trfm.f = jpeg.bbox.y0;

                memcpy32(pObject, &jpeg, sizeof32(jpeg));

                break;
                }

            case DRAW_OBJECT_TYPE_SPRITE:
            case DRAW_OBJECT_TYPE_TRFMSPRITE:
            case DRAW_OBJECT_TYPE_TRFMTEXT:
            case DRAW_OBJECT_TYPE_DS_DIB:
            case DRAW_OBJECT_TYPE_DS_DIBROT:
                {
                /* the bbox of these objects determines the contents format, not vice versa! */
                DRAW_OBJECT_HEADER objhdr;

                memcpy32(&objhdr, pObject, sizeof32(objhdr));

                /* shift and scale and shift bbox */
                draw_box_xform(&objhdr.bbox, &objhdr.bbox, &scale_xform);

                memcpy32(pObject, &objhdr, sizeof32(objhdr));

                break;
                }

            default:
                break;
            }
        }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &thisObject, &endObject, &pObject, TRUE));
    }

    { /* ensure this group recomputed and rebound */
    DRAW_BOX group_box;
    GR_RISCDIAG_PROCESS_T process;
    * (int *) &process = 0;
    process.recurse = 1;
    process.recompute = 1;
    gr_riscdiag_object_reset_bbox_between(p_gr_riscdiag, &group_box, /* just a dummy here, this reset_bbox is going to fix the encapsulator too */
                                          groupStart, groupStart + sizeof32(DRAW_OBJECT_GROUP) + diagLength - total_skip_size, process);
    } /*block*/

    /* now client's responsibility to ensure whole diagram simply
     * rebound after (or it happens on diagram end anyway)
    */

    /* not forgetting to tell client where we put it! */
    *pPictStart = groupStart;

    return(STATUS_OK);
}

#if defined(UNUSED_KEEP_ALIVE)

/******************************************************************************
*
* scale the contents of a diagram.
*
* Note that text column objects and tagged objects can NOT be scaled at this point
*
******************************************************************************/

_Check_return_
extern STATUS
gr_riscdiag_scale_diagram(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InRef_     PC_GR_BOX pBox,
    _InRef_     PC_GR_FILLSTYLEB fillstyleb)
{
    DRAW_DIAG_OFFSET thisObject, endObject;
    P_BYTE pObject;
    GR_POINT initposn, initsize;
    GR_POINT posn, size;
    GR_SCALE_PAIR simple_scale;
    GR_SCALE simple_scaling;
    GR_XFORMMATRIX  scale_xform;
    BOOL isotropic =  fillstyleb->bits.isotropic;

    trace_6(0, TEXT("gr_riscdiag_scale_diagram(") PTR_XTFMT TEXT(", (") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT("), iso=") S32_TFMT TEXT(")"),
        p_gr_riscdiag, pBox->x0, pBox->y0, pBox->x1, pBox->y1, isotropic);

    /* run over all appropriate objects shifting and scaling */

    {
    PC_DRAW_FILE_HEADER pDrawFileHdr = gr_riscdiag_getoffptr(DRAW_FILE_HEADER, p_gr_riscdiag, 0);
    initposn.x = pDrawFileHdr->bbox.x0;
    initposn.y = pDrawFileHdr->bbox.y0;
    initsize.x = pDrawFileHdr->bbox.x1 - initposn.x;
    initsize.y = pDrawFileHdr->bbox.y1 - initposn.y;
    } /*block*/

    posn.x = pBox->x0;
    posn.y = pBox->y0;
    size.x = pBox->x1 - posn.x;
    size.y = pBox->y1 - posn.y;

    if(isotropic)
    {
        /* make box square and reposition to centre */
        if(size.x > size.y)
        {
            posn.x += (size.x - size.y) / 2;
            size.x = size.y;
        }
        else if(size.x < size.y)
        {
            posn.x += (size.y - size.x) / 2;
            size.y = size.x;
        }
    }

    simple_scale.x = gr_scale_from_s32_pair(size.x, initsize.x);
    simple_scale.y = gr_scale_from_s32_pair(size.y, initsize.y);

    /* simple_scaling is (for want of a better value) the geometric mean of x and y scales */
    if(simple_scale.x == simple_scale.y)
        simple_scaling = simple_scale.x;
    else
        simple_scaling = gr_scale_from_f64(sqrt(gr_f64_from_scale(simple_scale.x) * gr_f64_from_scale(simple_scale.y)));

    /* now I have sufficient confidence, I do this as a comination xform: */
    /* i)   translation to get old origin at (0,0) */
    /* ii)  scaling about (0,0) */
    /* iii) translation to new origin */
    {
    GR_XFORMMATRIX temp_xform;
    static const GR_POINT origin = { 0, 0 };

    /* i)   translation to get old origin at (0,0) */
    initposn.x = -initposn.x;
    initposn.y = -initposn.y;
    gr_xform_make_translation(&temp_xform, &initposn);
    initposn.x = -initposn.x;
    initposn.y = -initposn.y;

    /* ii)  scaling about (0,0) */
    gr_xform_make_scale(&scale_xform, &origin, simple_scale.x, simple_scale.y);

    gr_xform_make_combination(&scale_xform, &temp_xform, &scale_xform);

    /* iii) translation to new origin */
    gr_xform_make_translation(&temp_xform, &posn);

    gr_xform_make_combination(&scale_xform, &scale_xform, &temp_xform);
    } /*block*/

    thisObject = DRAW_DIAG_OFFSET_FIRST;
    endObject  = DRAW_DIAG_OFFSET_LAST;

    if(gr_riscdiag_object_first(p_gr_riscdiag, &thisObject, &endObject, &pObject, TRUE))
    {
        do  {
            /* note that there is no awkward font table object now !*/
            switch(*DRAW_OBJHDR(U32, pObject, type))
            {
            case DRAW_OBJECT_TYPE_TEXT:
                {
                DRAW_OBJECT_TEXT text;

                memcpy32(&text, pObject, sizeof32(text));

                /* shift and scale and shift baseline origin */
                draw_point_xform(&text.coord, &text.coord, &scale_xform);

                /* can scale both width and height */
                text.fsize_x = gr_coord_scale(text.fsize_x, simple_scale.x);
                text.fsize_y = gr_coord_scale(text.fsize_y, simple_scale.y);

                memcpy32(pObject, &text, sizeof32(text));

                break;
                }

            case DRAW_OBJECT_TYPE_PATH:
                {
                DRAW_OBJECT_PATH path;
                P_BYTE p_path;

                memcpy32(&path, pObject, sizeof32(path));

                /* simply scale the line width if not Thin */
                if(path.pathwidth != 0)
                    path.pathwidth = gr_coord_scale(path.pathwidth, simple_scaling);

                /* path ordinarily starts here */
                p_path = pObject + sizeof32(path);

                /* simply scale the dash lengths and start offset if present */
                if(path.pathstyle.flags & DRAW_PS_DASH_PACK_MASK)
                {
                    const S32 dashcount = ((PC_DRAW_DASH_HEADER) p_path)->dashcount;
                    S32 i;
                    P_S32 p_s32;

                    p_s32 = (P_S32) &((P_DRAW_DASH_HEADER) p_path)->dashstart;
                    *p_s32 = gr_coord_scale(*p_s32, simple_scaling);

                    for(i = 0; i < dashcount; ++i)
                    {
                        p_s32 = PtrAddBytes(P_S32, p_path, sizeof32(DRAW_DASH_HEADER) + sizeof32(S32) * i);
                        *p_s32 = gr_coord_scale(*p_s32, simple_scaling);
                    }

                    /* skip dash header and pattern */
                    p_path += sizeof32(DRAW_DASH_HEADER) + sizeof32(S32) * dashcount;
                }

                /* shift and scale the path coordinates */

                while(* (P_U32) p_path != DRAW_PATH_TYPE_TERM)
                    switch(* (P_U32) p_path)
                    {
                    case DRAW_PATH_TYPE_MOVE:
                    case DRAW_PATH_TYPE_LINE:
                        {
                        /* shift and scale and shift */
                        DRAW_PATH_LINE line;

                        memcpy32(&line, p_path, sizeof32(line));

                        draw_point_xform(&line.pt, &line.pt, &scale_xform);

                        memcpy32(p_path, &line, sizeof32(line));

                        p_path += sizeof32(line);
                        break;
                        }

                    case DRAW_PATH_TYPE_CURVE:
                        {
                        /* shift and scale and shift */
                        DRAW_PATH_CURVE curve;

                        memcpy32(&curve, p_path, sizeof32(curve));

                        draw_point_xform(&curve.cp1, &curve.cp1, &scale_xform);
                        draw_point_xform(&curve.cp2, &curve.cp2, &scale_xform);
                        draw_point_xform(&curve.end, &curve.end, &scale_xform);

                        memcpy32(p_path, &curve, sizeof32(curve));

                        p_path += sizeof32(curve);
                        break;
                        }

                    default:
#if CHECKING
                        myassert1(TEXT("unknown path object tag ") U32_TFMT, * (P_U32) p_path);

                        /*FALLTHRU*/

                    case DRAW_PATH_TYPE_CLOSE_WITH_LINE:
#endif
                        p_path += sizeof32(DRAW_PATH_CLOSE);
                        break;
                    }

                memcpy32(pObject, &path, sizeof32(path));

                break;
                }

            case DRAW_OBJECT_TYPE_JPEG:
                break;

            case DRAW_OBJECT_TYPE_SPRITE:
            case DRAW_OBJECT_TYPE_TRFMSPRITE:
            case DRAW_OBJECT_TYPE_TRFMTEXT:
            case DRAW_OBJECT_TYPE_DS_DIB:
            case DRAW_OBJECT_TYPE_DS_DIBROT:
                {
                /* the bbox of these objects determines the contents format, not vice versa! */
                DRAW_OBJECT_HEADER objhdr;

                memcpy32(&objhdr, pObject, sizeof32(objhdr));

                /* shift and scale and shift bbox */
                draw_box_xform(&objhdr.bbox, &objhdr.bbox, &scale_xform);

                memcpy32(pObject, &objhdr, sizeof32(objhdr));

                break;
                }

            default:
                break;
            }
        }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &thisObject, &endObject, &pObject, TRUE));
    }

    /* now client's responsibility to ensure whole diagram bbox recalced */

    return(STATUS_OK);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* search diagram's font list object for given font, returning its fontref
*
******************************************************************************/

_Check_return_
extern DRAW_FONT_REF16
gr_riscdiag_fontlist_lookup_textstyle(
    _InRef_     PC_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET fontListR,
    _InVal_     DRAW_DIAG_OFFSET fontListW,
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle)
{
    DRAW_FONT_REF16 fontRefNum = 0; /* will have to use System font unless request matched */
    HOST_FONT_SPEC host_font_spec;
    STATUS status = gr_riscdiag_host_font_spec_riscos_from_textstyle(&host_font_spec, p_gr_textstyle);

    if(status_ok(status) && fontListR && array_elements(&host_font_spec.h_host_name_tstr))
    {
        const PC_SBSTR szHostFontName = _sbstr_from_tstr(array_tstr(&host_font_spec.h_host_name_tstr));
        const U32 HostFontName_lenp1 = strlen32p1(szHostFontName); /*CH_NULL*/
        P_DRAW_OBJECT_FONTLIST pFontListObject = gr_riscdiag_getoffptr(DRAW_OBJECT_FONTLIST, p_gr_riscdiag, fontListR);
        DRAW_DIAG_OFFSET nextObject = fontListR + pFontListObject->size;
        DRAW_DIAG_OFFSET thisOffset = fontListR + sizeof32(*pFontListObject);
        PC_DRAW_FONTLIST_ELEM pFontListElemR = gr_riscdiag_getoffptr(DRAW_FONTLIST_ELEM, p_gr_riscdiag, thisOffset);

/*reportf(TEXT("%s(") PTR_XTFMT TEXT(", %d, %d, %s)"), __Tfunc__, p_gr_riscdiag, fontListR, fontListW, report_sbstr(szHostFontName));*/
        /* actual end of RISC OS font list object data may not be word aligned */
        while((nextObject - thisOffset) >= 4)
        {
            const DRAW_DIAG_OFFSET lenp1 = strlen32p1(pFontListElemR->szHostFontName); /*CH_NULL*/
            const DRAW_DIAG_OFFSET thislen = offsetof32(DRAW_FONTLIST_ELEM, szHostFontName) + lenp1;

/*reportf(TEXT("fontListR %d, %s"), pFontListElemR->fontref8, report_sbstr(pFontListElemR->szHostFontName));*/
            if((lenp1 == HostFontName_lenp1) && (0 == C_stricmp(pFontListElemR->szHostFontName, szHostFontName)))
            {
                fontRefNum = (DRAW_FONT_REF16) pFontListElemR->fontref8;
                break;
            }

            thisOffset += thislen;

            pFontListElemR = PtrAddBytes(PC_DRAW_FONTLIST_ELEM, pFontListElemR, thislen);
        }
    }

    if(status_ok(status) && fontListW && (0 == fontRefNum) && array_elements(&host_font_spec.h_host_name_tstr))
    {
        const PC_SBSTR szHostFontName = _sbstr_from_tstr(array_tstr(&host_font_spec.h_host_name_tstr));
        DRAW_DS_WINDOWS_LOGFONT draw_ds_windows_logfont;
        P_DRAW_OBJECT_HEADER pFontListObject;
        DRAW_DIAG_OFFSET nextObject, thisOffset;

        draw_ds_windows_logfont_from_textstyle(&draw_ds_windows_logfont, p_gr_textstyle, szHostFontName);

        pFontListObject = gr_riscdiag_getoffptr(DRAW_OBJECT_HEADER, p_gr_riscdiag, fontListW);

        nextObject = fontListW + pFontListObject->size;
        thisOffset = fontListW + sizeof32(*pFontListObject);

        /* actual end of Windows font list object data may not be word aligned (in this case, paranoia) */
        while((nextObject - thisOffset) >= 4)
        {
            PC_DRAW_DS_WINFONTLIST_ELEM pFontListElemW = gr_riscdiag_getoffptr(DRAW_DS_WINFONTLIST_ELEM, p_gr_riscdiag, thisOffset);

/*reportf(TEXT("fontListW %d, %s"), pFontListElemW->draw_font_ref16, report_sbstr(pFontListElemW->draw_ds_windows_logfont.lfFaceName));*/
            if(0 == draw_ds_windows_logfont_compare(&pFontListElemW->draw_ds_windows_logfont, &draw_ds_windows_logfont))
            {
                fontRefNum = pFontListElemW->draw_font_ref16;
                break;
            }

            thisOffset += sizeof32(*pFontListElemW);
        }
    }

    host_font_spec_dispose(&host_font_spec);

    return(fontRefNum);
}

/* end of gr_rdia3.c */
