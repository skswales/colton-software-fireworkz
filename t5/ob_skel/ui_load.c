/* ui_load.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* UI for loading for Fireworkz */

/* SKS August 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/ff_io.h"

#include "ob_file/xp_file.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

typedef struct TEMPLATE_LIST_ENTRY
{
    QUICK_TBLOCK_WITH_BUFFER(fullname_quick_tblock, 64); /* NB buffer adjacent for fixup */

    QUICK_TBLOCK_WITH_BUFFER(leafname_quick_tblock, elemof32("doseight.fwk")); /* NB buffer adjacent for fixup */

    int sort_order; /* 0 by default */
}
TEMPLATE_LIST_ENTRY, * P_TEMPLATE_LIST_ENTRY;

enum SELECT_TEMPLATE_CONTROL_IDS
{
    SELECT_TEMPLATE_ID_LIST = 333
};

static /*poked*/ DIALOG_CONTROL
select_template_list =
{
    SELECT_TEMPLATE_ID_LIST, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0 },
    { DRT(LTLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CTL_CREATE
select_template_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &select_template_list }, &stdlisttext_data },

    { { &defbutton_ok }, &defbutton_create_data },
    { { &stdbutton_cancel }, &stdbutton_cancel_data }
};

static bool g_use_default_template = false;

static S32 g_selected_template = 0;

static UI_SOURCE
select_template_list_source;

_Check_return_
static STATUS
dialog_select_template_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case SELECT_TEMPLATE_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &select_template_list_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_select_template_ctl_create_state(P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case SELECT_TEMPLATE_ID_LIST:
        {
        const PC_S32 p_selected_template = (PC_S32) p_dialog_msg_ctl_create_state->client_handle;
        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALTERNATE;
        p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = *p_selected_template;
        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_select_template_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    const P_S32 p_selected_template = (P_S32) p_dialog_msg_process_end->client_handle;
    *p_selected_template = ui_dlg_get_list_idx(p_dialog_msg_process_end->h_dialog, SELECT_TEMPLATE_ID_LIST);

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_select_template)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_select_template_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_select_template_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_select_template_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* oh, the joys of sorting ARRAY_QUICK_BLOCKs...
*
******************************************************************************/

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, template_list_sort, TEMPLATE_LIST_ENTRY)
{
    QSORT_ARG1_VAR_DECL(P_TEMPLATE_LIST_ENTRY, p_template_list_entry_1);
    QSORT_ARG2_VAR_DECL(P_TEMPLATE_LIST_ENTRY, p_template_list_entry_2);

    const S32 res_so = p_template_list_entry_1->sort_order - p_template_list_entry_2->sort_order;

    if(0 == res_so)
    {
        P_QUICK_TBLOCK p_quick_tblock_1 = &p_template_list_entry_1->leafname_quick_tblock;
        P_QUICK_TBLOCK p_quick_tblock_2 = &p_template_list_entry_2->leafname_quick_tblock;

        PCTSTR tstr_1 = (0 != quick_tblock_array_handle_ref(p_quick_tblock_1)) ? array_tstr(&quick_tblock_array_handle_ref(p_quick_tblock_1)) : (PCTSTR) (p_quick_tblock_1 + 1);
        PCTSTR tstr_2 = (0 != quick_tblock_array_handle_ref(p_quick_tblock_2)) ? array_tstr(&quick_tblock_array_handle_ref(p_quick_tblock_2)) : (PCTSTR) (p_quick_tblock_2 + 1);

        int res_cmp = tstr_compare_nocase(tstr_1, tstr_2);

        return(res_cmp);
    }

    return((res_so > 0) ? 1 : -1);
}

_Check_return_
static int
template_sort_order(
    _In_z_      PCTSTR leafname)
{
    const PC_DOCU p_docu_config = p_docu_from_config();
    const PC_ARRAY_HANDLE p_ui_numform_handle = &p_docu_config->numforms;
    const ARRAY_INDEX n_elements = array_elements(p_ui_numform_handle);
    ARRAY_INDEX i;
    const UI_NUMFORM_CLASS ui_numform_class = UI_NUMFORM_CLASS_LOAD_TEMPLATE;

    for(i = 0; i < n_elements; ++i)
    {
        const PC_UI_NUMFORM p_ui_numform = array_ptrc(p_ui_numform_handle, UI_NUMFORM, i);
        PC_USTR numform_leafname;
        PC_A7STR numform_sort_order;

        if(p_ui_numform->numform_class != ui_numform_class)
            continue;

        numform_leafname = p_ui_numform->ustr_numform;
        PTR_ASSERT(numform_leafname);

        if(0 != tstr_compare_nocase(leafname, _tstr_from_ustr(numform_leafname)))
            continue;

        numform_sort_order = (PC_A7STR) p_ui_numform->ustr_opt;

        return(atoi(numform_sort_order));
    }

    return(0);
}

_Check_return_
static STATUS
list_templates_from(
    P_ARRAY_HANDLE p_template_list_handle,
    /*inout*/ P_P_FILE_OBJENUM p_p_file_objenum,
    P_FILE_OBJINFO p_file_objinfo,
    _InVal_     BOOL allow_dirs
    WINDOWS_ONLY_ARG(_InVal_ BOOL fFallback),
    _InoutRef_  P_PIXIT p_max_width)
{
    P_TEMPLATE_LIST_ENTRY p_template_list_entry;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*p_template_list_entry), 1);
    STATUS status = STATUS_OK;

    for(; p_file_objinfo; p_file_objinfo = file_find_next(p_p_file_objenum))
    {
        const BOOL is_file = (file_objinfo_type(p_file_objinfo) == FILE_OBJECT_FILE);

        if( !is_file && !allow_dirs)
            continue;

#if WINDOWS
        /* omit the template that is only present to allow New in an Explorer menu */
        if( !fFallback && (0 == _tcscmp(file_objinfo_name_ll(p_file_objinfo), TEXT("firewrkz.fwt"))) )
            continue;
#endif

        if(NULL != (p_template_list_entry = al_array_extend_by(p_template_list_handle, TEMPLATE_LIST_ENTRY, 1, &array_init_block, &status)))
        {
            quick_tblock_with_buffer_setup(p_template_list_entry->fullname_quick_tblock);
            quick_tblock_with_buffer_setup(p_template_list_entry->leafname_quick_tblock);

            if(status_ok(status = file_objinfo_name(p_file_objinfo, &p_template_list_entry->leafname_quick_tblock)))
            if(status_ok(status = file_objenum_fullname(p_p_file_objenum, p_file_objinfo, &p_template_list_entry->fullname_quick_tblock)))
            {
                PCTSTR tstr = quick_tblock_tstr(&p_template_list_entry->leafname_quick_tblock);
                const PIXIT width = ui_width_from_tstr(tstr);
                *p_max_width = MAX(*p_max_width, width);
                p_template_list_entry->sort_order = template_sort_order(tstr);
            }
        }

        status_break(status);
    }

    return(status);
}

static void
select_a_template_resize_controls(
    _InRef_     PC_UI_TEXT p_ui_text_caption,
    _InRef_     PC_ARRAY_HANDLE p_template_list_handle,
    _InVal_     PIXIT max_width)
{
    /* make appropriate size box */
    const S32 show_elements = array_elements(p_template_list_handle);
    const PIXIT caption_width = ui_width_from_p_ui_text(p_ui_text_caption) + DIALOG_CAPTIONOVH_H;
    const PIXIT buttons_width = DIALOG_DEFOK_H + DIALOG_STDSPACING_H + DIALOG_FATCANCEL_H;
    PIXIT_SIZE list_size;
    DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
    dialog_cmd_ctl_size_estimate.p_dialog_control = &select_template_list;
    dialog_cmd_ctl_size_estimate.p_dialog_control_data.list_text = &stdlisttext_data;
    ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
    dialog_cmd_ctl_size_estimate.size.x += max_width;
    ui_list_size_estimate(show_elements, &list_size);
    dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
    dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
    dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, buttons_width);
    dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, caption_width);
    select_template_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
    select_template_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
}

_Check_return_
static STATUS
select_a_template_dialog(
    _DocuRef_   P_DOCU cur_p_docu,
    _InRef_     PC_UI_TEXT p_ui_text_caption)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup_ui_text(&dialog_cmd_process_dbox, select_template_ctl_create, elemof32(select_template_ctl_create), p_ui_text_caption);
    dialog_cmd_process_dbox.help_topic_resource_id = MSG_DIALOG_SELECT_TEMPLATE_HELP_TOPIC;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_select_template;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) (&g_selected_template);
    return(object_call_DIALOG_with_docu(cur_p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

_Check_return_
extern STATUS
select_a_template(
    _DocuRef_   P_DOCU cur_p_docu,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/,
    /*out*/ P_BOOL p_just_the_one,
    _InVal_     BOOL allow_dirs,
    _InRef_     PC_UI_TEXT p_ui_text_caption)
{
    ARRAY_HANDLE template_list_handle = 0;
    PCTSTR p_template_name;
    PIXIT max_width = DIALOG_SYSCHARSL_H(8); /* arbitrary minimum */
    STATUS status = STATUS_OK;

    select_template_list_source.type = UI_SOURCE_TYPE_NONE;

    {
    P_FILE_OBJENUM p_file_objenum;
    P_FILE_OBJINFO p_file_objinfo;
    TCHARZ wildcard_buffer[8];

    /* Only enumerate Fireworkz templates on Windows as there are loads of other unrelated types present in the All Users & User's Templates directory */
    tstr_xstrkpy(wildcard_buffer, elemof32(wildcard_buffer), RISCOS_OR_WINDOWS(FILE_WILD_MULTIPLE_ALL_TSTR, TEXT("*.fwt")));

    /* Add all from the Templates path, no subdir */
    p_file_objinfo = file_find_first(&p_file_objenum, file_get_templates_path(), wildcard_buffer);

    if(NULL != p_file_objenum)
    {
        status = list_templates_from(&template_list_handle, &p_file_objenum, p_file_objinfo, allow_dirs WINDOWS_ONLY_ARG(FALSE), &max_width);

        file_find_close(&p_file_objenum);
    }

    if(0 == array_elements(&template_list_handle))
    {   /* If none found so far, fallback is to add all from directories on search path, Templates subdir */
        p_file_objinfo = file_find_first_subdir(&p_file_objenum, file_get_search_path(), wildcard_buffer, TEMPLATES_SUBDIR);

        if(NULL != p_file_objenum)
        {
            status = list_templates_from(&template_list_handle, &p_file_objenum, p_file_objinfo, allow_dirs WINDOWS_ONLY_ARG(TRUE), &max_width);

            file_find_close(&p_file_objenum);
        }
    }
    } /*block*/

    /* sort the array we built */
    ui_source_list_fixup_tb(&template_list_handle, offsetof32(TEMPLATE_LIST_ENTRY, fullname_quick_tblock)); /* fixup both the sets of quick blocks prior to sort */
    ui_source_list_fixup_tb(&template_list_handle, offsetof32(TEMPLATE_LIST_ENTRY, leafname_quick_tblock));
    al_array_qsort(&template_list_handle, template_list_sort);
    ui_source_list_fixup_tb(&template_list_handle, offsetof32(TEMPLATE_LIST_ENTRY, fullname_quick_tblock)); /* fixup both the sets of quick blocks after the sort */
    ui_source_list_fixup_tb(&template_list_handle, offsetof32(TEMPLATE_LIST_ENTRY, leafname_quick_tblock));

    p_template_name = NULL;

    *p_just_the_one = (1 == array_elements(&template_list_handle));

    if(*p_just_the_one)
        p_template_name = quick_tblock_tstr(&(array_ptr(&template_list_handle, TEMPLATE_LIST_ENTRY, 0))->fullname_quick_tblock);

    if( status_ok(status) && (NULL == p_template_name) )
        /* make a source of text pointers to these elements for list box processing */
        status = ui_source_create_tb(&template_list_handle, &select_template_list_source, UI_TEXT_TYPE_TSTR_PERM, offsetof32(TEMPLATE_LIST_ENTRY, leafname_quick_tblock));

    if (status_ok(status) && (NULL == p_template_name) )
    {
        if(g_use_default_template)
        {   /* just popup the first one we think of from the sorted list */
            g_use_default_template = false;

            g_selected_template = 0;
        }
        else
        {
            select_a_template_resize_controls(p_ui_text_caption, &template_list_handle, max_width);

            status = select_a_template_dialog(cur_p_docu, p_ui_text_caption);
        }

        if(status_ok(status))
        {   /* selected template to be copied back to caller */
            const S32 selected_template = g_selected_template;

            if(array_index_is_valid(&template_list_handle, selected_template))
                p_template_name = quick_tblock_tstr(&(array_ptr(&template_list_handle, TEMPLATE_LIST_ENTRY, selected_template))->fullname_quick_tblock);
        }
    }

    if(status_ok(status) && (NULL != p_template_name))
        /* selected template gets copied back to caller */
        status = quick_tblock_tchars_add(p_quick_tblock, p_template_name, tstrlen32p1(p_template_name) /*CH_NULL*/);

    {
    ARRAY_INDEX i = array_elements(&template_list_handle);
    while(--i >= 0)
    {
        P_TEMPLATE_LIST_ENTRY p_template_list_entry = array_ptr(&template_list_handle, TEMPLATE_LIST_ENTRY, i);
        quick_tblock_dispose(&p_template_list_entry->fullname_quick_tblock);
    }
    } /*block*/

    ui_lists_dispose_tb(&template_list_handle, &select_template_list_source, offsetof32(TEMPLATE_LIST_ENTRY, leafname_quick_tblock));

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_new_document_intro)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    /* pop up template list or load single template */
    return(load_this_template_file_rl(p_docu, NULL));
}

T5_CMD_PROTO(extern, t5_cmd_new_document_default)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    g_use_default_template = true; /* one-shot */

    return(load_this_template_file_rl(P_DOCU_NONE, NULL));
}

/* end of ui_load.c */
