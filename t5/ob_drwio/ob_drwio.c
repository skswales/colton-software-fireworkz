/* ob_drwio.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2001-2015 R W Colton */

/* Save as Drawfile object module for Fireworkz */

/* SKS July 2001 */

#include "common/gflags.h"

#include "ob_drwio/ob_drwio.h"

#include "ob_file/xp_file.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#if RISCOS
#include "ob_dlg/xp_dlgr.h"
#endif

#if WINDOWS
#include "commdlg.h"

#include "cderr.h"

#include "direct.h"
#endif

#include "ob_file/xp_file.h"

#if RISCOS
#define MSG_WEAK &rb_draw_io_msg_weak
extern PC_U8 rb_draw_io_msg_weak;
#endif
#define P_BOUND_RESOURCES_OBJECT_ID_DRAW_IO NULL

#if !RISCOS
#define DRAWFILE_EXTENSION_TSTR FILE_EXT_SEP_TSTR TEXT("aff")
#endif

/*
internal structures
*/

typedef struct SAVE_AS_DRAWFILE_CALLBACK * P_SAVE_AS_DRAWFILE_CALLBACK;

typedef struct SAVE_AS_DRAWFILE_CALLBACK
{
    /* this first half from SAVE_CALLBACK */

    DOCNO        docno;
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

    UI_TEXT ui_text;

    /* this second half from PRINT_CALLBACK */

    /*S32 extra;*/
    S32 max_page_y;
    S32 all_or_range;

    UI_TEXT range_list;
}
SAVE_AS_DRAWFILE_CALLBACK;

/*
internal routines
*/

static void
save_as_drawfile_percentage_reflect(
    P_PRINTER_PERCENTAGE p_save_as_drawfile_percentage,
    _InVal_     S32 sub_percentage);

static void
save_as_drawfile_details_query(
    _InoutRef_  P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback,
    _OutRef_    P_T5_FILETYPE p_t5_filetype,
    /*out*/ P_UI_TEXT p_ui_text_filename);

_Check_return_
static STATUS
save_as_drawfile_dialog_check(
    _InoutRef_  P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback);

_Check_return_
static STATUS
save_drawfile_save(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback,
    _In_z_      PCTSTR filename);

/* ------------------------------------------------------------------------------------------------------- */

/*
construct argument types
*/

static const ARG_TYPE
args_cmd_save_as_drawfile[] =
{
#define ARG_SAVE_AS_DRAWFILE_FILENAME 0
    ARG_TYPE_TSTR | ARG_MANDATORY, /* filename  */
#define ARG_SAVE_AS_DRAWFILE_RANGE 1
    ARG_TYPE_S32,   /* 0=all/1=range */
#define ARG_SAVE_AS_DRAWFILE_RANGE_LIST 2
    ARG_TYPE_TSTR,  /* range  */
#define ARG_SAVE_AS_DRAWFILE_N_ARGS 3
    ARG_TYPE_NONE
};

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "SaveAsDrawfileIntro",    NULL,                       T5_CMD_SAVE_AS_DRAWFILE_INTRO,              { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "SaveAsDrawfile",         args_cmd_save_as_drawfile,  T5_CMD_SAVE_AS_DRAWFILE,                    { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },

#if WINDOWS && defined(T5_CMD_SAVE_AS_METAFILE)
    { "SaveAsMetafileIntro",    NULL,                       T5_CMD_SAVE_AS_METAFILE_INTRO,              { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "SaveAsMetafile",         args_cmd_save_as_drawfile,  T5_CMD_SAVE_AS_METAFILE,                    { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
#endif

    { NULL,                     NULL,                       T5_EVENT_NONE } /* end of table */
};

enum SAVE_AS_DRAWFILE_CTRL_IDS
{
#if RISCOS
    CONTROL_ID_NAME = 33,
    CONTROL_ID_PICT,
#endif

#define AR_GROUP
#ifdef AR_GROUP
    CONTROL_ID_AR_GROUP = 64,
    CONTROL_ID_ALL,
    CONTROL_ID_RANGE,

    CONTROL_ID_RANGE_GROUP,
    CONTROL_ID_RANGE_EDIT
#else
    CONTROL_ID_DUMMY
#endif
};

#define SAVE_AS_DRAWFILE_TOTAL_H        ((PIXITS_PER_INCH * 22) / 10)

#if RISCOS

#define SAVE_AS_DRAWFILE_PICT_OFFSET_H  ((SAVE_AS_DRAWFILE_TOTAL_H - 68 * PIXITS_PER_RISCOS) / 2)

static DIALOG_CONTROL
save_as_drawfile_pict =
{
    CONTROL_ID_PICT, DIALOG_MAIN_GROUP,
    { CONTROL_ID_NAME, DIALOG_CONTROL_PARENT },
    { -SAVE_AS_DRAWFILE_PICT_OFFSET_H, 0, 68 * PIXITS_PER_RISCOS, 68 * PIXITS_PER_RISCOS /* standard RISC OS file icon size */ },
    { DRT(LTLT, USER) }
};

static const RGB
save_as_drawfile_pict_rgb = { 0, 0, 0, 1 /*transparent*/ };

static const DIALOG_CONTROL_DATA_USER
save_as_drawfile_pict_data = { 0, { FRAMED_BOX_NONE /* border_style */ }, (P_RGB) &save_as_drawfile_pict_rgb };

/*
* name of file
*/

static DIALOG_CONTROL
save_as_drawfile_name =
{
    CONTROL_ID_NAME, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, CONTROL_ID_PICT, CONTROL_ID_RANGE_GROUP },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V },
    { DRT(LBRT, EDIT), 1 }
};

static
BITMAP(save_as_drawfile_name_validation, 256);

static DIALOG_CONTROL_DATA_EDIT
save_as_drawfile_name_data = { { { FRAMED_BOX_EDIT }, save_as_drawfile_name_validation }, /* EDIT_XX */ { UI_TEXT_TYPE_NONE } /* state */ };

#endif /* RISCOS */

#ifdef AR_GROUP

static const DIALOG_CONTROL
save_as_drawfile_ar_group =
{
    CONTROL_ID_AR_GROUP, DIALOG_MAIN_GROUP,
#if RISCOS
    { CONTROL_ID_NAME, CONTROL_ID_NAME, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, 0, 0 },
    { DRT(LBRB, GROUPBOX), 0, 1 /*logical_group*/ }
#else
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, 0, 0 },
    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
#endif
};

static const DIALOG_CONTROL
save_as_drawfile_all =
{
    CONTROL_ID_ALL, CONTROL_ID_AR_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
save_as_drawfile_all_data = { { 0 }, 0 /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_ALL) };

static const DIALOG_CONTROL
save_as_drawfile_range =
{
    CONTROL_ID_RANGE, CONTROL_ID_AR_GROUP,
    { CONTROL_ID_ALL, CONTROL_ID_ALL },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTONF
save_as_drawfile_range_data = { { { 0, 1 /*move_focus*/ }, 1 /* activate_state */, UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_RANGE) }, CONTROL_ID_RANGE_EDIT };

static const DIALOG_CONTROL
save_as_drawfile_range_group =
{
    CONTROL_ID_RANGE_GROUP, DIALOG_MAIN_GROUP,
    { CONTROL_ID_AR_GROUP, CONTROL_ID_RANGE, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDSPACING_H, (DIALOG_STDBUMP_V - DIALOG_STDRADIO_V) /2 /*DIALOG_SMALLSPACING_V*/ },
    { DRT(RTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
save_as_drawfile_range_edit =
{
    CONTROL_ID_RANGE_EDIT, CONTROL_ID_RANGE_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, PIXITS_PER_INCH + PIXITS_PER_HALF_INCH, PIXITS_PER_INCH },
    { DRT(LTLT, EDIT), 1 }
};

static BITMAP(save_as_drawfile_range_edit_validation, 256);

static const DIALOG_CONTROL_DATA_EDIT
save_as_drawfile_range_edit_data = { { { FRAMED_BOX_EDIT, 0, 0, 1 /*multiline*/ }, save_as_drawfile_range_edit_validation } /* EDIT_XX */ };

#endif /* AR_GROUP */

/* OK - this is NOT defbutton_ok_data! */

static const DIALOG_CONTROL_DATA_PUSHBUTTON
save_as_drawfile_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_OK) };

static const DIALOG_CONTROL
save_as_drawfile_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_MAIN_GROUP, DIALOG_MAIN_GROUP, DIALOG_CONTROL_SELF },
    { DIALOG_CONTENTS_CALC, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CTL_CREATE
save_as_drawfile_dialog_create[] =
{
    { &dialog_main_group },

#if RISCOS
    { &save_as_drawfile_pict, &save_as_drawfile_pict_data },
    { &save_as_drawfile_name, &save_as_drawfile_name_data },
#endif

#ifdef AR_GROUP
    { &save_as_drawfile_ar_group },
    { &save_as_drawfile_all, &save_as_drawfile_all_data },
    { &save_as_drawfile_range, &save_as_drawfile_range_data },

    { &save_as_drawfile_range_group },
    { &save_as_drawfile_range_edit, &save_as_drawfile_range_edit_data },
#endif

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &save_as_drawfile_ok, &save_as_drawfile_ok_data }
};

static S32 save_as_drawfile_range_y0, save_as_drawfile_range_y1;

static BOOL
proc_save_as_drawfile(
    _In_z_      PCTSTR filename /*low lifetime*/,
    CLIENT_HANDLE client_handle)
{
    const P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback = (P_SAVE_AS_DRAWFILE_CALLBACK) client_handle;
    const P_DOCU p_docu = p_docu_from_docno(p_save_as_drawfile_callback->docno);
    TCHARZ save_name[BUF_MAX_PATHSTRING];
    STATUS status;

    tstr_xstrkpy(save_name, elemof32(save_name), filename);

    status = save_drawfile_save(p_docu, p_save_as_drawfile_callback, save_name);

    if(status_fail(status))
    {
        reperr(status, save_name);
        return(TRUE);
    }

    if(p_save_as_drawfile_callback->h_dialog)
    {
        DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
        msgclr(dialog_cmd_complete_dbox);
        dialog_cmd_complete_dbox.h_dialog = p_save_as_drawfile_callback->h_dialog;
        dialog_cmd_complete_dbox.completion_code = DIALOG_COMPLETION_OK;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox));
    }

    return(TRUE);
}

_Check_return_
static STATUS
dialog_save_as_drawfile_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
#if RISCOS
    const P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback = (P_SAVE_AS_DRAWFILE_CALLBACK) p_dialog_msg_ctl_create_state->client_handle;
#endif
    STATUS status = STATUS_OK;

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
#if RISCOS
    case CONTROL_ID_NAME:
        p_dialog_msg_ctl_create_state->state_set.state.edit.ui_text = p_save_as_drawfile_callback->init_filename;
        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_DONT_MSG;
        break;
#endif

#ifdef AR_GROUP
    case CONTROL_ID_RANGE_EDIT:
        {
        UI_TEXT ui_text;
        UCHARZ ustr_buf[256];

        consume_int(xsnprintf(ustr_buf, elemof32(ustr_buf),
                              S32_FMT " - " S32_FMT,
                              save_as_drawfile_range_y0, save_as_drawfile_range_y1));

        ui_text.type = UI_TEXT_TYPE_USTR_TEMP;
        ui_text.text.ustr = ustr_bptr(ustr_buf);
        status = ui_dlg_set_edit(p_dialog_msg_ctl_create_state->h_dialog, CONTROL_ID_RANGE_EDIT, &ui_text);
        p_dialog_msg_ctl_create_state->processed = 1;

        break;
        }
#endif

    default:
        break;
    }

    return(status);
}

_Check_return_
static STATUS
dialog_save_as_drawfile_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
#if RISCOS
    const P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback = (P_SAVE_AS_DRAWFILE_CALLBACK) p_dialog_msg_ctl_state_change->client_handle;
#endif
    STATUS status = STATUS_OK;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
#if RISCOS
    case CONTROL_ID_NAME:
        {
        ui_text_dispose(&p_save_as_drawfile_callback->filename);

        status = ui_text_copy(&p_save_as_drawfile_callback->filename, &p_dialog_msg_ctl_state_change->new_state.edit.ui_text);

        if(!p_save_as_drawfile_callback->self_abuse)
            p_save_as_drawfile_callback->filename_edited++;

        break;
        }
#endif

#ifdef AR_GROUP
    case CONTROL_ID_AR_GROUP:
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, CONTROL_ID_RANGE_GROUP, (p_dialog_msg_ctl_state_change->new_state.radiobutton == 1));
        break;
#endif

    default:
        break;
    }

    return(status);
}

#ifdef AR_GROUP

_Check_return_
static STATUS
dialog_save_as_drawfile_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback = (P_SAVE_AS_DRAWFILE_CALLBACK) p_dialog_msg_process_start->client_handle;

    return(ui_dlg_set_radio(p_dialog_msg_process_start->h_dialog, CONTROL_ID_AR_GROUP, p_save_as_drawfile_callback->all_or_range && (save_as_drawfile_range_y1 > 1)));
}

#endif /* AR_GROUP */

_Check_return_
static STATUS
dialog_save_as_drawfile_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    const P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback = (P_SAVE_AS_DRAWFILE_CALLBACK) p_dialog_msg_ctl_pushbutton->client_handle;

    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case IDOK:
        {
        PCTSTR filename;

        p_save_as_drawfile_callback->h_dialog = p_dialog_msg_ctl_pushbutton->h_dialog;

        save_as_drawfile_details_query(p_save_as_drawfile_callback, &p_save_as_drawfile_callback->t5_filetype, &p_save_as_drawfile_callback->filename);

        filename = ui_text_tstr(&p_save_as_drawfile_callback->filename);

        if(!p_save_as_drawfile_callback->allow_not_rooted && !file_is_rooted(filename))
        {
            reperr_null(create_error(ERR_SAVE_DRAG_TO_DIRECTORY));

            return(STATUS_OK);
        }

        status_return(save_as_drawfile_dialog_check(p_save_as_drawfile_callback));

        proc_save_as_drawfile(filename, (CLIENT_HANDLE) p_save_as_drawfile_callback);

        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

#if RISCOS

_Check_return_
static STATUS
dialog_save_as_drawfile_ctl_user_redraw(
    _InRef_     PC_DIALOG_MSG_CTL_USER_REDRAW p_dialog_msg_ctl_user_redraw)
{
    const P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback = (P_SAVE_AS_DRAWFILE_CALLBACK) p_dialog_msg_ctl_user_redraw->client_handle;
    T5_FILETYPE t5_filetype;
    WimpIconBlockWithBitset icon;

    p_save_as_drawfile_callback->h_dialog = p_dialog_msg_ctl_user_redraw->h_dialog;

    save_as_drawfile_details_query(p_save_as_drawfile_callback, &t5_filetype, NULL);

    zero_struct(icon);

    host_ploticon_setup_bbox(&icon, &p_dialog_msg_ctl_user_redraw->control_inner_box, &p_dialog_msg_ctl_user_redraw->redraw_context);

    /* fills in icon with name of a sprite representing the filetype from the Window Manager Sprite Pool */
    dialog_riscos_file_icon_setup(&icon, t5_filetype);

    host_ploticon(&icon);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_save_as_drawfile_ctl_user_mouse(
    _InRef_     PC_DIALOG_MSG_CTL_USER_MOUSE p_dialog_msg_ctl_user_mouse)
{
    const P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback = (P_SAVE_AS_DRAWFILE_CALLBACK) p_dialog_msg_ctl_user_mouse->client_handle;

    switch(p_dialog_msg_ctl_user_mouse->click)
    {
    case DIALOG_MSG_USER_MOUSE_CLICK_LEFT_DRAG:
        {
        WimpIconBlockWithBitset icon;

        p_save_as_drawfile_callback->h_dialog = p_dialog_msg_ctl_user_mouse->h_dialog;

        save_as_drawfile_details_query(p_save_as_drawfile_callback, &p_save_as_drawfile_callback->t5_filetype, &p_save_as_drawfile_callback->filename);

        zero_struct(icon);

        icon.bbox = p_dialog_msg_ctl_user_mouse->riscos.icon.bbox;

        /* fills in icon with name of a sprite representing the filetype from the Window Manager Sprite Pool */
        dialog_riscos_file_icon_setup(&icon, p_save_as_drawfile_callback->t5_filetype);

        status_return(save_as_drawfile_dialog_check(p_save_as_drawfile_callback));

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
dialog_save_as_drawfile_riscos_drag_ended(
    _InRef_     P_DIALOG_MSG_RISCOS_DRAG_ENDED p_dialog_msg_riscos_drag_ended)
{
    const P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback = (P_SAVE_AS_DRAWFILE_CALLBACK) p_dialog_msg_riscos_drag_ended->client_handle;

    /* try saving file to target window */
    if(p_dialog_msg_riscos_drag_ended->mouse.window_handle != -1)
    {
        status_return(save_as_drawfile_dialog_check(p_save_as_drawfile_callback));

        consume_bool(
            host_xfer_save_file(
                ui_text_tstr(&p_save_as_drawfile_callback->filename),
                p_save_as_drawfile_callback->t5_filetype,
                42 /*estimated_size*/,
                proc_save_as_drawfile,
                (CLIENT_HANDLE) p_save_as_drawfile_callback,
                &p_dialog_msg_riscos_drag_ended->mouse));
    }

    return(STATUS_OK);
}

#endif /* OS */

PROC_DIALOG_EVENT_PROTO(static, dialog_event_save_as_drawfile)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_save_as_drawfile_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_save_as_drawfile_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

#ifdef AR_GROUP
    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_save_as_drawfile_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));
#endif

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_save_as_drawfile_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

#if RISCOS

    case DIALOG_MSG_CODE_CTL_USER_REDRAW:
        return(dialog_save_as_drawfile_ctl_user_redraw((PC_DIALOG_MSG_CTL_USER_REDRAW) p_data));

    case DIALOG_MSG_CODE_CTL_USER_MOUSE:
        return(dialog_save_as_drawfile_ctl_user_mouse((PC_DIALOG_MSG_CTL_USER_MOUSE) p_data));

    case DIALOG_MSG_CODE_RISCOS_DRAG_ENDED:
        return(dialog_save_as_drawfile_riscos_drag_ended((P_DIALOG_MSG_RISCOS_DRAG_ENDED) p_data));

#endif /* RISCOS */

    default:
        return(STATUS_OK);
    }
}

static void
save_as_drawfile_details_query(
    _InoutRef_  P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback,
    _OutRef_    P_T5_FILETYPE p_t5_filetype,
    /*out*/ P_UI_TEXT p_ui_text_filename)
{
    *p_t5_filetype = FILETYPE_DRAW;

#if RISCOS
    if(NULL != p_ui_text_filename)
    {
        ui_text_dispose(p_ui_text_filename);

        ui_dlg_get_edit(p_save_as_drawfile_callback->h_dialog, CONTROL_ID_NAME, p_ui_text_filename);
    }
#else
    /* Assume filename already set up correctly, and is the one in p_save_as_drawfile_callback */
    IGNOREPARM_InoutRef_(p_save_as_drawfile_callback);
    IGNOREPARM(p_ui_text_filename);
#endif
}

_Check_return_
static STATUS
save_as_drawfile_dialog_check(
    _InoutRef_  P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback)
{
    IGNOREPARM_InoutRef_(p_save_as_drawfile_callback);

    /* as good a place as any to get state back from UI */
    p_save_as_drawfile_callback->all_or_range = ui_dlg_get_check(p_save_as_drawfile_callback->h_dialog, CONTROL_ID_AR_GROUP);

    ui_text_dispose(&p_save_as_drawfile_callback->range_list);

    ui_dlg_get_edit(p_save_as_drawfile_callback->h_dialog, CONTROL_ID_RANGE_EDIT, &p_save_as_drawfile_callback->range_list);

    return(STATUS_OK);
}

_Check_return_
static STATUS
pagelist_fill_from(
    P_ARRAY_HANDLE p_h_page_list,
    _In_z_      PCTSTR tstr)
{
    STATUS status = STATUS_OK;
    SS_RECOG_CONTEXT ss_recog_context;

    ss_recog_context_push(&ss_recog_context);

    for(;;)
    {
        TCHAR ch;
        PAGE_NUM page_num_1;
        PAGE_NUM page_num_2;
        U32 used;

        /* skip spaces, commas (list separators) and newlines */
        while(((ch = *tstr++) == CH_SPACE) || (ch == g_ss_recog_context.list_sep_char) || (ch == LF))
        { /*EMPTY*/ }
        --tstr;

        if(!ch)
            break;

        if((used = get_pagenum(tstr, &page_num_1)) > 0)
        {
            tstr += used;

            /* skip over any unrecognized debris */
            while(((ch = *tstr++) != CH_SPACE) && (ch != g_ss_recog_context.list_sep_char) && (ch != CH_HYPHEN_MINUS) && (ch != LF) && ch)
            { /*EMPTY*/ }
            --tstr;

            /* skip_spaces */
            while(((ch = *tstr++) == CH_SPACE))
            { /*EMPTY*/ }
            --tstr;

            if(ch != CH_HYPHEN_MINUS)
            {
                if((page_num_1.x > 0) && (page_num_1.y > 0))
                    status = pagelist_add_page(p_h_page_list, page_num_1.x, page_num_1.y);
                else
                    status = pagelist_add_blank(p_h_page_list);
            }
            else
            {
                /* it looks like a range */
                ch = *tstr++;

                if((used = get_pagenum(tstr, &page_num_2)) > 0)
                {
                    tstr += used;

                    if((page_num_1.x > 0) && (page_num_1.y > 0) && (page_num_2.x > 0) && (page_num_2.y > 0))
                        status = pagelist_add_range(p_h_page_list, page_num_1.x, page_num_1.y, page_num_2.x, page_num_2.y);

                    /* skip over any unrecognized debris */
                    while(((ch = *tstr++) != CH_SPACE) && (ch != g_ss_recog_context.list_sep_char) && (ch != LF) && ch)
                    { /*EMPTY*/ }
                    --tstr;
                }
            }
        }

        if(status_fail(status))
        {
            status_assertc(status);
            break;
        }
    }

    ss_recog_context_pull(&ss_recog_context);

    return(status);
}

extern void
save_as_drawfile_percentage_initialise(
    _DocuRef_   P_DOCU p_docu,
    P_PRINTER_PERCENTAGE p_save_as_drawfile_percentage,
    _In_        S32 page_count)
{
    assert(page_count > 0);
    if(page_count <= 0)         /* page_count should be >0, but lets be paranoid about division by 0 */
        page_count = 1;

    zero_struct_ptr(p_save_as_drawfile_percentage);
    p_save_as_drawfile_percentage->process_status.flags.foreground = 1;

    p_save_as_drawfile_percentage->final_page_count   = page_count;
    p_save_as_drawfile_percentage->current_page_count = 0;
    p_save_as_drawfile_percentage->percent_per_page   = 100 / page_count;

    process_status_begin(p_docu, &p_save_as_drawfile_percentage->process_status, PROCESS_STATUS_PERCENT);

    save_as_drawfile_percentage_reflect(p_save_as_drawfile_percentage, 0);
}

extern void
save_as_drawfile_percentage_finalise(
    P_PRINTER_PERCENTAGE p_save_as_drawfile_percentage)
{
    process_status_end(&p_save_as_drawfile_percentage->process_status);
}

extern void
save_as_drawfile_percentage_page_inc(
    P_PRINTER_PERCENTAGE p_save_as_drawfile_percentage)
{
    p_save_as_drawfile_percentage->current_page_count++;

    save_as_drawfile_percentage_reflect(p_save_as_drawfile_percentage, 0);
}

static void
save_as_drawfile_percentage_reflect(
    P_PRINTER_PERCENTAGE p_save_as_drawfile_percentage,
    _InVal_     S32 sub_percentage)
{
    S32 percent = p_save_as_drawfile_percentage->current_page_count * 100 / p_save_as_drawfile_percentage->final_page_count;

    p_save_as_drawfile_percentage->process_status.data.percent.current = percent + sub_percentage;

    process_status_reflect(&p_save_as_drawfile_percentage->process_status);
}

_Check_return_
static STATUS
save_drawfile_save(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback,
    _In_z_      PCTSTR filename)
{
    STATUS status = STATUS_OK;
    S32 last_page_x_read = last_page_x_non_blank(p_docu);
    S32 last_page_y_read = last_page_y_non_blank(p_docu);
    PRINT_CTRL print_ctrl;
    S32 x0 = 1;
    S32 x1 = last_page_x_read;
    S32 y0 = 1;
    S32 y1 = last_page_y_read;

    zero_struct(print_ctrl);

    /* y0,y1 & x0,x1 are used for straight-forward cases, like simple-all, simple-range and extra-all */
    /* initialised to all, trim values later for simple-range */

    print_ctrl.h_page_list = 0;

    assert(y0>0);
    assert(y1>0);
    assert(x0>0);
    assert(x1>0);

    if(p_save_as_drawfile_callback->all_or_range && !ui_text_is_blank(&p_save_as_drawfile_callback->range_list))
    {
        status = pagelist_fill_from(&print_ctrl.h_page_list, ui_text_tstr(&p_save_as_drawfile_callback->range_list));
    }
    else
    {
        status = pagelist_add_range(&print_ctrl.h_page_list, x0, y0, x1, y1);
    }
  /*else                    */
  /*    list already filled */

    print_ctrl.flags.landscape = p_docu->page_def.landscape;

    if(status_ok(status))
        status = save_as_drawfile_host_print_document(p_docu, &print_ctrl, filename);

    al_array_dispose(&print_ctrl.h_page_list);

    return(status);
}

static void
t5_cmd_save_as_drawfile_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback)
{
    zero_struct_ptr(p_save_as_drawfile_callback);

    p_save_as_drawfile_callback->docno = docno_from_p_docu(p_docu);

    /*p_save_as_drawfile_callback->saving = SAVING_OWNFORM;*/
    /*p_save_as_drawfile_callback->p_proc_save = save_drawfile_save;*/

    p_save_as_drawfile_callback->t5_filetype = FILETYPE_DRAW /*which_t5_filetype(p_docu)*/;

    p_save_as_drawfile_callback->rename_after_save = FALSE /*TRUE*/;
    p_save_as_drawfile_callback->clear_modify_after_save = FALSE /*TRUE*/;
}

#if WINDOWS

_Check_return_
static STATUS /*n_filters*/
save_as_drawfile_windows_filter_list_create(
    _InoutRef_  P_QUICK_TBLOCK p_filter_quick_tblock)
{
    /* build filters as description,CH_NULL,wildcard spec,CH_NULL sets */
    STATUS status = STATUS_OK;
    S32 n_filters = 0;

    for(;;) /* loop for structure */
    {
        const T5_FILETYPE t5_filetype = FILETYPE_DRAW;

        BOOL fFound_description, fFound_extension;
        const PC_USTR ustr_description = description_ustr_from_t5_filetype(t5_filetype, &fFound_description);
        const PC_USTR ustr_extension_srch = extension_srch_ustr_from_t5_filetype(t5_filetype, &fFound_extension);

        status_break(status = quick_tblock_ustr_add_n(p_filter_quick_tblock, ustr_description, strlen_with_NULLCH));
        status_break(status = quick_tblock_ustr_add_n(p_filter_quick_tblock, ustr_extension_srch, strlen_with_NULLCH));

        n_filters++;

        break; /* end of loop for structure */
    }

    /* list ends with one final CH_NULL */
    if(status_ok(status))
        status = quick_tblock_nullch_add(p_filter_quick_tblock);

    if(status_fail(status))
    {
        quick_tblock_dispose(p_filter_quick_tblock);
        return(status);
    }

    return(n_filters);
}

/* First, pop up a Save As box with initial_filename, filling in filename
 * Then we can proceed to the business of actually choosing the range etc.
 */

_Check_return_
static STATUS
save_as_drawfile_get_filename(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_AS_DRAWFILE_CALLBACK p_save_as_drawfile_callback)
{
    STATUS status = STATUS_OK;
    TCHARZ szDirName[BUF_MAX_PATHSTRING];
    TCHARZ szFile[BUF_MAX_PATHSTRING];
    TCHARZ szDialogTitle[64];
    OPENFILENAME openfilename;
    BOOL ofnResult;
    QUICK_TBLOCK_WITH_BUFFER(filter_quick_tblock, 32);
    quick_tblock_with_buffer_setup(filter_quick_tblock);

    /* build filters as description,CH_NULL,wildcard spec,CH_NULL sets */
    status_return(save_as_drawfile_windows_filter_list_create(&filter_quick_tblock));

    /* try to find some sensible place to dump these */
    if(!ui_text_is_blank(&p_save_as_drawfile_callback->init_directory))
        tstr_xstrkpy(szDirName, elemof32(szDirName), ui_text_tstr(&p_save_as_drawfile_callback->init_directory));
    else if(0 == MyGetProfileString(TEXT("DefaultDirectory"), tstr_empty_string, szDirName, elemof32(szDirName)))
    {
        if(!GetPersonalDirectoryName(szDirName, elemof32(szDirName)))
        {
            if(GetModuleFileName(GetInstanceHandle(), szDirName, elemof32(szDirName)))
            {
                file_dirname(szDirName, szDirName);
            }
            else
            {
                szDirName[0] = CH_NULL;
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

    tstr_xstrkpy(szFile, elemof32(szFile), ui_text_tstr(&p_save_as_drawfile_callback->init_filename));

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
    if(p_save_as_drawfile_callback->save_selection)
    {
        resource_lookup_tstr_buffer(szDialogTitle, elemof32(szDialogTitle), MSG_DIALOG_SAVE_SELECTION_CAPTION);
        openfilename.lpstrTitle = szDialogTitle;
    }
    openfilename.Flags =
        OFN_EXPLORER            |
        OFN_NOCHANGEDIR         |
        OFN_PATHMUSTEXIST       |
        OFN_HIDEREADONLY        |
        OFN_OVERWRITEPROMPT     |
        OFN_ENABLESIZING        ;
    openfilename.nFileOffset = 0;
    openfilename.nFileExtension = 0;
    openfilename.lpstrDefExt = TEXT("aff"); /* without the preceding dot */
    openfilename.lCustData = 0;
    openfilename.lpTemplateName = NULL;

    /* SaveFileHook not needed */

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

    status = ui_text_alloc_from_tstr(&p_save_as_drawfile_callback->filename, openfilename.lpstrFile);

    return(status);
}

#endif /* WINDOWS */

/******************************************************************************
*
* save as drawfile intro
*
******************************************************************************/

T5_CMD_PROTO(static, t5_cmd_save_as_drawfile_intro)
{
    STATUS status = STATUS_OK;
    S32 last_page_y_read = last_page_y_non_blank(p_docu);
    BIT_NUMBER bit_number;
    SAVE_AS_DRAWFILE_CALLBACK save_as_drawfile_callback;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
    quick_tblock_with_buffer_setup(quick_tblock);

    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    /* encode initial state of control(s) */

    t5_cmd_save_as_drawfile_init(p_docu, &save_as_drawfile_callback);

#if RISCOS
    status_assert(resource_lookup_quick_tblock(&quick_tblock, MSG_DRAW_IO_DRAWFILE));
    quick_tblock_nullch_add(&quick_tblock);
#else
    status_assert(name_make_wholename(&p_docu->docu_name, &quick_tblock, FALSE));
    quick_tblock_nullch_strip(&quick_tblock);
    status_assert(quick_tblock_tstr_add_n(&quick_tblock, DRAWFILE_EXTENSION_TSTR, strlen_with_NULLCH));
#endif /* OS */

    save_as_drawfile_callback.init_filename.type = UI_TEXT_TYPE_TSTR_TEMP;
    save_as_drawfile_callback.init_filename.text.tstr = quick_tblock_tstr(&quick_tblock);

    {
    TCHARZ directory[BUF_MAX_PATHSTRING];
    if(!file_dirname(directory, ui_text_tstr(&save_as_drawfile_callback.init_filename)))
        save_as_drawfile_callback.init_directory.type = UI_TEXT_TYPE_NONE;
    else
        status_assert(ui_text_alloc_from_tstr(&save_as_drawfile_callback.init_directory, directory));
    } /*block*/

#if WINDOWS
    if(p_docu->mark_info_cells.h_markers) /* always do save selection */
    {
        save_as_drawfile_callback.save_selection = TRUE;

        quick_tblock_dispose(&quick_tblock);

        status_assert(resource_lookup_quick_tblock(&quick_tblock, MSG_SELECTION));
        status_assert(quick_tblock_tstr_add_n(&quick_tblock, TEXT(".aff"), strlen_with_NULLCH));

        save_as_drawfile_callback.init_filename.type = UI_TEXT_TYPE_TSTR_TEMP;
        save_as_drawfile_callback.init_filename.text.tstr = quick_tblock_tstr(&quick_tblock);
    }
#endif

    save_as_drawfile_callback.all_or_range = 0; /* will set full ranges below */

    save_as_drawfile_range_y0 = 1;
    save_as_drawfile_range_y1 = last_page_y_read;

#if RISCOS
    save_as_drawfile_pict.relative_offset[0] = -SAVE_AS_DRAWFILE_PICT_OFFSET_H;
#endif

#if RISCOS
    bitmap_set(save_as_drawfile_name_validation, N_BITS_ARG(256)); /* allow all characters bar those below in filenames */

    for(bit_number = 0; bit_number <= 31; ++bit_number)
        bitmap_bit_clear(save_as_drawfile_name_validation, bit_number, N_BITS_ARG(256));

    bitmap_bit_clear(save_as_drawfile_name_validation, CH_SPACE, N_BITS_ARG(256));

    bitmap_bit_clear(save_as_drawfile_name_validation, CH_QUOTATION_MARK, N_BITS_ARG(256));
    bitmap_bit_clear(save_as_drawfile_name_validation, CH_VERTICAL_LINE, N_BITS_ARG(256));
    bitmap_bit_clear(save_as_drawfile_name_validation, CH_DELETE, N_BITS_ARG(256));
#endif

#ifdef AR_GROUP
    bitmap_clear(save_as_drawfile_range_edit_validation, N_BITS_ARG(256));

    for(bit_number = CH_DIGIT_ZERO; bit_number <= CH_DIGIT_NINE; bit_number++)
        bitmap_bit_set(save_as_drawfile_range_edit_validation, bit_number, N_BITS_ARG(256));

    {
    SS_RECOG_CONTEXT ss_recog_context;
    ss_recog_context_push(&ss_recog_context);
    bitmap_bit_set(save_as_drawfile_range_edit_validation, g_ss_recog_context.list_sep_char, N_BITS_ARG(256));
    ss_recog_context_pull(&ss_recog_context);
    } /*block*/
    bitmap_bit_set(save_as_drawfile_range_edit_validation, CH_SPACE, N_BITS_ARG(256));
    bitmap_bit_set(save_as_drawfile_range_edit_validation, CH_HYPHEN_MINUS, N_BITS_ARG(256));
    bitmap_bit_set(save_as_drawfile_range_edit_validation, CH_FULL_STOP, N_BITS_ARG(256)); /* <<< not a decimal point */
    bitmap_bit_set(save_as_drawfile_range_edit_validation, '\n', N_BITS_ARG(256));
    bitmap_bit_set(save_as_drawfile_range_edit_validation, 'B', N_BITS_ARG(256));
    bitmap_bit_set(save_as_drawfile_range_edit_validation, 'b', N_BITS_ARG(256));
#endif

    save_as_drawfile_callback.max_page_y = last_page_y_read;

#if WINDOWS
    /* Get the filename to use in a separate step before the all/range dialog */
    status = save_as_drawfile_get_filename(p_docu, &save_as_drawfile_callback);
#endif

    if(status_ok(status))
    {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, save_as_drawfile_dialog_create, elemof32(save_as_drawfile_dialog_create), MSG_DIALOG_SAVE_AS_DRAWFILE_INTRO_HELP_TOPIC);
        /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
        dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_SAVE_AS_DRAWFILE_INTRO_CAPTION;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_save_as_drawfile;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &save_as_drawfile_callback;
        status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        ui_text_dispose(&dialog_cmd_process_dbox.caption);
    }

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    quick_tblock_dispose(&quick_tblock);

    ui_text_dispose(&save_as_drawfile_callback.init_directory);
    ui_text_dispose(&save_as_drawfile_callback.init_filename);
    ui_text_dispose(&save_as_drawfile_callback.filename);

    return(status);
}

T5_CMD_PROTO(static, t5_cmd_save_as_drawfile)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, ARG_SAVE_AS_DRAWFILE_N_ARGS);
    SAVE_AS_DRAWFILE_CALLBACK save_as_drawfile_callback;
    PCTSTR filename;

    IGNOREPARM_InVal_(t5_message);

    t5_cmd_save_as_drawfile_init(p_docu, &save_as_drawfile_callback);

    assert(p_args[ARG_SAVE_AS_DRAWFILE_FILENAME].type == (ARG_TYPE_TSTR | ARG_MANDATORY));
    filename = p_args[ARG_SAVE_AS_DRAWFILE_FILENAME].val.tstr;

    save_as_drawfile_callback.all_or_range = 0;

    if(arg_is_present(p_args, ARG_SAVE_AS_DRAWFILE_RANGE) && p_args[ARG_SAVE_AS_DRAWFILE_RANGE].val.s32)           /* 0=all/1=range */
    {
        /* page range specified */
        if(arg_is_present(p_args, ARG_SAVE_AS_DRAWFILE_RANGE_LIST))
        {
            save_as_drawfile_callback.all_or_range = 1;

            status_return(ui_text_alloc_from_tstr(&save_as_drawfile_callback.range_list, p_args[ARG_SAVE_AS_DRAWFILE_RANGE_LIST].val.tstr));
        }
    }

    return(proc_save_as_drawfile(filename, (CLIENT_HANDLE) &save_as_drawfile_callback));
}

/******************************************************************************
*
* draw_io converter object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, draw_io_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(resource_init(OBJECT_ID_DRAW_IO, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_DRAW_IO));

        return(register_object_construct_table(OBJECT_ID_DRAW_IO, object_construct_table, FALSE /* no inlines */));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_DRAW_IO));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_draw_io);
OBJECT_PROTO(extern, object_draw_io)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(draw_io_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_CMD_SAVE_AS_DRAWFILE_INTRO:
        return(t5_cmd_save_as_drawfile_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SAVE_AS_DRAWFILE:
        return(t5_cmd_save_as_drawfile(p_docu, t5_message, (PC_T5_CMD) p_data));

#if WINDOWS && defined(T5_CMD_SAVE_AS_METAFILE)
    case T5_CMD_SAVE_AS_METAFILE_INTRO:
        return(t5_cmd_save_as_drawfile_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SAVE_AS_METAFILE:
        return(t5_cmd_save_as_drawfile(p_docu, t5_message, (PC_T5_CMD) p_data));
#endif /* T5_CMD_SAVE_AS_METAFILE */

    default:
        return(STATUS_OK);
    }
}

/* end of ob_drwio.c */
