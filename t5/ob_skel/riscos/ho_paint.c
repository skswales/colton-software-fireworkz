/* riscos/ho_paint.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RISC OS specific graphics routines for Fireworkz */

/* RCM April 1992 */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#define EXPOSE_RISCOS_BBC 1
#define EXPOSE_RISCOS_SWIS 1
#define EXPOSE_RISCOS_FONT 1

#include "ob_skel/xp_skelr.h"

#ifndef __gr_diag_h
#include "cmodules/gr_diag.h"
#endif

#ifndef __gr_rdia3_h
#include "cmodules/gr_rdia3.h"
#endif

#include "cmodules/collect.h"

#if CROSS_COMPILE
#include "drawmod.h" /* csnf...RISC_OSLib */
#else
#include "RISC_OSLib:drawmod.h" /* RISC_OSLib: */
#endif

/*
internal routines
*/

_Check_return_
static int
host_setcolour(
    _InRef_     PC_RGB p_rgb);

_Check_return_
static STATUS
host_setfontcolours(
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background);

/*
internal types
*/

typedef union RISCOS_PALETTE_U
{
    unsigned int word;

    struct RISCOS_PALETTE_U_BYTES
    {
        char gcol;
        char red;
        char green;
        char blue;
    } bytes;
}
RISCOS_PALETTE_U;

/* The gcol byte (least significant) is a gcol colour except in 8-bpp
 * modes, when bits 0..2 are the tint and bits 3..7 are the gcol colour.
*/

static
struct HOST_CACHE
{
    struct HOST_CACHE_ENTRY
    {
        U32 rgb_key;
        S32 use;
    } bg, fg;

    struct HOST_CACHE_FONT_ENTRY
    {
        RISCOS_PALETTE_U rgb_fg;
        RISCOS_PALETTE_U rgb_bg;

        STATUS set;
    } font;

    Palette palette;
}
cache;

typedef struct HOST_CACHE_ENTRY * P_HOST_CACHE_ENTRY;

RGB rgb_stash[16]; /* exported */

#if defined(UNUSED_KEEP_ALIVE)

_Check_return_
/*static*/ int
host_find_rgb_in_stash(
    _InRef_     PC_RGB p_rgb)
{
    int i;

    if(p_rgb->transparent)
        return(-1);

    for(i = 0; i < elemof32(rgb_stash); ++i)
        if(p_rgb->r == rgb_stash[i].r)
        if(p_rgb->g == rgb_stash[i].g)
        if(p_rgb->b == rgb_stash[i].b)
            return(i);

    return(-1);
}

#endif /* UNUSED_KEEP_ALIVE */

static inline void /* inline the version used in ho_paint.c */
_host_invalidate_cache(
    _InVal_     S32 bits)
{
    if(bits & HIC_PURGE)
        /*EMPTY*/;

    if(bits & HIC_BG)
        cache.bg.rgb_key = 0xFFFFFFFFU;

    if(bits & HIC_FG)
        cache.fg.rgb_key = 0xFFFFFFFFU;

    if(bits & HIC_FONT)
        cache.font.set = 0;
}

extern void
host_invalidate_cache(
    _InVal_     S32 bits)
{
    _host_invalidate_cache(bits);
}

extern void
host_bleep(void)
{
    bbc_vdu(7);
}

extern void
host_clg(void)
{
    bbc_vdu(16);
}

_Check_return_
static int /*colnum*/
colourtrans_ReturnColourNumber(
    _In_        unsigned int word)
{
    _kernel_swi_regs rs;
    rs.r[0] = word;
    return(_kernel_swi(ColourTrans_ReturnColourNumber, &rs, &rs) ? 0 : rs.r[0]);
}

_Check_return_
static inline int /*gcol_out*/
colourtrans_SetGCOL(
    _In_        unsigned int word,
    _In_        int flags,
    _In_        int gcol_action)
{
    _kernel_swi_regs rs;
    rs.r[0] = word;
    rs.r[3] = flags;
    rs.r[4] = gcol_action;
    assert((rs.r[3] & 0xfffffe7f) == 0); /* just bits 7 and 8 are valid */
    return(_kernel_swi(ColourTrans_SetGCOL, &rs, &rs) ? 0 : rs.r[0]);
}

extern void
host_paint_start(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    UNREFERENCED_PARAMETER_InRef_(p_redraw_context);
}

extern void
host_paint_end(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    fonty_cache_trash(p_redraw_context);
}

/******************************************************************************
*
* patch Res00's loaded sprite file to redefine the names of our 3-D versions
* if new ones are not loaded
*
******************************************************************************/

static const
struct HOST_SYSTEM_SPRITES
{
    /*U32 crc;*/
    /*OBJECT_ID object_id;*/
    U8Z name[12];
}
host_system_sprites[] =
{
    { /*0x5E58,*/ /*OBJECT_ID_SKEL,*/ "up!" },
    { /*0x19D5,*/ /*OBJECT_ID_SKEL,*/ "down!" },
    { /*0x664D,*/ /*OBJECT_ID_SKEL,*/ "opton!" },
    { /*0x7649,*/ /*OBJECT_ID_SKEL,*/ "optoff!" },
    { /*0x06E9,*/ /*OBJECT_ID_SKEL,*/ "radioon!" },
    { /*0x3F4B,*/ /*OBJECT_ID_SKEL,*/ "radiooff!" },
    { /*0x0000,*/ /*OBJECT_ID_SKEL,*/ "gright!" },
    { /*0x0000,*/ /*OBJECT_ID_SKEL,*/ "ptr_write!" } /* not in any ROM a ce moment, CRC of 0 should sort it */
};

extern void
host_fixup_system_sprites(void)
{
    U32 i;

    if(host_os_version_query() >= RISCOS_3_5)
        /* SKS 15nov94 - all above sprites should be taken from ROM or wherever please */
        return;

    for(i = 0; i < elemof32(host_system_sprites); ++i)
    {
        RESOURCE_BITMAP_HANDLE resource_bitmap_handle; /* SKS 11may95 ALWAYS use our bound versions if not on 3.5+ */
        RESOURCE_BITMAP_ID resource_bitmap_id_pling;
        S32 area_id;

        resource_bitmap_id_pling.object_id = OBJECT_ID_SKEL /*host_system_sprites[i].object_id*/;
        resource_bitmap_id_pling.bitmap_name = host_system_sprites[i].name;

        /* rename the sprite in all areas we have loaded */
        for(area_id = RESOURCE_BITMAP_AREA_STANDARD; area_id < RESOURCE_BITMAP_AREA_COUNT; ++area_id)
        {
            resource_bitmap_handle = resource_bitmap_find_in_area(&resource_bitmap_id_pling, area_id);

            if(resource_bitmap_handle.i)
            {
                /* too many OS bugs to use rename sprite call so knock off the '!' by hand */
                P_U8 p_u8 = resource_bitmap_handle.p_scb->name;
                p_u8[strlen32(p_u8)-1] = CH_NULL;
            }
        }
    }
}

/******************************************************************************
*
* Paint to screen routines
*
* N.B. These routines may be called by view, skel or lower layer code on receipt of a T5_EVENT_REDRAW message
*      The caller passes PIXIT coordinates (i.e. coordinates relative to the layers natural origin) and the REDRAW_CONTEXT
*      supplied with the T5_EVENT_REDRAW message. The origin and scale information in the REDRAW_CONTEXT allows the
*      PIXIT coordinates from the caller, which actually form part of a VIEW_POINT (or VIEW_RECT etc) or SKEL_POINT
*      (or SKEL_RECT etc) to be converted to appropriatly zoomed machine specific screen, Window Manager or Font Manager
*      coordinates
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

_Check_return_
_Ret_writes_bytes_maybenull_(extraBytes)
static P_ANY
gr_riscdiag_path_new_raw(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pPathStart,
    _In_        const drawmod_line * const line_attributes,
    _InRef_     PC_RGB p_rgb,
    _InVal_     U32 extraBytes,
    _OutRef_    P_STATUS p_status)
{
    const drawmod_dashhdr * dash_pattern = line_attributes->dash_pattern;
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
    path.pathwidth = line_attributes->thickness;
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

struct gr_riscdiag_line_guts
{
    DRAW_PATH_MOVE  pos;    /* move to first point */
    DRAW_PATH_LINE  lineto; /* line to second point */
    DRAW_PATH_TERM  term;
};

/******************************************************************************
*
* host_paint_border_line
*
******************************************************************************/

#if defined(PROFILING)

/* split up at top level then join again - put hpbl_core in unwind list to find which one takes longest */

static void
hpbl_core(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb,
    _In_        BORDER_LINE_FLAGS flags);

static void
host_paint_border_line_h(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb,
    _In_        BORDER_LINE_FLAGS flags)
{
    RGB rgb = *p_rgb;
    hpbl_core(p_redraw_context, p_pixit_line, &rgb, flags);
}

static void
host_paint_border_line_v(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb,
    _In_        BORDER_LINE_FLAGS flags)
{
    RGB rgb = *p_rgb;
    hpbl_core(p_redraw_context, p_pixit_line, &rgb, flags);
}

static void
host_paint_border_line(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb,
    _In_        BORDER_LINE_FLAGS flags)
{
    (p_pixit_line->horizontal ? host_paint_border_line_h : host_paint_border_line_v) (p_redraw_context, p_pixit_line, p_rgb, flags);
}

static void
hpbl_core(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb,
    _In_        BORDER_LINE_FLAGS flags)

#else

extern void
host_paint_border_line(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb,
    _In_        BORDER_LINE_FLAGS flags)

#endif /* PROFILING */

{
    PIXIT_LINE pixit_line = *p_pixit_line;
    DRAW_POINT start, end;
    BOOL rect_fill;
    PIXIT_POINT line_width, line_width_eff, line_width_select;
    PIXIT_RECT pixit_rect;

    if(p_rgb->transparent)
        return;

    switch(flags.border_style)
    {
    case SF_BORDER_NONE:
        return;

    case SF_BORDER_THIN:
        line_width     = p_redraw_context->thin_width;
        line_width_eff = p_redraw_context->thin_width_eff;
        break;

    default: default_unhandled();
#if CHECKING
    case SF_BORDER_STANDARD:
    case SF_BORDER_BROKEN:
#endif
        line_width     = p_redraw_context->line_width;
        line_width_eff = p_redraw_context->line_width_eff;
        break;

    case SF_BORDER_THICK:
        line_width     = p_redraw_context->border_width;
        line_width_eff = p_redraw_context->border_width;
        break;
    }

    switch(flags.border_style)
    {
    default:
#if CHECKING
    case SF_BORDER_NONE:
        assert0();

        /*FALLTHRU*/

    case SF_BORDER_THIN:
    case SF_BORDER_STANDARD:
    case SF_BORDER_THICK:
#endif
        rect_fill = !(p_redraw_context->flags.printer || p_redraw_context->flags.drawfile);
        break;

    case SF_BORDER_BROKEN:
        rect_fill = 0;
        break;
    }

#if TRACE_ALLOWED
    tracef(TRACE_APP_HOST_PAINT, TEXT("host_paint_border_line: line tl ") S32_TFMT TEXT(",") S32_TFMT TEXT(" br ") S32_TFMT TEXT(",") S32_TFMT TEXT(" %s lw ") S32_TFMT TEXT(",") S32_TFMT TEXT(" lw_eff ") S32_TFMT TEXT(",") S32_TFMT,
           pixit_line.tl.x, pixit_line.tl.y, pixit_line.br.x, pixit_line.br.y, pixit_line.horizontal ? "H" : "V",
           line_width.x, line_width.y, line_width_eff.x, line_width_eff.y);
#endif

    if( p_redraw_context->flags.printer  ||
        /*p_redraw_context->flags.metafile || not on RISCOS!*/
        p_redraw_context->flags.drawfile )
        line_width_select = line_width;
    else
        line_width_select = line_width_eff;

    if(flags.add_lw_to_l)
        pixit_line.tl.x += line_width_select.x;
    if(flags.add_gw_to_l)
        pixit_line.tl.x += p_redraw_context->border_width.x;
    if(flags.sub_gw_from_l)
        pixit_line.tl.x -= p_redraw_context->border_width.x;

    if(flags.add_lw_to_t)
        pixit_line.tl.y += line_width_select.y;
    if(flags.add_gw_to_t)
        pixit_line.tl.y += p_redraw_context->border_width.y;
    if(flags.sub_gw_from_t)
        pixit_line.tl.y -= p_redraw_context->border_width.y;

    if(flags.add_lw_to_r)
        pixit_line.br.x += line_width_select.x;
    if(flags.add_gw_to_r)
        pixit_line.br.x += p_redraw_context->border_width.x;
    if(flags.sub_gw_from_r)
        pixit_line.br.x -= p_redraw_context->border_width.x;

    if(flags.add_lw_to_b)
        pixit_line.br.y += line_width_select.y;
    if(flags.add_gw_to_b)
        pixit_line.br.y += p_redraw_context->border_width.y;
    if(flags.sub_gw_from_b)
        pixit_line.br.y -= p_redraw_context->border_width.y;

    pixit_rect.tl.x = pixit_line.tl.x;
    pixit_rect.tl.y = pixit_line.tl.y;

    if(pixit_line.horizontal)
    {
        pixit_rect.br.x = pixit_line.br.x;
        pixit_rect.br.y = pixit_rect.tl.y + line_width_select.y;
    }
    else
    {
        pixit_rect.br.x = pixit_rect.tl.x + line_width_select.x;
        pixit_rect.br.y = pixit_line.br.y;
    }

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_border_line: rect tl ") S32_TFMT TEXT(",") S32_TFMT TEXT(" br ") S32_TFMT TEXT(",") S32_TFMT, pixit_rect.tl.x, pixit_rect.tl.y, pixit_rect.br.x, pixit_rect.br.y);

    if(p_redraw_context->flags.printer || p_redraw_context->flags.drawfile)
    {
        GDI_RECT gdi_rect;

        assert(!rect_fill);

        /* go straight into Draw units, no pixel rounding */
        status_consume(gdi_rect_from_pixit_rect_and_context_draw(&gdi_rect, &pixit_rect, p_redraw_context));

        start.x = gdi_rect.tl.x; /*inc*/
        start.y = gdi_rect.br.y; /*inc*/
        end.x   = gdi_rect.br.x; /*exc*/
        end.y   = gdi_rect.tl.y; /*exc*/
    }
    else
    {
        GDI_RECT gdi_rect;

        /* clip close to GDI limits (only ok because lines either horizontal or vertical) */
        if(!status_done(gdi_rect_limited_from_pixit_rect_and_context(&gdi_rect, &pixit_rect, p_redraw_context)))
        {   /* line too small to plot */
            trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_border_line: box ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT TEXT(" FAILED"), gdi_rect.tl.x, gdi_rect.tl.y, gdi_rect.br.x, gdi_rect.br.y);
            return;
        }

        if(rect_fill)
        {
            GDI_POINT os_start, os_end;

            os_start.x = gdi_rect.tl.x; /*inc*/
            os_start.y = gdi_rect.br.y; /*inc*/
            os_end.x   = gdi_rect.br.x; /*exc*/
            os_end.y   = gdi_rect.tl.y; /*exc*/

            os_end.x -= p_redraw_context->host_xform.riscos.d_x;
            os_end.y -= p_redraw_context->host_xform.riscos.d_y;

            if(flags.border_style == SF_BORDER_THIN)
            {
                /* thin is always drawn very thin on screen, whereas standard and thick may be scaled */
                if(pixit_line.horizontal)
                    os_start.y = os_end.y;
                else
                    os_end.x = os_start.x;
            }

            void_WrapOsErrorChecking(
                bbc_move(os_start.x, os_start.y));

            void_WrapOsErrorChecking(
                os_plot(host_setcolour(p_rgb) | bbc_MoveCursorAbs | bbc_RectangleFill,
                        (os_end.x + p_redraw_context->host_xform.riscos.d_x - 1 /* fill out to pixel edge for printer drivers */),
                        (os_end.y + p_redraw_context->host_xform.riscos.d_y - 1)));

            return;
        }

        /* go into Draw units after pixel rounding and GDI limiting */
        start.x = gdi_rect.tl.x << 8; /*inc*/
        start.y = gdi_rect.br.y << 8; /*inc*/
        end.x   = gdi_rect.br.x << 8; /*exc*/
        end.y   = gdi_rect.tl.y << 8; /*exc*/
    }

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_border_line: start ") S32_TFMT TEXT(",") S32_TFMT TEXT("; end") S32_TFMT TEXT(",") S32_TFMT, start.x, start.y, end.x, end.y);

    { /* use Draw module for all dotted lines and most printed lines */
    static S32 matrix[6] =
    {
        0x00010000, 0,
        0,          0x00010000,
        0,          0
    };

    struct { DRAW_PATH_MOVE move; DRAW_PATH_LINE line; DRAW_PATH_TERM term; } thin;
    struct { drawmod_dashhdr header; int pattern[2]; } dash_pattern;
    drawmod_line line_attributes;
    _kernel_swi_regs rs;
    S32 half_width;

    zero_struct(line_attributes);

    /* It would have been nice to use the draw modules transformation matrix to do our scaling  */
    /* (or at least to have used to do the <<8 RISC_OS to draw unit conversion), but such large */
    /* matrix multiplication factors (~256) cause overflow errors in the draw module.           */

    /* setup move x1,y1; line_to x2,y2; term sequence */

    thin.move.tag = path_move_2;
    thin.move.pt  = start;

    thin.line.tag = path_lineto;
    thin.line.pt  = end;

    if(pixit_line.horizontal)
        thin.move.pt.y = end.y;
    else
        thin.line.pt.x = start.x;

    thin.term.tag = path_term;

#define BROKEN_LINE_MARK  14513 /* 180*256*8/25.4 i.e. 1mm */
#define BROKEN_LINE_SPACE 14513

    if(flags.border_style == SF_BORDER_BROKEN)
    {
        dash_pattern.header.dashstart = pixit_line.horizontal
                                      ? os_unit_from_pixit_x(pixit_line.tl.x, &p_redraw_context->host_xform)
                                      : os_unit_from_pixit_y(pixit_line.tl.y, &p_redraw_context->host_xform);

        dash_pattern.pattern[0] = os_unit_from_pixit_x(BROKEN_LINE_MARK,  &p_redraw_context->host_xform);
        dash_pattern.pattern[1] = os_unit_from_pixit_x(BROKEN_LINE_SPACE, &p_redraw_context->host_xform);

        dash_pattern.header.dashstart = ((U32) abs((int) dash_pattern.header.dashstart)) << 8;
        dash_pattern.header.dashcount = 2;
        assert_EQ(2U, elemof32(dash_pattern.pattern));

        line_attributes.dash_pattern = &dash_pattern.header;
    }

    line_attributes.thickness = pixit_line.horizontal ? (end.y - start.y) : (end.x - start.x);
    if(p_redraw_context->flags.printer)
    {
        /* adjust downwards as I think the Draw module must fill pixels whose centres are touched by the line in both ways */
        line_attributes.thickness = MAX(0, line_attributes.thickness - 1);
    }
    else
    {
        line_attributes.thickness = MAX(0, line_attributes.thickness);
    }
    half_width = line_attributes.thickness >> 1;

    /* put the line down the middle of the rectangle that the line lies in */
    if(p_redraw_context->flags.printer)
    {
        if(pixit_line.horizontal)
        {
            if(half_width < 128)
            {
                thin.move.pt.y -= 256; /* come just inside */
                thin.line.pt.y -= 256;
            }
            else
            {
                thin.move.pt.y -= half_width;
                thin.line.pt.y -= half_width;
            }
        }
        else
        {
            thin.move.pt.x += half_width;
            thin.line.pt.x += half_width;
        }
    }
    else if(p_redraw_context->flags.drawfile)
    {
        if(pixit_line.horizontal)
        {
            thin.line.pt.x -= half_width;
        }
        else
        {
            thin.move.pt.y += half_width;
            thin.line.pt.y += half_width;
        }
    }

    line_attributes.spec.mitrelimit = 0xA0000; /* Mitre limit=10.0 (PostScript default) */

    if(p_redraw_context->flags.drawfile)
    {
        struct gr_riscdiag_line_guts line;
        STATUS status;
        P_BYTE pPathGuts;
        DRAW_DIAG_OFFSET line_start;

#ifndef gr_riscDraw_from_pixit
#define gr_riscDraw_from_pixit(x) ((x) * GR_RISCDRAW_PER_PIXIT)
#endif
        line.pos.tag  = DRAW_PATH_TYPE_MOVE;
        line.pos.pt.x = thin.move.pt.x + gr_riscDraw_from_pixit(p_redraw_context->page_pixit_origin_draw.x);
        line.pos.pt.y = thin.move.pt.y + gr_riscDraw_from_pixit(p_redraw_context->page_pixit_origin_draw.y);

        line.lineto.tag  = DRAW_PATH_TYPE_LINE;
        line.lineto.pt.x = thin.line.pt.x + gr_riscDraw_from_pixit(p_redraw_context->page_pixit_origin_draw.x);
        line.lineto.pt.y = thin.line.pt.y + gr_riscDraw_from_pixit(p_redraw_context->page_pixit_origin_draw.y);

        line.term.tag = DRAW_PATH_TYPE_TERM;
        if(NULL != (pPathGuts = gr_riscdiag_path_new_raw(p_redraw_context->p_gr_riscdiag, &line_start, &line_attributes, p_rgb, sizeof32(line), &status)))
            memcpy32(pPathGuts, &line, sizeof32(line));

        return;
    }

    (void) host_setfgcolour(p_rgb);

    rs.r[0] = (int) &thin;
    rs.r[1] = 0x38;
    rs.r[2] = (int) &matrix[0];
    rs.r[3] = 200; /* flatness: would normally use 200/zoomfactor, but since line IS flat, this should do */
    rs.r[4] = (int) line_attributes.thickness;
    rs.r[5] = (int) &line_attributes.spec;
    rs.r[6] = (int) line_attributes.dash_pattern;

    if(_kernel_swi(/*Draw_Stroke*/ 0x040704, &rs, &rs))
    {
        trace_1(TRACE_OUT | TRACE_APP_HOST_PAINT, TEXT("host_paint_border_line: Draw_Stroke error: %s"), _kernel_last_oserror()->errmess);
    }
    } /*block*/
}

/******************************************************************************
*
* host_paint_underline
*
******************************************************************************/

extern void
host_paint_underline(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb,
    _InVal_     PIXIT line_thickness)
{
    PIXIT_LINE pixit_line = *p_pixit_line;
    DRAW_POINT start, end;
    BOOL rect_fill;
    PIXIT line_width;

    if(p_rgb->transparent)
        return;

    /* SKS after 1.20/50 14mar95 */
    if(p_redraw_context->flags.printer || p_redraw_context->flags.drawfile)
        line_width = line_thickness /* was p_redraw_context->line_width.y */;
    else
        line_width = MAX(line_thickness, p_redraw_context->one_real_pixel.y /* was thin_width_eff */);

    rect_fill = !(p_redraw_context->flags.printer || p_redraw_context->flags.drawfile);

    {
    PIXIT_RECT pixit_rect;

    pixit_rect.tl.x = pixit_line.tl.x;
    pixit_rect.tl.y = pixit_line.tl.y;
    pixit_rect.br.x = pixit_line.br.x;
    pixit_rect.br.y = pixit_rect.tl.y + line_width;

    if(p_redraw_context->flags.printer || p_redraw_context->flags.drawfile)
    {
        GDI_RECT gdi_rect;

        assert(!rect_fill);

        /* go straight into Draw units, no pixel rounding */
        status_consume(gdi_rect_from_pixit_rect_and_context_draw(&gdi_rect, &pixit_rect, p_redraw_context));

        start.x = gdi_rect.tl.x; /*inc*/
        start.y = gdi_rect.br.y; /*inc*/
        end.x   = gdi_rect.br.x; /*exc*/
        end.y   = gdi_rect.tl.y; /*exc*/
    }
    else
    {
        GDI_RECT gdi_rect;

        /* clip close to GDI limits (only ok because lines either horizontal or vertical) */
        if(!status_done(gdi_rect_limited_from_pixit_rect_and_context(&gdi_rect, &pixit_rect, p_redraw_context)))
        {   /* line too small to plot */
            trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_underline: box ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT TEXT(" FAILED"), gdi_rect.tl.x, gdi_rect.tl.y, gdi_rect.br.x, gdi_rect.br.y);
            return;
        }

        if(rect_fill)
        {
            GDI_POINT os_start, os_end;

            os_start.x = gdi_rect.tl.x; /*inc*/
            os_start.y = gdi_rect.br.y; /*inc*/
            os_end.x   = gdi_rect.br.x; /*exc*/
            os_end.y   = gdi_rect.tl.y; /*exc*/

            os_end.x -= p_redraw_context->host_xform.riscos.d_x;
            os_end.y -= p_redraw_context->host_xform.riscos.d_y;

            void_WrapOsErrorChecking(
                bbc_move(os_start.x, os_start.y));

            void_WrapOsErrorChecking(
                os_plot(host_setcolour(p_rgb) | bbc_MoveCursorAbs | bbc_RectangleFill,
                        (os_end.x + p_redraw_context->host_xform.riscos.d_x - 1 /* fill out to pixel edge for printer drivers */),
                        (os_end.y + p_redraw_context->host_xform.riscos.d_y - 1)));

            return;
        }

        /* go into Draw units after pixel rounding and GDI limiting */
        start.x = gdi_rect.tl.x << 8; /*inc*/
        start.y = gdi_rect.br.y << 8; /*inc*/
        end.x   = gdi_rect.br.x << 8; /*exc*/
        end.y   = gdi_rect.tl.y << 8; /*exc*/
    }
    } /*block*/

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_underline: start ") S32_TFMT TEXT(",") S32_TFMT TEXT("; end") S32_TFMT TEXT(",") S32_TFMT, start.x, start.y, end.x, end.y);

    { /* use Draw module otherwise */
    static S32 matrix[6] =
    {
        0x00010000, 0,
        0,          0x00010000,
        0,          0
    };

    struct { DRAW_PATH_MOVE move; DRAW_PATH_LINE line; DRAW_PATH_TERM term; } thin;
    drawmod_line line_attributes;
    _kernel_swi_regs rs;
    S32 half_width;

    zero_struct(line_attributes);

    /* It would have been nice to use the draw modules transformation matrix to do our scaling  */
    /* (or at least to have used to do the <<8 RISC_OS to draw unit conversion), but such large */
    /* matrix multiplication factors (~256) cause overflow errors in the draw module.           */

    /* setup move x1,y1; line_to x2,y2; term sequence */

    thin.move.tag = path_move_2;
    thin.move.pt.x = start.x;
    thin.move.pt.y = end.y;

    thin.line.tag = path_lineto;
    thin.line.pt = end;

    /* line is horizontal */

    thin.term.tag = path_term;

    line_attributes.thickness = end.y - start.y;
    if(p_redraw_context->flags.printer)
    {
        /* adjust downwards as I think the Draw module must fill pixels whose centres are touched by the line in both ways */
        line_attributes.thickness = MAX(0, line_attributes.thickness - 1);
    }
    else
    {
        line_attributes.thickness = MAX(0, line_attributes.thickness);
    }
    half_width = line_attributes.thickness >> 1;

    /* put the line down the middle of the rectangle that the line lies in */
    if(p_redraw_context->flags.printer)
    {
        if(half_width < 128)
        {
            thin.move.pt.y -= 256; /* come just inside */
            thin.line.pt.y -= 256;
        }
        else
        {
            thin.move.pt.y -= half_width;
            thin.line.pt.y -= half_width;
        }
    }
    else if(p_redraw_context->flags.drawfile)
    {
        thin.line.pt.x -= half_width;
    }

    line_attributes.spec.mitrelimit = 0xA0000; /* Mitre limit=10.0 (PostScript default) */

    if(p_redraw_context->flags.drawfile)
    {
        struct gr_riscdiag_line_guts line;
        STATUS status;
        P_BYTE pPathGuts;
        DRAW_DIAG_OFFSET line_start;

#ifndef gr_riscDraw_from_pixit
#define gr_riscDraw_from_pixit(x) ((x) * GR_RISCDRAW_PER_PIXIT)
#endif
        line.pos.tag  = DRAW_PATH_TYPE_MOVE;
        line.pos.pt.x = thin.move.pt.x + gr_riscDraw_from_pixit(p_redraw_context->page_pixit_origin_draw.x);
        line.pos.pt.y = thin.move.pt.y + gr_riscDraw_from_pixit(p_redraw_context->page_pixit_origin_draw.y);

        line.lineto.tag  = DRAW_PATH_TYPE_LINE;
        line.lineto.pt.x = thin.line.pt.x + gr_riscDraw_from_pixit(p_redraw_context->page_pixit_origin_draw.x);
        line.lineto.pt.y = thin.line.pt.y + gr_riscDraw_from_pixit(p_redraw_context->page_pixit_origin_draw.y);

        line.term.tag = DRAW_PATH_TYPE_TERM;

        if(NULL != (pPathGuts = gr_riscdiag_path_new_raw(p_redraw_context->p_gr_riscdiag, &line_start, &line_attributes, p_rgb, sizeof32(line), &status)))
            memcpy32(pPathGuts, &line, sizeof32(line));

        return;
    }

    (void) host_setfgcolour(p_rgb);

    rs.r[0] = (int) &thin;
    rs.r[1] = 0x38;
    rs.r[2] = (int) &matrix[0];
    rs.r[3] = 200; /* flatness: would normally use 200/zoomfactor, but since line IS flat, this should do */
    rs.r[4] = (int) line_attributes.thickness;
    rs.r[5] = (int) &line_attributes.spec;
    rs.r[6] = (int) line_attributes.dash_pattern;

    void_WrapOsErrorChecking(_kernel_swi(/*Draw_Stroke*/ 0x040704, &rs, &rs));
    } /*block*/
}

/*
Task module documentation
=========================

The Task module in this directory is a cut down
version of a high level WIMP handler due for
release late in 1990.

It currently supports only one SWI required for
Tween.

The syntax is:

 SYS"Task_DrawRender",0,File%,[Transform%],FileEnd%[,Box%]
 or
 SYS"Task_DrawRender",1,Object%,[Transform%][,,Box%]

File%,FileEnd% and Object% are pointers to memory.
The area from File% to FileEnd% contains a
complete drawfile. Object% points to a single
object within a drawfile.

Transform% can be zero, indicating a null
transform, otherwise it takes the same form as for
transforms for the Draw module.

Box% is a pointer to a 4 word block giving the
left,bottom (internal) right,top (external)
coordinates of a bounding box for the rendering.
Box% can be zero indicating that the current
graphics clipping window should be used for the
rendering.

On exit from this routine, the graphics clipping
window will be set to that required, and the VDU 5
state may have been set.

Rendering includes the rendering of text areas.
The only known discrepencies with Draw are:

Soft-hyphens in text areas are ignored. Draw also
gets these wrong in any case (it splits under the
wrong circumstances).

Sprites without palettes are rendered using the
current Wimp pallete, rather than the default
palette for the mode in question. (This works
better for most icons as used on the desktop.)

 0.06: R0 can now have bit 1 set and bits 8-15
set to a mode number for rendering. This is to
allow use of the Printer Drivers which invalidate
calls to OS_ReadVduVariables.

 0.07: R0 can now have bit 2 set and R5 is the
flatness for calls to Draw_Fill and Draw_Stroke.

 0.08: Now handles object types 12 and 13 for
RISC OS 3.xx DrawFiles.

 0.09 Now does rotated text/sprites and 256 colour sprites.
*/

/* ------------------------------------------------------------------------
 * Function:      my_xdrawfile_render()
 *
 * Description:   Calls SWI 0x45540 (+ X)
 *
 * Input:         flags - value of R0 on entry
 *                diagram - value of R1 on entry
 *                size - value of R2 on entry
 *                trfm - value of R3 on entry
 *                clip - value of R4 on entry
 *                flatness - value of R5 on entry
 *
 * Saves linking with Acorn o.Drawfile
 */

static _kernel_oserror *
swi_DrawFile_Render(
    _In_        unsigned int flags,
    _In_        int diagram_address,
    _In_        int size,
    _In_        DRAW_TRANSFORM * trfm,
    _InRef_     PC_GDI_BOX clip,
    _In_        int flatness)
{
    _kernel_swi_regs rs;
    rs.r[0] = flags;
    rs.r[1] = diagram_address;
    rs.r[2] = size;
    rs.r[3] = (int) trfm;
    rs.r[4] = (int) clip;
    rs.r[5] = flatness;
    return(_kernel_swi(/*DrawFile_Render*/ 0x45540, &rs, &rs));
}

static _kernel_oserror *
swi_Task_DrawRender(
    _In_        int mode,
    _In_        int diagram_address,
    _In_        int size,
    _In_        DRAW_TRANSFORM * trfm,
    _InRef_     PC_GDI_BOX clip,
    _In_        int flatness)
{
    _kernel_swi_regs rs;
    rs.r[0] = mode | (1 << 2);
    rs.r[1] = diagram_address;
    rs.r[2] = (int) trfm;
    rs.r[3] = rs.r[1] + (int) size;
    rs.r[4] = (int) clip; /* ptr to clipping window */
    rs.r[5] = flatness;
    return(_kernel_swi(/*Task_DrawRender*/ 0x41195, &rs, &rs));
}

static void
host_paint_drawfile_render_path(
    _InRef_     P_DRAW_OBJECT_PATH pPathObject,
    _InRef_     PC_DRAW_TRANSFORM p_t,
    _InVal_     int flatness)
{
    DRAW_OBJECT_PATH path;
    P_BYTE path_seq = (P_BYTE) (pPathObject + 1); /* path really starts here (unless dashed) */
    P_BYTE line_dash_pattern = NULL;

    memcpy32(&path, pPathObject, sizeof32(path));

    if(path.pathstyle.flags & DRAW_PS_DASH_PACK_MASK)
    {
        line_dash_pattern = path_seq;

        path_seq = line_dash_pattern + sizeof32(DRAW_DASH_HEADER)
                 + sizeof32(S32) * (* (P_U32) (line_dash_pattern + offsetof32(DRAW_DASH_HEADER, dashcount)));
    }

    /* fill the path */

    if(path.fillcolour != DRAW_COLOUR_Transparent)
    {
        int gcol_out = colourtrans_SetGCOL(path.fillcolour, 0x80 /*bg*/, 3 /*EOR*/);
        int fill = DMFT_PLOT_Bint | DMFT_PLOT_NonBint;
        _kernel_swi_regs rs;

        UNREFERENCED_PARAMETER(gcol_out);

        /* two bit winding rule field, either 10 (even-odd) or 00 (non-zero) from Draw path object */
        if(path.pathstyle.flags & DRAW_PS_WINDRULE_PACK_MASK)
            fill |= (((unsigned int) path.pathstyle.flags & DRAW_PS_WINDRULE_PACK_MASK) >> DRAW_PS_WINDRULE_PACK_SHIFT) << 1; /* see PRM */

        rs.r[0] = (int) path_seq;
        rs.r[1] = fill;
        rs.r[2] = (int) p_t;
        rs.r[3] = flatness;

        void_WrapOsErrorChecking(_kernel_swi(/*Draw_Fill*/ 0x040702, &rs, &rs));
    }

    /* stroke the path */

    if(path.pathcolour != DRAW_COLOUR_Transparent)
    {
        //int gcol_out = colourtrans_SetGCOL(path.pathcolour, 0x00 /*fg*/, 3 /*EOR*/);
        int fill = DMFT_PLOT_Bint | DMFT_PLOT_NonBint | DMFT_PLOT_Bext;
        DRAW_MODULE_CAP_JOIN_SPEC cjspec;
        U32 temp;
        RISCOS_PALETTE_U os_rgb_foreground;
        RISCOS_PALETTE_U os_rgb_background;

        os_rgb_foreground.word = path.pathcolour;

        os_rgb_background.bytes.gcol  = 0;
        os_rgb_background.bytes.red   = 0xFF; /*p_rgb_background->r;*/
        os_rgb_background.bytes.green = 0xFF; /*p_rgb_background->g;*/
        os_rgb_background.bytes.blue  = 0xFF; /*p_rgb_background->b;*/

        { /* New machines usually demand this mechanism */
        int colnum_foreground = colourtrans_ReturnColourNumber(os_rgb_foreground.word);
        int colnum_background = colourtrans_ReturnColourNumber(os_rgb_background.word);
        _kernel_swi_regs rs;
        rs.r[0] = 3;
        rs.r[1] = colnum_foreground ^ colnum_background;
        void_WrapOsErrorChecking(_kernel_swi(OS_SetColour, &rs, &rs));
        } /*block*/

        /* flatness (no DrawFiles recommendation. Draw module recommends 2 OS units) */
        temp = ((U32) path.pathstyle.flags & DRAW_PS_JOIN_PACK_MASK)     >> DRAW_PS_JOIN_PACK_SHIFT;
        cjspec.join_style           = (unsigned char) temp;
        temp = ((U32) path.pathstyle.flags & DRAW_PS_STARTCAP_PACK_MASK) >> DRAW_PS_STARTCAP_PACK_SHIFT;
        cjspec.leading_cap_style    = (unsigned char) temp;
        temp = ((U32) path.pathstyle.flags & DRAW_PS_ENDCAP_PACK_MASK)   >> DRAW_PS_ENDCAP_PACK_SHIFT;
        cjspec.trailing_cap_style   = (unsigned char) temp;
        cjspec.reserved             = 0;
        cjspec.mitre_limit = 0x000A0000; /* 10.0 "PostScript default" from DrawFiles doc'n */
        cjspec.leading_tricap_width     = (U16) (path.pathstyle.tricap_w * pPathObject->pathwidth) / 16;
        cjspec.leading_tricap_height    = (U16) (path.pathstyle.tricap_h * pPathObject->pathwidth) / 16;
        cjspec.trailing_tricap_width    = cjspec.leading_tricap_width;
        cjspec.trailing_tricap_height   = cjspec.leading_tricap_height;

        {
        _kernel_swi_regs rs;

        rs.r[0] = (int) path_seq;
        rs.r[1] = (int) fill;
        rs.r[2] = (int) p_t;
        rs.r[3] = 2 * GR_RISCDRAW_PER_RISCOS; /* flatness (no DrawFiles recommendation. Draw module recommends 2 OS units) */
        rs.r[4] = (int) pPathObject->pathwidth;
        rs.r[5] = (int) &cjspec;
        rs.r[6] = (int) line_dash_pattern;

        void_WrapOsErrorChecking(_kernel_swi(/*Draw_Stroke*/ 0x040704, &rs, &rs));
        } /*block*/
    }
}

extern void
host_paint_drawfile(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_GR_SCALE_PAIR p_gr_scale_pair,
    _InRef_     PC_DRAW_DIAG p_draw_diag,
    _InVal_     BOOL eor_paths)
{
    PC_BYTE p_diag = p_draw_diag->data;
    S32 length = p_draw_diag->length;
    GDI_POINT gdi_point;
    GR_SCALE_PAIR gr_scale_pair;
    DRAW_TRANSFORM t;
    int flatness;

    if(!p_draw_diag->length)
        return;

    if(p_redraw_context->flags.drawfile)
    {
        assert(!eor_paths);
        drawfile_paint_drawfile(p_redraw_context, p_pixit_point, p_gr_scale_pair, p_draw_diag);
        return;
    }

    gdi_point_from_pixit_point_and_context(&gdi_point, p_pixit_point, p_redraw_context);

    gr_scale_pair.x = muldiv64(p_gr_scale_pair->x, p_redraw_context->host_xform.scale.t.x, p_redraw_context->host_xform.scale.b.x);
    gr_scale_pair.y = muldiv64(p_gr_scale_pair->y, p_redraw_context->host_xform.scale.t.y, p_redraw_context->host_xform.scale.b.y);

    /* scaling part of matrix */
    t.a = gr_scale_pair.x;
    t.b = 0;
    t.c = 0;
    t.d = gr_scale_pair.y;

    { /* translation part of matrix; move diagram to the point and then ever so slightly right and down */
    P_DRAW_FILE_HEADER pDiagHdr = (P_DRAW_FILE_HEADER) p_diag;
    t.e = (gdi_point.x << 8) - muldiv64_floor(pDiagHdr->bbox.x0, gr_scale_pair.x, GR_SCALE_ONE);
    t.f = (gdi_point.y << 8) - muldiv64_ceil( pDiagHdr->bbox.y1, gr_scale_pair.y, GR_SCALE_ONE);
    } /*block*/

    /* no DrawFiles recommendation. Draw module recommends 2 OS units */
    flatness = (int) muldiv64(2 * GR_RISCDRAW_PER_RISCOS, GR_SCALE_ONE, gr_scale_pair.y);

    if(!eor_paths)
    {
        /* SKS for 1.30 09nov96 render Draw file using Drawfile module where present */
        unsigned int flags = (1 << 2 /*flatness*/);
        _kernel_oserror * result;

        if(NULL != (result = swi_DrawFile_Render(flags, (intptr_t) p_diag, (int) length, &t, &p_redraw_context->riscos.host_machine_clip_box, flatness)))
        {
            int mode = 0;

            if(p_redraw_context->flags.printer)
            {
                mode = -1; /* get Mr Cheal to read mode vars on RISC PC */
                if(host_os_version_query() < RISCOS_3_5)
                    /* SKS fixed 08oct92 after 1.05 - was drug crazed on printing. MUST use screen mode which was in use prior to printing start job */
                    mode = host_modevar_cache_current.mode_specifier;
                mode <<= 8;
                mode |= (1 << 1);
            }

            result = swi_Task_DrawRender(mode, (intptr_t) p_diag, (int) length, &t, &p_redraw_context->riscos.host_machine_clip_box, flatness);
        }

#if TRACE_ALLOWED
        if(NULL != result)
        {
            trace_1(TRACE_OUT | TRACE_APP_HOST_PAINT, TEXT("host_paint_drawfile: Drawfile render error: %s"), _kernel_last_oserror()->errmess);
        }
#endif

        host_invalidate_cache(HIC_BG | HIC_FG | HIC_FONT /* SKS after 1.05 18oct93 - silly beggars might use coloured fonts */);

        return;
    }

    {
    GR_RISCDIAG gr_riscdiag;
    P_GR_RISCDIAG p_gr_riscdiag = &gr_riscdiag;
    P_DRAW_OBJECT_HEADER pObject;
    DRAW_DIAG_OFFSET sttObject = DRAW_DIAG_OFFSET_FIRST /* first object */;
    DRAW_DIAG_OFFSET endObject = DRAW_DIAG_OFFSET_LAST  /* last object */;

    gr_riscdiag_diagram_setup_from_data(&gr_riscdiag, p_diag, length); /* OK for loan */ /* no need to set up hglobal */

    if(gr_riscdiag_object_first(p_gr_riscdiag, &sttObject, &endObject, (P_P_BYTE) &pObject, TRUE))
    {
        do  {
            /* render just the path objects using the Draw module */
            if(pObject->type == DRAW_OBJECT_TYPE_PATH)
                host_paint_drawfile_render_path((P_DRAW_OBJECT_PATH) pObject, &t, flatness);
        }
        while(gr_riscdiag_object_next(p_gr_riscdiag, &sttObject, &endObject, (P_P_BYTE) &pObject, TRUE));

        host_invalidate_cache(HIC_BG | HIC_FG);
    }
    } /*block*/
}

/******************************************************************************
*
* Return a machine specific font handle for the specified font.
*
* Use the scaling info contained in the REDRAW_CONTEXT
* or use 1:1 scaling when there is no drawing context (e.g. formatting)
*
******************************************************************************/

_Check_return_
extern HOST_FONT
host_font_find(
    _InRef_     PC_HOST_FONT_SPEC p_host_font_spec,
    _InRef_maybenone_ PC_REDRAW_CONTEXT p_redraw_context)
{
    HOST_FONT host_font = HOST_FONT_NONE;
    S32 numer, denom;
    _kernel_swi_regs rs;
    _kernel_oserror * p_kernel_oserror;

    /* RISC OS Font Manager needs 16x fontsize */
    S32 x16_font_size_x, x16_font_size_y;

    /* SKS 20jan93 notes that a 16000 pixit sized font (i.e. one page) would only overflow 31bits at ~8000% scale factor! */
    numer = (16 * p_host_font_spec->size_y);
    denom = PIXITS_PER_POINT;

    if(!IS_REDRAW_CONTEXT_NONE(p_redraw_context))
    {
        if(p_redraw_context->host_xform.do_x_scale && p_redraw_context->host_xform.do_y_scale)
        {   /* only if both are scaled */
            numer *= p_redraw_context->host_xform.scale.t.y;
            denom *= p_redraw_context->host_xform.scale.b.y;
        }
    }

    x16_font_size_y = numer / denom;

    if(0 == p_host_font_spec->size_x)
    {   /* doesn't cater for anisotropic scaling with zero size_x, but we don't do that... */
        x16_font_size_x = x16_font_size_y; /* same width as height */
    }
    else
    {
        numer = (16 * p_host_font_spec->size_x);
        denom = PIXITS_PER_POINT;

        if(!IS_REDRAW_CONTEXT_NONE(p_redraw_context))
        {
            if(p_redraw_context->host_xform.do_x_scale && p_redraw_context->host_xform.do_y_scale)
            {   /* only if both are scaled */
                numer *= p_redraw_context->host_xform.scale.t.x;
                denom *= p_redraw_context->host_xform.scale.b.x;
            }
        }

        x16_font_size_x = numer / denom;
    }

    if( x16_font_size_x < 16) /* SKS 04dec95 stop Font Manager barfing */
        x16_font_size_x = 16;

    if( x16_font_size_y < 16) /* SKS 04dec95 stop Font Manager barfing */
        x16_font_size_y = 16;

    rs.r[1] = (int) array_tstr(&p_host_font_spec->h_host_name_tstr);
    rs.r[2] = (int) x16_font_size_x;
    rs.r[3] = (int) x16_font_size_y;
    rs.r[4] = 0;
    rs.r[5] = 0;

    if(NULL == (p_kernel_oserror = _kernel_swi(/*Font_FindFont*/ 0x040081, &rs, &rs)))
        host_font = (HOST_FONT) rs.r[0];

    /*if(*array_tstr(&p_host_font_spec->h_host_name_tstr) == '\\')*/
    /*    reportf(TEXT("HFF(%s) got %d"), array_tstr(&p_host_font_spec->h_host_name_tstr), host_font);*/
    return(host_font);
}

extern void
host_font_dispose(
    _InoutRef_  P_HOST_FONT p_host_font,
    _InRef_maybenone_ PC_REDRAW_CONTEXT p_redraw_context)
{
    HOST_FONT host_font = *p_host_font;

    if(HOST_FONT_NONE != host_font)
    {
        *p_host_font = HOST_FONT_NONE;

        UNREFERENCED_PARAMETER_InRef_(p_redraw_context);
        WrapOsErrorReporting(font_LoseFont(host_font));
    }
}

_Check_return_
static S32
host_fonty_uchars_width_mp(
    _HfontRef_  HOST_FONT host_font,
    _In_reads_(uchars_n) PC_USTR uchars,
    _InVal_     U32 uchars_n)
{
    _kernel_swi_regs rs;
    _kernel_oserror * p_kernel_oserror;

    rs.r[0] = host_font;
    rs.r[1] = (int) uchars;
    rs.r[2] = FONT_SCANSTRING_USE_HANDLE /*r0*/ | FONT_SCANSTRING_USE_LENGTH /*r7*/;
    rs.r[3] = INT_MAX;
    rs.r[4] = INT_MAX;
    rs.r[7] = (int) uchars_n;

    if(host_version_font_m_read(HOST_FONT_KERNING))
        rs.r[2] |= FONT_SCANSTRING_KERNING;

    if(NULL == (p_kernel_oserror = WrapOsErrorChecking(_kernel_swi(/*Font_ScanString*/ 0x0400A1, &rs, &rs))))
        return(rs.r[3]); /*x*/

    return(0);
}

_Check_return_
extern PIXIT
host_fonty_uchars_width(
    _HfontRef_  HOST_FONT host_font,
    _In_reads_(uchars_n) PC_USTR uchars,
    _InVal_     U32 uchars_n)
{
    S32 mp_width = host_fonty_uchars_width_mp(host_font, uchars, uchars_n);
    PIXIT pixit_width = pixits_from_millipoints_ceil(mp_width);
    return(pixit_width);
}

/* generates true sub-pixel x but pixel-aligned y as per gdi_point_from_pixit_point_and_context() */

static void
gdi_point_mp_from_pixit_point_and_context(
    _OutRef_    P_GDI_POINT p_gdi_point,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT_POINT pixit_point = *p_pixit_point;
    PIXIT_POINT multiplier;
    PIXIT_POINT divisor;

    pixit_point.x += p_redraw_context->pixit_origin.x;
    pixit_point.y += p_redraw_context->pixit_origin.y;

    multiplier.x = (MILLIPOINTS_PER_PIXIT * p_redraw_context->host_xform.scale.t.x);
    multiplier.y = (p_redraw_context->host_xform.scale.t.y);

    divisor.x = (p_redraw_context->host_xform.scale.b.x);
    divisor.y = p_redraw_context->host_xform.riscos.d_y * (PIXITS_PER_RISCOS * p_redraw_context->host_xform.scale.b.y);

    p_gdi_point->x = +muldiv64_round_floor(/*+*/pixit_point.x, multiplier.x, divisor.x);
    p_gdi_point->y = -muldiv64_round_floor(/*-*/pixit_point.y, multiplier.y, divisor.y);

    p_gdi_point->y <<= p_redraw_context->host_xform.riscos.eig_y;
    p_gdi_point->y *= MILLIPOINTS_PER_RISCOS;

    p_gdi_point->x += (p_redraw_context->gdi_org.x * MILLIPOINTS_PER_RISCOS);
    p_gdi_point->y += (p_redraw_context->gdi_org.y * MILLIPOINTS_PER_RISCOS);
}

extern void
host_fonty_text_paint_uchars_rubout(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_USTR uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background,
    _HfontRef_opt_ HOST_FONT host_font,
    _InRef_opt_ PC_PIXIT_RECT p_pixit_rect_rubout)
{
    STATUS status;
    GDI_POINT gdi_point;
    struct fontmanager_coords
    {
        int space_extra_x;
        int space_extra_y;
        int letter_extra_x;
        int letter_extra_y;
        BBox rubout;
    }
    coords;
    _kernel_swi_regs rs;
    GDI_RECT rubout_gdi_rect;
    CODE_ANALYSIS_ONLY(zero_struct(rubout_gdi_rect));

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_fonty_text_paint_uchars_rubout fgcol(r,g,b,t)=(") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
            p_rgb_foreground->r, p_rgb_foreground->g, p_rgb_foreground->b, p_rgb_foreground->transparent);
    trace_4(TRACE_APP_HOST_PAINT, TEXT("    bgcol(r,g,b,t)=(") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
            p_rgb_background->r, p_rgb_background->g, p_rgb_background->b, p_rgb_background->transparent);

    if(status_fail(status = host_setfontcolours(p_rgb_foreground, p_rgb_background)))
        return;

    gdi_point_mp_from_pixit_point_and_context(&gdi_point, p_pixit_point, p_redraw_context);

    rs.r[1] = (int) uchars;
    rs.r[2] = FONT_PAINT_USE_LENGTH /*r7*/ | FONT_PAINT_RUBOUT; /* rubout now always required (see simple below) */
    rs.r[7] = (int) uchars_n;

    if(HOST_FONT_NONE != host_font)
    {
        rs.r[0] = host_font;
        rs.r[2] |= FONT_PAINT_USE_HANDLE /*r0*/;
    }

    if(host_version_font_m_read(HOST_FONT_KERNING))
        rs.r[2] |= FONT_PAINT_KERNING;

    /* clip close to GDI limits (only ok because it's a rectangle) */
    if(!status_done(gdi_rect_limited_from_pixit_rect_and_context(&rubout_gdi_rect, p_pixit_rect_rubout, p_redraw_context)))
    {   /* rubout box is outwith GDI - don't use it */
        rs.r[2] &= ~FONT_PAINT_RUBOUT;
    }
    else /*if(host_version_font_m_read(HOST_FONT_RUBOUTBLOCK))*/
    {
        coords.space_extra_x = 0;
        coords.space_extra_y = 0;
        coords.letter_extra_x = 0;
        coords.letter_extra_y = 0;

        coords.rubout.xmin = (rubout_gdi_rect.tl.x * MILLIPOINTS_PER_RISCOS);
        coords.rubout.ymin = (rubout_gdi_rect.br.y * MILLIPOINTS_PER_RISCOS);
        coords.rubout.xmax = (rubout_gdi_rect.br.x * MILLIPOINTS_PER_RISCOS);
        coords.rubout.ymax = (rubout_gdi_rect.tl.y * MILLIPOINTS_PER_RISCOS);

        rs.r[2] |= FONT_PAINT_RUBOUTBLOCK;
        rs.r[5] = (int) &coords;
    }

    rs.r[3] = gdi_point.x; /* NB coordinates are in millipoints */
    rs.r[4] = gdi_point.y;

    if(NULL != WrapOsErrorChecking(_kernel_swi(/*Font_Paint*/ 0x40086, &rs, &rs)))
        /*ERR_FONT_PAINT*/;
}

extern void
host_fonty_text_paint_uchars_simple(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_USTR uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background,
    _HfontRef_opt_ HOST_FONT host_font,
    _InVal_     int text_align_lcr)
{
    STATUS status;
    GDI_POINT gdi_point;
    _kernel_swi_regs rs;

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_fonty_text_paint_uchars_simple fgcol(r,g,b,t)=(") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
            p_rgb_foreground->r, p_rgb_foreground->g, p_rgb_foreground->b, p_rgb_foreground->transparent);
    trace_4(TRACE_APP_HOST_PAINT, TEXT("    bgcol(r,g,b,t)=(") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
            p_rgb_background->r, p_rgb_background->g, p_rgb_background->b, p_rgb_background->transparent);

    if(status_fail(status = host_setfontcolours(p_rgb_foreground, p_rgb_background)))
        return;

    gdi_point_mp_from_pixit_point_and_context(&gdi_point, p_pixit_point, p_redraw_context);

    if(TA_LEFT != text_align_lcr)
    {
        S32 mp_width = 0;
        if(HOST_FONT_NONE != host_font)
            mp_width = host_fonty_uchars_width_mp(host_font, uchars, uchars_n);
        if(TA_CENTER == text_align_lcr)
            gdi_point.x -= mp_width / 2;
        else /*if(TA_RIGHT == text_align_lcr)*/
            gdi_point.x -= mp_width;
    }

    rs.r[1] = (int) uchars;
    rs.r[2] = FONT_PAINT_USE_LENGTH /*r7*/;
    rs.r[3] = gdi_point.x;
    rs.r[4] = gdi_point.y;
    rs.r[7] = (int) uchars_n;

    if(HOST_FONT_NONE != host_font)
    {
        rs.r[0] = host_font;
        rs.r[2] |= FONT_PAINT_USE_HANDLE /*r0*/;
    }

    if(host_version_font_m_read(HOST_FONT_KERNING))
        rs.r[2] |= FONT_PAINT_KERNING;

    if(NULL != WrapOsErrorChecking(_kernel_swi(/*Font_Paint*/ 0x40086, &rs, &rs)))
        /*ERR_FONT_PAINT*/;
}

extern void
host_paint_plain_text_counted(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_RGB p_rgb)
{
    GDI_POINT gdi_point;

    gdi_point_from_pixit_point_and_context(&gdi_point, p_pixit_point, p_redraw_context);

    if(host_setfgcolour(p_rgb))
    {
        void_WrapOsErrorChecking(
            bbc_move(gdi_point.x, (gdi_point.y - 4))); /* VDU 5 text has always been a strange one; 4 is correct, dy is not */

        void_WrapOsErrorChecking(
            os_writeN((const char *) uchars, (int) uchars_n));
    }
}

#if defined(UNUSED_KEEP_ALIVE)

extern void
host_fonty_text_paint_uchars_in_framed_box(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _In_        FRAMED_BOX_STYLE border_style,
    _InVal_     S32 fill_wimpcolour,
    _InVal_     S32 text_wimpcolour,
    _HfontRef_opt_ HOST_FONT host_font)
{
    GDI_RECT gdi_rect;
    GDI_BOX gdi_box, clip_box;

    if(!status_done(gdi_rect_limited_from_pixit_rect_and_context(&gdi_rect, p_pixit_rect, p_redraw_context)))
        return;

    gdi_box.x0 = gdi_rect.tl.x;
    gdi_box.y0 = gdi_rect.br.y;
    gdi_box.x1 = gdi_rect.br.x;
    gdi_box.y1 = gdi_rect.tl.y;

    host_framed_box_paint_frame(&gdi_box, border_style);

    host_framed_box_trim_frame(&gdi_box, border_style);

    if(fill_wimpcolour >= 0)
        host_framed_box_paint_core(&gdi_box, &rgb_stash[fill_wimpcolour]);

    /* never set a clip rect outside the parent clip rect */
    clip_box.x0 = MAX(gdi_box.x0, p_redraw_context->riscos.host_machine_clip_box.x0);
    clip_box.y0 = MAX(gdi_box.y0, p_redraw_context->riscos.host_machine_clip_box.y0);
    clip_box.x1 = MIN(gdi_box.x1, p_redraw_context->riscos.host_machine_clip_box.x1);
    clip_box.y1 = MIN(gdi_box.y1, p_redraw_context->riscos.host_machine_clip_box.y1);

    if((clip_box.x0 < clip_box.x1) && (clip_box.y0 < clip_box.y1))
    {
        /* NB host_ploticon() plots window relative */
        WimpIconBlockWithBitset icon;
        BOOL plot_icon = FALSE;

        icon.bbox.xmin = (gdi_box.x0 - p_redraw_context->gdi_org.x);
        icon.bbox.ymin = (gdi_box.y0 - p_redraw_context->gdi_org.y);
        icon.bbox.xmax = (gdi_box.x1 - p_redraw_context->gdi_org.x);
        icon.bbox.ymax = (gdi_box.y1 - p_redraw_context->gdi_org.y);

        icon.flags.u32 = 0;

        /*if(!p_rgb_fill->transparent)*/
        {
            /*int fill_wimpcolour = host_find_rgb_in_stash(p_rgb_fill);*/

            if(fill_wimpcolour >= 0)
            {
                icon.flags.bits.filled    = 1;
                icon.flags.bits.bg_colour = fill_wimpcolour;

                plot_icon = TRUE;
            }
        }

        if(0 != uchars_n)
        {
            icon.flags.bits.text        = 1;
            icon.flags.bits.horz_centre = 1;
            icon.flags.bits.vert_centre = 1;
            icon.flags.bits.indirect    = 1;
            icon.flags.bits.fg_colour   = (int) text_wimpcolour;

            icon.data.it.buffer = de_const_cast(P_U8, uchars);
            icon.data.it.validation = NULL;
            icon.data.it.buffer_size = uchars_n;

            plot_icon = TRUE;
        }

        if(plot_icon)
        {
            trace_4(TRACE_APP_HOST_PAINT, TEXT("host_fonty_text_paint_uchars_in_framed_box: gw ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT, clip_box.x0, clip_box.y0, clip_box.x1, clip_box.y1);

            void_WrapOsErrorChecking(
                riscos_vdu_define_graphics_window(
                    clip_box.x0,
                    clip_box.y0,
                    clip_box.x1 - 1 /* clip near pixel edge for printer drivers */,
                    clip_box.y1 - 1));

            if(HOST_FONT_NONE == host_font)
            {
                host_ploticon(&icon);
            }
            else
            {
                /* plot background first */
                icon.flags.bits.text = 0;

                host_ploticon(&icon);

                /* then plot text */
                icon.flags.bits.text   = 1;
                icon.flags.bits.filled = 0;
                icon.flags.bits.font   = 1;

                ((P_U8) &icon.flags.bits)[3] = (U8) host_font;

                host_ploticon(&icon);
            }

            host_restore_clip_rectangle(p_redraw_context);
        }
    }
}

#endif /* UNUSED_KEEP_ALIVE */

_Check_return_
static PIXIT
host_font_ascent(
    _HfontRef_  HOST_FONT host_font,
    _InVal_     int this_character)
{
    PIXIT ascent = 0;
    _kernel_swi_regs rs;
    _kernel_oserror * p_kernel_oserror;

    rs.r[0] = host_font;
    rs.r[1] = this_character;
    rs.r[2] = FONT_PAINT_OSCOORDS;
    if(NULL == (p_kernel_oserror = _kernel_swi(/*Font_CharBBox*/ 0x04008E, &rs, &rs)))
        ascent = abs(rs.r[4]) * PIXITS_PER_RISCOS;

    if(0 == ascent)
    {   /* Caters for silly techie fonts with only a couple of symbols defined */
        rs.r[0] = host_font;
        if(NULL == (p_kernel_oserror = _kernel_swi(/*Font_ReadInfo*/ 0x040084, &rs, &rs)))
        {
            if(0 == ascent)
                ascent = abs(rs.r[4]) * PIXITS_PER_RISCOS;
        }
    }

    return(ascent);
}

extern void
host_fonty_text_paint_uchars_in_rectangle(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_BORDER_FLAGS p_border_flags,
    _InRef_     PC_RGB p_rgb_fill,
    _InRef_     PC_RGB p_rgb_line,
    _InRef_     PC_RGB p_rgb_text,
    _HfontRef_opt_ HOST_FONT host_font)
{
    GDI_RECT gdi_rect;
    GDI_BOX gdi_box, clip_box;
    int modifier;

    if(!status_done(gdi_rect_limited_from_pixit_rect_and_context(&gdi_rect, p_pixit_rect, p_redraw_context)))
        return;

    gdi_box.x0 = gdi_rect.tl.x;
    gdi_box.y0 = gdi_rect.br.y;
    gdi_box.x1 = gdi_rect.br.x;
    gdi_box.y1 = gdi_rect.tl.y;


    /* never set a clip rect outside the parent clip rect */
    clip_box.x0 = MAX(gdi_box.x0, p_redraw_context->riscos.host_machine_clip_box.x0);
    clip_box.y0 = MAX(gdi_box.y0, p_redraw_context->riscos.host_machine_clip_box.y0);
    clip_box.x1 = MIN(gdi_box.x1, p_redraw_context->riscos.host_machine_clip_box.x1);
    clip_box.y1 = MIN(gdi_box.y1, p_redraw_context->riscos.host_machine_clip_box.y1);

    if((clip_box.x0 < clip_box.x1) && (clip_box.y0 < clip_box.y1))
    {
        trace_4(TRACE_APP_HOST_PAINT, TEXT("host_fonty_text_paint_uchars_in_rectangle: gw ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT, clip_box.x0, clip_box.y0, clip_box.x1, clip_box.y1);

        void_WrapOsErrorChecking(
            riscos_vdu_define_graphics_window(
                clip_box.x0,
                clip_box.y0,
                clip_box.x1 - 1 /* clip near pixel edge for printer drivers */,
                clip_box.y1 - 1));

        /* make inc,inc inc,inc */
        gdi_rect.br.x -= p_redraw_context->host_xform.riscos.d_x;
        gdi_rect.tl.y -= p_redraw_context->host_xform.riscos.d_y;

#if 1
        /* all the rectangle is filled with this (background) colour */
        if(host_setbgcolour(p_rgb_fill))
            host_clg();
#else

        void_WrapOsErrorChecking(
            bbc_move(gdi_rect.tl.x, gdi_rect.br.y));

        void_WrapOsErrorChecking(
            os_plot(host_setcolour(p_rgb_fill) | bbc_MoveCursorAbs | bbc_RectangleFill,
                    (gdi_rect.br.x + p_redraw_context->host_xform.riscos.d_x - 1 /* fill to pixel edge for printer drivers */),
                    (gdi_rect.tl.y + p_redraw_context->host_xform.riscos.d_y - 1)));
#endif

#if 1
        {
        PIXIT_POINT pixit_point;
        pixit_point.x = (p_pixit_rect->br.x + p_pixit_rect->tl.x) / 2;
        pixit_point.y = (p_pixit_rect->br.y + p_pixit_rect->tl.y) / 2;
        pixit_point.y += host_font_ascent(host_font, '0') / 2;
        host_fonty_text_paint_uchars_simple(p_redraw_context, &pixit_point, uchars, uchars_n, p_rgb_text, p_rgb_fill, host_font, TA_CENTER);
        } /*block*/
#else
        if(p_redraw_context->flags.printer)
        {
            if(0 != uchars_n)
            {
                GDI_POINT gdi_point;

                gdi_point.x = (gdi_rect.tl.x + gdi_rect.br.x - 16*(S32)uchars_n) / 2;
                gdi_point.y = (gdi_rect.br.y + gdi_rect.tl.y + 32              ) / 2 -  4; /* VDU 5 text has always been a strange one; 4 is correct, dy is not */

                if(host_setfgcolour(&rgb_stash[text_wimpcolour]))
                {
                    void_WrapOsErrorChecking(
                        bbc_move(gdi_point.x, gdi_point.y));

                    void_WrapOsErrorChecking(
                        os_writeN((const char *) uchars, uchars_n));
                }
            }
        }
        else
        {
            WimpIconBlockWithBitset icon;

            /* NB host_ploticon() plots window relative */
            host_ploticon_setup_bbox(&icon, p_pixit_rect, p_redraw_context);

            icon.flags.u32 = 0;
            icon.flags.bits.text         = 1;
            icon.flags.bits.horz_centre  = 1;
            icon.flags.bits.vert_centre  = 1;
          /*icon.flags.bits.filled       = 1;*/
            icon.flags.bits.indirect     = 1;
            icon.flags.bits.fg_colour    = (int) text_wimpcolour;
            icon.flags.bits.bg_colour    = (int) fill_wimpcolour;

            icon.data.it.buffer = de_const_cast(P_U8, uchars);
            icon.data.it.validation = NULL;
            icon.data.it.buffer_size = uchars_n;

            if(HOST_FONT_NONE == host_font)
            {
                host_ploticon(&icon);
            }
            else
            {
                /* plot background first */
                /*icon.flags.bits.text = 0;*/

                /*host_ploticon(&icon);*/

                if(0 != uchars_n)
                {
                    /* then plot text */
                    icon.flags.bits.text   = 1;
                    icon.flags.bits.filled = 0;
                    icon.flags.bits.font   = 1;

                    ((P_U8) &icon.flags.bits)[3] = (U8) host_font;

                    host_ploticon(&icon);
                }
            }
        }
#endif

        if( p_border_flags->left.show   ||
            p_border_flags->right.show  ||
            p_border_flags->top.show    ||
            p_border_flags->bottom.show )
        {
            modifier = host_setcolour(p_rgb_line);

            if(p_border_flags->left.show)
            {
                void_WrapOsErrorChecking(
                    bbc_move(gdi_rect.tl.x, gdi_rect.tl.y));

                void_WrapOsErrorChecking(
                    os_plot(modifier | bbc_MoveCursorAbs | (p_border_flags->left.dashed   ? bbc_DottedBoth : bbc_SolidBoth),
                            gdi_rect.tl.x, gdi_rect.br.y));
            }

            if(p_border_flags->right.show)
            {
                void_WrapOsErrorChecking(
                    bbc_move(gdi_rect.br.x, gdi_rect.tl.y));

                void_WrapOsErrorChecking(
                    os_plot(modifier | bbc_MoveCursorAbs | (p_border_flags->right.dashed  ? bbc_DottedBoth : bbc_SolidBoth),
                            gdi_rect.br.x, gdi_rect.br.y));
            }

            if(p_border_flags->top.show)
            {
                void_WrapOsErrorChecking(
                    bbc_move(gdi_rect.tl.x, gdi_rect.tl.y));

                void_WrapOsErrorChecking(
                    os_plot(modifier | bbc_MoveCursorAbs | (p_border_flags->top.dashed    ? bbc_DottedBoth : bbc_SolidBoth),
                            gdi_rect.br.x, gdi_rect.tl.y));
            }

            if(p_border_flags->bottom.show)
            {
                void_WrapOsErrorChecking(
                    bbc_move(gdi_rect.tl.x, gdi_rect.br.y));

                void_WrapOsErrorChecking(
                    os_plot(modifier | bbc_MoveCursorAbs | (p_border_flags->bottom.dashed ? bbc_DottedBoth : bbc_SolidBoth),
                            gdi_rect.br.x, gdi_rect.br.y));
            }
        }
    }

    host_restore_clip_rectangle(p_redraw_context);
}

/*
* NB start is inclusive, end exclusive
*/

extern void
host_paint_line_dashed(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_line,
    _InRef_     PC_RGB p_rgb)
{
    GDI_POINT start, end;

    if(p_rgb->transparent)
        return;

    gdi_point_from_pixit_point_and_context(&start, &p_line->tl, p_redraw_context);
    gdi_point_from_pixit_point_and_context(&end,   &p_line->br, p_redraw_context);

    void_WrapOsErrorChecking(
        bbc_move(start.x, start.y));

    void_WrapOsErrorChecking(
        os_plot(host_setcolour(p_rgb) | bbc_MoveCursorAbs | bbc_DottedExFinal,
                end.x, end.y));
}

/* used to render the ruler */

/*
* NB start is inclusive, end exclusive
*/

extern void
host_paint_line_solid(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb)
{
    PIXIT_RECT pixit_rect;
    GDI_RECT gdi_rect;
    GDI_POINT start, end;

    if(p_rgb->transparent)
        return;

    assert(!p_redraw_context->flags.printer);

    pixit_rect.tl = p_pixit_line->tl;
    pixit_rect.br = p_pixit_line->br;

    if(p_pixit_line->horizontal)
        pixit_rect.br.y = pixit_rect.tl.y + p_redraw_context->one_program_pixel.y;
    else
        pixit_rect.br.x = pixit_rect.tl.x + p_redraw_context->one_program_pixel.x;

    /* clip close to GDI limits (only OK because lines either horizontal or vertical) */
    if(!status_done(gdi_rect_limited_from_pixit_rect_and_context(&gdi_rect, &pixit_rect, p_redraw_context)))
        return;

    start.x = gdi_rect.tl.x; /*inc*/
    start.y = gdi_rect.br.y; /*inc*/
    end.x   = gdi_rect.br.x; /*exc*/
    end.y   = gdi_rect.tl.y; /*exc*/

    end.x -= p_redraw_context->host_xform.riscos.d_x; /*exc->inc*/
    end.y -= p_redraw_context->host_xform.riscos.d_y;

    /* ruler lines always drawn very thin on screen, never scaled */
    if(p_pixit_line->horizontal)
        start.y = end.y;
    else
        end.x = start.x;

    void_WrapOsErrorChecking(
        bbc_move(start.x, start.y));

    void_WrapOsErrorChecking(
        os_plot(host_setcolour(p_rgb) | bbc_MoveCursorAbs | bbc_RectangleFill,
                (end.x + p_redraw_context->host_xform.riscos.d_x - 1 /* fill out to pixel edge for printer drivers */),
                (end.y + p_redraw_context->host_xform.riscos.d_y - 1)));
}

extern void
host_paint_rectangle_filled(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_RGB p_rgb)
{
    GDI_RECT gdi_rect;

    if(p_rgb->transparent)
        return;

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_rectangle_filled: rect tl ") S32_TFMT TEXT(",") S32_TFMT TEXT(" br ") S32_TFMT TEXT(",") S32_TFMT, p_pixit_rect->tl.x, p_pixit_rect->tl.y, p_pixit_rect->br.x, p_pixit_rect->br.y);

    if(p_redraw_context->flags.drawfile)
    {
        drawfile_paint_rectangle_filled(p_redraw_context, p_pixit_rect, p_rgb);
        return;
    }

    if(!status_done(gdi_rect_limited_from_pixit_rect_and_context(&gdi_rect, p_pixit_rect, p_redraw_context)))
        return;

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_paint_rectangle_filled: rect ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT, gdi_rect.tl.x, gdi_rect.tl.y, gdi_rect.br.x, gdi_rect.br.y);

    void_WrapOsErrorChecking(
        bbc_move(gdi_rect.tl.x, gdi_rect.br.y)); /* fill to pixel edge for printer drivers */

    void_WrapOsErrorChecking(
        os_plot(host_setcolour(p_rgb) | bbc_MoveCursorAbs | bbc_RectangleFill,
                (gdi_rect.br.x - 1), (gdi_rect.tl.y - 1)));
}

extern void
host_invert_rectangle_filled(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background)
{
    GDI_RECT gdi_rect;
    RISCOS_PALETTE_U os_rgb_foreground;
    RISCOS_PALETTE_U os_rgb_background;

    if(!status_done(gdi_rect_from_pixit_rect_and_context(&gdi_rect, p_pixit_rect, p_redraw_context)))
        return;

    /* clip close to GDI limits */
    if(gdi_rect.tl.x < -0x7FF0) gdi_rect.tl.x = -0x7FF0;
    if(gdi_rect.tl.y > +0x7FF0) gdi_rect.tl.y = +0x7FF0;
    if(gdi_rect.br.x > +0x7FF0) gdi_rect.br.x = +0x7FF0;
    if(gdi_rect.br.y < -0x7FF0) gdi_rect.br.y = -0x7FF0;

    os_rgb_foreground.bytes.gcol  = 0;
    os_rgb_foreground.bytes.red   = p_rgb_foreground->r;
    os_rgb_foreground.bytes.green = p_rgb_foreground->g;
    os_rgb_foreground.bytes.blue  = p_rgb_foreground->b;

    os_rgb_background.bytes.gcol  = 0;
    os_rgb_background.bytes.red   = p_rgb_background->r;
    os_rgb_background.bytes.green = p_rgb_background->g;
    os_rgb_background.bytes.blue  = p_rgb_background->b;

    { /* New machines usually demand this mechanism */
    int colnum_foreground = colourtrans_ReturnColourNumber(os_rgb_foreground.word);
    int colnum_background = colourtrans_ReturnColourNumber(os_rgb_background.word);
    _kernel_swi_regs rs;
    rs.r[0] = 3;
    rs.r[1] = colnum_foreground ^ colnum_background;
    void_WrapOsErrorChecking(_kernel_swi(OS_SetColour, &rs, &rs));
    } /*block*/

    host_invalidate_cache(HIC_FG);

    void_WrapOsErrorChecking(
        bbc_move(gdi_rect.tl.x, gdi_rect.br.y));

    void_WrapOsErrorChecking(
        os_plot(bbc_DrawAbsFore | bbc_RectangleFill,
                (gdi_rect.br.x - 1), (gdi_rect.tl.y - 1)));
}

extern void
host_paint_rectangle_outline(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_RGB p_rgb)
{
    GDI_RECT gdi_rect;
    int modifier;

    if(p_rgb->transparent)
        return;

    if(!status_done(gdi_rect_from_pixit_rect_and_context(&gdi_rect, p_pixit_rect, p_redraw_context)))
        return;

    /* clip close to GDI limits */
    if(gdi_rect.tl.x < -0x7FF0) gdi_rect.tl.x = -0x7FF0;
    if(gdi_rect.tl.y > +0x7FF0) gdi_rect.tl.y = +0x7FF0;
    if(gdi_rect.br.x > +0x7FF0) gdi_rect.br.x = +0x7FF0;
    if(gdi_rect.br.y < -0x7FF0) gdi_rect.br.y = -0x7FF0;

    /* make inc,inc inc,inc */
    gdi_rect.br.x -= p_redraw_context->host_xform.riscos.d_x;
    gdi_rect.tl.y -= p_redraw_context->host_xform.riscos.d_y;

    modifier = host_setcolour(p_rgb);

    void_WrapOsErrorChecking(
        bbc_move(gdi_rect.tl.x, gdi_rect.tl.y));

    if(gdi_rect.tl.y == gdi_rect.br.y)
    {
        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidBoth,   gdi_rect.br.x, gdi_rect.tl.y));
    }
    else if(gdi_rect.tl.x == gdi_rect.br.x)
    {
        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidBoth,   gdi_rect.tl.x, gdi_rect.br.y));
    }
    else
    {
        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidExInit, gdi_rect.br.x, gdi_rect.tl.y));
        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidExInit, gdi_rect.br.x, gdi_rect.br.y));
        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidExInit, gdi_rect.tl.x, gdi_rect.br.y));
        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidExInit, gdi_rect.tl.x, gdi_rect.tl.y));
    }
}

extern void
host_paint_rectangle_crossed(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_RGB p_rgb)
{
    GDI_RECT gdi_rect;
    int modifier;

    if(p_rgb->transparent)
        return;

    if(!status_done(gdi_rect_from_pixit_rect_and_context(&gdi_rect, p_pixit_rect, p_redraw_context)))
        return;

    if(gdi_rect.tl.x < -0x7FF0) gdi_rect.tl.x = -0x7FF0;
    if(gdi_rect.tl.y > +0x7FF0) gdi_rect.tl.y = +0x7FF0;
    if(gdi_rect.br.x > +0x7FF0) gdi_rect.br.x = +0x7FF0;
    if(gdi_rect.br.y < -0x7FF0) gdi_rect.br.y = -0x7FF0;

    /* make inc,inc inc,inc */
    gdi_rect.br.x -= p_redraw_context->host_xform.riscos.d_x;
    gdi_rect.tl.y -= p_redraw_context->host_xform.riscos.d_y;

    modifier = host_setcolour(p_rgb);

    void_WrapOsErrorChecking(
        bbc_move(gdi_rect.tl.x, gdi_rect.tl.y));

    if(gdi_rect.tl.y == gdi_rect.br.y)
    {
        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidBoth,   gdi_rect.br.x, gdi_rect.tl.y));
    }
    else if(gdi_rect.tl.x == gdi_rect.br.x)
    {
        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidBoth,   gdi_rect.tl.x, gdi_rect.br.y));
    }
    else
    {
        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidExInit, gdi_rect.br.x, gdi_rect.tl.y));
        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidExInit, gdi_rect.br.x, gdi_rect.br.y));
        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidExInit, gdi_rect.tl.x, gdi_rect.br.y));
        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidExInit, gdi_rect.tl.x, gdi_rect.tl.y));

        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidExInit, gdi_rect.br.x, gdi_rect.br.y));

        void_WrapOsErrorChecking(
            bbc_move(gdi_rect.tl.x, gdi_rect.br.y));

        void_WrapOsErrorChecking(
            os_plot(modifier | bbc_MoveCursorAbs | bbc_SolidExInit, gdi_rect.br.x, gdi_rect.tl.y));
    }
}

extern void
host_invert_rectangle_outline(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background)
{
    GDI_RECT gdi_rect;
    RISCOS_PALETTE_U os_rgb_foreground;
    RISCOS_PALETTE_U os_rgb_background;

    if(!status_done(gdi_rect_from_pixit_rect_and_context(&gdi_rect, p_pixit_rect, p_redraw_context)))
        return;

    if(gdi_rect.tl.x < -0x7FF0) gdi_rect.tl.x = -0x7FF0;
    if(gdi_rect.tl.y > +0x7FF0) gdi_rect.tl.y = +0x7FF0;
    if(gdi_rect.br.x > +0x7FF0) gdi_rect.br.x = +0x7FF0;
    if(gdi_rect.br.y < -0x7FF0) gdi_rect.br.y = -0x7FF0;

    /* make inc,inc inc,inc */
    gdi_rect.br.x -= p_redraw_context->host_xform.riscos.d_x;
    gdi_rect.tl.y -= p_redraw_context->host_xform.riscos.d_y;

    os_rgb_foreground.bytes.gcol  = 0;
    os_rgb_foreground.bytes.red   = p_rgb_foreground->r;
    os_rgb_foreground.bytes.green = p_rgb_foreground->g;
    os_rgb_foreground.bytes.blue  = p_rgb_foreground->b;

    os_rgb_background.bytes.gcol  = 0;
    os_rgb_background.bytes.red   = p_rgb_background->r;
    os_rgb_background.bytes.green = p_rgb_background->g;
    os_rgb_background.bytes.blue  = p_rgb_background->b;

    { /* New machines usually demand this mechanism */
    int colnum_foreground = colourtrans_ReturnColourNumber(os_rgb_foreground.word);
    int colnum_background = colourtrans_ReturnColourNumber(os_rgb_background.word);
    _kernel_swi_regs rs;
    rs.r[0] = 3;
    rs.r[1] = colnum_foreground ^ colnum_background;
    void_WrapOsErrorChecking(_kernel_swi(OS_SetColour, &rs, &rs));
    } /*block*/

    host_invalidate_cache(HIC_FG);

    void_WrapOsErrorChecking(
        bbc_move(gdi_rect.tl.x, gdi_rect.tl.y));

    if(     gdi_rect.tl.y == gdi_rect.br.y)
    {
        void_WrapOsErrorChecking(
            os_plot(bbc_DrawAbsFore | bbc_SolidBoth,   gdi_rect.br.x, gdi_rect.tl.y));
    }
    else if(gdi_rect.tl.x == gdi_rect.br.x)
    {
        void_WrapOsErrorChecking(
            os_plot(bbc_DrawAbsFore | bbc_SolidBoth,   gdi_rect.tl.x, gdi_rect.br.y));
    }
    else
    {
        void_WrapOsErrorChecking(
            os_plot(bbc_DrawAbsFore | bbc_SolidExInit, gdi_rect.br.x, gdi_rect.tl.y));
        void_WrapOsErrorChecking(
            os_plot(bbc_DrawAbsFore | bbc_SolidExInit, gdi_rect.br.x, gdi_rect.br.y));
        void_WrapOsErrorChecking(
            os_plot(bbc_DrawAbsFore | bbc_SolidExInit, gdi_rect.tl.x, gdi_rect.br.y));
        void_WrapOsErrorChecking(
            os_plot(bbc_DrawAbsFore | bbc_SolidExInit, gdi_rect.tl.x, gdi_rect.tl.y));
    }
}

/*
calculate super- / sub- shifts for RISC OS Font Manager
*/

extern PIXIT /* NB Cartesian y */
riscos_fonty_paint_calc_shift_y(
    _InRef_     PC_FONT_CONTEXT p_font_context)
{
    if(p_font_context->font_spec.superscript)
        return(+((p_font_context->font_spec.size_y * 24) / 64));

    if(p_font_context->font_spec.subscript)
        return(-((p_font_context->font_spec.size_y * 9 ) / 64));

    return(0);
}

_Check_return_
extern STATUS
riscos_fonty_paint_shift_x(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     PIXIT shift_x,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    S32 shift_scaled = scale_pixit_x(shift_x, &p_redraw_context->host_xform);
    UCHARB shift_seq[4];
    shift_seq[0] = 9;
    shift_seq[1] = (BYTE) ((shift_scaled      ) & 0xFF);
    shift_seq[2] = (BYTE) ((shift_scaled >>  8) & 0xFF);
    shift_seq[3] = (BYTE) ((shift_scaled >> 16) & 0xFF);
    return(quick_ublock_uchars_add(p_quick_ublock, shift_seq, elemof32(shift_seq)));
}

_Check_return_
extern STATUS
riscos_fonty_paint_shift_y(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     PIXIT shift_y,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    S32 shift_scaled = scale_pixit_y(shift_y, &p_redraw_context->host_xform);
    UCHARB shift_seq[4];
    shift_seq[0] = 11;
    shift_seq[1] = (BYTE) ((shift_scaled      ) & 0xFF);
    shift_seq[2] = (BYTE) ((shift_scaled >>  8) & 0xFF);
    shift_seq[3] = (BYTE) ((shift_scaled >> 16) & 0xFF);
    return(quick_ublock_uchars_add(p_quick_ublock, shift_seq, elemof32(shift_seq)));
}

_Check_return_
extern STATUS
riscos_fonty_paint_underline(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/)
{
    static const UCHARB underline_seq[] = { 25, 230, 14 };
    return(quick_ublock_uchars_add(p_quick_ublock, underline_seq, elemof32(underline_seq)));
}

_Check_return_
extern STATUS
riscos_fonty_paint_font_change(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _HfontRef_  HOST_FONT host_font)
{
    UCHARB font_seq[2];
    font_seq[0] = 26;
    font_seq[1] = (BYTE) host_font;
    return(quick_ublock_uchars_add(p_quick_ublock, font_seq, elemof32(font_seq)));
}

extern void
fonty_text_paint_simple_uchars(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     S32 uchars_n,
    _InVal_     PIXIT base_line,
    _InRef_     PC_RGB p_rgb_back,
    _InVal_     FONTY_HANDLE fonty_handle)
{
    PIXIT_POINT pixit_point = *p_pixit_point;
    const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);
    PIXIT shift_y;
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
    quick_ublock_with_buffer_setup(quick_ublock);

    pixit_point.y += base_line;

    if(p_redraw_context->flags.drawfile)
    {
        pixit_point.y += drawfile_fonty_paint_calc_shift_y(p_font_context);
        drawfile_paint_uchars(p_redraw_context, &pixit_point, uchars, uchars_n, p_rgb_back, p_font_context);
        return;
    }

    shift_y = riscos_fonty_paint_calc_shift_y(p_font_context);

    for(;;) /* loop for structure */
    {
        HOST_FONT host_font = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, p_docu->flags.draft_mode);
        U32 out_len;

        //status_break(status = riscos_fonty_paint_font_change(&quick_ublock, host_font));

        /* output any y shift to fonty string (so we don't get affected by our own pixel rounding) */
        if(0 != shift_y)
            status_break(status = riscos_fonty_paint_shift_y(&quick_ublock, shift_y * MILLIPOINTS_PER_PIXIT, p_redraw_context));

        /* switch underline on */
        if(p_font_context->font_spec.underline)
            status_break(status = riscos_fonty_paint_underline(&quick_ublock));

        status_break(status = quick_ublock_uchars_add(&quick_ublock, uchars, uchars_n));

        out_len = quick_ublock_bytes(&quick_ublock);

        status_break(status = quick_ublock_nullch_add(&quick_ublock));

        host_fonty_text_paint_uchars_simple(p_redraw_context, &pixit_point, quick_ublock_uchars(&quick_ublock), out_len, &p_font_context->font_spec.colour, p_rgb_back, host_font, TA_LEFT);

        break; /* out of loop for structure */
        /*NOTREACHED*/
    }

    status_assert(status);

    quick_ublock_dispose(&quick_ublock);
}

/******************************************************************************
*
* Set a machine colour closest to the supplied RGB colour.
*
* The 'graphics foreground' or 'graphics background' colour (chosen on a least recently used basis)
* is set to the colour which closest matches the supplied RGB value.
*
* The colour chosen is indicated by the value returned as a modifier:
*   1 for 'graphics foreground',
*   3 for 'graphics background',
*
* this should be ORed into the base value of the required plotcode prior to calling os_plot().
*
******************************************************************************/

_Check_return_
static int
host_setcolour(
    _InRef_     PC_RGB p_rgb)
{
    U32 rgb_key = (U32) p_rgb->r + ((U32) p_rgb->g << 8) + ((U32) p_rgb->b << 16);

    assert(!p_rgb->transparent); /* callers MUST check for transparent */

    if(cache.fg.rgb_key == rgb_key)
    {   /* foreground colour already set to give a 'good match' to required RGB */
        cache.fg.use++;
        return(1 /* specify plot in foreground colour */);
    }

    if(cache.bg.rgb_key == rgb_key)
    {   /* background colour already set to give a 'good match' to required RGB */
        cache.bg.use++;
        return(3 /* specify plot in background colour */);
    }

    { /* cannot use GCOLs when printing or on new machines, must do... */
    P_HOST_CACHE_ENTRY p_cache_entry;
    RISCOS_PALETTE_U os_rgb;

    if(     cache.fg.rgb_key == 0xFFFFFFFFU)
        p_cache_entry = &cache.fg;
    else if(cache.bg.rgb_key == 0xFFFFFFFFU)
        p_cache_entry = &cache.bg;
    else
        p_cache_entry = (cache.fg.use < cache.bg.use) ? &cache.fg : &cache.bg;

#if FALSE /* SKS thinks some of the print using bg ops don't work using bg with Tony Cheal's RO2 DJ500C driver. Surely that's long past... */
    if(p_redraw_context->flags.printer)
        p_cache_entry = &cache.fg;
#endif

    os_rgb.bytes.gcol  = 0;
    os_rgb.bytes.red   = p_rgb->r;
    os_rgb.bytes.green = p_rgb->g;
    os_rgb.bytes.blue  = p_rgb->b;

    (void) colourtrans_SetGCOL(os_rgb.word, 0x100 /* SKS allow ECF */ | ((p_cache_entry == &cache.bg) ? 0x80 : 0x00 /*background:foreground*/), 0 /*GCol action is store*/);

    p_cache_entry->rgb_key = rgb_key;
    p_cache_entry->use = 1;
    return((p_cache_entry == &cache.bg) ? 3 : 1 /* plot background/forground */);
    } /*block*/
}

/******************************************************************************
*
* Set the 'graphics foreground' colour to the
* machine colour closest to the supplied RGB colour
*
* Similar to host_setcolour(), to be used by
* graphics operations such as drawmod_stroke that
* can only render in graphics foreground colour
*
******************************************************************************/

_Check_return_
static BOOL
host_setcolour_in_this_cache(
    _InRef_     PC_RGB p_rgb,
    _InoutRef_  P_HOST_CACHE_ENTRY p_cache_entry)
{
    U32 rgb_key = (U32) p_rgb->r + ((U32) p_rgb->g << 8) + ((U32) p_rgb->b << 16);

    if(p_rgb->transparent)
        return(FALSE);

    if(p_cache_entry->rgb_key == rgb_key)
    {   /* fore/background colour already set to required RGB */
        p_cache_entry->use++;
        return(TRUE);
    }

    { /* cannot use GCOLs when printing or on new machines, must do... */
    RISCOS_PALETTE_U os_rgb;

    os_rgb.bytes.gcol  = 0;
    os_rgb.bytes.red   = p_rgb->r;
    os_rgb.bytes.green = p_rgb->g;
    os_rgb.bytes.blue  = p_rgb->b;

    (void) colourtrans_SetGCOL(os_rgb.word, 0x100 /* SKS allow ECF */ | ((p_cache_entry == &cache.bg) ? 0x80 : 0x00 /*fore/background*/), 0 /*GCol action is store*/);

    p_cache_entry->rgb_key = rgb_key;
    p_cache_entry->use = 1;
    } /*block*/

    return(TRUE);
}

_Check_return_
extern BOOL
host_setfgcolour(
    _InRef_     PC_RGB p_rgb)
{
    return(host_setcolour_in_this_cache(p_rgb, &cache.fg));
}

_Check_return_
extern BOOL
host_setbgcolour(
    _InRef_     PC_RGB p_rgb)
{
    return(host_setcolour_in_this_cache(p_rgb, &cache.bg));
}

_Check_return_
static STATUS
host_setfontcolours(
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background)
{
    RISCOS_PALETTE_U rgb_fg;
    RISCOS_PALETTE_U rgb_bg;
    _kernel_swi_regs rs;

    assert(!p_rgb_foreground->transparent);
    rgb_fg.bytes.gcol  = 0;
    rgb_fg.bytes.red   = p_rgb_foreground->r;
    rgb_fg.bytes.green = p_rgb_foreground->g;
    rgb_fg.bytes.blue  = p_rgb_foreground->b;

    rgb_bg.bytes.gcol  = 0;
    if(!p_rgb_background->transparent)
    {
        rgb_bg.bytes.red   = p_rgb_background->r;
        rgb_bg.bytes.green = p_rgb_background->g;
        rgb_bg.bytes.blue  = p_rgb_background->b;
    }
    else
    {
        rgb_bg.bytes.red   = rgb_stash[COLOUR_OF_PAPER].r;
        rgb_bg.bytes.green = rgb_stash[COLOUR_OF_PAPER].g;
        rgb_bg.bytes.blue  = rgb_stash[COLOUR_OF_PAPER].b;
    }

    if(cache.font.set && (rgb_fg.word == cache.font.rgb_fg.word) && (rgb_bg.word == cache.font.rgb_bg.word))
        return(STATUS_OK);

    rs.r[0] = 0;
    rs.r[1] = rgb_bg.word;
    rs.r[2] = rgb_fg.word;
    rs.r[3] = 14; /* max offset - some magic number, !Draw uses 14 */

    if(_kernel_swi(ColourTrans_SetFontColours, &rs, &rs))
        return(ERR_FONT_COLOUR);

    cache.font.set = 1;
    cache.font.rgb_fg = rgb_fg;
    cache.font.rgb_bg = rgb_bg;

    return(STATUS_OK);
}

_Check_return_
extern STATUS
host_setfontcolours_for_mlec(
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background)
{
    RISCOS_PALETTE_U rgb_fg;
    RISCOS_PALETTE_U rgb_bg;
    _kernel_swi_regs rs;

    rgb_fg.bytes.gcol  = 0;
    rgb_fg.bytes.red   = p_rgb_foreground->r;
    rgb_fg.bytes.green = p_rgb_foreground->g;
    rgb_fg.bytes.blue  = p_rgb_foreground->b;

    rgb_bg.bytes.gcol  = 0;
    rgb_bg.bytes.red   = p_rgb_background->r;
    rgb_bg.bytes.green = p_rgb_background->g;
    rgb_bg.bytes.blue  = p_rgb_background->b;

    rs.r[0] = 0;
    rs.r[1] = rgb_bg.word;
    rs.r[2] = rgb_fg.word;
    rs.r[3] = 14; /* max offset - some magic number, !Draw uses 14 */

    if(_kernel_swi(ColourTrans_SetFontColours, &rs, &rs))
        return(ERR_FONT_COLOUR);

    cache.font.set = 0;

    return(STATUS_OK);
}

/* Read the pixit size of the Draw file, returning the width and height in pixits */

extern void
host_read_drawfile_pixit_size(
    /*_In_reads_bytes_(DRAW_FILE_HEADER)*/ PC_ANY p_any,
    _OutRef_    P_PIXIT_SIZE p_pixit_size)
{
    PC_DRAW_FILE_HEADER pDrawFileHdr = (PC_DRAW_FILE_HEADER) p_any;
    DRAW_POINT draw_size;

    draw_size.x = pDrawFileHdr->bbox.x1 - pDrawFileHdr->bbox.x0;
    draw_size.y = pDrawFileHdr->bbox.y1 - pDrawFileHdr->bbox.y0;

    /* flooring ok 'cos paths got rounded out on loading to worst-case pixels */
    p_pixit_size->cx = muldiv64_floor(draw_size.x, 1, 2 * RISCDRAW_PER_RISCOS) * 2 * PIXITS_PER_RISCOS;
    p_pixit_size->cy = muldiv64_floor(draw_size.y, 1, 2 * RISCDRAW_PER_RISCOS) * 2 * PIXITS_PER_RISCOS;
}

/******************************************************************************
*
* Set the shape of the mouse pointer
*
******************************************************************************/

typedef struct POINTER_INFO
{
    RESOURCE_BITMAP_ID id;
    GDI_POINT active_point_offset;
}
POINTER_INFO; typedef const POINTER_INFO * PC_POINTER_INFO;

static const POINTER_INFO
pointer_table[] =
{   /* pointer id                        active point offset (OS units, not pixels) */
    { { OBJECT_ID_SKEL, "ptr_default" }, {  0,  0 } }, /* not actually used, but here for indexing simplicity */
    { { OBJECT_ID_SKEL, "ptr_write"   }, {  8, 20 } },
    { { OBJECT_ID_SKEL, "ptr_menu"    }, { 12,  4 } },

    { { OBJECT_ID_SS,   "ptr_formula" }, { 26, 26 } },

    { { OBJECT_ID_SKEL, "ptr_hand"    }, { 24, 32 } },

    { { OBJECT_ID_SKEL, "ptr_drag"    }, { 24, 24 } },
    { { OBJECT_ID_SKEL, "ptr_drag_lr" }, { 24, 24 } },
    { { OBJECT_ID_SKEL, "ptr_drag_ud" }, { 24, 24 } },

    { { OBJECT_ID_SKEL, "ptr_column"  }, { 24, 24 } },
    { { OBJECT_ID_SKEL, "ptr_colum_l" }, { 24, 24 } },
    { { OBJECT_ID_SKEL, "ptr_colum_r" }, { 24, 24 } },

    { { OBJECT_ID_SKEL, "ptr_row"     }, { 24, 24 } },
    { { OBJECT_ID_SKEL, "ptr_row_u"   }, { 24, 24 } },
    { { OBJECT_ID_SKEL, "ptr_row_d"   }, { 24, 24 } }

};

static POINTER_SHAPE current_pointer_shape = POINTER_DEFAULT;

extern void
host_set_pointer_shape(
    _In_        POINTER_SHAPE pointer_shape)
{
    assert((U32) pointer_shape < elemof32(pointer_table));

    if(current_pointer_shape == pointer_shape)
        return;

    riscos_hourglass_off();

    current_pointer_shape = pointer_shape;

    if(pointer_shape != POINTER_DEFAULT)
    {
        const PC_POINTER_INFO p_pointer_info = &pointer_table[pointer_shape];
        const PC_RESOURCE_BITMAP_ID p_resource_bitmap_id = &p_pointer_info->id;
        P_SCB p_scb = resource_bitmap_find(p_resource_bitmap_id).p_scb;

        if(NULL == p_scb)
            p_scb = resource_bitmap_find_system(p_resource_bitmap_id).p_scb;

        if(NULL == p_scb)
            pointer_shape = POINTER_DEFAULT;
        else
        {
            static const BYTE pointer__ttab[] = "\0\1\2\3\0\1\2\3\0\1\2\3\0\1\2\3";
            S32 XEigFactor, YEigFactor;
            _kernel_swi_regs rs;

            host_modevar_cache_query_eigs(p_scb->mode, &XEigFactor, &YEigFactor);

            rs.r[0] = 36 + 512;
            rs.r[1] = 0x12345678; /* kill the OS or any twerp who dares to access this! */
            rs.r[2] = (int) p_scb;
            rs.r[3] = 2; /* shape number */
            rs.r[4] = (int) (p_pointer_info->active_point_offset.x >> XEigFactor); /* OS units -> pixels */
            rs.r[5] = (int) (p_pointer_info->active_point_offset.y >> YEigFactor);
            rs.r[6] = 0; /* scale appropriately */
            rs.r[7] = (int) pointer__ttab;

            if(_kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs))
                pointer_shape = POINTER_DEFAULT;
            else
                /* set pointer to shape 2 */
                _kernel_osbyte(106, 2, 0);
        }
    }

    if(pointer_shape == POINTER_DEFAULT)
        /* restore pointer to shape 1  */
        _kernel_osbyte(106, 1, 0);

    riscos_hourglass_on();
}

extern void
host_restore_clip_rectangle(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_restore_clip_rectangle: gw ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT,
            p_redraw_context->riscos.host_machine_clip_box.x0, p_redraw_context->riscos.host_machine_clip_box.y0,
            p_redraw_context->riscos.host_machine_clip_box.x1, p_redraw_context->riscos.host_machine_clip_box.y1);

    profile_ensure_frame();

    void_WrapOsErrorChecking(
        riscos_vdu_define_graphics_window(
            p_redraw_context->riscos.host_machine_clip_box.x0,
            p_redraw_context->riscos.host_machine_clip_box.y0,
            p_redraw_context->riscos.host_machine_clip_box.x1 - 1 /* clip near pixel edge for printer drivers */,
            p_redraw_context->riscos.host_machine_clip_box.y1 - 1));
}

extern BOOL
host_set_clip_rectangle(
    _InoutRef_  P_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InVal_     RECT_FLAGS rect_flags)
{
    PIXIT_RECT pixit_rect = *p_pixit_rect; /* copy rectangle, then apply rect_flags */
    GDI_RECT gdi_rect;
    GDI_BOX gdi_box;

    assert_EQ(sizeof32(rect_flags), sizeof32(U32));

    if(0 != * (P_U32) &rect_flags) /* SKS 18apr95 more optimisation */
    {
      /*pixit_rect.tl.x -= rect_flags.extend_left_currently_unused  * p_redraw_context->border_width.x;*/
        if(rect_flags.reduce_left_by_2)
            pixit_rect.tl.x += p_redraw_context->border_width_2.x;
        if(rect_flags.reduce_left_by_1) /* only used by PMF */
            pixit_rect.tl.x += p_redraw_context->border_width.x;

      /*pixit_rect.tl.y -= rect_flags.extend_up_currently_unused    * p_redraw_context->border_width.y;*/
        if(rect_flags.reduce_up_by_2)
            pixit_rect.tl.y += p_redraw_context->border_width_2.y;
        if(rect_flags.reduce_up_by_1)
            pixit_rect.tl.y += p_redraw_context->border_width.y;

        if(rect_flags.extend_right_by_1)
            pixit_rect.br.x += p_redraw_context->border_width.x;
        if(rect_flags.reduce_right_by_1)
            pixit_rect.br.x -= p_redraw_context->border_width.x;

        if(rect_flags.extend_down_by_1)
            pixit_rect.br.y += p_redraw_context->border_width.y;
        if(rect_flags.reduce_down_by_1)
            pixit_rect.br.y -= p_redraw_context->border_width.y;
    }

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_set_clip_rectangle: rect tl ") S32_TFMT TEXT(",") S32_TFMT TEXT(" br ") S32_TFMT TEXT(",") S32_TFMT,
            pixit_rect.tl.x, pixit_rect.tl.y, pixit_rect.br.x, pixit_rect.br.y);

    /* convert pixit_rect to inc,inc,exc,exc coords */
    status_consume(gdi_rect_from_pixit_rect_and_context(&gdi_rect, &pixit_rect, p_redraw_context));

    gdi_box.x0 = gdi_rect.tl.x;
    gdi_box.y0 = gdi_rect.br.y;
    gdi_box.x1 = gdi_rect.br.x;
    gdi_box.y1 = gdi_rect.tl.y;

#if 0 || CHECKING
    if(0 != * (P_U32) &rect_flags) /* SKS 18apr95 more optimisation */
    {
        assert(0 == rect_flags.extend_left_ppixels);
        assert(0 == rect_flags.extend_down_ppixels);
        assert(0 == rect_flags.extend_right_ppixels);
        assert(0 == rect_flags.extend_up_ppixels);
#if 0
        gdi_box.x0 -= rect_flags.extend_left_ppixels  * RISCOS_PER_PROGRAM_PIXEL_X;
        gdi_box.y0 -= rect_flags.extend_down_ppixels  * RISCOS_PER_PROGRAM_PIXEL_Y;
        gdi_box.x1 += rect_flags.extend_right_ppixels * RISCOS_PER_PROGRAM_PIXEL_X;
        gdi_box.y1 += rect_flags.extend_up_ppixels    * RISCOS_PER_PROGRAM_PIXEL_Y;
#endif
    }
#endif

    /* never set a clip rect outside the parent clip rect (SKS - which we now modify 14jul93) (SKS 18apr95 threw away max/min - duff compiler) */
    if( p_redraw_context->riscos.host_machine_clip_box.x0 < gdi_box.x0)
        p_redraw_context->riscos.host_machine_clip_box.x0 = gdi_box.x0;
    if( p_redraw_context->riscos.host_machine_clip_box.y0 < gdi_box.y0)
        p_redraw_context->riscos.host_machine_clip_box.y0 = gdi_box.y0;
    if( p_redraw_context->riscos.host_machine_clip_box.x1 > gdi_box.x1)
        p_redraw_context->riscos.host_machine_clip_box.x1 = gdi_box.x1;
    if( p_redraw_context->riscos.host_machine_clip_box.y1 > gdi_box.y1)
        p_redraw_context->riscos.host_machine_clip_box.y1 = gdi_box.y1;

    if( (p_redraw_context->riscos.host_machine_clip_box.x0 < p_redraw_context->riscos.host_machine_clip_box.x1)
    &&  (p_redraw_context->riscos.host_machine_clip_box.y0 < p_redraw_context->riscos.host_machine_clip_box.y1) )
    {
        trace_4(TRACE_APP_HOST_PAINT, TEXT("host_set_clip_rectangle: gw ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT TEXT(" OK"),
                p_redraw_context->riscos.host_machine_clip_box.x0, p_redraw_context->riscos.host_machine_clip_box.y0,
                p_redraw_context->riscos.host_machine_clip_box.x1, p_redraw_context->riscos.host_machine_clip_box.y1);

        void_WrapOsErrorChecking(
            riscos_vdu_define_graphics_window(
                p_redraw_context->riscos.host_machine_clip_box.x0,
                p_redraw_context->riscos.host_machine_clip_box.y0,
                p_redraw_context->riscos.host_machine_clip_box.x1 - 1 /* clip near pixel edge for printer drivers */,
                p_redraw_context->riscos.host_machine_clip_box.y1 - 1));

        return(TRUE);
    }

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_set_clip_rectangle: box ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT TEXT(" FAILED"),
            p_redraw_context->riscos.host_machine_clip_box.x0, p_redraw_context->riscos.host_machine_clip_box.y0,
            p_redraw_context->riscos.host_machine_clip_box.x1, p_redraw_context->riscos.host_machine_clip_box.y1);
    return(FALSE);
}

extern BOOL
host_set_clip_rectangle2(
    _InoutRef_  P_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_rect_paper,
    _InRef_     PC_PIXIT_RECT p_rect_object,
    _InVal_     RECT_FLAGS rect_flags)
{
    PIXIT_RECT pixit_rect = *p_rect_object; /* copy rectangle, then apply rect_flags */
    GDI_RECT gdi_rect;
    GDI_BOX gdi_box;

    assert_EQ(sizeof32(rect_flags), sizeof32(U32));

    if(0 != * (P_U32) &rect_flags) /* SKS 18apr95 more optimisation */
    {
      /*pixit_rect.tl.x -= rect_flags.extend_left_currently_unused  * p_redraw_context->border_width.x;*/
        if(rect_flags.reduce_left_by_2)
            pixit_rect.tl.x += p_redraw_context->border_width_2.x;
        if(rect_flags.reduce_left_by_1) /* only used by PMF */
            pixit_rect.tl.x += p_redraw_context->border_width.x;

      /*pixit_rect.tl.y -= rect_flags.extend_up_currently_unused    * p_redraw_context->border_width.y;*/
        if(rect_flags.reduce_up_by_2)
            pixit_rect.tl.y += p_redraw_context->border_width_2.y;
        if(rect_flags.reduce_up_by_1)
            pixit_rect.tl.y += p_redraw_context->border_width.y;

        if(rect_flags.extend_right_by_1)
            pixit_rect.br.x += p_redraw_context->border_width.x;
        if(rect_flags.reduce_right_by_1)
            pixit_rect.br.x -= p_redraw_context->border_width.x;

        if(rect_flags.extend_down_by_1)
            pixit_rect.br.y += p_redraw_context->border_width.y;
        if(rect_flags.reduce_down_by_1)
            pixit_rect.br.y -= p_redraw_context->border_width.y;
    }

    if( pixit_rect.tl.x < p_rect_paper->tl.x)
        pixit_rect.tl.x = p_rect_paper->tl.x;
    if( pixit_rect.tl.y < p_rect_paper->tl.y)
        pixit_rect.tl.y = p_rect_paper->tl.y;
    if( pixit_rect.br.x > p_rect_paper->br.x)
        pixit_rect.br.x = p_rect_paper->br.x;
    if( pixit_rect.br.y > p_rect_paper->br.y)
        pixit_rect.br.y = p_rect_paper->br.y;

    /* SKS 18apr95 copy of the above code to avoid slow multiply by zero cases! */
    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_set_clip_rectangle2: rect tl ") S32_TFMT TEXT(",") S32_TFMT TEXT(" br ") S32_TFMT TEXT(",") S32_TFMT,
            pixit_rect.tl.x, pixit_rect.tl.y, pixit_rect.br.x, pixit_rect.br.y);

    /* convert pixit_rect to inc,inc,exc,exc coords */
    status_consume(gdi_rect_from_pixit_rect_and_context(&gdi_rect, &pixit_rect, p_redraw_context));

    gdi_box.x0 = gdi_rect.tl.x;
    gdi_box.y0 = gdi_rect.br.y;
    gdi_box.x1 = gdi_rect.br.x;
    gdi_box.y1 = gdi_rect.tl.y;

    /* does not take into account the rect_flags.extend_left_pixels etc. - this is correct, see old action */

    /* never set a clip rect outside the parent clip rect (SKS - which we now modify 14jul93) */
    if( p_redraw_context->riscos.host_machine_clip_box.x0 < gdi_box.x0)
        p_redraw_context->riscos.host_machine_clip_box.x0 = gdi_box.x0;
    if( p_redraw_context->riscos.host_machine_clip_box.y0 < gdi_box.y0)
        p_redraw_context->riscos.host_machine_clip_box.y0 = gdi_box.y0;
    if( p_redraw_context->riscos.host_machine_clip_box.x1 > gdi_box.x1)
        p_redraw_context->riscos.host_machine_clip_box.x1 = gdi_box.x1;
    if( p_redraw_context->riscos.host_machine_clip_box.y1 > gdi_box.y1)
        p_redraw_context->riscos.host_machine_clip_box.y1 = gdi_box.y1;

    if( (p_redraw_context->riscos.host_machine_clip_box.x0 < p_redraw_context->riscos.host_machine_clip_box.x1)
    &&  (p_redraw_context->riscos.host_machine_clip_box.y0 < p_redraw_context->riscos.host_machine_clip_box.y1) )
    {
        trace_4(TRACE_APP_HOST_PAINT, TEXT("host_set_clip_rectangle2: gw ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT TEXT(" OK"),
                p_redraw_context->riscos.host_machine_clip_box.x0, p_redraw_context->riscos.host_machine_clip_box.y0,
                p_redraw_context->riscos.host_machine_clip_box.x1, p_redraw_context->riscos.host_machine_clip_box.y1);

        void_WrapOsErrorChecking(
            riscos_vdu_define_graphics_window(
                p_redraw_context->riscos.host_machine_clip_box.x0,
                p_redraw_context->riscos.host_machine_clip_box.y0,
                p_redraw_context->riscos.host_machine_clip_box.x1 - 1,
                p_redraw_context->riscos.host_machine_clip_box.y1 - 1));

        return(TRUE);
    }

    trace_4(TRACE_APP_HOST_PAINT, TEXT("host_set_clip_rectangle2: box ") S32_TFMT TEXT(",") S32_TFMT TEXT(";") S32_TFMT TEXT(",") S32_TFMT TEXT(" FAILED"),
            p_redraw_context->riscos.host_machine_clip_box.x0, p_redraw_context->riscos.host_machine_clip_box.y0,
            p_redraw_context->riscos.host_machine_clip_box.x1, p_redraw_context->riscos.host_machine_clip_box.y1);
    return(FALSE);
}

/******************************************************************************
*
* draw a border round a box
*
******************************************************************************/

/* draw a 2 OS unit band of colour */

static void
band_of_colour_2(
    _InoutRef_  P_GDI_BOX p_box,
    _In_        int tl_plot,
    _In_        int br_plot)
{
    /* bl-tl */
    void_WrapOsErrorChecking(
        bbc_move(                            (p_box->x0),         (p_box->y0)));
    void_WrapOsErrorChecking(
        os_plot(bbc_RectangleFill + tl_plot, (p_box->x0 + 2 - 1), (p_box->y1 - 1)));

    /* tl-tr */
    void_WrapOsErrorChecking(
        bbc_move(                            (p_box->x0 + 2),     (p_box->y1 - 2 + 1)));
    void_WrapOsErrorChecking(
        os_plot(bbc_RectangleFill + tl_plot, (p_box->x1 - 2 - 1), (p_box->y1 - 1)));

    /* tr-br */
    void_WrapOsErrorChecking(
        bbc_move(                            (p_box->x1 - 2),     (p_box->y1 - 1)));
    void_WrapOsErrorChecking(
        os_plot(bbc_RectangleFill + br_plot, (p_box->x1 - 1),     (p_box->y0)));

    /* br-bl */
    void_WrapOsErrorChecking(
        bbc_move(                            (p_box->x1 - 2 - 1), (p_box->y0 + 2 - 1)));
    void_WrapOsErrorChecking(
        os_plot(bbc_RectangleFill + br_plot, (p_box->x0 + 2),     (p_box->y0)));

    p_box->x0 += 2;
    p_box->y0 += 2;
    p_box->x1 -= 2;
    p_box->y1 -= 2;
}

/* draw a 4 OS unit band of colour */

static void
band_of_colour_4(
    _InoutRef_  P_GDI_BOX p_box,
    _In_        int tl_plot,
    _In_        int br_plot)
{
    /* bl-tl */
    void_WrapOsErrorChecking(
        bbc_move(                            (p_box->x0),         (p_box->y0)));
    void_WrapOsErrorChecking(
        os_plot(bbc_RectangleFill + tl_plot, (p_box->x0 + 4 - 1), (p_box->y1 - 1)));

    /* tl-tr */
    void_WrapOsErrorChecking(
        bbc_move(                            (p_box->x0 + 4),     (p_box->y1 - 4 + 1)));
    void_WrapOsErrorChecking(
        os_plot(bbc_RectangleFill + tl_plot, (p_box->x1 - 4 - 1), (p_box->y1 - 1)));

    /* tr-br */
    void_WrapOsErrorChecking(
        bbc_move(                            (p_box->x1 - 4),     (p_box->y1 - 1)));
    void_WrapOsErrorChecking(
        os_plot(bbc_RectangleFill + br_plot, (p_box->x1 - 1),     (p_box->y0)));

    /* br-bl */
    void_WrapOsErrorChecking(
        bbc_move(                            (p_box->x1 - 4 - 1), (p_box->y0 + 4 - 1)));
    void_WrapOsErrorChecking(
        os_plot(bbc_RectangleFill + br_plot, (p_box->x0 + 4),     (p_box->y0)));

    if(br_plot != tl_plot)
        if(host_modevar_cache_current.YEig < 2)
        {
            /* slight tweaks (add more rectangles!) */

            /* bl chamfer */
            void_WrapOsErrorChecking(
                bbc_move(                            (p_box->x0 + 2),     (p_box->y0)));
            void_WrapOsErrorChecking(
                os_plot(bbc_RectangleFill + br_plot, (p_box->x0 + 4 - 1), (p_box->y0 + 2 - 1)));

            /* tr chamfer */
            void_WrapOsErrorChecking(
                bbc_move(                            (p_box->x1 - 4),         (p_box->y1 - 1)));
            void_WrapOsErrorChecking(
                os_plot(bbc_RectangleFill + tl_plot, (p_box->x1 - 4 + 2 - 1), (p_box->y1 - 2 + 1)));
        }

    p_box->x0 += 4;
    p_box->y0 += 4;
    p_box->x1 -= 4;
    p_box->y1 -= 4;
}

extern void
host_framed_box_paint_frame(
    _InRef_     PC_GDI_BOX p_box_abs,
    _In_        FRAMED_BOX_STYLE border_style)
{
    GDI_BOX box = *p_box_abs;
    FRAMED_BOX_STYLE disabled;

    disabled     = border_style &  FRAMED_BOX_DISABLED;
    border_style = border_style & ~disabled;

    switch(border_style)
    {
    default:
#if CHECKING
    case FRAMED_BOX_TROUGH_LBOX:
        myassert1(TEXT("unhandled framed_box_style %d"), border_style);

        /*FALLTHRU*/

    case FRAMED_BOX_NONE:
#endif
        break;

    /* thinnest (but min 2 OS) lines around inside */
    case FRAMED_BOX_PLAIN:
#if !defined(FRAMED_BOX_EDIT_FANCY)
    case FRAMED_BOX_EDIT:
#endif
        {
        int line_colour;

        line_colour = disabled ? 0x02 /*lt grey*/ : 0x06 /*dark grey - was black*/;

        (void) host_setfgcolour(&rgb_stash[line_colour]);

        /* bl */
        void_WrapOsErrorChecking(
            bbc_move(box.x0, box.y0));
        /* bl-tl */
        void_WrapOsErrorChecking(
            os_plot(bbc_RectangleFill + bbc_DrawAbsFore, (box.x0 + 2 - 1), (box.y1 - 1)));
        /* tl-tr */
        void_WrapOsErrorChecking(
            os_plot(bbc_RectangleFill + bbc_DrawAbsFore, (box.x1 - 1),     (box.y1 - 1)));
        /* tr-br */
        void_WrapOsErrorChecking(
            os_plot(bbc_RectangleFill + bbc_DrawAbsFore, (box.x1 - 2),     (box.y0)));
        /* br-bl */
        void_WrapOsErrorChecking(
            os_plot(bbc_RectangleFill + bbc_DrawAbsFore, (box.x0),         (box.y0 + 2 - 1)));

        break;
    }

    case FRAMED_BOX_BUTTON_IN:  /* 3-D sunken recess   */
    case FRAMED_BOX_BUTTON_OUT: /* 3-D raised platform */
    case FRAMED_BOX_PLINTH:
    case FRAMED_BOX_TROUGH:
        {
        int tl_colour, br_colour;

        switch(border_style)
        {
        case FRAMED_BOX_BUTTON_IN:
        case FRAMED_BOX_TROUGH:
            tl_colour = disabled ? 0x02 /*lt grey*/ : 0x04; /*dk grey*/
            br_colour = 0x00; /*white*/
            break;

        default:
        case FRAMED_BOX_BUTTON_OUT:
            tl_colour = 0x00; /*white*/
            br_colour = disabled ? 0x02 /*lt grey*/ : 0x04; /*dk grey*/
            break;
        }

        (void) host_setfgcolour(&rgb_stash[tl_colour]);
        (void) host_setbgcolour(&rgb_stash[br_colour]);

        band_of_colour_4(&box, bbc_DrawAbsFore, bbc_DrawAbsBack);

        break;
        }

    case FRAMED_BOX_DEFBUTTON_IN:  /* 3-D sunken recess within a well   */
    case FRAMED_BOX_DEFBUTTON_OUT: /* 3-D raised platform within a well */
        {
        int band_colour;
        FRAMED_BOX_STYLE box_style;

        /* draw the well */
        box_style = FRAMED_BOX_BUTTON_IN;

        host_framed_box_paint_frame(&box, box_style | disabled);
        host_framed_box_trim_frame( &box, box_style | disabled);

        switch(border_style)
        {
        case FRAMED_BOX_DEFBUTTON_IN:
            band_colour = disabled ? 0x04 /*dk grey*/ : 0x0E; /*orange*/
            break;

        default:
        case FRAMED_BOX_DEFBUTTON_OUT:
            band_colour = disabled ? 0x02 /*lt grey*/ : 0x0C; /*cream*/
            break;
        }

        (void) host_setfgcolour(&rgb_stash[band_colour]);

        band_of_colour_4(&box, bbc_DrawAbsFore, bbc_DrawAbsFore);

        /* finally draw the corresponding button inside this */
        box_style = border_style - (FRAMED_BOX_DEFBUTTON_OUT - FRAMED_BOX_BUTTON_OUT);

        host_framed_box_paint_frame(&box, box_style | disabled);

        break;
        }

    case FRAMED_BOX_CHANNEL_IN:
    case FRAMED_BOX_CHANNEL_OUT:
        {
        int tl_colour, br_colour;

        switch(border_style)
        {
        default:
        case FRAMED_BOX_BUTTON_IN:
            tl_colour = disabled ? 0x02 /*lt grey*/ : 0x02; /*lt grey*/
            br_colour = 0x00; /*white*/
            break;

        case FRAMED_BOX_BUTTON_OUT:
            tl_colour = 0x00; /*white*/
            br_colour = disabled ? 0x02 /*lt grey*/ : 0x02; /*lt grey*/
            break;
        }

        (void) host_setfgcolour(&rgb_stash[tl_colour]);
        (void) host_setbgcolour(&rgb_stash[br_colour]);

        band_of_colour_4(&box, bbc_DrawAbsFore, bbc_DrawAbsBack);

        /* one inside this is simply reversed! */
        band_of_colour_4(&box, bbc_DrawAbsBack, bbc_DrawAbsFore);

        break;
        }

#if defined(FRAMED_BOX_EDIT_FANCY)
    case FRAMED_BOX_EDIT:
        {
        int tl_colour, br_colour;

        if(host_modevar_cache_current.YEig >= 2)
        {
            host_framed_box_paint_frame(&box, FRAMED_BOX_PLAIN | disabled);
            return;
        }

        tl_colour = disabled ? 0x02 : 0x04;
        br_colour = disabled ? 0x00 : 0x00;

        (void) host_setfgcolour(&rgb_stash[tl_colour]);
        (void) host_setbgcolour(&rgb_stash[br_colour]);

        band_of_colour_2(&box, bbc_DrawAbsFore, bbc_DrawAbsBack);

        /*tl_colour = disabled ? 0x02 : 0x04;*/
        br_colour = disabled ? 0x00 : 0x01;

        /*(void) host_setfgcolour(&rgb_stash[tl_colour]);*/
        (void) host_setbgcolour(&rgb_stash[br_colour]);

        band_of_colour_2(&box, bbc_DrawAbsFore, bbc_DrawAbsBack);

        break;
        }
#endif /* FRAMED_BOX_EDIT_FANCY */

    case FRAMED_BOX_W31_BUTTON_IN:  /* filled Windows 3.1 style 3-D sunken recessed button */
    case FRAMED_BOX_W31_BUTTON_OUT: /* filled Windows 3.1 style 3-D raised button */
        {
        int tl_colour, br_colour;

        if(host_modevar_cache_current.YEig >= 2)
        {
            host_framed_box_paint_frame(p_box_abs, ((border_style - FRAMED_BOX_W31_BUTTON_IN) + FRAMED_BOX_BUTTON_IN) | disabled);
            return;
        }

        { /* thin black border round the button omitting the corner pixels */
        int line_colour = 0x07 /*black*/;

        if(host_setfgcolour(&rgb_stash[line_colour]))
        {
            /* bl-br */
            void_WrapOsErrorChecking(
                bbc_move((box.x0 + 2), (box.y0    )));
            void_WrapOsErrorChecking(
                os_plot(bbc_RectangleFill + bbc_DrawAbsFore, (box.x1 - 2 - 1), (box.y0 + 2 - 1)));
            /* bl-tl */
            void_WrapOsErrorChecking(
                bbc_move((box.x0    ), (box.y0 + 2)));
            void_WrapOsErrorChecking(
                os_plot(bbc_RectangleFill + bbc_DrawAbsFore, (box.x0 + 2 - 1), (box.y1 - 2 - 1)));
            /* tl-tr */
            void_WrapOsErrorChecking(
                bbc_move((box.x0 + 2), (box.y1 - 2)));
            void_WrapOsErrorChecking(
                os_plot(bbc_RectangleFill + bbc_DrawAbsFore, (box.x1 - 2 - 1), (box.y1     - 1)));
            /* tr-br */
            void_WrapOsErrorChecking(
                bbc_move((box.x1 - 2), (box.y0 + 2)));
            void_WrapOsErrorChecking(
                os_plot(bbc_RectangleFill + bbc_DrawAbsFore, (box.x1     - 1), (box.y1 - 2 - 1)));

            box.x0 += 2;
            box.y0 += 2;
            box.x1 -= 2;
            box.y1 -= 2;
        }
        } /*block*/

        switch(border_style)
        {
        case FRAMED_BOX_W31_BUTTON_IN:
            tl_colour = disabled ? 0x00 /*white*/ : 0x04; /*dk grey*/
            br_colour = 0x00; /*white*/
            break;

        default:
            tl_colour = 0x00; /*white*/
            br_colour = disabled ? 0x00 /*white*/ : 0x04; /*dk grey*/
            break;
        }

        (void) host_setfgcolour(&rgb_stash[tl_colour]);
        (void) host_setbgcolour(&rgb_stash[br_colour]);

        band_of_colour_2(&box, bbc_DrawAbsFore, bbc_DrawAbsBack);

        break;
        }
    }
}

extern void
host_framed_box_paint_core(
    _InRef_     PC_GDI_BOX p_box_abs,
    _InRef_     PC_RGB p_rgb)
{
    GDI_BOX box = *p_box_abs;

    if(host_setfgcolour(p_rgb))
    {
        /* bl */
        void_WrapOsErrorChecking(
            bbc_move(box.x0, box.y0));

        /* bl-tr */
        void_WrapOsErrorChecking(
            os_plot(bbc_RectangleFill + bbc_DrawAbsFore, (box.x1 - 1), (box.y1 - 1)));
    }
}

/******************************************************************************
*
* trim a border's worth off this box
*
******************************************************************************/

extern void
host_framed_box_trim_frame(
    _InoutRef_  P_GDI_BOX p_box,
    _In_        FRAMED_BOX_STYLE border_style)
{
    switch(border_style & ~FRAMED_BOX_DISABLED)
    {
    default: default_unhandled();
#if CHECKING
    case FRAMED_BOX_NONE:
#endif
        break;

    case FRAMED_BOX_PLAIN:
#if !defined(FRAMED_BOX_EDIT_FANCY)
    case FRAMED_BOX_EDIT:
#endif
        p_box->x0 += MAX(2, host_modevar_cache_current.dx);
        p_box->y0 += MAX(2, host_modevar_cache_current.dy);
        p_box->x1 -= MAX(2, host_modevar_cache_current.dx);
        p_box->y1 -= MAX(2, host_modevar_cache_current.dy);
        break;

    case FRAMED_BOX_BUTTON_IN:
    case FRAMED_BOX_BUTTON_OUT:
    case FRAMED_BOX_W31_BUTTON_IN:
    case FRAMED_BOX_W31_BUTTON_OUT:
    case FRAMED_BOX_PLINTH:
    case FRAMED_BOX_TROUGH:
    case FRAMED_BOX_TROUGH_LBOX: /* for R2 style frame */
        p_box->x0 += 4;
        p_box->y0 += 4;
        p_box->x1 -= 4;
        p_box->y1 -= 4;
        break;

    case FRAMED_BOX_DEFBUTTON_IN:
    case FRAMED_BOX_DEFBUTTON_OUT:
        p_box->x0 += (4 + 4) + 4;
        p_box->y0 += (4 + 4) + 4;
        p_box->x1 -= (4 + 4) + 4;
        p_box->y1 -= (4 + 4) + 4;
        break;

    case FRAMED_BOX_CHANNEL_IN:
    case FRAMED_BOX_CHANNEL_OUT:
        p_box->x0 += 4 + 4;
        p_box->y0 += 4 + 4;
        p_box->x1 -= 4 + 4;
        p_box->y1 -= 4 + 4;
        break;

#if defined(FRAMED_BOX_EDIT_FANCY)
    case FRAMED_BOX_EDIT:
        if(host_modevar_cache_current.XEig >= 2)
        {
            host_framed_box_trim_frame(p_box, FRAMED_BOX_PLAIN);
            return;
        }

        p_box->x0 += 4;
        p_box->y0 += 4;
        p_box->x1 -= 4;
        p_box->y1 -= 4;
        break;
#endif /* FRAMED_BOX_EDIT_FANCY */
    }
}

/* recache local copy after a palette change  */

extern void
host_palette_cache_reset(void)
{
    /* RISC OS 3 thing does nibble conversion itself as needed */
    void_WrapOsErrorReporting(wimp_read_palette(&cache.palette));

#if TRACE_ALLOWED
    {
    U32 i;
    for(i = 0; i < elemof32(cache.palette.colours); ++i)
        trace_2(TRACE_RISCOS_HOST, TEXT("palette entry %2u = ") U32_XTFMT, i, cache.palette.colours[i]);
    } /*block*/
#endif

    host_rgb_stash(); /* always keep stash in sync */
}

static void
host_rgb_stash_colour(
    _In_        UINT index)
{
    RISCOS_PALETTE_U os_rgb;

    os_rgb.word = cache.palette.colours[index] & 0xFFFFFF00;

    rgb_set(&rgb_stash[index], os_rgb.bytes.red, os_rgb.bytes.green, os_rgb.bytes.blue);
}

extern void
host_rgb_stash(void)
{
    UINT index;

    for(index = 0; index < 16; ++index)
        host_rgb_stash_colour(index);
}

/* NB pixels */

extern void
host_work_area_gdi_size_query(
    _OutRef_    P_GDI_SIZE p_gdi_size)
{
    *p_gdi_size = host_modevar_cache_current.gdi_size;
}

/******************************************************************************
*
* read font manager version and set flags for facilities available
*
******************************************************************************/

static const int version_font_m = 307; /* always expect kerning support now */

static int version_font_m_kerning_ok = 307;

_Check_return_
extern U32
host_version_font_m_read(
    _InVal_     U32 flags)
{
    U32 flags_temp = flags;

    if((flags == 0) /*|| (version_font_m == 0)*/)
        host_version_font_m_reset();

    if(version_font_m < version_font_m_kerning_ok)
        flags_temp &= ~HOST_FONT_KERNING;

    return(flags_temp);
}

extern void
host_version_font_m_reset(void)
{
    version_font_m_kerning_ok =
        (global_preferences.disable_kerning)
            ? INT_MAX /* version_font_m is always < INT_MAX */
            : 307;
}

extern void
host_ploticon(
    _In_        WimpIconBlockWithBitset * const p_icon)
{
    _kernel_swi_regs rs;

    host_invalidate_cache(HIC_PLOTICON);

    rs.r[0] = 0;
    rs.r[1] = (int) p_icon;
    void_WrapOsErrorReporting(_kernel_swi(/*Wimp_PlotIcon*/ 0x0400E2, &rs, &rs));
}

/* host_ploticon() plots window relative */

extern void
host_ploticon_setup_bbox(
    _Inout_     WimpIconBlockWithBitset * const p_icon,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT_RECT pixit_rect;
    GDI_RECT gdi_rect;

    pixit_rect.tl.x = p_pixit_rect->tl.x + p_redraw_context->pixit_origin.x;
    pixit_rect.tl.y = p_pixit_rect->tl.y + p_redraw_context->pixit_origin.y;
    pixit_rect.br.x = p_pixit_rect->br.x + p_redraw_context->pixit_origin.x;
    pixit_rect.br.y = p_pixit_rect->br.y + p_redraw_context->pixit_origin.y;

    status_consume(window_rect_from_pixit_rect(&gdi_rect, &pixit_rect, &p_redraw_context->host_xform));

    BBox_from_gdi_rect(p_icon->bbox, gdi_rect);
}

_Check_return_
static STATUS
supersprite(
    _InRef_     PC_SCB p_scb,
    _InVal_     S32 x,
    _InVal_     S32 y,
    _InVal_     S32 w,
    _InVal_     S32 h,
    _InVal_     BOOL stretch);

typedef struct SPRITEOP_SCALING_FACTORS
{
    S32 numerator_x;
    S32 numerator_y;
    S32 divisor_x;
    S32 divisor_y;
}
SPRITEOP_SCALING_FACTORS, * P_SPRITEOP_SCALING_FACTORS;

_Check_return_
static STATUS
plotscaled2(
    _InRef_     PC_SCB p_scb,
    _InVal_     U32 mode,
    _InVal_     S32 x,
    _InVal_     S32 y,
    P_SPRITEOP_SCALING_FACTORS r6);

static P_U8
generate_table(
    _InRef_     PC_SCB p_scb,
    _InVal_     U32 mode,
    P_U8 paltemp,
    P_U8 pixtrans);

_Check_return_
static STATUS
process_sprite_pre_riscos_3_5(
    _InoutRef_  P_SCB p_scb)
{
    const U32 t = ((U32) p_scb->mode & 0xF8000000U) >> 27;

    switch(t)
    {
    case 0:
        break;

    case 1:
        p_scb->offset_to_mask = p_scb->offset_to_data;
        p_scb->mode =  0; /* 1 bpp mode */
        break;

    case 2:
        p_scb->offset_to_mask = p_scb->offset_to_data;
        p_scb->mode =  8; /* 2 bpp mode */
        break;

    case 3:
        p_scb->offset_to_mask = p_scb->offset_to_data;
        p_scb->mode = 27; /* 4 bpp mode */
        break;

    case 4:
        p_scb->offset_to_mask = p_scb->offset_to_data;
        p_scb->mode = 28; /* 8 bpp mode */
        break;

    case 5:
    case 6:
        if((U32) p_scb->mode & 0xF8000000U)
            return(TRUE); /* try plotting with supersprite() */
        break;
    }

    return(FALSE);
}

_Check_return_
static STATUS
plot_sprite(
    _InRef_     P_SCB p_scb /*poked+restored*/,
    _InVal_     S32 x,
    _InVal_     S32 y,
    _InVal_     S32 w,
    _InVal_     S32 h,
    _InVal_     BOOL stretch)
{
    const S32 oldpo = p_scb->offset_to_mask;
    const S32 oldmo = p_scb->mode;
    _kernel_swi_regs rs;
    STATUS status;

    /* Test for OS supersprite compatibility */
    if(host_os_version_query() < RISCOS_3_5)
    {
        if(process_sprite_pre_riscos_3_5(p_scb))
        {
            status_return(status = supersprite(p_scb, x, y, w, h, stretch));
            if(status_done(status)) /* DONE -> plotted */
                return(STATUS_OK);
        }
    }

    rs.r[0] = 0x200 | 0x28;
    rs.r[1] = 0xFF;
    rs.r[2] = (int) p_scb;

    if(NULL != _kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs))
        status = STATUS_FAIL;
    else
    {
        S32 pixW = rs.r[3];
        S32 pixH = rs.r[4];
        U32 mode = rs.r[6];
        S32 XEig, YEig;

        host_modevar_cache_query_eigs(mode, &XEig, &YEig);

        if(stretch)
        {   /* Reputedly... this gives a correct set! */
            BOOL scaling;
            SPRITEOP_SCALING_FACTORS spriteop_scaling_factors;
            spriteop_scaling_factors.numerator_x = w >> XEig;
            spriteop_scaling_factors.numerator_y = h >> YEig;
            spriteop_scaling_factors.divisor_x = pixW;
            spriteop_scaling_factors.divisor_y = pixH;
            scaling =
                (spriteop_scaling_factors.numerator_x != spriteop_scaling_factors.divisor_x) ||
                (spriteop_scaling_factors.numerator_y != spriteop_scaling_factors.divisor_y);
            status = plotscaled2(p_scb, mode, x, y, scaling ? &spriteop_scaling_factors : NULL);
        }
        else
        {   /* just h/v centre */
            status = plotscaled2(p_scb, mode,
                                 x + (w - (pixW << XEig)) / 2,
                                 y + (h - (pixH << YEig)) / 2,
                                 NULL);
        }
    }

    if(p_scb->offset_to_mask != oldpo)
        p_scb->offset_to_mask = oldpo;

    if(p_scb->mode != oldmo)
        p_scb->mode = oldmo;

    return(status);
}

extern void
host_paint_sprite(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InoutRef_  P_SCB p_scb,
    _InVal_     BOOL stretch)
{
    GDI_RECT gdi_rect;

    if(!status_done(gdi_rect_from_pixit_rect_and_context(&gdi_rect, p_pixit_rect, p_redraw_context)))
        return;

    status_consume(plot_sprite(p_scb, gdi_rect.tl.x, gdi_rect.br.y, gdi_rect.br.x - gdi_rect.tl.x, gdi_rect.tl.y - gdi_rect.br.y, stretch));
}

extern void
host_paint_sprite_abs(
    _InRef_     PC_GDI_BOX p_gdi_box,
    _InoutRef_  P_SCB p_scb,
    _InVal_     BOOL stretch)
{
    status_consume(plot_sprite(p_scb, p_gdi_box->x0, p_gdi_box->y0, p_gdi_box->x1 - p_gdi_box->x0, p_gdi_box->y1 - p_gdi_box->y0, stretch));
}

/*
Super sprite conversion down something which RISC OS 3 can hack
*/

/*
\ Convert a 16/24 bit supersprite to an 8-bit sprite
\
\ On Entry
\
\ R0 -> SuperSprite (scbptr) NOT (sahptr)
\ R1 -> Target area or is 0 to enquire as to required size
\ R2 = options
\       0 means ->8bit no palette
\       no other values available
\
\ On Exit
\   If R1 = 0 on entry (enquiry call)
\      R1 is sprite size in bytes or zero if not understood
\      If R1 on exit indicates a valid size then the following are also set up
\         R2 = mode
\         R3 = pixwidth
\         R4 = scanlines
\         R5 = bpp of output
\         R6 = original bpp
\   Otherwise Undefined
*/

_Check_return_
static STATUS
supersprite(
    _InRef_     PC_SCB p_scb,
    _InVal_     S32 x,
    _InVal_     S32 y,
    _InVal_     S32 w,
    _InVal_     S32 h,
    _InVal_     BOOL stretch)
{
    _kernel_swi_regs rs;
    S32 len;
    ARRAY_HANDLE handle;
    P_SAH p_sah;
    STATUS status;

    rs.r[0] = (int) p_scb; /* This is really NOT a P_SAH */
    rs.r[1] = 0;
    rs.r[2] = 0;
    rs.r[3] = 0;

    status_return(image_convert_ensure_PicConvert());

    if(NULL != _kernel_swi(/*PicConvert_SStoFF9*/ 0x48E01, &rs, &rs))
        return(STATUS_FAIL);

    len = rs.r[1];
    /*S32 mode = rs.r[2];*/
    /*S32 pw   = rs.r[3];*/
    /*S32 ph   = rs.r[4];*/
    /*S32 bpp  = rs.r[5];*/

    if(NULL == (p_sah = (P_SAH) al_array_alloc_BYTE(&handle, len + 4, &array_init_block_u8, &status)))
        return(status);

    p_sah->area_size = len;
    p_sah->number_of_sprites = 0;
    p_sah->offset_to_first =
    p_sah->offset_to_free = sizeof32(*p_sah);

#if 0 /* we've correctly initialised it ^^^ */
    rs.r[0] = 0x109;
    rs.r[1] = (int) p_sah;

    if(NULL != _kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs))
        status = STATUS_FAIL;
    else
#endif
    {
        rs.r[0] = (int) p_scb; /* P_SCB really NOT a P_SAH */
        rs.r[1] = (int) p_sah;
        rs.r[2] = 0;
        rs.r[3] = 0;

        if(NULL != _kernel_swi(/*PicConvert_SStoFF9*/ 0x48E01, &rs, &rs))
            status = STATUS_FAIL;
        else
            status = plot_sprite((P_SCB) p_sah, x, y, w, h, stretch);

        if(status_ok(status))
            status = STATUS_DONE; /* plotted */
    }

    al_array_dispose(&handle);

    return(status);
}

_Check_return_
static STATUS
plotscaled2(
    _InRef_     PC_SCB p_scb,
    _InVal_     U32 mode /*that the mode the sprite was defined in */,
    _InVal_     S32 x,
    _InVal_     S32 y,
    P_SPRITEOP_SCALING_FACTORS r6)
{
    U8 temp_pixtrans[256];
    P_U8 r7;
    U8 temppal[1024]; /* SKS 04oct95 this used to be static */
    _kernel_swi_regs rs;

    r7 = generate_table(p_scb, mode, temppal, temp_pixtrans);

    rs.r[0] = 0x200 | 0x34;
    rs.r[1] = 0xFF;
    rs.r[2] = (int) p_scb;
    rs.r[3] = x;
    rs.r[4] = y;
    rs.r[5] = 8;
    rs.r[6] = (int) r6;
    rs.r[7] = (int) r7;

    if(NULL != _kernel_swi(/*OS_SpriteOp*/ 0x0000002E, &rs, &rs))
        return(STATUS_FAIL);

    return(STATUS_OK);
}

/* Build a table
 return -1 or a pixtrans table for use in r7 of the SpriteOp(52)
 sptr%     -> sprite control block
 paltemp%  -> 1K buffer
 pixtrans% -> 256 byte buffer
 mode is the mode the sprite was defined in
*/

static P_U8
generate_table(
    _InRef_     PC_SCB p_scb,
    _InVal_     U32 mode,
    P_U8 paltemp,
    P_U8 pixtrans)
{
    STATUS status = STATUS_OK;
    int Q;
    S32 grab;
    P_U8 spx;
    _kernel_swi_regs rs;
    P_U8 palptr;

    if( p_scb->offset_to_data == 44 )
    {
        /* Offset to sprite image implies no palette */
        S32 bpp;

        /* palptr%=-1  :REM Current palette          */
        /* palptr%=0   :REM Default for mode         */

        host_modevar_cache_query_bpp(mode, &bpp);

        if( bpp == 8 )
            /* Leave as 0 */
            palptr = 0;
        else
        {
#if 1
            memcpy32(paltemp, &cache.palette, 16 * sizeof32(U32));
#else
            /* Use Wimp_ReadPalette */
            rs.r[0] = 0;
            rs.r[1] = (int)paltemp;

            if( _kernel_swi(Wimp_ReadPalette, &rs, &rs) )
                status = STATUS_FAIL;
#endif
            palptr = paltemp;
       }
    }
    else
    {
        P_S32 p_s32_paltemp = (P_S32) paltemp;
        P_S32 p_s32_sptr = (P_S32) p_scb;

        /* Read a 2K palette hunk */
        for(grab = 0; grab <= 2048-8; grab += 8)
            p_s32_paltemp[ ((grab >> 1) >> 2) ] = p_s32_sptr[ ((grab + 44) >> 2) ];

        palptr = paltemp;
    }

    for(Q = 0; Q <= 255; Q++)
        pixtrans[Q] = u8_from_int(Q);

    if(p_scb->offset_to_data == 44+2048)
    {
        /* Implication is that there is a 256 entry palette */
        for(Q = 0; Q <= 255; Q++)
        {
            P_S32 p_s32 = (P_S32) palptr;
            rs.r[0] = (int) p_s32[Q];
            if( _kernel_swi(ColourTrans_ReturnColourNumber, &rs, &rs) )
                status = STATUS_FAIL;
            else
                pixtrans[Q] = u8_from_int(rs.r[0]);
        }
    }
    else
    {
        rs.r[0] = (int) mode;
        rs.r[1] = (int) palptr;
        rs.r[2] = -1;
        rs.r[3] = -1;
        rs.r[4] = (int) pixtrans;

        if( _kernel_swi(ColourTrans_SelectTable, &rs, &rs))
            status = STATUS_FAIL;
    }

    UNREFERENCED_PARAMETER(status);

    spx = (P_U8) NULL;/*-1;*/

    for(Q = 0; Q <= 255; Q++ )
        if( pixtrans[Q] != Q )
        {
            spx = pixtrans;
            break;
        }

    return(spx);
}

/* Use the PicConvert module to convert it to a sprite
    R0 = address of BMPFILEHEADER
    R1 = 0 for enquiry call, otherwise address for output sprite
    R2 = options for use with 24bit BMPs
        use 0 for lowest o/p
*/

extern void
host_paint_bitmap(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    /*_In_*/    PC_ANY p_bmp,
    _InVal_     BOOL stretch)
{
    GDI_RECT gdi_rect;
    S32 w, h;

    if(!status_done(gdi_rect_from_pixit_rect_and_context(&gdi_rect, p_pixit_rect, p_redraw_context)))
        return;

    w = gdi_rect.br.x - gdi_rect.tl.x;
    h = gdi_rect.tl.y - gdi_rect.br.y;

    {
    _kernel_swi_regs rs;
    int options = 3; /* 24 bpp BMP -> 24 bpp sprite */
    S32 newlength;

    if(host_os_version_query() < RISCOS_3_5)
        options = 0; /* 24 bpp BMP -> 8 bpp,default palette sprite */

    /* Do Enq call */
    rs.r[0] = (int) p_bmp;
    rs.r[1] = 0;
    rs.r[2] = options;

    if(status_fail(image_convert_ensure_PicConvert()))
        return;

    if(NULL != _kernel_swi(/*PicConvert_BMPtoFF9*/ 0x48E00, &rs, &rs))
        return;

    newlength = rs.r[1];

    if(newlength)
    {
        STATUS status;
        ARRAY_HANDLE newhandle;
        P_SAH p_sah = (P_SAH) al_array_alloc_BYTE(&newhandle, newlength, &array_init_block_u8, &status);
        P_SCB p_scb;

        /* Do Conversion call */
        rs.r[0] = (int) p_bmp;
        rs.r[1] = (int) p_sah;
        rs.r[2] = options;

        if(NULL != _kernel_swi(/*PicConvert_BMPtoFF9*/ 0x48E00, &rs, &rs))
        {
            al_array_dispose(&newhandle);
            return;
        }

        p_scb = PtrAddBytes(P_SCB, p_sah, sizeof_SPRITE_FILE_HEADER); /* Pop a Doodle Do */

        status = plot_sprite(p_scb, gdi_rect.tl.x, gdi_rect.br.y, w, h, stretch);

        al_array_dispose(&newhandle);
    }
    } /*block*/
}

typedef struct RISCOS_MODE_SELECTOR
{
    U32 flags;
    U32 x_res_pixels;
    U32 y_res_pixels;
    U32 log2bpp;
    S32 frame_rate;

    /* followed by pairs of var,val */

    /* always terminated by -1 (included in structure size for convenience) */
    S32 terminator;
}
RISCOS_MODE_SELECTOR, * P_RISCOS_MODE_SELECTOR;

HOST_MODEVAR_CACHE_ENTRY host_modevar_cache_current;

static ARRAY_HANDLE host_modevar_cache_handle;

_Check_return_
static BOOL
host_modevar_cache_entry_compare_equals(
    _InRef_     PC_HOST_MODEVAR_CACHE_ENTRY p_host_modevar_cache_entry,
    _InVal_     U32 mode_specifier)
{
    const U32 cache_mode_specifier = p_host_modevar_cache_entry->mode_specifier;
    const BOOL cms_is_mode_number = (cache_mode_specifier < 256U);
    const BOOL ms_is_mode_number = (mode_specifier < 256U);
    U32 cms_bit_zero;
    U32 ms_bit_zero;

    if(ms_is_mode_number || cms_is_mode_number)
    {   /* at least one is a Mode Number */
        if(ms_is_mode_number != cms_is_mode_number)
        {   /* can't compare - they are different types */
            return(FALSE);
        }

        /* both are Mode Numbers - can simply compare value */
        return(mode_specifier == cache_mode_specifier);
    }

    cms_bit_zero = (cache_mode_specifier & 1U);
    ms_bit_zero = (mode_specifier & 1U);

    if(ms_bit_zero != cms_bit_zero)
    {   /* can't compare - they are different types */
        return(FALSE);
    }

    if(0 == ms_bit_zero)
    {   /* both are Mode Selectors - need to compare contents */
        P_RISCOS_MODE_SELECTOR p_mode_selector = (P_RISCOS_MODE_SELECTOR) mode_specifier;
        U32 sizeof_mode_selector = sizeof32(*p_mode_selector);
        PC_S32 p_s32;

        for(p_s32 = &p_mode_selector->terminator; *p_s32 != -1; p_s32 += 2)
            sizeof_mode_selector += 2 * sizeof32(S32);

        if(sizeof_mode_selector != array_elements32(&p_host_modevar_cache_entry->h_mode_selector))
            return(FALSE);

        return(0 == short_memcmp32(p_mode_selector, array_base(&p_host_modevar_cache_entry->h_mode_selector, U8), sizeof_mode_selector));
    }

    /* if(1 == ms_bit_zero) */
    /* both are Sprite Mode Words - can simply compare value */
    return(mode_specifier == cache_mode_specifier);
}

static void
host_modevar_cache_entry_dispose(
    _InoutRef_  P_HOST_MODEVAR_CACHE_ENTRY p_host_modevar_cache_entry)
{
    al_array_dispose(&p_host_modevar_cache_entry->h_mode_selector);
}

static void
host_modevar_cache_dispose(void)
{
    ARRAY_INDEX array_index;

    for(array_index = 0; array_index < array_elements(&host_modevar_cache_handle); ++array_index)
        host_modevar_cache_entry_dispose(array_ptr(&host_modevar_cache_handle, HOST_MODEVAR_CACHE_ENTRY, array_index));

    al_array_dispose(&host_modevar_cache_handle);

    host_modevar_cache_entry_dispose(&host_modevar_cache_current);

    zero_struct(host_modevar_cache_current);
}

#if TRACE_ALLOWED

static void
report_mode_specifier(
    _InVal_     U32 mode_specifier)
{
    if(mode_specifier < 256U)
    {   /* Mode Number */
    }
    else if(0 == (mode_specifier & 1U))
    {   /* Mode Selector */
        P_RISCOS_MODE_SELECTOR p_mode_selector = (P_RISCOS_MODE_SELECTOR) mode_specifier;
        PC_S32 p_s32;

        trace_3(0, " Selector f:%d x:%d y:%d", p_mode_selector->flags, p_mode_selector->x_res_pixels, p_mode_selector->y_res_pixels);

        for(p_s32 = &p_mode_selector->terminator; *p_s32 != -1; p_s32 += 2)
        {
            trace_2(0, "- %d:%d", p_s32[0], p_s32[1]);
        }
    }
    else
    {   /* Sprite Mode Word */
    }
}

#endif /* TRACE_ALLOWED */

_Check_return_
static int
os_read_mode_variable(
    _InVal_     U32 mode_specifier,
    _InVal_     int variable)
{
    _kernel_swi_regs rs;
    rs.r[0] = mode_specifier;
    rs.r[1] = variable;
    void_WrapOsErrorChecking(_kernel_swi(OS_ReadModeVariable, &rs, &rs));
    return(rs.r[2]);
}

static void
host_modevar_cache_obtain_data(
    _InVal_     U32 mode_specifier,
    _OutRef_    P_HOST_MODEVAR_CACHE_ENTRY p_host_modevar_cache_entry)
{
    BOOL fScreenMode = TRUE;

#if TRACE_ALLOWED
    trace_2(0, "obtain_mode %d (%x)", mode_specifier, mode_specifier); report_mode_specifier(mode_specifier);
#endif

    zero_struct_ptr(p_host_modevar_cache_entry);

    p_host_modevar_cache_entry->mode_specifier = mode_specifier;

    if(mode_specifier < 256U)
    {   /* Mode Number */
    }
    else if(0 == (mode_specifier & 1U))
    {   /* Mode Specifier */
        P_RISCOS_MODE_SELECTOR p_mode_selector = (P_RISCOS_MODE_SELECTOR) mode_specifier;
        U32 sizeof_mode_selector = sizeof32(*p_mode_selector);
        PC_S32 p_s32;

        for(p_s32 = &p_mode_selector->terminator; *p_s32 != -1; p_s32 += 2)
            sizeof_mode_selector += 2 * sizeof32(S32);

        status_assert(al_array_add(&p_host_modevar_cache_entry->h_mode_selector, BYTE, sizeof_mode_selector, &array_init_block_u8, p_mode_selector));
    }
    else
    {   /* Sprite Mode Word */
        fScreenMode = FALSE; /* e.g. no sensible screen size */
    }

    /* obtain raw values */
    p_host_modevar_cache_entry->XEig = (U8) os_read_mode_variable(mode_specifier, 4/*XEigFactor*/);
    p_host_modevar_cache_entry->YEig = (U8) os_read_mode_variable(mode_specifier, 5/*YEigFactor*/);

    p_host_modevar_cache_entry->Log2BPP = (U8) os_read_mode_variable(mode_specifier, 9/*Log2BPP*/);

    if(fScreenMode)
    {
        p_host_modevar_cache_entry->XWindLimit = os_read_mode_variable(mode_specifier, 11/*XWindLimit*/);
        p_host_modevar_cache_entry->YWindLimit = os_read_mode_variable(mode_specifier, 12/*YWindLimit*/);
    }

    /* compute derived values */
    p_host_modevar_cache_entry->dx = 1 << p_host_modevar_cache_entry->XEig;
    p_host_modevar_cache_entry->dy = 1 << p_host_modevar_cache_entry->YEig;

    p_host_modevar_cache_entry->bpp = 1U << p_host_modevar_cache_entry->Log2BPP;

    if(fScreenMode)
    {
        p_host_modevar_cache_entry->gdi_size.cx = (p_host_modevar_cache_entry->XWindLimit + 1) << p_host_modevar_cache_entry->XEig;
        p_host_modevar_cache_entry->gdi_size.cy = (p_host_modevar_cache_entry->YWindLimit + 1) << p_host_modevar_cache_entry->YEig;
    }

    trace_2(0, "- dx,dy = %d,%d", p_host_modevar_cache_entry->dx, p_host_modevar_cache_entry->dy);

#if TRACE_ALLOWED
    if(fScreenMode)
        trace_2(0, "- gdi_size x,y = %d,%d", p_host_modevar_cache_entry->gdi_size.cx, p_host_modevar_cache_entry->gdi_size.cy);
#endif
}

_Check_return_
_Ret_maybenull_
static PC_HOST_MODEVAR_CACHE_ENTRY
host_modevar_cache_ensure_mode(
    _InVal_     U32 mode_specifier)
{
    ARRAY_INDEX array_index;

    {
    PC_HOST_MODEVAR_CACHE_ENTRY p_host_modevar_cache_entry = &host_modevar_cache_current;

    if(host_modevar_cache_entry_compare_equals(p_host_modevar_cache_entry, mode_specifier))
    {
        /*trace_2(0, "HIT %d (%x) (CURRENT)", mode_specifier, mode_specifier); report_mode_specifier(mode_specifier);*/
        return(p_host_modevar_cache_entry);
    }
    } /*block*/

    for(array_index = 0; array_index < array_elements(&host_modevar_cache_handle); ++array_index)
    {
        PC_HOST_MODEVAR_CACHE_ENTRY p_host_modevar_cache_entry = array_ptr(&host_modevar_cache_handle, HOST_MODEVAR_CACHE_ENTRY, array_index);

        if(host_modevar_cache_entry_compare_equals(p_host_modevar_cache_entry, mode_specifier))
        {
            /*trace_2(0, "HIT %d (%x)", mode_specifier, mode_specifier); report_mode_specifier(mode_specifier);*/
            return(p_host_modevar_cache_entry);
        }
    }

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(HOST_MODEVAR_CACHE_ENTRY), 0);
    HOST_MODEVAR_CACHE_ENTRY host_modevar_cache_entry;

#if TRACE_ALLOWED
    trace_2(0, "MISS %d (%x)", mode_specifier, mode_specifier); report_mode_specifier(mode_specifier);
#endif

    host_modevar_cache_obtain_data(mode_specifier, &host_modevar_cache_entry);

    if(status_fail(al_array_add(&host_modevar_cache_handle, HOST_MODEVAR_CACHE_ENTRY, 1, &array_init_block, &host_modevar_cache_entry)))
        return(NULL);
    } /*block*/

    return(array_ptrc(&host_modevar_cache_handle, HOST_MODEVAR_CACHE_ENTRY, array_elements32(&host_modevar_cache_handle)-1));
}

extern void
host_modevar_cache_reset(void)
{
    _kernel_swi_regs rs;
    U32 current_mode_specifier;

    rs.r[0] = 135;
    void_WrapOsErrorChecking(_kernel_swi(OS_Byte, &rs, &rs));

    current_mode_specifier = rs.r[2]; /* a mode specifier (number < 256 | selector >= 256) may be returned on RISC OS 3.5 and on */

    host_modevar_cache_dispose();

    host_modevar_cache_obtain_data(current_mode_specifier, &host_modevar_cache_current);
}

extern void
host_modevar_cache_query_bpp(
    _InVal_     U32 mode_specifier,
    _OutRef_    P_S32 p_bpp)
{
    PC_HOST_MODEVAR_CACHE_ENTRY p_host_modevar_cache_entry = host_modevar_cache_ensure_mode(mode_specifier);

    if(NULL != p_host_modevar_cache_entry)
    {
        *p_bpp = p_host_modevar_cache_entry->bpp;
        return;
    }

    *p_bpp = 1 << os_read_mode_variable(mode_specifier, 9);
}

extern void
host_modevar_cache_query_eigs(
    _InVal_     U32 mode_specifier,
    _OutRef_    P_S32 p_XEig,
    _OutRef_    P_S32 p_YEig)
{
    PC_HOST_MODEVAR_CACHE_ENTRY p_host_modevar_cache_entry = host_modevar_cache_ensure_mode(mode_specifier);

    if(NULL != p_host_modevar_cache_entry)
    {
        *p_XEig = p_host_modevar_cache_entry->XEig;
        *p_YEig = p_host_modevar_cache_entry->YEig;
        return;
    }

    *p_XEig = os_read_mode_variable(mode_specifier, 4);
    *p_YEig = os_read_mode_variable(mode_specifier, 5);
}

extern void
host_shutdown(void)
{
    host_modevar_cache_dispose();
}

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
riscos_vdu_define_graphics_window(
    _In_        int x1,
    _In_        int y1,
    _In_        int x2,
    _In_        int y2)
{
    /* SKS 19apr95 get round VDU funnel/multiple SWI overhead */
    U8 buffer[9 /*length of VDU 24 sequence*/];
    P_U8 p_u8 = buffer;
    *p_u8++ = 24;
    *p_u8++ = u8_from_int(x1);
    *p_u8++ = u8_from_int(x1 >> 8);
    *p_u8++ = u8_from_int(y1);
    *p_u8++ = u8_from_int(y1 >> 8);
    *p_u8++ = u8_from_int(x2);
    *p_u8++ = u8_from_int(x2 >> 8);
    *p_u8++ = u8_from_int(y2);
    *p_u8++ = u8_from_int(y2 >> 8);
    return(os_writeN(buffer, sizeof32(buffer)));
}

#endif /* RISCOS */

/* end of riscos/ho_paint.c */
