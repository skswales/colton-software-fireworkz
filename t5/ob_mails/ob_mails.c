/* ob_mails.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Mailshot object module for Fireworkz */

/* SKS March 1993 */

#include "common/gflags.h"

#include "ob_mails/ob_mails.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

/*
callback routines
*/

MAEVE_EVENT_PROTO(static, maeve_event_ob_mails);

#if RISCOS
#define MSG_WEAK &rb_mails_msg_weak
extern PC_U8 rb_mails_msg_weak;
#endif
#define P_BOUND_RESOURCES_OBJECT_ID_MAILSHOT NULL

/*
internal structure
*/

typedef struct MAILSHOT_INSTANCE_DATA
{
    DOCNO source_docno;
    U8 _spare[3];

    MAEVE_HANDLE maeve_handle;
    ARRAY_HANDLE field_data_handle;

    ROW row;
    BOOL blank_blank;
    S32 blank_stt, blank_end;
}
MAILSHOT_INSTANCE_DATA, * P_MAILSHOT_INSTANCE_DATA;

_Check_return_
_Ret_valid_
static inline P_MAILSHOT_INSTANCE_DATA
p_object_instance_data_MAILSHOT_valid(
    _InRef_     P_DOCU p_docu)
{
    P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data = (P_MAILSHOT_INSTANCE_DATA)
        _p_object_instance_data(p_docu, OBJECT_ID_MAILSHOT CODE_ANALYSIS_ONLY_ARG(sizeof32(MAILSHOT_INSTANCE_DATA)));
    PTR_ASSERT(p_mailshot_instance_data);
    return(p_mailshot_instance_data);
}

_Check_return_
_Ret_maybenone_
static inline P_MAILSHOT_INSTANCE_DATA
p_object_instance_data_MAILSHOT_maybenone(
    _InRef_     P_DOCU p_docu)
{
    P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data = (P_MAILSHOT_INSTANCE_DATA)
        _p_object_instance_data(p_docu, OBJECT_ID_MAILSHOT CODE_ANALYSIS_ONLY_ARG(sizeof32(MAILSHOT_INSTANCE_DATA)));
    return(p_mailshot_instance_data);
}

/*
internal routines
*/

static void
mailshot_fields_dispose(
    P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data);


/*
construct table
*/

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "MailshotField",          NULL,                       T5_CMD_INSERT_FIELD_INTRO_FIELD,            { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "MailshotSelect",         NULL,                       T5_CMD_MAILSHOT_SELECT,                     { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "MailshotPrint",          NULL,                       T5_CMD_MAILSHOT_PRINT,                      { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },

    { NULL,                     NULL,                       T5_EVENT_NONE } /* end of table */
};

static void
mailshot_change(
    _DocuRef_   P_DOCU p_docu)
{
    /* reformat ***all*** of this document because of changes to the source of mailshot fields */
    reformat_all(p_docu);
}

/******************************************************************************
*
* insert field dialog box
*
******************************************************************************/

enum MAILSHOT_FIELD_INSERT_IDS
{
    MAILSHOT_FIELD_INSERT_ID_NUMBER_TEXT = 32,
    MAILSHOT_FIELD_INSERT_ID_NUMBER
};

static const DIALOG_CONTROL
mailshot_field_insert_number_text =
{
    MAILSHOT_FIELD_INSERT_ID_NUMBER_TEXT, DIALOG_MAIN_GROUP,

    { DIALOG_CONTROL_PARENT, MAILSHOT_FIELD_INSERT_ID_NUMBER,
      DIALOG_CONTROL_SELF,   MAILSHOT_FIELD_INSERT_ID_NUMBER },

    { 0, 0, DIALOG_SYSCHARSL_H(12), 0 },

    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
mailshot_field_insert_number_text_data = { UI_TEXT_INIT_RESID(MAILSHOT_MSG_DIALOG_INSERT_FIELD_NUMBER) };

static const DIALOG_CONTROL
mailshot_field_insert_number =
{
    MAILSHOT_FIELD_INSERT_ID_NUMBER, DIALOG_MAIN_GROUP,

    { MAILSHOT_FIELD_INSERT_ID_NUMBER_TEXT, DIALOG_CONTROL_PARENT },

    { DIALOG_STDSPACING_H, 0, DIALOG_BUMP_H(5), DIALOG_STDBUMP_V },

    { DRT(RTLT, BUMP_S32), 1 }
};

static const UI_CONTROL_S32
mailshot_field_insert_number_control = { 1, 1000 };

static const DIALOG_CONTROL_DATA_BUMP_S32
mailshot_field_insert_number_data = { { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/, &mailshot_field_insert_number_control } /* BUMP_XX */ };

static S32
mailshot_field_insert_number_data_state = 1;

static const ARG_TYPE
mailshot_field_insert_ok_data_args[] = { ARG_TYPE_S32, ARG_TYPE_NONE };

static const DIALOG_CTL_ID
mailshot_field_insert_ok_data_argmap[] = { MAILSHOT_FIELD_INSERT_ID_NUMBER };

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
mailshot_field_insert_ok_command = { T5_CMD_FIELD_INS_MS_FIELD, OBJECT_ID_SKEL, mailshot_field_insert_ok_data_args, mailshot_field_insert_ok_data_argmap };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
mailshot_field_insert_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_OK), &mailshot_field_insert_ok_command };

static const DIALOG_CTL_CREATE
mailshot_field_insert_ctl_create[] =
{
    { &dialog_main_group },

    { &mailshot_field_insert_number_text, &mailshot_field_insert_number_text_data },
    { &mailshot_field_insert_number,      &mailshot_field_insert_number_data },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &defbutton_ok, &mailshot_field_insert_ok_data }
};

/******************************************************************************
*
* insert a numbered field into a document
*
******************************************************************************/

_Check_return_
static STATUS
dialog_mailshot_insert_field_intro_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    return(ui_dlg_set_s32(p_dialog_msg_process_start->h_dialog, MAILSHOT_FIELD_INSERT_ID_NUMBER, mailshot_field_insert_number_data_state));
}

_Check_return_
static STATUS
dialog_mailshot_insert_field_intro_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        mailshot_field_insert_number_data_state = ui_dlg_get_s32(p_dialog_msg_process_end->h_dialog, MAILSHOT_FIELD_INSERT_ID_NUMBER);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_mailshot_insert_field_intro)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_mailshot_insert_field_intro_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_mailshot_insert_field_intro_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
mailshot_cmd_insert_field_intro_field(
    _DocuRef_   P_DOCU p_docu)
{
    /* put up a dialog box and get the punter to choose something */
    STATUS status;

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, mailshot_field_insert_ctl_create, elemof32(mailshot_field_insert_ctl_create), MAILSHOT_MSG_DIALOG_INSERT_FIELD_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = MAILSHOT_MSG_DIALOG_INSERT_FIELD_CAPTION;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_mailshot_insert_field_intro;
    if((status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox)) == STATUS_FAIL)
        status = STATUS_OK;
    } /*block*/

    return(status);
}

_Check_return_
static STATUS
mailshot_fields_create(
    P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data)
{
    STATUS status = STATUS_OK;

    mailshot_fields_dispose(p_mailshot_instance_data);

    if(DOCNO_NONE != p_mailshot_instance_data->source_docno)
    {
        P_DOCU p_docu_src = p_docu_from_docno(p_mailshot_instance_data->source_docno);
        SC_ARRAY_INIT_BLOCK array_init_block_fields = aib_init(1, sizeof32(ARRAY_HANDLE), TRUE);
        POSITION position;

        position.slr.row = p_mailshot_instance_data->row; /* This is where it derives the source row */

        for(position.slr.col = 0; position.slr.col < n_cols_logical(p_docu_src); ++position.slr.col)
        {
            P_ARRAY_HANDLE p_array_handle;
            OBJECT_READ_TEXT object_read_text;
            QUICK_UBLOCK quick_ublock;

            if(NULL == (p_array_handle = al_array_extend_by_ARRAY_HANDLE(&p_mailshot_instance_data->field_data_handle, 1, &array_init_block_fields, &status)))
                break;

            status_consume(object_data_from_slr(p_docu_src, &object_read_text.object_data, &position.slr));
            object_read_text.object_data.object_position_start.object_id = object_read_text.object_data.object_id;
            object_read_text.object_data.object_position_start.data = 0;

            status_break(status = al_array_alloc_zero(p_array_handle, &array_init_block_u8));

            quick_ublock_setup_using_array(&quick_ublock, *p_array_handle);

            object_read_text.p_quick_ublock = &quick_ublock;
            object_read_text.type = OBJECT_READ_TEXT_RESULT;
            status_break(status = object_call_id(object_read_text.object_data.object_id, p_docu_src, T5_MSG_OBJECT_READ_TEXT, &object_read_text));
        }

        if(p_mailshot_instance_data->blank_blank)
        {
            ARRAY_INDEX field, stt_field;
            P_ARRAY_HANDLE p_last_ok_field = NULL;

            assert(p_mailshot_instance_data->blank_stt >= 0);
            stt_field = MAX(0, p_mailshot_instance_data->blank_stt);
            field = MIN(p_mailshot_instance_data->blank_end /*incl*/ + 1, array_elements(&p_mailshot_instance_data->field_data_handle));
            while(--field >= stt_field)
            {
                P_ARRAY_HANDLE p_array_handle = array_ptr(&p_mailshot_instance_data->field_data_handle, ARRAY_HANDLE, field);

                /* bubble non-blank fields along towards start of range */
                if(array_elements(p_array_handle) <= 1)
                {
                    al_array_dispose(p_array_handle); /* may have a handle but no data, so kill it dead */

                    if(NULL != p_last_ok_field)
                    {
                        /* move this block of non-blank fields down */
                        memmove32(p_array_handle, p_array_handle + 1, PtrDiffBytesU32(p_last_ok_field, (p_array_handle + 1)));
                        /* so its end comes down too, bubbling the blank field towards the end of the range */
                        *--p_last_ok_field = 0;
                    }
                }
                else if(NULL == p_last_ok_field)
                    p_last_ok_field = p_array_handle + 1;
            }
        }

    }

    if(status_fail(status))
        mailshot_fields_dispose(p_mailshot_instance_data);
    else
    {
        /* SKS after 1.03 18May93 - omit blank records from print */
        ARRAY_INDEX field;
        STATUS all_blank = 1;

        for(field = 0; field < array_elements(&p_mailshot_instance_data->field_data_handle); ++field)
        {
            P_ARRAY_HANDLE p_array_handle = array_ptr(&p_mailshot_instance_data->field_data_handle, ARRAY_HANDLE, field);

            if(array_elements(p_array_handle) > 1)
            {
                all_blank = 0;
                break;
            }
        }

        status = all_blank ? STATUS_OK : STATUS_DONE; /* SKS 29mar95 made it set status to fix mailshot printing in case where object_call_id now returned different status */
    }

    return(status);
}

static void
mailshot_fields_dispose(
    P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data)
{
    ARRAY_INDEX field;

    for(field = 0; field < array_elements(&p_mailshot_instance_data->field_data_handle); ++field)
        al_array_dispose(array_ptr(&p_mailshot_instance_data->field_data_handle, ARRAY_HANDLE, field));

    al_array_dispose(&p_mailshot_instance_data->field_data_handle);
}

_Check_return_
static STATUS
mailshot_instance_data_dispose(
    _DocuRef_   P_DOCU p_docu)
{
    P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data = p_object_instance_data_MAILSHOT_maybenone(p_docu);

    /*reportf(TEXT("mailshot_instance_data_dispose: ") PTR_XTFMT, p_mailshot_instance_data);*/

    if(P_DATA_NONE != p_mailshot_instance_data)
    {
        if(0 != p_mailshot_instance_data->maeve_handle)
        {
            P_DOCU p_docu_source = p_docu_from_docno(p_mailshot_instance_data->source_docno);

            if(!IS_DOCU_NONE(p_docu_source))
            {
                maeve_event_handler_del_handle(p_docu_source, p_mailshot_instance_data->maeve_handle);
                p_mailshot_instance_data->maeve_handle = 0;
            }
        }

        mailshot_fields_dispose(p_mailshot_instance_data);

        object_instance_data_dispose(p_docu, OBJECT_ID_MAILSHOT); /* called other than in CLOSE2 */
    }

    return(STATUS_OK);
}

enum MAILSHOT_SELECT_IDS
{
    MAILSHOT_SELECT_ID_OK = IDOK,

    MAILSHOT_SELECT_ID_LIST = 666,
    MAILSHOT_SELECT_ID_ROW_TEXT,
    MAILSHOT_SELECT_ID_ROW,
    MAILSHOT_SELECT_ID_BLANK_GROUP,
    MAILSHOT_SELECT_ID_BLANK_BLANK,
    MAILSHOT_SELECT_ID_BLANK_STT_TEXT,
    MAILSHOT_SELECT_ID_BLANK_STT,
    MAILSHOT_SELECT_ID_BLANK_END_TEXT,
    MAILSHOT_SELECT_ID_BLANK_END
};

#define MAILSHOT_SELECT_V (DIALOG_STDLISTOVH_V + 4 * DIALOG_STDLISTITEM_V)

static const DIALOG_CONTROL
mailshot_select_list =
{
    MAILSHOT_SELECT_ID_LIST, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, MAILSHOT_SELECT_ID_BLANK_GROUP, DIALOG_CONTROL_SELF },
    { 0, 0, 0, MAILSHOT_SELECT_V },
    { DRT(LTRT, LIST_TEXT), 1 }
};

static const DIALOG_CONTROL
mailshot_select_row_text =
{
    MAILSHOT_SELECT_ID_ROW_TEXT, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, MAILSHOT_SELECT_ID_ROW, DIALOG_CONTROL_SELF, MAILSHOT_SELECT_ID_ROW },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
mailshot_select_row_text_data = { UI_TEXT_INIT_RESID(MAILSHOT_MSG_DIALOG_SELECT_ROW), { 0 /*left_text*/ } };

static const DIALOG_CONTROL
mailshot_select_row =
{
    MAILSHOT_SELECT_ID_ROW, DIALOG_MAIN_GROUP,
    { MAILSHOT_SELECT_ID_ROW_TEXT, MAILSHOT_SELECT_ID_LIST },
    { DIALOG_STDSPACING_H, DIALOG_STDSPACING_V, DIALOG_BUMP_H(5), DIALOG_STDBUMP_V },
    { DRT(RBLT, BUMP_S32), 1 }
};

static const UI_CONTROL_S32
mailshot_select_row_control = { 1, S32_MAX };

static const DIALOG_CONTROL_DATA_BUMP_S32
mailshot_select_row_data = { { { { FRAMED_BOX_EDIT } }, &mailshot_select_row_control } };

static const DIALOG_CONTROL
mailshot_select_blank_group =
{
    MAILSHOT_SELECT_ID_BLANK_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, MAILSHOT_SELECT_ID_ROW, MAILSHOT_SELECT_ID_BLANK_END, MAILSHOT_SELECT_ID_BLANK_END },
    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
mailshot_select_blank_group_data = { UI_TEXT_INIT_RESID(MAILSHOT_MSG_DIALOG_SELECT_BLANK_GROUP), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
mailshot_select_blank_blank =
{
    MAILSHOT_SELECT_ID_BLANK_BLANK, MAILSHOT_SELECT_ID_BLANK_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX) }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
mailshot_select_blank_blank_data = { { 0 }, UI_TEXT_INIT_RESID(MAILSHOT_MSG_DIALOG_SELECT_BLANK_BLANK) };

/*
blank start
*/

static const DIALOG_CONTROL
mailshot_select_blank_stt_text =
{
    MAILSHOT_SELECT_ID_BLANK_STT_TEXT, MAILSHOT_SELECT_ID_BLANK_GROUP,
    { MAILSHOT_SELECT_ID_BLANK_BLANK, MAILSHOT_SELECT_ID_BLANK_STT, DIALOG_CONTROL_SELF, MAILSHOT_SELECT_ID_BLANK_STT },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
mailshot_select_blank_stt_text_data = { UI_TEXT_INIT_RESID(MAILSHOT_MSG_DIALOG_SELECT_BLANK_STT) };

static const DIALOG_CONTROL
mailshot_select_blank_stt =
{
    MAILSHOT_SELECT_ID_BLANK_STT, MAILSHOT_SELECT_ID_BLANK_GROUP,
    { MAILSHOT_SELECT_ID_BLANK_STT_TEXT, MAILSHOT_SELECT_ID_BLANK_BLANK },
    { DIALOG_STDSPACING_H, DIALOG_STDSPACING_V, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },
    { DRT(RBLT, BUMP_S32), 1 }
};

static const UI_CONTROL_S32
mailshot_select_blank_sttend_control = { 1, S32_MAX };

static const DIALOG_CONTROL_DATA_BUMP_S32
mailshot_select_blank_stt_data = { { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/, &mailshot_select_blank_sttend_control } /* BUMP_XX */ };

/*
blank end
*/

static const DIALOG_CONTROL
mailshot_select_blank_end_text =
{
    MAILSHOT_SELECT_ID_BLANK_END_TEXT, MAILSHOT_SELECT_ID_BLANK_GROUP,
    { MAILSHOT_SELECT_ID_BLANK_STT_TEXT, MAILSHOT_SELECT_ID_BLANK_END, MAILSHOT_SELECT_ID_BLANK_STT_TEXT, MAILSHOT_SELECT_ID_BLANK_END },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
mailshot_select_blank_end_text_data = { UI_TEXT_INIT_RESID(MAILSHOT_MSG_DIALOG_SELECT_BLANK_END) };

static const DIALOG_CONTROL
mailshot_select_blank_end =
{
    MAILSHOT_SELECT_ID_BLANK_END, MAILSHOT_SELECT_ID_BLANK_GROUP,
    { MAILSHOT_SELECT_ID_BLANK_STT, MAILSHOT_SELECT_ID_BLANK_STT, MAILSHOT_SELECT_ID_BLANK_STT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_S32), 1 }
};

static const DIALOG_CONTROL_DATA_BUMP_S32
mailshot_select_blank_end_data = { { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/, &mailshot_select_blank_sttend_control } /* BUMP_XX */ };

static const DIALOG_CTL_CREATE
mailshot_select_ctl_create[] =
{
    { &dialog_main_group },

    { &mailshot_select_list, &stdlisttext_data_dd },
    { &mailshot_select_row_text,       &mailshot_select_row_text_data },
    { &mailshot_select_row,            &mailshot_select_row_data },

    { &mailshot_select_blank_group,    &mailshot_select_blank_group_data },
    { &mailshot_select_blank_blank,    &mailshot_select_blank_blank_data },
    { &mailshot_select_blank_stt_text, &mailshot_select_blank_stt_text_data },
    { &mailshot_select_blank_stt,      &mailshot_select_blank_stt_data },
    { &mailshot_select_blank_end_text, &mailshot_select_blank_end_text_data },
    { &mailshot_select_blank_end,      &mailshot_select_blank_end_data },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &defbutton_ok, &defbutton_ok_data }
};

typedef struct MAILSHOT_SELECT_LIST_ENTRY
{
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, elemof32("doseight.fwk")); /* NB buffer adjacent for fixup */

    DOCNO docno;
}
MAILSHOT_SELECT_LIST_ENTRY, * P_MAILSHOT_SELECT_LIST_ENTRY;

typedef struct MAILSHOT_SELECT_CALLBACK
{
    ROW row;
    BOOL blank_blank;
    S32 blank_stt, blank_end;
    S32 selected_item;
}
MAILSHOT_SELECT_CALLBACK, * P_MAILSHOT_SELECT_CALLBACK;

static UI_SOURCE
mailshot_select_list_source;

_Check_return_
static STATUS
dialog_mailshot_select_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case MAILSHOT_SELECT_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &mailshot_select_list_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_mailshot_select_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    const P_MAILSHOT_SELECT_CALLBACK p_mailshot_select_callback = (P_MAILSHOT_SELECT_CALLBACK) p_dialog_msg_ctl_create_state->client_handle;

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case MAILSHOT_SELECT_ID_LIST:
        p_dialog_msg_ctl_create_state->state_set.bits = DIALOG_STATE_SET_ALTERNATE;
        p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = p_mailshot_select_callback->selected_item;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_mailshot_select_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_MAILSHOT_SELECT_CALLBACK p_mailshot_select_callback = (P_MAILSHOT_SELECT_CALLBACK) p_dialog_msg_process_start->client_handle;
    const H_DIALOG h_dialog = p_dialog_msg_process_start->h_dialog;

    status_return(ui_dlg_set_s32(h_dialog, MAILSHOT_SELECT_ID_ROW, p_mailshot_select_callback->row));
    status_return(ui_dlg_set_check(h_dialog, MAILSHOT_SELECT_ID_BLANK_BLANK, p_mailshot_select_callback->blank_blank));
    status_return(ui_dlg_set_s32(h_dialog, MAILSHOT_SELECT_ID_BLANK_STT, p_mailshot_select_callback->blank_stt));
    status_return(ui_dlg_set_s32(h_dialog, MAILSHOT_SELECT_ID_BLANK_END, p_mailshot_select_callback->blank_end));

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_mailshot_select_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_MAILSHOT_SELECT_CALLBACK p_mailshot_select_callback = (P_MAILSHOT_SELECT_CALLBACK) p_dialog_msg_process_end->client_handle;
        const H_DIALOG h_dialog = p_dialog_msg_process_end->h_dialog;

        p_mailshot_select_callback->row     = (ROW) ui_dlg_get_s32(h_dialog, MAILSHOT_SELECT_ID_ROW);
        p_mailshot_select_callback->blank_blank   = ui_dlg_get_check(h_dialog, MAILSHOT_SELECT_ID_BLANK_BLANK);
        p_mailshot_select_callback->blank_stt     = ui_dlg_get_s32(h_dialog, MAILSHOT_SELECT_ID_BLANK_STT);
        p_mailshot_select_callback->blank_end     = ui_dlg_get_s32(h_dialog, MAILSHOT_SELECT_ID_BLANK_END);
        p_mailshot_select_callback->selected_item = ui_dlg_get_list_idx(h_dialog, MAILSHOT_SELECT_ID_LIST);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_mailshot_select)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_mailshot_select_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_mailshot_select_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_mailshot_select_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_mailshot_select_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
mailshot_cmd_mailshot_select(
    _DocuRef_   P_DOCU p_docu_dependent)
{
    DOCNO docno_dependent = docno_from_p_docu(p_docu_dependent);
    ARRAY_HANDLE mailshot_select_list_handle = 0;
    P_MAILSHOT_SELECT_LIST_ENTRY p_mailshot_select_list_entry;
    S32 selected_item = 0 /*always select one DIALOG_CTL_STATE_LIST_ITEM_NONE*/;
    PCTSTR p_selected_name = NULL;
    DOCNO selected_docno = DOCNO_NONE;
    STATUS status = STATUS_OK;
    S32 i;
    P_DOCU p_docu_src;
    MAILSHOT_SELECT_CALLBACK mailshot_select_callback;
    DOCNO cur_docno = DOCNO_NONE;

    zero_struct(mailshot_select_callback);

    {
    P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data = p_object_instance_data_MAILSHOT_maybenone(p_docu_dependent);

    if(P_DATA_NONE != p_mailshot_instance_data)
    {
        mailshot_select_callback.row = p_mailshot_instance_data->row + 1;
        mailshot_select_callback.blank_blank = p_mailshot_instance_data->blank_blank;
        mailshot_select_callback.blank_stt = p_mailshot_instance_data->blank_stt + 1;
        mailshot_select_callback.blank_end = p_mailshot_instance_data->blank_end + 1;

        cur_docno = p_mailshot_instance_data->source_docno;
    }
    else
    {
        mailshot_select_callback.row = 1; /* SKS 15apr96 do some init */
        mailshot_select_callback.blank_stt = 1; /* SKS 22may96 do some more init */
        mailshot_select_callback.blank_end = 1;
    }
    } /*block*/

    mailshot_select_list_source.type = UI_SOURCE_TYPE_NONE;

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*p_mailshot_select_list_entry), TRUE);
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
    {
        if(docno == docno_dependent)
            continue;

        p_docu_src = p_docu_from_docno(docno);

        if(n_cols_logical(p_docu_src) < 2)
            continue;

        if(NULL != (p_mailshot_select_list_entry = al_array_extend_by(&mailshot_select_list_handle, MAILSHOT_SELECT_LIST_ENTRY, 1, &array_init_block, &status)))
        {
            p_mailshot_select_list_entry->docno = docno;

            quick_tblock_with_buffer_setup(p_mailshot_select_list_entry->quick_tblock);

            {
            PCTSTR leafname = p_docu_src->docu_name.leaf_name;
            status = quick_tblock_tstr_add_n(&p_mailshot_select_list_entry->quick_tblock, leafname, strlen_with_NULLCH);
            } /*block*/

            if(p_mailshot_select_list_entry->docno == cur_docno)
                selected_item = array_indexof_element(&mailshot_select_list_handle, MAILSHOT_SELECT_LIST_ENTRY, p_mailshot_select_list_entry);
        }

        status_break(status);
    }
    } /*block*/

    if(status_ok(status))
    {
        if(!array_elements(&mailshot_select_list_handle))
            return(create_error(ERR_MAILSHOT_NONE_TO_SELECT));

        /* make a source of text pointers to these elements for list box processing */
        status = ui_source_create_tb(&mailshot_select_list_handle, &mailshot_select_list_source, UI_TEXT_TYPE_TSTR_PERM, offsetof32(MAILSHOT_SELECT_LIST_ENTRY, quick_tblock));
    }

    if(status_ok(status))
    {
        /* get the punter to pick one */

        /* suss this pointer now it won't move! */
        if(selected_item >= 0)
            p_selected_name = ui_text_tstr_no_default(array_ptr(&mailshot_select_list_source.source.array_handle, UI_TEXT, selected_item));

        /*array_qsort(&mailshot_select_list_source.source.array_handle, ui_text_sort_alpha);*/

        /* find the selected item again */
        if(NULL != p_selected_name)
            for(i = 0; i < array_elements(&mailshot_select_list_source.source.array_handle); ++i)
                if(p_selected_name == ui_text_tstr_no_default(array_ptr(&mailshot_select_list_source.source.array_handle, UI_TEXT, i)))
                    selected_item = i;

        mailshot_select_callback.selected_item = selected_item;

        {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, mailshot_select_ctl_create, elemof32(mailshot_select_ctl_create), MAILSHOT_MSG_DIALOG_SELECT_CAPTION);
        /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
        dialog_cmd_process_dbox.caption.text.resource_id = MAILSHOT_MSG_DIALOG_SELECT_CAPTION;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_mailshot_select;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &mailshot_select_callback;
        status = call_dialog_with_docu(p_docu_dependent, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        } /*block*/
    }

    if(status_ok(status))
    {
        /* selected item NB. it's the UI list that's sorted! */
        p_selected_name = ui_text_tstr_no_default(array_ptr(&mailshot_select_list_source.source.array_handle, UI_TEXT, mailshot_select_callback.selected_item));

        for(i = 0; i < array_elements(&mailshot_select_list_handle); ++i)
        {
            p_mailshot_select_list_entry = array_ptr(&mailshot_select_list_handle, MAILSHOT_SELECT_LIST_ENTRY, i);

            if(p_selected_name == quick_tblock_tstr(&p_mailshot_select_list_entry->quick_tblock))
                selected_docno = p_mailshot_select_list_entry->docno;
        }

        assert(selected_docno != DOCNO_NONE);
    }

    ui_lists_dispose_tb(&mailshot_select_list_handle, &mailshot_select_list_source, offsetof32(MAILSHOT_SELECT_LIST_ENTRY, quick_tblock));

    status_return(status);

    /* get rid of any previous association */
    if(P_DATA_NONE != p_object_instance_data_MAILSHOT_maybenone(p_docu_dependent))
        status_consume(mailshot_instance_data_dispose(p_docu_dependent));

    status_return(object_instance_data_alloc(p_docu_dependent, OBJECT_ID_MAILSHOT, sizeof32(MAILSHOT_INSTANCE_DATA)));

    {
    P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data = p_object_instance_data_MAILSHOT_valid(p_docu_dependent);
    /*reportf(TEXT("mailshot_cmd_mailshot_select: p_mailshot_instance_data ") PTR_XTFMT TEXT(" with source_docno=%d"), p_mailshot_instance_data, selected_docno);*/

    p_mailshot_instance_data->source_docno = selected_docno;
    p_mailshot_instance_data->maeve_handle = maeve_event_handler_add(p_docu_from_docno(p_mailshot_instance_data->source_docno), maeve_event_ob_mails, (U32) docno_dependent);

    if(status_fail(p_mailshot_instance_data->maeve_handle))
    {
        STATUS status = (STATUS) p_mailshot_instance_data->maeve_handle;
        p_mailshot_instance_data->source_docno = DOCNO_NONE;
        p_mailshot_instance_data->maeve_handle = 0;
        status_consume(mailshot_instance_data_dispose(p_docu_dependent));
        return(status);
    }

    p_docu_src = p_docu_from_docno(p_mailshot_instance_data->source_docno);

    if( mailshot_select_callback.row > n_rows(p_docu_src))
        mailshot_select_callback.row = n_rows(p_docu_src);
    if(mailshot_select_callback.row < 1)
        mailshot_select_callback.row = 1;

    p_mailshot_instance_data->row = mailshot_select_callback.row - 1;
    p_mailshot_instance_data->blank_blank = mailshot_select_callback.blank_blank;
    p_mailshot_instance_data->blank_stt = mailshot_select_callback.blank_stt - 1;
    p_mailshot_instance_data->blank_end = mailshot_select_callback.blank_end - 1;

    (void) mailshot_fields_create(p_mailshot_instance_data);
    } /*block*/

    mailshot_change(p_docu_dependent);

    return(STATUS_OK);
}

_Check_return_
static STATUS
mailshot_cmd_mailshot_print(
    _InVal_     DOCNO docno)
{
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    const T5_MESSAGE t5_message = T5_CMD_PRINT;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    DOCNO source_docno;
    ROW old_row;
    STATUS status;

    {
    P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data = p_object_instance_data_MAILSHOT_maybenone(p_docu_from_docno(docno));

    if((P_DATA_NONE == p_mailshot_instance_data) || (DOCNO_NONE == p_mailshot_instance_data->source_docno))
        return(create_error(ERR_MAILSHOT_NONE_SELECTED));

    source_docno = p_mailshot_instance_data->source_docno;

    old_row = p_mailshot_instance_data->row;

    p_mailshot_instance_data->row = -1; /* preincremented in below loop */
    } /*block*/

    /* loop printing each row of source */
    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 2);

        p_args[0].val.s32   = 1;     /* copies */
        p_args[1].val.fBool = FALSE; /* all */

        for(;;)
        {
            P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data = p_object_instance_data_MAILSHOT_valid(p_docu_from_docno(docno));

            ++p_mailshot_instance_data->row; /* Controls the source document row */

            if(p_mailshot_instance_data->row >= n_rows(p_docu_from_docno(source_docno)))
                break;

            status_break(status = mailshot_fields_create(p_mailshot_instance_data));

            /* SKS after 1.03 18May93 - loop omitting completely blank records */
            if(status_done(status))
            {
                mailshot_change(p_docu_from_docno(docno)); /* ensure reformatted before every print */

                status_break(status = execute_command(object_id, p_docu_from_docno(docno), t5_message, &arglist_handle));
            }
        }

        arglist_dispose(&arglist_handle);
    }

    {
    P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data = p_object_instance_data_MAILSHOT_valid(p_docu_from_docno(docno));
    STATUS status1;

    p_mailshot_instance_data->row = old_row;

    status1 = mailshot_fields_create(p_mailshot_instance_data);

    if(status_ok(status))
        status = status1;

    mailshot_change(p_docu_from_docno(docno));
    } /*block*/

    return(status);
}

T5_MSG_PROTO(static, mailshot_msg_read_mail_text, _InoutRef_ P_READ_MAIL_TEXT p_read_mail_text)
{
    P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data = p_object_instance_data_MAILSHOT_maybenone(p_docu);
    STATUS status = STATUS_OK;

    IGNOREPARM_InVal_(t5_message);

    if((P_DATA_NONE != p_mailshot_instance_data) && array_elements(&p_mailshot_instance_data->field_data_handle))
    {
        P_ARRAY_HANDLE_UCHARS p_array_handle_uchars = P_ARRAY_HANDLE_NONE;
        U32 len = 0;

        if(array_elements(&p_mailshot_instance_data->field_data_handle) > p_read_mail_text->field_no)
            p_array_handle_uchars = array_ptr(&p_mailshot_instance_data->field_data_handle, ARRAY_HANDLE, p_read_mail_text->field_no);

        if(!IS_P_DATA_NONE(p_array_handle_uchars))
            len = array_elements32(p_array_handle_uchars);

        if(0 != len)
            status = quick_ublock_uchars_add(p_read_mail_text->p_quick_ublock, uchars_from_h_uchars(p_array_handle_uchars), len);

        if(status_ok(status))
            p_read_mail_text->responded = 1;
    }

    return(status);
}

/*
main events
*/

/* detect when document we are using as mailshot source goes away */

_Check_return_
static STATUS
mailshot_msg_close1(
    _InVal_     DOCNO source_docno,
    _InVal_     DOCNO dependent_docno)
{
    /* the document that depends on the mailshot source document that is closing must be updated */
    P_DOCU p_docu_dependent = p_docu_from_docno(dependent_docno);
    P_MAILSHOT_INSTANCE_DATA p_mailshot_instance_data = p_object_instance_data_MAILSHOT_valid(p_docu_dependent);

    IGNOREPARM_InVal_(source_docno);
    /*reportf(TEXT("mailshot_msg_close1(source_docno=%d): dependent_docno=%d (maeve_handle was %d)"), source_docno, dependent_docno, p_mailshot_instance_data->maeve_handle);*/

    p_mailshot_instance_data->source_docno = DOCNO_NONE;
    p_mailshot_instance_data->maeve_handle = 0;

    mailshot_fields_dispose(p_mailshot_instance_data);

    mailshot_change(p_docu_dependent);

    return(STATUS_OK);
}

_Check_return_
static STATUS
maeve_mailshot_msg_initclose(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message_dummy,
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose,
    _InRef_     PC_MAEVE_BLOCK p_maeve_block)
{
    IGNOREPARM_InVal_(t5_message_dummy); /* dummy for APCS */

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__CLOSE1:
        {
        const DOCNO dependent_docno = (DOCNO) p_maeve_block->client_handle;
        /*reportf(TEXT("maeve_mailshot_msg_initclose__close1(docno=%d): this maeve_handle=%d"), docno_from_p_docu(p_docu), p_maeve_block->maeve_handle);*/
        maeve_event_handler_del_handle(p_docu, p_maeve_block->maeve_handle);
        return(mailshot_msg_close1(docno_from_p_docu(p_docu), dependent_docno));
        }

    default:
        return(STATUS_OK);
    }
}

MAEVE_EVENT_PROTO(static, maeve_event_ob_mails)
{
    IGNOREPARM_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_mailshot_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data, p_maeve_block));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* mailshot object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, mailshot_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(resource_init(OBJECT_ID_MAILSHOT, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_MAILSHOT));

        return(register_object_construct_table(OBJECT_ID_MAILSHOT, object_construct_table, FALSE /* no inlines */));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_MAILSHOT));

    case T5_MSG_IC__CLOSE1:
        /*reportf(TEXT("mailshot_msg_initclose__close1: docno=%d"), docno_from_p_docu(p_docu));*/
        return(mailshot_instance_data_dispose(p_docu));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_mailshot);
OBJECT_PROTO(extern, object_mailshot)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(mailshot_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_READ_MAIL_TEXT:
        return(mailshot_msg_read_mail_text(p_docu, t5_message, (P_READ_MAIL_TEXT) p_data));

    case T5_CMD_INSERT_FIELD_INTRO_FIELD:
        return(mailshot_cmd_insert_field_intro_field(p_docu));

    case T5_CMD_MAILSHOT_SELECT:
        return(mailshot_cmd_mailshot_select(p_docu));

    case T5_CMD_MAILSHOT_PRINT:
        return(mailshot_cmd_mailshot_print(docno_from_p_docu(p_docu)));

    default:
        return(STATUS_OK);
    }
}

/* end of ob_mails.c */
