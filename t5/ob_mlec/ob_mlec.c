/* ob_mlec.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* mlec service module for Fireworkz */

/* SKS April 1992 */

#include "common/gflags.h"

#include "ob_mlec/ob_mlec.h"

#if RISCOS

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_MLEC)
extern PC_U8 rb_mlec_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_MLEC &rb_mlec_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_MLEC LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_MLEC DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_MLEC DONT_LOAD_RESOURCES

/******************************************************************************
*
* mlec service module object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, mlec_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP_SERVICES:
        return(resource_init(OBJECT_ID_MLEC, P_BOUND_MESSAGES_OBJECT_ID_MLEC, P_BOUND_RESOURCES_OBJECT_ID_MLEC));

    case T5_MSG_IC__SERVICES_EXIT2:
        return(resource_close(OBJECT_ID_MLEC));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_mlec);
OBJECT_PROTO(extern, object_mlec)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(mlec_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

#endif /* RISCOS */

/* end of ob_mlec.c */
