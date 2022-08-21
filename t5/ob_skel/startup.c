/* startup.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

int
g_has_colour_picker = TRUE;

/*
explicit imports from other skeleton components here to save exporting them to all and sundry
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_dll);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_event);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_print);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_win);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_fonty);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_object);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_of_load);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_bord);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_cont);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_docno);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_find);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_form);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_hefod);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_mark);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_menu);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_null);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_print);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_prost);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_save);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_slot);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_styl);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_tx_cache);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ui_save);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_vi_cmd);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_vi_edge);

static const P_PROC_MAEVE_SERVICES_EVENT
rb_skel_maeve_services[] =
{
#if RISCOS
    maeve_services_event_ho_event, /* MUST be first */
#if defined(LOADS_CODE_MODULES)
    maeve_services_event_ho_dll,
#endif
#elif WINDOWS
    maeve_services_event_ho_win,
    maeve_services_event_ho_print,
#if defined(LOADS_CODE_MODULES)
    maeve_services_event_ho_dll,
#endif
#endif /* OS */
    maeve_services_event_fonty,
    maeve_services_event_sk_styl,
    maeve_services_event_sk_slot,
    maeve_services_event_sk_form,
    maeve_services_event_sk_mark,
    maeve_services_event_sk_bord,
    maeve_services_event_sk_menu,
    maeve_services_event_sk_cont,
    maeve_services_event_sk_docno,
    maeve_services_event_sk_find,
    maeve_services_event_sk_hefod,
    maeve_services_event_sk_null,
    maeve_services_event_sk_print,
    maeve_services_event_sk_prost,
    maeve_services_event_sk_save,
    maeve_services_event_tx_cache,
    maeve_services_event_ui_save,
    maeve_services_event_vi_cmd,
    maeve_services_event_vi_edge,
    maeve_services_event_object,
    maeve_services_event_of_load,
    NULL
};

/******************************************************************************
*
* Handle startup, after host specific initialisation
* and prior to polling the host system for events and
* decoding the command line.
*
******************************************************************************/

/* NB this one is called prior to allocator init */

_Check_return_
extern STATUS
startup_t5_application_1(void)
{
#if RISCOS
#if 1
    {
    _kernel_swi_regs rs;
    rs.r[1] = (int) "ColourPicker_OpenDialogue" /*0x47702*/;
    if(NULL != _kernel_swi(0x39 /*OS_SWINumberFromString*/, &rs, &rs))
        g_has_colour_picker = FALSE;
    } /*block*/
#else
    g_has_colour_picker = FALSE;
#endif
#endif /* RISCOS */

    sk_alpha_startup();
    sk_docno_startup();

    return(STATUS_OK);
}

_Check_return_
extern STATUS
startup_t5_application_2(void)
{
    status_return(object_startup(0));

    /* suss bound objects. don't call any till all are bound that are going to be */
    t5_glue_objects();

    status_return(object_startup(1));

    { /* add skeleton startup/init/close service handlers */
    UINT i = 0;
    P_PROC_MAEVE_SERVICES_EVENT p_proc_maeve_services_event;
    STATUS status;

    while(NULL != (p_proc_maeve_services_event = rb_skel_maeve_services[i++]))
    {
        if(status_fail(status = maeve_services_event_handler_add(p_proc_maeve_services_event)))
        {
            reperr_null(status);
            return(STATUS_FAIL);
        }
    }
    } /*block*/

    {
    MSG_INITCLOSE msg_initclose;
    STATUS status;
    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__STARTUP;
    if(status_fail(status = maeve_service_event(P_DOCU_NONE, T5_MSG_INITCLOSE, &msg_initclose)))
    {
        reperr_null(status);
        return(STATUS_FAIL);
    }
    } /*block*/

    status_return(object_startup(2));

    status_return(load_ownform_config_file());

    status_return(object_startup(3));

    return(STATUS_OK);
}

#if RISCOS

_Check_return_
extern STATUS
ensure_memory_froth(void)
{
    return(alloc_ensure_froth(0x6000));
}

#elif WINDOWS
    /* STATUS_OK */
#endif /* OS */

/* end of startup.c */
