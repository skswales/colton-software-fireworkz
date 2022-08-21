/* uistyle.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Style editing for ob_rec */

/* SKS December 94 */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

#include "ob_skspt/xp_uisty.h"

#include "ob_ss/resource/resource.h"

/******************************************************************************
*
* style intro dialog
*
******************************************************************************/

#define STYLE_INTRO_ID_LIST 355

static /*poked*/ DIALOG_CONTROL
style_intro_list =
{
    STYLE_INTRO_ID_LIST, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF },
    { 0, 0, 0/*poked*/, 0/*poked*/ },
    { DRT(LTLT, LIST_TEXT), 1 }
};

static const DIALOG_CONTROL
style_intro_change =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_MAIN_GROUP, DIALOG_MAIN_GROUP, DIALOG_CONTROL_SELF },
    { DIALOG_CONTENTS_CALC, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
style_intro_change_data = { { IDOK }, UI_TEXT_INIT_RESID(MSG_CHANGE) };

static const DIALOG_CTL_CREATE
style_intro_ctl_create[] =
{
    { &dialog_main_group },
    { &style_intro_list,   &stdlisttext_data },
    { &style_intro_change,  &style_intro_change_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

typedef struct REC_STYLE_ENTRY
{
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, elemof32("A reasonable style")); /* NB buffer adjacent for fixup */
}
REC_STYLE_ENTRY, * P_REC_STYLE_ENTRY;

typedef struct REC_STYLE_INTRO_CALLBACK
{
    UI_SOURCE ui_source;
    UI_TEXT ui_text;
    S32 selected_item;
}
REC_STYLE_INTRO_CALLBACK, * P_REC_STYLE_INTRO_CALLBACK;

_Check_return_
static STATUS
dialog_rec_style_intro_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    const P_REC_STYLE_INTRO_CALLBACK p_rec_style_intro_callback = (P_REC_STYLE_INTRO_CALLBACK) p_dialog_msg_ctl_fill_source->client_handle;

    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case STYLE_INTRO_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &p_rec_style_intro_callback->ui_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_rec_style_intro_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    const P_REC_STYLE_INTRO_CALLBACK p_rec_style_intro_callback = (P_REC_STYLE_INTRO_CALLBACK)p_dialog_msg_ctl_create_state->client_handle;

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case STYLE_INTRO_ID_LIST:
        p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = p_rec_style_intro_callback->selected_item;
        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALTERNATE;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_rec_style_intro_ctl_state_change(
    _InoutRef_  PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_REC_STYLE_INTRO_CALLBACK p_rec_style_intro_callback = (P_REC_STYLE_INTRO_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case STYLE_INTRO_ID_LIST:
        p_rec_style_intro_callback->selected_item = p_dialog_msg_ctl_state_change->new_state.list_text.itemno;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_rec_style_intro)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_rec_style_intro_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_rec_style_intro_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_rec_style_intro_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    default:
        return(STATUS_OK);
    }
}

#define STYLE_INDEX_BASE 0
#define STYLE_INDEX_FIELDS 1
#define STYLE_INDEX_TITLES 2
#define STYLE_INDEX_FIELD_0 3

_Check_return_
static STATUS
db_list_create_style_entry(
    /*inout*/ P_ARRAY_HANDLE p_array_handle,
    _In_z_      PCTSTR tstr)
{
    P_REC_STYLE_ENTRY p_rec_style_entry;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(*p_rec_style_entry), TRUE);
    STATUS status;

    if(NULL == (p_rec_style_entry = al_array_extend_by(p_array_handle, REC_STYLE_ENTRY, 1, &array_init_block, &status)))
        return(status);

    quick_tblock_with_buffer_setup(p_rec_style_entry->quick_tblock);

    return(quick_tblock_tstr_add_n(&p_rec_style_entry->quick_tblock, tstr, strlen_with_NULLCH));
}

_Check_return_
static STATUS
db_list_create_style(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector,
    /*out*/ P_ARRAY_HANDLE p_array_handle,
    /*out*/ P_UI_SOURCE p_ui_source,
    _OutRef_    P_PIXIT p_max_width)
{
    PIXIT max_width = DIALOG_SYSCHARSL_H(16); /* arbitrary minimum */
    STATUS status = STATUS_OK;

    *p_array_handle = 0;
    *p_max_width = 0;

    IGNOREPARM_DocuRef_(p_docu);

    for(;;) /* loop for structure */
    {
        {
        PCTSTR tstr = resource_lookup_tstr(REC_MSG_STYLE_INTRO_DATABASE);
        const PIXIT width = ui_width_from_tstr(tstr);
        max_width = MAX(max_width, width);
        status_break(status = db_list_create_style_entry(p_array_handle, tstr));
        } /*block*/

        {
        PCTSTR tstr = resource_lookup_tstr(REC_MSG_STYLE_INTRO_ALL_FIELDS);
        const PIXIT width = ui_width_from_tstr(tstr);
        max_width = MAX(max_width, width);
        status_break(status = db_list_create_style_entry(p_array_handle, tstr));
        } /*block*/

        {
        PCTSTR tstr = resource_lookup_tstr(REC_MSG_STYLE_INTRO_ALL_TITLES);
        const PIXIT width = ui_width_from_tstr(tstr);
        max_width = MAX(max_width, width);
        status_break(status = db_list_create_style_entry(p_array_handle, tstr));
        } /*block*/

        { /* all the fields */
        ARRAY_INDEX i;
        for(i = 0; i < array_elements(&p_rec_projector->opendb.table.h_fielddefs); i++)
        {
            P_FIELDDEF p_fielddef = array_ptr(&p_rec_projector->opendb.table.h_fielddefs, FIELDDEF, i);
            PCTSTR tstr;
            PIXIT width;
            TCHARZ buffer[32];
            if(p_fielddef && p_fielddef->name[0])
                tstr = p_fielddef->name;
            else
            {
                PCTSTR format = resource_lookup_tstr(REC_MSG_STYLE_INTRO_ANON_FIELD);
                consume_int(tstr_xsnprintf(buffer, elemof32(buffer), format, i));
                tstr = buffer;
            }
            width = ui_width_from_tstr(tstr);
            max_width = MAX(max_width, width);
            status_break(status = db_list_create_style_entry(p_array_handle, tstr));
        }
        status_break(status);
        } /*block*/

        break; /* out of loop for structure */
        /*NOTREACHED*/
    }

    max_width = MIN(max_width, DIALOG_SYSCHARSL_H(32)); /* arbitrary maximum */

    *p_max_width = max_width;

    /* make a source of text pointers to these elements for list box processing */
    if(status_ok(status))
        status = ui_source_create_tb(p_array_handle, p_ui_source, UI_TEXT_TYPE_TSTR_PERM, offsetof32(REC_STYLE_ENTRY, quick_tblock));

    if(status_fail(status))
        ui_list_dispose_style(p_array_handle, p_ui_source);

    return(status);
}

static void
rec_base_style_on_handle(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_STYLE p_style,
    _OutRef_    P_STYLE_SELECTOR p_style_selector,
    _InVal_     STYLE_HANDLE style_handle)
{
    STYLE based_style;
    STYLE_SELECTOR selector;

    style_init(&based_style);

    style_struct_from_handle(p_docu, &based_style, style_handle, &style_selector_all);

    /* all bits are set for edit that are present in this first style */
    style_selector_copy(p_style_selector, &based_style.selector);

    /* search for these other bits */
    void_style_selector_not(&selector, p_style_selector);

    /* copy from default all those bits which aren't yet set */
    style_copy_defaults(p_docu, &based_style, &style_selector_all);

    style_init(p_style);

    status_assert(style_duplicate(p_style, &based_style, &based_style.selector));

    style_dispose(&based_style);
}

_Check_return_
static STATUS
rec_style_insert_implied(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector,
    _In_z_      PCTSTR tstr_style_name,
    _In_        S32 arg_implied,
    _In_        REGION_CLASS region_class)
{
    STYLE_HANDLE style_handle = style_handle_from_name(p_docu, tstr_style_name);

    if(0 != style_handle)
    {
        PC_STYLE p_style = p_style_from_handle(p_docu, style_handle);

        if(!IS_P_STYLE_NONE(p_style))
        {
            /* check scope of style */
            STYLE_SELECTOR scope;

            style_selector_copy(&scope, &style_selector_all);
            style_selector_bit_clear(&scope, STYLE_SW_NAME);
            style_selector_bit_clear(&scope, STYLE_SW_KEY);
            style_selector_bit_clear(&scope, STYLE_SW_HANDLE);
            style_selector_bit_clear(&scope, STYLE_SW_SEARCH);

            if(!style_selector_test(&p_style->selector, &scope))
            {
                T5_MESSAGE t5_message = T5_EXT_STYLE_RECORDZ_FIELDS;
                ARRAY_INDEX array_index = -1;
                P_STYLE_DOCU_AREA p_style_docu_area;

                /* delete any exisiting implied style regions */
                while(NULL != (p_style_docu_area = style_docu_area_enum_implied(p_docu,
                                                                                &array_index,
                                                                                OBJECT_ID_REC,
                                                                                &t5_message,
                                                                                &arg_implied)))
                {
                    BOOL delete_it = FALSE;
                    if(arg_implied != DB_IMPLIED_ARG_FIELD)
                        delete_it = TRUE;
                    else
                    {
                        const PC_STYLE p_style_found = p_style_from_docu_area(p_docu, p_style_docu_area);
                        if(!IS_P_STYLE_NONE(p_style_found))
                            if(style_bit_test(p_style_found, STYLE_SW_NAME) &&
                               (0 == tstricmp(array_tstr(&p_style_found->h_style_name_tstr), tstr_style_name)) )
                                delete_it = TRUE;
                    }

                    if(delete_it)
                    {
                        assert(p_style_docu_area->style_handle);
                        style_handle_remove(p_docu, p_style_docu_area->style_handle);
                        style_docu_area_delete(p_docu, p_style_docu_area);

                    }
                }
            }
            else
            {
                /* add implied style region */
                STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;

                STYLE_DOCU_AREA_ADD_IMPLIED(&style_docu_area_add_parm,
                                            NULL,
                                            OBJECT_ID_REC,
                                            T5_EXT_STYLE_RECORDZ_FIELDS,
                                            arg_implied,
                                            region_class);
                style_docu_area_add_parm.type = STYLE_DOCU_AREA_ADD_TYPE_HANDLE;
                style_docu_area_add_parm.data.style_handle = style_handle;
                style_docu_area_add_parm.internal = 1;

                style_docu_area_add(p_docu, &p_rec_projector->rec_docu_area, &style_docu_area_add_parm);
            }
        }
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
rec_style_styles_apply(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector)
{
    TCHARZ style_name_prefix[128];
    U32 style_name_prefix_len;
    STYLE_HANDLE style_handle = STYLE_HANDLE_ENUM_START;
    P_STYLE p_style;
    STATUS status = STATUS_OK;

    {
    PTSTR tstr = style_name_prefix;
    *tstr++ = 0x03;
    tstr_xstrkpy(tstr, (elemof32(style_name_prefix) - 2) - (tstr - style_name_prefix), p_rec_projector->opendb.table.name);
    tstr += tstrlen32(tstr);
    *tstr++ = 0x03;
    *tstr = CH_NULL;
    style_name_prefix_len = tstrlen32(style_name_prefix);
    } /*block*/

    while(status_ok(style_enum_styles(p_docu, &p_style, &style_handle)))
    {
        PCTSTR tstr_style_name = array_tstr(&p_style->h_style_name_tstr); /* only get named styles from style_enum_styles */
        S32 arg_implied = DB_IMPLIED_ARG_FIELD;
        REGION_CLASS region_class = REGION_LOWER + 3;

        if(0 != C_strnicmp(tstr_style_name, style_name_prefix, style_name_prefix_len))
            continue;

        {
        PCTSTR tstr_style_name_sub = tstr_style_name + style_name_prefix_len;

        if(0 == tstricmp(tstr_style_name_sub, resource_lookup_tstr(REC_MSG_STYLE_INTRO_DATABASE)))
        {
            arg_implied = DB_IMPLIED_ARG_DATABASE;
            region_class = REGION_LOWER;
        }
        else if(0 == tstricmp(tstr_style_name_sub, resource_lookup_tstr(REC_MSG_STYLE_INTRO_ALL_FIELDS)))
        {
            arg_implied = DB_IMPLIED_ARG_ALL_FIELDS;
            region_class = REGION_LOWER + 1;
        }
        else if(0 == tstricmp(tstr_style_name_sub, resource_lookup_tstr(REC_MSG_STYLE_INTRO_ALL_TITLES)))
        {
            arg_implied = DB_IMPLIED_ARG_ALL_TITLES;
            region_class = REGION_LOWER + 2;
        }
        } /*block*/

        status_break(status = rec_style_insert_implied(p_docu, p_rec_projector, tstr_style_name, arg_implied, region_class));
    }

    return(status);
}

_Check_return_
extern STATUS
t5_cmd_db_style(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_db_id(p_docu, p_data_ref->arg.db_field.db_id);
    STATUS status;
    REC_STYLE_INTRO_CALLBACK rec_style_intro_callback;
    STYLE_HANDLE style_handle = STYLE_HANDLE_NONE; /* keep dataflower happy */
    ARRAY_HANDLE rec_style_intro_list_handle;
    PIXIT max_width;

    zero_struct(rec_style_intro_callback);
    rec_style_intro_callback.selected_item = STYLE_INDEX_FIELDS;

    status_return(status = db_list_create_style(p_docu, p_rec_projector, &rec_style_intro_list_handle, &rec_style_intro_callback.ui_source, &max_width));

    for(;;) /* loop for structure */
    {
        { /* make appropriate size box */
        PIXIT_SIZE list_size;
        DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
        dialog_cmd_ctl_size_estimate.p_dialog_control = &style_intro_list;
        dialog_cmd_ctl_size_estimate.p_dialog_control_data = &stdlisttext_data;
        ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
        dialog_cmd_ctl_size_estimate.size.x += max_width;
        ui_list_size_estimate(array_elements(&rec_style_intro_list_handle), &list_size);
        dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
        dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
        style_intro_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
        style_intro_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
        } /*block*/

        rec_style_intro_callback.selected_item = STYLE_INDEX_BASE;

        if(DATA_DB_FIELD == p_data_ref->data_space)
        { /* lookup 'current' style name from where we are field-wise and suggest he edits that */
            ARRAY_INDEX i;
            for(i = 0; i < array_elements(&p_rec_projector->opendb.table.h_fielddefs); i++)
            {
                PC_FIELDDEF p_fielddef = array_ptrc(&p_rec_projector->opendb.table.h_fielddefs, FIELDDEF, i);
                if(p_fielddef->id == p_data_ref->arg.db_field.field_id)
                {
                    rec_style_intro_callback.selected_item = STYLE_INDEX_FIELD_0 + i;
                    break;
                }
            }
        }
        else if(DATA_DB_TITLE == p_data_ref->data_space)
            rec_style_intro_callback.selected_item = STYLE_INDEX_TITLES;

        {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, style_intro_ctl_create, elemof32(style_intro_ctl_create), 0);
        /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
        dialog_cmd_process_dbox.caption.text.resource_id = REC_MSG_STYLE_INTRO_CAPTION;
        dialog_cmd_process_dbox.bits.note_position = 1;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_rec_style_intro;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &rec_style_intro_callback;
        status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        } /*block*/

        if(status_ok(status))
        {
            P_REC_STYLE_ENTRY p_rec_style_entry = array_ptr(&rec_style_intro_list_handle, REC_STYLE_ENTRY, rec_style_intro_callback.selected_item);
            TCHARZ style_name_buffer[128];
            STYLE style_in;
            STYLE_SELECTOR style_selector;
            QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 32);
            quick_tblock_with_buffer_setup(quick_tblock);

            style_init(&style_in);

            assert(array_index_valid(&rec_style_intro_list_handle, rec_style_intro_callback.selected_item));

            { /* ensure we have a named style for this database/field name combination*/
            PTSTR tstr = style_name_buffer;
            *tstr++ = 0x03;
            *tstr = CH_NULL;
            tstr_xstrkat(style_name_buffer, (elemof32(style_name_buffer) - 2),
                           p_rec_projector->opendb.table.name);
            tstr += tstrlen32(tstr);
            *tstr++ = 0x03;
            *tstr = CH_NULL;
            tstr_xstrkat(style_name_buffer, (elemof32(style_name_buffer) - 2),
                           quick_tblock_tstr(&p_rec_style_entry->quick_tblock));

            style_handle = style_handle_from_name(p_docu, style_name_buffer);
            if(0 == style_handle)
            {
                STYLE style;
                style_init(&style);
                if(status_ok(status = al_tstr_set(&style.h_style_name_tstr, style_name_buffer)))
                {
                    style_bit_set(&style, STYLE_SW_NAME);
                    status = style_handle_add(p_docu, &style);
                    status_break(status);
                    style_handle = (STYLE_HANDLE) status;
                }
            }
            } /*block*/

            /* start editing a duplicate of this style (because of paranoia about handles being deleted behind our backs) */
            rec_base_style_on_handle(p_docu, &style_in, &style_selector, style_handle);

            if(status_ok(status = quick_tblock_printf(&quick_tblock, resource_lookup_tstr(REC_MSG_STYLE_EDITING_CAPTION), quick_tblock_tstr(&p_rec_style_entry->quick_tblock)))
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

                style_init(&style_out);

                /* don't let the buggers turn this off! */
                style_selector_clear(&prohibited_enabler);
                style_selector_bit_set(&prohibited_enabler, STYLE_SW_NAME);

                style_selector_clear(&prohibited_enabler_2);

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
                status = object_call_id_load(p_docu, T5_MSG_UISTYLE_STYLE_EDIT, &msg_uistyle_style_edit, OBJECT_ID_SKEL_SPLIT);
                } /*block*/

                if(status == STATUS_OK) /* Cancel was mapped to STATUS_OK so map back */
                    status = STATUS_CANCEL;

                if(status_done(status))
                    if(0 != (style_handle = style_handle_from_name(p_docu, style_name_buffer)))
                    {
                        S32 arg_implied;
                        REGION_CLASS region_class;

                        style_handle_modify(p_docu, style_handle, &style_out, &style_modified, &style_selector_modified);
                        docu_modify(p_docu);

                        /* these codes are implicitly bound into
                         * the order of insertion into the dialog
                         */
                        switch(rec_style_intro_callback.selected_item)
                        {
                        case 0:
                            arg_implied = DB_IMPLIED_ARG_DATABASE;
                            region_class = REGION_LOWER;
                            break;

                        case 1:
                            arg_implied = DB_IMPLIED_ARG_ALL_FIELDS;
                            region_class = REGION_LOWER + 1;
                            break;

                        case 2:
                            arg_implied = DB_IMPLIED_ARG_ALL_TITLES;
                            region_class = REGION_LOWER + 2;
                            break;

                        default:
                            arg_implied = DB_IMPLIED_ARG_FIELD;
                            region_class = REGION_LOWER + 3;
                            break;
                        }

                        if(status_ok(status = rec_style_insert_implied(p_docu, p_rec_projector, style_name_buffer, arg_implied, region_class)))
                            status = STATUS_DONE;
                    }

                style_free_resources_all(&style_out);
                style_free_resources_all(&style_in);
            }

            quick_tblock_dispose(&quick_tblock);
        }

        break; /* end of loop for structure */
        /*NOTREACHED*/
    }

    ui_list_dispose_style(&rec_style_intro_list_handle, &rec_style_intro_callback.ui_source);

    status_assert(call_dialog(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    return(status);
}

/* ensure we create a /db/Database style for this database */

_Check_return_
_Ret_z_
static PTSTR
rec_style_database_name(
    _Out_writes_z_(elemof_buffer) PTSTR style_name_buffer,
    _InVal_     U32 elemof_buffer,
    P_REC_PROJECTOR p_rec_projector,
    _InVal_     STATUS resource_id)
{
    PTSTR tstr = style_name_buffer;
    U32 table_name_len = tstrlen32(p_rec_projector->opendb.table.name);
    PCTSTR res_tstr = resource_lookup_tstr(resource_id);
    U32 res_tstr_len = tstrlen32(res_tstr);
    U32 tot_len = 1 + table_name_len + 1 + res_tstr_len + 1 /*CH_NULL*/;

    if(tot_len <= elemof_buffer)
    {
        *tstr++ = 0x03;
        (void) strcpy(tstr, p_rec_projector->opendb.table.name);
        tstr += table_name_len;
        *tstr++ = 0x03;
        (void) strcpy(tstr, res_tstr);
    }

    return(style_name_buffer);
}

_Check_return_
extern STATUS
rec_style_database_create(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector)
{
    TCHARZ style_name_buffer[128];
    STYLE_HANDLE style_handle = style_handle_from_name(p_docu, rec_style_database_name(style_name_buffer, elemof32(style_name_buffer), p_rec_projector, REC_MSG_STYLE_INTRO_DATABASE));
    if(0 == style_handle)
    {
        STYLE style;
        style_init(&style);

        status_return(al_tstr_set(&style.h_style_name_tstr, style_name_buffer));
        style_bit_set(&style, STYLE_SW_NAME);

#if 0 /* SKS wishes he could remember why he put these in in the first place */
        if(status_ok(font_spec_name_alloc(&style.font_spec, TEXT("Helvetica"))))
            style_bit_set(&style, STYLE_SW_FS_NAME);

        style.font_spec.size_x = 0; /* ie same as height */
        style.font_spec.size_y = 12 * PIXITS_PER_POINT;
        style_bit_set(&style, STYLE_SW_FS_SIZE_X);
        style_bit_set(&style, STYLE_SW_FS_SIZE_Y);

        rgb_set(&style.font_spec.colour, 0, 0, 0); /* true black */
        style_bit_set(&style, STYLE_SW_FS_COLOUR);
#endif

        if(status_fail(table_check_access(&p_rec_projector->opendb.table, ACCESS_FUNCTION_EDIT)))
        {
            style.para_style.protect = 1;
            style_bit_set(&style, STYLE_SW_PS_PROTECT);
        }

        style.para_style.grid_left =
        style.para_style.grid_top =
        style.para_style.grid_right =
        style.para_style.grid_bottom = SF_BORDER_STANDARD;
        style_bit_set(&style, STYLE_SW_PS_GRID_LEFT);
        style_bit_set(&style, STYLE_SW_PS_GRID_TOP);
        style_bit_set(&style, STYLE_SW_PS_GRID_RIGHT);
        style_bit_set(&style, STYLE_SW_PS_GRID_BOTTOM);

        rgb_set(&style.para_style.rgb_grid_left, 0, 0, 0); /* true black */
        style.para_style.rgb_grid_top =
        style.para_style.rgb_grid_right =
        style.para_style.rgb_grid_bottom =
        style.para_style.rgb_grid_left;
        style_bit_set(&style, STYLE_SW_PS_RGB_GRID_LEFT);
        style_bit_set(&style, STYLE_SW_PS_RGB_GRID_TOP);
        style_bit_set(&style, STYLE_SW_PS_RGB_GRID_RIGHT);
        style_bit_set(&style, STYLE_SW_PS_RGB_GRID_BOTTOM);

        style.para_style.justify = SF_JUSTIFY_LEFT;
        style_bit_set(&style, STYLE_SW_PS_JUSTIFY);

        style.para_style.margin_para = 0;
        style.para_style.margin_left = 114;
        style.para_style.margin_right = 114;
        style.para_style.h_tab_list = 0;
        style_bit_set(&style, STYLE_SW_PS_MARGIN_PARA);
        style_bit_set(&style, STYLE_SW_PS_MARGIN_LEFT);
        style_bit_set(&style, STYLE_SW_PS_MARGIN_RIGHT);
        style_bit_set(&style, STYLE_SW_PS_TAB_LIST);

#if 0 /* SKS 01apr95 needs removing for draft stuff */
        style.para_style.para_start = 32;
        style.para_style.para_end = 16;
#endif
        style.para_style.line_space.type = SF_LINE_SPACE_SINGLE;
        style.para_style.line_space.leading = 72;
#if 0
        style_bit_set(&style, STYLE_SW_PS_PARA_START);
        style_bit_set(&style, STYLE_SW_PS_PARA_END);
#endif
        style_bit_set(&style, STYLE_SW_PS_LINE_SPACE);

        status_return(style_handle = style_handle_add(p_docu, &style));
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
rec_style_all_fields_create(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector)
{
    TCHARZ style_name_buffer[128];
    STYLE_HANDLE style_handle = style_handle_from_name(p_docu, rec_style_database_name(style_name_buffer, elemof32(style_name_buffer), p_rec_projector, REC_MSG_STYLE_INTRO_ALL_FIELDS));
    if(0 == style_handle)
    {
        STYLE style;
        style_init(&style);

        status_return(al_tstr_set(&style.h_style_name_tstr, style_name_buffer));
        style_bit_set(&style, STYLE_SW_NAME);

        style.para_style.border = SF_BORDER_STANDARD; /* SKS 04apr95 moved here to prevent compatibility winges */
        style_bit_set(&style, STYLE_SW_PS_BORDER);

        rgb_set(&style.para_style.rgb_border, 0, 0, 0); /* true black */
        style_bit_set(&style, STYLE_SW_PS_RGB_BORDER);

        status_return(style_handle = style_handle_add(p_docu, &style));
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
rec_style_all_titles_create(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector)
{
    TCHARZ style_name_buffer[128];
    STYLE_HANDLE style_handle = style_handle_from_name(p_docu, rec_style_database_name(style_name_buffer, elemof32(style_name_buffer), p_rec_projector, REC_MSG_STYLE_INTRO_ALL_TITLES));
    if(0 == style_handle)
    {
        STYLE style;
        style_init(&style);

        status_return(al_tstr_set(&style.h_style_name_tstr, style_name_buffer));
        style_bit_set(&style, STYLE_SW_NAME);

        style.para_style.border = SF_BORDER_NONE;
        style_bit_set(&style, STYLE_SW_PS_BORDER);

        style.para_style.margin_left = style.para_style.margin_right = style.para_style.margin_para = 0;
        style_bit_set(&style, STYLE_SW_PS_MARGIN_LEFT);
        style_bit_set(&style, STYLE_SW_PS_MARGIN_RIGHT);
        style_bit_set(&style, STYLE_SW_PS_MARGIN_PARA);

        status_return(style_handle = style_handle_add(p_docu, &style));
    }

    return(STATUS_OK);
}

/* end of uistyle.c */
