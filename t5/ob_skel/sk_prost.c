/* sk_prost.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Process indicator routines for Fireworkz */

/* SKS July 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

/*
callback functions
*/

#if WINDOWS && WINDOWS_PROCESS_DIALOG_WINDOW

static LRESULT CALLBACK
wndproc_sk_prost(
    _HwndRef_   HWND hwnd,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam);

#endif /* WINDOWS && WINDOWS_PROCESS_DIALOG_WINDOW*/

/*
internal structure
*/

typedef struct STATUS_LINE_ENTRY
{
    STATUS_LINE_LEVEL level;
    UI_TEXT ui_text;
}
STATUS_LINE_ENTRY, * P_STATUS_LINE_ENTRY;

/*
internal functions
*/

_Check_return_
static BOOL
ui_text_from_process_status(
    _OutRef_    P_UI_TEXT p_ui_text,
    P_PROCESS_STATUS p_process_status);

#if WINDOWS && WINDOWS_PROCESS_DIALOG_WINDOW

static const TCHARZ
sk_prost_class_name[] = TEXT("T5_ob_skel_sk_prost");

static LOGFONT g_logfont_sk_prost;

static HFONT g_h_font_sk_prost;

static void
ensure_h_font_sk_prost(void)
{
    if(NULL != g_h_font_sk_prost)
        return;

    { /* SKS 22feb2012 - obtain message font from system metrics */
    NONCLIENTMETRICS nonclientmetrics;
    nonclientmetrics.cbSize = (UINT) sizeof32(NONCLIENTMETRICS);
#if (WINVER >= 0x0600) /* keep size compatible with older OSes even if we can target newer */
    nonclientmetrics.cbSize -= sizeof32(nonclientmetrics.iPaddedBorderWidth);
#endif
    if(WrapOsBoolChecking(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, nonclientmetrics.cbSize, &nonclientmetrics, 0)))
    {
        g_logfont_sk_prost = nonclientmetrics.lfMessageFont;
    }
    else
    {
        LOGFONT logfont;
        zero_struct(logfont);
        logfont.lfHeight = -10;
        logfont.lfWeight = FW_NORMAL;
        logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_ROMAN;
        tstr_xstrkpy(logfont.lfFaceName, elemof32(logfont.lfFaceName), TEXT("MS Shell Dlg"));
        g_logfont_sk_prost = logfont;
    }
    trace_2(TRACE_APP_DIALOG, TEXT("*** sk_prost logfont is %d, %s ***"), g_logfont_sk_prost.lfHeight, report_tstr(g_logfont_sk_prost.lfFaceName));
    } /*block*/

    void_WrapOsBoolChecking(HOST_FONT_NONE != (
    g_h_font_sk_prost = CreateFontIndirect(&g_logfont_sk_prost)));
}

_Check_return_
static BOOL
wndproc_sk_prost_onCreate(
    _HwndRef_   HWND hwnd,
    _In_        LPCREATESTRUCT lpCreateStruct)
{
    consume(LONG_PTR, SetWindowLongPtr(hwnd, 0, (LONG_PTR) lpCreateStruct->lpCreateParams));

    ensure_h_font_sk_prost();

    return(TRUE);
}

static void
sk_prost_onPaint(
    _HwndRef_   HWND hwnd,
    _InRef_     PPAINTSTRUCT p_paintstruct)
{
    P_PROCESS_STATUS p_process_status = (P_PROCESS_STATUS) GetWindowLongPtr(hwnd, 0);
    const HDC hdc = p_paintstruct->hdc;
    HFONT h_font_old = SelectFont(hdc, g_h_font_sk_prost);
    RECT client_rect;
    UI_TEXT ui_text;

    GetClientRect(hwnd, &client_rect);

    SetTextAlign(hdc, TA_CENTER | TA_BASELINE);

    SetBkMode(hdc, TRANSPARENT);

    if(ui_text_from_process_status(&ui_text, p_process_status))
    {
        PCTSTR tstr = ui_text_tstr(&ui_text);

        /* centre this text in our wee window */
        TextOut(hdc, (client_rect.right - client_rect.left) / 2, (client_rect.bottom - client_rect.top) / 2 + 2, tstr, (int) tstrlen32(tstr));
    }

    ui_text_dispose(&ui_text);

    consume(HFONT, SelectFont(hdc, h_font_old));
}

static void
wndproc_sk_prost_onPaint(
    _HwndRef_   HWND hwnd)
{
    PAINTSTRUCT paintstruct;

    if(!BeginPaint(hwnd, &paintstruct))
        return;

    hard_assert(TRUE);

    /* rect to be painted is paintstruct.rcPaint */
    sk_prost_onPaint(hwnd, &paintstruct);

    hard_assert(FALSE);

    EndPaint(hwnd, &paintstruct);
}

static LRESULT CALLBACK
wndproc_sk_prost(
    _HwndRef_   HWND hwnd,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam)
{
    switch(uiMsg)
    {
    HANDLE_MSG(hwnd, WM_CREATE, wndproc_sk_prost_onCreate);
    HANDLE_MSG(hwnd, WM_PAINT,  wndproc_sk_prost_onPaint);

    default:
        return(DefWindowProc(hwnd, uiMsg, wParam, lParam));
    }
}

#endif /* WINDOWS && WINDOWS_PROCESS_DIALOG_WINDOW */

static void
do_actind(
    P_PROCESS_STATUS p_process_status)
{
    S32 percent = p_process_status->data.percent.current;

    static S32 last_percent = -1;

    /* don't bother with 0% change */
    if(percent == last_percent)
        return;

    if(percent < 0)
    {
        if(last_percent >= 0)
            /* switch off and reset statics */
            last_percent = -1;

        return;
    }

    if(percent > 99)
    {
        /* SKS 24feb93 but let anything bigger than 100 wrap */
        percent = percent % 100;

        if(percent == 0)
            percent = 99;
    }

    last_percent = percent;

#if RISCOS
    if(percent > 0)
        riscos_hourglass_percent(percent);
#endif
}

/* SKS 27apr95 removed support for dialog on RISCOS.  It's now a HWND on WINDOWS */
#if (WINDOWS && !WINDOWS_PROCESS_DIALOG_WINDOW) || (RISCOS && 0)

enum PROCESS_STATUS_CONTROL_IDS
{
    PROCESS_STATUS_CONTROL_ID_REASON = 20,
    PROCESS_STATUS_CONTROL_ID_STATUS
};

/* these data must be copied */

static const DIALOG_CONTROL
process_status_reason =
{
    PROCESS_STATUS_CONTROL_ID_REASON, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, PROCESS_STATUS_CONTROL_ID_STATUS, PROCESS_STATUS_CONTROL_ID_STATUS, PROCESS_STATUS_CONTROL_ID_STATUS },
    { DIALOG_SYSCHARSL_H(10), 0, DIALOG_STDSPACING_H, 0 },
    { DRT(RTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
process_status_reason_data = { { UI_TEXT_TYPE_NONE }, { 1 /*left_text*/ } };

static const DIALOG_CONTROL
process_status_status =
{
    PROCESS_STATUS_CONTROL_ID_STATUS, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_SYSCHARSL_H(5), 32 * PIXITS_PER_RISCOS },
    { DRT(LTLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
process_status_status_data = { { UI_TEXT_TYPE_NONE } };

_Check_return_
static STATUS
dialog_process_status_reflect(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    const P_PROCESS_STATUS p_process_status = (P_PROCESS_STATUS) p_dialog_msg_ctl_create_state->client_handle;

    if(p_dialog_msg_ctl_create_state->dialog_control_id == process_status_reason.control_id)
    {
        p_process_status->h_dialog = p_dialog_msg_ctl_create_state->h_dialog;
        p_dialog_msg_ctl_create_state->state_set.state.statictext = p_process_status->reason;
        /* auto-size the reason field sometime */
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_process_status_reflect);
PROC_DIALOG_EVENT_PROTO(static, dialog_event_process_status_reflect)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(t5_message)
    {
    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_process_status_reflect((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    default:
        return(STATUS_OK);
    }
}

#endif /* OS etc. */

extern void
process_status_begin(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_PROCESS_STATUS p_process_status,
    _In_        PROCESS_STATUS_TYPE type)
{
    assert((type == PROCESS_STATUS_PERCENT) || (type == PROCESS_STATUS_TEXT));

    p_process_status->type                 = type;
    p_process_status->docno                = (DOCNO) (p_docu ? docno_from_p_docu(p_docu) : DOCNO_NONE);

    p_process_status->data.percent.cutoff  = 50;

#if RISCOS
    p_process_status->initial_run          = MONOTIMEDIFF_VALUE_FROM_MS(1000);
    p_process_status->granularity          = MONOTIMEDIFF_VALUE_FROM_MS( 100);
#else
    p_process_status->initial_run          = MONOTIMEDIFF_VALUE_FROM_MS( 250);
    p_process_status->granularity          = MONOTIMEDIFF_VALUE_FROM_MS(  25);
#endif

    p_process_status->last_time            = monotime();

    p_process_status->flags.foreground     = 1;
}

extern void
process_status_end(
    _InoutRef_  P_PROCESS_STATUS p_process_status)
{
    if(p_process_status->flags.hourglass_bashed)
    {
        p_process_status->data.percent.current = -1;

        do_actind(p_process_status);
    }

#if WINDOWS && WINDOWS_PROCESS_DIALOG_WINDOW
    if(p_process_status->flags.dialog_is_hwnd && (HOST_WND_NONE != p_process_status->hwnd))
    {
        const HWND hwnd = p_process_status->hwnd;
        p_process_status->flags.dialog_is_hwnd = 0;
        p_process_status->hwnd = 0;
        DestroyWindow(hwnd);
    }
#endif

#if (WINDOWS && !WINDOWS_PROCESS_DIALOG_WINDOW) || (RISCOS && 0)
    if(p_process_status->h_dialog)
    {
        DIALOG_CMD_DISPOSE_DBOX dialog_cmd_dispose_dbox;
        dialog_cmd_dispose_dbox.h_dialog = p_process_status->h_dialog;
        p_process_status->h_dialog = 0;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_DISPOSE_DBOX, &dialog_cmd_dispose_dbox));
    }
#endif

    if(p_process_status->flags.status_line_written)
        if(DOCNO_NONE != p_process_status->docno)
            status_line_clear(p_docu_from_docno(p_process_status->docno), STATUS_LINE_LEVEL_PROCESS_INFO);
}

#if (WINDOWS) || (RISCOS && 0)

extern void
process_status_reflect_create_dialog(
    _InoutRef_  P_PROCESS_STATUS p_process_status)
{
#if TRACE_ALLOWED
    if(p_process_status->type != PROCESS_STATUS_PERCENT)
        trace_0(TRACE_APP_LOAD, TEXT("process_status_reflect: creating dialog because not percentage"));
    else
        trace_2(TRACE_APP_LOAD, TEXT("process_status_reflect: creating dialog because current ") S32_TFMT TEXT(" < cutoff ") S32_TFMT,
                p_process_status->data.percent.current, p_process_status->data.percent.cutoff);
#endif

#if (WINDOWS && !WINDOWS_PROCESS_DIALOG_WINDOW) || (RISCOS && 0)
    /* make copies of the controls' data in this instance of a process dialog */
    p_process_status->controls.reason_data = process_status_reason_data;
    p_process_status->controls.status_data = process_status_status_data;
#endif

    {

#if RISCOS

    static const UI_TEXT caption = { UI_TEXT_TYPE_NONE };
    DIALOG_CTL_CREATE dialog_ctl_create[2];
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;

    dialog_ctl_create[0].p_dialog_control.p_dialog_control = &process_status_reason;
    dialog_ctl_create[0].p_dialog_control_data = &p_process_status->controls.reason_data;
    dialog_ctl_create[1].p_dialog_control.p_dialog_control = &process_status_status;
    dialog_ctl_create[1].p_dialog_control_data = &p_process_status->controls.status_data;

    dialog_cmd_process_dbox_setup_ui_text(&dialog_cmd_process_dbox, dialog_ctl_create, elemof32(dialog_ctl_create), &caption);
    dialog_cmd_process_dbox.bits.modeless = 1;
    dialog_cmd_process_dbox.bits.dialog_position_type = ENUM_PACK(UBF, DIALOG_POSITION_CENTRE_WINDOW);
    dialog_cmd_process_dbox.p_proc_client = dialog_event_process_status_reflect;
    dialog_cmd_process_dbox.client_handle = p_process_status;
    status_assert(object_call_DIALOG_with_docu(p_docu_from_docno(p_process_status->docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));

#elif WINDOWS

    DWORD dwStyle = WS_POPUP | WS_DLGFRAME | WS_VISIBLE;
    RECT screen_rect;
    int x, y;
    int cx = 128;
    int cy = 64;
    HWND hwnd = HOST_WND_NONE;

    if(NULL == g_hInstancePrev)
    {   /* register sk_prost window class */
        static WNDCLASS wndclass;
        if(0 == wndclass.style)
        {
            wndclass.style = CS_SAVEBITS;
            wndclass.lpfnWndProc = wndproc_sk_prost;
          /*wndclass.cbClsExtra = 0;*/
            wndclass.cbWndExtra = sizeof32(LONG);
            wndclass.hInstance = GetInstanceHandle();
          /*wndclass.hIcon = NULL;*/
            wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
            wndclass.hbrBackground = GetStockBrush(LTGRAY_BRUSH);
          /*wndclass.lpszMenuName = NULL;*/
            wndclass.lpszClassName = sk_prost_class_name;
            if(!WrapOsBoolChecking(RegisterClass(&wndclass)))
                return;
        }
    }

    CODE_ANALYSIS_ONLY(zero_struct(screen_rect));
    if(WrapOsBoolChecking(SystemParametersInfo(SPI_GETWORKAREA, 0, &screen_rect, 0)))
    {
        x = screen_rect.left + ((screen_rect.right  - screen_rect.left) - cx) / 2;
        y = screen_rect.top  + ((screen_rect.bottom - screen_rect.top ) - cy) / 2;
        void_WrapOsBoolChecking(HOST_WND_NONE != (
        hwnd =
            CreateWindowEx(
                WS_EX_TOPMOST, sk_prost_class_name, NULL,
                dwStyle,
                x, y, cx, cy,
                NULL /*host_get_icon_hwnd()*/, NULL, GetInstanceHandle(), p_process_status)));
        p_process_status->hwnd = hwnd;
    }

    if(HOST_WND_NONE != hwnd)
    {
        p_process_status->flags.dialog_is_hwnd = 1;
        InvalidateRect(hwnd, NULL, TRUE);
        //SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        ShowWindow(hwnd, SW_SHOWNORMAL);
        UpdateWindow(hwnd);
    }

#endif /* OS */

    } /*block*/
}

#endif

static void
process_status_reflect_update(
    _InoutRef_  P_PROCESS_STATUS p_process_status,
    _InVal_     BOOL viable_status_line)
{
#if WINDOWS && WINDOWS_PROCESS_DIALOG_WINDOW
    if(p_process_status->flags.dialog_is_hwnd)
    {
        const HWND hwnd = p_process_status->hwnd;
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        return;
    }
#endif

#if (WINDOWS && !WINDOWS_PROCESS_DIALOG_WINDOW) || (RISCOS && 0)
    if(p_process_status->h_dialog)
    {
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
        TCHARZ text[16];

        msgclr(dialog_cmd_ctl_state_set);

        switch(p_process_status->type)
        {
        default: default_unhandled();
#if CHECKING
        case PROCESS_STATUS_PERCENT:
#endif
            trace_1(TRACE_APP_LOAD, TEXT("process_status_reflect: dialog with ") S32_TFMT, p_process_status->data.percent.current);
            consume_int(tstr_xsnprintf(text, elemof32(text),
                                       S32_TFMT TEXT("%%"),
                                       p_process_status->data.percent.current));
            dialog_cmd_ctl_state_set.state.statictext.type      = UI_TEXT_TYPE_TSTR_TEMP;
            dialog_cmd_ctl_state_set.state.statictext.text.tstr = text;
            break;

        case PROCESS_STATUS_TEXT:
            dialog_cmd_ctl_state_set.state.statictext = p_process_status->data.text;
            break;
        }

        /* reflect state into dialog */
        dialog_cmd_ctl_state_set.h_dialog = p_process_status->h_dialog;
        dialog_cmd_ctl_state_set.dialog_control_id = PROCESS_STATUS_CONTROL_ID_STATUS;
        dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_UPDATE_NOW;

        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));

        return;
    }
#endif

    if(viable_status_line)
    {   /* a visible status line exists; use that */
        UI_TEXT ui_text;

        /* reflect state into status line */
        if(ui_text_from_process_status(&ui_text, p_process_status))
        {
            status_line_set(p_docu_from_docno(p_process_status->docno), STATUS_LINE_LEVEL_PROCESS_INFO, &ui_text);
            p_process_status->flags.status_line_written = 1;
        }

        ui_text_dispose(&ui_text);

        return;
    }

    /* have to use hourglass */
    switch(p_process_status->type)
    {
    default: default_unhandled();
#if CHECKING
    case PROCESS_STATUS_PERCENT:
#endif
        trace_1(TRACE_APP_LOAD, TEXT("process_status_reflect: hourglass with ") S32_TFMT, p_process_status->data.percent.current);
        do_actind(p_process_status);
        break;

    case PROCESS_STATUS_TEXT:
        break;
    }
}

extern void
process_status_reflect(
    _InoutRef_  P_PROCESS_STATUS p_process_status)
{
    MONOTIMEDIFF dt;
    STATUS viable_status_line;
    STATUS changed;

    dt = monotime_diff(p_process_status->last_time);

    /* allow process to run in foreground for a short time before considering displaying status anywhere */
    if(!p_process_status->flags.initial_run_completed)
    {
        if(dt < p_process_status->initial_run)
            return;

        p_process_status->flags.initial_run_completed = 1;

        trace_2(TRACE_APP_LOAD, TEXT("process_status_reflect: initial_run exceeded ") S32_TFMT TEXT(" > ") S32_TFMT, dt, p_process_status->initial_run);
    }
    else
    {
        if(dt < p_process_status->granularity)
            return;

        trace_2(TRACE_APP_LOAD, TEXT("process_status_reflect: granularity exceeded ") S32_TFMT TEXT(" > ") S32_TFMT, dt, p_process_status->granularity);
    }

#if WINDOWS && 0
    /* force output style */
    p_process_status->flags.foreground = 0;
#endif

#if WINDOWS
    viable_status_line = (DOCNO_NONE != p_process_status->docno) && p_docu_from_docno(p_process_status->docno)->n_views /*&& !p_process_status->flags.foreground*/;
#else
    viable_status_line = (DOCNO_NONE != p_process_status->docno) && p_docu_from_docno(p_process_status->docno)->n_views && !p_process_status->flags.foreground;
#endif

#if WINDOWS || 0
    if(!viable_status_line || p_process_status->flags.dialog)
    {
#if WINDOWS && WINDOWS_PROCESS_DIALOG_WINDOW
        BOOL has_dialog = (HOST_WND_NONE != p_process_status->hwnd);
#else
        BOOL has_dialog = (NULL != p_process_status->h_dialog);
#endif

        if(!has_dialog && !p_process_status->flags.foreground)
            /* if by the time we consider appropriate to pop up the status dialog
             * the process has reached a certain proportion completed then
             * we will just hourglass to the end of the process without giving a dialog
            */
            if((p_process_status->type != PROCESS_STATUS_PERCENT) || (p_process_status->data.percent.current < p_process_status->data.percent.cutoff))
            {
                process_status_reflect_create_dialog(p_process_status);
            }
    }
#else
    assert(!p_process_status->flags.dialog);
#endif

    /* now reflect status iff changed somewhere */

    switch(p_process_status->type)
    {
    default: default_unhandled();
#if CHECKING
    case PROCESS_STATUS_PERCENT:
#endif
        changed = (p_process_status->data.percent.last != p_process_status->data.percent.current);

        p_process_status->data.percent.last = p_process_status->data.percent.current;
        break;

    case PROCESS_STATUS_TEXT:
        changed = 1;
        break;
    }

    if(changed)
        process_status_reflect_update(p_process_status, viable_status_line);

    /* if not foreground jamming then allow other events */
    if(!p_process_status->flags.foreground)
    {
        trace_0(TRACE_APP_LOAD, TEXT("process_status_reflect: going for a fg event"));

        for(;;)
        {
            WM_EVENT res = wm_event_get(TRUE /*fgNullEventsWanted*/);

            if(res == WM_EVENT_PROCESSED)
                continue;

            break;
        }
    }

    p_process_status->last_time = monotime();
}

/* transmit the contents of the status line to interactive help app */

extern void
fill_in_help_request(
    _DocuRef_   P_DOCU p_docu,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer)
{
    tstr_buf[0] = CH_NULL;

    if(0 != array_elements(&p_docu->status_line_info))
    {   /* point at the topmost entry */
        P_STATUS_LINE_ENTRY p_status_line_entry = array_ptr(&p_docu->status_line_info, STATUS_LINE_ENTRY, array_elements(&p_docu->status_line_info) - 1);
        PCTSTR tstr_status_line = ui_text_tstr(&p_status_line_entry->ui_text);

        tstr_xstrkpy(tstr_buf, elemof_buffer, tstr_status_line);

#if RISCOS
        {
        PTSTR tstr;

        if(tstrlen(tstr_status_line) >= elemof_buffer)
        {   /* indicate that it has been truncated at this point */
            tstr_buf[elemof_buffer - 2] = '\x8C'; /* Acorn Extended Latin-1 ELLIPSIS */
        }

        /* Terminate 'sentences' from status line with a 'newline' for interactive help */
        for(tstr = tstr_buf;;)
        {
            PTSTR tstr_next = strchr(tstr, '.');

            if(NULL == tstr_next++)
                break;

            if(CH_SPACE == *tstr_next)
            {
                U32 remaining_chars = tstrlen32(tstr_next + 1); /* what's left after that space */
                *tstr_next++ = CH_VERTICAL_LINE; /* replace the space here */
                /* care with moving remainder of text up */
                if((tstr_next + remaining_chars + 1) == (tstr_buf + elemof_buffer))
                {   /* buffer has been CH_NULL terminated at its last character, which we will preserve, but have to truncate the remainder */
                    memmove(tstr_next + 1, tstr_next, (remaining_chars - 1) * sizeof32(TCHAR)); /* could be r-2 but leave for ease of understanding and debug */
                    tstr_buf[elemof_buffer - 2] = '\x8C'; /* Acorn Extended Latin-1 ELLIPSIS */
                }
                else
                {   /* safe to move remainder up, including the CH_NULL terminator */
                    memmove(tstr_next + 1, tstr_next, (remaining_chars + 1) * sizeof32(TCHAR));
                }
                *tstr_next++ = 'M';
            }

            tstr = tstr_next;
        }
        } /*block*/
#endif /* RISCOS */
    }
}

extern void
status_line_auto_clear(
    _DocuRef_   P_DOCU p_docu)
{
    status_line_clear(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR);
}

extern void
status_line_clear(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STATUS_LINE_LEVEL level)
{
    static const UI_TEXT ui_text = { UI_TEXT_TYPE_NONE };

    status_line_set(p_docu, level, &ui_text);
}

extern void
status_line_set(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STATUS_LINE_LEVEL level,
    _InRef_     PC_UI_TEXT p_ui_text)
{
    /* search for level in list, find insert before if not already present */
    STATUS status;
    P_STATUS_LINE_ENTRY this_p_status_line_entry = NULL;
    ARRAY_INDEX insert_before;
    S32 current_level;

    {
    const ARRAY_INDEX n_elements = array_elements(&p_docu->status_line_info);
    P_STATUS_LINE_ENTRY p_status_line_entry = array_range(&p_docu->status_line_info, STATUS_LINE_ENTRY, 0, n_elements);

    current_level = n_elements ? array_ptr(&p_docu->status_line_info, STATUS_LINE_ENTRY, n_elements - 1)->level : S32_MIN;

    for(insert_before = 0; insert_before < n_elements; ++insert_before, ++p_status_line_entry)
    {
        if(p_status_line_entry->level >= level)
        {
            if(p_status_line_entry->level == level)
                this_p_status_line_entry = p_status_line_entry;

            break;
        }
    }
    } /*block*/

    if(this_p_status_line_entry)
    {
        /* use existing entry */
        if(ui_text_is_blank(p_ui_text))
        {
            ui_text_dispose(&this_p_status_line_entry->ui_text);

            al_array_delete_at(&p_docu->status_line_info, -1, array_indexof_element(&p_docu->status_line_info, STATUS_LINE_ENTRY, this_p_status_line_entry));

            this_p_status_line_entry = NULL;
        }
        else
        {
            if(0 == ui_text_compare(&this_p_status_line_entry->ui_text, p_ui_text, 0, 0))
                /* setting the same text is uninteresting */
                return;

            ui_text_dispose(&this_p_status_line_entry->ui_text);
        }
    }
    else
    {
        /* no entry at this level */
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(*this_p_status_line_entry), 1);

        if(ui_text_is_blank(p_ui_text))
            return;

        /* new entry needed */

        if(NULL != (this_p_status_line_entry = al_array_insert_before(&p_docu->status_line_info, STATUS_LINE_ENTRY, +1, &array_init_block, &status, insert_before)))
            this_p_status_line_entry->level = level;

        status_assert(status);
    }

    if(this_p_status_line_entry)
        if(status_fail(ui_text_copy(&this_p_status_line_entry->ui_text, p_ui_text)))
        {
            al_array_delete_at(&p_docu->status_line_info, -1, array_indexof_element(&p_docu->status_line_info, STATUS_LINE_ENTRY, this_p_status_line_entry));
            return; /* oh well ... */
        }

    /* change not yet significant? */
    if(level < current_level)
        return;

    { /* reflect whatever is now the topmost entry into status line */
    UI_TEXT ui_text;
    const ARRAY_INDEX n_elements = array_elements(&p_docu->status_line_info);

    if(n_elements)
    {
        this_p_status_line_entry = array_ptr(&p_docu->status_line_info, STATUS_LINE_ENTRY, n_elements - 1);
        ui_text = this_p_status_line_entry->ui_text;
    }
    else
        ui_text.type = UI_TEXT_TYPE_NONE;

    status = object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_STATUS_LINE_SET, &ui_text);
    } /*block*/

    status_assert(status);
}

extern void __cdecl
status_line_setf(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STATUS_LINE_LEVEL level,
    _InVal_     STATUS status,
    /**/        ...)
{
    UI_TEXT ui_text;
    UCHARZ array2[BUF_MAX_ERRORSTRING];

    if(!status)
    {
        status_line_clear(p_docu, level);
        return;
    }

    {
    PC_USTR format = resource_lookup_ustr(status);
    va_list args;

    va_start(args, status);
    consume_int(ustr_xvsnprintf(ustr_bptr(array2), elemof32(array2), format, args));
    va_end(args);
    } /*block*/

    ui_text.type = UI_TEXT_TYPE_USTR_TEMP;
    ui_text.text.ustr = ustr_bptr(array2);

    status_line_set(p_docu, level, &ui_text);
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_prost);

_Check_return_
static STATUS
sk_prost_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_docu->status_line_info); ++i)
    {
        const P_STATUS_LINE_ENTRY p_status_line_entry = array_ptr(&p_docu->status_line_info, STATUS_LINE_ENTRY, i);

        ui_text_dispose(&p_status_line_entry->ui_text);
    }

    al_array_dispose(&p_docu->status_line_info);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_sk_prost_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__CLOSE1:
        return(sk_prost_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_prost)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_prost_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static BOOL
ui_text_from_process_status(
    _OutRef_    P_UI_TEXT p_ui_text,
    P_PROCESS_STATUS p_process_status)
{
    PC_USTR ustr_text;
    U32 len_text;
    UCHARZ text_ustr_buf[16];

    p_ui_text->type = UI_TEXT_TYPE_NONE;

    switch(p_process_status->type)
    {
    default: default_unhandled();
#if CHECKING
    case PROCESS_STATUS_PERCENT:
#endif
        consume_int(ustr_xsnprintf(ustr_bptr(text_ustr_buf), elemof32(text_ustr_buf),
                                   USTR_TEXT(S32_FMT "%%"),
                                   p_process_status->data.percent.current));
        ustr_text = ustr_bptr(text_ustr_buf);
        break;

    case PROCESS_STATUS_TEXT:
        ustr_text = ui_text_ustr(&p_process_status->data.text);
        break;
    }

    len_text = ustrlen32(ustr_text);

    if(len_text || !ui_text_is_blank(&p_process_status->reason))
    {
        PC_USTR ustr_reason = ui_text_ustr(&p_process_status->reason);
        U32 len_reason = (P_DATA_NONE != ustr_reason) ? ustrlen32(ustr_reason) : 0;
        U32 len_alloc;
        STATUS status;

        if(len_reason && len_text)
            len_reason += elemof32(" ")-1;

        len_alloc = len_reason + len_text + 1 /*CH_NULL*/;

        p_ui_text->type         = UI_TEXT_TYPE_USTR_ALLOC;
        p_ui_text->text.ustr_wr = al_ptr_alloc_bytes(P_USTR, len_alloc, &status);
        status_assert(status);

        if(NULL != p_ui_text->text.ustr_wr)
        {
            PtrPutByte(p_ui_text->text.ustr_wr, CH_NULL);

            if(len_reason)
                ustr_xstrkat(p_ui_text->text.ustr_wr, len_alloc, ustr_reason);

            if(len_reason && len_text)
                ustr_xstrkat(p_ui_text->text.ustr_wr, len_alloc, USTR_TEXT(" "));

            if(len_text)
                ustr_xstrkat(p_ui_text->text.ustr_wr, len_alloc, ustr_text);
        }
    }

    return(!ui_text_is_blank(p_ui_text));
}

/* end of sk_prost.c */
