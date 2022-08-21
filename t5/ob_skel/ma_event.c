/* ma_event.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/******************************************************************************
*
* master event handler for documents
*
* all those interested in receiving document level events register their dependency here
*
* MRJC Election day April 1992
*
******************************************************************************/

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
master event list
*/

typedef struct MAEVE_ENTRY
{
    P_PROC_MAEVE_EVENT p_proc_maeve_event;
    MAEVE_HANDLE maeve_handle;
    CLIENT_HANDLE client_handle;
    U8 is_deleted;
    U8 _spare[3];
}
MAEVE_ENTRY, * P_MAEVE_ENTRY; typedef const MAEVE_ENTRY * PC_MAEVE_ENTRY;

static MAEVE_HANDLE next_maeve_handle = 1001;                   /* our next handle */

/*
master service event list
*/

typedef struct MAEVE_SERVICES_ENTRY
{
    P_PROC_MAEVE_SERVICES_EVENT p_proc_maeve_services_event;
    MAEVE_SERVICES_HANDLE maeve_services_handle;
    U8 is_deleted;
    U8 _spare[3];
}
MAEVE_SERVICES_ENTRY, * P_MAEVE_SERVICES_ENTRY; typedef const MAEVE_SERVICES_ENTRY * PC_MAEVE_SERVICES_ENTRY;

static ARRAY_HANDLE h_maeve_services_table = 0;                 /* handle to global master services event table */
static MAEVE_SERVICES_HANDLE next_maeve_services_handle = 1;    /* our next handle */

/******************************************************************************
*
* add an interested party to the list of event handlers
*
******************************************************************************/

_Check_return_
extern STATUS /* MAEVE_HANDLE out */
maeve_event_handler_add(
    _DocuRef_   P_DOCU p_docu /* must pertain to a valid document */,
    _InRef_     P_PROC_MAEVE_EVENT p_proc_maeve_event /* event routine */,
    _InVal_     CLIENT_HANDLE client_handle /* clients handle in */)
{
    P_MAEVE_ENTRY p_maeve_entry;
    ARRAY_INDEX maeve_idx;
    STATUS status;

    assert(DOCNO_NONE != docno_from_p_docu(p_docu));

    /* look for a spare entry */
    p_maeve_entry = NULL;

    for(maeve_idx = 0; maeve_idx < array_elements(&p_docu->h_maeve_table); ++maeve_idx)
    {
        const P_MAEVE_ENTRY p_maeve_entry_i = array_ptr(&p_docu->h_maeve_table, MAEVE_ENTRY, maeve_idx);

        if(p_maeve_entry_i->is_deleted)
        {
            p_maeve_entry = p_maeve_entry_i;
            p_maeve_entry->is_deleted = 0;
            break;
        }
    }

    if(NULL == p_maeve_entry)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(MAEVE_ENTRY), TRUE);

        if(NULL == (p_maeve_entry = al_array_extend_by(&p_docu->h_maeve_table, MAEVE_ENTRY, 1, &array_init_block, &status)))
            return(status);
    }

    p_maeve_entry->p_proc_maeve_event = p_proc_maeve_event;
    p_maeve_entry->maeve_handle = next_maeve_handle++;
    p_maeve_entry->client_handle = client_handle;

    trace_4(TRACE_APP_MAEVE, TEXT("maeve_event_handler_add(docno=") DOCNO_TFMT TEXT(", %s, client_handle=") UINTPTR_XTFMT TEXT("): got maeve_handle ") STATUS_TFMT, p_docu->docno, report_procedure_name(report_proc_cast(p_proc_maeve_event)), client_handle, p_maeve_entry->maeve_handle);
    return(p_maeve_entry->maeve_handle);
}

/******************************************************************************
*
* delete an event handler
*
******************************************************************************/

extern void
maeve_event_handler_del(
    _DocuRef_   P_DOCU p_docu /* must pertain to a valid document */,
    _InRef_     P_PROC_MAEVE_EVENT p_proc_maeve_event /* event routine */,
    _InVal_     CLIENT_HANDLE client_handle /* clients handle in */)
{
    ARRAY_INDEX maeve_idx;

    assert(DOCNO_NONE != docno_from_p_docu(p_docu));

    for(maeve_idx = 0; maeve_idx < array_elements(&p_docu->h_maeve_table); ++maeve_idx)
    {
        const P_MAEVE_ENTRY p_maeve_entry = array_ptr(&p_docu->h_maeve_table, MAEVE_ENTRY, maeve_idx);

        if(p_maeve_entry->is_deleted)
            continue;

        if(p_maeve_entry->p_proc_maeve_event != p_proc_maeve_event)
            continue;

        if(p_maeve_entry->client_handle != client_handle)
            continue;

        trace_4(TRACE_APP_MAEVE, TEXT("maeve_event_handler_del(docno=") DOCNO_TFMT TEXT(", %s, client_handle") UINTPTR_XTFMT TEXT("): deleted maeve_handle ") STATUS_TFMT, p_docu->docno, report_procedure_name(report_proc_cast(p_proc_maeve_event)), client_handle, p_maeve_entry->maeve_handle);
        p_maeve_entry->is_deleted = 1;
        return;
    }

    trace_3(TRACE_APP_MAEVE, TEXT("maeve_event_handler_del(docno=") DOCNO_TFMT TEXT(", %s, client_handle") UINTPTR_XTFMT TEXT("): FAILED"), p_docu->docno, report_procedure_name(report_proc_cast(p_proc_maeve_event)), client_handle);
    assert0();
}

extern void
maeve_event_handler_del_handle(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     MAEVE_HANDLE maeve_handle)
{
    ARRAY_INDEX maeve_idx;

    assert(DOCNO_NONE != docno_from_p_docu(p_docu));

    if(0 == maeve_handle)
        return;

    for(maeve_idx = 0; maeve_idx < array_elements(&p_docu->h_maeve_table); ++maeve_idx)
    {
        const P_MAEVE_ENTRY p_maeve_entry = array_ptr(&p_docu->h_maeve_table, MAEVE_ENTRY, maeve_idx);

        if(p_maeve_entry->is_deleted)
            continue;

        if(p_maeve_entry->maeve_handle != maeve_handle)
            continue;

        trace_2(TRACE_APP_MAEVE, TEXT("maeve_event_handler_del_handle(docno=") DOCNO_TFMT TEXT(", maeve_handle=%d): OK"), p_docu->docno, maeve_handle);
        p_maeve_entry->is_deleted = 1;
        return;
    }

    trace_2(TRACE_APP_MAEVE, TEXT("maeve_event_handler_del_handle(docno=") DOCNO_TFMT TEXT(", maeve_handle=%d): FAILED"), p_docu->docno, maeve_handle);
    assert0();
}

/******************************************************************************
*
* call maeve_event() to generate an event to call all hangers on
*
******************************************************************************/

_Check_return_
static STATUS
maeve_event_call_handler(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     PC_MAEVE_ENTRY p_maeve_entry)
{
    MAEVE_BLOCK maeve_block;
    STATUS status;

    maeve_block.client_handle = p_maeve_entry->client_handle;
    maeve_block.maeve_handle = p_maeve_entry->maeve_handle;
#if defined(MAEVE_BLOCK_P_DATA)
    maeve_block.mb_p_data = p_data;
#endif

    if(status_fail(status = (* p_maeve_entry->p_proc_maeve_event) (p_docu, t5_message, p_data, &maeve_block)))
    {
        myassert4(TEXT("status = ") S32_TFMT TEXT(" from handler %s,") S32_TFMT TEXT(" maeve_event(t5_message = ") S32_TFMT TEXT(")"),
                 status, report_procedure_name(report_proc_cast(p_maeve_entry->p_proc_maeve_event)), maeve_block.maeve_handle, t5_message);

#if CHECKING
        (* p_maeve_entry->p_proc_maeve_event) (P_DOCU_NONE, T5_EVENT_NONE, P_DATA_NONE, _P_DATA_NONE(PC_MAEVE_BLOCK));
#endif
    }

    return(status);
}

/* SKS 23apr95 notes that maeve_event clients are never wildcard and maeve_event(DOCNO_NONE...) is forbidden */

PROC_EVENT_PROTO(extern, maeve_event)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX maeve_idx;

    DOCU_ASSERT(p_docu);

    for(maeve_idx = 0; maeve_idx < array_elements(&p_docu->h_maeve_table); ++maeve_idx)
    {
        /* must do inside loop as client procedures may add new handlers thereby reallocing the block */
        const PC_MAEVE_ENTRY p_maeve_entry = array_ptr(&p_docu->h_maeve_table, MAEVE_ENTRY, maeve_idx);

        if(p_maeve_entry->is_deleted)
            continue;

        status_accumulate(status, maeve_event_call_handler(p_docu, t5_message, p_data, p_maeve_entry));

        if(status_ok(status))
            continue;

        /* ensure that some messages are sent to all registered handlers */
        switch(t5_message)
        {
        case T5_MSG_INITCLOSE:
            switch(((PC_MSG_INITCLOSE) p_data)->t5_msg_initclose_message)
            {
            case T5_MSG_IC__CLOSE1:
            case T5_MSG_IC__CLOSE2:
            case T5_MSG_IC__CLOSE_THUNK:
                continue;

            default:
                return(status);
            }

        default:
            return(status);
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* free maeve table when all is done
*
******************************************************************************/

extern void
maeve_event_close_thunk(
    _DocuRef_   P_DOCU p_docu)
{
    DOCU_ASSERT(p_docu);

    trace_1(TRACE_APP_MAEVE, TEXT("maeve_event_close_thunk(docno=") DOCNO_TFMT TEXT("): dispose of h_maeve_table"), p_docu->docno);

#if CHECKING
    {
    ARRAY_INDEX maeve_idx;

    for(maeve_idx = 0; maeve_idx < array_elements(&p_docu->h_maeve_table); ++maeve_idx)
    {
        /* must do inside loop as client procedures may add new handlers thereby reallocing the block */
        const PC_MAEVE_ENTRY p_maeve_entry = array_ptr(&p_docu->h_maeve_table, MAEVE_ENTRY, maeve_idx);

        if(p_maeve_entry->is_deleted)
            continue;

        assert(p_maeve_entry->is_deleted);
    }
    } /*block*/
#endif /* CHECKING */

    al_array_dispose(&p_docu->h_maeve_table);
}

/******************************************************************************
*
* add a new maeve services handler
*
******************************************************************************/

_Check_return_
extern STATUS /* MAEVE_SERVICES_HANDLE out */
maeve_services_event_handler_add(
    _InRef_     P_PROC_MAEVE_SERVICES_EVENT p_proc_maeve_services_event)
{
    P_MAEVE_SERVICES_ENTRY p_maeve_services_entry;
    ARRAY_INDEX maeve_services_idx;
    STATUS status;

    /* SKS 14jun95 always DOCNO_NONE */

    /* look for a spare entry */
    p_maeve_services_entry = NULL;

    for(maeve_services_idx = 0; maeve_services_idx < array_elements(&h_maeve_services_table); ++maeve_services_idx)
    {
        const P_MAEVE_SERVICES_ENTRY p_maeve_services_entry_i = array_ptr(&h_maeve_services_table, MAEVE_SERVICES_ENTRY, maeve_services_idx);

        if(p_maeve_services_entry_i->is_deleted)
        {
            p_maeve_services_entry = p_maeve_services_entry_i;
            p_maeve_services_entry->is_deleted = 0;
            break;
        }
    }

    if(NULL == p_maeve_services_entry)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(MAEVE_SERVICES_ENTRY), TRUE);

        if(NULL == (p_maeve_services_entry = al_array_extend_by(&h_maeve_services_table, MAEVE_SERVICES_ENTRY, 1, &array_init_block, &status)))
            return(status);
    }

    p_maeve_services_entry->p_proc_maeve_services_event = p_proc_maeve_services_event;
    p_maeve_services_entry->maeve_services_handle = next_maeve_services_handle++;

    trace_2(TRACE_APP_MAEVE, TEXT("maeve_services_event_handler_add(%s): got maeve_services_handle %d"), report_procedure_name(report_proc_cast(p_proc_maeve_services_event)), p_maeve_services_entry->maeve_services_handle);
    return(p_maeve_services_entry->maeve_services_handle);
}

/******************************************************************************
*
* delete an event handler
*
******************************************************************************/

extern void
maeve_services_event_handler_del_handle(
    _InVal_     MAEVE_SERVICES_HANDLE maeve_services_handle)
{
    ARRAY_INDEX maeve_services_idx;

    if(0 == maeve_services_handle)
        return;

    for(maeve_services_idx = 0; maeve_services_idx < array_elements(&h_maeve_services_table); ++maeve_services_idx)
    {
        const P_MAEVE_SERVICES_ENTRY p_maeve_services_entry = array_ptr(&h_maeve_services_table, MAEVE_SERVICES_ENTRY, maeve_services_idx);

        if(p_maeve_services_entry->is_deleted)
            continue;

        if(p_maeve_services_entry->maeve_services_handle != maeve_services_handle)
            continue;

        trace_1(TRACE_APP_MAEVE, TEXT("maeve_services_event_handler_del_handle(maeve_services_handle=%d): OK"), maeve_services_handle);
        p_maeve_services_entry->is_deleted = 1;
        return;
    }

    trace_1(TRACE_APP_MAEVE, TEXT("maeve_services_event_handler_del_handle(maeve_services_handle=%d): FAILED"), maeve_services_handle);
    assert0();
}

/******************************************************************************
*
* call maeve_service_event() to generate an event to call all hangers on
*
******************************************************************************/

_Check_return_
static STATUS
maeve_service_event_call_handler(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     PC_MAEVE_SERVICES_ENTRY p_maeve_services_entry)
{
    MAEVE_SERVICES_BLOCK maeve_services_block;
    STATUS status;

    maeve_services_block.maeve_services_handle = p_maeve_services_entry->maeve_services_handle;
#if defined(MAEVE_SERVICES_BLOCK_P_DATA)
    maeve_services_block.msb_p_data = p_data;
#endif

    if(status_fail(status = (* p_maeve_services_entry->p_proc_maeve_services_event) (p_docu, t5_message, p_data, &maeve_services_block)))
    {
        if(STATUS_CANCEL != status)
        {
            myassert4(TEXT("status = ") S32_TFMT TEXT(" from handler %s,") S32_TFMT TEXT(" maeve_services_event(t5_message = ") S32_TFMT TEXT(")"),
                      status, report_procedure_name(report_proc_cast(p_maeve_services_entry->p_proc_maeve_services_event)), maeve_services_block.maeve_services_handle, t5_message);

#if CHECKING
            status_consume((* p_maeve_services_entry->p_proc_maeve_services_event) (P_DOCU_NONE, T5_EVENT_NONE, P_DATA_NONE, _P_DATA_NONE(PC_MAEVE_SERVICES_BLOCK)));
#endif
        }
    }

    return(status);
}

/* SKS 15jun95 - all registered maeve_service handlers register to DOCNO_NONE but may be called for any DOCU */

PROC_EVENT_PROTO(extern, maeve_service_event)
{
    const DOCNO docno = IS_DOCU_NONE(p_docu) ? DOCNO_NONE : p_docu->docno;
    STATUS status = STATUS_OK;
    ARRAY_INDEX maeve_services_idx;

    for(maeve_services_idx = 0; maeve_services_idx < array_elements(&h_maeve_services_table); ++maeve_services_idx)
    {
        /* must do inside loop as client procedures may add new handlers thereby reallocing the block */
        const PC_MAEVE_SERVICES_ENTRY p_maeve_services_entry = array_ptr(&h_maeve_services_table, MAEVE_SERVICES_ENTRY, maeve_services_idx);

        if(p_maeve_services_entry->is_deleted)
            continue;

        status_accumulate(status, maeve_service_event_call_handler(p_docu_from_docno(docno), t5_message, p_data, p_maeve_services_entry));

        if(status_ok(status))
            continue;

        /* ensure that some messages are sent to all registered handlers */
        switch(t5_message)
        {
        case T5_MSG_INITCLOSE:
            switch(((PC_MSG_INITCLOSE) p_data)->t5_msg_initclose_message)
            {
            case T5_MSG_IC__CLOSE1:
            case T5_MSG_IC__CLOSE2:
            case T5_MSG_IC__CLOSE_THUNK:
            case T5_MSG_IC__EXIT1:
            case T5_MSG_IC__EXIT2:
                continue;

            default:
                return(status);
            }

        default:
            return(status);
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* free maeve service table when all is done
*
******************************************************************************/

extern void
maeve_service_event_exit2(void)
{
    trace_0(TRACE_APP_MAEVE, TEXT("maeve_service_event_exit2(): dispose of h_maeve_services_table"));
    al_array_dispose(&h_maeve_services_table);
}

/* end of ma_event.c */
