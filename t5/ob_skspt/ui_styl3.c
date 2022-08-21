/* ui_styl3.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* UI core for style editing for Fireworkz */

/* SKS March 1993 */

#include "common/gflags.h"

#include "ob_skspt/ui_styin.h"

#if RISCOS
#include "ob_dlg/xp_dlgr.h"
#endif

/*
internal structure
*/

/*
mapping from control ids to style bits etc.
*/

typedef struct ES_WIBBLE
{
    UBF control_id          : 16; /* DIALOG_CTL_ID */
    UBF style_bit_number    : 8; /* could be bigger if pressed */
    UBF enabler             : 1;
    UBF set_state_on_enable : 1;
    UBF enable_units        : 1;
    UBF reserved            : 8-3;

    U16 data_type; /* must fit a ES_WIBBLE_TYPE_* so could be 4 bits if pushed */
    U32 member_offset;
}
ES_WIBBLE, * P_ES_WIBBLE; typedef const struct ES_WIBBLE * PC_ES_WIBBLE;

#define P_ES_WIBBLE_NONE _P_DATA_NONE(P_ES_WIBBLE)

/*
internal routines
*/

_Check_return_
static STATUS
create_using_es_wibble(
    P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state,
    P_ES_WIBBLE p_es_wibble,
    P_ES_CALLBACK p_es_callback);

_Check_return_
static STATUS
create_using_es_wibble_enabler(
    P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state,
    P_ES_WIBBLE p_es_wibble,
    P_ES_CALLBACK p_es_callback);

PROC_QSORT_PROTO(static, es_tab_list_compile_qsort, TAB_INFO);

_Check_return_
_Ret_maybenone_
static P_ES_WIBBLE
es_wibble_search(
    _InVal_     DIALOG_CTL_ID control_id);

_Check_return_
static STATUS
fill_using_es_wibble(
    P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source,
    P_ES_WIBBLE p_es_wibble,
    P_ES_CALLBACK p_es_callback);

_Check_return_
static STATUS
new_using_es_wibble(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change,
    P_ES_WIBBLE p_es_wibble,
    P_ES_CALLBACK p_es_callback);

_Check_return_
static STATUS
new_using_es_wibble_enabler(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change,
    P_ES_WIBBLE p_es_wibble,
    P_ES_CALLBACK p_es_callback);

_Check_return_
static STATUS
ui_list_create_new_object(
    /*inout*/ P_ARRAY_HANDLE p_array_handle,
    /*inout*/ P_UI_SOURCE p_ui_source);

_Check_return_
static STATUS
ui_list_create_numform(
    _DocuRef_   P_DOCU p_docu,
    /*inout*/ P_ARRAY_HANDLE p_array_handle,
    /*inout*/ P_UI_SOURCE p_ui_source,
    _InVal_     BIT_NUMBER bit_number);

_Check_return_
static STATUS
ui_list_create_style_key(
    _DocuRef_   P_DOCU p_docu,
    /*inout*/ P_ARRAY_HANDLE p_array_handle,
    /*inout*/ P_UI_SOURCE p_ui_source,
    _InVal_     STYLE_HANDLE style_handle_being_modified);

/*
listed info
*/

typedef struct UI_LIST_ENTRY_NEW_OBJECT
{
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 16); /* NB buffer adjacent for fixup */

    OBJECT_ID object_id;
}
UI_LIST_ENTRY_NEW_OBJECT, * P_UI_LIST_ENTRY_NEW_OBJECT;

static ARRAY_HANDLE
new_object_list_handle;

static UI_SOURCE
new_object_list_source;

/*
listed info
*/

typedef struct UI_LIST_ENTRY_NUMFORM
{
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 16); /* NB buffer adjacent for fixup */
}
UI_LIST_ENTRY_NUMFORM, * P_UI_LIST_ENTRY_NUMFORM;

static ARRAY_HANDLE
numform_list_nu_handle;

static UI_SOURCE
numform_list_nu_source;

static ARRAY_HANDLE
numform_list_dt_handle;

static UI_SOURCE
numform_list_dt_source;

static ARRAY_HANDLE
numform_list_se_handle;

static UI_SOURCE
numform_list_se_source;

/*
listed info
*/

typedef struct UI_LIST_ENTRY_STYLE_KEY
{
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 16); /* NB buffer adjacent for fixup */

    KMAP_CODE kmap_code;
}
UI_LIST_ENTRY_STYLE_KEY, * P_UI_LIST_ENTRY_STYLE_KEY;

static ARRAY_HANDLE
style_key_list_handle;

static UI_SOURCE
style_key_list_source;

/*
listed info
*/

static ARRAY_HANDLE
typeface_list_handle;

static UI_SOURCE
typeface_list_source;

/* ------------------------------------------------------------------------- */

static
STYLE_SELECTOR es_subdialog_style_selector[ES_SUBDIALOG_MAX];

_Check_return_
extern BOOL
es_subdialog_style_selector_test(
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InVal_     U32 subdialog)
{
    return(style_selector_test(p_style_selector, &es_subdialog_style_selector[subdialog]));
}

_Check_return_
static U32
es_light_on_for(
    P_ES_CALLBACK p_es_callback,
    _InVal_     U32 subdialog)
{
    STYLE_SELECTOR selector;

    /* form selector of which bits might be on at the moment */
    void_style_selector_bic(&selector, &p_es_callback->committed_style_selector, &p_es_callback->style.selector);
    void_style_selector_or(&selector, &selector, &p_es_callback->style_selector);

    return(style_selector_test(&selector, &es_subdialog_style_selector[subdialog]));
}

_Check_return_
static STATUS
tweak_style_create(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DIALOG_MSG_CREATE p_dialog_msg_create)
{
    STATUS status = STATUS_OK;
    const P_ES_CALLBACK p_es_callback = (P_ES_CALLBACK) p_dialog_msg_create->client_handle;

    switch(p_es_callback->subdialog_current)
    {
    case ES_SUBDIALOG_PS4:
        if(status_ok(status = ui_list_create_numform(p_docu, &numform_list_nu_handle, &numform_list_nu_source, STYLE_SW_PS_NUMFORM_NU)))
        if(status_ok(status = ui_list_create_numform(p_docu, &numform_list_dt_handle, &numform_list_dt_source, STYLE_SW_PS_NUMFORM_DT)))
        if(status_ok(status = ui_list_create_numform(p_docu, &numform_list_se_handle, &numform_list_se_source, STYLE_SW_PS_NUMFORM_SE)))
                     status = ui_list_create_new_object(&new_object_list_handle, &new_object_list_source);
        break;

    case ES_SUBDIALOG_NAME:
        status = ui_list_create_style_key(p_docu, &style_key_list_handle, &style_key_list_source, p_es_callback->style_handle_being_modified);
        break;

    case ES_SUBDIALOG_FS:
        /* NB the collection of available fonts may have changed so go look again */
        status_assert(maeve_service_event(p_docu, T5_MSG_FONTS_RECACHE, P_DATA_NONE));

        status = ui_list_create_typeface(&typeface_list_handle, &typeface_list_source);
        break;

    default:
        break;
    }

    p_es_callback->creating++;

    return(status);
}

_Check_return_
static STATUS
tweak_style_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    STATUS status = STATUS_OK;
    const P_ES_CALLBACK p_es_callback = (P_ES_CALLBACK) p_dialog_msg_process_start->client_handle;
    DIALOG_CTL_ID i;

    p_es_callback->creating--;

    { /* SKS 27may93: set the current category up (these radiobuttons no longer get DIALOG_MSG_CODE_CTL_CREATE_STATE) */
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = p_dialog_msg_process_start->h_dialog;
    dialog_cmd_ctl_state_set.control_id = ES_ID_CATEGORY_GROUP;
    dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_DONT_MSG;
    dialog_cmd_ctl_state_set.state.radiobutton = ES_ID_RADIO_STT + p_es_callback->subdialog_current;
    status = call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set);
    } /*block*/

    /* setting of initial light states needed (SKS 29apr94) */
    for(i = 0; (i < ES_SUBDIALOG_MAX) && status_ok(status); ++i)
    {
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
        msgclr(dialog_cmd_ctl_state_set);
        dialog_cmd_ctl_state_set.h_dialog = p_dialog_msg_process_start->h_dialog;
        dialog_cmd_ctl_state_set.control_id = (DIALOG_CTL_ID) (ES_ID_LIGHT_STT + i);
        dialog_cmd_ctl_state_set.bits = 0;
        * (P_RGB) &dialog_cmd_ctl_state_set.state.user = es_light_on_for(p_es_callback, i) ? rgb_stash[10 /*green*/] : rgb_stash[1 /*lt grey*/];
        status = call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set);
    }

#if defined(VALIDATE_MAIN_ALLOCS) && defined(VALIDATE_EDIT_STYLE)
    alloc_validate((P_ANY) 1, "tweak_style");
#endif

    /* nothing yet modified in subdialog edit (do after create has set up all state) */
    style_selector_clear(&p_es_callback->style_modified);
    style_selector_clear(&p_es_callback->style_selector_modified);

    if(!p_es_callback->has_colour_picker)
    {
#if RISCOS && 1
        /* no Colour... buttons! */
#else
        /* no colour picker - disable all the Colour... buttons */
        ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, ES_FS_ID_COLOUR_BUTTON, 0);
        ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, ES_PS_ID_RGB_BACK_BUTTON, 0);
        ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, ES_PS_ID_BORDER_BUTTON, 0);
        ui_dlg_ctl_enable(p_dialog_msg_process_start->h_dialog, ES_PS_ID_GRID_BUTTON, 0);
#endif
    }

    return(status);
}

_Check_return_
static STATUS
tweak_style_dispose(void)
{
    ui_lists_dispose_ub(&new_object_list_handle, &new_object_list_source, offsetof32(UI_LIST_ENTRY_NEW_OBJECT, quick_ublock));
    ui_lists_dispose_ub(&numform_list_nu_handle, &numform_list_nu_source, offsetof32(UI_LIST_ENTRY_NUMFORM, quick_ublock));
    ui_lists_dispose_ub(&numform_list_dt_handle, &numform_list_dt_source, offsetof32(UI_LIST_ENTRY_NUMFORM, quick_ublock));
    ui_lists_dispose_ub(&numform_list_se_handle, &numform_list_se_source, offsetof32(UI_LIST_ENTRY_NUMFORM, quick_ublock));
    ui_lists_dispose_tb(&style_key_list_handle, &style_key_list_source, offsetof32(UI_LIST_ENTRY_STYLE_KEY, quick_tblock));
    ui_lists_dispose_tb(&typeface_list_handle, &typeface_list_source, offsetof32(UI_LIST_ENTRY_TYPEFACE, quick_tblock));
    return(STATUS_OK);
}

_Check_return_
static STATUS
tweak_style_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    const P_ES_CALLBACK p_es_callback = (P_ES_CALLBACK) p_dialog_msg_ctl_fill_source->client_handle;
    const P_ES_WIBBLE p_es_wibble = es_wibble_search(p_dialog_msg_ctl_fill_source->dialog_control_id);

    if((P_ES_WIBBLE_NONE != p_es_wibble) && !p_es_wibble->enabler)
        return(fill_using_es_wibble(p_dialog_msg_ctl_fill_source, p_es_wibble, p_es_callback));

    return(STATUS_OK);
}

_Check_return_
static STATUS
tweak_style_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    const P_ES_CALLBACK p_es_callback = (P_ES_CALLBACK) p_dialog_msg_ctl_create_state->client_handle;
    const P_ES_WIBBLE p_es_wibble = es_wibble_search(p_dialog_msg_ctl_create_state->dialog_control_id);

    if(P_ES_WIBBLE_NONE != p_es_wibble)
        return((p_es_wibble->enabler ? create_using_es_wibble_enabler : create_using_es_wibble) (p_dialog_msg_ctl_create_state, p_es_wibble, p_es_callback));

    return(STATUS_OK);
}

_Check_return_
static STATUS
tweak_style_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    STATUS status = STATUS_OK;
    const P_ES_CALLBACK p_es_callback = (P_ES_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;
    const DIALOG_CTL_ID control_id = p_dialog_msg_ctl_state_change->dialog_control_id;
    const P_ES_WIBBLE p_es_wibble = es_wibble_search(control_id);

    if(P_ES_WIBBLE_NONE != p_es_wibble)
        return((p_es_wibble->enabler ? new_using_es_wibble_enabler : new_using_es_wibble)
                   (p_dialog_msg_ctl_state_change, p_es_wibble, p_es_callback));

    switch(control_id)
    {
    /* fire off the nth subdialog */
    case ES_ID_CATEGORY_GROUP:
        {
        DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
        msgclr(dialog_cmd_complete_dbox);
        dialog_cmd_complete_dbox.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
        dialog_cmd_complete_dbox.completion_code = p_dialog_msg_ctl_state_change->new_state.radiobutton;
        status = call_dialog(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox);
        break;
        }

    default:
        break;
    }

    return(status);
}

_Check_return_
static STATUS
tweak_style_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    STATUS status = STATUS_OK;
    const DIALOG_CTL_ID control_id = p_dialog_msg_ctl_pushbutton->dialog_control_id;

    switch(control_id)
    {
    case ES_FS_ID_COLOUR_BUTTON:
    case ES_PS_ID_RGB_BACK_BUTTON:
    case ES_PS_ID_BORDER_BUTTON:
    case ES_PS_ID_GRID_BUTTON:
        {
        MSG_UISTYLE_COLOUR_PICKER msg_uistyle_colour_picker;
        msg_uistyle_colour_picker.h_dialog = p_dialog_msg_ctl_pushbutton->h_dialog;
        assert_EQ(ES_FS_ID_COLOUR_BUTTON - ES_FS_ID_COLOUR_PATCH, ES_PS_ID_RGB_BACK_BUTTON - ES_PS_ID_RGB_BACK_PATCH);
        assert_EQ(ES_FS_ID_COLOUR_BUTTON - ES_FS_ID_COLOUR_PATCH, ES_PS_ID_BORDER_BUTTON   - ES_PS_ID_BORDER_PATCH);
        assert_EQ(ES_FS_ID_COLOUR_BUTTON - ES_FS_ID_COLOUR_PATCH, ES_PS_ID_GRID_BUTTON     - ES_PS_ID_GRID_PATCH);
        msg_uistyle_colour_picker.rgb_control_id = (DIALOG_CTL_ID) ((control_id - ES_FS_ID_COLOUR_BUTTON) + ES_FS_ID_COLOUR_PATCH);
        msg_uistyle_colour_picker.button_control_id = control_id;
        status = object_call_id(OBJECT_ID_SKEL_SPLIT, P_DOCU_NONE, T5_MSG_UISTYLE_COLOUR_PICKER, &msg_uistyle_colour_picker);
        break;
        }

    default:
        break;
    }

    return(status);
}

_Check_return_
static STATUS
tweak_style_ctl_user_mouse(
    _InRef_     PC_DIALOG_MSG_CTL_USER_MOUSE p_dialog_msg_ctl_user_mouse)
{
    STATUS status = STATUS_OK;
    const DIALOG_CTL_ID control_id = p_dialog_msg_ctl_user_mouse->dialog_control_id;
  /*const P_ES_CALLBACK p_es_callback = p_dialog_msg_ctl_user_mouse->client_handle;*/
    DIALOG_CTL_ID control_id_patch = 0;
    DIALOG_CTL_ID control_id_trans = 0;
    RGB rgb;

    if(p_dialog_msg_ctl_user_mouse->click != DIALOG_MSG_USER_MOUSE_CLICK_LEFT_SINGLE)
        return(status);

    if((control_id >= (DIALOG_CTL_ID) ES_FS_ID_COLOUR_0) && (control_id <= (DIALOG_CTL_ID) ES_FS_ID_COLOUR_15))
    {
        rgb = rgb_stash[control_id - ES_FS_ID_COLOUR_0];

        control_id_patch = ES_FS_ID_COLOUR_PATCH;
    }
    else if((control_id >= (DIALOG_CTL_ID) ES_PS_ID_RGB_BACK_0) && (control_id <= (DIALOG_CTL_ID) ES_PS_ID_RGB_BACK_15))
    {
        rgb = rgb_stash[control_id - ES_PS_ID_RGB_BACK_0];

        control_id_patch = ES_PS_ID_RGB_BACK_PATCH;

        control_id_trans = ES_PS_ID_RGB_BACK_T;
    }
    else if((control_id >= (DIALOG_CTL_ID) ES_PS_ID_BORDER_0) && (control_id <= (DIALOG_CTL_ID) ES_PS_ID_BORDER_15))
    {
        rgb = rgb_stash[control_id - ES_PS_ID_BORDER_0];

        control_id_patch = ES_PS_ID_BORDER_PATCH;
    }
    else if((control_id >= (DIALOG_CTL_ID) ES_PS_ID_GRID_0) && (control_id <= (DIALOG_CTL_ID) ES_PS_ID_GRID_15))
    {
        rgb = rgb_stash[control_id - ES_PS_ID_GRID_0];

        control_id_patch = ES_PS_ID_GRID_PATCH;
    }
    else
    {
        rgb = rgb_stash[0];
    }

    if(control_id_trans)
    {
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
        msgclr(dialog_cmd_ctl_state_set);
        dialog_cmd_ctl_state_set.h_dialog = p_dialog_msg_ctl_user_mouse->h_dialog;
        dialog_cmd_ctl_state_set.bits = 0;
        dialog_cmd_ctl_state_set.control_id = control_id_trans;
        dialog_cmd_ctl_state_set.state.checkbox = (U8) rgb.transparent;
        status = call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set);
    }

    if(control_id_patch)
    {
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
        msgclr(dialog_cmd_ctl_state_set);
        dialog_cmd_ctl_state_set.h_dialog = p_dialog_msg_ctl_user_mouse->h_dialog;
        dialog_cmd_ctl_state_set.bits = 0;
        dialog_cmd_ctl_state_set.control_id = control_id_patch;
        dialog_cmd_ctl_state_set.state.user.rgb = rgb;
        status_accumulate(status, call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
    }

    return(status);
}

#if RISCOS

_Check_return_
static STATUS
tweak_style_ctl_user_redraw(
    _InRef_     PC_DIALOG_MSG_CTL_USER_REDRAW p_dialog_msg_ctl_user_redraw)
{
    const DIALOG_CTL_ID control_id = p_dialog_msg_ctl_user_redraw->dialog_control_id;
    const P_ES_CALLBACK p_es_callback = (P_ES_CALLBACK) p_dialog_msg_ctl_user_redraw->client_handle;
    P_RGB p_rgb;

    if(!p_dialog_msg_ctl_user_redraw->enabled)
        p_rgb = &rgb_stash[0];
    else
    {
        S32 i = (S32) control_id - ES_ID_LIGHT_STT;

        if((i >= 0) && (i < ES_SUBDIALOG_MAX))
        {
            p_rgb = es_light_on_for(p_es_callback, i) ? &rgb_stash[10] /* green */ : &rgb_stash[1]; /* lt grey */
        }
        else switch(control_id)
        {
        case ES_FS_ID_COLOUR_PATCH:
            p_rgb = &p_es_callback->style.font_spec.colour;
            break;

        case ES_PS_ID_RGB_BACK_PATCH:
            p_rgb = &p_es_callback->style.para_style.rgb_back;
            break;

        case ES_PS_ID_BORDER_PATCH:
            p_rgb = &p_es_callback->style.para_style.rgb_border;
            break;

        case ES_PS_ID_GRID_PATCH:
            p_rgb = &p_es_callback->style.para_style.rgb_grid_left;
            break;

        default:
            return(STATUS_OK);
        }
    }

    if(host_setbgcolour(p_rgb))
        host_clg();

    return(STATUS_OK);
}

#endif /* RISCOS */

PROC_DIALOG_EVENT_PROTO(extern, dialog_event_tweak_style)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CREATE:
        return(tweak_style_create(p_docu, (PC_DIALOG_MSG_CREATE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(tweak_style_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_DISPOSE:
        return(tweak_style_dispose());

    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(tweak_style_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(tweak_style_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(tweak_style_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(tweak_style_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    case DIALOG_MSG_CODE_CTL_USER_MOUSE:
        return(tweak_style_ctl_user_mouse((PC_DIALOG_MSG_CTL_USER_MOUSE) p_data));

#if RISCOS
    case DIALOG_MSG_CODE_CTL_USER_REDRAW:
        return(tweak_style_ctl_user_redraw((PC_DIALOG_MSG_CTL_USER_REDRAW) p_data));
#endif

    default:
        return(STATUS_OK);
    }
}

enum ES_WIBBLE_TYPES
{
    ES_WIBBLE_TYPE_ANY = 0,
    ES_WIBBLE_TYPE_RADIO,
    ES_WIBBLE_TYPE_CHECK,
    ES_WIBBLE_TYPE_U8,
    ES_WIBBLE_TYPE_S32,
    ES_WIBBLE_TYPE_F64,
    ES_WIBBLE_TYPE_POINTS,
    ES_WIBBLE_TYPE_H_UNITS,
    ES_WIBBLE_TYPE_V_UNITS,
    ES_WIBBLE_TYPE_V_UNITS_FINE
};

static ES_WIBBLE
es_wibble[] =
{
/*
style name
*/

    { ES_NAME_ID_NAME_ENABLE,             STYLE_SW_NAME,             1, 0, 0 },
    { ES_NAME_ID_NAME,                    STYLE_SW_NAME,             0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, h_style_name_tstr) },

    { ES_KEY_ID_LIST_ENABLE,              STYLE_SW_KEY,              1, 0, 0 },
    { ES_KEY_ID_LIST,                     STYLE_SW_KEY,              0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, style_key) },

/*
character style
*/

    { ES_FS_ID_COLOUR_GROUP_ENABLE,       STYLE_SW_FS_COLOUR,        1, 0, 0 },
    { ES_FS_ID_COLOUR_PATCH,              STYLE_SW_FS_COLOUR,        0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, colour) },
    { ES_FS_ID_COLOUR_R,                  STYLE_SW_FS_COLOUR,        0, 0, 0, 0, ES_WIBBLE_TYPE_U8,           offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, colour) + 0x00 },
    { ES_FS_ID_COLOUR_G,                  STYLE_SW_FS_COLOUR,        0, 0, 0, 0, ES_WIBBLE_TYPE_U8,           offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, colour) + 0x01 },
    { ES_FS_ID_COLOUR_B,                  STYLE_SW_FS_COLOUR,        0, 0, 0, 0, ES_WIBBLE_TYPE_U8,           offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, colour) + 0x02 },

    { ES_FS_ID_TYPEFACE_ENABLE,           STYLE_SW_FS_NAME,          1, 0, 0 },
    { ES_FS_ID_TYPEFACE,                  STYLE_SW_FS_NAME,          0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, h_app_name_tstr) },

    { ES_FS_ID_HEIGHT_ENABLE,             STYLE_SW_FS_SIZE_Y,        1, 0, 1 },
    { ES_FS_ID_HEIGHT,                    STYLE_SW_FS_SIZE_Y,        0, 0, 0, 0, ES_WIBBLE_TYPE_POINTS,       offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, size_y) },

    { ES_FS_ID_WIDTH_ENABLE,              STYLE_SW_FS_SIZE_X,        1, 0, 1 },
    { ES_FS_ID_WIDTH,                     STYLE_SW_FS_SIZE_X,        0, 0, 0, 0, ES_WIBBLE_TYPE_POINTS,       offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, size_x) },

    { ES_FS_ID_BOLD_ENABLE,               STYLE_SW_FS_BOLD,          1, 1, 0 },
    { ES_FS_ID_BOLD,                      STYLE_SW_FS_BOLD,          0, 0, 0, 0, ES_WIBBLE_TYPE_CHECK,        offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, bold) },

    { ES_FS_ID_ITALIC_ENABLE,             STYLE_SW_FS_ITALIC,        1, 1, 0 },
    { ES_FS_ID_ITALIC,                    STYLE_SW_FS_ITALIC,        0, 0, 0, 0, ES_WIBBLE_TYPE_CHECK,        offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, italic) },

    { ES_FS_ID_UNDERLINE_ENABLE,          STYLE_SW_FS_UNDERLINE,     1, 1, 0 },
    { ES_FS_ID_UNDERLINE,                 STYLE_SW_FS_UNDERLINE,     0, 0, 0, 0, ES_WIBBLE_TYPE_CHECK,        offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, underline) },

    { ES_FS_ID_SUPERSCRIPT_ENABLE,        STYLE_SW_FS_SUPERSCRIPT,   1, 1, 0 },
    { ES_FS_ID_SUPERSCRIPT,               STYLE_SW_FS_SUPERSCRIPT,   0, 0, 0, 0, ES_WIBBLE_TYPE_CHECK,        offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, superscript) },

    { ES_FS_ID_SUBSCRIPT_ENABLE,          STYLE_SW_FS_SUBSCRIPT,     1, 1, 0 },
    { ES_FS_ID_SUBSCRIPT,                 STYLE_SW_FS_SUBSCRIPT,     0, 0, 0, 0, ES_WIBBLE_TYPE_CHECK,        offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, subscript) },

/*
paragraph style
*/

    { ES_PS_ID_MARGIN_PARA_ENABLE,        STYLE_SW_PS_MARGIN_PARA,   1, 0, 1 },
    { ES_PS_ID_MARGIN_PARA,               STYLE_SW_PS_MARGIN_PARA,   0, 0, 0, 0, ES_WIBBLE_TYPE_H_UNITS,      offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, margin_para) },

    { ES_PS_ID_MARGIN_LEFT_ENABLE,        STYLE_SW_PS_MARGIN_LEFT,   1, 0, 1 },
    { ES_PS_ID_MARGIN_LEFT,               STYLE_SW_PS_MARGIN_LEFT,   0, 0, 0, 0, ES_WIBBLE_TYPE_H_UNITS,      offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, margin_left) },

    { ES_PS_ID_MARGIN_RIGHT_ENABLE,       STYLE_SW_PS_MARGIN_RIGHT,  1, 0, 1 },
    { ES_PS_ID_MARGIN_RIGHT,              STYLE_SW_PS_MARGIN_RIGHT,  0, 0, 0, 0, ES_WIBBLE_TYPE_H_UNITS,      offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, margin_right) },

    { ES_PS_ID_TAB_LIST_ENABLE,           STYLE_SW_PS_TAB_LIST,      1, 0, 1 },
    { ES_PS_ID_TAB_LIST,                  STYLE_SW_PS_TAB_LIST,      0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, h_tab_list) },

/*
para - horz justify
*/

    { ES_PS_ID_HORZ_JUSTIFY_GROUP_ENABLE, STYLE_SW_PS_JUSTIFY,       1, 0, 0 },
    { ES_PS_ID_HORZ_JUSTIFY_GROUP,        STYLE_SW_PS_JUSTIFY,       0, 0, 0, 0, ES_WIBBLE_TYPE_RADIO,        offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, justify) },

/*
para - vert justify
*/

    { ES_PS_ID_VERT_JUSTIFY_GROUP_ENABLE, STYLE_SW_PS_JUSTIFY_V,     1, 0, 0 },
    { ES_PS_ID_VERT_JUSTIFY_GROUP,        STYLE_SW_PS_JUSTIFY_V,     0, 0, 0, 0, ES_WIBBLE_TYPE_RADIO,        offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, justify_v) },

/*
para - paragraph spacing
*/

    { ES_PS_ID_PARA_START_ENABLE,         STYLE_SW_PS_PARA_START,    1, 0, 1 },
    { ES_PS_ID_PARA_START,                STYLE_SW_PS_PARA_START,    0, 0, 0, 0, ES_WIBBLE_TYPE_V_UNITS_FINE, offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, para_start) },

    { ES_PS_ID_PARA_END_ENABLE,           STYLE_SW_PS_PARA_END,      1, 0, 1 },
    { ES_PS_ID_PARA_END,                  STYLE_SW_PS_PARA_END,      0, 0, 0, 0, ES_WIBBLE_TYPE_V_UNITS_FINE, offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, para_end) },

/*
para - line spacing
*/

    { ES_PS_ID_LINE_SPACE_GROUP_ENABLE,   STYLE_SW_PS_LINE_SPACE,    1, 0, 0 },
    { ES_PS_ID_LINE_SPACE_GROUP,          STYLE_SW_PS_LINE_SPACE,    0, 0, 0, 0, ES_WIBBLE_TYPE_RADIO,        offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, line_space) + offsetof32(LINE_SPACE, type) },
    { ES_PS_ID_LINE_SPACE_N_VAL,          STYLE_SW_PS_LINE_SPACE,    0, 0, 0, 0, ES_WIBBLE_TYPE_POINTS,       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, line_space) + offsetof32(LINE_SPACE, leading) },

/*
para - back colour
*/

    { ES_PS_ID_RGB_BACK_GROUP_ENABLE,     STYLE_SW_PS_RGB_BACK,      1, 0, 0 },
    { ES_PS_ID_RGB_BACK_PATCH,            STYLE_SW_PS_RGB_BACK,      0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_back) },
    { ES_PS_ID_RGB_BACK_R,                STYLE_SW_PS_RGB_BACK,      0, 0, 0, 0, ES_WIBBLE_TYPE_U8,           offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_back) + 0x00 },
    { ES_PS_ID_RGB_BACK_G,                STYLE_SW_PS_RGB_BACK,      0, 0, 0, 0, ES_WIBBLE_TYPE_U8,           offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_back) + 0x01 },
    { ES_PS_ID_RGB_BACK_B,                STYLE_SW_PS_RGB_BACK,      0, 0, 0, 0, ES_WIBBLE_TYPE_U8,           offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_back) + 0x02 },

    { ES_PS_ID_RGB_BACK_T,                STYLE_SW_PS_RGB_BACK,      0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_back) },

/*
para - border
*/

    { ES_PS_ID_BORDER_RGB_GROUP_ENABLE,   STYLE_SW_PS_RGB_BORDER,    1, 0, 0 },
    { ES_PS_ID_BORDER_PATCH,              STYLE_SW_PS_RGB_BORDER,    0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_border) },
    { ES_PS_ID_BORDER_R,                  STYLE_SW_PS_RGB_BORDER,    0, 0, 0, 0, ES_WIBBLE_TYPE_U8,           offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_border) + 0x00 },
    { ES_PS_ID_BORDER_G,                  STYLE_SW_PS_RGB_BORDER,    0, 0, 0, 0, ES_WIBBLE_TYPE_U8,           offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_border) + 0x01 },
    { ES_PS_ID_BORDER_B,                  STYLE_SW_PS_RGB_BORDER,    0, 0, 0, 0, ES_WIBBLE_TYPE_U8,           offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_border) + 0x02 },

    { ES_PS_ID_BORDER_LINE_GROUP_ENABLE,  STYLE_SW_PS_BORDER,        1, 0, 0 },
    { ES_PS_ID_BORDER_LINE_GROUP,         STYLE_SW_PS_BORDER,        0, 0, 0, 0, ES_WIBBLE_TYPE_RADIO,        offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, border) },

/*
para - grid
*/

    { ES_PS_ID_GRID_RGB_GROUP_ENABLE,     STYLE_SW_PS_RGB_GRID_LEFT, 1, 0, 0 },
    { ES_PS_ID_GRID_PATCH,                STYLE_SW_PS_RGB_GRID_LEFT, 0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_grid_left) },
    { ES_PS_ID_GRID_R,                    STYLE_SW_PS_RGB_GRID_LEFT, 0, 0, 0, 0, ES_WIBBLE_TYPE_U8,           offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_grid_left) + 0x00 },
    { ES_PS_ID_GRID_G,                    STYLE_SW_PS_RGB_GRID_LEFT, 0, 0, 0, 0, ES_WIBBLE_TYPE_U8,           offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_grid_left) + 0x01 },
    { ES_PS_ID_GRID_B,                    STYLE_SW_PS_RGB_GRID_LEFT, 0, 0, 0, 0, ES_WIBBLE_TYPE_U8,           offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_grid_left) + 0x02 },

    { ES_PS_ID_GRID_LINE_GROUP_ENABLE,    STYLE_SW_PS_GRID_LEFT,     1, 0, 0 },
    { ES_PS_ID_GRID_LINE_GROUP,           STYLE_SW_PS_GRID_LEFT,     0, 0, 0, 0, ES_WIBBLE_TYPE_RADIO,        offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, grid_left) },

/*
para - protection
*/

    { ES_PS_ID_PROTECTION_ENABLE,         STYLE_SW_PS_PROTECT,       1, 1, 0 },
    { ES_PS_ID_PROTECTION,                STYLE_SW_PS_PROTECT,       0, 0, 0, 0, ES_WIBBLE_TYPE_CHECK,        offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, protect) },

/*
para - new object
*/

    { ES_PS_ID_NEW_OBJECT_LIST_ENABLE,    STYLE_SW_PS_NEW_OBJECT,    1, 0, 0 },
    { ES_PS_ID_NEW_OBJECT_LIST,           STYLE_SW_PS_NEW_OBJECT,    0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, new_object) },

/*
para - numforms
*/

    { ES_PS_ID_NUMFORM_LIST_NU_ENABLE,    STYLE_SW_PS_NUMFORM_NU,    1, 0, 0 },
    { ES_PS_ID_NUMFORM_LIST_NU,           STYLE_SW_PS_NUMFORM_NU,    0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, h_numform_nu) },

    { ES_PS_ID_NUMFORM_LIST_DT_ENABLE,    STYLE_SW_PS_NUMFORM_DT,    1, 0, 0 },
    { ES_PS_ID_NUMFORM_LIST_DT,           STYLE_SW_PS_NUMFORM_DT,    0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, h_numform_dt) },

    { ES_PS_ID_NUMFORM_LIST_SE_ENABLE,    STYLE_SW_PS_NUMFORM_SE,    1, 0, 0 },
    { ES_PS_ID_NUMFORM_LIST_SE,           STYLE_SW_PS_NUMFORM_SE,    0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, h_numform_se) },

/*
row style
*/

    { ES_RS_ID_HEIGHT_ENABLE,             STYLE_SW_RS_HEIGHT,        1, 0, 1 },
    { ES_RS_ID_HEIGHT,                    STYLE_SW_RS_HEIGHT,        0, 0, 0, 0, ES_WIBBLE_TYPE_V_UNITS,      offsetof32(STYLE, row_style) + offsetof32(ROW_STYLE, height) },

    { ES_RS_ID_HEIGHT_FIXED_ENABLE,       STYLE_SW_RS_HEIGHT_FIXED,  1, 1, 0 },
    { ES_RS_ID_HEIGHT_FIXED,              STYLE_SW_RS_HEIGHT_FIXED,  0, 0, 0, 0, ES_WIBBLE_TYPE_CHECK,        offsetof32(STYLE, row_style) + offsetof32(ROW_STYLE, height_fixed) },

    { ES_RS_ID_UNBREAKABLE_ENABLE,        STYLE_SW_RS_UNBREAKABLE,   1, 1, 0 },
    { ES_RS_ID_UNBREAKABLE,               STYLE_SW_RS_UNBREAKABLE,   0, 0, 0, 0, ES_WIBBLE_TYPE_CHECK,        offsetof32(STYLE, row_style) + offsetof32(ROW_STYLE, unbreakable) },

    { ES_RS_ID_ROW_NAME_ENABLE,           STYLE_SW_RS_ROW_NAME,      1, 0, 0 },
    { ES_RS_ID_ROW_NAME,                  STYLE_SW_RS_ROW_NAME,      0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, row_style) + offsetof32(ROW_STYLE, h_numform) },

/*
col style
*/

    { ES_CS_ID_WIDTH_ENABLE,              STYLE_SW_CS_WIDTH,         1, 0, 1 },
    { ES_CS_ID_WIDTH,                     STYLE_SW_CS_WIDTH,         0, 0, 0, 0, ES_WIBBLE_TYPE_H_UNITS,      offsetof32(STYLE, col_style) + offsetof32(COL_STYLE, width) },

    { ES_CS_ID_COL_NAME_ENABLE,           STYLE_SW_CS_COL_NAME,      1, 0, 0 },
    { ES_CS_ID_COL_NAME,                  STYLE_SW_CS_COL_NAME,      0, 0, 0, 0, ES_WIBBLE_TYPE_ANY,          offsetof32(STYLE, col_style) + offsetof32(COL_STYLE, h_numform) }
};

PROC_BSEARCH_PROTO(static, es_wibble_control_id_bsearch, DIALOG_CTL_ID, PC_ES_WIBBLE)
{
    BSEARCH_KEY_VAR_DECL(P_DIALOG_CTL_ID, key);
    const DIALOG_CTL_ID key_id = *key;
    BSEARCH_DATUM_VAR_DECL(PC_ES_WIBBLE, datum);
    const DIALOG_CTL_ID datum_id = (DIALOG_CTL_ID) datum->control_id;

    if(key_id > datum_id)
        return(+1);

    if(key_id < datum_id)
        return(-1);

    return(0);
}

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, es_wibble_control_id_qsort, ES_WIBBLE)
{
    QSORT_ARG1_VAR_DECL(PC_ES_WIBBLE, esw1);
    QSORT_ARG2_VAR_DECL(PC_ES_WIBBLE, esw2);

    const DIALOG_CTL_ID id1 = (DIALOG_CTL_ID) esw1->control_id;
    const DIALOG_CTL_ID id2 = (DIALOG_CTL_ID) esw2->control_id;

    if(id1 > id2)
        return(+1);

    if(id1 < id2)
        return(-1);

    return(0);
}

_Check_return_
_Ret_maybenone_
static P_ES_WIBBLE
es_wibble_search(
    _InVal_     DIALOG_CTL_ID control_id)
{
    const P_ES_WIBBLE p_es_wibble = (P_ES_WIBBLE)
        bsearch(&control_id, es_wibble, elemof(es_wibble), sizeof(es_wibble[0]), es_wibble_control_id_bsearch);

    if(NULL == p_es_wibble)
        return(P_ES_WIBBLE_NONE);

    return(p_es_wibble);
}

static const STATUS
es_tab_list_types[] =
{
    MSG_DIALOG_ES_CS_TAB_L,
    MSG_DIALOG_ES_CS_TAB_C,
    MSG_DIALOG_ES_CS_TAB_R,
    MSG_DIALOG_ES_CS_TAB_D,
};

static TAB_TYPE
es_tab_list_compile_type(
    _In_z_      PC_USTR ustr,
    _OutRef_    P_PC_USTR p_ustr)
{
    /* NB only compares first letter of representation */
    U8 user_type = sbchar_toupper(PtrGetByte(ustr));
    U8 ch;
    TAB_TYPE tab_type;

    /* skip trailing alphas from representation */
    for(;;)
    {
        ustr_IncByte(ustr);

        ch = PtrGetByte(ustr);

        if(!sbchar_isalpha(ch))
        {
            *p_ustr = ustr;
            break;
        }
    }

    for(tab_type = 0; tab_type < (TAB_TYPE) elemof32(es_tab_list_types); ++tab_type)
    {
        PCTSTR tstr_type = resource_lookup_tstr(es_tab_list_types[tab_type]);

        if(*tstr_type == (TCHAR) user_type)
            return(tab_type);
    }

    assert0();
    return(TAB_LEFT);
}

_Check_return_
static inline PC_USTR
es_tab_list_decompile_type(
    TAB_TYPE tab_type)
{
    assert((U32) tab_type < elemof32(es_tab_list_types));
    return(resource_lookup_ustr(es_tab_list_types[tab_type]));
}

_Check_return_
static STATUS
es_tab_list_compile(
    _InRef_     PC_ES_CALLBACK p_es_callback,
    _InoutRef_  P_ARRAY_HANDLE p_h_tab_list /*added*/,
    _InRef_     PC_UI_TEXT p_ui_text)
{
    if(!ui_text_is_blank(p_ui_text))
    {
        PC_USTR ustr = ui_text_ustr(p_ui_text);

        for(;;)
        {
            U8 ch;
            TAB_TYPE tab_type;
            F64 offset;
            PC_USTR new_ustr;

            while(((ch = PtrGetByte(ustr)) == CH_SPACE) || (ch == LF))
                ustr_IncByte(ustr);

            if(CH_NULL == ch)
                break;

            tab_type = es_tab_list_compile_type(ustr, &ustr);

            errno = 0;

            offset = ui_strtod(ustr, &new_ustr);

            if(ustr != new_ustr)
            {
                ustr = new_ustr;

                if(!errno || (offset == 0.0) /* underflow? */)
                {
                    offset *= p_es_callback->info[IDX_HORZ].fp_pixits_per_user_unit;

                    if(fabs(offset) <= S32_MAX)
                    {
                        TAB_INFO tab_info;
                        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(tab_info), FALSE);
                        tab_info.type = tab_type;
                        tab_info.offset = (S32) offset;
                        if(status_fail(al_array_add(p_h_tab_list, TAB_INFO, 1, &array_init_block, &tab_info)))
                        {
                            al_array_dispose(p_h_tab_list);
                            return(status_nomem());
                        }
                    }
                }
            }

            /* skip over any unrecognized debris */
            while(((ch = PtrGetByte(ustr)) != CH_SPACE) && (ch != LF) && (CH_NULL != ch))
                ustr_IncByte(ustr);

            if(CH_NULL == ch)
                break;
        }
    }

    al_array_qsort(p_h_tab_list, es_tab_list_compile_qsort);

    return(STATUS_OK);
}

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, es_tab_list_compile_qsort, TAB_INFO)
{
    QSORT_ARG1_VAR_DECL(PC_TAB_INFO, p_tab_info_1);
    QSORT_ARG2_VAR_DECL(PC_TAB_INFO, p_tab_info_2);

    if(p_tab_info_1->offset > p_tab_info_2->offset)
        return(+1);

    if(p_tab_info_1->offset < p_tab_info_2->offset)
        return(-1);

    return(0);
}

_Check_return_
static STATUS
es_tab_list_decompile(
    _InRef_     PC_ES_CALLBACK p_es_callback,
    _InRef_     PC_ARRAY_HANDLE p_h_tab_list,
    _OutRef_    P_UI_TEXT p_ui_text)
{
    P_USTR ustr;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(16, sizeof32(UCHARZ), TRUE);
    ARRAY_INDEX i;
    STATUS status;

    p_ui_text->type = UI_TEXT_TYPE_USTR_ARRAY;
    p_ui_text->text.array_handle_ustr = 0;

    status = STATUS_OK;

    for(i = 0; i < array_elements(p_h_tab_list); ++i)
    {
        PC_TAB_INFO p_tab_list = array_ptrc(p_h_tab_list, TAB_INFO, i);
        PC_USTR ustr_type = es_tab_list_decompile_type(p_tab_list->type);
        const U32 l_type = ustrlen32(ustr_type);
        EV_DATA ev_data;
        NUMFORM_PARMS numform_parms;
        QUICK_UBLOCK quick_ublock;

        if(NULL == (ustr = (P_USTR) al_array_extend_by_U8(&p_ui_text->text.array_handle_ustr, l_type, &array_init_block, &status)))
            break;

        if(i)
            PtrPutByteOff(ustr, -1, LF); /* overwrite previous CH_NULL terminator with separator */

        memcpy32(ustr, ustr_type, l_type);

        ev_data_set_real(&ev_data, /*FP_USER_UNIT*/ ((FP_PIXIT) p_tab_list->offset / p_es_callback->info[IDX_HORZ].fp_pixits_per_user_unit));

        zero_struct(numform_parms);
        numform_parms.ustr_numform_numeric = (PC_USTR) (p_es_callback->info[IDX_HORZ].normal.user_unit_numform_ustr_buf);

        quick_ublock_setup_using_array(&quick_ublock, p_ui_text->text.array_handle_ustr);

        if(status_fail(status = numform(&quick_ublock, P_QUICK_TBLOCK_NONE, &ev_data, &numform_parms)))
            break;
    }

    if(status_fail(status))
    {
        status_assertc(status);
        p_ui_text->type = UI_TEXT_TYPE_NONE;
        al_array_dispose(&p_ui_text->text.array_handle_ustr);
    }

    return(STATUS_OK);
}

static void
es_tab_list_validation_setup(
    /*out*/ P_BITMAP p_bitmap_validation)
{
    PC_SBSTR sbstr = " .+-0123456789\n";
    SBCHAR sbchar;
    TAB_TYPE tab_type;

    bitmap_clear(p_bitmap_validation, N_BITS_ARG(256));

    while((sbchar = *sbstr++) != CH_NULL)
        bitmap_bit_set(p_bitmap_validation, sbchar, N_BITS_ARG(256));

    for(tab_type = 0; tab_type < (TAB_TYPE) elemof32(es_tab_list_types); ++tab_type)
    {
        PCTSTR tstr = resource_lookup_tstr(es_tab_list_types[tab_type]);
        TCHAR tchar = *tstr;

        if(ucs4_is_sbchar(tchar))
        {
            sbchar = (SBCHAR) tchar;

            bitmap_bit_set(p_bitmap_validation, sbchar_toupper(sbchar), N_BITS_ARG(256));
            bitmap_bit_set(p_bitmap_validation, sbchar_tolower(sbchar), N_BITS_ARG(256));
        }
    }
}

_Check_return_
static STATUS
create_using_es_wibble(
    P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state,
    P_ES_WIBBLE p_es_wibble,
    P_ES_CALLBACK p_es_callback)
{
    const DIALOG_CTL_ID control_id = (DIALOG_CTL_ID) p_es_wibble->control_id;
    P_ANY p_member = PtrAddBytes(P_ANY, &p_es_callback->style, p_es_wibble->member_offset);
    STYLE_BIT_NUMBER style_bit_number = ENUM_UNPACK(STYLE_BIT_NUMBER, p_es_wibble->style_bit_number);
    STATUS discard_edit = 0;
    STATUS discard_list_text = 0;
    STATUS status;

    p_dialog_msg_ctl_create_state->processed = 1;

    /* some style bits are used multiple times, eg. RGB values, so only copy in once per subdialog */
    if(!style_bit_test(&p_es_callback->style, style_bit_number))
    {
        STYLE_SELECTOR selector;

        /* dup this one bit of style */
        style_selector_clear(&selector);
        style_selector_bit_set(&selector, style_bit_number);

        if((control_id == ES_PS_ID_GRID_LINE_GROUP) || (control_id == ES_PS_ID_GRID_PATCH)) /* MUST be first control to use this style bit to be created */
        {   /* dup our friends bits of style too */
            U32 i;
            for(i = 1; i <= 3; ++i)
                style_selector_bit_set(&selector, style_bit_number + i);
        }

        status_return(style_duplicate(&p_es_callback->style, &p_es_callback->committed_style, &selector));

        /* copy over this one bit of enable state */
        style_selector_bit_copy(&p_es_callback->style_selector, &p_es_callback->committed_style_selector, style_bit_number);

        if((control_id == ES_PS_ID_GRID_LINE_GROUP) || (control_id == ES_PS_ID_GRID_PATCH))
        {   /* copy over our friends bits of enable state too */
            U32 i;
            for(i = 1; i <= 3; ++i)
                style_selector_bit_copy(&p_es_callback->style_selector, &p_es_callback->committed_style_selector, style_bit_number + i);
        }
    }

    switch(p_es_wibble->data_type)
    {
    default: default_unhandled();
#if CHECKING
    case ES_WIBBLE_TYPE_ANY:
#endif
        break;

    case ES_WIBBLE_TYPE_RADIO:
        p_dialog_msg_ctl_create_state->state_set.state.radiobutton = * (PC_U8)  p_member;
        break;

    case ES_WIBBLE_TYPE_CHECK:
        p_dialog_msg_ctl_create_state->state_set.state.checkbox = * (PC_U8)  p_member;
        break;

    case ES_WIBBLE_TYPE_U8:
        p_dialog_msg_ctl_create_state->state_set.state.bump_s32 = * (PC_U8)  p_member;
        break;

    case ES_WIBBLE_TYPE_S32:
        p_dialog_msg_ctl_create_state->state_set.state.bump_s32 = * (PC_S32) p_member;
        break;

    case ES_WIBBLE_TYPE_F64:
        p_dialog_msg_ctl_create_state->state_set.state.bump_f64 = * (PC_F64) p_member;
        break;

    case ES_WIBBLE_TYPE_POINTS:
        /* translate in UI to what punter expects to see, in this case points, not pixits or units */
        p_dialog_msg_ctl_create_state->state_set.state.bump_f64 =
            /*FP_POINTS*/ (((FP_PIXIT) (* (PC_PIXIT) p_member)) / PIXITS_PER_POINT);
        break;

    case ES_WIBBLE_TYPE_H_UNITS:
        /* translate in UI to what punter expects to see, in this case h user units, not pixits */
        p_dialog_msg_ctl_create_state->state_set.state.bump_f64 =
            /*FP_USER_UNIT*/ (((FP_PIXIT) (* (PC_PIXIT) p_member)) / p_es_callback->info[IDX_HORZ].fp_pixits_per_user_unit);
        break;

    case ES_WIBBLE_TYPE_V_UNITS:
    case ES_WIBBLE_TYPE_V_UNITS_FINE:
        /* translate in UI to what punter expects to see, in this case v user units, not pixits */
        p_dialog_msg_ctl_create_state->state_set.state.bump_f64 =
            /*FP_USER_UNIT*/ (((FP_PIXIT) (* (PC_PIXIT) p_member)) / p_es_callback->info[IDX_VERT].fp_pixits_per_user_unit);
        break;
    }

    switch(control_id)
    {
    case ES_NAME_ID_NAME:
    case ES_CS_ID_COL_NAME:
    case ES_RS_ID_ROW_NAME:
#if 0
        /* borrow the handle to some text */
        p_dialog_msg_ctl_create_state->state_set.state.edit.ui_text.type = UI_TEXT_TYPE_USTR_ARRAY;
        p_dialog_msg_ctl_create_state->state_set.state.edit.ui_text.text.array_handle_ustr = * (PC_ARRAY_HANDLE) p_member;
        break;
#endif

        /*FALLTHRU*/

    case ES_PS_ID_NUMFORM_LIST_NU:
    case ES_PS_ID_NUMFORM_LIST_DT:
    case ES_PS_ID_NUMFORM_LIST_SE:
        /* borrow the handle to some text */
        p_dialog_msg_ctl_create_state->state_set.state.combo_text.ui_text.type = UI_TEXT_TYPE_USTR_ARRAY;
        p_dialog_msg_ctl_create_state->state_set.state.combo_text.ui_text.text.array_handle_ustr = * (PC_ARRAY_HANDLE) p_member;
        break;

    case ES_KEY_ID_LIST:
        {
        KMAP_CODE kmap_code = (KMAP_CODE) (* (PC_S32) p_member);
        P_ARRAY_HANDLE p_array_handle = &style_key_list_handle;
        ARRAY_INDEX i;

        p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = DIALOG_CTL_STATE_LIST_ITEM_NONE;
        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALTERNATE;

        for(i = 0; i < array_elements(p_array_handle); ++i)
            if(array_ptr(p_array_handle, UI_LIST_ENTRY_STYLE_KEY, i)->kmap_code == kmap_code)
            {
                p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = i;
                break;
            }

        break;
        }

    case ES_FS_ID_TYPEFACE:
        {
        PC_ARRAY_HANDLE_TSTR p_array_handle_tstr = (PC_ARRAY_HANDLE_TSTR) p_member;
        PCTSTR tstr_app_font_name = array_tstr(p_array_handle_tstr);
        ARRAY_HANDLE_TSTR h_host_font_name_tstr;

        /* translate in UI to what punter expects to see on the host system */
        if((PTSTR_NONE != tstr_app_font_name) && status_ok(fontmap_host_base_name_from_app_font_name(&h_host_font_name_tstr, tstr_app_font_name)))
        {
            /* borrow the handle to some text */
            p_dialog_msg_ctl_create_state->state_set.state.list_text.ui_text.type = UI_TEXT_TYPE_TSTR_ARRAY;
            p_dialog_msg_ctl_create_state->state_set.state.list_text.ui_text.text.array_handle_tstr = h_host_font_name_tstr;

            discard_list_text = 1;
        }
        else
        {
            p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = DIALOG_CTL_STATE_LIST_ITEM_NONE;
            p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALTERNATE;
        }

        break;
        }

    case ES_PS_ID_TAB_LIST:
        {
        PC_ARRAY_HANDLE p_array_handle = (PC_ARRAY_HANDLE) p_member;
        PC_DIALOG_CONTROL_DATA_EDIT p_dialog_control_data_edit = (PC_DIALOG_CONTROL_DATA_EDIT) p_dialog_msg_ctl_create_state->p_dialog_control_data;
        es_tab_list_validation_setup((P_BITMAP) p_dialog_control_data_edit->edit_xx.p_bitmap_validation);
        status_return(es_tab_list_decompile(p_es_callback, p_array_handle, &p_dialog_msg_ctl_create_state->state_set.state.edit.ui_text));
        discard_edit = 1;
        break;
        }

    case ES_PS_ID_NEW_OBJECT_LIST:
        {
        U8 style_object_id = * (PC_U8) p_member;
        P_ARRAY_HANDLE p_array_handle = &new_object_list_handle;
        ARRAY_INDEX i;

        p_dialog_msg_ctl_create_state->state_set.state.combo_text.itemno = DIALOG_CTL_STATE_LIST_ITEM_NONE;
        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALTERNATE;

        for(i = 0; i < array_elements(p_array_handle); ++i)
            if(array_ptr(p_array_handle, UI_LIST_ENTRY_NEW_OBJECT, i)->object_id == style_object_id)
            {
                p_dialog_msg_ctl_create_state->state_set.state.combo_text.itemno = i;
                break;
            }
        break;
        }

    case ES_PS_ID_RGB_BACK_T:
        {
        PC_RGB p_rgb = (PC_RGB) p_member;

        {
        DIALOG_CMD_CTL_ENABLE dialog_cmd_ctl_enable;
        msgclr(dialog_cmd_ctl_enable);
        dialog_cmd_ctl_enable.h_dialog = p_dialog_msg_ctl_create_state->h_dialog;
        dialog_cmd_ctl_enable.control_id = ES_PS_ID_RGB_BACK_GROUP_INNER;
        dialog_cmd_ctl_enable.enabled = !p_rgb->transparent;
        status_assert(call_dialog(DIALOG_CMD_CODE_CTL_ENABLE, &dialog_cmd_ctl_enable));
        } /*block*/

        p_dialog_msg_ctl_create_state->state_set.state.checkbox = (U8) p_rgb->transparent;

        break;
        }

    case ES_FS_ID_COLOUR_PATCH:
    case ES_PS_ID_RGB_BACK_PATCH:
    case ES_PS_ID_BORDER_PATCH:
    case ES_PS_ID_GRID_PATCH:
        {
        PC_RGB p_rgb = (PC_RGB) p_member;
        p_dialog_msg_ctl_create_state->state_set.state.user.rgb = *p_rgb;
        break;
        }

    case ES_PS_ID_LINE_SPACE_N_VAL:
        if(p_es_callback->style.para_style.line_space.type != SF_LINE_SPACE_SET)
        {
            PIXIT size_y = 12 * PIXITS_PER_POINT;
            if(style_selector_bit_test(&p_es_callback->style_selector, STYLE_SW_FS_SIZE_Y))
                size_y = p_es_callback->style.font_spec.size_y;
            /* translate in UI to what punter expects to see, in this case points, not pixits or units */
            p_dialog_msg_ctl_create_state->state_set.state.bump_f64 = (size_y * 120.0) / (100.0 * PIXITS_PER_POINT);
        }

        break;
    }

    p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_DONT_MSG;

    status = call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &p_dialog_msg_ctl_create_state->state_set);

    if(discard_edit)
        ui_text_dispose(&p_dialog_msg_ctl_create_state->state_set.state.edit.ui_text);

    if(discard_list_text)
        ui_text_dispose(&p_dialog_msg_ctl_create_state->state_set.state.list_text.ui_text);

    return(status);
}

_Check_return_
static STATUS
create_using_es_wibble_enabler(
    P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state,
    P_ES_WIBBLE p_es_wibble,
    P_ES_CALLBACK p_es_callback)
{
    STYLE_BIT_NUMBER style_bit_number = ENUM_UNPACK(STYLE_BIT_NUMBER, p_es_wibble->style_bit_number);
    BOOL enabled = style_selector_bit_test(&p_es_callback->style_selector, style_bit_number);

    if((p_es_wibble->control_id == ES_PS_ID_GRID_LINE_GROUP_ENABLE) || (p_es_wibble->control_id == ES_PS_ID_GRID_RGB_GROUP_ENABLE))
    {
        /* get our friends opinions too (enabled iff unanimous) */
        if(!style_selector_bit_test(&p_es_callback->style_selector, style_bit_number + 1))
            enabled = 0;
        if(!style_selector_bit_test(&p_es_callback->style_selector, style_bit_number + 2))
            enabled = 0;
        if(!style_selector_bit_test(&p_es_callback->style_selector, style_bit_number + 3))
            enabled = 0;
    }

    if(style_selector_bit_test(&p_es_callback->prohibited_enabler, style_bit_number))
    {
        DIALOG_CMD_CTL_ENABLE dialog_cmd_ctl_enable;

        msgclr(dialog_cmd_ctl_enable);

        dialog_cmd_ctl_enable.h_dialog = p_dialog_msg_ctl_create_state->h_dialog;
        dialog_cmd_ctl_enable.control_id = (DIALOG_CTL_ID) p_es_wibble->control_id;
        dialog_cmd_ctl_enable.enabled = 0;

        status_assert(call_dialog(DIALOG_CMD_CODE_CTL_ENABLE, &dialog_cmd_ctl_enable));
    }

    if(style_selector_bit_test(&p_es_callback->prohibited_enabler_2, style_bit_number))
    {
        DIALOG_CMD_CTL_NOBBLE dialog_cmd_ctl_nobble;

        msgclr(dialog_cmd_ctl_nobble);

        dialog_cmd_ctl_nobble.h_dialog = p_dialog_msg_ctl_create_state->h_dialog;
        dialog_cmd_ctl_nobble.control_id = (DIALOG_CTL_ID) p_es_wibble->control_id;
        dialog_cmd_ctl_nobble.nobbled = 1;

        status_assert(call_dialog(DIALOG_CMD_CODE_CTL_NOBBLE, &dialog_cmd_ctl_nobble));
    }

    /* on enable control creation, set the state of the enable control which
     * will (below) enable/disable the group or control (which must be created first - no longer true) initially
    */
    p_dialog_msg_ctl_create_state->processed = 1;

    p_dialog_msg_ctl_create_state->state_set.bits = DIALOG_STATE_SET_ALWAYS_MSG;

    p_dialog_msg_ctl_create_state->state_set.state.checkbox = (U8) (enabled ? DIALOG_BUTTONSTATE_ON : DIALOG_BUTTONSTATE_OFF);

    p_es_callback->creating++;

    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &p_dialog_msg_ctl_create_state->state_set));

    p_es_callback->creating--;

    return(STATUS_OK);
}

_Check_return_
static STATUS
fill_using_es_wibble(
    P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source,
    P_ES_WIBBLE p_es_wibble,
    P_ES_CALLBACK p_es_callback)
{
    IGNOREPARM(p_es_callback);

    switch(p_es_wibble->control_id)
    {
    case ES_PS_ID_NUMFORM_LIST_NU:
        p_dialog_msg_ctl_fill_source->p_ui_source = &numform_list_nu_source;
        break;

    case ES_PS_ID_NUMFORM_LIST_DT:
        p_dialog_msg_ctl_fill_source->p_ui_source = &numform_list_dt_source;
        break;

    case ES_PS_ID_NUMFORM_LIST_SE:
        p_dialog_msg_ctl_fill_source->p_ui_source = &numform_list_se_source;
        break;

    case ES_KEY_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &style_key_list_source;
        break;

    case ES_FS_ID_TYPEFACE:
        p_dialog_msg_ctl_fill_source->p_ui_source = &typeface_list_source;
        break;

    case ES_PS_ID_NEW_OBJECT_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &new_object_list_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* punter has gone playing with state, so record it
*
******************************************************************************/

_Check_return_
static STATUS
new_using_es_wibble(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change,
    P_ES_WIBBLE p_es_wibble,
    P_ES_CALLBACK p_es_callback)
{
    DIALOG_CTL_ID control_id = (DIALOG_CTL_ID) p_es_wibble->control_id;
    P_ANY p_member = PtrAddBytes(P_ANY, &p_es_callback->style, p_es_wibble->member_offset);
    STYLE_BIT_NUMBER style_bit_number = ENUM_UNPACK(STYLE_BIT_NUMBER, p_es_wibble->style_bit_number);
    F64 f64;

    /* SKS 04jul93 after 1.03 - note the fact that the state has this attribute (we're poking the member) */
    style_bit_set(&p_es_callback->style, style_bit_number);

    /* note the fact that the state has been modified */
    style_selector_bit_set(&p_es_callback->style_modified, style_bit_number);

    switch(p_es_wibble->data_type)
    {
    default: default_unhandled();
#if CHECKING
    case ES_WIBBLE_TYPE_ANY:
#endif
        break;

    case ES_WIBBLE_TYPE_RADIO:
        assert((U32) p_dialog_msg_ctl_state_change->new_state.radiobutton <= 255);
        * (P_U8)  p_member = (U8) p_dialog_msg_ctl_state_change->new_state.radiobutton;
        break;

    case ES_WIBBLE_TYPE_CHECK:
        * (P_U8)  p_member = p_dialog_msg_ctl_state_change->new_state.checkbox;
        break;

    case ES_WIBBLE_TYPE_U8:
        assert(p_dialog_msg_ctl_state_change->p_dialog_control->bits.type == DIALOG_CONTROL_BUMP_S32);
        * (P_U8)  p_member = (U8) p_dialog_msg_ctl_state_change->new_state.bump_s32;
        break;

    case ES_WIBBLE_TYPE_S32:
        assert(p_dialog_msg_ctl_state_change->p_dialog_control->bits.type == DIALOG_CONTROL_BUMP_S32);
        * (P_S32) p_member = p_dialog_msg_ctl_state_change->new_state.bump_s32;
        break;

    case ES_WIBBLE_TYPE_F64:
        assert(p_dialog_msg_ctl_state_change->p_dialog_control->bits.type == DIALOG_CONTROL_BUMP_F64);
        * (P_F64) p_member = p_dialog_msg_ctl_state_change->new_state.bump_f64;
        break;

    case ES_WIBBLE_TYPE_POINTS:
        /* translate from what punter expects to see, in this case points, not pixits or units */
        assert(p_dialog_msg_ctl_state_change->p_dialog_control->bits.type == DIALOG_CONTROL_BUMP_F64);
        f64 = /*FP_PIXIT*/ (p_dialog_msg_ctl_state_change->new_state.bump_f64 * PIXITS_PER_POINT + 0.5);
        * (P_PIXIT) p_member = (fabs(f64) <= S32_MAX) ? (PIXIT) f64 : 0;
        break;

    case ES_WIBBLE_TYPE_H_UNITS:
        /* translate from what punter expects to see, in this case user units, not pixits */
        assert(p_dialog_msg_ctl_state_change->p_dialog_control->bits.type == DIALOG_CONTROL_BUMP_F64);
        f64 = /*FP_PIXIT*/ (p_dialog_msg_ctl_state_change->new_state.bump_f64 * p_es_callback->info[IDX_HORZ].fp_pixits_per_user_unit + 0.5);
        * (P_PIXIT) p_member = (fabs(f64) <= S32_MAX) ? (PIXIT) f64 : 0;
        break;

    case ES_WIBBLE_TYPE_V_UNITS:
    case ES_WIBBLE_TYPE_V_UNITS_FINE:
        /* translate from what punter expects to see, in this case user units, not pixits */
        assert(p_dialog_msg_ctl_state_change->p_dialog_control->bits.type == DIALOG_CONTROL_BUMP_F64);
        f64 = /*FP_PIXIT*/ (p_dialog_msg_ctl_state_change->new_state.bump_f64 * p_es_callback->info[IDX_VERT].fp_pixits_per_user_unit + 0.5);
        * (P_PIXIT) p_member = (fabs(f64) <= S32_MAX) ? (PIXIT) f64 : 0;
        break;
    }

    switch(control_id)
    {
    case ES_NAME_ID_NAME:
    case ES_CS_ID_COL_NAME:
    case ES_RS_ID_ROW_NAME:
#if 0
        {
        P_ARRAY_HANDLE p_array_handle = (P_ARRAY_HANDLE) p_member;

        al_array_dispose(p_array_handle);

        status_return(al_tstr_set(p_array_handle, ui_text_tstr(&p_dialog_msg_ctl_state_change->new_state.edit.ui_text)));

        break;
        }
#endif

        /*FALLTHRU*/

    case ES_PS_ID_NUMFORM_LIST_NU:
    case ES_PS_ID_NUMFORM_LIST_DT:
    case ES_PS_ID_NUMFORM_LIST_SE:
        {
        P_ARRAY_HANDLE p_array_handle = (P_ARRAY_HANDLE) p_member;

        al_array_dispose(p_array_handle);

        status_return(al_tstr_set(p_array_handle, ui_text_tstr(&p_dialog_msg_ctl_state_change->new_state.combo_text.ui_text)));

        break;
        }

    case ES_KEY_ID_LIST:
        {
        PCTSTR tstr = ui_text_tstr(&p_dialog_msg_ctl_state_change->new_state.list_text.ui_text);
        ARRAY_INDEX i;

        * (P_S32) p_member = 0;

        for(i = 0; i < array_elements(&style_key_list_handle); ++i)
        {
            P_UI_LIST_ENTRY_STYLE_KEY p_ui_list_entry_style_key = array_ptr(&style_key_list_handle, UI_LIST_ENTRY_STYLE_KEY, i);

            if(0 == tstrcmp(quick_tblock_tstr(&p_ui_list_entry_style_key->quick_tblock), tstr))
            {
                * (P_S32) p_member = p_ui_list_entry_style_key->kmap_code;
                break;
            }
        }

        break;
        }

    case ES_FS_ID_TYPEFACE:
        {
        P_ARRAY_HANDLE_TSTR p_array_handle_tstr = (P_ARRAY_HANDLE_TSTR) p_member;
        FONT_SPEC font_spec;
        STATUS status;

        al_array_dispose(p_array_handle_tstr);

        /* translate from what punter expects to see on the host system */
        if(status_ok(status = create_error(fontmap_font_spec_from_host_base_name(&font_spec, ui_text_tstr(&p_dialog_msg_ctl_state_change->new_state.list_text.ui_text)))))
            *p_array_handle_tstr = font_spec.h_app_name_tstr;
        else if(status != STATUS_FAIL)
            return(status);

        break;
        }

    case ES_PS_ID_TAB_LIST:
        {
        P_ARRAY_HANDLE p_array_handle = (P_ARRAY_HANDLE) p_member;

        al_array_dispose(p_array_handle);

        return(es_tab_list_compile(p_es_callback, p_array_handle, &p_dialog_msg_ctl_state_change->new_state.edit.ui_text));
        }

    case ES_PS_ID_NEW_OBJECT_LIST:
        {
        P_U8 p_object_id = (P_U8) p_member;
        ARRAY_INDEX i = p_dialog_msg_ctl_state_change->new_state.combo_text.itemno;

        *p_object_id = OBJECT_ID_TEXT;

        if(array_index_valid(&new_object_list_handle, i))
        {
            P_UI_LIST_ENTRY_NEW_OBJECT p_ui_list_entry_new_object = array_ptr_no_checks(&new_object_list_handle, UI_LIST_ENTRY_NEW_OBJECT, i);

            *p_object_id = (U8) p_ui_list_entry_new_object->object_id;
        }

        break;
        }

    case ES_PS_ID_RGB_BACK_T:
        {
        P_RGB p_rgb = (P_RGB) p_member;

        p_rgb->transparent = (U8) (p_dialog_msg_ctl_state_change->new_state.checkbox == DIALOG_BUTTONSTATE_ON);

        {
        DIALOG_CMD_CTL_ENABLE dialog_cmd_ctl_enable;
        msgclr(dialog_cmd_ctl_enable);
        dialog_cmd_ctl_enable.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
        dialog_cmd_ctl_enable.control_id = ES_PS_ID_RGB_BACK_GROUP_INNER;
        dialog_cmd_ctl_enable.enabled = !p_rgb->transparent;
        status_assert(call_dialog(DIALOG_CMD_CODE_CTL_ENABLE, &dialog_cmd_ctl_enable));
        } /*block*/

        control_id += ES_PS_ID_RGB_BACK_B;
        control_id -= ES_PS_ID_RGB_BACK_T;
        p_member = (P_U8) p_member + 0x02;
        }

        /*FALLTHRU*/

    case ES_FS_ID_COLOUR_B:
    case ES_PS_ID_RGB_BACK_B:
    case ES_PS_ID_BORDER_B:
    case ES_PS_ID_GRID_B:
        control_id += ES_FS_ID_COLOUR_G;
        control_id -= ES_FS_ID_COLOUR_B;
        p_member = (P_U8) p_member - 1;

        /*FALLTHRU*/

    case ES_FS_ID_COLOUR_G:
    case ES_PS_ID_RGB_BACK_G:
    case ES_PS_ID_BORDER_G:
    case ES_PS_ID_GRID_G:
        control_id += ES_FS_ID_COLOUR_R;
        control_id -= ES_FS_ID_COLOUR_G;
        p_member = (P_U8) p_member - 1;

        /*FALLTHRU*/

    case ES_FS_ID_COLOUR_R:
    case ES_PS_ID_RGB_BACK_R:
    case ES_PS_ID_BORDER_R:
    case ES_PS_ID_GRID_R:
        {
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
        msgclr(dialog_cmd_ctl_state_set);
        dialog_cmd_ctl_state_set.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
        dialog_cmd_ctl_state_set.control_id = (DIALOG_CTL_ID) (control_id + (ES_FS_ID_COLOUR_PATCH - ES_FS_ID_COLOUR_R));
        dialog_cmd_ctl_state_set.bits = 0;
        dialog_cmd_ctl_state_set.state.user.rgb = * (PC_RGB) p_member;
        status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
        break;
        }

    case ES_FS_ID_COLOUR_PATCH:
    case ES_PS_ID_RGB_BACK_PATCH:
    case ES_PS_ID_BORDER_PATCH:
    case ES_PS_ID_GRID_PATCH:
        {
        RGB rgb = p_dialog_msg_ctl_state_change->new_state.user.rgb;

        * (P_RGB) p_member = rgb;

        {
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;

        msgclr(dialog_cmd_ctl_state_set);

        dialog_cmd_ctl_state_set.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
        dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_DONT_MSG;

        dialog_cmd_ctl_state_set.control_id = (DIALOG_CTL_ID) ((control_id - ES_FS_ID_COLOUR_PATCH) + ES_FS_ID_COLOUR_R);
        dialog_cmd_ctl_state_set.state.bump_s32 = rgb.r;
        status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));

        dialog_cmd_ctl_state_set.control_id = (DIALOG_CTL_ID) ((control_id - ES_FS_ID_COLOUR_PATCH) + ES_FS_ID_COLOUR_G);
        dialog_cmd_ctl_state_set.state.bump_s32 = rgb.g;
        status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));

        dialog_cmd_ctl_state_set.control_id = (DIALOG_CTL_ID) ((control_id - ES_FS_ID_COLOUR_PATCH) + ES_FS_ID_COLOUR_B);
        dialog_cmd_ctl_state_set.state.bump_s32 = rgb.b;
        status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
        } /*block*/

        if(control_id == ES_PS_ID_GRID_PATCH)
        {   /* set my friends up too */
            U32 i;

            p_es_callback->style.para_style.rgb_grid_right =
            p_es_callback->style.para_style.rgb_grid_top =
            p_es_callback->style.para_style.rgb_grid_bottom = rgb;

            for(i = 1; i <= 3; ++i)
                style_selector_bit_set(&p_es_callback->style_modified, style_bit_number + i);
        }

        break;
        }

    case ES_PS_ID_GRID_LINE_GROUP:
        { /* set my friends up too */
        U32 i;

        p_es_callback->style.para_style.grid_right =
        p_es_callback->style.para_style.grid_top =
        p_es_callback->style.para_style.grid_bottom = * (PC_U8) p_member;

        for(i = 1; i <= 3; ++i)
            style_selector_bit_set(&p_es_callback->style_modified, style_bit_number + i);

        break;
        }

    case ES_PS_ID_LINE_SPACE_GROUP:
        if(p_es_callback->style.para_style.line_space.type == SF_LINE_SPACE_SET)
        {
            /* read what's currently set back in as we've only set one piece of the line_space structure */
            p_member = &p_es_callback->style.para_style.line_space.leading;
            ui_dlg_get_f64(p_dialog_msg_ctl_state_change->h_dialog, ES_PS_ID_LINE_SPACE_N_VAL, &f64);
            f64 = f64 * PIXITS_PER_POINT + 0.5;
            * (P_S32) p_member = (fabs(f64) <= S32_MAX) ? (S32) f64 : 0;
        }
        break;

    case ES_PS_ID_LINE_SPACE_N_VAL:
        status_return(ui_dlg_set_radio(p_dialog_msg_ctl_state_change->h_dialog, ES_PS_ID_LINE_SPACE_GROUP, SF_LINE_SPACE_SET));
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* enable/disable corresponding control or group of controls on enable control change
*
******************************************************************************/

_Check_return_
static STATUS
new_using_es_wibble_enabler(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change,
    P_ES_WIBBLE p_es_wibble,
    P_ES_CALLBACK p_es_callback)
{
    BOOL enabled = (p_dialog_msg_ctl_state_change->new_state.checkbox == DIALOG_BUTTONSTATE_ON);
    STYLE_BIT_NUMBER style_bit_number = ENUM_UNPACK(STYLE_BIT_NUMBER, p_es_wibble->style_bit_number);

    /* set the style's selector bit to the new enable state */
    if(enabled)
        style_selector_bit_set(&p_es_callback->style_selector, style_bit_number);
    else
        style_selector_bit_clear(&p_es_callback->style_selector, style_bit_number);

    if((p_es_wibble->control_id == ES_PS_ID_GRID_LINE_GROUP_ENABLE) || (p_es_wibble->control_id == ES_PS_ID_GRID_RGB_GROUP_ENABLE))
    {   /* hack our friends too */
        U32 i;
        for(i = 1; i <= 3; ++i)
        {
            if(enabled)
                style_selector_bit_set(&p_es_callback->style_selector, style_bit_number + i);
            else
                style_selector_bit_clear(&p_es_callback->style_selector, style_bit_number + i);
        }
    }

    if(!p_es_callback->creating)
    {
        /* note the fact that the selector has been modified */
        style_selector_bit_set(&p_es_callback->style_selector_modified, style_bit_number);

        if((p_es_wibble->control_id == ES_PS_ID_GRID_LINE_GROUP_ENABLE) || (p_es_wibble->control_id == ES_PS_ID_GRID_RGB_GROUP_ENABLE))
        {   /* hack our friends too */
            U32 i;
            for(i = 1; i <= 3; ++i)
                style_selector_bit_set(&p_es_callback->style_selector_modified, style_bit_number + i);
        }
    }

    {
    DIALOG_CMD_CTL_ENABLE dialog_cmd_ctl_enable;
    const DIALOG_CTL_ID state_control_id = (DIALOG_CTL_ID) (p_dialog_msg_ctl_state_change->dialog_control_id - 1);

    assert_EQ(ES_NAME_ID_NAME_ENABLE - 1, ES_NAME_ID_NAME);

    assert_EQ(ES_CS_ID_WIDTH_ENABLE    - 1, ES_CS_ID_WIDTH);
    assert_EQ(ES_CS_ID_COL_NAME_ENABLE - 1, ES_CS_ID_COL_NAME);

    assert_EQ(ES_RS_ID_HEIGHT_ENABLE       - 1, ES_RS_ID_HEIGHT);
    assert_EQ(ES_RS_ID_HEIGHT_FIXED_ENABLE - 1, ES_RS_ID_HEIGHT_FIXED);
    assert_EQ(ES_RS_ID_UNBREAKABLE_ENABLE  - 1, ES_RS_ID_UNBREAKABLE);
    assert_EQ(ES_RS_ID_ROW_NAME_ENABLE     - 1, ES_RS_ID_ROW_NAME);

    assert_EQ(ES_PS_ID_NUMFORM_LIST_NU_ENABLE    - 1, ES_PS_ID_NUMFORM_LIST_NU);
    assert_EQ(ES_PS_ID_NUMFORM_LIST_DT_ENABLE    - 1, ES_PS_ID_NUMFORM_LIST_DT);
    assert_EQ(ES_PS_ID_NUMFORM_LIST_SE_ENABLE    - 1, ES_PS_ID_NUMFORM_LIST_SE);
    assert_EQ(ES_PS_ID_NEW_OBJECT_LIST_ENABLE    - 1, ES_PS_ID_NEW_OBJECT_LIST);

    assert_EQ(ES_PS_ID_MARGIN_PARA_ENABLE        - 1, ES_PS_ID_MARGIN_PARA);
    assert_EQ(ES_PS_ID_MARGIN_LEFT_ENABLE        - 1, ES_PS_ID_MARGIN_LEFT);
    assert_EQ(ES_PS_ID_MARGIN_RIGHT_ENABLE       - 1, ES_PS_ID_MARGIN_RIGHT);
    assert_EQ(ES_PS_ID_TAB_LIST_ENABLE           - 1, ES_PS_ID_TAB_LIST);
    assert_EQ(ES_PS_ID_PARA_START_ENABLE         - 1, ES_PS_ID_PARA_START);
    assert_EQ(ES_PS_ID_PARA_END_ENABLE           - 1, ES_PS_ID_PARA_END);
    assert_EQ(ES_PS_ID_LINE_SPACE_GROUP_ENABLE   - 1, ES_PS_ID_LINE_SPACE_GROUP);

    assert_EQ(ES_PS_ID_BORDER_RGB_GROUP_ENABLE   - 1, ES_PS_ID_BORDER_RGB_GROUP);
    assert_EQ(ES_PS_ID_BORDER_LINE_GROUP_ENABLE  - 1, ES_PS_ID_BORDER_LINE_GROUP);
    assert_EQ(ES_PS_ID_GRID_RGB_GROUP_ENABLE     - 1, ES_PS_ID_GRID_RGB_GROUP);
    assert_EQ(ES_PS_ID_GRID_LINE_GROUP_ENABLE    - 1, ES_PS_ID_GRID_LINE_GROUP);
    assert_EQ(ES_PS_ID_RGB_BACK_GROUP_ENABLE     - 1, ES_PS_ID_RGB_BACK_GROUP);
    assert_EQ(ES_PS_ID_HORZ_JUSTIFY_GROUP_ENABLE - 1, ES_PS_ID_HORZ_JUSTIFY_GROUP);
    assert_EQ(ES_PS_ID_VERT_JUSTIFY_GROUP_ENABLE - 1, ES_PS_ID_VERT_JUSTIFY_GROUP);

    assert_EQ(ES_FS_ID_TYPEFACE_ENABLE     - 1, ES_FS_ID_TYPEFACE);
    assert_EQ(ES_FS_ID_HEIGHT_ENABLE       - 1, ES_FS_ID_HEIGHT);
    assert_EQ(ES_FS_ID_WIDTH_ENABLE        - 1, ES_FS_ID_WIDTH);
    assert_EQ(ES_FS_ID_BOLD_ENABLE         - 1, ES_FS_ID_BOLD);
    assert_EQ(ES_FS_ID_ITALIC_ENABLE       - 1, ES_FS_ID_ITALIC);
    assert_EQ(ES_FS_ID_UNDERLINE_ENABLE    - 1, ES_FS_ID_UNDERLINE);
    assert_EQ(ES_FS_ID_SUPERSCRIPT_ENABLE  - 1, ES_FS_ID_SUPERSCRIPT);
    assert_EQ(ES_FS_ID_SUBSCRIPT_ENABLE    - 1, ES_FS_ID_SUBSCRIPT);
    assert_EQ(ES_FS_ID_COLOUR_GROUP_ENABLE - 1, ES_FS_ID_COLOUR_GROUP);

    msgclr(dialog_cmd_ctl_enable);
    dialog_cmd_ctl_enable.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
    dialog_cmd_ctl_enable.control_id = state_control_id;
    dialog_cmd_ctl_enable.enabled = enabled;
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_ENABLE, &dialog_cmd_ctl_enable));

    if(p_es_wibble->enable_units)
    {
        assert_EQ(ES_CS_ID_WIDTH_ENABLE             + 1, ES_CS_ID_WIDTH_UNITS);
        assert_EQ(ES_PS_ID_MARGIN_PARA_ENABLE       + 1, ES_PS_ID_MARGIN_PARA_UNITS);
        assert_EQ(ES_PS_ID_MARGIN_LEFT_ENABLE       + 1, ES_PS_ID_MARGIN_LEFT_UNITS);
        assert_EQ(ES_PS_ID_MARGIN_RIGHT_ENABLE      + 1, ES_PS_ID_MARGIN_RIGHT_UNITS);
        assert_EQ(ES_PS_ID_TAB_LIST_ENABLE          + 1, ES_PS_ID_TAB_LIST_UNITS);

        assert_EQ(ES_RS_ID_HEIGHT_ENABLE            + 1, ES_RS_ID_HEIGHT_UNITS);
        assert_EQ(ES_PS_ID_PARA_START_ENABLE        + 1, ES_PS_ID_PARA_START_UNITS);
        assert_EQ(ES_PS_ID_PARA_END_ENABLE          + 1, ES_PS_ID_PARA_END_UNITS);

        assert_EQ(ES_FS_ID_HEIGHT_ENABLE            + 1, ES_FS_ID_HEIGHT_UNITS);
        assert_EQ(ES_FS_ID_WIDTH_ENABLE             + 1, ES_FS_ID_WIDTH_UNITS);

        dialog_cmd_ctl_enable.control_id = (DIALOG_CTL_ID) (p_dialog_msg_ctl_state_change->dialog_control_id + 1);
        status_assert(call_dialog(DIALOG_CMD_CODE_CTL_ENABLE, &dialog_cmd_ctl_enable));
    }

    /* auto-turn on feature for 4-3 states */
    if(p_es_wibble->set_state_on_enable && enabled && !p_es_callback->creating)
    {
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
        msgclr(dialog_cmd_ctl_state_set);
        dialog_cmd_ctl_state_set.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
        dialog_cmd_ctl_state_set.control_id = state_control_id;
        dialog_cmd_ctl_state_set.bits = 0;
        dialog_cmd_ctl_state_set.state.checkbox = DIALOG_BUTTONSTATE_ON;
        status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
    }
    } /*block*/

    { /* repaint our light if needed */
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = p_dialog_msg_ctl_state_change->h_dialog;
    dialog_cmd_ctl_state_set.control_id = (DIALOG_CTL_ID) (ES_ID_LIGHT_STT + p_es_callback->subdialog_current);
    dialog_cmd_ctl_state_set.bits = 0;
    * (P_RGB) &dialog_cmd_ctl_state_set.state.user = es_light_on_for(p_es_callback, p_es_callback->subdialog_current) ? rgb_stash[10 /*green*/] : rgb_stash[1 /*lt grey*/];
    status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
    } /*block*/

    return(STATUS_OK);
}

extern void
es_tweak_style_init(
    P_ES_CALLBACK p_es_callback)
{
    qsort(es_wibble, elemof32(es_wibble), sizeof32(es_wibble[0]), es_wibble_control_id_qsort);

    /* corresponds closely to style granularity hierarchy ha ha */
    zero_array(es_subdialog_style_selector);

    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_NAME], STYLE_SW_NAME);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_NAME], STYLE_SW_KEY);

    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_RS],   STYLE_SW_RS_HEIGHT);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_RS],   STYLE_SW_RS_HEIGHT_FIXED);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_RS],   STYLE_SW_RS_UNBREAKABLE);
    if(p_es_callback->num)
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_RS],   STYLE_SW_RS_ROW_NAME);

    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS1],  STYLE_SW_PS_RGB_BACK);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS1],  STYLE_SW_PS_RGB_BORDER);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS1],  STYLE_SW_PS_BORDER);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS1],  STYLE_SW_PS_RGB_GRID_LEFT);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS1],  STYLE_SW_PS_RGB_GRID_TOP);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS1],  STYLE_SW_PS_RGB_GRID_RIGHT);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS1],  STYLE_SW_PS_RGB_GRID_BOTTOM);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS1],  STYLE_SW_PS_GRID_LEFT);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS1],  STYLE_SW_PS_GRID_TOP);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS1],  STYLE_SW_PS_GRID_RIGHT);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS1],  STYLE_SW_PS_GRID_BOTTOM);

    assert_EQ(STYLE_SW_PS_GRID_LEFT + 1, STYLE_SW_PS_GRID_TOP);
    assert_EQ(STYLE_SW_PS_GRID_LEFT + 2, STYLE_SW_PS_GRID_RIGHT);
    assert_EQ(STYLE_SW_PS_GRID_LEFT + 3, STYLE_SW_PS_GRID_BOTTOM);

    assert_EQ(STYLE_SW_PS_RGB_GRID_LEFT + 1, STYLE_SW_PS_RGB_GRID_TOP);
    assert_EQ(STYLE_SW_PS_RGB_GRID_LEFT + 2, STYLE_SW_PS_RGB_GRID_RIGHT);
    assert_EQ(STYLE_SW_PS_RGB_GRID_LEFT + 3, STYLE_SW_PS_RGB_GRID_BOTTOM);

    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS2],  STYLE_SW_PS_MARGIN_PARA);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS2],  STYLE_SW_PS_MARGIN_LEFT);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS2],  STYLE_SW_PS_MARGIN_RIGHT);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS2],  STYLE_SW_CS_WIDTH);
    if(p_es_callback->num)
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS2],  STYLE_SW_CS_COL_NAME);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS2],  STYLE_SW_PS_JUSTIFY);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS2],  STYLE_SW_PS_JUSTIFY_V);
    if(p_es_callback->atx)
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS2],  STYLE_SW_PS_TAB_LIST);

    if(p_es_callback->atx)
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS3],  STYLE_SW_PS_PARA_START);
    if(p_es_callback->atx)
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS3],  STYLE_SW_PS_PARA_END);
    if(p_es_callback->atx)
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS3],  STYLE_SW_PS_LINE_SPACE);

    if(p_es_callback->num)
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS4],  STYLE_SW_PS_NUMFORM_NU);
    if(p_es_callback->num)
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS4],  STYLE_SW_PS_NUMFORM_DT);
    if(p_es_callback->num)
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS4],  STYLE_SW_PS_NUMFORM_SE);
    if(p_es_callback->num)
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS4],  STYLE_SW_PS_NEW_OBJECT);
    if(p_es_callback->num)
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_PS4],  STYLE_SW_PS_PROTECT);

    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_FS],   STYLE_SW_FS_NAME);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_FS],   STYLE_SW_FS_SIZE_Y);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_FS],   STYLE_SW_FS_SIZE_X);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_FS],   STYLE_SW_FS_BOLD);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_FS],   STYLE_SW_FS_ITALIC);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_FS],   STYLE_SW_FS_UNDERLINE);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_FS],   STYLE_SW_FS_SUPERSCRIPT);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_FS],   STYLE_SW_FS_SUBSCRIPT);
    style_selector_bit_set(&es_subdialog_style_selector[ES_SUBDIALOG_FS],   STYLE_SW_FS_COLOUR);
}

/******************************************************************************
*
* quick pre-create pass over controls
*
******************************************************************************/

extern void
es_tweak_style_precreate(
    P_ES_CALLBACK p_es_callback,
    P_DIALOG_CMD_PROCESS_DBOX p_dialog_cmd_process_dbox)
{
    U32 i;

    assert(NULL != p_dialog_cmd_process_dbox->p_ctl_create);

    for(i = 0; i < p_dialog_cmd_process_dbox->n_ctls; ++i)
    {
        const P_DIALOG_CTL_CREATE p_dialog_ctl_create = &p_dialog_cmd_process_dbox->p_ctl_create[i];
        const PC_DIALOG_CONTROL p_dialog_control = p_dialog_ctl_create->p_dialog_control.p_dialog_control;
        const DIALOG_CTL_ID control_id = p_dialog_control->control_id;
        const P_ES_WIBBLE p_es_wibble = es_wibble_search(control_id);

        if(P_ES_WIBBLE_NONE != p_es_wibble)
        {
            switch(p_es_wibble->data_type)
            {
            case ES_WIBBLE_TYPE_H_UNITS:
            case ES_WIBBLE_TYPE_V_UNITS:
            case ES_WIBBLE_TYPE_V_UNITS_FINE:
                {
                const P_ES_USER_UNIT_INFO p_es_user_unit_info =
                    (p_es_wibble->data_type == ES_WIBBLE_TYPE_H_UNITS)
                        ? &p_es_callback->info[IDX_HORZ]
                        : &p_es_callback->info[IDX_VERT];

                const PC_DIALOG_CONTROL_DATA_BUMP_F64 p_dialog_control_data_bump_f64 = (PC_DIALOG_CONTROL_DATA_BUMP_F64) p_dialog_ctl_create->p_dialog_control_data;
                UI_CONTROL_F64 * const p_ui_control_f64 = (UI_CONTROL_F64 *) p_dialog_control_data_bump_f64->bump_xx.p_uic;
                p_ui_control_f64->ustr_numform  = ustr_bptr(p_es_user_unit_info->normal.user_unit_numform_ustr_buf);
                p_ui_control_f64->inc_dec_round = p_es_user_unit_info->normal.user_unit_multiple;
                if(p_es_wibble->data_type == ES_WIBBLE_TYPE_V_UNITS_FINE)
                {
                    p_ui_control_f64->ustr_numform  = ustr_bptr(p_es_user_unit_info->fine.user_unit_numform_ustr_buf);
                    p_ui_control_f64->inc_dec_round = p_es_user_unit_info->fine.user_unit_multiple;
                }
                p_ui_control_f64->bump_val = 1.0 / p_ui_control_f64->inc_dec_round;
                p_ui_control_f64->min_val = 0.0;
                p_ui_control_f64->max_val =
                    /*FP_USER_UNIT*/ ((+100.0 * PIXITS_PER_INCH) / p_es_user_unit_info->fp_pixits_per_user_unit);
                if(control_id == ES_PS_ID_MARGIN_PARA)
                    p_ui_control_f64->min_val = -p_ui_control_f64->max_val;

                break;
                }

            default:
                break;
            }
        }
        else
        {
            switch(control_id)
            {
            case ES_RS_ID_HEIGHT_UNITS:
            case ES_PS_ID_PARA_START_UNITS:
            case ES_PS_ID_PARA_END_UNITS:
                {
                const PC_DIALOG_CONTROL_DATA_STATICTEXT p_dialog_control_data_statictext = (PC_DIALOG_CONTROL_DATA_STATICTEXT) p_dialog_ctl_create->p_dialog_control_data;
                const P_UI_TEXT p_ui_text = (P_UI_TEXT) &p_dialog_control_data_statictext->caption;
                p_ui_text->type = UI_TEXT_TYPE_RESID;
                p_ui_text->text.resource_id = p_es_callback->info[IDX_VERT].user_unit_resource_id;
                break;
                }

            case ES_CS_ID_WIDTH_UNITS:
            case ES_PS_ID_MARGIN_PARA_UNITS:
            case ES_PS_ID_MARGIN_LEFT_UNITS:
            case ES_PS_ID_MARGIN_RIGHT_UNITS:
            case ES_PS_ID_TAB_LIST_UNITS:
                {
                const PC_DIALOG_CONTROL_DATA_STATICTEXT p_dialog_control_data_statictext = (PC_DIALOG_CONTROL_DATA_STATICTEXT) p_dialog_ctl_create->p_dialog_control_data;
                const P_UI_TEXT p_ui_text = (P_UI_TEXT) &p_dialog_control_data_statictext->caption;
                p_ui_text->type = UI_TEXT_TYPE_RESID;
                p_ui_text->text.resource_id = p_es_callback->info[IDX_HORZ].user_unit_resource_id;
                break;
                }

            default:
                break;
            }
        }
    }
}

/*
build a list of new objects to choose from in style editor
*/

_Check_return_
static STATUS
ui_list_create_new_object(
    /*out*/ P_ARRAY_HANDLE p_array_handle,
    /*inout*/ P_UI_SOURCE p_ui_source)
{
    P_UI_LIST_ENTRY_NEW_OBJECT p_ui_list_entry_new_object;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(*p_ui_list_entry_new_object), TRUE);
    STATUS status = STATUS_OK;
    OBJECT_ID object_id;

    *p_array_handle = 0;

    /* use new_object from all loaded new_objects */
    object_id = OBJECT_ID_ENUM_START;

    while(status_ok(object_next(&object_id)))
    {
        UCHARZ object_name_ustr_buf[32];
        STATUS resource_id = 0;

        switch(object_id)
        {
        case OBJECT_ID_SS:   resource_id = MSG_OBJECT_TYPE_SS;   break;
        case OBJECT_ID_TEXT: resource_id = MSG_OBJECT_TYPE_TEXT; break;
        case OBJECT_ID_REC:  resource_id = MSG_OBJECT_TYPE_REC;  break;

        default:
            continue;
        }

        if(resource_id)
            resource_lookup_ustr_buffer(ustr_bptr(object_name_ustr_buf), elemof32(object_name_ustr_buf), resource_id);

        if(NULL != (p_ui_list_entry_new_object = al_array_extend_by(p_array_handle, UI_LIST_ENTRY_NEW_OBJECT, 1, &array_init_block, &status)))
        {
            quick_ublock_with_buffer_setup(p_ui_list_entry_new_object->quick_ublock);
            status = quick_ublock_ustr_add_n(&p_ui_list_entry_new_object->quick_ublock, ustr_bptr(object_name_ustr_buf), strlen_with_NULLCH);
            p_ui_list_entry_new_object->object_id = object_id;
        }

        status_break(status);
    }

    /* make a source of text pointers to these elements for list box processing */
    if(status_ok(status))
        status = ui_source_create_ub(p_array_handle, p_ui_source, UI_TEXT_TYPE_USTR_PERM, offsetof32(UI_LIST_ENTRY_NEW_OBJECT, quick_ublock));

    if(status_fail(status))
        ui_lists_dispose_ub(p_array_handle, p_ui_source, offsetof32(UI_LIST_ENTRY_NEW_OBJECT, quick_ublock));

    return(status);
}

/*
build one of a list of numforms to choose from in style editor
*/

_Check_return_
static STATUS
numform_list_add(
    /*inout*/ P_ARRAY_HANDLE p_array_handle,
    _In_z_      PC_USTR ustr_numform)
{
    P_UI_LIST_ENTRY_NUMFORM p_ui_list_entry_numform;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*p_ui_list_entry_numform), TRUE);
    STATUS status = STATUS_OK;
    ARRAY_INDEX i;

    ui_source_list_fixup_ub(p_array_handle, offsetof32(UI_LIST_ENTRY_NUMFORM, quick_ublock));

    for(i = 0; i < array_elements(p_array_handle); ++i)
    {
        p_ui_list_entry_numform = array_ptr(p_array_handle, UI_LIST_ENTRY_NUMFORM, i);

        if(0 == /*"C"*/strcmp((const char *) quick_ublock_ustr(&p_ui_list_entry_numform->quick_ublock), (const char *) ustr_numform))
            return(STATUS_OK);
    }

    if(NULL != (p_ui_list_entry_numform = al_array_extend_by(p_array_handle, UI_LIST_ENTRY_NUMFORM, 1, &array_init_block, &status)))
    {
        quick_ublock_with_buffer_setup(p_ui_list_entry_numform->quick_ublock);

        status = quick_ublock_ustr_add_n(&p_ui_list_entry_numform->quick_ublock, ustr_numform, strlen_with_NULLCH);
    }

    return(status);
}

_Check_return_
static STATUS
ui_list_create_numform(
    _DocuRef_   P_DOCU p_docu,
    /*out*/ P_ARRAY_HANDLE p_array_handle,
    /*out*/ P_UI_SOURCE p_ui_source,
    _InVal_     BIT_NUMBER bit_number)
{
    STATUS status = STATUS_OK;

    *p_array_handle = 0;

    { /* use numform from all loaded numforms */
    PC_ARRAY_HANDLE p_ui_numform_handle = &p_docu_from_config()->numforms;
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(p_ui_numform_handle); ++i)
    {
        PC_UI_NUMFORM p_ui_numform = array_ptrc(p_ui_numform_handle, UI_NUMFORM, i);
        PC_USTR ustr_numform = p_ui_numform->ustr_numform;

        if(p_ui_numform->numform_class != UI_NUMFORM_CLASS_NUMFORM_NU + (bit_number - STYLE_SW_PS_NUMFORM_NU))
            continue;

        status_break(status = numform_list_add(p_array_handle, ustr_numform));
    }
    } /*block*/

    /* use numform from all loaded styles */
    if(status_ok(status))
    {
        STYLE_HANDLE style_handle = STYLE_HANDLE_ENUM_START;
        P_STYLE p_style;

        while(style_enum_styles(p_docu, &p_style, &style_handle) >= 0)
        {
            if(style_bit_test(p_style, bit_number))
            {
                PC_USTR ustr_numform;

                switch(bit_number)
                {
                default: default_unhandled();
#if CHECKING
                case STYLE_SW_PS_NUMFORM_NU:
#endif
                    ustr_numform = array_ustr(&p_style->para_style.h_numform_nu);
                    break;

                case STYLE_SW_PS_NUMFORM_DT:
                    ustr_numform = array_ustr(&p_style->para_style.h_numform_dt);
                    break;

                case STYLE_SW_PS_NUMFORM_SE:
                    ustr_numform = array_ustr(&p_style->para_style.h_numform_se);
                    break;
                }

                status_break(status = numform_list_add(p_array_handle, ustr_numform));
            }
        }
    }

    /* we can have endless hours of debate about how many of these we want to put on show ... do we do regions too? */

    /* make a source of text pointers to these elements for list box processing */
    if(status_ok(status))
        status = ui_source_create_ub(p_array_handle, p_ui_source, UI_TEXT_TYPE_USTR_PERM, offsetof32(UI_LIST_ENTRY_NUMFORM, quick_ublock));

    if(status_fail(status))
        ui_lists_dispose_ub(p_array_handle, p_ui_source, offsetof32(UI_LIST_ENTRY_NUMFORM, quick_ublock));

    return(status);
}

/******************************************************************************
*
* style key list
*
******************************************************************************/

_Check_return_
static STATUS
ui_list_create_style_key(
    _DocuRef_   P_DOCU p_docu,
    /*out*/ P_ARRAY_HANDLE p_array_handle,
    /*out*/ P_UI_SOURCE p_ui_source,
    _InVal_     STYLE_HANDLE style_handle_being_modified)
{
    P_UI_LIST_ENTRY_STYLE_KEY p_ui_list_entry_style_key;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*p_ui_list_entry_style_key), TRUE);
    STATUS status = STATUS_OK;

    IGNOREPARM_DocuRef_(p_docu);

    *p_array_handle = 0;

    { /* present a wee selection of keys that aren't already in use (SKS 07sep95) */
    static const KMAP_CODE base_code[] = { KMAP_BASE_CFUNC, KMAP_BASE_SFUNC, KMAP_BASE_CSFUNC, KMAP_BASE_FUNC };
    U32 i;

    for(i = 0; i < elemof32(base_code); ++i)
    {
        KMAP_CODE kmap_code;

        for(kmap_code = base_code[i] + 0x01; kmap_code <= base_code[i] + 0x0B; ++kmap_code)
        {
            T5_MESSAGE t5_message;
            ARRAY_HANDLE h_commands = command_array_handle_from_key_code(p_docu, kmap_code, &t5_message);

            if(t5_message == T5_CMD_STYLE_APPLY_STYLE_HANDLE)
            { /* obviously we have to list our current setting! */
                if((STYLE_HANDLE) h_commands != style_handle_being_modified)
                    continue;
            }
            else if(0 != h_commands)
                continue;

            if(NULL != (p_ui_list_entry_style_key = al_array_extend_by(p_array_handle, UI_LIST_ENTRY_STYLE_KEY, 1, &array_init_block, &status)))
            {
                PCTSTR tstr = key_ui_name_from_key_code(kmap_code, 1 /*long*/);

                quick_tblock_with_buffer_setup(p_ui_list_entry_style_key->quick_tblock);

                status = quick_tblock_tchars_add(&p_ui_list_entry_style_key->quick_tblock, tstr, tstrlen32p1(tstr));

                p_ui_list_entry_style_key->kmap_code = kmap_code;
            }

            status_break(status);
        }

        status_break(status);
    }
    } /*block*/

    /* make a source of text pointers to these elements for list box processing */
    if(status_ok(status))
        status = ui_source_create_tb(p_array_handle, p_ui_source, UI_TEXT_TYPE_TSTR_PERM, offsetof32(UI_LIST_ENTRY_STYLE_KEY, quick_tblock));

    if(status_fail(status))
        ui_lists_dispose_tb(p_array_handle, p_ui_source, offsetof32(UI_LIST_ENTRY_STYLE_KEY, quick_tblock));

    return(status);
}

/* end of ui_styl3.c */
