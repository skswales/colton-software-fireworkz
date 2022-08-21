/* ob_recn.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2016 Stuart Swales */

/* rec-dummy object module */

/* SKS Aug 2014 */

#include "common/gflags.h"

#include "ob_recn/ob_recn.h"

#include "ob_skel/xp_skeld.h"

#if RISCOS
#define MSG_WEAK &rb_recn_msg_weak
extern PC_U8 rb_recn_msg_weak;
#endif
#define P_BOUND_RESOURCES_OBJECT_ID_RECN NULL

/*
construct table
*/

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    /* NB We only intercept this command in ob_recn if ob_rec is marked as not available in the Config file */
    { "DBTable",                NULL,                       T5_CMD_RECORDZ_IO_TABLE  },

    { NULL,                     NULL,                       T5_EVENT_NONE } /* end of table */
};

/*
Flow:39 (OBJECT_ID_REC_FLOW) is handled OK by skeleton when ob_recb is not present
*/

T5_MSG_PROTO(static, recn_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(resource_init(OBJECT_ID_RECN, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_RECN));

        return(register_object_construct_table(OBJECT_ID_RECN, object_construct_table, FALSE /* no inlines */));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_RECN));

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(static, recn_cmd_recordz_io_table)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    reperr_null(RECN_ERR_NO_DATABASE_MODULE);

    return(STATUS_OK); /* but keep going - this may be a database in a table in a doc we can otherwise read */
}

OBJECT_PROTO(extern, object_recn);
OBJECT_PROTO(extern, object_recn)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(recn_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_CMD_RECORDZ_IO_TABLE:
        return(recn_cmd_recordz_io_table(p_docu, t5_message, (PC_T5_CMD) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of ob_recn.c */
