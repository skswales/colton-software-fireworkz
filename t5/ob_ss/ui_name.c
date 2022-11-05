/* ui_name.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* UI for adding names */

/* SKS July 1993 */

/* diz July 93 hacks added for name modification and editing line */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

/*
internal routines
*/

_Check_return_
static STATUS
ss_name_add(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id);

_Check_return_
static STATUS
ss_name_edit(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR p_name,
    _InoutRef_opt_ P_QUICK_UBLOCK p_quick_ublock_value,
    _In_opt_z_  PC_USTR ustr_description,
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id);

/******************************************************************************
*
* insert a name into a document
*
******************************************************************************/

typedef struct SS_NAME_LIST_ENTRY
{
    QUICK_UBLOCK_WITH_BUFFER(name_quick_ublock, 16); /* NB buffer adjacent for fixup */
}
SS_NAME_LIST_ENTRY, * P_SS_NAME_LIST_ENTRY; typedef const SS_NAME_LIST_ENTRY * PC_SS_NAME_LIST_ENTRY;

enum SS_NAME_INTRO_CONTROL_IDS
{
    SS_NAME_INTRO_ID_LIST_LABEL = 164,
    SS_NAME_INTRO_ID_LIST,
    SS_NAME_INTRO_ID_VALUE_LABEL,
    SS_NAME_INTRO_ID_VALUE,
    SS_NAME_INTRO_ID_DESC_LABEL,
    SS_NAME_INTRO_ID_DESC,

    SS_NAME_INTRO_ID_INSERT_ADJUST, /*logical*/

    SS_NAME_INTRO_ID_DELETE,
    SS_NAME_INTRO_ID_DELETE_ADJUST, /*logical*/
    SS_NAME_INTRO_ID_EDIT,
    SS_NAME_INTRO_ID_ADD
};

#define SS_NAME_INTRO_NAME_FIELD_WIDTH DIALOG_SYSCHARS_H("QuiteLongNamesGetDisplayedHere")

static const DIALOG_CONTROL
ss_name_intro_list_label =
{
    SS_NAME_INTRO_ID_LIST_LABEL, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, SS_NAME_INTRO_ID_LIST },
    { 0, 0, 0, DIALOG_STDTEXT_V },
    { DRT(LTRT, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
ss_name_intro_list_label_data = { UI_TEXT_INIT_RESID(SS_MSG_DIALOG_NAME_NAME), { 1 /* left text */ } };

static const DIALOG_CONTROL
ss_name_intro_list =
{
    SS_NAME_INTRO_ID_LIST, DIALOG_MAIN_GROUP,
    { SS_NAME_INTRO_ID_LIST_LABEL, SS_NAME_INTRO_ID_LIST_LABEL },
    { 0, DIALOG_LABELGAP_V, DIALOG_STDEDITOVH_H + SS_NAME_INTRO_NAME_FIELD_WIDTH, DIALOG_STDLISTOVH_V + (8 * DIALOG_STDLISTITEM_V) },
    { DRT(LBLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
ss_name_intro_value_label =
{
    SS_NAME_INTRO_ID_VALUE_LABEL, DIALOG_MAIN_GROUP,
    { SS_NAME_INTRO_ID_LIST_LABEL, SS_NAME_INTRO_ID_LIST, SS_NAME_INTRO_ID_VALUE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDTEXT_V },
    { DRT(LBRT, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
ss_name_intro_value_label_data = { UI_TEXT_INIT_RESID(SS_MSG_DIALOG_NAME_VALUE), { 1 /* left text */ } };

static const DIALOG_CONTROL
ss_name_intro_value =
{
    SS_NAME_INTRO_ID_VALUE, DIALOG_MAIN_GROUP,
    { SS_NAME_INTRO_ID_LIST, SS_NAME_INTRO_ID_VALUE_LABEL, SS_NAME_INTRO_ID_LIST },
    { 0, DIALOG_LABELGAP_V, 0, DIALOG_STDEDIT_V },
    { DRT(LBRT, EDIT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL_DATA_EDIT
ss_name_intro_value_data = { { { FRAMED_BOX_EDIT_READ_ONLY, 1 /*read_only*/ }, NULL } /*EDIT_XX*/ };

static const DIALOG_CONTROL
ss_name_intro_desc_label =
{
    SS_NAME_INTRO_ID_DESC_LABEL, DIALOG_MAIN_GROUP,
    { SS_NAME_INTRO_ID_VALUE_LABEL, SS_NAME_INTRO_ID_VALUE, SS_NAME_INTRO_ID_DESC },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDTEXT_V },
    { DRT(LBRT, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
ss_name_intro_desc_label_data = { UI_TEXT_INIT_RESID(SS_MSG_DIALOG_NAME_DESC), { 1 /* left text */ } };

static const DIALOG_CONTROL
ss_name_intro_desc =
{
    SS_NAME_INTRO_ID_DESC, DIALOG_MAIN_GROUP,
    { SS_NAME_INTRO_ID_VALUE, SS_NAME_INTRO_ID_DESC_LABEL, IDOK, DIALOG_CONTROL_SELF },
    { 0, DIALOG_LABELGAP_V, 0, DIALOG_MULEDIT_V(3) },
    { DRT(LBRT, EDIT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_EDIT
ss_name_intro_desc_data = { { { FRAMED_BOX_EDIT_READ_ONLY, 1 /*read_only*/, 0, 1 /*multiline*/ }, NULL }, /* EDIT_XX */ { UI_TEXT_TYPE_NONE } /* UI_TEXT state */ };

static const DIALOG_CONTROL
ss_name_intro_insert =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { SS_NAME_INTRO_ID_LIST, SS_NAME_INTRO_ID_LIST_LABEL, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF },
    { DIALOG_STDSPACING_H, 0, ((3 * PIXITS_PER_INCH) / 4) + DIALOG_PUSHBUTTONOVH_H + (2 * DIALOG_DEFPUSHEXTRA_H), DIALOG_DEFPUSHBUTTON_V },
    { DRT(RTLT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTONR
ss_name_intro_insert_data = { { IDOK, 0, 0, 1 /*alternate_right*/ }, UI_TEXT_INIT_RESID(SS_MSG_DIALOG_NAME_INSERT), SS_NAME_INTRO_ID_INSERT_ADJUST };

static const DIALOG_CONTROL
ss_name_intro_edit =
{
    SS_NAME_INTRO_ID_EDIT, DIALOG_CONTROL_WINDOW,
    { IDOK, IDOK, IDOK, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
ss_name_intro_edit_data = { { SS_NAME_INTRO_ID_EDIT }, UI_TEXT_INIT_RESID(SS_MSG_DIALOG_NAME_EDIT) };

static const DIALOG_CONTROL
ss_name_intro_add =
{
    SS_NAME_INTRO_ID_ADD, DIALOG_CONTROL_WINDOW,
    { SS_NAME_INTRO_ID_EDIT, SS_NAME_INTRO_ID_EDIT, SS_NAME_INTRO_ID_EDIT, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
ss_name_intro_add_data = { { SS_NAME_INTRO_ID_ADD }, UI_TEXT_INIT_RESID(SS_MSG_DIALOG_NAME_ADD) };

static const DIALOG_CONTROL
ss_name_intro_delete =
{
    SS_NAME_INTRO_ID_DELETE, DIALOG_CONTROL_WINDOW,
    { SS_NAME_INTRO_ID_ADD, SS_NAME_INTRO_ID_ADD, SS_NAME_INTRO_ID_ADD, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTONR
ss_name_intro_delete_data = { { SS_NAME_INTRO_ID_DELETE, 0, 0, 1 /*alternate_right*/ }, UI_TEXT_INIT_RESID(MSG_BUTTON_DELETE), SS_NAME_INTRO_ID_DELETE_ADJUST };

static const DIALOG_CONTROL
ss_name_intro_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
    { SS_NAME_INTRO_ID_DELETE, SS_NAME_INTRO_ID_DELETE, SS_NAME_INTRO_ID_DELETE, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CTL_CREATE
ss_name_intro_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &ss_name_intro_list_label },    &ss_name_intro_list_label_data },
    { { &ss_name_intro_list },          &stdlisttext_data },

    { { &ss_name_intro_value_label },   &ss_name_intro_value_label_data },
    { { &ss_name_intro_value },         &ss_name_intro_value_data },

    { { &ss_name_intro_desc_label },    &ss_name_intro_desc_label_data },
    { { &ss_name_intro_desc },          &ss_name_intro_desc_data },

    { { &ss_name_intro_insert },        &ss_name_intro_insert_data },
    { { &ss_name_intro_edit },          &ss_name_intro_edit_data },
    { { &ss_name_intro_add },           &ss_name_intro_add_data },
    { { &ss_name_intro_delete },        &ss_name_intro_delete_data },
    { { &ss_name_intro_cancel },        &stdbutton_cancel_data }
};

static const DIALOG_CONTROL
ss_name_intro_insert_desc =
{
    SS_NAME_INTRO_ID_DESC, DIALOG_MAIN_GROUP,
    { SS_NAME_INTRO_ID_VALUE, SS_NAME_INTRO_ID_DESC_LABEL, SS_NAME_INTRO_ID_VALUE, DIALOG_CONTROL_SELF },
    { 0, DIALOG_LABELGAP_V, 0, (3 * PIXITS_PER_INCH) / 4 },
    { DRT(LBRT, EDIT), 1 /*tabstop*/ }
};

static const DIALOG_CTL_CREATE
ss_name_intro_insert_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &ss_name_intro_list_label },    &ss_name_intro_list_label_data },
    { { &ss_name_intro_list },          &stdlisttext_data },

    { { &ss_name_intro_value_label },   &ss_name_intro_value_label_data },
    { { &ss_name_intro_value },         &ss_name_intro_value_data },

    { { &ss_name_intro_desc_label },    &ss_name_intro_desc_label_data },
    { { &ss_name_intro_insert_desc },   &ss_name_intro_desc_data }, /* different */

    { { &defbutton_ok },                &ss_name_intro_insert_data }, /* different */
    { { &stdbutton_cancel },            &stdbutton_cancel_data } /* different */
};

/******************************************************************************
*
* oh, the joys of sorting ARRAY_QUICK_BLOCKs...
*
******************************************************************************/

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, proc_qsort_ss_name_list, SS_NAME_LIST_ENTRY)
{
    QSORT_ARG1_VAR_DECL(PC_SS_NAME_LIST_ENTRY, p_ss_name_list_entry_1);
    QSORT_ARG2_VAR_DECL(PC_SS_NAME_LIST_ENTRY, p_ss_name_list_entry_2);

    PC_QUICK_UBLOCK p_quick_ublock_1 = &p_ss_name_list_entry_1->name_quick_ublock;
    PC_QUICK_UBLOCK p_quick_ublock_2 = &p_ss_name_list_entry_2->name_quick_ublock;

    PC_USTR ustr_1 = quick_ublock_array_handle_ref(p_quick_ublock_1) ? array_ustr(&quick_ublock_array_handle_ref(p_quick_ublock_1)) : (PC_USTR) (p_quick_ublock_1 + 1);
    PC_USTR ustr_2 = quick_ublock_array_handle_ref(p_quick_ublock_2) ? array_ustr(&quick_ublock_array_handle_ref(p_quick_ublock_2)) : (PC_USTR) (p_quick_ublock_2 + 1);

    return(ustr_compare_nocase(ustr_1, ustr_2));
}

_Check_return_
static STATUS
name_handle_from_external_id(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR ustr_name_id)
{
    STATUS status = STATUS_FAIL;
    DOCNO docno;

    if(status_ok(status = docno_from_id(p_docu, &docno, ustr_name_id, FALSE /* ensure */)))
    {
        const U32 doc_prefix_len = (U32) status;
        ARRAY_INDEX name_num;

        ustr_name_id = ustr_AddBytes(ustr_name_id, doc_prefix_len);

        if((name_num = find_name_in_list((EV_DOCNO) docno, ustr_name_id)) >= 0)
            status = array_ptr(&name_def_deptable.h_table, EV_NAME, name_num)->handle;
    }

    return(status);
}

typedef struct SS_NAME_INTRO_CALLBACK
{
    S32 name_selector_type;
    S32 selected_ss_name;
    ARRAY_HANDLE ss_name_list_handle;
    UI_SOURCE ss_name_list_source;
}
SS_NAME_INTRO_CALLBACK, * P_SS_NAME_INTRO_CALLBACK;

static void
encode_from_selected_name(
    _DocuRef_   P_DOCU p_docu,
    P_SS_NAME_INTRO_CALLBACK p_ss_name_intro_callback,
    _InVal_     H_DIALOG h_dialog)
{
    P_USTR ustr_description = NULL;
    BOOL kosher_selection = FALSE;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 16);
    quick_ublock_with_buffer_setup(quick_ublock);

    if(array_index_is_valid(&p_ss_name_intro_callback->ss_name_list_handle, p_ss_name_intro_callback->selected_ss_name))
    {
        P_SS_NAME_LIST_ENTRY p_ss_name_list_entry = array_ptr_no_checks(&p_ss_name_intro_callback->ss_name_list_handle, SS_NAME_LIST_ENTRY, p_ss_name_intro_callback->selected_ss_name);
        PC_USTR ustr_name = quick_ublock_ustr(&p_ss_name_list_entry->name_quick_ublock);
        STATUS status;

        if(status_ok(status = name_handle_from_external_id(p_docu, ustr_name)))
        {
            SS_NAME_READ ss_name_read;
            SS_RECOG_CONTEXT ss_recog_context;
            zero_struct_fn(ss_name_read);
            ss_name_read.ev_handle = status;
            ss_data_set_blank(&ss_name_read.ss_data);
            ss_name_read.follow_indirection = FALSE;
            ss_recog_context_push(&ss_recog_context);
            status = object_call_id(OBJECT_ID_SS, p_docu, T5_MSG_SS_NAME_READ, &ss_name_read);
            if(status_ok(status))
                status = ss_data_decode(&quick_ublock, &ss_name_read.ss_data, ev_docno_from_p_docu(p_docu));
            ss_recog_context_pull(&ss_recog_context);
            ustr_description = ss_name_read.ustr_description; /* NOT owned by us */
            kosher_selection = TRUE;
        }
    }

    {
    UI_TEXT ui_text;
    ui_text.type = UI_TEXT_TYPE_NONE;
    if(0 != quick_ublock_bytes(&quick_ublock))
    {
        if(status_ok(status_wrap(quick_ublock_nullch_add(&quick_ublock))))
        {
            ui_text.type = UI_TEXT_TYPE_USTR_TEMP;
            ui_text.text.ustr = quick_ublock_ustr(&quick_ublock);
        }
    }
    status_assert(ui_dlg_set_edit(h_dialog, SS_NAME_INTRO_ID_VALUE, &ui_text));
    quick_ublock_dispose(&quick_ublock);
    } /*block*/

    {
    UI_TEXT ui_text;
    ui_text.type = UI_TEXT_TYPE_NONE;
    if(P_DATA_NONE != ustr_description)
    {
        ui_text.type = UI_TEXT_TYPE_USTR_TEMP;
        ui_text.text.ustr = ustr_description;
    }
    status_assert(ui_dlg_set_edit(h_dialog, SS_NAME_INTRO_ID_DESC, &ui_text));
    } /*block*/

    /* If no item selected in the list then grey out insert, edit and delete 'cos they are no longer applicable */
    ui_dlg_ctl_enable(h_dialog, IDOK, kosher_selection);
    if(p_ss_name_intro_callback->name_selector_type == 1)
    {
        ui_dlg_ctl_enable(h_dialog, SS_NAME_INTRO_ID_EDIT, kosher_selection);
        ui_dlg_ctl_enable(h_dialog, SS_NAME_INTRO_ID_DELETE, kosher_selection);
    }
}

_Check_return_
static STATUS
ss_name_intro_lists_create(
    _DocuRef_   P_DOCU p_docu,
    P_SS_NAME_INTRO_CALLBACK p_ss_name_intro_callback)
{
    STATUS status = STATUS_OK;

    p_ss_name_intro_callback->ss_name_list_handle = 0;

    p_ss_name_intro_callback->ss_name_list_source.type = UI_SOURCE_TYPE_NONE;

    {
    EV_RESOURCE ev_resource;
    RESOURCE_SPEC resource_spec;

    ev_enum_resource_init(&ev_resource, &resource_spec, EV_RESO_NAMES, 0, DOCNO_NONE, ev_docno_from_p_docu(p_docu));

    for(;;)
    {
        P_SS_NAME_LIST_ENTRY p_ss_name_list_entry;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*p_ss_name_list_entry), 0);
        STATUS ev_item_no = ev_enum_resource_get(&resource_spec, &ev_resource);

        if(ev_item_no < 0)
        {
            if(STATUS_FAIL != ev_item_no)
                status = ev_item_no;
            break;
        }

        if(NULL != (p_ss_name_list_entry = al_array_extend_by(&p_ss_name_intro_callback->ss_name_list_handle, SS_NAME_LIST_ENTRY, 1, &array_init_block, &status)))
        {
            /* each ***entry*** has a quick buf for textual representation in list box! */
            quick_ublock_with_buffer_setup(p_ss_name_list_entry->name_quick_ublock);

            status = quick_ublock_ustr_add_n(&p_ss_name_list_entry->name_quick_ublock, array_ustr(&resource_spec.h_id_ustr), strlen_with_NULLCH);
        }

        status_break(status);
    }

    ev_enum_resource_dispose(&resource_spec);
    } /*block*/

    /* sort the array we built */
    ui_source_list_fixup_ub(&p_ss_name_intro_callback->ss_name_list_handle, offsetof32(SS_NAME_LIST_ENTRY, name_quick_ublock)); /* fixup the quick blocks prior to sort */
    al_array_qsort(&p_ss_name_intro_callback->ss_name_list_handle, proc_qsort_ss_name_list);
    ui_source_list_fixup_ub(&p_ss_name_intro_callback->ss_name_list_handle, offsetof32(SS_NAME_LIST_ENTRY, name_quick_ublock)); /* fixup the quick blocks after the sort */

    /* make a source of text pointers to these elements for list box processing */
    if(status_ok(status))
        status = ui_source_create_ub(&p_ss_name_intro_callback->ss_name_list_handle, &p_ss_name_intro_callback->ss_name_list_source, UI_TEXT_TYPE_USTR_PERM, offsetof32(SS_NAME_LIST_ENTRY, name_quick_ublock));

    return(status);
}

_Check_return_
static STATUS
ss_name_intro_delete_using_callback(
    _DocuRef_   P_DOCU p_docu,
    P_SS_NAME_INTRO_CALLBACK p_ss_name_intro_callback)
{
    STATUS status = STATUS_OK;

    if(array_index_is_valid(&p_ss_name_intro_callback->ss_name_list_handle, p_ss_name_intro_callback->selected_ss_name))
    {
        P_SS_NAME_LIST_ENTRY p_ss_name_list_entry = array_ptr_no_checks(&p_ss_name_intro_callback->ss_name_list_handle, SS_NAME_LIST_ENTRY, p_ss_name_intro_callback->selected_ss_name);
        PC_USTR ustr_name = quick_ublock_ustr(&p_ss_name_list_entry->name_quick_ublock);
        SS_NAME_MAKE ss_name_make;
        zero_struct_fn(ss_name_make);
        ss_name_make.ustr_name_id = ustr_name;
        ss_name_make.ustr_name_def = NULL;
        ss_name_make.undefine = TRUE;
        status = object_call_id(OBJECT_ID_SS, p_docu, T5_MSG_SS_NAME_MAKE, &ss_name_make);
    }

    return(status);
}

_Check_return_
static STATUS
ss_name_intro_edit_using_callback(
    _DocuRef_   P_DOCU p_docu,
    P_SS_NAME_INTRO_CALLBACK p_ss_name_intro_callback,
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id)
{
    STATUS status = STATUS_OK;

    if(array_index_is_valid(&p_ss_name_intro_callback->ss_name_list_handle, p_ss_name_intro_callback->selected_ss_name))
    {
        P_SS_NAME_LIST_ENTRY p_ss_name_list_entry = array_ptr_no_checks(&p_ss_name_intro_callback->ss_name_list_handle, SS_NAME_LIST_ENTRY, p_ss_name_intro_callback->selected_ss_name);
        PC_USTR ustr_name = quick_ublock_ustr(&p_ss_name_list_entry->name_quick_ublock);

        if(status_ok(status = name_handle_from_external_id(p_docu, ustr_name)))
        {
            P_USTR ustr_description;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 16);
            quick_ublock_with_buffer_setup(quick_ublock);

            {
            SS_NAME_READ ss_name_read;
            SS_RECOG_CONTEXT ss_recog_context;
            zero_struct_fn(ss_name_read);
            ss_name_read.ev_handle = status;
            ss_data_set_blank(&ss_name_read.ss_data);
            ss_name_read.follow_indirection = FALSE;
            ss_recog_context_push(&ss_recog_context);
            if(status_ok(status = object_call_id(OBJECT_ID_SS, p_docu, T5_MSG_SS_NAME_READ, &ss_name_read)))
                status = ss_data_decode(&quick_ublock, &ss_name_read.ss_data, ev_docno_from_p_docu(p_docu));
            ss_recog_context_pull(&ss_recog_context);
            ustr_description = ss_name_read.ustr_description; /* NOT owned by us */
            } /*block*/

            if(status_ok(status))
                status = ss_name_edit(p_docu, ustr_name, &quick_ublock, ustr_description, h_dialog, dialog_control_id);

            quick_ublock_dispose(&quick_ublock);
        }
    }

    return(status);
}

_Check_return_
static STATUS
dialog_ss_name_intro_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    const P_SS_NAME_INTRO_CALLBACK p_ss_name_intro_callback = (P_SS_NAME_INTRO_CALLBACK) p_dialog_msg_ctl_fill_source->client_handle;

    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case SS_NAME_INTRO_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &p_ss_name_intro_callback->ss_name_list_source;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_ss_name_intro_ctl_state_change(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_SS_NAME_INTRO_CALLBACK p_ss_name_intro_callback = (P_SS_NAME_INTRO_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case SS_NAME_INTRO_ID_LIST:
        p_ss_name_intro_callback->selected_ss_name = p_dialog_msg_ctl_state_change->new_state.list_text.itemno;
        encode_from_selected_name(p_docu, p_ss_name_intro_callback, p_dialog_msg_ctl_state_change->h_dialog);
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_ss_name_intro_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_SS_NAME_INTRO_CALLBACK p_ss_name_intro_callback = (P_SS_NAME_INTRO_CALLBACK) p_dialog_msg_process_start->client_handle;

    return(ui_dlg_set_list_idx(p_dialog_msg_process_start->h_dialog, SS_NAME_INTRO_ID_LIST, p_ss_name_intro_callback->selected_ss_name));
}

#if WINDOWS

_Check_return_
static STATUS
dialog_ss_name_intro_ctl_pushbutton(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    const P_SS_NAME_INTRO_CALLBACK p_ss_name_intro_callback = (P_SS_NAME_INTRO_CALLBACK) p_dialog_msg_ctl_pushbutton->client_handle;
    const DIALOG_CONTROL_ID dialog_control_id = p_dialog_msg_ctl_pushbutton->dialog_control_id;
    STATUS status = STATUS_OK;

    switch(dialog_control_id)
    {
    case SS_NAME_INTRO_ID_ADD:
        status = ss_name_add(p_docu, p_dialog_msg_ctl_pushbutton->h_dialog, dialog_control_id);
        p_dialog_msg_ctl_pushbutton->processed = TRUE;
        /* and do NOT complete dialog */

        if(status_ok(status))
        {
            ui_lists_dispose_ub(&p_ss_name_intro_callback->ss_name_list_handle, &p_ss_name_intro_callback->ss_name_list_source, offsetof32(SS_NAME_LIST_ENTRY, name_quick_ublock));
            status = ss_name_intro_lists_create(p_docu, p_ss_name_intro_callback);
        }

        ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, SS_NAME_INTRO_ID_LIST);

        encode_from_selected_name(p_docu, p_ss_name_intro_callback,  p_dialog_msg_ctl_pushbutton->h_dialog);
        break;

    case SS_NAME_INTRO_ID_DELETE:
        status = ss_name_intro_delete_using_callback(p_docu, p_ss_name_intro_callback);
        p_dialog_msg_ctl_pushbutton->processed = TRUE;
        /* and do NOT complete dialog */

        if(status_ok(status))
        {
            ui_lists_dispose_ub(&p_ss_name_intro_callback->ss_name_list_handle, &p_ss_name_intro_callback->ss_name_list_source, offsetof32(SS_NAME_LIST_ENTRY, name_quick_ublock));
            status = ss_name_intro_lists_create(p_docu, p_ss_name_intro_callback);
        }

        ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, SS_NAME_INTRO_ID_LIST);

        encode_from_selected_name(p_docu, p_ss_name_intro_callback,  p_dialog_msg_ctl_pushbutton->h_dialog);
        break;

    case SS_NAME_INTRO_ID_EDIT:
        status = ss_name_intro_edit_using_callback(p_docu, p_ss_name_intro_callback, p_dialog_msg_ctl_pushbutton->h_dialog, dialog_control_id);
        p_dialog_msg_ctl_pushbutton->processed = TRUE;
        /* and do NOT complete dialog */

        encode_from_selected_name(p_docu, p_ss_name_intro_callback,  p_dialog_msg_ctl_pushbutton->h_dialog);
        break;

    default:
        break;
    }

    return(status);
}

#endif /* OS */

PROC_DIALOG_EVENT_PROTO(static, dialog_event_ss_name_intro)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_ss_name_intro_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_ss_name_intro_ctl_state_change(p_docu, (PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_ss_name_intro_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

#if WINDOWS
    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_ss_name_intro_ctl_pushbutton(p_docu, (P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));
#endif

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(extern, t5_cmd_ss_name_intro)
{
    SS_NAME_INTRO_CALLBACK ss_name_intro_callback;
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* 0 -> insert name as inline into text */
    /* 1 -> Fireworkz variables box */

    ss_name_intro_callback.name_selector_type = 1; /* main names dialog */

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        P_ARGLIST_ARG p_arg;
        if(arg_present(&p_t5_cmd->arglist_handle, 0, &p_arg))
            ss_name_intro_callback.name_selector_type = p_arg->val.s32;
    }

    ss_name_intro_callback.selected_ss_name = 0;

    if(status_ok(status = ss_name_intro_lists_create(p_docu, &ss_name_intro_callback)))
    {
        {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        if(ss_name_intro_callback.name_selector_type != 1)
            dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, ss_name_intro_insert_ctl_create, elemof32(ss_name_intro_insert_ctl_create), SS_MSG_DIALOG_NAME_INTRO_CAPTION);
        else
            dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, ss_name_intro_ctl_create, elemof32(ss_name_intro_ctl_create), SS_MSG_DIALOG_NAME_INTRO_CAPTION);
        dialog_cmd_process_dbox.help_topic_resource_id = SS_MSG_DIALOG_NAME_INTRO_HELP_TOPIC;
#if RISCOS
        dialog_cmd_process_dbox.bits.note_position = 1;
#endif
        dialog_cmd_process_dbox.p_proc_client = dialog_event_ss_name_intro;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &ss_name_intro_callback;
        status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        } /*block*/

        switch(status)
        {
        default:
            break;

        case IDOK:
        case SS_NAME_INTRO_ID_INSERT_ADJUST:
            {
            if(OBJECT_ID_SLE != p_docu->focus_owner)
                status = check_protection_simple(p_docu, FALSE);

            if(status_ok(status))
            if(array_index_is_valid(&ss_name_intro_callback.ss_name_list_handle, ss_name_intro_callback.selected_ss_name))
            {
                P_SS_NAME_LIST_ENTRY p_ss_name_list_entry = array_ptr_no_checks(&ss_name_intro_callback.ss_name_list_handle, SS_NAME_LIST_ENTRY, ss_name_intro_callback.selected_ss_name);

                if(ss_name_intro_callback.name_selector_type == 0)
                {
                    PC_USTR ustr_name = quick_ublock_ustr(&p_ss_name_list_entry->name_quick_ublock);

                    if(status_ok(status = name_handle_from_external_id(p_docu, ustr_name)))
                    {
                        EV_HANDLE ev_handle_name = status;
                        const OBJECT_ID object_id = OBJECT_ID_SKEL;
                        T5_MESSAGE new_t5_message = T5_CMD_INSERT_FIELD_SS_NAME;
                        PC_CONSTRUCT_TABLE p_construct_table;
                        ARGLIST_HANDLE arglist_handle;

                        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, new_t5_message, &p_construct_table)))
                        {
                            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
                            p_args[0].val.s32 = ev_handle_name;
                            status = execute_command(p_docu, new_t5_message, &arglist_handle, object_id);
                            arglist_dispose(&arglist_handle);
                        }
                    }
                }
                else if((ss_name_intro_callback.name_selector_type == 1) || (ss_name_intro_callback.name_selector_type == 3))
                {
                    T5_PASTE_EDITLINE t5_paste_editline;
                    t5_paste_editline.p_quick_ublock = &p_ss_name_list_entry->name_quick_ublock; /* insert actual name */
                    t5_paste_editline.select = FALSE;
                    t5_paste_editline.special_select = FALSE;
                    quick_ublock_nullch_strip(&p_ss_name_list_entry->name_quick_ublock);
                    status = object_call_id(OBJECT_ID_SLE, p_docu, (OBJECT_ID_SLE == p_docu->focus_owner) ? T5_MSG_PASTE_EDITLINE : T5_MSG_ACTIVATE_EDITLINE, &t5_paste_editline);
                }
                /* otherwise it's Edit->Variables where ok simply closes dialog */
            }

            break;
            }

#if RISCOS /* do these here */
        case SS_NAME_INTRO_ID_ADD:
            status = ss_name_add(p_docu, 0, 0);
            break;

        case SS_NAME_INTRO_ID_EDIT:
            status = ss_name_intro_edit_using_callback(p_docu, &ss_name_intro_callback, 0, 0);
            break;

        case SS_NAME_INTRO_ID_DELETE:
        case SS_NAME_INTRO_ID_DELETE_ADJUST:
            status = ss_name_intro_delete_using_callback(p_docu, &ss_name_intro_callback);
            break;
#endif
        }
    }

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    ui_lists_dispose_ub(&ss_name_intro_callback.ss_name_list_handle, &ss_name_intro_callback.ss_name_list_source, offsetof32(SS_NAME_LIST_ENTRY, name_quick_ublock));

    return(status);
}

/******************************************************************************
*
* Edit name
*
******************************************************************************/

enum SS_NAME_EDIT_CONTROL_IDS
{
    SS_NAME_EDIT_ID_NAME_LABEL = 289,
    SS_NAME_EDIT_ID_NAME,
    SS_NAME_EDIT_ID_VALUE_LABEL,
    SS_NAME_EDIT_ID_VALUE,
    SS_NAME_EDIT_ID_DESC_LABEL,
    SS_NAME_EDIT_ID_DESC,
    SS_NAME_EDIT_ID_HELP
};

static const DIALOG_CONTROL
ss_name_edit_name_label =
{
    SS_NAME_EDIT_ID_NAME_LABEL, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, SS_NAME_EDIT_ID_NAME },
    { 0, 0, 0, DIALOG_STDTEXT_V },
    { DRT(LTRT, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
ss_name_edit_name_label_data = { UI_TEXT_INIT_RESID(SS_MSG_DIALOG_NAME_NAME), { 1 /*left_text*/ } };

static /*poked*/ DIALOG_CONTROL
ss_name_edit_name =
{
    SS_NAME_EDIT_ID_NAME, DIALOG_MAIN_GROUP,
    { SS_NAME_EDIT_ID_NAME_LABEL, SS_NAME_EDIT_ID_NAME_LABEL },
    { 0, DIALOG_LABELGAP_V, DIALOG_STDEDITOVH_H + SS_NAME_INTRO_NAME_FIELD_WIDTH, DIALOG_STDEDIT_V },
    { DRT(LBLT, EDIT), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_EDIT
ss_name_edit_name_data = { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/ };

static const DIALOG_CONTROL
ss_name_edit_value_label =
{
    SS_NAME_EDIT_ID_VALUE_LABEL, DIALOG_MAIN_GROUP,
    { SS_NAME_EDIT_ID_NAME_LABEL, SS_NAME_EDIT_ID_NAME, SS_NAME_EDIT_ID_VALUE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDTEXT_V },
    { DRT(LBRT, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
ss_name_edit_value_label_data = { UI_TEXT_INIT_RESID(SS_MSG_DIALOG_NAME_VALUE), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
ss_name_edit_value =
{
    SS_NAME_EDIT_ID_VALUE, DIALOG_MAIN_GROUP,
    { SS_NAME_EDIT_ID_VALUE_LABEL, SS_NAME_EDIT_ID_VALUE_LABEL, SS_NAME_EDIT_ID_NAME },
    { 0, DIALOG_LABELGAP_V, 0, DIALOG_STDEDIT_V },
    { DRT(LBRT, EDIT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_EDIT
ss_name_edit_value_data = { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/ };

static const DIALOG_CONTROL
ss_name_edit_desc_label =
{
    SS_NAME_EDIT_ID_DESC_LABEL, DIALOG_MAIN_GROUP,
    { SS_NAME_EDIT_ID_VALUE_LABEL, SS_NAME_EDIT_ID_VALUE, SS_NAME_EDIT_ID_DESC },
    { 0, DIALOG_LABELGAP_V, 0, DIALOG_STDTEXT_V },
    { DRT(LBRT, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
ss_name_edit_desc_label_data = { UI_TEXT_INIT_RESID(SS_MSG_DIALOG_NAME_DESC), { 1 /* left text */ } };

static const DIALOG_CONTROL
ss_name_edit_desc =
{
    SS_NAME_EDIT_ID_DESC, DIALOG_MAIN_GROUP,
    { SS_NAME_EDIT_ID_VALUE, SS_NAME_EDIT_ID_DESC_LABEL, SS_NAME_EDIT_ID_VALUE, DIALOG_CONTROL_SELF },
    { 0, DIALOG_LABELGAP_V, 0, DIALOG_MULEDIT_V(3) },
    { DRT(LBRT, EDIT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_EDIT
ss_name_edit_desc_data = { { { FRAMED_BOX_EDIT, 0 /*read_only*/, 0, 1 /*multiline*/ }, NULL }, /* EDIT_XX */ { UI_TEXT_TYPE_NONE } /* UI_TEXT state */ };

static /*poked*/ DIALOG_CONTROL_DATA_PUSHBUTTON
ss_name_edit_ok_data = { { DIALOG_COMPLETION_OK }, UI_TEXT_INIT_RESID(MSG_BUTTON_CREATE) };

static const DIALOG_CTL_CREATE
ss_name_edit_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &ss_name_edit_name_label },     &ss_name_edit_name_label_data },
    { { &ss_name_edit_name },           &ss_name_edit_name_data },
    { { &ss_name_edit_value_label },    &ss_name_edit_value_label_data },
    { { &ss_name_edit_value },          &ss_name_edit_value_data },
    { { &ss_name_edit_desc_label },     &ss_name_edit_desc_label_data },
    { { &ss_name_edit_desc },           &ss_name_edit_desc_data },

    { { &defbutton_ok },     &ss_name_edit_ok_data },
    { { &stdbutton_cancel }, &stdbutton_cancel_data }
};

typedef struct SS_NAME_EDIT_CALLBACK
{
    UI_TEXT ui_text_name;
    UI_TEXT ui_text_value;
    UI_TEXT ui_text_description;
    BITMAP(name_validation, 256);
}
SS_NAME_EDIT_CALLBACK, * P_SS_NAME_EDIT_CALLBACK;

_Check_return_
static STATUS
dialog_ss_name_edit_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    const P_SS_NAME_EDIT_CALLBACK p_ss_name_edit_callback = (P_SS_NAME_EDIT_CALLBACK) p_dialog_msg_ctl_create_state->client_handle;

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case SS_NAME_EDIT_ID_NAME:
        p_dialog_msg_ctl_create_state->state_set.state.edit.ui_text = p_ss_name_edit_callback->ui_text_name;
        break;

    case SS_NAME_EDIT_ID_VALUE:
        p_dialog_msg_ctl_create_state->state_set.state.edit.ui_text = p_ss_name_edit_callback->ui_text_value;
        break;

    case SS_NAME_EDIT_ID_DESC:
        p_dialog_msg_ctl_create_state->state_set.state.edit.ui_text = p_ss_name_edit_callback->ui_text_description;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_ss_name_edit_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_SS_NAME_EDIT_CALLBACK p_ss_name_edit_callback = (P_SS_NAME_EDIT_CALLBACK) p_dialog_msg_process_end->client_handle;
        const H_DIALOG h_dialog = p_dialog_msg_process_end->h_dialog;

        ui_dlg_get_edit(h_dialog, SS_NAME_EDIT_ID_NAME,  &p_ss_name_edit_callback->ui_text_name);
        ui_dlg_get_edit(h_dialog, SS_NAME_EDIT_ID_VALUE, &p_ss_name_edit_callback->ui_text_value);
        ui_dlg_get_edit(h_dialog, SS_NAME_EDIT_ID_DESC,  &p_ss_name_edit_callback->ui_text_description);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_ss_name_edit)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_ss_name_edit_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_ss_name_edit_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

#if WINDOWS

_Check_return_
static HWND
ss_name_get_parent_hwnd(
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id)
{
    if(0 != h_dialog)
    {
        DIALOG_CMD_HWND_QUERY dialog_cmd_hwnd_query;
        dialog_cmd_hwnd_query.h_dialog = h_dialog;
        dialog_cmd_hwnd_query.dialog_control_id = dialog_control_id;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_HWND_QUERY, &dialog_cmd_hwnd_query));
        return(dialog_cmd_hwnd_query.hwnd);
    }

    return(HOST_WND_NONE);
}

#endif /* OS */

static void
ss_name_edit_validation_setup(
    P_BITMAP p_bitmap_validation)
{
    BIT_NUMBER bit_number;
    bitmap_clear(p_bitmap_validation, N_BITS_ARG(256));
    for(bit_number = 0; bit_number < 256; ++bit_number)
        if(sbchar_isalpha(bit_number) || sbchar_isdigit(bit_number)) /* Latin-1, no remapping (for document portability) */
            bitmap_bit_set(p_bitmap_validation, bit_number, N_BITS_ARG(256));
    bitmap_bit_set(p_bitmap_validation, CH_UNDERSCORE, N_BITS_ARG(256));
}

_Check_return_
static STATUS
ss_name_add(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id)
{
    UCHARZ ustr_buf[BUF_EV_LONGNAMLEN + BUF_EV_INTNAMLEN]; /* SKS 18jul93 buffer[50] was a bit tiny */
    SS_NAME_EDIT_CALLBACK ss_name_edit_callback;
    STATUS status;

#if RISCOS
    UNREFERENCED_PARAMETER_InVal_(h_dialog);
    UNREFERENCED_PARAMETER_InVal_(dialog_control_id);
#endif

    ss_name_edit_callback.ui_text_name.type = UI_TEXT_TYPE_NONE;
    ss_name_edit_callback.ui_text_value.type = UI_TEXT_TYPE_NONE;
    ss_name_edit_callback.ui_text_description.type = UI_TEXT_TYPE_NONE;

    ss_name_edit_validation_setup(ss_name_edit_callback.name_validation);
    ss_name_edit_name_data.edit_xx.p_bitmap_validation = ss_name_edit_callback.name_validation;
    ss_name_edit_name_data.edit_xx.bits.border_style = FRAMED_BOX_EDIT;
    ss_name_edit_name_data.edit_xx.bits.read_only = FALSE;
    ss_name_edit_name.bits.tabstop = 1;

    if(p_docu->mark_info_cells.h_markers)
    {
        const DOCNO docno = docno_from_p_docu(p_docu);
        DOCU_AREA docu_area;
        SLR slr[2];
        EV_SLR ev_slr[2];
        U32 len;
        docu_area_from_markers_first(p_docu, &docu_area);
        limits_from_docu_area(p_docu, &slr[0].col, &slr[1].col, &slr[0].row, &slr[1].row, &docu_area);
        slr[1].col -= 1;
        slr[1].row -= 1;
        ev_slr_from_slr(p_docu, &ev_slr[0], &slr[0]);
        ev_slr_from_slr(p_docu, &ev_slr[1], &slr[1]);
        ev_slr[1].docno = EV_DOCNO_PACK(docno);
        len = ev_dec_slr_ustr_buf(ustr_bptr(ustr_buf), elemof32(ustr_buf), docno, &ev_slr[0]);
        if((ev_slr[0].col != ev_slr[1].col) || (ev_slr[0].row != ev_slr[1].row))
            len += ev_dec_slr_ustr_buf(ustr_bptr(ustr_buf + len), elemof32(ustr_buf) - len, docno, &ev_slr[1]);
        ustr_buf[len] = CH_NULL;
        ss_name_edit_callback.ui_text_value.type = UI_TEXT_TYPE_USTR_PERM;
        ss_name_edit_callback.ui_text_value.text.ustr = ustr_bptr(ustr_buf);
    }

    ss_name_edit_ok_data.caption.text.resource_id = MSG_BUTTON_CREATE;
    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, ss_name_edit_ctl_create, elemof32(ss_name_edit_ctl_create), SS_MSG_DIALOG_NAME_ADD_CAPTION);
    dialog_cmd_process_dbox.help_topic_resource_id = SS_MSG_DIALOG_NAME_INTRO_HELP_TOPIC;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_ss_name_edit;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &ss_name_edit_callback;
#if WINDOWS
    if(0 != h_dialog)
    {
        dialog_cmd_process_dbox.windows.hwnd = ss_name_get_parent_hwnd(h_dialog, dialog_control_id);
        dialog_cmd_process_dbox.bits.use_windows_hwnd = 1;
    }
#endif
    status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    if(status_ok(status))
    {
        if(ui_text_is_blank(&ss_name_edit_callback.ui_text_name))
        {
            status = create_error(SS_ERR_NAME_BLANK_NAME);
        }
        else if(ui_text_is_blank(&ss_name_edit_callback.ui_text_value))
        {
            status = create_error(SS_ERR_NAME_BLANK_VALUE);
        }
        else
        {
            SS_NAME_MAKE ss_name_make;
            SS_RECOG_CONTEXT ss_recog_context;
            zero_struct_fn(ss_name_make);
            ss_name_make.ustr_name_id = ui_text_ustr(&ss_name_edit_callback.ui_text_name);
            ss_name_make.ustr_name_def = ui_text_ustr(&ss_name_edit_callback.ui_text_value);
            ss_name_make.undefine = FALSE;
            ss_name_make.ustr_description = ui_text_ustr_no_default(&ss_name_edit_callback.ui_text_description);
            ss_recog_context_push(&ss_recog_context);
            status = object_call_id(OBJECT_ID_SS, p_docu, T5_MSG_SS_NAME_MAKE, &ss_name_make);
            ss_recog_context_pull(&ss_recog_context);
        }
    }

    ui_text_dispose(&ss_name_edit_callback.ui_text_name);
    ui_text_dispose(&ss_name_edit_callback.ui_text_value);
    ui_text_dispose(&ss_name_edit_callback.ui_text_description);

    return(status);
}

_Check_return_
static STATUS
ss_name_edit(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR ustr_name,
    _InoutRef_opt_ P_QUICK_UBLOCK p_quick_ublock_value /*modified*/,
    _In_opt_z_  PC_USTR ustr_description,
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id)
{
    SS_NAME_EDIT_CALLBACK ss_name_edit_callback;
    STATUS status;

#if RISCOS
    UNREFERENCED_PARAMETER_InVal_(h_dialog);
    UNREFERENCED_PARAMETER_InVal_(dialog_control_id);
#endif

    ss_name_edit_callback.ui_text_name.type = UI_TEXT_TYPE_USTR_TEMP;
    ss_name_edit_callback.ui_text_name.text.ustr = ustr_name;

    ss_name_edit_callback.ui_text_value.type = UI_TEXT_TYPE_NONE;

    if((NULL != p_quick_ublock_value) && (0 != quick_ublock_bytes(p_quick_ublock_value)))
    {
        status_assert(quick_ublock_nullch_add(p_quick_ublock_value)); /*quick butchery*/
        ss_name_edit_callback.ui_text_value.type = UI_TEXT_TYPE_USTR_TEMP;
        ss_name_edit_callback.ui_text_value.text.ustr = quick_ublock_ustr(p_quick_ublock_value);
    }

    ss_name_edit_callback.ui_text_description.type = UI_TEXT_TYPE_NONE;

    if(P_DATA_NONE != ustr_description)
    {
        ss_name_edit_callback.ui_text_description.type = UI_TEXT_TYPE_USTR_TEMP;
        ss_name_edit_callback.ui_text_description.text.ustr = ustr_description;
    }

    ss_name_edit_validation_setup(ss_name_edit_callback.name_validation);
    ss_name_edit_name_data.edit_xx.p_bitmap_validation = ss_name_edit_callback.name_validation;
    ss_name_edit_name_data.edit_xx.bits.border_style = FRAMED_BOX_EDIT_READ_ONLY;
    ss_name_edit_name_data.edit_xx.bits.read_only = TRUE;
    ss_name_edit_name.bits.tabstop = 0;

    ss_name_edit_ok_data.caption.text.resource_id = MSG_BUTTON_APPLY;
    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, ss_name_edit_ctl_create, elemof32(ss_name_edit_ctl_create), SS_MSG_DIALOG_NAME_EDIT_CAPTION);
    dialog_cmd_process_dbox.help_topic_resource_id = SS_MSG_DIALOG_NAME_INTRO_HELP_TOPIC;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_ss_name_edit;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &ss_name_edit_callback;
#if WINDOWS
    if(0 != h_dialog)
    {
        dialog_cmd_process_dbox.windows.hwnd = ss_name_get_parent_hwnd(h_dialog, dialog_control_id);
        dialog_cmd_process_dbox.bits.use_windows_hwnd = 1;
    }
#endif
    status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    if(status_ok(status))
    {
        if(ui_text_is_blank(&ss_name_edit_callback.ui_text_name))
        {
            status = create_error(SS_ERR_NAME_BLANK_NAME);
        }
        else if(ui_text_is_blank(&ss_name_edit_callback.ui_text_value))
        {
            status = create_error(SS_ERR_NAME_BLANK_VALUE);
        }
        else
        {
            SS_NAME_MAKE ss_name_make;
            SS_RECOG_CONTEXT ss_recog_context;
            zero_struct_fn(ss_name_make);
            ss_name_make.ustr_name_id = ui_text_ustr(&ss_name_edit_callback.ui_text_name);
            ss_name_make.ustr_name_def = ui_text_ustr(&ss_name_edit_callback.ui_text_value);
            ss_name_make.undefine = FALSE;
            ss_name_make.ustr_description = ui_text_ustr_no_default(&ss_name_edit_callback.ui_text_description);
            ss_recog_context_push(&ss_recog_context);
            status = object_call_id(OBJECT_ID_SS, p_docu, T5_MSG_SS_NAME_MAKE, &ss_name_make);
            ss_recog_context_pull(&ss_recog_context);
        }
    }

    ui_text_dispose(&ss_name_edit_callback.ui_text_name);
    ui_text_dispose(&ss_name_edit_callback.ui_text_value);
    ui_text_dispose(&ss_name_edit_callback.ui_text_description);

    return(status);
}

/* end of ui_name.c */
