/* windows/ho_dde.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#pragma warning(push)
#pragma warning(disable:4214) /* nonstandard extension used : bit field types other than int */
#include "dde.h"
#pragma warning(pop)

/* comment out '| (TRACE_OUT | TRACE_ANY)' when not needed */
#define TRACE__DDE  (0 | (TRACE_OUT | TRACE_ANY))

/*
internal routines
*/

static HWND hwnd_dde_server; /* needed for window-less DDE in any case */

static ATOM atom_Application;

static ATOM atom_Topic_System;

BOOL g_started_for_dde = FALSE;

static void
host_create_window_for_dde_server(void);

extern void
ho_dde_msg_exit2(void)
{
    if(0 != atom_Application)
    {
        GlobalDeleteAtom(atom_Application);
        atom_Application = 0;
    }

    if(0 != atom_Topic_System)
    {
        GlobalDeleteAtom(atom_Topic_System);
        atom_Topic_System = 0;
    }
}

extern void
host_dde_startup(void)
{
    /* DDE stuff */
    atom_Application = GlobalAddAtomW(atom_program_wstr);
    atom_Topic_System = GlobalAddAtomW(L"system");
    reportf(TEXT("DDE created aApplication: ") U32_XTFMT TEXT(", aTopicSystem: ") U32_XTFMT, (U32) atom_Application, (U32) atom_Topic_System);

    host_create_window_for_dde_server();
}

static BOOL
host_dde_execute(
    _In_z_      PCTSTR tstr)
{
    PTSTR match;
    U32 len_match;
    TCHARZ buffer[_MAX_PATH];
    U32 outidx = 0;
    TCHAR ch;

    match = TEXT("[FileNew(");
    len_match = tstrlen32(match);
    if(0 == _tcsncicmp(tstr, match, len_match))
    {
        TCHAR term = CH_RIGHT_PARENTHESIS;

        tstr += len_match;

        if(*tstr == '\"') { ++tstr; term = '\"'; }
        do  {
            ch = *tstr++;
            if(ch == term) ch = CH_NULL;
            buffer[outidx++] = ch;
        }
        while(ch);

        status_consume(load_this_template_file_rl(P_DOCU_NONE, buffer));
        return(TRUE);
    }

    match = TEXT("[FileOpen(");
    len_match = tstrlen32(match);
    if(0 == _tcsncicmp(tstr, match, len_match))
    {
        TCHAR term = CH_RIGHT_PARENTHESIS;

        tstr += len_match;

        if(*tstr == '\"') { ++tstr; term = '\"'; }
        do  {
                ch = *tstr++;
                if(ch == term) ch = CH_NULL;
                buffer[outidx++] = ch;
        }
        while(ch);

        status_consume(load_file_for_windows_startup_rl(buffer));
        return(TRUE);
    }

    match = TEXT("[FilePrint(");
    len_match = tstrlen32(match);
    if(0 == _tcsncicmp(tstr, match, len_match))
    {
        TCHAR term = CH_RIGHT_PARENTHESIS;

        tstr += len_match;

        if(*tstr == '\"') { ++tstr; term = '\"'; }
        do  {
            ch = *tstr++;
            if(ch == term) ch = CH_NULL;
            buffer[outidx++] = ch;
        }
        while(ch);

        status_consume(load_and_print_this_file_rl(buffer, NULL));
        return(TRUE);
    }

    match = TEXT("[FilePrintTo(");
    len_match = tstrlen32(match);
    if(0 == _tcsncicmp(tstr, match, len_match))
    {
        TCHARZ printer[256];
        TCHAR qterm = CH_NULL;
        TCHAR term = CH_COMMA;
        BOOL skip = FALSE;

        buffer[0] = CH_NULL; /* total paranoia in case of bad DDE */

        tstr += len_match;

        if(*tstr == '\"') { ++tstr; qterm = '\"'; }
        do  {
            ch = *tstr++;
            if(qterm && (ch == qterm))
                /* skip all after quote till term */
                skip = TRUE;
            if(ch == term) ch = CH_NULL;
            if((CH_NULL == ch) || !skip)
                buffer[outidx++] = ch;
        }
        while(ch);

        term = CH_RIGHT_PARENTHESIS;
        outidx = 0;
        if(*tstr == '\"') { ++tstr; term = '\"'; }
        do  {
            ch = *tstr++;
            if(ch == term) ch = CH_NULL;
            printer[outidx++] = ch;
        }
        while(ch);

        status_consume(load_and_print_this_file_rl(buffer, printer));
        return(TRUE);
    }

    return(FALSE);
}

static LRESULT
host_wm_dde_initiate(
    _HwndRef_   HWND hwnd_server,
    _InVal_     UINT uiMsg,
    _InVal_     WPARAM wParam,
    _InVal_     LPARAM lParam)
{
    const HWND hwnd_client = (HWND) wParam;
    const ATOM aApplication = LOWORD(lParam);
    const ATOM aTopic = HIWORD(lParam);

    UNREFERENCED_PARAMETER_InVal_(uiMsg);

    if((aApplication != atom_Application) && ((ATOM) 0 != aApplication))
        return(0);

    if((aTopic != atom_Topic_System) && ((ATOM) 0 != aTopic))
        return(0);

    reportf(TEXT("WM_DDE_INITIATE(server: ") U32_XTFMT TEXT(") [client=") U32_XTFMT TEXT("; aApplication=") U32_XTFMT TEXT("; aTopic=") U32_XTFMT TEXT("] matched"),
            (U32) (UINT_PTR) hwnd_server, (U32) (UINT_PTR) hwnd_client, (U32) aApplication, (U32) aTopic);

    {
    const WPARAM wParam_reply = (WPARAM) hwnd_server;
    const LPARAM lParam_reply = MAKELPARAM(atom_Application, atom_Topic_System); /* reply with our allocated ATOMs */
    reportf(TEXT("DDE SendMessage(client: ") U32_XTFMT TEXT(", WM_DDE_ACK, wParam ") U32_XTFMT TEXT(", lParam ") U32_XTFMT TEXT(")"),
            (U32) (UINT_PTR) hwnd_client, (U32) wParam_reply, (U32) lParam_reply);
    void_WrapOsBoolChecking(SendMessage(hwnd_client, WM_DDE_ACK, wParam_reply, lParam_reply));
    } /*block*/

    return(0);
}

static LRESULT
host_wm_dde_terminate(
    _HwndRef_   HWND hwnd_server,
    _InVal_     UINT uiMsg,
    _InVal_     WPARAM wParam,
    _InVal_     LPARAM lParam)
{
    const HWND hwnd_client = (HWND) wParam;

    UNREFERENCED_PARAMETER_InVal_(uiMsg);
    UNREFERENCED_PARAMETER_InVal_(lParam);

    reportf(TEXT("WM_DDE_TERMINATE(server: ") U32_XTFMT TEXT(") [client=") U32_XTFMT TEXT("]"),
            (U32) (UINT_PTR) hwnd_server, (U32) (UINT_PTR) hwnd_client);

    { /* Respond with another WM_DDE_TERMINATE */
    const WPARAM wParam_reply = (WPARAM) hwnd_server;
    const LPARAM lParam_reply = 0L;
    reportf(TEXT("DDE PostMessage(client: ") U32_XTFMT TEXT(", WM_DDE_TERMINATE, wParam ") U32_XTFMT TEXT(", lParam ") U32_XTFMT TEXT(")"),
            (U32) (UINT_PTR) hwnd_client, (U32) wParam_reply, (U32) lParam_reply);
    void_WrapOsBoolChecking(PostMessage(hwnd_client, WM_DDE_TERMINATE, wParam_reply, lParam_reply));
    } /*block*/

    return(0);
}

static LRESULT
host_wm_dde_execute(
    _HwndRef_   HWND hwnd_server,
    _InVal_     UINT uiMsg,
    _InVal_     WPARAM wParam,
    _InVal_     LPARAM lParam)
{
    const HWND hwnd_client = (HWND) wParam;
    HGLOBAL hCommands = (HGLOBAL) lParam;
    DDEACK ddeack;

    UNREFERENCED_PARAMETER_InVal_(uiMsg);

    ddeack.bAppReturnCode = 0;
    ddeack.reserved = 0;
    ddeack.fBusy = 0;
    ddeack.fAck = 0;

    {
    PCTSTR tstr = (PCTSTR) GlobalLock(hCommands);
    reportf(TEXT("WM_DDE_EXECUTE(server: ") U32_XTFMT TEXT(") [client=") U32_XTFMT TEXT("; hCommands=") U32_XTFMT TEXT("] %s"),
            (U32) (UINT_PTR) hwnd_server, (U32) (UINT_PTR) hwnd_client, (U32) (UINT_PTR) hCommands, report_tstr(tstr));
    if((NULL != tstr) && host_dde_execute(tstr))
    {
        ddeack.fAck = 1;
    }
    GlobalUnlock(hCommands);
    } /*block*/

    hard_assert(TRUE);

    {
    const WORD wStatus = * (const WORD *) &ddeack;
    const WPARAM wParam_reply = (WPARAM) hwnd_server;
    const LPARAM lParam_reply = ReuseDDElParam(lParam, uiMsg, WM_DDE_ACK, wStatus, (UINT_PTR) hCommands);
    reportf(TEXT("DDE PostMessage(client: ") U32_XTFMT TEXT(", WM_DDE_ACK, wParam ") U32_XTFMT TEXT(", lParam ") U32_XTFMT TEXT(")"),
            (U32) (UINT_PTR) hwnd_client, (U32) wParam_reply, (U32) lParam_reply);
    if(!WrapOsBoolChecking(PostMessage(hwnd_client, WM_DDE_ACK, wParam_reply, lParam_reply)))
        GlobalFree(hCommands);
    } /*block*/

    hard_assert(FALSE);

    return(0);
}

static LRESULT CALLBACK
host_dde_window_event_handler(
    _HwndRef_   HWND hwnd,
    _InVal_     UINT uiMsg,
    _InVal_     WPARAM wParam,
    _InVal_     LPARAM lParam)
{
    switch(uiMsg)
    {
    case WM_DDE_INITIATE:
        return(host_wm_dde_initiate(hwnd, uiMsg, wParam, lParam));

    case WM_DDE_TERMINATE:
        return(host_wm_dde_terminate(hwnd, uiMsg, wParam, lParam));

    case WM_DDE_EXECUTE:
        return(host_wm_dde_execute(hwnd, uiMsg, wParam, lParam));

    default:
        return(host_top_level_window_event_handler(hwnd, uiMsg, wParam, lParam)); /* and so to DefWindowProc */
    }
}

/*
create DDE application window - should be kept invisible
*/

static void
host_create_window_for_dde_server(void)
{
    if(NULL == g_hInstancePrev)
    {   /* register DDE window class */
        WNDCLASS wndclass;
        zero_struct(wndclass);
      /*wndclass.style = 0;*/
        wndclass.lpfnWndProc = (WNDPROC) host_dde_window_event_handler;
      /*wndclass.cbClsExtra = 0;*/
      /*wndclass.cbWndExtra = 0;*/
        wndclass.hInstance = GetInstanceHandle();
        wndclass.hIcon = host_get_prog_icon_large();
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW); /* Do not destroy system cursor */
        wndclass.hbrBackground = (HBRUSH) (UINT_PTR) (COLOR_BTNFACE + 1);
      /*wndclass.lpszMenuName = NULL;*/
        wndclass.lpszClassName = window_class[APP_WINDOW_CLASS_DDE];
        if(!WrapOsBoolChecking(RegisterClass(&wndclass)))
            return;
    }

    /* caller can test for hwnd_dde_server */
    hwnd_dde_server =
        CreateWindowEx(0L, window_class[APP_WINDOW_CLASS_DDE], product_ui_id(),
                       WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX,
                       CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                       NULL, NULL, GetInstanceHandle(), NULL);

    reportf(TEXT("DDE created server: ") U32_XTFMT, (U32) (UINT_PTR) hwnd_dde_server);
}

/* end of windows/ho_icon.c */
