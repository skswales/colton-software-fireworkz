/* ui_save.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* UI for saving for Fireworkz */

/* SKS August 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/ff_io.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#if RISCOS
#define EXPOSE_RISCOS_SWIS 1

#include "ob_dlg/xp_dlgr.h"
#endif

#if WINDOWS
#include "commdlg.h"

#include "cderr.h"

#include "direct.h"
#endif

#include "ob_file/xp_file.h"

/*
internal structures
*/

typedef struct SAVE_CALLBACK * P_SAVE_CALLBACK;

typedef STATUS (* P_PROC_SAVE) (
    _DocuRef_   P_DOCU p_docu,
    P_SAVE_CALLBACK p_save_callback,
    _In_z_      PCTSTR filename);

typedef struct SAVE_CALLBACK
{
#define SAVING_OWNFORM  0
#define SAVING_FOREIGN  1
#define SAVING_TEMPLATE 2
#define SAVING_PICTURE  3
#define LOCATING_TEMPLATE 4
    S32          saving;

    P_PROC_SAVE  p_proc_save;
    DOCNO        docno;
    U8           _spare_u8[3];
    H_DIALOG     h_dialog; /* only valid after USER_REDRAW, USER_MOUSE, PUSHBUTTON events */
    UI_TEXT      filename;
    UI_TEXT      init_filename;
    UI_TEXT      init_directory;
    T5_FILETYPE  t5_filetype;
    S32          self_abuse;
    STATUS       filename_edited;
    BOOL         allow_not_rooted;
    BOOL         rename_after_save;
    BOOL         clear_modify_after_save;
    BOOL         save_selection;
    BOOL         test_for_saved_file_is_safe;

#define SAVING_TEMPLATE_ALL        0
#define SAVING_TEMPLATE_ALL_STYLES 1
#define SAVING_TEMPLATE_ONE_STYLE  2
    S32          style_radio;

    STYLE_HANDLE style_handle;
    ARRAY_HANDLE other_filemap;
    OBJECT_ID    picture_object_id;
    P_ANY        picture_object_data_ref;

    UI_TEXT ui_text;

    PCTSTR initial_template_dir;
}
SAVE_CALLBACK;

/*
internal routines
*/

static void
save_common_details_query(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _OutRef_    P_T5_FILETYPE p_filetype,
    /*out*/ P_UI_TEXT p_ui_text_filename);

_Check_return_
static STATUS
save_common_dialog_check(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback);

_Check_return_
static T5_FILETYPE
save_foreign_lookup_filetype_from_description(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InRef_     PC_UI_TEXT p_description);

_Check_return_
static T5_FILETYPE
which_t5_filetype(
    _DocuRef_   P_DOCU p_docu);

#if WINDOWS

_Check_return_
static STATUS
windows_save_as(
    _DocuRef_   P_DOCU p_docu,
    P_SAVE_CALLBACK p_save_callback);

#endif /* OS */

ARRAY_HANDLE
installed_save_objects_handle;

#if RISCOS
static
BITMAP(save_name_validation, 256);
#endif

static UI_SOURCE
save_foreign_type_list_source;

static UI_SOURCE
save_template_style_list_source;

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ui_save);

_Check_return_
static STATUS
ui_save_msg_startup(void)
{
#if RISCOS
    bitmap_set(save_name_validation, N_BITS_ARG(256)); /* allow all characters bar those below in filenames */

    {
    BIT_NUMBER i;
    for(i = 0; i <= 31; ++i)
        bitmap_bit_clear(save_name_validation, i, N_BITS_ARG(256));
    } /*block*/

    bitmap_bit_clear(save_name_validation, CH_SPACE, N_BITS_ARG(256));

    bitmap_bit_clear(save_name_validation, CH_QUOTATION_MARK, N_BITS_ARG(256));
    bitmap_bit_clear(save_name_validation, CH_VERTICAL_LINE, N_BITS_ARG(256));
    bitmap_bit_clear(save_name_validation, CH_DELETE, N_BITS_ARG(256));
#endif

    return(STATUS_OK);
}

_Check_return_
static STATUS
ui_save_msg_exit2(void)
{
#if WINDOWS
    {
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&installed_save_objects_handle); ++i)
    {
        P_INSTALLED_SAVE_OBJECT p_installed_save_object = array_ptr(&installed_save_objects_handle, INSTALLED_SAVE_OBJECT, i);

        al_array_dispose(&p_installed_save_object->h_tstr_ClipboardFormat);

        /* no mechanism to unregister Windows clipboard formats */
    }
    } /*block*/
#endif

    al_array_dispose(&installed_save_objects_handle);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_ui_save_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(ui_save_msg_startup());

    case T5_MSG_IC__EXIT2:
        return(ui_save_msg_exit2());

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ui_save)
{
    IGNOREPARM_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_ui_save_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(extern, t5_cmd_object_bind_saver)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 3);
    INSTALLED_SAVE_OBJECT installed_save_object;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(installed_save_object), FALSE);

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    zero_struct(installed_save_object);
    installed_save_object.object_id   = p_args[0].val.object_id;
    installed_save_object.t5_filetype = p_args[1].val.t5_filetype;
#if WINDOWS
    if(arg_is_present(p_args, 2) && (NULL != p_args[2].val.tstr))
    {
        status_return(al_tstr_set(&installed_save_object.h_tstr_ClipboardFormat, p_args[2].val.tstr));
        void_WrapOsBoolChecking(0 != (installed_save_object.uClipboardFormat =
            RegisterClipboardFormat(array_tstr(&installed_save_object.h_tstr_ClipboardFormat))));
    }
#endif

    return(al_array_add(&installed_save_objects_handle, INSTALLED_SAVE_OBJECT, 1, &array_init_block, &installed_save_object));
}

/******************************************************************************
*
* save document
*
******************************************************************************/

enum SAVE_CONTROL_IDS
{
    SAVE_ID_STT = 32,
    SAVE_ID_NAME,
    SAVE_ID_PICT,
    SAVE_ID_SELECTION,
    SAVE_ID_TYPE,
    SAVE_ID_WAFFLE_1,
    SAVE_ID_WAFFLE_2,
    SAVE_ID_WAFFLE_3,
    SAVE_ID_WAFFLE_4,

    SAVE_ID_STYLE_GROUP = 50,
    SAVE_ID_STYLE_0,
    SAVE_ID_STYLE_1,
    SAVE_ID_STYLE_2,
    SAVE_ID_STYLE_LIST
};

#define SAVE_OWNFORM_TOTAL_H        ((PIXITS_PER_INCH * 17) / 10)
#define SAVE_OWNFORM_PICT_OFFSET_H  ((SAVE_OWNFORM_TOTAL_H - 68 * PIXITS_PER_RISCOS) / 2)

#define SAVE_FOREIGN_LIST_H         ((PIXITS_PER_INCH * 17) / 10)
#define SAVE_FOREIGN_PICT_OFFSET_H  ((SAVE_FOREIGN_LIST_H  - 68 * PIXITS_PER_RISCOS) / 2)

#define SAVE_TEMPLATE_LIST_H        ((PIXITS_PER_INCH * 17) / 10)
#define SAVE_TEMPLATE_PICT_OFFSET_H ((SAVE_TEMPLATE_LIST_H - 68 * PIXITS_PER_RISCOS) / 2)

#if RISCOS

static DIALOG_CONTROL
save_pict =
{
    SAVE_ID_PICT, DIALOG_MAIN_GROUP,
    { SAVE_ID_NAME, DIALOG_CONTROL_PARENT },
    { -SAVE_OWNFORM_PICT_OFFSET_H, 0, 68 * PIXITS_PER_RISCOS, 68 * PIXITS_PER_RISCOS /* standard RISC OS file icon size */ },
    { DRT(LTLT, USER) }
};

static const RGB
save_pict_rgb = { 0, 0, 0, 1 /*transparent*/ };

static const DIALOG_CONTROL_DATA_USER
save_pict_data = { 0, { FRAMED_BOX_NONE /* border_style */ }, (P_RGB) &save_pict_rgb };

#endif /* OS */

/*
* name of file
*/

static DIALOG_CONTROL
save_name_control =
{
    SAVE_ID_NAME, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, SAVE_ID_PICT },
    { 0, DIALOG_STDSPACING_V, SAVE_OWNFORM_TOTAL_H, DIALOG_STDEDIT_V },
    { DRT(LBLT, EDIT), 1 }
};

static DIALOG_CONTROL
save_name_template =
{
    SAVE_ID_NAME, DIALOG_MAIN_GROUP,

#if RISCOS
    { DIALOG_CONTROL_PARENT, SAVE_ID_PICT },
    { 0, DIALOG_STDSPACING_V, SAVE_TEMPLATE_LIST_H, DIALOG_STDEDIT_V },
    { DRT(LBLT, EDIT), 1 }
#else
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, SAVE_TEMPLATE_LIST_H, DIALOG_STDEDIT_V },
    { DRT(LTLT, EDIT), 1 }
#endif
};

#if RISCOS
static DIALOG_CONTROL_DATA_EDIT
save_name_data = { { { FRAMED_BOX_EDIT }, save_name_validation }, /* EDIT_XX */ { UI_TEXT_TYPE_NONE } /* state */ };
#else
static DIALOG_CONTROL_DATA_EDIT
save_name_data = { { { FRAMED_BOX_EDIT }, NULL }, /* EDIT_XX */ { UI_TEXT_TYPE_NONE } /* state */ };
#endif

/* OK - this is NOT defbutton_ok_data! */

static const DIALOG_CONTROL_DATA_PUSHBUTTON
save_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_OK) };

static DIALOG_CONTROL
save_selection =
{
    SAVE_ID_SELECTION, DIALOG_MAIN_GROUP,
    { SAVE_ID_NAME, SAVE_ID_NAME, SAVE_ID_NAME },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCHECK_V },
    { DRT(LBRT, CHECKBOX) }
};

static DIALOG_CONTROL_DATA_CHECKBOX
save_selection_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_SELECTION) };

/*
* list of types of file
*/

static DIALOG_CONTROL
save_foreign_type_list =
{
    SAVE_ID_TYPE, DIALOG_MAIN_GROUP,
    { SAVE_ID_SELECTION, SAVE_ID_SELECTION },
    { 0, DIALOG_STDSPACING_V, (PIXITS_PER_INCH * 22) / 10 },
    { DRT(LBLT, LIST_TEXT), 1 }
};

static S32 save_foreign_type_itemno;

static DIALOG_CONTROL
save_template_style_group =
{
    SAVE_ID_STYLE_GROUP, DIALOG_MAIN_GROUP,
    { SAVE_ID_NAME, SAVE_ID_NAME, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V },
    { DRT(LBRB, GROUPBOX) }
};

static DIALOG_CONTROL
save_template_style_0 =
{
    SAVE_ID_STYLE_0, SAVE_ID_STYLE_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON) }
};

static DIALOG_CONTROL_DATA_RADIOBUTTON
save_template_style_0_data = { { 0 }, SAVING_TEMPLATE_ALL, UI_TEXT_INIT_RESID(MSG_DIALOG_SAVE_TEMPLATE_ALL) };

static DIALOG_CONTROL
save_template_style_1 =
{
    SAVE_ID_STYLE_1, SAVE_ID_STYLE_GROUP,
    { SAVE_ID_STYLE_0, SAVE_ID_STYLE_0, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static DIALOG_CONTROL_DATA_RADIOBUTTON
save_template_style_1_data = { { 0 }, SAVING_TEMPLATE_ALL_STYLES, UI_TEXT_INIT_RESID(MSG_DIALOG_SAVE_TEMPLATE_ALL_STYLES) };

static DIALOG_CONTROL
save_template_style_2 =
{
    SAVE_ID_STYLE_2, SAVE_ID_STYLE_GROUP,
    { SAVE_ID_STYLE_1, SAVE_ID_STYLE_1, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static DIALOG_CONTROL_DATA_RADIOBUTTON
save_template_style_2_data = { { 0 }, SAVING_TEMPLATE_ONE_STYLE, UI_TEXT_INIT_RESID(MSG_DIALOG_SAVE_TEMPLATE_ONE_STYLE) };

/*
* list of styles
*/

static DIALOG_CONTROL
save_template_style_list =
{
    SAVE_ID_STYLE_LIST, SAVE_ID_STYLE_GROUP,
    { SAVE_ID_STYLE_2, SAVE_ID_STYLE_2 },
    { 0, DIALOG_STDSPACING_V },
    { DRT(LBLT, LIST_TEXT), 1 }
};

static BOOL
proc_save_common(
    _In_z_      PCTSTR filename /*low lifetime*/,
    CLIENT_HANDLE client_handle)
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) client_handle;
    const P_DOCU p_docu = p_docu_from_docno(p_save_callback->docno);
    TCHARZ save_name[BUF_MAX_PATHSTRING];
    STATUS status;

    if(p_save_callback->allow_not_rooted && !file_is_rooted(filename))
    {
#if RISCOS
        tstr_xstrkpy(save_name, elemof32(save_name), TEXT("<Choices$Write>") FILE_DIR_SEP_TSTR TEXT("Fireworkz"));

        if(status_fail(status = file_create_directory(save_name)))
        {
            reperr(status, save_name);
            return(TRUE);
        }

        tstr_xstrkat(save_name, elemof32(save_name), FILE_DIR_SEP_TSTR);
#elif WINDOWS
        /* prefix using first element from path and appropriate subdir */
        file_get_prefix(save_name, elemof32(save_name), NULL);
#endif

        tstr_xstrkat(save_name, elemof32(save_name), TEMPLATES_SUBDIR);

        if(status_fail(status = file_create_directory(save_name)))
        {
            reperr(status, save_name);
            return(TRUE);
        }

        tstr_xstrkat(save_name, elemof32(save_name), FILE_DIR_SEP_TSTR);
        tstr_xstrkat(save_name, elemof32(save_name), filename);
    }
    else
        tstr_xstrkpy(save_name, elemof32(save_name), filename);

    if(status_fail(status = (* p_save_callback->p_proc_save) (p_docu, p_save_callback, save_name)))
        reperr(status, save_name);
    else if(p_save_callback->h_dialog)
    {
        DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
        msgclr(dialog_cmd_complete_dbox);
        dialog_cmd_complete_dbox.h_dialog = p_save_callback->h_dialog;
        dialog_cmd_complete_dbox.completion_code = DIALOG_COMPLETION_OK;
        status_assert(call_dialog(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox));
    }

    return(TRUE);
}

_Check_return_
static STATUS
dialog_save_common_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case SAVE_ID_STYLE_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &save_template_style_list_source;
        break;

    case SAVE_ID_TYPE:
        p_dialog_msg_ctl_fill_source->p_ui_source = &save_foreign_type_list_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_save_common_ctl_create(
    _InRef_     PC_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE p_dialog_msg_ctl_create)
{
    switch(p_dialog_msg_ctl_create->dialog_control_id)
    {
    case SAVE_ID_SELECTION:
        {
        if(!p_docu->mark_info_cells.h_markers)
            ui_dlg_ctl_enable(p_dialog_msg_ctl_create->h_dialog, SAVE_ID_SELECTION, FALSE);

        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_save_common_ctl_create_state(
    _InRef_     P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) p_dialog_msg_ctl_create_state->client_handle;

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case SAVE_ID_NAME:
        {
        p_dialog_msg_ctl_create_state->state_set.state.edit.ui_text = p_save_callback->init_filename;
        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_DONT_MSG;
        break;
        }

    case SAVE_ID_STYLE_GROUP:
        /* setup style radiobutton state */
        p_dialog_msg_ctl_create_state->state_set.state.radiobutton = 0; /* SKS 27jan93 changed from 1 */
        break;

    case SAVE_ID_STYLE_LIST:
        assert(p_save_callback->saving == SAVING_TEMPLATE);
        style_name_from_marked_area(p_docu, &p_dialog_msg_ctl_create_state->state_set.state.list_text.ui_text);
        if(ui_text_is_blank(&p_dialog_msg_ctl_create_state->state_set.state.list_text.ui_text))
        {
            p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = 0 /*select one of 'em anyway*/ /*DIALOG_CTL_STATE_LIST_ITEM_NONE*/;
            p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALTERNATE;
        }
        break;

    case SAVE_ID_TYPE:
        p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = save_foreign_type_itemno;
        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALTERNATE;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_save_common_ctl_state_change(
    _InRef_     P_DOCU p_docu,
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case SAVE_ID_NAME:
        {
        ui_text_dispose(&p_save_callback->filename);

        status_assert(ui_text_copy(&p_save_callback->filename, &p_dialog_msg_ctl_state_change->new_state.edit.ui_text));

        if(!p_save_callback->self_abuse)
            p_save_callback->filename_edited++;

        break;
        }

    case SAVE_ID_TYPE:
        {
        DIALOG_CMD_CTL_ENCODE dialog_cmd_ctl_encode;
        msgclr(dialog_cmd_ctl_encode);
        dialog_cmd_ctl_encode.h_dialog   = p_dialog_msg_ctl_state_change->h_dialog;
        dialog_cmd_ctl_encode.control_id = SAVE_ID_PICT;
        dialog_cmd_ctl_encode.bits       = 0;
        status_assert(call_dialog(DIALOG_CMD_CODE_CTL_ENCODE, &dialog_cmd_ctl_encode));
        break;
        }

    case SAVE_ID_SELECTION:
        {
        p_save_callback->save_selection = (p_dialog_msg_ctl_state_change->new_state.checkbox == DIALOG_BUTTONSTATE_ON);

        if(p_save_callback->filename_edited <= 1)
        {
            static UI_TEXT selection_filename = UI_TEXT_INIT_RESID(MSG_SELECTION);

            DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
            msgclr(dialog_cmd_ctl_state_set);
            dialog_cmd_ctl_state_set.h_dialog   = p_dialog_msg_ctl_state_change->h_dialog;
            dialog_cmd_ctl_state_set.control_id = SAVE_ID_NAME;
            dialog_cmd_ctl_state_set.bits       = 0;
            dialog_cmd_ctl_state_set.state.edit.ui_text = p_save_callback->save_selection ? selection_filename : p_save_callback->init_filename;
            p_save_callback->self_abuse++;
            status_assert(call_dialog(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
            p_save_callback->self_abuse--;
        }

        break;
        }

    case SAVE_ID_STYLE_GROUP:
        p_save_callback->style_radio = p_dialog_msg_ctl_state_change->new_state.radiobutton;
        /* SKS after 1.03 08jun93 - only allow implicit save into template directory if it's a whole file save */
        p_save_callback->allow_not_rooted = (p_save_callback->style_radio == SAVING_TEMPLATE_ALL);
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, SAVE_ID_STYLE_LIST, (p_save_callback->style_radio == SAVING_TEMPLATE_ONE_STYLE));
        break;

    case SAVE_ID_STYLE_LIST:
        p_save_callback->style_handle = style_handle_from_name(p_docu, ui_text_tstr(&p_dialog_msg_ctl_state_change->new_state.list_text.ui_text));
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_save_common_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) p_dialog_msg_ctl_pushbutton->client_handle;

    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case IDOK:
        {
        PCTSTR filename;

        p_save_callback->h_dialog = p_dialog_msg_ctl_pushbutton->h_dialog;

        save_common_details_query(p_save_callback, &p_save_callback->t5_filetype, &p_save_callback->filename);

        filename = ui_text_tstr(&p_save_callback->filename);

        if(!p_save_callback->allow_not_rooted && !file_is_rooted(filename))
        {
            reperr_null(create_error(ERR_SAVE_DRAG_TO_DIRECTORY));

            return(STATUS_OK);
        }

        status_return(save_common_dialog_check(p_save_callback));

        proc_save_common(filename, (CLIENT_HANDLE) p_save_callback);
        return(STATUS_OK);
        }

    default:
        return(STATUS_OK);
    }
}

#if RISCOS

_Check_return_
static STATUS
dialog_save_common_ctl_user_redraw(
    _InRef_     PC_DIALOG_MSG_CTL_USER_REDRAW p_dialog_msg_ctl_user_redraw)
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) p_dialog_msg_ctl_user_redraw->client_handle;
    T5_FILETYPE t5_filetype;
    WimpIconBlockWithBitset icon;

    p_save_callback->h_dialog = p_dialog_msg_ctl_user_redraw->h_dialog;

    save_common_details_query(p_save_callback, &t5_filetype, NULL);

    zero_struct(icon);

    host_ploticon_setup_bbox(&icon, &p_dialog_msg_ctl_user_redraw->control_inner_box, &p_dialog_msg_ctl_user_redraw->redraw_context);

    /* fills in sprite_name with name of a sprite representing the filetype from the Window Manager Sprite Pool */
    dialog_riscos_file_icon_setup(&icon, t5_filetype);

    host_ploticon(&icon);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_save_common_ctl_user_mouse(
    _InRef_     PC_DIALOG_MSG_CTL_USER_MOUSE p_dialog_msg_ctl_user_mouse)
{
    switch(p_dialog_msg_ctl_user_mouse->click)
    {
    case DIALOG_MSG_USER_MOUSE_CLICK_LEFT_DRAG:
        {
        const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) p_dialog_msg_ctl_user_mouse->client_handle;
        WimpIconBlockWithBitset icon;

        p_save_callback->h_dialog = p_dialog_msg_ctl_user_mouse->h_dialog;

        save_common_details_query(p_save_callback, &p_save_callback->t5_filetype, &p_save_callback->filename);

        zero_struct(icon);

        icon.bbox = p_dialog_msg_ctl_user_mouse->riscos.icon.bbox;

        /* fills in icon with name of a sprite representing the filetype from the Window Manager Sprite Pool */
        dialog_riscos_file_icon_setup(&icon, p_save_callback->t5_filetype);

        status_return(save_common_dialog_check(p_save_callback));

        status_return(dialog_riscos_file_icon_drag(p_dialog_msg_ctl_user_mouse->h_dialog, &icon));

        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_save_common_riscos_drag_ended(
    _InRef_     P_DIALOG_MSG_RISCOS_DRAG_ENDED p_dialog_msg_riscos_drag_ended)
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) p_dialog_msg_riscos_drag_ended->client_handle;

    /* try saving file to target window */
    if(p_dialog_msg_riscos_drag_ended->mouse.window_handle != -1)
    {
        status_return(save_common_dialog_check(p_save_callback));

        consume_bool(
            host_xfer_save_file(
                ui_text_tstr(&p_save_callback->filename),
                p_save_callback->t5_filetype,
                42 /*estimated_size*/,
                proc_save_common,
                (CLIENT_HANDLE) p_save_callback,
                &p_dialog_msg_riscos_drag_ended->mouse));
    }

    return(STATUS_OK);
}

#endif /* RISCOS */

PROC_DIALOG_EVENT_PROTO(static, dialog_event_save_common)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_save_common_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE:
        return(dialog_save_common_ctl_create(p_docu, (P_DIALOG_MSG_CTL_CREATE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_save_common_ctl_create_state(p_docu, (P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_save_common_ctl_state_change(p_docu, (PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_save_common_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

#if RISCOS

    case DIALOG_MSG_CODE_CTL_USER_REDRAW:
        return(dialog_save_common_ctl_user_redraw((PC_DIALOG_MSG_CTL_USER_REDRAW) p_data));

    case DIALOG_MSG_CODE_CTL_USER_MOUSE:
        return(dialog_save_common_ctl_user_mouse((PC_DIALOG_MSG_CTL_USER_MOUSE) p_data));

    case DIALOG_MSG_CODE_RISCOS_DRAG_ENDED:
        return(dialog_save_common_riscos_drag_ended((P_DIALOG_MSG_RISCOS_DRAG_ENDED) p_data));

#endif /* RISCOS */

    default:
        return(STATUS_OK);
    }
}

static void
save_common_details_query(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _OutRef_   P_T5_FILETYPE p_filetype,
    /*out*/ P_UI_TEXT p_ui_text_filename)
{
    switch(p_save_callback->saving)
    {
    default: default_unhandled();
#if CHECKING
    case SAVING_OWNFORM:
#endif
        *p_filetype = which_t5_filetype(p_docu_from_docno(p_save_callback->docno));
        break;

    case SAVING_FOREIGN:
    case SAVING_PICTURE:
        if(array_elements(&p_save_callback->other_filemap) >= 2)
        {
            UI_TEXT ui_text;

            ui_dlg_get_list_text(p_save_callback->h_dialog, SAVE_ID_TYPE, &ui_text);

            if((*p_filetype = save_foreign_lookup_filetype_from_description(p_save_callback, &ui_text)) == STATUS_FAIL)
                *p_filetype = FILETYPE_ASCII;

            ui_text_dispose(&ui_text);
        }
        else
            *p_filetype = array_ptr(&p_save_callback->other_filemap, SAVE_FILETYPE, 0)->t5_filetype;
        break;

    case SAVING_TEMPLATE:
        *p_filetype = FILETYPE_T5_TEMPLATE;
        break;

    case LOCATING_TEMPLATE:
        *p_filetype = FILETYPE_DIRECTORY;
        break;
    }

    if(NULL != p_ui_text_filename)
    {
        ui_text_dispose(p_ui_text_filename);

        ui_dlg_get_edit(p_save_callback->h_dialog, SAVE_ID_NAME, p_ui_text_filename);
    }
}

_Check_return_
static STATUS
save_common_dialog_check(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    /*if(p_save_callback->saving == SAVING_OWNFORM)
        if(!p_save_callback->save_selection)
        {
            const P_DOCU p_docu = p_docu_from_docno(p_save_callback->docno);
        }*/

    if(p_save_callback->saving == SAVING_TEMPLATE)
        if(p_save_callback->style_radio == SAVING_TEMPLATE_ONE_STYLE)
            if(status_fail(p_save_callback->style_handle))
            {
                host_bleep();
                return(STATUS_FAIL);
            }

    return(STATUS_OK);
}

_Check_return_
static STATUS
save_ownform_save(
    _DocuRef_   P_DOCU p_docu,
    P_SAVE_CALLBACK p_save_callback,
    _In_z_      PCTSTR filename)
{
    OF_OP_FORMAT of_op_format = OF_OP_FORMAT_INIT;
    STATUS status;
    DOCU_AREA save_docu_area;

    if(p_save_callback->save_selection)
        if(!p_docu->mark_info_cells.h_markers)
            p_save_callback->save_selection = 0;

    if(p_save_callback->save_selection)
    {
        docu_area_from_markers_first(p_docu, &save_docu_area);

        p_save_callback->rename_after_save = FALSE;
        p_save_callback->clear_modify_after_save = FALSE;
    }

    of_op_format.process_status.flags.foreground = 1;

    * (P_U32) &of_op_format.of_template = 0xFFFFFFFFU;

    if(p_save_callback->save_selection)
        of_op_format.of_template.data_class = DATA_SAVE_DOC; /* SKS 05jan95 attempt to make saved selections directly reloadable */

    status_return(ownform_initialise_save(p_docu, &of_op_format, NULL, filename, p_save_callback->t5_filetype,
                                          p_save_callback->save_selection ? &save_docu_area : NULL));

    status = save_ownform_file(p_docu, &of_op_format);

    status = ownform_finalise_save(&p_docu, &of_op_format, status);

    status_return(status);

    if(!p_save_callback->test_for_saved_file_is_safe || host_xfer_saved_file_is_safe())
    {
        if(p_save_callback->clear_modify_after_save || p_save_callback->rename_after_save)
            docu_modify_clear(p_docu);

        if(p_save_callback->rename_after_save)
        {
            p_docu->file_date = of_op_format.output.u.file.ev_date;

            /* rename this document to be filename */
            status_assert(maeve_event(p_docu, T5_MSG_DOCU_RENAME, (P_ANY) de_const_cast(PTSTR, filename)));
        }
    }

    return(status);
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_ownform)
{
    STATUS status;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
    quick_tblock_with_buffer_setup(quick_tblock);

    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    if(status_ok(status = name_make_wholename(&p_docu->docu_name, &quick_tblock, TRUE)))
    {
        PCTSTR filename = quick_tblock_tstr(&quick_tblock);

        if(file_is_rooted(filename))
        {
            SAVE_CALLBACK save_callback;

            zero_struct(save_callback);

            save_callback.docno = docno_from_p_docu(p_docu);

            save_callback.saving = SAVING_OWNFORM;
            save_callback.p_proc_save = save_ownform_save;

            save_callback.t5_filetype = which_t5_filetype(p_docu);

            /*save_callback.rename_after_save = FALSE; implied */
            save_callback.clear_modify_after_save = TRUE;

            status = proc_save_common(filename, (CLIENT_HANDLE) &save_callback);

            quick_tblock_dispose(&quick_tblock);

            return(status);
        }
    }

    quick_tblock_dispose(&quick_tblock);

    status_return(status);

    return(execute_command(OBJECT_ID_SKEL, p_docu, T5_CMD_SAVE_OWNFORM_AS_INTRO, _P_DATA_NONE(P_ARGLIST_HANDLE)));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_ownform)

#if RISCOS

static DIALOG_CTL_CREATE
save_ctl_create[] =
{
    { &dialog_main_group }

,   { &save_pict,           &save_pict_data       }
,   { &save_name_control,   &save_name_data       }
,   { &save_selection,      &save_selection_data  }

,   { &stdbutton_cancel, &stdbutton_cancel_data }
,   { &defbutton_ok, &save_ok_data }
};

#endif

static void
t5_cmd_save_as_common_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback)
{
    zero_struct_ptr(p_save_callback);

    p_save_callback->docno = docno_from_p_docu(p_docu);

    p_save_callback->saving = SAVING_OWNFORM;
    p_save_callback->p_proc_save = save_ownform_save;

    p_save_callback->t5_filetype = which_t5_filetype(p_docu);

    p_save_callback->rename_after_save = TRUE;
    p_save_callback->clear_modify_after_save = TRUE;
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_ownform_as_intro)
{
    STATUS status;

    if(object_present(OBJECT_ID_NOTE))
    {
        NOTELAYER_SELECTION_INFO notelayer_selection_info;
        if(status_ok(object_call_id(OBJECT_ID_NOTE, p_docu, T5_MSG_NOTELAYER_SELECTION_INFO, &notelayer_selection_info)))
            return(t5_cmd_save_picture_intro(p_docu, t5_message, p_t5_cmd)); /* note(s) selected, process differently */
    }

    {
    SAVE_CALLBACK save_callback;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
    quick_tblock_with_buffer_setup(quick_tblock);

    t5_cmd_save_as_common_init(p_docu, &save_callback);

    status_assert(name_make_wholename(&p_docu->docu_name, &quick_tblock, TRUE));

    save_callback.init_filename.type = UI_TEXT_TYPE_TSTR_TEMP;
    save_callback.init_filename.text.tstr = quick_tblock_tstr(&quick_tblock);

    {
    TCHARZ directory[BUF_MAX_PATHSTRING];
    if(!file_dirname(directory, ui_text_tstr(&save_callback.init_filename)))
        save_callback.init_directory.type = UI_TEXT_TYPE_NONE;
    else
        status_assert(ui_text_alloc_from_tstr(&save_callback.init_directory, directory));
    } /*block*/

#if WINDOWS
    if(p_docu->mark_info_cells.h_markers) /* always do save selection */
    {
        save_callback.save_selection = TRUE;

        save_callback.init_filename.type = UI_TEXT_TYPE_RESID;
        save_callback.init_filename.text.resource_id = MSG_SELECTION;
    }
#endif

#if RISCOS
    save_callback.test_for_saved_file_is_safe = TRUE;
    host_xfer_set_saved_file_is_safe(TRUE);

    save_pict.relative_offset[0] = -SAVE_OWNFORM_PICT_OFFSET_H;

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, save_ctl_create, elemof32(save_ctl_create), MSG_DIALOG_SAVE_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_SAVE_CAPTION;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_save_common;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &save_callback;
    status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/
#elif WINDOWS
    status = windows_save_as(p_docu, &save_callback);
#endif

    quick_tblock_dispose(&quick_tblock);

    ui_text_dispose(&save_callback.init_directory);
    ui_text_dispose(&save_callback.init_filename);
    ui_text_dispose(&save_callback.filename);
    } /*block*/

    return(status);
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_ownform_as_intro)

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_ownform_as)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    SAVE_CALLBACK save_callback;
    PCTSTR filename;

    IGNOREPARM_InVal_(t5_message);

    t5_cmd_save_as_common_init(p_docu, &save_callback);

    filename = p_args[0].val.tstr;
    assert(p_args[0].type == (ARG_TYPE_TSTR | ARG_MANDATORY));

    if(arg_is_present(p_args, 1))
        save_callback.save_selection = p_args[1].val.fBool;

    return(proc_save_common(filename, (CLIENT_HANDLE) &save_callback));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_ownform_as)

PROC_BSEARCH_PROTO(static, save_foreign_filemap_description_compare_bsearch, UI_TEXT, SAVE_FILETYPE)
{
    BSEARCH_KEY_VAR_DECL(PC_UI_TEXT, p1);
    BSEARCH_DATUM_VAR_DECL(PC_SAVE_FILETYPE, p2);

    return(ui_text_compare(p1, &p2->description, 1 /*fussy*/, 1 /*insensitive*/));
}

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, save_foreign_filemap_description_compare_qsort, SAVE_FILETYPE)
{
    QSORT_ARG1_VAR_DECL(PC_SAVE_FILETYPE, p1);
    QSORT_ARG2_VAR_DECL(PC_SAVE_FILETYPE, p2);

    PC_UI_TEXT text_1 = &p1->description;
    PC_UI_TEXT text_2 = &p2->description;

    return(ui_text_compare(text_1, text_2, 1 /*fussy*/, 1 /*insensitive*/));
}

/******************************************************************************
*
* lookup returned description on list to yield filetype to use
*
******************************************************************************/

_Check_return_
static T5_FILETYPE
save_foreign_lookup_filetype_from_description(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InRef_     PC_UI_TEXT p_description)
{
    P_SAVE_FILETYPE p_save_filetype = al_array_bsearch(p_description, &p_save_callback->other_filemap, SAVE_FILETYPE, save_foreign_filemap_description_compare_bsearch);
    return(!IS_P_DATA_NONE(p_save_filetype) ? p_save_filetype->t5_filetype : FILETYPE_UNDETERMINED);
}

_Check_return_
static OBJECT_ID
save_foreign_lookup_object_id_from_t5_filetype(
    P_SAVE_CALLBACK p_save_callback,
    _InVal_     T5_FILETYPE t5_filetype)
{
    const ARRAY_INDEX filemap_elements = array_elements(&p_save_callback->other_filemap);
    ARRAY_INDEX i;
    P_SAVE_FILETYPE p_save_filetype = array_range(&p_save_callback->other_filemap, SAVE_FILETYPE, 0, filemap_elements);

    for(i = 0; i < filemap_elements; ++i, ++p_save_filetype)
    {
        if(p_save_filetype->t5_filetype == t5_filetype)
            return(p_save_filetype->object_id);
    }

    return(OBJECT_ID_NONE);
}

/******************************************************************************
*
* create a list of filetypes and textual representations thereof from the loaded objects
*
******************************************************************************/

static T5_FILETYPE save_foreign_type_filetype = FILETYPE_ASCII;

static T5_FILETYPE save_picture_type_filetype = FILETYPE_DRAW;

_Check_return_
static STATUS
save_foreign_filemap_create(
    P_SAVE_CALLBACK p_save_callback,
    /*out*/ P_UI_TEXT p_ui_text_dst,
    P_UI_SOURCE p_ui_source_dst,
    _InVal_     BOOL graphic)
{
    P_T5_FILETYPE p_current_filetype = (p_save_callback->saving == SAVING_PICTURE) ? &save_picture_type_filetype : &save_foreign_type_filetype;
    STATUS status = STATUS_OK;

    { /* objects will each append to this handle */
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(SAVE_FILETYPE), 0);
    status_return(al_array_preallocate_zero(&p_save_callback->other_filemap, &array_init_block));
    } /*block*/

    if(graphic)
    {
#if RISCOS
        STATUS status;
        if(status_fail(status = object_call_id(p_save_callback->picture_object_id, P_DOCU_NONE, T5_MSG_SAVE_PICTURE_FILETYPES_REQUEST_RISCOS, &p_save_callback->other_filemap)))
        {
            al_array_dispose(&p_save_callback->other_filemap);
            return(status);
        }
#endif
    }
    else
    {
        const ARRAY_INDEX n_elements = array_elements(&installed_save_objects_handle);
        ARRAY_INDEX i;
        P_INSTALLED_SAVE_OBJECT p_installed_save_object = array_range(&installed_save_objects_handle, INSTALLED_SAVE_OBJECT, 0, n_elements);

        for(i = 0; i < n_elements; ++i, ++p_installed_save_object)
        {
            SAVE_FILETYPE save_filetype;
            save_filetype.object_id = p_installed_save_object->object_id;
            save_filetype.t5_filetype  = p_installed_save_object->t5_filetype;
            save_filetype.description.type = UI_TEXT_TYPE_TSTR_PERM;
            save_filetype.description.text.tstr = description_text_from_t5_filetype(p_installed_save_object->t5_filetype);
            if(status_fail(status = al_array_add(&p_save_callback->other_filemap, SAVE_FILETYPE, 1, PC_ARRAY_INIT_BLOCK_NONE, &save_filetype)))
            {
                al_array_dispose(&p_save_callback->other_filemap);
                return(status);
            }
        }
    }

    /* now sort the textual representations list into ASCII order for neat display */
    al_array_qsort(&p_save_callback->other_filemap, save_foreign_filemap_description_compare_qsort);

    save_foreign_type_itemno = DIALOG_CTL_STATE_LIST_ITEM_NONE;

    {
    BOOL had_default = FALSE;
    const ARRAY_INDEX filemap_elements = array_elements(&p_save_callback->other_filemap);
    ARRAY_INDEX i;
    PC_SAVE_FILETYPE p_save_filetype = array_range(&p_save_callback->other_filemap, SAVE_FILETYPE, 0, filemap_elements);

    for(i = 0; i < filemap_elements; ++i)
    {
        if(p_save_filetype->t5_filetype == *p_current_filetype)
        {
            had_default = TRUE;
            save_foreign_type_itemno = i;
            break;
        }
    }

    if(!had_default && filemap_elements)
    {
        /* be content with the first thing we find */
        p_save_filetype = array_basec(&p_save_callback->other_filemap, SAVE_FILETYPE);

        save_foreign_type_itemno = 0;

        *p_current_filetype = p_save_filetype->t5_filetype;
    }
    } /*block*/

    if(p_ui_text_dst && p_ui_source_dst)
    {
        P_UI_TEXT p_ui_text;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_ui_text), 0);
        const ARRAY_INDEX filemap_elements = array_elements(&p_save_callback->other_filemap);

        al_array_dispose(&p_ui_source_dst->source.array_handle);
        p_ui_text_dst->type = UI_TEXT_TYPE_NONE;

        /* lookup filetype in list to obtain current state for combo,
         * also make a source of text pointers to these elements
        */
        if(NULL == (p_ui_text = al_array_alloc_UI_TEXT(&p_ui_source_dst->source.array_handle, filemap_elements, &array_init_block, &status)))
        {
            al_array_dispose(&p_save_callback->other_filemap);
            return(status);
        }

        {
        ARRAY_INDEX i;
        P_SAVE_FILETYPE p_save_filetype = array_range(&p_save_callback->other_filemap, SAVE_FILETYPE, 0, filemap_elements);

        for(i = 0; i < filemap_elements; ++i, ++p_save_filetype, ++p_ui_text)
        {
            status_assert(ui_text_copy(p_ui_text, &p_save_filetype->description));

            if(p_save_filetype->t5_filetype == *p_current_filetype)
                status_assert(ui_text_copy(p_ui_text_dst, p_ui_text));
        }
        } /*block*/

        p_ui_source_dst->type = UI_SOURCE_TYPE_ARRAY;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* Initialises everything for a foreign data save, then calls the relevant object
*
******************************************************************************/

_Check_return_
static STATUS
save_foreign_save(
    _DocuRef_   P_DOCU p_docu,
    P_SAVE_CALLBACK p_save_callback,
    _In_z_      PCTSTR filename)
{
    FF_OP_FORMAT ff_op_format = { OP_OUTPUT_INVALID };
    STATUS status;
    DOCU_AREA save_docu_area;
    const T5_FILETYPE t5_filetype = p_save_callback->t5_filetype;
    const OBJECT_ID object_id = save_foreign_lookup_object_id_from_t5_filetype(p_save_callback, t5_filetype);

    status_return(object_load(object_id));

    if(p_save_callback->save_selection)
        if(!p_docu->mark_info_cells.h_markers)
            p_save_callback->save_selection = 0;

    if(p_save_callback->save_selection)
        docu_area_from_markers_first(p_docu, &save_docu_area);

    status_return(foreign_initialise_save(p_docu, &ff_op_format, NULL, filename, t5_filetype,
                                          p_save_callback->save_selection ? &save_docu_area : NULL));

    ff_op_format.of_op_format.of_template.data_class = DATA_SAVE_WHOLE_DOC;

    {
    MSG_SAVE_FOREIGN msg_save_foreign;
    msg_save_foreign.t5_filetype = t5_filetype;
    msg_save_foreign.p_ff_op_format = &ff_op_format;
    status = object_call_id_load(p_docu, T5_MSG_SAVE_FOREIGN, &msg_save_foreign, object_id);
    } /*block*/

    return(foreign_finalise_save(&p_docu, &ff_op_format, status));
}

#if RISCOS

static DIALOG_CTL_CREATE
save_foreign_ctl_create[] =
{
    { &dialog_main_group },

    { &save_pict,               &save_pict_data },
    { &save_name_control,       &save_name_data },
    { &save_selection,          &save_selection_data },
    { &save_foreign_type_list,  &stdlisttext_data_dd },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &defbutton_ok,  &save_ok_data }
};

#endif

_Check_return_
static STATUS
t5_cmd_save_foreign_common_init(
    _DocuRef_   P_DOCU p_docu,
    P_SAVE_CALLBACK p_save_callback,
    _InVal_     BOOL intro)
{
    P_UI_TEXT p_ui_text = NULL;
    P_UI_SOURCE p_ui_source = NULL;

    zero_struct_ptr(p_save_callback);

    p_save_callback->docno = docno_from_p_docu(p_docu);

    p_save_callback->saving = SAVING_FOREIGN;
    p_save_callback->p_proc_save = save_foreign_save;

    p_save_callback->t5_filetype = save_foreign_type_filetype;

#if RISCOS
    if(intro)
    {
        p_ui_text = &p_save_callback->ui_text;
        p_ui_source = &save_foreign_type_list_source;
    }
#else
    IGNOREPARM_InVal_(intro);
#endif

    return(save_foreign_filemap_create(p_save_callback, p_ui_text, p_ui_source, FALSE));
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_foreign_intro)
{
    SAVE_CALLBACK save_callback;
    STATUS status;
    BOOL name_set = FALSE;

    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    status_return(t5_cmd_save_foreign_common_init(p_docu, &save_callback, TRUE));

    save_callback.init_filename.type = UI_TEXT_TYPE_RESID;
    save_callback.init_filename.text.resource_id = MSG_DIALOG_SAVE_FOREIGN_NAME;

#if RISCOS
    { /* make appropriate size box */
    S32 show_elements = array_elements(&save_callback.other_filemap);
    DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
    dialog_cmd_ctl_size_estimate.p_dialog_control = &save_foreign_type_list;
    dialog_cmd_ctl_size_estimate.p_dialog_control_data = NULL;
    ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
    show_elements = MAX(show_elements, 4);
    show_elements = MIN(show_elements, 8);
    save_foreign_type_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y + DIALOG_STDLISTITEM_V * show_elements;
    } /*block*/
#endif

    {
    TCHARZ directory[BUF_MAX_PATHSTRING];
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
    quick_tblock_with_buffer_setup(quick_tblock);

    status_assert(name_make_wholename(&p_docu->docu_name, &quick_tblock, TRUE));
    if(!file_dirname(directory, name_set ? ui_text_tstr(&save_callback.init_filename) : quick_tblock_tstr(&quick_tblock)))
        save_callback.init_directory.type = UI_TEXT_TYPE_NONE;
    else
        status_assert(ui_text_alloc_from_tstr(&save_callback.init_directory, directory));

    quick_tblock_dispose(&quick_tblock);
    } /*block*/

#if RISCOS
    save_callback.test_for_saved_file_is_safe = TRUE;
    host_xfer_set_saved_file_is_safe(TRUE);

    /* create a list of filetypes and textual representations thereof from the loaded objects */
    save_pict.relative_offset[0] = -SAVE_FOREIGN_PICT_OFFSET_H;

    save_foreign_type_list.relative_control_id[0] = SAVE_ID_SELECTION;
    save_foreign_type_list.relative_control_id[1] = SAVE_ID_SELECTION;

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, save_foreign_ctl_create, elemof32(save_foreign_ctl_create), 0);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_SAVE_FOREIGN_CAPTION;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_save_common;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &save_callback;
    status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/
#else
    status = windows_save_as(p_docu, &save_callback);
#endif

    al_array_dispose(&save_foreign_type_list_source.source.array_handle);

    ui_text_dispose(&save_callback.ui_text);

    ui_text_dispose(&save_callback.init_directory);
    ui_text_dispose(&save_callback.init_filename);
    ui_text_dispose(&save_callback.filename);

    al_array_dispose(&save_callback.other_filemap);

    return(status);
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_foreign_intro)

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_foreign)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    SAVE_CALLBACK save_callback;
    STATUS status;
    PCTSTR filename;

    IGNOREPARM_InVal_(t5_message);

    status_return(t5_cmd_save_foreign_common_init(p_docu, &save_callback, FALSE));

    filename = p_args[0].val.tstr;
    assert(p_args[0].type == (ARG_TYPE_TSTR | ARG_MANDATORY));

    save_callback.t5_filetype = p_args[1].val.t5_filetype;
    assert(p_args[1].type == (ARG_TYPE_S32 | ARG_MANDATORY));

    status = proc_save_common(filename, (CLIENT_HANDLE) &save_callback);

    al_array_dispose(&save_callback.other_filemap);

    return(status);
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_foreign)

_Check_return_
static STATUS
save_picture_save(
    _DocuRef_   P_DOCU p_docu,
    P_SAVE_CALLBACK p_save_callback,
    _In_z_      PCTSTR filename)
{
    FF_OP_FORMAT ff_op_format = { OP_OUTPUT_INVALID };
    STATUS status;
    const T5_FILETYPE t5_filetype = p_save_callback->t5_filetype;

    status_return(foreign_initialise_save(p_docu, &ff_op_format, NULL, filename, t5_filetype, NULL));

    {
    MSG_SAVE_PICTURE msg_save_picture;
    msg_save_picture.t5_filetype = t5_filetype;
    msg_save_picture.p_ff_op_format = &ff_op_format;
    msg_save_picture.extra = (S32) (intptr_t) p_save_callback->picture_object_data_ref;
    status = object_call_id(p_save_callback->picture_object_id, p_docu, T5_MSG_SAVE_PICTURE, &msg_save_picture);
    } /*block*/

    status = foreign_finalise_save(&p_docu, &ff_op_format, status);

    return(status);
}

#if RISCOS

static DIALOG_CTL_CREATE
save_picture_ctl_create[] =
{
    { &dialog_main_group },

    { &save_pict,               &save_pict_data }, /*[1]*/
    { &save_name_control,       &save_name_data }, /*[2]*/
    { &save_foreign_type_list,  &stdlisttext_data_dd }, /*[3]*/

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &defbutton_ok, &save_ok_data }
};

#endif

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_picture_intro)
{
    SAVE_CALLBACK save_callback;
    STATUS status;
    BOOL name_set;

    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    zero_struct(save_callback);

    save_callback.docno = docno_from_p_docu(p_docu);

    save_callback.saving = SAVING_PICTURE;
    save_callback.p_proc_save = save_picture_save;

    save_callback.t5_filetype = save_picture_type_filetype;

    /* identify note's object and ref, store in callback */
    if(!object_present(OBJECT_ID_NOTE))
        return(STATUS_OK);      /* no note(s) to be selected, give up now */

    {
    NOTELAYER_SELECTION_INFO notelayer_selection_info;
    if(status_fail(object_call_id(OBJECT_ID_NOTE, p_docu, T5_MSG_NOTELAYER_SELECTION_INFO, &notelayer_selection_info)))
        return(STATUS_OK);      /* no note(s) selected, give up now */
    save_callback.picture_object_id = notelayer_selection_info.object_id;
    save_callback.picture_object_data_ref = notelayer_selection_info.object_data_ref;
    } /*block*/

    status_return(save_foreign_filemap_create(&save_callback, &save_callback.ui_text, &save_foreign_type_list_source, TRUE));

    /* encode initial state of control(s) */
    save_callback.init_filename.type = UI_TEXT_TYPE_RESID;
    save_callback.init_filename.text.resource_id = MSG_DIALOG_SAVE_PICTURE_NAME;
    name_set = FALSE;

    {
    TCHARZ directory[BUF_MAX_PATHSTRING];
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
    quick_tblock_with_buffer_setup(quick_tblock);

    status_assert(name_make_wholename(&p_docu->docu_name, &quick_tblock, TRUE));
    if(!file_dirname(directory, name_set ? ui_text_tstr(&save_callback.init_filename) : quick_tblock_tstr(&quick_tblock)))
        save_callback.init_directory.type = UI_TEXT_TYPE_NONE;
    else
        status_assert(ui_text_alloc_from_tstr(&save_callback.init_directory, directory));

    quick_tblock_dispose(&quick_tblock);
    } /*block*/

#if RISCOS
    /* create a list of filetypes and textual representations thereof from the loaded objects */
    save_pict.relative_offset[0] = -SAVE_FOREIGN_PICT_OFFSET_H;

    save_foreign_type_list.relative_control_id[0] = SAVE_ID_NAME;
    save_foreign_type_list.relative_control_id[1] = SAVE_ID_NAME;

    assert((save_picture_ctl_create[3].p_dialog_control.p_dialog_control == &save_foreign_type_list) || (NULL == save_picture_ctl_create[3].p_dialog_control.p_dialog_control));
    save_picture_ctl_create[3].p_dialog_control.p_dialog_control = (array_elements(&save_foreign_type_list_source.source.array_handle) >= 2) ? &save_foreign_type_list : NULL;

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, save_picture_ctl_create, elemof32(save_picture_ctl_create), MSG_DIALOG_SAVE_PICTURE_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_SAVE_PICTURE_CAPTION;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_save_common;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &save_callback;
    status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/
#else
    status = windows_save_as(p_docu, &save_callback);
#endif

    al_array_dispose(&save_foreign_type_list_source.source.array_handle);

    ui_text_dispose(&save_callback.ui_text);

    ui_text_dispose(&save_callback.init_directory);
    ui_text_dispose(&save_callback.init_filename);
    ui_text_dispose(&save_callback.filename);

    al_array_dispose(&save_callback.other_filemap);

    return(status);
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_picture_intro)

_Check_return_
static STATUS
save_template_save(
    _DocuRef_   P_DOCU p_docu,
    P_SAVE_CALLBACK p_save_callback,
    _In_z_      PCTSTR filename)
{
    OF_OP_FORMAT of_op_format = OF_OP_FORMAT_INIT;
    STATUS status;

    * (P_U32) &of_op_format.of_template = 0xFFFFFFFFU;

    status_return(ownform_initialise_save(p_docu, &of_op_format, NULL, filename, p_save_callback->t5_filetype, NULL));

    switch(p_save_callback->style_radio)
    {
    default:
        status = save_ownform_file(p_docu, &of_op_format);
        break;

    case 1:
        {
        STYLE_HANDLE style_handle = STYLE_HANDLE_ENUM_START;
        P_STYLE p_style;

        status = STATUS_OK;

        while(style_enum_styles(p_docu, &p_style, &style_handle) >= 0)
        {
            status_break(status = skeleton_save_style_handle(&of_op_format, style_handle, TRUE));
        }

        break;
        }

    case 2:
        status_assertc(p_save_callback->style_handle);
        status = skeleton_save_style_handle(&of_op_format, p_save_callback->style_handle, TRUE);
        break;
    }

    return(ownform_finalise_save(&p_docu, &of_op_format, status));
}

static DIALOG_CTL_CREATE
save_template_ctl_create[] =
{
    { &dialog_main_group },

#if RISCOS
    { &save_pict, &save_pict_data },
#endif

    { &save_name_template,        &save_name_data },
    { &save_template_style_group },
    { &save_template_style_list,  &stdlisttext_data_dd }, /* first to get initial state set! */
    { &save_template_style_0,     &save_template_style_0_data },
    { &save_template_style_1,     &save_template_style_1_data },
    { &save_template_style_2,     &save_template_style_2_data },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &defbutton_ok, &save_ok_data }
};

static void
t5_cmd_save_template_common_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback)
{
    zero_struct_ptr(p_save_callback);

    p_save_callback->docno = docno_from_p_docu(p_docu);

    p_save_callback->saving = SAVING_TEMPLATE;
    p_save_callback->p_proc_save = save_template_save;

    p_save_callback->t5_filetype = FILETYPE_T5_TEMPLATE;

    p_save_callback->style_handle = STATUS_FAIL;
    p_save_callback->allow_not_rooted = TRUE;
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_template_intro)
{
    SAVE_CALLBACK save_callback;
    STATUS status;
    ARRAY_HANDLE save_template_style_list_handle;
    PIXIT max_width;

    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    t5_cmd_save_template_common_init(p_docu, &save_callback);

    save_callback.init_filename.type = UI_TEXT_TYPE_RESID;
    save_callback.init_filename.text.resource_id = MSG_DIALOG_SAVE_TEMPLATE_NAME;

    /* encode initial state of control(s) */
    status_return(ui_list_create_style(p_docu, &save_template_style_list_handle, &save_template_style_list_source, &max_width));

    { /* make appropriate size box */
    PIXIT_SIZE list_size;
    DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
    dialog_cmd_ctl_size_estimate.p_dialog_control = &save_template_style_list;
    dialog_cmd_ctl_size_estimate.p_dialog_control_data = &stdlisttext_data;
    ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
    dialog_cmd_ctl_size_estimate.size.x += max_width;
    ui_list_size_estimate(array_elements(&save_template_style_list_handle), &list_size);
    dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
    dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
    dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, SAVE_TEMPLATE_LIST_H);
    save_template_style_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
    save_template_style_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
    } /*block*/

#if RISCOS
    save_callback.test_for_saved_file_is_safe = TRUE;
    host_xfer_set_saved_file_is_safe(TRUE);

    save_pict.relative_offset[0] = -SAVE_TEMPLATE_PICT_OFFSET_H;
#endif

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, save_template_ctl_create, elemof32(save_template_ctl_create), MSG_DIALOG_SAVE_TEMPLATE_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_SAVE_TEMPLATE_CAPTION;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_save_common;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &save_callback;
    status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    ui_list_dispose_style(&save_template_style_list_handle, &save_template_style_list_source);

    ui_text_dispose(&save_callback.init_filename);
    ui_text_dispose(&save_callback.filename);

    return(status);
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_template_intro)

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_template)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 3);
    SAVE_CALLBACK save_callback;
    PCTSTR filename;

    IGNOREPARM_InVal_(t5_message);

    t5_cmd_save_template_common_init(p_docu, &save_callback);

    filename = p_args[0].val.tstr;
    assert(p_args[0].type == (ARG_TYPE_TSTR | ARG_MANDATORY));

    save_callback.style_radio = p_args[1].val.fBool;
    assert(p_args[1].type == (ARG_TYPE_BOOL | ARG_MANDATORY));

    if(save_callback.style_radio == SAVING_TEMPLATE_ONE_STYLE)
    {
        PCTSTR tstr_style_name = p_args[2].val.tstr;
        assert(p_args[2].type == ARG_TYPE_TSTR);

        if(0 == (save_callback.style_handle = style_handle_from_name(p_docu, tstr_style_name)))
            return(create_error(ERR_NAMED_STYLE_NOT_FOUND));
    }

    return(proc_save_common(filename, (CLIENT_HANDLE) &save_callback));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_template)

_Check_return_
static T5_FILETYPE
which_t5_filetype(
    _DocuRef_   P_DOCU p_docu)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(g_product_id)
    {
    default:
        if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
            return(FILETYPE_T5_RECORDZ);

        return(FILETYPE_T5_FIREWORKZ);
    }
}

#if WINDOWS

#include <dlgs.h>

static void
filetype_changed(
    _HwndRef_   HWND hwnd_filter_combo,
    _In_        int nFilterIndex,
    _HwndRef_opt_ HWND hwnd_edit,
    _HwndRef_opt_ HWND hwnd_filename_combo)
{
    STATUS status = STATUS_OK;
    DWORD len_filter = ComboBox_GetLBTextLen(hwnd_filter_combo, nFilterIndex);
    if(len_filter != CB_ERR)
    {
        PTSTR tstr_filter;
        len_filter += 1 /*for CH_NULL*/;
        tstr_filter = al_ptr_alloc_elem(TCHARZ, len_filter, &status);
        if(NULL != tstr_filter)
        {
            static const TCHARZ test[] = TEXT("(*.");
            PTSTR tstr_extension;
            (void) ComboBox_GetLBText(hwnd_filter_combo, nFilterIndex, tstr_filter);

            tstr_extension = tstrstr(tstr_filter, test);
            if(NULL != tstr_extension) /* replace any current extension with what is enclosed here - wot about (*.bmp,*dib) - just the first */
            {
                if(NULL != hwnd_edit)
                {
                    DWORD len_edit = Edit_GetTextLength(hwnd_edit);
                    if(0 != len_edit)
                    {
                        PTSTR tstr_edit;
                        len_edit += 1 /*for CH_NULL*/ + 4 /*may need .ext sticking on the end too*/;
                        tstr_edit = al_ptr_alloc_elem(TCHARZ, len_edit, &status);
                        if(NULL != tstr_edit)
                        {
                            PTSTR tstr_curext;
                            Edit_GetText(hwnd_edit, tstr_edit, (int) len_edit);
                            tstr_curext = file_extension(tstr_edit);
                            if(NULL == tstr_curext)
                            {
                                tstr_curext = tstr_edit + tstrlen32(tstr_edit);
                                *tstr_curext++ = FILE_EXT_SEP_CH;
                            }
                            tstr_extension += elemof32(test)-1;
                            { /* copy over the new extension */
                            TCHAR ch;
                            while(((ch = *tstr_extension++) != CH_RIGHT_PARENTHESIS) && (ch != CH_COMMA) && ch)
                                *tstr_curext++ = ch;
                            *tstr_curext = CH_NULL;
                            } /*block*/
                            Edit_SetText(hwnd_edit, tstr_edit);
                            tstr_clr(&tstr_edit);
                        }
                    }
                }
                else if(NULL != hwnd_filename_combo)
                {
                    DWORD len_filename_combo = ComboBox_GetTextLength(hwnd_filename_combo);
                    if(0 != len_filename_combo)
                    {
                        PTSTR tstr_filename_combo;
                        len_filename_combo += 1 /*for CH_NULL*/ + 4 /*may need .ext sticking on the end too*/;
                        tstr_filename_combo = al_ptr_alloc_elem(TCHARZ, len_filename_combo, &status);
                        if(NULL != tstr_filename_combo)
                        {
                            PTSTR tstr_curext;
                            ComboBox_GetText(hwnd_filename_combo, tstr_filename_combo, (int) len_filename_combo);
                            tstr_curext = file_extension(tstr_filename_combo);
                            if(NULL == tstr_curext)
                            {
                                tstr_curext = tstr_filename_combo + tstrlen32(tstr_filename_combo);
                                *tstr_curext++ = FILE_EXT_SEP_CH;
                            }
                            tstr_extension += elemof32(test)-1;
                            { /* copy over the new extension */
                            TCHAR ch;
                            while(((ch = *tstr_extension++) != CH_RIGHT_PARENTHESIS) && (ch != CH_COMMA) && ch)
                                *tstr_curext++ = ch;
                            *tstr_curext = CH_NULL;
                            } /*block*/
                            ComboBox_SetText(hwnd_filename_combo, tstr_filename_combo);
                            tstr_clr(&tstr_filename_combo);
                        }
                    }
                }
            }

            tstr_clr(&tstr_filter);
        }
    }
    status_assert(status);
}

/* Hook procedure */

static void
SaveFileHook_onNotify(
    _HwndRef_   HWND hwnd,
    _InRef_     LPOFNOTIFY lpon)
{
    switch(lpon->hdr.code)
    {
    case CDN_TYPECHANGE:
        {
        int nFilterIndex = lpon->lpOFN->nFilterIndex - 1;
        /* remarkably 1136 is still the id of the filter combo box */
        HWND hwnd_filter_combo = GetDlgItem(hwnd, cmb1 /*1136*/);
        HWND hwnd_filename_combo = GetDlgItem(hwnd, cmb13); /* This takes precedence. Possibly. */
        /* remarkably 1152 is still the id of the filename edit box even in 32-bit Windows. Hmm, no more it seems... */
        HWND hwnd_edit = hwnd_filename_combo ? NULL : GetDlgItem(hwnd, edt1 /*1152*/);
        filetype_changed(hwnd_filter_combo, nFilterIndex, hwnd_edit, hwnd_filename_combo);
        break;
        }

    default:
        break;
    }
}

extern UINT_PTR CALLBACK
SaveFileHook(
    _HwndRef_   HWND hdlg,      /* handle to child dialog window NOT the 'Save As' window!!! */
    _In_        UINT uiMsg,     /* message identifier */
    _In_        WPARAM wParam,  /* message parameter */
    _In_        LPARAM lParam)  /* message parameter */
{
    const HWND hwnd = GetParent(hdlg); /* *This* is the 'Save As' window handle */

    IGNOREPARM(wParam);

    switch(uiMsg)
    {
    case WM_NOTIFY:
        SaveFileHook_onNotify(hwnd, (LPOFNOTIFY) lParam);
        break;

    default:
        break;
    }

    return(0); /* allow default action */
}

_Check_return_
static STATUS
windows_save_as(
    _DocuRef_   P_DOCU p_docu,
    P_SAVE_CALLBACK p_save_callback)
{
    STATUS status = STATUS_OK;
    TCHARZ szDirName[BUF_MAX_PATHSTRING];
    TCHARZ szFile[BUF_MAX_PATHSTRING];
    TCHARZ szDialogTitle[64];
    const S32 filter_mask = (p_save_callback->saving == SAVING_PICTURE) ? BOUND_FILETYPE_WRITE_PICT : BOUND_FILETYPE_WRITE;
    OPENFILENAME openfilename;
    BOOL ofnResult;
    QUICK_TBLOCK_WITH_BUFFER(filter_quick_tblock, 32);
    quick_tblock_with_buffer_setup(filter_quick_tblock);

    /* build filters as description,CH_NULL,wildcard spec,CH_NULL sets */
    status_return(windows_filter_list_create(&filter_quick_tblock, filter_mask));

    /* try to find some sensible place to dump these */
    if(!ui_text_is_blank(&p_save_callback->init_directory))
        tstr_xstrkpy(szDirName, elemof32(szDirName), ui_text_tstr(&p_save_callback->init_directory));
    else if(0 == MyGetProfileString(TEXT("DefaultDirectory"), tstr_empty_string, szDirName, elemof32(szDirName)))
    {
        if(!GetPersonalDirectoryName(szDirName, elemof32(szDirName)))
        {
            if(GetModuleFileName(GetInstanceHandle(), szDirName, elemof32(szDirName)))
            {
                file_dirname(szDirName, szDirName);
            }
        }
    }

    {
    U32 len = tstrlen32(szDirName);
    if(len > elemof32(TEXT("C:") FILE_DIR_SEP_TSTR)-1)
    {
        PTSTR tstr = szDirName + len;
        if(*--tstr == FILE_DIR_SEP_CH)
            *tstr = CH_NULL;
    }
    } /*block*/

    tstr_xstrkpy(szFile, elemof32(szFile), ui_text_tstr(&p_save_callback->init_filename));

    zero_struct(openfilename);

    openfilename.lStructSize = sizeof32(openfilename);
    {
    const P_VIEW p_view = p_view_from_viewno_caret(p_docu);
    openfilename.hwndOwner = !IS_VIEW_NONE(p_view) ? p_view->main[WIN_BACK].hwnd : NULL /*host_get_icon_hwnd()*/;
    } /*block*/
    openfilename.hInstance = GetInstanceHandle();
    openfilename.lpstrFilter = quick_tblock_tstr(&filter_quick_tblock);
    openfilename.lpstrCustomFilter = 0;
    openfilename.nMaxCustFilter = 0;
    openfilename.nFilterIndex = 1;
    openfilename.lpstrFile = szFile;
    openfilename.nMaxFile = elemof32(szFile);
  /*openfilename.lpstrFileTitle = NULL;*/
  /*openfilename.nMaxFileTitle = 0;*/
    openfilename.lpstrInitialDir = szDirName;
    openfilename.lpstrTitle = NULL;
    if(p_save_callback->save_selection)
    {
        resource_lookup_tstr_buffer(szDialogTitle, elemof32(szDialogTitle), MSG_DIALOG_SAVE_SELECTION_CAPTION);
        openfilename.lpstrTitle = szDialogTitle;
    }
    openfilename.Flags =
        OFN_NOCHANGEDIR         |
        OFN_PATHMUSTEXIST       |
        OFN_HIDEREADONLY        |
        OFN_NOREADONLYRETURN    |
        OFN_OVERWRITEPROMPT     |
        OFN_EXPLORER            |
        OFN_ENABLESIZING        |
        OFN_LONGNAMES           ;
    openfilename.nFileOffset = 0;
    openfilename.nFileExtension = 0;
    openfilename.lpstrDefExt = file_extension(szFile); /* let user call it what they like! */
    if(NULL == openfilename.lpstrDefExt)
    {
        switch(p_save_callback->saving)
        {
        case SAVING_OWNFORM:
            openfilename.lpstrDefExt = extension_document_tstr;
            break;
        case SAVING_PICTURE:
            openfilename.lpstrDefExt = TEXT("aff"); /* without the preceding dot */
            break;
        default:
            openfilename.lpstrDefExt = TEXT("txt");
            break;
        }
    }
    openfilename.lCustData = 0;
    openfilename.lpTemplateName = NULL;

    openfilename.Flags |= OFN_ENABLEHOOK;
    openfilename.lpfnHook = SaveFileHook;

    /* finally, try not to suggest saving files without extensions on Windows */
    if(NULL == file_extension(openfilename.lpstrFile))
    {
        tstr_xstrkat(openfilename.lpstrFile, openfilename.nMaxFile, FILE_EXT_SEP_TSTR);
        tstr_xstrkat(openfilename.lpstrFile, openfilename.nMaxFile, openfilename.lpstrDefExt);
    }

    ofnResult = GetSaveFileName(&openfilename);

    quick_tblock_dispose(&filter_quick_tblock);

    if(!ofnResult)
        switch(CommDlgExtendedError())
        {
        case 0: return(STATUS_CANCEL);
        case CDERR_STRUCTSIZE:      return(STATUS_FAIL);
        case CDERR_INITIALIZATION:  return(status_nomem());
        case CDERR_NOTEMPLATE:      return(STATUS_FAIL);
        case CDERR_LOADSTRFAILURE:  return(create_error(FILE_ERR_LOADSTRFAIL));
        case CDERR_FINDRESFAILURE:  return(create_error(FILE_ERR_FINDRESFAIL));
        case CDERR_LOADRESFAILURE:  return(create_error(FILE_ERR_LOADRESFAIL));
        case CDERR_LOCKRESFAILURE:  return(create_error(FILE_ERR_LOCKRESFAIL));
        case CDERR_MEMALLOCFAILURE: return(status_nomem());
        case CDERR_MEMLOCKFAILURE:  return(status_nomem());
        case CDERR_NOHOOK:          return(STATUS_FAIL);
        case CDERR_REGISTERMSGFAIL: return(STATUS_FAIL);
        case FNERR_SUBCLASSFAILURE: return(status_nomem());
        case FNERR_INVALIDFILENAME: return(create_error(FILE_ERR_BADNAME));
        case FNERR_BUFFERTOOSMALL:  return(status_nomem());
        default:                    return(STATUS_FAIL);
        }

#if 1
    { /* use nFilterIndex to determine file type to save as */
    T5_FILETYPE t5_filetype;
    t5_filetype = windows_filter_list_get_t5_filetype_from_filter_index(openfilename.nFilterIndex, filter_mask);
#else
    { /* NB User might have stripped the extension off - ensure we stick the default one on if so */
    T5_FILETYPE t5_filetype;
    PCTSTR tstr_extension = file_extension(openfilename.lpstrFile);

    if(NULL == tstr_extension)
    {
        tstr_extension = openfilename.lpstrDefExt;
        tstr_xstrkat(openfilename.lpstrFile, openfilename.nMaxFile, FILE_EXT_SEP_TSTR);
        tstr_xstrkat(openfilename.lpstrFile, openfilename.nMaxFile, tstr_extension);
    }

    t5_filetype = t5_filetype_from_extension(tstr_extension);
#endif

    switch(t5_filetype)
    {
    case FILETYPE_T5_TEMPLATE:
    case FILETYPE_T5_WORDZ:
    case FILETYPE_T5_RESULTZ:
    case FILETYPE_T5_FIREWORKZ:
        /* mutate into save_ownform if was foreign save */
        if(p_save_callback->saving != SAVING_OWNFORM)
        {
            p_save_callback->saving = SAVING_OWNFORM;
            p_save_callback->p_proc_save = save_ownform_save;

            p_save_callback->rename_after_save = TRUE;
            p_save_callback->clear_modify_after_save = TRUE;
        }

        p_save_callback->t5_filetype = t5_filetype;
        status = proc_save_common(openfilename.lpstrFile, (CLIENT_HANDLE) p_save_callback);
        break;

    case FILETYPE_UNDETERMINED:
        t5_filetype = FILETYPE_TEXT;

        /*FALLTHRU*/

    default:
        /* mutate into save_foreign if was normal save */
        if(p_save_callback->saving == SAVING_OWNFORM)
        {
            p_save_callback->saving = SAVING_FOREIGN;
            p_save_callback->p_proc_save = save_foreign_save;

            status_break(status = save_foreign_filemap_create(p_save_callback, NULL, NULL, FALSE));

            p_save_callback->rename_after_save = FALSE;
            p_save_callback->clear_modify_after_save = FALSE;
        }

        p_save_callback->t5_filetype = t5_filetype;
        status = proc_save_common(openfilename.lpstrFile, (CLIENT_HANDLE) p_save_callback);
        break;
    }
    } /*block*/

    return(status);
}

#endif /* WINDOWS */

#if RISCOS

static const DIALOG_CONTROL
locate_waffle_1 =
{
    SAVE_ID_WAFFLE_1, DIALOG_CONTROL_WINDOW,
    { DIALOG_MAIN_GROUP, DIALOG_MAIN_GROUP },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, DIALOG_STDTEXT_V },
    { DRT(RTLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
locate_waffle_1_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_LOCATE_TEMPLATE_WAFFLE_1), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
locate_waffle_2 =
{
    SAVE_ID_WAFFLE_2, DIALOG_CONTROL_WINDOW,
    { SAVE_ID_WAFFLE_1, SAVE_ID_WAFFLE_1 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDTEXT_V },
    { DRT(LBLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
locate_waffle_2_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_LOCATE_TEMPLATE_WAFFLE_2), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
locate_waffle_3 =
{
    SAVE_ID_WAFFLE_3, DIALOG_CONTROL_WINDOW,
    { SAVE_ID_WAFFLE_2, SAVE_ID_WAFFLE_2 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDTEXT_V },
    { DRT(LBLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
locate_waffle_3_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_LOCATE_TEMPLATE_WAFFLE_3), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
locate_waffle_4 =
{
    SAVE_ID_WAFFLE_4, DIALOG_CONTROL_WINDOW,
    { SAVE_ID_WAFFLE_3, SAVE_ID_WAFFLE_3 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDTEXT_V },
    { DRT(LBLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
locate_waffle_4_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_LOCATE_TEMPLATE_WAFFLE_4), { 1 /*left_text*/ } };

static const DIALOG_CTL_CREATE
locate_ctl_create[] =
{
    { &dialog_main_group },
    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &defbutton_ok, &save_ok_data },

    { &locate_waffle_1, &locate_waffle_1_data },
    { &locate_waffle_2, &locate_waffle_2_data },
    { &locate_waffle_3, &locate_waffle_3_data },
    { &locate_waffle_4, &locate_waffle_4_data },

    { &save_pict, &save_pict_data },
    { &save_name_control, &save_name_data }
};

#endif /* RISCOS */

static PTSTR g_dirname_buffer;
static U32 g_elemof_dirname_buffer;

_Check_return_
static STATUS
save_template_locate(
    _DocuRef_   P_DOCU cur_p_docu,
    P_SAVE_CALLBACK p_save_callback,
    _In_z_      PCTSTR filename)
{
    STATUS status = STATUS_OK;

#if RISCOS
    _kernel_swi_regs rs;
    _kernel_oserror * e;
    rs.r[0] = OSFile_ReadNoPath;
    rs.r[1] = (int) filename;
    if(NULL != (e = WrapOsErrorChecking(_kernel_swi(OS_File, &rs, &rs))))
        return(file_error_set(e->errmess));
    if(rs.r[0] != 0)
        return(create_error(ERR_TEMPLATE_DEST_EXISTS));

    rs.r[0] = 26;
    rs.r[1] = (int) p_save_callback->initial_template_dir;
    rs.r[2] = (int) filename;
    rs.r[3] = 0x5603; /*~a~c~dfln~p~qrs~t~v*/
    if(NULL != (e = WrapOsErrorChecking(_kernel_swi(OS_FSControl, &rs, &rs))))
        return(file_error_set(e->errmess));
#else
    IGNOREPARM(p_save_callback);
#endif

    tstr_xstrkpy(g_dirname_buffer, g_elemof_dirname_buffer, filename);

    IGNOREPARM(cur_p_docu);

    return(status);
}

_Check_return_
extern STATUS
locate_copy_of_dir_template(
    _DocuRef_   P_DOCU cur_p_docu,
    _In_z_      PCTSTR filename_template,
    _Out_writes_z_(elemof_dirname_buffer) PTSTR dirname_buffer,
    _InVal_     U32 elemof_dirname_buffer)
{
    SAVE_CALLBACK save_callback;
    STATUS status;

    dirname_buffer[0] = CH_NULL;

    g_dirname_buffer = dirname_buffer;
    g_elemof_dirname_buffer = elemof_dirname_buffer;

    zero_struct(save_callback);

    save_callback.docno = docno_from_p_docu(cur_p_docu);

    save_callback.saving = LOCATING_TEMPLATE;
    save_callback.p_proc_save = save_template_locate;

    save_callback.t5_filetype = FILETYPE_DIRECTORY;

    save_callback.initial_template_dir = filename_template;

    save_callback.init_filename.type = UI_TEXT_TYPE_TSTR_TEMP;
    save_callback.init_filename.text.tstr = file_leafname(save_callback.initial_template_dir);

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
#if RISCOS
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, locate_ctl_create, elemof32(locate_ctl_create), 0);
#else
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, NULL, 0, 0);
#endif
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_LOCATE_TEMPLATE_CAPTION;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_save_common;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &save_callback;
    status = call_dialog_with_docu(cur_p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    ui_text_dispose(&save_callback.init_filename);
    ui_text_dispose(&save_callback.filename);

    return(status);
}

extern void
style_name_from_marked_area(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_UI_TEXT p_ui_text)
{
    SELECTION_STYLE selection_style;

    /* enquire about just these bits of state */
    style_selector_clear(&selection_style.selector_in);
    style_selector_bit_set(&selection_style.selector_in, STYLE_SW_NAME);

    style_selector_clear(&selection_style.selector_fuzzy_out);

    style_init(&selection_style.style_out);

    /* ignore transient styles */
    style_region_class_limit_set(p_docu, REGION_UPPER);
    /* may return STATUS_FAIL but we've had to take care of style & selectors anyway */
    status_consume(object_skel(p_docu, T5_MSG_SELECTION_STYLE, &selection_style));
    style_region_class_limit_set(p_docu, REGION_END);

    p_ui_text->type = UI_TEXT_TYPE_NONE;

    if(!style_selector_bit_test(&selection_style.selector_fuzzy_out, STYLE_SW_NAME) &&
        style_bit_test(&selection_style.style_out, STYLE_SW_NAME) )
    {
        p_ui_text->type = UI_TEXT_TYPE_TSTR_ARRAY;
        p_ui_text->text.array_handle_tstr = selection_style.style_out.h_style_name_tstr;
    }
}

/* end of ui_save.c */
