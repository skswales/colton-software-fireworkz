/* riscos/ho_core.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#define EXPOSE_RISCOS_SWIS 1
#include "ob_skel/xp_skelr.h"

#include "ob_skel/prodinfo.h"

#include <signal.h>

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
wimp_dispose_icon(
    _InVal_     wimp_w window_handle,
    _Inout_     wimp_i * const p_icon_handle)
{
    WimpDeleteIconBlock wdib;

    wdib.window_handle = (int) window_handle;
    wdib.icon_handle   = (int) *p_icon_handle;

    if(*p_icon_handle == BAD_WIMP_I)
        return(NULL);

    *p_icon_handle = BAD_WIMP_I;

    return(wimp_delete_icon(&wdib));
}

static inline void
wimp_reporterror_copyerror(
     _Out_      _kernel_oserror * o_e,
     _In_       const _kernel_oserror * e)
{
    /* copy error, sanitising */
    *o_e = *e;

    {
    const char * p_u8 = e->errmess;
    char * p_u8_out = o_e->errmess;
    int len = 0;
    int non_blanks = 0;

    for(;;)
    {
        U8 c = *p_u8++;
        *p_u8_out++ = c;
        if(CH_NULL == c)
            break;
        ++len;

        if( (c != 0x20) && (c != 0xA0) )
            ++non_blanks;

        if( (c <= 0x1F) || (c == 0x7F) )
            /* Found control characters in error string */
            p_u8_out[-1] = CH_FULL_STOP;
    }

    if(0 == len)
        strcpy(o_e->errmess, "No characters in error string");
    else if(0 == non_blanks)
        strcpy(o_e->errmess, "All characters in error string are blank");
    } /*block*/

    reportf(TEXT("OS error: %d:%s"), o_e->errnum, o_e->errmess);
}

_Check_return_
_Ret_valid_
extern _kernel_oserror *
wimp_reporterror_rf(
    _In_        _kernel_oserror * e,
    _InVal_     int errflags,
    _Out_       int * const p_button_clicked,
    _In_opt_z_  const char * message,
    _InVal_     int error_level)
{
    static _kernel_oserror g_e;

    *p_button_clicked = 1;

    wimp_reporterror_copyerror(&g_e, e);

    e = &g_e;

    if(!g_silent_shutdown)
    {
        _kernel_oserror * err;
        _kernel_swi_regs rs;

        riscos_hourglass_off();

        /* restore pointer to shape 1 (default) */
        (void) _kernel_osbyte(106, 1, 0);

        rs.r[0] = (int) e;
        rs.r[1] =       errflags;
        rs.r[2] = (int) product_ui_id();

        if(host_os_version_query() >= RISCOS_3_5)
        {
            if(NULL != message)
            {
                rs.r[1] |= Wimp_ReportError_NoAppName;
                rs.r[2] = (int) message;
            }
            rs.r[1] |= Wimp_ReportError_UseCategory; /*new style*/
            rs.r[1] |= Wimp_ReportError_Category(error_level); /* 11..9 */
            rs.r[3] = (int) g_product_riscos_app_sprite;
            rs.r[4] = 1; /* Wimp Sprite Area */
            rs.r[5] = NULL; /* no additional buttons */

#if 1 /* needed because RISC PC error handler takes upon itself to mutate some warnings into program errors */
            if(2 == error_level)
                g_e.errnum = 1; /* lie to over-knowledgeable RISC PC error handler */
#endif
        }

        if(NULL == (err = _kernel_swi(Wimp_ReportError, &rs, &rs)))
            *p_button_clicked = rs.r[1];

        riscos_hourglass_on();
    }

    return(e);
}

_Check_return_
_Ret_valid_
extern _kernel_oserror *
wimp_reporterror_simple(
    _In_        _kernel_oserror * e)
{
    int button_clicked;

    return(wimp_reporterror_rf(e, Wimp_ReportError_OK, &button_clicked, NULL, 2 /*Warning*/));
}

static UINT g_host_os_version = RISCOS_3_1;

static int g_host_platform_features = 0; /* SKS 19aug96 */

extern void
host_os_version_determine(void)
{
    _kernel_swi_regs rs;

    g_host_os_version = (_kernel_osbyte(0x81, 0, 0xFF) & 0xFF);

    rs.r[0] = 0; /* Read code features */

    if(NULL == _kernel_swi(OS_PlatformFeatures, &rs, &rs))
    {
        g_host_platform_features = rs.r[0];
    }
}

_Check_return_
extern UINT
host_os_version_query(void)
{
    return(g_host_os_version);
}

_Check_return_
extern int
host_platform_features_query(void)
{
    return(g_host_platform_features);
}

_Check_return_
extern int
host_query_alphabet_number(void)
{
reportf(TEXT("alphabet_number: %d"), _kernel_osbyte(0x47, 0x7F, 0xFF) & 0xFF);
    return((_kernel_osbyte(0x47, 0x7F, 0xFF) & 0xFF));
}

#endif /* RISCOS */

/* end of riscos/ho_core.c */
