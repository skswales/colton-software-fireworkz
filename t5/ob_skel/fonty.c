/* fonty.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Font management for Fireworkz */

/* MRJC December 1991 / December 1993 (Windows) */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#define EXPOSE_RISCOS_FONT 1

#include "ob_skel/xp_skelr.h"
#endif

#ifndef          __gr_diag_h
#include "cmodules/gr_diag.h"
#endif

#ifndef          __gr_rdia3_h
#include "cmodules/gr_rdia3.h"
#endif

#if RISCOS
#define LF_FULLFACESIZE 64
#endif

#define BUF_FACENAME_LEN 64 /* SKS after 1.08 31aug94 has to be as big as possible for intermediate mapping steps */

#define FONTMAP_BIT_B  1 /* 1<<0 == FONTMAP_BOLD */
#define FONTMAP_BIT_I  2 /* 1<<1 == FONTMAP_ITALIC */

/*
internal structure
*/

typedef struct HOST_FONT_TAB
{
#if RISCOS
    PTSTR fullname;     /* eg Trinity.Bold.Italic */
#elif WINDOWS
    LOGFONT logfont;    /* copy of the LOGFONT passed during enumeration */
#endif

    U32 uses;           /* by APP_FONT_TAB mapping */
}
HOST_FONT_TAB, * P_HOST_FONT_TAB; typedef const HOST_FONT_TAB * PC_HOST_FONT_TAB;

#define p_host_entry(i) \
    array_ptr(&fontmap_table_host, HOST_FONT_TAB, i)

typedef struct APP_FONT_TAB
{
    PTSTR tstr_app_font_name;

    PTSTR tstr_host_base_name;

    U8  rtf_class;
    U8  rtf_master;

    U8  defined_by_config;

    U8  _spare;

    struct APP_FONT_TAB_HOST_FONT
    {
        ARRAY_INDEX host_array_index; /* index to corresponding font in fontmap_table_host */
        PTSTR tstr_config; /* usually always NULL on Windows, but usually always valid on RISC OS apart for single-font variants like Selwyn */
    }
    host_font[FONTMAP_N_BITS];

#if !RISCOS /* need to know what RISC OS would use for font names in Draw files */
    struct APP_FONT_TAB_RISCOS_FONT
    {
        PTSTR tstr_config;
    }
    riscos_font[FONTMAP_N_BITS];
#endif
}
APP_FONT_TAB, * P_APP_FONT_TAB, ** P_P_APP_FONT_TAB; typedef const APP_FONT_TAB * PC_APP_FONT_TAB;

#define P_APP_FONT_TAB_NONE _P_DATA_NONE(P_APP_FONT_TAB)

#define p_app_font_entry(i) \
    array_ptr(&fontmap_table_app, APP_FONT_TAB, i)

typedef struct FONTMAP_REMAP_ENTRY
{
    PTSTR tstr_app_font_name;

    PTSTR tstr_app_font_name_old;
}
FONTMAP_REMAP_ENTRY, * P_FONTMAP_REMAP_ENTRY; typedef const FONTMAP_REMAP_ENTRY * PC_FONTMAP_REMAP_ENTRY;

#if WINDOWS && defined(USE_CACHED_ABC_WIDTHS)
typedef       ABC *  P_ABC;
typedef const ABC * PC_ABC;
#endif

/*
internal routines
*/

#if RISCOS

static _kernel_oserror *
fonty_complain(
    _kernel_oserror * err);

#endif /* OS */

_Check_return_
static BOOL
fonty_spec_compare_not_equals(
    _InRef_     PC_FONT_SPEC p_font_spec_1,
    _InRef_     PC_FONT_SPEC p_font_spec_2);

_Check_return_
static STATUS
fontmap_invent_app_font_entries_for_host(void);

_Check_return_
static FONTMAP_BITS
fontmap_invent_app_font_name(
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf_app_font_name,
    _InVal_     U32 elemof_buffer,
    _In_        PC_HOST_FONT_TAB p_host_font_tab);

_Check_return_
static STATUS
fontmap_lookup_app_font_entries(void);

_Check_return_
static FONTMAP_BITS
fontmap_remap_using_rtf_class(
    _InoutRef_  P_P_APP_FONT_TAB p_p_font_tab,
    _InVal_     FONTMAP_BITS fontmap_bits,
    _InVal_     BOOL use_host_riscos,
    _InVal_     BOOL find_master);

_Check_return_
static FONTMAP_BITS
fontmap_remap_within_font(
    _In_        PC_APP_FONT_TAB p_app_font_tab,
    _InVal_     FONTMAP_BITS fontmap_bits);

#if !RISCOS

_Check_return_
static FONTMAP_BITS
fontmap_remap_within_font_riscos(
    _In_        PC_APP_FONT_TAB p_app_font_tab,
    _InVal_     FONTMAP_BITS fontmap_bits);

#endif

static ARRAY_HANDLE fontmap_table_host;

static ARRAY_HANDLE fontmap_table_app;

static ARRAY_HANDLE fontmap_table_remap;

/******************************************************************************
*
* Fireworkz font layer deals with Fireworkz font handles and also host font handles;
* fonty should be used in 'sessions' closed by a fonty_cache_trash; this frees all
* host font handles owned by fonty from the system; a session is such a time frame
* as a redraw request; object reformat; page print etc
*
******************************************************************************/

ARRAY_HANDLE h_font_cache = 0; /* holds 'session' cache of host font handles - don't GC as handles persist in text cache */

#define p_font_context_from_fonty_handle_wr(fonty_handle) \
    array_ptr(&h_font_cache, FONT_CONTEXT, (fonty_handle))

#if RISCOS

_Check_return_
_Ret_maybenull_
static _kernel_oserror *
fonty_scanstring(
    _In_        HOST_FONT host_font,
    _InoutRef_  P_S32 x_stop,
    _In_reads_(*p_len) PC_U8 sbstr, /* Font Manager may read sbstr[*p_len] */
    _InoutRef_  P_U32 p_len)
{
    _kernel_swi_regs rs;
    _kernel_oserror * p_kernel_oserror;

    rs.r[0] = host_font;
    rs.r[1] = (int) sbstr;
    rs.r[2] = FONT_SCANSTRING_USE_HANDLE /*r0*/ | FONT_SCANSTRING_USE_LENGTH /*r7*/;
    rs.r[3] = INT_MAX;
    rs.r[4] = INT_MAX;
    rs.r[7] = (int) *p_len;

    if(host_version_font_m_read(HOST_FONT_KERNING))
        rs.r[2] |= FONT_SCANSTRING_KERNING;

    if(0 != *x_stop)
    {
        rs.r[3] = *x_stop;
        rs.r[2] |= FONT_SCANSTRING_FIND;
    }

    if(NULL == (p_kernel_oserror = _kernel_swi(/*Font_ScanString*/ 0x400A1, &rs, &rs)))
    {
//reportf("fonty_scanstring(x:%u,len:%u,%.*s) returns x:%u,len:%u",
//        *x_stop, *p_len, (int) *p_len, sbstr, rs.r[3], PtrDiffBytesU32(rs.r[1], p_buffer));
        *x_stop = rs.r[3];
        *p_len = PtrDiffBytesU32(rs.r[1], sbstr);
    }

    return(p_kernel_oserror);
}

#if defined(UNUSED)

_Check_return_
_Ret_maybenull_
static _kernel_oserror *
fonty_scanstring_old(
    _In_        HOST_FONT host_font,
    _InoutRef_  P_S32 x_stop,
    _In_reads_(*p_len) PC_U8 p_str,
    _InoutRef_  P_U32 p_len)
{
    _kernel_swi_regs rs;
    _kernel_oserror * p_kernel_oserror;
    P_U8 p_buffer;
    STATUS status;
    QUICK_BLOCK_WITH_BUFFER(quick_block, 256);
    quick_block_with_buffer_setup(quick_block);

    if(NULL == (p_buffer = quick_block_extend_by(&quick_block, *p_len + 1, &status)))
    {
        status_assert(status);
        return(NULL);
    }

    memcpy32(p_buffer, p_str, *p_len);
    p_buffer[*p_len] = CH_NULL;

    rs.r[0] = host_font;
    rs.r[1] = (int) p_buffer;
    rs.r[2] = FONT_SCANSTRING_USE_HANDLE /*r0*/ | FONT_SCANSTRING_USE_LENGTH /*r7*/;
    rs.r[3] = INT_MAX;
    rs.r[4] = INT_MAX;
    rs.r[7] = (int) *p_len;

    if(host_version_font_m_read(HOST_FONT_KERNING))
        rs.r[2] |= FONT_SCANSTRING_KERNING;

    if(*x_stop)
    {
        rs.r[3] = *x_stop;
        rs.r[2] |= FONT_SCANSTRING_FIND;
    }

    if(NULL == (p_kernel_oserror = _kernel_swi(/*Font_ScanString*/ 0x400A1, &rs, &rs)))
    {
        *x_stop = rs.r[3];
        *p_len = (U32) ((P_U8) rs.r[1] - p_buffer);
    }

    quick_block_dispose(&quick_block);

    return(p_kernel_oserror);
}

#endif

#endif /* RISCOS */

/******************************************************************************
*
* apply font_spec attributes which alter the font specification required from system
*
******************************************************************************/

static void
fonty_size_adjust_attributes(
    _InoutRef_  P_HOST_FONT_SPEC p_font_spec,
    _InVal_     BOOL draft_mode)
{
    if(p_font_spec->superscript || p_font_spec->subscript)
    {
        if((0 != p_font_spec->size_x) && !draft_mode)
        {
            p_font_spec->size_x *= 7;
            p_font_spec->size_x /= 12;
        }
        p_font_spec->size_y *= 7;
        p_font_spec->size_y /= 12;
    }
}

#if RISCOS

static void
host_font_context_fill_info_riscos(
    _InoutRef_  P_FONT_CONTEXT p_font_context,
    _InRef_     P_HOST_FONT_SPEC p_host_font_spec,
    _InVal_     BOOL draft_mode)
{
    { /* eob in next conditional bit! */
    S32 limit_min = (p_host_font_spec->size_y * 15) / 16; /* ratio of 'Â' ascent to point size in worst font so far (Homerton) */

    { /* read width of a space */
    static /*non-const*/ U8Z space_string[] = " ";
    U32 term = 1;
    S32 width_space_mp = 0;

    fonty_complain(fonty_scanstring(p_font_context->host_font_formatting, &width_space_mp, &space_string[0], &term));

    p_font_context->space_width = pixits_from_millipoints_ceil(width_space_mp);
    } /*block*/

    { /* since the standard font manager doesn't tell us, we work out a 'reasonable' ascender height from a large ascender */
    HOST_FONT host_font_formatting = p_font_context->host_font_formatting;
    _kernel_swi_regs rs;

    /* super- and sub- scripts must use height/ascent of 'source' font */
    if(p_host_font_spec->superscript || p_host_font_spec->subscript)
        host_font_formatting = host_font_find(p_host_font_spec, P_REDRAW_CONTEXT_NONE);

    rs.r[0] = host_font_formatting;
    rs.r[1] = 'Â'; /* Generally the tallest Latin-1 character */
    rs.r[2] = 0;
    if(NULL == _kernel_swi(/*Font_CharBBox*/ 0x04008E, &rs, &rs))
        p_font_context->ascent = pixits_from_millipoints_ceil(abs(rs.r[4]));

    if(0 == p_font_context->ascent)
    {
        /* SKS 21sep94 stop Test Automation wingeing about silly techie fonts with only a couple of symbols defined */
        rs.r[0] = host_font_formatting;
        if(NULL == _kernel_swi(/*Font_ReadInfo*/ 0x040084, &rs, &rs))
            p_font_context->ascent = (S32) rs.r[4] * PIXITS_PER_RISCOS;
    }

    if(p_host_font_spec->superscript || p_host_font_spec->subscript)
        host_font_dispose(&host_font_formatting, P_REDRAW_CONTEXT_NONE);
    } /*block*/

    /* SKS 01mar95 stop Calibration Systems wingeing about their SpamGreek font which has 'B' at 194 */
    /* SKS 06apr95 stop stupid schools wingeing about their draft mode stuff having moved down slightly as a result of the above */
    if(!draft_mode)
        if( p_font_context->ascent < limit_min)
            p_font_context->ascent = limit_min;

    } /*block*/
}

#elif WINDOWS

static void
host_font_context_fill_info_windows(
    _InoutRef_  P_FONT_CONTEXT p_font_context,
    _InRef_     P_HOST_FONT_SPEC p_host_font_spec)
{
    /* avoid rereading font info. note - may need to throw away font info on printer change, new font load etc */
#if defined(USE_CACHED_ABC_WIDTHS)
    if(0 == p_font_context->h_abc_widths)
#else
    if(0 == p_font_context->space_width)
#endif
    {
        HOST_FONT host_font_formatting = p_font_context->host_font_formatting;
        const HDC hic_format_pixits = host_get_hic_format_pixits();
        HFONT h_font_old = SelectFont(hic_format_pixits, host_font_formatting);
        HOST_FONT host_font_ss = HOST_FONT_NONE;
        TEXTMETRIC textmetric;
        BOOL res;

        res = GetTextMetrics(hic_format_pixits, &textmetric);
        assert(res);

#if defined(USE_CACHED_ABC_WIDTHS)

#if APP_UNICODE
        /* SKS temp hack for transition to Unicode build */
        if(textmetric.tmLastChar > 255)
            textmetric.tmLastChar = 255;
#endif

        { /* read all the ABC character widths into the font context */
        P_ABC p_abc;
        STATUS status;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_abc), TRUE);

        if(NULL != (p_abc = al_array_alloc(&p_font_context->h_abc_widths, ABC, 256, &array_init_block, &status)))
        {
            res = GetCharABCWidths(hic_format_pixits, textmetric.tmFirstChar, textmetric.tmLastChar, &p_abc[textmetric.tmFirstChar]);
            if(!res)
            {
                /* printer driver not yielding Outline fonts info so invent some */
                int p_int[256];
                int i;
                res = GetCharWidth(hic_format_pixits, textmetric.tmFirstChar, textmetric.tmLastChar, &p_int[textmetric.tmFirstChar]);
                assert(res);
                for(i = textmetric.tmFirstChar; i <= textmetric.tmLastChar; ++i)
                    p_abc[i].abcB = p_int[i]; // abcA and abcC left zero from init
            }

            if((textmetric.tmFirstChar != 0x00) || (textmetric.tmLastChar != 0xFF))
            {
                ABC default_abc;
                int i;
                res = GetCharABCWidths(hic_format_pixits, textmetric.tmDefaultChar, textmetric.tmDefaultChar, &default_abc);
                if(!res)
                {
                    int width;
                    res = GetCharWidth(hic_format_pixits, textmetric.tmDefaultChar, textmetric.tmDefaultChar, &width);
                    assert(res);
                    default_abc.abcA = 0;
                    default_abc.abcB = width;
                    default_abc.abcC = 0;
                }
                for(i = 0x00; i < textmetric.tmFirstChar; ++i)
                    p_abc[i] = default_abc;
                for(i = textmetric.tmLastChar + 1; i <= 0xFF; ++i)
                    p_abc[i] = default_abc;
            }

            if((textmetric.tmFirstChar <= CH_SPACE) && (textmetric.tmLastChar >= CH_SPACE));
                p_font_context->space_width = p_abc[CH_SPACE].abcA + p_abc[CH_SPACE].abcB + p_abc[CH_SPACE].abcC;
        }

        status_assert(status);
        } /*block*/

#else /* NOT defined(USE_CACHED_ABC_WIDTHS) */

        { /* read a single ABC character width */
        ABC abc;

        res = GetCharABCWidths(hic_format_pixits, CH_SPACE, CH_SPACE, &abc);
        if(res)
        {
            p_font_context->space_width = abc.abcA + abc.abcB + abc.abcC;
        }
        else
        {
            int w;
            res = GetCharWidth(hic_format_pixits, CH_SPACE, CH_SPACE, &w);
            assert(res);
            p_font_context->space_width = w;
        }
        } /*block*/

#endif /* defined(USE_CACHED_ABC_WIDTHS) */

        /* super- and sub- scripts must use height/ascent of 'source' font */
        if(p_host_font_spec->superscript || p_host_font_spec->subscript)
        {
            host_font_ss = host_font_find(p_host_font_spec, P_REDRAW_CONTEXT_NONE);
            assert(HOST_FONT_NONE != host_font_ss);
            consume(HFONT, SelectFont(hic_format_pixits, host_font_ss));
            res = GetTextMetrics(hic_format_pixits, &textmetric);
            assert(res);
        }

        p_font_context->ascent = textmetric.tmAscent;

        consume(HFONT, SelectFont(hic_format_pixits, h_font_old));

        if(HOST_FONT_NONE != host_font_ss)
            DeleteFont(host_font_ss);
    }
}

#endif /* OS */

/******************************************************************************
*
* gets an OS font handle from an internal fonty handle
*
* Use the scaling info contained in the REDRAW_CONTEXT
* or use 1:1 scaling when there is no drawing context (e.g. formatting)
*
******************************************************************************/

_Check_return_
extern HOST_FONT
fonty_host_font_from_fonty_handle_formatting(
    _InVal_     FONTY_HANDLE fonty_handle,
    _InVal_     BOOL draft_mode)
{
    const P_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle_wr(fonty_handle);
    HOST_FONT_SPEC host_font_spec;

    if(p_font_context->flags.host_font_formatting_set)
        return(p_font_context->host_font_formatting);

    status_assert(fontmap_host_font_spec_from_font_spec(&host_font_spec, &p_font_context->font_spec, FALSE));
    {
    HOST_FONT_SPEC host_font_spec_adjust = host_font_spec;
    fonty_size_adjust_attributes(&host_font_spec_adjust, draft_mode);
    p_font_context->host_font_formatting = host_font_find(&host_font_spec_adjust, P_REDRAW_CONTEXT_NONE);
    p_font_context->flags.host_font_formatting_set = 1;
    } /*block*/

    /* now read some information about this font which we have just purchased */
#if RISCOS
    host_font_context_fill_info_riscos(p_font_context, &host_font_spec, draft_mode);
#elif WINDOWS
    host_font_context_fill_info_windows(p_font_context, &host_font_spec);
#endif /* OS */

    host_font_spec_dispose(&host_font_spec);
    return(p_font_context->host_font_formatting);
}

_Check_return_
extern HOST_FONT
fonty_host_font_from_fonty_handle_redraw(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InVal_     FONTY_HANDLE fonty_handle,
    _InVal_     BOOL draft_mode)
{
    const P_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle_wr(fonty_handle);
    HOST_FONT_SPEC host_font_spec;

    if(p_font_context->flags.host_font_redraw_set)
        return(p_font_context->host_font_redraw);

    status_assert(fontmap_host_font_spec_from_font_spec(&host_font_spec, &p_font_context->font_spec, FALSE));
    {
    HOST_FONT_SPEC host_font_spec_adjust = host_font_spec;
    fonty_size_adjust_attributes(&host_font_spec_adjust, draft_mode);
    p_font_context->host_font_redraw = host_font_find(&host_font_spec_adjust, p_redraw_context);
    p_font_context->flags.host_font_redraw_set = 1;
    } /*block*/

    host_font_spec_dispose(&host_font_spec);
    return(p_font_context->host_font_redraw);
}

#if RISCOS

_Check_return_
extern HOST_FONT
fonty_host_font_utf8_from_fonty_handle_formatting(
    _InVal_     FONTY_HANDLE fonty_handle,
    _InVal_     BOOL draft_mode)
{
    const P_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle_wr(fonty_handle);
    HOST_FONT_SPEC host_font_spec;

    if(p_font_context->flags.host_font_utf8_formatting_set)
        return(p_font_context->host_font_utf8_formatting);

    status_assert(fontmap_host_font_spec_from_font_spec(&host_font_spec, &p_font_context->font_spec, TRUE));
    {
    HOST_FONT_SPEC host_font_spec_adjust = host_font_spec;
    fonty_size_adjust_attributes(&host_font_spec_adjust, draft_mode);
    p_font_context->host_font_utf8_formatting = host_font_find(&host_font_spec_adjust, P_REDRAW_CONTEXT_NONE);
    p_font_context->flags.host_font_utf8_formatting_set = 1;
    } /*block*/

    host_font_spec_dispose(&host_font_spec);
    return(p_font_context->host_font_utf8_formatting);
}

_Check_return_
extern HOST_FONT
fonty_host_font_utf8_from_fonty_handle_redraw(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InVal_     FONTY_HANDLE fonty_handle,
    _InVal_     BOOL draft_mode)
{
    const P_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle_wr(fonty_handle);
    HOST_FONT_SPEC host_font_spec;

    PTR_ASSERT(p_redraw_context);

    if(p_font_context->flags.host_font_utf8_redraw_set)
        return(p_font_context->host_font_utf8_redraw);

    status_assert(fontmap_host_font_spec_from_font_spec(&host_font_spec, &p_font_context->font_spec, TRUE));
    {
    HOST_FONT_SPEC host_font_spec_adjust = host_font_spec;
    fonty_size_adjust_attributes(&host_font_spec_adjust, draft_mode);
    p_font_context->host_font_utf8_redraw = host_font_find(&host_font_spec_adjust, p_redraw_context);
    p_font_context->flags.host_font_utf8_redraw_set = 1;
    } /*block*/

    host_font_spec_dispose(&host_font_spec);
    return(p_font_context->host_font_utf8_redraw);
}

#endif /* RISCOS */

/******************************************************************************
*
* duplicate a font spec - indirected name too
*
******************************************************************************/

_Check_return_
extern STATUS
font_spec_duplicate(
    _OutRef_    P_FONT_SPEC p_font_spec_out,
    _InRef_     PC_FONT_SPEC p_font_spec_in)
{
    *p_font_spec_out = *p_font_spec_in;
    p_font_spec_out->h_app_name_tstr = 0;
    return(al_array_duplicate(&p_font_spec_out->h_app_name_tstr, &p_font_spec_in->h_app_name_tstr));
}

/******************************************************************************
*
* throw away our OS font handles for this fonty session
*
******************************************************************************/

extern void
fonty_cache_trash(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    const ARRAY_INDEX n_elements = array_elements(&h_font_cache);
    ARRAY_INDEX array_index;
    P_FONT_CONTEXT p_font_context = array_range(&h_font_cache, FONT_CONTEXT, 0, n_elements);

    for(array_index = 0; array_index < n_elements; ++array_index, ++p_font_context)
    {
        if(p_font_context->flags.deleted)
            continue;

        if(p_font_context->flags.host_font_formatting_set)
        {
            p_font_context->flags.host_font_formatting_set = 0;

            host_font_dispose(&p_font_context->host_font_formatting, p_redraw_context);
        }

        if(p_font_context->flags.host_font_redraw_set)
        {
            p_font_context->flags.host_font_redraw_set = 0;

            host_font_dispose(&p_font_context->host_font_redraw, p_redraw_context);
        }

#if RISCOS
        if(p_font_context->flags.host_font_utf8_formatting_set)
        {
            p_font_context->flags.host_font_utf8_formatting_set = 0;

            host_font_dispose(&p_font_context->host_font_utf8_formatting, p_redraw_context);
        }

        if(p_font_context->flags.host_font_utf8_redraw_set)
        {
            p_font_context->flags.host_font_utf8_redraw_set = 0;

            host_font_dispose(&p_font_context->host_font_utf8_redraw, p_redraw_context);
        }
#endif
    }

    trace_0(TRACE_APP_FORMAT, TEXT("fonty_cache_trash"));
}

static void
fonty_cache_trash_all(void)
{
    fonty_cache_trash(P_REDRAW_CONTEXT_NONE);

    { /* free font names */
    const ARRAY_INDEX n_elements = array_elements(&h_font_cache);
    ARRAY_INDEX array_index;

    for(array_index = 0; array_index < n_elements; ++array_index)
    {
        const P_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle_wr(array_index);

        if(p_font_context->flags.deleted)
            continue;

        font_spec_dispose(&p_font_context->font_spec);

#if WINDOWS && defined(USE_CACHED_ABC_WIDTHS)
        al_array_dispose(&p_font_context->h_abc_widths);
#endif
    }
    } /*block*/

    al_array_dispose(&h_font_cache);
}

/******************************************************************************
*
* given a position in pixits, find the character index into the fonty chunk
*
******************************************************************************/

_Check_return_
extern S32
fonty_chunk_index_from_pixit(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_FONTY_CHUNK p_fonty_chunk,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     S32 uchars_n,
    _InoutRef_  P_PIXIT p_pos)
{
    HOST_FONT host_font = fonty_host_font_from_fonty_handle_formatting(p_fonty_chunk->fonty_handle, p_docu->flags.draft_mode);

#if RISCOS

    U32 term = (U32) uchars_n;
    S32 pos_mp = (*p_pos * MILLIPOINTS_PER_PIXIT);

    pos_mp += MILLIPOINTS_PER_PIXIT / 2;

    fonty_complain(fonty_scanstring(host_font, &pos_mp, uchars, &term));

    *p_pos = pixits_from_millipoints_ceil(pos_mp);

    return(term);

#elif WINDOWS

    const HDC hic_format_pixits = host_get_hic_format_pixits();
    HFONT h_font_old = SelectFont(hic_format_pixits, host_font);
    int x_stop = (int) *p_pos;
    int term = (int) uchars_n;
    int last_extent = 0, extent = 0, ix = 0;
    SIZE size;

    while((ix < term) && (extent < x_stop))
    {
        ix += 1;

        if(status_fail(uchars_GetTextExtentPoint32(hic_format_pixits, uchars, ix, &size)))
            break;

        last_extent = extent;
        extent = size.cx;
    }

    if(ix)
    {
        int last_diff = x_stop - last_extent;
        int      diff = extent - x_stop;

        if(diff <= last_diff)
            *p_pos = (S32) extent;
        else
        {
            ix -= 1;
            *p_pos = (S32) last_extent;
        }
    }

    consume(HFONT, SelectFont(hic_format_pixits, h_font_old));

    return(ix);

#endif /* OS */
}

/******************************************************************************
*
* set up the info about a fonty chunk
*
******************************************************************************/

extern void
fonty_chunk_info_read_uchars(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_FONTY_CHUNK p_fonty_chunk,
    _InVal_     FONTY_HANDLE fonty_handle,
    _In_reads_(uchars_len_no_spaces) PC_UCHARS uchars,
    _InVal_     S32 uchars_len_no_spaces,
    _InVal_     S32 trail_spaces)
{
    const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);

    p_fonty_chunk->fonty_handle = fonty_handle;
    p_fonty_chunk->trail_space = p_font_context->space_width * trail_spaces;
    p_fonty_chunk->ascent = p_font_context->ascent;
    p_fonty_chunk->underline = p_font_context->font_spec.underline;
    CODE_ANALYSIS_ONLY(p_fonty_chunk->_spare[0] = p_fonty_chunk->_spare[1] = p_fonty_chunk->_spare[2] = 0);

#if RISCOS

    {
    HOST_FONT host_font = fonty_host_font_from_fonty_handle_formatting(fonty_handle, p_docu->flags.draft_mode);
    U32 term = (U32) uchars_len_no_spaces;

    p_fonty_chunk->width_mp = 0;

    fonty_complain(fonty_scanstring(host_font, &p_fonty_chunk->width_mp, uchars, &term));

    p_fonty_chunk->width = pixits_from_millipoints_ceil(p_fonty_chunk->width_mp) + p_fonty_chunk->trail_space;
    } /*block*/

#elif WINDOWS

#if defined(USE_CACHED_ABC_WIDTHS)
    if(p_font_context->h_abc_widths)
    {
        PIXIT x = 0;
        PC_U8 p_u8 = (PC_U8) uchars;
        S32 i;
        PC_ABC p_abc = array_rangec(&p_font_context->h_abc_widths, ABC, 0, 256);

        for(i = 0; i < uchars_len_no_spaces; ++i)
        {
            U8 ch = *p_u8++;

            x += p_abc[ch].abcA + p_abc[ch].abcB + p_abc[ch].abcC;
        }

#if TRACE_ALLOWED
        if_constant(tracing(TRACE_APP_FONTS))
        {
            trace_4(TRACE_APP_FONTS,
                    TEXT("ABC==uchars_GetTextExtentPoint32: got: %d %s for %.*s"),
                    x,
                    TEXT("twips"),
                    uchars_len_no_spaces, report_ustr(uchars));
        }
#endif

        if(0 != uchars_len_no_spaces)
        {
            int underhang_first, overhang_last;

            underhang_first = p_abc[PtrGetByteOff(uchars, 0)].abcA;

            if(underhang_first < 0)
                x -= underhang_first;

            overhang_last = p_abc[PtrGetByteOff(uchars, uchars_len_no_spaces - 1)].abcC;

            if(overhang_last < 0)
                x -= overhang_last;
        }

        p_fonty_chunk->width = x + p_fonty_chunk->trail_space;
    }
    else
#endif /* defined(USE_CACHED_ABC_WIDTHS) */
    {
        HOST_FONT host_font = fonty_host_font_from_fonty_handle_formatting(fonty_handle, p_docu->flags.draft_mode);
        const HDC hic_format_pixits = host_get_hic_format_pixits();
        HFONT h_font_old = SelectFont(hic_format_pixits, host_font);
        SIZE size;

        status_consume(uchars_GetTextExtentPoint32(hic_format_pixits, uchars, (U32) uchars_len_no_spaces, &size));

        if(0 != uchars_len_no_spaces)
        {   /* Caters for any underhang of first character and overhang of last character rendered in Italics */
            /* No need to have backup GetCharWidth as we are only considering abcA and abcC here */
            ABC abc;
            BOOL abc_ok = FALSE;

#if APP_UNICODE
            PC_UCHARS uchars_last;
            UCS4 ucs4_left, ucs4_right;

            ucs4_left = uchars_char_decode_NULL(uchars);
            abc_ok = GetCharABCWidthsW(hic_format_pixits, (UINT) ucs4_left, (UINT) ucs4_left, &abc);
#else
            TCHAR tch_left, tch_right;

            tch_left = PtrGetByteOff(uchars, 0);
            abc_ok = GetCharABCWidths(hic_format_pixits, (UINT) tch_left, (UINT) tch_left, &abc);
#endif

            if(abc_ok)
            {
                int underhang_first = abc.abcA;

                if(underhang_first < 0)
                    size.cx -= underhang_first;
            }

            /* we can save a call if left & right characters are the same as we've already got the data */
#if APP_UNICODE
            ucs4_right = utf8_char_prev(uchars, uchars_AddBytes(uchars, uchars_len_no_spaces), &uchars_last);
            if(ucs4_right != ucs4_left)
                abc_ok = GetCharABCWidthsW(hic_format_pixits, (UINT) ucs4_right, (UINT) ucs4_right, &abc);
#else
            tch_right = PtrGetByteOff(uchars, uchars_len_no_spaces - 1);
            if(tch_right != tch_left)
                abc_ok = GetCharABCWidths(hic_format_pixits, (UINT) tch_right, (UINT) tch_right, &abc);
#endif

            if(abc_ok)
            {
                int overhang_last = abc.abcC;

                if(overhang_last < 0)
                    size.cx -= overhang_last;
            }
        }

        consume(HFONT, SelectFont(hic_format_pixits, h_font_old));

        p_fonty_chunk->width = size.cx + p_fonty_chunk->trail_space;
    }

#endif /* WINDOWS */
}

#if RISCOS && USTR_IS_SBSTR

extern void
fonty_chunk_info_read_utf8(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_FONTY_CHUNK p_fonty_chunk,
    _InVal_     FONTY_HANDLE fonty_handle,
    _In_reads_(uchars_len_no_spaces) PC_UCHARS utf8,
    _InVal_     S32 uchars_len_no_spaces,
    _InVal_     S32 trail_spaces)
{
    const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);

    p_fonty_chunk->fonty_handle = fonty_handle;
    p_fonty_chunk->trail_space = p_font_context->space_width * trail_spaces;
    p_fonty_chunk->ascent = p_font_context->ascent;
    p_fonty_chunk->underline = p_font_context->font_spec.underline;
    CODE_ANALYSIS_ONLY(p_fonty_chunk->_spare[0] = p_fonty_chunk->_spare[1] = p_fonty_chunk->_spare[2] = 0);

    {
    HOST_FONT host_font_utf8 = fonty_host_font_utf8_from_fonty_handle_formatting(fonty_handle, p_docu->flags.draft_mode);
    U32 term = (U32) uchars_len_no_spaces;

    p_fonty_chunk->width_mp = 0;

    fonty_complain(fonty_scanstring(host_font_utf8, &p_fonty_chunk->width_mp, utf8, &term));

    p_fonty_chunk->width = pixits_from_millipoints_ceil(p_fonty_chunk->width_mp) + p_fonty_chunk->trail_space;
    } /*block*/
}

#endif /* RISCOS && USTR_IS_SBSTR */

#if WINDOWS && TSTR_IS_SBSTR

extern void
fonty_chunk_info_read_wchars(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_FONTY_CHUNK p_fonty_chunk,
    _InVal_     FONTY_HANDLE fonty_handle,
    _In_reads_(wchars_len_no_spaces) PCWCH wchars,
    _InVal_     S32 wchars_len_no_spaces,
    _InVal_     S32 trail_spaces)
{
    const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);

    p_fonty_chunk->fonty_handle = fonty_handle;
    p_fonty_chunk->trail_space = p_font_context->space_width * trail_spaces;
    p_fonty_chunk->ascent = p_font_context->ascent;
    p_fonty_chunk->underline = p_font_context->font_spec.underline;
    CODE_ANALYSIS_ONLY(p_fonty_chunk->_spare[0] = p_fonty_chunk->_spare[1] = p_fonty_chunk->_spare[2] = 0);

    {
        HOST_FONT host_font = fonty_host_font_from_fonty_handle_formatting(fonty_handle, p_docu->flags.draft_mode);
        const HDC hic_format_pixits = host_get_hic_format_pixits();
        HFONT h_font_old = SelectFont(hic_format_pixits, host_font);
        SIZE size;

        void_WrapOsBoolChecking(GetTextExtentPoint32W(hic_format_pixits, wchars, (U32) wchars_len_no_spaces, &size));

        if(0 != wchars_len_no_spaces)
        {   /* Caters for any underhang of first character and overhang of last character rendered in Italics */
            ABC abc;
            BOOL abc_ok = FALSE;
            WCHAR wchar_left, wchar_right;

            wchar_left = wchars[0];
            abc_ok = GetCharABCWidthsW(hic_format_pixits, wchar_left, wchar_left, &abc);

            if(abc_ok)
            {
                int underhang_first = abc.abcA;

                if(underhang_first < 0)
                    size.cx -= underhang_first;
            }

            /* we can save a call if left & right characters are the same as we've already got the data */
            wchar_right = wchars[wchars_len_no_spaces - 1];
            if(wchar_right != wchar_left)
                abc_ok = GetCharABCWidthsW(hic_format_pixits, wchar_right, wchar_right, &abc);

            if(abc_ok)
            {
                int overhang_last = abc.abcC;

                if(overhang_last < 0)
                    size.cx -= overhang_last;
            }
        }

        p_fonty_chunk->width = size.cx + p_fonty_chunk->trail_space;

        consume(HFONT, SelectFont(hic_format_pixits, h_font_old));
    } /*block*/
}

#endif /* WINDOWS && TSTR_IS_SBSTR */

/******************************************************************************
*
* initialise a fonty chunk
*
******************************************************************************/

extern void
fonty_chunk_init(
    P_FONTY_CHUNK p_fonty_chunk,
    _InVal_     FONTY_HANDLE fonty_handle)
{
    const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);

    p_fonty_chunk->width = 0;
    p_fonty_chunk->trail_space = 0;
    p_fonty_chunk->ascent = p_font_context->ascent;
    p_fonty_chunk->fonty_handle = fonty_handle;
}

#if RISCOS

/******************************************************************************
*
* report RISCOS font error
*
******************************************************************************/

static _kernel_oserror *
fonty_complain(
    _kernel_oserror * err)
{
    myassert0x(!err, err->errmess);
    return(err);
}

#endif

/******************************************************************************
*
* get an internal fonty handle for the font specified
*
******************************************************************************/

_Check_return_
extern STATUS /*FONTY_HANDLE*/
fonty_handle_from_font_spec(
    _InRef_     PC_FONT_SPEC p_font_spec,
    _InVal_     BOOL draft_mode)
{
    const ARRAY_INDEX n_elements = array_elements(&h_font_cache);
    ARRAY_INDEX array_index;
    P_FONT_CONTEXT p_font_context = array_range(&h_font_cache, FONT_CONTEXT, 0, n_elements);
    ARRAY_INDEX spare_index = -1;
    FONTY_HANDLE fonty_handle;

    /* search font context list for same font */
    for(array_index = 0; array_index < n_elements; ++array_index, ++p_font_context)
    {
        if(p_font_context->flags.deleted)
        {
            spare_index = array_index;
            continue;
        }

        if(!fonty_spec_compare_not_equals(&p_font_context->font_spec, p_font_spec))
            return(array_index);
    }

    /* get a font context */
    if(spare_index < 0)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(5, sizeof32(FONT_CONTEXT), TRUE);
        STATUS status;

        if(NULL == (p_font_context = al_array_extend_by(&h_font_cache, FONT_CONTEXT, 1, &array_init_block, &status)))
        {
            status_assert(status);
            return(STATUS_FAIL);
        }

        trace_2(TRACE_APP_MEMORY_USE,
                TEXT("font_context table now ") S32_TFMT TEXT(" entries, ") S32_TFMT TEXT(" bytes"),
                array_elements(&h_font_cache), array_elements(&h_font_cache) * sizeof32(FONT_CONTEXT));
    }
    else
        p_font_context = p_font_context_from_fonty_handle_wr(spare_index);

    /* initialise the font context */
    if(status_fail(font_spec_duplicate(&p_font_context->font_spec, p_font_spec)))
    {
        p_font_context->flags.deleted = 1;
        return(STATUS_FAIL);
    }

    /* set internal font handle */
    fonty_handle = array_indexof_element(&h_font_cache, FONT_CONTEXT, p_font_context);

    /* reads info about this font into font context */
    consume(HOST_FONT, fonty_host_font_from_fonty_handle_formatting(fonty_handle, draft_mode));

    return(fonty_handle);
}

/******************************************************************************
*
* calculate super- / sub- shifts for Drawfile
*
******************************************************************************/

_Check_return_
extern PIXIT
drawfile_fonty_paint_calc_shift_y(
    _InRef_     PC_FONT_CONTEXT p_font_context)
{
    if(p_font_context->font_spec.superscript)
        return(-((p_font_context->font_spec.size_y * 24) / 64));

    if(p_font_context->font_spec.subscript)
        return(+((p_font_context->font_spec.size_y * 9 ) / 64));

    return(0);
}

/******************************************************************************
*
* test two fonty specs for equality;
* return difference in difference bitmap
*
* --out--
* FALSE font specs are the same
* TRUE  font specs are different (NEQ)
*
* SKS 04dec95 cheap tests first, early out
* SKS 25sep14 separate compare/diff routines for efficiency
*
******************************************************************************/

_Check_return_
static BOOL
fonty_spec_compare_not_equals(
    _InRef_     PC_FONT_SPEC p_font_spec_1,
    _InRef_     PC_FONT_SPEC p_font_spec_2)
{
    if(p_font_spec_1->size_y != p_font_spec_2->size_y)
        return(TRUE);

    if(p_font_spec_1->size_x != p_font_spec_2->size_x)
        return(TRUE);

    if(p_font_spec_1->bold != p_font_spec_2->bold)
        return(TRUE);

    if(p_font_spec_1->italic != p_font_spec_2->italic)
        return(TRUE);

    if(p_font_spec_1->underline != p_font_spec_2->underline)
        return(TRUE);

    if(p_font_spec_1->superscript != p_font_spec_2->superscript)
        return(TRUE);

    if(p_font_spec_1->subscript != p_font_spec_2->subscript)
        return(TRUE);

    if(rgb_compare_not_equals(&p_font_spec_1->colour, &p_font_spec_2->colour))
        return(TRUE);

    if(p_font_spec_1->h_app_name_tstr != p_font_spec_2->h_app_name_tstr)
    {
        BOOL names_differ = TRUE;

        if((0 != p_font_spec_1->h_app_name_tstr) && (0 != p_font_spec_2->h_app_name_tstr))
        {
            const PCTSTR tstr_1 = array_tstr(&p_font_spec_1->h_app_name_tstr);
            const PCTSTR tstr_2 = array_tstr(&p_font_spec_2->h_app_name_tstr);

            names_differ = !tstr_compare_equals_nocase(tstr_1, tstr_2);
        }

        if(names_differ)
            return(TRUE);
    }

    return(FALSE);
}

#if defined(UNUSED_KEEP_ALIVE)

_Check_return_
/*static*/ BOOL
fonty_spec_diff_not_equals(
    _OutRef_    P_BITMAP p_diff_bitmap,
    _InRef_     PC_FONT_SPEC p_font_spec_1,
    _InRef_     PC_FONT_SPEC p_font_spec_2)
{
    bitmap_clear(p_diff_bitmap, N_BITS_ARG(FONTY_BIT_COUNT));

    if(p_font_spec_1->size_y != p_font_spec_2->size_y)
        bitmap_bit_set(p_diff_bitmap, FONTY_SIZE_Y, N_BITS_ARG(FONTY_BIT_COUNT));

    if(p_font_spec_1->size_x != p_font_spec_2->size_x)
        bitmap_bit_set(p_diff_bitmap, FONTY_SIZE_X, N_BITS_ARG(FONTY_BIT_COUNT));

    if(p_font_spec_1->bold != p_font_spec_2->bold)
        bitmap_bit_set(p_diff_bitmap, FONTY_BOLD, N_BITS_ARG(FONTY_BIT_COUNT));

    if(p_font_spec_1->italic != p_font_spec_2->italic)
        bitmap_bit_set(p_diff_bitmap, FONTY_ITALIC, N_BITS_ARG(FONTY_BIT_COUNT));

    if(p_font_spec_1->underline != p_font_spec_2->underline)
        bitmap_bit_set(p_diff_bitmap, FONTY_UNDERLINE, N_BITS_ARG(FONTY_BIT_COUNT));

    if(p_font_spec_1->superscript != p_font_spec_2->superscript)
        bitmap_bit_set(p_diff_bitmap, FONTY_SUPERSCRIPT, N_BITS_ARG(FONTY_BIT_COUNT));

    if(p_font_spec_1->subscript != p_font_spec_2->subscript)
        bitmap_bit_set(p_diff_bitmap, FONTY_SUBSCRIPT, N_BITS_ARG(FONTY_BIT_COUNT));

    if(rgb_compare_not_equals(&p_font_spec_1->colour, &p_font_spec_2->colour))
        bitmap_bit_set(p_diff_bitmap, FONTY_COLOUR, N_BITS_ARG(FONTY_BIT_COUNT));

    if(p_font_spec_1->h_app_name_tstr != p_font_spec_2->h_app_name_tstr)
    {
        BOOL names_differ = TRUE;

        if((0 != p_font_spec_1->h_app_name_tstr) && (0 != p_font_spec_2->h_app_name_tstr))
        {
            const PCTSTR tstr_1 = array_tstr(&p_font_spec_1->h_app_name_tstr);
            const PCTSTR tstr_2 = array_tstr(&p_font_spec_2->h_app_name_tstr);

            names_differ = !tstr_compare_equals_nocase(tstr_1, tstr_2);
        }

        if(names_differ)
            bitmap_bit_set(p_diff_bitmap, FONTY_NAME, N_BITS_ARG(FONTY_BIT_COUNT));
    }

    return(bitmap_any(p_diff_bitmap, N_BITS_ARG(FONTY_BIT_COUNT)));
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* Host font name <-> Fireworkz font name mapping
*
******************************************************************************/

#if WINDOWS

_Check_return_
extern int CALLBACK
fontmap_enum_faces_callback(
    _In_        CONST LOGFONT *lplf,
    _In_        CONST TEXTMETRIC *lpntm,
    _In_        DWORD font_type,
    _In_        LPARAM lParam);

#endif /* OS */

static const PCTSTR
rtf_class_name[] =
{
#define RTF_CLASS_ROMAN 0
    TEXT("roman"),

#define RTF_CLASS_SWISS 1
    TEXT("swiss"),

#define RTF_CLASS_MODERN 2
    TEXT("modern"),

#define RTF_CLASS_SCRIPT 3
    TEXT("script"),

#define RTF_CLASS_DECOR 4
    TEXT("decor"),

#define RTF_CLASS_TECH 5
    TEXT("tech")
};

/* NB host font name comparisions must be case-insensitive */

#if RISCOS

PROC_BSEARCH_PROTO_Z(static, fontmap_compare_host_fullname_bsearch, TCHARZ, HOST_FONT_TAB)
{
    BSEARCH_KEY_VAR_DECL(PCTSTR, s1);
    BSEARCH_DATUM_VAR_DECL(PC_HOST_FONT_TAB, p2);
    PCTSTR s2 = p2->fullname;

    return(tstricmp(s1, s2));
}

PROC_QSORT_PROTO(static, fontmap_compare_host_fullname_qsort, HOST_FONT_TAB)
{
    QSORT_ARG1_VAR_DECL(PC_HOST_FONT_TAB, p1);
    QSORT_ARG2_VAR_DECL(PC_HOST_FONT_TAB, p2);
    PCTSTR s1 = p1->fullname;
    PCTSTR s2 = p2->fullname;

    return(tstricmp(s1, s2));
}

#elif WINDOWS

PROC_BSEARCH_PROTO(static, fontmap_compare_host_logfont_bsearch, LOGFONT, HOST_FONT_TAB)
{
    BSEARCH_KEY_VAR_DECL(LOGFONT * const, l1);
    BSEARCH_DATUM_VAR_DECL(PC_HOST_FONT_TAB, p2);
    const LOGFONT * const l2 = &p2->logfont;
    PCTSTR s1 = l1->lfFaceName;
    PCTSTR s2 = l2->lfFaceName;
    int res;

    if(0 != (res = tstricmp(s1, s2)))
        return(res);

    if(l1->lfWeight > l2->lfWeight)
        return(+1);
    if(l1->lfWeight < l2->lfWeight)
        return(-1);

    if(l1->lfItalic > l2->lfItalic)
        return(+1);
    if(l1->lfItalic < l2->lfItalic)
        return(-1);

    return(0);
}

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, fontmap_compare_host_logfont_qsort, HOST_FONT_TAB)
{
    QSORT_ARG1_VAR_DECL(PC_HOST_FONT_TAB, p1);
    QSORT_ARG2_VAR_DECL(PC_HOST_FONT_TAB, p2);
    const LOGFONT * l1 = &p1->logfont;
    const LOGFONT * l2 = &p2->logfont;
    PCTSTR s1 = l1->lfFaceName;
    PCTSTR s2 = l2->lfFaceName;
    int res;

    if(0 != (res = tstricmp(s1, s2)))
        return(res);

    if(l1->lfWeight > l2->lfWeight)
        return(+1);
    if(l1->lfWeight < l2->lfWeight)
        return(-1);

    if(l1->lfItalic > l2->lfItalic)
        return(+1);
    if(l1->lfItalic < l2->lfItalic)
        return(-1);

    return(0);
}

PROC_BSEARCH_PROTO_Z(static, fontmap_compare_host_logfont_facename_lsearch, TCHARZ, HOST_FONT_TAB)
{
    BSEARCH_KEY_VAR_DECL(PCTSTR, s1);
    BSEARCH_DATUM_VAR_DECL(PC_HOST_FONT_TAB, p2);
    const LOGFONT * const l2 = &p2->logfont;
    PCTSTR s2 = l2->lfFaceName;
    int res;

    if(0 != (res = tstricmp(s1, s2)))
        return(res);

    return(0);
}

#endif /* OS */

/******************************************************************************
*
* Compare supplied font name with Fireworkz font names in table
*
* SKS 10may93 after 1.03 - Fireworkz names must be case-sensitive
*
******************************************************************************/

PROC_BSEARCH_PROTO_Z(static, fontmap_compare_app_name_bsearch, TCHARZ, APP_FONT_TAB)
{
    BSEARCH_KEY_VAR_DECL(PCTSTR, tstr_key);
    BSEARCH_DATUM_VAR_DECL(P_APP_FONT_TAB, datum);
    PCTSTR tstr_datum = datum->tstr_app_font_name;

    return(tstrcmp(tstr_key, tstr_datum));
}

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, fontmap_compare_app_name_qsort, APP_FONT_TAB)
{
    QSORT_ARG1_VAR_DECL(PC_APP_FONT_TAB, p1);
    QSORT_ARG2_VAR_DECL(PC_APP_FONT_TAB, p2);

    PCTSTR s1 = p1->tstr_app_font_name;
    PCTSTR s2 = p2->tstr_app_font_name;

    return(tstrcmp(s1, s2));
}

static void
fontmap_table_host_dispose(void)
{
#if RISCOS
    ARRAY_INDEX host_index;

    for(host_index = 0; host_index < array_elements(&fontmap_table_host); ++host_index)
    {
        P_HOST_FONT_TAB p_host_font_tab = p_host_entry(host_index);

        tstr_clr(&p_host_font_tab->fullname);
    }
#endif

    al_array_dispose(&fontmap_table_host);
}

#if CHECKING

static void
fontmap_table_app_dispose(void)
{
    /* type5 entry names are one-time alloc'ed from config or auto-create */

    al_array_dispose(&fontmap_table_app);
}

static void
fontmap_table_remap_dispose(void)
{
    /* remap entry names are one-time alloc'ed from config */

    al_array_dispose(&fontmap_table_remap);
}

#endif /* CHECKING */

/******************************************************************************
*
* Attempts to find the appropriate Helvetica variant as
* a default otherwise returns first usable font from table
*
******************************************************************************/

static FONTMAP_BITS
fontmap_default_font(
    _OutRef_    P_P_APP_FONT_TAB p_p_font_tab,
    _In_        FONTMAP_BITS fontmap_bits,
    _InVal_     BOOL use_host_riscos)
{
    P_APP_FONT_TAB p_app_font_tab = al_array_bsearch(TEXT("Helvetica"), &fontmap_table_app, APP_FONT_TAB, fontmap_compare_app_name_bsearch);

    if(!IS_P_DATA_NONE(p_app_font_tab))
    {
        FONTMAP_BITS try_fontmap_bits;

#if RISCOS
        IGNOREPARM_InVal_(use_host_riscos);
        assert(!use_host_riscos);
#else
        if(use_host_riscos)
            try_fontmap_bits = fontmap_remap_within_font_riscos(p_app_font_tab, fontmap_bits);
        else
#endif
            try_fontmap_bits = fontmap_remap_within_font(p_app_font_tab, fontmap_bits);

        if(try_fontmap_bits >= 0)
            fontmap_bits = try_fontmap_bits;
        else
            p_app_font_tab = P_APP_FONT_TAB_NONE;
    }

    if(IS_P_DATA_NONE(p_app_font_tab))
    {
        ARRAY_INDEX app_index;

        for(app_index = 0; app_index < array_elements(&fontmap_table_app); ++app_index)
        {
            p_app_font_tab = array_ptr(&fontmap_table_app, APP_FONT_TAB, app_index);

            if(p_app_font_tab->host_font[FONTMAP_BASIC].host_array_index >= 0)
            {
                fontmap_bits = FONTMAP_BASIC;
                break;
            }
        }
    }

    *p_p_font_tab = p_app_font_tab;
    return(fontmap_bits);
}

/******************************************************************************
*
* Looks up the RTF class of the specified Fireworkz font
*
******************************************************************************/

_Check_return_
extern STATUS
fontmap_rtf_class_from_font_spec(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InRef_     PC_FONT_SPEC p_font_spec)
{
    const P_APP_FONT_TAB p_app_font_tab = al_array_bsearch(array_tstr(&p_font_spec->h_app_name_tstr), &fontmap_table_app, APP_FONT_TAB, fontmap_compare_app_name_bsearch);
    int rtf_class = !IS_P_DATA_NONE(p_app_font_tab) ? p_app_font_tab->rtf_class : RTF_CLASS_ROMAN;
    PCTSTR tstr_rtf_class_name = rtf_class_name[rtf_class];
    return(quick_tblock_tstr_add(p_quick_tblock, tstr_rtf_class_name));
}

/******************************************************************************
*
* returns the set of host fonts available on the host system for a Fireworkz font
*
******************************************************************************/

_Check_return_
extern STATUS
fontmap_host_fonts_available(
    P_BITMAP p_bitmap_out,
    _InVal_     ARRAY_INDEX app_index)
{
    P_APP_FONT_TAB p_app_font_tab;
    FONTMAP_BITS fontmap_bits;

    bitmap_clear(p_bitmap_out, N_BITS_ARG(FONTMAP_N_BITS));

    if(!array_index_valid(&fontmap_table_app, app_index))
    {
        assert(app_index >= 0);
        assert(app_index <  array_elements(&fontmap_table_app));
        return(create_error(ERR_TYPE5_FONT_NOT_FOUND));
    }

    p_app_font_tab = p_app_font_entry(app_index);

    for(fontmap_bits = FONTMAP_BASIC; fontmap_bits < FONTMAP_N_BITS; ENUM_INCR(FONTMAP_BITS, fontmap_bits))
        if(p_app_font_tab->host_font[fontmap_bits].host_array_index >= 0)
            bitmap_bit_set(p_bitmap_out, fontmap_bits, N_BITS_ARG(FONTMAP_N_BITS));

    return(STATUS_OK);
}

/******************************************************************************
*
* returns a host base spec from the index into the ***Fireworkz*** table
*
******************************************************************************/

_Check_return_
extern STATUS
fontmap_host_base_name_from_app_index(
    _OutRef_    P_ARRAY_HANDLE_TSTR p_array_handle_tstr,
    _InVal_     ARRAY_INDEX app_index)
{
    P_APP_FONT_TAB p_app_font_tab;

    *p_array_handle_tstr = 0;

    if(!array_index_valid(&fontmap_table_app, app_index))
    {
        assert(app_index >= 0);
        assert(app_index <  array_elements(&fontmap_table_app));
        return(create_error(ERR_TYPE5_FONT_NOT_FOUND));
    }

    p_app_font_tab = p_app_font_entry(app_index);

    return(al_tstr_set(p_array_handle_tstr, p_app_font_tab->tstr_host_base_name));
}

_Check_return_
extern STATUS
fontmap_host_base_name_from_app_font_name(
    _OutRef_    P_ARRAY_HANDLE_TSTR p_array_handle_tstr,
    _In_z_      PCTSTR tstr_app_font_name)
{
    STATUS app_index;

    *p_array_handle_tstr = 0;

    app_index = fontmap_app_index_from_app_font_name(tstr_app_font_name);

    if(status_fail(app_index))
        return(create_error(app_index));

    return(fontmap_host_base_name_from_app_index(p_array_handle_tstr, (ARRAY_INDEX) app_index));
}

extern ARRAY_INDEX
fontmap_host_base_names(void)
{
    return(array_elements(&fontmap_table_app));
}

/******************************************************************************
*
* Maps a Fireworkz font spec (full) to a host font spec
*
* Returns default if no match
*
******************************************************************************/

#if !RISCOS

_Check_return_
extern STATUS
fontmap_host_font_spec_riscos_from_font_spec(
    _OutRef_    P_HOST_FONT_SPEC p_host_font_spec /* h_host_name_tstr as RISC OS eg Trinity.Bold.Italic */,
    _InRef_     PC_FONT_SPEC p_font_spec_key /*key*/)
{
    STATUS status;
    PCTSTR tstr_app_font_name;
    P_APP_FONT_TAB p_app_font_tab;
    FONTMAP_BITS fontmap_bits, try_fontmap_bits;

    /* Keep same attributes where applicable */
    p_host_font_spec->size_x = p_font_spec_key->size_x;
    p_host_font_spec->size_y = p_font_spec_key->size_y;

    p_host_font_spec->underline = p_font_spec_key->underline;
    p_host_font_spec->superscript = p_font_spec_key->superscript;
    p_host_font_spec->subscript = p_font_spec_key->subscript;

    if(p_font_spec_key->bold)
    {
        fontmap_bits = p_font_spec_key->italic ? FONTMAP_BOLDITALIC : FONTMAP_BOLD;
    }
    else
    {
        fontmap_bits = p_font_spec_key->italic ? FONTMAP_ITALIC : FONTMAP_BASIC;
    }

    if(0 != array_elements(&p_font_spec_key->h_app_name_tstr))
    {
        tstr_app_font_name = array_tstr(&p_font_spec_key->h_app_name_tstr);

        p_app_font_tab = al_array_bsearch(tstr_app_font_name, &fontmap_table_app, APP_FONT_TAB, fontmap_compare_app_name_bsearch);
    }
    else
    {
        p_app_font_tab = P_APP_FONT_TAB_NONE;
    }

    if(IS_P_DATA_NONE(p_app_font_tab))
        try_fontmap_bits = fontmap_default_font(&p_app_font_tab, fontmap_bits, TRUE);
    else
    {
        try_fontmap_bits = fontmap_remap_within_font_riscos(p_app_font_tab, fontmap_bits);

        if(try_fontmap_bits == FONTMAP_INVALID)
        {
            try_fontmap_bits = fontmap_remap_using_rtf_class(&p_app_font_tab, fontmap_bits, TRUE, TRUE);

            if(try_fontmap_bits == FONTMAP_INVALID)
            {
                try_fontmap_bits = fontmap_remap_using_rtf_class(&p_app_font_tab, fontmap_bits, TRUE, FALSE);

                if(try_fontmap_bits == FONTMAP_INVALID)
                    try_fontmap_bits = fontmap_default_font(&p_app_font_tab, fontmap_bits, TRUE);
            }
        }
    }

    assert(try_fontmap_bits >= 0);

    status = al_tstr_set(&p_host_font_spec->h_host_name_tstr, p_app_font_tab->riscos_font[try_fontmap_bits].tstr_config);

    return(status);
}

#endif /* !RISCOS */

_Check_return_
extern STATUS
fontmap_host_font_spec_from_font_spec(
    _OutRef_    P_HOST_FONT_SPEC p_host_font_spec,
    _InRef_     PC_FONT_SPEC p_font_spec_key /*key*/,
    _InVal_     BOOL for_riscos_utf8)
{
    P_APP_FONT_TAB p_app_font_tab;
    FONTMAP_BITS fontmap_bits, try_fontmap_bits;

#if WINDOWS
    IGNOREPARM_InVal_(for_riscos_utf8);
#endif

    /* Keep same attributes where applicable */
    p_host_font_spec->size_x = p_font_spec_key->size_x;
    p_host_font_spec->size_y = p_font_spec_key->size_y;

    p_host_font_spec->underline = p_font_spec_key->underline;
    p_host_font_spec->superscript = p_font_spec_key->superscript;
    p_host_font_spec->subscript = p_font_spec_key->subscript;

    if(p_font_spec_key->bold)
    {
        fontmap_bits = p_font_spec_key->italic ? FONTMAP_BOLDITALIC : FONTMAP_BOLD;
    }
    else
    {
        fontmap_bits = p_font_spec_key->italic ? FONTMAP_ITALIC : FONTMAP_BASIC;
    }

    if(0 != array_elements(&p_font_spec_key->h_app_name_tstr))
    {
        PCTSTR tstr_app_font_name = array_tstr(&p_font_spec_key->h_app_name_tstr);

        p_app_font_tab = al_array_bsearch(tstr_app_font_name, &fontmap_table_app, APP_FONT_TAB, fontmap_compare_app_name_bsearch);
    }
    else
        p_app_font_tab = P_APP_FONT_TAB_NONE;

    if(IS_P_DATA_NONE(p_app_font_tab))
        try_fontmap_bits = fontmap_default_font(&p_app_font_tab, fontmap_bits, FALSE);
    else
    {
        try_fontmap_bits = fontmap_remap_within_font(p_app_font_tab, fontmap_bits);

        if(try_fontmap_bits == FONTMAP_INVALID)
        {
            try_fontmap_bits = fontmap_remap_using_rtf_class(&p_app_font_tab, fontmap_bits, FALSE, TRUE);

            if(try_fontmap_bits == FONTMAP_INVALID)
            {
                try_fontmap_bits = fontmap_remap_using_rtf_class(&p_app_font_tab, fontmap_bits, FALSE, FALSE);

                if(try_fontmap_bits == FONTMAP_INVALID)
                    try_fontmap_bits = fontmap_default_font(&p_app_font_tab, fontmap_bits, FALSE);
            }
        }
    }

    assert(try_fontmap_bits >= 0);

    {
    PC_HOST_FONT_TAB p_host_font_tab = p_host_entry(p_app_font_tab->host_font[try_fontmap_bits].host_array_index);
    PCTSTR tstr;

#if RISCOS
    /* fullname determines bold and italic */
    tstr = p_host_font_tab->fullname;

    if(for_riscos_utf8)
    {
        /* \EUTF8\Ffontname */
        status_return(al_tstr_set(&p_host_font_spec->h_host_name_tstr, TEXT("\\E") TEXT("UTF8") TEXT("\\F")));

        return(al_tstr_append(&p_host_font_spec->h_host_name_tstr, tstr));
    }

    /* NO!!! protect against evil-doers who might set UTF8 alphabet before we are ready for it! */
    /* \ELatin1\Ffontname */
    status_return(al_tstr_set(&p_host_font_spec->h_host_name_tstr, /*TEXT("\\E") TEXT("Latin1")*/ TEXT("\\F")));

    return(al_tstr_append(&p_host_font_spec->h_host_name_tstr, tstr));
    } /*block*/
#elif WINDOWS
    /* caller wants info to fill a LOGFONT */
    p_host_font_spec->logfont = p_host_font_tab->logfont;

    switch(try_fontmap_bits)
    {
    default:
        /* NB leave lfWeight,lfItalic as per the returned values */
        break;

    case FONTMAP_BOLD:
        p_host_font_spec->logfont.lfWeight = FW_BOLD;
        break;

    case FONTMAP_ITALIC:
        p_host_font_spec->logfont.lfItalic = TRUE;
        break;

    case FONTMAP_BOLDITALIC:
        p_host_font_spec->logfont.lfWeight = FW_BOLD;
        p_host_font_spec->logfont.lfItalic = TRUE;
        break;
    }

    tstr = p_host_font_tab->logfont.lfFaceName;

    return(al_tstr_set(&p_host_font_spec->h_host_name_tstr, tstr));
    } /*block*/
#endif
}

/******************************************************************************
*
* Returns the number in our table of the specified Fireworkz font name
*
******************************************************************************/

_Check_return_
extern STATUS
fontmap_app_index_from_app_font_name(
    _In_z_      PCTSTR tstr_app_font_name)
{
    const P_APP_FONT_TAB p_app_font_tab = al_array_bsearch(tstr_app_font_name, &fontmap_table_app, APP_FONT_TAB, fontmap_compare_app_name_bsearch);

    if(IS_P_DATA_NONE(p_app_font_tab))
        return(/*create_error*/(ERR_TYPE5_FONT_NOT_FOUND));

    return((STATUS) array_indexof_element(&fontmap_table_app, APP_FONT_TAB, p_app_font_tab));
}

#if WINDOWS

typedef struct FONTENUM_CALLBACK
{
    /*IN*/
    ARRAY_INIT_BLOCK array_init_block;
    HDC hdc;

    /*INOUT*/
    STATUS status;
}
FONTENUM_CALLBACK, * P_FONTENUM_CALLBACK;

_Check_return_
static int
fontmap_enum_fonts_add_font(
    _In_        const ENUMLOGFONTEX * const lpelf,
    _InoutRef_  P_FONTENUM_CALLBACK p)
{
    P_HOST_FONT_TAB p_host_font_tab;
    ARRAY_INDEX insert_before;
    STATUS status;

    trace_4(TRACE_APP_FONTS, TEXT("enum facename '%s' weight=%d italic=%d charset=%d"),
            lpelf->elfLogFont.lfFaceName, lpelf->elfLogFont.lfWeight, lpelf->elfLogFont.lfItalic, lpelf->elfLogFont.lfCharSet);

    if(CH_COMMERCIAL_AT == lpelf->elfLogFont.lfFaceName[0])
    {   /* why do we get these wacky entries??? */
        /* "The one with the @ prefix is known as a vertical version of the same font" */
        trace_4(TRACE_APP_FONTS, TEXT("enum facename '%s' weight=%d italic=%d charsets=%d NOT ADDED (@)"),
                lpelf->elfLogFont.lfFaceName, lpelf->elfLogFont.lfWeight, lpelf->elfLogFont.lfItalic, lpelf->elfLogFont.lfCharSet);
        return(1);
    }

    { /* Don't insert duplicate fonts (there may be multiple character sets per font). Do insert all style variants */
    BOOL hit;
    ARRAY_INDEX array_index = al_array_bfind(&lpelf->elfLogFont, &fontmap_table_host, HOST_FONT_TAB, fontmap_compare_host_logfont_bsearch, &hit);

    if(hit)
    {   /* hit existing entry */
        trace_4(TRACE_APP_FONTS, TEXT("enum facename '%s' weight=%d italic=%d charsets=%d NOT ADDED (hit)"),
                lpelf->elfLogFont.lfFaceName, lpelf->elfLogFont.lfWeight, lpelf->elfLogFont.lfItalic, lpelf->elfLogFont.lfCharSet);
        return(1);
    }

    insert_before = array_index; /* use bfind suggested index for insert_before */
    } /*block*/

    if(NULL == (p_host_font_tab = al_array_insert_before(&fontmap_table_host, HOST_FONT_TAB, 1, &p->array_init_block, &status, insert_before)))
    {
        p->status = status;
        return(0);
    }

    /* Copy the entire LOGFONT away to be used later */
    p_host_font_tab->logfont = lpelf->elfLogFont;

    return(1);
}

/* Called by Windows for all available character sets in each uniquely named font */

_Check_return_
extern int CALLBACK
fontmap_enum_fonts_callback(
    _In_        CONST LOGFONT *lplf,
    _In_        CONST TEXTMETRIC *lpntm,
    _In_        DWORD font_type,
    _In_        LPARAM lParam)
{
    CONST ENUMLOGFONTEXDV *lpelfe = (CONST ENUMLOGFONTEXDV *) lplf; /* logical-font data */
    CONST ENUMLOGFONTEX *lpelf = &lpelfe->elfEnumLogfontEx;
    CONST ENUMTEXTMETRIC *lpntme = (CONST ENUMTEXTMETRIC *) lpntm; /* physical-font data */

    if(font_type & TRUETYPE_FONTTYPE)
    {
        P_FONTENUM_CALLBACK p = (P_FONTENUM_CALLBACK) lParam;
        return(fontmap_enum_fonts_add_font(lpelf, p));
    }

    IGNOREPARM(lpntme);
    return(1);
}

/* Called once per family */

_Check_return_
extern int CALLBACK
fontmap_enum_faces_callback(
    _In_        CONST LOGFONT *lplf,
    _In_        CONST TEXTMETRIC *lpntm,
    _In_        DWORD font_type,
    _In_        LPARAM lParam)
{
    CONST ENUMLOGFONTEXDV *lpelfe = (CONST ENUMLOGFONTEXDV *) lplf; /* logical-font data */

    if(font_type & TRUETYPE_FONTTYPE)
    {
        P_FONTENUM_CALLBACK p = (P_FONTENUM_CALLBACK) lParam;
        LOGFONT logfont = lpelfe->elfEnumLogfontEx.elfLogFont; /* always use the LOGFONT name as it must be a valid one */
        return(EnumFontFamiliesEx(p->hdc, &logfont, fontmap_enum_fonts_callback, lParam, 0));
    }

    IGNOREPARM(lpntm);
    return(1);
}

_Check_return_
static FONTMAP_BITS
fontmap_bits_from_logfont(
    _In_        const LOGFONT * const p_logfont)
{
    FONTMAP_BITS fontmap_bits = FONTMAP_BASIC;

    /* Bold and Italic can be derived from the logfont info */
    if(p_logfont->lfWeight >= FW_SEMIBOLD)
        fontmap_bits = FONTMAP_BOLD;

    if(p_logfont->lfItalic != 0)
        fontmap_bits = (fontmap_bits == FONTMAP_BOLD) ? FONTMAP_BOLDITALIC : FONTMAP_ITALIC;

    return(fontmap_bits);
}

#endif /* WINDOWS */

#if RISCOS

static _kernel_oserror *
fontmap_list_riscos(
    char * a,
    int * count)
{
    _kernel_swi_regs rs;
    _kernel_oserror * p_kernel_oserror;

    rs.r[1] = (int) a;
    rs.r[2] = *count;
    rs.r[3] = -1;

    if(NULL == (p_kernel_oserror = _kernel_swi(/*Font_ListFonts*/ 0x040091, &rs, &rs)))
    {
        int i;

        *count = rs.r[2];

        /* try to ensure correct termination */
        i = 0;
        while(a[i] >= 32 && i <= 99)
            ++i;
        a[i] = CH_NULL;
    }
    else /* error return: probably some filing system error */
        *count = -1; /* signal end of list */

    return(p_kernel_oserror);
}

#endif /* RISCOS */

_Check_return_
static STATUS
fontmap_table_host_create(void)
{
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(HOST_FONT_TAB), TRUE);
    STATUS status = STATUS_OK;

    { /* knock out all previous mappings */
    ARRAY_INDEX app_index;

    for(app_index = 0; app_index < array_elements(&fontmap_table_app); ++app_index)
    {
        const P_APP_FONT_TAB p_app_font_tab = p_app_font_entry(app_index);
        FONTMAP_BITS fontmap_bits;

        for(fontmap_bits = FONTMAP_BASIC; fontmap_bits < FONTMAP_N_BITS; ENUM_INCR(FONTMAP_BITS, fontmap_bits))
        {
            p_app_font_tab->host_font[fontmap_bits].host_array_index = -1;
        }
    }
    } /*block*/

    fontmap_table_host_dispose();

    { /* ask host for all the fonts it knows about */
#if RISCOS
    int host_font_count = 0;

    for(;;)
    {
        P_HOST_FONT_TAB p_host_font_tab;
        TCHARZ fullname[LF_FULLFACESIZE];

        if(WrapOsErrorReporting(fontmap_list_riscos(fullname, &host_font_count)) || (host_font_count < 0))
            break;

        /* SKS for 1.30 - lose stupid font from list */
        if(0 == tstricmp(TEXT("WIMPSymbol"), fullname))
            continue;

        if(NULL == (p_host_font_tab = al_array_extend_by(&fontmap_table_host, HOST_FONT_TAB, 1, &array_init_block, &status)))
            break;

        if(status_fail(status = tstr_set(&p_host_font_tab->fullname, fullname)))
            break;
    }

    al_array_qsort(&fontmap_table_host, fontmap_compare_host_fullname_qsort);
#elif WINDOWS
    FONTENUM_CALLBACK fontenum_callback;
    LOGFONT logfont;
    zero_struct(logfont);

    logfont.lfCharSet = ANSI_CHARSET; /* Enumerates all font faces in ANSI_CHARSET */

    fontenum_callback.array_init_block = array_init_block;
    fontenum_callback.hdc = host_get_hic_format_pixits(); /* always ask printer driver what fonts it supports as we use printer driver for formatting */
    fontenum_callback.status = STATUS_OK;

    (void) EnumFontFamiliesEx(fontenum_callback.hdc, &logfont, fontmap_enum_faces_callback, (LPARAM) &fontenum_callback, 0);

    status = fontenum_callback.status;

    if(status_ok(status))
    {   /* Add in the symbol fonts */
        logfont.lfCharSet = SYMBOL_CHARSET; /* Enumerates all font faces in SYMBOL_CHARSET */

        (void) EnumFontFamiliesEx(fontenum_callback.hdc, &logfont, fontmap_enum_fonts_callback, (LPARAM) &fontenum_callback, 0);

        status = fontenum_callback.status;
    }

    al_array_check_sorted(&fontmap_table_host, fontmap_compare_host_logfont_qsort);
#endif /* OS */
    } /*block*/

    status_return(status);

    status_return(fontmap_lookup_app_font_entries());

    return(fontmap_invent_app_font_entries_for_host());
}

_Check_return_
static STATUS
fontmap_invent_app_font_entry_for_host(
    _InVal_     ARRAY_INDEX host_index)
{
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(APP_FONT_TAB), 1);
    P_HOST_FONT_TAB p_host_font_tab = p_host_entry(host_index);
    TCHARZ tstr_app_font_name[BUF_FACENAME_LEN];
    FONTMAP_BITS fontmap_bits;
    P_APP_FONT_TAB p_app_font_tab;
    ARRAY_INDEX insert_before;
    STATUS status = STATUS_OK;

    if(0 != p_host_font_tab->uses)
    {
#if RISCOS
        trace_2(TRACE_APP_FONTS, TEXT("host font '%s' already used (%d times)"), report_tstr(p_host_font_tab->fullname), p_host_font_tab->uses);
#elif WINDOWS
        trace_4(TRACE_APP_FONTS, TEXT("host font '%s' weight:%d italic:%d already used (%d times)"), report_tstr(p_host_font_tab->logfont.lfFaceName), p_host_font_tab->logfont.lfWeight, p_host_font_tab->logfont.lfItalic, p_host_font_tab->uses);
#endif
        return(status);
    }

    /* work out what we would call this font for Fireworkz if no mapping from config exists */
    fontmap_bits = fontmap_invent_app_font_name(tstr_app_font_name, elemof32(tstr_app_font_name), p_host_font_tab);

    {
    BOOL hit;
    ARRAY_INDEX app_array_index = al_array_bfind(tstr_app_font_name, &fontmap_table_app, APP_FONT_TAB, fontmap_compare_app_name_bsearch, &hit);

    if(hit)
    {
        /* an (automatically created) entry already exists, so repopulate it appropriately (as below) */
        p_app_font_tab = array_ptr(&fontmap_table_app, APP_FONT_TAB, app_array_index);

        if(p_app_font_tab->host_font[fontmap_bits].host_array_index < 0)
        {
#if RISCOS
            trace_3(TRACE_APP_FONTS, TEXT("entry set for host font '%s' in Fireworkz '%s' fontmap_bits=%d"), report_tstr(p_host_font_tab->fullname), report_tstr(p_app_font_tab->tstr_app_font_name), fontmap_bits);
#elif WINDOWS
            trace_5(TRACE_APP_FONTS, TEXT("entry set for host font '%s' weight:%d italic:%d in Fireworkz '%s' fontmap_bits=%d"), report_tstr(p_host_font_tab->logfont.lfFaceName), p_host_font_tab->logfont.lfWeight, p_host_font_tab->logfont.lfItalic, report_tstr(p_app_font_tab->tstr_app_font_name), fontmap_bits);
#endif

            p_host_font_tab->uses++;

            p_app_font_tab->host_font[fontmap_bits].host_array_index = host_index;
        }
#if RISCOS && TRACE_ALLOWED
        else
            trace_3(TRACE_APP_FONTS, TEXT("entry NOT set for host font '%s' in Fireworkz '%s' fontmap_bits=%d"), report_tstr(p_host_font_tab->fullname), report_tstr(p_app_font_tab->tstr_app_font_name), fontmap_bits);
#elif WINDOWS && TRACE_ALLOWED
        else
            trace_5(TRACE_APP_FONTS, TEXT("entry NOT set for host font '%s' weight:%d italic:%d in Fireworkz '%s' fontmap_bits=%d"), report_tstr(p_host_font_tab->logfont.lfFaceName), p_host_font_tab->logfont.lfWeight, p_host_font_tab->logfont.lfItalic, report_tstr(p_app_font_tab->tstr_app_font_name), fontmap_bits);
#endif

        return(STATUS_OK);
    }

    insert_before = app_array_index; /* use bfind suggested index for insert_before */
    } /*block*/

    if(NULL == (p_app_font_tab = al_array_insert_before(&fontmap_table_app, APP_FONT_TAB, 1, &array_init_block, &status, insert_before)))
        return(status);

    {
    U8 rtf_class = RTF_CLASS_ROMAN;

#if WINDOWS
    switch(p_host_font_tab->logfont.lfPitchAndFamily & 0xF0)
    {
    default: /*DROPTHRU*/
    case FF_ROMAN:      rtf_class = RTF_CLASS_ROMAN;    break;
    case FF_SWISS:      rtf_class = RTF_CLASS_SWISS;    break;
    case FF_MODERN:     rtf_class = RTF_CLASS_MODERN;   break;
    case FF_SCRIPT:     rtf_class = RTF_CLASS_SCRIPT;   break;
    case FF_DECORATIVE: rtf_class = RTF_CLASS_DECOR;    break;
    }
#endif

    p_app_font_tab->rtf_class = rtf_class;
    } /*block*/

    if(status_ok(status = alloc_block_tstr_set(&p_app_font_tab->tstr_app_font_name, tstr_app_font_name, &global_string_alloc_block)))
        status = alloc_block_tstr_set(&p_app_font_tab->tstr_host_base_name, p_app_font_tab->tstr_app_font_name, &global_string_alloc_block);

    {
    FONTMAP_BITS fontmap_bits_loop;
    for(fontmap_bits_loop = FONTMAP_BASIC; fontmap_bits_loop < FONTMAP_N_BITS; ENUM_INCR(FONTMAP_BITS, fontmap_bits_loop))
        p_app_font_tab->host_font[fontmap_bits_loop].host_array_index = -1;
    } /*block*/

    if(status_ok(status))
    {
        p_host_font_tab->uses++;

        /* populate precisely rather than for subsequent remap speed (consider RISC OS auto-creation for font BI variants) */
        p_app_font_tab->host_font[fontmap_bits].host_array_index = host_index;

#if RISCOS
        trace_3(TRACE_APP_FONTS, TEXT("host font '%s' created new Fireworkz entry '%s' fontmap_bits=%d"), report_tstr(p_host_font_tab->fullname), report_tstr(p_app_font_tab->tstr_app_font_name), fontmap_bits);
#elif WINDOWS
        trace_5(TRACE_APP_FONTS, TEXT("host font '%s' weight:%d italic:%d created new Fireworkz entry '%s' fontmap_bits=%d"), report_tstr(p_host_font_tab->logfont.lfFaceName), p_host_font_tab->logfont.lfWeight, p_host_font_tab->logfont.lfItalic, report_tstr(p_app_font_tab->tstr_app_font_name), fontmap_bits);
#endif
    }

    return(status);
}

_Check_return_
static STATUS
fontmap_invent_app_font_entries_for_host(void)
{
    ARRAY_INDEX host_index;
    STATUS status = STATUS_OK;

    for(host_index = 0; host_index < array_elements(&fontmap_table_host); ++host_index)
    {
        status_break(status = fontmap_invent_app_font_entry_for_host(host_index));
    }

    return(status);
}

static void
fontmap_lookup_app_font_entry(
    _InVal_     ARRAY_INDEX app_index)
{
    const P_APP_FONT_TAB p_app_font_tab = p_app_font_entry(app_index);
    FONTMAP_BITS fontmap_bits;

    if(!p_app_font_tab->defined_by_config)
    {   /* auto-mapped ones are handled elsewhere after dealing with config-defined ones */
        return;
    }

    trace_2(TRACE_APP_FONTS, TEXT("lookup entries for Fireworkz config name '%s' with host base name '%s'"), report_tstr(p_app_font_tab->tstr_app_font_name), report_tstr(p_app_font_tab->tstr_host_base_name));

#if RISCOS

    for(fontmap_bits = FONTMAP_BASIC; fontmap_bits < FONTMAP_N_BITS; ENUM_INCR(FONTMAP_BITS, fontmap_bits))
    {
        P_HOST_FONT_TAB p_host_font_tab;
        PCTSTR tstr = p_app_font_tab->host_font[fontmap_bits].tstr_config;

        if(NULL != tstr)
        {
            p_host_font_tab = al_array_bsearch(tstr, &fontmap_table_host, HOST_FONT_TAB, fontmap_compare_host_fullname_bsearch);
            trace_3(TRACE_APP_FONTS, TEXT("- lookup config Fireworkz name '%s' host %s for fontmap_bits=%d"), report_tstr(tstr), report_boolstring(!IS_P_DATA_NONE(p_host_font_tab)), fontmap_bits);
        }
        else
        {   /* SKS added 10.11.02 to allow use of base name as simple alias (if non-standard, user will still have to specify required variant) */
            /* try appending standard RISC OS suffixes to base name */
            TCHARZ fullname_buffer[BUF_FACENAME_LEN];
            tstr = p_app_font_tab->tstr_host_base_name;
            tstr_xstrkpy(fullname_buffer, elemof32(fullname_buffer), p_app_font_tab->tstr_host_base_name);

            switch(fontmap_bits)
            {
            default:
            case FONTMAP_BASIC:
                break;

            case FONTMAP_BOLD:
                tstr_xstrkat(fullname_buffer, elemof32(fullname_buffer), TEXT(".Bold"));
                break;

            case FONTMAP_ITALIC:
                tstr_xstrkat(fullname_buffer, elemof32(fullname_buffer), TEXT(".Italic"));
                break;

            case FONTMAP_BOLDITALIC:
                tstr_xstrkat(fullname_buffer, elemof32(fullname_buffer), TEXT(".Bold.Italic"));
                break;
            }

            p_host_font_tab = al_array_bsearch(tstr, &fontmap_table_host, HOST_FONT_TAB, fontmap_compare_host_fullname_bsearch);
            trace_3(TRACE_APP_FONTS, TEXT("- lookup config host base name(+) '%s' host %s for fontmap_bits=%d"), tstr, report_boolstring(!IS_P_DATA_NONE(p_host_font_tab)), fontmap_bits);
        }

        if(!IS_P_DATA_NONE(p_host_font_tab))
        {
            p_host_font_tab->uses++;

            p_app_font_tab->host_font[fontmap_bits].host_array_index = array_indexof_element(&fontmap_table_host, HOST_FONT_TAB, p_host_font_tab);
        }
    }

#elif WINDOWS

    { /* SKS now only allow use of base name as simple alias (must loop over all styles) */
    PCTSTR tstr = p_app_font_tab->tstr_host_base_name;
    P_HOST_FONT_TAB p_host_font_tab = al_array_lsearch(tstr, &fontmap_table_host, HOST_FONT_TAB, fontmap_compare_host_logfont_facename_lsearch);

    while(!IS_P_DATA_NONE(p_host_font_tab))
    {
        fontmap_bits = fontmap_bits_from_logfont(&p_host_font_tab->logfont);

        trace_3(TRACE_APP_FONTS, TEXT("- lookup config host base name '%s' host %s for fontmap_bits=%d"), tstr, report_boolstring(!IS_P_DATA_NONE(p_host_font_tab)), fontmap_bits);

        p_host_font_tab->uses++;

        p_app_font_tab->host_font[fontmap_bits].host_array_index = array_indexof_element(&fontmap_table_host, HOST_FONT_TAB, p_host_font_tab);

        ++p_host_font_tab;

        if(0 != fontmap_compare_host_logfont_facename_lsearch(tstr, p_host_font_tab))
            break;
    }
    } /*block*/

#endif /* OS */
}

_Check_return_
static STATUS
fontmap_lookup_app_font_entries(void)
{
    ARRAY_INDEX app_index;

    if(0 == array_elements(&fontmap_table_host))
        return(create_error(ERR_NO_HOST_FONTS));

    /* for all config-defined fonts, perform lookup */
    for(app_index = 0; app_index < array_elements(&fontmap_table_app); ++app_index)
    {
        fontmap_lookup_app_font_entry(app_index);
    }

    return(STATUS_OK);
}

#define FONTMAP_BIT_BS 4 /* 1<<2 */

#define FONTMAP_BIT_B0 FONTMAP_BIT_BS | 0
#define FONTMAP_BIT_B1 FONTMAP_BIT_BS | FONTMAP_BOLD

#if RISCOS

typedef struct STRIPPER_ENTRY
{
    UINT bit;
    PCTSTR tstr;
}
STRIPPER_ENTRY; typedef const STRIPPER_ENTRY * PC_STRIPPER_ENTRY;

/* NB Sassoon.Primary needs to be left as such 'cos there's Sassoon.Primary.Bold */

static const STRIPPER_ENTRY
stripper[] =
{
    { FONTMAP_ITALIC, TEXT(".") TEXT("Italic")   },
    { FONTMAP_ITALIC, TEXT(".") TEXT("Oblique")  },
    { FONTMAP_ITALIC, TEXT(".") TEXT("Slanted")  },

    { FONTMAP_BIT_B1, TEXT(".") TEXT("Bold")     },

    { FONTMAP_BIT_B0, TEXT(".") TEXT("Medium")   },
    { FONTMAP_BIT_B0, TEXT(".") TEXT("Regular")  },
    { FONTMAP_BIT_B0, TEXT(".") TEXT("Standard") }
};

#endif /* RISCOS - no longer used on Windows */

_Check_return_
static FONTMAP_BITS
fontmap_invent_app_font_name(
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf_app_font_name,
    _InVal_     U32 elemof_buffer,
    _In_        PC_HOST_FONT_TAB p_host_font_tab)
{
    PCTSTR tstr_host_font_name;
    FONTMAP_BITS fontmap_bits;

#if RISCOS

    U32 stripper_index;
    UINT bits = 0;

    tstr_host_font_name = p_host_font_tab->fullname;

    tstr_xstrkpy(tstr_buf_app_font_name, elemof_buffer, tstr_host_font_name);

    for(stripper_index = 0; stripper_index < elemof32(stripper); ++stripper_index)
    {
        PC_STRIPPER_ENTRY p_stripper_entry = &stripper[stripper_index];
        PTSTR tstr;

        if(0 != (bits & p_stripper_entry->bit))
            /* already stripped one of this group */
            continue;

        if(NULL != (tstr = strstr(tstr_buf_app_font_name, p_stripper_entry->tstr)))
            /* SKS 30dec93 notes this is case sensitive ha ha up you Darwin.medium */
        {
            U32 len = tstrlen32(p_stripper_entry->tstr);

            if(!tstr[len])
                *tstr = CH_NULL; /* stripping off end */
            else if(tstr[len] == CH_FULL_STOP)
                memmove32(tstr, tstr + len, sizeof32(*tstr) * (tstrlen32(tstr + len) + 1)); /* move end segment down */
            else
                continue;

            bits |= p_stripper_entry->bit;
        }
    }

#ifdef UNDEF
    { /* at the end of the day, we must panic and rename the font if it now exactly matches a Fireworkz configured name eg Courier on Windows */
    ARRAY_INDEX app_index;

    for(app_index = 0; app_index < array_elements(&fontmap_table_app); ++app_index)
    {
        const P_APP_FONT_TAB p_app_font_tab = p_app_font_entry(app_index);

        if(p_app_font_tab->defined_by_config)
            if(0 == fontmap_compare_app_name_bsearch(tstr_buf_app_font_name, p_app_font_tab))
            {
                tstr_xstrkat(tstr_buf_app_font_name, elemof_buffer, "\xA0" /*"$$$"*/);
                break;
            }
    }
    } /*block*/
#endif /* UNDEF*/

    fontmap_bits = (FONTMAP_BITS) (bits & (FONTMAP_BOLD | FONTMAP_ITALIC));

#elif WINDOWS

    /* Always use the apparently sensible LOGFONT family facename for the Fireworkz name */
    tstr_host_font_name = p_host_font_tab->logfont.lfFaceName;

    tstr_xstrkpy(tstr_buf_app_font_name, elemof_buffer, tstr_host_font_name);

    fontmap_bits = fontmap_bits_from_logfont(&p_host_font_tab->logfont);

#endif /* OS */

#ifdef UNDEF
    reportf(TEXT("fontmap_invent_app_font_name(host_font_name=%s) => app_font_name='%s' %c %c"),
            tstr_host_font_name, tstr_buf_app_font_name,
            fontmap_bits & FONTMAP_BOLD   ? 'B' : CH_SPACE,
            fontmap_bits & FONTMAP_ITALIC ? 'I' : CH_SPACE);
#endif

    return(fontmap_bits); /* 0..3 - Regular, Bold, Italic, BoldItalic */
}

/******************************************************************************
*
* Attempts to find a font of the same RTF class as that given
*
******************************************************************************/

_Check_return_
static FONTMAP_BITS
fontmap_remap_using_rtf_class(
    _InoutRef_  P_P_APP_FONT_TAB p_p_font_tab,
    _InVal_     FONTMAP_BITS fontmap_bits,
    _InVal_     BOOL use_host_riscos,
    _InVal_     BOOL find_master)
{
    U8 rtf_class = (*p_p_font_tab)->rtf_class;
    ARRAY_INDEX scan_index = array_elements(&fontmap_table_app);

    while(--scan_index >= 0)
    {
        const P_APP_FONT_TAB p_app_font_tab = p_app_font_entry(scan_index);

        if(p_app_font_tab->rtf_class != rtf_class)
            continue;

        if(*p_p_font_tab == p_app_font_tab) /* obviously never return same entry as that we're attempting to replace */
            continue;

        if(p_app_font_tab->rtf_master || !find_master)
        {
            FONTMAP_BITS try_fontmap_bits;
            
#if RISCOS
            IGNOREPARM_InVal_(use_host_riscos);
            assert(!use_host_riscos);
#else
            if(use_host_riscos)
                try_fontmap_bits = fontmap_remap_within_font_riscos(p_app_font_tab, fontmap_bits);
            else
#endif
                try_fontmap_bits = fontmap_remap_within_font(p_app_font_tab, fontmap_bits);

            if(try_fontmap_bits >= 0)
            {
                *p_p_font_tab = p_app_font_tab;
                return(try_fontmap_bits);
            }
        }
    }

    return(FONTMAP_INVALID);
}

_Check_return_
static FONTMAP_BITS
fontmap_remap_within_font(
    _In_        PC_APP_FONT_TAB p_app_font_tab,
    _InVal_     FONTMAP_BITS fontmap_bits)
{
    FONTMAP_BITS try_fontmap_bits = fontmap_bits;

    if(p_app_font_tab->host_font[try_fontmap_bits].host_array_index >= 0)
    {
        trace_2(TRACE_APP_FONTS, TEXT("Fireworkz font '%s' host OK for fontmap_bits=%d"), report_tstr(p_app_font_tab->tstr_app_font_name), try_fontmap_bits);
        return(try_fontmap_bits);
    }

    /* try a different permutation (toggle Bold) */
    switch(fontmap_bits)
    {
    default:
        try_fontmap_bits = FONTMAP_BOLD;
        break;

    case FONTMAP_BOLD:
        try_fontmap_bits = FONTMAP_BASIC;
        break;

    case FONTMAP_ITALIC:
        try_fontmap_bits = FONTMAP_BOLDITALIC;
        break;

    case FONTMAP_BOLDITALIC:
        try_fontmap_bits = FONTMAP_ITALIC;
        break;
    }

    if(p_app_font_tab->host_font[try_fontmap_bits].host_array_index >= 0)
    {
        trace_3(TRACE_APP_FONTS, TEXT("Fireworkz font '%s' host OK for fontmap_bits=%d when B-toggled to %d"), report_tstr(p_app_font_tab->tstr_app_font_name), fontmap_bits, try_fontmap_bits);
        return(try_fontmap_bits);
    }

    /* try a yet different permutation (toggle Italic) */
    switch(fontmap_bits)
    {
    default:
        try_fontmap_bits = FONTMAP_ITALIC;
        break;

    case FONTMAP_BOLD:
        try_fontmap_bits = FONTMAP_BOLDITALIC;
        break;

    case FONTMAP_ITALIC:
        try_fontmap_bits = FONTMAP_BASIC;
        break;

    case FONTMAP_BOLDITALIC:
        try_fontmap_bits = FONTMAP_BOLD;
        break;
    }

    if(p_app_font_tab->host_font[try_fontmap_bits].host_array_index >= 0)
    {
        trace_3(TRACE_APP_FONTS, TEXT("Fireworkz font '%s' host OK for fontmap_bits=%d when I-toggled to %d"), report_tstr(p_app_font_tab->tstr_app_font_name), fontmap_bits, try_fontmap_bits);
        return(try_fontmap_bits);
    }

    /* try a yet different permutation (toggle Bold and Italic) */
    switch(fontmap_bits)
    {
    default:
        try_fontmap_bits = FONTMAP_BOLDITALIC;
        break;

    case FONTMAP_BOLD:
        try_fontmap_bits = FONTMAP_ITALIC;
        break;

    case FONTMAP_ITALIC:
        try_fontmap_bits = FONTMAP_BOLD;
        break;

    case FONTMAP_BOLDITALIC:
        try_fontmap_bits = FONTMAP_BASIC;
        break;
    }

    if(p_app_font_tab->host_font[try_fontmap_bits].host_array_index >= 0)
    {
        trace_3(TRACE_APP_FONTS, TEXT("Fireworkz font '%s' host OK for fontmap_bits=%d when BI-toggled to %d"), report_tstr(p_app_font_tab->tstr_app_font_name), fontmap_bits, try_fontmap_bits);
        return(try_fontmap_bits);
    }

    trace_2(TRACE_APP_FONTS, TEXT("Fireworkz font '%s' host NOT OK for fontmap_bits=%d with any mapping"), report_tstr(p_app_font_tab->tstr_app_font_name), fontmap_bits);
    return(FONTMAP_INVALID);
}

#if !RISCOS

/* a simple variant of the above function, but looks in riscos_font[] and only for configured fonts */

_Check_return_
static FONTMAP_BITS
fontmap_remap_within_font_riscos(
    _In_        PC_APP_FONT_TAB p_app_font_tab,
    _InVal_     FONTMAP_BITS fontmap_bits)
{
    FONTMAP_BITS try_fontmap_bits = fontmap_bits;

    if(NULL != p_app_font_tab->riscos_font[try_fontmap_bits].tstr_config)
        return(try_fontmap_bits);

    /* try a different permutation (toggle Bold) */
    switch(fontmap_bits)
    {
    default:
        try_fontmap_bits = FONTMAP_BOLD;
        break;

    case FONTMAP_BOLD:
        try_fontmap_bits = FONTMAP_BASIC;
        break;

    case FONTMAP_ITALIC:
        try_fontmap_bits = FONTMAP_BOLDITALIC;
        break;

    case FONTMAP_BOLDITALIC:
        try_fontmap_bits = FONTMAP_ITALIC;
        break;
    }

    if(NULL != p_app_font_tab->riscos_font[try_fontmap_bits].tstr_config)
        return(try_fontmap_bits);

    /* try a yet different permutation (toggle Italic) */
    switch(fontmap_bits)
    {
    default:
        try_fontmap_bits = FONTMAP_ITALIC;
        break;

    case FONTMAP_BOLD:
        try_fontmap_bits = FONTMAP_BOLDITALIC;
        break;

    case FONTMAP_ITALIC:
        try_fontmap_bits = FONTMAP_BASIC;
        break;

    case FONTMAP_BOLDITALIC:
        try_fontmap_bits = FONTMAP_BOLD;
        break;
    }

    if(NULL != p_app_font_tab->riscos_font[try_fontmap_bits].tstr_config)
        return(try_fontmap_bits);

    /* try a yet different permutation (toggle Bold and Italic) */
    switch(fontmap_bits)
    {
    default:
        try_fontmap_bits = FONTMAP_BOLDITALIC;
        break;

    case FONTMAP_BOLD:
        try_fontmap_bits = FONTMAP_ITALIC;
        break;

    case FONTMAP_ITALIC:
        try_fontmap_bits = FONTMAP_BOLD;
        break;

    case FONTMAP_BOLDITALIC:
        try_fontmap_bits = FONTMAP_BASIC;
        break;
    }

    if(NULL != p_app_font_tab->riscos_font[try_fontmap_bits].tstr_config)
        return(try_fontmap_bits);

    return(FONTMAP_INVALID);
}

#endif /* !RISCOS */

/******************************************************************************
*
* Maps a host font name (base) to a Fireworkz font spec
*
******************************************************************************/

_Check_return_
extern STATUS
fontmap_font_spec_from_host_base_name(
    _OutRef_    P_FONT_SPEC p_font_spec,
    _In_z_      PCTSTR p_host_base_name)
{
    const ARRAY_INDEX n = array_elements(&fontmap_table_app);
    ARRAY_INDEX i;
    PC_APP_FONT_TAB p_app_font_tab = array_rangec(&fontmap_table_app, APP_FONT_TAB, 0, n);

    zero_struct_ptr(p_font_spec);

    for(i = 0; i < n; ++i, ++p_app_font_tab)
        if(0 == tstrcmp(p_app_font_tab->tstr_host_base_name, p_host_base_name))
            return(font_spec_name_alloc(p_font_spec, p_app_font_tab->tstr_app_font_name));

    return(/*create_error*/(ERR_HOST_FONT_NOT_FOUND));
}

/******************************************************************************
*
* returns a Fireworkz spec from the index into the Fireworkz table
*
******************************************************************************/

_Check_return_
extern STATUS
fontmap_font_spec_from_app_index(
    _OutRef_    P_FONT_SPEC p_font_spec,
    _InVal_     ARRAY_INDEX app_index)
{
    P_APP_FONT_TAB p_app_font_tab;

    zero_struct_ptr(p_font_spec);

    if(!array_index_valid(&fontmap_table_app, app_index))
    {
        assert(app_index >= 0);
        assert(app_index == array_elements(&fontmap_table_app)); /* SKS: == case so you can enum from 0 till error return without whinge */
        return(/*create_error*/(ERR_TYPE5_FONT_NOT_FOUND));
    }

    p_app_font_tab = p_app_font_entry(app_index);

    return(font_spec_name_alloc(p_font_spec, p_app_font_tab->tstr_app_font_name));
}

T5_CMD_PROTO(extern, t5_cmd_fontmap)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 12);
    PCTSTR tstr_app_font_name = p_args[0].val.tstr;
    PCTSTR tstr_rtf_class_name = p_args[1].val.tstr;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(APP_FONT_TAB), 1);
    P_APP_FONT_TAB p_app_font_tab;
    FONTMAP_BITS fontmap_bits;
    ARRAY_INDEX insert_before;
    STATUS status = STATUS_OK;

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    trace_1(TRACE_APP_FONTS, TEXT("config Fireworkz font name '%s'"), tstr_app_font_name);

    { /* scan Fireworkz table to find where to insert this one */
    BOOL hit;
    ARRAY_INDEX array_index = al_array_bfind(tstr_app_font_name, &fontmap_table_app, APP_FONT_TAB, fontmap_compare_app_name_bsearch, &hit);

    if(hit)
        return(STATUS_OK); /* can't overwrite existing entries */

    insert_before = array_index; /* use bfind suggested index for insert_before */
    } /*block*/

    if(NULL == (p_app_font_tab = al_array_insert_before(&fontmap_table_app, APP_FONT_TAB, 1, &array_init_block, &status, insert_before)))
        return(status);

    for(fontmap_bits = FONTMAP_BASIC; fontmap_bits < FONTMAP_N_BITS; ENUM_INCR(FONTMAP_BITS, fontmap_bits))
        p_app_font_tab->host_font[fontmap_bits].host_array_index = -1; /* not yet mapped to a real host font */

    p_app_font_tab->rtf_class = RTF_CLASS_ROMAN;
    p_app_font_tab->rtf_master = 0;

    status = alloc_block_tstr_set(&p_app_font_tab->tstr_app_font_name, tstr_app_font_name, &global_string_alloc_block);

    if(status_ok(status))
    {
        U8 rtf_class;

        for(rtf_class = 0; rtf_class < elemof32(rtf_class_name); ++rtf_class)
        {
            if(0 == tstrcmp(rtf_class_name[rtf_class], tstr_rtf_class_name))
            {
                p_app_font_tab->rtf_class = rtf_class;
                if(arg_is_present(p_args, 2) && (p_args[2].val.fBool))
                    p_app_font_tab->rtf_master = 1;
                break;
            }
        }
    }

    if(status_ok(status))
        status = alloc_block_tstr_set(&p_app_font_tab->tstr_host_base_name, p_args[3].val.tstr, &global_string_alloc_block);

    trace_3(TRACE_APP_FONTS, TEXT("config host base name '%s', rtf_class=%s, rtf_master=%d"), p_args[3].val.tstr, rtf_class_name[p_app_font_tab->rtf_class], p_app_font_tab->rtf_master);

    for(fontmap_bits = FONTMAP_BASIC; fontmap_bits < FONTMAP_N_BITS; ENUM_INCR(FONTMAP_BITS, fontmap_bits))
    {
        U32 arg_idx = 3 + 1 + fontmap_bits;

        if(arg_is_present(p_args, arg_idx))
        {
            trace_3(TRACE_APP_FONTS, TEXT("arg[%d] present for fontmap_bits=") U32_TFMT TEXT(", full_name='%s'"), arg_idx, fontmap_bits, p_args[arg_idx].val.tstr);
            status_break(status = alloc_block_tstr_set(&p_app_font_tab->host_font[fontmap_bits].tstr_config, p_args[arg_idx].val.tstr, &global_string_alloc_block));
        }
    }

#if !RISCOS

    for(fontmap_bits = FONTMAP_BASIC; fontmap_bits < FONTMAP_N_BITS; ENUM_INCR(FONTMAP_BITS, fontmap_bits))
    {
        U32 arg_idx = 4 + 3 + 1 + fontmap_bits;

        if(arg_is_present(p_args, arg_idx))
        {
            trace_3(TRACE_APP_FONTS, TEXT("arg[%d] present for fontmap_bits=") U32_TFMT TEXT(", riscos_name='%s'"), arg_idx, fontmap_bits, p_args[arg_idx].val.tstr);
            status_break(status = alloc_block_tstr_set(&p_app_font_tab->riscos_font[fontmap_bits].tstr_config, p_args[arg_idx].val.tstr, &global_string_alloc_block));
        }
    }

#endif /* OS */

    /* don't yet know whether host font is present */

    p_app_font_tab->defined_by_config = TRUE;

    al_array_check_sorted(&fontmap_table_app, fontmap_compare_app_name_qsort);

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_fontmap_end)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    return(fontmap_table_host_create());
}

_Check_return_
extern PCTSTR
fontmap_remap(
    _In_z_      PCTSTR tstr_app_font_name)
{
    ARRAY_INDEX remap_index;

    /* should only be a tiny number of fixes - do low overhead linear search */
    for(remap_index = 0; remap_index < array_elements(&fontmap_table_remap); ++remap_index)
    {
        PC_FONTMAP_REMAP_ENTRY p_fontmap_remap_entry = array_ptr(&fontmap_table_remap, FONTMAP_REMAP_ENTRY, remap_index);

        if(0 == tstrcmp(p_fontmap_remap_entry->tstr_app_font_name_old, tstr_app_font_name))
        {
            trace_2(TRACE_APP_FONTS, TEXT("FontmapRemap Fireworkz font name '%s' from '%s'"), p_fontmap_remap_entry->tstr_app_font_name, tstr_app_font_name);
            return(p_fontmap_remap_entry->tstr_app_font_name);
        }
    }

    return(tstr_app_font_name);
}

T5_CMD_PROTO(extern, t5_cmd_fontmap_remap)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    PCTSTR tstr_app_font_name = p_args[0].val.tstr;
    PCTSTR tstr_app_font_name_old = p_args[1].val.tstr;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(FONTMAP_REMAP_ENTRY), 1);
    P_FONTMAP_REMAP_ENTRY p_fontmap_remap_entry;
    ARRAY_INDEX insert_before;
    STATUS status = STATUS_OK;

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    trace_2(TRACE_APP_FONTS, TEXT("config FontmapRemap Fireworkz font name '%s' from '%s'"), tstr_app_font_name, tstr_app_font_name_old);

    insert_before = 0; /* no bfind suggested index for insert_before (see fontmap_remap()) */

    if(NULL == (p_fontmap_remap_entry = al_array_insert_before(&fontmap_table_remap, FONTMAP_REMAP_ENTRY, 1, &array_init_block, &status, insert_before)))
        return(status);

    status = alloc_block_tstr_set(&p_fontmap_remap_entry->tstr_app_font_name, tstr_app_font_name, &global_string_alloc_block);

    if(status_ok(status))
        status = alloc_block_tstr_set(&p_fontmap_remap_entry->tstr_app_font_name_old, tstr_app_font_name_old, &global_string_alloc_block);

    return(status);
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_fonty);

_Check_return_
static STATUS
fonty_msg_exit2(void)
{
    fonty_cache_trash_all();

#if CHECKING
    fontmap_table_remap_dispose();
    fontmap_table_host_dispose();
    fontmap_table_app_dispose();
#endif /* CHECKING */
    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_fonty_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__EXIT2:
        return(fonty_msg_exit2());

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_fonty)
{
    IGNOREPARM_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_fonty_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_FONTS_RECACHE:
        return(fontmap_table_host_create());

    default:
        return(STATUS_OK);
    }
}

/* end of fonty.c */
