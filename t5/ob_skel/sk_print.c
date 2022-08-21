/* sk_print.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Printing for Fireworkz */

/* RCM Aug 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/ho_print.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

/*
internal routines
*/

_Check_return_
static STATUS
pamphlet(
    _OutRef_    P_UI_TEXT p_ui_text,
    _InVal_     S32 max_page_y);

_Check_return_
static STATUS
print_quality_save(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format);

extern void
page_def_from_phys_page_def(
    _InoutRef_  P_PAGE_DEF p_page_def,
    _InRef_     PC_PHYS_PAGE_DEF p_phys_page_def,
    _InVal_     S32 paper_scale)
{
    p_page_def->size_x        = muldiv64_round_floor(p_phys_page_def->size_x,        100, paper_scale);
    p_page_def->size_y        = muldiv64_round_floor(p_phys_page_def->size_y,        100, paper_scale);
    p_page_def->margin_top    = muldiv64_round_floor(p_phys_page_def->margin_top,    100, paper_scale);
    p_page_def->margin_bottom = muldiv64_round_floor(p_phys_page_def->margin_bottom, 100, paper_scale);
    p_page_def->margin_left   = muldiv64_round_floor(p_phys_page_def->margin_left,   100, paper_scale);
    p_page_def->margin_right  = muldiv64_round_floor(p_phys_page_def->margin_right,  100, paper_scale);
    p_page_def->margin_bind   = muldiv64_round_floor(p_phys_page_def->margin_bind,   100, paper_scale);
    p_page_def->margin_oe_swap = p_phys_page_def->margin_oe_swap;
    p_page_def->landscape      = p_phys_page_def->landscape;
}

#if RISCOS
#define LINE_WIDTH_STANDARD (2 * PIXITS_PER_RISCOS)
#endif

#define PAGE_SIZE_X_MIN     (PIXITS_PER_CM * 2)
#define PAGE_SIZE_Y_MIN     (PIXITS_PER_CM * 2)

extern void
page_def_validate(
    _InoutRef_  P_PAGE_DEF p_page_def)
{
#if RISCOS
    const PIXIT grid_size = p_page_def->grid_size ? MAX(LINE_WIDTH_STANDARD, p_page_def->grid_size) : 0;
#else
    const PIXIT grid_size = p_page_def->grid_size;
#endif
    PIXIT extra_x, extra_y;

    if(p_page_def->margin_bind && p_page_def->margin_oe_swap)
    {
        PIXIT max_lm_rm = MAX(p_page_def->margin_left, p_page_def->margin_right);
        extra_x = MAX(max_lm_rm, p_page_def->margin_bind) + max_lm_rm;
    }
    else
        extra_x = p_page_def->margin_left + p_page_def->margin_bind + p_page_def->margin_right;

    extra_y = p_page_def->margin_top + p_page_def->margin_bottom;

    extra_x += grid_size;
    extra_y += grid_size;

    extra_x += p_page_def->margin_row;
    extra_y += p_page_def->margin_col;

    if( p_page_def->size_x < extra_x + PAGE_SIZE_X_MIN)
        p_page_def->size_x = extra_x + PAGE_SIZE_X_MIN;
    if( p_page_def->size_y < extra_y + PAGE_SIZE_Y_MIN)
        p_page_def->size_y = extra_y + PAGE_SIZE_Y_MIN;

    p_page_def->cells_usable_x = p_page_def->size_x - extra_x;
    p_page_def->cells_usable_y = p_page_def->size_y - extra_y;
}

_Check_return_
static STATUS
paper_save(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     ARRAY_INDEX loaded_id)
{
    const PC_PHYS_PAGE_DEF p_phys_page_def = !loaded_id ? &p_docu->phys_page_def : array_ptr(&p_docu->loaded_phys_page_defs, PHYS_PAGE_DEF, loaded_id - 1);
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_PAPER, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, ARG_PAPER_N_ARGS);

        p_args[ARG_PAPER_NAME].val.ustr = (PC_USTR) p_phys_page_def->ustr_name;
        p_args[ARG_PAPER_ORIENTATION].val.s32           = p_phys_page_def->landscape;
        p_args[ARG_PAPER_HEIGHT].val.fp_pixit           = p_phys_page_def->size_y;
        p_args[ARG_PAPER_WIDTH].val.fp_pixit            = p_phys_page_def->size_x;
        p_args[ARG_PAPER_MARGIN_TOP].val.fp_pixit       = p_phys_page_def->margin_top;
        p_args[ARG_PAPER_MARGIN_BOTTOM].val.fp_pixit    = p_phys_page_def->margin_bottom;
        p_args[ARG_PAPER_MARGIN_LEFT].val.fp_pixit      = p_phys_page_def->margin_left;
        p_args[ARG_PAPER_MARGIN_RIGHT].val.fp_pixit     = p_phys_page_def->margin_right;
        p_args[ARG_PAPER_MARGIN_BIND].val.fp_pixit      = p_phys_page_def->margin_bind;
        p_args[ARG_PAPER_MARGIN_OE_SWAP].val.fBool      = p_phys_page_def->margin_oe_swap;

        if(loaded_id)
        {
            p_args[ARG_PAPER_LOADED_ID].val.s32 = loaded_id;

            arg_dispose(&arglist_handle, ARG_PAPER_MARGIN_COL);
            arg_dispose(&arglist_handle, ARG_PAPER_MARGIN_ROW);
            arg_dispose(&arglist_handle, ARG_PAPER_GRID_SIZE);
            arg_dispose(&arglist_handle, ARG_PAPER_GRID_FAINT);
        }
        else
        {
            arg_dispose(&arglist_handle, ARG_PAPER_LOADED_ID);

            p_args[ARG_PAPER_MARGIN_COL].val.fp_pixit   = p_docu->page_def.margin_col;
            p_args[ARG_PAPER_MARGIN_ROW].val.fp_pixit   = p_docu->page_def.margin_row;
            p_args[ARG_PAPER_GRID_SIZE].val.f64    = p_docu->page_def.grid_size; /*POINTS*/
            p_args[ARG_PAPER_GRID_FAINT].val.fBool = p_docu->flags.faint_grid;
        }

        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

_Check_return_
static STATUS
paper_scale_save(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_PAPER_SCALE, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        p_args[0].val.s32 = p_docu->paper_scale;
        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
        arglist_dispose(&arglist_handle);
    }

    return(status);
}

typedef struct PRINT_CALLBACK
{
    S32 extra;
    PAGE max_page_y;
    S32 all_or_range;
}
PRINT_CALLBACK, * P_PRINT_CALLBACK;

enum PRINT_CONTROL_IDS
{
    CONTROL_ID_EXTRA = 64,
    CONTROL_ID_MAILS,

    CONTROL_ID_COPIES_ORNAMENT,
    CONTROL_ID_COPIES,
    CONTROL_ID_AR_GROUP,
    CONTROL_ID_ALL,
    CONTROL_ID_RANGE,

    CONTROL_ID_RANGE_GROUP,
    CONTROL_ID_RANGE_Y_GROUP,
    CONTROL_ID_RANGE_Y0_TEXT,
    CONTROL_ID_RANGE_Y0,
    CONTROL_ID_RANGE_EDIT = CONTROL_ID_RANGE_Y0,
    CONTROL_ID_RANGE_Y1_TEXT,
    CONTROL_ID_RANGE_Y1,

    CONTROL_ID_SIDES_GROUP = 128,
    CONTROL_ID_BOTH,
    CONTROL_ID_ODD,
    CONTROL_ID_EVEN,
    CONTROL_ID_REVERSE,
    CONTROL_ID_COLLATE,
    CONTROL_ID_TWO_UP,
    CONTROL_ID_PAMPHLET,

    CONTROL_ID_MESSAGE1,        /* actually belongs to warning dialog box */
    CONTROL_ID_MESSAGE2         /* actually belongs to warning dialog box */
};

static const DIALOG_CONTROL
print_copies_ornament =
{
    CONTROL_ID_COPIES_ORNAMENT, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, CONTROL_ID_COPIES, DIALOG_CONTROL_SELF, CONTROL_ID_COPIES },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
print_copies_ornament_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_COPIES) };

static const DIALOG_CONTROL
print_copies =
{
    CONTROL_ID_COPIES, DIALOG_MAIN_GROUP,
    { CONTROL_ID_COPIES_ORNAMENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDSPACING_H, 0, DIALOG_BUMP_H(5), DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_S32), 1 }
};

static const UI_CONTROL_S32
print_copies_data_control = { 1, 1000 };

static /*poked*/ DIALOG_CONTROL_DATA_BUMP_S32
print_copies_data = { { { { FRAMED_BOX_EDIT } }, &print_copies_data_control } };

static const DIALOG_CONTROL
print_ar_group =
{
    CONTROL_ID_AR_GROUP, DIALOG_MAIN_GROUP,
    { CONTROL_ID_COPIES_ORNAMENT, CONTROL_ID_COPIES_ORNAMENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, 0, 0 },
    { DRT(LBRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
print_all =
{
    CONTROL_ID_ALL, CONTROL_ID_AR_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
print_all_data = { { 0 }, 0 /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_ALL) };

static const DIALOG_CONTROL
print_range =
{
    CONTROL_ID_RANGE, CONTROL_ID_AR_GROUP,
    { CONTROL_ID_ALL, CONTROL_ID_ALL },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTONF
e_print_range_data = { { { 0, 1 /*move_focus*/ }, 1 /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_RANGE) }, CONTROL_ID_RANGE_Y0 };

static const DIALOG_CONTROL_DATA_RADIOBUTTONF
s_print_range_data = { { { 0, 1 /*move_focus*/ }, 1 /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_RANGE) }, CONTROL_ID_RANGE_EDIT };

static const DIALOG_CONTROL
print_range_group =
{
    CONTROL_ID_RANGE_GROUP, DIALOG_MAIN_GROUP,
    { CONTROL_ID_AR_GROUP, CONTROL_ID_RANGE, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDSPACING_H, (DIALOG_STDBUMP_V - DIALOG_STDRADIO_V) /2 /*DIALOG_SMALLSPACING_V*/ },
    { DRT(RTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
print_range_edit =
{
    CONTROL_ID_RANGE_EDIT, CONTROL_ID_RANGE_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, PIXITS_PER_INCH + PIXITS_PER_HALF_INCH, PIXITS_PER_INCH },
    { DRT(LTLT, EDIT), 1 }
};

static BITMAP(print_range_edit_validation, 256);

static const DIALOG_CONTROL_DATA_EDIT
print_range_edit_data = { { { FRAMED_BOX_EDIT, 0, 0, 1 /*multiline*/ }, print_range_edit_validation } /* EDIT_XX */ };

static const DIALOG_CONTROL
print_range_y_group =
{
    CONTROL_ID_RANGE_Y_GROUP, CONTROL_ID_RANGE_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0 },
    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
print_range_y0_text =
{
    CONTROL_ID_RANGE_Y0_TEXT, CONTROL_ID_RANGE_Y_GROUP,
    { DIALOG_CONTROL_PARENT, CONTROL_ID_RANGE_Y0, DIALOG_CONTROL_SELF, CONTROL_ID_RANGE_Y0 },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
print_range_y0_text_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_RANGE_Y0) };

static const DIALOG_CONTROL
print_range_y0 =
{
    CONTROL_ID_RANGE_Y0, CONTROL_ID_RANGE_Y_GROUP,
    { CONTROL_ID_RANGE_Y0_TEXT, DIALOG_CONTROL_PARENT },
    { DIALOG_SMALLSPACING_H, 0, DIALOG_BUMP_H(5), DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_S32), 1 }
};

static /*poked*/ UI_CONTROL_S32
print_range_y0_data_control = { 1, 1, 1 };

static /*poked*/ DIALOG_CONTROL_DATA_BUMP_S32
print_range_y0_data = { { { { FRAMED_BOX_EDIT } }, &print_range_y0_data_control } };

static const DIALOG_CONTROL
print_range_y1_text =
{
    CONTROL_ID_RANGE_Y1_TEXT, CONTROL_ID_RANGE_Y_GROUP,
    { DIALOG_CONTROL_SELF, CONTROL_ID_RANGE_Y1, CONTROL_ID_RANGE_Y0_TEXT , CONTROL_ID_RANGE_Y1 },
    { DIALOG_CONTENTS_CALC, 0, 0, 0 },
    { DRT(RTRB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
print_range_y1_text_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_RANGE_Y1) };

static const DIALOG_CONTROL
print_range_y1 =
{
    CONTROL_ID_RANGE_Y1, CONTROL_ID_RANGE_Y_GROUP,
    { CONTROL_ID_RANGE_Y0, CONTROL_ID_RANGE_Y0, CONTROL_ID_RANGE_Y0 },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_S32), 1 }
};

static /*poked*/ UI_CONTROL_S32
print_range_y1_data_control = { 1, 1, 1 };

static /*poked*/ DIALOG_CONTROL_DATA_BUMP_S32
print_range_y1_data = { { { { FRAMED_BOX_EDIT } }, &print_range_y1_data_control } };

static const DIALOG_CONTROL
print_sides_group =
{
    CONTROL_ID_SIDES_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, CONTROL_ID_RANGE_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
print_sides_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_SIDES), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
print_both =
{
    CONTROL_ID_BOTH, CONTROL_ID_SIDES_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
print_both_data = { { 0 }, 0 /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_BOTH) };

static const DIALOG_CONTROL
print_odd =
{
    CONTROL_ID_ODD, CONTROL_ID_SIDES_GROUP,
    { CONTROL_ID_BOTH, CONTROL_ID_BOTH, DIALOG_CONTROL_SELF, CONTROL_ID_BOTH },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
print_odd_data = { { 0 }, 1 /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_ODD) };

static const DIALOG_CONTROL
print_even =
{
    CONTROL_ID_EVEN, CONTROL_ID_SIDES_GROUP,
    { CONTROL_ID_ODD, CONTROL_ID_ODD, DIALOG_CONTROL_SELF, CONTROL_ID_ODD },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
print_even_data = { { 0 }, 2 /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_EVEN) };

static const DIALOG_CONTROL
print_reverse =
{
    CONTROL_ID_REVERSE, DIALOG_MAIN_GROUP,
    { CONTROL_ID_SIDES_GROUP, CONTROL_ID_SIDES_GROUP },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
print_reverse_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_REVERSE) };

static const DIALOG_CONTROL
print_collate =
{
    CONTROL_ID_COLLATE, DIALOG_MAIN_GROUP,
    { CONTROL_ID_REVERSE, CONTROL_ID_REVERSE },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
print_collate_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_SORTED) };

static const DIALOG_CONTROL
print_two_up =
{
    CONTROL_ID_TWO_UP, DIALOG_MAIN_GROUP,
    { CONTROL_ID_COLLATE, CONTROL_ID_COLLATE },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
print_two_up_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_TWO_UP) };

static const DIALOG_CONTROL
print_pamphlet =
{
    CONTROL_ID_PAMPHLET, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF, CONTROL_ID_RANGE_GROUP, CONTROL_ID_TWO_UP },
#if RISCOS
    { DIALOG_CONTENTS_CALC, DIALOG_STDPUSHBUTTON_V, 0, 0 },
#else
    { DIALOG_STDPUSHBUTTON_H, DIALOG_STDPUSHBUTTON_V, 0, 0 },
#endif
    { DRT(RBRB, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
print_pamphlet_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_PAMPHLET) };

static const DIALOG_CONTROL
print_mails =
{
    CONTROL_ID_MAILS, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_SELF, CONTROL_ID_EXTRA, CONTROL_ID_EXTRA, CONTROL_ID_EXTRA },
#if RISCOS
    { DIALOG_CONTENTS_CALC, 0, DIALOG_STDSPACING_H, 0 },
#else
    { DIALOG_STDPUSHBUTTON_H, 0, DIALOG_STDSPACING_H, 0 },
#endif
    { DRT(RTLB, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
print_mails_command = { T5_CMD_MAILSHOT_PRINT, OBJECT_ID_MAILSHOT, NULL, NULL, { 0, 0, 0, 1 /*lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
print_mails_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_MAILS), &print_mails_command };

static const DIALOG_CONTROL
print_extra =
{
    CONTROL_ID_EXTRA, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_SELF, CONTROL_ID_RANGE_GROUP, CONTROL_ID_RANGE_GROUP, DIALOG_CONTROL_SELF },
#if RISCOS
    { DIALOG_CONTENTS_CALC, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
#else
    { DIALOG_STDPUSHBUTTON_H, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
#endif
    { DRT(RBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
print_extra_data = { { CONTROL_ID_EXTRA }, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_EXTRA) };

static const DIALOG_CONTROL_ID
e_print_ok_data_argmap[] =
{
#define ARG_PRINT_COPIES       0
    CONTROL_ID_COPIES,
#define ARG_PRINT_RANGE        1
    CONTROL_ID_AR_GROUP,
#define ARG_PRINT_RANGE_Y0     2
    0,
#define ARG_PRINT_RANGE_Y1     3
    0,
#define ARG_PRINT_RANGE_LIST   4
    CONTROL_ID_RANGE_EDIT,
#define ARG_PRINT_SIDES        5
    CONTROL_ID_SIDES_GROUP,
#define ARG_PRINT_REVERSE      6
    CONTROL_ID_REVERSE,
#define ARG_PRINT_COLLATE      7
    CONTROL_ID_COLLATE,
#define ARG_PRINT_TWO_UP       8
    CONTROL_ID_TWO_UP
#define ARG_PRINT_N_ARGS       9
};

static const DIALOG_CONTROL_ID
s_print_ok_data_argmap[elemof32(e_print_ok_data_argmap)] =
{
    CONTROL_ID_COPIES,
    CONTROL_ID_AR_GROUP,
    CONTROL_ID_RANGE_Y0,
    CONTROL_ID_RANGE_Y1
};

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
e_print_ok_command = { T5_CMD_PRINT_EXTRA, OBJECT_ID_SKEL, NULL, e_print_ok_data_argmap, { 0, 0, 0, 1 /*lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
e_print_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_OK), &e_print_ok_command };

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
s_print_ok_command = { T5_CMD_PRINT, OBJECT_ID_SKEL, NULL, s_print_ok_data_argmap, { 0, 0, 0, 1 /*lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
s_print_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_OK), &s_print_ok_command };

static const DIALOG_CTL_CREATE
e_print_dialog_create[] =
{
    { &dialog_main_group },
    { &print_copies_ornament, &print_copies_ornament_data },
    { &print_copies         , &print_copies_data          },
    { &print_ar_group },
    { &print_all            , &print_all_data             },
    { &print_range          , &e_print_range_data         },
    { &print_range_group },
    { &print_range_edit     , &print_range_edit_data      },
    { &print_sides_group    , &print_sides_group_data     },
    { &print_both           , &print_both_data            },
    { &print_odd            , &print_odd_data             },
    { &print_even           , &print_even_data            },
    { &print_reverse        , &print_reverse_data         },
    { &print_collate        , &print_collate_data         },
    { &print_two_up         , &print_two_up_data          },
    { &print_pamphlet       , &print_pamphlet_data        },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &defbutton_ok, &e_print_ok_data }
};

static /*poked*/ DIALOG_CTL_CREATE
s_print_dialog_create[] =
{
    { &dialog_main_group },
    { &print_copies_ornament, &print_copies_ornament_data },
    { &print_copies         , &print_copies_data },
    { &print_ar_group },
    { &print_all            , &print_all_data },
    { &print_range          , &s_print_range_data },
    { &print_range_group },
    { &print_range_y_group },
    { &print_range_y0_text  , &print_range_y0_text_data },
    { &print_range_y0       , &print_range_y0_data },
    { &print_range_y1_text  , &print_range_y1_text_data },
    { &print_range_y1       , &print_range_y1_data },

    { &print_mails          , &print_mails_data }, /* this is the fourth last entry and must not move wrt end */
    { &print_extra          , &print_extra_data }, /* this is the third last entry and must not move wrt end */

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &defbutton_ok, &s_print_ok_data }
};

_Check_return_
static STATUS
dialog_print_intro_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    STATUS status = STATUS_OK;

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case CONTROL_ID_SIDES_GROUP:
        p_dialog_msg_ctl_create_state->state_set.state.radiobutton = 0;
        break;

    case CONTROL_ID_RANGE_EDIT:
        {
        const P_PRINT_CALLBACK p_print_callback = (P_PRINT_CALLBACK) p_dialog_msg_ctl_create_state->client_handle;

        if(p_print_callback->extra)
        {
            UI_TEXT ui_text;
            UCHARZ ustr_buf[256];

#if 0
            consume_int(xsnprintf(ustr_buf, elemof32(ustr_buf),
                                  S32_FMT "." S32_FMT " - " S32_FMT "." S32_FMT,
                                  print_range_x0_data.state, print_range_y0_data.state, print_range_x1_data.state, print_range_y1_data.state));
#endif
            consume_int(xsnprintf(ustr_buf, elemof32(ustr_buf),
                                  S32_FMT " - " S32_FMT,
                                  print_range_y0_data.state, print_range_y1_data.state));

            ui_text.type = UI_TEXT_TYPE_USTR_TEMP;
            ui_text.text.ustr = ustr_bptr(ustr_buf);
            status = ui_dlg_set_edit(p_dialog_msg_ctl_create_state->h_dialog, CONTROL_ID_RANGE_EDIT, &ui_text);
            p_dialog_msg_ctl_create_state->processed = 1;
        }

        break;
        }

    default:
        break;
    }

    return(status);
}

_Check_return_
static STATUS
dialog_print_intro_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case CONTROL_ID_AR_GROUP:
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, CONTROL_ID_RANGE_GROUP, (p_dialog_msg_ctl_state_change->new_state.radiobutton == 1));
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_print_intro_process_start(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_PRINT_CALLBACK p_print_callback = (P_PRINT_CALLBACK) p_dialog_msg_process_start->client_handle;

    /* disable 'range' button plus associated UI */
    /* SKS says only disable things in wally's box - don't hassle Mr Extra */
    /* especially 'cos he might choose pamphlet printing */
    if(!p_print_callback->extra)
    {
        ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, CONTROL_ID_RANGE_GROUP, (print_range_y0_data_control.max_val > 1));

        {
        const P_BYTE p_data = _p_object_instance_data(p_docu, OBJECT_ID_MAILSHOT CODE_ANALYSIS_ONLY_ARG(1));
        ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, CONTROL_ID_MAILS, !IS_P_DATA_NONE(p_data));
        } /*block*/
    }
    else
    {
        /* disable two_up and pamphlet if in draft printing mode */
        ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, CONTROL_ID_PAMPHLET, !p_docu->flags.draft_mode);
        ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, CONTROL_ID_TWO_UP,   !p_docu->flags.draft_mode);
    }

    return(ui_dlg_set_radio(p_dialog_msg_process_start->h_dialog, CONTROL_ID_AR_GROUP, p_print_callback->all_or_range && (print_range_y0_data_control.max_val > 1)));
}

_Check_return_
static STATUS
dialog_print_intro_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    STATUS status = STATUS_OK;

    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case CONTROL_ID_PAMPHLET:
        {
        const P_PRINT_CALLBACK p_print_callback = (P_PRINT_CALLBACK) p_dialog_msg_ctl_pushbutton->client_handle;
        UI_TEXT ui_text;
        status_break(status = pamphlet(&ui_text, p_print_callback->max_page_y));
        status = ui_dlg_set_edit(p_dialog_msg_ctl_pushbutton->h_dialog, CONTROL_ID_RANGE_EDIT, &ui_text);
        ui_text_dispose(&ui_text);
        if(status_ok(status = ui_dlg_set_radio(p_dialog_msg_ctl_pushbutton->h_dialog, CONTROL_ID_AR_GROUP, 1))) /* set all/range radio button to range */
                     status = ui_dlg_set_check(p_dialog_msg_ctl_pushbutton->h_dialog, CONTROL_ID_TWO_UP, TRUE); /* and two up */
        break;
        }
    }

    return(status);
}

_Check_return_
static STATUS
dialog_print_intro_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(p_dialog_msg_process_end->completion_code == CONTROL_ID_EXTRA)
    {
        const P_PRINT_CALLBACK p_print_callback = (P_PRINT_CALLBACK) p_dialog_msg_process_end->client_handle;

        /* 'extra' button pressed, so read back copies, range etc, loop back and create new dialog box */
        print_copies_data.state = ui_dlg_get_s32(p_dialog_msg_process_end->h_dialog, CONTROL_ID_COPIES);

        p_print_callback->all_or_range = ui_dlg_get_radio(p_dialog_msg_process_end->h_dialog, CONTROL_ID_AR_GROUP);

        if(p_print_callback->all_or_range)
        {
            /* range selected: assume punter wants one vertical strip */
            print_range_y0_data.state = ui_dlg_get_s32(p_dialog_msg_process_end->h_dialog, CONTROL_ID_RANGE_Y0);
            print_range_y1_data.state = ui_dlg_get_s32(p_dialog_msg_process_end->h_dialog, CONTROL_ID_RANGE_Y1);
        }
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_print_intro)
{
    STATUS status = STATUS_OK;

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_print_intro_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_print_intro_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_print_intro_process_start(p_docu, (PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_print_intro_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_print_intro_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    default:
        UNREFERENCED_PARAMETER_DocuRef_(p_docu);
        break;
    }

    return(status);
}

_Check_return_
static STATUS
pagelist_fill_from(
    P_ARRAY_HANDLE p_h_page_list,
    _In_z_      PCTSTR tstr)
{
    STATUS status = STATUS_OK;
    SS_RECOG_CONTEXT ss_recog_context;

    ss_recog_context_push(&ss_recog_context);

    for(;;)
    {
        TCHAR ch;
        PAGE_NUM page_num_1;
        PAGE_NUM page_num_2;
        U32 used;

        /* skip spaces, commas and newlines */

        while(((ch = *tstr++) == CH_SPACE) || (ch == g_ss_recog_context.list_sep_char) || (ch == LF))
        { /*EMPTY*/ }
        --tstr;

        if(!ch)
            break;

        if((used = get_pagenum(tstr, &page_num_1)) > 0)
        {
            tstr += used;

            /* skip over any unrecognized debris */
            while(((ch = *tstr++) != CH_SPACE) && (ch != g_ss_recog_context.list_sep_char) && (ch != CH_HYPHEN_MINUS) && (ch != LF) && ch)
            { /*EMPTY*/ }
            --tstr;

            /* skip_spaces */
            while(((ch = *tstr++) == CH_SPACE))
            { /*EMPTY*/ }
            --tstr;

            if(ch != CH_HYPHEN_MINUS)
            {
                if((page_num_1.x > 0) && (page_num_1.y > 0))
                    status = pagelist_add_page(p_h_page_list, page_num_1.x, page_num_1.y);
                else
                    status = pagelist_add_blank(p_h_page_list);
            }
            else
            {
                /* it looks like a range */
                ch = *tstr++;

                if((used = get_pagenum(tstr, &page_num_2)) > 0)
                {
                    tstr += used;

                    if((page_num_1.x > 0) && (page_num_1.y > 0) && (page_num_2.x > 0) && (page_num_2.y > 0))
                        status = pagelist_add_range(p_h_page_list, page_num_1.x, page_num_1.y, page_num_2.x, page_num_2.y);

                    /* skip over any unrecognized debris */
                    while(((ch = *tstr++) != CH_SPACE) && (ch != g_ss_recog_context.list_sep_char) && (ch != LF) && ch)
                    { /*EMPTY*/ }
                    --tstr;
                }
            }
        }

        if(status_fail(status))
        {
            status_assertc(status);
            break;
        }
    }

    ss_recog_context_pull(&ss_recog_context);

    return(status);
}

extern U32 /* used */
get_pagenum(
    _In_z_      PCTSTR tstr_in,
    _OutRef_    P_PAGE_NUM p_page_num)
{
    PCTSTR tstr = tstr_in;
    PTSTR new_tstr;
    S32 num1;
    S32 num2;

    p_page_num->x = -1; /* blank */
    p_page_num->y = -1;

    if((*tstr == 'B') || (*tstr == 'b'))
    {
        ++tstr;
        /*p_page_num->x = -1;*/
        /*p_page_num->y = -1;*/
        return(PtrDiffElemU32(tstr, tstr_in)); /* read B or b for blank */
    }

    num1 = (S32) tstrtol(tstr, &new_tstr, 10);
    if(tstr == new_tstr)
        return(0); /* naff */
    tstr = new_tstr;

    if(*tstr != CH_FULL_STOP) /* <<< this dot ain't a decimal point */
    {
        p_page_num->x = 1;
        p_page_num->y = num1;
        return(PtrDiffElemU32(tstr, tstr_in)); /* read a page_num */
    }

    tstr++;

    if(!/*"C"*/isdigit(*tstr)) /* we want the next number to start immediately after the dot */
        return(0); /* naff */

    num2 = (S32) tstrtol(tstr, &new_tstr, 10);
    if(tstr == new_tstr)
        return(0); /* naff */
    tstr = new_tstr;

    /* read <number>.<number> */
    p_page_num->x = num1;
    p_page_num->y = num2;
    return(PtrDiffElemU32(tstr, tstr_in));  /* read a page_num */
}

SC_ARRAY_INIT_BLOCK page_entry_array_init_block = aib_init(1, sizeof32(PAGE_ENTRY), TRUE);

_Check_return_
extern STATUS
pagelist_add_blank(
    _InoutRef_  P_ARRAY_HANDLE p_h_page_list)
{
    PAGE_ENTRY page_entry;

    page_entry.page.x = -1;
    page_entry.page.y = -1;

    return(al_array_add(p_h_page_list, PAGE_ENTRY, 1, &page_entry_array_init_block, &page_entry));
}

_Check_return_
extern STATUS
pagelist_add_page(
    _InoutRef_  P_ARRAY_HANDLE p_h_page_list,
    _InVal_     S32 x,
    _InVal_     S32 y)
{
    PAGE_ENTRY page_entry;

    page_entry.page.x = x - 1; /* UI/cmd deals in 'sensible', non-zero numbers! */
    page_entry.page.y = y - 1;

    return(al_array_add(p_h_page_list, PAGE_ENTRY, 1, &page_entry_array_init_block, &page_entry));
}

_Check_return_
extern STATUS
pagelist_add_range(
    _InoutRef_  P_ARRAY_HANDLE p_h_page_list,
    _InVal_     S32 x0,
    _InVal_     S32 y0,
    _InVal_     S32 x1,
    _InVal_     S32 y1)
{
    STATUS status = STATUS_OK;
    S32 y;

    for(y = y0; y <= y1; ++y)
    {
        S32 x;

        for(x = x0; x <= x1; ++x)
        {
            status_break(status = pagelist_add_page(p_h_page_list, x, y));
        }

        status_break(status);
    }

    return(status);
}

_Check_return_
static STATUS
pamphlet(
    _OutRef_    P_UI_TEXT p_ui_text,
    _InVal_     PAGE max_page_y)
{
    STATUS status = STATUS_OK;
    S32 generate_pages;
    S32 l_count, r_count, swap;
    S32 l_page, r_page;
    U8Z l_string[BUF_MAX_S32_FMT];
    U8Z r_string[BUF_MAX_S32_FMT];
    U8Z entry_string[BUF_MAX_S32_FMT + BUF_MAX_S32_FMT + 1 + 1 + 8];    /* 2 numbers, a space, a LF and some for luck */

    p_ui_text->type = UI_TEXT_TYPE_USTR_ARRAY;
    p_ui_text->text.array_handle_ustr = 0;

    generate_pages = 4 * ((max_page_y + 3) / 4);        /* 1..4  -> 4  */
                                                        /* 5..8  -> 8  */
                                                        /* 9..12 -> 12 */

    for(l_count = generate_pages, r_count = 1, swap = 0;
        l_count   > r_count;
        l_count--,  r_count++, swap ^= 1)
    {
        l_page = (swap ? r_count : l_count);
        r_page = (swap ? l_count : r_count);

        if(l_page <= max_page_y)
            consume_int(xsnprintf(l_string, sizeof32(l_string), S32_FMT, l_page));
        else
            consume_int(xsnprintf(l_string, sizeof32(l_string), "B"));

        if(r_page <= max_page_y)
            consume_int(xsnprintf(r_string, sizeof32(r_string), S32_FMT, r_page));
        else
            consume_int(xsnprintf(r_string, sizeof32(r_string), "B"));

        consume_int(xsnprintf(entry_string, sizeof32(entry_string), "%s %s" "\n", l_string, r_string));

        status_break(status = al_array_add(&p_ui_text->text.array_handle_ustr, BYTE, strlen32(entry_string), &array_init_block_u8, entry_string));
    }

    if(status_ok(status))
        status = al_array_add(&p_ui_text->text.array_handle_ustr, BYTE, 1, &array_init_block_u8, empty_string);

    if(status_fail(status))
    {
        status_assertc(status);
        ui_text_dispose(p_ui_text);
    }

    return(STATUS_OK);
}

static const DIALOG_CONTROL
warning_message1 =
{
    CONTROL_ID_MESSAGE1, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDTEXT_V },
    { DRT(LTLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
warning_message1_data = { UI_TEXT_INIT_RESID(MSG_PRINT_WARNING_MSG1), { 0, 0 /*centre_text*/ } };

static const DIALOG_CONTROL
warning_message2 =
{
    CONTROL_ID_MESSAGE2, DIALOG_MAIN_GROUP,
    { CONTROL_ID_MESSAGE1, CONTROL_ID_MESSAGE1 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDTEXT_V },
    { DRT(LBLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
warning_message2_data = { UI_TEXT_INIT_RESID(MSG_PRINT_WARNING_MSG2), { 0, 0 /*centre_text*/ } };

static const DIALOG_CONTROL
warning_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_MAIN_GROUP, DIALOG_MAIN_GROUP },
    { DIALOG_CONTENTS_CALC, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
warning_ok_data = { { DIALOG_COMPLETION_OK }, UI_TEXT_INIT_RESID(MSG_PRINT_WARNING_OK) };

static const DIALOG_CTL_CREATE
warning_dialog_create[] =
{
    { &dialog_main_group },
    { &warning_message1, &warning_message1_data },
    { &warning_message2, &warning_message2_data },
    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &warning_ok, &warning_ok_data }
};

_Check_return_
static STATUS
margin_warning(
    _DocuRef_   P_DOCU p_docu,
    P_PAPER p_paper,
    P_PRINT_CTRL p_print_ctrl)
{
    STATUS status;

    if(p_print_ctrl->flags.two_up)
        return(STATUS_OK);              /* bent to fit automatically */

    if(p_docu->flags.print_margin_warning_issued)
        return(STATUS_OK);

    if(!p_print_ctrl->flags.landscape)
    {
        if((  p_paper->lm                    <=  p_docu->phys_page_def.margin_left                                  ) &&
           (  p_paper->bm                    <=  p_docu->phys_page_def.margin_bottom                                ) &&
           ( (p_paper->x_size - p_paper->rm) >= (p_docu->phys_page_def.size_x - p_docu->phys_page_def.margin_right) ) &&
           ( (p_paper->y_size - p_paper->tm) >= (p_docu->phys_page_def.size_y - p_docu->phys_page_def.margin_top)   )
          )
            return(STATUS_OK);
    }
    else
    {
        if(( (p_paper->y_size - p_paper->tm) >= (p_docu->phys_page_def.size_x - p_docu->phys_page_def.margin_left)  ) &&
           (  p_paper->lm                    <=  p_docu->phys_page_def.margin_bottom                                ) &&
           (  p_paper->bm                    <=  p_docu->phys_page_def.margin_right                                 ) &&
           ( (p_paper->x_size - p_paper->rm) >= (p_docu->phys_page_def.size_y - p_docu->phys_page_def.margin_top)   )
          )
            return(STATUS_OK);
    }

    { /* put up a dialog box, warning that the margins may crop the text */
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, warning_dialog_create, elemof32(warning_dialog_create), 0);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = MSG_PRINT_WARNING_TITLE;
    dialog_cmd_process_dbox.bits.note_position = 1;
    /*dialog_cmd_process_dbox.p_proc_client = NULL;*/
    if((status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox)) == STATUS_OK)
        p_docu->flags.print_margin_warning_issued = 1;
    } /*block*/

    return(status);
}

extern void
print_percentage_initialise(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_PRINTER_PERCENTAGE p_printer_percentage,
    _In_        S32 page_count)
{
    assert(page_count > 0);
    if(page_count <= 0)         /* page_count should be >0, but lets be paranoid about division by 0 */
        page_count = 1;

    zero_struct_ptr(p_printer_percentage);
    p_printer_percentage->process_status.flags.foreground = 1;

    p_printer_percentage->final_page_count   = page_count;
    p_printer_percentage->current_page_count = 0;
    p_printer_percentage->percent_per_page   = 100 / page_count;

    process_status_begin(p_docu, &p_printer_percentage->process_status, PROCESS_STATUS_PERCENT);

    print_percentage_reflect(p_printer_percentage, 0);
}

extern void
print_percentage_finalise(
    _InoutRef_  P_PRINTER_PERCENTAGE p_printer_percentage)
{
    process_status_end(&p_printer_percentage->process_status);
}

extern void
print_percentage_page_inc(
    _InoutRef_  P_PRINTER_PERCENTAGE p_printer_percentage)
{
    p_printer_percentage->current_page_count++;

    print_percentage_reflect(p_printer_percentage, 0);
}

extern void
print_percentage_reflect(
    _InoutRef_  P_PRINTER_PERCENTAGE p_printer_percentage,
    _InVal_     S32 sub_percentage)
{
    S32 percent = p_printer_percentage->current_page_count * 100 / p_printer_percentage->final_page_count;

    p_printer_percentage->process_status.data.percent.current = percent + sub_percentage;

    process_status_reflect(&p_printer_percentage->process_status);
}

/*
main events
*/

_Check_return_
static STATUS
sk_print_msg_pre_save_2(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    if(p_of_op_format->of_template.paper)
    {
#if 0
        for(i = 0; i < array_elements(&p_docu->loaded_phys_page_defs); ++i)
            status_break(status = paper_save(p_docu, p_of_op_format, i + 1));
#endif

        if(status_ok(status))
            status = paper_save(p_docu, p_of_op_format, 0);

        if(status_ok(status))
            status = paper_scale_save(p_docu, p_of_op_format);

        if(status_ok(status))
            status = print_quality_save(p_docu, p_of_op_format);
    }

    return(status);
}

_Check_return_
static STATUS
sk_print_msg_save(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_SAVE p_msg_save)
{
    switch(p_msg_save->t5_msg_save_message)
    {
    case T5_MSG_SAVE__PRE_SAVE_2:
        return(sk_print_msg_pre_save_2(p_docu, p_msg_save->p_of_op_format));

    default:
        return(STATUS_OK);
    }
}

MAEVE_EVENT_PROTO(static, maeve_event_sk_print)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_SAVE:
        return(sk_print_msg_save(p_docu, (PC_MSG_SAVE) p_data));

    default:
        return(STATUS_OK);
    }
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_print);

_Check_return_
static STATUS
sk_print_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    /* bang in some useful defaults in case of panic (no Paper and/or PaperScale in file) */
    zero_struct(p_docu->phys_page_def);
    ustr_xstrkpy(ustr_bptr(p_docu->phys_page_def.ustr_name), elemof32(p_docu->phys_page_def.ustr_name), USTR_TEXT("A4"));
    p_docu->phys_page_def.size_x = 11906;
    p_docu->phys_page_def.size_y = 16840;

    p_docu->paper_scale = 100;

    p_docu->page_def.grid_size = 32;
    page_def_from_phys_page_def(&p_docu->page_def, &p_docu->phys_page_def, p_docu->paper_scale);
    page_def_validate(&p_docu->page_def);

    return(maeve_event_handler_add(p_docu, maeve_event_sk_print, (CLIENT_HANDLE) 0));
}

_Check_return_
static STATUS
sk_print_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    maeve_event_handler_del(p_docu, maeve_event_sk_print, (CLIENT_HANDLE) 0);

    al_array_dispose(&p_docu->loaded_phys_page_defs);

    return(STATUS_OK);
}

_Check_return_
static STATUS
maeve_services_sk_print_msg_initclose(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose)
{
    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT1:
        return(sk_print_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(sk_print_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_print)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_print_msg_initclose(p_docu, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* print/print extra
*
******************************************************************************/

T5_CMD_PROTO(extern, t5_cmd_print_intro)
{
    BOOL extra = (T5_CMD_PRINT_EXTRA_INTRO == t5_message);
    STATUS status = STATUS_OK;
    S32 last_page_y_read = last_page_y_non_blank(p_docu);
    BIT_NUMBER bit_number;
    PRINT_CALLBACK print_callback;
    S32 completion_code;

    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    print_callback.extra = extra;

    /* encode initial state of control(s) */

    print_copies_data.state   = 1;
    print_callback.all_or_range = 0; /* will set full ranges below */
    print_reverse_data.init_state  = 0;
    print_collate_data.init_state  = 0;
    print_two_up_data.init_state   = 0;

    print_range_y0_data.state = 1;
    print_range_y1_data.state = last_page_y_read;
    print_range_y0_data_control.max_val = last_page_y_read;
    print_range_y1_data_control.max_val = last_page_y_read;

    bitmap_clear(print_range_edit_validation, N_BITS_ARG(256));

    for(bit_number = CH_DIGIT_ZERO; bit_number <= CH_DIGIT_NINE; bit_number++)
        bitmap_bit_set(print_range_edit_validation, bit_number, N_BITS_ARG(256));

    {
    SS_RECOG_CONTEXT ss_recog_context;
    ss_recog_context_push(&ss_recog_context);
    bitmap_bit_set(print_range_edit_validation, g_ss_recog_context.list_sep_char, N_BITS_ARG(256));
    ss_recog_context_pull(&ss_recog_context);
    } /*block*/
    bitmap_bit_set(print_range_edit_validation, CH_SPACE, N_BITS_ARG(256));
    bitmap_bit_set(print_range_edit_validation, CH_HYPHEN_MINUS, N_BITS_ARG(256));
    bitmap_bit_set(print_range_edit_validation, CH_FULL_STOP, N_BITS_ARG(256)); /* <<< not a decimal point */
    bitmap_bit_set(print_range_edit_validation, '\n', N_BITS_ARG(256));
    bitmap_bit_set(print_range_edit_validation, 'B', N_BITS_ARG(256));
    bitmap_bit_set(print_range_edit_validation, 'b', N_BITS_ARG(256));

    /* loop back to here on EXTRA */
    for(;;)
    {
        print_callback.max_page_y = last_page_y_read;

        /* in order to mailshot, we must have a mailshot object loaded and something selected */
        s_print_dialog_create[elemof32(s_print_dialog_create) - 4 /*eek*/].p_dialog_control.p_dialog_control
            = object_available(OBJECT_ID_MAILSHOT) ? &print_mails : NULL;

        {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        if(print_callback.extra)
            dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, e_print_dialog_create, elemof32(e_print_dialog_create), MSG_DIALOG_PRINT_EXTRA_HELP_TOPIC);
        else
            dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, s_print_dialog_create, elemof32(s_print_dialog_create), MSG_DIALOG_PRINT_BASIC_HELP_TOPIC);
        if(p_docu->flags.draft_mode)
        {
            /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
            dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_PRINT_DRAFT;
        }
        else
            host_printer_name_query(&dialog_cmd_process_dbox.caption);
        dialog_cmd_process_dbox.bits.note_position = !print_callback.extra;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_print_intro;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &print_callback;
        completion_code = status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        ui_text_dispose(&dialog_cmd_process_dbox.caption);
        } /*block*/

        status_break(status);

        if(completion_code != CONTROL_ID_EXTRA)
            break;

        print_callback.extra = TRUE;
    } /*loop*/

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_print)
{
    STATUS status = STATUS_OK;
    S32 last_page_x_read = last_page_x_non_blank(p_docu);
    S32 last_page_y_read = last_page_y_non_blank(p_docu);
    PRINT_CTRL print_ctrl;
    S32 x0 = 1;
    S32 x1 = last_page_x_read;
    S32 y0 = 1;
    S32 y1 = last_page_y_read;
    PCTSTR tstr_fill = PTSTR_NONE;
    BOOL reverse = FALSE;
    S32 sides = 0; /* do all */

    /* y0,y1 & x0,x1 are used for straight-forward cases, like simple-all, simple-range and extra-all */
    /* initialised to all, trim values later for simple-range */

    print_ctrl.copies          = 1;
    print_ctrl.h_page_list     = 0;
    print_ctrl.flags.collate   = 0;
    print_ctrl.flags.landscape = 0;
    print_ctrl.flags.two_up    = 0;

    UNREFERENCED_PARAMETER_InVal_(t5_message); /* we use arglist to decide on print action */

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, ARG_PRINT_N_ARGS);

        if(arg_is_present(p_args, ARG_PRINT_COPIES))
            print_ctrl.copies = p_args[ARG_PRINT_COPIES].val.s32;

        if(arg_is_present(p_args, ARG_PRINT_RANGE) && p_args[ARG_PRINT_RANGE].val.s32)           /* 0=all/1=range */
        {
            /* page range specified */
            if(arg_is_present(p_args, ARG_PRINT_RANGE_LIST))
                tstr_fill = p_args[ARG_PRINT_RANGE_LIST].val.tstr;

            if(arg_is_present(p_args, ARG_PRINT_RANGE_Y0))
                y0 = p_args[ARG_PRINT_RANGE_Y0].val.s32;

            if(arg_is_present(p_args, ARG_PRINT_RANGE_Y1))
                y1 = p_args[ARG_PRINT_RANGE_Y1].val.s32;
        }

        if(arg_is_present(p_args, ARG_PRINT_SIDES))
            sides = p_args[ARG_PRINT_SIDES].val.s32;

        if(arg_is_present(p_args, ARG_PRINT_REVERSE))
            reverse = p_args[ARG_PRINT_REVERSE].val.fBool;

        if(arg_is_present(p_args, ARG_PRINT_COLLATE))
            print_ctrl.flags.collate = p_args[ARG_PRINT_COLLATE].val.fBool;

        if(arg_is_present(p_args, ARG_PRINT_TWO_UP))
            print_ctrl.flags.two_up = p_args[ARG_PRINT_TWO_UP].val.fBool;
    }

    assert(y0>0);
    assert(y1>0);
    assert(x0>0);
    assert(x1>0);

    if(PTSTR_NONE != tstr_fill)
        status = pagelist_fill_from(&print_ctrl.h_page_list, tstr_fill);
    else
        status = pagelist_add_range(&print_ctrl.h_page_list, x0, y0, x1, y1);
  /*else                    */
  /*    list already filled */

    if(status_ok(status))
    {
        /* pad to double (or quad) page boundaries if doing two-up */
        /* process reverse                   (take care if two-up) */
        /* process filter odd/even sides     (take care if two-up) */

        /*                 padding, mask, */
        /* normal,  one_up 1        0 */
        /* normal,  two_up 2        1 */
        /* reverse, one_up 2        1 */
        /* reverse, two_up 4        3 */
        S32 padding_mask = 1;

        if(!reverse && !print_ctrl.flags.two_up)
            padding_mask = 0;

        if(reverse && print_ctrl.flags.two_up)
            padding_mask = 3;

        while(array_elements(&print_ctrl.h_page_list) & padding_mask)
            status_break(status = pagelist_add_blank(&print_ctrl.h_page_list));
    }

    if(status_ok(status))
    {
        switch(sides) /* 0=both/1=odd/2=even */
        {
        default: default_unhandled();
#if CHECKING
        case 0:
#endif
            break;

        case 1:
        case 2:
            {
            const ARRAY_INDEX n_elements = array_elements(&print_ctrl.h_page_list);
            P_PAGE_ENTRY p_page_entry_dst = array_range(&print_ctrl.h_page_list, PAGE_ENTRY, 0, n_elements);
            PC_PAGE_ENTRY p_page_entry_src = p_page_entry_dst;
            ARRAY_INDEX page_index = 0;
            ARRAY_INDEX new_count = 0;

            if(2 == sides)
            {
                if(print_ctrl.flags.two_up)
                {
                    p_page_entry_src += 2;                      /* if even pages printing, skip first pair of pages */
                    page_index += 2;
                }
                else
                {
                    p_page_entry_src += 1;                      /* if even pages printing, skip first page */
                    page_index += 1;
                }
            }

            while(page_index < n_elements)
            {
                if(print_ctrl.flags.two_up)
                {
                    *p_page_entry_dst++ = *p_page_entry_src++;  /* copy two */
                    *p_page_entry_dst++ = *p_page_entry_src++;
                    new_count += 2;

                    p_page_entry_src += 2;                      /* skip next pair of pages */

                    page_index += 4;                            /* copied two, skipped two */
                }
                else
                {
                    *p_page_entry_dst++ = *p_page_entry_src++;  /* copy one */
                    new_count += 1;

                    p_page_entry_src += 1;                      /* skip one */

                    page_index += 2;                            /* copied one, skipped one */
                }
            }

            assert(new_count <= n_elements);
            if(new_count < n_elements)
                al_array_shrink_by(&print_ctrl.h_page_list, (new_count - n_elements));

            break;
            }
        }

        if(reverse)
        {
            U32 n_elements = array_elements32(&print_ctrl.h_page_list);
            if(print_ctrl.flags.two_up)
            {
                const U32 element_width = 2 * sizeof32(PAGE_ENTRY);
                n_elements /= 2;
                /* NB - array_range would barf in CHECKING with two_up */
                memrev32(array_base(&print_ctrl.h_page_list, PAGE_ENTRY), n_elements, element_width);
            }
            else
            {
                memrev32(array_range(&print_ctrl.h_page_list, PAGE_ENTRY, 0, n_elements), n_elements, sizeof32(PAGE_ENTRY));
            }
        }
    }

    if(status_ok(status))
    {
        if(p_docu->flags.draft_mode)
            status = object_skel(p_docu, T5_MSG_DRAFT_PRINT, &print_ctrl);
        else
        {
            PAPER paper;

            if(status_ok(status = host_read_printer_paper_details(&paper)))
            {
                print_ctrl.flags.landscape = p_docu->page_def.landscape;

                if(status_ok(status = margin_warning(p_docu, &paper, &print_ctrl)))
                    status = host_print_document(p_docu, &print_ctrl);
                else if(STATUS_CANCEL == status)
                    status = STATUS_OK;
            }
        }
    }

    al_array_dispose(&print_ctrl.h_page_list);

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_paper)
{
    STATUS status = STATUS_OK;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, ARG_PAPER_N_ARGS);
    P_PHYS_PAGE_DEF p_phys_page_def = &p_docu->phys_page_def;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(arg_is_present(p_args, ARG_PAPER_LOADED_ID) && (p_args[ARG_PAPER_LOADED_ID].val.s32 > 0))
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_phys_page_def), TRUE);

        assert(p_docu == p_docu_from_config());

        if(NULL == (p_phys_page_def = al_array_extend_by(&p_docu->loaded_phys_page_defs, PHYS_PAGE_DEF, 1, &array_init_block, &status)))
            return(status);
    }
    else
    {
        /* these are only relevant to a docu's paper settings */
        if(arg_is_present(p_args, ARG_PAPER_MARGIN_COL))
            p_docu->page_def.margin_col = (PIXIT) p_args[ARG_PAPER_MARGIN_COL].val.fp_pixit;
        if(arg_is_present(p_args, ARG_PAPER_MARGIN_ROW))
            p_docu->page_def.margin_row = (PIXIT) p_args[ARG_PAPER_MARGIN_ROW].val.fp_pixit;

        if(arg_is_present(p_args, ARG_PAPER_GRID_SIZE))
            p_docu->page_def.grid_size = (S32) p_args[ARG_PAPER_GRID_SIZE].val.f64; /*FP_POINTS*/
        if(arg_is_present(p_args, ARG_PAPER_GRID_FAINT))
            p_docu->flags.faint_grid = p_args[ARG_PAPER_GRID_FAINT].val.fBool;
    }

    if(arg_is_present(p_args, ARG_PAPER_NAME))
        ustr_xstrkpy(ustr_bptr(p_phys_page_def->ustr_name), elemof32(p_phys_page_def->ustr_name), p_args[ARG_PAPER_NAME].val.ustr);

    if(arg_is_present(p_args, ARG_PAPER_ORIENTATION))
        p_phys_page_def->landscape = (U8) p_args[ARG_PAPER_ORIENTATION].val.s32;

    if(arg_is_present(p_args, ARG_PAPER_HEIGHT))
        p_phys_page_def->size_y = (PIXIT) p_args[ARG_PAPER_HEIGHT].val.fp_pixit;
    if(arg_is_present(p_args, ARG_PAPER_WIDTH))
        p_phys_page_def->size_x = (PIXIT) p_args[ARG_PAPER_WIDTH].val.fp_pixit;

    if(arg_is_present(p_args, ARG_PAPER_MARGIN_TOP))
        p_phys_page_def->margin_top = (PIXIT) p_args[ARG_PAPER_MARGIN_TOP].val.fp_pixit;
    if(arg_is_present(p_args, ARG_PAPER_MARGIN_BOTTOM))
        p_phys_page_def->margin_bottom = (PIXIT) p_args[ARG_PAPER_MARGIN_BOTTOM].val.fp_pixit;
    if(arg_is_present(p_args, ARG_PAPER_MARGIN_LEFT))
        p_phys_page_def->margin_left = (PIXIT) p_args[ARG_PAPER_MARGIN_LEFT].val.fp_pixit;
    if(arg_is_present(p_args, ARG_PAPER_MARGIN_RIGHT))
        p_phys_page_def->margin_right = (PIXIT) p_args[ARG_PAPER_MARGIN_RIGHT].val.fp_pixit;

    if(arg_is_present(p_args, ARG_PAPER_MARGIN_BIND))
        p_phys_page_def->margin_bind = (PIXIT) p_args[ARG_PAPER_MARGIN_BIND].val.fp_pixit;

    if(arg_is_present(p_args, ARG_PAPER_MARGIN_OE_SWAP))
        p_phys_page_def->margin_oe_swap = p_args[ARG_PAPER_MARGIN_OE_SWAP].val.u8n;

    if(p_phys_page_def == &p_docu->phys_page_def)
    {
        /* copy phys_page_def to page_def and transform */
        page_def_from_phys_page_def(&p_docu->page_def, p_phys_page_def, p_docu->paper_scale);
        page_def_validate(&p_docu->page_def);

        p_docu->flags.print_margin_warning_issued = 0;

        reformat_all(p_docu);
    }

    return(status);
}

#define ARG_PAPER_SCALE_VALUE 0

T5_CMD_PROTO(extern, t5_cmd_paper_scale)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(arg_is_present(p_args, ARG_PAPER_SCALE_VALUE))
        p_docu->paper_scale = p_args[ARG_PAPER_SCALE_VALUE].val.s32;

    /* copy phys_page_def to page_def and transform */
    page_def_from_phys_page_def(&p_docu->page_def, &p_docu->phys_page_def, p_docu->paper_scale);
    page_def_validate(&p_docu->page_def);

    reformat_all(p_docu);

    return(STATUS_OK);
}

/******************************************************************************
*
* print quality
*
******************************************************************************/

#define ARG_PRINT_QUALITY 0

_Check_return_
static STATUS
print_quality_save(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_PRINT_QUALITY, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        p_args[ARG_PRINT_QUALITY].val.u8n = (U8) p_docu->flags.draft_mode;
        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
        arglist_dispose(&arglist_handle);
    }

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_print_quality)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_docu->flags.draft_mode = 0;

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

        if(arg_is_present(p_args, ARG_PRINT_QUALITY))
            p_docu->flags.draft_mode = (p_args[ARG_PRINT_QUALITY].val.u8n == 1); /* 0=normal/1=draft */
    }

    return(STATUS_OK);
}

/* end of sk_print.c */
