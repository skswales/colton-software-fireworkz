/* windows/ho_win.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* David De Vorchik (diz) Late October 1993 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "common/res_defs.h"

#define APP_RESOURCE_PROG_ICON          100
#define APP_RESOURCE_DOC_ICON           101 /* see comment in .rc file */

#include "commctrl.h" /* for InitCommonControlsEx() */

#include "commdlg.h"/* for HELPMSGSTRING */

#if !defined(DD_DEFDRAGDELAY)
#define DD_DEFDRAGDELAY         400
#endif

// Define some static border information

#define DRAG_TIMER_ID           128
#define TRIPLE_CLICK_TIMER_ID   129

#define MIN_SPLIT_X             5
#define MIN_SPLIT_Y             5

#define PIXEL_HORZ              1
#define PIXEL_VERT              1
#define PIXEL_HORZ2             2
#define PIXEL_VERT2             2

#define SCROLL_RANGE 0x4000 // a number with a little bit of granularity, especially for wacky monitors

typedef struct SPLIT_INFO
{
    struct SPLIT_INFO_DRAGGING
    {
        PIXIT_RECT clip_pixit_rect;
        PIXIT_RECT valid_pixit_rect;
        PIXIT_POINT previous_pixit_point;

        RECT old_horz_rect;
        RECT old_vert_rect;

        struct SPLIT_INFO_DRAGGING_FLAGS
        {
            BOOL old_horz_rect;
            BOOL old_vert_rect;
        } flags;
    } dragging;

    struct SPLIT_INFO_FLAGS
    {
        BOOL horz;
        BOOL vert;
        BOOL horz_box;
        BOOL vert_box;
    } flags;

    POINTER_SHAPE pointer_shape;
}
SPLIT_INFO;

static SPLIT_INFO split_info;

typedef struct HO_WIN_STATE
{
    /* Cached information about the world we are in */
    struct
    {
        POINT scroll;
        POINT split;
    } metrics;

    /* Drag state, defines all important stuff about drags, active != 0 then valid */
    struct
    {
        BOOL threaded;      /* possible to call host_drag_start */
        BOOL pending;       /* drag is pending, i.e. timer active */
        BOOL enabled;       /* drag is in progress */
        BOOL right_button;  /* will start on right button */

        /* Extra drag state, if threaded / pending or enabled set then this info is valid */
        DOCNO docno;
        VIEWNO viewno;
        HWND hwnd;
        POINT start;
        POINT dist;
        PIXIT_POINT start_pixit_point;
        P_ANY p_reason_data;

        EVENT_HANDLER event_handler;
        int capture_size;
    } drag;

    /* Triple click stuff, if hwnd != 0 then valid for sending event */
    struct
    {
        HWND hwnd;
        POINT start;
        POINT dist;
    } triple;

    /* State for the caret, this identifies which window without our world has the caret */
    struct
    {
        GDI_RECT gdi_rect;                      // bounding box defining the caret

        struct
        {
            HWND hwnd;                          // window that the caret lives in
            POINT dimensions;                   // visible size of caret (pixels)
            POINT point;                        // visible position (scrolled etc)
        } visible;

    } caret;

    /* State for pending redraw stuff */
    struct
    {
        BOOL in_update_fast;
        REDRAW_FLAGS redraw_flags;              // new redraw_flags
        BOOL redraw_flags_valid;
    }
    update;
}
HO_WIN_STATE;

static HO_WIN_STATE ho_win_state;

/* Extra information encoded into the window information for panes */

typedef struct HOST_EVENT_DESC
{
    BOOL scroll_x;
    BOOL scroll_y;
    BOOL set_scroll_x;
    BOOL set_scroll_y;
    BOOL is_slave;
    PANE_ID pane_id;
    int slave_id;
    int ruler_id;
    int border_id;
    int xform_index;
    P_PROC_EVENT p_proc_event;
    int offset_of_hwnd;
}
HOST_EVENT_DESC; typedef const HOST_EVENT_DESC * PC_HOST_EVENT_DESC;

/* Function list */

static void
show_view_pane_set(
    _ViewRef_   P_VIEW p_view);

static void
calc_pane_posn(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

static void
set_pane_posn(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

static void
move_pane(
    _ViewRef_   P_VIEW p_view,
    _In_        PANE_ID pane_id,
    _InVal_     BOOL show);

static void
scroll_and_recache(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        PANE_ID pane_id,
    _InRef_     PC_GDI_POINT p_offset);

static void
scroll_pane(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler,
    _InVal_     S32 new_scroll);

static void
scroll_pane_absolute(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler,
    _InRef_     PC_GDI_POINT p_gdi_point,
    _InVal_     BOOL redraw_scroll);

_Check_return_
static STATUS
slave_window_create(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        int slave_id);

static void
slave_window_destroy(
    _ViewRef_   P_VIEW p_view,
    _In_        int slave_id);

_Check_return_
static int
point_to_blip_pos(
    _InVal_     S32 pos,
    _InVal_     S32 range);

_Check_return_
static S32
point_from_blip_pos(
    _InVal_     int pos,
    _InVal_     S32 range);

static void
move_slave(
    _ViewRef_   P_VIEW p_view,
    _In_        int slave_id,
    _In_        int x,
    _In_        int y,
    _In_        int h,
    _In_        int w);

_Check_return_
static STATUS
ensure_pane_set(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

static void
refresh_back_window(
    _ViewRef_   P_VIEW p_view);

PROC_EVENT_PROTO(static, proc_event_back_window);

static void
get_split_info(
    _InRef_     PC_VIEW_POINT p_view_point);

static void
get_split_drag_info(
    _InRef_     PC_VIEWEVENT_CLICK p_viewevent_click,
    _OutRef_    P_PIXIT_POINT p_pixit_point);

static void
render_split_drag_info(
    _ViewRef_   P_VIEW p_view,
    P_PIXIT_POINT p_pixit_point);

static void
rect_from_view_rect(
    _OutRef_    PRECT p_rect,
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEW_RECT p_view_rect,
    _InRef_opt_ PC_RECT_FLAGS p_rect_flags,
    _InVal_     EVENT_HANDLER event_handler);

static void
update_pane_window(
    _HwndRef_   HWND hwnd,
    _InVal_     REDRAW_FLAGS redraw_flags);

_Check_return_
static STATUS
send_key_to_docu(
    _DocuRef_   P_DOCU p_docu,
    _In_        KMAP_CODE key_code);

static void
enable_triple_click(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y);

static void
disable_triple_click(void);

static void
send_click_to_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _HwndRef_   HWND hwnd,
    _InVal_     UINT uiMsg,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     EVENT_HANDLER event_handler,
    _InVal_     BOOL ctrl_pressed,
    _InVal_     BOOL shift_pressed);

static void
start_drag_monitor(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     EVENT_HANDLER event_handler,
    _InVal_     BOOL right_button);

static void
stop_drag_monitor(void);

static void
send_mouse_event(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     T5_MESSAGE t5_message,
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     BOOL ctrl_pressed,
    _InVal_     BOOL shift_pressed,
    _InVal_     EVENT_HANDLER event_handler);

static void
maybe_scroll(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     EVENT_HANDLER event_handler);

_Check_return_
static STATUS
view_pane_create(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        PANE_ID pane_id);

static void
view_pane_destroy(
    _ViewRef_   P_VIEW p_view,
    _In_        PANE_ID pane_id);

static void
view_point_from_window_point_and_context(
    _OutRef_    P_VIEW_POINT p_view_point,
    _InRef_     PC_GDI_POINT p_gdi_point,
    _InRef_     PC_CLICK_CONTEXT p_click_context);

static LRESULT CALLBACK
wndproc_host(
    _HwndRef_   HWND hwnd,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam);

static void
host_onPaint_back_window(
    _ViewRef_   P_VIEW p_view,
    _InRef_     PPAINTSTRUCT p_paintstruct);

static void
host_onPaint_pane_window(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PPAINTSTRUCT p_paintstruct,
    _InVal_     EVENT_HANDLER event_handler);

/*
yukky global export
*/

HWND g_hwnd_sdi_back;

/*
Globals owned by the window management code
*/

static HICON h_prog_icon_large;

_Check_return_
extern HICON
host_get_prog_icon_large(void)
{
    return(h_prog_icon_large);
}

static HICON h_prog_icon_small;

_Check_return_
extern HICON
host_get_prog_icon_small(void)
{
    return(h_prog_icon_small);
}

/* This table is used in the window handling code to determine the characteristics of the event handler being used.
 * It is mainly used by the scrolling and event processing functions.
 */

static const HOST_EVENT_DESC
host_event_desc_table[/*event_handler*/] =
{
    // Scroll using X offset
    //    Scroll using Y offset
    //       Set X scroll pos
    //          Set Y scroll pos
    //             Is slave
    //               Pane ID                Slave ID                    Ruler                       Border                      XFORM index    p_event handler
    { 1, 1, 0, 0, 0, WIN_PANE_SPLIT_DIAG,   0,                          0,                          0,                          XFORM_PANE, view_event_pane_window, offsetof32(VIEW, pane[WIN_PANE_SPLIT_DIAG].hwnd)    },  /* pane window */
    { 1, 1, 0, 0, 0, WIN_PANE_SPLIT_VERT,   0,                          0,                          0,                          XFORM_PANE, view_event_pane_window, offsetof32(VIEW, pane[WIN_PANE_SPLIT_VERT].hwnd)    },  /* pane window */
    { 1, 1, 0, 0, 0, WIN_PANE_SPLIT_HORZ,   0,                          0,                          0,                          XFORM_PANE, view_event_pane_window, offsetof32(VIEW, pane[WIN_PANE_SPLIT_HORZ].hwnd)    },  /* pane window */
    { 1, 1, 0, 0, 0, WIN_PANE,              0,                          0,                          0,                          XFORM_PANE, view_event_pane_window, offsetof32(VIEW, pane[WIN_PANE].hwnd)               },  /* pane window */

    { 1, 0, 0, 0, 1, WIN_PANE_SPLIT_HORZ,   WIN_BORDER_HORZ_SPLIT,      0,                          0,                          XFORM_HORZ, view_event_border_horz, offsetof32(VIEW, edge[WIN_BORDER_HORZ_SPLIT].hwnd)  },  /* horizontal border for split*/
    { 1, 0, 0, 0, 1, WIN_PANE,              WIN_BORDER_HORZ,            0,                          0,                          XFORM_HORZ, view_event_border_horz, offsetof32(VIEW, edge[WIN_BORDER_HORZ].hwnd)        },  /* horizontal border */
    { 0, 1, 0, 0, 1, WIN_PANE_SPLIT_VERT,   WIN_BORDER_VERT_SPLIT,      0,                          0,                          XFORM_VERT, view_event_border_vert, offsetof32(VIEW, edge[WIN_BORDER_VERT_SPLIT].hwnd)  },  /* vertical border for split*/
    { 0, 1, 0, 0, 1, WIN_PANE,              WIN_BORDER_VERT,            0,                          0,                          XFORM_VERT, view_event_border_vert, offsetof32(VIEW, edge[WIN_BORDER_VERT].hwnd)        },  /* vertical border */

    { 1, 0, 0, 0, 1, WIN_PANE_SPLIT_HORZ,   WIN_RULER_HORZ_SPLIT,       0,                          0,                          XFORM_HORZ, view_event_ruler_horz,  offsetof32(VIEW, edge[WIN_RULER_HORZ_SPLIT].hwnd)   },  /* horizontal ruler for split*/
    { 1, 0, 0, 0, 1, WIN_PANE,              WIN_RULER_HORZ,             0,                          0,                          XFORM_HORZ, view_event_ruler_horz,  offsetof32(VIEW, edge[WIN_RULER_HORZ].hwnd)         },  /* horizontal ruler */
    { 0, 1, 0, 0, 1, WIN_PANE_SPLIT_VERT,   WIN_RULER_VERT_SPLIT,       0,                          0,                          XFORM_VERT, view_event_ruler_vert,  offsetof32(VIEW, edge[WIN_RULER_VERT_SPLIT].hwnd)   },  /* vertical ruler for split*/
    { 0, 1, 0, 0, 1, WIN_PANE,              WIN_RULER_VERT,             0,                          0,                          XFORM_VERT, view_event_ruler_vert,  offsetof32(VIEW, edge[WIN_RULER_VERT].hwnd)         },  /* vertical ruler */

    { 0, 0, 1, 0, 1, WIN_PANE_SPLIT_HORZ,   WIN_HSCROLL_SPLIT_HORZ,     WIN_RULER_HORZ_SPLIT,       WIN_BORDER_HORZ_SPLIT,      XFORM_BACK, NULL,                   offsetof32(VIEW, edge[WIN_HSCROLL_SPLIT_HORZ].hwnd) },  /* horizontal scroll bar for split */
    { 0, 0, 1, 0, 1, WIN_PANE,              WIN_HSCROLL,                WIN_RULER_HORZ,             WIN_BORDER_HORZ,            XFORM_BACK, NULL,                   offsetof32(VIEW, edge[WIN_HSCROLL].hwnd)            },  /* horizontal scroll bar */
    { 0, 0, 0, 1, 1, WIN_PANE_SPLIT_VERT,   WIN_VSCROLL_SPLIT_VERT,     WIN_RULER_VERT_SPLIT,       WIN_BORDER_VERT_SPLIT,      XFORM_BACK, NULL,                   offsetof32(VIEW, edge[WIN_VSCROLL_SPLIT_VERT].hwnd) },  /* vertical scroll bar for split */
    { 0, 0, 0, 1, 1, WIN_PANE,              WIN_VSCROLL,                WIN_RULER_VERT,             WIN_BORDER_VERT,            XFORM_BACK, NULL,                   offsetof32(VIEW, edge[WIN_VSCROLL].hwnd)            },  /* vertical scroll bar */

    { 0, 0, 0, 0, 0, WIN_PANE_NONE,         0,                          0,                          0,                          XFORM_BACK, proc_event_back_window, offsetof32(VIEW, main[WIN_BACK].hwnd)               },  /* back window */
    { 0, 0, 0, 0, 0, WIN_PANE_NONE,         0,                          0,                          0,                          XFORM_BACK, NULL,                   offsetof32(VIEW, main[WIN_TOOLBAR].hwnd)            },  /* toolbar */
    { 0, 0, 0, 0, 0, WIN_PANE_NONE,         0,                          0,                          0,                          XFORM_BACK, NULL,                   offsetof32(VIEW, main[WIN_SLE].hwnd)                },  /* sle */
    { 0, 0, 0, 0, 0, WIN_PANE_NONE,         0,                          0,                          0,                          XFORM_BACK, NULL,                   offsetof32(VIEW, main[WIN_UPPER_STATUS_LINE].hwnd)  },  /* upper status line */
    { 0, 0, 0, 0, 0, WIN_PANE_NONE,         0,                          0,                          0,                          XFORM_BACK, NULL,                   offsetof32(VIEW, main[WIN_LOWER_STATUS_LINE].hwnd)  }   /* lower status line */
};

/* Table defines the windows and borders that are associated with the given
 * pane.  It is indexed by the pane ID and gives the windows etc. that can
 * be updated, scroll etc..
 */

typedef struct HOST_PANE_DESC
{
    int hscroll_id; EVENT_HANDLER hscroll_event_handler;
    int vscroll_id; EVENT_HANDLER vscroll_event_handler;
    int hborder_id;
    int vborder_id;
    int hruler_id;
    int vruler_id;
}
HOST_PANE_DESC; typedef const HOST_PANE_DESC * PC_HOST_PANE_DESC;

static const HOST_PANE_DESC
host_pane_desc_table[/*pane_id*/ NUMPANE] =
{
//    Horizontal scroll         Horizontal scroll event hdlr Vertical scroll         Vertical scroll event hdlr   Horz border            Vert border            Horz ruler            Vert ruler
    { WIN_HSCROLL_SPLIT_HORZ,   EVENT_HANDLER_HSCROLL_SPLIT, WIN_VSCROLL_SPLIT_VERT, EVENT_HANDLER_VSCROLL_SPLIT, WIN_BORDER_HORZ_SPLIT, WIN_BORDER_VERT_SPLIT, WIN_RULER_HORZ_SPLIT, WIN_RULER_VERT_SPLIT },
    { WIN_HSCROLL,              EVENT_HANDLER_HSCROLL,       WIN_VSCROLL_SPLIT_VERT, EVENT_HANDLER_VSCROLL_SPLIT, WIN_BORDER_HORZ,       WIN_BORDER_VERT_SPLIT, WIN_RULER_HORZ,       WIN_RULER_VERT_SPLIT },
    { WIN_HSCROLL_SPLIT_HORZ,   EVENT_HANDLER_HSCROLL_SPLIT, WIN_VSCROLL,            EVENT_HANDLER_VSCROLL,       WIN_BORDER_HORZ_SPLIT, WIN_BORDER_VERT,       WIN_RULER_HORZ_SPLIT, WIN_RULER_VERT       },
    { WIN_HSCROLL,              EVENT_HANDLER_HSCROLL,       WIN_VSCROLL,            EVENT_HANDLER_VSCROLL,       WIN_BORDER_HORZ,       WIN_BORDER_VERT,       WIN_RULER_HORZ,       WIN_RULER_VERT       }
};

/* Mapping table that can be used to map a pane_id to an event_handler */

typedef struct INDEX_TO_EVENT_HANDLER_DESC
{
    EVENT_HANDLER event_handler;
}
INDEX_TO_EVENT_HANDLER_DESC, * P_INDEX_TO_EVENT_HANDLER_DESC;

static INDEX_TO_EVENT_HANDLER_DESC
pane_to_event_handler_table[/*pane_id*/ NUMPANE] =
{
    { EVENT_HANDLER_PANE_SPLIT_DIAG },
    { EVENT_HANDLER_PANE_SPLIT_VERT },
    { EVENT_HANDLER_PANE_SPLIT_HORZ },
    { EVENT_HANDLER_PANE }
};

/* Mapping table that can be used to map a slave_id to an event_handler */

static INDEX_TO_EVENT_HANDLER_DESC
slave_to_event_handler_table[/*slave_id*/] =
{
    { EVENT_HANDLER_BORDER_HORZ_SPLIT },
    { EVENT_HANDLER_BORDER_HORZ },
    { EVENT_HANDLER_BORDER_VERT_SPLIT },
    { EVENT_HANDLER_BORDER_VERT },

    { EVENT_HANDLER_RULER_HORZ_SPLIT },
    { EVENT_HANDLER_RULER_HORZ },
    { EVENT_HANDLER_RULER_VERT_SPLIT },
    { EVENT_HANDLER_RULER_VERT }
};

static HWND g_current_track_window = NULL;

extern void
host_clear_tracking_for_window(
    _HwndRef_   HWND hwnd)
{
    UNREFERENCED_PARAMETER_HwndRef_(hwnd);

    g_current_track_window = NULL;
}

_Check_return_
extern BOOL
host_set_tracking_for_window(
    _HwndRef_   HWND hwnd)
{
    TRACKMOUSEEVENT trackmouseevent;

    if(HOST_WND_NONE == hwnd)
        return(FALSE);

    /* already tracking this window? */
    if(hwnd == g_current_track_window)
        return(FALSE);

    if(NULL != g_current_track_window)
    {
        /* not been cancelled */
    }

    trackmouseevent.cbSize = sizeof32(trackmouseevent);
    trackmouseevent.dwFlags = TME_LEAVE;
    trackmouseevent.hwndTrack = hwnd;
    trackmouseevent.dwHoverTime = HOVER_DEFAULT;
    void_WrapOsBoolChecking(TrackMouseEvent(&trackmouseevent));

    g_current_track_window = hwnd;

    return(TRUE);
}

_Check_return_
static LRESULT
host_onQueryEndSession(void)
{
    static int wm_queryendsession_processed = FALSE;
    static int wm_queryendsession_response = FALSE;
    static MONOTIME wm_queryendsession_last_time;

    S32 count;
    STATUS status;

#define QUERYENDSESSION_TIME MONOTIMEDIFF_VALUE_FROM_SECONDS(10)

    if(wm_queryendsession_processed)
    {
        /* if we get another one soon after finishing processing one, ignore it, responding as before */
        if(monotime_diff(wm_queryendsession_last_time) < QUERYENDSESSION_TIME)
        {
            wm_queryendsession_last_time = monotime();
            return(wm_queryendsession_response);
        }

        /* some time has elapsed, check out a new response */
        wm_queryendsession_processed = FALSE;
    }

    count = docs_modified();

    status = query_quit(P_DOCU_NONE, count);

    wm_queryendsession_processed = TRUE;

    wm_queryendsession_last_time = monotime();

    if(status_ok(status))
    {
        wm_queryendsession_response = 1; /* remember response in case we need to reuse it */

        /* SKS says we should try to close down properly not just allow session to kill us! */
        PostQuitMessage(0);
    }
    else
        wm_queryendsession_response = 0; /* remember response in case we need to reuse it */

    return(wm_queryendsession_response);
}

_Check_return_
static LRESULT
host_onEndSession(
    _InVal_     WPARAM wParam,
    _InVal_     LPARAM lParam)
{
    /* message is sent to multiple windows */
    if(g_silent_shutdown)
        return(0L);

    if(wParam)
    {
        if(ENDSESSION_CRITICAL & lParam)
        {
            g_silent_shutdown = TRUE; /* suppress error reporting for uninterrupted processing */

            status_consume(save_all_modified_docs_for_shutdown());

            /* now get out of here as fast as possible */
#if defined(USE_GLOBAL_CLIPBOARD)
            host_release_global_clipboard(FALSE); /* DO bother to do this - avoids any deferred clipboard rendering request */
#endif
            /*docno_close_all();*/ /* don't bother */

            t5_do_exit();

            exit(EXIT_SUCCESS);
        }

        if(ENDSESSION_CLOSEAPP & lParam)
        {
            g_silent_shutdown = TRUE; /* suppress error reporting for uninterrupted processing */

            status_consume(save_all_modified_docs_for_shutdown());

            /* used to PostQuitMessage() but now just get out of here as fast as possible */
#if defined(USE_GLOBAL_CLIPBOARD)
            host_release_global_clipboard(FALSE); /* DO bother to do this - avoids any deferred clipboard rendering request */
#endif
            /*docno_close_all();*/ /* don't bother */

            t5_do_exit();

            exit(EXIT_SUCCESS);
        }
    }

    return(0L);
}

extern LRESULT CALLBACK
host_top_level_window_event_handler(
    _HwndRef_   HWND hwnd,
    _InVal_     UINT uiMsg,
    _InVal_     WPARAM wParam,
    _InVal_     LPARAM lParam)
{
    switch(uiMsg)
    {
    case WM_QUERYENDSESSION:
        return(host_onQueryEndSession());

    case WM_ENDSESSION:
        return(host_onEndSession(wParam, lParam));

    default:
        return(DefWindowProc(hwnd, uiMsg, wParam, lParam));
    }
}

static void
do_drag_start(
    _InVal_     BOOL ctrl_pressed,
    _InVal_     BOOL shift_pressed)
{
    const P_DOCU p_docu = p_docu_from_docno(ho_win_state.drag.docno);
    const P_VIEW p_view = p_view_from_viewno(p_docu, ho_win_state.drag.viewno);

    const T5_MESSAGE t5_message = ho_win_state.drag.right_button ? T5_EVENT_CLICK_RIGHT_DRAG : T5_EVENT_CLICK_LEFT_DRAG;

    stop_drag_monitor();

    ho_win_state.drag.threaded = TRUE;
    send_mouse_event(p_docu, p_view, t5_message, ho_win_state.drag.hwnd, ho_win_state.drag.start.x, ho_win_state.drag.start.y, ctrl_pressed, shift_pressed, ho_win_state.drag.event_handler);
    ho_win_state.drag.threaded = FALSE;
}

static void
send_drag_movement_and_maybe_scroll(
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     BOOL ctrl_pressed,
    _InVal_     BOOL shift_pressed)
{
    const P_DOCU p_docu = p_docu_from_docno(ho_win_state.drag.docno);
    const P_VIEW p_view = p_view_from_viewno(p_docu, ho_win_state.drag.viewno);

    assert(ho_win_state.drag.enabled);

    send_mouse_event(p_docu, p_view, T5_EVENT_CLICK_DRAG_MOVEMENT, ho_win_state.drag.hwnd, x, y, ctrl_pressed, shift_pressed, ho_win_state.drag.event_handler);

    maybe_scroll(p_docu, p_view, x, y, ho_win_state.drag.event_handler);
}

static void
GetMessagePosAsClient(
    _HwndRef_   HWND hwnd,
    _Out_       int * const p_x,
    _Out_       int * const p_y)
{
    DWORD packed_point = GetMessagePos();
    POINT point;
    point.x = LOWORD(packed_point);
    point.y = HIWORD(packed_point);
    ScreenToClient(hwnd, &point);
    *p_x = point.x;
    *p_y = point.y;
}

#define HO_WIN_DRAG_NULL_CLIENT_HANDLE ((CLIENT_HANDLE) 0x00000004)

_Check_return_
static STATUS
ho_win_maeve_service_win_host_null(void)
{
    if(ho_win_state.drag.enabled)
    {
        const BOOL ctrl_pressed = host_ctrl_pressed(); /* these may change during drag */
        const BOOL shift_pressed = host_shift_pressed();
        int x, y;

        GetMessagePosAsClient(ho_win_state.drag.hwnd, &x, &y);

        send_drag_movement_and_maybe_scroll(x, y, ctrl_pressed, shift_pressed);
    }

    return(STATUS_OK);
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_win);

#include "external/Microsoft/InsideOLE2/BTTNCURP/bttncur.h"

_Check_return_
static STATUS
ho_win_msg_exit2(void)
{
    ho_dde_msg_exit2();

    ho_dial_msg_exit2();

    ho_paint_msg_exit2();

    bttncur_WEP();

    if(NULL != h_prog_icon_large)
    {
        consume_bool(DestroyIcon(h_prog_icon_large));
        h_prog_icon_large = NULL;
    }

    if(NULL != h_prog_icon_small)
    {
        consume_bool(DestroyIcon(h_prog_icon_small));
        h_prog_icon_small = NULL;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
ho_win_msg_startup(void)
{
    /* read the system information (split points etc) */
    ho_win_state.metrics.scroll.x = GetSystemMetrics(SM_CXVSCROLL);
    ho_win_state.metrics.scroll.y = GetSystemMetrics(SM_CYHSCROLL);

    ho_win_state.metrics.split.x = MAX(MIN_SPLIT_X, GetSystemMetrics(SM_CXFRAME));
    ho_win_state.metrics.split.y = MAX(MIN_SPLIT_Y, GetSystemMetrics(SM_CYFRAME));

    { /* get the common controls we need */
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof32(icex);
    icex.dwICC  = ICC_UPDOWN_CLASS;
    void_WrapOsBoolChecking(InitCommonControlsEx(&icex));
    } /*block*/

    trace_2(TRACE_WINDOWS_HOST, TEXT("SM_CXICON=%d, SM_CYICON=%d"), GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    void_WrapOsBoolChecking(NULL != (
    h_prog_icon_large = (HICON)
        LoadImage(GetInstanceHandle(), MAKEINTRESOURCE(APP_RESOURCE_PROG_ICON),
                  IMAGE_ICON,
                  GetSystemMetrics(SM_CXICON),
                  GetSystemMetrics(SM_CYICON),
                  0)));

    trace_2(TRACE_WINDOWS_HOST, TEXT("SM_CXSMICON=%d, SM_CYSMICON=%d"), GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    void_WrapOsBoolChecking(NULL != (
    h_prog_icon_small = (HICON)
        LoadImage(GetInstanceHandle(), MAKEINTRESOURCE(APP_RESOURCE_PROG_ICON),
                  IMAGE_ICON,
                  GetSystemMetrics(SM_CXSMICON),
                  GetSystemMetrics(SM_CYSMICON),
                  0)));

    if(NULL == g_hInstancePrev)
    {   /* register document window classes */
        WNDCLASS wndclass;
        zero_struct(wndclass);
        wndclass.style = CS_DBLCLKS;
        wndclass.lpfnWndProc = (WNDPROC) wndproc_host;
      /*wndclass.cbClsExtra = 0;*/
      /*wndclass.cbWndExtra = 0;*/
        wndclass.hInstance = GetInstanceHandle();
        wndclass.hIcon = host_get_prog_icon_large();
      /*wndclass.hCursor = NULL;*/
        wndclass.hbrBackground = GetStockBrush(NULL_BRUSH);
      /*wndclass.lpszMenuName = NULL;*/
        wndclass.lpszClassName = window_class[APP_WINDOW_CLASS_BACK];
        if(!WrapOsBoolChecking(RegisterClass(&wndclass)))
            return(status_nomem());
        wndclass.lpszClassName = window_class[APP_WINDOW_CLASS_PANE];
        if(!WrapOsBoolChecking(RegisterClass(&wndclass)))
            return(status_nomem());
        wndclass.lpszClassName = window_class[APP_WINDOW_CLASS_BORDER];
        if(!WrapOsBoolChecking(RegisterClass(&wndclass)))
            return(status_nomem());
    }

    (void) bttncur_LibMain(GetInstanceHandle());
    (void) UIToolConfigureForDisplay(&tdd);

    return(STATUS_OK);
}

_Check_return_
static STATUS
maeve_services_ho_win_msg_initclose(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose)
{
    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(ho_win_msg_startup());

    case T5_MSG_IC__EXIT2:
        return(ho_win_msg_exit2());

    case T5_MSG_IC__CLOSE2:
        return(ho_menu_msg_close2(p_docu));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_win)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_ho_win_msg_initclose(p_docu, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/* Null events come through into this event handler */

PROC_EVENT_PROTO(static, null_event_ho_win)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER(p_data);

    switch(t5_message)
    {
    case T5_MSG_WIN_HOST_NULL:
        return(ho_win_maeve_service_win_host_null());

    default: default_unhandled();
        return(STATUS_OK);
    }
}

/*
Functions for passing the caret and ensuring that it is visible at the correct time
*/

/*ncr*/
extern BOOL
host_show_caret(
    _HwndRef_   HWND hwnd,
    _In_        int width,
    _In_        int height,
    _In_        int x,
    _In_        int y)
{
    // discard existing caret
    DestroyCaret();

    // initialise the state for the caret
    ho_win_state.caret.visible.hwnd = hwnd;
    ho_win_state.caret.visible.dimensions.x = width;
    ho_win_state.caret.visible.dimensions.y = height;
    ho_win_state.caret.visible.point.x = x;
    ho_win_state.caret.visible.point.y = y;

    // create a new caret for this viewer
    if(ho_win_state.caret.visible.dimensions.y)
    {
        CreateCaret(hwnd, (HBITMAP) NULL, ho_win_state.caret.visible.dimensions.x, ho_win_state.caret.visible.dimensions.y);
        SetCaretPos(ho_win_state.caret.visible.point.x, ho_win_state.caret.visible.point.y);
        ShowCaret(hwnd);
        trace_0(TRACE_APP_SKEL, TEXT("show caret"));
    }

    return(TRUE);
}

extern void
host_get_pixel_size(
    _HwndRef_opt_ HWND hwnd /* NULL for screen */,
    _OutRef_    PSIZE p_size)
{
    const HDC hDC = GetDC(hwnd);
    p_size->cx = GetDeviceCaps(hDC, LOGPIXELSX);
    p_size->cy = GetDeviceCaps(hDC, LOGPIXELSY);
    void_WrapOsBoolChecking(1 == ReleaseDC(hwnd, hDC));
}

/* Setup the structure for the specified view port.  We are
 * given its opening position and size. Our job is to make
 * the back window, the pane windows are attached at open time.
 */

_Check_return_
extern STATUS
host_view_init(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_PIXIT_POINT p_tl,
    _InRef_     PC_PIXIT_SIZE p_size)
{
    U32 packed_id = viewid_pack(p_docu, p_view, (U16) EVENT_HANDLER_BACK_WINDOW);
    STATUS status = STATUS_OK;
    GDI_POINT tl;
    GDI_SIZE size;
    HWND hwnd;

    trace_2(TRACE_WINDOWS_HOST, TEXT("host_view_init: Init view on view structure at ") PTR_XTFMT TEXT(", for docu ") PTR_XTFMT, p_view, p_docu);
    trace_2(TRACE_WINDOWS_HOST, TEXT("                 top left x = ") S32_TFMT TEXT(", y = ") S32_TFMT, p_tl->x, p_tl->y);
    trace_2(TRACE_WINDOWS_HOST, TEXT("                 width of view is ") S32_TFMT TEXT(", height is ") S32_TFMT, p_size->cx, p_size->cy);

    status_return(ho_create_docu_menus(p_docu));

    /* Convert from pixits to pixels for the top left of the view, or use defaults if x == y == 0) */
    // there's a problem to sort here 08jun94 <<<
    if((p_tl->x) && (p_tl->y))
        window_point_from_pixit_point(&tl, p_tl, &p_view->host_xform[XFORM_BACK]);
    //else
        tl.x = tl.y = CW_USEDEFAULT;

    if((p_size->cx) && (p_size->cy))
        window_point_from_pixit_point((P_GDI_POINT) &size, (PC_PIXIT_POINT) p_size, &p_view->host_xform[XFORM_BACK]);
    else
    {
#if 1
        SIZE PixelsPerInch;
        host_get_pixel_size(NULL /*screen*/, &PixelsPerInch); /* Get current pixel size for the screen e.g. 96 or 120 */
        size.cx = (89 * PixelsPerInch.cx) / 10;
        size.cy =   7 * PixelsPerInch.cy;
#else
        size.cx = size.cy = CW_USEDEFAULT;
#endif
    }

    /* Make the back window, extra data contains the document and other data */
    void_WrapOsBoolChecking(HOST_WND_NONE != (
    hwnd =
        CreateWindowEx(
            WS_EX_ACCEPTFILES, window_class[APP_WINDOW_CLASS_BACK], NULL,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
            tl.x, tl.y, size.cx, size.cy,
            NULL /*top-level*/, NULL, GetInstanceHandle(), &packed_id)));
    if(HOST_WND_NONE == hwnd)
        status = create_error(ERR_TOOMANYWINDOWS);
    else
    {   /* Back window has been created - register it */
        assert(hwnd == p_view->main[WIN_BACK].hwnd);

        if(HOST_WND_NONE == g_hwnd_sdi_back)
            g_hwnd_sdi_back = hwnd; /* for OLE server? */

        void_WrapOsBoolChecking(
            SetMenu(hwnd, ho_get_menu_bar(p_docu, p_view)));

        if(NULL != host_get_prog_icon_large())
            consume(LRESULT, /*old HICON*/ SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM) host_get_prog_icon_large()));

        if(NULL != host_get_prog_icon_small())
            consume(LRESULT, /*old HICON*/ SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM) host_get_prog_icon_small()));
    }

    if(status_fail(status))
        host_view_destroy(p_docu, p_view);

    return(status);
}

/* Attempt to tidy up the viewer window.  This is called when the view onto the document is closed.
 * We must attempt to tidy up (close the background) and then remove all the panes as required.
 */

extern void
host_view_destroy(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    trace_2(TRACE_WINDOWS_HOST, TEXT("host_view_destroy: called to remove windows from p_view ") PTR_XTFMT TEXT(" from p_docu ") PTR_XTFMT, p_view, p_docu);

    if(NULL == p_view)
        return;

    {
    PANE_ID pane_id;
    for(pane_id = PANE_ID_START; pane_id < NUMPANE; PANE_ID_INCR(pane_id))
        if(HOST_WND_NONE != p_view->pane[pane_id].hwnd)
            view_pane_destroy(p_view, pane_id);
    } /*block*/

    if(HOST_WND_NONE == p_view->main[WIN_BACK].hwnd)
        return;

    SetMenu(p_view->main[WIN_BACK].hwnd, NULL /* no menu bar! */);

    if(g_hwnd_sdi_back == p_view->main[WIN_BACK].hwnd)
        g_hwnd_sdi_back = NULL;

    DestroyWindow(p_view->main[WIN_BACK].hwnd);
    p_view->main[WIN_BACK].hwnd = NULL;
}

/* Show the viewer.  In this case we show the background window
 * and then the panes as required.  This code should call
 * the child handling functions to ensure that the child
 * windows are correctly sized and positioned.  The window
 * should then be moved to the top of the window stack.
 */

extern void
host_view_show(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    if(HOST_WND_NONE == p_view->main[WIN_BACK].hwnd)
        return;

    status_assert(ensure_pane_set(p_docu, p_view));
    calc_pane_posn(p_docu, p_view);
    set_pane_posn(p_docu, p_view);
    show_view_pane_set(p_view);
    ShowWindow(p_view->main[WIN_BACK].hwnd, SW_SHOW);
}

/* Attempt to re-open a viewer.  This is called as part of the
 * viewer has been changed - i.e. a resize, or, the viewer
 * information has been updated (split etc!)
 */

extern void
host_view_reopen(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    if(HOST_WND_NONE == p_view->main[WIN_BACK].hwnd)
        return;

    status_assert(ensure_pane_set(p_docu, p_view));
    calc_pane_posn(p_docu, p_view);
    set_pane_posn(p_docu, p_view);
    show_view_pane_set(p_view);
    refresh_back_window(p_view);
    ShowWindow(p_view->main[WIN_BACK].hwnd, SW_SHOWNA);
}

/* Read the bounding information about the view given (the back window)
 * and convert it from screen-pixels to a pixit coord giving
 * the top left, and a width and height value.
 */

extern void
host_view_query_posn(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _OutRef_    P_PIXIT_POINT p_tl,
    _OutRef_    P_PIXIT_SIZE p_size)
{
    RECT window_rect;
    GDI_RECT gdi_rect;
    PIXIT_RECT pixit_rect;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    GetWindowRect(p_view->main[WIN_BACK].hwnd, &window_rect);
    gdi_rect.tl.x = window_rect.left;
    gdi_rect.tl.y = window_rect.top;
    gdi_rect.br.x = window_rect.right;
    gdi_rect.br.y = window_rect.bottom;
    pixit_rect_from_window_rect(&pixit_rect, &gdi_rect, &p_view->host_xform[XFORM_BACK]);

    p_tl->x = pixit_rect.tl.x;
    p_tl->y = pixit_rect.tl.y;
    p_size->cx = (pixit_rect.br.x - pixit_rect.tl.x);
    p_size->cy = (pixit_rect.br.y - pixit_rect.tl.y);
}

/* Define the extent of the given view.  This is stored with each pane in
 * the view and is used intensively by the scrolling functions
 * to ensure that the user is doing something sensible.
 */

extern void
host_set_extent_this_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    GDI_POINT pane_extent;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    window_point_from_pixit_point(&pane_extent, &p_view->paneextent, &p_view->host_xform[XFORM_PANE]);

    { /* Modify the extent in the pane descriptions */
    PANE_ID pane_id = NUMPANE;
    do  {
        PANE_ID_DECR(pane_id);

        p_view->pane[pane_id].extent.x = pane_extent.x;
        p_view->pane[pane_id].extent.y = pane_extent.y;

    } while(PANE_ID_START != pane_id);
    } /*block*/

    { /* Cycle over the event handlers, looking for ones which are scroll widgets */
    EVENT_HANDLER event_handler;
    for(event_handler = 0; event_handler < NUMEVENTS; ++event_handler)
    {
        const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];

        if(p_host_event_desc->set_scroll_x)
        {
            const HWND hwnd = p_view->edge[p_host_event_desc->slave_id].hwnd;
            if(HOST_WND_NONE != hwnd)
            {
                const PC_PANE p_pane = &p_view->pane[p_host_event_desc->pane_id];
                SetScrollPos(hwnd, SB_CTL, point_to_blip_pos(p_pane->scroll.x, p_pane->extent.x - p_pane->size.x), TRUE);
            }
        }

        if(p_host_event_desc->set_scroll_y)
        {
            const HWND hwnd = p_view->edge[p_host_event_desc->slave_id].hwnd;
            if(HOST_WND_NONE != hwnd)
            {
                const PC_PANE p_pane = &p_view->pane[p_host_event_desc->pane_id];
                SetScrollPos(hwnd, SB_CTL, point_to_blip_pos(p_pane->scroll.y, p_pane->extent.y - p_pane->size.y), TRUE);
            }
        }
    }
    } /*block*/
}

/* Set the viewer title.  We are given the viewer information from
 * which we can extract the back window information, we are then
 * pass the title string (a static string) to the window.
 */

extern void
host_settitle(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_z_      PCTSTR p_new_title)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    trace_1(TRACE_WINDOWS_HOST, TEXT("host_settitle: set title string for window: '%s'"), p_new_title);
    SetWindowText(p_view->main[WIN_BACK].hwnd, p_new_title);
}

/* The view information has been changed so ensure that the
 * cached information about the visible rows and columns are correcltly
 * updated for each of the panes.
 */

extern void
host_recache_visible_row_range(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    PANE_ID pane_id = NUMPANE;

    do  {
        P_PANE p_pane;

        PANE_ID_DECR(pane_id);

        p_pane = &p_view->pane[pane_id];

        if(HOST_WND_NONE != p_pane->hwnd)
            skel_visible_row_range(p_docu, p_view, &p_pane->visible_skel_rect, &p_pane->visible_row_range);

    } while(PANE_ID_START != pane_id);
}

/* Make a supporting pane given its ID.  This code will then
 * create the window.  Panes often require supporting windows,
 * this code copes with creating the supporting windows
 * and generates a suitable set of scroll bars and other
 * gadgets that will be used.
 */

_Check_return_
static STATUS
view_pane_create(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        PANE_ID pane_id)
{
    STATUS status = STATUS_OK;
    EVENT_HANDLER event_handler = pane_to_event_handler_table[pane_id].event_handler;
    U32 packed_id = viewid_pack(p_docu, p_view, (U16) event_handler);
    HWND hwnd;

    trace_1(TRACE_WINDOWS_HOST, TEXT("view_pane_create: Called to make a view pane, id = ") S32_TFMT, pane_id);

    void_WrapOsBoolChecking(HOST_WND_NONE != (
    hwnd =
        CreateWindowEx(
            WS_EX_ACCEPTFILES, window_class[APP_WINDOW_CLASS_PANE], NULL,
            WS_CHILD,
            0, 0, 100, 100,
            p_view->main[WIN_BACK].hwnd, NULL, GetInstanceHandle(), &packed_id)));
    if(HOST_WND_NONE == hwnd)
        return(create_error(ERR_TOOMANYWINDOWS));

    assert(hwnd == p_view->pane[pane_id].hwnd);

    switch(pane_id)
    {
    case WIN_PANE_SPLIT_DIAG:
        break;

    case WIN_PANE_SPLIT_HORZ:
        status = slave_window_create(p_docu, p_view, WIN_HSCROLL_SPLIT_HORZ);
        break;

    case WIN_PANE_SPLIT_VERT:
        status = slave_window_create(p_docu, p_view, WIN_VSCROLL_SPLIT_VERT);
        break;

    default: default_unhandled();
    case WIN_PANE:
        status_break(status = slave_window_create(p_docu, p_view, WIN_HSCROLL));
        status_break(status = slave_window_create(p_docu, p_view, WIN_VSCROLL));
        break;
    }

    if(status_fail(status))
        view_pane_destroy(p_view, pane_id);

    return(status);
}

/* Given a pane ID attempt to remove it, including removing its
 * slave windows - scroll bars etc.  The pane window is
 * validated by checking its handle is non-zero, then and
 * only then will it and its slaves be removed.
 */

static void
view_pane_destroy(
    _ViewRef_   P_VIEW p_view,
    _In_        PANE_ID pane_id)
{
    P_PANE p_pane;

    trace_1(TRACE_WINDOWS_HOST, TEXT("view_pane_destroy: called to destroy view pane %d"), pane_id);

    if(!IS_PANE_ID_VALID(pane_id))
        return;

    p_pane = &p_view->pane[pane_id];

    assert(p_pane->hwnd);

    switch(pane_id)
    {
    case WIN_PANE_SPLIT_DIAG:
        break;

    case WIN_PANE_SPLIT_HORZ:
        slave_window_destroy(p_view, WIN_HSCROLL_SPLIT_HORZ);
        break;

    case WIN_PANE_SPLIT_VERT:
        slave_window_destroy(p_view, WIN_VSCROLL_SPLIT_VERT);
        break;

    default: default_unhandled();
    case WIN_PANE:
        slave_window_destroy(p_view, WIN_HSCROLL);
        slave_window_destroy(p_view, WIN_VSCROLL);
        break;
    }

    if(p_view->flags.horz_border_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_HORZ)))
        slave_window_destroy(p_view, (pane_id == WIN_PANE) ? WIN_BORDER_HORZ : WIN_BORDER_HORZ_SPLIT);
    if(p_view->flags.vert_border_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_VERT)))
        slave_window_destroy(p_view, (pane_id == WIN_PANE) ? WIN_BORDER_VERT : WIN_BORDER_VERT_SPLIT);

    if(p_view->flags.horz_ruler_on  && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_HORZ)))
        slave_window_destroy(p_view, (pane_id == WIN_PANE) ? WIN_RULER_HORZ : WIN_RULER_HORZ_SPLIT);
    if(p_view->flags.vert_ruler_on  && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_VERT)))
        slave_window_destroy(p_view, (pane_id == WIN_PANE) ? WIN_RULER_VERT : WIN_RULER_VERT_SPLIT);

    DestroyWindow(p_pane->hwnd);
    p_pane->hwnd = NULL;
}

/* Attempt to show the view specified. This involves ensuring that all the child windows are
 * shown for the specified back window by cycling through all the handles and opening them
 */

static void
show_view_pane_set(
    _ViewRef_   P_VIEW p_view)
{
    PANE_ID pane_id;
    EDGE_ID edge_id;

    for(pane_id = PANE_ID_START; pane_id < NUMPANE; PANE_ID_INCR(pane_id))
        if(HOST_WND_NONE != p_view->pane[pane_id].hwnd)
            ShowWindow(p_view->pane[pane_id].hwnd, SW_SHOW);

    for(edge_id = EDGE_ID_START; edge_id < NUMEDGE; EDGE_ID_INCR(edge_id))
        if(HOST_WND_NONE != p_view->edge[edge_id].hwnd)
            ShowWindow(p_view->edge[edge_id].hwnd, SW_SHOW);
}

/* Initialise the information about the panes and their positions
 * into the view structure.  We compute all the panes and other
 * information here and then we can use it to redraw, position
 * and click detect on the windows.
 */

#define set_rect(p_rect, x, y, dx, dy ) \
    SetRect(p_rect, x, y, x + dx, y +dy)

static void
calc_pane_posn(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    POINT ruler = { 0, 0 };
    POINT border = { 0, 0 };
    POINT start, inner, outer, max;
    PANE_ID pane_id;

    {
    RECT client_rect;
    GetClientRect(p_view->main[WIN_BACK].hwnd, &client_rect);
    p_view->outer_frame_gdi_rect.tl.x = client_rect.left;
    p_view->outer_frame_gdi_rect.tl.y = client_rect.top;
    p_view->outer_frame_gdi_rect.br.x = client_rect.right;
    p_view->outer_frame_gdi_rect.br.y = client_rect.bottom;
    } /*block*/

    {
    MSG_FRAME_WINDOW msg_frame_window;
    msg_frame_window.p_view = p_view;
    msg_frame_window.gdi_rect = p_view->outer_frame_gdi_rect;
    status_assert(object_call_all(p_docu, T5_MSG_FRAME_WINDOWS_DESCRIBE, &msg_frame_window));
    p_view->inner_frame_gdi_rect = msg_frame_window.gdi_rect;
    } /*block*/

    if(p_view->flags.horz_border_on)
        border.y = p_view->horz_border_gdi_height;
    if(p_view->flags.vert_border_on)
        border.x = p_view->vert_border_gdi_width;

    if(p_view->flags.horz_ruler_on)
        ruler.y = p_view->horz_ruler_gdi_height;
    if(p_view->flags.vert_ruler_on)
        ruler.x = p_view->vert_ruler_gdi_width;

    inner.x = inner.y = 0;

    start.x = p_view->inner_frame_gdi_rect.tl.x + (ruler.x + border.x);
    start.y = p_view->inner_frame_gdi_rect.tl.y + (ruler.y + border.y);
    outer.x = max.x = MAX(0, (p_view->inner_frame_gdi_rect.br.x - start.x) - ho_win_state.metrics.scroll.x);
    outer.y = max.y = MAX(0, (p_view->inner_frame_gdi_rect.br.y - start.y) - ho_win_state.metrics.scroll.y);

    if(p_view->flags.horz_split_on || p_view->flags.vert_split_on)
    {
        GDI_POINT split_point;
        PIXIT_POINT split_pixit_point;

        split_pixit_point.x = p_view->horz_split_pos;
        split_pixit_point.y = p_view->vert_split_pos;

        window_point_from_pixit_point(&split_point, &split_pixit_point, &p_view->host_xform[XFORM_BACK]);

        if(p_view->flags.horz_split_on)
        {
            outer.x = MAX(0, max.x - ((int) split_point.x + ho_win_state.metrics.split.x));
            inner.x = MIN(max.x, (int) split_point.x);
        }

        if(p_view->flags.vert_split_on)
        {
            outer.y = MAX(0, max.y - ((int) split_point.y + ho_win_state.metrics.split.y));
            inner.y = MIN(max.y, (int) split_point.y);
        }
    }

    p_view->pane_posn.split.x = (max.x - outer.x) + start.x;
    p_view->pane_posn.split.y = (max.y - outer.y) + start.y;

    p_view->pane_posn.grab.x = MAX(start.x, (p_view->pane_posn.split.x - (ho_win_state.metrics.split.x /* *2*/)));
    p_view->pane_posn.grab.y = MAX(start.y, (p_view->pane_posn.split.y - (ho_win_state.metrics.split.y /* *2*/)));

    p_view->pane_posn.start = start;

    p_view->pane_posn.inner_size = inner;
    p_view->pane_posn.outer_size = outer;

    p_view->pane_posn.max.x = start.x + max.x;
    p_view->pane_posn.max.y = start.y + max.y;

    p_view->pane_posn.grab_size = ho_win_state.metrics.split;

    p_view->flags.pseudo_horz_split_on = (outer.x <= 0);
    p_view->flags.pseudo_vert_split_on = (outer.y <= 0);

    if(p_view->flags.pseudo_horz_split_on)
        p_view->pane_posn.grab_size.x += PIXEL_HORZ;
    if(p_view->flags.pseudo_vert_split_on)
        p_view->pane_posn.grab_size.y += PIXEL_VERT;

    /* Compute the bounding boxes for the four pane windows */
    for(pane_id = PANE_ID_START; pane_id < NUMPANE; PANE_ID_INCR(pane_id))
    {
        const P_PANE p_pane = &p_view->pane[pane_id];

        if(HOST_WND_NONE == p_pane->hwnd)
            continue;

        switch(pane_id)
        {
        default: default_unhandled();
        case WIN_PANE:
            set_rect(&p_pane->rect, p_view->pane_posn.split.x, p_view->pane_posn.split.y, p_view->pane_posn.outer_size.x, p_view->pane_posn.outer_size.y);

            if(!p_view->flags.horz_split_on)
                p_pane->rect.left += PIXEL_HORZ;
            if(!p_view->flags.vert_split_on)
                p_pane->rect.top += PIXEL_VERT;

            break;

        case WIN_PANE_SPLIT_DIAG:
            set_rect(&p_pane->rect, p_view->pane_posn.start.x, p_view->pane_posn.start.y, p_view->pane_posn.inner_size.x, p_view->pane_posn.inner_size.y);

            p_pane->rect.left += PIXEL_HORZ;
            p_pane->rect.top += PIXEL_VERT;
            break;

        case WIN_PANE_SPLIT_HORZ:
            set_rect(&p_pane->rect, p_view->pane_posn.start.x, p_view->pane_posn.split.y, p_view->pane_posn.inner_size.x, p_view->pane_posn.outer_size.y);

            if(!p_view->flags.vert_split_on)
                p_pane->rect.top += PIXEL_VERT;

            p_pane->rect.left += PIXEL_HORZ;
            break;

        case WIN_PANE_SPLIT_VERT:
            set_rect(&p_pane->rect, p_view->pane_posn.split.x, p_view->pane_posn.start.y, p_view->pane_posn.outer_size.x, p_view->pane_posn.inner_size.y);

            if(!p_view->flags.horz_split_on)
                 p_pane->rect.left += PIXEL_HORZ;

            p_pane->rect.top += PIXEL_VERT;
            break;
        }
    }

    for(pane_id = PANE_ID_START; pane_id < NUMPANE; PANE_ID_INCR(pane_id))
    {
        const P_PANE p_pane = &p_view->pane[pane_id];

        if(HOST_WND_NONE == p_pane->hwnd)
            continue;

        p_pane->size.x = p_pane->rect.right  - p_pane->rect.left;
        p_pane->size.y = p_pane->rect.bottom - p_pane->rect.top;

        {
        GDI_RECT gdi_rect;
        gdi_rect.tl = p_pane->scroll;
        gdi_rect.br.x = gdi_rect.tl.x + p_pane->size.x;
        gdi_rect.br.y = gdi_rect.tl.y + p_pane->size.y;
        pixit_rect_from_window_rect(&p_pane->visible_pixit_rect, &gdi_rect, &p_view->host_xform[XFORM_PANE]);
        } /*block*/

        view_visible_skel_rect(p_docu, p_view, &p_pane->visible_pixit_rect, &p_pane->visible_skel_rect);
        skel_visible_row_range(p_docu, p_view, &p_pane->visible_skel_rect, &p_pane->visible_row_range);
    }

    for(pane_id = PANE_ID_START; pane_id < NUMPANE; PANE_ID_INCR(pane_id))
    {
        const P_PANE p_pane = &p_view->pane[pane_id];

        if(HOST_WND_NONE == p_pane->hwnd)
            continue;

        scroll_pane_absolute(p_docu, p_view, pane_to_event_handler_table[pane_id].event_handler, &p_pane->scroll, TRUE);
    }

    {
    VIEWEVENT_VISIBLEAREA_CHANGED event_info;

    event_info.p_view = event_info.visible_area.p_view = p_view;

    event_info.visible_area.rect.tl.x =
    event_info.visible_area.rect.tl.y = +HUGENUMBER;
    event_info.visible_area.rect.br.x =
    event_info.visible_area.rect.br.y = -HUGENUMBER;

    for(pane_id = PANE_ID_START; pane_id < NUMPANE; PANE_ID_INCR(pane_id))
    {
        const P_PANE p_pane = &p_view->pane[pane_id];

        if(HOST_WND_NONE == p_pane->hwnd)
            continue;

        event_info.visible_area.rect.tl.x = MIN(event_info.visible_area.rect.tl.x, p_pane->visible_pixit_rect.tl.x);
        event_info.visible_area.rect.tl.y = MIN(event_info.visible_area.rect.tl.y, p_pane->visible_pixit_rect.tl.y);

        event_info.visible_area.rect.br.x = MAX(event_info.visible_area.rect.br.x, p_pane->visible_pixit_rect.br.x);
        event_info.visible_area.rect.br.y = MAX(event_info.visible_area.rect.br.y, p_pane->visible_pixit_rect.br.y);
    }

    view_event_pane_window(p_docu, T5_EVENT_VISIBLEAREA_CHANGED, &event_info);
    } /*block*/
}

/* Set the window positions as required.  This assumes that the
 * calc pane pos function has been called to compute the
 * back window relative co-ordinates.  We are responsible for
 * setting the positions of the panes and their slaves.
 */

static void
set_pane_posn(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    POINT border_tl, ruler_tl;
    PANE_ID pane_id;

    {
    MSG_FRAME_WINDOW msg_frame_window;
    msg_frame_window.p_view = p_view;
    msg_frame_window.gdi_rect = p_view->outer_frame_gdi_rect;
    status_assert(object_call_all(p_docu, T5_MSG_FRAME_WINDOWS_POSN, &msg_frame_window));
    } /*block*/

    ruler_tl.x = border_tl.x = p_view->inner_frame_gdi_rect.tl.x;
    ruler_tl.y = border_tl.y = p_view->inner_frame_gdi_rect.tl.y;

    if(p_view->flags.horz_ruler_on)
        border_tl.y += p_view->horz_ruler_gdi_height;
    if(p_view->flags.vert_ruler_on)
        border_tl.x += p_view->vert_ruler_gdi_width;

    for(pane_id = PANE_ID_START; pane_id < NUMPANE; PANE_ID_INCR(pane_id))
    {
        const P_PANE p_pane = &p_view->pane[pane_id];

        if(HOST_WND_NONE == p_pane->hwnd)
            continue;

        move_pane(p_view, pane_id, p_pane->size.x && p_pane->size.y);

        if(p_view->flags.horz_border_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_HORZ)))
            move_slave(p_view, (pane_id == WIN_PANE) ? WIN_BORDER_HORZ : WIN_BORDER_HORZ_SPLIT, p_pane->rect.left, border_tl.y, (int) p_pane->size.x, p_view->horz_border_gdi_height);
        if(p_view->flags.vert_border_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_VERT)))
            move_slave(p_view, (pane_id == WIN_PANE) ? WIN_BORDER_VERT : WIN_BORDER_VERT_SPLIT, border_tl.x, p_pane->rect.top, p_view->vert_border_gdi_width, (int) p_pane->size.y);

        if(p_view->flags.horz_ruler_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_HORZ)))
            move_slave(p_view, (pane_id == WIN_PANE) ? WIN_RULER_HORZ : WIN_RULER_HORZ_SPLIT, p_pane->rect.left, ruler_tl.y, (int) p_pane->size.x, p_view->horz_ruler_gdi_height);
        if(p_view->flags.vert_ruler_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_VERT)))
            move_slave(p_view, (pane_id == WIN_PANE) ? WIN_RULER_VERT : WIN_RULER_VERT_SPLIT, ruler_tl.x, p_pane->rect.top, p_view->vert_ruler_gdi_width, (int) p_pane->size.y);
    }
}

/* Given the pane ID position it based on the information stored
 * within the view.  There is a bounding rectangle defined,
 * then attach the widgets to the pane as required.
 */

static void
move_pane(
    _ViewRef_   P_VIEW p_view,
    _In_        PANE_ID pane_id,
    _InVal_     BOOL show)
{
    const P_PANE p_pane = &p_view->pane[pane_id];
    POINT edge;

    if(HOST_WND_NONE == p_pane->hwnd)
        return;

    edge.x = p_view->pane[p_view->flags.pseudo_horz_split_on ? WIN_PANE_SPLIT_VERT : WIN_PANE].rect.right;
    edge.y = p_view->pane[p_view->flags.pseudo_vert_split_on ? WIN_PANE_SPLIT_HORZ:  WIN_PANE].rect.bottom;

    switch(pane_id)
    {
    default: default_unhandled();
#if CHECKING
    case WIN_PANE_SPLIT_DIAG:
#endif
        break;

    case WIN_PANE:
        {
        POINT start;

        start.x = p_view->pane_posn.grab.x + p_view->pane_posn.grab_size.x;
        start.y = p_view->pane_posn.grab.y + p_view->pane_posn.grab_size.y;

        move_slave(p_view, WIN_HSCROLL, start.x /*- PIXEL_HORZ*/, edge.y, (p_pane->rect.right - start.x) + PIXEL_HORZ2, ho_win_state.metrics.scroll.y);
        move_slave(p_view, WIN_VSCROLL, edge.x, start.y /*- PIXEL_VERT*/, ho_win_state.metrics.scroll.x, (p_pane->rect.bottom - start.y) + PIXEL_VERT2);

        break;
        }

    case WIN_PANE_SPLIT_HORZ:
        move_slave(p_view, WIN_HSCROLL_SPLIT_HORZ, p_view->pane_posn.start.x, edge.y, (p_view->pane_posn.grab.x - p_view->pane_posn.start.x) + PIXEL_HORZ, ho_win_state.metrics.scroll.y);
        break;

    case WIN_PANE_SPLIT_VERT:
        move_slave(p_view, WIN_VSCROLL_SPLIT_VERT, edge.x, p_view->pane_posn.start.y, ho_win_state.metrics.scroll.x, (p_view->pane_posn.grab.y - p_view->pane_posn.start.y) + PIXEL_VERT);
        break;
    }

    SetWindowPos(p_pane->hwnd, HWND_TOP, p_pane->rect.left, p_pane->rect.top, (int) p_pane->size.x, (int) p_pane->size.y,
        SWP_NOZORDER | SWP_NOACTIVATE | (show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}

/* Cope with moving a slave window (i.e. a scroll bar or a border window).  First
 * validate that the pane index is valid, i.e. the handle is non-zero
 * and then cause the window to be moved.
 */

static void
move_slave(
    _ViewRef_   P_VIEW p_view,
    _In_        int slave_id,
    _In_        int x,
    _In_        int y,
    _In_        int w,
    _In_        int h)
{
    if(HOST_WND_NONE == p_view->edge[slave_id].hwnd)
        return;

    SetWindowPos(p_view->edge[slave_id].hwnd, HWND_TOP, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

/* Internal function to scroll a pane by the given
 * offset and then re-cache the displayed
 * row / column information.
 */

static void
scroll_and_recache(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        PANE_ID pane_id,
    _InRef_     PC_GDI_POINT p_offset)
{
    const P_PANE p_pane = &p_view->pane[pane_id];

    {
    GDI_RECT gdi_rect;
    gdi_rect.tl = p_pane->scroll;
    gdi_rect.br.x = gdi_rect.tl.x + p_pane->size.x;
    gdi_rect.br.y = gdi_rect.tl.y + p_pane->size.y;
    pixit_rect_from_window_rect(&p_pane->visible_pixit_rect, &gdi_rect, &p_view->host_xform[XFORM_PANE]);
    } /*block*/

    view_visible_skel_rect(p_docu, p_view, &p_pane->visible_pixit_rect, &p_pane->visible_skel_rect);
    skel_visible_row_range(p_docu, p_view, &p_pane->visible_skel_rect, &p_pane->visible_row_range);

    {
    VIEWEVENT_VISIBLEAREA_CHANGED event_info;
    PANE_ID pane_id_loop;

    event_info.p_view = event_info.visible_area.p_view = p_view;

    event_info.visible_area.rect.tl.x =
    event_info.visible_area.rect.tl.y = +HUGENUMBER;
    event_info.visible_area.rect.br.x =
    event_info.visible_area.rect.br.y = -HUGENUMBER;

    for(pane_id_loop = PANE_ID_START; pane_id_loop < NUMPANE; PANE_ID_INCR(pane_id_loop))
    {
        const PC_PANE p_pane_loop = &p_view->pane[pane_id_loop];

        if(HOST_WND_NONE == p_pane_loop->hwnd)
            continue;

        event_info.visible_area.rect.tl.x = MIN(event_info.visible_area.rect.tl.x, p_pane_loop->visible_pixit_rect.tl.x);
        event_info.visible_area.rect.tl.y = MIN(event_info.visible_area.rect.tl.y, p_pane_loop->visible_pixit_rect.tl.y);
        event_info.visible_area.rect.br.x = MAX(event_info.visible_area.rect.br.x, p_pane_loop->visible_pixit_rect.br.x);
        event_info.visible_area.rect.br.y = MAX(event_info.visible_area.rect.br.y, p_pane_loop->visible_pixit_rect.br.y);
    }

    view_event_pane_window(p_docu, T5_EVENT_VISIBLEAREA_CHANGED, &event_info);
    } /*block*/

    assert(NULL != p_pane->hwnd);
    ScrollWindow(p_pane->hwnd, (int) p_offset->x, (int) p_offset->y, NULL, NULL);
    UpdateWindow(p_pane->hwnd);
}

/* Scroll the pane, scrolling the required slaves and updating the scroll position as is needed */

static void
scroll_pane(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler,
    _InVal_     S32 new_scroll)
{
    const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
    const P_PANE p_pane = &p_view->pane[p_host_event_desc->pane_id];
    GDI_POINT point;
    GDI_POINT offset;

    point.x = (p_host_event_desc->set_scroll_x) ? MIN(new_scroll, p_pane->extent.x - p_pane->size.x) : p_pane->scroll.x;
    point.y = (p_host_event_desc->set_scroll_y) ? MIN(new_scroll, p_pane->extent.y - p_pane->size.y) : p_pane->scroll.y;
    point.x = MAX(point.x, 0);
    point.y = MAX(point.y, 0);

    offset.x = p_pane->scroll.x - point.x;
    offset.y = p_pane->scroll.y - point.y;

    trace_2(TRACE_WINDOWS_HOST, TEXT("scroll_pane: event_handler %d, new scroll posn ") S32_TFMT, event_handler, new_scroll);
    trace_1(TRACE_WINDOWS_HOST, TEXT("scroll_pane: event handler yields slave id %d"), p_host_event_desc->slave_id);
    trace_1(TRACE_WINDOWS_HOST, TEXT("scroll_pane: event handler yields HWND ") HOST_WND_XTFMT, p_view->edge[p_host_event_desc->slave_id].hwnd);

    if(offset.x || offset.y)
    {
        if(p_host_event_desc->set_scroll_x)
            p_pane->scroll.x = p_view->pane[p_host_event_desc->pane_id ^ 2].scroll.x = point.x;
        if(p_host_event_desc->set_scroll_y)
            p_pane->scroll.y = p_view->pane[p_host_event_desc->pane_id ^ 1].scroll.y = point.y;

        scroll_and_recache(p_docu, p_view, p_host_event_desc->pane_id, &offset);

        if(p_host_event_desc->set_scroll_x)
        {
            if(HOST_WND_NONE != p_view->pane[p_host_event_desc->pane_id ^ 2].hwnd)
                scroll_and_recache(p_docu, p_view, (PANE_ID) (p_host_event_desc->pane_id ^ 2), &offset);

            if(p_view->flags.horz_border_on)
            {
                ScrollWindow(p_view->edge[p_host_event_desc->border_id].hwnd, (int) offset.x, 0, NULL, NULL);
                UpdateWindow(p_view->edge[p_host_event_desc->border_id].hwnd);
            }

            if(p_view->flags.horz_ruler_on)
            {
                ScrollWindow(p_view->edge[p_host_event_desc->ruler_id].hwnd, (int) offset.x, 0, NULL, NULL);
                UpdateWindow(p_view->edge[p_host_event_desc->ruler_id].hwnd);
            }
        }

        if(p_host_event_desc->set_scroll_y)
        {
            if(HOST_WND_NONE != p_view->pane[p_host_event_desc->pane_id ^ 1].hwnd)
                scroll_and_recache(p_docu, p_view, (PANE_ID) (p_host_event_desc->pane_id ^ 1), &offset);

            if(p_view->flags.vert_border_on)
            {
                ScrollWindow(p_view->edge[p_host_event_desc->border_id].hwnd, 0, (int) offset.y, NULL, NULL);
                UpdateWindow(p_view->edge[p_host_event_desc->border_id].hwnd);
            }

            if(p_view->flags.vert_ruler_on)
            {
                ScrollWindow(p_view->edge[p_host_event_desc->ruler_id].hwnd, 0, (int) offset.y, NULL, NULL);
                UpdateWindow(p_view->edge[p_host_event_desc->ruler_id].hwnd);
            }
        }

        SetScrollPos(p_view->edge[p_host_event_desc->slave_id].hwnd,
                     SB_CTL,
                     point_to_blip_pos(p_host_event_desc->set_scroll_x ? p_pane->scroll.x : p_pane->scroll.y,
                                       p_host_event_desc->set_scroll_x ? p_pane->extent.x - p_pane->size.x : p_pane->extent.y - p_pane->size.y),
                     TRUE);
    }
}

/* Scroll the given pane in the view to the specified position.  The
 * scroll co-ordinates must still be clipped suitably before
 * being passed around.
 *
 * We are given the view and event handler and a new pixel position.
 */

static void
scroll_pane_absolute(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler,
    _InRef_     PC_GDI_POINT p_gdi_point,
    _InVal_     BOOL repaint_scroll_bars)
{
    const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
    const PC_HOST_PANE_DESC p_host_pane_desc = &host_pane_desc_table[p_host_event_desc->pane_id];
    const P_PANE p_pane = &p_view->pane[p_host_event_desc->pane_id];
    GDI_POINT scroll = *p_gdi_point;
    GDI_POINT offset;

    trace_2(TRACE_WINDOWS_HOST, TEXT("scroll_pane_absolute: scroll x = ") S32_TFMT TEXT(", y = ") S32_TFMT, scroll.x, scroll.y);
    trace_2(TRACE_WINDOWS_HOST, TEXT("scroll_pane_absolute: top value x = ") S32_TFMT TEXT(", y = ") S32_TFMT, p_pane->extent.x - p_pane->size.x, p_pane->extent.y - p_pane->size.y);

    scroll.x = MIN(scroll.x, p_pane->extent.x - p_pane->size.x);
    scroll.x = MAX(scroll.x, 0);
    scroll.y = MIN(scroll.y, p_pane->extent.y - p_pane->size.y);
    scroll.y = MAX(scroll.y, 0);

    trace_2(TRACE_WINDOWS_HOST, TEXT("scroll_pane_absolute: scroll position x = ") S32_TFMT TEXT(", y = ") S32_TFMT, scroll.x, scroll.y);

    offset.x = p_pane->scroll.x - scroll.x;
    offset.y = p_pane->scroll.y - scroll.y;

    trace_2(TRACE_WINDOWS_HOST, TEXT("scroll_pane_absolute: offset x = ") S32_TFMT TEXT(", y = ") S32_TFMT, offset.x, offset.y);

    if(offset.x || offset.y)
    {
        p_pane->scroll.x = p_view->pane[p_host_event_desc->pane_id ^ 2].scroll.x = scroll.x;
        p_pane->scroll.y = p_view->pane[p_host_event_desc->pane_id ^ 1].scroll.y = scroll.y;

        trace_2(TRACE_WINDOWS_HOST, TEXT("scroll_pane_absolute: new x positions: ") S32_TFMT TEXT(" ") S32_TFMT, p_pane->scroll.x, p_view->pane[p_host_event_desc->pane_id ^ 2].scroll.x);
        trace_2(TRACE_WINDOWS_HOST, TEXT("scroll_pane_absolute: new y positions: ") S32_TFMT TEXT(" ") S32_TFMT, p_pane->scroll.y, p_view->pane[p_host_event_desc->pane_id ^ 1].scroll.y);

        scroll_and_recache(p_docu, p_view, p_host_event_desc->pane_id, &offset);

        if(offset.x)
        {
            GDI_POINT movement;
            int pos;

            movement.x = offset.x;
            movement.y = 0;

            if(HOST_WND_NONE != p_view->pane[p_host_event_desc->pane_id ^ 2].hwnd)
                scroll_and_recache(p_docu, p_view, (PANE_ID) (p_host_event_desc->pane_id ^ 2), &movement);

            if(p_view->flags.horz_border_on)
            {
                ScrollWindow(p_view->edge[p_host_pane_desc->hborder_id].hwnd, (int) movement.x, 0, NULL, NULL);
                UpdateWindow(p_view->edge[p_host_pane_desc->hborder_id].hwnd);
            }

            if(p_view->flags.horz_ruler_on)
            {
                ScrollWindow(p_view->edge[p_host_pane_desc->hruler_id].hwnd, (int) movement.x, 0, NULL, NULL);
                UpdateWindow(p_view->edge[p_host_pane_desc->hruler_id].hwnd);
            }

            pos = point_to_blip_pos(p_gdi_point->x, p_pane->extent.x - p_pane->size.x);

            SetScrollPos(p_view->edge[p_host_pane_desc->hscroll_id].hwnd, SB_CTL, pos, repaint_scroll_bars);
        }

        if(offset.y)
        {
            GDI_POINT movement;
            int pos;

            movement.x = 0;
            movement.y = offset.y;

            if(HOST_WND_NONE != p_view->pane[p_host_event_desc->pane_id ^ 1].hwnd)
                scroll_and_recache(p_docu, p_view, (PANE_ID) (p_host_event_desc->pane_id ^ 1), &movement);

            if(p_view->flags.vert_border_on)
            {
                ScrollWindow(p_view->edge[p_host_pane_desc->vborder_id].hwnd, 0, (int) movement.y, NULL, NULL);
                UpdateWindow(p_view->edge[p_host_pane_desc->vborder_id].hwnd);
            }

            if(p_view->flags.vert_ruler_on)
            {
                ScrollWindow(p_view->edge[p_host_pane_desc->vruler_id].hwnd, 0, (int) movement.y, NULL, NULL);
                UpdateWindow(p_view->edge[p_host_pane_desc->vruler_id].hwnd);
            }

            pos = point_to_blip_pos(p_gdi_point->y, p_pane->extent.y - p_pane->size.y);

            SetScrollPos(p_view->edge[p_host_pane_desc->vscroll_id].hwnd, SB_CTL, pos, repaint_scroll_bars);
        }
    }
}

/* Given a point and the required extent then attempt to convert it back to a sensible scroll value based on the defined scroll range */

_Check_return_
static int
point_to_blip_pos(
    _InVal_     S32 point,
    _InVal_     S32 range)
{
    int pos;

    trace_2(TRACE_WINDOWS_HOST, TEXT("point_to_blip_pos: point ") S32_TFMT TEXT(", range ") S32_TFMT, point, range);

    if(range)
    {
        pos = (int) muldiv64(point, SCROLL_RANGE, range);
        pos = MIN(pos, SCROLL_RANGE);
        pos = MAX(pos, 0);
    }
    else
        pos = 0;

    trace_1(TRACE_WINDOWS_HOST, TEXT("point_to_blip_pos: result %d"), pos);
    return(pos);
}

/* Given the blip position return a suitable value that indicates its position within the world we are dealing with */

_Check_return_
static S32
point_from_blip_pos(
    _InVal_     int pos,
    _InVal_     S32 range)
{
    S32 point;

    trace_2(TRACE_WINDOWS_HOST, TEXT("point_from_blip_pos: pos %d, range ") S32_TFMT, pos, range);

    if(range)
    {
        point = muldiv64(range, pos, SCROLL_RANGE);
        point = MIN(point, range);
        point = MAX(point, 0);
    }
    else
        point = 0;

    trace_1(TRACE_WINDOWS_HOST, TEXT("point_from_blip_pos: result ") S32_TFMT, point);
    return(point);
}

/* Attempt to paint in the back window for the viewer.  This includes painting
 * the grey and black areas as required.  The back window has a hollow
 * fill style to remove flicker, we are responsible for painting in the
 * grey region between the panes and outlining the windows as required.
 */

static void
host_onPaint_back_window(
    _ViewRef_   P_VIEW p_view,
    _InRef_     PPAINTSTRUCT p_paintstruct)
{
    const HDC hdc = p_paintstruct->hdc;
    HBRUSH ltgrey_brush = GetStockBrush(LTGRAY_BRUSH);
    HBRUSH black_brush = GetStockBrush(BLACK_BRUSH);
    POINT split_outer;
    RECT rect;

    split_outer.x = p_view->pane_posn.grab.x + p_view->pane_posn.grab_size.x;
    split_outer.y = p_view->pane_posn.grab.y + p_view->pane_posn.grab_size.y;

    SetRect(&rect, p_view->inner_frame_gdi_rect.tl.x, p_view->inner_frame_gdi_rect.tl.y, p_view->pane_posn.start.x, p_view->pane_posn.start.y);
    FillRect(hdc, &rect, ltgrey_brush);
    SetRect(&rect, p_view->pane_posn.max.x, p_view->inner_frame_gdi_rect.tl.y, p_view->inner_frame_gdi_rect.br.x, p_view->pane_posn.start.y);
    FillRect(hdc, &rect, ltgrey_brush);
    SetRect(&rect, p_view->pane_posn.max.x, p_view->pane_posn.max.y, p_view->inner_frame_gdi_rect.br.x, p_view->inner_frame_gdi_rect.br.y);
    FillRect(hdc, &rect, ltgrey_brush);
    SetRect(&rect, p_view->inner_frame_gdi_rect.tl.x, p_view->pane_posn.max.y, p_view->pane_posn.start.x, p_view->inner_frame_gdi_rect.br.y);
    FillRect(hdc, &rect, ltgrey_brush);

    if(p_view->flags.horz_split_on && (!p_view->flags.pseudo_horz_split_on))
    {
        SetRect(&rect, p_view->pane_posn.grab.x, p_view->inner_frame_gdi_rect.tl.y, split_outer.x, p_view->pane_posn.max.y);
        FillRect(hdc, &rect, ltgrey_brush);
    }

    if(p_view->flags.vert_split_on && (!p_view->flags.pseudo_vert_split_on))
    {
        SetRect(&rect, p_view->inner_frame_gdi_rect.tl.x, p_view->pane_posn.grab.y, p_view->pane_posn.max.x, split_outer.y);
        FillRect(hdc, &rect, ltgrey_brush);
    }

    /* Black the split pick up points */
    SetRect(&rect, p_view->pane_posn.grab.x, p_view->pane_posn.max.y, split_outer.x, p_view->inner_frame_gdi_rect.br.y);
    FillRect(hdc, &rect, black_brush);
    SetRect(&rect, p_view->pane_posn.max.x, p_view->pane_posn.grab.y, p_view->inner_frame_gdi_rect.br.x, split_outer.y);
    FillRect(hdc, &rect, black_brush);

    { /* Frame around all the panes in the view */
    HPEN old_pen = SelectPen(hdc, GetStockPen(BLACK_PEN));
    PANE_ID pane_id;

    for(pane_id = PANE_ID_START; pane_id < NUMPANE; PANE_ID_INCR(pane_id))
    {
        const P_PANE p_pane = &p_view->pane[pane_id];

        if(HOST_WND_NONE == p_pane->hwnd)
            continue;

        if((p_view->flags.horz_border_on || p_view->flags.horz_ruler_on) && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_HORZ)))
        {
            MoveToEx(hdc, p_pane->rect.left  - PIXEL_HORZ, p_view->inner_frame_gdi_rect.tl.y, NULL);
            LineTo(  hdc, p_pane->rect.left  - PIXEL_HORZ, p_view->pane_posn.start.y);

            MoveToEx(hdc, p_pane->rect.right,              p_view->inner_frame_gdi_rect.tl.y, NULL);
            LineTo(  hdc, p_pane->rect.right,              p_view->pane_posn.start.y);
        }

        if((p_view->flags.vert_border_on || p_view->flags.vert_ruler_on) && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_VERT)))
        {
            MoveToEx(hdc, p_view->inner_frame_gdi_rect.tl.x, p_pane->rect.top - PIXEL_VERT, NULL);
            LineTo(  hdc, p_view->pane_posn.start.x,         p_pane->rect.top - PIXEL_VERT);

            MoveToEx(hdc, p_view->inner_frame_gdi_rect.tl.x, p_pane->rect.bottom, NULL);
            LineTo(  hdc, p_view->pane_posn.start.x,         p_pane->rect.bottom);
        }

        SetRect(&rect, p_pane->rect.left - PIXEL_HORZ, p_pane->rect.top - PIXEL_VERT, p_pane->rect.right + PIXEL_HORZ, p_pane->rect.bottom + PIXEL_VERT);

        FrameRect(hdc, &rect, black_brush);
    }

    SelectPen(hdc, old_pen);
    } /*block*/
}

/* Back window has been resized so attempt to refresh it,
 * to do this we need to update its contents.  This
 * gets called when the window is resized or the split
 * information is changed.
 *
 * We attempt to work out the minimal area that
 * can be refreshed and force and update of that.
 */

static void
refresh_back_window(
    _ViewRef_   P_VIEW p_view)
{
    const HWND hwnd = p_view->main[WIN_BACK].hwnd;
    POINT split_inner, split_outer;
    RECT rect;

    split_inner.x = (p_view->flags.vert_split_on && (!p_view->flags.pseudo_horz_split_on)) ? p_view->inner_frame_gdi_rect.tl.x : p_view->pane_posn.max.x;
    split_inner.y = (p_view->flags.horz_split_on && (!p_view->flags.pseudo_vert_split_on)) ? p_view->inner_frame_gdi_rect.tl.y : p_view->pane_posn.max.y;

    split_outer.x = p_view->pane_posn.grab.x + p_view->pane_posn.grab_size.x;
    split_outer.y = p_view->pane_posn.grab.y + p_view->pane_posn.grab_size.y;

    SetRect(&rect, p_view->inner_frame_gdi_rect.tl.x, p_view->inner_frame_gdi_rect.tl.y, p_view->pane_posn.start.x, p_view->pane_posn.start.y);
    InvalidateRect(hwnd, &rect, FALSE);
    SetRect(&rect, p_view->pane_posn.max.x, p_view->inner_frame_gdi_rect.tl.y, p_view->inner_frame_gdi_rect.br.x, p_view->pane_posn.start.y);
    InvalidateRect(hwnd, &rect, FALSE);
    SetRect(&rect, p_view->pane_posn.max.x, p_view->pane_posn.max.y, p_view->inner_frame_gdi_rect.br.x, p_view->inner_frame_gdi_rect.br.y);
    InvalidateRect(hwnd, &rect, FALSE);
    SetRect(&rect, p_view->inner_frame_gdi_rect.tl.x, p_view->pane_posn.max.y, p_view->pane_posn.start.x, p_view->inner_frame_gdi_rect.br.y);
    InvalidateRect(hwnd, &rect, FALSE);

    SetRect(&rect, p_view->pane_posn.grab.x, split_inner.y, split_outer.x, p_view->inner_frame_gdi_rect.br.y);
    InvalidateRect(hwnd, &rect, FALSE);
    SetRect(&rect, split_inner.x, p_view->pane_posn.grab.y, p_view->inner_frame_gdi_rect.br.x, split_outer.y);
    InvalidateRect(hwnd, &rect, FALSE);

    SetRect(&rect, p_view->pane_posn.start.x, p_view->pane_posn.start.y, p_view->pane_posn.max.x, p_view->pane_posn.start.y + PIXEL_VERT);
    InvalidateRect(hwnd, &rect, FALSE);
    SetRect(&rect, p_view->pane_posn.start.x, p_view->pane_posn.start.y, p_view->pane_posn.start.x + PIXEL_HORZ, p_view->pane_posn.max.y);
    InvalidateRect(hwnd, &rect, FALSE);
}

static void
update_pane_window(
    _HwndRef_   HWND hwnd,
    _InVal_     REDRAW_FLAGS redraw_flags)
{
    // update using a special set of flags defined by our fine selves
    ho_win_state.update.redraw_flags_valid = TRUE;
    ho_win_state.update.redraw_flags = redraw_flags;
    UpdateWindow(hwnd);
    ho_win_state.update.redraw_flags_valid = FALSE;
}

/* Tidy the required panes attached to the view, this is mainly to do with
 * the extra window gadgets required, i.e. split views and rulers and
 * table borders.
 */

_Check_return_
static STATUS
ensure_pane_set(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    {
    PANE_ID pane_id = WIN_PANE;
    const HWND hwnd = p_view->pane[pane_id].hwnd;
    if(HOST_WND_NONE == hwnd)
        status_return(view_pane_create(p_docu, p_view, pane_id));
    } /*block*/

    { /* Ensure pane set for horizontally split views */
    PANE_ID pane_id = WIN_PANE_SPLIT_HORZ;
    const HWND hwnd = p_view->pane[pane_id].hwnd;
    if(p_view->flags.horz_split_on)
    {
        if(HOST_WND_NONE == hwnd)
            status_return(view_pane_create(p_docu, p_view, pane_id));
    }
    else if(HOST_WND_NONE != hwnd)
        view_pane_destroy(p_view, pane_id);
    } /*block*/

    { /* Ensure pane window for vertically split views */
    PANE_ID pane_id = WIN_PANE_SPLIT_VERT;
    const HWND hwnd = p_view->pane[pane_id].hwnd;
    if(p_view->flags.vert_split_on)
    {
        if(HOST_WND_NONE == hwnd)
            status_return(view_pane_create(p_docu, p_view, pane_id));
    }
    else if(HOST_WND_NONE != hwnd)
        view_pane_destroy(p_view, pane_id);
    } /*block*/

    { /* If both vertically and horizontally split then make the extra window */
    PANE_ID pane_id = WIN_PANE_SPLIT_DIAG;
    const HWND hwnd = p_view->pane[pane_id].hwnd;
    if(p_view->flags.horz_split_on && p_view->flags.vert_split_on)
    {
        if(HOST_WND_NONE == hwnd)
            status_return(view_pane_create(p_docu, p_view, pane_id));
    }
    else if(HOST_WND_NONE != hwnd)
        view_pane_destroy(p_view, pane_id);
    } /*block*/

    { /* Now make the slave windows for rulers and borders */
    PANE_ID pane_id = NUMPANE;

    do  {
        PANE_ID_DECR(pane_id);

        if(HOST_WND_NONE != p_view->pane[pane_id].hwnd)
        {
            /* Loop through destroying the rulers and borders as required */
            if(!p_view->flags.horz_border_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_HORZ)))
                slave_window_destroy(p_view, (pane_id == WIN_PANE) ? WIN_BORDER_HORZ : WIN_BORDER_HORZ_SPLIT);
            if(!p_view->flags.vert_border_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_VERT)))
                slave_window_destroy(p_view, (pane_id == WIN_PANE) ? WIN_BORDER_VERT : WIN_BORDER_VERT_SPLIT);

            if(!p_view->flags.horz_ruler_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_HORZ)))
                slave_window_destroy(p_view, (pane_id == WIN_PANE) ? WIN_RULER_HORZ : WIN_RULER_HORZ_SPLIT);
            if(!p_view->flags.vert_ruler_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_VERT)))
                slave_window_destroy(p_view, (pane_id == WIN_PANE) ? WIN_RULER_VERT : WIN_RULER_VERT_SPLIT);

            /* Loop through ensuring that rulers etc. have been created */
            if(p_view->flags.horz_border_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_HORZ)))
                status_return(slave_window_create(p_docu, p_view, (pane_id == WIN_PANE) ? WIN_BORDER_HORZ : WIN_BORDER_HORZ_SPLIT));
            if(p_view->flags.vert_border_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_VERT)))
                status_return(slave_window_create(p_docu, p_view, (pane_id == WIN_PANE) ? WIN_BORDER_VERT : WIN_BORDER_VERT_SPLIT));

            if(p_view->flags.horz_ruler_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_HORZ)))
                status_return(slave_window_create(p_docu, p_view, (pane_id == WIN_PANE) ? WIN_RULER_HORZ : WIN_RULER_HORZ_SPLIT));
            if(p_view->flags.vert_ruler_on && ((pane_id == WIN_PANE) || (pane_id == WIN_PANE_SPLIT_VERT)))
                status_return(slave_window_create(p_docu, p_view, (pane_id == WIN_PANE) ? WIN_RULER_VERT : WIN_RULER_VERT_SPLIT));
        }

    } while(PANE_ID_START != pane_id);
    } /*block*/

    return(STATUS_OK);
}

/*
loop over all documents and see if there are any views open
*/

_Check_return_
extern BOOL
some_document_windows(void)
{
    S32 n_windows = 0;
    S32 n_iconic_windows = 0;
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
    {
        const P_DOCU p_docu = p_docu_from_docno_valid(docno);
        VIEWNO viewno = VIEWNO_NONE;
        P_VIEW p_view;

        while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
        {
            const HWND hwnd = p_view->main[WIN_BACK].hwnd;

            if(!IsWindow(hwnd))
                continue;

            ++n_windows;

            if(IsIconic(hwnd))
                ++n_iconic_windows;
        }
    }

    if(0 == n_windows)
    {
        return(FALSE);
    }

    return(TRUE);
}

static void
host_onPaint_pane_window(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PPAINTSTRUCT p_paintstruct,
    _InVal_     EVENT_HANDLER event_handler)
{
    const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
    const P_PANE p_pane = &p_view->pane[p_host_event_desc->pane_id];
    REDRAW_CONTEXT_CACHE redraw_context_cache;
    VIEWEVENT_REDRAW viewevent_redraw;
    const P_VIEWEVENT_REDRAW p_viewevent_redraw = &viewevent_redraw;
    const P_REDRAW_CONTEXT p_redraw_context = &p_viewevent_redraw->redraw_context;

    zero_struct_ptr(p_viewevent_redraw);

    zero_struct(redraw_context_cache);
    p_redraw_context->p_redraw_context_cache = &redraw_context_cache;

    // define the redraw flags from the pending update
    if(ho_win_state.update.redraw_flags_valid)
        p_viewevent_redraw->flags = ho_win_state.update.redraw_flags;
    else
    {
        p_viewevent_redraw->flags.show_content = TRUE;
        p_viewevent_redraw->flags.show_selection = TRUE;
    }

    p_viewevent_redraw->p_view = p_view;
    p_viewevent_redraw->p_pane = p_pane;
    p_viewevent_redraw->area.p_view = p_view;

    p_redraw_context->p_docu = p_docu;
    p_redraw_context->p_view = p_view;

    p_redraw_context->windows.paintstruct = *p_paintstruct;

    p_redraw_context->display_mode = p_view->display_mode;

    p_redraw_context->border_width.x = p_redraw_context->border_width.y = p_docu->page_def.grid_size;
    p_redraw_context->border_width_2.x = p_redraw_context->border_width_2.y = 2 * p_docu->page_def.grid_size;

    host_redraw_context_set_host_xform(p_redraw_context, &p_view->host_xform[p_host_event_desc->xform_index]);

    host_redraw_context_fillin(p_redraw_context);

    // Convert the scroll posn to a suitable origin
    p_redraw_context->gdi_org.x = (p_host_event_desc->scroll_x) ? p_pane->scroll.x : 0;
    p_redraw_context->gdi_org.y = (p_host_event_desc->scroll_y) ? p_pane->scroll.y : 0;

    host_paint_start(p_redraw_context);

    p_redraw_context->windows.host_machine_clip_rect = p_redraw_context->windows.paintstruct.rcPaint;

    { // Convert the redraw rectangle to a suitable view rectangle
    GDI_RECT gdi_rect;
    gdi_rect.tl.x = p_redraw_context->windows.paintstruct.rcPaint.left   + p_redraw_context->gdi_org.x;
    gdi_rect.tl.y = p_redraw_context->windows.paintstruct.rcPaint.top    + p_redraw_context->gdi_org.y;
    gdi_rect.br.x = p_redraw_context->windows.paintstruct.rcPaint.right  + p_redraw_context->gdi_org.x;
    gdi_rect.br.y = p_redraw_context->windows.paintstruct.rcPaint.bottom + p_redraw_context->gdi_org.y;
    pixit_rect_from_window_rect(&p_viewevent_redraw->area.rect, &gdi_rect, &p_redraw_context->host_xform);
    } /*block*/

    (* p_host_event_desc->p_proc_event) (p_docu, T5_EVENT_REDRAW, &viewevent_redraw);

    host_paint_end(p_redraw_context);
}

_Check_return_
extern STATUS
clip_render_format(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UINT format);

_Check_return_
extern STATUS
clip_render_all_formats(
    _DocuRef_   P_DOCU p_docu);

static void
host_onCreate(
    _HwndRef_   HWND hwnd,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler)
{
    /* store the window handle ref as soon as it becomes valid */
    HWND * p_hwnd = PtrAddBytes(HWND *, p_view, host_event_desc_table[event_handler].offset_of_hwnd);
    *p_hwnd = hwnd;
}

_Check_return_
static BOOL
wndproc_host_onCreate(
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
        host_onCreate(hwnd, p_view, event_handler);
        return(TRUE);
    }

    return(FALSE);
}

static void
host_onDestroy(
    _HwndRef_   HWND hwnd,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler)
{
    /* destroy the window handle ref as soon as it becomes invalid */
    HWND * p_hwnd = PtrAddBytes(HWND *, p_view, host_event_desc_table[event_handler].offset_of_hwnd);
    *p_hwnd = NULL;

    if(ho_win_state.caret.visible.hwnd == hwnd) /* SKS after 1.08b2 */
    {
        ho_win_state.caret.visible.hwnd = NULL;
        ho_win_state.caret.visible.dimensions.y = 0;
    }
}

static void
wndproc_host_onDestroy(
    _HwndRef_   HWND hwnd)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        RemoveProp(hwnd, TYPE5_PROPERTY_WORD); /* AFTER resolve_hwnd!!! */
        host_onDestroy(hwnd, p_view, event_handler);
        return;
    }

    FORWARD_WM_DESTROY(hwnd, DefWindowProc);
}

static void
host_onPaint(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler,
    _InRef_     PPAINTSTRUCT p_paintstruct)
{
    BOOL call_maeve = TRUE;

    switch(event_handler)
    {
    case EVENT_HANDLER_BACK_WINDOW:
        host_onPaint_back_window(p_view, p_paintstruct);
        break;

    default: default_unhandled();
#if CHECKING
    case EVENT_HANDLER_BORDER_HORZ:
    case EVENT_HANDLER_BORDER_VERT:
    case EVENT_HANDLER_BORDER_HORZ_SPLIT:
    case EVENT_HANDLER_BORDER_VERT_SPLIT:
    case EVENT_HANDLER_RULER_HORZ:
    case EVENT_HANDLER_RULER_VERT:
    case EVENT_HANDLER_RULER_HORZ_SPLIT:
    case EVENT_HANDLER_RULER_VERT_SPLIT:
#endif
        call_maeve = FALSE;

        /*FALLTHRU*/

    case EVENT_HANDLER_PANE:
    case EVENT_HANDLER_PANE_SPLIT_VERT:
    case EVENT_HANDLER_PANE_SPLIT_HORZ:
    case EVENT_HANDLER_PANE_SPLIT_DIAG:
        host_onPaint_pane_window(p_docu, p_view, p_paintstruct, event_handler);

        if(call_maeve)
            status_assert(maeve_event(p_docu, T5_EVENT_REDRAW_AFTER, P_DATA_NONE));

        break;
    }
}

static void
wndproc_host_onPaint(
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
        host_onPaint(p_docu_from_docno_valid(docno), p_view, event_handler, &paintstruct);
    }

    hard_assert(FALSE);

    EndPaint(hwnd, &paintstruct);
}

static void
back_window_onSize(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    calc_pane_posn(p_docu, p_view);
    set_pane_posn(p_docu, p_view);
    refresh_back_window(p_view);
}

static void
host_onSize(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler)
{
    switch(event_handler)
    {
    case EVENT_HANDLER_BACK_WINDOW:
        back_window_onSize(p_docu, p_view);
        return;

    default:
        return;
    }
}

static void
wndproc_host_onSize(
    _HwndRef_   HWND hwnd,
    UINT state,
    int cx,
    int cy)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {   /* I think this is only called during first ShowWindow after creation now we handle WM_WINDOWPOSCHANGED */
        host_onSize(p_docu_from_docno_valid(docno), p_view, event_handler);
        return;
    }

    FORWARD_WM_SIZE(hwnd, state, cx, cy, DefWindowProc);
}

static void
wndproc_host_onWindowPosChanged(
    _HwndRef_   HWND hwnd,
    _In_        const WINDOWPOS * const lpwpos)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        if(NULL == lpwpos->hwnd)
            return; /* seem to get this during DestroyWindow() */
        host_onSize(p_docu_from_docno_valid(docno), p_view, event_handler);
        return; /* none of these windows require WM_SIZE and WM_MOVE so don't call DefWindowProc */
    }

    FORWARD_WM_WINDOWPOSCHANGED(hwnd, lpwpos, DefWindowProc);
}

#if defined(USE_GLOBAL_CLIPBOARD)

DOCNO  g_global_clipboard_owning_docno  = DOCNO_NONE;
VIEWNO g_global_clipboard_owning_viewno = VIEWNO_NONE;

_Check_return_
extern BOOL
host_acquire_global_clipboard(
    _DocuRef_   PC_DOCU p_docu,
    _ViewRef_   PC_VIEW p_view)
{
    const DOCNO acquiring_docno = docno_from_p_docu(p_docu);
    const VIEWNO acquiring_viewno = viewno_from_p_view_fn(p_view);
    HWND hwndClipOwner = NULL;
    BOOL res;

    trace_2(TRACE_WINDOWS_HOST, TEXT("host_acquire_global_clipboard(docno=") DOCNO_TFMT TEXT(", viewno=") DOCNO_TFMT TEXT(")"), acquiring_docno, acquiring_viewno);

    if(!IS_VIEW_NONE(p_view))
    {
        hwndClipOwner = p_view->main[WIN_BACK].hwnd;
        assert(p_view->docno == acquiring_docno);
    }

    res = WrapOsBoolChecking(OpenClipboard(hwndClipOwner)); /* NULL -> this task will own */
    trace_1(TRACE_WINDOWS_HOST, TEXT("host_acquire_global_clipboard: OpenClipboard res=%s"), report_boolstring(res));

    if(res)
    {
        trace_0(TRACE_WINDOWS_HOST, TEXT("host_acquire_global_clipboard: EmptyClipboard"));
        EmptyClipboard(); /* we may immediately get WM_DESTROYCLIPBOARD if this window has owned clipboard even if it has already emptied it */

        g_global_clipboard_owning_docno  = acquiring_docno;
        g_global_clipboard_owning_viewno = acquiring_viewno;
        trace_2(TRACE_WINDOWS_HOST, TEXT("host_acquire_global_clipboard: cbo docno:=%d, viewno:=%d"), g_global_clipboard_owning_docno, g_global_clipboard_owning_viewno);
    }
    else
    {
        g_global_clipboard_owning_docno  = DOCNO_NONE;
        g_global_clipboard_owning_viewno = VIEWNO_NONE;
        trace_0(TRACE_WINDOWS_HOST, TEXT("host_acquire_global_clipboard: cbo docno:=NONE, viewno:=NONE"));
    }

    return(res);
}

extern void
host_release_global_clipboard(
    _InVal_     BOOL render_if_acquired)
{
    trace_1(TRACE_WINDOWS_HOST, TEXT("host_release_global_clipboard(render_if_acquired=%s)"), report_boolstring(render_if_acquired));
    trace_2(TRACE_WINDOWS_HOST, TEXT("host_release_global_clipboard: cbo docno=") DOCNO_TFMT TEXT(", viewno=") DOCNO_TFMT TEXT(""), g_global_clipboard_owning_docno, g_global_clipboard_owning_viewno);

    if(DOCNO_NONE != g_global_clipboard_owning_docno)
    {
        P_DOCU global_clipboard_owning_p_docu = p_docu_from_docno(g_global_clipboard_owning_docno);
        P_VIEW global_clipboard_owning_p_view = p_view_from_viewno(global_clipboard_owning_p_docu, g_global_clipboard_owning_viewno);

        if(host_open_global_clipboard(global_clipboard_owning_p_docu, global_clipboard_owning_p_view))
        {
            if(render_if_acquired)
            {
                trace_0(TRACE_WINDOWS_HOST, TEXT("host_release_global_clipboard: clip_render_all_formats"));
                status_assert(clip_render_all_formats(global_clipboard_owning_p_docu));
            }
            else
            {
                trace_0(TRACE_WINDOWS_HOST, TEXT("host_release_global_clipboard: EmptyClipboard"));
                EmptyClipboard(); /* we will immediately get WM_DESTROYCLIPBOARD */
            }
        }
    }

    g_global_clipboard_owning_docno  = DOCNO_NONE;
    g_global_clipboard_owning_viewno = VIEWNO_NONE;
    trace_0(TRACE_WINDOWS_HOST, TEXT("host_release_global_clipboard: cbo docno:=NONE, viewno:=NONE"));
}

_Check_return_
extern BOOL
host_open_global_clipboard(
    _DocuRef_   PC_DOCU p_docu,
    _ViewRef_   PC_VIEW p_view)
{
    HWND hwndClipOwner = NULL;
    BOOL res;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    trace_2(TRACE_WINDOWS_HOST, TEXT("host_open_global_clipboard(docno=") DOCNO_TFMT TEXT(", viewno=") DOCNO_TFMT TEXT(")"), docno_from_p_docu(p_docu), viewno_from_p_view_fn(p_view));

    if(!IS_VIEW_NONE(p_view))
    {
        hwndClipOwner = p_view->main[WIN_BACK].hwnd;
        assert(p_view->docno == docno_from_p_docu(p_docu));
    }

    res = WrapOsBoolChecking(OpenClipboard(hwndClipOwner));
    trace_1(TRACE_WINDOWS_HOST, TEXT("host_open_global_clipboard: OpenClipboard res=%s"), report_boolstring(res));
    return(res);
}

static void
host_onClose_process_clipboard(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    trace_2(TRACE_WINDOWS_HOST, TEXT("host_onClose: msg docno=") DOCNO_TFMT TEXT(", viewno=") DOCNO_TFMT TEXT(""), docno_from_p_docu(p_docu), viewno_from_p_view(p_view));
    trace_2(TRACE_WINDOWS_HOST, TEXT("host_onClose: cbo docno=") DOCNO_TFMT TEXT(", viewno=") DOCNO_TFMT TEXT(""), g_global_clipboard_owning_docno, g_global_clipboard_owning_viewno);

    if( (g_global_clipboard_owning_docno  != docno_from_p_docu(p_docu))  ||
        (g_global_clipboard_owning_viewno != viewno_from_p_view(p_view)) )
    {
        return;
    }

    /* about to try to close the window that currently owns the clipboard */ 
#if 1
    /* careful inspection of the Windows messages shows we will get
     * WM_CLOSE/WM_RENDERALLFORMATS/WM_DESTROYCLIPBOARD/WM_DESTROY
     * so that the WM_RENDERALLFORMATS still has a valid P_DOCU (but no P_VIEW)
     */
#else
    /* render all we can before it goes awry */
    if(host_open_global_clipboard(p_docu, p_view))
    {
        trace_0(TRACE_WINDOWS_HOST, TEXT("host_onClose: clip_render_all_formats"));
        status_assert(clip_render_all_formats(p_docu));
    }

    g_global_clipboard_owning_docno  = DOCNO_NONE;
    g_global_clipboard_owning_viewno = VIEWNO_NONE;
    trace_0(TRACE_WINDOWS_HOST, TEXT("host_onClose: cbo docno:=NONE, viewno:=NONE"));
#endif
}

#endif /* USE_GLOBAL_CLIPBOARD */

static void
host_onClose(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
#if defined(USE_GLOBAL_CLIPBOARD)
    host_onClose_process_clipboard(p_docu, p_view);
#endif /* USE_GLOBAL_CLIPBOARD */

    process_close_request(p_docu, p_view, FALSE, TRUE /*closing_a_view*/, FALSE);

    if(!some_document_windows())
    {
        PostQuitMessage(0);
    }
}

static void
wndproc_host_onClose(
    _HwndRef_   HWND hwnd)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        host_onClose(p_docu_from_docno_valid(docno), p_view);
        return;
    }

    FORWARD_WM_CLOSE(hwnd, DefWindowProc);
}

static void
host_onButtonDown(
    _HwndRef_   HWND hwnd,
    _InVal_     UINT uiMsg,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler)
{
    const BOOL ctrl_pressed = (0 != (keyFlags & MK_CONTROL));
    const BOOL shift_pressed = (0 != (keyFlags & MK_SHIFT));

    status_line_auto_clear(p_docu);

    status_assert(host_key_cache_emit_events());

    switch(event_handler)
    {
    case EVENT_HANDLER_PANE:
    case EVENT_HANDLER_PANE_SPLIT_HORZ:
    case EVENT_HANDLER_PANE_SPLIT_VERT:
    case EVENT_HANDLER_PANE_SPLIT_DIAG:
        {
        const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
        const PANE_ID pane_id = p_host_event_desc->pane_id;
        //const P_PANE p_pane = &p_view->pane[pane_id];

        p_docu->viewno_caret = viewno_from_p_view(p_view);
        p_view->cur_pane = pane_id;

        send_click_to_view(p_docu, p_view, hwnd, uiMsg, x, y, event_handler, ctrl_pressed, shift_pressed);
        break;
        }

    default: default_unhandled();
#if CHECKING
    case EVENT_HANDLER_BACK_WINDOW:
    case EVENT_HANDLER_BORDER_HORZ_SPLIT:
    case EVENT_HANDLER_BORDER_HORZ:
    case EVENT_HANDLER_BORDER_VERT_SPLIT:
    case EVENT_HANDLER_BORDER_VERT:
    case EVENT_HANDLER_RULER_HORZ_SPLIT:
    case EVENT_HANDLER_RULER_HORZ:
    case EVENT_HANDLER_RULER_VERT_SPLIT:
    case EVENT_HANDLER_RULER_VERT:
#endif
        send_click_to_view(p_docu, p_view, hwnd, uiMsg, x, y, event_handler, ctrl_pressed, shift_pressed);
        break;
    }
}

static void
wndproc_host_onLButtonDown(
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
        const UINT uiMsg = fDoubleClick ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN;

        if(ho_win_state.drag.enabled)
            return;

        host_onButtonDown(hwnd, uiMsg, x, y, keyFlags, p_docu_from_docno_valid(docno), p_view, event_handler);
        return;
    }

    FORWARD_WM_LBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, DefWindowProc);
}

/* When the button comes up stop the drag monitor, and silence any pending drags */

static void
host_onButtonUp_dragging(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    const BOOL ctrl_pressed = (0 != (keyFlags & MK_CONTROL)); /* these may change during drag */
    const BOOL shift_pressed = (0 != (keyFlags & MK_SHIFT));
    P_DOCU p_docu = p_docu_from_docno(ho_win_state.drag.docno);
    P_VIEW p_view = p_view_from_viewno(p_docu, ho_win_state.drag.viewno);
    GDI_POINT point;

    assert(ho_win_state.drag.hwnd == hwnd);
    UNREFERENCED_PARAMETER_HwndRef_(hwnd);

    GetMessagePosAsClient(ho_win_state.drag.hwnd, &point.x, &point.y);
    UNREFERENCED_PARAMETER_InVal_(x);
    UNREFERENCED_PARAMETER_InVal_(y);
    assert(point.x == x);
    assert(point.y == y);

    ReleaseCapture();

    trace_1(TRACE__NULL, TEXT("WM_xBUTTONUP - terminating drag monitor - *** null_events_stop(docno=") DOCNO_TFMT TEXT(")"), ho_win_state.drag.docno);
    null_events_stop(ho_win_state.drag.docno, T5_MSG_WIN_HOST_NULL, null_event_ho_win, HO_WIN_DRAG_NULL_CLIENT_HANDLE);

    /* inform the current owner of the pending doom */
    trace_1(TRACE_WINDOWS_HOST, TEXT("WM_xBUTTONUP: *** TERMINATING DRAG in docno=") DOCNO_TFMT TEXT(" ***"), ho_win_state.drag.docno);
    send_mouse_event(p_docu, p_view, T5_EVENT_CLICK_DRAG_FINISHED, ho_win_state.drag.hwnd, point.x, point.y, ctrl_pressed, shift_pressed, ho_win_state.drag.event_handler);
    ho_win_state.drag.enabled = FALSE;
}

static void
wndproc_host_onLButtonUp(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    stop_drag_monitor();

    if(ho_win_state.drag.enabled)
    {
        host_onButtonUp_dragging(hwnd, x, y, keyFlags);
        return;
    }

    FORWARD_WM_LBUTTONUP(hwnd, x, y, keyFlags, DefWindowProc);
}

static void
wndproc_host_onRButtonDown(
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
        const UINT uiMsg = fDoubleClick ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN;

        if(ho_win_state.drag.enabled)
            return;

        host_onButtonDown(hwnd, uiMsg, x, y, keyFlags, p_docu_from_docno_valid(docno), p_view, event_handler);
        return;
    }

    FORWARD_WM_RBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, DefWindowProc);
}

static void
wndproc_host_onRButtonUp(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    stop_drag_monitor();

    if(ho_win_state.drag.enabled)
    {
        host_onButtonUp_dragging(hwnd, x, y, keyFlags);
        return;
    }

    FORWARD_WM_RBUTTONUP(hwnd, x, y, keyFlags, DefWindowProc);
}

/* Process drop files request */

_Check_return_
static STATUS
host_onDropFiles_process_one(
    HWND hwnd,
    HDROP hdrop,
    UINT wIndex,
    _InVal_     BOOL ctrl_pressed,
    _InVal_     BOOL shift_pressed,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler)
{
    TCHARZ szFileName[256];
    POINT gpointDrop; /* Point where the files were dropped */
    const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
    VIEWEVENT_CLICK viewevent_click;
    STATUS status;

    DragQueryPoint(hdrop, &gpointDrop); // Retrieve the window coordinates of the mouse pointer when the drop was made

    DragQueryFile(hdrop, wIndex, szFileName, elemof32(szFileName));

    zero_struct(viewevent_click);

    /* setup the click context information and mouse point */
    viewevent_click.click_context.hwnd = hwnd;
    viewevent_click.click_context.ctrl_pressed = ctrl_pressed;
    viewevent_click.click_context.shift_pressed = shift_pressed;
    viewevent_click.click_context.gdi_org.x = 0; /* window-relative */
    viewevent_click.click_context.gdi_org.y = 0;
    {
    const PC_PANE p_pane = (p_host_event_desc->pane_id >= 0) ? &p_view->pane[p_host_event_desc->pane_id] : NULL;
    if(NULL != p_pane)
    {
        if(p_host_event_desc->scroll_x)
            viewevent_click.click_context.gdi_org.x = p_pane->scroll.x;
        if(p_host_event_desc->scroll_y)
            viewevent_click.click_context.gdi_org.y = p_pane->scroll.y;
    }
    } /*block*/
    host_set_click_context(p_docu, p_view, &viewevent_click.click_context, &p_view->host_xform[p_host_event_desc->xform_index]);

    {
    GDI_POINT gdi_point;
    gdi_point.x = gpointDrop.x;
    gdi_point.y = gpointDrop.y;
    view_point_from_window_point_and_context(&viewevent_click.view_point, &gdi_point, &viewevent_click.click_context);
    } /*block*/

    viewevent_click.data.fileinsert.filename = szFileName;
    viewevent_click.data.fileinsert.safesource = TRUE;

    viewevent_click.data.fileinsert.t5_filetype = t5_filetype_from_filename(viewevent_click.data.fileinsert.filename);

    status = (* p_host_event_desc->p_proc_event) (p_docu, T5_EVENT_FILEINSERT_DOINSERT, &viewevent_click);

    return(status);
}

static void
host_onDropFiles(
    _HwndRef_   HWND hwnd,
    HDROP hdrop,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler)
{
    const BOOL ctrl_pressed = host_ctrl_pressed(); /* cache at start - may take some time to process several files */
    const BOOL shift_pressed = host_shift_pressed();

    const DOCNO docno = docno_from_p_docu(p_docu);
    UINT gwFilesDropped = DragQueryFile(hdrop, (UINT) -1, NULL, 0); /* Total number of files dropped */
    UINT wIndex;

    status_line_auto_clear(p_docu);

    for(wIndex = 0; wIndex < gwFilesDropped; ++wIndex) /* Retrieve each file name and process */
    {
        status_break(host_onDropFiles_process_one(hwnd, hdrop, wIndex, ctrl_pressed, shift_pressed, p_docu_from_docno(docno) /* reload just in case */, p_view, event_handler));
    }

    DragFinish(hdrop);
}

static void
wndproc_host_onDropFiles(
    _HwndRef_   HWND hwnd,
    HDROP hdrop)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        host_onDropFiles(hwnd, hdrop, p_docu_from_docno_valid(docno), p_view, event_handler);
        return;
    }

    FORWARD_WM_DROPFILES(hwnd, hdrop, DefWindowProc);
}

/* Prepare the menu, before it is displayed - tick and grey items */

static void
wndproc_host_onInitMenu(
    _HwndRef_   HWND hwnd,
    HMENU hMenu)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        host_onInitMenu(hMenu, p_docu_from_docno_valid(docno), p_view);
        return;
    }

    FORWARD_WM_INITMENU(hwnd, hMenu, DefWindowProc);
}

/* Execute the command from the menu */

static void
host_onCommand(
    _InVal_     int id,
    _InVal_     UINT codeNotify,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    UNREFERENCED_PARAMETER_InVal_(codeNotify);

    switch(id)
    {
    default:
        if(id >= IDM_START)
        {
            status_line_auto_clear(p_docu);
            consume_bool(ho_execute_menu_command(p_docu, p_view, id, FALSE));
        }
        break;
    }
}

static void
wndproc_host_onCommand(
    _HwndRef_   HWND hwnd,
    _InVal_     int id,
    _HwndRef_   HWND hwndCtl,
    _InVal_     UINT codeNotify)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        host_onCommand(id, codeNotify, p_docu_from_docno_valid(docno), p_view);
        return;
    }

    FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, DefWindowProc);
}

static void
host_onSettingChange(
    _HwndRef_   HWND hwnd,
    _InVal_     UINT uiMsg,
    _InVal_     WPARAM wParam,
    _InVal_     LPARAM lParam,
    _DocuRef_   P_DOCU p_docu)
{
    MSG_FROM_WINDOWS msg_from_windows;
    zero_struct(msg_from_windows);
    msg_from_windows.hwnd = hwnd;
    msg_from_windows.uiMsg = uiMsg;
    msg_from_windows.wParam = wParam;
    msg_from_windows.lParam = lParam;
    status_assert(maeve_service_event(p_docu, T5_MSG_FROM_WINDOWS, &msg_from_windows));
}

static void
wndproc_host_onSettingChange(
    _HwndRef_   HWND hwnd,
    _InVal_     UINT uiMsg,
    _InVal_     WPARAM wParam,
    _InVal_     LPARAM lParam)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        host_onSettingChange(hwnd, uiMsg, wParam, lParam, p_docu_from_docno(docno));
        return;
    }

    /* no FORWARD_WM_SETTINGCHANGE() */
}

_Check_return_
static S32
host_HScroll_get_new_scroll(
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler,
    _InRef_     P_PANE p_pane)
{
    const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
    const HWND hwnd = p_view->edge[p_host_event_desc->slave_id].hwnd;
    return(point_from_blip_pos(GetScrollPos(hwnd, SB_CTL), p_pane->extent.x - p_pane->size.x));
}

/* Horizontal scroll bar is being modified */

static void
host_onHScroll(
    _InVal_     UINT code,
    _InVal_     int pos,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler)
{
    const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
    PANE_ID pane_id = p_host_event_desc->pane_id;
    const P_PANE p_pane = &p_view->pane[pane_id];
    S32 new_scroll = 0;
    BOOL do_it = TRUE;

    switch(code)
    {
    case SB_PAGERIGHT:
        new_scroll = host_HScroll_get_new_scroll(p_view, event_handler, p_pane) + p_pane->size.x;
        break;

    case SB_PAGELEFT:
        new_scroll = host_HScroll_get_new_scroll(p_view, event_handler, p_pane) - p_pane->size.x;
        break;

    case SB_LINERIGHT:
        new_scroll = host_HScroll_get_new_scroll(p_view, event_handler, p_pane) + ho_win_state.metrics.scroll.x *2;
        break;

    case SB_LINELEFT:
        new_scroll = host_HScroll_get_new_scroll(p_view, event_handler, p_pane) - ho_win_state.metrics.scroll.x *2;
        break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        new_scroll = point_from_blip_pos(pos, p_pane->extent.x - p_pane->size.x);
        break;

    case SB_RIGHT:
        new_scroll = p_pane->extent.x;
        break;

    case SB_LEFT:
        new_scroll = 0;
        break;

    default:
        do_it = FALSE;
        break;
    }

    if(!do_it)
        return;

    trace_1(TRACE_WINDOWS_HOST, TEXT("WM_HSCROLL: scroll position (post modify) %d"), new_scroll);
    scroll_pane(p_docu, p_view, event_handler, new_scroll);
}

static void
wndproc_host_onHScroll(
    _HwndRef_   HWND hwnd,
    _HwndRef_   HWND hwndCtl,
    _InVal_     UINT code,
    _InVal_     int pos)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwndCtl, &p_view, &event_handler)))
    {
        host_onHScroll(code, pos, p_docu_from_docno_valid(docno), p_view, event_handler);
        return;
    }

    FORWARD_WM_HSCROLL(hwnd, hwndCtl, code, pos, DefWindowProc);
}

_Check_return_
static S32
host_VScroll_get_new_scroll(
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler,
    _InRef_     P_PANE p_pane)
{
    const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
    const HWND hwnd = p_view->edge[p_host_event_desc->slave_id].hwnd;
    return(point_from_blip_pos(GetScrollPos(hwnd, SB_CTL), p_pane->extent.y - p_pane->size.y));
}

/* Vertical scroll bar is being modified */

static void
host_onVScroll(
    _InVal_     UINT code,
    _InVal_     int pos,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler)
{
    const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
    PANE_ID pane_id = p_host_event_desc->pane_id;
    const P_PANE p_pane = &p_view->pane[pane_id];
    S32 new_scroll = 0;
    BOOL do_it = TRUE;

    switch(code)
    {
    case SB_PAGEDOWN:
        new_scroll = host_VScroll_get_new_scroll(p_view, event_handler, p_pane) + p_pane->size.y;
        break;

    case SB_PAGEUP:
        new_scroll = host_VScroll_get_new_scroll(p_view, event_handler, p_pane) - p_pane->size.y;
        break;

    case SB_LINEDOWN:
        new_scroll = host_VScroll_get_new_scroll(p_view, event_handler, p_pane) + ho_win_state.metrics.scroll.y *2;
        break;

    case SB_LINEUP:
        new_scroll = host_VScroll_get_new_scroll(p_view, event_handler, p_pane) - ho_win_state.metrics.scroll.y *2;
        break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        new_scroll = point_from_blip_pos(pos, p_pane->extent.y - p_pane->size.y);
        break;

    case SB_BOTTOM:
        new_scroll = p_pane->extent.y;
        break;

    case SB_TOP:
        new_scroll = 0;
        break;

    default:
        do_it = FALSE;
        break;
    }

    if(!do_it)
        return;

    trace_1(TRACE_WINDOWS_HOST, TEXT("WM_VSCROLL: scroll position (post modify) %d"), new_scroll);
    scroll_pane(p_docu, p_view, event_handler, new_scroll);
}

static void
wndproc_host_onVScroll(
    _HwndRef_   HWND hwnd,
    _HwndRef_   HWND hwndCtl,
    _InVal_     UINT code,
    _InVal_     int pos)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwndCtl, &p_view, &event_handler)))
    {
        host_onVScroll(code, pos, p_docu_from_docno_valid(docno), p_view, event_handler);
        return;
    }

    FORWARD_WM_VSCROLL(hwnd, hwndCtl, code, pos, DefWindowProc);
}

static void
host_onMouseLeave(
    _HwndRef_   HWND hwnd,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler)
{
    int x, y;

    GetMessagePosAsClient(hwnd, &x, &y);

    send_mouse_event(p_docu, p_view, T5_EVENT_POINTER_LEAVES_WINDOW, hwnd, x, y, FALSE, FALSE, event_handler);
}

static void
wndproc_host_onMouseLeave(
    _HwndRef_   HWND hwnd)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    trace_1(TRACE_WINDOWS_HOST, TEXT("wndproc_host_onMouseLeave: hwnd ") HOST_WND_XTFMT, hwnd);

    host_clear_tracking_for_window(hwnd);

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        host_onMouseLeave(hwnd, p_docu_from_docno_valid(docno), p_view, event_handler);
        return;
    }

    /* no FORWARD_WM_MOUSELEAVE() */
}

/* Mouse movement has occurred, check for drag start / post pointer movement message */

static void
host_onMouseMove(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags,
    _DocuRef_   P_DOCU p_docu,
    /*_ViewRef_*/ P_VIEW p_view,
    /*_InVal_*/   EVENT_HANDLER event_handler)
{
    const BOOL ctrl_pressed = (0 != (keyFlags & MK_CONTROL)); /* these may change between click and drag start / during drag */
    const BOOL shift_pressed = (0 != (keyFlags & MK_SHIFT));

    if(host_set_tracking_for_window(hwnd))
    {
        GDI_POINT point;

        GetMessagePosAsClient(hwnd, &point.x, &point.y);
        assert(x == point.x);
        assert(y == point.y);

        trace_1(TRACE_WINDOWS_HOST, TEXT("wndproc_host: entering window ") HOST_WND_XTFMT, hwnd);

        send_mouse_event(p_docu, p_view, T5_EVENT_POINTER_ENTERS_WINDOW, hwnd, point.x, point.y, FALSE, FALSE, event_handler);
    }

    {
    T5_MESSAGE t5_message;

    if(ho_win_state.drag.pending)
    {
        /* If not forcing the drag to occur then check the distance between start and current */
        int dx = abs(ho_win_state.drag.start.x - x);
        int dy = abs(ho_win_state.drag.start.y - y);

        if((dx >= ho_win_state.drag.dist.x) || (dy >= ho_win_state.drag.dist.y))
        {
            trace_0(TRACE_WINDOWS_HOST, TEXT("WM_MOUSEMOVE: **** POSTING DRAG START ****"));
            do_drag_start(ctrl_pressed, shift_pressed);
        }
    }

    if(ho_win_state.drag.enabled)
    {
        send_drag_movement_and_maybe_scroll(x, y, ctrl_pressed, shift_pressed);
    }
    else
    {
        t5_message = T5_EVENT_POINTER_MOVEMENT;

        send_mouse_event(p_docu, p_view, t5_message, hwnd, x, y, FALSE, FALSE, event_handler);
    }
    } /*block*/
}

static void
wndproc_host_onMouseMove(
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
        host_onMouseMove(hwnd, x, y, keyFlags, p_docu_from_docno_valid(docno), p_view, event_handler);
        return;
    }

    FORWARD_WM_MOUSEMOVE(hwnd, x, y, keyFlags, DefWindowProc);
}

static void
host_onMouseWheel(
    _InVal_     int xPos,
    _InVal_     int yPos,
    _InVal_     int zDelta,
    _InVal_     UINT fwKeys,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler)
{
    POINT point;

    point.x = xPos;
    point.y = yPos;

#if 1
    /* I can see this being potentially useful when drag is enabled too */
#else
    if(ho_win_state.drag.enabled)
    {   /* This can be provoked by wheeling when the mouse button is still down during a drag. */
        return;
    }
#endif

    if(fwKeys & MK_CONTROL)
    {   /* Zoom the whole view (button towards screen -> negative delta -> smaller text, i.e. lower zoom factor) */
        view_set_zoom_from_wheel_delta(p_docu, p_view, (-zDelta * 10) / WHEEL_DELTA);
    }
    else
    {   /* Scroll the corresponding vertical scroll bar */
        const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
        const PC_HOST_PANE_DESC p_host_pane_desc = &host_pane_desc_table[p_host_event_desc->pane_id];
        const P_PANE p_pane = &p_view->pane[p_host_event_desc->pane_id];
        EVENT_HANDLER event_handler_scroll = p_host_pane_desc->vscroll_event_handler;
        S32 new_scroll, inc_scroll;

        inc_scroll = (ho_win_state.metrics.scroll.y * zDelta) / WHEEL_DELTA;

        new_scroll = p_pane->scroll.y - inc_scroll;

        scroll_pane(p_docu, p_view, event_handler_scroll, new_scroll);
    }
}

static void
wndproc_host_onMouseWheel(
    _HwndRef_   HWND hwnd_msg,
    _InVal_     int xPos,
    _InVal_     int yPos,
    _InVal_     int zDelta,
    _InVal_     UINT fwKeys)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    { /*block*/
    /* NB This message is sent to the focus window (i.e. the back window)
     * so we must decide who to send it to given the window that has the caret
     */
    HWND hwnd_caret = ho_win_state.caret.visible.hwnd;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd_caret, &p_view, &event_handler)))
    {
        host_onMouseWheel(xPos, yPos, zDelta, fwKeys, p_docu_from_docno_valid(docno), p_view, event_handler);
        return;
    }
    } /*block*/

    FORWARD_WM_MOUSEWHEEL(hwnd_msg, xPos, yPos, zDelta, fwKeys, DefWindowProc);
}

_Check_return_
static BOOL
host_onMouseHWheel(
    _InVal_     int xPos,
    _InVal_     int yPos,
    _InVal_     int zDelta,
    _InVal_     UINT fwKeys,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     EVENT_HANDLER event_handler)
{
    POINT point;

    point.x = xPos;
    point.y = yPos;

    if(ho_win_state.drag.enabled)
        return(FALSE);

    if(fwKeys & MK_CONTROL)
    {   /* Zoom the whole view (button towards screen -> negative delta -> smaller text, i.e. lower zoom factor) */
        view_set_zoom_from_wheel_delta(p_docu, p_view, (-zDelta * 10) / WHEEL_DELTA);
    }
    else
    {   /* Scroll the corresponding horizontal scroll bar */
        const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
        const PC_HOST_PANE_DESC p_host_pane_desc = &host_pane_desc_table[p_host_event_desc->pane_id];
        const P_PANE p_pane = &p_view->pane[p_host_event_desc->pane_id];
        EVENT_HANDLER event_handler_scroll = p_host_pane_desc->hscroll_event_handler;
        S32 new_scroll, inc_scroll;

        new_scroll = p_pane->scroll.x;

        inc_scroll = (ho_win_state.metrics.scroll.x * zDelta) / WHEEL_DELTA;

        new_scroll -= inc_scroll;

        scroll_pane(p_docu, p_view, event_handler_scroll, new_scroll);
    }

    return(TRUE); /* MUST indicate this to stop OS doing emulation */
}

_Check_return_
static BOOL
wndproc_host_onMouseHWheel(
    _HwndRef_   HWND hwnd_in,
    _InVal_     int xPos,
    _InVal_     int yPos,
    _InVal_     int zDelta,
    _InVal_     UINT fwKeys)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    { /*block*/
    /* NB This message is sent to the focus window (i.e. the back window)
     * so we must decide who to send it to given the window that has the caret
     */
    const HWND hwnd = ho_win_state.caret.visible.hwnd;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        return(host_onMouseHWheel(xPos, yPos, zDelta, fwKeys, p_docu_from_docno_valid(docno), p_view, event_handler));
    }
    } /*block*/

    UNREFERENCED_PARAMETER_HwndRef_(hwnd_in);

    return(FALSE); /* allow OS to do emulation */
}

/* Receiving focus, attempt to show the caret */
/* Called only when the window is already known */

static void
host_onSetFocus(
    _DocuRef_   P_DOCU p_docu)
{
    { /* ask objects if they're happy for a new document focus */
    const DOCNO docno = docno_from_p_docu(p_docu);
    OBJECT_ID object_id = OBJECT_ID_ENUM_START;
    DOCU_FOCUS_QUERY docu_focus_query;
    docu_focus_query.processed = FALSE;
    docu_focus_query.docno = docno;
    while(!docu_focus_query.processed && status_ok(object_next(&object_id)))
        status_break(object_call_id(object_id, P_DOCU_NONE, T5_MSG_DOCU_FOCUS_QUERY, &docu_focus_query));

    p_docu = p_docu_from_docno(docu_focus_query.docno);
    DOCU_ASSERT(p_docu);
    } /*block*/

    caret_show_claim(p_docu, p_docu->focus_owner, FALSE); /* MRJC 3.8.94 */
}

static void
wndproc_host_onSetFocus(
    _HwndRef_   HWND hwnd,
    _HwndRef_   HWND hwndOldFocus)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        trace_1(TRACE_WINDOWS_HOST, TEXT("WM_SETFOCUS: received, event handler %d"), event_handler);
        trace_1(TRACE_WINDOWS_HOST, TEXT("WM_SETFOCUS: cached hwnd ") HOST_WND_XTFMT, ho_win_state.caret.visible.hwnd);
        host_onSetFocus(p_docu_from_docno_valid(docno));
        return;
    }

    FORWARD_WM_SETFOCUS(hwnd, hwndOldFocus, DefWindowProc);
}

/* Losing the focus so destroy it and flag our internal state */

static void
host_onKillFocus(
    _DocuRef_   P_DOCU p_docu)
{
    status_assert(host_key_cache_emit_events_for(docno_from_p_docu(p_docu)));

    DestroyCaret();
}

static void
wndproc_host_onKillFocus(
    _HwndRef_   HWND hwnd,
    _HwndRef_   HWND hwndOldFocus)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        trace_1(TRACE_WINDOWS_HOST, TEXT("WM_KILLFOCUS: received, event handler %d"), event_handler);
        trace_1(TRACE_WINDOWS_HOST, TEXT("WM_KILLFOCUS: cached hwnd ") HOST_WND_XTFMT, ho_win_state.caret.visible.hwnd);
        host_onKillFocus(p_docu_from_docno_valid(docno));
        return;
    }

    FORWARD_WM_KILLFOCUS(hwnd, hwndOldFocus, DefWindowProc);
}

_Check_return_
static BOOL
wndproc_host_onSetCursor(
    _HwndRef_   HWND hwnd,
    _HwndRef_   HWND hwndCursor,
    _InVal_     UINT codeHitTest,
    _InVal_     UINT uiMsg)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        if(hwndCursor == hwnd)
        {
            switch(codeHitTest)
            {
            case HTCLIENT:
                ho_paint_SetCursor();
                return(TRUE);

            default:
                break;
            }
        }
    }

    return(FORWARD_WM_SETCURSOR(hwnd, hwndCursor, codeHitTest, uiMsg, DefWindowProc));
}

static void
host_onTimer_drag(void)
{
    /*POINT point;*/

    /*GetMessagePosAsClient(ho_win_state.drag.hwnd, &point);*/

    if(ho_win_state.drag.pending)
    {
        const BOOL ctrl_pressed = host_ctrl_pressed(); /* these may change between click and drag start */
        const BOOL shift_pressed = host_shift_pressed();

        trace_0(TRACE_WINDOWS_HOST, TEXT("WM_TIMER: **** POSTING DRAG START ****"));
        do_drag_start(ctrl_pressed, shift_pressed);
    }
}

static void
wndproc_host_onTimer(
    _HwndRef_   HWND hwnd,
    _InVal_     UINT id)
{
    /* Timer has elapsed, it may be time to start a drag or kill a triple click */
    trace_1(TRACE_WINDOWS_HOST, TEXT("WM_TIMER: timer id ") UINTPTR_XTFMT, (uintptr_t) id);

    switch(id)
    {
    case DRAG_TIMER_ID:
        host_onTimer_drag();
        return;

    case TRIPLE_CLICK_TIMER_ID:
        /* Triple click is timing out */
        disable_triple_click();
        return;

    default:
        FORWARD_WM_TIMER(hwnd, id, DefWindowProc);
        return;
    }
}

/* Use the key down event to monitor function keys */

_Check_return_
static BOOL
host_onKeyDown(
    _InVal_     UINT vk,
    _DocuRef_   P_DOCU p_docu)
{
    const KMAP_CODE kmap_code = kmap_convert(vk);

    if(0 == kmap_code)
        return(FALSE);

    status_line_auto_clear(p_docu);

    status_assert(send_key_to_docu(p_docu, kmap_code)); /* don't pass on unprocessed keys */

    /* there will be a WM_KEYUP to come so flush there in any case */
    return(TRUE);
}

static void
wndproc_host_onKeyUpDown(
    _HwndRef_   HWND hwnd,
    _InVal_     UINT vk,
    _InVal_     BOOL fDown,
    _InVal_     int cRepeat,
    _InVal_     UINT flags)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        if(fDown)
        {
            if(host_onKeyDown(vk, p_docu_from_docno_valid(docno)))
                return;
        }
        else /* fUp */
        {
            /* have a good flush here */
            if(!host_keys_in_buffer())
                status_assert(host_key_cache_emit_events());
        }

        return;
    }

    if(fDown)
        FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, DefWindowProc);
    else
        FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, DefWindowProc);
}

/* Keyboard event being entered into the keyboard, let it go around */

static void
host_onChar(
    _InVal_     TCHAR ch,
    _DocuRef_   P_DOCU p_docu)
{
    KMAP_CODE kmap_code = (KMAP_CODE) ch;

    if((kmap_code >= 0x20) && (kmap_code <= 0xFF) && (kmap_code != 0x7F))
    {
        docu_modify(p_docu);

        status_assert(host_key_cache_event(docno_from_p_docu(p_docu), kmap_code, FALSE, 1));
    }

    if(!host_keys_in_buffer())
        status_assert(host_key_cache_emit_events());
}

static void
wndproc_host_onChar(
    _HwndRef_   HWND hwnd,
    _InVal_     TCHAR ch,
    _InVal_     int cRepeat)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        host_onChar(ch, p_docu_from_docno_valid(docno));
        return;
    }

    FORWARD_WM_CHAR(hwnd, ch, cRepeat, DefWindowProc);
}

#if defined(USE_GLOBAL_CLIPBOARD)

/* someone is emptying the clipboard that this window allegedly owns */

static void
wndproc_host_onDestroyClipboard(
    _HwndRef_   HWND hwnd)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        /*EMPTY*/ /* we no longer own the clipboard, regardless */
    }

    trace_2(TRACE_WINDOWS_HOST, TEXT("WM_DESTROYCLIPBOARD: msg docno=") DOCNO_TFMT TEXT(", viewno=") DOCNO_TFMT TEXT(""), docno, viewno_from_p_view_fn(p_view));
    trace_2(TRACE_WINDOWS_HOST, TEXT("WM_DESTROYCLIPBOARD: cbo docno=") DOCNO_TFMT TEXT(", viewno=") DOCNO_TFMT TEXT(""), g_global_clipboard_owning_docno, g_global_clipboard_owning_viewno);

    if(DOCNO_NONE == g_global_clipboard_owning_docno)
    {   /* may already have been 'released' and emptied */
        assert(g_global_clipboard_owning_viewno == VIEWNO_NONE);
    }
    else
    {
        assert(g_global_clipboard_owning_docno  == docno);
        assert(g_global_clipboard_owning_viewno == viewno_from_p_view_fn(p_view));

        assert(g_global_clipboard_owning_docno  != DOCNO_NONE);
        assert(g_global_clipboard_owning_viewno != VIEWNO_NONE);
    }

    g_global_clipboard_owning_docno  = DOCNO_NONE;
    g_global_clipboard_owning_viewno = VIEWNO_NONE;
    trace_0(TRACE_WINDOWS_HOST, TEXT("WM_DESTROYCLIPBOARD: cbo docno:=NONE, viewno:=NONE"));
}

static HANDLE /* surely wrong */
wndproc_host_onRenderFormat(
    _HwndRef_   HWND hwnd,
    _InVal_     UINT uFormat)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;
    LRESULT lresult = 0L; /* default return value for processed case */

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        trace_2(TRACE_WINDOWS_HOST, TEXT("WM_RENDERFORMAT: received, event handler %d, format ") U32_XTFMT, event_handler, (U32) uFormat);
        trace_2(TRACE_WINDOWS_HOST, TEXT("WM_RENDERFORMAT: msg docno=") DOCNO_TFMT TEXT(", viewno=") DOCNO_TFMT TEXT(""), docno, viewno_from_p_view_fn(p_view));
        trace_2(TRACE_WINDOWS_HOST, TEXT("WM_RENDERFORMAT: cbo docno=") DOCNO_TFMT TEXT(", viewno=") DOCNO_TFMT TEXT(""), g_global_clipboard_owning_docno, g_global_clipboard_owning_viewno);

        if(DOCNO_NONE == g_global_clipboard_owning_docno)
        {   /* may already have been 'released' and emptied */
            assert(VIEWNO_NONE == g_global_clipboard_owning_viewno);
            trace_0(TRACE_WINDOWS_HOST, TEXT("WM_RENDERFORMAT: deny render request"));
            lresult = 1; /* deny render request */
        }
        else
        {
            const P_DOCU global_clipboard_owning_p_docu = p_docu_from_docno(g_global_clipboard_owning_docno);

            assert(g_global_clipboard_owning_docno  == docno);
            assert(g_global_clipboard_owning_viewno == viewno_from_p_view_fn(p_view));

            /* clipboard is already open ready to receive data */

            if(status_fail(clip_render_format(global_clipboard_owning_p_docu, uFormat)))
            {
                trace_0(TRACE_WINDOWS_HOST, TEXT("WM_RENDERFORMAT: clip_render_format failed"));
                lresult = 1; /* didn't work */
            }
        }

        return((HANDLE) lresult);
    }

    return(FORWARD_WM_RENDERFORMAT(hwnd, uFormat, DefWindowProc));
}

/* message sent to window that owns the clipboard */

static void
wndproc_host_onRenderAllFormats(
    _HwndRef_   HWND hwnd)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;
    LRESULT lresult = 0L; /* default return value for processed case */

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        trace_1(TRACE_WINDOWS_HOST, TEXT("WM_RENDERALLFORMATS: received, event handler %d"), event_handler);
        trace_2(TRACE_WINDOWS_HOST, TEXT("WM_RENDERALLFORMATS: msg docno=") DOCNO_TFMT TEXT(", viewno=") DOCNO_TFMT TEXT(""), docno, viewno_from_p_view_fn(p_view));
        trace_2(TRACE_WINDOWS_HOST, TEXT("WM_RENDERALLFORMATS: cbo docno=") DOCNO_TFMT TEXT(", viewno=") DOCNO_TFMT TEXT(""), g_global_clipboard_owning_docno, g_global_clipboard_owning_viewno);

        if(DOCNO_NONE == g_global_clipboard_owning_docno)
        {   /* may already have been 'released' and emptied */
            assert(VIEWNO_NONE == g_global_clipboard_owning_viewno);
            trace_0(TRACE_WINDOWS_HOST, TEXT("WM_RENDERALLFORMATS: deny render request"));
            lresult = 1; /* deny render request */
        }
        else
        {
            P_DOCU global_clipboard_owning_p_docu = p_docu_from_docno(g_global_clipboard_owning_docno);
            P_VIEW global_clipboard_owning_p_view = p_view_from_viewno(global_clipboard_owning_p_docu, g_global_clipboard_owning_viewno);

            assert(g_global_clipboard_owning_docno  == docno);
            assert(g_global_clipboard_owning_viewno == viewno_from_p_view_fn(p_view));

            if(host_open_global_clipboard(global_clipboard_owning_p_docu, global_clipboard_owning_p_view))
            {
                if(status_fail(clip_render_all_formats(global_clipboard_owning_p_docu)))
                {
                    trace_0(TRACE_WINDOWS_HOST, TEXT("WM_RENDERALLFORMATS: clip_render_all_formats failed"));
                    lresult = 1; /* didn't work */
                }
            }
            else
            {
                trace_0(TRACE_WINDOWS_HOST, TEXT("WM_RENDERALLFORMATS: can't lock clipboard for our use"));
                lresult = 1; /* can't lock clipboard for our use */
            }
        }
    }
}

#endif /* USE_GLOBAL_CLIPBOARD */

static LRESULT CALLBACK
wndproc_host(
    _HwndRef_   HWND hwnd,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam)
{
    switch(uiMsg)
    {
    HANDLE_MSG(hwnd, WM_CREATE,             wndproc_host_onCreate);
    HANDLE_MSG(hwnd, WM_DESTROY,            wndproc_host_onDestroy);
    HANDLE_MSG(hwnd, WM_PAINT,              wndproc_host_onPaint);
    HANDLE_MSG(hwnd, WM_SIZE,               wndproc_host_onSize);
    HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGED,   wndproc_host_onWindowPosChanged);
    HANDLE_MSG(hwnd, WM_CLOSE,              wndproc_host_onClose);
    HANDLE_MSG(hwnd, WM_LBUTTONDOWN,        wndproc_host_onLButtonDown);
    HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK,      wndproc_host_onLButtonDown);
    HANDLE_MSG(hwnd, WM_LBUTTONUP,          wndproc_host_onLButtonUp);
    HANDLE_MSG(hwnd, WM_RBUTTONDOWN,        wndproc_host_onRButtonDown);
    HANDLE_MSG(hwnd, WM_RBUTTONDBLCLK,      wndproc_host_onRButtonDown);
    HANDLE_MSG(hwnd, WM_RBUTTONUP,          wndproc_host_onRButtonUp);
    HANDLE_MSG(hwnd, WM_DROPFILES,          wndproc_host_onDropFiles);
    HANDLE_MSG(hwnd, WM_INITMENU,           wndproc_host_onInitMenu);
    HANDLE_MSG(hwnd, WM_COMMAND,            wndproc_host_onCommand);

    case WM_SETTINGCHANGE:
        wndproc_host_onSettingChange(hwnd, uiMsg, wParam, lParam);
        return(0L);

    HANDLE_MSG(hwnd, WM_HSCROLL,            wndproc_host_onHScroll);
    HANDLE_MSG(hwnd, WM_VSCROLL,            wndproc_host_onVScroll);
    HANDLE_MSG(hwnd, WM_MOUSELEAVE,         wndproc_host_onMouseLeave);
    HANDLE_MSG(hwnd, WM_MOUSEMOVE,          wndproc_host_onMouseMove);
    HANDLE_MSG(hwnd, WM_MOUSEWHEEL,         wndproc_host_onMouseWheel);

    case WM_MOUSEHWHEEL:
        return(wndproc_host_onMouseHWheel(hwnd, LOWORD(lParam), HIWORD(lParam), GET_WHEEL_DELTA_WPARAM(wParam), GET_KEYSTATE_WPARAM(wParam)));

    HANDLE_MSG(hwnd, WM_SETFOCUS,           wndproc_host_onSetFocus);
    HANDLE_MSG(hwnd, WM_KILLFOCUS,          wndproc_host_onKillFocus);
    HANDLE_MSG(hwnd, WM_SETCURSOR,          wndproc_host_onSetCursor);
    HANDLE_MSG(hwnd, WM_TIMER,              wndproc_host_onTimer);
    HANDLE_MSG(hwnd, WM_KEYDOWN,            wndproc_host_onKeyUpDown);
    HANDLE_MSG(hwnd, WM_KEYUP,              wndproc_host_onKeyUpDown);
    HANDLE_MSG(hwnd, WM_CHAR,               wndproc_host_onChar);

#if defined(USE_GLOBAL_CLIPBOARD)
    HANDLE_MSG(hwnd, WM_DESTROYCLIPBOARD,   wndproc_host_onDestroyClipboard);
    HANDLE_MSG(hwnd, WM_RENDERFORMAT,       wndproc_host_onRenderFormat);
    HANDLE_MSG(hwnd, WM_RENDERALLFORMATS,   wndproc_host_onRenderAllFormats);
#endif /* USE_GLOBAL_CLIPBOARD */

    default:
        return(host_top_level_window_event_handler(hwnd, uiMsg, wParam, lParam)); /* and so to DefWindowProc */
    }
}

_Check_return_
static STATUS
back_window_event_pointer(
    _InRef_     PC_VIEWEVENT_CLICK p_viewevent_click)
{
    /* Ensure point reflects current state of play */
    get_split_info(&p_viewevent_click->view_point);

    host_set_pointer_shape(split_info.pointer_shape);

    return(STATUS_OK);
}

_Check_return_
static STATUS
back_window_event_click_left_double(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEWEVENT_CLICK p_viewevent_click)
{
    /* Double click toggles split, if enabling enables to half way into area */
    const P_VIEW p_view = p_viewevent_click->view_point.p_view;

    get_split_info(&p_viewevent_click->view_point);

    if((split_info.flags.horz) || (split_info.flags.vert))
    {
        GDI_RECT gdi_rect;
        PIXIT_RECT pixit_rect;

        gdi_rect.tl.x = p_view->pane_posn.start.x;
        gdi_rect.tl.y = p_view->pane_posn.start.y;
        gdi_rect.br.x = p_view->pane_posn.max.x;
        gdi_rect.br.y = p_view->pane_posn.max.y;

        pixit_rect_from_window_rect(&pixit_rect, &gdi_rect, &p_view->host_xform[XFORM_BACK]);

        p_view->flags.pseudo_horz_split_on &= ~split_info.flags.horz;
        p_view->flags.pseudo_vert_split_on &= ~split_info.flags.vert;

        p_view->flags.horz_split_on ^= split_info.flags.horz;
        p_view->flags.vert_split_on ^= split_info.flags.vert;

        if(split_info.flags.horz && p_view->flags.horz_split_on)
            p_view->horz_split_pos = (pixit_rect.br.x - pixit_rect.tl.x) / 2;
        if(split_info.flags.vert && p_view->flags.vert_split_on)
            p_view->vert_split_pos = (pixit_rect.br.y - pixit_rect.tl.y) / 2;

        host_view_reopen(p_docu, p_view);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
back_window_event_click_left_drag(
    _InRef_     PC_VIEWEVENT_CLICK p_viewevent_click)
{
    /* Drag occuring on the back window, look and see if over split points */
    const P_VIEW p_view = p_viewevent_click->view_point.p_view;

    get_split_info(&p_viewevent_click->view_point);

    if((split_info.flags.horz) || (split_info.flags.vert))
    {
        p_view->flags.pseudo_horz_split_on &= ~split_info.flags.horz;
        p_view->flags.pseudo_vert_split_on &= ~split_info.flags.vert;

        p_view->flags.horz_split_on |= split_info.flags.horz;
        p_view->flags.vert_split_on |= split_info.flags.vert;

        host_drag_start(NULL);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
back_window_event_click_drag_started(
    _InRef_     PC_VIEWEVENT_CLICK p_viewevent_click)
{
    /* Drag is starting, so initialise the dragging state */
    const P_VIEW p_view = p_viewevent_click->view_point.p_view;
    GDI_RECT gdi_rect;

    zero_struct(split_info.dragging);

    gdi_rect.tl.x = p_view->pane_posn.start.x;
    gdi_rect.tl.y = p_view->pane_posn.start.y;
    gdi_rect.br.x = p_view->pane_posn.max.x;
    gdi_rect.br.y = p_view->pane_posn.max.y;

    pixit_rect_from_window_rect(&split_info.dragging.clip_pixit_rect, &gdi_rect, &p_view->host_xform[XFORM_BACK]);

    gdi_rect.tl.x = ho_win_state.metrics.split.x;
    gdi_rect.tl.y = ho_win_state.metrics.split.y;
    gdi_rect.br.x = (p_view->pane_posn.max.x - ho_win_state.metrics.split.x) - p_view->pane_posn.start.x;
    gdi_rect.br.y = (p_view->pane_posn.max.y - ho_win_state.metrics.split.y) - p_view->pane_posn.start.y;

    pixit_rect_from_window_rect(&split_info.dragging.valid_pixit_rect, &gdi_rect, &p_view->host_xform[XFORM_BACK]);

    get_split_drag_info(p_viewevent_click, &split_info.dragging.previous_pixit_point);

    render_split_drag_info(p_view, &split_info.dragging.previous_pixit_point);

    host_set_pointer_shape(split_info.pointer_shape);

    return(STATUS_OK);
}

_Check_return_
static STATUS
back_window_event_click_drag_movement(
    _InRef_     PC_VIEWEVENT_CLICK p_viewevent_click)
{
    /* Pointer is moving within the window.  Render a suitable set of cross hairs */
    const P_VIEW p_view = p_viewevent_click->view_point.p_view;
    PIXIT_POINT pixit_point;

    get_split_drag_info(p_viewevent_click, &pixit_point);

    if(!split_info.flags.horz)
        pixit_point.x = p_view->horz_split_pos;
    if(!split_info.flags.vert)
        pixit_point.y = p_view->vert_split_pos;

    if((pixit_point.x != split_info.dragging.previous_pixit_point.x) || (pixit_point.y != split_info.dragging.previous_pixit_point.y))
    {
        render_split_drag_info(p_view, &pixit_point);

        split_info.dragging.previous_pixit_point = pixit_point;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
back_window_event_click_drag_finished(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEWEVENT_CLICK p_viewevent_click)
{
    /* Finish dragging, ensure that we should have the split points enabled */
    const P_VIEW p_view = p_viewevent_click->view_point.p_view;
    PIXIT_POINT pixit_point;

    get_split_drag_info(p_viewevent_click, &pixit_point);

    if(split_info.flags.horz)
    {
        if((pixit_point.x < split_info.dragging.valid_pixit_rect.tl.x) || (pixit_point.x >= split_info.dragging.valid_pixit_rect.br.x))
            p_view->flags.horz_split_on = FALSE;
        else
            p_view->horz_split_pos = pixit_point.x;
    }

    if(split_info.flags.vert)
    {
        if((pixit_point.y < split_info.dragging.valid_pixit_rect.tl.y) || (pixit_point.y >= split_info.dragging.valid_pixit_rect.br.y))
            p_view->flags.vert_split_on = FALSE;
        else
            p_view->vert_split_pos = pixit_point.y;
    }

    render_split_drag_info(p_view, NULL /* don't define new point */);

    host_view_reopen(p_docu, p_view);

    return(STATUS_OK);
}

_Check_return_
static STATUS
back_window_event_click_drag_aborted(
    _InRef_     PC_VIEWEVENT_CLICK p_viewevent_click)
{
    /* Drag has been aborted, erase the current cross hair stuff */
    const P_VIEW p_view = p_viewevent_click->view_point.p_view;

    render_split_drag_info(p_view, NULL /* don't define new point */);

    return(STATUS_OK);
}

PROC_EVENT_PROTO(static, proc_event_back_window)
{
    switch(t5_message)
    {
    case T5_EVENT_POINTER_ENTERS_WINDOW:
    case T5_EVENT_POINTER_MOVEMENT:
        return(back_window_event_pointer((PC_VIEWEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_LEFT_DOUBLE:
        return(back_window_event_click_left_double(p_docu, (PC_VIEWEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_LEFT_DRAG:
        return(back_window_event_click_left_drag((PC_VIEWEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_STARTED:
        return(back_window_event_click_drag_started((PC_VIEWEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_MOVEMENT:
        return(back_window_event_click_drag_movement((PC_VIEWEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_FINISHED:
        return(back_window_event_click_drag_finished(p_docu, (PC_VIEWEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_ABORTED:
        return(back_window_event_click_drag_aborted((PC_VIEWEVENT_CLICK) p_data));

    default:
        return(view_event_back_window(p_docu, t5_message, p_data));
    }
}

/* Get the information about the split point.  This is used for
 * detecting when the drag should start.  We configure flag
 * and other information about what is going on into the
 * supplied structure.
 */

static void
get_split_info(
    _InRef_     PC_VIEW_POINT p_view_point)
{
    const P_VIEW p_view = p_view_point->p_view;
    GDI_POINT cursor;
    POINT split_area;
    RECT grab;

    zero_struct(split_info);

    window_point_from_pixit_point(&cursor, &p_view_point->pixit_point, &p_view->host_xform[XFORM_BACK]);

    split_area.x = (p_view->flags.vert_split_on && (!p_view->flags.pseudo_horz_split_on)) ? p_view->inner_frame_gdi_rect.tl.x : p_view->pane_posn.max.x;
    split_area.y = (p_view->flags.horz_split_on && (!p_view->flags.pseudo_vert_split_on)) ? p_view->inner_frame_gdi_rect.tl.y : p_view->pane_posn.max.y;

    grab.right = grab.left = p_view->pane_posn.grab.x;
    grab.right += p_view->pane_posn.grab_size.x;
    grab.bottom = grab.top = p_view->pane_posn.grab.y;
    grab.bottom += p_view->pane_posn.grab_size.y;

    grab.left   = MAX(grab.left - ho_win_state.metrics.split.x, p_view->pane_posn.start.x);
    grab.top    = MAX(p_view->pane_posn.grab.y - ho_win_state.metrics.split.y, p_view->pane_posn.start.y);
    grab.right  = MIN(grab.right + ho_win_state.metrics.split.x, p_view->pane_posn.max.x);
    grab.bottom = MIN(grab.bottom + ho_win_state.metrics.split.y, p_view->pane_posn.max.y);

    trace_2(TRACE_WINDOWS_HOST, TEXT("get_split_info: grab          x/y == %d,%d"), grab.left, grab.top);
    trace_2(TRACE_WINDOWS_HOST, TEXT("get_split_info: grab size     x/y == %d,%d"), grab.right - grab.left, grab.bottom - grab.top);
    trace_2(TRACE_WINDOWS_HOST, TEXT("get_split_info: split metrics x/y == %d,%d"), ho_win_state.metrics.split.x, ho_win_state.metrics.split.y);
    trace_2(TRACE_WINDOWS_HOST, TEXT("get_split_info: co-ords       x/y == %d,%d"), (int) cursor.x, (int) cursor.y);

    if(((int) cursor.x >= grab.left) &&
       ((int) cursor.x  < grab.right) &&
       ((int) cursor.y >= split_area.y) &&
       ((int) cursor.y  < p_view->inner_frame_gdi_rect.br.y) )
    {
        split_info.flags.horz = TRUE;
    }

    if(((int) cursor.x >= grab.left) &&
       ((int) cursor.x  < grab.right) &&
       ((int) cursor.y >= p_view->pane_posn.max.y) &&
       ((int) cursor.y  < p_view->inner_frame_gdi_rect.br.y) )
    {
        split_info.flags.horz_box = TRUE;
    }

    if(((int) cursor.x >= split_area.x) &&
       ((int) cursor.x  < p_view->inner_frame_gdi_rect.br.x) &&
       ((int) cursor.y >= grab.top) &&
       ((int) cursor.y  < grab.bottom) )
    {
        split_info.flags.vert = TRUE;
    }

    if(((int) cursor.x >= p_view->pane_posn.max.x) &&
       ((int) cursor.x  < p_view->inner_frame_gdi_rect.br.x) &&
       ((int) cursor.y >= grab.top) &&
       ((int) cursor.y  < grab.bottom) )
    {
        split_info.flags.vert_box = TRUE;
    }

    split_info.pointer_shape = POINTER_DEFAULT;

    if(split_info.flags.horz || split_info.flags.horz_box)
    {
        if(split_info.flags.vert || split_info.flags.vert_box)
            split_info.pointer_shape = POINTER_SPLIT_ALL;
        else
            split_info.pointer_shape = POINTER_SPLIT_LEFTRIGHT;
    }
    else
        if(split_info.flags.vert || split_info.flags.vert_box)
            split_info.pointer_shape = POINTER_SPLIT_UPDOWN;
}

/* Given a click / drag movement event attempt to generate
 * a suitable offset into the back window world in pixits
 * to modify the split point.
 */

static void
get_split_drag_info(
    _InRef_     PC_VIEWEVENT_CLICK p_viewevent_click,
    _OutRef_    P_PIXIT_POINT p_pixit_point)
{
    *p_pixit_point = p_viewevent_click->view_point.pixit_point;

    p_pixit_point->x = MAX(p_pixit_point->x, split_info.dragging.clip_pixit_rect.tl.x);
    p_pixit_point->x = MIN(p_pixit_point->x, split_info.dragging.clip_pixit_rect.br.x);
    p_pixit_point->y = MAX(p_pixit_point->y, split_info.dragging.clip_pixit_rect.tl.y);
    p_pixit_point->y = MIN(p_pixit_point->y, split_info.dragging.clip_pixit_rect.br.y);

    p_pixit_point->x -= split_info.dragging.clip_pixit_rect.tl.x;
    p_pixit_point->y -= split_info.dragging.clip_pixit_rect.tl.y;
}

/* Render the split points as required.  This can be done directly
 * into the DC of the window.  Note that the cursor has been
 * captured and we can get away with any dirty tricks we like
 */

static void
render_split_drag_info(
    _ViewRef_   P_VIEW p_view,
    P_PIXIT_POINT p_pixit_point)
{
    const HWND hwnd = p_view->main[WIN_BACK].hwnd;
    const HDC hdc = GetDC(hwnd);
    GDI_POINT point = { 0, 0 };

    if(NULL != p_pixit_point)
    {
        PIXIT_POINT pixit_point = *p_pixit_point;

        pixit_point.x = MIN(pixit_point.x, split_info.dragging.valid_pixit_rect.br.x);
        pixit_point.y = MIN(pixit_point.y, split_info.dragging.valid_pixit_rect.br.y);

        pixit_point.x += split_info.dragging.clip_pixit_rect.tl.x;
        pixit_point.y += split_info.dragging.clip_pixit_rect.tl.y;

        window_point_from_pixit_point(&point, &pixit_point, &p_view->host_xform[XFORM_BACK]);
    }

    if(NULL != hdc)
    {
        if(split_info.flags.horz)
        {
            if(split_info.dragging.flags.old_horz_rect)
            {
                InvertRect(hdc, &split_info.dragging.old_horz_rect);
                split_info.dragging.flags.old_horz_rect = FALSE;
            }

            if(NULL != p_pixit_point)
            {
                SetRect(&split_info.dragging.old_horz_rect, (int) point.x, p_view->inner_frame_gdi_rect.tl.y, (int) (point.x + ho_win_state.metrics.split.x), p_view->pane_posn.max.y);
                InvertRect(hdc, &split_info.dragging.old_horz_rect);
                split_info.dragging.flags.old_horz_rect = TRUE;
            }
        }

        if(split_info.flags.vert)
        {
            if(split_info.dragging.flags.old_vert_rect)
            {
                InvertRect(hdc, &split_info.dragging.old_vert_rect);
                split_info.dragging.flags.old_vert_rect = FALSE;
            }

            if(NULL != p_pixit_point)
            {
                SetRect(&split_info.dragging.old_vert_rect, p_view->inner_frame_gdi_rect.tl.x, (int) point.y, p_view->pane_posn.max.x, (int) (point.y + ho_win_state.metrics.split.y));
                InvertRect(hdc, &split_info.dragging.old_vert_rect);
                split_info.dragging.flags.old_vert_rect = TRUE;
            }
        }

        void_WrapOsBoolChecking(1 == ReleaseDC(hwnd, hdc));
    }
}

_Check_return_
static BOOL
preprocess_key_event_ESCAPE(
    _DocuRef_   P_DOCU p_docu_for_key)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu_for_key);

    host_key_buffer_flush();

    /* If dragging then terminate the drag, otherwise we can pass the event down! */
    if(ho_win_state.drag.enabled)
    {
        const P_DOCU p_docu = p_docu_from_docno(ho_win_state.drag.docno);
        const P_VIEW p_view = p_view_from_viewno(p_docu, ho_win_state.drag.viewno);
        GDI_POINT point;

        GetMessagePosAsClient(ho_win_state.drag.hwnd, &point.x, &point.y);

        ReleaseCapture();

        trace_2(TRACE__NULL, TEXT("send_key_to_docu(docno=") DOCNO_TFMT TEXT(") - terminating drag monitor - *** null_events_stop(docno=") DOCNO_TFMT TEXT(")"), docno_from_p_docu(p_docu_for_key), ho_win_state.drag.docno);
        null_events_stop(ho_win_state.drag.docno, T5_MSG_WIN_HOST_NULL, null_event_ho_win, HO_WIN_DRAG_NULL_CLIENT_HANDLE);

        /* inform the current owner of the pending doom */
        trace_2(TRACE_WINDOWS_HOST, TEXT("send_key_to_docu(docno=") DOCNO_TFMT TEXT("): *** ABORTING DRAG in docno=") DOCNO_TFMT TEXT(" ***"), docno_from_p_docu(p_docu_for_key), ho_win_state.drag.docno);
        send_mouse_event(p_docu, p_view, T5_EVENT_CLICK_DRAG_ABORTED, ho_win_state.drag.hwnd, point.x, point.y, FALSE, FALSE, ho_win_state.drag.event_handler);
        ho_win_state.drag.enabled = FALSE;

        return(1 /*processed*/);
    }

    return(FALSE);
}

/* Send the key event to the view or cache it for a rainy day via ho_key.
 * We are given the docu that the key is to be posted to.
 */

_Check_return_
static STATUS
send_key_to_docu(
    _DocuRef_   P_DOCU p_docu,
    _In_        KMAP_CODE key_code)
{
    ARRAY_HANDLE h_commands;
    T5_MESSAGE t5_message;
    BOOL fn_key = FALSE;

    if((h_commands = command_array_handle_from_key_code(p_docu, key_code, &t5_message)) == 0)
    {
        if(t5_message == T5_CMD_ESCAPE)
            if(preprocess_key_event_ESCAPE(p_docu))
                return(1 /*processed*/);

        fn_key = (T5_EVENT_NONE != t5_message);

        if(!fn_key)
        {
            if((key_code < 0x20) || (key_code > 0xFF))
            {
                trace_1(TRACE_WINDOWS_HOST, TEXT("key kmap=") U32_XTFMT TEXT(" unprocessed"), (U32) key_code);
                return(0 /*unprocessed*/);
            }

            docu_modify(p_docu);
        }
    }
    else
        fn_key = TRUE;

    status_assert(host_key_cache_event(docno_from_p_docu(p_docu), key_code, fn_key, 1));

    return(1 /*processed*/);
}

/* This code handles the enabling/disabling of the triple click event handlers.
 * This code works via a timer.
 */

static void
enable_triple_click(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y)
{
    if(HOST_WND_NONE != ho_win_state.triple.hwnd)
        return;

    (void) SetTimer(hwnd, TRIPLE_CLICK_TIMER_ID, GetDoubleClickTime(), NULL);
    ho_win_state.triple.hwnd = hwnd;
    ho_win_state.triple.start.x = x;
    ho_win_state.triple.start.y = y;
    ho_win_state.triple.dist.x = GetSystemMetrics(SM_CXDOUBLECLK);
    ho_win_state.triple.dist.y = GetSystemMetrics(SM_CYDOUBLECLK);
}

static void
disable_triple_click(void)
{
    if(HOST_WND_NONE == ho_win_state.triple.hwnd)
        return;

    (void) KillTimer(ho_win_state.triple.hwnd, TRIPLE_CLICK_TIMER_ID);
    ho_win_state.triple.hwnd = NULL;
}

/* Convert the host specific message into something that can be posted
 * to the event handler for that given window pane etc.
 */

static void
send_click_to_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _HwndRef_   HWND hwnd,
    _InVal_     UINT uiMsg,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     EVENT_HANDLER event_handler,
    _InVal_     BOOL ctrl_pressed,
    _InVal_     BOOL shift_pressed)
{
    const BOOL emulate_right_button = FALSE; /* ctrl_pressed || shift_pressed; */ /* handled on case-by-case basis now */
    T5_MESSAGE t5_message;

    switch(uiMsg)
    {
    default:
        t5_message = T5_EVENT_NONE;
        break;

    case WM_LBUTTONDOWN:
        if(HOST_WND_NONE != ho_win_state.triple.hwnd)
        {
            /* Only generate triple click within same range as double */
            int dx = abs(ho_win_state.triple.start.x - x);
            int dy = abs(ho_win_state.triple.start.y - y);
            if((dx <= ho_win_state.triple.dist.x) && (dy <= ho_win_state.triple.dist.y))
            {
                t5_message = (emulate_right_button) ? T5_EVENT_CLICK_RIGHT_TRIPLE : T5_EVENT_CLICK_LEFT_TRIPLE;
                start_drag_monitor(p_docu, p_view, hwnd, x, y, event_handler, emulate_right_button);
                break;
            }
        }

        t5_message = (emulate_right_button) ? T5_EVENT_CLICK_RIGHT_SINGLE : T5_EVENT_CLICK_LEFT_SINGLE;
        start_drag_monitor(p_docu, p_view, hwnd, x, y, event_handler, emulate_right_button);
        break;

    case WM_RBUTTONDOWN:
        if(HOST_WND_NONE != ho_win_state.triple.hwnd)
        {
            /* Only generate triple click within same range as double */
            int dx = abs(ho_win_state.triple.start.x - x);
            int dy = abs(ho_win_state.triple.start.y - y);
            if((dx <= ho_win_state.triple.dist.x) && (dy <= ho_win_state.triple.dist.y))
            {
                t5_message = T5_EVENT_CLICK_RIGHT_TRIPLE;
                start_drag_monitor(p_docu, p_view, hwnd, x, y, event_handler, TRUE);
                break;
            }
        }

        t5_message = T5_EVENT_CLICK_RIGHT_SINGLE;
        start_drag_monitor(p_docu, p_view, hwnd, x, y, event_handler, TRUE);
        break;

    case WM_LBUTTONDBLCLK:
        t5_message = (emulate_right_button) ? T5_EVENT_CLICK_RIGHT_DOUBLE : T5_EVENT_CLICK_LEFT_DOUBLE;
        start_drag_monitor(p_docu, p_view, hwnd, x, y, event_handler, emulate_right_button);
        break;

    case WM_RBUTTONDBLCLK:
        t5_message = T5_EVENT_CLICK_RIGHT_DOUBLE;
        start_drag_monitor(p_docu, p_view, hwnd, x, y, event_handler, TRUE);
        break;
    }

    send_mouse_event(p_docu, p_view, t5_message, hwnd, x, y, ctrl_pressed, shift_pressed, event_handler);
}

/* Post the mouse event to the outside world.
 * We are given a view, the T5 message and other information
 * to be bundled down to the event handler.
 */

static void
send_mouse_event(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     T5_MESSAGE t5_message,
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     BOOL ctrl_pressed,
    _InVal_     BOOL shift_pressed,
    _InVal_     EVENT_HANDLER event_handler)
{
    const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
    GDI_POINT gdi_point;
    VIEWEVENT_CLICK viewevent_click;
    zero_struct(viewevent_click);

    if((T5_EVENT_NONE == t5_message) || (NULL == p_host_event_desc->p_proc_event))
        return;

    gdi_point.x = x;
    gdi_point.y = y;

    /* setup the click context information and mouse point */
    viewevent_click.click_context.hwnd = hwnd;
    viewevent_click.click_context.ctrl_pressed = ctrl_pressed;
    viewevent_click.click_context.shift_pressed = shift_pressed;
    viewevent_click.click_context.gdi_org.x = 0; /* window-relative */
    viewevent_click.click_context.gdi_org.y = 0;
    {
    const P_PANE p_pane = (p_host_event_desc->pane_id >= 0) ? &p_view->pane[p_host_event_desc->pane_id] : NULL;
    if(p_pane && p_host_event_desc->scroll_x) viewevent_click.click_context.gdi_org.x = p_pane->scroll.x;
    if(p_pane && p_host_event_desc->scroll_y) viewevent_click.click_context.gdi_org.y = p_pane->scroll.y;
    } /*block*/
    host_set_click_context(p_docu, p_view, &viewevent_click.click_context, &p_view->host_xform[p_host_event_desc->xform_index]);

    host_modify_click_point(&gdi_point);

    view_point_from_window_point_and_context(&viewevent_click.view_point, &gdi_point, &viewevent_click.click_context);

    /* compute the extra drag information, including the delta x/y */
    viewevent_click.data.drag.p_reason_data = ho_win_state.drag.p_reason_data;
    viewevent_click.data.drag.pixit_delta.x = viewevent_click.view_point.pixit_point.x - ho_win_state.drag.start_pixit_point.x;
    viewevent_click.data.drag.pixit_delta.y = viewevent_click.view_point.pixit_point.y - ho_win_state.drag.start_pixit_point.y;

    hard_assert(TRUE);

    /* pass the event down to the event handler */
    (*p_host_event_desc->p_proc_event) (p_docu, t5_message, &viewevent_click);

    hard_assert(FALSE);

    /* start a triple click timer if double just broadcast */
    if((T5_EVENT_CLICK_LEFT_DOUBLE == t5_message) || (T5_EVENT_CLICK_RIGHT_DOUBLE == t5_message))
        enable_triple_click(hwnd, x, y);
    else if(T5_EVENT_POINTER_MOVEMENT != t5_message)
        disable_triple_click();

    /* if its a drag starting then tell the client about it */
    if(((T5_EVENT_CLICK_LEFT_DRAG == t5_message) || (T5_EVENT_CLICK_RIGHT_DRAG == t5_message)) && (ho_win_state.drag.enabled))
    {
        ho_win_state.drag.start_pixit_point = viewevent_click.view_point.pixit_point;
        send_mouse_event(p_docu, p_view, T5_EVENT_CLICK_DRAG_STARTED, hwnd, x, y, ctrl_pressed, shift_pressed, event_handler);
    }
}

/* Enable a drag monitor.  This code is used to sense when the drag is about to start.
 * It installs a timer - drag starting timeout and also captures all mouse movement.
 */

static void
start_drag_monitor(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     EVENT_HANDLER event_handler,
    _InVal_     BOOL right_button)
{
    DOCU_ASSERT(p_docu);
    VIEW_ASSERT(p_view);
    assert(hwnd);

    ho_win_state.drag.right_button = right_button;
    ho_win_state.drag.docno = docno_from_p_docu(p_docu);
    ho_win_state.drag.viewno = viewno_from_p_view(p_view);
    ho_win_state.drag.hwnd = hwnd;
    ho_win_state.drag.start.x = x;
    ho_win_state.drag.start.y = y;
    ho_win_state.drag.dist.x = GetSystemMetrics(SM_CXDRAG);
    ho_win_state.drag.dist.y = GetSystemMetrics(SM_CYDRAG);
    ho_win_state.drag.event_handler = event_handler;

    if(SetTimer(ho_win_state.drag.hwnd, DRAG_TIMER_ID, GetProfileInt(TEXT("windows"), TEXT("DragDelay"), DD_DEFDRAGDELAY), NULL))
    {
        SetCapture(ho_win_state.drag.hwnd);
        ho_win_state.drag.pending = TRUE;
    }
}

/* Remove the drag monitor, this code kills the timer
 * restores the capture and performs
 * the tidy up if the drag has started, or the
 * timeout has elapsed.
 */

static void
stop_drag_monitor(void)
{
    trace_0(TRACE_WINDOWS_HOST, TEXT("stop_drag_monitor: killing the drag monitor / drag canceled"));

    if(ho_win_state.drag.pending)
    {
        ReleaseCapture();
        KillTimer(ho_win_state.drag.hwnd, DRAG_TIMER_ID);
        ho_win_state.drag.pending = FALSE;
    }
}

/* Enable a drag event.  This code is called as the result of a T5_EVENT_CLICK_?_DRAG,
 * the code infact assumes that you are calling it using this state, otherwise
 * it will fail - assumes on threaded information - thanks Richard!
 *
 * So we enable another capture - current one we know has been killed!
 */

extern void
host_drag_start(
    _In_opt_    P_ANY p_reason_data)
{
    static S32 default_drag_reason_data = CB_CODE_NOREASON;

    trace_0(TRACE_WINDOWS_HOST, TEXT("host_drag_start: **** CALLED TO START A DRAG! ****"));

    /* must be threaded, so that the state information is valid */
    assert(ho_win_state.drag.threaded == TRUE);

    if(NULL == p_reason_data)
        p_reason_data = &default_drag_reason_data;

    /* ensure we capture the mouse, ensures we get the mouse events */
    SetCapture(ho_win_state.drag.hwnd);

    trace_1(TRACE_OUT | TRACE_ANY, TEXT("host_drag_start() - starting drag monitor - *** null_events_start(docno=") DOCNO_TFMT TEXT(")"), ho_win_state.drag.docno);
    if(status_fail(status_wrap(null_events_start(ho_win_state.drag.docno, T5_MSG_WIN_HOST_NULL, null_event_ho_win, HO_WIN_DRAG_NULL_CLIENT_HANDLE))))
    {
        ReleaseCapture();
        return;
    }

    ho_win_state.drag.p_reason_data = p_reason_data;
    ho_win_state.drag.enabled = TRUE;
}

/* See if there is a drag active on any of the document windows.
 * The pointer is used to return the currently active p_reason_data
 */

_Check_return_
extern BOOL
host_drag_in_progress(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_P_ANY p_p_reason_data)
{
    if(ho_win_state.drag.enabled)
    {
        if(docno_from_p_docu(p_docu) == ho_win_state.drag.docno)
        {
            *p_p_reason_data = ho_win_state.drag.p_reason_data;
            return(TRUE);
        }
    }

    *p_p_reason_data = NULL;
    return(FALSE);
}

/* Post a movement message to the current drag owner.  This can be over-ridden
 * by passing an event handler and a p_docu, p_view.
 */

static void
maybe_scroll(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     EVENT_HANDLER event_handler)
{
    const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
    const PC_PANE p_pane = &p_view->pane[p_host_event_desc->pane_id];
    const HWND hwnd = p_host_event_desc->is_slave ? p_view->edge[p_host_event_desc->slave_id].hwnd : p_pane->hwnd;
    POINT point;
    RECT window_rect;
    POINT offset;

    if(!p_host_event_desc->scroll_x && !p_host_event_desc->scroll_y)
        return;

    point.x = x;
    point.y = y;

    ClientToScreen(hwnd, &point);
    GetWindowRect(hwnd, &window_rect);

    trace_2(TRACE_WINDOWS_HOST, TEXT("post_movement_message: screen point %d,%d"), point.x, point.y);
    trace_4(TRACE_WINDOWS_HOST, TEXT("post_movement_message: bounding rectangle tl == %d,%d; br == %d,%d"), window_rect.left, window_rect.top, window_rect.right, window_rect.bottom);

    offset.x = offset.y = 0;

    if(point.x < window_rect.left)
        offset.x = point.x - window_rect.left;
    if(point.x > window_rect.right)
        offset.x = point.x - window_rect.right;
    if(point.y < window_rect.top)
        offset.y = point.y - window_rect.top;
    if(point.y > window_rect.bottom)
        offset.y = point.y - window_rect.bottom;

    trace_2(TRACE_WINDOWS_HOST, TEXT("post_movement_message: offset after calculations %d,%d"), offset.x, offset.y);

    if(offset.x || offset.y)
    {
        GDI_POINT scroll = p_pane->scroll;

        trace_2(TRACE_WINDOWS_HOST, TEXT("post_movement_message: (before modify) scroll x == ") S32_TFMT TEXT(", y == ") S32_TFMT, scroll.x, scroll.y);

        if(p_host_event_desc->scroll_x)
            scroll.x += offset.x;
        if(p_host_event_desc->scroll_y)
            scroll.y += offset.y;

        trace_2(TRACE_WINDOWS_HOST, TEXT("post_movement_message: (after modify) scroll x == ") S32_TFMT TEXT(", y == ") S32_TFMT, scroll.x, scroll.y);

        scroll.x = MIN(scroll.x, p_pane->extent.x - p_pane->size.x); /* SKS notes that if extent is smaller than size then we tried to -ve scroll */
        scroll.x = MAX(scroll.x, 0);
        scroll.y = MIN(scroll.y, p_pane->extent.y - p_pane->size.y);
        scroll.y = MAX(scroll.y, 0);

        scroll_pane_absolute(p_docu, p_view, event_handler, &scroll, TRUE);
    }
}

/* convert the given point to a suitable pixit point, adjusting via the origin */

static void
view_point_from_window_point_and_context(
    _OutRef_    P_VIEW_POINT p_view_point,
    _InRef_     PC_GDI_POINT p_gdi_point,
    _InRef_     PC_CLICK_CONTEXT p_click_context)
{
    GDI_POINT gdi_point = *p_gdi_point;

    gdi_point.x += p_click_context->gdi_org.x;
    gdi_point.y += p_click_context->gdi_org.y;

    VIEW_ASSERT(p_click_context->p_view);
    p_view_point->p_view = p_click_context->p_view;
    pixit_point_from_window_point(&p_view_point->pixit_point, &gdi_point, &p_click_context->host_xform);
}

/* Given a rectangle onto the page and a docu and some other stuff
 * including an event handler convert the given rectangle to
 * suitable co-ordinates for invalidating the screen area.
 */

static void
rect_from_view_rect(
    _OutRef_    PRECT p_rect,
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEW_RECT p_view_rect,
    _InRef_opt_ PC_RECT_FLAGS p_rect_flags,
    _InVal_     EVENT_HANDLER event_handler)
{
    const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
    const PC_PANE p_pane = &p_view_rect->p_view->pane[p_host_event_desc->pane_id];
    GDI_RECT gdi_rect;
    PC_PIXIT_RECT p_pixit_rect = &p_view_rect->rect;
    PIXIT_RECT t_pixit_rect;

    trace_4(TRACE_WINDOWS_HOST, TEXT("rect_from_view_rect: (pixit rectangle) tl = ") S32_TFMT TEXT(",") S32_TFMT TEXT("; br = ") S32_TFMT TEXT(",") S32_TFMT,
                                p_pixit_rect->tl.x, p_pixit_rect->tl.y, p_pixit_rect->br.x, p_pixit_rect->br.y);

    if(NULL != p_rect_flags)
    {
        RECT_FLAGS rect_flags = *p_rect_flags;

        t_pixit_rect = * p_pixit_rect;

      /*t_pixit_rect.tl.x -= rect_flags.extend_left_currently_unused     * p_docu->page_def.grid_size;*/
        if(rect_flags.reduce_left_by_2)
            t_pixit_rect.tl.x += 2 * p_docu->page_def.grid_size;

      /*t_pixit_rect.tl.y -= rect_flags.extend_up_currently_unused       * p_docu->page_def.grid_size;*/
        if(rect_flags.reduce_up_by_2)
            t_pixit_rect.tl.y += 2 * p_docu->page_def.grid_size;

        if(rect_flags.extend_right_by_1)
            t_pixit_rect.br.x += p_docu->page_def.grid_size;
        if(rect_flags.reduce_right_by_1)
            t_pixit_rect.br.x -= p_docu->page_def.grid_size;

        if(rect_flags.extend_down_by_1)
            t_pixit_rect.br.y += p_docu->page_def.grid_size;
        if(rect_flags.reduce_down_by_1)
            t_pixit_rect.br.y -= p_docu->page_def.grid_size;

        p_pixit_rect = &t_pixit_rect;
    }

    trace_4(TRACE_WINDOWS_HOST, TEXT("rect_from_view_rect: (pixit - after adjust) tl = ") S32_TFMT TEXT(",") S32_TFMT TEXT("; br = ") S32_TFMT TEXT(",") S32_TFMT,
        p_pixit_rect->tl.x, p_pixit_rect->tl.y, p_pixit_rect->br.x, p_pixit_rect->br.y);

    status_consume(window_rect_from_pixit_rect(&gdi_rect, p_pixit_rect, &p_view_rect->p_view->host_xform[p_host_event_desc->xform_index]));

    trace_4(TRACE_WINDOWS_HOST, TEXT("rect_from_view-rect: (converted to window units) tl = ") S32_TFMT TEXT(",") S32_TFMT TEXT("; br = ") S32_TFMT TEXT(",") S32_TFMT,
            gdi_rect.tl.x, gdi_rect.tl.y, gdi_rect.br.x, gdi_rect.br.y);

    if(NULL != p_rect_flags)
    {
        RECT_FLAGS rect_flags = *p_rect_flags;

        gdi_rect.tl.x -= rect_flags.extend_left_ppixels  * DU_PER_PROGRAM_PIXEL_X; /* SKS 17may94 */
        gdi_rect.tl.y -= rect_flags.extend_up_ppixels    * DU_PER_PROGRAM_PIXEL_Y;
        gdi_rect.br.x += rect_flags.extend_right_ppixels * DU_PER_PROGRAM_PIXEL_X;
        gdi_rect.br.y += rect_flags.extend_down_ppixels  * DU_PER_PROGRAM_PIXEL_Y;
    }

    if(p_host_event_desc->scroll_x)
    {
        gdi_rect.tl.x -= p_pane->scroll.x;
        gdi_rect.br.x -= p_pane->scroll.x;
    }

    if(p_host_event_desc->scroll_y)
    {
        gdi_rect.tl.y -= p_pane->scroll.y;
        gdi_rect.br.y -= p_pane->scroll.y;
    }

    p_rect->left   = (int) MAX(T5_GDI_MIN_X, gdi_rect.tl.x);
    p_rect->top    = (int) MAX(T5_GDI_MIN_Y, gdi_rect.tl.y);
    p_rect->right  = (int) MIN(T5_GDI_MAX_X, gdi_rect.br.x);
    p_rect->bottom = (int) MIN(T5_GDI_MAX_Y, gdi_rect.br.y);

    trace_4(TRACE_WINDOWS_HOST, TEXT("rect_from_view-rect: (final result) tl = %d,%d; br = %d,%d"), p_rect->left, p_rect->top, p_rect->right, p_rect->bottom);
}

/* Update all the panes or the borders using the given
 * redraw tag = the code just blindly invalidates the
 * largest area it can.
 */

extern void
host_update_all(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     REDRAW_TAG redraw_tag)
{
    int index[2], i;
    HWND hwnd;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(redraw_tag)
    {
    default: default_unhandled();
#if CHECKING
    case UPDATE_PANE_MARGIN_HEADER:
    case UPDATE_PANE_MARGIN_FOOTER:
    case UPDATE_PANE_MARGIN_COL:
    case UPDATE_PANE_MARGIN_ROW:
    case UPDATE_PANE_CELLS_AREA:
    case UPDATE_PANE_PRINT_AREA:
    case UPDATE_PANE_PAPER:
#endif
        {
        PANE_ID pane_id = NUMPANE;

        do  {
            PANE_ID_DECR(pane_id);

            hwnd = p_view->pane[pane_id].hwnd;
            if(HOST_WND_NONE != hwnd)
                InvalidateRect(hwnd, NULL, FALSE);

        } while(PANE_ID_START != pane_id);

        return;
        }

    case UPDATE_BORDER_HORZ:
        index[0] = WIN_BORDER_HORZ_SPLIT;
        index[1] = WIN_BORDER_HORZ;
        break;

    case UPDATE_RULER_HORZ:
        index[0] = WIN_RULER_HORZ_SPLIT;
        index[1] = WIN_RULER_HORZ;
        break;

    case UPDATE_BORDER_VERT:
        index[0] = WIN_BORDER_VERT_SPLIT;
        index[1] = WIN_BORDER_VERT;
        break;

    case UPDATE_RULER_VERT:
        index[0] = WIN_RULER_VERT_SPLIT;
        index[1] = WIN_RULER_VERT;
        break;
    }

    for(i = 0; i <= 1; ++i)
    {
        hwnd = p_view->edge[index[i]].hwnd;
        if(HOST_WND_NONE != hwnd)
            InvalidateRect(hwnd, NULL, FALSE);
    }
}

/* Update all the view panes for the given docu in the given view.
 * This code fires off a group of update calls for these views and then returns.
 */

extern void
host_all_update_later(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEW_RECT p_view_rect)
{
    REDRAW_FLAGS redraw_flags;
    REDRAW_FLAGS_CLEAR(redraw_flags);

    host_update(p_docu, p_view_rect, NULL, redraw_flags, UPDATE_PANE_PAPER);
    host_update(p_docu, p_view_rect, NULL, redraw_flags, UPDATE_BORDER_HORZ);
    host_update(p_docu, p_view_rect, NULL, redraw_flags, UPDATE_BORDER_VERT);
    host_update(p_docu, p_view_rect, NULL, redraw_flags, UPDATE_RULER_HORZ);
    host_update(p_docu, p_view_rect, NULL, redraw_flags, UPDATE_RULER_VERT);
}

/* Post the update for the given area to the required part of the window (based
 * on the tag).  This code either posts an update later, or it will
 * invalidate an area of the clients window area using the handle.  If it
 * is an unknown tag then it assumes it is an edge area - handled differently.
 */

extern void
host_update(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEW_RECT p_view_rect,
    _InRef_opt_ PC_RECT_FLAGS p_rect_flags,
    _InVal_     REDRAW_FLAGS redraw_flags,
    _InVal_     REDRAW_TAG redraw_tag)
{
    const P_VIEW p_view = p_view_rect->p_view;
    int index[2], i;
    RECT rect;
    HWND hwnd;
    RECT update_rect;

    trace_5(TRACE_WINDOWS_HOST, TEXT("host_update: view for update ") PTR_XTFMT TEXT("; rect = tl = %d, %d; br = %d, %d"), p_view_rect->p_view, p_view_rect->rect.tl.x, p_view_rect->rect.tl.y, p_view_rect->rect.br.x, p_view_rect->rect.br.y);

    switch(redraw_tag)
    {
    default: default_unhandled();
#if CHECKING
    case UPDATE_PANE_MARGIN_HEADER:
    case UPDATE_PANE_MARGIN_FOOTER:
    case UPDATE_PANE_MARGIN_COL:
    case UPDATE_PANE_MARGIN_ROW:
    case UPDATE_PANE_CELLS_AREA:
    case UPDATE_PANE_PRINT_AREA:
    case UPDATE_PANE_PAPER:
#endif
        {
        PANE_ID pane_id = NUMPANE;

        do  {
            PANE_ID_DECR(pane_id);

            hwnd = p_view->pane[pane_id].hwnd;

            if(HOST_WND_NONE != hwnd)
            {
                rect_from_view_rect(&rect, p_docu, p_view_rect, p_rect_flags, pane_to_event_handler_table[pane_id].event_handler);

                trace_5(TRACE_WINDOWS_HOST, TEXT("host_update: (pane %d) rectangle tl %d,%d, br %d,%d"), pane_id, rect.left, rect.top, rect.right, rect.bottom);

                if(redraw_flags.update_now)
                {
                    // check for dangerous redraw situation where bombing out is a good idea
                    if(GetUpdateRect(hwnd, &update_rect, FALSE)
                       &&
                       (update_rect.left   < rect.left   ||
                        update_rect.right  > rect.right  ||
                        update_rect.top    < rect.top    ||
                        update_rect.bottom > rect.bottom))
                    {
                        trace_0(TRACE_WINDOWS_HOST, TEXT("host_update: *** INTENSE REDRAW SITUATION *** "));
                        InvalidateRect(hwnd, &rect, FALSE);
                        UpdateWindow(hwnd);
                        continue;

                    }
                    trace_1(TRACE_WINDOWS_HOST, TEXT("host_update: *** UPDATE NOW FOR PANE %d ***"), pane_id);
                    InvalidateRect(hwnd, &rect, FALSE);
                    update_pane_window(hwnd, redraw_flags);
                }
                else
                    InvalidateRect(hwnd, &rect, FALSE);
            }

        } while(PANE_ID_START != pane_id);

        return;
        }

    case UPDATE_BORDER_HORZ:
        index[0] = WIN_BORDER_HORZ_SPLIT;
        index[1] = WIN_BORDER_HORZ;
        break;

    case UPDATE_BORDER_VERT:
        index[0] = WIN_BORDER_VERT_SPLIT;
        index[1] = WIN_BORDER_VERT;
        break;

    case UPDATE_RULER_HORZ:
        index[0] = WIN_RULER_HORZ_SPLIT;
        index[1] = WIN_RULER_HORZ;
        break;

    case UPDATE_RULER_VERT:
        index[0] = WIN_RULER_VERT_SPLIT;
        index[1] = WIN_RULER_VERT;
        break;
    }

    /* Invalidate the rectangle and then force updates of those areas */
    for(i = 0; i <= 1; ++i)
    {
        hwnd = p_view->edge[index[i]].hwnd;

        if(HOST_WND_NONE != hwnd)
        {
            rect_from_view_rect(&rect, p_docu, p_view_rect, p_rect_flags, slave_to_event_handler_table[index[i]].event_handler);

            trace_4(TRACE_WINDOWS_HOST, TEXT("host_update (edge): rectangle tl %d,%d, br %d,%d"), rect.left, rect.top, rect.right, rect.bottom);

            if(redraw_flags.update_now)
            {
                // check for dangerous redraw situation where bombing out is a good idea
                if(GetUpdateRect(hwnd, &update_rect, FALSE)
                   &&
                   (update_rect.left   < rect.left  ||
                    update_rect.right  > rect.right ||
                    update_rect.top    < rect.top   ||
                    update_rect.bottom > rect.bottom))
                {
                    trace_0(TRACE_WINDOWS_HOST, TEXT("host_update: *** INTENSE REDRAW SITUATION ON INDEX 1 *** "));
                    InvalidateRect(hwnd, &rect, FALSE);
                    UpdateWindow(hwnd);
                }
                else
                {
                    trace_0(TRACE_WINDOWS_HOST, TEXT("edge_update: *** UPDATE NOW FOR INDEX 1 ***"));
                    InvalidateRect(hwnd, &rect, FALSE);
                    update_pane_window(hwnd, redraw_flags);
                }
            }
            else
                InvalidateRect(hwnd, &rect, FALSE);
        }
    }
}

/* Fast update continue is not really needed in Windows or a region
 * based GDI as you only ever receive a single rectangle / region that
 * describes the area that needs to be redrawn.
 * Come again says SKS?
 */

_Check_return_
extern BOOL
host_update_fast_continue(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_VIEWEVENT_REDRAW p_viewevent_redraw)
{
    const P_REDRAW_CONTEXT p_redraw_context = &p_viewevent_redraw->redraw_context;
    const P_VIEW p_view = p_redraw_context->p_view;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    trace_0(TRACE_WINDOWS_HOST, TEXT("host_update_fast_continue: **** EXITING FAST UPDATE ****"));

    host_paint_end(p_redraw_context);

    void_WrapOsBoolChecking(1 == ReleaseDC(p_view->pane[p_view->cur_pane].hwnd, p_redraw_context->windows.paintstruct.hdc));

    if(ho_win_state.caret.visible.dimensions.y)
        ShowCaret(ho_win_state.caret.visible.hwnd);

    ho_win_state.update.in_update_fast = FALSE;

    return(FALSE);
}

/* Fast update invalidates all the pane windows, then it will
 * attempt to redraw the currently active pane.  We are given
 * a redraw tag and a rectangle to be worked on.  Although the
 * redraw tag is ignored.
 */

_Check_return_
extern BOOL
host_update_fast_start(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_VIEWEVENT_REDRAW p_viewevent_redraw,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_VIEW_RECT p_view_rect,
    RECT_FLAGS rect_flags)
{
    const P_VIEW p_view = p_view_rect->p_view;

    UNREFERENCED_PARAMETER_InVal_(redraw_tag);

    zero_struct_ptr(p_viewevent_redraw);

    {
    PANE_ID pane_id = NUMPANE;

    do  {
        PANE_ID_DECR(pane_id);

        if(pane_id != p_view->cur_pane)
        {
            const P_PANE p_pane = &p_view->pane[pane_id];
            const HWND hwnd = p_pane->hwnd;
            if(HOST_WND_NONE != hwnd)
            {
                RECT rect;
                rect_from_view_rect(&rect, p_docu, p_view_rect, &rect_flags, pane_to_event_handler_table[pane_id].event_handler);
                InvalidateRect(hwnd, &rect, FALSE);
            }
        }

    } while(PANE_ID_START != pane_id);
    } /*block*/

    {
    const PANE_ID pane_id = p_view->cur_pane;
    if(IS_PANE_ID_VALID(pane_id))
    {
        const P_PANE p_pane = &p_view->pane[pane_id];
        const HWND hwnd = p_pane->hwnd;
        if(HOST_WND_NONE != hwnd)
        {
            EVENT_HANDLER event_handler = pane_to_event_handler_table[pane_id].event_handler;
            const PC_HOST_EVENT_DESC p_host_event_desc = &host_event_desc_table[event_handler];
            const P_REDRAW_CONTEXT p_redraw_context = &p_viewevent_redraw->redraw_context;
            RECT rect;

            rect_from_view_rect(&rect, p_docu, p_view_rect, &rect_flags, event_handler);

            // return to caller with viewevent_redraw set up
            trace_0(TRACE_WINDOWS_HOST, TEXT("host_update_fast_start: **** IN FAST UPDATE MODE ****"));
            ho_win_state.update.in_update_fast = TRUE;

            {
            static REDRAW_CONTEXT_CACHE redraw_context_cache; /* can't get caller to give us one sensibly */
            zero_struct(redraw_context_cache);
            p_redraw_context->p_redraw_context_cache = &redraw_context_cache;
            } /*block*/

            p_viewevent_redraw->flags.show_content = TRUE;
            p_viewevent_redraw->flags.show_selection = TRUE;

            p_viewevent_redraw->p_view = p_view;
            p_viewevent_redraw->p_pane = p_pane;
            p_viewevent_redraw->area.p_view = p_view;

            p_redraw_context->p_docu = p_docu;
            p_redraw_context->p_view = p_view;

            p_redraw_context->windows.paintstruct.hdc = GetWindowDC(hwnd);
            p_redraw_context->windows.paintstruct.fErase = FALSE;
            p_redraw_context->windows.paintstruct.rcPaint = rect;

            p_redraw_context->display_mode = p_view->display_mode;

            p_redraw_context->border_width.x = p_redraw_context->border_width.y = p_docu->page_def.grid_size;
            p_redraw_context->border_width_2.x = p_redraw_context->border_width_2.y = 2 * p_docu->page_def.grid_size;

            host_redraw_context_set_host_xform(p_redraw_context, &p_view->host_xform[p_host_event_desc->xform_index]);

            host_redraw_context_fillin(p_redraw_context);

            // Convert the scroll posn to a suitable origin
            p_redraw_context->gdi_org.x = (p_host_event_desc->scroll_x) ? p_pane->scroll.x : 0;
            p_redraw_context->gdi_org.y = (p_host_event_desc->scroll_y) ? p_pane->scroll.y : 0;

            host_paint_start(p_redraw_context);

            p_redraw_context->windows.host_machine_clip_rect = p_redraw_context->windows.paintstruct.rcPaint;

            { // Convert the redraw rectangle to a suitable view rectangle
            GDI_RECT gdi_rect;
            gdi_rect.tl.x = p_redraw_context->windows.paintstruct.rcPaint.left   + p_redraw_context->gdi_org.x;
            gdi_rect.tl.y = p_redraw_context->windows.paintstruct.rcPaint.top    + p_redraw_context->gdi_org.y;
            gdi_rect.br.x = p_redraw_context->windows.paintstruct.rcPaint.right  + p_redraw_context->gdi_org.x;
            gdi_rect.br.y = p_redraw_context->windows.paintstruct.rcPaint.bottom + p_redraw_context->gdi_org.y;
            pixit_rect_from_window_rect(&p_viewevent_redraw->area.rect, &gdi_rect, &p_redraw_context->host_xform);
            } /*block*/

            if(ho_win_state.caret.visible.dimensions.y)
                HideCaret(ho_win_state.caret.visible.hwnd);

            {
            HRGN h_clip_region = CreateRectRgn(p_redraw_context->windows.host_machine_clip_rect.left, p_redraw_context->windows.host_machine_clip_rect.top, p_redraw_context->windows.host_machine_clip_rect.right,  p_redraw_context->windows.host_machine_clip_rect.bottom);
            if(NULL != h_clip_region)
            {
                SelectClipRgn(p_redraw_context->windows.paintstruct.hdc, h_clip_region);
                DeleteRgn(h_clip_region);
            }
            } /*block*/

            return(TRUE);
        }
    }
    } /*block*/

    return(FALSE);
}

/* Minimise the back window and all of its children in one evil and deadly swipe of a window call */

extern void
host_view_minimize(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    ShowWindow(p_view->main[WIN_BACK].hwnd, SW_MINIMIZE);
}

/* Maximise the back window so that it covers the entire screen - this again is with an evil swipe of a window call */

extern void
host_view_maximize(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    ShowWindow(p_view->main[WIN_BACK].hwnd, SW_SHOWMAXIMIZED);
}

/* Make the caret visible or hide it.  It is hidden if the size is defined to be zero.
 * This code updates the structures about the caret and then attempts to refresh that caret state.
 */

extern void
host_main_show_caret(
    _DocuRef_   P_DOCU p_docu,
    P_VIEW_POINT p_view_point,
    _InVal_     PIXIT pixit_height)
{
    const P_VIEW p_view = p_view_point->p_view;
    const P_PANE p_pane = &p_view->pane[p_view->cur_pane];
    PIXIT_RECT pixit_rect;

    // get the bounding information for the caret
    pixit_rect.br = pixit_rect.tl = p_view_point->pixit_point;
    pixit_rect.br.y += pixit_height;
    status_consume(window_rect_from_pixit_rect(&ho_win_state.caret.gdi_rect, &pixit_rect, &p_view->host_xform[XFORM_PANE]));
    ho_win_state.caret.gdi_rect.br.x += GetSystemMetrics(SM_CXBORDER);

    if(HOST_WND_NONE != p_pane->hwnd)
    {
        (void) host_show_caret(p_pane->hwnd,
                        (ho_win_state.caret.gdi_rect.br.x - ho_win_state.caret.gdi_rect.tl.x),
                        (ho_win_state.caret.gdi_rect.br.y - ho_win_state.caret.gdi_rect.tl.y),
                        (ho_win_state.caret.gdi_rect.tl.x - p_pane->scroll.x),
                        (ho_win_state.caret.gdi_rect.tl.y - p_pane->scroll.y));

        if(!p_docu->flags.is_current)
        {
            DOCNO docno = DOCNO_NONE;

            while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
                p_docu_from_docno_valid(docno)->flags.is_current = 0;
        }

        p_docu->flags.is_current = 1;
    }
}

/* Ensure that the caret is visible within the current view.  Scrolling it to ensure that it is all sensible */

extern void
host_main_scroll_caret(
    _DocuRef_   P_DOCU p_docu,
    P_VIEW_POINT p_view_point,
    _InVal_     PIXIT left,
    _InVal_     PIXIT right,
    _InVal_     PIXIT top,
    _InVal_     PIXIT bottom)
{
    const P_VIEW p_view = p_view_point->p_view;
    const P_PANE p_pane = &p_view->pane[p_view->cur_pane];

    if(HOST_WND_NONE != p_pane->hwnd)
    {
        GDI_POINT scroll = p_pane->scroll;
        PIXIT_RECT pixit_rect;
        GDI_RECT gdi_rect;
        GDI_POINT gdi_point;

        /* left/right are relative to p_view_point position -
         * a rectangle which the caller wants:
         * a) kept on screen if possible
         * b) if caret is nearer to RHS, keep RHS on screen if possible
         * c) if caret is nearer to LHS, keep LHS on screen if possible
         * d) caret must be on screen whatever
         */

        pixit_rect.br = pixit_rect.tl = p_view_point->pixit_point;
        pixit_rect.tl.x += left;
        pixit_rect.br.x += right;
        pixit_rect.tl.y += top;
        pixit_rect.br.y += bottom;

        status_consume(window_rect_from_pixit_rect(&gdi_rect, &pixit_rect, &p_view->host_xform[XFORM_PANE]));

        window_point_from_pixit_point(&gdi_point, &p_view_point->pixit_point, &p_view->host_xform[XFORM_PANE]);

        // diz: 19th May 1994, ensure that it is all visible
        gdi_rect.tl.x -= GetSystemMetrics(SM_CXBORDER);
        gdi_rect.br.x += GetSystemMetrics(SM_CXBORDER) * 2;
        gdi_rect.tl.y -= GetSystemMetrics(SM_CYBORDER);
        gdi_rect.br.y += GetSystemMetrics(SM_CYBORDER) * 2;

        /* MRJC 11.7.94 */
        if(gdi_rect.br.x - gdi_rect.tl.x > p_pane->size.x)
        {
            if(gdi_point.x < p_pane->scroll.x + 1 * p_pane->size.x / 16)
                scroll.x = gdi_point.x - 1 * p_pane->size.x / 8;
            else if(gdi_point.x > p_pane->scroll.x + 15 * p_pane->size.x / 16)
                scroll.x = gdi_point.x - 7 * p_pane->size.x / 8;
        }
        else if(gdi_rect.br.x > p_pane->scroll.x + p_pane->size.x)
            scroll.x = gdi_rect.br.x - p_pane->size.x;
        else if(gdi_rect.tl.x < p_pane->scroll.x)
            scroll.x = gdi_rect.tl.x;

        scroll.x = MAX(scroll.x, 0);

        /* MRJC 16.3.95 */
        if(gdi_rect.br.y - gdi_rect.tl.y > p_pane->size.y)
        {
            if(gdi_point.y < p_pane->scroll.y + 1 * p_pane->size.y / 16)
                scroll.y = gdi_point.y - 1 * p_pane->size.y / 8;
            else if(gdi_point.y > p_pane->scroll.y + 15 * p_pane->size.y / 16)
                scroll.y = gdi_point.y - 7 * p_pane->size.y / 8;
        }
        else if(gdi_rect.br.y > p_pane->scroll.y + p_pane->size.y)
            scroll.y = gdi_rect.br.y - p_pane->size.y;
        else if(gdi_rect.tl.y < p_pane->scroll.y)
            scroll.y = gdi_rect.tl.y;

        scroll.y = MAX(scroll.y, 0);

        /* If we must then perform a nice little scroll */
        trace_2(TRACE_WINDOWS_HOST, TEXT("host_main_scroll_caret: new scroll x=") S32_TFMT TEXT(", y=") S32_TFMT, scroll.x, scroll.y);
        trace_2(TRACE_WINDOWS_HOST, TEXT("host_main_scroll_caret: current scroll x=") S32_TFMT TEXT(", y=") S32_TFMT, p_pane->scroll.x, p_pane->scroll.y);

        if((scroll.x != p_pane->scroll.x) || (scroll.y != p_pane->scroll.y))
            scroll_pane_absolute(p_docu, p_view, pane_to_event_handler_table[p_view->cur_pane].event_handler, &scroll, TRUE);
    }
}

/* The host scroll pane function is responsible for page up/down
 * functions on the given view/pane combination.  The code is
 * given a view point that is updated with the adjusted
 * scroll point on exit.
 */

extern void
host_scroll_pane(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     T5_MESSAGE t5_message,
    P_VIEW_POINT p_view_point)
{
    const P_PANE p_pane = &p_view->pane[p_view->cur_pane];
    GDI_POINT scroll = p_pane->scroll;
    S32 offset;
    GDI_POINT gdi_point;
    PIXIT_POINT pixit_point;

    DOCU_ASSERT(p_docu);
    VIEW_ASSERT(p_view);
    assert((t5_message == T5_CMD_PAGE_DOWN) || (t5_message == T5_CMD_PAGE_UP) || (t5_message == T5_CMD_SHIFT_PAGE_DOWN) || (t5_message == T5_CMD_SHIFT_PAGE_UP));

    if(HOST_WND_NONE == p_pane->hwnd)
        return;

    offset = ((t5_message == T5_CMD_PAGE_DOWN) || (t5_message == T5_CMD_SHIFT_PAGE_DOWN)) ? p_pane->size.y : -p_pane->size.y;

    scroll.y += offset;
    scroll.y = MAX(scroll.y, 0);

    scroll_pane_absolute(p_docu, p_view, pane_to_event_handler_table[p_view->cur_pane].event_handler, &scroll, TRUE);

    gdi_point.x = 0;
    gdi_point.y = offset;
    pixit_point_from_window_point(&pixit_point, &gdi_point, &p_view->host_xform[XFORM_PANE]);
    p_view_point->pixit_point.x += pixit_point.x;
    p_view_point->pixit_point.y += pixit_point.y;
    p_view_point->pixit_point.x = MAX(0, p_view_point->pixit_point.x);
    p_view_point->pixit_point.y = MAX(0, p_view_point->pixit_point.y);
}

/* helpful hint to users - if this returns docno != DOCNO_NONE, you can use p_docu_from_docno_valid() */

_Check_return_
extern DOCNO
resolve_hwnd(
    _HwndRef_   HWND hwnd,
    _OutRef_    P_P_VIEW p_p_view,
    _Out_       EVENT_HANDLER * const p_event_handler)
{
    U32 u32 = (U32) (UINT_PTR) GetProp(hwnd, TYPE5_PROPERTY_WORD);

    if(u32)
    {
        const P_DOCU p_docu = viewid_unpack(u32, p_p_view, p_event_handler);
        return(docno_from_p_docu(p_docu));
    }

    *p_p_view = NULL;
    *p_event_handler = 0xFFFF;
    return(DOCNO_NONE);
}

/* Re-enter window enquires to where the pointer is believed to be,
 * this is obtained from the track module.  Using this information
 * we can post a pointer entering window event to the handler
 * for that window.
 *
 * Assuming we can resolve the window handle then we must compute a
 * suitable cursor position.  This will be relative to the top
 * left of the window's client area.
 */

extern void
host_reenter_window(void)
{
#if 0
    const HWND hwnd = CurrentTrackWindow(); /*???*/
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(!IsWindow(hwnd))
        return;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        int x, y;

        GetMessagePosAsClient(hwnd, &x, &y);

        send_mouse_event(p_docu_from_docno_valid(docno), p_view, T5_EVENT_POINTER_ENTERS_WINDOW, hwnd, x, y, event_handler);
    }
#endif
}

/* Make a supporting window for the given pane, the supporting window ID is specified acts as
 * an index into the handle list and is used to decide which type of window should be created.
 *
 * Slave windows handle the features such as scroll bars, and border regions.
 * They have no real state as they inherit it from the pane windows (extents) and
 * they are always attached to the other windows within the system.
 */

static const
struct INTERNAL_SLAVE_TABLE
{
    EVENT_HANDLER event_handler;
    PANE_ID pane_id;
    BOOL track_events;
    BOOL is_horz_scroll_widget;
    BOOL is_vert_scroll_widget;
}
internal_slave_table[/*slave_id*/ /*NUMEDGE*/] =
{   /*     event handler                pane associated         track_events hsw vsw */
    { EVENT_HANDLER_BORDER_HORZ_SPLIT,  WIN_PANE_SPLIT_VERT,    1 }, /* horizontal border for with vertical split line */
    { EVENT_HANDLER_BORDER_HORZ,        WIN_PANE,               1 }, /* horizontal border */
    { EVENT_HANDLER_BORDER_VERT_SPLIT,  WIN_PANE_SPLIT_VERT,    1 }, /* vertical border for with horizontal split line */
    { EVENT_HANDLER_BORDER_VERT,        WIN_PANE,               1 }, /* vertical border */

    { EVENT_HANDLER_RULER_HORZ_SPLIT,   WIN_PANE_SPLIT_HORZ,    1 }, /* horizontal ruler for with vertical split line */
    { EVENT_HANDLER_RULER_HORZ,         WIN_PANE,               1 }, /* horizontal ruler */
    { EVENT_HANDLER_RULER_VERT_SPLIT,   WIN_PANE_SPLIT_HORZ,    1 }, /* vertical ruler for with horizontal split line */
    { EVENT_HANDLER_RULER_VERT,         WIN_PANE,               1 }, /* vertical ruler */

    { EVENT_HANDLER_HSCROLL_SPLIT,      WIN_PANE_SPLIT_HORZ,    0, 1, 0 }, /* horizontal scroll bar */
    { EVENT_HANDLER_HSCROLL,            WIN_PANE,               0, 1, 0 }, /* horizontal scroll bar for split */
    { EVENT_HANDLER_VSCROLL_SPLIT,      WIN_PANE_SPLIT_VERT,    0, 0, 1 }, /* vertical scroll bar */
    { EVENT_HANDLER_VSCROLL,            WIN_PANE,               0, 0, 1 }  /* vertical scroll bar for split */
};

_Check_return_
static STATUS
slave_window_create(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        int slave_id)
{
    assert_EQ(NUMEDGE, elemof32(internal_slave_table));

    if( IS_ARRAY_INDEX_VALID(slave_id, elemof32(internal_slave_table)) &&
        (HOST_WND_NONE == p_view->edge[slave_id].hwnd) )
    {
        U32 packed_id = viewid_pack(p_docu, p_view, (U16) internal_slave_table[slave_id].event_handler);
        DWORD dwStyle = WS_CHILD;
        PCTSTR p_class_name;
        HWND hwnd;

        if(internal_slave_table[slave_id].is_horz_scroll_widget)
        {
            dwStyle |= SBS_HORZ;
            p_class_name = TEXT("SCROLLBAR");
        }
        else if(internal_slave_table[slave_id].is_vert_scroll_widget)
        {
            dwStyle |= SBS_VERT;
            p_class_name = TEXT("SCROLLBAR");
        }
        else
            p_class_name = window_class[APP_WINDOW_CLASS_BORDER];

        void_WrapOsBoolChecking(HOST_WND_NONE != (
        hwnd =
            CreateWindowEx(
                WS_EX_ACCEPTFILES, p_class_name, NULL,
                dwStyle,
                0, 0, 0, 0,
                p_view->main[WIN_BACK].hwnd, NULL, GetInstanceHandle(), &packed_id)));
        if(HOST_WND_NONE == hwnd)
            return(create_error(ERR_TOOMANYWINDOWS));

        if(internal_slave_table[slave_id].is_horz_scroll_widget || internal_slave_table[slave_id].is_vert_scroll_widget)
        {
            // I assume we don't get a WM_CREATE for these
            SetProp(hwnd, TYPE5_PROPERTY_WORD, (HANDLE) (UINT_PTR) packed_id);

            p_view->edge[slave_id].hwnd = hwnd;

            SetScrollRange(hwnd, SB_CTL, 0, SCROLL_RANGE, FALSE);
        }
    }

    return(STATUS_OK);
}

/* Destroy the specified slave window, ensuring that its handle is reset to zero.
 * Slave windows are only valid if their handle is non-zero.
 */

static void
slave_window_destroy(
    _ViewRef_   P_VIEW p_view,
    _In_        int slave_id)
{
    if(HOST_WND_NONE == p_view->edge[slave_id].hwnd)
        return;

    if(internal_slave_table[slave_id].is_horz_scroll_widget || internal_slave_table[slave_id].is_vert_scroll_widget)
    {
        // I assume we don't get a WM_DESTROY for these
        RemoveProp(p_view->edge[slave_id].hwnd, TYPE5_PROPERTY_WORD);
    }

    DestroyWindow(p_view->edge[slave_id].hwnd);
    p_view->edge[slave_id].hwnd = NULL;
}

/* end of windows/ho_win.c */
