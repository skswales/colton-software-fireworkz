/* cs-winx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"

typedef struct WINX_FIND_KEY
{
    wimp_w window_handle;
    wimp_i icon_handle;
}
WINX_FIND_KEY, * P_WINX_FIND_KEY; typedef const WINX_FIND_KEY * PC_WINX_FIND_KEY;

/*
data structure for winx-controlled windows - entry exists as long as window, even if hidden
*/

typedef struct WINX_WINDOW
{
    wimp_w window_handle;   /* NB this structure must start with same structure as WINX_FIND_KEY */
    wimp_i icon_handle;     /* icon handle iff window handle == ICONBAR_WIMP_W, else BAD_WIMP_I */

    winx_event_handler      proc;                   /* handler for known events */
    void *                  handle;

    wimp_w                  parent_window_handle;   /* parent window handle if a child */
    ARRAY_HANDLE            h_winx_children;        /* winx windows can have a number of children hanging off them */

    void *                  menuh;                  /* registered menu (if any) */

    wimp_i                  menuhi_icon_handle;     /* icon to which menu is registered */
    void *                  menuhi;                 /* registered icon menu (if any) */

    WimpWindowWithBitset    ww;                     /* store ourselves 'cos Window Manager can't return this info */
}
WINX_WINDOW, * P_WINX_WINDOW; typedef const WINX_WINDOW * PC_WINX_WINDOW;

/*
data structure for array of children hanging off a parent winx_window
*/

typedef struct WINX_CHILD_WINDOW
{
    wimp_w window_handle; /* window handle of child */

    int xoffset, yoffset; /* offsets of child x0,y1 from parent x0,y1 */

    BOOL immediate_open_request;
}
WINX_CHILD_WINDOW, * P_WINX_CHILD_WINDOW;

static struct WINX_STATICS
{
    WimpCaret deferred_caret;

    wimp_w drag_window_handle;

    BOOL dragging_a_sprite;

    ARRAY_HANDLE h_winx_windows;

    wimp_w submenu_window_handle;
    wimp_w submenu_window_handle_child;
    STATUS submenu_complex;
}
winx_statics;

static BOOL
winx_default_event_handler(
    _InVal_     int event_code,
    _Inout_     WimpPollBlock * const p_event_data);

_Check_return_
extern BOOL
winx_adjustclicked(void)
{
    WimpGetPointerInfoBlock pointer_info;

    void_WrapOsErrorReporting(wimp_get_pointer_info(&pointer_info));

    return((pointer_info.button_state & Wimp_MouseButtonAdjust) != 0);
}

extern void
winx_changedtitle(
    _InVal_     wimp_w window_handle)
{
    WimpGetWindowStateBlock window_state;

    window_state.window_handle = window_handle;
    if(WrapOsErrorReporting(wimp_get_window_state(&window_state)))
        return;

    if(WimpWindow_Open & window_state.flags)
    {
        if(g_current_wm_version >= 380)
        {   /* use more efficient method */
            void_WrapOsErrorReporting(wimp_force_redraw(window_handle, 0x4B534154 /*TASK*/, 3 /*Title Bar*/, 0, 0));
        }
        else
        {
            const int dy = host_modevar_cache_current.dy;
            BBox bbox;
            bbox.xmin = window_state.visible_area.xmin;
            bbox.ymin = window_state.visible_area.ymax + dy; /* title bar contents starts one raster up */
            bbox.xmax = window_state.visible_area.xmax;
            bbox.ymax = bbox.ymin + wimp_win_title_height(dy) - 2*dy;
            trace_1(TRACE_RISCOS_HOST, TEXT("%s: forcing global redraw of inside of title bar area"), __Tfunc__);
            void_WrapOsErrorReporting(wimp_force_redraw_BBox(-1 /* entire screen */, &bbox));
        }
    }
}

PROC_BSEARCH_PROTO(static, winx_compare, WINX_FIND_KEY, WINX_WINDOW)
{
    BSEARCH_KEY_VAR_DECL(PC_WINX_FIND_KEY, key);
    BSEARCH_DATUM_VAR_DECL(PC_WINX_WINDOW, datum);

    /* sorted first by window handle */
    if((U32) key->window_handle > (U32) datum->window_handle)
        return(+1);

    if((U32) key->window_handle < (U32) datum->window_handle)
        return(-1);

    /* then by window handle */
    if((U32) key->icon_handle   > (U32) datum->icon_handle  )
        return(+1);

    if((U32) key->icon_handle   < (U32) datum->icon_handle  )
        return(+1);

    return(0); /* both window handle and icon handle match */
}

/* look for winx window on window list */

_Check_return_
_Ret_maybenull_
static P_WINX_WINDOW
winx_find_w_i(
    _InVal_     wimp_w window_handle,
    _InVal_     wimp_i icon_handle)
{
    WINX_FIND_KEY winx_find_key;

    winx_find_key.window_handle = window_handle;
    winx_find_key.icon_handle   = icon_handle;

    return(al_array_bsearch(&winx_find_key, &winx_statics.h_winx_windows, WINX_WINDOW, winx_compare));
}

_Check_return_
static ARRAY_INDEX
winx_find_w_i_index(
    _InVal_     wimp_w window_handle,
    _InVal_     wimp_i icon_handle,
    _OutRef_    P_BOOL p_hit)
{
    WINX_FIND_KEY winx_find_key;

    winx_find_key.window_handle = window_handle;
    winx_find_key.icon_handle   = icon_handle;

    return(al_array_bfind(&winx_find_key, &winx_statics.h_winx_windows, WINX_WINDOW, winx_compare, p_hit));
}

#define winx_find_w(/*wimp_w*/ window_handle) \
    winx_find_w_i(window_handle, BAD_WIMP_I)

static void
winx_window_deregister(
    _InVal_     wimp_w window_handle,
    _InVal_     wimp_i icon_handle)
{
    WINX_FIND_KEY winx_find_key;
    P_WINX_WINDOW p_winx_window;

    winx_find_key.window_handle = window_handle;
    winx_find_key.icon_handle   = icon_handle;

    trace_2(TRACE_RISCOS_HOST, TEXT("winx_window_deregister(window ") UINTPTR_XTFMT TEXT(", icon %d): "), window_handle, icon_handle);

    if(NULL != (p_winx_window = al_array_bsearch(&winx_find_key, &winx_statics.h_winx_windows, WINX_WINDOW, winx_compare)))
    {
        ARRAY_INDEX array_index = array_indexof_element(&winx_statics.h_winx_windows, WINX_WINDOW, p_winx_window);

        myassert1x(0 == array_elements(&p_winx_window->h_winx_children), TEXT("winx_window_deregister(window ") UINTPTR_XTFMT TEXT("): still has children"), window_handle);

        trace_1(TRACE_RISCOS_HOST, TEXT("deleting p_winx_window ") PTR_XTFMT, p_winx_window);
        al_array_delete_at(&winx_statics.h_winx_windows, -1, array_index);
        if(0 == array_elements32(&winx_statics.h_winx_windows))
            al_array_dispose(&winx_statics.h_winx_windows);
    }
}

_Check_return_
static STATUS
winx_window_register(
    _InVal_     wimp_w window_handle,
    _InVal_     wimp_i icon_handle,
    winx_event_handler proc,
    void * handle,
    WimpWindowWithBitset * wwp)
{
    WINX_FIND_KEY winx_find_key;
    P_WINX_WINDOW p_winx_window;
    ARRAY_INDEX array_index;
    BOOL hit = FALSE;

    if(NULL == proc)
    {
        winx_window_deregister(window_handle, icon_handle);
        return(STATUS_OK);
    }

    trace_4(TRACE_RISCOS_HOST, TEXT("winx_window_register(window ") UINTPTR_XTFMT TEXT(", icon %d, %s, ") PTR_XTFMT TEXT("): "),
            window_handle, icon_handle, report_procedure_name(report_proc_cast(proc)), handle);

    /* search list to see if it's already there */
    winx_find_key.window_handle = window_handle;
    winx_find_key.icon_handle   = icon_handle;

    array_index = al_array_bfind(&winx_find_key, &winx_statics.h_winx_windows, WINX_WINDOW, winx_compare, &hit);

    if(hit)
    {
        p_winx_window = array_ptr(&winx_statics.h_winx_windows, WINX_WINDOW, array_index);
        trace_1(TRACE_RISCOS_HOST, TEXT("updating p_winx_window ") PTR_XTFMT, p_winx_window);
    }
    else
    {
        STATUS status;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*p_winx_window), TRUE);
        ARRAY_INDEX insert_before = array_index; /* use bfind suggested index for insert_before */
        /*trace_1(TRACE_RISCOS_HOST, "h_winx_windows: insert_before = %d, elements = %d", insert_before, array_elements(&winx_statics.h_winx_windows));*/
        if(NULL == (p_winx_window = al_array_insert_before(&winx_statics.h_winx_windows, WINX_WINDOW, 1, &array_init_block, &status, insert_before)))
            return(status);
        trace_1(TRACE_RISCOS_HOST, TEXT("creating p_winx_window ") PTR_XTFMT, p_winx_window);
    }

    p_winx_window->window_handle = window_handle;
    p_winx_window->icon_handle = icon_handle;
    p_winx_window->proc = proc;
    p_winx_window->handle = handle;
    if(wwp)
        p_winx_window->ww = *wwp;
    else
        p_winx_window->ww.work_flags.bits.button_type = ButtonType_Click /*keep cache kosher*/;

    al_array_check_sorted(&winx_statics.h_winx_windows, (P_PROC_QSORT) winx_compare);

    return(STATUS_OK);
}

_Check_return_
extern STATUS
winx_register_event_handler(
    _InVal_     wimp_w window_handle,
    _InVal_     wimp_i icon_handle,
    winx_event_handler eventproc,
    void * handle)
{
    return(winx_window_register(window_handle, icon_handle, eventproc, handle, NULL));
}

_Check_return_
extern STATUS
winx_register_general_event_handler(
    winx_event_handler eventproc,
    void * handle)
{
    return(winx_window_register(NULL_WIMP_W, BAD_WIMP_I, eventproc, handle,  NULL));
}

/*
position the caret but coping with not-yet-open windows
*/

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_get_caret_position(
    _Out_       WimpCaret * const caret)
{
    if(winx_statics.deferred_caret.window_handle)
    {
        *caret = winx_statics.deferred_caret;
        return(NULL);
    }

    return(wimp_get_caret_position(caret));
}

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_set_caret_position(
    _In_        const WimpCaret * const caret)
{
    WimpGetWindowStateBlock window_state;

    window_state.window_handle = caret->window_handle;
    if(NULL == wimp_get_window_state(&window_state))
        if(!(WimpWindow_Open & window_state.flags))
        {
            winx_statics.deferred_caret = *caret;
            return(NULL);
        }

    winx_statics.deferred_caret.window_handle = 0;

    return(wimp_set_caret_position_block(caret));
}

_Check_return_
extern wimp_w
winx_drag_active(void)
{
    return(winx_statics.drag_window_handle);
}

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_drag_box(
    _In_opt_    WimpDragBox * const dr)
{
    if(dr)
    {
        /* window handle MUST be valid, unlike what PRM doc says, due to changed event handling here */
        P_WINX_WINDOW p_winx_window = winx_find_w(dr->wimp_window);

        myassert1x(p_winx_window, TEXT("winx_drag_box for unregistered window ") UINTPTR_XTFMT, dr->wimp_window);
        if(NULL != p_winx_window)
            winx_statics.drag_window_handle = dr->wimp_window;
    }
    else
    {
        winx_statics.drag_window_handle = 0;
    }

    return(wimp_drag_box(dr));
}

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_drag_a_sprite_start(
    int flags,
    int sprite_area_id,
    char * p_sprite_name,
    _In_        WimpDragBox * const dr)
{
    /* window handle MUST be valid, unlike what PRM doc says, due to changed event handling here */
    _kernel_swi_regs rs;
    P_WINX_WINDOW p_winx_window = winx_find_w(dr->wimp_window);

    if(NULL == p_winx_window)
    {
        myassert1x(p_winx_window, TEXT("winx_drag_box for unregistered window ") UINTPTR_XTFMT, dr->wimp_window);
        return(NULL);
    }

    winx_statics.drag_window_handle = dr->wimp_window;

    rs.r[0] = flags;
    rs.r[1] = sprite_area_id;
    rs.r[2] = (int) p_sprite_name;
    rs.r[3] = (int) &dr->dragging_box;
    winx_statics.dragging_a_sprite = 1;
    return(_kernel_swi(/*DragASprite_Start*/ 0x00042400, &rs, &rs));
}

extern void
winx_drag_a_sprite_stop(void)
{
    if(winx_statics.dragging_a_sprite)
    {
        _kernel_swi_regs rs;
        winx_statics.dragging_a_sprite = 0;
        void_WrapOsErrorReporting(_kernel_swi(/*DragASprite_Stop*/ 0x00042401, &rs, &rs));
    }
}

_Check_return_
extern void *
winx_menu_get_handle(
    _InVal_     wimp_w window_handle,
    _InVal_     wimp_i icon_handle)
{
    P_WINX_WINDOW p_winx_window;

    if(icon_handle == (wimp_i) -1 /*WORKAREA*/)
    {
        p_winx_window = winx_find_w_i(window_handle, BAD_WIMP_I);

        if(NULL != p_winx_window)
            return(p_winx_window->menuh);

        return(NULL);
    }

    p_winx_window = winx_find_w_i(window_handle, (window_handle == ICONBAR_WIMP_W) ? icon_handle : BAD_WIMP_I);

    if(NULL != p_winx_window)
        if(p_winx_window->menuhi_icon_handle == icon_handle)
            return(p_winx_window->menuhi);

    return(NULL);
}

extern void
winx_menu_set_handle(
    _InVal_     wimp_w window_handle,
    _InVal_     wimp_i icon_handle,
    void * handle)
{
    P_WINX_WINDOW p_winx_window;

    if(icon_handle == (wimp_i) -1 /*WORKAREA*/)
    {
        p_winx_window = winx_find_w_i(window_handle, BAD_WIMP_I);

        if(NULL != p_winx_window)
            p_winx_window->menuh = handle;

        return;
    }

    p_winx_window = winx_find_w_i(window_handle, (window_handle == ICONBAR_WIMP_W) ? icon_handle : BAD_WIMP_I);

    if(NULL != p_winx_window)
    {
        p_winx_window->menuhi_icon_handle = icon_handle;
        p_winx_window->menuhi = handle;
    }
}

/******************************************************************************
*
* register a window as a child of another (parent) window
*
******************************************************************************/

_Check_return_
extern BOOL
winx_register_child_window(
    _InVal_     wimp_w parent_window_handle,
    _InVal_     wimp_w child_window_handle,
    _InVal_     BOOL immediate /*for open_window_request*/)
{
    P_WINX_WINDOW p_winx_window_parent = winx_find_w(parent_window_handle);
    P_WINX_WINDOW p_winx_window_child = winx_find_w(child_window_handle);
    P_WINX_CHILD_WINDOW c = NULL;
    BOOL new_entry = TRUE;
    ARRAY_INDEX array_index;

    trace_2(TRACE_RISCOS_HOST, TEXT("winx_register_child_window(child window ") UINTPTR_XTFMT TEXT(" with parent window ") UINTPTR_XTFMT TEXT(")|"), child_window_handle, parent_window_handle);

    /* one or other window not created via this module? */
    if(!p_winx_window_parent || !p_winx_window_child)
        return(FALSE);

    /* already a child of someone else? */
    if((p_winx_window_child->parent_window_handle != NULL_WIMP_W)  &&  (p_winx_window_child->parent_window_handle != parent_window_handle))
        return(FALSE);

    /* search parent's list of children to see if it's already there */
    for(array_index = 0; array_index < array_elements(&p_winx_window_parent->h_winx_children); ++array_index)
    {
        c = array_ptr(&p_winx_window_parent->h_winx_children, WINX_CHILD_WINDOW, array_index);

        if(c->window_handle == child_window_handle)
        {
            /* already registered with this window as parent, but may change offset */
            new_entry = FALSE;
            break;
        }
    }

    if(new_entry)
    {   /* insert at head of list */
        STATUS status;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*c), TRUE);
        c = al_array_insert_before(&p_winx_window_parent->h_winx_children, WINX_CHILD_WINDOW, 1, &array_init_block, &status, 0);
        status_assert(status);
        if(NULL == c)
        {
            trace_0(TRACE_RISCOS_HOST, TEXT("failed to register child"));
            return(FALSE);
        }
    }

    p_winx_window_child->parent_window_handle = parent_window_handle;

    c->window_handle = child_window_handle;

    { /* note current parent window <-> child window offset */
    WimpGetWindowStateBlock window_state;
    int parx, pary;

    window_state.window_handle = parent_window_handle;
    void_WrapOsErrorReporting(wimp_get_window_state(&window_state));
    parx = window_state.visible_area.xmin;
    pary = window_state.visible_area.ymax;

    window_state.window_handle = child_window_handle;
    void_WrapOsErrorReporting(wimp_get_window_state(&window_state));
    c->xoffset = window_state.visible_area.xmin - parx; /* add on to parent pos to recover child pos */
    c->yoffset = window_state.visible_area.ymax - pary;
    } /*block*/

    c->immediate_open_request = immediate;

    trace_2(TRACE_RISCOS_HOST, TEXT("| at offset %d,%d from parent"), c->xoffset, c->yoffset);

    return(TRUE);
}

/******************************************************************************
*
* deregister a child window from its parent window
*
******************************************************************************/

/*ncr*/
extern BOOL
winx_deregister_child_window(
    _InVal_     wimp_w window_handle)
{
    P_WINX_WINDOW p_winx_window_parent;
    P_WINX_WINDOW p_winx_window_child = winx_find_w(window_handle);
    ARRAY_INDEX array_index;

    trace_1(TRACE_RISCOS_HOST, TEXT("winx_deregister_child_window(window ") UINTPTR_XTFMT TEXT("): "), window_handle);

    if(NULL == p_winx_window_child)
        return(FALSE);

    /* not a child? */
    if(p_winx_window_child->parent_window_handle == NULL_WIMP_W)
        return(FALSE);

    p_winx_window_parent = winx_find_w(p_winx_window_child->parent_window_handle);
    if(NULL == p_winx_window_parent)
        return(FALSE);

    /* remove parent window ref from child window structure */
    p_winx_window_child->parent_window_handle = NULL_WIMP_W;

    /* remove child window from parent's list */
    for(array_index = 0; array_index < array_elements(&p_winx_window_parent->h_winx_children); ++array_index)
    {
        P_WINX_CHILD_WINDOW c = array_ptr(&p_winx_window_parent->h_winx_children, WINX_CHILD_WINDOW, array_index);

        if(c->window_handle == window_handle)
        {
            trace_1(TRACE_RISCOS_HOST, TEXT("removing child window entry ") PTR_XTFMT, c);
            al_array_delete_at(&p_winx_window_parent->h_winx_children, -1, array_index);
            return(TRUE);
        }
    }

    trace_0(TRACE_RISCOS_HOST, TEXT("failed to deregister child from parent"));

    return(FALSE);
}

static BOOL
winx_call_event_handler(
    _InVal_     int event_code,
    _In_        WimpPollBlock * const p_event_data,
    _In_        wimp_w window_handle,
    _In_        wimp_i icon_handle)
{
    for(;;)
    {
        ARRAY_INDEX array_index;
        BOOL hit;
        P_WINX_WINDOW p;
        wimp_w parent_window_handle = NULL_WIMP_W;
        BOOL processed;

        array_index = winx_find_w_i_index(window_handle, icon_handle, &hit); /* find handler for specific combination of w,i */

short_circuit_retry:; /* from below, with same window_handle but now with BAD_WIMP_I */

        if(hit)
        {
            p = array_ptr(&winx_statics.h_winx_windows, WINX_WINDOW, array_index);

            parent_window_handle = p->parent_window_handle; /* quick stash in case we disappear */

            processed = (* p->proc) (event_code, p_event_data, p->handle);

            if(processed)
                return(TRUE);
        }

        switch(event_code)
        {
        case Wimp_ENull: /* ones with no window at all */
        case Wimp_EPollWordNonZero:
            break;

        case Wimp_ERedrawWindow: /* these can never be directed at an icon, and can't sensibly be passed to a parent window */
        case Wimp_EOpenWindow:
        case Wimp_ECloseWindow:
        case Wimp_EPointerLeavingWindow:
        case Wimp_EPointerEnteringWindow:
            break;

        default:
#if CHECKING
        case Wimp_EMouseClick:
        case Wimp_EUserDrag:
        case Wimp_EKeyPressed:
        case Wimp_ELoseCaret:
        case Wimp_EGainCaret:
        case Wimp_EUserMessage:
        case Wimp_EUserMessageRecorded:
        case Wimp_EUserMessageAcknowledge:
#endif
            if(BAD_WIMP_I != icon_handle)
            {
                if((NULL_WIMP_W != window_handle) && ((U32) window_handle < (U32) -3))
                {
                    /* try again for this real window with no icon this time */
                    icon_handle = BAD_WIMP_I;

                    if(array_index_is_valid(&winx_statics.h_winx_windows, array_index))
                    {   /* short-circuit: have a quick peek at where we stopped and see if it is the match we're about to test */
                        p = array_ptr_no_checks(&winx_statics.h_winx_windows, WINX_WINDOW, array_index);

                        if( (p->window_handle == window_handle) &&
                            (p->icon_handle   == icon_handle)   )
                        {
                            hit = TRUE;
                            goto short_circuit_retry;
                        }
                    }

                    continue;
                }

                break;
            }

            if(NULL_WIMP_W != parent_window_handle)
            {
                /* pass event on to parent window (no icon) for processing (and so on, until there are no more) */
                window_handle = parent_window_handle;
                icon_handle = BAD_WIMP_I;
                continue;
            }

            break;
        }

        return(winx_default_event_handler(event_code, p_event_data));
    }
}

/* called when no handler has processed an event */

static BOOL
winx_default_event_handler(
    _InVal_     int event_code,
    _Inout_     WimpPollBlock * const p_event_data)
{
    trace_2(TRACE_RISCOS_HOST, TEXT("%s: %s"), __Tfunc__, report_wimp_event(event_code, p_event_data));

    switch(event_code)
    {
    case Wimp_EOpenWindow:
        trace_2(TRACE_RISCOS_HOST, TEXT("%s: doing open for window ") UINTPTR_XTFMT, __Tfunc__, p_event_data->open_window_request.window_handle);
        void_WrapOsErrorReporting(winx_open_window(&p_event_data->open_window_request));
        break;

    case Wimp_ECloseWindow:
        {
        wimp_w window_handle = p_event_data->close_window_request.window_handle;
        trace_2(TRACE_RISCOS_HOST, TEXT("%s: doing disposing close for window ") UINTPTR_XTFMT, __Tfunc__, p_event_data->close_window_request.window_handle);
        void_WrapOsErrorReporting(winx_dispose_window(&window_handle));
        break;
        }

    case Wimp_ERedrawWindow:
        {
        WimpRedrawWindowBlock r;
        int wimp_more = 0;
        trace_2(TRACE_RISCOS_HOST, TEXT("%s: doing redraw for window ") UINTPTR_XTFMT, __Tfunc__, p_event_data->redraw_window_request.window_handle);
        r.window_handle = p_event_data->redraw_window_request.window_handle;
        if(!WrapOsErrorReporting(wimp_redraw_window(&r, &wimp_more)))
        {
            while(0 != wimp_more)
            {
                /* nothing to actually paint here */
                if(WrapOsErrorReporting(wimp_get_rectangle(&r, &wimp_more)))
                    wimp_more = 0;
            }
        }
        break;
        }

    case Wimp_EKeyPressed:
        {
        _kernel_swi_regs rs;
        rs.r[0] = p_event_data->key_pressed.key_code;
        void_WrapOsErrorReporting(_kernel_swi(/*Wimp_ProcessKey*/ 0x000400DC, &rs, &rs));
        break;
        }

    default:
        break;
    }

    return(FALSE);
}

_Check_return_
extern BOOL
winx_dispatch_event(
    _InVal_     int event_code,
    _In_        WimpPollBlock * const p_event_data)
{
    wimp_w window_handle = NULL_WIMP_W;
    wimp_i icon_handle = BAD_WIMP_I;

    trace_1(TRACE_RISCOS_HOST, TEXT("winx_dispatch_event: %s"), report_wimp_event(event_code, p_event_data));

    switch(event_code)
    {
    case Wimp_ERedrawWindow:
    case Wimp_EOpenWindow:
    case Wimp_ECloseWindow:
    case Wimp_EPointerLeavingWindow:
    case Wimp_EPointerEnteringWindow:
    case Wimp_EScrollRequest:
    case Wimp_ELoseCaret:
    case Wimp_EGainCaret:
        window_handle = p_event_data->close_window_request.window_handle;
        break;

    case Wimp_EMouseClick:
        window_handle = p_event_data->mouse_click.window_handle;
        icon_handle = p_event_data->mouse_click.icon_handle;
        break;

    case Wimp_EKeyPressed:
        window_handle = p_event_data->key_pressed.caret.window_handle;
        icon_handle = p_event_data->key_pressed.caret.icon_handle;
        break;

    case Wimp_EUserDrag:
        if(winx_statics.drag_window_handle)
        {
            window_handle = winx_statics.drag_window_handle;
            /* user drag comes but once per winx_drag_box */
            winx_statics.drag_window_handle = 0;
        }

        winx_drag_a_sprite_stop(); /* in case that's how it was started */
        break;

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        {
        const WimpMessage * const p_wimp_message = &p_event_data->user_message;

        switch(p_wimp_message->hdr.action_code)
        {
        case Wimp_MDataLoad:
        case Wimp_MDataSave:
            window_handle = p_wimp_message->data.data_load.destination_window;
            icon_handle = p_wimp_message->data.data_load.destination_icon;
            break;

        case Wimp_MHelpRequest:
            window_handle = p_wimp_message->data.help_request.window_handle;
            icon_handle = p_wimp_message->data.help_request.icon_handle;
            break;

        case Wimp_MWindowInfo:
            window_handle = ((const WimpWindowInfoMessage *) &p_wimp_message->data)->window_handle;
            break;

        default:
            break;
        }

        break;
        }

    default:
        break;
    }

    return(winx_call_event_handler(event_code, p_event_data, window_handle, icon_handle));
}
/*
use winx_create_window rather than wimp_create_window to get enhanced window handling
*/

_Check_return_
extern STATUS
winx_create_window(
    WimpWindowWithBitset * wwp,
    wimp_w * p_window_handle,
    winx_event_handler proc,
    void * handle)
{
    _kernel_oserror * e;
    STATUS status;

    *p_window_handle = NULL_WIMP_W;

    if(NULL != (e = wimp_create_window((WimpWindow *) wwp, p_window_handle)))
        return(create_error_from_kernel_oserror(e));

    if(status_fail(status = winx_window_register(*p_window_handle, BAD_WIMP_I, proc, handle, wwp)))
    {
        WimpDeleteWindowBlock delete_window;
        delete_window.window_handle = *p_window_handle;
        void_WrapOsErrorChecking(wimp_delete_window(&delete_window));
    }

    return(status);
}

_Check_return_
_Ret_maybenull_
extern const WimpWindowWithBitset * /* a bit low-lifetime too */
win_wimp_wind_query(
    _InVal_     wimp_w window_handle)
{
    P_WINX_WINDOW p = winx_find_w(window_handle);

    return(p ? &p->ww : NULL);
}

/******************************************************************************
*
* close (but don't delete) this window and all its children
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_close_window(
    _InVal_     wimp_w window_handle)
{
    _kernel_oserror *e = NULL;
    P_WINX_WINDOW p_winx_window = winx_find_w(window_handle);
    ARRAY_INDEX array_index;

    if(NULL == p_winx_window)
        return(NULL);

    if(winx_submenu_query_is_submenu(window_handle))
    {
        trace_1(TRACE_OUT | TRACE_RISCOS_HOST, TEXT("closing submenu window ") UINTPTR_XTFMT, window_handle);

        if(!winx_submenu_query_closed())
        {
            trace_0(TRACE_OUT | TRACE_RISCOS_HOST, TEXT(" - explicit closure of open window"));
            event_clear_current_menu();
        }
        else
            trace_0(TRACE_OUT | TRACE_RISCOS_HOST, TEXT(" - already been closed by Window Manager"));

        winx_statics.submenu_window_handle = 0;
        winx_statics.submenu_complex = 0;
    }

    if(winx_submenu_query_is_submenu_child(window_handle))
    {
        trace_1(TRACE_OUT | TRACE_RISCOS_HOST, TEXT("closing submenu child window ") UINTPTR_XTFMT, window_handle);

        if(!winx_submenu_query_closed_child())
        {
            trace_0(TRACE_OUT | TRACE_RISCOS_HOST, TEXT(" - explicit closure of open window"));
            event_clear_current_menu();
        }
        else
            trace_0(TRACE_OUT | TRACE_RISCOS_HOST, TEXT(" - already been closed by Window Manager"));

        winx_statics.submenu_window_handle_child = 0;
    }

    /* loop asking child windows to close */
    for(array_index = 0; array_index < array_elements(&p_winx_window->h_winx_children); ++array_index)
    {
        P_WINX_CHILD_WINDOW c = array_ptr(&p_winx_window->h_winx_children, WINX_CHILD_WINDOW, array_index);
        /* NB. calling this child window may eventually lead to it deregistering itself from this window */
        winx_send_close_window_request(c->window_handle, TRUE /*immediate*/);
    }

    { /* close window to Window Manager */
    WimpCloseWindowBlock close_window;
    close_window.window_handle = window_handle;
    e = wimp_close_window(&close_window.window_handle);
    } /*block*/

    return(e);
}

/******************************************************************************
*
* delete this window and all its children
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_dispose_window(
    _Inout_     wimp_w * const p_window_handle)
{
    wimp_w window_handle = *p_window_handle;
    _kernel_oserror * e;
    _kernel_oserror * e1;
    P_WINX_WINDOW p_winx_window;

    if(0 == window_handle)
        return(NULL);

    /* close internally (closes all children too) */
    e = winx_close_window(window_handle);

    /* delete children */
    p_winx_window = winx_find_w(window_handle);

    if(p_winx_window)
    {
        ARRAY_INDEX array_index;

        /* is this window a child? */
        if(p_winx_window->parent_window_handle)
            /* remove from parent window's list */
            winx_deregister_child_window(window_handle);

        /* delete all child windows of this window */
        for(array_index = 0; array_index < array_elements(&p_winx_window->h_winx_children); ++array_index)
        {
            P_WINX_CHILD_WINDOW c = array_ptr(&p_winx_window->h_winx_children, WINX_CHILD_WINDOW, array_index);
            wimp_w cw = c->window_handle;
            e1 = winx_dispose_window(&cw);
            e  = e ? e : e1;
        }
    }

    /* SKS sep92 - do after closing children as notifications may need to use handle for lookup */
    *p_window_handle = 0;

    /* remove any event handler for this window (any registered icon handlers must be done by caller) */
    winx_window_deregister(window_handle, BAD_WIMP_I);

    { /* delete window to Window Manager */
    WimpDeleteWindowBlock delete_window;
    delete_window.window_handle = window_handle;
    e1 = wimp_delete_window(&delete_window);
    e  = e ? e : e1;
    } /*block*/

#if 1
    if(e && (e->errnum == 0x288)) /* SKS suppress Illegal Window Handle from menu window closure */
        return(NULL);
#endif

    return(e);
}

/******************************************************************************
*
* open a window and its children
*
******************************************************************************/

/*
*
* ---------------------------------- all behind this w
*
*       -----
*         |
*       1.1.1
*         |
*       ----------
*            |
*            |
*            |                -----
*            |                  |
*            |                1.2.1
*            |                  |
*           1.1               ----------
*            |                    |
*            |                   1.2
*            |                    |
*       -------------------------------- c1
*                       |
*                       |
*                       |                           -----------
*                       |                                |
*                       |                               2.1
*                       1                                |
*                       |                           ------------------------ c2
*                       |                                      |
*                       |                                      2
*                       |                                      |
*       -------------------------------------------------------------------- w
*
* note that the entire child 2 stack is between the parent and child 1
* and that this applies recursively e.g. to child1.1 and child1.2 stacks
*
* recursive generation of winx_open_window() via Wimp_EOpenWindow of each child
* window is the key to this happening, either by the child window doing
* the winx_open_window itself or by allowing the default event handler to
* do it for it. note that the child windows get events to allow them to
* set extents etc. at a suitable place rather than this routine simply
* calling itself directly.
*
*/

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_open_window(
    _Inout_     WimpOpenWindowBlock * const p_open_window)
{
    WimpOpenWindowBlock open_window = *p_open_window;
    wimp_w window_handle = p_open_window->window_handle;
    BOOL has_children = FALSE;
    _kernel_oserror * e;

    trace_6(TRACE_RISCOS_HOST, TEXT("winx_open_window(window ") UINTPTR_XTFMT TEXT(" at %d,%d;%d,%d behind window ") UINTPTR_XTFMT,
            window_handle, p_open_window->visible_area.xmin, p_open_window->visible_area.ymin, p_open_window->visible_area.xmax, p_open_window->visible_area.ymax, p_open_window->behind);
 
    {
    P_WINX_WINDOW p_winx_window = winx_find_w(window_handle);
    myassert1x(p_winx_window, TEXT("winx_open_window for unregistered window ") UINTPTR_XTFMT, window_handle);
    if(NULL != p_winx_window)
        has_children = (0 != array_elements(&p_winx_window->h_winx_children));
    } /*block*/

    if(has_children)
    {   /* always open a parent window behind last pane window */
        /*trace_1(TRACE_RISCOS_HOST, TEXT(". has %d child windows"), array_elements(&winx_find_w(window_handle)->h_winx_children));*/
        winx_open_child_windows(window_handle, p_open_window, &p_open_window->behind);
    }

    /* open this window at the given position, possibly behind a different window if has_children */
    if(NULL != (e = wimp_open_window(p_open_window)))
        return(e);

    if(window_handle == winx_statics.deferred_caret.window_handle)
    {
        void_WrapOsErrorReporting(wimp_set_caret_position_block(&winx_statics.deferred_caret));
        winx_statics.deferred_caret.window_handle = 0;
    }

    if(has_children)
    {   /* reopen children with corrected coords behind the same guy as before iff needed */
        if(0 != memcmp32(&open_window.visible_area, &p_open_window->visible_area, sizeof32(open_window.visible_area)))
        {
            open_window.visible_area = p_open_window->visible_area;

            winx_open_child_windows(window_handle, &open_window, &open_window.behind);
        }
    }

    return(e);
}

/*
*
* deal with child window stack for a window that needs to
* maintain its own window stack as well
*
*/

extern void
winx_open_child_windows(
    _InVal_     wimp_w window_handle,
    _In_        const WimpOpenWindowBlock * const p_open_window,
    _Out_       wimp_w * const p_new_behind)
{
    wimp_w behind_window_handle = p_open_window->behind;
    P_WINX_WINDOW p_winx_window = winx_find_w(window_handle);
    ARRAY_INDEX array_index;

    trace_6(TRACE_RISCOS_HOST, TEXT("winx_open_child_windows(window ") UINTPTR_XTFMT TEXT(" at %d,%d;%d,%d behind window ") UINTPTR_XTFMT,
            window_handle, p_open_window->visible_area.xmin, p_open_window->visible_area.ymin, p_open_window->visible_area.xmax, p_open_window->visible_area.ymax, behind_window_handle);

    if((NULL != p_winx_window) && (0 != array_elements(&p_winx_window->h_winx_children)))
    {
        if(behind_window_handle != (wimp_w) -2)
        {
            /* not sending to back: bring stack forwards first to minimize redraws */

            /* on mode change, Window Manager tells us to open windows behind themselves! */
            if(behind_window_handle == window_handle)
            {
                /* see which window the pane stack is really behind AT THIS LEVEL (head of list of children is at front) */
                P_WINX_CHILD_WINDOW c = array_base(&p_winx_window->h_winx_children, WINX_CHILD_WINDOW);
                WimpGetWindowStateBlock window_state;
                window_state.window_handle = c->window_handle;
                void_WrapOsErrorReporting(wimp_get_window_state(&window_state));
                trace_2(TRACE_RISCOS_HOST, TEXT("pane stack of window ") UINTPTR_XTFMT TEXT(" is behind window ") UINTPTR_XTFMT, window_handle, window_state.behind);
                behind_window_handle = window_state.behind;
            }

            /* loop asking child windows to open at new positions iff needed */
            for(array_index = 0; array_index < array_elements(&p_winx_window->h_winx_children); ++array_index)
            {
                P_WINX_CHILD_WINDOW c = array_ptr(&p_winx_window->h_winx_children, WINX_CHILD_WINDOW, array_index);
                WimpPollBlock event_data;
                STATUS send_event;

                {
                union wimp_window_state_open_window_u window_u;

                /* need to preserve size and scrolls */
                window_u.window_state.window_handle = c->window_handle;
                void_WrapOsErrorReporting(wimp_get_window_state(&window_u.window_state));

                event_data.open_window_request = window_u.open_window;

                event_data.open_window_request.visible_area.xmin = p_open_window->visible_area.xmin + c->xoffset; /* keep top left relative */
                event_data.open_window_request.visible_area.ymax = p_open_window->visible_area.ymax + c->yoffset;
                event_data.open_window_request.visible_area.xmax = event_data.open_window_request.visible_area.xmin + BBox_width(&window_u.open_window.visible_area);
                event_data.open_window_request.visible_area.ymin = event_data.open_window_request.visible_area.ymax - BBox_height(&window_u.open_window.visible_area);

                event_data.open_window_request.behind = behind_window_handle;

                send_event =
                    (!(WimpWindow_Open & window_u.window_state.flags)) ||
                    (0 != memcmp32(&event_data.open_window_request, &window_u.open_window, sizeof32(event_data.open_window_request)));
                } /*block*/

                behind_window_handle = c->window_handle; /* open next child window behind this one */

                if(send_event)
                {
                    if(c->immediate_open_request)
                        consume(BOOL, winx_dispatch_event(Wimp_EOpenWindow, &event_data));
                    else
                        winx_send_open_window_request(c->window_handle, FALSE, &event_data.open_window_request); /* send via Window Manager queue */
                }
            }

            /* always open parent window (and its own managed stack) behind last pane window */
        }
    }

    *p_new_behind = behind_window_handle;
}

/***********************************************************
*
* SKS centralise submenu window handling code
*
***********************************************************/

static void
win_close_submenu(
    _Inout_     wimp_w * const p_submenu_window_handle)
{
    if(!*p_submenu_window_handle)
        return;

    /* send him a close request */
    trace_1(TRACE_OUT | TRACE_ANY, TEXT("win_close_submenu: send Wimp_ECloseWindow to window ") UINTPTR_XTFMT, *p_submenu_window_handle);

    winx_send_close_window_request(*p_submenu_window_handle, TRUE /*immediate*/);

    /* kill the bugger anyhow */
    if(*p_submenu_window_handle)
    {
        trace_1(TRACE_OUT | TRACE_ANY, TEXT("*p_submenu_w after Wimp_ECloseWindow is still set to ") UINTPTR_XTFMT TEXT(" - closing"), *p_submenu_window_handle);
        void_WrapOsErrorChecking(winx_close_window(*p_submenu_window_handle));
        *p_submenu_window_handle = 0;
    }
}

_Check_return_
static BOOL
win_is_child_of(
    _InVal_     wimp_w root_window_handle,
    _InVal_     wimp_w window_handle)
{
    P_WINX_WINDOW p_winx_window = winx_find_w(root_window_handle);
    ARRAY_INDEX array_index;

    if(NULL == p_winx_window)
        return(FALSE);

    for(array_index = 0; array_index < array_elements(&p_winx_window->h_winx_children); ++array_index)
    {
        P_WINX_CHILD_WINDOW c = array_ptr(&p_winx_window->h_winx_children, WINX_CHILD_WINDOW, array_index);

        if(c->window_handle == window_handle)
            return(TRUE);

        if(win_is_child_of(c->window_handle, window_handle))
            return(TRUE);
    }

    return(FALSE);
}

_Check_return_
extern BOOL
winx_menu_process(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data)
{
    STATUS close_up_shop;

    if(!winx_statics.submenu_window_handle)
        return(0);

    if(event_code == Wimp_ERedrawWindow)
        /* note that the Window Manager cannot cope with any window opening etc. issued
         * between the receipt of the Wimp_ERedrawWindow and the SWI Wimp_RedrawWindow
        */
        return(0);

    close_up_shop = winx_submenu_query_closed();

#if TRACE_ALLOWED
    if(close_up_shop)
        trace_1(TRACE_OUT | TRACE_ANY, TEXT("Window Manager must have gone and closed window ") UINTPTR_XTFMT TEXT(" the sly rat, so send our client a close request NOW"), winx_statics.submenu_window_handle);
#endif

    /* if it is a complex menu then we must help out */
    if(!close_up_shop && winx_statics.submenu_complex)
    {
        switch(event_code)
        {
        case Wimp_ECloseWindow:
            /* only closure of windows within submenu tree allowed */
            if(p_event_data->close_window_request.window_handle != winx_statics.submenu_window_handle)
                /* scan its children, if any */
                if(!win_is_child_of(winx_statics.submenu_window_handle, p_event_data->close_window_request.window_handle))
                {
                    close_up_shop = 1;
                    trace_2(TRACE_OUT | TRACE_ANY, TEXT("Close of window ") UINTPTR_XTFMT TEXT(" which is not a child of ") UINTPTR_XTFMT TEXT(", so send our client a close request NOW"), p_event_data->close_window_request.window_handle, winx_statics.submenu_window_handle);
                }
            break;

        case Wimp_EMouseClick:
            /* only clicks within submenu tree are allowed */
            if(p_event_data->mouse_click.window_handle != winx_statics.submenu_window_handle)
                /* scan its children, if any */
                if(!win_is_child_of(winx_statics.submenu_window_handle, p_event_data->mouse_click.window_handle))
                {
                    close_up_shop = 1;
                    trace_2(TRACE_OUT | TRACE_ANY, TEXT("Click in window ") UINTPTR_XTFMT TEXT(" which is not a child of ") UINTPTR_XTFMT TEXT(", so send our client a close request NOW"), p_event_data->mouse_click.window_handle, winx_statics.submenu_window_handle);
                }
            break;

        case Wimp_EUserMessage:
        case Wimp_EUserMessageRecorded:
            {
            switch(p_event_data->user_message.hdr.action_code)
            {
            case Wimp_MDataOpen:
                {
                const WimpDataLoadMessage * const p_wimp_message_data_load = &p_event_data->user_message.data.data_load;
                const T5_FILETYPE t5_filetype = (T5_FILETYPE) p_wimp_message_data_load->file_type;

                switch(t5_filetype)
                {
                case FILETYPE_TEXT: /* SKS 08may95 allows me to look at source files without closing t5 dialogs! */
                case FILETYPE_DIRECTORY:
                case FILETYPE_APPLICATION:
                    break;

                default:
                    close_up_shop = 1;
                    trace_0(TRACE_OUT | TRACE_ANY, TEXT("Wimp_MDataOpen event so send our client a close request NOW"));
                    break;
                }

                break;
                }

            case Wimp_MDataLoad:
                {
                close_up_shop = 1;
                trace_0(TRACE_OUT | TRACE_ANY, TEXT("Wimp_MDataLoad event so send our client a close request NOW"));
                break;
                }

            default:
                break;
            }

            break;
            }

        default:
            break;
        }
    }

    if(close_up_shop)
    {
        wimpt_fake_event(event_code, p_event_data); /* stuff received event back in the queue */

        win_close_submenu(&winx_statics.submenu_window_handle);

        return(1);
    }

    if(!winx_statics.submenu_window_handle_child)
        return(0);

    close_up_shop = winx_submenu_query_closed_child();

#if TRACE_ALLOWED
    if(close_up_shop)
        trace_1(TRACE_OUT | TRACE_ANY, TEXT("Window Manager must have gone and closed child window ") UINTPTR_XTFMT TEXT(" the sly rat, so send our client a close request NOW"), winx_statics.submenu_window_handle_child);
#endif

    if(close_up_shop)
    {
        wimpt_fake_event(event_code, p_event_data); /* stuff received event back in the queue */

        win_close_submenu(&winx_statics.submenu_window_handle_child);

        return(1);
    }

    return(0);
}

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_create_submenu_child(
    _InVal_     wimp_w window_handle,
    _InVal_     int x,
    _InVal_     int y)
{
    _kernel_oserror * err;

    assert(winx_statics.submenu_complex);
    assert(winx_statics.submenu_window_handle);

    /* do we have a different menu tree window up already? if so, close it */
    if(winx_statics.submenu_window_handle_child  &&  (winx_statics.submenu_window_handle_child != window_handle))
    {
        assert0(); /* should always have closed any existing menu windows before this happens! */
        win_close_submenu(&winx_statics.submenu_window_handle_child);
    }

    err = event_create_menu((WimpMenu *) window_handle, x, y); /* yes, a menu, else get whinge about submenus requiring a parent tree */

    if(NULL == err)
    {
        winx_statics.submenu_window_handle_child = window_handle; /* there can be only one */
        trace_2(TRACE_OUT | TRACE_ANY, TEXT("winx_create_submenu_child(window ") UINTPTR_XTFMT TEXT(") set as submenu of window ") UINTPTR_XTFMT, winx_statics.submenu_window_handle_child, winx_statics.submenu_window_handle);
    }
    else
        winx_statics.submenu_window_handle_child = 0;

    return(err);
}

/* open a window as the submenu window.
 * any prior submenu windows are sent a close event.
*/

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_create_submenu(
    _InVal_     wimp_w window_handle,
    _InVal_     int x,
    _InVal_     int y)
{
    _kernel_oserror * err;

    /* do we have a different menu tree window up already? if so, close it */
    if(winx_statics.submenu_window_handle   &&  (winx_statics.submenu_window_handle != window_handle))
    {
        assert0(); /* should always have closed any existing menu windows before this happens! */
        win_close_submenu(&winx_statics.submenu_window_handle);
    }

    err = event_create_submenu((WimpMenu *) window_handle, x, y);

    if(NULL == err)
    {
        winx_statics.submenu_window_handle = window_handle; /* there can be only one */
        trace_1(TRACE_OUT | TRACE_ANY, TEXT("winx_create_submenu(window ") UINTPTR_XTFMT TEXT(") set as submenu"), winx_statics.submenu_window_handle);
    }
    else
        winx_statics.submenu_window_handle = 0;

    winx_statics.submenu_complex = 0;

    return(err);
}

/* only needed 'cos Window Manager is
 * too stupid to realise that submenu creation
 * without a parent menu should be valid
*/

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_create_menu(
    _InVal_     wimp_w window_handle,
    _InVal_     int x,
    _InVal_     int y)
{
    _kernel_oserror *  err;

    /* do we have a different menu tree window up already? if so, close it */
    if(winx_statics.submenu_window_handle  &&  (winx_statics.submenu_window_handle != window_handle))
        win_close_submenu(&winx_statics.submenu_window_handle);

    err = event_create_menu((WimpMenu *) window_handle, x, y);

    if(NULL == err)
    {
        winx_statics.submenu_window_handle = window_handle; /* there can be only one */
        trace_1(TRACE_OUT | TRACE_ANY, TEXT("winx_create_menu(window ") UINTPTR_XTFMT TEXT(") set as menu"), winx_statics.submenu_window_handle);
    }
    else
        winx_statics.submenu_window_handle = 0;

    winx_statics.submenu_complex = 0;

    return(err);
}

/* SKS created to handle those windows which are not 'true' Window Manager
 * menu windows e.g. apply style dialog which has a list box hanging off it
*/

extern void
winx_create_complex_menu(
    _InVal_     wimp_w window_handle)
{
    /* do we have a different menu tree window up already? if so, close it */
    if(winx_statics.submenu_window_handle  &&  (winx_statics.submenu_window_handle != window_handle))
        win_close_submenu(&winx_statics.submenu_window_handle);

    winx_statics.submenu_window_handle = window_handle; /* there can be only one */
    winx_statics.submenu_complex = 1;

    trace_1(TRACE_OUT | TRACE_ANY, TEXT("winx_create_complex_menu(window ") UINTPTR_XTFMT TEXT(") set as complex submenu"), winx_statics.submenu_window_handle);
}

_Check_return_
extern BOOL
winx_submenu_query_is_submenu(
    _InVal_     wimp_w window_handle)
{
    return(window_handle ? (window_handle == winx_statics.submenu_window_handle) : 0);
}

_Check_return_
extern BOOL
winx_submenu_query_is_submenu_child(
    _InVal_     wimp_w window_handle)
{
    return(window_handle ? (window_handle == winx_statics.submenu_window_handle_child) : 0);
}

_Check_return_
extern BOOL
winx_submenu_query_closed(void)
{
    if(winx_statics.submenu_window_handle)
    {
        WimpGetWindowStateBlock window_state;
        window_state.window_handle = winx_statics.submenu_window_handle;
        if(!WrapOsErrorReporting(wimp_get_window_state(&window_state)))
            return(!(WimpWindow_Open & window_state.flags));
    }

    /* there is none, you fool */
    return(1);
}

_Check_return_
extern BOOL
winx_submenu_query_closed_child(void)
{
    if(winx_statics.submenu_window_handle_child)
    {
        WimpGetWindowStateBlock window_state;
        window_state.window_handle = winx_statics.submenu_window_handle_child;
        if(!WrapOsErrorReporting(wimp_get_window_state(&window_state)))
            return(!(WimpWindow_Open & window_state.flags));
    }

    /* there is none, you fool */
    return(1);
}

/******************************************************************************
*
* send (via WM queue) a close request to this window
*                             ~~~~~~~
******************************************************************************/

extern void
winx_send_close_window_request(
    _InVal_     wimp_w window_handle,
    _InVal_     BOOL immediate)
{
    WimpPollBlock event_data;

    trace_1(TRACE_RISCOS_HOST, TEXT("winx_send_close_window_request(window ") UINTPTR_XTFMT TEXT(")"), window_handle);

    event_data.close_window_request.window_handle = window_handle;

    if(!immediate)
    {
        void_WrapOsErrorReporting(wimp_send_message(Wimp_ECloseWindow, &event_data, window_handle, BAD_WIMP_I, NULL));
        return;
    }

    consume(BOOL, winx_dispatch_event(Wimp_ECloseWindow, &event_data));
}

/******************************************************************************
*
* send (either via WM queue, or immediate fake)
* an open request to this window
*         ~~~~~~~
******************************************************************************/

extern void
winx_send_open_window_request(
    _InVal_     wimp_w window_handle,
    _InVal_     BOOL immediate,
    _In_        const WimpOpenWindowBlock * const p_open_window)
{
    WimpPollBlock event_data;

    trace_2(TRACE_RISCOS_HOST, TEXT("winx_send_open_window_request(window ") UINTPTR_XTFMT TEXT(", immediate = %s)"), window_handle, report_boolstring(immediate));

    event_data.open_window_request = *p_open_window;
    event_data.open_window_request.window_handle = window_handle;

    if(!immediate)
    {
        void_WrapOsErrorReporting(wimp_send_message(Wimp_EOpenWindow, &event_data, window_handle, BAD_WIMP_I, NULL));
        return;
    }

    consume(BOOL, winx_dispatch_event(Wimp_EOpenWindow, &event_data));
}

/******************************************************************************
*
* send (either via WM queue, or immediate fake)
* an open at front request to this window
*                  ~~~~~~~
******************************************************************************/

extern void
winx_send_front_window_request(
    _InVal_     wimp_w window_handle,
    _InVal_     BOOL immediate,
    _InRef_opt_ PC_BBox p_bbox)
{
    union wimp_window_state_open_window_u window_u;

    trace_2(TRACE_RISCOS_HOST, TEXT("winx_send_front_window_request(window ") UINTPTR_XTFMT TEXT(", immediate = %s)"), window_handle, report_boolstring(immediate));

    /* get current scroll offsets */
    window_u.window_state.window_handle = window_handle;
    void_WrapOsErrorReporting(wimp_get_window_state(&window_u.window_state));

    window_u.open_window.behind = (wimp_w) -1; /* force to the top of the window stack */
    if(NULL != p_bbox)
        window_u.open_window.visible_area = *p_bbox;
    winx_send_open_window_request(window_handle, immediate, &window_u.open_window);
}

#endif /* RISCOS */

/* end of cs-winx.c */
