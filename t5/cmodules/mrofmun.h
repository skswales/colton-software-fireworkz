/* mrofmun.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __mrofmun_h
#define __mrofmun_h

typedef struct MROFMUN_ENTRY
{
    S32 search;
    STYLE_HANDLE style_handle;
    NUMFORM_PARMS numform_parms;
}
MROFMUN_ENTRY, * P_MROFMUN_ENTRY; typedef const MROFMUN_ENTRY * PC_MROFMUN_ENTRY;

_Check_return_
extern STATUS
autoformat(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _OutRef_    P_STYLE_HANDLE p_style_handle_out,
    _In_z_      PC_USTR ustr,
    _InRef_     PC_ARRAY_HANDLE p_array_handle_mrofmuns_in);

#endif /* __mrofmun_h */

/* end of mrofmun.h */
