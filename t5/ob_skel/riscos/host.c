/* riscos/host.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RISC OS specific window and event routines for Fireworkz */

/* RCM Jan 1992 */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/xp_skeld.h"

#include "ob_dlg/xp_dlgr.h"

#include "ob_toolb/xp_toolb.h"

/*
callback routines
*/

PROC_EVENT_PROTO(static, proc_event_back_window);

PROC_EVENT_PROTO(static, null_event_host_drag);

/*
internal structure
*/

typedef struct CTRL_HOST_VIEW
{
    P_PROC_EVENT p_proc_view_event;
    BOOL         do_x_scale;         /* TRUE if pane, horz border|ruler */
    BOOL         do_y_scale;         /* TRUE if pane, vert border|ruler */
    int          xform_idx;
    int          edge_mask;
}
CTRL_HOST_VIEW, * P_CTRL_HOST_VIEW;

typedef struct RCM_CALLBACK
{
    void *           event_handle;
    P_CTRL_HOST_VIEW p_ctrl_host_view;
    P_HOST_XFORM     p_host_xform;
    VIEWEVENT_CLICK  viewevent_click;
    PIXIT_POINT      first_mouse_position;
    PIXIT_POINT      last_mouse_position;
    MONOTIME         last_mouse_time;
}
RCM_CALLBACK, * P_RCM_CALLBACK;

typedef struct WBOUND
{
    BBox visible_area;
    int xscroll, yscroll;
}
WBOUND;

typedef struct WINDOW_POSITIONS
{
    WBOUND edge[NUMEDGE];
    WBOUND pane[NUMPANE];
}
WINDOW_POSITIONS, * P_WINDOW_POSITIONS, ** P_P_WINDOW_POSITIONS;

/*
internal routines
*/

static BOOL
back_window_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle);

static void
back_window_open(
    _ViewRef_   P_VIEW p_view,
    _In_        const WimpOpenWindowBlock * const p_open_window_block);

static void
calc_window_positions(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    /*out*/ P_WINDOW_POSITIONS p_w_p,
    _InoutRef_  P_BBox p_bbox);

static void
edge_window_break(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        int edge_id);

_Check_return_
static STATUS
edge_window_create(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        int edge_id);

_Check_return_
static BOOL
edge_window_open(
    P_ANY event_handle,
    _In_        const WimpOpenWindowBlock * const p_open_window_block);

static GDI_COORD
mouse_autoscroll_by(
    _In_        GDI_COORD diff);

static void
pane_window_break(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        PANE_ID pane_id);

_Check_return_
static STATUS
pane_window_create(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        PANE_ID pane_id);

static BOOL
pane_window_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle);

_Check_return_
static STATUS
pane_window_make_break(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

static void
pane_window_open(
    _ViewRef_   P_VIEW p_view,
    _In_        const WimpOpenWindowBlock * const p_open_window_block);

_Check_return_
static STATUS
ruler_window_make_break(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

static void
send_click_event_to_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    P_CTRL_HOST_VIEW p_ctrl_host_view,
    _In_        const WimpMouseClickEvent * const p_mouse_click_event);

_Check_return_
static STATUS
send_key_event_to_docu(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     S32 keycode);

static void
send_pointer_enter_event_to_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    P_CTRL_HOST_VIEW p_ctrl_host_view,
    wimp_w hwnd);

static void
send_pointer_leave_event_to_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    P_CTRL_HOST_VIEW p_ctrl_host_view);

static void
send_release_event_to_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     T5_MESSAGE t5_message);

static void
send_redraw_event_to_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    P_CTRL_HOST_VIEW p_ctrl_host_view,
    wimp_w hwnd);

static void
set_cur_pane(
    _ViewRef_   P_VIEW p_view,
    _InVal_     S32 window_type,
    _InVal_     S32 window_id,
    wimp_w window);

static void
view_point_from_screen_point_and_context(
    _OutRef_    P_VIEW_POINT p_view_point,
    _InRef_     PC_GDI_POINT p_gdi_point,
    _InRef_     PC_CLICK_CONTEXT p_click_context);

static void
window_rect_from_view_rect(
    _OutRef_    P_GDI_RECT p_gdi_rect,
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEW_RECT p_view_rect,
    _InRef_opt_ PC_RECT_FLAGS p_rect_flags,
    _InRef_     PC_HOST_XFORM p_host_xform);

static inline void
set_click_context_gdi_org_from_screen(
    _InoutRef_  P_CLICK_CONTEXT p_click_context,
    _InVal_     HOST_WND window_handle)
{
    host_gdi_org_from_screen(&p_click_context->gdi_org, window_handle); /* window w.a. ABS origin */
}

/*
internal static variables
*/

enum WINTYPE
{
    WINTYPE_BACK = 0,
    WINTYPE_PANE,
    WINTYPE_BORDER_HORZ,
    WINTYPE_BORDER_VERT,
    WINTYPE_RULER_HORZ,
    WINTYPE_RULER_VERT,
    WINTYPE_MAX
};

static CTRL_HOST_VIEW
window_ctrl[WINTYPE_MAX] =
{
    { proc_event_back_window, FALSE, FALSE, XFORM_BACK, 0 },
    { view_event_pane_window, TRUE,  TRUE,  XFORM_PANE, 0 }, /* scale x and y */
    { view_event_border_horz, TRUE,  FALSE, XFORM_HORZ, WIN_PANE_SPLIT_HORZ ^ WIN_PANE }, /* scale x but not y */
    { view_event_border_vert, FALSE, TRUE,  XFORM_VERT, WIN_PANE_SPLIT_VERT ^ WIN_PANE }, /* scale y but not x */
    { view_event_ruler_horz,  TRUE,  FALSE, XFORM_HORZ, WIN_PANE_SPLIT_HORZ ^ WIN_PANE }, /* scale x but not y */
    { view_event_ruler_vert,  FALSE, TRUE,  XFORM_VERT, WIN_PANE_SPLIT_VERT ^ WIN_PANE }  /* scale y but not x */
};

static HOST_XFORM
screen_host_xform;

_Check_return_
static STATUS
border_window_make_break(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    if(p_view->flags.horz_border_on)
    {
        /* Create one horizontal border window (aka column header) */
        status_return(edge_window_create(p_docu, p_view, WIN_BORDER_HORZ));
    }
    else
        edge_window_break(p_docu, p_view, WIN_BORDER_HORZ);

    if(p_view->flags.horz_border_on && p_view->flags.horz_split_on && (HOST_WND_NONE != p_view->pane[WIN_PANE_SPLIT_HORZ].hwnd))
    {
        /* Create second horizontal border (aka col header) */
        status_return(edge_window_create(p_docu, p_view, WIN_BORDER_HORZ_SPLIT));
    }
    else
        edge_window_break(p_docu, p_view, WIN_BORDER_HORZ_SPLIT);

    if(p_view->flags.vert_border_on)
    {
        /* Create one vertical border window (aka row border) */
        status_return(edge_window_create(p_docu, p_view, WIN_BORDER_VERT));
    }
    else
        edge_window_break(p_docu, p_view, WIN_BORDER_VERT);

    if(p_view->flags.vert_border_on && p_view->flags.vert_split_on && (HOST_WND_NONE != p_view->pane[WIN_PANE_SPLIT_VERT].hwnd))
    {
        /* Create second vertical border (aka row border) */
        status_return(edge_window_create(p_docu, p_view, WIN_BORDER_VERT_SPLIT));
    }
    else
        edge_window_break(p_docu, p_view, WIN_BORDER_VERT_SPLIT);

    return(STATUS_OK);
}

_Check_return_
static T5_MESSAGE
button_click_event_encode(
    _In_        const WimpMouseClickEvent * const p_mouse_click_event,
    _OutRef_    P_BOOL p_ctrl_pressed,
    _OutRef_    P_BOOL p_shift_pressed)
{
    static BOOL g_ctrl_pressed = FALSE; /* keep state consistent across all phases of click */
    static BOOL g_shift_pressed = FALSE;

    int buttonstate = p_mouse_click_event->buttons;
    T5_MESSAGE t5_message;

    switch(buttonstate)
    {
    case Wimp_MouseButtonSingleSelect: /* 0x400 Single 'select' */
    case Wimp_MouseButtonSingleAdjust: /* 0x100 Single 'adjust' */
        /* cache on first phase of left or right click */
        g_ctrl_pressed = host_ctrl_pressed();
        g_shift_pressed = host_shift_pressed();
        break;

    default:
         break;
    }

    *p_ctrl_pressed = g_ctrl_pressed;
    *p_shift_pressed = g_shift_pressed;

    switch(buttonstate)
    {
    case Wimp_MouseButtonSingleSelect: /* 0x400 Single 'select' */
        t5_message = T5_EVENT_CLICK_LEFT_SINGLE;
        break;

    case Wimp_MouseButtonDragSelect:
        t5_message = T5_EVENT_CLICK_LEFT_DRAG;
        break;

    case Wimp_MouseButtonSelect: /* 0x004 Double 'select' */
        t5_message = T5_EVENT_CLICK_LEFT_DOUBLE;
        break;

    case Wimp_MouseButtonTripleSelect:
        t5_message = T5_EVENT_CLICK_LEFT_TRIPLE;
        break;

    case Wimp_MouseButtonSingleAdjust: /* 0x100 Single 'adjust' */
        t5_message = T5_EVENT_CLICK_RIGHT_SINGLE;
        break;

    case Wimp_MouseButtonDragAdjust:
        t5_message = T5_EVENT_CLICK_RIGHT_DRAG;
        break;

    case Wimp_MouseButtonAdjust: /* 0x001 Double 'adjust' */
        t5_message = T5_EVENT_CLICK_RIGHT_DOUBLE;
        break;

    case Wimp_MouseButtonTripleAdjust:
        t5_message = T5_EVENT_CLICK_RIGHT_TRIPLE;
        break;

    default:
        t5_message = T5_EVENT_NONE;
        break;
    }

    return(t5_message);
}

_Check_return_
static PANE_ID
find_pane(
    _ViewRef_   P_VIEW p_view,
    wimp_w hwnd)
{
    PANE_ID pane_id = NUMPANE;

    do  {
        PANE_ID_DECR(pane_id);

        if(p_view->pane[pane_id].hwnd == hwnd)
            return(pane_id);

    } while(PANE_ID_START != pane_id);

    assert0();
    return(NUMPANE);
}

extern void
host_recache_mode_dependent_vars(void)
{
    const P_HOST_XFORM p_host_xform = &screen_host_xform;

    p_host_xform->scale.t.x = 1;
    p_host_xform->scale.t.y = 1;
    p_host_xform->scale.b.x = 1;
    p_host_xform->scale.b.y = 1;

    p_host_xform->do_x_scale = FALSE;
    p_host_xform->do_y_scale = FALSE;

    p_host_xform->riscos.dx = host_modevar_cache_current.dx;
    p_host_xform->riscos.dy = host_modevar_cache_current.dy;

    p_host_xform->riscos.XEigFactor = host_modevar_cache_current.XEigFactor;
    p_host_xform->riscos.YEigFactor = host_modevar_cache_current.YEigFactor;
}

/******************************************************************************
*
* send a leave and enter window to the window that the
* pointer is over, really just for click filter stuff
*
******************************************************************************/

extern void
host_reenter_window(void)
{
    WimpGetPointerInfoBlock pointer_info;
    WimpPollBlock event_data;
    _kernel_swi_regs rs;

    void_WrapOsErrorReporting(wimp_get_pointer_info(&pointer_info));

    if((int) pointer_info.window_handle < 0) /* iconbar or backdrop etc. */
        return;

    event_data.pointer_leaving_window.window_handle = pointer_info.window_handle;

    rs.r[0] = Wimp_EPointerLeavingWindow;
    rs.r[1] = (int) &event_data;
    rs.r[2] = (int)  event_data.pointer_leaving_window.window_handle;
    void_WrapOsErrorChecking(_kernel_swi(/*Wimp_SendMessage*/ 0x0400E7, &rs, &rs));

    rs.r[0] = Wimp_EPointerEnteringWindow;
    rs.r[1] = (int) &event_data;
    rs.r[2] = (int)  event_data.pointer_leaving_window.window_handle;
    void_WrapOsErrorChecking(_kernel_swi(/*Wimp_SendMessage*/ 0x0400E7, &rs, &rs));
}

/******************************************************************************
*
* RISC OS specific view initialisation code
* - called by view_new_create()
*
* Creates all the windows needed for a single-pane
* view and registers their low-level event handlers
*
******************************************************************************/

#define     WINDOW_TITLE_LEN 255
#define BUF_WINDOW_TITLE_LEN 256

_Check_return_
extern STATUS
host_view_init(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_PIXIT_POINT p_pixit_tl,
    _InRef_     PC_PIXIT_SIZE p_pixit_size)
{
    STATUS status;

#if defined(RISCOS_SLE_ICON)
    * (wimp_i *) &p_view->main[WIN_SLE].hwnd = BAD_WIMP_I;
#endif

    if(NULL == (p_view->window_title = al_ptr_alloc_elem(TCHARZ, BUF_WINDOW_TITLE_LEN, &status)))
        return(status);

    for(;;)
    {
        /* Create back and one pane window */
        WimpWindowWithBitset wind_defn;
        static int last_y_pos = 0;
        const GDI_SIZE screen_gdi_size = host_modevar_cache_current.gdi_size;
        GDI_SIZE gdi_size;
        BBox limit_bbox;
        U32 packed_viewid = viewid_pack(p_docu, p_view, 0);

        zero_struct(wind_defn);

        limit_bbox.xmin = host_modevar_cache_current.dx;
        limit_bbox.ymin = host_modevar_cache_current.dy;
        limit_bbox.xmax = screen_gdi_size.cx -                       host_modevar_cache_current.dx;
        limit_bbox.ymax = screen_gdi_size.cy - wimp_win_title_height(host_modevar_cache_current.dy);

        if(last_y_pos < 0)
            last_y_pos = ((screen_gdi_size.cy * 3) / 4 / host_modevar_cache_current.dy) * host_modevar_cache_current.dy;
        else if(!last_y_pos)
            last_y_pos = limit_bbox.ymax;
        else if(last_y_pos > limit_bbox.ymax)
            last_y_pos = limit_bbox.ymax - 48;

        gdi_size.cx = (GDI_COORD) (p_pixit_size->cx
            ? (p_pixit_size->cx / PIXITS_PER_RISCOS / host_modevar_cache_current.dx) * host_modevar_cache_current.dx
            : BBox_width(&limit_bbox) );
        gdi_size.cy = (GDI_COORD) (p_pixit_size->cy
            ? (p_pixit_size->cy / PIXITS_PER_RISCOS / host_modevar_cache_current.dy) * host_modevar_cache_current.dy
            : (((S32) screen_gdi_size.cy / 2)       / host_modevar_cache_current.dy) * host_modevar_cache_current.dy );

        wind_defn.visible_area.xmin = (int) (p_pixit_tl->x / PIXITS_PER_RISCOS / host_modevar_cache_current.dx) * host_modevar_cache_current.dx;
        wind_defn.visible_area.xmin = MAX(wind_defn.visible_area.xmin, limit_bbox.xmin);
        wind_defn.visible_area.xmax = wind_defn.visible_area.xmin + gdi_size.cx;
        wind_defn.visible_area.xmax = MIN(wind_defn.visible_area.xmax, limit_bbox.xmax);

        wind_defn.visible_area.ymax = p_pixit_tl->y
            ? (int) (p_pixit_tl->y / PIXITS_PER_RISCOS / host_modevar_cache_current.dy) * host_modevar_cache_current.dy
            : last_y_pos - 48;
        last_y_pos = wind_defn.visible_area.ymax;
        if( wind_defn.visible_area.ymax > limit_bbox.ymax)
        {
            /* too close to the top */
            wind_defn.visible_area.ymax = limit_bbox.ymax;
            wind_defn.visible_area.ymin = wind_defn.visible_area.ymax - gdi_size.cy;

            if( wind_defn.visible_area.ymin < limit_bbox.ymin)
                wind_defn.visible_area.ymin = limit_bbox.ymin;

            last_y_pos = 0;
        }
        else if((wind_defn.visible_area.ymin = wind_defn.visible_area.ymax - gdi_size.cy) < limit_bbox.ymin)
        {
            /* too close to the bottom */
            wind_defn.visible_area.ymin = limit_bbox.ymin;
            wind_defn.visible_area.ymax = wind_defn.visible_area.ymin + gdi_size.cy;

            if( wind_defn.visible_area.ymax > limit_bbox.ymax)
            {
                wind_defn.visible_area.ymax = limit_bbox.ymax;
                last_y_pos = 0;
            }
        }

        wind_defn.title_fg = '\x07';
        wind_defn.title_bg = '\x02';
        wind_defn.work_fg = '\x07';
        wind_defn.work_bg = '\x01'; /* SKS after 1.05 27oct93 - concede defeat and set this up - was'\xFF'*/
        wind_defn.highlight_bg = '\x0C';

        /* ExtraFlags byte on RISC OS 4 onwards */
        wind_defn.reserved = (char) (
            1 << 2 /* NEVER draw 3-D border*/);

        wind_defn.flags.bits.flags_are_new   = 1;
        wind_defn.flags.bits.moveable        = 1;
        wind_defn.flags.bits.has_back        = 1;
        wind_defn.flags.bits.has_close       = 1;
        wind_defn.flags.bits.has_title       = 1;
        wind_defn.flags.bits.has_toggle_size = 1;

        wind_defn.title_flags.bits.text            = 1;
        wind_defn.title_flags.bits.horz_centre     = 1;
        wind_defn.title_flags.bits.indirect        = 1;

        wind_defn.titledata.it.buffer = p_view->window_title;
        wind_defn.titledata.it.buffer_size = BUF_WINDOW_TITLE_LEN; /* includes CH_NULL terminator */

        wind_defn.work_flags.bits.button_type = ButtonType_Repeat; /*ButtonType_DoubleClickDrag;*/
        wind_defn.sprite_area = (void *) 1; /* Window Manager sprite area (needed to satisfy RISC PC) */

        p_view->margin.x0  = 0;
        p_view->margin.y0  = +wimp_win_hscroll_height(0);   /* the height of a horz_scroll bar (minus its bottom row, which we share) */
        p_view->margin.x1  = -wimp_win_vscroll_width(0);    /* the width of a vert_scroll bar (minus its right column, which we share) */
        p_view->margin.y1  = 0; /* being controlled by sk_cont these days */

        wind_defn.min_width  = (short) (64 + (                  - p_view->margin.x1));
        wind_defn.min_height = (short) (64 + (p_view->margin.y0 - p_view->margin.y1));

        /* We're not too fussy about the exact work area extents given to the back or pane windows as */
        /* they are set in back_window_open() after it has created/destroyed any borders, rulers or extra panes. */

        p_view->scaled_backextent = * (PC_GDI_BOX) &wind_defn.extent;
        p_view->scaled_paneextent = * (PC_GDI_BOX) &wind_defn.extent;
        /* so this is good enough! */

        /* Create the back window */
        status_break(status = winx_create_window(&wind_defn, &p_view->main[WIN_BACK].hwnd, back_window_event_handler, (P_ANY) (uintptr_t) packed_viewid));

        /* hack a menu into the back window */
        if(!event_register_window_menumaker(p_view->main[WIN_BACK].hwnd, ho_menu_event_maker, ho_menu_event_proc, (P_ANY) (uintptr_t) packed_viewid))
        {
            status = status_nomem();
            break;
        }

        /* And one pane (WIN_PANE) */
        status_break(status = pane_window_create(p_docu, p_view, WIN_PANE));

        status = STATUS_OK;

        break; /* all windows successfully created */
        /*NOTREACHED*/
    }

    if(status_fail(status))
        host_view_destroy(p_docu, p_view);      /* failed, so destroy as much as we created */

    return(status);
}

extern void
host_view_destroy(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(NULL == p_view)
        return;

    {
    PANE_ID pane_id;
    for(pane_id = PANE_ID_START; pane_id < NUMPANE; PANE_ID_INCR(pane_id))
        pane_window_break(p_docu, p_view, pane_id);
    } /*block*/

    {
    EDGE_ID edge_id;
    for(edge_id = EDGE_ID_START; edge_id < NUMEDGE; EDGE_ID_INCR(edge_id))
        edge_window_break(p_docu, p_view, edge_id);
    } /*block*/

    if(HOST_WND_NONE != p_view->main[WIN_BACK].hwnd)
    {
        U32 packed_viewid = viewid_pack(p_docu, p_view, 0);

        (void) event_register_window_menumaker(p_view->main[WIN_BACK].hwnd, NULL, NULL, (P_ANY) (uintptr_t) packed_viewid);

        void_WrapOsErrorReporting(winx_dispose_window(&p_view->main[WIN_BACK].hwnd));
    }

#if CHECKING
    {
    int i;
    for(i = 0; i < NUMMAIN; ++i)
        assert((NULL == p_view->main[i].hwnd) || (BAD_WIMP_I == * (wimp_i *) &p_view->main[i].hwnd));
    } /*block*/
#endif

    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_view->window_title));
}

extern void
host_view_query_posn(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _OutRef_    P_PIXIT_POINT p_tl,
    _OutRef_    P_PIXIT_SIZE p_size)
{
    WimpGetWindowStateBlock window_state;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    /* Get the state of the back window */
    window_state.window_handle = p_view->main[WIN_BACK].hwnd;
    if(!WrapOsErrorReporting(wimp_get_window_state(&window_state)))
    {
        p_tl->x = ((PIXIT) window_state.visible_area.xmin - host_modevar_cache_current.dx) * PIXITS_PER_RISCOS;
        p_tl->y = ((PIXIT) window_state.visible_area.ymax                                ) * PIXITS_PER_RISCOS;

        p_size->cx = (PIXIT) BBox_width(&window_state.visible_area)  * PIXITS_PER_RISCOS;
        p_size->cy = (PIXIT) BBox_height(&window_state.visible_area) * PIXITS_PER_RISCOS;
    }
    else
        p_tl->x = p_tl->y = p_size->cx = p_size->cy = 0;
}

extern void
host_view_show(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    union wimp_window_state_open_window_block_u window_u;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    /* Get the state of the window */
    window_u.window_state_block.window_handle = p_view->main[WIN_BACK].hwnd;
    if(!WrapOsErrorReporting(wimp_get_window_state(&window_u.window_state_block)))
    {
        window_u.open_window_block.behind = (wimp_w) -1; /* open at the top of the window stack */

        back_window_open(p_view, &window_u.open_window_block);
    }
}

extern void
host_view_reopen(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    union wimp_window_state_open_window_block_u window_u;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    /* Get the state of the window */
    window_u.window_state_block.window_handle = p_view->main[WIN_BACK].hwnd;
    if(!WrapOsErrorReporting(wimp_get_window_state(&window_u.window_state_block)))
    {
        /* Ask back window to open behind itself, as the window it is currently
         * behind may be one of the 'split' windows that we want to kill
        */
        window_u.open_window_block.behind = p_view->main[WIN_BACK].hwnd;

        back_window_open(p_view, &window_u.open_window_block);
    }
}

static int bodge_max = 0;

extern void
host_view_maximize(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    union wimp_window_state_open_window_block_u window_u;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(p_view->flags.maximized)
        return;

    window_u.window_state_block.window_handle = p_view->main[WIN_BACK].hwnd;
    void_WrapOsErrorReporting(wimp_get_window_state(&window_u.window_state_block));

    window_u.open_window_block.visible_area.xmin = 0;
    window_u.open_window_block.visible_area.ymin = 0;
    window_u.open_window_block.visible_area.xmax = host_modevar_cache_current.gdi_size.cx;
    window_u.open_window_block.visible_area.ymax = host_modevar_cache_current.gdi_size.cy;

    bodge_max = 1;

    back_window_open(p_view, &window_u.open_window_block);

    bodge_max = 0;
}

extern void
host_view_minimize(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    WimpMessage msg;
    WimpIconizeMessage * const p_wimp_message_iconize = (WimpIconizeMessage *) &msg.data;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(p_view->flags.minimized)
        return;

    /* ask a RISC OS iconizer to have a stab */
    zero_struct(msg);
    msg.hdr.size = sizeof32(msg.hdr) + sizeof32(WimpIconizeMessage);
  /*msg.hdr.your_ref = 0;*/ /* start of message sequence */
    msg.hdr.action_code = Wimp_MIconize;

    p_wimp_message_iconize->window_handle = p_view->main[WIN_BACK].hwnd;
    p_wimp_message_iconize->window_owner = host_task_handle();
    (void) strcpy(p_wimp_message_iconize->title, "foobar");

    void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessage /*don't care for ACK*/, &msg, 0 /*broadcast*/, BAD_WIMP_I, NULL));
}

extern void
reply_to_help_request(
    _DocuRef_   P_DOCU p_docu,
    PC_ANY p_any_wimp_message /*HelpRequest*/)
{
    const WimpMessage * const p_wimp_message = (const WimpMessage *) p_any_wimp_message;
    WimpMessage msg = *p_wimp_message;

    msg.hdr.size = sizeof32(msg.hdr);
    msg.hdr.your_ref = p_wimp_message->hdr.my_ref;
    msg.hdr.action_code = Wimp_MHelpReply;

    fill_in_help_request(p_docu, msg.data.bytes, sizeof32(msg.data.bytes));

    msg.hdr.size += strlen32p1(msg.data.bytes); /*CH_NULL*/
    msg.hdr.size  = (msg.hdr.size + (4-1)) & ~(4-1);

    void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessage, &msg, p_wimp_message->hdr.sender, BAD_WIMP_I, NULL));
}

_Check_return_
static STATUS
generic_window_process_DataLoad(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    P_CTRL_HOST_VIEW p_ctrl_host_view,
    _InRef_     PC_WimpMessage p_wimp_message)
{
    const WimpDataLoadMessage * const p_wimp_message_data_load = &p_wimp_message->data.data_load;
    T5_FILETYPE t5_filetype = (T5_FILETYPE) p_wimp_message_data_load->file_type;
    TCHARZ filename[256];
    STATUS status = STATUS_OK;

    switch(t5_filetype)
    {
    case FILETYPE_DIRECTORY:
    case FILETYPE_APPLICATION:
        return(create_error(FILE_ERR_ISADIR));

    default:
        break;
    }

    xstrkpy(filename, elemof32(filename), p_wimp_message_data_load->leaf_name); /* low-lifetime name */

    host_xfer_load_file_setup(p_wimp_message);

    switch(t5_filetype)
    {
    case FILETYPE_T5_FIREWORKZ:
    case FILETYPE_T5_WORDZ:
    case FILETYPE_T5_RESULTZ:
    case FILETYPE_T5_RECORDZ:
    case FILETYPE_T5_TEMPLATE:
    case FILETYPE_T5_COMMAND:
        break;

    case FILETYPE_TEXT:
        { /* SKS 10dec94 allow fred/fwk files of type Text (e.g. unmapped on NFS) to be detected - but does not scan these for recognisable headers */
        T5_FILETYPE t_t5_filetype = t5_filetype_from_extension(file_extension(filename));

        if(FILETYPE_UNDETERMINED != t_t5_filetype)
            t5_filetype = t_t5_filetype;

        break;
        }

    case FILETYPE_DOS:
    case FILETYPE_DATA:
    case FILETYPE_UNTYPED:
        {
        T5_FILETYPE t_t5_filetype = t5_filetype_from_extension(file_extension(filename)); /* thing/ext? */

        if(FILETYPE_UNDETERMINED == t_t5_filetype)
            t_t5_filetype = t5_filetype_from_file_header(filename); /* no, so scan for recognisable headers */

        if(FILETYPE_UNDETERMINED != t_t5_filetype)
            t5_filetype = t_t5_filetype;

        break;
        }

    default:
        break;
    }

    /*if(t5_filetype != FILETYPE_UNDETERMINED)*/
    {
        VIEWEVENT_CLICK viewevent_click;

        zero_struct(viewevent_click);

        /* NB DataLoad event x,y are absolute screen coordinates (i.e. not window relative) */
        viewevent_click.click_context.hwnd = p_wimp_message_data_load->destination_window;
        viewevent_click.click_context.ctrl_pressed = host_ctrl_pressed();
        viewevent_click.click_context.shift_pressed = host_shift_pressed();
        set_click_context_gdi_org_from_screen(&viewevent_click.click_context, p_wimp_message_data_load->destination_window);
        host_set_click_context(p_docu, p_view, &viewevent_click.click_context, &p_view->host_xform[p_ctrl_host_view->xform_idx]);

        view_point_from_screen_point_and_context(&viewevent_click.view_point, (PC_GDI_POINT) &p_wimp_message_data_load->destination_x, &viewevent_click.click_context);

        viewevent_click.data.fileinsert.filename = (P_U8) filename;
        viewevent_click.data.fileinsert.t5_filetype = t5_filetype;
        viewevent_click.data.fileinsert.safesource = host_xfer_loaded_file_is_safe();

        status = (* p_ctrl_host_view->p_proc_view_event) (p_docu, T5_EVENT_FILEINSERT_DOINSERT, &viewevent_click);
    }

    host_xfer_load_file_done();

    return(status);
}

static BOOL
generic_window_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle,
    _InVal_     S32 window_type,
    _InVal_     S32 window_id)
{
    P_CTRL_HOST_VIEW p_ctrl_host_view = &window_ctrl[window_type];
    P_VIEW p_view;
    P_DOCU p_docu = viewid_unpack((U32) handle, &p_view, NULL);
    const DOCNO docno = docno_from_p_docu(p_docu);
    STATUS status = STATUS_OK;

    trace_3(TRACE_RISCOS_HOST, TEXT("%s: %s handle=") PTR_XTFMT, __Tfunc__, report_wimp_event(event_code, p_event_data), handle);

    switch(event_code)
    {
    case Wimp_EOpenWindow:
        return(edge_window_open(handle, &p_event_data->open_window_request));

    case Wimp_ERedrawWindow:
        send_redraw_event_to_view(p_docu, p_view, p_ctrl_host_view, p_event_data->redraw_window_request.window_handle);
        return(TRUE); /* done something */

    case Wimp_EMouseClick:
        {
        status_line_auto_clear(p_docu);

        status_break(status = host_key_cache_emit_events());

        set_cur_pane(p_view, window_type, window_id, p_event_data->mouse_click.window_handle);

        p_docu->viewno_caret = viewno_from_p_view(p_view);

        send_click_event_to_view(p_docu, p_view, p_ctrl_host_view, &p_event_data->mouse_click);
        break;
        }

    case Wimp_EKeyPressed:
        {
        const KMAP_CODE kmap_code = kmap_convert(p_event_data->key_pressed.key_code);

        trace_2(TRACE_OUT | TRACE_RISCOS_HOST, TEXT("Key_Pressed key ") U32_XTFMT TEXT(" (kmap=") U32_XTFMT TEXT(")"), p_event_data->key_pressed.key_code, kmap_code);

        status_line_auto_clear(p_docu);

        status = send_key_event_to_docu(p_docu, kmap_code);

        if(status == 0)
            return(FALSE);

        break;
        }

    case Wimp_EPointerLeavingWindow:
        send_pointer_leave_event_to_view(p_docu, p_view, p_ctrl_host_view);
        break;

    case Wimp_EPointerEnteringWindow:
        send_pointer_enter_event_to_view(p_docu, p_view, p_ctrl_host_view, p_event_data->pointer_entering_window.window_handle);
        break;

    case Wimp_EUserDrag:
        send_release_event_to_view(p_docu, p_view, T5_EVENT_CLICK_DRAG_FINISHED);
        break;

    case Wimp_ELoseCaret:
        /* SKS 30jul93 dump out any keys still destined for this document */
        trace_1(TRACE_OUT | TRACE_RISCOS_HOST, TEXT("Wimp_ELoseCaret: host_key_cache_emit_events(docno ") S32_TFMT TEXT(")"), docno_from_p_docu(p_docu));
        status = host_key_cache_emit_events_for(docno_from_p_docu(p_docu));
        break;

    case Wimp_EGainCaret:
        return(TRUE);

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        {
        const PC_WimpMessage p_wimp_message = &p_event_data->user_message;

        switch(p_wimp_message->hdr.action_code)
        {
        case Wimp_MDataSave:
            { /* File dragged from another application, dropped on our window */
            const T5_FILETYPE t5_filetype = (T5_FILETYPE) p_wimp_message->data.data_save.file_type;

            switch(t5_filetype)
            {
            case FILETYPE_DIRECTORY:
            case FILETYPE_APPLICATION:
                reperr(FILE_ERR_ISADIR, p_wimp_message->data.data_save.leaf_name);
                break;

            default:
                host_xfer_import_file_via_scrap(p_wimp_message);
                break;
            }

            break;
            }

        case Wimp_MDataLoad:
            { /* File dragged from directory display, dropped on our window */
            status_line_auto_clear(p_docu);

            status_break(status = host_key_cache_emit_events());

            set_cur_pane(p_view, window_type, window_id, p_wimp_message->data.data_load.destination_window);

            p_docu->viewno_caret = viewno_from_p_view(p_view);

            status = generic_window_process_DataLoad(p_docu, p_view, p_ctrl_host_view, p_wimp_message);
            break;
            }

        case Wimp_MHelpRequest:
            {
            if((int) p_wimp_message->data.help_request.icon_handle >= -1) /* don't reply to system icons help requests */
                reply_to_help_request(p_docu, p_wimp_message);

            break;
            }
        }

        break;
        }

    default:
        return(FALSE);
    }

    if(status_fail(status))
        reperr_null(status);

    p_docu = p_docu_from_docno(docno);

    if(!IS_DOCU_NONE(p_docu))
    {
#if 0
        if(p_docu->flags.new_extent)
            docu_set_and_show_extent_all_views(p_docu);
#endif

        if(p_docu->flags.caret_position_after_command)
            status_assert(maeve_event(p_docu, T5_MSG_AFTER_SKEL_COMMAND, P_DATA_NONE));
    }

    /* done something, so... */
    return(TRUE);
}

static void
back_window_close(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    const BOOL adjust_clicked = winx_adjustclicked();
    const BOOL shift_pressed  = host_shift_pressed();
    const BOOL opening_a_directory = adjust_clicked;
    const BOOL closing_a_view = !(shift_pressed && adjust_clicked);

    process_close_request(p_docu, p_view, FALSE, closing_a_view, opening_a_directory);
}

static BOOL
back_window_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle)
{
    P_VIEW p_view;
    const P_DOCU p_docu = viewid_unpack((U32) handle, &p_view, NULL);

    trace_3(TRACE_RISCOS_HOST, TEXT("%s: %s handle=") PTR_XTFMT, __Tfunc__, report_wimp_event(event_code, p_event_data), handle);

    if(NULL == p_view)
        return(FALSE);

    switch(event_code)
    {
    case Wimp_EOpenWindow:
        back_window_open(p_view, &p_event_data->open_window_request); /* back, pane(s), ruler(s) etc. */
        break;

    case Wimp_ECloseWindow:
        back_window_close(p_docu, p_view);
        break;

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        {
        const PC_WimpMessage p_wimp_message = &p_event_data->user_message;

        switch(p_wimp_message->hdr.action_code)
        {
        case Wimp_MWindowInfo:
            { /* help out the iconizer - send him an acknowledgement of his request */
            WimpMessage msg = *p_wimp_message;
            WimpWindowInfoMessage * const p_wimp_message_window_info = (WimpWindowInfoMessage *) &msg.data;

            msg.hdr.size = sizeof32(msg.hdr) + sizeof32(WimpWindowInfoMessage);
            msg.hdr.your_ref = p_wimp_message->hdr.my_ref;
            msg.hdr.action_code = Wimp_MWindowInfo;

            p_wimp_message_window_info->reserved_0 = 0;
            (void) strcpy(p_wimp_message_window_info->sprite, /*"ic_"*/ "bdf"); /* bdf == FIREWORKZ */
            xstrkpy(p_wimp_message_window_info->title, elemof32(p_wimp_message_window_info->title), p_docu->docu_name.leaf_name);
            if(NULL != p_docu->docu_name.extension)
            {
                xstrkat(p_wimp_message_window_info->title, elemof32(p_wimp_message_window_info->title), FILE_EXT_SEP_TSTR);
                xstrkat(p_wimp_message_window_info->title, elemof32(p_wimp_message_window_info->title), p_docu->docu_name.extension);
            }

            void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessage, &msg, p_wimp_message->hdr.sender, BAD_WIMP_I, NULL));

            break;
            }

        default:
            break;
        }
        } /*block*/

        /*FALLTHRU*/

    default:
        return(generic_window_event_handler(event_code, p_event_data, handle, WINTYPE_BACK, 0));
    }

    /* done something, so... */
    return(TRUE);
}

/* rename as view_open_windows???*/

static void
back_window_open(
    _ViewRef_   P_VIEW p_view,
    _In_        const WimpOpenWindowBlock * const p_open_window_block)
{
    const P_DOCU p_docu = p_docu_from_docno(p_view->docno);
    union wimp_window_state_open_window_block_u back_window_u;
    wimp_w behind = p_open_window_block->behind;
    PIXIT_RECT visible_limits;
    WimpCaret current;
    BOOL had_input_focus = FALSE;
    const GDI_SIZE screen_gdi_size = host_modevar_cache_current.gdi_size;

    void_WrapOsErrorReporting(winx_get_caret_position(&current));

    if(HOST_WND_NONE != p_view->pane[p_view->cur_pane].hwnd)
        had_input_focus = (current.window_handle == p_view->pane[p_view->cur_pane].hwnd);

    p_view->flags.view_not_yet_open = 0;

    p_view->flags.h_v_b_r_change = 0;

    /* If scale factor changes, or p_view->flags.paper_off changes, window extents must be reset */
    host_set_extent_this_view(p_docu, p_view);

    frame_windows_set_margins(p_docu, p_view);

    /* check split limits */
    if(p_view->horz_split_pos <= 0)
        p_view->flags.horz_split_on = 0;

    if(p_view->vert_split_pos <= 0)
        p_view->flags.vert_split_on = 0;

    if(border_window_make_break(p_docu, p_view) < 0)
    {
        /* Error whilst creating border windows */
        p_view->flags.horz_border_on = p_view->flags.vert_border_on = FALSE;

        border_window_make_break(p_docu, p_view);
    }

    if(ruler_window_make_break(p_docu, p_view) < 0)
    {
        /* Error whilst creating ruler windows */
        p_view->flags.horz_ruler_on = p_view->flags.vert_ruler_on = FALSE;

        ruler_window_make_break(p_docu, p_view);
    }

    if(pane_window_make_break(p_docu, p_view) < 0)
    {
        /* Error whilst creating pane windows or their borders or rulers */
        p_view->flags.horz_split_on = p_view->flags.vert_split_on = 0;

        pane_window_make_break(p_docu, p_view);
    }

    host_set_extent_this_view(p_docu, p_view);  /* Set correct back window extent after pane_window_make_break's */

    /* p_open_window_block gives the screen position (p_open_window_block->box) and window stack position (p_open_window_block->behind)
     * that the Window Manager would like our back window opened at.
     * Since clicking 'fullsize' on the back window gives an p_open_window_block->box bigger than the screen,
     * we open the back window at its current stack position, but with the new screen position,
     * then read back the sanitised screen position.
     * We can then calculate the correct screen and stack positions for all
     * our borders/rulers/panes and a new stack position for the back window.
     */
    back_window_u.window_state_block.window_handle = p_view->main[WIN_BACK].hwnd;
    void_WrapOsErrorReporting(wimp_get_window_state(&back_window_u.window_state_block)); /* get back window's current 'behind' */

    back_window_u.open_window_block.window_handle = p_view->main[WIN_BACK].hwnd;
    back_window_u.open_window_block.visible_area = p_open_window_block->visible_area;
    back_window_u.open_window_block.xscroll = 0;
    back_window_u.open_window_block.yscroll = 0;

    /* SKS 30jun93 try to keep window on screen even when Window Manager goes mad */
    if(back_window_u.open_window_block.visible_area.xmin >= screen_gdi_size.cx)
        back_window_u.open_window_block.visible_area.xmin = screen_gdi_size.cx - 96; /* giving a view of the title bar past the close and back tools */

    if(back_window_u.open_window_block.visible_area.ymax >= screen_gdi_size.cy)
    {
        int y_size = BBox_height(&back_window_u.open_window_block.visible_area);
        back_window_u.open_window_block.visible_area.ymax = screen_gdi_size.cy - 32;
        back_window_u.open_window_block.visible_area.ymin = back_window_u.open_window_block.visible_area.ymax - y_size;
    }

    void_WrapOsErrorReporting(wimp_open_window(&back_window_u.open_window_block)); /* open back window at current 'behind', but new 'box' */

    back_window_u.window_state_block.window_handle = p_view->main[WIN_BACK].hwnd;
    void_WrapOsErrorReporting(wimp_get_window_state(&back_window_u.window_state_block)); /* read back sanitised box */

    if(p_open_window_block->behind == (wimp_w) -3 /* behind the Window Manager's backwindow => hidden */)
    {
        p_view->flags.maximized = 0;
        p_view->flags.minimized = 1;
    }
    else
    {
        p_view->flags.maximized = bodge_max || (WimpWindow_Toggled & back_window_u.window_state_block.flags);
        p_view->flags.minimized = 0;
    }

    if_constant(p_command_recorder_of_op_format)
    {
        ARGLIST_HANDLE arglist_handle = 0;
        STATUS status;

        if(p_view->flags.minimized || p_view->flags.maximized)
        {
            /* record min/maximisation */
            const OBJECT_ID object_id = OBJECT_ID_SKEL;
            PC_CONSTRUCT_TABLE p_construct_table;
            const T5_MESSAGE t5_message = p_view->flags.minimized ? T5_CMD_VIEW_MINIMIZE : T5_CMD_VIEW_MAXIMIZE;

            if(status_ok(status = command_set_current_view(p_docu, p_view))
            && status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
                         status = command_record(arglist_handle, object_id, p_construct_table);

            if(status_fail(status))
                reperr_null(status);
        }
        else
        {
            /* record change in posn, size, scroll */ /*EMPTY*/
        }

        arglist_dispose(&arglist_handle);
    }

    behind = p_open_window_block->behind;

#if defined(UNUSED) /* currently there are no such windows (e.g. list boxes) */
    /* open any windows maintained by win layer, modifying who to open this stack behind */
    winx_open_child_windows(p_view->main[WIN_BACK].hwnd, p_open_window, &behind);
#endif /* UNUSED */

    /* If wimp asked back window to open in place (i.e. behind itself)
     * then first window opened must be opened behind itself.
     * So find topmost window and set that as 'behind'
    */
    if(behind == p_view->main[WIN_BACK].hwnd)
    {
        PANE_ID pane_id;
        EDGE_ID edge_id;

        for(pane_id = PANE_ID_START; pane_id < NUMPANE; PANE_ID_INCR(pane_id))
            if(HOST_WND_NONE != p_view->pane[pane_id].hwnd)
                behind = p_view->pane[pane_id].hwnd;

        for(edge_id = EDGE_ID_START; edge_id < NUMEDGE; EDGE_ID_INCR(edge_id))
            if(HOST_WND_NONE != p_view->edge[edge_id].hwnd)
                behind = p_view->edge[edge_id].hwnd;
    }

    visible_limits.tl.x = p_view->paneextent.x;
    visible_limits.tl.y = p_view->paneextent.y;
    visible_limits.br.x = 0;
    visible_limits.br.y = 0;
    {
    WINDOW_POSITIONS position;

    calc_window_positions(p_docu, p_view, &position, &back_window_u.window_state_block.visible_area);

    { /* open the borders and rulers (if any) */
    EDGE_ID edge_id = NUMEDGE;

    do  {
        EDGE_ID_DECR(edge_id);

        if(HOST_WND_NONE != p_view->edge[edge_id].hwnd)
        {
            WimpOpenWindowBlock edge_open_window_block;

            edge_open_window_block.visible_area = position.edge[edge_id].visible_area;
            edge_open_window_block.xscroll = position.edge[edge_id].xscroll;
            edge_open_window_block.yscroll = position.edge[edge_id].yscroll;

            if( (edge_open_window_block.visible_area.xmin < edge_open_window_block.visible_area.xmax) &&
                (edge_open_window_block.visible_area.ymin < edge_open_window_block.visible_area.ymax) )
            {
                edge_open_window_block.behind = behind;
                edge_open_window_block.window_handle = behind = p_view->edge[edge_id].hwnd; /* everything else MUST open behind us */

                void_WrapOsErrorReporting(wimp_open_window(&edge_open_window_block));
            }
            else
            {   /* close window to Window Manager */
                WimpCloseWindowBlock edge_close_window_block;
                edge_close_window_block.window_handle = p_view->edge[edge_id].hwnd;
                void_WrapOsErrorReporting(wimp_close_window(&edge_close_window_block.window_handle));
            }
        }
    } while(edge_id != EDGE_ID_START);
    } /*block*/

    { /* now open the panes (at least one) */
    PANE_ID pane_id = NUMPANE;

    do  {
        P_PANE p_pane;

        PANE_ID_DECR(pane_id);

        p_pane = &p_view->pane[pane_id];

        if(HOST_WND_NONE != p_pane->hwnd)
        {
            WimpOpenWindowBlock pane_open_window_block;

            pane_open_window_block.visible_area = position.pane[pane_id].visible_area;
            pane_open_window_block.xscroll = position.pane[pane_id].xscroll;
            pane_open_window_block.yscroll = position.pane[pane_id].yscroll;

            pane_open_window_block.behind = behind;
            pane_open_window_block.window_handle = behind = p_pane->hwnd; /* everything else MUST open behind us */

            void_WrapOsErrorReporting(wimp_open_window(&pane_open_window_block));

            p_pane->lastopen.margin.x0 = (GDI_COORD) pane_open_window_block.visible_area.xmin - back_window_u.window_state_block.visible_area.xmin;
            p_pane->lastopen.margin.y0 = (GDI_COORD) pane_open_window_block.visible_area.ymin - back_window_u.window_state_block.visible_area.ymin;
            p_pane->lastopen.margin.x1 = (GDI_COORD) pane_open_window_block.visible_area.xmax - back_window_u.window_state_block.visible_area.xmax;
            p_pane->lastopen.margin.y1 = (GDI_COORD) pane_open_window_block.visible_area.ymax - back_window_u.window_state_block.visible_area.ymax;
            p_pane->lastopen.width     = BBox_width(&pane_open_window_block.visible_area);
            p_pane->lastopen.height    = BBox_height(&pane_open_window_block.visible_area);
            p_pane->lastopen.xscroll   = pane_open_window_block.xscroll;
            p_pane->lastopen.yscroll   = pane_open_window_block.yscroll;

            {
            GDI_RECT gdi_rect; /* cf work_area_origin_x_from_visible_area_and_scroll() */
            gdi_rect.tl.x = pane_open_window_block.xscroll; /* i.e. pane_open_window_block.visible_area.xmin - (pane_open_window_block.visible_area.xmin - pane_open_window_block.xscroll); */
            gdi_rect.tl.y = pane_open_window_block.yscroll; /* i.e. pane_open_window_block.visible_area.ymax - (pane_open_window_block.visible_area.ymax - pane_open_window_block.yscroll); */
            gdi_rect.br.x = pane_open_window_block.visible_area.xmax - ((GDI_COORD) pane_open_window_block.visible_area.xmin - pane_open_window_block.xscroll);
            gdi_rect.br.y = pane_open_window_block.visible_area.ymin - ((GDI_COORD) pane_open_window_block.visible_area.ymax - pane_open_window_block.yscroll);
            pixit_rect_from_window_rect(&p_pane->visible_pixit_rect, &gdi_rect, &p_view->host_xform[XFORM_PANE]);
            } /*block*/

            visible_limits.tl.x = MIN(visible_limits.tl.x, p_pane->visible_pixit_rect.tl.x);
            visible_limits.tl.y = MIN(visible_limits.tl.y, p_pane->visible_pixit_rect.tl.y);

            visible_limits.br.x = MAX(visible_limits.br.x, p_pane->visible_pixit_rect.br.x);
            visible_limits.br.y = MAX(visible_limits.br.y, p_pane->visible_pixit_rect.br.y);

            view_visible_skel_rect(p_docu, p_view, &p_pane->visible_pixit_rect, &p_pane->visible_skel_rect);
            skel_visible_row_range(p_docu, p_view, &p_pane->visible_skel_rect,  &p_pane->visible_row_range);
        }
    } while(pane_id != PANE_ID_START);
    }
    /*block*/

    /* last, but not least, open the back window behind the last pane */
    back_window_u.open_window_block.behind = behind;

    void_WrapOsErrorReporting(wimp_open_window(&back_window_u.open_window_block));

    { /* tell the toolbar what's going on */
    VIEWEVENT_VISIBLEAREA_CHANGED event_info;

    event_info.p_view              = p_view;
    event_info.visible_area.p_view = p_view;
    {
    GDI_RECT gdi_rect;
    gdi_rect.tl.x = 0;
    gdi_rect.tl.y = 0;
    gdi_rect.br.x = (GDI_COORD) BBox_width(&back_window_u.open_window_block.visible_area);
    gdi_rect.br.y = (GDI_COORD) BBox_height(&back_window_u.open_window_block.visible_area);
    pixit_rect_from_window_rect(&event_info.visible_area.rect, &gdi_rect, &p_view->host_xform[XFORM_BACK]);
    p_view->backwindow_pixit_rect = event_info.visible_area.rect;
    } /*block*/

    status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_EVENT_VISIBLEAREA_CHANGED, &event_info));
    } /*block*/

    } /*block*/

    {
    VIEWEVENT_VISIBLEAREA_CHANGED event_info;

    event_info.p_view              = p_view;
    event_info.visible_area.p_view = p_view;
    event_info.visible_area.rect   = visible_limits;

    view_event_pane_window(p_docu, T5_EVENT_VISIBLEAREA_CHANGED, &event_info);
    } /*block*/

    /* if we had the input focus, the earlier call to pane_window_make_break() may have closed the window with the focus/caret, */
    /* if so, claim the focus/caret back again in the same place for one of our other panes. */
    /* NB assumes pane_window_break() changed p_view->cur_pane to some other sensible window */

    if(had_input_focus)
    {
        if(current.window_handle != p_view->pane[p_view->cur_pane].hwnd)
        {
            current.window_handle = p_view->pane[p_view->cur_pane].hwnd;

            void_WrapOsErrorReporting(winx_set_caret_position(&current));
        }
    }
}

_Check_return_
static STATUS
pane_window_make_break(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    if(p_view->flags.horz_split_on)
    {
        status_return(pane_window_create(p_docu, p_view, WIN_PANE_SPLIT_HORZ));
    }
    else
        pane_window_break(p_docu, p_view, WIN_PANE_SPLIT_HORZ);

    if(p_view->flags.vert_split_on)
    {
        status_return(pane_window_create(p_docu, p_view, WIN_PANE_SPLIT_VERT));
    }
    else
        pane_window_break(p_docu, p_view, WIN_PANE_SPLIT_VERT);

    if(p_view->flags.horz_split_on && p_view->flags.vert_split_on)
    {
        status_return(pane_window_create(p_docu, p_view, WIN_PANE_SPLIT_DIAG));
    }
    else
        pane_window_break(p_docu, p_view, WIN_PANE_SPLIT_DIAG);

    status_return(border_window_make_break(p_docu, p_view));
    status_return(ruler_window_make_break(p_docu, p_view));

    return(STATUS_OK);
}

_Check_return_
static STATUS
ruler_window_make_break(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    if(p_view->flags.horz_ruler_on)
    {
        /* Create one horizontal ruler */
        status_return(edge_window_create(p_docu, p_view, WIN_RULER_HORZ));
    }
    else
        edge_window_break(p_docu, p_view, WIN_RULER_HORZ);

    if(p_view->flags.horz_ruler_on && p_view->flags.horz_split_on && (HOST_WND_NONE != p_view->pane[WIN_PANE_SPLIT_HORZ].hwnd))
    {
        /* Create second horizontal ruler (if not in existance) */
        status_return(edge_window_create(p_docu, p_view, WIN_RULER_HORZ_SPLIT));
    }
    else
        edge_window_break(p_docu, p_view, WIN_RULER_HORZ_SPLIT);

    if(p_view->flags.vert_ruler_on)
    {
        /* Create one vertical ruler */
        status_return(edge_window_create(p_docu, p_view, WIN_RULER_VERT));
    }
    else
        edge_window_break(p_docu, p_view, WIN_RULER_VERT);

    if(p_view->flags.vert_ruler_on && p_view->flags.vert_split_on && (HOST_WND_NONE != p_view->pane[WIN_PANE_SPLIT_VERT].hwnd))
    {
        /* Create second vertical ruler (aka row border) */
        status_return(edge_window_create(p_docu, p_view, WIN_RULER_VERT_SPLIT));
    }
    else
        edge_window_break(p_docu, p_view, WIN_RULER_VERT_SPLIT);

    return(STATUS_OK);
}

static void
pane_window_break(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        PANE_ID pane_id)
{
    if(pane_id == p_view->cur_pane)
    {
        p_view->cur_pane = WIN_PANE;            /* correct if breaking WIN_PANE_SPLIT_HORZ or WIN_PANE_SPLIT_VERT */
                                                /* but may be able to do better if breaking WIN_PANE_SPLIT_DIAG */
        if(pane_id == WIN_PANE_SPLIT_DIAG)
        {
            if(HOST_WND_NONE != p_view->pane[WIN_PANE_SPLIT_HORZ].hwnd)
                p_view->cur_pane = WIN_PANE_SPLIT_HORZ;
            else if(HOST_WND_NONE != p_view->pane[WIN_PANE_SPLIT_VERT].hwnd)
                p_view->cur_pane = WIN_PANE_SPLIT_VERT;
        }
    }

    if(HOST_WND_NONE != p_view->pane[pane_id].hwnd)
    {
        U32 packed_viewid = viewid_pack(p_docu, p_view, 0);

        (void) event_register_window_menumaker(p_view->pane[pane_id].hwnd, NULL, NULL, (P_ANY) (uintptr_t) packed_viewid);

        void_WrapOsErrorReporting(winx_dispose_window(&p_view->pane[pane_id].hwnd));
    }
}

static BOOL
edge_or_pane_window_needs_trespass(void)
{
    int Y;

    Y = (_kernel_osbyte(161, 0xC5, 0) >> 8) & 0xFF;

    if(Y & 0x60)
        return(TRUE); /* one or both of the WimpFlags is set to allow windows off screen */

    return(FALSE);
}

_Check_return_
static STATUS
pane_window_create(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        PANE_ID pane_id)
{
    STATUS status = STATUS_OK;

    if(HOST_WND_NONE == p_view->pane[pane_id].hwnd)
    {
        WimpWindowWithBitset wind_defn;
        U32 packed_viewid = viewid_pack(p_docu, p_view, 0);

        zero_struct(wind_defn);

        wind_defn.visible_area.xmin = 0;
        wind_defn.visible_area.ymin = 0;
        wind_defn.visible_area.xmax = +1024;
        wind_defn.visible_area.ymax = +1024;

        wind_defn.title_fg = '\x07';
        wind_defn.title_bg = '\x02';

        wind_defn.work_bg = '\xFF'; /* transparent work area */

        wind_defn.scroll_outer = '\x03';
        wind_defn.scroll_inner = '\x01';

        wind_defn.flags.bits.flags_are_new = 1;
        wind_defn.flags.bits.moveable = 1;
        wind_defn.flags.bits.pane = 1;
        wind_defn.flags.bits.trespass = UBF_PACK(edge_or_pane_window_needs_trespass());

        wind_defn.work_flags.bits.button_type = ButtonType_DoubleClickDrag;
        wind_defn.sprite_area = (void *) 1; /* Window Manager sprite area (needed to satisfy RISC PC) */

        switch(pane_id)
        {
        default: default_unhandled();
#if CHECKING
        case WIN_PANE:
#endif
            wind_defn.flags.bits.has_adjust_size = 1;
            wind_defn.flags.bits.has_vert_scroll = 1;
            wind_defn.flags.bits.has_horz_scroll = 1;
            break;

        case WIN_PANE_SPLIT_HORZ:
            wind_defn.flags.bits.has_adjust_size = 1;
            wind_defn.flags.bits.has_vert_scroll = 0;
            wind_defn.flags.bits.has_horz_scroll = 1;
            break;

        case WIN_PANE_SPLIT_VERT:
            wind_defn.flags.bits.has_adjust_size = 1;
            wind_defn.flags.bits.has_vert_scroll = 1;
            wind_defn.flags.bits.has_horz_scroll = 0;
            break;

        case WIN_PANE_SPLIT_DIAG:
            wind_defn.flags.bits.has_adjust_size = 0;
            wind_defn.flags.bits.has_vert_scroll = 0;
            wind_defn.flags.bits.has_horz_scroll = 0;
            break;
        }

        wind_defn.min_width  = 40;
        wind_defn.min_height = 40;

        wind_defn.extent = * (const BBox *) &p_view->scaled_paneextent;

        if(status_ok(status = winx_create_window(&wind_defn, &p_view->pane[pane_id].hwnd, pane_window_event_handler, (P_ANY) (uintptr_t) packed_viewid)))
            if(!event_register_window_menumaker(p_view->pane[pane_id].hwnd, ho_menu_event_maker, ho_menu_event_proc, (P_ANY) (uintptr_t) packed_viewid))
                status = status_nomem();
    }

    return(status);
}

static BOOL
pane_window_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    void * handle)
{
    P_VIEW p_view;
    const P_DOCU p_docu = viewid_unpack((U32) handle, &p_view, NULL);

    trace_3(TRACE_RISCOS_HOST, TEXT("%s: %s handle=") PTR_XTFMT, __Tfunc__, report_wimp_event(event_code, p_event_data), handle);

    /* NB cannot identify the pane here, as the window handle position varies with event! */

    switch(event_code)
    {
    case Wimp_EOpenWindow:
        pane_window_open(p_view, &p_event_data->open_window_request);
        return(TRUE); /* done something */

    case Wimp_ERedrawWindow:
        send_redraw_event_to_view(p_docu, p_view, &window_ctrl[WINTYPE_PANE], p_event_data->redraw_window_request.window_handle);

        {
        PANE_ID pane_id = find_pane(p_view, p_event_data->redraw_window_request.window_handle);
        view_visible_skel_rect(p_docu, p_view, &p_view->pane[pane_id].visible_pixit_rect, &p_view->pane[pane_id].visible_skel_rect);
        skel_visible_row_range(p_docu, p_view, &p_view->pane[pane_id].visible_skel_rect,  &p_view->pane[pane_id].visible_row_range);
        } /*block*/

        if(p_docu->flags.caret_position_later || p_docu->flags.caret_scroll_later) /* SKS 24mar95 try not to send too many of these around */
            status_assert(maeve_event(p_docu, T5_EVENT_REDRAW_AFTER, P_DATA_NONE));
        return(TRUE); /* done something */

    default:
        return(generic_window_event_handler(event_code, p_event_data, handle, WINTYPE_PANE, 0));
    }
}

static void
pane_window_open(
    _ViewRef_   P_VIEW p_view,
    _In_        const WimpOpenWindowBlock * const p_open_window_block)
{
    const PANE_ID pane_id = find_pane(p_view, p_open_window_block->window_handle);
    const P_PANE p_pane = &p_view->pane[pane_id];
    WimpOpenWindowBlock back_open_window_block;

    p_pane->lastopen.xscroll = p_open_window_block->xscroll;
    p_pane->lastopen.yscroll = p_open_window_block->yscroll;

    p_view->pane[pane_id ^ 2].lastopen.xscroll = p_open_window_block->xscroll;
    p_view->pane[pane_id ^ 1].lastopen.yscroll = p_open_window_block->yscroll;

    switch(pane_id)
    {
    default:
        break;

    case WIN_PANE_SPLIT_HORZ:
        {
        S32 change = ((S32) BBox_width(&p_open_window_block->visible_area)  + host_modevar_cache_current.dx) - p_view->horz_split_pos / PIXITS_PER_RISCOS;
        assert(p_view->flags.horz_split_on);
        p_view->horz_split_pos     += change * PIXITS_PER_RISCOS;
        p_pane->lastopen.margin.x1 += change;
        break;
        }

    case WIN_PANE_SPLIT_VERT:
        {
        S32 change = ((S32) BBox_height(&p_open_window_block->visible_area) + host_modevar_cache_current.dy) - p_view->vert_split_pos / PIXITS_PER_RISCOS;
        assert(p_view->flags.vert_split_on);
        p_view->vert_split_pos     += change * PIXITS_PER_RISCOS;
        p_pane->lastopen.margin.y0 -= change;
        break;
        }
    }

    back_open_window_block.window_handle = p_view->main[WIN_BACK].hwnd;
    back_open_window_block.visible_area.xmin = p_open_window_block->visible_area.xmin - p_pane->lastopen.margin.x0;
    back_open_window_block.visible_area.xmax = p_open_window_block->visible_area.xmax - p_pane->lastopen.margin.x1;
    back_open_window_block.visible_area.ymin = p_open_window_block->visible_area.ymin - p_pane->lastopen.margin.y0;
    back_open_window_block.visible_area.ymax = p_open_window_block->visible_area.ymax - p_pane->lastopen.margin.y1;
    back_open_window_block.xscroll = back_open_window_block.yscroll = 0;
    back_open_window_block.behind = p_open_window_block->behind;

    back_window_open(p_view, &back_open_window_block);
}

static BOOL
horzborder_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle)
{
    return(generic_window_event_handler(event_code, p_event_data, handle, WINTYPE_BORDER_HORZ, WIN_BORDER_HORZ_SPLIT));
}

static BOOL
horzruler_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle)
{
    return(generic_window_event_handler(event_code, p_event_data, handle, WINTYPE_RULER_HORZ, WIN_RULER_HORZ_SPLIT));
}

static BOOL
vertborder_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle)
{
    return(generic_window_event_handler(event_code, p_event_data, handle, WINTYPE_BORDER_VERT, WIN_BORDER_VERT_SPLIT));
}

static BOOL
vertruler_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle)
{
    return(generic_window_event_handler(event_code, p_event_data, handle, WINTYPE_RULER_VERT, WIN_RULER_VERT_SPLIT));
}

static void
edge_window_break(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        int edge_id)
{
    if(HOST_WND_NONE != p_view->edge[edge_id].hwnd)
    {
        U32 packed_viewid = viewid_pack(p_docu, p_view, 0);

        p_view->flags.h_v_b_r_change = 1;

        (void) event_register_window_menumaker(p_view->edge[edge_id].hwnd, NULL, NULL, (P_ANY) (uintptr_t) packed_viewid);

        void_WrapOsErrorReporting(winx_dispose_window(&p_view->edge[edge_id].hwnd));
    }
}

_Check_return_
static STATUS
edge_window_create(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        int edge_id)
{
    STATUS status = STATUS_OK;

    if(HOST_WND_NONE == p_view->edge[edge_id].hwnd)
    {
        winx_event_handler p_proc_host_event;
        WimpWindowWithBitset wind_defn;
        U32 packed_viewid = viewid_pack(p_docu, p_view, 0);

        zero_struct(wind_defn);

        wind_defn.behind = (wimp_w) -1; /* open at top */

        wind_defn.title_fg = '\xFF'; /* transparent border */
        wind_defn.work_bg  = '\xFF'; /* transparent work area */

        wind_defn.work_flags.bits.button_type = ButtonType_DoubleClickDrag;
        wind_defn.sprite_area = (void *) 1; /* Window Manager sprite area (needed to satisfy RISC PC) */

        wind_defn.min_width  = 4;
        wind_defn.min_height = 4;

        wind_defn.flags.bits.flags_are_new = 1;
        wind_defn.flags.bits.moveable = 1;
        wind_defn.flags.bits.pane = 1;
        wind_defn.flags.bits.trespass = UBF_PACK(edge_or_pane_window_needs_trespass());

        /*wind_defn.extent.xmin = 0; been zeroed */
        /*wind_defn.extent.ymax = 0; been zeroed */

        switch(edge_id)
        {
        default: default_unhandled();
#if CHECKING
        case WIN_BORDER_HORZ_SPLIT:
#endif
        case WIN_BORDER_HORZ:
            wind_defn.extent.ymin = -1024;
            wind_defn.extent.xmax = p_view->scaled_paneextent.x1;
            wind_defn.min_width = 40;
            p_proc_host_event = horzborder_event_handler;
            break;

        case WIN_BORDER_VERT_SPLIT:
        case WIN_BORDER_VERT:
            wind_defn.extent.ymin = p_view->scaled_paneextent.y0;
            wind_defn.extent.xmax = 1280;
            wind_defn.min_height = 40;
            p_proc_host_event = vertborder_event_handler;
            break;

        case WIN_RULER_HORZ_SPLIT:
        case WIN_RULER_HORZ:
            wind_defn.extent.ymin = -1024;
            wind_defn.extent.xmax = p_view->scaled_paneextent.x1;
            wind_defn.min_width = 40;
            p_proc_host_event = horzruler_event_handler;
            break;

        case WIN_RULER_VERT_SPLIT:
        case WIN_RULER_VERT:
            wind_defn.extent.ymin = p_view->scaled_paneextent.y0;
            wind_defn.extent.xmax = 1280;
            wind_defn.min_height = 40;
            p_proc_host_event = vertruler_event_handler;
            break;
        }

        p_view->flags.h_v_b_r_change = 1;

        if(status_ok(status = winx_create_window(&wind_defn, &p_view->edge[edge_id].hwnd, p_proc_host_event, (P_ANY) (uintptr_t) packed_viewid)))
            if(!event_register_window_menumaker(p_view->edge[edge_id].hwnd, ho_menu_event_maker, ho_menu_event_proc, (P_ANY) (uintptr_t) packed_viewid))
                status = status_nomem();
    }

    return(status);
}

_Check_return_
static BOOL
edge_window_open(
    P_ANY event_handle,
    _In_        const WimpOpenWindowBlock * const p_open_window_block)
{
    P_VIEW p_view;
    union wimp_window_state_open_window_block_u window_u;

    consume(P_DOCU, viewid_unpack((U32) event_handle, &p_view, NULL));

    window_u.window_state_block.window_handle = p_view->main[WIN_BACK].hwnd;
    if(NULL != WrapOsErrorReporting(wimp_get_window_state(&window_u.window_state_block)))
        return(FALSE);

    window_u.open_window_block.behind = p_open_window_block->behind;
    back_window_open(p_view, &window_u.open_window_block);
    return(TRUE);
}

static void
set_cur_pane(
    _ViewRef_   P_VIEW p_view,
    _InVal_     S32 window_type,
    _InVal_     S32 window_id,
    wimp_w hwnd)
{
    PANE_ID pane_id;

    switch(window_type)
    {
    case WINTYPE_BACK:
        return;

    case WINTYPE_PANE:
        pane_id = find_pane(p_view, hwnd);
        UNREFERENCED_PARAMETER_InVal_(window_id);
        break;

    default:
        {
        int mask = window_ctrl[window_type].edge_mask;
        assert_EQ((WIN_PANE_SPLIT_HORZ ^ WIN_PANE),(WIN_PANE_SPLIT_DIAG ^ WIN_PANE_SPLIT_VERT));
        pane_id = p_view->cur_pane;
        if(p_view->edge[window_id].hwnd == hwnd)
            pane_id = (PANE_ID) (pane_id & ~mask);
        else
            pane_id = (PANE_ID) (pane_id |  mask);

        break;
        }
    }

    if(p_view->cur_pane != pane_id)
    {
        WimpCaret current;

        void_WrapOsErrorReporting(winx_get_caret_position(&current));

        if(current.window_handle == p_view->pane[p_view->cur_pane].hwnd)
        {
            current.window_handle = p_view->pane[pane_id].hwnd;

            void_WrapOsErrorReporting(winx_set_caret_position(&current));
        }

        p_view->cur_pane = pane_id;
    }
}

/******************************************************************************
*
* Host to View interface routines:
*
*       send_redraw_event_to_view
*       send_release_event_to_view
*       send_pointer_enter_event_to_view
*       send_pointer_leave_event_to_view
*
* These routines map machine specific events
* and coordinates into T5 messages and
* machine independant pixit coordinates
*
******************************************************************************/

static BOOL threaded_through_redraw = FALSE;

static void
update_common_pre_loop(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _OutRef_    P_VIEWEVENT_REDRAW p_viewevent_redraw,
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/,
    wimp_w hwnd,
    _InVal_     REDRAW_FLAGS redraw_flags)
{
    const P_REDRAW_CONTEXT p_redraw_context = &p_viewevent_redraw->redraw_context;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    zero_struct_ptr(p_viewevent_redraw);

    p_viewevent_redraw->flags = redraw_flags;

    p_viewevent_redraw->p_view = p_view;

    p_redraw_context->riscos.hwnd = hwnd;

    /* force consistent pixel rounding in host_redraw_context_set_host_xform() */
    p_redraw_context->gdi_org.x = 0;
    p_redraw_context->gdi_org.y = 0;

    p_redraw_context->pixit_origin.x = 0;
    p_redraw_context->pixit_origin.y = 0;

    p_redraw_context->display_mode = p_view->display_mode;

    p_redraw_context->border_width.x = p_redraw_context->border_width.y = p_docu->page_def.grid_size;
    p_redraw_context->border_width_2.x = p_redraw_context->border_width_2.y = 2 * p_docu->page_def.grid_size;

    host_redraw_context_set_host_xform(p_redraw_context, p_host_xform);

    host_redraw_context_fillin(p_redraw_context);
}

static void
update_common_in_loop(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    P_VIEWEVENT_REDRAW p_viewevent_redraw,
    _In_        const WimpRedrawWindowBlock * const p_redraw_window_block)
{
    const P_REDRAW_CONTEXT p_redraw_context = &p_viewevent_redraw->redraw_context;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    /* invalidate cache foreground and background entries cos wimp may have drawn scroll bars/borders/icons etc. */
    host_invalidate_cache(HIC_REDRAW_LOOP_START);

    p_viewevent_redraw->area.p_view = p_view;

    p_redraw_context->p_docu = p_docu;
    p_redraw_context->p_view = p_view;

    p_redraw_context->gdi_org.x = work_area_origin_x_from_visible_area_and_scroll(p_redraw_window_block); /* window w.a. ABS origin */
    p_redraw_context->gdi_org.y = work_area_origin_y_from_visible_area_and_scroll(p_redraw_window_block);

    p_redraw_context->pixit_origin.x = 0;
    p_redraw_context->pixit_origin.y = 0;

    p_redraw_context->riscos.host_machine_clip_box = * (PC_GDI_BOX) &p_redraw_window_block->redraw_area;

    {
    GDI_RECT gdi_rect;
    gdi_rect.tl.x = p_redraw_window_block->redraw_area.xmin;
    gdi_rect.br.y = p_redraw_window_block->redraw_area.ymin;
    gdi_rect.br.x = p_redraw_window_block->redraw_area.xmax;
    gdi_rect.tl.y = p_redraw_window_block->redraw_area.ymax;
    pixit_rect_from_screen_rect_and_context(&p_viewevent_redraw->area.rect, &gdi_rect, p_redraw_context);
    } /*block*/
}

static void
send_redraw_event_to_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    P_CTRL_HOST_VIEW p_ctrl_host_view,
    wimp_w hwnd)
{
    const P_HOST_XFORM p_host_xform = &p_view->host_xform[p_ctrl_host_view->xform_idx];
    REDRAW_FLAGS redraw_flags;
    VIEWEVENT_REDRAW viewevent_redraw;
    WimpRedrawWindowBlock redraw_window_block;
    int wimp_more = 0;

    REDRAW_FLAGS_CLEAR(redraw_flags);
    redraw_flags.show_content = TRUE;
    redraw_flags.show_selection = TRUE;

    trace_0(TRACE_RISCOS_HOST, TEXT("host event: send_redraw_event_to_view"));
    update_common_pre_loop(p_docu, p_view, &viewevent_redraw, p_host_xform, hwnd, redraw_flags);

    redraw_window_block.window_handle = hwnd;

    if(NULL != WrapOsErrorReporting(wimp_redraw_window(&redraw_window_block, &wimp_more)))
        wimp_more = 0;

    while(0 != wimp_more)
    {
        update_common_in_loop(p_docu, p_view, &viewevent_redraw, &redraw_window_block);

        threaded_through_redraw = TRUE;
        (* p_ctrl_host_view->p_proc_view_event) (p_docu, T5_EVENT_REDRAW, &viewevent_redraw);
        threaded_through_redraw = FALSE;

        host_restore_clip_rectangle(&viewevent_redraw.redraw_context);

        if(NULL != WrapOsErrorReporting(wimp_get_rectangle(&redraw_window_block, &wimp_more)))
            wimp_more = 0;
    }
}

static void
callback(
    _InVal_     T5_MESSAGE t5_message,
    P_RCM_CALLBACK p_callback,
    _InVal_     S32 only_if_it_moves,
    _InVal_     S32 scroll_if_outside_window)
{
    VIEWEVENT_CLICK viewevent_click = p_callback->viewevent_click;
    P_VIEW p_view;
    const P_DOCU p_docu = viewid_unpack((U32) p_callback->event_handle, &p_view, NULL);
    WimpGetPointerInfoBlock pointer_info;
    WimpGetWindowStateBlock window_state;
    PIXIT_POINT point;
    MONOTIME now;
    STATUS do_call;

    /* All the viewevent_click fields except 'view_point' and pixit_delta were initialised and saved in p_callback->viewevent_click by */
    /* send_click_event_to_view() and host_drag_start(), so we only need to process the mouse position.        */

    /* Get pointer position, given in screen coordinates (i.e. not window relative) */
    void_WrapOsErrorReporting(wimp_get_pointer_info(&pointer_info));

    /* and window origin, (best to re-read as window may scroll or move under our feet) as per set_click_context_gdi_org_from_screen() */
    window_state.window_handle = viewevent_click.click_context.hwnd;
    void_WrapOsErrorReporting(wimp_get_window_state(&window_state));
    viewevent_click.click_context.gdi_org.x = work_area_origin_x_from_visible_area_and_scroll(&window_state); /* window w.a. ABS origin */
    viewevent_click.click_context.gdi_org.y = work_area_origin_y_from_visible_area_and_scroll(&window_state);

    point.x = pointer_info.x - viewevent_click.click_context.gdi_org.x;
    point.y = viewevent_click.click_context.gdi_org.y - pointer_info.y;

    viewevent_click.click_context.p_view = p_view;

    now = monotime();

    do_call = !only_if_it_moves                              ||
              (point.x != p_callback->last_mouse_position.x) ||
              (point.y != p_callback->last_mouse_position.y) ||
              (now - p_callback->last_mouse_time >= MONOTIMEDIFF_VALUE_FROM_MS(500)); /* SKS 30apr95 after 1.21 was 250 */

    if(do_call)
    {
        p_callback->last_mouse_position = point;
        p_callback->last_mouse_time = now;

        view_point_from_screen_point_and_context(&viewevent_click.view_point, (PC_GDI_POINT) &pointer_info.x, &viewevent_click.click_context);

        viewevent_click.data.drag.pixit_delta.x = viewevent_click.view_point.pixit_point.x - p_callback->first_mouse_position.x;
        viewevent_click.data.drag.pixit_delta.y = viewevent_click.view_point.pixit_point.y - p_callback->first_mouse_position.y;

        (* p_callback->p_ctrl_host_view->p_proc_view_event) (p_docu, t5_message, &viewevent_click);
    }

    if(scroll_if_outside_window)
    {
        /* make windows autoscroll if user moves pointer past the edge of the window */
        GDI_POINT scroll_by;

        scroll_by.x = 0;
        scroll_by.y = 0;

        trace_4(TRACE_APP_CLICK, TEXT("Window visible area (") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(") in screen coords"), window_state.visible_area.xmin, window_state.visible_area.ymin, window_state.visible_area.xmax, window_state.visible_area.ymax);
        trace_2(TRACE_APP_CLICK, TEXT("Mouse position (") S32_TFMT TEXT(",") S32_TFMT TEXT(")"), pointer_info.x, pointer_info.y);

        if(p_callback->p_ctrl_host_view->do_x_scale)
        {
            if(pointer_info.x < window_state.visible_area.xmin)
                scroll_by.x = -mouse_autoscroll_by(window_state.visible_area.xmin - pointer_info.x);

            if(pointer_info.x > window_state.visible_area.xmax)
                scroll_by.x = +mouse_autoscroll_by(pointer_info.x - window_state.visible_area.xmax);

            if(scroll_by.x)
            {
                if((viewevent_click.click_context.hwnd == p_view->pane[WIN_PANE].hwnd)            ||
                   (viewevent_click.click_context.hwnd == p_view->pane[WIN_PANE_SPLIT_VERT].hwnd) ||
                   (viewevent_click.click_context.hwnd == p_view->edge[WIN_BORDER_HORZ].hwnd)     ||
                   (viewevent_click.click_context.hwnd == p_view->edge[WIN_RULER_HORZ].hwnd)      )
                    p_view->pane[WIN_PANE_SPLIT_VERT].lastopen.xscroll = (p_view->pane[WIN_PANE           ].lastopen.xscroll += scroll_by.x);
                else
                    p_view->pane[WIN_PANE_SPLIT_DIAG].lastopen.xscroll = (p_view->pane[WIN_PANE_SPLIT_HORZ].lastopen.xscroll += scroll_by.x);
            }
        }

        if(p_callback->p_ctrl_host_view->do_y_scale)
        {
            if(pointer_info.y < window_state.visible_area.ymin)
                scroll_by.y = -mouse_autoscroll_by(window_state.visible_area.ymin - pointer_info.y);

            if(pointer_info.y > window_state.visible_area.ymax)
                scroll_by.y = +mouse_autoscroll_by(pointer_info.y - window_state.visible_area.ymax);

            if(scroll_by.y)
            {
                if((viewevent_click.click_context.hwnd == p_view->pane[WIN_PANE].hwnd)            ||
                   (viewevent_click.click_context.hwnd == p_view->pane[WIN_PANE_SPLIT_HORZ].hwnd) ||
                   (viewevent_click.click_context.hwnd == p_view->edge[WIN_BORDER_VERT].hwnd)     ||
                   (viewevent_click.click_context.hwnd == p_view->edge[WIN_RULER_VERT].hwnd)      )
                    p_view->pane[WIN_PANE_SPLIT_HORZ].lastopen.yscroll = (p_view->pane[WIN_PANE           ].lastopen.yscroll += scroll_by.y);
                else
                    p_view->pane[WIN_PANE_SPLIT_DIAG].lastopen.yscroll = (p_view->pane[WIN_PANE_SPLIT_VERT].lastopen.yscroll += scroll_by.y);
            }
        }

        if(scroll_by.x || scroll_by.y)
            host_view_reopen(p_docu, p_view);
    }
}

/* silly SKS function */

static GDI_COORD
mouse_autoscroll_by(
    _In_        GDI_COORD diff)
{
    if(diff > 32)
        return(1024); /* magic number */

    if(diff > 16)
        return(256); /* magic number */

    if(diff > 8)
        return(64); /* magic number */

    if(diff > 4)
        return(16); /* magic number */

    return(4); /* magic number */
}

static
struct MOVEMENT_STATICS
{
    BOOL allowed;
    BOOL active;
    BOOL activated;

    RCM_CALLBACK callback;

    struct MOVEMENT_STATICS_DEFERRED_LEAVE
    {
        P_CTRL_HOST_VIEW p_ctrl_host_view;
    }
    deferred_leave;
}
dragging_;

static RCM_CALLBACK tracking__callback;

static void
send_click_event_to_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    P_CTRL_HOST_VIEW p_ctrl_host_view,
    _In_        const WimpMouseClickEvent * const p_mouse_click_event)
{
    BOOL ctrl_pressed;
    BOOL shift_pressed;
    T5_MESSAGE t5_message;

    trace_0(TRACE_RISCOS_HOST, TEXT("host event: send_click_event_to_view"));

    if(T5_EVENT_NONE != (t5_message = button_click_event_encode(p_mouse_click_event, &ctrl_pressed, &shift_pressed)))
    {
        const P_HOST_XFORM p_host_xform = &p_view->host_xform[p_ctrl_host_view->xform_idx];

#if 0 /* handled on case-by-case basis now */
        if(shift_pressed) /* || ctrl_pressed PMF took this out */
        {
            switch(t5_message)
            {
            case T5_EVENT_CLICK_LEFT_SINGLE:
                t5_message = T5_EVENT_CLICK_RIGHT_SINGLE;
                break;

            case T5_EVENT_CLICK_LEFT_DRAG:
                t5_message = T5_EVENT_CLICK_RIGHT_DRAG;
                break;

            case T5_EVENT_CLICK_LEFT_DOUBLE:
                t5_message = T5_EVENT_CLICK_RIGHT_DOUBLE;
                break;

            case T5_EVENT_CLICK_LEFT_TRIPLE:
                t5_message = T5_EVENT_CLICK_RIGHT_TRIPLE;
                break;

            default:
                 break;
            }
        }
#endif

        {
        VIEWEVENT_CLICK viewevent_click;

        zero_struct(viewevent_click);

        /* NB click event x,y are screen coordinates (i.e. not window relative) */
        viewevent_click.click_context.hwnd = p_mouse_click_event->window_handle;
        viewevent_click.click_context.ctrl_pressed = ctrl_pressed;
        viewevent_click.click_context.shift_pressed = shift_pressed;
        set_click_context_gdi_org_from_screen(&viewevent_click.click_context, p_mouse_click_event->window_handle);
        host_set_click_context(p_docu, p_view, &viewevent_click.click_context, p_host_xform);

        view_point_from_screen_point_and_context(&viewevent_click.view_point, (PC_GDI_POINT) &p_mouse_click_event->mouse_x, &viewevent_click.click_context);

        dragging_.allowed = ((T5_EVENT_CLICK_LEFT_DRAG == t5_message) || (T5_EVENT_CLICK_RIGHT_DRAG == t5_message));

        if(dragging_.allowed)
        {
            /* Stash away some useful stuff needed by the null drag handler */
            dragging_.callback.last_mouse_position.x = p_mouse_click_event->mouse_x - viewevent_click.click_context.gdi_org.x;
            dragging_.callback.last_mouse_position.y = p_mouse_click_event->mouse_y - viewevent_click.click_context.gdi_org.y;
            dragging_.callback.last_mouse_time = 0;

            dragging_.callback.event_handle = (P_ANY) (uintptr_t) viewid_pack(p_docu, p_view, 0);
            dragging_.callback.p_ctrl_host_view = p_ctrl_host_view;
            dragging_.callback.p_host_xform = p_host_xform;

            dragging_.callback.viewevent_click = viewevent_click;
            dragging_.callback.first_mouse_position = viewevent_click.view_point.pixit_point;
        }

        (* p_ctrl_host_view->p_proc_view_event) (p_docu, t5_message, &viewevent_click);
        } /*block*/

        if(dragging_.activated)
        {
            dragging_.activated = 0;

            /* Send event to view. Don't scroll the window if the pointer is outside it */
            callback(T5_EVENT_CLICK_DRAG_STARTED, &dragging_.callback, FALSE, FALSE);
        }
    }
}

static void
send_pointer_enter_event_to_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    P_CTRL_HOST_VIEW p_ctrl_host_view,
    wimp_w hwnd)
{
    trace_0(TRACE_RISCOS_HOST, TEXT("host event: send_pointer_enter_event_to_view"));

    dragging_.deferred_leave.p_ctrl_host_view = NULL;

    trace_1(TRACE_OUT | TRACE_ANY, TEXT("send_pointer_enter_event_to_view(docno=%d) - *** null_events_start()"), docno_from_p_docu(p_docu));
    status_assert(null_events_start(docno_from_p_docu(p_docu), T5_EVENT_NULL, null_event_host_drag, 0));

    host_set_pointer_shape(POINTER_DEFAULT);

    {
    const P_HOST_XFORM p_host_xform = &p_view->host_xform[p_ctrl_host_view->xform_idx];
    WimpGetPointerInfoBlock pointer_info;
    VIEWEVENT_CLICK viewevent_click;

    zero_struct(viewevent_click);

    void_WrapOsErrorReporting(wimp_get_pointer_info(&pointer_info));

    /* NB pointer event x,y are screen coordinates (i.e. not window relative) */
    viewevent_click.click_context.hwnd = hwnd;
    viewevent_click.click_context.ctrl_pressed = FALSE;
    viewevent_click.click_context.shift_pressed = FALSE;
    set_click_context_gdi_org_from_screen(&viewevent_click.click_context, hwnd);
    host_set_click_context(p_docu, p_view, &viewevent_click.click_context, p_host_xform);

    view_point_from_screen_point_and_context(&viewevent_click.view_point, (PC_GDI_POINT) &pointer_info.x, &viewevent_click.click_context);

    /* Stash away some useful stuff needed by the null pointer tracking handler */
    tracking__callback.last_mouse_position.x = pointer_info.x - viewevent_click.click_context.gdi_org.x;
    tracking__callback.last_mouse_position.y = pointer_info.y - viewevent_click.click_context.gdi_org.y;
    tracking__callback.last_mouse_time = 0;

    tracking__callback.event_handle = (P_ANY) (uintptr_t) viewid_pack(p_docu, p_view, 0);
    tracking__callback.p_ctrl_host_view = p_ctrl_host_view;
    tracking__callback.p_host_xform = p_host_xform;

    tracking__callback.viewevent_click = viewevent_click;
    tracking__callback.first_mouse_position = viewevent_click.view_point.pixit_point;

    (* p_ctrl_host_view->p_proc_view_event) (p_docu, T5_EVENT_POINTER_ENTERS_WINDOW, &viewevent_click);
    } /*block*/
}

static void
send_pointer_leave_event_to_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    P_CTRL_HOST_VIEW p_ctrl_host_view)
{
    trace_0(TRACE_RISCOS_HOST, TEXT("host event: send_pointer_leave_event_to_view"));

    /* Window Manager sends us a pointer leaving window when we start a drag so we try to ignore it for a while */
    if(dragging_.active && (NULL == dragging_.deferred_leave.p_ctrl_host_view))
    {
        trace_0(TRACE_RISCOS_HOST, TEXT("host event: send_pointer_leave_event_to_view being deferred due to drag"));
        dragging_.deferred_leave.p_ctrl_host_view = p_ctrl_host_view;
        return;
    }

#if TRACE_ALLOWED
    if(dragging_.deferred_leave.p_ctrl_host_view)
        trace_0(TRACE_RISCOS_HOST, TEXT("host event: send_pointer_leave_event_to_view was deferred, now being sent"));
#endif

    dragging_.deferred_leave.p_ctrl_host_view = NULL;

    trace_1(TRACE_OUT | TRACE_ANY, TEXT("send_pointer_leave_event_to_view(docno=%d) - *** null_events_stop()"), docno_from_p_docu(p_docu));
    null_events_stop(docno_from_p_docu(p_docu), T5_EVENT_NULL, null_event_host_drag, 0);

    host_set_pointer_shape(POINTER_DEFAULT);

    { /* SKS 13aug93 - sufficient fudge to get it going */
    VIEWEVENT_CLICK viewevent_click;
    zero_struct(viewevent_click);
    viewevent_click.view_point.p_view = p_view;
    (* p_ctrl_host_view->p_proc_view_event) (p_docu, T5_EVENT_POINTER_LEAVES_WINDOW, &viewevent_click);
    } /*block*/
}

static void
send_release_event_to_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     T5_MESSAGE t5_message)
{
    trace_0(TRACE_RISCOS_HOST, TEXT("host event: send_release_event_to_view"));

    assert(dragging_.active);

    /* If aborted (rather than on receipt of Wimp_EUserDrag) then we need to cancel the current drag box */
    if(T5_EVENT_CLICK_DRAG_ABORTED == t5_message)
        void_WrapOsErrorReporting(winx_drag_box(NULL)); /* NB winx_drag_box() NOT wimp_drag_box() */

    /* Send dragging event to view only if mouse has moved. Scroll the window if the pointer is outside it. */
    callback(T5_EVENT_CLICK_DRAG_MOVEMENT, &dragging_.callback, TRUE, TRUE);

    /* Send drag finished event to view (even if mouse hasn't moved). Don't scroll the window if the pointer is outside it. */
    assert((T5_EVENT_CLICK_DRAG_ABORTED == t5_message) || (T5_EVENT_CLICK_DRAG_FINISHED == t5_message));
    callback(t5_message, &dragging_.callback, FALSE, FALSE);

    dragging_.active = FALSE;

    if(dragging_.deferred_leave.p_ctrl_host_view)
        send_pointer_leave_event_to_view(p_docu, p_view, dragging_.deferred_leave.p_ctrl_host_view);
}

_Check_return_
static BOOL
preprocess_key_event_ESCAPE(void)
{
    host_key_buffer_flush();

    trace_0(TRACE_OUT | TRACE_RISCOS_HOST, TEXT("key kmap=KMAP_ESCAPE, buffer flushed"));

    if(dragging_.active)
    {
        /* N.B. dragging_.callback.p_docu may not equal p_docu as we can drag with the right button in a document */
        /*      that doesn't have the input focus (its possible we may make right clicks claim focus one day)     */
        /*      the escape was sent to the document with the input focus (p_docu), but the user wants to abort    */
        /*      the drag happening in dragging_.callback.p_docu                                                   */
        P_VIEW dragging_p_view;
        P_DOCU dragging_p_docu = viewid_unpack((U32) dragging_.callback.event_handle, &dragging_p_view, NULL);

        trace_0(TRACE_OUT | TRACE_RISCOS_HOST, TEXT("key kmap=KMAP_ESCAPE processed, cancelling drag, buffer flushed"));

        send_release_event_to_view(dragging_p_docu, dragging_p_view, T5_EVENT_CLICK_DRAG_ABORTED);

        return(TRUE /*processed*/);
    }

    return(FALSE);
}

_Check_return_
static STATUS
send_key_event_to_docu(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     S32 keycode)
{
    ARRAY_HANDLE h_commands;
    T5_MESSAGE t5_message;
    BOOL fn_key;

    trace_0(TRACE_RISCOS_HOST, TEXT("host event: send_key_event_to_docu"));

    if((h_commands = command_array_handle_from_key_code(p_docu, keycode, &t5_message)) == 0)
    {
        if(t5_message == T5_CMD_ESCAPE)
            if(preprocess_key_event_ESCAPE())
                return(1 /*processed*/);

        fn_key = (T5_EVENT_NONE != t5_message);

        if(!fn_key)
        {
            if((keycode >= 0x20) && (keycode <= 0xFF))
                docu_modify(p_docu);
            else
            {
                trace_1(TRACE_OUT | TRACE_RISCOS_HOST, TEXT("key kmap=") U32_XTFMT TEXT(" unprocessed"), keycode);

                return(0 /*unprocessed*/);
            }
        }
    }
    else
    {
        UNREFERENCED_PARAMETER(h_commands);

        fn_key = 1;
    }

    status_return(host_key_cache_event(docno_from_p_docu(p_docu), keycode, fn_key, 1));

    return(1 /*processed*/);
}

/******************************************************************************
*
* NB. To be called by view, skel or lower layer only on receipt
* of T5_EVENT_CLICK_LEFT_DRAG or T5_EVENT_CLICK_RIGHT_DRAG
* This code relies on send_click_event_to_view() (through which
* we are currently threaded) having stashed away lots of info
*
******************************************************************************/

extern void
host_drag_start(
    _In_opt_    P_ANY p_reason_data)
{
    static S32 default_drag_reason_data = CB_CODE_NOREASON;

    wimp_w window_handle;
    WimpGetWindowStateBlock window_state;
    WimpDragBox dragstr;

    if(NULL == p_reason_data)
        p_reason_data = &default_drag_reason_data;

    dragging_.callback.viewevent_click.data.drag.p_reason_data = p_reason_data;

    window_handle = dragging_.callback.viewevent_click.click_context.hwnd;

    window_state.window_handle = window_handle;
    void_WrapOsErrorReporting(wimp_get_window_state(&window_state));

    dragstr.wimp_window = window_handle; /* Needed by winx_drag_box(), so it can send Wimp_EUserDrag to us */
    dragstr.drag_type = Wimp_DragBox_DragPoint;
    /* Window Manager ignores inner box on hidden drags */
    dragstr.parent_box.xmin = window_state.visible_area.xmin - 32 /*16*/;
    dragstr.parent_box.ymin = window_state.visible_area.ymin - 32;
    dragstr.parent_box.xmax = window_state.visible_area.xmax + 32 /*16*/;
    dragstr.parent_box.ymax = window_state.visible_area.ymax + 32;

    void_WrapOsErrorReporting(winx_drag_box(&dragstr)); /* NB winx_drag_box() NOT wimp_drag_box() */

    dragging_.active = 1;
    dragging_.activated = 1;

    /* All the null handler needs to fill in are view_point.pixit_point.(x,y), pixit_delta.(x,y) and context.(orgx,orgy) */
}

/******************************************************************************
*
* N.B. May be called by view, skel or lower layer only,
* usually on receipt of T5_EVENT_REDRAW, to determine if
* a drag is in progress on ANY of the documents windows
*
******************************************************************************/

_Check_return_
extern BOOL
host_drag_in_progress(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_P_ANY p_p_reason_data)
{
    if(dragging_.active)
    {
        P_VIEW dragging_p_view;
        P_DOCU dragging_p_docu = viewid_unpack((U32) dragging_.callback.event_handle, &dragging_p_view, NULL);

        if(dragging_p_docu == p_docu)
        {
            *p_p_reason_data = dragging_.callback.viewevent_click.data.drag.p_reason_data;
            return(TRUE);
        }
    }

    *p_p_reason_data = NULL;
    return(FALSE);
}

/******************************************************************************
*
* Process callback from null engine
*
******************************************************************************/

PROC_EVENT_PROTO(static, null_event_host_drag)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER(p_data);

#if CHECKING
    switch(t5_message)
    {
    default: default_unhandled();
        return(STATUS_OK);

    case T5_EVENT_NULL:
#else
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    {
#endif
        if(dragging_.active)
        {   /* Send event to view only if pointer has moved. Scroll the window if the pointer is outside it. */
            callback(T5_EVENT_CLICK_DRAG_MOVEMENT, &dragging_.callback, TRUE, TRUE);
        }
        else
        {   /* Send event to view only if pointer has moved. Don't scroll the window if the pointer is outside it. */
            callback(T5_EVENT_POINTER_MOVEMENT, &tracking__callback, TRUE, FALSE);
        }
        return(STATUS_OK);
    }
}

extern void
host_settitle(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_z_      PCTSTR title)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(0 != tstrcmp(p_view->window_title, title))
    {
        tstr_xstrkpy(p_view->window_title, WINDOW_TITLE_LEN, title);

        winx_changedtitle(p_view->main[WIN_BACK].hwnd);
    }
}

/*ncr*/
extern BOOL
host_show_caret(
    _HwndRef_   HOST_WND hwnd,
    _In_        int width,
    _In_        int height,
    _In_        int x,
    _In_        int y)
{
    WimpCaret caret_cur;
    WimpCaret caret_new;

    UNREFERENCED_PARAMETER(width);

    if(NULL != winx_get_caret_position(&caret_cur))
        zero_struct(caret_cur);

    caret_new.window_handle = hwnd;
    caret_new.icon_handle = BAD_WIMP_I;
    caret_new.xoffset = x;
    caret_new.yoffset = y;
    caret_new.height = height;
    caret_new.index = 0;

    if( (caret_cur.window_handle != caret_new.window_handle) ||
        (caret_cur.xoffset != caret_new.xoffset) ||
        (caret_cur.yoffset != caret_new.yoffset) ||
        (caret_cur.height != caret_new.height) /* cos this field holds caret (in)visible bit */ )
    {
        trace_3(TRACE_RISCOS_HOST, TEXT("host_show_caret (") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(")"), caret_new.xoffset, caret_new.yoffset, caret_new.height);
        void_WrapOsErrorReporting(winx_set_caret_position(&caret_new));
        return(TRUE);
    }

    return(FALSE);
}

extern void
host_main_show_caret(
    _DocuRef_   P_DOCU p_docu,
    P_VIEW_POINT p_view_point,
    _InVal_     PIXIT height)
{
    const P_VIEW p_view = p_view_point->p_view;
    VIEW_RECT view_rect; /* a rectangle to describe the caret bounds in view space */
    GDI_RECT gdi_rect; /* which is converted into a bounding box in wimp space   */
    int os_height;

    assert(height >= 0);

    view_rect.p_view    = p_view;                    /* NB similar code for calculating a caret bounding box */
    view_rect.rect.tl   = p_view_point->pixit_point; /*    exists in host_main_scroll_caret()                */
    view_rect.rect.br.x = view_rect.rect.tl.x;
    view_rect.rect.br.y = view_rect.rect.tl.y + height;

    window_rect_from_view_rect(&gdi_rect, P_DOCU_NONE, &view_rect, NULL, &p_view->host_xform[XFORM_PANE]);

    os_height = (gdi_rect.tl.y - gdi_rect.br.y);

    if(os_height <= 0)
        os_height  = 0x02000000; /* caret made invisible */
    else if(os_height < 32)
        os_height |= 0x01000000; /* VDU 5 (sharp) caret */

    if(host_show_caret(p_view->pane[p_view->cur_pane].hwnd, 0, os_height, gdi_rect.tl.x, gdi_rect.br.y))
    {
        if(!p_docu->flags.is_current)
        {
            DOCNO docno = DOCNO_NONE;

            while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
                p_docu_from_docno_valid(docno)->flags.is_current = 0;
        }

        p_docu->flags.is_current = 1;
    }
}

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
    VIEW_RECT view_rect;        /* a rectangle to describe the caret bounds in view space */
    WimpGetWindowStateBlock window_state;
    GDI_POINT scroll;
    GDI_RECT gdi_rect;
    GDI_POINT gdi_point;
    int visible_width, visible_height;

    if(HOST_WND_NONE == p_view->pane[p_view->cur_pane].hwnd)
        return;                         /* no preferred pane */

    /* left/right are relative to p_view_point position -
     * a rectangle which the caller wants:
     * a) kept on screen if possible
     * b) if caret is nearer to RHS, keep RHS on screen if possible
     * c) if caret is nearer to LHS, keep LHS on screen if possible
     * d) caret must be on screen whatever
     */
    view_rect.p_view = p_view;
    view_rect.rect.tl = view_rect.rect.br = p_view_point->pixit_point;
    view_rect.rect.tl.x += left;
    view_rect.rect.br.x += right;
    view_rect.rect.tl.y += top;
    view_rect.rect.br.y += bottom;

    window_rect_from_view_rect(&gdi_rect, P_DOCU_NONE, &view_rect, NULL, &p_view->host_xform[XFORM_PANE]);
    window_point_from_pixit_point(&gdi_point, &p_view_point->pixit_point, &p_view->host_xform[XFORM_PANE]);

    window_state.window_handle = p_view->pane[p_view->cur_pane].hwnd;
    void_WrapOsErrorReporting(wimp_get_window_state(&window_state));

    scroll.x = window_state.xscroll;
    scroll.y = window_state.yscroll;

    visible_width  = BBox_width(&window_state.visible_area);
    visible_height = BBox_height(&window_state.visible_area);

    /* MRJC 14.7.94 */
    gdi_rect.tl.x -= host_modevar_cache_current.dx;
    gdi_rect.br.x += 2 * host_modevar_cache_current.dx;

    if(gdi_rect.br.x - gdi_rect.tl.x > visible_width)
    {
        if(gdi_point.x < scroll.x + 1 * visible_width / 16)
            scroll.x = gdi_point.x - 1 * visible_width / 8;
        else if(gdi_point.x > scroll.x + 15 * visible_width / 16)
            scroll.x = gdi_point.x - 7 * visible_width / 8;
    }
    else if(gdi_rect.br.x > scroll.x + visible_width)
        scroll.x = gdi_rect.br.x - visible_width;
    else if(gdi_rect.tl.x < scroll.x)
        scroll.x = gdi_rect.tl.x;

    scroll.x = MAX(scroll.x, 0);

    /* MRJC 16.3.95 */
    if(gdi_rect.tl.y - gdi_rect.br.y > visible_height)
    {
        if(gdi_point.y > scroll.y - 1 * visible_height / 16)
            scroll.y = gdi_point.y + 1 * visible_height / 8;
        else if(gdi_point.y < scroll.y - 15 * visible_height / 16)
            scroll.y = gdi_point.y + 7 * visible_height / 8;
    }
    else if(gdi_rect.br.y < scroll.y - visible_height)
        scroll.y = gdi_rect.br.y + visible_height;
    else if(gdi_rect.tl.y > scroll.y)
        scroll.y = gdi_rect.tl.y;

    scroll.y = MIN(scroll.y, 0);

    if((scroll.x != window_state.xscroll) || (scroll.y != window_state.yscroll))
    {
        p_view->pane[p_view->cur_pane    ].lastopen.xscroll = scroll.x;
        p_view->pane[p_view->cur_pane    ].lastopen.yscroll = scroll.y;

        p_view->pane[p_view->cur_pane ^ 2].lastopen.xscroll = scroll.x;
        p_view->pane[p_view->cur_pane ^ 1].lastopen.yscroll = scroll.y;

        host_view_reopen(p_docu, p_view);
    }

}

extern void
host_main_hide_caret(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_ViewRef_(p_view);

#if FALSE
    myassert2(TEXT("why is host_main_hide_caret(") PTR_XTFMT TEXT(", ") PTR_XTFMT TEXT(") being called"), p_docu, p_view);
#endif
}

extern void
host_scroll_pane(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     T5_MESSAGE t5_message,
    P_VIEW_POINT p_view_point)
{
    int yscroll, visible_height, scroll;

    DOCU_ASSERT(p_docu);
    VIEW_ASSERT(p_view);
    assert((t5_message == T5_CMD_PAGE_DOWN) || (t5_message == T5_CMD_PAGE_UP) || (t5_message == T5_CMD_SHIFT_PAGE_DOWN) || (t5_message == T5_CMD_SHIFT_PAGE_UP));

    if(HOST_WND_NONE == p_view->pane[p_view->cur_pane].hwnd)
        return;                         /* no preferred pane */

    {
    WimpGetWindowStateBlock window_state;
    window_state.window_handle = p_view->pane[p_view->cur_pane].hwnd;
    void_WrapOsErrorReporting(wimp_get_window_state(&window_state));
    visible_height = BBox_height(&window_state.visible_area);
    } /*block*/

    scroll = ((t5_message == T5_CMD_PAGE_DOWN) || (t5_message == T5_CMD_SHIFT_PAGE_DOWN)) ? -visible_height : +visible_height;

    yscroll = p_view->pane[p_view->cur_pane].lastopen.yscroll + scroll;
    yscroll = MIN(yscroll, 0);

    p_view->pane[p_view->cur_pane    ].lastopen.yscroll = yscroll;
    p_view->pane[p_view->cur_pane ^ 1].lastopen.yscroll = yscroll;

    host_view_reopen(p_docu, p_view);

    {
    GDI_POINT gdi_point;
    VIEW_POINT view_point;

    view_point.p_view  = p_view_point->p_view;

    gdi_point.x = 0;
    gdi_point.y = scroll;

    pixit_point_from_window_point(&view_point.pixit_point, &gdi_point, &p_view->host_xform[XFORM_PANE]);

    p_view_point->pixit_point.x += view_point.pixit_point.x;
    p_view_point->pixit_point.y += view_point.pixit_point.y;

    p_view_point->pixit_point.x = MAX(0, p_view_point->pixit_point.x);
    p_view_point->pixit_point.y = MAX(0, p_view_point->pixit_point.y);
    } /*block*/
}

extern void
host_update_all(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     REDRAW_TAG redraw_tag)
{
    int index[2], i;
    WimpRedrawWindowBlock redraw_window_block;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(threaded_through_redraw)
        return;

    redraw_window_block.visible_area.xmin = 0;
    redraw_window_block.visible_area.ymin = p_view->scaled_paneextent.y0;
    redraw_window_block.visible_area.xmax = p_view->scaled_paneextent.x1;
    redraw_window_block.visible_area.ymax = 0;

    switch(redraw_tag)
    {
    case UPDATE_BACK_WINDOW:
        {
        redraw_window_block.window_handle = p_view->main[WIN_BACK].hwnd;

        if(HOST_WND_NONE == redraw_window_block.window_handle)
            return;

        void_WrapOsErrorReporting(wimp_force_redraw_BBox(redraw_window_block.window_handle, &redraw_window_block.visible_area));
        return;
        }

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

            redraw_window_block.window_handle = p_view->pane[pane_id].hwnd;

            if(HOST_WND_NONE == redraw_window_block.window_handle)
                continue;

            void_WrapOsErrorReporting(wimp_force_redraw_BBox(redraw_window_block.window_handle, &redraw_window_block.visible_area));

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

    for(i = 0; i <= 1; ++i)
    {
        redraw_window_block.window_handle = p_view->edge[index[i]].hwnd;

        if(HOST_WND_NONE == redraw_window_block.window_handle)
            continue;

        void_WrapOsErrorReporting(wimp_force_redraw_BBox(redraw_window_block.window_handle, &redraw_window_block.visible_area));
    }
}

/* perform update later on all windows on this view (bar the main windows) */

extern void
host_all_update_later(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEW_RECT p_view_rect)
{
    REDRAW_FLAGS redraw_flags;
    REDRAW_FLAGS_CLEAR(redraw_flags);

    host_update(p_docu, p_view_rect, NULL, redraw_flags, UPDATE_PANE_PAPER); /*biggest layer, covers all other*/
    host_update(p_docu, p_view_rect, NULL, redraw_flags, UPDATE_BORDER_HORZ);
    host_update(p_docu, p_view_rect, NULL, redraw_flags, UPDATE_BORDER_VERT);
    host_update(p_docu, p_view_rect, NULL, redraw_flags, UPDATE_RULER_HORZ);
    host_update(p_docu, p_view_rect, NULL, redraw_flags, UPDATE_RULER_VERT);
}

static void
host_update_back_window(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEW_RECT p_view_rect,
    _InRef_opt_ PC_RECT_FLAGS p_rect_flags,
    _InVal_     REDRAW_FLAGS redraw_flags)
{
    const P_VIEW p_view = p_view_rect->p_view;
    const P_HOST_XFORM p_host_xform = &p_view->host_xform[XFORM_BACK];
    VIEWEVENT_REDRAW viewevent_redraw;
    WimpRedrawWindowBlock redraw_window_block;
    int wimp_more = 0;

    redraw_window_block.window_handle = p_view->main[WIN_BACK].hwnd;

    if(HOST_WND_NONE == redraw_window_block.window_handle)
        return;

    {
    GDI_RECT gdi_rect;
    window_rect_from_view_rect(&gdi_rect, p_docu, p_view_rect, p_rect_flags, p_host_xform);
    redraw_window_block.visible_area.xmin = gdi_rect.tl.x;
    redraw_window_block.visible_area.ymin = gdi_rect.br.y;
    redraw_window_block.visible_area.xmax = gdi_rect.br.x;
    redraw_window_block.visible_area.ymax = gdi_rect.tl.y;
    } /*block*/

    if(redraw_flags.update_now)
    {
        update_common_pre_loop(p_docu, p_view, &viewevent_redraw, p_host_xform, redraw_window_block.window_handle, redraw_flags);

        if(NULL != WrapOsErrorReporting(wimp_update_window(&redraw_window_block, &wimp_more)))
            wimp_more = 0;

        while(0 != wimp_more)
        {
            update_common_in_loop(p_docu, p_view, &viewevent_redraw, &redraw_window_block);

            threaded_through_redraw = TRUE;
            (* window_ctrl[WINTYPE_BACK].p_proc_view_event) (p_docu, T5_EVENT_REDRAW, &viewevent_redraw);
            threaded_through_redraw = FALSE;

            host_restore_clip_rectangle(&viewevent_redraw.redraw_context);

            if(NULL != WrapOsErrorReporting(wimp_get_rectangle(&redraw_window_block, &wimp_more)))
                wimp_more = 0;
        }
    }
    else
    {
        void_WrapOsErrorReporting(wimp_force_redraw_BBox(redraw_window_block.window_handle, &redraw_window_block.visible_area));
    }
}

static void
host_update_pane_window(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEW_RECT p_view_rect,
    _InRef_opt_ PC_RECT_FLAGS p_rect_flags,
    _InVal_     REDRAW_FLAGS redraw_flags)
{
    const P_VIEW p_view = p_view_rect->p_view;
    const P_HOST_XFORM p_host_xform = &p_view->host_xform[XFORM_PANE];
    VIEWEVENT_REDRAW viewevent_redraw;
    WimpRedrawWindowBlock redraw_window_block;
    int wimp_more = 0;
    PANE_ID pane_id = NUMPANE;

    do  {
        PANE_ID_DECR(pane_id);

        redraw_window_block.window_handle = p_view->pane[pane_id].hwnd;

        if(HOST_WND_NONE == redraw_window_block.window_handle)
            continue;

        {
        GDI_RECT gdi_rect;
        window_rect_from_view_rect(&gdi_rect, p_docu, p_view_rect, p_rect_flags, p_host_xform);
        redraw_window_block.visible_area.xmin = gdi_rect.tl.x;
        redraw_window_block.visible_area.ymin = gdi_rect.br.y;
        redraw_window_block.visible_area.xmax = gdi_rect.br.x;
        redraw_window_block.visible_area.ymax = gdi_rect.tl.y;
        } /*block*/

        if(redraw_flags.update_now)
        {
            update_common_pre_loop(p_docu, p_view, &viewevent_redraw, p_host_xform, redraw_window_block.window_handle, redraw_flags);

            if(NULL != WrapOsErrorReporting(wimp_update_window(&redraw_window_block, &wimp_more)))
                wimp_more = 0;

            while(0 != wimp_more)
            {
                update_common_in_loop(p_docu, p_view, &viewevent_redraw, &redraw_window_block);

                threaded_through_redraw = TRUE;
                (* window_ctrl[WINTYPE_PANE].p_proc_view_event) (p_docu, T5_EVENT_REDRAW, &viewevent_redraw);
                threaded_through_redraw = FALSE;

                host_restore_clip_rectangle(&viewevent_redraw.redraw_context);

                if(NULL != WrapOsErrorReporting(wimp_get_rectangle(&redraw_window_block, &wimp_more)))
                    wimp_more = 0;
            }
        }
        else
        {
            void_WrapOsErrorReporting(wimp_force_redraw_BBox(redraw_window_block.window_handle, &redraw_window_block.visible_area));
        }

    } while(PANE_ID_START != pane_id);
}

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
    P_HOST_XFORM p_host_xform;
    P_PROC_EVENT p_proc_event;
    VIEWEVENT_REDRAW viewevent_redraw;
    WimpRedrawWindowBlock redraw_window_block;
    int wimp_more = 0;

    UNREFERENCED_PARAMETER_InVal_(redraw_flags);

    if(threaded_through_redraw)
        return;

    switch(redraw_tag)
    {
    case UPDATE_BACK_WINDOW:
        host_update_back_window(p_docu, p_view_rect, p_rect_flags, redraw_flags);
        return;

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
        host_update_pane_window(p_docu, p_view_rect, p_rect_flags, redraw_flags);
        return;

    case UPDATE_BORDER_HORZ:
        index[0] = WIN_BORDER_HORZ_SPLIT;
        index[1] = WIN_BORDER_HORZ;
        p_host_xform = &p_view->host_xform[XFORM_HORZ];
        p_proc_event = view_event_border_horz;
        break;

    case UPDATE_BORDER_VERT:
        index[0] = WIN_BORDER_VERT_SPLIT;
        index[1] = WIN_BORDER_VERT;
        p_host_xform = &p_view->host_xform[XFORM_VERT];
        p_proc_event = view_event_border_vert;
        break;

    case UPDATE_RULER_HORZ:
        index[0] = WIN_RULER_HORZ_SPLIT;
        index[1] = WIN_RULER_HORZ;
        p_host_xform = &p_view->host_xform[XFORM_HORZ];
        p_proc_event = view_event_ruler_horz;
        break;

    case UPDATE_RULER_VERT:
        index[0] = WIN_RULER_VERT_SPLIT;
        index[1] = WIN_RULER_VERT;
        p_host_xform = &p_view->host_xform[XFORM_VERT];
        p_proc_event = view_event_ruler_vert;
        break;
    }

    /* for horizontal windows, only tl.x and br.x have meaning, for vertical windows only tl.y and br.y  */
    /* we must therefore supply appropriate y (or x) limits. We copy the rectangle given, rather than    */
    /* poke it directly which allows the caller to build one rectangle (say for current slot) and use it */
    /* to update both horizontal and vertical window regions (such as column border and row border).     */

    for(i = 0; i <= 1; ++i)
    {
        redraw_window_block.window_handle = p_view->edge[index[i]].hwnd;

        if(HOST_WND_NONE == redraw_window_block.window_handle)
            continue;

        {
        GDI_RECT gdi_rect;
        window_rect_from_view_rect(&gdi_rect, p_docu, p_view_rect, p_rect_flags, p_host_xform);
        redraw_window_block.visible_area.xmin = gdi_rect.tl.x;
        redraw_window_block.visible_area.ymin = gdi_rect.br.y;
        redraw_window_block.visible_area.xmax = gdi_rect.br.x;
        redraw_window_block.visible_area.ymax = gdi_rect.tl.y;
        } /*block*/

        if(redraw_flags.update_now)
        {
            update_common_pre_loop(p_docu, p_view, &viewevent_redraw, p_host_xform, redraw_window_block.window_handle, redraw_flags);

            if(NULL != WrapOsErrorReporting(wimp_update_window(&redraw_window_block, &wimp_more)))
                wimp_more = 0;

            while(0 != wimp_more)
            {
                update_common_in_loop(p_docu, p_view, &viewevent_redraw, &redraw_window_block);

                threaded_through_redraw = TRUE;
                (* p_proc_event) (p_docu, T5_EVENT_REDRAW, &viewevent_redraw);
                threaded_through_redraw = FALSE;

                host_restore_clip_rectangle(&viewevent_redraw.redraw_context);

                if(NULL != WrapOsErrorReporting(wimp_get_rectangle(&redraw_window_block, &wimp_more)))
                    wimp_more = 0;
            }
        }
        else
        {
            void_WrapOsErrorReporting(wimp_force_redraw_BBox(redraw_window_block.window_handle, &redraw_window_block.visible_area));
        }
    }
}

_Check_return_
extern BOOL
host_update_fast_continue(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_VIEWEVENT_REDRAW p_viewevent_redraw)
{
    const P_REDRAW_CONTEXT p_redraw_context = &p_viewevent_redraw->redraw_context;
    const P_VIEW p_view = p_viewevent_redraw->p_view;
    WimpRedrawWindowBlock redraw_window_block;
    int wimp_more = 0;
    BOOL more;

    host_restore_clip_rectangle(p_redraw_context);

    redraw_window_block.window_handle = p_redraw_context->riscos.hwnd;

    if(NULL != WrapOsErrorReporting(wimp_get_rectangle(&redraw_window_block, &wimp_more)))
        wimp_more = 0;

    more = (wimp_more != 0);  /* Don't just return wimp_more, cos its 0/-1 !!! */

    if(more)
    {
        update_common_in_loop(p_docu, p_view, p_viewevent_redraw, &redraw_window_block);
        return(TRUE);
    }

    threaded_through_redraw = FALSE;

    host_paint_end(&p_viewevent_redraw->redraw_context);

    return(FALSE);
}

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
    PANE_ID pane_id = NUMPANE;
    WimpRedrawWindowBlock redraw_window_block;
    BOOL more;

    UNREFERENCED_PARAMETER_InVal_(redraw_tag);

    if(threaded_through_redraw)
        return(FALSE);          /* Don't be mean to the wimp, it bites back! */

    {
    GDI_RECT gdi_rect;
    window_rect_from_view_rect(&gdi_rect, p_docu, p_view_rect, &rect_flags, &p_view->host_xform[XFORM_PANE]);
    redraw_window_block.visible_area.xmin = gdi_rect.tl.x;
    redraw_window_block.visible_area.ymin = gdi_rect.br.y;
    redraw_window_block.visible_area.xmax = gdi_rect.br.x;
    redraw_window_block.visible_area.ymax = gdi_rect.tl.y;
    } /*block*/

    /* mark all panes except cur_pane for update_later */
    do  {
        PANE_ID_DECR(pane_id);

        redraw_window_block.window_handle = p_view->pane[pane_id].hwnd;

        if(HOST_WND_NONE == redraw_window_block.window_handle)
            continue;

#if FAST_UPDATE_CALLS_NOTELAYER
        if(pane_id == p_view->cur_pane)
            continue;
#endif

        void_WrapOsErrorReporting(wimp_force_redraw_BBox(redraw_window_block.window_handle, &redraw_window_block.visible_area));

    } while(PANE_ID_START != pane_id);

    host_paint_start(&p_viewevent_redraw->redraw_context);

    /* do a fast update of the cur_pane (if any) */
    redraw_window_block.window_handle = p_view->pane[p_view->cur_pane].hwnd;

    if(HOST_WND_NONE != redraw_window_block.window_handle)
    {
        REDRAW_FLAGS redraw_flags;
        int wimp_more = 0;

        REDRAW_FLAGS_CLEAR(redraw_flags);
        redraw_flags.show_content = TRUE;
        redraw_flags.show_selection = TRUE;

        update_common_pre_loop(p_docu, p_view, p_viewevent_redraw, &p_view->host_xform[XFORM_PANE], redraw_window_block.window_handle, redraw_flags);

        if(NULL != WrapOsErrorReporting(wimp_update_window(&redraw_window_block, &wimp_more)))
            wimp_more = 0;

        more = (wimp_more != 0); /* Don't just return wimp_more, 'cos its 0 or -1 !!! */

        if(more)
        {
            threaded_through_redraw = TRUE;
            update_common_in_loop(p_docu, p_view, p_viewevent_redraw, &redraw_window_block);
            threaded_through_redraw = FALSE; /* SKS is somewhat suspicious of this */
            return(TRUE);
        }
    }

    host_paint_end(&p_viewevent_redraw->redraw_context);

    return(FALSE);
}

extern void
host_recache_visible_row_range(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    PANE_ID pane_id = NUMPANE;

    do  {
        PANE_ID_DECR(pane_id);

        if(HOST_WND_NONE != p_view->pane[pane_id].hwnd)
            skel_visible_row_range(p_docu, p_view, &p_view->pane[pane_id].visible_skel_rect, &p_view->pane[pane_id].visible_row_range);

    } while(PANE_ID_START != pane_id);
}

#define HORZ_INTERPANE_GAP 0
#define BORDER_HORZ_GAP 0
#define RULER_HORZ_GAP  0
#define VERT_INTERPANE_GAP 0
#define BORDER_VERT_GAP 0
#define RULER_VERT_GAP  0

/* HORZ_INTERPANE_GAP or VERT_INTERPANE_GAP of zero means that the pane_window work areas abut
 * i.e. a split pane's border overlaps other pane's work area, calc_window_positions() kludges around this!
 * A gap of one pixel would cause the pane_window borders to overlap, whilst a gap of 3 pixels would
 * leave a one pixel strip of the back window visible between the panes
 */

/* SKS notes that these are only used in one place, all the rest are explicit 0's at the moment!          */

static void
calc_window_positions(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    /*out*/ P_WINDOW_POSITIONS p_w_p,
    _InoutRef_  P_BBox p_bbox)
{
    int con_x_5 = p_bbox->xmin + p_view->margin.x0;
    int con_x_6 = con_x_5 + (p_view->flags.vert_ruler_on  ? p_view->vert_ruler_gdi_width : 0);
    int con_x_6_= con_x_6 + (p_view->flags.vert_border_on ? p_view->vert_border_gdi_width : 0);
    int con_x_7 = con_x_6 + (p_view->flags.vert_border_on ? p_view->vert_border_gdi_width : (p_view->flags.vert_ruler_on ? 4 : 0)); /* overlap border or abutt */
    int con_x_8 = con_x_7 + (p_view->flags.horz_split_on ? (int) (p_view->horz_split_pos / PIXITS_PER_RISCOS) : 0);
    int con_x_9 = p_bbox->xmax + p_view->margin.x1;
    int scr_x_7 = p_view->pane[WIN_PANE_SPLIT_HORZ].lastopen.xscroll;
    int scr_x_8 = p_view->pane[WIN_PANE           ].lastopen.xscroll;

    int con_y_5 = p_bbox->ymax + p_view->margin.y1;
    int con_y_6 = con_y_5 - (p_view->flags.horz_ruler_on  ? p_view->horz_ruler_gdi_height : 0);
    int con_y_6_= con_y_6 - (p_view->flags.horz_border_on ? p_view->horz_border_gdi_height : 0);
    int con_y_7 = con_y_6 - (p_view->flags.horz_border_on ? p_view->horz_border_gdi_height : 4); /* overlap border or abutt */
    int con_y_8 = con_y_7 - (p_view->flags.vert_split_on ? (int) (p_view->vert_split_pos / PIXITS_PER_RISCOS) : 0);
    int con_y_9 = p_bbox->ymin + p_view->margin.y0;
    int scr_y_7 = p_view->pane[WIN_PANE_SPLIT_VERT].lastopen.yscroll;
    int scr_y_8 = p_view->pane[WIN_PANE           ].lastopen.yscroll;

    /* if the panes are split and the values of p_view->horz_split_pos or p_view->vert_split_pos are small, clicking */
    /* on 'fullsize' can cause us to try and open WIN_PANE with dimensions bigger than its extent, so check for this */
    /* and open the other windows a bit bigger (i.e. adjust con_x_8 or con_y_8)                                      */

    int excess_x = (con_x_9 - con_x_8) - (p_view->scaled_paneextent.x1 - p_view->scaled_paneextent.x0);
    int excess_y = (con_y_8 - con_y_9) - (p_view->scaled_paneextent.y1 - p_view->scaled_paneextent.y0);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(p_view->flags.horz_split_on  && (excess_x > 0))
    {
        con_x_8 += excess_x;
        p_view->horz_split_pos += excess_x * PIXITS_PER_RISCOS;
    }

    if(p_view->flags.vert_split_on && (excess_y > 0))
    {
        con_y_8 -= excess_y;
        p_view->vert_split_pos -= excess_y * PIXITS_PER_RISCOS;
    }

    /* The number of edge windows, split panes and the back window size may all conspire to give us a
     * very small (or negative) size for WIN_PANE, so kludge it and the back window if this happens
     * Enlarging WIN_PANE and the back window may give us values that hang off the edge of the
     * screen, but lets not worry about that for now!
    */
    excess_x = (40) - (con_x_9 - con_x_8);
    if(excess_x > 0)
    {
        con_x_9      += excess_x;
        p_bbox->xmax += excess_x;
    }

    excess_y = (40) - (con_y_8 - con_y_9);
    if(excess_y > 0)
    {
        con_y_9      -= excess_y;
        p_bbox->ymin -= excess_y;
    }

    /* set 'x' ordinates for RULER_VERT and BORDER_VERT */

    p_w_p->edge[WIN_RULER_VERT_SPLIT ].visible_area.xmin    = p_w_p->edge[WIN_RULER_VERT       ].visible_area.xmin  = con_x_5;
    p_w_p->edge[WIN_RULER_VERT_SPLIT ].visible_area.xmax    = p_w_p->edge[WIN_RULER_VERT       ].visible_area.xmax  = con_x_6;
    p_w_p->edge[WIN_RULER_VERT_SPLIT ].xscroll              = p_w_p->edge[WIN_RULER_VERT       ].xscroll            = 0;

    p_w_p->edge[WIN_BORDER_VERT_SPLIT].visible_area.xmin    = p_w_p->edge[WIN_BORDER_VERT      ].visible_area.xmin  = con_x_6;
    p_w_p->edge[WIN_BORDER_VERT_SPLIT].visible_area.xmax    = p_w_p->edge[WIN_BORDER_VERT      ].visible_area.xmax  = con_x_6_;
    p_w_p->edge[WIN_BORDER_VERT_SPLIT].xscroll              = p_w_p->edge[WIN_BORDER_VERT      ].xscroll            = 0;

    /* set 'x' ordinates for RULER_HORZ and BORDER_HORZ */

    p_w_p->edge[WIN_RULER_HORZ_SPLIT ].visible_area.xmin    = p_w_p->edge[WIN_BORDER_HORZ_SPLIT].visible_area.xmin  = con_x_7;
    p_w_p->edge[WIN_RULER_HORZ_SPLIT ].visible_area.xmax    = p_w_p->edge[WIN_BORDER_HORZ_SPLIT].visible_area.xmax  = con_x_8;
    p_w_p->edge[WIN_RULER_HORZ_SPLIT ].xscroll              = p_w_p->edge[WIN_BORDER_HORZ_SPLIT].xscroll            = scr_x_7;

    p_w_p->edge[WIN_RULER_HORZ       ].visible_area.xmin    = p_w_p->edge[WIN_BORDER_HORZ      ].visible_area.xmin  = con_x_8;
    p_w_p->edge[WIN_RULER_HORZ       ].visible_area.xmax    = p_w_p->edge[WIN_BORDER_HORZ      ].visible_area.xmax  = con_x_9;
    p_w_p->edge[WIN_RULER_HORZ       ].xscroll              = p_w_p->edge[WIN_BORDER_HORZ      ].xscroll            = scr_x_8;

    /* set 'x' ordinates for panes */

    p_w_p->pane[WIN_PANE_SPLIT_HORZ  ].visible_area.xmin    = p_w_p->pane[WIN_PANE_SPLIT_DIAG  ].visible_area.xmin  = con_x_7;
    p_w_p->pane[WIN_PANE_SPLIT_HORZ  ].visible_area.xmax    = p_w_p->pane[WIN_PANE_SPLIT_DIAG  ].visible_area.xmax  = con_x_8 - 1; /* mind the gap */
    p_w_p->pane[WIN_PANE_SPLIT_HORZ  ].xscroll              = p_w_p->pane[WIN_PANE_SPLIT_DIAG  ].xscroll            = scr_x_7;

    p_w_p->pane[WIN_PANE             ].visible_area.xmin    = p_w_p->pane[WIN_PANE_SPLIT_VERT  ].visible_area.xmin  = con_x_8;
    p_w_p->pane[WIN_PANE             ].visible_area.xmax    = p_w_p->pane[WIN_PANE_SPLIT_VERT  ].visible_area.xmax  = con_x_9;
    p_w_p->pane[WIN_PANE             ].xscroll              = p_w_p->pane[WIN_PANE_SPLIT_VERT  ].xscroll            = scr_x_8;

    /* set 'y' ordinates for RULER_HORZ and BORDER_HORZ */

    p_w_p->edge[WIN_RULER_HORZ_SPLIT ].visible_area.ymax    = p_w_p->edge[WIN_RULER_HORZ       ].visible_area.ymax  = con_y_5;
    p_w_p->edge[WIN_RULER_HORZ_SPLIT ].visible_area.ymin    = p_w_p->edge[WIN_RULER_HORZ       ].visible_area.ymin  = con_y_6;
    p_w_p->edge[WIN_RULER_HORZ_SPLIT ].yscroll              = p_w_p->edge[WIN_RULER_HORZ       ].yscroll            = 0;

    p_w_p->edge[WIN_BORDER_HORZ_SPLIT].visible_area.ymax    = p_w_p->edge[WIN_BORDER_HORZ      ].visible_area.ymax  = con_y_6;
    p_w_p->edge[WIN_BORDER_HORZ_SPLIT].visible_area.ymin    = p_w_p->edge[WIN_BORDER_HORZ      ].visible_area.ymin  = con_y_6_;
    p_w_p->edge[WIN_BORDER_HORZ_SPLIT].yscroll              = p_w_p->edge[WIN_BORDER_HORZ      ].yscroll            = 0;

    /* set 'y' ordinates for RULER_VERT and BORDER_VERT */

    p_w_p->edge[WIN_RULER_VERT_SPLIT ].visible_area.ymax    = p_w_p->edge[WIN_BORDER_VERT_SPLIT].visible_area.ymax  = con_y_7;
    p_w_p->edge[WIN_RULER_VERT_SPLIT ].visible_area.ymin    = p_w_p->edge[WIN_BORDER_VERT_SPLIT].visible_area.ymin  = con_y_8;
    p_w_p->edge[WIN_RULER_VERT_SPLIT ].yscroll              = p_w_p->edge[WIN_BORDER_VERT_SPLIT].yscroll            = scr_y_7;

    p_w_p->edge[WIN_RULER_VERT       ].visible_area.ymax    = p_w_p->edge[WIN_BORDER_VERT      ].visible_area.ymax  = con_y_8;
    p_w_p->edge[WIN_RULER_VERT       ].visible_area.ymin    = p_w_p->edge[WIN_BORDER_VERT      ].visible_area.ymin  = con_y_9;
    p_w_p->edge[WIN_RULER_VERT       ].yscroll              = p_w_p->edge[WIN_BORDER_VERT      ].yscroll            = scr_y_8;

    /* set 'y' ordinates for panes */

    p_w_p->pane[WIN_PANE_SPLIT_DIAG  ].visible_area.ymax    = p_w_p->pane[WIN_PANE_SPLIT_VERT  ].visible_area.ymax  = con_y_7;
    p_w_p->pane[WIN_PANE_SPLIT_DIAG  ].visible_area.ymin    = p_w_p->pane[WIN_PANE_SPLIT_VERT  ].visible_area.ymin  = con_y_8 + host_modevar_cache_current.dy; /* mind the gap */
    p_w_p->pane[WIN_PANE_SPLIT_DIAG  ].yscroll              = p_w_p->pane[WIN_PANE_SPLIT_VERT  ].yscroll            = scr_y_7;

    p_w_p->pane[WIN_PANE_SPLIT_HORZ  ].visible_area.ymax    = p_w_p->pane[WIN_PANE             ].visible_area.ymax  = con_y_8;
    p_w_p->pane[WIN_PANE_SPLIT_HORZ  ].visible_area.ymin    = p_w_p->pane[WIN_PANE             ].visible_area.ymin  = con_y_9;
    p_w_p->pane[WIN_PANE_SPLIT_HORZ  ].yscroll              = p_w_p->pane[WIN_PANE             ].yscroll            = scr_y_8;
}

/******************************************************************************
*
* Set the window extents of all the windows in the given view
*
* Typically called when some aspect of the view layout changes
*
* e.g. switching border/ruler windows on/off or when the zoom factor changes
*
******************************************************************************/

extern void
host_set_extent_this_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    BBox pane_box, edge_box, back_box;
    VIEW_RECT view_rect;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    view_rect.p_view = p_view;
    view_rect.rect.tl.x = view_rect.rect.tl.y = 0;
    view_rect.rect.br = p_view->paneextent;

    {
    GDI_RECT gdi_rect;
    int x, y, sx, sy;

    window_rect_from_view_rect(&gdi_rect, P_DOCU_NONE, &view_rect, NULL, &p_view->host_xform[XFORM_PANE]);
    x = gdi_rect.br.x;
    y = gdi_rect.br.y;

    window_rect_from_view_rect(&gdi_rect, P_DOCU_NONE, &view_rect, NULL, &screen_host_xform);
    sx = gdi_rect.br.x;
    sy = gdi_rect.br.y;
    UNREFERENCED_PARAMETER(sy);

    x = MAX(x, sx); /* never let x pane extent fall below 100% worth*/

    pane_box.xmin = 0;
    pane_box.ymin = y;
    pane_box.xmax = x;
    pane_box.ymax = 0;
    } /*block*/

    if( (pane_box.xmin != p_view->scaled_paneextent.x0) || (pane_box.ymin != p_view->scaled_paneextent.y0) ||
        (pane_box.xmax != p_view->scaled_paneextent.x1) || (pane_box.ymax != p_view->scaled_paneextent.y1) )
    {
        {
        PANE_ID pane_id = NUMPANE;

        do  {
            PANE_ID_DECR(pane_id);

            if(HOST_WND_NONE != p_view->pane[pane_id].hwnd)
                void_WrapOsErrorReporting(wimp_set_extent(p_view->pane[pane_id].hwnd, &pane_box));

        } while(PANE_ID_START != pane_id);
        } /*block*/

        {
        EDGE_ID edge_id = NUMEDGE;

        do  {
            EDGE_ID_DECR(edge_id);

            switch(edge_id)
            {
            default: default_unhandled();
#if CHECKING
            case WIN_BORDER_HORZ_SPLIT:
            case WIN_BORDER_HORZ:
            case WIN_RULER_HORZ_SPLIT:
            case WIN_RULER_HORZ:
#endif
                edge_box.xmin = 0;
                edge_box.ymin = -1024;
                edge_box.xmax = pane_box.xmax;
                edge_box.ymax = 0;
                break;

            case WIN_BORDER_VERT_SPLIT:
            case WIN_BORDER_VERT:
            case WIN_RULER_VERT_SPLIT:
            case WIN_RULER_VERT:
                edge_box.xmin = 0;
                edge_box.ymin = pane_box.ymin;
                edge_box.xmax = 1280;
                edge_box.ymax = 0;
                break;
            }

            if(HOST_WND_NONE != p_view->edge[edge_id].hwnd)
                void_WrapOsErrorReporting(wimp_set_extent(p_view->edge[edge_id].hwnd, &edge_box));
        } while(edge_id != EDGE_ID_START);
        } /*block*/

        * (BBox *) &p_view->scaled_paneextent = pane_box;
    }

    back_box.xmin = 0;
    back_box.ymin = (p_view->margin.y1 - p_view->margin.y0) - (pane_box.ymax - pane_box.ymin);
    back_box.xmax = (p_view->margin.x0 - p_view->margin.x1) + (pane_box.xmax - pane_box.xmin);
    back_box.ymax = 0;

    if(p_view->flags.horz_split_on)
        back_box.xmax += (pane_box.xmax - pane_box.xmin) + VERT_INTERPANE_GAP;

    if(p_view->flags.vert_ruler_on)
        back_box.xmax += p_view->vert_ruler_gdi_width + RULER_VERT_GAP;

    if(p_view->flags.vert_border_on)
        back_box.xmax += p_view->vert_border_gdi_width + BORDER_VERT_GAP;
    else if(p_view->flags.vert_ruler_on)
        back_box.xmax += 4;

    if(p_view->flags.vert_split_on)
        back_box.ymin -= (pane_box.ymax - pane_box.ymin) + HORZ_INTERPANE_GAP;

    if(p_view->flags.horz_ruler_on)
        back_box.ymin -= p_view->horz_ruler_gdi_height + RULER_HORZ_GAP;

    if(p_view->flags.horz_border_on)
        back_box.ymin -= p_view->horz_border_gdi_height + BORDER_HORZ_GAP;
    else /* SKS notes curious asymmetry */
        back_box.ymin -= 4;

    if( (back_box.xmin != p_view->scaled_backextent.x0) || (back_box.ymin != p_view->scaled_backextent.y0) ||
        (back_box.xmax != p_view->scaled_backextent.x1) || (back_box.ymax != p_view->scaled_backextent.y1) )
    {
        void_WrapOsErrorReporting(wimp_set_extent(p_view->main[WIN_BACK].hwnd, &back_box));

        * (BBox *) &p_view->scaled_backextent = back_box;
    }
}

static void
view_point_from_screen_point_and_context(
    _OutRef_    P_VIEW_POINT p_view_point,
    _InRef_     PC_GDI_POINT p_gdi_point,
    _InRef_     PC_CLICK_CONTEXT p_click_context)
{
    GDI_POINT gdi_point = *p_gdi_point;

    gdi_point.x -= p_click_context->gdi_org.x;
    gdi_point.y -= p_click_context->gdi_org.y;

    VIEW_ASSERT(p_click_context->p_view);
    p_view_point->p_view = p_click_context->p_view;
    pixit_point_from_window_point(&p_view_point->pixit_point, &gdi_point, &p_click_context->host_xform);
}

static void
window_rect_from_view_rect(
    _OutRef_    P_GDI_RECT p_gdi_rect,
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEW_RECT p_view_rect,
    _InRef_opt_ PC_RECT_FLAGS p_rect_flags,
    _InRef_     PC_HOST_XFORM p_host_xform)
{
    PC_PIXIT_RECT p_pixit_rect = &p_view_rect->rect;
    PIXIT_RECT t_pixit_rect;

    if(NULL != p_rect_flags)
    {
        RECT_FLAGS rect_flags = *p_rect_flags;

        t_pixit_rect = *p_pixit_rect;

        PTR_ASSERT(p_docu);

      /*t_pixit_rect.tl.x -= rect_flags.extend_left_currently_unused  * p_docu->page_def.grid_size;*/
        if(rect_flags.reduce_left_by_2)
            t_pixit_rect.tl.x += 2 * p_docu->page_def.grid_size;

      /*t_pixit_rect.tl.y -= rect_flags.extend_up_currently_unused    * p_docu->page_def.grid_size;*/
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

    status_consume(window_rect_from_pixit_rect(p_gdi_rect, p_pixit_rect, p_host_xform));

    if(NULL != p_rect_flags)
    {
        p_gdi_rect->tl.x -= (GDI_COORD) p_rect_flags->extend_left_ppixels  * RISCOS_PER_PROGRAM_PIXEL_X;
        p_gdi_rect->br.y -= (GDI_COORD) p_rect_flags->extend_down_ppixels  * RISCOS_PER_PROGRAM_PIXEL_Y;
        p_gdi_rect->br.x += (GDI_COORD) p_rect_flags->extend_right_ppixels * RISCOS_PER_PROGRAM_PIXEL_X;
        p_gdi_rect->tl.y += (GDI_COORD) p_rect_flags->extend_up_ppixels    * RISCOS_PER_PROGRAM_PIXEL_Y;
    }
}

PROC_EVENT_PROTO(static, proc_event_back_window)
{
    if(object_present(OBJECT_ID_TOOLBAR))
    {
        BACK_WINDOW_EVENT back_window_event;
        back_window_event.t5_message = t5_message;
        back_window_event.p_data = p_data;
        back_window_event.processed = FALSE;
        status_return(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_BACK_WINDOW_EVENT, &back_window_event));
        if(back_window_event.processed)
            return(STATUS_OK);
    }

    return(view_event_back_window(p_docu, t5_message, p_data));
}

extern void
filer_launch(
    _In_z_      PCTSTR filename)
{
    TCHARZ cmdbuffer[BUF_MAX_PATHSTRING];

    consume_int(snprintf(cmdbuffer, elemof32(cmdbuffer), "Filer_Run %s", filename));

    report_output(cmdbuffer);
    (void) _kernel_oscli(cmdbuffer);
}

extern void
filer_opendir(
    _In_z_      PCTSTR filename)
{
    TCHARZ cmdbuffer[BUF_MAX_PATHSTRING];
    PCTSTR leafname = file_leafname(filename);

    if(leafname == filename)
        return;

    consume_int(snprintf(cmdbuffer, elemof32(cmdbuffer), "Filer_OpenDir %.*s", (leafname - filename) - 1, filename));

    /* which will pop up ca. 0.5 seconds later ... */
    report_output(cmdbuffer);
    (void) _kernel_oscli(cmdbuffer);
}

/*ncr*/
_Ret_z_
extern PTSTR
make_var_name(
    _Out_cap_(elemof_buffer) PTSTR buffer,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR suffix)
{
    tstr_xstrkpy(buffer, elemof_buffer, product_id());
    tstr_xstrkat(buffer, elemof_buffer, suffix);
    return(buffer);
}

extern void
host_initialise_file_paths(void)
{
    TCHARZ var_name[BUF_MAX_PATHSTRING];
    TCHARZ resources_path[BUF_MAX_PATHSTRING * 3];

    if(NULL == _kernel_getenv(make_var_name(var_name, elemof32(var_name), "Res$Path"), resources_path, elemof32(resources_path)))
    {
        trace_2(TRACE_APP_SKEL, "var=%s, path=%s", report_tstr(var_name), report_tstr(resources_path));
        trace_2(TRACE_OUT | TRACE_ANY, TEXT("file_path_set(%d): %s"), FILE_PATH_RESOURCES, report_tstr(resources_path));
        status_assert(file_path_set(resources_path, FILE_PATH_RESOURCES));
    }

    if(NULL == _kernel_getenv(make_var_name(var_name, elemof32(var_name), "$Path"), resources_path, elemof32(resources_path)))
    {
        trace_2(TRACE_APP_SKEL, "var=%s, path=%s", report_tstr(var_name), report_tstr(resources_path));
        trace_2(TRACE_OUT | TRACE_ANY, TEXT("file_path_set(%d): %s"), FILE_PATH_STANDARD, report_tstr(resources_path));
        status_assert(file_path_set(resources_path, FILE_PATH_STANDARD));
    }
}

_Check_return_
extern STATUS
ho_help_contents(
    _In_z_      PCTSTR filename)
{
    TCHARZ help_filename[BUF_MAX_PATHSTRING];

    reportf("ho_help_contents(%s)", filename);

    if(CH_NULL == filename[0])
    {
        tstr_xstrkpy(help_filename, elemof32(help_filename), TEXT("<"));
        tstr_xstrkat(help_filename, elemof32(help_filename), product_id());
        tstr_xstrkat(help_filename, elemof32(help_filename), TEXT("$Dir>.!Help"));
        filename = help_filename;
    }

    filer_launch(filename);
    return(STATUS_OK);
}

_Check_return_
extern STATUS
ho_help_url(
    _In_z_      PCTSTR url)
{
    STATUS status = STATUS_OK;
    char tempstr[1024];

    if( (NULL == _kernel_getenv("Alias$Open_URI_http", tempstr, elemof32(tempstr)-1)) &&
        (CH_NULL != tempstr[0]) )
    {
       _kernel_swi_regs rs;
        rs.r[0] = 0;
        if( (NULL == _kernel_swi(/*URI_Version*/ 0x4E380, &rs, &rs)) )
        {
            consume_int(snprintf(tempstr, elemof32(tempstr), "URIdispatch %s", url));
            reportf("StartTask %s", tempstr);
            void_WrapOsErrorReporting(wimp_start_task(tempstr, NULL));
            return(status);
        }
    }

    if( (NULL == _kernel_getenv("Alias$URLOpen_HTTP", tempstr, elemof32(tempstr)-1)) &&
        (CH_NULL != tempstr[0]) )
    {
        consume_int(snprintf(tempstr, elemof32(tempstr), "URLOpen_HTTP %s", url));
        reportf("StartTask %s", tempstr);
        void_WrapOsErrorReporting(wimp_start_task(tempstr, NULL));
        return(status);
    }

    return(create_error(ERR_HELP_URL_FAILURE));
}

T5_CMD_PROTO(extern, t5_cmd_help)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(t5_message)
    {
    default: default_unhandled();
    case T5_CMD_HELP_CONTENTS:
        return(ho_help_contents(p_args[0].val.tstr));

    case T5_CMD_HELP_URL:
        return(ho_help_url(p_args[0].val.tstr));
    }
}

#if defined(USE_GLOBAL_CLIPBOARD)

DOCNO  g_global_clipboard_owning_docno  = DOCNO_NONE;
VIEWNO g_global_clipboard_owning_viewno = VIEWNO_NONE;

_Check_return_
extern BOOL
host_acquire_global_clipboard(
    _DocuRef_   PC_DOCU p_docu,
    _ViewRef_   PC_VIEW p_view,
    P_PROC_GLOBAL_CLIPBOARD_DATA_DISPOSE p_proc_global_clipboard_data_dispose,
    P_PROC_GLOBAL_CLIPBOARD_DATA_DATAREQUEST p_proc_global_clipboard_data_DataRequest)
{
    const DOCNO acquiring_docno = docno_from_p_docu(p_docu);
    const VIEWNO acquiring_viewno = viewno_from_p_view_fn(p_view);

    trace_2(TRACE_RISCOS_HOST, TEXT("host_acquire_global_clipboard(docno=%d, viewno=%d)"), acquiring_docno, acquiring_viewno);

#if CHECKING
    if(!IS_VIEW_NONE(p_view))
    {
        assert(p_view->docno == acquiring_docno);
    }
#endif

    winx_claim_global_clipboard(p_proc_global_clipboard_data_dispose, p_proc_global_clipboard_data_DataRequest);

    g_global_clipboard_owning_docno  = acquiring_docno;
    g_global_clipboard_owning_viewno = acquiring_viewno;
    trace_2(TRACE_RISCOS_HOST, TEXT("host_acquire_global_clipboard: cbo docno:=%d, viewno:=%d"), g_global_clipboard_owning_docno, g_global_clipboard_owning_viewno);

    return(TRUE);
}

extern void
host_release_global_clipboard(
    _InVal_     BOOL render_if_acquired)
{
    trace_1(TRACE_RISCOS_HOST, TEXT("host_release_global_clipboard(render_if_acquired=%s)"), report_boolstring(render_if_acquired));
    trace_2(TRACE_RISCOS_HOST, TEXT("host_release_global_clipboard: cbo docno=%d, viewno=%d"), g_global_clipboard_owning_docno, g_global_clipboard_owning_viewno);

    UNREFERENCED_PARAMETER_InVal_(render_if_acquired); /* no deferred rendering for close on RISC OS */

    if(DOCNO_NONE != g_global_clipboard_owning_docno)
    {
#if 0
        P_DOCU global_clipboard_owning_p_docu = p_docu_from_docno(g_global_clipboard_owning_docno);
        P_VIEW global_clipboard_owning_p_view = p_view_from_viewno(global_clipboard_owning_p_docu, g_global_clipboard_owning_viewno);

        /* nothing to see here */
#endif
    }

    g_global_clipboard_owning_docno  = DOCNO_NONE;
    g_global_clipboard_owning_viewno = VIEWNO_NONE;
    trace_0(TRACE_RISCOS_HOST, TEXT("host_release_global_clipboard: cbo docno:=NONE, viewno:=NONE"));
}

#endif /* USE_GLOBAL_CLIPBOARD */

_Check_return_
extern U32
myrand(
    _InoutRef_  P_MYRAND_SEED p_myrand_seed,
    _InVal_     U32 n /*excl*/,
    _InVal_     U32 bias);

static MYRAND_SEED myrand_seed = {0x12345678, 1};

_Check_return_
extern U32
host_rand_between(
    _InVal_ U32 lo /*incl*/,
    _InVal_ U32 hi /*excl*/) /* NB NOT like RANDBETWEEN() */
{
#if 1
    U32 res = lo;

    if(hi > lo)
        res = myrand(&myrand_seed, hi - lo, lo);

    /*reportf("hrb[%u,%u] %u", lo, hi, res);*/
    return(res);
#else
    /* fallback implementation using standard library */
    U32 res = lo;

    if(hi > lo)
    {
        const U32 n = hi - lo;
        const U32 r = (U32) rand();
        assert(n < (U32) RAND_MAX);
        res = lo + ((r & (U32) RAND_MAX) % n);
    }

    /*reportf("hrb[%u,%u] %u", lo, hi, res);*/
    return(res);
#endif
}

#endif /* RISCOS */

/* end of riscos/host.c */
