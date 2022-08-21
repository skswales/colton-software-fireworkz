/* fl_xls.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Local header file for Excel spreadsheet load object module */

/* MRJC March 1994 */

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

#include "fl_xls/ff_xls.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*
messages
*/

#define XLS_MSG_BASE    (STATUS_MSG_INCREMENT * OBJECT_ID_FL_XLS)

#define XLS_MSG_PASSWORD_CAPTION            (XLS_MSG_BASE + 0)

/*
error numbers
*/

#define XLS_ERR_BASE    (STATUS_ERR_INCREMENT * OBJECT_ID_FL_XLS)

#define XLS_ERR(n)      (XLS_ERR_BASE - (n))

#define XLS_ERR_BADFILE_BAD_BOF             XLS_ERR(0)
#define XLS_ERR_ERROR_RQ                    XLS_ERR(1)
#define XLS_ERR_EXP                         XLS_ERR(2) /* -1 is reserved */
#define XLS_ERR_spare_3                     XLS_ERR(3)
#define XLS_ERR_BADFILE_NO_DIMENSIONS       XLS_ERR(4)
#define XLS_ERR_BADFILE_BAD_BOUNDSHEET      XLS_ERR(5)
#define XLS_ERR_BADFILE_ENCRYPTION          XLS_ERR(6)
#define XLS_ERR_BADFILE_PASSWORD_MISMATCH   XLS_ERR(7)

/*
exported routines
*/

_Check_return_
extern STATUS
xls_error_set(
    _In_opt_z_  PCTSTR errorstr);

T5_MSG_PROTO(extern, xls_msg_insert_foreign, P_MSG_INSERT_FOREIGN p_msg_insert_foreign);

#if defined(__cplusplus)
}
#endif

/* end of fl_xls.h */
