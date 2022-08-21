/* uisrch.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* PMF March 94 */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

#include "ob_skel/xp_skeld.h"

/* Set to 1 to make combo choices put the caret in the adjacent edit field... never has worked well */
#define SET_FOCUS 1

/******************************************************************************
*
* goto record dialog box
*
******************************************************************************/

enum GOTO_RECORD_IDS
{
    GOTO_RECORD_ID_STT = 32,
    GOTO_RECORD_ID_NUMBER_TEXT,
    GOTO_RECORD_ID_NUMBER
};

static const DIALOG_CONTROL
goto_record_number_text =
{
    GOTO_RECORD_ID_NUMBER_TEXT, DIALOG_MAIN_GROUP,

    { DIALOG_CONTROL_PARENT, GOTO_RECORD_ID_NUMBER,
      DIALOG_CONTROL_SELF,   GOTO_RECORD_ID_NUMBER },

    { 0, 0, DIALOG_CONTENTS_CALC, 0 },

    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
goto_record_number_text_data = { UI_TEXT_INIT_RESID(REC_MSG_GOTO_RECORD_NUMBER) };

static const DIALOG_CONTROL
goto_record_number =
{
    GOTO_RECORD_ID_NUMBER, DIALOG_MAIN_GROUP,

    { GOTO_RECORD_ID_NUMBER_TEXT, DIALOG_CONTROL_PARENT },

    { DIALOG_STDSPACING_H, 0, DIALOG_BUMP_H(6), DIALOG_STDBUMP_V },

    { DRT(RTLT, BUMP_S32), 1 }
};

static /*poked*/ UI_CONTROL_S32
goto_record_bump_control = { 1, 0 /*poked*/ };

static /*poked*/ DIALOG_CONTROL_DATA_BUMP_S32
goto_record_number_data = { { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/, &goto_record_bump_control } /* BUMP_XX */, 1 };

static const DIALOG_CTL_ID
goto_record_ok_data_argmap[] = { GOTO_RECORD_ID_NUMBER };

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
goto_record_ok_command = { T5_CMD_RECORDZ_GOTO_RECORD, OBJECT_ID_REC, NULL, goto_record_ok_data_argmap, { 0, 0, 0, 1 /*lookup_arglist*/} };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
goto_record_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_OK), &goto_record_ok_command };

static const DIALOG_CTL_CREATE
goto_record_ctl_create[] =
{
    { &dialog_main_group },

    { &goto_record_number_text, &goto_record_number_text_data },
    { &goto_record_number,      &goto_record_number_data },

    { &stdbutton_cancel,        &stdbutton_cancel_data },
    { &defbutton_ok,            &goto_record_ok_data }
};

_Check_return_
extern STATUS
t5_cmd_db_goto_record_intro(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, goto_record_ctl_create, elemof32(goto_record_ctl_create), 0);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = REC_MSG_GOTO_CAPTION;
    /*dialog_cmd_process_dbox.p_proc_client = NULL;*/

    {
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_db_id(p_docu, p_data_ref->arg.db_field.db_id);
    S32 r;
    S32 n;
    record_current(&p_rec_projector->opendb, &r, &n);
    r = get_record_from_rec_data_ref(p_data_ref);
    goto_record_number_data.state = r + 1;
    goto_record_bump_control.max_val = n;
    } /*block*/

    return(call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

/******************************************************************************
*
* search dialog box
*
******************************************************************************/

enum SEARCH_IDS
{
    SEARCH_ID_GROUP = 137,

    SEARCH_ID_CARDS_GROUP,
    SEARCH_ID_CARDS_KEEP,
    SEARCH_ID_CARDS_DISCARD,

    SEARCH_ID_CAPTION_GROUP,

    SEARCH_ID_FIELDNAME1,
    SEARCH_ID_POPUP1,
    SEARCH_ID_TEXT1
};

/* A group box to contain the fields */

static const DIALOG_CONTROL
search_group =
{
    SEARCH_ID_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
search_group_data = { UI_TEXT_INIT_RESID(REC_MSG_FIELDS_GROUP), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
search_caption_group =
{
    SEARCH_ID_CAPTION_GROUP, SEARCH_ID_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0 },
    { DRT(LTRB, GROUPBOX) }
};

/******************************************************************************

The Field buttons - 4 of them

Field Name: Operator: Popup: Search Text

******************************************************************************/

static /*poked*/ DIALOG_CONTROL
search_fieldname_1 =
{
    SEARCH_ID_FIELDNAME1, SEARCH_ID_CAPTION_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDEDIT_V },
    { DRT(LTLT, STATICTEXT) }
};

static const DIALOG_CONTROL
search_fieldname_2 =
{
    0 /*SEARCH_ID_FIELDNAMEn*/, SEARCH_ID_CAPTION_GROUP,
    { SEARCH_ID_FIELDNAME1, 0 /*SEARCH_ID_FIELDNAMEn-1*/, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDEDIT_V },
    { DRT(LBLT, STATICTEXT) }
};

static const DIALOG_CONTROL
search_popup_1 =
{
    SEARCH_ID_POPUP1, SEARCH_ID_GROUP,
    { SEARCH_ID_CAPTION_GROUP, SEARCH_ID_FIELDNAME1 },
    { DIALOG_STDSPACING_H, 0, DIALOG_SYSCHARSL_H(20), DIALOG_STDEDIT_V },
    { DRT(RTLT, COMBO_TEXT) }
};

static const DIALOG_CONTROL_DATA_COMBO_TEXT
search_popup_data =
{
  {/*combo_xx*/

    {/*edit_xx*/ {/*bits*/ FRAMED_BOX_EDIT, 1 /*readonly*/ /*bits*/}, NULL /*edit_xx*/},

    {/*list_xx*/ { 0 /*force_v_scroll*/, 1 /*disable_double*/, 0 /*tab_position*/} /*list_xx*/},

        NULL,

        (1 + REC_MSG_OPERATOR_LAST - REC_MSG_OPERATOR_BASE) * DIALOG_STDLISTITEM_V /*dropdown_size*/
      /*combo_xx*/},

    { UI_TEXT_TYPE_NONE } /*state*/
};

/* a writable icon */

static const DIALOG_CONTROL
search_text_1 =
{
    SEARCH_ID_TEXT1, SEARCH_ID_GROUP,
    { SEARCH_ID_POPUP1, SEARCH_ID_POPUP1 },
    { DIALOG_STDSPACING_H, 0, DIALOG_SYSCHARSL_H(24), DIALOG_STDEDIT_V },
    { DRT(RTLT, EDIT), 1 }
};

static const DIALOG_CONTROL_DATA_EDIT
search_text_data= { { { FRAMED_BOX_EDIT } }, { UI_TEXT_TYPE_NONE } };

/* a writable icon */

static const DIALOG_CONTROL
search_text_popup_1 =
{
    SEARCH_ID_TEXT1, SEARCH_ID_GROUP,
    { SEARCH_ID_POPUP1, SEARCH_ID_POPUP1 },
    { DIALOG_STDSPACING_H, 0, DIALOG_SYSCHARSL_H(24), DIALOG_STDEDIT_V },
    { DRT(RTLT, COMBO_TEXT), 1 }
};

static const UI_TEXT
search_text_popup_caption = UI_TEXT_INIT_RESID(REC_MSG_VALUE_CAPTION);

static const DIALOG_CONTROL_DATA_COMBO_TEXT
search_text_popup_data =
{
  {/*combo_xx*/

    {/*edit_xx*/ {/*bits*/ FRAMED_BOX_EDIT, 0 /*bits*/}, NULL /*edit_xx*/},

    {/*list_xx*/ { 0 /*force_v_scroll*/, 1 /*disable_double*/, 0 /*tab_position*/} /*list_xx*/},

        NULL,

        8 * DIALOG_STDLISTITEM_V /*dropdown_size*/

#if RISCOS
        , &search_text_popup_caption
#endif

      /*combo_xx*/},

    { UI_TEXT_TYPE_NONE } /*state*/
};

static const DIALOG_CONTROL_DATA_STATICTEXT
search_fieldname_data = { { UI_TEXT_TYPE_TSTR_TEMP } };

static const DIALOG_CONTROL
search_cards_group =
{
    SEARCH_ID_CARDS_GROUP, DIALOG_MAIN_GROUP,
    { SEARCH_ID_GROUP, SEARCH_ID_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, 0, 0 },
    { DRT(LBRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
search_cards_keep =
{
    SEARCH_ID_CARDS_KEEP, SEARCH_ID_CARDS_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
search_cards_keep_data= { { 0 }, SEARCH_ID_CARDS_KEEP, UI_TEXT_INIT_RESID(REC_MSG_CARDS_KEEP) };

static const DIALOG_CONTROL
search_cards_discard =
{
    SEARCH_ID_CARDS_DISCARD, SEARCH_ID_CARDS_GROUP,
    { SEARCH_ID_CARDS_KEEP, SEARCH_ID_CARDS_KEEP },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
search_cards_discard_data= { { 0 }, SEARCH_ID_CARDS_DISCARD, UI_TEXT_INIT_RESID(REC_MSG_CARDS_DISCARD) };

static const DIALOG_CONTROL
search_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF, DIALOG_MAIN_GROUP, SEARCH_ID_CARDS_GROUP },
    { DIALOG_CONTENTS_CALC, DIALOG_DEFPUSHBUTTON_V, 0, 0 },
    { DRT(RBRB, PUSHBUTTON), 1 }
};

static UI_SOURCE rec_search_op_combo_ui_source;

static UI_SOURCE rec_search_vl_combo_ui_source;

static ARRAY_HANDLE h_field_combo_ui_source_array;

_Check_return_
static STATUS
search_combo_set_state(
    _InVal_     H_DIALOG h_dialog,
    _In_        S32 i,
    _In_        S32 itemno)
{
    return(ui_dlg_set_list_idx(h_dialog, (DIALOG_CTL_ID) (i*3) + SEARCH_ID_POPUP1, itemno - REC_MSG_OPERATOR_BASE));
}

static S32
search_combo_item(
    _InVal_     H_DIALOG h_dialog,
    _In_        S32 i)
{
    return(ui_dlg_get_list_idx(h_dialog, (DIALOG_CTL_ID) (i*3) + SEARCH_ID_POPUP1) + REC_MSG_OPERATOR_BASE);
}

static ARRAY_HANDLE_USTR
make_name_for_query(
    P_REC_PROJECTOR p_rec_projector,
    _InVal_     ARRAY_HANDLE h_search_pattern,
    QUERY_ID parent_id)
{
    STATUS status;
    ARRAY_HANDLE h_text_ustr;
    P_USTR ustr_out;

    if(parent_id == RECORDZ_WHOLE_FILE)
    {
        if(NULL != (ustr_out = al_array_alloc(&h_text_ustr, UCHARZ, 1, &array_init_block_uchars, &status)))
            *ustr_out = CH_NULL;
    }
    else
    {
        P_QUERY p_query = p_query_from_p_opendb(&p_rec_projector->opendb, parent_id);
        PC_USTR ustr_parent_text = array_ustr(&p_query->h_name_ustr);
        S32 length = ustrlen32(ustr_parent_text);
        PC_USTR ustr_joint_text;

        switch(p_rec_projector->opendb.search.suggest_type)
        {
#if CHECKING
            case SEARCH_TYPE_CHANGE:
            case SEARCH_TYPE_FILE:
                /* Must not happen */
                assert0();

                /*FALLTHRU*/

            case SEARCH_TYPE_EXTEND:
#endif
            default:
                ustr_joint_text = resource_lookup_ustr(REC_MSG_TEXT_OR);
                break;

            case SEARCH_TYPE_REFINE:
                ustr_joint_text = resource_lookup_ustr(p_rec_projector->opendb.search.suggest_exclude ? REC_MSG_TEXT_EXCLUDING : REC_MSG_TEXT_AND);
                break;
        }

        length += ustrlen32(ustr_joint_text);

        if(NULL != (ustr_out = al_array_alloc(&h_text_ustr, UCHARZ, length + 2 /*two spaces*/ + 1 /*CH_NULL*/, &array_init_block_uchars, &status)))
            (void) strcat(strcat(strcat(strcpy(ustr_out, ustr_parent_text), " "), ustr_joint_text), " ");
    }

    if(0 != h_text_ustr)
    {
        BOOL first = TRUE;
        ARRAY_INDEX i;

        for(i = 0; i < array_elements(&h_search_pattern); i++)
        {
            P_SEARCH_FIELD_PATTERN p_search_pattern = array_ptr(&h_search_pattern, SEARCH_FIELD_PATTERN, i);
            PC_USTR ustr_search_pattern_text = array_ustr(&p_search_pattern->h_text_ustr);
            U32 search_pattern_text_len = ustrlen32(ustr_search_pattern_text);
            U32 this_field_len = search_pattern_text_len;
            U32 extra_len = 0;
            U32 rem_suf = 0;
            U32 rem_pre = 0;
            PC_USTR ustr_extra = NULL;

            switch(p_search_pattern->sop_info.search_operator)
            {
            case REC_MSG_OPERATOR_CONTAINS:
            case REC_MSG_OPERATOR_NOTCONT:
                {
                ustr_extra = resource_lookup_ustr(p_search_pattern->sop_info.search_operator);
                extra_len = ustrlen32(ustr_extra);
                rem_suf = p_search_pattern->sop_info.suffix_length;
                rem_pre = p_search_pattern->sop_info.prefix_length;
                break;
                }

            default:
                break;
            }

            if(this_field_len)
            {
                PC_USTR ustr_operator = NULL;
                PC_USTR ustr_excl = NULL;
                P_FIELDDEF p_fielddef = p_fielddef_from_number(&p_rec_projector->opendb.table.h_fielddefs, i+1);

                this_field_len += strlen32(p_fielddef->name) + 1/*space*/;

                if(!first)
                {
                    ustr_operator = resource_lookup_ustr(p_rec_projector->opendb.search.suggest_andor ? REC_MSG_TEXT_AND : REC_MSG_TEXT_OR);

                    this_field_len += ustrlen32(ustr_operator) + 2/*two spaces*/;
                }

                if(p_rec_projector->opendb.search.suggest_exclude)
                {
                    ustr_excl = resource_lookup_ustr(REC_MSG_TEXT_EXCLUDING);

                    this_field_len += ustrlen32(ustr_excl) + 1;
                }

                this_field_len += (extra_len + 1) - rem_suf - rem_pre;

                if(NULL != (ustr_out = al_array_extend_by(&h_text_ustr, UCHARZ, this_field_len, &array_init_block_uchars, &status)))
                {
                    ustr_out = de_const_cast(P_USTR, array_ustr(&h_text_ustr)); /* sadly, we must append here */

                    if(!first)
                        (void) strcat(strcat(strcat(ustr_out, " "), ustr_operator), " ");

                    if(NULL != ustr_excl)
                        (void) strcat(strcat(ustr_out, ustr_excl), " ");

                    (void) strcat(strcat(ustr_out, p_fielddef->name), " ");

                    if(extra_len)
                       (void) strcat(strcat(ustr_out, ustr_extra), " ");

                    {
                    U32 n_bytes = search_pattern_text_len - rem_suf - rem_pre;
                    ustr_out += ustrlen32(ustr_out);
                    memcpy32(ustr_out, ustr_search_pattern_text + rem_pre, n_bytes);
                    ustr_out[n_bytes] = CH_NULL;
                    } /*block*/
                }

                first = FALSE;
            }
        }
    }

    return(h_text_ustr);
}

typedef struct SEARCH_CALLBACK
{
    PC_DATA_REF p_data_ref;
    P_REC_PROJECTOR p_rec_projector;
}
SEARCH_CALLBACK, * P_SEARCH_CALLBACK;

_Check_return_
static STATUS
encode_search_fields(
    P_REC_PROJECTOR p_rec_projector,
    _InVal_     H_DIALOG h_dialog)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX i;
    S32 j;

    if(!array_elements(&p_rec_projector->opendb.search.h_suggest_pattern))
        return(STATUS_OK);

    /* for all fields do... look at the suggested pattern */
    j = 0;

    for(i = 0; i < array_elements(&p_rec_projector->opendb.table.h_fielddefs); i++)
    {
        P_FIELDDEF p_fielddef = array_ptr(&p_rec_projector->opendb.table.h_fielddefs, FIELDDEF, i);
        P_SEARCH_FIELD_PATTERN p_search_pattern = array_ptr(&p_rec_projector->opendb.search.h_suggest_pattern, SEARCH_FIELD_PATTERN, i);
        PC_USTR ustr_text;
        S32 start, end, length;
        QUICK_UBLOCK_WITH_BUFFER(aqub, 80);
        quick_ublock_with_buffer_setup(aqub);

        if(p_fielddef->hidden)
            continue;

        ustr_text = array_ustr(&p_search_pattern->h_text_ustr);
        length = ustrlen32(ustr_text);

        start = p_search_pattern->sop_info.prefix_length;
        end   = p_search_pattern->sop_info.suffix_length;
        length -= (start + end);

        /* copy text from (text + start) use length bytes into the edit field */
        if(status_ok(quick_ublock_uchars_add(&aqub, ustr_text + start, length)))
        if(status_ok(quick_ublock_nullch_add(&aqub)))
        {
            UI_TEXT ui_text;
            ui_text_alloc_from_ustr(&ui_text, quick_ublock_ustr(&aqub));
            ui_dlg_set_edit(h_dialog, (DIALOG_CTL_ID) (SEARCH_ID_TEXT1 + j*3), &ui_text);
            ui_text_dispose(&ui_text);
        }

        quick_ublock_dispose(&aqub);

        status = search_combo_set_state(h_dialog, j, p_search_pattern->sop_info.search_operator);

        j++;
    }

    return(status);
}

/* SEARCH DIALOG BOX PROC */

/* Fill the combo boxes from the sources */

_Check_return_
static STATUS
dialog_uisrch_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    if((((S32) p_dialog_msg_ctl_fill_source->dialog_control_id - SEARCH_ID_POPUP1) % 3) == 0)
    {
        p_dialog_msg_ctl_fill_source->p_ui_source = &rec_search_op_combo_ui_source;
    }
    else
    {
        const S32 n = ((S32) p_dialog_msg_ctl_fill_source->dialog_control_id - SEARCH_ID_TEXT1) / 3;
        const P_UI_SOURCE p_ui_source = array_ptr(&h_field_combo_ui_source_array, UI_SOURCE, n);

        if(p_ui_source->type == UI_SOURCE_TYPE_NONE)
            p_dialog_msg_ctl_fill_source->p_ui_source = &rec_search_vl_combo_ui_source;
        else
            p_dialog_msg_ctl_fill_source->p_ui_source = p_ui_source;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_uisrch_process_start(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_SEARCH_CALLBACK p_search_callback = (P_SEARCH_CALLBACK) p_dialog_msg_process_start->client_handle;
    const P_REC_PROJECTOR p_rec_projector = p_search_callback->p_rec_projector;
    ARRAY_INDEX i;

    /* Set up the combo box contents */
    for(i = 0; i < array_elements(&p_rec_projector->opendb.table.h_fielddefs); i++)
        status_return(search_combo_set_state(p_dialog_msg_process_start->h_dialog, i, REC_MSG_OPERATOR_DONTCARE));

    /* set up the radio buttons appropriate to the suggestion */
    status_return(ui_dlg_set_radio(p_dialog_msg_process_start->h_dialog, SEARCH_ID_CARDS_GROUP,
                                   p_rec_projector->opendb.search.suggest_exclude ? SEARCH_ID_CARDS_DISCARD : SEARCH_ID_CARDS_KEEP));

    /* set up the combos and the edit controls according to the suggestions */
    if(p_rec_projector->opendb.search.h_suggest_pattern)
        status_return(encode_search_fields(p_rec_projector, p_dialog_msg_process_start->h_dialog));

    {
    S32 itemno = 0;

    /* let's try selecting the current field */
    if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
    {
        DATA_REF data_ref = *p_search_callback->p_data_ref;

        set_rec_data_ref_from_object_position(p_docu, &data_ref, &p_docu->cur.object_position);

        if(DATA_DB_FIELD == data_ref.data_space)
        {
            S32 current_field_number = fieldnumber_from_rec_data_ref(p_docu, &data_ref);
            if(status_ok(current_field_number))
                itemno = current_field_number - 1;
        }
    }
    else
    {
        S32 current_field_number = fieldnumber_from_field_id(&p_rec_projector->opendb.table, field_id_from_rec_data_ref(p_search_callback->p_data_ref));
        if(status_ok(current_field_number))
            itemno = current_field_number - 1;
    }

    p_dialog_msg_process_start->initial_focus = (DIALOG_CTL_ID) (SEARCH_ID_TEXT1 + 3*itemno);
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_uisrch_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const DIALOG_CTL_ID control_id = p_dialog_msg_ctl_state_change->dialog_control_id;
    const P_SEARCH_CALLBACK p_search_callback = (P_SEARCH_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;
    const P_REC_PROJECTOR p_rec_projector = p_search_callback->p_rec_projector;
    ARRAY_INDEX i;
    DIALOG_CTL_ID j = 0;

    for(i = 0; i < array_elements(&p_rec_projector->opendb.table.h_fielddefs); i++)
    {
        const P_FIELDDEF p_fielddef = array_ptr(&p_rec_projector->opendb.table.h_fielddefs, FIELDDEF, i);

        if(p_fielddef->hidden)
            continue;

        if(control_id == (SEARCH_ID_TEXT1 + j*3))
        {   /* State change in one of the writable fields. We must make the combo box non-dont-care! */
            S32 search_operator =  search_combo_item(p_dialog_msg_ctl_state_change->h_dialog, j);

            switch(search_operator)
            {
            case REC_MSG_OPERATOR_DONTCARE:
                {
                PC_USTR ustr = ui_text_ustr(&p_dialog_msg_ctl_state_change->new_state.edit.ui_text);
                U32 len = strspn(ustr, "<>="); /* how big is it then ? */
                S32 state = (len == 0) ? REC_MSG_OPERATOR_CONTAINS : REC_MSG_OPERATOR_EXPRESS;
                search_combo_set_state(p_dialog_msg_ctl_state_change->h_dialog, j, state);
                break;
                }
            }
        }
        else
        {
#if SET_FOCUS
            if(control_id == (SEARCH_ID_POPUP1 + j*3))
            {
                DIALOG_CMD_CTL_FOCUS_SET dialog_cmd_ctl_focus_set;
                /* the j-th combo just changed put the caret in the j-th box */
                dialog_cmd_ctl_focus_set.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
                dialog_cmd_ctl_focus_set.control_id = (SEARCH_ID_TEXT1 + j*3);
                status_assert(call_dialog(DIALOG_CMD_CODE_CTL_FOCUS_SET, &dialog_cmd_ctl_focus_set));
            }
#endif
        }

        j++;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_uisrch_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    const P_SEARCH_CALLBACK p_search_callback = (P_SEARCH_CALLBACK) p_dialog_msg_process_end->client_handle;
    P_REC_PROJECTOR p_rec_projector = p_search_callback->p_rec_projector;
    ARRAY_INDEX i;
    DIALOG_CTL_ID j;
    QUERY_ID query_id = RECORDZ_WHOLE_FILE; /* keep dataflower happy */
    QUERY_ID parent_id;
    ARRAY_HANDLE h_search_pattern;
    STATUS status = STATUS_OK;

    if(!status_ok(p_dialog_msg_process_end->completion_code))
        return(STATUS_OK);

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(SEARCH_FIELD_PATTERN), TRUE);
    if(NULL == al_array_alloc(&h_search_pattern, SEARCH_FIELD_PATTERN, array_elements(&p_rec_projector->opendb.table.h_fielddefs), &array_init_block, &status))
        return(status);
    } /*block*/

    /* Create an array of search patterns */
    j = 0;

    for(i = 0; i < array_elements(&p_rec_projector->opendb.table.h_fielddefs); i++)
    {
        P_FIELDDEF p_fielddef = array_ptr(&p_rec_projector->opendb.table.h_fielddefs, FIELDDEF, i);
        P_SEARCH_FIELD_PATTERN p_search_pattern = array_ptr(&h_search_pattern, SEARCH_FIELD_PATTERN, i);
        P_ARRAY_HANDLE_USTR p_array_handle_search = &p_search_pattern->h_text_ustr;

        p_search_pattern->sop_info.prefix_length = 0;
        p_search_pattern->sop_info.suffix_length = 0;

        p_search_pattern->field_id = p_fielddef->id;

        if(status_fail(status)) /* from ^^^ */
            p_search_pattern->field_id = FIELD_ID_BAD;

        if(p_fielddef->hidden)
        {
            p_search_pattern->sop_info.search_operator = REC_MSG_OPERATOR_DONTCARE;

            status_assert(al_ustr_set(p_array_handle_search, ""));
        }
        else
        {
            UI_TEXT ui_text;
            PC_U8 ustr;
            S32 length;

            ui_dlg_get_edit(p_dialog_msg_process_end->h_dialog, (DIALOG_CTL_ID) (SEARCH_ID_TEXT1+j*3),&ui_text);
            ustr = ui_text_ustr(&ui_text);
            length = ustrlen32(ustr);

/* We must try to intelligently set up search expressions
  dependant on the type of data in the field

  The ISEMPTY, NOTEMPTY EXPRESS and DONTCARE operators are type independant

  The IS and ISNT operators ???

  Type of field       CONTAINS        DOES NOT CONTAIN

  FIELD_PICTURE       illegal         illegal
  FIELD_FILE          illegal         illegal

  FIELD_BOOL          LIKE <argument> NOT LIKE <argument>

  FIELD_INTERVAL      LIKE <argument> NOT LIKE <argument>
  FIELD_DATE          LIKE <argument> NOT LIKE <argument>
  FIELD_REAL          LIKE <argument> NOT LIKE <argument>
  FIELD_INTEGER       LIKE <argument> NOT LIKE <argument>
  FIELD_FORMULA       LIKE <argument> NOT LIKE <argument>

  FIELD_TEXT          "*<argument>*"  NOT "*<argument>*"

  The GREATER and LESS operators ???
*/

            switch(p_search_pattern->sop_info.search_operator = search_combo_item(p_dialog_msg_process_end->h_dialog, j))
            {
            case REC_MSG_OPERATOR_DONTCARE: /* the null string */
                status_assert(al_ustr_set(p_array_handle_search, ""));
                break;

            case REC_MSG_OPERATOR_ISEMPTY:
                status_assert(al_ustr_set(p_array_handle_search, "= NULL"));
                p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                break;

            case REC_MSG_OPERATOR_NOTEMPTY:
                status_assert(al_ustr_set(p_array_handle_search, "<> NULL"));
                p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                break;

            case REC_MSG_OPERATOR_EXPRESS:
                status_assert(al_ustr_set(p_array_handle_search, ustr));
                break;

            case REC_MSG_OPERATOR_IS:
                switch(p_fielddef->type)
                {
                case FIELD_BOOL:
                    if(length != 0)
                    {   /* Really ought to be Yes No Null or some combination with OR */
                        p_search_pattern->sop_info.prefix_length = 0;
                        status_assert(al_ustr_set(p_array_handle_search, ustr));
                        status_assert(al_ustr_append(p_array_handle_search, ""));
                        p_search_pattern->sop_info.suffix_length = 0;
                    }
                    else
                    {
                        status_assert(al_ustr_set(p_array_handle_search, ""));
                    }
                    break;

                case FIELD_INTERVAL: /* Dont know */
                case FIELD_DATE:
                case FIELD_INTEGER:
                case FIELD_REAL:
                case FIELD_FORMULA:
                case FIELD_PICTURE:
                case FIELD_FILE:
                case FIELD_TEXT:
                default:
                    if(length != 0)
                    {
                        status_assert(al_ustr_set(p_array_handle_search, "= "));
                        p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                        status_assert(al_ustr_append(p_array_handle_search, ustr));
                        break;
                    }
                    else
                    {
                        status_assert(al_ustr_set(p_array_handle_search, "= NULL"));
                        p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                    }
                    break;
                }
                break;

            case REC_MSG_OPERATOR_ISNT:
                switch(p_fielddef->type)
                {
                case FIELD_BOOL:
                    if(length != 0)
                    {   /* Really ought to be Yes No Null or some combination with OR */
                        status_assert(al_ustr_set(p_array_handle_search, "NOT ( "));
                        p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                        status_assert(al_ustr_append(p_array_handle_search, ustr));
                        status_assert(al_ustr_append(p_array_handle_search, " )"));
                        p_search_pattern->sop_info.suffix_length = 2;
                    }
                    else
                    {
                        status_assert(al_ustr_set(p_array_handle_search, ""));
                    }
                    break;

                case FIELD_INTERVAL: /* Dont know */
                case FIELD_DATE:
                case FIELD_INTEGER:
                case FIELD_REAL:
                case FIELD_FORMULA:
                case FIELD_PICTURE:
                case FIELD_FILE:
                case FIELD_TEXT:
                default:
                    if(length != 0)
                    {
                        status_assert(al_ustr_set(p_array_handle_search, "<> "));
                        p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                        status_assert(al_ustr_append(p_array_handle_search, ustr));
                    }
                    else
                    {
                        status_assert(al_ustr_set(p_array_handle_search, "<> NULL"));
                        p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                    }
                    break;
                }
                break;

            case REC_MSG_OPERATOR_CONTAINS:
                switch(p_fielddef->type)
                {
                case FIELD_PICTURE:
                case FIELD_FILE:
                case FIELD_TEXT:
                default:
                    /* allowed */
                    if(length != 0)
                    {
                        status_assert(al_ustr_set(p_array_handle_search, "\"*"));
                        p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                        status_assert(al_ustr_append(p_array_handle_search, ustr));
                        status_assert(al_ustr_append(p_array_handle_search, "*\""));
                        p_search_pattern->sop_info.suffix_length = 2;
                    }
                    else
                    { /* CONTAINS "blank" is a bit silly! */
                        status_assert(al_ustr_set(p_array_handle_search, ""));
                    }
                    break;

                case FIELD_BOOL:
                    if(length != 0)
                    {
                        /* Really ought to be Yes No Null or some combination with OR */
                        p_search_pattern->sop_info.prefix_length = 0;
                        status_assert(al_ustr_set(p_array_handle_search, ustr));
                        status_assert(al_ustr_append(p_array_handle_search, ""));
                        p_search_pattern->sop_info.suffix_length = 0;
                    }
                    else
                    {
                        status_assert(al_ustr_set(p_array_handle_search, ""));
                    }
                    break;

                case FIELD_INTERVAL: /* Dont know */
                case FIELD_DATE:
                case FIELD_INTEGER:
                case FIELD_REAL:
                case FIELD_FORMULA:
                    /* change to LIKE */
                    if(length != 0)
                    {
                        status_assert(al_ustr_set(p_array_handle_search, "LIKE "));
                        p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                        status_assert(al_ustr_append(p_array_handle_search, ustr));
                        status_assert(al_ustr_append(p_array_handle_search, ""));
                        p_search_pattern->sop_info.suffix_length = 0;
                    }
                    else
                    { /* CONTAINS "blank" is a bit silly! */
                        status_assert(al_ustr_set(p_array_handle_search, ""));
                    }
                    break;

                } /* end switch field type */
                break;

            case REC_MSG_OPERATOR_NOTCONT:
                switch(p_fielddef->type)
                {
                case FIELD_PICTURE:
                case FIELD_FILE:
                case FIELD_TEXT:
                default:
                    /* allowed */
                    if(length != 0)
                    {
                        status_assert(al_ustr_set(p_array_handle_search, "NOT \"*"));
                        p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                        status_assert(al_ustr_append(p_array_handle_search, ustr));
                        status_assert(al_ustr_append(p_array_handle_search, "*\""));
                        p_search_pattern->sop_info.suffix_length = 2;
                    }
                    else
                    { /* DOES NOT CONTAIN "blank" is a bit silly! */
                        status_assert(al_ustr_set(p_array_handle_search, ""));
                    }
                    break;

                case FIELD_BOOL:
                    if(length != 0)
                    {   /* Really ought to be Yes No Null or some combination with OR */
                        status_assert(al_ustr_set(p_array_handle_search, "NOT ( "));
                        p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                        status_assert(al_ustr_append(p_array_handle_search, ustr));
                        status_assert(al_ustr_append(p_array_handle_search, " )"));
                        p_search_pattern->sop_info.suffix_length = 2;
                    }
                    else
                    {
                        status_assert(al_ustr_set(p_array_handle_search, ""));
                    }
                    break;

                case FIELD_INTERVAL: /* Dont know */
                case FIELD_DATE:
                case FIELD_INTEGER:
                case FIELD_REAL:
                case FIELD_FORMULA:
                    /* change to LIKE */
                    if(length != 0)
                    {
                        status_assert(al_ustr_set(p_array_handle_search, "NOT LIKE "));
                        p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                        status_assert(al_ustr_append(p_array_handle_search, ustr));
                        status_assert(al_ustr_append(p_array_handle_search, ""));
                        p_search_pattern->sop_info.suffix_length = 0;
                    }
                    else
                    { /* DOES NOT CONTAIN "blank" is a bit silly! */
                        status_assert(al_ustr_set(p_array_handle_search, ""));
                    }
                    break;

                } /* end switch field type */
                break;

            case REC_MSG_OPERATOR_GREATER:
                switch(p_fielddef->type)
                {
                case FIELD_INTERVAL: /* Dont know */
                case FIELD_DATE:
                case FIELD_INTEGER:
                case FIELD_REAL:
                case FIELD_FORMULA:
                case FIELD_TEXT:
                default:
                    if(length != 0)
                    {
                        status_assert(al_ustr_set(p_array_handle_search, "> "));
                        p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                        status_assert(al_ustr_append(p_array_handle_search, ustr));
                    }
                    else
                    { /* GREATER THAN "blank" is a bit silly! */
                        status_assert(al_ustr_set(p_array_handle_search, ""));
                    }

                    break;

                case FIELD_PICTURE:
                case FIELD_FILE:
                case FIELD_BOOL:
                    /* Silly - ignore it */
                    status_assert(al_ustr_set(p_array_handle_search, ""));
                    break;

                } /* end switch field type */
                break;

            case REC_MSG_OPERATOR_LESS:
                switch(p_fielddef->type)
                {
                case FIELD_INTERVAL: /* Dont know */
                case FIELD_DATE:
                case FIELD_INTEGER:
                case FIELD_REAL:
                case FIELD_FORMULA:
                case FIELD_TEXT:
                default:
                    if(length != 0)
                    {
                        status_assert(al_ustr_set(p_array_handle_search, "< "));
                        p_search_pattern->sop_info.prefix_length = array_elements(p_array_handle_search)-1;
                        status_assert(al_ustr_append(p_array_handle_search, ustr));
                    }
                    else
                    { /* GREATER THAN "blank" is a bit silly! */
                        status_assert(al_ustr_set(p_array_handle_search, ""));
                    }

                    break;

                case FIELD_PICTURE:
                case FIELD_FILE:
                case FIELD_BOOL:
                    /* Silly - ignore it */
                    status_assert(al_ustr_set(p_array_handle_search, ""));
                    break;

                } /* end switch field type */
                break;

            } /* end switch search operator */

            ui_text_dispose(&ui_text);

            j++;
        }
    }

    {
    P_QUERY p_query;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_query), TRUE);
    if(NULL != (p_query = al_array_extend_by(&p_rec_projector->opendb.search.h_query, QUERY, 1, &array_init_block, &status)))
        p_query->query_id = query_id = unique_query_id(&p_rec_projector->opendb.search.h_query);
    } /*block*/

    /* To Refine or Extend you should copy the query from the current one
       To search the whole file you should just start from a blank one
    */
    parent_id = p_rec_projector->opendb.search.query_id;

    if(p_rec_projector->opendb.search.suggest_type == SEARCH_TYPE_CHANGE)
    { /* Should delete the parent and just use this one */
        p_rec_projector->opendb.search.suggest_type = SEARCH_TYPE_FILE;

        close_cursor(&p_rec_projector->opendb.table); /* Closing the cursor first seems to help */

        close_query(&p_rec_projector->opendb, p_query_from_p_opendb(&p_rec_projector->opendb, parent_id));

        p_rec_projector->opendb.search.query_id = RECORDZ_WHOLE_FILE;

        parent_id = p_rec_projector->opendb.search.query_id;
    }

    {
    P_QUERY p_query = p_query_from_p_opendb(&p_rec_projector->opendb, query_id);

    p_rec_projector->opendb.search.query_id = query_id;

    p_query->parent_id = parent_id;

    p_query->search_type = p_rec_projector->opendb.search.suggest_type;
    p_rec_projector->opendb.search.suggest_andor = p_query->search_andor = TRUE;
    p_rec_projector->opendb.search.suggest_exclude = p_query->search_exclude = (ui_dlg_get_radio(p_dialog_msg_process_end->h_dialog, SEARCH_ID_CARDS_GROUP) == SEARCH_ID_CARDS_DISCARD);

    p_query->h_search_pattern = h_search_pattern;

    p_query->h_name_ustr = make_name_for_query(p_rec_projector, h_search_pattern, parent_id);
    } /*block*/

    open_query(&p_rec_projector->opendb, query_id);

    view_update_all(p_docu_from_docno(p_rec_projector->docno), UPDATE_PANE_CELLS_AREA);

    return(status);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_search)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_uisrch_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_uisrch_process_start(p_docu, (P_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_uisrch_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_uisrch_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
op_source_create(
    _OutRef_    P_UI_SOURCE p_ui_source)
{
    STATUS status;
    ARRAY_INDEX i;

    p_ui_source->type = UI_SOURCE_TYPE_NONE;

    if(NULL == al_array_alloc_UI_TEXT(&p_ui_source->source.array_handle, (REC_MSG_OPERATOR_LAST + 1) - REC_MSG_OPERATOR_BASE, &array_init_block_ui_text, &status))
        return(status);

    p_ui_source->type = UI_SOURCE_TYPE_ARRAY;

    for(i = 0; i < array_elements(&p_ui_source->source.array_handle); ++i)
    {
        P_UI_TEXT p_ui_text = array_ptr(&p_ui_source->source.array_handle, UI_TEXT, i);

        p_ui_text->type = UI_TEXT_TYPE_RESID;
        p_ui_text->text.resource_id = REC_MSG_OPERATOR_BASE + i;
    }

    return(status);
}

_Check_return_
static STATUS
vl_source_create(
    _OutRef_    P_UI_SOURCE p_ui_source)
{
    STATUS status;
    ARRAY_INDEX i;

    p_ui_source->type = UI_SOURCE_TYPE_NONE;

    if(NULL == al_array_alloc_UI_TEXT(&p_ui_source->source.array_handle, (REC_MSG_VL_LAST + 1) - REC_MSG_VL_BASE, &array_init_block_ui_text, &status))
        return(status_nomem());

    p_ui_source->type = UI_SOURCE_TYPE_ARRAY;

    for(i = 0; i < array_elements(&p_ui_source->source.array_handle); ++i)
    {
        P_UI_TEXT p_ui_text = array_ptr(&p_ui_source->source.array_handle, UI_TEXT, i);

        p_ui_text->type = UI_TEXT_TYPE_RESID;
        p_ui_text->text.resource_id = REC_MSG_VL_BASE + i;
    }

    return(status);
}

_Check_return_
static STATUS
field_source_create(
    _OutRef_    P_UI_SOURCE p_ui_source,
    _In_z_      PC_U8Z p_u8)
{
    STATUS status = STATUS_OK;

    p_ui_source->type = UI_SOURCE_TYPE_NONE;
    p_ui_source->source.array_handle = 0;

    if(0 == strlen32(p_u8))
        return(status);

    for(;;)
    {
        PC_U8Z p_u8_next = strchr(p_u8, CH_COMMA);
        P_UI_TEXT p_ui_text;
        PTSTR tstr_alloc = NULL;
        U32 len;

        if(NULL == (p_ui_text = al_array_extend_by_UI_TEXT(&p_ui_source->source.array_handle, 1, &array_init_block_ui_text, &status)))
            break;

        p_ui_source->type = UI_SOURCE_TYPE_ARRAY;

        len = (NULL != p_u8_next) ? (p_u8_next - p_u8) : strlen(p_u8);

        status_break(status = tstr_set_n(&tstr_alloc, p_u8, len));

        p_ui_text->type = UI_TEXT_TYPE_TSTR_ALLOC;
        p_ui_text->text.tstr = tstr_alloc;

        if(NULL == p_u8_next)
            break;

        p_u8 = p_u8_next + 1;
    }

    return(status);
}

typedef union SEARCH_CONTROL_DATA_UNION
{
    DIALOG_CONTROL_DATA_STATICTEXT statictext;
    DIALOG_CONTROL_DATA_COMBO_TEXT combo_text;
    DIALOG_CONTROL_DATA_EDIT edit;
}
SEARCH_CONTROL_DATA_UNION;

static void
add_dialog_control(
    _Inout_     P_DIALOG_CTL_CREATE_RW * const p_p_ctl_create,
    _InRef_     PC_DIALOG_CONTROL p_dialog_control,
  /*_InRef_*/   PC_ANY p_dialog_control_data)
{
    P_DIALOG_CTL_CREATE_RW p_ctl_create = *p_p_ctl_create;
    p_ctl_create->p_dialog_control.p_dialog_control = p_dialog_control;
    p_ctl_create->p_dialog_control_data = p_dialog_control_data;
    p_ctl_create += 1;
    *p_p_ctl_create = p_ctl_create;
}

_Check_return_
extern STATUS
t5_cmd_db_search(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_db_id(p_docu, p_data_ref->arg.db_field.db_id);
    SEARCH_CALLBACK search_callback;
    STATUS status;
    S32 n_visible;
    S32 n_ctls;

    /* The dialog control creation structure */
    ARRAY_HANDLE h_dialog_ctl_create = 0; /* keep dataflower happy */
    P_DIALOG_CTL_CREATE_RW p_search_ctl_create = NULL; /* keep dataflower happy */

    /* The dynamic controls */
    ARRAY_HANDLE h_dialog_control = 0; /* keep dataflower happy */
    DIALOG_CONTROL * p_dialog_control = NULL; /* keep dataflower happy */

    /* Data for the dynamic controls */
    ARRAY_HANDLE h_ctrl_data = 0; /* keep dataflower happy */
    SEARCH_CONTROL_DATA_UNION * p_ctrl_data = NULL; /* keep dataflower happy */

    status_return(table_check_access(&p_rec_projector->opendb.table, ACCESS_FUNCTION_SEARCH));

    search_callback.p_rec_projector = p_rec_projector;
    search_callback.p_data_ref = p_data_ref;

    n_visible = ensure_some_field_visible(&p_rec_projector->opendb.table); /* there will be (we hope!) */
    n_ctls = 3 + 3 + 2 + (3 * n_visible);

    rec_search_op_combo_ui_source.type = UI_SOURCE_TYPE_NONE;
    rec_search_vl_combo_ui_source.type = UI_SOURCE_TYPE_NONE;

    status = op_source_create(&rec_search_op_combo_ui_source);

    if(status_ok(status))
        status = vl_source_create(&rec_search_vl_combo_ui_source);

    if(status_ok(status))
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(UI_SOURCE), TRUE);
        consume_ptr(al_array_alloc(&h_field_combo_ui_source_array, UI_SOURCE, n_visible, &array_init_block, &status));
    }

    if(status_ok(status))
    {   /* Allocate the dialog_ctrl array */
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(DIALOG_CONTROL), FALSE);
        p_dialog_control = al_array_alloc(&h_dialog_control, DIALOG_CONTROL, 3 * n_visible, &array_init_block, &status);
    }

    if(status_ok(status))
    {   /* Allocate the dialog_ctrl_data array */
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(SEARCH_CONTROL_DATA_UNION), FALSE);
        p_ctrl_data = al_array_alloc(&h_ctrl_data, SEARCH_CONTROL_DATA_UNION, 3 * n_visible, &array_init_block, &status);
    }

    if(status_ok(status))
    {   /* Allocate the ctl_create */
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(DIALOG_CTL_CREATE), FALSE);
        p_search_ctl_create = al_array_alloc(&h_dialog_ctl_create, DIALOG_CTL_CREATE, n_ctls, &array_init_block, &status);
    }

    if(status_ok(status))
    {
        ARRAY_INDEX field;
        S32 j = 0;

        for(field = 0; field < array_elements(&p_rec_projector->opendb.table.h_fielddefs); ++field)
        {
            PC_FIELDDEF p_fielddef = array_ptrc(&p_rec_projector->opendb.table.h_fielddefs, FIELDDEF, field);
            DIALOG_CTL_ID control_id;

            if(p_fielddef->hidden)
                continue;

            status_break(status = field_source_create(array_ptr(&h_field_combo_ui_source_array, UI_SOURCE, field), p_fielddef->value_list));

            /* The field name control */
            if(j == 0)
                *p_dialog_control = search_fieldname_1;
            else
            {
                *p_dialog_control = search_fieldname_2;
              /*p_dialog_control->relative_control_id[0] = (DIALOG_CTL_ID) (j-3) + SEARCH_ID_FIELDNAME1; leave as SEARCH_ID_FIELDNAME1*/
                p_dialog_control->relative_control_id[1] = (DIALOG_CTL_ID) (j-3) + SEARCH_ID_FIELDNAME1;
            }

            p_dialog_control->control_id = control_id = (DIALOG_CTL_ID) (j+0) + SEARCH_ID_FIELDNAME1;
            p_dialog_control++;

            p_ctrl_data->statictext = search_fieldname_data;
            status_break(status = ui_text_alloc_from_tstr(&p_ctrl_data->statictext.caption, _tstr_from_sbstr(p_fielddef->name)));
            p_ctrl_data++;

            /* Edit control */
            if(strlen(p_fielddef->value_list))
            {
                *p_dialog_control = search_text_popup_1;

                p_dialog_control->relative_control_id[0] =
                p_dialog_control->relative_control_id[1] = control_id + 1;

                p_ctrl_data->combo_text = search_text_popup_data;
                p_ctrl_data++;
            }
            else
            {
                *p_dialog_control = search_text_1;

                p_dialog_control->relative_control_id[0] =
                p_dialog_control->relative_control_id[1] = control_id + 1;

                p_ctrl_data->edit = search_text_data;
                p_ctrl_data++;
            }

            p_dialog_control->control_id = control_id + 2;
            p_dialog_control++;

            /* The popup control */
            *p_dialog_control = search_popup_1;

            p_dialog_control->control_id             = control_id + 1;
            p_dialog_control->relative_control_id[0] = SEARCH_ID_CAPTION_GROUP /*control_id + 0*/;
            p_dialog_control->relative_control_id[1] = control_id + 0;
            p_dialog_control++;

            p_ctrl_data->combo_text = search_popup_data;
            p_ctrl_data++;

            j += 3;
        }
    }

    if(status_ok(status))
    {
        /* fill the DIALOG_CTL_CREATE */
        add_dialog_control(&p_search_ctl_create, &dialog_main_group, NULL);
        add_dialog_control(&p_search_ctl_create, &search_group, &search_group_data);
        add_dialog_control(&p_search_ctl_create, &search_caption_group, NULL);

        { /* Insert pointers to the dynamically created controls */
        ARRAY_INDEX i;

        for(i = 0, p_dialog_control = array_base(&h_dialog_control, DIALOG_CONTROL), p_ctrl_data = array_base(&h_ctrl_data, SEARCH_CONTROL_DATA_UNION); i < n_visible; i++)
        {
            add_dialog_control(&p_search_ctl_create, p_dialog_control++, &p_ctrl_data->statictext);
            p_ctrl_data++;

            add_dialog_control(&p_search_ctl_create, p_dialog_control++, &p_ctrl_data->edit);
            p_ctrl_data++;

            add_dialog_control(&p_search_ctl_create, p_dialog_control++, &p_ctrl_data->combo_text);
            p_ctrl_data++;
        }
        } /*block*/

        /* Add the pointers to the field & card ctl groups */
        add_dialog_control(&p_search_ctl_create, &search_cards_group,   NULL);
        add_dialog_control(&p_search_ctl_create, &search_cards_keep,    &search_cards_keep_data);
        add_dialog_control(&p_search_ctl_create, &search_cards_discard, &search_cards_discard_data);

        add_dialog_control(&p_search_ctl_create, &stdbutton_cancel, &stdbutton_cancel_data);
        add_dialog_control(&p_search_ctl_create, &search_ok,        &defbutton_ok_data);
    }

    if(status_ok(status))
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, array_range(&h_dialog_ctl_create, DIALOG_CTL_CREATE, 0, n_ctls), n_ctls, 0);
        /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
        dialog_cmd_process_dbox.caption.text.resource_id = REC_MSG_SEARCH_CAPTION;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_search;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &search_callback;
        status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    }

    {
    S32 i;

    for(i = 0, p_ctrl_data = array_base(&h_ctrl_data, SEARCH_CONTROL_DATA_UNION); i < n_visible; i++)
    {
        ui_text_dispose(&p_ctrl_data->statictext.caption);
        p_ctrl_data += 3; /* three at a time */
    }
    } /*block*/

    {
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&h_field_combo_ui_source_array); i++)
        ui_source_dispose(array_ptr(&h_field_combo_ui_source_array, UI_SOURCE, i));

    al_array_dispose(&h_field_combo_ui_source_array);
    } /*block*/

    ui_source_dispose(&rec_search_op_combo_ui_source);
    ui_source_dispose(&rec_search_vl_combo_ui_source);

    al_array_dispose(&h_dialog_ctl_create);
    al_array_dispose(&h_dialog_control);
    al_array_dispose(&h_ctrl_data);

    if(status_ok(status))
    {
        docu_modify(p_docu);

        rec_update_projector_adjust_goto(p_rec_projector_from_db_id(p_docu, p_data_ref->arg.db_field.db_id), 0);
    }

    return(status);
}

enum QUERY_CONTROL_IDS
{
    QUERY_ID_VIEW = IDOK,
    QUERY_ID_LIST = 237,
    QUERY_ID_REFINE,
    QUERY_ID_EXTEND,
    QUERY_ID_CHANGE,
    QUERY_ID_DELETE,
    QUERY_ID_NEW
};

static /*poked*/ DIALOG_CONTROL
query_list =
{
    QUERY_ID_LIST, DIALOG_CONTROL_WINDOW,
    { DIALOG_MAIN_GROUP, DIALOG_MAIN_GROUP, DIALOG_CONTROL_SELF, DIALOG_MAIN_GROUP },
    { 0 },
    { DRT(LTLB, LIST_TEXT), 1 }
};

/* ALL button */

static const DIALOG_CONTROL
query_view =
{
    QUERY_ID_VIEW, DIALOG_MAIN_GROUP,
    { QUERY_ID_LIST, QUERY_ID_LIST },
    { DIALOG_STDSPACING_H, 0, DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARS_H("Wechseln"), DIALOG_DEFPUSHBUTTON_V },
    { DRT(RTLT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
query_view_data = { { QUERY_ID_VIEW }, UI_TEXT_INIT_RESID(REC_MSG_VIEW_QUERY) };

static const DIALOG_CONTROLH
query_refine =
{
    { QUERY_ID_REFINE, DIALOG_MAIN_GROUP,
    { QUERY_ID_VIEW, QUERY_ID_VIEW, QUERY_ID_VIEW },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1, 0, 1 } }, UI_TEXT_INIT_RESID(REC_MSG_VIEW_EXPLAIN_REFINE)
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
query_refine_data = { { QUERY_ID_REFINE }, UI_TEXT_INIT_RESID(REC_MSG_REFINE_QUERY) };

static const DIALOG_CONTROLH
query_extend =
{
    { QUERY_ID_EXTEND, DIALOG_MAIN_GROUP,
    { QUERY_ID_REFINE, QUERY_ID_REFINE, QUERY_ID_REFINE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1, 0, 1 } }, UI_TEXT_INIT_RESID(REC_MSG_VIEW_EXPLAIN_EXTEND)
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
query_extend_data = { { QUERY_ID_EXTEND }, UI_TEXT_INIT_RESID(REC_MSG_EXTEND_QUERY) };

static const DIALOG_CONTROLH
query_change =
{
    { QUERY_ID_CHANGE, DIALOG_MAIN_GROUP,
    { QUERY_ID_EXTEND, QUERY_ID_EXTEND, QUERY_ID_EXTEND },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1, 0, 1 } }, UI_TEXT_INIT_RESID(REC_MSG_VIEW_EXPLAIN_CHANGE)
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
query_change_data = { { QUERY_ID_CHANGE }, UI_TEXT_INIT_RESID(REC_MSG_CHANGE_QUERY) };

static const DIALOG_CONTROL
query_delete =
{
    QUERY_ID_DELETE, DIALOG_MAIN_GROUP,
    { QUERY_ID_CHANGE, QUERY_ID_CHANGE, QUERY_ID_CHANGE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
query_delete_data = { { QUERY_ID_DELETE }, UI_TEXT_INIT_RESID(REC_MSG_DELETE_QUERY) };

static const DIALOG_CONTROL
query_new =
{
    QUERY_ID_NEW, DIALOG_MAIN_GROUP,
    { QUERY_ID_DELETE, QUERY_ID_DELETE, QUERY_ID_DELETE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
query_new_data = { { QUERY_ID_NEW }, UI_TEXT_INIT_RESID(REC_MSG_NEW_QUERY) };

static const DIALOG_CONTROL
query_cancel =
{
    IDCANCEL, DIALOG_MAIN_GROUP,
    { QUERY_ID_NEW, QUERY_ID_NEW, QUERY_ID_NEW },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CTL_CREATE
query_ctl_create[] =
{
    { &dialog_main_group },
    { &query_list,   &stdlisttext_data },
    { &query_view,   &query_view_data },
    { &query_refine, &query_refine_data },
    { &query_extend, &query_extend_data },
    { &query_change, &query_change_data },
    { &query_delete, &query_delete_data },
    { &query_new,    &query_new_data },
    { &query_cancel, &stdbutton_cancel_data }
};

static UI_SOURCE
rec_view_list_source;

_Check_return_
static STATUS
dialog_query_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case QUERY_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &rec_view_list_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_query_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    return(ui_dlg_set_list_idx(p_dialog_msg_process_start->h_dialog, QUERY_ID_LIST, 0));
}

_Check_return_
static STATUS
dialog_query_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case QUERY_ID_LIST:
        { /* The selected itemno in the list box has changed */
        const P_REC_PROJECTOR p_rec_projector = (P_REC_PROJECTOR) p_dialog_msg_ctl_state_change->client_handle;
        S32 itemno = ui_dlg_get_list_idx(p_dialog_msg_ctl_state_change->h_dialog, QUERY_ID_LIST);
        BOOL enable_change = FALSE;
        BOOL enable_delete = FALSE;

        itemno -= 1; /* skip 'whole file' entry */

        if(array_index_valid(&p_rec_projector->opendb.search.h_query, itemno))
        {
            P_QUERY p_query = array_ptr_no_checks(&p_rec_projector->opendb.search.h_query, QUERY, itemno);
            enable_change = (RECORDZ_WHOLE_FILE == p_query->parent_id); /* non-compound query? */
            enable_delete = !has_query_got_offspring(&p_rec_projector->opendb, p_query->query_id);
        }

        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, QUERY_ID_CHANGE, enable_change);
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, QUERY_ID_DELETE, enable_delete);
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, QUERY_ID_EXTEND, (itemno >= 0));

        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_query_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    P_REC_PROJECTOR p_rec_projector = (P_REC_PROJECTOR) p_dialog_msg_process_end->client_handle;
    S32 itemno = ui_dlg_get_list_idx(p_dialog_msg_process_end->h_dialog, QUERY_ID_LIST); /* Suss out which is the selected query */
    P_QUERY p_query = NULL;
    STATUS status = STATUS_OK;
    BOOL refresh = FALSE;

    if(!status_ok(p_dialog_msg_process_end->completion_code))
        return(STATUS_OK);

    itemno -= 1; /* skip 'whole file' entry */

    if(array_index_valid(&p_rec_projector->opendb.search.h_query, itemno))
        p_query = array_ptr_no_checks(&p_rec_projector->opendb.search.h_query, QUERY, itemno);

    switch(p_dialog_msg_process_end->completion_code)
    {
    default: default_unhandled();
#if CHECKING
    case QUERY_ID_VIEW:
#endif
        drop_search_suggestions(&p_rec_projector->opendb.search); /* Drop the suggestions now, we are about to make some new ones */
        refresh = TRUE;

        p_rec_projector->opendb.search.query_id        = (NULL != p_query) ? p_query->query_id : RECORDZ_WHOLE_FILE;
        p_rec_projector->opendb.search.suggest_type    = SEARCH_TYPE_FILE;
        p_rec_projector->opendb.search.suggest_andor   = 2; /* Eh ? */
        p_rec_projector->opendb.search.suggest_exclude = FALSE;

        /* oh, unless the query has not yet been "done" */
        if((NULL != p_query) && (NULL == p_query->p_query))
            open_query(&p_rec_projector->opendb, p_query->query_id);

        break;

    case QUERY_ID_REFINE:
        /* can't change this */
        if(NULL != p_query)
        {
            drop_search_suggestions(&p_rec_projector->opendb.search); /* Drop the suggestions now, we are about to make some new ones */
            refresh = TRUE;

            p_rec_projector->opendb.search.query_id        = p_query->query_id;
            p_rec_projector->opendb.search.suggest_type    = SEARCH_TYPE_REFINE;
            p_rec_projector->opendb.search.suggest_andor   = 2; /* Eh ? */
            p_rec_projector->opendb.search.suggest_exclude = FALSE;
        }

        break;

    case QUERY_ID_EXTEND:
        /* can't change this */
        if(NULL != p_query)
        {
            drop_search_suggestions(&p_rec_projector->opendb.search); /* Drop the suggestions now, we are about to make some new ones */
            refresh = TRUE;

            p_rec_projector->opendb.search.query_id        = p_query->query_id;
            p_rec_projector->opendb.search.suggest_type    = SEARCH_TYPE_EXTEND;
            p_rec_projector->opendb.search.suggest_andor   = 2; /* Eh ? */
            p_rec_projector->opendb.search.suggest_exclude = FALSE;
        }

        break;

    case QUERY_ID_CHANGE:
        /* can't change this */
        if(NULL != p_query)
        {
            drop_search_suggestions(&p_rec_projector->opendb.search); /* Drop the suggestions now, we are about to make some new ones */
            refresh = TRUE;

            if(RECORDZ_WHOLE_FILE != p_query->parent_id)
            {
                /* This is a compound query */
                status = STATUS_FAIL;
                break;
            }

            /* Copy the search stuff into the suggestions */
            p_rec_projector->opendb.search.query_id        = p_query->query_id;
            p_rec_projector->opendb.search.suggest_type    = SEARCH_TYPE_CHANGE;
            p_rec_projector->opendb.search.suggest_andor   = p_query->search_andor;
            p_rec_projector->opendb.search.suggest_exclude = p_query->search_exclude;

            status_break(status = copy_search_pattern(&p_rec_projector->opendb.search.h_suggest_pattern, &p_query->h_search_pattern));
        }

        break;

    case QUERY_ID_DELETE:
        /* can't delete this */
        if(NULL != p_query)
        {
            drop_search_suggestions(&p_rec_projector->opendb.search); /* Drop the suggestions now, we are about to make some new ones */
            refresh = TRUE;

            close_cursor(&p_rec_projector->opendb.table); /* Closing the cursor first seems to help */

            close_query(&p_rec_projector->opendb, p_query);

            p_rec_projector->opendb.search.query_id = RECORDZ_WHOLE_FILE;

            docu_modify(p_docu_from_docno(p_rec_projector->docno));
        }

        break;

    case QUERY_ID_NEW:
        drop_search_suggestions(&p_rec_projector->opendb.search); /* Drop the suggestions now, we are about to make some new ones */
        refresh = TRUE;
        break;
   }

    if(refresh)
    {
        /* Here we should close the cursor since we are changing views!!!! */
        close_cursor(&p_rec_projector->opendb.table);

        view_update_all(p_docu_from_docno(p_rec_projector->docno), UPDATE_PANE_CELLS_AREA);
    }

    return(status);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_query)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_query_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_query_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_query_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_query_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
query_source_create(
    P_REC_PROJECTOR p_rec_projector,
    _OutRef_    P_UI_SOURCE p_ui_source,
    _InoutRef_  P_PIXIT p_max_width)
{
    STATUS status;
    P_UI_TEXT p_ui_text;
    ARRAY_INDEX i;

    p_ui_source->type = UI_SOURCE_TYPE_NONE;

    if(NULL == (p_ui_text = al_array_alloc_UI_TEXT(&p_ui_source->source.array_handle, 1 /*whole file*/ + array_elements(&p_rec_projector->opendb.search.h_query), &array_init_block_ui_text, &status)))
        return(status);

    p_ui_source->type = UI_SOURCE_TYPE_ARRAY;

    p_ui_text->type = UI_TEXT_TYPE_RESID;
    p_ui_text->text.resource_id = REC_MSG_WHOLE_FILE;

    {
    const PIXIT width = ui_width_from_p_ui_text(p_ui_text);
    if( *p_max_width < width)
        *p_max_width = width;
    } /*block*/

    p_ui_text++;

    for(i = 0; i < array_elements(&p_rec_projector->opendb.search.h_query); ++i)
    {
        P_QUERY p_query = array_ptr(&p_rec_projector->opendb.search.h_query, QUERY, i);
        PC_USTR ustr = array_ustr(&p_query->h_name_ustr);

        if(IS_P_DATA_NONE(ustr))
        {
            assert0(); /* it has happened */
            ustr = ustr_empty_string;
        }

        {
        const PIXIT width = ui_width_from_ustr(ustr);
        if( *p_max_width < width)
            *p_max_width = width;
        } /*block*/

        p_ui_text->type = UI_TEXT_TYPE_USTR_PERM;
        p_ui_text->text.ustr = ustr;

        p_ui_text++;
    }

    return(status);
}

_Check_return_
extern STATUS
t5_cmd_db_view(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_db_id(p_docu, p_data_ref->arg.db_field.db_id);
    STATUS status;

    /* do some boring initialisation */
    rec_view_list_source.type = UI_SOURCE_TYPE_NONE;

    { /* make appropriate size box */
    PIXIT max_width = 0; /* no arbitrary minimum; we always include 'whole file' query */
    if(status_ok(status = query_source_create(p_rec_projector, &rec_view_list_source, &max_width)))
    {
        DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
        dialog_cmd_ctl_size_estimate.p_dialog_control = &query_list;
        dialog_cmd_ctl_size_estimate.p_dialog_control_data = &stdlisttext_data;
        ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
        max_width = MIN(max_width, DIALOG_SYSCHARSL_H(64)); /* arbitrary maximum */
        query_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x + max_width;
    }
    } /*block*/

    if(status_ok(status))
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, query_ctl_create, elemof32(query_ctl_create), 0);
        /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
        dialog_cmd_process_dbox.caption.text.resource_id = REC_MSG_QUERY_CAPTION;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_query;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_rec_projector;
        status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    }

    /* no disposal of the ui_texts as they're only on loan to the dialog */
    al_array_dispose(&rec_view_list_source.source.array_handle);

    switch(status)
    {
    default:
        status_break(status);

        /*FALLTHRU*/

    case QUERY_ID_VIEW:
        rec_update_projector_adjust(p_rec_projector);
        status = STATUS_DONE;
        break;

    case QUERY_ID_REFINE:
    case QUERY_ID_EXTEND:
    case QUERY_ID_NEW:
    case QUERY_ID_CHANGE:
        status = t5_cmd_db_search(p_docu, p_data_ref);
        break;
    }

    return(status);
}

/* -------------------------------------------------------------------------------------------- */

enum SORT_CONTROL_IDS
{
    SORT_ID_LIST  = 337,
    SORT_ID_UP,
    SORT_ID_DOWN,
    SORT_ID_ORDER
};

static /*poked*/ DIALOG_CONTROL
sort_list =
{
    SORT_ID_LIST, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0 },
    { DRT(LTLT, LIST_TEXT), 1 }
};

static const DIALOG_CONTROL
sort_up =
{
    SORT_ID_UP, DIALOG_CONTROL_WINDOW,
    { SORT_ID_LIST, SORT_ID_LIST },
    { DIALOG_STDSPACING_H, 0, DIALOG_STDCANCEL_H, DIALOG_STDPUSHBUTTON_V },
    { DRT(RTLT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
sort_up_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_SORT_UP) };

static const DIALOG_CONTROL
sort_order =
{
    SORT_ID_ORDER, DIALOG_CONTROL_WINDOW,
    { SORT_ID_UP, SORT_ID_UP, SORT_ID_UP },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
sort_order_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_SORT_ORDER) };

static const DIALOG_CONTROL
sort_down =
{
    SORT_ID_DOWN, DIALOG_CONTROL_WINDOW,
    { SORT_ID_ORDER, SORT_ID_ORDER, SORT_ID_ORDER },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
sort_down_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_SORT_DOWN) };

/* these ones stand up from the bottom */

static const DIALOG_CONTROL
sort_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
    { SORT_ID_DOWN, DIALOG_CONTROL_SELF, SORT_ID_DOWN, IDOK },
    { 0, DIALOG_STDPUSHBUTTON_V, 0, DIALOG_STDSPACING_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL
sort_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { IDCANCEL, DIALOG_CONTROL_SELF, IDCANCEL, SORT_ID_LIST },
    { 0, DIALOG_DEFPUSHBUTTON_V, 0, 0 },
    { DRT(LBRB, PUSHBUTTON), 1 }
};

static const DIALOG_CTL_CREATE
sort_ctl_create[] =
{
    { &sort_list,   &stdlisttext_data },
    { &sort_up,     &sort_up_data },
    { &sort_order,  &sort_order_data },
    { &sort_down,   &sort_down_data },
    { &sort_cancel, &stdbutton_cancel_data },
    { &sort_ok,     &defbutton_ok_data }
};

static UI_SOURCE rec_sort_fields;

static ARRAY_HANDLE rec_sort_array_handle; /* a corresponding array of REMAP_ENTRYs */

_Check_return_
static STATUS
dialog_sort_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case SORT_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &rec_sort_fields;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_sort_process_start(PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    return(ui_dlg_set_list_idx(p_dialog_msg_process_start->h_dialog, SORT_ID_LIST, 0));
}

_Check_return_
static STATUS
dialog_sort_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case SORT_ID_UP:
    case SORT_ID_DOWN:
        {
        const P_REC_PROJECTOR p_rec_projector = (P_REC_PROJECTOR) p_dialog_msg_ctl_pushbutton->client_handle;
        const S32 n_flds = array_elements(&p_rec_projector->opendb.table.h_fielddefs);
        S32 delta  = p_dialog_msg_ctl_pushbutton->right_button ? -1 : +1;
        S32 itemno = ui_dlg_get_list_idx(p_dialog_msg_ctl_pushbutton->h_dialog, SORT_ID_LIST);
        S32 itemno_m_delta;

        if(p_dialog_msg_ctl_pushbutton->dialog_control_id == SORT_ID_DOWN)
            delta = - delta;

        itemno_m_delta = itemno - delta;

        if((itemno >= 0) && (itemno < n_flds) && (itemno_m_delta >= 0) && (itemno_m_delta < n_flds))
        {
            P_UI_TEXT p_ui_text = array_range(&rec_sort_fields.source.array_handle, UI_TEXT, 0, n_flds);
            P_REMAP_ENTRY p_remap_entry = array_range(&rec_sort_array_handle, REMAP_ENTRY, 0, n_flds);

            /* Swap over the ui_texts in the array */
            memswap32(&p_ui_text[itemno], &p_ui_text[itemno - delta], sizeof32(*p_ui_text));

            memswap32(&p_remap_entry[itemno], &p_remap_entry[itemno - delta], sizeof32(*p_remap_entry));

            ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, SORT_ID_LIST);

            ui_dlg_set_list_idx(p_dialog_msg_ctl_pushbutton->h_dialog, SORT_ID_LIST, itemno_m_delta);
        }

        p_dialog_msg_ctl_pushbutton->processed = 1;

        break;
        }

    case IDOK: /* new new new !!! */
        if(!p_dialog_msg_ctl_pushbutton->double_control_id)
            break;

        /*FALLTHRU*/

    case SORT_ID_ORDER:
        {
        S32 itemno = ui_dlg_get_list_idx(p_dialog_msg_ctl_pushbutton->h_dialog, SORT_ID_LIST);
        P_UI_TEXT p_ui_text = array_ptr(&rec_sort_fields.source.array_handle, UI_TEXT, itemno);
        P_REMAP_ENTRY p_remap_entry = array_ptr(&rec_sort_array_handle, REMAP_ENTRY, itemno);

        assert(p_ui_text->type == UI_TEXT_TYPE_TSTR_ALLOC);

        switch(p_ui_text->text.tstr[0])
        {
        default:            p_ui_text->text.tstr_wr[0] = CH_PLUS_SIGN;  p_remap_entry->sort_order = SORT_AZ;   break;
        case CH_PLUS_SIGN:  p_ui_text->text.tstr_wr[0] = CH_MINUS_SIGN__BASIC; p_remap_entry->sort_order = SORT_ZA;   break;
        case CH_MINUS_SIGN__BASIC: p_ui_text->text.tstr_wr[0] = 'x';           p_remap_entry->sort_order = SORT_NULL; break;
        }

        ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, SORT_ID_LIST);

        p_dialog_msg_ctl_pushbutton->processed = 1;

        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_sort)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_sort_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_sort_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_sort_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
sort_source_create(
    P_REC_PROJECTOR p_rec_projector,
    /*out*/ P_UI_SOURCE p_ui_source,
    _InoutRef_  P_PIXIT p_max_width,
    _OutRef_    P_S32 p_n_fields)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX field;

    *p_n_fields = 0;

    p_ui_source->type = UI_SOURCE_TYPE_ARRAY;
    p_ui_source->source.array_handle = 0;

    for(field = 0; field < array_elements(&p_rec_projector->opendb.table.h_fielddefs); ++field)
    {
        PC_FIELDDEF p_fielddef = array_ptrc(&p_rec_projector->opendb.table.h_fielddefs, FIELDDEF, field);
        P_UI_TEXT p_ui_text;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
        quick_ublock_with_buffer_setup(quick_ublock);

        if(p_fielddef->hidden)
            continue;

        /* <<< Its supposed to be hidden so don't add it to the ui source
           Trouble is the remap code will have to add it to the remap array so you've got a problem!
           Perhaps the default remap could create a structure with all the hidden fields at the end?
        */

        if(NULL == (p_ui_text = al_array_extend_by_UI_TEXT(&p_ui_source->source.array_handle, 1, &array_init_block_ui_text, &status)))
            break;

        switch(p_fielddef->keyorder)
        {
        case 1:
            status = quick_ublock_a7char_add(&quick_ublock, CH_PLUS_SIGN);
            break;

        case -1:
            status = quick_ublock_a7char_add(&quick_ublock, CH_MINUS_SIGN__BASIC);
            break;

        default:
            status = quick_ublock_a7char_add(&quick_ublock, 'x');
            break;
        }

         if(status_ok(status))
         if(status_ok(status = quick_ublock_a7char_add(&quick_ublock, CH_SPACE)))
         if(status_ok(status = quick_ublock_sbstr_add_n(&quick_ublock, p_fielddef->name, strlen_with_NULLCH)))
         {
             if(status_ok(status = ui_text_alloc_from_ustr(p_ui_text, quick_ublock_ustr(&quick_ublock))))
             {
                 const PIXIT width = ui_width_from_p_ui_text(p_ui_text);
                 *p_max_width = MAX(*p_max_width, width);
                 (*p_n_fields) += 1;
             }
         }

         quick_ublock_dispose(&quick_ublock);

         status_break(status);
     }

    return(status);
}

_Check_return_
extern STATUS
t5_cmd_db_sort(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    static const UI_TEXT caption = UI_TEXT_INIT_RESID(REC_MSG_SORT_TITLE);
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_db_id(p_docu, p_data_ref->arg.db_field.db_id);
    STATUS status;

    if(!p_rec_projector->opendb.dbok)
        return(create_error(REC_ERR_DATABASE_NOT_OPEN));

#if DPLIB
    if(dplib_is_db_on_server(&p_rec_projector->opendb))
        return(create_error(REC_ERR_SORT_ON_SERVER));
#endif

    status_return(table_check_access(&p_rec_projector->opendb.table, ACCESS_FUNCTION_SORT));

    /* I think we should do a view whole file here */
    status_return(status = rec_revert_whole(p_rec_projector));

    /* build the default null remap array for the database */
    rec_sort_array_handle = 0;
    status_assert(status = remap_hidden_to_end(&p_rec_projector->opendb.table, &rec_sort_array_handle));

    { /* make appropriate size box */
    PIXIT max_width =  ui_width_from_p_ui_text(&caption) + DIALOG_CAPTIONOVH_H; /* bare minimum */
    S32 show_elements;
    if(status_ok(status = sort_source_create(p_rec_projector, &rec_sort_fields, &max_width, &show_elements)))
    {
        PIXIT_SIZE list_size;
        DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
        dialog_cmd_ctl_size_estimate.p_dialog_control = &sort_list;
        dialog_cmd_ctl_size_estimate.p_dialog_control_data = &stdlisttext_data;
        ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
        dialog_cmd_ctl_size_estimate.size.x += max_width;
        ui_list_size_estimate(show_elements, &list_size);
        dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
        dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
        { /* don't compact buttons on top of each other! */
        PIXIT minheight = 4 * DIALOG_STDPUSHBUTTON_V + DIALOG_DEFPUSHBUTTON_V + 4 * DIALOG_STDSPACING_V;
        if( dialog_cmd_ctl_size_estimate.size.y < minheight)
            dialog_cmd_ctl_size_estimate.size.y = minheight;
        } /*block*/
        sort_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
        sort_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
    }
    } /*block*/

    if(status_ok(status))
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, sort_ctl_create, elemof32(sort_ctl_create), 0);
        dialog_cmd_process_dbox.caption = caption;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_sort;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_rec_projector;
        status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    }

    if(status_ok(status))
    {
        /* The ordering is kept up to date for us in the static remap array */

        /* Make a tempfile! MUST be on the same file-root as the database so that it can be renamed */
        QUICK_TBLOCK_WITH_BUFFER(aqtb_buffer, BUF_MAX_PATHSTRING);
        QUICK_TBLOCK_WITH_BUFFER(aqtb_tempname, BUF_MAX_PATHSTRING);
        quick_tblock_with_buffer_setup(aqtb_buffer);
        quick_tblock_with_buffer_setup(aqtb_tempname);

        status_assert(quick_tblock_tstr_add_n(&aqtb_buffer, p_rec_projector->opendb.db.name, strlen_with_NULLCH));

        file_dirname(quick_tblock_tchars_wr(&aqtb_buffer), p_rec_projector->opendb.db.name);

        if(status_ok(status = file_tempname(quick_tblock_tstr(&aqtb_buffer), "db", NULL, 0, &aqtb_tempname)))
        {
            /* do the sort */
            status = remap_sort(&p_rec_projector->opendb, &rec_sort_array_handle, quick_tblock_tstr(&aqtb_tempname));

            if(status_ok(status))
                status = switch_database(&p_rec_projector->opendb, quick_tblock_tstr(&aqtb_tempname));
        }

        quick_tblock_dispose(&aqtb_tempname);
        quick_tblock_dispose(&aqtb_buffer);

        rec_update_projector_adjust(p_rec_projector); /* err, how does a sort affect the number of records? */

        view_update_all(p_docu, UPDATE_PANE_CELLS_AREA);
    }

    ui_source_dispose(&rec_sort_fields);

    al_array_dispose(&rec_sort_array_handle);

    if(STATUS_OK == status)
        status = STATUS_DONE; /* in case we've been called from t5_cmd_sort_intro - return without giving standard sort dialog */

    return(status);
}

/* end of uisrch.c */
