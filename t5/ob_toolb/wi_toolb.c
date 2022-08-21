/* wi_toolb.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Toolbar for Fireworkz (Windows code moved from ob_toolb) */

/* SKS Sep 2006 */

#include "common/gflags.h"

#include "ob_toolb/ob_toolb.h"

#if WINDOWS

__pragma(warning(push))
__pragma(warning(disable:4255)) /* no function prototype given: converting '()' to '(void) (Windows SDK 6.0A) */
#include "commctrl.h" /* for TTN_GETDISPINFO */
__pragma(warning(pop))

static void
t5_msg_frame_windows_describe_toolbar(
    _ViewRef_   P_VIEW p_view,
    _InoutRef_  P_MSG_FRAME_WINDOW p_msg_frame_window)
{
    const HOST_WND hwnd = p_view->main[WIN_TOOLBAR].hwnd;

    if(global_preferences.disable_toolbar)
        return;

    if(HOST_WND_NONE != hwnd)
        p_msg_frame_window->gdi_rect.tl.y += toolbar_height_pixels;
}

static void
t5_msg_frame_windows_describe_upper_status_line(
    _ViewRef_   P_VIEW p_view,
    _InoutRef_  P_MSG_FRAME_WINDOW p_msg_frame_window)
{
    const HOST_WND hwnd = p_view->main[WIN_UPPER_STATUS_LINE].hwnd;

    if(global_preferences.disable_status_line)
        return;

    if(HOST_WND_NONE != hwnd)
        p_msg_frame_window->gdi_rect.tl.y += status_line_height_pixels;
}

static void
t5_msg_frame_windows_describe_lower_status_line(
    _ViewRef_   P_VIEW p_view,
    _InoutRef_  P_MSG_FRAME_WINDOW p_msg_frame_window)
{
    const HOST_WND hwnd = p_view->main[WIN_LOWER_STATUS_LINE].hwnd;

    if(global_preferences.disable_status_line)
        return;

    if(HOST_WND_NONE != hwnd)
        p_msg_frame_window->gdi_rect.br.y -= status_line_height_pixels;
}

T5_MSG_PROTO(extern, t5_msg_frame_windows_describe, _InoutRef_ P_MSG_FRAME_WINDOW p_msg_frame_window)
{
    const P_VIEW p_view = p_msg_frame_window->p_view;

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    t5_msg_frame_windows_describe_toolbar(p_view, p_msg_frame_window);

    t5_msg_frame_windows_describe_upper_status_line(p_view, p_msg_frame_window);

    t5_msg_frame_windows_describe_lower_status_line(p_view, p_msg_frame_window);

    return(STATUS_OK);
}

static void
toolbar_posn_user_buttons(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InoutRef_  P_MSG_FRAME_WINDOW p_msg_frame_window)
{
    const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
    ARRAY_INDEX view_row_idx;
    P_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_range(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, 0, view_rows);

    for(view_row_idx = 0; view_row_idx < view_rows; ++view_row_idx, ++p_t5_toolbar_view_row_desc)
    {
        const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
        ARRAY_INDEX row_tool_idx;
        P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_range(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);

        for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++p_t5_toolbar_view_tool_desc)
        {
            const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);
            const PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc;

            switch(ENUM_UNPACK(T5_TOOLBAR_TOOL_TYPE, p_t5_toolbar_tool_desc->bits.type))
            {
            default:
                break;

            case T5_TOOLBAR_TOOL_TYPE_USER:
                {
                if(p_t5_toolbar_view_tool_desc->client_handle)
                {
                    T5_TOOLBAR_TOOL_USER_POSN_SET t5_toolbar_tool_user_posn_set;

                    t5_toolbar_tool_user_posn_set.client_handle = p_t5_toolbar_view_tool_desc->client_handle;
                    t5_toolbar_tool_user_posn_set.p_t5_toolbar_tool_desc = p_t5_toolbar_tool_desc;
                    t5_toolbar_tool_user_posn_set.viewno = viewno_from_p_view(p_view);

                    { /* Ensure clipped to parent client area */
                    const HOST_WND hwnd = p_view->main[WIN_TOOLBAR].hwnd;
                    SIZE PixelsPerInch;

                    host_get_pixel_size(hwnd, &PixelsPerInch); /* Get current pixel size for this window e.g. 96 or 120 */

                    t5_toolbar_tool_user_posn_set.pixit_rect.tl.x = MAX(p_t5_toolbar_view_tool_desc->pixit_rect.tl.x, (p_msg_frame_window->gdi_rect.tl.x * PIXITS_PER_INCH) / PixelsPerInch.cx);
                    t5_toolbar_tool_user_posn_set.pixit_rect.tl.y = MAX(p_t5_toolbar_view_tool_desc->pixit_rect.tl.y, (p_msg_frame_window->gdi_rect.tl.y * PIXITS_PER_INCH) / PixelsPerInch.cy);
                    t5_toolbar_tool_user_posn_set.pixit_rect.br.x = MIN(p_t5_toolbar_view_tool_desc->pixit_rect.br.x, (p_msg_frame_window->gdi_rect.br.x * PIXITS_PER_INCH) / PixelsPerInch.cx);
                    t5_toolbar_tool_user_posn_set.pixit_rect.br.y = MIN(p_t5_toolbar_view_tool_desc->pixit_rect.br.y, (p_msg_frame_window->gdi_rect.br.y * PIXITS_PER_INCH) / PixelsPerInch.cy);
                    } /*block*/

                    status_assert(object_call_id(p_t5_toolbar_tool_desc->command_object_id, p_docu, T5_MSG_TOOLBAR_TOOL_USER_POSN_SET, &t5_toolbar_tool_user_posn_set));
                }

                break;
                }
            }
        }
    }
}

static void
t5_msg_frame_windows_posn_toolbar(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InoutRef_  P_MSG_FRAME_WINDOW p_msg_frame_window)
{
    const HOST_WND hwnd = p_view->main[WIN_TOOLBAR].hwnd;
    RECT client_rect;
    SIZE size;

    GetClientRect(hwnd, &client_rect);

    /* resize to whatever width we should now be */
    size.cx = p_msg_frame_window->gdi_rect.br.x - p_msg_frame_window->gdi_rect.tl.x;
    size.cy = !global_preferences.disable_toolbar ? toolbar_height_pixels : 0;
    SetWindowPos(hwnd, NULL, p_msg_frame_window->gdi_rect.tl.x, p_msg_frame_window->gdi_rect.tl.y, size.cx, size.cy, SWP_NOZORDER);

    if(size.cy && (size.cy != (client_rect.bottom - client_rect.top)))
        InvalidateRect(hwnd, NULL, TRUE);

    if(size.cy)
        toolbar_posn_user_buttons(p_docu, p_view, p_msg_frame_window);

    p_msg_frame_window->gdi_rect.tl.y += size.cy;
}

static void
t5_msg_frame_windows_posn_upper_status_line(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InoutRef_  P_MSG_FRAME_WINDOW p_msg_frame_window)
{
    const HOST_WND hwnd = p_view->main[WIN_UPPER_STATUS_LINE].hwnd;
    RECT client_rect;
    SIZE size;

    IGNOREPARM_DocuRef_(p_docu);

    GetClientRect(hwnd, &client_rect);

    /* resize to whatever width we should now be */
    size.cx = p_msg_frame_window->gdi_rect.br.x - p_msg_frame_window->gdi_rect.tl.x;
    size.cy = !global_preferences.disable_status_line ? status_line_height_pixels : 0;
    SetWindowPos(hwnd, NULL, p_msg_frame_window->gdi_rect.tl.x, p_msg_frame_window->gdi_rect.tl.y, size.cx, size.cy, SWP_NOZORDER);

    if(size.cy && (size.cy != (client_rect.bottom - client_rect.top)))
        InvalidateRect(hwnd, NULL, TRUE);

    p_msg_frame_window->gdi_rect.tl.y += size.cy;
}

static void
t5_msg_frame_windows_posn_lower_status_line(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InoutRef_  P_MSG_FRAME_WINDOW p_msg_frame_window)
{
    const HOST_WND hwnd = p_view->main[WIN_LOWER_STATUS_LINE].hwnd;
    RECT client_rect;
    SIZE size;

    IGNOREPARM_DocuRef_(p_docu);

    GetClientRect(hwnd, &client_rect);

    /* resize to whatever width we should now be */
    size.cx = p_msg_frame_window->gdi_rect.br.x - p_msg_frame_window->gdi_rect.tl.x;
    size.cy = !global_preferences.disable_status_line ? status_line_height_pixels : 0;
    SetWindowPos(hwnd, NULL, p_msg_frame_window->gdi_rect.tl.x, p_msg_frame_window->gdi_rect.br.y - size.cy, size.cx, size.cy, SWP_NOZORDER);

    if(size.cy && (size.cy != (client_rect.bottom - client_rect.top)))
        InvalidateRect(hwnd, NULL, TRUE);

    p_msg_frame_window->gdi_rect.br.y -= size.cy;
}

T5_MSG_PROTO(extern, t5_msg_frame_windows_posn, _InoutRef_ P_MSG_FRAME_WINDOW p_msg_frame_window)
{
    const P_VIEW p_view = p_msg_frame_window->p_view;
    
    IGNOREPARM_InVal_(t5_message);

    if(HOST_WND_NONE != p_view->main[WIN_TOOLBAR].hwnd)
        t5_msg_frame_windows_posn_toolbar(p_docu, p_view, p_msg_frame_window);

    if(HOST_WND_NONE != p_view->main[WIN_UPPER_STATUS_LINE].hwnd)
        t5_msg_frame_windows_posn_upper_status_line(p_docu, p_view, p_msg_frame_window);

    if(HOST_WND_NONE != p_view->main[WIN_LOWER_STATUS_LINE].hwnd)
        t5_msg_frame_windows_posn_lower_status_line(p_docu, p_view, p_msg_frame_window);

    return(STATUS_OK);
}

/*
Upper or Lower Status Line
*/

_Check_return_
static BOOL
wndproc_status_line_onCreate(
    _HwndRef_   HWND hwnd,
    _InRef_     LPCREATESTRUCT lpCreateStruct)
{
    PC_U32 p_u32 = (PC_U32) lpCreateStruct->lpCreateParams;
    U32 packed_id = *p_u32;

    SetProp(hwnd, TYPE5_PROPERTY_WORD, (HANDLE) (UINT_PTR) packed_id);

    return(TRUE);
}

static void
wndproc_status_line_onDestroy(
    _HwndRef_   HWND hwnd)
{
    RemoveProp(hwnd, TYPE5_PROPERTY_WORD);
}

static void
wndproc_status_line_onPaint(
    _HwndRef_   HWND hwnd)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;
    PAINTSTRUCT paintstruct;

    if(!BeginPaint(hwnd, &paintstruct))
        return;
 
    hard_assert(TRUE);

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        paint_status_line_windows(hwnd, &paintstruct, p_docu_from_docno(docno), p_view);
    }

    hard_assert(FALSE);

    EndPaint(hwnd, &paintstruct);
}

static void
wndproc_status_line_onMouseLeave(
    _HwndRef_   HWND hwnd)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    host_clear_tracking_for_window(hwnd);

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        trace_0(TRACE_APP_CLICK, TEXT("wndproc_status_line_onMouseLeave T5_EVENT_POINTER_LEAVES_WINDOW"));
        status_line_clear(p_docu_from_docno(docno), STATUS_LINE_LEVEL_DIALOG_CONTROLS);
        return;
    }

    /* no FORWARD_WM_MOUSELEAVE() */
}

static void
status_line_onMouseMove(
    _HwndRef_   HWND hwnd,
    _DocuRef_   P_DOCU p_docu)
{
    if(host_set_tracking_for_window(hwnd))
    {
        trace_0(TRACE_APP_CLICK, TEXT("status_line_onMouseMove T5_EVENT_POINTER_ENTERS_WINDOW"));
        status_line_clear(p_docu, STATUS_LINE_LEVEL_DIALOG_CONTROLS);
    }

    {
    UI_TEXT ui_text;
    ui_text.type = UI_TEXT_TYPE_RESID;
    ui_text.text.resource_id = MSG_STATUS_STATUS_LINE;
    status_line_set(p_docu, STATUS_LINE_LEVEL_DIALOG_CONTROLS, &ui_text);
    } /*block*/
}

static void
wndproc_status_line_onMouseMove(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        status_line_onMouseMove(hwnd, p_docu_from_docno(docno));
        return;
    }

    FORWARD_WM_MOUSEMOVE(hwnd, x, y, keyFlags, DefWindowProc);
}

static LRESULT CALLBACK
wndproc_status_line(
    _HwndRef_   HWND hwnd,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam)
{
    switch(uiMsg)
    {
    HANDLE_MSG(hwnd, WM_CREATE,     wndproc_status_line_onCreate);
    HANDLE_MSG(hwnd, WM_DESTROY,    wndproc_status_line_onDestroy);

    case WM_ERASEBKGND:
        return(0L); /* just causes us to be called at WM_PAINT with background to still be painted */

    HANDLE_MSG(hwnd, WM_PAINT,      wndproc_status_line_onPaint);
    HANDLE_MSG(hwnd, WM_MOUSELEAVE, wndproc_status_line_onMouseLeave);
    HANDLE_MSG(hwnd, WM_MOUSEMOVE,  wndproc_status_line_onMouseMove);

    default:
        return(DefWindowProc(hwnd, uiMsg, wParam, lParam));
    }
}

_Check_return_
extern STATUS
status_line_register_wndclass(void)
{
    if(NULL == g_hInstancePrev)
    {   /* register a task-owned class for either upper or lower status line */
        WNDCLASS wndclass;
        zero_struct(wndclass);
        wndclass.style = CS_HREDRAW;
        wndclass.lpfnWndProc = (WNDPROC) wndproc_status_line;
      /*wndclass.cbClsExtra = 0;*/
      /*wndclass.cbWndExtra = 0;*/
        wndclass.hInstance = GetInstanceHandle();
      /*wndclass.hIcon = 0;*/
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH) (UINT_PTR) (COLOR_WINDOW + 1); /* we paint the lot */
      /*wndclass.lpszMenuName = NULL;*/
        wndclass.lpszClassName = window_class[APP_WINDOW_CLASS_STATUS_LINE];
        if(!WrapOsBoolChecking(RegisterClass(&wndclass)))
            return(status_nomem());
    }

    return(STATUS_OK);
}

/*
Toolbar
*/

#define TOOLBAR_TOOLTIPS 1

#if defined(TOOLBAR_TOOLTIPS)

_Check_return_
static HWND
CreateToolTip(
    _HwndRef_   HWND hwndParent)
{
    HWND hwndTT = NULL;

    { /* get the common controls we need */
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof32(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TAB_CLASSES;
    if(!WrapOsBoolChecking(InitCommonControlsEx(&icex)))
        return(hwndTT);
    } /*block*/

    /* create a ToolTip window */
    void_WrapOsBoolChecking(HOST_WND_NONE != (
    hwndTT =
        CreateWindowEx(WS_EX_TRANSPARENT,
            TOOLTIPS_CLASS, NULL,
            TTS_NOPREFIX,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            hwndParent, NULL, GetInstanceHandle(), NULL)));

    if(hwndTT)
    {   /* set up "tool" information */
        TTTOOLINFO ti = { 0 };
        ti.cbSize = sizeof32(ti);
        ti.uFlags = TTF_TRANSPARENT;
        ti.hwnd = hwndParent;
        ti.hinst = GetInstanceHandle();
        ti.lpszText = LPSTR_TEXTCALLBACK;
        /* NB in this case, the "tool" is the entire parent window */
        SetRectEmpty(&ti.rect);
        SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
    }

    return(hwndTT);
}

static void
toolbar_set_tooltip_text(
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_UI_TEXT p_ui_text)
{
    if(0 == ui_text_compare(&p_view->toolbar.ui_text, p_ui_text, 0, 0))
        /* setting the same text is uninteresting */
        return;

    if(NULL != p_view->toolbar.hwndTT)
    {
        ui_text_dispose(&p_view->toolbar.ui_text);
        status_assert(ui_text_copy(&p_view->toolbar.ui_text, p_ui_text));
        SendMessage(p_view->toolbar.hwndTT, TTM_POP, 0, 0);
    }
}

#else /* NOT TOOLBAR_TOOLTIPS */

static inline void
toolbar_set_tooltip_text(
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_UI_TEXT p_ui_text)
{
    IGNOREPARM_ViewRef_(p_view);
    IGNOREPARM_InRef_(p_ui_text);
}

#endif /* TOOLBAR_TOOLTIPS */

static void
toolbar_onCreate(
    _HwndRef_   HWND hwnd,
    _ViewRef_   P_VIEW p_view)
{
#if defined(TOOLBAR_TOOLTIPS)
    p_view->toolbar.hwndTT = CreateToolTip(hwnd);
#else
    IGNOREPARM_HwndRef_(hwnd);
    IGNOREPARM_ViewRef_(p_view);
#endif
}

_Check_return_
static BOOL
wndproc_toolbar_onCreate(
    _HwndRef_   HWND hwnd,
    _InRef_     LPCREATESTRUCT lpCreateStruct)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;
    PC_U32 p_u32 = (PC_U32) lpCreateStruct->lpCreateParams;
    U32 packed_id = *p_u32;

    SetProp(hwnd, TYPE5_PROPERTY_WORD, (HANDLE) (UINT_PTR) packed_id);

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        toolbar_onCreate(hwnd, p_view);
        return(TRUE);
    }

    return(TRUE);
}

static void
wndproc_toolbar_onDestroy(
    _HwndRef_   HWND hwnd)
{
    RemoveProp(hwnd, TYPE5_PROPERTY_WORD);
}

static void
wndproc_toolbar_onPaint(
    _HwndRef_   HWND hwnd)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;
    PAINTSTRUCT paintstruct;

    if(!BeginPaint(hwnd, &paintstruct))
        return;
 
    hard_assert(TRUE);

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        paint_toolbar_windows(hwnd, &paintstruct, p_docu_from_docno(docno), p_view);
    }

    hard_assert(FALSE);

    EndPaint(hwnd, &paintstruct);
}

static void
toolbar_onSize(
    _HwndRef_   HWND hwnd,
    _ViewRef_   P_VIEW p_view)
{
    if(NULL != p_view->toolbar.hwndTT)
    {
        TTTOOLINFO ti = { sizeof32(ti) };
        ti.hwnd = hwnd;
        ti.uId = 0;
        GetClientRect(hwnd, &ti.rect);
        SendMessage(p_view->toolbar.hwndTT, TTM_NEWTOOLRECT, 0, (LPARAM) &ti);
    }
}

static void
wndproc_toolbar_onSize(
    _HwndRef_   HWND hwnd,
    _InVal_     UINT state,
    _InVal_     int cx,
    _InVal_     int cy)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        toolbar_onSize(hwnd, p_view);
        /* pass on */
    }

    FORWARD_WM_SIZE(hwnd, state, cx, cy, DefWindowProc);
}

static void
toolbar_onLButtonDown(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     BOOL right_button_down)
{
    PIXIT_POINT pixit_point;
    P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc;

    {
    GDI_POINT gdi_point;
    gdi_point.x = x;
    gdi_point.y = y;
    pixit_point_from_window_point(&pixit_point, &gdi_point, &p_view->host_xform[XFORM_TOOLBAR]);
    } /*block*/

    status_assert(host_key_cache_emit_events(DOCNO_NONE));

    p_t5_toolbar_view_tool_desc = tool_from_point(p_view, &pixit_point);

    if(NULL != p_t5_toolbar_view_tool_desc)
    {
        const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);

        if(tool_enabled_query(p_t5_toolbar_docu_tool_desc))
        {
            if(p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc->bits.immediate)
            {
                status_line_clear(p_docu, STATUS_LINE_LEVEL_BACKWINDOW_CONTROLS);
                execute_tool(p_docu, p_view, p_t5_toolbar_docu_tool_desc, right_button_down || host_ctrl_pressed());
                return;
            }

            p_t5_toolbar_view_tool_desc->had_mouse_down = 1;
            p_t5_toolbar_view_tool_desc->button_down = 2;
            redraw_tool(p_docu, p_view, p_t5_toolbar_view_tool_desc);
            SetCapture(hwnd);
            /* and wait for the WM_LBUTTONUP */
        }
    }
}

static void
wndproc_toolbar_onLButtonDown(
    _HwndRef_   HWND hwnd,
    _InVal_     BOOL fDoubleClick,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        toolbar_onLButtonDown(hwnd, x, y, p_docu_from_docno(docno), p_view, FALSE);
        return;
    }

    FORWARD_WM_LBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, DefWindowProc);
}

static void
wndproc_toolbar_onRButtonDown(
    _HwndRef_   HWND hwnd,
    _InVal_     BOOL fDoubleClick,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        toolbar_onLButtonDown(hwnd, x, y, p_docu_from_docno(docno), p_view, TRUE);
        return;
    }

    FORWARD_WM_RBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, DefWindowProc);
}

static void
toolbar_onLButtonUp(
    _InVal_     int x,
    _InVal_     int y,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     BOOL right_button_up)
{
    PIXIT_POINT pixit_point;
    P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc;

    ReleaseCapture();

    {
    GDI_POINT gdi_point;
    gdi_point.x = x;
    gdi_point.y = y;
    pixit_point_from_window_point(&pixit_point, &gdi_point, &p_view->host_xform[XFORM_TOOLBAR]);
    } /*block*/

    p_t5_toolbar_view_tool_desc = tool_from_point(p_view, &pixit_point);

    if(NULL != p_t5_toolbar_view_tool_desc)
        /* only act on mouse up event for the tool that had the mouse down */
        if(!p_t5_toolbar_view_tool_desc->had_mouse_down)
            p_t5_toolbar_view_tool_desc = NULL;

    { /* ensure all buttons out of tracking mouse down state */
    const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
    ARRAY_INDEX view_row_idx;
    P_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_range(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, 0, view_rows);

    for(view_row_idx = 0; view_row_idx < view_rows; ++view_row_idx, ++p_t5_toolbar_view_row_desc)
    {
        const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
        ARRAY_INDEX row_tool_idx;
        P_T5_TOOLBAR_VIEW_TOOL_DESC this_p_t5_toolbar_view_tool_desc = array_range(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);

        for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++this_p_t5_toolbar_view_tool_desc)
        {
            BOOL redraw = FALSE;

            if(this_p_t5_toolbar_view_tool_desc->had_mouse_down)
            {
                this_p_t5_toolbar_view_tool_desc->had_mouse_down = 0;
                redraw = TRUE;
            }

            if(0 != this_p_t5_toolbar_view_tool_desc->button_down)
            {
                this_p_t5_toolbar_view_tool_desc->button_down = 0;
                redraw = TRUE;
            }

            if(redraw)
                redraw_tool(p_docu, p_view, this_p_t5_toolbar_view_tool_desc);
        }
    }
    } /*block*/

    if(NULL != p_t5_toolbar_view_tool_desc)
    {
        const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);
        status_line_clear(p_docu, STATUS_LINE_LEVEL_BACKWINDOW_CONTROLS);
        execute_tool(p_docu, p_view, p_t5_toolbar_docu_tool_desc, right_button_up || host_ctrl_pressed());
        return;
    }
}

static void
wndproc_toolbar_onLButtonUp(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        toolbar_onLButtonUp(x, y, p_docu_from_docno(docno), p_view, FALSE);
        return;
    }

    FORWARD_WM_LBUTTONUP(hwnd, x, y, keyFlags, DefWindowProc);
}

static void
wndproc_toolbar_onRButtonUp(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        toolbar_onLButtonUp(x, y, p_docu_from_docno(docno), p_view, TRUE);
        return;
    }

    FORWARD_WM_RBUTTONUP(hwnd, x, y, keyFlags, DefWindowProc);
}

static void
wndproc_toolbar_onMouseLeave(
    _HwndRef_   HWND hwnd)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    host_clear_tracking_for_window(hwnd);

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        static const UI_TEXT ui_text = { UI_TEXT_TYPE_NONE };
        trace_0(TRACE_APP_CLICK, TEXT("wndproc_toolbar_onMouseLeave T5_EVENT_POINTER_LEAVES_WINDOW"));
        status_line_clear(p_docu_from_docno(docno), STATUS_LINE_LEVEL_DIALOG_CONTROLS);
        toolbar_set_tooltip_text(p_view, &ui_text);
        return;
    }

    /* no FORWARD_WM_MOUSELEAVE() */
}

_Check_return_
_Ret_maybenull_
static P_T5_TOOLBAR_VIEW_TOOL_DESC
toolbar_onMouseMove_find_view_tool_desc(
    _InVal_     int x,
    _InVal_     int y,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    PIXIT_POINT pixit_point;
    P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc;

    {
    GDI_POINT gdi_point;
    gdi_point.x = x;
    gdi_point.y = y;
    pixit_point_from_window_point(&pixit_point, &gdi_point, &p_view->host_xform[XFORM_TOOLBAR]);
    } /*block*/

    p_t5_toolbar_view_tool_desc = tool_from_point(p_view, &pixit_point);

    if(NULL != p_t5_toolbar_view_tool_desc)
    {
        const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);
        PC_UI_TEXT p_ui_text = &p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc->ui_text;
        if(NULL != p_view->toolbar.hwndTT)
            toolbar_set_tooltip_text(p_view, p_ui_text);
        else
            status_line_set(p_docu, STATUS_LINE_LEVEL_DIALOG_CONTROLS, p_ui_text);
    }
    else
    {
        static const UI_TEXT ui_text = { UI_TEXT_TYPE_NONE };
        status_line_clear(p_docu, STATUS_LINE_LEVEL_DIALOG_CONTROLS);
        toolbar_set_tooltip_text(p_view, &ui_text);
    }

    return(p_t5_toolbar_view_tool_desc);
}

static void
toolbar_onMouseMove_update_buttons(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_opt_ P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc)
{
    const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
    ARRAY_INDEX view_row_idx;
    P_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_range(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, 0, view_rows);

    for(view_row_idx = 0; view_row_idx < view_rows; ++view_row_idx, ++p_t5_toolbar_view_row_desc)
    {
        const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
        ARRAY_INDEX row_tool_idx;
        P_T5_TOOLBAR_VIEW_TOOL_DESC this_p_t5_toolbar_view_tool_desc = array_range(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);

        for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++this_p_t5_toolbar_view_tool_desc)
        {
            UINT required_state = 0;
            PTR_ASSERT(this_p_t5_toolbar_view_tool_desc);

            if(p_t5_toolbar_view_tool_desc == this_p_t5_toolbar_view_tool_desc)
            {
                if(this_p_t5_toolbar_view_tool_desc->had_mouse_down)
                    required_state = 2;
                else
                    required_state = 1;
            }
            else if(this_p_t5_toolbar_view_tool_desc->had_mouse_down)
                required_state = 1;
            else
                required_state = 0;

            if(required_state != this_p_t5_toolbar_view_tool_desc->button_down)
            {
                BOOL redraw = TRUE;

                if((0 == required_state) && (1 == this_p_t5_toolbar_view_tool_desc->button_down))
                    redraw = UIToolButtonMouseOverSignificant();

                if((1 == required_state) && (0 == this_p_t5_toolbar_view_tool_desc->button_down))
                    redraw = UIToolButtonMouseOverSignificant();

                this_p_t5_toolbar_view_tool_desc->button_down = required_state;

                if(redraw)
                    redraw_tool(p_docu, p_view, this_p_t5_toolbar_view_tool_desc);
            }
        }
    }
}

static void
toolbar_onMouseMove(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc;

    if(host_set_tracking_for_window(hwnd))
    {
        trace_0(TRACE_APP_CLICK, TEXT("toolbar_onMouseMove T5_EVENT_POINTER_ENTERS_WINDOW"));
        status_line_clear(p_docu, STATUS_LINE_LEVEL_DIALOG_CONTROLS);
    }

    p_t5_toolbar_view_tool_desc = toolbar_onMouseMove_find_view_tool_desc(x, y, p_docu, p_view);

    toolbar_onMouseMove_update_buttons(p_docu, p_view, p_t5_toolbar_view_tool_desc);
}

static void
wndproc_toolbar_onMouseMove(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        toolbar_onMouseMove(hwnd, x, y, p_docu_from_docno(docno), p_view);
        return;
    }

    FORWARD_WM_MOUSEMOVE(hwnd, x, y, keyFlags, DefWindowProc);
}

#if defined(TOOLBAR_TOOLTIPS)

static void
toolbar_onTTNGetDispInfo(
    _Inout_     NMTTDISPINFO * const nmttdispinfo,
    _ViewRef_   PC_VIEW p_view)
{
    if(nmttdispinfo->hdr.hwndFrom != p_view->toolbar.hwndTT)
        return;

    if(!ui_text_is_blank(&p_view->toolbar.ui_text))
        tstr_xstrkpy(nmttdispinfo->szText, elemof32(nmttdispinfo->szText), ui_text_tstr(&p_view->toolbar.ui_text));
    else
        nmttdispinfo->szText[0] = CH_NULL;

    nmttdispinfo->lpszText = nmttdispinfo->szText;
}

static LRESULT
wndproc_toolbar_onTTNGetDispInfo(
    _HwndRef_   HWND hwnd,
    _Inout_     NMTTDISPINFO * const nmttdispinfo)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        toolbar_onTTNGetDispInfo(nmttdispinfo, p_view);
        return(0L);
    }

    return(0L);
}

static LRESULT
wndproc_toolbar_onNotify(
    _HwndRef_   HWND hwnd,
    _InVal_     int idFrom,
    _In_        NMHDR * pnm)
{
    IGNOREPARM_InVal_(idFrom);

    switch(pnm->code)
    {
    case TTN_GETDISPINFO:
        return(wndproc_toolbar_onTTNGetDispInfo(hwnd, (NMTTDISPINFO *) pnm));

    default:
        return(0L);
    }
}

static void
RelayMouseEvent(
    _HwndRef_   HWND hwnd,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        if(NULL != p_view->toolbar.hwndTT)
        {
            MSG msg;
            msg.hwnd = hwnd;
            msg.message = uiMsg;
            msg.wParam = wParam;
            msg.lParam = lParam;
            SendMessage(p_view->toolbar.hwndTT, TTM_RELAYEVENT, 0, (LPARAM) &msg);
        }
    }
}

#endif /* TOOLBAR_TOOLTIPS */

static LRESULT CALLBACK
wndproc_toolbar(
    _HwndRef_   HWND hwnd,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam)
{
#if defined(TOOLBAR_TOOLTIPS)
    if(((uiMsg >= WM_MOUSEFIRST) && (uiMsg <= WM_MOUSELAST)) || (uiMsg == WM_NCMOUSEMOVE))
    {
        RelayMouseEvent(hwnd, uiMsg, wParam, lParam);
    }
#endif

    switch(uiMsg)
    {
    HANDLE_MSG(hwnd, WM_CREATE,         wndproc_toolbar_onCreate);
    HANDLE_MSG(hwnd, WM_DESTROY,        wndproc_toolbar_onDestroy);

    case WM_ERASEBKGND:
        return(0L); /* just causes us to be called at WM_PAINT with background to still be painted */

    HANDLE_MSG(hwnd, WM_PAINT,          wndproc_toolbar_onPaint);
    HANDLE_MSG(hwnd, WM_SIZE,           wndproc_toolbar_onSize);
    HANDLE_MSG(hwnd, WM_LBUTTONDOWN,    wndproc_toolbar_onLButtonDown);
    HANDLE_MSG(hwnd, WM_LBUTTONUP,      wndproc_toolbar_onLButtonUp);
    HANDLE_MSG(hwnd, WM_RBUTTONDOWN,    wndproc_toolbar_onRButtonDown);
    HANDLE_MSG(hwnd, WM_RBUTTONUP,      wndproc_toolbar_onRButtonUp);
    HANDLE_MSG(hwnd, WM_MOUSELEAVE,     wndproc_toolbar_onMouseLeave);
    HANDLE_MSG(hwnd, WM_MOUSEMOVE,      wndproc_toolbar_onMouseMove);

#if defined(TOOLBAR_TOOLTIPS)
    HANDLE_MSG(hwnd, WM_NOTIFY,         wndproc_toolbar_onNotify);
#endif

    default:
        return(DefWindowProc(hwnd, uiMsg, wParam, lParam));
    }
}

_Check_return_
extern STATUS
toolbar_register_wndclass(void)
{
    if(NULL == g_hInstancePrev)
    {   /* register a task-owned class for toolbar */
        WNDCLASS wndclass;
        zero_struct(wndclass);
      /*wndclass.style = 0;*/
        wndclass.lpfnWndProc = (WNDPROC) wndproc_toolbar;
      /*wndclass.cbClsExtra = 0;*/
      /*wndclass.cbWndExtra = 0;*/
        wndclass.hInstance = GetInstanceHandle();
      /*wndclass.hIcon = 0;*/
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH) (UINT_PTR) (COLOR_WINDOW + 1); /* we paint the lot */
      /*wndclass.lpszMenuName = NULL;*/
        wndclass.lpszClassName = window_class[APP_WINDOW_CLASS_TOOLBAR];
        if(!WrapOsBoolChecking(RegisterClass(&wndclass)))
            return(status_nomem());
    }

    return(STATUS_OK);
}

#endif /* WINDOWS */

/* end of wi_toolb.c */
