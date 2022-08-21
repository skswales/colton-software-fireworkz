/* fs_xls.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2015 Stuart Swales */

/* Excel spreadsheet save object module for Fireworkz */

/* SKS April 2014 */

#include "common/gflags.h"

#include "fs_xls/fs_xls.h"

#if RISCOS
#define MSG_WEAK &rb_fs_xls_msg_weak
extern PC_U8 rb_fs_xls_msg_weak;
#endif
#define P_BOUND_RESOURCES_OBJECT_ID_FS_XLS NULL

/******************************************************************************
*
* MSG_SAVE_FOREIGN
*
******************************************************************************/

T5_MSG_PROTO(static, xls_msg_save_foreign, _InoutRef_ P_MSG_SAVE_FOREIGN p_msg_save_foreign)
{
    P_FF_OP_FORMAT p_ff_op_format = p_msg_save_foreign->p_ff_op_format;

    IGNOREPARM_InVal_(t5_message);

#if 1
    return(xls_save_biff(p_docu, p_ff_op_format));
#else
    /* write UTF-8 encoded XML file */
    status_return(foreign_save_set_io_type(p_ff_op_format, IO_TYPE_UTF_8));

    return(xls_save_xml(p_docu, p_ff_op_format));
#endif
}

/******************************************************************************
*
* Excel file save object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, xls_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        /*return(resource_init(OBJECT_ID_FS_XLS, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_FS_XLS));*/

    case T5_MSG_IC__EXIT1:
        /*return(resource_close(OBJECT_ID_FS_XLS));*/

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_fs_xls);
OBJECT_PROTO(extern, object_fs_xls)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(xls_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_SAVE_FOREIGN:
        return(xls_msg_save_foreign(p_docu, t5_message, (P_MSG_SAVE_FOREIGN) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of fs_xls.c */
