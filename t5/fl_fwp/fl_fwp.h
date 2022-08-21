/* fl_fwp.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Local header file for First Word Plus load object module */

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

/*
local structures
*/

typedef struct HILITE
{
    S32 pos;
    U8 state;
}
HILITE, * P_HILITE; typedef const HILITE * PC_HILITE;

/*
special FWP chars
*/

#define FWP_LF  '\x0A'
#define FWP_CPB '\x0B'
#define FWP_UPB '\x0C' /* SKS 03feb92 */
#define FWP_RET '\x0D'
#define FWP_FN  '\x18'
#define FWP_SH  '\x19'
#define FWP_ESC '\x1B'
#define FWP_SS1 '\x1C'
#define FWP_SS2 '\x1D'
#define FWP_SS3 '\x1E'
#define FWP_FMT '\x1F'

/*
FWP effect bits
*/

#define FWP_STATE_BOLD          1
#define FWP_STATE_ITALIC        4
#define FWP_STATE_UNDERLINE     8
#define FWP_STATE_SUPER         16
#define FWP_STATE_SUB           32

#define N_HILITES 5

/* end of fl_fwp.h */
