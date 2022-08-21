/* ob_skspt.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Split off UI for Fireworkz skeleton */

/* SKS October 1995 */

#include "common/gflags.h"

#include "ob_skspt/ob_skspt.h"

#include "ob_skspt/ui_styin.h"

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_SKEL_SPLIT)
extern PC_U8 rb_skel_split_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_SKEL_SPLIT &rb_skel_split_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_SKEL_SPLIT LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_SKEL_SPLIT DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_SKEL_SPLIT DONT_LOAD_RESOURCES

#ifndef          __ce_edit_h
#include "ob_cells/ce_edit.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

/******************************************************************************
*
* add columns dialog box
*
******************************************************************************/

enum ADD_CR_CONTROL_IDS
{
    ADD_CR_ID_NUMBER_LABEL = 32,
    ADD_CR_ID_NUMBER
};

/*
cols
*/

static const DIALOG_CONTROL
add_cr_number_label =
{
    ADD_CR_ID_NUMBER_LABEL, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, ADD_CR_ID_NUMBER, DIALOG_CONTROL_SELF, ADD_CR_ID_NUMBER },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
add_cols_number_label_data = { UI_TEXT_INIT_RESID(SKEL_SPLIT_MSG_DIALOG_INSERT_TABLE_COLS) };

static const DIALOG_CONTROL
add_cr_number =
{
    ADD_CR_ID_NUMBER, DIALOG_MAIN_GROUP,
    { ADD_CR_ID_NUMBER_LABEL, DIALOG_CONTROL_PARENT },
    { DIALOG_STDSPACING_H, 0, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_S32), 1 /*tabstop*/ }
};

static const UI_CONTROL_S32
add_cols_n_table_cols_bump_control = { 1, 1000 };

static const DIALOG_CONTROL_DATA_BUMP_S32
add_cols_number_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &add_cols_n_table_cols_bump_control } /* BUMP_XX */, 1 };

/*
CMD
*/

static const DIALOG_CONTROL_ID
add_cr_ok_data_argmap[] = { ADD_CR_ID_NUMBER };

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
add_cols_ok_command = { T5_CMD_COLS_ADD_AFTER, OBJECT_ID_SKEL, NULL, add_cr_ok_data_argmap, { 0, 0, 0, 1 /*lookup_arglist*/} };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
add_cols_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_BUTTON_ADD), &add_cols_ok_command };

static const DIALOG_CTL_CREATE
add_cols_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &add_cr_number_label },     &add_cols_number_label_data },
    { { &add_cr_number },           &add_cols_number_data },

    { { &defbutton_ok },            &add_cols_ok_data },
    { { &stdbutton_cancel },        &stdbutton_cancel_data }
};

T5_CMD_PROTO(static, t5_cmd_cols_add_after_intro)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, add_cols_ctl_create, elemof32(add_cols_ctl_create), SKEL_SPLIT_MSG_DIALOG_COLS_ADD);
    dialog_cmd_process_dbox.help_topic_resource_id = SKEL_SPLIT_MSG_DIALOG_ADD_CR_HELP_TOPIC;
  /*dialog_cmd_process_dbox.p_proc_client = NULL;*/
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

/******************************************************************************
*
* insert columns dialog box
*
******************************************************************************/

/*
CMD
*/

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
insert_cols_ok_command = { T5_CMD_COLS_INSERT_BEFORE, OBJECT_ID_SKEL, NULL, add_cr_ok_data_argmap, { 0, 0, 0, 1 /*lookup_arglist*/} };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
insert_cols_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_BUTTON_INSERT), &insert_cols_ok_command };

static const DIALOG_CTL_CREATE
insert_cols_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &add_cr_number_label },     &add_cols_number_label_data },
    { { &add_cr_number },           &add_cols_number_data },

    { { &defbutton_ok },            &insert_cols_ok_data },
    { { &stdbutton_cancel },        &stdbutton_cancel_data }
};

T5_CMD_PROTO(static, t5_cmd_cols_insert_before_intro)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, insert_cols_ctl_create, elemof32(insert_cols_ctl_create), SKEL_SPLIT_MSG_DIALOG_COLS_INSERT);
    dialog_cmd_process_dbox.help_topic_resource_id = SKEL_SPLIT_MSG_DIALOG_ADD_CR_HELP_TOPIC;
  /*dialog_cmd_process_dbox.p_proc_client = NULL;*/
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

/******************************************************************************
*
* add rows dialog box
*
******************************************************************************/

/*
rows
*/

static const DIALOG_CONTROL_DATA_TEXTLABEL
add_rows_number_label_data = { UI_TEXT_INIT_RESID(SKEL_SPLIT_MSG_DIALOG_INSERT_TABLE_ROWS) };

static const UI_CONTROL_S32
add_rows_n_table_rows_bump_control = { 1, 1000000 };

static const DIALOG_CONTROL_DATA_BUMP_S32
add_rows_number_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &add_rows_n_table_rows_bump_control } /* BUMP_XX */, 1 };

/*
CMD
*/

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
add_rows_ok_command = { T5_CMD_ROWS_ADD_AFTER, OBJECT_ID_SKEL, NULL, add_cr_ok_data_argmap, { 0, 0, 0, 1 /*lookup_arglist*/} };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
add_rows_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_BUTTON_ADD), &add_rows_ok_command };

static const DIALOG_CTL_CREATE
add_rows_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &add_cr_number_label },     &add_rows_number_label_data },
    { { &add_cr_number },           &add_rows_number_data },

    { { &defbutton_ok },            &add_rows_ok_data },
    { { &stdbutton_cancel },        &stdbutton_cancel_data }
};

T5_CMD_PROTO(static, t5_cmd_rows_add_after_intro)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, add_rows_ctl_create, elemof32(add_rows_ctl_create), SKEL_SPLIT_MSG_DIALOG_ROWS_ADD);
    dialog_cmd_process_dbox.help_topic_resource_id = SKEL_SPLIT_MSG_DIALOG_ADD_CR_HELP_TOPIC;
  /*dialog_cmd_process_dbox.p_proc_client = NULL;*/
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

/******************************************************************************
*
* insert rows dialog box
*
******************************************************************************/

/*
CMD
*/

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
insert_rows_ok_command = { T5_CMD_ROWS_INSERT_BEFORE, OBJECT_ID_SKEL, NULL, add_cr_ok_data_argmap, { 0, 0, 0, 1 /*lookup_arglist*/} };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
insert_rows_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_BUTTON_INSERT), &insert_rows_ok_command };

static const DIALOG_CTL_CREATE
insert_rows_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &add_cr_number_label },     &add_rows_number_label_data },
    { { &add_cr_number },           &add_rows_number_data },

    { { &defbutton_ok },            &insert_rows_ok_data },
    { { &stdbutton_cancel },        &stdbutton_cancel_data }
};

T5_CMD_PROTO(static, t5_cmd_rows_insert_before_intro)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, insert_rows_ctl_create, elemof32(insert_rows_ctl_create), SKEL_SPLIT_MSG_DIALOG_ROWS_INSERT);
    dialog_cmd_process_dbox.help_topic_resource_id = SKEL_SPLIT_MSG_DIALOG_ADD_CR_HELP_TOPIC;
  /*dialog_cmd_process_dbox.p_proc_client = NULL;*/
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

/******************************************************************************
*
* insert table dialog box
*
******************************************************************************/

enum INSERT_TABLE_CONTROL_IDS
{
    INSERT_TABLE_ID_COLS_LABEL = 64,
    INSERT_TABLE_ID_COLS,
    INSERT_TABLE_ID_ROWS_LABEL,
    INSERT_TABLE_ID_ROWS
};

/*
cols
*/

static const DIALOG_CONTROL
insert_table_cols_label =
{
    INSERT_TABLE_ID_COLS_LABEL, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, INSERT_TABLE_ID_COLS, DIALOG_CONTROL_SELF, INSERT_TABLE_ID_COLS },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
insert_table_cols_label_data = { UI_TEXT_INIT_RESID(SKEL_SPLIT_MSG_DIALOG_INSERT_TABLE_COLS) };

static const DIALOG_CONTROL
insert_table_cols =
{
    INSERT_TABLE_ID_COLS, DIALOG_MAIN_GROUP,
    { INSERT_TABLE_ID_COLS_LABEL, DIALOG_CONTROL_PARENT },
    { DIALOG_LABELGAP_H, 0, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_S32), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_S32
insert_table_cols_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &add_cols_n_table_cols_bump_control } /* BUMP_XX */, 4 };

/*
rows
*/

static const DIALOG_CONTROL
insert_table_rows_label =
{
    INSERT_TABLE_ID_ROWS_LABEL, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_SELF, INSERT_TABLE_ID_ROWS, INSERT_TABLE_ID_COLS_LABEL, INSERT_TABLE_ID_ROWS },
    { DIALOG_CONTENTS_CALC, 0, 0, 0 },
    { DRT(RTRB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
insert_table_rows_label_data = { UI_TEXT_INIT_RESID(SKEL_SPLIT_MSG_DIALOG_INSERT_TABLE_ROWS) };

static const DIALOG_CONTROL
insert_table_rows =
{
    INSERT_TABLE_ID_ROWS, DIALOG_MAIN_GROUP,
    { INSERT_TABLE_ID_COLS, INSERT_TABLE_ID_COLS, INSERT_TABLE_ID_COLS },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_S32), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_BUMP_S32
insert_table_rows_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &add_rows_n_table_rows_bump_control } /* BUMP_XX */, 9 };

/*
CMD
*/

static const DIALOG_CONTROL_ID
insert_table_insert_data_argmap[] = { INSERT_TABLE_ID_COLS, INSERT_TABLE_ID_ROWS };

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
insert_table_insert_command = { T5_CMD_INSERT_TABLE, OBJECT_ID_SKEL, NULL, insert_table_insert_data_argmap, { 0, 0, 0, 1 /*lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
insert_table_insert_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_BUTTON_INSERT), &insert_table_insert_command };

static const DIALOG_CTL_CREATE
insert_table_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &insert_table_cols_label }, &insert_table_cols_label_data },
    { { &insert_table_cols },       &insert_table_cols_data },
    { { &insert_table_rows_label }, &insert_table_rows_label_data },
    { { &insert_table_rows },       &insert_table_rows_data },

    { { &defbutton_ok },            &insert_table_insert_data },
    { { &stdbutton_cancel },        &stdbutton_cancel_data }
};

/******************************************************************************
*
* insert a table into a document
*
******************************************************************************/

T5_CMD_PROTO(static, t5_cmd_insert_table_intro)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, insert_table_ctl_create, elemof32(insert_table_ctl_create), SKEL_SPLIT_MSG_DIALOG_INSERT_TABLE_CAPTION);
    dialog_cmd_process_dbox.help_topic_resource_id = SKEL_SPLIT_MSG_DIALOG_INSERT_TABLE_HELP_TOPIC;
  /*dialog_cmd_process_dbox.p_proc_client = NULL;*/
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

/******************************************************************************
*
* add a table style to a docu_area;
* the table is assumed to start at column index 1,
* with a blank column index 0 for moving the table about
*
******************************************************************************/

_Check_return_
static STATUS
table_style_add(
    _DocuRef_   P_DOCU p_docu,
    P_DOCU_AREA p_docu_area,
    _InVal_     COL col_extra)
{
    STATUS status = STATUS_OK;
    S32 style_handle_base_table = 0;
    PC_STYLE p_style_base_table = NULL;

    { /* attempt to add a BaseTable style area if said style exists */
    DOCU_AREA docu_area_table;
    docu_area_table = *p_docu_area;
    docu_area_table.tl.slr.col += col_extra;
    status_return(status = table_apply_style_basetable(p_docu, &docu_area_table));
    style_handle_base_table = (S32) (status);
    if(0 != style_handle_base_table)
        p_style_base_table = p_style_from_handle(p_docu, style_handle_base_table);
    } /*block*/

    {
    STYLE style_table;
    STYLE_HANDLE style_handle_table = 0;

    style_init(&style_table);

    /* invent a new, appropriately named, style for this table (which will be 'renumbered' on file insertion) */
    status_return(table_invent_style_name(p_docu, &style_table));

    /* only apply style elements that are not already defined in the base table style */
    table_add_some_style_elements(p_docu, &style_table, p_style_base_table);

    status_return(status = style_handle_add(p_docu, &style_table));
    style_handle_table = (STYLE_HANDLE) status;

    { /* style resources are now owned by document */
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    DOCU_AREA docu_area_table;

    STYLE_DOCU_AREA_ADD_HANDLE(&style_docu_area_add_parm, style_handle_table);

    docu_area_table = *p_docu_area;
    docu_area_table.tl.slr.col += col_extra;

    status_return(style_docu_area_add(p_docu, &docu_area_table, &style_docu_area_add_parm));
    } /*block*/
    } /*block*/

    {
    int pass;

    for(pass = 1; pass <= 2; ++pass)
    {
        DOCU_AREA docu_area_cols = *p_docu_area;
        STYLE style_cols;

        style_init(&style_cols);

        /* derive initial state from BaseTable style if possible */
        if(1 == pass)
        {
            table_cols_add_some_style_elements(p_docu, &style_cols, p_style_base_table);
        }
        else /* 2 == pass */
        {
            /* if added, added as separate region so it can be deleted if required */
            if( (NULL == p_style_base_table) || !style_bit_test(p_style_base_table, STYLE_SW_PS_NEW_OBJECT) )
            {
                if(global_preferences.ss_edit_in_cell) /* SKS 09jan97 turn this section back on with this extra condition */
                {
                    if(object_present(OBJECT_ID_SS))
                    {
                        style_cols.para_style.new_object = OBJECT_ID_SS;
                        style_bit_set(&style_cols, STYLE_SW_PS_NEW_OBJECT);
                    }
                }
            }
        }

        if(!style_selector_any(&style_cols.selector))
            continue;

        /* empty column(s) at left to move table about as well as independent width regions for columns in table */
        while(docu_area_cols.tl.slr.col < docu_area_cols.br.slr.col)
        {
            DOCU_AREA docu_area_inner = docu_area_cols;
            STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
            STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style_cols);
            style_docu_area_add_parm.add_without_subsume = TRUE;
            docu_area_inner.br.slr.col = docu_area_inner.tl.slr.col + 1;
            status_return(style_docu_area_add(p_docu, &docu_area_inner, &style_docu_area_add_parm));
            ++docu_area_cols.tl.slr.col;
        }
    }
    } /*block*/

    return(STATUS_OK);
}

T5_MSG_PROTO(static, t5_msg_table_style_add, P_DOCU_AREA p_docu_area)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    return(table_style_add(p_docu, p_docu_area, 1));
}

T5_CMD_PROTO(static, t5_cmd_insert_table)
{
    STATUS status = STATUS_OK;
    COL n_cols_add = 1;
    ROW n_rows_add = 1;
    COL col_extra = 0;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_docu->flags.base_single_col)
        col_extra = 1;

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);

        if(arg_is_present(p_args, 0))
            n_cols_add = p_args[0].val.col;

        if(arg_is_present(p_args, 1))
            n_rows_add = p_args[1].val.row;
    }

    n_cols_add += col_extra; /* we may add a spare column at the start to shunt the table around with */

    for(;;) /* loop for structure */
    {
        POSITION position, output_position;
        DOCU_AREA docu_area;

        /* insert space at caret */
        position = p_docu->cur;

        docu_area_init(&docu_area);
        docu_area.br.slr.col = n_cols_add;
        docu_area.br.slr.row = n_rows_add;

        status_break(status = cells_docu_area_insert(p_docu, &position, &docu_area, &output_position));

        docu_area.tl = output_position;
        docu_area.br.slr.col = docu_area.tl.slr.col + n_cols_add;
        docu_area.br.slr.row = docu_area.tl.slr.row + n_rows_add;

        status_break(status = format_col_row_extents_set(p_docu,
                                                         MAX(docu_area.br.slr.col, n_cols_logical(p_docu)),
                                                         MAX(docu_area.br.slr.row, n_rows(p_docu))));

        status_break(status = table_style_add(p_docu, &docu_area, col_extra));

        /* reposition in first entry of table */
        p_docu->cur = output_position;
        p_docu->cur.slr.col += col_extra;

        caret_show_claim(p_docu, OBJECT_ID_CELLS, FALSE);

        break; /* out of loop for structure */
        /*NOTREACHED*/
    }

    return(status);
}

static struct SORT_CMD_STATICS
{
    ARGLIST_HANDLE arglist_handle;
    P_DOCU p_docu;
    COL col_s;
    COL col_e;
}
sort_cmd_statics;

PROC_QSORT_PROTO(static, proc_qsort_compare_rows, ROW)
{
    QSORT_ARG1_VAR_DECL(PC_ROW, p_row_1);
    QSORT_ARG2_VAR_DECL(PC_ROW, p_row_2);
    const U32 n_args = n_arglist_args(&sort_cmd_statics.arglist_handle);
    U32 arg_idx;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&sort_cmd_statics.arglist_handle, n_args);
    int res = 0;
    SLR slr_1;
    SLR slr_2;

    slr_1.row = *p_row_1;
    slr_2.row = *p_row_2;

    for(arg_idx = 0; arg_idx < n_args; arg_idx += 2)
    {
        const PC_ARGLIST_ARG p_arg = &p_args[arg_idx];

        if(ARG_TYPE_NONE != p_arg->type) /* skip any blank specifiers */
        {
            slr_1.col = slr_2.col = p_arg->val.col;

            if(slr_1.col >= sort_cmd_statics.col_s && slr_1.col < sort_cmd_statics.col_e)
            {
                OBJECT_DATA object_data_1, object_data_2;

                status_consume(object_data_from_slr(sort_cmd_statics.p_docu, &object_data_1, &slr_1));
                status_consume(object_data_from_slr(sort_cmd_statics.p_docu, &object_data_2, &slr_2));

                if(     (P_DATA_NONE != object_data_1.u.p_object) && (P_DATA_NONE == object_data_2.u.p_object))
                    res = 1;
                else if((P_DATA_NONE == object_data_1.u.p_object) && (P_DATA_NONE != object_data_2.u.p_object))
                    res = -1;
                else if((P_DATA_NONE != object_data_1.u.p_object) && (P_DATA_NONE != object_data_2.u.p_object))
                {
                    if(object_data_1.object_id > object_data_2.object_id)
                        res = 1;
                    else if(object_data_1.object_id < object_data_2.object_id)
                        res = -1;
                    else
                    {
                        OBJECT_COMPARE object_compare;
                        object_compare.p_object_1 = object_data_1.u.p_object;
                        object_compare.p_object_2 = object_data_2.u.p_object;
                        object_compare.res = 0;
                        status_assert(object_call_id(object_data_1.object_id, sort_cmd_statics.p_docu, T5_MSG_OBJECT_COMPARE, &object_compare));
                        res = (int) object_compare.res;
                    }
                }

                if(res && arg_is_present(p_args, arg_idx + 1))
                    if(p_args[arg_idx + 1].val.fBool)
                        res = (res > 0) ? -1 : 1;
            }
        }

        if(res)
            break;
    }

    return(res);
}

/******************************************************************************
*
* sort intro dialog
*
******************************************************************************/

enum SORT_CTL_CONTROL_IDS
{
    SORT_ID_COL_0 = 827,
    SORT_ID_ORDER_0,

    SORT_ID_COL_1,
    SORT_ID_ORDER_1,

    SORT_ID_COL_2,
    SORT_ID_ORDER_2,

    SORT_ID_COL_3,
    SORT_ID_ORDER_3,

    SORT_ID_COL_4,
    SORT_ID_ORDER_4
};

static const ARG_TYPE
args_cmd_sort_intro[] =
{
#define ARG_SORT_COL_0   0
#define ARG_SORT_ORDER_0 1
    ARG_TYPE_USTR, ARG_TYPE_BOOL,

#define ARG_SORT_COL_1   2
#define ARG_SORT_ORDER_1 3
    ARG_TYPE_USTR, ARG_TYPE_BOOL,

#define ARG_SORT_COL_2   4
#define ARG_SORT_ORDER_2 5
    ARG_TYPE_USTR, ARG_TYPE_BOOL,

#define ARG_SORT_COL_3   6
#define ARG_SORT_ORDER_3 7
    ARG_TYPE_USTR, ARG_TYPE_BOOL,

#define ARG_SORT_COL_4   8
#define ARG_SORT_ORDER_4 9
    ARG_TYPE_USTR, ARG_TYPE_BOOL,

    ARG_TYPE_NONE
};

static const DIALOG_CONTROL_ID
sort_intro_argmap[] =
{
    SORT_ID_COL_0, SORT_ID_ORDER_0,
    SORT_ID_COL_1, SORT_ID_ORDER_1,
    SORT_ID_COL_2, SORT_ID_ORDER_2,
    SORT_ID_COL_3, SORT_ID_ORDER_3,
    SORT_ID_COL_4, SORT_ID_ORDER_4
};

#define SORT_INTRO_COL_FIELDS_H DIALOG_STDEDITOVH_H + DIALOG_SYSCHARSL_H(4)

static const DIALOG_CONTROL
sort_intro_col[5] =
{
    {
        SORT_ID_COL_0, DIALOG_MAIN_GROUP, { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
        { 0, 0, SORT_INTRO_COL_FIELDS_H, DIALOG_STDEDIT_V }, { DRT(LTLT, EDIT), 1 /*tabstop*/ }
    },
    {
        SORT_ID_COL_1, DIALOG_MAIN_GROUP, { SORT_ID_COL_0, SORT_ID_COL_0, SORT_ID_COL_0 },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V }, { DRT(LBRT, EDIT), 1 /*tabstop*/ }
    },
    {
        SORT_ID_COL_2, DIALOG_MAIN_GROUP, { SORT_ID_COL_1, SORT_ID_COL_1, SORT_ID_COL_1 },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V }, { DRT(LBRT, EDIT), 1 /*tabstop*/ }
    },
    {
        SORT_ID_COL_3, DIALOG_MAIN_GROUP, { SORT_ID_COL_2, SORT_ID_COL_2, SORT_ID_COL_2 },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V }, { DRT(LBRT, EDIT), 1 /*tabstop*/ }
    },
    {
        SORT_ID_COL_4, DIALOG_MAIN_GROUP, { SORT_ID_COL_3, SORT_ID_COL_3, SORT_ID_COL_3 },
        { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V }, { DRT(LBRT, EDIT), 1 /*tabstop*/ }
    }
};

static const DIALOG_CONTROL_DATA_EDIT
sort_intro_col_data[5] =
{
    {
        { { FRAMED_BOX_EDIT } } /*EDIT_XX*/
    },
    {
        { { FRAMED_BOX_EDIT } } /*EDIT_XX*/
    },
    {
        { { FRAMED_BOX_EDIT } } /*EDIT_XX*/
    },
    {
        { { FRAMED_BOX_EDIT } } /*EDIT_XX*/
    },
    {
        { { FRAMED_BOX_EDIT } } /*EDIT_XX*/
    }
};

static const DIALOG_CONTROL
sort_intro_order[5] =
{
    {
        SORT_ID_ORDER_0, DIALOG_MAIN_GROUP, { SORT_ID_COL_0, SORT_ID_COL_0, DIALOG_CONTROL_SELF, SORT_ID_COL_0 },
        { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 }, { DRT(RTLB, CHECKBOX), 1 /*tabstop*/ }
    },
    {
        SORT_ID_ORDER_1, DIALOG_MAIN_GROUP, { SORT_ID_ORDER_0, SORT_ID_COL_1, SORT_ID_ORDER_0, SORT_ID_COL_1 },
        { 0 }, { DRT(LTRB, CHECKBOX), 1 /*tabstop*/ }
    },
    {
        SORT_ID_ORDER_2, DIALOG_MAIN_GROUP, { SORT_ID_ORDER_1, SORT_ID_COL_2, SORT_ID_ORDER_1, SORT_ID_COL_2 },
        { 0 }, { DRT(LTRB, CHECKBOX), 1 /*tabstop*/ }
    },
    {
        SORT_ID_ORDER_3, DIALOG_MAIN_GROUP, { SORT_ID_ORDER_2, SORT_ID_COL_3, SORT_ID_ORDER_2, SORT_ID_COL_3 },
        { 0 }, { DRT(LTRB, CHECKBOX), 1 /*tabstop*/ }
    },
    {
        SORT_ID_ORDER_4, DIALOG_MAIN_GROUP, { SORT_ID_ORDER_3, SORT_ID_COL_4, SORT_ID_ORDER_3, SORT_ID_COL_4 },
        { 0 }, { DRT(LTRB, CHECKBOX), 1 /*tabstop*/ }
    }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
sort_intro_order_data[5] =
{
    {
        { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_SORT_ORDER)
    },
    {
        { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_SORT_ORDER)
    },
    {
        { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_SORT_ORDER)
    },
    {
        { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_SORT_ORDER)
    },
    {
        { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_SORT_ORDER)
    }
};

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
sort_intro_ok_command = { T5_CMD_SORT, OBJECT_ID_SKEL, args_cmd_sort_intro, sort_intro_argmap, { 0 } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
sort_intro_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_BUTTON_APPLY), &sort_intro_ok_command };

static const DIALOG_CTL_CREATE
sort_intro_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &sort_intro_col[0] },   &sort_intro_col_data[0] },
    { { &sort_intro_order[0] }, &sort_intro_order_data[0] },
    { { &sort_intro_col[1] },   &sort_intro_col_data[1] },
    { { &sort_intro_order[1] }, &sort_intro_order_data[1] },
    { { &sort_intro_col[2] },   &sort_intro_col_data[2] },
    { { &sort_intro_order[2] }, &sort_intro_order_data[2] },
    { { &sort_intro_col[3] },   &sort_intro_col_data[3] },
    { { &sort_intro_order[3] }, &sort_intro_order_data[3] },
    { { &sort_intro_col[4] },   &sort_intro_col_data[4] },
    { { &sort_intro_order[4] }, &sort_intro_order_data[4] },

    { { &defbutton_ok }, &sort_intro_ok_data },
    { { &stdbutton_cancel }, &stdbutton_cancel_data }
};

_Check_return_
static STATUS
dialog_sort_intro_preprocess_command(
    _InoutRef_  P_DIALOG_MSG_PREPROCESS_COMMAND p_dialog_msg_preprocess_command)
{
    const U32 n_args = n_arglist_args(&p_dialog_msg_preprocess_command->arglist_handle);
    const P_ARGLIST_ARG p_args = p_arglist_args(&p_dialog_msg_preprocess_command->arglist_handle, n_args);
    U32 arg_idx;

    for(arg_idx = 0; arg_idx < n_args; ++arg_idx)
    {
        switch(arg_idx)
        {
        case ARG_SORT_COL_0:
        case ARG_SORT_COL_1:
        case ARG_SORT_COL_2:
        case ARG_SORT_COL_3:
        case ARG_SORT_COL_4:
            {
            if(arg_is_present(p_args, arg_idx))
            {
                S32 col;
                S32 len;
                assert((p_args[arg_idx].type & ARG_TYPE_MASK) == ARG_TYPE_USTR);
                len = stox(p_args[arg_idx].val.ustr, &col);
                arg_dispose(&p_dialog_msg_preprocess_command->arglist_handle, arg_idx);
                p_args[arg_idx].type = ARG_TYPE_NONE;
                if(len)
                {
                    p_args[arg_idx].type = ARG_TYPE_S32;
                    p_args[arg_idx].val.s32 = col;
                }
            }
            break;
            }

        default:
            break;
        }
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_sort_intro)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PREPROCESS_COMMAND:
        return(dialog_sort_intro_preprocess_command((P_DIALOG_MSG_PREPROCESS_COMMAND) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
sort_docu_area(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _OutRef_    P_ROW p_row_s)
{
    STATUS status = STATUS_OK;
    ROW row_s, row_e, n_rows;
    ARRAY_HANDLE h_sort_rows = 0;

    limits_from_docu_area(p_docu, &sort_cmd_statics.col_s, &sort_cmd_statics.col_e, &row_s, &row_e, p_docu_area);

    *p_row_s = row_s;

    n_rows = row_e - row_s;

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(ROW), FALSE);
    consume_ptr(al_array_alloc(&h_sort_rows, ROW, n_rows, &array_init_block, &status));
    } /*block*/

    if(status_ok(status))
    {
        PROCESS_STATUS process_status;

        process_status_init(&process_status);
        process_status.flags.foreground = TRUE;
        process_status.reason.type = UI_TEXT_TYPE_RESID;
        process_status.reason.text.resource_id = MSG_STATUS_SORTING;
        process_status_begin(p_docu, &process_status, PROCESS_STATUS_PERCENT);

        { /* load array list with rows to be sorted */
        ROW row;
        P_ROW p_row;

        for(row = row_s, p_row = array_base(&h_sort_rows, ROW); row < row_e; row += 1, p_row += 1)
            *p_row = row;
        } /*block*/

        qsort(array_base(&h_sort_rows, ROW), (U32) n_rows, sizeof(ROW), proc_qsort_compare_rows);

        {
        ROW row;
        P_ROW p_row;
        REGION region;

        region_from_docu_area_max(&region, p_docu_area);
        region.whole_col = 0;

#if RISCOS && 1
alloc_validate((P_ANY)1, "sort");
#endif
        for(row = row_s, p_row = array_base(&h_sort_rows, ROW); row < row_e; row += 1, p_row += 1)
        {
            if(row != *p_row)
            {
                region.tl.row = row;
                region.br.row = *p_row;
                status_break(status = cells_swap_rows(p_docu, &region));
            }

            process_status.data.percent.current = 100 * (row - row_s) / (row_e - row_s);
            process_status_reflect(&process_status);

            { /* update list of unsorted rows */
            ROW row_rest;
            P_ROW p_row_rest;
            for(row_rest = row + 1, p_row_rest = p_row + 1; row_rest < row_e; row_rest += 1, p_row_rest += 1)
                if(*p_row_rest == row)
                {
                    *p_row_rest = *p_row;
                    break;
                }
            } /*block*/
        }
        } /*block*/

        process_status_end(&process_status);
    }

    al_array_dispose(&h_sort_rows);

    return(status);
}

T5_CMD_PROTO(static, t5_cmd_sort)
{
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_docu->mark_info_cells.h_markers)
    {
        DOCU_AREA docu_area;
        docu_area_from_markers_first(p_docu, &docu_area);

        sort_cmd_statics.arglist_handle = p_t5_cmd->arglist_handle;
        sort_cmd_statics.p_docu = p_docu;

        if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
        {
            ROW row_s;

            status = sort_docu_area(p_docu, &docu_area, &row_s);

            reformat_from_row(p_docu, row_s, REFORMAT_Y);
        }
    }
    else
        status = STATUS_FAIL;

    return(status);
}

T5_CMD_PROTO(static, t5_cmd_sort_intro)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(object_present(OBJECT_ID_REC))
    {
        STATUS status = object_call_id(OBJECT_ID_REC, p_docu, T5_CMD_SORT_RECORDZ, de_const_cast(P_T5_CMD, p_t5_cmd));

        if(STATUS_OK != status) /* done, or error */
            return(status);
    }

    if(!p_docu->mark_info_cells.h_markers)
        return(STATUS_FAIL);

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, sort_intro_ctl_create, elemof32(sort_intro_ctl_create), MSG_DIALOG_SORT_CAPTION);
    dialog_cmd_process_dbox.help_topic_resource_id = MSG_DIALOG_SORT_HELP_TOPIC;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_sort_intro;
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
    } /*block*/
}

T5_CMD_PROTO(static, t5_cmd_word_count)
{
    S32 total_counted = 0;
    SCAN_BLOCK scan_block;
    OBJECT_WORD_COUNT object_word_count;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_DOWN, p_docu->mark_info_cells.h_markers ? SCAN_MARKERS : SCAN_WHOLE, NULL, OBJECT_ID_NONE)))
    {
        while(status_done(cells_scan_next(p_docu, &object_word_count.object_data, &scan_block)))
        {
            object_word_count.words_counted = 0;
            status_consume(cell_call_id(object_word_count.object_data.object_id, p_docu, T5_CMD_WORD_COUNT, &object_word_count, NO_CELLS_EDIT));
            total_counted += object_word_count.words_counted;
        }
    }

    status_line_setf(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, (total_counted == 1) ? MSG_STATUS_WORD_COUNTED : MSG_STATUS_WORDS_COUNTED, total_counted);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, t5_cmd_style_for_config)
{
    STATUS status;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    {
    const P_DOCU p_docu_config = p_docu_from_config_wr();

    status = t5_cmd_style_intro(p_docu_config, t5_message, p_t5_cmd);

    issue_choice_changed(T5_CMD_STYLE_FOR_CONFIG);
    } /*block*/

    return(status);
}

T5_MSG_PROTO(static, skel_split_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(resource_init(OBJECT_ID_SKEL_SPLIT, P_BOUND_MESSAGES_OBJECT_ID_SKEL_SPLIT, P_BOUND_RESOURCES_OBJECT_ID_SKEL_SPLIT));

        return(ui_style_msg_startup());

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_SKEL_SPLIT));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_skel_split);
OBJECT_PROTO(extern, object_skel_split)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(skel_split_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_TABLE_STYLE_ADD:
        return(t5_msg_table_style_add(p_docu, t5_message, (P_DOCU_AREA) p_data));

    case T5_MSG_BOX_APPLY:
        return(t5_msg_box_apply(p_docu, t5_message, (P_BOX_APPLY) p_data));

    case T5_MSG_UISTYLE_COLOUR_PICKER:
        return(t5_msg_uistyle_colour_picker(p_docu, t5_message, (P_MSG_UISTYLE_COLOUR_PICKER) p_data));

    case T5_MSG_UISTYLE_STYLE_EDIT:
        return(t5_msg_uistyle_style_edit(p_docu, t5_message, (P_MSG_UISTYLE_STYLE_EDIT) p_data));

    case T5_CMD_COLS_ADD_AFTER_INTRO:
        return(t5_cmd_cols_add_after_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_COLS_INSERT_BEFORE_INTRO:
        return(t5_cmd_cols_insert_before_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_ROWS_ADD_AFTER_INTRO:
        return(t5_cmd_rows_add_after_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_ROWS_INSERT_BEFORE_INTRO:
        return(t5_cmd_rows_insert_before_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_INSERT_TABLE:
        return(t5_cmd_insert_table(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_INSERT_TABLE_INTRO:
        return(t5_cmd_insert_table_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SORT:
        return(t5_cmd_sort(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SORT_INTRO:
        return(t5_cmd_sort_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_WORD_COUNT:
        return(t5_cmd_word_count(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_STYLE_BUTTON:
        return(t5_cmd_style_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_STYLE_FOR_CONFIG:
        return(t5_cmd_style_for_config(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_EFFECTS_BUTTON:
        return(t5_cmd_effects_button(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_STYLE_REGION_EDIT:
        return(t5_cmd_style_region_edit(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_BOX_INTRO:
        return(t5_cmd_box_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of ob_skspt.c */
