/* fl_xls.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Excel spreadsheet load object module for Fireworkz */

/* MRJC March 1994; SKS September 2006 */

#include "common/gflags.h"

#include "fl_xls/fl_xls.h"

#ifndef          __ev_eval_h
#include "cmodules/ev_eval.h"
#endif

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_FL_XLS)
extern PC_U8 rb_fl_xls_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_FL_XLS &rb_fl_xls_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_FL_XLS LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_FL_XLS DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_FL_XLS DONT_LOAD_RESOURCES

static struct FL_XLS_STATICS
{
    TCHARZ error_buffer[256]; /* must be >= 252 for RISC OS errors */
}
fl_xls_statics;

/*
hold an error string from the Excel I/O system
*/

static PTSTR fl_xls__errorbuffer;
static U32   fl_xls__errorbufelem;
static PTSTR fl_xls__errorptr;

static void
xls_error_buffer_set(
    _In_reads_(error_bufelem) PTSTR error_buffer,
    _InVal_     U32 error_bufelem)
{
    fl_xls__errorbuffer = error_buffer;
    fl_xls__errorbufelem = error_bufelem;
}

/******************************************************************************
*
* read the current error associated with this module if any
* clears error if set; error message only valid until next call to module
*
******************************************************************************/

_Check_return_
_Ret_maybenull_z_
static PCTSTR
xls_error_get(void)
{
    PCTSTR errorstr;

    errorstr = fl_xls__errorptr;
    fl_xls__errorptr = NULL;

    return(errorstr);
}

/******************************************************************************
*
* remember errors to get back later
*
******************************************************************************/

_Check_return_
extern STATUS
xls_error_set(
    _In_opt_z_  PCTSTR errorstr)
{
    if(errorstr  &&  !fl_xls__errorptr  &&  fl_xls__errorbuffer)
        tstr_xstrkpy(fl_xls__errorptr = fl_xls__errorbuffer, fl_xls__errorbufelem, errorstr);
#if TRACE_ALLOWED
    else if(fl_xls__errorptr)
        trace_0(TRACE_MODULE_FILE, TEXT("*** XLS ERROR LOST ***"));
#endif

    return(create_error(XLS_ERR_ERROR_RQ));
}

/******************************************************************************
*
* Excel file converter object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, xls_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        xls_error_buffer_set(fl_xls_statics.error_buffer, elemof32(fl_xls_statics.error_buffer));

        return(resource_init(OBJECT_ID_FL_XLS, P_BOUND_MESSAGES_OBJECT_ID_FL_XLS, P_BOUND_RESOURCES_OBJECT_ID_FL_XLS));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_FL_XLS));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, xls_msg_error_rq, _InoutRef_ P_MSG_ERROR_RQ p_msg_error_rq)
{
    PCTSTR p_error = xls_error_get();

    if(NULL == p_error)
    {   /* no error to be returned */
        assert0();
        return(STATUS_FAIL);
    }

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    tstr_xstrkpy(p_msg_error_rq->tstr_buf, p_msg_error_rq->elemof_buffer, p_error);
    return(STATUS_OK);
}

OBJECT_PROTO(extern, object_fl_xls);
OBJECT_PROTO(extern, object_fl_xls)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(xls_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_INSERT_FOREIGN:
        return(xls_msg_insert_foreign(p_docu, t5_message, (P_MSG_INSERT_FOREIGN) p_data));

    case T5_MSG_ERROR_RQ:
        return(xls_msg_error_rq(p_docu, t5_message, (P_MSG_ERROR_RQ) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of fl_xls.c */
