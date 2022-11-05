/* fl_rtf.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Local header file for RTF load object module for Fireworkz */

/* JAD Jul 1993 */

#ifndef __fl_rtf_h
#define __fl_rtf_h

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

#define OB_RTF_ERR_MISMATCHED_BRACKETS (STATUS_ERR_INCREMENT * OBJECT_ID_FL_RTF)
/* 1 is ERROR_RQ */

#define DEFAULT_TABLE_INDENT    1474

/*
internal structure
*/

enum RTF_HEFO_TYPES
{
    RTF_HEFO_LEFT = 0,
    RTF_HEFO_RIGHT,
    RTF_HEFO_FIRST,
    RTF_HEFO_ALL
};

typedef struct RTF_STYLESHEET
{
    U32 style_number;
    ARRAY_HANDLE_TSTR h_style_name_tstr;
}
RTF_STYLESHEET, * P_RTF_STYLESHEET;

typedef struct RTF_FONT_TABLE
{
    U32 font_number;
    ARRAY_HANDLE_TSTR h_rtf_font_name_tstr;
}
RTF_FONT_TABLE, * P_RTF_FONT_TABLE;

typedef struct STATE_STYLE
{
    STYLE style;
}
STATE_STYLE, * P_STATE_STYLE; typedef const STATE_STYLE * PC_STATE_STYLE;

static inline void
state_style_init(
    _OutRef_    P_STATE_STYLE p_state_style)
{
    zero_struct_ptr_fn(p_state_style);
}

typedef struct RTF_REGION
{
    STATE_STYLE state_style;
    DOCU_AREA docu_area;
    U8        closed;
}
RTF_REGION, * P_RTF_REGION;

typedef struct RTF_TABLE
{
    S32 width;

    U8 left;
    U8 top;
    U8 right;
    U8 bottom;
}
RTF_TABLE, * P_RTF_TABLE;

#define RTF_BORDER_OFF 0
#define RTF_BORDER_ON 1
#define RTF_BORDER_DONT_CARE 2

#endif /* __fl_rtf_h */

/* end of fl_rtf.h */
