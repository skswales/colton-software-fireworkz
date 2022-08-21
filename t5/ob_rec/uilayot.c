/* uilayot.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* layout/create/sort */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

#include "ob_skel/xp_skeld.h"

_Check_return_
extern STATUS
rec_fields_ui_source_create(
    /*out*/ P_UI_SOURCE p_ui_source,
    P_TABLEDEF p_tabledef,
    _InoutRef_  P_PIXIT p_max_width,
    _OutRef_    P_S32 p_n_fields)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX field;

    *p_n_fields = 0;

    p_ui_source->type = UI_SOURCE_TYPE_ARRAY;
    p_ui_source->source.array_handle = 0;

#if 0 /* PMF has ensured we can't get it right */
    { /* SKS 06nov95 loop over the visibles first, taking their order */
    ARRAY_INDEX frame;

    for(frame = 0; frame < array_elements(&p_rec_projector->h_rec_frames); frame++)
    {
        PC_REC_FRAME p_rec_frame = array_ptrc(&p_rec_projector->h_rec_frames, REC_FRAME, frame);
        PC_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->p_rec_projector->opendb.table.h_fielddefs, p_rec_frame->field_id);
        P_UI_TEXT p_ui_text;

        if(NULL == (p_ui_text = al_array_extend_by(&p_ui_source->source.array_handle, 1, &array_init_block_ui_text, &status)))
            break;

        status = ui_text_alloc_from_tstr(p_ui_text, _tstr_from_sbstr(p_fielddef->name));

        if(status_ok(status))
        {
            const PIXIT width = ui_width_from_p_ui_text(p_ui_text);
            *p_max_width = MAX(*p_max_width, width)
            (*p_n_fields) += 1;
        }
    }
    } /*block*/

    /* SKS 06nov95 loop over the invisibles last */
#endif

    for(field = 0; field < array_elements(&p_tabledef->h_fielddefs); ++field)
    {
        PC_FIELDDEF p_fielddef = array_ptrc(&p_tabledef->h_fielddefs, FIELDDEF, field);
        P_UI_TEXT p_ui_text;

#if 0
        if(!p_fielddef->hidden)
            continue;
#endif

        if(NULL == (p_ui_text = al_array_extend_by_UI_TEXT(&p_ui_source->source.array_handle, 1, &array_init_block_ui_text, &status)))
            break;

        status = ui_text_alloc_from_tstr(p_ui_text, _tstr_from_sbstr(p_fielddef->name));

        if(status_ok(status))
        {
            const PIXIT width = ui_width_from_p_ui_text(p_ui_text);
            *p_max_width = MAX(*p_max_width, width);
            (*p_n_fields) += 1;
        }

        status_break(status);
    }

    return(status);
}

extern S32
ensure_some_field_visible(
    P_TABLEDEF p_table)
{
    ARRAY_INDEX i = array_elements(&p_table->h_fielddefs);
    S32 fields_visible = 0;

    while(--i >= 0)
    {
        PC_FIELDDEF p_fielddef = array_ptrc(&p_table->h_fielddefs, FIELDDEF, i);

        if(!p_fielddef->hidden)
            ++fields_visible;
    }

    if(!fields_visible) /* unhide one of them then! */
    {
        fields_visible = 1;

        array_ptr(&p_table->h_fielddefs, FIELDDEF, 0)->hidden = FALSE;
    }

    return(fields_visible);
}

typedef struct LAYOUT_CALLBACK
{
    PC_DATA_REF p_data_ref;
    P_REC_PROJECTOR p_rec_projector;
    TABLEDEF fake_table;
    UI_SOURCE ui_source;

    /* i/o */
    PROJECTOR_TYPE type;
    S32 cols;
    S32 rows;
    BOOL adaptive_rows;
}
LAYOUT_CALLBACK, * P_LAYOUT_CALLBACK;

enum LAYOUT_CONTROL_ID
{
    LAYOUT_FIELDS_ID_GROUP  = 337,
    LAYOUT_FIELDS_ID_FIELDS,
    LAYOUT_FIELDS_ID_LIST,
    LAYOUT_FIELDS_ID_UP,
    LAYOUT_FIELDS_ID_DOWN,
    LAYOUT_FIELDS_ID_HIDDEN,
    LAYOUT_FIELDS_ID_TITLES,

    LAYOUT_ID_GROUP,
    LAYOUT_ID_CARD,
    LAYOUT_ID_SHEET,
    LAYOUT_ID_CTEXT,
    LAYOUT_ID_DTEXT,
    LAYOUT_ID_COLS,
    LAYOUT_ID_ROWS,
    LAYOUT_ID_AUTOROWS
};

/* A group box to contain the rest */

static const DIALOG_CONTROL
layout_fields_group =
{
    LAYOUT_FIELDS_ID_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, 0, 0 },
    { DRT(LTRB, GROUPBOX) }
};

/* A group box to contain the fields */

static const DIALOG_CONTROL
layout_fields_fields =
{
    LAYOUT_FIELDS_ID_FIELDS, LAYOUT_FIELDS_ID_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
layout_fields_fields_data = { UI_TEXT_INIT_RESID(REC_MSG_FIELDS_GROUP), { 0, 0, 0, FRAMED_BOX_GROUP } };

static /*poked*/ DIALOG_CONTROL
layout_fields_list =
{
    LAYOUT_FIELDS_ID_LIST, LAYOUT_FIELDS_ID_FIELDS,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, 0/*poked*/, 0/*poked*/ },
    { DRT(LTLT, LIST_TEXT), 1 }
};

static const DIALOG_CONTROL
layout_fields_up =
{
    LAYOUT_FIELDS_ID_UP, LAYOUT_FIELDS_ID_FIELDS,
    { LAYOUT_FIELDS_ID_LIST, LAYOUT_FIELDS_ID_LIST },
    { DIALOG_STDSPACING_H, 0, DIALOG_SYSCHARSL_H(10), DIALOG_STDPUSHBUTTON_V },
    { DRT(RTLT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
layout_fields_up_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_SORT_UP) };

static const DIALOG_CONTROL
layout_fields_down =
{
    LAYOUT_FIELDS_ID_DOWN, LAYOUT_FIELDS_ID_FIELDS,
    { LAYOUT_FIELDS_ID_UP, LAYOUT_FIELDS_ID_UP, LAYOUT_FIELDS_ID_UP },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
layout_fields_down_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_SORT_DOWN) };

static const DIALOG_CONTROL
layout_prop_hidden =
{
    LAYOUT_FIELDS_ID_HIDDEN, LAYOUT_FIELDS_ID_FIELDS,
    { LAYOUT_FIELDS_ID_DOWN, LAYOUT_FIELDS_ID_DOWN },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX) }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
layout_prop_hidden_data= { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_LAYOUT_HIDDEN) };

static const DIALOG_CONTROL
layout_titles =
{
    LAYOUT_FIELDS_ID_TITLES, LAYOUT_FIELDS_ID_FIELDS,
    { LAYOUT_FIELDS_ID_HIDDEN, LAYOUT_FIELDS_ID_HIDDEN },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX) }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
layout_titles_data= { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_LAYOUT_TITLES) };

/* A group box to contain the layout buttons */

static const DIALOG_CONTROL
layout_layout =
{
    LAYOUT_ID_GROUP, DIALOG_MAIN_GROUP,
    { LAYOUT_FIELDS_ID_GROUP, LAYOUT_FIELDS_ID_GROUP, LAYOUT_FIELDS_ID_GROUP, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
layout_layout_data = { UI_TEXT_INIT_RESID(REC_MSG_LAYOUT_CAPTION), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
layout_deep_text =
{
    LAYOUT_ID_DTEXT, LAYOUT_ID_GROUP,
    { DIALOG_CONTROL_PARENT, LAYOUT_ID_ROWS, DIALOG_CONTROL_SELF, LAYOUT_ID_ROWS },
    { DIALOG_STDGROUP_LM, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
layout_deep_text_data = { UI_TEXT_INIT_RESID(REC_MSG_TEXT_DEEP), { 0 /*left_text*/ } };

static const DIALOG_CONTROL
layout_deep =
{
    LAYOUT_ID_ROWS, LAYOUT_ID_GROUP,
    { LAYOUT_ID_DTEXT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDSPACING_H, DIALOG_STDGROUP_TM, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_S32), 1 }
};

static const UI_CONTROL_S32
layout_deep_control = { 1, 1000 };

static DIALOG_CONTROL_DATA_BUMP_S32
layout_deep_data = { { { { FRAMED_BOX_EDIT } }, &layout_deep_control }, 3 };

static const DIALOG_CONTROL
layout_auto_rows =
{
    LAYOUT_ID_AUTOROWS, LAYOUT_ID_GROUP,
    { LAYOUT_ID_ROWS, LAYOUT_ID_ROWS, DIALOG_CONTROL_SELF, LAYOUT_ID_ROWS },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, CHECKBOX) }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
layout_auto_rows_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_AUTO_ROWS) };

static const DIALOG_CONTROL
layout_sheet =
{
    LAYOUT_ID_SHEET, LAYOUT_ID_GROUP,
    { LAYOUT_ID_DTEXT, LAYOUT_ID_DTEXT },
    { 0,  DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
layout_sheet_data = { { 0 }, LAYOUT_ID_SHEET, UI_TEXT_INIT_RESID(REC_MSG_LAYOUT_SHEET) };

static const DIALOG_CONTROL
layout_card =
{
    LAYOUT_ID_CARD, LAYOUT_ID_GROUP,
    { LAYOUT_ID_SHEET, LAYOUT_ID_SHEET, LAYOUT_ID_SHEET },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDRADIO_V },
    { DRT(LBRT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
layout_card_data = { { 0 }, LAYOUT_ID_CARD, UI_TEXT_INIT_RESID(REC_MSG_LAYOUT_CARD) };

static const DIALOG_CONTROL
layout_across_text =
{
    LAYOUT_ID_CTEXT, LAYOUT_ID_GROUP,
    { LAYOUT_ID_DTEXT, LAYOUT_ID_COLS, DIALOG_CONTROL_SELF, LAYOUT_ID_COLS },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
layout_across_text_data = { UI_TEXT_INIT_RESID(REC_MSG_TEXT_ACROSS), { 0 /*left_text*/ } };

static const DIALOG_CONTROL
layout_across =
{
    LAYOUT_ID_COLS, LAYOUT_ID_GROUP,
    { LAYOUT_ID_CTEXT, LAYOUT_ID_CARD },
    { DIALOG_STDSPACING_H, DIALOG_STDSPACING_V, DIALOG_BUMP_H(2), DIALOG_STDBUMP_V },
    { DRT(RBLT, BUMP_S32), 1 }
};

static const UI_CONTROL_S32
layout_across_control = { 1, 1000 };

static DIALOG_CONTROL_DATA_BUMP_S32
layout_across_data = { { { { FRAMED_BOX_EDIT } }, &layout_across_control }, 3 };

static const DIALOG_CTL_CREATE
layout_ctl_create[] =
{
    { &dialog_main_group },

    { &layout_fields_group,  NULL                       },
    { &layout_fields_fields, &layout_fields_fields_data },
    { &layout_fields_list,   &stdlisttext_data_dd       },

    { &layout_fields_up,     &layout_fields_up_data     },
    { &layout_fields_down,   &layout_fields_down_data   },
    { &layout_prop_hidden,   &layout_prop_hidden_data   },
    { &layout_titles,        &layout_titles_data        },

    { &layout_layout,        &layout_layout_data        },
    { &layout_deep_text,     &layout_deep_text_data     },
    { &layout_deep,          &layout_deep_data          },
    { &layout_auto_rows,     &layout_auto_rows_data     },
    { &layout_sheet,         &layout_sheet_data         },
    { &layout_card,          &layout_card_data          },
    { &layout_across_text,   &layout_across_text_data   },
    { &layout_across,        &layout_across_data        },

    { &stdbutton_cancel,     &stdbutton_cancel_data     },
    { &defbutton_ok,         &defbutton_ok_data         }
};

_Check_return_
static STATUS
layout_list_updown(
    _InVal_     H_DIALOG h_dialog,
    P_LAYOUT_CALLBACK p_layout_callback,
    _In_        S32 delta)
{
    STATUS status = STATUS_OK;
    S32 itemno = ui_dlg_get_list_idx(h_dialog, LAYOUT_FIELDS_ID_LIST);
    S32 itemno_m_delta = itemno - delta;
    S32 n = array_elements(&p_layout_callback->ui_source.source.array_handle);

    if(((U32) itemno < (U32) n) && ((U32) itemno_m_delta < (U32) n))
    {
        memswap32(array_ptr(&p_layout_callback->fake_table.h_fielddefs, FIELDDEF, itemno_m_delta),
                  array_ptr(&p_layout_callback->fake_table.h_fielddefs, FIELDDEF, itemno),
                  sizeof32(FIELDDEF));

        memswap32(array_ptr(&p_layout_callback->ui_source.source.array_handle, UI_TEXT, itemno_m_delta),
                  array_ptr(&p_layout_callback->ui_source.source.array_handle, UI_TEXT, itemno),
                  sizeof32(UI_TEXT));

        ui_dlg_ctl_new_source(h_dialog, LAYOUT_FIELDS_ID_LIST);

        ui_dlg_set_list_idx(h_dialog, LAYOUT_FIELDS_ID_LIST, itemno_m_delta); /* track the item */
    }

    return(status);
}

_Check_return_
static STATUS
dialog_layout_msg_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    const P_LAYOUT_CALLBACK p_layout_callback = (P_LAYOUT_CALLBACK) p_dialog_msg_ctl_fill_source->client_handle;

    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case LAYOUT_FIELDS_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &p_layout_callback->ui_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_layout_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_LAYOUT_CALLBACK p_layout_callback = (P_LAYOUT_CALLBACK) p_dialog_msg_process_start->client_handle;
    const H_DIALOG h_dialog = p_dialog_msg_process_start->h_dialog;
    const P_DOCU p_docu = p_docu_from_docno(p_layout_callback->p_rec_projector->docno);
    S32 itemno = -1;

    status_return(ui_dlg_set_radio(h_dialog, LAYOUT_ID_GROUP, (PROJECTOR_TYPE_CARD == p_layout_callback->type) ? LAYOUT_ID_CARD : LAYOUT_ID_SHEET));
    status_return(ui_dlg_set_s32(h_dialog, LAYOUT_ID_COLS, p_layout_callback->cols));
    status_return(ui_dlg_set_s32(h_dialog, LAYOUT_ID_ROWS, p_layout_callback->rows));
    status_return(ui_dlg_set_check(h_dialog, LAYOUT_ID_AUTOROWS, p_layout_callback->adaptive_rows));

    /* let's try selecting the current field */
    if(PROJECTOR_TYPE_CARD == p_layout_callback->type)
    {
        DATA_REF data_ref = *p_layout_callback->p_data_ref;

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
        S32 current_field_number = fieldnumber_from_field_id(&p_layout_callback->p_rec_projector->opendb.table, field_id_from_rec_data_ref(p_layout_callback->p_data_ref));
        if(status_ok(current_field_number))
            itemno = current_field_number - 1;
    }

    return(ui_dlg_set_list_idx(h_dialog, LAYOUT_FIELDS_ID_LIST, (itemno < 0) ? 0 : itemno));
}

_Check_return_
static STATUS
dialog_layout_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_LAYOUT_CALLBACK p_layout_callback = (P_LAYOUT_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case LAYOUT_FIELDS_ID_LIST:
        {
        S32 itemno = p_dialog_msg_ctl_state_change->new_state.list_text.itemno;

        if(array_index_valid(&p_layout_callback->fake_table.h_fielddefs, itemno))
        {
            PC_FIELDDEF p_fielddef = array_ptrc_no_checks(&p_layout_callback->fake_table.h_fielddefs, FIELDDEF, itemno);
            PC_REC_FRAME p_rec_frame = p_rec_frame_from_field_id(&p_layout_callback->p_rec_projector->h_rec_frames, p_fielddef->id);

            ui_dlg_set_check(p_dialog_msg_ctl_state_change->h_dialog, LAYOUT_FIELDS_ID_HIDDEN, p_fielddef->hidden);

            if(P_DATA_NONE != p_rec_frame)
                ui_dlg_set_check(p_dialog_msg_ctl_state_change->h_dialog, LAYOUT_FIELDS_ID_TITLES, p_rec_frame->title_show);
        }

        break;
        }

    case LAYOUT_FIELDS_ID_HIDDEN:
        {
        S32 itemno = ui_dlg_get_list_idx(p_dialog_msg_ctl_state_change->h_dialog, LAYOUT_FIELDS_ID_LIST);

        if(array_index_valid(&p_layout_callback->fake_table.h_fielddefs, itemno))
        {
            P_FIELDDEF p_fielddef = array_ptr(&p_layout_callback->fake_table.h_fielddefs, FIELDDEF, itemno);

            p_fielddef->hidden = p_dialog_msg_ctl_state_change->new_state.checkbox;
        }

        break;
        }

    case LAYOUT_FIELDS_ID_TITLES:
        {
        S32 itemno = ui_dlg_get_list_idx(p_dialog_msg_ctl_state_change->h_dialog, LAYOUT_FIELDS_ID_LIST);

        if(array_index_valid(&p_layout_callback->fake_table.h_fielddefs, itemno))
        {
            P_FIELDDEF p_fielddef = array_ptr_no_checks(&p_layout_callback->fake_table.h_fielddefs, FIELDDEF, itemno);
            P_REC_FRAME p_rec_frame = p_rec_frame_from_field_id(&p_layout_callback->p_rec_projector->h_rec_frames, p_fielddef->id);

            if(P_DATA_NONE != p_rec_frame)
                p_rec_frame->title_show = p_dialog_msg_ctl_state_change->new_state.checkbox;
        }

        break;
        }

    case LAYOUT_ID_AUTOROWS:
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, LAYOUT_ID_ROWS, !ui_dlg_get_check(p_dialog_msg_ctl_state_change->h_dialog, LAYOUT_ID_AUTOROWS));
        break;

    case LAYOUT_ID_GROUP:
        {
        /* When you change card->sheet the number of cols is not relevant */
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, LAYOUT_ID_COLS, (ui_dlg_get_radio(p_dialog_msg_ctl_state_change->h_dialog, LAYOUT_ID_GROUP) ==  LAYOUT_ID_CARD ) );

        { /* When changing from sheet->card the number of cols could do with setting to a sensible value */
        S32 cols;

        if(ui_dlg_get_radio(p_dialog_msg_ctl_state_change->h_dialog, LAYOUT_ID_GROUP) ==  LAYOUT_ID_CARD)
            cols = (PROJECTOR_TYPE_CARD == p_layout_callback->type) ? p_layout_callback->cols : 1;
        else
            cols = array_elements(&p_layout_callback->fake_table.h_fielddefs);

        ui_dlg_set_s32(p_dialog_msg_ctl_state_change->h_dialog, LAYOUT_ID_COLS, cols);
        } /*block*/

        /* When you change card->sheet the number of cols is not relevant */
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, LAYOUT_ID_COLS, (ui_dlg_get_radio(p_dialog_msg_ctl_state_change->h_dialog, LAYOUT_ID_GROUP) ==  LAYOUT_ID_CARD ) );

        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_layout_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    STATUS status = STATUS_OK;

    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case LAYOUT_FIELDS_ID_UP:
    case LAYOUT_FIELDS_ID_DOWN:
        {
        S32 delta = 1;

        if(p_dialog_msg_ctl_pushbutton->right_button)
            delta = -delta;

        if(p_dialog_msg_ctl_pushbutton->dialog_control_id == LAYOUT_FIELDS_ID_DOWN)
            delta = -delta;

        status = layout_list_updown(p_dialog_msg_ctl_pushbutton->h_dialog, (P_LAYOUT_CALLBACK) p_dialog_msg_ctl_pushbutton->client_handle, delta);

        p_dialog_msg_ctl_pushbutton->processed = 1;

        break;
        }

    default:
        break;
    }

    return(status);
}

_Check_return_
static STATUS
dialog_layout_msg_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_LAYOUT_CALLBACK p_layout_callback = (P_LAYOUT_CALLBACK) p_dialog_msg_process_end->client_handle;

        p_layout_callback->type = (ui_dlg_get_radio(p_dialog_msg_process_end->h_dialog, LAYOUT_ID_GROUP) == LAYOUT_ID_SHEET) ? PROJECTOR_TYPE_SHEET : PROJECTOR_TYPE_CARD;
        p_layout_callback->cols = ui_dlg_get_s32(p_dialog_msg_process_end->h_dialog, LAYOUT_ID_COLS);
        p_layout_callback->rows = ui_dlg_get_s32(p_dialog_msg_process_end->h_dialog, LAYOUT_ID_ROWS);
        p_layout_callback->adaptive_rows = ui_dlg_get_check(p_dialog_msg_process_end->h_dialog, LAYOUT_ID_AUTOROWS);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_layout)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_layout_msg_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_layout_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_layout_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_layout_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_layout_msg_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
t5_cmd_db_layout(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    static const UI_TEXT caption = UI_TEXT_INIT_RESID(REC_MSG_LAYOUT_CAPTION);
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_db_id(p_docu, p_data_ref->arg.db_field.db_id);
    BOOL was_a_sheet = (p_rec_projector->projector_type == PROJECTOR_TYPE_SHEET);
    LAYOUT_CALLBACK layout_callback;
    STATUS status;

    status_return(table_check_access(&p_rec_projector->opendb.table, ACCESS_FUNCTION_LAYOUT));

    if(was_a_sheet)
        rec_sheet_stash_field_widths(p_rec_projector);

    layout_callback.p_data_ref = p_data_ref;
    layout_callback.p_rec_projector = p_rec_projector;
    layout_callback.fake_table = p_rec_projector->opendb.table;

    { /* make appropriate size box */
    PIXIT max_width = ui_width_from_p_ui_text(&caption) + DIALOG_CAPTIONOVH_H; /* bare minimum */
    S32 show_elements;
    if(status_ok(status = rec_fields_ui_source_create(&layout_callback.ui_source, &layout_callback.fake_table, &max_width, &show_elements)))
    {
        PIXIT_SIZE list_size;
        DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
        dialog_cmd_ctl_size_estimate.p_dialog_control = &layout_fields_list;
        dialog_cmd_ctl_size_estimate.p_dialog_control_data = &stdlisttext_data_dd;
        ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
        dialog_cmd_ctl_size_estimate.size.x += max_width;
        ui_list_size_estimate(show_elements, &list_size);
        dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
        dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
        layout_fields_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
        layout_fields_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
    }
    } /*block*/

    layout_callback.type = p_rec_projector->projector_type;
    layout_callback.cols = p_rec_projector->rec_docu_area.br.slr.col - p_rec_projector->rec_docu_area.tl.slr.col;
    layout_callback.rows = p_rec_projector->rec_docu_area.br.slr.row - p_rec_projector->rec_docu_area.tl.slr.row;
    layout_callback.adaptive_rows = p_rec_projector->adaptive_rows;

    if(status_ok(status))
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, layout_ctl_create, elemof32(layout_ctl_create), 0);
        dialog_cmd_process_dbox.caption = caption;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &layout_callback;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_layout;
        status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    }

    if(status_ok(status))
    {
        drop_projector_area(p_rec_projector);

        if(layout_callback.type == PROJECTOR_TYPE_SHEET)
            layout_callback.cols = ensure_some_field_visible(&layout_callback.fake_table);

        p_rec_projector->projector_type = layout_callback.type;
        p_rec_projector->rec_docu_area.br.slr.col = p_rec_projector->rec_docu_area.tl.slr.col + (COL) layout_callback.cols;
        p_rec_projector->rec_docu_area.br.slr.row = p_rec_projector->rec_docu_area.tl.slr.row + layout_callback.rows;
        p_rec_projector->adaptive_rows = layout_callback.adaptive_rows;

        /* Just use the newly modified table. Phew! */
        p_rec_projector->opendb.table = layout_callback.fake_table;

        if(status_ok(status = reconstruct_frames_from_fields(p_rec_projector)))
        {
            if(status_ok(status = rec_insert_projector_and_attach_with_styles(p_rec_projector, FALSE)))
            {
                if(was_a_sheet)
                {
                    if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
                    {
                        DATA_REF data_ref;

                        /* Try to get the object position in p_docu->cur sorted out */
                        if(rec_data_ref_from_slr(p_docu, &p_rec_projector->rec_docu_area.tl.slr, &data_ref))
                        {
                            p_docu->cur.object_position.object_id = OBJECT_ID_REC;

                            set_rec_object_position_from_data_ref(p_docu, &p_docu->cur.object_position, &data_ref);
                        }
                    }
                }
            }
        }
    }

    if(STATUS_CANCEL != status)
        /* SKS 04apr95 - do large reformat on layout change. 06may95 maybe now at right place? */
        reformat_from_row(p_docu, p_rec_projector->rec_docu_area.tl.slr.row, REFORMAT_Y);

    ui_source_dispose(&layout_callback.ui_source);

    return(status);
}

/* Create DatabaseDialog box set up */

enum CREATE_CONTROL_IDS
{
    CREATE_ID_FIELDS = 337,
    CREATE_ID_SAVE,
    CREATE_ID_EDIT,
    CREATE_ID_LIST,
    CREATE_ID_ADDONE,
    CREATE_ID_REMOVE,
    CREATE_ID_PROP,
    CREATE_ID_UP,
    CREATE_ID_DOWN,
    CREATE_ID_PICT,
    CREATE_ID_NAME,
    CREATE_ID_ADD
};

#define CREATE_ID_CREATE IDOK

/* Define some controls in a dialog box for obtaining a search request
 */

/* A group box to contain the fields */

static const DIALOG_CONTROL
create_fields =
{
    CREATE_ID_FIELDS, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
create_fields_data = { UI_TEXT_INIT_RESID(REC_MSG_FIELDS_GROUP), { 0, 0, 0, FRAMED_BOX_GROUP } };

/* a writable icon */

static const DIALOG_CONTROL
create_edit =
{
    CREATE_ID_EDIT, CREATE_ID_FIELDS,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_SYSCHARSL_H(32), DIALOG_STDEDIT_V },
    { DRT(LTLT, EDIT), 1 }
};

static const DIALOG_CONTROL_DATA_EDIT
create_edit_data= { { { FRAMED_BOX_EDIT } }, { UI_TEXT_TYPE_NONE } };

static const DIALOG_CONTROL
create_list =
{
    CREATE_ID_LIST, CREATE_ID_FIELDS,
    { DIALOG_CONTROL_PARENT, CREATE_ID_EDIT, CREATE_ID_EDIT },
    { DIALOG_STDGROUP_LM, DIALOG_STDSPACING_V, 0, 10*DIALOG_STDLISTITEM_V + DIALOG_STDLISTOVH_V },
    { DRT(LBRT, LIST_TEXT), 1 }
};

static const DIALOG_CONTROL
create_add =
{
    CREATE_ID_ADD, CREATE_ID_FIELDS,
    { CREATE_ID_EDIT, CREATE_ID_EDIT },
    { DIALOG_STDSPACING_H, 0, DIALOG_SYSCHARSL_H(10), DIALOG_DEFPUSHBUTTON_V },
    { DRT(RTLT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
create_add_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_CREATE_ADD) };

static const DIALOG_CONTROL
create_remove =
{
    CREATE_ID_REMOVE, CREATE_ID_FIELDS,
    { CREATE_ID_ADD, CREATE_ID_ADD, CREATE_ID_ADD },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
create_remove_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_CREATE_REMOVE) };

static const DIALOG_CONTROL
create_up =
{
    CREATE_ID_UP, CREATE_ID_FIELDS,
    { CREATE_ID_REMOVE, CREATE_ID_REMOVE, CREATE_ID_REMOVE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
create_up_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_SORT_UP) };

static const DIALOG_CONTROL
create_down =
{
    CREATE_ID_DOWN, CREATE_ID_FIELDS,
    { CREATE_ID_UP, CREATE_ID_UP, CREATE_ID_UP },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
create_down_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_SORT_DOWN) };

static const DIALOG_CONTROL
create_ok =
{
    CREATE_ID_CREATE, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, CREATE_ID_FIELDS, CREATE_ID_FIELDS, DIALOG_CONTROL_SELF },
    { DIALOG_CONTENTS_CALC, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 }
};

/* A Cancel button */

static const DIALOG_CONTROL
create_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, CREATE_ID_CREATE, CREATE_ID_CREATE, DIALOG_CONTROL_SELF },
    { DIALOG_CONTENTS_CALC, -DIALOG_DEFPUSHEXTRA_V, DIALOG_STDSPACING_H, DIALOG_STDPUSHBUTTON_V },
    { DRT(RTLT, PUSHBUTTON), 1 }
};

static const DIALOG_CTL_CREATE
create_ctl_create[] =
{
    { &dialog_main_group },
    { &create_fields, &create_fields_data },
    { &create_edit,   &create_edit_data   },
    { &create_list,   &stdlisttext_data_dd_vsc },
    { &create_add,    &create_add_data    },

    { &create_remove, &create_remove_data },
    { &create_up,     &create_up_data     },
    { &create_down,   &create_down_data   },

    { &create_cancel, &stdbutton_cancel_data },
    { &create_ok,     &defbutton_ok_data }
};

static TABLEDEF rec_creation_fake_table;

static UI_SOURCE rec_creation_fields;

_Check_return_
static STATUS
rec_create_field_updown(
    _InVal_     H_DIALOG h_dialog,
    _In_        S32 delta)
{
    S32 itemno = ui_dlg_get_list_idx(h_dialog, CREATE_ID_LIST);
    S32 itemno_m_delta = itemno - delta;
    S32 n = array_elements(&rec_creation_fields.source.array_handle);

    if(((U32) itemno < (U32) n) && ((U32) itemno_m_delta < (U32) n))
    {
        memswap32(array_ptr(&rec_creation_fake_table.h_fielddefs, FIELDDEF, itemno_m_delta),
                  array_ptr(&rec_creation_fake_table.h_fielddefs, FIELDDEF, itemno),
                  sizeof32(FIELDDEF));

        memswap32(array_ptr(&rec_creation_fields.source.array_handle, UI_TEXT, itemno_m_delta),
                  array_ptr(&rec_creation_fields.source.array_handle, UI_TEXT, itemno),
                  sizeof32(UI_TEXT));

        ui_dlg_ctl_new_source(h_dialog, CREATE_ID_LIST);

        ui_dlg_set_list_idx(h_dialog, CREATE_ID_LIST, itemno_m_delta); /* track the item */
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
rec_create_field_add(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    UI_TEXT ui_text;
    PCTSTR tstr;

    /* Add the contents of the edit control to the list box */
    ui_dlg_get_edit(p_dialog_msg_ctl_pushbutton->h_dialog, CREATE_ID_EDIT, &ui_text);

    tstr = ui_text_tstr_no_default(&ui_text);

    if((NULL == tstr) || (0 == tstrlen32(tstr)))
        return(create_error(REC_ERR_FIELD_NAME_BLANK));

    if(P_DATA_NONE != p_fielddef_from_name(&rec_creation_fake_table.h_fielddefs, tstr))
        return(create_error(REC_ERR_ALREADY_A_FIELD));

    /* Add the named field to the tabledef */
    status_return(add_new_field_to_table(&rec_creation_fake_table, tstr, FIELD_TEXT));

    if(status_fail(al_array_add(&rec_creation_fields.source.array_handle, UI_TEXT, 1, &array_init_block_ui_text, &ui_text)))
    {
        ui_text_dispose(&ui_text);
        return(status_nomem());
    }
    /* stolen the answer now */

    /* Flush the edit control */
    status_assert(ui_dlg_set_edit(p_dialog_msg_ctl_pushbutton->h_dialog, CREATE_ID_EDIT, P_DATA_NONE));

    ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, CREATE_ID_LIST);

    /* Activate the new item in the list box */
    ui_dlg_set_list_idx(p_dialog_msg_ctl_pushbutton->h_dialog, CREATE_ID_LIST, array_elements(&rec_creation_fields.source.array_handle)-1);

    ui_dlg_ctl_enable(p_dialog_msg_ctl_pushbutton->h_dialog, CREATE_ID_CREATE, array_elements(&rec_creation_fields.source.array_handle) > 0);

    ui_dlg_ctl_set_default(p_dialog_msg_ctl_pushbutton->h_dialog, CREATE_ID_CREATE);

    p_dialog_msg_ctl_pushbutton->processed = 1;

    return(STATUS_OK);
}

/* If there is a selection in the list box then remove it */

_Check_return_
static STATUS
rec_create_field_remove(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    S32 itemno = ui_dlg_get_list_idx(p_dialog_msg_ctl_pushbutton->h_dialog, CREATE_ID_LIST);

    if(array_index_valid(&rec_creation_fields.source.array_handle, itemno))
    {
        /* remove the named field aus der tabledef */
        remove_field_from_table(&rec_creation_fake_table, itemno);

        /* remove the named field aus der list box source */
        ui_text_dispose(array_ptr(&rec_creation_fields.source.array_handle, UI_TEXT, itemno));

        al_array_delete_at(&rec_creation_fields.source.array_handle, -1, itemno);

        ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, CREATE_ID_LIST);

        ui_dlg_ctl_enable(p_dialog_msg_ctl_pushbutton->h_dialog, CREATE_ID_CREATE, array_elements(&rec_creation_fields.source.array_handle) > 0);
    }

    p_dialog_msg_ctl_pushbutton->processed = 1;

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_create_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case CREATE_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &rec_creation_fields;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_create_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, CREATE_ID_ADD, FALSE);
    ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, CREATE_ID_CREATE, FALSE);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_create_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case CREATE_ID_EDIT:
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, CREATE_ID_ADD, !ui_text_is_blank(&p_dialog_msg_ctl_state_change->new_state.edit.ui_text));
        ui_dlg_ctl_set_default(p_dialog_msg_ctl_state_change->h_dialog, CREATE_ID_ADD);
        break;

    case CREATE_ID_LIST:
        {
        S32 itemno = p_dialog_msg_ctl_state_change->new_state.list_text.itemno;
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, CREATE_ID_UP,     itemno >= 0);
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, CREATE_ID_DOWN,   itemno >= 0);
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, CREATE_ID_REMOVE, itemno >= 0);
        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_create_ctl_pushbutton(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    STATUS status = STATUS_OK;

    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case CREATE_ID_CREATE:
        {
        const P_QUICK_TBLOCK p_quick_tblock = (P_QUICK_TBLOCK) p_dialog_msg_ctl_pushbutton->client_handle;

        status = file_derive_name(p_docu->docu_name.path_name, p_docu->docu_name.leaf_name, "_b", p_quick_tblock, 0 /*REC_ERR_CANT_CREATE_DB_ISAFILE*/);
        if(FILE_ERR_ISAFILE == status)
        { /* SKS 28jul95 have another go! */
            quick_tblock_dispose(p_quick_tblock);
            status = file_tempname(p_docu->docu_name.path_name, p_docu->docu_name.leaf_name, NULL, FILE_TEMPNAME_INITIAL_TRY, p_quick_tblock);
        }
        if(status_ok(status))
        {
            PCTSTR save_name = quick_tblock_tstr(p_quick_tblock);

            tstr_xstrkpy(rec_creation_fake_table.name, elemof32(rec_creation_fake_table.name), file_leafname(save_name));

#if DPLIB
            status = dplib_create_from_prototype(save_name, &rec_creation_fake_table);
#else
            status = create_error(ERR_NYI);
#endif

            if(status_fail(status))
                reperr(status, save_name);
            else
            {
                DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
                msgclr(dialog_cmd_complete_dbox);
                dialog_cmd_complete_dbox.h_dialog = p_dialog_msg_ctl_pushbutton->h_dialog;
                dialog_cmd_complete_dbox.completion_code = DIALOG_COMPLETION_OK;
                status_assert(call_dialog(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox));
            }
        }

        break;
        }

    case CREATE_ID_UP:
    case CREATE_ID_DOWN:
        {
        S32 delta = p_dialog_msg_ctl_pushbutton->right_button ? -1 : +1;

        if(p_dialog_msg_ctl_pushbutton->dialog_control_id == CREATE_ID_DOWN)
            delta = -delta;

        status = rec_create_field_updown(p_dialog_msg_ctl_pushbutton->h_dialog, delta);

        p_dialog_msg_ctl_pushbutton->processed = 1;

        break;
        }

    case CREATE_ID_ADD:
        return(rec_create_field_add(p_dialog_msg_ctl_pushbutton));

    case CREATE_ID_REMOVE:
        return(rec_create_field_remove(p_dialog_msg_ctl_pushbutton));
    }

    return(status);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_create)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_create_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
         return(dialog_create_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
         return(dialog_create_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_create_ctl_pushbutton(p_docu, (P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
t5_cmd_db_create(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 16);
    quick_tblock_with_buffer_setup(quick_tblock);

    if(NULL == p_docu->docu_name.path_name)
        return(create_error(REC_ERR_DOC_NOT_SAVED));

    rec_creation_fields.type = UI_SOURCE_TYPE_NONE;

    status_return(al_array_preallocate_zero(&rec_creation_fields.source.array_handle, &array_init_block_ui_text));

    rec_creation_fields.type = UI_SOURCE_TYPE_ARRAY;

    make_fake_table(&rec_creation_fake_table);

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, create_ctl_create, elemof32(create_ctl_create), 0);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = REC_MSG_CREATE_TITLE;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &quick_tblock;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_create;
    status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    ui_source_dispose(&rec_creation_fields);

    if(status_ok(status))
    {
        /* Open it just as if we'd dragged it in */
        MSG_INSERT_FOREIGN msg_insert_foreign;
        zero_struct(msg_insert_foreign);

        { /* as this is abused to create stuff we do nasty things */
        BOOL temp = p_docu->flags.allow_modified_change;
        p_docu->flags.allow_modified_change = 1;
        docu_modify(p_docu);
        p_docu->flags.allow_modified_change = temp;
        } /*block*/

        /* no loading bodge - we're doing a database create */

        cur_change_before(p_docu);

        msg_insert_foreign.filename = quick_tblock_tstr(&quick_tblock);
        msg_insert_foreign.t5_filetype = FILETYPE_DATAPOWER;
        msg_insert_foreign.insert = TRUE;
        /***msg_insert_foreign.of_ip_format.flags.insert = 1;*/
        msg_insert_foreign.position = p_docu->cur;
        skel_point_from_slr_tl(p_docu, &msg_insert_foreign.skel_point, &p_docu->cur.slr);
        status = object_call_id_load(p_docu, T5_MSG_INSERT_FOREIGN, &msg_insert_foreign, OBJECT_ID_REC);

        cur_change_after(p_docu);

        if(status_ok(status))
            status = rec_add_record(p_docu, TRUE); /* Insist there be at least one (blank) record - but in which database! */

        if(status_ok(status))
            status = auto_save(p_docu, -1); /* force the document out to disc */
    }

    quick_tblock_dispose(&quick_tblock);

    return(status);
}

/* end of uilayot.c */
