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

#include "Build/windows/resource_x_save.h"
#endif

#include "ob_file/xp_file.h"

/*
internal structures
*/

typedef struct SAVE_CALLBACK * P_SAVE_CALLBACK;

typedef STATUS (* P_PROC_SAVE) (
    _InoutRef_  P_SAVE_CALLBACK p_save_callback);

#define PROC_SAVE_PROTO(_proc_name) \
_Check_return_ \
static STATUS \
_proc_name( \
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)

typedef enum SAVE_CALLBACK_SAVING
{
    SAVING__UNINIT = 0U,
    SAVING_DOCUMENT,
    SAVING_HYBRID_DRAW,
    SAVING_AS_TEMPLATE,
    SAVING_AS_FOREIGN,
    SAVING_AS_DRAW,
    SAVING_PICTURE,
    LOCATING_TEMPLATE
}
SAVE_CALLBACK_SAVING;

typedef struct SAVE_CALLBACK
{
    /* members needed for save processing */
    DOCNO           docno;
    U8              _spare_u8[3];

    SAVE_CALLBACK_SAVING saving;

    UI_TEXT         saving_filename;
    T5_FILETYPE     saving_t5_filetype;

    BOOL            allow_not_rooted;
    BOOL            clear_modify_after_save;
    BOOL            rename_after_save;
    BOOL            save_selection;
    BOOL            test_for_saved_file_is_safe; /* really RISCOS_ONLY() but clearer code if left here */

    /* for SAVING_HYBRID_DRAW and SAVING_DRAW */
    BOOL            page_range_list_is_set;
    UI_TEXT         page_range_list;

    /* for SAVING_TEMPLATE */
#define SAVING_TEMPLATE_ALL        0
#define SAVING_TEMPLATE_ALL_STYLES 1
#define SAVING_TEMPLATE_ONE_STYLE  2
    S32             style_radio;
    STYLE_HANDLE    style_handle;       /* iff style_radio == SAVING_TEMPLATE_ONE_STYLE */

    /* for SAVING_PICTURE */
    OBJECT_ID       picture_object_id;
    P_ANY           picture_object_data_ref;

    /* additional members needed for save intro dialogue box processing */
    H_DIALOG        h_dialog; /* only valid after PUSHBUTTON, USER_MOUSE, USER_REDRAW events */

#if WINDOWS
    HWND            hdlg; /* valid after CDN_INITDONE in SaveFileHook */ /* handle to child dialog window NOT the 'Save As' window!!! */
#endif

#if WINDOWS
    UI_TEXT         initial_directory;
#endif
    UI_TEXT         initial_filename;

    S32             self_abuse;
    STATUS          filename_edited;

    ARRAY_HANDLE    h_save_filetype; /* SAVE_FILETYPE[] */
#if WINDOWS
    ARRAY_HANDLE    h_save_filetype_filters; /* SAVE_FILETYPE[] */
#endif

    PCTSTR          initial_template_dir;

    UI_TEXT         ui_text;
}
SAVE_CALLBACK; typedef const struct SAVE_CALLBACK * PC_SAVE_CALLBACK;

static inline void
save_callback_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback,
    _InVal_     T5_FILETYPE t5_filetype)
{
    zero_struct_ptr_fn(p_save_callback);

    p_save_callback->docno = docno_from_p_docu(p_docu);

    p_save_callback->saving_t5_filetype = t5_filetype;
}

/*
internal routines
*/

PROC_SAVE_PROTO(do_save_drawfile);
PROC_SAVE_PROTO(do_save_foreign);
PROC_SAVE_PROTO(do_save_ownform);
PROC_SAVE_PROTO(do_save_picture);
PROC_SAVE_PROTO(do_save_template);
PROC_SAVE_PROTO(save_template_locate);

_Check_return_
static STATUS
cmd_save_as_intro(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_FILETYPE t5_filetype); /* may be FILETYPE_UNDETERMINED */

_Check_return_
static STATUS
cmd_save_as_drawfile_intro(
    _DocuRef_   P_DOCU p_docu,
    _In_opt_z_  PCTSTR suggested_leafname);

_Check_return_
static STATUS
cmd_save_picture_intro(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
static STATUS
dialog_save_common_check(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback);

static void
dialog_save_common_details_update(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback);

_Check_return_
static T5_FILETYPE
filemap_lookup_filetype_from_description(
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
    _InoutRef_  P_SAVE_CALLBACK p_save_callback);

_Check_return_
static STATUS
windows_save_hybrid_draw_get_filename(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback);

#endif /* OS */

#if RISCOS || !defined(WINDOWS_SAVE_AS)
static
BITMAP(save_name_validation, 256);
#endif

static struct SAVE_AS_TEMPLATE_STATICS
{
    UI_SOURCE ui_source; /* for styles */
}
save_as_template_intro_statics;

static struct SAVE_AS_FOREIGN_STATICS
{
    T5_FILETYPE t5_filetype;
    S32 itemno;
    UI_SOURCE ui_source; /* for filetype */
}
save_as_foreign_intro_statics = { FILETYPE_ASCII };

static struct SAVE_PICTURE_STATICS
{
    T5_FILETYPE t5_filetype;
    S32 itemno;
    UI_SOURCE ui_source; /* for filetype */
}
save_picture_intro_statics = { FILETYPE_DRAW };

/******************************************************************************
*
* proc_save_common and its helpers
*
******************************************************************************/

static void
proc_save_common_complete_dbox(
    _InRef_     H_DIALOG h_dialog)
{
    DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
    msgclr(dialog_cmd_complete_dbox);
    dialog_cmd_complete_dbox.h_dialog = h_dialog;
    dialog_cmd_complete_dbox.completion_code = DIALOG_COMPLETION_OK;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox));
}

_Check_return_
static STATUS
proc_save_common_ensure_templates_dir(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _InoutRef_  P_BOOL p_report_save_name)
{
    STATUS status;

#if RISCOS
    status = quick_tblock_tstr_add_n(p_quick_tblock, TEXT("<Choices$Write>") FILE_DIR_SEP_TSTR TEXT("Fireworkz"), strlen_with_NULLCH);

    if(status_ok(status))
        if(status_fail(status = file_create_directory(quick_tblock_tstr(p_quick_tblock))))
            *p_report_save_name = TRUE;

    /* NB Template, rather than Templates. Kept for compatibility with previously stored ones */
    if(status_ok(status))
    {
        quick_tblock_nullch_strip(p_quick_tblock);

        status = quick_tblock_tstr_add_n(p_quick_tblock, FILE_DIR_SEP_TSTR TEXT("Template"), strlen_with_NULLCH);
    }
#else
    /* prefix using first element from path and appropriate subdir */
    TCHARZ save_name[BUF_MAX_PATHSTRING];

    file_get_prefix(save_name, elemof32(save_name), NULL);

    status = quick_tblock_tstr_add(p_quick_tblock, save_name);

    if(status_ok(status))
        status = quick_tblock_tstr_add_n(p_quick_tblock, TEMPLATES_SUBDIR, strlen_with_NULLCH);

#endif /* OS */

    if(status_ok(status))
        if(status_fail(status = file_create_directory(quick_tblock_tstr(p_quick_tblock))))
            *p_report_save_name = TRUE;

    if(status_ok(status))
        quick_tblock_nullch_strip(p_quick_tblock);

    return(status);
}

_Check_return_
static STATUS
proc_save_common_set_saving_filename_in_templates_dir(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_z_      PCTSTR filename /*low lifetime*/)
{
    BOOL report_save_name = FALSE;
    STATUS status = proc_save_common_ensure_templates_dir(p_quick_tblock, &report_save_name);

    /* append the filename that we have been given */
    if(status_ok(status)) status = quick_tblock_tstr_add(p_quick_tblock, FILE_DIR_SEP_TSTR);
    if(status_ok(status)) status = quick_tblock_tstr_add_n(p_quick_tblock, filename, strlen_with_NULLCH);

    if(status_fail(status))
    {
        if(report_save_name)
            return(reperr(status, quick_tblock_tstr(p_quick_tblock)));

        return(reperr_null(status));
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
proc_save_common_set_saving_filename(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_z_      PCTSTR filename /*low lifetime*/)
{
    if( p_save_callback->allow_not_rooted && !file_is_rooted(filename) )
    {   /* find a more appropriate name for un-rooted filename - bung it in user's templates directory */
        status_return(proc_save_common_set_saving_filename_in_templates_dir(p_quick_tblock, filename));
    }
    else
    {
        status_return(quick_tblock_tstr_add_n(p_quick_tblock, filename, strlen_with_NULLCH));
    }

    ui_text_dispose(&p_save_callback->saving_filename);
    p_save_callback->saving_filename.type = UI_TEXT_TYPE_TSTR_TEMP;
    p_save_callback->saving_filename.text.tstr = quick_tblock_tstr(p_quick_tblock);

    return(STATUS_OK);
}

_Check_return_
static STATUS
proc_save_common_do_save(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    /* call the appropriate function for this particular save */
    switch(p_save_callback->saving)
    {
    case SAVING__UNINIT:
        return(status_check());

    case SAVING_AS_TEMPLATE:
        return(do_save_template(p_save_callback));

    case SAVING_AS_FOREIGN:
        return(do_save_foreign(p_save_callback));

    case SAVING_AS_DRAW:
        return(do_save_drawfile(p_save_callback));

    case SAVING_PICTURE:
        return(do_save_picture(p_save_callback));

    case LOCATING_TEMPLATE:
        return(save_template_locate(p_save_callback));

    default: default_unhandled();
#if CHECKING
    case SAVING_DOCUMENT:
    case SAVING_HYBRID_DRAW:
#endif
        return(do_save_ownform(p_save_callback));
    }
}

/* NB don't fiddle with parameter list or SAL
 *
 * This is called back on RISC OS from host_xfer_save_file()
 * where filename comes from the wimp message block
 * but also from core of main non-interactive save commands
 *
 * filename and t5_filetype passed here override anything
 * currently in SAVE_CALLBACK saving_xxx members
 * (those are updated here for consistency)
 */

/*ncr*/
static BOOL
proc_save_common(
    _In_z_      PCTSTR filename /*low lifetime*/,
    _InVal_     T5_FILETYPE t5_filetype,
    _InVal_     CLIENT_HANDLE client_handle)
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) client_handle;
    STATUS status;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, BUF_MAX_PATHSTRING);
    quick_tblock_with_buffer_setup(quick_tblock);

    reportf(TEXT("proc_save_common(%u:%s, 0x%03X)"), tstrlen32(filename), filename, t5_filetype);

    p_save_callback->saving_t5_filetype = t5_filetype;

    if(status_fail(status = proc_save_common_set_saving_filename(p_save_callback, &quick_tblock, filename)))
    {
        /* without closing dialog */
        quick_tblock_dispose(&quick_tblock);
        return(TRUE);
    }

    if(status_fail(status = proc_save_common_do_save(p_save_callback)))
    {
        status_consume(reperr(status, quick_tblock_tstr(&quick_tblock)));

        /* without closing dialog */
        quick_tblock_dispose(&quick_tblock);
        return(TRUE);
    }

    if(p_save_callback->h_dialog)
        proc_save_common_complete_dbox(p_save_callback->h_dialog);

    quick_tblock_dispose(&quick_tblock);

    return(TRUE);
}

/******************************************************************************
*
* SAVE: save document in native format (including as Hybrid Draw)
*
******************************************************************************/

/*
SAVE - SAVE
*/

static void
do_save_ownform_init(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _OutRef_    P_DOCU_AREA p_docu_area)
{
    if(p_save_callback->save_selection)
    {
        if(0 == p_docu->mark_info_cells.h_markers)
            p_save_callback->save_selection = FALSE;
    }

    docu_area_init(p_docu_area);

    if(p_save_callback->save_selection)
    {
        docu_area_from_markers_first(p_docu, p_docu_area);

        p_save_callback->rename_after_save = FALSE;
        p_save_callback->clear_modify_after_save = FALSE;
    }

    if(SAVING_HYBRID_DRAW == p_save_callback->saving)
    {
        if(p_save_callback->page_range_list_is_set)
            status_assert(tstr_set(&p_docu->tstr_hybrid_draw_page_range, ui_text_tstr(&p_save_callback->page_range_list)));
        /* tstr_hybrid_draw_page_range may still be valid otherwise */
    }

    p_of_op_format->process_status.flags.foreground = 1;

    * (P_U32) &p_of_op_format->of_template = 0x00FFFFFFU;

    if(p_save_callback->save_selection)
        p_of_op_format->of_template.data_class = DATA_SAVE_DOC; /* SKS 05jan95 attempt to make saved selections directly reloadable */
}

static void
do_save_ownform_finalise(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    if( !p_save_callback->test_for_saved_file_is_safe || host_xfer_saved_file_is_safe() )
    {
        /* file has been saved to a 'safe' place */
        if(p_save_callback->clear_modify_after_save || p_save_callback->rename_after_save)
            docu_modify_clear(p_docu);

        if(p_save_callback->rename_after_save)
        {
            /* rename this document */
            status_assert(rename_document_as_filename(p_docu, ui_text_tstr(&p_save_callback->saving_filename)));

            p_docu->docu_preferred_filetype = p_save_callback->saving_t5_filetype;

            p_docu->file_ss_date = p_of_op_format->output.u.file.ss_date;

            if(p_docu->flags.read_only)
            {
                p_docu->flags.read_only = FALSE;
                status_assert(maeve_event(p_docu, T5_MSG_DOCU_READWRITE, P_DATA_NONE));
            }
        }
    }
}

PROC_SAVE_PROTO(do_save_ownform)
{
    /*poked*/ P_DOCU p_docu = p_docu_from_docno(p_save_callback->docno);
    OF_OP_FORMAT of_op_format = OF_OP_FORMAT_INIT;
    STATUS status;
    DOCU_AREA save_docu_area;

    do_save_ownform_init(p_docu, p_save_callback, &of_op_format, &save_docu_area);

    status_return(ownform_initialise_save(p_docu, &of_op_format, NULL,
                                          ui_text_tstr(&p_save_callback->saving_filename), p_save_callback->saving_t5_filetype,
                                          p_save_callback->save_selection ? &save_docu_area : NULL));

    status = save_ownform_file(p_docu, &of_op_format);

    status = ownform_finalise_save(&p_docu, &of_op_format, status);

    if(status_ok(status))
        do_save_ownform_finalise(p_docu, p_save_callback, &of_op_format);

    return(status);
}

/*
SAVE - CMD
*/

static void
cmd_save_init_set_saving(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    p_save_callback->saving = (FILETYPE_T5_HYBRID_DRAW == p_save_callback->saving_t5_filetype) ? SAVING_HYBRID_DRAW : SAVING_DOCUMENT;

    p_save_callback->clear_modify_after_save = !p_save_callback->save_selection;
}

_Check_return_
static STATUS
cmd_save_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock)
{
    save_callback_init(p_docu, p_save_callback, which_t5_filetype(p_docu));

    cmd_save_init_set_saving(p_save_callback);

    status_return(name_make_wholename(&p_docu->docu_name, p_quick_tblock, TRUE));

    p_save_callback->saving_filename.type = UI_TEXT_TYPE_TSTR_PERM;
    p_save_callback->saving_filename.text.tstr = quick_tblock_tstr(p_quick_tblock);

    /*p_save_callback->rename_after_save = FALSE; zero already */

    return(STATUS_OK);
}

static inline void
cmd_save_finalise(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock)
{
    quick_tblock_dispose(p_quick_tblock);
}

_Check_return_
static STATUS
cmd_save(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status;
    SAVE_CALLBACK save_callback;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, BUF_MAX_PATHSTRING);
    quick_tblock_with_buffer_setup(quick_tblock);

    status_return(cmd_save_init(p_docu, &save_callback, &quick_tblock));

    status = proc_save_common(quick_tblock_tstr(&quick_tblock), save_callback.saving_t5_filetype, (CLIENT_HANDLE) &save_callback);

    cmd_save_finalise(&quick_tblock);

    return(status);
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    if(object_present(OBJECT_ID_NOTE))
    {   /* note(s) selected? process differently */
        NOTELAYER_SELECTION_INFO notelayer_selection_info;
        if(status_ok(object_call_id(OBJECT_ID_NOTE, p_docu, T5_MSG_NOTELAYER_SELECTION_INFO, &notelayer_selection_info)))
            return(cmd_save_picture_intro(p_docu));
    }

    /* see if the document has somewhere to be saved to */
    /* and whether there is a selection (avoids silly overwrite) */
    if( (NULL == p_docu->docu_name.path_name) || (0 != p_docu->mark_info_cells.h_markers) )
        return(cmd_save_as_intro(p_docu, FILETYPE_UNDETERMINED)); /* suss it all there */

    return(cmd_save(p_docu));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save)

/*
SAVE AS - CMD
*/

static void
cmd_save_as_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback,
    _InVal_     T5_FILETYPE t5_filetype) /* may be FILETYPE_UNDETERMINED */
{
    save_callback_init(p_docu, p_save_callback, (FILETYPE_UNDETERMINED != t5_filetype) ? t5_filetype : which_t5_filetype(p_docu));

    cmd_save_init_set_saving(p_save_callback);

    p_save_callback->rename_after_save = TRUE;
}

_Check_return_
static STATUS
cmd_save_as(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    _InVal_     BOOL save_selection)
{
    SAVE_CALLBACK save_callback;

    cmd_save_as_init(p_docu, &save_callback, t5_filetype);

    save_callback.save_selection = save_selection;

    return(proc_save_common(filename, t5_filetype, (CLIENT_HANDLE) &save_callback));
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_as)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 3);
    T5_FILETYPE t5_filetype = FILETYPE_UNDETERMINED;
    BOOL save_selection = false;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(arg_is_present(p_args, 1))
    {   /* Can force to use any of Fireworkz family filetypes */
        assert(ARG_TYPE_X32 == p_args[1].type);
        t5_filetype = p_args[1].val.t5_filetype;
    }

    if(arg_is_present(p_args, 2))
    {
        assert(ARG_TYPE_BOOL == p_args[2].type);
        save_selection = p_args[2].val.fBool;
    }

    assert((ARG_TYPE_TSTR | ARG_MANDATORY) == p_args[0].type);
    return(cmd_save_as(p_docu, p_args[0].val.tstr, t5_filetype, save_selection));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_as)

/******************************************************************************
*
* SAVE AS TEMPLATE
*
******************************************************************************/

/*
SAVE AS TEMPLATE - SAVE
*/

_Check_return_
static inline STATUS
do_save_template_init(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    * (P_U32) &p_of_op_format->of_template = 0x00FFFFFFU;

    return(ownform_initialise_save(p_docu_from_docno(p_save_callback->docno), p_of_op_format, NULL,
                                   ui_text_tstr(&p_save_callback->saving_filename), p_save_callback->saving_t5_filetype, NULL));
}

_Check_return_
static inline STATUS
do_save_template_save_all_styles(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STYLE_HANDLE style_handle = STYLE_HANDLE_ENUM_START;
    P_STYLE p_style;

    while(style_enum_styles(p_docu, &p_style, &style_handle) >= 0)
    {
        status_return(skeleton_save_style_handle(p_of_op_format, style_handle, TRUE));
    }

    return(STATUS_OK);
}

_Check_return_
static inline STATUS
do_save_template_save(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    switch(p_save_callback->style_radio)
    {
    default:
        return(save_ownform_file(p_docu_from_docno(p_save_callback->docno), p_of_op_format));

    case 1:
        return(do_save_template_save_all_styles(p_docu_from_docno(p_save_callback->docno), p_of_op_format));

    case 2:
        status_assertc(p_save_callback->style_handle);
        return(skeleton_save_style_handle(p_of_op_format, p_save_callback->style_handle, TRUE));
    }
}

PROC_SAVE_PROTO(do_save_template)
{
    OF_OP_FORMAT of_op_format = OF_OP_FORMAT_INIT;
    STATUS status;

    status_return(do_save_template_init(p_save_callback, &of_op_format));

    status = do_save_template_save(p_save_callback, &of_op_format);

    return(ownform_finalise_save(NULL, &of_op_format, status));
}

/*
SAVE AS TEMPLATE - CMD
*/

static void
cmd_save_as_template_init_set_saving(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    p_save_callback->saving = SAVING_AS_TEMPLATE;

    p_save_callback->save_selection = FALSE;

    p_save_callback->clear_modify_after_save = FALSE;
    p_save_callback->rename_after_save = FALSE;
}

static void
cmd_save_as_template_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback,
    _InVal_     S32 style_radio)
{
    save_callback_init(p_docu, p_save_callback, FILETYPE_T5_TEMPLATE);

    cmd_save_as_template_init_set_saving(p_save_callback);

    p_save_callback->style_radio = style_radio;
    p_save_callback->style_handle = STATUS_FAIL;
}

_Check_return_
static STATUS
cmd_save_as_template(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR filename,
    _InVal_     S32 style_radio,
    _In_opt_z_  PCTSTR tstr_style_name)
{
    SAVE_CALLBACK save_callback;

    cmd_save_as_template_init(p_docu, &save_callback, style_radio);

    if(SAVING_TEMPLATE_ONE_STYLE == save_callback.style_radio)
        if(0 == (save_callback.style_handle = style_handle_from_name(p_docu, tstr_style_name)))
            return(create_error(ERR_NAMED_STYLE_NOT_FOUND));

    return(proc_save_common(filename, FILETYPE_T5_TEMPLATE, (CLIENT_HANDLE) &save_callback));
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_as_template)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 3);
    PCTSTR tstr_style_name = NULL;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(arg_is_present(p_args, 2))
    {
        assert(ARG_TYPE_TSTR == p_args[2].type);
        tstr_style_name = p_args[2].val.tstr;
    }

    assert((ARG_TYPE_TSTR | ARG_MANDATORY) == p_args[0].type);
    assert((ARG_TYPE_S32 | ARG_MANDATORY) == p_args[1].type);
    return(cmd_save_as_template(p_docu, p_args[0].val.tstr, p_args[1].val.s32, tstr_style_name));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_as_template)

/******************************************************************************
*
* SAVE AS FOREIGN
*
******************************************************************************/

/*
SAVE AS FOREIGN - FILEMAP
*/

/*ncr*/
static S32 /*itemno*/
filemap_locate_within_h_save_filetype(
    _InRef_     PC_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_T5_FILETYPE p_t5_filetype)
{
    S32 itemno = DIALOG_CTL_STATE_LIST_ITEM_NONE;
    BOOL had_default = FALSE;
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_save_callback->h_save_filetype); ++i)
    {
        const PC_SAVE_FILETYPE p_save_filetype = array_ptrc(&p_save_callback->h_save_filetype, SAVE_FILETYPE, i);

        if(p_save_filetype->t5_filetype == *p_t5_filetype) /* same as what we have? */
        {
            had_default = TRUE;
            itemno = (S32) i;
            break;
        }
    }

    if( !had_default && (0 != array_elements(&p_save_callback->h_save_filetype)) )
    {
        /* be content with the first thing we find */
        const PC_SAVE_FILETYPE p_save_filetype = array_basec(&p_save_callback->h_save_filetype, SAVE_FILETYPE);

        *p_t5_filetype = p_save_filetype->t5_filetype;

        itemno = 0;
    }

    return(itemno);
}

_Check_return_
static OBJECT_ID
filemap_lookup_object_id_from_t5_filetype(
    _InRef_     PC_SAVE_CALLBACK p_save_callback,
    _InVal_     T5_FILETYPE t5_filetype)
{
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_save_callback->h_save_filetype); ++i)
    {
        const PC_SAVE_FILETYPE p_save_filetype = array_ptrc(&p_save_callback->h_save_filetype, SAVE_FILETYPE, i);

        if(p_save_filetype->t5_filetype == t5_filetype)
            return(p_save_filetype->object_id);
    }

    return(OBJECT_ID_NONE);
}

/******************************************************************************
*
* create a list of filetypes from the loaded objects
*
******************************************************************************/

_Check_return_
static inline STATUS
filemap_add_h_save_filetype_for_cmd_save_as_foreign(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InVal_     ARRAY_INDEX i)
{
    const PC_INSTALLED_SAVE_OBJECT p_installed_save_object = array_ptrc(&g_installed_save_objects_handle, INSTALLED_SAVE_OBJECT, i);
    BOOL fFound_description;
    const PC_USTR ustr_description = description_ustr_from_t5_filetype(p_installed_save_object->t5_filetype, &fFound_description);
    SAVE_FILETYPE save_filetype; zero_struct_fn(save_filetype);
    save_filetype.object_id = p_installed_save_object->object_id;
    save_filetype.t5_filetype  = p_installed_save_object->t5_filetype;
    save_filetype.description.type = UI_TEXT_TYPE_USTR_PERM;
    if(fFound_description)
        save_filetype.description.text.ustr = ustr_description;
    else
    {
        const P_DOCU p_docu = p_docu_from_docno(p_save_callback->docno); 
        status_return(alloc_block_ustr_set(&save_filetype.description.text.ustr_wr, ustr_description, &p_docu->general_string_alloc_block));
    }
    return(al_array_add(&p_save_callback->h_save_filetype, SAVE_FILETYPE, 1, PC_ARRAY_INIT_BLOCK_NONE, &save_filetype));
}

_Check_return_
static STATUS
filemap_create_h_save_filetype_for_cmd_save_as_foreign(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    STATUS status;
    ARRAY_INDEX i;

    /* exporting objects will each append a type to this handle */
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(SAVE_FILETYPE), 0);
    status_return(status = al_array_preallocate_zero(&p_save_callback->h_save_filetype, &array_init_block));

    for(i = 0; i < array_elements(&g_installed_save_objects_handle); ++i)
        status_break(status = filemap_add_h_save_filetype_for_cmd_save_as_foreign(p_save_callback, i));

    if(status_fail(status))
        al_array_dispose(&p_save_callback->h_save_filetype);

    return(status);
}

/*
SAVE AS FOREIGN - SAVE
*/

_Check_return_
static inline STATUS
do_save_foreign_init(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _OutRef_    P_OBJECT_ID p_object_id)
{
    const P_DOCU p_docu = p_docu_from_docno(p_save_callback->docno);
    DOCU_AREA save_docu_area;

    if(OBJECT_ID_NONE == (*p_object_id = filemap_lookup_object_id_from_t5_filetype(p_save_callback, p_save_callback->saving_t5_filetype)))
        return(status_check());

    status_return(object_load(*p_object_id));

    if(p_save_callback->save_selection)
    {
        if(!p_docu->mark_info_cells.h_markers)
            p_save_callback->save_selection = FALSE;
    }

    docu_area_init(&save_docu_area);

    if(p_save_callback->save_selection)
    {
        docu_area_from_markers_first(p_docu, &save_docu_area);
    }

    status_return(foreign_initialise_save(p_docu, p_ff_op_format, NULL,
                                          ui_text_tstr(&p_save_callback->saving_filename), p_save_callback->saving_t5_filetype,
                                          p_save_callback->save_selection ? &save_docu_area : NULL));

    p_ff_op_format->of_op_format.of_template.data_class = DATA_SAVE_WHOLE_DOC;

    return(STATUS_OK);
}

_Check_return_
static inline STATUS
do_save_foreign_save(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     OBJECT_ID object_id)
{
    MSG_SAVE_FOREIGN msg_save_foreign;
    msg_save_foreign.t5_filetype = p_save_callback->saving_t5_filetype;
    msg_save_foreign.p_ff_op_format = p_ff_op_format;
    return(object_call_id_load(p_docu_from_docno(p_save_callback->docno), T5_MSG_SAVE_FOREIGN, &msg_save_foreign, object_id));
}

PROC_SAVE_PROTO(do_save_foreign)
{
    FF_OP_FORMAT ff_op_format = { OP_OUTPUT_INVALID };
    OBJECT_ID object_id;
    STATUS status;

    status_return(do_save_foreign_init(p_save_callback, &ff_op_format, &object_id));

    status = do_save_foreign_save(p_save_callback, &ff_op_format, object_id);

    return(foreign_finalise_save(NULL, &ff_op_format, status));
}

/*
SAVE AS FOREIGN - CMD
*/

_Check_return_
static STATUS
cmd_save_as_foreign_init_set_saving(
    _OutRef_    P_SAVE_CALLBACK p_save_callback)
{
    /* mostly leave p_save_callback->save_selection alone */

    p_save_callback->clear_modify_after_save = FALSE;
    p_save_callback->rename_after_save = FALSE;

    switch(p_save_callback->saving_t5_filetype)
    {
    case FILETYPE_DRAW:
        p_save_callback->saving = SAVING_AS_DRAW;

        p_save_callback->save_selection = FALSE;
        return(STATUS_OK);

    default:
        p_save_callback->saving = SAVING_AS_FOREIGN;

        return(filemap_create_h_save_filetype_for_cmd_save_as_foreign(p_save_callback));
    }
}

_Check_return_
static STATUS
cmd_save_as_foreign_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback,
    _InVal_     T5_FILETYPE t5_filetype)
{
    save_callback_init(p_docu, p_save_callback, t5_filetype);

    return(cmd_save_as_foreign_init_set_saving(p_save_callback));
}

_Check_return_
static STATUS
cmd_save_as_foreign(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    _InVal_     BOOL save_selection)
{
    SAVE_CALLBACK save_callback;
    STATUS status;

    status_return(cmd_save_as_foreign_init(p_docu, &save_callback, t5_filetype));

    save_callback.save_selection = save_selection;

    status = proc_save_common(filename, save_callback.saving_t5_filetype, (CLIENT_HANDLE) &save_callback);

    al_array_dispose(&save_callback.h_save_filetype);

    return(status);
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_as_foreign)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    BOOL save_selection = false;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(arg_is_present(p_args, 2))
    {
        assert(ARG_TYPE_BOOL == p_args[2].type);
        save_selection = p_args[2].val.fBool;
    }

    assert((ARG_TYPE_TSTR | ARG_MANDATORY) == p_args[0].type);
    assert((ARG_TYPE_X32 | ARG_MANDATORY) == p_args[1].type);
    return(cmd_save_as_foreign(p_docu, p_args[0].val.tstr, p_args[1].val.t5_filetype, save_selection));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_as_foreign)

/******************************************************************************
*
* SAVE AS DRAW FILE
*
******************************************************************************/

/*
SAVE AS DRAW FILE - SAVE
*/

_Check_return_
static inline STATUS
do_save_drawfile_init(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _OutRef_    P_PRINT_CTRL p_print_ctrl)
{
    const P_DOCU p_docu = p_docu_from_docno(p_save_callback->docno);
    const S32 x0 = 1;
    const S32 x1 = last_page_x_non_blank(p_docu);
    const S32 y0 = 1;
    const S32 y1 = last_page_y_non_blank(p_docu);
    STATUS status;

    zero_struct_ptr_fn(p_print_ctrl);

    p_print_ctrl->flags.landscape = p_docu->page_def.landscape;

    /* p_print_ctrl->h_page_list = 0; already zeroed */

    if( !p_save_callback->page_range_list_is_set || ui_text_is_blank(&p_save_callback->page_range_list) )
    {
        assert(x1 >= 1); assert(y1 >= 1);
        status = print_page_list_add_range(&p_print_ctrl->h_page_list, x0, y0, x1, y1);
    }
    else
    {
        status = print_page_list_fill_from_tstr(&p_print_ctrl->h_page_list, ui_text_tstr(&p_save_callback->page_range_list));
    }

    return(status);
}

#include "ob_drwio/ob_drwio.h" /* <<< make MSG */

_Check_return_
static inline STATUS
do_save_drawfile_save(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InRef_     P_PRINT_CTRL p_print_ctrl)
{
    return(save_as_drawfile_host_print_document(
                p_docu_from_docno(p_save_callback->docno),
                p_print_ctrl,
                ui_text_tstr(&p_save_callback->saving_filename),
                p_save_callback->saving_t5_filetype));
}

static inline void
do_save_drawfile_finalise(
    _InoutRef_  P_PRINT_CTRL p_print_ctrl)
{
    al_array_dispose(&p_print_ctrl->h_page_list);
}

PROC_SAVE_PROTO(do_save_drawfile)
{
    STATUS status = STATUS_OK;
    PRINT_CTRL print_ctrl;

    status = do_save_drawfile_init(p_save_callback, &print_ctrl);

    if(status_ok(status))
        status = do_save_drawfile_save(p_save_callback, &print_ctrl);

    do_save_drawfile_finalise(&print_ctrl);

    return(status);
}

/*
SAVE AS DRAW FILE - CMD
*/

static void
cmd_save_as_drawfile_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback)
{
    save_callback_init(p_docu, p_save_callback, FILETYPE_DRAW);

    cmd_save_as_foreign_init_set_saving(p_save_callback);
}

_Check_return_
static STATUS
cmd_save_as_drawfile(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR filename,
    _In_opt_z_  PCTSTR tstr_page_range)
{
    SAVE_CALLBACK save_callback;

    cmd_save_as_drawfile_init(p_docu, &save_callback);

    /* page range specified? */
    if(PTR_NOT_NULL_OR_NONE(tstr_page_range))
    {
        status_return(ui_text_alloc_from_tstr(&save_callback.page_range_list, tstr_page_range));
        save_callback.page_range_list_is_set = true;
    }

    /* no need to set save_callback.saving_filename here - proc_save_common will do that */

    return(proc_save_common(filename, save_callback.saving_t5_filetype, (CLIENT_HANDLE) &save_callback));
}

T5_CMD_PROTO(extern, t5_cmd_save_as_drawfile)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    PCTSTR tstr_page_range = NULL;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* page range specified? */
    if(arg_is_present(p_args, 1))
    {
        assert(ARG_TYPE_TSTR == p_args[1].type);
        tstr_page_range = p_args[1].val.tstr;
    }

    assert((ARG_TYPE_TSTR | ARG_MANDATORY) == p_args[0].type);
    return(cmd_save_as_drawfile(p_docu, p_args[0].val.tstr, tstr_page_range));
}

/******************************************************************************
*
* SAVE PICTURE
*
******************************************************************************/

/*
SAVE PICTURE - FILEMAP
*/

_Check_return_
static inline STATUS
filemap_add_h_save_filetype_for_cmd_save_picture(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    STATUS status;
    MSG_SAVE_PICTURE_FILETYPES_REQUEST msg_save_picture_filetypes_request;
    msg_save_picture_filetypes_request.h_save_filetype = p_save_callback->h_save_filetype;
    msg_save_picture_filetypes_request.extra = (S32) (intptr_t) p_save_callback->picture_object_data_ref;
    status = object_call_id(p_save_callback->picture_object_id, p_docu_from_docno(p_save_callback->docno), T5_MSG_SAVE_PICTURE_FILETYPES_REQUEST, &msg_save_picture_filetypes_request);
    p_save_callback->h_save_filetype = msg_save_picture_filetypes_request.h_save_filetype;
    return(status);
}

_Check_return_
static STATUS
filemap_create_h_save_filetype_for_cmd_save_picture(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    /* this picture's owning object will append one or more types to this handle */
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(2, sizeof32(SAVE_FILETYPE), 0);
    status_return(al_array_preallocate_zero(&p_save_callback->h_save_filetype, &array_init_block));

    return(filemap_add_h_save_filetype_for_cmd_save_picture(p_save_callback));
}

/*
SAVE PICTURE - SAVE
*/

_Check_return_
static inline STATUS
do_save_picture_init(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    return(foreign_initialise_save(p_docu_from_docno(p_save_callback->docno), p_ff_op_format, NULL,
                                   ui_text_tstr(&p_save_callback->saving_filename), p_save_callback->saving_t5_filetype, NULL));
}

_Check_return_
static inline STATUS
do_save_picture_save(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    MSG_SAVE_PICTURE msg_save_picture;
    msg_save_picture.t5_filetype = p_save_callback->saving_t5_filetype;
    msg_save_picture.p_ff_op_format = p_ff_op_format;
    msg_save_picture.extra = (S32) (intptr_t) p_save_callback->picture_object_data_ref;
    return(object_call_id(p_save_callback->picture_object_id, p_docu_from_docno(p_save_callback->docno), T5_MSG_SAVE_PICTURE, &msg_save_picture));
}

PROC_SAVE_PROTO(do_save_picture)
{
    FF_OP_FORMAT ff_op_format = { OP_OUTPUT_INVALID };
    STATUS status;

    status_return(do_save_picture_init(p_save_callback, &ff_op_format));

    status = do_save_picture_save(p_save_callback, &ff_op_format);

    return(foreign_finalise_save(NULL, &ff_op_format, status));
}

/*
SAVE PICTURE - CMD
*/

_Check_return_
static STATUS
cmd_save_picture_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback,
    _InVal_     T5_FILETYPE t5_filetype)
{
    save_callback_init(p_docu, p_save_callback, t5_filetype);

    { /* identify note's object and ref, store in callback */
    NOTELAYER_SELECTION_INFO notelayer_selection_info;
    if(status_fail(object_call_id(OBJECT_ID_NOTE, p_docu, T5_MSG_NOTELAYER_SELECTION_INFO, &notelayer_selection_info)))
        return(STATUS_OK);  /* no note(s) selected, give up now */
    p_save_callback->picture_object_id = notelayer_selection_info.object_id;
    p_save_callback->picture_object_data_ref = notelayer_selection_info.object_data_ref;
    } /*block*/

    p_save_callback->saving = SAVING_PICTURE;

    status_return(filemap_create_h_save_filetype_for_cmd_save_picture(p_save_callback));

    return(STATUS_DONE);
}

_Check_return_
static STATUS
cmd_save_picture(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype)
{
    SAVE_CALLBACK save_callback;
    STATUS status;

    if(!object_present(OBJECT_ID_NOTE))
        return(status_check()); /* no note(s) can be selected, give up now */

    status_return(cmd_save_picture_init(p_docu, &save_callback, t5_filetype));

    status = proc_save_common(filename, save_callback.saving_t5_filetype, (CLIENT_HANDLE) &save_callback);

    al_array_dispose(&save_callback.h_save_filetype);

    return(status);
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_picture)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    T5_FILETYPE t5_filetype = FILETYPE_UNDETERMINED;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(arg_is_present(p_args, 1))
    {
        assert(ARG_TYPE_X32 == p_args[1].type);
        t5_filetype = p_args[1].val.t5_filetype;
    }

    assert((ARG_TYPE_TSTR | ARG_MANDATORY) == p_args[0].type);
    return(cmd_save_picture(p_docu, p_args[0].val.tstr, t5_filetype));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_picture)

/******************************************************************************
*
* save intro dialogue box processing
*
******************************************************************************/

enum SAVE_CONTROL_IDS
{
    SAVE_ID_FILETYPE_PICT = 32,
    SAVE_ID_NAME,
    SAVE_ID_SELECTION,

    SAVE_ID_PAGE_RANGE_LABEL,
    SAVE_ID_PAGE_RANGE_EDIT,

    SAVE_ID_FOREIGN_TYPE_LIST,  /* distinct for code */
    SAVE_ID_PICTURE_TYPE_LIST,

    SAVE_ID_TEMPLATE_STYLE_GROUP = 50,
    SAVE_ID_TEMPLATE_STYLE_0,
    SAVE_ID_TEMPLATE_STYLE_1,
    SAVE_ID_TEMPLATE_STYLE_2,
    SAVE_ID_TEMPLATE_STYLE_LIST,

    SAVE_ID_WAFFLE_1,
    SAVE_ID_WAFFLE_2,
    SAVE_ID_WAFFLE_3,
    SAVE_ID_WAFFLE_4,
    SAVE_ID_WAFFLE_5
};

#define SAVE_OWNFORM_TOTAL_H    ((PIXITS_PER_INCH * 17) / 10)
#define SAVE_TEMPLATE_LIST_H    ((PIXITS_PER_INCH * 17) / 10)
#define SAVE_FOREIGN_LIST_H     ((PIXITS_PER_INCH * 22) / 10)
#define SAVE_PICTURE_LIST_H     ((PIXITS_PER_INCH * 17) / 10)

#if RISCOS || !defined(WINDOWS_SAVE_AS)

/*
* picture representing type of file - can be dragged to save
*/

static const DIALOG_CONTROL
save_filetype_pict =
{
    SAVE_ID_FILETYPE_PICT, DIALOG_MAIN_GROUP,
    { SAVE_ID_NAME, DIALOG_CONTROL_PARENT, SAVE_ID_NAME },
    { 0, 0, 0, 68 * PIXITS_PER_RISCOS /* standard RISC OS file icon size */ },
    { DRT(LTRT, USER) }
};

static const RGB
save_filetype_pict_rgb = { 0, 0, 0, 1 /*transparent*/ };

static const DIALOG_CONTROL_DATA_USER
save_filetype_pict_data = { 0, { FRAMED_BOX_NONE /* border_style */ }, (P_RGB) &save_filetype_pict_rgb };

/*
* name of file
*/

static const DIALOG_CONTROL
save_name_normal =
{
    SAVE_ID_NAME, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, SAVE_ID_FILETYPE_PICT },
    { 0, DIALOG_STDSPACING_V, SAVE_OWNFORM_TOTAL_H, DIALOG_STDEDIT_V },
    { DRT(LBLT, EDIT), 1 /*tabstop*/ }
};

#endif /* OS */

#if RISCOS || !defined(WINDOWS_SAVE_AS)
static DIALOG_CONTROL_DATA_EDIT
save_name_data = { { { FRAMED_BOX_EDIT }, save_name_validation }, /* EDIT_XX */ { UI_TEXT_TYPE_NONE } /* state */ };
#else
static const DIALOG_CONTROL_DATA_EDIT
save_name_data = { { { FRAMED_BOX_EDIT }, NULL }, /* EDIT_XX */ { UI_TEXT_TYPE_NONE } /* state */ };
#endif

/* OK - this is NOT defbutton_ok_data! */

static const DIALOG_CONTROL_DATA_PUSHBUTTON
save_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_BUTTON_SAVE) };

#if RISCOS || !defined(WINDOWS_SAVE_AS)

static DIALOG_CONTROL
save_selection =
{
    SAVE_ID_SELECTION, DIALOG_MAIN_GROUP,
    { SAVE_ID_NAME, SAVE_ID_NAME, SAVE_ID_NAME },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCHECK_V },
    { DRT(LBRT, CHECKBOX), 1 /*tabstop*/ }
};

static DIALOG_CONTROL_DATA_CHECKBOX
save_selection_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_SELECTION) };

#endif /* RISCOS */

_Check_return_
static STATUS
dialog_save_common_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) p_dialog_msg_ctl_fill_source->client_handle;

    UNREFERENCED_PARAMETER_CONST(p_save_callback);

    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case SAVE_ID_TEMPLATE_STYLE_LIST:
        assert(SAVING_AS_TEMPLATE == p_save_callback->saving);
        p_dialog_msg_ctl_fill_source->p_ui_source = &save_as_template_intro_statics.ui_source;
        break;

    case SAVE_ID_FOREIGN_TYPE_LIST:
        assert(SAVING_AS_FOREIGN == p_save_callback->saving);
        p_dialog_msg_ctl_fill_source->p_ui_source = &save_as_foreign_intro_statics.ui_source;
        break;

    case SAVE_ID_PICTURE_TYPE_LIST:
        assert(SAVING_PICTURE == p_save_callback->saving);
        p_dialog_msg_ctl_fill_source->p_ui_source = &save_picture_intro_statics.ui_source;
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
        p_dialog_msg_ctl_create_state->state_set.state.edit.ui_text = p_save_callback->initial_filename;
        /* get this reflected into saving_filename */
        break;

    case SAVE_ID_PAGE_RANGE_EDIT:
        assert( (SAVING_HYBRID_DRAW == p_save_callback->saving) || (SAVING_AS_DRAW == p_save_callback->saving) );
        if(p_save_callback->page_range_list_is_set)
            p_dialog_msg_ctl_create_state->state_set.state.edit.ui_text = p_save_callback->page_range_list;
        break;

    case SAVE_ID_TEMPLATE_STYLE_GROUP:
        /* setup style radiobutton state */
        assert(SAVING_AS_TEMPLATE == p_save_callback->saving);
        p_dialog_msg_ctl_create_state->state_set.state.radiobutton = 0; /* SKS 27jan93 changed from 1 */
        break;

    case SAVE_ID_TEMPLATE_STYLE_LIST:
        assert(SAVING_AS_TEMPLATE == p_save_callback->saving);
        style_name_from_marked_area(p_docu, &p_dialog_msg_ctl_create_state->state_set.state.list_text.ui_text);
        if(ui_text_is_blank(&p_dialog_msg_ctl_create_state->state_set.state.list_text.ui_text))
        {
            p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = 0 /*select one of 'em anyway*/ /*DIALOG_CTL_STATE_LIST_ITEM_NONE*/;
            p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALTERNATE;
        }
        break;

    case SAVE_ID_FOREIGN_TYPE_LIST:
        assert(SAVING_AS_FOREIGN == p_save_callback->saving);
        p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = save_as_foreign_intro_statics.itemno;
        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALTERNATE;
        break;

    case SAVE_ID_PICTURE_TYPE_LIST:
        assert(SAVING_PICTURE == p_save_callback->saving);
        p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = save_picture_intro_statics.itemno;
        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALTERNATE;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

static void
dialog_save_ctl_state_change_selection_filename(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InRef_     H_DIALOG h_dialog)
{
    static UI_TEXT selection_filename = UI_TEXT_INIT_RESID(MSG_SELECTION);
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = h_dialog;
    dialog_cmd_ctl_state_set.dialog_control_id = SAVE_ID_NAME;
    dialog_cmd_ctl_state_set.bits = 0;
    dialog_cmd_ctl_state_set.state.edit.ui_text = p_save_callback->save_selection ? selection_filename : p_save_callback->initial_filename;
    p_save_callback->self_abuse++;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));
    p_save_callback->self_abuse--;
}

static void
dialog_save_ctl_state_change_selection(
    P_SAVE_CALLBACK p_save_callback,
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    p_save_callback->save_selection = (p_dialog_msg_ctl_state_change->new_state.checkbox == DIALOG_BUTTONSTATE_ON);

    if(p_save_callback->filename_edited <= 1)
        dialog_save_ctl_state_change_selection_filename(p_save_callback, p_dialog_msg_ctl_state_change->h_dialog);
}

static void
dialog_save_common_provoke_filetype_pict_redraw(
    H_DIALOG    h_dialog)
{
#if RISCOS
    DIALOG_CMD_CTL_ENCODE dialog_cmd_ctl_encode;
    msgclr(dialog_cmd_ctl_encode);
    dialog_cmd_ctl_encode.h_dialog = h_dialog;
    dialog_cmd_ctl_encode.dialog_control_id = SAVE_ID_FILETYPE_PICT;
    dialog_cmd_ctl_encode.bits = 0;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_ENCODE, &dialog_cmd_ctl_encode));
#else
    UNREFERENCED_PARAMETER(h_dialog);
#endif /* OS */
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
        ui_text_dispose(&p_save_callback->saving_filename);
        status_assert(ui_text_copy(&p_save_callback->saving_filename, &p_dialog_msg_ctl_state_change->new_state.edit.ui_text));

        if(!p_save_callback->self_abuse)
            p_save_callback->filename_edited++;

        break;
        }

    case SAVE_ID_SELECTION:
        dialog_save_ctl_state_change_selection(p_save_callback, p_dialog_msg_ctl_state_change);
        break;

    case SAVE_ID_PAGE_RANGE_EDIT:
        assert( (SAVING_HYBRID_DRAW == p_save_callback->saving) || (SAVING_AS_DRAW == p_save_callback->saving) );
        ui_text_dispose(&p_save_callback->page_range_list);
        status_assert(ui_text_copy(&p_save_callback->page_range_list, &p_dialog_msg_ctl_state_change->new_state.edit.ui_text));
        assert(p_save_callback->page_range_list_is_set);
        break;

    case SAVE_ID_TEMPLATE_STYLE_GROUP:
        p_save_callback->style_radio = p_dialog_msg_ctl_state_change->new_state.radiobutton;
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, SAVE_ID_TEMPLATE_STYLE_LIST, (SAVING_TEMPLATE_ONE_STYLE == p_save_callback->style_radio));
        break;

    case SAVE_ID_TEMPLATE_STYLE_LIST:
        p_save_callback->style_handle = style_handle_from_name(p_docu, ui_text_tstr(&p_dialog_msg_ctl_state_change->new_state.list_text.ui_text));
        break;

    case SAVE_ID_FOREIGN_TYPE_LIST:
    case SAVE_ID_PICTURE_TYPE_LIST:
        dialog_save_common_provoke_filetype_pict_redraw(p_dialog_msg_ctl_state_change->h_dialog);
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_save_common_ctl_pushbutton_ok(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    PCTSTR filename;

    p_save_callback->h_dialog = p_dialog_msg_ctl_pushbutton->h_dialog;

    dialog_save_common_details_update(p_save_callback);

    ui_text_dispose(&p_save_callback->saving_filename);
    RISCOS_ONLY(ui_dlg_get_edit(p_save_callback->h_dialog, SAVE_ID_NAME, &p_save_callback->saving_filename));

    switch(p_save_callback->saving)
    {
    case SAVING_HYBRID_DRAW:
    case SAVING_AS_DRAW:
        ui_text_dispose(&p_save_callback->page_range_list);
        ui_dlg_get_edit(p_save_callback->h_dialog, SAVE_ID_PAGE_RANGE_EDIT, &p_save_callback->page_range_list);
        break;

    default:
        break;
    }

    status_return(dialog_save_common_check(p_save_callback)); /* NB sets p_save_callback->allow_not_rooted for subsequent test */

    filename = ui_text_tstr(&p_save_callback->saving_filename);

    if( !p_save_callback->allow_not_rooted && !file_is_rooted(filename) )
    {
        reperr_null(ERR_SAVE_DRAG_TO_DIRECTORY);
        return(STATUS_OK);
    }

    consume_bool(proc_save_common(filename, p_save_callback->saving_t5_filetype, (CLIENT_HANDLE) p_save_callback));

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_save_common_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case IDOK:
        return(dialog_save_common_ctl_pushbutton_ok(p_dialog_msg_ctl_pushbutton, (P_SAVE_CALLBACK) p_dialog_msg_ctl_pushbutton->client_handle));

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
    WimpIconBlockWithBitset icon;
    zero_struct_fn(icon);

    p_save_callback->h_dialog = p_dialog_msg_ctl_user_redraw->h_dialog;

    dialog_save_common_details_update(p_save_callback);

    host_ploticon_setup_bbox(&icon, &p_dialog_msg_ctl_user_redraw->control_inner_box, &p_dialog_msg_ctl_user_redraw->redraw_context);

    /* fills in sprite_name with name of a sprite representing the filetype from the Window Manager Sprite Pool */
    dialog_riscos_file_icon_setup(&icon, p_save_callback->saving_t5_filetype);

    host_ploticon(&icon);

    return(STATUS_OK);
}

_Check_return_
static inline BOOL
dialog_save_common_ctl_user_mouse_click_left_drag_test_hit(
    _InRef_     PC_DIALOG_MSG_CTL_USER_MOUSE p_dialog_msg_ctl_user_mouse)
{
    const WimpMouseClickEvent * const p_mouse_click = p_dialog_msg_ctl_user_mouse->riscos.p_mouse_click;
    const U32 XEigFactor = host_modevar_cache_current.XEigFactor;
    GDI_POINT gdi_org;
    GDI_POINT point;
    GDI_COORD centre_x;
    BBox bbox;

    host_gdi_org_from_screen(&gdi_org, p_mouse_click->window_handle); /* window work area ABS origin */

    point.x = p_mouse_click->mouse_x - gdi_org.x; /* mouse position relative to */
    point.y = p_mouse_click->mouse_y - gdi_org.y; /* window (Save As dbox) origin */

    bbox = p_dialog_msg_ctl_user_mouse->riscos.icon.bbox;

    /* standard RISC OS file icon size is 68 OS units square - did we hit that about the centre point? */
    centre_x = ((bbox.xmin + bbox.xmax) >> (XEigFactor + 1)) << XEigFactor;

    return( (point.x >= centre_x - (68/2)) && (point.x < centre_x + (68/2)) );
}

_Check_return_
static STATUS
dialog_save_common_ctl_user_mouse_click_left_drag(
    _InRef_     PC_DIALOG_MSG_CTL_USER_MOUSE p_dialog_msg_ctl_user_mouse,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    WimpIconBlockWithBitset icon;
    zero_struct_fn(icon);

    if(!dialog_save_common_ctl_user_mouse_click_left_drag_test_hit(p_dialog_msg_ctl_user_mouse))
        return(STATUS_OK); /* haven't hit the actual file icon this time */

    p_save_callback->h_dialog = p_dialog_msg_ctl_user_mouse->h_dialog;

    icon.bbox = p_dialog_msg_ctl_user_mouse->riscos.icon.bbox;

    dialog_save_common_details_update(p_save_callback);

    /* fills in icon with name of a sprite representing the filetype from the Window Manager Sprite Pool */
    dialog_riscos_file_icon_setup(&icon, p_save_callback->saving_t5_filetype);

    ui_dlg_get_edit(p_save_callback->h_dialog, SAVE_ID_NAME, &p_save_callback->saving_filename);

    status_return(dialog_save_common_check(p_save_callback));

    status_return(dialog_riscos_file_icon_drag(p_dialog_msg_ctl_user_mouse->h_dialog, &icon));

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
        return(dialog_save_common_ctl_user_mouse_click_left_drag(p_dialog_msg_ctl_user_mouse, (P_SAVE_CALLBACK) p_dialog_msg_ctl_user_mouse->client_handle));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
dialog_save_common_riscos_drag_ended(
    _InRef_     P_DIALOG_MSG_RISCOS_DRAG_ENDED p_dialog_msg_riscos_drag_ended)
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) p_dialog_msg_riscos_drag_ended->client_handle;

    if(p_dialog_msg_riscos_drag_ended->mouse.window_handle != -1)
    {
        /* try saving file to target window */
        consume_bool(
            host_xfer_save_file(
                ui_text_tstr(&p_save_callback->saving_filename),
                p_save_callback->saving_t5_filetype,
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
dialog_save_common_details_update_foreign( /* or _picture */
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    T5_FILETYPE t_t5_filetype = array_ptrc(&p_save_callback->h_save_filetype, SAVE_FILETYPE, 0)->t5_filetype;

    if(array_elements(&p_save_callback->h_save_filetype) >= 2)
    {
        const DIALOG_CONTROL_ID dialog_control_id = (SAVING_AS_FOREIGN == p_save_callback->saving) ? SAVE_ID_FOREIGN_TYPE_LIST : SAVE_ID_PICTURE_TYPE_LIST;
        const T5_FILETYPE t5_filetype_default = (SAVING_AS_FOREIGN == p_save_callback->saving) ? FILETYPE_ASCII : FILETYPE_DRAW;
        UI_TEXT ui_text;

        ui_dlg_get_list_text(p_save_callback->h_dialog, dialog_control_id, &ui_text);

        if(FILETYPE_UNDETERMINED == (t_t5_filetype = filemap_lookup_filetype_from_description(p_save_callback, &ui_text)))
            t_t5_filetype = t5_filetype_default;

        ui_text_dispose(&ui_text);
    }

    p_save_callback->saving_t5_filetype = t_t5_filetype;
}

static void
dialog_save_common_details_update(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    switch(p_save_callback->saving)
    {
    default: default_unhandled();
#if CHECKING
    case SAVING_DOCUMENT:
    case SAVING_AS_TEMPLATE:
    case SAVING_HYBRID_DRAW:
    case SAVING_AS_DRAW:
    case LOCATING_TEMPLATE:
#endif
        break;

    case SAVING_AS_FOREIGN:
#if !defined(FOREIGN_FILE_TYPE_LIST)
        break;
#endif
    case SAVING_PICTURE:
        dialog_save_common_details_update_foreign(p_save_callback);
        break;
    }
}

_Check_return_
static STATUS
dialog_save_common_check(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    p_save_callback->allow_not_rooted = FALSE;

    /*if(SAVING_DOCUMENT == p_save_callback->saving)
        if(!p_save_callback->save_selection)
        {
            const P_DOCU p_docu = p_docu_from_docno(p_save_callback->docno);
        }*/

    if(SAVING_AS_TEMPLATE == p_save_callback->saving)
    {
        if(SAVING_TEMPLATE_ONE_STYLE == p_save_callback->style_radio)
        {
            if(status_fail(p_save_callback->style_handle))
            {
                host_bleep();
                return(STATUS_FAIL);
            }
        }

        /* SKS after 1.03 08jun93 - only allow implicit save into template directory if it's a whole file save */
        p_save_callback->allow_not_rooted = (SAVING_TEMPLATE_ALL == p_save_callback->style_radio);
    }

    return(STATUS_OK);
}

#if RISCOS || !defined(WINDOWS_SAVE_AS)

static DIALOG_CTL_CREATE
save_ctl_create[] =
{
    { &dialog_main_group }

,   { { &save_filetype_pict },  &save_filetype_pict_data    }
,   { { &save_name_normal },    &save_name_data             }
,   { { &save_selection },      &save_selection_data        }

,   { { &defbutton_ok }, &save_ok_data }
,   { { &stdbutton_cancel }, &stdbutton_cancel_data }
};

_Check_return_
static STATUS
cmd_save_as_intro_dialog(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, save_ctl_create, elemof32(save_ctl_create), MSG_DIALOG_SAVE_CAPTION);
    dialog_cmd_process_dbox.help_topic_resource_id = MSG_DIALOG_SAVE_HELP_TOPIC;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_save_common;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_save_callback;
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

#endif /* RISCOS */

/*
SAVE HYBRID DRAW - INTRO CMD
*/

#if RISCOS || !defined(WINDOWS_SAVE_AS)

static const DIALOG_CONTROL
save_page_range_label =
{
    SAVE_ID_PAGE_RANGE_LABEL, DIALOG_MAIN_GROUP,
#if RISCOS || !defined(WINDOWS_SAVE_AS)
    { SAVE_ID_NAME, SAVE_ID_PAGE_RANGE_EDIT, DIALOG_CONTROL_SELF, SAVE_ID_PAGE_RANGE_EDIT },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
#else
    { DIALOG_CONTROL_PARENT, SAVE_ID_PAGE_RANGE_EDIT, DIALOG_CONTROL_SELF, SAVE_ID_PAGE_RANGE_EDIT },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, TEXTLABEL) }
#endif
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
save_page_range_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_PRINT_PAGES) };

static const DIALOG_CONTROL
save_page_range_edit =
{
    SAVE_ID_PAGE_RANGE_EDIT, DIALOG_MAIN_GROUP,
#if RISCOS || !defined(WINDOWS_SAVE_AS)
    { SAVE_ID_PAGE_RANGE_LABEL, SAVE_ID_NAME, SAVE_ID_NAME },
    { DIALOG_LABELGAP_H, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V },
    { DRT(RBRT, EDIT), 1 /*tabstop*/ }
#else
    { SAVE_ID_PAGE_RANGE_LABEL, DIALOG_CONTROL_PARENT },
    { DIALOG_LABELGAP_H, 0, (PIXITS_PER_INCH * 3) / 4, DIALOG_STDEDIT_V },
    { DRT(RTLT, EDIT), 1 /*tabstop*/ }
#endif
};

static const DIALOG_CONTROL_DATA_EDIT
save_page_range_edit_data = { { { FRAMED_BOX_EDIT }, print_page_range_edit_validation }, /* EDIT_XX */ { UI_TEXT_TYPE_NONE } /* state */ };

static DIALOG_CTL_CREATE
save_hybrid_draw_ctl_create[] =
{
    { &dialog_main_group }

#if RISCOS || !defined(WINDOWS_SAVE_AS)
,   { { &save_filetype_pict },  &save_filetype_pict_data    }
,   { { &save_name_normal },    &save_name_data             }
#endif

,   { { &save_page_range_label },   &save_page_range_label_data }
,   { { &save_page_range_edit },    &save_page_range_edit_data  }

,   { { &defbutton_ok }, &save_ok_data }
,   { { &stdbutton_cancel }, &stdbutton_cancel_data }
};

_Check_return_
static STATUS
cmd_save_as_hybrid_draw_intro_dialog(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, save_hybrid_draw_ctl_create, elemof32(save_hybrid_draw_ctl_create), MSG_DIALOG_SAVE_CAPTION);
    dialog_cmd_process_dbox.help_topic_resource_id = MSG_DIALOG_SAVE_HELP_TOPIC;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_save_common;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_save_callback;
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

#endif /* RISCOS */

#if WINDOWS

static void
cmd_save_as_intro_init_set_initial_directory(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    TCHARZ initial_directory[BUF_MAX_PATHSTRING];

    p_save_callback->initial_directory.type = UI_TEXT_TYPE_NONE;

    if(file_dirname(initial_directory, ui_text_tstr(&p_save_callback->initial_filename)))
        status_assert(ui_text_alloc_from_tstr(&p_save_callback->initial_directory, initial_directory));
}

#endif /* WINDOWS */

static void
cmd_save_as_intro_init_page_range(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback,
    _In_z_      PCTSTR tstr_page_range)
{
    const P_UI_TEXT p_ui_text = &p_save_callback->page_range_list;

    p_ui_text->type = UI_TEXT_TYPE_TSTR_TEMP;
    p_ui_text->text.tstr = tstr_page_range;

    p_save_callback->page_range_list_is_set = true;

    if( PTR_IS_NULL_OR_NONE(tstr_page_range) || (CH_NULL == *tstr_page_range) )
    {
#if 1
        /* Empty - leave blank, which implies All Pages */
        p_ui_text->type = UI_TEXT_TYPE_TSTR_PERM;
        p_ui_text->text.tstr = tstr_empty_string;

        UNREFERENCED_PARAMETER_DocuRef_(p_docu);
#else
        /* Empty - suggest everything */
        const PAGE page_range_y0 = 1;
        const PAGE page_range_y1 = last_page_y_non_blank(p_docu);
        static TCHARZ tstr_buf[32];

        consume_int(tstr_xsnprintf(tstr_buf, elemof32(tstr_buf),
                                   (page_range_y0 == page_range_y1)
                                       ? S32_TFMT
                                       : S32_TFMT TEXT(" - ") S32_TFMT,
                                   page_range_y0, page_range_y1));

        p_ui_text->type = UI_TEXT_TYPE_TSTR_PERM;
        p_ui_text->text.tstr = tstr_buf;
#endif
    }

    print_page_range_edit_validation_setup();
}

static void
cmd_save_as_intro_init(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock)
{
#if !RISCOS && 0
    if(0 != p_docu->mark_info_cells.h_markers) /* always do save selection if markers present */
        p_save_callback->save_selection = TRUE;
#endif

    if(p_save_callback->save_selection)
    {
        p_save_callback->initial_filename.type = UI_TEXT_TYPE_RESID;
        p_save_callback->initial_filename.text.resource_id = MSG_SELECTION;

#if !RISCOS
        if(NULL != p_docu->docu_name.path_name) /* suggest saving adjacent to this document */
            status_assert(quick_tblock_tstr_add(p_quick_tblock, p_docu->docu_name.path_name));
        status_assert(resource_lookup_quick_tblock(p_quick_tblock, MSG_SELECTION));
        status_assert(quick_tblock_tstr_add(p_quick_tblock, extension_document_tstr));

        p_save_callback->initial_filename.type = UI_TEXT_TYPE_TSTR_TEMP;
        p_save_callback->initial_filename.text.tstr = quick_tblock_tstr(p_quick_tblock);
#endif
    }
    else
    {
        BOOL changing_type = FALSE;

        if(NULL != p_docu->docu_name.path_name)
        {
            if(SAVING_HYBRID_DRAW == p_save_callback->saving)
                changing_type = (FILETYPE_T5_HYBRID_DRAW != p_docu->docu_preferred_filetype);
            else
                changing_type = (FILETYPE_T5_HYBRID_DRAW == p_docu->docu_preferred_filetype);
        }

        status_assert(name_make_wholename(&p_docu->docu_name, p_quick_tblock, FALSE));

        /* put the right extension on, but only if there was one already */
        /* or we are changing file type (suggests not to overwrite) */
        if( (NULL != p_docu->docu_name.extension) || changing_type )
        {
            quick_tblock_nullch_strip(p_quick_tblock);
            status_assert(quick_tblock_tstr_add(p_quick_tblock, FILE_EXT_SEP_TSTR));
            status_assert(quick_tblock_tstr_add_n(p_quick_tblock, ((SAVING_HYBRID_DRAW == p_save_callback->saving) ? extension_hybrid_draw_tstr : extension_document_tstr), strlen_with_NULLCH));
        }

        p_save_callback->initial_filename.type = UI_TEXT_TYPE_TSTR_TEMP;
        p_save_callback->initial_filename.text.tstr = quick_tblock_tstr(p_quick_tblock);
    }

    WINDOWS_ONLY(cmd_save_as_intro_init_set_initial_directory(p_save_callback));

    if(SAVING_HYBRID_DRAW == p_save_callback->saving)
        cmd_save_as_intro_init_page_range(p_docu, p_save_callback, p_docu->tstr_hybrid_draw_page_range);

#if RISCOS || !defined(WINDOWS_SAVE_AS)
    //save_filetype_pict.relative_offset[0] = -( (save_name_normal.relative_offset[2] - save_filetype_pict.relative_offset[2]) / 2 );
#endif

#if RISCOS
    p_save_callback->test_for_saved_file_is_safe = TRUE;
    host_xfer_set_saved_file_is_safe(TRUE);
#endif
}

static void
cmd_save_as_intro_finalise(
    _OutRef_    P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock)
{
    quick_tblock_dispose(p_quick_tblock);

    WINDOWS_ONLY(ui_text_dispose(&p_save_callback->initial_directory));
    ui_text_dispose(&p_save_callback->initial_filename);
    ui_text_dispose(&p_save_callback->saving_filename);
}

_Check_return_
static STATUS
cmd_save_as_intro(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_FILETYPE t5_filetype) /* may be FILETYPE_UNDETERMINED */
{
    STATUS status;
    SAVE_CALLBACK save_callback;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
    quick_tblock_with_buffer_setup(quick_tblock);

    cmd_save_as_init(p_docu, &save_callback, t5_filetype);

    cmd_save_as_intro_init(p_docu, &save_callback, &quick_tblock);

#if RISCOS || !defined(WINDOWS_SAVE_AS)
    if(SAVING_HYBRID_DRAW == save_callback.saving)
        status = cmd_save_as_hybrid_draw_intro_dialog(p_docu, &save_callback);
    else
#endif
        status = RISCOS_OR_WINDOWS(cmd_save_as_intro_dialog, windows_save_as) (p_docu, &save_callback);

    cmd_save_as_intro_finalise(&save_callback, &quick_tblock);

    return(status);
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_as_intro)
{
    T5_FILETYPE t5_filetype = FILETYPE_UNDETERMINED;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(object_present(OBJECT_ID_NOTE))
    {
        NOTELAYER_SELECTION_INFO notelayer_selection_info;
        if(status_ok(object_call_id(OBJECT_ID_NOTE, p_docu, T5_MSG_NOTELAYER_SELECTION_INFO, &notelayer_selection_info)))
            return(cmd_save_picture_intro(p_docu)); /* note(s) selected, process differently */
    }

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

        if(arg_is_present(p_args, 0))
        {
            assert(ARG_TYPE_X32 == p_args[0].type);
            t5_filetype = p_args[0].val.t5_filetype;

            /* consider useful mutation here */
            if(FILETYPE_T5_FIREWORKZ == t5_filetype)
                if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
                    t5_filetype = FILETYPE_T5_RECORDZ;
        }
    }

    return(cmd_save_as_intro(p_docu, t5_filetype));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_as_intro)

/*
SAVE AS TEMPLATE - INTRO CMD
*/

static DIALOG_CONTROL
save_name_template =
{
    SAVE_ID_NAME, DIALOG_MAIN_GROUP,

#if RISCOS || !defined(WINDOWS_SAVE_AS)
    { DIALOG_CONTROL_PARENT, SAVE_ID_FILETYPE_PICT },
    { 0, DIALOG_STDSPACING_V, SAVE_TEMPLATE_LIST_H, DIALOG_STDEDIT_V },
    { DRT(LBLT, EDIT), 1 /*tabstop*/ }
#else
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, SAVE_TEMPLATE_LIST_H, DIALOG_STDEDIT_V },
    { DRT(LTLT, EDIT), 1 /*tabstop*/ }
#endif
};

static DIALOG_CONTROL
save_template_style_group =
{
    SAVE_ID_TEMPLATE_STYLE_GROUP, DIALOG_MAIN_GROUP,
    { SAVE_ID_NAME, SAVE_ID_NAME, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V },
    { DRT(LBRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static DIALOG_CONTROL
save_template_style_0 =
{
    SAVE_ID_TEMPLATE_STYLE_0, SAVE_ID_TEMPLATE_STYLE_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static DIALOG_CONTROL_DATA_RADIOBUTTON
save_template_style_0_data = { { 0 }, SAVING_TEMPLATE_ALL, UI_TEXT_INIT_RESID(MSG_DIALOG_SAVE_TEMPLATE_ALL) };

static DIALOG_CONTROL
save_template_style_1 =
{
    SAVE_ID_TEMPLATE_STYLE_1, SAVE_ID_TEMPLATE_STYLE_GROUP,
    { SAVE_ID_TEMPLATE_STYLE_0, SAVE_ID_TEMPLATE_STYLE_0 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static DIALOG_CONTROL_DATA_RADIOBUTTON
save_template_style_1_data = { { 0 }, SAVING_TEMPLATE_ALL_STYLES, UI_TEXT_INIT_RESID(MSG_DIALOG_SAVE_TEMPLATE_ALL_STYLES) };

static DIALOG_CONTROL
save_template_style_2 =
{
    SAVE_ID_TEMPLATE_STYLE_2, SAVE_ID_TEMPLATE_STYLE_GROUP,
    { SAVE_ID_TEMPLATE_STYLE_1, SAVE_ID_TEMPLATE_STYLE_1 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static DIALOG_CONTROL_DATA_RADIOBUTTON
save_template_style_2_data = { { 0 }, SAVING_TEMPLATE_ONE_STYLE, UI_TEXT_INIT_RESID(MSG_DIALOG_SAVE_TEMPLATE_ONE_STYLE) };

/*
* list of styles
*/

#define SAVE_TEMPLATE_STYLE_COMBO 1

#if defined(SAVE_TEMPLATE_STYLE_COMBO)

static /*poked*/ DIALOG_CONTROL
save_template_style_combo =
{
    SAVE_ID_TEMPLATE_STYLE_LIST, SAVE_ID_TEMPLATE_STYLE_GROUP,
    { SAVE_ID_TEMPLATE_STYLE_2, SAVE_ID_TEMPLATE_STYLE_2 },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCOMBO_V },
    { DRT(LBLT, COMBO_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const UI_TEXT
save_template_style_combo_popup_caption = UI_TEXT_INIT_RESID(MSG_STYLE_TEXT_CONVERT_START); /*(MSG_DIALOG_SAVE_TEMPLATE_ONE_STYLE)*/

static const DIALOG_CONTROL_DATA_COMBO_TEXT
save_template_style_combo_data =
{
  {/*combo_xx*/

    {/*edit_xx*/ {/*bits*/ FRAMED_BOX_EDIT, 1 /*read_only*/ /*bits*/}, NULL /*edit_xx*/},

    {/*list_xx*/ { 0 /*force_v_scroll*/, 0 /*disable_double*/, 0 /*tab_position*/} /*list_xx*/},

        NULL,

        9 * DIALOG_STDLISTITEM_V /*dropdown_size*/

#if RISCOS
        , &save_template_style_combo_popup_caption
#endif

      /*combo_xx*/},

    { UI_TEXT_TYPE_NONE } /*state*/
};

#else /* NOT SAVE_TEMPLATE_STYLE_COMBO */

static /*poked*/ DIALOG_CONTROL
save_template_style_list =
{
    SAVE_ID_TEMPLATE_STYLE_LIST, SAVE_ID_TEMPLATE_STYLE_GROUP,
    { SAVE_ID_TEMPLATE_STYLE_2, SAVE_ID_TEMPLATE_STYLE_2 },
    { 0, DIALOG_STDSPACING_V },
    { DRT(LBLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

#endif /* SAVE_TEMPLATE_STYLE_COMBO */

static DIALOG_CTL_CREATE
save_as_template_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

#if RISCOS || !defined(WINDOWS_SAVE_AS)
    { { &save_filetype_pict },        &save_filetype_pict_data },
#endif

    { { &save_name_template },        &save_name_data },

    { { &save_template_style_group }, NULL },
#if defined(SAVE_TEMPLATE_STYLE_COMBO)
    { { &save_template_style_combo }, &save_template_style_combo_data },
#else
    { { &save_template_style_list },  &stdlisttext_data_dd }, /* first to get initial state set! */
#endif
    { { &save_template_style_0 },     &save_template_style_0_data },
    { { &save_template_style_1 },     &save_template_style_1_data },
    { { &save_template_style_2 },     &save_template_style_2_data },

    { { &defbutton_ok }, &save_ok_data },
    { { &stdbutton_cancel }, &stdbutton_cancel_data }
};

static void
cmd_save_as_template_intro_resize_controls(
  /*_InRef_     PC_SAVE_CALLBACK p_save_callback,*/
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InVal_     PIXIT max_width)
{
    /* make appropriate size box */
    S32 show_elements = array_elements(p_array_handle);
    PIXIT_SIZE list_size;
    DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
#if defined(SAVE_TEMPLATE_STYLE_COMBO)
    dialog_cmd_ctl_size_estimate.p_dialog_control = &save_template_style_combo;
    dialog_cmd_ctl_size_estimate.p_dialog_control_data.combo_text = &save_template_style_combo_data;
#else
    dialog_cmd_ctl_size_estimate.p_dialog_control = &save_template_style_list;
    dialog_cmd_ctl_size_estimate.p_dialog_control_data.list_text = &stdlisttext_data;
#endif
    ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
    dialog_cmd_ctl_size_estimate.size.x += max_width;
    show_elements = MAX(show_elements, 2);
    show_elements = MIN(show_elements, 8);
    ui_list_size_estimate(array_elements(p_array_handle), &list_size);
    dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
    dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
    dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, SAVE_TEMPLATE_LIST_H);
#if defined(SAVE_TEMPLATE_STYLE_COMBO)
    save_template_style_combo.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
#else
    save_template_style_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
    save_template_style_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
#endif

#if RISCOS || !defined(WINDOWS_SAVE_AS)
#if defined(SAVE_TEMPLATE_STYLE_COMBO)
    //save_filetype_pict.relative_offset[0] = -( (save_template_style_combo.relative_offset[2] - save_filetype_pict.relative_offset[2]) / 2 );
#else
    //save_filetype_pict.relative_offset[0] = -( (save_template_style_list.relative_offset[2] - save_filetype_pict.relative_offset[2]) / 2 );
#endif
#endif
}

_Check_return_
static STATUS
cmd_save_as_template_intro_init(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _OutRef_    P_ARRAY_HANDLE p_array_handle)
{
    PIXIT max_width;

    p_save_callback->initial_filename.type = UI_TEXT_TYPE_RESID;
    p_save_callback->initial_filename.text.resource_id = MSG_DIALOG_SAVE_TEMPLATE_NAME;

    /* encode initial state of control(s) */
    status_return(ui_list_create_style(p_docu_from_docno(p_save_callback->docno), p_array_handle, &save_as_template_intro_statics.ui_source, &max_width));

    cmd_save_as_template_intro_resize_controls(/*p_save_callback,*/ p_array_handle, max_width);

#if RISCOS
    p_save_callback->test_for_saved_file_is_safe = TRUE;
    host_xfer_set_saved_file_is_safe(TRUE);
#endif

    return(STATUS_OK);
}

_Check_return_
static STATUS
cmd_save_as_template_intro_dialog(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, save_as_template_ctl_create, elemof32(save_as_template_ctl_create), MSG_DIALOG_SAVE_TEMPLATE_CAPTION);
    dialog_cmd_process_dbox.help_topic_resource_id = MSG_DIALOG_SAVE_TEMPLATE_HELP_TOPIC;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_save_common;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_save_callback;
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

static inline void
cmd_save_as_template_intro_finalise(
    _OutRef_    P_SAVE_CALLBACK p_save_callback,
    _InoutRef_  P_ARRAY_HANDLE p_array_handle)
{
    ui_list_dispose_style(p_array_handle, &save_as_template_intro_statics.ui_source);

    ui_text_dispose(&p_save_callback->initial_filename);
    ui_text_dispose(&p_save_callback->saving_filename);
}

_Check_return_
static inline STATUS
cmd_save_as_template_intro(
    _DocuRef_   P_DOCU p_docu)
{
    SAVE_CALLBACK save_callback;
    STATUS status;
    ARRAY_HANDLE save_template_style_list_handle;

    cmd_save_as_template_init(p_docu, &save_callback, SAVING_TEMPLATE_ALL);

    status_return(cmd_save_as_template_intro_init(&save_callback, &save_template_style_list_handle));

    status = cmd_save_as_template_intro_dialog(p_docu, &save_callback);

    cmd_save_as_template_intro_finalise(&save_callback, &save_template_style_list_handle);

    return(status);
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_as_template_intro)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    return(cmd_save_as_template_intro(p_docu));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_as_template_intro)

/******************************************************************************
*
* lookup returned description on list to yield filetype to use
*
******************************************************************************/

PROC_BSEARCH_PROTO(static, save_filetype_description_compare_bsearch, UI_TEXT, SAVE_FILETYPE)
{
    BSEARCH_KEY_VAR_DECL(PC_UI_TEXT, p1);
    BSEARCH_DATUM_VAR_DECL(PC_SAVE_FILETYPE, p2);

    return(ui_text_compare(p1, &p2->description, 1 /*fussy*/, 1 /*insensitive*/));
}

#if defined(SAVE_FILETYPE_DESCRIPTION_SORTED)

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, save_filetype_description_compare_qsort, SAVE_FILETYPE)
{
    QSORT_ARG1_VAR_DECL(PC_SAVE_FILETYPE, p1);
    QSORT_ARG2_VAR_DECL(PC_SAVE_FILETYPE, p2);

    const PC_UI_TEXT text_1 = &p1->description;
    const PC_UI_TEXT text_2 = &p2->description;

    return(ui_text_compare(text_1, text_2, 1 /*fussy*/, 1 /*insensitive*/));
}

#endif

_Check_return_
static T5_FILETYPE
filemap_lookup_filetype_from_description(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InRef_     PC_UI_TEXT p_description)
{
#if defined(SAVE_FILETYPE_DESCRIPTION_SORTED)
    const PC_SAVE_FILETYPE p_save_filetype = al_array_bsearch(p_description, &p_save_callback->h_save_filetype, SAVE_FILETYPE, save_filetype_description_compare_bsearch);
#else
    const PC_SAVE_FILETYPE p_save_filetype = al_array_lsearch(p_description, &p_save_callback->h_save_filetype, SAVE_FILETYPE, save_filetype_description_compare_bsearch);
#endif
    return(!IS_P_DATA_NONE(p_save_filetype) ? p_save_filetype->t5_filetype : FILETYPE_UNDETERMINED);
}

/******************************************************************************
*
* create a list of textual representations of filetypes from the loaded objects
*
******************************************************************************/

_Check_return_
static STATUS
filemap_create_from_h_save_filetype_for_intro(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _InVal_     T5_FILETYPE t5_filetype,
    _OutRef_opt_ P_UI_TEXT p_ui_text_dst,
    _InoutRef_opt_ P_UI_SOURCE p_ui_source_dst)
{
    STATUS status = STATUS_OK;

    if(NULL != p_ui_text_dst)
        p_ui_text_dst->type = UI_TEXT_TYPE_NONE;

    if(NULL != p_ui_source_dst)
    {
        P_UI_TEXT p_ui_text;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_ui_text), 0);
        const ARRAY_INDEX filemap_elements = array_elements(&p_save_callback->h_save_filetype);

        al_array_dispose(&p_ui_source_dst->source.array_handle);

        /* lookup filetype in list to obtain current state for combo,
         * also make a source of text pointers to these elements
        */
        if(NULL == (p_ui_text = al_array_alloc_UI_TEXT(&p_ui_source_dst->source.array_handle, filemap_elements, &array_init_block, &status)))
        {
            al_array_dispose(&p_save_callback->h_save_filetype);
            return(status);
        }

        {
        ARRAY_INDEX i;
        P_SAVE_FILETYPE p_save_filetype = array_range(&p_save_callback->h_save_filetype, SAVE_FILETYPE, 0, filemap_elements);

        for(i = 0; i < filemap_elements; ++i, ++p_save_filetype, ++p_ui_text)
        {
            status_assert(ui_text_copy(p_ui_text, &p_save_filetype->description));

            if( (p_save_filetype->t5_filetype == t5_filetype) && (NULL != p_ui_text_dst) && !ui_text_is_blank(&p_save_filetype->suggested_leafname) )
                status_assert(ui_text_copy(p_ui_text_dst, &p_save_filetype->suggested_leafname));
        }
        } /*block*/

        p_ui_source_dst->type = UI_SOURCE_TYPE_ARRAY;
    }

    return(STATUS_OK);
}

/*
SAVE AS FOREIGN - INTRO CMD
*/

#if RISCOS || !defined(WINDOWS_SAVE_AS)

#if defined(FOREIGN_TYPE_LIST_BOX)

static const DIALOG_CONTROL
save_name_foreign =
{
    SAVE_ID_NAME, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, SAVE_ID_FILETYPE_PICT, SAVE_ID_FOREIGN_TYPE_LIST },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V },
    { DRT(LBRT, EDIT), 1 /*tabstop*/ }
};

/*
* list of types of file we can export document as
*/

static /*poked*/ DIALOG_CONTROL
save_foreign_type_list =
{
    SAVE_ID_FOREIGN_TYPE_LIST, DIALOG_MAIN_GROUP,
    { SAVE_ID_NAME, SAVE_ID_SELECTION },
    { 0, DIALOG_STDSPACING_V },
    { DRT(LBLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

#else

#define save_name_foreign save_name_normal

#endif /* FOREIGN_TYPE_LIST_BOX */

static DIALOG_CTL_CREATE
save_foreign_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &save_filetype_pict },      &save_filetype_pict_data    },
    { { &save_name_foreign },       &save_name_data             },
    { { &save_selection },          &save_selection_data        },
#if defined(FOREIGN_TYPE_LIST_BOX)
    { { &save_foreign_type_list },  &stdlisttext_data_dd        },
#endif

    { { &defbutton_ok },  &save_ok_data },
    { { &stdbutton_cancel }, &stdbutton_cancel_data }
};

static void
cmd_save_as_foreign_intro_resize_controls(
    _InRef_     PC_SAVE_CALLBACK p_save_callback)
{
#if defined(FOREIGN_TYPE_LIST_BOX)
    /* make appropriate size box */
    S32 show_elements = array_elements(&p_save_callback->h_save_filetype);
    PIXIT_SIZE list_size;
    DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
    dialog_cmd_ctl_size_estimate.p_dialog_control = &save_foreign_type_list;
    dialog_cmd_ctl_size_estimate.p_dialog_control_data.list_text = &stdlisttext_data_dd;
    ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
    show_elements = MAX(show_elements, 2);
    show_elements = MIN(show_elements, 8);
    ui_list_size_estimate(show_elements, &list_size);
    dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
    dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
    dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, SAVE_FOREIGN_LIST_H);
    save_foreign_type_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
    save_foreign_type_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
#else
    UNREFERENCED_PARAMETER_InRef_(p_save_callback);
#endif

#if defined(FOREIGN_TYPE_LIST_BOX)
    //save_filetype_pict.relative_offset[0] = -( (save_foreign_type_list.relative_offset[2] - save_filetype_pict.relative_offset[2]) / 2 ); /* yuk */
#else
    //save_filetype_pict.relative_offset[0] = -( (save_name_normal.relative_offset[2] - save_filetype_pict.relative_offset[2]) / 2 );
#endif
}

_Check_return_
static STATUS
cmd_save_as_foreign_intro_dialog(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    cmd_save_as_foreign_intro_resize_controls(p_save_callback);

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, save_foreign_ctl_create, elemof32(save_foreign_ctl_create), MSG_DIALOG_SAVE_AS_FOREIGN_CAPTION);
    dialog_cmd_process_dbox.help_topic_resource_id = MSG_DIALOG_SAVE_HELP_TOPIC;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_save_common;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_save_callback;
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
    } /*block*/
}

#endif /* RISCOS */

_Check_return_
static STATUS
filemap_create_for_cmd_save_as_foreign_intro(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
#if RISCOS || !defined(WINDOWS_SAVE_AS)
    P_UI_TEXT p_ui_text = &p_save_callback->ui_text;
    P_UI_SOURCE p_ui_source = &save_as_foreign_intro_statics.ui_source;
#else
    P_UI_TEXT p_ui_text = NULL;
    P_UI_SOURCE p_ui_source = NULL;
#endif

    status_return(filemap_create_h_save_filetype_for_cmd_save_as_foreign(p_save_callback));

#if defined(SAVE_FILETYPE_DESCRIPTION_SORTED)
    /* sort the textual representations list into order for neat display */
    al_array_qsort(&p_save_callback->h_save_filetype, save_filetype_description_compare_qsort);
#endif

    save_as_foreign_intro_statics.itemno = filemap_locate_within_h_save_filetype(p_save_callback, &save_as_foreign_intro_statics.t5_filetype);

    return(filemap_create_from_h_save_filetype_for_intro(p_save_callback, p_save_callback->saving_t5_filetype, p_ui_text, p_ui_source));
}

_Check_return_
static STATUS
cmd_save_as_foreign_intro_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback,
    _InVal_     T5_FILETYPE t5_filetype)
{
    status_return(cmd_save_as_foreign_init(p_docu, p_save_callback, t5_filetype));

    if(SAVING_AS_DRAW == p_save_callback->saving)
        cmd_save_as_intro_init_page_range(p_docu, p_save_callback, tstr_empty_string);

    return(filemap_create_for_cmd_save_as_foreign_intro(p_save_callback));
}

static inline void
cmd_save_as_foreign_intro_finalise(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    ui_source_dispose(&save_as_foreign_intro_statics.ui_source);

    ui_text_dispose(&p_save_callback->ui_text);

    WINDOWS_ONLY(ui_text_dispose(&p_save_callback->initial_directory));
    ui_text_dispose(&p_save_callback->initial_filename);
    ui_text_dispose(&p_save_callback->saving_filename);

    al_array_dispose(&p_save_callback->h_save_filetype);
}

#if WINDOWS

static void
cmd_save_as_foreign_intro_set_initial_directory(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
#if 1
    TCHARZ initial_directory[BUF_MAX_PATHSTRING];
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
    quick_tblock_with_buffer_setup(quick_tblock);

    p_save_callback->initial_directory.type = UI_TEXT_TYPE_NONE;

    status_assert(name_make_wholename(&p_docu->docu_name, &quick_tblock, TRUE));
    if(file_dirname(initial_directory, quick_tblock_tstr(&quick_tblock)))
        status_assert(ui_text_alloc_from_tstr(&p_save_callback->initial_directory, initial_directory));

    quick_tblock_dispose(&quick_tblock);
#else
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    cmd_save_as_intro_init_set_initial_directory(p_save_callback);
#endif
}

#endif /* WINDOWS */

_Check_return_
static STATUS
cmd_save_as_foreign_intro(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_FILETYPE t5_filetype,
    _In_opt_z_  PCTSTR suggested_leafname)
{
    SAVE_CALLBACK save_callback;
    STATUS status;

    /* create a list of filetypes and textual representations thereof from the loaded objects */
    status_return(cmd_save_as_foreign_intro_init(p_docu, &save_callback, t5_filetype));

    if(PTR_IS_NULL(suggested_leafname))
    {
        save_callback.initial_filename.type = UI_TEXT_TYPE_RESID;
        save_callback.initial_filename.text.resource_id = MSG_DIALOG_SAVE_AS_FOREIGN_NAME;
    }
    else
    {
        status_return(ui_text_alloc_from_tstr(&save_callback.initial_filename, suggested_leafname));
    }

    WINDOWS_ONLY(cmd_save_as_foreign_intro_set_initial_directory(p_docu, &save_callback));

    status = RISCOS_OR_WINDOWS(cmd_save_as_foreign_intro_dialog, windows_save_as) (p_docu, &save_callback);

    cmd_save_as_foreign_intro_finalise(&save_callback);

    return(status);
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_as_foreign_intro)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    PCTSTR suggested_leafname = NULL;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    assert(p_args[0].type == (ARG_TYPE_X32 | ARG_MANDATORY));

    if(n_arglist_args(&p_t5_cmd->arglist_handle) > 1)
    {
        assert(p_args[1].type == (ARG_TYPE_TSTR));
        suggested_leafname = p_args[1].val.tstr;
    }

    if(FILETYPE_DRAW == p_args[0].val.t5_filetype)
        return(cmd_save_as_drawfile_intro(p_docu, suggested_leafname));

    return(cmd_save_as_foreign_intro(p_docu, p_args[0].val.t5_filetype, suggested_leafname));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_as_foreign_intro)

/*
SAVE AS DRAWFILE - INTRO CMD
*/

#define save_as_drawfile_intro_set_initial_directory save_as_foreign_intro_set_initial_directory

#if RISCOS || !defined(WINDOWS_SAVE_AS)

#define save_drawfile_ctl_create save_hybrid_draw_ctl_create

_Check_return_
static STATUS
cmd_save_as_drawfile_intro_dialog(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, save_drawfile_ctl_create, elemof32(save_drawfile_ctl_create), MSG_DIALOG_SAVE_AS_DRAWFILE_CAPTION);
  /*dialog_cmd_process_dbox.help_topic_resource_id = 0;*/
    dialog_cmd_process_dbox.p_proc_client = dialog_event_save_common;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_save_callback;
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

#endif /* RISCOS */

static void
cmd_save_as_drawfile_intro_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback,
    _In_opt_z_  PCTSTR suggested_leafname)
{
    cmd_save_as_drawfile_init(p_docu, p_save_callback);

    if(PTR_IS_NULL(suggested_leafname))
    {
        p_save_callback->initial_filename.type = UI_TEXT_TYPE_RESID;
        p_save_callback->initial_filename.text.resource_id = MSG_DIALOG_SAVE_AS_DRAWFILE_NAME;
    }
    else
    {
        status_assert(ui_text_alloc_from_tstr(&p_save_callback->initial_filename, suggested_leafname));
    }

    WINDOWS_ONLY(cmd_save_as_foreign_intro_set_initial_directory(p_docu, p_save_callback));

#if RISCOS || !defined(WINDOWS_SAVE_AS)
    //save_filetype_pict.relative_offset[0] = -( (save_name_normal.relative_offset[2] - save_filetype_pict.relative_offset[2]) / 2 );
#endif
}

_Check_return_
static STATUS
cmd_save_as_drawfile_intro(
    _DocuRef_   P_DOCU p_docu,
    _In_opt_z_  PCTSTR suggested_leafname)
{
    SAVE_CALLBACK save_callback;

    cmd_save_as_drawfile_intro_init(p_docu, &save_callback, suggested_leafname);

    return(RISCOS_OR_WINDOWS(cmd_save_as_drawfile_intro_dialog, windows_save_as) (p_docu, &save_callback));
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_as_drawfile_intro)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    PCTSTR suggested_leafname = NULL;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        assert(p_args[0].type == (ARG_TYPE_TSTR));
        suggested_leafname = p_args[0].val.tstr;
    }

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    return(cmd_save_as_drawfile_intro(p_docu, suggested_leafname));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_as_drawfile_intro)

/*
SAVE PICTURE - INTRO CMD
*/

#if RISCOS || !defined(WINDOWS_SAVE_AS)

static const DIALOG_CONTROL
save_name_picture =
{
    SAVE_ID_NAME, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, SAVE_ID_FILETYPE_PICT, SAVE_ID_PICTURE_TYPE_LIST },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V },
    { DRT(LBRT, EDIT), 1 /*tabstop*/ }
};

/*
* list of types of file we can export picture as
*/

static /*poked*/ DIALOG_CONTROL
save_picture_type_list =
{
    SAVE_ID_PICTURE_TYPE_LIST, DIALOG_MAIN_GROUP,
    { SAVE_ID_NAME, SAVE_ID_NAME },
    { 0, DIALOG_STDSPACING_V },
    { DRT(LBLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static DIALOG_CTL_CREATE
save_picture_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &save_filetype_pict },      &save_filetype_pict_data    }, /*[1]*/
    { { &save_name_picture },       &save_name_data             }, /*[2]*/
    { { &save_picture_type_list },  &stdlisttext_data_dd        }, /*[3]*/

    { { &defbutton_ok }, &save_ok_data },
    { { &stdbutton_cancel }, &stdbutton_cancel_data }
};

#endif /* RISCOS */

_Check_return_ _Success_(status_ok(return))
static STATUS
filemap_create_for_cmd_save_picture_intro(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback,
    _OutRef_opt_ P_UI_TEXT p_ui_text_dst,
    _InoutRef_opt_ P_UI_SOURCE p_ui_source_dst)
{
    STATUS status;

    status_return(status = filemap_create_h_save_filetype_for_cmd_save_picture(p_save_callback));

    { /* supply a suitable default for filetypes with no description override */
    const ARRAY_INDEX filemap_elements = array_elements(&p_save_callback->h_save_filetype);
    ARRAY_INDEX i;
    P_SAVE_FILETYPE p_save_filetype = array_range(&p_save_callback->h_save_filetype, SAVE_FILETYPE, 0, filemap_elements);
    for(i = 0; i < filemap_elements; ++i, ++p_save_filetype)
    {
        if(ui_text_is_blank(&p_save_filetype->description))
        {
            BOOL fFound_description;
            PC_USTR ustr_description = description_ustr_from_t5_filetype(p_save_filetype->t5_filetype, &fFound_description);
            p_save_filetype->description.type = UI_TEXT_TYPE_USTR_TEMP;
            if(fFound_description)
                p_save_filetype->description.text.ustr = ustr_description;
            else
            {
                const P_DOCU p_docu = p_docu_from_docno(p_save_callback->docno);
                status_break(status = alloc_block_ustr_set(&p_save_filetype->description.text.ustr_wr, ustr_description, &p_docu->general_string_alloc_block));
            }
        }

        if(ui_text_is_blank(&p_save_filetype->suggested_leafname))
        {
        }
    }
    } /*block*/

    if(status_fail(status))
    {
        al_array_dispose(&p_save_callback->h_save_filetype);
        return(status);
    }

#if defined(SAVE_FILETYPE_DESCRIPTION_SORTED)
    /* sort the textual representations list into order for neat display */
    al_array_qsort(&p_save_callback->h_save_filetype, save_filetype_description_compare_qsort);
#endif

    save_picture_intro_statics.itemno = filemap_locate_within_h_save_filetype(p_save_callback, &save_picture_intro_statics.t5_filetype);

    return(filemap_create_from_h_save_filetype_for_intro(p_save_callback, p_save_callback->saving_t5_filetype, p_ui_text_dst, p_ui_source_dst));
}

#if RISCOS || !defined(WINDOWS_SAVE_AS)

static inline void
cmd_save_picture_intro_dialog_resize_controls(
    _InRef_     P_SAVE_CALLBACK p_save_callback)
{
    /* make appropriate size box */
    S32 show_elements = array_elements(&p_save_callback->h_save_filetype);
    PIXIT_SIZE list_size;
    DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
    dialog_cmd_ctl_size_estimate.p_dialog_control = &save_picture_type_list;
    dialog_cmd_ctl_size_estimate.p_dialog_control_data.list_text = &stdlisttext_data_dd;
    ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
    show_elements = MAX(show_elements, 2);
    show_elements = MIN(show_elements, 8);
    ui_list_size_estimate(show_elements, &list_size);
    dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
    dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
    dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, SAVE_PICTURE_LIST_H);
    save_picture_type_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
    save_picture_type_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;

    //save_filetype_pict.relative_offset[0] = -( (save_picture_type_list.relative_offset[2] - save_filetype_pict.relative_offset[2]) / 2 ); /* yuk */
}

#endif /*RISCOS*/

#if RISCOS

_Check_return_
static STATUS
cmd_save_picture_intro_dialog(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    cmd_save_picture_intro_dialog_resize_controls(p_save_callback);

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, save_picture_ctl_create, elemof32(save_picture_ctl_create), MSG_DIALOG_SAVE_PICTURE_CAPTION);
    dialog_cmd_process_dbox.help_topic_resource_id = MSG_DIALOG_SAVE_PICTURE_HELP_TOPIC;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_save_common;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_save_callback;
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
    } /*block*/
}

#endif /*RISCOS*/

/*
SAVE PICTURE - INTRO CMD
*/

#if WINDOWS

static void
cmd_save_picture_intro_init_set_initial_directory(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    BOOL name_set = FALSE;
    TCHARZ initial_directory[BUF_MAX_PATHSTRING];
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
    quick_tblock_with_buffer_setup(quick_tblock);

    p_save_callback->initial_directory.type = UI_TEXT_TYPE_NONE;

    status_assert(name_make_wholename(&p_docu->docu_name, &quick_tblock, TRUE));
    if(file_dirname(initial_directory, name_set ? ui_text_tstr(&p_save_callback->initial_filename) : quick_tblock_tstr(&quick_tblock)))
        status_assert(ui_text_alloc_from_tstr(&p_save_callback->initial_directory, initial_directory));

    quick_tblock_dispose(&quick_tblock);
}

#endif /* WINDOWS */

_Check_return_
static STATUS
cmd_save_picture_intro_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback)
{
    STATUS status;

    if(!status_done(status = cmd_save_picture_init(p_docu, p_save_callback, save_picture_intro_statics.t5_filetype)))
        return(status);

    save_picture_intro_statics.itemno = filemap_locate_within_h_save_filetype(p_save_callback, &save_picture_intro_statics.t5_filetype);

    p_save_callback->saving_t5_filetype = save_picture_intro_statics.t5_filetype;

    /* create a list of textual representations of filetypes from the loaded objects */
    status_return(filemap_create_for_cmd_save_picture_intro(p_save_callback, &p_save_callback->ui_text, &save_picture_intro_statics.ui_source));

    /* encode initial state of control(s) */
    ui_text_init_resid(&p_save_callback->initial_filename, MSG_DIALOG_SAVE_PICTURE_NAME);
    if(!ui_text_is_blank(&p_save_callback->ui_text))
        status_assert(ui_text_copy(&p_save_callback->initial_filename, &p_save_callback->ui_text));

    WINDOWS_ONLY(cmd_save_picture_intro_init_set_initial_directory(p_docu, p_save_callback));

    return(STATUS_DONE);
}

static void
cmd_save_picture_intro_finalise(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    ui_source_dispose(&save_picture_intro_statics.ui_source);

    ui_text_dispose(&p_save_callback->ui_text);

    WINDOWS_ONLY(ui_text_dispose(&p_save_callback->initial_directory));
    ui_text_dispose(&p_save_callback->initial_filename);
    ui_text_dispose(&p_save_callback->saving_filename);

    al_array_dispose(&p_save_callback->h_save_filetype);
}

_Check_return_
static STATUS
cmd_save_picture_intro(
    _DocuRef_   P_DOCU p_docu)
{
    SAVE_CALLBACK save_callback;
    STATUS status;

    if(!object_present(OBJECT_ID_NOTE))
        return(status_check()); /* no note(s) can be selected, give up now */

    if(!status_done(status = cmd_save_picture_intro_init(p_docu, &save_callback)))
    {
        assert(status_fail(status));
        return(status);
    }

    status = RISCOS_OR_WINDOWS(cmd_save_picture_intro_dialog, windows_save_as) (p_docu, &save_callback);

    cmd_save_picture_intro_finalise(&save_callback);

    return(status);
}

T5_CMD_PROTO(static, ccba_wrapped_t5_cmd_save_picture_intro)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    return(cmd_save_picture_intro(p_docu));
}

CCBA_WRAP_T5_CMD(extern, t5_cmd_save_picture_intro)

_Check_return_
static T5_FILETYPE
which_t5_filetype(
    _DocuRef_   P_DOCU p_docu)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(FILETYPE_T5_HYBRID_DRAW == p_docu->docu_preferred_filetype)
    {
        if(!object_available(OBJECT_ID_FS_OLE))
        {   /* winge here and mutate both to caller and persistently */
            reperr_null(ERR_WILL_SAVE_AS_FIREWORKZ);
            p_docu->docu_preferred_filetype = FILETYPE_T5_FIREWORKZ;
        }
    }
    else
    /* surely we can render a database as Draw? sounds useful... */
    {
        if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
            return(FILETYPE_T5_RECORDZ);
    }

    return(p_docu->docu_preferred_filetype);
}

/*
locate directory
*/

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

static const DIALOG_CONTROL
locate_waffle_5 =
{
    SAVE_ID_WAFFLE_5, DIALOG_CONTROL_WINDOW,
    { SAVE_ID_WAFFLE_4, SAVE_ID_WAFFLE_4 },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDTEXT_V },
    { DRT(LBLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
locate_waffle_5_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_LOCATE_TEMPLATE_WAFFLE_5), { 1 /*left_text*/ } };

static const DIALOG_CTL_CREATE
locate_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &locate_waffle_1 }, &locate_waffle_1_data },
    { { &locate_waffle_2 }, &locate_waffle_2_data },
    { { &locate_waffle_3 }, &locate_waffle_3_data },
    { { &locate_waffle_4 }, &locate_waffle_4_data },
    { { &locate_waffle_5 }, &locate_waffle_5_data },

    { { &save_filetype_pict }, &save_filetype_pict_data },
    { { &save_name_normal }, &save_name_data },

    { { &defbutton_ok }, &save_ok_data },
    { { &stdbutton_cancel }, &stdbutton_cancel_data }
};

#endif /* RISCOS */

static PTSTR g_dirname_buffer;
static U32 g_elemof_dirname_buffer;

_Check_return_
static STATUS
save_template_locate(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    STATUS status = STATUS_OK;
    const PCTSTR dirname = ui_text_tstr(&p_save_callback->saving_filename);

#if RISCOS
    _kernel_swi_regs rs;
    _kernel_oserror * e;
    rs.r[0] = OSFile_ReadNoPath;
    rs.r[1] = (intptr_t) dirname;
    if(NULL != (e = WrapOsErrorChecking(_kernel_swi(OS_File, &rs, &rs))))
        return(file_error_set(e->errmess));
    if(rs.r[0] != 0)
        return(create_error(ERR_TEMPLATE_DEST_EXISTS));

    rs.r[0] = 26;
    rs.r[1] = (intptr_t) p_save_callback->initial_template_dir;
    rs.r[2] = (intptr_t) dirname;
    rs.r[3] = 0x5603; /*~a~c~dfln~p~qrs~t~v*/
    if(NULL != (e = WrapOsErrorChecking(_kernel_swi(OS_FSControl, &rs, &rs))))
        return(file_error_set(e->errmess));
#endif

    tstr_xstrkpy(g_dirname_buffer, g_elemof_dirname_buffer, dirname);

    return(status);
}

static void
locate_copy_of_dir_template_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SAVE_CALLBACK p_save_callback,
    _In_z_      PCTSTR filename_template)
{
    save_callback_init(p_docu, p_save_callback, FILETYPE_DIRECTORY);

    p_save_callback->saving = LOCATING_TEMPLATE;

    p_save_callback->initial_template_dir = filename_template;

    p_save_callback->initial_filename.type = UI_TEXT_TYPE_TSTR_PERM;
    p_save_callback->initial_filename.text.tstr = file_leafname(p_save_callback->initial_template_dir);

#if RISCOS
    //save_filetype_pict.relative_offset[0] = -( (save_name_normal.relative_offset[2] - save_filetype_pict.relative_offset[2]) / 2 );
#endif
}

_Check_return_
static STATUS
locate_copy_of_dir_template_dialog(
    _DocuRef_   P_DOCU cur_p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
#if RISCOS
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, locate_ctl_create, elemof32(locate_ctl_create), MSG_DIALOG_LOCATE_TEMPLATE_CAPTION);
#else
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, NULL, 0, MSG_DIALOG_LOCATE_TEMPLATE_CAPTION);
#endif
  /*dialog_cmd_process_dbox.help_topic_resource_id = 0;*/
    dialog_cmd_process_dbox.p_proc_client = dialog_event_save_common;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) p_save_callback;
    return(object_call_DIALOG_with_docu(cur_p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
}

static void
locate_copy_of_dir_template_finalise(
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    ui_text_dispose(&p_save_callback->initial_filename);
    ui_text_dispose(&p_save_callback->saving_filename);
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

    g_dirname_buffer = dirname_buffer;
    g_elemof_dirname_buffer = elemof_dirname_buffer;

    dirname_buffer[0] = CH_NULL;

    locate_copy_of_dir_template_init(cur_p_docu, &save_callback, filename_template);

    status = locate_copy_of_dir_template_dialog(cur_p_docu, &save_callback);

    locate_copy_of_dir_template_finalise(&save_callback);

    return(status);
}

#if WINDOWS

/*
WINDOWS SAVE AS - INTRO CMD HELPERS
*/

_Ret_z_ _Check_return_
static PCTSTR
default_extension_from_save_callback(
    P_SAVE_CALLBACK p_save_callback)
{
    switch(p_save_callback->saving)
    {
    case SAVING_DOCUMENT:
        return(extension_document_tstr);

    case SAVING_HYBRID_DRAW:
        return(extension_hybrid_draw_tstr);

    case SAVING_AS_TEMPLATE:
        return(extension_template_tstr);

    case SAVING_AS_DRAW:
    case SAVING_PICTURE:
        return(TEXT("aff")); /* without the preceding dot */

    default: default_unhandled();
    case SAVING_AS_FOREIGN:
        return(TEXT("txt"));
    }
}

static void
windows_save_as_set_dirname(
    _Out_writes_z_(elemof_buffer) PTSTR szDirName,
    _InVal_     U32 elemof_buffer,
    _InRef_     P_SAVE_CALLBACK p_save_callback)
{
    if(!ui_text_is_blank(&p_save_callback->initial_directory))
    {
        tstr_xstrkpy(szDirName, elemof_buffer, ui_text_tstr(&p_save_callback->initial_directory));
    }
    else if(0 == MyGetProfileString(TEXT("DefaultDirectory"), tstr_empty_string, szDirName, elemof_buffer))
    {
        if(!GetPersonalDirectoryName(szDirName, elemof_buffer))
        {
            if(GetModuleFileName(GetInstanceHandle(), szDirName, elemof_buffer))
            {
                file_dirname(szDirName, szDirName);
            }
        }
    }

    { /* tidy up what we found */
    U32 len = tstrlen32(szDirName);
    if(len > elemof32(TEXT("C:") FILE_DIR_SEP_TSTR)-1)
    {
        PTSTR tstr = szDirName + len;
        if(*--tstr == FILE_DIR_SEP_CH)
            *tstr = CH_NULL;
    }
    } /*block*/
}

/*
SaveFileHook (SFH_ prefixed functions)
*/

#include <dlgs.h>

#if 0
static struct SaveFileHook_statics
{
    P_SAVE_CALLBACK p_save_callback;
}
SaveFileHook_statics;
#endif

_Check_return_
static inline HWND
SFH_GetDlgItem(
    _InRef_     P_SAVE_CALLBACK p_save_callback,
    _InVal_     int nIDDlgItem)
{
    return(GetDlgItem(p_save_callback->hdlg, nIDDlgItem));
}

/*
Selection checkbox
*/

static void
SFH_SAVE_SELECTION_enable(
    _InRef_     P_SAVE_CALLBACK p_save_callback)
{
    const P_DOCU p_docu = p_docu_from_docno(p_save_callback->docno);
    BOOL enable;

    switch(p_save_callback->saving)
    {
    case SAVING_HYBRID_DRAW:
    case SAVING_AS_TEMPLATE:
    case SAVING_AS_DRAW:
        enable = FALSE;
        break;

    default:
        enable = (0 != p_docu->mark_info_cells.h_markers);
        break;
    }

    Button_Enable(SFH_GetDlgItem(p_save_callback, IDC_CHECK_X_SAVE_SELECTION), enable);
}

static void
SFH_SAVE_SELECTION_encode(
    _InRef_     P_SAVE_CALLBACK p_save_callback)
{
    Button_SetCheck(SFH_GetDlgItem(p_save_callback, IDC_CHECK_X_SAVE_SELECTION), p_save_callback->save_selection);
}

static void
SFH_SAVE_SELECTION_localise(
    _InRef_     P_SAVE_CALLBACK p_save_callback)
{
    Button_SetText(SFH_GetDlgItem(p_save_callback, IDC_CHECK_X_SAVE_SELECTION), resource_lookup_tstr(MSG_SELECTION));
}

static void
SFH_SAVE_SELECTION_visibility(
    _InRef_     P_SAVE_CALLBACK p_save_callback)
{
    BOOL visible;

    switch(p_save_callback->saving)
    {
    case SAVING_HYBRID_DRAW:
    case SAVING_AS_TEMPLATE:
    case SAVING_AS_DRAW:
        visible = FALSE;
        break;

    default:
        visible = TRUE;
        break;
    }

    ShowWindow(SFH_GetDlgItem(p_save_callback, IDC_CHECK_X_SAVE_SELECTION), visible ? SW_SHOW : SW_HIDE);
}

/*
Page range edit
*/

static void
SFH_SAVE_PAGE_RANGE_enable(
    _InRef_     P_SAVE_CALLBACK p_save_callback)
{
    BOOL enable;

    switch(p_save_callback->saving)
    {
    case SAVING_HYBRID_DRAW:
    case SAVING_AS_DRAW:
        enable = TRUE;
        break;

    default:
        enable = FALSE;
        break;
    }

    Static_Enable(SFH_GetDlgItem(p_save_callback, IDC_STATIC_X_SAVE_PAGE_RANGE_LABEL), enable);
    Edit_Enable(SFH_GetDlgItem(p_save_callback, IDC_EDIT_X_SAVE_PAGE_RANGE), enable);
}

static void
SFH_SAVE_PAGE_RANGE_encode(
    _InRef_     P_SAVE_CALLBACK p_save_callback)
{
    const PCTSTR tstr = ui_text_tstr(&p_save_callback->page_range_list);

    Edit_SetText(SFH_GetDlgItem(p_save_callback, IDC_EDIT_X_SAVE_PAGE_RANGE), tstr ? tstr : tstr_empty_string);
}

static void
SFH_SAVE_PAGE_RANGE_visibility(
    _InRef_     P_SAVE_CALLBACK p_save_callback)
{
    BOOL visible;

    switch(p_save_callback->saving)
    {
    case SAVING_HYBRID_DRAW:
    case SAVING_AS_DRAW:
        visible = TRUE;
        break;

    default:
        visible = FALSE;
        break;
    }

    ShowWindow(SFH_GetDlgItem(p_save_callback, IDC_EDIT_X_SAVE_PAGE_RANGE), visible ? SW_SHOW : SW_HIDE);
    ShowWindow(SFH_GetDlgItem(p_save_callback, IDC_STATIC_X_SAVE_PAGE_RANGE_LABEL), visible ? SW_SHOW : SW_HIDE);
}

static void
SFH_SAVE_PAGE_RANGE_LABEL_localise(
    _InRef_     P_SAVE_CALLBACK p_save_callback)
{
    TCHARZ tstr_buffer[32];

    resource_lookup_tstr_buffer(tstr_buffer, elemof32(tstr_buffer), MSG_DIALOG_PRINT_PAGES);
    tstr_xstrkat(tstr_buffer, elemof32(tstr_buffer), TEXT(":"));

    Static_SetText(SFH_GetDlgItem(p_save_callback, IDC_STATIC_X_SAVE_PAGE_RANGE_LABEL), tstr_buffer); 
}

static void
SFH_saving_t5_filetype_changed(
    _InRef_     P_SAVE_CALLBACK p_save_callback)
{
    /* may need to mutate the SAVING_XXX as we might have started out with some other intent */
    switch(p_save_callback->saving_t5_filetype)
    {
    case FILETYPE_T5_FIREWORKZ: /* No need to worry about other T5 filetypes on Windows */
    case FILETYPE_T5_HYBRID_DRAW:
        cmd_save_init_set_saving(p_save_callback);
        break;

    case FILETYPE_T5_TEMPLATE:
        cmd_save_as_template_init_set_saving(p_save_callback);
        break;

    case FILETYPE_DRAW:
    default:
        if(SAVING_PICTURE != p_save_callback->saving)
            cmd_save_as_foreign_init_set_saving(p_save_callback);
        break;
    }

    SFH_SAVE_SELECTION_enable(p_save_callback);
    SFH_SAVE_PAGE_RANGE_enable(p_save_callback);

    SFH_SAVE_SELECTION_visibility(p_save_callback);
    SFH_SAVE_PAGE_RANGE_visibility(p_save_callback);
}

_Check_return_
_Ret_opt_z_
static PTSTR /* needs to be freed */
SFH_filetype_changed_get_filter(
    _HwndRef_   HWND hwnd_filter_combo,
    _In_        int nFilterIndex,
    _OutRef_    P_STATUS p_status)
{
    DWORD len_filter = ComboBox_GetLBTextLen(hwnd_filter_combo, nFilterIndex);
    PTSTR tstr_filter = NULL;

    if(CB_ERR != len_filter)
    {
        len_filter += 1 /*for CH_NULL*/;

        tstr_filter = al_ptr_alloc_elem(TCHARZ, len_filter, p_status);

        if(NULL != tstr_filter)
            (void) ComboBox_GetLBText(hwnd_filter_combo, nFilterIndex, tstr_filter);
    }

    return(tstr_filter);
}

_Check_return_
_Ret_opt_z_
static PTSTR
SFH_filetype_changed_find_extension(
    _InoutRef_  PTSTR tstr_filter)
{
    static const TCHARZ test[] = TEXT("(*.");
    PTSTR tstr_extension = tstrstr(tstr_filter, test);
    PTSTR tstr_end;

    if(NULL== tstr_extension)
        return(NULL);

    tstr_extension += elemof32(test)-1;

    /* NB extension ends at CH_RIGHT_PARENTHESIS or the first CH_COMMA */
    if(NULL != (tstr_end = tstrpbrk(tstr_extension, TEXT(",)"))))
        *tstr_end = CH_NULL;

    return(tstr_extension);
}

static void
SFH_filetype_changed_update_edit(
    _HwndRef_   HWND hwnd_edit,
    _In_z_      PCTSTR tstr_newext,
    _OutRef_    P_STATUS p_status)
{
    DWORD len_edit = Edit_GetTextLength(hwnd_edit);
    DWORD len_add = 1 /*for CH_NULL*/ + 1 /*for .*/ + tstrlen(tstr_newext) /*may need .exth sticking on the end too*/;
    PTSTR tstr_edit;

    if(0 == len_edit)
        return;

    len_edit += len_add;

    if(NULL != (tstr_edit = al_ptr_alloc_elem(TCHARZ, len_edit, p_status)))
    {
        BOOL ok;
        PTSTR tstr_curext;
        Edit_GetText(hwnd_edit, tstr_edit, (int) len_edit);
        tstr_curext = file_extension(tstr_edit);
        if(NULL == tstr_curext)
        {
            tstr_curext = tstr_edit + tstrlen32(tstr_edit);
            *tstr_curext++ = FILE_EXT_SEP_CH;
            --len_add;
        }
        /* copy over the new extension */
        ok = (0 == _tcscpy_s(tstr_curext, len_add, tstr_newext));
        assert(ok);
        if(ok)
            Edit_SetText(hwnd_edit, tstr_edit);
        tstr_clr(&tstr_edit);
    }
}

static void
SFH_filetype_changed_update_filename_combo(
    _HwndRef_   HWND hwnd_filename_combo,
    _In_z_      PCTSTR tstr_newext,
    _OutRef_    P_STATUS p_status)
{
    DWORD len_filename_combo = ComboBox_GetTextLength(hwnd_filename_combo);
    DWORD len_add = 1 /*for CH_NULL*/ + 1 /*for .*/ + tstrlen(tstr_newext) /*may need .exth sticking on the end too*/;
    PTSTR tstr_filename_combo;

    *p_status = STATUS_OK;

    if(0 == len_filename_combo)
        return;

    len_filename_combo += len_add;

    if(NULL != (tstr_filename_combo = al_ptr_alloc_elem(TCHARZ, len_filename_combo, p_status)))
    {
        BOOL ok;
        PTSTR tstr_curext;
        ComboBox_GetText(hwnd_filename_combo, tstr_filename_combo, (int) len_filename_combo);
        tstr_curext = file_extension(tstr_filename_combo);
        if(NULL == tstr_curext)
        {
            tstr_curext = tstr_filename_combo + tstrlen32(tstr_filename_combo);
            *tstr_curext++ = FILE_EXT_SEP_CH;
            --len_add;
        }
        /* copy over the new extension */
        ok = (0 == _tcscpy_s(tstr_curext, len_add, tstr_newext));
        assert(ok);
        if(ok)
            ComboBox_SetText(hwnd_filename_combo, tstr_filename_combo);
        tstr_clr(&tstr_filename_combo);
    }
}

static void
SFH_filetype_changed(
    _HwndRef_   HWND hdlg,      /* handle to child dialog window NOT the 'Save As' window!!! */
    _HwndRef_   HWND hwnd_filter_combo,
    _In_        int nFilterIndex,
    _HwndRef_opt_ HWND hwnd_edit,
    _HwndRef_opt_ HWND hwnd_filename_combo)
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) GetWindowLongPtr(hdlg, GWLP_USERDATA);
    STATUS status = STATUS_OK;
    PTSTR tstr_filter;

    if(NULL != (tstr_filter = SFH_filetype_changed_get_filter(hwnd_filter_combo, nFilterIndex, &status)))
    {
        PTSTR tstr_newext = SFH_filetype_changed_find_extension(tstr_filter);

        if(NULL != tstr_newext)
        {   /* replace any current extension with what is enclosed here */
            /* wot about (*.bmp,*dib) - just the first */
            if(NULL != hwnd_edit)
                SFH_filetype_changed_update_edit(hwnd_edit, tstr_newext, &status);
            else if(NULL != hwnd_filename_combo)
                SFH_filetype_changed_update_filename_combo(hwnd_filename_combo, tstr_newext, &status);

            p_save_callback->saving_t5_filetype = t5_filetype_from_extension(tstr_newext);
            SFH_saving_t5_filetype_changed(p_save_callback);
        }

        tstr_clr(&tstr_filter);
    }
    status_assert(status);
}

/* Hook procedure */

static void
SaveFileHook_onNotify_CDN_INITDONE(
    _HwndRef_   HWND hdlg,      /* handle to child dialog window NOT the 'Save As' window!!! */
    _InRef_     LPOFNOTIFY lpon)
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) lpon->lpOFN->lCustData;

    p_save_callback->hdlg = hdlg;

    SetWindowLongPtr(hdlg, GWLP_USERDATA, (LONG) p_save_callback);

    /* Selection checkbox */
    SFH_SAVE_SELECTION_localise(p_save_callback);
    SFH_SAVE_SELECTION_enable(p_save_callback);
    SFH_SAVE_SELECTION_encode(p_save_callback);

    /* Page range edit control */
    SFH_SAVE_PAGE_RANGE_LABEL_localise(p_save_callback);
    SFH_SAVE_PAGE_RANGE_enable(p_save_callback);
    SFH_SAVE_PAGE_RANGE_encode(p_save_callback);

    SFH_SAVE_SELECTION_visibility(p_save_callback);
    SFH_SAVE_PAGE_RANGE_visibility(p_save_callback);
}

static void
SaveFileHook_onNotify_CDN_FILEOK(
    _HwndRef_   HWND hdlg,      /* handle to child dialog window NOT the 'Save As' window!!! */
    _InRef_     LPOFNOTIFY lpon)
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) lpon->lpOFN->lCustData;
    HWND hwndCtl;
    /* use nFilterIndex to determine file type to save as */
    const S32 filter_mask = (SAVING_PICTURE == p_save_callback->saving) ? BOUND_FILETYPE_WRITE_PICT : BOUND_FILETYPE_WRITE;
    assert(p_save_callback->saving_t5_filetype == windows_filter_list_get_t5_filetype_from_filter_index(lpon->lpOFN->nFilterIndex, filter_mask, &p_save_callback->h_save_filetype_filters));
    p_save_callback->saving_t5_filetype = windows_filter_list_get_t5_filetype_from_filter_index(lpon->lpOFN->nFilterIndex, filter_mask, &p_save_callback->h_save_filetype_filters);

    assert(hdlg == p_save_callback->hdlg);
    UNREFERENCED_PARAMETER_HwndRef_(hdlg);

    /* Selection checkbox */
    hwndCtl = SFH_GetDlgItem(p_save_callback, IDC_CHECK_X_SAVE_SELECTION);
    p_save_callback->save_selection = IsWindowEnabled(hwndCtl) && Button_GetCheck(hwndCtl);

    /* Page range edit control */
    hwndCtl = SFH_GetDlgItem(p_save_callback, IDC_EDIT_X_SAVE_PAGE_RANGE);
    if(IsWindowEnabled(hwndCtl))
    {
        const P_UI_TEXT p_ui_text = &p_save_callback->page_range_list;
        U32 tstr_len = GetWindowTextLength(hwndCtl) + 1 /*CH_NULL*/;
        STATUS status;
        PTSTR tstr;

        ui_text_dispose(p_ui_text);

        p_ui_text->type = UI_TEXT_TYPE_NONE;

        if(NULL != (tstr = al_array_alloc_TCHAR(&p_ui_text->text.array_handle_tstr, tstr_len, &array_init_block_tchar, &status)))
        {
            p_ui_text->type = UI_TEXT_TYPE_TSTR_ARRAY;

            GetWindowText(hwndCtl, tstr, (int) tstr_len);
        }
    }
}

static void
SaveFileHook_onNotify_CDN_TYPECHANGE(
    _HwndRef_   HWND hdlg,      /* handle to child dialog window NOT the 'Save As' window!!! */
    _InRef_     LPOFNOTIFY lpon)
{
    const HWND hdlg_save = GetParent(hdlg); /* *This* is the 'Save As' window handle */
    const int nFilterIndex = lpon->lpOFN->nFilterIndex - 1;
    const HWND hwnd_filter_combo = GetDlgItem(hdlg_save, cmb1 /*1136*/);
    const HWND hwnd_filename_combo = GetDlgItem(hdlg_save, cmb13); /* This takes precedence. Possibly. */
    const HWND hwnd_edit = hwnd_filename_combo ? NULL : GetDlgItem(hdlg_save, edt1 /*1152*/);
    SFH_filetype_changed(hdlg, hwnd_filter_combo, nFilterIndex, hwnd_edit, hwnd_filename_combo);
}

static void
SaveFileHook_onNotify(
    _HwndRef_   HWND hdlg,      /* handle to child dialog window NOT the 'Save As' window!!! */
    _InRef_     LPOFNOTIFY lpon)
{
    switch(lpon->hdr.code)
    {
    case CDN_INITDONE:
        SaveFileHook_onNotify_CDN_INITDONE(hdlg, lpon);
        break;

    case CDN_SELCHANGE:
        break;

    case CDN_FOLDERCHANGE:
        break;

    case CDN_SHAREVIOLATION:
        break;

    case CDN_HELP:
        break;

    case CDN_FILEOK:
        SaveFileHook_onNotify_CDN_FILEOK(hdlg, lpon);
        break;

    case CDN_TYPECHANGE:
        SaveFileHook_onNotify_CDN_TYPECHANGE(hdlg, lpon);
        break;

    case CDN_INCLUDEITEM:
        break;

    default: /*default_unhandled();*/
        break;
    }
}

_Ret_opt_z_ _Check_return_
static PTSTR
SFH_filename_combo_GetText(
    _HwndRef_   HWND hwnd_filename_combo,
    _OutRef_    P_STATUS p_status)
{
    DWORD len_filename_combo = ComboBox_GetTextLength(hwnd_filename_combo);
    DWORD len_add = 1 /*for CH_NULL*/;
    PTSTR tstr_filename_combo;

    *p_status = STATUS_OK;

    if(0 == len_filename_combo)
        return(NULL);

    len_filename_combo += len_add;

    if(NULL != (tstr_filename_combo = al_ptr_alloc_elem(TCHARZ, len_filename_combo, p_status)))
        ComboBox_GetText(hwnd_filename_combo, tstr_filename_combo, (int) len_filename_combo);

    return(tstr_filename_combo); /* donate to caller */
}

static void
SFH_onCommand_CHECK_X_SAVE_SELECTION(
    _HwndRef_   HWND hdlg)      /* handle to child dialog window NOT the 'Save As' window!!! */
{
    const P_SAVE_CALLBACK p_save_callback = (P_SAVE_CALLBACK) GetWindowLongPtr(hdlg, GWLP_USERDATA);
    const HWND hdlg_save = GetParent(hdlg); /* *This* is the 'Save As' window handle */
    const HWND hwnd_filename_combo = GetDlgItem(hdlg_save, cmb13);
    STATUS status;
    PTSTR tstr_filename_combo, tstr_curext;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
    quick_tblock_with_buffer_setup(quick_tblock);

    tstr_filename_combo = SFH_filename_combo_GetText(hwnd_filename_combo, &status);
    if(status_fail(status))
        return;

    /* Always apply the extension currently in force when changing filename */
    tstr_curext = file_extension(tstr_filename_combo);

    if(Button_GetCheck(GetDlgItem(hdlg, IDC_CHECK_X_SAVE_SELECTION)))
    {
        /* Remember what user had typed/modified - WITHOUT the extension */
        ui_text_dispose(&p_save_callback->saving_filename);
        p_save_callback->saving_filename.type = UI_TEXT_TYPE_TSTR_ALLOC;
        p_save_callback->saving_filename.text.tstr = tstr_filename_combo; /* donate */
        if(NULL != tstr_curext) tstr_curext[-1] = CH_NULL; /* nobble */

        /* Change the filename to avoid accidental overwrites of whole file with selection */
        status = quick_tblock_tstr_add(&quick_tblock, resource_lookup_tstr(MSG_SELECTION));
    }
    else
    {
        /* Restore the filename, but using the current extension */
        status = quick_tblock_tstr_add(&quick_tblock, ui_text_tstr(&p_save_callback->saving_filename));
    }

    if(status_ok(status))
        status = quick_tblock_tstr_add(&quick_tblock, FILE_EXT_SEP_TSTR);
    if(status_ok(status))
        status = quick_tblock_tstr_add_n(&quick_tblock, tstr_curext ? tstr_curext : default_extension_from_save_callback(p_save_callback), strlen_with_NULLCH);

    if(status_ok(status))
        ComboBox_SetText(hwnd_filename_combo, quick_tblock_tstr(&quick_tblock));

    quick_tblock_dispose(&quick_tblock);
}

static void
SaveFileHook_onCommand(
    _HwndRef_   HWND hdlg,      /* handle to child dialog window NOT the 'Save As' window!!! */
    _InVal_     WPARAM wParam,  /* message parameter */
    _InVal_     LPARAM lParam)  /* message parameter */
{
    switch(wParam)
    {
    case IDC_CHECK_X_SAVE_SELECTION:
        reportf(TEXT("IDC_CHECK_X_SAVE_SELECTION %d"), Button_GetCheck(GetDlgItem(hdlg, IDC_CHECK_X_SAVE_SELECTION)));
        SFH_onCommand_CHECK_X_SAVE_SELECTION(hdlg);
        break;

    default:
        reportf(TEXT("SFH_WM_COMMAND %u %08x"), wParam, lParam);
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
    UNREFERENCED_PARAMETER(wParam);

    switch(uiMsg)
    {
    case WM_NOTIFY:
        SaveFileHook_onNotify(hdlg, (LPOFNOTIFY) lParam);
        break;

    case WM_COMMAND:
        SaveFileHook_onCommand(hdlg, wParam, lParam);
        break;

    default:
        break;
    }

    return(0); /* allow default action */
}

_Check_return_
static STATUS
windows_save_as_test_result(
    _InVal_     BOOL ofnResult)
{
    if(ofnResult)
        return(STATUS_OK);

    switch(CommDlgExtendedError())
    {
    case 0:                     return(STATUS_CANCEL);

    case CDERR_STRUCTSIZE:      return(status_check());
    case CDERR_INITIALIZATION:  return(status_nomem());
    case CDERR_NOTEMPLATE:      return(status_check());
    case CDERR_NOHINSTANCE:     return(status_check());
    case CDERR_LOADSTRFAILURE:  return(create_error(FILE_ERR_LOADSTRFAIL));
    case CDERR_FINDRESFAILURE:  return(create_error(FILE_ERR_FINDRESFAIL));
    case CDERR_LOADRESFAILURE:  return(create_error(FILE_ERR_LOADRESFAIL));
    case CDERR_LOCKRESFAILURE:  return(status_nomem());
    case CDERR_MEMALLOCFAILURE: return(status_nomem());
    case CDERR_MEMLOCKFAILURE:  return(status_nomem());
    case CDERR_NOHOOK:          return(status_check());
    case CDERR_REGISTERMSGFAIL: return(status_check());
    case FNERR_SUBCLASSFAILURE: return(status_nomem());
    case FNERR_INVALIDFILENAME: return(create_error(FILE_ERR_BADNAME));
    case FNERR_BUFFERTOOSMALL:  return(status_nomem());

    default:                    return(status_check());
    }
}

_Check_return_
static STATUS
windows_save_as_do_save(
    LPOPENFILENAME openfilename,
    P_SAVE_CALLBACK p_save_callback)
{
    /* determine file type to actually save as */
    { /* use nFilterIndex to determine file type to save as */
    const S32 filter_mask = (SAVING_PICTURE == p_save_callback->saving) ? BOUND_FILETYPE_WRITE_PICT : BOUND_FILETYPE_WRITE;
    T5_FILETYPE t5_filetype = windows_filter_list_get_t5_filetype_from_filter_index(openfilename->nFilterIndex, filter_mask, &p_save_callback->h_save_filetype_filters);

    if(FILETYPE_UNDETERMINED == t5_filetype)
        t5_filetype = FILETYPE_TEXT;

    p_save_callback->saving_t5_filetype = t5_filetype;
    } /*block*/

    { /* NB User might have stripped the extension off - ensure we stick the default one on if so */
    PCTSTR tstr_extension = file_extension(openfilename->lpstrFile);

    if(NULL == tstr_extension)
    {   /* or invent some api to extract an extension from filetype on Windows */
        tstr_xstrkat(openfilename->lpstrFile, openfilename->nMaxFile, FILE_EXT_SEP_TSTR);
        tstr_xstrkat(openfilename->lpstrFile, openfilename->nMaxFile, default_extension_from_save_callback(p_save_callback));
    }
    } /* block */

    /* may need to mutate the SAVING_XXX as we might have started out with some other intent */
    switch(p_save_callback->saving_t5_filetype)
    {
    case FILETYPE_T5_FIREWORKZ:
    case FILETYPE_T5_HYBRID_DRAW:
        cmd_save_init_set_saving(p_save_callback);
        p_save_callback->rename_after_save = !p_save_callback->save_selection;
        break;

    case FILETYPE_T5_TEMPLATE:
        cmd_save_as_template_init_set_saving(p_save_callback);
        break;

    case FILETYPE_DRAW:
    default:
        if(SAVING_PICTURE != p_save_callback->saving)
            status_return(cmd_save_as_foreign_init_set_saving(p_save_callback));
        break;
    }

    status_return(dialog_save_common_check(p_save_callback));

    return(proc_save_common(openfilename->lpstrFile, p_save_callback->saving_t5_filetype, (CLIENT_HANDLE) p_save_callback));
}

_Check_return_
static STATUS
windows_save_as(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SAVE_CALLBACK p_save_callback)
{
    TCHARZ szDirName[BUF_MAX_PATHSTRING];
    TCHARZ szFile[BUF_MAX_PATHSTRING];
    TCHARZ szDialogTitle[128];
    const S32 filter_mask = (SAVING_PICTURE == p_save_callback->saving) ? BOUND_FILETYPE_WRITE_PICT : BOUND_FILETYPE_WRITE;
    OPENFILENAME openfilename;
    BOOL ofnResult;
    QUICK_TBLOCK_WITH_BUFFER(filter_quick_tblock, 256);
    quick_tblock_with_buffer_setup(filter_quick_tblock);

    /* build filters as description,CH_NULL,wildcard spec,CH_NULL sets */
    status_return(windows_filter_list_create(&filter_quick_tblock, filter_mask, &p_save_callback->h_save_filetype_filters));

    /* try to find some sensible place to dump these */
    windows_save_as_set_dirname(szDirName, elemof32(szDirName), p_save_callback);

    tstr_xstrkpy(szFile, elemof32(szFile), ui_text_tstr(&p_save_callback->initial_filename));

    zero_struct_fn(openfilename);

    openfilename.lStructSize = sizeof32(openfilename);
    {
    const P_VIEW p_view = p_view_from_viewno_caret(p_docu);
    openfilename.hwndOwner = VIEW_NOT_NONE(p_view) ? p_view->main[WIN_BACK].hwnd : NULL /*host_get_icon_hwnd()*/;
    } /*block*/
    openfilename.hInstance = GetInstanceHandle();
    openfilename.lpstrFilter = quick_tblock_tstr(&filter_quick_tblock);
  /*openfilename.lpstrCustomFilter = 0;*/
  /*openfilename.nMaxCustFilter = 0;*/
    openfilename.nFilterIndex = windows_filter_list_get_filter_index_from_t5_filetype(p_save_callback->saving_t5_filetype, filter_mask, &p_save_callback->h_save_filetype_filters);
    openfilename.lpstrFile = szFile;
    openfilename.nMaxFile = elemof32(szFile);
  /*openfilename.lpstrFileTitle = NULL;*/
  /*openfilename.nMaxFileTitle = 0;*/
    openfilename.lpstrInitialDir = szDirName;
  /*openfilename.lpstrTitle = NULL;*/
    if(p_save_callback->save_selection)
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
  /*openfilename.nFileOffset = 0;*/
  /*openfilename.nFileExtension = 0;*/
  /*openfilename.lpstrDefExt = file_extension(szFile);*/ /* let user call it what they like! */

#if defined(SFH_STATICS)
    SaveFileHook_statics.p_save_callback = p_save_callback;
#else
    /* Use openfilename.lCustData and then GetWindowLongPtr(hdlg, GWLP_USERDATA) */
    openfilename.lCustData = (LPARAM) p_save_callback;
#endif

#if 1
    openfilename.Flags |= OFN_ENABLETEMPLATE;
    openfilename.lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG_X_SAVE_PAGE_RANGE);
#endif

    openfilename.Flags |= OFN_ENABLEHOOK;
    openfilename.lpfnHook = SaveFileHook;

    /* finally, try not to suggest saving files without extensions on Windows */
    /* GetSaveFileName only copies three characters of extension */
    if(NULL == file_extension(openfilename.lpstrFile))
    {
        tstr_xstrkat(openfilename.lpstrFile, openfilename.nMaxFile, FILE_EXT_SEP_TSTR);
        tstr_xstrkat(openfilename.lpstrFile, openfilename.nMaxFile, default_extension_from_save_callback(p_save_callback));
    }

    ofnResult = GetSaveFileName(&openfilename);

    quick_tblock_dispose(&filter_quick_tblock);

    status_return(windows_save_as_test_result(ofnResult));

    return(windows_save_as_do_save(&openfilename, p_save_callback));
}

#endif /* WINDOWS */

/*
rename this document to be filename
*/

_Check_return_
extern STATUS
rename_document_as_filename(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR fullname)
{
    STATUS status;
    DOCU_NAME docu_name;

    name_init(&docu_name);

    if(status_ok(status = name_read_tstr(&docu_name, fullname)))
    {
        status = maeve_event(p_docu, T5_MSG_DOCU_RENAME, (P_ANY) &docu_name);

        name_dispose(&docu_name);
    }

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

/******************************************************************************
*
* T5_CMD_OBJECT_BIND_SAVER
*
******************************************************************************/

T5_CMD_PROTO(extern, t5_cmd_object_bind_saver)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 3);
    INSTALLED_SAVE_OBJECT installed_save_object;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(installed_save_object), FALSE);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    WINDOWS_ONLY(zero_struct(installed_save_object));
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

    return(al_array_add(&g_installed_save_objects_handle, INSTALLED_SAVE_OBJECT, 1, &array_init_block, &installed_save_object));
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ui_save);

_Check_return_
static STATUS
ui_save_msg_startup(void)
{
#if RISCOS || !defined(WINDOWS_SAVE_AS)
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

    for(i = 0; i < array_elements(&g_installed_save_objects_handle); ++i)
    {
        const P_INSTALLED_SAVE_OBJECT p_installed_save_object = array_ptr(&g_installed_save_objects_handle, INSTALLED_SAVE_OBJECT, i);

        al_array_dispose(&p_installed_save_object->h_tstr_ClipboardFormat);

        /* no mechanism to unregister Windows clipboard formats */
    }
    } /*block*/
#endif

    al_array_dispose(&g_installed_save_objects_handle);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_ui_save_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

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
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_ui_save_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of ui_save.c */
