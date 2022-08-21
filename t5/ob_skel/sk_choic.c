/* sk_choic.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Choices handling */

/* SKS March 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_toolb/xp_toolb.h"

#include "cmodules/collect.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

GLOBAL_PREFERENCES global_preferences = { FALSE };

extern void
issue_choice_changed(
    _InVal_     T5_MESSAGE t5_message)
{
    MSG_CHOICE_CHANGED msg_choice_changed;
    DOCNO docno = DOCNO_NONE;

    msg_choice_changed.t5_message = t5_message;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
        status_assert(object_call_all(p_docu_from_docno_valid(docno), T5_MSG_CHOICE_CHANGED, &msg_choice_changed));

    status_assert(maeve_service_event(p_docu_from_config_wr(), T5_MSG_CHOICE_CHANGED, &msg_choice_changed));
}

PROC_EVENT_PROTO(static, scheduled_event_auto_save_all);

_Check_return_
static STATUS
do_auto_save_all(void)
{
    STATUS status = STATUS_OK;

    {
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
    {
        const P_DOCU p_docu = p_docu_from_docno_valid(docno);

        if(p_docu->auto_save_period_minutes)
            continue; /* this one is on its own schedule */

        if(!p_docu->modified)
            continue;

        status_break(status = execute_command_reperr(p_docu, T5_CMD_SAVE_OWNFORM, _P_DATA_NONE(P_ARGLIST_HANDLE), OBJECT_ID_SKEL));
    }

    status = STATUS_OK;
    } /*block*/

    if(global_preferences.auto_save_period_minutes)
    {   /* schedule me an AutoSaveAll event for sometime in the future */
        const MONOTIMEDIFF after = MONOTIMEDIFF_VALUE_FROM_SECONDS(global_preferences.auto_save_period_minutes * (MONOTIMEDIFF) 60);
        trace_0(TRACE__SCHEDULED, TEXT("do_auto_save_all - *** scheduled_event_after(DOCNO_NONE, n)"));
        status = status_wrap(scheduled_event_after(DOCNO_NONE, T5_EVENT_TRY_AUTO_SAVE_ALL, scheduled_event_auto_save_all, 0, after));
    }

    return(status);
}

PROC_EVENT_PROTO(static, scheduled_event_auto_save_all)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER(p_data);

    switch(t5_message)
    {
    case T5_EVENT_TRY_AUTO_SAVE_ALL:
        /* P_SCHEDULED_EVENT_BLOCK p_scheduled_event_block = (P_SCHEDULED_EVENT_BLOCK) p_data; */
        return(do_auto_save_all());

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(extern, t5_cmd_choices_auto_save)
{
    S32 auto_save_period_minutes = 0;
    STATUS status = STATUS_OK;

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

        if(arg_is_present(p_args, 0))
            auto_save_period_minutes = p_args[0].val.s32;
    }

    assert(p_docu == p_docu_from_config());
    if(global_preferences.auto_save_period_minutes != auto_save_period_minutes)
    {
        global_preferences.auto_save_period_minutes = auto_save_period_minutes;
        issue_choice_changed(t5_message);
    }

    /* cancel any outstanding autosaves for all documents */
    trace_0(TRACE__SCHEDULED, TEXT("t5_cmd_choices_auto_save -  *** scheduled_event_remove(DOCNO_NONE), cancel pending auto save all"));
    scheduled_event_remove(DOCNO_NONE, T5_EVENT_TRY_AUTO_SAVE_ALL, scheduled_event_auto_save_all, 0);

    if(p_docu->auto_save_period_minutes)
    {   /* schedule me an AutoSaveAll event for sometime in the future */
        const MONOTIMEDIFF after = MONOTIMEDIFF_VALUE_FROM_SECONDS(p_docu->auto_save_period_minutes * (MONOTIMEDIFF) 60);
        trace_0(TRACE__SCHEDULED, TEXT("t5_cmd_choices_auto_save - *** do scheduled_event_after(DOCNO_NONE, n)"));
        status = status_wrap(scheduled_event_after(DOCNO_NONE, T5_EVENT_TRY_AUTO_SAVE_ALL, scheduled_event_auto_save_all, 0, after));
    }

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_choices_display_pictures)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    assert(p_docu == p_docu_from_config());
    if(global_preferences.dont_display_pictures != !p_args[0].val.fBool)
    {
        global_preferences.dont_display_pictures = !p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_choices_embed_inserted_files)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    assert(p_docu == p_docu_from_config());
    if(global_preferences.embed_inserted_files != p_args[0].val.fBool)
    {
        global_preferences.embed_inserted_files = p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_choices_kerning)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    assert(p_docu == p_docu_from_config());
    if(global_preferences.disable_kerning != !p_args[0].val.fBool)
    {
        global_preferences.disable_kerning = !p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

#if RISCOS
    host_version_font_m_reset();
#endif

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_choices_dithering)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    assert(p_docu == p_docu_from_config());
    if(global_preferences.disable_dithering != !p_args[0].val.fBool)
    {
        global_preferences.disable_dithering = !p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

#if WINDOWS
    host_dithering_set(!global_preferences.disable_dithering);
#endif

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_choices_status_line)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    assert(p_docu == p_docu_from_config());
    if(global_preferences.disable_status_line != !p_args[0].val.fBool)
    {
        global_preferences.disable_status_line = !p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_choices_toolbar)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    assert(p_docu == p_docu_from_config());
    if(global_preferences.disable_toolbar != !p_args[0].val.fBool)
    {
        global_preferences.disable_toolbar = !p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_choices_update_styles_from_choices)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    assert(p_docu == p_docu_from_config());
    if(global_preferences.update_styles_from_choices != p_args[0].val.fBool)
    {
        global_preferences.update_styles_from_choices = p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_choices_ascii_load_as_delimited)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    assert(p_docu == p_docu_from_config());
    if(global_preferences.ascii_load_as_delimited != p_args[0].val.fBool)
    {
        global_preferences.ascii_load_as_delimited = p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_choices_ascii_load_delimiter)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    assert(p_docu == p_docu_from_config());
    if(global_preferences.ascii_load_delimiter != (UCS4) p_args[0].val.s32)
    {
        global_preferences.ascii_load_delimiter = (UCS4) p_args[0].val.s32;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_choices_ascii_load_as_paragraphs)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    assert(p_docu == p_docu_from_config());
    if(global_preferences.ascii_load_as_paragraphs != p_args[0].val.fBool)
    {
        global_preferences.ascii_load_as_paragraphs = p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

/*
listed info
*/

typedef struct CHOICES_MAIN_RULERS_LIST_INFO
{
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, elemof32("1/16 in")); /* NB buffer adjacent for fixup */

    SCALE_INFO scale_info;
}
CHOICES_MAIN_RULERS_LIST_INFO, * P_CHOICES_MAIN_RULERS_LIST_INFO;

_Check_return_
_Ret_z_
static PC_USTR
choices_main_rulers_scale_info_compile(
    _InoutRef_  P_SCALE_INFO p_scale_info,
    _In_z_      PC_USTR ustr)
{
    PC_U8Z p_u8 = (PC_U8Z) ustr;
    P_U8Z p_u8_end;
    S32 s32;

    p_scale_info->loaded = 1;

    {
    DISPLAY_UNIT display_unit = DISPLAY_UNIT_CM;

    s32 = fast_strtol(p_u8, &p_u8_end);

    if(p_u8 != p_u8_end)
    {
        display_unit = (DISPLAY_UNIT) s32;
        if( display_unit < DISPLAY_UNIT_STT )
            display_unit = DISPLAY_UNIT_STT;
        if( display_unit >= DISPLAY_UNIT_COUNT )
            display_unit = (DISPLAY_UNIT) (DISPLAY_UNIT_COUNT - 1);
    }

    p_scale_info->display_unit = display_unit;
    } /*block*/

    p_scale_info->coarse_div = 5;
    if(*p_u8_end == CH_SPACE)
    {
        p_u8 = p_u8_end + 1;
        s32 = fast_strtol(p_u8, &p_u8_end);
        if(p_u8 != p_u8_end)
            p_scale_info->coarse_div = MAX(1, s32);
    }

    p_scale_info->fine_div = 2;
    if(*p_u8_end == CH_SPACE)
    {
        p_u8 = p_u8_end + 1;
        s32 = fast_strtol(p_u8, &p_u8_end);
        if(p_u8 != p_u8_end)
            p_scale_info->fine_div = MAX(1, s32);
    }

    p_scale_info->numbered_units_multiplier = 1;
    if(*p_u8_end == CH_SPACE)
    {
        p_u8 = p_u8_end + 1;
        s32 = fast_strtol(p_u8, &p_u8_end);
        if(p_u8 != p_u8_end)
            p_scale_info->numbered_units_multiplier = MAX(1, s32);
    }

    if(*p_u8_end == CH_SPACE)
        p_u8 = p_u8_end + 1;
    else
    {
        assert0();
        p_u8 = "???";
    }

    return((PC_USTR) p_u8);
}

_Check_return_
static STATUS
choices_main_rulers_list_create(
    _DocuRef_   PC_DOCU p_docu_config,
    /*inout*/ P_ARRAY_HANDLE p_array_handle,
    /*inout*/ P_UI_SOURCE p_ui_source,
    _InoutRef_  P_PIXIT p_max_width)
{
    STATUS status = STATUS_OK;

    {
    P_CHOICES_MAIN_RULERS_LIST_INFO p_choices_main_rulers_list_info;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*p_choices_main_rulers_list_info), TRUE);
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_docu_config->numforms); ++i)
    {
        PC_UI_NUMFORM p_ui_numform = array_ptrc(&p_docu_config->numforms, UI_NUMFORM, i);

        if(p_ui_numform->numform_class != UI_NUMFORM_CLASS_RULER)
            continue;

        if(NULL != (p_choices_main_rulers_list_info = al_array_extend_by(p_array_handle, CHOICES_MAIN_RULERS_LIST_INFO, 1, &array_init_block, &status)))
        {
            PC_USTR ustr_name = choices_main_rulers_scale_info_compile(&p_choices_main_rulers_list_info->scale_info, p_ui_numform->ustr_numform);

            quick_ublock_with_buffer_setup(p_choices_main_rulers_list_info->quick_ublock);

            status = quick_ublock_ustr_add_n(&p_choices_main_rulers_list_info->quick_ublock, ustr_name, strlen_with_NULLCH);

            {
            const PIXIT width = ui_width_from_ustr(ustr_name);
            if( *p_max_width < width)
                *p_max_width = width;
            } /*block*/
        }

        status_break(status);
    }
    } /*block*/

    /* make a source of text pointers to these elements for list box processing */
    if(status_ok(status))
        status = ui_source_create_ub(p_array_handle, p_ui_source, UI_TEXT_TYPE_USTR_PERM, offsetof32(CHOICES_MAIN_RULERS_LIST_INFO, quick_ublock));

    if(status_fail(status))
        ui_lists_dispose_ub(p_array_handle, p_ui_source, offsetof32(CHOICES_MAIN_RULERS_LIST_INFO, quick_ublock));

    return(status);
}

_Check_return_
static STATUS
choices_do_save_val(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     T5_MESSAGE t5_message,
    _InVal_     S32 val)
{
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        p_args[0].val.s32 = val; /* even if u8n */
        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
        arglist_dispose(&arglist_handle);
    }

    return(status);
}

_Check_return_
static STATUS
skel_msg_choices_save(
    _InRef_     P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status;

    if(status_ok(status = choices_do_save_val(p_of_op_format, T5_CMD_CHOICES_AUTO_SAVE,                 (S32)  global_preferences.auto_save_period_minutes)))
    if(status_ok(status = choices_do_save_val(p_of_op_format, T5_CMD_CHOICES_DISPLAY_PICTURES,          (S32) !global_preferences.dont_display_pictures)))
    if(status_ok(status = choices_do_save_val(p_of_op_format, T5_CMD_CHOICES_EMBED_INSERTED_FILES,      (S32)  global_preferences.embed_inserted_files)))
    if(status_ok(status = choices_do_save_val(p_of_op_format, T5_CMD_CHOICES_DITHERING,                 (S32) !global_preferences.disable_dithering)))
    if(status_ok(status = choices_do_save_val(p_of_op_format, T5_CMD_CHOICES_KERNING,                   (S32) !global_preferences.disable_kerning)))
    if(status_ok(status = choices_do_save_val(p_of_op_format, T5_CMD_CHOICES_STATUS_LINE,               (S32) !global_preferences.disable_status_line)))
    if(status_ok(status = choices_do_save_val(p_of_op_format, T5_CMD_CHOICES_TOOLBAR,                   (S32) !global_preferences.disable_toolbar)))
    if(status_ok(status = choices_do_save_val(p_of_op_format, T5_CMD_CHOICES_UPDATE_STYLES_FROM_CHOICES,(S32)  global_preferences.update_styles_from_choices)))
    if(status_ok(status = choices_do_save_val(p_of_op_format, T5_CMD_CHOICES_ASCII_LOAD_AS_DELIMITED,   (S32)  global_preferences.ascii_load_as_delimited)))
    if(status_ok(status = choices_do_save_val(p_of_op_format, T5_CMD_CHOICES_ASCII_LOAD_DELIMITER,      (S32)  global_preferences.ascii_load_delimiter)))
    if(status_ok(status = choices_do_save_val(p_of_op_format, T5_CMD_CHOICES_ASCII_LOAD_AS_PARAGRAPHS,  (S32)  global_preferences.ascii_load_as_paragraphs)))
    { /*EMPTY*/ } /* then that's all right then */

    return(status);
}

/* try to save all of the data in one go to avoid truncated-by-error choices files */

_Check_return_
static STATUS
choices_save_array_data_direct_to_file(
    _In_z_      PCTSTR filename,
    _InRef_     PC_ARRAY_HANDLE p_array_handle)
{
    STATUS status;
    const U32 n_bytes = array_elements32(p_array_handle);
    PC_BYTE p_data = array_rangec(p_array_handle, BYTE, 0, n_bytes);

#if RISCOS
    _kernel_osfile_block osfile_block;

    osfile_block.load  = FILETYPE_ASCII;
    osfile_block.exec  = 0;
    osfile_block.start = (int) p_data;
    osfile_block.end   = osfile_block.start + (int) n_bytes;

    if(_kernel_ERROR == _kernel_osfile(OSFile_SaveStamp, filename, &osfile_block))
        status = file_error_set(_kernel_last_oserror()->errmess);
    else
        status = STATUS_OK;
#else
    FILE_HANDLE file_handle;
    if(status_ok(status = t5_file_open(filename, file_open_write, &file_handle, TRUE)))
    {
        status = file_write_bytes(p_data, n_bytes, file_handle);
        status_accumulate(status, t5_file_close(&file_handle));
    }
#endif /* OS */

    if(status_fail(status))
        status_consume(file_remove(filename));

    return(status);
}

_Check_return_
static STATUS
choices_save_docu_config(
    /*_DocuRef_*/ P_DOCU p_docu_config,
    _In_z_      PCTSTR dirname)
{
    STATUS status = STATUS_OK;
    OF_OP_FORMAT of_op_format = OF_OP_FORMAT_INIT;
    ARRAY_HANDLE array_handle = 0;

    /* choices docu should not be loadable directly */
    status_return(ownform_initialise_save(p_docu_config, &of_op_format, &array_handle, NULL, FILETYPE_ASCII, NULL));

    /* firstly save any options that might also get saved in a normal document */
    zero_struct(of_op_format.of_template);

    /* then be somewhat selective */
    of_op_format.of_template.used_styles = 1;
    of_op_format.of_template.unused_styles = 1;
    of_op_format.of_template.version = 1;
    /* of_op_format.of_template.key_defn = 1; */
    /* of_op_format.of_template.menu_struct = 1; */
    /* of_op_format.of_template.views = 1; */
    of_op_format.of_template.data_class = DATA_SAVE_WHOLE_DOC;
    of_op_format.of_template.ruler_scale = 1;
    /* of_op_format.of_template.note_layer = 1; */
    /* of_op_format.of_template.numform_context = 1; */
    /* of_op_format.of_template.paper = 1; */

    status = save_ownform_file(p_docu_config, &of_op_format);

    status = ownform_finalise_save(&p_docu_config, &of_op_format, status);

    if((status_ok(status)) && (array_elements32(&array_handle) != 0))
    {
        TCHARZ filename[BUF_MAX_PATHSTRING];

        consume_int(tstr_xsnprintf(filename, elemof32(filename),
                                   TEXT("%s") CHOICES_DOCU_FILE_NAME,
                                   dirname));

        status = choices_save_array_data_direct_to_file(filename, &array_handle);
    }

    al_array_dispose(&array_handle);

    return(status);
}

_Check_return_
static STATUS
choices_save_object_id(
    /*_DocuRef_*/ P_DOCU p_docu_config,
    _In_z_      PCTSTR dirname,
    _InVal_     OBJECT_ID object_id)
{
    STATUS status;
    OF_OP_FORMAT of_op_format = OF_OP_FORMAT_INIT;
    ARRAY_HANDLE array_handle = 0;
    U32 initial_elements = 0;

    /* choices files should not be loadable directly */
    status_return(ownform_initialise_save(p_docu_config, &of_op_format, &array_handle, NULL, FILETYPE_ASCII, NULL));

    /* firstly save any options that might also get saved in a normal document */
    zero_struct(of_op_format.of_template);

    of_op_format.of_template.version = 1;
    status = skel_save_version(&of_op_format);

    if(status_ok(status))
    {
        initial_elements = array_elements32(&array_handle);

        if(OBJECT_ID_SKEL == object_id) /* saves handling another case in the big switch */
            status = skel_msg_choices_save(&of_op_format);
        else
            status = object_call_id(object_id, p_docu_config, T5_MSG_CHOICES_SAVE, &of_op_format);
    }

    status = ownform_finalise_save(&p_docu_config, &of_op_format, status);

    /* try saving object's choices iff object added to the file data */
    if((status_ok(status)) && (initial_elements != array_elements32(&array_handle)))
    {
        TCHARZ filename[BUF_MAX_PATHSTRING];

        consume_int(tstr_xsnprintf(filename, elemof32(filename),
                                   TEXT("%s") CHOICES_FILE_FORMAT_STR,
                                   dirname, (S32) object_id));

        status = choices_save_array_data_direct_to_file(filename, &array_handle);
    }

    al_array_dispose(&array_handle);

    return(status);
}

_Check_return_
static STATUS
choices_save_objects(
    /*_DocuRef_*/ P_DOCU p_docu_config,
    _In_z_      PCTSTR dirname)
{
    OBJECT_ID object_id = OBJECT_ID_ENUM_START;

    while(status_ok(object_next(&object_id)))
        status_return(choices_save_object_id(p_docu_config, dirname, object_id));

    return(STATUS_OK);
}

_Check_return_
static STATUS
choices_do_save(
    /*_DocuRef_*/ P_DOCU p_docu_config)
{
    TCHARZ dirname[BUF_MAX_PATHSTRING];
    STATUS status = STATUS_OK;

#if RISCOS
    tstr_xstrkpy(dirname, elemof32(dirname), TEXT("<Choices$Write>") FILE_DIR_SEP_TSTR TEXT("Fireworkz"));

    status_return(file_create_directory(dirname));

    tstr_xstrkat(dirname, elemof32(dirname), FILE_DIR_SEP_TSTR);
#else
    file_get_prefix(dirname, elemof32(dirname), NULL); /* includes final dir sep ch */
#endif /* OS */

    if(status_ok(status))
    {
        TCHARZ subdirname[BUF_MAX_PATHSTRING];
        tstr_xstrkpy(subdirname, elemof32(subdirname), dirname);
        tstr_xstrkat(subdirname, elemof32(subdirname), CHOICES_DIR_NAME);
        status_return(status = file_create_directory(subdirname));
    }

    if(status_ok(status))
        status = choices_save_docu_config(p_docu_config, dirname);

    if(status_ok(status))
        status = choices_save_objects(p_docu_config, dirname);

    return(status);
}

/* choices are obviously object dependent! try to make this box adaptable at run-time to the objects present */

enum CHOICES_CONTROL_IDS
{
    CHOICES_ID_SAVE = 33,

    CHOICES_MAIN_ID_GROUP,
    CHOICES_MAIN_ID_AUTO_SAVE_TEXT,
    CHOICES_MAIN_ID_AUTO_SAVE_PERIOD,
    CHOICES_MAIN_ID_AUTO_SAVE_UNITS,
    CHOICES_MAIN_ID_DISPLAY_PICTURES,
    CHOICES_MAIN_ID_EMBED_INSERTED_FILES,
    CHOICES_MAIN_ID_KERNING,
    CHOICES_MAIN_ID_DITHERING,
    CHOICES_MAIN_ID_STATUS_LINE,
    CHOICES_MAIN_ID_TOOLBAR,
    CHOICES_MAIN_ID_UPDATE_STYLES_FROM_CHOICES,

    CHOICES_MAIN_ID_RULERS_RULER_TEXT,
    CHOICES_MAIN_ID_RULERS_RULER_COMBO,
#define CHOICES_MAIN_RULERS_LIST_V (DIALOG_STDLISTOVH_V + 4 * DIALOG_STDLISTITEM_V)

    CHOICES_MAIN_ID_ASCII_LOAD_AS_DELIMITED,
    CHOICES_MAIN_ID_ASCII_LOAD_DELIMITER,
    CHOICES_MAIN_ID_ASCII_LOAD_AS_PARAGRAPHS,

    CHOICES_ID_MAX
};

static const ARG_TYPE
args_s32[] = { ARG_TYPE_S32, ARG_TYPE_NONE };

static /*poked*/ DIALOG_CONTROL
choices_main_group =
{
    CHOICES_MAIN_ID_GROUP, DIALOG_MAIN_GROUP,

    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, CHOICES_MAIN_ID_RULERS_RULER_COMBO, CHOICES_MAIN_ID_RULERS_RULER_COMBO },

    { 0 },

    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
choices_main_auto_save_text =
{
    CHOICES_MAIN_ID_AUTO_SAVE_TEXT, CHOICES_MAIN_ID_GROUP,
    { DIALOG_CONTROL_PARENT, CHOICES_MAIN_ID_AUTO_SAVE_PERIOD, DIALOG_CONTROL_SELF, CHOICES_MAIN_ID_AUTO_SAVE_PERIOD },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
choices_main_auto_save_text_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_MAIN_AUTO_SAVE), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
choices_main_auto_save_period_minutes =
{
    CHOICES_MAIN_ID_AUTO_SAVE_PERIOD, DIALOG_CONTROL_WINDOW,
    { CHOICES_MAIN_ID_AUTO_SAVE_TEXT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDSPACING_H, 0, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_S32), 1 /*tabstop*/ }
};

static const UI_CONTROL_S32
choices_main_auto_save_period_minutes_control = { 0, 10000, 1 };

static /*poked*/ DIALOG_CONTROL_DATA_BUMP_S32
choices_main_auto_save_period_minutes_data = { { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/, &choices_main_auto_save_period_minutes_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
choices_main_auto_save_units =
{
    CHOICES_MAIN_ID_AUTO_SAVE_UNITS, CHOICES_MAIN_ID_GROUP,
    { CHOICES_MAIN_ID_AUTO_SAVE_PERIOD, CHOICES_MAIN_ID_AUTO_SAVE_PERIOD, DIALOG_CONTROL_SELF, CHOICES_MAIN_ID_AUTO_SAVE_PERIOD },
    { DIALOG_STDBUMPSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
choices_main_auto_save_units_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_MAIN_AUTO_SAVE_UNITS), { 1 /*left_text*/, 0, 1 /*windows_no_colon*/ } };

static const DIALOG_CONTROL
choices_main_display_pictures =
{
    CHOICES_MAIN_ID_DISPLAY_PICTURES, CHOICES_MAIN_ID_GROUP,
    { CHOICES_MAIN_ID_AUTO_SAVE_TEXT, CHOICES_MAIN_ID_AUTO_SAVE_PERIOD },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static DIALOG_CONTROL_DATA_CHECKBOX
choices_main_display_pictures_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_MAIN_DISPLAY_PICTURES) };

static const DIALOG_CONTROL
choices_main_embed_inserted_files =
{
    CHOICES_MAIN_ID_EMBED_INSERTED_FILES, CHOICES_MAIN_ID_GROUP,
    { CHOICES_MAIN_ID_DISPLAY_PICTURES, CHOICES_MAIN_ID_DISPLAY_PICTURES },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static DIALOG_CONTROL_DATA_CHECKBOX
choices_main_embed_inserted_files_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_MAIN_EMBED_INSERTED_FILES) };

static const DIALOG_CONTROL
choices_main_update_styles_from_choices =
{
    CHOICES_MAIN_ID_UPDATE_STYLES_FROM_CHOICES, CHOICES_MAIN_ID_GROUP,
    { CHOICES_MAIN_ID_EMBED_INSERTED_FILES, CHOICES_MAIN_ID_EMBED_INSERTED_FILES },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static DIALOG_CONTROL_DATA_CHECKBOX
choices_main_update_styles_from_choices_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_MAIN_UPDATE_STYLES_FROM_CHOICES) };

static const DIALOG_CONTROL
choices_main_ascii_load_as_delimited =
{
    CHOICES_MAIN_ID_ASCII_LOAD_AS_DELIMITED, CHOICES_MAIN_ID_GROUP,
    { CHOICES_MAIN_ID_UPDATE_STYLES_FROM_CHOICES, CHOICES_MAIN_ID_UPDATE_STYLES_FROM_CHOICES, 0, CHOICES_MAIN_ID_ASCII_LOAD_DELIMITER },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC /*, DIALOG_STDCHECK_V*/ },
    { DRT(LBLB, CHECKBOX), 1 /*tabstop*/ }
};

static DIALOG_CONTROL_DATA_CHECKBOX
choices_main_ascii_load_as_delimited_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_MAIN_ASCII_LOAD_AS_DELIMITED) };

static const DIALOG_CONTROL
choices_main_ascii_load_delimiter =
{
    CHOICES_MAIN_ID_ASCII_LOAD_DELIMITER, CHOICES_MAIN_ID_GROUP,
    { CHOICES_MAIN_ID_ASCII_LOAD_AS_DELIMITED, CHOICES_MAIN_ID_ASCII_LOAD_AS_DELIMITED },
    { DIALOG_STDSPACING_H, 0, DIALOG_STDEDITOVH_H + DIALOG_NUMCHAR_H * (1), DIALOG_STDBUMP_V },
    { DRT(RTLT, EDIT), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_EDIT
choices_main_ascii_load_delimiter_data = { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/ };

static const DIALOG_CONTROL
choices_main_ascii_load_as_paragraphs =
{
    CHOICES_MAIN_ID_ASCII_LOAD_AS_PARAGRAPHS, CHOICES_MAIN_ID_GROUP,
    { CHOICES_MAIN_ID_ASCII_LOAD_AS_DELIMITED, CHOICES_MAIN_ID_ASCII_LOAD_AS_DELIMITED },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static DIALOG_CONTROL_DATA_CHECKBOX
choices_main_ascii_load_as_paragraphs_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_MAIN_ASCII_LOAD_AS_PARAGRAPHS) };

#if RISCOS

static const DIALOG_CONTROL
choices_main_kerning =
{
    CHOICES_MAIN_ID_KERNING, CHOICES_MAIN_ID_GROUP,
    { CHOICES_MAIN_ID_ASCII_LOAD_AS_PARAGRAPHS, CHOICES_MAIN_ID_ASCII_LOAD_AS_PARAGRAPHS },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static DIALOG_CONTROL_DATA_CHECKBOX
choices_main_kerning_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_MAIN_KERNING) };

#elif WINDOWS

static const DIALOG_CONTROL
choices_main_dithering =
{
    CHOICES_MAIN_ID_DITHERING, CHOICES_MAIN_ID_GROUP,
    { CHOICES_MAIN_ID_ASCII_LOAD_AS_PARAGRAPHS, CHOICES_MAIN_ID_ASCII_LOAD_AS_PARAGRAPHS },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static DIALOG_CONTROL_DATA_CHECKBOX
choices_main_dithering_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_MAIN_DITHERING) };

#endif

static const DIALOG_CONTROL
choices_main_status_line =
{
    CHOICES_MAIN_ID_STATUS_LINE, CHOICES_MAIN_ID_GROUP,
#if RISCOS
    { CHOICES_MAIN_ID_KERNING, CHOICES_MAIN_ID_KERNING },
#elif WINDOWS
    { CHOICES_MAIN_ID_DITHERING, CHOICES_MAIN_ID_DITHERING },
#endif
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static DIALOG_CONTROL_DATA_CHECKBOX
choices_main_status_line_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_MAIN_STATUS_LINE) };

static const DIALOG_CONTROL
choices_main_toolbar =
{
    CHOICES_MAIN_ID_TOOLBAR, CHOICES_MAIN_ID_GROUP,
    { CHOICES_MAIN_ID_STATUS_LINE, CHOICES_MAIN_ID_STATUS_LINE },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static DIALOG_CONTROL_DATA_CHECKBOX
choices_main_toolbar_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_MAIN_TOOLBAR) };

static const DIALOG_CONTROL
choices_main_rulers_ruler_text =
{
    CHOICES_MAIN_ID_RULERS_RULER_TEXT, CHOICES_MAIN_ID_GROUP,
    { CHOICES_MAIN_ID_TOOLBAR, CHOICES_MAIN_ID_RULERS_RULER_COMBO, CHOICES_MAIN_ID_RULERS_RULER_COMBO, CHOICES_MAIN_ID_RULERS_RULER_COMBO },
    { 0, 0, DIALOG_STDSPACING_H, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
choices_main_rulers_ruler_text_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_MAIN_RULERS_RULER), { 1 /*left_text*/ } };

static /*poked*/ DIALOG_CONTROL
choices_main_rulers_ruler_combo =
{
    CHOICES_MAIN_ID_RULERS_RULER_COMBO, CHOICES_MAIN_ID_GROUP,

    /* SKS 26sep93 overlap previous item to save space */
    { DIALOG_CONTROL_SELF, CHOICES_MAIN_ID_TOOLBAR, CHOICES_MAIN_ID_AUTO_SAVE_UNITS },

    { 0 /*poked*/, DIALOG_STDSPACING_V, 0, DIALOG_STDCOMBO_V },

    { DRT(RBRT, COMBO_TEXT), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_COMBO_TEXT
choices_main_rulers_ruler_combo_data =
{
  {/*combo_xx*/

    {/*edit_xx*/ {/*bits*/ FRAMED_BOX_EDIT, 1 /*readonly*/ /*bits*/}, NULL /*edit_xx*/},

    {/*list_xx*/ { 0 /*force_v_scroll*/, 1 /*disable_double*/, 0 /*tab_position*/} /*list_xx*/},

        NULL,

        12 * DIALOG_STDLISTITEM_V /*dropdown_size*/

      /*combo_xx*/},

    { UI_TEXT_TYPE_NONE } /*state*/
};

static const DIALOG_CTL_CREATE
choices_main_ctl_create[] =
{
    { &dialog_main_group },

    { &choices_main_group },
    { &choices_main_auto_save_text,             &choices_main_auto_save_text_data },
    { &choices_main_auto_save_period_minutes,   &choices_main_auto_save_period_minutes_data },
    { &choices_main_auto_save_units,            &choices_main_auto_save_units_data },
    { &choices_main_display_pictures,           &choices_main_display_pictures_data },
    { &choices_main_embed_inserted_files,       &choices_main_embed_inserted_files_data },
    { &choices_main_update_styles_from_choices, &choices_main_update_styles_from_choices_data },
    { &choices_main_ascii_load_as_delimited,    &choices_main_ascii_load_as_delimited_data },
    { &choices_main_ascii_load_delimiter,       &choices_main_ascii_load_delimiter_data },
    { &choices_main_ascii_load_as_paragraphs,   &choices_main_ascii_load_as_paragraphs_data },
#if RISCOS
    { &choices_main_kerning,                    &choices_main_kerning_data },
#elif WINDOWS
    { &choices_main_dithering,                  &choices_main_dithering_data },
#endif
    { &choices_main_status_line,                &choices_main_status_line_data },
    { &choices_main_toolbar,                    &choices_main_toolbar_data },
    { &choices_main_rulers_ruler_text,          &choices_main_rulers_ruler_text_data },
    { &choices_main_rulers_ruler_combo,         &choices_main_rulers_ruler_combo_data }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
choices_apply_data = { { DIALOG_COMPLETION_OK }, UI_TEXT_INIT_RESID(MSG_APPLY) };

static const DIALOG_CONTROL
choices_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, CHOICES_ID_SAVE, CHOICES_ID_SAVE, CHOICES_ID_SAVE },
#if WINDOWS
    { DIALOG_STDCANCEL_H, 0, DIALOG_STDSPACING_H, 0 },
#else
    { DIALOG_CONTENTS_CALC, 0, DIALOG_STDSPACING_H, 0 },
#endif
    { DRT(RTLB, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
choices_save =
{
    CHOICES_ID_SAVE, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, IDOK, IDOK, IDOK },
#if WINDOWS
    { DIALOG_STDPUSHBUTTON_H, -DIALOG_DEFPUSHEXTRA_V, DIALOG_STDSPACING_H, -DIALOG_DEFPUSHEXTRA_V },
#else
    { DIALOG_CONTENTS_CALC, -DIALOG_DEFPUSHEXTRA_V, DIALOG_STDSPACING_H, -DIALOG_DEFPUSHEXTRA_V },
#endif
    { DRT(RTLB, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
choices_save_data = { { CHOICES_ID_SAVE }, UI_TEXT_INIT_RESID(MSG_QUERY_SAVE) };

static const DIALOG_CTL_CREATE
choices_ctl_create[] =
{
    { &defbutton_ok,   &choices_apply_data },
    { &choices_save,   &choices_save_data },
    { &choices_cancel, &stdbutton_cancel_data }
};

typedef struct CHOICES_MAIN_CALLBACK
{
    BOOL atx;
    ARRAY_HANDLE rulers_list_handle;
}
CHOICES_MAIN_CALLBACK, * P_CHOICES_MAIN_CALLBACK; typedef CHOICES_MAIN_CALLBACK * PC_CHOICES_MAIN_CALLBACK;

static UI_SOURCE
choices_main_rulers_list_source;

_Check_return_
static STATUS
dialog_choices_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case CHOICES_MAIN_ID_RULERS_RULER_COMBO:
        p_dialog_msg_ctl_fill_source->p_ui_source = &choices_main_rulers_list_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_choices_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const PC_CHOICES_MAIN_CALLBACK p_choices_main_callback = (PC_CHOICES_MAIN_CALLBACK) p_dialog_msg_process_start->client_handle;
    const PC_DOCU p_docu_config = p_docu_from_config();
    STATUS status = STATUS_OK;

    if(p_choices_main_callback->atx) /*select current ruler setting*/
    {
        S32 itemno = DIALOG_CTL_STATE_LIST_ITEM_NONE;
        ARRAY_INDEX i;

        for(i = 0; i < array_elements(&p_choices_main_callback->rulers_list_handle); ++i)
        {
            P_CHOICES_MAIN_RULERS_LIST_INFO p_choices_main_rulers_list_info = array_ptr(&p_choices_main_callback->rulers_list_handle, CHOICES_MAIN_RULERS_LIST_INFO, i);

            if( (p_choices_main_rulers_list_info->scale_info.display_unit               == p_docu_config->scale_info.display_unit               ) &&
                (p_choices_main_rulers_list_info->scale_info.coarse_div                 == p_docu_config->scale_info.coarse_div                 ) &&
                (p_choices_main_rulers_list_info->scale_info.fine_div                   == p_docu_config->scale_info.fine_div                   ) &&
                (p_choices_main_rulers_list_info->scale_info.numbered_units_multiplier  == p_docu_config->scale_info.numbered_units_multiplier  ) )
            {
                itemno = i;
                break;
            }
        }

        status = ui_dlg_set_list_idx(p_dialog_msg_process_start->h_dialog, CHOICES_MAIN_ID_RULERS_RULER_COMBO, itemno);
    }

    /* don't yet have to pass this around */
    return(status);
}

_Check_return_
static STATUS
dialog_choices_process_end_update_skel(
    _InRef_     P_DOCU p_docu_config,
    _InVal_     H_DIALOG h_dialog,
    _InRef_     PC_CHOICES_MAIN_CALLBACK p_choices_main_callback)
{
    DIALOG_CMD_CTL_STATE_QUERY dialog_cmd_ctl_state_query;
    ARGLIST_HANDLE arglist_handle;
    P_ARGLIST_ARG p_args;

    status_return(arglist_prepare(&arglist_handle, args_s32));

    p_args = p_arglist_args(&arglist_handle, 1);

    msgclr(dialog_cmd_ctl_state_query);
    dialog_cmd_ctl_state_query.h_dialog = h_dialog;
    dialog_cmd_ctl_state_query.bits = 0;

    dialog_cmd_ctl_state_query.dialog_control_id = CHOICES_MAIN_ID_AUTO_SAVE_PERIOD;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    p_args[0].val.s32 = dialog_cmd_ctl_state_query.state.bump_s32;
    status_consume(execute_command_reperr(p_docu_config, T5_CMD_CHOICES_AUTO_SAVE, &arglist_handle, OBJECT_ID_SKEL));

    dialog_cmd_ctl_state_query.dialog_control_id = CHOICES_MAIN_ID_ASCII_LOAD_DELIMITER;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    if(ui_text_is_blank(&dialog_cmd_ctl_state_query.state.edit.ui_text))
        p_args[0].val.s32 = 0;
    else
        p_args[0].val.s32 = (S32) uchars_char_decode_NULL(ui_text_ustr(&dialog_cmd_ctl_state_query.state.edit.ui_text));
    status_consume(execute_command_reperr(p_docu_config, T5_CMD_CHOICES_ASCII_LOAD_DELIMITER, &arglist_handle, OBJECT_ID_SKEL));

    /* bodge to save aggro */
    p_args[0].val.s32 = 0;
    p_args[0].type = ARG_TYPE_BOOL;

    dialog_cmd_ctl_state_query.dialog_control_id = CHOICES_MAIN_ID_DISPLAY_PICTURES;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    p_args[0].val.fBool = dialog_cmd_ctl_state_query.state.checkbox;
    status_consume(execute_command_reperr(p_docu_config, T5_CMD_CHOICES_DISPLAY_PICTURES, &arglist_handle, OBJECT_ID_SKEL));

    dialog_cmd_ctl_state_query.dialog_control_id = CHOICES_MAIN_ID_EMBED_INSERTED_FILES;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    p_args[0].val.fBool = dialog_cmd_ctl_state_query.state.checkbox;
    status_consume(execute_command_reperr(p_docu_config, T5_CMD_CHOICES_EMBED_INSERTED_FILES, &arglist_handle, OBJECT_ID_SKEL));

    dialog_cmd_ctl_state_query.dialog_control_id = CHOICES_MAIN_ID_UPDATE_STYLES_FROM_CHOICES;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    p_args[0].val.fBool = dialog_cmd_ctl_state_query.state.checkbox;
    status_consume(execute_command_reperr(p_docu_config, T5_CMD_CHOICES_UPDATE_STYLES_FROM_CHOICES, &arglist_handle, OBJECT_ID_SKEL));

    dialog_cmd_ctl_state_query.dialog_control_id = CHOICES_MAIN_ID_ASCII_LOAD_AS_DELIMITED;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    p_args[0].val.fBool = dialog_cmd_ctl_state_query.state.checkbox;
    status_consume(execute_command_reperr(p_docu_config, T5_CMD_CHOICES_ASCII_LOAD_AS_DELIMITED, &arglist_handle, OBJECT_ID_SKEL));

    dialog_cmd_ctl_state_query.dialog_control_id = CHOICES_MAIN_ID_ASCII_LOAD_AS_PARAGRAPHS;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    p_args[0].val.fBool = dialog_cmd_ctl_state_query.state.checkbox;
    status_consume(execute_command_reperr(p_docu_config, T5_CMD_CHOICES_ASCII_LOAD_AS_PARAGRAPHS, &arglist_handle, OBJECT_ID_SKEL));

#if RISCOS
    dialog_cmd_ctl_state_query.dialog_control_id = CHOICES_MAIN_ID_KERNING;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    p_args[0].val.fBool = dialog_cmd_ctl_state_query.state.checkbox;
    status_consume(execute_command_reperr(p_docu_config, T5_CMD_CHOICES_KERNING, &arglist_handle, OBJECT_ID_SKEL));
#elif WINDOWS
    dialog_cmd_ctl_state_query.dialog_control_id = CHOICES_MAIN_ID_DITHERING;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    p_args[0].val.fBool = dialog_cmd_ctl_state_query.state.checkbox;
    status_consume(execute_command_reperr(p_docu_config, T5_CMD_CHOICES_DITHERING, &arglist_handle, OBJECT_ID_SKEL));
#endif

    dialog_cmd_ctl_state_query.dialog_control_id = CHOICES_MAIN_ID_STATUS_LINE;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    p_args[0].val.fBool = dialog_cmd_ctl_state_query.state.checkbox;
    status_consume(execute_command_reperr(p_docu_config, T5_CMD_CHOICES_STATUS_LINE, &arglist_handle, OBJECT_ID_SKEL));

    dialog_cmd_ctl_state_query.dialog_control_id = CHOICES_MAIN_ID_TOOLBAR;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_QUERY, &dialog_cmd_ctl_state_query));
    p_args[0].val.fBool = dialog_cmd_ctl_state_query.state.checkbox;
    status_consume(execute_command_reperr(p_docu_config, T5_CMD_CHOICES_TOOLBAR, &arglist_handle, OBJECT_ID_SKEL));

    if(p_choices_main_callback->atx)
    {
        S32 itemno = ui_dlg_get_list_idx(h_dialog, CHOICES_MAIN_ID_RULERS_RULER_COMBO);

        if(array_index_is_valid(&p_choices_main_callback->rulers_list_handle, itemno))
        {
            P_CHOICES_MAIN_RULERS_LIST_INFO p_choices_main_rulers_list_info = array_ptr_no_checks(&p_choices_main_callback->rulers_list_handle, CHOICES_MAIN_RULERS_LIST_INFO, itemno);
            p_docu_config->scale_info  = p_choices_main_rulers_list_info->scale_info;
            p_docu_config->vscale_info = p_choices_main_rulers_list_info->scale_info;
        }
    }

    arglist_dispose(&arglist_handle);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_choices_process_end_update_others(
    _InRef_     P_DOCU p_docu_config,
    _InVal_     H_DIALOG h_dialog)
{
    CHOICES_SET_BLOCK choices_set_block;
    DOCNO docno = DOCNO_NONE;

    choices_set_block.h_dialog = h_dialog;

    status_assert(object_call_all(p_docu_config, T5_MSG_CHOICES_SET, &choices_set_block));

    maeve_service_event(p_docu_config, T5_MSG_CHOICES_CHANGED, P_DATA_NONE);

    while(DOCNO_NONE != (docno = docno_enum_docs(docno))) /* SKS 04oct95 this is what clients really want */
        status_assert(object_call_all(p_docu_from_docno_valid(docno), T5_MSG_CHOICES_CHANGED, P_DATA_NONE));

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_choices_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        const P_CHOICES_MAIN_CALLBACK p_choices_main_callback = (P_CHOICES_MAIN_CALLBACK) p_dialog_msg_process_end->client_handle;
        const H_DIALOG h_dialog = p_dialog_msg_process_end->h_dialog;
        const P_DOCU p_docu_config = p_docu_from_config_wr();

        status_return(dialog_choices_process_end_update_skel(p_docu_config, h_dialog, p_choices_main_callback));

        status_return(dialog_choices_process_end_update_others(p_docu_config, h_dialog));
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_choices)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_choices_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_choices_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_choices_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(extern, t5_cmd_choices)
{
    /* put up a dialog with the list of things */
    P_DOCU p_docu_config = p_docu_from_config_wr();
    CHOICES_QUERY_BLOCK choices_query_block;
    CHOICES_MAIN_CALLBACK choices_main_callback;
    STATUS status;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    choices_query_block.ctl_create = 0;

    choices_main_callback.atx = 1;
    choices_main_callback.rulers_list_handle = 0;

    choices_main_rulers_list_source.type = UI_SOURCE_TYPE_NONE;

    choices_main_auto_save_period_minutes_data.state = (S32) global_preferences.auto_save_period_minutes;
    choices_main_display_pictures_data.init_state = (U8) !global_preferences.dont_display_pictures;
    choices_main_embed_inserted_files_data.init_state = (U8) global_preferences.embed_inserted_files;
    choices_main_update_styles_from_choices_data.init_state = (U8) global_preferences.update_styles_from_choices;
#if RISCOS
    choices_main_kerning_data.init_state = (U8) !global_preferences.disable_kerning;
#elif WINDOWS
    choices_main_dithering_data.init_state = (U8) !global_preferences.disable_dithering;
#endif
    choices_main_status_line_data.init_state = (U8) !global_preferences.disable_status_line;
    choices_main_toolbar_data.init_state = (U8) !global_preferences.disable_toolbar;

    choices_main_ascii_load_as_delimited_data.init_state = (U8) global_preferences.ascii_load_as_delimited;
    choices_main_ascii_load_as_paragraphs_data.init_state = (U8) global_preferences.ascii_load_as_paragraphs;

    {
    static UCHARZ ustr_buffer[8];
    U32 bytes_of_char = 0;
    if(global_preferences.ascii_load_delimiter >= CH_SPACE)
        bytes_of_char = uchars_char_encode(ustr_bptr(ustr_buffer), elemof(ustr_buffer), global_preferences.ascii_load_delimiter);
    ustr_buffer[bytes_of_char] = CH_NULL;
    choices_main_ascii_load_delimiter_data.state.type = UI_TEXT_TYPE_USTR_PERM;
    choices_main_ascii_load_delimiter_data.state.text.ustr = ustr_bptr(ustr_buffer);
    } /*block*/

    if(choices_main_callback.atx)
    {
        DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
        PIXIT max_width = 0;
        status_return(choices_main_rulers_list_create(p_docu_config, &choices_main_callback.rulers_list_handle, &choices_main_rulers_list_source, &max_width));
        dialog_cmd_ctl_size_estimate.p_dialog_control = &choices_main_rulers_ruler_combo;
        dialog_cmd_ctl_size_estimate.p_dialog_control_data = &choices_main_rulers_ruler_combo_data;
        ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
#if RISCOS
        dialog_cmd_ctl_size_estimate.size.x += (6 * max_width) / 4; /* System font */
#else
        dialog_cmd_ctl_size_estimate.size.x += max_width;
        dialog_cmd_ctl_size_estimate.size.x += (PIXITS_PER_INCH / 12); /* be generous */
#endif
        choices_main_rulers_ruler_combo.relative_offset[0] = dialog_cmd_ctl_size_estimate.size.x;
    }

    for(;;) /* loop for structure */
    {
        {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(DIALOG_CTL_CREATE), FALSE);
        S32 n_ctls = elemof32(choices_main_ctl_create);

        if(choices_main_callback.atx)
        {
            choices_main_group.relative_dialog_control_id[2] =
            choices_main_group.relative_dialog_control_id[3] = CHOICES_MAIN_ID_RULERS_RULER_COMBO;
        }
        else
        {
            choices_main_group.relative_dialog_control_id[2] = CHOICES_MAIN_ID_AUTO_SAVE_PERIOD;
            choices_main_group.relative_dialog_control_id[3] = CHOICES_MAIN_ID_TOOLBAR;

            n_ctls -= 2; /* remove ruler controls */
        }

        status_break(status = al_array_add(&choices_query_block.ctl_create, DIALOG_CTL_CREATE, n_ctls, &array_init_block, choices_main_ctl_create));
        } /*block*/

        choices_query_block.tr_dialog_control_id =
        choices_query_block.br_dialog_control_id = CHOICES_MAIN_ID_GROUP;

        /* add controls from other objects - NB. we may have to pass a PROCESS_START to them if they become more complicated */
        status_break(status = object_call_all(p_docu_config, T5_MSG_CHOICES_QUERY, &choices_query_block));

        status_break(status = al_array_add(&choices_query_block.ctl_create, DIALOG_CTL_CREATE, elemof32(choices_ctl_create), PC_ARRAY_INIT_BLOCK_NONE, choices_ctl_create));

        {
        const U32 n_elements = array_elements32(&choices_query_block.ctl_create);
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, array_range(&choices_query_block.ctl_create, DIALOG_CTL_CREATE, 0, n_elements), n_elements, MSG_DIALOG_CHOICES_HELP_TOPIC);
        /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
        dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_CHOICES_CAPTION;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_choices;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &choices_main_callback;
        status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        } /*block*/

        break; /* out of loop for structure */
        /*NOTREACHED*/
    }

    ui_lists_dispose_ub(&choices_main_callback.rulers_list_handle, &choices_main_rulers_list_source, offsetof32(CHOICES_MAIN_RULERS_LIST_INFO, quick_ublock));

    al_array_dispose(&choices_query_block.ctl_create);

    p_docu_config = p_docu_from_config_wr();

    status_assert(object_call_all(p_docu_config, T5_MSG_CHOICES_ENDED, P_DATA_NONE));
    status_assert(maeve_service_event(p_docu_config, T5_MSG_CHOICES_ENDED, P_DATA_NONE));

    if(status == CHOICES_ID_SAVE)
        status = choices_do_save(p_docu_config);

    return(status);
}

/* end of sk_choic.c */
