/* sk_root.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Manage redirection of events according to input focus etc. */

/* MRJC October 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

PROC_EVENT_PROTO(extern, proc_event_sk_cont_direct);

/*
internal functions
*/

#if defined(UNUSED) || 0
T5_CMD_PROTO(static, t5_cmd_test);
#endif

T5_MSG_PROTO(static, skel_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP_SERVICES:
        /* resource_startup been done early */
#if RISCOS
        host_fixup_system_sprites();
#elif WINDOWS
        {
        static const RESOURCE_BITMAP_ID skel_toolbar_common_btn_16x16_4bpp  = { OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_COM_BTN_ID + 0 }; /* 96 dpi buttons, 4 bpp */
        static const RESOURCE_BITMAP_ID skel_toolbar_common_btn_24x24_4bpp  = { OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_COM_BTN_ID + 1 }; /* 120 dpi buttons, 4 bpp */
        static const RESOURCE_BITMAP_ID skel_toolbar_common_btn_16x16_32bpp = { OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_COM_BTN_ID + 2 }; /* 96 dpi buttons, 32 bpp */
        static const RESOURCE_BITMAP_ID skel_toolbar_common_btn_24x24_32bpp = { OBJECT_ID_SKEL, SKEL_ID_BM_TOOLBAR_COM_BTN_ID + 3 }; /* 120 dpi buttons, 32 bpp */
        static const RESOURCE_BITMAP_ID skel_common_btn_16x16 = { OBJECT_ID_SKEL, SKEL_ID_BM_COM_BTN_ID + 0 }; /* 96 dpi buttons */
        static const RESOURCE_BITMAP_ID skel_common_btn_24x24 = { OBJECT_ID_SKEL, SKEL_ID_BM_COM_BTN_ID + 1 }; /* 120 dpi buttons */
        static const RESOURCE_BITMAP_ID skel_common_btn_07x11 = { OBJECT_ID_SKEL, SKEL_ID_BM_COM07X11_ID };
        status_assert(resource_bitmap_tool_size_register(&skel_toolbar_common_btn_16x16_4bpp,  16, 16));
        status_assert(resource_bitmap_tool_size_register(&skel_toolbar_common_btn_24x24_4bpp,  24, 24));
        status_assert(resource_bitmap_tool_size_register(&skel_toolbar_common_btn_16x16_32bpp, 16, 16));
        status_assert(resource_bitmap_tool_size_register(&skel_toolbar_common_btn_24x24_32bpp, 24, 24));
        status_assert(resource_bitmap_tool_size_register(&skel_common_btn_16x16, 16, 16));
        status_assert(resource_bitmap_tool_size_register(&skel_common_btn_24x24, 24, 24));
        status_assert(resource_bitmap_tool_size_register(&skel_common_btn_07x11,  7, 11));
        } /*block*/
#endif
        return(STATUS_OK);

    case T5_MSG_IC__STARTUP:
        return(skeleton_table_startup());

    case T5_MSG_IC__STARTUP_CONFIG:
        return(load_object_config_file(OBJECT_ID_SKEL));

    case T5_MSG_IC__SERVICES_EXIT2:
        return(resource_close(OBJECT_ID_SKEL));

    /* initialise skeleton bits of new document */
    case T5_MSG_IC__INIT1:
        {
        p_docu->caret_height = 0;
        p_docu->caret.pixit_point.x = 0;
        p_docu->caret.pixit_point.y = 0;
        p_docu->caret.page_num.x = 0;
        p_docu->caret.page_num.y = 0;

        p_docu->focus_owner = OBJECT_ID_SKEL;
        return(STATUS_OK);
        }

    case T5_MSG_IC__CLOSE2:
        al_array_dispose(&p_docu->numforms);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}


T5_CMD_PROTO(static, t5_cmd_auto_save)
{
    S32 auto_save_period_minutes = 0;

    IGNOREPARM_InVal_(t5_message);

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

        if(arg_is_present(p_args, 0))
            auto_save_period_minutes = p_args[0].val.s32;
    }

    return(auto_save(p_docu, auto_save_period_minutes));
}

T5_CMD_PROTO(static, t5_cmd_trace)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

#if TRACE_ALLOWED
    if(p_args[0].val.s32)
        trace_on();
    else
        trace_off();
#else
    IGNOREPARM_CONST(p_args);
#endif

    return(STATUS_OK);
}

_Check_return_
static STATUS
t5_msg_set_tab(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message)
{
    assert_EQ((T5_MSG_SET_TAB_CENTRE  - T5_MSG_SET_TAB_LEFT), (TAB_CENTRE  - TAB_LEFT));
    assert_EQ((T5_MSG_SET_TAB_RIGHT   - T5_MSG_SET_TAB_LEFT), (TAB_RIGHT   - TAB_LEFT));
    assert_EQ((T5_MSG_SET_TAB_DECIMAL - T5_MSG_SET_TAB_LEFT), (TAB_DECIMAL - TAB_LEFT));
    p_docu->insert_tab_type = (TAB_TYPE) (TAB_LEFT + (t5_message - T5_MSG_SET_TAB_LEFT));

    return(proc_event_sk_cont_direct(p_docu, T5_MSG_SET_TAB_LEFT, P_DATA_NONE)); /* get sk_cont to encode tabs in toolbar */
}

T5_MSG_PROTO(static, t5_msg_skelcmd_view_destroy, P_SKELCMD_CLOSE_VIEW p_skelcmd_close_view)
{
    DOCNO docno = docno_from_p_docu(p_docu);

    IGNOREPARM_InVal_(t5_message);

    view_destroy(p_docu, p_skelcmd_close_view->p_view);

    if(!p_docu->n_views)
        docno_close(&docno);

    return(STATUS_OK);
}

static TCHARZ
error_from_tstr_buffer[BUF_MAX_ERRORSTRING];

_Check_return_
extern STATUS
create_error_from_tstr(
    _In_z_      PCTSTR tstr)
{
    assert(CH_NULL == error_from_tstr_buffer[0]);
    tstr_xstrkpy(error_from_tstr_buffer, elemof32(error_from_tstr_buffer), tstr);
    return(STATUS_ERROR_RQ);
}

T5_MSG_PROTO(static, skel_error_get, _InoutRef_ P_MSG_ERROR_RQ p_msg_error_rq)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    if(CH_NULL == error_from_tstr_buffer[0])
    {   /* no error to be returned */
        assert0();
        return(STATUS_FAIL);
    }

    tstr_xstrkpy(p_msg_error_rq->tstr_buf, p_msg_error_rq->elemof_buffer, error_from_tstr_buffer);
    return(STATUS_OK);
}

_Check_return_
_Ret_valid_
extern P_NUMFORM_CONTEXT
get_p_numform_context(
    _DocuRef_   PC_DOCU p_docu)
{
    if(!IS_DOCU_NONE(p_docu) && (NULL != p_docu->p_numform_context))
        return(p_docu->p_numform_context);

    return(p_docu_from_config()->p_numform_context);
}

T5_CMD_PROTO(static, t5_cmd_numform_data)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    P_USTR ustr_arg_string = p_args[0].val.ustr_wr;
    P_NUMFORM_CONTEXT p_numform_context = p_docu->p_numform_context;
    STATUS status = STATUS_OK;

    IGNOREPARM_InVal_(t5_message);

    if(NULL == p_numform_context)
    {
        if(NULL == (p_numform_context = p_docu->p_numform_context = (P_NUMFORM_CONTEXT)
            alloc_block_malloc(&p_docu->general_string_alloc_block, sizeof32(*p_numform_context), &status)))
        {
            return(status);
        }

        zero_struct_ptr(p_numform_context);
    }

    if(NULL != ustr_arg_string)
    {
        /* take a copy of this string - we decompose it later on here and form pointers into it */
        P_USTR ustr = NULL;
        if(status_ok(status = alloc_block_ustr_set(&ustr, ustr_arg_string, &p_docu->general_string_alloc_block)))
            ustr_arg_string = ustr;
    }

    if(status_ok(status) && (NULL != ustr_arg_string))
    {
        U32 month;

        for(month = 0; month < elemof32(p_numform_context->month_names); ++month)
        {
            P_USTR ustr;

            /* arg is string of space-separated strings */
            if(NULL != (ustr = ustrchr(ustr_arg_string, CH_SPACE)))
            {
                PtrPutByte(ustr, CH_NULL); ustr_IncByte_wr(ustr);
            }

            p_numform_context->month_names[month] = ustr_arg_string;

            ustr_arg_string = ustr;

            /* last space-separated arg reached? */
            if(NULL == ustr_arg_string)
                break;
        }
    }

    if(status_ok(status) && (NULL != ustr_arg_string))
    {
        U32 day;

        for(day = 0; day < elemof32(p_numform_context->day_names); ++day)
        {
            P_USTR ustr;

            /* arg is string of space-separated strings */
            if(NULL != (ustr = ustrchr(ustr_arg_string, CH_SPACE)))
            {
                PtrPutByte(ustr, CH_NULL); ustr_IncByte_wr(ustr);
            }

            p_numform_context->day_names[day] = ustr_arg_string;

            ustr_arg_string = ustr;

            /* last space-separated arg reached? */
            if(NULL == ustr_arg_string)
                break;
        }
    }

    if(status_ok(status) && (NULL != ustr_arg_string))
    {
        S32 day;

        for(day = 0; day < elemof32(p_numform_context->day_endings); ++day)
        {
            P_USTR ustr;

            /* arg is string of space-separated strings */
            if(NULL != (ustr = ustrchr(ustr_arg_string, CH_SPACE)))
            {
                PtrPutByte(ustr, CH_NULL); ustr_IncByte_wr(ustr);
            }

            p_numform_context->day_endings[day] = ustr_arg_string;

            ustr_arg_string = ustr;

            /* last space-separated arg reached? */
            if(NULL == ustr_arg_string)
                break;
        }
    }

    return(status);
}

/* copy into list of UI numforms */

T5_CMD_PROTO(static, t5_cmd_numform_load)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 3);
    UI_NUMFORM_CLASS c_numform = (UI_NUMFORM_CLASS) p_args[0].val.s32;
    P_USTR ustr_numform_arg = NULL;
    P_USTR ustr_numform_arg_opt = NULL;
    STATUS status = STATUS_OK;

    IGNOREPARM_InVal_(t5_message);

    if(arg_is_present(p_args, 1))
        status = alloc_block_ustr_set(&ustr_numform_arg, p_args[1].val.ustr, &p_docu->general_string_alloc_block);

    if(status_ok(status))
        if(arg_is_present(p_args, 2))
            status = alloc_block_ustr_set(&ustr_numform_arg_opt, p_args[2].val.ustr, &p_docu->general_string_alloc_block);

    if(status_ok(status))
    {
        P_UI_NUMFORM p_ui_numform;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(*p_ui_numform), 0);

        if(NULL != (p_ui_numform = al_array_extend_by(&p_docu->numforms, UI_NUMFORM, 1, &array_init_block, &status)))
        {
            p_ui_numform->numform_class = c_numform;
            p_ui_numform->ustr_numform = ustr_numform_arg;
            p_ui_numform->ustr_opt = ustr_numform_arg_opt;
        }
    }

    return(status);
}

T5_CMD_PROTO(static, t5_cmd_ss_context)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 10);

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    g_ss_recog_context_alt.thousands_char           = PtrGetByte(p_args[0].val.ustr);
    g_ss_recog_context_alt.decimal_point_char       = PtrGetByte(p_args[1].val.ustr);
    g_ss_recog_context_alt.date_sep_char            = PtrGetByte(p_args[2].val.ustr);
    g_ss_recog_context_alt.time_sep_char            = PtrGetByte(p_args[3].val.ustr);
    g_ss_recog_context_alt.array_col_sep            = PtrGetByte(p_args[4].val.ustr);
    g_ss_recog_context_alt.array_row_sep            = PtrGetByte(p_args[5].val.ustr);
    g_ss_recog_context_alt.list_sep_char            = PtrGetByte(p_args[6].val.ustr);
    g_ss_recog_context_alt.function_arg_sep         = PtrGetByte(p_args[7].val.ustr);
    g_ss_recog_context_alt.alternate_function_flag  = (U8) (CH_DIGIT_ZERO != PtrGetByte(p_args[8].val.ustr));

    if(arg_is_present(p_args, 9))
    g_ss_recog_context_alt.alternate_date_sep_char  = PtrGetByte(p_args[9].val.ustr);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, t5_msg_insert_ownform, P_MSG_INSERT_OWNFORM p_msg_insert_ownform)
{
    OF_IP_FORMAT of_ip_format = OF_IP_FORMAT_INIT;
    STATUS status;

    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_insert_ownform->t5_filetype)
    {
    default: default_unhandled();
#if CHECKING
    case FILETYPE_T5_FIREWORKZ:
    case FILETYPE_T5_WORDZ:
    case FILETYPE_T5_RESULTZ:
    case FILETYPE_T5_RECORDZ:
#endif
        break;

    case FILETYPE_T5_TEMPLATE:
    case FILETYPE_T5_COMMAND:
        of_ip_format.flags.is_template = 1;
        break;
    }

    of_ip_format.flags.insert = p_msg_insert_ownform->insert;

    of_ip_format.process_status.flags.foreground = 1;

    status_return(ownform_initialise_load(p_docu, &p_msg_insert_ownform->position, &of_ip_format, p_msg_insert_ownform->filename, &p_msg_insert_ownform->array_handle));

    status = load_ownform_file(&of_ip_format);

    status_accumulate(status, ownform_finalise_load(p_docu, &of_ip_format));

    docu_modify(p_docu);

    return(status);
}

T5_MSG_PROTO(static, t5_msg_load_construct_ownform, P_CONSTRUCT_CONVERT p_construct_convert)
{
    if(OBJECT_ID_NONE != p_construct_convert->p_of_ip_format->directed_object_id)
        return(object_call_id(p_construct_convert->p_of_ip_format->directed_object_id, p_docu, t5_message, p_construct_convert));

    return(skeleton_load_construct(p_docu, p_construct_convert));
}

T5_MSG_PROTO(static, t5_msg_load_ended, P_OF_IP_FORMAT p_of_ip_format)
{
    ARRAY_INDEX i;

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    /* throw away renamed style info */
    for(i = 0; i < array_elements(&p_of_ip_format->renamed_styles); ++i)
    {
        P_STYLE_LOAD_MAP p_style_load_map = array_ptr(&p_of_ip_format->renamed_styles, STYLE_LOAD_MAP, i);

        al_array_dispose(&p_style_load_map->h_new_name_tstr);
        al_array_dispose(&p_style_load_map->h_old_name_tstr);
    }

    al_array_dispose(&p_of_ip_format->renamed_styles);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, t5_cmd_object_ensure)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    const OBJECT_ID object_id = p_args[0].val.object_id;

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    return(object_load(object_id));
}

T5_CMD_PROTO(static, t5_cmd_selection_make)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, ARG_DOCU_AREA_N_ARGS);
    DOCU_AREA docu_area;

    IGNOREPARM_InVal_(t5_message);

    docu_area_init(&docu_area);

    if(arg_is_present(p_args, ARG_DOCU_AREA_TL_COL))
        docu_area.tl.slr.col = p_args[ARG_DOCU_AREA_TL_COL].val.col;
    if(arg_is_present(p_args, ARG_DOCU_AREA_TL_ROW))
        docu_area.tl.slr.row = p_args[ARG_DOCU_AREA_TL_ROW].val.row;
    if(arg_is_present(p_args, ARG_DOCU_AREA_TL_DATA))
        docu_area.tl.object_position.data = p_args[ARG_DOCU_AREA_TL_DATA].val.s32;
    if(arg_is_present(p_args, ARG_DOCU_AREA_TL_OBJECT_ID))
        docu_area.tl.object_position.object_id = p_args[ARG_DOCU_AREA_TL_OBJECT_ID].val.object_id;

    if(arg_is_present(p_args, ARG_DOCU_AREA_BR_COL))
        docu_area.br.slr.col = p_args[ARG_DOCU_AREA_BR_COL].val.col;
    if(arg_is_present(p_args, ARG_DOCU_AREA_BR_ROW))
        docu_area.br.slr.row = p_args[ARG_DOCU_AREA_BR_ROW].val.row;
    if(arg_is_present(p_args, ARG_DOCU_AREA_BR_DATA))
        docu_area.br.object_position.data = p_args[ARG_DOCU_AREA_BR_DATA].val.s32;
    if(arg_is_present(p_args, ARG_DOCU_AREA_BR_OBJECT_ID))
        docu_area.br.object_position.object_id = p_args[ARG_DOCU_AREA_BR_OBJECT_ID].val.object_id;

    if(arg_is_present(p_args, ARG_DOCU_AREA_WHOLE_COL))
        docu_area.whole_col = p_args[ARG_DOCU_AREA_WHOLE_COL].val.u8n;
    if(arg_is_present(p_args, ARG_DOCU_AREA_WHOLE_ROW))
        docu_area.whole_row = p_args[ARG_DOCU_AREA_WHOLE_ROW].val.u8n;

    return(object_skel(p_docu, T5_MSG_SELECTION_MAKE, &docu_area));
}

T5_CMD_PROTO(static, t5_cmd_nyi)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    return(create_error(ERR_NYI));
}

/******************************************************************************
*
* this is the top level event handler for the skeleton,
* typically receiver of commands and events from the user interface
*
******************************************************************************/

/* many events are vectored to flow object */

PROC_EVENT_PROTO(extern, docu_flow_object_call)
{
    if(IS_DOCU_NONE(p_docu) || !p_docu->flags.flow_installed)
    {
        trace_1(TRACE_APP_SKEL, TEXT("sk_root resolve flow object discarded message: ") S32_TFMT, t5_message);
        return(STATUS_OK);
    }

    return(object_call_id(p_docu->object_id_flow, p_docu, t5_message, p_data));
}

/* many events are vectored to input focus object */

PROC_EVENT_PROTO(extern, docu_focus_owner_object_call)
{
    STATUS status;

    if(OBJECT_ID_SKEL == p_docu->focus_owner)
        return(STATUS_FAIL);

    status = object_call_id(p_docu->focus_owner, p_docu, t5_message, p_data);

    status_assert(maeve_event(p_docu, T5_MSG_AFTER_SKEL_COMMAND, P_DATA_NONE));

    return(status);
}

/* slightly similar dispatch as docu_focus_owner_object_call but different enough */

T5_MSG_PROTO(static, skel_msg_caret_show_claim, P_CARET_SHOW_CLAIM p_caret_show_claim)
{
    OBJECT_ID focus_old = p_docu->focus_owner;
    BOOL focus_changed = (p_docu->focus_owner != p_caret_show_claim->focus);
    STATUS status;

    p_docu->focus_owner = p_caret_show_claim->focus;

    if(OBJECT_ID_SKEL == p_docu->focus_owner)
    {
        SKEL_POINT skel_point;

        skel_point.pixit_point.x =
        skel_point.pixit_point.y = 0;
        skel_point.page_num.x =
        skel_point.page_num.y = 0;

        view_show_caret(p_docu, UPDATE_PANE_CELLS_AREA, &skel_point, 0);
        status = STATUS_FAIL;
    }
    else
    {
        status = object_call_id(p_docu->focus_owner, p_docu, t5_message, (P_ANY) p_caret_show_claim);
    }

    if(focus_changed)
        status_assert(maeve_event(p_docu, T5_MSG_FOCUS_CHANGED, &focus_old));

    return(status);
}

OBJECT_PROTO(extern, object_skel)
{
    switch(t5_message)
    {
    default:
        return(docu_flow_object_call(p_docu, t5_message, p_data));

    /* many events are vectored to input focus object */

    case T5_EVENT_KEYS:
    case T5_EVENT_CLICK_DRAG_ABORTED:

    case T5_MSG_MOVE_LEFT_CELL:
    case T5_MSG_MOVE_RIGHT_CELL:
    case T5_MSG_MOVE_UP_CELL:
    case T5_MSG_MOVE_DOWN_CELL:

    case T5_MSG_CHECK_PROTECTION:
    case T5_MSG_MARK_INFO_READ:
    case T5_MSG_OBJECT_CHECK:
    case T5_MSG_RULER_INFO:
    case T5_MSG_SELECTION_STYLE:
    case T5_MSG_SELECTION_MAKE:

#if 0 /* commands handled with fo dispatch */
    case T5_CMD_SELECT_WORD:
    case T5_CMD_SELECT_CELL:
    case T5_CMD_SELECT_DOCUMENT:
    case T5_CMD_TOGGLE_MARKS:
    case T5_CMD_SELECTION_CLEAR:
    case T5_CMD_SELECTION_COPY:
    case T5_CMD_SELECTION_CUT:
    case T5_CMD_SELECTION_DELETE:
    case T5_CMD_PASTE_AT_CURSOR:

    case T5_CMD_SETC_UPPER:
    case T5_CMD_SETC_LOWER:
    case T5_CMD_SETC_INICAP:
    case T5_CMD_SETC_SWAP:

    case T5_CMD_DELETE_CHARACTER_LEFT:
    case T5_CMD_DELETE_CHARACTER_RIGHT:
    case T5_CMD_DELETE_LINE:

    case T5_CMD_CURSOR_LEFT:
    case T5_CMD_CURSOR_RIGHT:
    case T5_CMD_CURSOR_UP:
    case T5_CMD_CURSOR_DOWN:

    case T5_CMD_SHIFT_CURSOR_LEFT:
    case T5_CMD_SHIFT_CURSOR_RIGHT:
    case T5_CMD_SHIFT_CURSOR_UP:
    case T5_CMD_SHIFT_CURSOR_DOWN:

    case T5_CMD_WORD_LEFT:
    case T5_CMD_WORD_RIGHT:
    case T5_CMD_SHIFT_WORD_LEFT:
    case T5_CMD_SHIFT_WORD_RIGHT:

    case T5_CMD_PAGE_UP:
    case T5_CMD_PAGE_DOWN:
    case T5_CMD_SHIFT_PAGE_UP:
    case T5_CMD_SHIFT_PAGE_DOWN:

    case T5_CMD_LINE_START:
    case T5_CMD_LINE_END:
    case T5_CMD_SHIFT_LINE_START:
    case T5_CMD_SHIFT_LINE_END:

    case T5_CMD_DOCUMENT_BOTTOM:
    case T5_CMD_DOCUMENT_TOP:
    case T5_CMD_SHIFT_DOCUMENT_BOTTOM:
    case T5_CMD_SHIFT_DOCUMENT_TOP:

    case T5_CMD_TAB_LEFT:
    case T5_CMD_TAB_RIGHT:

    case T5_CMD_RETURN:
    case T5_CMD_ESCAPE:

    case T5_CMD_BOX:
    case T5_CMD_STYLE_APPLY:
    case T5_CMD_STYLE_APPLY_SOURCE:

    case T5_CMD_FORCE_RECALC:
    case T5_CMD_SEARCH:
    case T5_CMD_SNAPSHOT:

    case T5_CMD_INSERT_FIELD_DATE:
    case T5_CMD_INSERT_FIELD_FILE_DATE:
    case T5_CMD_INSERT_FIELD_PAGE_X:
    case T5_CMD_INSERT_FIELD_PAGE_Y:
    case T5_CMD_INSERT_FIELD_MS_FIELD:
    case T5_CMD_INSERT_FIELD_SS_NAME:
    case T5_CMD_INSERT_FIELD_WHOLENAME:
    case T5_CMD_INSERT_FIELD_LEAFNAME:
    case T5_CMD_INSERT_FIELD_RETURN:
    case T5_CMD_INSERT_FIELD_SOFT_HYPHEN:
#endif

    /*case T5_CMD_INSERT_FIELD_HARD_HYPHEN:*/

    case T5_CMD_WORD_COUNT:
        return(docu_focus_owner_object_call(p_docu, t5_message, p_data));

    case T5_MSG_INITCLOSE:
        return(skel_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_ERROR_RQ:
        return(skel_error_get(p_docu, t5_message, (P_MSG_ERROR_RQ) p_data));

    case T5_MSG_SKELCMD_VIEW_DESTROY:
        return(t5_msg_skelcmd_view_destroy(p_docu, t5_message, (P_SKELCMD_CLOSE_VIEW) p_data));

    case T5_MSG_CARET_SHOW_CLAIM:
        return(skel_msg_caret_show_claim(p_docu, t5_message, (P_CARET_SHOW_CLAIM) p_data));

    case T5_MSG_INSERT_OWNFORM:
        return(t5_msg_insert_ownform(p_docu, t5_message, (P_MSG_INSERT_OWNFORM) p_data));

    case T5_MSG_LOAD_CONSTRUCT_OWNFORM:
        return(t5_msg_load_construct_ownform(p_docu, t5_message, (P_CONSTRUCT_CONVERT) p_data));

    case T5_MSG_SAVE_CONSTRUCT_OWNFORM:
        return(skeleton_save_construct(p_docu, (P_SAVE_CONSTRUCT_OWNFORM) p_data));

    case T5_MSG_LOAD_ENDED:
        return(t5_msg_load_ended(p_docu, t5_message, (P_OF_IP_FORMAT) p_data));

    case T5_MSG_DRAFT_PRINT:
        return(t5_msg_draft_print(p_docu, t5_message, (P_PRINT_CTRL) p_data));

    case T5_MSG_DRAFT_PRINT_TO_FILE:
        return(t5_msg_draft_print_to_file(p_docu, t5_message, (P_DRAFT_PRINT_TO_FILE) p_data));

    case T5_CMD_OBJECT_ENSURE:
        return(t5_cmd_object_ensure(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SELECTION_MAKE:
        return(t5_cmd_selection_make(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_MSG_STYLE_CHANGE_BOLD:
    case T5_MSG_STYLE_CHANGE_ITALIC:
    case T5_MSG_STYLE_CHANGE_UNDERLINE:
    case T5_MSG_STYLE_CHANGE_SUPERSCRIPT:
    case T5_MSG_STYLE_CHANGE_SUBSCRIPT:
    case T5_MSG_STYLE_CHANGE_JUSTIFY_LEFT:
    case T5_MSG_STYLE_CHANGE_JUSTIFY_CENTRE:
    case T5_MSG_STYLE_CHANGE_JUSTIFY_RIGHT:
    case T5_MSG_STYLE_CHANGE_JUSTIFY_FULL:
    /*case T5_MSG_STATUS_LINE_MESSAGE_QUERY:*/
        return(proc_event_sk_cont_direct(p_docu, t5_message, p_data));

    case T5_MSG_SET_TAB_LEFT:
    case T5_MSG_SET_TAB_CENTRE:
    case T5_MSG_SET_TAB_RIGHT:
    case T5_MSG_SET_TAB_DECIMAL:
        return(t5_msg_set_tab(p_docu, t5_message));

    /*********************************************************************************************/

    case T5_CMD_QUIT:
        return(t5_cmd_quit(p_docu));

    case T5_CMD_INFO:
        return(t5_cmd_info(p_docu));

    case T5_CMD_CTYPETABLE:
        return(t5_cmd_ctypetable(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_NUMFORM_LOAD:
        return(t5_cmd_numform_load(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_NUMFORM_DATA:
        return(t5_cmd_numform_data(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SS_CONTEXT:
        return(t5_cmd_ss_context(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SET_INTERACTIVE:
        return(t5_cmd_set_interactive(/*p_docu, p_data*/));

    case T5_CMD_INSERT_FIELD_INTRO_DATE:
    case T5_CMD_INSERT_FIELD_INTRO_TIME:
        return(t5_cmd_insert_field_intro_date(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_INSERT_FIELD_INTRO_PAGE:
        return(t5_cmd_insert_field_intro_page(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_BACKDROP: /* make/adjust backdrop */
    case T5_CMD_BACKDROP_INTRO: /* make/adjust backdrop */
    case T5_CMD_NOTE:
    case T5_CMD_NOTE_TWIN:      /* compatibility with older readers */
    case T5_CMD_NOTE_BACKDROP:  /* different so that backdrops can be rejected on file insert 'cos they aren't cell relative */
    case T5_CMD_NOTE_BACK:
    case T5_CMD_NOTE_SWAP:
    case T5_CMD_NOTE_EMBED:
        return(object_call_id_load(p_docu, t5_message, p_data, OBJECT_ID_NOTE));

    case T5_CMD_BOX_INTRO:
    case T5_CMD_STYLE_BUTTON:
    case T5_CMD_STYLE_FOR_CONFIG:
    case T5_CMD_EFFECTS_BUTTON:
    case T5_CMD_STYLE_REGION_EDIT:
    case T5_MSG_BOX_APPLY:
        return(object_call_id_load(p_docu, t5_message, p_data, OBJECT_ID_SKEL_SPLIT));

    case T5_CMD_BUTTON:
        return(object_call_id(OBJECT_ID_TOOLBAR, p_docu, t5_message, p_data));

    case T5_CMD_CURRENT_DOCUMENT:
        return(t5_cmd_current_document(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CURRENT_POSITION:
        return(t5_cmd_current_position(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_AUTO_SAVE:
        return(t5_cmd_auto_save(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES_AUTO_SAVE:
        return(t5_cmd_choices_auto_save(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES_DISPLAY_PICTURES:
        return(t5_cmd_choices_display_pictures(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES_EMBED_INSERTED_FILES:
        return(t5_cmd_choices_embed_inserted_files(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES_KERNING:
        return(t5_cmd_choices_kerning(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES_DITHERING:
        return(t5_cmd_choices_dithering(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES_STATUS_LINE:
        return(t5_cmd_choices_status_line(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES_TOOLBAR:
        return(t5_cmd_choices_toolbar(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES_UPDATE_STYLES_FROM_CHOICES:
        return(t5_cmd_choices_update_styles_from_choices(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES_ASCII_LOAD_AS_DELIMITED:
        return(t5_cmd_choices_ascii_load_as_delimited(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES_ASCII_LOAD_DELIMITER:
        return(t5_cmd_choices_ascii_load_delimiter(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES_ASCII_LOAD_AS_PARAGRAPHS:
        return(t5_cmd_choices_ascii_load_as_paragraphs(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES:
        return(t5_cmd_choices(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_FONTMAP:
        return(t5_cmd_fontmap(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_FONTMAP_END:
        return(t5_cmd_fontmap_end(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_FONTMAP_REMAP:
        return(t5_cmd_fontmap_remap(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_MENU_ADD:
    case T5_CMD_MENU_ADD_RAW:
        return(t5_cmd_menu_add(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_MENU_DELETE:
        return(t5_cmd_menu_delete(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_MENU_NAME:
        return(t5_cmd_menu_name(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_DEFINE_KEY:
    case T5_CMD_DEFINE_KEY_RAW:
        return(t5_cmd_define_key(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_PAPER:
        return(t5_cmd_paper(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_PAPER_INTRO:
        return(t5_cmd_paper_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_PAPER_SCALE:
        return(t5_cmd_paper_scale(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_PAPER_SCALE_INTRO:
        return(t5_cmd_paper_scale_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_PRINT_INTRO:
    case T5_CMD_PRINT_EXTRA_INTRO:
        return(t5_cmd_print_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_PRINT:
    case T5_CMD_PRINT_EXTRA:
        return(t5_cmd_print(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_PRINT_QUALITY:
        return(t5_cmd_print_quality(p_docu, t5_message, (PC_T5_CMD) p_data));

#if WINDOWS
    case T5_CMD_PRINT_SETUP:
        return(t5_cmd_print_setup(p_docu, t5_message, (PC_T5_CMD) p_data));
#endif

    case T5_CMD_OBJECT_BIND_SAVER:
        return(t5_cmd_object_bind_saver(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SAVE_OWNFORM:
        return(t5_cmd_save_ownform(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SAVE_OWNFORM_AS_INTRO:
        return(t5_cmd_save_ownform_as_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SAVE_OWNFORM_AS:
        return(t5_cmd_save_ownform_as(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SAVE_TEMPLATE_INTRO:
        return(t5_cmd_save_template_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SAVE_TEMPLATE:
        return(t5_cmd_save_template(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SAVE_FOREIGN_INTRO:
        return(t5_cmd_save_foreign_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SAVE_FOREIGN:
        return(t5_cmd_save_foreign(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SAVE_PICTURE_INTRO:
        return(t5_cmd_save_picture_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SAVE_CLIPBOARD:
        return(t5_cmd_save_clipboard(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_LOAD:
        return(t5_cmd_load(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_LOAD_TEMPLATE:
        return(t5_cmd_load_template(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_LOAD_FOREIGN:
        return(t5_cmd_load_foreign(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_BIND_FILE_TYPE:
        return(t5_cmd_bind_file_type(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_OBJECT_BIND_CONSTRUCT:
        return(t5_cmd_object_bind_construct(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_OBJECT_BIND_LOADER:
        return(t5_cmd_object_bind_loader(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_VIEW_NEW:
        return(t5_cmd_view_new(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_VIEW_CONTROL_INTRO:
        return(t5_cmd_view_control_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_VIEW_CONTROL:
        return(t5_cmd_view_control(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_VIEW_CREATE:
        return(t5_cmd_view_create(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_VIEW_SIZE:
        return(t5_cmd_view_size(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_VIEW_POSN:
        return(t5_cmd_view_posn(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_VIEW_MAXIMIZE:
        return(t5_cmd_view_maximize(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_VIEW_MINIMIZE:
        return(t5_cmd_view_minimize(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_VIEW_SCROLL:
        return(t5_cmd_view_scroll(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_VIEW_CLOSE_REQ:
        return(t5_cmd_view_close_req(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CURRENT_VIEW:
        return(t5_cmd_current_view(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CURRENT_PANE:
        return(t5_cmd_current_pane(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_RULER_SCALE:
    case T5_CMD_RULER_SCALE_V:
        return(t5_cmd_ruler_scale(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_INSERT_PAGE_BREAK:
        return(t5_cmd_insert_page_break(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_HELP_CONTENTS:
    case T5_CMD_HELP_SEARCH_KEYWORD:
    case T5_CMD_HELP_URL:
        return(t5_cmd_help(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_NYI:
        return(t5_cmd_nyi(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_TRACE:
        return(t5_cmd_trace(p_docu, t5_message, (PC_T5_CMD) p_data));

#if defined(UNUSED) || 0
    case T5_CMD_TEST:
        return(t5_cmd_test(p_docu, t5_message, (PC_T5_CMD) p_data));
#endif
    }
}

extern void
caret_show_claim(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id,
    _InVal_     BOOL scroll)
{
    CARET_SHOW_CLAIM caret_show_claim;
    caret_show_claim.focus = object_id;
    caret_show_claim.scroll = scroll;
    status_consume(object_skel(p_docu, T5_MSG_CARET_SHOW_CLAIM, &caret_show_claim));
}

extern void
cur_change_after(
    _DocuRef_   P_DOCU p_docu)
{
    status_assert(maeve_event(p_docu, T5_MSG_CUR_CHANGE_AFTER, P_DATA_NONE));
}

extern void
cur_change_before(
    _DocuRef_   P_DOCU p_docu)
{
    status_assert(maeve_event(p_docu, T5_MSG_CUR_CHANGE_BEFORE, P_DATA_NONE));
}

/******************************************************************************
*
* report error message
*
******************************************************************************/

BOOL g_silent_shutdown = FALSE;

/*ncr*/
extern STATUS __cdecl
reperr(
    _InVal_     STATUS status,
    _In_opt_z_  PCTSTR text,
    /**/        ...)
{
#if RISCOS
    _kernel_oserror err;
#define output err.errmess
#else
    TCHARZ output[BUF_MAX_ERRORSTRING];
#endif
    TCHARZ lookup[BUF_MAX_ERRORSTRING];
    va_list args;

    assert(status_fail(status));

    if(status_ok(status))
        return(status);

    if(STATUS_CANCEL == status)
        return(status);

#if !TRACE_ALLOWED
    if(STATUS_FAIL == status)
        /* leave no trace of major accidents in release code */
        return(status);
#endif

    trace_2(0, TEXT("reperr(") S32_TFMT TEXT(", %s)"), status, report_tstr(text));

    if(status == ERR_OUTPUT_STRING)
    {
        assert(!IS_PTR_NULL_OR_NONE(text));

        tstr_xstrkpy(output, elemof32(output), text);
    }
    else if(status == ERR_OUTPUT_SPRINTF)
    {
        assert(!IS_PTR_NULL_OR_NONE(text));

        va_start(args, text);

        consume_int(tstr_xvsnprintf(output, elemof32(output), text, args));

        va_end(args);
    }
    else
    {
        U16 status_offset;
        OBJECT_ID object_id;

        consume_bool(resource_split_status(status, &status_offset, &object_id));

        assert(IS_OBJECT_ID_VALID(object_id));

        lookup[0] = CH_NULL;
        output[0] = CH_NULL;

        if(status_offset == -ERR_OFFSET_ERROR_RQ)
        {
            MSG_ERROR_RQ event_data;

            event_data.tstr_buf = lookup;
            event_data.elemof_buffer = elemof32(lookup);
            event_data.status = STATUS_OK;

            status_consume(object_call_id(object_id, P_DOCU_NONE, T5_MSG_ERROR_RQ, &event_data));

            if(event_data.status != STATUS_OK)
            {
                resource_lookup_tstr_buffer(output, elemof32(output), event_data.status);
                tstr_xstrkat(output, elemof32(output), TEXT(" "));
                tstr_xstrkat(output, elemof32(output), lookup);
            }
            else if(CH_NULL != event_data.tstr_buf[0])
            {
                tstr_xstrkpy(output, elemof32(output), lookup);
            }
            else
            {
                assert0();
                return(status);
            }

            if(!IS_PTR_NULL_OR_NONE(text))
            {
                if((NULL == tstrstr(output, text)) || !file_is_rooted(text)) /* don't repeat ourselves on looked-up filing system errors */
                {
                    tstr_xstrkat(output, elemof32(output), TEXT(" "));
                    tstr_xstrkat(output, elemof32(output), text);
                }
            }
        }
        else
        {
            resource_lookup_tstr_buffer(lookup, elemof32(lookup), status);

            if(NULL == tstrstr(lookup, TEXT("%s")))
            {
                tstr_xstrkpy(output, elemof32(output), lookup);

                /* if we have an arg then append it if not a %s error */
                if(!IS_PTR_NULL_OR_NONE(text))
                {
                    tstr_xstrkat(output, elemof32(output), TEXT(" "));
                    tstr_xstrkat(output, elemof32(output), text);
                }
            }
            else
            {
                /* ensure %s error messages don't look too bad */
                if(IS_PTR_NULL_OR_NONE(text))
                    text = tstr_empty_string;

                /* unusual start due to text being passed too */
                va_start(args, status);

                consume_int(tstr_xvsnprintf(output, elemof32(output), lookup, args));

                va_end(args);
            }
        }
    }

    report_output(output);

#if RISCOS
    if(!g_silent_shutdown)
    {
        err.errnum = 0;
        consume(_kernel_oserror *, wimp_reporterror_simple(&err));
    }
#undef output
#elif WINDOWS
    if(!g_silent_shutdown)
    {
        (void) MessageBox(NULL, output, product_ui_id(), MB_TASKMODAL | MB_OK | MB_ICONEXCLAMATION);
    }
#else
    fputs(stderr, output);
    fputc('\n', stderr);
#endif /* OS */

    assert(CH_NULL == error_from_tstr_buffer[0]);
    error_from_tstr_buffer[0] = CH_NULL; /* reset in case it was set and then ignored */

    return(status);
}

extern void __cdecl
messagef(
    _In_z_      PCTSTR text,
    /**/        ...)
{
#if RISCOS
    _kernel_oserror err;
#define output err.errmess
#else
    TCHARZ output[BUF_MAX_ERRORSTRING];
#endif
    va_list args;

    assert(!IS_PTR_NULL_OR_NONE(text));

    va_start(args, text);

    consume_int(tstr_xvsnprintf(output, elemof32(output), text, args));

    va_end(args);

#if RISCOS
    if(g_silent_shutdown)
    {
        report_output(output);
    }
    else
    {
        err.errnum = 0;
        consume(_kernel_oserror *, wimp_reporterror_simple(&err));
    }
#undef output
#elif WINDOWS
    if(g_silent_shutdown)
    {
        report_output(output);
    }
    else
    {
        (void) MessageBox(NULL, output, product_ui_id(), MB_TASKMODAL | MB_OK);
    }
#endif
}

#ifdef create_error
#undef create_error
#endif

_Check_return_
extern STATUS
create_error(
    _InVal_     STATUS status)
{
    if(status_ok(status))
        return(status);
    status_assertc(status);
    return(status);
}

#ifdef status_nomem
#undef status_nomem
#endif

_Check_return_
extern STATUS
status_nomem(void)
{
    STATUS status = STATUS_NOMEM;
    status_assertc(status);
    return(status);
}

#ifdef status_check
#undef status_check
#endif

_Check_return_
extern STATUS
status_check(void)
{
    STATUS status = STATUS_CHECK;
    status_assertc(status);
    return(status);
}

#if defined(UNUSED) || 0

T5_CMD_PROTO(static, t5_cmd_test)
{
    STATUS status = STATUS_OK;
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);
    return(status);
}

#endif /* UNUSED */

/* end of sk_root.c */
