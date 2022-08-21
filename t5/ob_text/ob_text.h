/* ob_text.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Text object module header */

/* MRJC December 1991 */

/*
ob_text instance data
*/

typedef struct TEXT_INSTANCE_DATA
{
    ARRAY_HANDLE h_ss_name_record;
}
TEXT_INSTANCE_DATA, * P_TEXT_INSTANCE_DATA;

_Check_return_
_Ret_valid_
static inline P_TEXT_INSTANCE_DATA
p_object_instance_data_TEXT(
    _InRef_     P_DOCU p_docu)
{
    P_TEXT_INSTANCE_DATA p_text_instance_data = (P_TEXT_INSTANCE_DATA)
        _p_object_instance_data(p_docu, OBJECT_ID_TEXT CODE_ANALYSIS_ONLY_ARG(sizeof32(TEXT_INSTANCE_DATA)));
    PTR_ASSERT(p_text_instance_data);
    return(p_text_instance_data);
}

typedef struct SS_NAME_RECORD
{
    EV_HANDLE ev_handle;                 /* handle of name*/
    SLR slr;                             /* cell containing reference */
    UREF_HANDLE uref_handle;             /* handle to uref dependency */
    CLIENT_HANDLE client_handle;         /* uref's handle to us */
    U8 deleted;                          /* record deleted */
}
SS_NAME_RECORD, * P_SS_NAME_RECORD;

/* end of ob_text.h */
