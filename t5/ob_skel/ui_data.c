/* ui_data.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* UI data handling */

/* SKS March 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_toolb/xp_toolb.h"

#include "cmodules/collect.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

static UCHARZ numform_2dp_ustr_buf[6]; /* must fit "0.,##" */

/*
exported data
*/

const ARRAY_INIT_BLOCK
array_init_block_ui_text = aib_init(1, sizeof32(UI_TEXT), TRUE);

AL_ARRAY_ALLOC_IMPL(extern, UI_TEXT)
AL_ARRAY_EXTEND_BY_IMPL(extern, UI_TEXT)
AL_ARRAY_INSERT_BEFORE_IMPL(extern, UI_TEXT)

const DIALOG_CONTROL
dialog_main_group =
{
    DIALOG_MAIN_GROUP, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0 },
    { DRT(LTRB, GROUPBOX) }
};

const DIALOG_CONTROL
dialog_col1_group =
{
    DIALOG_COL1_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0 },
    { DRT(LTRB, GROUPBOX) }
};

const DIALOG_CONTROL
dialog_col2_group =
{
    DIALOG_COL2_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_COL1_GROUP, DIALOG_COL1_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, 0, 0 },
    { DRT(RTRB, GROUPBOX) }
};

const DIALOG_CONTROL
defbutton_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_MAIN_GROUP, DIALOG_MAIN_GROUP, DIALOG_CONTROL_SELF },
#if WINDOWS
    { DIALOG_DEFOK_H, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
#else
    { DIALOG_CONTENTS_CALC, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
#endif
    { DRT(RBRT, PUSHBUTTON), 1 }
};

const DIALOG_CONTROL_DATA_PUSHBUTTON
defbutton_ok_data = { { DIALOG_COMPLETION_OK }, UI_TEXT_INIT_RESID(MSG_OK) };

const DIALOG_CONTROL_DATA_PUSHBUTTONR
defbutton_ok_persist_data = { { DIALOG_COMPLETION_OK, 0, 0, 1 /*alternate_right*/ }, UI_TEXT_INIT_RESID(MSG_OK), DIALOG_COMPLETION_OK_PERSIST };

const DIALOG_CONTROL
stdbutton_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, IDOK, IDOK, IDOK },
#if WINDOWS
    { DIALOG_STDCANCEL_H, -DIALOG_DEFPUSHEXTRA_V, DIALOG_STDSPACING_H, -DIALOG_DEFPUSHEXTRA_V },
#else
    { DIALOG_CONTENTS_CALC, -DIALOG_DEFPUSHEXTRA_V, DIALOG_STDSPACING_H, -DIALOG_DEFPUSHEXTRA_V },
#endif
    { DRT(RTLB, PUSHBUTTON), 1 }
};

const DIALOG_CONTROL_DATA_PUSHBUTTON
stdbutton_cancel_data = { { DIALOG_COMPLETION_CANCEL }, UI_TEXT_INIT_RESID(MSG_CANCEL) };

const DIALOG_CONTROL_DATA_LIST_TEXT
stdlisttext_data = { { 0 /*force_v_scroll*/, 0 /*disable_double*/,  2 /*tab_position - irrelevant for most clients*/ } };

const DIALOG_CONTROL_DATA_LIST_TEXT
stdlisttext_data_vsc = { { 1 /*force_v_scroll*/, 0 /*disable_double*/,  2 /*tab_position - irrelevant for most clients*/ } };

const DIALOG_CONTROL_DATA_LIST_TEXT
stdlisttext_data_dd = { { 0 /*force_v_scroll*/, 1 /*disable_double*/,  2 /*tab_position - irrelevant for most clients*/ } };

const DIALOG_CONTROL_DATA_LIST_TEXT
stdlisttext_data_dd_vsc = { { 1 /*force_v_scroll*/, 1 /*disable_double*/,  2 /*tab_position - irrelevant for most clients*/ } };

/*
standard RGB group
*/

const DIALOG_CONTROL
rgb_group_inner =
{
    DIALOG_ID_RGB_GROUP_INNER, DIALOG_ID_RGB_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM },
    { DRT(LTRB, GROUPBOX) }
};

const DIALOG_CONTROL
rgb_tx[3] =
{
    { DIALOG_ID_RGB_TX_R, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_PARENT, DIALOG_ID_RGB_R, DIALOG_CONTROL_SELF, DIALOG_ID_RGB_R },  { DIALOG_STDCHECK_H + DIALOG_SMALLSPACING_H, 0, RGB_TX_H }, { DRT(LTLB, STATICTEXT) } },
    { DIALOG_ID_RGB_TX_G, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_ID_RGB_TX_R, DIALOG_ID_RGB_G, DIALOG_ID_RGB_TX_R, DIALOG_ID_RGB_G },  { 0 }, { DRT(LTRB, STATICTEXT) } },
    { DIALOG_ID_RGB_TX_B, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_ID_RGB_TX_G, DIALOG_ID_RGB_B, DIALOG_ID_RGB_TX_G, DIALOG_ID_RGB_B },  { 0 }, { DRT(LTRB, STATICTEXT) } }
};

const DIALOG_CONTROL
rgb_bump[3] =
{
    { DIALOG_ID_RGB_R, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_ID_RGB_TX_R, DIALOG_CONTROL_PARENT },  { DIALOG_SMALLSPACING_H, 0, RGB_FIELDS_H, DIALOG_STDBUMP_V }, { DRT(RTLT, BUMP_S32), 1 } },
    { DIALOG_ID_RGB_G, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_ID_RGB_R, DIALOG_ID_RGB_R, DIALOG_ID_RGB_R },  { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V }, { DRT(LBRT, BUMP_S32), 1 } },
    { DIALOG_ID_RGB_B, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_ID_RGB_G, DIALOG_ID_RGB_G, DIALOG_ID_RGB_G },  { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V }, { DRT(LBRT, BUMP_S32), 1 } }
};

const DIALOG_CONTROL
rgb_patch =
{
    DIALOG_ID_RGB_PATCH, DIALOG_ID_RGB_GROUP_INNER,
    { DIALOG_ID_RGB_GROUP, DIALOG_ID_RGB_B },
    { -DIALOG_STDGROUP_LM, DIALOG_STDSPACING_V, 8 * RGB_PATCHES_H, RGB_PATCHES_V },
    { DRT(LBLT, USER) }
};

const DIALOG_CONTROL
rgb_button =
{
    DIALOG_ID_RGB_BUTTON, DIALOG_ID_RGB_GROUP_INNER,
    { DIALOG_ID_RGB_PATCH, DIALOG_ID_RGB_PATCH, DIALOG_ID_RGB_PATCH },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON) }
};

const DIALOG_CONTROL
rgb_patches[16] =
{
    { DIALOG_ID_RGB_0, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_7, DIALOG_ID_RGB_1, DIALOG_ID_RGB_7 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_1, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_7, DIALOG_ID_RGB_2, DIALOG_ID_RGB_7 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_2, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_7, DIALOG_ID_RGB_3, DIALOG_ID_RGB_7 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_3, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_7, DIALOG_ID_RGB_4, DIALOG_ID_RGB_7 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_4, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_7, DIALOG_ID_RGB_5, DIALOG_ID_RGB_7 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_5, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_7, DIALOG_ID_RGB_6, DIALOG_ID_RGB_7 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_6, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_7, DIALOG_ID_RGB_7, DIALOG_ID_RGB_7 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_7, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_BUTTON, DIALOG_ID_RGB_BUTTON }, { RGB_PATCHES_H, DIALOG_STDSPACING_V, 0, RGB_PATCHES_V }, { DRT(RBRT, USER) } },
    { DIALOG_ID_RGB_8, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_15, DIALOG_ID_RGB_9, DIALOG_ID_RGB_15 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_9, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_15, DIALOG_ID_RGB_10, DIALOG_ID_RGB_15 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_10, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_15, DIALOG_ID_RGB_11, DIALOG_ID_RGB_15 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_11, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_15, DIALOG_ID_RGB_12, DIALOG_ID_RGB_15 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_12, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_15, DIALOG_ID_RGB_13, DIALOG_ID_RGB_15 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_13, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_15, DIALOG_ID_RGB_14, DIALOG_ID_RGB_15 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_14, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_15, DIALOG_ID_RGB_15, DIALOG_ID_RGB_15 }, { RGB_PATCHES_H }, { DRT(RTLB, USER) } },
    { DIALOG_ID_RGB_15, DIALOG_ID_RGB_GROUP_INNER, { DIALOG_CONTROL_SELF, DIALOG_ID_RGB_7, DIALOG_ID_RGB_7 }, { RGB_PATCHES_H, 0, 0, RGB_PATCHES_V }, { DRT(RBRT, USER) } }
};

const DIALOG_CONTROL
rgb_transparent =
{
    DIALOG_ID_RGB_T, DIALOG_ID_RGB_GROUP,
    { DIALOG_ID_RGB_GROUP_INNER, DIALOG_ID_RGB_GROUP_INNER, DIALOG_ID_RGB_GROUP_INNER },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCHECK_V },
    { DRT(LBRT, CHECKBOX) }
};

const DIALOG_CONTROL_DATA_GROUPBOX
rgb_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_RGB_COLOUR), { 0, 0, 0, FRAMED_BOX_GROUP } };

const DIALOG_CONTROL_DATA_STATICTEXT
rgb_tx_data[3] =
{
    { UI_TEXT_INIT_RESID(MSG_DIALOG_RGB_TX_R), { 0 /*1 left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } },
    { UI_TEXT_INIT_RESID(MSG_DIALOG_RGB_TX_G), { 0 /*1 left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } },
    { UI_TEXT_INIT_RESID(MSG_DIALOG_RGB_TX_B), { 0 /*1 left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } },
};

static const UI_CONTROL_S32
rgb_bump_control = { 0, 255, 5 };

const DIALOG_CONTROL_DATA_BUMP_S32
rgb_bump_data = { { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/, &rgb_bump_control } /*BUMP_XX*/ };

const DIALOG_CONTROL_DATA_PUSHBUTTON
rgb_button_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_RGB_BUTTON) };

const DIALOG_CONTROL_DATA_USER
rgb_patch_data = { 0, { FRAMED_BOX_BUTTON_OUT, 1 /*state_is_rgb*/ }, NULL };

const DIALOG_CONTROL_DATA_USER
rgb_patches_data[16] =
{
    {  0, { FRAMED_BOX_BUTTON_OUT, 0, 1 }  },
    {  1, { FRAMED_BOX_BUTTON_OUT, 0, 1 }  },
    {  2, { FRAMED_BOX_BUTTON_OUT, 0, 1}  },
    {  3, { FRAMED_BOX_BUTTON_OUT, 0, 1 }  },
    {  4, { FRAMED_BOX_BUTTON_OUT, 0, 1 }  },
    {  5, { FRAMED_BOX_BUTTON_OUT, 0, 1 }  },
    {  6, { FRAMED_BOX_BUTTON_OUT, 0, 1 }  },
    {  7, { FRAMED_BOX_BUTTON_OUT, 0, 1 }  },
    {  8, { FRAMED_BOX_BUTTON_OUT, 0, 1 }  },
    {  9, { FRAMED_BOX_BUTTON_OUT, 0, 1 }  },
    { 10, { FRAMED_BOX_BUTTON_OUT, 0, 1 } },
    { 11, { FRAMED_BOX_BUTTON_OUT, 0, 1 } },
    { 12, { FRAMED_BOX_BUTTON_OUT, 0, 1 } },
    { 13, { FRAMED_BOX_BUTTON_OUT, 0, 1 } },
    { 14, { FRAMED_BOX_BUTTON_OUT, 0, 1 } },
    { 15, { FRAMED_BOX_BUTTON_OUT, 0, 1 } }
};

const DIALOG_CONTROL_DATA_CHECKBOX
rgb_transparent_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_RGB_TX_T) };

static const RESOURCE_BITMAP_ID
line_style_bitmap[5] =
{
    { OBJECT_ID_SKEL, SKEL_ID_BM_LINE_NONE  },
    { OBJECT_ID_SKEL, SKEL_ID_BM_LINE_THIN  },
    { OBJECT_ID_SKEL, SKEL_ID_BM_LINE_STD   },
    { OBJECT_ID_SKEL, SKEL_ID_BM_LINE_STDD  },
    { OBJECT_ID_SKEL, SKEL_ID_BM_LINE_THICK }
};

const DIALOG_CONTROL_DATA_RADIOPICTURE
line_style_data[5] =
{
    { { 0 }, SF_BORDER_NONE , &line_style_bitmap[0] },
    { { 0 }, SF_BORDER_THIN, &line_style_bitmap[1] },
    { { 0 }, SF_BORDER_STANDARD, &line_style_bitmap[2] },
    { { 0 }, SF_BORDER_BROKEN, &line_style_bitmap[3] },
    { { 0 } , SF_BORDER_THICK, &line_style_bitmap[4] }
};

static const RESOURCE_BITMAP_ID
style_text_bold_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_BOLD };

const DIALOG_CONTROL_DATA_CHECKPICTURE
style_text_bold_data = { { 0 },  &style_text_bold_bitmap };

static const RESOURCE_BITMAP_ID
style_text_italic_bitmap = { OBJECT_ID_SKEL, SKEL_ID_BM_ITALIC };

const DIALOG_CONTROL_DATA_CHECKPICTURE
style_text_italic_data = { { 0 }, &style_text_italic_bitmap };

const DIALOG_CONTROL_DATA_STATICTEXT
measurement_points_data = { UI_TEXT_INIT_RESID(MSG_MEASUREMENT_POINTS), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

#if RISCOS

/******************************************************************************
*
* form an allocated string from a UI_TEXT
* new string always owned by caller - should use tstr_clr() to dispose of it
*
******************************************************************************/

_Check_return_
extern STATUS
ui_text_copy_as_sbstr(
    _OutRef_    P_P_SBSTR p_sbstr,
    _InRef_     PC_UI_TEXT p_src_ui_text)
{
    PCTSTR src_tstr = ui_text_tstr_no_default(p_src_ui_text);
    PC_SBSTR src;
    U32 len;
    STATUS status;

    *p_sbstr = NULL;

    if(!src_tstr)
        return(STATUS_OK);

    src = _sbstr_from_tstr(src_tstr);

    len = strlen32p1(src);

    if(NULL != (*p_sbstr = al_ptr_alloc_bytes(P_SBSTR, len, &status)))
        memcpy32(*p_sbstr, src, len);

    return(status);
}

#if defined(UNUSED_KEEP_ALIVE)

extern void
ui_text_copy_as_sbstr_buf(
    _Out_writes_z_(buflen) P_SBSTR sbstr_buf,
    _InVal_     U32 buflen,
    _InRef_     PC_UI_TEXT p_src_ui_text)
{
    PCTSTR src_tstr = ui_text_tstr_no_default(p_src_ui_text);
    PC_SBSTR src;
    U32 len;

    *sbstr_buf = CH_NULL;

    if(!src_tstr)
        return;

    src = _sbstr_from_tstr(src_tstr);

    len = strlen32p1(src);

    memcpy32(sbstr_buf, src, MIN(len, buflen));

    if(len > buflen)
        sbstr_buf[buflen - 1] = CH_NULL;
}

#endif /* UNUSED_KEEP_ALIVE */

#endif /* RISCOS */

/******************************************************************************
*
* return the number of bytes this type occupies in a UI_DATA item
*
******************************************************************************/

_Check_return_
extern S32
ui_bytes_per_item(
    _InVal_     UI_DATA_TYPE ui_data_type)
{
    P_UI_DATA dummy = NULL; /* keep compilers happy */

    IGNOREPARM(dummy); /* keep dataflower happy */

    switch(ui_data_type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_DATA_TYPE_NONE:
#endif
        return(0);

    case UI_DATA_TYPE_ANY:
        return(sizeof32(dummy->any));

    case UI_DATA_TYPE_F64:
        return(sizeof32(dummy->f64));

    case UI_DATA_TYPE_P_U8:
        return(sizeof32(dummy->p_u8));

    case UI_DATA_TYPE_S32:
        return(sizeof32(dummy->s32));

    case UI_DATA_TYPE_TEXT:
        return(sizeof32(dummy->text));
    }
}

/******************************************************************************
*
* ensure that a UI_DATA item in whithin its allowable bounds
*
******************************************************************************/

_Check_return_
extern STATUS
ui_data_bound_check(
    _InVal_     UI_DATA_TYPE ui_data_type,
    _InoutRef_  P_UI_DATA p_ui_data,
    /*in*/      const void /*UI_CONTROL*/ * const p_ui_control)
{
    if(NULL != p_ui_control)
        switch(ui_data_type)
        {
        default: default_unhandled();
#if CHECKING
        case UI_DATA_TYPE_NONE:
        case UI_DATA_TYPE_P_U8:
        case UI_DATA_TYPE_TEXT:
#endif
            break;

        case UI_DATA_TYPE_ANY:
            {
            PC_UI_CONTROL_ANY p = (PC_UI_CONTROL_ANY) p_ui_control;
            status_return((* p->data_bound_check_proc) (p->data_bound_check_handle, &p_ui_data->any));
            break;
            }

        case UI_DATA_TYPE_F64:
            {
            PC_UI_CONTROL_F64 p = (PC_UI_CONTROL_F64) p_ui_control;
            if( p_ui_data->f64 < p->min_val)
                p_ui_data->f64 = p->min_val;
            if( p_ui_data->f64 > p->max_val)
                p_ui_data->f64 = p->max_val;
            break;
            }

        case UI_DATA_TYPE_S32:
            {
            PC_UI_CONTROL_S32 p = (PC_UI_CONTROL_S32) p_ui_control;
            if( p_ui_data->s32 < p->min_val)
                p_ui_data->s32 = p->min_val;
            if( p_ui_data->s32 > p->max_val)
                p_ui_data->s32 = p->max_val;
            break;
            }
        }

    return(STATUS_OK);
}

/******************************************************************************
*
* dispose of a UI data item
*
******************************************************************************/

extern void
ui_data_dispose(
    _InVal_     UI_DATA_TYPE ui_data_type,
    _InoutRef_  P_UI_DATA p_ui_data)
{
    switch(ui_data_type)
    {
    default:
        break;

    case UI_DATA_TYPE_TEXT:
        ui_text_dispose(&p_ui_data->text);
        break;
    }
}

/******************************************************************************
*
* increment or decrement a UI data item
*
******************************************************************************/

static void
ui_data_inc_dec_f64(
    _InVal_     S32 inc,
    /*inout*/ P_UI_DATA p_ui_data,
    _InRef_     PC_UI_CONTROL_F64 p_ui_control_f64)
{
    F64 f = p_ui_data->f64;
    F64 inc_dec_round;
    F64 bump_val;

    if(NULL != p_ui_control_f64)
    {
        inc_dec_round = p_ui_control_f64->inc_dec_round;
        bump_val = p_ui_control_f64->bump_val;
    }
    else
    {
        inc_dec_round = 1.0;
        bump_val = 1.0;
    }

    if(inc_dec_round)
    {
        F64 r = f * inc_dec_round;
        F64 i;

        r = modf(r, &i);

        if(inc)
        {
            if(r > -0.01)
            {
                if(r > 0.99)
                    /* already 'at' granularity so add one in */
                    i += 1.0;

                r = i / inc_dec_round;

                f = r + bump_val;
            }
            else
            {
                /* not already 'at' granularity so round up to this one (closer to zero) */
                r = i / inc_dec_round;

                f = r;
            }
        }
        else
        {
            if(r < 0.01)
            {
                if(r < -0.99)
                    /* already 'at' granularity so take one off */
                    i -= 1.0;

                r = i / inc_dec_round;

                f = r - bump_val;
            }
            else
            {
                /* not already 'at' granularity so round down to this one */
                r = i / inc_dec_round;

                f = r;
            }
        }
    }
    else
    {
        if(inc)
            f += bump_val;
        else
            f -= bump_val;
    }

    p_ui_data->f64 = f;
}

_Check_return_
extern STATUS
ui_data_inc_dec(
    _InVal_     UI_DATA_TYPE ui_data_type,
    _InoutRef_  P_UI_DATA p_ui_data,
    /*in*/      const void /*UI_CONTROL*/ * const p_ui_control,
    _InVal_     BOOL inc)
{
    BOOL bcheck = 1;

    switch(ui_data_type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_DATA_TYPE_NONE:
    case UI_DATA_TYPE_P_U8:
    case UI_DATA_TYPE_TEXT:
#endif
        bcheck = 0;
        break;

    case UI_DATA_TYPE_ANY:
        {
        PC_UI_CONTROL_ANY p = (PC_UI_CONTROL_ANY) p_ui_control;
        if(p)
            status_return((* p->data_inc_dec_proc) (p->data_inc_dec_handle, &p_ui_data->any, inc));
        break;
        }

    case UI_DATA_TYPE_F64:
        ui_data_inc_dec_f64(inc, p_ui_data, (PC_UI_CONTROL_F64) p_ui_control);
        break;

    case UI_DATA_TYPE_S32:
        {
        PC_UI_CONTROL_S32 p = (PC_UI_CONTROL_S32) p_ui_control;
        if(inc)
            p_ui_data->s32 += (p && p->bump_val) ? p->bump_val : 1;
        else
            p_ui_data->s32 -= (p && p->bump_val) ? p->bump_val : 1;
        break;
        }
    }

    if(bcheck)
        status_return(ui_data_bound_check(ui_data_type, p_ui_data, p_ui_control));

    return(STATUS_OK);
}

_Check_return_
extern S32
ui_data_n_items_query(
    _InVal_     UI_DATA_TYPE ui_data_type,
    _InRef_     PC_UI_SOURCE p_ui_source)
{
    IGNOREPARM_InVal_(ui_data_type);

    switch(p_ui_source->type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_SOURCE_TYPE_NONE:
#endif
        return(0);

    case UI_SOURCE_TYPE_SINGLE:
        return(1);

    case UI_SOURCE_TYPE_ARRAY:
        return(array_elements(&p_ui_source->source.array_handle));

    case UI_SOURCE_TYPE_LIST:
        return(list_numitem(p_ui_source->source.p_list_block));

#ifdef UI_SOURCE_TYPE_PROCS
    case UI_SOURCE_TYPE_PROC:
        return((* p_ui_source->source.proc.proc) (p_ui_source->source.proc.ui_source_handle, -1));
#endif
    }
}

/******************************************************************************
*
* query a UI data item
*
* the data item returned must be ui_data_dispose()'d of
*
******************************************************************************/

_Check_return_
extern STATUS
ui_data_query(
    _InoutRef_  P_UI_DATA_TYPE p_ui_data_type,
    _InRef_     PC_UI_SOURCE p_ui_source,
    _InVal_     S32 itemno,
    _OutRef_    P_UI_DATA p_ui_data)
{
    S32 itemsize = ui_bytes_per_item(*p_ui_data_type);
    PC_BYTE p_item;

    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_ui_data)); /* all valid paths set some member */

    switch(p_ui_source->type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_SOURCE_TYPE_NONE:
#endif
        return(STATUS_OK);

    case UI_SOURCE_TYPE_SINGLE:
        switch(*p_ui_data_type)
        {
        case UI_DATA_TYPE_TEXT:
            return(ui_text_copy(&p_ui_data->text, &p_ui_source->source.single_p_ui_data->text));

        default:
            memcpy32(p_ui_data, p_ui_source->source.single_p_ui_data, itemsize);
            return(STATUS_OK);
        }

    case UI_SOURCE_TYPE_ARRAY:
        if(!p_ui_source->source.array_handle || (itemno < 0) || (itemno >= array_elements(&p_ui_source->source.array_handle)))
        {
            assert(p_ui_source->source.array_handle);
            assert(itemno >= 0);
          /*assert(itemno < array_elements(&p_ui_source->source.array_handle)); don't whinge about this */
            *p_ui_data_type = UI_DATA_TYPE_NONE;
            return(STATUS_OK);
        }

        /* use size stored in handle to reference items */
        p_item = array_basec(&p_ui_source->source.array_handle, BYTE) + (itemno * array_element_size32(&p_ui_source->source.array_handle));
        break;

    case UI_SOURCE_TYPE_LIST:
        assert(itemno >= 0);
        p_item = collect_goto_item(BYTE, p_ui_source->source.p_list_block, (LIST_ITEMNO) itemno);

        if(NULL == p_item)
        {
            PTR_ASSERT(p_item);
            *p_ui_data_type = UI_DATA_TYPE_NONE;
            return(STATUS_OK);
        }

        break;

#ifdef UI_SOURCE_TYPE_PROCS
    case UI_SOURCE_TYPE_PROC:
        return((* p_ui_source->source.proc.proc) (p_ui_source->source.proc.ui_source_handle, itemno, p_ui_data));
#endif
    }

    switch(*p_ui_data_type)
    {
    case UI_DATA_TYPE_TEXT:
        return(ui_text_copy(&p_ui_data->text, (PC_UI_TEXT) p_item));

    default:
        memcpy32(p_ui_data, p_item, itemsize);
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* query a UI data item and convert to text all in one go
*
******************************************************************************/

_Check_return_
extern STATUS
ui_data_query_as_text(
    _InVal_     UI_DATA_TYPE ui_data_type_in,
    _InRef_     PC_UI_SOURCE p_ui_source,
    _InVal_     S32 itemno,
    /*in*/      const void /*UI_CONTROL*/ * const p_ui_control,
    _OutRef_    P_UI_TEXT p_ui_text)
{
    UI_DATA_TYPE ui_data_type = ui_data_type_in;
    UI_DATA ui_data;
    STATUS status;

    if(status_fail(status = ui_data_query(&ui_data_type, p_ui_source, itemno, &ui_data)))
    {
        zero_struct_ptr(p_ui_text);
        return(status);
    }

    status = ui_text_from_data(ui_data_type, &ui_data, p_ui_control, p_ui_text);

    ui_data_dispose(ui_data_type, &ui_data);

    return(status);
}

/******************************************************************************
*
* frequently used dialog box access routines
*
******************************************************************************/

extern void
dialog_cmd_process_dbox_setup(
    _OutRef_    P_DIALOG_CMD_PROCESS_DBOX p_dialog_cmd_process_dbox,
    _In_reads_opt_(n_ctls) P_DIALOG_CTL_CREATE p_ctl_create,
    _InVal_     U32 n_ctls,
    _InVal_     STATUS help_topic_resource_id)
{
    zero_struct_ptr(p_dialog_cmd_process_dbox);

    p_dialog_cmd_process_dbox->p_ctl_create = p_ctl_create;
    p_dialog_cmd_process_dbox->n_ctls = n_ctls;
    p_dialog_cmd_process_dbox->help_topic_resource_id = help_topic_resource_id;

    p_dialog_cmd_process_dbox->caption.type = UI_TEXT_TYPE_RESID; /* most generally useful */
}

extern void
ui_dlg_ctl_enable(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id,
    _InVal_     BOOL enabled)
{
    DIALOG_CMD_CTL_ENABLE dialog_cmd_ctl_enable;
    msgclr(dialog_cmd_ctl_enable);
    dialog_cmd_ctl_enable.h_dialog = h_dialog;
    dialog_cmd_ctl_enable.control_id = control_id;
    dialog_cmd_ctl_enable.enabled = enabled;
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_ENABLE, &dialog_cmd_ctl_enable));
}

extern void
ui_dlg_ctl_encode(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id)
{
    DIALOG_CMD_CTL_ENCODE dialog_cmd_ctl_encode;
    msgclr(dialog_cmd_ctl_encode);
    dialog_cmd_ctl_encode.h_dialog = h_dialog;
    dialog_cmd_ctl_encode.control_id = control_id;
    dialog_cmd_ctl_encode.bits = 0;
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_ENCODE, &dialog_cmd_ctl_encode));
}

extern void
ui_dlg_ctl_size_estimate(
    P_DIALOG_CMD_CTL_SIZE_ESTIMATE p_dialog_cmd_ctl_size_estimate)
{
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_SIZE_ESTIMATE, p_dialog_cmd_ctl_size_estimate));
}

extern void
ui_dlg_get_f64(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id,
    /*out*/ P_F64 p_f64)
{
    DIALOG_CMD_CTL_STATE_QUERY dialog_cmd_ctl_state_query;
    msgclr(dialog_cmd_ctl_state_query);
    dialog_cmd_ctl_state_query.h_dialog = h_dialog;
    dialog_cmd_ctl_state_query.control_id = control_id;
    dialog_cmd_ctl_state_query.bits = 0;
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    *p_f64 = dialog_cmd_ctl_state_query.state.bump_f64;
}

_Check_return_
extern S32
ui_dlg_get_s32(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id)
{
    DIALOG_CMD_CTL_STATE_QUERY dialog_cmd_ctl_state_query;
    msgclr(dialog_cmd_ctl_state_query);
    dialog_cmd_ctl_state_query.h_dialog = h_dialog;
    dialog_cmd_ctl_state_query.control_id = control_id;
    dialog_cmd_ctl_state_query.bits = 0;
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    return(dialog_cmd_ctl_state_query.state.bump_s32);
}

_Check_return_
extern BOOL
ui_dlg_get_check(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id)
{
    DIALOG_CMD_CTL_STATE_QUERY dialog_cmd_ctl_state_query;
    msgclr(dialog_cmd_ctl_state_query);
    dialog_cmd_ctl_state_query.h_dialog = h_dialog;
    dialog_cmd_ctl_state_query.control_id = control_id;
    dialog_cmd_ctl_state_query.bits = 0;
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    return(dialog_cmd_ctl_state_query.state.checkbox == DIALOG_BUTTONSTATE_ON);
}

extern void
ui_dlg_get_edit(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id,
    _OutRef_    P_UI_TEXT p_ui_text)
{
    DIALOG_CMD_CTL_STATE_QUERY dialog_cmd_ctl_state_query;
    msgclr(dialog_cmd_ctl_state_query);
    dialog_cmd_ctl_state_query.h_dialog = h_dialog;
    dialog_cmd_ctl_state_query.control_id = control_id;
    dialog_cmd_ctl_state_query.bits = 0;
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    *p_ui_text = dialog_cmd_ctl_state_query.state.edit.ui_text;
}

_Check_return_
extern S32 /*radio_state*/
ui_dlg_get_radio(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id)
{
    DIALOG_CMD_CTL_STATE_QUERY dialog_cmd_ctl_state_query;
    msgclr(dialog_cmd_ctl_state_query);
    dialog_cmd_ctl_state_query.h_dialog = h_dialog;
    dialog_cmd_ctl_state_query.control_id = control_id;
    dialog_cmd_ctl_state_query.bits = 0;
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    return(dialog_cmd_ctl_state_query.state.radiobutton);
}

_Check_return_
extern S32
ui_dlg_get_list_idx(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id)
{
    DIALOG_CMD_CTL_STATE_QUERY dialog_cmd_ctl_state_query;
    msgclr(dialog_cmd_ctl_state_query);
    dialog_cmd_ctl_state_query.h_dialog = h_dialog;
    dialog_cmd_ctl_state_query.control_id = control_id;
    dialog_cmd_ctl_state_query.bits = DIALOG_STATE_QUERY_ALTERNATE;
    dialog_cmd_ctl_state_query.state.list_text.itemno = -1;
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    return(dialog_cmd_ctl_state_query.state.list_text.itemno);
}

extern void
ui_dlg_get_list_text(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id,
    _OutRef_    P_UI_TEXT p_ui_text)
{
    DIALOG_CMD_CTL_STATE_QUERY dialog_cmd_ctl_state_query;
    msgclr(dialog_cmd_ctl_state_query);
    dialog_cmd_ctl_state_query.h_dialog = h_dialog;
    dialog_cmd_ctl_state_query.control_id = control_id;
    dialog_cmd_ctl_state_query.bits = 0;
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    *p_ui_text = dialog_cmd_ctl_state_query.state.list_text.ui_text;
}

extern void
ui_dlg_ctl_new_source(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id)
{
    DIALOG_CMD_CTL_NEW_SOURCE dialog_cmd_ctl_new_source;
    msgclr(dialog_cmd_ctl_new_source);
    dialog_cmd_ctl_new_source.h_dialog = h_dialog;
    dialog_cmd_ctl_new_source.control_id = control_id;
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_NEW_SOURCE, &dialog_cmd_ctl_new_source));
}

_Check_return_
extern STATUS
ui_dlg_set_f64(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id,
    _InRef_     PC_F64 p_f64)
{
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = h_dialog;
    dialog_cmd_ctl_state_set.control_id = control_id;
    dialog_cmd_ctl_state_set.bits = 0;
    dialog_cmd_ctl_state_set.state.bump_f64 = *p_f64;
    return(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
}

_Check_return_
extern STATUS
ui_dlg_set_s32(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id,
    _InVal_     S32 s32)
{
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = h_dialog;
    dialog_cmd_ctl_state_set.control_id = control_id;
    dialog_cmd_ctl_state_set.bits = 0;
    dialog_cmd_ctl_state_set.state.bump_s32 = s32;
    return(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
}

_Check_return_
extern STATUS
ui_dlg_set_check(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id,
    _InVal_     BOOL onoff)
{
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = h_dialog;
    dialog_cmd_ctl_state_set.control_id = control_id;
    dialog_cmd_ctl_state_set.bits = 0;
    dialog_cmd_ctl_state_set.state.checkbox = (U8) (onoff ? DIALOG_BUTTONSTATE_ON : DIALOG_BUTTONSTATE_OFF);
    return(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
}

_Check_return_
extern STATUS
ui_dlg_set_check_forcing(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id,
    _InVal_     BOOL onoff)
{
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = h_dialog;
    dialog_cmd_ctl_state_set.control_id = control_id;
    dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_ALWAYS_MSG;
    dialog_cmd_ctl_state_set.state.checkbox = (U8) (onoff ? DIALOG_BUTTONSTATE_ON : DIALOG_BUTTONSTATE_OFF);
    return(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
}

_Check_return_
extern STATUS
ui_dlg_set_edit(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id,
    _InRef_     PC_UI_TEXT p_ui_text)
{
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = h_dialog;
    dialog_cmd_ctl_state_set.control_id = control_id;
    dialog_cmd_ctl_state_set.bits = 0;
    if(P_DATA_NONE != p_ui_text)
        dialog_cmd_ctl_state_set.state.edit.ui_text = *p_ui_text;
    else
        dialog_cmd_ctl_state_set.state.edit.ui_text.type = UI_TEXT_TYPE_NONE;
    return(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
}

_Check_return_
extern STATUS
ui_dlg_set_list_idx(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id,
    _InVal_     S32 itemno)
{
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = h_dialog;
    dialog_cmd_ctl_state_set.control_id = control_id;
    dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_ALTERNATE;
    dialog_cmd_ctl_state_set.state.list_text.itemno = itemno;
    return(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
}

_Check_return_
extern STATUS
ui_dlg_set_radio(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id,
    _InVal_     S32 radio_state)
{
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = h_dialog;
    dialog_cmd_ctl_state_set.control_id = control_id;
    dialog_cmd_ctl_state_set.bits = 0;
    dialog_cmd_ctl_state_set.state.radiobutton = radio_state;
    return(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
}

_Check_return_
extern STATUS
ui_dlg_set_radio_forcing(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id,
    _InVal_     S32 radio_state)
{
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = h_dialog;
    dialog_cmd_ctl_state_set.control_id = control_id;
    dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_ALWAYS_MSG;
    dialog_cmd_ctl_state_set.state.radiobutton = radio_state;
    return(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
}

_Check_return_
extern STATUS
ui_dlg_ctl_set_default(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CTL_ID control_id)
{
    DIALOG_CMD_CTL_SET_DEFAULT dialog_cmd_ctl_set_default;
    msgclr(dialog_cmd_ctl_set_default);
    dialog_cmd_ctl_set_default.h_dialog = h_dialog;
    dialog_cmd_ctl_set_default.control_id = control_id;
    return(call_dialog(DIALOG_CMD_CODE_CTL_SET_DEFAULT, &dialog_cmd_ctl_set_default));
}

_Check_return_
extern S32
ui_dlg_s32_from_f64(
    _InRef_     PC_F64 p_f64,
    _InRef_     PC_F64 p_multiplier,
    _InVal_     S32 s32_min,
    _InVal_     S32 s32_max)
{
    F64 f64 = *p_f64;
    S32 s32 = 0;
    errno = 0;
    if(NULL != p_multiplier)
        f64 *= *p_multiplier;
    f64 += 0.5;
    if(f64 > (F64) s32_max)
    {
        errno = ERANGE;
        s32 = s32_max;
    }
    else if(f64 < (F64) s32_min)
    {
        errno = ERANGE;
        s32 = s32_min;
    }
    else
        s32 = (S32) f64;
    return(s32);
}

/******************************************************************************
*
* named style list for UI
*
******************************************************************************/

/******************************************************************************
*
* oh, the joys of sorting ARRAY_QUICK_BLOCKs...
*
******************************************************************************/

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, ui_list_create_style_sort, UI_LIST_ENTRY_STYLE)
{
    QSORT_ARG1_VAR_DECL(P_UI_LIST_ENTRY_STYLE, p_ui_list_entry_style_1);
    QSORT_ARG2_VAR_DECL(P_UI_LIST_ENTRY_STYLE, p_ui_list_entry_style_2);

    P_QUICK_TBLOCK p_quick_tblock_1 = &p_ui_list_entry_style_1->quick_tblock;
    P_QUICK_TBLOCK p_quick_tblock_2 = &p_ui_list_entry_style_2->quick_tblock;

    PCTSTR tstr_1 = (0 != quick_tblock_array_handle_ref(p_quick_tblock_1)) ? array_tstr(&quick_tblock_array_handle_ref(p_quick_tblock_1)) : (PCTSTR) (p_quick_tblock_1 + 1);
    PCTSTR tstr_2 = (0 != quick_tblock_array_handle_ref(p_quick_tblock_2)) ? array_tstr(&quick_tblock_array_handle_ref(p_quick_tblock_2)) : (PCTSTR) (p_quick_tblock_2 + 1);

    return(tstr_compare_nocase(tstr_1, tstr_2));
}

_Check_return_
extern STATUS
ui_list_create_style(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_array_handle,
    _OutRef_    P_UI_SOURCE p_ui_source,
    _OutRef_    P_PIXIT p_max_width)
{
    P_UI_LIST_ENTRY_STYLE p_ui_list_entry_style;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(*p_ui_list_entry_style), TRUE);
    STATUS status = STATUS_OK;
    PIXIT max_width = DIALOG_SYSCHAR_H;

    *p_array_handle = 0;

    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_ui_source));
    p_ui_source->type = UI_SOURCE_TYPE_NONE;

    {
    STYLE_HANDLE style_handle = STYLE_HANDLE_ENUM_START;
    P_STYLE p_style;

    while(style_enum_styles(p_docu, &p_style, &style_handle) >= 0)
    {
        PCTSTR tstr_style_name;

        if(!style_bit_test(p_style, STYLE_SW_NAME))
            continue;

        tstr_style_name = array_tstr(&p_style->h_style_name_tstr);

        if(*tstr_style_name < CH_SPACE) /* don't list wacky styles in UI */
            continue;

        if(NULL != (p_ui_list_entry_style = al_array_extend_by(p_array_handle, UI_LIST_ENTRY_STYLE, 1, &array_init_block, &status)))
        {
            const PIXIT width = ui_width_from_tstr(tstr_style_name);
            max_width = MAX(max_width, width);

            quick_tblock_with_buffer_setup(p_ui_list_entry_style->quick_tblock);

            status = quick_tblock_tchars_add(&p_ui_list_entry_style->quick_tblock, tstr_style_name, tstrlen32p1(tstr_style_name));
        }

        status_break(status);
    }
    } /*block*/

    *p_max_width = max_width;

    /* now sort the style list source into char order for neat display */
    ui_source_list_fixup_tb(p_array_handle, offsetof32(UI_LIST_ENTRY_STYLE, quick_tblock));
    al_array_qsort(p_array_handle, ui_list_create_style_sort);
    ui_source_list_fixup_tb(p_array_handle, offsetof32(UI_LIST_ENTRY_STYLE, quick_tblock));

    /* make a source of text pointers to these elements for list box processing */
    if(status_ok(status))
        status = ui_source_create_tb(p_array_handle, p_ui_source, UI_TEXT_TYPE_TSTR_PERM, offsetof32(UI_LIST_ENTRY_STYLE, quick_tblock));

    if(status_fail(status))
        ui_list_dispose_style(p_array_handle, p_ui_source);

    return(status);
}

/******************************************************************************
*
* typeface list for UI
*
******************************************************************************/

/******************************************************************************
*
* oh, the joys of sorting ARRAY_QUICK_TBLOCKs...
*
******************************************************************************/

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, ui_list_create_typeface_sort, UI_LIST_ENTRY_TYPEFACE)
{
    QSORT_ARG1_VAR_DECL(P_UI_LIST_ENTRY_TYPEFACE, p_ui_list_entry_typeface_1);
    QSORT_ARG2_VAR_DECL(P_UI_LIST_ENTRY_TYPEFACE, p_ui_list_entry_typeface_2);

    P_QUICK_TBLOCK p_quick_tblock_1 = &p_ui_list_entry_typeface_1->quick_tblock;
    P_QUICK_TBLOCK p_quick_tblock_2 = &p_ui_list_entry_typeface_2->quick_tblock;

    PCTSTR tstr_1 = (0 != quick_tblock_array_handle_ref(p_quick_tblock_1)) ? array_tstr(&quick_tblock_array_handle_ref(p_quick_tblock_1)) : (PCTSTR) (p_quick_tblock_1 + 1);
    PCTSTR tstr_2 = (0 != quick_tblock_array_handle_ref(p_quick_tblock_2)) ? array_tstr(&quick_tblock_array_handle_ref(p_quick_tblock_2)) : (PCTSTR) (p_quick_tblock_2 + 1);

    return(tstr_compare_nocase(tstr_1, tstr_2));
}

_Check_return_
extern STATUS
ui_list_create_typeface(
    _OutRef_    P_ARRAY_HANDLE p_array_handle,
    _OutRef_    P_UI_SOURCE p_ui_source)
{
    P_UI_LIST_ENTRY_TYPEFACE p_ui_list_entry_typeface;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(*p_ui_list_entry_typeface), TRUE);
    STATUS status = STATUS_OK;

    *p_array_handle = 0;

    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_ui_source));
    p_ui_source->type = UI_SOURCE_TYPE_NONE;

    { /* use available typefaces from fontmap */
    ARRAY_INDEX app_index;
    ARRAY_INDEX n = fontmap_host_base_names();

    for(app_index = 0; app_index < n; ++app_index)
    {
        ARRAY_HANDLE host_font_name;
        BITMAP(host_fonts_available, FONTMAP_N_BITS);

        status_assert(fontmap_host_base_name_from_app_index(&host_font_name, app_index));

        status_assert(fontmap_host_fonts_available(host_fonts_available, app_index));

        if(bitmap_any(host_fonts_available, N_BITS_ARG(FONTMAP_N_BITS)))
        {
            PCTSTR name = array_tstr(&host_font_name);

            if(NULL != (p_ui_list_entry_typeface = al_array_extend_by(p_array_handle, UI_LIST_ENTRY_TYPEFACE, 1, &array_init_block, &status)))
            {
                quick_tblock_with_buffer_setup(p_ui_list_entry_typeface->quick_tblock);

                status = quick_tblock_tchars_add(&p_ui_list_entry_typeface->quick_tblock, name, tstrlen32p1(name));
            }
        }

        al_array_dispose(&host_font_name);

        status_break(status);
    }
    } /*block*/

    /* now sort the typeface list source into alphabetical order for neat display */
    ui_source_list_fixup_tb(p_array_handle, offsetof32(UI_LIST_ENTRY_TYPEFACE, quick_tblock));
    al_array_qsort(p_array_handle, ui_list_create_typeface_sort);
    ui_source_list_fixup_tb(p_array_handle, offsetof32(UI_LIST_ENTRY_TYPEFACE, quick_tblock));

    /* make a source of text pointers to these elements for list box processing */
    if(status_ok(status))
        status = ui_source_create_tb(p_array_handle, p_ui_source, UI_TEXT_TYPE_TSTR_PERM, offsetof32(UI_LIST_ENTRY_TYPEFACE, quick_tblock));

    if(status_fail(status))
        ui_lists_dispose_tb(p_array_handle, p_ui_source, offsetof32(UI_LIST_ENTRY_TYPEFACE, quick_tblock));

    return(status);
}

extern void
ui_list_dispose_style(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InoutRef_  P_UI_SOURCE p_ui_source)
{
    ui_lists_dispose_tb(p_array_handle, p_ui_source, offsetof32(UI_LIST_ENTRY_STYLE, quick_tblock));
}

extern void
ui_lists_dispose(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InoutRef_  P_UI_SOURCE p_ui_source)
{
    if(p_ui_source->type != UI_SOURCE_TYPE_NONE)
    {
        p_ui_source->type = UI_SOURCE_TYPE_NONE;
        al_array_dispose(&p_ui_source->source.array_handle);
    }

    al_array_dispose(p_array_handle);
}

extern void
ui_lists_dispose_tb(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InoutRef_  P_UI_SOURCE p_ui_source,
    _InVal_     S32 quick_tblock_offset /*-1 -> none*/)
{
    if(quick_tblock_offset >= 0)
    {
        const U32 elem_size = array_element_size32(p_array_handle);
        P_BYTE ptr = array_base(p_array_handle, BYTE);
        const ARRAY_INDEX n_elements = array_elements(p_array_handle);
        ARRAY_INDEX i;

        ptr += quick_tblock_offset;

        for(i = 0; i < n_elements; ++i, ptr += elem_size)
            quick_tblock_dispose((P_QUICK_TBLOCK) ptr);
    }

    ui_lists_dispose(p_array_handle, p_ui_source);
}

extern void
ui_lists_dispose_ub(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InoutRef_  P_UI_SOURCE p_ui_source,
    _InVal_     S32 quick_ublock_offset /*-1 -> none*/)
{
    if(quick_ublock_offset >= 0)
    {
        const U32 elem_size = array_element_size32(p_array_handle);
        P_BYTE ptr = array_base(p_array_handle, BYTE);
        const ARRAY_INDEX n_elements = array_elements(p_array_handle);
        ARRAY_INDEX i;

        ptr += quick_ublock_offset;

        for(i = 0; i < n_elements; ++i, ptr += elem_size)
            quick_ublock_dispose((P_QUICK_UBLOCK) ptr);
    }

    ui_lists_dispose(p_array_handle, p_ui_source);
}

_Check_return_
extern STATUS
ui_source_create_tb(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _OutRef_    P_UI_SOURCE p_ui_source,
    _InVal_     UI_TEXT_TYPE ui_text_type,
    _InVal_     S32 quick_tblock_offset /*-1 -> none, or already fixed up, 0th is ptr*/)
{
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_ui_source));
    p_ui_source->type = UI_SOURCE_TYPE_NONE;

    { /* make a source of text pointers to these elements for list box processing */
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(UI_TEXT), FALSE);
    if(NULL == al_array_alloc_UI_TEXT(&p_ui_source->source.array_handle, array_elements(p_array_handle), &array_init_block, &status))
        return(status);
    } /*block*/

    p_ui_source->type = UI_SOURCE_TYPE_ARRAY;

    if(quick_tblock_offset >= 0)
        ui_source_list_fixup_tb(p_array_handle, quick_tblock_offset);

    {
    P_UI_TEXT p_ui_text = array_base(&p_ui_source->source.array_handle, UI_TEXT);
    const U32 elem_size = array_element_size32(p_array_handle);
    P_BYTE ptr = array_base(p_array_handle, BYTE);
    const ARRAY_INDEX n_elements = array_elements(p_array_handle);
    ARRAY_INDEX i;

    if(quick_tblock_offset >= 0)
        ptr += quick_tblock_offset;

    for(i = 0; i < n_elements; ++i, ptr += elem_size, ++p_ui_text)
    {
        p_ui_text->type = ui_text_type;
        p_ui_text->text.tstr = (quick_tblock_offset >= 0) ? quick_tblock_tstr((P_QUICK_TBLOCK) ptr) : (PTSTR) ptr;
    }
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
extern STATUS
ui_source_create_ub(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _OutRef_    P_UI_SOURCE p_ui_source,
    _InVal_     UI_TEXT_TYPE ui_text_type,
    _InVal_     S32 quick_ublock_offset /*-1 -> none, or already fixed up, 0th is ptr*/)
{
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_ui_source));
    p_ui_source->type = UI_SOURCE_TYPE_NONE;

    { /* make a source of text pointers to these elements for list box processing */
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(UI_TEXT), FALSE);
    if(NULL == al_array_alloc_UI_TEXT(&p_ui_source->source.array_handle, array_elements(p_array_handle), &array_init_block, &status))
        return(status);
    } /*block*/

    p_ui_source->type = UI_SOURCE_TYPE_ARRAY;

    if(quick_ublock_offset >= 0)
        ui_source_list_fixup_ub(p_array_handle, quick_ublock_offset);

    {
    P_UI_TEXT p_ui_text = array_base(&p_ui_source->source.array_handle, UI_TEXT);
    const U32 elem_size = array_element_size32(p_array_handle);
    P_BYTE ptr = array_base(p_array_handle, BYTE);
    const ARRAY_INDEX n_elements = array_elements(p_array_handle);
    ARRAY_INDEX i;

    if(quick_ublock_offset >= 0)
        ptr += quick_ublock_offset;

    for(i = 0; i < n_elements; ++i, ptr += elem_size, ++p_ui_text)
    {
        p_ui_text->type = ui_text_type;
        p_ui_text->text.ustr = (quick_ublock_offset >= 0) ? quick_ublock_ustr((P_QUICK_UBLOCK) ptr) : (P_USTR) ptr;
    }
    } /*block*/

    return(STATUS_OK);
}

/*
NB anything with ARRAY_QUICK_xBLOCKs in it must be got rid of using ui_lists_dispose_xb()...
*/

extern void
ui_source_dispose(
    _InoutRef_  P_UI_SOURCE p_ui_source)
{
    if(UI_SOURCE_TYPE_ARRAY == p_ui_source->type)
    {
        ARRAY_INDEX i = array_elements(&p_ui_source->source.array_handle);

        while(--i >= 0)
        {
            P_UI_TEXT p_ui_text = array_ptr(&p_ui_source->source.array_handle, UI_TEXT, i);
            ui_text_dispose(p_ui_text);
        }

        al_array_dispose(&p_ui_source->source.array_handle);
    }

    p_ui_source->type = UI_SOURCE_TYPE_NONE;
}

extern void
ui_source_list_fixup_tb(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InVal_     S32 quick_tblock_offset)
{
    const U32 elem_size = array_element_size32(p_array_handle);
    P_BYTE ptr = array_base(p_array_handle, BYTE);
    const ARRAY_INDEX n_elements = array_elements(p_array_handle);
    ARRAY_INDEX i;

    assert(quick_tblock_offset >= 0);
    ptr += quick_tblock_offset;

    for(i = 0; i < n_elements; ++i, ptr += elem_size)
    {
        /* fixup static buffer address */
        P_QUICK_TBLOCK p_quick_tblock = (P_QUICK_TBLOCK) ptr;
        p_quick_tblock->tb_p_static_buffer = (PTCH) (p_quick_tblock + 1); /* buffer MUST be immediately after the block */
    }
}

extern void
ui_source_list_fixup_ub(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InVal_     S32 quick_ublock_offset)
{
    const U32 elem_size = array_element_size32(p_array_handle);
    P_BYTE ptr = array_base(p_array_handle, BYTE);
    const ARRAY_INDEX n_elements = array_elements(p_array_handle);
    ARRAY_INDEX i;

    assert(quick_ublock_offset >= 0);
    ptr += quick_ublock_offset;

    for(i = 0; i < n_elements; ++i, ptr += elem_size)
    {
        /* fixup static buffer address */
        P_QUICK_UBLOCK p_quick_ublock = (P_QUICK_UBLOCK) ptr;
        p_quick_ublock->ub_p_static_buffer = (P_UCHARS) (p_quick_ublock + 1); /* buffer MUST be immediately after the block */
    }
}

extern void
ui_string_to_text(
    _DocuRef_   P_DOCU p_docu,
    _Inout_updates_z_(elemof_buffer) PTSTR tstr_buf /*appended*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR src)
{
    PTSTR dst = tstr_buf;
    U32 remain = elemof_buffer;
    U32 len;

    len = tstrlen32(dst); /* always point to the CH_NULL */
    dst += len;
    remain -= len;

    for(;;) /* loop attempting to printf-like the string */
    {
        PCTSTR tstr = tstrchr(src, CH_PERCENT_SIGN);

        if(!tstr)
            break;

        /* copy current fragment */
        len = PtrDiffElemU32(tstr, src);
        if(len && (len <= remain))
        {
            memcpy32(dst, src, len * sizeof32(*dst));
            dst += len;
            remain -= len;
        }

        ++tstr;

        switch(*tstr++)
        {
        default: default_unhandled();
#if CHECKING
        case CH_NULL:
        case CH_PERCENT_SIGN:
#endif
            *dst++ = CH_PERCENT_SIGN;
            remain--;
            break;

        case 'P': /* product id */
            tstr_xstrkpy(dst, remain, product_ui_id());
            dst += (len = tstrlen32(dst));
            remain -= len;
            break;

        case 'L': /* leafname */
            {
            PC_DOCU_NAME p_docu_name = &p_docu->docu_name;

            tstr_xstrkpy(dst, remain, p_docu_name->leaf_name);
            dst += (len = tstrlen32(dst));
            remain -= len;

            if(NULL != p_docu_name->extension)
            {
                tstr_xstrkpy(dst, remain, FILE_EXT_SEP_TSTR);
                dst += (len = tstrlen32(dst));
                remain -= len;

                tstr_xstrkpy(dst, remain, p_docu_name->extension);
                dst += (len = tstrlen32(dst));
                remain -= len;
            }

            break;
            }

        case 'W': /* wholename */
            {
            STATUS status;
            PC_DOCU_NAME p_docu_name = &p_docu->docu_name;
            QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
            quick_tblock_with_buffer_setup(quick_tblock);

            status = name_make_wholename(p_docu_name, &quick_tblock, TRUE);

            if(status_ok(status))
            {
                tstr_xstrkpy(dst, remain, quick_tblock_tstr(&quick_tblock));
                dst += (len = tstrlen32(dst));
                remain -= len;
            }

            quick_tblock_dispose(&quick_tblock);
            break;
            }
        }

        src = tstr;
    }

    /* copy remaining fragment */
    len = tstrlen32(src);
    if(0 != len)
    {
        len = MIN(len, remain);
        memcpy32(dst, src, len * sizeof32(*dst));
        dst += len;
        remain -= len;
    }

    if(0 == remain)
        --dst; /* sorry, chaps */

    *dst = CH_NULL; /* terminate */
}

/******************************************************************************
*
* make a UI_TEXT from a copy of a string
*
******************************************************************************/

_Check_return_
extern STATUS
ui_text_alloc_from_tstr(
    _OutRef_    P_UI_TEXT p_dst_ui_text,
    _In_opt_z_  PCTSTR tstr)
{
    p_dst_ui_text->type = UI_TEXT_TYPE_NONE;

    if(NULL == tstr)
        return(STATUS_OK);

    if(CH_NULL == *tstr)
    {
        p_dst_ui_text->type = UI_TEXT_TYPE_TSTR_PERM;
        p_dst_ui_text->text.tstr = tstr_empty_string;
        return(STATUS_OK);
    }

    status_return(tstr_set(&p_dst_ui_text->text.tstr_wr, tstr));
    p_dst_ui_text->type = UI_TEXT_TYPE_TSTR_ALLOC;
    return(STATUS_OK);
}

_Check_return_
extern STATUS
ui_text_alloc_from_ustr(
    _OutRef_    P_UI_TEXT p_dst_ui_text,
    _In_opt_z_  PC_USTR ustr)
{
    p_dst_ui_text->type = UI_TEXT_TYPE_NONE;

    if(NULL == ustr)
        return(STATUS_OK);

    if(CH_NULL == PtrGetByte(ustr))
    {
        p_dst_ui_text->type = UI_TEXT_TYPE_TSTR_PERM;
        p_dst_ui_text->text.tstr = tstr_empty_string;
        return(STATUS_OK);
    }

    status_return(ustr_set(&p_dst_ui_text->text.ustr_wr, ustr));
    p_dst_ui_text->type = UI_TEXT_TYPE_USTR_ALLOC;
    return(STATUS_OK);
}

_Check_return_
extern STATUS
ui_text_alloc_from_p_ev_string(
    _OutRef_    P_UI_TEXT p_dst_ui_text,
    _InRef_opt_ PC_EV_STRINGC p_ev_string)
{
    p_dst_ui_text->type = UI_TEXT_TYPE_NONE;

    if((NULL == p_ev_string) || (NULL == p_ev_string->uchars))
        return(STATUS_OK);

    if(0 == p_ev_string->size)
    {
        p_dst_ui_text->type = UI_TEXT_TYPE_TSTR_PERM;
        p_dst_ui_text->text.tstr = tstr_empty_string;
        return(STATUS_OK);
    }

    status_return(ustr_set_n(&p_dst_ui_text->text.ustr_wr, p_ev_string->uchars, p_ev_string->size));
    p_dst_ui_text->type = UI_TEXT_TYPE_USTR_ALLOC;
    return(STATUS_OK);
}

_Check_return_
extern BOOL
ui_text_is_blank(
    _InRef_opt_ PC_UI_TEXT p_ui_text)
{
    BOOL res = TRUE;

    if(NULL == p_ui_text)
        return(TRUE);

    switch(p_ui_text->type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_TEXT_TYPE_NONE:
#endif
        break;

    case UI_TEXT_TYPE_RESID:
        if(0 == p_ui_text->text.resource_id)
            break;

        {
        QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
        quick_tblock_with_buffer_setup(quick_tblock);

        status_assert(resource_lookup_quick_tblock_no_default(&quick_tblock, p_ui_text->text.resource_id));

        res = (0 == quick_tblock_chars(&quick_tblock) || !*quick_tblock_tstr(&quick_tblock));

        quick_tblock_dispose(&quick_tblock);

        break;
        } /*block*/

    case UI_TEXT_TYPE_USTR_PERM:
    case UI_TEXT_TYPE_USTR_TEMP:
    case UI_TEXT_TYPE_USTR_ALLOC:
        res = ((NULL == p_ui_text->text.ustr) || (CH_NULL == PtrGetByte(p_ui_text->text.ustr)));
        break;

    case UI_TEXT_TYPE_USTR_ARRAY:
        res = (0 == array_elements(&p_ui_text->text.array_handle_ustr) || (CH_NULL == PtrGetByte(array_ustr(&p_ui_text->text.array_handle_ustr))));
        break;

#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_PERM:
    case UI_TEXT_TYPE_TSTR_TEMP:
    case UI_TEXT_TYPE_TSTR_ALLOC:
        res = ((NULL == p_ui_text->text.tstr) || (CH_NULL == *p_ui_text->text.tstr));
        break;

    case UI_TEXT_TYPE_TSTR_ARRAY:
        res = (0 == array_elements(&p_ui_text->text.array_handle_tstr) || (CH_NULL == *array_tstr(&p_ui_text->text.array_handle_tstr)));
        break;
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */
    }

    return(res);
}

/******************************************************************************
*
* compare UI textual representations
*
******************************************************************************/

_Check_return_
extern int
ui_text_compare(
    _InRef_     PC_UI_TEXT p_ui_text_1,
    _InRef_     PC_UI_TEXT p_ui_text_2,
    _InVal_     BOOL fussy,
    _InVal_     BOOL insensitive)
{
    if(p_ui_text_1->type == p_ui_text_2->type)
    {
        switch(p_ui_text_1->type)
        {
        case UI_TEXT_TYPE_NONE:
            return(0);

        case UI_TEXT_TYPE_RESID:
            if(p_ui_text_1->text.resource_id == p_ui_text_2->text.resource_id)
                return(0);

            if(!fussy)
                return(1);

            break;

        default:
            break;
        }
    }

    {
    STATUS res;
    PCTSTR p1 = PTSTR_NONE;
    PCTSTR p2 = PTSTR_NONE;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock_1, 256); /* change to UBLOCK, UTF-8 in due course */
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock_2, 256);
    quick_tblock_with_buffer_setup(quick_tblock_1);
    quick_tblock_with_buffer_setup(quick_tblock_2);

    switch(p_ui_text_1->type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_TEXT_TYPE_NONE:
#endif
        break;

    case UI_TEXT_TYPE_RESID:
        status_assert(resource_lookup_quick_tblock_no_default(&quick_tblock_1, p_ui_text_1->text.resource_id));
        if(quick_tblock_chars(&quick_tblock_1) && status_ok(quick_tblock_nullch_add(&quick_tblock_1)))
            p1 = quick_tblock_tstr(&quick_tblock_1);
        break;

    case UI_TEXT_TYPE_USTR_PERM:
    case UI_TEXT_TYPE_USTR_TEMP:
    case UI_TEXT_TYPE_USTR_ALLOC:
        if(p_ui_text_1->text.ustr)
            p1 = _tstr_from_ustr(p_ui_text_1->text.ustr);
        break;

    case UI_TEXT_TYPE_USTR_ARRAY:
        if(array_elements(&p_ui_text_1->text.array_handle_ustr) > 0)
            p1 = _tstr_from_ustr(array_ustr(&p_ui_text_1->text.array_handle_ustr));
        break;

#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_PERM:
    case UI_TEXT_TYPE_TSTR_TEMP:
    case UI_TEXT_TYPE_TSTR_ALLOC:
        if(p_ui_text_1->text.tstr)
            p1 = p_ui_text_1->text.tstr;
        break;

    case UI_TEXT_TYPE_TSTR_ARRAY:
        if(array_elements(&p_ui_text_1->text.array_handle_tstr) > 0)
            p1 = array_tstr(&p_ui_text_1->text.array_handle_tstr);
        break;
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */
    }

    switch(p_ui_text_2->type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_TEXT_TYPE_NONE:
#endif
        break;

    case UI_TEXT_TYPE_RESID:
        status_assert(resource_lookup_quick_tblock_no_default(&quick_tblock_2, p_ui_text_2->text.resource_id));
        if(quick_tblock_chars(&quick_tblock_2) && status_ok(quick_tblock_nullch_add(&quick_tblock_2)))
            p2 = quick_tblock_tstr(&quick_tblock_2);
        break;

    case UI_TEXT_TYPE_USTR_PERM:
    case UI_TEXT_TYPE_USTR_TEMP:
    case UI_TEXT_TYPE_USTR_ALLOC:
        if(p_ui_text_2->text.ustr)
            p2 = _tstr_from_ustr(p_ui_text_2->text.ustr);
        break;

    case UI_TEXT_TYPE_USTR_ARRAY:
        if(array_elements(&p_ui_text_2->text.array_handle_ustr) > 0)
            p2 = _tstr_from_ustr(array_ustr(&p_ui_text_2->text.array_handle_ustr));
        break;

#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_PERM:
    case UI_TEXT_TYPE_TSTR_TEMP:
    case UI_TEXT_TYPE_TSTR_ALLOC:
        if(p_ui_text_2->text.tstr)
            p2 = p_ui_text_2->text.tstr;
        break;

    case UI_TEXT_TYPE_TSTR_ARRAY:
        if(array_elements(&p_ui_text_2->text.array_handle_tstr) > 0)
            p2 = array_tstr(&p_ui_text_2->text.array_handle_tstr);
        break;
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */
    }

    if(p1 == p2)
        res =  0;
    else if(P_DATA_NONE == p1)
        res = +1;
    else if(P_DATA_NONE == p2)
        res = -1;
    else if(insensitive)
        res = tstr_compare_nocase(p1, p2);
    else
        res = tstrcmp(p1, p2);

    quick_tblock_dispose(&quick_tblock_1);
    quick_tblock_dispose(&quick_tblock_2);

    return(res);
    } /*block*/
}

/******************************************************************************
*
* copy a UI textual representation
*
******************************************************************************/

_Check_return_
extern STATUS
ui_text_copy(
    _OutRef_    P_UI_TEXT p_dst_ui_text,
    _InRef_     PC_UI_TEXT p_src_ui_text)
{
    switch(p_src_ui_text->type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_TEXT_TYPE_NONE:
    case UI_TEXT_TYPE_RESID:
    case UI_TEXT_TYPE_USTR_PERM:
#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_PERM:
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */
#endif
        /* simple duplication */
        *p_dst_ui_text = *p_src_ui_text;
        return(STATUS_OK);

    case UI_TEXT_TYPE_USTR_TEMP:
    case UI_TEXT_TYPE_USTR_ALLOC:
        /* alloced strings must be copied */
        return(ui_text_alloc_from_ustr(p_dst_ui_text, p_src_ui_text->text.ustr));

#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_TEMP:
    case UI_TEXT_TYPE_TSTR_ALLOC:
        /* alloced strings must be copied */
        return(ui_text_alloc_from_tstr(p_dst_ui_text, p_src_ui_text->text.tstr));
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */

    case UI_TEXT_TYPE_USTR_ARRAY:
#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_ARRAY:
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */
        /* handled strings in arrays must be copied */
        (void) al_array_duplicate(&p_dst_ui_text->text.array_handle_ustr, &p_src_ui_text->text.array_handle_ustr);
        if(0 != p_dst_ui_text->text.array_handle_ustr)
        {
            p_dst_ui_text->type = p_src_ui_text->type;
            return(STATUS_OK);
        }
        break;
    }

    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_dst_ui_text));
    p_dst_ui_text->type = UI_TEXT_TYPE_NONE;
    return(status_nomem());
}

/******************************************************************************
*
* dispose of a UI textual representation
*
******************************************************************************/

extern void
ui_text_dispose(
    _InoutRef_  P_UI_TEXT p_ui_text)
{
    switch(p_ui_text->type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_TEXT_TYPE_NONE:
    case UI_TEXT_TYPE_RESID:
    case UI_TEXT_TYPE_USTR_PERM:
    case UI_TEXT_TYPE_USTR_TEMP:
#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_PERM:
    case UI_TEXT_TYPE_TSTR_TEMP:
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */
#endif
        break;

    case UI_TEXT_TYPE_USTR_ALLOC:
        ustr_clr(&p_ui_text->text.ustr_wr);
        break;

#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_ALLOC:
        tstr_clr(&p_ui_text->text.tstr_wr);
        break;
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */

    case UI_TEXT_TYPE_USTR_ARRAY:
#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_ARRAY:
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */
        al_array_dispose(&p_ui_text->text.array_handle_ustr);
        break;
    }

    p_ui_text->type = UI_TEXT_TYPE_NONE;
}

/******************************************************************************
*
* convert a UI data item into a textual representation
*
******************************************************************************/

_Check_return_
extern STATUS
ui_text_from_data(
    _InVal_     UI_DATA_TYPE ui_data_type,
    /*in*/      const UI_DATA * const p_ui_data,
    const void /*UI_CONTROL*/ * p_ui_control,
    _OutRef_    P_UI_TEXT p_ui_text)
{
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_ui_text));
    p_ui_text->type = UI_TEXT_TYPE_NONE;

    switch(ui_data_type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_DATA_TYPE_NONE:
#endif
        return(STATUS_OK);

    case UI_DATA_TYPE_ANY:
        {
        PC_UI_CONTROL_ANY p = (PC_UI_CONTROL_ANY) p_ui_control;
        return((*   p->text_from_data_proc) (p->text_from_data_handle, p_ui_data->any, p_ui_text));
        }

    case UI_DATA_TYPE_F64:
        {
        PC_UI_CONTROL_F64 p = (PC_UI_CONTROL_F64) p_ui_control;
        EV_DATA ev_data;
        NUMFORM_PARMS numform_parms;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 1);
        quick_ublock_with_buffer_setup(quick_ublock);

        p_ui_text->text.array_handle_ustr = 0;

        ev_data_set_real(&ev_data, p_ui_data->f64);

        zero_struct(numform_parms);

        if(NULL != p)
            numform_parms.ustr_numform_numeric = p->ustr_numform;

        if(NULL == numform_parms.ustr_numform_numeric)
        {
            if(CH_NULL == numform_2dp_ustr_buf[0])
                resource_lookup_ustr_buffer(ustr_bptr(numform_2dp_ustr_buf), elemof32(numform_2dp_ustr_buf), MSG_NUMFORM_2_DP);

            numform_parms.ustr_numform_numeric = ustr_bptr(numform_2dp_ustr_buf);
        }

        numform_parms.p_numform_context = get_p_numform_context(P_DOCU_NONE);

        status_return(numform(&quick_ublock, P_QUICK_TBLOCK_NONE, &ev_data, &numform_parms));

        if(quick_ublock_bytes(&quick_ublock) < 2)
        {
            assert(0 == quick_ublock_array_handle_ref(&quick_ublock));

            p_ui_text->type = UI_TEXT_TYPE_TSTR_PERM;
            p_ui_text->text.tstr = tstr_empty_string;
        }
        else
        {   /* steal the handle */
            assert(0 != quick_ublock_array_handle_ref(&quick_ublock));

            p_ui_text->type = UI_TEXT_TYPE_USTR_ARRAY;
            p_ui_text->text.array_handle_ustr = quick_ublock_array_handle_ref(&quick_ublock);
        }

        return(STATUS_OK);
        }

    case UI_DATA_TYPE_S32:
        {
        TCHARZ tmp[32];
        consume_int(tstr_xsnprintf(tmp, elemof32(tmp),
                                   S32_TFMT,
                                   p_ui_data->s32));
        return(ui_text_alloc_from_tstr(p_ui_text, tmp));
        }

    case UI_DATA_TYPE_P_U8:
        p_ui_text->type      = UI_TEXT_TYPE_USTR_PERM;
        p_ui_text->text.ustr = p_ui_data->p_u8;
        return(STATUS_OK);

    case UI_DATA_TYPE_TEXT:
        return(ui_text_copy(p_ui_text, &p_ui_data->text));
    }
}

_Check_return_
_Ret_z_ /* never NULL */
extern PCTSTR
ui_text_tstr(
    _InRef_opt_ PC_UI_TEXT p_ui_text /* may be NULL */)
{
    PCTSTR tstr = NULL;

    if(NULL == p_ui_text)
        return(tstr_empty_string);

    switch(p_ui_text->type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_TEXT_TYPE_NONE:
#endif
        break;

    case UI_TEXT_TYPE_RESID:
        tstr = resource_lookup_tstr_no_default(p_ui_text->text.resource_id);
#if CHECKING
        if(!tstr)
            tstr = resource_lookup_tstr(p_ui_text->text.resource_id);
#endif
        break;

    case UI_TEXT_TYPE_USTR_PERM:
    case UI_TEXT_TYPE_USTR_ALLOC:
    case UI_TEXT_TYPE_USTR_TEMP:
        if(p_ui_text->text.ustr)
            tstr = _tstr_from_ustr(p_ui_text->text.ustr);
        break;

    case UI_TEXT_TYPE_USTR_ARRAY:
        if(array_elements32(&p_ui_text->text.array_handle_ustr))
            tstr = _tstr_from_ustr(array_ustr(&p_ui_text->text.array_handle_ustr));
        break;

#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_PERM:
    case UI_TEXT_TYPE_TSTR_ALLOC:
    case UI_TEXT_TYPE_TSTR_TEMP:
        tstr = p_ui_text->text.tstr;
        break;

    case UI_TEXT_TYPE_TSTR_ARRAY:
        if(array_elements32(&p_ui_text->text.array_handle_tstr))
            tstr = array_tstr(&p_ui_text->text.array_handle_tstr);
        break;
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */
    }

    return(tstr ? tstr : tstr_empty_string);
}

_Check_return_
_Ret_maybenull_z_ /* may be NULL */
extern PCTSTR
ui_text_tstr_no_default(
    _InRef_opt_ PC_UI_TEXT p_ui_text /* may be NULL */)
{
    PCTSTR tstr = NULL;

    if(NULL == p_ui_text)
        return(NULL);

    switch(p_ui_text->type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_TEXT_TYPE_NONE:
#endif
        break;

    case UI_TEXT_TYPE_RESID:
        tstr = resource_lookup_tstr_no_default(p_ui_text->text.resource_id);
        break;

    case UI_TEXT_TYPE_USTR_PERM:
    case UI_TEXT_TYPE_USTR_ALLOC:
    case UI_TEXT_TYPE_USTR_TEMP:
        if(p_ui_text->text.ustr)
            tstr = _tstr_from_ustr(p_ui_text->text.ustr);
        break;

    case UI_TEXT_TYPE_USTR_ARRAY:
        if(array_elements32(&p_ui_text->text.array_handle_ustr))
            tstr = _tstr_from_ustr(array_ustr(&p_ui_text->text.array_handle_ustr));
        break;

#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_PERM:
    case UI_TEXT_TYPE_TSTR_ALLOC:
    case UI_TEXT_TYPE_TSTR_TEMP:
        tstr = p_ui_text->text.tstr;
        break;

    case UI_TEXT_TYPE_TSTR_ARRAY:
        if(array_elements32(&p_ui_text->text.array_handle_tstr))
            tstr = array_tstr(&p_ui_text->text.array_handle_tstr);
        break;
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */
    }

    return(tstr);
}

_Check_return_
_Ret_z_ /* never NULL */
extern PC_USTR
ui_text_ustr(
    _InRef_opt_ PC_UI_TEXT p_ui_text /* may be NULL */)
{
    PC_USTR ustr = NULL;

    if(NULL == p_ui_text)
        return(ustr_empty_string);

    switch(p_ui_text->type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_TEXT_TYPE_NONE:
#endif
        break;

    case UI_TEXT_TYPE_RESID:
        ustr = resource_lookup_ustr_no_default(p_ui_text->text.resource_id);
#if CHECKING
        if(!ustr)
            ustr = resource_lookup_ustr(p_ui_text->text.resource_id);
#endif
        break;

    case UI_TEXT_TYPE_USTR_PERM:
    case UI_TEXT_TYPE_USTR_ALLOC:
    case UI_TEXT_TYPE_USTR_TEMP:
        ustr = p_ui_text->text.ustr;
        break;

    case UI_TEXT_TYPE_USTR_ARRAY:
        if(array_elements32(&p_ui_text->text.array_handle_ustr))
            ustr = array_ustr(&p_ui_text->text.array_handle_ustr);
        break;

#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_PERM:
    case UI_TEXT_TYPE_TSTR_ALLOC:
    case UI_TEXT_TYPE_TSTR_TEMP:
        if(p_ui_text->text.tstr)
            ustr = _ustr_from_tstr(p_ui_text->text.tstr);
        break;

    case UI_TEXT_TYPE_TSTR_ARRAY:
        if(array_elements32(&p_ui_text->text.array_handle_tstr))
            ustr = _ustr_from_tstr(array_tstr(&p_ui_text->text.array_handle_tstr));
        break;
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */
    }

    return(ustr ? ustr : ustr_empty_string);
}

_Check_return_
_Ret_maybenull_z_ /* may be NULL */
extern PC_USTR
ui_text_ustr_no_default(
    _InRef_opt_ PC_UI_TEXT p_ui_text /* may be NULL */)
{
    PC_USTR ustr = NULL;

    if(NULL == p_ui_text)
        return(NULL);

    switch(p_ui_text->type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_TEXT_TYPE_NONE:
#endif
        break;

    case UI_TEXT_TYPE_RESID:
        ustr = resource_lookup_ustr_no_default(p_ui_text->text.resource_id);
        break;

    case UI_TEXT_TYPE_USTR_PERM:
    case UI_TEXT_TYPE_USTR_TEMP:
    case UI_TEXT_TYPE_USTR_ALLOC:
        ustr = p_ui_text->text.ustr;
        break;

    case UI_TEXT_TYPE_USTR_ARRAY:
        if(array_elements32(&p_ui_text->text.array_handle_ustr))
            ustr = array_ustr(&p_ui_text->text.array_handle_ustr);
        break;

#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_PERM:
    case UI_TEXT_TYPE_TSTR_TEMP:
    case UI_TEXT_TYPE_TSTR_ALLOC:
        if(p_ui_text->text.tstr)
            ustr = _ustr_from_tstr(p_ui_text->text.tstr);
        break;

    case UI_TEXT_TYPE_TSTR_ARRAY:
        if(array_elements32(&p_ui_text->text.array_handle_tstr))
            ustr = _ustr_from_tstr(array_tstr(&p_ui_text->text.array_handle_tstr));
        break;
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */
    }

    return(ustr);
}

_Check_return_
extern STATUS
ui_text_read(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InRef_opt_ PC_UI_TEXT p_ui_text /* may be NULL */)
{
    PCTSTR tstr = ui_text_tstr_no_default(p_ui_text);
    const U32 len = (NULL != tstr) ? tstrlen32(tstr) : 0;

    if(0 == len)
        return(STATUS_OK);

    return(quick_tblock_tchars_add(p_quick_tblock, tstr, len));
}

/******************************************************************************
*
* validate the contents of a UI_TEXT using an acceptance bitmap
*
* --Out--
*   number of characters rejected
*
******************************************************************************/

#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)

static U32
tstr_accept_process(
    _In_z_      PTSTR p_in,
    _InRef_opt_ PC_BITMAP p_bitmap_accept)
{
    PTSTR p_out = p_in;
    TCHAR ch;

    /* default is to accept all */
    if(NULL == p_bitmap_accept)
        return(0);

    do  {
        ch = *p_in++;

        if(!ch || (ch >= 256) || bitmap_bit_test(p_bitmap_accept, ch, N_BITS_ARG(256)))
            *p_out++ = ch;
    }
    while(CH_NULL != ch);

    return(PtrDiffElemU32(p_in, p_out));
}

#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */

static U32
ustr_accept_process(
    _In_z_      P_USTR p_in,
    _InRef_opt_ PC_BITMAP p_bitmap_accept)
{
    P_USTR p_out = p_in;
    UCS4 ucs4;

    /* default is to accept all */
    if(NULL == p_bitmap_accept)
        return(0);

    do  {
        U32 bytes_of_char;

        ucs4 = uchars_char_decode(p_in, bytes_of_char), uchars_IncBytes_wr(p_in, bytes_of_char);

        if((CH_NULL == ucs4) || !ucs4_is_sbchar(ucs4) || bitmap_bit_test(p_bitmap_accept, ucs4, N_BITS_ARG(256)))
        {
            (void) uchars_char_encode(p_out, bytes_of_char, ucs4), uchars_IncBytes_wr(p_out, bytes_of_char);
        }
    }
    while(CH_NULL != ucs4);

    return(PtrDiffBytesU32(p_in, p_out));
}

_Check_return_
extern U32
ui_text_validate(
    _InoutRef_  P_UI_TEXT p_ui_text,
    _InRef_opt_ PC_BITMAP p_bitmap_accept)
{
    switch(p_ui_text->type)
    {
    default: default_unhandled();
#if CHECKING
    case UI_TEXT_TYPE_NONE:
    case UI_TEXT_TYPE_RESID:
#endif
        return(0);

    case UI_TEXT_TYPE_USTR_PERM:
    case UI_TEXT_TYPE_USTR_TEMP:
    case UI_TEXT_TYPE_USTR_ALLOC:
        return(ustr_accept_process(p_ui_text->text.ustr_wr, p_bitmap_accept));

    case UI_TEXT_TYPE_USTR_ARRAY:
        return(ustr_accept_process(de_const_cast(P_USTR, array_ustr(&p_ui_text->text.array_handle_ustr)), p_bitmap_accept));

#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    case UI_TEXT_TYPE_TSTR_PERM:
    case UI_TEXT_TYPE_TSTR_TEMP:
    case UI_TEXT_TYPE_TSTR_ALLOC:
        return(tstr_accept_process(p_ui_text->text.tstr_wr, p_bitmap_accept));

    case UI_TEXT_TYPE_TSTR_ARRAY:
        return(tstr_accept_process(de_const_cast(PTSTR, array_tstr(&p_ui_text->text.array_handle_tstr)), p_bitmap_accept));
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */
    }
}

_Check_return_
extern PIXIT
ui_width_from_ustr(
    _In_z_      PC_USTR ustr)
{
    return(ui_width_from_tstr(_tstr_from_ustr(ustr)));
}

_Check_return_
extern PIXIT
ui_width_from_tstr(
    _In_z_      PCTSTR tstr)
{
    PIXIT width = ui_width_from_tstr_host(tstr);

#if WINDOWS || RISCOS
    BOOL has_hot_key = (NULL != tstrchr(tstr, CH_AMPERSAND));

    if(has_hot_key)
    {
        static PIXIT hot_key_width;
        if(!hot_key_width)
            hot_key_width = ui_width_from_tstr_host(TEXT("&"));
        width -= hot_key_width;
    }
#endif

    return(width);
}

_Check_return_
extern PIXIT
ui_width_from_p_ui_text(
    _InRef_     PC_UI_TEXT p_ui_text)
{
    return(ui_width_from_tstr(ui_text_tstr(p_ui_text)));
}

extern void
font_spec_from_ui_style(
    _OutRef_    P_FONT_SPEC p_font_spec,
    _InRef_     PC_STYLE p_ui_style)
{
    STYLE_SELECTOR style_selector_ensure;

    *p_font_spec = p_ui_style->font_spec;

    if(!style_selector_bic(&style_selector_ensure, &style_selector_font_spec, &p_ui_style->selector))
        return;

    /* we expect all font_spec bits to have been set by UI.Base - this is paranoia */

    /* OK to have no typeface specified - just get the default */
    if(!style_bit_test(p_ui_style, STYLE_SW_FS_NAME))
        p_font_spec->h_app_name_tstr = 0;

    if(!style_bit_test(p_ui_style, STYLE_SW_FS_SIZE_X))
        p_font_spec->size_x = 0;

    if(!style_bit_test(p_ui_style, STYLE_SW_FS_SIZE_Y))
        p_font_spec->size_y = 10 * PIXITS_PER_POINT;

    if(!style_bit_test(p_ui_style, STYLE_SW_FS_UNDERLINE))
        p_font_spec->underline = 0;

    if(!style_bit_test(p_ui_style, STYLE_SW_FS_BOLD))
        p_font_spec->bold = 0;

    if(!style_bit_test(p_ui_style, STYLE_SW_FS_ITALIC))
        p_font_spec->italic = 0;

    if(!style_bit_test(p_ui_style, STYLE_SW_FS_SUPERSCRIPT))
        p_font_spec->superscript = 0;

    if(!style_bit_test(p_ui_style, STYLE_SW_FS_SUBSCRIPT))
        p_font_spec->subscript = 0;

    if(!style_bit_test(p_ui_style, STYLE_SW_FS_SUBSCRIPT))
        p_font_spec->colour = rgb_stash[COLOUR_OF_TEXT];
}

/* end of ui_data.c */
