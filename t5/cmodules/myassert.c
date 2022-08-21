/* myassert.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Non-standard but helpful assertion */

/* SKS February 1991 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

#if WINDOWS
#include <crtdbg.h>
#endif

/* Allow CHECKING even in RELEASED code if really wanted */
#undef CHECKING
#define CHECKING 1

#include "cmodules/myassert.h"

static S32
hard_assertion = 0;

#if WINDOWS && 0
hard_assertion = 1000; /* an OLE server really needs this */
#endif

#if WINDOWS

static int
WINAPI
MessageBoxInReportingThread(
    _In_opt_    HWND hWnd,
    _In_opt_    PCTSTR lpText,
    _In_opt_    PCTSTR lpCaption,
    _In_        UINT uType);

#endif /* OS */

/* on assertion failure, this procedure is called to report the failure in a system-dependent fashion */

#define ASSERTION_FAILURE_PREFIX TEXT("Runtime failure: %s() in file %s, line ") U32_TFMT TEXT(".")

#if RISCOS
#define ASSERTION_FAILURE_YN TEXT("Click Continue to exit immediately, losing data, Cancel to attempt to resume execution.")
#elif WINDOWS
#define ASSERTION_FAILURE_YN TEXT("Click OK to exit immediately, losing data, Cancel to attempt to resume execution.")
#else
#define ASSERTION_FAILURE_YN TEXT("")
#endif

#if RISCOS
#define ASSERTION_FAILURE_PREFIX_RISCOS TEXT("Assert: ")
#endif

#if RISCOS
#define ASSERTION_TRAP_QUERY TEXT("Click Continue to trap to debugger, Cancel to exit normally")
#elif WINDOWS
#define ASSERTION_TRAP_QUERY TEXT("Click OK to trap to debugger, Cancel to exit normally")
#endif

_Check_return_
extern BOOL __cdecl
__myasserted(
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _InVal_     U32 line_no,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...)
{
    va_list va;
    BOOL crash_and_burn;

    va_start(va, format);
    crash_and_burn = __vmyasserted(tstr_function, tstr_file, line_no, NULL, format, va);
    va_end(va);

    return(crash_and_burn);
}

_Check_return_
extern BOOL __cdecl
__myasserted_msg(
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _InVal_     U32 line_no,
    _In_z_      PCTSTR message,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...)
{
    va_list va;
    BOOL crash_and_burn;

    va_start(va, format);
    crash_and_burn = __vmyasserted(tstr_function, tstr_file, line_no, message, format, va);
    va_end(va);

    return(crash_and_burn);
}

_Check_return_
extern BOOL
__myasserted_EQ(
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _InVal_     U32 line_no,
    _InVal_     U32 val1,
    _InVal_     U32 val2)
{
    if(val1 == val2)
        return(FALSE);

    return(__myasserted(tstr_function, tstr_file, line_no, U32_TFMT TEXT("==") U32_TFMT, val1, val2));
}

_Check_return_
extern BOOL
__vmyasserted(
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _InVal_     U32 line_no,
    _In_opt_z_  PCTSTR message,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list va_in)
{

#if RISCOS

    va_list va;
    _kernel_oserror err;
    PTSTR p = err.errmess;
    int button_clicked;

    /* we need to know how to copy this! */
#if !RISCOS
    va = va_in;
#else
    va[0] = va_in[0];
#endif

    err.errnum = 0;

    /* test output string for overrun of wimp error box */
    if(!IS_PTR_NULL_OR_NONE(format))
    {
        consume_int(vsnprintf(p, elemof32(err.errmess), format, va));

        /* unbeknown to SKS, vsprintf modifies va, so reload necessary ... */
#if !RISCOS
        va = va_in;
#else
        va[0] = va_in[0];
#endif

        if(strlen(p) > 32)
        {
            /* make a hole to put the first prefix in */
            memmove32(p +  sizeof32(ASSERTION_FAILURE_PREFIX_RISCOS)-1 /*to*/, p /*from*/, strlen32(p) + 1);

            /* plonk it in */
            memcpy32(p, ASSERTION_FAILURE_PREFIX_RISCOS, sizeof32(ASSERTION_FAILURE_PREFIX_RISCOS)-1);

            consume(_kernel_oserror *, wimp_reporterror_rf(&err, Wimp_ReportError_OK, &button_clicked, NULL, 3 /*STOP*/));

            format = NULL;
        }
    }

    p += xsnprintf(p, elemof32(err.errmess) - (p - err.errmess), ASSERTION_FAILURE_PREFIX TEXT(" - "), tstr_function, tstr_file, line_no);

    p += xsnprintf(p, elemof32(err.errmess) - (p - err.errmess), TEXT("%s"), message ? message : ASSERTION_FAILURE_YN);

    if(!IS_PTR_NULL_OR_NONE(format))
    {
        *p++ = CH_SPACE;
        consume_int(vsnprintf(p, elemof32(err.errmess) - (p - err.errmess), format, va));
    }

    consume(_kernel_oserror *, wimp_reporterror_rf(&err, Wimp_ReportError_OK | Wimp_ReportError_Cancel, &button_clicked, NULL, 3 /*STOP*/));

    if(button_clicked & Wimp_ReportError_OK)
    {
#if !RELEASED
        tstr_xstrkpy(err.errmess, elemof32(err.errmess), message ? message : ASSERTION_TRAP_QUERY);

        consume(_kernel_oserror *, wimp_reporterror_rf(&err, Wimp_ReportError_OK | Wimp_ReportError_Cancel, &button_clicked, NULL, 3 /*STOP*/));

        if(button_clicked & Wimp_ReportError_OK)
            return(TRUE);
#endif

        exit(EXIT_FAILURE);
    }

#elif WINDOWS

    TCHARZ szBuffer[1024];
    size_t len = 0;
    UINT uType = MB_TASKMODAL | MB_OKCANCEL | MB_ICONEXCLAMATION;

    len = _sntprintf_s(szBuffer, elemof32(szBuffer), _TRUNCATE,
                       ASSERTION_FAILURE_PREFIX TEXT("\n\n%s"),
                       tstr_function, tstr_file, line_no, message ? message : ASSERTION_FAILURE_YN);
    assert(len != (size_t) -1);

    if(!IS_PTR_NULL_OR_NONE(format))
    {
        if(len < elemof32(szBuffer) - 3)
        {
            szBuffer[len++] = '\n';
            szBuffer[len++] = '\n';
            (void) _vsntprintf_s(szBuffer + len, elemof32(szBuffer) - len, _TRUNCATE, format, va_in);
        }
    }

#if TRACE_ALLOWED
    trace_1(TRACE_OUT | TRACE_ANY, TEXT("!!!%s"), szBuffer);
#endif

#if 1
    if(0 != hard_assertion)
    {
        /* avoid putting up a simple message box in this state */
        reportf(TEXT("!!!%s"), szBuffer);
        return(TRUE);
    }
#endif

    if(g_silent_shutdown)
    {
        FatalExit(1);
    }
    else
    {
        hard_assertion++;

        if(IDOK == MessageBoxInReportingThread(NULL, szBuffer, product_ui_id(), uType))
        {
#if RELEASED
            FatalExit(1);
#else
#if 1 /* always break without further ado for ease of running under debugger */
            hard_assertion--;

            return(TRUE);
#else
            PCTSTR message_query = message ? message : ASSERTION_TRAP_QUERY;

            if(IDOK == MessageBoxInReportingThread(NULL, message_query, product_ui_id(), uType))
            {
                hard_assertion--;

                return(TRUE);
            }

            FatalExit(1);
#endif
#endif
        }

        hard_assertion--;
    }


#else /* generic */

    fprintf(stderr, ASSERTION_FAILURE_PREFIX, tstr_file, line_no);

    if(!IS_PTR_NULL_OR_NONE(format))
    {
        fputc(CH_SPACE, stderr);
        vfprintf(stderr, format, va_in);
    }

    fputc('\n', stderr);

    fflush(stderr);

#endif /* OS */

    return(FALSE);
}

_Check_return_
extern BOOL
__bool_assert(
    _InVal_     BOOL bool_val,
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _InVal_     U32 line_no,
    _In_z_      PCTSTR tstr)
{
    if(bool_val)
        return(bool_val);

    if(__myasserted(tstr_function, tstr_file, line_no, TEXT("%s: is %s"), tstr, report_boolstring(bool_val)))
        __crash_and_burn_here();

    return(bool_val);
}

extern void
__hard_assert(
    _InVal_     BOOL hard_mode)
{
    if(hard_mode)
        hard_assertion++;
    else
        hard_assertion--;
}

_Check_return_
extern STATUS
__status_assert(
    _InVal_     STATUS status,
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _InVal_     U32 line_no,
    _In_z_      PCTSTR tstr)
{
    if(status_ok(status) || (STATUS_CANCEL == status))
        return(status);

#if 0 /* sometimes you may need to take the assertions out */
    UNREFERENCED_PARAMETER(tstr_function);
    UNREFERENCED_PARAMETER(tstr_file);
    UNREFERENCED_PARAMETER_InVal_(line_no);
    UNREFERENCED_PARAMETER(tstr);
#else
    {
    PCTSTR tstr_s = resource_lookup_tstr_no_default(status);
    if(__myasserted(tstr_function, tstr_file, line_no, tstr_s ? TEXT("%s: status = ") S32_TFMT TEXT(", %s") : TEXT("%s: status = ") S32_TFMT, tstr, status, tstr_s))
        __crash_and_burn_here();
    } /*block*/
#endif

    return(status);
}

#if RISCOS

/*ncr*/
extern _kernel_oserror *
__WrapOsErrorChecking(
    _In_opt_    _kernel_oserror * const p_kernel_oserror,
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _In_        int line_no,
    _In_z_      PCTSTR tstr)
{
    const _kernel_oserror * const p_kernel_oserror_real = (const _kernel_oserror *) p_kernel_oserror;

    if(NULL == p_kernel_oserror)
        return(NULL);

    if(__myasserted(tstr_function, tstr_file, line_no,
                    TEXT("%s")
                    TEXT("\n")
                    TEXT("FAILED: err=%d:") U32_XTFMT TEXT(" %s"),
                    tstr,
                    p_kernel_oserror_real->errnum, p_kernel_oserror_real->errnum,
                    p_kernel_oserror_real->errmess))
        __crash_and_burn_here();

    return(p_kernel_oserror);
}

#elif WINDOWS

_Check_return_
extern BOOL
__WrapOsBoolChecking(
    _InVal_     BOOL res,
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _In_        int line_no,
    _In_z_      PCTSTR tstr)
{
    if(res)
        return(res);

    {
    DWORD dwLastError = GetLastError();

    if(NO_ERROR != dwLastError)
    {
        PTSTR buffer = NULL;

        if(!FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL,
            dwLastError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &buffer,
            0,
            NULL))
        {
            buffer = NULL;
        }

        reportf(TEXT("%s")
                TEXT("\n")
                TEXT("FAILED: err=") S32_TFMT TEXT(":") U32_XTFMT TEXT(" %s at %s:%s(%d)"),
                tstr,
                (S32) dwLastError, dwLastError,
                buffer ? buffer : TEXT("Error message unavailable"),
                tstr_function, tstr_file, line_no);

        if(!hard_assertion) /* best not to do assert message box in this state */
        {
            if(__myasserted(tstr_function, tstr_file, line_no,
                            TEXT("%s")
                            TEXT("\n")
                            TEXT("FAILED: err=") S32_TFMT TEXT(":") U32_XTFMT TEXT(" %s"),
                            tstr,
                            (S32) dwLastError, dwLastError,
                            buffer ? buffer : TEXT("Error message unavailable")))
                __crash_and_burn_here();

        }

        if(NULL != buffer)
            LocalFree(buffer);
    }
    } /*block*/

    return(res);
}

/*ncr*/
extern BOOL
WrapOsBoolReporting(
    _InVal_     BOOL res)
{
    if(res)
        return(res);

    {
    DWORD dwLastError = GetLastError();

    if(NO_ERROR != dwLastError)
    {
        PTSTR buffer = NULL;
        TCHARZ szBuffer[1024];
        size_t len = 0;
        UINT uType = MB_TASKMODAL | MB_OK | MB_ICONEXCLAMATION;

        if(!FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL,
            dwLastError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &buffer,
            0,
            NULL))
        {
            buffer = NULL;
        }

        len = _sntprintf_s(szBuffer, elemof32(szBuffer), _TRUNCATE,
                           TEXT("Error from Windows: err=") DWORD_TFMT TEXT(":") DWORD_XTFMT TEXT(" %s)"),
                           dwLastError, dwLastError,
                           buffer ? buffer : TEXT("Error message unavailable"));

        report_output(szBuffer);

        hard_assertion++;

        if(IDOK == MessageBoxInReportingThread(NULL, szBuffer, product_ui_id(), uType))
        { /*EMPTY*/ }

        hard_assertion--;

        if(NULL != buffer)
            LocalFree(buffer);
    }
    } /*block*/

    return(res);
}

typedef struct ReportingData
{
    /*IN*/
    HWND hWnd;
    PCTSTR lpText;
    PCTSTR lpCaption;
    UINT uType;

    /*OUT*/
    int mb_result;
}
ReportingData, * P_ReportingData;

static DWORD
WINAPI
ReportingThreadProc(
    _In_    LPVOID lpParameter)
{
    P_ReportingData p_ReportingData = (P_ReportingData) lpParameter;

    p_ReportingData->mb_result =
        MessageBox(p_ReportingData->hWnd, p_ReportingData->lpText, p_ReportingData->lpCaption, p_ReportingData->uType);

    return(0);
}

static int
WINAPI
MessageBoxInReportingThread(
    _In_opt_    HWND hWnd,
    _In_opt_    PCTSTR lpText,
    _In_opt_    PCTSTR lpCaption,
    _In_        UINT uType)
{
    HANDLE hReportingThread;
    DWORD dwThreadId;
    ReportingData ReportingData;

    ReportingData.hWnd = hWnd;
    ReportingData.lpText = lpText;
    ReportingData.lpCaption = lpCaption;
    ReportingData.uType = uType;
    ReportingData.mb_result = 0;

    hReportingThread = CreateThread(NULL, 0, ReportingThreadProc, &ReportingData, 0, &dwThreadId);

    if(NULL != hReportingThread)
    {
        (void) WaitForSingleObject(hReportingThread, INFINITE);
    }

    return(ReportingData.mb_result);
}

#endif /* OS */

/* end of myassert.c */
