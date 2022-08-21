/* uiprops.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

#include "ob_skel/xp_skeld.h"

#if RISCOS
#include "ob_dlg/xp_dlgr.h"
#endif

enum REC_PROPERTIES_CONTROL_IDS
{
    PROP_ID_FIELD_FIELDS = 336,
    PROP_ID_FIELD_EDIT,
    PROP_ID_FIELD_LIST,
    PROP_ID_FIELD_ADD,
    PROP_ID_FIELD_REMOVE,
    PROP_ID_FIELD_UP,
    PROP_ID_FIELD_DOWN,
    PROP_ID_FIELD_RENAME,

    PROP_ID_PROP_GROUP,
    PROP_ID_PROP_CAPTION_GROUP,
    PROP_ID_PROP_TYPE_CAPTION,
    PROP_ID_PROP_TYPE_COMBO,
    PROP_ID_PROP_DEFAULT_CAPTION,
    PROP_ID_PROP_DEFAULT_EDIT,
    PROP_ID_PROP_CHECK_CAPTION,
    PROP_ID_PROP_CHECK_EDIT,
    PROP_ID_PROP_VALUES_CAPTION,
    PROP_ID_PROP_VALUES_EDIT,
    PROP_ID_PROP_VALUES_LIST,
    PROP_ID_PROP_VALUES_ADD,
    PROP_ID_PROP_VALUES_REMOVE,
    PROP_ID_PROP_VALUES_PROP,
    PROP_ID_PROP_VALUES_UP,
    PROP_ID_PROP_VALUES_DOWN,
    PROP_ID_PROP_COMPULSORY
};

/* A group box to contain the fields */

static const DIALOG_CONTROL
prop_field_fields =
{
    PROP_ID_FIELD_FIELDS, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
prop_field_fields_data = { UI_TEXT_INIT_RESID(REC_MSG_FIELDS_GROUP), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
prop_field_edit =
{
    PROP_ID_FIELD_EDIT, PROP_ID_FIELD_FIELDS,
    { PROP_ID_FIELD_LIST, DIALOG_CONTROL_PARENT, PROP_ID_FIELD_LIST },
    { 0, DIALOG_STDGROUP_TM, 0, DIALOG_STDEDIT_V },
    { DRT(LTRT, EDIT) }
};

static const DIALOG_CONTROL_DATA_EDIT
prop_field_edit_data= { { { FRAMED_BOX_EDIT } }, { UI_TEXT_TYPE_NONE } };

static /*poked*/ DIALOG_CONTROL
prop_field_list =
{
    PROP_ID_FIELD_LIST, PROP_ID_FIELD_FIELDS,
    { DIALOG_CONTROL_PARENT, PROP_ID_FIELD_EDIT },
    { DIALOG_STDGROUP_LM, DIALOG_STDSPACING_V, 0/*poked*/, 0/*poked*/ },
    { DRT(LBLT, LIST_TEXT), 1 }
};

static const DIALOG_CONTROL
prop_field_add =
{
    PROP_ID_FIELD_ADD, PROP_ID_FIELD_FIELDS,
    { PROP_ID_FIELD_EDIT, PROP_ID_FIELD_EDIT, DIALOG_CONTROL_SELF, PROP_ID_FIELD_EDIT },
    { DIALOG_STDSPACING_H, 0, DIALOG_SYSCHARSL_H(10), 0 },
    { DRT(RTLB, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
prop_field_add_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_CREATE_ADD) };

static const DIALOG_CONTROL
prop_field_remove =
{
    PROP_ID_FIELD_REMOVE, PROP_ID_FIELD_FIELDS,
    { PROP_ID_FIELD_ADD, PROP_ID_FIELD_ADD, PROP_ID_FIELD_ADD },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
prop_field_remove_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_CREATE_REMOVE) };

static const DIALOG_CONTROL
prop_field_up =
{
    PROP_ID_FIELD_UP, PROP_ID_FIELD_FIELDS,
    { PROP_ID_FIELD_REMOVE, PROP_ID_FIELD_REMOVE, PROP_ID_FIELD_REMOVE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
prop_field_up_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_SORT_UP) };

static const DIALOG_CONTROL
prop_field_down =
{
    PROP_ID_FIELD_DOWN, PROP_ID_FIELD_FIELDS,
    { PROP_ID_FIELD_UP, PROP_ID_FIELD_UP, PROP_ID_FIELD_UP },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
prop_field_down_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_SORT_DOWN) };

static const DIALOG_CONTROL
prop_field_rename =
{
    PROP_ID_FIELD_RENAME, PROP_ID_FIELD_FIELDS,
    { PROP_ID_FIELD_DOWN, PROP_ID_FIELD_DOWN, PROP_ID_FIELD_DOWN },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
prop_field_rename_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_SORT_RENAME) };

/* A group box to contain the fields */

static const DIALOG_CONTROL
prop_prop_group =
{
    PROP_ID_PROP_GROUP, DIALOG_MAIN_GROUP,
    { PROP_ID_FIELD_FIELDS, PROP_ID_FIELD_FIELDS, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDSPACING_H, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
prop_prop_group_data = { UI_TEXT_INIT_RESID(REC_MSG_PROP_GROUP), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
prop_prop_caption_group =
{
    PROP_ID_PROP_CAPTION_GROUP, PROP_ID_PROP_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0 },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL
prop_prop_field_type =
{
    PROP_ID_PROP_TYPE_CAPTION, PROP_ID_PROP_CAPTION_GROUP,
    { DIALOG_CONTROL_PARENT, PROP_ID_PROP_TYPE_COMBO, DIALOG_CONTROL_SELF, PROP_ID_PROP_TYPE_COMBO },
    { DIALOG_STDGROUP_LM, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
prop_prop_field_type_data = { UI_TEXT_INIT_RESID(REC_MSG_PROP_FIELDTYPE) };

static const DIALOG_CONTROL
prop_prop_type_combo =
{
    PROP_ID_PROP_TYPE_COMBO, PROP_ID_PROP_GROUP,
    { PROP_ID_PROP_CAPTION_GROUP, DIALOG_CONTROL_PARENT },
    { DIALOG_STDSPACING_H, DIALOG_STDGROUP_TM, DIALOG_SYSCHARSL_H(14) + (44*PIXITS_PER_RISCOS), DIALOG_STDCOMBO_V },
    { DRT(RTLT, COMBO_TEXT), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_COMBO_TEXT
prop_prop_type_combo_data =
{
  {/*combo_xx*/

    {/*edit_xx*/ {/*bits*/ FRAMED_BOX_EDIT, 1 /*readonly*/ /*bits*/}, NULL /*edit_xx*/},

    {/*list_xx*/ { 0 /*force_v_scroll*/, 0 /*disable_double*/, 0 /*tab_position*/} /*list_xx*/},

        NULL,

        9 * DIALOG_STDLISTITEM_V /*dropdown_size*/

      /*combo_xx*/},

    { UI_TEXT_TYPE_NONE } /*state*/
};

static const DIALOG_CONTROL
prop_prop_default_value_caption =
{
    PROP_ID_PROP_DEFAULT_CAPTION, PROP_ID_PROP_CAPTION_GROUP,
    { PROP_ID_PROP_TYPE_CAPTION, PROP_ID_PROP_DEFAULT_EDIT, DIALOG_CONTROL_SELF, PROP_ID_PROP_DEFAULT_EDIT },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
prop_prop_default_value_caption_data = { UI_TEXT_INIT_RESID(REC_MSG_PROP_DEFAULTVALUE) };

static const DIALOG_CONTROL
prop_prop_default_edit =
{
    PROP_ID_PROP_DEFAULT_EDIT, PROP_ID_PROP_GROUP,
    { PROP_ID_PROP_TYPE_COMBO, PROP_ID_PROP_TYPE_COMBO, PROP_ID_PROP_VALUES_ADD },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V },
    { DRT(LBRT, EDIT) }
};

static const DIALOG_CONTROL_DATA_EDIT
prop_prop_default_edit_data= { { { FRAMED_BOX_EDIT } }, { UI_TEXT_TYPE_NONE } };

static const DIALOG_CONTROL
prop_prop_check_formula =
{
    PROP_ID_PROP_CHECK_CAPTION, PROP_ID_PROP_CAPTION_GROUP,
    { PROP_ID_PROP_DEFAULT_CAPTION, PROP_ID_PROP_CHECK_EDIT, DIALOG_CONTROL_SELF, PROP_ID_PROP_CHECK_EDIT},
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
prop_prop_check_formula_data = { UI_TEXT_INIT_RESID(REC_MSG_CHECK_FORMULA) };

static const DIALOG_CONTROL
prop_prop_check_edit =
{
    PROP_ID_PROP_CHECK_EDIT, PROP_ID_PROP_GROUP,
    { PROP_ID_PROP_DEFAULT_EDIT, PROP_ID_PROP_DEFAULT_EDIT, PROP_ID_PROP_DEFAULT_EDIT },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V },
    { DRT(LBRT, EDIT) }
};

static const DIALOG_CONTROL_DATA_EDIT
prop_prop_check_edit_data= { { { FRAMED_BOX_EDIT } }, { UI_TEXT_TYPE_NONE } };

/* the values list handling stuff */

static const DIALOG_CONTROL
prop_prop_values_static =
{
    PROP_ID_PROP_VALUES_CAPTION, PROP_ID_PROP_CAPTION_GROUP,
    { PROP_ID_PROP_CHECK_CAPTION, PROP_ID_PROP_VALUES_EDIT, DIALOG_CONTROL_SELF, PROP_ID_PROP_VALUES_EDIT },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
prop_prop_values_static_data = { UI_TEXT_INIT_RESID(REC_MSG_PROP_VALUELIST) };

/* a writable icon */

static const DIALOG_CONTROL
prop_prop_values_edit =
{
    PROP_ID_PROP_VALUES_EDIT, PROP_ID_PROP_GROUP,
    { PROP_ID_PROP_CHECK_EDIT, PROP_ID_PROP_CHECK_EDIT},
    { 0, DIALOG_STDSPACING_V, DIALOG_SYSCHARSL_H(14), DIALOG_STDEDIT_V },
    { DRT(LBLT, EDIT) }
};

static const DIALOG_CONTROL_DATA_EDIT
prop_prop_values_edit_data= { { { FRAMED_BOX_EDIT } }, { UI_TEXT_TYPE_NONE } };

static /*poked*/ DIALOG_CONTROL
prop_prop_values_list =
{
    PROP_ID_PROP_VALUES_LIST, PROP_ID_PROP_GROUP,
    { PROP_ID_PROP_VALUES_EDIT, PROP_ID_PROP_VALUES_EDIT, PROP_ID_PROP_VALUES_EDIT },
    { 0, DIALOG_STDSPACING_V, 0, 5*DIALOG_STDLISTITEM_V + DIALOG_STDLISTOVH_V },
    { DRT(LBRT, LIST_TEXT), 1 }
};

static const DIALOG_CONTROL
prop_prop_values_add =
{
    PROP_ID_PROP_VALUES_ADD, PROP_ID_PROP_GROUP,
    { PROP_ID_PROP_VALUES_EDIT, PROP_ID_PROP_VALUES_EDIT, DIALOG_CONTROL_SELF, PROP_ID_PROP_VALUES_EDIT },
    { DIALOG_STDSPACING_H, 0, DIALOG_SYSCHARSL_H(10), 0 },
    { DRT(RTLB, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
prop_prop_values_add_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_CREATE_ADD) };

static const DIALOG_CONTROL
prop_prop_values_remove =
{
    PROP_ID_PROP_VALUES_REMOVE, PROP_ID_PROP_GROUP,
    { PROP_ID_PROP_VALUES_ADD, PROP_ID_PROP_VALUES_ADD, PROP_ID_PROP_VALUES_ADD },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
prop_prop_values_remove_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_CREATE_REMOVE) };

static const DIALOG_CONTROL
prop_prop_values_up =
{
    PROP_ID_PROP_VALUES_UP, PROP_ID_PROP_GROUP,
    { PROP_ID_PROP_VALUES_REMOVE, PROP_ID_PROP_VALUES_REMOVE, PROP_ID_PROP_VALUES_REMOVE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
prop_prop_values_up_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_SORT_UP) };

static const DIALOG_CONTROL
prop_prop_values_down =
{
    PROP_ID_PROP_VALUES_DOWN, PROP_ID_PROP_GROUP,
    { PROP_ID_PROP_VALUES_UP, PROP_ID_PROP_VALUES_UP, PROP_ID_PROP_VALUES_UP },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
prop_prop_values_down_data = { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_SORT_DOWN) };

#define COMPULSORY 0

#if COMPULSORY

static const DIALOG_CONTROL
prop_prop_compulsory =
{
    PROP_ID_PROP_COMPULSORY, PROP_ID_PROP_GROUP,
    { PROP_ID_PROP_VALUES_CAPTION, PROP_ID_PROP_VALUES_LIST },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX) }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
prop_prop_compulsory_data= { { 0 }, UI_TEXT_INIT_RESID(REC_MSG_PROP_COMPULSORY) };

#endif

static const DIALOG_CTL_CREATE
properties_ctl_create[] =
{
    { &dialog_main_group },

    { &prop_field_fields, &prop_field_fields_data },
    { &prop_field_edit,   &prop_field_edit_data },
    { &prop_field_list,   &stdlisttext_data_dd_vsc },
    { &prop_field_add,    &prop_field_add_data },
    { &prop_field_remove, &prop_field_remove_data },
    { &prop_field_up,     &prop_field_up_data },
    { &prop_field_down,   &prop_field_down_data },
    { &prop_field_rename, &prop_field_rename_data },

    { &prop_prop_group,  &prop_prop_group_data },
    { &prop_prop_caption_group },

    { &prop_prop_field_type,    &prop_prop_field_type_data },
    { &prop_prop_type_combo,    &prop_prop_type_combo_data },

    { &prop_prop_default_value_caption,  &prop_prop_default_value_caption_data },
    { &prop_prop_default_edit,  &prop_prop_default_edit_data },

    { &prop_prop_check_formula, &prop_prop_check_formula_data },
    { &prop_prop_check_edit,    &prop_prop_check_edit_data },

    { &prop_prop_values_static, &prop_prop_values_static_data },

    { &prop_prop_values_edit,   &prop_prop_values_edit_data },
    { &prop_prop_values_list,   &stdlisttext_data_dd_vsc },
    { &prop_prop_values_add,    &prop_prop_values_add_data },

    { &prop_prop_values_remove, &prop_prop_values_remove_data },
    { &prop_prop_values_up,     &prop_prop_values_up_data },
    { &prop_prop_values_down,   &prop_prop_values_down_data },

#if COMPULSORY
    { &prop_prop_compulsory,   &prop_prop_compulsory_data },
#endif

    { &stdbutton_cancel,    &stdbutton_cancel_data },
    { &defbutton_ok,        &defbutton_ok_data }
};

typedef struct PROPERTIES_STRUCTURE
{
    S32 current_item;
    TABLEDEF fake_table;
    UI_SOURCE fields; /* array of ui_text for the fields list box */
    UI_SOURCE values; /* array of ui_text for the values list box */
    UI_SOURCE tcombo;
    UI_TEXT filename; /* The filename */
}
PROPERTIES_STRUCTURE, * P_PROPERTIES_STRUCTURE;

static PROPERTIES_STRUCTURE rec_properties;

static P_REC_PROJECTOR static_p_rec_projector = NULL;

static S32 interlock;

/* Read from the dialog, write to the field */

static void
decode_field_properties(
    _InVal_     H_DIALOG h_dialog,
    P_TABLEDEF p_table,
    _In_        S32 n)
{
    P_FIELDDEF p_fielddef = array_ptr(&p_table->h_fielddefs, FIELDDEF, n);

    {
    UI_TEXT ui_text;
    ui_dlg_get_edit(h_dialog, PROP_ID_PROP_DEFAULT_EDIT, &ui_text);
    zero_array(p_fielddef->default_formula);
    tstr_xstrkpy(p_fielddef->default_formula, elemof32(p_fielddef->default_formula), ui_text_tstr(&ui_text));
    ui_text_dispose(&ui_text);
    } /*block*/

    {
    UI_TEXT ui_text;
    ui_dlg_get_edit(h_dialog, PROP_ID_PROP_CHECK_EDIT, &ui_text);
    zero_array(p_fielddef->check_formula);
    tstr_xstrkpy(p_fielddef->check_formula, elemof32(p_fielddef->check_formula), ui_text_tstr(&ui_text));
    ui_text_dispose(&ui_text);
    } /*block*/

    { /* Build up a string based on each of the items in the list */
    ARRAY_INDEX i;

    zero_array(p_fielddef->value_list);

    for(i = 0; i < array_elements(&rec_properties.values.source.array_handle); ++i)
    {
        P_UI_TEXT p_ui_text = array_ptr(&rec_properties.values.source.array_handle, UI_TEXT, i);

        if(i)
            tstr_xstrkat(p_fielddef->value_list, elemof32(p_fielddef->value_list), TEXT(","));

        tstr_xstrkat(p_fielddef->value_list, elemof32(p_fielddef->value_list), ui_text_tstr(p_ui_text));
    }
    } /*block*/

    /* Now read the type from the combo */
    switch(ui_dlg_get_list_idx(h_dialog, PROP_ID_PROP_TYPE_COMBO))
    {
    default:
    case 0: p_fielddef->type = FIELD_TEXT;     break;
    case 1: p_fielddef->type = FIELD_REAL;     break;
    case 2: p_fielddef->type = FIELD_DATE;     break;
    case 3: p_fielddef->type = FIELD_INTEGER;  break;
    case 4: p_fielddef->type = FIELD_BOOL;     break;
    case 5: p_fielddef->type = FIELD_PICTURE;  break;
    case 6: p_fielddef->type = FIELD_FILE;     break;
    case 7: p_fielddef->type = FIELD_FORMULA;  break;
    case 8: p_fielddef->type = FIELD_INTERVAL; break;
    }

#if COMPULSORY
    p_fielddef->compulsory = ui_dlg_get_check(h_dialog, PROP_ID_PROP_COMPULSORY);
#endif
}

_Check_return_
static STATUS
encode_field_properties(
    _InVal_     H_DIALOG h_dialog,
    P_TABLEDEF p_table,
    _In_        S32 n)
{
    P_FIELDDEF p_fielddef = array_ptr(&p_table->h_fielddefs, FIELDDEF, n);
    STATUS status = STATUS_OK;

    {
    UI_TEXT ui_text;
    ui_text.type = UI_TEXT_TYPE_TSTR_TEMP;
    ui_text.text.tstr = p_fielddef->default_formula;
    ui_dlg_set_edit(h_dialog, PROP_ID_PROP_DEFAULT_EDIT, &ui_text);
    } /*block*/

    {
    UI_TEXT ui_text;
    ui_text.type = UI_TEXT_TYPE_TSTR_TEMP;
    ui_text.text.tstr = p_fielddef->check_formula;
    ui_dlg_set_edit(h_dialog, PROP_ID_PROP_CHECK_EDIT, &ui_text);
    } /*block*/

    /* first we should dispose of any extant value list */
    ui_source_dispose(&rec_properties.values);

    { /* now create a new array */
    const PC_SBSTR sbstr = p_fielddef->value_list;
    const S32 len = strlen(sbstr);

    if(len)
    {
        S32 i = 0;
        S32 j = 0;

        rec_properties.values.type = UI_SOURCE_TYPE_ARRAY;

        for(;;)
        {
            if((CH_COMMA == sbstr[i]) || (CH_NULL == sbstr[i]))
            {
                P_UI_TEXT p_ui_text;
                QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
                quick_tblock_with_buffer_setup(quick_tblock);

                if(status_ok(quick_tblock_tchars_add(&quick_tblock, sbstr + j, i - j)))
                    quick_tblock_nullch_add(&quick_tblock);

                if(NULL != (p_ui_text = al_array_extend_by_UI_TEXT(&rec_properties.values.source.array_handle, 1, &array_init_block_ui_text, &status)))
                    status = ui_text_alloc_from_tstr(p_ui_text, quick_tblock_tstr(&quick_tblock));

                quick_tblock_dispose(&quick_tblock);

                status_break(status);

                if(CH_NULL == sbstr[i])
                    break;

                j = i + 1; /* remember start of next component */
            }

            ++i;
        }
    }
    } /*block*/

    ui_dlg_ctl_new_source(h_dialog, PROP_ID_PROP_VALUES_LIST);

    {
    S32 combo_item;

    switch(p_fielddef->type)
    {
    default:
    case FIELD_TEXT:     combo_item = 0; break;
    case FIELD_REAL:     combo_item = 1; break;
    case FIELD_DATE:     combo_item = 2; break;
    case FIELD_INTEGER:  combo_item = 3; break;
    case FIELD_BOOL:     combo_item = 4; break;
    case FIELD_PICTURE:  combo_item = 5; break;
    case FIELD_FILE:     combo_item = 6; break;
    case FIELD_FORMULA:  combo_item = 7; break;
    case FIELD_INTERVAL: combo_item = 8; break;
    }

    ui_dlg_set_list_idx(h_dialog, PROP_ID_PROP_TYPE_COMBO, combo_item);
    } /*block*/

#if COMPULSORY
    ui_dlg_set_check(h_dialog, PROP_ID_PROP_COMPULSORY, p_fielddef->compulsory);
#endif

    return(status);
}

_Check_return_
static STATUS
re_encode_properties(
    _InVal_     H_DIALOG h_dialog,
    P_TABLEDEF p_table)
{
    S32 new_item = ui_dlg_get_list_idx(h_dialog, PROP_ID_FIELD_LIST);

    if(rec_properties.current_item != -1)
        /* setup the fielddef for the previously selected item */
        decode_field_properties(h_dialog, p_table, rec_properties.current_item);

    /* Mark the new item as current */
    rec_properties.current_item = new_item;

    if(rec_properties.current_item != -1)
        /* reflect the state of the newly selected item */
        status_assert(encode_field_properties(h_dialog, p_table, rec_properties.current_item));

    return(STATUS_OK);
}

_Check_return_
static STATUS
rec_properties_field_updown(
    _InVal_     H_DIALOG h_dialog,
    _In_        S32 delta)
{
    STATUS status = STATUS_OK;
    S32 itemno = ui_dlg_get_list_idx(h_dialog, PROP_ID_FIELD_LIST);
    S32 itemno_m_delta = itemno - delta;
    S32 n = array_elements(&rec_properties.fields.source.array_handle);

    if((itemno >= 0) && (itemno < n) && ((itemno_m_delta) >= 0) && ((itemno_m_delta) < n))
    {
        /* ensure the field is up to date wrt the dialog settings */
        if(rec_properties.current_item != -1)
            decode_field_properties(h_dialog, &rec_properties.fake_table, rec_properties.current_item);

        memswap32(array_ptr(&rec_properties.fake_table.h_fielddefs, FIELDDEF, itemno_m_delta),
                  array_ptr(&rec_properties.fake_table.h_fielddefs, FIELDDEF, itemno),
                  sizeof32(FIELDDEF));

        memswap32(array_ptr(&rec_properties.fields.source.array_handle, UI_TEXT, itemno_m_delta),
                  array_ptr(&rec_properties.fields.source.array_handle, UI_TEXT, itemno),
                  sizeof32(UI_TEXT));

        /* rec_properties.current_item needs to be changed !
        calling re_encode_properties does this BUT it also enforces the dialog state against the new occupier of the current slot!
        NB the re_encode_properties can get invoked but a state change caused by the prop_field_list???? calls which follow...
        The solution is to trash the current state first
        */
        rec_properties.current_item = -1;

        ui_dlg_ctl_new_source(h_dialog, PROP_ID_FIELD_LIST);

        ui_dlg_set_list_idx(h_dialog, PROP_ID_FIELD_LIST, itemno_m_delta);

        re_encode_properties(h_dialog, &rec_properties.fake_table);
    }

    return(status);
}

/* Add the contents of the edit control to the list box */

_Check_return_
static STATUS
rec_properties_field_add(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    UI_TEXT ui_text;
    PCTSTR tstr;

    ui_dlg_get_edit(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_FIELD_EDIT, &ui_text);

    tstr = ui_text_tstr_no_default(&ui_text);

    if((NULL == tstr) || (0 == tstrlen32(tstr)))
        return(create_error(REC_ERR_FIELD_NAME_BLANK));

    if(P_DATA_NONE != p_fielddef_from_name(&rec_properties.fake_table.h_fielddefs, tstr))
        return(create_error(REC_ERR_ALREADY_A_FIELD));

    status_return(add_new_field_to_table(&rec_properties.fake_table, tstr, FIELD_TEXT));

    if(status_fail(al_array_add(&rec_properties.fields.source.array_handle, UI_TEXT, 1, &array_init_block_ui_text, &ui_text)))
    {
        ui_text_dispose(&ui_text);
        return(status_nomem());
    }
    /* stolen the answer now */

    /* Flush the edit control */
    ui_dlg_set_edit(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_FIELD_EDIT, P_DATA_NONE);

    ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_FIELD_LIST);

    /* Activate the new item in the list box */
    ui_dlg_set_list_idx(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_FIELD_LIST, array_elements(&rec_properties.fields.source.array_handle) - 1);

    ui_dlg_ctl_set_default(p_dialog_msg_ctl_pushbutton->h_dialog, IDOK);

    re_encode_properties(p_dialog_msg_ctl_pushbutton->h_dialog, &rec_properties.fake_table);

    p_dialog_msg_ctl_pushbutton->processed = 1;

    return(STATUS_OK);
}

/* If there is a selection in the list box then remove it */

_Check_return_
static STATUS
rec_properties_field_remove(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    S32 itemno = ui_dlg_get_list_idx(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_FIELD_LIST);

    if(array_index_valid(&rec_properties.fields.source.array_handle, itemno))
    {
        /* remove the named field aus der tabledef */
        remove_field_from_table(&rec_properties.fake_table, itemno);

        /* remove the named field aus der list box source */
        ui_text_dispose(array_ptr(&rec_properties.fields.source.array_handle, UI_TEXT, itemno));

        al_array_delete_at(&rec_properties.fields.source.array_handle, -1, itemno);

        ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_FIELD_LIST);

        rec_properties.current_item = -1;

        re_encode_properties(p_dialog_msg_ctl_pushbutton->h_dialog, &rec_properties.fake_table);
    }

    p_dialog_msg_ctl_pushbutton->processed = 1;

    return(STATUS_OK);
}

/* Change the name of the selected field to the text in the edit control */

_Check_return_
static STATUS
rec_properties_field_rename(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    S32 itemno = ui_dlg_get_list_idx(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_FIELD_LIST);
    UI_TEXT ui_text;
    PCTSTR tstr;
    P_FIELDDEF p_fielddef;
    STATUS status;

    if(!array_index_valid(&rec_properties.fake_table.h_fielddefs, itemno))
        return(STATUS_OK);

    status_return(status = ensure_memory_froth());

    ui_dlg_get_edit(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_FIELD_EDIT, &ui_text);

    tstr = ui_text_tstr_no_default(&ui_text);

    if((NULL == tstr) || (0 == strlen(tstr)))
        return(create_error(REC_ERR_FIELD_NAME_BLANK));

    if(P_DATA_NONE != p_fielddef_from_name(&rec_properties.fake_table.h_fielddefs, tstr))
        return(create_error(REC_ERR_ALREADY_A_FIELD));

    p_fielddef = array_ptr(&rec_properties.fake_table.h_fielddefs, FIELDDEF, itemno);

    { /* check if we have a named style for this database/field name combination - if so, rename it BEFORE we hack the new name in */
    STYLE_HANDLE style_handle;
    TCHARZ style_name_buffer[128];
    PTSTR tstr_style = style_name_buffer;
    PTSTR tstr_style_replace;

    *tstr_style++ = 0x03;
    tstr_xstrkpy(tstr_style, elemof32(style_name_buffer) - (tstr_style - style_name_buffer),
                   rec_properties.fake_table.name);
    tstr_style += tstrlen32(tstr_style);
    *tstr_style++ = 0x03;
    *tstr_style = CH_NULL;

    tstr_style_replace = tstr_style;
    tstr_xstrkpy(tstr_style_replace, elemof32(style_name_buffer) - (tstr_style_replace - style_name_buffer),
                   p_fielddef->name);
    style_handle = style_handle_from_name(p_docu, style_name_buffer);
    tstr_xstrkpy(tstr_style_replace, elemof32(style_name_buffer) - (tstr_style_replace - style_name_buffer),
                   tstr); /* replace field name part */

    if(0 != style_handle)
    {
        STYLE style;
        style_init(&style);
        style_bit_set(&style, STYLE_SW_NAME);
        if(status_ok(status = al_tstr_set(&style.h_style_name_tstr, style_name_buffer)))
            /* donate to style system */
            style_handle_modify(p_docu, style_handle, &style, &style.selector, &style.selector);
    }
    } /*block*/

    { /* rename the field title too, if it hasn't been changed (SKS 28jul95) */
    P_REC_FRAME p_rec_frame = p_rec_frame_from_field_id(&static_p_rec_projector->h_rec_frames, p_fielddef->id);

    if(P_DATA_NONE != p_rec_frame)
    {
        BOOL rename = TRUE;

        if(0 != array_elements(&p_rec_frame->h_title_text_ustr))
            if(0 != /*"C"*/strcmp(array_ustr(&p_rec_frame->h_title_text_ustr), p_fielddef->name))
                rename = FALSE;

        if(rename)
        {
            al_array_dispose(&p_rec_frame->h_title_text_ustr);
            status = al_ustr_set(&p_rec_frame->h_title_text_ustr, tstr); /*T==U*/
        }
    }
    } /*block*/

    tstr_xstrkpy(p_fielddef->name, elemof32(p_fielddef->name), tstr);

    IGNOREPARM(status);

    { /* change what's stored in the list too. just steal the answer */
    P_UI_TEXT p_ui_text = array_ptr(&rec_properties.fields.source.array_handle, UI_TEXT, itemno);
    ui_text_dispose(p_ui_text);
    *p_ui_text = ui_text;
    } /*block*/

    ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_FIELD_LIST);

    /* Flush the edit control */
    ui_dlg_set_edit(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_FIELD_EDIT, P_DATA_NONE);

    rec_properties.current_item = -1; /* can't be bothered to track it */

    p_dialog_msg_ctl_pushbutton->processed = 1;

    re_encode_properties(p_dialog_msg_ctl_pushbutton->h_dialog, &rec_properties.fake_table);

    return(STATUS_OK);
}

/******************************************************************************
*
* dialog box handler for properties boxes
*
******************************************************************************/

_Check_return_
static STATUS
dialog_properties_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case PROP_ID_FIELD_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &rec_properties.fields;
        break;

    case PROP_ID_PROP_VALUES_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &rec_properties.values;
        break;

    case PROP_ID_PROP_TYPE_COMBO:
        p_dialog_msg_ctl_fill_source->p_ui_source = &rec_properties.tcombo;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_properties_process_start(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    ui_dlg_set_list_idx(p_dialog_msg_process_start->h_dialog, PROP_ID_PROP_VALUES_LIST, -1);

    { /* let's try selecting the current field */
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    S32 itemno = -1;

    if(NULL != (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref)))
    {
        if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
        {
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
            S32 current_field_number = fieldnumber_from_field_id(&p_rec_projector->opendb.table, field_id_from_rec_data_ref(&data_ref));
            if(status_ok(current_field_number))
                itemno = current_field_number - 1;
        }
    }

    ui_dlg_set_list_idx(p_dialog_msg_process_start->h_dialog, PROP_ID_FIELD_LIST, (itemno < 0) ? 0 : itemno);
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_properties_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const H_DIALOG h_dialog = p_dialog_msg_ctl_state_change->h_dialog;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case PROP_ID_FIELD_EDIT:
        if(0 == interlock)
            ui_dlg_ctl_set_default(h_dialog, PROP_ID_FIELD_ADD);
        break;

    case PROP_ID_PROP_VALUES_EDIT:
        ui_dlg_ctl_set_default(h_dialog, PROP_ID_PROP_VALUES_ADD);
        break;

    case PROP_ID_FIELD_LIST:
        { /* The selected itemno in the list box has changed */
        BOOL flag = ( ui_dlg_get_list_idx(p_dialog_msg_ctl_state_change->h_dialog, PROP_ID_FIELD_LIST) >= 0);

        ++interlock;
        ui_dlg_set_edit(p_dialog_msg_ctl_state_change->h_dialog, PROP_ID_FIELD_EDIT, &p_dialog_msg_ctl_state_change->new_state.list_text.ui_text);
        /* SKS 07apr95 reflect value in edit control for easy rename */
        --interlock;

        re_encode_properties(p_dialog_msg_ctl_state_change->h_dialog, &rec_properties.fake_table);

        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, PROP_ID_PROP_GROUP, flag);

        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, PROP_ID_FIELD_REMOVE, flag);
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, PROP_ID_FIELD_UP, flag);
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, PROP_ID_FIELD_DOWN, flag);
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, PROP_ID_FIELD_RENAME, flag);

        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_properties_ctl_pushbutton(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    STATUS status = STATUS_OK;

    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case PROP_ID_FIELD_UP:
    case PROP_ID_FIELD_DOWN:
        {
        S32 delta = p_dialog_msg_ctl_pushbutton->right_button ? -1 : +1;

        if(p_dialog_msg_ctl_pushbutton->dialog_control_id == PROP_ID_FIELD_DOWN)
            delta = -delta;

        status = rec_properties_field_updown(p_dialog_msg_ctl_pushbutton->h_dialog, delta);

        p_dialog_msg_ctl_pushbutton->processed = 1;

        break;
        }

    case PROP_ID_FIELD_ADD:
        return(rec_properties_field_add(p_dialog_msg_ctl_pushbutton));

    case PROP_ID_FIELD_REMOVE:
        return(rec_properties_field_remove(p_dialog_msg_ctl_pushbutton));

    case PROP_ID_FIELD_RENAME:
        return(rec_properties_field_rename(p_docu, p_dialog_msg_ctl_pushbutton));

    case PROP_ID_PROP_VALUES_UP:
    case PROP_ID_PROP_VALUES_DOWN:
        {
        S32 itemno = ui_dlg_get_list_idx(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_PROP_VALUES_LIST);
        S32 n = array_elements(&rec_properties.values.source.array_handle);
        S32 delta = p_dialog_msg_ctl_pushbutton->right_button ? -1 : +1;
        S32 itemno_m_delta;

        if(p_dialog_msg_ctl_pushbutton->dialog_control_id == PROP_ID_PROP_VALUES_DOWN)
            delta = - delta;

        itemno_m_delta = itemno - delta;

        if((itemno >= 0) && (itemno < n) && (itemno_m_delta >= 0) && (itemno_m_delta < n))
        {
            /* Swap over the ui_texts in the array and track the selection */
            memswap32(array_ptr(&rec_properties.values.source.array_handle, UI_TEXT, itemno_m_delta),
                      array_ptr(&rec_properties.values.source.array_handle, UI_TEXT, itemno),
                      sizeof32(UI_TEXT));

            ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_PROP_VALUES_LIST);

            status = ui_dlg_set_list_idx(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_PROP_VALUES_LIST, itemno_m_delta);
        }

        p_dialog_msg_ctl_pushbutton->processed = 1;

        break;
        }

    /* Add the contents of the edit control to the values list box */
    case PROP_ID_PROP_VALUES_ADD:
        {
        UI_TEXT ui_text;

        ui_dlg_get_edit(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_PROP_VALUES_EDIT, &ui_text);

        if(!ui_text_is_blank(&ui_text))
        {
            if(status_ok(status = al_array_add(&rec_properties.values.source.array_handle, UI_TEXT, 1, &array_init_block_ui_text, &ui_text))) /* steal the ui_text */
            {
                rec_properties.values.type = UI_SOURCE_TYPE_ARRAY; /* SKS 21feb96 */
                ui_text.type = UI_TEXT_TYPE_NONE; /* yum! */
            }
        }
        else
            host_bleep();

        ui_text_dispose(&ui_text);

        /* Flush the edit control */
        status_accumulate(status, ui_dlg_set_edit(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_PROP_VALUES_EDIT, P_DATA_NONE));

        ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_PROP_VALUES_LIST);

        ui_dlg_ctl_set_default(p_dialog_msg_ctl_pushbutton->h_dialog, IDOK);

        p_dialog_msg_ctl_pushbutton->processed = 1;

        break;
        }

    case PROP_ID_PROP_VALUES_REMOVE:
        {
        /* If there is a selection in the list box then remove it */
        S32 itemno = ui_dlg_get_list_idx(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_PROP_VALUES_LIST);

        if(array_index_valid(&rec_properties.values.source.array_handle, itemno))
        {
            ui_text_dispose(array_ptr_no_checks(&rec_properties.values.source.array_handle, UI_TEXT, itemno));

            al_array_delete_at(&rec_properties.values.source.array_handle, -1, itemno);
        }

        ui_dlg_ctl_new_source(p_dialog_msg_ctl_pushbutton->h_dialog, PROP_ID_PROP_VALUES_LIST);

        p_dialog_msg_ctl_pushbutton->processed = 1;

        break;
        }
    }

    return(status);
}

_Check_return_
static STATUS
dialog_properties_process_end(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        if(rec_properties.current_item != -1)
            /* Ensure the state of the dialog gets reflected correctly */
            decode_field_properties(p_dialog_msg_process_end->h_dialog, &rec_properties.fake_table, rec_properties.current_item);

        view_update_all(p_docu, UPDATE_PANE_CELLS_AREA);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_properties)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_properties_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_properties_process_start(p_docu, (PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_properties_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_properties_ctl_pushbutton(p_docu, (P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_properties_process_end(p_docu, (PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
t5_cmd_db_properties(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    static const UI_TEXT caption = UI_TEXT_INIT_RESID(REC_MSG_PROP_GROUP);
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_db_id(p_docu, p_data_ref->arg.db_field.db_id);
    DOCU_AREA old_docu_area;
    DOCU_AREA new_docu_area;
    STATUS status;

    status_return(table_check_access(&p_rec_projector->opendb.table, ACCESS_FUNCTION_LAYOUT));

#if DPLIB
    if(dplib_is_db_on_server(&p_rec_projector->opendb))
        return(create_error(REC_ERR_PROP_ON_SERVER));
#endif

    /* I think we should do a view whole file here */
    status_return(rec_revert_whole(p_rec_projector));

    old_docu_area = p_rec_projector->rec_docu_area;

    rec_properties.fake_table = p_rec_projector->opendb.table;
    rec_properties.fake_table.h_fielddefs = 0;
    status = table_copy_fields(&rec_properties.fake_table, &p_rec_projector->opendb.table);

    static_p_rec_projector = p_rec_projector;

    rec_properties.current_item = -1;

    rec_properties.fields.type = UI_SOURCE_TYPE_NONE;
    rec_properties.tcombo.type = UI_SOURCE_TYPE_NONE;
    rec_properties.values.type = UI_SOURCE_TYPE_NONE;

    { /* make appropriate size box */
    PIXIT max_width =  ui_width_from_p_ui_text(&caption) + DIALOG_CAPTIONOVH_H; /* bare minimum */
    S32 show_elements;
    if(status_ok(status = rec_fields_ui_source_create(&rec_properties.fields, &rec_properties.fake_table, &max_width, &show_elements)))
    {
        PIXIT_SIZE list_size;
        DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
        dialog_cmd_ctl_size_estimate.p_dialog_control = &prop_field_list;
        dialog_cmd_ctl_size_estimate.p_dialog_control_data = &stdlisttext_data;
        ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
        dialog_cmd_ctl_size_estimate.size.x += max_width;
        ui_list_size_estimate(show_elements, &list_size);
        dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
        dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
        prop_field_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
        prop_field_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
    }
    } /*block*/

    /* create source for field type list */
    if(status_ok(status))
    {
        P_UI_TEXT p_ui_text;
        ARRAY_INDEX i;

        rec_properties.tcombo.type = UI_SOURCE_TYPE_NONE;

        if(NULL == (p_ui_text = al_array_alloc_UI_TEXT(&rec_properties.tcombo.source.array_handle, 1 + (REC_MSG_FIELD_TYPE_LAST - REC_MSG_FIELD_TYPE_BASE), &array_init_block_ui_text, &status)))
            return(status);

        rec_properties.tcombo.type = UI_SOURCE_TYPE_ARRAY;

        for(i = 0; i <= (REC_MSG_FIELD_TYPE_LAST - REC_MSG_FIELD_TYPE_BASE); ++i)
        {
            p_ui_text->type = UI_TEXT_TYPE_RESID;
            p_ui_text++->text.resource_id = REC_MSG_FIELD_TYPE_BASE + i;
        }
    }

    if(status_ok(status))
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, properties_ctl_create, elemof32(properties_ctl_create), 0);
        dialog_cmd_process_dbox.caption = caption;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_rec_projector;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_properties;
        status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    }

    if(status_ok(status))
    {
        S32 n_visible = ensure_some_field_visible(&rec_properties.fake_table);

        if(PROJECTOR_TYPE_SHEET == p_rec_projector->projector_type)
            p_rec_projector->rec_docu_area.br.slr.col = p_rec_projector->rec_docu_area.tl.slr.col + (COL) n_visible;
    }

    new_docu_area = p_rec_projector->rec_docu_area;

    if(status_ok(status))
    {
        /* Here we need to suss out what changes have been made to the field structure (if any) and if
           required we must remap the database file to a new temp file and then do the suspend & resume.

           Significant changes are: field type changed, field added, field removed

           The remap can be done by building a suitable remap table and letting rip
        */

        /* Assuming a remap is required */
        { /* Make a tempfile! MUST be on the same file-root and the database so that it can be renamed */
        QUICK_TBLOCK_WITH_BUFFER(aqtb_tempname, 40);
        quick_tblock_with_buffer_setup(aqtb_tempname);

        {
        TCHARZ dirname_buffer[BUF_MAX_PATHSTRING];
        file_dirname(dirname_buffer, p_rec_projector->opendb.db.name);
        status = file_tempname(dirname_buffer, "db", NULL, 0, &aqtb_tempname);
        } /*block*/

        if(status_ok(status))
        {
            /* The hidden status which exists per field is NOT stored in the datapower file therefore it gets lost in this process!
               the remap_copy_fireworkz_attributes() call fixes-up this and other similar problems
            */
            status = remap_remap(&p_rec_projector->opendb, &rec_properties.fake_table, quick_tblock_tstr(&aqtb_tempname));

            if(status_ok(status))
            {
                status = switch_database(&p_rec_projector->opendb, quick_tblock_tstr(&aqtb_tempname));

                remap_copy_fireworkz_attributes(&p_rec_projector->opendb.table, &rec_properties.fake_table);
            }
        }

        drop_fake_table(&rec_properties.fake_table);

        quick_tblock_dispose(&aqtb_tempname);
        } /*block*/

        if(status_ok(status))
        {
            /* We must drop the old insertion, using the original docu area */
            p_rec_projector->rec_docu_area = old_docu_area;

            drop_projector_area(p_rec_projector);

            /* repair the damage */
            p_rec_projector->rec_docu_area = new_docu_area;

            if(status_ok(status = reconstruct_frames_from_fields(p_rec_projector)))
                status = rec_insert_projector_and_attach_with_styles(p_rec_projector, FALSE);
        }
    }
    else
        drop_fake_table(&rec_properties.fake_table); /* This causes it to spam now because the fields get the chop! */

    ui_source_dispose(&rec_properties.fields);
    ui_source_dispose(&rec_properties.tcombo);
    ui_source_dispose(&rec_properties.values);

    return(status);
}

/* the get password box has OK, Cancel and Enter password boxes */

enum PSWD_CONTROL_IDS
{
    PSWD_ID_EDIT = 137,
    PSWD_ID_MESS
};

static const DIALOG_CONTROL
pswd_edit =
{
    PSWD_ID_EDIT, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_SYSCHARSL_H(40), DIALOG_STDEDIT_V },
    { DRT(LTLT, EDIT), 1 }
};

static const DIALOG_CONTROL_DATA_EDIT
pswd_edit_data= { { { FRAMED_BOX_EDIT } }, { UI_TEXT_TYPE_NONE } };

static const DIALOG_CONTROL
pswd_message =
{
    PSWD_ID_MESS, DIALOG_MAIN_GROUP,
    { PSWD_ID_EDIT, PSWD_ID_EDIT, PSWD_ID_EDIT},
    { 0,DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V },
    { DRT(LBRT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
pswd_message_data = { UI_TEXT_INIT_RESID(REC_MSG_PSWD_BLANKOK), { 1 /*left_text*/ }};

static const DIALOG_CTL_CREATE
pswd_ctl_create[] =
{
    { &dialog_main_group },
    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &defbutton_ok,     &defbutton_ok_data },
    { &pswd_edit,        &pswd_edit_data },
    { &pswd_message,     &pswd_message_data } /* must be last one */
};

static UI_TEXT ui_text_edit;

_Check_return_
static STATUS
dialog_pswd_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        ui_dlg_get_edit(p_dialog_msg_process_end->h_dialog, (DIALOG_CTL_ID) PSWD_ID_EDIT, &ui_text_edit);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_pswd)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_pswd_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
get_db_password(
    _DocuRef_   P_DOCU p_docu,
    P_U8 buffer,
    _In_        S32 buffsize,
    _InVal_     BOOL blankok)
{
    STATUS status;

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, pswd_ctl_create, elemof32(pswd_ctl_create), 0);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = REC_MSG_PSWD_TITLE;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_pswd;
    /*dialog_cmd_process_dbox.client_handle = NULL;*/
    if(!blankok)
        dialog_cmd_process_dbox.n_ctls -= 1; /* strip last one off */
    status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    if(status_ok(status))
    {
        PCTSTR tstr = ui_text_tstr(&ui_text_edit);
        S32 length = tstrlen32(tstr);
        xstrkpy(buffer, buffsize, tstr);
        if(length < buffsize)
        {
            myassert0(TEXT("Password is too long"));
            status = STATUS_FAIL;
        }
   }

    return(status);
}

/* end of uiprops.c */
