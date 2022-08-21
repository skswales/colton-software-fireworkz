/* mathxtr2.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS September 1991 */

#ifndef __mathxtr2_h
#define __mathxtr2_h

/*
colID states
*/

#define LINEST_A_COLOFF (-2)
#define LINEST_Y_COLOFF (-1)
#define LINEST_X_COLOFF ( 0)

#define LINEST_COL_ID int

typedef S32 LINEST_COLOFF;
typedef S32 LINEST_ROWOFF;

typedef /*_Check_return_*/ F64 (* P_PROC_LINEST_DATA_GET) (
    _InVal_     CLIENT_HANDLE client_handle,
    _In_        LINEST_COLOFF colID,
    _In_        LINEST_ROWOFF row);

#define PROC_LINEST_DATA_GET_PROTO(_e_s, _proc_name, client_handle, colID, row) \
_Check_return_ \
_e_s F64 \
_proc_name( \
    _InVal_     CLIENT_HANDLE client_handle, \
    _In_        LINEST_COLOFF colID, \
    _In_        LINEST_ROWOFF row)

typedef /*_Check_return_*/ STATUS (* P_PROC_LINEST_DATA_PUT) (
    _InVal_     CLIENT_HANDLE client_handle,
    _In_        LINEST_COLOFF colID,
    _In_        LINEST_ROWOFF row,
    _InRef_     PC_F64 value);

#define PROC_LINEST_DATA_PUT_PROTO(_e_s, _proc_name, client_handle, colID, row, value) \
_Check_return_ \
_e_s STATUS \
_proc_name( \
    _InVal_     CLIENT_HANDLE client_handle, \
    _In_        LINEST_COLOFF colID, \
    _In_        LINEST_ROWOFF row, \
    _InRef_     PC_F64 value)

/*
exported functions
*/

_Check_return_
extern STATUS
linest(
    _InRef_     P_PROC_LINEST_DATA_GET p_proc_get,
    _InRef_     P_PROC_LINEST_DATA_PUT p_proc_put,
    _InVal_     CLIENT_HANDLE client_handle,
    _InVal_     U32 ext_m /* number of independent x variables */,
    _InVal_     U32 n     /* number of data points */);

#endif /* __mathxtr2_h */

/* end of mathxtr2.h */
