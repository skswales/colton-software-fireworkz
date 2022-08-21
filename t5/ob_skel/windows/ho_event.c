/* windows/ho_event.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* host event processing */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/collect.h"

/*
dummy structure for list
*/

typedef struct EVENTS_MODELESS_DIALOG_LISTED_DATA
{
    char foo;
}
EVENTS_MODELESS_DIALOG_LISTED_DATA;

static P_LIST_BLOCK
events__modeless_dialog_list = NULL;

#if defined(WINDOWS_ACCELERATOR_TABLE)
static HACCEL
events__accelerator_table = 0;
#endif

/******************************************************************************
*
* if any modeless dialogs are registered, we must pass any messages around
* ***ALL*** of them until one claims it or we run out of registered dialogs
* due to the fact that messages for component windows of the dialog boxes are
* sent individually to these components and not to the dialog that is their
* parent (this screwed up implementation #1)
*
******************************************************************************/

static S32
events__modeless_dialog_process(
    MSG * pMSG)
{
    const P_LIST_BLOCK p_list_block = events__modeless_dialog_list;
    LIST_ITEMNO item;
    S32 res = 0;

    if(NULL != collect_first(BYTE, p_list_block, &item))
    {
        do  {
            /* try this dialog */
            res = IsDialogMessage((HWND) (UINT_PTR) item, pMSG);

            if(res)
                break;
        }
        while(NULL != collect_next(BYTE, p_list_block, &item));
    }

    return(res);
}

_Check_return_
extern STATUS
events_modeless_dialog(
    _HwndRef_   HWND hDlg,
    _InVal_     S32 add)
{
    const P_LIST_BLOCK p_list_block = events__modeless_dialog_list;
    LIST_ITEMNO item;

    item = (LIST_ITEMNO) (UINT_PTR) hDlg;

    if(add)
    {
        EVENTS_MODELESS_DIALOG_LISTED_DATA e;
        STATUS status;

        if(NULL != collect_goto_item(EVENTS_MODELESS_DIALOG_LISTED_DATA, p_list_block, item))
            /* already registered */
            return(STATUS_OK);

        /* create object at this position  */
        zero_struct(e);
        if(NULL == collect_add_entry_elem(EVENTS_MODELESS_DIALOG_LISTED_DATA, p_list_block, &e, item, &status))
            return(status);
    }
    else
    {
        if(NULL == collect_goto_item(EVENTS_MODELESS_DIALOG_LISTED_DATA, p_list_block, item))
            /* not on list */
            return(STATUS_FAIL);

        /* delete this item */
        collect_subtract_entry(p_list_block, item);

        collect_compress(p_list_block);
    }

    return(STATUS_OK);
}

#if defined(WINDOWS_ACCELERATOR_TABLE)

extern void
events_set_accelerator(
    HACCEL hAccel)
{
    events__accelerator_table = hAccel;
}

#endif /* WINDOWS_ACCELERATOR_TABLE */

/******************************************************************************
*
* wait to see whether another input event wakes us up before a time out period
*
******************************************************************************/

static void
pause_for_thought(void)
{
    HANDLE handles[1];
    DWORD dword;
    /*reportf(TEXT("pause_for_thought"));*/
    dword =
        MsgWaitForMultipleObjects(
            0 /*nCount*/, handles /*pHandles*/,
            FALSE /*bWaitAll*/,
            10 /*dwMilliseconds*/,
            QS_ALLINPUT /*dwWakeMask*/);
    IGNOREPARM(dword);
    /*reportf(TEXT("MsgWaitForMultipleObjects returns %u"), dword);*/
}

/******************************************************************************
*
* get and process events - returning only if foreground null events are wanted.
*
* centralises null processing, modeless dialog, accelerator translation and message dispatch
*
******************************************************************************/

_Check_return_
extern WM_EVENT
wm_event_get(
    _InVal_     BOOL fgNullEventsWanted)
{
    static BOOL last_event_was_null = FALSE;

    MSG msg;
    BOOL peek, bgScheduledEventsWanted, bgNullEventsWanted, bgNullTestWanted;
    S32 res;

    /* start exit sequence; anyone interested in setting up bgNulls
     * better do their thinking in the null query handlers
    */
    bgScheduledEventsWanted = (scheduled_events_do_query() == SCHEDULED_EVENTS_REQUIRED);

    if(last_event_was_null)
    {
        /* only ask our null query handlers at the end of each real
         * requested null event whether they want to continue having nulls
         * or not; this saves wasted time querying after every single event
         * null or otherwise; so we must request a single test null event
         * to come through to query the handlers whenever some idle time turns up
        */
        bgNullEventsWanted = (null_events_do_query() == NULL_EVENTS_REQUIRED);
        trace_1(TRACE_APP_WM_EVENT, TEXT("wm_event_get: last_event_was_null: query handlers returns bgNullEventsWanted = %s"), report_boolstring(bgNullEventsWanted));
        bgNullTestWanted   = 0;
    }
    else
    {
        bgNullEventsWanted = 0;
        bgNullTestWanted   = 1 /*Null_HandlersPresent()*/;
    }

    peek = (fgNullEventsWanted || bgScheduledEventsWanted || bgNullEventsWanted || bgNullTestWanted);

    if(peek)
    {
        res = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);

        if(res < 0) res = 0; /* paranoia */

        /* res = 0 if we got a peek at a null event */
        if((0 != res) && (msg.message == WM_QUIT))
            return(WM_EVENT_EXIT);
    }
    else
    {
        res = GetMessage(&msg, 0, 0, 0);

        if(0 == res /* WM_QUIT */)
            return(WM_EVENT_EXIT);

        if(res < 0) res = 0; /* paranoia */

        assert(msg.message != WM_QUIT);
    }

    /* if we have a real event, process it and look again */
    if(res)
    {
        last_event_was_null = FALSE;

        /* check modeless dialog processing */
        if( events__modeless_dialog_list  &&
            events__modeless_dialog_process(&msg))
        {   /* modeless dialog processed */
            return(WM_EVENT_PROCESSED);
        }

#if defined(WINDOWS_ACCELERATOR_TABLE)
        /* check accelerator processing */
        if( events__accelerator_table  &&
            TranslateAccelerator(msg.hwnd, events__accelerator_table, &msg))
        {   /* accelerator processed */
            return(WM_EVENT_PROCESSED);
        }

#endif /* WINDOWS_ACCELERATOR_TABLE */

        /* normal event processing - dispatch event appropriately */
        TranslateMessage(&msg);
        trace_4(TRACE_APP_WM_EVENT, TEXT("wm_event_get: pre-dispatch message hwnd=") UINTPTR_XTFMT TEXT(", message=") U32_XTFMT TEXT(", wParam=") UINTPTR_XTFMT TEXT(", lParam=") UINTPTR_XTFMT, (uintptr_t) msg.hwnd, msg.message, msg.wParam, msg.lParam);
        DispatchMessage(&msg);
        trace_0(TRACE_APP_WM_EVENT, TEXT("wm_event_get: post-dispatch message"));
        /* normal event processed */
        return(WM_EVENT_PROCESSED);
    }

    /* only allow our processors a look in now at nulls they wanted */

    last_event_was_null = TRUE;

    /* perform the initial idle time test if some idle time has come */
    if(bgNullTestWanted)
    {
        bgNullEventsWanted = (null_events_do_query() == NULL_EVENTS_REQUIRED);
        trace_1(TRACE_APP_WM_EVENT, TEXT("wm_event_get: bgNullTestWanted: query handlers returns NULL_EVENTS_%s"), bgNullEventsWanted ? TEXT("REQUIRED") : TEXT("OFF"));
    }

    /* processor is idle, dispatch scheduled & null events appropriately */

    if(bgScheduledEventsWanted)
    {
        (void) scheduled_events_do_events();

        bgScheduledEventsWanted = (scheduled_events_do_query() == SCHEDULED_EVENTS_REQUIRED);
    }

    if(bgNullEventsWanted)
        (void) null_events_do_events();

    if(fgNullEventsWanted)
    {
        /* we have our desired fg idle, so return */
        trace_0(TRACE_APP_WM_EVENT, TEXT("wm_event_get: return WM_EVENT_FG_NULL"));
        return(WM_EVENT_FG_NULL);
    }

    if(bgScheduledEventsWanted && !(fgNullEventsWanted || bgNullEventsWanted))
        pause_for_thought();

#if 1
    return(WM_EVENT_PROCESSED);
#else
    /* check bg null requirements to loop again and again */
    if(bgNullEventsWanted || bgScheduledEventsWanted)
        return(WM_EVENT_PROCESSED);

    /* exiting strangely */
    trace_0(TRACE_APP_WM_EVENT, TEXT("wm_event_get: return WM_EVENT_EXIT"));
    return(WM_EVENT_EXIT);
#endif
}

/* end of windows/ho_event.c */
