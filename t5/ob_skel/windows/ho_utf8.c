/* windows/ho_utf8.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2006-2015 Stuart Swales */

#include "common/gflags.h"

#if WINDOWS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*ncr*/
extern int
utf8str_MessageBox(
    _HwndRef_opt_ HWND hWnd /* may be NULL */,
    _In_z_      PC_UTF8STR pText,
    _In_z_      PC_UTF8STR pCaption,
    _In_        UINT uType)
{
    STATUS status;
    int res = 0;

    if(g_silent_shutdown)
    {
        QUICK_TBLOCK_WITH_BUFFER(quick_tblock_text, 256);
        quick_tblock_with_buffer_setup(quick_tblock_text);

        status = quick_tblock_utf8str_add_n(&quick_tblock_text, pText, strlen_with_NULLCH);

        status_assert(status);

        if(status_ok(status))
            report_output(quick_tblock_tstr(&quick_tblock_text));

        quick_tblock_dispose(&quick_tblock_text);
    }
    else
    {
        QUICK_WBLOCK_WITH_BUFFER(quick_wblock_text, 256);
        QUICK_WBLOCK_WITH_BUFFER(quick_wblock_caption, 256);
        quick_wblock_with_buffer_setup(quick_wblock_text);
        quick_wblock_with_buffer_setup(quick_wblock_caption);

        if(status_ok(status = quick_wblock_utf8str_add_n(&quick_wblock_text, pText, strlen_with_NULLCH)))
            status = quick_wblock_utf8str_add_n(&quick_wblock_caption, pCaption, strlen_with_NULLCH);

        status_assert(status);

        res = MessageBoxW(hWnd, quick_wblock_wstr(&quick_wblock_text), quick_wblock_wstr(&quick_wblock_caption), uType);

        quick_wblock_dispose(&quick_wblock_text);
        quick_wblock_dispose(&quick_wblock_caption);
    }

    return(res);
}

_Check_return_
extern STATUS
utf8_ExtTextOut(
    _HdcRef_    HDC hdc,
    _In_        int x,
    _In_        int y,
    _In_        UINT options,
    _In_opt_    CONST RECT *pRect,
    _In_reads_opt_(uchars_n) PC_UTF8 pString,
    _InVal_     U32 uchars_n,
    _In_opt_    CONST INT *pDx)
{
    STATUS status;
    BOOL res;

    QUICK_WBLOCK_WITH_BUFFER(quick_wblock, 256);
    quick_wblock_with_buffer_setup(quick_wblock);

    /*RETURN_STATUS_CHECK_IF_NULL_OR_BAD_POINTER(pString);*/

    assert(NULL != pString);
    assert(NULL == pDx);

    if(status_ok(status = quick_wblock_utf8_add(&quick_wblock, pString, uchars_n)))
    {
        PCWCH pwch = quick_wblock_wchars(&quick_wblock);
        UINT cbCount = (UINT) quick_wblock_chars(&quick_wblock);

        res = WrapOsBoolChecking(ExtTextOutW(hdc, x, y, options, pRect, pwch, cbCount, pDx));

        if(!res)
            status = status_check();
    }

    quick_wblock_dispose(&quick_wblock);

    return(status);
}

_Check_return_
extern STATUS
utf8_GetTextExtentPoint32(
    _HdcRef_    HDC hdc,
    _In_reads_(uchars_n) PC_UTF8 pString,
    _InVal_     U32 uchars_n,
    _OutRef_    PSIZE pSize)
{
    STATUS status;
    BOOL res;

    QUICK_WBLOCK_WITH_BUFFER(quick_wblock, 256);
    quick_wblock_with_buffer_setup(quick_wblock);

    pSize->cx = pSize->cy = 0;

    if(status_ok(status = quick_wblock_utf8_add(&quick_wblock, pString, uchars_n)))
    {
        PCWCH pwch = quick_wblock_wchars(&quick_wblock);
        int count = (int) quick_wblock_chars(&quick_wblock);

        res = WrapOsBoolChecking(GetTextExtentPoint32W(hdc, pwch, count, pSize));

        if(!res)
            status = status_check();
#if TRACE_ALLOWED
        else if_constant(tracing(TRACE_APP_FONTS))
        {
            trace_5(TRACE_APP_FONTS,
                    TEXT("utf8_GetTextExtentPoint32: got: %d,%d %s for %.*s"),
                    pSize->cx, pSize->cy,
                    (MM_TWIPS == GetMapMode(hdc)) ? TEXT("twips") : TEXT("pixels"),
                    uchars_n, report_ustr(pString));
        }
#endif
    }

    quick_wblock_dispose(&quick_wblock);

    return(status);
}

#if (TSTR_IS_SBSTR && USTR_IS_SBSTR)

/* no conversion is required */

#else /* (TSTR_IS_SBSTR && USTR_IS_SBSTR) */

/*
low-lifetime conversion routines
*/

static struct tstr_from_ustr_statics
{
    PCTSTR last;
    U32 used;
    TCHAR buffer[4 * 1024];
}
tstr_from_ustr_statics;

_Check_return_
_Ret_z_ /* never NULL */
extern PCTSTR /*low-lifetime*/
_tstr_from_ustr(
    _In_z_      PC_USTR ustr)
{
    PTSTR dstptr;
    int wchars_n;
    int pass = 1;

#if CHECKING
    if(NULL == ustr)
    {
        assert0();
        return(TEXT("<<tstr_from_ustr - NULL>>"));
    }

    if(IS_PTR_NONE_ANY(ustr))
    {
        assert0();
        return(TEXT("<<tstr_from_ustr - NONE>>"));
    }

    if(contains_inline(ustr, ustrlen32(ustr)))
    {
        assert0();
        return(TEXT("<<tstr_from_ustr - CONTAINS INLINES>>"));
    }
#endif

#if CHECKING_UTF8
    /* Check that the string we are converting is valid UTF-8 */
    if(status_fail(ustr_validate(TEXT("_tstr_from_ustr arg"), ustr)))
        return(TEXT("<<tstr_from_ustr - INVALID UTF-8>>"));
#endif

    do  {
        U32 avail = elemof32(tstr_from_ustr_statics.buffer) - tstr_from_ustr_statics.used; /* NB may be zero! */

        dstptr = tstr_from_ustr_statics.buffer + tstr_from_ustr_statics.used;

        wchars_n =
            MultiByteToWideChar(CP_UTF8 /*SourceCodePage*/, 0 /*dwFlags*/,
                                (PCSTR) ustr, -1 /*strlen_with_NULLCH*/,
                                dstptr, (int) avail);

        if((0 != avail) && (0 != wchars_n))
        {
            assert(CH_NULL == dstptr[wchars_n-1]);
            tstr_from_ustr_statics.last = dstptr;
            break;
        }

        /* retry with whole buffer */
        tstr_from_ustr_statics.used = 0;
    }
    while(++pass <= 2);

    if(wchars_n > 0)
        tstr_from_ustr_statics.used += wchars_n;

    return(dstptr);
}

static struct ustr_from_tstr_statics
{
    PC_USTR last;
    U32 used;
    U8 buffer[4 * 1024];
}
ustr_from_tstr_statics;

_Check_return_
_Ret_z_ /* never NULL */
extern PC_USTR /*low-lifetime*/
_ustr_from_tstr(
    _In_z_      PCTSTR tstr)
{
    const UINT mbchars_CodePage = CP_UTF8;
    P_USTR dstptr;
    P_BOOL p_fUsedDefaultChar = NULL;
    int multi_n;
    int pass = 1;

    if(NULL == tstr)
        return(USTR_TEXT("<<ustr_from_tstr - NULL>>"));

    if(IS_PTR_NONE_ANY(tstr))
        return(USTR_TEXT("<<ustr_from_tstr - NONE>>"));

    do  {
        U32 avail = elemof32(ustr_from_tstr_statics.buffer) - ustr_from_tstr_statics.used; /* NB may be zero! */

#if CHECKING && 1
        if(avail < 200)
            avail = 0;
#endif

        dstptr = ustr_AddBytes_wr(ustr_from_tstr_statics.buffer, ustr_from_tstr_statics.used);

        multi_n =
            WideCharToMultiByte(mbchars_CodePage, 0 /*dwFlags*/,
                                tstr, -1 /*strlen_with_NULLCH*/,
                                (PSTR) dstptr, (int) avail,
                                NULL /*lpDefaultChar*/, p_fUsedDefaultChar);

        if((0 != avail) && (0 != multi_n))
        {
            assert(CH_NULL == PtrGetByteOff(dstptr, multi_n-1));
            /*assert(!fUsedDefaultChar);*/
            ustr_from_tstr_statics.last = dstptr;
            break;
        }

        CODE_ANALYSIS_ONLY(if(0 == multi_n) dstptr[0] = CH_NULL);

        /* retry with whole buffer */
        ustr_from_tstr_statics.used = 0;
    }
    while(++pass <= 2);

    if(multi_n > 0)
        ustr_from_tstr_statics.used += multi_n;

#if CHECKING_UTF8
    if(multi_n > 0) /* Check that the converted string is valid UTF-8 */
        (void) ustr_validate_n(TEXT("_ustr_from_tstr tmpbuf"), dstptr, multi_n);
#endif

    return(dstptr);
}

#endif /* (TSTR_IS_SBSTR && USTR_IS_SBSTR) */

#endif /* WINDOWS */

/* end of windows/ho_utf8.c */
