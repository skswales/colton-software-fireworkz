/* ui_style.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Style editor UI for Fireworkz */

/* SKS December 1993 */

#include "common/gflags.h"

#include "ob_skspt/ui_styin.h"

#if RISCOS
#include "ob_dlg/xp_dlgr.h"
#endif

/*
internal routines
*/

_Check_return_
static STATUS
new_style(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR tstr_style_name);

_Check_return_
static STATUS
style_apply_struct(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     PC_STYLE p_style);

#define MUTEX_AVAILABLE   0x12349876U
#define MUTEX_UNAVAILABLE 0x9ABC0FEDU

static U32
es_process_interlock;

static U32
style_intro_interlock;

static void
base_style_on_handles(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_STYLE p_style,
    _OutRef_    P_STYLE_SELECTOR p_style_selector,
    _InVal_     STYLE_HANDLE style_handle_1,
    _InVal_     STYLE_HANDLE style_handle_2)
{
    STYLE based_style;
    STYLE_SELECTOR selector;

    style_init(&based_style);

    style_struct_from_handle(p_docu, &based_style, style_handle_1, &style_selector_all);

    /* all bits are set for edit that are present in this first style */
    style_selector_copy(p_style_selector, &based_style.selector);

    /* search for these other bits */
    void_style_selector_not(&selector, p_style_selector);

    /* decide where to get rest of defaults from */
    if(style_handle_2 == -2)
    {
        SELECTION_STYLE selection_style;

        /* enquire about just these bits of state */
        style_selector_copy(&selection_style.selector_in, &selector);

        style_selector_clear(&selection_style.selector_fuzzy_out);

        style_init(&selection_style.style_out);

        /* ignore transient styles */
        style_region_class_limit_set(p_docu, REGION_UPPER);
        /* may return STATUS_FAIL but we've had to take care of style & selectors anyway */
        status_consume(object_skel(p_docu, T5_MSG_SELECTION_STYLE, &selection_style));
        style_region_class_limit_set(p_docu, REGION_END);

        style_copy(&based_style, &selection_style.style_out, &selection_style.style_out.selector);
    }
    else if(style_handle_2)
        style_struct_from_handle(p_docu, &based_style, style_handle_2, &selector);

    /* copy from default all those bits which aren't yet set */
    style_copy_defaults(p_docu, &based_style, &style_selector_all);

    style_init(p_style);

    status_assert(style_duplicate(p_style, &based_style, &based_style.selector));

    style_dispose(&based_style);
}

/*
remove style uses query box
*/

#define REMOVE_STYLE_USE_QUERY_BUTTON_H     (DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARS_H("Cancel"))
#define REMOVE_STYLE_USE_QUERY_BUTTON_GAP_H ((REMOVE_STYLE_USE_QUERY_ACROSS_H - 3 * REMOVE_STYLE_USE_QUERY_BUTTON_H) / 2)

enum REMOVE_STYLE_USE_QUERY_CONTROL_IDS
{
    REMOVE_STYLE_USE_QUERY_ID_TEXT_1 = 30
};

static const DIALOG_CONTROL
remove_style_use_query_text_1 =
{
    REMOVE_STYLE_USE_QUERY_ID_TEXT_1, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDTEXT_V },
    { DRT(LTLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
remove_style_use_query_text_1_data = { UI_TEXT_INIT_RESID(SKEL_SPLIT_MSG_DIALOG_REMOVE_STYLE_USE_QUERY), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
remove_style_use_query_ok_data = { { DIALOG_COMPLETION_OK }, UI_TEXT_INIT_RESID(MSG_OK) };

static const DIALOG_CTL_CREATE
remove_style_use_query_ctl_create[] =
{
    { &dialog_main_group },

    { &remove_style_use_query_text_1, &remove_style_use_query_text_1_data },

    { &defbutton_ok, &remove_style_use_query_ok_data},
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

_Check_return_
static STATUS
remove_style_use_query(
    _DocuRef_   P_DOCU p_docu)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, remove_style_use_query_ctl_create, elemof32(remove_style_use_query_ctl_create), SKEL_SPLIT_MSG_DIALOG_REMOVE_STYLE_USE_QUERY_HELP_TOPIC);
    dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_TSTR_PERM;
    dialog_cmd_process_dbox.caption.text.tstr = product_ui_id();
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

/******************************************************************************
*
* style intro dialog
*
******************************************************************************/

enum STYLE_INTRO_CONTROL_IDS
{
#define STYLE_INTRO_MINLIST_H (12 * PIXITS_PER_INCH / 8)
#define STYLE_INTRO_MINLIST_V (DIALOG_DEFPUSHBUTTON_V + 4 * (DIALOG_STDPUSHBUTTON_V +  DIALOG_STDSPACING_V))

#define DIALOG_BIGSPACING_V (2 * DIALOG_STDSPACING_V)
    STYLE_INTRO_ID_CHANGE = IDOK,
    STYLE_INTRO_ID_APPLY = 164,
    STYLE_INTRO_ID_APPLY_ADJUST, /*logical*/
    STYLE_INTRO_ID_DELETE,
    STYLE_INTRO_ID_DELETE_ADJUST, /*logical*/
    STYLE_INTRO_ID_NEW,
    STYLE_INTRO_ID_REMOVE_USE,

#if WINDOWS
#define STYLE_INTRO_BUTTONS_H DIALOG_STDPUSHBUTTON_H
#else
#define STYLE_INTRO_BUTTONS_H (DIALOG_PUSHBUTTONOVH_H + MAX(DIALOG_SYSCHARS_H("Change"),DIALOG_SYSCHARS_H("Wechseln")))
#endif

    STYLE_INTRO_ID_LIST
};

static /*poked*/ DIALOG_CONTROL
style_intro_list =
{
    STYLE_INTRO_ID_LIST, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF },
    { 0, 0, 0/*poked*/, 0/*poked*/ },
    { DRT(LTLT, LIST_TEXT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
style_intro_change =
{
    STYLE_INTRO_ID_CHANGE, DIALOG_MAIN_GROUP,
    { STYLE_INTRO_ID_LIST, STYLE_INTRO_ID_LIST },
    { DIALOG_STDSPACING_H, 0, STYLE_INTRO_BUTTONS_H, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RTLT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
style_intro_change_data = { { STYLE_INTRO_ID_CHANGE }, UI_TEXT_INIT_RESID(MSG_CHANGE) };

static const DIALOG_CONTROL
style_intro_apply =
{
    STYLE_INTRO_ID_APPLY, DIALOG_MAIN_GROUP,
    { STYLE_INTRO_ID_CHANGE, STYLE_INTRO_ID_CHANGE, STYLE_INTRO_ID_CHANGE },
    { 0, DIALOG_BIGSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTONR
style_intro_apply_data = { { STYLE_INTRO_ID_APPLY, 0, 0, 1 /*alternate_right*/ }, UI_TEXT_INIT_RESID(MSG_APPLY), STYLE_INTRO_ID_APPLY_ADJUST };

static const DIALOG_CONTROL
style_intro_delete =
{
    STYLE_INTRO_ID_DELETE, DIALOG_MAIN_GROUP,
    { STYLE_INTRO_ID_APPLY, STYLE_INTRO_ID_APPLY, STYLE_INTRO_ID_APPLY },
    { 0, DIALOG_BIGSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTONR
style_intro_delete_data = { { STYLE_INTRO_ID_DELETE, 0, 0, 1 /*alternate_right*/ }, UI_TEXT_INIT_RESID(MSG_DELETE), STYLE_INTRO_ID_DELETE_ADJUST };

static const DIALOG_CONTROL
style_intro_new =
{
    STYLE_INTRO_ID_NEW, DIALOG_MAIN_GROUP,
    { STYLE_INTRO_ID_DELETE, STYLE_INTRO_ID_DELETE, STYLE_INTRO_ID_DELETE },
    { 0, DIALOG_BIGSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
style_intro_new_data = { { STYLE_INTRO_ID_NEW }, UI_TEXT_INIT_RESID(MSG_NEW) };

static const DIALOG_CONTROL
style_intro_cancel =
{
    IDCANCEL, DIALOG_MAIN_GROUP,
    { STYLE_INTRO_ID_NEW, STYLE_INTRO_ID_NEW, STYLE_INTRO_ID_NEW },
    { 0, DIALOG_BIGSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CTL_CREATE
style_intro_ctl_create[] =
{
    { &dialog_main_group },

    { &style_intro_list,   &stdlisttext_data },

    { &style_intro_change, &style_intro_change_data },
    { &style_intro_apply,  &style_intro_apply_data },
    { &style_intro_delete, &style_intro_delete_data },
    { &style_intro_new,    &style_intro_new_data },
    { &style_intro_cancel, &stdbutton_cancel_data }
};

/*
always interactive - put up a dialog with the list of styles
*/

typedef struct STYLE_INTRO_CALLBACK
{
    UI_SOURCE ui_source;
    UI_TEXT ui_text;
    S32 selected_item;
}
STYLE_INTRO_CALLBACK, * P_STYLE_INTRO_CALLBACK;

_Check_return_
static STATUS
dialog_style_intro_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    const P_STYLE_INTRO_CALLBACK p_style_intro_callback = (P_STYLE_INTRO_CALLBACK) p_dialog_msg_ctl_fill_source->client_handle;

    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case STYLE_INTRO_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &p_style_intro_callback->ui_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_style_intro_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    const P_STYLE_INTRO_CALLBACK p_style_intro_callback = (P_STYLE_INTRO_CALLBACK) p_dialog_msg_ctl_create_state->client_handle;

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case STYLE_INTRO_ID_LIST:
        {
        if(!ui_text_is_blank(&p_style_intro_callback->ui_text))
            p_dialog_msg_ctl_create_state->state_set.state.list_text.ui_text = p_style_intro_callback->ui_text;
        else
        {
            p_style_intro_callback->selected_item = -1;

            p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = DIALOG_CTL_STATE_LIST_ITEM_NONE;
            p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALTERNATE;
        }

        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_style_intro_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_STYLE_INTRO_CALLBACK p_style_intro_callback = (P_STYLE_INTRO_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case STYLE_INTRO_ID_LIST:
        p_style_intro_callback->selected_item = p_dialog_msg_ctl_state_change->new_state.list_text.itemno;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_style_intro)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_style_intro_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_style_intro_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_style_intro_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(extern, t5_cmd_style_intro)
{
    STATUS status;
    BOOL looping_outer;
    STYLE_INTRO_CALLBACK style_intro_callback;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    assert((style_intro_interlock == MUTEX_AVAILABLE) || (style_intro_interlock == MUTEX_UNAVAILABLE));

    if(style_intro_interlock == MUTEX_UNAVAILABLE)
    {
        host_bleep();
        return(STATUS_OK);
    }

    zero_struct(style_intro_callback);
    style_intro_callback.selected_item = -1;

    do  {
        PCTSTR tstr_style_name = tstr_empty_string; /* keep Code Analysis happy */
        STYLE_HANDLE style_handle = STYLE_HANDLE_NONE; /* keep dataflower happy */
        ARRAY_HANDLE style_intro_list_handle;
        PIXIT max_width;

        looping_outer = 0;

        status_return(ui_list_create_style(p_docu, &style_intro_list_handle, &style_intro_callback.ui_source, &max_width));

        { /* make appropriate size box */
        PIXIT_SIZE list_size;
        DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
        dialog_cmd_ctl_size_estimate.p_dialog_control = &style_intro_list;
        dialog_cmd_ctl_size_estimate.p_dialog_control_data = &stdlisttext_data;
        ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
        dialog_cmd_ctl_size_estimate.size.x += max_width;
        ui_list_size_estimate(array_elements(&style_intro_list_handle), &list_size);
        dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
        dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
        dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, STYLE_INTRO_MINLIST_H);
        style_intro_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
        style_intro_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
        } /*block*/

        for(;;) /* loop for structure */
        {
            /* lookup 'current' style name and suggest that */
            style_name_from_marked_area(p_docu, &style_intro_callback.ui_text);

            {
            DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
            dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, style_intro_ctl_create, elemof32(style_intro_ctl_create), SKEL_SPLIT_MSG_DIALOG_STYLE_HELP_TOPIC);
            /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
            dialog_cmd_process_dbox.caption.text.resource_id = SKEL_SPLIT_MSG_DIALOG_STYLE_CAPTION;
            dialog_cmd_process_dbox.bits.note_position = 1;
            dialog_cmd_process_dbox.p_proc_client = dialog_event_style_intro;
            dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &style_intro_callback;
            style_intro_interlock = MUTEX_UNAVAILABLE;
            status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
            style_intro_interlock = MUTEX_AVAILABLE;
            } /*block*/

            if(status_ok(status))
            {
                if(style_intro_callback.selected_item < 0)
                {
                    if(status != STYLE_INTRO_ID_NEW)
                    {
                        host_bleep();
                        continue;
                    }
                }
                else
                {
                    assert(array_index_is_valid(&style_intro_list_handle, style_intro_callback.selected_item));

                    /* NB. it's the UI list that's sorted, so get name from there! */
                    tstr_style_name = ui_text_tstr(array_ptr(&style_intro_callback.ui_source.source.array_handle, UI_TEXT, style_intro_callback.selected_item));
                    style_handle = style_handle_from_name(p_docu, tstr_style_name);

                    if(0 == style_handle)
                        status = create_error(ERR_NAMED_STYLE_NOT_FOUND);
                }
            }

            switch(status)
            {
            default:
                break;

            case STYLE_INTRO_ID_APPLY_ADJUST:
                looping_outer = 1;

                /*FALLTHRU*/

            case STYLE_INTRO_ID_APPLY:
                {
                STYLE style;

                style_init(&style);
                style_bit_set(&style, STYLE_SW_HANDLE);

                style.style_handle = style_handle;

                status_assert(status = style_apply_struct(p_docu, T5_CMD_STYLE_APPLY, &style));

                break;
                }

            /* >>>MRJC */
            case STYLE_INTRO_ID_REMOVE_USE:
                {
                ARRAY_INDEX style_docu_area_ix;
                if(0 <= (style_docu_area_ix = style_handle_use_find(p_docu, style_handle, &p_docu->cur, &p_docu->h_style_docu_area)))
                    style_docu_area_remove(p_docu, &p_docu->h_style_docu_area, style_docu_area_ix);
                break;
                }

            case STYLE_INTRO_ID_CHANGE:
                {
                BOOL looping;

                do  {
                    STYLE style_in;
                    STYLE_SELECTOR style_selector;
                    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 32);
                    quick_tblock_with_buffer_setup(quick_tblock);

                    /* start editing a duplicate of this style (because of paranoia about handles being deleted behind our backs) */
                    base_style_on_handles(p_docu, &style_in, &style_selector, style_handle, 0);

                    if(status_ok(status = quick_tblock_printf(&quick_tblock, resource_lookup_tstr(SKEL_SPLIT_MSG_DIALOG_EDITING_STYLE), array_tstr(&style_in.h_style_name_tstr)))
                    && status_ok(status = quick_tblock_nullch_add(&quick_tblock)))
                    {
                        UI_TEXT ui_text;
                        STYLE_SELECTOR style_modified;
                        STYLE_SELECTOR style_selector_modified;
                        STYLE_SELECTOR prohibited_enabler;
                        STYLE_SELECTOR prohibited_enabler_2;
                        STYLE style_out;

                        ui_text.type = UI_TEXT_TYPE_TSTR_TEMP;
                        ui_text.text.tstr = quick_tblock_tstr(&quick_tblock);

                        /* don't let the buggers turn this off! */
                        style_selector_clear(&prohibited_enabler);
                        style_selector_bit_set(&prohibited_enabler, STYLE_SW_NAME);

                        style_selector_clear(&prohibited_enabler_2);
                        /* SKS 16aug93 - don't let people turn things off that are present in the base style */
                        if(style_handle == style_handle_base(&p_docu->h_style_docu_area))
                            style_selector_copy(&prohibited_enabler_2, &style_in.selector);

                        {
                        MSG_UISTYLE_STYLE_EDIT msg_uistyle_style_edit;
                        zero_struct(msg_uistyle_style_edit);
                        msg_uistyle_style_edit.p_caption = &ui_text;
                        msg_uistyle_style_edit.p_style_in = &style_in;
                        msg_uistyle_style_edit.p_style_selector = &style_selector;
                        msg_uistyle_style_edit.p_style_modified = &style_modified;
                        msg_uistyle_style_edit.p_style_selector_modified = &style_selector_modified;
                        msg_uistyle_style_edit.p_prohibited_enabler = &prohibited_enabler;
                        msg_uistyle_style_edit.p_prohibited_enabler_2 = &prohibited_enabler_2;
                        msg_uistyle_style_edit.p_style_out = &style_out;
                        msg_uistyle_style_edit.style_handle_being_modified = style_handle;
                        msg_uistyle_style_edit.subdialog = 1;
                        status = object_call_id(OBJECT_ID_SKEL_SPLIT, p_docu, T5_MSG_UISTYLE_STYLE_EDIT, &msg_uistyle_style_edit);
                        } /*block*/

                        looping = (status == ES_ID_ADJUST_APPLY);

                        if(status > STATUS_OK) /* Cancel -> STATUS_OK */
                        {
                            if(0 != (style_handle = style_handle_from_name(p_docu, tstr_style_name)))
                            {
                                style_handle_modify(p_docu, style_handle, &style_out, &style_modified, &style_selector_modified);
                                docu_modify(p_docu);
                            }
                        }

                        style_free_resources_all(&style_out);
                        style_free_resources_all(&style_in);
                    }
                    else
                        looping = FALSE;

                    quick_tblock_dispose(&quick_tblock);
                }
                while(looping);

                break;
                }

            case STYLE_INTRO_ID_DELETE_ADJUST:
                looping_outer = 1;

                /*FALLTHRU*/

            case STYLE_INTRO_ID_DELETE:
                {
                STYLE_USE_QUERY style_use_query;
                P_STYLE_USE_QUERY_ENTRY p_style_use_query_entry;
                SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_style_use_query_entry), FALSE);

                style_use_query.docu_area.whole_col = 1;
                style_use_query.docu_area.whole_row = 1;
                style_use_query.h_style_use = 0;
                style_use_query.data_class = DATA_SAVE_WHOLE_DOC;
                style_use_query.region_class = REGION_END;

                if(NULL == (p_style_use_query_entry = al_array_extend_by(&style_use_query.h_style_use, STYLE_USE_QUERY_ENTRY, 1, &array_init_block, &status)))
                    break;

                if(style_handle == style_handle_base(&p_docu->h_style_docu_area))
                {
                    status = create_error(SKEL_SPLIT_ERR_CANT_DELETE_BASE_STYLE);
                    break;
                }

                p_style_use_query_entry->style_handle = style_handle;
                p_style_use_query_entry->use = 0;

                status_assert(maeve_event(p_docu, T5_MSG_STYLE_USE_QUERY, &style_use_query));

                p_style_use_query_entry = array_ptr(&style_use_query.h_style_use, STYLE_USE_QUERY_ENTRY, 0);

                {
                BOOL remove_style = TRUE;

                if(p_style_use_query_entry->use)
                {
                    /* option to remove style uses */
                    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

                    status = remove_style_use_query(p_docu);

                    switch(status)
                    {
                    case DIALOG_COMPLETION_OK:
                        {
                        STYLE_USE_REMOVE style_use_remove;
                        style_use_remove.style_handle = style_handle;
                        style_use_remove.row = n_rows(p_docu);
                        status_assert(maeve_event(p_docu, T5_MSG_STYLE_USE_REMOVE, &style_use_remove));
                        remove_style = TRUE;

                        {
                        DOCU_REFORMAT docu_reformat;
                        docu_reformat.data.row = style_use_remove.row;
                        docu_reformat.action = REFORMAT_XY;
                        docu_reformat.data_type = REFORMAT_ROW;
                        docu_reformat.data_space = DATA_NONE;
                        status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
                        } /*block*/

                        break;
                        }

                    default:
                        break;
                    }
                }
                else
                    remove_style = TRUE;

                al_array_dispose(&style_use_query.h_style_use); /* SKS 1.03 17mar93 */

                if(status_ok(status) && remove_style)
                {
                    style_handle_remove(p_docu, style_handle);
                    docu_modify(p_docu);
                }

                if(status_fail(status))
                    reperr_null(status);
                } /*block*/

                break;
                }

            case STYLE_INTRO_ID_NEW:
                status = new_style(p_docu, tstr_style_name);
                break;
            }

            break; /* end of loop for structure */
        }

        if(status_fail(status))
            looping_outer = 0;

        ui_list_dispose_style(&style_intro_list_handle, &style_intro_callback.ui_source);
    }
    while(looping_outer);

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_effects_button)
{
    /* always interactive - go straight to style editor */
    S32 subdialog = -1;
    STATUS status;
    BOOL looping;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {   /* specified subdialog? */
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
        if(arg_is_present(p_args, 0))
            subdialog = p_args[0].val.s32;
    }

    assert((es_process_interlock == MUTEX_AVAILABLE) || (es_process_interlock == MUTEX_UNAVAILABLE));

    if(es_process_interlock == MUTEX_UNAVAILABLE)
    {
        host_bleep();
        return(STATUS_OK);
    }

    do  {
        STYLE style_in;
        STYLE_SELECTOR style_selector;

        style_init(&style_in);

        /* initially no effects are on at all */
        style_selector_clear(&style_selector);

        {
        SELECTION_STYLE selection_style;

        /* enquire about all editable bits of state */
        selection_style.selector_in = style_selector_all;
        style_selector_bit_clear(&selection_style.selector_in, STYLE_SW_HANDLE);

        style_selector_clear(&selection_style.selector_fuzzy_out);

        style_init(&selection_style.style_out);

        /* ignore transient styles */
        style_region_class_limit_set(p_docu, REGION_UPPER);
        status_consume(object_skel(p_docu, T5_MSG_SELECTION_STYLE, &selection_style));
        style_region_class_limit_set(p_docu, REGION_END);

        style_copy_defaults(p_docu, &selection_style.style_out, &style_selector_all);

        status_assert(style_duplicate(&style_in, &selection_style.style_out, &selection_style.style_out.selector));
        } /*block*/

        {
        UI_TEXT ui_text;
        STYLE_SELECTOR prohibited_enabler;
        STYLE style_out;

        ui_text.type = UI_TEXT_TYPE_RESID;
        ui_text.text.resource_id = SKEL_SPLIT_MSG_DIALOG_APPLY_EFFECTS;

        /* don't let the buggers turn this on! */
        style_selector_clear(&prohibited_enabler);
        style_selector_bit_set(&prohibited_enabler, STYLE_SW_KEY);

        {
        MSG_UISTYLE_STYLE_EDIT msg_uistyle_style_edit;
        zero_struct(msg_uistyle_style_edit);
        msg_uistyle_style_edit.p_caption = &ui_text;
        msg_uistyle_style_edit.p_style_in = &style_in;
        msg_uistyle_style_edit.p_style_selector = &style_selector;
        msg_uistyle_style_edit.p_style_modified = NULL;
        msg_uistyle_style_edit.p_style_selector_modified = NULL;
        msg_uistyle_style_edit.p_prohibited_enabler = &prohibited_enabler;
        msg_uistyle_style_edit.p_prohibited_enabler_2 = NULL;
        msg_uistyle_style_edit.p_style_out = &style_out;
        msg_uistyle_style_edit.style_handle_being_modified = 0;
        msg_uistyle_style_edit.subdialog = subdialog;
        status = object_call_id(OBJECT_ID_SKEL_SPLIT, p_docu, T5_MSG_UISTYLE_STYLE_EDIT, &msg_uistyle_style_edit);
        } /*block*/

        looping = (status == ES_ID_ADJUST_APPLY);

        if(status > STATUS_OK) /* Cancel -> STATUS_OK */
            status_assert(style_apply_struct(p_docu, T5_CMD_STYLE_APPLY, &style_out));

        style_free_resources_all(&style_out);
        } /*block*/

        style_free_resources_all(&style_in);
    }
    while(looping);

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    return(status);
}

/******************************************************************************
*
* define a new style based on another
* then fire off a style editor on this new style
*
******************************************************************************/

enum NEW_STYLE_CONTROL_IDS
{
    NEW_STYLE_ID_NAME_ORNAMENT = 64,
    NEW_STYLE_ID_NAME,
    NEW_STYLE_ID_BASED_ON,
    NEW_STYLE_ID_USE_GROUP,
    NEW_STYLE_ID_USE_SELECTION,
    NEW_STYLE_ID_USE_STYLE,
    NEW_STYLE_ID_LIST
};

#define NEW_STYLE_USE_STYLE     0
#define NEW_STYLE_USE_SELECTION 1

static const DIALOG_CONTROL
new_style_name_ornament =
{
    NEW_STYLE_ID_NAME_ORNAMENT, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, (3 * PIXITS_PER_INCH) / 2, DIALOG_STDTEXT_V },
    { DRT(LTLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
new_style_name_ornament_data = { UI_TEXT_INIT_RESID(SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_NAME), { 1 /*left_text*/} };

static const DIALOG_CONTROL
new_style_name =
{
    NEW_STYLE_ID_NAME, DIALOG_MAIN_GROUP,
    { NEW_STYLE_ID_NAME_ORNAMENT, NEW_STYLE_ID_NAME_ORNAMENT, NEW_STYLE_ID_LIST },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V },
    { DRT(LBRT, EDIT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_EDIT
new_style_name_data = { { { FRAMED_BOX_EDIT }, NULL }, /* EDIT_XX */ UI_TEXT_INIT_RESID(SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_SUGGESTED) /* state */ };

static const DIALOG_CONTROL
new_style_based_on =
{
    NEW_STYLE_ID_BASED_ON, DIALOG_MAIN_GROUP,
    { NEW_STYLE_ID_NAME_ORNAMENT, NEW_STYLE_ID_NAME, NEW_STYLE_ID_NAME_ORNAMENT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDTEXT_V },
    { DRT(LBRT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
new_style_based_on_data = { UI_TEXT_INIT_RESID(SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_BASED_ON), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
new_style_use_group =
{
    NEW_STYLE_ID_USE_GROUP, DIALOG_MAIN_GROUP,
    { NEW_STYLE_ID_BASED_ON, NEW_STYLE_ID_BASED_ON, NEW_STYLE_ID_USE_SELECTION, NEW_STYLE_ID_USE_SELECTION },
    { 0, DIALOG_STDSPACING_V, 0, 0 },
    { DRT(LBLT, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
new_style_use_style =
{
    NEW_STYLE_ID_USE_STYLE, NEW_STYLE_ID_USE_GROUP,
    { NEW_STYLE_ID_BASED_ON, NEW_STYLE_ID_BASED_ON },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
new_style_use_style_data = { { 0 }, NEW_STYLE_USE_STYLE, UI_TEXT_INIT_RESID(SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_STYLE) };

static const DIALOG_CONTROL
new_style_use_selection =
{
    NEW_STYLE_ID_USE_SELECTION, NEW_STYLE_ID_USE_GROUP,
    { NEW_STYLE_ID_USE_STYLE, NEW_STYLE_ID_USE_STYLE },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(RTLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
new_style_use_selection_data = { { 0 }, NEW_STYLE_USE_SELECTION, UI_TEXT_INIT_RESID(MSG_SELECTION) };

static /*poked*/ DIALOG_CONTROL
new_style_list =
{
    NEW_STYLE_ID_LIST, DIALOG_MAIN_GROUP,
    { NEW_STYLE_ID_USE_STYLE, NEW_STYLE_ID_USE_STYLE },
    { 0, DIALOG_STDSPACING_V },
    { DRT(LBLT, LIST_TEXT), 1 /*tabstop*/ }
};

#define NEW_STYLE_MINLIST_H (16 * PIXITS_PER_INCH / 8)

static const DIALOG_CTL_CREATE
new_style_ctl_create[] =
{
    { &dialog_main_group },

    { &new_style_name_ornament, &new_style_name_ornament_data },
    { &new_style_name,          &new_style_name_data },
    { &new_style_based_on,      &new_style_based_on_data },
    { &new_style_use_group,     NULL },
    { &new_style_use_style,     &new_style_use_style_data },
    { &new_style_use_selection, &new_style_use_selection_data },
    { &new_style_list,          &stdlisttext_data_dd },

    { &defbutton_ok,            &defbutton_ok_data },
    { &stdbutton_cancel,        &stdbutton_cancel_data }
};

typedef struct NEW_STYLE_CALLBACK
{
    PCTSTR tstr_style_name; /*IN*/
    UI_SOURCE ui_source;
    S32 selected_item;
    UI_TEXT ui_text_style_new; /*OUT*/
}
NEW_STYLE_CALLBACK, * P_NEW_STYLE_CALLBACK;

_Check_return_
static STATUS
dialog_new_style_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    const P_NEW_STYLE_CALLBACK p_new_style_callback = (P_NEW_STYLE_CALLBACK) p_dialog_msg_ctl_fill_source->client_handle;

    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case NEW_STYLE_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &p_new_style_callback->ui_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_new_style_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const DIALOG_CONTROL_ID dialog_control_id = p_dialog_msg_ctl_state_change->dialog_control_id;

    switch(dialog_control_id)
    {
    case NEW_STYLE_ID_USE_GROUP:
        /* enable style list according to possibility of use */
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, NEW_STYLE_ID_LIST, (p_dialog_msg_ctl_state_change->new_state.radiobutton == NEW_STYLE_USE_STYLE));
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_new_style_process_start(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const H_DIALOG h_dialog = p_dialog_msg_process_start->h_dialog;
    const P_NEW_STYLE_CALLBACK p_new_style_callback = (P_NEW_STYLE_CALLBACK) p_dialog_msg_process_start->client_handle;

    { /* suggest a style name not in use at the moment in this document */
    S32 i;
    STATUS status = STATUS_OK;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 32);
    quick_tblock_with_buffer_setup(quick_tblock);

    for(i = 1; ; ++i)
    {
        UI_TEXT ui_text;
        status_break(status = quick_tblock_printf(&quick_tblock, resource_lookup_tstr(SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_SUGGESTED), i));
        status_break(status = quick_tblock_nullch_add(&quick_tblock));
        ui_text.type = UI_TEXT_TYPE_TSTR_TEMP;
        ui_text.text.tstr = quick_tblock_tstr(&quick_tblock);
        if(0 == style_handle_from_name(p_docu, ui_text.text.tstr))
        {
            status = ui_dlg_set_edit(h_dialog, NEW_STYLE_ID_NAME, &ui_text);
            break;
        }
        quick_tblock_dispose(&quick_tblock); /* reset */
    }

    quick_tblock_dispose(&quick_tblock);

    status_return(status);
    } /*block*/

    { /* lookup 'current' style name and suggest that he bases the new style on that */
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = h_dialog;
    dialog_cmd_ctl_state_set.dialog_control_id = NEW_STYLE_ID_LIST;
    dialog_cmd_ctl_state_set.bits = 0;

    if(p_new_style_callback->tstr_style_name && *p_new_style_callback->tstr_style_name)
    {
        dialog_cmd_ctl_state_set.state.list_text.ui_text.type      = UI_TEXT_TYPE_TSTR_TEMP;
        dialog_cmd_ctl_state_set.state.list_text.ui_text.text.tstr = p_new_style_callback->tstr_style_name;
    }
    else
    {
        style_name_from_marked_area(p_docu, &dialog_cmd_ctl_state_set.state.list_text.ui_text);

        if(ui_text_is_blank(&dialog_cmd_ctl_state_set.state.list_text.ui_text))
        {
            dialog_cmd_ctl_state_set.state.list_text.itemno = DIALOG_CTL_STATE_LIST_ITEM_NONE;
            dialog_cmd_ctl_state_set.bits |= DIALOG_STATE_SET_ALTERNATE;
        }
    }

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
    } /*block*/

    { /* if a suitable selection exists, allow that to be used as well as a named style */
    MARK_INFO mark_info;
    BOOL any_selection;
    BOOL texty_selection;

    if(status_fail(object_skel(p_docu, T5_MSG_MARK_INFO_READ, &mark_info)))
        mark_info.h_markers = 0;

    any_selection = (mark_info.h_markers != 0);

    texty_selection = any_selection &&
                      ((OBJECT_ID_CELLS == p_docu->focus_owner)  ||
                       (OBJECT_ID_HEADER == p_docu->focus_owner) ||
                       (OBJECT_ID_FOOTER == p_docu->focus_owner) );

    ui_dlg_ctl_enable(h_dialog, NEW_STYLE_ID_USE_SELECTION, texty_selection);
    } /*block*/

    status_return(ui_dlg_set_radio(h_dialog, NEW_STYLE_ID_USE_GROUP, NEW_STYLE_USE_STYLE));

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_new_style_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_NEW_STYLE_CALLBACK p_new_style_callback = (P_NEW_STYLE_CALLBACK) p_dialog_msg_process_end->client_handle;

        /* note name of new named style to edit */
        ui_dlg_get_edit(p_dialog_msg_process_end->h_dialog, NEW_STYLE_ID_NAME, &p_new_style_callback->ui_text_style_new);

        /* which style are we basing the new style's defaults on, or is it the selection? */
        if(ui_dlg_get_radio(p_dialog_msg_process_end->h_dialog, NEW_STYLE_ID_USE_GROUP) == NEW_STYLE_USE_SELECTION)
            p_new_style_callback->selected_item = -2;
        else
        {
            p_new_style_callback->selected_item = ui_dlg_get_list_idx(p_dialog_msg_process_end->h_dialog, NEW_STYLE_ID_LIST);

            if(p_new_style_callback->selected_item < 0)
                p_new_style_callback->selected_item = -1;
        }
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_new_style)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_new_style_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_new_style_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_new_style_process_start(p_docu, (PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_new_style_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
new_style_do(
    _DocuRef_   P_DOCU p_docu,
    P_NEW_STYLE_CALLBACK p_new_style_callback,
    _InVal_     BOOL texty_selection)
{
    STATUS status;
    STYLE_HANDLE style_handle, style_handle_2;
    BOOL looping;

    { /* create an empty style with this name */
    STYLE style;
    style_init(&style);
    status = al_tstr_set(&style.h_style_name_tstr, ui_text_tstr(&p_new_style_callback->ui_text_style_new));
    style_bit_set(&style, STYLE_SW_NAME);
    status_return(status = style_handle_add(p_docu, &style));
    style_handle = (STYLE_HANDLE) status;
    docu_modify(p_docu);
    } /*block*/

    if(p_new_style_callback->selected_item == -2)
        style_handle_2 = -2;
    else if(p_new_style_callback->selected_item == -1)
        style_handle_2 = 0;
    else
        /* read named style info now as we've spent a long time in dialog and handles may have changed etc. */
        /* NB. it's the UI list that's sorted! */
        style_handle_2 = style_handle_from_name(p_docu, ui_text_tstr(array_ptr(&p_new_style_callback->ui_source.source.array_handle, UI_TEXT, p_new_style_callback->selected_item)));

    do  {
        STYLE style_in;
        STYLE_SELECTOR style_selector;
        STYLE style_out;
        STYLE_SELECTOR style_modified;
        STYLE_SELECTOR style_selector_modified;
        QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 32);
        quick_tblock_with_buffer_setup(quick_tblock);

        base_style_on_handles(p_docu, &style_in, &style_selector, style_handle, style_handle_2);

        if(status_ok(status = quick_tblock_printf(&quick_tblock, resource_lookup_tstr(SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_DEFINING), array_tstr(&style_in.h_style_name_tstr)))
        && status_ok(status = quick_tblock_nullch_add(&quick_tblock)))
        {
            UI_TEXT ui_text;

            ui_text.type = UI_TEXT_TYPE_TSTR_TEMP;
            ui_text.text.tstr = quick_tblock_tstr(&quick_tblock);

            {
            MSG_UISTYLE_STYLE_EDIT msg_uistyle_style_edit;
            zero_struct(msg_uistyle_style_edit);
            msg_uistyle_style_edit.p_caption = &ui_text;
            msg_uistyle_style_edit.p_style_in = &style_in;
            msg_uistyle_style_edit.p_style_selector = &style_selector;
            msg_uistyle_style_edit.p_style_modified = &style_modified;
            msg_uistyle_style_edit.p_style_selector_modified = &style_selector_modified;
            msg_uistyle_style_edit.p_prohibited_enabler = NULL;
            msg_uistyle_style_edit.p_prohibited_enabler_2 = NULL;
            msg_uistyle_style_edit.p_style_out = &style_out;
            msg_uistyle_style_edit.style_handle_being_modified = style_handle;
            msg_uistyle_style_edit.subdialog = 1;
            status = object_call_id(OBJECT_ID_SKEL_SPLIT, p_docu, T5_MSG_UISTYLE_STYLE_EDIT, &msg_uistyle_style_edit);
            } /*block*/

            looping = (status == ES_ID_ADJUST_APPLY);

            if(status > STATUS_OK) /* Cancel -> STATUS_OK */
            {
                /* it is assumed that style_handle is still kosher even if the punter
                 * has renamed the style etc. as he won't have deleted it
                */
                style_handle_modify(p_docu, style_handle, &style_out, &style_modified, &style_selector_modified);
                docu_modify(p_docu);

                if(texty_selection)
                {
                    STYLE named_style;

                    style_init(&named_style);

                    named_style.style_handle = style_handle;
                    style_bit_set(&named_style, STYLE_SW_HANDLE);

                    if(status_fail(status = style_apply_struct(p_docu, T5_CMD_STYLE_APPLY, &named_style)))
                        looping = 0;
                }
            }

            style_free_resources_all(&style_out);
            style_free_resources_all(&style_in);
        }
        else
            looping = FALSE;

        quick_tblock_dispose(&quick_tblock);
    }
    while(looping);

    return(status);
}

_Check_return_
static STATUS
new_style(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR tstr_style_name)
{
    STATUS status;
    NEW_STYLE_CALLBACK new_style_callback;
    BOOL texty_selection;
    ARRAY_HANDLE new_style_list_handle;
    PIXIT max_width;

    {
    MARK_INFO mark_info;
    BOOL any_selection;

    if(status_fail(object_skel(p_docu, T5_MSG_MARK_INFO_READ, &mark_info)))
        mark_info.h_markers = 0;

    any_selection = (mark_info.h_markers != 0);

    texty_selection = any_selection &&
                      ((OBJECT_ID_CELLS == p_docu->focus_owner)  ||
                       (OBJECT_ID_HEADER == p_docu->focus_owner) ||
                       (OBJECT_ID_FOOTER == p_docu->focus_owner) );
    } /*block*/

    zero_struct(new_style_callback);
    new_style_callback.tstr_style_name = tstr_style_name;
    new_style_callback.selected_item = -1;

    assert((es_process_interlock == MUTEX_AVAILABLE) || (es_process_interlock == MUTEX_UNAVAILABLE));

    if(es_process_interlock == MUTEX_UNAVAILABLE)
    {
        host_bleep();
        return(STATUS_OK);
    }

    status_return(status = ui_list_create_style(p_docu, &new_style_list_handle, &new_style_callback.ui_source, &max_width));

    while(status_ok(status)) /* loop for structure */
    {
        { /* make appropriate size box */
        PIXIT_SIZE list_size;
        DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
        dialog_cmd_ctl_size_estimate.p_dialog_control = &new_style_list;
        dialog_cmd_ctl_size_estimate.p_dialog_control_data = &stdlisttext_data;
        ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
        dialog_cmd_ctl_size_estimate.size.x += max_width;
        ui_list_size_estimate(array_elements(&new_style_list_handle), &list_size);
        dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
        dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
        dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, NEW_STYLE_MINLIST_H);
        new_style_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
        new_style_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
        } /*block*/

        {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, new_style_ctl_create, elemof32(new_style_ctl_create), SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_HELP_TOPIC);
        /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
        dialog_cmd_process_dbox.caption.text.resource_id = SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_CAPTION;
        dialog_cmd_process_dbox.bits.note_position = 1;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_new_style;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &new_style_callback;
        es_process_interlock = MUTEX_UNAVAILABLE;
        status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        es_process_interlock = MUTEX_AVAILABLE;
        } /*block*/

        if(status == DIALOG_COMPLETION_OK)
            status = new_style_do(p_docu, &new_style_callback, texty_selection);

        break; /* end of loop for structure */
        /*NOTREACHED*/
    }

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    ui_list_dispose_style(&new_style_list_handle, &new_style_callback.ui_source);

    ui_text_dispose(&new_style_callback.ui_text_style_new);

    return(status);
}

_Check_return_
static STATUS
style_apply_struct(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     PC_STYLE p_style)
{
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 500);
    quick_ublock_with_buffer_setup(quick_ublock);

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);

        if(status_ok(status = style_ustr_inline_from_struct(p_docu, &quick_ublock, p_style)))
        {
            p_args[0].val.ustr_inline = quick_ublock_ustr_inline(&quick_ublock);
            status = execute_command(p_docu, t5_message, &arglist_handle, object_id);
            quick_ublock_dispose(&quick_ublock);
        }

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

_Check_return_
static HOST_WND
get_colour_picker_parent_window_handle(P_MSG_UISTYLE_COLOUR_PICKER p_msg_uistyle_colour_picker)
{
    DIALOG_CMD_HWND_QUERY dialog_cmd_hwnd_query;
    dialog_cmd_hwnd_query.h_dialog = p_msg_uistyle_colour_picker->h_dialog;
    dialog_cmd_hwnd_query.dialog_control_id = p_msg_uistyle_colour_picker->rgb_dialog_control_id;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_HWND_QUERY, &dialog_cmd_hwnd_query));
    return(dialog_cmd_hwnd_query.hwnd);
}

T5_MSG_PROTO(extern, t5_msg_uistyle_colour_picker, P_MSG_UISTYLE_COLOUR_PICKER p_msg_uistyle_colour_picker)
{
    RGB rgb;
    BOOL res;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    {
    DIALOG_CMD_CTL_STATE_QUERY dialog_cmd_ctl_state_query;
    msgclr(dialog_cmd_ctl_state_query);
    dialog_cmd_ctl_state_query.h_dialog = p_msg_uistyle_colour_picker->h_dialog;
    dialog_cmd_ctl_state_query.bits = 0;
    dialog_cmd_ctl_state_query.dialog_control_id = p_msg_uistyle_colour_picker->rgb_dialog_control_id;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    rgb = dialog_cmd_ctl_state_query.state.user.rgb;
    } /*block*/

    /* disable controlling button as a trivial UI interlock */
    ui_dlg_ctl_enable(p_msg_uistyle_colour_picker->h_dialog, p_msg_uistyle_colour_picker->button_dialog_control_id, FALSE);

#if WINDOWS
    /* wop up Windows' colour selector */
    res = windows_colour_picker(get_colour_picker_parent_window_handle(p_msg_uistyle_colour_picker), &rgb);
#elif RISCOS
    /* wop up RISC OS's colour selector */
    res = riscos_colour_picker(get_colour_picker_parent_window_handle(p_msg_uistyle_colour_picker), &rgb);
#endif /* OS */

    ui_dlg_ctl_enable(p_msg_uistyle_colour_picker->h_dialog, p_msg_uistyle_colour_picker->button_dialog_control_id, TRUE);

    if(res)
    {
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
        msgclr(dialog_cmd_ctl_state_set);
        dialog_cmd_ctl_state_set.h_dialog = p_msg_uistyle_colour_picker->h_dialog;
        dialog_cmd_ctl_state_set.bits = 0;
        dialog_cmd_ctl_state_set.dialog_control_id = p_msg_uistyle_colour_picker->rgb_dialog_control_id;
        dialog_cmd_ctl_state_set.state.user.rgb = rgb;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(extern, t5_msg_uistyle_style_edit, P_MSG_UISTYLE_STYLE_EDIT p_msg_uistyle_style_edit)
{
    STATUS status;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    es_process_interlock = MUTEX_UNAVAILABLE;

    status = es_process(p_docu, p_msg_uistyle_style_edit->hwnd_parent, p_msg_uistyle_style_edit->p_caption,
                                p_msg_uistyle_style_edit->p_style_in, p_msg_uistyle_style_edit->p_style_selector,
                                p_msg_uistyle_style_edit->p_style_modified, p_msg_uistyle_style_edit->p_style_selector_modified,
                                p_msg_uistyle_style_edit->p_prohibited_enabler, p_msg_uistyle_style_edit->p_prohibited_enabler_2,
                                p_msg_uistyle_style_edit->p_style_out, p_msg_uistyle_style_edit->style_handle_being_modified,
                                p_msg_uistyle_style_edit->subdialog);

    es_process_interlock = MUTEX_AVAILABLE;

    return(status);
}

_Check_return_
extern STATUS
ui_style_msg_startup(void)
{
    /* SKS 02jan94 initialise statics for Windows restart */
    es_process_interlock = MUTEX_AVAILABLE;
    style_intro_interlock = MUTEX_AVAILABLE;
    return(STATUS_OK);
}

/* end of ui_style.c */
