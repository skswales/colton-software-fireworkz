/* gr_numcv.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Numeric conversion */

#include "common/gflags.h"

#define EXPOSE_RISCOS_FONT 1
#define EXPOSE_RISCOS_SWIS 1

#include "ob_chart/ob_chart.h"

/******************************************************************************
*
* truncate a fonty string to a given width in millipoints
*
* returns the new truncated width
*
******************************************************************************/

_Check_return_
extern GR_MILLIPOINT
gr_host_font_string_truncate(
    _HfontRef_opt_ HOST_FONT host_font,
    _Inout_z_   PC_USTR ustr /*poked to truncate*/,
    _InVal_     GR_MILLIPOINT swidth_mp_truncate)
{
    GR_MILLIPOINT swidth_mp = swidth_mp_truncate;

    if(HOST_FONT_NONE != host_font)
    {
#if RISCOS
        _kernel_swi_regs rs;
        _kernel_oserror * p_kernel_oserror;

#if 1
        rs.r[0] = host_font;
        rs.r[1] = (int) ustr;
        rs.r[2] = FONT_SCANSTRING_USE_HANDLE /*r0*/;
        rs.r[3] = swidth_mp_truncate;
        rs.r[4] = INT_MAX;

        if(NULL == (p_kernel_oserror = WrapOsErrorChecking(_kernel_swi(Font_ScanString, &rs, &rs))))
        {
            swidth_mp = rs.r[3]; /*x*/
reportf(TEXT("gr_host_font_string_truncate(x:%u,len:%u,%s) returns x:%u,len:%u"), swidth_mp_truncate, ustrlen32(ustr), report_ustr(ustr), swidth_mp, PtrDiffBytesU32(rs.r[1], ustr));
            PtrPutByte(rs.r[1] /*term_ptr*/, CH_NULL);
#else
        /* do initial font change economically */
        if(NULL != (p_kernel_oserror = font_SetFont((font) f)))
        {
            trace_1(TRACE_RISCOS_HOST, TEXT("gr_host_font_string_truncate: setfont error %s"), p_kernel_oserror->errmess);
            return(p_kernel_oserror);
        }

        rs.r[1] = (int) ustr;
        rs.r[2] = swidth_mp_truncate;
        rs.r[3] = INT_MAX;
        rs.r[4] = -1;
        rs.r[5] = INT_MAX;

        if(NULL == (p_kernel_oserror = WrapOsErrorChecking(_kernel_swi(Font_StringWidth, &rs, &rs))))
        {
            swidth_mp = rs.r[2]; /*x*/
//reportf(TEXT("gr_host_font_string_truncate(x:%u,len:%u,%s) returns x:%u,len:%u"), swidth_mp_truncate, ustrlen32(ustr), report_ustr(ustr), swidth_mp, rs.r[5]);
            PtrPutByteOff(str, rs.r[5] /*term_idx*/, CH_NULL);
#endif
            trace_2(TRACE_RISCOS_HOST, TEXT("gr_host_font_string_truncate new width: %d mp, str: \")%s\" (terminated)"), swidth_mp, report_ustr(ustr));
            return(swidth_mp);
        }
        else
        {
            trace_1(TRACE_RISCOS_HOST, TEXT("gr_host_font_string_truncate error: %s"), p_kernel_oserror->errmess);
        }
#elif WINDOWS
        const HDC hic_format_pixits = host_get_hic_format_pixits();
        HFONT h_font_old = SelectFont(hic_format_pixits, host_font);
        int nMaxExtent /*swidth_px*/ = swidth_mp / MILLIPOINTS_PER_PIXIT;
        int nFit = 0;
        SIZE size;
        PCTSTR tstr = _tstr_from_ustr(ustr);
        U32 tchars_n = tstrlen32(tstr);

        if(WrapOsBoolChecking(GetTextExtentExPoint(hic_format_pixits, tstr, tchars_n, nMaxExtent, &nFit, NULL /*pDx*/, &size)))
        {
            swidth_mp = size.cx * MILLIPOINTS_PER_PIXIT;
reportf(TEXT("gr_host_font_string_truncate(x:%u,len:%u,%s) returns x:%u,len:%u"), swidth_mp_truncate, ustrlen32(ustr), report_ustr(ustr), swidth_mp, nFit);
            PtrPutByteOff(ustr, nFit, CH_NULL);
            consume(HFONT, SelectFont(hic_format_pixits, h_font_old));
            return(swidth_mp);
        }
        consume(HFONT, SelectFont(hic_format_pixits, h_font_old));
#endif /* OS */
    }

    {
    U32 uchars_n = ustrlen32(ustr);
    U32 nchars_limit = (U32) (swidth_mp / SYSCHARWIDTH_MP); /* rounding down */
    U32 truncate_idx = MIN(uchars_n, nchars_limit);
    GR_MILLIPOINT truncate_mp = SYSCHARWIDTH_MP * truncate_idx;
    swidth_mp = truncate_mp;
    PtrPutByteOff(ustr, truncate_idx, CH_NULL);
    } /*block*/

    return(swidth_mp);
}

/******************************************************************************
*
* return the printing width of a fonty string in millipoints
*
******************************************************************************/

_Check_return_
extern GR_MILLIPOINT
gr_host_font_string_width(
    _HfontRef_opt_ HOST_FONT host_font,
    _In_z_      PC_USTR ustr)
{
    GR_MILLIPOINT swidth_mp;

    if(HOST_FONT_NONE != host_font)
    {
#if RISCOS
        _kernel_swi_regs rs;
        _kernel_oserror * p_kernel_oserror;

#if 1
        rs.r[0] = host_font;
        rs.r[1] = (int) ustr;
        rs.r[2] = FONT_SCANSTRING_USE_HANDLE /*r0*/;
        rs.r[3] = INT_MAX;
        rs.r[4] = INT_MAX;

        if(NULL == (p_kernel_oserror = WrapOsErrorChecking(_kernel_swi(Font_ScanString, &rs, &rs))))
        {
            swidth_mp = rs.r[3]; /*x*/
#else
        /* do initial font change economically */
        if(NULL != (p_kernel_oserror = font_SetFont(host_font)))
        {
            trace_1(TRACE_RISCOS_HOST, TEXT("fontxtra_stringwidth: setfont error %s"), p_kernel_oserror->errmess);
            return(0);
        }

        trace_1(TRACE_RISCOS_HOST, TEXT("fontxtra_stringwidth str in: \")%s\", |"), report_ustr(str));

        rs.r[1] = (int) ustr;
        rs.r[2] = INT_MAX;
        rs.r[3] = INT_MAX;
        rs.r[4] = -1;
        rs.r[5] = INT_MAX;

        if(NULL == (p_kernel_oserror = WrapOsErrorChecking(_kernel_swi(Font_StringWidth, &rs, &rs))))
        {
            swidth_mp = rs.r[2];
#endif
reportf(TEXT("gr_host_font_stringwidth(%s) returns x:%u mp"), report_ustr(ustr), swidth_mp);
            trace_1(TRACE_RISCOS_HOST, TEXT("|width: %d"), swidth_mp);
            return(swidth_mp);
        }
        else
        {
            trace_1(TRACE_RISCOS_HOST, TEXT("|error: %s"), p_kernel_oserror->errmess);
        }
#elif WINDOWS
        const HDC hic_format_pixits = host_get_hic_format_pixits();
        HFONT h_font_old = SelectFont(hic_format_pixits, host_font);
        SIZE size;

        if(status_ok(uchars_GetTextExtentPoint32(hic_format_pixits, ustr, ustrlen32(ustr), &size)))
        {
            swidth_mp = size.cx * MILLIPOINTS_PER_PIXIT;
reportf(TEXT("gr_host_font_stringwidth(%s) returns x:%u mp"), report_ustr(ustr), swidth_mp);
            consume(HFONT, SelectFont(hic_format_pixits, h_font_old));
            return(swidth_mp);
        }
        consume(HFONT, SelectFont(hic_format_pixits, h_font_old));
#endif /* OS */
    }

    swidth_mp = ustrlen32(ustr) * SYSCHARWIDTH_MP;
    return(swidth_mp);
}

/* end of gr_numcv.c */
