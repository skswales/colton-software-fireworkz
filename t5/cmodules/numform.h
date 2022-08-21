/* numform.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __numform_h
#define __numform_h

typedef struct NUMFORM_CONTEXT
{
    PC_USTR month_names[12];
    PC_USTR day_names[7];
    PC_USTR day_endings[31];
}
NUMFORM_CONTEXT, * P_NUMFORM_CONTEXT; typedef const NUMFORM_CONTEXT * PC_NUMFORM_CONTEXT;

typedef struct NUMFORM_PARMS
{
    PC_NUMFORM_CONTEXT  p_numform_context;
    PC_USTR             ustr_numform_numeric;
    PC_USTR             ustr_numform_datetime;
    PC_USTR             ustr_numform_texterror;
}
NUMFORM_PARMS, * P_NUMFORM_PARMS; typedef const NUMFORM_PARMS * PC_NUMFORM_PARMS;

#define NUMFORM_PARMS_INIT { NULL, NULL, NULL, NULL }

_Check_return_
extern STATUS
numform(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _InoutRef_opt_ P_QUICK_TBLOCK p_quick_tblock_style /*appended,terminated*/,
    _InRef_     PC_EV_DATA p_ev_data,
    _InRef_     PC_NUMFORM_PARMS p_numform_parms);

#endif /* __numform_h */

/* end of numform.h */
