/* windows/splash.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#if WINDOWS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#define APP_RESOURCE_BANNER_BITMAP  80

enum SPLASH_TIMER_IDS
{
    TIMER_ID_REMOVE,
    TIMER_MAX
};

#define SPLASH_WINDOW_TIMER_TIMEOUT 18 * 100  /* up for ~2 seconds */

static BOOL g_h_splash_class_created;

static HBITMAP g_h_bitmap_banner;

static struct tagBITMAP g_banner_bitmap;

static LOGFONT g_logfont_splash;

static HFONT g_h_font_splash;

static HWND hwnd_splash;

static int g_extra_text_pixels_y;

static void
splash_onPaint_banner(
    _InRef_     PPAINTSTRUCT p_paintstruct,
    _OutRef_    P_GDI_SIZE p_size)
{
    const HDC hdcDisplay = p_paintstruct->hdc;
    const HDC hdcMem = CreateCompatibleDC(hdcDisplay);
    HBITMAP hbmOld = SelectBitmap(hdcMem, g_h_bitmap_banner);
    GDI_COORD x, y, w, h, xm, ym, wm, hm;

    p_size->cx = g_banner_bitmap.bmWidth; /* Find out ye size of ye bitmap */
    p_size->cy = g_banner_bitmap.bmHeight;

    /* source */
    xm = 0;
    ym = 0;
    wm = p_size->cx;
    hm = p_size->cy;

    { /* DPI-aware */
    GDI_SIZE PixelsPerInch;
    host_get_pixel_size(NULL /*screen*/, &PixelsPerInch.cx, &PixelsPerInch.cy); /* Get current pixel size for the screen e.g. 96 or 120 */

    p_size->cx = idiv_ceil_u(p_size->cx * PixelsPerInch.cx, 96);
    p_size->cy = idiv_ceil_u(p_size->cy * PixelsPerInch.cy, 96);
    } /*block*/

    /* dest */
    x = 0;
    y = 0;
    w = p_size->cx;
    h = p_size->cy;

#if 0
    {
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255 /*alpha*/, AC_SRC_ALPHA };
    void_WrapOsBoolChecking(AlphaBlend(hdcDisplay, x, y, w, h, hdcMem, xm, ym, wm, hm, bf));
    } /*block*/
#else
    void_WrapOsBoolChecking(StretchBlt(hdcDisplay, x, y, w, h, hdcMem, xm, ym, wm, hm, SRCCOPY));
#endif

    SelectBitmap(hdcMem, hbmOld);
    DeleteDC(hdcMem);
}

#define SPLASH_LEFT_MARGIN_PIXELS   4
#define SPLASH_RIGHT_MARGIN_PIXELS  SPLASH_LEFT_MARGIN_PIXELS
#define SPLASH_BOTTOM_MARGIN_PIXELS 6
#define SPLASH_INTER_LINE_SPACING   4

static void
splash_onPaint_text(
    _In_        const PAINTSTRUCT * const p_paintstruct,
    _InRef_     PC_GDI_SIZE p_size)
{
    const HDC hdc = p_paintstruct->hdc;
    PCTSTR tstr;
    HFONT h_font_old = SelectFont(hdc, g_h_font_splash);
    int text_y_0, text_y_1, text_y_2, text_y_inter_line, bottom_margin_pixels;
    int user_info_height = -g_logfont_splash.lfHeight; /* hopefully negative */

    if(user_info_height < 8) /* now negative or too small? */
        user_info_height = 16;

    text_y_inter_line = user_info_height;

    { /* DPI-aware */
    GDI_SIZE PixelsPerInch;
    host_get_pixel_size(NULL /*screen*/, &PixelsPerInch.cx, &PixelsPerInch.cy); /* Get current pixel size for the screen e.g. 96 or 120 */

    text_y_inter_line   += idiv_ceil_u(SPLASH_INTER_LINE_SPACING   * PixelsPerInch.cy, 96);
    bottom_margin_pixels = idiv_ceil_u(SPLASH_BOTTOM_MARGIN_PIXELS * PixelsPerInch.cy, 96);
    } /*block*/

    text_y_0 = p_size->cy - (user_info_height + bottom_margin_pixels);  /* bottom line */
    text_y_1 = text_y_0   - text_y_inter_line;                          /* up one line from bottom */
    text_y_2 = text_y_1   - text_y_inter_line;                          /* up two lines from bottom */

    SetBkMode(hdc, TRANSPARENT);

    tstr = user_id();
    if(CH_NULL != tstr[0])
    {
        SetTextAlign(hdc, TA_LEFT);
        void_WrapOsBoolChecking(
            ExtTextOut(hdc,
                       SPLASH_LEFT_MARGIN_PIXELS, text_y_2,
                       0, NULL,
                       tstr, (UINT) tstrlen32(tstr), NULL));
    }

    tstr = user_organ_id();
    if(CH_NULL != tstr[0])
    {
        SetTextAlign(hdc, TA_LEFT);
        void_WrapOsBoolChecking(
            ExtTextOut(hdc,
                       SPLASH_LEFT_MARGIN_PIXELS, text_y_1,
                       0, NULL,
                       tstr, (UINT) tstrlen32(tstr), NULL));
    }

    {
    QUICK_TBLOCK_WITH_BUFFER(version_quick_tblock, 64);
    quick_tblock_with_buffer_setup(version_quick_tblock);

    tstr = NULL;
    if(status_ok(resource_lookup_quick_tblock(&version_quick_tblock, MSG_SKEL_VERSION)))
    {
#if WINDOWS && defined(_M_X64)
        if(status_ok(quick_tblock_tstr_add(&version_quick_tblock, TEXT(" 64-bit"))))
#endif
        if(status_ok(quick_tblock_tchar_add(&version_quick_tblock, CH_SPACE)))
        if(status_ok(quick_tblock_tchar_add(&version_quick_tblock, UCH_LEFT_PARENTHESIS)))
        if(status_ok(resource_lookup_quick_tblock(&version_quick_tblock, MSG_SKEL_DATE)))
        if(status_ok(quick_tblock_tchar_add(&version_quick_tblock, UCH_RIGHT_PARENTHESIS)))
        if(status_ok(quick_tblock_nullch_add(&version_quick_tblock)))
            tstr = quick_tblock_tstr(&version_quick_tblock);
    }
    if(NULL == tstr)
        tstr = resource_lookup_tstr(MSG_SKEL_VERSION);

    SetTextAlign(hdc, TA_RIGHT);
    void_WrapOsBoolChecking(
        ExtTextOut(hdc,
                   p_size->cx - SPLASH_RIGHT_MARGIN_PIXELS, text_y_2,
                   0, NULL,
                   tstr, (UINT) tstrlen32(tstr), NULL));

    quick_tblock_dispose(&version_quick_tblock);
    } /*block*/

    tstr = resource_lookup_tstr(MSG_COPYRIGHT);

    SetTextAlign(hdc, TA_RIGHT);
    void_WrapOsBoolChecking(
        ExtTextOut(hdc,
                   p_size->cx - SPLASH_RIGHT_MARGIN_PIXELS, text_y_1,
                   0, NULL,
                   tstr, (UINT) tstrlen32(tstr), NULL));

    tstr = resource_lookup_tstr(MSG_DIALOG_INFO_WEB_URL);

    SetTextAlign(hdc, TA_RIGHT);
    void_WrapOsBoolChecking(
        ExtTextOut(hdc,
                   p_size->cx - SPLASH_RIGHT_MARGIN_PIXELS, text_y_0,
                   0, NULL,
                   tstr, (UINT) tstrlen32(tstr), NULL));

    if(0 != g_extra_text_pixels_y)
    {   /* clear the bottom rectangle */
        int text_y = text_y_0 + text_y_inter_line;
        RECT rect;
        rect.left = 0;
        rect.top = p_size->cy;
        rect.right = p_size->cx;
        rect.bottom = rect.top + g_extra_text_pixels_y;
        FillRect(hdc, &rect, GetStockBrush(WHITE_BRUSH));

        tstr = resource_lookup_tstr(MSG_USES_COMPONENTS);

        SetTextAlign(hdc, TA_RIGHT);
        void_WrapOsBoolChecking(
            ExtTextOut(hdc,
                       p_size->cx - SPLASH_RIGHT_MARGIN_PIXELS, text_y,
                       0, NULL,
                       tstr, (UINT) tstrlen32(tstr), NULL));

        text_y += text_y_inter_line;

        tstr = resource_lookup_tstr(MSG_PORTIONS_DIAL_SOLUTIONS);

        SetTextAlign(hdc, TA_RIGHT);
        void_WrapOsBoolChecking(
            ExtTextOut(hdc,
                       p_size->cx - SPLASH_RIGHT_MARGIN_PIXELS, text_y,
                       0, NULL,
                       tstr, (UINT) tstrlen32(tstr), NULL));

        text_y += text_y_inter_line;
    }

    consume(HFONT, SelectFont(hdc, h_font_old));
}

static void
splash_onPaint(
    _InRef_     PPAINTSTRUCT p_paintstruct)
{
    const HDC hdc = p_paintstruct->hdc;
    HPALETTE h_palette_old;
    GDI_SIZE size = { 0, 0 };

    host_select_default_palette(hdc, &h_palette_old);

    if(NULL != g_h_bitmap_banner)
        splash_onPaint_banner(p_paintstruct, &size);

    splash_onPaint_text(p_paintstruct, &size);

    host_select_old_palette(hdc, &h_palette_old);
}

static void
wndproc_splash_onPaint(
    _HwndRef_   HWND hwnd)
{
    PAINTSTRUCT paintstruct;

    if(!BeginPaint(hwnd, &paintstruct))
        return;

    hard_assert(TRUE);

    splash_onPaint(&paintstruct);

    hard_assert(FALSE);

    EndPaint(hwnd, &paintstruct);
}

static void
wndproc_splash_onLButtonUp(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    UNREFERENCED_PARAMETER_HwndRef_(hwnd);
    UNREFERENCED_PARAMETER_InVal_(x);
    UNREFERENCED_PARAMETER_InVal_(y);
    UNREFERENCED_PARAMETER_InVal_(keyFlags);

    splash_window_remove();
}

static void
wndproc_splash_onTimer(
    _HwndRef_   HWND hwnd,
    _In_        UINT id)
{
    trace_1(TRACE_WINDOWS_HOST, TEXT("WM_TIMER(splash): timer id %u"), id);

    switch(id)
    {
    case TIMER_ID_REMOVE:
        splash_window_remove();
        return;

    default:
        FORWARD_WM_TIMER(hwnd, id, DefWindowProc);
        return;
    }
}

_Check_return_
extern LRESULT CALLBACK
wndproc_splash(
    _HwndRef_   HWND hwnd,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam)
{
    switch(uiMsg)
    {
    HANDLE_MSG(hwnd, WM_PAINT, wndproc_splash_onPaint);
    HANDLE_MSG(hwnd, WM_LBUTTONUP, wndproc_splash_onLButtonUp);
    HANDLE_MSG(hwnd, WM_TIMER, wndproc_splash_onTimer);

    default:
        return(DefWindowProc(hwnd, uiMsg, wParam, lParam));
    }
}

/* Load the title screen bitmap and show a window central on the screen
 * showing the bitmap whilst the application starts.
 * This window will be removed after a given timeout.
 */

extern HBITMAP
gdiplus_load_bitmap_from_file(
    _In_z_      PCWSTR wstr_filename);

extern void
splash_window_create(
    _HwndRef_opt_ HWND hwndParent,
    _In_        UINT splash_timeout_ms)
{
    RECT screen_rect;
    GDI_SIZE size = { 0, 0 };
    GDI_SIZE PixelsPerInch; /* DPI-aware */
    host_get_pixel_size(NULL /*screen*/, &PixelsPerInch.cx, &PixelsPerInch.cy); /* Get current pixel size for the screen e.g. 96 or 120 */

    if(0 == splash_timeout_ms)
    {
        TCHARZ buffer[16];

        if(0 != MyGetProfileString(TEXT("SplashTimeoutMS"), tstr_empty_string, buffer, elemof32(buffer)))
        {
            splash_timeout_ms = _tstoi(buffer); /*atoi*/
            if( splash_timeout_ms > SPLASH_WINDOW_TIMER_TIMEOUT)
                splash_timeout_ms = SPLASH_WINDOW_TIMER_TIMEOUT;
        }
        else
            splash_timeout_ms = SPLASH_WINDOW_TIMER_TIMEOUT;
    } /*block*/

    if(splash_timeout_ms < USER_TIMER_MINIMUM)
        return;

    if((NULL == g_hInstancePrev) && !g_h_splash_class_created)
    {   /* register splash window class */
        WNDCLASS wndclass;
        zero_struct(wndclass);
      /*wndclass.style = 0;*/
        wndclass.lpfnWndProc = (WNDPROC) wndproc_splash;
      /*wndclass.cbClsExtra = 0;*/
      /*wndclass.cbWndExtra = 0;*/
        wndclass.hInstance = GetInstanceHandle();
      /*wndclass.hIcon = NULL;*/
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
#if 0
        wndclass.hbrBackground = GetStockBrush(WHITE_BRUSH);
#else
        wndclass.hbrBackground = GetStockBrush(HOLLOW_BRUSH);
#endif
      /*wndclass.lpszMenuName = NULL;*/
        wndclass.lpszClassName = window_class[APP_WINDOW_CLASS_SPLASH];
        if(!WrapOsBoolChecking(RegisterClass(&wndclass)))
            return;
        g_h_splash_class_created = TRUE;
    }

    if(!WrapOsBoolChecking(SystemParametersInfo(SPI_GETWORKAREA, 0, &screen_rect, 0)))
        return;

#if 0
    g_h_bitmap_banner = gdiplus_load_bitmap_from_file(L("c:\\Users\\sks\\Pictures\\wiki.png"));
#else
    void_WrapOsBoolChecking(NULL != (
    g_h_bitmap_banner = (HBITMAP)
        LoadImage(GetInstanceHandle(), (PCTSTR) (UINT_PTR) APP_RESOURCE_BANNER_BITMAP, IMAGE_BITMAP, 0, 0, 0)));
#endif

    if(NULL != g_h_bitmap_banner)
    {
        GetObject(g_h_bitmap_banner, sizeof32(g_banner_bitmap), &g_banner_bitmap);

        size.cx = g_banner_bitmap.bmWidth;
        size.cy = g_banner_bitmap.bmHeight;

        /* DPI-aware */
        size.cx = idiv_ceil_u(size.cx * PixelsPerInch.cx, 96);
        size.cy = idiv_ceil_u(size.cy * PixelsPerInch.cy, 96);
    }

    if(NULL != g_h_bitmap_banner)
    {
        { /* SKS 22feb2012 - obtain message font from system metrics */
        NONCLIENTMETRICS nonclientmetrics;
        nonclientmetrics.cbSize = (UINT) sizeof32(NONCLIENTMETRICS);
#if (WINVER >= 0x0600) /* keep size compatible with older OSes even if we can target newer */
        nonclientmetrics.cbSize -= sizeof32(nonclientmetrics.iPaddedBorderWidth);
#endif
        if(WrapOsBoolChecking(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, nonclientmetrics.cbSize, &nonclientmetrics, 0)))
        {
            g_logfont_splash = nonclientmetrics.lfMessageFont;
        }
        else
        {
            LOGFONT logfont;
            zero_struct(logfont);
            logfont.lfHeight = -10;
            logfont.lfWeight = FW_NORMAL;
            logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_ROMAN;
            tstr_xstrkpy(logfont.lfFaceName, elemof32(logfont.lfFaceName), TEXT("MS Shell Dlg"));
            g_logfont_splash = logfont;
        }
        trace_2(TRACE_APP_DIALOG, TEXT("*** splash logfont is %d, %s ***"), g_logfont_splash.lfHeight, report_tstr(g_logfont_splash.lfFaceName));
        } /*block*/

        void_WrapOsBoolChecking(HOST_FONT_NONE != (
        g_h_font_splash = CreateFontIndirect(&g_logfont_splash)));

        if(NULL == hwndParent)
        {
            g_extra_text_pixels_y = 0;
        }
        else
        {
            int text_y_inter_line, bottom_margin_pixels;
            int user_info_height = -g_logfont_splash.lfHeight; /* hopefully negative */

            if(user_info_height < 8) /* now negative or too small? */
                user_info_height = 16;

            text_y_inter_line = user_info_height;

            /* DPI-aware */
            text_y_inter_line   += idiv_ceil_u(SPLASH_INTER_LINE_SPACING   * PixelsPerInch.cy, 96);
            bottom_margin_pixels = idiv_ceil_u(SPLASH_BOTTOM_MARGIN_PIXELS * PixelsPerInch.cy, 96);

            g_extra_text_pixels_y = (1 * text_y_inter_line) + (user_info_height + bottom_margin_pixels);

            size.cy += g_extra_text_pixels_y;
        }

        size.cx += ((GDI_COORD) GetSystemMetrics(SM_CXDLGFRAME) * 2);
        size.cy += ((GDI_COORD) GetSystemMetrics(SM_CYDLGFRAME) * 2);

        void_WrapOsBoolChecking(HOST_WND_NONE != (
        hwnd_splash =
            CreateWindowEx(
                WS_EX_TOPMOST | 0, window_class[APP_WINDOW_CLASS_SPLASH], NULL, /*product_ui_id(),*/
                WS_POPUP | WS_VISIBLE | WS_DLGFRAME,
                screen_rect.left + ((screen_rect.right  - screen_rect.left) - size.cx) / 2,
                screen_rect.top  + ((screen_rect.bottom - screen_rect.top ) - size.cy) / 2,
                size.cx, size.cy,
                hwndParent, NULL, GetInstanceHandle(), NULL)));

        if(HOST_WND_NONE == hwnd_splash)
            splash_window_remove();
        else
        {
            UpdateWindow(hwnd_splash);
            SetTimer(hwnd_splash, TIMER_ID_REMOVE, splash_timeout_ms, NULL);
            while(HOST_WND_NONE != hwnd_splash)
            {
                MSG msg;
                int res = GetMessage(&msg, 0, 0, 0);
                if(res <= 0)
                    break;
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            //Sleep(splash_timeout_ms);
            splash_window_remove();
        }
    }
}

/* Remove the startup splash from the screen.
 * This is called just before polling for the first message (why not let it time out)
 */

extern void
splash_window_remove(void)
{
    /* Ensure we remove the window if on exists */
    if(HOST_WND_NONE != hwnd_splash)
    {
        KillTimer(hwnd_splash, TIMER_ID_REMOVE);
        DestroyWindow(hwnd_splash);
        hwnd_splash = NULL;
    }

    if(NULL != g_h_font_splash)
    {
        DeleteFont(g_h_font_splash);
        g_h_font_splash = NULL;
    }

    if(NULL != g_h_bitmap_banner)
    {
        consume_bool(WrapOsBoolChecking(DeleteBitmap(g_h_bitmap_banner)));
        g_h_bitmap_banner = NULL;
    }
}

#endif /* WINDOWS */

/* end of windows/splash.c */
