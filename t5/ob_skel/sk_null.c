/* sk_null.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Idle processing */

/* SKS October 1990 / December 1991 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
internal structure
*/

typedef struct NULL_FUNCLIST_ENTRY
{
    DOCNO           docno;          /* document to call */
    T5_MESSAGE      t5_message;     /* message to call with */
    P_PROC_EVENT    p_proc_event;   /* procedure to call */
    CLIENT_HANDLE   client_handle;  /* handle to call with */
}
NULL_FUNCLIST_ENTRY, * P_NULL_FUNCLIST_ENTRY;

typedef struct SCHEDULED_FUNCLIST_ENTRY
{
    DOCNO           docno;          /* document to call */
    T5_MESSAGE      t5_message;     /* message to call with */
    P_PROC_EVENT    p_proc_event;   /* procedure to call */
    CLIENT_HANDLE   client_handle;  /* handle to call with */

    MONOTIME        event_time;     /* when to call */
}
SCHEDULED_FUNCLIST_ENTRY, * P_SCHEDULED_FUNCLIST_ENTRY;

/******************************************************************************
*
* call background null procedures - simple round-robin scheduler with each
* process requested to time out after a certain period, where the scheduler
* will time out for a process if it returns having taken too long. In the
* case of any timeout, subsequent procedures will have to wait for the next
* background null event. null processors can be on the list but refuse to be
* called by saying no on their being queried.
*
* NB. background null event processors may be reentered if they do any event
* processing, so watch out!
*
* --out--
*
*   NULL_EVENT_COMPLETED
*   NULL_EVENT_TIMED_OUT
*
******************************************************************************/

/* everything we need to restart the null event list */

static
struct NULL_EVENT_STATICS
{
    MONOTIMEDIFF max_slice;

    ARRAY_HANDLE event_list;

    ARRAY_INDEX item;
}
null_;

_Check_return_
static NULL_EVENT_RETURN_CODE
do_this_null_event(
    _InVal_     ARRAY_INDEX i,
    _InVal_     MONOTIME initialTime)
{
    NULL_FUNCLIST_ENTRY null_funclist_entry = *array_ptr(&null_.event_list, NULL_FUNCLIST_ENTRY, i);
    const P_DOCU p_docu = p_docu_from_docno(null_funclist_entry.docno);
    NULL_EVENT_RETURN_CODE res = NULL_EVENT_COMPLETED;

    trace_6(TRACE__NULL, TEXT("null_events_do_events --- calling item=%d, docno=%d/") PTR_XTFMT TEXT(" message=%d ") PTR_XTFMT TEXT(" handle=") UINTPTR_TFMT,
            i, null_funclist_entry.docno, p_docu, null_funclist_entry.t5_message, null_funclist_entry.p_proc_event, null_funclist_entry.client_handle);

    /* have seen null events still going after associated document closure */
    assert((DOCNO_NONE == null_funclist_entry.docno) || !IS_DOCU_NONE(p_docu));
    if((DOCNO_NONE == null_funclist_entry.docno) || !IS_DOCU_NONE(p_docu))
    {
        NULL_EVENT_BLOCK null_event_block;

        null_event_block.rc = NULL_EVENT_COMPLETED;

        null_event_block.client_handle = null_funclist_entry.client_handle;
        null_event_block.initial_time = initialTime;
        null_event_block.max_slice = null_.max_slice;

        status_assert((* null_funclist_entry.p_proc_event) (p_docu, null_funclist_entry.t5_message, &null_event_block));

        res = null_event_block.rc;
    }

    return(res);
}

/*ncr*/
extern NULL_EVENT_RETURN_CODE
null_events_do_events(void)
{
    NULL_EVENT_RETURN_CODE res = NULL_EVENT_COMPLETED;
    S32 n = array_elements(&null_.event_list); /* do at most this many per slice; ignore list changing size during slice */
    S32 i = null_.item;
    MONOTIME initialTime;

    if(0 == n) /* need for null events may have gone away during last event processing */
        return(res);

    initialTime = monotime();

    trace_2(TRACE__NULL, TEXT("\n") TEXT("null_events_do_events --- slice started with %u null events, item=%d"),
            array_elements32(&null_.event_list), null_.item);

    for(;;)
    {
        if(n-- == 0)
            break;

        i += 1;

        if(i >= array_elements(&null_.event_list))
        {
            i = 0;

            if(!array_elements(&null_.event_list))
                break;
        }

        res = do_this_null_event(i, initialTime);

        if(res != NULL_EVENT_COMPLETED)
        {
            trace_1(TRACE__NULL, TEXT("null_events_do_events --- item=%d BAD_RES"),
                    i);
            break;
        }

        /* did this process take longer than allowed? */
        if(monotime_diff(initialTime) >= null_.max_slice)
        {
            trace_1(TRACE__NULL, TEXT("null_events_do_events --- item=%d TIMED_OUT"),
                    i);
            res = NULL_EVENT_TIMED_OUT;
            break;
        }

        trace_1(TRACE__NULL, TEXT("null_events_do_events --- item=%d COMPLETED"),
                i);
    }

    null_.item = i;

    trace_3(TRACE__NULL, TEXT("null_events_do_events --- slice ended   with %u null events, item=%d, res=") U32_XTFMT,
            array_elements32(&null_.event_list), null_.item, res);

    return(res);
}

_Check_return_
extern NULL_EVENT_RETURN_CODE
null_events_do_query(void)
{
    NULL_EVENT_RETURN_CODE res = NULL_EVENTS_OFF;
    ARRAY_INDEX i = array_elements(&null_.event_list);

    if(i != 0)
    {
        res = NULL_EVENTS_REQUIRED;
    }

    return(res);
}

_Check_return_
extern STATUS
null_events_start(
    _InVal_     DOCNO docno,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_PROC_EVENT p_proc_event,
    _InVal_     CLIENT_HANDLE client_handle)
{
    P_NULL_FUNCLIST_ENTRY p_null_funclist_entry = NULL;
    ARRAY_INDEX i;
    STATUS status = STATUS_OK;

    {
    const ARRAY_INDEX n_elements = array_elements(&null_.event_list);
    P_NULL_FUNCLIST_ENTRY t_p_null_funclist_entry = array_range(&null_.event_list, NULL_FUNCLIST_ENTRY, 0, n_elements);

    for(i = 0; i < n_elements; ++i, ++t_p_null_funclist_entry)
    {
        if( (t_p_null_funclist_entry->docno         == docno        ) &&
            (t_p_null_funclist_entry->t5_message    == t5_message   ) &&
            (t_p_null_funclist_entry->p_proc_event  == p_proc_event ) &&
            (t_p_null_funclist_entry->client_handle == client_handle) )
        {
            trace_5(TRACE__NULL, TEXT("null_events_start --- updating docno=%d message=%d ") PTR_XTFMT TEXT(" handle=") UINTPTR_TFMT TEXT(", still %u null events"),
                    docno, t5_message, p_proc_event, client_handle, array_elements32(&null_.event_list));
            p_null_funclist_entry = t_p_null_funclist_entry;
            break;
        }
    }
    } /*block*/

    if(NULL != p_null_funclist_entry)
    {
        zero_struct_ptr(p_null_funclist_entry);
    }
    else if(NULL == (p_null_funclist_entry = al_array_extend_by(&null_.event_list, NULL_FUNCLIST_ENTRY, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
    {
        return(status);
    }
    else
    {
        trace_5(TRACE__NULL, TEXT("null_events_start --- adding docno=%d message=%d ") PTR_XTFMT TEXT(" handle=") UINTPTR_TFMT TEXT(", now %u null events"),
                docno, t5_message, p_proc_event, client_handle, array_elements32(&null_.event_list));
    }

    p_null_funclist_entry->docno         = docno;
    p_null_funclist_entry->t5_message    = t5_message;
    p_null_funclist_entry->p_proc_event  = p_proc_event;
    p_null_funclist_entry->client_handle = client_handle;

    return(status);
}

extern void
null_events_stop(
    _InVal_     DOCNO docno,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_PROC_EVENT p_proc_event,
    _InVal_     CLIENT_HANDLE client_handle)
{
    ARRAY_INDEX i = array_elements(&null_.event_list);

    while(--i >= 0)
    {
        P_NULL_FUNCLIST_ENTRY p_null_funclist_entry = array_ptr(&null_.event_list, NULL_FUNCLIST_ENTRY, i);

        if( (p_null_funclist_entry->docno         == docno        ) &&
            (p_null_funclist_entry->t5_message    == t5_message   ) &&
            (p_null_funclist_entry->p_proc_event  == p_proc_event ) &&
            (p_null_funclist_entry->client_handle == client_handle))
        {
            trace_5(TRACE__NULL, TEXT("null_events_stop --- removing docno=%d message=%d ") PTR_XTFMT TEXT(" handle=") UINTPTR_TFMT TEXT(", leaving %u null events"),
                    docno, t5_message, p_proc_event, client_handle, array_elements32(&null_.event_list)-1);
            al_array_delete_at(&null_.event_list, -1, i);
            return;
        }
    }

    trace_4(TRACE__NULL, TEXT("null_events_stop --- failed to remove docno=%d message=%d ") PTR_XTFMT TEXT(" handle=") UINTPTR_TFMT,
            docno, t5_message, p_proc_event, client_handle);
}

static
struct SCHEDULED_EVENT_STATICS
{
    MONOTIMEDIFF max_slice;

    ARRAY_HANDLE event_list;
}
scheduled_;

_Check_return_
static SCHEDULED_EVENT_RETURN_CODE
do_this_scheduled_event(
    _InoutRef_  P_ARRAY_INDEX p_i,
    _InVal_     MONOTIME initialTime)
{
    SCHEDULED_FUNCLIST_ENTRY scheduled_funclist_entry = *array_ptr(&scheduled_.event_list, SCHEDULED_FUNCLIST_ENTRY, *p_i);
    P_DOCU p_docu;
    SCHEDULED_EVENT_RETURN_CODE res = SCHEDULED_EVENT_COMPLETED;

    if(monotime_difftimes(scheduled_funclist_entry.event_time, initialTime) > 0)
        return(res);

    p_docu = p_docu_from_docno(scheduled_funclist_entry.docno);

    trace_8(TRACE__SCHEDULED, TEXT("do_this_scheduled_event, index %d (@ %+d) --- removing and calling docno=%d/") PTR_XTFMT TEXT(" message=%d ") PTR_XTFMT TEXT(" handle=") UINTPTR_TFMT TEXT(", leaving %u scheduled events"),
            *p_i, monotime_difftimes(scheduled_funclist_entry.event_time, initialTime), scheduled_funclist_entry.docno, p_docu, scheduled_funclist_entry.t5_message, scheduled_funclist_entry.p_proc_event, scheduled_funclist_entry.client_handle, array_elements32(&scheduled_.event_list)-1);

    al_array_delete_at(&scheduled_.event_list, -1, *p_i); /* remove event */
    *p_i = *p_i - 1; /* use same list index again */

    /* ensure that we dont try to deliver scheduled events after associated document closure */
    DOCU_ASSERT(p_docu);
    if(!IS_DOCU_NONE(p_docu))
    {
        SCHEDULED_EVENT_BLOCK scheduled_event_block;

        scheduled_event_block.rc = SCHEDULED_EVENT_COMPLETED;

        scheduled_event_block.client_handle = scheduled_funclist_entry.client_handle;

        status_assert((* scheduled_funclist_entry.p_proc_event) (p_docu, scheduled_funclist_entry.t5_message, &scheduled_event_block));

        res = scheduled_event_block.rc;
    }

    return(res);
}

/*ncr*/
extern SCHEDULED_EVENT_RETURN_CODE
scheduled_events_do_events(void)
{
#if 1
    /* Execute all pending events in this time slot, don't defer */
    SCHEDULED_EVENT_RETURN_CODE res = SCHEDULED_EVENT_COMPLETED;
    MONOTIME initialTime;
    S32 i;

    if(0 == array_elements(&scheduled_.event_list))
        return(res);

    initialTime = monotime();

#if TRACE_ALLOWED
    {
    SCHEDULED_FUNCLIST_ENTRY scheduled_funclist_entry = *array_ptrc(&scheduled_.event_list, SCHEDULED_FUNCLIST_ENTRY, 0);
    trace_4(TRACE__NULL, TEXT("\n") TEXT("scheduled_events_do_events --- slice started at %u with %u scheduled events, first @ %u, %+d"),
            initialTime, array_elements32(&scheduled_.event_list), scheduled_funclist_entry.event_time, monotime_difftimes(scheduled_funclist_entry.event_time, initialTime));
    } /*block*/
#endif

    for(i = 0; i < array_elements(&scheduled_.event_list); i++)
    {
        res = do_this_scheduled_event(&i, initialTime); /* NB i may be modified */
    }

    trace_2(TRACE__NULL, TEXT("scheduled_events_do_events --- slice ended   at %u with %u scheduled events"),
            monotime(), array_elements32(&scheduled_.event_list));

    /* Executed all pending events in this time slot, not deferred */
    res = SCHEDULED_EVENT_COMPLETED;
#else
    SCHEDULED_EVENT_RETURN_CODE res = SCHEDULED_EVENT_COMPLETED;
    S32 n = array_elements(&scheduled_.event_list); /* do at most this many per slice; ignore list changing size during slice */
    S32 i = 0;
    MONOTIME initialTime = monotime();

    for(;;)
    {
        if(n-- == 0)
            break;

        i += 1;

        if(i >= array_elements(&scheduled_.event_list))
        {
            break;
        }

        res = do_this_scheduled_event(&i, initialTime);

        if(res != SCHEDULED_EVENT_COMPLETED)
            break;

        /* did this process take longer than allowed? */
        if(monotime_diff(initialTime) >= scheduled_.max_slice)
        {
            res = SCHEDULED_EVENT_TIMED_OUT;
            break;
        }
    }
#endif

    return(res);
}

_Check_return_
extern SCHEDULED_EVENT_RETURN_CODE
scheduled_events_do_query(void)
{
    SCHEDULED_EVENT_RETURN_CODE res = SCHEDULED_EVENTS_OFF;
    ARRAY_INDEX i = array_elements(&scheduled_.event_list);

    if(i != 0)
    {
        res = SCHEDULED_EVENTS_REQUIRED;
    }

    return(res);
}

_Check_return_
extern STATUS
scheduled_event_after(
    _InVal_     DOCNO docno,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_PROC_EVENT p_proc_event,
    _InVal_     CLIENT_HANDLE client_handle,
    _InVal_     MONOTIMEDIFF event_timediff)
{
    return(scheduled_event_at(docno, t5_message, p_proc_event, client_handle, monotime() + (MONOTIME) event_timediff));
}

_Check_return_
extern STATUS
scheduled_event_at(
    _InVal_     DOCNO docno,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_PROC_EVENT p_proc_event,
    _InVal_     CLIENT_HANDLE client_handle,
    _InVal_     MONOTIME event_time)
{
    P_SCHEDULED_FUNCLIST_ENTRY p_scheduled_funclist_entry = NULL;
    ARRAY_INDEX i;
    STATUS status = STATUS_OK;

    trace_2(TRACE_APP_WM_EVENT, TEXT("scheduled_event_at: %s,") UINTPTR_XTFMT, report_procedure_name(report_proc_cast(p_proc_event)), client_handle);

    assert(t5_message);

    {
    const ARRAY_INDEX n_elements = array_elements(&scheduled_.event_list);
    P_SCHEDULED_FUNCLIST_ENTRY t_p_scheduled_funclist_entry = array_range(&scheduled_.event_list, SCHEDULED_FUNCLIST_ENTRY, 0, n_elements);

    for(i = 0; i < n_elements; ++i, ++t_p_scheduled_funclist_entry)
    {
        if( (t_p_scheduled_funclist_entry->docno         == docno        ) &&
            (t_p_scheduled_funclist_entry->t5_message    == t5_message   ) &&
            (t_p_scheduled_funclist_entry->p_proc_event  == p_proc_event ) &&
            (t_p_scheduled_funclist_entry->client_handle == client_handle) )
        {
            p_scheduled_funclist_entry = t_p_scheduled_funclist_entry;
            trace_6(TRACE__SCHEDULED, TEXT("scheduled_event_at --- updating docno=%d message=%d ") PTR_XTFMT TEXT(" handle=") UINTPTR_TFMT TEXT(" @ ") U32_XTFMT TEXT(", still %u scheduled events"),
                    docno, t5_message, p_proc_event, client_handle, event_time, array_elements32(&scheduled_.event_list));
            break;
        }
    }
    } /*block*/

    if(NULL != p_scheduled_funclist_entry)
    {
        zero_struct_ptr(p_scheduled_funclist_entry);
    }
    else if(NULL == (p_scheduled_funclist_entry = al_array_extend_by(&scheduled_.event_list, SCHEDULED_FUNCLIST_ENTRY, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
    {
        return(status);
    }
    else
    {
        trace_6(TRACE__SCHEDULED, TEXT("scheduled_event_at --- adding docno=%d message=%d ") PTR_XTFMT TEXT(" handle=") UINTPTR_TFMT TEXT(" @ ") U32_XTFMT TEXT(", now %u scheduled events"),
                docno, t5_message, p_proc_event, client_handle, event_time, array_elements32(&scheduled_.event_list));
    }

    p_scheduled_funclist_entry->docno         = docno;
    p_scheduled_funclist_entry->t5_message    = t5_message;
    p_scheduled_funclist_entry->p_proc_event  = p_proc_event;
    p_scheduled_funclist_entry->client_handle = client_handle;

    p_scheduled_funclist_entry->event_time    = event_time;

    return(status);
}

extern void
scheduled_event_remove(
    _InVal_     DOCNO docno,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_PROC_EVENT p_proc_event,
    _InVal_     CLIENT_HANDLE client_handle)
{
    ARRAY_INDEX i = array_elements(&scheduled_.event_list);

    while(--i >= 0)
    {
        P_SCHEDULED_FUNCLIST_ENTRY p_scheduled_funclist_entry = array_ptr(&scheduled_.event_list, SCHEDULED_FUNCLIST_ENTRY, i);

        if( (p_scheduled_funclist_entry->docno         == docno        ) &&
            (p_scheduled_funclist_entry->t5_message    == t5_message   ) &&
            (p_scheduled_funclist_entry->p_proc_event  == p_proc_event ) &&
            (p_scheduled_funclist_entry->client_handle == client_handle) )
        {
            trace_5(TRACE__SCHEDULED, TEXT("scheduled_event_remove --- removing docno=%d message=%d ") PTR_XTFMT TEXT(" handle=") UINTPTR_TFMT TEXT(", leaving %u scheduled events"),
                    docno, t5_message, p_proc_event, client_handle, array_elements32(&scheduled_.event_list)-1);
            al_array_delete_at(&scheduled_.event_list, -1, i);
            return;
        }
    }

    trace_4(TRACE__SCHEDULED, TEXT("scheduled_event_remove --- failed to remove docno=%d message=%d ") PTR_XTFMT TEXT(" handle=") UINTPTR_TFMT,
            docno, t5_message, p_proc_event, client_handle);
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_null);

_Check_return_
static STATUS
sk_null_msg_startup(void)
{
    { /* preallocate the null event array; makes pretty sure a claimer won't ever fail */
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(NULL_FUNCLIST_ENTRY), 1);
    status_return(al_array_preallocate_zero(&null_.event_list, &array_init_block));
    } /*block*/

    { /* preallocate the scheduled event array; makes pretty sure a claimer won't ever fail */
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(SCHEDULED_FUNCLIST_ENTRY), 1);
    status_return(al_array_preallocate_zero(&scheduled_.event_list, &array_init_block));
    } /*block*/

#if RISCOS
    null_.max_slice = 25; /* cs */
#elif WINDOWS
    null_.max_slice = 250; /* ms */
#else
    null_.max_slice = 1; /* s */
#endif

    scheduled_.max_slice = null_.max_slice;

    return(STATUS_OK);
}

_Check_return_
static STATUS
sk_null_msg_exit2(void)
{
    al_array_dispose(&null_.event_list);

    al_array_dispose(&scheduled_.event_list);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_sk_null_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(sk_null_msg_startup());

    case T5_MSG_IC__EXIT2:
        return(sk_null_msg_exit2());

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_null)
{
    IGNOREPARM_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_null_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of sk_null.c */
