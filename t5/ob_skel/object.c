/* object.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Object call interface for Fireworkz */

/* MRJC December 1991 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/ho_dll.h"

#include "cmodules/collect.h"

/*
internal routines
*/

_Check_return_
static STATUS
object_load_internal(
    _InVal_     OBJECT_ID object_id);

/*
array of object procedure addresses
*/

static P_PROC_OBJECT
g_proc_object[MAX_OBJECTS];

/******************************************************************************
*
* called by atexit() on exit(rc) or explicitly on Windows
*
******************************************************************************/

/* On RISC OS don't bother to release the escape & event handlers as the C runtime library
 * restores the caller's handlers prior to exiting to the calling program.
 * It also gives us a new ms more protection against SIGINT.
*/

static BITMAP(bitmap_exit_services1, MAX_OBJECTS);
static OBJECT_ID object_id_exit_services1 = OBJECT_ID_FIRST;

static BITMAP(bitmap_exit_services2, MAX_OBJECTS);
static OBJECT_ID object_id_exit_services2 = OBJECT_ID_FIRST;

#if RISCOS

static BOOL cwimp_exit_services2;

extern void
riscos_hourglass_off(void);

extern void
riscos_hourglass_on(void);

#endif /* RISCOS */

/* send the T5_MSG_IC__SERVICES_EXIT2 to all inited objects */

PROC_ATEXIT_PROTO(static, atexit_services2)
{
    MSG_INITCLOSE msg_initclose;

    while(object_id_exit_services2 < OBJECT_ID_MAX)
    {
        OBJECT_ID this_object = object_id_exit_services2; OBJECT_ID_INCR(object_id_exit_services2);

        if(bitmap_bit_test(bitmap_exit_services2, this_object, N_BITS_ARG(MAX_OBJECTS)))
        {
            bitmap_bit_clear(bitmap_exit_services2, this_object, N_BITS_ARG(MAX_OBJECTS));

            /* catch serious errors from object call, exit() will come back here, but with the next object to process */
#if RISCOS
            atexit(atexit_services2);
#endif

            trace_on();
            msg_initclose.t5_msg_initclose_message = T5_MSG_IC__SERVICES_EXIT2;
            status_assert(object_call_id(this_object, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));
            trace_off();

#if RISCOS
            return; /* back to exit(), which will call this atexit() handler again */
#endif
        }
    }

#if RISCOS
    if(cwimp_exit_services2)
    {
        cwimp_exit_services2 = 0;

        /* catch serious errors from proc, exit() will come back here, but with the next object to process */
        atexit(atexit_services2);

        riscos_hourglass_off();

        return; /* back to exit(), which will call this atexit() handler again */
    }
#endif

    resource_shutdown(); /* SKS after 1.12 28oct94 - nasty explicit tidy up */

#if RISCOS
    gr_cache_trash(); /* SKS after 1.05 24oct93 - nasty explicit tidy up */

    host_shutdown();
#endif

    alloc_block_dispose(&global_string_alloc_block);

    aligator_exit();
}

/* send the T5_IC_MSG_SERVICES_EXIT1 to all inited objects */

PROC_ATEXIT_PROTO(static, atexit_services1)
{
    MSG_INITCLOSE msg_initclose;

    while(object_id_exit_services1 < OBJECT_ID_MAX)
    {
        OBJECT_ID this_object = object_id_exit_services1; OBJECT_ID_INCR(object_id_exit_services1);

        if(bitmap_bit_test(bitmap_exit_services1, this_object, N_BITS_ARG(MAX_OBJECTS)))
        {
            bitmap_bit_clear(bitmap_exit_services1, this_object, N_BITS_ARG(MAX_OBJECTS));

            /* catch serious errors from object call, exit() will come back here, but with the next object to process */
#if RISCOS
            atexit(atexit_services1);
#endif

            trace_on();
            msg_initclose.t5_msg_initclose_message = T5_MSG_IC__SERVICES_EXIT1;
            status_assert(object_call_id(this_object, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));
            trace_off();

#if RISCOS
            return; /* not exit() 'cos we don't know the rc */
            /* back to exit(), which will call this atexit() handler again */
#endif
        }
    }
}

static BITMAP(bitmap_exit_application1, MAX_OBJECTS);
static OBJECT_ID object_id_exit_application1 = OBJECT_ID_FIRST;
static BOOL send_maeve_exit_application1;

static BITMAP(bitmap_exit_application2, MAX_OBJECTS);
static OBJECT_ID object_id_exit_application2 = OBJECT_ID_FIRST;
static BOOL send_maeve_exit_application2;

PROC_ATEXIT_PROTO(static, atexit_application2)
{
    MSG_INITCLOSE msg_initclose;

    if(send_maeve_exit_application2)
    {
        send_maeve_exit_application2 = FALSE;

        /* catch serious errors from maeve procs, exit() will come back here, but with the objects still to process */
#if RISCOS
        atexit(atexit_application2);
#endif

        trace_on();
        msg_initclose.t5_msg_initclose_message = T5_MSG_IC__EXIT2;
        status_assert(maeve_service_event(P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));
        maeve_service_event_exit2(); /* tidy up */
        trace_off();

        /* nasty explicit once offs */
#if RISCOS
        host_invalidate_cache(HIC_PURGE);
#endif
        arglist_cache_reduce();

#if RISCOS
        return; /* back to exit(), which will call this atexit() handler again */
#endif
    }

    /* send the T5_MSG_IC__EXIT2 to all inited objects */
    while(object_id_exit_application2 < OBJECT_ID_MAX)
    {
        OBJECT_ID this_object = object_id_exit_application2; OBJECT_ID_INCR(object_id_exit_application2);

        if(bitmap_bit_test(bitmap_exit_application2, this_object, N_BITS_ARG(MAX_OBJECTS)))
        {
            bitmap_bit_clear(bitmap_exit_application2, this_object, N_BITS_ARG(MAX_OBJECTS));

            /* catch serious errors from object call, exit() will come back here, but with the next object to process */
#if RISCOS
            atexit(atexit_application2);
#endif

            trace_on();
            msg_initclose.t5_msg_initclose_message = T5_MSG_IC__EXIT2;
            status_assert(object_call_id(this_object, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));
            trace_off();

#if RISCOS
            return; /* back to exit(), which will call this atexit() handler again */
#endif
        }
    }
}

PROC_ATEXIT_PROTO(static, atexit_application1)
{
    MSG_INITCLOSE msg_initclose;

    if(send_maeve_exit_application1)
    {
        send_maeve_exit_application1 = FALSE;

        /* catch serious errors from maeve, exit() will come back here, but with the objects still to process */
#if RISCOS
        atexit(atexit_application1);
#endif

        trace_on();
        msg_initclose.t5_msg_initclose_message = T5_MSG_IC__EXIT1;
        status_assert(maeve_service_event(P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));
        trace_off();

#if RISCOS
        return; /* back to exit(), which will call this atexit() handler again */
#endif
    }

    /* send the T5_MSG_IC__EXIT1 to all inited objects */
    while(object_id_exit_application1 < OBJECT_ID_MAX)
    {
        OBJECT_ID this_object = object_id_exit_application1; OBJECT_ID_INCR(object_id_exit_application1);

        if(bitmap_bit_test(bitmap_exit_application1, this_object, N_BITS_ARG(MAX_OBJECTS)))
        {
            bitmap_bit_clear(bitmap_exit_application1, this_object, N_BITS_ARG(MAX_OBJECTS));

            /* catch serious errors from object call, exit() will come back here, but with the next object to process */
#if RISCOS
            atexit(atexit_application1);
#endif

            trace_on();
            msg_initclose.t5_msg_initclose_message = T5_MSG_IC__EXIT1;
            status_assert(object_call_id(this_object, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));
            trace_off();

#if RISCOS
            return; /* back to exit(), which will call this atexit() handler again */
#endif
        }
    }
}

/******************************************************************************
*
* return the object type for a blank cell
*
******************************************************************************/

static OBJECT_ID
object_new_type(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    STYLE style;
    STYLE_SELECTOR selector;
    OBJECT_ID object_id;

    /* get new object type */
    style_selector_clear(&selector);
    style_selector_bit_set(&selector, STYLE_SW_PS_NEW_OBJECT);

    style_init(&style);
    style_from_slr(p_docu, &style, &selector, p_slr);
    object_id = OBJECT_ID_UNPACK(style.para_style.new_object);
    if(!object_present(object_id))
        object_id = OBJECT_ID_TEXT;

    return(object_id);
}

/******************************************************************************
*
* call all loaded objects
*
* stop and return error if encountered
*
******************************************************************************/

_Check_return_
extern STATUS
object_call_all(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data)
{
    OBJECT_ID object_id = OBJECT_ID_FIRST;
    STATUS status = STATUS_OK;

    while(object_id < OBJECT_ID_MAX)
    {
        if(NULL != g_proc_object[object_id])
            status = (*g_proc_object[object_id])(p_docu, t5_message, p_data);

        status_break(status);

        OBJECT_ID_INCR(object_id);
    }

    return(status);
}

/******************************************************************************
*
* call all loaded objects
*
* accumulate errors encountered
*
******************************************************************************/

_Check_return_
extern STATUS
object_call_all_accumulate(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data)
{
    OBJECT_ID object_id = OBJECT_ID_FIRST;
    STATUS status = STATUS_OK;

    while(object_id < OBJECT_ID_MAX)
    {
        if(NULL != g_proc_object[object_id])
        {
            STATUS this_status = (*g_proc_object[object_id])(p_docu, t5_message, p_data);

            status_accumulate(status, this_status);
        }

        OBJECT_ID_INCR(object_id);
    }

    return(status);
}

/******************************************************************************
*
* call a range of loaded objects
*
* stop and return error if encountered
*
* --in--
* *p_object_id gives object number to start at, OBJECT_ID_ENUM_START for beginning
* max_object_id gives object number to stop before
*
* --out--
* on error, *p_object_id contains next object to try
*
******************************************************************************/

_Check_return_
extern STATUS
object_call_between(
    _InoutRef_  P_OBJECT_ID p_object_id,
    _InVal_     OBJECT_ID max_object_id, /*excl*/
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data)
{
    STATUS status = STATUS_OK;

    assert(NULL != p_docu);

    if(!IS_OBJECT_ID_VALID(max_object_id) && (OBJECT_ID_MAX != max_object_id))
    {
        myassert4(TEXT("object_call_between(max_object_id ") S32_TFMT TEXT(" out of range, docno ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") PTR_XTFMT TEXT(")"), (S32) max_object_id, (S32) docno_from_p_docu(p_docu), t5_message, p_data);
        return(status_check());
    }

    if(OBJECT_ID_ENUM_START == *p_object_id)
        *p_object_id = OBJECT_ID_FIRST;

    while(*p_object_id < max_object_id)
    {
        if(g_proc_object[*p_object_id])
            status = (*g_proc_object[*p_object_id])(p_docu, t5_message, p_data);

        OBJECT_ID_INCR(*p_object_id);

        status_break(status);
    }

    return(status);
}

/******************************************************************************
*
* call a specific object - when its id is known
*
******************************************************************************/

_Check_return_
extern STATUS
object_call_id_reordered(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InVal_     OBJECT_ID object_id)
{
    assert(NULL != p_docu);

    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert4(TEXT("object_call_id(object_id ") S32_TFMT TEXT(" out of range, docno ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") PTR_XTFMT TEXT(")"), (S32) object_id, (S32) docno_from_p_docu(p_docu), t5_message, p_data);
        return(status_check());
    }

    if(NULL == g_proc_object[object_id])
        return(STATUS_FAIL);

    return((*g_proc_object[object_id])(p_docu, t5_message, p_data));
}

_Check_return_
extern STATUS
object_call_id_load(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InVal_     OBJECT_ID object_id)
{
    assert(NULL != p_docu);

    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert4(TEXT("object_call_id_load(object_id ") S32_TFMT TEXT(" out of range, docno ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") PTR_XTFMT TEXT(")"), (S32) object_id, (S32) docno_from_p_docu(p_docu), t5_message, p_data);
        return(status_check());
    }

    if(NULL == g_proc_object[object_id])
    {
        status_return(object_load_internal(object_id));
        assert(NULL != g_proc_object[object_id]);
    }

    return((*g_proc_object[object_id]) (p_docu, t5_message, p_data));
}

/******************************************************************************
*
* send a message to the relevant object for a cell, whether the cell is blank or not
*
******************************************************************************/

_Check_return_
extern STATUS
object_call_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     PC_SLR p_slr)
{
    const P_CELL p_cell = p_cell_from_slr(p_docu, p_slr);
    OBJECT_ID object_id;

    if(NULL != p_cell)
        object_id = object_id_from_cell(p_cell);
    else
        object_id = object_new_type(p_docu, p_slr);

    return(object_call_id(object_id, p_docu, t5_message, p_data));
}

/******************************************************************************
*
* make up some object data from a docu_area
*
******************************************************************************/

/*ncr*/
extern STATUS
object_data_from_docu_area_br(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    POSITION position;

    if(slr_last_in_docu_area(p_docu_area, &p_docu_area->tl.slr))
        position = p_docu_area->tl;
    else
    {
        position = p_docu_area->br;
        position.slr.col -= 1;
        position.slr.row -= 1;
        object_position_init(&position.object_position);
    }

    return(object_data_from_position(p_docu, p_object_data, &position, &p_docu_area->br.object_position));
}

/******************************************************************************
*
* make up some object data from a docu_area
*
******************************************************************************/

/*ncr*/
extern STATUS
object_data_from_docu_area_tl(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    PC_OBJECT_POSITION p_object_position_end;

    if(slr_last_in_docu_area(p_docu_area, &p_docu_area->tl.slr))
        p_object_position_end = &p_docu_area->br.object_position;
    else
        p_object_position_end = NULL;

    return(object_data_from_position(p_docu, p_object_data, &p_docu_area->tl, p_object_position_end));
}

/******************************************************************************
*
* make up some object data from a position
*
******************************************************************************/

/*ncr*/
extern STATUS /* STATUS_DONE == cell contains data */
object_data_from_position(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_POSITION p_position,
    _InRef_opt_ PC_OBJECT_POSITION p_object_position_end)
{
    STATUS status = object_data_from_slr(p_docu, p_object_data, &p_position->slr);

    if(p_object_data->object_id == p_position->object_position.object_id)
        p_object_data->object_position_start = p_position->object_position;

    if( (NULL != p_object_position_end) &&
        (p_object_data->object_id == p_object_position_end->object_id) )
        p_object_data->object_position_end = *p_object_position_end;

    return(status);
}

/******************************************************************************
*
* make up some object data from an slr
*
******************************************************************************/

/*ncr*/
extern STATUS /* STATUS_DONE == cell contains data */
object_data_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_SLR p_slr)
{
    STATUS status = STATUS_OK;
    P_CELL p_cell;

    data_ref_from_slr(&p_object_data->data_ref, p_slr);
    object_position_init(&p_object_data->object_position_start);
    object_position_init(&p_object_data->object_position_end);

    p_cell = p_cell_from_slr(p_docu, p_slr);

    if(NULL != p_cell)
    {
        p_object_data->object_id = object_id_from_cell(p_cell);
        p_object_data->u.p_object = &p_cell->object[0];
        status = STATUS_DONE;
    }
    else
    {
        p_object_data->object_id = object_new_type(p_docu, p_slr);
        p_object_data->u.p_object = P_DATA_NONE;
    }

    return(status);
}

/******************************************************************************
*
* initialise object data
*
******************************************************************************/

extern void
object_data_init(
    _OutRef_    P_OBJECT_DATA p_object_data)
{
    zero_struct_ptr(p_object_data);
    p_object_data->data_ref.data_space = DATA_NONE;
    p_object_data->object_id = OBJECT_ID_NONE;
    object_position_init(&p_object_data->object_position_start);
    object_position_init(&p_object_data->object_position_end);
}

/******************************************************************************
*
* install handler for object at program init time
*
******************************************************************************/

extern void
object_install(
    _InVal_     OBJECT_ID object_id,
    _InRef_     P_PROC_OBJECT p_proc_object)
{
    myassert1x(IS_OBJECT_ID_VALID(object_id), TEXT("object_install(object_id ") S32_TFMT TEXT(" out of range)"), (S32) object_id);

    if(!IS_OBJECT_ID_VALID(object_id))
        return;

    g_proc_object[object_id] = p_proc_object;
}

/******************************************************************************
*
* allocate object instance data space
*
******************************************************************************/

_Check_return_
extern STATUS
object_instance_data_alloc(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id,
    _InVal_     S32 size)
{
    STATUS status;
    P_BYTE p_data;

    DOCU_ASSERT(p_docu);

    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert1(TEXT("object_instance_data_alloc(object_id ") S32_TFMT TEXT(" out of range)"), (S32) object_id);
        return(status_check());
    }

    if(NULL != (p_data = _collect_add_entry(&p_docu->object_data_list, P_DATA_NONE, size, (LIST_ITEMNO) object_id, &status)))
        memset32(p_data, 0, size);

    return(status);
}

/******************************************************************************
*
* dispose of instance data owned by object
*
******************************************************************************/

extern void
object_instance_data_dispose(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id)
{
    DOCU_ASSERT(p_docu);

    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert1(TEXT("object_instance_data_dispose(object_id ") S32_TFMT TEXT(" out of range)"), (S32) object_id);
        return;
    }

    collect_subtract_entry(&p_docu->object_data_list, (LIST_ITEMNO) object_id);
}

/******************************************************************************
*
* Load a module from the objects directory
*
******************************************************************************/

_Check_return_
static STATUS
object_load_internal_install(
    _InVal_     OBJECT_ID object_id,
    _InRef_     P_PROC_OBJECT p_proc_object)
{
    STATUS status;
    MSG_INITCLOSE msg_initclose;

    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert1(TEXT("object_load_internal_install(object_id ") S32_TFMT TEXT(" out of range)"), (S32) object_id);
        return(status_check());
    }

    status_return(ensure_memory_froth()); /* SKS after 1.20/50 16mar95 attempt to make it not fail as much during module initialisation */

    g_proc_object[object_id] = p_proc_object;

    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__STARTUP_SERVICES;
    status_return(object_call_id(object_id, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));

    /* keep these in step during startup */
    bitmap_bit_set(bitmap_exit_services1, object_id, N_BITS_ARG(MAX_OBJECTS));
    bitmap_bit_set(bitmap_exit_services2, object_id, N_BITS_ARG(MAX_OBJECTS));

    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__STARTUP;
    if(status_ok(status = object_call_id(object_id, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose)))
    {
        /* keep these in step during startup */
        bitmap_bit_set(bitmap_exit_application1, object_id, N_BITS_ARG(MAX_OBJECTS));
        bitmap_bit_set(bitmap_exit_application2, object_id, N_BITS_ARG(MAX_OBJECTS));
    }

    if(status_ok(status))
    {
        DOCNO docno = DOCNO_NONE;

        while(DOCNO_NONE != (docno = docno_enum_thunks(docno)))
        {
            msg_initclose.t5_msg_initclose_message = T5_MSG_IC__INIT_THUNK;
            status_break(status = object_call_id(object_id, p_docu_from_docno(docno), T5_MSG_INITCLOSE, &msg_initclose));
        }
    }

    if(status_ok(status))
    {
        DOCNO docno = DOCNO_NONE;

        while(DOCNO_NONE != (docno = docno_enum_thunks(docno)))
        {
            if(!p_docu_from_docno(docno)->flags.init_close)
                continue;

            msg_initclose.t5_msg_initclose_message = T5_MSG_IC__INIT1;
            status_break(status = object_call_id(object_id, p_docu_from_docno(docno), T5_MSG_INITCLOSE, &msg_initclose));

            msg_initclose.t5_msg_initclose_message = T5_MSG_IC__INIT2;
            status_break(status = object_call_id(object_id, p_docu_from_docno(docno), T5_MSG_INITCLOSE, &msg_initclose));
        }
    }

    if(status_ok(status))
    {
        /* ensure that a module loads in its config file (if present) */
        msg_initclose.t5_msg_initclose_message = T5_MSG_IC__STARTUP_CONFIG;
        status = object_call_id(object_id, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose);

        if(status_fail(status))
        {
            TCHARZ errstring[BUF_MAX_ERRORSTRING];
            resource_lookup_tstr_buffer(errstring, elemof32(errstring), status);
            reperr(ERR_LOADING_CONFIG, product_ui_id(), errstring);
        }

        msg_initclose.t5_msg_initclose_message = T5_MSG_IC__STARTUP_CONFIG_ENDED;
        status_assert(object_call_id(object_id, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));
    }

    if(status_ok(status))
        return(status);

    /* SKS after 1.20/50 attempt better cleanup for failed modules so that we can attempt to reload them */

    /* if these go exceptional then it's down the toilet for the whole app */

    /* NB these are directed to the failed object - we aren't actually closing any docs/thunks */

    {
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_thunks(docno)))
    {
        if(!p_docu_from_docno(docno)->flags.init_close)
            continue;

        msg_initclose.t5_msg_initclose_message = T5_MSG_IC__CLOSE1;
        status_assert(object_call_id(object_id, p_docu_from_docno(docno), T5_MSG_INITCLOSE, &msg_initclose));

        msg_initclose.t5_msg_initclose_message = T5_MSG_IC__CLOSE2;
        status_assert(object_call_id(object_id, p_docu_from_docno(docno), T5_MSG_INITCLOSE, &msg_initclose));
    }
    } /*block*/

    {
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_thunks(docno)))
    {
        msg_initclose.t5_msg_initclose_message = T5_MSG_IC__CLOSE_THUNK;
        status_assert(object_call_id(object_id, p_docu_from_docno(docno), T5_MSG_INITCLOSE, &msg_initclose));
    }
    } /*block*/

    if(bitmap_bit_test(bitmap_exit_application2, object_id, N_BITS_ARG(MAX_OBJECTS)))
    {
        bitmap_bit_clear(bitmap_exit_application2, object_id, N_BITS_ARG(MAX_OBJECTS));

        trace_on();
        msg_initclose.t5_msg_initclose_message = T5_MSG_IC__EXIT2;
        status_assert(object_call_id(object_id, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));
        trace_off();
    }

    if(bitmap_bit_test(bitmap_exit_application1, object_id, N_BITS_ARG(MAX_OBJECTS)))
    {
        bitmap_bit_clear(bitmap_exit_application1, object_id, N_BITS_ARG(MAX_OBJECTS));

        trace_on();
        msg_initclose.t5_msg_initclose_message = T5_MSG_IC__EXIT1;
        status_assert(object_call_id(object_id, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));
        trace_off();
    }

    if(bitmap_bit_test(bitmap_exit_services2, object_id, N_BITS_ARG(MAX_OBJECTS)))
    {
        bitmap_bit_clear(bitmap_exit_services2, object_id, N_BITS_ARG(MAX_OBJECTS));

        trace_on();
        msg_initclose.t5_msg_initclose_message = T5_MSG_IC__SERVICES_EXIT2;
        status_assert(object_call_id(object_id, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));
        trace_off();
    }

    if(bitmap_bit_test(bitmap_exit_services1, object_id, N_BITS_ARG(MAX_OBJECTS)))
    {
        bitmap_bit_clear(bitmap_exit_services1, object_id, N_BITS_ARG(MAX_OBJECTS));

        trace_on();
        msg_initclose.t5_msg_initclose_message = T5_MSG_IC__SERVICES_EXIT1;
        status_assert(object_call_id(object_id, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));
        trace_off();
    }

    return(status);
}

#if defined(LOADS_CODE_MODULES)

OBJECT_PROTO(static, object_load_fail_sink)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM(p_data);
    return(STATUS_OK);
}

#endif /* LOADS_CODE_MODULES */

_Check_return_
static STATUS
object_load_internal(
    _InVal_     OBJECT_ID object_id)
{
    P_PROC_OBJECT p_proc_object = t5_glued_object(object_id);
    STATUS status;
#if defined(LOADS_CODE_MODULES)
    RUNTIME_INFO runtime_info;
    zero_struct(runtime_info);
#endif

    if(NULL == p_proc_object)
    {
#if defined(LOADS_CODE_MODULES)
        TCHARZ filename[BUF_MAX_PATHSTRING];

        if(status_fail(status = resource_dll_find(object_id, filename, elemof32(filename))))
        {
            if(STATUS_FAIL == status)
            {
                /* SKS 15apr95 reinstated event sink for this particular mode of object_load failure */
                g_proc_object[object_id] = object_load_fail_sink;
                status = STATUS_MODULE_NOT_FOUND;
            }
            return(status);
        }

#if RISCOS
        status_return(host_load_module(filename, &runtime_info, VERSION_NUMBER, NULL /* force to use global stubs */));
#elif WINDOWS
        status_return(host_load_module(filename, &runtime_info, object_id));
#endif

        p_proc_object = (P_PROC_OBJECT) runtime_info.p_module_entry;
        assert(p_proc_event);
#else
        return(STATUS_MODULE_NOT_FOUND);
#endif /* LOADS_CODE_MODULES */
    }

    if(status_fail(status = object_load_internal_install(object_id, p_proc_object)))
    {
#if defined(LOADS_CODE_MODULES)
        host_discard_module(&runtime_info);
#endif
        g_proc_object[object_id] = NULL;
        return(status);
    }

    return(status);
}

_Check_return_
extern STATUS
object_load(
    _InVal_     OBJECT_ID object_id)
{
    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert1(TEXT("object_load(object_id ") S32_TFMT TEXT(" out of range)"), (S32) object_id);
        return(status_check());
    }

    if(NULL == g_proc_object[object_id])
        status_return(object_load_internal(object_id));

    return(STATUS_OK);
}

/******************************************************************************
*
* loop over present objects
* start with OBJECT_ID_ENUM_START
* ends with STATUS_FAIL
*
******************************************************************************/

_Check_return_
extern STATUS
object_next(
    _InoutRef_  P_OBJECT_ID p_object_id)
{
    for(;;)
    {
        OBJECT_ID_INCR(*p_object_id);

        if(*p_object_id >= OBJECT_ID_MAX)
            return(STATUS_FAIL);

        if(g_proc_object[*p_object_id])
            return(STATUS_OK);
    }
}

/******************************************************************************
*
* return object prescence indicator
*
******************************************************************************/

_Check_return_
extern BOOL
object_present(
    _InVal_     OBJECT_ID object_id)
{
    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert1(TEXT("object_present(object_id ") S32_TFMT TEXT(" out of range)"), (S32) object_id);
        return(FALSE);
    }

    return(NULL != g_proc_object[object_id]);
}

/******************************************************************************
*
* get a specified space for object data
*
******************************************************************************/

_Check_return_
extern STATUS
object_realloc(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_P_ANY p_p_any /* pointer to object data */,
    _InRef_     PC_SLR p_slr,
    _InVal_     OBJECT_ID object_id,
    _InVal_     S32 object_size)
{
    STATUS status;
    P_CELL p_cell, p_cell_old;

    DOCU_ASSERT(p_docu);

    *p_p_any = P_DATA_NONE;

    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert1(TEXT("object_realloc(object_id ") S32_TFMT TEXT(" out of range)"), (S32) object_id);
        return(status_check());
    }

    p_cell_old = p_cell_from_slr(p_docu, p_slr);

    if((NULL == p_cell_old) || (object_id_from_cell(p_cell_old) != object_id))
    {
        /* throw away caches when an object changes type */
        CACHES_DISPOSE caches_dispose;
        region_from_two_slrs(&caches_dispose.region, p_slr, p_slr, TRUE);
        caches_dispose.data_space = DATA_NONE;
        status_assert(maeve_event(p_docu, T5_MSG_CACHES_DISPOSE, &caches_dispose));
    }

    status = slr_realloc(p_docu, &p_cell, p_slr, object_id, object_size);

    if(NULL != p_cell)
        *p_p_any = &p_cell->object[0];

    return(status);
}

/* init service modules before other objects */

_Check_return_
static STATUS
object_startup_phase_1(void)
{
    STATUS status = STATUS_OK;
    MSG_INITCLOSE msg_initclose;
    OBJECT_ID object_id;

#if RISCOS
    if(atexit(atexit_services2)) /* ensure service SERVICES_EXIT2 closedown proc called on exit */
        return(EXIT_FAILURE);

    if(atexit(atexit_services1)) /* ensure service SERVICES_EXIT1 closedown proc called on exit before SERVICES_EXIT2 closedown proc */
        return(EXIT_FAILURE);
#endif

    /* service-init interested bound objects on startup (reverse order to get file module up first of all) */
    object_id = OBJECT_ID_MAX;

    do  {
        OBJECT_ID_DECR(object_id);

        if(object_present(object_id))
        {
            msg_initclose.t5_msg_initclose_message = T5_MSG_IC__STARTUP_SERVICES;
            if(status_fail(status = object_call_id(object_id, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose)))
            {
                reperr_null(status);
                return(STATUS_FAIL);
            }

            /* keep these in step during startup */
            bitmap_bit_set(bitmap_exit_services1, object_id, N_BITS_ARG(MAX_OBJECTS));
            bitmap_bit_set(bitmap_exit_services2, object_id, N_BITS_ARG(MAX_OBJECTS));
        }
    }
    while(OBJECT_ID_FIRST != object_id);

#if RISCOS
    riscos_hourglass_on();

    cwimp_exit_services2 = 1; /* we piggyback on atexit_services2 so it's no-go until that's installed */
#endif

    return(status);
}

/* application-init interested bound objects on startup */

_Check_return_
static STATUS
object_startup_phase_2(void)
{
    STATUS status = STATUS_OK;
    MSG_INITCLOSE msg_initclose;
    OBJECT_ID object_id;

#if RISCOS
    if(atexit(atexit_application2)) /* ensure object EXIT2 closedown proc called on exit */
        return(STATUS_FAIL);

    if(atexit(atexit_application1)) /* ensure object EXIT1 closedown proc called on exit before EXIT2 closedown proc */
        return(STATUS_FAIL);
#endif

    for(object_id = OBJECT_ID_FIRST; object_id < OBJECT_ID_MAX; OBJECT_ID_INCR(object_id))
    {
        if(object_present(object_id))
        {
            msg_initclose.t5_msg_initclose_message = T5_MSG_IC__STARTUP;
            if(status_fail(status = object_call_id(object_id, P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose)))
            {
                reperr_null(status);
                return(STATUS_FAIL);
            }

            /* keep these in step during startup */
            bitmap_bit_set(bitmap_exit_application1, object_id, N_BITS_ARG(MAX_OBJECTS));
            bitmap_bit_set(bitmap_exit_application2, object_id, N_BITS_ARG(MAX_OBJECTS));
        }
    }

    send_maeve_exit_application1 = 1;
    send_maeve_exit_application2 = 1;

    return(status);
}

/* tell all loaded objects that they should attempt to load their config files */

_Check_return_
static STATUS
object_startup_phase_3(void)
{
    STATUS status = STATUS_OK;
    MSG_INITCLOSE msg_initclose;

    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__STARTUP_CONFIG;
    status = object_call_all(P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose);

    if(status_fail(status))
    {
        TCHARZ errstring[BUF_MAX_ERRORSTRING];
        resource_lookup_tstr_buffer(errstring, elemof32(errstring), status);
        reperr(ERR_LOADING_CONFIG, product_ui_id(), errstring);
    }

    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__STARTUP_CONFIG_ENDED;
    status_assert(object_call_all(P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose));

    if(status_fail(status))
        return(STATUS_FAIL);

    return(status);
}

_Check_return_
extern STATUS
object_startup(
    _In_        int phase)
{
    switch(phase)
    {
    default: default_unhandled();
#if CHECKING
    case 0:
#endif
        /* nothing needed here yet */
        return(STATUS_OK);

    case 1:
        return(object_startup_phase_1());

    case 2:
        return(object_startup_phase_2());

    case 3:
        return(object_startup_phase_3());
    }
}

/******************************************************************************
*
* return a pointer to object data
*
******************************************************************************/

_Check_return_
_Ret_maybenone_
extern P_ANY
p_object_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr,
    _InVal_     OBJECT_ID object_id)
{
    P_CELL p_cell;

    DOCU_ASSERT(p_docu);

    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert1(TEXT("p_object_from_slr(object_id ") S32_TFMT TEXT(" out of range)"), (S32) object_id);
        return(P_DATA_NONE);
    }
 
    p_cell = p_cell_from_slr(p_docu, p_slr);

    if(NULL != p_cell)
        if((OBJECT_ID_NONE == object_id) || (object_id_from_cell(p_cell) == object_id))
            return(&p_cell->object[0]);

    return(P_DATA_NONE);
}

/******************************************************************************
*
* return pointer to object instance data
*
* SKS says allow for not being allocated
*
******************************************************************************/

_Check_return_
_Ret_writes_maybenone_(bytesof_data)
extern P_BYTE
_p_object_instance_data(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id
    CODE_ANALYSIS_ONLY_ARG(_In_ U32 bytesof_data))
{
    P_BYTE p_data;

    DOCU_ASSERT(p_docu);
    CODE_ANALYSIS_ONLY(IGNOREPARM_InVal_(bytesof_data));

    if(!IS_OBJECT_ID_VALID(object_id))
    {
        myassert1(TEXT("p_object_instance_data(object_id ") S32_TFMT TEXT(" out of range)"), (S32) object_id);
        return(_P_DATA_NONE(P_BYTE));
    }

    p_data = collect_goto_item(BYTE, &p_docu->object_data_list, (LIST_ITEMNO) object_id);

    if(NULL == p_data)
        return(_P_DATA_NONE(P_BYTE));

    return(p_data);
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_object);

T5_MSG_PROTO(static, maeve_services_object_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT_THUNK:
        /* zero object data list then call all objects */
        list_init(&p_docu->object_data_list);

        return(object_call_all(p_docu, T5_MSG_INITCLOSE, (P_ANY) p_msg_initclose));

    case T5_MSG_IC__CLOSE_THUNK:
        { /* call all objects then delete object data list */
        STATUS status = object_call_all_accumulate(p_docu, T5_MSG_INITCLOSE, (P_ANY) p_msg_initclose);

        collect_delete(&p_docu->object_data_list);

        return(status);
        }

    case T5_MSG_IC__INIT1:
    case T5_MSG_IC__INIT2:
        /* call all objects */
        return(object_call_all(p_docu, T5_MSG_INITCLOSE, (P_ANY) p_msg_initclose));

    case T5_MSG_IC__CLOSE1:
    case T5_MSG_IC__CLOSE2:
        /* call all objects */
        return(object_call_all_accumulate(p_docu, T5_MSG_INITCLOSE, (P_ANY) p_msg_initclose));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_object)
{
    IGNOREPARM_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_object_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

#if defined(UNUSED_KEEP_ALIVE) /* let these ones through if there are any non-maeve_service object clients */
    case T5_MSG_FROM_WINDOWS:
    case T5_MSG_HAD_SERIOUS_ERROR:
        /* call all objects */
        return(object_call_all(p_docu, t5_message, p_data));
#endif

    default:
        return(STATUS_OK);
    }
}

#if WINDOWS

/******************************************************************************
*
* Call the exit handlers for Windows version
*
******************************************************************************/

extern void
t5_do_exit(void)
{
    atexit_application1();
    atexit_application2();
    atexit_services1();
    atexit_services2();
}

#endif /* WINDOWS */

/* end of object.c */
